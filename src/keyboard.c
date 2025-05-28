#include "keyboard.h"
#include "stdmem.h"
#include <stdint.h>
#include <stdbool.h>

// PS/2 Keyboard ports
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64

// Keyboard status register bits
#define KEYBOARD_STATUS_OUTPUT_FULL 0x01
#define KEYBOARD_STATUS_INPUT_FULL  0x02

// Special scan codes
#define KEY_RELEASED_MASK 0x80
#define EXTENDED_SCANCODE 0xE0

// Modifier key states
static struct {
    bool shift_left;
    bool shift_right;
    bool ctrl_left;
    bool ctrl_right;
    bool alt_left;
    bool alt_right;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
} key_state = {0};

// Scancode to ASCII mapping (US QWERTY layout)
static const char scancode_to_ascii[] = {
    0,    0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*',  0,   ' '
};

// Shifted versions of the above
static const char scancode_to_ascii_shifted[] = {
    0,    0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*',  0,   ' '
};

// Scancode definitions
#define SC_LSHIFT     0x2A
#define SC_RSHIFT     0x36
#define SC_LCTRL      0x1D
#define SC_LALT       0x38
#define SC_CAPS_LOCK  0x3A
#define SC_NUM_LOCK   0x45
#define SC_SCROLL_LOCK 0x46

// Key buffer
#define KEY_BUFFER_SIZE 256
static char key_buffer[KEY_BUFFER_SIZE];
static volatile int buffer_head = 0;
static volatile int buffer_tail = 0;
static volatile int buffer_count = 0;

// Port I/O functions
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

// Wait for keyboard controller to be ready for input
static void keyboard_wait_input(void) {
    while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_INPUT_FULL);
}

// Wait for keyboard controller to have output ready
static void keyboard_wait_output(void) {
    while (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL));
}

// Add character to keyboard buffer
static void keyboard_buffer_add(char c) {
    if (buffer_count < KEY_BUFFER_SIZE) {
        key_buffer[buffer_head] = c;
        buffer_head = (buffer_head + 1) % KEY_BUFFER_SIZE;
        buffer_count++;
    }
}

// Get character from keyboard buffer
static char keyboard_buffer_get(void) {
    if (buffer_count > 0) {
        char c = key_buffer[buffer_tail];
        buffer_tail = (buffer_tail + 1) % KEY_BUFFER_SIZE;
        buffer_count--;
        return c;
    }
    return 0;
}

// Process a scancode
static void process_scancode(uint8_t scancode) {
    bool key_released = (scancode & KEY_RELEASED_MASK) != 0;
    uint8_t key = scancode & ~KEY_RELEASED_MASK;
    
    // Handle modifier keys
    switch (key) {
        case SC_LSHIFT:
            key_state.shift_left = !key_released;
            return;
        case SC_RSHIFT:
            key_state.shift_right = !key_released;
            return;
        case SC_LCTRL:
            key_state.ctrl_left = !key_released;
            return;
        case SC_LALT:
            key_state.alt_left = !key_released;
            return;
        case SC_CAPS_LOCK:
            if (!key_released) {
                key_state.caps_lock = !key_state.caps_lock;
            }
            return;
        case SC_NUM_LOCK:
            if (!key_released) {
                key_state.num_lock = !key_state.num_lock;
            }
            return;
        case SC_SCROLL_LOCK:
            if (!key_released) {
                key_state.scroll_lock = !key_state.scroll_lock;
            }
            return;
    }
    
    // Only process key presses (not releases) for regular keys
    if (key_released) {
        return;
    }
    
    // Convert scancode to ASCII
    if (key < sizeof(scancode_to_ascii)) {
        char ascii = 0;
        bool shift_pressed = key_state.shift_left || key_state.shift_right;
        bool caps_active = key_state.caps_lock;
        
        // Determine if we should use shifted character
        if (shift_pressed) {
            ascii = scancode_to_ascii_shifted[key];
        } else {
            ascii = scancode_to_ascii[key];
        }
        
        // Handle caps lock for letters
        if (ascii >= 'a' && ascii <= 'z' && caps_active && !shift_pressed) {
            ascii = ascii - 'a' + 'A';
        } else if (ascii >= 'A' && ascii <= 'Z' && caps_active && shift_pressed) {
            ascii = ascii - 'A' + 'a';
        }
        
        // Handle control key combinations
        if (key_state.ctrl_left || key_state.ctrl_right) {
            if (ascii >= 'a' && ascii <= 'z') {
                ascii = ascii - 'a' + 1; // Ctrl+A = 1, Ctrl+B = 2, etc.
            } else if (ascii >= 'A' && ascii <= 'Z') {
                ascii = ascii - 'A' + 1;
            }
        }
        
        if (ascii != 0) {
            keyboard_buffer_add(ascii);
        }
    }
}

