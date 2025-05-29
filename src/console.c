#include "console.h"
#include "stdmem.h"
#include "fonts.h"
#include "keyboard.h"
#include "port.h"
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

static struct console_state console;

int load_font(struct psf_font* font, void* font_data, size_t font_size) {
    if (psf_load_font(font, font_data, font_size) != 0) {
        return -1;
    }
    
    // Update console dimensions based on new font size
    console.width = (console.fb->width / (font->width * console.scale));
    console.height = (console.fb->height / (font->height * console.scale));
    return 0;
}

void init_shell(struct limine_framebuffer* framebuffer) {
    console.fb = framebuffer;
    console.cursor_x = 0;
    console.cursor_y = 0;
    console.scale = FONT_SCALE;
    console.fg_color = GRAY;
    console.bg_color = BLACK;

    // Try to load PSF font first
    extern char _binary_src_fonts_default_psf_start[];
    extern char _binary_src_fonts_default_psf_end[];
    
    if (load_font(&console.font, 
                         _binary_src_fonts_default_psf_start,
                         (size_t)(_binary_src_fonts_default_psf_end - _binary_src_fonts_default_psf_start)) != 0) {
        // PSF font loading failed
        printf("PSF font loading failed!\nFalling back to VGA font\n", RED, BLACK);
        // PSF font loading failed, fall back to VGA font
        console.font.width = 8;  // VGA font is 8x16
        console.font.height = 16;
        console.font.glyph_buffer = vga_font; 
        console.font.glyph_count = 256;
        console.font.glyph_size = 16;
        console.font.version = 0;  // Use 0 to indicate VGA font
    } else {
        // PSF font loaded successfully
        printf("PSF font loaded successfully!\n", BLUE, BLACK);
    }

    // Calculate console dimensions based on font size
    console.width = (console.fb->width / (console.font.width * console.scale));
    console.height = (console.fb->height / (console.font.height * console.scale));
}

void console_clear(void) {
    volatile uint32_t* fb = console.fb->address;
    for (size_t i = 0; i < console.fb->height; i++) {
        for (size_t j = 0; j < console.fb->width; j++) {
            fb[i * (console.fb->pitch / 4) + j] = console.bg_color;
        }
    }
    console.cursor_x = 0;
    console.cursor_y = 0;
}

static void draw_char(char c, size_t x, size_t y, unsigned int fg_color, unsigned int bg_color) {    
    if (!console.font.glyph_buffer) return;  // No font loaded
    
    volatile uint32_t* fb = console.fb->address;
    size_t fb_x = x * console.font.width * console.scale;
    size_t fb_y = y * console.font.height * console.scale;
    
    const uint8_t* glyph = psf_get_glyph(&console.font, (unsigned char)c);
    if (!glyph) return;
    
    for (size_t row = 0; row < console.font.height; row++) {
        uint8_t glyph_row = glyph[row];
        for (size_t col = 0; col < console.font.width; col++) {
            uint32_t color = (glyph_row & (1 << (7 - col))) ? fg_color : bg_color;
            
            // Draw scaled pixel
            for (size_t scale_y = 0; scale_y < console.scale; scale_y++) {
                for (size_t scale_x = 0; scale_x < console.scale; scale_x++) {
                    size_t pixel_x = fb_x + (col * console.scale) + scale_x;
                    size_t pixel_y = fb_y + (row * console.scale) + scale_y;
                    
                    if (pixel_x >= console.fb->width || pixel_y >= console.fb->height) continue;
                    
                    fb[pixel_y * (console.fb->pitch / 4) + pixel_x] = color;
                }
            }
        }
    }
}

void putChar(char c, unsigned int fg_color, unsigned int bg_color) {
    if (c == '\n') {
        console.cursor_x = 0;
        console.cursor_y++;
    } else if (c == '\r') {
        console.cursor_x = 0;
    } else if (c == '\b') {
        // Handle backspace
        if (console.cursor_x > 0) {
            console.cursor_x--;
        } else if (console.cursor_y > 0) {
            console.cursor_y--;
            console.cursor_x = console.width - 1;
        }
    } else {
        draw_char(c, console.cursor_x, console.cursor_y, fg_color, bg_color);
        console.cursor_x++;
    }
    
    if (console.cursor_x >= console.width) {
        console.cursor_x = 0;
        console.cursor_y++;
    }
    
    if (console.cursor_y >= console.height) {
        // Basic scrolling - just clear for now
        console_clear();
    }
}

