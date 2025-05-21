#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "psf.h"

#define FONT_SCALE 2  // Default scaling factor for the font

// Print colors
#define WHITE 0xFFFFFF
#define GRAY 0xAAAAAA
#define GREEN 0x008000  
#define BLACK 0x000000  
#define RED 0xFF0000
#define BLUE 0x0000FF

// Console state
struct console_state {
    struct limine_framebuffer* fb;
    size_t cursor_x;
    size_t cursor_y;
    size_t width;       // Width in characters
    size_t height;      // Height in characters
    uint32_t fg_color;
    uint32_t bg_color;
    unsigned int scale; // Font scaling factor
    struct psf_font font;  // Current font
};

int load_font(struct psf_font* font, void* font_data, size_t font_size);
void init_shell(struct limine_framebuffer* framebuffer);
void console_clear(void);
void putChar(char c, unsigned int fg_color, unsigned int bg_color);
void printf(const char* str, unsigned int fg_color, unsigned int bg_color);
void prompt(void);

#endif