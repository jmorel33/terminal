#ifndef KT_LAYOUT_H
#define KT_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef enum {
    PANE_SPLIT_VERTICAL,   // Top/Bottom split
    PANE_SPLIT_HORIZONTAL, // Left/Right split
    PANE_LEAF              // Contains a session
} KTermPaneType;

typedef struct KTermPane_T KTermPane;

struct KTermPane_T {
    KTermPaneType type;
    KTermPane* parent;

    // For Splits
    KTermPane* child_a;
    KTermPane* child_b;
    float split_ratio; // 0.0 to 1.0 (relative size of child_a)

    // For Leaves
    int session_index; // -1 if empty

    // Geometry (Calculated)
    int x, y, width, height; // Cells
};

typedef struct {
    KTermPane* root;
    KTermPane* focused;
    int width;
    int height;
} KTermLayout;

// Callback for session resize events
// user_data is typically KTerm*
typedef void (*KTermLayout_ResizeCallback)(void* user_data, int session_index, int cols, int rows);

KTermLayout* KTermLayout_Create(int width, int height);
void KTermLayout_Destroy(KTermLayout* layout);
void KTermLayout_Resize(KTermLayout* layout, int width, int height, KTermLayout_ResizeCallback callback, void* user_data);
KTermPane* KTermLayout_Split(KTermLayout* layout, KTermPane* target, KTermPaneType type, float ratio, int new_session_index, KTermLayout_ResizeCallback callback, void* user_data);
void KTermLayout_Close(KTermLayout* layout, KTermPane* pane, KTermLayout_ResizeCallback callback, void* user_data);

// Helpers
void KTermLayout_SetRoot(KTermLayout* layout, KTermPane* root);
KTermPane* KTermLayout_GetRoot(KTermLayout* layout);

#ifdef __cplusplus
}
#endif

#endif // KT_LAYOUT_H

#ifdef KTERM_LAYOUT_IMPLEMENTATION

static void* KL_Calloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}

static void KL_Free(void* ptr) {
    free(ptr);
}

static void KTermLayout_Recalculate(KTermLayout* layout, KTermPane* pane, int x, int y, int w, int h, KTermLayout_ResizeCallback callback, void* user_data) {
    if (!pane) return;

    pane->x = x;
    pane->y = y;
    pane->width = w;
    pane->height = h;

    if (pane->type == PANE_LEAF) {
        if (pane->session_index >= 0 && callback) {
            callback(user_data, pane->session_index, w, h);
        }
    } else {
        int size_a, size_b;

        if (pane->type == PANE_SPLIT_HORIZONTAL) {
            // Split Horizontally (Left/Right)
            size_a = (int)(w * pane->split_ratio);
            size_b = w - size_a;

            // Recurse
            KTermLayout_Recalculate(layout, pane->child_a, x, y, size_a, h, callback, user_data);
            KTermLayout_Recalculate(layout, pane->child_b, x + size_a, y, size_b, h, callback, user_data);
        } else {
            // Split Vertically (Top/Bottom)
            size_a = (int)(h * pane->split_ratio);
            size_b = h - size_a;

            // Recurse
            KTermLayout_Recalculate(layout, pane->child_a, x, y, w, size_a, callback, user_data);
            KTermLayout_Recalculate(layout, pane->child_b, x, y + size_a, w, size_b, callback, user_data);
        }
    }
}

KTermLayout* KTermLayout_Create(int width, int height) {
    KTermLayout* layout = (KTermLayout*)KL_Calloc(1, sizeof(KTermLayout));
    if (!layout) return NULL;
    layout->width = width;
    layout->height = height;

    // Create default root pane
    layout->root = (KTermPane*)KL_Calloc(1, sizeof(KTermPane));
    if (!layout->root) {
        KL_Free(layout);
        return NULL;
    }
    layout->root->type = PANE_LEAF;
    layout->root->session_index = 0; // Default session
    layout->root->width = width;
    layout->root->height = height;

    layout->focused = layout->root;

    return layout;
}

