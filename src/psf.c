#include "psf.h"
#include "stdmem.h"

int psf_load_font(struct psf_font* font, void* data, size_t size) {
    if (!font || !data || size < 4) return -1;
    
    uint8_t* bytes = (uint8_t*)data;
    
    // Check PSF1 magic
    if (bytes[0] == PSF1_MAGIC0 && bytes[1] == PSF1_MAGIC1) {
        struct psf1_header* header = (struct psf1_header*)data;
        font->version = 1;
        font->width = 8;  // PSF1 fonts are always 8 pixels wide
        font->height = header->charsize;
        font->glyph_size = header->charsize;
        font->glyph_count = header->mode == 0 ? 256 : 512;
        font->header = header;
        font->glyph_buffer = bytes + sizeof(struct psf1_header);
        return 0;
    }
    
    // Check PSF2 magic
    if (bytes[0] == PSF2_MAGIC0 && bytes[1] == PSF2_MAGIC1 &&
        bytes[2] == PSF2_MAGIC2 && bytes[3] == PSF2_MAGIC3) {
        struct psf2_header* header = (struct psf2_header*)data;
        font->version = 2;
        font->width = header->width;
        font->height = header->height;
        font->glyph_size = header->charsize;
        font->glyph_count = header->length;
        font->header = header;
        font->glyph_buffer = bytes + header->headersize;
        return 0;
    }
    
    return -1;  // Invalid PSF magic numbers
}

void psf_unload_font(struct psf_font* font) {
    if (font) {
        font->header = NULL;
        font->glyph_buffer = NULL;
        font->glyph_count = 0;
        font->glyph_size = 0;
        font->width = 0;
        font->height = 0;
        font->version = 0;
    }
}

const uint8_t* psf_get_glyph(struct psf_font* font, unsigned int c) {
    if (!font || !font->glyph_buffer || c >= font->glyph_count)
        return NULL;
    
    return (const uint8_t*)font->glyph_buffer + (c * font->glyph_size);
}
