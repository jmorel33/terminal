#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Mock printer callback to verify MC commands
static char last_printer_data[256];
static void MockPrinterCallback(KTerm* term, const char* data, size_t length) {
    (void)term;
    if (length < sizeof(last_printer_data)) {
        strncpy(last_printer_data, data, length);
        last_printer_data[length] = '\0';
    }
}

// Mock response callback to verify DSR
static char last_response[256];
static void MockResponseCallback(KTerm* term, const char* response, int length) {
    (void)term;
    if (length < sizeof(last_response)) {
        strncpy(last_response, response, length);
        last_response[length] = '\0';
    }
}

void test_ed2_ansi_sys_homing(KTerm* term) {
    // Set to ANSI.SYS mode
    KTerm_SetLevel(term, GET_SESSION(term), VT_LEVEL_ANSI_SYS);

    // Move cursor away from 0,0
    GET_SESSION(term)->cursor.x = 10;
    GET_SESSION(term)->cursor.y = 10;

    // Send ED 2 (CSI 2 J)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '2');
    KTerm_ProcessChar(term, GET_SESSION(term), 'J');

    // Check if cursor moved to 0,0 (1,1)
    if (GET_SESSION(term)->cursor.x != 0 || GET_SESSION(term)->cursor.y != 0) {
        fprintf(stderr, "FAIL: ED 2 in ANSI.SYS mode did not home cursor. Got %d,%d\n",
                GET_SESSION(term)->cursor.x, GET_SESSION(term)->cursor.y);
        exit(1);
    }
    printf("PASS: ED 2 ANSI.SYS Cursor Homing\n");
}

void test_ed3_scrollback_clear(KTerm* term) {
    // Set to xterm mode (supports ED 3)
    KTerm_SetLevel(term, GET_SESSION(term), VT_LEVEL_XTERM);

    // Fill scrollback with something
    // Manually pollute the scrollback area (above rows)
    // Buffer height is rows + MAX_SCROLLBACK_LINES
    // Scrollback starts at index 0 of the ring buffer if we assume logical mapping,
    // but better to just use internal buffer access.

    // Let's just set a char in the very first line of the buffer,
    // and ensure the screen_head puts it in scrollback.
    // Or simpler: Resize implies new buffer.
    // Let's just modify the buffer directly.

    // Assuming screen_head is 0.
    // We want to verify clear.

    // Set some dirty data everywhere
    int total_cells = GET_SESSION(term)->buffer_height * GET_SESSION(term)->cols;
    for (int i=0; i<total_cells; i++) {
        GET_SESSION(term)->screen_buffer[i].ch = 'X';
    }

    // Send ED 3 (CSI 3 J)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '3');
    KTerm_ProcessChar(term, GET_SESSION(term), 'J');

    // Verify all cells are cleared (space)
    for (int i=0; i<total_cells; i++) {
        if (GET_SESSION(term)->screen_buffer[i].ch != ' ') {
            fprintf(stderr, "FAIL: ED 3 did not clear entire buffer. Found char '%c' at index %d\n",
                    GET_SESSION(term)->screen_buffer[i].ch, i);
            exit(1);
        }
    }
    printf("PASS: ED 3 Scrollback Clear\n");
}

void test_aux_port(KTerm* term) {
    KTerm_SetLevel(term, GET_SESSION(term), VT_LEVEL_100);
    GET_SESSION(term)->printer_available = true;

    // Test CSI 5 i (Auto Print On)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '5');
    KTerm_ProcessChar(term, GET_SESSION(term), 'i');

    if (!GET_SESSION(term)->auto_print_enabled) {
        fprintf(stderr, "FAIL: CSI 5 i did not enable auto print\n");
        exit(1);
    }

    // Test CSI 4 i (Auto Print Off)
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '4');
    KTerm_ProcessChar(term, GET_SESSION(term), 'i');

    if (GET_SESSION(term)->auto_print_enabled) {
        fprintf(stderr, "FAIL: CSI 4 i did not disable auto print\n");
        exit(1);
    }
    printf("PASS: AUX Port On/Off\n");
}

void test_dsr(KTerm* term) {
    KTerm_SetLevel(term, GET_SESSION(term), VT_LEVEL_100);
    KTerm_SetResponseCallback(term, MockResponseCallback);
    last_response[0] = '\0';

    // Move cursor to 5,5
    GET_SESSION(term)->cursor.x = 4;
    GET_SESSION(term)->cursor.y = 4;

    // Send CSI 6 n
    KTerm_ProcessChar(term, GET_SESSION(term), '\x1B');
    KTerm_ProcessChar(term, GET_SESSION(term), '[');
    KTerm_ProcessChar(term, GET_SESSION(term), '6');
    KTerm_ProcessChar(term, GET_SESSION(term), 'n');

    // We need to trigger update/process events loop usually, but here response is queued immediately in ExecuteDSR
    // Flush response manually if needed?
    // ExecuteDSR calls KTerm_QueueResponse which adds to answerback_buffer.
    // KTerm_Update flushes it to callback.
    // We should call KTerm_Update or manually check buffer.

    // Manually trigger flush for test
    if (GET_SESSION(term)->response_length > 0) {
        MockResponseCallback(term, GET_SESSION(term)->answerback_buffer, GET_SESSION(term)->response_length);
    }

    if (strcmp(last_response, "\x1B[5;5R") != 0) {
        fprintf(stderr, "FAIL: DSR 6n response incorrect. Got '%s', expected '\\x1B[5;5R'\n", last_response);
        exit(1);
    }
    printf("PASS: DSR 6n\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 25;

    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    test_ed2_ansi_sys_homing(term);

    // Re-create term for clean state or reset
    KTerm_Cleanup(term); free(term);
    term = KTerm_Create(config);

    test_ed3_scrollback_clear(term);

    KTerm_Cleanup(term); free(term);
    term = KTerm_Create(config);

    test_aux_port(term);
    test_dsr(term);

    KTerm_Cleanup(term); free(term);

    printf("All CSI coverage tests passed.\n");
    return 0;
}
