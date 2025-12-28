# The "Situation" Advanced Platform Awareness, Control, and Timing

_Core API library v2.3.36 "Velocity"_

_(c) 2025 Jacques Morel_

_MIT Licenced_

Welcome to "Situation", a public API engineered for high-performance, cross-platform development. "Situation" is a single-file, cross-platform C/C++ library providing unified, low-level access and control over essential application subsystems. Its purpose is to abstract away platform-specific complexities, offering a lean yet powerful API for building sophisticated, high-performance software. This library is designed as a foundational layer for professional applications, including but not limited to: real-time simulations, game engines, multimedia installations, and scientific visualization tools. We are actively seeking contributions from the community to help us build a truly exceptional and robust platform.

Our immediate development roadmap is focused on several key areas:
*   **Full Thread-Safety**: We are working towards making the entire library thread-safe, which will allow for even greater performance and scalability in multi-threaded applications.
*   **OpenGL Command Buffer Enhancement**: We are committed to ensuring the proper functioning of OpenGL within our command buffer system, providing a seamless and efficient rendering experience.
*   **Comprehensive Vulkan Abstraction**: Our goal is to provide a full-featured, high-level wrapper for the Vulkan API, simplifying its complexity while retaining its power and performance.
*   **Robust Virtual Display and Windows Integration**: We are continuously improving our virtual display system and its integration with Windows to ensure bullet-proof stability across diverse hardware configurations, including multi-monitor setups and various application states (e.g., switching, pausing).

"Situation" is an ambitious project that aims to become a premier, go-to solution for developers seeking a reliable and powerful platform layer. We encourage you to explore the library, challenge its capabilities, and contribute to its evolution.

The library's philosophy is reflected in its name, granting developers complete situational "Awareness," precise "Control," and fine-grained "Timing."

It provides deep **Awareness** of the host system through APIs for querying hardware and multi-monitor display information, and by handling operating system events like window focus and file drops.

This foundation enables precise **Control** over the entire application stack, from window management (fullscreen, borderless) and input devices (keyboard, mouse, gamepad) to a comprehensive audio pipeline with playback, capture, and real-time effects. This control extends to the graphics and compute pipeline, abstracting modern OpenGL and Vulkan through a unified command-buffer model. It offers simplified management of GPU resources—such as shaders, meshes, and textures—and includes powerful utilities for high-quality text rendering and robust filesystem I/O.

Finally, its **Timing** capabilities range from high-resolution performance measurement and frame rate management to an advanced **Temporal Oscillator System** for creating complex, rhythmically synchronized events. By handling the foundational boilerplate of platform interaction, "Situation" empowers developers to focus on core application logic, enabling the creation of responsive and sophisticated software—from games and creative coding projects to data visualization tools—across all major desktop platforms.

---

# Situation v2.3.36 API Programming Guide

"Situation" is a single-file, cross-platform C/C++ library designed for advanced platform awareness, control, and timing. It provides a comprehensive, immediate-mode API that abstracts the complexities of windowing, graphics (OpenGL/Vulkan), audio, and input. This guide serves as the primary technical manual for the library, detailing its architecture, usage patterns, and the complete Application Programming Interface (API).

---

<details open>
<summary><h2>Introduction and Core Concepts</h2></summary>

This section explains the fundamental concepts, design philosophy, and core architectural patterns of the "Situation" library. A solid understanding of these ideas is crucial for using the library effectively.

### 1. Core Philosophy and Design
#### Immediate Mode and Explicit Control
The library favors a mostly **"immediate mode"** style API. This means that for many operations, you call a function and it takes effect immediately within the current frame. For example, `SituationCmdDrawQuad()` directly records a draw command into the current frame's command buffer. This approach is designed to be simple, intuitive, and easy to debug, contrasting with "retained mode" systems where you would build a scene graph of objects that is then rendered by the engine.

Complementing this is a philosophy of **explicit resource management**. Any resource you create (a texture, a mesh, a sound) must be explicitly destroyed by you using its corresponding `SituationDestroy...()` or `SituationUnload...()` function. This design choice avoids the complexities and performance overhead of automatic garbage collection and puts you in full control of resource lifecycles. To aid in debugging, the library will warn you at shutdown if you've leaked any GPU resources.

#### C-Style, Single-File, and Backend-Agnostic Architecture
- **C-Style, Data-Oriented API:** The API is pure C, promoting maximum portability and interoperability. It uses handles (structs passed by value) to represent opaque resources and pointers for functions that need to modify or destroy those resources. This approach is data-oriented, focusing on transforming data (e.g., vertex data into a mesh, image data into a texture) rather than on object-oriented hierarchies.
- **Single-File, Header-Only Distribution:** "Situation" is distributed as a single header file (`situation.h`), making it incredibly easy to integrate into your projects. To use it, you simply `#include "situation.h"` in your source files. In exactly one C or C++ file, you must first define `SITUATION_IMPLEMENTATION` before the include to create the implementation.
- **Backend Abstraction:** The library provides a unified API that works over different graphics backends (currently OpenGL and Vulkan). You choose the backend at compile time by defining either `SITUATION_USE_OPENGL` or `SITUATION_USE_VULKAN`.

#### Strictly Single-Threaded Model
The library is **strictly single-threaded**. All API functions must be called from the same thread that called `SituationInit()`. Asynchronous operations, such as asset loading on a worker thread, must be handled by the user with care, ensuring that no `SITAPI` calls are made from outside the main thread.

### 2. Application Structure
#### The Application Lifecycle
The library enforces a strict and predictable lifecycle:
1.  **Initialization:** Call `SituationInit()` exactly once at the beginning. This sets up all subsystems. No other library functions should be called before this.
2.  **Main Loop:** After initialization, your application enters a loop that continues as long as `SituationWindowShouldClose()` returns `false`. This is where all real-time processing occurs.
3.  **Shutdown:** When the main loop terminates, you must call `SituationShutdown()` exactly once to gracefully tear down all subsystems and free resources.

#### The Three-Phase Frame
To ensure stable and predictable behavior, every iteration of the main loop must be divided into three distinct phases, executed in a specific order:
1.  **Input Phase:** At the very beginning of the frame, call `SituationPollInputEvents()`. This gathers all pending input from the operating system, ensuring that all logic in the frame operates on a consistent snapshot of input.
2.  **Update Phase:** Next, call `SituationUpdateTimers()` to calculate `deltaTime`. Immediately after, execute all of your application's logic (physics, AI, state changes). Using `deltaTime` is crucial for creating frame-rate-independent behavior.
3.  **Render Phase:** Finally, perform all rendering. This phase begins with `SituationAcquireFrameCommandBuffer()`, followed by recording all drawing commands, and concludes with `SituationEndFrame()`, which submits the work to the GPU.

### 3. Core API Patterns
#### Handles vs. Pointers
The API uses two patterns for interacting with objects:
- **Handles (by value):** Opaque structs like `SituationMesh` or `SituationShader` are typically passed by value to drawing or binding functions (e.g., `SituationCmdDrawMesh(my_mesh)`). These are lightweight identifiers for GPU resources.
- **Pointers (for modification):** When a function needs to modify or destroy a resource, you must pass a pointer to its handle (e.g., `SituationDestroyMesh(&my_mesh)`). This allows the function to invalidate the handle by setting its internal ID to 0, preventing accidental use after destruction.

#### Input Handling: Polling vs. Callbacks
The library offers two complementary models for handling input:
1.  **State Polling (`SituationIs...Down`, `SituationIs...Pressed`)**: This is the most common approach for real-time applications. Within your main `Update` phase, you can query the current state of any key or button. This is ideal for continuous actions (character movement) or single-trigger game events (jumping, shooting).
2.  **Event-Driven Callbacks (`SituationSet...Callback`)**: This model allows you to register callback functions that are invoked the moment an input event occurs. This is more efficient for UI interactions, text input, or other event-driven logic, as it avoids the need to check for input every single frame.

### 4. Rendering and Graphics
#### The Command Buffer
At the core of the rendering system is the **command buffer**. Rather than telling the GPU to "draw this now," you record a series of commands (prefixed with `SituationCmd...`) into a buffer. Once all commands for a frame are recorded, `SituationEndFrame()` submits the entire buffer to the GPU for execution. This batching approach is far more efficient and is central to how modern graphics APIs operate.

#### CPU-Side vs. GPU-Side Resources
The library makes a clear distinction between resources in system memory (CPU) and video memory (GPU).
- **`SituationImage` (CPU):** A block of pixel data in RAM. The Image module functions operate on this data, allowing for flexible manipulation (resizing, drawing text, etc.) without GPU overhead.
- **`SituationTexture` (GPU):** A GPU resource created by uploading a `SituationImage`. This is the object used by shaders for rendering.
The typical workflow is to load/generate a `SituationImage`, perform all desired manipulations, and then upload it once to a `SituationTexture` for efficient rendering.

#### Logical vs. Physical Coordinates (High-DPI)
Modern displays often have a high pixel density (High-DPI). The library abstracts this complexity:
-   **Logical Size (Screen Coordinates):** Dimensions used by the OS for window sizing and positioning. Functions like `SituationGetScreenWidth()` and `SituationGetMousePosition()` operate in this space. Use this for UI layout and logic.
-   **Physical Size (Render Pixels):** The actual number of pixels in the framebuffer (`SituationGetRenderWidth()`). This is the resolution the GPU renders to.
The library automatically handles this scaling. You can query the scaling factor using `SituationGetWindowScaleDPI()`.

#### The Virtual Display System
A "Virtual Display" is an **off-screen render target**. Instead of drawing directly to the main window, you can render a scene into a virtual display. This is incredibly powerful for post-processing effects (bloom, blur), UI layering (rendering UI at a fixed resolution), and caching parts of a scene that don't change frequently.

### 5. Other Key Systems
#### Audio: Sounds vs. Streams
The audio module can handle audio in two ways:
-   **Loaded Sounds (`SituationLoadSoundFromFile`):** Decodes the entire audio file into memory. Ideal for short, low-latency sound effects.
-   **Streamed Sounds (`SituationLoadSoundFromStream`):** Decodes the audio in small chunks as it's playing. Uses significantly less memory, making it perfect for long background music tracks.

#### Filesystem: Cross-Platform and Special Paths
The filesystem module abstracts away OS-specific differences. All paths are UTF-8. To ensure your application is portable, use the provided helper functions instead of hardcoding paths:
-   `SituationGetBasePath()`: Returns the directory containing your executable. Use this for loading application assets.
-   `SituationGetAppSavePath()`: Returns a platform-appropriate, user-specific directory for saving configuration files and user data.

#### The Temporal Oscillator System
This is a high-level timing utility for creating rhythmic, periodic events. You can initialize oscillators with specific periods (e.g., 0.5 seconds for 120 BPM). The library updates these timers independent of the frame rate, allowing you to easily synchronize animations, game logic, or visual effects to a steady, musical beat using functions like `SituationTimerHasOscillatorUpdated()`.

</details>

---

<details>
<summary><h2>Building the Library</h2></summary>

### 2.1 Integration Models (Header-Only vs. Shared Library)
**A) Header-Only:**
- Add `situation.h` to your project.
- In *one* C/C++ source file (e.g., `sit_lib.c`), define `SITUATION_IMPLEMENTATION` *before* including `situation.h`.
```c
#define SITUATION_IMPLEMENTATION
#include "situation.h"
```
- Compile this source file with your project.

**B) Shared Library (DLL):**
- Create a separate source file (e.g., `sit_dll.c`).
- Define `SITUATION_IMPLEMENTATION` and `SITUATION_BUILD_SHARED`.
```c
#define SITUATION_IMPLEMENTATION
#define SITUATION_BUILD_SHARED
#include "situation.h"
```
- Compile this into a shared library (DLL/.so).
- In your main application, define `SITUATION_USE_SHARED` and include `situation.h`.
```c
#define SITUATION_USE_SHARED
#include "situation.h"
```
- Link your application against the generated library.

### 2.2 Project Structure Recommendations
```
your_project/
├── src/
│   ├── main.c              // Your application entry point
│   └── (other .c files)    // Your application logic
├── lib/
│   └── situation.h         // This library header
├── ext/                    // External dependencies (if not system-installed)
│   ├── glad/               // For OpenGL (if SITUATION_USE_OPENGL)
│   │   ├── glad.c
│   │   └── glad.h
│   ├── cglm/               // For math (if used)
│   │   └── ...             // cglm headers
│   ├── stb/                // For image loading (stb_image.h, etc.)
│   │   └── ...             // stb headers (define STB_IMAGE_IMPLEMENTATION in one .c file)
│   └── miniaudio/          // Audio library (miniaudio.h)
│       └── miniaudio.h     // (define MINIAUDIO_IMPLEMENTATION in one .c file)
├── assets/                 // Your application's assets
│   ├── models/
│   │   └── cube.obj
│   ├── textures/
│   │   └── diffuse.png
│   ├── shaders/
│   │   ├── basic.vert
│   │   ├── basic.frag
│   │   └── compute_filter.comp
│   └── audio/
│       └── background_music.wav
└── build/                  // Build output directory
```

### 2.3 Compilation Requirements & Dependencies
- A C99 or C++ compiler.
- **Required Dependencies (provided or system-installed):**
    - **GLFW3:** For windowing and input. Headers and library linking required.
    - **OpenGL Context Loader (e.g., GLAD):** If using `SITUATION_USE_OPENGL`. `glad.c` must be compiled.
    - **Vulkan SDK:** If using `SITUATION_USE_VULKAN`. Headers and linking required. Includes shaderc, VMA.
    - **cglm:** For math types and functions (vec3, mat4, etc.). Headers needed.
- **Optional Dependencies (for extra features):**
    - **stb_image.h, stb_image_write.h, stb_image_resize.h:** For image loading/saving/resizing. Define `STB_IMAGE_IMPLEMENTATION` etc. in one .c file.
    - **stb_truetype.h:** For styled text rendering (SDF generation). Define `STB_TRUETYPE_IMPLEMENTATION`.
    - **miniaudio.h:** For audio. Define `MINIAUDIO_IMPLEMENTATION` in one .c file.

### 2.4 Build & Feature Defines
This section details the preprocessor defines that control the library's features and build configuration.

#### Backend Selection
- **`SITUATION_USE_OPENGL`**: Enables the modern OpenGL backend. Must be defined before including `situation.h`.
- **`SITUATION_USE_VULKAN`**: Enables the explicit Vulkan backend. Must be defined before including `situation.h`.

#### Shared Library Support
- **`SITUATION_BUILD_SHARED`**: Must be defined when compiling the library as a shared object (DLL). This controls symbol visibility for export.
- **`SITUATION_USE_SHARED`**: Must be defined in the application code when linking against the shared library to control symbol import.

#### Feature Enablement
- **`SITUATION_ENABLE_SHADER_COMPILER`**: Mandatory for using compute shaders with the Vulkan backend as it enables runtime compilation of GLSL to SPIR-V.

</details>

---

<details>
<summary><h2>Getting Started</h2></summary>

Here is a minimal, complete example of a "Situation" application that opens a window, clears it to a blue color, and runs until the user closes it.

### Step 1: Include the Library
First, make sure `situation.h` is in your project's include path. In your main C file, define `SITUATION_IMPLEMENTATION` and include the header.

```c
#define SITUATION_IMPLEMENTATION
// Define a graphics backend before including the library
#define SITUATION_USE_OPENGL // or SITUATION_USE_VULKAN
#include "situation.h"

#include <stdio.h> // For printf
```

### Step 2: Initialize the Library
In your `main` function, you need to initialize the library. Create a `SituationInitInfo` struct to configure your application's startup properties, such as the window title and initial dimensions. Then, call `SituationInit()`.

```c
int main(int argc, char** argv) {
    SituationInitInfo init_info = {
        .app_name = "My First Situation App",
        .app_version = "1.0",
        .initial_width = 1280,
        .initial_height = 720,
        .window_flags = SITUATION_FLAG_WINDOW_RESIZABLE | SITUATION_FLAG_VSYNC_HINT,
        .target_fps = 60,
        .headless = false
    };

    if (SituationInit(argc, argv, &init_info) != SIT_SUCCESS) {
        printf("Failed to initialize Situation: %s\n", SituationGetLastErrorMsg());
        return -1;
    }
```

### Step 3: The Main Loop
The heart of your application is the main loop. This loop continues as long as the user has not tried to close the window (`!SituationWindowShouldClose()`). Inside the loop, you follow a strict three-phase structure: Input, Update, and Render.

```c
    while (!SituationWindowShouldClose()) {
        // --- 1. Input ---
        SituationPollInputEvents();

        // --- 2. Update ---
        SituationUpdateTimers();
        // Your application logic, physics, etc. would go here.
        if (SituationIsKeyPressed(SIT_KEY_ESCAPE)) {
            break; // Exit the loop
        }

        // --- 3. Render ---
        if (SituationAcquireFrameCommandBuffer()) {
            SituationRenderPassInfo pass_info = {
                .color_load_action = SIT_LOAD_ACTION_CLEAR,
                .clear_color = { .r = 0, .g = 12, .b = 24, .a = 255 }, // A dark blue
                .color_store_action = SIT_STORE_ACTION_STORE,
            };
            SituationCmdBeginRenderPass(SituationGetMainCommandBuffer(), &pass_info);
            // ... Drawing commands go here ...
            SituationCmdEndRenderPass(SituationGetMainCommandBuffer());
            SituationEndFrame();
        }
    }
```

### Step 4: Shutdown
After the main loop finishes, it is critical to call `SituationShutdown()` to clean up all resources.

```c
    SituationShutdown();
    return 0;
}
```

---

#### `SituationIsAppPaused`

Checks if the application is currently paused.

```c
SITAPI bool SituationIsAppPaused(void);
```

**Usage Example:**
```c
if (SituationIsAppPaused()) {
    printf("Application is paused.\n");
}
```

---

#### `SituationResumeApp`

Resumes the application.

```c
SITAPI void SituationResumeApp(void);
```

**Usage Example:**
```c
SituationResumeApp();
```

---

#### `SituationGetWindowScaleDPI`

Gets the DPI scaling factor for the window.

```c
SITAPI Vector2 SituationGetWindowScaleDPI(void);
```

**Usage Example:**
```c
Vector2 dpi_scale = SituationGetWindowScaleDPI();
printf("DPI scale: x=%.2f, y=%.2f\n", dpi_scale.x, dpi_scale.y);
```

---

#### `SituationGetWindowPosition`

Gets the position of the top-left corner of the window in screen coordinates.

```c
SITAPI Vector2 SituationGetWindowPosition(void);
```

**Usage Example:**
```c
Vector2 window_pos = SituationGetWindowPosition();
printf("Window position: x=%.0f, y=%.0f\n", window_pos.x, window_pos.y);
```

---

#### `SituationGetMonitorName`

Gets the human-readable name of a monitor.

```c
SITAPI const char* SituationGetMonitorName(int monitor_id);
```

**Usage Example:**
```c
const char* monitor_name = SituationGetMonitorName(0);
printf("Primary monitor name: %s\n", monitor_name);
```

---
#### `SituationUnloadDroppedFiles`
Unloads the file paths list loaded by `SituationLoadDroppedFiles`.
```c
void SituationUnloadDroppedFiles(char** files, int count);
```
**Usage Example:**
```c
// In the update loop
if (SituationIsFileDropped()) {
    int file_count;
    char** files = SituationLoadDroppedFiles(&file_count);
    for (int i = 0; i < file_count; i++) {
        printf("Dropped file: %s\n", files[i]);
    }
    // IMPORTANT: Always free the memory after you're done with the file list.
    SituationUnloadDroppedFiles(files, file_count);
}
```

---
#### `SituationSetWindowMonitor`
Sets the monitor that the window should be on. This is useful for multi-monitor setups.
```c
void SituationSetWindowMonitor(int monitor);
```
**Usage Example:**
```c
// Get the number of connected monitors
int monitor_count;
SituationGetDisplays(&monitor_count);

// If there is a second monitor, move the window to it.
if (monitor_count > 1) {
    SituationSetWindowMonitor(1); // Monitor indices are 0-based
}
```

---
#### `SituationGetWindowHandle`
Gets the native, platform-specific window handle.
```c
void* SituationGetWindowHandle(void);
```
**Usage Example:**
```c
// On Windows, this would return an HWND. On macOS, an NSWindow*.
// This is useful for interoperability with other libraries.
void* native_handle = SituationGetWindowHandle();
if (native_handle) {
    // Pass the handle to a library that needs it, e.g., an external UI toolkit.
}
```

### Full Example Code

```c
#define SITUATION_IMPLEMENTATION
#define SITUATION_USE_OPENGL // Or SITUATION_USE_VULKAN
#include "situation.h"

#include <stdio.h>

int main(int argc, char** argv) {
    // 1. Configure and Initialize
    SituationInitInfo init_info = {
        .app_name = "My First Situation App",
        .app_version = "1.0",
        .initial_width = 1280,
        .initial_height = 720,
        .window_flags = SITUATION_FLAG_WINDOW_RESIZABLE | SITUATION_FLAG_VSYNC_HINT,
    };
    if (SituationInit(argc, argv, &init_info) != SIT_SUCCESS) {
        printf("Failed to initialize Situation: %s\n", SituationGetLastErrorMsg());
        return -1;
    }

    // 2. Main Loop
    while (!SituationWindowShouldClose()) {
        SituationPollInputEvents();
        SituationUpdateTimers();
        if (SituationIsKeyPressed(SIT_KEY_ESCAPE)) break;

        if (SituationAcquireFrameCommandBuffer()) {
            SituationRenderPassInfo pass_info = {
                .color_load_action = SIT_LOAD_ACTION_CLEAR,
                .clear_color = {0, 12, 24, 255},
                .color_store_action = SIT_STORE_ACTION_STORE,
            };
            SituationCmdBeginRenderPass(SituationGetMainCommandBuffer(), &pass_info);
            SituationCmdEndRenderPass(SituationGetMainCommandBuffer());
            SituationEndFrame();
        }
    }

    // 3. Shutdown
    SituationShutdown();
    return 0;
}
```

---
</details>

---

## Detailed API Reference

This section provides a complete list of all functions available in the "Situation" library, organized by module.

<details>
<summary><h3>Core Module</h3></summary>

**Overview:** The Core module is the heart of the "Situation" library, providing the essential functions for application lifecycle management. It handles initialization (`SituationInit`) and shutdown (`SituationShutdown`), processes the main event loop, and manages frame timing and rate control. This module also serves as a gateway to crucial system information, offering functions to query hardware details, manage command-line arguments, and set up critical application-wide callbacks.

### Core Structs

#### `SituationInitInfo`
This struct is passed to `SituationInit()` to configure the application at startup. It allows for detailed control over the initial state of the window, rendering backend, and timing systems.

> **Titanium Tip:** Field names use strictly `snake_case`. Ensure you are not using legacy `camelCase` names (e.g. `windowWidth`) from older versions.

```c
typedef struct {
    // ── Window Creation Parameters ──
    int          window_width;              // Initial window width in screen coordinates
    int          window_height;             // Initial window height in screen coordinates
    const char*  window_title;              // Window title bar text (UTF-8)

    // ── Window State Flags (Applied via GLFW window hints or direct state changes) ──
    uint32_t     initial_active_window_flags;    // Flags when window has focus (e.g. SIT_WINDOW_BORDERLESS | SIT_WINDOW_VSYNC)
    uint32_t     initial_inactive_window_flags;  // Flags when window is unfocused (e.g. pause rendering or reduce refresh rate)

    // ── Vulkan-Specific Options ──
    bool         enable_vulkan_validation;       // Enable VK_LAYER_KHRONOS_validation (debug builds only - auto-disabled in release)
    bool         force_single_queue;             // Force shared compute/graphics queue (debug/compatibility)
    uint32_t     max_frames_in_flight;           // Override SITUATION_MAX_FRAMES_IN_FLIGHT (usually 2 or 3)

    // Optional: Provide custom Vulkan instance extensions (e.g. for VR, ray tracing, etc.)
    const char** required_vulkan_extensions;     // Array of extension names (null or empty = use defaults)
    uint32_t     required_vulkan_extension_count;// Length of the above array

    // ── Engine Feature Flags ──
    uint32_t     flags;  // Bitfield: SITUATION_INIT_AUDIO_CAPTURE_MAIN_THREAD

    // ── Audio Configuration ──
    uint32_t     max_audio_voices; // Max concurrent audio voices. 0 = Unlimited (Dynamic).

    int          render_thread_count; // Number of render threads to spawn (0 = Single Threaded)
    // [v2.3.22] Backpressure Policy: 0: Spin (Low Latency), 1: Yield (Balanced), 2: Sleep (Low CPU)
    int          backpressure_policy;

    // [v2.3.34] Async I/O
    uint32_t     io_queue_capacity; // Size of the IO queue (Low Priority). Default: 1024.
} SituationInitInfo;
```
-   `window_width`, `window_height`: The desired initial dimensions for the main window's client area.
-   `window_title`: The text to display in the window title bar.
-   `initial_active_window_flags`: A bitmask of `SituationWindowStateFlags` to set the initial state of the window when focused.
    -   **VSync Control:** To enable VSync, include the `SITUATION_FLAG_VSYNC_HINT` (or `SITUATION_WINDOW_STATE_VSYNC_HINT`) flag here. There is no separate boolean for VSync.
-   `initial_inactive_window_flags`: Flags to apply when the window loses focus (e.g., lower framerate).
-   `enable_vulkan_validation`: Enables Vulkan validation layers for debugging.
-   `io_queue_capacity`: The size of the low-priority IO queue for the dedicated IO thread.

**Note on Backend Selection:**
You do **not** select the graphics backend (OpenGL vs Vulkan) inside this struct. Instead, you must define either `SITUATION_USE_VULKAN` or `SITUATION_USE_OPENGL` in your code *before* including `situation.h`.

---
#### `SituationTimerSystem`
Manages all timing-related functionality for the application, including frame time (`deltaTime`), total elapsed time, and the Temporal Oscillator System. This struct is managed internally by the library; you interact with it through functions like `SituationGetFrameTime()` and `SituationTimerHasOscillatorUpdated()`.
```c
typedef struct SituationTimerSystem {
    double current_time;
    double previous_time;
    float frame_time;
    int target_fps;
    int oscillator_count;
    SituationTimerOscillator* oscillators;
} SituationTimerSystem;
```
-   `current_time`, `previous_time`: Internal timestamps used to calculate `frame_time`.
-   `frame_time`: The duration of the last frame in seconds (`deltaTime`).
-   `target_fps`: The current target frame rate.
-   `oscillator_count`: The number of active oscillators.
-   `oscillators`: A pointer to the internal array of oscillator states.

---
#### `SituationTimerOscillator`
Represents the internal state of a single temporal oscillator. This struct is managed by the library as part of the `SituationTimerSystem` and is not typically interacted with directly. Its properties are exposed through functions like `SituationTimerHasOscillatorUpdated()` and `SituationTimerGetOscillatorValue()`.
```c
typedef struct SituationTimerOscillator {
    double period;
    bool state;
    bool previous_state;
    double last_ping_time;
    uint64_t trigger_count;
} SituationTimerOscillator;
```
-   `period`: The duration of one full cycle of the oscillator in seconds.
-   `state`: The current binary state of the oscillator (`true` or `false`). This flips each time half of the `period` elapses.
-   `previous_state`: The state of the oscillator in the previous frame. Used to detect when the state has changed.
-   `last_ping_time`: An internal timestamp used by `SituationTimerPingOscillator()` to track time since the last successful "ping".
-   `trigger_count`: The total number of times the oscillator has flipped its state since initialization.

---
### Functions

#### `SituationInit`
Initializes all library subsystems. This is the entry point of the "Situation" library and **must be the first function you call**. It sets up the window, initializes the selected graphics backend (OpenGL or Vulkan), prepares the audio device, and processes any command-line arguments.

The function takes a pointer to a `SituationInitInfo` struct, which allows you to configure initial properties like window size, title, and desired frame rate. Passing `NULL` will initialize the library with default settings.
```c
SituationError SituationInit(int argc, char** argv, const SituationInitInfo* init_info);
```
**Usage Example:**
```c
int main(int argc, char* argv[]) {
    // Define the initial configuration for your application.
    SituationInitInfo init_info = {
        .app_name = "Core Example",
        .app_version = "1.0.0",
        .initial_width = 1280,
        .initial_height = 720,
        .window_flags = SITUATION_FLAG_WINDOW_RESIZABLE | SITUATION_FLAG_VSYNC_HINT,
        .target_fps = 60,
        .headless = false
    };

    // Initialize the library with the specified configuration.
    if (SituationInit(argc, argv, &init_info) != SIT_SUCCESS) {
        // If initialization fails, retrieve and print the error message.
        char* error_msg = SituationGetLastErrorMsg();
        fprintf(stderr, "Failed to initialize Situation: %s\n", error_msg);
        SituationFreeString(error_msg); // Free the error message string
        return -1;
    }

    // ... main application loop ...

    // Shutdown the library to release all resources.
    SituationShutdown();
    return 0;
}
```

