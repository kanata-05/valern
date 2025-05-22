#ifndef __VALERN_PAGING_H
#define __VALERN_PAGING_H

#include <stdint.h>

// Page entry flags
#define PAGE_PRESENT       (1ULL << 0)
#define PAGE_WRITABLE      (1ULL << 1)
#define PAGE_USER          (1ULL << 2)
#define PAGE_WRITE_THROUGH (1ULL << 3)
#define PAGE_CACHE_DISABLE (1ULL << 4)
#define PAGE_ACCESSED      (1ULL << 5)
#define PAGE_DIRTY         (1ULL << 6)
#define PAGE_HUGE          (1ULL << 7)
#define PAGE_GLOBAL        (1ULL << 8)
#define PAGE_NX           (1ULL << 63)  // No Execute bit

// 4-level paging structure entries count
#define ENTRIES_PER_TABLE 512

// Page size constants
#define PAGE_SIZE_4K    0x1000
#define PAGE_SIZE_2M    0x200000
#define PAGE_SIZE_1G    0x40000000

// Page table entry structure
typedef uint64_t page_table_entry;

// Page table structure (aligned to 4K boundary)
typedef struct __attribute__((aligned(PAGE_SIZE_4K))) {
    page_table_entry entries[ENTRIES_PER_TABLE];
} page_table;

// Initialize paging system
void init_paging(void);

// Map a virtual address to a physical address with specified flags
void map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);

// Unmap a virtual address
void unmap_page(uint64_t virtual_addr);

// Get the physical address for a virtual address
uint64_t get_physical_addr(uint64_t virtual_addr);

#endif // __VALERN_PAGING_H
