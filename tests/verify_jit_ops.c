#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include "mock_situation.h"
#include <assert.h>
#include <stdio.h>

void test_op_queue_integration(void) {
    KTermConfig config = {0};
    KTerm* term = KTerm_Create(config);
    KTermSession* session = &term->sessions[term->active_session];

    // Enable Op Queue (should be default, but verify/force)
    session->use_op_queue = true;

    // Write 'A'
    KTerm_WriteChar(term, 'A');

    // Process Events (simulates one frame update)
    KTerm_Update(term);

    // Check Grid
    // Coordinate (0,0) should be 'A'
    EnhancedTermChar* cell = GetScreenCell(session, 0, 0);
    if (cell->ch != 'A') {
        printf("FAILURE: Grid not updated. Expected 'A', got '%c' (0x%02X). OpQueue count: %d\n",
               (char)cell->ch, cell->ch, session->op_queue.count);
        if (session->op_queue.count > 0) {
            printf("Ops are queued but not flushed!\n");
        }
    } else {
        printf("SUCCESS: Grid updated correctly.\n");
    }

    // Check Dirty Rect
    // Should contain (0,0)
    if (session->dirty_rect.w > 0) {
        printf("Dirty Rect: %d,%d %dx%d\n",
               session->dirty_rect.x, session->dirty_rect.y,
               session->dirty_rect.w, session->dirty_rect.h);
    } else {
        printf("FAILURE: Dirty rect empty.\n");
    }

    assert(cell->ch == 'A');
    assert(session->dirty_rect.w > 0);

    KTerm_Destroy(term);
}

int main() {
    test_op_queue_integration();
    return 0;
}
