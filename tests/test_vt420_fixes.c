#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void send_sequence(KTerm* term, const char* seq) {
    while (*seq) {
        KTerm_ProcessChar(term, GET_SESSION(term), *seq++);
    }
}

void test_declrmm_margin_mode(KTerm* term) {
    // 1. Check default: DECLRMM should be false
    if (GET_SESSION(term)->dec_modes & KTERM_MODE_DECLRMM) {
        fprintf(stderr, "FAIL: DECLRMM should be disabled by default\n");
        exit(1);
    }

    // 2. Enable DECLRMM: CSI ? 69 h
    send_sequence(term, "\x1B[?69h");
    if (!(GET_SESSION(term)->dec_modes & KTERM_MODE_DECLRMM)) {
        fprintf(stderr, "FAIL: CSI ? 69 h should enable DECLRMM\n");
        exit(1);
    }

    // 3. Set Margins (DECSLRM): CSI 2 ; 10 s (Left=2, Right=10)
    // Note: 1-based args. Internal 0-based. Left=1, Right=9.
    send_sequence(term, "\x1B[2;10s");

    if (GET_SESSION(term)->left_margin != 1 || GET_SESSION(term)->right_margin != 9) {
        fprintf(stderr, "FAIL: DECSLRM failed to set margins. Got L=%d, R=%d. Expected 1, 9\n",
                GET_SESSION(term)->left_margin, GET_SESSION(term)->right_margin);
        exit(1);
    }

    // 4. Disable DECLRMM: CSI ? 69 l
    send_sequence(term, "\x1B[?69l");
    if (GET_SESSION(term)->dec_modes & KTERM_MODE_DECLRMM) {
        fprintf(stderr, "FAIL: CSI ? 69 l should disable DECLRMM\n");
        exit(1);
    }

    // 5. Verify Margins Reset on Disable
    if (GET_SESSION(term)->left_margin != 0 || GET_SESSION(term)->right_margin != term->width - 1) {
        fprintf(stderr, "FAIL: Disabling DECLRMM should reset margins. Got L=%d, R=%d\n",
                GET_SESSION(term)->left_margin, GET_SESSION(term)->right_margin);
        exit(1);
    }

    // 6. Verify CSI s acts as SCOSC (Save Cursor) when DECLRMM disabled
    // Move cursor
    GET_SESSION(term)->cursor.x = 5;
    GET_SESSION(term)->cursor.y = 5;
    send_sequence(term, "\x1B[s"); // Save
    GET_SESSION(term)->cursor.x = 0;
    GET_SESSION(term)->cursor.y = 0;
    send_sequence(term, "\x1B[u"); // Restore

    if (GET_SESSION(term)->cursor.x != 5 || GET_SESSION(term)->cursor.y != 5) {
        fprintf(stderr, "FAIL: CSI s should save cursor when DECLRMM disabled\n");
        exit(1);
    }

    printf("PASS: DECLRMM (Mode 69) and DECSLRM Syntax\n");
}

void test_deccolm_resizing(KTerm* term) {
    // Current width should be 80 (from init)
    if (term->width != 80) {
        fprintf(stderr, "FAIL: Initial width should be 80\n");
        exit(1);
    }

    // 1. Enable DECCOLM (132 cols): CSI ? 3 h
    send_sequence(term, "\x1B[?3h");

    if (term->width != 132) {
        fprintf(stderr, "FAIL: CSI ? 3 h should resize to 132 cols. Got %d\n", term->width);
        exit(1);
    }
    if (GET_SESSION(term)->cols != 132) {
        fprintf(stderr, "FAIL: Session cols should resize to 132. Got %d\n", GET_SESSION(term)->cols);
        exit(1);
    }

    // 2. Disable DECCOLM (80 cols): CSI ? 3 l
    send_sequence(term, "\x1B[?3l");
    if (term->width != 80) {
        fprintf(stderr, "FAIL: CSI ? 3 l should resize to 80 cols. Got %d\n", term->width);
        exit(1);
    }

    // 3. Test DECNCSM (No Clear on Switch) - Mode 95
    send_sequence(term, "\x1B[?95h"); // Enable No Clear
    if (!(GET_SESSION(term)->dec_modes & KTERM_MODE_DECNCSM)) {
        fprintf(stderr, "FAIL: CSI ? 95 h should enable no_clear_on_column_change\n");
        exit(1);
    }

    // Write something to screen
    EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), 0, 0);
    cell->ch = 'X';

    // Switch to 132
    send_sequence(term, "\x1B[?3h");

    // Check if 'X' is still there (clearing usually wipes it)
    // Note: Resize might preserve content if implemented correctly, but `no_clear` flag explicitly prevents the *commanded* clear.
    // My implementation of Resize preserves content from top-left.
    // My implementation of DECCOLM in SetModeInternal calls Resize, then explicitly Clears Screen if !no_clear.
    // Since no_clear is true, it should NOT clear.

    cell = GetScreenCell(GET_SESSION(term), 0, 0);
    if (cell->ch != 'X') {
        fprintf(stderr, "FAIL: Screen content lost after resize with DECNCSM enabled. Got %c\n", (char)cell->ch);
        // This might fail if Resize itself clears, but Resize logic copies.
        exit(1);
    }

    printf("PASS: DECCOLM (Mode 3) Resizing and DECNCSM (Mode 95)\n");
}

void test_decrqcra_syntax(KTerm* term) {
    // Enable Rectangular Operations
    GET_SESSION(term)->conformance.features.rectangular_operations = true;

    // Checksum Rectangular Area
    // Standard: CSI Pi ; Pg ; Pt ; Pl ; Pb ; Pr * y
    // Request ID 1, Page 1, Top 1, Left 1, Bottom 1, Right 1
    // Should trigger DECRQCRA.

    // Reset buffer
    GET_SESSION(term)->response_length = 0;

    send_sequence(term, "\x1B[1;1;1;1;1;1*y");

    // Check response in answerback_buffer
    // Expected: DCS 1 ! ~ xxxx ST
    // xxxx is hex checksum.
    if (GET_SESSION(term)->response_length == 0) {
        fprintf(stderr, "FAIL: No response to DECRQCRA\n");
        exit(1);
    }

    if (strncmp(GET_SESSION(term)->answerback_buffer, "\x1BP1!~", 5) != 0) {
        fprintf(stderr, "FAIL: DECRQCRA Response format wrong. Got: %s\n", GET_SESSION(term)->answerback_buffer);
        exit(1);
    }

    printf("PASS: DECRQCRA Syntax (* y)\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;

    KTerm* term = KTerm_Create(config);
    if (!term) {
        fprintf(stderr, "Failed to create terminal\n");
        return 1;
    }

    // Set to VT420 level for rectangular ops support
    KTerm_SetLevel(term, GET_SESSION(term), VT_LEVEL_420);

    test_declrmm_margin_mode(term);
    test_deccolm_resizing(term);
    test_decrqcra_syntax(term);

    KTerm_Destroy(term);
    printf("All VT420 fixes tests passed.\n");
    return 0;
}
