# K-Term Test Harness Plan

Completion Date: `2026/05/17`

Goal: all K-Term library behavior is tested through our own Situation-style harness. Every actionable below has its own checkbox. Mark `[x]` only when the test/code/docs change is implemented and the relevant harness run passes.

Current baseline: 258 tests pass across `terminal_core`, `parser`, `attributes`, `verification`, `graphics`, `integration`, `advanced_grid`, `vt_protocols`, `terminal_render_buffer`, `font_atlas`, `display_consistency`, `protocol_response`, `serialization`, `security_hardening`, `io`, `fuzz_stress`, `gateway`, and `voice`. `v2.7.12` is a consolidation pass, so this baseline is unchanged while the rebased security coverage is reverified.

Default command:

```bat
cmd /c "sit\k-term\tests\compile_focused_tests.bat && sit\k-term\tests\run_focused_tests.bat --no-color"
```

## Phase 0: Harness Foundation

- [x] Create `sit/k-term/tests/kterm_test_main.c` as the harness entry point.
- [x] Create `sit/k-term/tests/kterm_test_registry.c` as the module registry.
- [x] Create `sit/k-term/tests/kterm_test_modules.c` as the white-box implementation/module unit.
- [x] Add `compile_focused_tests.bat` for the local K-Term harness build.
- [x] Add `run_focused_tests.bat` for the local K-Term harness run.
- [x] Register `terminal_core`.
- [x] Register `parser`.
- [x] Register `attributes`.
- [x] Register `verification`.
- [x] Register `graphics`.
- [x] Register `integration`.
- [x] Register `advanced_grid`.
- [x] Register `vt_protocols`.
- [x] Verify the current baseline: 61 total, 61 passed, 0 failed, 0 skipped.
- [x] Rename the scripts from `focused_*` to default K-Term harness names, or document why the focused name remains.
- [x] Document the required steps for adding a new harness module.
- [x] Document the white-box module-unit rule for tests that inspect private K-Term internals.
- [x] Add a convention that old standalone `main()` tests are not the preferred path.
- [x] Add a short harness overview in `sit/k-term/tests`.

Completion criteria: new modules can be added predictably, the default command works from a clean checkout, and standalone executables are no longer the organizing model.

## Phase 1: Complete Existing Modules

### `terminal_core`

- [x] Test scroll-down basic behavior.
- [x] Test scroll-down with margins.
- [x] Test scroll-down partial regions.
- [x] Test scroll-down overscroll clearing.
- [x] Test scroll-up basic behavior.
- [x] Test scroll-up with margins.
- [x] Test scroll-up partial regions.
- [x] Test scroll-up overscroll clearing.
- [x] Test insert-line behavior inside scroll margins.
- [x] Test delete-line behavior inside scroll margins.
- [x] Test cursor save/restore with origin mode disabled.
- [x] Test cursor save/restore with origin mode enabled.
- [x] Test cursor save/restore with horizontal margins.
- [x] Test scrollback ring-buffer head movement.
- [x] Test scrollback history population.

Completion criteria: scrolling, line mutation, cursor state, margins, and scrollback are covered with boundary cases.

### `parser`

- [x] Test basic CSI parameter parsing.
- [x] Test CSI default parameters.
- [x] Test CSI subparameters.
- [x] Test CSI garbage handling.
- [x] Test CSI parameter overflow protection.
- [x] Test stream identifier scanning.
- [x] Test stream boolean scanning.
- [x] Test direct input mode flag handling.
- [x] Test input pipeline smoke behavior.
- [x] Test 8-bit C1 CSI input if supported.
- [x] Test 8-bit C1 OSC input if supported.
- [x] Test 8-bit C1 DCS input if supported.
- [x] Test OSC termination by `BEL`.
- [x] Test OSC termination by `ST`.
- [x] Test DCS termination by `BEL`.
- [x] Test DCS termination by `ST`.
- [x] Test APC termination by `ST`.
- [x] Test PM termination by `ST`.
- [x] Test SOS termination by `ST`.
- [x] Test interrupted string parsing with nested `ESC`.
- [x] Test escape buffer truncation for CSI.
- [x] Test escape buffer truncation for OSC.
- [x] Test escape buffer truncation for DCS.
- [x] Test malformed parser input in permissive mode.
- [x] Test malformed parser input in strict mode.
- [x] Test unsupported APC/PM/SOS no-op accounting.
- [x] Test malformed non-report sequence recovery in strict mode.
- [x] Test malformed non-report sequence recovery in permissive mode.
- [x] Test entry and exit for the currently reachable `VTParseState` values in this module.
- [x] Test entry and exit for remaining `VTParseState` enum values that require feature-specific payloads.

Completion criteria: each parser state has success, malformed, termination, and bounds coverage.

### `attributes`

- [x] Test basic bold SGR.
- [x] Test italic SGR.
- [x] Test underline SGR.
- [x] Test blink SGR.
- [x] Test reverse SGR flag setup.
- [x] Test conceal SGR.
- [x] Test strike SGR.
- [x] Test attribute reset cascade.
- [x] Test standard 16-color foreground SGR.
- [x] Test standard 16-color background SGR.
- [x] Test 256-color foreground SGR.
- [x] Test 256-color background SGR.
- [x] Test truecolor foreground SGR.
- [x] Test truecolor background SGR.
- [x] Test underline color.
- [x] Test underline style single.
- [x] Test underline style double.
- [x] Test underline style curly.
- [x] Test underline style dotted.
- [x] Test underline style dashed.
- [x] Test strikethrough color.
- [x] Test overline.
- [x] Test framed text.
- [x] Test encircled text.
- [x] Test superscript.
- [x] Test subscript.
- [x] Test reverse-video cell write semantics.
- [x] Test protected cell attributes.
- [x] Test selective erase preserving protected cells.
- [x] Test `XTPUSHSGR` if supported.
- [x] Test `XTPOPSGR` if supported.

Completion criteria: every `KTERM_ATTR_*` flag and every color mode has a cell-level assertion.

### `verification`

