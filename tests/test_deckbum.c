#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../tests/mock_situation.h"
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

void write_sequence(KTerm* term, const char* seq) {
    while (*seq) {
        KTerm_ProcessChar(term, GET_SESSION(term), *seq++);
    }
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    KTermSession* session = GET_SESSION(term);
    KTerm_SetLevel(term, session, VT_LEVEL_XTERM);

    // Verify initial state (DECKBUM off by default)
    if (session->dec_modes & KTERM_MODE_DECKBUM) {
        printf("FAIL: DECKBUM (Mode 68) should be OFF initially\n");
        return 1;
    }

    // Enable Mode 68
    write_sequence(term, "\x1B[?68h");
    if (!(session->dec_modes & KTERM_MODE_DECKBUM)) {
        printf("FAIL: DECKBUM (Mode 68) not set by CSI ? 68 h\n");
        return 1;
    }
    printf("PASS: DECKBUM enabled\n");

    // Disable Mode 68
    write_sequence(term, "\x1B[?68l");
    if (session->dec_modes & KTERM_MODE_DECKBUM) {
        printf("FAIL: DECKBUM (Mode 68) not cleared by CSI ? 68 l\n");
        return 1;
    }
    printf("PASS: DECKBUM disabled\n");

    KTerm_Destroy(term);
    return 0;
}
