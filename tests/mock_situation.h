#ifndef MOCK_SITUATION_H
#define MOCK_SITUATION_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock Control Flags
// Use weak linkage or static to avoid link errors in single-file usage
typedef int SituationError;
static bool mock_fail_texture_creation = false;
static char last_clipboard_text[4096]; // Buffer for verification

// Mock Key State
#define MOCK_KEY_QUEUE_SIZE 64
static int mock_key_queue[MOCK_KEY_QUEUE_SIZE];
static int mock_key_head = 0;
static int mock_key_tail = 0;
static bool mock_key_down_state[512] = {false};

static void MockPressKey(int key) {
    if ((mock_key_tail + 1) % MOCK_KEY_QUEUE_SIZE != mock_key_head) {
        mock_key_queue[mock_key_tail] = key;
        mock_key_tail = (mock_key_tail + 1) % MOCK_KEY_QUEUE_SIZE;
    }
    if (key >= 0 && key < 512) mock_key_down_state[key] = true;
}

static void MockReleaseKey(int key) {
    if (key >= 0 && key < 512) mock_key_down_state[key] = false;
}

static void MockResetKeys(void) {
    mock_key_head = 0;
    mock_key_tail = 0;
    memset(mock_key_down_state, 0, sizeof(mock_key_down_state));
}

// Mock basic Situation types
typedef struct {
    unsigned char r, g, b, a;
} Color;

typedef union {
    struct { float x, y; };
    float v[2];
} Vector2;

typedef struct {
    unsigned int id;
    int width;
    int height;
    int mipmaps;
    int format;
    int channels; // Added
    unsigned char* data;
} SituationImage;

typedef struct {
    unsigned int id;
    int width;
    int height;
    int mipmaps;
    int format;
    int generation;
} SituationTexture;

typedef struct {
    unsigned int id;
} SituationBuffer;

typedef struct {
    unsigned int id;
} SituationComputePipeline;

typedef struct {
    void* handle;
} SituationCommandBuffer;

typedef struct {
    int window_width;
    int window_height;
    const char* window_title;
    int initial_active_window_flags;
} SituationInitInfo;

// Mock Constants
#define SITUATION_SUCCESS 0
#define SITUATION_FAILURE 1
#define SITUATION_TEXTURE_USAGE_SAMPLED 1
#define SITUATION_TEXTURE_USAGE_STORAGE 2
#define SITUATION_TEXTURE_USAGE_TRANSFER_SRC 4
#define SITUATION_TEXTURE_USAGE_TRANSFER_DST 8
#define SITUATION_BUFFER_USAGE_STORAGE_BUFFER 1
#define SITUATION_BUFFER_USAGE_TRANSFER_DST 2
#define SITUATION_BARRIER_COMPUTE_SHADER_WRITE 1
#define SITUATION_BARRIER_COMPUTE_SHADER_READ 2
#define SITUATION_BARRIER_TRANSFER_READ 4
#define SIT_COMPUTE_LAYOUT_TERMINAL 0
#define SIT_COMPUTE_LAYOUT_VECTOR 1
#define SIT_COMPUTE_LAYOUT_SIXEL 2
#define GLFW_MOUSE_BUTTON_LEFT 0
#define GLFW_MOUSE_BUTTON_MIDDLE 1
#define GLFW_MOUSE_BUTTON_RIGHT 2
#define SIT_KEY_LEFT_CONTROL 341
#define SIT_KEY_RIGHT_CONTROL 345
#define SIT_KEY_LEFT_ALT 342
#define SIT_KEY_RIGHT_ALT 346
#define SIT_KEY_LEFT_SHIFT 340
#define SIT_KEY_RIGHT_SHIFT 344
#define SIT_KEY_UP 265
#define SIT_KEY_DOWN 264
#define SIT_KEY_LEFT 263
#define SIT_KEY_RIGHT 262
#define SIT_KEY_PAGE_UP 266
#define SIT_KEY_PAGE_DOWN 267
// Keys added for compilation
#define SIT_KEY_F1 290
#define SIT_KEY_F2 291
#define SIT_KEY_F3 292
#define SIT_KEY_F4 293
#define SIT_KEY_F5 294
#define SIT_KEY_F6 295
#define SIT_KEY_F7 296
#define SIT_KEY_F8 297
#define SIT_KEY_F9 298
#define SIT_KEY_F10 299
#define SIT_KEY_F11 300
#define SIT_KEY_F12 301
#define SIT_KEY_F13 302
#define SIT_KEY_F14 303
#define SIT_KEY_F15 304
#define SIT_KEY_F16 305
#define SIT_KEY_F17 306
#define SIT_KEY_F18 307
#define SIT_KEY_F19 308
#define SIT_KEY_F20 309
#define SIT_KEY_F21 310
#define SIT_KEY_F22 311
#define SIT_KEY_F23 312
#define SIT_KEY_F24 313
#define SIT_KEY_ENTER 257
#define SIT_KEY_BACKSPACE 259
#define SIT_KEY_DELETE 261
#define SIT_KEY_TAB 258
#define SIT_KEY_ESCAPE 256
#define SIT_KEY_HOME 268
#define SIT_KEY_END 269
#define SIT_KEY_INSERT 260
#define SIT_KEY_KP_0 320
#define SIT_KEY_KP_1 321
#define SIT_KEY_KP_2 322
#define SIT_KEY_KP_3 323
#define SIT_KEY_KP_4 324
#define SIT_KEY_KP_5 325
#define SIT_KEY_KP_6 326
#define SIT_KEY_KP_7 327
#define SIT_KEY_KP_8 328
#define SIT_KEY_KP_9 329
#define SIT_KEY_KP_DECIMAL 330
#define SIT_KEY_KP_DIVIDE 331
#define SIT_KEY_KP_MULTIPLY 332
#define SIT_KEY_KP_SUBTRACT 333
#define SIT_KEY_KP_ADD 334
#define SIT_KEY_KP_ENTER 335
#define SIT_KEY_KP_EQUAL 336
#define SIT_KEY_A 65
#define SIT_KEY_Z 90
#define SIT_KEY_0 48
#define SIT_KEY_9 57
#define SIT_KEY_SPACE 32
#define SIT_KEY_LEFT_BRACKET 91
#define SIT_KEY_BACKSLASH 92
#define SIT_KEY_RIGHT_BRACKET 93
#define SIT_KEY_GRAVE_ACCENT 96
#define SIT_KEY_MINUS 45

