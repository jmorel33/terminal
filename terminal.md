# terminal.h - Technical Reference Manual

**(c) 2025 Jacques Morel**

This document provides an exhaustive technical reference for `terminal.h`, an enhanced single-header terminal emulation library for C. It is intended for developers integrating the library into their applications and those who need a deep understanding of its capabilities, supported protocols, and internal architecture.

<details>
<summary>Table of Contents</summary>

1.  [Overview](#1-overview)
    *   1.1. [Description](#11-description)
    *   1.2. [Key Features](#12-key-features)
    *   1.3. [Architectural Deep Dive](#13-architectural-deep-dive)
        *   1.3.1. [Core Philosophy and The `Terminal` Struct](#131-core-philosophy-and-the-terminal-struct)
        *   1.3.2. [The Input Pipeline](#132-the-input-pipeline)
        *   1.3.3. [The Processing Loop and State Machine](#133-the-processing-loop-and-state-machine)
        *   1.3.4. [The Screen Buffer](#134-the-screen-buffer)
        *   1.3.5. [The Rendering Engine](#135-the-rendering-engine)
        *   1.3.6. [The Output Pipeline (Response System)](#136-the-output-pipeline-response-system)
2.  [Compliance and Emulation Levels](#2-compliance-and-emulation-levels)
    *   2.1. [Setting the Compliance Level](#21-setting-the-compliance-level)
    *   2.2. [Feature Breakdown by `VTLevel`](#22-feature-breakdown-by-vtlevel)
        *   2.2.1. [`VT_LEVEL_52`](#221-vt_level_52)
        *   2.2.2. [`VT_LEVEL_100`](#222-vt_level_100)
        *   2.2.3. [`VT_LEVEL_220`](#223-vt_level_220)
        *   2.2.4. [`VT_LEVEL_320`](#224-vt_level_320)
        *   2.2.5. [`VT_LEVEL_420`](#225-vt_level_420)
        *   2.2.6. [`VT_LEVEL_XTERM` (Default)](#226-vt_level_xterm-default)
3.  [Control and Escape Sequences](#3-control-and-escape-sequences)
    *   3.1. [C0 Control Codes](#31-c0-control-codes)
    *   3.2. [C1 Control Codes (7-bit and 8-bit)](#32-c1-control-codes-7-bit-and-8-bit)
    *   3.3. [CSI - Control Sequence Introducer (`ESC [`)](#33-csi---control-sequence-introducer-esc-)
        *   3.3.1. [Cursor Movement](#331-cursor-movement)
        *   3.3.2. [Erasing](#332-erasing)
        *   3.3.3. [Scrolling](#333-scrolling)
        *   3.3.4. [Select Graphic Rendition (SGR)](#334-select-graphic-rendition-sgr)
        *   3.3.5. [Mode Setting](#335-mode-setting)
        *   3.3.6. [Status Reports](#336-status-reports)
    *   3.4. [OSC - Operating System Command (`ESC ]`)](#34-osc---operating-system-command-esc--)
    *   3.5. [DCS - Device Control String (`ESC P`)](#35-dcs---device-control-string-esc-p)
    *   3.6. [Other Escape Sequences](#36-other-escape-sequences)
    *   3.7. [VT52 Mode Sequences](#37-vt52-mode-sequences)
4.  [Key Features In-Depth](#4-key-features-in-depth)
    *   4.1. [Color Support](#41-color-support)
    *   4.2. [Mouse Tracking](#42-mouse-tracking)
    *   4.3. [Character Sets](#43-character-sets)
    *   4.4. [Screen and Buffer Management](#44-screen-and-buffer-management)
    *   4.5. [Sixel Graphics](#45-sixel-graphics)
    *   4.6. [Bracketed Paste Mode](#46-bracketed-paste-mode)
5.  [API Reference](#5-api-reference)
    *   5.1. [Lifecycle Functions](#51-lifecycle-functions)
    *   5.2. [Host Input (Pipeline) Management](#52-host-input-pipeline-management)
    *   5.3. [Keyboard and Mouse Output](#53-keyboard-and-mouse-output)
    *   5.4. [Configuration and Mode Setting](#54-configuration-and-mode-setting)
    *   5.5. [Callbacks](#55-callbacks)
    *   5.6. [Diagnostics and Testing](#56-diagnostics-and-testing)
    *   5.7. [Advanced Control](#57-advanced-control)
6.  [Internal Operations and Data Flow](#6-internal-operations-and-data-flow)
    *   6.1. [Stage 1: Ingestion](#61-stage-1-ingestion)
    *   6.2. [Stage 2: Consumption and Parsing](#62-stage-2-consumption-and-parsing)
    *   6.3. [Stage 3: Character Processing and Screen Buffer Update](#63-stage-3-character-processing-and-screen-buffer-update)
    *   6.4. [Stage 4: Rendering](#64-stage-4-rendering)
    *   6.5. [Stage 5: Keyboard Input Processing](#65-stage-5-keyboard-input-processing)
7.  [Data Structures Reference](#7-data-structures-reference)
    *   7.1. [Enums](#71-enums)
        *   7.1.1. [`VTLevel`](#711-vtlevel)
        *   7.1.2. [`VTParseState`](#712-vtparsestate)
        *   7.1.3. [`MouseTrackingMode`](#713-mousetrackingmode)
        *   7.1.4. [`CursorShape`](#714-cursorshape)
        *   7.1.5. [`CharacterSet`](#715-characterset)
    *   7.2. [Core Structs](#72-core-structs)
        *   7.2.1. [`Terminal`](#721-terminal)
        *   7.2.2. [`EnhancedTermChar`](#722-enhancedtermchar)
        *   7.2.3. [`ExtendedColor`](#723-extendedcolor)
        *   7.2.4. [`VTKeyEvent`](#724-vtkeyevent)
8.  [Configuration Constants](#8-configuration-constants)
9.  [License](#9-license)

</details>

---

## 1. Overview

### 1.1. Description

`terminal.h` is a comprehensive, single-header C library for terminal emulation. It is designed for integration into applications requiring a text-based user interface, such as embedded systems, remote access clients, or development tools. The library uses [Raylib](https://www.raylib.com/) for rendering, windowing, and input handling, providing a complete solution out of the box.

The library emulates a wide range of historical and modern terminal standards, from the DEC VT52 to contemporary xterm extensions. It processes a stream of bytes, interprets control codes and escape sequences, and maintains an internal model of the terminal screen, which is then rendered to the display.

### 1.2. Key Features

-   **Broad Compatibility:** Emulates VT52, VT100, VT220, VT320, VT420, and xterm.
-   **Advanced Color Support:**
    -   16-color ANSI (standard and bright).
    -   256-color indexed palette.
    -   24-bit RGB True Color.
-   **Modern UI Features:**
    -   Advanced mouse tracking (X10, VT200, SGR, and more).
    -   Bracketed paste mode (`CSI ? 2004 h/l`).
    -   Sixel graphics rendering.
    -   Customizable cursor styles (block, underline, bar, with blink).
-   **Comprehensive Terminal Emulation:**
    -   Alternate screen buffer.
    -   Scrolling regions and margins (including vertical and horizontal).
    -   Character sets (ASCII, DEC Special Graphics, NRCS).
    -   Soft fonts (DECDLD) and User-Defined Keys (DECUDK).
-   **Performance and Diagnostics:**
    -   Tunable input pipeline for performance management.
    -   Callback system for host responses, title changes, and bell.
    -   Debugging utilities for logging unsupported sequences.

### 1.3. Architectural Deep Dive

This section provides a more detailed examination of the library's internal components and data flow, expanding on the brief overview. Understanding this architecture is key to extending the library or diagnosing complex emulation issues.

#### 1.3.1. Core Philosophy and The `Terminal` Struct

The library's design is centered on a single, comprehensive data structure: the `Terminal` struct. This monolithic struct, defined in `terminal.h`, encapsulates the entire state of the emulated device. This includes everything from screen buffers and cursor state to parsing buffers, mode flags (`DECModes`, `ANSIModes`), and color palettes. This centralized approach simplifies state management, debugging, and serialization (if needed), as the entire terminal's condition can be understood by inspecting this one structure.

#### 1.3.2. The Input Pipeline

The terminal is a consumer of sequential character data. The entry point for all incoming data from a host application (e.g., a shell, a remote server) is the input pipeline.

-   **Mechanism:** A fixed-size circular buffer (`input_pipeline`) of `unsigned char`.
-   **Ingestion:** Host applications use `PipelineWriteChar()`, `PipelineWriteString()`, or `PipelineWriteFormat()` to append data to this buffer. These functions are thread-safe in principle (if a lock were added) but are intended for use from the main application thread.
-   **Flow Control:** The pipeline has a fixed size (`16384` bytes). If the host writes data faster than the terminal can process it, an overflow flag (`pipeline_overflow`) is set. This allows the host application to detect the overflow and potentially pause data transmission.

#### 1.3.3. The Processing Loop and State Machine

The heart of the emulation is the main processing loop within `UpdateTerminal()`, which drives a sophisticated state machine.

-   **Consumption:** `UpdateTerminal()` calls `ProcessPipeline()`, which consumes a tunable number of characters from the input pipeline each frame. This prevents the emulation from freezing the application when large amounts of data are received. The number of characters processed can be adjusted for performance (`VTperformance` struct).
-   **Parsing:** Each character is fed into `ProcessChar()`, which acts as a dispatcher based on the current `VTParseState`.
    -   `VT_PARSE_NORMAL`: In the default state, printable characters are sent to the screen, and control characters (like `ESC` or C0 codes) change the parser's state.
    -   `VT_PARSE_ESCAPE`: After an `ESC` (`0x1B`) is received, the parser enters this state, waiting for the next character to determine the type of sequence (e.g., `[` for CSI, `]` for OSC).
    -   `PARSE_CSI`, `PARSE_OSC`, `PARSE_DCS`, etc.: In these states, the parser accumulates parameters and intermediate bytes into `escape_buffer` until a final character (terminator) is received.
    -   **Execution:** Once a sequence is complete, a corresponding `Execute...()` function is called (e.g., `ExecuteCSICommand`, `ExecuteOSCCommand`). `ExecuteCSICommand` uses a highly efficient computed-goto dispatch table to jump directly to the handler for the specific command (`ExecuteCUU`, `ExecuteED`, etc.), minimizing lookup overhead.

#### 1.3.4. The Screen Buffer

The visual state of the terminal is stored in one of two screen buffers, both of which are 2D arrays of `EnhancedTermChar`.

-   **`EnhancedTermChar`:** This crucial struct represents a single character cell on the screen. It goes far beyond a simple `char`, storing:
    -   The Unicode codepoint (`ch`).
    -   Foreground and background colors (`ExtendedColor`), which can be an indexed palette color or a 24-bit RGB value.
    -   A comprehensive set of boolean flags for attributes like `bold`, `italic`, `underline`, `blink`, `reverse`, `strikethrough`, `conceal`, and more.
    -   Flags for DEC special modes like double-width or double-height characters (currently unsupported).
-   **Primary vs. Alternate Buffer:** The terminal maintains `screen` and `alt_screen`. Applications like `vim` or `less` switch to the alternate buffer (`CSI ?1049 h`) to create a temporary full-screen interface. When they exit, they switch back (`CSI ?1049 l`), restoring the original screen content and scrollback.

#### 1.3.5. The Rendering Engine

The `DrawTerminal()` function is responsible for translating the in-memory screen buffer into pixels on the screen, using Raylib for all drawing operations.

-   **Iteration:** The function iterates over every cell of the active screen buffer.
-   **Attribute Resolution:** For each cell, it resolves all attributes:
    -   It translates the `ExtendedColor` into a Raylib `Color`.
    -   It applies `bold` (often by selecting a bright color variant for the standard 16 ANSI colors).
    -   It handles `reverse` video by swapping the resolved foreground and background colors.
-   **Drawing:**
    1.  The background rectangle of the cell is drawn first with the final resolved background color.
    2.  The character glyph is looked up in a pre-rendered font atlas (`font_texture`) and drawn with the final resolved foreground color.
    3.  Decorative lines for `underline`, `strikethrough`, and `overline` are drawn on top.
    4.  The cursor is drawn last, according to its shape, visibility, and blink state.

#### 1.3.6. The Output Pipeline (Response System)

The terminal needs to send data back to the host in response to certain queries or events. This is handled by the output pipeline, or response system.

-   **Mechanism:** When the terminal needs to send a response (e.g., a cursor position report `CSI {row};{col} R`), it doesn't send it immediately. Instead, it queues the response string into an `answerback_buffer`.
-   **Events:** The following events generate responses:
    -   **User Input:** Keystrokes (`UpdateVTKeyboard()`) and mouse events (`UpdateMouse()`) are translated into the appropriate VT sequences and queued.
    -   **Status Reports:** Commands like `DSR` (Device Status Report) or `DA` (Device Attributes) queue their predefined response strings.
-   **Callback:** The `UpdateTerminal()` function checks if there is data in the response buffer. If so, it invokes the `ResponseCallback` function pointer, passing the buffered data to the host application. It is the host application's responsibility to set this callback and handle the data (e.g., by sending it over a serial or network connection).

---

## 2. Compliance and Emulation Levels

The library's behavior can be tailored to match historical and modern terminal standards by setting a compliance level. This not only changes the feature set but also alters the terminal's identity as reported to host applications.

### 2.1. Setting the Compliance Level

The primary function for this is `void SetVTLevel(VTLevel level);`. Setting a level enables all features of that level and all preceding levels.

### 2.2. Feature Breakdown by `VTLevel`

#### 2.2.1. `VT_LEVEL_52`
This is the most basic level, emulating the DEC VT52.
-   **Features:**
    -   Simple cursor movement (`ESC A`, `ESC B`, `ESC C`, `ESC D`).
    -   Direct cursor addressing (`ESC Y r c`).
    -   Basic erasing (`ESC J`, `ESC K`).
    -   Alternate keypad mode.
    -   Simple identification sequence (`ESC Z`).
-   **Limitations:** No ANSI CSI sequences, no scrolling regions, no advanced attributes (bold, underline, etc.), no color.

#### 2.2.2. `VT_LEVEL_100`
The industry-defining standard that introduced ANSI escape sequences.
-   **Features:**
    -   All VT52 features (when in VT52 mode, `ESC <`).
    -   **CSI Sequences:** The full range of `ESC [` commands for cursor control, erasing, etc.
    -   **SGR Attributes:** Basic graphic rendition (bold, underline, reverse, blink).
    -   **Scrolling Region:** `DECSTBM` (`CSI Pt;Pb r`) for defining a scrollable window.
    -   **Character Sets:** Support for DEC Special Graphics (line drawing).
    -   **Modes:** Auto-wrap (`DECAWM`), Application Cursor Keys (`DECCKM`).
-   **Limitations:** No color support beyond basic SGR attributes, no mouse tracking, no soft fonts.

#### 2.2.3. `VT_LEVEL_220`
An evolution of the VT100, adding more international and customization features.
-   **Features:**
    -   All VT100 features.
    -   **Character Sets:** Adds DEC Multinational Character Set (MCS) and National Replacement Character Sets (NRCS).
    -   **Soft Fonts:** Support for Downloadable Fonts (`DECDLD`).
    -   **Function Keys:** User-Defined Keys (`DECUDK`) become available.
    -   **C1 Controls:** Full support for 7-bit and 8-bit C1 control codes.
-   **Limitations:** No Sixel graphics, no rectangular area operations.

#### 2.2.4. `VT_LEVEL_320`
This level primarily adds raster graphics capabilities.
-   **Features:**
    -   All VT220 features.
    -   **Sixel Graphics:** The ability to render bitmap graphics using DCS Sixel sequences.

#### 2.2.5. `VT_LEVEL_420`
Adds more sophisticated text and area manipulation features.
-   **Features:**
    -   All VT320 features.
    -   **Left/Right Margins:** `DECSLRM` (`CSI ? Pl;Pr s`) for horizontal scrolling regions.
    -   **Rectangular Area Operations:** Commands like `DECCRA` (Copy), `DECERA` (Erase), and `DECFRA` (Fill) for manipulating rectangular blocks of text.

#### 2.2.6. `VT_LEVEL_XTERM` (Default)
Emulates a modern `xterm` terminal, which is the de facto standard for terminal emulators. This level includes a vast number of extensions built on top of the DEC standards.
-   **Features:**
    -   All VT420 features.
    -   **Color Support:**
        -   256-color palette (`CSI 38;5;Pn m`).
        -   24-bit True Color (`CSI 38;2;R;G;B m`).
    -   **Advanced Mouse Tracking:**
        -   VT200 mouse tracking, button-event, and any-event modes.
        -   SGR extended mouse reporting for higher precision.
        -   Focus In/Out event reporting.
    -   **Window Manipulation:** OSC sequences for setting window/icon titles.
    -   **Bracketed Paste Mode:** Protects shells from accidentally executing pasted code.
    -   **Alternate Screen Buffer:** The more robust `CSI ?1049 h/l` variant, which includes saving and restoring the cursor position.
    -   Numerous other SGR attributes (`italic`, `strikethrough`, etc.) and CSI sequences.

---

## 3. Control and Escape Sequences

The terminal's behavior is controlled by a stream of characters, including printable text and non-printable control sequences.

### 3.1. C0 Control Codes

These are single-byte codes in the range `0x00-0x1F`.

| Code | Hex | Name | Description                                                  |
| :--- | :-- | :--- | :----------------------------------------------------------- |
| `NUL`| 0x00| Null | Ignored.                                                     |
| `ENQ`| 0x05| Enquiry| Triggers the answerback message.                             |
| `BEL`| 0x07| Bell | Triggers the bell callback or a visual bell flash.           |
| `BS` | 0x08| Backspace| Moves the cursor one position to the left.                 |
| `HT` | 0x09| Horizontal Tab| Moves the cursor to the next tab stop.                |
| `LF` | 0x0A| Line Feed| Moves cursor down one line. Also performs a carriage return if LNM is set. |
| `VT` | 0x0B| Vertical Tab| Same as Line Feed.                                     |
| `FF` | 0x0C| Form Feed| Same as Line Feed.                                     |
| `CR` | 0x0D| Carriage Return| Moves cursor to the beginning of the current line.    |
| `SO` | 0x0E| Shift Out| Invokes the G1 character set into GL.                    |
| `SI` | 0x0F| Shift In | Invokes the G0 character set into GL.                    |
| `CAN`| 0x18| Cancel | Cancels the current escape sequence.                       |
| `SUB`| 0x1A| Substitute| Same as Cancel.                                        |
| `ESC`| 0x1B| Escape | Begins an escape sequence.                                 |
| `DEL`| 0x7F| Delete | Ignored.                                                     |

### 3.2. C1 Control Codes (7-bit and 8-bit)

In a 7-bit environment, C1 codes (`0x80-0x9F`) are represented by `ESC` followed by a character in the range `0x40-0x5F`. The library primarily uses the 7-bit form.

| Sequence | Name | Description                                           |
| :------- | :--- | :---------------------------------------------------- |
| `ESC D`  | IND  | Index: Moves cursor down one line.                    |
| `ESC E`  | NEL  | Next Line: Moves cursor to start of next line.        |
| `ESC H`  | HTS  | Horizontal Tabulation Set: Sets a tab stop at the current cursor column. |
| `ESC M`  | RI   | Reverse Index: Moves cursor up one line.              |
| `ESC N`  | SS2  | Single Shift 2: Uses G2 charset for the next character only. |
| `ESC O`  | SS3  | Single Shift 3: Uses G3 charset for the next character only. |
| `ESC P`  | DCS  | Device Control String: Begins a device-specific command. |
| `ESC Z`  | DECID| DEPRECATED - Return Device Attributes. Use `CSI c`.   |
| `ESC [`  | CSI  | Control Sequence Introducer: Begins a CSI sequence.   |
| `ESC \`  | ST   | String Terminator: Ends DCS, OSC, APC, PM strings.    |
| `ESC ]`  | OSC  | Operating System Command: Begins an OS-level command (e.g., set title). |
| `ESC ^`  | PM   | Privacy Message: Begins a privacy message string.     |
| `ESC _`  | APC  | Application Program Command: Begins an application command string. |

### 3.3. CSI - Control Sequence Introducer (`ESC [`)

CSI sequences are the most common type of control sequence. They follow the format `CSI [private] [params] [intermediate] final`.

-   **Private Marker (`?`)**: A `?` after the `[` indicates a DEC Private Mode sequence.
-   **Parameters (`Pn`)**: A series of semicolon-separated numbers (e.g., `1;5`). Default is 1 or 0 depending on context.
-   **Intermediate Bytes**: Characters in the range `0x20-0x2F` (e.g., ` ` (space), `$`, `"`).
-   **Final Byte**: A character in the range `0x40-0x7E`.

#### 3.3.1. CSI Command Reference

This section provides a comprehensive list of all supported CSI sequences, categorized by function. `Pn`, `Pm`, `Ps`, `Pt`, `Pb`, `Pl`, `Pr` represent numeric parameters.

| Command | Final Byte | Name | Description |
| :--- | :--- | :--- | :--- |
| **Cursor Movement** | | | |
| `CSI Pn A` | `A` | CUU | **Cursor Up.** Moves cursor up by `Pn` lines. Default `Pn=1`. |
| `CSI Pn B` | `B` | CUD | **Cursor Down.** Moves cursor down by `Pn` lines. Default `Pn=1`. |
| `CSI Pn C` | `C` | CUF | **Cursor Forward.** Moves cursor right by `Pn` columns. Default `Pn=1`. |
| `CSI Pn D` | `D` | CUB | **Cursor Back.** Moves cursor left by `Pn` columns. Default `Pn=1`. |
| `CSI Pn E` | `E` | CNL | **Cursor Next Line.** Moves cursor to the start of the line `Pn` lines down. Default `Pn=1`. |
| `CSI Pn F` | `F` | CPL | **Cursor Previous Line.** Moves cursor to the start of the line `Pn` lines up. Default `Pn=1`. |
| `CSI Pn G` | `G` | CHA | **Cursor Horizontal Absolute.** Moves cursor to column `Pn`. Default `Pn=1`. |
| `CSI Pn ` ` | `\`` | HPA | **Horizontal Position Absolute.** Same as CHA. |
| `CSI Pn a` | `a` | HPR | **Horizontal Position Relative.** Moves cursor right by `Pn` columns. |
| `CSI Pn;Pm H` | `H` | CUP | **Cursor Position.** Moves cursor to row `Pn`, column `Pm`. Defaults `Pn=1`, `Pm=1`. |
| `CSI Pn;Pm f` | `f` | HVP | **Horizontal and Vertical Position.** Same as CUP. |
| `CSI Pn d` | `d` | VPA | **Vertical Position Absolute.** Moves cursor to row `Pn`. Default `Pn=1`. |
| `CSI Pn e` | `e` | VPR | **Vertical Position Relative.** Moves cursor down `Pn` lines. |
| `CSI s` | `s` | ANSISYSSC | **Save Cursor Position (ANSI.SYS).** For DEC-style save, see `ESC 7`. |
| `CSI u` | `u` | ANSISYSRC | **Restore Cursor Position (ANSI.SYS).** For DEC-style restore, see `ESC 8`. |
| **Erasing & Editing** | | | |
| `CSI Ps J` | `J` | ED | **Erase in Display.** `Ps=0`: from cursor to end. `Ps=1`: from start to cursor. `Ps=2`: entire screen. `Ps=3`: entire screen and scrollback (xterm). |
| `CSI Ps K` | `K` | EL | **Erase in Line.** `Ps=0`: from cursor to end. `Ps=1`: from start to cursor. `Ps=2`: entire line. |
| `CSI Pn L` | `L` | IL | **Insert Lines.** Inserts `Pn` blank lines at the cursor. Default `Pn=1`. |
| `CSI Pn M` | `M` | DL | **Delete Lines.** Deletes `Pn` lines at the cursor. Default `Pn=1`. |
| `CSI Pn P` | `P` | DCH | **Delete Characters.** Deletes `Pn` characters at the cursor. Default `Pn=1`. |
| `CSI Pn X` | `X` | ECH | **Erase Characters.** Erases `Pn` characters from the cursor without deleting them. Default `Pn=1`. |
| `CSI Pn @` | `@` | ICH | **Insert Characters.** Inserts `Pn` blank spaces at the cursor. Default `Pn=1`. |
| `CSI Pn b` | `b` | REP | **Repeat Preceding Character.** Repeats the previous graphic character `Pn` times. |
| **Scrolling** | | | |
| `CSI Pn S` | `S` | SU | **Scroll Up.** Scrolls the active region up by `Pn` lines. Default `Pn=1`. |
| `CSI Pn T` | `T` | SD | **Scroll Down.** Scrolls the active region down by `Pn` lines. Default `Pn=1`. |
| `CSI Pt;Pb r` | `r` | DECSTBM | **Set Top and Bottom Margins.** Defines the scrollable area from row `Pt` to `Pb`. |
| `CSI ? Pl;Pr s`| `s` | DECSLRM | **Set Left and Right Margins.** Defines horizontal margins (VT420+). |
| **Tabulation** | | | |
| `CSI Pn I` | `I` | CHT | **Cursor Horizontal Tab.** Moves cursor forward `Pn` tab stops. Default `Pn=1`. |
| `CSI Pn Z` | `Z` | CBT | **Cursor Backward Tab.** Moves cursor backward `Pn` tab stops. Default `Pn=1`. |
| `CSI Ps g` | `g` | TBC | **Tabulation Clear.** `Ps=0`: clear stop at current column. `Ps=3`: clear all stops. |
| **Text Attributes (SGR)** | | | |
| `CSI Pm m` | `m` | SGR | **Select Graphic Rendition.** Sets text attributes. See SGR table below for `Pm` values. |
| **Modes** | | | |
| `CSI Pm h` | `h` | SM | **Set Mode.** Enables an ANSI or DEC private mode. See Mode table below. |
| `CSI Pm l` | `l` | RM | **Reset Mode.** Disables an ANSI or DEC private mode. See Mode table below. |
| **Device & Status Reporting**| | | |
| `CSI Ps c` | `c` | DA | **Device Attributes.** `Ps=0` (or omitted) for Primary DA. `>c` for Secondary DA. `=c` for Tertiary DA. |
| `CSI Ps n` | `n` | DSR | **Device Status Report.** `Ps=5`: Status OK (`CSI 0 n`). `Ps=6`: Cursor Position Report (`CSI r;c R`). |
| `CSI ? Ps n` | `n` | DSR (DEC)| **DEC-specific DSR.** E.g., `?15n` (printer), `?26n` (keyboard), `?63n` (checksum). |
| `CSI Ps x` | `x` | DECREQTPARM | **Request Terminal Parameters.** Reports terminal settings. |
| **Miscellaneous** | | | |
| `CSI Pi i` | `i` | MC | **Media Copy.** `Pi=0`: Print screen. `Pi=4`: Disable auto-print. `Pi=5`: Enable auto-print. |
| `CSI ? Pi i`| `i` | MC (DEC) | **DEC Media Copy.** `?4i`: Disable printer controller. `?5i`: Enable printer controller. |
| `CSI Ps q` | `q` | DECLL | **Load LEDs.** `Ps` is a bitmask for keyboard LEDs (VT220+). |
| `CSI Ps SP q`| `q` | DECSCUSR | **Set Cursor Style.** `Ps` selects cursor shape (block, underline, bar) and blink. |
| `CSI ! p` | `p` | DECSTR | **Soft Terminal Reset.** Resets many modes to their default values. |
| `CSI " p` | `p` | DECSCL | **Select Conformance Level.** Sets the terminal's strict VT emulation level. |
| `CSI $ y` | `y` | DECRQM | **Request Mode.** Host requests the current state (set/reset) of a given mode. |
| `CSI $ u` | `u` | DECRQPSR | **Request Presentation State Report.** E.g., Sixel or ReGIS state. |
| `CSI ? Ps y`| `y` | DECTST | **Invoke Confidence Test.** Performs a self-test (e.g., screen fill). |

#### 3.3.2. SGR (Select Graphic Rendition) Parameters
The `CSI Pm m` command sets display attributes based on the numeric parameter `Pm`. Multiple parameters can be combined in a single sequence, separated by semicolons (e.g., `CSI 1;31m`).

| Code | Effect                       | Reset Code |
| :--- | :--------------------------- | :--------- |
| 0    | Reset all attributes         | -          |
| 1    | Bold / Bright                | 22         |
| 2    | Faint / Dim                  | 22         |
| 3    | Italic                       | 23         |
| 4    | Underline                    | 24         |
| 5    | Blink (Slow)                 | 25         |
| 7    | Reverse Video                | 27         |
| 8    | Conceal / Hide               | 28         |
| 9    | Strikethrough                | 29         |
| 21   | Double Underline             | 24         |
| 30-37| Set Foreground (ANSI Colors 0-7) | 39         |
| 40-47| Set Background (ANSI Colors 0-7) | 49         |
| 90-97| Set Bright Foreground (8-15) | 39         |
| 100-107| Set Bright Background (8-15)| 49         |
| 38;5;Pn | Set 256-color Foreground     | 39         |
| 48;5;Pn | Set 256-color Background     | 49         |
| 38;2;Pr;Pg;Pb | Set True Color Foreground | 39         |
| 48;2;Pr;Pg;Pb | Set True Color Background | 49         |

#### 3.3.3. Mode Setting Parameters
The `CSI Pm h` (Set Mode) and `CSI Pm l` (Reset Mode) commands control various terminal behaviors. Sequences starting with `?` are DEC Private Modes.

| Mode (`Pm`) | Name | Description |
| :--- | :--- | :--- |
| **ANSI Modes** | | |
| 4 | `IRM`| **Insert/Replace Mode.** When set (`h`), new characters shift existing text right. When reset (`l`), they overwrite existing text. |
| 20 | `LNM`| **Linefeed/New Line Mode.** When set (`h`), `LF` is treated as `CRLF`. |
| **DEC Private Modes (`?`)** | | |
| 1 | `DECCKM`| **Application Cursor Keys.** `h` enables, `l` disables. When enabled, cursor keys send `ESC O` sequences. |
| 2 | `DECANM`| **ANSI/VT52 Mode.** `l` switches to VT52 mode. Not typically set with `h`. |
| 3 | `DECCOLM`| **132 Column Mode.** `h` switches to 132 columns, `l` to 80. |
| 4 | `DECSCLM`| **Scrolling Mode.** `h` enables smooth scroll, `l` enables jump scroll. |
| 5 | `DECSCNM`| **Reverse Video Screen.** `h` swaps default foreground/background, `l` returns to normal. |
| 6 | `DECOM`| **Origin Mode.** `h` makes cursor movements relative to the scrolling region, `l` makes them relative to the window. |
| 7 | `DECAWM`| **Auto-Wrap Mode.** `h` enables auto-wrap, `l` disables it. |
| 8 | `DECARM`| **Auto-Repeat Mode.** `h` enables key auto-repeat, `l` disables it. |
| 9 | `-`| **X10 Mouse Reporting.** `h` enables basic X10 mouse reporting, `l` disables all mouse tracking. |
| 12 | `-`| **Blinking Cursor.** `h` enables cursor blink, `l` disables it. |
| 25 | `DECTCEM`| **Text Cursor Enable Mode.** `h` shows the cursor, `l` hides it. |
| 40 | `-`| **Allow 80/132 Column Switching.** `h` allows `DECCOLM` to work. |
| 47 | `-`| **Alternate Screen Buffer.** `h` switches to alternate buffer, `l` restores (compatibility). |
| 1000 | `-`| **VT200 Mouse Tracking.** `h` enables reporting of button press/release. `l` disables. |
| 1001 | `-`| **VT200 Highlight Mouse Tracking.** `h` enables reporting on mouse drag. `l` disables. |
| 1002 | `-`| **Button-Event Mouse Tracking.** `h` enables reporting of press, release, and drag. `l` disables. |
| 1003 | `-`| **Any-Event Mouse Tracking.** `h` enables reporting all mouse motion. `l` disables. |
| 1004 | `-`| **Focus In/Out reporting.** `h` enables reporting window focus events. `l` disables. |
| 1005 | `-`| **UTF-8 Mouse Mode.** (Not implemented). |
| 1006 | `-`| **SGR Extended Mouse Reporting.** `h` enables the modern SGR mouse protocol. `l` disables it. |
| 1015 | `-`| **URXVT Mouse Mode.** `h` enables an alternative extended mouse protocol. `l` disables. |
| 1016 | `-`| **Pixel Position Mouse Mode.** `h` enables reporting mouse position in pixels. `l` disables. |
| 1047 | `-`| **Alternate Screen Buffer.** `h` switches to alternate buffer (xterm). |
| 1048 | `-`| **Save/Restore Cursor.** `h` saves, `l` restores cursor position (xterm). |
| 1049 | `-`| **Alternate Screen with Save.** Combines `?1047` and `?1048` (xterm). `h` saves cursor and switches to alternate buffer, `l` restores. |
| 2004 | `-`| **Bracketed Paste Mode.** `h` enables, `l` disables. Wraps pasted text with control sequences. |

### 3.4. OSC - Operating System Command (`ESC ]`)

OSC sequences are used for interacting with the host operating system or terminal window manager. The format is `ESC ] Ps ; Pt BEL` or `ESC ] Ps ; Pt ST`, where `Ps` is the command parameter and `Pt` is the command string.

| `Ps` | Command | `Pt` (Parameters) | Description |
| :--- | :--- | :--- | :--- |
| 0 | Set Icon and Window Title | `string` | Sets both the window and icon titles to the given string. |
| 1 | Set Icon Title | `string` | Sets the icon title. |
| 2 | Set Window Title | `string` | Sets the main window title. |
| 4 | Set/Query Color Palette | `c;spec` | `c` is the color index (0-255). `spec` can be `?` to query, or `rgb:RR/GG/BB` to set. |
| 10 | Set/Query Foreground Color | `?` or `color`| Sets the default text color. `?` queries the current color. |
| 11 | Set/Query Background Color | `?` or `color`| Sets the default background color. `?` queries the current color. |
| 12 | Set/Query Cursor Color | `?` or `color`| Sets the text cursor color. `?` queries the current color. |
| 50 | Set Font | `font_spec` | Sets the terminal font. (Partially implemented). |
| 52 | Clipboard Operations | `c;data` | `c` specifies the clipboard (`c` for clipboard, `p` for primary). `data` is the base64-encoded string to set, or `?` to query. (Query is a no-op). |
| 104| Reset Color Palette | `c1;c2;...` | Resets the specified color indices to their default values. If no parameters are given, resets the entire palette. |
| 110| Reset Foreground Color | (none) | Resets the default foreground color to the initial default. |
| 111| Reset Background Color | (none) | Resets the default background color to the initial default. |
| 112| Reset Cursor Color | (none) | Resets the cursor color to its initial default. |

### 3.5. DCS - Device Control String (`ESC P`)

DCS sequences are for device-specific commands, often with complex data payloads. They are terminated by `ST` (`ESC \`).

| Introduction | Name | Description |
| :--- | :--- | :--- |
| `DCS 1;1\|... ST` | `DECUDK` | **Program User-Defined Keys.** The payload `...` is a list of `key/hex_string` pairs separated by semicolons, where `key` is the keycode and `hex_string` is the hexadecimal representation of the string it should send. Requires VT320+ mode. |
| `DCS 0;1\|... ST` | `DECUDK` | **Clear User-Defined Keys.** |
| `DCS 2;1\|... ST` | `DECDLD` | **Download Soft Font.** Downloads custom character glyphs into the terminal's memory. Requires VT220+ mode. (Partially implemented). |
| `DCS $q... ST` | `DECRQSS` | **Request Status String.** The payload `...` is a name representing the setting to be queried (e.g., `m` for SGR, `r` for scrolling region). The terminal responds with another DCS sequence. |
| `DCS +q... ST` | `XTGETTCAP` | **Request Termcap/Terminfo String.** An xterm feature to query termcap capabilities like `Co` (colors) or `lines`. |
| `DCS ...q ... ST`| `SIXEL` | **Sixel Graphics.** The payload contains Sixel image data to be rendered on the screen. Requires VT320+ mode. (Partially implemented). |

### 3.6. Other Escape Sequences

This table covers common non-CSI escape sequences.

| Sequence | Name | Description |
| :--- | :--- | :--- |
| `ESC c` | `RIS` | **Hard Reset.** Resets the terminal to its initial power-on state. |
| `ESC =` | `DECKPAM` | **Keypad Application Mode.** Sets the numeric keypad to send application-specific sequences. |
| `ESC >` | `DECKPNM` | **Keypad Numeric Mode.** Sets the numeric keypad to send numeric characters. |
| `ESC (` C | `SCS` | **Select G0 Character Set.** Designates character set `C` (e.g., `B` for ASCII, `0` for DEC Special Graphics) to the G0 slot. |
| `ESC )` C | `SCS` | **Select G1 Character Set.** Designates a character set to the G1 slot. |
| `ESC *` C | `SCS` | **Select G2 Character Set.** Designates a character set to the G2 slot. |
| `ESC +` C | `SCS` | **Select G3 Character Set.** Designates a character set to the G3 slot. |

### 3.7. VT52 Mode Sequences

When the terminal is in VT52 mode (`SetVTLevel(VT_LEVEL_52)` or by sending `ESC <`), it uses a different, simpler set of non-ANSI commands.

| Command | Description |
| :--- | :--- |
| `ESC A` | **Cursor Up.** Moves cursor up one line. |
| `ESC B` | **Cursor Down.** Moves cursor down one line. |
| `ESC C` | **Cursor Right.** Moves cursor right one column. |
| `ESC D` | **Cursor Left.** Moves cursor left one column. |
| `ESC H` | **Cursor Home.** Moves cursor to row 0, column 0. |
| `ESC Y r c`| **Direct Cursor Address.** Moves the cursor to the specified row and column. `r` and `c` are single characters with a value of `32 + coordinate`. |
| `ESC I` | **Reverse Line Feed.** Moves the cursor up one line, scrolling the screen down if at the top margin. |
| `ESC J` | **Erase to End of Screen.** Clears from the cursor to the end of the display. |
| `ESC K` | **Erase to End of Line.** Clears from the cursor to the end of the current line. |
| `ESC Z` | **Identify.** The terminal responds with its VT52 identifier: `ESC / Z`. |
| `ESC =` | **Enter Alternate Keypad Mode.** The numeric keypad sends application sequences. |
| `ESC >` | **Exit Alternate Keypad Mode.** The numeric keypad sends numeric characters. |
| `ESC <` | **Enter ANSI Mode.** Switches the terminal back to its configured VT100+ emulation level. |
| `ESC F` | **Enter Graphics Mode.** Selects the DEC Special Graphics character set. |
| `ESC G` | **Exit Graphics Mode.** Selects the default US ASCII character set. |

---

## 4. Key Features In-Depth

### 4.1. Color Support

The library supports three color modes, selected via SGR sequences:

1.  **ANSI 16-color:** SGR codes `30-37`, `40-47`, `90-97`, `100-107`. The exact appearance is determined by the `ansi_colors` array.
2.  **256-color:** `CSI 38;5;Pn m` (foreground) and `CSI 48;5;Pn m` (background). The 256-color palette is a standard xterm palette.
3.  **True Color (24-bit):** `CSI 38;2;R;G;B m` (foreground) and `CSI 48;2;R;G;B m` (background).

### 4.2. Mouse Tracking

Mouse tracking must be enabled by the host application sending the appropriate CSI sequence. When enabled, mouse events are translated into byte sequences and sent to the host via the `ResponseCallback`.

-   **Modes:**
    -   `X10`: Basic button press reporting.
    -   `VT200`: Reports press and release events.
    -   `Button-Event`: Reports press, release, and motion while a button is held.
    -   `Any-Event`: Reports all mouse events, including motion.
    -   `SGR`: A modern, more robust protocol that supports higher-resolution coordinates and more modifier keys.
-   **Enabling:** Use `CSI ?{mode} h` (e.g., `CSI ?1003 h` for Any-Event tracking).
-   **Focus Reporting:** `CSI ?1004 h` enables reporting of window focus gain/loss events (`CSI I` and `CSI O`).

### 4.3. Character Sets

The terminal supports multiple character sets (G0-G3) and can map them to the active GL (left) and GR (right) slots.

-   **`ESC ( C`**: Designates character set `C` to G0.
-   **`ESC ) C`**: Designates character set `C` to G1.
-   **`SO` (`^N`) / `SI` (`^O`)**: Switch between G0 and G1 for the active GL set.
-   **Supported Sets (`C`):**
    -   `B`: US ASCII (Default)
    -   `A`: UK National
    -   `0`: DEC Special Graphics & Line Drawing
    -   `<`: DEC Multinational Character Set (MCS)

### 4.4. Screen and Buffer Management

-   **Alternate Screen Buffer:** Enabled with `CSI ?1049 h`. This saves the current screen state, clears the screen, and switches to a new buffer. `CSI ?1049 l` restores the original screen. This is commonly used by full-screen applications like `vim` or `less`.
-   **Scrolling Region:** `CSI Pt;Pb r` confines scrolling operations to the lines between `Pt` and `Pb`.
-   **Margins:** `CSI ? Pl;Pr s` confines the cursor to the columns between `Pl` and `Pr`.

### 4.5. Sixel Graphics

Sixel is a bitmap graphics format for terminals. The library provides basic support for rendering Sixel data.

-   **Sequence:** `DCS ...q <sixel_data> ST`
-   **Enabling:** Requires a VT level of VT320+ or XTERM.
-   **Functionality:** Sixel data is parsed and rendered onto the terminal grid.

### 4.6. Bracketed Paste Mode

-   **Sequence:** `CSI ?2004 h`
-   **Functionality:** When enabled, pasted text is bracketed by `CSI 200~` and `CSI 201~`. This allows the host application to distinguish between typed and pasted text, preventing accidental execution of commands within the pasted content.

---

## 5. API Reference

This section provides a comprehensive reference for the public API of `terminal.h`.

### 5.1. Lifecycle Functions

These functions manage the initialization and destruction of the terminal instance.

-   `void InitTerminal(void);`
    Initializes the entire `Terminal` struct to a default state. This includes setting up screen buffers, default modes (e.g., auto-wrap on), tab stops, character sets, and the color palette. It also creates the font texture used for rendering. **Must be called once** after the Raylib window is created.

-   `void CleanupTerminal(void);`
    Frees all resources allocated by the terminal. This includes the font texture, memory for programmable keys, and any other dynamically allocated buffers. **Must be called once** before the application exits to prevent memory leaks.

-   `void UpdateTerminal(void);`
    This is the main "tick" function for the terminal. It should be called once per frame. It drives all ongoing processes:
    -   Processes a batch of characters from the input pipeline.
    -   Updates internal timers for cursor and text blinking.
    -   Polls Raylib for keyboard and mouse input and translates them into VT events.
    -   Invokes the `ResponseCallback` if any data is queued to be sent to the host.

-   `void DrawTerminal(void);`
    Renders the current state of the terminal to the screen. It iterates over the screen buffer, drawing each character with its correct attributes. It also handles drawing the cursor, Sixel graphics, and visual bell. Must be called within a Raylib `BeginDrawing()` / `EndDrawing()` block.

### 5.2. Host Input (Pipeline) Management

These functions are used by the host application to feed data *into* the terminal for emulation.

-   `bool PipelineWriteChar(unsigned char ch);`
    Writes a single character to the input pipeline. Returns `false` if the pipeline is full.

-   `bool PipelineWriteString(const char* str);`
    Writes a null-terminated string to the input pipeline.

-   `bool PipelineWriteFormat(const char* format, ...);`
    Writes a printf-style formatted string to the input pipeline.

-   `void ProcessPipeline(void);`
    Manually processes a batch of characters from the pipeline. This is called automatically by `UpdateTerminal()` but can be called manually for finer control.

-   `void ClearPipeline(void);`
    Discards all data currently in the input pipeline.

-   `int GetPipelineCount(void);`
    Returns the number of bytes currently waiting in the input pipeline.

-   `bool IsPipelineOverflow(void);`
    Returns `true` if the pipeline has overflowed since the last `ClearPipeline()` call.

### 5.3. Keyboard and Mouse Output

These functions are used to get user input events *from* the terminal, ready to be sent to the host.

-   `bool GetVTKeyEvent(VTKeyEvent* event);`
    Retrieves the next processed keyboard event from the output queue. `UpdateVTKeyboard()` (called by `UpdateTerminal()`) is responsible for polling the keyboard and populating this queue. This function is the primary way for the host application to receive user keyboard input. Returns `true` if an event was retrieved.

-   `void UpdateVTKeyboard(void);`
    Polls the keyboard, processes modifier keys and terminal modes (e.g., Application Cursor Keys), and places `VTKeyEvent`s into the output queue. Called automatically by `UpdateTerminal()`.

-   `void UpdateMouse(void);`
    Polls the mouse position and button states, translates them into the appropriate VT mouse protocol sequence, and queues the result for the `ResponseCallback`. Called automatically by `UpdateTerminal()`.

### 5.4. Configuration and Mode Setting

Functions for configuring the terminal's behavior at runtime.

-   `void SetVTLevel(VTLevel level);`
    Sets the terminal's emulation compatibility level (e.g., `VT_LEVEL_220`, `VT_LEVEL_XTERM`). This is a critical function that changes which features and escape sequences are active.

-   `VTLevel GetVTLevel(void);`
    Returns the current `VTLevel`.

-   `void SetTerminalMode(const char* mode, bool enable);`
    A generic function to enable or disable specific terminal modes by name, such as `"application_cursor"`, `"auto_wrap"`, `"origin"`, or `"insert"`.

-   `void SetCursorShape(CursorShape shape);`
    Sets the visual style of the cursor (e.g., `CURSOR_BLOCK_BLINK`, `CURSOR_UNDERLINE`).

-   `void SetCursorColor(ExtendedColor color);`
    Sets the color of the cursor.

-   `void SetMouseTracking(MouseTrackingMode mode);`
    Explicitly enables a specific mouse tracking protocol (e.g., `MOUSE_TRACKING_SGR`). This is usually controlled by the host application via escape sequences, but can be set manually.

-   `void EnableBracketedPaste(bool enable);`
    Manually enables or disables bracketed paste mode.

-   `void DefineFunctionKey(int key_num, const char* sequence);`
    Programs a function key (F1-F24) to send a custom string sequence when pressed.

### 5.5. Callbacks

These functions allow the host application to receive data and notifications from the terminal.

-   `void SetResponseCallback(ResponseCallback callback);`
    Sets the callback function that receives all data the terminal sends back to the host. This includes keyboard input, mouse events, and status reports.
    `typedef void (*ResponseCallback)(const char* response, int length);`

-   `void SetTitleCallback(TitleCallback callback);`
    Sets the callback function that is invoked whenever the window or icon title is changed by the host via an OSC sequence.
    `typedef void (*TitleCallback)(const char* title, bool is_icon);`

-   `void SetBellCallback(BellCallback callback);`
    Sets the callback function for the audible bell (`BEL`, `0x07`). If `NULL`, a visual bell is used instead.
    `typedef void (*BellCallback)(void);`

### 5.6. Diagnostics and Testing

Utilities for inspecting the terminal's state and verifying its functionality.

-   `void EnableDebugMode(bool enable);`
    Enables or disables verbose logging of unsupported sequences and other diagnostic information.

-   `TerminalStatus GetTerminalStatus(void);`
    Returns a `TerminalStatus` struct containing information about buffer usage and performance metrics.

-   `void ShowBufferDiagnostics(void);`
    A convenience function that prints buffer usage information directly to the terminal screen.

-   `void RunVTTest(const char* test_name);`
    Runs built-in test sequences to verify functionality. Valid test names include `"cursor"`, `"colors"`, `"charset"`, `"modes"`, `"mouse"`, and `"all"`.

-   `void ShowTerminalInfo(void);`
    A convenience function that prints a summary of the current terminal state (VT level, modes, etc.) directly to the screen.

### 5.7. Advanced Control

These functions provide finer-grained control over specific terminal features.

-   `void SelectCharacterSet(int gset, CharacterSet charset);`
    Designates a `CharacterSet` (e.g., `CHARSET_DEC_SPECIAL`) to one of the four character set "slots" (G0-G3).

-   `void SetTabStop(int column);`, `void ClearTabStop(int column);`, `void ClearAllTabStops(void);`
    Functions for manually managing horizontal tab stops.

-   `void LoadSoftFont(const unsigned char* font_data, int char_start, int char_count);`
    Loads custom character glyph data into the terminal's soft font memory (DECDLD).

-   `void SelectSoftFont(bool enable);`
    Enables or disables the use of the loaded soft font.

---

## 6. Internal Operations and Data Flow

This chapter provides a deeper, narrative look into the internal mechanics of the library, tracing the journey of a single character from its arrival in the input pipeline to its final rendering on the screen.

### 6.1. Stage 1: Ingestion

1.  **Entry Point:** A host application calls `PipelineWriteString("ESC[31mHello")`.
2.  **Buffering:** Each character of the string (`E`, `S`, `C`, `[`, `3`, `1`, `m`, `H`, `e`, `l`, `l`, `o`) is sequentially written into the `input_pipeline`, a large circular byte buffer. The `pipeline_head` index advances with each write.

### 6.2. Stage 2: Consumption and Parsing

1.  **The Tick:** The main `UpdateTerminal()` function is called. It determines it has a processing budget to handle, for example, 200 characters.
2.  **Dequeuing:** `ProcessPipeline()` begins consuming characters from the `pipeline_tail`.
3.  **The State Machine in Action:** `ProcessChar()` is called for each character:
    -   **`E`, `S`, `C`:** These are initially processed in the `VT_PARSE_NORMAL` state. Since they are regular printable characters, the terminal would normally just print them. However, the parser is about to hit the `ESC` character.
    -   **`ESC` (`0x1B`):** When `ProcessNormalChar()` receives the Escape character, it does not print anything. Instead, it immediately changes the parser's state: `terminal.parse_state = VT_PARSE_ESCAPE;`.
    -   **`[`:** The next character is processed by `ProcessEscapeChar()`. It sees `[` and knows this is a Control Sequence Introducer. It changes the state again: `terminal.parse_state = PARSE_CSI;` and clears the `escape_buffer`.
    -   **`3`, `1`, `m`:** Now `ProcessCSIChar()` is being called.
        -   The characters `3` and `1` are numeric parameters. They are appended to the `escape_buffer`.
        -   The character `m` is a "final byte" (in the range `0x40`-`0x7E`). This terminates the sequence.
4.  **Execution:**
    -   `ProcessCSIChar()` calls `ParseCSIParams()` on the `escape_buffer` ("31"). This populates the `escape_params` array with the integer `31`.
    -   It then calls `ExecuteCSICommand('m')`.
    -   The command dispatcher for `m` (`ExecuteSGR`) is invoked. `ExecuteSGR` iterates through its parameters. It sees `31`, which corresponds to setting the foreground color to ANSI red.
    -   It updates the *current terminal state* by changing `terminal.current_fg` to represent the color red. It does **not** yet change any character on the screen.
    -   Finally, the parser state is reset to `VT_PARSE_NORMAL`.

### 6.3. Stage 3: Character Processing and Screen Buffer Update

1.  **`H`, `e`, `l`, `l`, `o`:** The parser is now back in `VT_PARSE_NORMAL`. `ProcessNormalChar()` is called for each of these characters.
2.  **Placement:** For the character 'H':
    -   The function checks the cursor's current position (`terminal.cursor.x`, `terminal.cursor.y`).
    -   It retrieves the `EnhancedTermChar` struct at that position in the `screen` buffer.
    -   It sets that struct's `ch` field to `'H'`.
    -   Crucially, it copies the *current* terminal attributes into the cell's attributes: `cell->fg_color` gets the value of `terminal.current_fg` (which we just set to red).
    -   The `dirty` flag for this cell is set to `true`.
    -   The cursor's X position is incremented: `terminal.cursor.x++`.
3.  This process repeats for 'e', 'l', 'l', 'o', each time placing the character, applying the current SGR attributes (red foreground), and advancing the cursor.

### 6.4. Stage 4: Rendering

1.  **Drawing Frame:** `DrawTerminal()` is called within the application's drawing loop.
2.  **Iteration:** It begins a nested loop through every `EnhancedTermChar` in the `screen` buffer.
3.  **Decision:** When it gets to the cells for "Hello", it checks their `dirty` flag (or redraws all cells regardless, depending on optimization).
4.  **Attribute Resolution:** For the 'H' cell:
    -   It looks at `cell->fg_color` (red).
    -   It looks at `cell->bg_color` (the default black).
    -   It checks for other attributes like `bold`, `underline`, etc.
5.  **Raylib Calls:**
    -   It calls `DrawRectangle()` to draw the cell's background with the resolved background color.
    -   It looks up the glyph for 'H' in the `font_texture` and calls `DrawTextureRec()` to render it using the resolved foreground color (red).

This entire cycle, from ingestion to rendering, happens continuously, allowing the terminal to process a stream of data and translate it into a visual representation.

### 6.5. Stage 5: Keyboard Input Processing

Concurrent to the host-to-terminal data flow, the library handles user input from the physical keyboard and mouse, translating it into byte sequences that a host application can understand.

1.  **Polling:** In each frame, `UpdateTerminal()` calls `UpdateVTKeyboard()`. This function polls Raylib for any key presses, releases, or character inputs.
2.  **Event Creation:** For each input, a `VTKeyEvent` struct is created, capturing the raw key code, modifier states (Ctrl, Alt, Shift), and a timestamp.
3.  **Sequence Generation:** The core of the translation happens in `GenerateVTSequence()`. This function takes a `VTKeyEvent` and populates its `sequence` field based on a series of rules:
    -   **Control Keys:** `Ctrl+A` is translated to `0x01`, `Ctrl+[` to `ESC`, etc.
    -   **Application Modes:** It checks `terminal.vt_keyboard.cursor_key_mode` (DECCKM) and `terminal.vt_keyboard.keypad_mode` (DECKPAM). If these modes are active, it generates application-specific sequences (e.g., `ESC O A` for the up arrow) instead of the default ANSI sequences (`ESC [ A`).
    -   **Meta Key:** If `terminal.vt_keyboard.meta_sends_escape` is true, pressing `Alt` in combination with another key will prefix that key's character with an `ESC` byte.
    -   **Normal Characters:** Standard printable characters are typically encoded as UTF-8.
4.  **Buffering:** The processed `VTKeyEvent`, now containing the final byte sequence, is placed into the `vt_keyboard.buffer`, a circular event buffer.
5.  **Host Retrieval (API):** The host application integrating the library is expected to call `GetVTKeyEvent()` in its main loop. This function dequeues the next event from the buffer, providing the host with the translated sequence.
6.  **Host Transmission (Callback):** The typical application pattern is to take the sequence received from `GetVTKeyEvent()` and send it immediately back to the PTY or remote connection that the terminal is displaying. This is often done via the `ResponseCallback` mechanism. For local echo, the same sequence can also be written back into the terminal's *input* pipeline to be displayed on screen.

This clear separation of input (`input_pipeline`) and output (`vt_keyboard.buffer` -> `ResponseCallback`) ensures that the terminal acts as a proper two-way communication device, faithfully translating between user actions and the byte streams expected by terminal-aware applications.

---

## 7. Data Structures Reference

This section provides an exhaustive reference to the core data structures and enumerations used within `terminal.h`. A deep understanding of these structures is essential for advanced integration, debugging, or extending the library's functionality.

### 7.1. Enums

#### 7.1.1. `VTLevel`

Determines the terminal's emulation compatibility level, affecting which escape sequences are recognized and how the terminal identifies itself.

| Value | Description |
| :--- | :--- |
| `VT_LEVEL_52` | Emulates the DEC VT52, a basic glass teletype with a simple command set. |
| `VT_LEVEL_100` | Emulates the DEC VT100, the foundational standard for ANSI escape sequences. |
| `VT_LEVEL_220` | Emulates the DEC VT220, adding features like soft fonts and more character sets. |
| `VT_LEVEL_320` | Emulates the DEC VT320, notably introducing Sixel graphics support. |
| `VT_LEVEL_420` | Emulates the DEC VT420, adding rectangular area operations and left/right margins. |
| `VT_LEVEL_XTERM` | Emulates a modern xterm terminal, the de facto standard with the widest feature support, including True Color, advanced mouse tracking, and numerous extensions. This is the default level. |

#### 7.1.2. `VTParseState`

Tracks the current state of the escape sequence parser as it consumes characters from the input pipeline.

| Value | Description |
| :--- | :--- |
| `VT_PARSE_NORMAL` | The default state. Regular characters are printed to the screen, and control codes trigger state transitions. |
| `VT_PARSE_ESCAPE` | Entered after an `ESC` (`0x1B`) is received. The parser waits for the next byte to identify the sequence type. |
| `PARSE_CSI` | Control Sequence Introducer (`ESC [`). The parser is accumulating parameters for a CSI sequence. |
| `PARSE_OSC` | Operating System Command (`ESC ]`). The parser is accumulating a string for an OSC command. |
| `PARSE_DCS` | Device Control String (`ESC P`). The parser is accumulating a device-specific command string. |
| `PARSE_APC` | Application Program Command (`ESC _`). |
| `PARSE_PM` | Privacy Message (`ESC ^`). |
| `PARSE_SOS` | Start of String (`ESC X`). |
| `PARSE_STRING_TERMINATOR`| State indicating the parser expects a String Terminator (`ST`, `ESC \`) to end a command string. |
| `PARSE_CHARSET` | The parser is selecting a character set after an `ESC (` or similar sequence. |
| `PARSE_VT52` | The parser is in VT52 compatibility mode, using a different command set. |
| `PARSE_SIXEL` | The parser is processing a Sixel graphics data stream. |

#### 7.1.3. `MouseTrackingMode`

Defines the active protocol for reporting mouse events to the host application.

| Value | Description |
| :--- | :--- |
| `MOUSE_TRACKING_OFF` | No mouse events are reported. |
| `MOUSE_TRACKING_X10` | Basic protocol; reports only on button press. |
| `MOUSE_TRACKING_VT200` | Reports on both button press and release. |
| `MOUSE_TRACKING_VT200_HIGHLIGHT`| Reports press, release, and motion while a button is held down. |
| `MOUSE_TRACKING_BTN_EVENT` | Same as `VT200_HIGHLIGHT`. |
| `MOUSE_TRACKING_ANY_EVENT` | Reports all mouse events, including motion even when no buttons are pressed. |
| `MOUSE_TRACKING_SGR` | Modern, robust protocol that reports coordinates with higher precision and more modifier key information. |
| `MOUSE_TRACKING_URXVT` | An alternative extended coordinate protocol. |
| `MOUSE_TRACKING_PIXEL` | Reports mouse coordinates in pixels rather than character cells. |

#### 7.1.4. `CursorShape`

Defines the visual style of the text cursor.

| Value | Description |
| :--- | :--- |
| `CURSOR_BLOCK`, `CURSOR_BLOCK_BLINK` | A solid or blinking rectangle covering the entire character cell. |
| `CURSOR_UNDERLINE`, `CURSOR_UNDERLINE_BLINK` | A solid or blinking line at the bottom of the character cell. |
| `CURSOR_BAR`, `CURSOR_BAR_BLINK` | A solid or blinking vertical line at the left of the character cell. |

#### 7.1.5. `CharacterSet`

Represents a character encoding standard that can be mapped to one of the G0-G3 slots.

| Value | Description |
| :--- | :--- |
| `CHARSET_ASCII` | Standard US ASCII. |
| `CHARSET_DEC_SPECIAL` | DEC Special Graphics and Line Drawing character set. |
| `CHARSET_UK` | UK National character set (replaces '#' with '£'). |
| `CHARSET_DEC_MULTINATIONAL`| DEC Multinational Character Set (MCS). |
| `CHARSET_ISO_LATIN_1` | ISO/IEC 8859-1 character set. |
| `CHARSET_UTF8` | UTF-8 encoding (requires multi-byte processing). |

### 7.2. Core Structs

#### 7.2.1. `Terminal`

This is the master struct that encapsulates the entire state of the terminal emulator.

-   `EnhancedTermChar screen[H][W]`, `alt_screen[H][W]`: The primary and alternate screen buffers, 2D arrays representing every character cell on the display.
-   `EnhancedCursor cursor`, `saved_cursor`: The current state and position of the cursor, and a saved copy for `DECSC`/`DECRC` operations.
-   `VTConformance conformance`: Tracks the emulation level (e.g., `VT_LEVEL_220`), feature support, and compliance diagnostics.
-   `char device_attributes[128]`, `secondary_attributes[128]`, `tertiary_attributes[128]`: Strings reported back to the host for device attribute queries (`CSI c`, `CSI >c`, `CSI =c`).
-   `DECModes dec_modes`, `ANSIModes ansi_modes`: Structures holding boolean flags for all terminal modes.
-   `ExtendedColor current_fg`, `current_bg`: The active foreground and background colors that will be applied to new characters printed to the screen.
-   `bool bold_mode`, `italic_mode`, `...`: A series of boolean flags for the currently active SGR text attributes.
-   `int scroll_top`, `scroll_bottom`: Defines the 0-indexed top and bottom lines of the active scrolling region (`DECSTBM`).
-   `int left_margin`, `right_margin`: Defines the 0-indexed left and right column boundaries for margin-based operations (`DECSLRM`, VT420+).
-   `CharsetState charset`: Manages the four designated character sets (G0-G3) and which are currently mapped to the active GL/GR slots.
-   `TabStops tab_stops`: An array-based structure tracking the column positions of all horizontal tab stops.
-   `BracketedPaste bracketed_paste`: Holds the state and buffer for bracketed paste mode.
-   `ProgrammableKeys programmable_keys`: Stores sequences for user-defined keys (`DECUDK`).
-   `SixelGraphics sixel`: Holds the state and data buffer for displaying any active Sixel image.
-   `SoftFont soft_font`: Storage for downloaded, user-defined character glyphs (`DECDLD`).
-   `TitleManager title`: Buffers for storing the window and icon titles set by OSC sequences.
-   `struct mouse`: Contains the complete state of mouse tracking, including the current mode, button states, and last reported position.
-   `unsigned char input_pipeline[]`: The circular buffer that ingests all incoming data from the host application.
-   `struct vt_keyboard`: Encapsulates the complete state of the keyboard, including modes (e.g., Application Keypad) and the output buffer for processed key events.
-   `VTParseState parse_state`: The current state of the main escape sequence parser.
-   `char escape_buffer[]`: A buffer for accumulating the parameter and intermediate bytes of a control sequence as it is being parsed.
-   `int escape_params[]`, `param_count`: The array of parsed numeric parameters from a CSI sequence and their count.
-   `char answerback_buffer[]`: The output buffer for queueing all data to be sent back to the host, including keypresses, mouse events, and status reports.
-   `ResponseCallback response_callback`, `TitleCallback title_callback`, `BellCallback bell_callback`: Function pointers for handling events.

#### 7.2.2. `EnhancedTermChar`

Represents a single character cell on the screen, storing all of its visual and metadata attributes.

-   `unsigned int ch`: The Unicode codepoint of the character in the cell.
-   `ExtendedColor fg_color`, `bg_color`: The foreground and background colors for this specific cell.
-   `bool bold`, `faint`, `italic`, `underline`, `blink`, `reverse`, `strikethrough`, `conceal`, `overline`, `double_underline`: A complete set of boolean flags for all standard SGR attributes.
-   `bool double_width`, `double_height_top`, `double_height_bottom`: Flags for DEC line attributes (`DECDWL`, `DECDHL`).
-   `bool protected_cell`: Flag for the DECSCA attribute, which can protect the cell from being erased by certain sequences.
-   `bool dirty`: A rendering hint flag. When `true`, it signals to the renderer that this cell has changed and needs to be redrawn.

#### 7.2.3. `ExtendedColor`

A flexible structure for storing color information, capable of representing both indexed and true-color values.

-   `int color_mode`: A flag indicating the color representation: `0` for an indexed palette color, `1` for a 24-bit RGB color.
-   `union value`:
    -   `int index`: If `color_mode` is `0`, this stores the 0-255 palette index.
    -   `RGB_Color rgb`: If `color_mode` is `1`, this struct stores the 24-bit R, G, B values.

#### 7.2.4. `EnhancedCursor`
-   `int x`, `y`: The 0-indexed column and row position of the cursor on the screen grid.
-   `bool visible`: Whether the cursor is currently visible, controlled by the `DECTCEM` mode (`CSI ?25 h/l`).
-   `bool blink_enabled`: Whether the cursor is set to a blinking shape (e.g., `CURSOR_BLOCK_BLINK`).
-   `bool blink_state`: The current on/off state of the blink, which is toggled by an internal timer.
-   `double blink_timer`: The timer used to toggle `blink_state` at a regular interval.
-   `CursorShape shape`: The visual style of the cursor (`CURSOR_BLOCK`, `CURSOR_UNDERLINE`, or `CURSOR_BAR`).
-   `ExtendedColor color`: The color of the cursor itself, which can be set independently from the text color via an OSC sequence.

#### 7.2.5. `DECModes` and `ANSIModes`
These two structs contain boolean flags that track the state of all major terminal modes.

-   `bool application_cursor_keys`: `DECCKM` state. If `true`, cursor keys send application-specific sequences (e.g., `ESC O A` instead of `ESC [ A`).
-   `bool origin_mode`: `DECOM` state. If `true`, the home cursor position and absolute cursor movements are relative to the top-left corner of the scrolling region, not the screen.
-   `bool auto_wrap_mode`: `DECAWM` state. If `true`, the cursor automatically wraps to the beginning of the next line upon reaching the right margin.
-   `bool alternate_screen`: Tracks if the alternate screen buffer is currently active, as set by `CSI ?1049 h`.
-   `bool insert_mode`: `IRM` state. If `true`, new characters typed at the cursor shift existing characters to the right instead of overwriting them.
-   `bool new_line_mode`: `LNM` state. If `true`, a line feed (`LF`) operation is treated as a `CRLF`, moving the cursor to the beginning of the new line.
-   `bool column_mode_132`: `DECCOLM` state. Tracks if the terminal is logically in 132-column mode.
-   `bool reverse_video`: `DECSCNM` state. If `true`, the entire screen's foreground and background colors are globally swapped during rendering.

#### 7.2.6. `VTKeyEvent`

A structure containing a fully processed keyboard event, ready to be sent back to the host application.

-   `int key_code`: The original Raylib key code or Unicode character code that generated the event.
-   `bool ctrl`, `shift`, `alt`, `meta`: The state of the modifier keys at the time of the press.
-   `char sequence[32]`: The final, translated byte sequence to be sent to the host (e.g., `"A"`, `"\x1B[A"`, or `"\x01"` for Ctrl+A).

#### 7.2.7. `CharsetState`
-   `CharacterSet g0, g1, g2, g3`: Stores which character set is designated to each of the four "G-set" slots.
-   `CharacterSet *gl, *gr`: Pointers that determine the active "left" (GL) and "right" (GR) character sets. For 7-bit terminals, characters are typically interpreted from the GL set.
-   `bool single_shift_2`, `single_shift_3`: Flags that are set by `SS2` (`ESC N`) and `SS3` (`ESC O`). When set, they indicate that the *very next* character should be interpreted from the G2 or G3 set, respectively, for a single character lookup.

---

## 8. Configuration Constants

These `#define` constants, located at the top of `terminal.h`, allow for compile-time configuration of the terminal's default behaviors and resource limits. To change them, you must define them *before* the `#include "terminal.h"` line where `TERMINAL_IMPLEMENTATION` is defined.

| Constant | Default Value | Description |
| :--- | :--- | :--- |
| `DEFAULT_TERM_WIDTH` | `132` | Sets the default width of the terminal screen in character cells. This is the value used on initial startup. |
| `DEFAULT_TERM_HEIGHT`| `50` | Sets the default height of the terminal screen in character cells. |
| `DEFAULT_CHAR_WIDTH` | `8` | The width in pixels of a single character glyph in the bitmap font. This value is tied to the included `font_data.h`. |
| `DEFAULT_CHAR_HEIGHT`| `16` | The height in pixels of a single character glyph. Also tied to the font data. |
| `DEFAULT_WINDOW_SCALE`| `1` | A scaling factor applied to the window size and font rendering. A value of `2` would create a 2x scaled window. |
| `MAX_ESCAPE_PARAMS` | `32` | The maximum number of numeric parameters that can be parsed from a single CSI sequence (e.g., `CSI Pn;Pn;...;Pn m`). Increasing this allows for more complex SGR sequences but uses more memory. |
| `MAX_COMMAND_BUFFER`| `512` | The size of the buffer used to temporarily store incoming escape sequences (CSI, OSC, DCS, etc.) during parsing. If you expect very long OSC title strings or DCS payloads, you may need to increase this. |
| `MAX_TAB_STOPS` | `256` | The maximum number of columns for which tab stops can be set. This should be greater than or equal to `DEFAULT_TERM_WIDTH`. |
| `MAX_TITLE_LENGTH` | `256` | The maximum length of the window and icon titles that can be set via OSC sequences. |
| `KEY_EVENT_BUFFER_SIZE`| `65536`| The size of the circular buffer for queuing processed keyboard events before they are sent to the host. A larger buffer can handle rapid typing without dropping events. |
| `OUTPUT_BUFFER_SIZE` | `16384`| The size of the buffer used to queue responses (keyboard input, mouse events, status reports) to be sent back to the host application via the `ResponseCallback`. |

---

## 9. License

`terminal.h` is licensed under the MIT License.
