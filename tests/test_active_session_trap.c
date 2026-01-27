#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#define KTERM_IMPLEMENTATION
#define SITUATION_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define KTERM_TESTING // Force inclusion of mock_situation.h
#include "../kterm.h"
// mock_situation.h is included by kterm.h when KTERM_TESTING is defined

static int last_resized_session_index = -1;
static int last_resized_cols = -1;
static int last_resized_rows = -1;

void my_resize_callback(KTerm* term, int session_index, int cols, int rows) {
    last_resized_session_index = session_index;
    last_resized_cols = cols;
    last_resized_rows = rows;
    printf("Callback: Session %d resized to %dx%d\n", session_index, cols, rows);
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 24;
    KTerm* term = KTerm_Create(config);

    if (!term) {
        printf("Failed to create KTerm\n");
        return 1;
    }

    term->session_resize_callback = my_resize_callback;

    // Open session 1
    KTerm_InitSession(term, 1);
    term->sessions[1].session_open = true;

    // Call KTerm_ResizeSession_Internal directly (simulating internal call)
    // We resize session 1
    KTermSession* session1 = &term->sessions[1];

    printf("Invoking KTerm_ResizeSession_Internal for Session 1...\n");
    KTerm_ResizeSession_Internal(term, session1, 100, 30);

    // Verify callback
    if (last_resized_session_index == 1) {
        printf("PASS: Callback received correct session index 1\n");
    } else {
        printf("FAIL: Callback received index %d, expected 1\n", last_resized_session_index);
        return 1;
    }

    if (last_resized_cols == 100 && last_resized_rows == 30) {
        printf("PASS: Callback received correct dimensions\n");
    } else {
        printf("FAIL: Dimensions mismatch\n");
        return 1;
    }

    KTerm_Destroy(term);
    return 0;
}
