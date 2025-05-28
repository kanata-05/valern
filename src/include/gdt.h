#ifndef __VALERN_GDT_MINIMAL_H
#define __VALERN_GDT_MINIMAL_H

#include <stdint.h>

// GDT Access Flags (kept for TSS setup)
#define GDT_PRESENT    0x80
#define GDT_TSS        0x89

// TSS structure for 64-bit mode
struct TSS {
    uint32_t reserved0;
    uint64_t rsp0;        // Stack pointer for ring 0
    uint64_t rsp1;        // Stack pointer for ring 1  
    uint64_t rsp2;        // Stack pointer for ring 2
    uint64_t reserved1;
    uint64_t ist[7];      // Interrupt stack table
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset; // I/O map base address
} __attribute__((packed));

// Initialize TSS in existing GDT
void gdt_init_tss(void);

// Set kernel stack in TSS (for privilege level switches)
void tss_set_kernel_stack(uint64_t stack_top);

// Get TSS pointer for direct access
struct TSS* get_tss(void);

// Segment selector functions (Limine's layout)
uint16_t gdt_get_code_segment(void);      // 0x08
uint16_t gdt_get_data_segment(void);      // 0x10  
uint16_t gdt_get_user_code_segment(void); // 0x18
uint16_t gdt_get_user_data_segment(void); // 0x20
uint16_t gdt_get_tss_segment(void);       // 0x28

#endif // __VALERN_GDT_MINIMAL_H