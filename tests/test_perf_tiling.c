#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include "mock_situation.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void feed_session(KTerm* term, int session_idx, const char* data) {
    int saved = term->active_session;
    term->active_session = session_idx;
    KTerm_WriteString(term, data);
    term->active_session = saved;
}

int main() {
    printf("Starting Performance Profiling (Tiling 4 Panes)...\n");

    KTermConfig config = {0};
    config.width = 200;
    config.height = 100;
    KTerm* term = KTerm_Create(config);
    KTerm_Init(term);

    // Setup 4 Panes (Quad Split)
    // Root is Leaf 0.
    // Split Root (H) -> Top (0), Bot (New 1)
    KTermPane* root = term->layout->root;
    KTermPane* bot = KTerm_SplitPane(term, root, PANE_SPLIT_VERTICAL, 0.5f);
    int s1_idx = bot->session_index;

    // Split Top (V) -> TL (0), TR (New 2)
    KTermPane* top = root->child_a; // Now root is split, child_a is the top pane
    KTermPane* tr = KTerm_SplitPane(term, top, PANE_SPLIT_HORIZONTAL, 0.5f);
    int s2_idx = tr->session_index;

    // Split Bot (V) -> BL (1), BR (New 3)
    // bot is the leaf for session 1. But wait, root was split.
    // KTerm_SplitPane returns the NEW pane (child_b).
    // root->child_b is the bottom pane (bot).
    // We want to split root->child_b.
    // Note: KTerm_SplitPane modifies the passed pane to be a split node.
    // So 'bot' pointer is still valid but now points to the Split node?
    // No, `target_pane` is converted.
    // Let's re-traverse to be safe or use returned pointers.
    // root is split. child_a is top (was 0). child_b is bot (was 1).
    // We split child_a. child_a is now split. child_a->child_a is 0. child_a->child_b is 2.
    // We split child_b.
    KTermPane* br = KTerm_SplitPane(term, root->child_b, PANE_SPLIT_HORIZONTAL, 0.5f);
    int s3_idx = br->session_index;

    printf("Sessions: 0, %d, %d, %d\n", s1_idx, s2_idx, s3_idx);

    // Feed massive data to all 4 sessions
    const char* text_block = "The quick brown fox jumps over the lazy dog. \x1B[31mRed\x1B[0m \x1B[1mBold\x1B[0m\n";
    // Approx 64 bytes.
    // Target: Measure FPS drop. We want > 60 FPS.
    // 1 Frame = Update + Draw.
    // Update consumes pipeline.
    // We'll fill pipeline, run Update/Draw, refill.

    int frames = 600; // 10 seconds at 60 FPS
    clock_t start = clock();

    for (int f = 0; f < frames; f++) {
        // Feed 1KB to each session per frame (simulating heavy `cat`)
        for (int i = 0; i < 16; i++) {
            feed_session(term, 0, text_block);
            feed_session(term, s1_idx, text_block);
            feed_session(term, s2_idx, text_block);
            feed_session(term, s3_idx, text_block);
        }

        KTerm_Update(term);
        // KTerm_Draw is mocked but logic runs (SSBO update etc)
    }

    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    double fps = frames / cpu_time_used;

    printf("Processed %d frames in %.2f seconds.\n", frames, cpu_time_used);
    printf("Average FPS: %.2f\n", fps);

    if (fps < 60.0) {
        printf("WARNING: FPS below 60!\n");
        // Don't fail the test in CI environment as it might be slow, but warn.
        // exit(1);
    } else {
        printf("PASS: Performance is acceptable.\n");
    }

    KTerm_Destroy(term);
    return 0;
}
