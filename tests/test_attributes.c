#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

void test_sgr_attributes(KTerm* term) {
    KTermSession* session = GET_SESSION(term);

    // 1. Reset
    KTerm_ResetAllAttributes(term);
    assert(session->current_attributes == 0);

    // 2. Set Bold (CSI 1 m)
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '1');
    KTerm_ProcessChar(term, session, 'm');

    if (!(session->current_attributes & KTERM_ATTR_BOLD)) {
        fprintf(stderr, "FAIL: Bold attribute not set\n");
        exit(1);
    }

    // 3. Set Italic (CSI 3 m)
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '3');
    KTerm_ProcessChar(term, session, 'm');

    if (!(session->current_attributes & KTERM_ATTR_ITALIC)) {
        fprintf(stderr, "FAIL: Italic attribute not set\n");
        exit(1);
    }
    // Check Bold persisted
    if (!(session->current_attributes & KTERM_ATTR_BOLD)) {
        fprintf(stderr, "FAIL: Bold attribute lost\n");
        exit(1);
    }

    // 4. Clear Bold (CSI 22 m)
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '2');
    KTerm_ProcessChar(term, session, '2');
    KTerm_ProcessChar(term, session, 'm');

    if (session->current_attributes & KTERM_ATTR_BOLD) {
        fprintf(stderr, "FAIL: Bold attribute not cleared\n");
        exit(1);
    }
    // Check Italic persisted
    if (!(session->current_attributes & KTERM_ATTR_ITALIC)) {
        fprintf(stderr, "FAIL: Italic attribute lost after clearing bold\n");
        exit(1);
    }

    // 5. Output Character and verify cell attributes
    KTerm_ProcessChar(term, session, 'A');
    int x = session->cursor.x - 1; // Cursor moved
    if (x < 0) x = 0; // Should be 0 -> 1

    EnhancedTermChar* cell = GetScreenCell(session, session->cursor.y, x);
    if (!(cell->flags & KTERM_ATTR_ITALIC)) {
        fprintf(stderr, "FAIL: Cell did not inherit Italic attribute\n");
        exit(1);
    }
    if (cell->flags & KTERM_ATTR_BOLD) {
        fprintf(stderr, "FAIL: Cell inherited cleared Bold attribute\n");
        exit(1);
    }

    printf("PASS: SGR Attributes\n");
}

void test_protected_attribute(KTerm* term) {
    KTermSession* session = GET_SESSION(term);
    KTerm_ResetAllAttributes(term);

    // DECSCA 1 (Protected)
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '1');
    KTerm_ProcessChar(term, session, '"');
    KTerm_ProcessChar(term, session, 'q');

    if (!(session->current_attributes & KTERM_ATTR_PROTECTED)) {
        fprintf(stderr, "FAIL: Protected attribute not set by DECSCA\n");
        exit(1);
    }

    // Write Char
    KTerm_ProcessChar(term, session, 'P');
    EnhancedTermChar* cell = GetScreenCell(session, session->cursor.y, session->cursor.x - 1);
    if (!(cell->flags & KTERM_ATTR_PROTECTED)) {
        fprintf(stderr, "FAIL: Cell did not inherit Protected attribute\n");
        exit(1);
    }

    printf("PASS: Protected Attribute\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    test_sgr_attributes(term);
    test_protected_attribute(term);

    KTerm_Destroy(term);
    printf("All Attribute Tests Passed\n");
    return 0;
}