---
#### `SituationPollInputEvents`
Polls for all pending input and window events from the operating system. This function is the first part of the mandatory three-phase frame structure and **must be called exactly once at the beginning of every frame**. It gathers all keyboard, mouse, gamepad, and window events (like resizing or closing) and stores them in an internal state buffer. All other input and window functions in the frame will operate on this consistent snapshot of the state.
```c
void SituationPollInputEvents(void);
```
**Usage Example:**
```c
while (!SituationWindowShouldClose()) {
    // --- 1. Input Phase ---
    SituationPollInputEvents(); // First call in the loop

    // --- 2. Update Phase ---
    SituationUpdateTimers();
    // ... game logic that relies on input ...

    // --- 3. Render Phase ---
    // ... rendering code ...
}
```

---
#### `SituationUpdateTimers`
Updates all internal timers. This is the second part of the mandatory three-phase frame structure and **must be called exactly once per frame**, immediately after `SituationPollInputEvents()`. This function calculates the `deltaTime` for the previous frame (accessible via `SituationGetFrameTime()`), updates the total elapsed time, and advances the state of the Temporal Oscillator System.
```c
void SituationUpdateTimers(void);
```
**Usage Example:**
```c
while (!SituationWindowShouldClose()) {
    // --- 1. Input Phase ---
    SituationPollInputEvents();

    // --- 2. Update Phase ---
    SituationUpdateTimers(); // Second call in the loop

    // Now it's safe to get the frame time for physics and logic.
    float dt = SituationGetFrameTime();
    player_position.x += player_velocity.x * dt;

    // --- 3. Render Phase ---
    // ... rendering code ...
}
```

---
#### `SituationShutdown`
Shuts down all library subsystems, releases all GPU and CPU resources, and closes the application window. This **must be the last function called** before your application exits. It ensures a graceful cleanup and will report any leaked GPU resources (in debug mode) if you forgot to destroy them.
```c
void SituationShutdown(void);
```
**Usage Example:**
```c
int main(int argc, char* argv[]) {
    // It's good practice to pair Init and Shutdown in the same scope.
    if (SituationInit(argc, argv, NULL) != SIT_SUCCESS) {
        return -1;
    }

    while (!SituationWindowShouldClose()) {
        // Main application loop
    }

    SituationShutdown(); // The very last call.
    return 0;
}
```

---
#### `SituationIsInitialized`
Checks if the library has been successfully initialized. Returns `true` if `SituationInit()` has been called and completed without errors, `false` otherwise.
```c
bool SituationIsInitialized(void);
```
**Usage Example:**
```c
// You might use this in a helper function to ensure it's safe to call library functions.
void UpdatePlayer() {
    if (!SituationIsInitialized()) {
        printf("Error: Cannot update player before the library is initialized.\n");
        return;
    }
    // ... proceed with player update logic ...
}
```

---
#### `SituationWindowShouldClose`
Returns `true` if the user has attempted to close the window (e.g., by clicking the 'X' button, pressing Alt+F4, or sending a quit signal). This is the canonical way to control your main application loop.
```c
bool SituationWindowShouldClose(void);
```
**Usage Example:**
```c
// The main loop should continue as long as this function returns false.
while (!SituationWindowShouldClose()) {
    // Poll events, update logic, render frame
}
// Loop terminates, and the application proceeds to shutdown.
```

---
#### `SituationSetTargetFPS`
Sets a target frame rate for the application. The main loop will sleep to avoid exceeding this rate, reducing CPU usage.
```c
void SituationSetTargetFPS(int fps);
```
**Usage Example:**
```c
// Cap the application at 60 FPS.
SituationSetTargetFPS(60);
// To uncap the frame rate, use 0.
// SituationSetTargetFPS(0);
```

---
#### `SituationGetFrameTime`
Gets the time in seconds that the previous frame took to complete, commonly known as `deltaTime`. This value is calculated by `SituationUpdateTimers()` and is essential for creating frame-rate-independent logic for movement, physics, and animations.
```c
float SituationGetFrameTime(void);
```
**Usage Example:**
```c
// Inside the main loop, after SituationUpdateTimers()
float dt = SituationGetFrameTime();

// Update player position based on time, not frames, ensuring smooth movement
// regardless of the frame rate.
player_position.x += player_speed * dt;
```

---
#### `SituationGetFPS`
Gets the current frames-per-second, calculated periodically by the library. This is useful for displaying performance metrics to the user.
```c
int SituationGetFPS(void);
```
**Usage Example:**
```c
// Inside the main loop, update the window title with the current FPS.
int current_fps = SituationGetFPS();
char window_title[128];
sprintf(window_title, "My App | FPS: %d", current_fps);
SituationSetWindowTitle(window_title);
```

---
#### `SituationGetLastErrorMsg`
Retrieves a copy of the last error message generated by the library. This is useful for debugging initialization failures or other runtime errors. The caller is responsible for freeing the returned string with `SituationFreeString()`.
```c
SituationError SituationGetLastErrorMsg(char** out_msg);
```
**Usage Example:**
```c
if (SituationInit(argc, argv, &init_info) != SITUATION_SUCCESS) {
    char* error_msg = NULL;
    if (SituationGetLastErrorMsg(&error_msg) == SITUATION_SUCCESS) {
        fprintf(stderr, "Initialization failed: %s\n", error_msg);
        SituationFreeString(error_msg); // IMPORTANT: Free the memory
    }
    return -1;
}
```

---
#### `SituationSetExitCallback`
Registers a callback function to be executed just before the library shuts down. This is useful for performing final cleanup tasks, such as saving application state.
```c
typedef void (*SituationExitCallback)(void* user_data);
void SituationSetExitCallback(SituationExitCallback callback, void* user_data);
```
**Usage Example:**
```c
void on_exit_cleanup(void* user_data) {
    const char* message = (const char*)user_data;
    printf("Shutting down. Custom data: %s\n", message);
    // You could save game settings or user progress here.
}

// In main, after Init
const char* my_data = "Saving settings...";
SituationSetExitCallback(on_exit_cleanup, (void*)my_data);
```

---
#### `SituationIsArgumentPresent`
Checks if a specific command-line argument flag (e.g., `"-server"`) was provided when the application was launched. The library automatically parses `argv` during `SituationInit`.
```c
bool SituationIsArgumentPresent(const char* arg_name);
```
**Usage Example:**
```c
// Run as: ./my_app -fullscreen -debug
int main(int argc, char* argv[]) {
    SituationInit(argc, argv, NULL);

    // Check for a simple flag to enable fullscreen mode at startup.
    if (SituationIsArgumentPresent("-fullscreen")) {
        SituationToggleFullscreen();
    }
    // Check for a debug flag.
    if (SituationIsArgumentPresent("-debug")) {
        g_enable_debug_mode = true;
    }
}
```

---
#### `SituationGetArgumentValue`
Gets the value associated with a command-line argument. It supports both colon-separated (`-level:jungle`) and space-separated (`-level jungle`) formats. Returns `NULL` if the argument is not found.
```c
const char* SituationGetArgumentValue(const char* arg_name);
```
**Usage Example:**
```c
// Run as: ./my_app -level:forest -player "Jules"
int main(int argc, char* argv[]) {
    SituationInit(argc, argv, NULL);

    // Get the level name to load.
    const char* level_name = SituationGetArgumentValue("-level");
    if (level_name) {
        printf("Loading level: %s\n", level_name);
    } else {
        printf("Loading default level.\n");
    }

    // Get the player's name.
    const char* player_name = SituationGetArgumentValue("-player");
    if (player_name) {
        printf("Welcome, %s!\n", player_name);
    }
}
```

---
#### `SituationGetVersionString`
Gets the version of the Situation library as a string.
```c
SITAPI const char* SituationGetVersionString(void);
```
**Usage Example:**
```c
const char* version = SituationGetVersionString();
printf("Situation library version: %s\n", version);
```

---
#### `SituationGetGPUName`
Gets the human-readable name of the active GPU.
```c
SITAPI const char* SituationGetGPUName(void);
```
**Usage Example:**
```c
const char* gpu_name = SituationGetGPUName();
printf("GPU: %s\n", gpu_name);
```

---
#### `SituationGetVRAMUsage`
Gets the estimated total Video RAM (VRAM) usage in bytes. This is a best-effort query and may not be perfectly accurate on all platforms.
```c
SITAPI uint64_t SituationGetVRAMUsage(void);
```
**Usage Example:**
```c
uint64_t vram_usage = SituationGetVRAMUsage();
printf("VRAM Usage: %.2f MB\n", (double)vram_usage / (1024.0 * 1024.0));
```

---
#### `SituationGetDrawCallCount`
Gets the number of draw calls submitted in the last completed frame. This is a key performance metric for identifying rendering bottlenecks.
```c
SITAPI uint32_t SituationGetDrawCallCount(void);
```
**Usage Example:**
```c
// In the update loop, display the draw call count in the window title.
char title[256];
sprintf(title, "My App | FPS: %d | Draw Calls: %u",
        SituationGetFPS(), SituationGetDrawCallCount());
SituationSetWindowTitle(title);
```

---
#### `SituationExecuteCommand`
Executes a system shell command in a hidden process and captures the output (stdout/stderr).
```c
SITAPI int SituationExecuteCommand(const char *cmd, char **output);
```
**Usage Example:**
```c
char* output = NULL;
int exit_code = SituationExecuteCommand("ls -la", &output);
if (exit_code == 0 && output) {
    printf("Command output:\n%s\n", output);
    SituationFreeString(output);
}
```

---
#### `SituationGetCPUThreadCount`
Gets the number of logical CPU cores.
```c
SITAPI uint32_t SituationGetCPUThreadCount(void);
```

---
#### `SituationGetMaxComputeWorkGroups`
Queries the maximum supported compute shader work group count (X, Y, Z).
```c
SITAPI void SituationGetMaxComputeWorkGroups(uint32_t* x, uint32_t* y, uint32_t* z);
```

---
#### `SituationLogWarning`
Logs a warning message in debug builds.
```c
SITAPI void SituationLogWarning(SituationError code, const char* fmt, ...);
```
**Usage Example:**
```c
if (score > 9000) {
    SituationLogWarning(SITUATION_ERROR_GENERAL, "Score is over 9000!");
}
```

---
#### `SituationGetDeviceInfo`
Gathers and returns a comprehensive snapshot of the host system's hardware and operating system. This is useful for logging, debugging, or adjusting application settings based on the user's hardware.
```c
SituationDeviceInfo SituationGetDeviceInfo(void);
```
**Usage Example:**
```c
// Log device information at startup.
SituationInit(argc, argv, NULL);
SituationDeviceInfo device = SituationGetDeviceInfo();
printf("--- System Information ---\n");
printf("OS: %s %s\n", device.os_name, device.os_version);
printf("CPU: %s (%d cores, %d threads)\n", device.cpu_brand, device.cpu_core_count, device.cpu_thread_count);
printf("RAM: %.2f GB\n", (double)device.system_ram_bytes / (1024.0*1024.0*1024.0));
printf("GPU: %s\n", device.gpu_brand);
printf("VRAM: %.2f GB\n", (double)device.gpu_vram_bytes / (1024.0*1024.0*1024.0));
printf("--------------------------\n");
```

---
#### `SituationGetTime`
Gets the total elapsed time in seconds since `SituationInit()` was called. This is a high-precision monotonic timer.
```c
double SituationGetTime(void);
```
**Usage Example:**
```c
// Use the total elapsed time to drive a continuous animation, like a rotation.
double current_time = SituationGetTime();
mat4 rotation_matrix;
glm_rotate_y(model_matrix, (float)current_time * 0.5f, rotation_matrix); // Rotate over time
```

---
#### `SituationGetFrameCount`
Gets the total number of frames that have been rendered since the application started.
```c
uint64_t SituationGetFrameCount(void);
```
**Usage Example:**
```c
// Use the frame count for simple, periodic logic that doesn't need to be tied to real time.
if (SituationGetFrameCount() % 120 == 0) {
    printf("120 frames have passed.\n");
}
```

---
#### `SituationWaitTime`
Pauses the application thread for a specified duration in seconds. This is a simple wrapper over the system's sleep function and can be useful for debugging or simple timing.
```c
void SituationWaitTime(double seconds);
```
**Usage Example:**
```c
printf("Preparing to load assets...\n");
// Wait for 500 milliseconds before proceeding to give the user time to read the message.
SituationWaitTime(0.5);
printf("Loading...\n");
```

---
#### `SituationEnableEventWaiting`
Enables event waiting. When enabled, `SituationPollInputEvents()` will wait for new events instead of immediately returning, putting the application to sleep and saving CPU cycles when idle. This is ideal for tools and non-game applications.
```c
void SituationEnableEventWaiting(void);
```
**Usage Example:**
```c
// In an editor or tool, enable event waiting to reduce resource usage.
SituationEnableEventWaiting();
while (!SituationWindowShouldClose()) {
    SituationPollInputEvents(); // This will now block until an event occurs.
    // ... update UI or process data only when there are new events ...
    // ... render ...
}
```

---
#### `SituationDisableEventWaiting`
Disables event waiting, restoring the default behavior where `SituationPollInputEvents()` returns immediately. This is necessary for real-time applications like games that need to run the update loop continuously.
```c
void SituationDisableEventWaiting(void);
```
**Usage Example:**
```c
// When switching from an editor mode to a real-time game simulation.
SituationDisableEventWaiting();
```

---
#### `SituationOpenFile`
Asks the operating system to open a file or folder with its default application.
```c
void SituationOpenFile(const char* filePath);
```
**Usage Example:**
```c
// This will open the specified file in its default application (e.g., Notepad).
SituationOpenFile("C:/path/to/your/log.txt");

// This will open the specified directory in the file explorer.
SituationOpenFile("C:/Users/Default/Documents");
```

---
#### `SituationOpenURL`
Asks the operating system to open a URL in the default web browser.
```c
void SituationOpenURL(const char* url);
```
**Usage Example:**
```c
// This will open the user's web browser to the specified URL.
SituationOpenURL("https://www.github.com");
```

---
#### `SituationSetErrorCallback`
Sets a callback for handling library errors.
```c
void SituationSetErrorCallback(SituationErrorCallback callback);
```
**Usage Example:**
```c
void my_error_logger(int error_code, const char* message) {
    fprintf(stderr, "Situation Error [%d]: %s\n", error_code, message);
}

// In main, after Init
SituationSetErrorCallback(my_error_logger);
```

---
#### `SituationSetVSync`
Enables or disables VSync.
```c
void SituationSetVSync(bool enabled);
```
**Usage Example:**
```c
// Disable VSync for performance testing
SituationSetVSync(false);
```

---
#### `SituationGetPlatform`
Gets the current platform.
```c
int SituationGetPlatform(void);
```
**Usage Example:**
```c
int platform = SituationGetPlatform();
#if defined(PLATFORM_DESKTOP)
    if (platform == PLATFORM_DESKTOP) printf("Running on a desktop platform.\n");
#endif
```

---
#### `SituationUpdate`
DEPRECATED: Use `SituationPollInputEvents()` and `SituationUpdateTimers()`.
```c
void SituationUpdate(void);
```

---
#### `SituationSetResizeCallback`
Sets a callback function for window framebuffer resize events.
```c
void SituationSetResizeCallback(void (*callback)(int width, int height, void* user_data), void* user_data);
```

---

#### `SituationSetFocusCallback`

Sets a callback function to be called when the window gains or loses focus.

```c
SITAPI void SituationSetFocusCallback(SituationFocusCallback callback, void* user_data);
```

**Usage Example:**
```c
void OnFocusChanged(bool focused, void* user_data) {
    printf("Window focus changed: %d\n", focused);
}

SituationSetFocusCallback(OnFocusChanged, NULL);
```

---
#### `SituationSetFocusCallback`
Sets a callback for window focus events.
```c
void SituationSetFocusCallback(SituationFocusCallback callback, void* user_data);
```

---
#### `SituationSetFileDropCallback`
Sets a callback for file drop events.
```c
void SituationSetFileDropCallback(SituationFileDropCallback callback, void* user_data);
```

---
#### `SituationGetUserDirectory`
Gets the full path to the current user's home directory. The caller must free the returned string.
```c
char* SituationGetUserDirectory(void);
```

---
#### `SituationGetCurrentDriveLetter`
Gets the drive letter of the running executable (Windows only).
```c
char SituationGetCurrentDriveLetter(void);
```

---
#### `SituationGetDriveInfo`
Gets information for a specific drive (Windows only).
```c
bool SituationGetDriveInfo(char drive_letter, uint64_t* out_total_capacity_bytes, uint64_t* out_free_space_bytes, char* out_volume_name, int volume_name_len);
```

---
#### `SituationGetRendererType`
Gets the current active renderer type (OpenGL or Vulkan).
```c
SituationRendererType SituationGetRendererType(void);
```

---
#### `SituationGetGLFWwindow`
Gets the raw GLFW window handle.
```c
GLFWwindow* SituationGetGLFWwindow(void);
```

---
#### `SituationGetVulkanInstance`
Gets the raw Vulkan instance handle.
```c
VkInstance SituationGetVulkanInstance(void);
```

---
#### `SituationGetVulkanDevice`
Gets the raw Vulkan logical device handle.
```c
VkDevice SituationGetVulkanDevice(void);
```

---
#### `SituationGetVulkanPhysicalDevice`
Gets the raw Vulkan physical device handle.
```c
VkPhysicalDevice SituationGetVulkanPhysicalDevice(void);
```

---
#### `SituationGetMainWindowRenderPass`
Gets the render pass for the main window.
```c
VkRenderPass SituationGetMainWindowRenderPass(void);
```

---
#### `SituationPauseApp`
Pauses the application's main loop. This is useful for when the application loses focus, allowing you to halt updates and rendering to conserve system resources.
```c
void SituationPauseApp(void);
```
**Usage Example:**
```c
// A window focus callback is a good place to use this.
void on_focus_change(bool focused, void* user_data) {
    if (!focused) {
        SituationPauseApp();
    } else {
        SituationResumeApp();
    }
}
```

---
#### `SituationResumeApp`
Resumes the application's main loop after it has been paused.
```c
void SituationResumeApp(void);
```
**Usage Example:**
```c
// See the example for SituationPauseApp. When the window regains focus,
// this function is called to resume normal operation.
```

---
#### `SituationIsAppPaused`
Checks if the application is currently paused.
```c
bool SituationIsAppPaused(void);
```

</details>
<details>
<summary><h3>Window and Display Module</h3></summary>

**Overview:** This module provides an exhaustive suite of tools for managing the application's window and querying the properties of physical display devices. It handles everything from basic window state changes (fullscreen, minimized, borderless) to more advanced features like DPI scaling, opacity, and clipboard access.

### Structs and Flags

#### `SituationDeviceInfo`
This struct, returned by `SituationGetDeviceInfo()`, provides a snapshot of the host system's hardware.
```c
typedef struct SituationDeviceInfo {
    char cpu_brand[49];
    int cpu_core_count;
    int cpu_thread_count;
    uint64_t system_ram_bytes;
    char gpu_brand[128];
    uint64_t gpu_vram_bytes;
    int display_count;
    char os_name[32];
    char os_version[32];
    uint64_t total_storage_bytes;
    uint64_t free_storage_bytes;
} SituationDeviceInfo;
```
-   `cpu_brand`: The brand and model of the CPU.
-   `cpu_core_count`, `cpu_thread_count`: The number of physical cores and logical threads.
-   `system_ram_bytes`: Total system RAM in bytes.
-   `gpu_brand`: The brand and model of the GPU.
-   `gpu_vram_bytes`: Total dedicated video RAM in bytes.
-   `display_count`: The number of connected displays.
-   `os_name`, `os_version`: The name and version of the operating system.
-   `total_storage_bytes`, `free_storage_bytes`: The total and free space on the primary storage device.

---
#### `SituationDisplayInfo`
Returned by `SituationGetDisplays()`, this struct contains detailed information about a connected monitor.
```c
typedef struct SituationDisplayInfo {
    int id;
    char name[128];
    int current_mode;
    int mode_count;
    SituationDisplayMode* modes;
    vec2 position;
    vec2 physical_size;
} SituationDisplayInfo;
```
-   `id`: The internal ID of the monitor.
-   `name`: The human-readable name of the monitor.
-   `current_mode`: The index of the display's current mode in the `modes` array.
-   `mode_count`: The number of available display modes.
-   `modes`: A pointer to an array of `SituationDisplayMode` structs.
-   `position`: The physical position of the monitor's top-left corner on the virtual desktop.
-   `physical_size`: The physical size of the display in millimeters.

---
#### `SituationDisplayMode`
Represents a single supported display mode (resolution, refresh rate, etc.) for a monitor.
```c
typedef struct SituationDisplayMode {
    int width;
    int height;
    int refresh_rate;
    int red_bits;
    int green_bits;
    int blue_bits;
} SituationDisplayMode;
```
-   `width`, `height`: The resolution of the display mode in pixels.
-   `refresh_rate`: The refresh rate in Hertz (Hz).
-   `red_bits`, `green_bits`, `blue_bits`: The color depth for each channel.

---
#### `SituationWindowStateFlags`
These flags are used with `SituationSetWindowState()` and `SituationClearWindowState()` to control the window's appearance and behavior. They can be combined using the bitwise `|` operator.
| Flag | Description |
|---|---|
| `SITUATION_FLAG_VSYNC_HINT` | Suggests that the graphics backend should wait for vertical sync, reducing screen tearing. |
| `SITUATION_FLAG_FULLSCREEN_MODE` | Enables exclusive fullscreen mode. |
| `SITUATION_FLAG_WINDOW_RESIZABLE` | Allows the user to resize the window. |
| `SITUATION_FLAG_WINDOW_UNDECORATED` | Removes the window's border, title bar, and other decorations. |
| `SITUATION_FLAG_WINDOW_HIDDEN` | Hides the window from view. |
| `SITUATION_FLAG_WINDOW_MINIMIZED` | Minimizes the window to the taskbar. |
| `SITUATION_FLAG_WINDOW_MAXIMIZED` | Maximizes the window to fill the work area. |
| `SITUATION_FLAG_WINDOW_UNFOCUSED` | Prevents the window from gaining focus when created. |
| `SITUATION_FLAG_WINDOW_TOPMOST` | Keeps the window on top of all other windows. |
| `SITUATION_FLAG_WINDOW_ALWAYS_RUN` | Allows the application to continue running even when the window is minimized. |
| `SITUATION_FLAG_WINDOW_TRANSPARENT` | Enables a transparent framebuffer for non-rectangular window shapes. |
| `SITUATION_FLAG_HIGHDPI_HINT` | Requests a high-DPI framebuffer on platforms that support it (e.g., macOS Retina). |
| `SITUATION_FLAG_MSAA_4X_HINT` | Suggests that the graphics backend should use 4x multisample anti-aliasing. |

#### Functions
### Functions

#### `SituationSetWindowState`
Sets one or more window state flags, such as `SITUATION_FLAG_WINDOW_RESIZABLE` or `SITUATION_FLAG_WINDOW_TOPMOST`. You can combine multiple flags using the bitwise OR `|` operator.
```c
void SituationSetWindowState(uint32_t flags);
```
**Usage Example:**
```c
// Make the window resizable and always on top.
SituationSetWindowState(SITUATION_FLAG_WINDOW_RESIZABLE | SITUATION_FLAG_WINDOW_TOPMOST);
```

---
#### `SituationClearWindowState`
Clears one or more window state flags, reverting them to their default behavior.
```c
void SituationClearWindowState(uint32_t flags);
```
**Usage Example:**
```c
// The window was previously set to be always on top.
// This will remove that state, allowing it to behave like a normal window again.
SituationClearWindowState(SITUATION_FLAG_WINDOW_TOPMOST);
```

---
#### `SituationToggleFullscreen`
Toggles the window between exclusive fullscreen and windowed mode. This is a convenient wrapper around `SituationSetWindowState` and `SituationClearWindowState` for the `SITUATION_FLAG_FULLSCREEN_MODE` flag.
```c
void SituationToggleFullscreen(void);
```
**Usage Example:**
```c
// In the update loop, toggle fullscreen when F11 is pressed.
if (SituationIsKeyPressed(SIT_KEY_F11)) {
    SituationToggleFullscreen();
}
```

---
#### `SituationSetWindowTitle`
Sets the text that appears in the window's title bar.
```c
void SituationSetWindowTitle(const char *title);
```
**Usage Example:**
```c
// You can dynamically update the title to show game state or performance info.
char title[128];
sprintf(title, "My Game | Score: %d | FPS: %d", g_player_score, SituationGetFPS());
SituationSetWindowTitle(title);
```

---
#### `SituationSetWindowPosition`
Sets the screen-space position of the top-left corner of the window's client area.
```c
void SituationSetWindowPosition(int x, int y);
```
**Usage Example:**
```c
// Center the window on the primary monitor.
int monitor_width = SituationGetMonitorWidth(0);
int monitor_height = SituationGetMonitorHeight(0);
int window_width = SituationGetScreenWidth();
int window_height = SituationGetScreenHeight();
int x_pos = (monitor_width / 2) - (window_width / 2);
int y_pos = (monitor_height / 2) - (window_height / 2);
SituationSetWindowPosition(x_pos, y_pos);
```

---
#### `SituationGetWindowPosition`
Gets the screen-space position of the top-left corner of the window's client area.
```c
vec2 SituationGetWindowPosition(void);
```
**Usage Example:**
```c
// Get the current window position.
vec2 current_pos;
glm_vec2_copy(SituationGetWindowPosition(), current_pos);
printf("Window is at: (%.0f, %.0f)\n", current_pos[0], current_pos[1]);
```

---
#### `SituationSetWindowSize` / `SituationGetWindowSize`
Sets or gets the dimensions of the window's client area in screen coordinates.
```c
void SituationSetWindowSize(int width, int height);
vec2 SituationGetWindowSize(void);
```
**Usage Example:**
```c
// Set the window to a fixed 800x600 size
SituationSetWindowSize(800, 600);

// Verify the size
vec2 size;
glm_vec2_copy(SituationGetWindowSize(), size);
printf("Window size is now: %.0fx%.0f\n", size[0], size[1]);
```

---
#### `SituationSetWindowSizeLimits`
Sets the minimum and maximum size for a resizable window.
```c
void SituationSetWindowSizeLimits(int min_width, int min_height, int max_width, int max_height);
```
**Usage Example:**
```c
// Ensure the window is never smaller than 640x480 or larger than the primary monitor's resolution.
int max_w = SituationGetMonitorWidth(0);
int max_h = SituationGetMonitorHeight(0);
SituationSetWindowSizeLimits(640, 480, max_w, max_h);
```

---
#### `SituationSetWindowOpacity`
Sets the opacity of the entire window, where `1.0` is fully opaque and `0.0` is fully transparent.
```c
void SituationSetWindowOpacity(float opacity);
```
**Usage Example:**
```c
// Make the window semi-transparent.
SituationSetWindowOpacity(0.5f);
```

---
#### `SituationSetWindowIcon`
Sets a custom icon for the window from a `SituationImage`. The library handles the conversion to the platform's native icon format. For best results, provide a square image (e.g., 256x256).
```c
void SituationSetWindowIcon(SituationImage image);
```
**Usage Example:**
```c
// During initialization, load an icon from a file.
SituationImage icon;
if (SituationLoadImage("assets/icon.png", &icon) == SITUATION_SUCCESS) {
    SituationSetWindowIcon(icon);
    // The icon data is copied by the system, so we can unload the CPU image.
    SituationUnloadImage(icon);
}
```

---
#### `SituationRequestWindowAttention`
Requests the user's attention by making the window's icon flash or bounce in the taskbar. This is a non-intrusive way to signal that a long-running task has completed or that user input is required.
```c
void SituationRequestWindowAttention(void);
```
**Usage Example:**
```c
// When a file export or level loading process is complete:
printf("Export complete!\n");
SituationRequestWindowAttention();
```

---
#### `SituationIsWindowState`
Checks if a specific window state (e.g., `SITUATION_FLAG_WINDOW_MAXIMIZED`) is currently active.
```c
bool SituationIsWindowState(uint32_t flag);
```
**Usage Example:**
```c
if (SituationIsWindowState(SITUATION_FLAG_FULLSCREEN_MODE)) {
    printf("Application is in fullscreen mode.\n");
}
if (SituationIsWindowState(SITUATION_FLAG_WINDOW_MINIMIZED)) {
    printf("Application is minimized.\n");
}
```

---
#### `SituationGetScreenWidth`
Gets the current width of the window in screen coordinates (logical size). This may differ from the render width on high-DPI displays. Use this for UI layout and mouse coordinate calculations.
```c
int SituationGetScreenWidth(void);
```
**Usage Example:**
```c
// Get the logical width for UI layout calculations.
int screen_width = SituationGetScreenWidth();
// Position a UI element 10 pixels from the right edge of the window.
float element_x = screen_width - element_width - 10;
```

---
#### `SituationGetScreenHeight`
Gets the current height of the window in screen coordinates (logical size).
```c
int SituationGetScreenHeight(void);
```
**Usage Example:**
```c
// Center an element vertically based on the logical screen height.
int screen_height = SituationGetScreenHeight();
float element_y = (screen_height / 2.0f) - (element_height / 2.0f);
```

---
#### `SituationGetRenderWidth`
Gets the current width of the rendering framebuffer in pixels. This is the physical size, which may be larger than the logical window size on high-DPI displays. Use this for graphics calculations like setting the viewport.
```c
int SituationGetRenderWidth(void);
```
**Usage Example:**
```c
// Use the render width to set the graphics viewport.
int framebuffer_width = SituationGetRenderWidth();
int framebuffer_height = SituationGetRenderHeight();
SituationCmdSetViewport(SituationGetMainCommandBuffer(), 0, 0, framebuffer_width, framebuffer_height);
```

