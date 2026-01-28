#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

char output_buffer[4096];
int output_pos = 0;

void response_callback(KTerm* term, const char* response, int length) {
    if (output_pos + length < sizeof(output_buffer)) {
        memcpy(output_buffer + output_pos, response, length);
        output_pos += length;
        output_buffer[output_pos] = '\0';
    }
}

void write_sequence(KTerm* term, const char* seq) {
    KTerm_WriteString(term, seq);
    KTerm_ProcessEvents(term);
    KTerm_Update(term);
}

void test_decsnls(KTerm* term) {
    printf("Testing DECSNLS (Set Number of Lines per Screen)...\n");
    KTermSession* session = GET_SESSION(term);

    // Initial rows
    assert(session->rows == 25);

    // Resize to 36 lines
    write_sequence(term, "\x1B[36*|");
    if (session->rows != 36) {
        fprintf(stderr, "FAIL: DECSNLS 36 failed. Rows: %d\n", session->rows);
    } else {
        printf("PASS: DECSNLS 36\n");
    }

    // Resize to 48 lines
    write_sequence(term, "\x1B[48*|");
    if (session->rows != 48) {
        fprintf(stderr, "FAIL: DECSNLS 48 failed. Rows: %d\n", session->rows);
    } else {
        printf("PASS: DECSNLS 48\n");
    }
}

void test_decslpp(KTerm* term) {
    printf("Testing DECSLPP (Set Lines Per Page)...\n");
    KTermSession* session = GET_SESSION(term);

    write_sequence(term, "\x1B[66*{");
    if (session->lines_per_page != 66) {
        fprintf(stderr, "FAIL: DECSLPP 66 failed. Lines: %d\n", session->lines_per_page);
    } else {
        printf("PASS: DECSLPP 66\n");
    }
}

void test_decrqpku(KTerm* term) {
    printf("Testing DECRQPKU (Request Programmed Key)...\n");
    KTermSession* session = GET_SESSION(term);

    // Manually inject a UDK for F6 (key code 17 -> SIT_KEY_F6 which is typically 295 or similar,
    // but we use the SIT_KEY_F6 macro if available, or just map manually in implementation)
    // Wait, implementation maps 17 -> SIT_KEY_F6.
    // I need to know what SIT_KEY_F6 value is.
    // In mock_situation.h (or situation.h), keys are defined.
    // I will search for a keycode in programmable_keys.

    // Allocate keys
    session->programmable_keys.capacity = 1;
    session->programmable_keys.count = 1;
    session->programmable_keys.keys = (ProgrammableKey*)calloc(1, sizeof(ProgrammableKey));
    session->programmable_keys.keys[0].key_code = SIT_KEY_F6;
    session->programmable_keys.keys[0].sequence = strdup("HELLO");
    session->programmable_keys.keys[0].active = true;

    output_pos = 0;
    // Query F6 (17)
    write_sequence(term, "\x1B[?26;17u");

    // Expected response: DCS 17 ; 1 ; HELLO ST (\x1BP 17 ; 1 ; HELLO \x1B \\)
    const char* expected = "\x1BP17;1;HELLO\x1B\\";
    if (strstr(output_buffer, expected)) {
        printf("PASS: DECRQPKU Response correct: %s\n", output_buffer);
    } else {
        fprintf(stderr, "FAIL: DECRQPKU Response incorrect. Expected '%s', Got '%s'\n", expected, output_buffer);
    }
}

void test_decskcv(KTerm* term) {
    printf("Testing DECSKCV (Select Keyboard Variant)...\n");
    KTermSession* session = GET_SESSION(term);

    write_sequence(term, "\x1B[5 =");
    if (session->input.keyboard_variant != 5) {
        fprintf(stderr, "FAIL: DECSKCV 5 failed. Variant: %d\n", session->input.keyboard_variant);
    } else {
        printf("PASS: DECSKCV 5\n");
    }

    write_sequence(term, "\x1B[0 =");
    if (session->input.keyboard_variant != 0) {
        fprintf(stderr, "FAIL: DECSKCV 0 failed. Variant: %d\n", session->input.keyboard_variant);
    } else {
        printf("PASS: DECSKCV 0\n");
    }
}

void test_decxrlm(KTerm* term) {
    printf("Testing DECXRLM (Flow Control)...\n");
    KTermSession* session = GET_SESSION(term);

    // Enable DECXRLM
    write_sequence(term, "\x1B[?88h");

    // Verify mode enabled
    if (!(session->dec_modes & KTERM_MODE_DECXRLM)) {
        fprintf(stderr, "FAIL: DECXRLM enable failed.\n");
        return;
    }

    output_pos = 0;

    // Fill pipeline to trigger XOFF (>75% of 1024*1024)
    // Pipeline size is huge (1MB).
    // I can cheat by manually setting pipeline pointers or usage?
    // Or reducing pipeline size for test?
    // Modifying internal state directly for test.
    // head = 800000, tail = 0. Usage ~ 800KB (~80%).

    atomic_store(&session->pipeline_head, 800000);
    atomic_store(&session->pipeline_tail, 0);

    // Run Update (which calls ProcessEventsInternal)
    // Note: pipeline_overflow check might trigger?
    // ProcessEventsInternal calculates usage.

    KTerm_Update(term);

    // Check for XOFF (0x13)
    bool found_xoff = false;
    for(int i=0; i<output_pos; i++) {
        if (output_buffer[i] == 0x13) found_xoff = true;
    }

    if (found_xoff) {
        printf("PASS: DECXRLM XOFF sent.\n");
    } else {
        fprintf(stderr, "FAIL: DECXRLM XOFF not sent. Output len: %d\n", output_pos);
    }

    // Now clear pipeline (< 25%)
    output_pos = 0;
    atomic_store(&session->pipeline_head, 100);
    atomic_store(&session->pipeline_tail, 0);

    KTerm_Update(term);

    bool found_xon = false;
    for(int i=0; i<output_pos; i++) {
        if (output_buffer[i] == 0x11) found_xon = true;
    }

    if (found_xon) {
        printf("PASS: DECXRLM XON sent.\n");
    } else {
        fprintf(stderr, "FAIL: DECXRLM XON not sent.\n");
    }
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;
    config.response_callback = response_callback;

    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    test_decsnls(term);
    test_decslpp(term);
    test_decrqpku(term);
    test_decskcv(term);
    test_decxrlm(term);

    KTerm_Destroy(term);
    return 0;
}
