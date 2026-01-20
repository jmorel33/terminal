# K-Term v2.1 Refactoring Plan

## Overview
This plan outlines the steps to decouple the core K-Term logic from the Situation library. The goal is to move all Situation-dependent code (input and rendering) into separate header-only libraries, leaving `kterm.h` as a pure terminal emulation library.

## Phase 1: Externalize Input (`kterm_io_sit.h`)

The objective is to move Situation keyboard and mouse handling out of `kterm.h`.

1.  **Create `kterm_io_sit.h`**
    *   Create a new header file `kterm_io_sit.h`.
    *   Include `situation.h` and `kterm.h` within it.
    *   Define `KTERM_IO_SIT_IMPLEMENTATION` guard.

2.  **Define Key Constants in `kterm.h`**
    *   Define `KT_KEY_*` macros in `kterm.h` to replace `SIT_KEY_*` usage.
        *   Define printable keys (A-Z, a-z, 0-9, Punctuation) using standard ASCII values.
        *   Define functional keys (UP, DOWN, LEFT, RIGHT, F1-F12, ENTER, BACKSPACE, DELETE, TAB, ESCAPE).
        *   Define `KT_KEY_*` constants for "normal codes" (the lower 5 bits of characters) to help with the decoding process.
        *   **Do NOT** define constants for modifier keys (SHIFT, CTRL, ALT, META), as these are handled via flags and not passed as raw key codes to the terminal logic.
    *   Update `VTKeyEvent` documentation to reference `KT_KEY_*`.

3.  **Move Input Functions**
    *   Move the implementation of `KTerm_UpdateKeyboard(KTerm* term)` from `kterm.h` to `kterm_io_sit.h`.
    *   Move the implementation of `KTerm_UpdateMouse(KTerm* term)` from `kterm.h` to `kterm_io_sit.h`.
    *   **Implement Translation**: In `kterm_io_sit.h`, implement logic to translate `SIT_KEY_*` codes from Situation events into `KT_KEY_*` codes before passing them to K-Term's internal logic.

4.  **Update `kterm.h`**
    *   Remove `situation.h` include for Input dependencies.
    *   Keep `VTKeyboard` and `MouseTrackingMode` definitions in `kterm.h`.
    *   Ensure no `SIT_KEY_*` constants remain in `kterm.h`.

## Phase 2: Externalize Video Output (`kterm_render_sit.h`)

The objective is to move Situation rendering, buffer management, and shader logic out of `kterm.h`.

1.  **Create `kterm_render_sit.h`**
    *   Create a new header file `kterm_render_sit.h`.
    *   Include `situation.h`, `stb_truetype.h`, and `kterm.h`.
    *   Define `KTERM_RENDER_SIT_IMPLEMENTATION` guard.

2.  **Define Renderer Context**
    *   Define `struct KTermSituationRenderer` in `kterm_render_sit.h` to hold all Situation-specific resources currently in `KTerm`:
        *   `SituationComputePipeline compute_pipeline`
        *   `SituationBuffer terminal_buffer`
        *   `SituationTexture output_texture`
        *   `SituationTexture font_texture`
        *   `SituationTexture sixel_texture`
        *   `SituationTexture dummy_sixel_texture`
        *   `GPUCell* gpu_staging_buffer`
        *   Vector Engine resources (`vector_buffer`, `vector_pipeline`, `vector_staging_buffer`, `vector_layer_texture`)
        *   Sixel Engine resources (`sixel_buffer`, `sixel_palette_buffer`, `sixel_pipeline`)
        *   Font Atlas resources (`font_atlas_pixels`, `glyph_map`, `ttf` context).

3.  **Move Rendering Functions**
    *   Move the following implementations from `kterm.h` to `kterm_render_sit.h`:
        *   `KTerm_Draw(KTerm* term)`
        *   `KTerm_InitCompute(KTerm* term)`
        *   `KTerm_CreateFontTexture(KTerm* term)`
        *   `KTerm_UpdateSSBO(KTerm* term)` and `KTerm_UpdateRow` logic.
        *   `KTerm_AllocateGlyph` and `RenderGlyphToAtlas` (as they manage the texture atlas).
        *   Shader source strings (`terminal_shader_body` etc.) and `KTermPushConstants`.

4.  **Refactor `kterm.h` Core Structs**
    *   Modify `struct KTerm` in `kterm.h`:
        *   Remove all fields moved to `KTermSituationRenderer`.
        *   Add `void* renderer;` (opaque pointer) to hold the renderer context.
        *   Remove `#include "situation.h"` and `#include "stb_truetype.h"`.
        *   Replace `Color` usage with `RGB_Color` (struct defined in `kterm.h`) if necessary to avoid `situation.h` dependency.

5.  **Lifecycle Management**
    *   **Initialization**:
        *   `KTerm_Init` in `kterm.h` should only initialize core logic (sessions, buffers).
        *   Create `KTermSit_Init(KTerm* term)` in `kterm_render_sit.h` to allocate `KTermSituationRenderer`, assign it to `term->renderer`, and call `KTerm_InitCompute` etc.
    *   **Resizing**:
        *   `KTerm_Resize` in `kterm.h` should only resize logical buffers (`screen_buffer`).
        *   Create `KTermSit_Resize(KTerm* term, int width, int height)` in `kterm_render_sit.h` which calls `KTerm_Resize` and then recreates GPU resources.
    *   **Cleanup**:
        *   `KTerm_Cleanup` in `kterm.h` should only free core buffers.
        *   Create `KTermSit_Cleanup(KTerm* term)` in `kterm_render_sit.h` to destroy Situation resources and free the renderer struct.

## Verification Checklist
- [ ] `kterm.h` compiles without `situation.h`.
- [ ] `kterm_io_sit.h` handles input correctly with `SIT_KEY` to `KT_KEY` translation.
- [ ] `kterm_render_sit.h` handles rendering correctly.
- [ ] Core logic remains intact.
