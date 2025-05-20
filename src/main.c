#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "stdmem.h" // Revised memory management shifted to this library.
#include "console.h" // Console implementation
#include "gdt.h" // Global Descriptor Table implementation
#include "idt.h" // Interrupt Descriptor Table implementation
#include "pic.h" // Programmable Interrupt Controller implementation
#include "keyboard.h" // Keyboard input handling

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// Memory Managers have been shifted to stdmem.h

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

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    
    // Initialize our console with the framebuffer
    console_init(framebuffer);    // Initialize CPU segmentation
    gdt_init();

    // Initialize and remap the PIC first
    pic_init();
    
    // Disable all interrupts during setup
    pic_disable();

    // Initialize interrupt handling
    idt_init();

    // Initialize keyboard and its interrupt handler
    keyboard_init();

    // Enable interrupts
    asm volatile("sti");

    // Write a welcome message
    console_write("Welcome to Valern!\n");
    console_write("A minimal operating system\n");
    console_write("CPU segmentation, interrupts, and keyboard initialized!\n");
    console_draw_prompt();

    // We're done with initialization, now we can enter the main loop
    for (;;) {
        asm ("hlt");  // Halt CPU until next interrupt
    }
}