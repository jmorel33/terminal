#ifndef KT_RENDER_SIT_H
#define KT_RENDER_SIT_H

#ifdef KTERM_TESTING
#include "tests/mock_situation.h"
#else
#include "situation.h"
#endif

// =============================================================================
// RENDER ADAPTER (Situation -> KTerm)
// =============================================================================
// This adapter abstracts the underlying platform/engine (Situation) from the K-Term core.
// It allows for easier porting or mocking in the future while currently aliasing
// directly to the Situation library.

// --- Types ---
typedef SituationComputePipeline KTermPipeline;
typedef SituationBuffer KTermBuffer;
typedef SituationTexture KTermTexture;
typedef SituationImage KTermImage;
typedef SituationCommandBuffer KTermCommandBuffer;

typedef Vector2 KTermVector2;
typedef Color KTermColor;

// --- Constants ---
#define KTERM_SUCCESS SITUATION_SUCCESS
#define KTERM_FAILURE (-1)

#define KTERM_TEXTURE_USAGE_SAMPLED SITUATION_TEXTURE_USAGE_SAMPLED
#define KTERM_TEXTURE_USAGE_STORAGE SITUATION_TEXTURE_USAGE_STORAGE
#define KTERM_TEXTURE_USAGE_TRANSFER_SRC SITUATION_TEXTURE_USAGE_TRANSFER_SRC
#define KTERM_TEXTURE_USAGE_TRANSFER_DST SITUATION_TEXTURE_USAGE_TRANSFER_DST
#define KTERM_TEXTURE_USAGE_COMPUTE_SAMPLED SITUATION_TEXTURE_USAGE_COMPUTE_SAMPLED

#define KTERM_BUFFER_USAGE_STORAGE_BUFFER SITUATION_BUFFER_USAGE_STORAGE_BUFFER
#define KTERM_BUFFER_USAGE_TRANSFER_DST SITUATION_BUFFER_USAGE_TRANSFER_DST
#define KTERM_BUFFER_USAGE_STORAGE_COMPUTE SITUATION_BUFFER_USAGE_STORAGE_COMPUTE

#define KTERM_BARRIER_COMPUTE_SHADER_WRITE SITUATION_BARRIER_COMPUTE_SHADER_WRITE
#define KTERM_BARRIER_COMPUTE_SHADER_READ SITUATION_BARRIER_COMPUTE_SHADER_READ
#define KTERM_BARRIER_TRANSFER_READ SITUATION_BARRIER_TRANSFER_READ

#define KTERM_COMPUTE_LAYOUT_TERMINAL SIT_COMPUTE_LAYOUT_TERMINAL
#define KTERM_COMPUTE_LAYOUT_VECTOR SIT_COMPUTE_LAYOUT_VECTOR
#define KTERM_COMPUTE_LAYOUT_SIXEL SIT_COMPUTE_LAYOUT_TERMINAL  // Sixel layout not in current API

#define KTERM_SCALING_INTEGER SITUATION_SCALING_INTEGER
#define KTERM_BLEND_ALPHA SITUATION_BLEND_ALPHA
#define KTERM_WINDOW_STATE_RESIZABLE SITUATION_WINDOW_STATE_RESIZABLE

// --- Functions ---
#define KTerm_CreateBuffer SituationCreateBuffer
#define KTerm_UpdateBuffer SituationUpdateBuffer
#define KTerm_DestroyBuffer SituationDestroyBuffer

#define KTerm_CreateImage SituationCreateImage
#define KTerm_UnloadImage SituationUnloadImage

#define KTerm_CreateTexture SituationCreateTexture
#define KTerm_CreateTextureEx SituationCreateTextureEx
#define KTerm_DestroyTexture SituationDestroyTexture
#define KTerm_GetTextureHandle SituationGetTextureHandle
#define KTerm_GetTextureInfo SituationGetTextureInfo
#define KTerm_SetTextureSamplerParams SituationSetTextureSamplerParams
#define KTerm_ReadTexture SituationReadTexture
#define KTerm_ReadTextureAlloc SituationReadTextureAlloc
#define KTerm_ReadFramebuffer SituationReadFramebuffer
typedef SituationTextureInfo KTermTextureInfo;
typedef SituationTextureReadbackDesc KTermTextureReadbackDesc;
typedef SituationReadPixelsDesc KTermReadPixelsDesc;
#define KTERM_TEXTURE_READ_RGBA8 SIT_TEXTURE_READ_RGBA8

