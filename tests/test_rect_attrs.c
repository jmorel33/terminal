#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

void test_rect_attributes(KTerm* term) {
    KTermSession* session = GET_SESSION(term);

    // 1. Setup: Clear screen and write text
    KTerm_ResetAllAttributes(term);
    KTerm_WriteString(term, "\x1B[2J\x1B[H");
    KTerm_WriteString(term, "ABCDE\r\n");
    KTerm_WriteString(term, "FGHIJ\r\n");

    // Grid:
    // A B C D E  (Row 0)
    // F G H I J  (Row 1)
    // 0 1 2 3 4

    // 2. Test DECCARA (Change Attributes)
    // Apply Bold to (0,1) - (1,3) [Row 0-1, Col 1-3] -> B C D, G H I
    // Sequence: CSI 1 ; 2 ; 2 ; 4 ; 1 $ t
    // Pt=1(Row0), Pl=2(Col1), Pb=2(Row1), Pr=4(Col3), Ps=1(Bold)
    // Note: CSI coords are 1-based.
    KTerm_WriteString(term, "\x1B[1;2;2;4;1$t");
    KTerm_ProcessEvents(term);

    // Verify BOLD on target cells
    // Row 0
    if (GetScreenCell(session, 0, 0)->flags & KTERM_ATTR_BOLD) { fprintf(stderr, "FAIL: Cell (0,0) 'A' unexpectedly Bold\n"); exit(1); }
    if (!(GetScreenCell(session, 0, 1)->flags & KTERM_ATTR_BOLD)) { fprintf(stderr, "FAIL: Cell (0,1) 'B' not Bold\n"); exit(1); }
    if (!(GetScreenCell(session, 0, 2)->flags & KTERM_ATTR_BOLD)) { fprintf(stderr, "FAIL: Cell (0,2) 'C' not Bold\n"); exit(1); }
    if (!(GetScreenCell(session, 0, 3)->flags & KTERM_ATTR_BOLD)) { fprintf(stderr, "FAIL: Cell (0,3) 'D' not Bold\n"); exit(1); }
    if (GetScreenCell(session, 0, 4)->flags & KTERM_ATTR_BOLD) { fprintf(stderr, "FAIL: Cell (0,4) 'E' unexpectedly Bold\n"); exit(1); }

    // Row 1
    if (GetScreenCell(session, 1, 0)->flags & KTERM_ATTR_BOLD) { fprintf(stderr, "FAIL: Cell (1,0) 'F' unexpectedly Bold\n"); exit(1); }
    if (!(GetScreenCell(session, 1, 1)->flags & KTERM_ATTR_BOLD)) { fprintf(stderr, "FAIL: Cell (1,1) 'G' not Bold\n"); exit(1); }
    if (!(GetScreenCell(session, 1, 2)->flags & KTERM_ATTR_BOLD)) { fprintf(stderr, "FAIL: Cell (1,2) 'H' not Bold\n"); exit(1); }
    if (!(GetScreenCell(session, 1, 3)->flags & KTERM_ATTR_BOLD)) { fprintf(stderr, "FAIL: Cell (1,3) 'I' not Bold\n"); exit(1); }
    if (GetScreenCell(session, 1, 4)->flags & KTERM_ATTR_BOLD) { fprintf(stderr, "FAIL: Cell (1,4) 'J' unexpectedly Bold\n"); exit(1); }

    printf("PASS: DECCARA Bold Applied\n");

    // 3. Test DECCARA Color
    // Apply Red FG to (0,1) only ('B')
    // CSI 1 ; 2 ; 1 ; 2 ; 31 $ t
    KTerm_WriteString(term, "\x1B[1;2;1;2;31$t");
    KTerm_ProcessEvents(term);

    EnhancedTermChar* cellB = GetScreenCell(session, 0, 1);
    if (!(cellB->flags & KTERM_ATTR_BOLD)) { fprintf(stderr, "FAIL: 'B' lost Bold attribute\n"); exit(1); }
    // Check color (Red is index 1)
    if (cellB->fg_color.color_mode != 0 || cellB->fg_color.value.index != 1) {
        fprintf(stderr, "FAIL: 'B' not Red\n");
        exit(1);
    }

    printf("PASS: DECCARA Color Applied\n");

    // 4. Test DECRARA (Reverse Attributes)
    // Reverse Bold on (0,1) - (0,2) ('B', 'C')
    // 'B' is Bold -> Should become Non-Bold
    // 'C' is Bold -> Should become Non-Bold
    // 'A' is Normal -> Should become Bold (if included, but we exclude it)
    // Let's include 'A' to test toggling ON.
    // Rect: (0,0) - (0,2) [Row 0, Col 0-2] -> A, B, C
    // CSI 1 ; 1 ; 1 ; 3 ; 1 $ u
    KTerm_WriteString(term, "\x1B[1;1;1;3;1$u");
    KTerm_ProcessEvents(term);

    // 'A' (was Normal) -> Should be Bold
    if (!(GetScreenCell(session, 0, 0)->flags & KTERM_ATTR_BOLD)) { fprintf(stderr, "FAIL: 'A' did not toggle to Bold\n"); exit(1); }

    // 'B' (was Bold) -> Should be Normal
    if (GetScreenCell(session, 0, 1)->flags & KTERM_ATTR_BOLD) { fprintf(stderr, "FAIL: 'B' did not toggle to Normal\n"); exit(1); }

    // 'C' (was Bold) -> Should be Normal
    if (GetScreenCell(session, 0, 2)->flags & KTERM_ATTR_BOLD) { fprintf(stderr, "FAIL: 'C' did not toggle to Normal\n"); exit(1); }

    // 'B' should still be Red (DECRARA ignores color)
    cellB = GetScreenCell(session, 0, 1);
    if (cellB->fg_color.color_mode != 0 || cellB->fg_color.value.index != 1) {
        fprintf(stderr, "FAIL: 'B' lost Color after DECRARA\n");
        exit(1);
    }

    printf("PASS: DECRARA Toggled Attributes\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    // Enable rectangular operations feature (VT420+)
    KTerm_SetLevel(term, VT_LEVEL_420);

    test_rect_attributes(term);

    KTerm_Destroy(term);
    printf("All Rectangular Attribute Tests Passed\n");
    return 0;
}
