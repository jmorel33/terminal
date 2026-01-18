#include "terminal.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static Terminal* term = NULL;

// Mock callbacks
static char last_gateway_class[64];
static char last_gateway_id[64];
static char last_gateway_command[64];
static char last_gateway_params[256];
static int gateway_call_count = 0;

void MockGatewayCallback(const char* class_id, const char* id, const char* command, const char* params) {
    printf("Gateway Callback: Class=%s, ID=%s, Cmd=%s, Params=%s\n", class_id, id, command, params);
    strncpy(last_gateway_class, class_id, sizeof(last_gateway_class) - 1);
    strncpy(last_gateway_id, id, sizeof(last_gateway_id) - 1);
    strncpy(last_gateway_command, command, sizeof(last_gateway_command) - 1);
    strncpy(last_gateway_params, params, sizeof(last_gateway_params) - 1);
    gateway_call_count++;
}

int main() {

    TerminalConfig config = {0};
    term = Terminal_Create(config);

    SetGatewayCallback(term, MockGatewayCallback);

    printf("Testing Gateway Protocol...\n");

    // Test Case 1: Standard Command
    // DCS GATE MAT;1;SET;COLOR;RED ST
    // In pipeline, this is ESC P G A T E ... ST
    const char* dcs_seq = "\x1BPGATE;MAT;1;SET;COLOR;RED\x1B\\";

    // Reset state
    gateway_call_count = 0;
    memset(last_gateway_class, 0, sizeof(last_gateway_class));

    // Process
    for (int i = 0; dcs_seq[i] != '\0'; i++) {
        ProcessChar(term, (unsigned char)dcs_seq[i]);
    }

    // Verify
    if (gateway_call_count == 1 &&
        strcmp(last_gateway_class, "MAT") == 0 &&
        strcmp(last_gateway_id, "1") == 0 &&
        strcmp(last_gateway_command, "SET") == 0 &&
        strcmp(last_gateway_params, "COLOR;RED") == 0) {
        printf("PASS: Standard Gateway Command\n");
    } else {
        printf("FAIL: Standard Gateway Command\n");
        printf("Expected: MAT, 1, SET, COLOR;RED\n");
        printf("Got: %s, %s, %s, %s\n", last_gateway_class, last_gateway_id, last_gateway_command, last_gateway_params);
        return 1;
    }

    // Test Case 2: No Params
    const char* dcs_seq2 = "\x1BPGATE;SYS;0;RESET\x1B\\";
    gateway_call_count = 0;
    memset(last_gateway_params, 0, sizeof(last_gateway_params));

    for (int i = 0; dcs_seq2[i] != '\0'; i++) {
        ProcessChar(term, (unsigned char)dcs_seq2[i]);
    }

    if (gateway_call_count == 1 &&
        strcmp(last_gateway_class, "SYS") == 0 &&
        strcmp(last_gateway_id, "0") == 0 &&
        strcmp(last_gateway_command, "RESET") == 0 &&
        strlen(last_gateway_params) == 0) {
        printf("PASS: No Params Command\n");
    } else {
        printf("FAIL: No Params Command\n");
        printf("Got Params: '%s'\n", last_gateway_params);
        return 1;
    }

    printf("All Gateway tests passed.\n");
    return 0;
}