---
#### `SituationGetRenderHeight`
Gets the current height of the rendering framebuffer in pixels.
```c
int SituationGetRenderHeight(void);
```
**Usage Example:**
```c
// This value is often needed for aspect ratio calculations in projection matrices.
float aspect_ratio = (float)SituationGetRenderWidth() / (float)SituationGetRenderHeight();
```

---
#### `SituationGetWindowScaleDPI`
Gets the ratio of physical pixels to logical screen coordinates.
```c
vec2 SituationGetWindowScaleDPI(void);
```
**Usage Example:**
```c
// On a Retina display, this might return (2.0, 2.0). On standard displays, (1.0, 1.0).
vec2 dpi_scale;
glm_vec2_copy(SituationGetWindowScaleDPI(), dpi_scale);
printf("DPI Scale: %.1fx, %.1fy\n", dpi_scale[0], dpi_scale[1]);
```

---
#### `SituationGetCurrentMonitor`
Gets the index of the monitor that the window is currently on.
```c
int SituationGetCurrentMonitor(void);
```
**Usage Example:**
```c
int current_monitor_id = SituationGetCurrentMonitor();
printf("Window is on monitor %d.\n", current_monitor_id);
```

---
#### `SituationGetMonitorName`
Gets the human-readable name of a monitor by its index.
```c
const char* SituationGetMonitorName(int monitor);
```
**Usage Example:**
```c
int monitor_count;
SituationGetDisplays(&monitor_count);
for (int i = 0; i < monitor_count; i++) {
    printf("Monitor %d is called: %s\n", i, SituationGetMonitorName(i));
}
```

---
#### `SituationGetMonitorWidth`
Gets the current width of a monitor's resolution in pixels.
```c
int SituationGetMonitorWidth(int monitor);
```
**Usage Example:**
```c
// Get the width of the primary monitor (index 0).
int primary_monitor_width = SituationGetMonitorWidth(0);
printf("Primary monitor width: %dpx\n", primary_monitor_width);
```

---
#### `SituationGetMonitorHeight`
Gets the current height of a monitor's resolution in pixels.
```c
int SituationGetMonitorHeight(int monitor);
```
**Usage Example:**
```c
// Get the height of the primary monitor (index 0).
int primary_monitor_height = SituationGetMonitorHeight(0);
printf("Primary monitor height: %dpx\n", primary_monitor_height);
```

---
#### `SituationGetDisplays`
Retrieves detailed information for all connected displays. The caller is responsible for freeing the returned array using `SituationFreeDisplays`.
```c
SituationError SituationGetDisplays(SituationDisplayInfo** out_displays, int* out_count);
```
**Usage Example:**
```c
int display_count = 0;
SituationDisplayInfo* displays = NULL;
if (SituationGetDisplays(&displays, &display_count) == SITUATION_SUCCESS) {
    for (int i = 0; i < display_count; i++) {
        printf("Display %d: %s\n", i, displays[i].name);
    }
    SituationFreeDisplays(displays, display_count); // Don't forget to free!
}
```

---
#### `SituationFreeDisplays`
Frees the memory allocated for the display list returned by `SituationGetDisplays`.
```c
void SituationFreeDisplays(SituationDisplayInfo* displays, int count);
```
**Usage Example:**
```c
int display_count;
SituationDisplayInfo* displays = SituationGetDisplays(&display_count);
// ... use the display info ...
SituationFreeDisplays(displays, display_count); // Free the memory.
```

---
#### `SituationShowCursor`

Shows the cursor.

```c
SITAPI void SituationShowCursor(void);
```

**Usage Example:**
```c
SituationShowCursor();
```

---

#### `SituationHideCursor`

Hides the cursor.

```c
SITAPI void SituationHideCursor(void);
```

**Usage Example:**
```c
SituationHideCursor();
```

---

#### `SituationDisableCursor`

Disables the cursor, hiding it and providing unlimited mouse movement.

```c
SITAPI void SituationDisableCursor(void);
```

**Usage Example:**
```c
SituationDisableCursor();
```

---
#### `SituationShowCursor`
Makes the mouse cursor visible and restores its normal behavior.
```c
void SituationShowCursor(void);
```
**Usage Example:**
```c
// When returning to the main menu from gameplay, show the cursor.
SituationShowCursor();
```

---
#### `SituationHideCursor`
Makes the mouse cursor invisible when it is over the application's window, but it does not lock the cursor.
```c
void SituationHideCursor(void);
```
**Usage Example:**
```c
// In a media player application, hide the cursor when it's over the video playback area.
SituationHideCursor();
```

---
#### `SituationDisableCursor`
Hides the mouse cursor and locks it to the window, providing unbounded movement. This is ideal for 3D camera controls.
```c
void SituationDisableCursor(void);
```
**Usage Example:**
```c
// When entering a first-person 3D scene, disable the cursor for camera look.
SituationDisableCursor();
// Mouse delta can now be used to rotate the camera without the cursor leaving the window.
```

---
#### `SituationGetClipboardText`
Gets UTF-8 encoded text from the system clipboard. The returned string is heap-allocated and must be freed by the caller using `SituationFreeString`.
```c
SituationError SituationGetClipboardText(const char** out_text);
```
**Usage Example:**
```c
// In an input handler for Ctrl+V
if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) && SituationIsKeyPressed(SIT_KEY_V)) {
    const char* clipboard_text = NULL;
    if (SituationGetClipboardText(&clipboard_text) == SITUATION_SUCCESS) {
        // Paste text into an input field.
        SituationFreeString((char*)clipboard_text);
    }
}
```

---
#### `SituationSetClipboardText`
Sets the system clipboard to the provided UTF-8 encoded text.
```c
SituationError SituationSetClipboardText(const char* text);
```
**Usage Example:**
```c
// In an input handler for Ctrl+C
if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) && SituationIsKeyPressed(SIT_KEY_C)) {
    // Copy selected text to the clipboard.
    SituationSetClipboardText(selected_text);
}
```

---
#### `SituationIsFileDropped`
Checks if files were dropped onto the window this frame.
```c
bool SituationIsFileDropped(void);
```

---
#### `SituationLoadDroppedFiles`
Get the paths of files dragged and dropped onto the window. The caller must free the returned list with `SituationUnloadDroppedFiles`.
```c
char** SituationLoadDroppedFiles(int* count);
```
**Usage Example:**
```c
// In the update loop
if (SituationIsFileDropped()) {
    int file_count;
    char** files = SituationLoadDroppedFiles(&file_count);
    for (int i = 0; i < file_count; i++) {
        printf("Dropped file: %s\n", files[i]);
    }
    SituationUnloadDroppedFiles(files, file_count); // Free the list
}
```

---
#### `SituationToggleBorderlessWindowed`
Toggles the window between borderless and decorated mode.
```c
void SituationToggleBorderlessWindowed(void);
```

---
#### `SituationMaximizeWindow`
Maximizes the window if it's resizable.
```c
void SituationMaximizeWindow(void);
```

---
#### `SituationMinimizeWindow`
Minimizes the window (iconify).
```c
void SituationMinimizeWindow(void);
```

---
#### `SituationRestoreWindow`
Restores a minimized or maximized window.
```c
void SituationRestoreWindow(void);
```

---
#### `SituationSetWindowFocused`
Sets the window to be focused.
```c
void SituationSetWindowFocused(void);
```

---
#### `SituationSetWindowIcons`
Sets the icon for the window (multiple sizes).
```c
void SituationSetWindowIcons(SituationImage *images, int count);
```

---
#### `SituationSetWindowMinSize`
Sets the window minimum dimensions.
```c
void SituationSetWindowMinSize(int width, int height);
```

---
#### `SituationSetWindowMaxSize`
Sets the window maximum dimensions.
```c
void SituationSetWindowMaxSize(int width, int height);
```

---
#### `SituationIsWindowFullscreen`
Checks if the window is currently in fullscreen mode.
```c
bool SituationIsWindowFullscreen(void);
```

---
#### `SituationIsWindowHidden`
Checks if the window is currently hidden.
```c
bool SituationIsWindowHidden(void);
```

---
#### `SituationIsWindowMinimized`
Checks if the window is currently minimized.
```c
bool SituationIsWindowMinimized(void);
```

---
#### `SituationIsWindowMaximized`
Checks if the window is currently maximized.
```c
bool SituationIsWindowMaximized(void);
```

---
#### `SituationHasWindowFocus`
Checks if the window is currently focused.
```c
bool SituationHasWindowFocus(void);
```

---
#### `SituationIsWindowResized`
Checks if the window was resized in the last frame.
```c
bool SituationIsWindowResized(void);
```

---
#### `SituationGetWindowSize`
Gets the current logical window size.
```c
void SituationGetWindowSize(int* width, int* height);
```

---
#### `SituationGetMonitorCount`
Gets the number of connected monitors.
```c
int SituationGetMonitorCount(void);
```

---
#### `SituationRefreshDisplays`
Force a refresh of the cached display information.
```c
void SituationRefreshDisplays(void);
```

---
#### `SituationSetDisplayMode`
Sets the display mode for a monitor.
```c
SituationError SituationSetDisplayMode(int monitor_id, const SituationDisplayMode* mode, bool fullscreen);
```

---
#### `SituationGetMonitorPosition`
Gets the top-left position of a monitor on the desktop.
```c
vec2 SituationGetMonitorPosition(int monitor_id);
```

---
#### `SituationGetMonitorPhysicalWidth`
Gets the physical width of a monitor in millimeters.
```c
int SituationGetMonitorPhysicalWidth(int monitor_id);
```

---
#### `SituationGetMonitorPhysicalHeight`
Gets the physical height of a monitor in millimeters.
```c
int SituationGetMonitorPhysicalHeight(int monitor_id);
```

---
#### `SituationGetMonitorRefreshRate`
Gets the refresh rate of a monitor.
```c
int SituationGetMonitorRefreshRate(int monitor_id);
```

---
#### `SituationSetCursor`
Sets the mouse cursor to a standard shape.
```c
void SituationSetCursor(SituationCursor cursor);
```

---
#### `SituationSetWindowStateProfiles`
Sets the flag profiles for when the window is focused vs. unfocused.
```c
SituationError SituationSetWindowStateProfiles(uint32_t active_flags, uint32_t inactive_flags);
```

---
#### `SituationApplyCurrentProfileWindowState`
Manually apply the appropriate window state profile based on current focus.
```c
SituationError SituationApplyCurrentProfileWindowState(void);
```

---
#### `SituationToggleWindowStateFlags`
Toggles flags in the current profile and apply the result.
```c
SituationError SituationToggleWindowStateFlags(SituationWindowStateFlags flags_to_toggle);
```

---
#### `SituationGetCurrentActualWindowStateFlags`
Gets flags based on current GLFW window state.
```c
uint32_t SituationGetCurrentActualWindowStateFlags(void);
```

---

#### `SituationGetGLFWwindow`

Gets the underlying GLFW window handle.

```c
SITAPI GLFWwindow* SituationGetGLFWwindow(void);
```

**Usage Example:**
```c
GLFWwindow* glfw_window = SituationGetGLFWwindow();
// Use with GLFW functions
```

</details>
<details>
<summary><h3>Image Module</h3></summary>

**Overview:** The Image module is a comprehensive, CPU-side toolkit for all forms of image and font manipulation. It allows you to load images, generate new images programmatically, perform transformations, and render text. The `SituationImage` objects produced by this module are the primary source for creating GPU-side `SituationTexture`s.

### Structs and Enums

#### `Vector2`
A simple 2D vector, used for positions, sizes, and other 2D coordinates.
```c
typedef struct Vector2 {
    float x;
    float y;
} Vector2;
```
-   `x`: The x-component of the vector.
-   `y`: The y-component of the vector.

---
#### `Rectangle`
Represents a rectangle with a position (x, y) and dimensions (width, height).
```c
typedef struct Rectangle {
    float x;
    float y;
    float width;
    float height;
} Rectangle;
```
-   `x`, `y`: The screen coordinates of the top-left corner.
-   `width`, `height`: The dimensions of the rectangle.

---
#### `SituationImage`
A handle representing a CPU-side image. It contains the raw pixel data and metadata. All pixel data is stored in uncompressed 32-bit RGBA format unless otherwise specified. This struct is the primary source for creating GPU-side `SituationTexture`s.
```c
typedef struct SituationImage {
    void *data;
    int width;
    int height;
    int mipmaps;
    int format;
} SituationImage;
```
-   `data`: A pointer to the raw pixel data in system memory (RAM).
-   `width`: The width of the image in pixels.
-   `height`: The height of the image in pixels.
-   `mipmaps`: The number of mipmap levels generated for the image. `1` means no mipmaps.
-   `format`: The pixel format of the data (e.g., `SIT_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8`).

---
#### `SituationFont`
A handle representing a font, which includes a texture atlas of its characters (glyphs) and the data needed to render them. Fonts are typically loaded from TTF/OTF files and are used for both CPU-side (`SituationImageDrawText`) and GPU-side rendering.
```c
typedef struct SituationFont {
    int baseSize;
    int glyphCount;
    int glyphPadding;
    SituationTexture texture;
    Rectangle *recs;
    GlyphInfo *glyphs;
} SituationFont;
```
-   `baseSize`: The base font size (in pixels) that the font atlas was generated with.
-   `glyphCount`: The number of character glyphs packed into the texture atlas.
-   `glyphPadding`: The padding (in pixels) around each glyph in the atlas to prevent texture bleeding.
-   `texture`: The GPU `SituationTexture` handle for the font atlas image.
-   `recs`: A pointer to an array of `Rectangle` structs, defining the source rectangle for each glyph within the texture atlas.
-   `glyphs`: A pointer to an array of internal `GlyphInfo` structs, containing the character code, advance width, and offset for each glyph. This data is used for positioning characters correctly when rendering text.

---
#### `GlyphInfo`
Contains the rendering metrics for a single character glyph within a `SituationFont`. This struct is managed by the library and is used internally to calculate the correct position and spacing for each character when drawing text.
```c
typedef struct GlyphInfo {
    int value;
    int offsetX;
    int offsetY;
    int advanceX;
    SituationImage image;
} GlyphInfo;
```
-   `value`: The Unicode codepoint for the character (e.g., `65` for 'A').
-   `offsetX`, `offsetY`: The offset from the cursor position to the top-left corner of the glyph image when rendering.
-   `advanceX`: The horizontal distance to advance the cursor after rendering this glyph.
-   `image`: A CPU-side `SituationImage` containing the pixel data for the glyph. This is primarily used during the font atlas generation process.

### Functions

#### Image Loading and Unloading
---
#### `SituationLoadImage`
Loads an image from a file into CPU memory (RAM). Supported formats include PNG, BMP, TGA, and JPEG. All loaded images are converted to a 32-bit RGBA format.
```c
SituationError SituationLoadImage(const char *fileName, SituationImage* out_image);
```
**Usage Example:**
```c
// Load an image to be used for a player sprite.
SituationImage player_avatar;
if (SituationLoadImage("assets/sprites/player.png", &player_avatar) == SITUATION_SUCCESS) {
    // The image is now in CPU memory, ready to be manipulated or uploaded to the GPU.
    SituationTexture player_texture;
    SituationCreateTexture(player_avatar, true, &player_texture);
    // Once uploaded to a texture, the CPU-side copy can often be safely unloaded.
    SituationUnloadImage(player_avatar);
}
```

---
#### `SituationUnloadImage`
Unloads a CPU-side image and frees its associated memory.
```c
void SituationUnloadImage(SituationImage image);
```
**Usage Example:**
```c
SituationImage temp_image = SituationGenImageColor(128, 128, (ColorRGBA){255, 0, 255, 255});
// ... perform some operations on the image ...
SituationUnloadImage(temp_image); // Free the memory when done.
```
---
#### `SituationLoadImageFromMemory`
Loads an image from a data buffer in memory. The `fileType` parameter must include the leading dot (e.g., `.png`).
```c
SituationError SituationLoadImageFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, SituationImage* out_image);
```
**Usage Example:**
```c
// Assume 'g_embedded_player_png' is a byte array with an embedded PNG file,
// and 'g_embedded_player_png_len' is its size.
SituationImage player_img;
SituationLoadImageFromMemory(".png", g_embedded_player_png, g_embedded_player_png_len, &player_img);
// ... use player_img ...
SituationUnloadImage(player_img);
```

---
#### `SituationLoadImageFromTexture`
Creates a CPU-side `SituationImage` by reading back pixel data from a GPU `SituationTexture`. This can be a slow operation and should be used sparingly.
```c
SituationImage SituationLoadImageFromTexture(SituationTexture texture);
```

---
#### `SituationExportImage`
Exports the pixel data of a `SituationImage` to a file. The file format is determined by the extension. Currently, `.png` and `.bmp` are supported.
```c
SituationError SituationExportImage(SituationImage image, const char *fileName);
```
**Usage Example:**
```c
// Take a screenshot and save it as a PNG.
SituationImage screenshot = {0};
if (SituationLoadImageFromScreen(&screenshot) == SITUATION_SUCCESS) {
    SituationExportImage(screenshot, "screenshots/capture.png");
    SituationUnloadImage(screenshot);
}
```

---
#### Image Generation and Manipulation

---
#### Image Generation and Manipulation
---
#### `SituationImageFromImage`
Creates a new `SituationImage` by copying a sub-rectangle from a source image. This is useful for extracting sprites from a spritesheet.
```c
SituationImage SituationImageFromImage(SituationImage image, Rectangle rect);
```
**Usage Example:**
```c
// Extract a 16x16 sprite from a larger spritesheet.
SituationImage spritesheet;
if (SituationLoadImage("assets/sprites.png", &spritesheet) == SITUATION_SUCCESS) {
    Rectangle sprite_rect = { .x = 32, .y = 16, .width = 16, .height = 16 };
    SituationImage single_sprite = SituationImageFromImage(spritesheet, sprite_rect);
    // 'single_sprite' is now a new 16x16 image that can be used independently.
    // ... use single_sprite ...
    SituationUnloadImage(single_sprite);
    SituationUnloadImage(spritesheet);
}
```

---
#### `SituationImageCopy`
Creates a new image by making a deep copy of another. This is useful when you need to modify an image while preserving the original.
```c
SituationError SituationImageCopy(SituationImage image, SituationImage* out_image);
```

---
#### `SituationCreateImage`
Allocates a new SituationImage container with UNINITIALIZED data.
```c
SituationError SituationCreateImage(int width, int height, int channels, SituationImage* out_image);
```

---
#### `SituationGenImageColor`
Generates a new image filled with a single, solid color.
```c
SituationError SituationGenImageColor(int width, int height, ColorRGBA color, SituationImage* out_image);
```
**Usage Example:**
```c
// Create a solid red 1x1 pixel image to use as a default texture.
SituationImage red_pixel;
if (SituationGenImageColor(1, 1, (ColorRGBA){255, 0, 0, 255}, &red_pixel) == SITUATION_SUCCESS) {
    SituationTexture default_texture;
    SituationCreateTexture(red_pixel, false, &default_texture);
    SituationUnloadImage(red_pixel);
}
```

---
#### `SituationGenImageGradient`
Generates an image with a linear, radial, or square gradient.
```c
SituationError SituationGenImageGradient(int width, int height, ColorRGBA tl, ColorRGBA tr, ColorRGBA bl, ColorRGBA br, SituationImage* out_image);
```
**Usage Example:**
```c
// Create a vertical gradient from red to black
SituationImage background;
if (SituationGenImageGradient(1280, 720, (ColorRGBA){255,0,0,255}, (ColorRGBA){255,0,0,255}, (ColorRGBA){0,0,0,255}, (ColorRGBA){0,0,0,255}, &background) == SITUATION_SUCCESS) {
    // ... use background ...
    SituationUnloadImage(background);
}
```

---
#### `SituationImageClearBackground`
Fills the entire destination image with a specified color.
```c
void SituationImageClearBackground(SituationImage *dst, ColorRGBA color);
```

---
#### `SituationImageDraw`
Draws a source image (or a sub-rectangle of it) onto a destination image, with scaling and tinting.
```c
void SituationImageDraw(SituationImage *dst, SituationImage src, Rectangle srcRect, Rectangle dstRect, ColorRGBA tint);
```

---
#### `SituationImageDrawRectangle` / `SituationImageDrawLine`
Draws a colored rectangle or line directly onto an image's pixel data.
```c
void SituationImageDrawRectangle(SituationImage *dst, Rectangle rect, ColorRGBA color);
void SituationImageDrawLine(SituationImage *dst, Vector2 start, Vector2 end, ColorRGBA color);
```
**Usage Example:**
```c
// Create a canvas and draw a red border around it.
SituationImage canvas = SituationGenImageColor(256, 256, (ColorRGBA){255,255,255,255});
Rectangle border = { .x = 0, .y = 0, .width = 256, .height = 256 };
SituationImageDrawRectangleLines(&canvas, border, 4, (ColorRGBA){255,0,0,255});
```

---
#### `SituationImageCrop` / `SituationImageResize`
Crops or resizes an image in-place.
```c
void SituationImageCrop(SituationImage *image, Rectangle crop);
void SituationImageResize(SituationImage *image, int newWidth, int newHeight);
```
**Usage Example:**
```c
SituationImage atlas;
if (SituationLoadImage("sprite_atlas.png", &atlas) == SITUATION_SUCCESS) {
    // Crop the atlas to get the first sprite (e.g., a 32x32 sprite at top-left)
    SituationImageCrop(&atlas, (Rectangle){0, 0, 32, 32});
    // Now 'atlas' contains only the cropped sprite data.
    SituationUnloadImage(atlas);
}
```

---
#### `SituationImageFlipVertical` / `SituationImageFlipHorizontal`
Flips an image vertically or horizontally in-place.
```c
void SituationImageFlipVertical(SituationImage *image);
void SituationImageFlipHorizontal(SituationImage *image);
```

---
#### `SituationImageRotate`
Rotates an image by a multiple of 90 degrees clockwise in-place.
```c
void SituationImageRotate(SituationImage *image, int rotations);
```

---
#### `SituationImageColorTint` / `SituationImageColorInvert`
Applies a color tint or inverts the colors of an image in-place.
```c
void SituationImageColorTint(SituationImage *image, ColorRGBA color);
void SituationImageColorInvert(SituationImage *image);
```

---
#### `SituationImageColorGrayscale` / `SituationImageColorContrast` / `SituationImageColorBrightness`
Adjusts the grayscale, contrast, or brightness of an image in-place.
```c
void SituationImageColorGrayscale(SituationImage *image);
void SituationImageColorContrast(SituationImage *image, float contrast);
void SituationImageColorBrightness(SituationImage *image, int brightness);
```

---
#### `SituationImageAlphaMask` / `SituationImagePremultiplyAlpha`
Applies an alpha mask to an image or premultiplies the color channels by the alpha channel.
```c
void SituationImageAlphaMask(SituationImage *image, SituationImage alphaMask);
void SituationImagePremultiplyAlpha(SituationImage *image);
```

---
#### Font and Text Rendering
---
#### `SituationLoadFont` / `SituationUnloadFont`
Loads a font from a TTF/OTF file for CPU-side rendering, and later unloads it.
```c
SituationError SituationLoadFont(const char *fileName, SituationFont* out_font);
void SituationUnloadFont(SituationFont font);
```

---
#### `SituationLoadFontFromMemory`
Loads a font from a data buffer in memory.
```c
SituationError SituationLoadFontFromMemory(const void *fileData, int dataSize, SituationFont* out_font);
```

---
#### `SituationBakeFontAtlas`
Generates a texture atlas (a single image containing all characters) from a font. This is a more advanced and efficient way to handle font rendering.
```c
SituationError SituationBakeFontAtlas(SituationFont* font, float fontSizePixels);
```

---
#### `SituationMeasureText`
Measures the dimensions of a string of text if it were to be rendered with a specific font, size, and spacing.
```c
Rectangle SituationMeasureText(SituationFont font, const char *text, float fontSize);
```
**Usage Example:**
```c
const char* button_text = "Click Me!";
Rectangle text_size = SituationMeasureText(my_font, button_text, 20);
// Now you can create a button rectangle that perfectly fits the text.
Rectangle button_rect = { .x = 100, .y = 100, .width = text_size.width + 20, .height = text_size.height + 10 };
```

---
#### `SituationImageDrawText`
Draws a simple, tinted text string onto an image.
```c
void SituationImageDrawText(SituationImage *dst, SituationFont font, const char *text, Vector2 position, float fontSize, float spacing, ColorRGBA tint);
```
**Usage Example:**
```c
SituationImage canvas;
SituationGenImageColor(800, 600, (ColorRGBA){20, 20, 20, 255}, &canvas);
SituationFont my_font;
if (SituationLoadFont("fonts/my_font.ttf", &my_font) == SITUATION_SUCCESS) {
    SituationImageDrawText(&canvas, my_font, "Hello, World!", (Vector2){50, 50}, 40, 1, (ColorRGBA){255, 255, 255, 255});

    // ... you can now upload 'canvas' to a GPU texture ...

    SituationUnloadFont(my_font);
}
SituationUnloadImage(canvas);
```

---
#### Pixel-Level Access
---
#### `SituationGetPixelColor`
Gets the color of a single pixel from an image.
```c
ColorRGBA SituationGetPixelColor(SituationImage image, int x, int y);
```
**Usage Example:**
```c
SituationImage my_image;
if (SituationLoadImage("assets/my_image.png", &my_image) == SITUATION_SUCCESS) {
    ColorRGBA top_left_pixel = SituationGetPixelColor(my_image, 0, 0);
    printf("Top-left pixel color: R%d G%d B%d A%d\n",
           top_left_pixel.r, top_left_pixel.g, top_left_pixel.b, top_left_pixel.a);
    SituationUnloadImage(my_image);
}
```

---
#### `SituationSetPixelColor`
Sets the color of a single pixel in an image.
```c
void SituationSetPixelColor(SituationImage* image, int x, int y, ColorRGBA color);
```
**Usage Example:**
```c
SituationImage canvas = SituationGenImageColor(10, 10, (ColorRGBA){0, 0, 0, 255});
// Draw a red pixel in the center
SituationSetPixelColor(&canvas, 5, 5, (ColorRGBA){255, 0, 0, 255});
// ... use the canvas ...
SituationUnloadImage(canvas);
```

---
#### `SituationIsImageValid`
Checks if an image has been loaded successfully.
```c
bool SituationIsImageValid(SituationImage image);
```

---
#### `SituationImageDrawAlpha`
Copying portion of one image into another image at destination placement with alpha blending.
```c
void SituationImageDrawAlpha(SituationImage *dst, SituationImage src, Rectangle srcRect, Vector2 dstPos, ColorRGBA tint);
```

---
#### `SituationImageResize`
Resizes an image using default bicubic scaling.
```c
void SituationImageResize(SituationImage *image, int newWidth, int newHeight);
```

---
#### `SituationImageFlip`
Flips an image.
```c
void SituationImageFlip(SituationImage *image, SituationImageFlipMode mode);
```

---
#### `SituationImageAdjustHSV`
Controls an image by Hue Saturation and Brightness.
```c
void SituationImageAdjustHSV(SituationImage *image, float hue_shift, float sat_factor, float val_factor, float mix);
```

---
#### `SituationUnloadFont`
Unloads a CPU-side font and frees its memory.
```c
void SituationUnloadFont(SituationFont font);
```

---
#### `SituationImageDrawCodepoint`
Draws a single Unicode character with advanced styling onto an image.
```c
void SituationImageDrawCodepoint(SituationImage *dst, SituationFont font, int codepoint, Vector2 position, float fontSize, float rotationDegrees, float skewFactor, ColorRGBA fillColor, ColorRGBA outlineColor, float outlineThickness);
```

---
#### `SituationImageDrawTextEx`
Draws a text string with advanced styling (rotation, outline) onto an image.
```c
void SituationImageDrawTextEx(SituationImage *dst, SituationFont font, const char *text, Vector2 position, float fontSize, float spacing, float rotationDegrees, float skewFactor, ColorRGBA fillColor, ColorRGBA outlineColor, float outlineThickness);
```

---

#### `SituationImageDraw`
Draws a source image onto a destination image.
```c
SITAPI void SituationImageDraw(SituationImage *dst, SituationImage src, Rectangle srcRect, Vector2 dstPos);
```
**Usage Example:**
```c
SituationImage canvas;
if (SituationGenImageColor(256, 26, (ColorRGBA){255, 255, 255, 255}, &canvas) == SITUATION_SUCCESS) {
    SituationImage sprite;
    if (SituationLoadImage("assets/sprite.png", &sprite) == SITUATION_SUCCESS) {
        Rectangle sprite_rect = { .x = 0, .y = 0, .width = 16, .height = 16 };
        Vector2 position = { .x = 120, .y = 120 };
        SituationImageDraw(&canvas, sprite, sprite_rect, position);
        SituationUnloadImage(sprite);
    }
    // ... use canvas ...
    SituationUnloadImage(canvas);
}
```

---