- [x] Test resource invariants.
- [x] Test basic CSI movement coverage.
- [x] Test error callback smoke behavior.
- [x] Test JIT operation smoke behavior.
- [x] Test cursor status reporting state.
- [x] Test refactor validation smoke behavior.
- [x] Test primary DA exact response.
- [x] Test secondary DA exact response.
- [x] Test tertiary DA exact response.
- [x] Test DSR status exact response.
- [x] Test DSR cursor-position exact response.
- [x] Test DECRQM exact response.
- [x] Test unsupported sequence counter increments.
- [x] Test `last_unsupported` content.
- [x] Test diagnostics/debug-sequence option behavior.
- [x] Fold missing checks from `test_verification_suite.c`.
- [x] Fold missing checks from `verify_diagnostics.c`.

Completion criteria: reports, diagnostics, and unsupported-sequence accounting are asserted by exact observable output or state.

### `graphics`

- [x] Test Sixel parser entry and repeat processing.
- [x] Test compositor prepare/draw smoke path.
- [x] Test basic font-rendering cell write.
- [x] Test rectangle attribute smoke behavior.
- [x] Test grid out-of-bounds smoke behavior.
- [x] Test raw buffer smoke behavior.
- [x] Test shader configuration smoke behavior.
- [x] Test ReGIS parser smoke behavior.
- [x] Test Sixel raster attributes.
- [x] Test Sixel palette changes.
- [x] Test Sixel transparency behavior.
- [x] Test Sixel scrolling behavior.
- [x] Test Sixel reset behavior.
- [x] Test ReGIS line command behavior.
- [x] Test ReGIS polygon command behavior.
- [x] Test ReGIS text command behavior.
- [x] Test ReGIS reset behavior.
- [x] Test Tektronix vector generation.
- [x] Test Tektronix reset behavior.
- [x] Test Kitty image upload.
- [x] Test Kitty image chunking.
- [x] Test Kitty delete actions.
- [x] Test Kitty placement.
- [x] Test Kitty memory limits.

Completion criteria: each graphics protocol has parser, state, resource, and reset coverage.

### `integration`

- [x] Test public write API smoke behavior.
- [x] Test basic decoupling smoke behavior.
- [x] Test tab-stop smoke behavior.
- [x] Test threading smoke behavior.
- [x] Test safety smoke behavior.
- [x] Test active-session smoke behavior.
- [x] Test public API without private internal assertions.
- [x] Test create/destroy/recreate lifecycle.
- [x] Test multi-session creation.
- [x] Test active-session switching.
- [x] Test active-session isolation.
- [x] Test resize callback behavior.
- [x] Test session resize invariants.
- [x] Test key event queue processing.
- [x] Test mouse event queue processing.
- [x] Test focus event queue processing.
- [x] Test paste event queue processing.
- [x] Test byte event queue processing.
- [x] Test callback ordering.

Completion criteria: K-Term has both black-box public API coverage and white-box state coverage.

### `advanced_grid`

- [x] Test insert/delete character row mutation.
- [x] Test erase character with active attributes.
- [x] Test top/bottom margins.
- [x] Test left/right margins.
- [x] Test origin mode with margins.
- [x] Test alternate-screen isolation.
- [x] Test tab stop clearing and navigation.
- [x] Test DEC line attributes.
- [x] Test rectangular copy.
- [x] Test rectangular fill.
- [x] Test rectangular erase.
- [x] Test rectangular selective erase.
- [x] Test rectangular reverse attributes.
- [x] Test protected field write behavior.
- [x] Test protected field erase behavior.
- [x] Test form-skip behavior.
- [x] Test wide-character grid layout.
- [x] Test combining-character grid layout.
- [x] Test dirty rectangle propagation.
- [x] Test row dirty persistence across frames.
- [x] Test scrollback plus alternate-screen isolation.

Completion criteria: grid behavior is covered for mutation, protection, margins, history, width, attributes, and dirty state.

### `vt_protocols`

- [x] Test VT level feature matrix.
- [x] Test core DEC/xterm private modes.
- [x] Test mouse protocol modes.
- [x] Test charset designation.
- [x] Test DEC Special Graphics.
- [x] Test UTF-8 box drawing maps to CP437 glyph IDs.
- [x] Test raw CP437 box bytes are rejected/replaced in UTF-8 mode.
- [x] Test OSC title commands.
- [x] Test OSC color commands.
- [x] Test APC terminator behavior.
- [x] Test PM terminator behavior.
- [x] Test SOS terminator behavior.
- [x] Test Sixel DCS entry.
- [x] Test ReGIS DCS entry.
- [x] Test Tektronix parser transition.
- [x] Test VT52 parser transition.
- [x] Test Kitty graphics entry.
- [x] Test Kitty keyboard flag handling.
- [x] Test every supported DECSET mode.
- [x] Test every supported DECRST mode.
- [x] Test DA response payloads.
- [x] Test DSR response payloads.
- [x] Test DECRQM response payloads.
- [x] Test DECRQSS response payloads.
- [x] Test XTGETTCAP response payloads.
- [x] Test DECUDK behavior.
- [x] Test DECDMAC behavior.
- [x] Test DECDLD behavior.
- [x] Test locator protocol modes.
- [x] Test printer protocol modes.

Completion criteria: every protocol K-Term claims to support has state and response coverage.

## Phase 2: Terminal Render Buffer

