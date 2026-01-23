# KTerm Tab Stop Implementation Plan

## 1. Executive Summary

This document outlines the plan to ensure "Complete Tab Stop Support" in `KTerm`. While the current codebase implements the core mechanisms (HT, HTS, TBC, CBT), strict VT/DEC compliance requires verifying edge cases, handling resizing robustly, and ensuring margin interactions are correct.

The goal is a non-breaking, fully compliant implementation that mimics hardware terminal behavior (e.g., VT520).

## 2. Current State Analysis

Based on codebase analysis (`kterm.h`):

*   **Data Structure**: `TabStops` uses a fixed boolean array `stops[MAX_TAB_STOPS]` (Size 256).
*   **Control Handling**:
    *   `HT` (0x09) appears to call `NextTabStop`.
    *   `HTS` (ESC H) is implemented.
    *   `TBC` (CSI g) is implemented.
    *   `CBT` (CSI Z) is implemented.
*   **Limitations**:
    *   **Width Constraint**: `MAX_TAB_STOPS` is 256. Modern terminals often exceed 256 columns. Tab behavior undefined beyond column 255.
    *   **Logic Verification**: `NextTabStop` logic needs to be verified against the `stops` array (ensure it doesn't just calculate `(x/8+1)*8`).
    *   **Resize Behavior**: Currently, resizing the session (`KTerm_ResizeSession`) does not appear to manage tab stops. If the width increases, new default tabs are not automatically added (standard behavior might be to have defaults, or empty).

## 3. Objectives

1.  **Full VT Conformance**: Support standard escape sequences (HT, HTS, TBC, CBT) with correct parameter handling (e.g., `TBC 0` vs `TBC 3`).
2.  **Scalability**: Support tab stops for terminal widths > 256 columns.
3.  **Robustness**: Ensure tab operations respect margins and cursor wrapping states where applicable.

## 4. Implementation Plan

### Phase 1: Core Logic Verification & Fixes

*   **Verify `NextTabStop`**:
    *   Ensure it iterates through the `stops` array starting from `cursor.x + 1`.
    *   Ensure it halts at `session->cols` (or right margin).
    *   **Fix**: If `NextTabStop` currently uses a mathematical fallback (modulo 8) instead of the array, it must be updated to strictly use the array.
*   **Verify `HT` (0x09) Handling**:
    *   Confirm `KTerm_ProcessControlChar` calls `NextTabStop`.
*   **Verify `TBC` (CSI g)**:
    *   `CSI 0 g`: Clear tab at current column.
    *   `CSI 3 g`: Clear all tab stops.
    *   Ensure default param is 0.

### Phase 2: Scalability (Dynamic Tab Stops)

*   **Problem**: `MAX_TAB_STOPS` = 256 is static.
*   **Solution**:
    1.  Convert `stops` to a dynamic bitfield or boolean array allocated based on `session->cols`.
    2.  Update `KTerm_InitTabStops` to allocate `stops` based on terminal width.
    3.  Update `KTerm_ResizeSession` to:
        *   Reallocate `stops` array when width increases.
        *   Initialize new columns (typically no tabs, or default every 8th? VT standard implies tabs are set at fixed intervals by default, but extending screen might simply leave them empty until set. *Decision: Initialize new space with default 8-width intervals for user convenience, or leave empty for strictness*).
        *   Preserve existing tabs.

### Phase 3: Margin & Cursor Interaction

*   **Cursor Bounds**: Ensure `HT` does not move cursor beyond the rightmost column.
*   **Margins**:
    *   Standard VT behavior: Tabs are absolute columns, not relative to scrolling regions.
    *   However, movement might be clamped by the right margin if `DECOM` (Origin Mode) is active? (Need to verify standard: usually HT ignores scroll margins but stops at screen edge).

### Phase 4: Testing Strategy

A new test suite `tests/test_tabs_full.c` will be created to verify:

1.  **Defaults**: Verify tabs exist at 8, 16, 24... on init.
2.  **Navigation**:
    *   Move cursor to 0, emit `\t`, verify x=8.
    *   Emit `\t` multiple times, verify 16, 24.
3.  **Set/Clear**:
    *   Move to 4, emit `ESC H` (HTS).
    *   Move to 0, emit `\t`, verify x=4.
    *   Emit `CSI 0 g` (TBC) at 4.
    *   Move to 0, emit `\t`, verify x=8 (4 is skipped).
    *   Emit `CSI 3 g` (Clear All). Verify `\t` goes to right edge.
4.  **Backward**:
    *   Move to 16, emit `CSI Z` (CBT), verify x=8.
5.  **Resize**:
    *   Resize to 300 cols.
    *   Verify tab stop at column 296 (if default 8-width applied).

## 5. Proposed Code Changes

### `kterm.h` - Struct Update
```c
typedef struct {
    bool* stops;       // Changed from fixed array to pointer
    int capacity;      // Track allocation size
    int count;
    int default_width;
} TabStops;
```

### `kterm.h` - Lifecycle Updates
*   **Init**: `stops = calloc(width, sizeof(bool))`
*   **Cleanup**: `free(stops)`
*   **Resize**: `realloc` logic.

### `kterm.h` - Logic Updates
Refine `NextTabStop_Internal` to strictly check the `stops` array bounds.

---
**Status**: Plan Drafted. Ready for Review.
