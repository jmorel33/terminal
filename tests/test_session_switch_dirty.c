#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

void mock_response(KTerm* term, const char* response, int length) {
    (void)term; (void)response; (void)length;
}

int main() {
    printf("Testing Session Switch Dirty Flag...\n");

    KTermConfig config = {0};
    config.width = 100;
    config.height = 50;
    config.response_callback = mock_response;

    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    // Manually clear the flag to start from a known state.
    term->font_atlas_dirty = false;

    printf("Initial active session: %d\n", term->active_session);
    assert(term->active_session == 0);

    // Switch to session 1
    printf("Switching to session 1...\n");
    KTerm_SetActiveSession(term, 1);

    // Verify
    assert(term->active_session == 1);
    if (term->font_atlas_dirty) {
        printf("PASS: font_atlas_dirty set to true.\n");
    } else {
        printf("FAIL: font_atlas_dirty NOT set.\n");
        return 1;
    }

    // Reset flag
    term->font_atlas_dirty = false;

    // Switch back to 0
    printf("Switching back to session 0...\n");
    KTerm_SetActiveSession(term, 0);
    assert(term->active_session == 0);
    if (term->font_atlas_dirty) {
        printf("PASS: font_atlas_dirty set to true.\n");
    } else {
        printf("FAIL: font_atlas_dirty NOT set.\n");
        return 1;
    }

    // Switch to same session (should not set dirty if optimized, but current impl checks equality first)
    term->font_atlas_dirty = false;
    KTerm_SetActiveSession(term, 0);
    if (!term->font_atlas_dirty) {
        printf("PASS: font_atlas_dirty NOT set when switching to same session.\n");
    } else {
        printf("WARN: font_atlas_dirty set even when session didn't change (acceptable but not optimal).\n");
    }

    KTerm_Destroy(term);
    return 0;
}