- [x] Create `sit/k-term/tests/test_terminal_render_buffer_module.c`.
- [x] Add `extern const SitTestModule g_kterm_module_terminal_render_buffer` to `kterm_test_registry.c`.
- [x] Register `g_kterm_module_terminal_render_buffer` in `kterm_test_register_all()`.
- [x] Include `test_terminal_render_buffer_module.c` in `kterm_test_modules.c`.
- [x] Test default clear-cell conversion.
- [x] Test ASCII glyph `EnhancedTermChar -> GPUCell` mapping.
- [x] Test DEC Special Graphics glyph mapping into render cells.
- [x] Test CP437 glyph mapping into render cells.
- [x] Test standard 16-color fg/bg conversion.
- [x] Test 256-color fg/bg conversion.
- [x] Test truecolor fg/bg conversion.
- [x] Test reverse-video fg/bg conversion.
- [x] Test conceal conversion.
- [x] Test blink flag conversion.
- [x] Test bold flag conversion.
- [x] Test faint flag conversion.
- [x] Test italic flag conversion.
- [x] Test underline flag conversion.
- [x] Test strike flag conversion.
- [x] Test dirty row propagation into render data.
- [x] Test full-frame rebuild behavior.
- [x] Test cleared cells never emit uninitialized fg/bg.
- [x] Test written cells never emit uninitialized fg/bg.

Completion criteria: random white-block symptoms can be classified as CPU-cell, render-buffer/upload, or shader-output issues using harness evidence.

## Phase 3: Font Atlas And Glyph Contract

- [x] Create `sit/k-term/tests/test_font_atlas_module.c`.
- [x] Add `extern const SitTestModule g_kterm_module_font_atlas` to `kterm_test_registry.c`.
- [x] Register `g_kterm_module_font_atlas` in `kterm_test_register_all()`.
- [x] Include `test_font_atlas_module.c` in `kterm_test_modules.c`.
- [x] Test default terminal cell width is 10.
- [x] Test default terminal cell height is 10.
- [x] Test built-in IBM font glyph width is 8.
- [x] Test built-in IBM font glyph height is 8.
- [x] Test IBM glyphs are centered inside padded 10x10 cells.
- [x] Test built-in font atlas slot calculation.
- [x] Test soft-font atlas slot calculation.
- [x] Test built-in and soft-font atlas paths use the same slot contract.
- [x] Test DEC Special Graphics line glyphs map to expected CP437 glyph IDs.
- [x] Test CP437 box glyphs land in expected atlas slots.
- [x] Test shader push constants expose correct font data width.
- [x] Test shader push constants expose correct font data height.
- [x] Add a regression test for the atlas layout mismatch in `kterm_bug_fix_plan.md`.

Completion criteria: built-in font, soft font, CP437 glyphs, DEC Special Graphics, and shader sampling dimensions share one tested contract.

## Phase 4: Protocol Response Coverage

- [x] Add response-ring helper functions.
- [x] Add response-callback helper functions.
- [x] Test primary DA response for VT52.
- [x] Test primary DA response for VT100.
- [x] Test primary DA response for VT220.
- [x] Test primary DA response for VT320.
- [x] Test primary DA response for VT420.
- [x] Test primary DA response for VT525.
- [x] Test primary DA response for xterm mode.
- [x] Test secondary DA response for supported VT levels.
- [x] Test tertiary DA response where supported.
- [x] Test DSR terminal status response.
- [x] Test DSR cursor-position response.
- [x] Test DECRQM set-state response.
- [x] Test DECRQM reset-state response.
- [x] Test DECRQM unknown-mode response.
- [x] Test DECRQSS SGR response.
- [x] Test DECRQSS margin response.
- [x] Test XTGETTCAP known capability response.
- [x] Test XTGETTCAP unknown capability response.
- [x] Test malformed report requests in strict mode.

Completion criteria: host-observable report payloads are exact and stable.

## Phase 5: Legacy Suite Migration

- [x] Fold `test_attributes_modes_suite.c` into `attributes`, `advanced_grid`, and `vt_protocols`.
- [x] Fold `test_parser_suite.c` into `parser` and `vt_protocols`.
- [x] Fold `test_graphics_suite.c` into `graphics`, `font_atlas`, and `advanced_grid`.
- [x] Fold `test_integration_suite.c` into `integration`.
- [x] Fold `test_verification_suite.c` into `verification`.
- [x] Classify `test_gateway_suite.c` for the Phase 6 `gateway` module.
- [x] Classify `test_gateway_case.c` for the Phase 6 `gateway` module.
- [x] Classify `test_io_suite.c` for the Phase 7 `io` module.
- [x] Fold `test_serialize_suite.c` into `serialization`.
- [x] Classify `test_fuzz_suite.c` for the Phase 9 deterministic `fuzz_stress` module.
- [x] Classify `stress_tests.c` for the Phase 9 deterministic `fuzz_stress` module.
- [x] Fold `test_buffer_hardening.c` into `security_hardening`.
- [x] Classify `repro_vulnerability.c` for optional networking/security hardening.
- [x] Fold `test_decsca.c` into `attributes` and `advanced_grid`.
- [x] Classify `test_dissectors.c` for the Phase 10 optional networking/dissectors module.
- [x] Compare `verify_scroll_down.c` against `terminal_core`.
- [x] Migrate missing `verify_scroll_down.c` cases.
- [x] Classify `verify_diagnostics.c` for optional networking diagnostics.
- [x] Classify `verify_gateway_threading.c` for the Phase 6 `gateway` module.
- [x] Classify `verify_auth_fields.c` for optional networking/auth coverage.
- [x] Classify `verify_advanced_auth.c` for optional networking/auth and dissector coverage.
- [x] Classify `verify_traceroute.c` for optional networking.
- [x] Classify `verify_traceroute_continuous.c` for optional networking.
- [x] Decide ownership for `verify_voice.c`.
- [x] Decide ownership for `verify_voice_commands.c`.
- [x] Decide ownership for `verify_voip.c`.
- [x] Classify `benchmark_banner.c` as performance-only.
- [x] Classify `benchmark_diagnostics.c` as performance-only.
- [x] Classify `benchmark_net_alloc.c` as performance-only.
- [x] Keep `mock_pcap.c` as support code for networking/dissector tests.
- [x] Classify `test_networking_suite.c` for optional networking.
- [x] Classify `net_tests.c` for optional networking.
- [x] Document every migrated, optional, support-only, performance-only, or retired legacy file with a reason.

Legacy migration notes:

