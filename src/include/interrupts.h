#ifndef __VALERN_INTERRUPTS_H
#define __VALERN_INTERRUPTS_H

// Initialize interrupt system (IDT and PIC)
void interrupts_init(void);

// Keyboard interrupt service routine
void keyboard_isr(void);

#endif // __VALERN_INTERRUPTS_H