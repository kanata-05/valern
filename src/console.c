#include "console.h"
#include "stdmem.h"
#include "vga_font.h"

static struct console_state console;

void console_init(struct limine_framebuffer* framebuffer) {
    console.fb = framebuffer;
    console.cursor_x = 0;
    console.cursor_y = 0;
    console.width = framebuffer->width / FONT_WIDTH;
    console.height = framebuffer->height / FONT_HEIGHT;
    console.fg_color = DEFAULT_FG_COLOR;
    console.bg_color = DEFAULT_BG_COLOR;
    
    console_clear();
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

static void draw_char(char c, size_t x, size_t y) {    volatile uint32_t* fb = console.fb->address;
    size_t fb_x = x * FONT_WIDTH;
    size_t fb_y = y * FONT_HEIGHT;
    
    for (size_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t glyph_row = vga_font[(size_t)c][row];
        for (size_t col = 0; col < FONT_WIDTH; col++) {
            size_t pixel_x = fb_x + col;
            size_t pixel_y = fb_y + row;
            
            if (pixel_x >= console.fb->width || pixel_y >= console.fb->height) continue;
            
            uint32_t color = (glyph_row & (1 << (7 - col))) ? console.fg_color : console.bg_color;
            fb[pixel_y * (console.fb->pitch / 4) + pixel_x] = color;
        }
    }
}

void console_putchar(char c) {
    if (c == '\n') {
        console.cursor_x = 0;
        console.cursor_y++;
    } else if (c == '\r') {
        console.cursor_x = 0;
    } else {
        draw_char(c, console.cursor_x, console.cursor_y);
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

void console_write(const char* str) {
    while (*str) {
        console_putchar(*str++);
    }
}

void console_draw_prompt(void) {
    console_write("valern> ");
}