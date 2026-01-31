#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include "mock_situation.h"
#include <assert.h>
#include <stdio.h>

void test_combining_char_storage(void) {
    KTermConfig config = {0};
    KTerm* term = KTerm_Create(config);
    KTermSession* session = &term->sessions[term->active_session];

    // Enable Wide Chars / Unicode Width logic
    // This is required for KTerm_wcwidth to return 0 for combining chars
    session->enable_wide_chars = true;
    session->charset.g0 = CHARSET_UTF8;

    // 1. Write 'e' (Base Char)
    KTerm_WriteChar(term, 'e');
    KTerm_Update(term); // Flush ops

    EnhancedTermChar* cell0 = GetScreenCell(session, 0, 0);
    assert(cell0->ch == 'e');
    assert(!(cell0->flags & KTERM_FLAG_COMBINING));
    assert(session->cursor.x == 1);

    // 2. Write Combining Acute Accent (U+0301)
    // UTF-8: 0xCC 0x81
    KTerm_WriteChar(term, 0xCC);
    KTerm_WriteChar(term, 0x81);
    KTerm_Update(term);

    // Verify storage
    // 'e' should still be at 0,0
    cell0 = GetScreenCell(session, 0, 0);
    assert(cell0->ch == 'e');

    // Accent should be at 0,1
    EnhancedTermChar* cell1 = GetScreenCell(session, 0, 1);
    // 0x0301
    if (cell1->ch != 0x0301) {
        printf("FAILURE: Expected U+0301 at (0,1), got 0x%X\n", cell1->ch);
    }
    assert(cell1->ch == 0x0301);

    // Check Flag
    if (!(cell1->flags & KTERM_FLAG_COMBINING)) {
        printf("FAILURE: KTERM_FLAG_COMBINING not set for U+0301\n");
    }
    assert(cell1->flags & KTERM_FLAG_COMBINING);

    // Check Cursor
    // Should have advanced to 2 (storage index)
    if (session->cursor.x != 2) {
        printf("FAILURE: Cursor at %d, expected 2 (advanced past combining char)\n", session->cursor.x);
    }
    assert(session->cursor.x == 2);

    printf("SUCCESS: Combining char stored correctly with flag.\n");

    KTerm_Destroy(term);
}

int main() {
    test_combining_char_storage();
    return 0;
}
