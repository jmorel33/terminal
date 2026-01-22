#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "kterm.h"

// Mock callbacks
void mock_response_callback(KTerm* term, const char* response, int length) {}

int main() {
    KTermConfig config = {
        .width = 80,
        .height = 24,
        .response_callback = mock_response_callback
    };

    KTerm* term = KTerm_Create(config);
    if (!term) {
        fprintf(stderr, "Failed to create KTerm\n");
        return 1;
    }

    printf("Testing Font Padding Logic...\n");

    // 1. Check Default (VT220)
    // Should be 8x10 cell, 8x10 data
    printf("Checking Default (VT220)...\n");
    if (term->char_width != 8 || term->char_height != 10) {
         fprintf(stderr, "FAIL: Default char dimensions wrong. Got %dx%d, expected 8x10\n", term->char_width, term->char_height);
         return 1;
    }
    if (term->font_data_width != 8 || term->font_data_height != 10) {
         fprintf(stderr, "FAIL: Default font data dimensions wrong. Got %dx%d, expected 8x10\n", term->font_data_width, term->font_data_height);
         return 1;
    }

    // 2. Switch to IBM (Should be 10x10 cell, 8x8 data)
    printf("Switching to IBM...\n");
    KTerm_SetFont(term, "IBM");

    if (term->char_width != 10 || term->char_height != 10) {
         fprintf(stderr, "FAIL: IBM char dimensions wrong. Got %dx%d, expected 10x10\n", term->char_width, term->char_height);
         return 1;
    }
    if (term->font_data_width != 8 || term->font_data_height != 8) {
         fprintf(stderr, "FAIL: IBM font data dimensions wrong. Got %dx%d, expected 8x8\n", term->font_data_width, term->font_data_height);
         return 1;
    }

    // 3. Verify Texture Generation logic didn't crash
    // We can inspect the atlas buffer roughly.
    // IBM char 'A' (0x41).
    // It should be centered.
    // cell=10, data=8. pad_x=1, pad_y=1.
    // So pixel at (0,0) of the cell should be OFF.
    // Pixel at (1,1) should correspond to (0,0) of font data.

    printf("Verifying Atlas buffer content...\n");

    unsigned char* pixels = term->font_atlas_pixels;
    int atlas_w = term->atlas_width;
    int char_w = term->char_width;
    int char_h = term->char_height;

    // Check all 256 chars for padding clearing
    int errors = 0;
    for (int i = 0; i < 256; i++) {
        int glyph_col = i % (atlas_w / char_w);
        int glyph_row = i / (atlas_w / char_w);
        int dest_x_start = glyph_col * char_w;
        int dest_y_start = glyph_row * char_h;

        // Top row of cell (y=0)
        for (int x = 0; x < char_w; x++) {
             int idx = ((dest_y_start + 0) * atlas_w + (dest_x_start + x)) * 4;
             if (pixels[idx] != 0) errors++;
        }
        // Bottom row of cell (y=9)
        for (int x = 0; x < char_w; x++) {
             int idx = ((dest_y_start + 9) * atlas_w + (dest_x_start + x)) * 4;
             if (pixels[idx] != 0) errors++;
        }
        // Left col of cell (x=0)
        for (int y = 0; y < char_h; y++) {
             int idx = ((dest_y_start + y) * atlas_w + (dest_x_start + 0)) * 4;
             if (pixels[idx] != 0) errors++;
        }
        // Right col of cell (x=9)
        for (int y = 0; y < char_h; y++) {
             int idx = ((dest_y_start + y) * atlas_w + (dest_x_start + 9)) * 4;
             if (pixels[idx] != 0) errors++;
        }
    }

    if (errors > 0) {
        fprintf(stderr, "FAIL: Found %d non-zero pixels in padding area for IBM font!\n", errors);
        return 1;
    }

    printf("PASS: IBM font padding verified.\n");

    KTerm_Destroy(term);
    return 0;
}
