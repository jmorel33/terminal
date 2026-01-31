#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#define KTERM_IMPLEMENTATION
#define SITUATION_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"

// Sink Context
typedef struct {
    char buffer[1024];
    size_t length;
} SinkContext;

// Sink Callback
void MySink(void* ctx, const char* data, size_t len) {
    SinkContext* sc = (SinkContext*)ctx;
    if (sc->length + len < sizeof(sc->buffer)) {
        memcpy(sc->buffer + sc->length, data, len);
        sc->length += len;
        // Don't enforce null termination here for binary test verification
    }
}

void test_sink_output() {
    printf("Testing Sink Output...\n");
    KTermConfig config = {0};
    KTerm* term = KTerm_Create(config);

    // 1. Test Legacy Mode (Buffering)
    KTerm_QueueResponse(term, "Hello");
    KTermSession* session = &term->sessions[term->active_session];

    // Verify buffer
    if (session->response_length == 5 && strncmp(session->answerback_buffer, "Hello", 5) == 0) {
        printf("PASS: Legacy buffering worked.\n");
    } else {
        printf("FAIL: Legacy buffering failed. Len: %d, Content: %s\n", session->response_length, session->answerback_buffer);
    }

    // 2. Test Flush on SetOutputSink
    SinkContext sc = {0};
    KTerm_SetOutputSink(term, MySink, &sc);

    // Verify buffer flushed to sink
    if (session->response_length == 0) {
        if (sc.length == 5 && strncmp(sc.buffer, "Hello", 5) == 0) {
            printf("PASS: Flush to Sink worked.\n");
        } else {
            printf("FAIL: Flush content mismatch. Got: '%.*s'\n", (int)sc.length, sc.buffer);
        }
    } else {
        printf("FAIL: Buffer not cleared after SetOutputSink.\n");
    }

    // 3. Test Direct Sink Output
    sc.length = 0;

    KTerm_QueueResponse(term, "World");

    if (session->response_length == 0) {
        if (sc.length == 5 && strncmp(sc.buffer, "World", 5) == 0) {
            printf("PASS: Direct Sink output worked.\n");
        } else {
            printf("FAIL: Direct Sink output mismatch. Got: '%.*s'\n", (int)sc.length, sc.buffer);
        }
    } else {
        printf("FAIL: Legacy buffer used during Sink mode.\n");
    }

    KTerm_Destroy(term);
}

void test_binary_safety() {
    printf("Testing Binary Safety...\n");
    KTermConfig config = {0};
    KTerm* term = KTerm_Create(config);
    KTermSession* session = &term->sessions[term->active_session];

    // Case 1: Binary data exactly filling buffer (minus 1, or full?)
    // KTERM_OUTPUT_PIPELINE_SIZE is usually large (16384).
    // Let's use a small buffer if we could mock it, but we can't easily change define.
    // We will simulate near-full buffer logic by artificially increasing response_length.

    // Fill buffer up to capacity-1
    size_t capacity = sizeof(session->answerback_buffer);
    session->response_length = capacity - 1;
    // Set last byte to known value
    session->answerback_buffer[capacity - 1] = 0xAA;

    // Write 1 byte of binary data
    char bin_data[] = {0xFF};
    KTerm_QueueResponseBytes(term, bin_data, 1);

    // Should fill the last byte (index capacity-1)
    if (session->answerback_buffer[capacity - 1] == (char)0xFF) {
        printf("PASS: Binary data written to end of buffer.\n");
    } else {
        printf("FAIL: Binary data corrupted. Expected 0xFF, got 0x%02X\n", (unsigned char)session->answerback_buffer[capacity - 1]);
    }

    // Since it's binary, it should NOT have written a null terminator at capacity (out of bounds)
    // or overwritten the data if we passed `is_binary=true`.
    // Wait, implementation:
    // if (!is_binary) buffer[length] = \0.
    // If is_binary, we don't write \0.
    // So 0xFF should persist.

    // Case 2: Buffer overflow handling
    session->response_length = capacity;
    char overflow[] = "overflow";
    // This should trigger truncation/drop because response_callback is NULL (default).
    // KTerm_QueueResponseBytes
    KTerm_QueueResponseBytes(term, overflow, 8);

    if (session->response_length == capacity) {
        printf("PASS: Overflow data correctly dropped/truncated.\n");
    } else {
        printf("FAIL: Buffer overflow occurred or length mismatch. Len: %zu\n", session->response_length);
    }

    KTerm_Destroy(term);
}

int main() {
    test_sink_output();
    test_binary_safety();
    return 0;
}
