#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../tests/mock_situation.h"
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

void write_sequence(KTerm* term, const char* seq) {
    while (*seq) {
        KTerm_ProcessChar(term, GET_SESSION(term), *seq++);
    }
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    KTermSession* session = GET_SESSION(term);
    KTerm_SetLevel(term, session, VT_LEVEL_510);

    // Ensure Local Echo is OFF initially
    session->dec_modes &= ~KTERM_MODE_LOCALECHO;
    // Ensure Mode 67 is OFF initially
    session->dec_modes &= ~KTERM_MODE_DECHDPXM;

    // Test 1: No Echo
    KTermEvent event1 = {0};
    event1.key_code = 'A';
    snprintf(event1.sequence, sizeof(event1.sequence), "A");
    KTerm_QueueInputEvent(term, event1);

    KTerm_Update(term);

    EnhancedTermChar* cell = GetActiveScreenCell(session, 0, 0);
    // Note: Default char is space
    if (cell->ch == 'A') {
        printf("FAIL: 'A' echoed when Local Echo is OFF\n");
        return 1;
    }

    // Test 2: Enable Mode 103 (DECHDPXM)
    write_sequence(term, "\x1B[?103h");

    if (!(session->dec_modes & KTERM_MODE_DECHDPXM)) {
        printf("FAIL: Mode 103 (DECHDPXM) not set\n");
        return 1;
    }

    // Test 3: Echo with Mode 67
    KTermEvent event2 = {0};
    event2.key_code = 'B';
    snprintf(event2.sequence, sizeof(event2.sequence), "B");
    KTerm_QueueInputEvent(term, event2);

    KTerm_Update(term);
    KTerm_Update(term); // Run again to process the echoed character from pipeline

    // Cursor should have advanced if B was echoed.
    // If A was not echoed, cursor is at 0,0.
    // If B is echoed, it writes 'B' at 0,0 and moves to 0,1.
    cell = GetActiveScreenCell(session, 0, 0);
    if (cell->ch != 'B') {
        printf("FAIL: 'B' not echoed when Mode 67 is ON. char=%c\n", cell->ch);
        return 1;
    }

    // Test 4: Disable Mode 103
    write_sequence(term, "\x1B[?103l");

    if (session->dec_modes & KTERM_MODE_DECHDPXM) {
        printf("FAIL: Mode 103 not cleared\n");
        return 1;
    }

    // Test 5: No Echo Again
    KTermEvent event3 = {0};
    event3.key_code = 'C';
    snprintf(event3.sequence, sizeof(event3.sequence), "C");
    KTerm_QueueInputEvent(term, event3);

    KTerm_Update(term);

    // Cursor should NOT have moved from where 'B' left it (0,1).
    // And 0,1 should be empty (or default).
    cell = GetActiveScreenCell(session, 0, 1);
    if (cell->ch == 'C') {
        printf("FAIL: 'C' echoed after Mode 67 disabled\n");
        return 1;
    }

    printf("PASS: DECHDPXM Mode 67 works\n");
    KTerm_Destroy(term);
    return 0;
}
