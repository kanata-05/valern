#include <idt.h>
#include <isr.h>
#include <util.h>

void remap_pic() {
    outportb(0x20, 0x11);
    outportb(0xA0, 0x11);
    outportb(0x21, 0x20);
    outportb(0xA1, 0x28);
    outportb(0x21, 0x04);
    outportb(0xA1, 0x02);
    outportb(0x21, 0x01);
    outportb(0xA1, 0x01);
    outportb(0x21, 0x00);
    outportb(0xA1, 0x00);
}

void initiateISR() {
    remap_pic();
    // Set up ISRs and IRQs as needed
    for (int i = 0; i < 48; i++) {
        set_idt_gate(i, (uint64_t)asm_isr_redirect_table[i], 0x8E);
    }
    set_idt();
    asm volatile("sti");
}

irqHandler *registerIRQhandler(uint8_t id, void *handler) {
    // Register your handler here
}