# terminal.md - Comprehensive Documentation

This document provides a detailed overview of the `terminal.h` library, covering its features, API, and terminal emulation capabilities.

## 1. Introduction

`terminal.h` is an enhanced, single-header C library for terminal emulation. It is designed to be integrated into applications requiring a text-based user interface, providing a comprehensive solution for processing terminal input streams and rendering a visual representation of the terminal state. The library uses Raylib for rendering, input handling, and window management, making it a powerful tool for creating terminal emulators, embedded consoles, and other TUI-based applications.

### 1.1. Key Features

*   **Broad Compatibility**: Emulates a wide range of terminals, including VT52, VT100, VT220, VT320, VT420, and xterm.
*   **Modern Enhancements**: Supports modern features like true color (24-bit), Sixel graphics, advanced mouse tracking, and bracketed paste mode.
*   **Comprehensive API**: Offers a rich set of functions for terminal lifecycle management, input/output pipeline control, and feature configuration.
*   **Performance-Oriented**: Includes adaptive processing and burst mode to efficiently handle large amounts of data.
*   **Extensive Character Set Support**: Manages multiple character sets, including DEC Special Graphics, UK National, and DEC Multinational.

## 2. Compatibility

The library's compatibility is configurable, allowing developers to target specific VT levels. The supported levels are:

*   `VT_LEVEL_52`: Basic VT52 compatibility.
*   `VT_LEVEL_100`: VT100 compatibility, including basic ANSI escape sequences.
*   `VT_LEVEL_220`: VT220 features, including 8-bit controls and additional character sets.
*   `VT_LEVEL_320`: Adds support for features like User-Defined Keys (UDKs).
*   `VT_LEVEL_420`: Includes rectangular area operations.
*   `VT_LEVEL_XTERM`: A superset of the VT levels, incorporating many modern extensions found in `xterm`.

The default level is `VT_LEVEL_XTERM` for the widest feature support. The compatibility level can be changed at runtime using the `SetVTLevel()` function.

## 3. Core Concepts

The library is built around a few core concepts that work together to provide the terminal emulation.

### 3.1. The `Terminal` Structure

The state of the entire terminal is stored in a global `Terminal` structure. This structure holds the screen buffers, cursor state, current modes, and all other parameters that define the terminal's behavior.

### 3.2. Screen Buffer

The terminal maintains two screen buffers: a primary buffer and an alternate buffer. The alternate buffer is used for full-screen applications like text editors, allowing them to be closed without disrupting the command history in the primary buffer. Each cell in the buffer is an `EnhancedTermChar` structure, which stores not just the character, but also its color, attributes (bold, underline, etc.), and rendering state.

### 3.3. Input Pipeline

Incoming data from the host application is placed into an input pipeline (a circular buffer). The `UpdateTerminal()` function processes this pipeline, parsing escape sequences and updating the terminal state accordingly. This design decouples data reception from processing, which is beneficial for performance.

### 3.4. State Machine

The library uses a state machine to parse incoming byte streams. The parser can be in one of several states, such as `VT_PARSE_NORMAL`, `VT_PARSE_ESCAPE`, or `PARSE_CSI`. This allows it to correctly interpret complex, multi-byte escape sequences.

## 4. API Reference

The following is a reference for the public API of `terminal.h`.

### 4.1. Lifecycle

These functions manage the initialization and cleanup of the terminal.

*   `void InitTerminal(void)`: Initializes the terminal, setting up the screen buffers, default modes, and color palette.
*   `void CleanupTerminal(void)`: Frees all resources allocated by the terminal.

### 4.2. Main Loop

These are the core functions to be called in the application's main loop.

*   `void UpdateTerminal(void)`: Processes the input pipeline, updates timers, and handles all state changes.
*   `void DrawTerminal(void)`: Renders the current state of the terminal to the screen.

### 4.3. VT Compliance

Functions for managing the terminal's compatibility level and features.

*   `void SetVTLevel(VTLevel level)`: Sets the VT compatibility level.
*   `VTLevel GetVTLevel(void)`: Gets the current VT compatibility level.
*   `void EnableVTFeature(const char* feature, bool enable)`: Enables or disables a specific feature.
*   `bool IsVTFeatureSupported(const char* feature)`: Checks if a feature is supported at the current level.
*   `void GetDeviceAttributes(char* primary, char* secondary, size_t buffer_size)`: Retrieves the device attribute strings.

### 4.4. Input Pipeline

Functions for writing data to the terminal for processing.

*   `bool PipelineWriteChar(unsigned char ch)`: Writes a single character to the pipeline.
*   `bool PipelineWriteString(const char* str)`: Writes a null-terminated string.
*   `bool PipelineWriteFormat(const char* format, ...)`: Writes a formatted string.
*   `void ProcessPipeline(void)`: Manually processes a batch of characters from the pipeline.
*   `void ClearPipeline(void)`: Clears the input pipeline.
*   `int GetPipelineCount(void)`: Returns the number of characters in the pipeline.
*   `bool IsPipelineOverflow(void)`: Checks if the pipeline has overflowed.

### 4.5. Callbacks

Set callback functions to handle events from the terminal.

*   `void SetResponseCallback(ResponseCallback callback)`: Sets a callback for data that the terminal sends back to the host (e.g., status reports, mouse coordinates).
*   `void SetTitleCallback(TitleCallback callback)`: Sets a callback for window title changes.
*   `void SetBellCallback(BellCallback callback)`: Sets a callback for the audible/visual bell.

### 4.6. Scripting API

A set of high-level functions for easily controlling the terminal from the host application.

