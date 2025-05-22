#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "stdmem.h"     // Revised memory management shifted to this library.
#include "console.h"
// #include "gdt.h"     This section has been commented out to prevent GDT 
// #include "paging.h"  and paging from causing issues

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// Global GDT instance - placed in .data section and properly aligned
// static uint8_t gdt_storage[4096] __attribute__((section(".data"), aligned(0x1000))); This section has been commented out to prevent GDT and paging from causing issues

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

// Declare stack symbols from linker script
// extern uint64_t kernel_stack_top;  This section has been commented out to prevent GDT and paging from causing issues

// The following will be our kernel's entry point.
// If renaming kernel() to something else, make sure to change the
// linker script accordingly.
void kernel(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

/*  This section has been commented out to prevent GDT and paging from causing issues

    // Disable interrupts during initialization
    __asm__ volatile("cli");    // Set up the stack pointer with proper alignment
    __asm__ volatile(
        "mov %0, rsp\n"
        "and rsp, -16"      // Ensure 16-byte stack alignment using -16 (same as ~0xF)
        : : "r"(&kernel_stack_top) : "memory"
    );    // Initialize GDT
    gdt_init((GDT*)gdt_storage);

    // Initialize paging
    init_paging();  */ 

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    // Initialize our console with the framebuffer
    init_shell(framebuffer);
//    printf("GDT Initialized!\n", BLUE, BLACK);
//    printf("Paging Initialized!\n\n", BLUE, BLACK);


    printf("Welcome to Valern!\n", GRAY, BLACK);
    printf("A minimal operating system.\n\n", GRAY, BLACK);
    
    prompt();

    hcf(); // Nothing else to do currently, halt.

}