#define KTerm_CreateComputePipeline SituationCreateComputePipelineFromMemory
#define KTerm_DestroyPipeline SituationDestroyComputePipeline

#define KTerm_GetBufferAddress SituationGetBufferDeviceAddress

#define KTerm_AcquireFrameCommandBuffer SituationAcquireFrameCommandBuffer
#define KTerm_GetCommandBuffer SituationGetMainCommandBuffer
#define KTerm_EndFrame SituationEndFrame

// Wrapper for SituationCmdBindComputePipeline (maps SituationError to KTERM_SUCCESS/KTERM_FAILURE)
static inline int KTerm_CmdBindPipeline(SituationCommandBuffer cmd, SituationComputePipeline pipeline) {
    SituationError err = SituationCmdBindComputePipeline(cmd, pipeline);
    return (err == SITUATION_SUCCESS) ? KTERM_SUCCESS : KTERM_FAILURE;
}

#define KTerm_CmdBindTexture SituationCmdBindComputeTexture
#define KTerm_CmdBindSampledTexture SituationCmdBindSampledTexture
#define KTerm_CmdBindBuffer SituationCmdBindComputeBuffer
#define KTerm_CmdSetPushConstant SituationCmdSetPushConstant
#if defined(SITUATION_USE_OPENGL)
static inline SituationError KTerm_CmdSetTerminalConstants(SituationCommandBuffer cmd, const void* data, size_t size) {
    return SituationCmdSetPushConstant(cmd, 4, data, size);
}
#else
static inline SituationError KTerm_CmdSetTerminalConstants(SituationCommandBuffer cmd, const void* data, size_t size) {
    return SituationCmdSetPushConstant(cmd, 0, data, size);
}
#endif
#define KTerm_CmdDispatch SituationCmdDispatch
#define KTerm_CmdPipelineBarrier SituationCmdPipelineBarrier

// --- Render Target (Virtual Display) Abstraction ---
// KTermRenderTarget wraps a Situation Virtual Display for compute-shader rendering.
// In standalone mode, falls back to direct presentation (no VD, no compositing).
typedef int KTermRenderTarget;
#define KTERM_RENDER_TARGET_INVALID (-1)

#ifndef KTERM_STANDALONE_MODE
// --- Virtual Display Mode (default): kterm renders into a compositable VD ---

/**
 * @brief Creates a compute-target virtual display and returns its writable texture.
 * @param width   Pixel width of the render target.
 * @param height  Pixel height of the render target.
 * @param out_target Receives the render target handle (VD ID).
 * @param out_texture Receives the KTermTexture handle for compute shader binding.
 * @return KTERM_SUCCESS on success, negative error code on failure.
 */
static inline int KTerm_CreateRenderTarget(int width, int height, KTermRenderTarget* out_target, KTermTexture* out_texture) {
    if (!out_target || !out_texture) return KTERM_FAILURE;
    *out_target = KTERM_RENDER_TARGET_INVALID;
    *out_texture = (KTermTexture){0};

    int vd_id = -1;
    Vector2 resolution = {{(float)width, (float)height}};
    SituationError err = SituationCreateVirtualDisplayEx(
        resolution, 1.0, 0,
        SITUATION_SCALING_INTEGER,
        SITUATION_BLEND_ALPHA,
        SITUATION_VD_FLAG_COMPUTE_TARGET,
        &vd_id
    );
    if (err != SITUATION_SUCCESS) return (int)err;

    SituationTexture tex = {0};
    err = SituationGetVirtualDisplayTexture(vd_id, &tex);
    if (err != SITUATION_SUCCESS) {
        SituationDestroyVirtualDisplay(vd_id);
        return (int)err;
    }

    *out_target = vd_id;
    *out_texture = tex;
    return KTERM_SUCCESS;
}

/**
 * @brief Destroys a render target (virtual display) and releases its resources.
 * @param target The render target handle.
 * @param texture Pointer to the output texture (zeroed on return). In VD mode, the VD owns the
 *               texture so this just zeroes the handle. In standalone mode, this destroys the texture.
 */
