#include "console.h"
#include "stdmem.h"
#include "fonts.h"
#include <stdint.h>

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
        printf("PSF font loading failed!\n Falling back to VGA font\n", RED, BLACK);
        // PSF font loading failed, fall back to VGA font
        console.font.width = 8;  // VGA font is 8x16
        console.font.height = 16;
     // console.font.glyph_buffer = vga_font; This is currently commented out for .psf file testing.
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

void printf(const char* str, unsigned int fg_color, unsigned int bg_color) {
    while (*str) {
        putChar(*str++, fg_color, bg_color);
    }
}

void prompt(void) {
    printf("valern> ", GREEN, BLACK);
}