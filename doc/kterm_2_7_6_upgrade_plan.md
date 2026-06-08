# K-Term 2.7.6+ Upgrade Plan

Status: rebased through `v2.7.12`; the supplied backlog and reserve-slot consolidation are complete.

Completion Date: `2026/05/17`

Goal: rebase the unimplemented `v2.7.5` through `v2.7.13` backlog onto the current K-Term line. The current tree already uses `v2.7.5` for harness hardening and display contracts, so the work below starts at `v2.7.6` and continues forward.

## Version Rebase

- [x] Rebase the supplied old `v2.7.5` string-concatenation optimization work into this plan.
- [x] Rebase the supplied old `v2.7.6` Speedtest socket-loop guard work into this plan.
- [x] Rebase the supplied old `v2.7.7` Gateway Grid wrap fix work into this plan.
- [x] Rebase the supplied old `v2.7.8` console command-history buffer hardening work into this plan.
- [x] Rebase the supplied old `v2.7.9` Gateway Grid, Speedtest, string-performance, font-listing, banner, Whois, and API cleanup work into this plan.
- [x] Rebase the supplied old `v2.7.10` MTU probing work into this plan.
- [x] Rebase the supplied old `v2.7.11` console tab-completion performance work into this plan.
- [x] Rebase the supplied old `v2.7.12` K-Term core bounds/null-termination hardening work into this plan.
- [x] Rebase the supplied old `v2.7.13` SSH/Telnet user-string null-termination hardening work into this plan.

### Rebase Landing Map

- Old `v2.7.5` string-concatenation optimization landed as current `v2.7.10`.
- Old `v2.7.6` Speedtest socket-loop guard landed as current `v2.7.8`.
- Old `v2.7.7` Gateway Grid wrap fix landed as current `v2.7.9`.
- Old `v2.7.8` console command-history hardening landed as current `v2.7.6`.
- Old `v2.7.9` mixed Gateway/Speedtest/string/API cleanup was split across current `v2.7.8`, `v2.7.9`, `v2.7.10`, and `v2.7.11`.
- Old `v2.7.10` MTU probing landed as current `v2.7.8`.
- Old `v2.7.11` console tab-completion performance landed as current `v2.7.10`.
- Old `v2.7.12` K-Term core bounds/null-termination hardening landed as current `v2.7.6`.
- Old `v2.7.13` SSH/Telnet user-string null-termination hardening landed as current `v2.7.7`.

## Planned v2.7.6: Security Hardening Round 1

Goal: close direct string and buffer safety risks before performance work.

- [x] Audit `examples/kterm_console.c` command-history and edit-buffer writes. *(Originally `console.c`; merged 2026-06.)*
- [x] Replace unsafe command-history `strcpy` usage with bounded copies.
- [x] Add explicit null-termination after every bounded copy into console history buffers.
- [x] Add tests or harness hooks for max-length console history entries.
- [x] Audit `kterm_impl.h` string writes involving function keys.
- [x] Audit `kterm_impl.h` string writes involving event sequences.
- [x] Audit `kterm_impl.h` string writes involving extension names.
- [x] Audit `kterm_impl.h` string writes involving terminal and icon titles.
- [x] Audit `kterm_impl.h` string writes involving DCS answerback buffers.
- [x] Replace hardcoded destination sizes with `sizeof(destination)` where possible.
- [x] Add explicit null-termination after `strncpy`/bounded copy sites in `kterm_impl.h`.
- [x] Fix `KTerm_ExecuteDCSAnswerback` length checks to use the actual destination buffer size, not `MAX_COMMAND_BUFFER`.
- [x] Add regression coverage for oversized DCS answerback payloads.
- [x] Add regression coverage for oversized function-key/event/title payloads.
- [x] Bump K-Term to `2.7.6` when this section lands.
- [x] Add a `v2.7.6` update-log entry.

## Planned v2.7.7: SSH And Telnet Client String Safety

