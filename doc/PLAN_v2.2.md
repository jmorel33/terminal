# K-Term v2.2 Refactoring Plan: "The Multiplexer & Graphics Update"

**Execution Status**: PLANNING

## Philosophy & Architecture
Version 2.2 transforms K-Term from a single-session emulator into a modern, compositing terminal multiplexer with advanced graphics capabilities.
- **Multiplexer (The Brain)**: The Core (`kterm.h`) moves from managing a linear list of `KTermSession` to a `KTermLayout` tree. It handles routing input to the "focused pane" while keeping all sessions alive and processing output.
- **Compositor (The Eyes)**: The rendering logic is updated to iterate over visible panes in the layout tree, calculating viewports and applying clipping rectangles for each session.
- **Kitty Graphics (The Visuals)**: Implementation of the Kitty Graphics Protocol allowing direct GPU texture uploads, overlaying the text grid, supporting transparency and animations.
- **Production Readiness**: Every new feature is strictly bounded by memory limits and fuzzed for security.

**Goal**: Seamless tiling window management and rich media support with **Zero Regression** in text performance.

---

## Phase 1: Architecture Refinement (Multiplexer Foundation)
*Objective: Prepare the core structures to support multiple simultaneous sessions with distinct geometries.*

### 1.1 Data Structures
- [ ] **Define `KTermPane`**: A recursive tree structure (or Binary Space Partition) to hold layout state.
    - Types: `SPLIT_H`, `SPLIT_V`, `LEAF`.
    - Content: `LEAF` nodes point to a `KTermSession` index.
    - Geometry: `relative_size` (float) or `fixed_size` (int) for resizing.
- [ ] **Update `KTerm` Context**: Replace/Augment the current `active_session` logic:
    - `layout_root`: The root node of the layout tree.
    - `focused_pane`: Pointer to the currently active pane receiving keyboard input.
    - `sessions`: Continue using `KTermSession sessions[MAX_SESSIONS]` pool, but decouple their size from `term->width/height`.

### 1.2 Session Management & Resizing
- [ ] **Session Factory**: Isolate `KTerm_CreateSession` logic to easily spawn new shells into new panes.
- [ ] **Recursive Resize Logic**: Update `KTerm_Resize` (Window Resize) to:
    1.  Update `term->width/height`.
    2.  Traverse the `KTermLayout` tree to calculate the pixel/cell dimensions of each pane.
    3.  Call `KTerm_ResizeSession(session, w, h)` for each active session.
    4.  **Crucial**: Ensure `SIGWINCH` is propagated to the host shell for *each* resized session.
- [ ] **Input Routing**:
    - Update `KTerm_Update`: Continue iterating *all* sessions to call `KTerm_ProcessEvents` (keep background shells alive).
    - Update `KTerm_QueueInputEvent`: Route keyboard events *only* to the `focused_pane`'s session.

### 1.3 Verification
- [ ] **Unit Tests**: Test tree manipulation (splitting, closing, resizing) without rendering.
- [ ] **Memory Check**: Ensure closing a split correctly frees the associated session resources in the pool.

## Phase 2: The Compositor (Rendering Splits)
*Objective: Update the rendering pipeline to draw multiple viewports on one screen.*

### 2.1 Viewport Logic
- [ ] **Layout Calculation Pass**: A pre-render pass traversing the tree to calculate absolute screen coordinates (Rects) for each pane.
- [ ] **Clipping**: Update `KTerm_Draw` to accept a `Viewport` rect. The Compute Shader or Scissor Test must respect this boundary.
- [ ] **Independent Cursors**: Ensure the hardware cursor is only drawn for the `focused_pane` (or dimmed for others).

### 2.2 Render Loop Update
- [ ] **Iterative Rendering**: Refactor `KTerm_Draw` to loop through all visible leaf nodes.
    - *Optimization*: Only upload dirty rows for visible panes.
- [ ] **Coordinate Transform**: Adjust the render offset so each session draws at its calculated `(x, y)` on the GPU canvas.
- [ ] **Decoration**: (Optional) Draw borders or separators between panes using a simple line drawing pass.

### 2.3 Verification
- [ ] **Visual Test**: Manually verify two active sessions (e.g., `top` and `vim`) running side-by-side.
- [ ] **Resize Test**: Resize the main window and verify the layout adjusts proportionally and shells update their wrapping.

## Phase 3: Kitty Graphics Protocol (Core)
*Objective: Implement the foundational support for displaying images using the Kitty protocol.*

### 3.1 Protocol Parsing
- [ ] **APC/OSC Parser**: Hook into the `KTerm_ProcessInput` loop to detect `\x1B_G...` (APC) sequences used by Kitty.
- [ ] **Base64 Decoder**: High-performance, streaming Base64 decoder for image data chunks.
- [ ] **Command Handling**: Implement actions: `a=t` (transmit), `a=p` (put), `a=d` (delete), `q=1` (quiet).

### 3.2 Texture Management
- [ ] **Texture Abstraction**: Utilize the existing `KTermTexture` abstraction in `kterm.h` to manage image assets.
- [ ] **Image Storage**: `KTermSession` stores a list of "Placed Images" with their coordinates (rows/cols or pixels) and z-index.
- [ ] **Memory Limits**: Hard caps on total VRAM usage per session (e.g., 64MB) to prevent DoS.

### 3.3 Render Integration
- [ ] **Overlay Pass**: Update `KTerm_Draw` to draw the image list.
    - *Z-Ordering*: Images can be behind text (background) or in front (overlay).
- [ ] **Scroll Handling**: Ensure images anchored to text move correctly when the buffer scrolls (modify `y` coordinate relative to `screen_head`).
- [ ] **Clipping**: Images must be clipped to their session's viewport.

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
