#ifndef __VALERN_GDT_H
#define __VALERN_GDT_H

#include <stdint.h>

// GDT Access Flags
#define GDT_PRESENT    0x80
#define GDT_RING0      0x00
#define GDT_RING3      0x60
#define GDT_CODE       0x18
#define GDT_DATA       0x10
#define GDT_READ       0x02
#define GDT_WRITE      0x02
#define GDT_EXEC       0x08
#define GDT_GRAN_4K    0x80
#define GDT_LONG_MODE  0x20
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

// Opaque type for GDT
typedef struct GDT GDT;

// C interface functions
void gdt_init(GDT* gdt);
uint16_t gdt_get_code_segment(void);
uint16_t gdt_get_data_segment(void);

#endif // __VALERN_GDT_C_H
