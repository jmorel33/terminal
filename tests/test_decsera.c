#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
// Mock Situation to avoid linking issues
#include "../tests/mock_situation.h"
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

void write_sequence(KTerm* term, const char* seq) {
    while (*seq) {
        KTerm_ProcessChar(term, GET_SESSION(term), *seq++);
    }
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    // Enable VT420 mode for Rectangular Operations
    KTerm_SetLevel(term, GET_SESSION(term), VT_LEVEL_420);

    KTermSession* session = GET_SESSION(term);

    // Setup:
    // Row 0: "AAAAA" (Unprotected)
    // Row 1: "PPPPP" (Protected)

    // Move to 1,1
    write_sequence(term, "\x1B[H");

    // Write AAAAA
    write_sequence(term, "AAAAA");

    // Move to 2,1
    write_sequence(term, "\x1B[2;1H");

    // Set Protected (DECSCA 1)
    write_sequence(term, "\x1B[1\"q");
    write_sequence(term, "PPPPP");
    // Unset Protected (DECSCA 0)
    write_sequence(term, "\x1B[0\"q");

    // Verify initial state
    EnhancedTermChar* cell0 = GetActiveScreenCell(session, 0, 0);
    EnhancedTermChar* cell1 = GetActiveScreenCell(session, 1, 0);

    if (cell0->ch != 'A' || (cell0->flags & KTERM_ATTR_PROTECTED)) {
        printf("Setup fail Row 0: %c flags %X\n", cell0->ch, cell0->flags);
        return 1;
    }
    if (cell1->ch != 'P' || !(cell1->flags & KTERM_ATTR_PROTECTED)) {
        printf("Setup fail Row 1: %c flags %X\n", cell1->ch, cell1->flags);
        return 1;
    }

    // Execute DECSERA on 1,1 to 2,5 (covering both rows)
    // CSI Ptop ; Pleft ; Pbottom ; Pright $ {
    // Top=1, Left=1, Bottom=2, Right=5
    write_sequence(term, "\x1B[1;1;2;5${");

    // Verify Row 0 (Should be erased to Space)
    cell0 = GetActiveScreenCell(session, 0, 0);
    if (cell0->ch != ' ') {
        printf("FAIL: Row 0 not erased. char=%c (%d)\n", cell0->ch, cell0->ch);
        return 1;
    } else {
        printf("PASS: Row 0 erased\n");
    }

    // Verify Row 1 (Should remain P)
    cell1 = GetActiveScreenCell(session, 1, 0);
    if (cell1->ch != 'P') {
        printf("FAIL: Row 1 erased (should be protected). char=%c\n", cell1->ch);
        return 1;
    } else {
        printf("PASS: Row 1 protected\n");
    }

    KTerm_Destroy(term);
    return 0;
}
