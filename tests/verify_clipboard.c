#include <stdio.h>
#include <string.h>
#include <assert.h>

#define TERMINAL_IMPLEMENTATION
#define TERMINAL_TESTING
#include "terminal.h"

static Terminal* term = NULL;

// Define stubs for Situation functions that might be called but not in mock (if any missing)
// But mock_situation.h should cover them.

int main() {

    TerminalConfig config = {0};
    term = Terminal_Create(config);


    // 1. Setup a test string in the terminal
    // We'll manually insert a Unicode character (Snowman: U+2603)
    // UTF-8: E2 98 83

    // Clear screen first
    PipelineWriteString(term, "\x1B[2J\x1B[H");
    ProcessPipeline(term);

    // Manually inject codepoint into screen buffer
    // Row 0, Col 0
    EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, 0, 0);
    cell->ch = 0x2603; // Snowman

    // Set selection covering this cell
    ACTIVE_SESSION.selection.active = true;
    ACTIVE_SESSION.selection.start_x = 0;
    ACTIVE_SESSION.selection.start_y = 0;
    ACTIVE_SESSION.selection.end_x = 0;
    ACTIVE_SESSION.selection.end_y = 0;

    // 2. Perform Copy
    CopySelectionToClipboard();

    // 3. Verify
    // Expected UTF-8 for U+2603 is 0xE2 0x98 0x83
    unsigned char expected[] = {0xE2, 0x98, 0x83, 0x00};

    printf("Last Clipboard Content (Hex): ");
    for(int i=0; i<strlen(last_clipboard_text); i++) {
        printf("%02X ", (unsigned char)last_clipboard_text[i]);
    }
    printf("\n");

    if (strcmp(last_clipboard_text, (char*)expected) == 0) {
        printf("PASS: Clipboard contains correct UTF-8 Snowman.\n");
    } else {
        printf("FAIL: Clipboard content mismatch.\n");
        printf("Expected: E2 98 83\n");
        return 1;
    }

    CleanupTerminal(term);
    return 0;
}
