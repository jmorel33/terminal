#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include "mock_situation.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

// Use standard headers for the test logic regardless of KTerm internals
#include <pthread.h>

volatile bool running = true;

void* producer_thread(void* arg) {
    KTerm* term = (KTerm*)arg;
    unsigned char counter = 0;
    while (running) {
        for (int i = 0; i < 50; i++) {
            // Write alphanumeric to avoid triggering control char bugs (like ENQ)
            KTerm_WriteChar(term, 'A' + ((counter++) % 26));
        }
        usleep(100);
    }
    return NULL;
}

int main() {
    setbuf(stdout, NULL);
    printf("Testing Thread Safety (Lock-Free Pipeline)...\n");

    KTermConfig config = {0};
    KTerm* term = KTerm_Create(config);
    if (!term) {
        printf("Failed to create KTerm\n");
        return 1;
    }

    if (!KTerm_Init(term)) {
        printf("Failed to init KTerm\n");
        return 1;
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, producer_thread, term) != 0) {
        printf("Failed to create thread\n");
        return 1;
    }

    for (int i = 0; i < 200; i++) {
        KTerm_Update(term);
        KTermStatus status = KTerm_GetStatus(term);
        (void)status;
        usleep(1000);
    }

    running = false;
    pthread_join(thread, NULL);

    KTermStatus final_status = KTerm_GetStatus(term);
    printf("Final Pipeline Usage: %zu\n", final_status.pipeline_usage);

    KTerm_Destroy(term);
    printf("PASS: Threading test completed without crash.\n");
    return 0;
}
