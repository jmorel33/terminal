#include "terminal.h"
#include <stdio.h>
#include <assert.h>

// Mock callbacks
void mock_response(const char* response, int length) {
    (void)response;
    (void)length;
}

int main() {
    printf("Starting Resize Test...\n");

    // Initialize Terminal
    InitTerminal();

    // Set callbacks
    SetResponseCallback(mock_response);

    // Initial state check
    // Since we are mocking Situation, InitTerminal might fail to create textures,
    // but the struct allocation should happen.
    // However, without a real GPU context, some parts might be skipped.
    // The library uses `compute_initialized` flag.
    // `InitTerminalCompute` tries to create buffers. Mock `SituationCreateBuffer` should succeed.

    printf("Initial Size: %d x %d\n", terminal.width, terminal.height);
    assert(terminal.width == 132);
    assert(terminal.height == 50);
    assert(terminal.sessions[0].cols == 132);
    assert(terminal.sessions[0].rows == 50);

    // Test Resize
    int new_cols = 100;
    int new_rows = 40;
    printf("Resizing to %d x %d...\n", new_cols, new_rows);
    ResizeTerminal(new_cols, new_rows);

    printf("New Size: %d x %d\n", terminal.width, terminal.height);
    assert(terminal.width == new_cols);
    assert(terminal.height == new_rows);
    assert(terminal.sessions[0].cols == new_cols);
    assert(terminal.sessions[0].rows == new_rows);

    // Check buffer allocation (indirectly via size)
    // We can check if `row_dirty` is accessible at new limit
    // terminal.sessions[0].row_dirty is dynamic now.
    printf("Checking row_dirty at index %d...\n", new_rows - 1);
    bool dirty = terminal.sessions[0].row_dirty[new_rows - 1]; // Should not segfault
    (void)dirty;

    // Test Expand
    new_cols = 200;
    new_rows = 60;
    printf("Resizing to %d x %d...\n", new_cols, new_rows);
    ResizeTerminal(new_cols, new_rows);

    printf("New Size: %d x %d\n", terminal.width, terminal.height);
    assert(terminal.width == new_cols);
    assert(terminal.height == new_rows);
    assert(terminal.sessions[0].cols == new_cols);
    assert(terminal.sessions[0].rows == new_rows);

    CleanupTerminal();
    printf("Resize Test Completed Successfully.\n");
    return 0;
}
