# K-Term Bug Fix Plan

This document tracks issues found while repairing the K-Term console path and building the K-Term test harness. It is organized by phases so the remaining work can be followed without reading a chronological log.

## Current Status

- **Completion date:** `2026/05/17`
- **Default command:** `cmd /c "sit\k-term\tests\compile_focused_tests.bat && sit\k-term\tests\run_focused_tests.bat --no-color"`
- **Latest default result:** 258 total, 258 passed, 0 failed, 0 skipped.
- **Default modules:** `terminal_core`, `parser`, `attributes`, `verification`, `graphics`, `integration`, `advanced_grid`, `vt_protocols`, `terminal_render_buffer`, `font_atlas`, `display_consistency`, `protocol_response`, `serialization`, `security_hardening`, `io`, `fuzz_stress`, `gateway`, `voice`.
- **Optional networking result:** `KTERM_OPTIONAL_NET=1` compile succeeds, and focused runs report 270 total passing tests, including 11 `networking` tests and 15 `gateway` tests.
- **Backlog consolidation:** `v2.7.12` verifies that the remaining historical `v2.7.13+` reserve work was absorbed by earlier rebased patches rather than left as duplicate future security, networking, or performance work.

## Phase 0: Harness Foundation And Environment

Goal: make K-Term tests runnable as one Situation-style suite in this checkout.

- [x] Replace scattered standalone executables with one harness entry point.
- [x] Add `kterm_test_main.c`, `kterm_test_registry.c`, and `kterm_test_modules.c`.
- [x] Keep white-box modules in one `KTERM_IMPLEMENTATION` translation unit.
- [x] Update the local Situation mock shape for current handles and API stubs.
- [x] Link `tinycthread.c` in the focused compile script.
- [x] Replace stale verification field assumptions with current invariants.

Evidence:

- Standalone test binaries were replaced by a registered module harness.
- `KTERM_IMPLEMENTATION` now lives once in `kterm_test_modules.c`.
- Current harness compiles and runs from the local Windows/MSYS2 setup.

## Phase 1: Parser And VT Protocol Fixes

Goal: make parser state transitions and VT protocol reports deterministic and host-observable.

- [x] Fix CSI parser state reset after intentional transitions.
- [x] Fix `CSI > c` secondary DA routing.
- [x] Fix `CSI = c` tertiary DA routing.
- [x] Fix XTGETTCAP DCS dispatch so `DCS + q` is not claimed by Sixel.
- [x] Fix DECUDK payload dispatch after `DCS |`.
- [x] Fix 8-bit C1 parser control dispatch after S8C1T.
- [x] Add parser-owned tests for interrupted strings, buffer truncation, strict malformed recovery, and feature-specific states.
- [x] Add exact protocol response coverage for DA, DSR, DECRQM, DECRQSS, XTGETTCAP, and malformed report requests.
- [x] List unsupported, partial, and stubbed protocol features that currently have host-visible behavior.
- [x] Add negative tests for unsupported feature responses or no-op behavior.
- [x] Add strict-mode malformed non-report sequence tests.
- [x] Add permissive-mode recovery tests for malformed non-report sequences.

Evidence:

- `vt_protocols.tektronix_and_vt52_protocols` caught the CSI reset bug.
- `vt_protocols.da_dsr_decrqm_response_payloads` caught secondary DA routing.
- `vt_protocols.decrqss_and_xtgettcap_response_payloads` caught XTGETTCAP dispatch.
- `vt_protocols.decudk_decdmac_decdld_behavior` caught DECUDK payload handling.
- `parser.8bit_c1_csi_input`, `parser.8bit_c1_osc_input`, and `parser.8bit_c1_dcs_input` cover C1 controls.
- Unsupported/partial host-visible behavior is explicitly owned as: APC/PM/SOS execute as no-op strings with unsupported accounting when diagnostics are enabled; malformed non-report `ED`/`EL` recover in strict and permissive modes; unknown report requests return documented negative/no-op responses in `protocol_response`.
- `parser.unsupported_string_protocol_noops_are_accounted`, `parser.malformed_non_report_sequence_strict_mode_recovers`, and `parser.malformed_non_report_sequence_permissive_mode_recovers` cover these negative paths.
- `protocol_response` now owns 21 exact-payload tests.

