# v2.0 Plan: Multi-Session & Protocol Completeness

This plan outlines the strict, phased roadmap to upgrade `terminal.h` from v1.5 to v2.0, focusing on closing specific implementation holes in multi-session support, VT520 compliance, and robustness.

**Goal**: Full DEC VT520 session management compliance, robust per-session state, and xterm/modern extension parity.
**Timeline**: 4–6 weeks (Est. 20-30 hrs/week).

---

## Phase 1: VT520 Session Protocol Integration (1–1.5 weeks)

**Objective**: Enable host-controlled session management via standard VT520 escape sequences, replacing client-side-only C API calls.

### Task 1.1: Implement Session Selection (DECSN) & Reporting (DECRSN)
*Allow the host to switch the active session and query which session is active.*

- [x] **Define Constants/State**: Ensure `MAX_SESSIONS` (3) is respected.
- [x] **Implement `ExecuteDECSN` (Select Session Number)**:
    - [x] Create function `ExecuteDECSN(void)` handling `CSI Ps ! ~`.
    - [x] Parse `Ps` (1-based session number).
    - [x] Validate against `MAX_SESSIONS`.
    - [x] Call `SetActiveSession(Ps - 1)`.
    - [x] Integrate into `ProcessCSIChar` dispatch logic for `! ~`.
- [x] **Implement `ExecuteDECRSN` (Report Session Number)**:
    - [x] Create function `ExecuteDECRSN(void)` handling `CSI ? Ps n` (specifically `Ps=12` for Device Status which can cover session).
    - [x] Implement response: `CSI ? 12 ; {session_num} n` indicating the active session number.
- [x] **Verification**:
    - [x] Create a test script that sends `\x1B[2!~` (Select Session 2) and asserts `active_session == 1`.
    - [x] Send `\x1B[?12n` and verify the answerback buffer contains `\x1B[?12;{n}n`.

### Task 1.2: Implement Session Status Reporting (DECRS)
*Allow the host to query detailed status of sessions.*

- [x] **Implement `ExecuteDECRS`**:
    - [x] Handle `CSI ? 21 n` (Report Session Status).
    - [x] Iterate through `terminal.sessions`.
    - [x] Construct response string indicating open/active sessions.
    - [x] Format: `DCS $ p {data} ST`.
- [x] **Update `TerminalSession`**:
    - [x] Ensure `bool session_open` or similar flag exists (implicitly via `MAX_SESSIONS` and initialization state).
- [x] **Verification**:
    - [x] Create a test script to send `CSI ? 21 n` and inspect the output buffer for the session status report.

### Task 1.3: Implement Split-Screen Protocol (DECSASD / DECSSDT)
*Enable host-controlled split-screen views.*

- [x] **Implement `ExecuteDECSASD` (Select Active Status Display)**:
    - [x] Handle `CSI Ps $ }`.
    - [x] `Ps=0`: Main display. `Ps=1`: Status line.
- [x] **Implement `ExecuteDECSSDT` (Select Split Definition)**:
    - [x] Handle `CSI Ps $ ~`.
    - [x] `Ps=0`: No split.
    - [x] `Ps=1`: Horizontal split.
    - [x] Map to `SetSplitScreen(true/false, ...)` logic.
- [x] **Refine `SetSplitScreen`**:
    - [x] Add logic to define split point (default center or parameterized).
- [x] **Verification**:
    - [x] Send `CSI 1 $ ~` and visually verify (or check `terminal.split_screen_active` state) that the screen splits.

### Task 1.4: Multi-Session Enable/Disable (CSI ? 64 h/l)
*Allow host to lock/unlock multi-session capability.*

- [x] **Add Mode Flag**: Add `bool multi_session_mode` to `VTFeatures` or `Terminal` struct.
- [x] **Update `ExecuteSM`/`ExecuteRM`**:
    - [x] Handle private parameter `64`.
    - [x] On `h`: Enable session switching logic.
    - [x] On `l`: Force switch to Session 1, disable `ExecuteDECSN` handling.
- [x] **Verification**:
    - [x] Send `CSI ? 64 l`, then attempt `CSI 2 ! ~` and verify session does *not* switch.

---

## Phase 2: Per-Session State & Input Robustness (1 week)

**Objective**: Ensure every session maintains independent state (cursors, modes, stacks) and input is correctly routed even when targeted remotely.

