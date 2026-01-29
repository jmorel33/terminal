#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static KTerm* term = NULL;

void ProcessString(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)str[i]);
    }
}

int main() {
    KTermConfig config = {0};
    term = KTerm_Create(config);

    // Initialize Session 1 (Session 0 is default active)
    if (!term->sessions[1].session_open) {
        KTerm_InitSession(term, 1);
        term->sessions[1].session_open = true;
    }

    // Set level to XTERM for Sixel support
    KTerm_SetLevel(term, &term->sessions[0], VT_LEVEL_XTERM);
    KTerm_SetLevel(term, &term->sessions[1], VT_LEVEL_XTERM);

    printf("Testing Gateway Sixel Integration...\n");

    // 1. Verify Default State
    assert(term->sixel_target_session == -1);
    assert(!term->sessions[0].sixel.active);
    assert(!term->sessions[1].sixel.active);

    // 2. Set Sixel Target to Session 1 via Gateway
    const char* set_target_seq = "\x1BPGATE;KTERM;0;SET;SIXEL_SESSION;1\x1B\\";
    ProcessString(set_target_seq);

    if (term->sixel_target_session != 1) {
        printf("FAIL: SET;SIXEL_SESSION;1 failed. Got %d\n", term->sixel_target_session);
        return 1;
    }
    printf("PASS: SET;SIXEL_SESSION;1\n");

    // 3. Send Sixel Data (while Session 0 is active)
    // DCS q (Sixel) ... ST
    // Simple Sixel: ? (000000) repeated 10 times
    const char* sixel_seq = "\x1BPq!10?\x1B\\";
    ProcessString(sixel_seq);

    // 4. Verify Routing
    if (term->sessions[0].sixel.active) {
        printf("FAIL: Sixel active on Session 0 (Source) - Should be routed\n");
        return 1;
    }
    if (!term->sessions[1].sixel.active) {
        printf("FAIL: Sixel NOT active on Session 1 (Target)\n");
        return 1;
    }
    if (term->sessions[1].sixel.strip_count == 0) {
        printf("FAIL: No Sixel strips on Session 1\n");
        return 1;
    }
    printf("PASS: Sixel routed to Session 1\n");

    // 5. Test RESET;SIXEL
    // Should reset Session 1 because target is set
    const char* reset_sixel_seq = "\x1BPGATE;KTERM;0;RESET;SIXEL\x1B\\";
    ProcessString(reset_sixel_seq);

    // active flag is usually not cleared by reset directly (it resets parse state/buffers),
    // but KTerm_InitSixelGraphics clears active.
    // KTerm_ResetGraphics calls KTerm_InitSixelGraphics.
    if (term->sessions[1].sixel.active) {
        printf("FAIL: RESET;SIXEL did not deactivate Sixel on Session 1\n");
        return 1;
    }
    printf("PASS: RESET;SIXEL on Target\n");

    // 6. Test INIT;SIXEL_SESSION (Re-init)
    // Initializes sixel for target (Session 1)
    // We expect it to be clean.
    const char* init_sixel_seq = "\x1BPGATE;KTERM;0;INIT;SIXEL_SESSION\x1B\\";
    ProcessString(init_sixel_seq);
    if (term->sessions[1].sixel.active) {
        printf("FAIL: INIT;SIXEL_SESSION should leave active=false (init state)\n");
        return 1;
    }
    // Set active manually to test reset by INIT if needed, but INIT usually clears.

    // 7. Test RESET;SIXEL_SESSION (Reset Target)
    const char* reset_target_seq = "\x1BPGATE;KTERM;0;RESET;SIXEL_SESSION\x1B\\";
    ProcessString(reset_target_seq);

    if (term->sixel_target_session != -1) {
        printf("FAIL: RESET;SIXEL_SESSION failed. Got %d\n", term->sixel_target_session);
        return 1;
    }
    printf("PASS: RESET;SIXEL_SESSION\n");

    // 8. Verify Sixel goes to Session 0 (Active) after target reset
    ProcessString(sixel_seq);
    if (!term->sessions[0].sixel.active) {
        printf("FAIL: Sixel should be on Session 0 after target reset\n");
        return 1;
    }
    printf("PASS: Sixel routed to Active Session after reset\n");

    printf("All Gateway Sixel tests passed.\n");
    KTerm_Destroy(term);
    return 0;
}
