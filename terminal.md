# terminal.h - Technical Reference Manual

**(c) 2025 Jacques Morel**

This document provides an exhaustive technical reference for `terminal.h`, an enhanced single-header terminal emulation library for C. It is intended for developers integrating the library into their applications and those who need a deep understanding of its capabilities, supported protocols, and internal architecture.

<details>
<summary>Table of Contents</summary>

1.  [Overview](#1-overview)
    1.  [1.1. Description](#11-description)
    2.  [1.2. Key Features](#12-key-features)
    3.  [1.3. Architectural Deep Dive](#13-architectural-deep-dive)
2.  [Compliance and Emulation Levels](#2-compliance-and-emulation-levels)
    1.  [2.1. Setting the Compliance Level](#21-setting-the-compliance-level)
    2.  [2.2. Feature Breakdown by `VTLevel`](#22-feature-breakdown-by-vtlevel)
3.  [Control and Escape Sequences](#3-control-and-escape-sequences)
    1.  [3.1. C0 Control Codes](#31-c0-control-codes)
    2.  [3.2. C1 Control Codes (7-bit and 8-bit)](#32-c1-control-codes-7-bit-and-8-bit)
    3.  [3.3. CSI - Control Sequence Introducer (`ESC [`)](#33-csi---control-sequence-introducer-esc-)
    4.  [3.4. OSC - Operating System Command (`ESC ]`)](#34-osc---operating-system-command-esc--)
    5.  [3.5. DCS - Device Control String (`ESC P`)](#35-dcs---device-control-string-esc-p)
    6.  [3.6. Other Escape Sequences](#36-other-escape-sequences)
    7.  [3.7. VT52 Mode Sequences](#37-vt52-mode-sequences)
4.  [Key Features In-Depth](#4-key-features-in-depth)
    1.  [4.1. Color Support](#41-color-support)
    2.  [4.2. Mouse Tracking](#42-mouse-tracking)
    3.  [4.3. Character Sets](#43-character-sets)
    4.  [4.4. Screen and Buffer Management](#44-screen-and-buffer-management)
    5.  [4.5. Sixel Graphics](#45-sixel-graphics)
    6.  [4.6. Bracketed Paste Mode](#46-bracketed-paste-mode)
5.  [Expanded API Reference](#5-expanded-api-reference)
    1.  [5.1. Lifecycle Functions](#51-lifecycle-functions)
    2.  [5.2. Host Input (Pipeline) Management](#52-host-input-pipeline-management)
    3.  [5.3. Keyboard and Mouse Output](#53-keyboard-and-mouse-output)
    4.  [5.4. Configuration and Mode Setting](#54-configuration-and-mode-setting)
    5.  [5.5. Callbacks](#55-callbacks)
    6.  [5.6. Diagnostics and Testing](#56-diagnostics-and-testing)
    7.  [5.7. Advanced Control](#57-advanced-control)
6.  [Internal Operations and Data Flow](#6-internal-operations-and-data-flow)
    1.  [6.1. Stage 1: Ingestion](#61-stage-1-ingestion)
    2.  [6.2. Stage 2: Consumption and Parsing](#62-stage-2-consumption-and-parsing)
    3.  [6.3. Stage 3: Character Processing and Screen Buffer Update](#63-stage-3-character-processing-and-screen-buffer-update)
    4.  [6.4. Stage 4: Rendering](#64-stage-4-rendering)
7.  [Data Structures Reference](#7-data-structures-reference)
    1.  [7.1. Enums](#71-enums)
    2.  [7.2. Core Structs](#72-core-structs)
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
    -   Flags for DEC special modes like double-width or double-height characters.
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

#### `VT_LEVEL_52`
This is the most basic level, emulating the DEC VT52.
-   **Features:**
    -   Simple cursor movement (`ESC A`, `ESC B`, `ESC C`, `ESC D`).
    -   Direct cursor addressing (`ESC Y r c`).
    -   Basic erasing (`ESC J`, `ESC K`).
    -   Alternate keypad mode.
    -   Simple identification sequence (`ESC Z`).
-   **Limitations:** No ANSI CSI sequences, no scrolling regions, no advanced attributes (bold, underline, etc.), no color.

#### `VT_LEVEL_100`
The industry-defining standard that introduced ANSI escape sequences.
-   **Features:**
    -   All VT52 features (when in VT52 mode, `ESC <`).
    -   **CSI Sequences:** The full range of `ESC [` commands for cursor control, erasing, etc.
    -   **SGR Attributes:** Basic graphic rendition (bold, underline, reverse, blink).
    -   **Scrolling Region:** `DECSTBM` (`CSI Pt;Pb r`) for defining a scrollable window.
    -   **Character Sets:** Support for DEC Special Graphics (line drawing).
    -   **Modes:** Auto-wrap (`DECAWM`), Application Cursor Keys (`DECCKM`).
-   **Limitations:** No color support beyond basic SGR attributes, no mouse tracking, no soft fonts.

#### `VT_LEVEL_220`
An evolution of the VT100, adding more international and customization features.
-   **Features:**
    -   All VT100 features.
    -   **Character Sets:** Adds DEC Multinational Character Set (MCS) and National Replacement Character Sets (NRCS).
    -   **Soft Fonts:** Support for Downloadable Fonts (`DECDLD`).
    -   **Function Keys:** User-Defined Keys (`DECUDK`) become available.
    -   **C1 Controls:** Full support for 7-bit and 8-bit C1 control codes.
-   **Limitations:** No Sixel graphics, no rectangular area operations.

#### `VT_LEVEL_320`
This level primarily adds raster graphics capabilities.
-   **Features:**
    -   All VT220 features.
    -   **Sixel Graphics:** The ability to render bitmap graphics using DCS Sixel sequences.

#### `VT_LEVEL_420`
Adds more sophisticated text and area manipulation features.
-   **Features:**
    -   All VT320 features.
    -   **Left/Right Margins:** `DECSLRM` (`CSI ? Pl;Pr s`) for horizontal scrolling regions.
    -   **Rectangular Area Operations:** Commands like `DECCRA` (Copy), `DECERA` (Erase), and `DECFRA` (Fill) for manipulating rectangular blocks of text.

#### `VT_LEVEL_XTERM` (Default)
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

#### Cursor Movement

| Command | Final | Name | Description                                    |
| :------ | :---- | :--- | :--------------------------------------------- |
| `CSI Pn A`| A | `CUU`| Cursor Up `Pn` lines.                          |
| `CSI Pn B`| B | `CUD`| Cursor Down `Pn` lines.                        |
| `CSI Pn C`| C | `CUF`| Cursor Forward `Pn` columns.                   |
| `CSI Pn D`| D | `CUB`| Cursor Back `Pn` columns.                      |
| `CSI Pn E`| E | `CNL`| Cursor Next Line `Pn` times.                   |
| `CSI Pn F`| F | `CPL`| Cursor Previous Line `Pn` times.               |
| `CSI Pn G`| G | `CHA`| Cursor Horizontal Absolute to column `Pn`.     |
| `CSI Pn;Pm H`| H | `CUP`| Cursor Position to row `Pn`, column `Pm`.      |
| `CSI Pn;Pm f`| f | `HVP`| Horizontal and Vertical Position (same as CUP).|
| `CSI Pn d`| d | `VPA`| Vertical Line Position Absolute to row `Pn`.   |
| `CSI s`   | s | `ANSISYSSC`| Save Cursor Position (ANSI.SYS compatible).    |
| `CSI u`   | u | `ANSISYSRC`| Restore Cursor Position (ANSI.SYS compatible).   |
| `ESC 7`   | - | `DECSC`| Save Cursor Position and attributes (DEC).     |
| `ESC 8`   | - | `DECRC`| Restore Cursor Position and attributes (DEC).  |

#### Erasing

| Command | Final | Name | Description                                           |
| :------ | :---- | :--- | :---------------------------------------------------- |
| `CSI Pn J`| J | `ED` | Erase in Display. `Pn=0` (default): cursor to end. `Pn=1`: start to cursor. `Pn=2`: entire screen. `Pn=3`: entire screen + scrollback (xterm). |
| `CSI Pn K`| K | `EL` | Erase in Line. `Pn=0` (default): cursor to end. `Pn=1`: start to cursor. `Pn=2`: entire line. |
| `CSI Pn X`| X | `ECH`| Erase `Pn` Characters starting at the cursor.       |
| `CSI Pn P`| P | `DCH`| Delete `Pn` Characters starting at the cursor.      |
| `CSI Pn @`| @ | `ICH`| Insert `Pn` blank Characters at the cursor.         |
| `CSI Pn L`| L | `IL` | Insert `Pn` blank Lines at the cursor.                |
| `CSI Pn M`| M | `DL` | Delete `Pn` Lines at the cursor.                      |

#### Scrolling

| Command | Final | Name | Description                                |
| :------ | :---- | :--- | :----------------------------------------- |
| `CSI Pn S`| S | `SU` | Scroll Up `Pn` lines.                      |
| `CSI Pn T`| T | `SD` | Scroll Down `Pn` lines.                    |
| `CSI Pt;Pb r`| r |`DECSTBM`| Set Top (`Pt`) and Bottom (`Pb`) Margins (Scrolling Region). |
| `CSI ? Pl;Pr s`| s |`DECSLRM`| Set Left (`Pl`) and Right (`Pr`) Margins (VT420+). |

#### Select Graphic Rendition (SGR)

| Command | Final | Name | Description                                      |
| :------ | :---- | :--- | :----------------------------------------------- |
| `CSI Pm m`| m | `SGR`| Sets display attributes. Multiple `Pm` codes can be combined. |

*Common SGR Parameters (`Pm`):*

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

#### Mode Setting

Modes are set with `h` and reset with `l`.

| Command (Set) | Command (Reset) | Name | Description                                           |
| :------------ | :-------------- | :--- | :---------------------------------------------------- |
| **ANSI Modes**|                 |      |                                                       |
| `CSI 4 h`     | `CSI 4 l`       | `IRM`| Insert/Replace Mode.                                  |
| `CSI 20 h`    | `CSI 20 l`      | `LNM`| Linefeed/New Line Mode (`LF` acts as `CRLF`).         |
| **DEC Private Modes** |                 |      |                                                       |
| `CSI ?1 h`    | `CSI ?1 l`      |`DECCKM`| Application Cursor Keys.                              |
| `CSI ?3 h`    | `CSI ?3 l`      |`DECCOLM`| 132 Column Mode.                                    |
| `CSI ?5 h`    | `CSI ?5 l`      |`DECSCNM`| Reverse Video Screen.                               |
| `CSI ?6 h`    | `CSI ?6 l`      | `DECOM`| Origin Mode (cursor is relative to scrolling region). |
| `CSI ?7 h`    | `CSI ?7 l`      |`DECAWM`| Auto-Wrap Mode.                                     |
| `CSI ?12 h`   | `CSI ?12 l`     | `-`    | Blinking Cursor.                                      |
| `CSI ?25 h`   | `CSI ?25 l`     |`DECTCEM`| Show/Hide Cursor.                                   |
| `CSI ?47 h`   | `CSI ?47 l`     | `-`    | Use Alternate Screen Buffer.                        |
| `CSI ?1049 h` | `CSI ?1049 l`   | `-`    | Use Alternate Screen, save/restore cursor (xterm).    |
| `CSI ?2004 h` | `CSI ?2004 l`   | `-`    | Bracketed Paste Mode.                               |
| `CSI ?1000 h` | `CSI ?1000 l`   | `-`    | Enable VT200 Mouse Tracking.                        |
| `CSI ?1002 h` | `CSI ?1002 l`   | `-`    | Enable Button-Event Mouse Tracking.                 |
| `CSI ?1003 h` | `CSI ?1003 l`   | `-`    | Enable Any-Event Mouse Tracking.                    |
| `CSI ?1004 h` | `CSI ?1004 l`   | `-`    | Enable Focus In/Out reporting.                      |
| `CSI ?1006 h` | `CSI ?1006 l`   | `-`    | Enable SGR Extended Mouse Reporting.                |

#### Status Reports

| Command | Final | Name | Description                                          | Response                                |
| :------ | :---- | :--- | :--------------------------------------------------- | :-------------------------------------- |
| `CSI c` | c | `DA` | Primary Device Attributes.                           | `CSI ?{level};...c`                     |
| `CSI >c`| c | `DA2`| Secondary Device Attributes.                         | `CSI >{id};{ver};0c`                     |
| `CSI 5 n` | n | `DSR`| Status Report (device is OK).                        | `CSI 0 n`                               |
| `CSI 6 n` | n | `DSR-CPR`| Cursor Position Report.                          | `CSI {row};{col} R`                      |
| `CSI ?15 n`| n | `DSR-PP`| Printer Port Status.                             | `CSI ?10 n` (Ready) or `CSI ?13 n` (Not Ready) |
| `CSI ?26 n`| n | `DSR-KBD`| Keyboard Dialect Report.                         | `CSI ?27;{dialect} n`                    |
| `CSI ?63 n`| n | `DSR-CRC`| Request Screen Checksum.                         | `CSI ?63;{page};{alg};{checksum} n`       |

### 3.4. OSC - Operating System Command (`ESC ]`)

OSC sequences are used for interacting with the host operating system or terminal window manager. They are terminated by a Bell (`BEL`, `0x07`) or a String Terminator (`ST`, `ESC \`).

| Command (`Ps`) | Description                                | Example                                   |
| :------------- | :----------------------------------------- | :---------------------------------------- |
| `0`            | Set Window and Icon Title                  | `ESC ] 0 ; My Window Title BEL`             |
| `1`            | Set Icon Title                             | `ESC ] 1 ; Icon Name BEL`                 |
| `2`            | Set Window Title                           | `ESC ] 2 ; My Window Title BEL`             |
| `4;c;spec`     | Set/Query color `c`. `spec` is `rgb:R/G/B`.| `ESC ] 4 ; 1 ; rgb:FF/00/00 BEL` (Set red)  |
| `10`           | Set/Query Dynamic Foreground Color         | `ESC ] 10 ; ? BEL` (Query fg)               |
| `11`           | Set/Query Dynamic Background Color         | `ESC ] 11 ; cyan BEL` (Set bg to cyan)      |
| `12`           | Set/Query Cursor Color                     | `ESC ] 12 ; white BEL` (Set cursor to white)|
| `52;c;data`    | Set/Query Clipboard. `c`=clipboard,`data`=base64. | `ESC ] 52 ; c ; ? BEL` (Query clipboard)|
| `104;c`        | Reset color `c`. `c` is a `;` separated list. | `ESC ] 104 ; 1;2;3 BEL` (Reset colors 1-3)|

### 3.5. DCS - Device Control String (`ESC P`)

DCS sequences are for device-specific commands and are terminated by `ST`.

| Command | Name | Description                                |
| :------ | :--- | :----------------------------------------- |
| `DCS 1;1|... ST` | `DECUDK` | Program User-Defined Keys. Data is `key/hex;...` |
| `DCS 0;1|... ST` | `DECUDK` | Clear User-Defined Keys.                   |
| `DCS 2;1|... ST` | `DECDLD` | Download Soft Font.                        |
| `DCS $q... ST`   |`DECRQSS`| Request Status String (e.g., `m` for SGR). |
| `DCS +q... ST`   |`XTGETTCAP`| Request Termcap/Terminfo string (xterm). |
| `DCS ...q ... ST`| `SIXEL`  | Sixel Graphics data.                       |

### 3.6. Other Escape Sequences

| Sequence | Name | Description                                  |
| :------- | :--- | :------------------------------------------- |
| `ESC c`  | `RIS`| Hard Reset: Resets the terminal to its initial state. |
| `ESC =`  |`DECKPAM`| Keypad Application Mode.                   |
| `ESC >`  |`DECKPNM`| Keypad Numeric Mode.                       |
| `ESC (` C | `SCS`| Select G0 Character Set. `C` identifies the set. |
| `ESC )` C | `SCS`| Select G1 Character Set.                     |
| `ESC *` C | `SCS`| Select G2 Character Set.                     |
| `ESC +` C | `SCS`| Select G3 Character Set.                     |

### 3.7. VT52 Mode Sequences

When in VT52 mode (`SetVTLevel(VT_LEVEL_52)` or `ESC <`), a different, simpler set of commands is used.

| Command | Description                  |
| :------ | :--------------------------- |
| `ESC A` | Cursor Up                    |
| `ESC B` | Cursor Down                  |
| `ESC C` | Cursor Right                 |
| `ESC D` | Cursor Left                  |
| `ESC H` | Cursor Home                  |
| `ESC Y r c`| Direct Cursor Address (row `r`, col `c`) |
| `ESC I` | Reverse Line Feed            |
| `ESC J` | Erase to End of Screen       |
| `ESC K` | Erase to End of Line         |
| `ESC Z` | Identify (`ESC / Z`)         |
| `ESC =` | Enter Alternate Keypad Mode  |
| `ESC >` | Exit Alternate Keypad Mode   |
| `ESC <` | Enter ANSI Mode              |
| `ESC F` | Enter Graphics Mode          |
| `ESC G` | Exit Graphics Mode           |

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

## 5. Expanded API Reference

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

## 7. Internal Operations and Data Flow

This chapter provides a deeper, narrative look into the internal mechanics of the library, tracing the journey of a single character from its arrival in the input pipeline to its final rendering on the screen.

### 7.1. Stage 1: Ingestion

1.  **Entry Point:** A host application calls `PipelineWriteString("ESC[31mHello")`.
2.  **Buffering:** Each character of the string (`E`, `S`, `C`, `[`, `3`, `1`, `m`, `H`, `e`, `l`, `l`, `o`) is sequentially written into the `input_pipeline`, a large circular byte buffer. The `pipeline_head` index advances with each write.

### 7.2. Stage 2: Consumption and Parsing

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

### 7.3. Stage 3: Character Processing and Screen Buffer Update

1.  **`H`, `e`, `l`, `l`, `o`:** The parser is now back in `VT_PARSE_NORMAL`. `ProcessNormalChar()` is called for each of these characters.
2.  **Placement:** For the character 'H':
    -   The function checks the cursor's current position (`terminal.cursor.x`, `terminal.cursor.y`).
    -   It retrieves the `EnhancedTermChar` struct at that position in the `screen` buffer.
    -   It sets that struct's `ch` field to `'H'`.
    -   Crucially, it copies the *current* terminal attributes into the cell's attributes: `cell->fg_color` gets the value of `terminal.current_fg` (which we just set to red).
    -   The `dirty` flag for this cell is set to `true`.
    -   The cursor's X position is incremented: `terminal.cursor.x++`.
3.  This process repeats for 'e', 'l', 'l', 'o', each time placing the character, applying the current SGR attributes (red foreground), and advancing the cursor.

### 7.4. Stage 4: Rendering

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

## 8. Data Structures Reference

This section provides an exhaustive reference to the core data structures and enumerations used within `terminal.h`. A deep understanding of these structures is essential for advanced integration, debugging, or extending the library's functionality.

### 8.1. Enums

#### 8.1.1. `VTLevel`

Determines the terminal's emulation compatibility level, affecting which escape sequences are recognized and how the terminal identifies itself.

| Value | Description |
| :--- | :--- |
| `VT_LEVEL_52` | Emulates the DEC VT52, a basic glass teletype with a simple command set. |
| `VT_LEVEL_100` | Emulates the DEC VT100, the foundational standard for ANSI escape sequences. |
| `VT_LEVEL_220` | Emulates the DEC VT220, adding features like soft fonts and more character sets. |
| `VT_LEVEL_320` | Emulates the DEC VT320, notably introducing Sixel graphics support. |
| `VT_LEVEL_420` | Emulates the DEC VT420, adding rectangular area operations and left/right margins. |
| `VT_LEVEL_XTERM` | Emulates a modern xterm terminal, the de facto standard with the widest feature support, including True Color, advanced mouse tracking, and numerous extensions. This is the default level. |

#### 8.1.2. `VTParseState`

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

#### 8.1.3. `MouseTrackingMode`

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

#### 8.1.4. `CursorShape`

Defines the visual style of the text cursor.

| Value | Description |
| :--- | :--- |
| `CURSOR_BLOCK`, `CURSOR_BLOCK_BLINK` | A solid or blinking rectangle covering the entire character cell. |
| `CURSOR_UNDERLINE`, `CURSOR_UNDERLINE_BLINK` | A solid or blinking line at the bottom of the character cell. |
| `CURSOR_BAR`, `CURSOR_BAR_BLINK` | A solid or blinking vertical line at the left of the character cell. |

#### 8.1.5. `CharacterSet`

Represents a character encoding standard that can be mapped to one of the G0-G3 slots.

| Value | Description |
| :--- | :--- |
| `CHARSET_ASCII` | Standard US ASCII. |
| `CHARSET_DEC_SPECIAL` | DEC Special Graphics and Line Drawing character set. |
| `CHARSET_UK` | UK National character set (replaces '#' with '£'). |
| `CHARSET_DEC_MULTINATIONAL`| DEC Multinational Character Set (MCS). |
| `CHARSET_ISO_LATIN_1` | ISO 8859-1 character set. |
| `CHARSET_UTF8` | UTF-8 encoding (requires multi-byte processing). |

### 8.2. Core Structs

#### 8.2.1. `Terminal`

This is the master struct that encapsulates the entire state of the terminal emulator.

-   `EnhancedTermChar screen[H][W]`, `alt_screen[H][W]`: The primary and alternate screen buffers.
-   `EnhancedCursor cursor`, `saved_cursor`: The current and saved cursor states.
-   `VTConformance conformance`: Tracks the current VT level and feature flags.
-   `DECModes dec_modes`, `ANSIModes ansi_modes`: Structs containing boolean flags for all terminal modes (e.g., `application_cursor_keys`, `auto_wrap_mode`).
-   `ExtendedColor current_fg`, `current_bg`: The currently selected foreground and background colors for new text.
-   `bool bold_mode`, `italic_mode`, etc.: The currently active SGR attributes.
-   `int scroll_top`, `scroll_bottom`, `left_margin`, `right_margin`: Defines the active scrolling region and margins.
-   `CharsetState charset`: Manages the G0-G3 character sets and the active GL/GR mappings.
-   `TabStops tab_stops`: Stores the positions of all horizontal tab stops.
-   `BracketedPaste bracketed_paste`: State for the bracketed paste mode.
-   `SixelGraphics sixel`: Stores data for any active Sixel image.
-   `TitleManager title`: Holds the window and icon titles.
-   `struct mouse`: Contains the complete state of mouse tracking, including mode, button states, and last known position.
-   `unsigned char input_pipeline[]`: The circular buffer for incoming host data.
-   `struct vt_keyboard`: Contains the state of the keyboard, including modes and the output event buffer.
-   `VTParseState parse_state`: The current state of the main parser.
-   `char escape_buffer[]`, `int escape_params[]`: Buffers for parsing escape sequences.

#### 8.2.2. `EnhancedTermChar`

Represents a single character cell on the screen, storing all of its visual and metadata attributes.

-   `unsigned int ch`: The Unicode codepoint of the character in the cell.
-   `ExtendedColor fg_color`, `bg_color`: The foreground and background colors for this specific cell.
-   `bool bold`, `faint`, `italic`, `underline`, `blink`, `reverse`, `strikethrough`, `conceal`, `overline`, `double_underline`: A complete set of flags for all standard SGR attributes.
-   `bool double_width`, `double_height_top`, `double_height_bottom`: Flags for DEC line attributes.
-   `bool protected_cell`: Flag for the DECSCA attribute, protecting the cell from erasure.
-   `bool dirty`: A rendering hint flag, indicating that this cell has changed and needs to be redrawn.

#### 8.2.3. `ExtendedColor`

A flexible structure for storing color information, capable of representing both indexed and true-color values.

-   `int color_mode`: A flag indicating the color representation. `0` for indexed palette, `1` for RGB.
-   `union value`:
    -   `int index`: If `color_mode` is `0`, this stores the 256-color palette index.
    -   `RGB_Color rgb`: If `color_mode` is `1`, this struct stores the 24-bit R, G, B values.

#### 8.2.4. `VTKeyEvent`

A structure containing a fully processed keyboard event, ready to be sent back to the host application.

-   `int key_code`: The original Raylib key code that generated the event.
-   `bool ctrl`, `shift`, `alt`, `meta`: The state of the modifier keys at the time of the press.
-   `char sequence[32]`: The final, translated byte sequence to be sent to the host (e.g., `"A"`, `"\x1B[A"`, `"\x01"` for Ctrl+A).

---

## 9. Configuration Constants

These `#define` constants, located at the top of `terminal.h`, allow for compile-time configuration of the terminal's default behaviors and resource limits. To change them, you must define them before the `#include "terminal.h"` line where the implementation is defined.

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

## 10. License

`terminal.h` is licensed under the MIT License.