*   `void Script_PutChar(unsigned char ch)`: Puts a single character on the screen.
*   `void Script_Print(const char* text)`: Prints a string.
*   `void Script_Printf(const char* format, ...)`: Prints a formatted string.
*   `void Script_Cls(void)`: Clears the screen.
*   `void Script_SetColor(int fg, int bg)`: Sets the foreground and background colors.

## 5. Escape Sequences

The terminal's behavior is controlled by a stream of characters that includes printable text and non-printable control sequences. These sequences, often called "escape sequences," are used to perform actions like moving the cursor, changing colors, and clearing the screen. The library supports several types of control sequences.

### 5.1. Control Sequence Introducer (CSI)

CSI sequences are the most common type and are used for a wide range of functions. They begin with `ESC [` and are followed by parameters and a final character that specifies the command.

*   **Cursor Movement**:
    *   `ESC [ {n} A`: Move cursor up `n` lines.
    *   `ESC [ {n} B`: Move cursor down `n` lines.
    *   `ESC [ {n} C`: Move cursor forward `n` columns.
    *   `ESC [ {n} D`: Move cursor back `n` columns.
    *   `ESC [ {y} ; {x} H`: Move cursor to row `y`, column `x`.
*   **Screen Clearing**:
    *   `ESC [ 2 J`: Clear the entire screen.
    *   `ESC [ K`: Clear from the cursor to the end of the line.
*   **Text Attributes (SGR - Select Graphic Rendition)**:
    *   `ESC [ {n} m`: Set a text attribute. For example, `ESC [ 1 m` for bold, `ESC [ 31 m` for red text.

### 5.2. Operating System Command (OSC)

OSC sequences are used to interact with the "operating system" of the terminal, which in this case refers to the terminal emulator application itself. They begin with `ESC ]` and are typically used for tasks like setting the window title.

*   `ESC ] 0 ; {title} ST`: Set the window title. (`ST` is the string terminator `ESC \`).

### 5.3. Device Control String (DCS)

DCS sequences are used for more complex interactions and device-specific features. They begin with `ESC P`. This library uses them for features like Sixel graphics and user-defined keys.

*   `ESC P ... ST`: A device control string, with the content depending on the specific feature.

## 6. Color System

`terminal.h` supports a flexible and powerful color system, allowing for a wide range of visual expression.

### 6.1. 16-Color ANSI

This is the standard, most widely supported color mode. It includes 8 standard colors and their "bright" versions.

*   **Foreground**: `ESC [ 30-37 m` for standard colors (Black to White), `ESC [ 90-97 m` for bright colors.
*   **Background**: `ESC [ 40-47 m` for standard colors, `ESC [ 100-107 m` for bright colors.

### 6.2. 256-Color Indexed

This mode provides a palette of 256 colors.

*   **Foreground**: `ESC [ 38 ; 5 ; {n} m`, where `{n}` is a color index from 0 to 255.
*   **Background**: `ESC [ 48 ; 5 ; {n} m`, where `{n}` is a color index from 0 to 255.

### 6.3. 24-bit True Color

For the highest color fidelity, the library supports 24-bit true color, allowing for millions of colors.

*   **Foreground**: `ESC [ 38 ; 2 ; {r} ; {g} ; {b} m`, where `{r}`, `{g}`, and `{b}` are red, green, and blue values from 0 to 255.
*   **Background**: `ESC [ 48 ; 2 ; {r} ; {g} ; {b} m`, where `{r}`, `{g}`, and `{b}` are red, green, and blue values from 0 to 255.

## 7. Mouse Tracking

The library provides several modes for mouse tracking, allowing applications to receive mouse events.

*   **X10 Compatibility Mode**: The simplest mouse protocol. Enabled with `CSI ? 9 h`.
*   **VT200 Mode**: Reports button presses. Enabled with `CSI ? 1000 h`.
*   **Button-Event Tracking**: Reports button presses and releases. Enabled with `CSI ? 1002 h`.
*   **Any-Event Tracking**: Reports button events and mouse movement. Enabled with `CSI ? 1003 h`.
*   **SGR (Set Graphics Rendition) Mode**: An extended mode that provides more detailed reports, including support for scroll wheel events and modifier keys. Enabled with `CSI ? 1006 h`.

When mouse tracking is enabled, mouse events are sent to the host application via the response callback.

## 8. Character Sets

`terminal.h` supports the designation and invocation of different character sets, a key feature of VT-series terminals. The terminal manages four character sets, designated G0 through G3, and can map them to the GL (left-side, for 7-bit characters) and GR (right-side, for 8-bit characters) slots.

*   **Supported Sets**:
    *   `CHARSET_ASCII`: Standard US ASCII.
    *   `CHARSET_DEC_SPECIAL`: DEC Special Graphics, used for line drawing.
    *   `CHARSET_UK`: UK National character set.
    *   `CHARSET_DEC_MULTINATIONAL`: DEC Multinational Character Set (MCS).

*   **Designation**:
    *   `ESC ( B`: Designate G0 as ASCII.
    *   `ESC ( 0`: Designate G0 as DEC Special Graphics.
    *   The designator `(` can be `)`, `*`, or `+` for G1, G2, and G3 respectively.

## 9. Sixel Graphics

The library includes support for Sixel graphics, a bitmap graphics format for terminals. Sixel data is embedded in a DCS (Device Control String) sequence.

*   **Sequence**: `ESC P {params} q {sixel data} ST`

When the terminal receives a Sixel sequence, it parses the data and renders the resulting image onto the screen at the current cursor position. This feature is available when the terminal is set to a compatibility level of `VT_LEVEL_320` or higher.
