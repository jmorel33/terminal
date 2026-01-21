# Terminal Upgrade Analysis: The Path to v2.0 (Museum-Grade Compliance)

## Executive Summary
This document outlines the architectural upgrade from the current v1.3 "Modern Commercial" terminal to v2.0 "Hardware Accurate" Tektronix/ReGIS vector terminal. The core innovation is the shift from CPU-side rasterization to a **GPU-Driven Vector Engine**, utilizing Compute Shaders to emulate the specific phosphor persistence and glow of Tektronix storage tubes.

## Current Architecture (v1.3 -> v2.0 Transition)
*   **State Machine:** `VTParseState` enum now handles ANSI/VT52/Sixel/Tektronix/ReGIS.
*   **Rendering:** Dual-stage Compute Shader pipeline.
    1.  **Vector Pass:** `VECTOR_COMPUTE_SHADER_SRC` draws vectors into a persistent `vector_layer_texture` (simulating the storage tube surface).
    2.  **Composite Pass:** `TERMINAL_COMPUTE_SHADER_SRC` blends the text grid with the vector layer, applying CRT effects (curvature, scanlines).
*   **Storage:** `EnhancedTermChar` 2D array on CPU for text, `GPUVectorLine` SSBO for vector display list.

## Phase 1: The GPU Vector Engine (Tektronix 4010/4014)

**Status: COMPLETED**

This phase introduced the fundamental structures for GPU-accelerated vector drawing.

- [x] **Structs:** Added `GPUVectorLine` to `terminal.h`.
- [x] **State:** Added `PARSE_TEKTRONIX` to `VTParseState`.
- [x] **Init:** Updated `InitTerminalCompute` to allocate `vector_buffer` and `vector_layer_texture`.
- [x] **Shader:** Implemented `VECTOR_COMPUTE_SHADER_SRC` with additive "glow" blending.
- [x] **Parser:** Implemented `ProcessTektronixChar` with HiY/LoY/HiX/LoX state machine.
- [x] **Compositing:** Integrated vector layer sampling into the main terminal shader.

## Phase 2: ReGIS Support (The "CAD" Milestone)

**Status: NEARLY COMPLETE**

ReGIS (Remote Graphics Instruction Set) provides higher-level graphics primitives. The basic infrastructure is in place, but advanced features are pending.

### Implemented Features
- [x] **Parser Infrastructure:** `ProcessReGISChar` state machine (Command, Options, Arguments, Data).
- [x] **Command: Position (P):** Absolute and Relative positioning.
- [x] **Command: Vector (V):** Line drawing with absolute/relative coordinates.
- [x] **Command: Curve (C):**
    - [x] Circle (Unclosed/Closed).
    - [x] Arc (Interpolated).
    - [x] B-Spline interpolation.
- [x] **Command: Text (T):** Basic string rendering using 8x8 bitmap font.
- [x] **Command: Screen (S):** Screen erasure (`S(E)`).
- [x] **Command: Write (W):** Color selection (`W(I...)`, `W(C...)`).

### Missing / Todo Actionables
- [x] **Text Attributes:** Implement size and rotation controls in the Text command.
    - *Current:* Hardcoded scale 2.0, no rotation.
    - *Goal:* Support `T(S[n], D[angle])`.
- [x] **Polygon Fill (F):** Implement scanline or vector-based filling for closed shapes.
    - *Actionable:* Add a "Fill" flag to the vector pipeline or a separate compute pass for rasterizing filled polygons.
- [x] **Load Alphabet (L):** Support custom soft-fonts for ReGIS text.
    - *Actionable:* Parse `L` command and update a section of the font atlas.
- [x] **Macrographs (@):** Record and playback sequences of commands.
    - *Actionable:* Implement a buffer to store command strings and a playback mechanism.
- [x] **Report (R):** Report cursor position and state back to the host.
    - *Actionable:* Implement `R(P)` to queue position response.
- [x] **Display Addressing:** Support standard ReGIS 800x480 coordinate space scaling (currently normalized, needs verification of aspect ratio handling).
- [x] **Write Controls (W):** Implement `W(R)` (Replace), `W(E)` (Erase), `W(V)` (Overlay), `W(C)` (Complement/XOR).
    - *Actionable:* Update `GPUVectorLine` and Compute Shader to support drawing modes beyond additive glow.

## Phase 3: ISO 2022 & NRCS (The "Linguist" Milestone)

**Status: COMPLETED**

- [x] **Refactor `TranslateCharacter`:** Replace switch-cases with a robust lookup table (LUT) approach for National Replacement Character Sets (NRCS).
- [x] **Locking Shifts:** Fully implement LS0, LS1, LS2, LS3 for switching between G0-G3 charsets.
- [x] **UTF-8 Renderer:** Upgrade from 256-glyph CP437 texture to a dynamic glyph cache or signed distance field (SDF) font for full Unicode support.

## Phase 4: Scrollback & Session Management

**Status: COMPLETED**

- [x] **Ring Buffer:** Replace the fixed `screen[ROW][COL]` array with a large ring buffer to support infinite scrollback.
- [x] **Viewport Uniform:** Add `viewport_offset` to the Compute Shader to render the correct "window" of the ring buffer without re-uploading data.
- [ ] **Session UI:** Add visual indicators (tabs or status bar) for the active session in the split-screen view.
