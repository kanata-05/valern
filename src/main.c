#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "stdmem.h"
#include "console.h"
#include "gdt.h"
#include "interrupts.h"
#include "keyboard.h"

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

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

// The following will be our kernel's entry point.
// If renaming kernel() to something else, make sure to change the
// linker script accordingly.
void kernel(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    gdt_init_tss();

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    // Initialize our console with the framebuffer
    init_shell(framebuffer);

    printf("GDT TSS Started!\n\n", BLUE, BLACK);

    printf("Welcome to Valern!\n", GRAY, BLACK);
    printf("A minimal operating system.\n\n", GRAY, BLACK);
        
    prompt();

    hcf(); // Nothing else to do currently, halt.
}