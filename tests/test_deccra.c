#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static KTerm* term = NULL;

// Helper to check cell content
static void CheckCell(int y, int x, char expected_char, const char* message) {
    EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
    if (!cell) {
        printf("FAIL: %s - Cell (%d,%d) is invalid (NULL)\n", message, y, x);
        return;
    }
    if (cell->ch != expected_char) {
        printf("FAIL: %s - Cell (%d,%d) expected '%c', got '%c' (0x%X)\n", message, y, x, expected_char, (char)cell->ch, cell->ch);
    } else {
        printf("PASS: %s - Cell (%d,%d) matches '%c'\n", message, y, x, expected_char);
    }
}

// Helper to set cell content
static void SetCell(int y, int x, char ch) {
    EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
    if (cell) {
        cell->ch = ch;
    }
}

static void ClearScreen() {
    for (int y = 0; y < term->height; y++) {
        for (int x = 0; x < term->width; x++) {
            SetCell(y, x, ' ');
        }
    }
}

int main() {
    KTermConfig config = {0};
    term = KTerm_Create(config);

    // 1. Enable VT420 Level (Rectangular Ops)
    KTerm_SetLevel(term, VT_LEVEL_420);

    printf("Running DECCRA Tests...\n");

    // --- Test 1: Full 8 Parameters (Control) ---
    ClearScreen();
    SetCell(0, 0, 'A'); // Source at 0,0 (1,1)

    // Copy (1,1)-(1,1) to (2,2)
    // CSI 1;1;1;1;1;2;2;1 $ v
    const char* seq1 = "\x1B[1;1;1;1;1;2;2;1$v";
    KTerm_WriteString(term, seq1);
    KTerm_ProcessEvents(term);

    CheckCell(1, 1, 'A', "Test 1: Full 8 Params");

    // --- Test 2: Missing Trailing Parameters ---
    ClearScreen();
    SetCell(0, 0, 'B');

    // Copy (1,1)-(1,1) to (3,3). Omit dest page (param 8).
    // CSI 1;1;1;1;1;3;3 $ v
    const char* seq2 = "\x1B[1;1;1;1;1;3;3$v";
    KTerm_WriteString(term, seq2);
    KTerm_ProcessEvents(term);

    CheckCell(2, 2, 'B', "Test 2: Missing Trailing Params (7 params)");

    // --- Test 3: Default Bottom/Right ---
    ClearScreen();
    SetCell(0, 0, 'C');
    SetCell(term->height - 1, term->width - 1, 'D'); // Bottom-Right corner

    // Copy Full Screen (default) to (1,1) (implies full overwrite, but we check specific spots)
    // No, copy full screen to (1,1) is identity.
    // Let's copy full screen to same place? Hard to verify.
    // Let's copy (1,1)-Default to (1,1).
    // Wait, if I omit bottom/right, it should default to page end.
    // Let's try to copy the whole screen 1 line down?
    // Dest: (2,1).
    // Source: (1,1)-(H,W).
    // But overlap handling handles this.
    // Let's set (0,0)='C' and (0,1)='X'.
    // Copy (1,1)-End to (2,1).
    // Result: (1,0) should become 'C'.
    // Params: top=1, left=1, bot=def, right=def, src_page=1, dest_top=2, dest_left=1.
    // CSI 1;1;;;1;2;1 $ v
    const char* seq3 = "\x1B[1;1;;;1;2;1$v";
    KTerm_WriteString(term, seq3);
    KTerm_ProcessEvents(term);

    CheckCell(1, 0, 'C', "Test 3: Default Bottom/Right (should be 'C' copied from 0,0)");

    // --- Test 4: DECOM (Origin Mode) ---
    ClearScreen();
    // Enable DECOM (Mode 6)
    // Need to set margins first to make it interesting.
    // DECSET 69 (Left/Right Margin) - Requires VT420
    // Set margins: Top=2, Bottom=H-1. Left=2, Right=W-1.
    // DECSTBM 2;H-1. DECSLRM 2;W-1.
    // 0-indexed: Top=1, Bot=H-2. Left=1, Right=W-2.

    // Set Scroll Region (Top/Bottom): CSI 2 ; H-1 r
    char buf[64];
    snprintf(buf, sizeof(buf), "\x1B[2;%dr", term->height - 1);
    KTerm_WriteString(term, buf);

    // Enable DECSLRM (needed for left/right margins, usually requires mode 69)
    KTerm_WriteString(term, "\x1B[?69h");

    // Set Left/Right Margins: CSI 2 ; W-1 s
    snprintf(buf, sizeof(buf), "\x1B[2;%ds", term->width - 1);
    KTerm_WriteString(term, buf);

    // Enable DECOM: CSI ? 6 h
    KTerm_WriteString(term, "\x1B[?6h");

    // Now origin (1,1) is physically (2,2) (indices 1,1).
    // Put 'O' at physical (1,1).
    SetCell(1, 1, 'O');

    // DECCRA with (1,1) should copy 'O'.
    // Copy 'O' (Origin) to (2,2) (Relative).
    // Relative (2,2) is Physical (2+1, 2+1) = (3,3).
    // CSI 1;1;1;1;1;2;2 $ v
    // Note: 1;1;1;1 means 1 cell at Origin.
    const char* seq4 = "\x1B[1;1;1;1;1;2;2$v";
    KTerm_WriteString(term, seq4);
    KTerm_ProcessEvents(term);

    CheckCell(2, 2, 'O', "Test 4: DECOM Origin Mode (Dest should be at 2,2 relative -> 3,3 physical)");

    KTerm_Cleanup(term);
    return 0;
}