#define SITUATION_SCALING_INTEGER 0
#define SITUATION_BLEND_ALPHA 0
#define SITUATION_WINDOW_STATE_RESIZABLE 1

// Mock Functions
static inline void SituationSetWindowTitle(const char* title) { (void)title; }
static double mock_current_time = 0.0;
static inline void MockSetTime(double t) { mock_current_time = t; }

static inline double SituationGetFrameTime(void) { return 0.016; }
static inline bool SituationTimerGetOscillatorState(int ms) { (void)ms; return true; }
static inline double SituationTimerGetTime(void) { return mock_current_time; }
static inline int SituationLoadFileData(const char* fileName, unsigned int* bytesRead, unsigned char** data) { return SITUATION_FAILURE; }
static inline void SituationCreateBuffer(size_t size, void* data, int usage, SituationBuffer* buffer) { buffer->id = 1; }
static inline void SituationUpdateBuffer(SituationBuffer buffer, size_t offset, size_t size, const void* data) {}
static inline void SituationDestroyBuffer(SituationBuffer* buffer) { buffer->id = 0; }
static inline int SituationCreateImage(int width, int height, int channels, SituationImage* image) {
    image->width = width; image->height = height; image->channels = channels;
    image->data = (unsigned char*)calloc(width * height * channels, 1);
    return SITUATION_SUCCESS;
}
static inline void SituationUnloadImage(SituationImage image) { if(image.data) free(image.data); }