Goal: harden optional/reference network clients against unterminated user-controlled strings.

- [x] Locate current SSH client implementation files in this tree.
- [x] Locate current Telnet client implementation files in this tree.
- [x] Audit all `strncpy` calls in SSH client code.
- [x] Audit all `strncpy` calls in Telnet client code.
- [x] Add explicit null-termination after every bounded SSH user-string copy.
- [x] Add explicit null-termination after every bounded Telnet user-string copy.
- [x] Add regression tests or compile-time harness checks for maximum-length host/user/path strings.
- [x] Bump K-Term to `2.7.7` when this section lands.
- [x] Add a `v2.7.7` update-log entry.

## Planned v2.7.8: Network Reliability And MTU Probing

Goal: prevent repeated socket creation loops and replace placeholder MTU behavior with platform-backed probing.

- [x] Audit Speedtest phases `AUTO_SELECT`, `CONNECT_DL`, `CONNECT_UL`, and `RUN_UL`.
- [x] Add or verify per-phase `initiated` flags before socket creation calls.
- [x] Ensure download socket creation occurs only once per download phase.
- [x] Ensure upload socket creation occurs only once per upload phase.
- [x] Close sockets and transition state on fatal socket creation errors.
- [x] Explicitly check `send()` return values during upload.
- [x] Close upload sockets on fatal `send()` errors.
- [x] Close upload sockets on remote closure.
- [x] Add deterministic tests for socket-creation failure paths where mocks allow it.
- [x] Implement local MTU lookup in `KTerm_Net_ProcessMtuProbe` for Linux/Unix via `getifaddrs`.
- [x] Implement local MTU lookup in `KTerm_Net_ProcessMtuProbe` for Windows via `GetBestInterface` and `GetIfEntry`.
- [x] Preserve a documented fallback when platform MTU lookup is unavailable.
- [x] Add optional networking tests for MTU probe success/fallback behavior.
- [x] Bump K-Term to `2.7.8` when this section lands.
- [x] Add a `v2.7.8` update-log entry.

## Planned v2.7.9: Gateway Grid FillSpan Correctness

Goal: make Gateway Grid fill spans robust for wrapped and non-wrapped out-of-bounds coordinates.

- [x] Audit `KTerm_Grid_FillSpan` wrapped coordinate handling.
- [x] Add a regression test for wrap enabled with initial `x >= cols`.
- [x] Add a regression test for wrap disabled with initial `x >= cols`.
- [x] Replace negative-width masking hacks with correct modulo/division arithmetic for wrapped spans.
- [x] Add an early break for non-wrapped spans when computed width is `<= 0`.
- [x] Verify out-of-bounds fill spans do not enter infinite loops.
- [x] Verify out-of-bounds fill spans do not write outside terminal rows.
- [x] Remove obsolete speculative comments around the old clamping behavior.
- [x] Bump K-Term to `2.7.9` when this section lands.
- [x] Add a `v2.7.9` update-log entry.

## Planned v2.7.10: Hot-Path String Performance

Goal: remove avoidable O(N^2) string work in Gateway, console, banner, SSH trigger listing, and network diagnostics.

- [x] Optimize Gateway `GET;FONTS` generation to avoid repeated `strcat`.
- [x] Add `name_len` to `KTermFontDef` or an equivalent cached length field.
- [x] Replace repeated font-name `strlen` scans with O(1) cached length usage.
- [x] Replace font-listing `strcpy` with length-aware `memcpy` where appropriate.
- [x] Benchmark `GET;FONTS` before and after the change.
- [x] Optimize SSH `ext;ssh;trigger;list` string concatenation if the SSH extension exists in this tree.
- [x] Hoist redundant `strlen` calls in `CompleteCommonPrefix` in `examples/kterm_console.c`. *(Originally `console.c`; merged 2026-06.)*
- [x] Add a tab-completion performance sanity test or benchmark if practical.
- [x] Optimize `KTerm_GenerateBanner` by using `snprintf` return values for emitted color sequence lengths.
- [x] Optimize `KTerm_Whois_Callback` by avoiding repeated `strlen` on large output buffers.
- [x] Add focused regression coverage for generated output equivalence after each optimization.
- [x] Bump K-Term to `2.7.10` when this section lands.
- [x] Add a `v2.7.10` update-log entry with measured benchmark deltas.

