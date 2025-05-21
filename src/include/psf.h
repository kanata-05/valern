#ifndef PSF_H
#define PSF_H

#include <stdint.h>
#include <stddef.h>

// PSF1 magic bytes
#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04

// PSF2 magic bytes
#define PSF2_MAGIC0 0x72
#define PSF2_MAGIC1 0xb5
#define PSF2_MAGIC2 0x4a
#define PSF2_MAGIC3 0x86

struct psf1_header {
    uint8_t magic[2];     // Magic bytes for identification
    uint8_t mode;         // Mode bits
    uint8_t charsize;     // Character size in bytes
};

struct psf2_header {
    uint8_t magic[4];     // Magic bytes for identification
    uint32_t version;     // Zero
    uint32_t headersize;  // Header size in bytes
    uint32_t flags;       // Format flags
    uint32_t length;      // Number of glyphs
    uint32_t charsize;    // Character size in bytes
    uint32_t height;      // Character height in pixels
    uint32_t width;       // Character width in pixels
};

// Font data structure
struct psf_font {
    void* header;         // PSF header (either version)
    void* glyph_buffer;   // Font bitmap data
    uint32_t glyph_count; // Number of glyphs
    uint32_t glyph_size;  // Size of each glyph in bytes
    uint32_t width;       // Character width in pixels
    uint32_t height;      // Character height in pixels
    uint8_t version;      // PSF version (1 or 2)
};

// Function declarations
int psf_load_font(struct psf_font* font, void* data, size_t size);
void psf_unload_font(struct psf_font* font);
const uint8_t* psf_get_glyph(struct psf_font* font, unsigned int c);

#endif
