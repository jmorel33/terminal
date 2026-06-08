# K-Term API Alignment Plan

**Date**: 2026-06-01  
**Status**: COMPLETE  
**Goal**: Bring the K-Term test harness and kterm_console example back to clean compilation and full test pass against the current Situation v2.4.197 API.

## Problem Summary

The K-Term test harness (`sit/k-term/tests/compile_focused_tests.bat`) fails to compile due to:

1. **Include path** — the focused test build can't resolve `sit/situation_api.h` (uses `../../../tests/harness/sit_api_include.h` which expects the workspace root in the include path)
2. **Removed constant** — `KTERM_FAILURE` maps to `SITUATION_FAILURE` which no longer exists
3. **kt_voice.h type visibility** — `SituationAudioGraph*`, `SituationNodeHandle` are not visible when kterm headers are included before the audio section of `situation_api.h`
4. **Compile flag** — `SITUATION_ERROR_INVALID_PARAM` not visible in the kterm test build (needs proper includes or the test build to define the right flags)

## Root Causes

| Error | File | Cause | Fix |
|-------|------|-------|-----|
| `situation_api.h: No such file` | `sit_api_include.h` | Test include path missing `-I.` from workspace root | Fix `compile_focused_tests.bat` include flags |
| `SITUATION_FAILURE` undeclared | `kt_render_sit.h:31` | Constant removed from API | Replace `#define KTERM_FAILURE SITUATION_FAILURE` with `#define KTERM_FAILURE SITUATION_ERROR_INIT_FAILED` or a generic non-zero error |
| `SituationAudioGraph` unknown type | `kt_voice.h:96` | Forward declaration exists but not visible to kterm's isolated include chain | Add forward decl or ensure `situation_api.h` is included before `kt_voice.h` |
| `SituationNodeHandle` unknown type | `kt_voice.h:97` | Same as above | Same fix |
| `SITUATION_ERROR_INVALID_PARAM` undeclared | `kt_voice.h:210` | Error enum not included in kterm test path | Ensure `situation_base_errno.h` is reachable |
| `SituationStartAudioCaptureEx` pointer mismatch | `kt_voice.h:227` | Already fixed in this session (callback signature updated) | Verify fix propagated |
| `SituationGetActiveGraph` undeclared | `kt_voice.h:231` | Function exists in API but not visible to the test build | Include path fix |
| `SituationCreateNode` undeclared | `kt_voice.h:234` | Same | Same |
| `SITUATION_NODE_PCM_INPUT` undeclared | `kt_voice.h:234` | Same | Same |
| `SituationPushNodePCM` undeclared | `kt_voice.h:427` | Same | Same |

## Analysis: What's Missing on the Situation Side

**Nothing is missing.** All APIs referenced by kterm exist in `situation_api.h`:
- `SituationAudioGraph`, `SituationNodeHandle` — declared
- `SituationCreateNode`, `SituationDestroyNode` — declared and implemented
- `SITUATION_NODE_PCM_INPUT` — in `SituationNodeType` enum
- `SituationPushNodePCM`, `SituationGetNodePCMFreeFrames` — declared
- `SituationGetActiveGraph` — declared
- `SituationReadTexture`, `SituationReadFramebuffer`, `SituationTextureReadbackDesc`, `SituationReadPixelsDesc`, `SIT_TEXTURE_READ_RGBA8` — all present
- `SituationGetTextureInfo`, `SituationTextureInfo` — present (field renamed `internal_format` → `format`, already fixed)

**One item to verify on Situation side:**
- `SituationPushNodePCM` and `SituationGetNodePCMFreeFrames` are **declared** in `situation_api.h` but need verification that they have a working **implementation** in `situation_impl_audio.h`. If not implemented, the linker will fail even after include fixes.

## Tasks

### Phase 1: Include Path Fix (compile_focused_tests.bat)

- [x] Add `-I../../..` (workspace root) to the kterm focused test compile command so `sit/situation_api.h` resolves
- [x] Add `-DSITUATION_USE_OPENGL` (or appropriate backend flag) so Situation API sections guarded by backend are visible
- [x] Add `-DSITUATION_ENABLE_THREADING` so threading types are visible
- [x] Verify `situation_base_errno.h` is reachable through the include chain

### Phase 2: kt_render_sit.h Constant Fix

- [x] Replace `#define KTERM_FAILURE SITUATION_FAILURE` with `#define KTERM_FAILURE (-1)` (generic non-success, no dependency on removed constant)
- [x] Verify all `KTERM_SUCCESS` / `KTERM_FAILURE` usage still makes sense

### Phase 3: kt_voice.h Type Visibility

- [x] Ensure `kt_voice.h` is included after `situation_api.h` in the include chain (it already is for `kterm_console.c` but verify for the test harness)
- [x] Alternatively, add a minimal forward declaration block at the top of `kt_voice.h`:
  ```c
  #ifndef SITUATION_API_H
  typedef struct SituationAudioGraph SituationAudioGraph;
  typedef uint32_t SituationNodeHandle;
  #endif
  ```
- [x] Verify `SituationGetActiveGraph`, `SituationCreateNode`, `SituationDestroyNode`, `SituationPushNodePCM` are all visible

### Phase 4: Verify PCM Input Node Implementation

- [x] Check `situation_impl_audio.h` for `SITUATION_NODE_PCM_INPUT` process function
- [x] If missing: implement the ring buffer node (see `doc/plan/PCM_INPUT_NODE_PLAN.md`)
- [x] If present: verify it links correctly when kterm is built

### Phase 5: Rebuild and Verify

- [x] Run `compile_focused_tests.bat` — expect 0 errors
- [x] Run `run_focused_tests.bat --no-color` — expect 258+ passed, 0 failed
- [x] Run `build_examples.bat opengl kterm_console` — expect SUCCESS
- [ ] Run `kterm_console.exe` briefly to verify runtime (window opens, terminal renders, no crash)

### Phase 6: Test Harness Integration (Optional/Future)

- [ ] Consider adding a `kterm` module to the main `sit_test.exe` harness (would require linking against both DLL and kterm code)
- [ ] Or: add kterm focused test run to a CI script alongside `sit_test.exe`

## Notes

- The kterm test harness is already at high quality (258 tests, 18 modules, fully planned through Phase 12). This plan is purely about **recompilation alignment**, not test coverage gaps.
- The `kterm_console.c` example already compiles clean after today's fixes (v2.4.197).
- The kterm focused test harness uses a white-box model (includes `kterm_impl.h` directly) so it needs the full Situation header chain available.