## Planned v2.7.11: API Cleanup And Documentation Consistency

Goal: remove stale references to APIs that were removed in the v2.1 era and align docs with the current event-driven input system.

- [x] Verify whether `KTerm_UpdateMouse` remains referenced in public headers or docs.
- [x] Verify whether `KTerm_UpdateKeyboard` remains referenced in public headers or docs.
- [x] Verify whether `UpdateKeyboard` remains referenced in public headers or docs.
- [x] Verify whether `GetKeyEvent` remains referenced in public headers or docs.
- [x] Remove dead declarations from `kterm_api.h` if any remain.
- [x] Remove dead references from `kterm_impl.h` comments if any remain.
- [x] Update `doc/kterm.md` to describe the modern event-driven input system.
- [x] Add grep-based verification notes to the update log.
- [x] Bump K-Term to `2.7.11` when this section lands.
- [x] Add a `v2.7.11` update-log entry.

## Planned v2.7.12: Consolidated Security Verification

Goal: verify every security fix above has deterministic coverage and no duplicate stale patch claims remain.

- [x] Run the default focused K-Term harness.
- [x] Run optional networking tests with `KTERM_OPTIONAL_NET=1`.
- [x] Run any SSH/Telnet-specific compile or smoke tests available in the tree.
- [x] Add missing security tests to `security_hardening` or a new focused module, or document that existing deterministic coverage is complete.
- [x] Update `kterm_bug_fix_plan.md` with newly fixed security issues if implementation uncovers actual defects, or document that no new defects were found.
- [x] Confirm all bounded-copy changes preserve exact intended output.
- [x] Confirm update-log entries do not conflict with the current `v2.7.5` release history.
- [x] Bump K-Term to `2.7.12` when this section lands.
- [x] Add a `v2.7.12` update-log entry.

## Planned v2.7.13 And Beyond: Reserve

Goal: document that the reserve slots are no longer needed for the supplied backlog because their work was absorbed by earlier rebased patches.

- [x] Reserve `v2.7.13` for spillover security fixes found during implementation.
- [x] Reserve `v2.7.14` for spillover networking reliability fixes found during implementation.
- [x] Reserve `v2.7.15` for spillover performance cleanup found during implementation.

### Reserve Slot Consolidation

- `v2.7.13` security spillover is not needed for the supplied backlog: core bounds, answerback, function-key/event/title, console history, and SSH/Telnet user-string hardening landed in current `v2.7.6` and `v2.7.7`, with deterministic coverage in `security_hardening`.
- `v2.7.14` networking spillover is not needed for the supplied backlog: Speedtest socket lifecycle and MTU probing landed in current `v2.7.8`, with optional networking coverage under `KTERM_OPTIONAL_NET=1`.
- `v2.7.15` performance spillover is not needed for the supplied backlog: Gateway font listing, banner, WHOIS, SSH trigger listing, PacketDiag flags, and console completion string work landed in current `v2.7.10`, with output-equivalence coverage where practical.

## Verification Policy

- [x] Every landed patch must update `KTERM_VERSION_PATCH` and `KTERM_VERSION_STRING`.
- [x] Every landed patch must add an entry to `sit/k-term/doc/updatelog.md`.
- [x] Every landed patch must run the default focused harness.
- [x] Optional networking changes must run the optional networking build.
- [x] Security changes must add focused regression coverage or a documented reason why coverage is compile/manual only.
- [x] Performance changes must preserve generated output and include before/after measurements when practical.