## Phase 2: Harness Ownership And Legacy Migration

Goal: make every legacy or ad hoc test source either migrated, optional, support-only, benchmark-only, or explicitly excluded.

- [x] Classify legacy standalone tests and remove them as the organizing model.
- [x] Migrate parser stream token/peek and signed-param checks.
- [x] Migrate DECSCA and protected-field checks.
- [x] Migrate serialization round-trip and rejection checks.
- [x] Migrate security hardening and queued resize/copy regression coverage.
- [x] Keep optional networking, optional voice, support-only, and benchmark files classified in the harness plan.

Evidence:

- Phase 12 in `kterm_test_harness_plan.md` maps old K-Term test files to migrated, support-only, optional/manual, benchmark-only, or reviewed/folded status.

## Phase 3: IO, Gateway, Sessions, And Public API Contracts

Goal: make embedder-facing behavior stable at event, callback, gateway, and session boundaries.

- [x] Fix `KTERM_EVENT_MOUSE` return behavior.
- [x] Fix resize callback ordering so callbacks fire when queued resize operations apply.
- [x] Fix output sink/callback ordering when switching sinks with queued response data.
- [x] Add integration tests for key, mouse, focus, paste, and callback ordering.
- [x] Add gateway tests for command scanning and case-insensitive built-in dispatch.
- [x] Add gateway tests for extension registration and extension callback responses.
- [x] Add gateway tests for malformed commands, tokenizer threading, locking, runtime configuration, target sessions, and auth ownership.

Evidence:

- `io.mouse_event_processing`, `io.resize_event_processing`, and `io.callback_ordering` cover the fixed public contracts.
- `integration` now has 15 tests.
- `gateway` now has 14 default tests and 15 optional-networking tests, including wrapped/non-wrapped Grid FillSpan out-of-bounds coverage, optimized font listing coverage, and networking-gated WHOIS output coverage.

## Phase 4: Grid, Attributes, And Display State

Goal: prevent stale attributes, broken grid invariants, and inconsistent display state before rendering.

- [x] Add per-test fresh terminal helpers where module setup leaked state.
- [x] Add advanced grid coverage for rectangular selective erase.
- [x] Add protected erase behavior coverage.
- [x] Add forms-mode protected-field skip coverage.
- [x] Add wide UTF-8 and combining UTF-8 grid layout coverage.
- [x] Add alternate-screen plus scrollback metadata coverage.
- [x] Add SGR strikethrough color handling.
- [x] Add remaining attribute flag assertions.
- [x] Add protected-cell and selective erase attribute coverage.
- [x] Add `XTPUSHSGR` and `XTPOPSGR` stack restoration coverage.
- [x] Make queued full-screen scroll update scrollback history metadata.
- [x] Add a queued/public-path scrollback regression for `history_rows_populated`.
- [x] Move terminal-core scrollback tests from internal helper reliance to the queued public path.
- [x] Keep internal scrollback metadata coverage in place until the queued path is fixed.

Evidence:

- `advanced_grid` now has 18 tests.
- `attributes` now has 17 tests.
- `KTerm_ApplyScrollOp()` now updates `history_rows_populated`, clamps `view_offset`, and marks the viewport dirty for queued full-screen scrolls.
- `terminal_core.scrollback_head_moves_after_full_screen_scroll` and `terminal_core.scrollback_history_population` now use the queued public `KTerm_ScrollUpRegion()` path.

## Phase 5: Serialization And Persistent State

Goal: make save/restore preserve persistent terminal state, not just visible cells.

