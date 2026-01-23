#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

// Mock Situation functions needed for linking if not using the full library
// Since we define KTERM_IMPLEMENTATION, we need stubs if they are not header-only or if we don't link against a situation lib.
// But kterm.h includes kterm_render_sit.h which includes mock_situation.h if KTERM_TESTING is defined.
// So we should be good if we include the tests directory in include path.

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 24;

    KTerm* term = KTerm_Create(config);
    assert(term != NULL);

    KTermSession* session = &term->sessions[0];

    // 1. Test existing SET LEVEL
    KTerm_WriteString(term, "\x1BPGATE;KTERM;0;SET;LEVEL;100\x1B\\");
    KTerm_ProcessEvents(term);
    assert(session->conformance.level == VT_LEVEL_100);
    printf("Existing SET LEVEL works.\n");

    // 2. Test SET ATTR
    // Clear attributes
    session->current_attributes = 0;
    session->current_fg.value.index = COLOR_WHITE; // Default

    KTerm_WriteString(term, "\x1BPGATE;KTERM;0;SET;ATTR;BOLD=1;ITALIC=1;FG=4\x1B\\");
    KTerm_ProcessEvents(term);

    if ((session->current_attributes & KTERM_ATTR_BOLD) &&
        (session->current_attributes & KTERM_ATTR_ITALIC) &&
        session->current_fg.value.index == 4) {
        printf("SET ATTR BOLD/ITALIC/FG works.\n");
    } else {
        printf("SET ATTR failed. Attr: %x, FG: %d\n", session->current_attributes, session->current_fg.value.index);
        return 1;
    }

    // 3. Test RESET ATTR
    KTerm_WriteString(term, "\x1BPGATE;KTERM;0;RESET;ATTR\x1B\\");
    KTerm_ProcessEvents(term);

    if (session->current_attributes == 0 && session->current_fg.value.index == COLOR_WHITE) {
        printf("RESET ATTR works.\n");
    } else {
         printf("RESET ATTR failed. Attr: %x, FG: %d\n", session->current_attributes, session->current_fg.value.index);
         return 1;
    }

    // 4. Test SET BLINK
    // Default values
    assert(session->fast_blink_rate == 255);

    KTerm_WriteString(term, "\x1BPGATE;KTERM;0;SET;BLINK;FAST=100;SLOW=1000;BG=2000\x1B\\");
    KTerm_ProcessEvents(term);

    if (session->fast_blink_rate == 100 &&
        session->slow_blink_rate == 1000 &&
        session->bg_blink_rate == 2000) {
        printf("SET BLINK works.\n");
    } else {
        printf("SET BLINK failed. Fast: %d, Slow: %d, BG: %d\n",
            session->fast_blink_rate, session->slow_blink_rate, session->bg_blink_rate);
        return 1;
    }

    // 5. Test RESET BLINK
    KTerm_WriteString(term, "\x1BPGATE;KTERM;0;RESET;BLINK\x1B\\");
    KTerm_ProcessEvents(term);

    if (session->fast_blink_rate == 255 &&
        session->slow_blink_rate == 500 &&
        session->bg_blink_rate == 500) {
        printf("RESET BLINK works.\n");
    } else {
        printf("RESET BLINK failed.\n");
        return 1;
    }

    KTerm_Destroy(term);
    return 0;
}
