#include "gdt.h"
#include <stddef.h>
#include <stdbool.h>

// Structure definitions for GDT entries
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

// The actual GDT structure
struct GDT {
    struct GDTEntry null;
    struct GDTEntry kernel_code;
    struct GDTEntry kernel_data;
    struct GDTEntry user_code;
    struct GDTEntry user_data;
    struct GDTEntry tss_low;
    struct GDTEntry tss_high;
} __attribute__((packed));

// TSS structure instance
static struct TSS tss = {0};

// Helper function to set a GDT entry
static void gdt_set_gate(struct GDTEntry* entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    entry->base_low = (base & 0xFFFF);
    entry->base_middle = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;
    entry->limit_low = (limit & 0xFFFF);
    entry->granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    entry->access = access;
}

// Add to top of gdt_init
bool check_cpu_features(void) {
    uint32_t eax, ebx, ecx, edx;
    
    // Check for long mode support
    __asm__ volatile("cpuid" 
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
        : "a"(0x80000001));
    
    if (!(edx & (1 << 29))) {
        // Long mode not supported
        return false;
    }
    
    // Check for PAE support
    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1));
    
    if (!(edx & (1 << 6))) {
        // PAE not supported
        return false;
    }
    
    return true;
}

void gdt_init(GDT* gdt) {
    // Disable interrupts first
    __asm__ volatile("cli");

    // Check CPU features
    if (!check_cpu_features()) {
        // CPU does not support required features
        return;
    }

    // Enable Protected Mode by setting CR0.PE
    uint64_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 1;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

    // Enable PAE (required for Long Mode)
    uint64_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x20;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4) : "memory");

    // Enable Long Mode by setting EFER.LME
    uint32_t eax, edx;
    __asm__ volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(0xC0000080));
    eax |= 0x100;
    __asm__ volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(0xC0000080));

    // Setup GDT pointer
    struct GDTPtr gdtr = {
        .limit = sizeof(struct GDT) - 1,
        .base = (uint64_t)gdt
    };

    // Clear TSS
    for (size_t i = 0; i < sizeof(struct TSS); i++) {
        ((uint8_t*)&tss)[i] = 0;
    }

    // Configure null descriptor
    gdt_set_gate(&gdt->null, 0, 0, 0, 0);

    // Configure kernel code segment
    gdt_set_gate(&gdt->kernel_code, 0, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_CODE | GDT_READ | GDT_EXEC,
                 GDT_GRAN_4K | GDT_LONG_MODE);

    // Configure kernel data segment
    gdt_set_gate(&gdt->kernel_data, 0, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_DATA | GDT_WRITE,
                 GDT_GRAN_4K);

    // Configure user code segment (if needed)
    gdt_set_gate(&gdt->user_code, 0, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING3 | GDT_CODE | GDT_READ | GDT_EXEC,
                 GDT_GRAN_4K | GDT_LONG_MODE);

    // Configure user data segment (if needed)
    gdt_set_gate(&gdt->user_data, 0, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING3 | GDT_DATA | GDT_WRITE,
                 GDT_GRAN_4K);

    // Configure TSS
    uint64_t tss_base = (uint64_t)&tss;
    gdt_set_gate(&gdt->tss_low, tss_base & 0xFFFFFFFF, sizeof(struct TSS),
                 GDT_PRESENT | GDT_TSS | 0x0, 0x0);
    
    // Set up high bits of TSS base in tss_high
    struct GDTEntry* tss_high = &gdt->tss_high;
    tss_high->base_low = (tss_base >> 32) & 0xFFFF;
    tss_high->base_middle = (tss_base >> 48) & 0xFF;
    tss_high->base_high = (tss_base >> 56) & 0xFF;
    tss_high->access = 0;
    tss_high->granularity = 0;
    tss_high->limit_low = 0;

    // Load TSS
    tss.iopb_offset = sizeof(struct TSS);

    // Load the GDT
    __asm__ volatile("lgdt %0" : : "m"(gdtr));

    // Reload code segment using a far return
    __asm__ volatile (
        "pushq %[cs]\n"
        "pushq 1f\n"
        "lretq\n"
        "1:\n"
        :
        : [cs] "i"(0x08)
        : "memory"
    );

    // Reload data segments
    __asm__ volatile(
        "mov %0, %%ds\n"
        "mov %0, %%es\n"
        "mov %0, %%fs\n"
        "mov %0, %%gs\n"
        "mov %0, %%ss\n"
        : : "r"((uint16_t)0x10) : "memory");

    // Enable paging (required for Long Mode)
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

    // Load TSS
    __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}

uint16_t gdt_get_code_segment(void) {
    return 0x08;  // Kernel code segment offset
}

uint16_t gdt_get_data_segment(void) {
    return 0x10;  // Kernel data segment offset
}