#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static KTerm* term = NULL;

void MockGatewayCallback(KTerm* term, const char* class_id, const char* id, const char* command, const char* params) {
    (void)term; (void)class_id; (void)id; (void)command; (void)params;
    // Do nothing, just ensure we don't crash
}

void InjectDCS(KTerm* term, const char* dcs_seq) {
    for (int i = 0; dcs_seq[i] != '\0'; i++) {
        KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)dcs_seq[i]);
    }
}

int main() {
    KTermConfig config = {0};
    term = KTerm_Create(config);
    KTerm_SetGatewayCallback(term, MockGatewayCallback);

    printf("Testing Gateway Hardening...\n");

    // 1. Test Buffer Overflow in KTerm_GenerateBanner (via PIPE;BANNER)
    // We construct a very long parameter string
    // "PIPE;BANNER;TEXT=..."
    printf("Test 1: Long Banner Text\n");
    char huge_banner[20000];
    snprintf(huge_banner, sizeof(huge_banner), "\x1BPGATE;KTERM;1;PIPE;BANNER;TEXT=");
    int prefix_len = strlen(huge_banner);
    for(int i = 0; i < 16000; i++) {
        huge_banner[prefix_len + i] = 'A';
    }
    huge_banner[prefix_len + 16000] = '\0';
    strcat(huge_banner, "\x1B\\");

    InjectDCS(term, huge_banner);
    printf("PASS: Long Banner Text (Did not crash)\n");

    // 2. Test deeply nested or malformed Banner params
    printf("Test 2: Malformed Banner Params\n");
    const char* bad_banner = "\x1BPGATE;KTERM;1;PIPE;BANNER;GRADIENT=255,0,0|0,0,255;ALIGN=CENTER;FONT=UNKNOWN\x1B\\";
    InjectDCS(term, bad_banner);
    printf("PASS: Malformed Banner Params\n");

    // 3. Test Long Attribute String
    printf("Test 3: Long Attribute String\n");
    char huge_attr[2048];
    snprintf(huge_attr, sizeof(huge_attr), "\x1BPGATE;KTERM;1;SET;ATTR;");
    prefix_len = strlen(huge_attr);
    for(int i = 0; i < 1000; i++) {
        // Repeatedly add "BOLD=1;"
        strcat(huge_attr, "BOLD=1;");
    }
    strcat(huge_attr, "\x1B\\");
    InjectDCS(term, huge_attr);
    printf("PASS: Long Attribute String\n");

    // 4. Test Invalid Numbers (atoi hardening)
    printf("Test 4: Invalid Numbers\n");
    InjectDCS(term, "\x1BPGATE;KTERM;1;SET;WIDTH;INVALID\x1B\\");
    InjectDCS(term, "\x1BPGATE;KTERM;1;SET;HEIGHT;-100\x1B\\");
    InjectDCS(term, "\x1BPGATE;KTERM;1;SET;SESSION;99999\x1B\\"); // OOB Session
    printf("PASS: Invalid Numbers\n");

    // 5. Test PIPE decoding with invalid Base64
    printf("Test 5: Invalid Base64\n");
    InjectDCS(term, "\x1BPGATE;KTERM;1;PIPE;VT;B64;!!@@##\x1B\\");
    printf("PASS: Invalid Base64\n");

    // 6. Test NULL pointer guards (Direct API call if possible, but here we test via parser)
    // We can call KTerm_GatewayProcess directly with NULLs to test API hardening
    printf("Test 6: Direct API NULL checks\n");
    KTerm_GatewayProcess(term, GET_SESSION(term), NULL, "ID", "CMD", "PARAMS");
    KTerm_GatewayProcess(term, GET_SESSION(term), "CLASS", NULL, "CMD", "PARAMS");
    KTerm_GatewayProcess(term, GET_SESSION(term), "CLASS", "ID", NULL, "PARAMS");
    KTerm_GatewayProcess(term, GET_SESSION(term), "CLASS", "ID", "CMD", NULL);
    printf("PASS: Direct API NULL checks\n");

    KTerm_Destroy(term);
    printf("All Hardening tests passed.\n");
    return 0;
}
