# K-Term v2.1 Refactoring Plan: "Situation Decoupling"

**Execution Status**: APPROVED FOR EXECUTION

## Philosophy & Architecture
The core philosophy of K-Term v2.1 is strict **Separation of Concerns**.
- **The Core (`kterm.h`)**: Pure terminal emulation logic. It manages the state (screen buffer, cursor, modes), processes the "Pipeline Input" (byte stream from host), and generates "Pipeline Output" (byte stream to host). It is agnostic to the windowing system, input method, or rendering backend.
- **The Adapters (`kterm_io_sit.h`, `kterm_render_sit.h`)**: These are *reference implementations* binding K-Term to the **Situation** library. Users are free to replace these with adapters for SDL, GLFW, Raylib, or custom engines.
- **Independence**: The Core must not depend on external types like `SIT_KEY_*`, `Vector2`, or `Color`. It defines its own abstract interfaces or uses standard C types, which the Adapters translate to/from their specific backend requirements.

**Status**: Planning
**Goal**: Fail-Safe Refactoring with **Zero Functional Regression**. The refactored system, when using the provided Situation adapters, must behave exactly as the current monolithic version.

---

## Phase 0: Preparation & Baseline
*Ensure the current system is stable and tests are ready to detect any regressions during the split.*

- [ ] **Baseline Verification**: Run all existing tests (`test_vttest_suite.c`, `test_safety.c`, etc.) and ensure they pass.
- [ ] **Analyze Dependencies**: Map all Situation API calls currently in `kterm.h` to ensure nothing is missed.
- [ ] **Backup**: Create a "pre-refactor" tag or branch (implicit via git).

## Phase 1: Input Decoupling (`kterm_io_sit.h`)
*Objective: Completely detach keyboard/mouse handling from the core. The Core should only deal with pipeline input (Display) and pipeline output (Host).*

### 1.1 Create IO Adapter (`kterm_io_sit.h`)
- [ ] **Create Header**: Setup `kterm_io_sit.h` with `situation.h` dependency.
- [ ] **Implement Reference Logic**:
    - [ ] Create `KTermSit_ProcessInput(KTerm* term)`.
    - [ ] **Logic**: Poll `SITUATION` events.
    - [ ] **Translation**: The adapter (not the core) decides that `Shift` + `A` = `A` or `Up Arrow` = `\x1B[A`.
    - [ ] **Output**: Call `KTerm_QueueResponse(term, sequence)` to push the resulting byte stream to the host (Pipeline Output).
    - [ ] **State Access**: Read generic Core state (e.g., `term->dec_modes.application_cursor_keys`) to adjust translation logic.

### 1.2 Clean Core Input Surface (`kterm.h`)
- [ ] **Remove `VTKeyEvent`**: Remove this struct entirely. The core does not need to know about key events, modifiers, or key codes.
- [ ] **Remove `vt_keyboard`**: Remove the `vt_keyboard` struct/buffer from `KTermSession`.
- [ ] **Remove Polling**: Delete `KTerm_UpdateKeyboard` and `KTerm_UpdateMouse` prototypes and implementations from `kterm.h`.
- [ ] **Update Core Loop**: Remove calls to `KTerm_UpdateKeyboard/Mouse` from `KTerm_Update`. The user application must call the adapter's input function explicitly.

### 1.3 Verification (Fail Safe)
- [ ] **Compile Test**: Verify `kterm_io_sit.h` compiles.
- [ ] **Logic Test**: Verify `KTermSit_ProcessInput` correctly generates sequences and they appear in the `answerback_buffer`.

## Phase 2: Render Decoupling (`kterm_render_sit.h`)
*Objective: Move all Vulkan/OpenGL/Situation rendering logic out of the core, allowing for pluggable renderers.*

### 2.1 Define Generic Renderer Interface
- [ ] **Opaque Renderer Handle**: Add `void* renderer_context;` to `struct KTerm` in `kterm.h` to store backend-specific data.
- [ ] **Remove Situation Fields**: Remove `SituationComputePipeline`, `SituationBuffer`, etc. from `struct KTerm`.
- [ ] **Move `stb_truetype`**: Move font parsing logic and `stb_truetype.h` include to the Adapter layer (`kterm_render_sit.h`). The core only cares about codepoints and cells.