static void KTermLayout_DestroyPane(KTermPane* pane) {
    if (!pane) return;
    KTermLayout_DestroyPane(pane->child_a);
    KTermLayout_DestroyPane(pane->child_b);
    KL_Free(pane);
}

void KTermLayout_Destroy(KTermLayout* layout) {
    if (!layout) return;
    if (layout->root) {
        KTermLayout_DestroyPane(layout->root);
    }
    KL_Free(layout);
}

void KTermLayout_Resize(KTermLayout* layout, int width, int height, KTermLayout_ResizeCallback callback, void* user_data) {
    if (!layout) return;
    layout->width = width;
    layout->height = height;
    if (layout->root) {
        KTermLayout_Recalculate(layout, layout->root, 0, 0, width, height, callback, user_data);
    }
}

KTermPane* KTermLayout_Split(KTermLayout* layout, KTermPane* target_pane, KTermPaneType split_type, float ratio, int new_session_index, KTermLayout_ResizeCallback callback, void* user_data) {
    if (!layout || !target_pane || target_pane->type != PANE_LEAF) return NULL;
    if (split_type == PANE_LEAF) return NULL;

    // Create new child panes
    KTermPane* child_a = (KTermPane*)KL_Calloc(1, sizeof(KTermPane));
    KTermPane* child_b = (KTermPane*)KL_Calloc(1, sizeof(KTermPane));

    if (!child_a || !child_b) {
        if (child_a) KL_Free(child_a);
        if (child_b) KL_Free(child_b);
        return NULL;
    }

    // Configure Child A (Existing content)
    child_a->type = PANE_LEAF;
    child_a->session_index = target_pane->session_index;
    child_a->parent = target_pane;

    // Configure Child B (New content)
    child_b->type = PANE_LEAF;
    child_b->session_index = new_session_index;
    child_b->parent = target_pane;

    // Convert Target Pane to Split
    target_pane->type = split_type;
    target_pane->child_a = child_a;
    target_pane->child_b = child_b;
    target_pane->split_ratio = ratio;
    target_pane->session_index = -1; // No longer a leaf

    // Trigger Layout Update
    if (layout->root) {
        KTermLayout_Recalculate(layout, layout->root, 0, 0, layout->width, layout->height, callback, user_data);
    }

    return child_b;
}

void KTermLayout_Close(KTermLayout* layout, KTermPane* pane, KTermLayout_ResizeCallback callback, void* user_data) {
    if (!layout || !pane || pane->type != PANE_LEAF) return;
    if (pane == layout->root) return; // Cannot close the last root pane

    KTermPane* parent = pane->parent;
    if (!parent) return;

    // Identify sibling
    KTermPane* sibling = (parent->child_a == pane) ? parent->child_b : parent->child_a;

    // Prune tree
    KTermPane* grandparent = parent->parent;
    if (grandparent) {
        if (grandparent->child_a == parent) grandparent->child_a = sibling;
        else grandparent->child_b = sibling;
        sibling->parent = grandparent;
    } else {
        // Parent was root, make sibling the new root
        layout->root = sibling;
        sibling->parent = NULL;
    }

    // Free resources
    KL_Free(pane);
    KL_Free(parent);

    // Update Layout
    KTermLayout_Recalculate(layout, layout->root, 0, 0, layout->width, layout->height, callback, user_data);

    // Update Focus (Target the sibling, or find a leaf in it)
    KTermPane* focus_target = sibling;
    while (focus_target && focus_target->type != PANE_LEAF) {
        focus_target = focus_target->child_a; // Default to first child
    }
    layout->focused = focus_target;
}

void KTermLayout_SetRoot(KTermLayout* layout, KTermPane* root) {
    if (layout) layout->root = root;
}

KTermPane* KTermLayout_GetRoot(KTermLayout* layout) {
    return layout ? layout->root : NULL;
}

#endif // KTERM_LAYOUT_IMPLEMENTATION