- `test_decsca.c` is now covered by `attributes.decsca_protected_attribute`, `attributes.decsca_clear_variants`, `advanced_grid.protected_cells_block_overwrite`, and `advanced_grid.selective_erase_respects_protection`.
- `test_parser_suite.c` added `parser.stream_match_token`, `parser.stream_peek_identifier`, `parser.signed_csi_params`, and `parser.strict_signed_csi_params_clamp`; its existing CSI, input pipeline, string, VT, and response cases were already covered by `parser`, `vt_protocols`, and `protocol_response`.
- `test_serialize_suite.c` is now covered by `serialization.session_round_trip_restores_cursor_cells_and_attributes`, plus bad magic, truncated payload, and dimension mismatch rejection.
- `test_buffer_hardening.c` is now covered by `security_hardening.queued_resize_before_wide_copy_rect_does_not_escape_buffer`.
- `verify_scroll_down.c` has no missing cases after comparison; `terminal_core` already covers basic, margins, partial, and overscroll scroll-down behavior.
- `benchmark_banner.c`, `benchmark_diagnostics.c`, and `benchmark_net_alloc.c` stay out of the correctness harness as performance-only sources.
- `mock_pcap.c` is support-only for future optional networking/dissector tests.
- Voice, traceroute, dissector, networking, and auth legacy files are owned by later gated/optional phases rather than the default core harness; gateway runtime behavior is now covered by the default `gateway` module.
- No legacy file is retired yet; all old sources are either migrated, support-only, performance-only, or assigned to a later optional/gated module.

Completion criteria: every existing K-Term test source is migrated, optional, support-only, or retired.

## Phase 6: Gateway And Runtime Configuration

- [x] Create `sit/k-term/tests/test_gateway_module.c`.
- [x] Add `extern const SitTestModule g_kterm_module_gateway` to `kterm_test_registry.c`.
- [x] Register `g_kterm_module_gateway` in `kterm_test_register_all()`.
- [x] Include `test_gateway_module.c` in `kterm_test_modules.c`.
- [x] Test Gateway command scanner behavior.
- [x] Test Gateway command dispatch.
- [x] Test Gateway extension registration.
- [x] Test Gateway extension callback response.
- [x] Test malformed Gateway commands.
- [x] Test Gateway threading behavior.
- [x] Test Gateway locking behavior.
- [x] Test runtime configuration through Gateway `set` and `get` commands.
- [x] Test Gateway target-session runtime configuration.
- [x] Decide whether auth tests belong to Gateway.
- [x] Document that `gateway_auth` is not created for the default harness because auth belongs with optional networking/security dependencies.
- [x] Document that external auth dependencies are not mocked in the default Gateway harness.

Completion criteria: Gateway behavior is covered without external services.

## Phase 7: IO, Events, Sessions, And Public API

- [x] Create `sit/k-term/tests/test_io_module.c`.
- [x] Add `extern const SitTestModule g_kterm_module_io` to `kterm_test_registry.c`.
- [x] Register `g_kterm_module_io` in `kterm_test_register_all()`.
- [x] Include `test_io_module.c` in `kterm_test_modules.c`.
- [x] Test response callback behavior.
- [x] Test response ring buffer wraparound.
- [x] Test input byte queue processing.
- [x] Test key event processing.
- [x] Test mouse event processing.
- [x] Test focus event processing.
- [x] Test paste event processing.
- [x] Test resize event processing.
- [x] Test printer callback behavior.
- [x] Add public API tests that do not inspect private structs.
- [x] Add multi-session lifecycle tests.
- [x] Add active-session switching tests.
- [x] Add active-session isolation tests.
- [x] Add callback ordering tests.

Completion criteria: embedders are covered through public API tests, not only private white-box tests.

## Phase 8: Serialization And Persistent State

- [x] Create `sit/k-term/tests/test_serialization_module.c`.
- [x] Add `extern const SitTestModule g_kterm_module_serialization` to `kterm_test_registry.c`.
- [x] Register `g_kterm_module_serialization` in `kterm_test_register_all()`.
- [x] Include `test_serialization_module.c` in `kterm_test_modules.c`.
- [x] Test screen cell round-trip.
- [x] Test cursor round-trip.
- [x] Test modes round-trip.
- [x] Test attributes round-trip.
- [x] Test colors round-trip.
- [x] Test titles round-trip.
- [x] Test tab stops round-trip.
- [x] Test scroll margins round-trip.
- [x] Test serialization version compatibility.
- [x] Test corrupt data rejection.
- [x] Test truncated data rejection.

Completion criteria: serialized terminal state round-trips or fails safely with explicit assertions.

## Phase 9: Security Hardening And Fuzz

- [x] Create `sit/k-term/tests/test_security_hardening_module.c`.
- [x] Add `extern const SitTestModule g_kterm_module_security_hardening` to `kterm_test_registry.c`.
- [x] Register `g_kterm_module_security_hardening` in `kterm_test_register_all()`.
- [x] Include `test_security_hardening_module.c` in `kterm_test_modules.c`.
- [x] Test CSI escape buffer overflow handling.
- [x] Test OSC long payload handling.
- [x] Test DCS long payload handling.
- [x] Test APC long payload handling.
- [x] Test PM long payload handling.
- [x] Test SOS long payload handling.
- [x] Test malformed UTF-8 handling.
- [x] Test historical vulnerability repro cases.
- [x] Test public API null arguments where permitted.
- [x] Test public API empty arguments where permitted.
- [x] Create deterministic `sit/k-term/tests/test_fuzz_stress_module.c`.
- [x] Register deterministic `fuzz_stress` module in the default harness.
- [x] Add deterministic fuzz seed corpus.
- [x] Add bounded random byte-stream parser tests.
- [x] Add scrollback stress tests.
- [x] Add resize stress tests.
- [x] Add Sixel payload stress tests.
- [x] Add Kitty payload stress tests.
- [x] Add OSC payload stress tests.

Completion criteria: security tests run in the default harness; bounded fuzz/stress tests run deterministically in the default harness; unbounded or destructive fuzzing remains optional.

## Phase 10: Optional Feature Modules