### Task 2.1: Per-Session Save/Restore Stacks (DECSC/DECRC)
*Fix the hole where `ESC 7` / `ESC 8` might overwrite a global shared state.*

- [x] **Verify `TerminalSession` Struct**:
    - [x] Confirm `saved_cursor` is present in `TerminalSession`.
- [x] **Refactor Save/Restore Functions**:
    - [x] Ensure `SaveCursor()` uses `ACTIVE_SESSION.saved_cursor`.
    - [x] Ensure `RestoreCursor()` uses `ACTIVE_SESSION.saved_cursor`.
- [x] **Add Extended State Saving**:
    - [x] Ensure `saved_cursor` struct includes `origin_mode`, `attribute` flags, and `charset` state.
- [x] **Verification**:
    - [x] Save cursor in Session 1, move cursor, switch to Session 2, save/move/restore. Switch back to Session 1 and restore. Verify both restored correctly.

### Task 2.2: Input Routing Protocol
*Allow host to pipe input to a specific session without switching the active view.*

- [x] **Enhance `PipelineWriteCharToSession`**:
    - [x] Ensure it writes to the target session's `input_pipeline`.
    - [x] Verify `ProcessPipeline` iterates ALL sessions.
- [x] **Verification**:
    - [x] Use a test harness to inject characters into Session 2's pipeline while Session 1 is active. Verify Session 2's screen buffer updates.

### Task 2.3: Session-Aware Locator & Printer
*Fix the "global default" behavior of auxiliary devices.*

- [x] **Printer Controller**:
    - [x] Ensure `ProcessPrinterControllerChar` uses `ACTIVE_SESSION` buffer.
- [x] **Mouse/Locator**:
    - [x] Update `DECRQLP` (Request Locator Pos) to report coordinates relative to the *session's* scrolling region.
    - [x] Update `ExecuteDECSLE` to set `ACTIVE_SESSION.locator_events`.
- [x] **Verification**:
    - [x] Enable printer controller in Session 1. Send data. Switch to Session 2. Verify Session 2 does not capture printer data unless enabled there.

### Task 2.4: Independent Glyph Caches & Charsets
*Ensure Session 1 (Greek) and Session 2 (Line Draw) don't corrupt each other's rendering.*

- [x] **Review `TranslateCharacter`**:
    - [x] Confirm `ACTIVE_SESSION.charset` is used.
- [x] **Split Rendering Logic**:
    - [x] Refactor `UpdateTerminalSSBO` to explicitly accept `TerminalSession*` for the row being processed.
    - [x] Ensure `GetScreenCell` calls in the loop use the correct session pointer.
- [x] **Verification**:
    - [x] Set Session 1 to G0=ASCII, Session 2 to G0=Special Graphics. Render split screen and verify character mapping is correct for each half.

---

## Phase 3: xterm/Modern + Rendering Polish (1 week)

**Objective**: Modernize the experience for xterm applications running in split sessions.

### Task 3.1: Session-Specific xterm Extensions
- [x] **OSC 0/1/2 (Title/Icon)**:
    - [x] Ensure `VTSituationSetWindowTitle` updates `ACTIVE_SESSION.title`.
    - [x] Update window title logic to reflect the active session or split state.
- [x] **OSC 9 (Notifications)**:
    - [x] Implement `ExecuteOSC` case `9`.
    - [x] Trigger notification callback with `active_session` ID.
- [x] **Mouse Focus in Splits**:
    - [x] Update `UpdateMouse`:
        - [x] Calculate which session viewport is under the mouse cursor.
        - [x] Route click/scroll events to that session.
- [x] **Verification**:
    - [x] Send OSC 0 to Session 1 and Session 2. Check internal `window_title` struct for each.

### Task 3.2: Scrollback & Dirty Rows in Splits
*Fix visual glitches when scrolling history in a split view.*

- [x] **Refine `ScrollUpRegion`**:
    - [x] Ensure it strictly respects `scroll_top` and `scroll_bottom` of the specific session.
- [x] **Per-Session Scrollback View**:
    - [x] Ensure `Shift+PageUp/Down` targets the *focused* session in the split.
- [x] **Verification**:
    - [x] Fill scrollback in Session 1. Split screen. Scroll back in Session 1. Verify Session 2 remains static.

### Task 3.3: Visual Bell Per Session
- [x] **Update `TerminalPushConstants`**:
    - [x] Add `float visual_bell_intensity;` to `TerminalPushConstants` struct in `terminal.h`.
