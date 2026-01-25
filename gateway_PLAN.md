# K-Term Gateway Protocol Extraction Plan
## Migrating Gateway Protocol to `kt_gateway.h` — Phased Approach

**Author**: Jacques Morel (@jmorel33)
**Date**: January 25, 2026
**Version**: Draft for v2.4+
**Goal**: Extract the Gateway Protocol (runtime introspection/configuration system) from the core `kterm.h` into a dedicated optional module `kt_gateway.h`. This improves modularity, reduces core complexity, makes the feature truly optional (via compile-time define), and allows easier maintenance/extension of Gateway commands without bloating the main terminal emulation logic.

### Rationale
- **Modularity**: Gateway is a powerful but non-essential extension (not part of DEC/xterm standards). Separating it keeps the core focused on emulation compliance.
- **Optional Feature**: Many embedders (e.g., minimal retro emulators) may not need runtime control → save compile time/code size.
- **Cleaner Core**: Removes ~500-1000 lines of parsing/command handling from the main parser.
- **Future-Proof**: Easier to add new PIPE commands (e.g., font control, diagnostics) without touching core.
- **Single-Header Compatibility**: `kt_gateway.h` will be included only in the implementation section of `kterm.h`, preserving the embeddable model.

### High-Level Changes
- New file: `kt_gateway.h` (declarations + implementation when defined).
- New optional define: `#define KTERM_ENABLE_GATEWAY` (default ON for backward compat, but can be disabled).
- Core `kterm.h` will forward Gateway parsing to the new module if enabled.
- Gateway callback remains `GatewayCallback` in `kterm.h` (public API unchanged).

### Phased Implementation Plan

#### Phase 1: Preparation & Isolation
**Goal**: Identify and isolate all Gateway-related code without breaking anything.

- [x] **Audit `kterm.h` for Gateway dependencies**
  - [x] Locate `GatewayCallback` typedef.
  - [x] Locate `term->gateway` struct/field (or scattered variables).
  - [x] Identify parser logic for "PIPE;" sequences (DCS/APC/OSC handlers).
  - [x] Identify command handlers: BANNER, SET, GET, RESET.
  - [x] Identify helper functions (banner generation, attribute parsing).

- [x] **Apply Temporary Guards**
  - [x] Wrap existing Gateway code in `kterm.h` with `#ifdef KTERM_ENABLE_GATEWAY`.
  - [x] Ensure `KTERM_ENABLE_GATEWAY` is defined by default for now (to maintain current behavior).

- [x] **Create Stub `kt_gateway.h`**
  - [x] Create file `kt_gateway.h`.
  - [x] Add include guard `#ifndef KT_GATEWAY_H`.
  - [x] Add placeholder comment.

- [x] **Verification**
  - [x] Compile with `KTERM_ENABLE_GATEWAY` defined (should work as before).
  - [x] Compile with `KTERM_ENABLE_GATEWAY` undefined (core should work, Gateway features absent).

#### Phase 2: Create `kt_gateway.h` Structure
**Goal**: Move declarations and implementation to new file.

- [x] **Setup `kt_gateway.h` Header Section**
  - [x] Include `kterm.h` (forward declarations).
  - [x] Declare internal command table structures (if needed).
  - [x] Declare internal helper functions (static or hidden).

- [x] **Setup `kt_gateway.h` Implementation Section**
  - [x] Add `#ifdef KTERM_GATEWAY_IMPLEMENTATION` block.
  - [x] Create `KTerm_GatewayProcess` function signature:
    ```c
    void KTerm_GatewayProcess(KTerm* term, const char* class_id, const char* id, const char* command, const char* params);
    ```

- [x] **Migrate Logic**
  - [x] Move `Gateway_HandleBanner` logic to `kt_gateway.h`.
  - [x] Move `Gateway_HandleSet` logic (ATTR, GRID, CONCEAL, OUTPUT, BLINK) to `kt_gateway.h`.
  - [x] Move `Gateway_HandleGet` logic (OUTPUT, STATUS) to `kt_gateway.h`.
  - [x] Move `Gateway_HandleReset` logic to `kt_gateway.h`.
  - [x] Move the user callback invocation logic into `KTerm_GatewayProcess`.

- [x] **Refine Define Strategy**
  - [x] Ensure `kt_gateway.h` can be included effectively in single-header mode.

#### Phase 3: Refactor Core `kterm.h`
**Goal**: Remove Gateway from core, forward to new module.

- [ ] **Update `kterm.h` Header Definitions**
  - [ ] Wrap `GatewayCallback` typedef in `#ifdef KTERM_ENABLE_GATEWAY`.
  - [ ] Add `#include "kt_gateway.h"` inside the guard (for declarations).

- [ ] **Update `KTerm` Struct**
  - [ ] Remove hardcoded Gateway fields if they can be moved or are redundant.
  - [ ] Keep `term->gateway_callback` but guard it if strictly necessary (or leave as NULL-able).

- [ ] **Update Parser Handlers**
  - [ ] In DCS/OSC/APC handlers, replace inline Gateway logic with:
    ```c
    #ifdef KTERM_ENABLE_GATEWAY
    KTerm_GatewayProcess(term, class_id, id, command, params);
    #endif
    ```

- [ ] **Update Implementation Section**
  - [ ] In `#ifdef KTERM_IMPLEMENTATION`:
    ```c
    #ifdef KTERM_ENABLE_GATEWAY
    #define KTERM_GATEWAY_IMPLEMENTATION
    #include "kt_gateway.h"
    #undef KTERM_GATEWAY_IMPLEMENTATION
    #endif
    ```

- [ ] **API Compatibility**
  - [ ] Ensure `KTerm_SetGatewayCallback` is guarded or handles the disabled state gracefully (e.g., no-op).

#### Phase 4: Polish & Testing
**Goal**: Ensure seamless operation and optional disable.

- [ ] **Documentation Updates**
  - [ ] Update `README.md` to mention `KTERM_ENABLE_GATEWAY`.
  - [ ] Update `kterm.md` (if exists) with usage instructions for the separated module.
  - [ ] Document how to disable it for minimal builds.

- [ ] **Testing Scenarios**
  - [ ] **Default Build**: Compile and run existing test suite. Verify Gateway commands (BANNER) work.
  - [ ] **Minimal Build**: Define `#define KTERM_ENABLE_GATEWAY 0` (or undefine). Compile. Verify binary size reduction (if measurable) and absence of Gateway symbols.
  - [ ] **Manual Testing**:
    - [ ] Test `PIPE;BANNER;`
    - [ ] Test `PIPE;SET;ATTR;KEY=VAL`
    - [ ] Test user callback invocation.
  - [ ] **Edge Case Testing**: Send malformed Gateway strings.

- [ ] **Optional Enhancements**
  - [ ] Add public helper `KTerm_GatewayCommand(term, cmd_string)` if useful for users.

#### Phase 5: Release Integration

- [ ] **Version Bump**: Prepare for v2.3.1.
- [ ] **Changelog**: Add entry: "Extracted Gateway Protocol to optional `kt_gateway.h` for cleaner core and smaller builds."
- [ ] **Code Review**: Final pass over the split to ensure no internal state leaks.

### Risks & Mitigations

- **API Break**: None — callback signature unchanged.
- **Compile Errors**: Guard everything carefully.
- **Performance**: Negligible (Gateway rare).
- **File Management**: Ship `kt_gateway.h` alongside `kterm.h`.
