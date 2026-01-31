#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Define implementation to build the library
#define KTERM_IMPLEMENTATION
#define KTERM_IO_SIT_IMPLEMENTATION // If needed for input config
// Minimal stubs if needed, but kterm.h includes dependencies.
// We assume standard includes are available.

#include "../kterm.h"

// Stub for platform specific if needed (none used in this test)

int main() {
    KTermConfig config = {0};
    // Initialize KTerm
    // Note: KTerm_Create calls KTerm_Init which might fail if dependencies missing,
    // but for unit test of logic it might pass if no backend is strictly required.
    // kterm depends on stb_truetype.h (header only) and kt_render_sit.h (header only).

    KTerm* term = KTerm_Create(config);
    if (!term) {
        printf("Failed to create KTerm\n");
        return 1;
    }

    KTermSession* session = &term->sessions[0];
    session->charset.g0 = CHARSET_UTF8; // Enable UTF-8
    session->dec_modes |= KTERM_MODE_DECAWM; // Ensure wrap is on
    session->left_margin = 0;
    session->right_margin = term->width - 1; // 131 by default

    // Test 0: Default state (Fixed Width 1 for everything)
    session->enable_wide_chars = false; // Explicitly set to default
    session->cursor.x = 0;
    // '中' (Width 2 if enabled)
    KTerm_ProcessNormalChar(term, session, 0xE4);
    KTerm_ProcessNormalChar(term, session, 0xB8);
    KTerm_ProcessNormalChar(term, session, 0xAD);

    if (session->cursor.x != 1) {
        printf("Test 0 Failed: Default behavior for wide char should be width 1, got %d\n", session->cursor.x);
        return 1;
    }
    printf("Test 0 Passed: Default Fixed Width (1)\n");

    // Enable Wide Chars for subsequent tests
    session->enable_wide_chars = true;

    // Test 1: ASCII 'A'
    session->cursor.x = 0;
    KTerm_ProcessNormalChar(term, session, 'A');
    if (session->cursor.x != 1) {
        printf("Test 1 Failed: 'A' should advance cursor to 1, got %d\n", session->cursor.x);
        return 1;
    }
    printf("Test 1 Passed: 'A' (Width 1)\n");

    // Test 2: CJK '中' (U+4E2D)
    // UTF-8: E4 B8 AD
    // Reset cursor
    session->cursor.x = 0;
    KTerm_ProcessNormalChar(term, session, 0xE4);
    KTerm_ProcessNormalChar(term, session, 0xB8);
    KTerm_ProcessNormalChar(term, session, 0xAD);

    if (session->cursor.x != 2) {
        printf("Test 2 Failed: '中' should advance cursor to 2, got %d\n", session->cursor.x);
        return 1;
    }
    printf("Test 2 Passed: '中' (Width 2)\n");

    // Test 3: Combining Char (U+0301 Combining Acute Accent)
    // UTF-8: CC 81
    // Reset cursor to 1 (simulate after 'A')
    session->cursor.x = 1;
    KTerm_ProcessNormalChar(term, session, 0xCC);
    KTerm_ProcessNormalChar(term, session, 0x81);

    if (session->cursor.x != 1) {
         printf("Test 3 Failed: Combining char should not advance cursor, got %d\n", session->cursor.x);
         return 1;
    }
    printf("Test 3 Passed: Combining Char (Width 0)\n");

    // Test 4: Wrap with Wide Char
    // Move to right margin (last valid column)
    session->cursor.x = session->right_margin; // 131
    // Print CJK '中' (Width 2). Should wrap.
    // Width 2 requires 2 cells. We have 1 cell at 131.
    // So it should wrap to next line, start at left_margin (0), occupy 2 cells (0, 1).
    // Cursor should end up at 2.
    int old_y = session->cursor.y;

    KTerm_ProcessNormalChar(term, session, 0xE4);
    KTerm_ProcessNormalChar(term, session, 0xB8);
    KTerm_ProcessNormalChar(term, session, 0xAD);

    if (session->cursor.y != old_y + 1) {
        printf("Test 4 Failed: Should wrap to next line, got y=%d (old=%d)\n", session->cursor.y, old_y);
        return 1;
    }
    if (session->cursor.x != 2) {
        printf("Test 4 Failed: Should be at x=2 after wrap, got %d\n", session->cursor.x);
        return 1;
    }
    printf("Test 4 Passed: Wide Char Wrap\n");

    // Test 5: Verify no-wrap behavior
    // Turn off DECAWM
    session->dec_modes &= ~KTERM_MODE_DECAWM;
    session->cursor.x = session->right_margin;
    int current_y = session->cursor.y;

    // Print wide char
    KTerm_ProcessNormalChar(term, session, 0xE4);
    KTerm_ProcessNormalChar(term, session, 0xB8);
    KTerm_ProcessNormalChar(term, session, 0xAD);

    // Should NOT wrap.
    if (session->cursor.y == current_y) {
        printf("Test 5 Passed: No wrap occured.\n");
    } else {
        printf("Test 5 Failed: Wrapped despite DECAWM off (y=%d -> %d)\n", current_y, session->cursor.y);
        return 1;
    }

    // Test 6: Non-UTF8 Mode (ASCII) - Width should always be 1
    // Reset
    KTerm_Destroy(term);
    term = KTerm_Create(config);
    session = &term->sessions[0];
    session->dec_modes |= KTERM_MODE_DECAWM;
    session->charset.g0 = CHARSET_ASCII; // Standard ASCII

    // Send 0x80 (C1 control). In UTF-8 it's invalid start, in ASCII extended it might be raw.
    // wcwidth(0x80) is -1.
    // If we rely on wcwidth, it would return -1 -> 1.
    // If we skip wcwidth (because not UTF8), it defaults to 1.
    // Result is the same (1).
    // Send 0xC4 (Latin-1 'Ä' or CP437 line). wcwidth(0xC4) is 1.

    // Let's test assumption: If not UTF-8, we don't call wcwidth.
    // We can verify by sending something that wcwidth says is 0 or 2, but we want 1?
    // Hard to find a single byte in a legacy set that is "wide".
    // Legacy sets are fixed width 1.
    // But maybe we can verify that 0x80 advances cursor by 1.

    session->cursor.x = 0;
    KTerm_ProcessNormalChar(term, session, 0x80);
    if (session->cursor.x != 1) {
        printf("Test 6 Failed: Non-UTF8 0x80 should have width 1, got %d\n", session->cursor.x);
        return 1;
    }
    printf("Test 6 Passed: Non-UTF8 Width Default\n");

    KTerm_Destroy(term);
    return 0;
}
