#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Helper to drain pipeline and check content
void VerifyPipelineContent(KTerm* term, const char* expected) {
    KTermSession* session = GET_SESSION(term);

    // We can't easily read the ring buffer without modifying head/tail or calculating indices.
    // But since we are in a single threaded test and we just wrote to it (and haven't processed it yet),
    // we can assume head has moved and tail is stationary (if we didn't call ProcessEvents).
    // KTerm_WriteChar appends to head.

    int head = atomic_load_explicit(&session->pipeline_head, memory_order_relaxed);
    int tail = atomic_load_explicit(&session->pipeline_tail, memory_order_relaxed);

    int count = (head - tail + sizeof(session->input_pipeline)) % sizeof(session->input_pipeline);

    if (count != strlen(expected)) {
        printf("FAIL: Pipeline count mismatch. Expected %lu, Got %d\n", strlen(expected), count);
        return;
    }

    char buffer[1024];
    for (int i = 0; i < count; i++) {
        buffer[i] = session->input_pipeline[(tail + i) % sizeof(session->input_pipeline)];
    }
    buffer[count] = '\0';

    if (strcmp(buffer, expected) == 0) {
        printf("PASS: Pipeline content matches '%s'\n", expected);
    } else {
        printf("FAIL: Pipeline content mismatch. Expected '%s', Got '%s'\n", expected, buffer);
    }

    // Clear the pipeline for next test
    atomic_store(&session->pipeline_tail, head);
}

int main() {
    printf("Testing VT Pipe via Gateway Protocol...\n");

    KTermConfig config = {0};
    KTerm* term = KTerm_Create(config);
    if (!term) {
        printf("Failed to create terminal\n");
        return 1;
    }

    // Ensure pipeline is empty
    KTermSession* session = GET_SESSION(term);
    atomic_store(&session->pipeline_tail, atomic_load(&session->pipeline_head));

    // ---------------------------------------------------------
    // Test Case 1: Raw Text Injection
    // Command: DCS GATE KTERM;0;PIPE;VT;RAW;Hello ST
    // ---------------------------------------------------------
    printf("\nTest 1: RAW Injection\n");
    const char* raw_seq = "\x1BPGATE;KTERM;0;PIPE;VT;RAW;Hello\x1B\\";
    for (int i = 0; raw_seq[i] != '\0'; i++) {
        KTerm_ProcessChar(term, session, (unsigned char)raw_seq[i]);
    }
    VerifyPipelineContent(term, "Hello");

    // ---------------------------------------------------------
    // Test Case 2: Hex Injection
    // Command: DCS GATE KTERM;0;PIPE;VT;HEX;414243 ST (ABC)
    // ---------------------------------------------------------
    printf("\nTest 2: HEX Injection\n");
    const char* hex_seq = "\x1BPGATE;KTERM;0;PIPE;VT;HEX;414243\x1B\\";
    for (int i = 0; hex_seq[i] != '\0'; i++) {
        KTerm_ProcessChar(term, session, (unsigned char)hex_seq[i]);
    }
    VerifyPipelineContent(term, "ABC");

    // ---------------------------------------------------------
    // Test Case 3: Base64 Injection
    // Command: DCS GATE KTERM;0;PIPE;VT;B64;SGVsbG8= ST (Hello)
    // ---------------------------------------------------------
    printf("\nTest 3: B64 Injection\n");
    const char* b64_seq = "\x1BPGATE;KTERM;0;PIPE;VT;B64;SGVsbG8=\x1B\\";
    for (int i = 0; b64_seq[i] != '\0'; i++) {
        KTerm_ProcessChar(term, session, (unsigned char)b64_seq[i]);
    }
    VerifyPipelineContent(term, "Hello");

    // ---------------------------------------------------------
    // Test Case 4: Base64 Injection with Escape Sequence
    // Command: DCS GATE KTERM;0;PIPE;VT;B64;G1szMW0= ST (ESC [ 3 1 m)
    // ---------------------------------------------------------
    printf("\nTest 4: B64 Injection (Escape Sequence)\n");
    const char* b64_esc_seq = "\x1BPGATE;KTERM;0;PIPE;VT;B64;G1szMW0=\x1B\\";
    for (int i = 0; b64_esc_seq[i] != '\0'; i++) {
        KTerm_ProcessChar(term, session, (unsigned char)b64_esc_seq[i]);
    }
    VerifyPipelineContent(term, "\x1B[31m");

    KTerm_Destroy(term);
    return 0;
}