- [x] Create gated `sit/k-term/tests/test_networking_module.c`.
- [x] Register gated `networking` module.
- [x] Build networking tests only when networking is enabled.
- [x] Mock network dependencies.
- [x] Test traceroute logic under mocks.
- [x] Test pcap behavior.
- [x] Test dissector behavior.
- [x] Test network allocation behavior.
- [x] Decide whether voice belongs in K-Term harness.
- [x] Create gated `sit/k-term/tests/test_voice_module.c` if voice stays.
- [x] Register gated `voice` module if voice stays.
- [x] Mock voice dependencies.
- [x] Test voice command parsing.
- [x] Test VoIP setup.
- [x] Test VoIP teardown.

Optional module notes:

- Default harness keeps networking disabled with `KTERM_DISABLE_NET`.
- Gated networking build is enabled with `KTERM_OPTIONAL_NET=1`; verified `networking` with 11 passed tests.
- Packet capture tests assert the disabled/default PacketDiag build contract unless `KTERM_ENABLE_PACKETDIAG` is added.
- VoIP currently has stub signaling functions without PJSIP state; the harness asserts the default stub contract, while real PJSIP setup/teardown remains a future external-dependency build target.

Completion criteria: optional modules are build-gated, documented, and runnable with required mocks/dependencies.

## Phase 11: Harness UX And CI Readiness

- [x] Document run-all default command.
- [x] Document run-one-module command.
- [x] Document run-one-test command.
- [x] Document optional module command.
- [x] Document long/fuzz module command.
- [x] Add CI-friendly output mode if supported by the framework.
- [x] Add clear failure summary output.
- [x] Ensure logs go to a known location.
- [x] Ensure normal runs do not spam debug output.
- [x] Ensure harness runs from repository root.
- [x] Ensure harness runs from `sit/k-term/tests`.
- [x] Ensure compile errors are easy to read on Windows.

Phase 11 notes:

- Run-all from repository root: `sit\k-term\tests\compile_focused_tests.bat && sit\k-term\tests\run_focused_tests.bat --no-color`.
- Run-all from `sit\k-term\tests`: `compile_focused_tests.bat && run_focused_tests.bat --no-color`.
- Run one module: `run_focused_tests.bat --module parser --no-color`.
- Run one test by substring: `run_focused_tests.bat --module parser --filter 8bit_c1 --no-color`.
- Run deterministic stress/fuzz coverage: `run_focused_tests.bat --module fuzz_stress --no-color`.
- Run optional networking: `set KTERM_OPTIONAL_NET=1`, then compile and run `run_focused_tests.bat --module networking --no-color`.
- CI-friendly output uses `--no-color`; fail-fast jobs can add `--stop-on-fail`.
- Script logs are written to `sit\k-term\tests\logs\kterm_focused_compile.log` and `sit\k-term\tests\logs\kterm_focused_run.log`.
- Windows compile failures print the log path and replay the compiler log immediately.
- Failure summaries now list failed tests by `module.test` at the end of the run.
- Verified root launch: `verification` module passed 15/15.
- Verified tests-directory launch: targeted `voice_command_injection` passed.
- Verified default run: 237 total, 237 passed, 0 failed, 0 skipped.

Completion criteria: a developer can run K-Term tests consistently without knowing historical setup details.

## Phase 12: Completion Audit

- [x] List every public K-Term function.
- [x] Map every public K-Term function to tests.
- [x] List every `VTParseState` value.
- [x] Map every `VTParseState` value to tests.
- [x] List every `KTERM_FEATURE_*` flag.
- [x] Map every `KTERM_FEATURE_*` flag to tests.
- [x] List every optional compile flag.
- [x] Map every optional compile flag to default or optional modules.
- [x] List every old K-Term test file.
- [x] Mark every old K-Term test file as migrated, support-only, optional, or retired.
- [x] Run the default harness.
- [x] Record the default harness result.
- [x] Run optional networking tests or document blockers.
- [x] Run optional voice tests or document blockers.
- [x] Run optional fuzz/stress tests or document blockers.
- [x] Update `kterm_bug_fix_plan.md` with every bug found during harness expansion.

Phase 12 public API ownership:

