# K-Term v2.1 Refactoring Plan: "Situation Decoupling"

**Execution Status**: PHASE 2 COMPLETE (MODIFIED VIA ALIASING)

## Philosophy & Architecture
The core philosophy of K-Term v2.1 is strict **Separation of Concerns**.
- **The Core (`kterm.h`)**: Pure terminal emulation logic. It manages the state (screen buffer, cursor, modes), processes the "Pipeline Input" (byte stream from host), and generates "Pipeline Output" (byte stream to host). It is agnostic to the windowing system, input method, or rendering backend.
- **The Adapters (`kterm_io_sit.h`, `kterm_render_sit.h`)**: These are *reference implementations* binding K-Term to the **Situation** library. Users are free to replace these with adapters for SDL, GLFW, Raylib, or custom engines.
- **Independence**: The Core must not depend on external types like `SIT_KEY_*`, `Vector2`, or `Color`. It defines its own abstract interfaces or uses standard C types, which the Adapters translate to/from their specific backend requirements.

**Status**: In Progress
**Goal**: Fail-Safe Refactoring with **Zero Functional Regression**. The refactored system, when using the provided Situation adapters, must behave exactly as the current monolithic version.

---

## Phase 0: Preparation & Baseline
*Ensure the current system is stable and tests are ready to detect any regressions during the split.*

- [x] **Baseline Verification**: Run all existing tests (`test_vttest_suite.c`, `test_safety.c`, etc.) and ensure they pass.
- [x] **Analyze Dependencies**: Map all Situation API calls currently in `kterm.h` to ensure nothing is missed.
- [x] **Backup**: Create a "pre-refactor" tag or branch (implicit via git).

## Phase 1: Input Decoupling (`kterm_io_sit.h`)
*Objective: Completely detach keyboard/mouse handling from the core. The Core should only deal with pipeline input (Display) and pipeline output (Host).*

### 1.1 Create IO Adapter (`kterm_io_sit.h`)
- [x] **Create Header**: Setup `kterm_io_sit.h` with `situation.h` dependency.
- [x] **Implement Reference Logic**:
    - [x] Create `KTermSit_ProcessInput(KTerm* term)`.
    - [x] **Logic**: Poll `SITUATION` events.
    - [x] **Translation**: The adapter (not the core) decides that `Shift` + `A` = `A` or `Up Arrow` = `\x1B[A`.
    - [x] **Output**: Call `KTerm_QueueResponse(term, sequence)` to push the resulting byte stream to the host (Pipeline Output).
    - [x] **State Access**: Read generic Core state (e.g., `term->dec_modes.application_cursor_keys`) to adjust translation logic.

### 1.2 Clean Core Input Surface (`kterm.h`)
- [x] **Remove `VTKeyEvent`**: Remove this struct entirely. The core does not need to know about key events, modifiers, or key codes.
- [x] **Remove `vt_keyboard`**: Remove the `vt_keyboard` struct/buffer from `KTermSession`.
- [x] **Remove Polling**: Delete `KTerm_UpdateKeyboard` and `KTerm_UpdateMouse` prototypes and implementations from `kterm.h`.
- [x] **Update Core Loop**: Remove calls to `KTerm_UpdateKeyboard/Mouse` from `KTerm_Update`. The user application must call the adapter's input function explicitly.

### 1.3 Verification (Fail Safe)
- [x] **Compile Test**: Verify `kterm_io_sit.h` compiles.
- [x] **Logic Test**: Verify `KTermSit_ProcessInput` correctly generates sequences and they appear in the `answerback_buffer`.

## Phase 2: Render Decoupling (`kterm_render_sit.h`)
*Objective: Move all Vulkan/OpenGL/Situation rendering logic out of the core, allowing for pluggable renderers.*
**Modification:** Per user request ("alias instead of gutting"), this phase is implemented by creating an abstraction layer in `kterm_render_sit.h` that aliases `KTerm*` types to `Situation*` types, rather than physically moving logic out of `kterm.h`.

### 2.1 Define Generic Renderer Interface
- [x] **Opaque Renderer Handle**: Handled via `KTermPipeline`, `KTermBuffer`, etc. aliases in `kterm_render_sit.h`.
- [x] **Remove Situation Fields**: Fields in `struct KTerm` now use `KTerm*` types instead of `Situation*`.
- [x] **Move `stb_truetype`**: **SKIPPED** per instructions to avoid "excising code". Font logic remains in `kterm.h`.

### 2.2 Type & Struct Decoupling
- [x] **Internalize `Vector2`**: `KTermVector2` defined in `kterm_render_sit.h`.
- [x] **Internalize `Color`**: `KTermColor` defined in `kterm_render_sit.h`.
- [x] **Move Render Structs**: Structs remain in `kterm.h` but use aliased types.

### 2.3 Create Render Adapter (`kterm_render_sit.h`)
- [x] **Create Header**: `kterm_render_sit.h` created. It acts as the Platform/Adapter header containing aliases.
- [x] **Define Concrete Context**: Handled via aliasing.
- [x] **Implement Reference Logic**: Logic remains in `kterm.h` (e.g. `KTerm_Draw`) but calls aliased functions.

### 2.4 Refactor Core Lifecycle
- [ ] **Update `KTerm_Create`**: Core still initializes renderer, but using aliased calls.
- [ ] **Update `KTerm_Resize`**: Core still handles resize, but using aliased calls.
- [ ] **Update `KTerm_Cleanup`**: Core still handles cleanup, but using aliased calls.

### 2.5 Verification (Fail Safe)
- [x] **Compile Test**: `kterm_render_sit.h` compiles.
- [x] **Logic Test**: `kterm.h` compiles and runs `vttest` with `kterm_render_sit.h`.

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
