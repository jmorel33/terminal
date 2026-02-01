#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include "mock_situation.h"
#include <assert.h>
#include <stdio.h>

void test_fill_rect_op(void) {
    printf("Testing FILL_RECT (DECFRA)...\n");
    KTermConfig config = {0};
    config.width = 20;
    config.height = 10;
    KTerm* term = KTerm_Create(config);
    KTermSession* session = &term->sessions[term->active_session];

    // 1. Fill screen with 'A'
    for(int i=0; i<term->height * term->width; i++) {
        KTerm_WriteChar(term, 'A');
    }
    KTerm_Update(term);

    // 2. Use DECFRA to fill a rectangle with 'X'
    // CSI Pchar; Pt; Pl; Pb; Pr $ x
    // Fill 'X' (88) from (2,2) to (5,5) (1-based)
    // Coords: Top=2, Left=2, Bottom=5, Right=5
    KTerm_WriteString(term, "\x1B[88;2;2;5;5$x");
    KTerm_Update(term);

    // Verify center is 'X'
    // 1-based (2,2) is 0-based (1,1)
    EnhancedTermChar* cell = GetScreenCell(session, 1, 1);
    if (cell->ch != 'X') {
        printf("FAILURE: Cell at (1,1) is '%c' (0x%X), expected 'X'\n", (char)cell->ch, cell->ch);
    }
    assert(cell->ch == 'X');

    // Verify outside is 'A'
    // (0,0) should be 'A'
    cell = GetScreenCell(session, 0, 0);
    assert(cell->ch == 'A');
    // (1,5) (Right+1) should be 'A'
    cell = GetScreenCell(session, 1, 5);
    assert(cell->ch == 'A');

    printf("SUCCESS: FILL_RECT passed.\n");
    KTerm_Destroy(term);
}

void test_copy_rect_op(void) {
    printf("Testing COPY_RECT (DECCRA)...\n");
    KTermConfig config = {0};
    config.width = 20;
    config.height = 10;
    KTerm* term = KTerm_Create(config);
    KTermSession* session = &term->sessions[term->active_session];

    // 1. Write "SOURCE" at (1,1)
    KTerm_WriteString(term, "\x1B[1;1HSOURCE");
    KTerm_Update(term);

    // 2. Use DECCRA to copy "SOURCE" to (3,3)
    // CSI Ptop; Pleft; Pbottom; Pright; Ppage; Pdst_top; Pdst_left; Pdst_page $ v
    // Copy (1,1)-(1,6) to (3,3)
    // Top=1, Left=1, Bottom=1, Right=6
    // DstTop=3, DstLeft=3
    KTerm_WriteString(term, "\x1B[1;1;1;6;1;3;3;1$v");
    KTerm_Update(term);

    // Verify copy at (3,3) (0-based 2,2)
    // (2,2) should be 'S'
    EnhancedTermChar* cell = GetScreenCell(session, 2, 2);
    if (cell->ch != 'S') {
        printf("FAILURE: Cell at (2,2) is '%c' (0x%X), expected 'S'\n", (char)cell->ch, cell->ch);
    }
    assert(cell->ch == 'S');
    cell = GetScreenCell(session, 2, 7);
    assert(cell->ch == 'E');

    printf("SUCCESS: COPY_RECT passed.\n");
    KTerm_Destroy(term);
}

void test_set_attr_rect_op(void) {
    printf("Testing SET_ATTR_RECT (DECCARA)...\n");
    KTermConfig config = {0};
    config.width = 20;
    config.height = 10;
    KTerm* term = KTerm_Create(config);
    KTermSession* session = &term->sessions[term->active_session];

    // 1. Write "TEXT" at (1,1)
    KTerm_WriteString(term, "\x1B[1;1HTEXT");
    KTerm_Update(term);

    // Verify initial attr (none)
    EnhancedTermChar* cell = GetScreenCell(session, 0, 0);
    assert((cell->flags & KTERM_ATTR_BOLD) == 0);

    // 2. Use DECCARA to set BOLD (1) on (1,1)-(1,4)
    // CSI Ptop; Pleft; Pbottom; Pright; Pattrs... $ r
    // Top=1, Left=1, Bottom=1, Right=4
    // Attr=1 (Bold)
    KTerm_WriteString(term, "\x1B[1;1;1;4;1$r");
    KTerm_Update(term);

    // Verify BOLD is set
    cell = GetScreenCell(session, 0, 0);
    if (!(cell->flags & KTERM_ATTR_BOLD)) {
         printf("FAILURE: Cell at (0,0) does not have BOLD flag. Flags: 0x%X\n", cell->flags);
    }
    assert(cell->flags & KTERM_ATTR_BOLD);

    // 3. Use DECRARA (Reverse Attributes) to unset BOLD?
    // Or just DECCARA with 0?
    // DECCARA sets attributes. DECRARA reverses them.
    // Let's assume we just want to verify setting works for now.

    printf("SUCCESS: SET_ATTR_RECT passed.\n");
    KTerm_Destroy(term);
}

void test_reverse_attr_rect_op(void) {
    printf("Testing REVERSE_ATTR_RECT (DECRARA)...\n");
    KTermConfig config = {0};
    config.width = 20;
    config.height = 10;
    KTerm* term = KTerm_Create(config);
    KTermSession* session = &term->sessions[term->active_session];

    // 1. Write "BOLD" at (1,1) with BOLD attribute
    KTerm_WriteString(term, "\x1B[1mBOLD");
    KTerm_Update(term);

    // Verify BOLD is set
    EnhancedTermChar* cell = GetScreenCell(session, 0, 0);
    assert(cell->flags & KTERM_ATTR_BOLD);

    // 2. Use DECRARA to toggle BOLD (1) on (1,1)-(1,4)
    // CSI Ptop; Pleft; Pbottom; Pright; Pattrs... $ u
    // Top=1, Left=1, Bottom=1, Right=4
    // Attr=1 (Bold)
    KTerm_WriteString(term, "\x1B[1;1;1;4;1$u");
    KTerm_Update(term);

    // Verify BOLD is Cleared (Toggled Off)
    cell = GetScreenCell(session, 0, 0);
    if (cell->flags & KTERM_ATTR_BOLD) {
         printf("FAILURE: Cell at (0,0) has BOLD flag, expected cleared. Flags: 0x%X\n", cell->flags);
    }
    assert(!(cell->flags & KTERM_ATTR_BOLD));

    // 3. Use DECRARA to toggle BOLD (1) again (Toggle On)
    KTerm_WriteString(term, "\x1B[1;1;1;4;1$u");
    KTerm_Update(term);

    // Verify BOLD is Set (Toggled On)
    cell = GetScreenCell(session, 0, 0);
    assert(cell->flags & KTERM_ATTR_BOLD);

    printf("SUCCESS: REVERSE_ATTR_RECT passed.\n");
    KTerm_Destroy(term);
}

int main() {
    test_fill_rect_op();
    test_copy_rect_op();
    test_set_attr_rect_op();
    test_reverse_attr_rect_op();
    return 0;
}
