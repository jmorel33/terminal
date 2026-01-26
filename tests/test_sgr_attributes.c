#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

// Helper definitions matching the planned modification
#define NEW_ATTR_FRAMED       (1 << 16)
#define NEW_ATTR_ENCIRCLED    (1 << 17)
#define NEW_ATTR_SUPERSCRIPT  (1 << 19)
#define NEW_ATTR_SUBSCRIPT    (1 << 23)
#define NEW_ATTR_PROTECTED    (1 << 28)
#define NEW_ATTR_SOFT_HYPHEN  (1 << 29)

void write_sequence(KTerm* term, const char* seq) {
    while (*seq) {
        KTerm_ProcessChar(term, GET_SESSION(term), *seq++);
    }
}

void test_extended_sgr(KTerm* term) {
    KTermSession* session = GET_SESSION(term);
    KTerm_ResetAllAttributes(term);

    printf("Testing SGR Extended Attributes...\n");

    // 1. Test Framed (SGR 51)
    write_sequence(term, "\x1B[51m");
    if (!(session->current_attributes & NEW_ATTR_FRAMED)) {
        fprintf(stderr, "FAIL: Framed attribute (Bit 16) not set by SGR 51. Current: %08X\n", session->current_attributes);
    } else {
        printf("PASS: Framed (SGR 51)\n");
    }

    // 2. Test Encircled (SGR 52)
    write_sequence(term, "\x1B[0m"); // Reset
    write_sequence(term, "\x1B[52m");
    if (!(session->current_attributes & NEW_ATTR_ENCIRCLED)) {
        fprintf(stderr, "FAIL: Encircled attribute (Bit 17) not set by SGR 52. Current: %08X\n", session->current_attributes);
    } else {
        printf("PASS: Encircled (SGR 52)\n");
    }

    // 3. Test Framed/Encircled Clear (SGR 54)
    write_sequence(term, "\x1B[51;52m"); // Set both
    if (!((session->current_attributes & NEW_ATTR_FRAMED) && (session->current_attributes & NEW_ATTR_ENCIRCLED))) {
        fprintf(stderr, "FAIL: Failed to set both Framed and Encircled\n");
    }
    write_sequence(term, "\x1B[54m"); // Clear both
    if ((session->current_attributes & NEW_ATTR_FRAMED) || (session->current_attributes & NEW_ATTR_ENCIRCLED)) {
        fprintf(stderr, "FAIL: SGR 54 did not clear Framed/Encircled\n");
    } else {
        printf("PASS: Clear Framed/Encircled (SGR 54)\n");
    }

    // 4. Test Superscript (SGR 73)
    write_sequence(term, "\x1B[0m");
    write_sequence(term, "\x1B[73m");
    if (!(session->current_attributes & NEW_ATTR_SUPERSCRIPT)) {
        fprintf(stderr, "FAIL: Superscript attribute (Bit 19) not set by SGR 73. Current: %08X\n", session->current_attributes);
    } else {
        printf("PASS: Superscript (SGR 73)\n");
    }

    // 5. Test Subscript (SGR 74)
    write_sequence(term, "\x1B[0m");
    write_sequence(term, "\x1B[74m");
    if (!(session->current_attributes & NEW_ATTR_SUBSCRIPT)) {
        fprintf(stderr, "FAIL: Subscript attribute (Bit 23) not set by SGR 74. Current: %08X\n", session->current_attributes);
    } else {
        printf("PASS: Subscript (SGR 74)\n");
    }

    // 6. Test Mutual Exclusion (SGR 73 clears 74 and vice versa)
    write_sequence(term, "\x1B[73m"); // Set Super
    write_sequence(term, "\x1B[74m"); // Set Sub (Should clear Super)
    if (session->current_attributes & NEW_ATTR_SUPERSCRIPT) {
        fprintf(stderr, "FAIL: Superscript not cleared when Subscript set\n");
    }
    if (!(session->current_attributes & NEW_ATTR_SUBSCRIPT)) {
        fprintf(stderr, "FAIL: Subscript not set\n");
    } else {
        printf("PASS: Subscript/Superscript Mutual Exclusion\n");
    }

    // 7. Test Super/Sub Clear (SGR 75)
    write_sequence(term, "\x1B[75m");
    if (session->current_attributes & NEW_ATTR_SUBSCRIPT) {
        fprintf(stderr, "FAIL: SGR 75 did not clear Subscript\n");
    } else {
        printf("PASS: Clear Super/Sub (SGR 75)\n");
    }

    // 8. Verify Relocation of Protected (DECSCA 1)
    write_sequence(term, "\x1B[0m");
    write_sequence(term, "\x1B[1\"q"); // DECSCA 1 (Select Protected)
    if (!(session->current_attributes & NEW_ATTR_PROTECTED)) {
        fprintf(stderr, "FAIL: Protected attribute not found at relocated Bit 28. Current: %08X\n", session->current_attributes);
    } else {
        printf("PASS: Relocated Protected Attribute (DECSCA 1)\n");
    }
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    test_extended_sgr(term);

    KTerm_Destroy(term);
    return 0;
}
