#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "paging.h"
#include "stdmem.h"

#define MAX_PAGES 4096
#define PAGE_SIZE 4096

// Statically allocate space for PML4 (top level page table)
static __attribute__((aligned(PAGE_SIZE_4K))) page_table pml4;

// Function to get indexes into page tables for a given virtual address
static inline unsigned int pml4_index(uint64_t addr) { return (addr >> 39) & 0x1FF; }
static inline unsigned int pdpt_index(uint64_t addr) { return (addr >> 30) & 0x1FF; }
static inline unsigned int pd_index(uint64_t addr)   { return (addr >> 21) & 0x1FF; }
static inline unsigned int pt_index(uint64_t addr)   { return (addr >> 12) & 0x1FF; }
static uint8_t page_pool[MAX_PAGES * PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

// Free list structure
static void* free_list[MAX_PAGES];
static size_t free_list_top = 0;

static size_t next_free_page = 0;

void* kalloc_page(void) {
    if (free_list_top > 0) {
        return free_list[--free_list_top];
    }

    if (next_free_page >= MAX_PAGES)
        return NULL;

    void* page = &page_pool[next_free_page * PAGE_SIZE];
    next_free_page++;
    return page;
}

void kfree_page(void* ptr) {
    if (!ptr) return;

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t base = (uintptr_t)page_pool;

    // Ensure the pointer is within the page pool range and page-aligned
    if (addr < base || addr >= base + MAX_PAGES * PAGE_SIZE || addr % PAGE_SIZE != 0)
        return;

    if (free_list_top < MAX_PAGES) {
        free_list[free_list_top++] = ptr;
    }
}


// Get or create a page table
static page_table* get_next_level(page_table* current, int index, bool create) {
    uint64_t entry = current->entries[index];
    
    if (!(entry & PAGE_PRESENT)) {
        if (!create) return NULL;
        
        // Allocate a new page table
        page_table* new_table = (page_table*)kalloc_page();
        if (!new_table) return NULL;
        
        // Clear the new table
        memset(new_table, 0, sizeof(page_table));
        
        // Set up the entry to point to the new table
        current->entries[index] = (uint64_t)new_table | PAGE_PRESENT | PAGE_WRITABLE;
        return new_table;
    }
    
    return (page_table*)(entry & ~0xFFF);
}

void init_paging(void) {
    // Clear PML4
    memset(&pml4, 0, sizeof(pml4));
    
    // Identity map first 1GB of physical memory (for kernel space)
    for (uint64_t addr = 0; addr < 0x40000000; addr += PAGE_SIZE_2M) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE);
    }
    
    // Map kernel to higher half (last 2GB of virtual address space)
    uint64_t kernel_phys_start = 0x100000; // Assuming kernel starts at 1MB physical
    uint64_t kernel_virt_start = 0xFFFFFFFF80000000;
    uint64_t kernel_size = 0x1000000; // Assume 16MB kernel size for now
    
    for (uint64_t offset = 0; offset < kernel_size; offset += PAGE_SIZE_4K) {
        map_page(kernel_virt_start + offset, 
                kernel_phys_start + offset,
                PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    // Load CR3 with PML4 base address
    asm volatile("mov %0, %%cr3" : : "r"(&pml4));
}

void map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    // Get indices for each level
    unsigned int pml4_idx = pml4_index(virtual_addr);
    unsigned int pdpt_idx = pdpt_index(virtual_addr);
    unsigned int pd_idx = pd_index(virtual_addr);
    unsigned int pt_idx = pt_index(virtual_addr);
    
    // Navigate/create the page table hierarchy
    page_table* pdpt = get_next_level(&pml4, pml4_idx, true);
    if (!pdpt) return;
    
    // Handle 1GB pages
    if (flags & PAGE_HUGE && (physical_addr & (PAGE_SIZE_1G - 1)) == 0) {
        pdpt->entries[pdpt_idx] = physical_addr | flags;
        return;
    }
    
    page_table* pd = get_next_level(pdpt, pdpt_idx, true);
    if (!pd) return;
    
    // Handle 2MB pages
    if (flags & PAGE_HUGE && (physical_addr & (PAGE_SIZE_2M - 1)) == 0) {
        pd->entries[pd_idx] = physical_addr | flags;
        return;
    }
    
    page_table* pt = get_next_level(pd, pd_idx, true);
    if (!pt) return;
    
    // Map 4KB page
    pt->entries[pt_idx] = physical_addr | flags;
}

void unmap_page(uint64_t virtual_addr) {
    // Get indices for each level
    unsigned int pml4_idx = pml4_index(virtual_addr);
    unsigned int pdpt_idx = pdpt_index(virtual_addr);
    unsigned int pd_idx = pd_index(virtual_addr);
    unsigned int pt_idx = pt_index(virtual_addr);
    
    // Navigate the page table hierarchy
    page_table* pdpt = get_next_level(&pml4, pml4_idx, false);
    if (!pdpt) return;
    
    page_table* pd = get_next_level(pdpt, pdpt_idx, false);
    if (!pd) return;
    
    page_table* pt = get_next_level(pd, pd_idx, false);
    if (!pt) return;
    
    // Clear the page table entry
    pt->entries[pt_idx] = 0;
    
    // Invalidate TLB for this address
    asm volatile("invlpg (%0)" : : "m"(*(volatile char *)virtual_addr) : "memory");
}

uint64_t get_physical_addr(uint64_t virtual_addr) {
    // Get indices for each level
    unsigned int pml4_idx = pml4_index(virtual_addr);
    unsigned int pdpt_idx = pdpt_index(virtual_addr);
    unsigned int pd_idx = pd_index(virtual_addr);
    unsigned int pt_idx = pt_index(virtual_addr);
    
    // Navigate the page table hierarchy
    page_table* pdpt = get_next_level(&pml4, pml4_idx, false);
    if (!pdpt) return 0;
    
    // Check for 1GB page
    uint64_t pdpt_entry = pdpt->entries[pdpt_idx];
    if (pdpt_entry & PAGE_HUGE) {
        return (pdpt_entry & ~0x3FFFFFFF) + (virtual_addr & 0x3FFFFFFF);
    }
    
    page_table* pd = get_next_level(pdpt, pdpt_idx, false);
    if (!pd) return 0;
    
    // Check for 2MB page
    uint64_t pd_entry = pd->entries[pd_idx];
    if (pd_entry & PAGE_HUGE) {
        return (pd_entry & ~0x1FFFFF) + (virtual_addr & 0x1FFFFF);
    }
    
    page_table* pt = get_next_level(pd, pd_idx, false);
    if (!pt) return 0;
    
    // Get 4KB page physical address
    uint64_t pt_entry = pt->entries[pt_idx];
    return (pt_entry & ~0xFFF) + (virtual_addr & 0xFFF);
}