#### `SituationGenImageGradient`
Generates an image with a linear gradient.
```c
SITAPI SituationError SituationGenImageGradient(int width, int height, ColorRGBA tl, ColorRGBA tr, ColorRGBA bl, ColorRGBA br, SituationImage* out_image);
```
**Usage Example:**
```c
// Create a vertical gradient from red to black
SituationImage background;
if (SituationGenImageGradient(1280, 720, (ColorRGBA){255,0,0,255}, (ColorRGBA){255,0,0,255}, (ColorRGBA){0,0,0,255}, (ColorRGBA){0,0,0,255}, &background) == SITUATION_SUCCESS) {
    // ... use background ...
    SituationUnloadImage(background);
}
```

---

#### `SituationMeasureText`
Measures the dimensions of a string of text if it were to be rendered with a specific font and size.
```c
SITAPI Rectangle SituationMeasureText(SituationFont font, const char *text, float fontSize);
```
**Usage Example:**
```c
const char* button_text = "Click Me!";
SituationFont my_font;
if (SituationLoadFont("fonts/my_font.ttf", &my_font) == SITUATION_SUCCESS) {
    Rectangle text_bounds = SituationMeasureText(my_font, button_text, 20);
    // Now you can create a button rectangle that perfectly fits the text.
    Rectangle button_rect = { .x = 100, .y = 100, .width = text_bounds.width + 20, .height = text_bounds.height + 10 };
    SituationUnloadFont(my_font);
}
```
</details>
<details>
<summary><h3>Graphics Module</h3></summary>

**Overview:** The Graphics module forms the core of the rendering pipeline, offering a powerful, backend-agnostic API for interacting with the GPU. It is responsible for all GPU resource management (meshes, shaders, textures) and its command-buffer-centric design (`SituationCmd...`) allows you to precisely sequence rendering operations.

### Structs, Enums, and Handles

#### `SituationCommandBuffer`
An opaque handle representing a command buffer, which is a list of rendering commands to be executed by the GPU. The main command buffer for the window is retrieved via `SituationGetMainCommandBuffer()`. All rendering commands (`SituationCmd...`) are recorded into a command buffer.
```c
typedef struct SituationCommandBuffer_t* SituationCommandBuffer;
```

---
#### `SituationAttachmentInfo`
Describes a single attachment (like a color or depth buffer) for a render pass. It specifies how the attachment's contents should be handled at the beginning and end of the pass.
```c
typedef struct SituationAttachmentInfo {
    SituationAttachmentLoadOp  loadOp;
    SituationAttachmentStoreOp storeOp;
    SituationClearValue        clear;
} SituationAttachmentInfo;
```
-   `loadOp`: The action to take at the beginning of the render pass.
    -   `SIT_LOAD_OP_LOAD`: Preserve the existing contents of the attachment.
    -   `SIT_LOAD_OP_CLEAR`: Clear the attachment to the value specified in `clear`.
    -   `SIT_LOAD_OP_DONT_CARE`: The existing contents are undefined and can be discarded.
-   `storeOp`: The action to take at the end of the render pass.
    -   `SIT_STORE_OP_STORE`: Store the rendered contents to memory.
    -   `SIT_STORE_OP_DONT_CARE`: The rendered contents may be discarded.
-   `clear`: A struct containing the color or depth/stencil values to use if `loadOp` is `SIT_LOAD_OP_CLEAR`.

---
#### `SituationClearValue`
A union that specifies the clear values for color and depth/stencil attachments. It is used within the `SituationAttachmentInfo` struct to define what value an attachment should be cleared to at the start of a render pass.
```c
typedef union SituationClearValue {
    ColorRGBA color;
    struct {
        double depth;
        int32_t stencil;
    } depth_stencil;
} SituationClearValue;
```
-   `color`: The RGBA color value to clear a color attachment to.
-   `depth_stencil`: A struct containing the depth and stencil values to clear a depth/stencil attachment to.
    -   `depth`: The depth value, typically `1.0` for clearing.
    -   `stencil`: The stencil value, typically `0`.

---
#### `SituationRenderPassInfo`
Configures a rendering pass. This is a crucial struct used with `SituationCmdBeginRenderPass()` to define the render target and its initial state.
```c
typedef struct SituationRenderPassInfo {
    int                     display_id;
    SituationAttachmentInfo color_attachment;
    SituationAttachmentInfo depth_attachment;
} SituationRenderPassInfo;
```
-   `display_id`: The ID of a `SituationVirtualDisplay` to render to. Use `-1` to target the main window's backbuffer.
-   `color_attachment`: Configuration for the color buffer, including load/store operations and clear color.
-   `depth_attachment`: Configuration for the depth buffer, including load/store operations and clear value.

---
#### `ViewDataUBO`
Defines the standard memory layout for a Uniform Buffer Object (UBO) containing camera projection and view matrices. You don't typically create this struct directly; rather, you should structure your GLSL uniform blocks to match this layout to be compatible with the library's default scene data.
```c
typedef struct ViewDataUBO {
    mat4 view;
    mat4 projection;
} ViewDataUBO;
```
-   `view`: The view matrix, which transforms world-space coordinates to view-space (camera) coordinates.
-   `projection`: The projection matrix, which transforms view-space coordinates to clip-space coordinates.

---
#### Resource Handles
The following are opaque handles to GPU resources. Their internal structure is not exposed to the user. You create them with `SituationCreate...` or `SituationLoad...` functions and free them with their corresponding `SituationDestroy...` or `SituationUnload...` functions.

#### `SituationMesh`
An opaque handle to a self-contained GPU resource representing a drawable mesh. A mesh bundles a vertex buffer and an optional index buffer, representing a complete piece of geometry that can be rendered with a single command.
```c
typedef struct SituationMesh {
    uint64_t id;
    int index_count;
} SituationMesh;
```
- **Creation:** `SituationCreateMesh()`
- **Usage:** `SituationCmdDrawMesh()`
- **Destruction:** `SituationDestroyMesh()`

---
#### `SituationBuffer`
An opaque handle to a generic region of GPU memory. Buffers are highly versatile and can be used to store vertex data, index data, uniform data for shaders (UBOs), or general-purpose storage data (SSBOs). The intended usage is specified on creation using `SituationBufferUsageFlags`.
```c
typedef struct SituationBuffer {
    uint64_t id;
    size_t size_in_bytes;
} SituationBuffer;
```
- **Creation:** `SituationCreateBuffer()`
- **Usage:** `SituationUpdateBuffer()`, `SituationCmdBindVertexBuffer()`, `SituationCmdBindIndexBuffer()`, `SituationCmdBindDescriptorSet()`
- **Destruction:** `SituationDestroyBuffer()`

---
#### `SituationComputePipeline`
An opaque handle representing a compiled compute shader program. It encapsulates a single compute shader stage and its resource layout, ready to be dispatched for general-purpose GPU computation.
```c
typedef struct SituationComputePipeline {
    uint64_t id;
} SituationComputePipeline;
```
- **Creation:** `SituationCreateComputePipeline()`
- **Usage:** `SituationCmdBindComputePipeline()`, `SituationCmdDispatch()`
- **Destruction:** `SituationDestroyComputePipeline()`

---
#### `SituationShader`
An opaque handle representing a compiled graphics shader pipeline. It encapsulates a vertex shader, a fragment shader, and the state required to use them for rendering (like vertex input layout and descriptor set layouts).
```c
typedef struct SituationShader {
    uint64_t id;
} SituationShader;
```
- **Creation:** `SituationLoadShader()`, `SituationLoadShaderFromMemory()`
- **Usage:** `SituationCmdBindPipeline()`
- **Destruction:** `SituationUnloadShader()`

---
#### `SituationTexture`
An opaque handle to a GPU texture resource. Textures are created by uploading `SituationImage` data from the CPU. They are used by shaders for sampling colors (e.g., albedo maps) or as storage images for compute operations.
```c
typedef struct SituationTexture {
    uint64_t id;
    int width;
    int height;
    int mipmaps;
} SituationTexture;
```
-   `width`, `height`: The dimensions of the texture in pixels.
-   `mipmaps`: The number of mipmap levels in the texture.
- **Creation:** `SituationCreateTexture()`
- **Usage:** `SituationCmdBindShaderTexture()`, `SituationCmdBindComputeTexture()`
- **Destruction:** `SituationDestroyTexture()`

---
#### `SituationModelMesh`
Represents a single drawable sub-mesh within a larger `SituationModel`. It combines the raw geometry (`SituationMesh`) with a full PBR (Physically-Based Rendering) material definition, including color factors and texture maps.
```c
typedef struct SituationModelMesh {
    SituationMesh mesh;
    // Material Data
    int material_id;
    char material_name[SITUATION_MAX_NAME_LEN];
    ColorRGBA base_color_factor;
    float metallic_factor;
    float roughness_factor;
    vec3 emissive_factor;
    float alpha_cutoff;
    bool double_sided;
    // Texture Maps (if available)
    SituationTexture base_color_texture;
    SituationTexture metallic_roughness_texture;
    SituationTexture normal_texture;
    SituationTexture occlusion_texture;
    SituationTexture emissive_texture;
} SituationModelMesh;
```
-   `mesh`: The `SituationMesh` handle containing the vertex and index buffers for this part of the model.
-   `material_name`: The name of the material.
-   `base_color_factor`, `metallic_factor`, `roughness_factor`: PBR material parameters.
-   `base_color_texture`, `metallic_roughness_texture`, etc.: Handles to the GPU textures used by this material.

---
#### `SituationModel`
A handle representing a complete 3D model, loaded from a file (e.g., GLTF). It acts as a container for all the `SituationModelMesh` and `SituationTexture` resources that make up the model.
```c
typedef struct SituationModel {
    SituationModelMesh* meshes;
    SituationTexture* all_model_textures;
    int mesh_count;
    int texture_count;
} SituationModel;
```
-   `meshes`: A pointer to an array of the model's sub-meshes.
-   `all_model_textures`: A pointer to an array of all unique textures used by the model.
-   `mesh_count`, `texture_count`: The number of meshes and textures in their respective arrays.
- **Creation:** `SituationLoadModel()`
- **Usage:** `SituationDrawModel()`
- **Destruction:** `SituationUnloadModel()`

---
#### `SituationBufferUsageFlags`
Specifies how a `SituationBuffer` will be used. This helps the driver place the buffer in the most optimal memory. Combine flags using the bitwise `|` operator.
| Flag | Description |
|---|---|
| `SITUATION_BUFFER_USAGE_VERTEX_BUFFER` | The buffer will be used as a source of vertex data. |
| `SITUATION_BUFFER_USAGE_INDEX_BUFFER` | The buffer will be used as a source of index data. |
| `SITUATION_BUFFER_USAGE_UNIFORM_BUFFER` | The buffer will be used as a Uniform Buffer Object (UBO). |
| `SITUATION_BUFFER_USAGE_STORAGE_BUFFER` | The buffer will be used as a Shader Storage Buffer Object (SSBO). |
| `SITUATION_BUFFER_USAGE_INDIRECT_BUFFER`| The buffer will be used for indirect drawing commands. |
| `SITUATION_BUFFER_USAGE_TRANSFER_SRC`| The buffer can be used as a source for a copy operation. |
| `SITUATION_BUFFER_USAGE_TRANSFER_DST`| The buffer can be used as a destination for a copy operation. |

---
#### `SituationComputeLayoutType`
Defines a set of common, pre-configured layouts for compute pipelines, telling the GPU what kind of resources the shader expects.

| Type | Description |
|---|---|
| `SIT_COMPUTE_LAYOUT_ONE_SSBO` | One SSBO at binding 0 (Set 0). |
| `SIT_COMPUTE_LAYOUT_TWO_SSBOS` | Two SSBOs at bindings 0 and 1 (Set 0). |
| `SIT_COMPUTE_LAYOUT_IMAGE_AND_SSBO` | One Storage Image at binding 0, one SSBO at binding 1 (Set 0). |
| `SIT_COMPUTE_LAYOUT_PUSH_CONSTANT` | 64-byte push constant range (no descriptor sets). |
| `SIT_COMPUTE_LAYOUT_EMPTY` | No external resources. |
| `SIT_COMPUTE_LAYOUT_BUFFER_IMAGE` | One SSBO at binding 0, one Storage Image at binding 1 (Set 0). |
| `SIT_COMPUTE_LAYOUT_TERMINAL` | Specialized layout: SSBO (Set 0), Storage Image (Set 1), Font Sampler (Set 2). |

---
#### Resource Handles
`SituationMesh`, `SituationShader`, `SituationTexture`, `SituationBuffer`, `SituationModel`, `SituationComputePipeline`: These are opaque handles to GPU resources. Their internal structure is not exposed to the user. You create them with `SituationCreate...` or `SituationLoad...` functions and free them with their corresponding `SituationDestroy...` or `SituationUnload...` functions.

---
#### `SituationBufferUsageFlags`
Specifies how a `SituationBuffer` will be used. This helps the driver place the buffer in the most optimal memory.
| Flag | Description |
|---|---|
| `SIT_BUFFER_USAGE_VERTEX` | The buffer will be used as a vertex buffer. |
| `SIT_BUFFER_USAGE_INDEX` | The buffer will be used as an index buffer. |
| `SIT_BUFFER_USAGE_UNIFORM` | The buffer will be used as a Uniform Buffer Object (UBO). |
| `SIT_BUFFER_USAGE_STORAGE` | The buffer will be used as a Shader Storage Buffer Object (SSBO). |
| `SIT_BUFFER_USAGE_TRANSFER_SRC`| The buffer can be used as a source for a copy operation. |
| `SIT_BUFFER_USAGE_TRANSFER_DST`| The buffer can be used as a destination for a copy operation. |

---
#### `SituationComputeLayoutType`
Defines the descriptor set layout for a compute pipeline, telling the GPU what kind of resources the shader expects.
| Type | Description |
|---|---|
| `SIT_COMPUTE_LAYOUT_EMPTY`| The compute shader does not use any resources. |
| `SIT_COMPUTE_LAYOUT_IMAGE`| The pipeline expects a single storage image to be bound at binding 0. |
| `SIT_COMPUTE_LAYOUT_BUFFER`| The pipeline expects a single storage buffer to be bound at binding 0. |
| `SIT_COMPUTE_LAYOUT_BUFFER_X2`| The pipeline expects two storage buffers to be bound at bindings 0 and 1. |


#### Functions
### Functions

#### Frame Lifecycle & Command Buffer
These functions control the overall rendering loop.

---
---
#### `SituationAcquireFrameCommandBuffer`
Prepares the backend for a new frame of rendering, acquiring the next available render target from the swap chain. This is the first function to call in the render phase and it must be guarded by a conditional check. It returns `false` if the frame cannot be acquired (e.g., because the window is minimized), in which case you should skip all rendering for that frame.
```c
bool SituationAcquireFrameCommandBuffer(void);
```
**Usage Example:**
```c
// At the start of the rendering phase
if (SituationAcquireFrameCommandBuffer()) {
    // It's safe to record rendering commands now.
    // ...
    SituationEndFrame();
} else {
    // Skip rendering for this frame.
}
```

---
#### `SituationEndFrame`
Submits all recorded commands for the frame to the GPU for execution and presents the final rendered image to the screen. This is the last function to call in the render phase.
```c
SituationError SituationEndFrame(void);
```
**Usage Example:**
```c
// At the very end of the rendering phase
if (SituationAcquireFrameCommandBuffer()) {
    // ... record all rendering commands ...

    // Finally, submit and present the frame.
    SituationEndFrame();
}
```

---
#### `SituationGetMainCommandBuffer`
Gets a handle to the main command buffer. This command buffer is used for all rendering that targets the main window.
```c
SituationCommandBuffer SituationGetMainCommandBuffer(void);
```

---
#### `SituationCmdBeginRenderPass` / `SituationCmdEndRenderPass`
Begins and ends a render pass. A render pass defines the render target (e.g., the main window or a virtual display) and how its attachments (color, depth) should be handled. All drawing commands must be recorded between these two calls.
```c
SituationError SituationCmdBeginRenderPass(SituationCommandBuffer cmd, const SituationRenderPassInfo* info);
SituationError SituationCmdEndRenderPass(SituationCommandBuffer cmd);
```
**Usage Example:**
```c
// Define a render pass that clears the screen to a dark blue color.
SituationRenderPassInfo pass_info = {
    .color_load_action = SIT_LOAD_ACTION_CLEAR,
    .clear_color = {20, 30, 40, 255},
    .color_store_action = SIT_STORE_ACTION_STORE,
    .virtual_display_id = -1 // Target the main window
};
SituationCmdBeginRenderPass(SituationGetMainCommandBuffer(), &pass_info);

// ... All your drawing commands for this pass go here ...

SituationCmdEndRenderPass(SituationGetMainCommandBuffer());
```

---
#### Rendering Commands
These functions record drawing and state-setting operations into the command buffer.

---

#### `SituationPauseApp`

Pauses the application.

```c
SITAPI void SituationPauseApp(void);
```

**Usage Example:**
```c
SituationPauseApp();
```

---

#### `SituationGetMonitorHeight`

Gets the current height of a monitor in screen coordinates.

```c
SITAPI int SituationGetMonitorHeight(int monitor_id);
```

**Usage Example:**
```c
int primary_monitor_height = SituationGetMonitorHeight(0);
printf("Primary monitor height: %d\n", primary_monitor_height);
```

---
#### `SituationCmdSetViewport` / `SituationCmdSetScissor`
Sets the dynamic viewport or scissor rectangle for the current render pass. The viewport transforms the normalized device coordinates to window coordinates, while the scissor rectangle discards fragments outside its bounds.
```c
SituationError SituationCmdSetViewport(SituationCommandBuffer cmd, float x, float y, float width, float height);
SituationError SituationCmdSetScissor(SituationCommandBuffer cmd, int x, int y, int width, int height);
```
**Usage Example:**
```c
// Render to the left half of the screen for a split-screen effect.
int w = SituationGetRenderWidth();
int h = SituationGetRenderHeight();
SituationCmdSetViewport(SituationGetMainCommandBuffer(), 0, 0, w / 2.0f, h);
SituationCmdSetScissor(SituationGetMainCommandBuffer(), 0, 0, w / 2, h);
```

---
#### `SituationCmdBindPipeline`
Binds a graphics pipeline (which includes the shader program and its state) for subsequent drawing commands. All draws following this call will use this pipeline until a new one is bound.
```c
SituationError SituationCmdBindPipeline(SituationCommandBuffer cmd, SituationShader shader);
```
**Usage Example:**
```c
// Bind the main shader for drawing the 3D scene.
SituationCmdBindPipeline(SituationGetMainCommandBuffer(), my_3d_shader);
SituationCmdDrawMesh(SituationGetMainCommandBuffer(), my_scene_mesh);

// Bind a different shader for drawing the UI.
SituationCmdBindPipeline(SituationGetMainCommandBuffer(), my_ui_shader);
SituationCmdDrawMesh(SituationGetMainCommandBuffer(), my_ui_mesh);
```

---
#### `SituationCmdBindVertexBuffer`
Binds a vertex buffer for subsequent drawing commands. The bound vertex buffer provides the vertex data (position, color, normals, etc.) for the draws that follow.
```c
void SituationCmdBindVertexBuffer(SituationCommandBuffer cmd, SituationBuffer buffer);
```
**Usage Example:**
```c
// Before drawing, bind the vertex buffer containing your model's vertices.
SituationCmdBindVertexBuffer(cmd, my_model_vertex_buffer);
SituationCmdDraw(cmd, 0, 36); // Draw 36 vertices from the bound buffer.
```

---
#### `SituationCmdBindIndexBuffer`
Binds an index buffer for subsequent indexed drawing commands (`SituationCmdDrawIndexed`). An index buffer tells the GPU the order in which to draw vertices from the vertex buffer, allowing for the reuse of vertices and more efficient rendering of complex meshes.
```c
void SituationCmdBindIndexBuffer(SituationCommandBuffer cmd, SituationBuffer buffer);
```
**Usage Example:**
```c
// Before an indexed draw, bind both the vertex and index buffers.
SituationCmdBindVertexBuffer(cmd, my_mesh_vbo);
SituationCmdBindIndexBuffer(cmd, my_mesh_ibo);
// Draw using the index buffer. This will draw 12 triangles (36 indices).
SituationCmdDrawIndexed(cmd, 0, 36, 0);
```

---
#### `SituationCmdBindShaderBuffer` / `SituationCmdBindShaderTexture`
Binds a uniform/storage buffer or a texture to a specific binding point, making it accessible to the currently bound shader. The `binding` index corresponds to the `binding = N` layout qualifier in the GLSL shader code.
```c
void SituationCmdBindShaderBuffer(SituationCommandBuffer cmd, int binding, SituationBuffer buffer);
void SituationCmdBindShaderTexture(SituationCommandBuffer cmd, int binding, SituationTexture texture);
```
**Usage Example:**
```c
/* GLSL Shader Code:
layout(binding = 0) uniform sampler2D u_albedo;
layout(binding = 1) uniform SceneData {
    mat4 view;
    mat4 proj;
} u_scene;
*/

// In C, bind the corresponding resources to the correct binding points:
SituationCmdBindShaderTexture(cmd, 0, my_albedo_texture);
SituationCmdBindShaderBuffer(cmd, 1, my_scene_ubo);
```

---
#### `SituationCmdDraw` / `SituationCmdDrawIndexed`
Records a non-indexed or indexed drawing command into the command buffer. `SituationCmdDraw` draws vertices sequentially from the bound vertex buffer, while `SituationCmdDrawIndexed` uses the bound index buffer to determine the order of vertices.
```c
SituationError SituationCmdDraw(SituationCommandBuffer cmd, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
SituationError SituationCmdDrawIndexed(SituationCommandBuffer cmd, uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);
```
**Usage Example:**
```c
// Draw a mesh using previously bound vertex and index buffers.
SituationCmdBindVertexBuffer(cmd, my_vbo);
SituationCmdBindIndexBuffer(cmd, my_ibo);
// Draw 36 indices, starting from the beginning of the index buffer.
SituationCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);
```

---
#### `SituationCmdDrawMesh`
A high-level drawing command that records a command to draw a complete, pre-configured `SituationMesh` object. This is often more convenient than binding vertex and index buffers manually.
```c
SituationError SituationCmdDrawMesh(SituationCommandBuffer cmd, SituationMesh mesh);
```
**Usage Example:**
```c
// Bind the shader you want to use for this mesh.
SituationCmdBindPipeline(SituationGetMainCommandBuffer(), my_shader);
// The mesh object contains its own vertex and index buffers, which are automatically used.
SituationCmdDrawMesh(SituationGetMainCommandBuffer(), my_complex_model_mesh);
```

---
#### `SituationCmdDrawQuad`
Records a command to draw a simple, colored, and transformed 2D quad. This uses an internally managed quad mesh, so you don't need to create your own. It's useful for debug rendering, particles, or simple UI elements.
```c
SituationError SituationCmdDrawQuad(SituationCommandBuffer cmd, mat4 model, Vector4 color);
```
**Usage Example:**
```c
// Create a transformation matrix to position and scale the quad.
mat4 transform;
glm_translate_make(transform, (vec3){100.0f, 200.0f, 0.0f});
glm_scale_uni(transform, 50.0f); // Make it 50x50 pixels

// Define a color (in this case, magenta).
Vector4 quad_color = {{1.0f, 0.0f, 1.0f, 1.0f}};

// Record the draw command.
SituationCmdDrawQuad(SituationGetMainCommandBuffer(), transform, quad_color);
```

---
#### Resource Management
These functions create and destroy GPU resources.

---

#### `SituationShowCursor`

Shows the cursor.

```c
SITAPI void SituationShowCursor(void);
```

**Usage Example:**
```c
SituationShowCursor();
```

---
#### `SituationCreateMesh`
Creates a self-contained GPU mesh from vertex and index data. This operation uploads the provided data to video memory.
```c
SituationError SituationCreateMesh(const void* vertex_data, int vertex_count, size_t vertex_stride, const uint32_t* index_data, int index_count, SituationMesh* out_mesh);
```
**Usage Example:**
```c
// Define vertex and index data for a quad.
MyVertex vertices[] = { ... };
uint32_t indices[] = { ... };

// Create the mesh resource.
SituationMesh quad_mesh;
if (SituationCreateMesh(vertices, 4, sizeof(MyVertex), indices, 6, &quad_mesh) == SITUATION_SUCCESS) {
    // ...
}
```

---
#### `SituationDestroyMesh`
Destroys a mesh and frees its associated GPU memory. The handle is invalidated after this call.
```c
void SituationDestroyMesh(SituationMesh* mesh);
```
**Usage Example:**
```c
// Assume quad_mesh is a valid handle created with SituationCreateMesh.
// At application shutdown or when the mesh is no longer needed:
SituationDestroyMesh(&quad_mesh);
// The quad_mesh handle is now invalid.
```

---
#### `SituationLoadShader`
Loads, compiles, and links a graphics shader pipeline from GLSL vertex and fragment shader files.
```c
SituationError SituationLoadShader(const char* vs_path, const char* fs_path, SituationShader* out_shader);
```
**Usage Example:**
```c
// At application startup, load the main shader.
SituationShader main_shader;
SituationLoadShader("shaders/main.vert", "shaders/main.frag", &main_shader);
```

---
#### `SituationUnloadShader`
Unloads a shader pipeline and frees its associated GPU resources.
```c
void SituationUnloadShader(SituationShader* shader);
```
**Usage Example:**
```c
// At application shutdown, unload the main shader.
SituationUnloadShader(&main_shader);
```

---
#### `SituationCreateTexture`
Creates a GPU texture from a CPU-side `SituationImage`. This involves uploading the pixel data from RAM to VRAM.
```c
SituationError SituationCreateTexture(SituationImage image, bool generate_mipmaps, SituationTexture* out_texture);
```
**Usage Example:**
```c
// Load a CPU image from a file.
SituationImage cpu_image;
if (SituationLoadImage("textures/player_character.png", &cpu_image) == SITUATION_SUCCESS) {
    // Create a GPU texture from the image, generating mipmaps for better quality.
    SituationTexture player_texture;
    if (SituationCreateTexture(cpu_image, true, &player_texture) == SITUATION_SUCCESS) {
        // The CPU-side image can now be unloaded as the data is on the GPU.
        SituationUnloadImage(cpu_image);
    }
}
```

---
#### `SituationDestroyTexture`
Destroys a texture and frees its associated GPU memory. The handle is invalidated after this call.
```c
void SituationDestroyTexture(SituationTexture* texture);
```
**Usage Example:**
```c
// Assume player_texture is a valid handle.
// At application shutdown or when the texture is no longer needed:
SituationDestroyTexture(&player_texture);
// The player_texture handle is now invalid.
```

---
#### `SituationUpdateTexture`
Updates a texture with new pixel data from a `SituationImage`.
```c
void SituationUpdateTexture(SituationTexture texture, SituationImage image);
```
**Usage Example:**
```c
// Create a blank texture
SituationImage blank = SituationGenImageColor(256, 256, (ColorRGBA){0,0,0,255});
SituationTexture dynamic_texture;
SituationCreateTexture(blank, false, &dynamic_texture);
SituationUnloadImage(blank);

// Later, in the update loop, generate new image data
SituationImage new_data = generate_procedural_image();
SituationUpdateTexture(dynamic_texture, new_data);
SituationUnloadImage(new_data);
```

**Pro Tip (Zero-Copy Update):**
If you already have a raw data buffer (e.g., from a procedural generation function) and want to avoid allocating a new `SituationImage` on the heap, you can wrap your raw pointer in a stack-allocated `SituationImage`.
```c
// 'my_raw_pixels' is a pointer to your RGBA data.
SituationImage wrapper = {
    .width = 256,
    .height = 256,
    .data = my_raw_pixels,
    // .format defaults to 0 (RGBA), .mipmaps to 0/1
};
SituationUpdateTexture(dynamic_texture, wrapper);
// No need to call SituationUnloadImage(wrapper) since it owns nothing.
```

---
#### `SituationGetTextureHandle`
Retrieves the bindless texture handle for passing to shaders.
```c
SITAPI uint64_t SituationGetTextureHandle(SituationTexture texture);
```
-   **OpenGL:** Returns the 64-bit `GLuint64` handle from `glGetTextureHandleARB`. In GLSL, this is typically passed as a `uvec2` (unless `GL_ARB_gpu_shader_int64` is used) or accessed via `sampler2D` if using specific extensions.
-   **Vulkan:** Not yet implemented (returns 0).

---
#### `SituationCmdCopyTexture`
Records a command to copy pixel data from a source texture to a destination texture.
```c
SITAPI SituationError SituationCmdCopyTexture(SituationCommandBuffer cmd, SituationTexture src, SituationTexture dst);
```
-   **Usage:** Useful for feedback loops, copying render targets for post-processing, or backing up texture state.
-   **Note:** The source and destination textures must generally have compatible formats and dimensions.

---
#### `SituationGetTextureFormat`
Gets the internal GPU format of a texture.
```c
int SituationGetTextureFormat(SituationTexture texture);
```
**Usage Example:**
```c
int format = SituationGetTextureFormat(my_texture);
// The format will be one of the backend-specific pixel format enums (e.g., GL_RGBA8)
printf("Texture format ID: %d\n", format);
```

---
#### `SituationLoadModel`
Loads a 3D model from a file (GLTF, OBJ). This function parses the model file and uploads all associated meshes and materials to the GPU.
```c
SituationError SituationLoadModel(const char* file_path, SituationModel* out_model);
```
**Usage Example:**
```c
// At application startup, load the player model.
SituationModel player_model;
SituationLoadModel("models/player.gltf", &player_model);
```

