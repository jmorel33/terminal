#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void write_sequence(KTerm* term, const char* seq) {
    while (*seq) {
        KTerm_ProcessChar(term, GET_SESSION(term), *seq++);
    }
}

void test_modes(KTerm* term) {
    KTermSession* session = GET_SESSION(term);
    printf("Testing Mode Setting Coverage...\n");

    // 1. Test DEC Private Mode 2 (DECANM - ANSI Mode)
    session->dec_modes &= ~KTERM_MODE_VT52; // Ensure ANSI start
    write_sequence(term, "\x1B[?2l"); // RM 2 -> VT52 On
    if (!(session->dec_modes & KTERM_MODE_VT52)) {
        fprintf(stderr, "FAIL: RM ?2 did not set VT52 mode.\n");
    } else {
        printf("PASS: RM ?2 (VT52 Mode)\n");
    }

    // To return to ANSI from VT52, we MUST use ESC < (Enter ANSI Mode)
    // CSI is not available in VT52.
    write_sequence(term, "\x1B<");
    if (session->dec_modes & KTERM_MODE_VT52) {
        fprintf(stderr, "FAIL: ESC < did not clear VT52 mode.\n");
    } else {
        printf("PASS: ESC < (ANSI Mode Restored)\n");
    }

    // Now we are back in ANSI mode, CSI should work.

    // 2. Test DEC Private Mode 67 (DECBKM)
    session->dec_modes &= ~KTERM_MODE_DECBKM;
    write_sequence(term, "\x1B[?67h");
    if (!(session->dec_modes & KTERM_MODE_DECBKM)) {
        fprintf(stderr, "FAIL: SM ?67 did not set DECBKM.\n");
    } else {
        printf("PASS: SM ?67\n");
    }
    write_sequence(term, "\x1B[?67l");
    if (session->dec_modes & KTERM_MODE_DECBKM) {
        fprintf(stderr, "FAIL: RM ?67 did not clear DECBKM.\n");
    } else {
        printf("PASS: RM ?67\n");
    }

    // 3. Test DEC Private Mode 68 (DECKBUM)
    session->dec_modes &= ~KTERM_MODE_DECKBUM;
    write_sequence(term, "\x1B[?68h");
    if (!(session->dec_modes & KTERM_MODE_DECKBUM)) {
        fprintf(stderr, "FAIL: SM ?68 did not set DECKBUM.\n");
    } else {
        printf("PASS: SM ?68\n");
    }

    // 4. Test DEC Private Mode 103 (DECHDPXM)
    session->dec_modes &= ~KTERM_MODE_DECHDPXM;
    write_sequence(term, "\x1B[?103h");
    if (!(session->dec_modes & KTERM_MODE_DECHDPXM)) {
        fprintf(stderr, "FAIL: SM ?103 did not set DECHDPXM.\n");
    } else {
        printf("PASS: SM ?103\n");
    }

    // 5. Test DEC Private Mode 104 (DECESKM)
    session->dec_modes &= ~KTERM_MODE_DECESKM;
    write_sequence(term, "\x1B[?104h");
    if (!(session->dec_modes & KTERM_MODE_DECESKM)) {
        fprintf(stderr, "FAIL: SM ?104 did not set DECESKM.\n");
    } else {
        printf("PASS: SM ?104\n");
    }

    // 6. Test Mouse Modes (1000)
    session->mouse.mode = MOUSE_TRACKING_OFF;
    write_sequence(term, "\x1B[?1000h");
    if (session->mouse.mode != MOUSE_TRACKING_VT200) {
        fprintf(stderr, "FAIL: SM ?1000 did not set VT200 mouse mode.\n");
    } else {
        printf("PASS: SM ?1000\n");
    }
    write_sequence(term, "\x1B[?1000l");
    if (session->mouse.mode != MOUSE_TRACKING_OFF) {
        fprintf(stderr, "FAIL: RM ?1000 did not disable mouse mode.\n");
    } else {
        printf("PASS: RM ?1000\n");
    }

    // 7. Test SGR Mouse (1006) interaction
    write_sequence(term, "\x1B[?1006h"); // Enable SGR
    if (!session->mouse.sgr_mode) {
        fprintf(stderr, "FAIL: SM ?1006 did not set sgr_mode.\n");
    }
    write_sequence(term, "\x1B[?1000h"); // Enable Tracking
    if (session->mouse.mode != MOUSE_TRACKING_SGR) {
        fprintf(stderr, "FAIL: SM ?1000 with SGR enabled should set MOUSE_TRACKING_SGR. Got %d\n", session->mouse.mode);
    } else {
        printf("PASS: Mouse SGR Interaction\n");
    }

    // 8. Test ANSI Mode 12 (SRM - Send/Receive)
    // KTerm maps this to LOCALECHO (Inverse).
    // Set (h) = Local Echo OFF.
    // Reset (l) = Local Echo ON.

    session->dec_modes &= ~KTERM_MODE_LOCALECHO; // Start OFF
    write_sequence(term, "\x1B[12l"); // ANSI RM 12 -> Local Echo ON
    if (!(session->dec_modes & KTERM_MODE_LOCALECHO)) {
        fprintf(stderr, "FAIL: ANSI RM 12 did not enable Local Echo.\n");
    } else {
        printf("PASS: ANSI RM 12 (Local Echo On)\n");
    }

    write_sequence(term, "\x1B[12h"); // ANSI SM 12 -> Local Echo OFF
    if (session->dec_modes & KTERM_MODE_LOCALECHO) {
        fprintf(stderr, "FAIL: ANSI SM 12 did not disable Local Echo.\n");
    } else {
        printf("PASS: ANSI SM 12 (Local Echo Off)\n");
    }

    // 9. Test DEC Private Mode 64 (Multi-Session)
    session->conformance.features.multi_session_mode = false;
    write_sequence(term, "\x1B[?64h");
    if (!session->conformance.features.multi_session_mode) {
         fprintf(stderr, "FAIL: SM ?64 did not enable multi-session mode.\n");
    } else {
         printf("PASS: SM ?64\n");
    }
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    test_modes(term);

    KTerm_Destroy(term);
    return 0;
}
