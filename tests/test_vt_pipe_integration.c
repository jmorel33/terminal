#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Helper to inspect the screen buffer
void VerifyCellColor(KTerm* term, int x, int y, int expected_color_idx) {
    EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
    if (!cell) {
        printf("FAIL: Invalid cell at %d,%d\n", x, y);
        return;
    }

    // Check FG color
    if (cell->fg_color.color_mode == 0 && cell->fg_color.value.index == expected_color_idx) {
        printf("PASS: Cell at %d,%d has correct color index %d\n", x, y, expected_color_idx);
    } else {
        printf("FAIL: Cell at %d,%d has color %d (mode %d), expected %d\n",
               x, y,
               cell->fg_color.value.index,
               cell->fg_color.color_mode,
               expected_color_idx);
    }
}

int main() {
    printf("Testing VT Pipe Integration (End-to-End)...\n");

    KTermConfig config = {0};
    KTerm* term = KTerm_Create(config);
    if (!term) {
        printf("Failed to create terminal\n");
        return 1;
    }

    KTermSession* session = GET_SESSION(term);

    // ---------------------------------------------------------
    // Scenario: Remote Color Change
    // 1. Inject "ESC [ 31 m" (Red FG) via VT Pipe (Base64)
    //    Base64 of "\x1B[31m" is "G1szMW0="
    // 2. Inject "A" via normal text
    // 3. Verify "A" is red.
    // ---------------------------------------------------------

    printf("\nScenario 1: Remote Color Change via Base64 Pipe\n");

    // 1. Inject Control Sequence
    const char* b64_cmd = "\x1BPGATE;KTERM;0;PIPE;VT;B64;G1szMW0=\x1B\\";
    for (int i = 0; b64_cmd[i] != '\0'; i++) {
        KTerm_ProcessChar(term, session, (unsigned char)b64_cmd[i]);
    }

    // At this point, the sequence "\x1B[31m" should be in the pipeline.
    // We must process the events for the terminal engine to execute the color change.
    KTerm_ProcessEvents(term);

    // Verify State: Current FG should be Red (Index 1)
    // Note: KTerm internal color indices map to ANSI standard (0=Black, 1=Red...)
    if (session->current_fg.value.index == 1) {
        printf("PASS: Session current_fg is Red (1) after pipe execution.\n");
    } else {
        printf("FAIL: Session current_fg is %d, expected 1.\n", session->current_fg.value.index);
    }

    // 2. Inject Text "A" (Normal)
    KTerm_WriteChar(term, 'A');
    KTerm_ProcessEvents(term);

    // 3. Verify Screen Buffer
    VerifyCellColor(term, 0, 0, 1); // Expect Red at (0,0)

    // ---------------------------------------------------------
    // Scenario 2: Complex Injection (Hex)
    // Inject "ESC [ 32 m B" (Green Text 'B')
    // Hex: 1B 5B 33 32 6D 42
    // ---------------------------------------------------------
    printf("\nScenario 2: Hex Injection (Green Text)\n");

    const char* hex_cmd = "\x1BPGATE;KTERM;0;PIPE;VT;HEX;1B5B33326D42\x1B\\";
    for (int i = 0; hex_cmd[i] != '\0'; i++) {
        KTerm_ProcessChar(term, session, (unsigned char)hex_cmd[i]);
    }

    KTerm_ProcessEvents(term);

    // 'A' was at 0,0. 'B' should be at 1,0.
    VerifyCellColor(term, 1, 0, 2); // Expect Green (2) at (1,0)

    KTerm_Destroy(term);
    return 0;
}
