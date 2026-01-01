#define TERMINAL_IMPLEMENTATION
#define TERMINAL_TESTING
#include "../terminal.h"
#include <assert.h>
#include <stdio.h>

// Simple test framework
void verify_task_2_1(void) {
    printf("Verifying Task 2.1: Per-Session Save/Restore Stacks (DECSC/DECRC)...\n");

    InitTerminal();

    // --- Session 0 ---
    SetActiveSession(0);
    // Set some state
    terminal.sessions[0].cursor.x = 10;
    terminal.sessions[0].cursor.y = 5;
    terminal.sessions[0].bold_mode = true;
    terminal.sessions[0].current_fg.value.index = 1; // Red

    // Save cursor (DECSC)
    ExecuteSaveCursor();

    // Verify saved state matches current state
    assert(terminal.sessions[0].saved_cursor_valid == true);
    assert(terminal.sessions[0].saved_cursor.x == 10);
    assert(terminal.sessions[0].saved_cursor.y == 5);
    assert(terminal.sessions[0].saved_cursor.bold_mode == true);
    assert(terminal.sessions[0].saved_cursor.fg_color.value.index == 1);

    // Modify state
    terminal.sessions[0].cursor.x = 20;
    terminal.sessions[0].cursor.y = 10;
    terminal.sessions[0].bold_mode = false;
    terminal.sessions[0].current_fg.value.index = 2; // Green

    // --- Session 1 ---
    SetActiveSession(1);
    // Set some state for Session 1
    terminal.sessions[1].cursor.x = 5;
    terminal.sessions[1].cursor.y = 2;
    terminal.sessions[1].bold_mode = false;

    // Save cursor (DECSC) for Session 1
    ExecuteSaveCursor();

    // Verify saved state for Session 1
    assert(terminal.sessions[1].saved_cursor_valid == true);
    assert(terminal.sessions[1].saved_cursor.x == 5);
    assert(terminal.sessions[1].saved_cursor.y == 2);

    // Modify Session 1 state
    terminal.sessions[1].cursor.x = 30;
    terminal.sessions[1].cursor.y = 15;

    // --- Switch back to Session 0 ---
    SetActiveSession(0);

    // Restore cursor (DECRC)
    ExecuteRestoreCursor();

    // Verify restored state matches originally saved state
    assert(terminal.sessions[0].cursor.x == 10);
    assert(terminal.sessions[0].cursor.y == 5);
    assert(terminal.sessions[0].bold_mode == true);
    assert(terminal.sessions[0].current_fg.value.index == 1); // Red

    // --- Switch back to Session 1 ---
    SetActiveSession(1);

    // Restore cursor (DECRC)
    ExecuteRestoreCursor();

    // Verify restored state for Session 1
    assert(terminal.sessions[1].cursor.x == 5);
    assert(terminal.sessions[1].cursor.y == 2);

    CleanupTerminal();
    printf("Task 2.1 Verification Passed!\n");
}

void verify_task_2_2(void) {
    printf("Verifying Task 2.2: Input Routing Protocol...\n");

    InitTerminal();

    // Ensure Session 0 is active
    SetActiveSession(0);

    // Write to Session 1's pipeline (while Session 0 is active)
    PipelineWriteCharToSession(1, 'A');
    PipelineWriteCharToSession(1, 'B');
    PipelineWriteCharToSession(1, 'C');

    // Verify Session 1's pipeline has data
    assert(terminal.sessions[1].pipeline_count == 3);
    assert(terminal.sessions[1].input_pipeline[0] == 'A');
    assert(terminal.sessions[1].input_pipeline[1] == 'B');
    assert(terminal.sessions[1].input_pipeline[2] == 'C');

    // Verify Session 0's pipeline is empty
    assert(terminal.sessions[0].pipeline_count == 0);

    // Call UpdateTerminal, which should process pipelines for ALL sessions
    // UpdateTerminal iterates 0 to MAX_SESSIONS, sets active_session, calls ProcessPipeline
    UpdateTerminal();

    // Verify Session 1's pipeline is processed (empty)
    assert(terminal.sessions[1].pipeline_count == 0);

    // Verify Session 1's screen buffer received the characters
    // Note: 'A', 'B', 'C' are written at cursor 0,0, 0,1, 0,2
    EnhancedTermChar* cell0 = GetScreenCell(&terminal.sessions[1], 0, 0);
    EnhancedTermChar* cell1 = GetScreenCell(&terminal.sessions[1], 0, 1);
    EnhancedTermChar* cell2 = GetScreenCell(&terminal.sessions[1], 0, 2);

    assert(cell0->ch == 'A');
    assert(cell1->ch == 'B');
    assert(cell2->ch == 'C');

    CleanupTerminal();
    printf("Task 2.2 Verification Passed!\n");
}

int main(void) {
    verify_task_2_1();
    verify_task_2_2();
    return 0;
}