- [x] Add serialized `version` and `header_size` validation.
- [x] Persist DEC/ANSI modes.
- [x] Persist current colors and attributes.
- [x] Persist window/icon titles.
- [x] Persist tab-stop metadata and payload.
- [x] Persist left/right margins.
- [x] Persist scrollback head/view metadata.
- [x] Add rejection tests for bad magic, truncated payload, unsupported version, invalid header ranges, and dimension mismatch.

Evidence:

- `serialization` now has 8 tests covering round-trip and corrupt input paths.

## Phase 6: Security, Hardening, And Fuzz

Goal: keep hostile input, malformed payloads, and bounded stress paths covered in the default harness.

- [x] Add long CSI overflow handling coverage.
- [x] Add long OSC/DCS/APC/PM/SOS string handling coverage.
- [x] Add malformed UTF-8 handling coverage.
- [x] Add public API null/empty argument coverage where permitted.
- [x] Add deterministic fuzz seed corpus.
- [x] Add bounded random byte-stream parser fuzzing.
- [x] Add scrollback stress.
- [x] Add resize-buffer stress on a safe session-resize path.
- [x] Add Sixel, Kitty, and OSC payload stress.
- [x] Fix the public `KTerm_Resize()` lock/layout hang.
- [x] Isolate a minimal public `KTerm_Resize()` hang repro that can be run safely without wedging the default harness.
- [x] Audit lock ordering between `term->lock`, layout recalculation, resize callbacks, and queued resize operations.
- [x] Add a bounded public-resize regression test after the lock/layout fix.
- [x] Move stress/display resize coverage back toward the public resize path once it is stable.
- [x] Keep session-resize stress/display invariants covered until the public resize path is fixed.

Evidence:

- `security_hardening` now has 15 tests, including oversized DCS answerback, function-key/event, title truncation, and shared client string-copy coverage.
- The SSH/Telnet user-string null-termination backlog is covered by `kt_client_string.h` helper tests and by audits showing the optional clients no longer use raw fixed-buffer `strcpy`/`strncpy`/`strcat`/`strncat`/`sprintf` calls.
- `fuzz_stress` now has 7 tests.
- Public `KTerm_Resize()` deadlocked because it held `compositor.render_lock` before calling `KTermCompositor_Resize()`, which attempted to lock the same non-recursive mutex again.
- `fuzz_stress.resize_stress` and `display_consistency.scroll_and_resize_frame_keeps_render_cells_initialized` now use the public `KTerm_Resize()` path and pass.

## Phase 7: Optional Modules And Platform Portability

Goal: make optional feature builds testable without destabilizing the default harness.

- [x] Add default voice tests for enable/disable, capture packet format, playback, command injection, VAD activation/reset, and VoIP stub contract.
- [x] Fix voice context slot exhaustion caused by passive compositor reads.
- [x] Reuse disabled voice context slots and clear session/term binding on disable.
- [x] Add optional networking tests for protocol catalog lookup, auth metadata, media/discovery ports, context configuration, connect status, traceroute context initialization, PacketDiag build contract, auth scan contract, and disconnect.
- [x] Fix optional networking build portability on Windows by adding a `gettimeofday()` shim and guarding port-scan timestamps.
- [x] Keep live networking and external services manual/gated.

Evidence:

- `voice` now has 6 tests in the default harness.
- Optional `networking` has 11 tests under `KTERM_OPTIONAL_NET=1`, including Speedtest phase-guard and MTU local/fallback initialization coverage.

## Phase 8: Font Atlas, Encoding, And Box Drawing

Goal: protect the exact contracts behind box drawing, CP437, DEC Special Graphics, and font padding.

