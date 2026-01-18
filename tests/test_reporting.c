#define TERMINAL_IMPLEMENTATION
#define TERMINAL_TESTING
#include "terminal.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static Terminal* term = NULL;

// Mock response callback
static char last_response[4096];
static int response_count = 0;

void TestResponseCallback(Terminal* term, const char* response, int length) {
    if (length < sizeof(last_response)) {
        memcpy(last_response, response, length);
        last_response[length] = '\0';
    } else {
        memcpy(last_response, response, sizeof(last_response) - 1);
        last_response[sizeof(last_response) - 1] = '\0';
    }
    response_count++;
    printf("Response received: %s\n", last_response);
}

void FlushResponse() {
    if (ACTIVE_SESSION.response_length > 0 && term->response_callback) {
        term->response_callback(term, ACTIVE_SESSION.answerback_buffer, ACTIVE_SESSION.response_length);
        ACTIVE_SESSION.response_length = 0;
    }
}

void TestDECRS(void) {
    printf("Testing DECRS (Session Status)...\n");
    // Send CSI ? 21 n
    // Buffer the response
    response_count = 0;
    last_response[0] = '\0';

    term->response_callback = TestResponseCallback;

    // Simulate input
    // CSI ? 21 n
    ProcessChar(term, '\x1B');
    ProcessChar(term, '[');
    ProcessChar(term, '?');
    ProcessChar(term, '2');
    ProcessChar(term, '1');
    ProcessChar(term, 'n');

    FlushResponse();

    // Expected: DCS $ p 1;2;0|2;3;0|3;3;0 ST
    if (strstr(last_response, "\x1BP$p") && strstr(last_response, "1;2;0")) {
        printf("PASS: DECRS response valid format.\n");
    } else {
        printf("FAIL: DECRS response invalid: %s\n", last_response);
    }
}

void TestDECRQSS_SGR(void) {
    printf("Testing DECRQSS SGR...\n");
    response_count = 0;
    last_response[0] = '\0';

    // Set some attributes
    ACTIVE_SESSION.bold_mode = true;
    ACTIVE_SESSION.current_fg.value.index = 1; // Red (ANSI 1 -> 31)
    ACTIVE_SESSION.current_fg.color_mode = 0;

    // Send DCS $ q m ST
    ProcessChar(term, '\x1B');
    ProcessChar(term, 'P');
    ProcessChar(term, '$');
    ProcessChar(term, 'q');
    ProcessChar(term, 'm');
    ProcessChar(term, '\x1B'); // ESC
    ProcessChar(term, '\\');   // ST

    FlushResponse();

    // Expected: DCS 1 $ r 0;1;31 m ST
    // Note: My implementation outputs \x1BP1$r...m\x1B\\
    // 0;1;31 (Reset, Bold, Red)
    // Red index 1 is standard color. < 8 -> 30+1 = 31.
    // So "0;1;31" is expected.

    if (strstr(last_response, "\x1BP1$r0;1;31m\x1B\\")) {
        printf("PASS: DECRQSS SGR response correct.\n");
    } else {
        printf("FAIL: DECRQSS SGR response incorrect: %s\n", last_response);
    }
}

void TestDECRQSS_Margins(void) {
    printf("Testing DECRQSS Margins...\n");
    response_count = 0;
    last_response[0] = '\0';

    // Send DCS $ q r ST
    ProcessChar(term, '\x1B');
    ProcessChar(term, 'P');
    ProcessChar(term, '$');
    ProcessChar(term, 'q');
    ProcessChar(term, 'r');
    ProcessChar(term, '\x1B'); // ESC
    ProcessChar(term, '\\');   // ST

    FlushResponse();

    // Expected: DCS 1 $ r 1;50 r ST (Assuming defaults)

    if (strstr(last_response, "\x1BP1$r1;50r\x1B\\")) {
        printf("PASS: DECRQSS Margins response correct.\n");
    } else {
        printf("FAIL: DECRQSS Margins response incorrect: %s\n", last_response);
    }
}

int main() {
    TerminalConfig config = {
        .width = 80,
        .height = 24
    };
    term = Terminal_Create(config);

    // Enable VT525 level for Multi-Session DECRS test
    SetVTLevel(term, VT_LEVEL_525);

    TestDECRS();
    TestDECRQSS_SGR();
    TestDECRQSS_Margins();

    Terminal_Destroy(term);
    return 0;
}