// Helper function to convert integer to string
static int int_to_str(int value, char* buffer, int base) {
    char temp[32];
    int i = 0;
    int is_negative = 0;
    
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }
    
    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value;
    }
    
    while (value > 0) {
        int digit = value % base;
        temp[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
        value /= base;
    }
    
    int pos = 0;
    if (is_negative) {
        buffer[pos++] = '-';
    }
    
    while (i > 0) {
        buffer[pos++] = temp[--i];
    }
    
    buffer[pos] = '\0';
    return pos;
}

// Helper function to convert unsigned integer to string
static int uint_to_str(unsigned int value, char* buffer, int base) {
    char temp[32];
    int i = 0;
    
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }
    
    while (value > 0) {
        int digit = value % base;
        temp[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
        value /= base;
    }
    
    int pos = 0;
    while (i > 0) {
        buffer[pos++] = temp[--i];
    }
    
    buffer[pos] = '\0';
    return pos;
}

// Helper function for hexadecimal (uppercase)
static int uint_to_hex_upper(unsigned int value, char* buffer) {
    char temp[32];
    int i = 0;
    
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }
    
    while (value > 0) {
        int digit = value % 16;
        temp[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        value /= 16;
    }
    
    int pos = 0;
    while (i > 0) {
        buffer[pos++] = temp[--i];
    }
    
    buffer[pos] = '\0';
    return pos;
}

// Improved printf function with format specifiers
void printf(const char* format, unsigned int fg_color, unsigned int bg_color, ...) {
    va_list args;
    va_start(args, bg_color);
    
    char buffer[32];
    const char* ptr = format;
    
    while (*ptr) {
        if (*ptr == '%' && *(ptr + 1)) {
            ptr++; // Skip '%'
            
            switch (*ptr) {
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    int_to_str(value, buffer, 10);
                    for (char* b = buffer; *b; b++) {
                        putChar(*b, fg_color, bg_color);
                    }
                    break;
                }
                
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    uint_to_str(value, buffer, 10);
                    for (char* b = buffer; *b; b++) {
                        putChar(*b, fg_color, bg_color);
                    }
                    break;
                }
                
                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    uint_to_str(value, buffer, 16);
                    for (char* b = buffer; *b; b++) {
                        putChar(*b, fg_color, bg_color);
                    }
                    break;
                }
                
                case 'X': {
                    unsigned int value = va_arg(args, unsigned int);
                    uint_to_hex_upper(value, buffer);
                    for (char* b = buffer; *b; b++) {
                        putChar(*b, fg_color, bg_color);
                    }
                    break;
                }
                
                case 'c': {
                    char value = (char)va_arg(args, int);
                    putChar(value, fg_color, bg_color);
                    break;
                }
                
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str) {
                        while (*str) {
                            putChar(*str++, fg_color, bg_color);
                        }
                    } else {
                        // Handle NULL pointer
                        const char* null_str = "(null)";
                        while (*null_str) {
                            putChar(*null_str++, fg_color, bg_color);
                        }
                    }
                    break;
                }
                
                case 'p': {
                    void* ptr_val = va_arg(args, void*);
                    putChar('0', fg_color, bg_color);
                    putChar('x', fg_color, bg_color);
                    uint_to_str((uintptr_t)ptr_val, buffer, 16);
                    for (char* b = buffer; *b; b++) {
                        putChar(*b, fg_color, bg_color);
                    }
                    break;
                }
                
                case '%': {
                    putChar('%', fg_color, bg_color);
                    break;
                }
                
                default: {
                    // Unknown format specifier, just print it
                    putChar('%', fg_color, bg_color);
                    putChar(*ptr, fg_color, bg_color);
                    break;
                }
            }
        } else {
            putChar(*ptr, fg_color, bg_color);
        }
        ptr++;
    }
    
    va_end(args);
}

