# K-Term v2.2 Refactoring Plan: "The Multiplexer & Graphics Update"

**Execution Status**: IN PROGRESS (Phase 2 Complete)

## Philosophy & Architecture
Version 2.2 transforms K-Term from a single-session emulator into a modern, compositing terminal multiplexer with advanced graphics capabilities.
- **Multiplexer (The Brain)**: The Core (`kterm.h`) moves from managing a linear list of `KTermSession` to a `KTermLayout` tree. It handles routing input to the "focused pane" while keeping all sessions alive and processing output.
- **Compositor (The Eyes)**: The rendering logic is updated to iterate over visible panes in the layout tree, calculating viewports and applying clipping rectangles for each session.
- **Kitty Graphics (The Visuals)**: Implementation of the Kitty Graphics Protocol allowing direct GPU texture uploads, overlaying the text grid, supporting transparency and animations.
- **Production Readiness**: Every new feature is strictly bounded by memory limits and fuzzed for security.

**Goal**: Seamless tiling window management and rich media support with **Zero Regression** in text performance.

## Technical Critique & Refinements (Post-Phase 1 Analysis)
Following the initial Phase 1 implementation, several critical integration challenges between the Multiplexer and Graphics subsystems have been identified. These refinements must be addressed in subsequent phases.

1.  **Graphics vs. Split Conflict**:
    *   *Problem*: Tiling and Graphics compete for screen real estate. Resizing a pane could clip, scale, or destroy existing graphics (Kitty/ReGIS).
    *   *Refinement (Phase 3.3)*: Explicitly define a "Resize Policy". Kitty images will generally anchor to their row/col position and be **clipped** (not scaled) by the new viewport to preserve aspect ratio. ReGIS logical coordinates (800x480) must be dynamically re-mapped to the new viewport dimensions during the render pass.

2.  **The Reflow Nightmare**:
    *   *Problem*: Sending `SIGWINCH` is not enough; the internal shell history (scrollback) must visually reflow when width changes, otherwise text disappears.
    *   *Refinement (Phase 1.2 Extension)*: Implement **Buffer Reflow Logic**. When `KTerm_ResizeSession` reduces column count, the internal ring buffer must re-wrap lines. Cursor Y position must be adjusted to track the new height of the content.

3.  **Input Routing & The "Meta" Key**:
    *   *Problem*: Need a way to distinguish between typing and controlling the multiplexer (e.g., `Ctrl+B %` to split).
    *   *Refinement (Phase 1.2 Extension)*: Implement a **Multiplexer Input Interceptor** state machine within `KTerm_ProcessInput`. It should swallow a configurable Prefix Key and wait for a command key before routing data to the active session.

4.  **Memory Management for Graphics**:
    *   *Problem*: Unlimited graphics loading can lead to DoS (VRAM exhaustion).
    *   *Refinement (Phase 3.2)*: Enforce a strict VRAM cap (e.g., 64MB) per session. Implement an **LRU Eviction Policy** to automatically discard the oldest images that have scrolled off-screen when the limit is reached.

5.  **Z-Ordering and Text**:
    *   *Problem*: Kitty protocol allows images to be interspersed with text (above/below).
    *   *Refinement (Phase 3.3)*: The render loop must respect Z-order: Background -> Images (z<0) -> Text -> Images (z>0). Batching draw calls is essential for performance with multiple images.

---

## Phase 1: Architecture Refinement (Multiplexer Foundation)
*Objective: Prepare the core structures to support multiple simultaneous sessions with distinct geometries.*

### 1.1 Data Structures (DONE)
- [x] **Define `KTermPane`**: Recursive tree structure (`KTermPane`) implemented in `kterm.h`.
    - Types: `PANE_SPLIT_H`, `PANE_SPLIT_V`, `PANE_LEAF`.
    - Content: Leaves hold `session_index`.
- [x] **Update `KTerm` Context**: Added `layout_root` and `focused_pane` to `KTerm` struct.

### 1.2 Session Management & Resizing (DONE)
- [x] **Session Factory**: `KTerm_SplitPane` implements logic to find free sessions and initialize them.
- [x] **Recursive Resize Logic**: `KTerm_Resize` now calls `KTerm_RecalculateLayout` to traverse the tree and `KTerm_ResizeSession` to update individual session geometries.
- [x] **Callback**: Added `SessionResizeCallback` to notify the host application of session-specific resize events (for `TIOCSWINSZ`).

### 1.3 Verification (DONE)
- [x] **Unit Tests**: `tests/test_multiplexer.c` created and verified.
- [x] **Memory Check**: `KTerm_Destroy` updated to recursively free the pane tree.

