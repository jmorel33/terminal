#define KTERM_DISABLE_GATEWAY
#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static KTerm* term = NULL;

int main() {
    KTermConfig config = {0};
    term = KTerm_Create(config);

    printf("Testing Gateway Protocol (DISABLED)...\n");

    // Test Case: Standard Command
    // DCS GATE MAT;1;SET;COLOR;RED ST
    const char* dcs_seq = "\x1BPGATE;MAT;1;SET;COLOR;RED\x1B\\";

    // Process
    // Should NOT crash.
    // Should act as unknown DCS (silently ignored by default, or logged if debug enabled).

    // Enable debug to see if we can catch it?
    // KTerm_EnableDebug(term, true);
    // But we don't have a way to intercept the log in this test setup easily unless we mock KTerm_LogUnsupportedSequence.
    // For now, just ensuring it compiles and runs without crashing is a good first step.

    for (int i = 0; dcs_seq[i] != '\0'; i++) {
        KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)dcs_seq[i]);
    }

    printf("PASS: Gateway sequence processed without crash in disabled mode.\n");

    return 0;
}