---
#### `SituationUnloadModel`
Unloads a model and all of its associated resources (meshes, materials) from GPU memory.
```c
void SituationUnloadModel(SituationModel* model);
```
**Usage Example:**
```c
// At application shutdown, unload the player model.
SituationUnloadModel(&player_model);
```

---
#### `SituationCreateBuffer`
Creates a generic GPU buffer and optionally initializes it with data. Buffers can be used for vertices, indices, uniforms (UBOs), or storage (SSBOs).
```c
SituationError SituationCreateBuffer(size_t size, const void* initial_data, SituationBufferUsageFlags usage_flags, SituationBuffer* out_buffer);
```
**Usage Example:**
```c
// Create a uniform buffer for camera matrices
mat4 proj, view;
// ... calculate projection and view matrices ...
CameraMatrices ubo_data = { .projection = proj, .view = view };
SituationBuffer camera_ubo;
if (SituationCreateBuffer(sizeof(ubo_data), &ubo_data, SITUATION_BUFFER_USAGE_UNIFORM_BUFFER, &camera_ubo) == SITUATION_SUCCESS) {
    // ... use the buffer ...
}
```

---
#### `SituationDestroyBuffer`
Destroys a GPU buffer and frees its associated video memory. The handle is invalidated after this call.
```c
void SituationDestroyBuffer(SituationBuffer* buffer);
```
**Usage Example:**
```c
// Assume camera_ubo is a valid SituationBuffer handle created earlier.
// At application shutdown or when the buffer is no longer needed:
SituationDestroyBuffer(&camera_ubo);
// The camera_ubo handle is now invalid and should not be used.
```

---
#### `SituationGetBufferDeviceAddress`
Retrieves the GPU device address of a buffer. This `uint64_t` address can be passed to shaders (e.g., via a push constant) to access the buffer's memory directly using the `buffer_reference` feature (bindless).
```c
SITAPI uint64_t SituationGetBufferDeviceAddress(SituationBuffer buffer);
```

---
#### `SituationUpdateBuffer`
Updates the contents of a GPU buffer with new data from the CPU.
```c
SituationError SituationUpdateBuffer(SituationBuffer buffer, const void* data, size_t size);
```

---
#### Compute Shaders

---
#### `SituationCreateComputePipeline` / `SituationDestroyComputePipeline`
Creates a compute pipeline from a GLSL shader file.
```c
SituationError SituationCreateComputePipeline(const char* compute_shader_path, SituationComputeLayoutType layout_type, SituationComputePipeline* out_pipeline);
void SituationDestroyComputePipeline(SituationComputePipeline* pipeline);
```

---
#### `SituationCmdBindComputePipeline`
Binds a compute pipeline for a subsequent dispatch.
```c
void SituationCmdBindComputePipeline(SituationCommandBuffer cmd, SituationComputePipeline pipeline);
```

---
#### `SituationCmdBindComputeBuffer` / `SituationCmdBindComputeTexture`
Binds a storage buffer or storage image to a specific binding point for use in a compute shader.
```c
void SituationCmdBindComputeBuffer(SituationCommandBuffer cmd, int binding, SituationBuffer buffer);
void SituationCmdBindComputeTexture(SituationCommandBuffer cmd, int binding, SituationTexture texture);
```

---
#### `SituationCmdDispatch`
Records a command to execute a compute shader.
```c
void SituationCmdDispatch(SituationCommandBuffer cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
```
**Usage Example:**
```c
// In render loop, before the main render pass
SituationCmdBindComputePipeline(SituationGetMainCommandBuffer(), my_compute_pipeline);
SituationCmdBindComputeTexture(SituationGetMainCommandBuffer(), 0, my_storage_image);
// Dispatch a 16x16 grid of thread groups.
SituationCmdDispatch(SituationGetMainCommandBuffer(), 16, 16, 1);
// A pipeline barrier is needed here to sync with the graphics pass
```

---
#### `SituationCmdPipelineBarrier`
Inserts a barrier into the command buffer, synchronizing memory access between different pipeline stages (e.g., between a compute pass and a graphics pass).
```c
void SituationCmdPipelineBarrier(SituationCommandBuffer cmd);
```

---
#### Virtual Displays

---
#### `SituationVirtualDisplay`
Represents a complete off-screen rendering target, often called a framebuffer object (FBO). It encapsulates not only the GPU resources (like color and depth textures) but also the state required to manage and composite it, such as its resolution, visibility, and blend mode. This is the core struct for implementing post-processing effects, rendering UI at a fixed resolution, or caching complex scenes.
```c
typedef struct SituationVirtualDisplay {
    // Core Properties
    int id;
    bool visible;
    bool is_dirty;
    vec2 resolution;
    vec2 offset;
    float opacity;
    int z_order;

    // Behavior
    double frame_time_multiplier;
    SituationScalingMode scaling_mode;
    SituationBlendMode blend_mode;

    // Backend-Specific GPU Resources
    union {
        struct {
            // OpenGL-specific handles
            uint32_t fbo_id;
            uint32_t texture_id;
            uint32_t rbo_id;
        } gl;
        struct {
            // Vulkan-specific handles
            VkFramebuffer framebuffer;
            VkRenderPass render_pass;
            VkSampler sampler;
            SituationTexture texture; // The texture containing the rendered output
        } vk;
    };
} SituationVirtualDisplay;
```
-   `id`: The unique identifier for the virtual display, used to reference it in API calls.
-   `visible`: If `true`, the display will be automatically drawn to the main window during the compositing phase (`SituationRenderVirtualDisplays`).
-   `is_dirty`: A flag used with time-multiplied displays. If `true`, the display is re-rendered; if `false`, the previous frame's content is reused, saving performance.
-   `resolution`: The internal width and height of the display's render textures in pixels.
-   `offset`: The top-left position (in screen coordinates) where the display will be drawn during compositing.
-   `opacity`: The opacity (0.0 to 1.0) of the display when it is blended onto the target.
-   `z_order`: An integer used to sort visible displays before compositing. Lower numbers are drawn first (further back).
-   `frame_time_multiplier`: Controls the update rate. `1.0` updates every frame, `0.5` every other frame, `0.0` only when marked dirty.
-   `scaling_mode`: An enum (`SituationScalingMode`) that determines how the display's texture is scaled if its resolution differs from its target area (e.g., `SITUATION_SCALING_STRETCH`, `SITUATION_SCALING_LETTERBOX`).
-   `blend_mode`: An enum (`SituationBlendMode`) that defines how the display is blended during compositing (e.g., `SITUATION_BLEND_ALPHA`, `SITUATION_BLEND_ADDITIVE`).
-   `gl`, `vk`: A union containing backend-specific handles to the underlying GPU resources. These are managed internally by the library.
---

#### `SituationCreateVirtualDisplay`
Creates an off-screen render target (framebuffer object).
```c
SituationError SituationCreateVirtualDisplay(Vector2 resolution, double frame_time_mult, int z_order, SituationScalingMode scaling_mode, SituationBlendMode blend_mode, int* out_id);
```

---
#### `SituationDestroyVirtualDisplay`
Destroys a virtual display and its associated resources.
```c
SituationError SituationDestroyVirtualDisplay(int* display_id);
```

---
#### `SituationSetVirtualDisplayVisible`
Sets whether a virtual display should be rendered during the compositing pass.
```c
void SituationSetVirtualDisplayVisible(int display_id, bool visible);
```

---
#### `SituationGetVirtualDisplayTexture`
Gets a handle to the underlying `SituationTexture` for a virtual display's color buffer. This allows you to use the output of one render pass as an input texture for another (e.g., for post-processing).
```c
SituationTexture SituationGetVirtualDisplayTexture(int display_id);
```

---
#### `SituationRenderVirtualDisplays`
Composites all visible virtual displays onto the current render target.
```c
SituationError SituationRenderVirtualDisplays(SituationCommandBuffer cmd);
```
**Usage Example:**
```c
// At init: Create a display for the 3D scene
int scene_vd;
SituationCreateVirtualDisplay((Vector2){640, 360}, 1.0, 0, SITUATION_SCALING_FIT, SITUATION_BLEND_ALPHA, &scene_vd);

// In render loop:
// 1. Render scene to the virtual display
SituationRenderPassInfo scene_pass = { .display_id = scene_vd };
SituationCmdBeginRenderPass(cmd, &scene_pass);
// ... draw 3D models ...
SituationCmdEndRenderPass(cmd);

// 2. Render to the main window
SituationRenderPassInfo final_pass = { .display_id = -1 };
SituationCmdBeginRenderPass(cmd, &final_pass);
// This composites the 3D scene from its virtual display onto the main window
SituationRenderVirtualDisplays(cmd);
// ... draw UI on top ...
SituationCmdEndRenderPass(cmd);
```

---
#### Legacy Shader Uniforms
---
#### `SituationGetShaderLocation`
Gets the location of a uniform variable in a shader by name.
```c
int SituationGetShaderLocation(SituationShader shader, const char* uniformName);
```
**Usage Example:**
```c
// Get the location of the "u_time" uniform in the shader.
int time_uniform_loc = SituationGetShaderLocation(my_shader, "u_time");
// This location can then be used with SituationSetShaderValue.
```

---
#### `SituationGetShaderLocationAttrib`
Gets the location of a vertex attribute in a shader by name.
```c
int SituationGetShaderLocationAttrib(SituationShader shader, const char* attribName);
```
**Usage Example:**
```c
// Get the location of the "a_color" vertex attribute.
int color_attrib_loc = SituationGetShaderLocationAttrib(my_shader, "a_color");
// This is useful for advanced, custom vertex buffer layouts.
```

---
#### `SituationSetShaderValue`
Sets a uniform value in a shader.
```c
void SituationSetShaderValue(SituationShader shader, int locIndex, const void* value, int uniformType);
```
**Usage Example:**
```c
int time_loc = SituationGetShaderLocation(my_shader, "u_time");
float current_time = (float)SituationGetTime();
// Note: This is a legacy way to set uniforms. Using UBOs with SituationCmdBindShaderBuffer is preferred.
SituationSetShaderValue(my_shader, time_loc, &current_time, SIT_UNIFORM_FLOAT);
```

---
#### `SituationSetShaderValueMatrix`
Sets a matrix uniform value in a shader.
```c
void SituationSetShaderValueMatrix(SituationShader shader, int locIndex, mat4 mat);
```
**Usage Example:**
```c
int mvp_loc = SituationGetShaderLocation(my_shader, "u_mvp");
mat4 mvp_matrix = /* ... calculate matrix ... */;
SituationSetShaderValueMatrix(my_shader, mvp_loc, mvp_matrix);
```

---
#### `SituationSetShaderValueTexture`
Sets a texture uniform value in a shader.
```c
void SituationSetShaderValueTexture(SituationShader shader, int locIndex, SituationTexture texture);
```
**Usage Example:**
```c
int albedo_loc = SituationGetShaderLocation(my_shader, "u_albedo_texture");
// This tells the shader to use my_texture for the texture sampler at `albedo_loc`.
SituationSetShaderValueTexture(my_shader, albedo_loc, my_texture);
```

---
#### Miscellaneous

---
#### `SituationLoadImageFromScreen`
Captures the current contents of the main window's backbuffer into a CPU-side image.
```c
SituationError SituationLoadImageFromScreen(SituationImage* out_image);
```
**Usage Example:**
```c
if (SituationIsKeyPressed(SIT_KEY_F12)) {
    SituationImage screenshot = {0};
    if (SituationLoadImageFromScreen(&screenshot) == SITUATION_SUCCESS) {
        SituationExportImage(screenshot, "screenshot.png");
        SituationUnloadImage(screenshot);
    }
}
```

---
#### `SituationCmdBeginRenderToDisplay`
[DEPRECATED] Begins a render pass on a target (-1 for main window), clearing it.
```c
SituationError SituationCmdBeginRenderToDisplay(SituationCommandBuffer cmd, int display_id, ColorRGBA clear_color);
```

---
#### `SituationCmdEndRender`
[DEPRECATED] End the current render pass.
```c
SituationError SituationCmdEndRender(SituationCommandBuffer cmd);
```

---
#### `SituationCmdSetScissor`
Sets the dynamic scissor rectangle to clip rendering.
```c
void SituationCmdSetScissor(SituationCommandBuffer cmd, int x, int y, int width, int height);
```

---
#### `SituationCmdSetPushConstant`
[Core] Set a small block of per-draw uniform data (push constant).
```c
SituationError SituationCmdSetPushConstant(SituationCommandBuffer cmd, uint32_t contract_id, const void* data, size_t size);
```

---
#### `SituationCmdBindDescriptorSet`
[Core] Binds a buffer's descriptor set (UBO/SSBO) to a set index.
```c
SituationError SituationCmdBindDescriptorSet(SituationCommandBuffer cmd, uint32_t set_index, SituationBuffer buffer);
```

---
#### `SituationCmdBindTextureSet`
[Core] Binds a texture's descriptor set (sampler/storage) to a set index.
```c
SituationError SituationCmdBindTextureSet(SituationCommandBuffer cmd, uint32_t set_index, SituationTexture texture);
```

---
#### `SituationCmdBindComputeTexture`
[Core] Binds a texture as a storage image for compute shaders.
```c
SituationError SituationCmdBindComputeTexture(SituationCommandBuffer cmd, uint32_t binding, SituationTexture texture);
```

---
#### `SituationCmdSetVertexAttribute`
[Core] Define the format of a vertex attribute for the active VAO.
```c
SituationError SituationCmdSetVertexAttribute(SituationCommandBuffer cmd, uint32_t location, int size, SituationDataType type, bool normalized, size_t offset);
```

---
#### `SituationCmdDrawIndexed`
[Core] Record an indexed draw call.
```c
SituationError SituationCmdDrawIndexed(SituationCommandBuffer cmd, uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);
```

---
#### `SituationCmdEndRenderPass`
Ends the current render pass.
```c
SituationError SituationCmdEndRenderPass(SituationCommandBuffer cmd);
```

---
#### `SituationLoadShaderFromMemory`
Creates a graphics shader pipeline from in-memory GLSL source.
```c
SituationError SituationLoadShaderFromMemory(const char* vs_code, const char* fs_code, SituationShader* out_shader);
```

---
#### `SituationSetShaderUniform`
[OpenGL] Set a standalone uniform value by name (uses a cache).
```c
SituationError SituationSetShaderUniform(SituationShader shader, const char* uniform_name, const void* data, SituationUniformType type);
```

---
#### `SituationCreateComputePipeline`
Creates a compute pipeline from a shader file.
```c
SituationError SituationCreateComputePipeline(const char* compute_shader_path, SituationComputeLayoutType layout_type, SituationComputePipeline* out_pipeline);
```

---
#### `SituationCreateComputePipelineFromMemory`
Creates a compute pipeline from in-memory GLSL source.
```c
SituationError SituationCreateComputePipelineFromMemory(const char* compute_shader_source, SituationComputeLayoutType layout_type, SituationComputePipeline* out_pipeline);
```

---
#### `SituationDestroyComputePipeline`
Destroys a compute pipeline and free its GPU resources.
```c
void SituationDestroyComputePipeline(SituationComputePipeline* pipeline);
```

---
#### `SituationGetBufferData`
Reads data from a GPU buffer.
```c
SituationError SituationGetBufferData(SituationBuffer buffer, size_t offset, size_t size, void* out_data);
```

---
#### `SituationConfigureVirtualDisplay`
Configures a virtual display's properties.
```c
SituationError SituationConfigureVirtualDisplay(int display_id, vec2 offset, float opacity, int z_order, bool visible, double frame_time_mult, SituationBlendMode blend_mode);
```

---
#### `SituationGetVirtualDisplay`
Gets a pointer to a virtual display's state.
```c
SituationVirtualDisplay* SituationGetVirtualDisplay(int display_id);
```

---
#### `SituationSetVirtualDisplayScalingMode`
Sets the scaling/filtering mode for a virtual display.
```c
SituationError SituationSetVirtualDisplayScalingMode(int display_id, SituationScalingMode scaling_mode);
```

---
#### `SituationSetVirtualDisplayDirty`
Marks a virtual display as needing to be re-rendered.
```c
void SituationSetVirtualDisplayDirty(int display_id, bool is_dirty);
```

---
#### `SituationIsVirtualDisplayDirty`
Checks if a virtual display is marked as dirty.
```c
bool SituationIsVirtualDisplayDirty(int display_id);
```

---
#### `SituationGetLastVDCompositeTimeMS`
Gets the time taken for the last virtual display composite pass.
```c
double SituationGetLastVDCompositeTimeMS(void);
```

---
#### `SituationGetVirtualDisplaySize`
Gets the internal resolution of a virtual display.
```c
void SituationGetVirtualDisplaySize(int display_id, int* width, int* height);
```

---
#### `SituationDrawModel`
Draws all sub-meshes of a model with a single root transformation.
```c
void SituationDrawModel(SituationCommandBuffer cmd, SituationModel model, mat4 transform);
```

---
#### `SituationSaveModelAsGltf`
Exports a model to a human-readable .gltf and a .bin file for debugging.
```c
bool SituationSaveModelAsGltf(SituationModel model, const char* file_path);
```

---
#### `SituationTakeScreenshot`
Takes a screenshot and saves it to a file (PNG or BMP).
```c
SituationError SituationTakeScreenshot(const char *fileName);
```

---
#### `SituationCmdBindUniformBuffer`
[DEPRECATED] [Core] Binds a Uniform Buffer Object (UBO) to a shader binding point.
```c
SituationError SituationCmdBindUniformBuffer(SituationCommandBuffer cmd, uint32_t contract_id, SituationBuffer buffer);
```

---
#### `SituationCmdBindTexture`
[DEPRECATED] [Core] Binds a texture and sampler to a shader binding point.
```c
SituationError SituationCmdBindTexture(SituationCommandBuffer cmd, uint32_t set_index, SituationTexture texture);
```

---
#### `SituationCmdBindComputeBuffer`
[DEPRECATED] Binds a buffer to a compute shader binding point.
```c
SituationError SituationCmdBindComputeBuffer(SituationCommandBuffer cmd, uint32_t binding, SituationBuffer buffer);
```

---
#### `SituationLoadComputeShader`
[DEPRECATED] Loads a compute shader from a file. Use `SituationCreateComputePipeline` instead.
```c
SituationError SituationLoadComputeShader(const char* cs_path, SituationShader* out_shader);
```

---
#### `SituationLoadComputeShaderFromMemory`
[DEPRECATED] Creates a compute shader from memory. Use `SituationCreateComputePipelineFromMemory` instead.
```c
SituationError SituationLoadComputeShaderFromMemory(const char* cs_code, SituationShader* out_shader);
```

---
#### `SituationMemoryBarrier`
[DEPRECATED] Inserts a coarse-grained memory barrier. Use `SituationCmdPipelineBarrier` instead.
```c
void SituationMemoryBarrier(SituationCommandBuffer cmd, uint32_t barrier_bits);
```

</details>
<details>
<summary><h3>Input Module</h3></summary>

**Overview:** The Input module provides a flexible interface for handling user input from keyboards, mice, and gamepads. It supports both state polling (e.g., `SituationIsKeyDown()`) for continuous actions and event-driven callbacks (e.g., `SituationSetKeyCallback()`) for discrete events.

### Key Codes
The library defines a comprehensive set of key codes for use with keyboard input functions.

| Key Code | Value | Description |
|---|---|---|
| `SIT_KEY_SPACE` | 32 | Spacebar |
| `SIT_KEY_APOSTROPHE` | 39 | ' |
| `SIT_KEY_COMMA` | 44 | , |
| `SIT_KEY_MINUS` | 45 | - |
| `SIT_KEY_PERIOD` | 46 | . |
| `SIT_KEY_SLASH` | 47 | / |
| `SIT_KEY_0` | 48 | 0 |
| `SIT_KEY_1` | 49 | 1 |
| `SIT_KEY_2` | 50 | 2 |
| `SIT_KEY_3` | 51 | 3 |
| `SIT_KEY_4` | 52 | 4 |
| `SIT_KEY_5` | 53 | 5 |
| `SIT_KEY_6` | 54 | 6 |
| `SIT_KEY_7` | 55 | 7 |
| `SIT_KEY_8` | 56 | 8 |
| `SIT_KEY_9` | 57 | 9 |
| `SIT_KEY_SEMICOLON` | 59 | ; |
| `SIT_KEY_EQUAL` | 61 | = |
| `SIT_KEY_A` | 65 | A |
| `SIT_KEY_B` | 66 | B |
| `SIT_KEY_C` | 67 | C |
| `SIT_KEY_D` | 68 | D |
| `SIT_KEY_E` | 69 | E |
| `SIT_KEY_F` | 70 | F |
| `SIT_KEY_G` | 71 | G |
| `SIT_KEY_H` | 72 | H |
| `SIT_KEY_I` | 73 | I |
| `SIT_KEY_J` | 74 | J |
| `SIT_KEY_K` | 75 | K |
| `SIT_KEY_L` | 76 | L |
| `SIT_KEY_M` | 77 | M |
| `SIT_KEY_N` | 78 | N |
| `SIT_KEY_O` | 79 | O |
| `SIT_KEY_P` | 80 | P |
| `SIT_KEY_Q` | 81 | Q |
| `SIT_KEY_R` | 82 | R |
| `SIT_KEY_S` | 83 | S |
| `SIT_KEY_T` | 84 | T |
| `SIT_KEY_U` | 85 | U |
| `SIT_KEY_V` | 86 | V |
| `SIT_KEY_W` | 87 | W |
| `SIT_KEY_X` | 88 | X |
| `SIT_KEY_Y` | 89 | Y |
| `SIT_KEY_Z` | 90 | Z |
| `SIT_KEY_LEFT_BRACKET` | 91 | [ |
| `SIT_KEY_BACKSLASH` | 92 | \ |
| `SIT_KEY_RIGHT_BRACKET` | 93 | ] |
| `SIT_KEY_GRAVE_ACCENT` | 96 | ` |
| `SIT_KEY_WORLD_1` | 161 | non-US #1 |
| `SIT_KEY_WORLD_2` | 162 | non-US #2 |
| `SIT_KEY_ESCAPE` | 256 | Escape |
| `SIT_KEY_ENTER` | 257 | Enter |
| `SIT_KEY_TAB` | 258 | Tab |
| `SIT_KEY_BACKSPACE` | 259 | Backspace |
| `SIT_KEY_INSERT` | 260 | Insert |
| `SIT_KEY_DELETE` | 261 | Delete |
| `SIT_KEY_RIGHT` | 262 | Right Arrow |
| `SIT_KEY_LEFT` | 263 | Left Arrow |
| `SIT_KEY_DOWN` | 264 | Down Arrow |
| `SIT_KEY_UP` | 265 | Up Arrow |
| `SIT_KEY_PAGE_UP` | 266 | Page Up |
| `SIT_KEY_PAGE_DOWN` | 267 | Page Down |
| `SIT_KEY_HOME` | 268 | Home |
| `SIT_KEY_END` | 269 | End |
| `SIT_KEY_CAPS_LOCK` | 280 | Caps Lock |
| `SIT_KEY_SCROLL_LOCK` | 281 | Scroll Lock |
| `SIT_KEY_NUM_LOCK` | 282 | Num Lock |
| `SIT_KEY_PRINT_SCREEN` | 283 | Print Screen |
| `SIT_KEY_PAUSE` | 284 | Pause |
| `SIT_KEY_F1` | 290 | F1 |
| `SIT_KEY_F2` | 291 | F2 |
| `SIT_KEY_F3` | 292 | F3 |
| `SIT_KEY_F4` | 293 | F4 |
| `SIT_KEY_F5` | 294 | F5 |
| `SIT_KEY_F6` | 295 | F6 |
| `SIT_KEY_F7` | 296 | F7 |
| `SIT_KEY_F8` | 297 | F8 |
| `SIT_KEY_F9` | 298 | F9 |
| `SIT_KEY_F10` | 299 | F10 |
| `SIT_KEY_F11` | 300 | F11 |
| `SIT_KEY_F12` | 301 | F12 |
| `SIT_KEY_F13` | 302 | F13 |
| `SIT_KEY_F14` | 303 | F14 |
| `SIT_KEY_F15` | 304 | F15 |
| `SIT_KEY_F16` | 305 | F16 |
| `SIT_KEY_F17` | 306 | F17 |
| `SIT_KEY_F18` | 307 | F18 |
| `SIT_KEY_F19` | 308 | F19 |
| `SIT_KEY_F20` | 309 | F20 |
| `SIT_KEY_F21` | 310 | F21 |
| `SIT_KEY_F22` | 311 | F22 |
| `SIT_KEY_F23` | 312 | F23 |
| `SIT_KEY_F24` | 313 | F24 |
| `SIT_KEY_F25` | 314 | F25 |
| `SIT_KEY_KP_0` | 320 | Keypad 0 |
| `SIT_KEY_KP_1` | 321 | Keypad 1 |
| `SIT_KEY_KP_2` | 322 | Keypad 2 |
| `SIT_KEY_KP_3` | 323 | Keypad 3 |
| `SIT_KEY_KP_4` | 324 | Keypad 4 |
| `SIT_KEY_KP_5` | 325 | Keypad 5 |
| `SIT_KEY_KP_6` | 326 | Keypad 6 |
| `SIT_KEY_KP_7` | 327 | Keypad 7 |
| `SIT_KEY_KP_8` | 328 | Keypad 8 |
| `SIT_KEY_KP_9` | 329 | Keypad 9 |
| `SIT_KEY_KP_DECIMAL` | 330 | Keypad . |
| `SIT_KEY_KP_DIVIDE` | 331 | Keypad / |
| `SIT_KEY_KP_MULTIPLY` | 332 | Keypad * |
| `SIT_KEY_KP_SUBTRACT` | 333 | Keypad - |
| `SIT_KEY_KP_ADD` | 334 | Keypad + |
| `SIT_KEY_KP_ENTER` | 335 | Keypad Enter |
| `SIT_KEY_KP_EQUAL` | 336 | Keypad = |
| `SIT_KEY_LEFT_SHIFT` | 340 | Left Shift |
| `SIT_KEY_LEFT_CONTROL` | 341 | Left Control |
| `SIT_KEY_LEFT_ALT` | 342 | Left Alt |
| `SIT_KEY_LEFT_SUPER` | 343 | Left Super/Windows/Command |
| `SIT_KEY_RIGHT_SHIFT` | 344 | Right Shift |
| `SIT_KEY_RIGHT_CONTROL` | 345 | Right Control |
| `SIT_KEY_RIGHT_ALT` | 346 | Right Alt |
| `SIT_KEY_RIGHT_SUPER` | 347 | Right Super/Windows/Command |
| `SIT_KEY_MENU` | 348 | Menu |

### Modifier Bitmasks
These bitmasks are used in callback functions to check for modifier key states.

| Bitmask | Value |
|---|---|
| `SIT_MOD_SHIFT` | 0x0001 |
| `SIT_MOD_CONTROL` | 0x0002 |
| `SIT_MOD_ALT` | 0x0004 |
| `SIT_MOD_SUPER` | 0x0008 |
| `SIT_MOD_CAPS_LOCK` | 0x0010 |
| `SIT_MOD_NUM_LOCK` | 0x0020 |

### Callbacks
The input module allows you to register callback functions to be notified of input events as they happen, as an alternative to polling for state each frame.

#### `SituationKeyCallback`
`typedef void (*SituationKeyCallback)(int key, int scancode, int action, int mods, void* user_data);`
-   `key`: The keyboard key that was pressed or released (e.g., `SIT_KEY_A`).
-   `scancode`: The system-specific scancode of the key.
-   `action`: The key action (`SIT_PRESS`, `SIT_RELEASE`, or `SIT_REPEAT`).
-   `mods`: A bitmask of modifier keys that were held down (`SIT_MOD_SHIFT`, `SIT_MOD_CONTROL`, etc.).
-   `user_data`: The custom user data pointer you provided when setting the callback.

---
#### `SituationMouseButtonCallback`
`typedef void (*SituationMouseButtonCallback)(int button, int action, int mods, void* user_data);`
-   `button`: The mouse button that was pressed or released (e.g., `SIT_MOUSE_BUTTON_LEFT`).
-   `action`: The button action (`SIT_PRESS` or `SIT_RELEASE`).
-   `mods`: A bitmask of modifier keys.
-   `user_data`: Custom user data.

---
#### `SituationCursorPosCallback`
`typedef void (*SituationCursorPosCallback)(double xpos, double ypos, void* user_data);`
-   `xpos`, `ypos`: The new cursor position in screen coordinates.
-   `user_data`: Custom user data.

---
#### `SituationScrollCallback`
`typedef void (*SituationScrollCallback)(double xoffset, double yoffset, void* user_data);`
-   `xoffset`, `yoffset`: The scroll offset.
-   `user_data`: Custom user data.

#### Functions
### Functions

#### Keyboard Input
---
#### `SituationIsKeyDown` / `SituationIsKeyUp`
Checks if a key is currently being held down or is up. This checks the *state* of the key and will return `true` for every frame the key is held. It is ideal for continuous actions like character movement.
```c
bool SituationIsKeyDown(int key);
bool SituationIsKeyUp(int key);
```
**Usage Example:**
```c
// For continuous movement, check the key state every frame.
if (SituationIsKeyDown(SIT_KEY_W)) {
    player.y -= PLAYER_SPEED * SituationGetFrameTime();
}
if (SituationIsKeyDown(SIT_KEY_S)) {
    player.y += PLAYER_SPEED * SituationGetFrameTime();
}
```
---
#### `SituationIsKeyPressed` / `SituationIsKeyReleased`
Checks if a key was just pressed down or released in the current frame. This is a single-trigger *event* and will only return `true` for the exact frame the action occurred. It is ideal for discrete actions like jumping or opening a menu.
```c
bool SituationIsKeyPressed(int key);
bool SituationIsKeyReleased(int key);
```
**Usage Example:**
```c
// For a discrete action like jumping, use the key pressed event.
if (SituationIsKeyPressed(SIT_KEY_SPACE)) {
    player.velocity_y = JUMP_FORCE;
}