### 2.2 Type & Struct Decoupling
- [ ] **Internalize `Vector2`**: Replace `Vector2` usage in `kterm.h` with a local `struct { float x, y; }` or `KTermVector2` (standard C POD).
- [ ] **Internalize `Color`**: Replace `Color` usage (from Situation) with `RGB_Color` (already in `kterm.h`) or `KTermColor` (standard C POD).
- [ ] **Move Render Structs**: Move `KTermPushConstants` and other shader-specific structs to `kterm_render_sit.h`.

### 2.3 Create Render Adapter (`kterm_render_sit.h`)
- [ ] **Create Header**: Setup guards.
- [ ] **Define Concrete Context**: Define `struct KTermSituationRenderer` to hold Situation resources.
- [ ] **Implement Reference Logic**:
    - [ ] `KTermSit_InitRenderer(KTerm* term)`: Allocates context, shaders, and textures.
    - [ ] `KTermSit_Draw(KTerm* term)`: Reads Core state (screen buffer, cursor) and issues Situation draw calls.

### 2.4 Refactor Core Lifecycle
- [ ] **Update `KTerm_Create`**: Remove renderer initialization.
- [ ] **Update `KTerm_Resize`**: Remove texture resizing. Add a hook or require the user to call `KTermSit_Resize`.
- [ ] **Update `KTerm_Cleanup`**: Remove renderer cleanup.

### 2.5 Verification (Fail Safe)
- [ ] **Compile Test**: Compile `kterm_render_sit.h` in isolation.
- [ ] **Logic Test**: Ensure `kterm.h` compiles with render logic removed.

## Phase 3: Full Core Disconnection
*Objective: Sever all remaining ties to Situation and ensure strict independence.*

### 3.1 Remove Dependencies
- [ ] **Remove Includes**: Remove `#include "situation.h"` and `#include "stb_truetype.h"` from `kterm.h`.
- [ ] **Remove Conditional Blocks**: Remove any `#ifdef SITUATION_IMPLEMENTATION` or `SITUATION_USE_VULKAN` blocks from `kterm.h`.

### 3.2 Sanitize Types
- [ ] **Audit Types**: Verify no `Situation*` types or `SIT_*` constants remain in `kterm.h`.
- [ ] **Generic Interfaces**: Ensure any remaining hooks use `void*` or standard types.

### 3.3 Standalone Verification (Fail Safe)
- [ ] **Standalone Compile**: Create a minimal test file `test_standalone.c` that includes *only* `kterm.h` and tries to compile. It **must** succeed without any Situation headers in the include path.
- [ ] **ABI Check**: Verify the size of `struct KTerm` is fixed and independent of Situation definitions.

## Phase 4: Integration & Functional Parity
*Objective: Reassemble the system using the new modular components and prove it works exactly as before.*

- [ ] **Update Examples**: Update `main.c` to use `kterm.h` + `kterm_io_sit.h` + `kterm_render_sit.h`.
- [ ] **Functional Parity Check**:
    - [ ] **Run Regression Suite**: Run the full `vttest` suite (cursor, screen, scrolling). The behavior must be 1:1 identical to Phase 0.
    - [ ] **Graphics Verification**: Verify Sixel and ReGIS graphics render correctly using the new `kterm_render_sit.h` adapter.
    - [ ] **Input Verification**: Verify complex key combos (Ctrl+C, Arrow Keys) work exactly as before.
- [ ] **Update Tests**: Refactor `tests/` to use the Situation Adapters for integration tests.
- [ ] **Documentation**: Update `README.md` and `kterm.md` to explain the "Separation of Concerns" and how to swap adapters.
- [ ] **Final Clean**: Remove any deprecated code.

## Fail-Safe Rollback Strategy
- If the "Functional Parity Check" in Phase 4 fails (i.e., the modular system behaves differently than the monolithic one), treat it as a critical regression.
- Use `git stash` or feature branches for the experimental split before committing to main.
- Validate with `tests/test_safety.c` after every structural change.