// Keyboard interrupt handler
void keyboard_interrupt_handler(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    
    if (status & KEYBOARD_STATUS_OUTPUT_FULL) {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        
        // Handle extended scancodes (for now, just ignore them)
        static bool extended_scancode = false;
        if (scancode == EXTENDED_SCANCODE) {
            extended_scancode = true;
            return;
        }
        
        if (extended_scancode) {
            extended_scancode = false;
            // Handle extended scancodes here if needed
            // For now, we'll just ignore them
            return;
        }
        
        process_scancode(scancode);
    }
}

// Initialize keyboard driver
void keyboard_init(void) {
    // Clear the buffer
    buffer_head = 0;
    buffer_tail = 0;
    buffer_count = 0;
    
    // Clear modifier states
    memset(&key_state, 0, sizeof(key_state));
    
    // Enable keyboard (this is usually already done by BIOS/UEFI)
    keyboard_wait_input();
    outb(KEYBOARD_COMMAND_PORT, 0xAE); // Enable keyboard
    
    keyboard_wait_input();
    outb(KEYBOARD_COMMAND_PORT, 0x20); // Read controller configuration
    keyboard_wait_output();
    uint8_t config = inb(KEYBOARD_DATA_PORT);
    
    config |= 0x01; // Enable keyboard interrupt
    config &= ~0x10; // Enable keyboard
    
    keyboard_wait_input();
    outb(KEYBOARD_COMMAND_PORT, 0x60); // Write controller configuration
    keyboard_wait_input();
    outb(KEYBOARD_DATA_PORT, config);
    
    // Set keyboard to scancode set 1 (default)
    keyboard_wait_input();
    outb(KEYBOARD_DATA_PORT, 0xF0);
    keyboard_wait_output();
    inb(KEYBOARD_DATA_PORT); // Acknowledge
    
    keyboard_wait_input();
    outb(KEYBOARD_DATA_PORT, 0x01); // Scancode set 1
    keyboard_wait_output();
    inb(KEYBOARD_DATA_PORT); // Acknowledge
}

// Check if a key is available
bool keyboard_has_key(void) {
    return buffer_count > 0;
}

// Get a character from keyboard (blocking)
char keyboard_getchar(void) {
    while (!keyboard_has_key()) {
        asm volatile("hlt"); // Wait for interrupt
    }
    return keyboard_buffer_get();
}

// Get a character from keyboard (non-blocking)
char keyboard_getchar_nonblock(void) {
    return keyboard_buffer_get();
}

// Get modifier key states
bool keyboard_shift_pressed(void) {
    return key_state.shift_left || key_state.shift_right;
}

bool keyboard_ctrl_pressed(void) {
    return key_state.ctrl_left || key_state.ctrl_right;
}

bool keyboard_alt_pressed(void) {
    return key_state.alt_left || key_state.alt_right;
}

bool keyboard_caps_lock_active(void) {
    return key_state.caps_lock;
}

// Clear keyboard buffer
void keyboard_clear_buffer(void) {
    buffer_head = 0;
    buffer_tail = 0;
    buffer_count = 0;
}