- Core lifecycle/session/window APIs: `KTerm_Create`, `KTerm_Destroy`, `KTerm_Init`, `KTerm_Cleanup`, `KTerm_Update`, `KTerm_Draw`, `KTerm_SetActiveSession`, `KTerm_SetSplitScreen`, `KTerm_InitSession`, `KTerm_Resize`, `KTerm_SetResponseEnabled`, `KTerm_WriteCharToSession`, `KTerm_ClosePane`. Owned by `integration`, `io`, `terminal_core`, `advanced_grid`, and `security_hardening`.
- Host input/output pipeline APIs: `KTerm_ProcessEvent`, `KTerm_WriteChar`, `KTerm_PushInput`, `KTerm_WriteString`, `KTerm_WriteFormat`, `KTerm_ProcessEvents`, `KTerm_ClearEvents`, `KTerm_GetPendingEventCount`, `KTerm_IsEventOverflow`, `KTerm_GetKey`, `KTerm_QueueInputEvent`, `KTerm_SetPipelineTargetFPS`, `KTerm_SetPipelineTimeBudget`. Owned by `parser`, `integration`, `io`, `security_hardening`, and `fuzz_stress`.
- Mode, keyboard, mouse, focus, title, tab, charset, and paste APIs: `KTerm_SetLevel`, `KTerm_SetMode`, `KTerm_SetKeyboardMode`, `KTerm_SetMouseTracking`, `KTerm_EnableMouseFeature`, `KTerm_SetFocus`, `KTerm_DefineFunctionKey`, `KTerm_SetCursorShape`, `KTerm_SetCursorKTermColor`, `KTerm_SelectCharacterSet`, `KTerm_SetCharacterSet`, `KTerm_SetTabStop`, `KTerm_ClearTabStop`, `KTerm_ClearAllTabStops`, `KTerm_EnableBracketedPaste`, `KTerm_SetWindowTitle`, `KTerm_SetIconTitle`, `KTerm_GetWindowTitle`, `KTerm_GetIconTitle`. Owned by `parser`, `vt_protocols`, `attributes`, `integration`, `io`, `advanced_grid`, and `serialization`.
- Rectangular/grid/cursor APIs: `KTerm_DefineRectangle`, `KTerm_ExecuteRectangularOperation`, `KTerm_CopyRectangle`, `KTerm_ExecuteRectangularOps`, `KTerm_ExecuteRectangularOps2`, `KTerm_SetCellDirect`, `KTerm_MarkRegionDirty`, `KTerm_ScrollUpRegion`, `KTerm_ScrollDownRegion`, `KTerm_InsertLinesAt`, `KTerm_DeleteLinesAt`, `KTerm_InsertCharactersAt`, `KTerm_DeleteCharactersAt`, `KTerm_InsertCharacterAtCursor`, `KTerm_ExecuteSaveCursor`, `KTerm_ExecuteRestoreCursor`, `KTerm_FlushOps`, `KTerm_ClearCell`, `KTerm_ResetAllAttributes`. Owned by `terminal_core`, `advanced_grid`, `attributes`, `terminal_render_buffer`, and `security_hardening`.
- Graphics/font/render APIs: `KTerm_ResetGraphics`, `KTerm_InitSixelGraphics`, `KTerm_ProcessSixelData`, `KTerm_DrawSixelGraphics`, `KTerm_WriteRawGraphics`, `KTerm_LoadSoftFont`, `KTerm_SelectSoftFont`, `KTerm_LoadFont`, `KTerm_CalculateFontMetrics`, `KTerm_AllocateGlyph`, `KTerm_CreateFontTexture`, `KTerm_InitCompute`, `KTerm_PrepareRenderBuffer`. Owned by `graphics`, `font_atlas`, `terminal_render_buffer`, and `vt_protocols`; renderer/Situation integration remains environment-facing rather than live-GPU in the default harness.
- Callback/diagnostic/response APIs: `KTerm_SetResponseCallback`, `KTerm_SetOutputSink`, `KTerm_SetPrinterCallback`, `KTerm_SetTitleCallback`, `KTerm_SetBellCallback`, `KTerm_SetNotificationCallback`, `KTerm_SetErrorCallback`, `KTerm_SetSessionResizeCallback`, `KTerm_RunTest`, `KTerm_ShowInfo`, `KTerm_EnableDebug`, `KTerm_LogUnsupportedSequence`, `KTerm_ReportError`, `KTerm_GetStatus`, `KTerm_ShowDiagnostics`, `KTerm_QueueResponse`, `KTerm_QueueResponseBytes`, `KTerm_CopySelectionToClipboard`, `KTerm_Free`. Owned by `verification`, `protocol_response`, `integration`, `io`, `gateway`, `serialization`, and `security_hardening`.
- Parser/dispatch APIs exported for advanced/testing use: `KTerm_ProcessChar`, `KTerm_ProcessPrinterControllerChar`, `KTerm_ProcessNormalChar`, `KTerm_ProcessEscapeChar`, `KTerm_ProcessCSIChar`, `KTerm_ProcessOSCChar`, `KTerm_ProcessDCSChar`, `KTerm_ProcessAPCChar`, `KTerm_ProcessPMChar`, `KTerm_ProcessSOSChar`, `KTerm_ProcessVT52Char`, `KTerm_ProcessKittyChar`, `KTerm_ProcessSixelChar`, `KTerm_ProcessSixelSTChar`, `KTerm_ProcessControlChar`, `KTerm_ProcessStringTerminator`, `KTerm_ProcessCharsetCommand`, `KTerm_ProcessHashChar`, `KTerm_ProcessPercentChar`, `KTerm_ProcessnFChar`, `KTerm_DispatchSequence`, `KTerm_ParseCSIParams`, `KTerm_GetCSIParam`, `KTerm_ExecuteCSICommand`, `KTerm_ExecuteOSCCommand`, `KTerm_ExecuteDCSCommand`, `KTerm_ExecuteAPCCommand`, `KTerm_ExecuteKittyCommand`, `KTerm_ExecutePMCommand`, `KTerm_ExecuteSOSCommand`, `KTerm_ExecuteDCSAnswerback`. Owned by `parser`, `vt_protocols`, `graphics`, `protocol_response`, `verification`, `security_hardening`, and `fuzz_stress`.
- Scripting APIs: `KTerm_Script_PutChar`, `KTerm_Script_Print`, `KTerm_Script_Printf`, `KTerm_Script_Cls`, `KTerm_Script_SetKTermColor`. Owned indirectly through write/grid/attribute behavior in `integration`, `attributes`, and `terminal_render_buffer`; no separate script-host integration is required for Phase 12.
- Serialization APIs: `KTerm_SerializeSession`, `KTerm_DeserializeSession`. Owned by `serialization`.
- Gateway APIs: `KTerm_GatewayProcess`, `KTerm_RegisterBuiltinExtensions`, `KTerm_SetGatewayCallback`, `KTerm_RegisterGatewayExtension`. Owned by `gateway`.
- Situation adapter API: `KTermSit_ProcessInput`. Owned by `io`/`integration` through event translation contracts; live Situation polling remains outside the mocked harness.
- Voice/VoIP APIs: `KTerm_Voice_Enable`, `KTerm_Voice_SetTarget`, `KTerm_Voice_Command`, `KTerm_Voice_SetGlobalMute`, `KTerm_Voice_GetContext`, `KTerm_Voice_ProcessCapture`, `KTerm_Voice_ProcessPlayback`, `KTerm_Voice_InjectCommand`, `KTerm_VoIP_Register`, `KTerm_VoIP_Dial`, `KTerm_VoIP_DTMF`, `KTerm_VoIP_Hangup`. Owned by `voice`; real PJSIP remains a manual optional build.
- Networking APIs: `KTerm_Net_Init`, `KTerm_Net_Process`, `KTerm_Net_Connect`, `KTerm_Net_Disconnect`, `KTerm_Net_GetStatus`, `KTerm_Net_GetCredentials`, `KTerm_Net_Listen`, `KTerm_Net_SetCallbacks`, `KTerm_Net_SetSecurity`, `KTerm_Net_SetProtocol`, `KTerm_Net_SetKeepAlive`, `KTerm_Net_SetAutoReconnect`, `KTerm_Net_GetSocket`, `KTerm_Net_SetTargetSession`, `KTerm_Net_SendPacket`, `KTerm_Net_SendTelnetCommand`, `KTerm_Net_GetLocalIP`, `KTerm_Net_Ping`, `KTerm_Net_Resolve`, `KTerm_Net_DumpConnections`, `KTerm_Net_Traceroute`, `KTerm_Net_TracerouteContinuous`, `KTerm_Net_ResponseTime`, `KTerm_Net_PortScan`, `KTerm_Net_Whois`, `KTerm_Net_Speedtest`, `KTerm_Net_HttpProbe`, `KTerm_Net_MTUProbe`, `KTerm_Net_FragTest`, `KTerm_Net_PingExt`, `KTerm_Net_PacketDiag_Start`, `KTerm_Net_PacketDiag_Stop`, `KTerm_Net_PacketDiag_GetStatus`, `KTerm_Net_PacketDiag_Pause`, `KTerm_Net_PacketDiag_Resume`, `KTerm_Net_PacketDiag_SetFilter`, `KTerm_Net_PacketDiag_GetDetail`, `KTerm_Net_PacketDiag_Follow`, `KTerm_Net_PacketDiag_GetStats`, `KTerm_Net_PacketDiag_GetFlows`, `KTerm_Net_QueryProtocol`, `KTerm_Net_ScanAuthFlows`, `KTerm_Net_FreeTraceroute`, `KTerm_Net_FreeResponseTime`, `KTerm_Net_FreePortScan`, `KTerm_Net_FreeWhois`, `KTerm_Net_FreeSpeedtest`, `KTerm_Net_FreeHttpProbe`, `KTerm_Net_FreeMtuProbe`, `KTerm_Net_FreeFragTest`, `KTerm_Net_FreePingExt`, `KTerm_Net_FreePacketDiag`. Owned by optional `networking` for local/mockable contracts; live socket, traceroute, speedtest, HTTP probe, MTU, frag, packet capture, SSH/libssh, and PJSIP-like paths are manual/gated checks only.