// Toggling a menu is another good use case for a single-trigger event.
if (SituationIsKeyPressed(SIT_KEY_ESCAPE)) {
    g_is_menu_open = !g_is_menu_open;
}
```

---
#### `SituationGetKeyPressed`
Gets the last key pressed.
```c
int SituationGetKeyPressed(void);
```
**Usage Example:**
```c
int last_key = SituationGetKeyPressed();
if (last_key > 0) {
    // A key was pressed this frame, you can use it for text input or debug commands.
    printf("Key pressed: %c\n", (char)last_key);
}
```
---
#### `SituationSetKeyCallback`
Sets a callback function for all keyboard key events.
```c
void SituationSetKeyCallback(SituationKeyCallback callback, void* user_data);
```
**Usage Example:**
```c
void my_key_logger(int key, int scancode, int action, int mods, void* user_data) {
    if (action == SIT_PRESS) {
        printf("Key pressed: %d\n", key);
    }
}
SituationSetKeyCallback(my_key_logger, NULL);
```

---
#### `SituationSetMouseButtonCallback` / `SituationSetCursorPosCallback` / `SituationSetScrollCallback`
Sets callback functions for mouse button, cursor movement, and scroll wheel events.
```c
void SituationSetMouseButtonCallback(SituationMouseButtonCallback callback, void* user_data);
void SituationSetCursorPosCallback(SituationCursorPosCallback callback, void* user_data);
void SituationSetScrollCallback(SituationScrollCallback callback, void* user_data);
```

---
#### `SituationSetCharCallback`
Sets a callback for Unicode character input, which is useful for text entry fields.
```c
void SituationSetCharCallback(SituationCharCallback callback, void* user_data);
```

---
#### `SituationSetDropCallback`
Sets a callback that is fired when files are dragged and dropped onto the window.
```c
void SituationSetDropCallback(SituationDropCallback callback, void* user_data);
```

---
#### Clipboard
---
#### `SituationGetClipboardText`
Gets UTF-8 encoded text from the system clipboard. The returned string is heap-allocated and must be freed by the caller using `SituationFreeString`.
```c
SituationError SituationGetClipboardText(const char** out_text);
```
**Usage Example:**
```c
// In an input handler for Ctrl+V
if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) && SituationIsKeyPressed(SIT_KEY_V)) {
    const char* clipboard_text = NULL;
    if (SituationGetClipboardText(&clipboard_text) == SITUATION_SUCCESS) {
        // Paste text into an input field.
        SituationFreeString((char*)clipboard_text);
    }
}
```
---
#### `SituationSetClipboardText`
Sets the system clipboard to the provided UTF-8 encoded text.
```c
SituationError SituationSetClipboardText(const char* text);
```
**Usage Example:**
```c
// In an input handler for Ctrl+C
if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) && SituationIsKeyPressed(SIT_KEY_C)) {
    // Copy selected text to the clipboard.
    SituationSetClipboardText(selected_text);
}
```
---
#### Mouse Input
---
#### `SituationGetMousePosition` / `SituationGetMouseDelta`
Gets the mouse cursor position in screen coordinates, or the mouse movement since the last frame. `SituationGetMouseDelta` is particularly useful for implementing camera controls when the cursor is disabled.
```c
vec2 SituationGetMousePosition(void);
vec2 SituationGetMouseDelta(void);
```
**Usage Example:**
```c
// For a 3D camera, use the mouse delta to control pitch and yaw.
if (IsCursorDisabled()) { // Assuming you have a check for this state
    vec2 mouse_delta = SituationGetMouseDelta();
    camera.yaw   += mouse_delta[0] * MOUSE_SENSITIVITY;
    camera.pitch -= mouse_delta[1] * MOUSE_SENSITIVITY;
}
```
---
#### `SituationIsMouseButtonDown`
Checks if a mouse button is currently being held down. This is a *state* check and is suitable for continuous actions like dragging or aiming.
```c
bool SituationIsMouseButtonDown(int button);
```
**Usage Example:**
```c
// Useful for continuous actions like aiming down sights.
if (SituationIsMouseButtonDown(SIT_MOUSE_BUTTON_RIGHT)) {
    // Zoom in with weapon sights.
}
```

---
#### `SituationIsMouseButtonPressed`
Checks if a mouse button was just pressed down in the current frame. This is a single-trigger *event*, ideal for discrete actions like clicking a button or firing a weapon.
```c
bool SituationIsMouseButtonPressed(int button);
```
**Usage Example:**
```c
// Ideal for discrete actions like firing a weapon.
if (SituationIsMouseButtonPressed(SIT_MOUSE_BUTTON_LEFT)) {
    FireProjectile();
}
```

---
#### `SituationGetMouseButtonPressed`
Gets the mouse button that was pressed this frame.
```c
int SituationGetMouseButtonPressed(void);
```
**Usage Example:**
```c
// Useful for UI interactions where you need to know which button was clicked.
int clicked_button = SituationGetMouseButtonPressed();
if (clicked_button == SIT_MOUSE_BUTTON_LEFT) {
    // Handle left click on a UI element.
} else if (clicked_button == SIT_MOUSE_BUTTON_RIGHT) {
    // Open a context menu.
}
```

---
#### `SituationIsMouseButtonReleased`
Checks if a mouse button was released this frame (a single-trigger event).
```c
bool SituationIsMouseButtonReleased(int button);
```

---
#### `SituationSetMousePosition`
Sets the mouse cursor position within the window.
```c
void SituationSetMousePosition(Vector2 pos);
```
---
#### Gamepad Input
---
#### `SituationIsJoystickPresent`
Checks if a joystick or gamepad is connected at the given joystick ID (0-15).
```c
bool SituationIsJoystickPresent(int jid);
```
**Usage Example:**
```c
// Check for a joystick at the first slot.
if (SituationIsJoystickPresent(0)) {
    printf("A joystick/gamepad is connected at JID 0.\n");
}
```

---
#### `SituationIsGamepad`
Checks if the joystick at the given ID has a standard gamepad mapping, making it compatible with the `SIT_GAMEPAD_*` enums.
```c
bool SituationIsGamepad(int jid);
```
**Usage Example:**
```c
// Before using gamepad-specific functions, check if the device has a standard mapping.
if (SituationIsJoystickPresent(0) && SituationIsGamepad(0)) {
    // Now it's safe to use functions like SituationIsGamepadButtonPressed.
}
```

---
#### `SituationGetJoystickName`
Gets the implementation-defined name of a joystick (e.g., "Xbox Controller").
```c
const char* SituationGetJoystickName(int jid);
```
**Usage Example:**
```c
#define GAMEPAD_ID 0
if (SituationIsJoystickPresent(GAMEPAD_ID) && SituationIsGamepad(GAMEPAD_ID)) {
    printf("Gamepad '%s' is ready.\n", SituationGetJoystickName(GAMEPAD_ID));
}
```
---
#### `SituationIsGamepadButtonDown` / `SituationIsGamepadButtonPressed`
Checks if a gamepad button is held down (state) or was just pressed (event).
```c
bool SituationIsGamepadButtonDown(int jid, int button);
bool SituationIsGamepadButtonPressed(int jid, int button);
```
**Usage Example:**
```c
if (SituationIsGamepadButtonPressed(GAMEPAD_ID, SIT_GAMEPAD_BUTTON_A)) {
    // Jump
}
```

---
#### `SituationGetGamepadButtonPressed`
Gets the last gamepad button pressed.
```c
int SituationGetGamepadButtonPressed(void);
```
**Usage Example:**
```c
int last_button = SituationGetGamepadButtonPressed();
if (last_button != SIT_GAMEPAD_BUTTON_UNKNOWN) {
    // A button was pressed this frame
    printf("Gamepad button pressed: %d\n", last_button);
}
```
---
#### `SituationGetGamepadAxisValue`
Gets the value of a gamepad axis, between -1.0 and 1.0, with deadzone applied.
```c
float SituationGetGamepadAxisValue(int jid, int axis);
```
**Usage Example:**
```c
float move_x = SituationGetGamepadAxisValue(GAMEPAD_ID, SIT_GAMEPAD_AXIS_LEFT_X);
float move_y = SituationGetGamepadAxisValue(GAMEPAD_ID, SIT_GAMEPAD_AXIS_LEFT_Y);
player.x += move_x * PLAYER_SPEED * SituationGetFrameTime();
player.y += move_y * PLAYER_SPEED * SituationGetFrameTime();
```

---
#### `SituationIsKeyUp`
Checks if a key is currently up (a state).
```c
bool SituationIsKeyUp(int key);
```

---
#### `SituationIsKeyReleased`
Checks if a key was released this frame (an event).
```c
bool SituationIsKeyReleased(int key);
```

---
#### `SituationPeekKeyPressed`
Peeks at the next key in the press queue without consuming it.
```c
int SituationPeekKeyPressed(void);
```

---
#### `SituationGetCharPressed`
Gets the next character from the text input queue.
```c
unsigned int SituationGetCharPressed(void);
```

---
#### `SituationIsLockKeyPressed`
Checks if a lock key (Caps, Num) is currently active.
```c
bool SituationIsLockKeyPressed(int lock_key_mod);
```

---
#### `SituationIsScrollLockOn`
Checks if Scroll Lock is currently toggled on.
```c
bool SituationIsScrollLockOn(void);
```

---
#### `SituationIsModifierPressed`
Checks if a modifier key (Shift, Ctrl, Alt) is pressed.
```c
bool SituationIsModifierPressed(int modifier);
```

---
#### `SituationGetMouseDelta`
Gets the mouse movement since the last frame.
```c
vec2 SituationGetMouseDelta(void);
```

---
#### `SituationGetMouseWheelMove`
Gets vertical mouse wheel movement.
```c
float SituationGetMouseWheelMove(void);
```

---
#### `SituationGetMouseWheelMoveV`
Gets vertical and horizontal mouse wheel movement.
```c
Vector2 SituationGetMouseWheelMoveV(void);
```

---
#### `SituationIsMouseButtonPressed`
Checks if a mouse button was pressed down this frame (an event).
```c
bool SituationIsMouseButtonPressed(int button);
```

---
#### `SituationIsMouseButtonReleased`
Checks if a mouse button was released this frame.
```c
bool SituationIsMouseButtonReleased(int button);
```

---
#### `SituationSetMouseOffset`
Sets a software offset for the mouse position.
```c
void SituationSetMouseOffset(vec2 offset);
```

---
#### `SituationSetMouseScale`
Sets a software scale for the mouse position and delta.
```c
void SituationSetMouseScale(vec2 scale);
```

---
#### `SituationSetCursorPosCallback`
Sets a callback for mouse movement events.
```c
void SituationSetCursorPosCallback(SituationCursorPosCallback callback, void* user_data);
```

---
#### `SituationSetScrollCallback`
Sets a callback for mouse scroll events.
```c
void SituationSetScrollCallback(SituationScrollCallback callback, void* user_data);
```

---
#### `SituationSetJoystickCallback`
Sets a callback for joystick connection events.
```c
void SituationSetJoystickCallback(SituationJoystickCallback callback, void* user_data);
```

---
#### `SituationSetGamepadMappings`
Loads a new set of gamepad mappings from a string.
```c
int SituationSetGamepadMappings(const char *mappings);
```

---
#### `SituationIsGamepadButtonPressed`
Checks if a gamepad button was pressed down this frame (an event).
```c
bool SituationIsGamepadButtonPressed(int jid, int button);
```

---
#### `SituationIsGamepadButtonReleased`
Checks if a gamepad button was released this frame (an event).
```c
bool SituationIsGamepadButtonReleased(int jid, int button);
```

---
#### `SituationGetGamepadAxisCount`
Gets the number of axes for a gamepad.
```c
int SituationGetGamepadAxisCount(int jid);
```

---
#### `SituationSetGamepadVibration`
Sets gamepad vibration/rumble (Windows only).
```c
void SituationSetGamepadVibration(int jid, float left_motor, float right_motor);
```

---

#### `SituationSetMousePosition`
Sets the mouse cursor position within the window.
```c
SITAPI void SituationSetMousePosition(Vector2 pos);
```
**Usage Example:**
```c
// Center the mouse cursor in a 1280x720 window
Vector2 center = { .x = 1280 / 2.0f, .y = 720 / 2.0f };
SituationSetMousePosition(center);
```
</details>
<details>
<summary><h3>Audio Module</h3></summary>

**Overview:** The Audio module offers a full-featured audio engine capable of loading sounds (`SituationLoadSoundFromFile`) for low-latency playback and streaming longer tracks (`SituationLoadSoundFromStream`) to conserve memory. It supports device management, playback control (volume, pan, pitch), a built-in effects chain (filters, reverb), and custom real-time audio processors.

### Structs and Enums

#### `SituationAudioDeviceInfo`
Contains information about a single audio playback device available on the system.
```c
typedef struct SituationAudioDeviceInfo {
    int internal_id;
    char name[SITUATION_MAX_DEVICE_NAME_LEN];
    bool is_default;
    int min_channels, max_channels;
    int min_sample_rate, max_sample_rate;
} SituationAudioDeviceInfo;
```
-   `internal_id`: The ID used to select this device with `SituationSetAudioDevice()`.
-   `name`: The human-readable name of the device.
-   `is_default`: `true` if this is the operating system's default audio device.
-   `min_channels`, `max_channels`: The minimum and maximum number of channels supported by the device.
-   `min_sample_rate`, `max_sample_rate`: The minimum and maximum sample rates supported by the device.

---
#### `SituationAudioFormat`
Describes the format of audio data, used when initializing the audio device or loading sounds from custom streams.
```c
typedef struct SituationAudioFormat {
    int channels;
    int sample_rate;
    int bit_depth;
} SituationAudioFormat;
```
-   `channels`: Number of audio channels (e.g., 1 for mono, 2 for stereo).
-   `sample_rate`: Number of samples per second (e.g., 44100 Hz).
-   `bit_depth`: Number of bits per sample (e.g., 16-bit).

---
#### `SituationSound`
An opaque handle to a sound resource. This handle encapsulates all the necessary internal state for a sound, whether it's fully loaded into memory or streamed from a source. It is initialized by `SituationLoadSoundFromFile()` or `SituationLoadSoundFromStream()` and must be cleaned up with `SituationUnloadSound()`.
```c
typedef struct SituationSound {
    uint64_t id; // Internal unique ID
    // Internal data is not exposed to the user
} SituationSound;
```
- **Creation:** `SituationLoadSoundFromFile()`, `SituationLoadSoundFromStream()`
- **Usage:** `SituationPlayLoadedSound()`, `SituationSetSoundVolume()`
- **Destruction:** `SituationUnloadSound()`

---
#### `SituationFilterType`
Specifies the type of filter to apply to a sound.
| Type | Description |
|---|---|
| `SIT_FILTER_NONE` | No filter is applied. |
| `SIT_FILTER_LOW_PASS` | Allows low frequencies to pass through. |
| `SIT_FILTER_HIGH_PASS` | Allows high frequencies to pass through. |

#### Functions
### Functions

#### Audio Device Management
---
#### `SituationIsAudioDeviceReady`
Checks if the audio device has been successfully initialized via `SituationInit`.
```c
bool SituationIsAudioDeviceReady(void);
```

---
#### `SituationGetAudioDevices`
Enumerates all available audio playback devices on the system. This is useful for providing the user with a choice of audio output devices.
```c
SituationAudioDeviceInfo* SituationGetAudioDevices(int* count);
```
**Usage Example:**
```c
int device_count;
SituationAudioDeviceInfo* devices = SituationGetAudioDevices(&device_count);
printf("Available Audio Devices:\n");
for (int i = 0; i < device_count; i++) {
    printf("- %s %s\n", devices[i].name, devices[i].is_default ? "(Default)" : "");
}
// Note: The returned array's memory is managed by the library and should not be freed.
```

---
#### `SituationSetAudioDevice`
Sets the active audio playback device by its ID. This should be called before loading any sounds.
```c
SituationError SituationSetAudioDevice(int device_id);
```

---
#### `SituationSetAudioMasterVolume`
Sets the master volume for the entire audio device, from `0.0` (silent) to `1.0` (full volume).
```c
SituationError SituationSetAudioMasterVolume(float volume);
```

---
#### `SituationSuspendAudioContext` / `SituationResumeAudioContext`
Suspends or resumes the entire audio context, stopping or restarting all sounds.
```c
SituationError SituationSuspendAudioContext(void);
SituationError SituationResumeAudioContext(void);
```
---
#### Sound Loading and Management
---
#### `SituationLoadSoundFromFile` / `SituationUnloadSound`
Loads a sound from a file (WAV, MP3, OGG, FLAC). The `mode` parameter determines whether to decode fully to RAM (`SITUATION_AUDIO_LOAD_FULL`, `AUTO`) or stream from disk (`SITUATION_AUDIO_LOAD_STREAM`). `SituationUnloadSound` frees the sound's memory.
```c
SituationError SituationLoadSoundFromFile(const char* file_path, SituationAudioLoadMode mode, bool looping, SituationSound* out_sound);
void SituationUnloadSound(SituationSound* sound);
```
**Usage Example:**
```c
// At init:
SituationSound jump_sound;
SituationLoadSoundFromFile("sounds/jump.wav", SITUATION_AUDIO_LOAD_AUTO, false, &jump_sound);

// During gameplay:
if (SituationIsKeyPressed(SIT_KEY_SPACE)) {
    SituationPlayLoadedSound(&jump_sound);
}

// At shutdown:
SituationUnloadSound(&jump_sound);
```
---
#### `SituationLoadSoundFromStream`
Initializes a sound for playback by streaming it from a custom data source. This is highly memory-efficient and is the preferred method for long music tracks. You provide callbacks to read and seek in your custom data stream.
```c
SituationError SituationLoadSoundFromStream(SituationStreamReadCallback on_read, SituationStreamSeekCallback on_seek, void* user_data, const SituationAudioFormat* format, bool looping, SituationSound* out_sound);
```

---
#### `SituationLoadSoundFromMemory`
Loads a sound from a data buffer in memory.
```c
SituationError SituationLoadSoundFromMemory(const char* file_type, const unsigned char* data, int data_size, bool looping, SituationSound* out_sound);
```
---
#### Playback Control
---
#### `SituationPlayLoadedSound` / `SituationStopLoadedSound`
Begins or stops playback of a specific loaded sound.
```c
SituationError SituationPlayLoadedSound(SituationSound* sound);
SituationError SituationStopLoadedSound(SituationSound* sound);
```
**Usage Example:**
```c
if (SituationIsKeyPressed(SIT_KEY_SPACE)) {
    SituationPlayLoadedSound(&jump_sound);
}
```

---
#### `SituationIsSoundPlaying`
Checks if a specific sound is currently playing.
```c
bool SituationIsSoundPlaying(SituationSound* sound);
```

---
#### `SituationSetSoundVolume`
Sets the volume for a specific, individual sound (`0.0` to `1.0`).
```c
SituationError SituationSetSoundVolume(SituationSound* sound, float volume);
```

---
#### `SituationSetSoundPan`
Sets the stereo panning for a sound (`-1.0` is full left, `0.0` is center, `1.0` is full right).
```c
SituationError SituationSetSoundPan(SituationSound* sound, float pan);
```

---
#### `SituationSetSoundPitch`
Sets the playback pitch for a sound by resampling (`1.0` is normal pitch, `0.5` is one octave lower, `2.0` is one octave higher).
```c
SituationError SituationSetSoundPitch(SituationSound* sound, float pitch);
```
**Usage Example:**
```c
// Make the sound effect's pitch slightly random
float random_pitch = 1.0f + ((rand() % 200) - 100) / 1000.0f; // Range 0.9 to 1.1
SituationSetSoundPitch(&jump_sound, random_pitch);
SituationPlayLoadedSound(&jump_sound);
```

---
#### Querying Sound State
---
#### `SituationIsSoundLooping`
Checks if a sound is set to loop.
```c
bool SituationIsSoundLooping(SituationSound* sound);
```
**Usage Example:**
```c
if (SituationIsSoundLooping(&music)) {
    printf("The music track is set to loop.\n");
}
```

---
#### `SituationGetSoundLength`
Gets the total length of a sound in seconds.
```c
double SituationGetSoundLength(SituationSound* sound);
```
**Usage Example:**
```c
double length = SituationGetSoundLength(&music);
printf("Music track length: %.2f seconds\n", length);
```

---
#### `SituationGetSoundCursor`
Gets the current playback position of a sound in seconds.
```c
double SituationGetSoundCursor(SituationSound* sound);
```
**Usage Example:**
```c
double position = SituationGetSoundCursor(&music);
printf("Music is currently at %.2f seconds\n", position);
```

---
#### `SituationSetSoundCursor`
Sets the current playback position of a sound in seconds.
```c
void SituationSetSoundCursor(SituationSound* sound, double seconds);
```
**Usage Example:**
```c
// Skip 30 seconds into the music track
SituationSetSoundCursor(&music, 30.0);
```
---
#### Effects and Custom Processing
---
#### `SituationSetSoundFilter`
Applies a low-pass or high-pass filter to a sound's effects chain.
```c
SituationError SituationSetSoundFilter(SituationSound* sound, SituationFilterType type, float cutoff_hz, float q_factor);
```
**Usage Example:**
```c
// To simulate sound coming from another room, apply a low-pass filter.
SituationSetSoundFilter(&music, SIT_FILTER_LOW_PASS, 800.0f, 1.0f); // Cut off frequencies above 800 Hz
```

---
#### `SituationSetSoundReverb`
Applies a reverb effect to a sound.
```c
SituationError SituationSetSoundReverb(SituationSound* sound, bool enabled, float room_size, float damping, float wet_mix, float dry_mix);
```

---
#### `SituationAttachAudioProcessor`
Attaches a custom DSP processor to a sound's effect chain for real-time processing, like visualization or custom effects.
```c
SituationError SituationAttachAudioProcessor(SituationSound* sound, SituationAudioProcessorCallback processor, void* userData);
```

---
#### `SituationDetachAudioProcessor`
Detaches a custom DSP processor from a sound.
```c
SituationError SituationDetachAudioProcessor(SituationSound* sound, SituationAudioProcessorCallback processor);
```

---
#### `SituationGetAudioPlaybackSampleRate`
Gets the sample rate of the current audio device.
```c
int SituationGetAudioPlaybackSampleRate(void);
```

---
#### `SituationSetAudioPlaybackSampleRate`
Re-initializes the audio device with a new sample rate.
```c
SituationError SituationSetAudioPlaybackSampleRate(int sample_rate);
```

---
#### `SituationGetAudioMasterVolume`
Gets the master volume for the audio device.
```c
float SituationGetAudioMasterVolume(void);
```

---
#### `SituationIsAudioDevicePlaying`
Checks if the audio device is currently playing.
```c
bool SituationIsAudioDevicePlaying(void);
```

---
#### `SituationPauseAudioDevice`
Pauses audio playback on the device.
```c
SituationError SituationPauseAudioDevice(void);
```

---
#### `SituationResumeAudioDevice`
Resumes audio playback on the device.
```c
SituationError SituationResumeAudioDevice(void);
```

---
#### `SituationStopLoadedSound`
Stops a specific sound from playing.
```c
SituationError SituationStopLoadedSound(SituationSound* sound);
```

---
#### `SituationStopAllLoadedSounds`
Stops all currently playing sounds.
```c
SituationError SituationStopAllLoadedSounds(void);
```

---
#### `SituationSoundCopy`
Creates a new sound by copying the raw PCM data from a source.
```c
SituationError SituationSoundCopy(const SituationSound* source, SituationSound* out_destination);
```

---
#### `SituationSoundCrop`
Crops a sound's PCM data in-place to a new range.
```c
SituationError SituationSoundCrop(SituationSound* sound, uint64_t initFrame, uint64_t finalFrame);
```

---
#### `SituationSoundExportAsWav`
Exports the sound's raw PCM data to a WAV file.
```c
bool SituationSoundExportAsWav(const SituationSound* sound, const char* fileName);
```

---
#### `SituationGetSoundVolume`
Gets the volume of a specific sound.
```c
float SituationGetSoundVolume(SituationSound* sound);
```

---
#### `SituationGetSoundPan`
Gets the stereo pan of a sound.
```c
float SituationGetSoundPan(SituationSound* sound);
```

---
#### `SituationGetSoundPitch`
Gets the pitch of a sound.
```c
float SituationGetSoundPitch(SituationSound* sound);
```

---
#### `SituationSetSoundEcho`
Applies an echo effect to a sound.
```c
SituationError SituationSetSoundEcho(SituationSound* sound, bool enabled, float delay_sec, float feedback, float wet_mix);
```

---
#### `SituationUnloadSound`
Unloads a sound and frees its resources.
```c
void SituationUnloadSound(SituationSound* sound);
```

---
#### Audio Handle API
These functions operate on the new `SituationSoundHandle` system, which simplifies audio management by using opaque handles instead of structs.

---
#### `SituationLoadAudio`
Loads an audio file and returns a handle.
```c
SituationSoundHandle SituationLoadAudio(const char* file_path, SituationAudioLoadMode mode, bool looping);
```

---
#### `SituationPlayAudio`
Plays audio using its handle.
```c
SituationError SituationPlayAudio(SituationSoundHandle handle);
```

---
#### `SituationUnloadAudio`
Unloads audio using its handle.
```c
void SituationUnloadAudio(SituationSoundHandle handle);
```

---
#### `SituationSetAudioVolume`
Sets the volume for an audio handle.
```c
SituationError SituationSetAudioVolume(SituationSoundHandle handle, float volume);
```

---
#### `SituationSetAudioPan`
Sets the pan for an audio handle.
```c
SituationError SituationSetAudioPan(SituationSoundHandle handle, float pan);
```

---
#### `SituationSetAudioPitch`
Sets the pitch for an audio handle.
```c
SituationError SituationSetAudioPitch(SituationSoundHandle handle, float pitch);
```

---

#### `SituationSetAudioDevice`
Sets the active audio playback device by its ID and format.
```c
SITAPI SituationError SituationSetAudioDevice(int situation_internal_id, const SituationAudioFormat* format);
```
**Usage Example:**
```c
int device_count;
SituationAudioDeviceInfo* devices = SituationGetAudioDevices(&device_count);
if (device_count > 0) {
    SituationAudioFormat format = { .channels = 2, .sample_rate = 44100, .bit_depth = 16 };
    SituationSetAudioDevice(devices[0].internal_id, &format);
}
```

---

#### `SituationSetSoundReverb`
Applies a reverb effect to a sound.
```c
SITAPI SituationError SituationSetSoundReverb(SituationSound* sound, bool enabled, float room_size, float damping, float wet_mix, float dry_mix);
```
**Usage Example:**
```c
SituationSound my_sound;
if (SituationLoadSoundFromFile("sounds/footstep.wav", SITUATION_AUDIO_LOAD_AUTO, false, &my_sound) == SITUATION_SUCCESS) {
    // Apply a reverb to simulate a large room
    SituationSetSoundReverb(&my_sound, true, 0.8f, 0.5f, 0.6f, 0.4f);
    SituationPlayLoadedSound(&my_sound);
}
```

---
#### Audio Capture
---
#### `SituationStartAudioCapture`
Initializes and starts capturing audio from the default microphone or recording device. The captured audio data is delivered via a callback that you provide.
```c
SITAPI SituationError SituationStartAudioCapture(SituationAudioCaptureCallback on_capture, void* user_data);
```
-   `on_capture`: A pointer to a function that will be called whenever a new buffer of audio data is available. The callback receives the raw audio buffer, the number of frames, and the user data pointer.
-   `user_data`: A custom pointer that will be passed to your `on_capture` callback.
**Usage Example:**
```c
// Define a callback to process the incoming audio data.
void MyAudioCaptureCallback(const float* frames, int frame_count, void* user_data) {
    // 'frames' is an interleaved buffer of 32-bit float samples.
    // For stereo, it would be [L, R, L, R, ...].
    printf("Captured %d audio frames.\n", frame_count);
    // You could write this data to a file, perform FFT, or visualize it.
}

