#include "interrupts.h"
#include "keyboard.h"
#include <stdint.h>

// IDT entry structure
struct IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

// IDT pointer structure
struct IDTPtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// IDT with 256 entries
static struct IDTEntry idt[256];
static struct IDTPtr idtr;

// PIC (Programmable Interrupt Controller) ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// Set an IDT entry
static void idt_set_gate(int num, uint64_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].selector = selector;
    idt[num].ist = 0;
    idt[num].type_attr = flags;
    idt[num].reserved = 0;
}

// Assembly interrupt stubs (defined at the end of this file)
extern void keyboard_interrupt_stub(void);

// Keyboard interrupt handler wrapper
void keyboard_isr(void) {
    keyboard_interrupt_handler();
    
    // Send End of Interrupt (EOI) to PIC
    outb(PIC1_COMMAND, 0x20);
}

// Initialize PIC
static void pic_init(void) {
    // Save current interrupt masks
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);
    
    // Initialize PIC1
    outb(PIC1_COMMAND, 0x11); // Initialize command
    outb(PIC1_DATA, 0x20);    // IRQ 0-7 mapped to interrupts 0x20-0x27
    outb(PIC1_DATA, 0x04);    // PIC1 connected to PIC2 via IRQ2
    outb(PIC1_DATA, 0x01);    // 8086 mode
    
    // Initialize PIC2
    outb(PIC2_COMMAND, 0x11); // Initialize command
    outb(PIC2_DATA, 0x28);    // IRQ 8-15 mapped to interrupts 0x28-0x2F
    outb(PIC2_DATA, 0x02);    // PIC2 connected to PIC1 via IRQ2
    outb(PIC2_DATA, 0x01);    // 8086 mode
    
    // Restore interrupt masks (disable all except keyboard)
    outb(PIC1_DATA, 0xFD); // Enable only IRQ1 (keyboard)
    outb(PIC2_DATA, 0xFF); // Disable all IRQ8-15
}

// Initialize interrupt system
void interrupts_init(void) {
    // Clear IDT
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Set up keyboard interrupt (IRQ1 -> interrupt 0x21)
    idt_set_gate(0x21, (uint64_t)keyboard_interrupt_stub, 0x08, 0x8E);
    
    // Set up IDT pointer
    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint64_t)idt;
    
    // Initialize PIC
    pic_init();
    
    // Load IDT
    asm volatile("lidt %0" : : "m"(idtr));
    
    // Enable interrupts
    asm volatile("sti");
}

// Assembly interrupt stub for keyboard
asm(
    ".global keyboard_interrupt_stub\n"
    "keyboard_interrupt_stub:\n"
    "    push %rax\n"
    "    push %rbx\n"
    "    push %rcx\n"
    "    push %rdx\n"
    "    push %rsi\n"
    "    push %rdi\n"
    "    push %rbp\n"
    "    push %r8\n"
    "    push %r9\n"
    "    push %r10\n"
    "    push %r11\n"
    "    push %r12\n"
    "    push %r13\n"
    "    push %r14\n"
    "    push %r15\n"
    "    call keyboard_isr\n"
    "    pop %r15\n"
    "    pop %r14\n"
    "    pop %r13\n"
    "    pop %r12\n"
    "    pop %r11\n"
    "    pop %r10\n"
    "    pop %r9\n"
    "    pop %r8\n"
    "    pop %rbp\n"
    "    pop %rdi\n"
    "    pop %rsi\n"
    "    pop %rdx\n"
    "    pop %rcx\n"
    "    pop %rbx\n"
    "    pop %rax\n"
    "    iretq\n"
);