static inline void KTerm_DestroyRenderTarget(KTermRenderTarget target, KTermTexture* texture) {
    if (target != KTERM_RENDER_TARGET_INVALID) {
        SituationDestroyVirtualDisplay(target);
    }
    if (texture) *texture = (KTermTexture){0};
}

/**
 * @brief Marks the render target as dirty so the compositor knows to re-sample it.
 */
static inline void KTerm_MarkRenderTargetDirty(KTermRenderTarget target) {
    if (target != KTERM_RENDER_TARGET_INVALID) {
        SituationSetVirtualDisplayDirty(target, true);
    }
}

/**
 * @brief Composites all virtual displays (including kterm) to the swapchain.
 * Called by the host application after kterm's compute dispatches.
 */
#define KTerm_CompositeAndPresent SituationRenderVirtualDisplays

// Legacy: keep CmdPresent available for host code that needs direct blit
#define KTerm_CmdPresent SituationCmdPresent

#else
// --- Standalone Mode: kterm owns the frame, direct swapchain blit ---

static inline int KTerm_CreateRenderTarget(int width, int height, KTermRenderTarget* out_target, KTermTexture* out_texture) {
    if (!out_target || !out_texture) return KTERM_FAILURE;
    *out_target = 0; // Dummy valid handle

    // Create a standalone storage texture (the old path)
    SituationImage empty_img = {0};
    SituationError err = SituationCreateImage(width, height, 4, &empty_img);
    if (err != SITUATION_SUCCESS) return (int)err;

    err = SituationCreateTextureEx(empty_img, false,
        SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_SRC,
        out_texture);
    SituationUnloadImage(empty_img);
    return (int)err;
}

static inline void KTerm_DestroyRenderTarget(KTermRenderTarget target, KTermTexture* texture) {
    (void)target;
    // In standalone mode, we own the texture and must destroy it
    if (texture && texture->generation != 0) {
        SituationDestroyTexture(texture);
    }
}

static inline void KTerm_MarkRenderTargetDirty(KTermRenderTarget target) {
    (void)target; // No VD to mark
}

#define KTerm_CmdPresent SituationCmdPresent

#endif // KTERM_STANDALONE_MODE

#define KTerm_TimerGetOscillator SituationTimerGetOscillatorState
#define KTerm_TimerGetTime SituationTimerGetTime
#define KTerm_GetFrameTime SituationGetFrameTime

#define KTerm_LoadFileData SituationLoadFileData

#define KTerm_CreateVirtualDisplay SituationCreateVirtualDisplay
#define KTerm_CreateVirtualDisplayEx SituationCreateVirtualDisplayEx
#define KTerm_SetWindowTitlePlatform SituationSetWindowTitle

// Window Management & Clipboard
#define KTerm_SetClipboardText SituationSetClipboardText
#define KTerm_GetClipboardText SituationGetClipboardText
#define KTerm_FreeString SituationFreeString

#define KTerm_RestoreWindow SituationRestoreWindow
#define KTerm_MinimizeWindow SituationMinimizeWindow
#define KTerm_SetWindowPosition SituationSetWindowPosition
#define KTerm_SetWindowSize SituationSetWindowSize
#define KTerm_SetWindowFocused SituationSetWindowFocused
#define KTerm_MaximizeWindow SituationMaximizeWindow
#define KTerm_IsWindowFullscreen SituationIsWindowFullscreen
#define KTerm_ToggleFullscreen SituationToggleFullscreen
#define KTerm_GetScreenHeight SituationGetScreenHeight
#define KTerm_GetScreenWidth SituationGetScreenWidth

#define KTerm_Platform_Init SituationInit
#define KTerm_SetTargetFPS SituationSetTargetFPS
#define KTerm_BeginFrame SituationBeginFrame
#define KTerm_Platform_Shutdown SituationShutdown
typedef SituationInitInfo KTermInitInfo;

// Memory
#ifdef SIT_FREE
  #define KTERM_FREE SIT_FREE
#else
  #define KTERM_FREE free
#endif

#endif // KTERM_RENDER_SIT_H
