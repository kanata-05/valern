#include "gdt.h"
#include <stddef.h>
#include <stdbool.h>

// Minimal GDT structure for additional segments
struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct GDTPtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Extended GDT with TSS (assumes Limine provides first 5 entries)
struct ExtendedGDT {
    struct GDTEntry existing[5];  // Limine's entries (null, kernel code/data, user code/data)
    struct GDTEntry tss_low;      // TSS descriptor low part
    struct GDTEntry tss_high;     // TSS descriptor high part (64-bit extension)
} __attribute__((packed));

// TSS instance
static struct TSS tss = {0};
static struct ExtendedGDT* current_gdt = NULL;

// Helper function to set a GDT entry
static void gdt_set_gate(struct GDTEntry* entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    entry->base_low = (base & 0xFFFF);
    entry->base_middle = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;
    entry->limit_low = (limit & 0xFFFF);
    entry->granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    entry->access = access;
}

// Get current GDT base address
static uint64_t get_current_gdt_base(void) {
    struct GDTPtr gdtr;
    __asm__ volatile("sgdt %0" : "=m"(gdtr));
    return gdtr.base;
}

void gdt_init_tss(void) {
    // Get the current GDT that Limine set up
    uint64_t gdt_base = get_current_gdt_base();
    current_gdt = (struct ExtendedGDT*)gdt_base;
    
    // Clear TSS
    memset(&tss, 0, sizeof(struct TSS));
    
    // Set up TSS with default values
    tss.iopb_offset = sizeof(struct TSS);  // No I/O permission bitmap
    
    // You can set ring 0 stack pointer here if needed
    // tss.rsp0 = (uint64_t)kernel_stack_top;
    
    // Configure TSS descriptor (takes 2 GDT entries in 64-bit mode)
    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(struct TSS) - 1;
    
    // TSS Low descriptor (entry 5, selector 0x28)
    gdt_set_gate(&current_gdt->tss_low, 
                 tss_base & 0xFFFFFFFF, 
                 tss_limit,
                 GDT_PRESENT | GDT_TSS, 
                 0x0);
    
    // TSS High descriptor (entry 6) - contains upper 32 bits of base address
    struct GDTEntry* tss_high = &current_gdt->tss_high;
    tss_high->base_low = (tss_base >> 32) & 0xFFFF;
    tss_high->base_middle = (tss_base >> 48) & 0xFF;
    tss_high->base_high = (tss_base >> 56) & 0xFF;
    tss_high->access = 0;
    tss_high->granularity = 0;
    tss_high->limit_low = 0;
    
    // Update GDT limit to include TSS entries
    struct GDTPtr gdtr;
    __asm__ volatile("sgdt %0" : "=m"(gdtr));
    gdtr.limit = sizeof(struct ExtendedGDT) - 1;
    __asm__ volatile("lgdt %0" : : "m"(gdtr));
    
    // Load TSS (selector 0x28 = entry 5)
    __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}

// Set the ring 0 stack pointer in TSS (call this when switching tasks)
void tss_set_kernel_stack(uint64_t stack_top) {
    tss.rsp0 = stack_top;
}

// Get TSS pointer for direct manipulation if needed
struct TSS* get_tss(void) {
    return &tss;
}

// Limine segment selectors (these should match what Limine sets up)
uint16_t gdt_get_code_segment(void) {
    return 0x08;  // Kernel code segment (entry 1)
}

uint16_t gdt_get_data_segment(void) {
    return 0x10;  // Kernel data segment (entry 2)
}

uint16_t gdt_get_user_code_segment(void) {
    return 0x18;  // User code segment (entry 3)
}

uint16_t gdt_get_user_data_segment(void) {
    return 0x20;  // User data segment (entry 4)
}

uint16_t gdt_get_tss_segment(void) {
    return 0x28;  // TSS segment (entry 5)
}