Phase 12 parser state ownership:

- `VT_PARSE_NORMAL`: `parser`, `integration`, `fuzz_stress`.
- `VT_PARSE_ESCAPE`: `parser`, `vt_protocols`, `security_hardening`.
- `PARSE_CSI`: `parser`, `attributes`, `vt_protocols`, `protocol_response`.
- `PARSE_OSC`: `parser`, `security_hardening`, `fuzz_stress`.
- `PARSE_DCS`: `parser`, `vt_protocols`, `protocol_response`, `security_hardening`.
- `PARSE_APC`: `parser`, `security_hardening`.
- `PARSE_PM`: `parser`, `security_hardening`.
- `PARSE_SOS`: `parser`, `security_hardening`.
- `PARSE_STRING_TERMINATOR`: `parser`, `vt_protocols`, `graphics`.
- `PARSE_CHARSET`: `parser`, `vt_protocols`, `terminal_render_buffer`.
- `PARSE_HASH`: `parser`, `advanced_grid`.
- `PARSE_PERCENT`: `parser`, `vt_protocols`.
- `PARSE_VT52`: `parser`, `vt_protocols`.
- `PARSE_SIXEL`: `graphics`, `vt_protocols`, `fuzz_stress`.
- `PARSE_SIXEL_ST`: `graphics`, `parser`.
- `PARSE_TEKTRONIX`: `graphics`, `vt_protocols`.
- `PARSE_REGIS`: `graphics`, `vt_protocols`.
- `PARSE_KITTY`: `graphics`, `parser`, `fuzz_stress`.
- `PARSE_nF`: `parser`.

Phase 12 feature flag ownership:

- VT level flags `KTERM_FEATURE_VT52_MODE`, `KTERM_FEATURE_VT100_MODE`, `KTERM_FEATURE_VT102_MODE`, `KTERM_FEATURE_VT132_MODE`, `KTERM_FEATURE_VT220_MODE`, `KTERM_FEATURE_VT320_MODE`, `KTERM_FEATURE_VT340_MODE`, `KTERM_FEATURE_VT420_MODE`, `KTERM_FEATURE_VT510_MODE`, `KTERM_FEATURE_VT520_MODE`, `KTERM_FEATURE_VT525_MODE`, `KTERM_FEATURE_K95_MODE`, `KTERM_FEATURE_XTERM_MODE`, `KTERM_FEATURE_TT_MODE`, and `KTERM_FEATURE_PUTTY_MODE`: owned by `vt_protocols.vt_level_feature_matrix`.
- Graphics/grid flags `KTERM_FEATURE_SIXEL_GRAPHICS`, `KTERM_FEATURE_REGIS_GRAPHICS`, `KTERM_FEATURE_RECT_OPERATIONS`, `KTERM_FEATURE_SELECTIVE_ERASE`, `KTERM_FEATURE_SOFT_FONTS`, `KTERM_FEATURE_NATIONAL_CHARSETS`, `KTERM_FEATURE_LEFT_RIGHT_MARGIN`: owned by `vt_protocols`, `graphics`, `advanced_grid`, `font_atlas`, and `terminal_render_buffer`.
- Interaction/session flags `KTERM_FEATURE_USER_DEFINED_KEYS`, `KTERM_FEATURE_MOUSE_TRACKING`, `KTERM_FEATURE_ALTERNATE_SCREEN`, `KTERM_FEATURE_TRUE_COLOR`, `KTERM_FEATURE_WINDOW_MANIPULATION`, `KTERM_FEATURE_LOCATOR`, `KTERM_FEATURE_MULTI_SESSION_MODE`: owned by `vt_protocols`, `protocol_response`, `integration`, `io`, `attributes`, and `advanced_grid`.