- [x] Preserve intentional 10x10 terminal cells with centered 8x8 IBM glyph padding.
- [x] Verify built-in IBM glyph width and height.
- [x] Verify built-in and soft-font atlas slot calculations.
- [x] Verify DEC Special Graphics mappings.
- [x] Verify CP437 box glyph atlas slots.
- [x] Verify shader push constants expose the expected font data dimensions.
- [x] Test DEC Special Graphics box-drawing mappings through parser and render-buffer paths.
- [ ] Add GPU texture upload assertions for font atlas dimensions and byte layout.
- [ ] Add backend shader sampling checks that prove render shaders use the same atlas dimensions as the CPU contract.
- [ ] Add a visual/backend smoke case for box-drawing continuity once backend inspection is available.
- [x] Decide whether public byte APIs are UTF-8-only, CP437-capable, or mode-gated.
- [x] Document the chosen byte/encoding contract for console text.
- [x] Add tests for the chosen raw CP437 behavior or explicit rejection/replacement behavior.
- [x] Update console strings to use the documented contract consistently.

Evidence:

- `font_atlas` now has 13 tests.
- Public write APIs are documented as VT byte streams: UTF-8 text after `ESC % G`, DEC Special Graphics for legacy line drawing, and no raw CP437 text-input contract.
- `vt_protocols.utf8_box_drawing_maps_to_cp437_glyph_ids` and `vt_protocols.utf8_mode_rejects_raw_cp437_box_bytes` pin the chosen behavior.
- `examples/kterm_console.c` uses UTF-8 mode (`ESC % G`) for the welcome banner. *(Legacy `console.c` removed; see CONSOLE_MERGE_DEPRECATION_PLAN.)*
- `terminal_render_buffer` and `display_consistency` cover DEC Special Graphics render output.
- Raw CP437 through the UTF-8 parser is explicitly replacement behavior, not a hidden box-drawing path.

## Phase 9: Render Buffer And Display Consistency

Goal: trap white-block, stale-attribute, dirty-row, and graphics-exit display issues before GPU presentation.

- [x] Test default clear cell conversion.
- [x] Test ASCII, DEC Special Graphics, and CP437 glyph mapping.
- [x] Test 16-color, 256-color, and truecolor conversion.
- [x] Test reverse, conceal, blink, bold, faint, italic, underline, and strike flags.
- [x] Test dirty row propagation.
- [x] Test full-frame rebuild behavior.
- [x] Test cleared and written cells never emit uninitialized fg/bg render data.
- [x] Add `display_consistency` module for cross-layer visual invariants.
- [x] Test realistic mixed display sequences never emit uninitialized `GPUCell` fields.
- [x] Test clear-after-white foreground/background restores transparent default background.
- [x] Test DEC box drawing survives color reset and render-buffer update.
- [x] Test resize-facing render-buffer rebuild keeps cells initialized.
- [x] Test alternate-screen swap restores main-buffer render content.
- [x] Test graphics protocol exit allows normal text rendering afterward.
- [ ] Add GPU buffer upload tests that compare CPU `GPUCell` data to uploaded backend buffer contents.
- [ ] Add shader-side interpretation checks for fg/bg alpha and reverse/opaque background behavior.
- [ ] Add backend presentation diagnostics for any remaining white-block reports.

Evidence:

- `terminal_render_buffer` now has 14 tests.
- `display_consistency` now has 6 tests.
- CPU cell to `GPUCell` conversion and realistic display consistency pass headlessly; remaining white-block root-cause area is GPU upload/shader/backend presentation.

## Phase 10: Diagnostics And Verification

Goal: keep diagnostics, debug reporting, status, and verification behavior owned by the harness.

- [x] Add tertiary DA exact response coverage.
- [x] Add diagnostics/debug sequence option behavior coverage.
- [x] Add debug callback payload sanitization.
- [x] Add diagnostics text generation coverage.
- [x] Fold legacy verification suite checks.
- [x] Classify optional network diagnostics separately from default-safe diagnostics.

Evidence:

- `verification` now has 15 tests.

## Completion Criteria

- [x] Default harness passes: 258 total, 258 passed, 0 failed, 0 skipped.
- [x] Optional networking compile and module pass under `KTERM_OPTIONAL_NET=1`.
- [x] Every known fixed issue has coverage or a documented harness ownership path.
- [ ] All open actionables above are fixed or explicitly reclassified.
