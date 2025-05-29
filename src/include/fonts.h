#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

#define VGA_FONT_HEIGHT 16
#define VGA_FONT_WIDTH 8

// Symbols for embedded PSF font
extern char _binary_src_fonts_default_psf_start[];
extern char _binary_src_fonts_default_psf_end[];
extern char _binary_src_fonts_default_psf_size[];

// The actual font data
extern uint8_t vga_font[256][16];  

#endif // FONTS_H