// In your initialization code:
if (SituationStartAudioCapture(MyAudioCaptureCallback, NULL) != SIT_SUCCESS) {
    fprintf(stderr, "Failed to start audio capture: %s\n", SituationGetLastErrorMsg());
}
```

---
#### `SituationStopAudioCapture`
Stops the audio capture stream and releases the microphone device.
```c
SITAPI SituationError SituationStopAudioCapture(void);
```
**Usage Example:**
```c
// When the user clicks a "Stop Recording" button.
SituationStopAudioCapture();
printf("Audio capture stopped.\n");
```

---
#### `SituationIsAudioCapture`
Checks if the audio capture stream is currently active.
```c
SITAPI bool SituationIsAudioCapture(void);
```
**Usage Example:**
```c
if (SituationIsAudioCapture()) {
    // Update UI to show a "Recording" indicator.
}
```

</details>
<details>
<summary><h3>Filesystem Module</h3></summary>

**Overview:** The Filesystem module provides a robust, cross-platform, and UTF-8 aware API for interacting with the host's file system. It includes functions for checking file/directory existence, reading/writing files, and path manipulation. Use helpers like `SituationGetBasePath()` (for assets) and `SituationGetAppSavePath()` (for user data) for maximum portability.

### Functions

#### Path Management & Special Directories
---
#### `SituationGetBasePath`
Gets the full, absolute path to the directory containing the application's executable. This is the recommended way to locate asset files, as it is not affected by the working directory. The returned string must be freed with `SituationFreeString`.
```c
char* SituationGetBasePath(void);
```
---
#### `SituationGetAppSavePath`
Gets a platform-appropriate, user-specific directory for saving configuration files, save games, and other user data. For example, on Windows, this typically resolves to `%APPDATA%/YourAppName`. The returned string must be freed with `SituationFreeString`.
```c
char* SituationGetAppSavePath(const char* app_name);
```
**Usage Example:**
```c
char* save_path = SituationGetAppSavePath("MyCoolGame");
if (save_path) {
    char* config_file_path = SituationJoinPath(save_path, "config.ini");
    printf("Saving config to: %s\n", config_file_path);
    // ... save file ...
    SituationFreeString(config_file_path);
    SituationFreeString(save_path);
}
```
---
#### `SituationJoinPath`
Joins two path components with the correct OS separator.
```c
char* SituationJoinPath(const char* base_path, const char* file_or_dir_name);
```
**Usage Example:**
```c
char* base = SituationGetBasePath();
char* texture_path = SituationJoinPath(base, "textures/player.png");
// ... use texture_path ...
SituationFreeString(base);
SituationFreeString(texture_path);
```
---
#### File and Directory Queries
---
#### `SituationGetFileName`
Extracts the file name component from a path, including the extension.
```c
const char* SituationGetFileName(const char* file_path);
```
**Usage Example:**
```c
const char* path = "C:/assets/textures/player_avatar.png";
printf("File name: %s\n", SituationGetFileName(path)); // -> "player_avatar.png"
```

---
#### `SituationGetFileExtension`
Extracts the extension from a path, including the leading dot (`.`).
```c
const char* SituationGetFileExtension(const char* file_path);
```
**Usage Example:**
```c
const char* path = "C:/assets/textures/player_avatar.png";
printf("Extension: %s\n", SituationGetFileExtension(path)); // -> ".png"
```

---
#### `SituationGetFileNameWithoutExt`
Extracts the file name from a path, excluding the extension. The caller is responsible for freeing the returned string with `SituationFreeString`.
```c
char* SituationGetFileNameWithoutExt(const char* file_path);
```
**Usage Example:**
```c
const char* path = "C:/assets/textures/player_avatar.png";
char* filename = SituationGetFileNameWithoutExt(path);
if (filename) {
    printf("File name without ext: %s\n", filename); // -> "player_avatar"
    SituationFreeString(filename);
}
```

---
#### `SituationIsFileExtension`
Checks if a file path has a specific extension (case-insensitive).
```c
bool SituationIsFileExtension(const char* file_path, const char* ext);
```
**Usage Example:**
```c
if (SituationIsFileExtension("my_archive.ZIP", ".zip")) {
    printf("This is a zip archive.\n");
}
```

---
#### `SituationFileExists`
Checks if a file exists at the given path and is accessible.
```c
bool SituationFileExists(const char* file_path);
```
**Usage Example:**
```c
// Before attempting to load a file, check if it exists.
if (SituationFileExists("settings.ini")) {
    LoadSettingsFromFile("settings.ini");
} else {
    CreateDefaultSettings();
}
```

---
#### `SituationDirectoryExists`
Checks if a directory exists at the given path and is accessible.
```c
bool SituationDirectoryExists(const char* dir_path);
```
**Usage Example:**
```c
// Check if a directory for save games exists before trying to list its files.
if (SituationDirectoryExists("save_games")) {
    // ... proceed to load a save file ...
}
```

---
#### `SituationIsPathFile`
Checks if a given path points to a file.
```c
bool SituationIsPathFile(const char* path);
```
**Usage Example:**
```c
// Differentiate between a file and a directory.
const char* path = "assets/player.png";
if (SituationIsPathFile(path)) {
    printf("%s is a file.\n", path);
} else if (SituationDirectoryExists(path)) {
    printf("%s is a directory.\n", path);
}
```
---
#### `SituationGetFileModTime`
Gets the last modification time of a file, returned as a Unix timestamp. Returns `-1` if the file does not exist. This is useful for hot-reloading assets.
```c
long SituationGetFileModTime(const char* file_path);
```
**Usage Example:**
```c
// In the update loop, check if a shader file has changed.
long mod_time = SituationGetFileModTime("shaders/main.frag");
if (mod_time > g_last_shader_load_time) {
    // Reload the shader.
    SituationUnloadShader(&g_main_shader);
    g_main_shader = SituationLoadShader("shaders/main.vert", "shaders/main.frag");
    g_last_shader_load_time = mod_time;
}
```
---
#### File I/O
---
#### `SituationLoadFileText`
Loads a text file as a null-terminated string. The caller is responsible for freeing the returned string with `SituationFreeString`.
```c
char* SituationLoadFileText(const char* file_path);
```
**Usage Example:**
```c
char* shader_code = SituationLoadFileText("shaders/main.glsl");
if (shader_code) {
    // ... use the shader code ...
    SituationFreeString(shader_code);
}
```

---
#### `SituationSaveFileText`
Saves a null-terminated string to a text file.
```c
SituationError SituationSaveFileText(const char* file_path, const char* text);
```
**Usage Example:**
```c
const char* settings = "[Graphics]\nwidth=1920\nheight=1080";
if (SituationSaveFileText("settings.ini", settings) == SITUATION_SUCCESS) {
    printf("Settings saved.\n");
}
```
---
#### `SituationLoadFileData`
Loads an entire file into a memory buffer. The caller is responsible for freeing the returned buffer with `SituationFreeString`.
```c
SituationError SituationLoadFileData(const char* file_path, unsigned int* out_bytes_read, unsigned char** out_data);
```
**Usage Example:**
```c
unsigned int data_size = 0;
unsigned char* file_data = NULL;
if (SituationLoadFileData("assets/level.dat", &data_size, &file_data) == SITUATION_SUCCESS) {
    // Process the loaded binary data.
    SituationFreeString((char*)file_data); // Cast and free the memory.
}
```

---
#### `SituationSaveFileData`
Saves a buffer of raw data to a file.
```c
SituationError SituationSaveFileData(const char* file_path, const void* data, unsigned int bytes_to_write);
```
**Usage Example:**
```c
// Assume 'player_data' is a struct containing game state.
PlayerState player_data = { .health = 100, .score = 5000 };
// Save the player state to a binary file.
if (SituationSaveFileData("save.dat", &player_data, sizeof(PlayerState)) == SITUATION_SUCCESS) {
    printf("Game saved successfully.\n");
}
```
---
#### `SituationFreeFileData`
Frees the memory for a data buffer returned by `SituationLoadFileData`.
```c
void SituationFreeFileData(unsigned char* data);
```
**Usage Example:**
```c
unsigned int data_size;
unsigned char* file_data = SituationLoadFileData("assets/level.dat", &data_size);
if (file_data) {
    // ... process data ...
    SituationFreeFileData(file_data); // Free the memory.
}
```
---
#### Directory Operations
---
#### `SituationCreateDirectory`
Creates a directory, optionally creating all parent directories in the path.
```c
SituationError SituationCreateDirectory(const char* dir_path, bool create_parents);
```
**Usage Example:**
```c
// Creates "assets", "assets/models", and "assets/models/player" if they don't exist.
SituationCreateDirectory("assets/models/player", true);
```
---
#### `SituationListDirectoryFiles`
Lists files and subdirectories in a path. The returned list must be freed with `SituationFreeDirectoryFileList`.
```c
SituationError SituationListDirectoryFiles(const char* dir_path, char*** out_files, int* out_count);
```
**Usage Example:**
```c
int file_count = 0;
char** files = NULL;
if (SituationListDirectoryFiles("assets", &files, &file_count) == SITUATION_SUCCESS) {
    for (int i = 0; i < file_count; ++i) {
        printf("Found file: %s\n", files[i]);
    }
    SituationFreeDirectoryFileList(files, file_count);
}
```

---
#### `SituationChangeDirectory` / `SituationGetWorkingDirectory`
Changes the application's current working directory or gets the current one.
```c
bool SituationChangeDirectory(const char* dir_path);
char* SituationGetWorkingDirectory(void);
```
**Usage Example:**
```c
char* initial_dir = SituationGetWorkingDirectory();
printf("Started in: %s\n", initial_dir);
if (SituationChangeDirectory("assets")) {
    printf("Changed dir to 'assets'\n");
}
SituationChangeDirectory(initial_dir); // Change back
SituationFreeString(initial_dir);
```

---
#### `SituationGetDirectoryPath`
Gets the directory path for a file path.
```c
char* SituationGetDirectoryPath(const char* file_path);
```
**Usage Example:**
```c
char* dir_path = SituationGetDirectoryPath("C:/assets/models/player.gltf");
// dir_path is now "C:/assets/models"
printf("The model's directory is: %s\n", dir_path);
SituationFreeString(dir_path);
```

---
#### `SituationCopyFile`
Copies a file.
```c
SituationError SituationCopyFile(const char* source_path, const char* dest_path);
```

---
#### `SituationDeleteFile`
Deletes a file.
```c
SituationError SituationDeleteFile(const char* file_path);
```

---
#### `SituationMoveFile`
Moves/renames a file, even across drives on Windows.
```c
bool SituationMoveFile(const char* old_path, const char* new_path);
```

---
#### `SituationRenameFile`
Alias for `SituationMoveFile`.
```c
SituationError SituationRenameFile(const char* old_path, const char* new_path);
```

---
#### `SituationDeleteDirectory`
Deletes a directory, optionally deleting all its contents.
```c
SituationError SituationDeleteDirectory(const char* dir_path, bool recursive);
```
**Usage Example:**
```c
// Delete the "temp_files" directory and everything inside it.
if (SituationDirectoryExists("temp_files")) {
    SituationDeleteDirectory("temp_files", true);
}
```

---

#### `SituationFreeDirectoryFileList`
Frees the memory for the list of file paths returned by `SituationListDirectoryFiles`.
```c
SITAPI void SituationFreeDirectoryFileList(char** files, int count);
```
**Usage Example:**
```c
int file_count = 0;
char** files = SituationListDirectoryFiles("assets", &file_count);
for (int i = 0; i < file_count; ++i) {
    printf("Found file: %s\n", files[i]);
}
SituationFreeDirectoryFileList(files, file_count);
```
</details>
<details>
<summary><h3>Threading Module</h3></summary>

**Overview:** The Threading module provides a high-performance **Generational Task System** for executing tasks asynchronously. It features a lock-minimized dual-queue architecture (High/Low priority) to prevent asset loading from stalling critical gameplay physics. The system uses O(1) generational IDs to prevent ABA problems and "Small Object Optimization" to avoid memory allocation for most jobs.

### Structs and Enums

#### `SituationJobFlags`
Flags to control job submission behavior, including priority and backpressure handling.
```c
typedef enum {
    SIT_SUBMIT_DEFAULT       = 0,       // Low Priority, Return 0 if full
    SIT_SUBMIT_HIGH_PRIORITY = 1 << 0,  // Use High Priority Queue (Physics, Audio)
    SIT_SUBMIT_BLOCK_IF_FULL = 1 << 1,  // Spin/Sleep until a slot opens
    SIT_SUBMIT_RUN_IF_FULL   = 1 << 2,  // Execute immediately on current thread if full
    SIT_SUBMIT_POINTER_ONLY  = 1 << 3   // Do not copy large data; user guarantees lifetime
} SituationJobFlags;
```

---
#### `SituationThreadPool`
Manages a pool of worker threads and dual priority queues.
```c
typedef struct SituationThreadPool {
    bool is_active;
    size_t thread_count;
    // ... internal state (queues, threads, synchronization) ...
} SituationThreadPool;
```

### Functions

---
#### `SituationCreateThreadPool`
Creates a thread pool with a specified number of worker threads and a ring buffer size.
```c
bool SituationCreateThreadPool(SituationThreadPool* pool, size_t num_threads, size_t queue_size);
```
**Usage Example:**
```c
SituationThreadPool pool;
// Create a pool with auto-detected threads and 256 slots per queue
if (SituationCreateThreadPool(&pool, 0, 256)) {
    printf("Thread pool initialized.\n");
}
```

---
#### `SituationDestroyThreadPool`
Shuts down the thread pool, stopping all worker threads and freeing resources. Blocks until all running jobs are finished.
```c
void SituationDestroyThreadPool(SituationThreadPool* pool);
```

---
#### `SituationSubmitJobEx`
Submits a job with advanced control flags and embedded data.
```c
SituationJobId SituationSubmitJobEx(SituationThreadPool* pool, void (*func)(void*, void*), const void* data, size_t data_size, SituationJobFlags flags);
```
- `data`: Pointer to data to pass to the function. If `data_size` <= 64, it is copied into the job structure (zero-allocation). If larger, the pointer is passed directly.
- `flags`: Controls priority and behavior when the queue is full.

**Usage Example:**
```c
typedef struct { mat4 view; vec3 pos; } RenderData; // > 64 bytes

void ProcessRender(void* data, void* unused) {
    RenderData* rd = (RenderData*)data;
    // ...
}

RenderData my_data = { ... };
// Submit to High Priority queue, run immediately if full
SituationSubmitJobEx(&pool, ProcessRender, &my_data, sizeof(RenderData), SIT_SUBMIT_HIGH_PRIORITY | SIT_SUBMIT_RUN_IF_FULL);
```

---
#### `SituationSubmitJob`
Legacy wrapper for simple pointer passing. Equivalent to `SituationSubmitJobEx` with default flags.
```c
#define SituationSubmitJob(pool, func, user_ptr) ...
```

---
#### `SituationSubmitRenderList` (Momentum)
Submits a pre-recorded list of render commands (`SituationRenderList`) to the main execution queue. This allows recording on any thread and submitting on the main thread.
```c
void SituationSubmitRenderList(SituationRenderList list);
```
**Usage Example:**
```c
// Record on worker thread
SituationBeginList(list);
SituationCmdDrawMesh(list, mesh);
SituationEndList(list);

// Submit on main thread
SituationSubmitRenderList(list);
```

---
#### `SituationGetRenderLatencyStats` (Latency Metrics)
Retrieves high-precision, drift-proof latency statistics.
```c
void SituationGetRenderLatencyStats(uint64_t* avg_ns, uint64_t* max_ns);
```
**Usage Example:**
```c
uint64_t avg, max;
SituationGetRenderLatencyStats(&avg, &max);
printf("Latency: Avg %.2fms, Max %.2fms\n", avg / 1e6, max / 1e6);
```

---
#### `SituationDispatchParallel`
Executes a parallel-for loop (Fork-Join pattern). Splits `count` items into batches and distributes them across threads. The calling thread actively participates ("helps") until all items are processed.
```c
void SituationDispatchParallel(SituationThreadPool* pool, int count, int min_batch_size, void (*func)(int index, void* user_data), void* user_data);
```
**Usage Example:**
```c
void ProcessParticle(int index, void* user_data) {
    Particle* particles = (Particle*)user_data;
    UpdateParticle(&particles[index]);
}

// Update 10,000 particles in parallel
SituationDispatchParallel(&pool, 10000, 64, ProcessParticle, all_particles);
```

---
#### `SituationWaitForJob`
Waits for a specific job to complete. Returns immediately if the job is already done.
```c
bool SituationWaitForJob(SituationThreadPool* pool, SituationJobId job_id);
```

---
#### `SituationWaitForAllJobs`
Blocks the calling thread until ALL queues are empty and no jobs are running.
```c
void SituationWaitForAllJobs(SituationThreadPool* pool);
```

### Async I/O Functions
These functions offload file operations to the dedicated I/O thread (Low Priority Queue).

---
#### `SituationLoadSoundFromFileAsync`
Asynchronously loads an audio file from disk in a background thread. It performs a full decode to RAM to avoid main-thread disk I/O.
```c
SituationJobId SituationLoadSoundFromFileAsync(SituationThreadPool* pool, const char* file_path, bool looping, SituationSound* out_sound);
```
**Usage Example:**
```c
SituationSound music;
SituationJobId job = SituationLoadSoundFromFileAsync(&pool, "music.mp3", true, &music);
// ... later ...
if (SituationWaitForJob(&pool, job)) {
    SituationPlayLoadedSound(&music);
}
```

---
#### `SituationLoadFileAsync`
Asynchronously loads a binary file from disk.
```c
SituationJobId SituationLoadFileAsync(SituationThreadPool* pool, const char* file_path, SituationFileLoadCallback callback, void* user_data);
```
**Usage Example:**
```c
void OnDataLoaded(void* data, size_t size, void* user) {
    printf("Loaded %zu bytes.\n", size);
    SIT_FREE(data); // You own the data!
}
SituationLoadFileAsync(&pool, "data.bin", OnDataLoaded, NULL);
```

---
#### `SituationLoadFileTextAsync`
Asynchronously loads a text file from disk.
```c
SituationJobId SituationLoadFileTextAsync(SituationThreadPool* pool, const char* file_path, SituationFileTextLoadCallback callback, void* user_data);
```

---
#### `SituationSaveFileAsync`
Asynchronously saves data to a file. The data is copied to a temporary buffer so the caller can free their copy immediately.
```c
SituationJobId SituationSaveFileAsync(SituationThreadPool* pool, const char* file_path, const void* data, size_t size, SituationFileSaveCallback callback, void* user_data);
```

---
#### `SituationSaveFileTextAsync`
Asynchronously saves a string to a text file.
```c
SituationJobId SituationSaveFileTextAsync(SituationThreadPool* pool, const char* file_path, const char* text, SituationFileSaveCallback callback, void* user_data);
```

</details>
<details>
<summary><h3>Miscellaneous Module</h3></summary>

**Overview:** This module includes powerful utilities like the Temporal Oscillator System for rhythmic timing, a suite of color space conversion functions (RGBA, HSV, YPQA), and essential memory management helpers for data allocated by the library.

### Structs and Enums

#### `ColorRGBA`
Represents a color in the Red, Green, Blue, Alpha color space. Each component is an 8-bit unsigned integer (0-255).
```c
typedef struct ColorRGBA {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} ColorRGBA;
```

---
#### `ColorHSVA`
Represents a color in the Hue, Saturation, Value, Alpha color space.
```c
typedef struct ColorHSVA {
    float h; // Hue (0-360)
    float s; // Saturation (0-1)
    float v; // Value (0-1)
    float a; // Alpha (0-1)
} ColorHSVA;
```

---
#### `ColorYPQA`
Represents a color in a custom YPQA color space (Luma, Phase, Quadrature, Alpha).
```c
typedef struct ColorYPQA {
    float y; // Luma
    float p; // Phase
    float q; // Quadrature
    float a; // Alpha
} ColorYPQA;
```

### Functions

#### Temporal Oscillator System
---
#### `SituationTimerHasOscillatorUpdated`
Checks if an oscillator has just completed a cycle in the current frame. This is a single-frame trigger, ideal for synchronizing events to a rhythmic beat.
```c
bool SituationTimerHasOscillatorUpdated(int oscillator_id);
```
**Usage Example:**
```c
// In Init, create an oscillator with a 0.5s period (120 BPM).
double period = 0.5;
init_info.oscillator_count = 1;
init_info.oscillator_periods = &period;

// In the Update loop, check for the beat.
if (SituationTimerHasOscillatorUpdated(0)) {
    printf("Beat!\n");
    // Play a drum sound, flash a light, or trigger a game event.
}
```
---
#### `SituationSetTimerOscillatorPeriod`
Sets a new period for an oscillator at runtime, allowing you to change the tempo of rhythmic events dynamically.
```c
SituationError SituationSetTimerOscillatorPeriod(int oscillator_id, double period_seconds);
```
---
#### `SituationTimerGetOscillatorValue` / `SituationTimerGetOscillatorPhase`
Gets the current value (typically 0.0 to 1.0) or phase (0.0 to 2*PI) of an oscillator.
```c
double SituationTimerGetOscillatorValue(int oscillator_id);
double SituationTimerGetOscillatorPhase(int oscillator_id);
```
**Usage Example:**
```c
// Use an oscillator to create a smooth pulsing animation
double pulse = SituationTimerGetOscillatorValue(0); // This will smoothly go 0 -> 1 -> 0
float scale = 1.0f + (float)pulse * 0.2f;
// Apply 'scale' to a transform
```

---
#### `SituationTimerGetTime`
`double SituationTimerGetTime(void);`
Gets the total time elapsed since the library was initialized. This function returns the master application time, updated once per frame by `SituationUpdateTimers()`. It serves as the high-resolution monotonic clock for the entire application and is the basis for all other timing functions, including the Temporal Oscillator system.

**Usage Example:**
```c
// Get the total elapsed time to drive a procedural animation.
double totalTime = SituationTimerGetTime();
float pulse = sinf((float)totalTime * 2.0f); // A simple pulsing effect over time.
// Use 'pulse' to modify an object's color, size, etc.
```
---
#### Color Space Conversions
---
#### `SituationRgbToHsv` / `SituationHsvToRgb`
Converts a color between RGBA and HSV (Hue, Saturation, Value) color spaces.
```c
ColorHSV SituationRgbToHsv(ColorRGBA rgb);
ColorRGBA SituationHsvToRgb(ColorHSV hsv);
```
**Usage Example:**
```c
// Create a rainbow effect by cycling the hue
ColorHSV hsv_color = {.h = fmodf(SituationTimerGetTime() * 50.0f, 360.0f), .s = 1.0, .v = 1.0};
ColorRGBA final_color = SituationHsvToRgb(hsv_color);
```

---
#### `SituationTimerGetOscillatorState`
Gets the current binary state (0 or 1) of an oscillator.
```c
bool SituationTimerGetOscillatorState(int oscillator_id);
```

---
#### `SituationTimerGetPreviousOscillatorState`
Gets the previous frame's state of an oscillator.
```c
bool SituationTimerGetPreviousOscillatorState(int oscillator_id);
```

---
#### `SituationTimerPingOscillator`
Checks if an oscillator's period has elapsed since the last ping.
```c
bool SituationTimerPingOscillator(int oscillator_id);
```

---
#### `SituationTimerGetOscillatorTriggerCount`
Gets the total number of times an oscillator has triggered.
```c
uint64_t SituationTimerGetOscillatorTriggerCount(int oscillator_id);
```

---
#### `SituationTimerGetOscillatorPeriod`
Gets the period of an oscillator in seconds.
```c
double SituationTimerGetOscillatorPeriod(int oscillator_id);
```

---
#### `SituationTimerGetPingProgress`
Gets progress [0.0 to 1.0+] of the interval since the last successful ping.
```c
double SituationTimerGetPingProgress(int oscillator_id);
```

---
#### `SituationConvertColorToVec4`
Converts an 8-bit ColorRGBA struct to a normalized vec4.
```c
void SituationConvertColorToVec4(ColorRGBA c, vec4 out_normalized_color);
```

---
#### `SituationHsvToRgb`
Converts a Hue, Saturation, Value color back to the standard RGBA color space.
```c
ColorRGBA SituationHsvToRgb(ColorHSV hsv);
```

---
#### `SituationColorToYPQ`
Converts a standard RGBA color to the YPQA (Luma, Phase, Quadrature) color space.
```c
ColorYPQA SituationColorToYPQ(ColorRGBA color);
```

---
#### `SituationColorFromYPQ`
Converts a YPQA color back to the standard RGBA color space.
```c
ColorRGBA SituationColorFromYPQ(ColorYPQA ypq_color);
```

---
#### `SituationRgbToYpqa` / `SituationYpqaToRgb`
Converts a color between RGBA and the YPQA color space (a custom space for signal processing-like effects).
```c
ColorYPQA SituationRgbToYpqa(ColorRGBA rgb);
ColorRGBA SituationYpqaToRgb(ColorYPQA ypqa);
```
---
#### Memory Management Helpers
---
#### `SituationFreeString`
Frees the memory for a string allocated and returned by the library (e.g., from `SituationGetLastErrorMsg`).
```c
void SituationFreeString(char* str);
```
---
#### `SituationFreeDisplays`
Frees the memory for the array of display information returned by `SituationGetDisplays`.
```c
void SituationFreeDisplays(SituationDisplayInfo* displays, int count);
```
---
#### `SituationFreeDirectoryFileList`
Frees the memory for the list of file paths returned by `SituationListDirectoryFiles`.
```c
void SituationFreeDirectoryFileList(char** files, int count);
```
</details>
<details>
<summary><h3>Hot-Reloading Module</h3></summary>

**Overview:** The Hot-Reloading module provides a powerful suite of functions to dynamically reload assets like shaders, textures, and models while the application is running. This significantly accelerates development by allowing for instant iteration on visual and computational resources without needing to restart the application. The system works by monitoring the last modification time of source files and triggering a reload when a change is detected.

### Functions

---
#### `SituationCheckHotReloads`
Checks all registered resources for changes and reloads them if necessary. This is the main entry point for the hot-reloading system and should be called once per frame in your main update loop.
```c
SITAPI void SituationCheckHotReloads(void);
```
**Usage Example:**
```c
// In the main application loop, after polling for input and updating timers.
while (!SituationWindowShouldClose()) {
    SITUATION_BEGIN_FRAME();

    // Check for any modified assets and reload them automatically.
    SituationCheckHotReloads();

    // ... proceed with application logic and rendering ...
}
```

---
#### `SituationReloadShader`
Forces an immediate reload of a specific graphics shader from its source files. This function is useful for targeted reloads or when you want to trigger a reload manually, for example, via a debug console command.
```c
SITAPI SituationError SituationReloadShader(SituationShader* shader);
```
**Usage Example:**
```c
// Assume 'g_main_shader' is the handle to your primary shader.
// In a debug input handler:
if (SituationIsKeyPressed(SIT_KEY_R)) {
    if (SituationReloadShader(&g_main_shader) == SIT_SUCCESS) {
        printf("Main shader reloaded successfully.\n");
    } else {
        printf("Failed to reload main shader: %s\n", SituationGetLastErrorMsg());
    }
}
```

---
#### `SituationReloadComputePipeline`
Forces an immediate reload of a specific compute pipeline from its source file.
```c
SITAPI SituationError SituationReloadComputePipeline(SituationComputePipeline* pipeline);
```
**Usage Example:**
```c
// Assume 'g_particle_sim' is your compute pipeline for particle physics.
// Reload it when 'F5' is pressed.
if (SituationIsKeyPressed(SIT_KEY_F5)) {
    SituationReloadComputePipeline(&g_particle_sim);
}
```

---
#### `SituationReloadTexture`
Forces an immediate reload of a specific texture from its source file. Note that the texture must have been originally created from a file for this to work.
```c
SITAPI SituationError SituationReloadTexture(SituationTexture* texture);
```
**Usage Example:**
```c
// In a material editor, when the user saves changes to a texture file.
void OnTextureFileSaved(const char* filepath) {
    // Find the texture associated with this filepath and reload it.
    SituationTexture* texture_to_reload = FindTextureByFilepath(filepath);
    if (texture_to_reload) {
        SituationReloadTexture(texture_to_reload);
    }
}
```

---
#### `SituationReloadModel`
Forces an immediate reload of a 3D model, including all its meshes and materials, from its source file (e.g., GLTF).
```c
SITAPI SituationError SituationReloadModel(SituationModel* model);
```
**Usage Example:**
```c
// In a 3D modeling workflow, reload the main scene model when requested.
if (UserRequestedModelReload()) {
    SituationReloadModel(&g_main_scene_model);
}
```
</details>

<details>
<summary><h3>Logging Module</h3></summary>

**Overview:** This module provides simple and direct functions for logging messages to the console. It allows for different log levels, enabling you to control the verbosity of the output for debugging and informational purposes.

### Functions

---
#### `SituationLog`
Prints a formatted log message to the console with a specified log level (e.g., `SIT_LOG_INFO`, `SIT_LOG_WARNING`, `SIT_LOG_ERROR`). It uses a `printf`-style format string.
```c
void SituationLog(int msgType, const char* text, ...);
```
**Usage Example:**
```c
int score = 100;
// Logs an informational message.
SituationLog(SIT_LOG_INFO, "Player reached a score of %d", score);

// Logs a warning if a value is unexpected.
if (score > 9000) {
    SituationLog(SIT_LOG_WARNING, "Score is over 9000!");
}
```

---
#### `SituationSetTraceLogLevel`
Sets the minimum log level to be displayed. Any message with a lower severity will be ignored. For example, setting the level to `SIT_LOG_WARNING` will show warnings, errors, and fatal messages, but hide info, debug, and trace messages.
```c
void SituationSetTraceLogLevel(int logType);
```
**Usage Example:**
```c
// During development, show all log messages for detailed debugging.
SituationSetTraceLogLevel(SIT_LOG_ALL);