Phase 12 optional compile flag ownership:

- `KTERM_DISABLE_NET`: default harness compile flag; `verification.network_diagnostics_legacy_classification` and README document disabled ownership.
- `KTERM_OPTIONAL_NET`: harness environment flag enabling optional `networking`; verified with 11 passed module tests.
- `KTERM_ENABLE_PACKETDIAG`: optional/manual networking flag; `networking.packetdiag_build_contract` and `networking.packetdiag_auth_scan_contract` assert the enabled/disabled contract.
- `KTERM_USE_BUNDLED_PCAP`: optional/manual packet capture dependency; legacy `mock_pcap.c` is support-only and live capture stays manual/gated.
- `KTERM_DISABLE_TELNET`: optional networking compile shape; catalog/auth metadata is covered when telnet is enabled, disabled builds remain optional/manual.
- `KTERM_USE_LIBSSH`: optional/manual external dependency; classified as manual because default MinGW/local harness does not link libssh.
- `KTERM_DISABLE_VOICE`: optional compile exclusion; default harness keeps `voice` enabled and the module is conditionally registered.
- `KTERM_DISABLE_VOIP`: optional compile exclusion; `voice.voip_gateway_contract_is_stubbed` owns the default stub contract.
- `KTERM_USE_PJSIP`: optional/manual external dependency; real PJSIP signaling is future/manual and not part of normal CI.
- `KTERM_DISABLE_GATEWAY`/`KTERM_ENABLE_GATEWAY`: default gateway ownership is `gateway`; disabled-gateway builds are compile-shape optional.
- `KTERM_ENABLE_DEBUG_OUTPUT`: default scripts force `0`; `verification.diagnostics_debug_sequence_options` owns debug callback behavior without normal output spam.
- `KTERM_ENABLE_MT_ASSERTS`: internal debug assertion flag; not enabled by default, with thread/locking behavior covered by `integration` and `gateway`.
- `KTERM_USE_SHARED`/`KTERM_BUILD_SHARED`: packaging/export flags; not required for the local white-box harness.
- `KTERM_IMPLEMENTATION`, `KTERM_*_IMPLEMENTATION`: single-header implementation guards; owned by the white-box harness structure in `kterm_test_modules.c`.

Phase 12 legacy test classification:

- Migrated: `test_attributes_modes_suite.c`, `test_parser_suite.c`, `test_graphics_suite.c`, `test_integration_suite.c`, `test_verification_suite.c`, `test_serialize_suite.c`, `test_io_suite.c`, `test_gateway_suite.c`, `test_gateway_case.c`, `verify_gateway_threading.c`, `test_fuzz_suite.c`, `stress_tests.c`, `test_buffer_hardening.c`, `test_decsca.c`, `verify_scroll_down.c`.
- Support-only: `mock_pcap.c`.
- Benchmark-only: `benchmark_banner.c`, `benchmark_diagnostics.c`, `benchmark_net_alloc.c`.
- Optional/manual networking or live-environment candidates: `test_networking_suite.c`, `net_tests.c`, `verify_diagnostics.c`, `verify_traceroute.c`, `verify_traceroute_continuous.c`, `repro_vulnerability.c`.
- Optional/manual voice/VoIP candidates: `verify_voice.c`, `verify_voice_commands.c`, `verify_voip.c`.
- Reviewed and folded during Phase 12: `test_dissectors.c`, `verify_auth_fields.c`, `verify_advanced_auth.c`, `verify_voice_commands.c`. Deterministic ownership now lives in `networking.protocol_catalog_auth_metadata`, `networking.protocol_catalog_media_and_discovery_ports`, `networking.packetdiag_auth_scan_contract`, and `voice.voice_vad_activation_and_silence_reset`; live packet capture remains manual/gated.
- Retired: none.

Phase 12 verification results:

- Default harness: 238 total, 238 passed, 0 failed, 0 skipped.
- Optional networking build: `KTERM_OPTIONAL_NET=1`; 270 total, `networking` 11 passed, 0 failed.
- Optional voice/default voice module: `voice` 6 passed, 0 failed.
- Optional fuzz/stress module: `fuzz_stress` 7 passed, 0 failed.
- No new implementation bug was found during Phase 12; `kterm_bug_fix_plan.md` status was updated with current harness results only.

Completion criteria: no K-Term subsystem remains unowned by a test module or explicit exclusion.

## Phase 13: Display Consistency And Visual Invariants

- [x] Create a `display_consistency` harness module for cross-layer display invariants.
- [x] Register `display_consistency` in the module registry.
- [x] Include `display_consistency` in the white-box implementation unit.
- [x] Test mixed SGR text never emits uninitialized render cells.
- [x] Test clearing after white foreground/background restores default transparent background.
- [x] Test DEC box-drawing glyphs survive color reset and render-buffer update.
- [x] Test resize-facing render-buffer rebuild keeps cells initialized.
- [x] Test alternate-screen swap restores main-buffer render content.
- [x] Test graphics protocol exit allows normal text rendering afterward.
- [x] Run the `display_consistency` module.
- [x] Run the default harness with `display_consistency` enabled.
- [x] Document bugs found during Phase 13 in `kterm_bug_fix_plan.md`.

Phase 13 notes:

- New module: `sit/k-term/tests/test_display_consistency_module.c`.
- The tests assert initialized `GPUCell` glyph/color/flag fields after realistic mixed writes, clears, box drawing, session resize, alternate screen swaps, and Sixel-to-text transitions.
- The clear test specifically guards against stale opaque white backgrounds becoming rendered blocks after a colored row is cleared.
- The resize invariant now uses the public `KTerm_Resize()` path. The earlier hang was traced to a recursive `compositor.render_lock` acquisition and fixed.
- Verified `display_consistency`: 6 passed, 0 failed.
- Verified default harness: 258 total, 258 passed, 0 failed, 0 skipped.

Completion criteria: realistic display sequences produce initialized, stable render-buffer cells and trap the white-block/stale-attribute class of regressions before GPU presentation.
