#include "terminal.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Helper to inspect screen state
void VerifyScreenCell(int row, int col, char expected_char, int fg_idx, int bg_idx, bool reverse) {
    EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, row, col);
    if (!cell) {
        printf("FAIL: Cell %d,%d out of bounds\n", row, col);
        return;
    }

    if (cell->ch != expected_char) {
        printf("FAIL at %d,%d: Expected char '%c', got '%c'\n", row, col, expected_char, (char)cell->ch);
    }

    if (cell->fg_color.value.index != fg_idx) {
        printf("FAIL at %d,%d: Expected FG %d, got %d\n", row, col, fg_idx, cell->fg_color.value.index);
    }

    if (cell->bg_color.value.index != bg_idx) {
        printf("FAIL at %d,%d: Expected BG %d, got %d\n", row, col, bg_idx, cell->bg_color.value.index);
    }

    if (cell->reverse != reverse) {
        printf("FAIL at %d,%d: Expected Reverse %d, got %d\n", row, col, reverse, cell->reverse);
    }
}

int main() {
    InitTerminal();
    printf("Starting Simulated VTTEST Compliance Checks...\n");

    // 1. Cursor Movement
    // "Cursor Movements" - CUU, CUD, CUF, CUB, CUP
    PipelineWriteString("\x1B[2J\x1B[H"); // Clear
    PipelineWriteString("\x1B[10;10H"); // Move to 10,10 (1-based) -> 9,9 (0-based)
    PipelineWriteString("A"); // At 9,9. Cursor moves to 9,10.
    PipelineWriteString("\x1B[2A"); // Up 2 -> 7,10
    PipelineWriteString("B"); // At 7,10. Cursor moves to 7,11.
    PipelineWriteString("\x1B[2B"); // Down 2 -> 9,11
    PipelineWriteString("C"); // At 9,11.
    PipelineWriteString("\x1B[2D"); // Left 2 -> 9,10
    PipelineWriteString("D"); // At 9,10. Overwrites ' ' or previous char?
    // Wait, sequence was:
    // 9,9: 'A' -> Cursor 9,10
    // Up 2 -> 7,10
    // 7,10: 'B' -> Cursor 7,11
    // Down 2 -> 9,11
    // 9,11: 'C' -> Cursor 9,12
    // Left 2 -> 9,10
    // 9,10: 'D' -> Cursor 9,11 (Overwrites what was at 9,10? nothing was written there, 9,9 was 'A')

    ProcessPipeline();

    // Verify
    VerifyScreenCell(9, 9, 'A', 7, 0, false);
    VerifyScreenCell(7, 10, 'B', 7, 0, false);
    VerifyScreenCell(9, 11, 'C', 7, 0, false);
    VerifyScreenCell(9, 10, 'D', 7, 0, false);
    printf("Cursor Movement Test: Done (Check logs for FAIL)\n");

    // 2. Screen Features
    // "Screen Features" - Reverse Video
    PipelineWriteString("\x1B[2J\x1B[H");
    PipelineWriteString("\x1B[7mReverse\x1B[0mNormal");
    ProcessPipeline();

    // 'R' at 0,0 should be reverse
    VerifyScreenCell(0, 0, 'R', 7, 0, true); // FG/BG indices don't swap in struct, 'reverse' flag sets.
    // 'N' at 0,7 should be normal
    VerifyScreenCell(0, 7, 'N', 7, 0, false);
    printf("Screen Features (SGR) Test: Done\n");

    // 3. Insert/Delete
    PipelineWriteString("\x1B[2J\x1B[H");
    PipelineWriteString("12345");
    PipelineWriteString("\x1B[1G"); // Move to col 1
    PipelineWriteString("\x1B[2@"); // Insert 2 blanks: "  12345"
    ProcessPipeline();
    VerifyScreenCell(0, 0, ' ', 7, 0, false);
    VerifyScreenCell(0, 1, ' ', 7, 0, false);
    VerifyScreenCell(0, 2, '1', 7, 0, false);
    printf("Insert Char Test: Done\n");

    printf("VTTEST Simulation Complete.\n");
    return 0;
}
