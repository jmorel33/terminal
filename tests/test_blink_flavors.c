#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

void test_blink_flavors(KTerm* term) {
    KTermSession* session = GET_SESSION(term);

    // 1. Reset
    KTerm_ResetAllAttributes(term);
    assert(session->current_attributes == 0);
    printf("Reset verified.\n");

    // 2. Test SGR 6 (Slow blink) -> FG Blink Slow only
    // Expect: KTERM_ATTR_BLINK_SLOW set. KTERM_ATTR_BLINK NOT set. KTERM_ATTR_BLINK_BG NOT set.
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '6');
    KTerm_ProcessChar(term, session, 'm');

    if (!(session->current_attributes & KTERM_ATTR_BLINK_SLOW)) {
        fprintf(stderr, "FAIL: SGR 6 did not set Blink Slow\n");
        exit(1);
    }
    if (session->current_attributes & KTERM_ATTR_BLINK) {
        fprintf(stderr, "FAIL: SGR 6 set Classic Blink incorrectly\n");
        exit(1);
    }
    if (session->current_attributes & KTERM_ATTR_BLINK_BG) {
        fprintf(stderr, "FAIL: SGR 6 set BG Blink incorrectly\n");
        exit(1);
    }
    printf("SGR 6 (Slow Blink) passed independent attribute check.\n");

    // 3. Test SGR 5 (Classic blink) overwriting SGR 6
    // Expect: KTERM_ATTR_BLINK set. KTERM_ATTR_BLINK_BG set. KTERM_ATTR_BLINK_SLOW CLEARED.
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '5');
    KTerm_ProcessChar(term, session, 'm');

    if (!(session->current_attributes & KTERM_ATTR_BLINK)) {
        fprintf(stderr, "FAIL: SGR 5 did not set Classic Blink\n");
        exit(1);
    }
    if (!(session->current_attributes & KTERM_ATTR_BLINK_BG)) {
        fprintf(stderr, "FAIL: SGR 5 did not set BG Blink\n");
        exit(1);
    }
    if (session->current_attributes & KTERM_ATTR_BLINK_SLOW) {
        fprintf(stderr, "FAIL: SGR 5 did not clear Blink Slow\n");
        exit(1);
    }
    printf("SGR 5 (Classic Blink) overwriting Slow passed.\n");

    // 4. Test SGR 25 (Blink off) -> Clears all
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '2');
    KTerm_ProcessChar(term, session, '5');
    KTerm_ProcessChar(term, session, 'm');

    if (session->current_attributes & (KTERM_ATTR_BLINK | KTERM_ATTR_BLINK_BG | KTERM_ATTR_BLINK_SLOW)) {
        fprintf(stderr, "FAIL: SGR 25 did not clear all blink attributes\n");
        exit(1);
    }
    printf("SGR 25 (Blink Off) passed.\n");

    // 5. Test SGR 105 (Blink Background) independent of FG speed
    // Set SGR 6 (Slow FG)
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '6');
    KTerm_ProcessChar(term, session, 'm');
    // Set SGR 105 (BG Blink)
    KTerm_ProcessChar(term, session, '\x1B');
    KTerm_ProcessChar(term, session, '[');
    KTerm_ProcessChar(term, session, '1');
    KTerm_ProcessChar(term, session, '0');
    KTerm_ProcessChar(term, session, '5');
    KTerm_ProcessChar(term, session, 'm');

    if (!(session->current_attributes & KTERM_ATTR_BLINK_SLOW)) {
        fprintf(stderr, "FAIL: SGR 105 cleared Blink Slow (FG)\n");
        exit(1);
    }
    if (!(session->current_attributes & KTERM_ATTR_BLINK_BG)) {
        fprintf(stderr, "FAIL: SGR 105 did not set Blink BG\n");
        exit(1);
    }
    printf("SGR 6 + SGR 105 combination passed.\n");

    printf("ALL BLINK FLAVOR TESTS PASSED.\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    test_blink_flavors(term);

    KTerm_Destroy(term);
    return 0;
}
