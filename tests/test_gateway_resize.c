#define KTERM_IMPLEMENTATION
#define KTERM_ENABLE_GATEWAY
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <assert.h>

int main() {
    KTermConfig config = {0};
    config.width = 80;
    config.height = 24;
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    printf("Testing Gateway Resize...\n");

    // 1. Test SET;WIDTH
    // DCS GATE KTERM;1;SET;WIDTH;200 ST
    const char* seq1 = "\x1BPGATE;KTERM;1;SET;WIDTH;200\x1B\\";
    for(int i=0; seq1[i]; i++) KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)seq1[i]);

    if (term->width == 200 && term->height == 24) {
        printf("PASS: SET;WIDTH;200 -> %d x %d\n", term->width, term->height);
    } else {
        printf("FAIL: SET;WIDTH;200 -> %d x %d\n", term->width, term->height);
        return 1;
    }

    // 2. Test SET;HEIGHT
    const char* seq2 = "\x1BPGATE;KTERM;1;SET;HEIGHT;100\x1B\\";
    for(int i=0; seq2[i]; i++) KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)seq2[i]);

    if (term->width == 200 && term->height == 100) {
        printf("PASS: SET;HEIGHT;100 -> %d x %d\n", term->width, term->height);
    } else {
        printf("FAIL: SET;HEIGHT;100 -> %d x %d\n", term->width, term->height);
        return 1;
    }

    // 3. Test SET;SIZE
    const char* seq3 = "\x1BPGATE;KTERM;1;SET;SIZE;150;150\x1B\\";
    for(int i=0; seq3[i]; i++) KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)seq3[i]);

    if (term->width == 150 && term->height == 150) {
        printf("PASS: SET;SIZE;150;150 -> %d x %d\n", term->width, term->height);
    } else {
        printf("FAIL: SET;SIZE;150;150 -> %d x %d\n", term->width, term->height);
        return 1;
    }

    // 4. Test Clamping Width
    const char* seq4 = "\x1BPGATE;KTERM;1;SET;WIDTH;3000\x1B\\";
    for(int i=0; seq4[i]; i++) KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)seq4[i]);

    if (term->width == 2048) {
        printf("PASS: SET;WIDTH;3000 -> Clamped to %d\n", term->width);
    } else {
        printf("FAIL: SET;WIDTH;3000 -> %d (Expected 2048)\n", term->width);
        return 1;
    }

    // 5. Test Clamping Height
    const char* seq5 = "\x1BPGATE;KTERM;1;SET;HEIGHT;3000\x1B\\";
    for(int i=0; seq5[i]; i++) KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)seq5[i]);

    if (term->height == 2048) {
        printf("PASS: SET;HEIGHT;3000 -> Clamped to %d\n", term->height);
    } else {
        printf("FAIL: SET;HEIGHT;3000 -> %d (Expected 2048)\n", term->height);
        return 1;
    }

    // 6. Test SIZE Clamping
    const char* seq6 = "\x1BPGATE;KTERM;1;SET;SIZE;4000;4000\x1B\\";
    for(int i=0; seq6[i]; i++) KTerm_ProcessChar(term, GET_SESSION(term), (unsigned char)seq6[i]);

    if (term->width == 2048 && term->height == 2048) {
        printf("PASS: SET;SIZE;4000;4000 -> Clamped to %d x %d\n", term->width, term->height);
    } else {
        printf("FAIL: SET;SIZE;4000;4000 -> %d x %d\n", term->width, term->height);
        return 1;
    }

    KTerm_Destroy(term);
    printf("All resize tests passed.\n");
    return 0;
}
