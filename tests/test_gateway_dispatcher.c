#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Mock validation
void test_dispatcher_basic(KTerm* term) {
    KTermSession* s = GET_SESSION(term);

    // SET;SESSION;1
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "SESSION;1");
    assert(term->gateway_target_session == 1);
    printf("PASS: SET SESSION\n");

    // RESET;SESSION
    KTerm_GatewayProcess(term, s, "KTERM", "0", "RESET", "SESSION");
    assert(term->gateway_target_session == -1);
    printf("PASS: RESET SESSION\n");

    // SET;WIDTH;100
    term->last_resize_time = -1.0; // Bypass throttle
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "WIDTH;100");
    assert(term->width == 100);
    printf("PASS: SET WIDTH\n");

    // SET;SIZE;80;25
    term->last_resize_time = -1.0; // Bypass throttle
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "SIZE;80;25");
    assert(term->width == 80);
    assert(term->height == 25);
    printf("PASS: SET SIZE\n");

    // SET;DEBUG;ON
    KTerm_EnableDebug(term, false);
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "DEBUG;ON");
    assert(s->status.debugging == true);

    KTerm_EnableDebug(term, false);
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "DEBUG;TRUE");
    assert(s->status.debugging == true);
    printf("PASS: SET DEBUG (Bool)\n");
}

void test_dispatcher_edge_cases(KTerm* term) {
    KTermSession* s = GET_SESSION(term);

    // Extra spaces
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "  SESSION  ;  1  ");
    assert(term->gateway_target_session == 1);
    printf("PASS: Extra Spaces\n");

    // Case sensitivity for command (Strict)
    term->gateway_target_session = -1;
    KTerm_GatewayProcess(term, s, "KTERM", "0", "set", "SESSION;1");
    assert(term->gateway_target_session == -1); // Should fail/callback
    printf("PASS: Case Sensitivity (Strict)\n");

    // Params robustness (My refactor preserves case sensitivity for subcommands)
    term->gateway_target_session = -1;
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "session;1");
    assert(term->gateway_target_session == -1); // session != SESSION
    printf("PASS: Params Case Sensitivity (Strict)\n");

    // Verify robustness against garbage
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "INVALID;123");
    // Should do nothing / not crash.
    printf("PASS: Garbage Command\n");

    // Malformed Params
    // SET;SIZE;ABC;DEF
    int w = term->width;
    int h = term->height;
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "SIZE;ABC;DEF");
    assert(term->width == w); // Should not change
    assert(term->height == h);
    printf("PASS: Malformed Params\n");

    // Empty Params
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", "");
    printf("PASS: Empty Params\n");

    // NULL Params
    KTerm_GatewayProcess(term, s, "KTERM", "0", "SET", NULL);
    printf("PASS: NULL Params\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80; config.height = 25;
    KTerm* term = KTerm_Create(config);

    printf("Testing Gateway Dispatcher...\n");
    test_dispatcher_basic(term);
    test_dispatcher_edge_cases(term);

    KTerm_Destroy(term);
    printf("All Dispatcher tests passed.\n");
    return 0;
}
