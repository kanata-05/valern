// VGA 8x16 font
// This is the classic VGA font from the demoscene and early PC era
// Source: http://www.spaceballs.org/vgafont.html
// This font is in the public domain

#ifndef VGA_FONT_H
#define VGA_FONT_H

#include <stdint.h>

#define VGA_FONT_HEIGHT 16
#define VGA_FONT_WIDTH 8

// The actual font data
extern const uint8_t vga_font[256][16];

#endif
