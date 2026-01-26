#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static KTerm* term = NULL;

// Mock callbacks
void mock_response(KTerm* term, const char* response, int length) {
    (void)term;
    (void)response;
    (void)length;
}

int main() {
    printf("Starting DECCOLM/DECSCPP Test...\n");

    KTermConfig config = {0};
    config.width = 80;
    config.height = 24;
    term = KTerm_Create(config);
    if (!term) {
        printf("Failed to create KTerm\n");
        return 1;
    }

    KTerm_SetResponseCallback(term, mock_response);

    // Initial check
    printf("Initial Size: %d\n", term->width);
    assert(term->width == 80);

    // 1. Try to resize with DECCOLM (CSI ? 3 h) - Should fail without Mode 40
    printf("Attempting resize without Mode 40...\n");
    KTerm_ProcessEvents(term); // Flush any pending
    KTerm_WriteString(term, "\x1B[?3h");
    KTerm_ProcessEvents(term);

    printf("Size after ignored resize: %d\n", term->width);
    assert(term->width == 80); // Should remain 80

    // 2. Enable Mode 40 (Allow 80/132)
    printf("Enabling Mode 40...\n");
    KTerm_WriteString(term, "\x1B[?40h");
    KTerm_ProcessEvents(term);
    assert(term->sessions[0].dec_modes & KTERM_MODE_ALLOW_80_132);

    // 3. Resize to 132 via DECCOLM
    printf("Resizing to 132 (DECCOLM)...\n");
    KTerm_WriteString(term, "\x1B[?3h");
    KTerm_ProcessEvents(term);
    printf("New Session Size: %d\n", term->sessions[0].cols);
    assert(term->sessions[0].cols == 132);

    // Verify cursor reset (side effect)
    term->sessions[0].cursor.x = 10;
    term->sessions[0].cursor.y = 10;

    // 4. Resize to 80 via DECCOLM
    printf("Resizing to 80 (DECCOLM)...\n");
    KTerm_WriteString(term, "\x1B[?3l");
    KTerm_ProcessEvents(term);
    printf("New Session Size: %d\n", term->sessions[0].cols);
    assert(term->sessions[0].cols == 80);
    // Cursor should be homed
    assert(term->sessions[0].cursor.x == 0);
    assert(term->sessions[0].cursor.y == 0);

    // 5. Test DECSCPP (CSI 132 $ |)
    printf("Resizing to 132 (DECSCPP)...\n");
    KTerm_WriteString(term, "\x1B[132$|");
    KTerm_ProcessEvents(term);
    printf("New Session Size: %d\n", term->sessions[0].cols);
    assert(term->sessions[0].cols == 132);
    assert(term->sessions[0].dec_modes & KTERM_MODE_DECCOLM);

    // 6. Test DECSCPP (CSI 80 $ |)
    printf("Resizing to 80 (DECSCPP)...\n");
    KTerm_WriteString(term, "\x1B[80$|");
    KTerm_ProcessEvents(term);
    printf("New Session Size: %d\n", term->sessions[0].cols);
    assert(term->sessions[0].cols == 80);
    assert(!(term->sessions[0].dec_modes & KTERM_MODE_DECCOLM));

    // 7. Test DECNCSM (No Clear Screen)
    printf("Testing DECNCSM...\n");
    // Write some text at 0,0 (cursor was homed by step 6)
    KTerm_WriteString(term, "Hello World");
    KTerm_ProcessEvents(term);

    // Move cursor manually
    term->sessions[0].cursor.x = 5;
    term->sessions[0].cursor.y = 5;

    // Enable DECNCSM (CSI ? 95 h)
    KTerm_WriteString(term, "\x1B[?95h");
    KTerm_ProcessEvents(term);

    // Resize to 132
    KTerm_WriteString(term, "\x1B[132$|");
    KTerm_ProcessEvents(term);

    assert(term->sessions[0].cols == 132);
    // Cursor should NOT be homed
    printf("Cursor at %d,%d\n", term->sessions[0].cursor.x, term->sessions[0].cursor.y);
    assert(term->sessions[0].cursor.x == 5);
    assert(term->sessions[0].cursor.y == 5);

    // Check if text persists (cell 0,0 should have 'H')
    EnhancedTermChar* cell = GetScreenCell(&term->sessions[0], 0, 0);
    printf("Cell 0,0 char: '%c'\n", (char)cell->ch);
    assert(cell->ch == 'H');

    KTerm_Cleanup(term);
    free(term);
    printf("DECCOLM/DECSCPP Test Completed Successfully.\n");
    return 0;
}
