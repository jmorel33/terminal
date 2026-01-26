#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_cursor_save_restore(KTerm* term) {
    // Move cursor to 5,5
    GET_SESSION(term)->cursor.x = 5;
    GET_SESSION(term)->cursor.y = 5;

    // Save Cursor (ANSI.SYS)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), 's');

    // Move cursor elsewhere
    GET_SESSION(term)->cursor.x = 10;
    GET_SESSION(term)->cursor.y = 10;

    // Restore Cursor (ANSI.SYS)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), 'u');

    if (GET_SESSION(term)->cursor.x != 5 || GET_SESSION(term)->cursor.y != 5) {
        fprintf(stderr, "FAIL: Cursor restore failed. Expected 5,5, Got %d,%d\n", GET_SESSION(term)->cursor.x, GET_SESSION(term)->cursor.y);
        exit(1);
    }
    printf("PASS: Cursor Save/Restore (ANSI.SYS)\n");
}

void test_private_modes_ignored(KTerm* term) {
    // 1. Ensure DECCKM (Private Mode 1) is ignored
    GET_SESSION(term)->dec_modes &= ~KTERM_MODE_DECCKM;

    // Send CSI ? 1 h
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '?');
    KTerm_ProcessChar(term, GET_SESSION(term), '1');
    KTerm_ProcessChar(term, GET_SESSION(term), 'h');

    if (GET_SESSION(term)->dec_modes & KTERM_MODE_DECCKM) {
        fprintf(stderr, "FAIL: DECCKM (Private Mode 1) should be ignored in ANSI.SYS mode\n");
        exit(1);
    }
    printf("PASS: Private Modes Ignored\n");
}

void test_standard_line_wrap(KTerm* term) {
    // 1. Disable Auto Wrap
    GET_SESSION(term)->dec_modes &= ~KTERM_MODE_DECAWM;

    // Send CSI 7 h (Standard Mode 7)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '7');
    KTerm_ProcessChar(term, GET_SESSION(term), 'h');

    if (!(GET_SESSION(term)->dec_modes & KTERM_MODE_DECAWM)) {
        fprintf(stderr, "FAIL: Standard Mode 7 should enable Auto Wrap in ANSI.SYS mode\n");
        exit(1);
    }

    // Send CSI 7 l (Standard Mode 7)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '7');
    KTerm_ProcessChar(term, GET_SESSION(term), 'l');

    if (GET_SESSION(term)->dec_modes & KTERM_MODE_DECAWM) {
        fprintf(stderr, "FAIL: Standard Mode 7 (l) should disable Auto Wrap\n");
        exit(1);
    }

    printf("PASS: Standard Mode 7 (Line Wrap) supported\n");
}

void test_cga_palette_enforcement(KTerm* term) {
    // Check color 3 (Brown)
    RGB_KTermColor brown = term->color_palette[3];
    if (brown.r != 0xAA || brown.g != 0x55 || brown.b != 0x00) {
        fprintf(stderr, "FAIL: Color 3 (Brown) is incorrect. Expected AA,55,00. Got %02X,%02X,%02X\n", brown.r, brown.g, brown.b);
        exit(1);
    }

    // Check color 11 (Yellow)
    RGB_KTermColor yellow = term->color_palette[11];
    if (yellow.r != 0xFF || yellow.g != 0xFF || yellow.b != 0x55) {
        fprintf(stderr, "FAIL: Color 11 (Yellow) is incorrect. Expected FF,FF,55. Got %02X,%02X,%02X\n", yellow.r, yellow.g, yellow.b);
        exit(1);
    }
    printf("PASS: CGA Palette Enforcement\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;

    KTerm* term = KTerm_Create(config);
    if (!term) {
        fprintf(stderr, "Failed to create terminal\n");
        return 1;
    }

    // Set to IBM DOS ANSI Mode
    KTerm_SetLevel(term, VT_LEVEL_ANSI_SYS);

    // 1. Verify Font Auto-Switch
    if (term->char_width != 10 || term->char_height != 10) {
        fprintf(stderr, "FAIL: Font cell dimensions wrong for IBM mode. Got %dx%d, expected 10x10\n", term->char_width, term->char_height);
        return 1;
    }
    printf("PASS: IBM Font loaded automatically\n");

    // 2. Verify Answerback
    if (strcmp(GET_SESSION(term)->answerback_buffer, "ANSI.SYS") != 0) {
        fprintf(stderr, "FAIL: Answerback string wrong. Got '%s', expected 'ANSI.SYS'\n", GET_SESSION(term)->answerback_buffer);
        return 1;
    }
    printf("PASS: Answerback is ANSI.SYS\n");

    // 3. Verify DA suppression
    if (strlen(GET_SESSION(term)->device_attributes) > 0) {
        fprintf(stderr, "FAIL: Device Attributes should be empty for ANSI.SYS. Got '%s'\n", GET_SESSION(term)->device_attributes);
        return 1;
    }
    printf("PASS: Device Attributes suppressed\n");

    // 4. Verify Cursor Save/Restore
    test_cursor_save_restore(term);

    // 5. Verify Private Modes Ignored
    test_private_modes_ignored(term);

    // 6. Verify Standard Line Wrap
    test_standard_line_wrap(term);

    // 7. Verify CGA Palette
    test_cga_palette_enforcement(term);

    KTerm_Destroy(term);
    printf("All ANSI.SYS tests passed.\n");
    return 0;
}