// For a release build, only show warnings and errors to reduce noise.
#ifdef NDEBUG
    SituationSetTraceLogLevel(SIT_LOG_WARNING);
#endif
```
</details>

<details>
<summary><h3>Compute Shaders</h3></summary>

### 4.7 Compute Shaders
#### 4.7.1 Overview & Capabilities
Compute shaders enable developers to harness the parallel processing power of the GPU for general-purpose computations that are independent of the traditional graphics rendering pipeline. This includes tasks like physics simulations, AI calculations, image/video processing, procedural generation, and more. `situation.h` provides a unified, backend-agnostic API to create, manage, and execute compute shaders using either OpenGL Compute Shaders or Vulkan Compute Pipelines.

#### 4.7.2 Initialization Prerequisites
- The core `situation.h` library must be successfully initialized using `SituationInit`.
- Define `SITUATION_ENABLE_SHADER_COMPILER` in your build. This is **mandatory** for the Vulkan backend and highly recommended for OpenGL if you are providing GLSL source code. It enables the inclusion and use of the `shaderc` library for runtime compilation of GLSL to SPIR-V bytecode.
- For Vulkan: Ensure that the selected physical device (GPU) supports compute capabilities. This is checked during `SituationInit` if a Vulkan backend is chosen.

#### 4.7.3 Creating Compute Pipelines
##### 4.7.3.1 From GLSL Source Code (SituationCreateComputePipelineFromMemory)
This is the primary function for creating a compute pipeline.
- **Signature:** `SITAPI SituationError SituationCreateComputePipelineFromMemory(const char* compute_shader_source, SituationComputeLayoutType layout_type, SituationComputePipeline* out_pipeline);`
- **Parameters:**
    - `compute_shader_source`: A null-terminated string containing the GLSL compute shader source code.
- **Process:**
    1.  Validates that the library is initialized and the source is not NULL.
    2.  If `SITUATION_ENABLE_SHADER_COMPILER` is defined:
        a.  Invokes `shaderc` to compile the provided GLSL source into SPIR-V bytecode.
        b.  Handles compilation errors and reports them via the error system.
    3.  Backend-Specific Creation:
        - **OpenGL**: Uses the SPIR-V (if compiled) or directly the GLSL source (if `ARB_gl_spirv` is not used/available) to create and link an OpenGL Compute Program (`glCreateProgram`, `glCreateShader(GL_COMPUTE_SHADER)`, `glLinkProgram`).
        - **Vulkan**: Uses the compiled SPIR-V bytecode to create a `VkShaderModule`, then a `VkPipelineLayout` (handling push constants), and finally the `VkComputePipeline` object.
    4.  Generates a unique `id` for the `SituationComputePipeline` handle.
    5.  Stores backend-specific handles internally (e.g., `gl_program_id`, `vk_pipeline`, `vk_pipeline_layout`).
    6.  Adds the pipeline to an internal tracking list for resource management and leak detection.
- **Return Value:**
    - Returns `SITUATION_SUCCESS` on success.
    - On success, `out_pipeline->id` will be a non-zero value.
    - On failure, returns an error code and `out_pipeline->id` will be 0.

##### 4.7.3.2 Backend Compilation (OpenGL SPIR-V, Vulkan Runtime)
- The use of `shaderc` via `SITUATION_ENABLE_SHADER_COMPILER` standardizes the input (GLSL) and the intermediate representation (SPIR-V) for both backends, making the API consistent.
- OpenGL traditionally uses GLSL directly, but `ARB_gl_spirv` allows using SPIR-V. The library abstracts this choice.
- Vulkan *requires* SPIR-V, making runtime compilation with `shaderc` essential unless pre-compiled SPIR-V is used (which this function doesn't directly support, but the underlying Vulkan creation could be adapted).

#### 4.7.4 Using Compute Pipelines
Once a `SituationComputePipeline` is created, it can be used within a command buffer to perform computations.

##### 4.7.4.1 Binding a Compute Pipeline (SituationCmdBindComputePipeline)
- **Signature:** `SITAPI void SituationCmdBindComputePipeline(SituationCommandBuffer cmd, SituationComputePipeline pipeline);`
- **Parameters:**
    - `cmd`: The command buffer obtained from `SituationAcquireFrameCommandBuffer` or `SituationBeginVirtualDisplayFrame`.
    - `pipeline`: The `SituationComputePipeline` handle returned by `SituationCreateComputePipelineFromMemory`.
- **Process:**
    1.  Validates the command buffer and pipeline handle.
    2.  Records the command to bind the pipeline state (program/pipeline object) to the command buffer for subsequent compute operations.
    3.  Backend-Specific:
        - **OpenGL**: Calls `glUseProgram(pipeline.gl_program_id)`.
        - **Vulkan**: Calls `vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.vk_pipeline)`.

##### 4.7.4.2 Binding Resources (Buffers, Images)
Compute shaders often read from and write to GPU resources like Shader Storage Buffer Objects (SSBOs) or Images.
- `SITAPI void SituationCmdBindComputeBuffer(SituationCommandBuffer cmd, SituationBuffer buffer, uint32_t binding);`
    - Binds a `SituationBuffer` (created with appropriate usage flags like `SITUATION_BUFFER_USAGE_STORAGE_BUFFER`) to a specific binding point in the currently bound compute shader.
    - **Backend-Specific:**
        - **OpenGL**: Calls `glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer.gl_buffer_id)`.
        - **Vulkan**: Allocates a temporary descriptor set (or uses a pre-allocated one) that describes the buffer binding, then records `vkCmdBindDescriptorSets` for that set.

##### 4.7.4.3 Dispatching Work (SituationCmdDispatch)
- **Signature:** `SITAPI void SituationCmdDispatch(SituationCommandBuffer cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);`
- **Parameters:**
    - `cmd`: The command buffer.
    - `group_count_x/y/z`: The number of local work groups to dispatch in each dimension. The total number of invocations is `group_count_x * group_count_y * group_count_z * local_size_x * local_size_y * local_size_z` (where local size is defined in the shader).
- **Process:**
    1.  Validates the command buffer.
    2.  Records the command to dispatch the compute work.
    3.  Backend-Specific:
        - **OpenGL**: Calls `glDispatchCompute(group_count_x, group_count_y, group_count_z)`.
        - **Vulkan**: Calls `vkCmdDispatch(cmd, group_count_x, group_count_y, group_count_z)`.

#### 4.7.5 Synchronization & Memory Barriers
##### 4.7.5.1 Importance of Synchronization in Compute
GPU operations, including compute shaders, can execute asynchronously and out-of-order relative to CPU commands and other GPU operations. Memory barriers are crucial to ensure that reads happen after writes, and that dependencies between operations are correctly observed.

##### 4.7.5.2 Using SituationCmdPipelineBarrier
The primary tool for synchronization is `SituationCmdPipelineBarrier`. It provides fine-grained control by explicitly defining the source of a memory dependency (the producer stage) and the destination (the consumer stage). This allows the driver to create a much more efficient barrier than a coarse, "sledgehammer" approach.

- **Signature:** `SITAPI void SituationCmdPipelineBarrier(SituationCommandBuffer cmd, uint32_t src_flags, uint32_t dst_flags);`
- **Parameters:**
    - `cmd`: The command buffer to record the barrier into.
    - `src_flags`: A bitmask of `SituationBarrierSrcFlags`.
    - `dst_flags`: A bitmask of `SituationBarrierDstFlags`.

**Source Flags (`SituationBarrierSrcFlags`):**
| Flag | Description |
|---|---|
| `SITUATION_BARRIER_VERTEX_SHADER_WRITE` | Vertex shader wrote to an SSBO or Image. |
| `SITUATION_BARRIER_FRAGMENT_SHADER_WRITE` | Fragment shader wrote to an SSBO, Image, or Color Attachment. |
| `SITUATION_BARRIER_COMPUTE_SHADER_WRITE` | Compute shader wrote to a Storage Buffer or Image. |
| `SITUATION_BARRIER_TRANSFER_WRITE` | A transfer operation (Copy, Blit, Fill) wrote data. |

**Destination Flags (`SituationBarrierDstFlags`):**
| Flag | Description |
|---|---|
| `SITUATION_BARRIER_VERTEX_SHADER_READ` | Vertex shader will read data (SSBO, Image, Vertex Attribute). |
| `SITUATION_BARRIER_FRAGMENT_SHADER_READ` | Fragment shader will read data. |
| `SITUATION_BARRIER_COMPUTE_SHADER_READ` | Compute shader will read data. |
| `SITUATION_BARRIER_TRANSFER_READ` | A transfer operation will read data. |
| `SITUATION_BARRIER_INDIRECT_COMMAND_READ` | Data will be read as indirect command parameters. |

- **Process:** This function maps the abstract source and destination flags to the precise stage and access masks required by the underlying API (`vkCmdPipelineBarrier` in Vulkan or a combination of flags for `glMemoryBarrier` in OpenGL).
- **Example: GPU Particle Simulation**
A common use case is updating particle positions in a compute shader and then immediately rendering them. A barrier is required between the dispatch and the draw call.
```c
// 1. Dispatch compute shader to update particle data in an SSBO
SituationCmdBindComputePipeline(cmd, particle_update_pipeline);
SituationCmdBindComputeBuffer(cmd, 0, particle_data_ssbo);
SituationCmdDispatch(cmd, PARTICLE_GROUPS, 1, 1);

// 2. *** CRITICAL BARRIER ***
//    Ensure the compute shader's writes to the SSBO are finished and visible
//    before the vertex shader stage attempts to read that data as vertex attributes.
SituationCmdPipelineBarrier(cmd,
                          SITUATION_BARRIER_COMPUTE_SHADER_WRITE,
                          SITUATION_BARRIER_VERTEX_SHADER_READ);

// 3. Draw the particles using a graphics pipeline
SituationCmdBindPipeline(cmd, particle_render_pipeline);
// The same SSBO is now used as the source for vertex data
SituationCmdBindVertexBuffer(cmd, particle_data_ssbo);
SituationCmdDraw(cmd, PARTICLE_COUNT, 1, 0, 0);
```
- **Note on `SituationMemoryBarrier`:**
The library also provides a simpler, deprecated function `SituationMemoryBarrier(cmd, barrier_bits)`. This function is less optimal as it creates a very coarse barrier. It is provided for backward compatibility or extremely simple cases. For all new development, **`SituationCmdPipelineBarrier` is strongly recommended.**

#### 4.7.6 Destroying Compute Pipelines (SituationDestroyComputePipeline)
- **Signature:** `SITAPI void SituationDestroyComputePipeline(SituationComputePipeline* pipeline);`
- **Parameters:**
    - `pipeline`: A pointer to the `SituationComputePipeline` handle to be destroyed. The handle's `id` field will be set to 0 upon successful destruction.
- **Process:**
    1.  Validates the input pointer and that the pipeline has a non-zero `id`.
    2.  Removes the pipeline from the internal tracking list.
    3.  Backend-Specific Cleanup:
        - **OpenGL**: Calls `glDeleteProgram(pipeline->gl_program_id)`.
        - **Vulkan**:
            a. Waits for the device to be idle (`vkDeviceWaitIdle`) to ensure no commands using the pipeline are pending.
            b. Destroys the Vulkan objects: `vkDestroyPipeline`, `vkDestroyPipelineLayout`.
    4.  Invalidates the handle by setting `pipeline->id = 0`.

#### 4.7.7 Compute Presentation (SituationCmdPresent)
- **Signature:** `SITAPI SituationError SituationCmdPresent(SituationCommandBuffer cmd, SituationTexture texture);`
- **Description:** Submits a command to copy a texture to the main window's swapchain. This is specifically designed for **Compute-Only** applications where there is no standard render pass (e.g., ray tracing or terminal emulators) and you need to display the result of a compute shader (a storage image) directly to the screen.

</details>
<details>
<summary><h3>Text Rendering</h3></summary>

### 4.9 Text Rendering
#### 4.9.1 Simple Text Drawing (SituationDrawTextSimple)
- **Signature:** `SITAPI void SituationDrawTextSimple(const char* text, float x, float y, float scale, ColorRGBA color);`
- Draws text character by character using a simple, built-in font (often 8x8 or similar bitmap).
- Parameters define position (`x`, `y`), size (`scale`), and color.
- **Note:** As indicated in the library code comments, this method is intentionally slow and intended for debugging or simple UI elements where performance is not critical.

#### 4.9.2 Styled Text Rendering (SituationDrawTextStyled)
- **Signature:** `SITAPI void SituationDrawTextStyled(SituationFont font, const char* text, float x, float y, float font_size, ColorRGBA color);`
- Draws high-quality text using pre-rendered font atlases (textures) and Signed Distance Fields (SDF).
- Requires a `SituationFont` handle, obtained via font loading functions.
- Offers superior performance and visual quality (smooth scaling, sharp edges) compared to `SituationDrawTextSimple`.
- Parameters define the font, text string, position, size (`font_size`), and color.

#### 4.9.3 Font Loading & Management
- `SITAPI SituationError SituationLoadFont(const char* fileName, SituationFont* out_font);`
    - Loads a TrueType Font (TTF) file.
    - Internally uses `stb_truetype` to parse the font and generate SDF data for an atlas texture.
    - Returns `SITUATION_SUCCESS` on success.
- `SITAPI void SituationUnloadFont(SituationFont font);`
    - Destroys a loaded font, freeing the associated atlas texture and `stbtt_fontinfo` data.

#### 4.9.4 GPU Text Drawing (Command Buffer)
For best performance and integration with the rendering pipeline, use these command-buffer variants.

- **`SituationCmdDrawText`**
    - **Signature:** `SITAPI SituationError SituationCmdDrawText(SituationCommandBuffer cmd, SituationFont font, const char* text, Vector2 pos, ColorRGBA color);`
    - Draws a text string using GPU-accelerated textured quads. This is the preferred method for rendering text within a render pass.

- **`SituationCmdDrawTextEx`**
    - **Signature:** `SITAPI SituationError SituationCmdDrawTextEx(SituationCommandBuffer cmd, SituationFont font, const char* text, Vector2 pos, float fontSize, float spacing, ColorRGBA color);`
    - Advanced version of `SituationCmdDrawText` that allows for custom scaling (`fontSize`) and character spacing (`spacing`).

</details>
<details>
<summary><h3>2D Rendering & Drawing</h3></summary>

### 4.10 2D Rendering & Drawing
While "Situation" is a powerful 3D rendering library, it also provides a comprehensive and high-performance suite of tools specifically for classic 2D drawing. This is ideal for building user interfaces, debugging overlays, data visualizations, or complete 2D games. All 2D drawing functions operate within the Command Buffer model.

#### 4.10.1  2D Coordinate System & Camera
For all 2D drawing commands, "Situation" automatically establishes a 2D orthographic coordinate system. The origin (0, 0) is at the **top-left** corner of the current render target (either the main window or a Virtual Display). The X-axis increases to the right, and the Y-axis increases downwards. You do not need to set up a 3D camera or projection matrix; the library manages this internally for all `SituationCmdDraw*` 2D functions.

#### 4.10.2  Drawing Basic Shapes
The library provides commands for rendering primitive geometric shapes, which form the building blocks of any 2D scene.

##### 4.10.2.1 Rectangles (SituationCmdDrawQuad)
This is the primary function for drawing solid-colored rectangles. It uses the library's internal, optimized quad renderer.
- **Signature:** `SITAPI SituationError SituationCmdDrawQuad(SituationCommandBuffer cmd, mat4 model, Vector4 color);`
- `model`: A `mat4` transformation matrix used to set the rectangle's position, size, and rotation. Use `cglm` helpers (`glm_translate`, `glm_scale`, `glm_rotate`) to build this matrix.
- `color`: A normalized `Vector4` representing the RGBA color of the quad.

##### 4.10.2.2 Lines & Circles (Concept)
While not yet implemented, the API is designed to easily accommodate high-level commands for drawing other primitives like lines (`SituationCmdDrawLine`), circles (`SituationCmdDrawCircle`), and polygons.

#### 4.10.3  Drawing Textures (Sprites)
This is the core of 2D rendering, allowing you to draw images and sprite sheets to the screen.

##### 4.10.3.1 Loading Textures
First, load your image file into a `SituationTexture` handle using the functions described in section `4.6.3`.
- `SituationTexture my_sprite = SituationCreateTexture(SituationLoadImage("assets/player.png"), true);`

##### 4.10.3.2 Drawing a Texture (SituationCmdDrawTexture)
This unified high-level command draws a texture (or part of it) with full control over source region, destination rectangle, rotation, origin, and color tint.
- **Signature:** `SITAPI SituationError SituationCmdDrawTexture(SituationCommandBuffer cmd, SituationTexture texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, ColorRGBA tint);`
- `texture`: The `SituationTexture` to draw.
- `source`: The rectangular region of the texture to draw (for sprite sheets). Use a full rect `{0,0,w,h}` for the whole texture.
- `dest`: The destination rectangle on the screen, defining position and size.
- `origin`: The rotation pivot point, relative to the top-left of the destination rectangle. `(0,0)` pivots from the top-left corner.
- `rotation`: The rotation in degrees (clockwise).
- `tint`: A `ColorRGBA` multiplier. White `{255,255,255,255}` draws the texture with its original colors.

#### 4.10.4  Text Rendering
The library includes a powerful text rendering system suitable for UI, HUDs, and any in-game text. For a full API reference, see section `4.9`.
- **High-Quality Styled Text:** Use `SituationDrawTextStyled` for crisp, anti-aliased text with support for TrueType fonts (.ttf). This is the recommended function for all user-facing text.
- **Simple Debug Text:** Use `SituationDrawTextSimple` for quick, unstyled text output, ideal for debugging information where performance and visual quality are not critical.

#### 4.10.5  UI & Layer Management
"Situation" provides two key features that are essential for building complex 2D UIs and managing render layers.

##### 4.10.5.1 Scissor/Clipping (SituationCmdSetScissor)
The scissor command restricts all subsequent drawing to a specific rectangular area on the screen. This is indispensable for creating UI elements like scrollable lists, text boxes, or windows where content must be clipped to a boundary.
- See section `4.6.8.5` for the full API reference.
- **Example workflow:**
  1. Call `SituationCmdSetScissor(cmd, panel_x, panel_y, panel_width, panel_height);`
  2. Draw all content that should appear inside the panel.
  3. Disable the scissor by setting it to the full screen size.

##### 4.10.5.2 Virtual Displays as UI Layers
The Virtual Display system (see `4.6.5`) is a perfect tool for 2D layer management. You can render an entire UI screen or game layer to an off-screen Virtual Display first. This allows you to:
- Apply shaders and post-processing effects to the entire UI layer at once.
- Scale a low-resolution UI to a high-resolution screen with pixel-perfect filtering (`SITUATION_SCALING_INTEGER`).
- Easily manage render order using the `z_order` property when compositing the layers back to the main window.
</details>

</details>





---
## License (MIT)

"Situation" is licensed under the permissive MIT License. In simple terms, this means you are free to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the software for both commercial and private projects. The only requirement is that you include the original copyright and license notice in any substantial portion of the software or derivative work you distribute. This library is provided "as is", without any warranty.

---

Copyright (c) 2025 Jacques Morel

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

<details>
<summary><h3>Examples & Tutorials</h3></summary>

### 5.1 Terminal Module
The "Terminal" module is a comprehensive extension subsystem that provides a complete VT100/xterm-compatible terminal emulator. It is located in `sit/terminal/` and integrates deeply with the Situation rendering and input pipeline.

#### 5.1.1 Overview
The terminal library emulates a wide range of standards (VT52 through VT420/xterm) and supports modern features like True Color (24-bit), mouse tracking (SGR), and bracketed paste. It uses a **Compute Shader** based rendering pipeline (`SIT_COMPUTE_LAYOUT_TERMINAL`) to efficiently render the character grid, attributes, and colors on the GPU.

#### 5.1.2 Key APIs
-   `InitTerminal()`: Initializes the terminal state and compute resources.
-   `UpdateTerminal()`: Processes input, updates state, and manages the host-terminal pipeline.
-   `DrawTerminal()`: Renders the terminal to the screen using a compute shader dispatch.
-   `PipelineWrite...()`: Functions to send data (text, escape sequences) to the terminal emulation.
-   `SetVTLevel()`: Configures the emulation compliance level.

#### 5.1.3 Integration
The terminal module relies on the `SIT_COMPUTE_LAYOUT_TERMINAL` layout defined in `situation.h`. This layout configures the descriptor sets required by the terminal's compute shader:
1.  **Set 0 (SSBO):** Contains the terminal grid data (`GPUCell` array).
2.  **Set 1 (Storage Image):** The target image for the rendered output.
3.  **Set 2 (Sampler):** The font atlas texture.

For detailed documentation, see the [Terminal Technical Reference](sit/terminal/terminal.md) and [Terminal README](sit/terminal/README.md).

### 6.1 Basic Triangle Rendering
This example demonstrates the minimal steps required to render a single, colored triangle using `situation.h`. It covers window setup, shader creation, mesh definition, and the core rendering loop.

The full source code for this example can be found in `examples/basic_triangle.c`.

### 6.2 Loading and Rendering a 3D Model
This example shows how to load a 3D model from a file (e.g., Wavefront .OBJ) and render it using `situation.h`. It assumes the existence of a function like `SituationLoadModelFromObj` (based on library snippets) or a similar model loading mechanism.

The full source code for this example can be found in `examples/loading_and_rendering_a_3d_model.c`.

### 6.3 Playing Background Music
This example demonstrates how to load and play a sound file (e.g., WAV, OGG) in a continuous loop using the audio capabilities of `situation.h`.

The full source code for this example can be found in `examples/playing_background_music.c`.

### 6.4 Handling Keyboard and Mouse Input
This example shows how to query the state of keyboard keys and the mouse position within the main application loop, and how to use this input to control simple application behavior (e.g., moving an on-screen element or closing the window).

The full source code for this example can be found in `examples/handling_keyboard_and_mouse_input.c`.

### 6.5 Compute Shader Example: Image Processing (SSBO Version - Updated for Persistent Descriptor Sets)
This example demonstrates using compute shaders with `situation.h` to perform a simple image processing task (inverting colors) by reading from and writing to Shader Storage Buffer Objects (SSBOs). This approach uses the confirmed API function `SituationCmdBindComputeBuffer`, which now correctly implements the high-performance, persistent descriptor set model for Vulkan.

The full source code for this example can be found in `examples/compute_shader_image_processing.c`.

#### 6.5.1 Problem Definition (Updated)
We want to take the pixel data of an image (already loaded into a `SituationBuffer` configured as an SSBO) and invert its colors using the GPU compute shader. The result will be written to another `SituationBuffer`.
**Crucial Note on Performance:** The library now ensures that binding these buffers for the compute shader is highly efficient. When a `SituationBuffer` is created using `SituationCreateBuffer`, the Vulkan backend internally:
1.  Allocates a `VkBuffer` and `VmaAllocation`.
2.  **Crucially:** Allocates a *persistent* `VkDescriptorSet` from a dedicated pool (`sit_gs.vk.persistent_descriptor_pool`).
3.  Immediately populates this descriptor set with the buffer's `VkBuffer` handle.
4.  Stores this `VkDescriptorSet` within the `SituationBuffer` struct (`buffer.descriptor_set`).
This means that subsequent binding operations are extremely fast, as they do not involve any runtime allocation or update of descriptor sets.

#### 6.5.2 Compute Shader Code (GLSL using SSBOs)
The shader reads RGBA values from an input SSBO, inverts them, and writes the result to an output SSBO. Each invocation processes one pixel.

#### 6.5.3 Host Code Walkthrough (Init, Create, Bind Buffers, Dispatch, Sync, Destroy)
This C code shows how to prepare data, create buffers, load the shader, bind resources, dispatch the compute job, synchronize, and clean up using the *existing* `situation.h` API.

### 6.6 Example: GPU Particle Simulation and Rendering (Concept)
This example concept demonstrates a fundamental and powerful technique: combining compute and graphics pipelines within a single frame. It illustrates how to use a compute shader to update simulation data (like particle positions and velocities) stored in GPU buffers, and then immediately use a standard graphics pipeline to render the results in the same frame.

A conceptual implementation for this example can be found in `examples/gpu_particle_simulation.c`.

#### 6.6.1 Scenario
The core idea is to perform calculations on the GPU (using a compute shader) and then visualize the results (using a graphics shader) without stalling the pipeline or introducing race conditions.

1. **Compute Shader:** A compute shader operates on a buffer of particle data (e.g., struct { vec2 position; vec2 velocity; }). It reads the current state,
applies simulation logic (e.g., physics updates like velocity integration, applying forces), and writes the new state back to the same or a different buffer.
2. **Graphics Shader:** A vertex shader (potentially using instancing) reads the updated particle data from the buffer and uses it to position geometry
(e.g., a quad or sprite) for each particle on the screen.
3. **Synchronization:** The critical aspect is ensuring the compute shader's writes are globally visible and finished before the vertex shader attempts
to read that data. This requires explicit synchronization.

#### 6.6.2 Key APIs Demonstrated
This example concept highlights the interaction between several situation.h APIs:

- **SituationCreateBuffer / SituationDestroyBuffer:** Used to create the GPU buffer(s) that will store the particle simulation data (positions, velocities).
These buffers must be created with appropriate usage flags (e.g., SITUATION_BUFFER_USAGE_STORAGE_BUFFER for compute read/write, potentially
SITUATION_BUFFER_USAGE_VERTEX_BUFFER if used as such in the graphics pipeline, or bound via SituationCmdBindUniformBuffer if accessed as an SSBO).
- **SituationCreateComputePipelineFromMemory / SituationDestroyComputePipeline:**
Used to create the compute pipeline that will execute the particle update logic.
- **SituationCmdBindComputePipeline:** Binds the compute pipeline for subsequent dispatch commands.
- **SituationCmdBindComputeBuffer:** Binds the particle data buffer to a specific binding point within the compute shader's descriptor set.
- **SituationCmdDispatch:** Launches the compute shader work groups to perform the particle simulation update.
- **SituationMemoryBarrier:** Crucially, this function is used after the compute dispatch and before the graphics draw call. It inserts a memory and execution
barrier to ensure all compute shader invocations have completed their writes (SITUATION_BARRIER_COMPUTE_SHADER_STORAGE_WRITE) and that these writes are
visible to the subsequent graphics pipeline stages that will read the data (SITUATION_BARRIER_VERTEX_SHADER_STORAGE_READ or similar). Without this
barrier, the graphics pipeline might read stale or partially updated data.
- **SituationCmdBindPipeline (Graphics):** Binds the graphics pipeline used for rendering the particles.
- **SituationCmdBindVertexBuffer / SituationCmdBindIndexBuffer:** Binds the mesh data (e.g., a simple quad) used for instanced rendering of particles.
- **SituationCmdBindUniformBuffer / SituationCmdBindTexture:** Binds resources needed by the graphics shaders (e.g., the particle data buffer if accessed as an SSBO, textures for particle appearance).
- **SituationCmdDrawIndexedInstanced / SituationCmdDrawInstanced:** Renders the particle geometry, typically using instancing where the instance count equals
the number of particles, and the instance ID is used in the vertex shader to fetch data from the particle buffer.

#### 6.6.3 Purpose
This conceptual example should clarify the intended workflow for integrating compute-generated data into subsequent graphics rendering passes.
It emphasizes the essential role of SituationMemoryBarrier for correctness when sharing data between different pipeline types within the same command stream.
This bridges the gap between the existing separate compute and graphics examples, showing how they can be combined effectively.
</details>


<details>
<summary><h3>Frequently Asked Questions (FAQ) & Troubleshooting</h3></summary>

### 7.1 Common Initialization Failures
- **GLFW Init Failed:** Check GLFW installation, system libraries (X11 on Linux).
- **OpenGL Loader Failed:** Ensure \`GLAD\` is compiled and linked correctly when using \`SITUATION_USE_OPENGL\`.
- **Vulkan Instance/Device Failed:** Verify Vulkan SDK installation, compatible driver. Enable validation layers (\`init_info.enable_vulkan_validation = true;\`) for detailed errors.
- **Audio Device Failed:** Check system audio settings, permissions.

### 7.2 "Resource Invalid" Errors
- Occur when trying to use a resource handle (Shader, Mesh, Texture, Buffer, ComputePipeline) that hasn't been created successfully (\`id == 0\`) or has already been destroyed.

### 7.3 Performance Considerations
- Minimize state changes (binding different shaders, textures) within a single command buffer recording.
- Batch similar draw calls.
- Use \`SituationDrawTextStyled\` instead of \`SituationDrawTextSimple\` for significant text rendering.
- Profile your application to identify bottlenecks.

### 7.4 Backend-Specific Issues (OpenGL vs. Vulkan)
- OpenGL might be easier to set up initially but can have driver-specific quirks.
- Vulkan offers more explicit control and potentially better performance but has a steeper learning curve and more verbose setup.

### 7.5 Debugging Tips (Validation Layers, Error Messages)
- Always check the return value of \`SituationInit\` and resource creation functions.
- Use \`SituationGetLastErrorMsg()\` to get detailed error descriptions.
- For Vulkan, enable validation layers during development (\`init_info.enable_vulkan_validation = true;\`) to catch API misuse.
</details>