- [x] **Update `DrawTerminal`**:
    - [x] Pass `ACTIVE_SESSION.visual_bell_timer` to the shader via the new push constant.
- [x] **Update Shader**:
    - [x] Modify `TERMINAL_SHADER_BODY` to use the new push constant for the flash effect.
- [x] **Verification**:
    - [x] Trigger bell (`\a`) in Session 1. Verify visual flash.

---

## Phase 4: Full Testing & Gateway Prep (0.5–1 week)

**Objective**: Verify v2.0 stability and prepare hooks for the Gateway component.

### Task 4.1: VTTEST Compliance
- [ ] **Run `vttest`**:
    - [ ] Menu 1 (Test of cursor movements).
    - [ ] Menu 2 (Test of screen features).
- [ ] **Fix Regressions**:
    - [ ] Log any failures in `conformance.compliance`.

### Task 4.2: Graphics & Stress Tests
- [ ] **Sixel Split Test**:
    - [ ] Display Sixel image in Session 1 while scrolling text in Session 2.
    - [ ] Verify no overlap/corruption.
- [ ] **ReGIS Macro Test**:
    - [ ] Define macro in Session 1, try to invoke in Session 2.

### Task 4.3: Gateway Hooks
- [x] **DCS Passthrough**:
    - [x] Implement `DCS p` (Gateway Protocol placeholder).
    - [x] Create stub `ParseGatewayCommand(const char* data, size_t len)`.
- [x] **Verification**:
    - [x] Verify `ParseGatewayCommand` exists via `grep` and test compilation.

### Task 4.4: Documentation Update
- [ ] Update `terminal.md` with:
    - [ ] New DECSN/DECRSN/DECRS commands.
    - [ ] Split screen usage guide.
    - [ ] xterm extension support list.
- [ ] **Verification**:
    - [ ] Read `terminal.md` to confirm the new sections.

---

**Exit Criteria**:
- All Phase 1-3 tasks complete with checked actionables.
- `vttest` passes standard cursor/screen tests.
- Multi-session switching works via escape sequences.
- Code compiles with no warnings.


## Phase 5: Architectural Refactoring (Thread Safety & Instance Management)

**Goal**: Remove the reliance on the global `terminal` struct and make the library instance-based and thread-safe.

### Task 5.1: Introduce Terminal Context Handle
**Objective**: Change the API to operate on a pointer rather than a global object.

- [x] **Redefine `Terminal` Struct**:
    - Move the definition of `struct Terminal_T` (typedef `Terminal`) to a visible location but encourage usage via pointer.
    - Ensure `Terminal` struct contains all state currently in the global `terminal` variable.
- [x] **Update Initialization API**:
    - Deprecate/Remove `void InitTerminal(void)`.
    - Introduce `Terminal* Terminal_Create(TerminalConfig config)` which allocates and initializes a new instance.
    - Introduce `void Terminal_Destroy(Terminal* term)` for cleanup.
    - Define `TerminalConfig` struct to pass initial settings (width, height, callbacks, etc.).
- [x] **Update Callbacks**:
    - Update function pointer typedefs (`ResponseCallback`, `BellCallback`, `TitleCallback`, etc.) to accept `Terminal* term` as the first argument.
    - Update `TerminalSession` to include `void* user_data` for identifying the calling instance in callbacks.

### Task 5.2: Eliminate Global State Macros
**Objective**: Remove the global terminal variable and refactor internal macros.

- [x] **Remove Global Variable**:
    - Delete `extern struct Terminal_T terminal;` from the header and its definition in the implementation.
- [x] **Refactor `ACTIVE_SESSION`**:
    - Remove the global macro `#define ACTIVE_SESSION (terminal.sessions[terminal.active_session])`.
    - Introduce a macro or inline function `GET_SESSION(term)` that resolves to `(&(term)->sessions[(term)->active_session])`.
- [x] **Update Internal Functions**:
    - Refactor every internal function (e.g., `ProcessChar`, `DrawTerminal`, `ExecuteCSICommand`) to accept `Terminal* term` as a parameter.
    - Update all references inside these functions to use `term->` instead of `terminal.`.
    - Propagate the `term` pointer down the entire call stack.
- [x] **Update External API**:
    - Update public functions (e.g., `PipelineWriteChar`, `SetVTLevel`) to take `Terminal* term`.

## Phase 6: Portability & Code Hygiene

