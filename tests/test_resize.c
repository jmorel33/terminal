#define KTERM_IMPLEMENTATION
#include "kterm.h"
#include <stdio.h>
#include <assert.h>

static KTerm* term = NULL;

// Mock callbacks
void mock_response(KTerm* term, const char* response, int length) {
    (void)term;
    (void)response;
    (void)length;
}

int main() {
    printf("Starting Resize Test...\n");

    // Initialize Terminal

    KTermConfig config = {0};
    term = KTerm_Create(config);


    // Set callbacks
    KTerm_SetResponseCallback(term, mock_response);

    // Initial state check
    // Since we are mocking Situation, KTerm_Init might fail to create textures,
    // but the struct allocation should happen.
    // However, without a real GPU context, some parts might be skipped.
    // The library uses `compute_initialized` flag.
    // `KTerm_InitCompute` tries to create buffers. Mock `SituationCreateBuffer` should succeed.

    printf("Initial Size: %d x %d\n", term->width, term->height);
    assert(term->width == 132);
    assert(term->height == 50);
    assert(term->sessions[0].cols == 132);
    assert(term->sessions[0].rows == 50);

    // Test Resize
    int new_cols = 100;
    int new_rows = 40;
    printf("Resizing to %d x %d...\n", new_cols, new_rows);
    KTerm_Resize(term, new_cols, new_rows);

    printf("New Size: %d x %d\n", term->width, term->height);
    assert(term->width == new_cols);
    assert(term->height == new_rows);
    assert(term->sessions[0].cols == new_cols);
    assert(term->sessions[0].rows == new_rows);

    // Check buffer allocation (indirectly via size)
    // We can check if `row_dirty` is accessible at new limit
    // term->sessions[0].row_dirty is dynamic now.
    printf("Checking row_dirty at index %d...\n", new_rows - 1);
    bool dirty = term->sessions[0].row_dirty[new_rows - 1]; // Should not segfault
    (void)dirty;

    // Test Expand
    new_cols = 200;
    new_rows = 60;
    printf("Resizing to %d x %d...\n", new_cols, new_rows);
    KTerm_Resize(term, new_cols, new_rows);

    printf("New Size: %d x %d\n", term->width, term->height);
    assert(term->width == new_cols);
    assert(term->height == new_rows);
    assert(term->sessions[0].cols == new_cols);
    assert(term->sessions[0].rows == new_rows);

    KTerm_Cleanup(term);
    printf("Resize Test Completed Successfully.\n");
    return 0;
}
