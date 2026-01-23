#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

void test_ansi_sys_sgr_restrictions(KTerm* term) {
    KTermSession* session = GET_SESSION(term);

    // 1. Set Level to ANSI.SYS
    KTerm_SetLevel(term, VT_LEVEL_ANSI_SYS);
    KTerm_ResetAllAttributes(term);

    // 2. Test SGR 5 (Blink) - Should work (Standard ANSI)
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '5');
    KTerm_ProcessChar(term, session, 'm');

    if (!(session->current_attributes & KTERM_ATTR_BLINK)) {
        fprintf(stderr, "FAIL: SGR 5 (Blink) should be supported in ANSI.SYS mode\n");
        exit(1);
    }
    printf("SGR 5 supported in ANSI.SYS mode.\n");

    // 3. Test SGR 101 (Bright Red BG) - Should be IGNORED (AIXterm extension, not ANSI.SYS)
    KTerm_ResetAllAttributes(term);
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '1');
    KTerm_ProcessChar(term, session, '0');
    KTerm_ProcessChar(term, session, '1');
    KTerm_ProcessChar(term, session, 'm');

    // Default BG is 0 (Black). If 101 worked, it would be 9 (Bright Red).
    if (session->current_bg.value.index != 0) {
        fprintf(stderr, "FAIL: SGR 101 (Bright BG) should be ignored in strict ANSI.SYS mode. Got index %d\n", session->current_bg.value.index);
        exit(1);
    }
    printf("SGR 101 ignored in ANSI.SYS mode.\n");

    // 4. Test SGR 90-97 (Bright FG) - Should be IGNORED (AIXterm)
    // ANSI.SYS uses SGR 1 (Bold) for bright FG.
    KTerm_ResetAllAttributes(term);
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '9');
    KTerm_ProcessChar(term, session, '1');
    KTerm_ProcessChar(term, session, 'm');

    if (session->current_fg.value.index != 7) { // Default White (7)
        fprintf(stderr, "FAIL: SGR 91 (Bright FG) should be ignored in strict ANSI.SYS mode. Got index %d\n", session->current_fg.value.index);
        exit(1);
    }
    printf("SGR 91 ignored in ANSI.SYS mode.\n");

    // 5. Test SGR 66 (Custom BG Blink) - Should be IGNORED (Not standard)
    KTerm_ResetAllAttributes(term);
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '6');
    KTerm_ProcessChar(term, session, '6');
    KTerm_ProcessChar(term, session, 'm');

    if (session->current_attributes & KTERM_ATTR_BLINK_BG) {
        fprintf(stderr, "FAIL: SGR 66 (Custom BG Blink) should be ignored in strict ANSI.SYS mode\n");
        exit(1);
    }
    printf("SGR 66 ignored in ANSI.SYS mode.\n");

    printf("ALL ANSI.SYS COMPLIANCE TESTS PASSED.\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    test_ansi_sys_sgr_restrictions(term);

    KTerm_Destroy(term);
    return 0;
}
