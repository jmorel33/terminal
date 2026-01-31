#define KTERM_TESTING
#define KTERM_IMPLEMENTATION

// Required dependencies
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Include KTerm
// Assuming we compile with -I.. to find kterm.h
#include "../kterm.h"

// Callback for terminal responses
void FuzzResponseCallback(KTerm* term, const char* data, int len) {
    // Drop response
}

int main(int argc, char** argv) {
    // Basic fuzzing harness reading from stdin

    // 1. Initialize Mock Environment (handled by KTERM_TESTING / mock_situation.h)

    // 2. Create Terminal
    KTermConfig config = {
        .width = 80,
        .height = 24,
        .response_callback = FuzzResponseCallback
    };

    KTerm* term = KTerm_Create(config);
    if (!term) {
        fprintf(stderr, "Failed to create terminal\n");
        return 1;
    }

    // 3. Consume Input
    unsigned char buffer[4096];
    ssize_t bytes_read;

    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        // Process chunk
        for (ssize_t i = 0; i < bytes_read; i++) {
            KTerm_WriteChar(term, buffer[i]);
        }

        // Process the buffered input
        KTerm_Update(term);
    }

    // 4. Cleanup
    KTerm_Destroy(term);
    return 0;
}
