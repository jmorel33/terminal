#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_esc_sp_f_g(KTerm* term) {
    // Default: 7-bit controls (use_8bit_controls = false)
    if (GET_SESSION(term)->input.use_8bit_controls) {
        fprintf(stderr, "FAIL: Default should be 7-bit controls\n");
        exit(1);
    }

    // Send ESC SP G (S8C1T)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), ' ');
    KTerm_ProcessChar(term, GET_SESSION(term), 'G');

    if (!GET_SESSION(term)->input.use_8bit_controls) {
        fprintf(stderr, "FAIL: ESC SP G should enable 8-bit controls\n");
        exit(1);
    }

    // Send ESC SP F (S7C1T)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), ' ');
    KTerm_ProcessChar(term, GET_SESSION(term), 'F');

    if (GET_SESSION(term)->input.use_8bit_controls) {
        fprintf(stderr, "FAIL: ESC SP F should disable 8-bit controls\n");
        exit(1);
    }
    printf("PASS: ESC SP F/G (S7C1T/S8C1T)\n");
}

void test_esc_hash(KTerm* term) {
    // Test DECDHL Top
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '#');
    KTerm_ProcessChar(term, GET_SESSION(term), '3');

    EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, 0);
    if (!(cell->flags & KTERM_ATTR_DOUBLE_HEIGHT_TOP)) {
        fprintf(stderr, "FAIL: ESC # 3 did not set DOUBLE_HEIGHT_TOP\n");
        exit(1);
    }

    // Test DECSWL
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '#');
    KTerm_ProcessChar(term, GET_SESSION(term), '5');

    cell = GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, 0);
    if (cell->flags & (KTERM_ATTR_DOUBLE_HEIGHT_TOP | KTERM_ATTR_DOUBLE_WIDTH)) {
        fprintf(stderr, "FAIL: ESC # 5 did not clear double attributes\n");
        exit(1);
    }
    printf("PASS: ESC # (DECDHL/DECSWL)\n");
}

void test_ansi_key_reassignment_safety(KTerm* term) {
    // Set ANSI.SYS mode
    KTerm_SetLevel(term, GET_SESSION(term), VT_LEVEL_ANSI_SYS);
    KTerm_EnableDebug(term, true); // Log output

    // Send malformed sequence with string: CSI 0; "DIR"; 13 p
    // This is tricky because standard parser expects integers.
    // If we send it, we want to ensure it doesn't crash or hang the parser.
    const char* seq = "\x1B[0;\"DIR\";13p";
    for (int i=0; seq[i]; i++) {
        KTerm_ProcessChar(term, GET_SESSION(term), seq[i]);
    }

    // Check if parser recovered (state should be NORMAL)
    if (GET_SESSION(term)->parse_state != VT_PARSE_NORMAL) {
        fprintf(stderr, "FAIL: Parser stuck in state %d after ANSI key reassignment attempt\n", GET_SESSION(term)->parse_state);
        exit(1);
    }
    printf("PASS: ANSI Key Reassignment Safety\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    // We need to add use_8bit_controls to KTermInputConfig first,
    // but for TDD we write the test assuming it exists or will exist.
    // Since I can't compile if it doesn't exist, I'll have to modify kterm.h first or comment this out.
    // I'll modify kterm.h in the next step. For now, I'll comment out the struct check
    // and just check if the sequence is accepted (not unsupported).
    // Actually, checking internal state is better. I will add the field to kterm.h in step 3.

    // For now, I will assume the field will be named `use_8bit_controls`.

    test_esc_sp_f_g(term);
    test_esc_hash(term);
    test_ansi_key_reassignment_safety(term);

    KTerm_Cleanup(term);
    free(term);
    printf("All nF/Hash/ANSI tests passed.\n");
    return 0;
}
