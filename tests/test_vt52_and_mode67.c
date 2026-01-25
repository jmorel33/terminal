#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_mode_2_vt52_switching(KTerm* term) {
    // 1. Ensure we start in ANSI mode
    GET_SESSION(term)->dec_modes &= ~KTERM_MODE_VT52;
    GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;

    // 2. Send CSI ? 2 l (Reset to VT52 Mode)
    // Sequence: ESC [ ? 2 l
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '?');
    KTerm_ProcessChar(term, GET_SESSION(term), '2');
    KTerm_ProcessChar(term, GET_SESSION(term), 'l');

    if (!(GET_SESSION(term)->dec_modes & KTERM_MODE_VT52)) {
        fprintf(stderr, "FAIL: CSI ? 2 l should enable VT52 mode\n");
        exit(1);
    }

    // BUG REGRESSION TEST:
    // Verify that we are NOT in PARSE_VT52 state immediately.
    // Sending 'H' (VT52 Home) should print 'H' (move cursor +1), not Home Cursor (move to 0,0).
    GET_SESSION(term)->cursor.x = 10;
    GET_SESSION(term)->cursor.y = 10;
    KTerm_ProcessChar(term, GET_SESSION(term), 'H');

    if (GET_SESSION(term)->cursor.x == 0 && GET_SESSION(term)->cursor.y == 0) {
        fprintf(stderr, "FAIL: 'H' immediately after mode switch acted as command (Home)!\n");
        exit(1);
    }

    if (GET_SESSION(term)->cursor.x != 11) {
         fprintf(stderr, "FAIL: 'H' should have printed (x=11), but got x=%d\n", GET_SESSION(term)->cursor.x);
         exit(1);
    }

    // 3. Test VT52 Command execution (Explicit ESC)
    // Reset cursor
    GET_SESSION(term)->cursor.x = 5;
    GET_SESSION(term)->cursor.y = 5;

    // Send ESC A (Cursor Up)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), 'A');

    if (GET_SESSION(term)->cursor.y != 4) {
        fprintf(stderr, "FAIL: VT52 Cursor Up (ESC A) failed in VT52 mode. y=%d\n", GET_SESSION(term)->cursor.y);
        exit(1);
    }
    printf("PASS: VT52 Mode Active (ESC A works)\n");

    // 4. Test ANSI command failure (CSI)
    // Send ESC [ A (ANSI Cursor Up)
    // In VT52 mode, ESC enters VT52 command mode. '[' is not a standard VT52 command.
    // It might be ignored or printed? Standard says undefined commands are ignored.
    // Let's see if it is treated as CSI.
    GET_SESSION(term)->cursor.y = 5;
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '['); // Should be consumed as VT52 param or command?
    // If [ is not a command, it returns to NORMAL.
    KTerm_ProcessChar(term, GET_SESSION(term), 'A'); // Should be printed 'A'

    // Cursor should NOT move
    if (GET_SESSION(term)->cursor.y != 5) {
        fprintf(stderr, "FAIL: ANSI CSI sequence executed in VT52 mode! y=%d\n", GET_SESSION(term)->cursor.y);
        exit(1);
    }
    printf("PASS: ANSI CSI ignored in VT52 mode\n");

    // 5. Return to ANSI Mode (ESC <)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '<');

    if (GET_SESSION(term)->dec_modes & KTERM_MODE_VT52) {
        fprintf(stderr, "FAIL: ESC < should disable VT52 mode\n");
        exit(1);
    }

    // 6. Test ANSI command success
    GET_SESSION(term)->cursor.y = 5;
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), 'A');

    if (GET_SESSION(term)->cursor.y != 4) {
        fprintf(stderr, "FAIL: ANSI CSI failed after returning to ANSI mode. y=%d\n", GET_SESSION(term)->cursor.y);
        exit(1);
    }
    printf("PASS: Return to ANSI mode successful\n");
}

void test_mode_67_backarrow(KTerm* term) {
    // 1. Send CSI ? 67 h (Set BS)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '?');
    KTerm_ProcessChar(term, GET_SESSION(term), '6');
    KTerm_ProcessChar(term, GET_SESSION(term), '7');
    KTerm_ProcessChar(term, GET_SESSION(term), 'h');

    if (!(GET_SESSION(term)->dec_modes & KTERM_MODE_DECBKM)) {
        fprintf(stderr, "FAIL: CSI ? 67 h should set backarrow_key_mode\n");
        exit(1);
    }
    if (!GET_SESSION(term)->input.backarrow_sends_bs) {
        fprintf(stderr, "FAIL: CSI ? 67 h should set input.backarrow_sends_bs to true\n");
        exit(1);
    }
    printf("PASS: Mode 67 Set (BS)\n");

    // 2. Send CSI ? 67 l (Reset DEL)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '?');
    KTerm_ProcessChar(term, GET_SESSION(term), '6');
    KTerm_ProcessChar(term, GET_SESSION(term), '7');
    KTerm_ProcessChar(term, GET_SESSION(term), 'l');

    if (GET_SESSION(term)->dec_modes & KTERM_MODE_DECBKM) {
        fprintf(stderr, "FAIL: CSI ? 67 l should clear backarrow_key_mode\n");
        exit(1);
    }
    if (GET_SESSION(term)->input.backarrow_sends_bs) {
        fprintf(stderr, "FAIL: CSI ? 67 l should set input.backarrow_sends_bs to false\n");
        exit(1);
    }
    printf("PASS: Mode 67 Reset (DEL)\n");
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

    // Ensure we are in a mode that supports these features (e.g. VT420 or XTERM)
    KTerm_SetLevel(term, VT_LEVEL_XTERM);

    test_mode_2_vt52_switching(term);
    test_mode_67_backarrow(term);

    KTerm_Destroy(term);
    printf("All VT52 and Mode 67 tests passed.\n");
    return 0;
}
