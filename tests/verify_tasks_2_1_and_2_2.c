#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <assert.h>
#include <stdio.h>

static KTerm* term = NULL;

// Simple test framework
void verify_task_2_1(void) {
    printf("Verifying Task 2.1: Per-Session Save/Restore Stacks (DECSC/DECRC)...\n");


    KTermConfig config = {0};
    term = KTerm_Create(config);


    // --- Session 0 ---
    KTerm_SetActiveSession(term, 0);
    // Set some state
    term->sessions[0].cursor.x = 10;
    term->sessions[0].cursor.y = 5;
    term->sessions[0].current_attributes |= KTERM_ATTR_BOLD;
    term->sessions[0].current_fg.value.index = 1; // Red

    // Save cursor (DECSC)
    KTerm_ExecuteSaveCursor(term, GET_SESSION(term));

    // Verify saved state matches current state
    assert(term->sessions[0].saved_cursor_valid == true);
    assert(term->sessions[0].saved_cursor.x == 10);
    assert(term->sessions[0].saved_cursor.y == 5);
    assert(term->sessions[0].saved_cursor.attributes & KTERM_ATTR_BOLD);
    assert(term->sessions[0].saved_cursor.fg_color.value.index == 1);

    // Modify state
    term->sessions[0].cursor.x = 20;
    term->sessions[0].cursor.y = 10;
    term->sessions[0].current_attributes &= ~KTERM_ATTR_BOLD;
    term->sessions[0].current_fg.value.index = 2; // Green

    // --- Session 1 ---
    KTerm_SetActiveSession(term, 1);
    // Set some state for Session 1
    term->sessions[1].cursor.x = 5;
    term->sessions[1].cursor.y = 2;
    term->sessions[1].current_attributes &= ~KTERM_ATTR_BOLD;

    // Save cursor (DECSC) for Session 1
    KTerm_ExecuteSaveCursor(term, GET_SESSION(term));

    // Verify saved state for Session 1
    assert(term->sessions[1].saved_cursor_valid == true);
    assert(term->sessions[1].saved_cursor.x == 5);
    assert(term->sessions[1].saved_cursor.y == 2);

    // Modify Session 1 state
    term->sessions[1].cursor.x = 30;
    term->sessions[1].cursor.y = 15;

    // --- Switch back to Session 0 ---
    KTerm_SetActiveSession(term, 0);

    // Restore cursor (DECRC)
    KTerm_ExecuteRestoreCursor(term, GET_SESSION(term));

    // Verify restored state matches originally saved state
    assert(term->sessions[0].cursor.x == 10);
    assert(term->sessions[0].cursor.y == 5);
    assert(term->sessions[0].current_attributes & KTERM_ATTR_BOLD);
    assert(term->sessions[0].current_fg.value.index == 1); // Red

    // --- Switch back to Session 1 ---
    KTerm_SetActiveSession(term, 1);

    // Restore cursor (DECRC)
    KTerm_ExecuteRestoreCursor(term, GET_SESSION(term));

    // Verify restored state for Session 1
    assert(term->sessions[1].cursor.x == 5);
    assert(term->sessions[1].cursor.y == 2);

    KTerm_Cleanup(term);
    printf("Task 2.1 Verification Passed!\n");
}

void verify_task_2_2(void) {
    printf("Verifying Task 2.2: Input Routing Protocol...\n");


    KTermConfig config = {0};
    term = KTerm_Create(config);


    // Ensure Session 0 is active
    KTerm_SetActiveSession(term, 0);

    // Write to Session 1's pipeline (while Session 0 is active)
    KTerm_WriteCharToSession(term, 1, 'A');
    KTerm_WriteCharToSession(term, 1, 'B');
    KTerm_WriteCharToSession(term, 1, 'C');

    // Verify Session 1's pipeline has data
    assert(term->sessions[1].pipeline_count == 3);
    assert(term->sessions[1].input_pipeline[0] == 'A');
    assert(term->sessions[1].input_pipeline[1] == 'B');
    assert(term->sessions[1].input_pipeline[2] == 'C');

    // Verify Session 0's pipeline is empty
    assert(term->sessions[0].pipeline_count == 0);

    // Call KTerm_Update, which should process pipelines for ALL sessions
    // KTerm_Update iterates 0 to MAX_SESSIONS, sets active_session, calls KTerm_ProcessEvents
    KTerm_Update(term);

    // Verify Session 1's pipeline is processed (empty)
    assert(term->sessions[1].pipeline_count == 0);

    // Verify Session 1's screen buffer received the characters
    // Note: 'A', 'B', 'C' are written at cursor 0,0, 0,1, 0,2
    EnhancedTermChar* cell0 = GetScreenCell(&term->sessions[1], 0, 0);
    EnhancedTermChar* cell1 = GetScreenCell(&term->sessions[1], 0, 1);
    EnhancedTermChar* cell2 = GetScreenCell(&term->sessions[1], 0, 2);

    assert(cell0->ch == 'A');
    assert(cell1->ch == 'B');
    assert(cell2->ch == 'C');

    KTerm_Cleanup(term);
    printf("Task 2.2 Verification Passed!\n");
}

int main(void) {
    verify_task_2_1();
    verify_task_2_2();
    return 0;
}
