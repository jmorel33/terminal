#define TEST_INTERNAL_ACCESS
#define KTERM_TESTING
#define KTERM_IMPLEMENTATION
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

// Mock callback to capture responses
char last_response[4096];
void TestResponseCallback(KTerm* term, const char* response, int length) {
    strncpy(last_response, response, sizeof(last_response)-1);
    last_response[length] = '\0';
    // printf("Response: %s\n", last_response);
}

void Test_DECRQTSR(KTerm* term) {
    printf("Testing DECRQTSR (CSI ? 1 $ u)...\n");
    memset(last_response, 0, sizeof(last_response));
    KTerm_Update(term); // clear
    KTerm_WriteString(term, "\x1B[?1$u"); // Request all
    KTerm_Update(term);

    // Expect DCS 1 $ r ... ST
    // Check prefix
    if (strncmp(last_response, "\x1BP1$r", 5) == 0) {
        printf("PASS: DECRQTSR response format correct.\n");
    } else {
        printf("FAIL: DECRQTSR response: %s\n", last_response);
    }
}

void Test_DECRQUPSS(KTerm* term) {
    printf("Testing DECRQUPSS (CSI ? 26 u)...\n");
    memset(last_response, 0, sizeof(last_response));

    // First, set a preferred set via DECUPSS (simulated or direct set)
    // We don't have DECUPSS implemented fully yet maybe, but we can set the internal field if we expose it
    // or just check default (0).
    KTerm_WriteString(term, "\x1B[?26u");
    KTerm_Update(term);

    // Expect DCS 0 $ r ST (assuming default 0)
    // User requested "DCS Ps $ r ST"
    if (strncmp(last_response, "\x1BP0$r\x1B\\", 7) == 0) {
        printf("PASS: DECRQUPSS response format correct (Default).\n");
    } else {
        printf("FAIL: DECRQUPSS response: %s\n", last_response);
    }
}

void Test_DECARR(KTerm* term) {
    printf("Testing DECARR (CSI 15 SP r)...\n");
    // Send CSI 15 SP r
    KTerm_WriteString(term, "\x1B[15 r");
    KTerm_Update(term);

    KTermSession* s = &term->sessions[0];
    // We need to inspect internal state. Since it's a single header included, we can access if struct definition is visible.
    // kterm.h exposes the struct.
    // We expect a new field auto_repeat_rate.
    // Since we haven't added it yet, this test will fail to compile unless we add it first.
    // But we are writing the test first. I will comment out the direct check and rely on "run tests after modification".
    // For now, I will assume the field is named `auto_repeat_rate`.

    #ifdef TEST_INTERNAL_ACCESS
    if (s->auto_repeat_rate == 15) {
        printf("PASS: DECARR set rate to 15.\n");
    } else {
        printf("FAIL: DECARR rate is %d, expected 15.\n", s->auto_repeat_rate);
    }
    #endif
}

void Test_DECRQDE(KTerm* term) {
    printf("Testing DECRQDE (CSI ? 53 $ u)...\n");
    memset(last_response, 0, sizeof(last_response));
    KTerm_WriteString(term, "\x1B[?53$u");
    KTerm_Update(term);

    // Expect DCS 1 $ r ... (similar to DECRQTSR but with defaults)
    // Or maybe DCS 53 $ r? User said "Response example: DCS 1 $ r ... ST".
    if (strncmp(last_response, "\x1BP1$r", 5) == 0) {
        printf("PASS: DECRQDE response format correct.\n");
    } else {
        printf("FAIL: DECRQDE response: %s\n", last_response);
    }
}

void Test_DECST8C(KTerm* term) {
    printf("Testing DECST8C (CSI ? 5 W)...\n");
    KTermSession* s = &term->sessions[0];

    // Mess up tabs
    KTerm_ClearAllTabStops(term);
    KTerm_SetTabStop(term, 3);

    // Send DECST8C
    KTerm_WriteString(term, "\x1B[?5W");
    KTerm_Update(term);

    // Check tabs (every 8)
    bool ok = true;
    for (int i=0; i<term->width; i++) {
        bool should_set = (i > 0 && (i % 8) == 0);
        if (s->tab_stops.stops[i] != should_set) {
            ok = false;
            // printf("Tab mismatch at %d: %d\n", i, s->tab_stops.stops[i]);
        }
    }

    if (ok) {
        printf("PASS: DECST8C reset tabs correctly.\n");
    } else {
        printf("FAIL: DECST8C tabs incorrect.\n");
    }
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 24;
    config.response_callback = TestResponseCallback;

    KTerm* term = KTerm_Create(config);
    if (!term) {
        fprintf(stderr, "Failed to create KTerm\n");
        return 1;
    }

    // Enable Gateway if needed? No, these are CSI.

    printf("--- Starting Esoteric VT510 Tests ---\n");

    Test_DECRQTSR(term);
    Test_DECRQUPSS(term);
    Test_DECARR(term);
    Test_DECRQDE(term);
    Test_DECST8C(term);

    KTerm_Destroy(term);
    return 0;
}
