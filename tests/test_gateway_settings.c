#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static KTerm* term = NULL;

// Mock callbacks
static char last_gateway_class[64];
static char last_gateway_id[64];
static char last_gateway_command[64];
static char last_gateway_params[256];
static int gateway_call_count = 0;

static char last_response[4096];
static int response_call_count = 0;

void MockGatewayCallback(KTerm* term, const char* class_id, const char* id, const char* command, const char* params) {
    strncpy(last_gateway_class, class_id, sizeof(last_gateway_class) - 1);
    strncpy(last_gateway_id, id, sizeof(last_gateway_id) - 1);
    strncpy(last_gateway_command, command, sizeof(last_gateway_command) - 1);
    strncpy(last_gateway_params, params, sizeof(last_gateway_params) - 1);
    gateway_call_count++;
}

void MockResponseCallback(KTerm* term, const char* response, int length) {
    strncpy(last_response, response, sizeof(last_response) - 1);
    last_response[length] = '\0';
    response_call_count++;
}

int main() {
    KTermConfig config = {0};
    config.response_callback = MockResponseCallback;
    term = KTerm_Create(config);

    KTerm_SetGatewayCallback(term, MockGatewayCallback);

    printf("Testing Gateway Settings...\n");

    // 1. Test Internal "SET" Command
    // DCS GATE;KTERM;0;SET;LEVEL;525 ST
    const char* dcs_set = "\x1BPGATE;KTERM;0;SET;LEVEL;525\x1B\\";

    // Initial state
    KTerm_SetLevel(term, GET_SESSION(term), VT_LEVEL_100);
    assert(KTerm_GetLevel(term) == VT_LEVEL_100);
    gateway_call_count = 0;

    KTerm_WriteString(term, dcs_set);
    KTerm_ProcessEvents(term);

    // Verify: Callback NOT called, Level CHANGED
    if (gateway_call_count == 0) {
        printf("PASS: Callback bypassed for KTERM class\n");
    } else {
        printf("FAIL: Callback called for KTERM class (%d)\n", gateway_call_count);
        return 1;
    }

    if (KTerm_GetLevel(term) == VT_LEVEL_525) {
        printf("PASS: Level changed to 525\n");
    } else {
        printf("FAIL: Level not changed (Level=%d)\n", KTerm_GetLevel(term));
        return 1;
    }

    // 2. Test Internal "GET" Command (Externalizing)
    // DCS GATE;KTERM;0;GET;LEVEL ST
    const char* dcs_get = "\x1BPGATE;KTERM;0;GET;LEVEL\x1B\\";
    response_call_count = 0;
    memset(last_response, 0, sizeof(last_response));

    KTerm_WriteString(term, dcs_get);
    KTerm_ProcessEvents(term);
    KTerm_Update(term); // Flush response buffer

    if (response_call_count > 0) {
        // Expected response format: DCS GATE;KTERM;0;REPORT;LEVEL=525 ST
        // Response callback might receive chunks, but here likely one chunk
        printf("Response: %s\n", last_response);
        if (strstr(last_response, "REPORT;LEVEL=525") != NULL) {
            printf("PASS: GET LEVEL response correct\n");
        } else {
            printf("FAIL: Unexpected response\n");
            return 1;
        }
    } else {
        printf("FAIL: No response for GET command\n");
        return 1;
    }

// 2b. Test GET VERSION
const char* dcs_ver = "\x1BPGATE;KTERM;0;GET;VERSION\x1B\\";
response_call_count = 0;
memset(last_response, 0, sizeof(last_response));

KTerm_WriteString(term, dcs_ver);
KTerm_ProcessEvents(term);
KTerm_Update(term);

if (response_call_count > 0) {
    printf("Response: %s\n", last_response);
    if (strstr(last_response, "REPORT;VERSION=2.3.0") != NULL) {
        printf("PASS: GET VERSION response correct (2.3.0)\n");
    } else {
        printf("FAIL: Unexpected VERSION response: %s\n", last_response);
        return 1;
    }
} else {
    printf("FAIL: No response for GET VERSION\n");
    return 1;
}

    // 3. Test External Command (Pass-through)
    // DCS GATE;APP;1;DO;SOMETHING ST
    const char* dcs_ext = "\x1BPGATE;APP;1;DO;SOMETHING\x1B\\";
    gateway_call_count = 0;

    KTerm_WriteString(term, dcs_ext);
    KTerm_ProcessEvents(term);

    if (gateway_call_count == 1 && strcmp(last_gateway_class, "APP") == 0) {
        printf("PASS: External command passed to callback\n");
    } else {
        printf("FAIL: External command not passed correctly\n");
        return 1;
    }

    // 4. Test SET SIZE
    const char* dcs_size = "\x1BPGATE;KTERM;0;SET;SIZE;80;24\x1B\\";
    KTerm_WriteString(term, dcs_size);
    KTerm_ProcessEvents(term);

    if (term->width == 80 && term->height == 24) {
         printf("PASS: Size changed to 80x24\n");
    } else {
         printf("FAIL: Size change failed. Got %dx%d\n", term->width, term->height);
         return 1;
    }

    // 5. Test SET FONT
    const char* dcs_font = "\x1BPGATE;KTERM;0;SET;FONT;VCR\x1B\\"; // VCR is 12x14
    KTerm_WriteString(term, dcs_font);
    KTerm_ProcessEvents(term);

    if (term->char_width == 12 && term->char_height == 14) {
        printf("PASS: Font changed to VCR (12x14)\n");
    } else {
        printf("FAIL: Font change failed. Got %dx%d\n", term->char_width, term->char_height);
        return 1;
    }

    // 6. Test GET FONTS
    const char* dcs_fonts = "\x1BPGATE;KTERM;0;GET;FONTS\x1B\\";
    response_call_count = 0;
    memset(last_response, 0, sizeof(last_response));
    KTerm_WriteString(term, dcs_fonts);
    KTerm_ProcessEvents(term);
    KTerm_Update(term);

    if (strstr(last_response, "REPORT;FONTS=VT220,IBM,VGA,ULTIMATE") != NULL) {
         printf("PASS: GET FONTS response contains expected fonts\n");
    } else {
         printf("FAIL: Unexpected fonts response: %s\n", last_response);
         return 1;
    }

    printf("All Gateway Settings tests passed.\n");
    KTerm_Destroy(term);
    return 0;
}
