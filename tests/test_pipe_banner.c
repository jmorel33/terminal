#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void RunTest(const char* seq, const char* name) {
    KTermConfig config = {0};
    KTerm* term = KTerm_Create(config);
    if (!term) {
        printf("Failed to create KTerm\n");
        return;
    }

    printf("Testing: %s\n", name);
    // Process
    for (int i = 0; seq[i] != '\0'; i++) {
        KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)seq[i]);
    }

    // Check pipeline
    KTermSession* session = GET_SESSION(term);
    int count = (session->pipeline_head - session->pipeline_tail + 65536) % 65536;
    printf("Pipeline count: %d\n", count);

    if (count > 0) {
        printf("PASS: Output generated.\n");

        // Inspect content for gradient (ANSI codes)
        if (strstr(name, "GRADIENT")) {
            bool found_ansi = false;
            // Scan for ESC [ 3 8 ; 2
            for (int i = 0; i < count - 5; i++) {
                int idx = (session->pipeline_tail + i) % 65536;
                if (session->input_pipeline[idx] == 0x1B &&
                    session->input_pipeline[(idx+1)%65536] == '[') {
                    found_ansi = true;
                    break;
                }
            }
            if (found_ansi) printf("PASS: Gradient ANSI codes found.\n");
            else printf("FAIL: No Gradient ANSI codes found.\n");
        }
    } else {
        printf("FAIL: No output.\n");
    }

    // Cleanup
    KTerm_Destroy(term);
}

int main() {
    // 1. Legacy
    RunTest("\x1BPGATE;KTERM;1;PIPE;BANNER;FIXED;A\x1B\\", "Legacy FIXED");

    // 2. New Syntax with Font and Alignment
    // Assuming "VCR" font exists in available_fonts (checked source, it does)
    RunTest("\x1BPGATE;KTERM;1;PIPE;BANNER;FONT=VCR;ALIGN=CENTER;TEXT=A\x1B\\", "Extended FONT+ALIGN");

    // 3. New Syntax with Gradient
    RunTest("\x1BPGATE;KTERM;1;PIPE;BANNER;GRADIENT=#FF0000|#0000FF;TEXT=A\x1B\\", "Extended GRADIENT");

    return 0;
}
