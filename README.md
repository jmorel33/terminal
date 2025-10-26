 terminal.h - Enhanced Terminal Emulation Library
(c) 2025 Jacques Morel

<details>
<summary>Table of Contents</summary>

1.  [Description](#description)
2.  [Key Features](#key-features)
3.  [How It Works](#how-it-works)
    1.  [3.1. Main Loop and Initialization](#31-main-loop-and-initialization)
    2.  [3.2. Input Pipeline and Character Processing](#32-input-pipeline-and-character-processing)
    3.  [3.3. Escape Sequence Parsing](#33-escape-sequence-parsing)
    4.  [3.4. Keyboard and Mouse Handling](#34-keyboard-and-mouse-handling)
    5.  [3.5. Rendering](#35-rendering)
    6.  [3.6. Callbacks](#36-callbacks)
4.  [How to Use](#how-to-use)
    1.  [4.1. Basic Setup](#41-basic-setup)
    2.  [4.2. Sending Data to the Terminal (Simulating Host Output)](#42-sending-data-to-the-terminal-simulating-host-output)
    3.  [4.3. Receiving Responses and Key Events from the Terminal](#43-receiving-responses-and-key-events-from-the-terminal)
    4.  [4.4. Configuring Terminal Behavior](#44-configuring-terminal-behavior)
    5.  [4.5. Advanced Features](#45-advanced-features)
    6.  [4.6. Window and Title Management](#46-window-and-title-management)
    7.  [4.7. Diagnostics and Testing](#47-diagnostics-and-testing)
    8.  [4.8. Scripting API](#48-scripting-api)
5.  [Configuration Constants](#configuration-constants)
6.  [Key Data Structures](#key-data-structures)
7.  [Implementation Model](#implementation-model)
8.  [Dependencies](#dependencies)
9.  [License](#license)

</details>

## Description

This library provides a comprehensive terminal emulation solution, aiming for compatibility with VT52, VT100, VT220, VT320, VT420, and xterm standards, while also incorporating modern features like true color support, Sixel graphics, advanced mouse tracking, and bracketed paste mode. It is designed to be integrated into applications that require a text-based terminal interface, using the Raylib library for rendering, input, and window management.

The library processes a stream of input characters (typically from a host application or PTY) and updates an internal screen buffer. This buffer, representing the terminal display, is then rendered to the screen. It handles a wide range of escape sequences to control cursor movement, text attributes, colors, screen clearing, scrolling, and various terminal modes.

## Key Features

-   VT52, VT100, VT220, VT320, VT420, and xterm compatibility levels.
-   256-color and 24-bit True Color (RGB) support for text.
-   Advanced cursor styling (block, underline, bar) with blink options.
-   Multiple character set support (ASCII, DEC Special Graphics, UK, DEC Multinational).
-   Comprehensive mouse tracking modes (X10, VT200, Button Event, Any Event, SGR).
-   Alternate screen buffer implementation.
-   Scrolling regions and margins (including VT420 left/right margins).
-   Bracketed paste mode (CSI ? 2004 h/l).
-   Sixel graphics display (basic support).
-   Soft font downloading (DECDLD - basic framework).
-   User-Defined Keys (DECUDK).
-   Window title and icon name control via OSC sequences.
-   Rich set of text attributes (bold, faint, italic, underline, blink, reverse, etc.).
-   Built-in bitmap font (8x16, CP437 based).
-   Input pipeline with performance management options.
-   Callback system for responses to host, title changes, and bell.
-   Diagnostic and testing utilities.

## How It Works

The terminal operates around a central `Terminal` structure that holds the entire state, including screen buffers, cursor information, modes, and parsing state.

### 3.1. Main Loop and Initialization

-   `InitTerminal()`: Sets up the default terminal state: screen buffers (arrays of `EnhancedTermChar`), cursor, modes (DECModes, ANSIModes), color palettes, character sets (`CharsetState`), tab stops, keyboard (`VTKeyboard`), performance settings, and initializes the Raylib font texture.
-   `UpdateTerminal()`: This is the main update tick. It calls `ProcessPipeline()` to handle incoming data, updates cursor and text blink timers, and flushes any responses queued for the host application via the `ResponseCallback`.
-   `DrawTerminal()`: Renders the current state of the active screen buffer to the Raylib window. It iterates through each `EnhancedTermChar` cell, resolves its foreground and background colors (from `ExtendedColor`), applies attributes (bold, underline, etc.), and draws the character glyph from the `font_texture`. It also handles drawing Sixel graphics and the cursor.

### 3.2. Input Pipeline and Character Processing

-   Data intended for the terminal (e.g., from a PTY or application simulating a host) is fed into `terminal.input_pipeline` using functions like `PipelineWriteChar()`, `PipelineWriteString()`, `PipelineWriteFormat()`.
-   `ProcessPipeline()` (called by `UpdateTerminal`) consumes characters from this pipeline. The number of characters processed per frame can be tuned for performance.
-   Each character is passed to `ProcessChar()`, which acts as a state machine dispatcher based on `terminal.parse_state`.
-   `ProcessNormalChar()` handles printable characters by translating them through the active character set, applying current attributes, placing them on the screen (considering auto-wrap and insert mode), and handling C0 control codes (like BEL, BS, LF, CR, ESC) by delegating to `ProcessControlChar()`.

### 3.3. Escape Sequence Parsing

-   When an ESC (0x1B) is encountered, `terminal.parse_state` changes.
-   `ProcessEscapeChar()` handles the character immediately following ESC. For example, '[' transitions to `PARSE_CSI`, ']' to `PARSE_OSC`, 'P' to `PARSE_DCS`. Other characters might trigger single-character escape sequences (e.g., ESC D for IND).
-   `ProcessCSIChar()` accumulates parameters (numbers separated by ';') into `terminal.escape_buffer` until a final command character (A-Z, a-z, @, etc.) is received. `ParseCSIParams()` then converts the buffered string into numeric parameters, and `ExecuteCSICommand()` dispatches to the specific handler for
    that CSI sequence (e.g., `ExecuteCUU()` for Cursor Up `CSI n A`).
-   `ProcessOSCChar()`, `ProcessDCSChar()`, etc., buffer their respective string data until a terminator (BEL or ST `ESC \`) is found, then call their
    respective `Execute...Command()` functions.

### 3.4. Keyboard and Mouse Handling

-   `UpdateVTKeyboard()` (called by the application in its main loop) uses Raylib's input functions to detect key presses and releases.
-   It considers modifier keys (Ctrl, Shift, Alt) and terminal modes like DECCKM (Application Cursor Keys) and DECKPAM/DECKPNM (Application Keypad).
-   `GenerateVTSequence()` converts these Raylib key events into the appropriate VT escape sequences (e.g., Up Arrow -> `ESC [ A` or `ESC O A`). These are
    buffered in `vt_keyboard.buffer`.
-   `GetVTKeyEvent()` allows the application to retrieve these processed key events. A typical terminal application would send these sequences to the connected host.
-   `UpdateMouse()` (also called by the application) uses Raylib's mouse functions. Based on the active `MouseTrackingMode` (e.g., X10, SGR), it generates VT mouse
    report sequences and enqueues them into `terminal.input_pipeline` for processing, or sends them via `ResponseCallback` if configured.

### 3.5. Rendering

-   `DrawTerminal()` is responsible for visualizing the terminal state.
-   It iterates through the `terminal.screen` buffer (a 2D array of `EnhancedTermChar`).
-   For each cell, it resolves `fg_color` and `bg_color` (handling palette-indexed, 256-color, and RGB true color via `ExtendedColor`), applies attributes like
    `reverse`, `bold` (often by selecting a bright color variant for standard ANSI colors), `faint`, `blink` (by alternating visibility based on `terminal.text_blink_state`).
-   It draws the cell background, then the character glyph (looked up in `font_texture`, a pre-rendered bitmap of CP437 characters). Text decorations like `underline`,
    `strikethrough`, and `overline` are drawn as lines.
-   Sixel graphics (`terminal.sixel`) are overlaid if active.
-   The `terminal.cursor` is drawn based on its `shape`, `visible`, and `blink_state`.

### 3.6. Callbacks

-   `ResponseCallback`: The terminal uses this to send data back to the "host" application (e.g., answers to Device Status Report `DSR`, Device Attributes `DA`
    requests, or some mouse reports). Data is queued via `QueueResponse()` and flushed during `UpdateTerminal()`.
-   `TitleCallback`: Invoked when an OSC sequence changes the window or icon title, allowing the parent GUI application to update the actual OS window.
-   `BellCallback`: Called when a BEL (0x07) control character is processed. If not set, a visual bell effect might be triggered.

## How to Use

This library is designed as a single-header library.

### 4.1. Basic Setup

-   In one of your C files, define `TERMINAL_IMPLEMENTATION` before including "terminal.h":
    ```c
    #define TERMINAL_IMPLEMENTATION
    #include "terminal.h"
    ```
-   In other files, just `#include "terminal.h"`.
-   Initialize Raylib: `InitWindow(...)`.
-   Initialize the terminal: `InitTerminal()`.
-   Set target FPS for Raylib: `SetTargetFPS(60)`.
-   Optionally, set terminal performance: `SetPipelineTargetFPS(60)`, `SetPipelineTimeBudget(0.5)`.
-   In your main application loop:
    ```c
    // Process host inputs and terminal updates
    UpdateVTKeyboard(); // Translates Raylib key events to VT sequences
    UpdateMouse();      // Translates Raylib mouse events to VT sequences
    UpdateTerminal();   // Processes pipeline, timers, and callbacks

    // Render the terminal
    BeginDrawing();
        ClearBackground(BLACK); // Or your desired background color
        DrawTerminal();
    EndDrawing();
    ```
-   On exit: `CleanupTerminal()` and `CloseWindow()`.

### 4.2. Sending Data to the Terminal (Simulating Host Output)

-   `PipelineWriteChar(unsigned char ch)`: Send a single byte.
-   `PipelineWriteString(const char* str)`: Send a null-terminated string.
-   `PipelineWriteFormat(const char* format, ...)`: Send a printf-style formatted string.

These functions add data to an internal buffer, which `UpdateTerminal()` processes.

### 4.3. Receiving Responses and Key Events from the Terminal

-   `SetResponseCallback(ResponseCallback callback)`: Register a function like `void my_response_handler(const char* response, int length)` to receive data
    that the terminal emulator needs to send back (e.g., status reports, DA).
-   `GetVTKeyEvent(VTKeyEvent* event)`: Retrieve a fully processed `VTKeyEvent` from the keyboard buffer. The `event->sequence` field contains the string
    to be sent to the host or processed by a local application.

### 4.4. Configuring Terminal Behavior

-   **VT Compliance Level:**
    -   `SetVTLevel(VTLevel level)` (e.g., `VT_LEVEL_XTERM`, `VT_LEVEL_420`).
    -   `GetVTLevel()`.
-   **Terminal Modes (DEC/ANSI):**
    -   `SetTerminalMode(const char* mode_name, bool enable)` (e.g., "application_cursor"). Most modes are set/reset via standard VT escape sequences (e.g., `CSI ? 1 h/l`
        for DECCKM, `CSI ? 25 h/l` for cursor visibility).
-   **Cursor Customization:**
    -   `SetCursorShape(CursorShape shape)` (e.g., `CURSOR_BLOCK_BLINK`).
    -   `SetCursorColor(ExtendedColor color)`.
    -   Also settable via DECSCUSR sequence (`CSI Ps SP q`).
-   **Mouse Tracking:**
    -   `SetMouseTracking(MouseTrackingMode mode)` (e.g., `MOUSE_TRACKING_SGR`).
    -   Enable/disable features like focus reporting: `EnableMouseFeature("focus", true)`.
    -   Also settable via `CSI ? Pn h/l` (e.g., `CSI ? 1000 h`, `CSI ? 1006 h`).
-   **Character Sets:**
    -   `SelectCharacterSet(int gset, CharacterSet charset)` (designates G0-G3).
    -   `SetCharacterSet(CharacterSet charset)` (sets current GL, usually G0).
    -   Also settable via ESC sequences (`ESC ( C`, `ESC ) C`, etc.).
-   **Tab Stops:**
    -   `SetTabStop(int column)`, `ClearTabStop(int column)`, `ClearAllTabStops()`.
    -   `NextTabStop(int current_col)`, `PreviousTabStop(int current_col)`.

### 4.5. Advanced Features

-   **Sixel Graphics:** Enabled if VT level is VT320+ or XTERM. Sixel data is sent via DCS: `ESC P Pa;Pb;Ph;Pv q <sixel_data> ST`.
-   **Soft Fonts (DECDLD):** Enabled if VT level is VT220+. Loaded via DCS. `LoadSoftFont(...)`, `SelectSoftFont(...)`.
-   **Programmable Keys (DECUDK):** Enabled if VT level is VT320+. `DefineFunctionKey(int fkey_num, const char* seq)` for F1-F24.
-   **Bracketed Paste:**
    -   `EnableBracketedPaste(bool enable)`. Controlled by `CSI ? 2004 h/l`. `IsBracketedPasteActive()`, `ProcessPasteData(...)`.

### 4.6. Window and Title Management

-   `VTSetWindowTitle(const char* title)`, `SetIconTitle(const char* title)`.
-   `GetWindowTitle()`, `GetIconTitle()`.
-   `SetTitleCallback(TitleCallback callback)`: Register `void my_title_handler(const char* title, bool is_icon)` to be notified of title changes (e.g., from OSC sequences `ESC ]0;...ST`).

### 4.7. Diagnostics and Testing

-   `EnableDebugMode(bool enable)`: Toggles verbose logging of unsupported/unknown sequences.
-   `GetTerminalStatus()`: Returns `TerminalStatus` with buffer usage, etc.
-   `RunVTTest(const char* test_name)`: Executes predefined test sequences ("cursor", "all").
-   `ShowTerminalInfo()`: Prints current terminal state to the terminal screen.

### 4.8. Scripting API

-   A set of `Script_` functions provide simple wrappers for common operations:
    `Script_PutChar()`, `Script_Print()`, `Script_Printf()`, `Script_Cls()`, `Script_SetColor()`.

## Configuration Constants

The library uses several compile-time constants defined at the beginning of this
file (e.g., `DEFAULT_TERM_WIDTH`, `DEFAULT_TERM_HEIGHT`, `DEFAULT_CHAR_WIDTH`, `DEFAULT_CHAR_HEIGHT`, `DEFAULT_WINDOW_SCALE`,
`OUTPUT_BUFFER_SIZE`) to set default terminal dimensions, font size, rendering scale,
and buffer sizes. These can be modified before compilation.

## Key Data Structures

-   `Terminal`: The main struct encapsulating the entire terminal state. Most API functions operate on a global instance of this structure.
-   `EnhancedTermChar`: Represents a single character cell on the screen, including its Unicode codepoint, foreground/background `ExtendedColor`, and text attributes
    (bold, italic, underline, etc.).
-   `VTParseState`: Enum tracking the current state of the escape sequence parser.
-   `VTLevel`: Enum defining the VT compatibility level (e.g., VT100, VT220, VT420, XTERM).
-   `ExtendedColor`: Struct for representing colors, supporting both standard ANSI palette indices and 24-bit RGB true color values.
-   `VTKeyboard`: Struct managing keyboard input state, modifier keys, application modes (cursor keys, keypad), and a buffer for processed `VTKeyEvent`s.
-   `DECModes`, `ANSIModes`: Structs containing boolean flags for various DEC private and ANSI standard modes.
-   `EnhancedCursor`: Struct detailing cursor properties like position, visibility, shape (`CursorShape`), and blink status.
-   `CharsetState`: Manages the G0-G3 character sets and the active GL/GR mappings.
-   `MouseTrackingMode`: Enum for the various mouse reporting protocols.
-   `SixelGraphics`, `SoftFont`, `BracketedPaste`, `ProgrammableKeys`: Structs for managing state related to these advanced features.

## Implementation Model

This is a single-header library. To include the implementation, define `TERMINAL_IMPLEMENTATION` in exactly one C source file before including this header:
```c
#define TERMINAL_IMPLEMENTATION
#include "terminal.h"
```
Other source files can simply include "terminal.h" for declarations.

## Dependencies

-   Raylib (version 4.0 or later recommended): Used for window creation, graphics rendering, input handling (keyboard/mouse), and font texture management.
-   Standard C11 libraries: `stdio.h`, `stdlib.h`, `string.h`, `stdbool.h`, `ctype.h`, `stdarg.h`, `math.h`, `time.h`.

## License

MIT License
