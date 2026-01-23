#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void test_default_tabs(KTerm* term) {
    // Default tabs should be at 8, 16, 24...
    // Start at 0. Next should be 8.
    int next = NextTabStop(term, 0);
    if (next != 8) {
        fprintf(stderr, "FAIL: Default tab from 0 should be 8, got %d\n", next);
        exit(1);
    }
    next = NextTabStop(term, 8);
    if (next != 16) {
        fprintf(stderr, "FAIL: Default tab from 8 should be 16, got %d\n", next);
        exit(1);
    }
    printf("PASS: Default tabs\n");
}

void test_clear_all_tabs(KTerm* term) {
    // TBC 3 -> CSI 3 g
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '3');
    KTerm_ProcessChar(term, GET_SESSION(term), 'g');

    // With all tabs cleared, NextTabStop should jump to end or right margin
    int next = NextTabStop(term, 0);
    // If we clear all tabs, standard behavior depends on implementation (often right margin)
    // The previous implementation used math fallback, so it would STILL return 8!
    // We expect it to be width-1 (or similar) if we strictly follow "no tabs".

    // In strict implementation, if no tabs, it goes to right margin.
    int expected = term->width - 1;

    if (next != expected) {
        fprintf(stderr, "FAIL: After TBC 3, NextTabStop(0) should be %d, got %d. (Is legacy fallback logic active?)\n", expected, next);
        exit(1);
    }
    printf("PASS: Clear all tabs\n");
}

void test_set_tab(KTerm* term) {
    // Set tab at 4: ESC H
    GET_SESSION(term)->cursor.x = 4;
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), 'H');

    int next = NextTabStop(term, 0);
    if (next != 4) {
        fprintf(stderr, "FAIL: After HTS at 4, NextTabStop(0) should be 4, got %d\n", next);
        exit(1);
    }
    printf("PASS: Set tab stop\n");
}

void test_resize_tabs(KTerm* term) {
    // Resize to 300.
    // Check if tabs exist beyond 256.
    // Currently MAX_TAB_STOPS is 256.

    KTerm_Resize(term, 300, 25);

    // Check tab at 296 (assuming default tabs are populated or retained logic).
    // The plan says we should support > 256.
    // Let's verify if we can set a tab at 290.

    // Clear all first to be clean
    KTerm_ClearAllTabStops(term);

    // Set at 290
    GET_SESSION(term)->cursor.x = 290;
    KTerm_SetTabStop(term, 290); // Direct call or via escape

    int next = NextTabStop(term, 280);
    if (next != 290) {
        fprintf(stderr, "FAIL: Tab stop at 290 (beyond old MAX) failed. Got %d\n", next);
        exit(1);
    }
    printf("PASS: Resize & Large Tab support\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 100;
    config.height = 25;

    KTerm* term = KTerm_Create(config);
    if (!term) {
        fprintf(stderr, "Failed to create terminal\n");
        return 1;
    }

    test_default_tabs(term);
    test_clear_all_tabs(term); // This should fail with current code due to fallback logic
    test_set_tab(term);

    // Reset for resize test
    KTerm_Destroy(term);
    term = KTerm_Create(config);
    test_resize_tabs(term); // This should fail due to MAX_TAB_STOPS limit

    KTerm_Destroy(term);
    printf("All Tab Stop tests passed.\n");
    return 0;
}
