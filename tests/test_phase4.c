#define TERMINAL_IMPLEMENTATION
#define TERMINAL_TESTING
#include "../terminal.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

// Mock callbacks
void MockResponseCallback(Terminal* term, const char* response, int length) {
    // printf("Response: %.*s\n", length, response);
}

int main() {
    printf("Starting Phase 4 Tests...\n");

    TerminalConfig config = {
        .width = 80,
        .height = 24,
        .response_callback = MockResponseCallback
    };
    Terminal* term = Terminal_Create(config);
    SetVTLevel(term, VT_LEVEL_525); // Enable Sixel, ReGIS, Multi-session

    // --- Test 1: Sixel Split Isolation ---
    printf("Test 1: Sixel Split Isolation\n");

    // Active Session 0
    SetActiveSession(term, 0);
    // Send basic Sixel: DCS q ... ST
    const char* sixel_data = "\x1BPq#0;2;0;0;0#0!255~-\x1B\\"; // Simple line
    for(int i=0; sixel_data[i]; i++) ProcessChar(term, (unsigned char)sixel_data[i]);

    if (!GET_SESSION(term)->sixel.active) {
        printf("FAILURE: Sixel not active in Session 0\n");
        return 1;
    }
    printf("Session 0 Sixel Active: %d\n", GET_SESSION(term)->sixel.active);

    // Switch to Session 1
    SetActiveSession(term, 1);
    // Sixel state is in TerminalSession, so it should be inactive here
    if (GET_SESSION(term)->sixel.active) {
        printf("FAILURE: Sixel leakage to Session 1\n");
        return 1;
    }
    printf("Session 1 Sixel Active: %d (Correct)\n", GET_SESSION(term)->sixel.active);

    // --- Test 2: ReGIS Macro Sharing (Behavior Verification) ---
    printf("Test 2: ReGIS Macro Sharing\n");
    SetActiveSession(term, 0);

    // Define Macro @A containing "C(A)" (Circle)
    // ReGIS command: DCS p ... ST
    const char* regis_def = "\x1BPp@:AC(A)@;\x1B\\";
    for(int i=0; regis_def[i]; i++) ProcessChar(term, (unsigned char)regis_def[i]);

    // Verify macro stored (Access internal state directly for test)
    // Note: regis struct is in Terminal (shared)
    if (term->regis.macros[0] == NULL) {
        printf("FAILURE: Macro A not defined in Session 0\n");
        return 1;
    }
    printf("Macro A defined: %s\n", term->regis.macros[0]);

    // Switch to Session 1
    SetActiveSession(term, 1);
    // Try to access macro
    if (term->regis.macros[0] == NULL) {
        printf("Observation: Macro A NOT visible in Session 1 (Isolated)\n");
    } else {
        printf("Observation: Macro A visible in Session 1 (Shared)\n");
        // This is expected given current architecture
    }

    // --- Test 3: Split Screen State ---
    printf("Test 3: Split Screen State\n");
    SetActiveSession(term, 0);
    // CSI 1 $ ~ (Split horizontal)
    const char* split_cmd = "\x1B[1$~";
    for(int i=0; split_cmd[i]; i++) ProcessChar(term, (unsigned char)split_cmd[i]);

    if (!term->split_screen_active) {
        printf("FAILURE: Split screen not activated\n");
        return 1;
    }
    printf("Split Screen Active: %d\n", term->split_screen_active);

    Terminal_Destroy(term);
    printf("Phase 4 Tests Passed.\n");
    return 0;
}