**Goal**: Remove non-standard C extensions and improve developer experience with shaders.

### Task 6.1: Replace Computed Gotos with Switch-Case
**Objective**: Remove GCC-specific `&&label` extension to support MSVC and standard C compilers.

- [x] **Locate `ExecuteCSICommand`**:
    - Identify the `static const void* const csi_dispatch_table[256]` and the `goto *target` logic.
- [x] **Refactor Logic**:
    - Replace the dispatch table with a standard `switch (command) { case 'A': ... }` block.
    - Move the logic from the labels (e.g., `L_CSI_A:`) into the corresponding `case` blocks.
    - Ensure the `default` case handles unsupported sequences (formerly `L_CSI_UNSUPPORTED`).
- [x] **Rationale**: Modern compilers optimize dense switch statements into jump tables automatically. This removes a non-portable GNU extension.

### Task 6.2: Shader Code Externalization (or Clean Embedding)
**Objective**: Make shader code readable and editable.

- [x] **Remove Macro Definitions**:
    - Remove `#define TERMINAL_SHADER_BODY "..."` style macros which are hard to edit.
- [x] **Implement `const char*` Constants**:
    - Move shader code to the bottom of the implementation section as `const char* terminal_vs_source = ...;`.
    - Use C string literal concatenation (implicit) instead of line-continuation backslashes for multi-line strings.
    - Example:
      ```c
      const char* shader_source =
      "#version 450\n"
      "void main() {\n"
      "   ...\n"
      "}\n";
      ```
- [x] **(Optional) Tooling**: Create a pre-build script to convert `.glsl` files to C headers.

## Phase 7: Safety & Robustness

**Goal**: Fix buffer overflow risks in the ReGIS and generic string parsers.

### Task 7.1: Introduce Safe Parsing Primitives
**Objective**: Create a standardized way to parse data streams safely.

- [ ] **Define `StreamScanner`**:
    ```c
    typedef struct {
        const unsigned char* ptr;
        size_t len;
        size_t pos;
    } StreamScanner;
    ```
- [ ] **Implement Helper Functions**:
    - `Stream_Peek(scanner)`: Safe peek.
    - `Stream_Consume(scanner)`: Safe advance.
    - `Stream_ReadInt(scanner)`: Parse integer safely.
    - `Stream_Expect(scanner, char)`: Verify expected character.
    - Ensure all helpers check `if (pos >= len)` and handle bounds gracefully.

### Task 7.2: Refactor ReGIS and Parsing Logic
**Objective**: Apply safe primitives to critical areas.

- [ ] **Refactor `ProcessReGISChar`**:
    - Replace raw pointer arithmetic (e.g., `ptr++`, `while(*ptr)`) with `StreamScanner` functions.
- [ ] **Refactor Soft Font Loading**:
    - In `ProcessSoftFontDownload` (or `ProcessDCSChar` handling `DECDLD`), ensure font data parsing respects buffer limits.
- [ ] **Harden Escape Buffers**:
    - In `ProcessCSIChar`, `ProcessOSCChar`, `ProcessDCSChar`:
    - Check if `active_session->escape_pos` exceeds `MAX_COMMAND_BUFFER` before appending.
    - Implement truncation or error logging on overflow.

## Phase 8: Feature Polish (BiDi & Unicode)

**Goal**: Cleanup strict Unicode/BiDi handling.

### Task 8.1: Unicode Strictness
**Objective**: Ensure invalid UTF-8 sequences do not corrupt state.

- [ ] **Update Decoder**:
    - In `ProcessNormalChar` (or the UTF-8 state machine), check for invalid sequences (e.g., overlong encodings, unexpected continuation bytes).
    - Upon error, immediately emit the Replacement Character (U+FFFD) and reset the decoder state.
- [ ] **Visual Feedback**:
    - Ensure `AllocateGlyph` specifically handles `U+FFFD` by rendering a distinct glyph (e.g., `?` inside a diamond/box) instead of a space or null.

## Recommended Execution Order
1. **Task 6.1 (Computed Gotos)**: Low hanging fruit, high impact on portability.
2. **Task 5.1 & 5.2 (Global State)**: The heaviest lift. Doing this early prevents merge conflicts later.
3. **Task 7.1 & 7.2 (Buffer Safety)**: Critical for security, but easier to implement once the `Terminal*` context is passed around correctly.
4. **Task 6.2 (Shaders)**: Quality of life improvement, can be done anytime.
