#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static KTerm* term = NULL;

void MockResponseCallback(KTerm* term, const char* response, int length) {
    (void)term; (void)response; (void)length;
}

int main() {
    KTermConfig config = {0};
    config.response_callback = MockResponseCallback;
    term = KTerm_Create(config);

    printf("Testing Conceal Character Code...\n");

    // 1. Check Default
    KTermSession* session = &term->sessions[term->active_session];
    if (session->conceal_char_code == 0) {
        printf("PASS: Default conceal_char_code is 0\n");
    } else {
        printf("FAIL: Default conceal_char_code is %u\n", session->conceal_char_code);
        return 1;
    }

    // 2. Test SET CONCEAL to 'A' (65)
    // DCS GATE;KTERM;0;SET;CONCEAL;65 ST
    const char* dcs_set = "\x1BPGATE;KTERM;0;SET;CONCEAL;65\x1B\\";
    KTerm_WriteString(term, dcs_set);
    KTerm_ProcessEvents(term);

    if (session->conceal_char_code == 65) {
        printf("PASS: conceal_char_code changed to 65\n");
    } else {
        printf("FAIL: conceal_char_code not changed (Code=%u)\n", session->conceal_char_code);
        return 1;
    }

    // 3. Test SET CONCEAL to 0
    const char* dcs_reset = "\x1BPGATE;KTERM;0;SET;CONCEAL;0\x1B\\";
    KTerm_WriteString(term, dcs_reset);
    KTerm_ProcessEvents(term);

    if (session->conceal_char_code == 0) {
        printf("PASS: conceal_char_code reset to 0\n");
    } else {
        printf("FAIL: conceal_char_code not reset (Code=%u)\n", session->conceal_char_code);
        return 1;
    }

    KTerm_Destroy(term);
    return 0;
}
