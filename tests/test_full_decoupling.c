#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include "mock_situation.h"
#include <assert.h>
#include <stdio.h>

// Mock callbacks
void mock_resize_callback(KTerm* term, int session_index, int cols, int rows) {
    (void)term;
    printf("Callback: Session %d resized to %dx%d\n", session_index, cols, rows);
}

void test_insert_lines_decoupled(void) {
    printf("Testing Insert Lines Decoupling...\n");
    KTermConfig config = {0};
    config.width = 20;
    config.height = 10;
    KTerm* term = KTerm_Create(config);
    KTermSession* session = &term->sessions[term->active_session];
    
    // 1. Fill screen with 'A'
    for(int i=0; i<term->height * term->width; i++) {
        KTerm_WriteChar(term, 'A');
    }
    // Process input to generate ops, then flush ops to grid
    KTerm_Update(term); 

    // Verify row 2 (index 1) is 'A'
    EnhancedTermChar* cell = GetScreenCell(session, 1, 0);
    if (cell->ch != 'A') {
        printf("Setup failure: Expected 'A', got '%c' (0x%X)\n", (char)cell->ch, cell->ch);
    }
    assert(cell->ch == 'A');

    // 2. Queue Insert Line at row 2 (index 1)
    KTerm_InsertLinesAt(term, 1, 1);

    // 3. Verify NO CHANGE yet (Decoupling)
    cell = GetScreenCell(session, 1, 0);
    assert(cell->ch == 'A'); 

    // 4. Flush
    KTerm_FlushOps(term, session);

    // 5. Verify CHANGE (Row 1 should be empty/cleared)
    cell = GetScreenCell(session, 1, 0);
    // Assuming clear char is space or null with attrs. Usually space.
    if (cell->ch != ' ' && cell->ch != 0) {
        printf("Failure: Expected ' ' or 0, got '%c'\n", (char)cell->ch);
    }
    // KTerm_ClearCell_Internal sets ' '.
    assert(cell->ch == ' '); 

    printf("SUCCESS: Insert Lines Decoupling passed.\n");
    KTerm_Destroy(term);
}

void test_resize_decoupled(void) {
    printf("Testing Resize Decoupling...\n");
    KTermConfig config = {0};
    config.width = 20;
    config.height = 10;
    KTerm* term = KTerm_Create(config);
    KTerm_SetSessionResizeCallback(term, mock_resize_callback);
    KTermSession* session = &term->sessions[term->active_session];

    int old_cols = session->cols;
    assert(old_cols == 20);

    // 1. Trigger Resize
    KTerm_Resize(term, 30, 15);

    // 2. Verify term dimensions updated immediately
    assert(term->width == 30);
    assert(term->height == 15);

    // 3. Verify Session dimensions NOT updated yet (Queue)
    
    if (session->cols != 20) {
    }
    assert(session->cols == 20); 

    // 4. Flush
    KTerm_FlushOps(term, session);

    // 5. Verify Session dimensions UPDATED
    assert(session->cols == 30);
    assert(session->rows == 15);

    printf("SUCCESS: Resize Decoupling passed.\n");
    KTerm_Destroy(term);
}

int main() {
    test_insert_lines_decoupled();
    test_resize_decoupled();
    return 0;
}
