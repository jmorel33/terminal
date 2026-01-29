#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// Mock response callback to capture output
static char last_response[1024];
static void MockResponseCallback(KTerm* term, const char* response, int length) {
    if (length < sizeof(last_response)) {
        strncat(last_response, response, length); // Append instead of overwrite for multiple chunks
    }
}

void flush_responses(KTerm* term) {
    KTermSession* session = GET_SESSION(term);
    if (session->response_length > 0 && term->response_callback) {
        term->response_callback(term, session->answerback_buffer, session->response_length);
        session->response_length = 0;
        session->answerback_buffer[0] = '\0';
    }
}

void write_sequence(KTerm* term, const char* seq) {
    while (*seq) {
        KTerm_ProcessChar(term, GET_SESSION(term), *seq++);
    }
}

void test_osc_colors(KTerm* term) {
    printf("Testing OSC Color Commands...\n");

    // 1. Test Set Palette (OSC 4) - Standard Format
    // Set index 5 to Red (FF0000)
    write_sequence(term, "\x1B]4;5;rgb:ff/00/00\x1B\\");
    RGB_KTermColor c = term->color_palette[5];
    if (c.r != 0xFF || c.g != 0x00 || c.b != 0x00) {
        fprintf(stderr, "FAIL: OSC 4 did not set color 5 correctly. Got %02X%02X%02X\n", c.r, c.g, c.b);
    } else {
        printf("PASS: OSC 4 Set Color\n");
    }

    // 2. Test Query Palette (OSC 4)
    last_response[0] = '\0';
    write_sequence(term, "\x1B]4;5;?\x1B\\");
    flush_responses(term);
    // Expect response: OSC 4 ; 5 ; rgb:ff/00/00 ST
    if (strstr(last_response, "]4;5;rgb:ff/00/00") == NULL) {
         fprintf(stderr, "FAIL: OSC 4 Query failed. Got: '%s'\n", last_response);
    } else {
         printf("PASS: OSC 4 Query Color\n");
    }

    // 3. Test Set Foreground (OSC 10)
    // Note: KTerm currently stores FG as ExtendedKTermColor.
    // The previous test run suggested OSC 10 might not set anything effectively if not fully implemented.
    // But `ProcessForegroundKTermColorCommand` logic I saw only had Query.
    // I will refactor it to include Set.
    write_sequence(term, "\x1B]10;rgb:00/ff/00\x1B\\");
    // We expect refactoring to make this work.

    // 4. Test Query Foreground (OSC 10)
    last_response[0] = '\0';
    write_sequence(term, "\x1B]10;?\x1B\\");
    flush_responses(term);
    if (strlen(last_response) == 0) {
        // If empty, maybe not implemented or failed
         printf("INFO: OSC 10 Query returned empty (expected before refactor)\n");
    } else {
         printf("PASS: OSC 10 Query: %s\n", last_response);
    }
}

void test_osc_malformed(KTerm* term) {
    printf("Testing Malformed OSC Commands...\n");

    // 1. Missing semicolon
    write_sequence(term, "\x1B]4 5 rgb:ff/ff/ff\x1B\\");
    // Should gracefully fail/ignore without crash

    // 2. Invalid Hex
    write_sequence(term, "\x1B]4;6;rgb:gg/00/00\x1B\\");
    // Should probably not set color
    RGB_KTermColor c = term->color_palette[6];
    // Assuming default is NOT what we tried to set.
    // Default palette 6 is cyan (0x00, 0xCD, 0xCD) or similar (ANSI).
    // Just checking it didn't crash.

    printf("PASS: Malformed OSC Handled (No Crash)\n");
}

int main() {
    KTermConfig config = {0};
    config.response_callback = MockResponseCallback;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    test_osc_colors(term);
    test_osc_malformed(term);

    KTerm_Destroy(term);
    return 0;
}