void shell(void) {
    char input_buffer[256];
    int buffer_pos = 0;
    
    printf("valern> ", GREEN, BLACK);
    
    while (true) {
        char c = keyboard_getchar();
        
        switch (c) {
            case KEY_ENTER:
                printf("\n", WHITE, BLACK);
                input_buffer[buffer_pos] = '\0';
                process_command(input_buffer);
                buffer_pos = 0;
                printf("valern> ", GREEN, BLACK);
                break;
                
            case KEY_BACKSPACE:
                if (buffer_pos > 0) {
                    buffer_pos--;
                    printf("\b \b", WHITE, BLACK); // Move back, print space, move back
                }
                break;
                
            case CTRL_C:
                printf("^C\n", RED, BLACK);
                buffer_pos = 0;
                printf("valern> ", GREEN, BLACK);
                break;
                
            case CTRL_L:
                console_clear();
                printf("valern> ", GREEN, BLACK);
                for (int i = 0; i < buffer_pos; i++) {
                    printf("%c", WHITE, BLACK, input_buffer[i]);
                }
                break;
                
            default:
                if (c >= 32 && c <= 126 && buffer_pos < sizeof(input_buffer) - 1) {
                    input_buffer[buffer_pos++] = c;
                    printf("%c", WHITE, BLACK, c);
                }
                break;
        }
    }
}

void process_command(const char* command) {
    if (strcmp(command, "help") == 0) {
        printf("Available commands:\n", WHITE, BLACK);
        printf("  help    - Show this help message\n", GRAY, BLACK);
        printf("  clear   - Clear the screen\n", GRAY, BLACK);
        printf("  hello   - Say hello\n", GRAY, BLACK);
        printf("  test    - Test printf formatting\n", GRAY, BLACK);
        printf("  info    - Show system information\n", GRAY, BLACK);
        printf("  reboot  - Reboot the system\n", GRAY, BLACK);
    }
    else if (strcmp(command, "clear") == 0) {
        console_clear();
    }
    else if (strcmp(command, "hello") == 0) {
        printf("Hello from Valern OS!\n", GREEN, BLACK);
    }
    else if (strcmp(command, "test") == 0) {
        // Demonstrate the new printf capabilities
        printf("Testing printf formatting:\n", BLUE, BLACK);
        printf("Integer: %d\n", WHITE, BLACK, 42);
        printf("Negative: %d\n", WHITE, BLACK, -123);
        printf("Unsigned: %u\n", WHITE, BLACK, 3000000000U);
        printf("Hex lowercase: 0x%x\n", WHITE, BLACK, 255);
        printf("Hex uppercase: 0x%X\n", WHITE, BLACK, 255);
        printf("Character: %c\n", WHITE, BLACK, 'A');
        printf("String: %s\n", WHITE, BLACK, "Hello World!");
        printf("Pointer: %p\n", WHITE, BLACK, (void*)0x12345678);
        printf("Percent: 100%%\n", WHITE, BLACK);
    }
    else if (strcmp(command, "info") == 0) {
        printf("Valern OS System Information:\n", GREEN, BLACK);
        printf("Console dimensions: %dx%d characters\n", WHITE, BLACK, console.width, console.height);
        printf("Font size: %dx%d pixels\n", WHITE, BLACK, console.font.width, console.font.height);
        printf("Framebuffer: %dx%d pixels\n", WHITE, BLACK, console.fb->width, console.fb->height);
        printf("Scale factor: %dx\n", WHITE, BLACK, console.scale);
        printf("Font version: %d\n", WHITE, BLACK, console.font.version);
        printf("Glyph count: %d\n", WHITE, BLACK, console.font.glyph_count);
    }
    else if (strcmp(command, "reboot") == 0) {
        printf("Rebooting...\n", BLUE, BLACK);
        // Simple reboot via keyboard controller
        outb(0x64, 0xFE);
    }
    else if (strlen(command) > 0) {
        printf("Unknown command: %s\n", RED, BLACK, command);
        printf("Type 'help' for available commands.\n", GRAY, BLACK);
    }
}