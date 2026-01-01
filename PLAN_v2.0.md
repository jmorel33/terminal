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

- [ ] **Implement `ExecuteDECSASD` (Select Active Status Display)**:
    - [ ] Handle `CSI Ps $ }`.
    - [ ] `Ps=0`: Main display. `Ps=1`: Status line.
- [ ] **Implement `ExecuteDECSSDT` (Select Split Definition)**:
    - [ ] Handle `CSI Ps $ ~`.
    - [ ] `Ps=0`: No split.
    - [ ] `Ps=1`: Horizontal split.
    - [ ] Map to `SetSplitScreen(true/false, ...)` logic.
- [ ] **Refine `SetSplitScreen`**:
    - [ ] Add logic to define split point (default center or parameterized).
- [ ] **Verification**:
    - [ ] Send `CSI 1 $ ~` and visually verify (or check `terminal.split_screen_active` state) that the screen splits.

### Task 1.4: Multi-Session Enable/Disable (CSI ? 64 h/l)
*Allow host to lock/unlock multi-session capability.*

- [ ] **Add Mode Flag**: Add `bool multi_session_mode` to `VTFeatures` or `Terminal` struct.
- [ ] **Update `ExecuteSM`/`ExecuteRM`**:
    - [ ] Handle private parameter `64`.
    - [ ] On `h`: Enable session switching logic.
    - [ ] On `l`: Force switch to Session 1, disable `ExecuteDECSN` handling.
- [ ] **Verification**:
    - [ ] Send `CSI ? 64 l`, then attempt `CSI 2 ! ~` and verify session does *not* switch.

---

## Phase 2: Per-Session State & Input Robustness (1 week)

**Objective**: Ensure every session maintains independent state (cursors, modes, stacks) and input is correctly routed even when targeted remotely.

### Task 2.1: Per-Session Save/Restore Stacks (DECSC/DECRC)
*Fix the hole where `ESC 7` / `ESC 8` might overwrite a global shared state.*

- [ ] **Verify `TerminalSession` Struct**:
    - [ ] Confirm `saved_cursor` is present in `TerminalSession`.
- [ ] **Refactor Save/Restore Functions**:
    - [ ] Ensure `SaveCursor()` uses `ACTIVE_SESSION.saved_cursor`.
    - [ ] Ensure `RestoreCursor()` uses `ACTIVE_SESSION.saved_cursor`.
- [ ] **Add Extended State Saving**:
    - [ ] Ensure `saved_cursor` struct includes `origin_mode`, `attribute` flags, and `charset` state.
- [ ] **Verification**:
    - [ ] Save cursor in Session 1, move cursor, switch to Session 2, save/move/restore. Switch back to Session 1 and restore. Verify both restored correctly.

### Task 2.2: Input Routing Protocol
*Allow host to pipe input to a specific session without switching the active view.*

- [ ] **Enhance `PipelineWriteCharToSession`**:
    - [ ] Ensure it writes to the target session's `input_pipeline`.
    - [ ] Verify `ProcessPipeline` iterates ALL sessions.
- [ ] **Verification**:
    - [ ] Use a test harness to inject characters into Session 2's pipeline while Session 1 is active. Verify Session 2's screen buffer updates.

### Task 2.3: Session-Aware Locator & Printer
*Fix the "global default" behavior of auxiliary devices.*

- [ ] **Printer Controller**:
    - [ ] Ensure `ProcessPrinterControllerChar` uses `ACTIVE_SESSION` buffer.
- [ ] **Mouse/Locator**:
    - [ ] Update `DECRQLP` (Request Locator Pos) to report coordinates relative to the *session's* scrolling region.
    - [ ] Update `ExecuteDECSLE` to set `ACTIVE_SESSION.locator_events`.
- [ ] **Verification**:
    - [ ] Enable printer controller in Session 1. Send data. Switch to Session 2. Verify Session 2 does not capture printer data unless enabled there.

### Task 2.4: Independent Glyph Caches & Charsets
*Ensure Session 1 (Greek) and Session 2 (Line Draw) don't corrupt each other's rendering.*

- [ ] **Review `TranslateCharacter`**:
    - [ ] Confirm `ACTIVE_SESSION.charset` is used.
- [ ] **Split Rendering Logic**:
    - [ ] Refactor `UpdateTerminalSSBO` to explicitly accept `TerminalSession*` for the row being processed.
    - [ ] Ensure `GetScreenCell` calls in the loop use the correct session pointer.
- [ ] **Verification**:
    - [ ] Set Session 1 to G0=ASCII, Session 2 to G0=Special Graphics. Render split screen and verify character mapping is correct for each half.

---

## Phase 3: xterm/Modern + Rendering Polish (1 week)

**Objective**: Modernize the experience for xterm applications running in split sessions.

### Task 3.1: Session-Specific xterm Extensions
- [ ] **OSC 0/1/2 (Title/Icon)**:
    - [ ] Ensure `VTSituationSetWindowTitle` updates `ACTIVE_SESSION.title`.
    - [ ] Update window title logic to reflect the active session or split state.
- [ ] **OSC 9 (Notifications)**:
    - [ ] Implement `ExecuteOSC` case `9`.
    - [ ] Trigger notification callback with `active_session` ID.
- [ ] **Mouse Focus in Splits**:
    - [ ] Update `UpdateMouse`:
        - [ ] Calculate which session viewport is under the mouse cursor.
        - [ ] Route click/scroll events to that session.
- [ ] **Verification**:
    - [ ] Send OSC 0 to Session 1 and Session 2. Check internal `window_title` struct for each.

### Task 3.2: Scrollback & Dirty Rows in Splits
*Fix visual glitches when scrolling history in a split view.*

- [ ] **Refine `ScrollUpRegion`**:
    - [ ] Ensure it strictly respects `scroll_top` and `scroll_bottom` of the specific session.
- [ ] **Per-Session Scrollback View**:
    - [ ] Ensure `Shift+PageUp/Down` targets the *focused* session in the split.
- [ ] **Verification**:
    - [ ] Fill scrollback in Session 1. Split screen. Scroll back in Session 1. Verify Session 2 remains static.

### Task 3.3: Visual Bell Per Session
- [ ] **Update `TerminalPushConstants`**:
    - [ ] Add `float visual_bell_intensity;` to `TerminalPushConstants` struct in `terminal.h`.
- [ ] **Update `DrawTerminal`**:
    - [ ] Pass `ACTIVE_SESSION.visual_bell_timer` to the shader via the new push constant.
- [ ] **Update Shader**:
    - [ ] Modify `TERMINAL_SHADER_BODY` to use the new push constant for the flash effect.
- [ ] **Verification**:
    - [ ] Trigger bell (`\a`) in Session 1. Verify visual flash.

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
- [ ] **DCS Passthrough**:
    - [ ] Implement `DCS p` (Gateway Protocol placeholder).
    - [ ] Create stub `ParseGatewayCommand(const char* data, size_t len)`.
- [ ] **Verification**:
    - [ ] Verify `ParseGatewayCommand` exists via `grep` and test compilation.

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
