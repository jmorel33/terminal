#include <stdio.h>
#include <assert.h>
#include <string.h>

#define KTERM_TESTING
#define KTERM_IMPLEMENTATION
#include "kterm.h"

static KTerm* term = NULL;

void TestDefaults() {
    printf("Testing Kitty Defaults...\n");
    KTermSession* session = GET_SESSION(term);

    // Clear images
    if (session->kitty.images) {
        for(int i=0; i<session->kitty.image_count; i++) free(session->kitty.images[i].data);
        free(session->kitty.images);
        session->kitty.images = NULL;
        session->kitty.image_count = 0;
        session->kitty.current_memory_usage = 0;
    }

    // Move cursor to 5, 5 (0-indexed cells)
    session->cursor.x = 5;
    session->cursor.y = 5;

    // Transmit image (a=t) without x/y
    // Expected: x = 5 * 8 = 40, y = 5 * 10 = 50
    const char* upload = "\x1B_Ga=t,i=100;SGVsbG8=\x1B\\";
    KTerm_WriteString(term, upload);
    KTerm_ProcessEvents(term);

    assert(session->kitty.image_count == 1);
    assert(session->kitty.images[0].id == 100);

    printf("Image X: %d (Expected 40)\n", session->kitty.images[0].x);
    assert(session->kitty.images[0].x == 40);

    printf("Image Y: %d (Expected 50)\n", session->kitty.images[0].y);
    assert(session->kitty.images[0].y == 50);

    // Verify start_row initialization
    assert(session->kitty.images[0].start_row == session->screen_head);

    printf("Defaults Verified.\n");
}

void TestScrollState() {
    printf("Testing Scroll State...\n");
    KTermSession* session = GET_SESSION(term);

    // Ensure screen_head is 0
    session->screen_head = 0;

    // Add lines to scroll (simulate scrolling)
    // Actually simpler to just manually set screen_head and verify logic?
    // But I can't verify KTerm_Draw logic easily here as it issues GPU commands.
    // I can only verify that `start_row` is captured correctly.

    // Create new image at current screen_head
    const char* upload = "\x1B_Ga=T,i=101;SGVsbG8=\x1B\\";
    KTerm_WriteString(term, upload);
    KTerm_ProcessEvents(term);

    int img_idx = session->kitty.image_count - 1;
    assert(session->kitty.images[img_idx].id == 101);
    assert(session->kitty.images[img_idx].start_row == 0);

    // Simulate scroll
    session->screen_head = 5;

    // Create another image
    const char* upload2 = "\x1B_Ga=T,i=102;SGVsbG8=\x1B\\";
    KTerm_WriteString(term, upload2);
    KTerm_ProcessEvents(term);

    img_idx = session->kitty.image_count - 1;
    assert(session->kitty.images[img_idx].id == 102);
    assert(session->kitty.images[img_idx].start_row == 5);

    printf("Scroll State Verified.\n");
}

int main() {
    KTermConfig config = {0};
    term = KTerm_Create(config);
    KTerm_SetLevel(term, VT_LEVEL_XTERM);

    TestDefaults();
    TestScrollState();

    KTerm_Cleanup(term);
    return 0;
}
