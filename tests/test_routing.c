#include <stdio.h>
#include <string.h>

#define KTERM_IMPLEMENTATION
#define SITUATION_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "../kterm.h"
#include "mock_situation.h" // Assuming mock situation is available for testing

void test_gateway_routing() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 24;
    KTerm* term = KTerm_Create(config);

    if (!term) {
        printf("Failed to create KTerm\n");
        return;
    }

    // Initialize 2 sessions
    KTerm_InitSession(term, 1);
    term->sessions[1].session_open = true;

    KTermSession* session0 = &term->sessions[0];
    KTermSession* session1 = &term->sessions[1];

    printf("Testing ReGIS Routing...\n");
    // Send Gateway Command to route ReGIS to Session 1
    // Command: DCS GATE KTERM;0;SET;REGIS_SESSION;1 ST
    const char* gw_cmd = "\x1BPGATE;KTERM;0;SET;REGIS_SESSION;1\x1B\\";
    for(int i=0; gw_cmd[i]; i++) KTerm_ProcessChar(term, session0, gw_cmd[i]);

    if (term->regis_target_session == 1) {
        printf("PASS: ReGIS target set to 1\n");
    } else {
        printf("FAIL: ReGIS target is %d, expected 1\n", term->regis_target_session);
    }

    // Reset ReGIS Routing
    const char* gw_reset = "\x1BPGATE;KTERM;0;RESET;REGIS_SESSION\x1B\\";
    for(int i=0; gw_reset[i]; i++) KTerm_ProcessChar(term, session0, gw_reset[i]);

    if (term->regis_target_session == -1) {
        printf("PASS: ReGIS target reset to -1\n");
    } else {
        printf("FAIL: ReGIS target is %d, expected -1\n", term->regis_target_session);
    }

    // Init ReGIS Routing (Set to current session 0)
    const char* gw_init = "\x1BPGATE;KTERM;0;INIT;REGIS_SESSION\x1B\\";
    for(int i=0; gw_init[i]; i++) KTerm_ProcessChar(term, session0, gw_init[i]);

    if (term->regis_target_session == 0) {
        printf("PASS: ReGIS target init to 0\n");
    } else {
        printf("FAIL: ReGIS target is %d, expected 0\n", term->regis_target_session);
    }

    // Test Kitty Routing
    printf("Testing Kitty Routing...\n");
    const char* kitty_set = "\x1BPGATE;KTERM;0;SET;KITTY_SESSION;1\x1B\\";
    for(int i=0; kitty_set[i]; i++) KTerm_ProcessChar(term, session0, kitty_set[i]);

    if (term->kitty_target_session == 1) {
        printf("PASS: Kitty target set to 1\n");
    } else {
         printf("FAIL: Kitty target is %d, expected 1\n", term->kitty_target_session);
    }

    KTerm_Destroy(term);
}

int main() {
    test_gateway_routing();
    return 0;
}