static inline void SituationCreateTexture(SituationImage image, bool genMips, SituationTexture* texture) {
    if (mock_fail_texture_creation) {
        texture->id = 0; // Failure
    } else {
        texture->id = 1;
        texture->generation++;
    }
}
static inline void SituationCreateTextureEx(SituationImage image, bool genMips, int usage, SituationTexture* texture) {
    if (mock_fail_texture_creation) {
        texture->id = 0; // Failure
    } else {
        texture->id = 1;
        texture->generation++;
    }
}
static inline void SituationDestroyTexture(SituationTexture* texture) { texture->id = 0; }
static inline void SituationCreateComputePipelineFromMemory(const char* shaderCode, int layout, SituationComputePipeline* pipeline) { pipeline->id = 1; }
static inline void SituationDestroyComputePipeline(SituationComputePipeline* pipeline) { pipeline->id = 0; }
static inline uint64_t SituationGetBufferDeviceAddress(SituationBuffer buffer) { return 1000; }
static inline uint64_t SituationGetTextureHandle(SituationTexture texture) { return 2000; }
static inline bool SituationAcquireFrameCommandBuffer(void) { return false; } // Don't run draw commands in tests
static inline SituationCommandBuffer SituationGetMainCommandBuffer(void) { SituationCommandBuffer cmd = {0}; return cmd; }
static inline SituationError SituationCmdBindComputePipeline(SituationCommandBuffer cmd, SituationComputePipeline pipeline) { return SITUATION_SUCCESS; }
static inline SituationError SituationCmdBindComputeTexture(SituationCommandBuffer cmd, int binding, SituationTexture texture) { return SITUATION_SUCCESS; }
static inline SituationError SituationCmdSetPushConstant(SituationCommandBuffer cmd, int offset, const void* data, size_t size) { return SITUATION_SUCCESS; }
static inline SituationError SituationCmdDispatch(SituationCommandBuffer cmd, int x, int y, int z) { return SITUATION_SUCCESS; }
static inline SituationError SituationCmdPipelineBarrier(SituationCommandBuffer cmd, int src, int dst) { return SITUATION_SUCCESS; }
static inline SituationError SituationCmdPresent(SituationCommandBuffer cmd, SituationTexture texture) { return SITUATION_SUCCESS; }
static inline void SituationEndFrame(void) {}
static inline void SituationHideCursor(void) {}
static inline void SituationShowCursor(void) {}
static inline Vector2 SituationGetMousePosition(void) { Vector2 v = {{0,0}}; return v; }
static inline float SituationGetMouseWheelMove(void) { return 0.0f; }
static inline bool SituationIsMouseButtonDown(int button) { return false; }
static inline bool SituationIsMouseButtonPressed(int button) { return false; }
static inline bool SituationIsMouseButtonReleased(int button) { return false; }
static inline bool SituationIsKeyDown(int key) {
    if (key >= 0 && key < 512) return mock_key_down_state[key];
    return false;
}
static inline int SituationGetCharPressed(void) { return 0; }
static inline int SituationGetKeyPressed(void) {
    if (mock_key_head == mock_key_tail) return 0;
    int key = mock_key_queue[mock_key_head];
    mock_key_head = (mock_key_head + 1) % MOCK_KEY_QUEUE_SIZE;
    return key;
}
static inline bool SituationIsKeyPressed(int key) { return false; }
static inline void SituationRestoreWindow(void) {}
static inline void SituationMinimizeWindow(void) {}
static inline void SituationSetWindowPosition(int x, int y) {}
static inline void SituationSetWindowSize(int w, int h) {}
static inline void SituationSetWindowFocused(void) {}
static inline void SituationMaximizeWindow(void) {}
static inline bool SituationIsWindowFullscreen(void) { return false; }
static inline void SituationToggleFullscreen(void) {}
static inline int SituationGetScreenHeight(void) { return 1080; }
static inline int SituationGetScreenWidth(void) { return 1920; }
static inline int SituationGetClipboardText(const char** text) { *text = NULL; return SITUATION_FAILURE; }
static inline void SituationSetClipboardText(const char* text) {
    if (text) strncpy(last_clipboard_text, text, sizeof(last_clipboard_text) - 1);
    else last_clipboard_text[0] = '\0';
}
static inline void SituationFreeString(char* text) {}
static inline int SituationCreateVirtualDisplay(Vector2 size, float scale, int flags, int scaling, int blend, int* id) { *id=1; return SITUATION_SUCCESS; }
static inline bool SituationHasWindowFocus(void) { return true; }
static inline bool SituationIsWindowResized(void) { return false; }
static inline void SituationGetWindowSize(int* width, int* height) { if(width) *width=800; if(height) *height=600; }

static inline void SituationInit(int flags, const char* title, SituationInitInfo* info) { (void)flags; (void)title; (void)info; }
static inline void SituationSetTargetFPS(int fps) { (void)fps; }
static inline void SituationBeginFrame(void) {}
static inline void SituationShutdown(void) {}

#define SIT_FREE(p) free(p)

// Define Shaders for Compilation
#define TERMINAL_COMPUTE_SHADER_SRC ""
#define VECTOR_COMPUTE_SHADER_SRC ""
#define SIXEL_COMPUTE_SHADER_SRC ""

#endif
#define SIT_KEY_LAST 512