## Phase 2: The Compositor (Rendering Splits)
*Objective: Update the rendering pipeline to draw multiple viewports on one screen.*

### 2.1 Viewport Logic
- [x] **Layout Calculation Pass**: A pre-render pass traversing the tree to calculate absolute screen coordinates (Rects) for each pane.
- [x] **Clipping**: Update `KTerm_Draw` to accept a `Viewport` rect. The Compute Shader or Scissor Test must respect this boundary.
- [x] **Independent Cursors**: Ensure the hardware cursor is only drawn for the `focused_pane` (or dimmed for others).

### 2.2 Render Loop Update
- [x] **Iterative Rendering**: Refactor `KTerm_Draw` to loop through all visible leaf nodes.
    - *Optimization*: Only upload dirty rows for visible panes.
- [x] **Coordinate Transform**: Adjust the render offset so each session draws at its calculated `(x, y)` on the GPU canvas.
- [ ] **Decoration**: (Optional) Draw borders or separators between panes using a simple line drawing pass.

### 2.3 Verification
- [x] **Visual Test**: Manually verify two active sessions (e.g., `top` and `vim`) running side-by-side.
- [x] **Resize Test**: Resize the main window and verify the layout adjusts proportionally and shells update their wrapping.

## Phase 3: Kitty Graphics Protocol (Core)
*Objective: Implement the foundational support for displaying images using the Kitty protocol.*

### 3.1 Protocol Parsing (Partially Complete - Core)
- [x] **APC/OSC Parser**: Hook into the `KTerm_ProcessInput` loop to detect `\x1B_G...` (APC) sequences used by Kitty.
- [x] **Base64 Decoder**: High-performance, streaming Base64 decoder for image data chunks.
- [x] **Command Handling**: Implement actions: `a=t` (transmit), `a=p` (put), `a=d` (delete), `q=1` (quiet).

### 3.2 Texture Management
- [x] **Texture Abstraction**: Utilize the existing `KTermTexture` abstraction in `kterm.h` to manage image assets.
- [x] **Image Storage**: `KTermSession` stores a list of "Placed Images" with their coordinates (rows/cols or pixels) and z-index.
- [x] **Memory Limits**: Hard caps on total VRAM usage per session (e.g., 64MB) to prevent DoS.

### 3.3 Render Integration
- [ ] **Overlay Pass**: Update `KTerm_Draw` to draw the image list.
    - *Z-Ordering*: Images can be behind text (background) or in front (overlay).
- [ ] **Scroll Handling**: Ensure images anchored to text move correctly when the buffer scrolls (modify `y` coordinate relative to `screen_head`).
- [ ] **Clipping**: Images must be clipped to their session's viewport.

### 3.4 Cleanup & Refactoring (Nitpicks)
- [ ] **Refactor Parser**: Extract duplicated buffer setup logic in `KTerm_ProcessKittyChar` into a helper function.
- [ ] **Global Cleanup**: Update `KTerm_Cleanup` to iterate through *all* sessions (not just the active one) to ensure Kitty resources in background sessions are freed.
- [ ] **Chunked Transmission**: Handle `m=1` to append data to an existing image ID rather than creating a new entry every time.
- [ ] **Placement Logic**: Implement `a=p` logic to store image placement coordinates (x, y, z) in `KittyImageBuffer`, which is currently missing.

## Phase 4: Kitty Advanced (Animation & Polish)
*Objective: Support advanced features like animation and transparency.*

### 4.1 Animation Support
- [ ] **Frame Storage**: Store multiple frames for a single Image ID.
- [ ] **Time Management**: Track delta time in `KTerm_Update` to advance animation frames based on metadata (delay).

### 4.2 Transparency & Blending
- [ ] **Shader Update**: Ensure the Render Adapter supports alpha blending for the image quad.
- [ ] **Composition**: Handle cases where text is drawn *on top* of a semi-transparent image.

## Phase 5: Production Readiness & Quality Assurance
*Objective: Ensure features are robust and secure.*

- [ ] **Fuzzing**: Fuzz the Image Protocol parser. Malformed Base64 or invalid dimensions must not crash the terminal.
- [ ] **Performance Profiling**: Ensure tiling 4+ panes does not drop FPS below 60.
- [ ] **Regression Testing**: Verify `vttest` still passes perfectly in a single pane.
- [ ] **Documentation**: Update user manual with keybindings for the Multiplexer (e.g., `Ctrl+B %`).

## Fail-Safe Rollback Strategy
- The Multiplexer features will be gated behind a `KTERM_FEATURE_MULTIPLEXER` define during development.
- If rendering regressions occur, we revert the `KTerm_Draw` loop to the legacy "single active session" mode while debugging.
