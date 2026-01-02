# Refined Improvement Plan (Mock-Driven Verification)

This plan focuses on verifying and refining core terminal features (Sixel, Scrollback, Printer, Locator) using mock-driven unit tests to bypass external dependencies (`situation.h`).

**Goal**: Achieve v1.6-v1.8 stability and feature completeness with high confidence through isolated testing.
**Timeline**: 3-4.5 weeks.

---

## Phase 1: Core Protocol Refinement & Verification (0.5–1 week)

**Objective**: Verify "Missed Things" (Sixel, Scrollback, Printer, Locator) are correctly implemented and integrated with session management.

### Task 1.1: Sixel Rendering Verification
*Verify Sixel parsing, compute shader data generation, and session state.*
- [ ] **Test Harness**: Create `tests/test_sixel.c` using `mock_situation.h`.
- [ ] **Verify Parsing**: Send raw Sixel data (`DCS q ... ST`) and assert `ACTIVE_SESSION.sixel.strips` are populated correctly.
- [ ] **Verify Compute Setup**: Mock `SituationUpdateBuffer` and assert Sixel data is "uploaded".
- [ ] **Refinement**: If "compute decode" is missing or buggy, fix `ProcessSixelChar` or shader logic.

### Task 1.2: Scrollback Ring Buffer & Session Integration
*Verify the ring buffer logic works per-session and handles wrapping/view offsets correctly.*
- [ ] **Test Harness**: Create `tests/test_scrollback.c`.
- [ ] **Verify Ring Logic**: Simulate filling buffer beyond `DEFAULT_TERM_HEIGHT`. Assert `GetScreenRow` returns correct pointers for scrolled history.
- [ ] **Verify Session Isolation**: Fill Session 1, leave Session 2 empty. Verify Session 2's buffer remains clean.
- [ ] **Verify View Offset**: Test `scroll_up` / `scroll_down` logic affects `view_offset` and `GetScreenRow` correctly.

### Task 1.3: Full Printer Controller (MC) Logic
*Verify `CSI 4 i`, `CSI 5 i`, and especially `CSI ? 9 i` (Print Screen).*
- [ ] **Test Harness**: Create `tests/test_printer.c`.
- [ ] **Verify Controller Mode**: Send `CSI 5 i`, write text, verify `printer_callback` receives it and screen does not. Send `CSI 4 i` to exit.
- [ ] **Verify Print Screen**: Send `CSI ? 9 i` (or equivalent MC). Mock `GetActiveScreenCell` iteration and assert `printer_callback` receives full screen dump.
- [ ] **Fix Missing**: If `ExecuteMC` doesn't handle `? 9 i`, implement it.

### Task 1.4: ANSI Text Locator (DECRQLP)
*Verify Locator input reporting and event handling.*
- [ ] **Test Harness**: Create `tests/test_locator.c`.
- [ ] **Verify Request**: Send `CSI ? 12 n` or `DECRQLP` sequence. Assert correct response format (including session split offset logic).
- [ ] **Verify Filter**: Test `DECSLE` sets `locator_events` flags correctly.

---

## Phase 2: Internationalization (Intl) Refinement (1 week)

**Objective**: Expand Unicode support and verify BiDi/NRCS.

### Task 2.1: Supplementary Unicode & LRU Cache
- [ ] **Test Harness**: `tests/test_unicode.c`.
- [ ] **Verify LRU**: Simulate filling the Glyph Atlas. Verify least recently used glyphs are evicted/replaced.
- [ ] **Verify Supplementary**: Send non-BMP characters (if supported) or edge cases.

### Task 2.2: Basic BiDi Verification
- [ ] **Test Harness**: `tests/test_bidi.c`.
- [ ] **Verify Reordering**: Inject Hebrew/Arabic text. Verify `UpdateTerminalRow` reorders characters visually (visual order vs logical order).

### Task 2.3: Character Set Tests (GR/SS2/SS3)
- [ ] **Test Harness**: `tests/test_charsets.c`.
- [ ] **Verify Locking Shifts**: Test `LS0`-`LS3`.
- [ ] **Verify Single Shifts**: Test `SS2`/`SS3` affecting next character only.

---

## Phase 3: Gateway Protocol Full Implementation (1–1.5 weeks)

**Objective**: Implement and verify the Gateway Protocol (DCS custom hooks) for game/app integration.

### Task 3.1: Gateway Parser & Class A (MAT)
- [ ] **Implement**: `ParseGatewayCommand` in `terminal.h`.
- [ ] **Verify**: Test parsing of `DCS G A ... ST` (Material updates).

### Task 3.2: Class C (LOG) & WizVM
- [ ] **Implement**: WizVM hooks or logic within Gateway parser.
- [ ] **Verify**: Mock logic ticks.

### Task 3.3: Classes B/D/E (GEO/SCN/SON)
- [ ] **Implement**: Geometry, Scene, Sound commands.
- [ ] **Verify**: Mock resource updates.

### Task 3.4: Macros & Handshake
- [ ] **Implement**: Record/Play macros via Gateway. Handshake/Discovery DCS.
- [ ] **Verify**: Full handshake loop with mock host.
