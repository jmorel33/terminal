# terminal.h - Technical Reference Manual

**(c) 2025 Jacques Morel**

This document provides an exhaustive technical reference for `terminal.h`, an enhanced single-header terminal emulation library for C. It is intended for developers integrating the library into their applications and those who need a deep understanding of its capabilities, supported protocols, and internal architecture.

<details>
<summary>Table of Contents</summary>

1.  [Overview](#1-overview)
    1.  [1.1. Description](#11-description)
    2.  [1.2. Key Features](#12-key-features)
    3.  [1.3. Architecture](#13-architecture)
2.  [Compliance and Emulation Levels](#2-compliance-and-emulation-levels)
    1.  [2.1. Supported Standards](#21-supported-standards)
    2.  [2.2. Setting Compliance Level](#22-setting-compliance-level)
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
5.  [API Reference](#5-api-reference)
    1.  [5.1. Lifecycle Functions](#51-lifecycle-functions)
    2.  [5.2. Input/Output Functions](#52-inputoutput-functions)
    3.  [5.3. Configuration Functions](#53-configuration-functions)
    4.  [5.4. Callback Functions](#54-callback-functions)
    5.  [5.5. Diagnostic Functions](#55-diagnostic-functions)
6.  [Data Structures](#6-data-structures)
7.  [Configuration Constants](#7-configuration-constants)
8.  [License](#8-license)

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

### 1.3. Architecture

The library is built around a central `Terminal` struct which encapsulates the entire state of the emulated device.

1.  **Input Pipeline:** The application feeds character data into an internal buffer using `PipelineWrite...()` functions.
2.  **Processing Loop:** `UpdateTerminal()` is the main tick function. It consumes data from the pipeline, processes a certain number of characters per frame (tunable for performance), and updates internal timers (e.g., for cursor blinking).
3.  **State Machine Parser:** A state machine (`VTParseState`) reads the input stream. Normal characters are placed on the screen, while control characters (like `ESC`) transition the parser into states for handling multi-byte escape sequences (CSI, OSC, DCS, etc.).
4.  **Screen Buffer:** The terminal maintains one or two `EnhancedTermChar` grids (primary and alternate screen buffers). This structure stores not just the character code but also all its attributes (colors, bold, underline, etc.).
5.  **Rendering:** `DrawTerminal()` iterates over the active screen buffer and uses Raylib to render each character cell to the window, applying all visual attributes.
6.  **Keyboard/Mouse Handling:** `UpdateVTKeyboard()` and `UpdateMouse()` use Raylib to capture user input and translate it into the appropriate VT byte sequences, which are then typically sent back to the host application via a callback.

---

## 2. Compliance and Emulation Levels

### 2.1. Supported Standards

The library can be configured to emulate the feature sets of the following terminal standards:

-   **VT52:** A basic glass teletype with direct cursor addressing.
-   **VT100:** The industry-defining standard, introducing ANSI escape sequences (CSI).
-   **VT220:** Added more character sets (DEC Multinational), soft fonts (DECDLD), and function key improvements.
-   **VT320:** Introduced Sixel graphics support.
-   **VT420:** Added scrolling margins (left/right) and rectangular area operations.
-   **xterm:** A de facto standard for modern terminal emulators, adding numerous extensions such as 256-color and True Color support, advanced mouse reporting, and window manipulation sequences.

### 2.2. Setting Compliance Level

The compliance level determines which sequences are recognized and how the terminal identifies itself in response to requests like Device Attributes (DA).

-   **Function:** `void SetVTLevel(VTLevel level);`
-   **Enum `VTLevel`:**
    -   `VT_LEVEL_52`
    -   `VT_LEVEL_100`
    -   `VT_LEVEL_220`
    -   `VT_LEVEL_320`
    -   `VT_LEVEL_420`
    -   `VT_LEVEL_XTERM` (Default)

Setting the level automatically configures internal feature flags and the DA response strings.

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

## 5. API Reference

This section details the public API for interacting with the terminal library.

### 5.1. Lifecycle Functions

-   `void InitTerminal(void);`
    Initializes the terminal state, sets up screen buffers, default modes, and creates the font texture. Must be called after `InitWindow()`.
-   `void CleanupTerminal(void);`
    Frees all resources used by the terminal, including the font texture.
-   `void UpdateTerminal(void);`
    The main update function. Processes the input pipeline, updates timers, and handles rendering. Call once per frame.
-   `void DrawTerminal(void);`
    Renders the terminal state to the screen. Call within a Raylib `BeginDrawing()`/`EndDrawing()` block.

### 5.2. Input/Output Functions

-   `bool PipelineWriteChar(unsigned char ch);`
-   `bool PipelineWriteString(const char* str);`
-   `bool PipelineWriteFormat(const char* format, ...);`
    Functions to write data to the terminal's input buffer for processing.
-   `bool GetVTKeyEvent(VTKeyEvent* event);`
    Retrieves a processed keyboard event from the keyboard buffer. The application should call this to get input to send to the host.

### 5.3. Configuration Functions

-   `void SetVTLevel(VTLevel level);`
-   `VTLevel GetVTLevel(void);`
    Set or get the current terminal emulation level.
-   `void SetTerminalMode(const char* mode_name, bool enable);`
-   `void SetCursorShape(CursorShape shape);`
-   `void SetMouseTracking(MouseTrackingMode mode);`
-   `void EnableBracketedPaste(bool enable);`
    Functions to programmatically configure various terminal behaviors.

### 5.4. Callback Functions

-   `void SetResponseCallback(ResponseCallback callback);`
    `typedef void (*ResponseCallback)(const char* response, int length);`
    Sets the callback function for the terminal to send data back to the host (e.g., status reports, mouse events, keyboard input).
-   `void SetTitleCallback(TitleCallback callback);`
-   `void SetBellCallback(BellCallback callback);`

### 5.5. Diagnostic Functions

-   `void EnableDebugMode(bool enable);`
    Toggles verbose logging of unsupported sequences.
-   `TerminalStatus GetTerminalStatus(void);`
    Returns a struct with information about buffer usage and performance.
-   `void RunVTTest(const char* test_name);`
    Runs pre-defined test sequences ("cursor", "colors", "all", etc.) to verify functionality.
-   `void ShowTerminalInfo(void);`
    Displays a summary of the current terminal state on the screen.

---

## 6. Data Structures

-   **`Terminal`**: The primary struct holding the entire state of the terminal.
-   **`EnhancedTermChar`**: Represents a single cell on the screen, storing its character, colors, and attributes (bold, underline, etc.).
-   **`VTParseState`**: Enum that tracks the current state of the escape sequence parser.
-   **`ExtendedColor`**: A struct that can hold either a 256-palette index or a 24-bit RGB value.
-   **`VTKeyEvent`**: A struct containing a processed keyboard event, including modifier states and the final VT sequence to be sent.

---

## 7. Configuration Constants

These `_#define_` constants at the top of `terminal.h` can be modified before including the implementation to change default behaviors:

-   `DEFAULT_TERM_WIDTH`, `DEFAULT_TERM_HEIGHT`: Default terminal dimensions in character cells.
-   `DEFAULT_CHAR_WIDTH`, `DEFAULT_CHAR_HEIGHT`: Pixel dimensions of a single character glyph.
-   `MAX_ESCAPE_PARAMS`: Maximum number of parameters in a CSI sequence.
-   `OUTPUT_BUFFER_SIZE`: Size of the buffer for responses to the host.

---

## 8. License

`terminal.h` is licensed under the MIT License.
