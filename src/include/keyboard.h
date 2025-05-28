#ifndef __VALERN_KEYBOARD_H
#define __VALERN_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// Special key codes
#define KEY_BACKSPACE  '\b'
#define KEY_TAB        '\t'
#define KEY_ENTER      '\n'
#define KEY_ESCAPE     27

// Control key combinations (Ctrl+A = 1, Ctrl+B = 2, etc.)
#define CTRL_A 1
#define CTRL_B 2
#define CTRL_C 3
#define CTRL_D 4
#define CTRL_L 12
#define CTRL_Z 26

// Initialize keyboard driver
void keyboard_init(void);

// Keyboard interrupt handler (to be called from IDT)
void keyboard_interrupt_handler(void);

// Check if a key is available in buffer
bool keyboard_has_key(void);

// Get character from keyboard (blocking)
char keyboard_getchar(void);

// Get character from keyboard (non-blocking, returns 0 if no key)
char keyboard_getchar_nonblock(void);

// Modifier key state functions
bool keyboard_shift_pressed(void);
bool keyboard_ctrl_pressed(void);
bool keyboard_alt_pressed(void);
bool keyboard_caps_lock_active(void);

// Clear keyboard input buffer
void keyboard_clear_buffer(void);

#endif // __VALERN_KEYBOARD_H