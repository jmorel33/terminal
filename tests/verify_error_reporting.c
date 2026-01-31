#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Mock callback
static bool callback_invoked = false;
static KTermErrorLevel last_level = (KTermErrorLevel)-1;
static KTermErrorSource last_source = (KTermErrorSource)-1;
static char last_message[1024] = {0};

void MyErrorCallback(KTerm* term, KTermErrorLevel level, KTermErrorSource source, const char* msg, void* user_data) {
    (void)term;
    (void)user_data;
    callback_invoked = true;
    last_level = level;
    last_source = source;
    strncpy(last_message, msg, sizeof(last_message) - 1);
    printf("Callback captured: [Level=%d Source=%d] %s\n", level, source, msg);
}

int main() {
    KTermConfig config = {0};
    // Note: KTerm_Create usually requires SITUATION backend.
    // Since we are in KTERM_TESTING mode, we use mock_situation.h (implicitly included via kterm.h if setup correctly).
    // Assuming mock implementation allows basic init.

    KTerm* term = KTerm_Create(config);
    if (!term) {
        fprintf(stderr, "Failed to create KTerm\n");
        return 1;
    }

    // 1. Test Setting Callback
    KTerm_SetErrorCallback(term, MyErrorCallback, NULL);

    // 2. Trigger Error (Load non-existent font)
    // KTerm_LoadFont calls KTerm_LoadFileData which is mocked or implemented.
    // If mocked, it likely fails for unknown files.
    printf("Triggering font load error...\n");
    KTerm_LoadFont(term, "non_existent_font.ttf");

    if (!callback_invoked) {
        fprintf(stderr, "Error: Callback was NOT invoked for font failure.\n");
        return 1;
    }
    if (last_level != KTERM_LOG_ERROR) {
        fprintf(stderr, "Error: Unexpected level %d (expected %d)\n", last_level, KTERM_LOG_ERROR);
        return 1;
    }
    if (last_source != KTERM_SOURCE_SYSTEM) {
        fprintf(stderr, "Error: Unexpected source %d (expected %d)\n", last_source, KTERM_SOURCE_SYSTEM);
        return 1;
    }
    if (strstr(last_message, "Failed to load font file") == NULL) {
        fprintf(stderr, "Error: Unexpected message '%s'\n", last_message);
        return 1;
    }

    // Reset
    callback_invoked = false;

    // 3. Trigger ReportError directly (Simulate Parser Error)
    printf("Triggering manual parser warning...\n");
    KTerm_ReportError(term, KTERM_LOG_WARNING, KTERM_SOURCE_PARSER, "Test Parser Warning %d", 42);

    if (!callback_invoked) {
        fprintf(stderr, "Error: Callback was NOT invoked for manual report.\n");
        return 1;
    }
    if (last_level != KTERM_LOG_WARNING) {
        fprintf(stderr, "Error: Unexpected level %d (expected %d)\n", last_level, KTERM_LOG_WARNING);
        return 1;
    }
    if (strstr(last_message, "Test Parser Warning 42") == NULL) {
        fprintf(stderr, "Error: Unexpected message '%s'\n", last_message);
        return 1;
    }

    printf("Verification SUCCESS!\n");
    KTerm_Destroy(term);
    return 0;
}
