#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define TERMINAL_IMPLEMENTATION
#define TERMINAL_TESTING
#include "../terminal.h"

static Terminal* term = NULL;

// Mock callback to capture response
char last_response[1024];
int last_response_len = 0;

void MockResponseCallback(Terminal* term, const char* response, int length) {
    if (length < sizeof(last_response) - 1) {
        memcpy(last_response, response, length);
        last_response[length] = '\0';
        last_response_len = length;
    }
}

void FlushResponse() {
    if (GET_SESSION(term)->response_length > 0 && term->response_callback) {
        term->response_callback(term, GET_SESSION(term)->answerback_buffer, GET_SESSION(term)->response_length);
        GET_SESSION(term)->response_length = 0;
    }
}

int main() {

    TerminalConfig config = {0};
    term = Terminal_Create(config);

    SetResponseCallback(term, MockResponseCallback);

    // 1. Enable Overline Mode (SGR 53) using high-level pipeline
    // We can use ProcessChar directly to simulate input stream
    printf("Setting Overline Mode (SGR 53)...\n");

    const char* sgr_seq = "\x1B[53m";
    for (int i = 0; sgr_seq[i]; i++) {
        ProcessChar(term, sgr_seq[i]);
    }

    // Verify manually if overline_mode is set
    if (term->sessions[0].overline_mode) {
        printf("Overline Mode is ACTIVE.\n");
    } else {
        printf("Overline Mode is NOT ACTIVE (Failed to set via CSI).\n");
        return 1;
    }

    // 2. Send DECRQSS for SGR
    // DCS $ q m ST
    printf("Sending DECRQSS for SGR...\n");

    // Clear response buffer
    last_response[0] = '\0';

    const char* dcs_seq = "\x1BP$qm\x1B\\";
    for (int i = 0; dcs_seq[i]; i++) {
        ProcessChar(term, dcs_seq[i]);
    }

    FlushResponse();

    // 3. Check Response
    printf("Response: %s\n", last_response);

    // Expected: DCS 1 $ r ... ;53 ... m ST
    // Note: The response format includes DCS ... ST.
    if (strstr(last_response, ";53") != NULL) {
        printf("SUCCESS: Response contains ';53' (Overline).\n");
    } else {
        printf("FAILURE: Response does NOT contain ';53'.\n");
        return 1;
    }

    return 0;
}
