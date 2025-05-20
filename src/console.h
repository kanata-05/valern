#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "vga_font.h"

#define FONT_WIDTH VGA_FONT_WIDTH
#define FONT_HEIGHT VGA_FONT_HEIGHT
#define DEFAULT_FG_COLOR 0xAAAAAA  // Light gray, more authentic to old terminals
#define DEFAULT_BG_COLOR 0x000000  // Black

// Console state
struct console_state {
    struct limine_framebuffer* fb;
    size_t cursor_x;
    size_t cursor_y;
    size_t width;       // Width in characters
    size_t height;      // Height in characters
    uint32_t fg_color;
    uint32_t bg_color;
};

void console_init(struct limine_framebuffer* framebuffer);
void console_clear(void);
void console_putchar(char c);
void console_write(const char* str);
void console_draw_prompt(void);

#endif
