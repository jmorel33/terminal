#define KTERM_TESTING
#define KTERM_IMPLEMENTATION
#include "../kterm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Mock callbacks
void mock_response_callback(KTerm* term, const char* response, int length) {
    (void)term; (void)response; (void)length;
}

int main() {
    printf("Starting DECDLD Test...\n");

    KTermConfig config = {
        .width = 80,
        .height = 24,
        .response_callback = mock_response_callback
    };

    KTerm* term = KTerm_Create(config);
    assert(term != NULL);
    if (!KTerm_Init(term)) {
        printf("Init failed\n");
        return 1;
    }
    KTermSession* session = &term->sessions[term->active_session];

    // DECDLD Sequence: DCS 1;33;1 { @ A/B ST
    // Pcn=33 -> Start loading at Char 33 ('!')
    // Dscs='@' (Space @, but scanner might see just @ depending on spacing, test string has NO space)
    // Sixel 'A' (val 2, 000010) -> y=1 set.
    // Separator '/' -> Advance to Char 34.
    // Sixel 'B' (val 3, 000011) -> y=0, y=1 set.

    // Note: My parsing logic treats '/' and ';' as char separator.

    const char* decdld = "\x1BP1;33;1{@A/B\x1B\\";

    printf("Sending DECDLD sequence...\n");
    KTerm_WriteString(term, decdld);
    KTerm_ProcessEvents(term);

    printf("Soft Font Dirty: %d\n", session->soft_font.dirty);
    assert(session->soft_font.dirty == true);

    // Check Dscs
    printf("Soft Font Name: '%s'\n", session->soft_font.name);
    // Should be "@"
    assert(strcmp(session->soft_font.name, "@") == 0);

    // Check Font Data
    // Char 33
    // Sixel 'A' -> 2 -> bit 1 set (0-indexed 0..5).
    // y=1. col=0.
    // font_data is 8-bit wide. col 0 is MSB (0x80).
    // So font_data[33][1] should have 0x80 bit set.
    unsigned char byte_33_1 = session->soft_font.font_data[33][1];
    printf("Char 33 Row 1: 0x%02X\n", byte_33_1);
    assert((byte_33_1 & 0x80) != 0);

    // Char 34
    // Sixel 'B' -> 3 -> bits 0,1 set.
    // y=0, y=1. col=0.
    unsigned char byte_34_0 = session->soft_font.font_data[34][0];
    unsigned char byte_34_1 = session->soft_font.font_data[34][1];
    printf("Char 34 Row 0: 0x%02X, Row 1: 0x%02X\n", byte_34_0, byte_34_1);
    assert((byte_34_0 & 0x80) != 0);
    assert((byte_34_1 & 0x80) != 0);

    // Trigger Atlas Update
    printf("Calling KTerm_Update to update atlas...\n");
    KTerm_Update(term);

    printf("Soft Font Dirty after Update: %d\n", session->soft_font.dirty);
    assert(session->soft_font.dirty == false); // Should be cleared

    // Verify Atlas Pixels
    // Atlas Width 1024. Cols 128. Char W 8.
    // Char 33.
    // Atlas X = (33 % 128) * 8 = 33 * 8 = 264.
    // Atlas Y = (33 / 128) * 16 = 0.
    // Pixel (264, 1).
    // Index = (1 * 1024 + 264) * 4.
    // Red component at Index + 0.

    // Note: KTerm_UpdateAtlasWithSoftFont calculates atlas positions.
    // It assumes session->soft_font.char_width is used.
    // Default initialized to 8.

    unsigned char* pixels = term->font_atlas_pixels;
    assert(pixels != NULL);

    int pixel_idx = (1 * 1024 + 264) * 4;
    unsigned char r = pixels[pixel_idx];
    printf("Atlas Pixel (264, 1) R: %d\n", r);
    assert(r == 255);

    // Check neighbor (265, 1) should be 0 (col 1)
    unsigned char r_next = pixels[pixel_idx + 4];
    printf("Atlas Pixel (265, 1) R: %d\n", r_next);
    assert(r_next == 0);

    KTerm_Destroy(term);
    printf("DECDLD Test PASS.\n");
    return 0;
}
