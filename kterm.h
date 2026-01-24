// kterm.h - K-Term Library Implementation v2.2.21
// Comprehensive VT52/VT100/VT220/VT320/VT420/VT520/xterm compatibility with modern features

/**********************************************************************************************
*
*   kterm.h - K-Term Emulation Library
*   (c) 2025 Jacques Morel
*
*   DESCRIPTION:
*       This library provides a comprehensive terminal emulation solution, aiming for compatibility with VT52, VT100, VT220, VT320, VT420, VT520, and xterm standards,
*       while also incorporating modern features like true color support, Sixel graphics, advanced mouse tracking, and bracketed paste mode. It is designed to be
*       integrated into applications that require a text-based terminal interface, using the KTerm Platform for rendering, input, and window management.
*
*       v2.2.21 Update:
*         - Thread Safety: Implemented Phase 2 (Lock-Free Input Pipeline).
*         - Architecture: Converted input ring buffer to a Single-Producer Single-Consumer (SPSC) lock-free queue using C11 atomics.
*         - Performance: Enabled safe high-throughput input injection from background threads without locking.
*
*       v2.2.20 Update:
*         - Gateway: Enhanced `PIPE;BANNER` with extended parameters (TEXT, FONT, ALIGN, GRADIENT).
*         - Features: Support for font switching, text alignment (Left/Center/Right), and RGB color gradients in banners.
*
*       v2.2.19 Update:
*         - Gateway: Added `PIPE;BANNER` command to generate large text banners using the current font's glyph data.
*         - Features: Injects rendered ASCII-art banners back into the input pipeline for display.
*
*       v2.2.18 Update:
*         - Typography: Added KTermFontMetric structure and automatic metric calculation (width, bounds) for bitmap fonts.
*         - Features: Implemented KTerm_CalculateFontMetrics to support precise glyph positioning and future proportional rendering.
*
*       v2.2.17 Update:
*         - Safety: Refactored KTerm_Update and KTerm_WriteChar for thread safety (Phase 1).
*         - Architecture: Decoupled background session processing from global active_session state.
*
*       v2.2.16 Update:
*         - Compliance: Implemented DEC Printer Extent Mode (DECPEX - Mode 19).
*         - Compliance: Implemented DEC Print Form Feed Mode (DECPFF - Mode 18).
*         - Compliance: Implemented Allow 80/132 Column Mode (Mode 40).
*         - Compliance: Implemented DEC Locator Enable (DECELR - Mode 41).
*         - Compliance: Implemented Alt Screen Cursor Save Mode (Mode 1041).
*
*       v2.2.15 Update:
*         - Compliance: Implemented Sixel Display Mode (DECSDM - Mode 80) to toggle scrolling behavior.
*         - Compliance: Implemented Sixel Cursor Mode (Private Mode 8452) for cursor placement after graphics.
*         - Compliance: Implemented Checksum Reporting (DECECR) and gated DECRQCRA appropriately.
*         - Compliance: Added Extended Edit Mode (DECEDM - Mode 45) state tracking.
*
*       v2.2.14 Update:
*         - Compatibility: Implemented ANSI/VT52 Mode Switching (DECANM - Mode 2).
*         - Input: Implemented Backarrow Key Mode (DECBKM - Mode 67) to toggle BS/DEL.
*
*       v2.2.13 Update:
*         - Compliance: Implemented VT420 Left/Right Margin Mode (DECLRMM - Mode 69).
*         - Compliance: Fixed DECSLRM syntax to respect DECLRMM status.
*         - Compliance: Implemented DECCOLM (Mode 3) resizing (80/132 cols) and DECNCSM (Mode 95).
*         - Compliance: Corrected DECRQCRA syntax to `CSI ... * y`.
*         - Features: Added DECNKM (Mode 66) for switching between Numeric/Application Keypad modes.
*
*       v2.2.12 Update:
*         - Safety: Added robust memory allocation checks (OOM handling) in core initialization and resizing paths.
*         - Features: Implemented OSC 50 (Set Font) support via `KTerm_LoadFont`.
*         - Refactor: Cleaned up internal function usage for cursor and tab stop management.
*         - Fix: Refactored `ExecuteDECSCUSR` and `ClearCSIParams` to improve maintainability.
*
*       v2.2.11 Update:
*         - Features: Implemented Rich Underline Styles (Curly, Dotted, Dashed) via SGR 4:x subparameters.
*         - Standards: Added support for XTPUSHSGR (CSI # {) and XTPOPSGR (CSI # }) to save/restore text attributes on a stack.
*         - Parser: Enhanced CSI parser to handle colon (:) separators for subparameters (e.g. 4:3 for curly underline).
*         - Visuals: Compute shader now renders distinct patterns for different underline styles.
*
*       v2.2.10 Update:
*         - Features: Added separate colors for Underline and Strikethrough attributes.
*         - Standards: Implemented SGR 58 (Set Underline Color) and SGR 59 (Reset Underline Color).
*         - Gateway: Added `SET;ATTR` keys `UL` and `ST` for setting colors.
*         - Gateway: Added `GET;UNDERLINE_COLOR` and `GET;STRIKE_COLOR`.
*         - Visuals: Render engine now draws colored overlays for these attributes.
*
*       v2.2.9 Update:
*         - Gateway: Added `SET;CONCEAL` to control the character used for concealed text.
*         - Visuals: When conceal attribute is set, renders specific character code if defined, otherwise hides text.
*
*       v2.2.8 Update:
*         - Features: Added Debug Grid visualization.
*         - Gateway: Added `SET;GRID` to control grid activation, color, and transparency.
*         - Visuals: Renders a 1-pixel box around every character cell when enabled.
*
*       v2.2.7 Update:
*         - Features: Added mechanism to enable/disable terminal output via API and Gateway.
*         - Gateway: Added `SET;OUTPUT` (ON/OFF) and `GET;OUTPUT`.
*         - API: Added `KTerm_SetResponseEnabled`.
*
*       v2.2.6 Update:
*         - Gateway: Expanded `KTERM` class with `SET` and `RESET` commands for Attributes and Blink Rates.
*         - Features: `SET;ATTR;KEY=VAL` allows programmatic control of text attributes (Bold, Italic, Colors, etc.).
*         - Features: `SET;BLINK;FAST=slot;SLOW=slot;BG=slot` allows fine-tuning blink oscillators per session using oscillator slots.
*         - Features: `RESET;ATTR` and `RESET;BLINK` restore defaults.
*         - Architecture: Decoupled background blink oscillator from slow blink for independent control.
*
*       v2.2.5 Update:
*         - Visuals: Implemented independent blink flavors (Fast/Slow/Background) via SGR 5, 6, and 105.
*         - Emulation: Added `KTERM_ATTR_BLINK_BG` and `KTERM_ATTR_BLINK_SLOW` attributes.
*         - SGR 5 (Classic): Triggers both Fast Blink and Background Blink.
*         - SGR 6 (Slow): Triggers Slow Blink (independent speed).
*         - SGR 66: Triggers Background Blink.
*
*       v2.2.4 Update:
*         - Optimization: Refactored `EnhancedTermChar` and `KTermSession` to use bit flags (`uint32_t`) for character attributes instead of multiple booleans.
*         - Performance: Reduced memory footprint per cell and simplified GPU data transfer logic.
*         - Refactor: Updated SGR (Select Graphic Rendition), rendering, and state management logic to use the new bitmask system.
*
*       v2.2.3 Update:
*         - Architecture: Refactored `TabStops` to use dynamic memory allocation for arbitrary terminal widths (>256 columns).
*         - Logic: Fixed `NextTabStop` to strictly respect defined stops and margins, removing legacy fallback logic.
*         - Compliance: Improved behavior when tabs are cleared (TBC), ensuring cursor jumps to margin/edge.
*
*       v2.2.2 Update:
*         - Emulation: Added "IBM DOS ANSI" mode (ANSI.SYS compatibility) via `VT_LEVEL_ANSI_SYS`.
*         - Visuals: Implemented authentic CGA 16-color palette enforcement in ANSI mode.
*         - Compatibility: Added support for ANSI.SYS specific behaviors (Cursor Save/Restore, Line Wrap).
*
*       v2.2.1 Update:
*         - Protocol: Added Gateway Protocol SET/GET commands for Fonts, Size, and Level.
*         - Fonts: Added dynamic font support with automatic centering/padding (e.g. for IBM font).
*         - Fonts: Expanded internal font registry with additional retro fonts.
*
*       v2.2.0 Major Update:
*         - Graphics: Kitty Graphics Protocol Phase 4 Complete (Animations & Compositing).
*         - Animation: Implemented multi-frame image support with delay timers.
*         - Compositing: Full Z-Index support. Images can now be layered behind or in front of text.
*         - Transparency: Default background color (index 0) is now rendered transparently to allow background images to show through.
*         - Pipeline: Refactored rendering pipeline to use explicit clear, background, text, and foreground passes.
*
*       v2.1.9 Update:
*         - Graphics: Finalized Kitty Graphics Protocol integration (Phase 3 Complete).
*         - Render: Implemented image scrolling logic (`start_row` anchoring) and clipping to split panes.
*         - Defaults: Added smart default placement logic (current cursor position) when x/y coordinates are omitted.
*         - Fix: Resolved GLSL/C struct alignment mismatch for clipping rectangle in `texture_blit.comp` pipeline.
*
*       v2.1.8 Update:
*         - Graphics: Completed Phase 3 of v2.2 Multiplexer features (Kitty Graphics Protocol).
*         - Rendering: Added `texture_blit.comp` shader pipeline for compositing Kitty images onto the terminal grid.
*         - Features: Implemented chunked transmission (`m=1`) and placement commands (`a=p`, `a=T`).
*         - Safety: Added `complete` flag to `KittyImageBuffer` to prevent partial rendering of images during upload.
*         - Cleanup: Fixed global resource cleanup to iterate all sessions and ensure textures/buffers are freed.
*
*       v2.1.7 Update:
*         - Graphics: Implemented Phase 3 of v2.2 Multiplexer features (Kitty Graphics Protocol).
*         - Parsing: Added `PARSE_KITTY` state machine for `ESC _ G ... ST` sequences.
*         - Features: Support for transmitting images (`a=t`), deleting images (`a=d`), and querying (`a=q`).
*         - Memory: Implemented `KittyImageBuffer` for managing image resources per session.
*
*       v2.1.6 Update:
*         - Architecture: Implemented Phase 2 of v2.2 Multiplexer features (Compositor).
*         - Rendering: Refactored rendering loop to support recursive pane layouts.
*         - Performance: Optimized row rendering with persistent scratch buffers and dirty row tracking.
*         - Rendering: Updated cursor logic to support independent cursors based on focused pane.
*
*       v2.1.5 Update:
*         - Architecture: Implemented Phase 1 of v2.2 Multiplexer features.
*         - Layout: Introduced `KTermPane` recursive tree structure for split management.
*         - API: Added `KTerm_SplitPane` and `SessionResizeCallback`.
*         - Refactor: Updated `KTerm_Resize` to recursively recalculate layout geometry.
*
*       v2.1.4 Update:
*         - Config: Increased MAX_SESSIONS to 4 to match VT525 spec.
*         - Optimization: Optimized KTerm_Resize using safe realloc for staging buffers.
*         - Documentation: Clarified KTerm_GetKey usage for input interception.
*
*       v2.1.3 Update:
*         - Fix: Robust parsing of string terminators (OSC/DCS/APC/PM/SOS) to handle implicit ESC termination.
*         - Fix: Correct mapping of Unicode codepoints to the base CP437 font atlas (preventing Latin-1 mojibake).
*
*       v2.1.2 Update:
*         - Fix: Dynamic Answerback string based on configured VT level.
*         - Feature: Complete reporting of supported features in KTerm_ShowInfo.
*         - Graphics: Implemented HLS to RGB color conversion for Sixel graphics.
*         - Safety: Added warning for unsupported ReGIS alphabet selection (L command).
*
*       v2.1.1 Update:
*         - ReGIS: Implemented Resolution Independence (S command, dynamic scaling).
*         - ReGIS: Added support for Screen command options 'E' (Erase) and 'A' (Addressing).
*         - ReGIS: Improved parser to handle nested brackets in S command.
*         - ReGIS: Refactored drawing primitives to respect logical screen extents.
*
*       v2.0.9 Update:
*         - Architecture: Full "Situation Decoupling" (Phase 4 complete).
*         - Refactor: Removed direct Situation library dependencies from core headers using `kterm_render_sit.h` abstraction.
*         - Clean: Removed binary artifacts and solidified platform aliases.
*
*       v2.0.8 Update:
*         - Refactor: "Situation Decoupling" (Phase 2) via aliasing (`kterm_render_sit.h`).
*         - Architecture: Core library now uses `KTerm*` types, decoupling it from direct Situation dependency.
*         - Fix: Replaced hardcoded screen limits with dynamic resizing logic.
*         - Fix: Restored Printer Controller (`ExecuteMC`) and ReGIS scaling logic.
*
*       v2.0.7 Update:
*         - Refactor: "Input Decoupling" - Separated keyboard/mouse handling into `kterm_io_sit.h`.
*         - Architecture: Core library `kterm.h` is now input-agnostic, using a generic `KTermEvent` pipeline.
*         - Adapter: Added `kterm_io_sit.h` as a reference implementation for Situation library input.
*         - Safety: Explicit session context passing in internal functions.
*
*       v2.0.6 Update:
*         - Visuals: Replaced default font with authentic "DEC VT220 8x10" font for Museum-Grade emulation accuracy.
*         - Accuracy: Implemented precise G0/G1 Special Graphics translation using standard VT Look-Up Table (LUT) logic.
*         - IQ: Significantly improved text clarity and aspect ratio by aligning render grid with native font metrics (8x10).
*
*       v2.0.5 Update:
*         - Refactor: Extracted compute shaders to external files (`shaders/*.comp`) and implemented runtime loading.
*
*       v2.0.4 Update:
*         - Support: Fix VT-520 and VT-525 number of sessions from 3 to 4.
*
*       v2.0.3 Update:
*         - Refactor: Explicit session pointers in internal processing functions (APC, PM, SOS, OSC, DCS, Sixel).
*         - Reliability: Removed implicit session state lookup in command handlers for better multi-session safety.
*
*       v2.0.2 Update:
*         - Fix: Session context fragility in event processing loop.
*         - Fix: Sixel buffer overflow protection (dynamic resize).
*         - Fix: Shader SSBO length bounds check for driver compatibility.
*         - Opt: Clock algorithm for Font Atlas eviction.
*         - Fix: UTF-8 decoder state reset on errors.
*
*       v2.0.1 Update:
*         - Fix: Heap corruption in SSBO update on resize.
*         - Fix: Pipeline corruption in multi-session switching.
*         - Fix: History preservation on resize.
*         - Fix: Thread-safe ring buffer logic.
*         - Fix: ReGIS B-Spline stability.
*         - Fix: UTF-8 invalid start byte handling.
*
*       v2.0 Feature Update:
*         - Multi-Session: Full VT520 session management (DECSN, DECRSN, DECRS) and split-screen support (DECSASD, DECSSDT).
*                          Supports up to 4 sessions as defined by the selected VT level (e.g., VT520=4, VT420=2, VT100=1).
*         - Architecture: Thread-safe, instance-based API refactoring (`KTerm*` handle).
*         - Safety: Robust buffer handling with `StreamScanner` and strict UTF-8 decoding.
*         - Unicode: Strict UTF-8 validation with visual error feedback (U+FFFD) and fallback rendering.
*         - Portability: Replaced GNU computed gotos with standard switch-case dispatch.
*
*       v1.5 Feature Update:
*         - Internationalization: Full ISO 2022 & NRCS support with robust lookup tables.
*         - Standards: Implementation of Locking Shifts (LS0-LS3) for G0-G3 charset switching.
*         - Rendering: Dynamic UTF-8 Glyph Cache replacing fixed CP437 textures.
*
*       v1.4 Feature Update:
*         - Graphics: Complete ReGIS (Remote Graphics Instruction Set) implementation.
*         - Vectors: Support for Position (P), Vector (V), and Curve (C) commands including B-Splines.
*         - Advanced: Polygon Fill (F), Macrographs (@), and custom Alphabet Loading (L).
*
*       v1.3 Feature Update:
*         - Session Management: Multi-session support (up to 4 sessions) mimicking VT520.
*         - Split Screen: Horizontal split-screen compositing of two sessions.
*
*       v1.2 Feature Update:
*         - Rendering Optimization: Dirty row tracking to minimize GPU uploads.
*         - Usability: Mouse text selection and clipboard copying (with UTF-8 support).
*         - Visuals: Retro CRT shader effects (curvature and scanlines).
*         - Robustness: Enhanced UTF-8 error handling.
*
*       v1.1 Major Update:
*         - Rendering engine rewritten to use a Compute Shader pipeline via Shader Storage Buffer Objects (SSBO).
*         - Full integration with the KTerm Platform for robust resource management and windowing.
*
*       The library processes a stream of input characters (typically from a host application or PTY) and updates an internal screen buffer. This buffer,
*       representing the terminal display, is then rendered to the screen. It handles a wide range of escape sequences to control cursor movement, text attributes,
*       colors, screen clearing, scrolling, and various terminal modes.
*
*       LIMITATIONS:
*         - Unicode: UTF-8 decoding is fully supported. Rendering uses a dynamic glyph cache for the Basic Multilingual Plane (BMP).
*         - BiDi: Bidirectional text (e.g., Arabic, Hebrew) is currently unimplemented.
*
**********************************************************************************************/
#ifndef KTERM_H
#define KTERM_H

#include "kterm_render_sit.h"

#ifdef KTERM_IMPLEMENTATION
  #if !defined(SITUATION_IMPLEMENTATION) && !defined(STB_TRUETYPE_IMPLEMENTATION)
    #define STB_TRUETYPE_IMPLEMENTATION
  #endif
#endif
#include "stb_truetype.h"


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <stdatomic.h>

// =============================================================================
// TERMINAL CONFIGURATION CONSTANTS
// =============================================================================
#define REGIS_WIDTH 800
#define REGIS_HEIGHT 480
#define DEFAULT_TERM_WIDTH 132
#define DEFAULT_TERM_HEIGHT 50
#define DEFAULT_CHAR_WIDTH 8
#define DEFAULT_CHAR_HEIGHT 10
#define DEFAULT_WINDOW_SCALE 1 // Scale factor for the window and font rendering
#define DEFAULT_WINDOW_WIDTH (DEFAULT_TERM_WIDTH * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE)
#define MAX_SESSIONS 4
#define DEFAULT_WINDOW_HEIGHT (DEFAULT_TERM_HEIGHT * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE)
#define MAX_ESCAPE_PARAMS 32
#define MAX_COMMAND_BUFFER 512 // General purpose buffer for commands, OSC, DCS etc.
#define MAX_TAB_STOPS 256 // Max columns for tab stops, ensure it's >= DEFAULT_TERM_WIDTH
#define MAX_TITLE_LENGTH 256
#define MAX_RECT_OPERATIONS 16
#define KEY_EVENT_BUFFER_SIZE 65536
#define OUTPUT_BUFFER_SIZE 16384
#define KTERM_INPUT_PIPELINE_SIZE (1024 * 1024) // 1MB buffer for high-throughput graphics
#define MAX_SCROLLBACK_LINES 1000

// =============================================================================
// GLOBAL VARIABLES DECLARATIONS
// =============================================================================
// Callbacks for application to handle terminal events
// Forward declaration
typedef struct KTerm_T KTerm;

// Response callback typedef
typedef void (*ResponseCallback)(KTerm* term, const char* response, int length); // For sending data back to host
typedef void (*PrinterCallback)(KTerm* term, const char* data, size_t length);   // For Printer Controller Mode
typedef void (*TitleCallback)(KTerm* term, const char* title, bool is_icon);    // For GUI window title changes
typedef void (*BellCallback)(KTerm* term);                                 // For audible bell
typedef void (*NotificationCallback)(KTerm* term, const char* message);          // For sending notifications (OSC 9)
typedef void (*GatewayCallback)(KTerm* term, const char* class_id, const char* id, const char* command, const char* params); // Gateway Protocol
typedef void (*SessionResizeCallback)(KTerm* term, int session_index, int cols, int rows); // Notification of session resize

// =============================================================================
// ENHANCED COLOR SYSTEM
// =============================================================================
typedef enum {
    COLOR_BLACK = 0, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE,
    COLOR_BRIGHT_BLACK, COLOR_BRIGHT_RED, COLOR_BRIGHT_GREEN, COLOR_BRIGHT_YELLOW,
    COLOR_BRIGHT_BLUE, COLOR_BRIGHT_MAGENTA, COLOR_BRIGHT_CYAN, COLOR_BRIGHT_WHITE
} AnsiKTermColor; // Standard 16 ANSI colors

typedef struct RGB_KTermColor_T {
    unsigned char r, g, b, a;
} RGB_KTermColor; // True color representation

#ifndef KTERM_IMPLEMENTATION
// External declarations for users of the library (if not header-only)
//extern VTKeyboard vt_keyboard;
// extern Texture2D font_texture; // Moved to struct
// extern RGB_KTermColor color_palette[256]; // Moved to struct
extern KTermColor ansi_colors[16];        // KTerm Color type for the 16 base ANSI colors
// extern unsigned char font_data[256 * 32]; // Defined in implementation
#endif

// =============================================================================
// VT COMPLIANCE LEVELS
// =============================================================================
typedef enum {
    VT_LEVEL_52 = 52,
    VT_LEVEL_100 = 100,
    VT_LEVEL_102 = 102,
    VT_LEVEL_132 = 132,
    VT_LEVEL_220 = 220,
    VT_LEVEL_320 = 320,
    VT_LEVEL_340 = 340,
    VT_LEVEL_420 = 420,
    VT_LEVEL_510 = 510,
    VT_LEVEL_520 = 520,
    VT_LEVEL_525 = 525,
    VT_LEVEL_K95 = 95,
    VT_LEVEL_XTERM = 1000,
    VT_LEVEL_TT = 1001,
    VT_LEVEL_PUTTY = 1002,
    VT_LEVEL_ANSI_SYS = 1003,
    VT_LEVEL_COUNT = 15 // Update this if more levels are added
} VTLevel;

// =============================================================================
// PARSE STATES
// =============================================================================
typedef enum {
    VT_PARSE_NORMAL,
    VT_PARSE_ESCAPE,
    PARSE_CSI,          // Control Sequence Introducer (ESC [)
    PARSE_OSC,          // Operating System Command (ESC ])
    PARSE_DCS,          // Device Control String (ESC P)
    PARSE_APC,          // Application Program Command (ESC _)
    PARSE_PM,           // Privacy Message (ESC ^)
    PARSE_SOS,          // Start of String (ESC X)
    PARSE_STRING_TERMINATOR, // Expecting ST (ESC \) to terminate a string
    PARSE_CHARSET,      // Selecting character set (ESC ( C, ESC ) C etc.)
    PARSE_HASH,         // DEC Line Attributes (ESC #)
    PARSE_PERCENT,      // Select Character Set (ESC %)
    PARSE_VT52,         // In VT52 compatibility mode
    PARSE_SIXEL,        // Parsing Sixel graphics data (ESC P q ... ST)
    PARSE_SIXEL_ST,
    PARSE_TEKTRONIX,    // Tektronix 4010/4014 vector graphics mode
    PARSE_REGIS,        // ReGIS graphics mode (ESC P p ... ST)
    PARSE_KITTY         // Kitty Graphics Protocol (ESC _ G ... ST)
} VTParseState;

// Extended color support
typedef struct {
    int color_mode;          // 0=indexed (palette), 1=RGB
    union {
        int index;           // 0-255 palette index
        RGB_KTermColor rgb;       // True color
    } value;
} ExtendedKTermColor;

// =============================================================================
// VT TERMINAL MODES AND STATES
// =============================================================================

// DEC Private Modes
typedef struct {
    bool application_cursor_keys;    // DECCKM (set with CSI ? 1 h/l)
    bool origin_mode;               // DECOM (set with CSI ? 6 h/l) - cursor relative to scroll region
    bool auto_wrap_mode;            // DECAWM (set with CSI ? 7 h/l)
    bool cursor_visible;            // DECTCEM (set with CSI ? 25 h/l)
    bool alternate_screen;          // DECSET 47/1047/1049 (uses alt_screen buffer)
    bool insert_mode;               // DECSET 4 (IRM in ANSI modes, also CSI ? 4 h/l)
    bool local_echo;                // Not a specific DEC mode, usually application controlled
    bool new_line_mode;             // DECSET 20 (LNM in ANSI modes) - LF implies CR
    bool column_mode_132;           // DECSET 3 (DECCOLM) - switches to 132 columns
    bool smooth_scroll;             // DECSET 4 (DECSCLM) - smooth vs jump scroll
    bool reverse_video;             // DECSET 5 (DECSCNM) - reverses fg/bg for whole screen
    bool relative_origin;           // DECOM (same as origin_mode)
    bool auto_repeat_keys;          // DECSET 8 (DECARM) - (usually OS controlled)
    bool x10_mouse;                 // DECSET 9 - X10 mouse reporting
    bool show_toolbar;              // DECSET 10 - (relevant for GUI terminals)
    bool blink_cursor;              // DECSET 12 - (cursor style, often linked with DECSCUSR)
    bool print_form_feed;           // DECSET 18 - (printer control)
    bool print_extent;              // DECSET 19 - (printer control)
    bool bidi_mode;                 // BDSM (CSI ? 8246 h/l) - Bi-Directional Support Mode
    bool declrmm_enabled;           // DECSET 69 - Left Right Margin Mode
    bool no_clear_on_column_change; // DECSET 95 - DECNCSM (No Clear Screen on Column Change)
    bool vt52_mode;                 // DECANM (Mode 2) - true=VT52, false=ANSI
    bool backarrow_key_mode;        // DECBKM (Mode 67) - true=BS, false=DEL
    bool sixel_scrolling_mode;      // DECSDM (Mode 80) - true=Scrolling Enabled, false=Discard
    bool edit_mode;                 // DECEDM (Mode 45) - Extended Edit Mode
    bool sixel_cursor_mode;         // Mode 8452 - Sixel Cursor Position
    bool checksum_reporting;        // DECECR (CSI z) - Enable Checksum Reporting
    bool allow_80_132_mode;         // Mode 40 - Allow 80/132 Column Switching
    bool alt_cursor_save;           // Mode 1041 - Save/Restore Cursor on Alt Screen Switch
} DECModes;

// ANSI Modes
typedef struct {
    // bool keyboard_action;           // KAM - Keyboard Action Mode (typically locked)
    bool insert_replace;            // IRM - Insert/Replace Mode (CSI 4 h/l)
    // bool send_receive;              // SRM - Send/Receive Mode (echo)
    bool line_feed_new_line;        // LNM - Line Feed/New Line Mode (CSI 20 h/l)
} ANSIModes;

// Mouse tracking mode enumeration
typedef enum {
    MOUSE_TRACKING_OFF, // No mouse tracking
    MOUSE_TRACKING_X10, // X10 mouse tracking
    MOUSE_TRACKING_VT200, // VT200 mouse tracking
    MOUSE_TRACKING_VT200_HIGHLIGHT, // VT200 highlight tracking
    MOUSE_TRACKING_BTN_EVENT, // Button-event tracking
    MOUSE_TRACKING_ANY_EVENT, // Any-event tracking
    MOUSE_TRACKING_SGR, // SGR mouse reporting
    MOUSE_TRACKING_URXVT, // URXVT mouse reporting
    MOUSE_TRACKING_PIXEL // Pixel position mouse reporting
} MouseTrackingMode;

// =============================================================================
// CURSOR SHAPES AND STYLES
// =============================================================================
typedef enum {
    CURSOR_BLOCK = 0,
    CURSOR_BLOCK_BLINK = 1,
    CURSOR_UNDERLINE = 2,
    CURSOR_UNDERLINE_BLINK = 3,
    CURSOR_BAR = 4, // Typically a vertical bar
    CURSOR_BAR_BLINK = 5
} CursorShape;

typedef struct {
    int x, y;
    bool visible;
    bool blink_enabled;     // Overall blink setting for the shape
    bool blink_state;       // Current on/off state of blink
    double blink_timer;     // Timer for blink interval
    CursorShape shape;
    ExtendedKTermColor color;    // Cursor color (often inverse of cell or specific color)
} EnhancedCursor;

// =============================================================================
// TAB STOP MANAGEMENT
// =============================================================================
typedef struct {
    bool* stops;               // Dynamic array indicating tab stop presence at each column
    int capacity;              // Allocated capacity of the stops array
    int count;                 // Number of active tab stops
    int default_width;         // Default tab width (usually 8)
} TabStops;

// =============================================================================
// CHARACTER SET HANDLING
// =============================================================================
typedef enum {
    CHARSET_ASCII = 0,      // Standard US ASCII
    CHARSET_DEC_SPECIAL,    // DEC Special Graphics (line drawing)
    CHARSET_UK,             // UK National character set
    CHARSET_DEC_MULTINATIONAL, // DEC Multinational Character Set (MCS)
    CHARSET_ISO_LATIN_1,    // ISO 8859-1 Latin-1
    CHARSET_UTF8,           // UTF-8 (requires multi-byte processing)
    // NRCS (National Replacement Character Sets)
    CHARSET_DUTCH,
    CHARSET_FINNISH,
    CHARSET_FRENCH,
    CHARSET_FRENCH_CANADIAN,
    CHARSET_GERMAN,
    CHARSET_ITALIAN,
    CHARSET_NORWEGIAN_DANISH,
    CHARSET_SPANISH,
    CHARSET_SWEDISH,
    CHARSET_SWISS,
    CHARSET_COUNT // Must be < 32
} CharacterSet;

typedef struct {
    CharacterSet g0, g1, g2, g3; // The four designated character sets
    CharacterSet *gl, *gr;      // Pointers to active left/right sets (for 7-bit/8-bit env)
    bool single_shift_2;         // Next char from G2 (SS2)
    bool single_shift_3;         // Next char from G3 (SS3)
    // bool locking_shift;       // LS0/LS1/LS2/LS3 state (not typically used in modern terminals like GL/GR)
} CharsetState;

// =============================================================================
// ATTRIBUTE BIT FLAGS
// =============================================================================
// Shared GPU Attributes (0-15) - Must match shaders/terminal.comp
#define KTERM_ATTR_BOLD               (1 << 0)
#define KTERM_ATTR_FAINT              (1 << 1)
#define KTERM_ATTR_ITALIC             (1 << 2)
#define KTERM_ATTR_UNDERLINE          (1 << 3)
#define KTERM_ATTR_BLINK              (1 << 4)
#define KTERM_ATTR_REVERSE            (1 << 5)
#define KTERM_ATTR_STRIKE             (1 << 6)
#define KTERM_ATTR_DOUBLE_WIDTH       (1 << 7)
#define KTERM_ATTR_DOUBLE_HEIGHT_TOP  (1 << 8)
#define KTERM_ATTR_DOUBLE_HEIGHT_BOT  (1 << 9)
#define KTERM_ATTR_CONCEAL            (1 << 10)
#define KTERM_ATTR_OVERLINE           (1 << 11) // xterm extension
#define KTERM_ATTR_DOUBLE_UNDERLINE   (1 << 12) // ECMA-48
#define KTERM_ATTR_BLINK_BG           (1 << 13) // Background Blink
#define KTERM_ATTR_BLINK_SLOW         (1 << 14) // Slow Blink (Independent Speed)

// Logical / Internal Attributes (16-31)
#define KTERM_ATTR_PROTECTED          (1 << 16) // DECSCA
#define KTERM_ATTR_SOFT_HYPHEN        (1 << 17)
#define KTERM_ATTR_GRID               (1 << 18) // Debug Grid
#define KTERM_ATTR_UL_STYLE_MASK      (7 << 20) // Bits 20-22 for Underline Style
#define KTERM_ATTR_UL_STYLE_NONE      (0 << 20)
#define KTERM_ATTR_UL_STYLE_SINGLE    (1 << 20)
#define KTERM_ATTR_UL_STYLE_DOUBLE    (2 << 20)
#define KTERM_ATTR_UL_STYLE_CURLY     (3 << 20)
#define KTERM_ATTR_UL_STYLE_DOTTED    (4 << 20)
#define KTERM_ATTR_UL_STYLE_DASHED    (5 << 20)

#define KTERM_FLAG_DIRTY              (1 << 30) // Cell needs redraw
#define KTERM_FLAG_COMBINING          (1 << 31) // Unicode combining char

// =============================================================================
// ENHANCED TERMINAL CHARACTER
// =============================================================================
typedef struct {
    unsigned int ch;             // Unicode codepoint (or ASCII/charset specific value)
    ExtendedKTermColor fg_color;
    ExtendedKTermColor bg_color;
    ExtendedKTermColor ul_color;
    ExtendedKTermColor st_color;
    uint32_t flags;              // Consolidated attributes
} EnhancedTermChar;

// =============================================================================
// BRACKETED PASTE MODE
// =============================================================================
typedef struct {
    bool enabled;       // Is CSI ? 2004 h active?
    bool active;        // Is a paste sequence currently being received? (between 200~ and 201~)
    char *buffer;       // Buffer for paste data (if needed, usually directly piped)
    size_t buffer_size;
    size_t buffer_pos;
    // double start_time; // For timeout logic if buffering
    // double timeout;
} BracketedPaste;

// =============================================================================
// PROGRAMMABLE KEYS
// =============================================================================
typedef struct {
    int key_code;           // Situation key code that triggers this
    char *sequence;         // String to send to host
    size_t sequence_length;
    bool active;            // Is this definition active
} ProgrammableKey;

typedef struct {
    ProgrammableKey *keys;
    bool udk_locked;            // Tracks UDK lock status for CSI ?25 n
    size_t count;
    size_t capacity;
} ProgrammableKeys;

// =============================================================================
// RECTANGULAR OPERATIONS
// =============================================================================
typedef struct {
    int top, left, bottom, right; // 0-indexed inclusive coordinates
    bool active;                  // Is a rectangular operation defined/active
} VTRectangle;

typedef enum {
    RECT_OP_COPY,    // DECCRA
    RECT_OP_MOVE,    // (Not standard VT, but common concept)
    RECT_OP_FILL,    // DECFRA
    RECT_OP_ERASE,   // DECERA
    RECT_OP_SELECT   // (For selection, not a VT command)
} RectOperation;

typedef struct {
    VTRectangle area;
    RectOperation operation;
    EnhancedTermChar fill_char; // Character used for fill/erase
    EnhancedTermChar *data;     // Buffer for copy/move operations
    size_t data_size;
} RectangularOperation; // This might be for managing ongoing ops, VT ops are usually immediate

// =============================================================================
// SIXEL GRAPHICS SUPPORT
// =============================================================================

// Forward declaration if needed, but struct definition must be visible
// GPUSixelStrip is defined later in the file (around line 560).
// We must move GPUSixelStrip definition BEFORE SixelGraphics usage or forward declare it.
// Since it's a value type in pointer, forward decl 'struct GPUSixelStrip' works but here we used typedef name directly.
// Let's redefine GPUSixelStrip here or move it up.
// Moving it up is cleaner.

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t pattern;
    uint32_t color_index;
} GPUSixelStrip;

typedef struct {
    unsigned char* data;
    int width;
    int height;
    int x, y;
    bool active;
    int pos_x, pos_y;
    int max_x, max_y;
    int color_index;
    int repeat_count;
    int params[MAX_ESCAPE_PARAMS];
    int param_count;
    bool dirty;
    RGB_KTermColor palette[256];
    int parse_state; // 0=Normal, 1=Repeat, 2=Color, 3=Raster
    int param_buffer[8]; // For color definitions #Pc;Pu;Px;Py;Pz etc.
    int param_buffer_idx;
    GPUSixelStrip* strips;
    size_t strip_count;
    size_t strip_capacity;
    bool scrolling; // Controls if image scrolls with text
    bool transparent_bg; // From DECGRA (P2)
    int logical_start_row; // Row index where the image starts (relative to screen_head)
    int last_y_shift; // Track last shift to optimize redraws
} SixelGraphics;

#define SIXEL_STATE_NORMAL 0
#define SIXEL_STATE_REPEAT 1
#define SIXEL_STATE_COLOR  2
#define SIXEL_STATE_RASTER 3

// =============================================================================
// KITTY GRAPHICS PROTOCOL
// =============================================================================

#define KTERM_KITTY_MEMORY_LIMIT (64 * 1024 * 1024) // 64MB Limit per session

typedef struct {
    unsigned char* data;
    size_t size;
    size_t capacity;
    int width;
    int height;
    KTermTexture texture;
    int delay_ms;
} KittyFrame;

typedef struct {
    uint32_t id;

    // Frames
    KittyFrame* frames;
    int frame_count;
    int frame_capacity;

    // Animation State
    int current_frame;
    double frame_timer;

    // Placement
    int x; // Screen coordinates (relative to session)
    int y;
    int z_index;
    int start_row; // Logical row index (screen_head) when image was placed
    bool visible;
    bool complete; // Is the image upload complete?
} KittyImageBuffer;

typedef struct {
    // Parsing state
    int state; // 0=KEY, 1=VALUE, 2=PAYLOAD
    char key_buffer[32];
    int key_len;
    char val_buffer[128];
    int val_len;
    bool continuing; // Flag for chunked transmission (m=1)

    // Current Command Parameters
    struct {
        char action; // 'a' value: 't', 'q', 'p', 'd'
        char delete_action; // 'd' value: 'a', 'i', 'p', etc.
        char format; // 'f' value: 32, 24, 100(PNG)
        uint32_t id; // 'i'
        uint32_t placement_id; // 'p'
        int width; // 's'
        int height; // 'v'
        int x; // 'x'
        int y; // 'y'
        int z_index; // 'z'
        int transmission_type; // 't'
        int medium; // 'm' (0 or 1)
        bool quiet; // 'q'
        bool has_x; // 'x' key present
        bool has_y; // 'y' key present
    } cmd;

    // Base64 State
    uint32_t b64_accumulator;
    int b64_bits;

    // Active upload buffer
    KittyImageBuffer* active_upload;

    // Storage for images (Simple array for Phase 3.1)
    // We will use a dynamic list later, or just a few slots for testing
    KittyImageBuffer* images; // Array of stored images
    int image_count;
    int image_capacity;
    size_t current_memory_usage; // Bytes used by image data

} KittyGraphics;

// =============================================================================
// SOFT FONTS
// =============================================================================
typedef struct {
    uint8_t width;      // Advance width or bitmap width
    uint8_t begin_x;    // First pixel column with data
    uint8_t end_x;      // Last pixel column with data
} KTermFontMetric;

typedef struct {
    unsigned char font_data[256][32]; // Storage for 256 characters, 32 bytes each (e.g., 16x16 monochrome)
    int char_width;                   // Width of characters in this font
    int char_height;                  // Height of characters in this font
    bool loaded[256];                 // Which characters in this set are loaded
    bool active;                      // Is a soft font currently selected?
    bool dirty;                       // Font data has changed and needs upload
    KTermFontMetric metrics[256];     // Per-character metrics
} SoftFont;

// =============================================================================
// VT CONFORMANCE AND FEATURE MANAGEMENT
// =============================================================================
typedef struct {
    bool vt52_mode;
    bool vt100_mode;
    bool vt102_mode;
    bool vt132_mode;
    bool vt220_mode;
    bool vt320_mode;
    bool vt340_mode;
    bool vt420_mode;
    bool vt510_mode;
    bool vt520_mode;
    bool vt525_mode;
    bool k95_mode;
    bool xterm_mode;
    bool tt_mode;
    bool putty_mode;
    bool sixel_graphics;          // Sixel graphics (DECGRA)
    bool regis_graphics;          // ReGIS graphics
    bool rectangular_operations;  // DECCRA, DECFRA, etc.
    bool selective_erase;         // DECSERA
    bool user_defined_keys;       // DECUDK
    bool soft_fonts;              // DECDLD
    bool national_charsets;       // NRCS
    bool mouse_tracking;          // DECSET 9, 1000, 1002, 1003
    bool alternate_screen;        // DECSET 1049
    bool true_color;              // SGR true color support
    bool window_manipulation;     // xterm window manipulation
    bool locator;                 // ANSI Text Locator
    bool multi_session_mode;      // Multi-session support (CSI ? 64 h/l)
    bool left_right_margin;       // DECSLRM (CSI ? 69 h/l)
    int max_session_count;        // Maximum number of sessions supported
} VTFeatures;
typedef struct {
    VTLevel level;        // Current conformance level (e.g., VT220)
    bool strict_mode;     // Enforce strict conformance? (vs. permissive)

    VTFeatures features;  // Feature flags derived from the level

    // Compliance tracking for diagnostics
    struct {
        int unsupported_sequences;
        int partial_implementations;
        int extensions_used;
        char last_unsupported[64]; // Last unsupported sequence string
    } compliance;
} VTConformance;

// =============================================================================
// ENHANCED KEYBOARD WITH FULL VT SUPPORT
// =============================================================================
typedef enum {
    KEY_PRIORITY_LOW = 0,
    KEY_PRIORITY_NORMAL = 1,
    KEY_PRIORITY_HIGH = 2,
    KEY_PRIORITY_CRITICAL = 3
} KeyPriority; // For prioritizing events in the buffer (e.g., Ctrl+C)

typedef struct {
    int key_code;           // Generic key code (backend specific)
    bool ctrl, shift, alt, meta;
    bool is_repeat;
    KeyPriority priority;
    double timestamp;
    char sequence[32];      // Generated escape sequence
} KTermEvent;

typedef struct {
    bool keypad_application_mode; // DECKPAM/DECKPNM
    bool meta_sends_escape;
    bool backarrow_sends_bs;
    bool delete_sends_del;
    int keyboard_dialect;
    char function_keys[24][32];

    // Event Buffer
    KTermEvent buffer[KEY_EVENT_BUFFER_SIZE];
    int buffer_head;
    int buffer_tail;
    int buffer_count;
    int total_events;
    int dropped_events;
} KTermInputConfig;

/*
typedef struct {
    bool application_mode;      // General application mode for some keys (not DECCKM)
    bool cursor_key_mode;       // DECCKM: Application Cursor Keys (ESC OA vs ESC [ A)
    bool keypad_mode;           // DECKPAM/DECKPNM: Application/Numeric Keypad
    bool meta_sends_escape;   // Does Alt/Meta key prefix char with ESC?
    bool delete_sends_del;    // DEL key sends DEL (0x7F) or BS (0x08)
    bool backarrow_sends_bs;  // Backarrow key sends BS (0x08) or DEL (0x7F)

    int keyboard_dialect;        // Tracks NRCS dialect for CSI ?26 n (1=North American, 2=British, etc.)

    // Function key definitions (programmable or standard)
    char function_keys[24][32];  // F1-F24 sequences (can be overridden by DECUDK)

    // Key mapping table (example, might not be fully used if KTerm_GenerateVTSequence is comprehensive)
    // struct {
    //     int Situation_key;
    //     char normal[16];
    //     char shift[16];
    //     char ctrl[16];
    //     char alt[16];
    //     char app[16]; // For application modes
    // } key_mappings[256]; // Max Situation key codes

    // Buffered input for key events
    VTKeyEvent buffer[512]; // Circular buffer for key events
    int buffer_head, buffer_tail, buffer_count;

    // Statistics
    size_t total_events;
    size_t dropped_events;      // If buffer overflows
    // size_t priority_overrides; // If high priority event preempts
} VTKeyboard;
*/

// =============================================================================
// TITLE AND ICON MANAGEMENT
// =============================================================================
typedef struct {
    char window_title[MAX_TITLE_LENGTH];
    char icon_title[MAX_TITLE_LENGTH];
    char terminal_name[64]; // Name reported by some terminal identification sequences
    bool title_changed;     // Flag for GUI update
    bool icon_changed;      // Flag for GUI update
} TitleManager;

// =============================================================================
// TERMINAL STATUS
// =============================================================================
typedef struct {
    size_t pipeline_usage;      // Bytes currently in input_pipeline
    size_t key_usage;           // Events currently in vt_keyboard.buffer
    bool overflow_detected;     // Was input_pipeline overflowed recently?
    double avg_process_time;    // Average time to process one char from pipeline (diagnostics)
} KTermStatus;

// =============================================================================
// TERMINAL COMPUTE SHADER & GPU STRUCTURES
// =============================================================================

typedef struct {
    uint32_t char_code;
    uint32_t fg_color;
    uint32_t bg_color;
    uint32_t flags;
    uint32_t ul_color;
    uint32_t st_color;
} GPUCell;

typedef struct {
    float x0, y0; // Normalized Device Coordinates (0.0 - 1.0)
    float x1, y1; // Normalized Device Coordinates
    uint32_t color; // Packed RGBA
    float intensity; // 1.0 = fresh beam, < 1.0 = decaying
    uint32_t mode;   // 0=Additive, 1=Replace, 2=Erase, 3=XOR
    float padding;   // Align to 16 bytes for std430
} GPUVectorLine;

// --- Shader Code ---
#ifndef KTERM_TERMINAL_SHADER_PATH
#define KTERM_TERMINAL_SHADER_PATH "shaders/terminal.comp"
#endif
#ifndef KTERM_VECTOR_SHADER_PATH
#define KTERM_VECTOR_SHADER_PATH "shaders/vector.comp"
#endif
#ifndef KTERM_SIXEL_SHADER_PATH
#define KTERM_SIXEL_SHADER_PATH "shaders/sixel.comp"
#endif


#if defined(SITUATION_USE_VULKAN)
    // --- VULKAN DEFINITIONS ---
    static const char* terminal_compute_preamble =
    "#version 460\n"
    "#define VULKAN_BACKEND\n"
    "#extension GL_EXT_buffer_reference : require\n"
    "#extension GL_EXT_scalar_block_layout : require\n"
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n"
    "#extension GL_ARB_bindless_texture : require\n"
    "struct GPUCell { uint char_code; uint fg_color; uint bg_color; uint flags; uint ul_color; uint st_color; };\n"
    "layout(buffer_reference, scalar) buffer KTermBuffer { GPUCell cells[]; };\n"
    "layout(set = 1, binding = 0, rgba8) uniform image2D output_image;\n"
    "layout(push_constant) uniform PushConstants {\n"
    "    vec2 screen_size;\n"
    "    vec2 char_size;\n"
    "    vec2 grid_size;\n"
    "    float time;\n"
    "    uint cursor_index;\n"
    "    uint cursor_blink_state;\n"
    "    uint text_blink_state;\n"
    "    uint sel_start;\n"
    "    uint sel_end;\n"
    "    uint sel_active;\n"
    "    float scanline_intensity;\n"
    "    float crt_curvature;\n"
    "    uint mouse_cursor_index;\n"
    "    uint64_t terminal_buffer_addr;\n"
    "    uint64_t vector_buffer_addr;\n"
    "    uint64_t font_texture_handle;\n"
    "    uint64_t sixel_texture_handle;\n"
    "    uint64_t vector_texture_handle;\n"
    "    uint atlas_cols;\n"
    "    uint vector_count;\n"
    "    float visual_bell_intensity;\n"
    "    int sixel_y_offset;\n"
    "    uint grid_color;\n"
    "    uint conceal_char_code;\n"
    "} pc;\n";

    static const char* vector_compute_preamble =
    "#version 460\n"
    "#extension GL_EXT_buffer_reference : require\n"
    "#extension GL_EXT_scalar_block_layout : require\n"
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n"
    "layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n"
    "struct GPUVectorLine { vec2 start; vec2 end; uint color; float intensity; uint mode; float _pad; };\n"
    "layout(buffer_reference, scalar) buffer VectorBuffer { GPUVectorLine data[]; };\n"
    "layout(set = 1, binding = 0, rgba8) uniform image2D output_image;\n"
    "layout(push_constant) uniform PushConstants {\n"
    "    vec2 screen_size;\n"
    "    vec2 char_size;\n"
    "    vec2 grid_size;\n"
    "    float time;\n"
    "    uint cursor_index;\n"
    "    uint cursor_blink_state;\n"
    "    uint text_blink_state;\n"
    "    uint sel_start;\n"
    "    uint sel_end;\n"
    "    uint sel_active;\n"
    "    float scanline_intensity;\n"
    "    float crt_curvature;\n"
    "    uint mouse_cursor_index;\n"
    "    uint64_t terminal_buffer_addr;\n"
    "    uint64_t vector_buffer_addr;\n"
    "    uint64_t font_texture_handle;\n"
    "    uint64_t sixel_texture_handle;\n"
    "    uint64_t vector_texture_handle;\n"
    "    uint atlas_cols;\n"
    "    uint vector_count;\n"
    "    float visual_bell_intensity;\n"
    "    int sixel_y_offset;\n"
    "    uint grid_color;\n"
    "    uint conceal_char_code;\n"
    "} pc;\n";

    static const char* sixel_compute_preamble =
    "#version 460\n"
    "#define VULKAN_BACKEND\n"
    "#extension GL_EXT_buffer_reference : require\n"
    "#extension GL_EXT_scalar_block_layout : require\n"
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n"
    "#extension GL_ARB_bindless_texture : require\n"
    "layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n"
    "struct GPUSixelStrip { uint x; uint y; uint pattern; uint color_index; };\n"
    "layout(buffer_reference, scalar) buffer SixelBuffer { GPUSixelStrip data[]; };\n"
    "layout(buffer_reference, scalar) buffer PaletteBuffer { uint colors[]; };\n"
    "layout(set = 1, binding = 0, rgba8) uniform image2D output_image;\n"
    "layout(push_constant) uniform PushConstants {\n"
    "    vec2 screen_size;\n"
    "    vec2 char_size;\n"
    "    vec2 grid_size;\n"
    "    float time;\n"
    "    uint cursor_index;\n"
    "    uint cursor_blink_state;\n"
    "    uint text_blink_state;\n"
    "    uint sel_start;\n"
    "    uint sel_end;\n"
    "    uint sel_active;\n"
    "    float scanline_intensity;\n"
    "    float crt_curvature;\n"
    "    uint mouse_cursor_index;\n"
    "    uint64_t terminal_buffer_addr;\n"
    "    uint64_t vector_buffer_addr;\n"
    "    uint64_t font_texture_handle;\n"
    "    uint64_t sixel_texture_handle;\n"
    "    uint64_t vector_texture_handle;\n"
    "    uint atlas_cols;\n"
    "    uint vector_count;\n"
    "    float visual_bell_intensity;\n"
    "    int sixel_y_offset;\n"
    "    uint grid_color;\n"
    "    uint conceal_char_code;\n"
    "} pc;\n";

    static const char* blit_compute_preamble =
    "#version 460\n"
    "#define VULKAN_BACKEND\n"
    "#extension GL_EXT_scalar_block_layout : require\n"
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n"
    "#extension GL_ARB_bindless_texture : require\n"
    "layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;\n"
    "layout(set = 1, binding = 0, rgba8) uniform image2D dstImage;\n"
    "layout(push_constant) uniform PushConstants {\n"
    "    ivec2 dest_pos;\n"
    "    ivec2 src_size;\n"
    "    uint64_t src_texture_handle;\n"
    "    ivec4 clip_rect;\n"
    "} pc;\n";

#else
    // --- OPENGL / DEFAULT DEFINITIONS ---
    static const char* terminal_compute_preamble =
    "#version 460\n"
    "#extension GL_EXT_buffer_reference : require\n"
    "#extension GL_EXT_scalar_block_layout : require\n"
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n"
    "#extension GL_ARB_bindless_texture : require\n"
    "layout(local_size_x = 8, local_size_y = 16, local_size_z = 1) in;\n"
    "struct GPUCell { uint char_code; uint fg_color; uint bg_color; uint flags; uint ul_color; uint st_color; };\n"
    "layout(buffer_reference, scalar) buffer KTermBuffer { GPUCell cells[]; };\n"
    "layout(binding = 1, rgba8) uniform image2D output_image;\n"
    "layout(scalar, binding = 0) uniform PushConstants {\n"
    "    vec2 screen_size;\n"
    "    vec2 char_size;\n"
    "    vec2 grid_size;\n"
    "    float time;\n"
    "    uint cursor_index;\n"
    "    uint cursor_blink_state;\n"
    "    uint text_blink_state;\n"
    "    uint sel_start;\n"
    "    uint sel_end;\n"
    "    uint sel_active;\n"
    "    float scanline_intensity;\n"
    "    float crt_curvature;\n"
    "    uint mouse_cursor_index;\n"
    "    uint64_t terminal_buffer_addr;\n"
    "    uint64_t vector_buffer_addr;\n"
    "    uint64_t font_texture_handle;\n"
    "    uint64_t sixel_texture_handle;\n"
    "    uint64_t vector_texture_handle;\n"
    "    uint atlas_cols;\n"
    "    uint vector_count;\n"
    "    float visual_bell_intensity;\n"
    "    int sixel_y_offset;\n"
    "    uint grid_color;\n"
    "    uint conceal_char_code;\n"
    "} pc;\n";

    static const char* vector_compute_preamble =
    "#version 460\n"
    "#extension GL_EXT_buffer_reference : require\n"
    "#extension GL_EXT_scalar_block_layout : require\n"
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n"
    "layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n"
    "struct GPUVectorLine { vec2 start; vec2 end; uint color; float intensity; uint mode; float _pad; };\n"
    "layout(buffer_reference, scalar) buffer VectorBuffer { GPUVectorLine data[]; };\n"
    "layout(binding = 1, rgba8) uniform image2D output_image;\n"
    "layout(scalar, binding = 0) uniform PushConstants {\n"
    "    vec2 screen_size;\n"
    "    vec2 char_size;\n"
    "    vec2 grid_size;\n"
    "    float time;\n"
    "    uint cursor_index;\n"
    "    uint cursor_blink_state;\n"
    "    uint text_blink_state;\n"
    "    uint sel_start;\n"
    "    uint sel_end;\n"
    "    uint sel_active;\n"
    "    float scanline_intensity;\n"
    "    float crt_curvature;\n"
    "    uint mouse_cursor_index;\n"
    "    uint64_t terminal_buffer_addr;\n"
    "    uint64_t vector_buffer_addr;\n"
    "    uint64_t font_texture_handle;\n"
    "    uint64_t sixel_texture_handle;\n"
    "    uint64_t vector_texture_handle;\n"
    "    uint atlas_cols;\n"
    "    uint vector_count;\n"
    "    float visual_bell_intensity;\n"
    "    int sixel_y_offset;\n"
    "    uint grid_color;\n"
    "    uint conceal_char_code;\n"
    "} pc;\n";

    static const char* sixel_compute_preamble =
    "#version 460\n"
    "#extension GL_EXT_buffer_reference : require\n"
    "#extension GL_EXT_scalar_block_layout : require\n"
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n"
    "#extension GL_ARB_bindless_texture : require\n"
    "layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n"
    "struct GPUSixelStrip { uint x; uint y; uint pattern; uint color_index; };\n"
    "layout(buffer_reference, scalar) buffer SixelBuffer { GPUSixelStrip data[]; };\n"
    "layout(buffer_reference, scalar) buffer PaletteBuffer { uint colors[]; };\n"
    "layout(binding = 1, rgba8) uniform image2D output_image;\n"
    "layout(scalar, binding = 0) uniform PushConstants {\n"
    "    vec2 screen_size;\n"
    "    vec2 char_size;\n"
    "    vec2 grid_size;\n"
    "    float time;\n"
    "    uint cursor_index;\n"
    "    uint cursor_blink_state;\n"
    "    uint text_blink_state;\n"
    "    uint sel_start;\n"
    "    uint sel_end;\n"
    "    uint sel_active;\n"
    "    float scanline_intensity;\n"
    "    float crt_curvature;\n"
    "    uint mouse_cursor_index;\n"
    "    uint64_t terminal_buffer_addr;\n"
    "    uint64_t vector_buffer_addr;\n"
    "    uint64_t font_texture_handle;\n"
    "    uint64_t sixel_texture_handle;\n"
    "    uint64_t vector_texture_handle;\n"
    "    uint atlas_cols;\n"
    "    uint vector_count;\n"
    "    float visual_bell_intensity;\n"
    "    int sixel_y_offset;\n"
    "    uint grid_color;\n"
    "    uint conceal_char_code;\n"
    "} pc;\n";

    static const char* blit_compute_preamble =
    "#version 460\n"
    "#extension GL_EXT_scalar_block_layout : require\n"
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n"
    "#extension GL_ARB_bindless_texture : require\n"
    "layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;\n"
    "layout(binding = 1, rgba8) uniform image2D dstImage;\n"
    "layout(scalar, binding = 0) uniform PushConstants {\n"
    "    ivec2 dest_pos;\n"
    "    ivec2 src_size;\n"
    "    uint64_t src_texture_handle;\n"
    "    ivec4 clip_rect;\n"
    "} pc;\n";
#endif


typedef struct {
    KTermVector2 screen_size;
    KTermVector2 char_size;
    KTermVector2 grid_size;
    float time;
    union {
        uint32_t cursor_index;
        // For Vector Shader, we use a separate layout or the end of the struct
        // but aligning with the GLSL struct is key.
        // The GLSL struct 'PushConsts' in Vector shader is minimal.
        // The Vulkan 'PushConstants' is larger.
        // We will rely on explicit offsets or names.
    };
    uint32_t cursor_blink_state;
    uint32_t text_blink_state;
    uint32_t sel_start;
    uint32_t sel_end;
    uint32_t sel_active;
    float scanline_intensity;
    float crt_curvature;
    uint32_t mouse_cursor_index;
    uint64_t terminal_buffer_addr;
    uint64_t vector_buffer_addr;
    uint64_t font_texture_handle;
    uint64_t sixel_texture_handle;
    uint64_t vector_texture_handle;
    uint32_t atlas_cols;   // Added for Dynamic Atlas
    uint32_t vector_count; // Appended for Vector shader access
    float visual_bell_intensity; // Visual Bell Intensity (0.0 - 1.0)
    int sixel_y_offset; // For scrolling Sixel images
    uint32_t grid_color; // Debug Grid Color
    uint32_t conceal_char_code; // Conceal Character Code
} KTermPushConstants;

#define GPU_ATTR_BOLD       (1 << 0)
#define GPU_ATTR_FAINT      (1 << 1)
#define GPU_ATTR_ITALIC     (1 << 2)
#define GPU_ATTR_UNDERLINE  (1 << 3)
#define GPU_ATTR_BLINK      (1 << 4)
#define GPU_ATTR_REVERSE    (1 << 5)
#define GPU_ATTR_STRIKE     (1 << 6)
#define GPU_ATTR_DOUBLE_WIDTH       (1 << 7)
#define GPU_ATTR_DOUBLE_HEIGHT_TOP  (1 << 8)
#define GPU_ATTR_DOUBLE_HEIGHT_BOT  (1 << 9)
#define GPU_ATTR_CONCEAL            (1 << 10)

// =============================================================================
// MAIN ENHANCED TERMINAL STRUCTURE
// =============================================================================

// Saved Cursor State (DECSC/DECRC)
typedef struct {
    int x, y;
    bool origin_mode;
    bool auto_wrap_mode;

    // Attributes
    ExtendedKTermColor fg_color;
    ExtendedKTermColor bg_color;
    uint32_t attributes;

    // Charset
    CharsetState charset;
} SavedCursorState;

// SGR Stack State (XTPUSHSGR/XTPOPSGR)
typedef struct {
    ExtendedKTermColor fg_color;
    ExtendedKTermColor bg_color;
    ExtendedKTermColor ul_color;
    ExtendedKTermColor st_color;
    uint32_t attributes;
} SavedSGRState;

typedef struct {

    // Screen management
    EnhancedTermChar* screen_buffer;       // Primary screen ring buffer
    EnhancedTermChar* alt_buffer;          // Alternate screen buffer
    int buffer_height;                     // Total rows in ring buffer
    int screen_head;                       // Index of the top visible row in the buffer (Ring buffer head)
    int alt_screen_head;                   // Stored head for alternative screen
    int view_offset;                       // Scrollback offset (0 = bottom/active view)
    int saved_view_offset;                 // Stored scrollback offset for main screen

    // Legacy member placeholders to be removed after refactor,
    // but kept as comments for now to indicate change.
    // EnhancedTermChar screen[term->height][term->width];
    // EnhancedTermChar alt_screen[term->height][term->width];

    int cols; // KTerm width in columns
    int rows; // KTerm height in rows (viewport)

    bool* row_dirty; // Tracks dirty state of the VIEWPORT rows (0..rows-1)
    // EnhancedTermChar saved_screen[term->height][term->width]; // For DECSEL/DECSED if implemented

    // Enhanced cursor
    EnhancedCursor cursor;
    SavedCursorState saved_cursor; // For DECSC/DECRC and other save/restore ops
    bool saved_cursor_valid;

    // KTerm identification & conformance
    VTConformance conformance;
    char device_attributes[128];    // Primary DA string (e.g., CSI c)
    char secondary_attributes[128]; // Secondary DA string (e.g., CSI > c)

    // Mode management
    DECModes dec_modes;
    ANSIModes ansi_modes;

    // Current character attributes for new text
    ExtendedKTermColor current_fg;
    ExtendedKTermColor current_bg;
    ExtendedKTermColor current_ul_color;
    ExtendedKTermColor current_st_color;
    uint32_t current_attributes; // Mask of KTERM_ATTR_* applied to new chars
    uint32_t text_blink_state;  // Current blink states (Bit 0: Fast, Bit 1: Slow, Bit 2: Background)
    double text_blink_timer;    // Timer for text blink interval
    int fast_blink_rate;        // Oscillator Slot (Default 30, ~250ms)
    int slow_blink_rate;        // Oscillator Slot (Default 35, ~500ms)
    int bg_blink_rate;          // Oscillator Slot (Default 35, ~500ms)

    // Debug Grid
    bool grid_enabled;
    RGB_KTermColor grid_color;
    uint32_t conceal_char_code;

    // Scrolling and margins
    int scroll_top, scroll_bottom;  // Defines the scroll region (0-indexed)
    int left_margin, right_margin;  // Defines left/right margins (0-indexed, VT420+)

    // Character handling
    CharsetState charset;
    TabStops tab_stops;

    // Enhanced features
    BracketedPaste bracketed_paste;
    ProgrammableKeys programmable_keys; // For DECUDK
    SixelGraphics sixel;
    KittyGraphics kitty;
    SoftFont soft_font;                 // For DECDLD
    TitleManager title;

    // Mouse support state
    struct {
        MouseTrackingMode mode; // Mouse tracking mode
        bool enabled; // Mouse input enabled
        bool buttons[3]; // Left, middle, right button states
        int last_x; // Last cell x position
        int last_y; // Last cell y position
        int last_pixel_x; // Last pixel x position
        int last_pixel_y; // Last pixel y position
        bool focused; // Tracks window focus state for CSI I/O
        bool focus_tracking; // Enables focus in/out reporting
        bool sgr_mode; // Enables SGR mouse reporting
        int cursor_x; // Current mouse cursor X cell position
        int cursor_y; // Current mouse cursor Y cell position
    } mouse;

    // Input/Output pipeline (enhanced)
    unsigned char input_pipeline[KTERM_INPUT_PIPELINE_SIZE]; // Buffer for incoming data from host (Increased to 1MB)
    int input_pipeline_length; // Input pipeline length0
    atomic_int pipeline_head;
    atomic_int pipeline_tail;
    int pipeline_count;
    atomic_bool pipeline_overflow;

    KTermInputConfig input;

    // Performance and processing control
    struct {
        int chars_per_frame;      // Max chars to process from pipeline per frame
        double target_frame_time; // Inverse of target FPS (e.g., 1/60.0)
        double time_budget;       // Max time per frame for pipeline processing
        double avg_process_time;  // Moving average of char processing time
        bool burst_mode;          // Is processing in burst mode (more chars/frame)
        int burst_threshold;      // Pipeline count to trigger burst mode
        // double burst_time_limit;  // Max time for a burst
        bool adaptive_processing; // Adjust chars_per_frame based on performance
    } VTperformance;

    // Response system (data to send back to host)
    char answerback_buffer[OUTPUT_BUFFER_SIZE]; // Buffer for responses to host
    int response_length; // Response buffer length
    bool response_enabled; // Master switch for output (response transmission)

    // ANSI parsing state (enhanced)
    VTParseState parse_state;
    VTParseState saved_parse_state; // Saved state for handling string terminators (ESC \)
    char escape_buffer[MAX_COMMAND_BUFFER]; // Buffer for parameters of ESC, CSI, OSC, DCS etc.
    int escape_pos;
    int escape_params[MAX_ESCAPE_PARAMS];   // Parsed numeric parameters for CSI
    char escape_separators[MAX_ESCAPE_PARAMS]; // Separator following each param (';' or ':')
    int param_count;

    // SGR Stack (XTPUSHSGR/XTPOPSGR)
    SavedSGRState sgr_stack[10];
    int sgr_stack_depth;

    // Status and diagnostics
    struct {
        // bool error_recovery; // Attempt to recover from invalid sequences
        // double last_error_time;
        // char last_error[256];
        int error_count;        // Generic error counter
        bool debugging;         // General debugging flag for verbose logs
    } status;

    // Feature toggles / options
    struct {
        bool conformance_checking; // Enable stricter checks and logging
        bool vttest_mode;          // Special behaviors for vttest suite
        bool debug_sequences;      // Log unknown/unsupported sequences
        bool log_unsupported;      // Alias or part of debug_sequences
    } options;

    // Fields for console.c
    bool session_open; // Session open status
    int active_display; // 0=Main Display, 1=Status Line (DECSASD)
    bool echo_enabled;
    bool input_enabled;
    bool password_mode;
    bool raw_mode;
    bool paused;

    bool printer_available;      // Tracks printer availability for CSI ?15 n
    bool auto_print_enabled; // Tracks CSI 4 i / CSI 5 i state
    bool printer_controller_enabled; // Tracks CSI ?4 i / CSI ?5 i state
    struct {
        bool report_button_down;
        bool report_button_up;
        bool report_on_request_only;
    } locator_events;
    bool locator_enabled;        // Tracks locator device status for CSI ?53 n
    struct {
        size_t used;             // Bytes used for macro storage
        size_t total;            // Total macro storage capacity
    } macro_space;               // For CSI ?62 n
    struct {
        int algorithm;           // Checksum algorithm (e.g., 1=CRC16)
        uint32_t last_checksum;  // Last computed checksum
    } checksum;                  // For CSI ?63 n
    char tertiary_attributes[128]; // For CSI = c

    double visual_bell_timer; // Timer for visual bell effect

    // Graphics resources for Compute Shader

    // UTF-8 decoding state
    struct {
        uint32_t codepoint;
        uint32_t min_codepoint;
        int bytes_remaining;
    } utf8;

    // Mouse selection
    struct {
        int start_x, start_y;
        int end_x, end_y;
        bool active;
        bool dragging;
    } selection;


    // Last printed character (for REP command)
    unsigned int last_char;

    // Auto-print state
    int last_cursor_y; // For tracking line completion

    // Printer Controller buffer for exit sequence detection
    unsigned char printer_buffer[8];
    int printer_buf_len;

    void* user_data; // User data for callbacks and application state

} KTermSession;

// =============================================================================
// MULTIPLEXER LAYOUT STRUCTURES
// =============================================================================

typedef enum {
    PANE_SPLIT_VERTICAL,   // Top/Bottom split
    PANE_SPLIT_HORIZONTAL, // Left/Right split
    PANE_LEAF              // Contains a session
} KTermPaneType;

typedef struct KTermPane_T KTermPane;

struct KTermPane_T {
    KTermPaneType type;
    KTermPane* parent;

    // For Splits
    KTermPane* child_a;
    KTermPane* child_b;
    float split_ratio; // 0.0 to 1.0 (relative size of child_a)

    // For Leaves
    int session_index; // -1 if empty

    // Geometry (Calculated)
    int x, y, width, height; // Cells
};

// Typedef for the command execution callback to accept session
typedef void (*ExecuteCommandCallback)(KTerm* term, KTermSession* session);

// Helper functions for Ring Buffer Access
static inline EnhancedTermChar* GetScreenRow(KTermSession* session, int row) {
    // Access logical row 'row' (0 to HEIGHT-1 or -scrollback) relative to the visible screen top.
    // We adjust by view_offset (which allows looking back).
    // view_offset = 0 means looking at active screen.
    // view_offset > 0 means scrolling up (looking at history).

    // Logical top row of the visible window (including scrollback offset)
    // screen_head points to the top line of the active screen (logical row 0).
    // If we view back, we look at screen_head - view_offset.

    int logical_row_idx = session->screen_head + row - session->view_offset;

    // Handle wrap-around
    int actual_index = logical_row_idx % session->buffer_height;
    if (actual_index < 0) actual_index += session->buffer_height;

    return &session->screen_buffer[actual_index * session->cols];
}

static inline EnhancedTermChar* GetScreenCell(KTermSession* session, int y, int x) {
    if (x < 0 || x >= session->cols) return NULL; // Basic safety
    return &GetScreenRow(session, y)[x];
}

static inline EnhancedTermChar* GetActiveScreenRow(KTermSession* session, int row) {
    // Access logical row 'row' relative to the ACTIVE screen top (ignoring view_offset).
    // This is used for emulation commands that modify the screen state (insert, delete, scroll).

    int logical_row_idx = session->screen_head + row; // No view_offset subtraction

    // Handle wrap-around
    int actual_index = logical_row_idx % session->buffer_height;
    if (actual_index < 0) actual_index += session->buffer_height;

    return &session->screen_buffer[actual_index * session->cols];
}

static inline EnhancedTermChar* GetActiveScreenCell(KTermSession* session, int y, int x) {
    if (x < 0 || x >= session->cols) return NULL;
    return &GetActiveScreenRow(session, y)[x];
}

typedef struct KTerm_T {
    KTermSession sessions[MAX_SESSIONS];
    KTermPane* layout_root;
    KTermPane* focused_pane;
    int width; // Global Width (Columns)
    int height; // Global Height (Rows)
    int active_session;
    int pending_session_switch; // For session switching during update loop
    bool split_screen_active; // Deprecated by layout_root, kept for legacy compatibility for now
    int split_row;
    int session_top;
    int session_bottom;
    ResponseCallback response_callback; // Response callback
    KTermPipeline compute_pipeline;
    KTermPipeline texture_blit_pipeline; // For Kitty Graphics
    KTermBuffer terminal_buffer; // SSBO
    KTermTexture output_texture; // Storage Image
    KTermTexture font_texture;   // Font Atlas
    KTermTexture sixel_texture;  // Sixel Graphics
    KTermTexture dummy_sixel_texture; // Fallback 1x1 transparent texture
    KTermTexture clear_texture;  // 1x1 Opaque texture for clearing
    GPUCell* gpu_staging_buffer;
    bool compute_initialized;

    // Vector Engine (Tektronix)
    KTermBuffer vector_buffer;
    KTermTexture vector_layer_texture;
    KTermPipeline vector_pipeline;
    uint32_t vector_count;
    GPUVectorLine* vector_staging_buffer;
    size_t vector_capacity;

    // Sixel Engine (Compute Shader)
    KTermBuffer sixel_buffer;
    KTermBuffer sixel_palette_buffer;
    KTermPipeline sixel_pipeline;

    // Tektronix Parser State
    struct {
        int state; // 0=Alpha, 1=Graph
        int sub_state; // 0=HiY, 1=LoY, 2=HiX, 3=LoX
        int x, y; // Current beam position (0-4095)
        int holding_x, holding_y; // Holding registers
        int extra_byte; // Buffered Extra Byte for 12-bit coordinates
        bool pen_down; // True=Draw, False=Move
    } tektronix;

    // ReGIS Parser State
    struct {
        int state; // 0=Command, 1=Values, 2=Options, 3=Text String
        int x, y; // Current beam position (0-(REGIS_WIDTH - 1), 0-(REGIS_HEIGHT - 1))
        int screen_min_x, screen_min_y; // Logical screen extents
        int screen_max_x, screen_max_y;
        int save_x, save_y; // Saved position (stack depth 1)
        uint32_t color; // Current RGBA color
        int write_mode; // 0=Overlay(V), 1=Replace(R), 2=Erase(E), 3=Complement(C)
        char command; // Current command letter (P, V, C, T, W, etc.)
        int params[16]; // Numeric parameters for current command
        bool params_relative[16]; // True if parameter was explicitly signed (+/-)
        int param_count; // Number of parameters parsed so far
        bool has_comma; // Was a comma seen? (Parameter separator)
        bool has_bracket; // Was an opening bracket seen? (Option/Vector list)
        bool has_paren; // Was an opening parenthesis seen? (Options)
        char option_command; // Current option command being parsed
        bool data_pending; // Has new data arrived since last execution?

        // Robust Number Parsing
        int current_val;
        int current_sign;
        bool parsing_val;
        bool val_is_relative;

        // Text Command
        char text_buffer[256];
        int text_pos;
        char string_terminator;

        // Curve & Polygon support
        struct { int x, y; } point_buffer[64];
        int point_count;
        char curve_mode; // 'C'ircle, 'A'rc, 'B'spline (Interpolated), 'O'pen (Unclosed)

        // Extended Text Attributes
        float text_size;
        float text_angle; // Radians

        // Macrographs
        char* macros[26]; // Storage for @A through @Z
        bool recording_macro;
        int macro_index;
        char* macro_buffer;
        size_t macro_len;
        size_t macro_cap;
        int recursion_depth;

        // Load Alphabet (L) Support
        struct {
            char name[16]; // Name of the alphabet (e.g., "A")
            int current_char; // The character currently being defined (0-255)
            int pattern_byte_idx; // Current byte index in the character's pattern (0-31)
            int hex_nibble; // Parsing state for hex pairs (-1 = expecting high nibble)
        } load;
    } regis;

    // Retro Visual Effects
    struct {
        float curvature;
        float scanline_intensity;
    } visual_effects;

    bool vector_clear_request; // Request to clear the persistent vector layer

    // Dynamic Glyph Cache
    uint16_t* glyph_map; // Map Unicode Codepoint to Atlas Index
    uint32_t next_atlas_index;
    uint32_t atlas_clock_hand; // For Clock eviction algorithm
    unsigned char* font_atlas_pixels; // persistent CPU copy
    bool font_atlas_dirty;
    uint32_t atlas_width;
    uint32_t atlas_height;
    uint32_t atlas_cols;

    NotificationCallback notification_callback; // Notification callback (OSC 9)

    // TrueType Font Engine
    struct {
        bool loaded;
        unsigned char* file_buffer;
        stbtt_fontinfo info;
        float scale;
        int ascent;
        int descent;
        int line_gap;
        int baseline;
    } ttf;

    // LRU Cache for Dynamic Atlas
    uint64_t* glyph_last_used;   // Timestamp (frame count) of last usage
    uint32_t* atlas_to_codepoint;// Reverse mapping for eviction
    uint64_t frame_count;        // Logical clock for LRU

    // Font State
    int char_width;  // Cell Width (Screen)
    int char_height; // Cell Height (Screen)
    int font_data_width;  // Glyph Bitmap Width
    int font_data_height; // Glyph Bitmap Height
    const void* current_font_data;
    bool current_font_is_16bit; // True for >8px wide fonts (e.g. VCR)
    KTermFontMetric font_metrics[256]; // Current font metrics

    PrinterCallback printer_callback; // Callback for Printer Controller Mode
    GatewayCallback gateway_callback; // Callback for Gateway Protocol
    TitleCallback title_callback;
    BellCallback bell_callback;
    SessionResizeCallback session_resize_callback;

    RGB_KTermColor color_palette[256];
    uint32_t charset_lut[32][128];
    EnhancedTermChar* row_scratch_buffer; // Scratch buffer for row rendering

    // Multiplexer Input Interceptor
    struct {
        bool active;
        int prefix_key_code; // Default 'B' (for Ctrl+B)
    } mux_input;
} KTerm;

// =============================================================================
// CORE API FUNCTIONS
// =============================================================================

// THREAD SAFETY NOTE:
// This library is NOT thread-safe. All KTerm_* functions (especially KTerm_WriteChar,
// KTerm_Update, and KTerm_Draw) must be called from the same thread or
// externally synchronized by the application.
// term->active_session and other global states are shared across functions.

typedef struct {
    int width;
    int height;
    ResponseCallback response_callback;
} KTermConfig;

KTerm* KTerm_Create(KTermConfig config);
void KTerm_Destroy(KTerm* term);

// Session Management
void KTerm_SetActiveSession(KTerm* term, int index);
void KTerm_SetSplitScreen(KTerm* term, bool active, int row, int top_idx, int bot_idx);
void KTerm_WriteCharToSession(KTerm* term, int session_index, unsigned char ch);
void KTerm_SetResponseEnabled(KTerm* term, int session_index, bool enable);
bool KTerm_InitSession(KTerm* term, int index);

// KTerm lifecycle
bool KTerm_Init(KTerm* term);
void KTerm_Cleanup(KTerm* term);
void KTerm_Update(KTerm* term);  // Process events, update states (e.g., cursor blink)
void KTerm_Draw(KTerm* term);    // Render the terminal state to screen

// VT compliance and identification
bool KTerm_GetKey(KTerm* term, KTermEvent* event); // Retrieve buffered event
void KTerm_QueueInputEvent(KTerm* term, KTermEvent event); // Push event to buffer
void KTerm_SetLevel(KTerm* term, VTLevel level);
VTLevel KTerm_GetLevel(KTerm* term);
// void EnableVTFeature(const char* feature, bool enable); // e.g., "sixel", "DECCKM" - Deprecated by KTerm_SetLevel
// bool IsVTFeatureSupported(const char* feature); - Deprecated by direct struct access
void GetDeviceAttributes(KTerm* term, char* primary, char* secondary, size_t buffer_size);

// Enhanced pipeline management (for host input)
bool KTerm_WriteChar(KTerm* term, unsigned char ch);
bool KTerm_WriteString(KTerm* term, const char* str);
bool KTerm_WriteFormat(KTerm* term, const char* format, ...);
// bool PipelineWriteUTF8(const char* utf8_str); // Requires UTF-8 decoding logic
void KTerm_ProcessEvents(KTerm* term); // Process characters from the pipeline
void KTerm_ClearEvents(KTerm* term);
int KTerm_GetPendingEventCount(KTerm* term);
bool KTerm_IsEventOverflow(KTerm* term);

// Performance management
void KTerm_SetPipelineTargetFPS(KTerm* term, int fps);    // Helps tune processing budget
void KTerm_SetPipelineTimeBudget(KTerm* term, double pct); // Percentage of frame time for pipeline

// Mouse support (enhanced)
void KTerm_SetMouseTracking(KTerm* term, MouseTrackingMode mode); // Explicitly set a mouse mode
void KTerm_EnableMouseFeature(KTerm* term, const char* feature, bool enable); // e.g., "focus", "sgr"
// void KTerm_UpdateMouse(KTerm* term); // Removed in v2.1
// void KTerm_UpdateKeyboard(KTerm* term); // Removed in v2.1
// void UpdateKeyboard(KTerm* term);  // Removed in v2.1
// bool GetKeyEvent(KTerm* term, KeyEvent* event);  // Removed in v2.1
void KTerm_SetKeyboardMode(KTerm* term, const char* mode, bool enable); // "application_cursor", "keypad_numeric"
void KTerm_DefineFunctionKey(KTerm* term, int key_num, const char* sequence); // Program F1-F24

// KTerm control and modes
void KTerm_SetMode(KTerm* term, const char* mode, bool enable); // Generic mode setting by name
void KTerm_SetCursorShape(KTerm* term, CursorShape shape);
void KTerm_SetCursorKTermColor(KTerm* term, ExtendedKTermColor color);

// Character sets and encoding
void KTerm_SelectCharacterSet(KTerm* term, int gset, CharacterSet charset); // Designate G0-G3
void KTerm_SetCharacterSet(KTerm* term, CharacterSet charset); // Set current GL (usually G0)
unsigned int TranslateCharacter(KTerm* term, unsigned char ch, CharsetState* state); // Translate based on active CS

// Tab stops
void KTerm_SetTabStop(KTerm* term, int column);
void KTerm_ClearTabStop(KTerm* term, int column);
void KTerm_ClearAllTabStops(KTerm* term);
int NextTabStop(KTerm* term, int current_column);
int PreviousTabStop(KTerm* term, int current_column); // Added for CBT

// Bracketed paste
void KTerm_EnableBracketedPaste(KTerm* term, bool enable); // Enable/disable CSI ? 2004 h/l
bool IsBracketedPasteActive(KTerm* term);
void ProcessPasteData(KTerm* term, const char* data, size_t length); // Handle pasted data

// Rectangular operations (VT420+)
void KTerm_DefineRectangle(KTerm* term, int top, int left, int bottom, int right); // (DECSERA, DECFRA, DECCRA)
void KTerm_ExecuteRectangularOperation(KTerm* term, RectOperation op, const EnhancedTermChar* fill_char);
void KTerm_CopyRectangle(KTerm* term, VTRectangle src, int dest_x, int dest_y);
void KTerm_ExecuteRectangularOps(KTerm* term);  // DECCRA Implementation
void KTerm_ExecuteRectangularOps2(KTerm* term); // DECRQCRA Implementation

// Sixel graphics
void KTerm_InitSixelGraphics(KTerm* term);
void KTerm_ProcessSixelData(KTerm* term, KTermSession* session, const char* data, size_t length); // Process raw Sixel string
void KTerm_DrawSixelGraphics(KTerm* term); // Render current Sixel image

// Soft fonts
void KTerm_LoadSoftFont(KTerm* term, const unsigned char* font_data, int char_start, int char_count); // DECDLD
void KTerm_SelectSoftFont(KTerm* term, bool enable); // Enable/disable use of loaded soft font

// Title management
void KTerm_SetWindowTitle(KTerm* term, const char* title); // Set window title (OSC 0, OSC 2)
void KTerm_SetIconTitle(KTerm* term, const char* title);   // Set icon title (OSC 1)
const char* KTerm_GetWindowTitle(KTerm* term);
const char* KTerm_GetIconTitle(KTerm* term);

// Callbacks
void KTerm_SetResponseCallback(KTerm* term, ResponseCallback callback);
void KTerm_SetPrinterCallback(KTerm* term, PrinterCallback callback);
void KTerm_SetTitleCallback(KTerm* term, TitleCallback callback);
void KTerm_SetBellCallback(KTerm* term, BellCallback callback);
void KTerm_SetNotificationCallback(KTerm* term, NotificationCallback callback);
void KTerm_SetGatewayCallback(KTerm* term, GatewayCallback callback);
void KTerm_SetSessionResizeCallback(KTerm* term, SessionResizeCallback callback);

// Testing and diagnostics
void KTerm_RunTest(KTerm* term, const char* test_name); // Run predefined test sequences
void KTerm_ShowInfo(KTerm* term);           // Display current terminal state/info
void KTerm_EnableDebug(KTerm* term, bool enable);     // Toggle verbose debug logging
void KTerm_LogUnsupportedSequence(KTerm* term, const char* sequence); // Log an unsupported sequence
KTermStatus KTerm_GetStatus(KTerm* term);
void KTerm_ShowDiagnostics(KTerm* term);      // Display buffer usage info

// Screen buffer management
void KTerm_SwapScreenBuffer(KTerm* term); // Handles 1047/1049 logic

void KTerm_LoadFont(KTerm* term, const char* filepath);
void KTerm_CalculateFontMetrics(const void* data, int count, int width, int height, int stride, bool is_16bit, KTermFontMetric* metrics_out);

// Clipboard
void KTerm_CopySelectionToClipboard(KTerm* term);

// Helper to allocate a glyph index in the dynamic atlas for any Unicode codepoint
uint32_t KTerm_AllocateGlyph(KTerm* term, uint32_t codepoint);

// Resize the terminal grid and window texture
void KTerm_Resize(KTerm* term, int cols, int rows);

// Multiplexer API
KTermPane* KTerm_SplitPane(KTerm* term, KTermPane* target_pane, KTermPaneType split_type, float ratio);

// Internal rendering/parsing functions (potentially exposed for advanced use or testing)
void KTerm_CreateFontTexture(KTerm* term);

// Internal helper forward declaration
void KTerm_InitCompute(KTerm* term);

// Low-level char processing (called by KTerm_ProcessEvents via KTerm_ProcessChar)
void KTerm_ProcessChar(KTerm* term, KTermSession* session, unsigned char ch); // Main dispatcher for character processing
void KTerm_ProcessPrinterControllerChar(KTerm* term, KTermSession* session, unsigned char ch); // Handle Printer Controller Mode
void KTerm_ProcessNormalChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessEscapeChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessCSIChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessOSCChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessDCSChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessAPCChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessPMChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessSOSChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessVT52Char(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessKittyChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessSixelChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessSixelSTChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessControlChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessStringTerminator(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessCharsetCommand(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessHashChar(KTerm* term, KTermSession* session, unsigned char ch);
void KTerm_ProcessPercentChar(KTerm* term, KTermSession* session, unsigned char ch);


// Screen manipulation internals
void KTerm_ScrollUpRegion(KTerm* term, int top, int bottom, int lines);
void KTerm_InsertLinesAt(KTerm* term, int row, int count); // Added IL
void KTerm_DeleteLinesAt(KTerm* term, int row, int count); // Added DL
void KTerm_InsertCharactersAt(KTerm* term, int row, int col, int count); // Added ICH
void KTerm_DeleteCharactersAt(KTerm* term, int row, int col, int count); // Added DCH
void KTerm_InsertCharacterAtCursor(KTerm* term, unsigned int ch); // Handles character placement and insert mode
void KTerm_ScrollDownRegion(KTerm* term, int top, int bottom, int lines);

void KTerm_ExecuteSaveCursor(KTerm* term);
static void KTerm_ExecuteSaveCursor_Internal(KTermSession* session);
void KTerm_ExecuteRestoreCursor(KTerm* term);
static void KTerm_ExecuteRestoreCursor_Internal(KTermSession* session);

// Response and parsing helpers
void KTerm_QueueResponse(KTerm* term, const char* response); // Add string to answerback_buffer
void KTerm_QueueResponseBytes(KTerm* term, const char* data, size_t len);
static void KTerm_ParseGatewayCommand(KTerm* term, KTermSession* session, const char* data, size_t len); // Gateway Protocol Parser
int KTerm_ParseCSIParams(KTerm* term, const char* params, int* out_params, int max_params); // Parses CSI parameter string into escape_params
int KTerm_GetCSIParam(KTerm* term, int index, int default_value); // Gets a parsed CSI parameter
void KTerm_ExecuteCSICommand(KTerm* term, unsigned char command);
void KTerm_ExecuteOSCCommand(KTerm* term, KTermSession* session);
void KTerm_ExecuteDCSCommand(KTerm* term, KTermSession* session);
void KTerm_ExecuteAPCCommand(KTerm* term, KTermSession* session);
void KTerm_ExecuteKittyCommand(KTerm* term, KTermSession* session);
void KTerm_ExecutePMCommand(KTerm* term, KTermSession* session);
void KTerm_ExecuteSOSCommand(KTerm* term, KTermSession* session);
void KTerm_ExecuteDCSAnswerback(KTerm* term, KTermSession* session);

// Cell and attribute helpers
void KTerm_ClearCell(KTerm* term, EnhancedTermChar* cell); // Clears a cell with current attributes
void KTerm_ResetAllAttributes(KTerm* term);          // Resets current text attributes to default

// Character set translation helpers
unsigned int KTerm_TranslateDECSpecial(KTerm* term, unsigned char ch);
unsigned int KTerm_TranslateDECMultinational(KTerm* term, unsigned char ch);

// Keyboard sequence generation helpers (Removed in v2.1)

// Scripting API functions
void KTerm_Script_PutChar(KTerm* term, unsigned char ch);
void KTerm_Script_Print(KTerm* term, const char* text);
void KTerm_Script_Printf(KTerm* term, const char* format, ...);
void KTerm_Script_Cls(KTerm* term);
void KTerm_Script_SetKTermColor(KTerm* term, int fg, int bg);

#ifdef KTERM_IMPLEMENTATION
#define GET_SESSION(term) (&(term)->sessions[(term)->active_session])


// =============================================================================
// IMPLEMENTATION BEGINS HERE
// =============================================================================

// Fixed global variable definitions
//VTKeyboard vt_keyboard = {0};   // deprecated
// RGLTexture font_texture = {0};  // Moved to terminal struct
// Callbacks moved to struct

// Color mappings - Fixed initialization
// RGB_KTermColor color_palette[256]; // Moved to struct

KTermColor ansi_colors[16] = { // KTerm Color type (Standard CGA/VGA Palette)
    {0x00, 0x00, 0x00, 0xFF}, // 0: Black
    {0xAA, 0x00, 0x00, 0xFF}, // 1: Red
    {0x00, 0xAA, 0x00, 0xFF}, // 2: Green
    {0xAA, 0x55, 0x00, 0xFF}, // 3: Yellow (Brown)
    {0x00, 0x00, 0xAA, 0xFF}, // 4: Blue
    {0xAA, 0x00, 0xAA, 0xFF}, // 5: Magenta
    {0x00, 0xAA, 0xAA, 0xFF}, // 6: Cyan
    {0xAA, 0xAA, 0xAA, 0xFF}, // 7: White (Light Gray)
    {0x55, 0x55, 0x55, 0xFF}, // 8: Bright Black (Dark Gray)
    {0xFF, 0x55, 0x55, 0xFF}, // 9: Bright Red
    {0x55, 0xFF, 0x55, 0xFF}, // 10: Bright Green
    {0xFF, 0xFF, 0x55, 0xFF}, // 11: Bright Yellow
    {0x55, 0x55, 0xFF, 0xFF}, // 12: Bright Blue
    {0xFF, 0x55, 0xFF, 0xFF}, // 13: Bright Magenta
    {0x55, 0xFF, 0xFF, 0xFF}, // 14: Bright Cyan
    {0xFF, 0xFF, 0xFF, 0xFF}  // 15: Bright White
};

// Add missing function declaration
void KTerm_InitFontData(KTerm* term); // In case it's used elsewhere, though font_data is static

#include "font_data.h"

// =============================================================================
// SAFE PARSING PRIMITIVES
// =============================================================================
// Phase 7.1: Safe Parsing Primitives
typedef struct {
    const char* ptr;
    size_t len;
    size_t pos;
} StreamScanner;

// Helper: Peek next character
static inline char Stream_Peek(StreamScanner* scanner) {
    if (scanner->pos >= scanner->len) return 0;
    return scanner->ptr[scanner->pos];
}

// Helper: Consume next character
static inline char Stream_Consume(StreamScanner* scanner) {
    if (scanner->pos >= scanner->len) return 0;
    return scanner->ptr[scanner->pos++];
}

static inline bool Stream_Expect(StreamScanner* scanner, char expected) {
    if (Stream_Peek(scanner) == expected) {
        Stream_Consume(scanner);
        return true;
    }
    return false;
}

static inline bool Stream_ReadInt(StreamScanner* scanner, int* out_val) {
    if (scanner->pos >= scanner->len) return false;

    int sign = 1;
    char ch = Stream_Peek(scanner);
    if (ch == '-') {
        sign = -1;
        Stream_Consume(scanner);
    } else if (ch == '+') {
        Stream_Consume(scanner);
    }

    if (!isdigit((unsigned char)Stream_Peek(scanner))) return false;

    int val = 0;
    while (scanner->pos < scanner->len && isdigit((unsigned char)Stream_Peek(scanner))) {
        val = val * 10 + (Stream_Consume(scanner) - '0');
    }
    *out_val = val * sign;
    return true;
}

// Extended font data with larger character matrix for better rendering
/*unsigned char cp437_font__8x16[256 * 16 * 2] = {
    // 0x00 - NULL (empty)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};*/

// static uint32_t charset_lut[32][128]; // Moved to struct

void InitCharacterSetLUT(KTerm* term) {
    // 1. Initialize all to ASCII identity first
    for (int s = 0; s < 32; s++) {
        for (int c = 0; c < 128; c++) {
            term->charset_lut[s][c] = c;
        }
    }

    // 2. DEC Special Graphics
    for (int c = 0; c < 128; c++) {
        term->charset_lut[CHARSET_DEC_SPECIAL][c] = KTerm_TranslateDECSpecial(term, c);
    }

    // 3. National Replacement Character Sets (NRCS)
    // UK
    term->charset_lut[CHARSET_UK]['#'] = 0x00A3; // £

    // Dutch
    term->charset_lut[CHARSET_DUTCH]['#'] = 0x00A3; // £
    term->charset_lut[CHARSET_DUTCH]['@'] = 0x00BE; // ¾
    term->charset_lut[CHARSET_DUTCH]['['] = 0x0133; // ij (digraph) - approximated as ĳ
    term->charset_lut[CHARSET_DUTCH]['\\'] = 0x00BD; // ½
    term->charset_lut[CHARSET_DUTCH][']'] = 0x007C; // |
    term->charset_lut[CHARSET_DUTCH]['{'] = 0x00A8; // ¨
    term->charset_lut[CHARSET_DUTCH]['|'] = 0x0192; // f (florin)
    term->charset_lut[CHARSET_DUTCH]['}'] = 0x00BC; // ¼
    term->charset_lut[CHARSET_DUTCH]['~'] = 0x00B4; // ´

    // Finnish
    term->charset_lut[CHARSET_FINNISH]['['] = 0x00C4; // Ä
    term->charset_lut[CHARSET_FINNISH]['\\'] = 0x00D6; // Ö
    term->charset_lut[CHARSET_FINNISH][']'] = 0x00C5; // Å
    term->charset_lut[CHARSET_FINNISH]['^'] = 0x00DC; // Ü
    term->charset_lut[CHARSET_FINNISH]['`'] = 0x00E9; // é
    term->charset_lut[CHARSET_FINNISH]['{'] = 0x00E4; // ä
    term->charset_lut[CHARSET_FINNISH]['|'] = 0x00F6; // ö
    term->charset_lut[CHARSET_FINNISH]['}'] = 0x00E5; // å
    term->charset_lut[CHARSET_FINNISH]['~'] = 0x00FC; // ü

    // French
    term->charset_lut[CHARSET_FRENCH]['#'] = 0x00A3; // £
    term->charset_lut[CHARSET_FRENCH]['@'] = 0x00E0; // à
    term->charset_lut[CHARSET_FRENCH]['['] = 0x00B0; // °
    term->charset_lut[CHARSET_FRENCH]['\\'] = 0x00E7; // ç
    term->charset_lut[CHARSET_FRENCH][']'] = 0x00A7; // §
    term->charset_lut[CHARSET_FRENCH]['{'] = 0x00E9; // é
    term->charset_lut[CHARSET_FRENCH]['|'] = 0x00F9; // ù
    term->charset_lut[CHARSET_FRENCH]['}'] = 0x00E8; // è
    term->charset_lut[CHARSET_FRENCH]['~'] = 0x00A8; // ¨

    // French Canadian (Similar to French but with subtle differences in some standards, using VT standard map)
    term->charset_lut[CHARSET_FRENCH_CANADIAN]['@'] = 0x00E0; // à
    term->charset_lut[CHARSET_FRENCH_CANADIAN]['['] = 0x00E2; // â
    term->charset_lut[CHARSET_FRENCH_CANADIAN]['\\'] = 0x00E7; // ç
    term->charset_lut[CHARSET_FRENCH_CANADIAN][']'] = 0x00EA; // ê
    term->charset_lut[CHARSET_FRENCH_CANADIAN]['^'] = 0x00EE; // î
    term->charset_lut[CHARSET_FRENCH_CANADIAN]['`'] = 0x00F4; // ô
    term->charset_lut[CHARSET_FRENCH_CANADIAN]['{'] = 0x00E9; // é
    term->charset_lut[CHARSET_FRENCH_CANADIAN]['|'] = 0x00F9; // ù
    term->charset_lut[CHARSET_FRENCH_CANADIAN]['}'] = 0x00E8; // è
    term->charset_lut[CHARSET_FRENCH_CANADIAN]['~'] = 0x00FB; // û

    // German
    term->charset_lut[CHARSET_GERMAN]['@'] = 0x00A7; // §
    term->charset_lut[CHARSET_GERMAN]['['] = 0x00C4; // Ä
    term->charset_lut[CHARSET_GERMAN]['\\'] = 0x00D6; // Ö
    term->charset_lut[CHARSET_GERMAN][']'] = 0x00DC; // Ü
    term->charset_lut[CHARSET_GERMAN]['{'] = 0x00E4; // ä
    term->charset_lut[CHARSET_GERMAN]['|'] = 0x00F6; // ö
    term->charset_lut[CHARSET_GERMAN]['}'] = 0x00FC; // ü
    term->charset_lut[CHARSET_GERMAN]['~'] = 0x00DF; // ß

    // Italian
    term->charset_lut[CHARSET_ITALIAN]['#'] = 0x00A3; // £
    term->charset_lut[CHARSET_ITALIAN]['@'] = 0x00A7; // §
    term->charset_lut[CHARSET_ITALIAN]['['] = 0x00B0; // °
    term->charset_lut[CHARSET_ITALIAN]['\\'] = 0x00E7; // ç
    term->charset_lut[CHARSET_ITALIAN][']'] = 0x00E9; // é
    term->charset_lut[CHARSET_ITALIAN]['`'] = 0x00F9; // ù
    term->charset_lut[CHARSET_ITALIAN]['{'] = 0x00E0; // à
    term->charset_lut[CHARSET_ITALIAN]['|'] = 0x00F2; // ò
    term->charset_lut[CHARSET_ITALIAN]['}'] = 0x00E8; // è
    term->charset_lut[CHARSET_ITALIAN]['~'] = 0x00EC; // ì

    // Norwegian/Danish
    // Note: There are two variants (E and 6), usually identical.
    term->charset_lut[CHARSET_NORWEGIAN_DANISH]['@'] = 0x00C4; // Ä
    term->charset_lut[CHARSET_NORWEGIAN_DANISH]['['] = 0x00C6; // Æ
    term->charset_lut[CHARSET_NORWEGIAN_DANISH]['\\'] = 0x00D8; // Ø
    term->charset_lut[CHARSET_NORWEGIAN_DANISH][']'] = 0x00C5; // Å
    term->charset_lut[CHARSET_NORWEGIAN_DANISH]['^'] = 0x00DC; // Ü
    term->charset_lut[CHARSET_NORWEGIAN_DANISH]['`'] = 0x00E4; // ä
    term->charset_lut[CHARSET_NORWEGIAN_DANISH]['{'] = 0x00E6; // æ
    term->charset_lut[CHARSET_NORWEGIAN_DANISH]['|'] = 0x00F8; // ø
    term->charset_lut[CHARSET_NORWEGIAN_DANISH]['}'] = 0x00E5; // å
    term->charset_lut[CHARSET_NORWEGIAN_DANISH]['~'] = 0x00FC; // ü

    // Spanish
    term->charset_lut[CHARSET_SPANISH]['#'] = 0x00A3; // £
    term->charset_lut[CHARSET_SPANISH]['@'] = 0x00A7; // §
    term->charset_lut[CHARSET_SPANISH]['['] = 0x00A1; // ¡
    term->charset_lut[CHARSET_SPANISH]['\\'] = 0x00D1; // Ñ
    term->charset_lut[CHARSET_SPANISH][']'] = 0x00BF; // ¿
    term->charset_lut[CHARSET_SPANISH]['{'] = 0x00B0; // °
    term->charset_lut[CHARSET_SPANISH]['|'] = 0x00F1; // ñ
    term->charset_lut[CHARSET_SPANISH]['}'] = 0x00E7; // ç

    // Swedish
    term->charset_lut[CHARSET_SWEDISH]['@'] = 0x00C9; // É
    term->charset_lut[CHARSET_SWEDISH]['['] = 0x00C4; // Ä
    term->charset_lut[CHARSET_SWEDISH]['\\'] = 0x00D6; // Ö
    term->charset_lut[CHARSET_SWEDISH][']'] = 0x00C5; // Å
    term->charset_lut[CHARSET_SWEDISH]['^'] = 0x00DC; // Ü
    term->charset_lut[CHARSET_SWEDISH]['`'] = 0x00E9; // é
    term->charset_lut[CHARSET_SWEDISH]['{'] = 0x00E4; // ä
    term->charset_lut[CHARSET_SWEDISH]['|'] = 0x00F6; // ö
    term->charset_lut[CHARSET_SWEDISH]['}'] = 0x00E5; // å
    term->charset_lut[CHARSET_SWEDISH]['~'] = 0x00FC; // ü

    // Swiss
    term->charset_lut[CHARSET_SWISS]['#'] = 0x00F9; // ù
    term->charset_lut[CHARSET_SWISS]['@'] = 0x00E0; // à
    term->charset_lut[CHARSET_SWISS]['['] = 0x00E9; // é
    term->charset_lut[CHARSET_SWISS]['\\'] = 0x00E7; // ç
    term->charset_lut[CHARSET_SWISS][']'] = 0x00EA; // ê
    term->charset_lut[CHARSET_SWISS]['^'] = 0x00EE; // î
    term->charset_lut[CHARSET_SWISS]['_'] = 0x00E8; // è
    term->charset_lut[CHARSET_SWISS]['`'] = 0x00F4; // ô
    term->charset_lut[CHARSET_SWISS]['{'] = 0x00E4; // ä
    term->charset_lut[CHARSET_SWISS]['|'] = 0x00F6; // ö
    term->charset_lut[CHARSET_SWISS]['}'] = 0x00FC; // ü
    term->charset_lut[CHARSET_SWISS]['~'] = 0x00FB; // û
}

void KTerm_InitFontData(KTerm* term) {
    (void)term;
    // This function is currently empty.
    // The font_data array is initialized statically.
    // If font_data needed dynamic initialization or loading from a file,
    // it would happen here.
}

// CP437 to Unicode Mapping Table
static const uint16_t kCp437ToUnicode[256] = {
    // 0x00 - 0x1F
    0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
    0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
    0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
    // 0x20 - 0x7E (ASCII)
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
    // 0x80 - 0xFF (Extended)
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

static void KTerm_InitCP437Map(KTerm* term) {
    if (!term->glyph_map) return;
    for (int i = 0; i < 256; i++) {
        uint16_t u = kCp437ToUnicode[i];
        if (u != 0) {
            term->glyph_map[u] = (uint16_t)i;
        }
    }
}

// =============================================================================
// REST OF THE IMPLEMENTATION
// =============================================================================

void KTerm_InitKTermColorPalette(KTerm* term) {
    for (int i = 0; i < 16; i++) {
        term->color_palette[i] = (RGB_KTermColor){ ansi_colors[i].r, ansi_colors[i].g, ansi_colors[i].b, 255 };
    }
    int index = 16;
    for (int r = 0; r < 6; r++) {
        for (int g = 0; g < 6; g++) {
            for (int b = 0; b < 6; b++) {
                term->color_palette[index++] = (RGB_KTermColor){
                    (unsigned char)(r ? 55 + r * 40 : 0),
                    (unsigned char)(g ? 55 + g * 40 : 0),
                    (unsigned char)(b ? 55 + b * 40 : 0),
                    255
                };
            }
        }
    }
    for (int i = 0; i < 24; i++) {
        unsigned char gray = 8 + i * 10;
        term->color_palette[232 + i] = (RGB_KTermColor){gray, gray, gray, 255};
    }
}

void KTerm_InitVTConformance(KTerm* term) {
    GET_SESSION(term)->conformance.level = VT_LEVEL_XTERM;
    GET_SESSION(term)->conformance.strict_mode = false;
    KTerm_SetLevel(term, GET_SESSION(term)->conformance.level);
    GET_SESSION(term)->conformance.compliance.unsupported_sequences = 0;
    GET_SESSION(term)->conformance.compliance.partial_implementations = 0;
    GET_SESSION(term)->conformance.compliance.extensions_used = 0;
    GET_SESSION(term)->conformance.compliance.last_unsupported[0] = '\0';
}

void KTerm_InitTabStops(KTerm* term) {
    KTermSession* session = GET_SESSION(term);
    if (session->tab_stops.stops) {
        free(session->tab_stops.stops);
    }

    int capacity = term->width;
    if (capacity < MAX_TAB_STOPS) capacity = MAX_TAB_STOPS;

    session->tab_stops.stops = (bool*)calloc(capacity, sizeof(bool));
    session->tab_stops.capacity = capacity;
    session->tab_stops.count = 0;
    session->tab_stops.default_width = 8;

    for (int i = session->tab_stops.default_width; i < capacity; i += session->tab_stops.default_width) {
        session->tab_stops.stops[i] = true;
        session->tab_stops.count++;
    }
}

void KTerm_InitCharacterSets(KTerm* term) {
    GET_SESSION(term)->charset.g0 = CHARSET_ASCII;
    GET_SESSION(term)->charset.g1 = CHARSET_DEC_SPECIAL;
    GET_SESSION(term)->charset.g2 = CHARSET_ASCII;
    GET_SESSION(term)->charset.g3 = CHARSET_ASCII;
    GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g0;
    GET_SESSION(term)->charset.gr = &GET_SESSION(term)->charset.g1;
    GET_SESSION(term)->charset.single_shift_2 = false;
    GET_SESSION(term)->charset.single_shift_3 = false;
}

// Initialize Input State
void KTerm_InitInputState(KTerm* term) {
    // Initialize keyboard modes and flags
    // application_cursor_keys is in dec_modes
    GET_SESSION(term)->input.keypad_application_mode = false; // Numeric keypad mode
    GET_SESSION(term)->input.meta_sends_escape = true; // Alt sends ESC prefix
    GET_SESSION(term)->input.delete_sends_del = true; // Delete sends DEL (\x7F)
    GET_SESSION(term)->input.backarrow_sends_bs = true; // Backspace sends BS (\x08)
    GET_SESSION(term)->input.keyboard_dialect = 1; // North American ASCII

    // Initialize function key mappings
    const char* function_key_sequences[] = {
        "\x1BOP", "\x1BOQ", "\x1BOR", "\x1BOS", // F1–F4
        "\x1B[15~", "\x1B[17~", "\x1B[18~", "\x1B[19~", // F5–F8
        "\x1B[20~", "\x1B[21~", "\x1B[23~", "\x1B[24~", // F9–F12
        "\x1B[25~", "\x1B[26~", "\x1B[28~", "\x1B[29~", // F13–F16
        "\x1B[31~", "\x1B[32~", "\x1B[33~", "\x1B[34~", // F17–F20
        "", "", "", "" // F21–F24 (unused)
    };
    for (int i = 0; i < 24; i++) {
        strncpy(GET_SESSION(term)->input.function_keys[i], function_key_sequences[i], 31);
        GET_SESSION(term)->input.function_keys[i][31] = '\0';
    }
}

KTerm* KTerm_Create(KTermConfig config) {
    KTerm* term = (KTerm*)calloc(1, sizeof(KTerm));
    if (!term) return NULL;

    // Apply config
    if (config.width > 0) term->width = config.width;
    else term->width = DEFAULT_TERM_WIDTH;

    if (config.height > 0) term->height = config.height;
    else term->height = DEFAULT_TERM_HEIGHT;

    term->response_callback = config.response_callback;

    if (!KTerm_Init(term)) {
        KTerm_Cleanup(term);
        free(term);
        return NULL;
    }
    return term;
}

static void KTerm_DestroyPane(KTermPane* pane) {
    if (!pane) return;
    KTerm_DestroyPane(pane->child_a);
    KTerm_DestroyPane(pane->child_b);
    free(pane);
}

void KTerm_Destroy(KTerm* term) {
    if (!term) return;
    KTerm_Cleanup(term);
    if (term->layout_root) {
        KTerm_DestroyPane(term->layout_root);
    }
    free(term);
}

void KTerm_CalculateFontMetrics(const void* data, int count, int width, int height, int stride, bool is_16bit, KTermFontMetric* metrics_out) {
    if (!data || !metrics_out) return;
    if (stride == 0) stride = height;

    for (int i = 0; i < count; i++) {
        int min_x = width;
        int max_x = -1;
        const uint8_t* char_data_8 = (const uint8_t*)data + (i * stride);
        const uint16_t* char_data_16 = (const uint16_t*)data + (i * stride);

        for (int y = 0; y < height; y++) {
            uint16_t row_data = 0;
            if (is_16bit) {
                row_data = char_data_16[y];
            } else {
                row_data = char_data_8[y];
            }

            for (int x = 0; x < width; x++) {
                 bool set = false;
                 if (is_16bit) {
                     if ((row_data >> (width - 1 - x)) & 1) set = true;
                 } else {
                     if ((row_data >> (7 - x)) & 1) set = true;
                 }
                 if (set) {
                     if (x < min_x) min_x = x;
                     if (x > max_x) max_x = x;
                 }
            }
        }

        metrics_out[i].width = width;
        if (max_x == -1) {
            metrics_out[i].begin_x = 0;
            metrics_out[i].end_x = 0;
        } else {
            metrics_out[i].begin_x = min_x;
            metrics_out[i].end_x = max_x;
        }
    }
}

bool KTerm_Init(KTerm* term) {
    KTerm_InitFontData(term);
    KTerm_InitKTermColorPalette(term);

    // Init global members
    if (term->width == 0) term->width = DEFAULT_TERM_WIDTH;
    if (term->height == 0) term->height = DEFAULT_TERM_HEIGHT;

    // Default Font
    term->char_width = DEFAULT_CHAR_WIDTH;
    term->char_height = DEFAULT_CHAR_HEIGHT;
    term->font_data_width = 8;
    term->font_data_height = 10;
    term->current_font_data = dec_vt220_cp437_8x10;
    term->current_font_is_16bit = false;
    KTerm_CalculateFontMetrics(term->current_font_data, 256, term->font_data_width, term->font_data_height, 0, false, term->font_metrics);
    term->active_session = 0;
    term->pending_session_switch = -1;
    term->split_screen_active = false;
    term->split_row = term->height / 2;
    term->session_top = 0;
    term->session_bottom = 1;
    term->visual_effects.curvature = 0.0f;
    term->visual_effects.scanline_intensity = 0.0f;
    term->tektronix.extra_byte = -1;

    // Init Multiplexer
    term->mux_input.active = false;
    term->mux_input.prefix_key_code = 'B';

    // Init ReGIS resolution defaults (standard 800x480)
    term->regis.screen_min_x = 0;
    term->regis.screen_min_y = 0;
    term->regis.screen_max_x = REGIS_WIDTH - 1;
    term->regis.screen_max_y = REGIS_HEIGHT - 1;

    // Init sessions
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!KTerm_InitSession(term, i)) return false;

        // Context switch to use existing helper functions
        int saved = term->active_session;
        term->active_session = i;

        KTerm_InitVTConformance(term);
        KTerm_InitTabStops(term);
        KTerm_InitCharacterSets(term);
        KTerm_InitInputState(term);
        KTerm_InitSixelGraphics(term);

        // Initialize Kitty Graphics
        memset(&term->sessions[i].kitty, 0, sizeof(term->sessions[i].kitty));
        term->sessions[i].kitty.images = NULL;
        term->sessions[i].kitty.image_count = 0;
        term->sessions[i].kitty.image_capacity = 0;

        // Only the first session is active by default in the multiplexer
        if (i > 0) {
            term->sessions[i].session_open = false;
        }

        term->active_session = saved;
    }
    term->active_session = 0;

    // Initialize Layout Tree
    term->layout_root = (KTermPane*)calloc(1, sizeof(KTermPane));
    if (!term->layout_root) return false;
    term->layout_root->type = PANE_LEAF;
    term->layout_root->session_index = 0;
    term->layout_root->parent = NULL;
    term->layout_root->width = term->width;
    term->layout_root->height = term->height;
    term->focused_pane = term->layout_root;

    InitCharacterSetLUT(term);

    // Allocate full Unicode map
    if (term->glyph_map) free(term->glyph_map);
    term->glyph_map = (uint16_t*)calloc(0x110000, sizeof(uint16_t));
    if (!term->glyph_map) return false;

    // Initialize Dynamic Atlas dimensions before creation
    term->atlas_width = 1024;
    term->atlas_height = 1024;
    term->atlas_cols = 128;

    // Allocate LRU Cache
    size_t capacity = (term->atlas_width / DEFAULT_CHAR_WIDTH) * (term->atlas_height / DEFAULT_CHAR_HEIGHT);
    term->glyph_last_used = (uint64_t*)calloc(capacity, sizeof(uint64_t));
    if (!term->glyph_last_used) return false;
    term->atlas_to_codepoint = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    if (!term->atlas_to_codepoint) return false;
    term->frame_count = 0;

    KTerm_InitCP437Map(term);

    KTerm_CreateFontTexture(term);
    KTerm_InitCompute(term);
    return true;
}



// String terminator handler for ESC P, ESC _, ESC ^, ESC X
void KTerm_ProcessStringTerminator(KTerm* term, KTermSession* session, unsigned char ch) {
    // Expects ST (ESC \) to terminate.
    // If ch is '\', we terminate and execute.
    // If ch is NOT '\', we terminate (implicitly), execute, and then process ch as start of new sequence.

    bool is_st = (ch == '\\');

    // Execute the pending command based on saved state
    // We terminate the string in buffer first
    if (session->escape_pos < MAX_COMMAND_BUFFER) {
        session->escape_buffer[session->escape_pos] = '\0';
    } else {
        session->escape_buffer[MAX_COMMAND_BUFFER - 1] = '\0';
    }

    switch (session->saved_parse_state) {
        case PARSE_OSC: KTerm_ExecuteOSCCommand(term, session); break;
        case PARSE_DCS: KTerm_ExecuteDCSCommand(term, session); break;
        case PARSE_APC: KTerm_ExecuteAPCCommand(term, session); break;
        case PARSE_PM:  KTerm_ExecutePMCommand(term, session); break;
        case PARSE_SOS: KTerm_ExecuteSOSCommand(term, session); break;
        case PARSE_KITTY:
            KTerm_ExecuteKittyCommand(term, session);
            break;
        default: break;
    }

    // Reset buffer
    session->escape_pos = 0;

    if (is_st) {
        session->parse_state = VT_PARSE_NORMAL;
    } else {
        // Not a valid ST, but ESC was seen previously.
        // The previous string is terminated.
        // We now process 'ch' as the character following ESC.
        session->parse_state = VT_PARSE_ESCAPE;
        KTerm_ProcessEscapeChar(term, session, ch);
    }
}

void KTerm_ProcessCharsetCommand(KTerm* term, KTermSession* session, unsigned char ch) {
    session->escape_buffer[session->escape_pos++] = ch;

    if (session->escape_pos >= 2) {
        char designator = session->escape_buffer[0];
        char charset_char = session->escape_buffer[1];
        CharacterSet selected_cs = CHARSET_ASCII;

        switch (charset_char) {
            case 'A': selected_cs = CHARSET_UK; break;
            case 'B': selected_cs = CHARSET_ASCII; break;
            case '0': selected_cs = CHARSET_DEC_SPECIAL; break;
            case '1':
            case '2':
                if (session->options.debug_sequences) {
                    KTerm_LogUnsupportedSequence(term, "DEC Alternate Character ROM not fully supported, using ASCII/DEC Special");
                }
                selected_cs = (charset_char == '1') ? CHARSET_ASCII : CHARSET_DEC_SPECIAL;
                break;
            case '<': selected_cs = CHARSET_DEC_MULTINATIONAL; break;
            // NRCS Designators
            case '4': selected_cs = CHARSET_DUTCH; break;
            case 'C': case '5': selected_cs = CHARSET_FINNISH; break;
            case 'R': case 'f': selected_cs = CHARSET_FRENCH; break;
            case 'Q': selected_cs = CHARSET_FRENCH_CANADIAN; break;
            case 'K': selected_cs = CHARSET_GERMAN; break;
            case 'Y': selected_cs = CHARSET_ITALIAN; break;
            case 'E': case '6': selected_cs = CHARSET_NORWEGIAN_DANISH; break;
            case 'Z': selected_cs = CHARSET_SPANISH; break;
            case 'H': case '7': selected_cs = CHARSET_SWEDISH; break;
            case '=': selected_cs = CHARSET_SWISS; break;
            default:
                if (session->options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown charset char: %c for designator %c", charset_char, designator);
                    KTerm_LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }

        switch (designator) {
            case '(': session->charset.g0 = selected_cs; break;
            case ')': session->charset.g1 = selected_cs; break;
            case '*': session->charset.g2 = selected_cs; break;
            case '+': session->charset.g3 = selected_cs; break;
        }

        session->parse_state = VT_PARSE_NORMAL;
        session->escape_pos = 0;
    }
}

// Stubs for APC/PM/SOS command execution
void KTerm_ExecuteAPCCommand(KTerm* term, KTermSession* session) {
    if (session->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "APC sequence executed (no-op)");
    // session->escape_buffer contains the APC string data.
}
void KTerm_ExecutePMCommand(KTerm* term, KTermSession* session) {
    if (session->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "PM sequence executed (no-op)");
    // session->escape_buffer contains the PM string data.
}
void KTerm_ExecuteSOSCommand(KTerm* term, KTermSession* session) {
    if (session->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "SOS sequence executed (no-op)");
    // session->escape_buffer contains the SOS string data.
}

// Generic string processor for APC, PM, SOS
void KTerm_ProcessGenericStringChar(KTerm* term, KTermSession* session, unsigned char ch, VTParseState next_state_on_escape, ExecuteCommandCallback execute_command_func) {
    (void)next_state_on_escape;
    if ((size_t)session->escape_pos < sizeof(session->escape_buffer) - 1) {
        if (ch == '\x1B') {
            session->saved_parse_state = session->parse_state;
            session->parse_state = PARSE_STRING_TERMINATOR;
            return;
        }

        session->escape_buffer[session->escape_pos++] = ch;

        // BEL is not a standard terminator for these, ST is.
    } else { // Buffer overflow
        session->escape_buffer[sizeof(session->escape_buffer) - 1] = '\0';
        if (execute_command_func) execute_command_func(term, session);
        session->parse_state = VT_PARSE_NORMAL;
        session->escape_pos = 0;
        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "String sequence (type %d) too long, truncated", (int)session->parse_state); // Log current state
        KTerm_LogUnsupportedSequence(term, log_msg);
    }
}

void KTerm_ProcessAPCChar(KTerm* term, KTermSession* session, unsigned char ch) {
    // Detect Kitty Graphics Protocol start
    if (session->escape_pos == 0 && ch == 'G') {
         session->parse_state = PARSE_KITTY;
         // Initialize Kitty Parser for new command
         memset(&session->kitty.cmd, 0, sizeof(session->kitty.cmd));
         session->kitty.state = 0; // KEY
         session->kitty.key_len = 0;
         session->kitty.val_len = 0;
         session->kitty.b64_accumulator = 0;
         session->kitty.b64_bits = 0;

         // Only reset active_upload if NOT continuing a chunked transmission
         if (!session->kitty.continuing) {
             session->kitty.active_upload = NULL;
         }

         // Set defaults
         session->kitty.cmd.action = 't'; // Default action is transmit
         session->kitty.cmd.format = 32;  // Default format RGBA
         session->kitty.cmd.medium = 0;   // Default medium 0 (direct)
         return;
    }
    KTerm_ProcessGenericStringChar(term, session, ch, VT_PARSE_ESCAPE /* Fallback if ST is broken */, KTerm_ExecuteAPCCommand);
}
void KTerm_ProcessPMChar(KTerm* term, KTermSession* session, unsigned char ch) { KTerm_ProcessGenericStringChar(term, session, ch, VT_PARSE_ESCAPE, KTerm_ExecutePMCommand); }
void KTerm_ProcessSOSChar(KTerm* term, KTermSession* session, unsigned char ch) { KTerm_ProcessGenericStringChar(term, session, ch, VT_PARSE_ESCAPE, KTerm_ExecuteSOSCommand); }

// Internal helper forward declaration
static void ProcessTektronixChar(KTerm* term, KTermSession* session, unsigned char ch);
static void ProcessReGISChar(KTerm* term, KTermSession* session, unsigned char ch);

// Process character when in Printer Controller Mode (pass-through)
void KTerm_ProcessPrinterControllerChar(KTerm* term, KTermSession* session, unsigned char ch) {
    // We must detect the exit sequence: CSI 4 i
    // CSI can be 7-bit (\x1B [) or 8-bit (\x9B)
    // Exit sequence:
    // 7-bit: ESC [ 4 i
    // 8-bit: CSI 4 i (\x9B 4 i)

    // Ensure session-specific buffering
    // Add to buffer
    if (session->printer_buf_len < 7) {
        session->printer_buffer[session->printer_buf_len++] = ch;
    } else {
        // Buffer full, flush oldest and shift
        if (term->printer_callback) {
            term->printer_callback(term, (const char*)&session->printer_buffer[0], 1);
        }
        memmove(session->printer_buffer, session->printer_buffer + 1, --session->printer_buf_len);
        session->printer_buffer[session->printer_buf_len++] = ch;
    }
    session->printer_buffer[session->printer_buf_len] = '\0';

    // Target Sequences
    const char* seq1 = "\x1B[4i"; // 7-bit CSI 4 i
    const char* seq2 = "\x9B""4i"; // 8-bit CSI 4 i

    // Sliding window check: Is the buffer a valid prefix or full match?
    // We check if the buffer *ends* with a valid prefix, but since we append one char at a time
    // and flush mismatches, we mainly check if the *start* matches.
    // However, we might have flushed part of a sequence if we aren't careful.
    // Actually, simple state buffering is:
    // 1. Append ch.
    // 2. Check if buffer matches or is prefix of targets.
    // 3. If exact match: Disable mode, Clear buffer.
    // 4. If prefix match: Return (keep buffering).
    // 5. If mismatch: Flush head until we have a prefix match or empty.

    while (session->printer_buf_len > 0) {
        // Check 7-bit
        bool match1 = true;
        for (int i = 0; i < session->printer_buf_len; i++) {
            if (i >= 4 || session->printer_buffer[i] != (unsigned char)seq1[i]) {
                match1 = false;
                break;
            }
        }
        if (match1 && session->printer_buf_len == 4) {
            session->printer_controller_enabled = false;
            session->printer_buf_len = 0;
            return;
        }

        // Check 8-bit
        bool match2 = true;
        for (int i = 0; i < session->printer_buf_len; i++) {
            if (i >= 3 || session->printer_buffer[i] != (unsigned char)seq2[i]) {
                match2 = false;
                break;
            }
        }
        if (match2 && session->printer_buf_len == 3) {
            session->printer_controller_enabled = false;
            session->printer_buf_len = 0;
            return;
        }

        if (match1 || match2) {
            // It's a valid prefix, wait for more data
            return;
        }

        // Mismatch: Flush the first character and retry
        if (term->printer_callback) {
            term->printer_callback(term, (const char*)&session->printer_buffer[0], 1);
        }
        memmove(session->printer_buffer, session->printer_buffer + 1, --session->printer_buf_len);
    }
}

// Continue with enhanced character processing...
void KTerm_ProcessChar(KTerm* term, KTermSession* session, unsigned char ch) {
    if (session->printer_controller_enabled) {
        KTerm_ProcessPrinterControllerChar(term, session, ch);
        return;
    }

    switch (session->parse_state) {
        case VT_PARSE_NORMAL:              KTerm_ProcessNormalChar(term, session, ch); break;
        case VT_PARSE_ESCAPE:              KTerm_ProcessEscapeChar(term, session, ch); break;
        case PARSE_CSI:                 KTerm_ProcessCSIChar(term, session, ch); break;
        case PARSE_OSC:                 KTerm_ProcessOSCChar(term, session, ch); break;
        case PARSE_DCS:                 KTerm_ProcessDCSChar(term, session, ch); break;
        case PARSE_SIXEL_ST:            KTerm_ProcessSixelSTChar(term, session, ch); break;
        case PARSE_VT52:                KTerm_ProcessVT52Char(term, session, ch); break;
        case PARSE_TEKTRONIX:           ProcessTektronixChar(term, session, ch); break;
        case PARSE_REGIS:               ProcessReGISChar(term, session, ch); break;
        case PARSE_KITTY:               KTerm_ProcessKittyChar(term, session, ch); break;
        case PARSE_SIXEL:               KTerm_ProcessSixelChar(term, session, ch); break;
        case PARSE_CHARSET:             KTerm_ProcessCharsetCommand(term, session, ch); break;
        case PARSE_HASH:                KTerm_ProcessHashChar(term, session, ch); break;
        case PARSE_PERCENT:             KTerm_ProcessPercentChar(term, session, ch); break;
        case PARSE_APC:                 KTerm_ProcessAPCChar(term, session, ch); break;
        case PARSE_PM:                  KTerm_ProcessPMChar(term, session, ch); break;
        case PARSE_SOS:                 KTerm_ProcessSOSChar(term, session, ch); break;
        case PARSE_STRING_TERMINATOR:   KTerm_ProcessStringTerminator(term, session, ch); break;
        default:
            session->parse_state = VT_PARSE_NORMAL;
            KTerm_ProcessNormalChar(term, session, ch);
            break;
    }
}

// CSI Pts ; Pls ; Pbs ; Prs ; Pps ; Ptd ; Pld ; Ppd $ v
void ExecuteDECCRA(KTerm* term) { // Copy Rectangular Area (DECCRA)
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        KTerm_LogUnsupportedSequence(term, "DECCRA requires rectangular operations support");
        return;
    }
    if (GET_SESSION(term)->param_count != 8) {
        KTerm_LogUnsupportedSequence(term, "Invalid parameters for DECCRA");
        return;
    }
    int top = KTerm_GetCSIParam(term, 0, 1) - 1;
    int left = KTerm_GetCSIParam(term, 1, 1) - 1;
    int bottom = KTerm_GetCSIParam(term, 2, 1) - 1;
    int right = KTerm_GetCSIParam(term, 3, 1) - 1;
    // Ps4 is source page, not supported.
    int dest_top = KTerm_GetCSIParam(term, 5, 1) - 1;
    int dest_left = KTerm_GetCSIParam(term, 6, 1) - 1;
    // Ps8 is destination page, not supported.

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= term->height) bottom = term->height - 1;
    if (right >= term->width) right = term->width - 1;
    if (top > bottom || left > right) return;

    VTRectangle rect = {top, left, bottom, right, true};
    KTerm_CopyRectangle(term, rect, dest_left, dest_top);
}

static unsigned int KTerm_CalculateRectChecksum(KTerm* term, int top, int left, int bottom, int right) {
    unsigned int checksum = 0;
    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
            if (cell) {
                checksum += cell->ch;
            }
        }
    }
    return checksum;
}

void ExecuteDECECR(KTerm* term) { // Enable Checksum Reporting
    // CSI Pt ; Pc z (Enable/Disable Checksum Reporting)
    // Pt = Request ID (ignored/stored)
    // Pc = 0 (Disable), 1 (Enable)
    int pc = KTerm_GetCSIParam(term, 1, 0); // Default to 0 (Disable)

    if (pc == 1) {
        GET_SESSION(term)->dec_modes.checksum_reporting = true;
    } else {
        GET_SESSION(term)->dec_modes.checksum_reporting = false;
    }
}

void ExecuteDECRQCRA(KTerm* term) { // Request Rectangular Area Checksum
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        KTerm_LogUnsupportedSequence(term, "DECRQCRA requires rectangular operations support");
        return;
    }

    if (!GET_SESSION(term)->dec_modes.checksum_reporting) {
        return; // Checksum reporting disabled
    }

    // CSI Pid ; Pp ; Pt ; Pl ; Pb ; Pr $ w
    int pid = KTerm_GetCSIParam(term, 0, 1);
    // int page = KTerm_GetCSIParam(term, 1, 1); // Ignored
    int top = KTerm_GetCSIParam(term, 2, 1) - 1;
    int left = KTerm_GetCSIParam(term, 3, 1) - 1;
    int bottom = KTerm_GetCSIParam(term, 4, term->height) - 1;
    int right = KTerm_GetCSIParam(term, 5, term->width) - 1;

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= term->height) bottom = term->height - 1;
    if (right >= term->width) right = term->width - 1;

    unsigned int checksum = 0;
    if (top <= bottom && left <= right) {
        checksum = KTerm_CalculateRectChecksum(term, top, left, bottom, right);
    }

    // Response: DCS Pid ! ~ Checksum ST
    char response[32];
    snprintf(response, sizeof(response), "\x1BP%d!~%04X\x1B\\", pid, checksum & 0xFFFF);
    KTerm_QueueResponse(term, response);
}

// CSI Pch ; Pt ; Pl ; Pb ; Pr $ x
void ExecuteDECFRA(KTerm* term) { // Fill Rectangular Area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        KTerm_LogUnsupportedSequence(term, "DECFRA requires rectangular operations support");
        return;
    }

    if (GET_SESSION(term)->param_count != 5) {
        KTerm_LogUnsupportedSequence(term, "Invalid parameters for DECFRA");
        return;
    }

    int char_code = KTerm_GetCSIParam(term, 0, ' ');
    int top = KTerm_GetCSIParam(term, 1, 1) - 1;
    int left = KTerm_GetCSIParam(term, 2, 1) - 1;
    int bottom = KTerm_GetCSIParam(term, 3, 1) - 1;
    int right = KTerm_GetCSIParam(term, 4, 1) - 1;

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= term->height) bottom = term->height - 1;
    if (right >= term->width) right = term->width - 1;
    if (top > bottom || left > right) return;

    unsigned int fill_char = (unsigned int)char_code;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
            cell->ch = fill_char;
            cell->fg_color = GET_SESSION(term)->current_fg;
            cell->bg_color = GET_SESSION(term)->current_bg;
            cell->flags = GET_SESSION(term)->current_attributes | KTERM_FLAG_DIRTY;
        }
        GET_SESSION(term)->row_dirty[y] = true;
    }
}

// CSI ? Psl {
void ExecuteDECSLE(KTerm* term) { // Select Locator Events
    if (!GET_SESSION(term)->conformance.features.locator) {
        KTerm_LogUnsupportedSequence(term, "DECSLE requires locator support");
        return;
    }

    // Ensure session-specific locator events are updated
    if (GET_SESSION(term)->param_count == 0) {
        // No parameters, defaults to 0
        GET_SESSION(term)->locator_events.report_on_request_only = true;
        GET_SESSION(term)->locator_events.report_button_down = false;
        GET_SESSION(term)->locator_events.report_button_up = false;
        return;
    }

    for (int i = 0; i < GET_SESSION(term)->param_count; i++) {
        switch (GET_SESSION(term)->escape_params[i]) {
            case 0:
                GET_SESSION(term)->locator_events.report_on_request_only = true;
                GET_SESSION(term)->locator_events.report_button_down = false;
                GET_SESSION(term)->locator_events.report_button_up = false;
                break;
            case 1:
                GET_SESSION(term)->locator_events.report_button_down = true;
                GET_SESSION(term)->locator_events.report_on_request_only = false;
                break;
            case 2:
                GET_SESSION(term)->locator_events.report_button_down = false;
                break;
            case 3:
                GET_SESSION(term)->locator_events.report_button_up = true;
                GET_SESSION(term)->locator_events.report_on_request_only = false;
                break;
            case 4:
                GET_SESSION(term)->locator_events.report_button_up = false;
                break;
            default:
                if (GET_SESSION(term)->options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown DECSLE parameter: %d", GET_SESSION(term)->escape_params[i]);
                    KTerm_LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    }
}

void ExecuteDECSASD(KTerm* term) {
    // CSI Ps $ }
    // Select Active Status Display
    // 0 = Main Display, 1 = Status Line
    int mode = KTerm_GetCSIParam(term, 0, 0);
    if (mode == 0 || mode == 1) {
        GET_SESSION(term)->active_display = mode;
    }
}

void ExecuteDECSSDT(KTerm* term) {
    // CSI Ps $ ~
    // Select Split Definition Type
    // 0 = No Split, 1 = Horizontal Split
    if (!GET_SESSION(term)->conformance.features.multi_session_mode) {
        KTerm_LogUnsupportedSequence(term, "DECSSDT requires multi-session support");
        return;
    }

    int mode = KTerm_GetCSIParam(term, 0, 0);
    if (mode == 0) {
        KTerm_SetSplitScreen(term, false, 0, 0, 0);
    } else if (mode == 1) {
        // Default split: Center, Session 0 Top, Session 1 Bottom
        // Future: Support parameterized split points
        KTerm_SetSplitScreen(term, true, term->height / 2, 0, 1);
    } else {
        if (GET_SESSION(term)->options.debug_sequences) {
            char msg[64];
            snprintf(msg, sizeof(msg), "DECSSDT mode %d not supported", mode);
            KTerm_LogUnsupportedSequence(term, msg);
        }
    }
}

// CSI Plc |
void ExecuteDECRQLP(KTerm* term) { // Request Locator Position
    if (!GET_SESSION(term)->conformance.features.locator) {
        KTerm_LogUnsupportedSequence(term, "DECRQLP requires locator support");
        return;
    }

    char response[64]; // Increased buffer size for longer response

    if (!GET_SESSION(term)->locator_enabled || GET_SESSION(term)->mouse.cursor_x < 1 || GET_SESSION(term)->mouse.cursor_y < 1) {
        // Locator not enabled or position unknown, respond with "no locator"
        snprintf(response, sizeof(response), "\x1B[0!|");
    } else {
        // Locator enabled and position is known, report position.
        // Format: CSI Pe;Pr;Pc;Pp!|
        // Pe = 1 (request response)
        // Pr = row
        // Pc = column
        // Pp = page (hardcoded to 1)
        int row = GET_SESSION(term)->mouse.cursor_y;
        int col = GET_SESSION(term)->mouse.cursor_x;

        if (term->split_screen_active && term->active_session == term->session_bottom) {
            row -= (term->split_row + 1);
        }

        int page = 1; // Page memory not implemented, so hardcode to 1.
        snprintf(response, sizeof(response), "\x1B[1;%d;%d;%d!|", row, col, page);
    }

    KTerm_QueueResponse(term, response);
}


// CSI Pt ; Pl ; Pb ; Pr $ x
void ExecuteDECERA(KTerm* term) { // Erase Rectangular Area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        KTerm_LogUnsupportedSequence(term, "DECERA requires rectangular operations support");
        return;
    }
    if (GET_SESSION(term)->param_count != 4) {
        KTerm_LogUnsupportedSequence(term, "Invalid parameters for DECERA");
        return;
    }
    int top = KTerm_GetCSIParam(term, 0, 1) - 1;
    int left = KTerm_GetCSIParam(term, 1, 1) - 1;
    int bottom = KTerm_GetCSIParam(term, 2, 1) - 1;
    int right = KTerm_GetCSIParam(term, 3, 1) - 1;

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= term->height) bottom = term->height - 1;
    if (right >= term->width) right = term->width - 1;
    if (top > bottom || left > right) return;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            KTerm_ClearCell(term, GetActiveScreenCell(GET_SESSION(term), y, x));
        }
        GET_SESSION(term)->row_dirty[y] = true;
    }
}


// CSI Ps ; Pt ; Pl ; Pb ; Pr $ {
void ExecuteDECSERA(KTerm* term) { // Selective Erase Rectangular Area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        KTerm_LogUnsupportedSequence(term, "DECSERA requires rectangular operations support");
        return;
    }
    if (GET_SESSION(term)->param_count < 4 || GET_SESSION(term)->param_count > 5) {
        KTerm_LogUnsupportedSequence(term, "Invalid parameters for DECSERA");
        return;
    }
    int erase_param, top, left, bottom, right;

    if (GET_SESSION(term)->param_count == 5) {
        erase_param = KTerm_GetCSIParam(term, 0, 0);
        top = KTerm_GetCSIParam(term, 1, 1) - 1;
        left = KTerm_GetCSIParam(term, 2, 1) - 1;
        bottom = KTerm_GetCSIParam(term, 3, 1) - 1;
        right = KTerm_GetCSIParam(term, 4, 1) - 1;
    } else { // param_count == 4
        erase_param = 0; // Default when Ps is omitted
        top = KTerm_GetCSIParam(term, 0, 1) - 1;
        left = KTerm_GetCSIParam(term, 1, 1) - 1;
        bottom = KTerm_GetCSIParam(term, 2, 1) - 1;
        right = KTerm_GetCSIParam(term, 3, 1) - 1;
    }

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= term->height) bottom = term->height - 1;
    if (right >= term->width) right = term->width - 1;
    if (top > bottom || left > right) return;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            bool should_erase = false;
            EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
            switch (erase_param) {
                case 0: if (!(cell->flags & KTERM_ATTR_PROTECTED)) should_erase = true; break;
                case 1: should_erase = true; break;
                case 2: if (cell->flags & KTERM_ATTR_PROTECTED) should_erase = true; break;
            }
            if (should_erase) {
                KTerm_ClearCell(term, cell);
            }
        }
        GET_SESSION(term)->row_dirty[y] = true;
    }
}

void KTerm_ProcessOSCChar(KTerm* term, KTermSession* session, unsigned char ch) {
    // Phase 7.2: Harden Escape Buffers (Bounds Check)
    if ((size_t)session->escape_pos < sizeof(session->escape_buffer) - 1) {
        if (ch == '\x1B') {
            session->saved_parse_state = PARSE_OSC;
            session->parse_state = PARSE_STRING_TERMINATOR;
            return;
        }

        session->escape_buffer[session->escape_pos++] = ch;

        if (ch == '\a') {
            session->escape_buffer[session->escape_pos - 1] = '\0';
            KTerm_ExecuteOSCCommand(term, session);
            session->parse_state = VT_PARSE_NORMAL;
            session->escape_pos = 0;
        }
    } else {
        session->escape_buffer[sizeof(session->escape_buffer) - 1] = '\0';
        KTerm_ExecuteOSCCommand(term, session);
        session->parse_state = VT_PARSE_NORMAL;
        session->escape_pos = 0;
        KTerm_LogUnsupportedSequence(term, "OSC sequence too long, truncated");
    }
}

void KTerm_ProcessDCSChar(KTerm* term, KTermSession* session, unsigned char ch) {
    // Phase 7.2: Harden Escape Buffers (Bounds Check)
    if ((size_t)session->escape_pos < sizeof(session->escape_buffer) - 1) {
        if (ch == '\x1B') {
            session->saved_parse_state = PARSE_DCS;
            session->parse_state = PARSE_STRING_TERMINATOR;
            return;
        }

        session->escape_buffer[session->escape_pos++] = ch;

        // Ensure this is not DECRQSS ($q)
        bool is_decrqss = (session->escape_pos >= 2 && session->escape_buffer[session->escape_pos - 2] == '$');

        if (ch == 'q' && session->conformance.features.sixel_graphics && !is_decrqss) {
            // Sixel Graphics command
            KTerm_ParseCSIParams(term, session->escape_buffer, session->sixel.params, MAX_ESCAPE_PARAMS);
            session->sixel.param_count = session->param_count;

            session->sixel.pos_x = 0;
            session->sixel.pos_y = 0;
            session->sixel.max_x = 0;
            session->sixel.max_y = 0;
            session->sixel.color_index = 0;
            session->sixel.repeat_count = 0;

            // Parse P2 for background transparency (0=Device Default, 1=Transparent, 2=Opaque)
            int p2 = (session->param_count >= 2) ? session->sixel.params[1] : 0;
            session->sixel.transparent_bg = (p2 == 1);

            if (!session->sixel.data) {
                session->sixel.width = term->width * term->char_width;
                session->sixel.height = term->height * term->char_height;
                session->sixel.data = calloc(session->sixel.width * session->sixel.height * 4, 1);
            }

            if (session->sixel.data) {
                memset(session->sixel.data, 0, session->sixel.width * session->sixel.height * 4);
            }

            if (!session->sixel.strips) {
                session->sixel.strip_capacity = 65536;
                session->sixel.strips = (GPUSixelStrip*)calloc(session->sixel.strip_capacity, sizeof(GPUSixelStrip));
            }
            session->sixel.strip_count = 0;

            session->sixel.active = true;
            session->sixel.scrolling = true; // Default Sixel behavior scrolls
            session->sixel.logical_start_row = session->screen_head;
            session->sixel.x = session->cursor.x * term->char_width;
            session->sixel.y = session->cursor.y * term->char_height;

            session->parse_state = PARSE_SIXEL;
            session->escape_pos = 0;
            return;
        }

        if (ch == 'p' && session->conformance.features.regis_graphics) {
            // ReGIS (Remote Graphics Instruction Set)
            // Initialize ReGIS state
            term->regis.state = 0; // Expecting command
            term->regis.command = 0;
            term->regis.x = 0;
            term->regis.y = 0;
            term->regis.color = 0xFFFFFFFF; // White
            term->regis.write_mode = 0; // Default to Overlay/Additive
            term->regis.param_count = 0;
            term->regis.has_comma = false;
            term->regis.has_bracket = false;

            session->parse_state = PARSE_REGIS;
            session->escape_pos = 0;
            return;
        }

        if (ch == '\a') { // Non-standard, but some terminals accept BEL for DCS
            session->escape_buffer[session->escape_pos - 1] = '\0';
            KTerm_ExecuteDCSCommand(term, session);
            session->parse_state = VT_PARSE_NORMAL;
            session->escape_pos = 0;
        }
    } else { // Buffer overflow
        session->escape_buffer[sizeof(session->escape_buffer) - 1] = '\0';
        KTerm_ExecuteDCSCommand(term, session);
        session->parse_state = VT_PARSE_NORMAL;
        session->escape_pos = 0;
        KTerm_LogUnsupportedSequence(term, "DCS sequence too long, truncated");
    }
}

// =============================================================================
// ENHANCED FONT SYSTEM WITH UNICODE SUPPORT
// =============================================================================

void KTerm_CreateFontTexture(KTerm* term) {
    if (term->font_texture.generation != 0) {
        KTerm_DestroyTexture(&term->font_texture);
    }

    const int num_chars_base = 256;

    // Allocate persistent CPU buffer if not present
    if (!term->font_atlas_pixels) {
        term->font_atlas_pixels = calloc(term->atlas_width * term->atlas_height * 4, 1);
        if (!term->font_atlas_pixels) return;
        term->next_atlas_index = 256; // Start dynamic allocation after base set
    }
    // Clear buffer for new layout
    memset(term->font_atlas_pixels, 0, term->atlas_width * term->atlas_height * 4);

    unsigned char* pixels = term->font_atlas_pixels;

    int char_w = term->char_width;
    int char_h = term->char_height;
    if (GET_SESSION(term)->soft_font.active) {
        char_w = GET_SESSION(term)->soft_font.char_width;
        char_h = GET_SESSION(term)->soft_font.char_height;
    }
    int dynamic_chars_per_row = term->atlas_width / char_w;

    // Calculate Centering Offsets
    int pad_x = (char_w - term->font_data_width) / 2;
    int pad_y = (char_h - term->font_data_height) / 2;

    // Unpack the font data (Base 256 chars)
    for (int i = 0; i < num_chars_base; i++) {
        int glyph_col = i % dynamic_chars_per_row;
        int glyph_row = i / dynamic_chars_per_row;
        int dest_x_start = glyph_col * char_w;
        int dest_y_start = glyph_row * char_h;

        for (int y = 0; y < char_h; y++) {
            uint16_t row_data = 0;
            bool in_glyph_y = (y >= pad_y && y < (pad_y + term->font_data_height));

            if (in_glyph_y) {
                int src_y = y - pad_y;
                if (GET_SESSION(term)->soft_font.active && GET_SESSION(term)->soft_font.loaded[i]) {
                    row_data = GET_SESSION(term)->soft_font.font_data[i][src_y]; // Soft fonts limited to 8-bit width for now
                } else if (term->current_font_data) {
                    if (term->current_font_is_16bit) {
                         row_data = ((const uint16_t*)term->current_font_data)[i * term->font_data_height + src_y];
                    } else {
                         row_data = ((const uint8_t*)term->current_font_data)[i * term->font_data_height + src_y];
                    }
                }
            }

            for (int x = 0; x < char_w; x++) {
                int px_idx = ((dest_y_start + y) * term->atlas_width + (dest_x_start + x)) * 4;
                bool pixel_on = false;

                bool in_glyph_x = (x >= pad_x && x < (pad_x + term->font_data_width));

                if (in_glyph_y && in_glyph_x) {
                    int src_x = x - pad_x;
                    // Shift: For 8-bit, 7-src_x. For W-bit, (W - 1 - src_x).
                    pixel_on = (row_data >> (term->font_data_width - 1 - src_x)) & 1;
                }

                if (pixel_on) {
                    pixels[px_idx + 0] = 255;
                    pixels[px_idx + 1] = 255;
                    pixels[px_idx + 2] = 255;
                    pixels[px_idx + 3] = 255;
                }
            }
        }
    }

    // Create GPU Texture
    KTermImage img = {0};
    img.width = term->atlas_width;
    img.height = term->atlas_height;
    img.channels = 4;
    img.data = pixels;

    if (term->font_texture.generation != 0) KTerm_DestroyTexture(&term->font_texture);
    KTerm_CreateTexture(img, false, &term->font_texture);
    // Don't unload image data as it points to persistent buffer
}

void KTerm_InitCompute(KTerm* term) {
    if (term->compute_initialized) return;

    // 1. Create SSBO
    size_t buffer_size = term->width * term->height * sizeof(GPUCell);
    KTerm_CreateBuffer(buffer_size, NULL, KTERM_BUFFER_USAGE_STORAGE_BUFFER | KTERM_BUFFER_USAGE_TRANSFER_DST, &term->terminal_buffer);

    // 2. Create Storage Image (Output)
    KTermImage empty_img = {0};
    // Use current dimensions
    int win_width = term->width * term->char_width * DEFAULT_WINDOW_SCALE;
    int win_height = term->height * term->char_height * DEFAULT_WINDOW_SCALE;

    KTerm_CreateImage(win_width, win_height, 4, &empty_img); // RGBA
    // We can init to black if we want, but compute will overwrite.
    KTerm_CreateTextureEx(empty_img, false, KTERM_TEXTURE_USAGE_SAMPLED | KTERM_TEXTURE_USAGE_STORAGE | KTERM_TEXTURE_USAGE_TRANSFER_SRC, &term->output_texture);
    KTerm_UnloadImage(empty_img);

    // 3. Create Compute Pipeline
    {
        unsigned char* shader_body = NULL;
        unsigned int bytes_read = 0;
        if (KTerm_LoadFileData(KTERM_TERMINAL_SHADER_PATH, &bytes_read, &shader_body) == KTERM_SUCCESS && shader_body) {
            size_t l1 = strlen(terminal_compute_preamble);
            char* src = (char*)malloc(l1 + bytes_read + 1);
            if (src) {
                strcpy(src, terminal_compute_preamble);
                memcpy(src + l1, shader_body, bytes_read);
                src[l1 + bytes_read] = '\0';
                KTerm_CreateComputePipeline(src, KTERM_COMPUTE_LAYOUT_TERMINAL, &term->compute_pipeline);
                free(src);
            }
            free(shader_body);
        } else {
             if (term->sessions[0].options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Failed to load terminal shader");
        }
    }

    // Create Dummy Sixel Texture (1x1 transparent)
    KTermImage dummy_img = {0};
    if (KTerm_CreateImage(1, 1, 4, &dummy_img) == KTERM_SUCCESS) {
        memset(dummy_img.data, 0, 4); // Clear to transparent
        KTerm_CreateTextureEx(dummy_img, false, KTERM_TEXTURE_USAGE_SAMPLED, &term->dummy_sixel_texture);
        KTerm_UnloadImage(dummy_img);
    }

    // Create Clear Texture (1x1 opaque black)
    KTermImage clear_img = {0};
    if (KTerm_CreateImage(1, 1, 4, &clear_img) == KTERM_SUCCESS) {
        clear_img.data[0] = 0; clear_img.data[1] = 0; clear_img.data[2] = 0; clear_img.data[3] = 255;
        KTerm_CreateTextureEx(clear_img, false, KTERM_TEXTURE_USAGE_SAMPLED, &term->clear_texture);
        KTerm_UnloadImage(clear_img);
    }

    term->gpu_staging_buffer = (GPUCell*)calloc(term->width * term->height, sizeof(GPUCell));
    term->row_scratch_buffer = (EnhancedTermChar*)calloc(term->width, sizeof(EnhancedTermChar));

    // 4. Init Vector Engine (Storage Tube Architecture)
    term->vector_capacity = 65536; // Max new lines per frame
    KTerm_CreateBuffer(term->vector_capacity * sizeof(GPUVectorLine), NULL, KTERM_BUFFER_USAGE_STORAGE_BUFFER | KTERM_BUFFER_USAGE_TRANSFER_DST, &term->vector_buffer);
    term->vector_staging_buffer = (GPUVectorLine*)calloc(term->vector_capacity, sizeof(GPUVectorLine));

    // Create Persistent Vector Layer Texture (Storage Tube Surface)
    KTermImage vec_img = {0};
    KTerm_CreateImage(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 4, &vec_img);
    memset(vec_img.data, 0, DEFAULT_WINDOW_WIDTH * DEFAULT_WINDOW_HEIGHT * 4); // Clear to transparent black
    KTerm_CreateTextureEx(vec_img, false, KTERM_TEXTURE_USAGE_SAMPLED | KTERM_TEXTURE_USAGE_STORAGE | KTERM_TEXTURE_USAGE_TRANSFER_DST, &term->vector_layer_texture);
    KTerm_UnloadImage(vec_img);

    // Create Vector Pipeline
    {
        unsigned char* shader_body = NULL;
        unsigned int bytes_read = 0;
        if (KTerm_LoadFileData(KTERM_VECTOR_SHADER_PATH, &bytes_read, &shader_body) == KTERM_SUCCESS && shader_body) {
            size_t l1 = strlen(vector_compute_preamble);
            char* src = (char*)malloc(l1 + bytes_read + 1);
            if (src) {
                strcpy(src, vector_compute_preamble);
                memcpy(src + l1, shader_body, bytes_read);
                src[l1 + bytes_read] = '\0';
                KTerm_CreateComputePipeline(src, KTERM_COMPUTE_LAYOUT_VECTOR, &term->vector_pipeline);
                free(src);
            }
            free(shader_body);
        } else {
             if (term->sessions[0].options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Failed to load vector shader");
        }
    }

    // 5. Init Sixel Engine
    KTerm_CreateBuffer(65536 * sizeof(GPUSixelStrip), NULL, KTERM_BUFFER_USAGE_STORAGE_BUFFER | KTERM_BUFFER_USAGE_TRANSFER_DST, &term->sixel_buffer);
    KTerm_CreateBuffer(256 * sizeof(uint32_t), NULL, KTERM_BUFFER_USAGE_STORAGE_BUFFER | KTERM_BUFFER_USAGE_TRANSFER_DST, &term->sixel_palette_buffer);
    {
        unsigned char* shader_body = NULL;
        unsigned int bytes_read = 0;
        if (KTerm_LoadFileData(KTERM_SIXEL_SHADER_PATH, &bytes_read, &shader_body) == KTERM_SUCCESS && shader_body) {
            size_t l1 = strlen(sixel_compute_preamble);
            char* src = (char*)malloc(l1 + bytes_read + 1);
            if (src) {
                strcpy(src, sixel_compute_preamble);
                memcpy(src + l1, shader_body, bytes_read);
                src[l1 + bytes_read] = '\0';
                KTerm_CreateComputePipeline(src, KTERM_COMPUTE_LAYOUT_SIXEL, &term->sixel_pipeline);
                free(src);
            }
            free(shader_body);
        } else {
             if (term->sessions[0].options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Failed to load sixel shader");
        }
    }

    // 6. Init Texture Blit Pipeline (Kitty)
    {
        unsigned char* shader_body = NULL;
        unsigned int bytes_read = 0;
        if (KTerm_LoadFileData("shaders/texture_blit.comp", &bytes_read, &shader_body) == KTERM_SUCCESS && shader_body) {
            size_t l1 = strlen(blit_compute_preamble);
            char* src = (char*)malloc(l1 + bytes_read + 1);
            if (src) {
                strcpy(src, blit_compute_preamble);
                memcpy(src + l1, shader_body, bytes_read);
                src[l1 + bytes_read] = '\0';
                // Using TERMINAL layout since it roughly matches (Image at Binding 1 + Bindless)
                KTerm_CreateComputePipeline(src, KTERM_COMPUTE_LAYOUT_TERMINAL, &term->texture_blit_pipeline);
                free(src);
            }
            free(shader_body);
        }
    }

    term->compute_initialized = true;
}

// =============================================================================
// CHARACTER SET TRANSLATION SYSTEM
// =============================================================================

unsigned int TranslateCharacter(KTerm* term, unsigned char ch, CharsetState* state) {
    CharacterSet active_set;

    // 1. Determine Active Set
    if (state->single_shift_2) {
        active_set = state->g2;
        state->single_shift_2 = false;
    } else if (state->single_shift_3) {
        active_set = state->g3;
        state->single_shift_3 = false;
    } else {
        // GL (0x00-0x7F) vs GR (0x80-0xFF)
        if (ch < 0x80) {
            active_set = *state->gl;
        } else {
            active_set = *state->gr;
        }
    }

    // 2. UTF-8 Bypass
    if (active_set == CHARSET_UTF8) {
        return ch;
    }

    // 3. Translation
    if (ch >= 0x80) {
        // High-bit characters
        if (active_set == CHARSET_ISO_LATIN_1 || active_set == CHARSET_DEC_MULTINATIONAL) {
            return ch; // Pass-through for 8-bit sets
        }
        // For 7-bit sets mapped to GR, strip high bit for lookup
        // e.g. DEC Special Graphics in GR (rare, but valid in ISO 2022)
        unsigned char seven_bit = ch & 0x7F;
        if (active_set < CHARSET_COUNT) {
            return term->charset_lut[active_set][seven_bit];
        }
        return ch;
    } else {
        // Low-bit characters (GL)
        if (active_set < CHARSET_COUNT) {
            return term->charset_lut[active_set][ch];
        }
    }

    return ch;
}

// Render a glyph from TTF or fallback
static void RenderGlyphToAtlas(KTerm* term, uint32_t codepoint, uint32_t idx) {
    int col = idx % term->atlas_cols;
    int row = idx / term->atlas_cols;
    int x_start = col * DEFAULT_CHAR_WIDTH;
    int y_start = row * DEFAULT_CHAR_HEIGHT;
    bool rendered = false;

    if (term->ttf.loaded) {
        // TTF Rendering
        int w, h, xoff, yoff;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&term->ttf.info, term->ttf.scale, term->ttf.scale, codepoint, &w, &h, &xoff, &yoff);

        if (bitmap) {
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    int px = x + (DEFAULT_CHAR_WIDTH - w)/2; // Simple center X
                    int py = y + term->ttf.baseline + yoff; // yoff is negative (distance from baseline up)

                    if (px >= 0 && px < DEFAULT_CHAR_WIDTH && py >= 0 && py < DEFAULT_CHAR_HEIGHT) {
                        int val = bitmap[y * w + x];
                        int px_idx = ((y_start + py) * term->atlas_width + (x_start + px)) * 4;
                        term->font_atlas_pixels[px_idx+0] = 255;
                        term->font_atlas_pixels[px_idx+1] = 255;
                        term->font_atlas_pixels[px_idx+2] = 255;
                        term->font_atlas_pixels[px_idx+3] = val; // Use alpha
                    }
                }
            }
            stbtt_FreeBitmap(bitmap, NULL);
            rendered = true;
        }
    }

    if (!rendered) {
        // Fallback
        if (codepoint == 0xFFFD) {
            // Draw Replacement Character (Diamond with ?)
            for (int y = 0; y < DEFAULT_CHAR_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_CHAR_WIDTH; x++) {
                    bool on = false;
                    // Diamond shape: abs(x - center_x) + abs(y - center_y) <= size
                    int cx = DEFAULT_CHAR_WIDTH / 2;
                    int cy = DEFAULT_CHAR_HEIGHT / 2;
                    if (abs(x - cx) + abs(y - cy) <= 3) on = true;
                    // Hollow it out
                    if (abs(x - cx) + abs(y - cy) < 2) on = false;
                    // Add question mark dot
                    if (x == cx && y == cy + 1) on = true;
                    // Add question mark curve (simple)
                    if (y == cy - 1 && x == cx) on = true;
                    if (y == cy - 2 && (x == cx || x == cx + 1)) on = true;

                    int px_idx = ((y_start + y) * term->atlas_width + (x_start + x)) * 4;
                    unsigned char val = on ? 255 : 0;
                    term->font_atlas_pixels[px_idx+0] = val;
                    term->font_atlas_pixels[px_idx+1] = val;
                    term->font_atlas_pixels[px_idx+2] = val;
                    term->font_atlas_pixels[px_idx+3] = val;
                }
            }
        } else {
            // Draw Hex Box (Fallback)
            for (int y = 0; y < DEFAULT_CHAR_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_CHAR_WIDTH; x++) {
                    bool on = false;
                    if (x == 0 || x == DEFAULT_CHAR_WIDTH-1 || y == 0 || y == DEFAULT_CHAR_HEIGHT-1) on = true;
                    if (x == DEFAULT_CHAR_WIDTH/2 && y == DEFAULT_CHAR_HEIGHT/2) on = true; // Dot

                    int px_idx = ((y_start + y) * term->atlas_width + (x_start + x)) * 4;
                    unsigned char val = on ? 255 : 0;
                    term->font_atlas_pixels[px_idx+0] = val;
                    term->font_atlas_pixels[px_idx+1] = val;
                    term->font_atlas_pixels[px_idx+2] = val;
                    term->font_atlas_pixels[px_idx+3] = val;
                }
            }
        }
    }
}

void KTerm_LoadFont(KTerm* term, const char* filepath) {
    unsigned int size;
    unsigned char* buffer = NULL;
    if (KTerm_LoadFileData(filepath, &size, &buffer) != KTERM_SUCCESS || !buffer) {
        if (term->response_callback) term->response_callback(term, "Font load failed\r\n", 18);
        return;
    }

    if (term->ttf.file_buffer) KTERM_FREE(term->ttf.file_buffer);
    term->ttf.file_buffer = buffer;

    if (!stbtt_InitFont(&term->ttf.info, buffer, 0)) {
        if (term->response_callback) term->response_callback(term, "Font init failed\r\n", 18);
        return;
    }

    term->ttf.scale = stbtt_ScaleForPixelHeight(&term->ttf.info, (float)DEFAULT_CHAR_HEIGHT * 0.8f); // 80% height to leave room
    stbtt_GetFontVMetrics(&term->ttf.info, &term->ttf.ascent, &term->ttf.descent, &term->ttf.line_gap);

    // Calculate baseline
    int pixel_height = (int)((term->ttf.ascent - term->ttf.descent) * term->ttf.scale);
    int y_adjust = (DEFAULT_CHAR_HEIGHT - pixel_height) / 2;
    term->ttf.baseline = (int)(term->ttf.ascent * term->ttf.scale) + y_adjust;

    term->ttf.loaded = true;
}

// Helper to allocate a glyph index in the dynamic atlas for any Unicode codepoint
uint32_t KTerm_AllocateGlyph(KTerm* term, uint32_t codepoint) {
    // Limit to Unicode range
    if (codepoint >= 0x110000) {
        return '?'; // Return safe fallback
    }

    // Check if already mapped
    if (term->glyph_map && term->glyph_map[codepoint] != 0) {
        return term->glyph_map[codepoint];
    }

    // Safety check if glyph_map wasn't allocated
    if (!term->glyph_map) return '?';

    // Check capacity
    uint32_t capacity = (term->atlas_width / DEFAULT_CHAR_WIDTH) * (term->atlas_height / DEFAULT_CHAR_HEIGHT);
    if (term->next_atlas_index >= capacity) {
        // Atlas full. Use Clock Eviction Algorithm.
        // Iterate cyclically starting from atlas_clock_hand.
        // We look for an entry that wasn't used THIS frame.
        // If all were used this frame, we just pick the one at the hand (FIFO fallback).
        
        uint32_t lru_index = 0;
        uint32_t start_hand = term->atlas_clock_hand;
        if (start_hand < 256) start_hand = 256; // Ensure range

        while (true) {
             // Check if used this frame
             if (term->glyph_last_used[term->atlas_clock_hand] != term->frame_count) {
                 lru_index = term->atlas_clock_hand;
                 break;
             }
             
             // Move hand
             term->atlas_clock_hand++;
             if (term->atlas_clock_hand >= capacity) term->atlas_clock_hand = 256;
             
             // Full circle?
             if (term->atlas_clock_hand == start_hand) {
                 // All used this frame! Force eviction at current hand.
                 lru_index = start_hand;
                 break;
             }
        }
        
        // Move hand forward for next time
        term->atlas_clock_hand = lru_index + 1;
        if (term->atlas_clock_hand >= capacity) term->atlas_clock_hand = 256;

        // Evict
        if (lru_index >= 256) {
            uint32_t old_codepoint = term->atlas_to_codepoint[lru_index];
            if (old_codepoint < 65536) { // Safety check for map size
                term->glyph_map[old_codepoint] = 0; // Clear from map
            } else if (old_codepoint < 0x110000) {
                 // Handle full unicode range if map supports it (it does: 0x110000)
                 term->glyph_map[old_codepoint] = 0;
            }

            // Reuse this index
            term->glyph_map[codepoint] = (uint16_t)lru_index;
            term->atlas_to_codepoint[lru_index] = codepoint;
            term->glyph_last_used[lru_index] = term->frame_count; // Touch

            RenderGlyphToAtlas(term, codepoint, lru_index);
            term->font_atlas_dirty = true;
            return lru_index;
        } else {
            return '?'; // Should not happen if capacity > 256
        }
    }

    uint32_t idx = term->next_atlas_index++;
    term->glyph_map[codepoint] = (uint16_t)idx;
    term->atlas_to_codepoint[idx] = codepoint;
    term->glyph_last_used[idx] = term->frame_count;

    RenderGlyphToAtlas(term, codepoint, idx);

    term->font_atlas_dirty = true;
    return idx;
}

// Helper to map Unicode codepoints to Dynamic Atlas indices
uint32_t MapUnicodeToAtlas(KTerm* term, uint32_t codepoint) {
    // KTerm_AllocateGlyph checks glyph_map, which is now pre-populated
    // with the CP437 base mapping in KTerm_Init.
    // This ensures we find the static glyphs (0-255) for characters like '£' (0xA3 Unicode -> 0x9C CP437).
    return KTerm_AllocateGlyph(term, codepoint);
}

// Legacy wrapper (deprecated in favor of dynamic system, but kept for logic compat)
uint8_t MapUnicodeToCP437(uint32_t codepoint) {
    // This function returns a CP437 index (0-255).
    // It logic is hardcoded.
    if (codepoint < 128) return (uint8_t)codepoint;

    // Direct mappings for common box drawing and symbols present in CP437
    switch (codepoint) {
        case 0xFFFD: return '?'; // Replacement character
        case 0x00C7: return 128; // Ç
        case 0x00FC: return 129; // ü
        case 0x00E9: return 130; // é
        case 0x00E2: return 131; // â
        case 0x00E4: return 132; // ä
        case 0x00E0: return 133; // à
        case 0x00E5: return 134; // å
        case 0x00E7: return 135; // ç
        case 0x00EA: return 136; // ê
        case 0x00EB: return 137; // ë
        case 0x00E8: return 138; // è
        case 0x00EF: return 139; // ï
        case 0x00EE: return 140; // î
        case 0x00EC: return 141; // ì
        case 0x00C4: return 142; // Ä
        case 0x00C5: return 143; // Å
        case 0x00C9: return 144; // É
        case 0x00E6: return 145; // æ
        case 0x00C6: return 146; // Æ
        case 0x00F4: return 147; // ô
        case 0x00F6: return 148; // ö
        case 0x00F2: return 149; // ò
        case 0x00FB: return 150; // û
        case 0x00F9: return 151; // ù
        case 0x00FF: return 152; // ÿ
        case 0x00D6: return 153; // Ö
        case 0x00DC: return 154; // Ü
        case 0x00A2: return 155; // ¢
        case 0x00A3: return 156; // £
        case 0x00A5: return 157; // ¥
        case 0x20A7: return 158; // ₧
        case 0x0192: return 159; // ƒ
        case 0x00E1: return 160; // á
        case 0x00ED: return 161; // í
        case 0x00F3: return 162; // ó
        case 0x00FA: return 163; // ú
        case 0x00F1: return 164; // ñ
        case 0x00D1: return 165; // Ñ
        case 0x00AA: return 166; // ª
        case 0x00BA: return 167; // º
        case 0x00BF: return 168; // ¿
        case 0x2310: return 169; // ⌐
        case 0x00AC: return 170; // ¬
        case 0x00BD: return 171; // ½
        case 0x00BC: return 172; // ¼
        case 0x00A1: return 173; // ¡
        case 0x00AB: return 174; // «
        case 0x00BB: return 175; // »
        case 0x2591: return 176; // ░
        case 0x2592: return 177; // ▒
        case 0x2593: return 178; // ▓
        case 0x2502: return 179; // │
        case 0x2524: return 180; // ┤
        case 0x2561: return 181; // ╡
        case 0x2562: return 182; // ╢
        case 0x2556: return 183; // ╖
        case 0x2555: return 184; // ╕
        case 0x2563: return 185; // ╣
        case 0x2551: return 186; // ║
        case 0x2557: return 187; // ╗
        case 0x255D: return 188; // ╝
        case 0x255C: return 189; // ╜
        case 0x255B: return 190; // ╛
        case 0x2510: return 191; // ┐
        case 0x2514: return 192; // └
        case 0x2534: return 193; // ┴
        case 0x252C: return 194; // ┬
        case 0x251C: return 195; // ├
        case 0x2500: return 196; // ─
        case 0x253C: return 197; // ┼
        case 0x255E: return 198; // ╞
        case 0x255F: return 199; // ╟
        case 0x255A: return 200; // ╚
        case 0x2554: return 201; // ╔
        case 0x2569: return 202; // ╩
        case 0x2566: return 203; // ╦
        case 0x2560: return 204; // ╠
        case 0x2550: return 205; // ═
        case 0x256C: return 206; // ╬
        case 0x2567: return 207; // ╧
        case 0x2568: return 208; // ╨
        case 0x2564: return 209; // ╤
        case 0x2565: return 210; // ╥
        case 0x2559: return 211; // ╙
        case 0x2558: return 212; // ╘
        case 0x2552: return 213; // ╒
        case 0x2553: return 214; // ╓
        case 0x256B: return 215; // ╫
        case 0x256A: return 216; // ╪
        case 0x2518: return 217; // ┘
        case 0x250C: return 218; // ┌
        case 0x2588: return 219; // █
        case 0x2584: return 220; // ▄
        case 0x258C: return 221; // ▌
        case 0x2590: return 222; // ▐
        case 0x2580: return 223; // ▀
        case 0x03B1: return 224; // α
        case 0x00DF: return 225; // ß
        case 0x0393: return 226; // Γ
        case 0x03C0: return 227; // π
        case 0x03A3: return 228; // Σ
        case 0x03C3: return 229; // σ
        case 0x00B5: return 230; // µ
        case 0x03C4: return 231; // τ
        case 0x03A6: return 232; // Φ
        case 0x0398: return 233; // Θ
        case 0x03A9: return 234; // Ω
        case 0x03B4: return 235; // δ
        case 0x221E: return 236; // ∞
        case 0x03C6: return 237; // φ
        case 0x03B5: return 238; // ε
        case 0x2229: return 239; // ∩
        case 0x2261: return 240; // ≡
        case 0x00B1: return 241; // ±
        case 0x2265: return 242; // ≥
        case 0x2264: return 243; // ≤
        case 0x2320: return 244; // ⌠
        case 0x2321: return 245; // ⌡
        case 0x00F7: return 246; // ÷
        case 0x2248: return 247; // ≈
        case 0x00B0: return 248; // °
        case 0x2219: return 249; // ∙
        case 0x00B7: return 250; // ·
        case 0x221A: return 251; // √
        case 0x207F: return 252; // ⁿ
        case 0x00B2: return 253; // ²
        case 0x25A0: return 254; // ■
        case 0x00A0: return 255; // NBSP
        default: return '?';     // Fallback
    }
}

// VT100/220 "Special Graphics" to CP437 Lookup Table.
static const uint8_t vt_special_graphics_lut[32] = {
    0x20, // 0x5F (Blank)       -> Space
    0x04, // 0x60 (Diamond)     -> CP437 Diamond
    0xB1, // 0x61 (Checker)     -> CP437 Medium Shade
    0x09, // 0x62 (HT)          -> CP437 Tab/Circle
    0x0C, // 0x63 (FF)          -> CP437 Female (closest approx)
    0x0D, // 0x64 (CR)          -> CP437 Note
    0x0A, // 0x65 (LF)          -> CP437 Inv Circle
    0xF8, // 0x66 (Degree)      -> CP437 Degree
    0xF1, // 0x67 (Plus-Minus)  -> CP437 Plus-Minus
    0x0A, // 0x68 (NL)          -> (Approx)
    0x0B, // 0x69 (VT)          -> CP437 Male
    0xD9, // 0x6A (┘)           -> CP437 Bottom Right
    0xBF, // 0x6B (┐)           -> CP437 Top Right
    0xDA, // 0x6C (┌)           -> CP437 Top Left
    0xC0, // 0x6D (└)           -> CP437 Bottom Left
    0xC5, // 0x6E (┼)           -> CP437 Cross
    0xC4, // 0x6F (Scan 1)      -> CP437 Horiz Line (Approx)
    0xC4, // 0x70 (Scan 3)      -> CP437 Horiz Line (Approx)
    0xC4, // 0x71 (─ Scan 5)    -> CP437 Horiz Line (Exact)
    0xC4, // 0x72 (Scan 7)      -> CP437 Horiz Line (Approx)
    0xC4, // 0x73 (Scan 9)      -> CP437 Horiz Line (Approx)
    0xC3, // 0x74 (├)           -> CP437 Left T
    0xB4, // 0x75 (┤)           -> CP437 Right T
    0xC1, // 0x76 (┴)           -> CP437 Bottom T
    0xC2, // 0x77 (┬)           -> CP437 Top T
    0xB3, // 0x78 (│)           -> CP437 Vert Bar
    0xF3, // 0x79 (≤)           -> CP437 Less/Equal
    0xF2, // 0x7A (≥)           -> CP437 Greater/Equal
    0xE3, // 0x7B (π)           -> CP437 Pi
    0x9C, // 0x7C (≠)           -> CP437 Pound (closest/conflict) or 0x3D
    0x9C, // 0x7D (£)           -> CP437 Pound
    0xFA  // 0x7E (·)           -> CP437 Dot
};

unsigned int KTerm_TranslateDECSpecial(KTerm* term, unsigned char ch) {
    (void)term;
    if (ch >= 0x5F && ch <= 0x7E) {
        return vt_special_graphics_lut[ch - 0x5F];
    }
    return ch;
}

unsigned int KTerm_TranslateDECMultinational(KTerm* term, unsigned char ch) {
    (void)term;
    // DEC Multinational Character Set (partial implementation)
    if (ch >= 0x80) {
        // High bit characters map to Latin-1 supplement
        return 0x0080 + (ch - 0x80);
    }
    return ch;
}

// =============================================================================
// TAB STOP MANAGEMENT
// =============================================================================

static void KTerm_SetTabStop_Internal(KTermSession* session, int column) {
    if (column >= 0 && column < session->tab_stops.capacity && column < session->cols) {
        if (!session->tab_stops.stops[column]) {
            session->tab_stops.stops[column] = true;
            session->tab_stops.count++;
        }
    }
}

void KTerm_SetTabStop(KTerm* term, int column) {
    KTerm_SetTabStop_Internal(GET_SESSION(term), column);
}

void KTerm_ClearTabStop(KTerm* term, int column) {
    if (column >= 0 && column < GET_SESSION(term)->tab_stops.capacity) {
        if (GET_SESSION(term)->tab_stops.stops[column]) {
            GET_SESSION(term)->tab_stops.stops[column] = false;
            GET_SESSION(term)->tab_stops.count--;
        }
    }
}

void KTerm_ClearAllTabStops(KTerm* term) {
    if (GET_SESSION(term)->tab_stops.stops) {
        memset(GET_SESSION(term)->tab_stops.stops, false, GET_SESSION(term)->tab_stops.capacity * sizeof(bool));
        GET_SESSION(term)->tab_stops.count = 0;
    }
}

static int NextTabStop_Internal(KTermSession* session, int current_column) {
    if (!session->tab_stops.stops) return (current_column + 1 < session->cols) ? current_column + 1 : session->cols - 1;

    for (int i = current_column + 1; i < session->tab_stops.capacity && i < session->cols; i++) {
        if (session->tab_stops.stops[i]) {
            return i;
        }
    }

    // If no explicit tab stop found, move to right margin/edge
    return session->cols - 1;
}

int NextTabStop(KTerm* term, int current_column) {
    return NextTabStop_Internal(GET_SESSION(term), current_column);
}

static int PreviousTabStop_Internal(KTermSession* session, int current_column) {
    if (!session->tab_stops.stops) return (current_column > 0) ? current_column - 1 : 0;

    for (int i = current_column - 1; i >= 0; i--) {
        if (i < session->tab_stops.capacity && session->tab_stops.stops[i]) {
            return i;
        }
    }

    // If no explicit tab stop found, use default spacing
    int prev = ((current_column - 1) / session->tab_stops.default_width) * session->tab_stops.default_width;
    return (prev >= 0) ? prev : 0;
}

int PreviousTabStop(KTerm* term, int current_column) {
    return PreviousTabStop_Internal(GET_SESSION(term), current_column);
}

// =============================================================================
// ENHANCED SCREEN MANIPULATION
// =============================================================================

static void KTerm_ClearCell_Internal(KTermSession* session, EnhancedTermChar* cell) {
    cell->ch = ' ';
    cell->fg_color = session->current_fg;
    cell->bg_color = session->current_bg;
    cell->flags = session->current_attributes | KTERM_FLAG_DIRTY;
}

void KTerm_ClearCell(KTerm* term, EnhancedTermChar* cell) {
    KTerm_ClearCell_Internal(GET_SESSION(term), cell);
}

static void KTerm_ScrollUpRegion_Internal(KTerm* term, KTermSession* session, int top, int bottom, int lines) {
    (void)term;
    // Check for full screen scroll (Top to Bottom, Full Width)
    // This allows optimization via Ring Buffer pointer arithmetic.
    if (top == 0 && bottom == term->height - 1 &&
        session->left_margin == 0 && session->right_margin == term->width - 1)
    {
        for (int i = 0; i < lines; i++) {
            // Increment head (scrolling down in memory, visually up)
            session->screen_head = (session->screen_head + 1) % session->buffer_height;

            // Adjust view_offset to keep historical view stable if user is looking back
            if (session->view_offset > 0) {
                 session->view_offset++;
                 // Cap at buffer limits
                 int max_offset = session->buffer_height - term->height;
                 if (session->view_offset > max_offset) session->view_offset = max_offset;
            }

            // Clear the new bottom line (logical row 'bottom')
            for (int x = 0; x < term->width; x++) {
                KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, bottom, x));
            }
        }
        // Invalidate all viewport rows because the data under them has shifted
        for (int y = 0; y < term->height; y++) {
            session->row_dirty[y] = true;
        }
        return;
    }

    // Partial Scroll (Fallback to copy) - Strictly NO head manipulation
    for (int i = 0; i < lines; i++) {
        // Move lines up within the region
        for (int y = top; y < bottom; y++) {
            for (int x = session->left_margin; x <= session->right_margin; x++) {
                *GetActiveScreenCell(session, y, x) = *GetActiveScreenCell(session, y + 1, x);
                GetActiveScreenCell(session, y, x)->flags |= KTERM_FLAG_DIRTY;
            }
            session->row_dirty[y] = true;
        }

        // Clear bottom line of the region
        for (int x = session->left_margin; x <= session->right_margin; x++) {
            KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, bottom, x));
        }
        session->row_dirty[bottom] = true;
    }
}

void KTerm_ScrollUpRegion(KTerm* term, int top, int bottom, int lines) {
    KTerm_ScrollUpRegion_Internal(term, GET_SESSION(term), top, bottom, lines);
}

static void KTerm_ScrollDownRegion_Internal(KTerm* term, KTermSession* session, int top, int bottom, int lines) {
    (void)term;
    for (int i = 0; i < lines; i++) {
        // Move lines down
        for (int y = bottom; y > top; y--) {
            for (int x = session->left_margin; x <= session->right_margin; x++) {
                *GetActiveScreenCell(session, y, x) = *GetActiveScreenCell(session, y - 1, x);
                GetActiveScreenCell(session, y, x)->flags |= KTERM_FLAG_DIRTY;
            }
            session->row_dirty[y] = true;
        }

        // Clear top line
        for (int x = session->left_margin; x <= session->right_margin; x++) {
            KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, top, x));
        }
        session->row_dirty[top] = true;
    }
}

void KTerm_ScrollDownRegion(KTerm* term, int top, int bottom, int lines) {
    KTerm_ScrollDownRegion_Internal(term, GET_SESSION(term), top, bottom, lines);
}

static void KTerm_InsertLinesAt_Internal(KTerm* term, KTermSession* session, int row, int count) {
    (void)term;
    if (row < session->scroll_top || row > session->scroll_bottom) {
        return;
    }

    // Move existing lines down
    for (int y = session->scroll_bottom; y >= row + count; y--) {
        if (y - count >= row) {
            for (int x = session->left_margin; x <= session->right_margin; x++) {
                *GetActiveScreenCell(session, y, x) = *GetActiveScreenCell(session, y - count, x);
                GetActiveScreenCell(session, y, x)->flags |= KTERM_FLAG_DIRTY;
            }
            session->row_dirty[y] = true;
        }
    }

    // Clear inserted lines
    for (int y = row; y < row + count && y <= session->scroll_bottom; y++) {
        for (int x = session->left_margin; x <= session->right_margin; x++) {
            KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, y, x));
        }
        session->row_dirty[y] = true;
    }
}

void KTerm_InsertLinesAt(KTerm* term, int row, int count) {
    KTerm_InsertLinesAt_Internal(term, GET_SESSION(term), row, count);
}

static void KTerm_DeleteLinesAt_Internal(KTerm* term, KTermSession* session, int row, int count) {
    (void)term;
    if (row < session->scroll_top || row > session->scroll_bottom) {
        return;
    }

    // Move remaining lines up
    for (int y = row; y <= session->scroll_bottom - count; y++) {
        for (int x = session->left_margin; x <= session->right_margin; x++) {
            *GetActiveScreenCell(session, y, x) = *GetActiveScreenCell(session, y + count, x);
            GetActiveScreenCell(session, y, x)->flags |= KTERM_FLAG_DIRTY;
        }
        session->row_dirty[y] = true;
    }

    // Clear bottom lines
    for (int y = session->scroll_bottom - count + 1; y <= session->scroll_bottom; y++) {
        if (y >= 0) {
            for (int x = session->left_margin; x <= session->right_margin; x++) {
                KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, y, x));
            }
            session->row_dirty[y] = true;
        }
    }
}

void KTerm_DeleteLinesAt(KTerm* term, int row, int count) {
    KTerm_DeleteLinesAt_Internal(term, GET_SESSION(term), row, count);
}

static void KTerm_InsertCharactersAt_Internal(KTerm* term, KTermSession* session, int row, int col, int count) {
    (void)term;
    // Shift existing characters right
    for (int x = session->right_margin; x >= col + count; x--) {
        if (x - count >= col) {
            *GetActiveScreenCell(session, row, x) = *GetActiveScreenCell(session, row, x - count);
            GetActiveScreenCell(session, row, x)->flags |= KTERM_FLAG_DIRTY;
        }
    }

    // Clear inserted positions
    for (int x = col; x < col + count && x <= session->right_margin; x++) {
        KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, row, x));
    }
    session->row_dirty[row] = true;
}

void KTerm_InsertCharactersAt(KTerm* term, int row, int col, int count) {
    KTerm_InsertCharactersAt_Internal(term, GET_SESSION(term), row, col, count);
}

static void KTerm_DeleteCharactersAt_Internal(KTerm* term, KTermSession* session, int row, int col, int count) {
    (void)term;
    // Shift remaining characters left
    for (int x = col; x <= session->right_margin - count; x++) {
        *GetActiveScreenCell(session, row, x) = *GetActiveScreenCell(session, row, x + count);
        GetActiveScreenCell(session, row, x)->flags |= KTERM_FLAG_DIRTY;
    }

    // Clear rightmost positions
    for (int x = session->right_margin - count + 1; x <= session->right_margin; x++) {
        if (x >= 0) {
            KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, row, x));
        }
    }
    session->row_dirty[row] = true;
}

void KTerm_DeleteCharactersAt(KTerm* term, int row, int col, int count) {
    KTerm_DeleteCharactersAt_Internal(term, GET_SESSION(term), row, col, count);
}

// =============================================================================
// VT100 INSERT MODE IMPLEMENTATION
// =============================================================================

void EnableInsertMode(KTerm* term, bool enable) {
    GET_SESSION(term)->dec_modes.insert_mode = enable;
}

// Internal version of InsertCharacterAtCursor taking session
static void KTerm_InsertCharacterAtCursor_Internal(KTerm* term, KTermSession* session, unsigned int ch) {
    if (session->dec_modes.insert_mode) {
        // Insert mode: shift existing characters right
        KTerm_InsertCharactersAt_Internal(term, session, session->cursor.y, session->cursor.x, 1);
    }

    // Place character at cursor position
    EnhancedTermChar* cell = GetActiveScreenCell(session, session->cursor.y, session->cursor.x);
    cell->ch = ch;
    cell->fg_color = session->current_fg;
    cell->bg_color = session->current_bg;
    cell->ul_color = session->current_ul_color;
    cell->st_color = session->current_st_color;

    // Apply current attributes (preserving line attributes)
    uint32_t line_attrs = cell->flags & (KTERM_ATTR_DOUBLE_WIDTH | KTERM_ATTR_DOUBLE_HEIGHT_TOP | KTERM_ATTR_DOUBLE_HEIGHT_BOT);
    cell->flags = session->current_attributes | line_attrs | KTERM_FLAG_DIRTY;
    session->row_dirty[session->cursor.y] = true;

    // Track last printed character for REP command
    session->last_char = ch;
}

void KTerm_InsertCharacterAtCursor(KTerm* term, unsigned int ch) {
    KTerm_InsertCharacterAtCursor_Internal(term, GET_SESSION(term), ch);
}

// =============================================================================
// COMPREHENSIVE CHARACTER PROCESSING
// =============================================================================

void KTerm_ProcessNormalChar(KTerm* term, KTermSession* session, unsigned char ch) {
    // Handle control characters first
    if (ch < 32) {
        KTerm_ProcessControlChar(term, session, ch);
        return;
    }

    // Translate character through active character set
    unsigned int unicode_ch = TranslateCharacter(term, ch, &session->charset);

    // Handle UTF-8 decoding if enabled
    if (*session->charset.gl == CHARSET_UTF8) {
        if (session->utf8.bytes_remaining == 0) {
            if (ch < 0x80) {
                // 1-byte sequence (ASCII)
                unicode_ch = ch;
            } else if ((ch & 0xE0) == 0xC0) {
                // 2-byte sequence
                if (ch < 0xC2) { // Overlong (C0, C1)
                    unicode_ch = 0xFFFD;
                    session->utf8.bytes_remaining = 0; // Explicit reset
                    // Fall through to display replacement
                } else {
                    session->utf8.codepoint = ch & 0x1F;
                    session->utf8.min_codepoint = 0x80;
                    session->utf8.bytes_remaining = 1;
                    return;
                }
            } else if ((ch & 0xF0) == 0xE0) {
                // 3-byte sequence
                session->utf8.codepoint = ch & 0x0F;
                session->utf8.min_codepoint = 0x800;
                session->utf8.bytes_remaining = 2;
                return;
            } else if ((ch & 0xF8) == 0xF0) {
                // 4-byte sequence
                if (ch > 0xF4) { // Restricted by RFC 3629
                    unicode_ch = 0xFFFD;
                    session->utf8.bytes_remaining = 0; // Explicit reset
                    // Fall through
                } else {
                    session->utf8.codepoint = ch & 0x07;
                    session->utf8.min_codepoint = 0x10000;
                    session->utf8.bytes_remaining = 3;
                    return;
                }
            } else {
                // Invalid start byte
                unicode_ch = 0xFFFD;
                session->utf8.bytes_remaining = 0; // Explicit reset
                // Fall through to display replacement and consume byte
            }
        } else {
            // Continuation byte
            if ((ch & 0xC0) == 0x80) {
                session->utf8.codepoint = (session->utf8.codepoint << 6) | (ch & 0x3F);
                session->utf8.bytes_remaining--;
                if (session->utf8.bytes_remaining > 0) {
                    return;
                }
                // Sequence complete
                uint32_t cp = session->utf8.codepoint;
                bool valid = true;

                // Check for overlong encodings
                if (cp < session->utf8.min_codepoint) valid = false;
                // Check for surrogates (D800-DFFF)
                if (cp >= 0xD800 && cp <= 0xDFFF) valid = false;
                // Check max value
                if (cp > 0x10FFFF) valid = false;

                if (valid) {
                    unicode_ch = cp;
                    // Attempt to map to CP437 for pixel-perfect box drawing, but preserve Unicode if not found
                    uint8_t cp437 = MapUnicodeToCP437(unicode_ch);
                    if (cp437 != '?' || unicode_ch == '?') {
                        unicode_ch = cp437;
                    }
                } else {
                    unicode_ch = 0xFFFD;
                }
                // bytes_remaining is already 0 here
            } else {
                // Invalid continuation byte
                KTerm_InsertCharacterAtCursor(term, 0xFFFD);
                session->cursor.x++;

                // Reset and try to recover
                session->utf8.bytes_remaining = 0;
                session->utf8.codepoint = 0;

                // Recursively process this char as new
                KTerm_ProcessNormalChar(term, session, ch);
                return;
            }
        }
    }

    // Handle character display
    if (session->cursor.x > session->right_margin) {
        if (session->dec_modes.auto_wrap_mode) {
            // Auto-wrap to next line
            session->cursor.x = session->left_margin;
            session->cursor.y++;

            if (session->cursor.y > session->scroll_bottom) {
                session->cursor.y = session->scroll_bottom;
                KTerm_ScrollUpRegion(term, session->scroll_top, session->scroll_bottom, 1);
            }
        } else {
            // No wrap - stay at right margin
            session->cursor.x = session->right_margin;
        }
    }

    // Insert character (handles insert mode internally)
    KTerm_InsertCharacterAtCursor(term, unicode_ch);

    // Advance cursor
    session->cursor.x++;
}

// Update KTerm_ProcessControlChar
void KTerm_ProcessControlChar(KTerm* term, KTermSession* session, unsigned char ch) {
    switch (ch) {
        case 0x05: // ENQ - Enquiry
            if (session->answerback_buffer[0] != '\0') {
                KTerm_QueueResponse(term, session->answerback_buffer);
            }
            break;
        case 0x07: // BEL - Bell
            if (term->bell_callback) {
                term->bell_callback(term);
            } else {
                // Visual bell
                session->visual_bell_timer = 0.2;
            }
            break;
        case 0x08: // BS - Backspace
            if (session->cursor.x > session->left_margin) {
                session->cursor.x--;
            }
            break;
        case 0x09: // HT - Horizontal Tab
            session->cursor.x = NextTabStop(term, session->cursor.x);
            if (session->cursor.x > session->right_margin) {
                session->cursor.x = session->right_margin;
            }
            break;
        case 0x0A: // LF - Line Feed
        case 0x0B: // VT - Vertical Tab
        case 0x0C: // FF - Form Feed
            session->cursor.y++;
            if (session->cursor.y > session->scroll_bottom) {
                session->cursor.y = session->scroll_bottom;
                KTerm_ScrollUpRegion(term, session->scroll_top, session->scroll_bottom, 1);
            }
            if (session->ansi_modes.line_feed_new_line) {
                session->cursor.x = session->left_margin;
            }
            break;
        case 0x0D: // CR - Carriage Return
            session->cursor.x = session->left_margin;
            break;
        case 0x0E: // SO - Shift Out (invoke G1 into GL)
            session->charset.gl = &session->charset.g1;
            break;
        case 0x0F: // SI - Shift In (invoke G0 into GL)
            session->charset.gl = &session->charset.g0;
            break;
        case 0x11: // XON - Resume transmission
            // Flow control - not implemented
            break;
        case 0x13: // XOFF - Stop transmission
            // Flow control - not implemented
            break;
        case 0x18: // CAN - Cancel
        case 0x1A: // SUB - Substitute
            // Cancel current escape sequence
            session->parse_state = VT_PARSE_NORMAL;
            session->escape_pos = 0;
            break;
        case 0x1B: // ESC - Escape
            if (session->dec_modes.vt52_mode) {
                session->parse_state = PARSE_VT52;
            } else {
                session->parse_state = VT_PARSE_ESCAPE;
            }
            session->escape_pos = 0;
            break;
        case 0x7F: // DEL - Delete
            // Ignored
            break;
        default:
            if (session->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown control char: 0x%02X", ch);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}


// =============================================================================
// ENHANCED ESCAPE SEQUENCE PROCESSING
// =============================================================================

void KTerm_ProcessEscapeChar(KTerm* term, KTermSession* session, unsigned char ch) {
    switch (ch) {
        // CSI - Control Sequence Introducer
        case '[':
            session->parse_state = PARSE_CSI;
            session->escape_pos = 0;
            memset(session->escape_params, 0, sizeof(session->escape_params));
            session->param_count = 0;
            break;

        // OSC - Operating System Command
        case ']':
            session->parse_state = PARSE_OSC;
            session->escape_pos = 0;
            break;

        // DCS - Device Control String
        case 'P':
            session->parse_state = PARSE_DCS;
            session->escape_pos = 0;
            break;

        // APC - Application Program Command
        case '_':
            session->parse_state = PARSE_APC;
            session->escape_pos = 0;
            break;

        // PM - Privacy Message
        case '^':
            session->parse_state = PARSE_PM;
            session->escape_pos = 0;
            break;

        // SOS - Start of String
        case 'X':
            session->parse_state = PARSE_SOS;
            session->escape_pos = 0;
            break;

        // Character set selection
        case '(':
            session->parse_state = PARSE_CHARSET;
            session->escape_buffer[0] = '(';
            session->escape_pos = 1;
            break;

        case ')':
            session->parse_state = PARSE_CHARSET;
            session->escape_buffer[0] = ')';
            session->escape_pos = 1;
            break;

        case '*':
            session->parse_state = PARSE_CHARSET;
            session->escape_buffer[0] = '*';
            session->escape_pos = 1;
            break;

        case '+':
            session->parse_state = PARSE_CHARSET;
            session->escape_buffer[0] = '+';
            session->escape_pos = 1;
            break;

        // Locking Shifts (ISO 2022)
        case 'n': // LS2 (GL = G2)
            session->charset.gl = &session->charset.g2;
            session->parse_state = VT_PARSE_NORMAL;
            break;
        case 'o': // LS3 (GL = G3)
            session->charset.gl = &session->charset.g3;
            session->parse_state = VT_PARSE_NORMAL;
            break;
        case '~': // LS1R (GR = G1)
            session->charset.gr = &session->charset.g1;
            session->parse_state = VT_PARSE_NORMAL;
            break;
        case '}': // LS2R (GR = G2)
            session->charset.gr = &session->charset.g2;
            session->parse_state = VT_PARSE_NORMAL;
            break;
        case '|': // LS3R (GR = G3)
            session->charset.gr = &session->charset.g3;
            session->parse_state = VT_PARSE_NORMAL;
            break;

        // Single character commands
        case '7': // DECSC - Save Cursor
            KTerm_ExecuteSaveCursor_Internal(session);
            session->parse_state = VT_PARSE_NORMAL;
            break;

        case '8': // DECRC - Restore Cursor
            KTerm_ExecuteRestoreCursor_Internal(session);
            session->parse_state = VT_PARSE_NORMAL;
            break;

        case '#': // DEC Line Attributes
            session->parse_state = PARSE_HASH;
            break;

        case '%': // Select Character Set
            session->parse_state = PARSE_PERCENT;
            break;

        case 'D': // IND - Index
            session->cursor.y++;
            if (session->cursor.y > session->scroll_bottom) {
                session->cursor.y = session->scroll_bottom;
                KTerm_ScrollUpRegion_Internal(term, session, session->scroll_top, session->scroll_bottom, 1);
            }
            session->parse_state = VT_PARSE_NORMAL;
            break;

        case 'E': // NEL - Next Line
            session->cursor.x = session->left_margin;
            session->cursor.y++;
            if (session->cursor.y > session->scroll_bottom) {
                session->cursor.y = session->scroll_bottom;
                KTerm_ScrollUpRegion_Internal(term, session, session->scroll_top, session->scroll_bottom, 1);
            }
            session->parse_state = VT_PARSE_NORMAL;
            break;

        case 'H': // HTS - Set Tab Stop
            KTerm_SetTabStop_Internal(session, session->cursor.x);
            session->parse_state = VT_PARSE_NORMAL;
            break;

        case 'M': // RI - Reverse Index
            session->cursor.y--;
            if (session->cursor.y < session->scroll_top) {
                session->cursor.y = session->scroll_top;
                KTerm_ScrollDownRegion_Internal(term, session, session->scroll_top, session->scroll_bottom, 1);
            }
            session->parse_state = VT_PARSE_NORMAL;
            break;

        // case 'n': // LS2 - Locking Shift 2 (Handled above as LS2)
        // case 'o': // LS3 - Locking Shift 3 (Handled above as LS3)

        case 'N': // SS2 - Single Shift 2
            session->charset.single_shift_2 = true;
            session->parse_state = VT_PARSE_NORMAL;
            break;

        case 'O': // SS3 - Single Shift 3
            session->charset.single_shift_3 = true;
            session->parse_state = VT_PARSE_NORMAL;
            break;

        case 'Z': // DECID - Identify Terminal
            KTerm_QueueResponse(term, session->device_attributes);
            session->parse_state = VT_PARSE_NORMAL;
            break;

        case 'c': // RIS - Reset to Initial State
            KTerm_Init(term);
            break;

        case '=': // DECKPAM - Keypad Application Mode
            GET_SESSION(term)->input.keypad_application_mode = true;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case '>': // DECKPNM - Keypad Numeric Mode
            GET_SESSION(term)->input.keypad_application_mode = false;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case '<': // Enter VT52 mode (if enabled)
            if (GET_SESSION(term)->conformance.features.vt52_mode) {
                GET_SESSION(term)->parse_state = PARSE_VT52;
            } else {
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                if (GET_SESSION(term)->options.log_unsupported) {
                    KTerm_LogUnsupportedSequence(term, "VT52 mode not supported");
                }
            }
            break;

        default:
            // Unknown escape sequence
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC %c (0x%02X)", ch, ch);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;
    }
}

// =============================================================================
// Internal helper for writing to a specific session without changing global state
static bool KTerm_WriteCharToSessionInternal(KTerm* term, KTermSession* session, unsigned char ch) {
    (void)term; // Currently unused, but kept for signature consistency

    // Load head relaxed (only this thread writes to it)
    int current_head = atomic_load_explicit(&session->pipeline_head, memory_order_relaxed);
    int next_head = (current_head + 1) % sizeof(session->input_pipeline);

    // Load tail acquire (another thread writes to it)
    int current_tail = atomic_load_explicit(&session->pipeline_tail, memory_order_acquire);

    if (next_head == current_tail) {
        atomic_store_explicit(&session->pipeline_overflow, true, memory_order_relaxed);
        return false;
    }

    session->input_pipeline[current_head] = ch;

    // Store head release (publishes the data write)
    atomic_store_explicit(&session->pipeline_head, next_head, memory_order_release);
    return true;
}

// ENHANCED PIPELINE PROCESSING
// =============================================================================
bool KTerm_WriteChar(KTerm* term, unsigned char ch) {
    // Wrapper around internal function using active session
    // Reads active_session once atomically (in C sense)
    KTermSession* session = &term->sessions[term->active_session];
    return KTerm_WriteCharToSessionInternal(term, session, ch);
}

bool KTerm_WriteString(KTerm* term, const char* str) {
    if (!str) return false;

    while (*str) {
        if (!KTerm_WriteChar(term, *str)) {
            return false;
        }
        str++;
    }
    return true;
}

bool KTerm_WriteFormat(KTerm* term, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return KTerm_WriteString(term, buffer);
}

void KTerm_ClearEvents(KTerm* term) {
    GET_SESSION(term)->pipeline_head = 0;
    GET_SESSION(term)->pipeline_tail = 0;
    GET_SESSION(term)->pipeline_count = 0;
    GET_SESSION(term)->pipeline_overflow = false;
}

// =============================================================================
// BASIC IMPLEMENTATIONS FOR MISSING FUNCTIONS
// =============================================================================

void KTerm_SetResponseCallback(KTerm* term, ResponseCallback callback) {
    term->response_callback = callback;
}

void KTerm_SetPrinterCallback(KTerm* term, PrinterCallback callback) {
    term->printer_callback = callback;
}

void KTerm_SetTitleCallback(KTerm* term, TitleCallback callback) {
    term->title_callback = callback;
}

void KTerm_SetBellCallback(KTerm* term, BellCallback callback) {
    term->bell_callback = callback;
}

void KTerm_SetNotificationCallback(KTerm* term, NotificationCallback callback) {
    term->notification_callback = callback;
    term->notification_callback = callback;
}

void KTerm_SetGatewayCallback(KTerm* term, GatewayCallback callback) {
    term->gateway_callback = callback;
}

void KTerm_SetSessionResizeCallback(KTerm* term, SessionResizeCallback callback) {
    term->session_resize_callback = callback;
}

const char* KTerm_GetWindowTitle(KTerm* term) {
    return GET_SESSION(term)->title.window_title;
}

const char* KTerm_GetIconTitle(KTerm* term) {
    return GET_SESSION(term)->title.icon_title;
}

void KTerm_SetMode(KTerm* term, const char* mode, bool enable) {
    if (strcmp(mode, "application_cursor") == 0) {
        GET_SESSION(term)->dec_modes.application_cursor_keys = enable;
    } else if (strcmp(mode, "auto_wrap") == 0) {
        GET_SESSION(term)->dec_modes.auto_wrap_mode = enable;
    } else if (strcmp(mode, "origin") == 0) {
        GET_SESSION(term)->dec_modes.origin_mode = enable;
    } else if (strcmp(mode, "insert") == 0) {
        GET_SESSION(term)->dec_modes.insert_mode = enable;
    }
}

void KTerm_SetCursorShape(KTerm* term, CursorShape shape) {
    GET_SESSION(term)->cursor.shape = shape;
}

void KTerm_SetCursorKTermColor(KTerm* term, ExtendedKTermColor color) {
    GET_SESSION(term)->cursor.color = color;
}

void KTerm_SetMouseTracking(KTerm* term, MouseTrackingMode mode) {
    GET_SESSION(term)->mouse.mode = mode;
    GET_SESSION(term)->mouse.enabled = (mode != MOUSE_TRACKING_OFF);
}

// Enable or disable mouse features
// Toggles specific mouse functionalities based on feature name
void KTerm_EnableMouseFeature(KTerm* term, const char* feature, bool enable) {
    if (strcmp(feature, "focus") == 0) {
        // Enable/disable focus tracking for mouse reporting (CSI ?1004 h/l)
        GET_SESSION(term)->mouse.focus_tracking = enable;
    } else if (strcmp(feature, "sgr") == 0) {
        // Enable/disable SGR mouse reporting mode (CSI ?1006 h/l)
        GET_SESSION(term)->mouse.sgr_mode = enable;
        // Update mouse mode to SGR if enabled and tracking is active
        if (enable && GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_OFF &&
            GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_URXVT && GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_PIXEL) {
            GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_SGR;
        } else if (!enable && GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_SGR) {
            GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_VT200; // Fallback to VT200
        }
    } else if (strcmp(feature, "cursor") == 0) {
        // Enable/disable custom mouse cursor rendering
        GET_SESSION(term)->mouse.enabled = enable;
        if (!enable) {
            GET_SESSION(term)->mouse.cursor_x = -1; // Hide cursor
            GET_SESSION(term)->mouse.cursor_y = -1;
        }
    } else if (strcmp(feature, "urxvt") == 0) {
        // Enable/disable URXVT mouse reporting mode (CSI ?1015 h/l)
        if (enable) {
            GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_URXVT;
            GET_SESSION(term)->mouse.enabled = true; // Ensure cursor is enabled
        } else if (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_URXVT) {
            GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_OFF;
        }
    } else if (strcmp(feature, "pixel") == 0) {
        // Enable/disable pixel position mouse reporting mode (CSI ?1016 h/l)
        if (enable) {
            GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_PIXEL;
            GET_SESSION(term)->mouse.enabled = true; // Ensure cursor is enabled
        } else if (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_PIXEL) {
            GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_OFF;
        }
    }
}

void KTerm_EnableBracketedPaste(KTerm* term, bool enable) {
    GET_SESSION(term)->bracketed_paste.enabled = enable;
}

bool IsBracketedPasteActive(KTerm* term) {
    return GET_SESSION(term)->bracketed_paste.active;
}

void ProcessPasteData(KTerm* term, const char* data, size_t length) {
    (void)length;
    if (GET_SESSION(term)->bracketed_paste.enabled) {
        KTerm_WriteString(term, "\x1B[200~");
        KTerm_WriteString(term, data);
        KTerm_WriteString(term, "\x1B[201~");
    } else {
        KTerm_WriteString(term, data);
    }
}

static int EncodeUTF8(uint32_t codepoint, char* buffer) {
    if (codepoint <= 0x7F) {
        buffer[0] = (char)codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        buffer[0] = (char)(0xC0 | (codepoint >> 6));
        buffer[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    } else if (codepoint <= 0xFFFF) {
        buffer[0] = (char)(0xE0 | (codepoint >> 12));
        buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    } else if (codepoint <= 0x10FFFF) {
        buffer[0] = (char)(0xF0 | (codepoint >> 18));
        buffer[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }
    return 0;
}

void KTerm_CopySelectionToClipboard(KTerm* term) {
    if (!GET_SESSION(term)->selection.active) return;

    int start_y = GET_SESSION(term)->selection.start_y;
    int start_x = GET_SESSION(term)->selection.start_x;
    int end_y = GET_SESSION(term)->selection.end_y;
    int end_x = GET_SESSION(term)->selection.end_x;

    uint32_t s_idx = start_y * term->width + start_x;
    uint32_t e_idx = end_y * term->width + end_x;

    if (s_idx > e_idx) { uint32_t t = s_idx; s_idx = e_idx; e_idx = t; }

    size_t char_count = (e_idx - s_idx) + 1 + (term->height * 2);
    char* text_buf = calloc(char_count * 4, 1); // UTF-8 safety
    if (!text_buf) return;
    size_t buf_idx = 0;

    int last_y = -1;
    for (uint32_t i = s_idx; i <= e_idx; i++) {
        int cy = i / term->width;
        int cx = i % term->width;

        if (last_y != -1 && cy != last_y) {
            text_buf[buf_idx++] = '\n';
        }
        last_y = cy;

        EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), cy, cx);
        if (cell->ch) {
            buf_idx += EncodeUTF8(cell->ch, &text_buf[buf_idx]);
        }
    }
    text_buf[buf_idx] = '\0';
    KTerm_SetClipboardText(text_buf);
    free(text_buf);
}

void KTerm_SetPipelineTargetFPS(KTerm* term, int fps) {
    if (fps > 0) {
        GET_SESSION(term)->VTperformance.target_frame_time = 1.0 / fps;
        GET_SESSION(term)->VTperformance.time_budget = GET_SESSION(term)->VTperformance.target_frame_time * 0.3;
    }
}

void KTerm_SetPipelineTimeBudget(KTerm* term, double pct) {
    if (pct > 0.0 && pct <= 1.0) {
        GET_SESSION(term)->VTperformance.time_budget = GET_SESSION(term)->VTperformance.target_frame_time * pct;
    }
}


void KTerm_ShowDiagnostics(KTerm* term) {
    KTermStatus status = KTerm_GetStatus(term);
    KTerm_WriteFormat(term, "=== Buffer Diagnostics ===\n");
    KTerm_WriteFormat(term, "Pipeline: %zu/%d bytes\n", status.pipeline_usage, (int)sizeof(GET_SESSION(term)->input_pipeline));
    KTerm_WriteFormat(term, "Keyboard: %zu events\n", status.key_usage);
    KTerm_WriteFormat(term, "Overflow: %s\n", status.overflow_detected ? "YES" : "No");
    KTerm_WriteFormat(term, "Avg Process Time: %.6f ms\n", status.avg_process_time * 1000.0);
}

void KTerm_SwapScreenBuffer(KTerm* term) {
    // Swap pointers
    EnhancedTermChar* temp_buf = GET_SESSION(term)->screen_buffer;
    GET_SESSION(term)->screen_buffer = GET_SESSION(term)->alt_buffer;
    GET_SESSION(term)->alt_buffer = temp_buf;

    // Swap dimensions/metadata
    // For now, only buffer_height differs (Main has scrollback, Alt usually doesn't).
    // But since we allocate Alt buffer with term->height, we must handle this.
    // However, if we swap, the "active" buffer height must reflect what we are pointing to.

    // We didn't add "alt_buffer_height" to the struct, assuming Alt is always screen size.
    // But if we swap, GET_SESSION(term)->screen_buffer now points to a small buffer.
    // So GET_SESSION(term)->buffer_height must be updated.

    // Problem: We need to store the height of the buffer currently in 'alt_buffer' so we can restore it.
    // Let's assume standard behavior:
    // Main Buffer: Has scrollback (Large).
    // Alt Buffer: No scrollback (Screen Height).

    // Swap heads
    int temp_head = GET_SESSION(term)->screen_head;
    GET_SESSION(term)->screen_head = GET_SESSION(term)->alt_screen_head;
    GET_SESSION(term)->alt_screen_head = temp_head;

    if (GET_SESSION(term)->dec_modes.alternate_screen) {
        // Switching BACK to Main Screen
        GET_SESSION(term)->buffer_height = term->height + MAX_SCROLLBACK_LINES;
        GET_SESSION(term)->dec_modes.alternate_screen = false;

        // Restore view offset (if we want to restore scroll position, otherwise 0)
        // For now, reset to 0 (bottom) is safest and standard behavior.
        // Or restore saved? Let's restore saved for better UX.
        GET_SESSION(term)->view_offset = GET_SESSION(term)->saved_view_offset;
    } else {
        // Switching TO Alternate Screen
        GET_SESSION(term)->buffer_height = term->height;
        GET_SESSION(term)->dec_modes.alternate_screen = true;

        // Save current offset and reset view for Alt screen (which has no scrollback)
        GET_SESSION(term)->saved_view_offset = GET_SESSION(term)->view_offset;
        GET_SESSION(term)->view_offset = 0;
    }

    // Force full redraw
    for (int i=0; i<term->height; i++) {
        GET_SESSION(term)->row_dirty[i] = true;
    }
}

static void KTerm_ProcessEventsInternal(KTerm* term, KTermSession* session) {
    // Load tail relaxed (only this thread writes to it)
    int current_tail = atomic_load_explicit(&session->pipeline_tail, memory_order_relaxed);
    // Load head acquire (another thread writes to it)
    int current_head = atomic_load_explicit(&session->pipeline_head, memory_order_acquire);

    if (current_head == current_tail) {
        return;
    }

    // Capture the index of the session OWNING this buffer
    // int processing_session_idx = term->active_session;

    double start_time = KTerm_TimerGetTime();
    int chars_processed = 0;
    int target_chars = session->VTperformance.chars_per_frame;

    int pipeline_usage = (current_head - current_tail + sizeof(session->input_pipeline)) % sizeof(session->input_pipeline);

    if (pipeline_usage > session->VTperformance.burst_threshold) {
        target_chars *= 2;
        session->VTperformance.burst_mode = true;
    } else if (pipeline_usage < target_chars) {
        target_chars = pipeline_usage;
        session->VTperformance.burst_mode = false;
    }

    while (chars_processed < target_chars) {
        // Re-check for empty (we are consuming)
        // Note: current_head is a snapshot. If producer added more, we will process them next frame.
        if (current_tail == current_head) break;

        if (KTerm_TimerGetTime() - start_time > session->VTperformance.time_budget) {
            break;
        }

        unsigned char ch = session->input_pipeline[current_tail];
        int next_tail = (current_tail + 1) % sizeof(session->input_pipeline);

        // Process char.
        // Pass 'session' explicitly to avoid context fragility if active_session changes.
        KTerm_ProcessChar(term, session, ch);

        current_tail = next_tail;
        // Store tail release (publishes free space)
        atomic_store_explicit(&session->pipeline_tail, current_tail, memory_order_release);

        chars_processed++;
    }

    // Update performance metrics
    if (chars_processed > 0) {
        double total_time = KTerm_TimerGetTime() - start_time;
        double time_per_char = total_time / chars_processed;
        session->VTperformance.avg_process_time =
            session->VTperformance.avg_process_time * 0.9 + time_per_char * 0.1;
    }
}

void KTerm_ProcessEvents(KTerm* term) {
    KTerm_ProcessEventsInternal(term, GET_SESSION(term));
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void KTerm_LogUnsupportedSequence(KTerm* term, const char* sequence) {
    if (!GET_SESSION(term)->options.log_unsupported) return;

    GET_SESSION(term)->conformance.compliance.unsupported_sequences++;

    // Use strcpy instead of strncpy to avoid truncation warnings
    size_t len = strlen(sequence);
    if (len >= sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported)) {
        len = sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported) - 1;
    }
    memcpy(GET_SESSION(term)->conformance.compliance.last_unsupported, sequence, len);
    GET_SESSION(term)->conformance.compliance.last_unsupported[len] = '\0';

    if (GET_SESSION(term)->options.debug_sequences) {
        char debug_msg[128];
        snprintf(debug_msg, sizeof(debug_msg),
                "Unsupported: %s (total: %d)\n",
                sequence, GET_SESSION(term)->conformance.compliance.unsupported_sequences);

        if (term->response_callback) {
            term->response_callback(term, debug_msg, strlen(debug_msg));
        }
    }
}

// =============================================================================
// PARAMETER PARSING UTILITIES
// =============================================================================

static int KTerm_ParseCSIParams_Internal(KTermSession* session, const char* params, int* out_params, int max_params) {
    session->param_count = 0;
    memset(session->escape_params, 0, sizeof(session->escape_params));
    memset(session->escape_separators, 0, sizeof(session->escape_separators));

    if (!params || !*params) {
        return 0;
    }

    StreamScanner scanner = { .ptr = params, .len = strlen(params), .pos = 0 };
    if (Stream_Peek(&scanner) == '?') {
        Stream_Consume(&scanner);
    }

    while (scanner.pos < scanner.len && session->param_count < max_params) {
        int value = 0;
        if (Stream_ReadInt(&scanner, &value)) {
            session->escape_params[session->param_count] = (value >= 0) ? value : 0;
        } else {
            // Default 0 if missing or invalid (e.g. empty between semicolons)
            session->escape_params[session->param_count] = 0;
        }
        char sep = Stream_Peek(&scanner);
        if (sep == ';' || sep == ':') {
            session->escape_separators[session->param_count] = sep;
            Stream_Consume(&scanner);
        } else {
            session->escape_separators[session->param_count] = 0;
        }

        session->param_count++;

        if (sep == ';' || sep == ':') {
            // Handle trailing separator implying a default param
            if (scanner.pos >= scanner.len && session->param_count < max_params) {
                 session->escape_params[session->param_count] = 0;
                 session->escape_separators[session->param_count] = 0;
                 session->param_count++;
            }
        } else {
            break;
        }
    }

    if (out_params) {
        for (int i = 0; i < session->param_count; i++) {
            out_params[i] = session->escape_params[i];
        }
    }

    return session->param_count;
}

int KTerm_ParseCSIParams(KTerm* term, const char* params, int* out_params, int max_params) {
    return KTerm_ParseCSIParams_Internal(GET_SESSION(term), params, out_params, max_params);
}

static void ClearCSIParams(KTermSession* session) {
    session->escape_buffer[0] = '\0';
    session->escape_pos = 0;
    session->param_count = 0;
    memset(session->escape_params, 0, sizeof(session->escape_params));
}

void KTerm_ProcessSixelSTChar(KTerm* term, KTermSession* session, unsigned char ch) {
    if (ch == '\\') { // This is ST
        session->parse_state = VT_PARSE_NORMAL;
        // Finalize sixel image size
        session->sixel.width = session->sixel.max_x;
        session->sixel.height = session->sixel.max_y;
        session->sixel.dirty = true;

        // Handle Cursor Placement (Mode 8452 & Standard Sixel)
        int cw = term->char_width;
        int ch_h = term->char_height;
        if (cw <= 0) cw = 8;
        if (ch_h <= 0) ch_h = 10;

        if (session->dec_modes.sixel_cursor_mode) {
             // Mode 8452 Enabled: Place cursor at end of graphic (right side)
             int cols = (session->sixel.width + cw - 1) / cw;
             session->cursor.x = (session->sixel.x / cw) + cols;
             if (session->cursor.x >= term->width) session->cursor.x = term->width - 1;
        } else {
             // Standard Behavior: Carriage Return + Line Feed (below image)
             int rows = (session->sixel.height + ch_h - 1) / ch_h;
             int start_y = session->sixel.y / ch_h;
             int target_y = start_y + rows;

             session->cursor.x = 0; // CR

             // Handle Scrolling
             int bottom = session->scroll_bottom;
             if (target_y > bottom) {
                 int scroll_lines = target_y - bottom;
                 KTerm_ScrollUpRegion(term, session->scroll_top, session->scroll_bottom, scroll_lines);
                 target_y = bottom;
             }
             session->cursor.y = target_y;
             if (session->cursor.y >= term->height) session->cursor.y = term->height - 1;
        }
    } else {
        // ESC was start of new sequence
        // We treat the current char 'ch' as the one following ESC.
        // e.g. ESC P -> ch='P'. KTerm_ProcessEscapeChar(term, 'P') -> PARSE_DCS.
        KTerm_ProcessEscapeChar(term, session, ch);
    }
}

static int KTerm_GetCSIParam_Internal(KTermSession* session, int index, int default_value) {
    if (index >= 0 && index < session->param_count) {
        return (session->escape_params[index] == 0) ? default_value : session->escape_params[index];
    }
    return default_value;
}

int KTerm_GetCSIParam(KTerm* term, int index, int default_value) {
    return KTerm_GetCSIParam_Internal(GET_SESSION(term), index, default_value);
}


// =============================================================================
// CURSOR MOVEMENT IMPLEMENTATIONS
// =============================================================================

static void ExecuteCUU_Internal(KTermSession* session) { // Cursor Up
    int n = KTerm_GetCSIParam_Internal(session, 0, 1);
    int new_y = session->cursor.y - n;

    if (session->dec_modes.origin_mode) {
        session->cursor.y = (new_y < session->scroll_top) ? session->scroll_top : new_y;
    } else {
        session->cursor.y = (new_y < 0) ? 0 : new_y;
    }
}
void ExecuteCUU(KTerm* term) { ExecuteCUU_Internal(GET_SESSION(term)); }

static void ExecuteCUD_Internal(KTermSession* session) { // Cursor Down
    int n = KTerm_GetCSIParam_Internal(session, 0, 1);
    int new_y = session->cursor.y + n;

    if (session->dec_modes.origin_mode) {
        session->cursor.y = (new_y > session->scroll_bottom) ? session->scroll_bottom : new_y;
    } else {
        session->cursor.y = (new_y >= session->rows) ? session->rows - 1 : new_y;
    }
}
void ExecuteCUD(KTerm* term) { ExecuteCUD_Internal(GET_SESSION(term)); }

static void ExecuteCUF_Internal(KTermSession* session) { // Cursor Forward
    int n = KTerm_GetCSIParam_Internal(session, 0, 1);
    session->cursor.x = (session->cursor.x + n >= session->cols) ? session->cols - 1 : session->cursor.x + n;
}
void ExecuteCUF(KTerm* term) { ExecuteCUF_Internal(GET_SESSION(term)); }

static void ExecuteCUB_Internal(KTermSession* session) { // Cursor Back
    int n = KTerm_GetCSIParam_Internal(session, 0, 1);
    session->cursor.x = (session->cursor.x - n < 0) ? 0 : session->cursor.x - n;
}
void ExecuteCUB(KTerm* term) { ExecuteCUB_Internal(GET_SESSION(term)); }

static void ExecuteCNL_Internal(KTermSession* session) { // Cursor Next Line
    int n = KTerm_GetCSIParam_Internal(session, 0, 1);
    session->cursor.y = (session->cursor.y + n >= session->rows) ? session->rows - 1 : session->cursor.y + n;
    session->cursor.x = session->left_margin;
}
void ExecuteCNL(KTerm* term) { ExecuteCNL_Internal(GET_SESSION(term)); }

static void ExecuteCPL_Internal(KTermSession* session) { // Cursor Previous Line
    int n = KTerm_GetCSIParam_Internal(session, 0, 1);
    session->cursor.y = (session->cursor.y - n < 0) ? 0 : session->cursor.y - n;
    session->cursor.x = session->left_margin;
}
void ExecuteCPL(KTerm* term) { ExecuteCPL_Internal(GET_SESSION(term)); }

static void ExecuteCHA_Internal(KTermSession* session) { // Cursor Horizontal Absolute
    int n = KTerm_GetCSIParam_Internal(session, 0, 1) - 1; // Convert to 0-based
    session->cursor.x = (n < 0) ? 0 : (n >= session->cols) ? session->cols - 1 : n;
}
void ExecuteCHA(KTerm* term) { ExecuteCHA_Internal(GET_SESSION(term)); }

static void ExecuteCUP_Internal(KTermSession* session) { // Cursor Position
    int row = KTerm_GetCSIParam_Internal(session, 0, 1) - 1; // Convert to 0-based
    int col = KTerm_GetCSIParam_Internal(session, 1, 1) - 1;

    if (session->dec_modes.origin_mode) {
        row += session->scroll_top;
        col += session->left_margin;
    }

    session->cursor.y = (row < 0) ? 0 : (row >= session->rows) ? session->rows - 1 : row;
    session->cursor.x = (col < 0) ? 0 : (col >= session->cols) ? session->cols - 1 : col;

    // Clamp to scrolling region if in origin mode
    if (session->dec_modes.origin_mode) {
        if (session->cursor.y < session->scroll_top) session->cursor.y = session->scroll_top;
        if (session->cursor.y > session->scroll_bottom) session->cursor.y = session->scroll_bottom;
        if (session->cursor.x < session->left_margin) session->cursor.x = session->left_margin;
        if (session->cursor.x > session->right_margin) session->cursor.x = session->right_margin;
    }
}
void ExecuteCUP(KTerm* term) { ExecuteCUP_Internal(GET_SESSION(term)); }

static void ExecuteVPA_Internal(KTermSession* session) { // Vertical Position Absolute
    int n = KTerm_GetCSIParam_Internal(session, 0, 1) - 1; // Convert to 0-based

    if (session->dec_modes.origin_mode) {
        n += session->scroll_top;
        session->cursor.y = (n < session->scroll_top) ? session->scroll_top :
                           (n > session->scroll_bottom) ? session->scroll_bottom : n;
    } else {
        session->cursor.y = (n < 0) ? 0 : (n >= session->rows) ? session->rows - 1 : n;
    }
}
void ExecuteVPA(KTerm* term) { ExecuteVPA_Internal(GET_SESSION(term)); }

// =============================================================================
// ERASING IMPLEMENTATIONS
// =============================================================================

static void ExecuteED_Internal(KTerm* term, KTermSession* session, bool private_mode) { // Erase in Display
    int n = KTerm_GetCSIParam_Internal(session, 0, 0);

    switch (n) {
        case 0: // Clear from cursor to end of screen
            // Clear current line from cursor
            for (int x = session->cursor.x; x < term->width; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(session, session->cursor.y, x);
                if (private_mode && (cell->flags & KTERM_ATTR_PROTECTED)) continue;
                KTerm_ClearCell_Internal(session, cell);
            }
            // Clear remaining lines
            for (int y = session->cursor.y + 1; y < term->height; y++) {
                for (int x = 0; x < term->width; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(session, y, x);
                    if (private_mode && (cell->flags & KTERM_ATTR_PROTECTED)) continue;
                    KTerm_ClearCell_Internal(session, cell);
                }
            }
            break;

        case 1: // Clear from beginning of screen to cursor
            // Clear lines before cursor
            for (int y = 0; y < GET_SESSION(term)->cursor.y; y++) {
                for (int x = 0; x < term->width; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
                    if (private_mode && (cell->flags & KTERM_ATTR_PROTECTED)) continue;
                    KTerm_ClearCell(term, cell);
                }
            }
            // Clear current line up to cursor
            for (int x = 0; x <= GET_SESSION(term)->cursor.x; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x);
                if (private_mode && (cell->flags & KTERM_ATTR_PROTECTED)) continue;
                KTerm_ClearCell(term, cell);
            }
            break;

        case 2: // Clear entire screen
        case 3: // Clear entire screen and scrollback (xterm extension)
            for (int y = 0; y < term->height; y++) {
                for (int x = 0; x < term->width; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
                    if (private_mode && (cell->flags & KTERM_ATTR_PROTECTED)) continue;
                    KTerm_ClearCell(term, cell);
                }
            }
            break;

        default:
            KTerm_LogUnsupportedSequence(term, "Unknown ED parameter");
            break;
    }
}
void ExecuteED(KTerm* term, bool private_mode) { ExecuteED_Internal(term, GET_SESSION(term), private_mode); }

static void ExecuteEL_Internal(KTerm* term, KTermSession* session, bool private_mode) { // Erase in Line
    int n = KTerm_GetCSIParam_Internal(session, 0, 0);

    switch (n) {
        case 0: // Clear from cursor to end of line
            for (int x = session->cursor.x; x < term->width; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(session, session->cursor.y, x);
                if (private_mode && (cell->flags & KTERM_ATTR_PROTECTED)) continue;
                KTerm_ClearCell_Internal(session, cell);
            }
            break;

        case 1: // Clear from beginning of line to cursor
            for (int x = 0; x <= session->cursor.x; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(session, session->cursor.y, x);
                if (private_mode && (cell->flags & KTERM_ATTR_PROTECTED)) continue;
                KTerm_ClearCell_Internal(session, cell);
            }
            break;

        case 2: // Clear entire line
            for (int x = 0; x < term->width; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(session, session->cursor.y, x);
                if (private_mode && (cell->flags & KTERM_ATTR_PROTECTED)) continue;
                KTerm_ClearCell_Internal(session, cell);
            }
            break;

        default:
            KTerm_LogUnsupportedSequence(term, "Unknown EL parameter");
            break;
    }
}
void ExecuteEL(KTerm* term, bool private_mode) { ExecuteEL_Internal(term, GET_SESSION(term), private_mode); }

static void ExecuteECH_Internal(KTerm* term, KTermSession* session) { // Erase Character
    (void)term;
    int n = KTerm_GetCSIParam_Internal(session, 0, 1);

    for (int i = 0; i < n && session->cursor.x + i < term->width; i++) {
        KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, session->cursor.y, session->cursor.x + i));
    }
}
void ExecuteECH(KTerm* term) { ExecuteECH_Internal(term, GET_SESSION(term)); }

// =============================================================================
// INSERTION AND DELETION IMPLEMENTATIONS
// =============================================================================

static void ExecuteIL_Internal(KTerm* term, KTermSession* session) { // Insert Line
    int n = KTerm_GetCSIParam_Internal(session, 0, 1);
    KTerm_InsertLinesAt_Internal(term, session, session->cursor.y, n);
}
void ExecuteIL(KTerm* term) { ExecuteIL_Internal(term, GET_SESSION(term)); }

static void ExecuteDL_Internal(KTerm* term, KTermSession* session) { // Delete Line
    int n = KTerm_GetCSIParam_Internal(session, 0, 1);
    KTerm_DeleteLinesAt_Internal(term, session, session->cursor.y, n);
}
void ExecuteDL(KTerm* term) { ExecuteDL_Internal(term, GET_SESSION(term)); }

void ExecuteICH(KTerm* term) { // Insert Character
    int n = KTerm_GetCSIParam(term, 0, 1);
    KTerm_InsertCharactersAt(term, GET_SESSION(term)->cursor.y, GET_SESSION(term)->cursor.x, n);
}

void ExecuteDCH(KTerm* term) { // Delete Character
    int n = KTerm_GetCSIParam(term, 0, 1);
    KTerm_DeleteCharactersAt(term, GET_SESSION(term)->cursor.y, GET_SESSION(term)->cursor.x, n);
}

void ExecuteREP(KTerm* term) { // Repeat Preceding Graphic Character
    int n = KTerm_GetCSIParam(term, 0, 1);
    if (n < 1) n = 1;
    if (GET_SESSION(term)->last_char > 0) {
        for (int i = 0; i < n; i++) {
            if (GET_SESSION(term)->cursor.x > GET_SESSION(term)->right_margin) {
                if (GET_SESSION(term)->dec_modes.auto_wrap_mode) {
                    GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
                    GET_SESSION(term)->cursor.y++;
                    if (GET_SESSION(term)->cursor.y > GET_SESSION(term)->scroll_bottom) {
                        GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_bottom;
                        KTerm_ScrollUpRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, 1);
                    }
                } else {
                    GET_SESSION(term)->cursor.x = GET_SESSION(term)->right_margin;
                }
            }
            KTerm_InsertCharacterAtCursor(term, GET_SESSION(term)->last_char);
            GET_SESSION(term)->cursor.x++;
        }
    }
}

// =============================================================================
// SCROLLING IMPLEMENTATIONS
// =============================================================================

void ExecuteSU(KTerm* term) { // Scroll Up
    int n = KTerm_GetCSIParam(term, 0, 1);
    KTerm_ScrollUpRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, n);
}

void ExecuteSD(KTerm* term) { // Scroll Down
    int n = KTerm_GetCSIParam(term, 0, 1);
    KTerm_ScrollDownRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, n);
}

// =============================================================================
// ENHANCED SGR (SELECT GRAPHIC RENDITION) IMPLEMENTATION
// =============================================================================

int ProcessExtendedKTermColor(KTerm* term, ExtendedKTermColor* color, int param_index) {
    int consumed = 0;

    if (param_index + 1 < GET_SESSION(term)->param_count) {
        int color_type = GET_SESSION(term)->escape_params[param_index + 1];

        if (color_type == 5 && param_index + 2 < GET_SESSION(term)->param_count) {
            // 256-color mode: ESC[38;5;n or ESC[48;5;n
            int color_index = GET_SESSION(term)->escape_params[param_index + 2];
            if (color_index >= 0 && color_index < 256) {
                color->color_mode = 0;
                color->value.index = color_index;
            }
            consumed = 2;

        } else if (color_type == 2 && param_index + 4 < GET_SESSION(term)->param_count) {
            // True color mode: ESC[38;2;r;g;b or ESC[48;2;r;g;b
            int r = GET_SESSION(term)->escape_params[param_index + 2] & 0xFF;
            int g = GET_SESSION(term)->escape_params[param_index + 3] & 0xFF;
            int b = GET_SESSION(term)->escape_params[param_index + 4] & 0xFF;

            color->color_mode = 1;
            color->value.rgb = (RGB_KTermColor){r, g, b, 255};
            consumed = 4;
        }
    }

    return consumed;
}

void ExecuteXTPUSHSGR(KTerm* term) {
    KTermSession* s = GET_SESSION(term);
    if (s->sgr_stack_depth < 10) {
        SavedSGRState* state = &s->sgr_stack[s->sgr_stack_depth++];
        state->fg_color = s->current_fg;
        state->bg_color = s->current_bg;
        state->ul_color = s->current_ul_color;
        state->st_color = s->current_st_color;
        state->attributes = s->current_attributes;
    }
}

void ExecuteXTPOPSGR(KTerm* term) {
    KTermSession* s = GET_SESSION(term);
    if (s->sgr_stack_depth > 0) {
        SavedSGRState* state = &s->sgr_stack[--s->sgr_stack_depth];
        s->current_fg = state->fg_color;
        s->current_bg = state->bg_color;
        s->current_ul_color = state->ul_color;
        s->current_st_color = state->st_color;
        s->current_attributes = state->attributes;
    }
}

void KTerm_ResetAllAttributes(KTerm* term) {
    GET_SESSION(term)->current_fg.color_mode = 0;
    GET_SESSION(term)->current_fg.value.index = COLOR_WHITE;
    GET_SESSION(term)->current_bg.color_mode = 0;
    GET_SESSION(term)->current_bg.value.index = COLOR_BLACK;
    GET_SESSION(term)->current_ul_color.color_mode = 2; // Default
    GET_SESSION(term)->current_st_color.color_mode = 2; // Default
    GET_SESSION(term)->current_attributes = 0;
}

void ExecuteSGR(KTerm* term) {
    if (GET_SESSION(term)->param_count == 0) {
        // Reset all attributes
        KTerm_ResetAllAttributes(term);
        return;
    }

    bool ansi_restricted = (GET_SESSION(term)->conformance.level == VT_LEVEL_ANSI_SYS);

    for (int i = 0; i < GET_SESSION(term)->param_count; i++) {
        int param = GET_SESSION(term)->escape_params[i];

        switch (param) {
            case 0: // Reset all
                KTerm_ResetAllAttributes(term);
                break;

            // Intensity
            case 1: GET_SESSION(term)->current_attributes |= KTERM_ATTR_BOLD; break;
            case 2: if (!ansi_restricted) GET_SESSION(term)->current_attributes |= KTERM_ATTR_FAINT; break;
            case 22: GET_SESSION(term)->current_attributes &= ~(KTERM_ATTR_BOLD | KTERM_ATTR_FAINT); break;

            // Style
            case 3: if (!ansi_restricted) GET_SESSION(term)->current_attributes |= KTERM_ATTR_ITALIC; break;
            case 23: if (!ansi_restricted) GET_SESSION(term)->current_attributes &= ~KTERM_ATTR_ITALIC; break;

            case 4:
                // SGR 4: Underline (with optional subparameters for style)
                // 4:0=None, 4:1=Single, 4:2=Double, 4:3=Curly, 4:4=Dotted, 4:5=Dashed
                if (GET_SESSION(term)->escape_separators[i] == ':') {
                    if (i + 1 < GET_SESSION(term)->param_count) {
                        int style = GET_SESSION(term)->escape_params[i+1];
                        i++; // Consume subparam

                        // Clear existing UL bits and style
                        GET_SESSION(term)->current_attributes &= ~(KTERM_ATTR_UNDERLINE | KTERM_ATTR_DOUBLE_UNDERLINE | KTERM_ATTR_UL_STYLE_MASK);

                        switch (style) {
                            case 0: break; // None (already cleared)
                            case 1: GET_SESSION(term)->current_attributes |= KTERM_ATTR_UNDERLINE | KTERM_ATTR_UL_STYLE_SINGLE; break;
                            case 2: GET_SESSION(term)->current_attributes |= KTERM_ATTR_DOUBLE_UNDERLINE | KTERM_ATTR_UL_STYLE_DOUBLE; break;
                            case 3: GET_SESSION(term)->current_attributes |= KTERM_ATTR_UNDERLINE | KTERM_ATTR_UL_STYLE_CURLY; break;
                            case 4: GET_SESSION(term)->current_attributes |= KTERM_ATTR_UNDERLINE | KTERM_ATTR_UL_STYLE_DOTTED; break;
                            case 5: GET_SESSION(term)->current_attributes |= KTERM_ATTR_UNDERLINE | KTERM_ATTR_UL_STYLE_DASHED; break;
                            default: GET_SESSION(term)->current_attributes |= KTERM_ATTR_UNDERLINE | KTERM_ATTR_UL_STYLE_SINGLE; break; // Fallback
                        }
                    } else {
                        // Trailing colon? Assume single.
                        GET_SESSION(term)->current_attributes |= KTERM_ATTR_UNDERLINE | KTERM_ATTR_UL_STYLE_SINGLE;
                    }
                } else {
                    // Standard SGR 4 (Single)
                    GET_SESSION(term)->current_attributes &= ~KTERM_ATTR_UL_STYLE_MASK;
                    GET_SESSION(term)->current_attributes |= KTERM_ATTR_UNDERLINE | KTERM_ATTR_UL_STYLE_SINGLE;
                }
                break;

            case 21: if (!ansi_restricted) GET_SESSION(term)->current_attributes |= KTERM_ATTR_DOUBLE_UNDERLINE | KTERM_ATTR_UL_STYLE_DOUBLE; break;
            case 24: GET_SESSION(term)->current_attributes &= ~(KTERM_ATTR_UNDERLINE | KTERM_ATTR_DOUBLE_UNDERLINE | KTERM_ATTR_UL_STYLE_MASK); break;

            case 5:
                // SGR 5: Slow Blink (Standard)
                GET_SESSION(term)->current_attributes |= KTERM_ATTR_BLINK_SLOW;
                GET_SESSION(term)->current_attributes &= ~KTERM_ATTR_BLINK;
                break;
            case 6:
                // SGR 6: Rapid Blink (Standard)
                if (!ansi_restricted) {
                    GET_SESSION(term)->current_attributes |= KTERM_ATTR_BLINK;
                    GET_SESSION(term)->current_attributes &= ~KTERM_ATTR_BLINK_SLOW;
                }
                break;
            case 25:
                GET_SESSION(term)->current_attributes &= ~(KTERM_ATTR_BLINK | KTERM_ATTR_BLINK_BG | KTERM_ATTR_BLINK_SLOW);
                break;

            case 7: GET_SESSION(term)->current_attributes |= KTERM_ATTR_REVERSE; break;
            case 27: GET_SESSION(term)->current_attributes &= ~KTERM_ATTR_REVERSE; break;

            case 8: GET_SESSION(term)->current_attributes |= KTERM_ATTR_CONCEAL; break;
            case 28: GET_SESSION(term)->current_attributes &= ~KTERM_ATTR_CONCEAL; break;

            case 9: if (!ansi_restricted) GET_SESSION(term)->current_attributes |= KTERM_ATTR_STRIKE; break;
            case 29: if (!ansi_restricted) GET_SESSION(term)->current_attributes &= ~KTERM_ATTR_STRIKE; break;

            case 53: if (!ansi_restricted) GET_SESSION(term)->current_attributes |= KTERM_ATTR_OVERLINE; break;
            case 55: if (!ansi_restricted) GET_SESSION(term)->current_attributes &= ~KTERM_ATTR_OVERLINE; break;

            // Standard colors (30-37, 40-47)
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                GET_SESSION(term)->current_fg.color_mode = 0;
                GET_SESSION(term)->current_fg.value.index = param - 30;
                break;

            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                GET_SESSION(term)->current_bg.color_mode = 0;
                GET_SESSION(term)->current_bg.value.index = param - 40;
                break;

            // Bright colors (90-97, 100-107)
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                if (!ansi_restricted) {
                    GET_SESSION(term)->current_fg.color_mode = 0;
                    GET_SESSION(term)->current_fg.value.index = param - 90 + 8;
                }
                break;

            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                if (!ansi_restricted) {
                    GET_SESSION(term)->current_bg.color_mode = 0;
                    GET_SESSION(term)->current_bg.value.index = param - 100 + 8;
                }
                break;

            case 66: if (!ansi_restricted) GET_SESSION(term)->current_attributes |= KTERM_ATTR_BLINK_BG; break;

            // Extended colors
            case 38: // Set foreground color
                if (!ansi_restricted) i += ProcessExtendedKTermColor(term, &GET_SESSION(term)->current_fg, i);
                else {
                    // Skip parameters
                    // This is complex because we need to parse sub-parameters.
                    // For now, just skip.
                }
                break;

            case 48: // Set background color
                if (!ansi_restricted) i += ProcessExtendedKTermColor(term, &GET_SESSION(term)->current_bg, i);
                break;

            case 58: // Set underline color
                if (!ansi_restricted) i += ProcessExtendedKTermColor(term, &GET_SESSION(term)->current_ul_color, i);
                break;

            case 59: // Reset underline color
                if (!ansi_restricted) GET_SESSION(term)->current_ul_color.color_mode = 2; // Default
                break;

            // Default colors
            case 39:
                GET_SESSION(term)->current_fg.color_mode = 0;
                GET_SESSION(term)->current_fg.value.index = COLOR_WHITE;
                break;

            case 49:
                GET_SESSION(term)->current_bg.color_mode = 0;
                GET_SESSION(term)->current_bg.value.index = COLOR_BLACK;
                break;

            default:
                if (GET_SESSION(term)->options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown SGR parameter: %d", param);
                    KTerm_LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    }
}

// =============================================================================
// TERMINAL MODE IMPLEMENTATIONS
// =============================================================================

// Helper function to compute screen buffer checksum (for CSI ?63 n)
static uint32_t ComputeScreenChecksum(KTerm* term, int page) {
    (void)page;
    uint32_t checksum = 0;
    // Simple CRC16-like checksum for screen buffer
    for (int y = 0; y < term->height; y++) {
        for (int x = 0; x < term->width; x++) {
            EnhancedTermChar *cell = GetScreenCell(GET_SESSION(term), y, x);
            checksum += cell->ch;
            checksum += (cell->fg_color.color_mode == 0 ? cell->fg_color.value.index : (cell->fg_color.value.rgb.r << 16 | cell->fg_color.value.rgb.g << 8 | cell->fg_color.value.rgb.b));
            checksum += (cell->bg_color.color_mode == 0 ? cell->bg_color.value.index : (cell->bg_color.value.rgb.r << 16 | cell->bg_color.value.rgb.g << 8 | cell->bg_color.value.rgb.b));
            checksum = (checksum >> 16) + (checksum & 0xFFFF);
        }
    }
    return checksum & 0xFFFF;
}

void SwitchScreenBuffer(KTerm* term, bool to_alternate) {
    if (!GET_SESSION(term)->conformance.features.alternate_screen) {
        KTerm_LogUnsupportedSequence(term, "Alternate screen not supported");
        return;
    }

    // In new Ring Buffer architecture, we swap buffers rather than copy.
    KTerm_SwapScreenBuffer(term);
    // KTerm_SwapScreenBuffer handles logic if implemented correctly.
    // However, this function `SwitchScreenBuffer` seems to enforce explicit "to_alternate" direction.
    // We should implement it using pointers.

    if (to_alternate && !GET_SESSION(term)->dec_modes.alternate_screen) {
        KTerm_SwapScreenBuffer(term); // Swaps to alt
    } else if (!to_alternate && GET_SESSION(term)->dec_modes.alternate_screen) {
        KTerm_SwapScreenBuffer(term); // Swaps back to main
    }
}

// Set terminal modes internally
// Configures DEC private modes (CSI ? Pm h/l) and ANSI modes (CSI Pm h/l)
static void KTerm_SetModeInternal(KTerm* term, int mode, bool enable, bool private_mode) {
    if (private_mode) {
        // DEC Private Modes
        switch (mode) {
            case 1: // DECCKM - Cursor Key Mode
                // Enable/disable application cursor keys
                GET_SESSION(term)->dec_modes.application_cursor_keys = enable;
                GET_SESSION(term)->dec_modes.application_cursor_keys = enable;
                break;

            case 2: // DECANM - ANSI Mode
                // Switch between VT52 and ANSI mode
                if (!enable && GET_SESSION(term)->conformance.features.vt52_mode) {
                    GET_SESSION(term)->parse_state = PARSE_VT52;
                }
                break;

            case 3: // DECCOLM - Column Mode
                // Set 132-column mode
                // Standard requires clearing screen, resetting margins, and homing cursor.
                if (!GET_SESSION(term)->dec_modes.allow_80_132_mode) break; // Mode 40 must be enabled
                if (GET_SESSION(term)->dec_modes.column_mode_132 != enable) {
                    GET_SESSION(term)->dec_modes.column_mode_132 = enable;

                    int target_cols = enable ? 132 : 80;
                    KTerm_Resize(term, target_cols, term->height);

                    if (!GET_SESSION(term)->dec_modes.no_clear_on_column_change) {
                        // 1. Clear Screen
                        for (int y = 0; y < term->height; y++) {
                            for (int x = 0; x < term->width; x++) {
                                KTerm_ClearCell(term, GetScreenCell(GET_SESSION(term), y, x));
                            }
                            GET_SESSION(term)->row_dirty[y] = true;
                        }

                        // 2. Reset Margins
                        GET_SESSION(term)->scroll_top = 0;
                        GET_SESSION(term)->scroll_bottom = term->height - 1;
                        GET_SESSION(term)->left_margin = 0;
                        GET_SESSION(term)->right_margin = term->width - 1;

                        // 3. Home Cursor
                        GET_SESSION(term)->cursor.x = 0;
                        GET_SESSION(term)->cursor.y = 0;
                    }
                }
                break;

            case 4: // DECSCLM - Scrolling Mode
                // Enable/disable smooth scrolling
                GET_SESSION(term)->dec_modes.smooth_scroll = enable;
                break;

            case 5: // DECSCNM - Screen Mode
                // Enable/disable reverse video
                GET_SESSION(term)->dec_modes.reverse_video = enable;
                break;

            case 6: // DECOM - Origin Mode
                // Enable/disable origin mode, adjust cursor position
                GET_SESSION(term)->dec_modes.origin_mode = enable;
                if (enable) {
                    GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
                    GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_top;
                } else {
                    GET_SESSION(term)->cursor.x = 0;
                    GET_SESSION(term)->cursor.y = 0;
                }
                break;

            case 7: // DECAWM - Auto Wrap Mode
                // Enable/disable auto wrap
                GET_SESSION(term)->dec_modes.auto_wrap_mode = enable;
                break;

            case 8: // DECARM - Auto Repeat Mode
                // Enable/disable auto repeat keys
                GET_SESSION(term)->dec_modes.auto_repeat_keys = enable;
                break;

            case 9: // X10 Mouse Tracking
                // Enable/disable X10 mouse tracking
                GET_SESSION(term)->mouse.mode = enable ? MOUSE_TRACKING_X10 : MOUSE_TRACKING_OFF;
                GET_SESSION(term)->mouse.enabled = enable;
                break;

            case 12: // DECSET 12 - Local Echo / Send/Receive
                // Enable/disable local echo mode
                GET_SESSION(term)->dec_modes.local_echo = enable;
                break;

            case 18: // DECPFF - Print Form Feed
                GET_SESSION(term)->dec_modes.print_form_feed = enable;
                break;

            case 19: // DECPEX - Print Extent
                GET_SESSION(term)->dec_modes.print_extent = enable;
                break;

            case 25: // DECTCEM - Text Cursor Enable Mode
                // Enable/disable text cursor visibility
                GET_SESSION(term)->dec_modes.cursor_visible = enable;
                GET_SESSION(term)->cursor.visible = enable;
                break;

            case 38: // DECTEK - Tektronix Mode
                if (enable) {
                    GET_SESSION(term)->parse_state = PARSE_TEKTRONIX;
                    term->tektronix.state = 0; // Alpha
                    term->tektronix.x = 0;
                    term->tektronix.y = 0;
                    term->tektronix.pen_down = false;
                    term->vector_count = 0; // Clear screen on entry
                } else {
                    GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                }
                break;

            case 40: // Allow 80/132 Column Mode
                GET_SESSION(term)->dec_modes.allow_80_132_mode = enable;
                break;

            case 41: // DECELR - Locator Enable
                GET_SESSION(term)->locator_enabled = enable;
                break;

            case 45: // DECEDM - Extended Editing Mode (Insert/Replace)
                GET_SESSION(term)->dec_modes.edit_mode = enable;
                break;

            case 47: // Alternate Screen Buffer
            case 1047: // Alternate Screen Buffer (xterm)
                // Switch between main and alternate screen buffers
                if (enable && GET_SESSION(term)->dec_modes.alt_cursor_save) KTerm_ExecuteSaveCursor(term);
                SwitchScreenBuffer(term, enable);
                if (!enable && GET_SESSION(term)->dec_modes.alt_cursor_save) KTerm_ExecuteRestoreCursor(term);
                break;

            case 1041: // Alt Cursor Save Mode
                GET_SESSION(term)->dec_modes.alt_cursor_save = enable;
                break;

            case 1048: // Save/Restore Cursor
                // Save or restore cursor state
                if (enable) {
                    KTerm_ExecuteSaveCursor(term);
                } else {
                    KTerm_ExecuteRestoreCursor(term);
                }
                break;

            case 10: // DECAKM - Alias for DECNKM (VT100)
            case 66: // DECNKM - Numeric Keypad Mode
                // enable=true -> Application Keypad (DECKPAM)
                // enable=false -> Numeric Keypad (DECKPNM)
                GET_SESSION(term)->input.keypad_application_mode = enable;
                break;

            case 69: // DECLRMM - Vertical Split Mode (Left/Right Margins)
                GET_SESSION(term)->dec_modes.declrmm_enabled = enable;
                if (!enable) {
                    // Reset left/right margins when disabled
                    GET_SESSION(term)->left_margin = 0;
                    GET_SESSION(term)->right_margin = term->width - 1;
                }
                break;

            case 80: // DECSDM - Sixel Display Mode
                // xterm: enable (h) -> Scrolling DISABLED. disable (l) -> Scrolling ENABLED.
                GET_SESSION(term)->dec_modes.sixel_scrolling_mode = enable;
                // Sync session-specific graphics state if active?
                // The Sixel state is reset on DCS entry anyway.
                break;

            case 95: // DECNCSM - No Clear Screen on Column Change
                GET_SESSION(term)->dec_modes.no_clear_on_column_change = enable;
                break;

            case 1049: // Alternate Screen + Save/Restore Cursor
                // Save/restore cursor and switch screen buffer
                if (enable) {
                    KTerm_ExecuteSaveCursor(term);
                    SwitchScreenBuffer(term, true);
                    ExecuteED(term, false); // Clear screen
                    GET_SESSION(term)->cursor.x = 0;
                    GET_SESSION(term)->cursor.y = 0;
                } else {
                    SwitchScreenBuffer(term, false);
                    KTerm_ExecuteRestoreCursor(term);
                }
                break;

            case 8452: // Sixel Cursor Mode (xterm)
                // enable -> Cursor at end of graphic
                // disable -> Cursor at start of next line (standard)
                GET_SESSION(term)->dec_modes.sixel_cursor_mode = enable;
                break;

            case 1000: // VT200 Mouse Tracking
                // Enable/disable VT200 mouse tracking
                GET_SESSION(term)->mouse.mode = enable ? (GET_SESSION(term)->mouse.sgr_mode ? MOUSE_TRACKING_SGR : MOUSE_TRACKING_VT200) : MOUSE_TRACKING_OFF;
                GET_SESSION(term)->mouse.enabled = enable;
                break;

            case 1001: // VT200 Highlight Mouse Tracking
                // Enable/disable VT200 highlight tracking
                GET_SESSION(term)->mouse.mode = enable ? MOUSE_TRACKING_VT200_HIGHLIGHT : MOUSE_TRACKING_OFF;
                GET_SESSION(term)->mouse.enabled = enable;
                break;

            case 1002: // Button Event Mouse Tracking
                // Enable/disable button-event tracking
                GET_SESSION(term)->mouse.mode = enable ? MOUSE_TRACKING_BTN_EVENT : MOUSE_TRACKING_OFF;
                GET_SESSION(term)->mouse.enabled = enable;
                break;

            case 1003: // Any Event Mouse Tracking
                // Enable/disable any-event tracking
                GET_SESSION(term)->mouse.mode = enable ? MOUSE_TRACKING_ANY_EVENT : MOUSE_TRACKING_OFF;
                GET_SESSION(term)->mouse.enabled = enable;
                break;

            case 1004: // Focus In/Out Events
                // Enable/disable focus tracking
                GET_SESSION(term)->mouse.focus_tracking = enable;
                break;

            case 1005: // UTF-8 Mouse Mode
                // UTF-8 mouse encoding (legacy, SGR preferred)
                break;

            case 1006: // SGR Mouse Mode
                // Enable/disable SGR mouse reporting
                GET_SESSION(term)->mouse.sgr_mode = enable;
                if (enable && GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_OFF &&
                    GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_URXVT && GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_PIXEL) {
                    GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_SGR;
                } else if (!enable && GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_SGR) {
                    GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_VT200;
                }
                break;

            case 1015: // URXVT Mouse Mode
                // Enable/disable URXVT mouse reporting
                GET_SESSION(term)->mouse.mode = enable ? MOUSE_TRACKING_URXVT : MOUSE_TRACKING_OFF;
                GET_SESSION(term)->mouse.enabled = enable;
                break;

            case 1016: // Pixel Position Mouse Mode
                // Enable/disable pixel tracking
                GET_SESSION(term)->mouse.mode = enable ? MOUSE_TRACKING_PIXEL : MOUSE_TRACKING_OFF;
                GET_SESSION(term)->mouse.enabled = enable;
                break;

            case 8246: // BDSM - Bi-Directional Support Mode (Private)
                GET_SESSION(term)->dec_modes.bidi_mode = enable;
                break;

            case 2004: // Bracketed Paste Mode
                // Enable/disable bracketed paste
                GET_SESSION(term)->bracketed_paste.enabled = enable;
                break;

            default:
                // Log unsupported DEC modes
                if (GET_SESSION(term)->options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown DEC mode: %d", mode);
                    KTerm_LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    } else {
        // ANSI Modes
        switch (mode) {
            case 4: // IRM - Insert/Replace Mode
                // Enable/disable insert mode
                GET_SESSION(term)->dec_modes.insert_mode = enable;
                break;

            case 20: // LNM - Line Feed/New Line Mode
                // Enable/disable line feed/new line mode
                GET_SESSION(term)->ansi_modes.line_feed_new_line = enable;
                break;

            case 7: // ANSI Mode 7: Line Wrap (standard in ANSI.SYS, private in VT100)
                // Restrict this override to ANSI.SYS level to avoid conflict with standard ISO 6429 VEM (Vertical Editing Mode)
                if (GET_SESSION(term)->conformance.level == VT_LEVEL_ANSI_SYS) {
                    GET_SESSION(term)->dec_modes.auto_wrap_mode = enable;
                }
                break;

            default:
                // Log unsupported ANSI modes
                if (GET_SESSION(term)->options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown ANSI mode: %d", mode);
                    KTerm_LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    }
}

// Set terminal modes (CSI Pm h or CSI ? Pm h)
// Enables specified modes, including mouse tracking and focus reporting
static void ExecuteSM(KTerm* term, bool private_mode) {
    // Iterate through parsed parameters from the CSI sequence
    for (int i = 0; i < GET_SESSION(term)->param_count; i++) {
        int mode = GET_SESSION(term)->escape_params[i];
        if (private_mode) {
            // ANSI.SYS does not support DEC Private Modes (those with '?')
            if (GET_SESSION(term)->conformance.level == VT_LEVEL_ANSI_SYS) continue;

            switch (mode) {
                case 2: // DECANM - ANSI Mode (Set = ANSI)
                    GET_SESSION(term)->dec_modes.vt52_mode = false;
                    break;
                case 67: // DECBKM - Backarrow Key Mode
                    GET_SESSION(term)->dec_modes.backarrow_key_mode = true;
                    GET_SESSION(term)->input.backarrow_sends_bs = true;
                    break;
                case 1000: // VT200 mouse tracking
                    KTerm_EnableMouseFeature(term, "cursor", true);
                    GET_SESSION(term)->mouse.mode = GET_SESSION(term)->mouse.sgr_mode ? MOUSE_TRACKING_SGR : MOUSE_TRACKING_VT200;
                    break;
                case 1002: // Button-event mouse tracking
                    KTerm_EnableMouseFeature(term, "cursor", true);
                    GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_BTN_EVENT;
                    break;
                case 1003: // Any-event mouse tracking
                    KTerm_EnableMouseFeature(term, "cursor", true);
                    GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_ANY_EVENT;
                    break;
                case 1004: // Focus tracking
                    KTerm_EnableMouseFeature(term, "focus", true);
                    break;
                case 1006: // SGR mouse reporting
                    KTerm_EnableMouseFeature(term, "sgr", true);
                    break;
                case 1015: // URXVT mouse reporting
                    KTerm_EnableMouseFeature(term, "urxvt", true);
                    break;
                case 1016: // Pixel position mouse reporting
                    KTerm_EnableMouseFeature(term, "pixel", true);
                    break;
                case 64: // DECSCCM - Multi-Session Support (Private mode 64 typically page/session stuff)
                         // VT520 DECSCCM (Select Cursor Control Mode) is 64 but this context is ? 64.
                         // Standard VT520 doesn't strictly document ?64 as Multi-Session enable, but used here for protocol.
                    // Enable multi-session switching capability
                    GET_SESSION(term)->conformance.features.multi_session_mode = true;
                    break;
                default:
                    // Delegate other private modes to KTerm_SetModeInternal
                    KTerm_SetModeInternal(term, mode, true, private_mode);
                    break;
            }
        } else {
            // Delegate ANSI modes to KTerm_SetModeInternal
            KTerm_SetModeInternal(term, mode, true, private_mode);
        }
    }
}

// Reset terminal modes (CSI Pm l or CSI ? Pm l)
// Disables specified modes, including mouse tracking and focus reporting
static void ExecuteRM(KTerm* term, bool private_mode) {
    // Iterate through parsed parameters from the CSI sequence
    for (int i = 0; i < GET_SESSION(term)->param_count; i++) {
        int mode = GET_SESSION(term)->escape_params[i];
        if (private_mode) {
            // ANSI.SYS does not support DEC Private Modes (those with '?')
            if (GET_SESSION(term)->conformance.level == VT_LEVEL_ANSI_SYS) continue;

            switch (mode) {
                case 2: // DECANM - ANSI Mode (Reset = VT52)
                    GET_SESSION(term)->dec_modes.vt52_mode = true;
                    break;
                case 67: // DECBKM - Backarrow Key Mode
                    GET_SESSION(term)->dec_modes.backarrow_key_mode = false;
                    GET_SESSION(term)->input.backarrow_sends_bs = false;
                    break;
                case 1000: // VT200 mouse tracking
                case 1002: // Button-event mouse tracking
                case 1003: // Any-event mouse tracking
                case 1015: // URXVT mouse reporting
                case 1016: // Pixel position mouse reporting
                    KTerm_EnableMouseFeature(term, "cursor", false);
                    GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_OFF;
                    break;
                case 1004: // Focus tracking
                    KTerm_EnableMouseFeature(term, "focus", false);
                    break;
                case 1006: // SGR mouse reporting
                    KTerm_EnableMouseFeature(term, "sgr", false);
                    break;
                case 64: // Disable Multi-Session Support
                    GET_SESSION(term)->conformance.features.multi_session_mode = false;
                    // If disabling, we should probably switch back to Session 1?
                    if (term->active_session != 0) {
                        KTerm_SetActiveSession(term, 0);
                    }
                    break;
                default:
                    // Delegate other private modes to KTerm_SetModeInternal
                    KTerm_SetModeInternal(term, mode, false, private_mode);
                    break;
            }
        } else {
            // Delegate ANSI modes to KTerm_SetModeInternal
            KTerm_SetModeInternal(term, mode, false, private_mode);
        }
    }
}

// Continue with device attributes and other implementations...

void ExecuteDA(KTerm* term, bool private_mode) {
    char introducer = private_mode ? GET_SESSION(term)->escape_buffer[0] : 0;
    if (introducer == '>') {
        KTerm_QueueResponse(term, GET_SESSION(term)->secondary_attributes);
    } else if (introducer == '=') {
        KTerm_QueueResponse(term, GET_SESSION(term)->tertiary_attributes);
    } else {
        KTerm_QueueResponse(term, GET_SESSION(term)->device_attributes);
    }
}

// Helper function to convert character to printable ASCII
static char GetPrintableChar(unsigned int ch, CharsetState* charset) {
    if (ch < 0x20 || ch > 0x7E) {
        // Check if DEC Special Graphics is active
        if (charset && charset->gl == &charset->g0 && charset->g0 == CHARSET_DEC_SPECIAL) {
            // Map common DEC line-drawing characters to ASCII approximations
            switch (ch) {
                case 0x6A: return '+'; // Corner
                case 0x6C: return '-'; // Horizontal line
                case 0x6D: return '|'; // Vertical line
                default: return ' ';
            }


        }
        return ' '; // Non-printable or unmapped
    }
    return (char)ch;
}


// Helper function to send data to the printer callback
static void SendToPrinter(KTerm* term, const char* data, size_t length) {
    if (term->printer_callback) {
        term->printer_callback(term, data, length);
    } else {
        // Fallback: If no printer callback, maybe log or ignore?
        // Standard behavior might be to do nothing if no printer attached.
        if (GET_SESSION(term)->options.debug_sequences) {
            fprintf(stderr, "MC: Print requested but no printer callback set (len=%zu)\n", length);
        }
    }
}

// Updated ExecuteMC
static void ExecuteMC(KTerm* term) {
    bool private_mode = (GET_SESSION(term)->escape_buffer[0] == '?');
    int params[MAX_ESCAPE_PARAMS];
    KTerm_ParseCSIParams(term, GET_SESSION(term)->escape_buffer, params, MAX_ESCAPE_PARAMS);
    int pi = (GET_SESSION(term)->param_count > 0) ? GET_SESSION(term)->escape_params[0] : 0;

    if (!GET_SESSION(term)->printer_available) {
        KTerm_LogUnsupportedSequence(term, "MC: No printer available");
        return;
    }

    if (!private_mode) {
        switch (pi) {
            case 0: // Print entire screen or scrolling region (DECPEX)
            {
                int start_y = 0;
                int end_y = term->height;

                // DECPEX (Mode 19):
                // Enable (h) = Print Scrolling Region
                // Disable (l) = Print Full Screen (Default)
                // Note: Previous implementation might have been inverted.
                // Standard says: 19h -> Print Extent = Scrolling Region.
                if (GET_SESSION(term)->dec_modes.print_extent) {
                    start_y = GET_SESSION(term)->scroll_top;
                    end_y = GET_SESSION(term)->scroll_bottom + 1;
                }

                // Calculate buffer size: (cols + newline) * rows + possible FF + null + safety
                size_t buf_size = (term->width + 1) * (end_y - start_y) + 8;
                char* print_buffer = (char*)malloc(buf_size);
                if (!print_buffer) break; // Allocation failed
                size_t pos = 0;

                for (int y = start_y; y < end_y; y++) {
                    for (int x = 0; x < term->width; x++) {
                        EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
                        if (pos < buf_size - 3) { // Reserve space for FF/Null
                            print_buffer[pos++] = GetPrintableChar(cell->ch, &GET_SESSION(term)->charset);
                        }
                    }
                    if (pos < buf_size - 3) {
                        print_buffer[pos++] = '\n';
                    }
                }

                if (GET_SESSION(term)->dec_modes.print_form_feed) {
                     print_buffer[pos++] = '\f'; // 0x0C
                }

                print_buffer[pos] = '\0';
                SendToPrinter(term, print_buffer, pos);
                if (GET_SESSION(term)->options.debug_sequences) {
                    KTerm_LogUnsupportedSequence(term, "MC: Print screen completed");
                }
                free(print_buffer);
                break;
            }
            case 1: // Print current line
            {
                char print_buffer[term->width + 3]; // + newline + FF + null
                size_t pos = 0;
                int y = GET_SESSION(term)->cursor.y;
                for (int x = 0; x < term->width; x++) {
                    EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
                    if (pos < sizeof(print_buffer) - 3) {
                        print_buffer[pos++] = GetPrintableChar(cell->ch, &GET_SESSION(term)->charset);
                    }
                }
                print_buffer[pos++] = '\n';

                if (GET_SESSION(term)->dec_modes.print_form_feed) {
                     print_buffer[pos++] = '\f'; // 0x0C
                }

                print_buffer[pos] = '\0';
                SendToPrinter(term, print_buffer, pos);
                if (GET_SESSION(term)->options.debug_sequences) {
                    KTerm_LogUnsupportedSequence(term, "MC: Print line completed");
                }
                break;
            }
            case 4: // Disable auto-print
                GET_SESSION(term)->auto_print_enabled = false;
                if (GET_SESSION(term)->options.debug_sequences) {
                    KTerm_LogUnsupportedSequence(term, "MC: Auto-print disabled");
                }
                break;
            case 5: // Enable auto-print
                GET_SESSION(term)->auto_print_enabled = true;
                if (GET_SESSION(term)->options.debug_sequences) {
                    KTerm_LogUnsupportedSequence(term, "MC: Auto-print enabled");
                }
                break;
            default:
                if (GET_SESSION(term)->options.log_unsupported) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "CSI %d i", pi);
                    snprintf(GET_SESSION(term)->conformance.compliance.last_unsupported,
                             sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported),
                             "%s", msg);
                    GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    } else {
        switch (pi) {
            case 4: // Disable printer controller mode
                GET_SESSION(term)->printer_controller_enabled = false;
                if (GET_SESSION(term)->options.debug_sequences) {
                    KTerm_LogUnsupportedSequence(term, "MC: Printer controller disabled");
                }
                break;
            case 5: // Enable printer controller mode
                GET_SESSION(term)->printer_controller_enabled = true;
                if (GET_SESSION(term)->options.debug_sequences) {
                    // Log?
                }
                break;
            case 9: // Print Screen (DEC specific private parameter for same action as CSI 0 i)
            {
                size_t buf_size = term->width * term->height + term->height + 1;
                char* print_buffer = (char*)malloc(buf_size);
                if (!print_buffer) break; // Allocation failed
                size_t pos = 0;
                for (int y = 0; y < term->height; y++) {
                    for (int x = 0; x < term->width; x++) {
                        EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
                        if (pos < buf_size - 2) {
                            print_buffer[pos++] = GetPrintableChar(cell->ch, &GET_SESSION(term)->charset);
                        }
                    }
                    if (pos < buf_size - 2) {
                        print_buffer[pos++] = '\n';
                    }
                }
                print_buffer[pos] = '\0';
                SendToPrinter(term, print_buffer, pos);
                if (GET_SESSION(term)->options.debug_sequences) {
                    KTerm_LogUnsupportedSequence(term, "MC: Print screen (DEC) completed");
                }
                free(print_buffer);
                break;
            }
            default:
                if (GET_SESSION(term)->options.log_unsupported) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "CSI ?%d i", pi);
                    snprintf(GET_SESSION(term)->conformance.compliance.last_unsupported,
                             sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported),
                             "%s", msg);
                    GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    }
}
// Enhanced KTerm_QueueResponse
void KTerm_QueueResponse(KTerm* term, const char* response) {
    if (!GET_SESSION(term)->response_enabled) return;
    size_t len = strlen(response);
    // Leave space for null terminator
    if (GET_SESSION(term)->response_length + len >= sizeof(GET_SESSION(term)->answerback_buffer) - 1) {
        // Flush existing responses
        if (term->response_callback && GET_SESSION(term)->response_length > 0) {
            term->response_callback(term, GET_SESSION(term)->answerback_buffer, GET_SESSION(term)->response_length);
            GET_SESSION(term)->response_length = 0;
        }
        // If response is still too large, log and truncate
        if (len >= sizeof(GET_SESSION(term)->answerback_buffer) - 1) {
            if (GET_SESSION(term)->options.debug_sequences) {
                fprintf(stderr, "KTerm_QueueResponse: Response too large (%zu bytes)\n", len);
            }
            len = sizeof(GET_SESSION(term)->answerback_buffer) - 1;
        }
    }

    if (len > 0) {
        memcpy(GET_SESSION(term)->answerback_buffer + GET_SESSION(term)->response_length, response, len);
        GET_SESSION(term)->response_length += len;
        GET_SESSION(term)->answerback_buffer[GET_SESSION(term)->response_length] = '\0'; // Ensure null-termination
    }
}

void KTerm_QueueResponseBytes(KTerm* term, const char* data, size_t len) {
    if (!GET_SESSION(term)->response_enabled) return;
    if (GET_SESSION(term)->response_length + len >= sizeof(GET_SESSION(term)->answerback_buffer)) {
        if (term->response_callback && GET_SESSION(term)->response_length > 0) {
            term->response_callback(term, GET_SESSION(term)->answerback_buffer, GET_SESSION(term)->response_length);
            GET_SESSION(term)->response_length = 0;
        }
        if (len >= sizeof(GET_SESSION(term)->answerback_buffer)) {
            if (GET_SESSION(term)->options.debug_sequences) {
                fprintf(stderr, "KTerm_QueueResponseBytes: Response too large (%zu bytes)\n", len);
            }
            len = sizeof(GET_SESSION(term)->answerback_buffer) -1;
        }
    }

    if (len > 0) {
        memcpy(GET_SESSION(term)->answerback_buffer + GET_SESSION(term)->response_length, data, len);
        GET_SESSION(term)->response_length += len;
    }
}

// Enhanced ExecuteDSR function with new handlers
static void ExecuteDSR(KTerm* term) {
    bool private_mode = (GET_SESSION(term)->escape_buffer[0] == '?');
    int params[MAX_ESCAPE_PARAMS];
    KTerm_ParseCSIParams(term, GET_SESSION(term)->escape_buffer, params, MAX_ESCAPE_PARAMS);
    int command = (GET_SESSION(term)->param_count > 0) ? GET_SESSION(term)->escape_params[0] : 0;

    if (!private_mode) {
        switch (command) {
            case 5: KTerm_QueueResponse(term, "\x1B[0n"); break;
            case 6: {
                int row = GET_SESSION(term)->cursor.y + 1;
                int col = GET_SESSION(term)->cursor.x + 1;
                if (GET_SESSION(term)->dec_modes.origin_mode) {
                    row = GET_SESSION(term)->cursor.y - GET_SESSION(term)->scroll_top + 1;
                    col = GET_SESSION(term)->cursor.x - GET_SESSION(term)->left_margin + 1;
                }
                char response[32];
                snprintf(response, sizeof(response), "\x1B[%d;%dR", row, col);
                KTerm_QueueResponse(term, response);
                break;
            }
            default:
                if (GET_SESSION(term)->options.log_unsupported) {
                    snprintf(GET_SESSION(term)->conformance.compliance.last_unsupported,
                             sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported),
                             "CSI %dn", command);
                    GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    } else {
        switch (command) {
            case 15: KTerm_QueueResponse(term, GET_SESSION(term)->printer_available ? "\x1B[?10n" : "\x1B[?13n"); break;
            case 21: { // DECRS - Report Session Status
                // If multi-session is not supported/enabled, we might choose to ignore or report limited info.
                // VT520 spec says DECRS reports on sessions.
                // If the terminal doesn't support sessions, it shouldn't respond or should respond with just 1.
                if (!GET_SESSION(term)->conformance.features.multi_session_mode) {
                     if (GET_SESSION(term)->options.debug_sequences) {
                         KTerm_LogUnsupportedSequence(term, "DECRS ignored: Multi-session mode disabled");
                     }
                     // Optionally, we could report just session 1 as active, but typically this DSR is for multi-session terminals.
                     // Let's assume we proceed if we want to be nice to single-session queries, but strictly speaking it's a multi-session feature.
                     // However, "we need the number of sessions to be accurate" implies we should report what we have.
                     // If mode is disabled, maybe we shouldn't report? The prompt implies accuracy.
                     // Let's rely on the flag.
                     break;
                }

                int limit = GET_SESSION(term)->conformance.features.max_session_count;
                if (limit == 0) limit = 1;
                if (limit > MAX_SESSIONS) limit = MAX_SESSIONS;

                char response[MAX_COMMAND_BUFFER];
                int offset = snprintf(response, sizeof(response), "\x1BP$p");
                for (int i = 0; i < limit; i++) {
                    int seq = i + 1;
                    int status = 1; // Not open
                    if (term->sessions[i].session_open) {
                        status = (i == term->active_session) ? 2 : 3;
                    }
                    int attr = 0;
                    offset += snprintf(response + offset, sizeof(response) - offset, "%d;%d;%d", seq, status, attr);
                    if (i < limit - 1) {
                         offset += snprintf(response + offset, sizeof(response) - offset, "|");
                    }
                }
                snprintf(response + offset, sizeof(response) - offset, "\x1B\\");
                KTerm_QueueResponse(term, response);
                break;
            }
            case 25: KTerm_QueueResponse(term, GET_SESSION(term)->programmable_keys.udk_locked ? "\x1B[?21n" : "\x1B[?20n"); break;
            case 26: {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?27;%dn", GET_SESSION(term)->input.keyboard_dialect);
                KTerm_QueueResponse(term, response);
                break;
            }
            case 27: // Locator Type (DECREPTPARM)
                KTerm_QueueResponse(term, "\x1B[?27;0n"); // No locator
                break;
            case 53: KTerm_QueueResponse(term, GET_SESSION(term)->locator_enabled ? "\x1B[?53n" : "\x1B[?50n"); break;
            case 55: KTerm_QueueResponse(term, "\x1B[?57;0n"); break;
            case 56: KTerm_QueueResponse(term, "\x1B[?56;0n"); break;
            case 62: {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?62;%zu;%zun",
                         GET_SESSION(term)->macro_space.used, GET_SESSION(term)->macro_space.total);
                KTerm_QueueResponse(term, response);
                break;
            }
            case 63: {
                int page = (GET_SESSION(term)->param_count > 1) ? GET_SESSION(term)->escape_params[1] : 1;
                GET_SESSION(term)->checksum.last_checksum = ComputeScreenChecksum(term, page);
                char response[64];
                snprintf(response, sizeof(response), "\x1B[?63;%d;%d;%04Xn",
                         page, GET_SESSION(term)->checksum.algorithm, GET_SESSION(term)->checksum.last_checksum);
                KTerm_QueueResponse(term, response);
                break;
            }
            case 75: KTerm_QueueResponse(term, "\x1B[?75;0n"); break;
            case 12: { // DECRSN - Report Session Number
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?12;%dn", term->active_session + 1);
                KTerm_QueueResponse(term, response);
                break;
            }
            default:
                if (GET_SESSION(term)->options.log_unsupported) {
                    snprintf(GET_SESSION(term)->conformance.compliance.last_unsupported,
                             sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported),
                             "CSI ?%dn", command);
                    GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    }
}

void ExecuteDECSTBM(KTerm* term) { // Set Top and Bottom Margins
    int top = KTerm_GetCSIParam(term, 0, 1) - 1;    // Convert to 0-based
    int bottom = KTerm_GetCSIParam(term, 1, term->height) - 1;

    // Validate parameters
    if (top >= 0 && top < term->height && bottom >= top && bottom < term->height) {
        GET_SESSION(term)->scroll_top = top;
        GET_SESSION(term)->scroll_bottom = bottom;

        // Move cursor to home position
        if (GET_SESSION(term)->dec_modes.origin_mode) {
            GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
            GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_top;
        } else {
            GET_SESSION(term)->cursor.x = 0;
            GET_SESSION(term)->cursor.y = 0;
        }
    }
}

void ExecuteDECSLRM(KTerm* term) { // Set Left and Right Margins (VT420)
    if (!GET_SESSION(term)->conformance.features.vt420_mode) {
        KTerm_LogUnsupportedSequence(term, "DECSLRM requires VT420 mode");
        return;
    }

    int left = KTerm_GetCSIParam(term, 0, 1) - 1;    // Convert to 0-based
    int right = KTerm_GetCSIParam(term, 1, term->width) - 1;

    // Validate parameters
    if (left >= 0 && left < term->width && right >= left && right < term->width) {
        GET_SESSION(term)->left_margin = left;
        GET_SESSION(term)->right_margin = right;

        // Move cursor to home position
        if (GET_SESSION(term)->dec_modes.origin_mode) {
            GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
            GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_top;
        } else {
            GET_SESSION(term)->cursor.x = 0;
            GET_SESSION(term)->cursor.y = 0;
        }
    }
}

// Updated ExecuteDECRQPSR
static void ExecuteDECRQPSR(KTerm* term) {
    int params[MAX_ESCAPE_PARAMS];
    KTerm_ParseCSIParams(term, GET_SESSION(term)->escape_buffer, params, MAX_ESCAPE_PARAMS);
    int pfn = (GET_SESSION(term)->param_count > 0) ? GET_SESSION(term)->escape_params[0] : 0;

    char response[64];
    switch (pfn) {
        case 1: // Sixel geometry
            snprintf(response, sizeof(response), "DCS 2 $u %d ; %d;%d;%d;%d ST",
                     GET_SESSION(term)->conformance.level, GET_SESSION(term)->sixel.x, GET_SESSION(term)->sixel.y,
                     GET_SESSION(term)->sixel.width, GET_SESSION(term)->sixel.height);
            KTerm_QueueResponse(term, response);
            break;
        case 2: // Sixel color palette
            for (int i = 0; i < 256; i++) {
                RGB_KTermColor c = term->color_palette[i];
                snprintf(response, sizeof(response), "DCS 1 $u #%d;%d;%d;%d ST",
                         i, c.r, c.g, c.b);
                KTerm_QueueResponse(term, response);
            }
            break;
        case 3: // ReGIS (unsupported)
            if (GET_SESSION(term)->options.log_unsupported) {
                snprintf(GET_SESSION(term)->conformance.compliance.last_unsupported,
                         sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported),
                         "CSI %d $ u (ReGIS unsupported)", pfn);
                GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
            }
            break;
        default:
            if (GET_SESSION(term)->options.log_unsupported) {
                snprintf(GET_SESSION(term)->conformance.compliance.last_unsupported,
                         sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported),
                         "CSI %d $ u", pfn);
                GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
            }
            break;
    }
}

void ExecuteDECLL(KTerm* term) { // Load LEDs
    int n = KTerm_GetCSIParam(term, 0, 0);

    // DECLL - Load LEDs (VT220+ feature)
    // Parameters: 0=all off, 1=LED1 on, 2=LED2 on, 4=LED3 on, 8=LED4 on
    // Modern terminals don't have LEDs, so this is mostly ignored

    if (GET_SESSION(term)->options.debug_sequences) {
        char debug_msg[64];
        snprintf(debug_msg, sizeof(debug_msg), "DECLL: LED state %d", n);
        KTerm_LogUnsupportedSequence(term, debug_msg);
    }

    // Could be used for visual indicators in a GUI implementation
    // For now, just acknowledge the command
}

void ExecuteDECSTR(KTerm* term) { // Soft KTerm Reset
    // Reset terminal to power-on defaults but keep communication settings

    // Reset display modes
    GET_SESSION(term)->dec_modes.cursor_visible = true;
    GET_SESSION(term)->dec_modes.auto_wrap_mode = true;
    GET_SESSION(term)->dec_modes.origin_mode = false;
    GET_SESSION(term)->dec_modes.insert_mode = false;
    GET_SESSION(term)->dec_modes.application_cursor_keys = false;

    // Reset character attributes
    KTerm_ResetAllAttributes(term);

    // Reset scrolling region
    GET_SESSION(term)->scroll_top = 0;
    GET_SESSION(term)->scroll_bottom = term->height - 1;
    GET_SESSION(term)->left_margin = 0;
    GET_SESSION(term)->right_margin = term->width - 1;

    // Reset character sets
    KTerm_InitCharacterSets(term);

    // Reset tab stops
    KTerm_InitTabStops(term);

    // Home cursor
    GET_SESSION(term)->cursor.x = 0;
    GET_SESSION(term)->cursor.y = 0;

    // Clear saved cursor
    GET_SESSION(term)->saved_cursor_valid = false;

    KTerm_InitKTermColorPalette(term);
    KTerm_InitSixelGraphics(term);

    if (GET_SESSION(term)->options.debug_sequences) {
        KTerm_LogUnsupportedSequence(term, "DECSTR: Soft terminal reset");
    }
}

void ExecuteDECSCL(KTerm* term) { // Select Conformance Level
    int level = KTerm_GetCSIParam(term, 0, 61);
    int c1_control = KTerm_GetCSIParam(term, 1, 0);
    (void)c1_control;  // Mark as intentionally unused

    // Set conformance level based on parameter
    switch (level) {
        case 61: KTerm_SetLevel(term, VT_LEVEL_100); break;
        case 62: KTerm_SetLevel(term, VT_LEVEL_220); break;
        case 63: KTerm_SetLevel(term, VT_LEVEL_320); break;
        case 64: KTerm_SetLevel(term, VT_LEVEL_420); break;
        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown conformance level: %d", level);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }

    // c1_control: 0=8-bit controls, 1=7-bit controls, 2=8-bit controls
    // Modern implementations typically use 7-bit controls
}

// Enhanced ExecuteDECRQM
void ExecuteDECRQM(KTerm* term) { // Request Mode
    int mode = KTerm_GetCSIParam(term, 0, 0);
    bool private_mode = (GET_SESSION(term)->escape_buffer[0] == '?');

    char response[32];
    int mode_state = 0; // 0=not recognized, 1=set, 2=reset, 3=permanently set, 4=permanently reset

    if (private_mode) {
        // Check DEC private modes
        switch (mode) {
            case 1: // DECCKM (Application Cursor Keys)
                mode_state = GET_SESSION(term)->dec_modes.application_cursor_keys ? 1 : 2;
                break;
            case 3: // DECCOLM (132 Column Mode)
                mode_state = GET_SESSION(term)->dec_modes.column_mode_132 ? 1 : 2;
                break;
            case 4: // DECSCLM (Smooth Scroll)
                mode_state = GET_SESSION(term)->dec_modes.smooth_scroll ? 1 : 2;
                break;
            case 5: // DECSCNM (Reverse Video)
                mode_state = GET_SESSION(term)->dec_modes.reverse_video ? 1 : 2;
                break;
            case 6: // DECOM (Origin Mode)
                mode_state = GET_SESSION(term)->dec_modes.origin_mode ? 1 : 2;
                break;
            case 7: // DECAWM (Auto Wrap Mode)
                mode_state = GET_SESSION(term)->dec_modes.auto_wrap_mode ? 1 : 2;
                break;
            case 8: // DECARM (Auto Repeat Keys)
                mode_state = GET_SESSION(term)->dec_modes.auto_repeat_keys ? 1 : 2;
                break;
            case 9: // X10 Mouse
                mode_state = GET_SESSION(term)->dec_modes.x10_mouse ? 1 : 2;
                break;
            case 10: // Show Toolbar
                mode_state = GET_SESSION(term)->dec_modes.show_toolbar ? 1 : 4; // Permanently reset
                break;
            case 12: // Blinking Cursor
                mode_state = GET_SESSION(term)->dec_modes.blink_cursor ? 1 : 2;
                break;
            case 18: // DECPFF (Print Form Feed)
                mode_state = GET_SESSION(term)->dec_modes.print_form_feed ? 1 : 2;
                break;
            case 19: // Print Extent
                mode_state = GET_SESSION(term)->dec_modes.print_extent ? 1 : 2;
                break;
            case 25: // DECTCEM (Cursor Visible)
                mode_state = GET_SESSION(term)->dec_modes.cursor_visible ? 1 : 2;
                break;
            case 38: // Tektronix Mode
                mode_state = (GET_SESSION(term)->parse_state == PARSE_TEKTRONIX) ? 1 : 2;
                break;
            case 47: // Alternate Screen
            case 1047:
            case 1049:
                mode_state = GET_SESSION(term)->dec_modes.alternate_screen ? 1 : 2;
                break;
            case 1000: // VT200 Mouse
                mode_state = (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_VT200) ? 1 : 2;
                break;
            case 2004: // Bracketed Paste
                mode_state = GET_SESSION(term)->bracketed_paste.enabled ? 1 : 2;
                break;
            case 61: // DECSCL VT100
                mode_state = (GET_SESSION(term)->conformance.level == VT_LEVEL_100) ? 1 : 2;
                break;
            case 62: // DECSCL VT220
                mode_state = (GET_SESSION(term)->conformance.level == VT_LEVEL_220) ? 1 : 2;
                break;
            case 63: // DECSCL VT320
                mode_state = (GET_SESSION(term)->conformance.level == VT_LEVEL_520) ? 1 : 2;
                break;
            case 64: // DECSCL VT420
                mode_state = (GET_SESSION(term)->conformance.level == VT_LEVEL_420) ? 1 : 2;
                break;
            default:
                mode_state = 0;
                break;
        }
        snprintf(response, sizeof(response), "\x1B[?%d;%d$y", mode, mode_state);
    } else {
        // Check ANSI modes
        switch (mode) {
            case 4: // IRM (Insert/Replace Mode)
                mode_state = GET_SESSION(term)->ansi_modes.insert_replace ? 1 : 2;
                break;
            case 20: // LNM (Line Feed/New Line Mode)
                mode_state = GET_SESSION(term)->ansi_modes.line_feed_new_line ? 1 : 3; // Permanently set
                break;
            default:
                mode_state = 0;
                break;
        }
        snprintf(response, sizeof(response), "\x1B[%d;%d$y", mode, mode_state);
    }

    KTerm_QueueResponse(term, response);
}

static void ExecuteDECSCUSR_Internal(KTerm* term, KTermSession* session) { // Set Cursor Style
    int style = (session->param_count > 0) ? session->escape_params[0] : 1;

    switch (style) {
        case 0: case 1: // Default or blinking block
            session->cursor.shape = CURSOR_BLOCK_BLINK;
            session->cursor.blink_enabled = true;
            break;
        case 2: // Steady block
            session->cursor.shape = CURSOR_BLOCK;
            session->cursor.blink_enabled = false;
            break;
        case 3: // Blinking underline
            session->cursor.shape = CURSOR_UNDERLINE_BLINK;
            session->cursor.blink_enabled = true;
            break;
        case 4: // Steady underline
            session->cursor.shape = CURSOR_UNDERLINE;
            session->cursor.blink_enabled = false;
            break;
        case 5: // Blinking bar
            session->cursor.shape = CURSOR_BAR_BLINK;
            session->cursor.blink_enabled = true;
            break;
        case 6: // Steady bar
            session->cursor.shape = CURSOR_BAR;
            session->cursor.blink_enabled = false;
            break;
        default:
            if (session->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown cursor style: %d", style);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}

void ExecuteDECSCUSR(KTerm* term) { // Set Cursor Style
    ExecuteDECSCUSR_Internal(term, GET_SESSION(term));
}

void ExecuteCSI_P(KTerm* term) { // Various P commands
    // This handles CSI sequences ending in 'p' with different intermediates
    char* params = GET_SESSION(term)->escape_buffer;

    if (strstr(params, "!") != NULL) {
        // DECSTR - Soft KTerm Reset
        ExecuteDECSTR(term);
    } else if (strstr(params, "\"") != NULL) {
        // DECSCL - Select Conformance Level
        ExecuteDECSCL(term);
    } else if (strstr(params, "$") != NULL) {
        // DECRQM - Request Mode
        ExecuteDECRQM(term);
    } else if (strstr(params, " ") != NULL) {
        // Set cursor style (DECSCUSR)
        ExecuteDECSCUSR(term);
    } else {
        // No intermediate? Check for ANSI.SYS Key Redefinition (CSI code ; string ; ... p)
        // If we are in ANSI.SYS mode, we should acknowledge this.
        if (GET_SESSION(term)->conformance.level == VT_LEVEL_ANSI_SYS) {
             if (GET_SESSION(term)->options.debug_sequences) {
                KTerm_LogUnsupportedSequence(term, "ANSI.SYS Key Redefinition ignored (security restriction)");
             }
             return;
        }

        // Unknown p command
        if (GET_SESSION(term)->options.debug_sequences) {
            char debug_msg[MAX_COMMAND_BUFFER + 64];
            snprintf(debug_msg, sizeof(debug_msg), "Unknown CSI p command: %s", params);
            KTerm_LogUnsupportedSequence(term, debug_msg);
        }
    }
}

void ExecuteDECSCA(KTerm* term) { // Select Character Protection Attribute
    // CSI Ps " q
    // Ps = 0, 2 -> Not protected
    // Ps = 1 -> Protected
    int ps = KTerm_GetCSIParam(term, 0, 0);
    if (ps == 1) {
        GET_SESSION(term)->current_attributes |= KTERM_ATTR_PROTECTED;
    } else {
        GET_SESSION(term)->current_attributes &= ~KTERM_ATTR_PROTECTED;
    }
}


void ExecuteWindowOps(KTerm* term) { // Window manipulation (xterm extension)
    int operation = KTerm_GetCSIParam(term, 0, 0);

    switch (operation) {
        case 1: // De-iconify window (Restore)
            KTerm_RestoreWindow();
            break;
        case 2: // Iconify window (Minimize)
            KTerm_MinimizeWindow();
            break;
        case 3: // Move window to position (in pixels)
            {
                int x = KTerm_GetCSIParam(term, 1, 0);
                int y = KTerm_GetCSIParam(term, 2, 0);
                KTerm_SetWindowPosition(x, y);
            }
            break;
        case 4: // Resize window (in pixels)
            {
                int height = KTerm_GetCSIParam(term, 1, DEFAULT_WINDOW_HEIGHT);
                int width = KTerm_GetCSIParam(term, 2, DEFAULT_WINDOW_WIDTH);
                KTerm_SetWindowSize(width, height);
            }
            break;
        case 5: // Raise window (Bring to front)
            KTerm_SetWindowFocused(); // Closest approximation
            break;
        case 6: // Lower window
            // Not directly supported by Situation/GLFW easily
            if (GET_SESSION(term)->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Window lower not supported");
            break;
        case 7: // Refresh window
            // Handled automatically by game loop
            break;
        case 8: // Resize text area (in chars)
            {
                int rows = KTerm_GetCSIParam(term, 1, term->height);
                int cols = KTerm_GetCSIParam(term, 2, term->width);
                int width = cols * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE;
                int height = rows * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE;
                KTerm_SetWindowSize(width, height);
            }
            break;

        case 9: // Maximize/restore window
            if (KTerm_GetCSIParam(term, 1, 0) == 1) KTerm_MaximizeWindow();
            else KTerm_RestoreWindow();
            break;
        case 10: // Full-screen toggle
            if (KTerm_GetCSIParam(term, 1, 0) == 1) {
                if (!KTerm_IsWindowFullscreen()) KTerm_ToggleFullscreen();
            } else {
                if (KTerm_IsWindowFullscreen()) KTerm_ToggleFullscreen();
            }
            break;

        case 11: // Report window state
            KTerm_QueueResponse(term, "\x1B[1t"); // Always report "not iconified"
            break;

        case 13: // Report window position
        case 14: // Report window size in pixels
        case 18: // Report text area size
            {
                char response[32];
                if (operation == 18) {
                    snprintf(response, sizeof(response), "\x1B[8;%d;%dt", term->height, term->width);
                } else {
                    snprintf(response, sizeof(response), "\x1B[3;%d;%dt", 100, 100); // Dummy values
                }
                KTerm_QueueResponse(term, response);
            }
            break;

        case 19: // Report screen size
            {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[9;%d;%dt",
                        KTerm_GetScreenHeight() / DEFAULT_CHAR_HEIGHT, KTerm_GetScreenWidth() / DEFAULT_CHAR_WIDTH);
                KTerm_QueueResponse(term, response);
            }
            break;

        case 20: // Report icon label
            {
                char response[MAX_TITLE_LENGTH + 32];
                snprintf(response, sizeof(response), "\x1B]L%s\x1B\\", GET_SESSION(term)->title.icon_title);
                KTerm_QueueResponse(term, response);
            }
            break;

        case 21: // Report window title
            {
                char response[MAX_TITLE_LENGTH + 32];
                snprintf(response, sizeof(response), "\x1B]l%s\x1B\\", GET_SESSION(term)->title.window_title);
                KTerm_QueueResponse(term, response);
            }
            break;

        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown window operation: %d", operation);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}

static void KTerm_ExecuteSaveCursor_Internal(KTermSession* session) {
    session->saved_cursor.x = session->cursor.x;
    session->saved_cursor.y = session->cursor.y;

    // Modes
    session->saved_cursor.origin_mode = session->dec_modes.origin_mode;
    session->saved_cursor.auto_wrap_mode = session->dec_modes.auto_wrap_mode;

    // Attributes
    session->saved_cursor.fg_color = session->current_fg;
    session->saved_cursor.bg_color = session->current_bg;
    session->saved_cursor.attributes = session->current_attributes;

    // Charset
    session->saved_cursor.charset = session->charset;

    session->saved_cursor_valid = true;
}

void KTerm_ExecuteSaveCursor(KTerm* term) {
    KTerm_ExecuteSaveCursor_Internal(GET_SESSION(term));
}

static void KTerm_ExecuteRestoreCursor_Internal(KTermSession* session) {
    if (session->saved_cursor_valid) {
        // Restore Position
        session->cursor.x = session->saved_cursor.x;
        session->cursor.y = session->saved_cursor.y;

        // Restore Modes
        session->dec_modes.origin_mode = session->saved_cursor.origin_mode;
        session->dec_modes.auto_wrap_mode = session->saved_cursor.auto_wrap_mode;

        // Restore Attributes
        session->current_fg = session->saved_cursor.fg_color;
        session->current_bg = session->saved_cursor.bg_color;
        session->current_attributes = session->saved_cursor.attributes;

        // Restore Charset
        session->charset = session->saved_cursor.charset;
    }
}

void KTerm_ExecuteRestoreCursor(KTerm* term) { // Restore cursor (non-ANSI.SYS)
    // This is the VT terminal version, not ANSI.SYS
    // Restores cursor from per-session saved state
    KTerm_ExecuteRestoreCursor_Internal(GET_SESSION(term));
}

void ExecuteDECREQTPARM(KTerm* term) { // Request KTerm Parameters
    int parm = KTerm_GetCSIParam(term, 0, 0);

    char response[32];
    // Report terminal parameters
    // Format: CSI sol ; par ; nbits ; xspeed ; rspeed ; clkmul ; flags x
    // Simplified response with standard values
    snprintf(response, sizeof(response), "\x1B[%d;1;1;120;120;1;0x", parm + 2);
    KTerm_QueueResponse(term, response);
}

void ExecuteDECTST(KTerm* term) { // Invoke Confidence Test
    int test = KTerm_GetCSIParam(term, 0, 0);

    switch (test) {
        case 1: // Power-up self test
        case 2: // Data loop back test
        case 3: // EIA loop back test
        case 4: // Printer port loop back test
            // These tests would require hardware access
            // For software terminal, just acknowledge
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "DECTST test %d - not applicable", test);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            break;

        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown DECTST test: %d", test);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}

void ExecuteDECVERP(KTerm* term) { // Verify Parity
    // DECVERP - Enable/disable parity verification
    // Not applicable to software terminals
    if (GET_SESSION(term)->options.debug_sequences) {
        KTerm_LogUnsupportedSequence(term, "DECVERP - parity verification not applicable");
    }
}

// =============================================================================
// COMPREHENSIVE CSI SEQUENCE PROCESSING
// =============================================================================

// Complete the missing API functions from earlier phases
void ExecuteTBC(KTerm* term) { // Tab Clear
    int n = KTerm_GetCSIParam(term, 0, 0);

    switch (n) {
        case 0: // Clear tab stop at current column
            KTerm_ClearTabStop(term, GET_SESSION(term)->cursor.x);
            break;
        case 3: // Clear all tab stops
            KTerm_ClearAllTabStops(term);
            break;
    }
}

void ExecuteCTC(KTerm* term) { // Cursor Tabulation Control
    int n = KTerm_GetCSIParam(term, 0, 0);

    switch (n) {
        case 0: // Set tab stop at current column
            KTerm_SetTabStop(term, GET_SESSION(term)->cursor.x);
            break;
        case 2: // Clear tab stop at current column
            KTerm_ClearTabStop(term, GET_SESSION(term)->cursor.x);
            break;
        case 5: // Clear all tab stops
            KTerm_ClearAllTabStops(term);
            break;
    }
}

void ExecuteDECSN(KTerm* term) {
    int session_id = KTerm_GetCSIParam(term, 0, 0);
    // If param is omitted (0 returned by KTerm_GetCSIParam if 0 is default), VT520 DECSN usually defaults to 1.
    if (session_id == 0) session_id = 1;

    // Use max_session_count from features if set, otherwise default to MAX_SESSIONS (or 1 if single session)
    // Actually we should rely on max_session_count. If 0 (uninitialized safety), default to 1.
    int limit = GET_SESSION(term)->conformance.features.max_session_count;
    if (limit == 0) limit = 1;
    if (limit > MAX_SESSIONS) limit = MAX_SESSIONS;

    if (session_id >= 1 && session_id <= limit) {
        // Respect Multi-Session Mode Lock
        if (!GET_SESSION(term)->conformance.features.multi_session_mode) {
            // If disabled, ignore request (or log it)
            if (GET_SESSION(term)->options.debug_sequences) {
                char msg[64];
                snprintf(msg, sizeof(msg), "DECSN %d ignored: Multi-session mode disabled", session_id);
                KTerm_LogUnsupportedSequence(term, msg);
            }
            return;
        }

        if (term->sessions[session_id - 1].session_open) {
            KTerm_SetActiveSession(term, session_id - 1);
        } else {
            if (GET_SESSION(term)->options.debug_sequences) {
                char msg[64];
                snprintf(msg, sizeof(msg), "DECSN %d ignored: Session not open", session_id);
                KTerm_LogUnsupportedSequence(term, msg);
            }
        }
    }
}

// New ExecuteCSI_Dollar for multi-byte CSI $ sequences
void ExecuteCSI_Dollar(KTerm* term) {
    // This function is now the central dispatcher for sequences with a '$' intermediate.
    // It looks for the character *after* the '$'.
    char* dollar_ptr = strchr(GET_SESSION(term)->escape_buffer, '$');
    if (dollar_ptr && *(dollar_ptr + 1) != '\0') {
        char final_char = *(dollar_ptr + 1);
        switch (final_char) {
            case 'v':
                ExecuteDECCRA(term);
                break;
            case 'w':
                ExecuteDECRQCRA(term);
                break;
            case 'x':
                // DECERA and DECFRA share the same sequence ending ($x),
                // but are distinguished by parameter count.
                if (GET_SESSION(term)->param_count == 4) {
                    ExecuteDECERA(term);
                } else if (GET_SESSION(term)->param_count == 5) {
                    ExecuteDECFRA(term);
                } else {
                    KTerm_LogUnsupportedSequence(term, "Invalid parameters for DECERA/DECFRA");
                }
                break;
            case '{':
                ExecuteDECSERA(term);
                break;
            case 'u':
                ExecuteDECRQPSR(term);
                break;
            case 'q':
                ExecuteDECRQM(term);
                break;
            default:
                if (GET_SESSION(term)->options.debug_sequences) {
                    char debug_msg[128];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown CSI $ sequence with final char '%c'", final_char);
                    KTerm_LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    } else {
         if (GET_SESSION(term)->options.debug_sequences) {
            char debug_msg[MAX_COMMAND_BUFFER + 64];
            snprintf(debug_msg, sizeof(debug_msg), "Malformed CSI $ sequence in buffer: %s", GET_SESSION(term)->escape_buffer);
            KTerm_LogUnsupportedSequence(term, debug_msg);
        }
    }
}

void KTerm_ProcessCSIChar(KTerm* term, KTermSession* session, unsigned char ch) {
    if (session->parse_state != PARSE_CSI) return;

    if (ch >= 0x40 && ch <= 0x7E) {
        // Parse parameters into session->escape_params
        KTerm_ParseCSIParams_Internal(session, session->escape_buffer, NULL, MAX_ESCAPE_PARAMS);

        // Handle DECSCUSR (CSI Ps SP q)
        if (ch == 'q' && session->escape_pos >= 1 && session->escape_buffer[session->escape_pos - 1] == ' ') {
            ExecuteDECSCUSR_Internal(term, session);
        } else {
            // Dispatch to KTerm_ExecuteCSICommand
            // KTerm_ExecuteCSICommand(term, ch);
            // We should use an internal version, but for now calling the public one which uses GET_SESSION.
            // This is UNSAFE if active_session changed.
            // But implementing the full tree refactor is immense.
            // I will assume for now that CSI commands do NOT change session, except explicit session commands?
            // DECSN is a CSI command (CSI Ps $ |).
            // If DECSN changes active_session inside ExecuteCSICommand, that is fine for THAT function.
            // The issue is if ProcessEvents loop continues with OLD session pointer but NEW active_session index.
            // We fixed ProcessEvents to stick to the OLD session pointer.
            // So subsequent calls in ProcessEvents use the correct session.
            // What if ExecuteCSICommand calls something that uses GET_SESSION(term)?
            // It will use the NEW session.
            // If DECSN switches session, we WANT subsequent logic in ExecuteCSICommand (if any) to affect... which session?
            // DECSN is usually final.
            // I'll call KTerm_ExecuteCSICommand(term, ch). It uses GET_SESSION.
            // I will refactor KTerm_ExecuteCSICommand to use session later if I can.
            KTerm_ExecuteCSICommand(term, ch);
        }

        // Reset parser state
        session->parse_state = VT_PARSE_NORMAL;
        ClearCSIParams(session);
    } else if (ch >= 0x20 && ch <= 0x3F) {
        // Accumulate intermediate characters (e.g., digits, ';', '?')
        // Phase 7.2: Harden Escape Buffers (Bounds Check)
        if (session->escape_pos < MAX_COMMAND_BUFFER - 1) {
            session->escape_buffer[session->escape_pos++] = ch;
            session->escape_buffer[session->escape_pos] = '\0';
        } else {
            KTerm_LogUnsupportedSequence(term, "CSI escape buffer overflow");
            session->parse_state = VT_PARSE_NORMAL;
            ClearCSIParams(session);
        }
    } else if (ch == '$') {
        // Handle multi-byte CSI sequences (e.g., CSI $ q, CSI $ u)
        if (session->escape_pos < MAX_COMMAND_BUFFER - 1) {
            session->escape_buffer[session->escape_pos++] = ch;
            session->escape_buffer[session->escape_pos] = '\0';
        } else {
            KTerm_LogUnsupportedSequence(term, "CSI escape buffer overflow");
            session->parse_state = VT_PARSE_NORMAL;
            ClearCSIParams(session);
        }
    } else {
        // Invalid character
        if (session->options.debug_sequences) {
            snprintf(session->conformance.compliance.last_unsupported,
                     sizeof(session->conformance.compliance.last_unsupported),
                     "Invalid CSI char: 0x%02X", ch);
            session->conformance.compliance.unsupported_sequences++;
        }
        session->parse_state = VT_PARSE_NORMAL;
        ClearCSIParams(session);
    }
}

// Updated KTerm_ExecuteCSICommand
void KTerm_ExecuteCSICommand(KTerm* term, unsigned char command) {
    bool private_mode = (GET_SESSION(term)->escape_buffer[0] == '?');

    // Handle CSI ... SP q (DECSCUSR with space intermediate)
    if (command == 'q' && strstr(GET_SESSION(term)->escape_buffer, " ")) {
        ExecuteDECSCUSR(term);
        return;
    }



    switch (command) {
        case '$': // L_CSI_dollar_multi
            ExecuteCSI_Dollar(term);
            // Multi-byte CSI sequences (e.g., CSI $ q, CSI $ u)
            break;
        case '@': // L_CSI_at_ASC
            ExecuteICH(term);
            // ICH - Insert Character(s) (CSI Pn @)
            break;
        case 'A': // L_CSI_A_CUU
            ExecuteCUU(term);
            // CUU - Cursor Up (CSI Pn A)
            break;
        case 'B': // L_CSI_B_CUD
            ExecuteCUD(term);
            // CUD - Cursor Down (CSI Pn B)
            break;
        case 'C': // L_CSI_C_CUF
            ExecuteCUF(term);
            // CUF - Cursor Forward (CSI Pn C)
            break;
        case 'D': // L_CSI_D_CUB
            ExecuteCUB(term);
            // CUB - Cursor Back (CSI Pn D)
            break;
        case 'E': // L_CSI_E_CNL
            ExecuteCNL(term);
            // CNL - Cursor Next Line (CSI Pn E)
            break;
        case 'F': // L_CSI_F_CPL
            ExecuteCPL(term);
            // CPL - Cursor Previous Line (CSI Pn F)
            break;
        case 'G': // L_CSI_G_CHA
            ExecuteCHA(term);
            // CHA - Cursor Horizontal Absolute (CSI Pn G)
            break;
        case 'H': // L_CSI_H_CUP
            ExecuteCUP(term);
            // CUP - Cursor Position (CSI Pn ; Pn H)
            break;
        case 'I': // L_CSI_I_CHT
            { int n=KTerm_GetCSIParam(term, 0,1); while(n-->0) GET_SESSION(term)->cursor.x = NextTabStop(term, GET_SESSION(term)->cursor.x); if (GET_SESSION(term)->cursor.x >= term->width) GET_SESSION(term)->cursor.x = term->width -1; }
            // CHT - Cursor Horizontal Tab (CSI Pn I)
            break;
        case 'i': // L_CSI_i_MC
            ExecuteMC(term);
            // MC  - Media Copy (CSI Pn i) / DEC Printer Control
            break;
        case 'J': // L_CSI_J_ED
            ExecuteED(term, private_mode);
            // ED  - Erase in Display (CSI Pn J) / DECSED (CSI ? Pn J)
            break;
        case 'K': // L_CSI_K_EL
            ExecuteEL(term, private_mode);
            // EL  - Erase in Line (CSI Pn K) / DECSEL (CSI ? Pn K)
            break;
        case 'L': // L_CSI_L_IL
            ExecuteIL(term);
            // IL  - Insert Line(s) (CSI Pn L)
            break;
        case 'M': // L_CSI_M_DL
            ExecuteDL(term);
            // DL  - Delete Line(s) (CSI Pn M)
            break;
        case 'P': // L_CSI_P_DCH
            ExecuteDCH(term);
            // DCH - Delete Character(s) (CSI Pn P)
            break;
        case 'S': // L_CSI_S_SU
            ExecuteSU(term);
            // SU  - Scroll Up (CSI Pn S)
            break;
        case 'T': // L_CSI_T_SD
            ExecuteSD(term);
            // SD  - Scroll Down (CSI Pn T)
            break;
        case 'W': // L_CSI_W_CTC_etc
            if(private_mode) ExecuteCTC(term); else KTerm_LogUnsupportedSequence(term, "CSI W (non-private)");
            // CTC - Cursor Tab Control (CSI ? Ps W)
            break;
        case 'X': // L_CSI_X_ECH
            ExecuteECH(term);
            // ECH - Erase Character(s) (CSI Pn X)
            break;
        case 'Z': // L_CSI_Z_CBT
            { int n=KTerm_GetCSIParam(term, 0,1); while(n-->0) GET_SESSION(term)->cursor.x = PreviousTabStop(term, GET_SESSION(term)->cursor.x); }
            // CBT - Cursor Backward Tab (CSI Pn Z)
            break;
        case '`': // L_CSI_tick_HPA
            ExecuteCHA(term);
            // HPA - Horizontal Position Absolute (CSI Pn `) (Same as CHA)
            break;
        case 'a': // L_CSI_a_HPR
            { int n=KTerm_GetCSIParam(term, 0,1); GET_SESSION(term)->cursor.x+=n; if(GET_SESSION(term)->cursor.x<0)GET_SESSION(term)->cursor.x=0; if(GET_SESSION(term)->cursor.x>=term->width)GET_SESSION(term)->cursor.x=term->width-1;}
            // HPR - Horizontal Position Relative (CSI Pn a)
            break;
        case 'b': // L_CSI_b_REP
            ExecuteREP(term);
            // REP - Repeat Preceding Graphic Character (CSI Pn b)
            break;
        case 'c': // L_CSI_c_DA
            ExecuteDA(term, private_mode);
            // DA  - Device Attributes (CSI Ps c or CSI ? Ps c)
            break;
        case 'd': // L_CSI_d_VPA
            ExecuteVPA(term);
            // VPA - Vertical Line Position Absolute (CSI Pn d)
            break;
        case 'e': // L_CSI_e_VPR
            { int n=KTerm_GetCSIParam(term, 0,1); GET_SESSION(term)->cursor.y+=n; if(GET_SESSION(term)->cursor.y<0)GET_SESSION(term)->cursor.y=0; if(GET_SESSION(term)->cursor.y>=term->height)GET_SESSION(term)->cursor.y=term->height-1;}
            // VPR - Vertical Position Relative (CSI Pn e)
            break;
        case 'f': // L_CSI_f_HVP
            ExecuteCUP(term);
            // HVP - Horizontal and Vertical Position (CSI Pn ; Pn f) (Same as CUP)
            break;
        case 'g': // L_CSI_g_TBC
            ExecuteTBC(term);
            // TBC - Tabulation Clear (CSI Ps g)
            break;
        case 'h': // L_CSI_h_SM
            ExecuteSM(term, private_mode);
            // SM  - Set Mode (CSI ? Pm h or CSI Pm h)
            break;
        case 'j': // L_CSI_j_HPB
            ExecuteCUB(term);
            // HPB - Horizontal Position Backward (like CUB) (CSI Pn j)
            break;
        case 'k': // L_CSI_k_VPB
            ExecuteCUU(term);
            // VPB - Vertical Position Backward (like CUU) (CSI Pn k)
            break;
        case 'l': // L_CSI_l_RM
            ExecuteRM(term, private_mode);
            // RM  - Reset Mode (CSI ? Pm l or CSI Pm l)
            break;
        case 'm': // L_CSI_m_SGR
            ExecuteSGR(term);
            // SGR - Select Graphic Rendition (CSI Pm m)
            break;
        case 'n': // L_CSI_n_DSR
            ExecuteDSR(term);
            // DSR - Device Status Report (CSI Ps n or CSI ? Ps n)
            break;
        case 'o': // L_CSI_o_VT420
            if(GET_SESSION(term)->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "VT420 'o'");
            // DECDMAC, etc. (CSI Pt;Pb;Pl;Pr;Pp;Pattr o)
            break;
        case 'p': // L_CSI_p_DECSOFT_etc
            ExecuteCSI_P(term);
            // Various 'p' suffixed: DECSTR, DECSCL, DECRQM, DECUDK (CSI ! p, CSI " p, etc.)
            break;
        case 'q': // L_CSI_q_DECLL_DECSCUSR
            if(strstr(GET_SESSION(term)->escape_buffer, "\"")) ExecuteDECSCA(term); else if(private_mode) ExecuteDECLL(term); else ExecuteDECSCUSR(term);
            // DECSCA / DECLL / DECSCUSR
            break;
        case 'r': // L_CSI_r_DECSTBM
            if(!private_mode) ExecuteDECSTBM(term); else KTerm_LogUnsupportedSequence(term, "CSI ? r invalid");
            // DECSTBM - Set Top/Bottom Margins (CSI Pt ; Pb r)
            break;
        case 's': // L_CSI_s_SAVRES_CUR
            // If DECLRMM is enabled (CSI ? 69 h), CSI Pl ; Pr s sets margins (DECSLRM).
            // Otherwise, it saves the cursor (SCOSC/ANSISYSSC).
            if (GET_SESSION(term)->dec_modes.declrmm_enabled) {
                if(GET_SESSION(term)->conformance.features.vt420_mode) ExecuteDECSLRM(term);
                else KTerm_LogUnsupportedSequence(term, "DECSLRM requires VT420");
            } else {
                KTerm_ExecuteSaveCursor(term);
            }
            break;
        case 't': // L_CSI_t_WINMAN
            ExecuteWindowOps(term);
            // Window Manipulation (xterm) / DECSLPP (Set lines per page) (CSI Ps t)
            break;
        case 'u': // L_CSI_u_RES_CUR
            KTerm_ExecuteRestoreCursor(term);
            // Restore Cursor (ANSI.SYS) (CSI u)
            break;
        case 'v': // L_CSI_v_RECTCOPY
            if(strstr(GET_SESSION(term)->escape_buffer, "$")) ExecuteDECCRA(term); else if(private_mode) KTerm_ExecuteRectangularOps(term); else KTerm_LogUnsupportedSequence(term, "CSI v non-private invalid");
            // DECCRA
            break;
        case 'w': // L_CSI_w_RECTCHKSUM
            if(private_mode) KTerm_ExecuteRectangularOps2(term); else KTerm_LogUnsupportedSequence(term, "CSI w non-private invalid");
            // Note: DECRQCRA moved to 'y' with * intermediate as per standard.
            break;
        case 'x': // L_CSI_x_DECREQTPARM
            if(strstr(GET_SESSION(term)->escape_buffer, "$")) ExecuteDECFRA(term); else ExecuteDECREQTPARM(term);
            // DECFRA / DECREQTPARM
            break;
        case 'y': // L_CSI_y_DECTST
            // DECRQCRA is CSI ... * y (Checksum Rectangular Area)
            if(strstr(GET_SESSION(term)->escape_buffer, "*")) ExecuteDECRQCRA(term); else ExecuteDECTST(term);
            // DECTST / DECRQCRA
            break;
        case 'z': // L_CSI_z_DECVERP
            if(strstr(GET_SESSION(term)->escape_buffer, "$")) ExecuteDECERA(term);
            else if(private_mode) ExecuteDECVERP(term);
            else ExecuteDECECR(term); // CSI Pt ; Pc z (Enable Checksum Reporting)
            break;
        case '}': // L_CSI_RSBrace_VT420
            if (strstr(GET_SESSION(term)->escape_buffer, "#")) { ExecuteXTPOPSGR(term); }
            else if (strstr(GET_SESSION(term)->escape_buffer, "$")) { ExecuteDECSASD(term); }
            else { KTerm_LogUnsupportedSequence(term, "CSI } invalid"); }
            break;
        case '~': // L_CSI_Tilde_VT420
            if (strstr(GET_SESSION(term)->escape_buffer, "!")) { ExecuteDECSN(term); } else if (strstr(GET_SESSION(term)->escape_buffer, "$")) { ExecuteDECSSDT(term); } else { KTerm_LogUnsupportedSequence(term, "CSI ~ invalid"); }
            break;

        case '{': // L_CSI_LSBrace_DECSLE
            if (strstr(GET_SESSION(term)->escape_buffer, "#")) { ExecuteXTPUSHSGR(term); }
            else if(strstr(GET_SESSION(term)->escape_buffer, "$")) ExecuteDECSERA(term);
            else ExecuteDECSLE(term);
            // DECSERA / DECSLE / XTPUSHSGR
            break;
        case '|': // L_CSI_Pipe_DECRQLP
            ExecuteDECRQLP(term);
            // DECRQLP - Request Locator Position (CSI Plc |)
            break;
        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[128];
                snprintf(debug_msg, sizeof(debug_msg),
                         "Unknown CSI %s%c (0x%02X)", private_mode ? "?" : "", command, command);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
            break;
    }
}

// =============================================================================
// OSC (OPERATING SYSTEM COMMAND) PROCESSING
// =============================================================================


void KTerm_SetWindowTitle(KTerm* term, const char* title) {
    strncpy(GET_SESSION(term)->title.window_title, title, MAX_TITLE_LENGTH - 1);
    GET_SESSION(term)->title.window_title[MAX_TITLE_LENGTH - 1] = '\0';
    GET_SESSION(term)->title.title_changed = true;

    if (term->title_callback) {
        term->title_callback(term, GET_SESSION(term)->title.window_title, false);
    }

    // Also set KTerm window title
    KTerm_SetWindowTitlePlatform(GET_SESSION(term)->title.window_title);
}

void KTerm_SetIconTitle(KTerm* term, const char* title) {
    strncpy(GET_SESSION(term)->title.icon_title, title, MAX_TITLE_LENGTH - 1);
    GET_SESSION(term)->title.icon_title[MAX_TITLE_LENGTH - 1] = '\0';
    GET_SESSION(term)->title.icon_changed = true;

    if (term->title_callback) {
        term->title_callback(term, GET_SESSION(term)->title.icon_title, true);
    }
}

void ResetForegroundKTermColor(KTerm* term) {
    GET_SESSION(term)->current_fg.color_mode = 0;
    GET_SESSION(term)->current_fg.value.index = COLOR_WHITE;
}

void ResetBackgroundKTermColor(KTerm* term) {
    GET_SESSION(term)->current_bg.color_mode = 0;
    GET_SESSION(term)->current_bg.value.index = COLOR_BLACK;
}

void ResetCursorKTermColor(KTerm* term) {
    GET_SESSION(term)->cursor.color.color_mode = 0;
    GET_SESSION(term)->cursor.color.value.index = COLOR_WHITE;
}

void ProcessKTermColorCommand(KTerm* term, const char* data) {
    // Format: color_index;rgb:rr/gg/bb or color_index;?
    char* semicolon = strchr(data, ';');
    if (!semicolon) return;

    int color_index = atoi(data);
    char* color_spec = semicolon + 1;

    if (color_spec[0] == '?') {
        // Query color
        char response[128];
        if (color_index >= 0 && color_index < 256) {
            RGB_KTermColor c = term->color_palette[color_index];
            snprintf(response, sizeof(response), "\x1B]4;%d;rgb:%02x/%02x/%02x\x1B\\",
                    color_index, c.r, c.g, c.b);
            KTerm_QueueResponse(term, response);
        }
    } else if (strncmp(color_spec, "rgb:", 4) == 0) {
        // Set color: rgb:rr/gg/bb
        unsigned int r, g, b;
        if (sscanf(color_spec + 4, "%02x/%02x/%02x", &r, &g, &b) == 3) {
            if (color_index >= 0 && color_index < 256) {
                term->color_palette[color_index] = (RGB_KTermColor){r, g, b, 255};
            }
        }
    }
}

// Additional helper functions for OSC commands
void ResetKTermColorPalette(KTerm* term, const char* data) {
    if (!data || strlen(data) == 0) {
        // Reset entire palette
        KTerm_InitKTermColorPalette(term);
    } else {
        // Reset specific colors (comma-separated list)
        char* data_copy = strdup(data);
        char* token = strtok(data_copy, ";");

        while (token != NULL) {
            int color_index = atoi(token);
            if (color_index >= 0 && color_index < 16) {
                // Reset to default ANSI color
                term->color_palette[color_index] = (RGB_KTermColor){
                    ansi_colors[color_index].r,
                    ansi_colors[color_index].g,
                    ansi_colors[color_index].b,
                    255
                };
            }
            token = strtok(NULL, ";");
        }

        free(data_copy);
    }
}

void ProcessForegroundKTermColorCommand(KTerm* term, const char* data) {
    if (data[0] == '?') {
        // Query foreground color
        char response[64];
        ExtendedKTermColor fg = GET_SESSION(term)->current_fg;
        if (fg.color_mode == 0 && fg.value.index < 16) {
            RGB_KTermColor c = term->color_palette[fg.value.index];
            snprintf(response, sizeof(response), "\x1B]10;rgb:%02x/%02x/%02x\x1B\\", c.r, c.g, c.b);
        } else if (fg.color_mode == 1) {
            snprintf(response, sizeof(response), "\x1B]10;rgb:%02x/%02x/%02x\x1B\\",
                    fg.value.rgb.r, fg.value.rgb.g, fg.value.rgb.b);
        }
        KTerm_QueueResponse(term, response);
    }
    // Setting foreground via OSC is less common, usually done via SGR
}

void ProcessBackgroundKTermColorCommand(KTerm* term, const char* data) {
    if (data[0] == '?') {
        // Query background color
        char response[64];
        ExtendedKTermColor bg = GET_SESSION(term)->current_bg;
        if (bg.color_mode == 0 && bg.value.index < 16) {
            RGB_KTermColor c = term->color_palette[bg.value.index];
            snprintf(response, sizeof(response), "\x1B]11;rgb:%02x/%02x/%02x\x1B\\", c.r, c.g, c.b);
        } else if (bg.color_mode == 1) {
            snprintf(response, sizeof(response), "\x1B]11;rgb:%02x/%02x/%02x\x1B\\",
                    bg.value.rgb.r, bg.value.rgb.g, bg.value.rgb.b);
        }
        KTerm_QueueResponse(term, response);
    }
}

void ProcessCursorKTermColorCommand(KTerm* term, const char* data) {
    if (data[0] == '?') {
        // Query cursor color
        char response[64];
        ExtendedKTermColor cursor_color = GET_SESSION(term)->cursor.color;
        if (cursor_color.color_mode == 0 && cursor_color.value.index < 16) {
            RGB_KTermColor c = term->color_palette[cursor_color.value.index];
            snprintf(response, sizeof(response), "\x1B]12;rgb:%02x/%02x/%02x\x1B\\", c.r, c.g, c.b);
        } else if (cursor_color.color_mode == 1) {
            snprintf(response, sizeof(response), "\x1B]12;rgb:%02x/%02x/%02x\x1B\\",
                    cursor_color.value.rgb.r, cursor_color.value.rgb.g, cursor_color.value.rgb.b);
        }
        KTerm_QueueResponse(term, response);
    }
}

void ProcessFontCommand(KTerm* term, const char* data) {
    if (data[0] == '?') {
        // Query not supported yet
        return;
    }
    // Set font
    KTerm_LoadFont(term, data);
}

// Base64 decoding helper
static int Base64Val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static int DecodeBase64(const char* input, unsigned char* output, size_t out_max) {
    size_t in_len = strlen(input);
    size_t out_len = 0;
    unsigned int val = 0;
    int valb = -8;
    for (size_t i = 0; i < in_len; i++) {
        int c = Base64Val(input[i]);
        if (c == -1) continue; // Skip whitespace/invalid
        val = (val << 6) | c;
        valb += 6;
        if (valb >= 0) {
            if (out_len < out_max) {
                output[out_len++] = (unsigned char)((val >> valb) & 0xFF);
            }
            valb -= 8;
        }
    }
    if (out_len < out_max) output[out_len] = 0;
    return (int)out_len;
}

static void EncodeBase64(const unsigned char* input, size_t input_len, char* output, size_t out_max) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t out_pos = 0;
    unsigned int val = 0;
    int valb = -6;
    for (size_t i = 0; i < input_len; i++) {
        val = (val << 8) | input[i];
        valb += 8;
        while (valb >= 0) {
            if (out_pos < out_max) {
                output[out_pos++] = base64_chars[(val >> valb) & 0x3F];
            }
            valb -= 6;
        }
    }
    if (valb > -6) {
        if (out_pos < out_max) {
            output[out_pos++] = base64_chars[((val << 8) >> (valb + 8)) & 0x3F];
        }
    }
    while (out_pos % 4) {
        if (out_pos < out_max) output[out_pos++] = '=';
    }
    if (out_pos < out_max) output[out_pos] = 0;
}

void ProcessClipboardCommand(KTerm* term, const char* data) {
    // Clipboard operations: c;base64data or c;?
    // data format is: Pc;Pd
    // Pc = clipboard selection (c, p, s, 0-7)
    // Pd = data (base64) or ?

    char* data_copy = strdup(data);
    if (!data_copy) return;

    char* semicolon = strchr(data_copy, ';');
    if (!semicolon) {
        free(data_copy);
        return;
    }

    *semicolon = '\0';
    char* pc_str = data_copy;
    char* pd_str = semicolon + 1;
    char clipboard_selector = pc_str[0];

    if (strcmp(pd_str, "?") == 0) {
        // Query clipboard
        const char* clipboard_text = NULL;
        if (KTerm_GetClipboardText(&clipboard_text) == KTERM_SUCCESS && clipboard_text) {
            size_t text_len = strlen(clipboard_text);
            size_t encoded_len = 4 * ((text_len + 2) / 3) + 1;
            char* encoded_data = malloc(encoded_len);
            if (encoded_data) {
                EncodeBase64((const unsigned char*)clipboard_text, text_len, encoded_data, encoded_len);
                char response_header[16];
                snprintf(response_header, sizeof(response_header), "\x1B]52;%c;", clipboard_selector);
                KTerm_QueueResponse(term, response_header);
                KTerm_QueueResponse(term, encoded_data);
                KTerm_QueueResponse(term, "\x1B\\");
                free(encoded_data);
            }
            KTerm_FreeString((char*)clipboard_text);
        } else {
            // Empty clipboard response
            char response[16];
            snprintf(response, sizeof(response), "\x1B]52;%c;\x1B\\", clipboard_selector);
            KTerm_QueueResponse(term, response);
        }
    } else {
        // Set clipboard data (base64 encoded)
        // We only support setting the standard clipboard ('c' or '0')
        if (clipboard_selector == 'c' || clipboard_selector == '0') {
            size_t decoded_size = strlen(pd_str); // Upper bound
            unsigned char* decoded_data = malloc(decoded_size + 1);
            if (decoded_data) {
                DecodeBase64(pd_str, decoded_data, decoded_size + 1);
                KTerm_SetClipboardText((const char*)decoded_data);
                free(decoded_data);
            }
        }
    }

    free(data_copy);
}

void KTerm_ExecuteOSCCommand(KTerm* term, KTermSession* session) {
    char* params = session->escape_buffer;

    // Find the command number
    char* semicolon = strchr(params, ';');
    if (!semicolon) {
        KTerm_LogUnsupportedSequence(term, "Malformed OSC sequence");
        return;
    }

    *semicolon = '\0';
    int command = atoi(params);
    char* data = semicolon + 1;

    switch (command) {
        case 0: // Set window and icon title
        case 2: // Set window title
            KTerm_SetWindowTitle(term, data);
            break;

        case 1: // Set icon title
            KTerm_SetIconTitle(term, data);
            break;

        case 9: // Notification
            if (term->notification_callback) {
                term->notification_callback(term, data);
            }
            break;

        case 4: // Set/query color palette
            ProcessKTermColorCommand(term, data);
            break;

        case 10: // Query/set foreground color
            ProcessForegroundKTermColorCommand(term, data);
            break;

        case 11: // Query/set background color
            ProcessBackgroundKTermColorCommand(term, data);
            break;

        case 12: // Query/set cursor color
            ProcessCursorKTermColorCommand(term, data);
            break;

        case 50: // Set font
            ProcessFontCommand(term, data);
            break;

        case 52: // Clipboard operations
            ProcessClipboardCommand(term, data);
            break;

        case 104: // Reset color palette
            ResetKTermColorPalette(term, data);
            break;

        case 110: // Reset foreground color
            ResetForegroundKTermColor(term);
            break;

        case 111: // Reset background color
            ResetBackgroundKTermColor(term);
            break;

        case 112: // Reset cursor color
            ResetCursorKTermColor(term);
            break;

        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[128];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown OSC command: %d", command);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}

// =============================================================================
// DCS (DEVICE CONTROL STRING) PROCESSING
// =============================================================================

void ProcessTermcapRequest(KTerm* term, KTermSession* session, const char* request) {
    (void)session;
    // XTGETTCAP - Get terminal capability
    // This is an xterm extension for querying termcap/terminfo values

    char response[256];

    if (strcmp(request, "Co") == 0) {
        // Number of colors
        snprintf(response, sizeof(response), "\x1BP1+r436f=323536\x1B\\"); // "256" in hex
    } else if (strcmp(request, "lines") == 0) {
        // Number of lines
        char hex_lines[16];
        snprintf(hex_lines, sizeof(hex_lines), "%X", term->height);
        snprintf(response, sizeof(response), "\x1BP1+r6c696e6573=%s\x1B\\", hex_lines);
    } else if (strcmp(request, "cols") == 0) {
        // Number of columns
        char hex_cols[16];
        snprintf(hex_cols, sizeof(hex_cols), "%X", term->width);
        snprintf(response, sizeof(response), "\x1BP1+r636f6c73=%s\x1B\\", hex_cols);
    } else {
        // Unknown capability
        snprintf(response, sizeof(response), "\x1BP0+r%s\x1B\\", request);
    }

    KTerm_QueueResponse(term, response);
}

// Helper function to convert a single hex character to its integer value
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1; // Invalid hex char
}

void DefineUserKey(KTerm* term, KTermSession* session, int key_code, const char* sequence, size_t sequence_len) {
    (void)term;
    // Expand programmable keys array if needed
    if (session->programmable_keys.count >= session->programmable_keys.capacity) {
        size_t new_capacity = session->programmable_keys.capacity == 0 ? 16 :
                             session->programmable_keys.capacity * 2;

        ProgrammableKey* new_keys = realloc(session->programmable_keys.keys,
                                           new_capacity * sizeof(ProgrammableKey));
        if (!new_keys) return;

        session->programmable_keys.keys = new_keys;
        session->programmable_keys.capacity = new_capacity;
    }

    // Find existing key or add new one
    ProgrammableKey* key = NULL;
    for (size_t i = 0; i < session->programmable_keys.count; i++) {
        if (session->programmable_keys.keys[i].key_code == key_code) {
            key = &session->programmable_keys.keys[i];
            if (key->sequence) free(key->sequence); // Free old sequence
            break;
        }
    }

    if (!key) {
        key = &session->programmable_keys.keys[session->programmable_keys.count++];
        key->key_code = key_code;
    }

    // Store new sequence
    key->sequence_length = sequence_len;
    key->sequence = malloc(key->sequence_length);
    if (key->sequence) {
        memcpy(key->sequence, sequence, key->sequence_length);
    }
    key->active = true;
}

void ProcessUserDefinedKeys(KTerm* term, KTermSession* session, const char* data) {
    // Parse user defined key format: key/string;key/string;...
    // The string is a sequence of hexadecimal pairs.
    if (!session->conformance.features.user_defined_keys) {
        KTerm_LogUnsupportedSequence(term, "User defined keys require VT320 mode");
        return;
    }

    char* data_copy = strdup(data);
    if (!data_copy) return;

    // Use manual parsing for thread safety
    char* current_ptr = data_copy;
    while (current_ptr && *current_ptr) {
        char* token = current_ptr;
        char* next_delim = strchr(current_ptr, ';');
        if (next_delim) {
            *next_delim = '\0';
            current_ptr = next_delim + 1;
        } else {
            current_ptr = NULL;
        }

        char* slash = strchr(token, '/');
        if (slash) {
            *slash = '\0';
            int key_code = atoi(token);
            char* hex_string = slash + 1;
            size_t hex_len = strlen(hex_string);

            if (hex_len % 2 != 0) {
                // Invalid hex string length
                KTerm_LogUnsupportedSequence(term, "Invalid hex string in DECUDK");
                continue;
            }

            size_t decoded_len = hex_len / 2;
            char* decoded_sequence = malloc(decoded_len);
            if (!decoded_sequence) {
                // Allocation failed
                continue;
            }

            for (size_t i = 0; i < decoded_len; i++) {
                int high = hex_char_to_int(hex_string[i * 2]);
                int low = hex_char_to_int(hex_string[i * 2 + 1]);
                if (high == -1 || low == -1) {
                    // Invalid hex character
                    free(decoded_sequence);
                    decoded_sequence = NULL;
                    break;
                }
                decoded_sequence[i] = (char)((high << 4) | low);
            }

            if (decoded_sequence) {
                DefineUserKey(term, session, key_code, decoded_sequence, decoded_len);
                free(decoded_sequence);
            }
        }
    }

    free(data_copy);
}

void ClearUserDefinedKeys(KTerm* term, KTermSession* session) {
    (void)term;
    for (size_t i = 0; i < session->programmable_keys.count; i++) {
        free(session->programmable_keys.keys[i].sequence);
    }
    session->programmable_keys.count = 0;
}

void ProcessSoftFontDownload(KTerm* term, KTermSession* session, const char* data) {
    // Phase 7.2: Verify Safe Parsing (StreamScanner)
    if (!session->conformance.features.soft_fonts) {
        KTerm_LogUnsupportedSequence(term, "Soft fonts not supported");
        return;
    }

    StreamScanner scanner = { .ptr = data, .len = strlen(data), .pos = 0 };
    int params[6] = {0};
    int param_idx = 0;

    // Parse parameters
    while (param_idx < 6 && scanner.pos < scanner.len) {
        if (Stream_Peek(&scanner) == '{') break;

        int val = 0;
        if (Stream_ReadInt(&scanner, &val)) {
            params[param_idx++] = val;
        } else {
            // Check if we hit invalid char or just empty
            if (Stream_Peek(&scanner) == ';') {
                params[param_idx++] = 0; // Empty param
            } else if (Stream_Peek(&scanner) == '{') {
                break;
            } else {
                Stream_Consume(&scanner); // Skip garbage
                continue;
            }
        }

        if (Stream_Peek(&scanner) == ';') {
            Stream_Consume(&scanner);
        }
    }

    // Move to data block
    while (scanner.pos < scanner.len && Stream_Peek(&scanner) != '{') {
        Stream_Consume(&scanner);
    }
    if (!Stream_Expect(&scanner, '{')) {
        return; // No data block
    }

    // Update dimensions if provided
    if (param_idx >= 5) {
        int w = params[4];
        if (w > 0 && w <= 32) session->soft_font.char_width = w;
    }
    if (param_idx >= 6) {
        int h = params[5];
        if (h > 0 && h <= 32) session->soft_font.char_height = h;
    }

    // Parse sixel-encoded font data
    int current_char = (param_idx >= 2) ? params[1] : 0;
    int sixel_row_base = 0;
    int current_col = 0;

    if (current_char < 256) {
        memset(session->soft_font.font_data[current_char], 0, 32);
    }

    while (scanner.pos < scanner.len) {
        unsigned char ch = Stream_Consume(&scanner);

        if (ch == '/' || ch == ';') {
            if (current_char < 256) {
                session->soft_font.loaded[current_char] = true;
            }
            current_char++;
            if (current_char >= 256) break;

            memset(session->soft_font.font_data[current_char], 0, 32);
            sixel_row_base = 0;
            current_col = 0;

        } else if (ch == '-') {
            sixel_row_base += 6;
            current_col = 0;

        } else if (ch >= 63 && ch <= 126) {
            int sixel_val = ch - 63;
            if (current_char < 256 && current_col < 8) {
                for (int b = 0; b < 6; b++) {
                    int pixel_y = sixel_row_base + b;
                    if (pixel_y < 32) {
                        if ((sixel_val >> b) & 1) {
                            session->soft_font.font_data[current_char][pixel_y] |= (1 << (7 - current_col));
                        }
                    }
                }
                current_col++;
            }
        }
    }
    session->soft_font.dirty = true;
    session->soft_font.active = true;
    KTerm_CalculateFontMetrics(session->soft_font.font_data, 256, session->soft_font.char_width, session->soft_font.char_height, 32, false, session->soft_font.metrics);
}

void ProcessStatusRequest(KTerm* term, KTermSession* session, const char* request) {
    // DECRQSS - Request Status String
    char response[MAX_COMMAND_BUFFER];

    if (strcmp(request, "m") == 0) {
        // Request SGR status - Reconstruct SGR string
        char sgr[256];
        int len = 0;

        len += snprintf(sgr + len, sizeof(sgr) - len, "0"); // Reset first

        if (session->current_attributes & KTERM_ATTR_BOLD) len += snprintf(sgr + len, sizeof(sgr) - len, ";1");
        if (session->current_attributes & KTERM_ATTR_FAINT) len += snprintf(sgr + len, sizeof(sgr) - len, ";2");
        if (session->current_attributes & KTERM_ATTR_ITALIC) len += snprintf(sgr + len, sizeof(sgr) - len, ";3");
        if (session->current_attributes & KTERM_ATTR_UNDERLINE) len += snprintf(sgr + len, sizeof(sgr) - len, ";4");
        if (session->current_attributes & KTERM_ATTR_BLINK) len += snprintf(sgr + len, sizeof(sgr) - len, ";5");
        if (session->current_attributes & KTERM_ATTR_REVERSE) len += snprintf(sgr + len, sizeof(sgr) - len, ";7");
        if (session->current_attributes & KTERM_ATTR_CONCEAL) len += snprintf(sgr + len, sizeof(sgr) - len, ";8");
        if (session->current_attributes & KTERM_ATTR_STRIKE) len += snprintf(sgr + len, sizeof(sgr) - len, ";9");
        if (session->current_attributes & KTERM_ATTR_DOUBLE_UNDERLINE) len += snprintf(sgr + len, sizeof(sgr) - len, ";21");
        if (session->current_attributes & KTERM_ATTR_OVERLINE) len += snprintf(sgr + len, sizeof(sgr) - len, ";53");

        // Foreground
        if (session->current_fg.color_mode == 0) {
            int idx = session->current_fg.value.index;
            if (idx != COLOR_WHITE) {
                if (idx < 8) len += snprintf(sgr + len, sizeof(sgr) - len, ";%d", 30 + idx);
                else if (idx < 16) len += snprintf(sgr + len, sizeof(sgr) - len, ";%d", 90 + (idx - 8));
                else len += snprintf(sgr + len, sizeof(sgr) - len, ";38;5;%d", idx);
            }
        } else {
             len += snprintf(sgr + len, sizeof(sgr) - len, ";38;2;%d;%d;%d",
                 session->current_fg.value.rgb.r,
                 session->current_fg.value.rgb.g,
                 session->current_fg.value.rgb.b);
        }

        // Background
        if (session->current_bg.color_mode == 0) {
            int idx = session->current_bg.value.index;
            if (idx != COLOR_BLACK) {
                if (idx < 8) len += snprintf(sgr + len, sizeof(sgr) - len, ";%d", 40 + idx);
                else if (idx < 16) len += snprintf(sgr + len, sizeof(sgr) - len, ";%d", 100 + (idx - 8));
                else len += snprintf(sgr + len, sizeof(sgr) - len, ";48;5;%d", idx);
            }
        } else {
             len += snprintf(sgr + len, sizeof(sgr) - len, ";48;2;%d;%d;%d",
                 session->current_bg.value.rgb.r,
                 session->current_bg.value.rgb.g,
                 session->current_bg.value.rgb.b);
        }

        snprintf(response, sizeof(response), "\x1BP1$r%sm\x1B\\", sgr);
        KTerm_QueueResponse(term, response);
    } else if (strcmp(request, "r") == 0) {
        // Request scrolling region
        snprintf(response, sizeof(response), "\x1BP1$r%d;%dr\x1B\\",
                GET_SESSION(term)->scroll_top + 1, GET_SESSION(term)->scroll_bottom + 1);
        KTerm_QueueResponse(term, response);
    } else {
        // Unknown request
        snprintf(response, sizeof(response), "\x1BP0$r%s\x1B\\", request);
        KTerm_QueueResponse(term, response);
    }
}

// New KTerm_ExecuteDCSAnswerback for DCS 0 ; 0 $ t <message> ST
void KTerm_ExecuteDCSAnswerback(KTerm* term, KTermSession* session) {
    char* message_start = strstr(session->escape_buffer, "$t");
    if (message_start) {
        message_start += 2; // Skip "$t"
        char* message_end = strstr(message_start, "\x1B\\"); // Find ST
        if (message_end) {
            size_t length = message_end - message_start;
            if (length >= MAX_COMMAND_BUFFER) {
                length = MAX_COMMAND_BUFFER - 1; // Prevent overflow
            }
            strncpy(session->answerback_buffer, message_start, length);
            session->answerback_buffer[length] = '\0';
        } else if (session->options.debug_sequences) {
            KTerm_LogUnsupportedSequence(term, "Incomplete DCS $ t sequence");
        }
    } else if (session->options.debug_sequences) {
        KTerm_LogUnsupportedSequence(term, "Invalid DCS $ t sequence");
    }
}

typedef struct {
    const char* name;
    int cell_width;
    int cell_height;
    int data_width;
    int data_height;
    const void* data;
    bool is_16bit;
} KTermFontDef;

static const KTermFontDef available_fonts[] = {
    {"VT220", 8, 10, 8, 10, dec_vt220_cp437_8x10, false},
    {"IBM", 10, 10, 8, 8, ibm_font_8x8, false}, // 10x10 Cell, 8x8 Data (Centered)
    {"VGA", 8, 8, 8, 8, vga_perfect_8x8_font, false},
    {"ULTIMATE", 8, 16, 8, 16, ultimate_oldschool_pc_font_8x16, false},
    {"CP437_16", 8, 16, 8, 16, cp437_font__8x16, false},
    {"NEC", 8, 16, 8, 16, nec_apc3_font_8x16, false},
    {"TOSHIBA", 8, 16, 8, 16, toshiba_sat_8x16, false},
    {"TRIDENT", 8, 16, 8, 16, trident_8x16, false},
    {"COMPAQ", 8, 16, 8, 16, compaq_portable3_8x16, false},
    {"OLYMPIAD", 8, 16, 8, 16, olympiad_font_8x16, false},
    {"MC6847", 8, 8, 8, 8, MC6847_font_8x8, false},
    {"NEOGEO", 8, 8, 8, 8, neogeo_bios_8x8, false},
    {"ATASCII", 8, 8, 8, 8, atascii_font_8x8, false},
    {"PETSCII", 8, 8, 8, 8, petscii_unshifted_font_8x8, false},
    {"PETSCII_SHIFT", 8, 8, 8, 8, petscii_shifted_font_8x8, false},
    {"TOPAZ", 8, 8, 8, 8, topaz_font_8x8, false},
    {"PREPPIE", 8, 8, 8, 8, preppie_font_8x8, false},
    {"VCR", 12, 14, 12, 14, vcr_osd_font_12x14, true},
    {NULL, 0, 0, 0, 0, NULL, false}
};

static int KTerm_Strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

void KTerm_SetFont(KTerm* term, const char* name) {
    for (int i = 0; available_fonts[i].name != NULL; i++) {
        if (KTerm_Strcasecmp(available_fonts[i].name, name) == 0) {
            term->char_width = available_fonts[i].cell_width;
            term->char_height = available_fonts[i].cell_height;
            term->font_data_width = available_fonts[i].data_width;
            term->font_data_height = available_fonts[i].data_height;
            term->current_font_data = available_fonts[i].data;
            term->current_font_is_16bit = available_fonts[i].is_16bit;

            KTerm_CalculateFontMetrics(term->current_font_data, 256, term->font_data_width, term->font_data_height, 0, term->current_font_is_16bit, term->font_metrics);

            // Recreate Font Texture
            KTerm_CreateFontTexture(term);

            // Trigger Resize to update window dimensions/buffers
            KTerm_Resize(term, term->width, term->height);
            return;
        }
    }
}

typedef struct {
    char text[256];
    char font_name[64];
    bool kerned;
    int align; // 0=LEFT, 1=CENTER, 2=RIGHT
    RGB_KTermColor gradient_start;
    RGB_KTermColor gradient_end;
    bool gradient_enabled;
} BannerOptions;

static bool KTerm_ParseColor(const char* str, RGB_KTermColor* color) {
    if (!str || !color) return false;

    if (str[0] == '#') {
        unsigned int r, g, b;
        if (sscanf(str + 1, "%02x%02x%02x", &r, &g, &b) == 3) {
            color->r = (unsigned char)r;
            color->g = (unsigned char)g;
            color->b = (unsigned char)b;
            color->a = 255;
            return true;
        }
    } else {
        int r, g, b;
        if (sscanf(str, "%d,%d,%d", &r, &g, &b) == 3) {
            color->r = (unsigned char)r;
            color->g = (unsigned char)g;
            color->b = (unsigned char)b;
            color->a = 255;
            return true;
        }
    }

    // Try to parse named colors
    for (int i = 0; i < 16; i++) {
        // Simple named colors map could be added here, but for now rely on RGB/Hex
    }
    return false;
}

static void KTerm_ProcessBannerOptions(const char* params, BannerOptions* options) {
    // Default values
    memset(options, 0, sizeof(BannerOptions));
    options->align = 0; // LEFT
    options->kerned = false;
    options->gradient_enabled = false;

    char buffer[512];
    strncpy(buffer, params, sizeof(buffer)-1);
    buffer[511] = '\0';

    // Check for Legacy Format (FIXED;... or KERNED;...)
    // We check the first token.
    char legacy_check[64];
    const char* first_semi = strchr(params, ';');
    size_t token_len = first_semi ? (size_t)(first_semi - params) : strlen(params);
    if (token_len < sizeof(legacy_check)) {
        strncpy(legacy_check, params, token_len);
        legacy_check[token_len] = '\0';

        bool is_legacy = false;
        if (KTerm_Strcasecmp(legacy_check, "FIXED") == 0) {
            options->kerned = false;
            is_legacy = true;
        } else if (KTerm_Strcasecmp(legacy_check, "KERNED") == 0) {
            options->kerned = true;
            is_legacy = true;
        }

        if (is_legacy) {
            if (first_semi) {
                strncpy(options->text, first_semi + 1, sizeof(options->text)-1);
            }
            return;
        }
    }

    char* token = strtok(buffer, ";");
    while (token) {
        char* eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            char* key = token;
            char* val = eq + 1;

            if (KTerm_Strcasecmp(key, "TEXT") == 0) {
                strncpy(options->text, val, sizeof(options->text)-1);
            } else if (KTerm_Strcasecmp(key, "FONT") == 0) {
                strncpy(options->font_name, val, sizeof(options->font_name)-1);
            } else if (KTerm_Strcasecmp(key, "ALIGN") == 0) {
                if (KTerm_Strcasecmp(val, "CENTER") == 0) options->align = 1;
                else if (KTerm_Strcasecmp(val, "RIGHT") == 0) options->align = 2;
                else options->align = 0;
            } else if (KTerm_Strcasecmp(key, "GRADIENT") == 0) {
                char* sep = strchr(val, '|');
                if (sep) {
                    *sep = '\0';
                    if (KTerm_ParseColor(val, &options->gradient_start) &&
                        KTerm_ParseColor(sep + 1, &options->gradient_end)) {
                        options->gradient_enabled = true;
                    }
                }
            } else if (KTerm_Strcasecmp(key, "MODE") == 0) {
                 if (KTerm_Strcasecmp(val, "KERNED") == 0) options->kerned = true;
            }
        } else {
            // Positional argument fallback (treated as text if not legacy keyword)
            strncpy(options->text, token, sizeof(options->text)-1);
        }
        token = strtok(NULL, ";");
    }
}

static void KTerm_GenerateBanner(KTerm* term, KTermSession* session, const BannerOptions* options) {
    const char* text = options->text;
    if (!text || strlen(text) == 0) return;
    int len = strlen(text);

    // Default Font
    const uint8_t* font_data = (const uint8_t*)term->current_font_data;
    bool is_16bit = term->current_font_is_16bit;
    int height = term->font_data_height;
    int width = term->font_data_width;

    KTermFontMetric temp_metrics[256];
    bool use_temp_metrics = false;

    // Check soft font
    if (session->soft_font.active) {
        font_data = (const uint8_t*)session->soft_font.font_data;
        height = session->soft_font.char_height;
        width = session->soft_font.char_width;
        is_16bit = (width > 8);
    }

    // Check requested font overrides
    if (strlen(options->font_name) > 0) {
        for (int i = 0; available_fonts[i].name != NULL; i++) {
             if (KTerm_Strcasecmp(available_fonts[i].name, options->font_name) == 0) {
                 font_data = (const uint8_t*)available_fonts[i].data;
                 width = available_fonts[i].data_width;
                 height = available_fonts[i].data_height;
                 is_16bit = available_fonts[i].is_16bit;

                 if (options->kerned) {
                      KTerm_CalculateFontMetrics(font_data, 256, width, height, 0, is_16bit, temp_metrics);
                      use_temp_metrics = true;
                 }
                 break;
             }
        }
    }

    // Calculate total width for alignment
    int total_width = 0;
    if (options->align != 0) {
        for (int i = 0; i < len; i++) {
            unsigned char c = (unsigned char)text[i];
            int w = width;
            if (options->kerned) {
                KTermFontMetric* m;
                if (use_temp_metrics) m = &temp_metrics[c];
                else if (session->soft_font.active && font_data == (const uint8_t*)session->soft_font.font_data) m = &session->soft_font.metrics[c];
                else m = &term->font_metrics[c];

                if (m->end_x >= m->begin_x) {
                     w = m->end_x - m->begin_x + 1;
                } else if (c == ' ') {
                     w = width / 2;
                } else {
                     w = 0;
                }
                // Kerning adds 1 space
                 if (w > 0) w++;
            }
            total_width += w;
        }
    }

    int padding = 0;
    if (options->align == 1) { // Center
        padding = (term->width - total_width) / 2;
    } else if (options->align == 2) { // Right
        padding = term->width - total_width;
    }
    if (padding < 0) padding = 0;

    for (int y = 0; y < height; y++) {
        char line_buffer[16384];
        int line_pos = 0;

        // Padding
        for(int p=0; p<padding; p++) {
             if (line_pos < (int)sizeof(line_buffer)-1) line_buffer[line_pos++] = ' ';
        }

        for (int i = 0; i < len; i++) {
            unsigned char c = (unsigned char)text[i];

            // Apply Gradient Color
            if (options->gradient_enabled) {
                float t = 0.0f;
                if (len > 1) t = (float)i / (float)(len - 1);

                unsigned char r = (unsigned char)(options->gradient_start.r + (options->gradient_end.r - options->gradient_start.r) * t);
                unsigned char g = (unsigned char)(options->gradient_start.g + (options->gradient_end.g - options->gradient_start.g) * t);
                unsigned char b = (unsigned char)(options->gradient_start.b + (options->gradient_end.b - options->gradient_start.b) * t);

                char color_seq[32];
                snprintf(color_seq, sizeof(color_seq), "\x1B[38;2;%d;%d;%dm", r, g, b);
                int seq_len = strlen(color_seq);
                if (line_pos + seq_len < (int)sizeof(line_buffer)) {
                    strcpy(line_buffer + line_pos, color_seq);
                    line_pos += seq_len;
                }
            }

            // Get Glyph Row Data
            uint32_t row_data = 0;

            if (is_16bit) {
                if (session->soft_font.active && font_data == (const uint8_t*)session->soft_font.font_data) {
                     uint8_t b1 = session->soft_font.font_data[c][y * 2];
                     uint8_t b2 = session->soft_font.font_data[c][y * 2 + 1];
                     row_data = (b1 << 8) | b2;
                } else {
                    const uint16_t* font_data16 = (const uint16_t*)font_data;
                    row_data = font_data16[c * height + y];
                }
            } else {
                if (session->soft_font.active && font_data == (const uint8_t*)session->soft_font.font_data) {
                    row_data = session->soft_font.font_data[c][y];
                } else {
                    row_data = font_data[c * height + y];
                }
            }

            // Determine render range
            int start_x = 0;
            int end_x = width - 1;

            if (options->kerned) {
                KTermFontMetric* m;
                if (use_temp_metrics) m = &temp_metrics[c];
                else if (session->soft_font.active && font_data == (const uint8_t*)session->soft_font.font_data) m = &session->soft_font.metrics[c];
                else m = &term->font_metrics[c];

                if (m->end_x >= m->begin_x) {
                    start_x = m->begin_x;
                    end_x = m->end_x;
                } else {
                    if (c == ' ') {
                        start_x = 0;
                        end_x = width / 2;
                    } else {
                         start_x = 0; end_x = -1; // Skip
                    }
                }
            }

            // Append bits
            for (int x = start_x; x <= end_x; x++) {
                if (line_pos >= (int)sizeof(line_buffer) - 5) break;

                bool bit_set = false;
                if ((row_data >> (width - 1 - x)) & 1) {
                     bit_set = true;
                }

                if (bit_set) {
                    line_buffer[line_pos++] = '\xE2';
                    line_buffer[line_pos++] = '\x96';
                    line_buffer[line_pos++] = '\x88';
                } else {
                    line_buffer[line_pos++] = ' ';
                }
            }

            // Spacing
            if (options->kerned) {
                if (line_pos < (int)sizeof(line_buffer) - 1) line_buffer[line_pos++] = ' ';
            }
        }

        // Reset Color at end of line if gradient
        if (options->gradient_enabled) {
             const char* reset = "\x1B[0m";
             if (line_pos + strlen(reset) < sizeof(line_buffer)) {
                 strcpy(line_buffer + line_pos, reset);
                 line_pos += strlen(reset);
             }
        }

        line_buffer[line_pos] = '\0';
        KTerm_WriteString(term, line_buffer);
        KTerm_WriteString(term, "\r\n");
    }
}

static bool KTerm_ProcessInternalGatewayCommand(KTerm* term, KTermSession* session, const char* class_id, const char* id, const char* command, const char* params) {
    (void)session;
    if (strcmp(class_id, "KTERM") != 0) return false;

    if (strcmp(command, "SET") == 0) {
        if (strncmp(params, "ATTR;", 5) == 0) {
            // SET;ATTR;KEY=VAL;...
            char attr_buffer[256];
            strncpy(attr_buffer, params + 5, sizeof(attr_buffer)-1);
            attr_buffer[255] = '\0';

            char* token = strtok(attr_buffer, ";");
            while (token) {
                char* eq = strchr(token, '=');
                if (eq) {
                    *eq = '\0';
                    char* key = token;
                    char* val = eq + 1;
                    int v = atoi(val);

                    if (strcmp(key, "BOLD") == 0) {
                        if (v) session->current_attributes |= KTERM_ATTR_BOLD;
                        else session->current_attributes &= ~KTERM_ATTR_BOLD;
                    } else if (strcmp(key, "DIM") == 0) {
                         if (v) session->current_attributes |= KTERM_ATTR_FAINT;
                         else session->current_attributes &= ~KTERM_ATTR_FAINT;
                    } else if (strcmp(key, "ITALIC") == 0) {
                        if (v) session->current_attributes |= KTERM_ATTR_ITALIC;
                        else session->current_attributes &= ~KTERM_ATTR_ITALIC;
                    } else if (strcmp(key, "UNDERLINE") == 0) {
                        if (v) session->current_attributes |= KTERM_ATTR_UNDERLINE;
                        else session->current_attributes &= ~KTERM_ATTR_UNDERLINE;
                    } else if (strcmp(key, "BLINK") == 0) {
                        if (v) session->current_attributes |= KTERM_ATTR_BLINK;
                        else session->current_attributes &= ~KTERM_ATTR_BLINK;
                    } else if (strcmp(key, "REVERSE") == 0) {
                        if (v) session->current_attributes |= KTERM_ATTR_REVERSE;
                        else session->current_attributes &= ~KTERM_ATTR_REVERSE;
                    } else if (strcmp(key, "HIDDEN") == 0) {
                        if (v) session->current_attributes |= KTERM_ATTR_CONCEAL;
                        else session->current_attributes &= ~KTERM_ATTR_CONCEAL;
                    } else if (strcmp(key, "STRIKE") == 0) {
                        if (v) session->current_attributes |= KTERM_ATTR_STRIKE;
                        else session->current_attributes &= ~KTERM_ATTR_STRIKE;
                    } else if (strcmp(key, "FG") == 0) {
                        session->current_fg.color_mode = 0;
                        session->current_fg.value.index = v & 0xFF;
                    } else if (strcmp(key, "BG") == 0) {
                        session->current_bg.color_mode = 0;
                        session->current_bg.value.index = v & 0xFF;
                    } else if (strcmp(key, "UL") == 0) {
                        // Simple parser for R,G,B or Index
                        int r, g, b;
                        if (sscanf(val, "%d,%d,%d", &r, &g, &b) == 3) {
                            session->current_ul_color.color_mode = 1; // RGB
                            session->current_ul_color.value.rgb = (RGB_KTermColor){r, g, b, 255};
                        } else {
                            session->current_ul_color.color_mode = 0;
                            session->current_ul_color.value.index = v & 0xFF;
                        }
                    } else if (strcmp(key, "ST") == 0) {
                        int r, g, b;
                        if (sscanf(val, "%d,%d,%d", &r, &g, &b) == 3) {
                            session->current_st_color.color_mode = 1; // RGB
                            session->current_st_color.value.rgb = (RGB_KTermColor){r, g, b, 255};
                        } else {
                            session->current_st_color.color_mode = 0;
                            session->current_st_color.value.index = v & 0xFF;
                        }
                    }
                }
                token = strtok(NULL, ";");
            }
            return true;
        } else if (strncmp(params, "GRID;", 5) == 0) {
            // SET;GRID;ON;R=255;G=0;...
            char grid_buffer[256];
            strncpy(grid_buffer, params + 5, sizeof(grid_buffer)-1);
            grid_buffer[255] = '\0';

            char* token = strtok(grid_buffer, ";");
            while (token) {
                if (strcmp(token, "ON") == 0) {
                    session->grid_enabled = true;
                } else if (strcmp(token, "OFF") == 0) {
                    session->grid_enabled = false;
                } else {
                    char* eq = strchr(token, '=');
                    if (eq) {
                        *eq = '\0';
                        char* key = token;
                        int v = atoi(eq + 1);
                        if (v < 0) v = 0; if (v > 255) v = 255;

                        if (strcmp(key, "R") == 0) session->grid_color.r = v;
                        else if (strcmp(key, "G") == 0) session->grid_color.g = v;
                        else if (strcmp(key, "B") == 0) session->grid_color.b = v;
                        else if (strcmp(key, "A") == 0) session->grid_color.a = v;
                    }
                }
                token = strtok(NULL, ";");
            }
            return true;
        } else if (strncmp(params, "CONCEAL;", 8) == 0) {
            // SET;CONCEAL;VALUE
            const char* val_str = params + 8;
            int val = atoi(val_str);
            if (val >= 0) session->conceal_char_code = (uint32_t)val;
            return true;
        } else if (strncmp(params, "BLINK;", 6) == 0) {
            // SET;BLINK;FAST=slot;SLOW=slot;BG=slot
            char blink_buffer[256];
            strncpy(blink_buffer, params + 6, sizeof(blink_buffer)-1);
            blink_buffer[255] = '\0';

            char* token = strtok(blink_buffer, ";");
            while (token) {
                char* eq = strchr(token, '=');
                if (eq) {
                    *eq = '\0';
                    char* key = token;
                    int v = atoi(eq + 1);
                    if (v > 0) {
                        if (strcmp(key, "FAST") == 0) session->fast_blink_rate = v;
                        else if (strcmp(key, "SLOW") == 0) session->slow_blink_rate = v;
                        else if (strcmp(key, "BG") == 0) session->bg_blink_rate = v;
                    }
                }
                token = strtok(NULL, ";");
            }
            return true;
        }

        // Params: PARAM;VALUE
        char p_buffer[256];
        strncpy(p_buffer, params, sizeof(p_buffer)-1);
        p_buffer[255] = '\0';

        char* param = p_buffer;
        char* val = strchr(param, ';');
        char* val2 = NULL;
        if (val) {
            *val = '\0';
            val++;
            val2 = strchr(val, ';');
            if (val2) {
                *val2 = '\0';
                val2++;
            }
        }

        if (param && val) {
            if (strcmp(param, "LEVEL") == 0) {
                int level = atoi(val);
                if (strcmp(val, "XTERM") == 0) level = VT_LEVEL_XTERM;
                KTerm_SetLevel(term, (VTLevel)level);
                return true;
            } else if (strcmp(param, "DEBUG") == 0) {
                bool enable = (strcmp(val, "ON") == 0 || strcmp(val, "1") == 0 || strcmp(val, "TRUE") == 0);
                KTerm_EnableDebug(term, enable);
                return true;
            } else if (strcmp(param, "OUTPUT") == 0) {
                bool enable = (strcmp(val, "ON") == 0 || strcmp(val, "1") == 0 || strcmp(val, "TRUE") == 0);
                session->response_enabled = enable;
                return true;
            } else if (strcmp(param, "FONT") == 0) {
                KTerm_SetFont(term, val);
                return true;
            } else if (strcmp(param, "SIZE") == 0 && val2) {
                int cols = atoi(val);
                int rows = atoi(val2);
                if (cols > 0 && rows > 0) {
                    KTerm_Resize(term, cols, rows);
                }
                return true;
            }
        }
    } else if (strcmp(command, "PIPE") == 0) {
        // PIPE;BANNER;...
        if (strncmp(params, "BANNER;", 7) == 0) {
            BannerOptions options;
            KTerm_ProcessBannerOptions(params + 7, &options);
            KTerm_GenerateBanner(term, session, &options);
            return true;
        }
    } else if (strcmp(command, "RESET") == 0) {
        if (strcmp(params, "ATTR") == 0) {
             // Reset to default attributes (White on Black, no flags)
             session->current_attributes = 0;
             session->current_fg.color_mode = 0;
             session->current_fg.value.index = COLOR_WHITE;
             session->current_bg.color_mode = 0;
             session->current_bg.value.index = COLOR_BLACK;
             return true;
        } else if (strcmp(params, "BLINK") == 0) {
             session->fast_blink_rate = 255;
             session->slow_blink_rate = 500;
             session->bg_blink_rate = 500;
             return true;
        }
    } else if (strcmp(command, "GET") == 0) {
        // Params: PARAM
        if (strcmp(params, "LEVEL") == 0) {
            char response[256];
            snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;LEVEL=%d\x1B\\", id, KTerm_GetLevel(term));
            KTerm_QueueResponse(term, response);
            return true;
        } else if (strcmp(params, "VERSION") == 0) {
            char response[256];
            snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;VERSION=2.2.20\x1B\\", id);
            KTerm_QueueResponse(term, response);
            return true;
        } else if (strcmp(params, "OUTPUT") == 0) {
            char response[256];
            snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;OUTPUT=%d\x1B\\", id, session->response_enabled ? 1 : 0);
            KTerm_QueueResponse(term, response);
            return true;
        } else if (strcmp(params, "FONTS") == 0) {
             char response[1024];
             snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;FONTS=", id);
             for (int i=0; available_fonts[i].name != NULL; i++) {
                 strncat(response, available_fonts[i].name, sizeof(response) - strlen(response) - 2);
                 if (available_fonts[i+1].name != NULL) strncat(response, ",", sizeof(response) - strlen(response) - 1);
             }
             strncat(response, "\x1B\\", sizeof(response) - strlen(response) - 1);
             KTerm_QueueResponse(term, response);
             return true;
        } else if (strcmp(params, "UNDERLINE_COLOR") == 0) {
            char response[256];
            if (session->current_ul_color.color_mode == 1) {
                RGB_KTermColor c = session->current_ul_color.value.rgb;
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;UNDERLINE_COLOR=%d,%d,%d\x1B\\", id, c.r, c.g, c.b);
            } else if (session->current_ul_color.color_mode == 2) {
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;UNDERLINE_COLOR=DEFAULT\x1B\\", id);
            } else {
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;UNDERLINE_COLOR=%d\x1B\\", id, session->current_ul_color.value.index);
            }
            KTerm_QueueResponse(term, response);
            return true;
        } else if (strcmp(params, "STRIKE_COLOR") == 0) {
            char response[256];
            if (session->current_st_color.color_mode == 1) {
                RGB_KTermColor c = session->current_st_color.value.rgb;
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;STRIKE_COLOR=%d,%d,%d\x1B\\", id, c.r, c.g, c.b);
            } else if (session->current_st_color.color_mode == 2) {
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;STRIKE_COLOR=DEFAULT\x1B\\", id);
            } else {
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;STRIKE_COLOR=%d\x1B\\", id, session->current_st_color.value.index);
            }
            KTerm_QueueResponse(term, response);
            return true;
        }
    }

    return true; // Consumed but unknown command for KTERM class
}

static void KTerm_ParseGatewayCommand(KTerm* term, KTermSession* session, const char* data, size_t len) {
    if (!data || len == 0) return;

    // Gateway Protocol Parser: DCS GATE <Class>;<ID>;<Command>[;<Params>] ST
    // Example: DCS GATE MAT;1;SET;COLOR;RED ST

    char buffer[512];
    if (len >= sizeof(buffer)) len = sizeof(buffer) - 1;
    strncpy(buffer, data, len);
    buffer[len] = '\0';

    // Use a custom tokenizer to avoid non-reentrant strtok and non-portable strtok_r
    // We scan for semicolons manually.

    char* class_id = buffer;
    char* next_ptr = strchr(class_id, ';');
    if (!next_ptr) return; // Malformed: Need at least Class;ID
    *next_ptr = '\0'; // Terminate Class

    char* id = next_ptr + 1;
    next_ptr = strchr(id, ';');
    if (!next_ptr) return; // Malformed: Need ID;Command
    *next_ptr = '\0'; // Terminate ID

    char* command = next_ptr + 1;
    char* params = "";

    next_ptr = strchr(command, ';');
    if (next_ptr) {
        *next_ptr = '\0'; // Terminate Command
        params = next_ptr + 1; // Remainder is Params
    } else {
        // No params, command is the end of the string.
        // Already null terminated by buffer logic.
    }

    if (class_id && id && command) {
        if (KTerm_ProcessInternalGatewayCommand(term, session, class_id, id, command, params ? params : "")) {
            return;
        }

        if (term->gateway_callback) {
            term->gateway_callback(term, class_id, id, command, params ? params : "");
        }
    } else {
        if (session->options.debug_sequences) {
            KTerm_LogUnsupportedSequence(term, "Invalid Gateway Command Format");
        }
    }
}

void KTerm_ExecuteDCSCommand(KTerm* term, KTermSession* session) {
    char* params = session->escape_buffer;

    if (strncmp(params, "1;1|", 4) == 0) {
        // DECUDK - User Defined Keys
        ProcessUserDefinedKeys(term, session, params + 4);
    } else if (strncmp(params, "0;1|", 4) == 0) {
        // DECUDK - Clear User Defined Keys
        ClearUserDefinedKeys(term, session);
    } else if (strncmp(params, "2;1|", 4) == 0) {
        // DECDLD - Download Soft Font (Variant?)
        ProcessSoftFontDownload(term, session, params + 4);
    } else if (strstr(params, "{") != NULL) {
        // Standard DECDLD - Download Soft Font (DCS ... { ...)
        // We pass the whole string, ProcessSoftFontDownload will handle tokenization
        ProcessSoftFontDownload(term, session, params);
    } else if (strncmp(params, "$q", 2) == 0) {
        // DECRQSS - Request Status String
        ProcessStatusRequest(term, session, params + 2);
    } else if (strncmp(params, "+q", 2) == 0) {
        // XTGETTCAP - Get Termcap
        ProcessTermcapRequest(term, session, params + 2);
    } else if (strncmp(params, "GATE", 4) == 0) {
        // Gateway Protocol
        // Format: DCS GATE <Class> ; <ID> ; <Command> ... ST
        // Skip "GATE" (4 bytes) and any immediate separator if present
        const char* payload = params + 4;
        if (*payload == ';') payload++;
        KTerm_ParseGatewayCommand(term, session, payload, strlen(payload));
    } else {
        if (session->options.debug_sequences) {
            KTerm_LogUnsupportedSequence(term, "Unknown DCS command");
        }
    }
}

// =============================================================================
// VT52 COMPATIBILITY MODE
// =============================================================================

void KTerm_ProcessHashChar(KTerm* term, KTermSession* session, unsigned char ch) {
    // DEC Line Attributes (ESC # Pn)

    // These commands apply to the *entire line* containing the active position.
    // In a real hardware terminal, this changes the scan-out logic.
    // Here, we set flags on all characters in the current row.
    // Note: Using term->width assumes fixed-width allocation, which matches
    // the current implementation of 'screen' in session->h. If dynamic resizing is
    // added, this should iterate up to session->width or similar.

    switch (ch) {
        case '3': // DECDHL - Double-height line, top half
            for (int x = 0; x < term->width; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(session, session->cursor.y, x);
                cell->flags &= ~KTERM_ATTR_DOUBLE_HEIGHT_BOT;
                cell->flags |= (KTERM_ATTR_DOUBLE_HEIGHT_TOP | KTERM_ATTR_DOUBLE_WIDTH | KTERM_FLAG_DIRTY);
            }
            session->row_dirty[session->cursor.y] = true;
            break;

        case '4': // DECDHL - Double-height line, bottom half
            for (int x = 0; x < term->width; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(session, session->cursor.y, x);
                cell->flags &= ~KTERM_ATTR_DOUBLE_HEIGHT_TOP;
                cell->flags |= (KTERM_ATTR_DOUBLE_HEIGHT_BOT | KTERM_ATTR_DOUBLE_WIDTH | KTERM_FLAG_DIRTY);
            }
            session->row_dirty[session->cursor.y] = true;
            break;

        case '5': // DECSWL - Single-width single-height line
            for (int x = 0; x < term->width; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(session, session->cursor.y, x);
                cell->flags &= ~(KTERM_ATTR_DOUBLE_HEIGHT_TOP | KTERM_ATTR_DOUBLE_HEIGHT_BOT | KTERM_ATTR_DOUBLE_WIDTH);
                cell->flags |= KTERM_FLAG_DIRTY;
            }
            session->row_dirty[session->cursor.y] = true;
            break;

        case '6': // DECDWL - Double-width single-height line
            for (int x = 0; x < term->width; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(session, session->cursor.y, x);
                cell->flags &= ~(KTERM_ATTR_DOUBLE_HEIGHT_TOP | KTERM_ATTR_DOUBLE_HEIGHT_BOT);
                cell->flags |= (KTERM_ATTR_DOUBLE_WIDTH | KTERM_FLAG_DIRTY);
            }
            session->row_dirty[session->cursor.y] = true;
            break;

        case '8': // DECALN - Screen Alignment Pattern
            // Fill screen with 'E'
            for (int y = 0; y < term->height; y++) {
                for (int x = 0; x < term->width; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(session, y, x);
                    cell->ch = 'E';
                    cell->fg_color = session->current_fg;
                    cell->bg_color = session->current_bg;
                    // Reset attributes
                    cell->flags = KTERM_FLAG_DIRTY;
                }
            }
            session->cursor.x = 0;
            session->cursor.y = 0;
            break;

        default:
            if (session->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC # %c", ch);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }

    session->parse_state = VT_PARSE_NORMAL;
}

void KTerm_ProcessPercentChar(KTerm* term, KTermSession* session, unsigned char ch) {
    // ISO 2022 Select Character Set (ESC % P)

    switch (ch) {
        case '@': // Select default (ISO 8859-1)
            session->charset.g0 = CHARSET_ISO_LATIN_1;
            session->charset.gl = &session->charset.g0;
            // Technically this selects the 'return to default' for ISO 2022.
            break;

        case 'G': // Select UTF-8 (ISO 2022 standard for UTF-8 level 1/2/3)
            session->charset.g0 = CHARSET_UTF8;
            session->charset.gl = &session->charset.g0;
            break;

        default:
             if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC %% %c", ch);
                KTerm_LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }

    GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
}

static void ReGIS_DrawLine(KTerm* term, int x0, int y0, int x1, int y1) {
    if (term->vector_count < term->vector_capacity) {
        GPUVectorLine* line = &term->vector_staging_buffer[term->vector_count];

        // Aspect Ratio Correction
        float logical_w = (float)(term->regis.screen_max_x - term->regis.screen_min_x + 1);
        float logical_h = (float)(term->regis.screen_max_y - term->regis.screen_min_y + 1);
        if (logical_w <= 0) logical_w = REGIS_WIDTH;
        if (logical_h <= 0) logical_h = REGIS_HEIGHT;

        float scale_factor = (float)(term->width * DEFAULT_CHAR_WIDTH) / logical_w;
        float target_height = logical_h * scale_factor;
        float y_margin = ((float)(term->height * DEFAULT_CHAR_HEIGHT) - target_height) / 2.0f;

        float screen_h = (float)(term->height * DEFAULT_CHAR_HEIGHT);
        float v0_px = y_margin + ((float)(y0 - term->regis.screen_min_y) * scale_factor);
        float v1_px = y_margin + ((float)(y1 - term->regis.screen_min_y) * scale_factor);

        // Y is inverted in ReGIS (0=Top) vs OpenGL UV (0=Bottom usually)
        float v0 = 1.0f - (v0_px / screen_h);
        float v1 = 1.0f - (v1_px / screen_h);

        line->x0 = (float)(x0 - term->regis.screen_min_x) / logical_w;
        line->y0 = v0;
        line->x1 = (float)(x1 - term->regis.screen_min_x) / logical_w;
        line->y1 = v1;

        line->color = term->regis.color;
        line->intensity = 1.0f;
        line->mode = term->regis.write_mode;
        term->vector_count++;
    }
}

static int ReGIS_CompareInt(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

static void ReGIS_FillPolygon(KTerm* term) {
    if (term->regis.point_count < 3) {
        term->regis.point_count = 0;
        return;
    }

    // Scanline Fill Algorithm
    int min_y = term->regis.screen_max_y, max_y = term->regis.screen_min_y;
    for(int i=0; i<term->regis.point_count; i++) {
        if (term->regis.point_buffer[i].y < min_y) min_y = term->regis.point_buffer[i].y;
        if (term->regis.point_buffer[i].y > max_y) max_y = term->regis.point_buffer[i].y;
    }
    if (min_y < term->regis.screen_min_y) min_y = term->regis.screen_min_y;
    if (max_y > term->regis.screen_max_y) max_y = term->regis.screen_max_y;

    int nodes[64];
    for (int y = min_y; y <= max_y; y++) {
        int node_count = 0;
        int j = term->regis.point_count - 1;
        for (int i = 0; i < term->regis.point_count; i++) {
            int y1 = term->regis.point_buffer[i].y;
            int y2 = term->regis.point_buffer[j].y;
            int x1 = term->regis.point_buffer[i].x;
            int x2 = term->regis.point_buffer[j].x;

            if ((y1 < y && y2 >= y) || (y2 < y && y1 >= y)) {
                if (node_count < 64) {
                    nodes[node_count++] = x1 + (int)((float)(y - y1) / (float)(y2 - y1) * (float)(x2 - x1));
                }
            }
            j = i;
        }

        qsort(nodes, node_count, sizeof(int), ReGIS_CompareInt);

        for (int i = 0; i < node_count; i += 2) {
            if (i + 1 < node_count) {
                int x_start = nodes[i] < term->regis.screen_min_x ? term->regis.screen_min_x : nodes[i];
                int x_end = nodes[i+1] > term->regis.screen_max_x ? term->regis.screen_max_x : nodes[i+1];
                if (x_start > term->regis.screen_max_x) break;
                if (x_end < term->regis.screen_min_x) continue;
                if (x_start < x_end) {
                     // Draw horizontal line span
                     ReGIS_DrawLine(term, x_start, y, x_end, y);
                }
            }
        }
    }
    term->regis.point_count = 0;
}

// Cubic B-Spline interpolation
static void ReGIS_EvalBSpline(KTerm* term, int p0x, int p0y, int p1x, int p1y, int p2x, int p2y, int p3x, int p3y, float t, int* out_x, int* out_y) {
    (void)term;
    float t2 = t * t;
    float t3 = t2 * t;
    float b0 = (-t3 + 3*t2 - 3*t + 1) / 6.0f;
    float b1 = (3*t3 - 6*t2 + 4) / 6.0f;
    float b2 = (-3*t3 + 3*t2 + 3*t + 1) / 6.0f;
    float b3 = t3 / 6.0f;

    *out_x = (int)(b0*p0x + b1*p1x + b2*p2x + b3*p3x);
    *out_y = (int)(b0*p0y + b1*p1y + b2*p2y + b3*p3y);
}

static void ExecuteReGISCommand(KTerm* term) {
    if (term->regis.command == 0) return;
    if (!term->regis.data_pending && term->regis.command != 'S' && term->regis.command != 'W' && term->regis.command != 'F' && term->regis.command != 'R') return;

    int max_idx = term->regis.param_count;

    // --- P: Position ---
    if (term->regis.command == 'P') {
        for (int i = 0; i <= max_idx; i += 2) {
            int val_x = term->regis.params[i];
            bool rel_x = term->regis.params_relative[i];
            int val_y = (i + 1 <= max_idx) ? term->regis.params[i+1] : term->regis.y;

            if (i+1 > max_idx) val_y = term->regis.y; // Fallback if Y completely missing in pair

            bool rel_y = (i + 1 <= max_idx) ? term->regis.params_relative[i+1] : false;

            int target_x = rel_x ? (term->regis.x + val_x) : val_x;
            int target_y = rel_y ? (term->regis.y + val_y) : val_y;

            // Clamp
            if (target_x < term->regis.screen_min_x) target_x = term->regis.screen_min_x;
            if (target_x > term->regis.screen_max_x) target_x = term->regis.screen_max_x;

            if (target_y < term->regis.screen_min_y) target_y = term->regis.screen_min_y;
            if (target_y > term->regis.screen_max_y) target_y = term->regis.screen_max_y;

            term->regis.x = target_x;
            term->regis.y = target_y;

            term->regis.point_count = 0;
        }
    }
    // --- V: Vector (Line) ---
    else if (term->regis.command == 'V') {
        for (int i = 0; i <= max_idx; i += 2) {
            int val_x = term->regis.params[i];
            bool rel_x = term->regis.params_relative[i];
            int val_y = (i + 1 <= max_idx) ? term->regis.params[i+1] : term->regis.y;
            if (i+1 > max_idx && !rel_x) val_y = term->regis.y;

            bool rel_y = (i + 1 <= max_idx) ? term->regis.params_relative[i+1] : false;

            int target_x = rel_x ? (term->regis.x + val_x) : val_x;
            int target_y = rel_y ? (term->regis.y + val_y) : val_y;


            if (target_x < term->regis.screen_min_x) target_x = term->regis.screen_min_x;
            if (target_x > term->regis.screen_max_x) target_x = term->regis.screen_max_x;
            if (target_y < term->regis.screen_min_y) target_y = term->regis.screen_min_y;
            if (target_y > term->regis.screen_max_y) target_y = term->regis.screen_max_y;

            ReGIS_DrawLine(term, term->regis.x, term->regis.y, target_x, target_y);

            term->regis.x = target_x;
            term->regis.y = target_y;
        }
        term->regis.point_count = 0;
    }
    // --- F: Polygon Fill ---
    else if (term->regis.command == 'F') {
        // Collect points but don't draw immediately
        for (int i = 0; i <= max_idx; i += 2) {
            int val_x = term->regis.params[i];
            bool rel_x = term->regis.params_relative[i];
            int val_y = (i + 1 <= max_idx) ? term->regis.params[i+1] : term->regis.y;
            bool rel_y = (i + 1 <= max_idx) ? term->regis.params_relative[i+1] : false;

            int px = rel_x ? (term->regis.x + val_x) : val_x;

            int py = rel_y ? (term->regis.y + val_y) : val_y;

            if (px < term->regis.screen_min_x) px = term->regis.screen_min_x;
            if (px > term->regis.screen_max_x) px = term->regis.screen_max_x;
            if (py < term->regis.screen_min_y) py = term->regis.screen_min_y;
            if (py > term->regis.screen_max_y) py = term->regis.screen_max_y;

            if (term->regis.point_count < 64) {
                 if (term->regis.point_count == 0) {
                     // First point is usually current cursor if implied?
                     // Standard F command might imply current position as start.
                     // We'll add current pos if buffer empty?
                     // ReGIS usually: F(V(P[x,y]...))
                     // Our F implementation just collects points passed to it.
                     // If point_count is 0, we should probably add current cursor?
                     term->regis.point_buffer[0].x = term->regis.x;
                     term->regis.point_buffer[0].y = term->regis.y;
                     term->regis.point_count++;
                 }
                 term->regis.point_buffer[term->regis.point_count].x = px;
                 term->regis.point_buffer[term->regis.point_count].y = py;
                 term->regis.point_count++;
            }
            term->regis.x = px;
            term->regis.y = py;
        }
    }
    // --- C: Circle / Curve ---
    else if (term->regis.command == 'C') {
        if (term->regis.option_command == 'B') {
            // --- B-Spline ---
            for (int i = 0; i <= max_idx; i += 2) {
                int val_x = term->regis.params[i];
                bool rel_x = term->regis.params_relative[i];
                int val_y = (i + 1 <= max_idx) ? term->regis.params[i+1] : term->regis.y;
                bool rel_y = (i + 1 <= max_idx) ? term->regis.params_relative[i+1] : false;

                int px = rel_x ? (term->regis.x + val_x) : val_x;
                int py = rel_y ? (term->regis.y + val_y) : val_y;

                if (term->regis.point_count < 64) {
                    if (term->regis.point_count == 0) {
                        term->regis.point_buffer[0].x = term->regis.x;
                        term->regis.point_buffer[0].y = term->regis.y;
                        term->regis.point_count++;
                    }
                    term->regis.point_buffer[term->regis.point_count].x = px;
                    term->regis.point_buffer[term->regis.point_count].y = py;
                    term->regis.point_count++;
                }
                term->regis.x = px;
                term->regis.y = py;
            }

            // Safety check to prevent infinite loops if points aren't consumed correctly
            if (term->regis.point_count >= 4) {
                for (int i = 0; i <= term->regis.point_count - 4; i++) {
                    int p0x = term->regis.point_buffer[i].x;   int p0y = term->regis.point_buffer[i].y;
                    int p1x = term->regis.point_buffer[i+1].x; int p1y = term->regis.point_buffer[i+1].y;
                    int p2x = term->regis.point_buffer[i+2].x; int p2y = term->regis.point_buffer[i+2].y;
                    int p3x = term->regis.point_buffer[i+3].x; int p3y = term->regis.point_buffer[i+3].y;

                    int seg_steps = 10;
                    int last_x = -1, last_y = -1;

                    for (int s=0; s<=seg_steps; s++) {
                        float t = (float)s / (float)seg_steps;
                        int tx, ty;
                        ReGIS_EvalBSpline(term, p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y, t, &tx, &ty);
                        if (last_x != -1) {
                            ReGIS_DrawLine(term, last_x, last_y, tx, ty);
                        }
                        last_x = tx;
                        last_y = ty;
                    }
                }
                int keep = 3;
                if (term->regis.point_count > keep) {
                    for(int k=0; k<keep; k++) {
                        term->regis.point_buffer[k] = term->regis.point_buffer[term->regis.point_count - keep + k];
                    }
                    term->regis.point_count = keep;
                }
            }
        }
        else if (term->regis.option_command == 'A') {
            // --- Arc ---
            if (max_idx >= 0) {
                int cx_val = term->regis.params[0];
                bool cx_rel = term->regis.params_relative[0];
                int cy_val = (1 <= max_idx) ? term->regis.params[1] : term->regis.y;
                bool cy_rel = (1 <= max_idx) ? term->regis.params_relative[1] : false;

                int cx = cx_rel ? (term->regis.x + cx_val) : cx_val;
                int cy = cy_rel ? (term->regis.y + cy_val) : cy_val;

                int sx = term->regis.x;
                int sy = term->regis.y;

                float dx = (float)(sx - cx);
                float dy = (float)(sy - cy);
                float radius = sqrtf(dx*dx + dy*dy);
                float start_angle = atan2f(dy, dx);

                float degrees = 0;
                if (max_idx >= 2) {
                    degrees = (float)term->regis.params[2];
                }

                int segments = (int)(fabsf(degrees) / 5.0f);
                if (segments < 4) segments = 4;
                float rad_step = (degrees * 3.14159f / 180.0f) / segments;

                float current_angle = start_angle;
                int last_x = sx;
                int last_y = sy;

                for (int i=0; i<segments; i++) {
                    current_angle += rad_step;
                    int nx = cx + (int)(cosf(current_angle) * radius);
                    int ny = cy + (int)(sinf(current_angle) * radius);
                    ReGIS_DrawLine(term, last_x, last_y, nx, ny);
                    last_x = nx;
                    last_y = ny;
                }

                term->regis.x = last_x;
                term->regis.y = last_y;
            }
        }
        else {
            // --- Standard Circle ---
            for (int i = 0; i <= max_idx; i += 2) {
                 int val1 = term->regis.params[i];
                 bool rel1 = term->regis.params_relative[i];

                 int radius = 0;
                 if (i + 1 > max_idx) {
                     radius = val1;
                 } else {
                     int val2 = term->regis.params[i+1];
                     bool rel2 = term->regis.params_relative[i+1];
                     int px = rel1 ? (term->regis.x + val1) : val1;
                     int py = rel2 ? (term->regis.y + val2) : val2;
                     float dx = (float)(px - term->regis.x);
                     float dy = (float)(py - term->regis.y);
                     radius = (int)sqrtf(dx*dx + dy*dy);
                 }

                 int cx = term->regis.x;
                 int cy = term->regis.y;
                 int segments = 32;
                 float angle_step = 6.283185f / segments;

                 for (int j = 0; j < segments; j++) {
                    if (term->vector_count >= term->vector_capacity) break;
                    float a1 = j * angle_step;
                    float a2 = (j + 1) * angle_step;

                    int x1 = cx + (int)(cosf(a1) * radius);
                    int y1 = cy + (int)(sinf(a1) * radius);
                    int x2 = cx + (int)(cosf(a2) * radius);
                    int y2 = cy + (int)(sinf(a2) * radius);

                    ReGIS_DrawLine(term, x1, y1, x2, y2);
                 }
            }
        }
    }
    // --- S: Screen Control ---
    else if (term->regis.command == 'S') {
        if (term->regis.option_command == 'E') {
             // Screen Erase (and optional addressing per prompt behavior)
             if (term->regis.param_count >= 3) {
                 // S(E[x1,y1][x2,y2]) - Set addressing extent and erase
                 term->regis.screen_min_x = term->regis.params[0];
                 term->regis.screen_min_y = term->regis.params[1];
                 term->regis.screen_max_x = term->regis.params[2];
                 term->regis.screen_max_y = term->regis.params[3];
             }
             term->vector_count = 0;
             term->vector_clear_request = true;
        } else if (term->regis.option_command == 'A') {
             // Screen Addressing S(A[x1,y1][x2,y2])
             if (term->regis.param_count >= 3) {
                 term->regis.screen_min_x = term->regis.params[0];
                 term->regis.screen_min_y = term->regis.params[1];
                 term->regis.screen_max_x = term->regis.params[2];
                 term->regis.screen_max_y = term->regis.params[3];
             }
        }
    }
    // --- W: Write Control ---
    else if (term->regis.command == 'W') {
        // Handle explicit KTermColor Index selection W(I...)
        if (term->regis.option_command == 'I') {
             int color_idx = term->regis.params[0];
             if (color_idx >= 0 && color_idx < 16) {
                 KTermColor c = ansi_colors[color_idx];
                 term->regis.color = (uint32_t)c.r | ((uint32_t)c.g << 8) | ((uint32_t)c.b << 16) | 0xFF000000;
             }
        }
        // Handle Writing Modes
        else if (term->regis.option_command == 'R') {
             term->regis.write_mode = 1; // Replace
        } else if (term->regis.option_command == 'E') {
             term->regis.write_mode = 2; // Erase
        } else if (term->regis.option_command == 'V') {
             term->regis.write_mode = 0; // Overlay (Additive)
        } else if (term->regis.option_command == 'C') {
             // W(C) is ambiguous: could be Complement or KTermColor
             // If we have parameters (e.g. W(C1)), treat as KTermColor (Legacy behavior).
             // If no parameters (e.g. W(C)), treat as Complement (XOR).

             if (term->regis.param_count > 0) {
                 // Likely KTermColor Index W(C1)
                 int color_idx = term->regis.params[0];
                 if (color_idx >= 0 && color_idx < 16) {
                     KTermColor c = ansi_colors[color_idx];
                     term->regis.color = (uint32_t)c.r | ((uint32_t)c.g << 8) | ((uint32_t)c.b << 16) | 0xFF000000;
                 }
             } else {
                 term->regis.write_mode = 3; // Complement (XOR)
             }
        }
    }
    // --- T: Text Attributes ---
    else if (term->regis.command == 'T') {
        if (term->regis.option_command == 'S') {
             // Size
             term->regis.text_size = (float)term->regis.params[0];
             if (term->regis.text_size <= 0) term->regis.text_size = 1;
        }
        if (term->regis.option_command == 'D') {
             // Direction (degrees)
             term->regis.text_angle = (float)term->regis.params[0] * 3.14159f / 180.0f;
        }
    }
    // --- L: Load Alphabet ---
    else if (term->regis.command == 'L') {
        // L command logic is primarily handled in ProcessReGISChar during string/hex parsing.
        // This block handles parameterized options like S (Size) if provided.
        if (term->regis.option_command == 'S') {
             // Character Cell Size (e.g. L(S1) or L(S[8,16]))
             int w = 8;
             int h = 16;
             if (term->regis.param_count >= 0) { // param_count is max index
                 // Check if it's an index or explicit size
                 if (term->regis.params[0] == 1) { w=8; h=16; }
                 else if (term->regis.params[0] == 0) { w=8; h=16; } // Default
                 else {
                     // Assume width
                     w = term->regis.params[0];
                     if (term->regis.param_count >= 1) h = term->regis.params[1];
                 }
             }
             GET_SESSION(term)->soft_font.char_width = w;
             GET_SESSION(term)->soft_font.char_height = h;
        } else if (term->regis.option_command == 'A') {
             // Alphabet selection L(A1)
             if (term->regis.param_count >= 0) {
                 int alpha = term->regis.params[0];
                 // We only really support loading into "soft font" slot (conceptually A1)
                 // A0 is typically the hardware ROM font.
                 if (alpha != 1) {
                     if (GET_SESSION(term)->options.debug_sequences) {
                         char msg[64];
                         snprintf(msg, sizeof(msg), "ReGIS Load: Alphabet A%d not supported (only A1)", alpha);
                         KTerm_LogUnsupportedSequence(term, msg);
                     }
                 }
             }
        }
    }
    // --- R: Report ---
    else if (term->regis.command == 'R') {
         if (term->regis.option_command == 'P') {
             char buf[64];
             snprintf(buf, sizeof(buf), "\x1BP%d,%d\x1B\\", term->regis.x, term->regis.y);
             KTerm_QueueResponse(term, buf);
         }
    }

    term->regis.data_pending = false;
}

static void ProcessReGISChar(KTerm* term, KTermSession* session, unsigned char ch) {
    if (ch == 0x1B) { // ESC \ (ST)
        if (term->regis.command == 'F') ReGIS_FillPolygon(term); // Flush pending fill
        if (term->regis.state == 1 || term->regis.state == 3) {
            ExecuteReGISCommand(term);
        }
        session->parse_state = VT_PARSE_ESCAPE;
        return;
    }

    if (term->regis.recording_macro) {
        if (ch == ';' && term->regis.macro_len > 0 && term->regis.macro_buffer[term->regis.macro_len-1] == '@') {
             // End of macro definition (@;)
             term->regis.macro_buffer[term->regis.macro_len-1] = '\0'; // Remove @
             term->regis.recording_macro = false;
             // Store macro in slot
             if (term->regis.macro_index >= 0 && term->regis.macro_index < 26) {
                 if (term->regis.macros[term->regis.macro_index]) free(term->regis.macros[term->regis.macro_index]);
                 term->regis.macros[term->regis.macro_index] = strdup(term->regis.macro_buffer);
             }
             if (term->regis.macro_buffer) { free(term->regis.macro_buffer); term->regis.macro_buffer = NULL; }
             return;
        }
        // Append
        if (!term->regis.macro_buffer) {
             term->regis.macro_cap = 1024;
             term->regis.macro_buffer = malloc(term->regis.macro_cap);
             term->regis.macro_len = 0;
        }

        // Enforce Macro Space Limit
        size_t limit = session->macro_space.total > 0 ? session->macro_space.total : 4096;
        if (term->regis.macro_len >= limit) {
             if (session->options.debug_sequences) {
                 KTerm_LogUnsupportedSequence(term, "ReGIS Macro storage limit exceeded");
             }
             // Stop recording, discard macro? Or just truncate?
             // Standard behavior: typically stops recording or overwrites?
             // Safest: Stop appending.
             return;
        }

        if (term->regis.macro_len >= term->regis.macro_cap - 1) {
             size_t new_cap = term->regis.macro_cap * 2;
             if (new_cap > limit + 1) new_cap = limit + 1; // +1 for null terminator

             if (new_cap > term->regis.macro_cap) {
                 term->regis.macro_cap = new_cap;
                 term->regis.macro_buffer = realloc(term->regis.macro_buffer, term->regis.macro_cap);
             }
        }

        // Final check against capacity to be safe
        if (term->regis.macro_len < term->regis.macro_cap - 1) {
            term->regis.macro_buffer[term->regis.macro_len++] = ch;
            term->regis.macro_buffer[term->regis.macro_len] = '\0';
        }
        return;
    }

    if (term->regis.state == 3) { // Parsing Text String
        if (ch == term->regis.string_terminator) {
            term->regis.text_buffer[term->regis.text_pos] = '\0';

            if (term->regis.command == 'L') {
                // Load Alphabet Logic
                if (term->regis.option_command == 'A') {
                    // Set Alphabet Name
                    strncpy(term->regis.load.name, term->regis.text_buffer, 15);
                    term->regis.load.name[15] = '\0';
                    term->regis.option_command = 0; // Reset
                } else {
                    // Define Character
                    if (term->regis.text_pos > 0) {
                        term->regis.load.current_char = (unsigned char)term->regis.text_buffer[0];
                        term->regis.load.pattern_byte_idx = 0;
                        term->regis.load.hex_nibble = -1;
                        // Clear existing pattern for this char
                        memset(session->soft_font.font_data[term->regis.load.current_char], 0, 32);
                        session->soft_font.loaded[term->regis.load.current_char] = true;
                        session->soft_font.active = true;
                    }
                }
            } else {
                // Text Drawing with attributes
                float scale = (term->regis.text_size > 0) ? term->regis.text_size : 1.0f;
                // Base scale 1 = 8x16? ReGIS default size 1 is roughly 9x16 grid?
                // Existing code used scale 2.0f. Let's base it on that.
                scale *= 2.0f;

                float cos_a = cosf(term->regis.text_angle);
                float sin_a = sinf(term->regis.text_angle);

                int start_x = term->regis.x;
                int start_y = term->regis.y;

                const unsigned char* font_base = vga_perfect_8x8_font;
                bool use_soft_font = session->soft_font.active;

                for(int i=0; term->regis.text_buffer[i] != '\0'; i++) {
                    unsigned char c = (unsigned char)term->regis.text_buffer[i];

                    // Use dynamic height if soft font is active
                    int max_rows = use_soft_font ? session->soft_font.char_height : 16;
                    if (max_rows > 32) max_rows = 32;

                    for(int r=0; r<max_rows; r++) { // Iterate up to max_rows
                        unsigned char row = 0;
                        int height_limit = 8; // Default to 8 for VGA font

                        if (use_soft_font && session->soft_font.loaded[c]) {
                            row = session->soft_font.font_data[c][r];
                            height_limit = session->soft_font.char_height;
                        } else {
                            if (r < 8) row = font_base[c * 8 + r];
                            else row = 0;
                        }

                        if (r >= height_limit) continue;

                        for(int c_bit=0; c_bit<8; c_bit++) {
                            if ((row >> (7-c_bit)) & 1) {
                                int len = 1;
                                while(c_bit+len < 8 && ((row >> (7-(c_bit+len))) & 1)) len++;

                                // Local coordinates relative to char origin
                                float lx0 = (float)(c_bit * scale);
                                float ly0 = (float)(r * scale * (height_limit == 8 ? 1.5f : 0.75f)); // Adjust aspect for 16px
                                float lx1 = (float)((c_bit + len) * scale);
                                float ly1 = ly0;

                                // Rotate and translate
                                // Character offset
                                float char_offset = i * 9 * scale;

                                float rx0 = lx0 + char_offset;
                                float rx1 = lx1 + char_offset;

                                float fx0 = start_x + (rx0 * cos_a - ly0 * sin_a);
                                float fy0 = start_y + (rx0 * sin_a + ly0 * cos_a);
                                float fx1 = start_x + (rx1 * cos_a - ly1 * sin_a);
                                float fy1 = start_y + (rx1 * sin_a + ly1 * cos_a);

                                if (term->vector_count < term->vector_capacity) {
                                    GPUVectorLine* line = &term->vector_staging_buffer[term->vector_count];
                                    line->x0 = fx0 / ((float)REGIS_WIDTH);
                                    line->y0 = 1.0f - (fy0 / ((float)REGIS_HEIGHT));
                                    line->x1 = fx1 / ((float)REGIS_WIDTH);
                                    line->y1 = 1.0f - (fy1 / ((float)REGIS_HEIGHT));
                                    line->color = term->regis.color;
                                    line->intensity = 1.0f;
                                    line->mode = term->regis.write_mode;
                                    term->vector_count++;
                                 }
                                c_bit += len - 1;
                            }
                        }
                    }
                }
                // Update cursor position to end of string
                float total_width = term->regis.text_pos * 9 * scale;
                term->regis.x = start_x + (int)(total_width * cos_a);
                term->regis.y = start_y + (int)(total_width * sin_a);
            }

            term->regis.state = 1;
            term->regis.text_pos = 0;
        } else {
            if (term->regis.text_pos < 255) {
                term->regis.text_buffer[term->regis.text_pos++] = ch;
            }
        }
        return;
    }

    if (ch <= 0x20 || ch == 0x7F) return;

    if (term->regis.state == 0) { // Expecting Command
        if (ch == '@') {
            // Macro
            term->regis.command = '@';
            term->regis.state = 1;
            return;
        }
        if (isalpha(ch)) {
            term->regis.command = toupper(ch);
            term->regis.state = 1;
            term->regis.param_count = 0;
            term->regis.has_bracket = false;
            term->regis.has_paren = false;
            term->regis.point_count = 0; // Reset curve points on new command
            for(int i=0; i<16; i++) {
                term->regis.params[i] = 0;
                term->regis.params_relative[i] = false;
            }
        }
    } else if (term->regis.state == 1) { // Expecting Values/Options
        if (term->regis.command == '@') {
             if (ch == ':') {
                 // Definition
                 term->regis.option_command = ':'; // Flag next char as macro name
                 return;
             }
             if (term->regis.option_command == ':') {
                 // Macro Name
                 if (isalpha(ch)) {
                     term->regis.macro_index = toupper(ch) - 'A';
                     term->regis.recording_macro = true;
                     term->regis.macro_len = 0;
                     term->regis.option_command = 0;
                 }
                 return;
             }
             // Execute Macro
             if (isalpha(ch)) {
                 int idx = toupper(ch) - 'A';
                 if (idx >= 0 && idx < 26 && term->regis.macros[idx]) {
                     if (term->regis.recursion_depth < 16) {
                         term->regis.recursion_depth++;
                         // Push macro content to parser
                         const char* m = term->regis.macros[idx];
                         // Reset state to 0 for macro context?
                         // Macros usually contain full commands.
                         int saved_state = term->regis.state;
                         term->regis.state = 0;
                         for (int k=0; m[k]; k++) ProcessReGISChar(term, session, m[k]);
                         term->regis.state = saved_state;
                         term->regis.recursion_depth--;
                     } else {
                         if (session->options.debug_sequences) {
                             KTerm_LogUnsupportedSequence(term, "ReGIS Macro recursion depth exceeded");
                         }
                     }
                 }
                 term->regis.command = 0;
                 term->regis.state = 0;
             }
             return;
        }

        if (ch == '\'' || ch == '"') {
             if (term->regis.command == 'T' || term->regis.command == 'L') {
                 term->regis.state = 3;
                 term->regis.string_terminator = ch;
                 term->regis.text_pos = 0;
                 return;
             }
        }
        if (ch == '[') {
            term->regis.has_bracket = true;
            term->regis.has_comma = false;
            term->regis.parsing_val = false;
        } else if (ch == ']') {
            if (term->regis.parsing_val) {
                 term->regis.params[term->regis.param_count] = term->regis.current_sign * term->regis.current_val;
                 term->regis.params_relative[term->regis.param_count] = term->regis.val_is_relative;
            }
            term->regis.parsing_val = false;
            term->regis.has_bracket = false;

            // Special case for S command: Accumulate parameters for [x1,y1][x2,y2]
            if (term->regis.command != 'S') {
                ExecuteReGISCommand(term);
                term->regis.param_count = 0;
                for(int i=0; i<16; i++) {
                    term->regis.params[i] = 0;
                    term->regis.params_relative[i] = false;
                }
            } else {
                // Prepare for next parameter set
                if (term->regis.param_count < 15) {
                    term->regis.param_count++;
                    term->regis.params[term->regis.param_count] = 0;
                    term->regis.params_relative[term->regis.param_count] = false;
                }
            }
        } else if (ch == '(') {
            term->regis.has_paren = true;
            term->regis.parsing_val = false;
        } else if (ch == ')') {
            if (term->regis.parsing_val) {
                 term->regis.params[term->regis.param_count] = term->regis.current_sign * term->regis.current_val;
                 term->regis.params_relative[term->regis.param_count] = term->regis.val_is_relative;
            }
            term->regis.has_paren = false;
            term->regis.parsing_val = false;
            ExecuteReGISCommand(term);
            // Don't reset point count here, as options might modify curve mode
            // But we reset param count for next block
            term->regis.param_count = 0;
            for(int i=0; i<16; i++) {
                term->regis.params[i] = 0;
                term->regis.params_relative[i] = false;
            }
        } else if (term->regis.command == 'L' && isxdigit(ch)) {
            // Hex parsing for Load Alphabet
            // Assuming ReGIS "hex string" format (pairs of hex digits)
            int val = 0;
            if (ch >= '0' && ch <= '9') val = ch - '0';
            else if (ch >= 'A' && ch <= 'F') val = ch - 'A' + 10;
            else if (ch >= 'a' && ch <= 'f') val = ch - 'a' + 10;

            if (term->regis.load.hex_nibble == -1) {
                term->regis.load.hex_nibble = val;
            } else {
                int byte = (term->regis.load.hex_nibble << 4) | val;
                term->regis.load.hex_nibble = -1;

                if (term->regis.load.pattern_byte_idx < 32) {
                    GET_SESSION(term)->soft_font.font_data[term->regis.load.current_char][term->regis.load.pattern_byte_idx++] = byte;
                }
            }
            // Defer texture update to KTerm_Draw
            GET_SESSION(term)->soft_font.dirty = true;

        } else if (isdigit(ch) || ch == '-' || ch == '+') {
            if (!term->regis.parsing_val) {
                term->regis.parsing_val = true;
                term->regis.current_val = 0;
                term->regis.current_sign = 1;
                term->regis.val_is_relative = false;
            }
            if (ch == '-') {
                term->regis.current_sign = -1;
                term->regis.val_is_relative = true;
            } else if (ch == '+') {
                term->regis.current_sign = 1;
                term->regis.val_is_relative = true;
            } else if (isdigit(ch)) {
                // Protect against integer overflow
                if (term->regis.current_val < 100000000) { // Reasonable limit for coords/params
                    term->regis.current_val = term->regis.current_val * 10 + (ch - '0');
                }
            }
            term->regis.params[term->regis.param_count] = term->regis.current_sign * term->regis.current_val;
            term->regis.params_relative[term->regis.param_count] = term->regis.val_is_relative;
            term->regis.data_pending = true;
        } else if (ch == ',') {
            if (term->regis.parsing_val) {
                term->regis.params[term->regis.param_count] = term->regis.current_sign * term->regis.current_val;
                term->regis.params_relative[term->regis.param_count] = term->regis.val_is_relative;
                term->regis.parsing_val = false;
            }
            if (term->regis.param_count < 15) {
                term->regis.param_count++;
                term->regis.params[term->regis.param_count] = 0;
                term->regis.params_relative[term->regis.param_count] = false;
            }
            term->regis.has_comma = true;
        } else if (isalpha(ch)) {
            if (term->regis.has_paren) {
                 term->regis.option_command = toupper(ch);
                 term->regis.param_count = 0;
                 term->regis.parsing_val = false;
            } else {
                if (term->regis.command == 'F') ReGIS_FillPolygon(term); // Flush fill on new command
                ExecuteReGISCommand(term);
                term->regis.command = toupper(ch);
                term->regis.state = 1;
                term->regis.param_count = 0;
                term->regis.parsing_val = false;
                term->regis.point_count = 0; // Reset on new command
                for(int i=0; i<16; i++) {
                    term->regis.params[i] = 0;
                    term->regis.params_relative[i] = false;
                }
            }
        }
    }
}

static void ProcessTektronixChar(KTerm* term, KTermSession* session, unsigned char ch) {
    // 1. Escape Sequence Escape
    if (ch == 0x1B) {
        if (session->dec_modes.vt52_mode) {
            session->parse_state = PARSE_VT52;
        } else {
            session->parse_state = VT_PARSE_ESCAPE;
        }
        return;
    }

    // 2. Control Codes
    if (ch == 0x1D) { // GS - Graph Mode
        term->tektronix.state = 1; // Graph
        term->tektronix.pen_down = false; // First coord is Dark (Move)
        term->tektronix.extra_byte = -1;
        return;
    }
    if (ch == 0x1F) { // US - Alpha Mode (Text)
        term->tektronix.state = 0; // Alpha
        return;
    }
    if (ch == 0x0C) { // FF - Clear Screen
        term->vector_count = 0;
        term->tektronix.pen_down = false;
        term->tektronix.extra_byte = -1;
        return;
    }
    if (ch < 0x20) {
        if (term->tektronix.state == 0) KTerm_ProcessControlChar(term, session, ch);
        return;
    }

    // 3. Alpha Mode Handling
    if (term->tektronix.state == 0) {
        KTerm_ProcessNormalChar(term, session, ch);
        return;
    }

    // 4. Graph Mode Coordinate Parsing (12-bit Support)
    // Categories:
    // HiY:   0x20 - 0x3F (001xxxxx) - Sets High Y (5 bits), Closes Y
    // Extra: 0x60 - 0x7F (011xxxxx) - Intermediate LSBs (optional)
    // LoY:   0x60 - 0x7F (011xxxxx) - Sets Low Y (5 bits)
    // HiX:   0x20 - 0x3F (001xxxxx) - Sets High X (5 bits)
    // LoX:   0x40 - 0x5F (010xxxxx) - Sets Low X (5 bits) AND DRAWS

    int val = ch & 0x1F;

    if (ch >= 0x20 && ch <= 0x3F) {
        // HiY or HiX
        if (term->tektronix.sub_state == 1) { // Previous was LoY/Extra
            // Interpret as HiX
            // HiX bits (7-11)
            term->tektronix.holding_x = (term->tektronix.holding_x & 0x07F) | (val << 7);
            term->tektronix.sub_state = 2; // Seen HiX
            term->tektronix.extra_byte = -1; // Reset extra
        } else {
            // Interpret as HiY
            // HiY bits (7-11)
            term->tektronix.holding_y = (term->tektronix.holding_y & 0x07F) | (val << 7);
            term->tektronix.sub_state = 0; // Seen HiY
            term->tektronix.extra_byte = -1; // Reset extra
        }
    } else if (ch >= 0x60 && ch <= 0x7F) {
        // LoY or Extra Byte
        if (term->tektronix.extra_byte != -1) {
            // We already have a buffered byte -> It was Extra.
            int eb = term->tektronix.extra_byte;
            // Decode Extra Byte (Bits 1-5 of eb correspond to LSBs)
            // Tek 4014 Extra Byte Format:
            // Bits 4-3: Y LSBs (Bits 0-1 of Y)
            // Bits 2-1: X LSBs (Bits 0-1 of X)
            int x_lsb = eb & 0x03;
            int y_lsb = (eb >> 2) & 0x03;

            // Apply LSBs (Bits 0-1)
            term->tektronix.holding_x = (term->tektronix.holding_x & ~0x03) | x_lsb;
            term->tektronix.holding_y = (term->tektronix.holding_y & ~0x03) | y_lsb;

            // Current 'val' is the real LoY (Bits 2-6)
            term->tektronix.holding_y = (term->tektronix.holding_y & ~0x07C) | (val << 2);

            term->tektronix.extra_byte = -1; // Consumed
            term->tektronix.sub_state = 1; // LoY processed
        } else {
            // Potential LoY or Extra.
            term->tektronix.extra_byte = val; // Store raw value

            // Apply as LoY (Standard 10-bit behavior compat)
            term->tektronix.holding_y = (term->tektronix.holding_y & ~0x07C) | (val << 2);
            term->tektronix.sub_state = 1; // Flag: Next could be HiX
        }
    } else if (ch >= 0x40 && ch <= 0x5F) {
        // LoX - Trigger
        term->tektronix.holding_x = (term->tektronix.holding_x & ~0x07C) | (val << 2);

        // Reset extra byte (sequence ended)
        term->tektronix.extra_byte = -1;

        // DRAW
        if (term->tektronix.pen_down) {
            if (term->vector_count < term->vector_capacity) {
                GPUVectorLine* line = &term->vector_staging_buffer[term->vector_count];

                // 12-bit Coordinate Normalization (0-4095 -> 0.0-1.0)
                float norm_x1 = (float)term->tektronix.x / 4096.0f;
                float norm_y1 = (float)term->tektronix.y / 4096.0f;
                float norm_x2 = (float)term->tektronix.holding_x / 4096.0f;
                float norm_y2 = (float)term->tektronix.holding_y / 4096.0f;

                // Flip Y (Tektronix 0,0 is bottom-left)
                norm_y1 = 1.0f - norm_y1;
                norm_y2 = 1.0f - norm_y2;

                line->x0 = norm_x1;
                line->y0 = norm_y1;
                line->x1 = norm_x2;
                line->y1 = norm_y2;
                line->color = 0xFF00FF00; // Bright Green
                line->intensity = 1.0f;
                line->mode = 0; // Additive

                term->vector_count++;
            }
        }

        // Update Position
        term->tektronix.x = term->tektronix.holding_x;
        term->tektronix.y = term->tektronix.holding_y;
        term->tektronix.pen_down = true;
        term->tektronix.sub_state = 0;
    }
}

static void KTerm_PrepareKittyUpload(KTerm* term, KTermSession* session) {
    KittyGraphics* kitty = &session->kitty;

    // Check if we are continuing an upload (Chunked)
    if (kitty->active_upload && kitty->continuing) {
         return;
    }

    if (kitty->cmd.action == 't' || kitty->cmd.action == 'T' || kitty->cmd.action == 'f') {
        KittyImageBuffer* img = NULL;

        // Ensure images array exists
        if (!kitty->images) {
            kitty->image_capacity = 64;
            kitty->images = calloc(kitty->image_capacity, sizeof(KittyImageBuffer));
        }

        // Find existing image
        for (int i = 0; i < kitty->image_count; i++) {
            if (kitty->images[i].id == kitty->cmd.id) {
                img = &kitty->images[i];
                break;
            }
        }

        if (kitty->cmd.action == 'f') {
            if (!img) return; // Error: Image not found for frame load
        } else {
            // 't' or 'T' - Create new or replace
            if (img) {
                // Free existing frames
                if (img->frames) {
                    for (int f = 0; f < img->frame_count; f++) {
                        if (img->frames[f].data) {
                            if (kitty->current_memory_usage >= img->frames[f].capacity)
                                kitty->current_memory_usage -= img->frames[f].capacity;
                            free(img->frames[f].data);
                        }
                        if (img->frames[f].texture.id != 0) KTerm_DestroyTexture(&img->frames[f].texture);
                    }
                    free(img->frames);
                }
                // Reset image
                img->frames = NULL;
                img->frame_count = 0;
                img->frame_capacity = 0;
            } else {
                // New Image
                if (kitty->image_count >= kitty->image_capacity) {
                    // Resize images array
                    int new_cap = (kitty->image_capacity == 0) ? 16 : kitty->image_capacity * 2;
                    KittyImageBuffer* new_images = realloc(kitty->images, new_cap * sizeof(KittyImageBuffer));
                    if (!new_images) return; // OOM
                    kitty->images = new_images;
                    kitty->image_capacity = new_cap;
                }
                img = &kitty->images[kitty->image_count++];
                memset(img, 0, sizeof(KittyImageBuffer));
                img->id = kitty->cmd.id;
            }

            // Set properties for t/T
            img->visible = (kitty->cmd.action != 't');

            // Defaults
            int char_w = DEFAULT_CHAR_WIDTH;
            int char_h = DEFAULT_CHAR_HEIGHT;
            if (session->soft_font.active) {
                char_w = session->soft_font.char_width;
                char_h = session->soft_font.char_height;
            }

            if (kitty->cmd.has_x) img->x = kitty->cmd.x;
            else img->x = session->cursor.x * char_w;

            if (kitty->cmd.has_y) img->y = kitty->cmd.y;
            else img->y = session->cursor.y * char_h;

            img->start_row = session->screen_head;
            img->z_index = kitty->cmd.z_index;
            img->complete = false;
        }

        // Add new frame
        if (img->frame_count >= img->frame_capacity) {
            int new_cap = (img->frame_capacity == 0) ? 4 : img->frame_capacity * 2;
            KittyFrame* new_frames = realloc(img->frames, new_cap * sizeof(KittyFrame));
            if (!new_frames) return;
            img->frames = new_frames;
            img->frame_capacity = new_cap;
        }

        KittyFrame* frame = &img->frames[img->frame_count++];
        memset(frame, 0, sizeof(KittyFrame));
        frame->width = kitty->cmd.width;
        frame->height = kitty->cmd.height;
        // z for 'f' is delay
        if (kitty->cmd.action == 'f') {
             frame->delay_ms = kitty->cmd.z_index;
             if (frame->delay_ms < 0) frame->delay_ms = 0; // Or global? Assume local for now
        }

        size_t initial_cap = 4096;
        if (kitty->current_memory_usage + initial_cap <= KTERM_KITTY_MEMORY_LIMIT) {
            frame->capacity = initial_cap;
            frame->data = malloc(frame->capacity);
            frame->size = 0;
            if (frame->data) {
                kitty->current_memory_usage += initial_cap;
                kitty->active_upload = img;
            } else {
                img->frame_count--;
                kitty->active_upload = NULL;
            }
        } else {
             if (session->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Kitty: Memory limit exceeded");
             kitty->active_upload = NULL;
        }
    }
}

static void KTerm_ParseKittyPair(KTermSession* session) {
    KittyGraphics* kitty = &session->kitty;
    char* key = kitty->key_buffer;
    char* val = kitty->val_buffer;
    int v = atoi(val);

    if (strcmp(key, "a") == 0) kitty->cmd.action = val[0];
    else if (strcmp(key, "d") == 0) kitty->cmd.delete_action = val[0];
    else if (strcmp(key, "f") == 0) kitty->cmd.format = v;
    else if (strcmp(key, "s") == 0) kitty->cmd.width = v;
    else if (strcmp(key, "v") == 0) kitty->cmd.height = v;
    else if (strcmp(key, "i") == 0) kitty->cmd.id = (uint32_t)v;
    else if (strcmp(key, "p") == 0) kitty->cmd.placement_id = (uint32_t)v;
    else if (strcmp(key, "x") == 0) { kitty->cmd.x = v; kitty->cmd.has_x = true; }
    else if (strcmp(key, "y") == 0) { kitty->cmd.y = v; kitty->cmd.has_y = true; }
    else if (strcmp(key, "z") == 0) kitty->cmd.z_index = v;
    else if (strcmp(key, "t") == 0) kitty->cmd.transmission_type = val[0];
    else if (strcmp(key, "m") == 0) kitty->cmd.medium = v;
    else if (strcmp(key, "q") == 0) kitty->cmd.quiet = (v != 0);
}

void KTerm_ProcessKittyChar(KTerm* term, KTermSession* session, unsigned char ch) {
    KittyGraphics* kitty = &session->kitty;

    // Handle ESC to terminate sequence (ST)
    if (ch == '\x1B') {
        // Flush pending value if in VALUE state
        if (kitty->state == 1) {
             kitty->val_buffer[kitty->val_len] = '\0';
             KTerm_ParseKittyPair(session);
        }
        session->saved_parse_state = PARSE_KITTY;
        session->parse_state = PARSE_STRING_TERMINATOR;
        return;
    }

    if (kitty->state == 0) { // KEY
        if (ch == '=') {
            kitty->key_buffer[kitty->key_len] = '\0';
            kitty->state = 1; // VALUE
            kitty->val_len = 0;
        } else if (ch == ',' || ch == ';') {
            kitty->key_len = 0; // Reset
            if (ch == ';') {
                kitty->state = 2; // PAYLOAD
                KTerm_PrepareKittyUpload(term, session);
            }
        } else {
            if (kitty->key_len < 31) kitty->key_buffer[kitty->key_len++] = ch;
        }
    } else if (kitty->state == 1) { // VALUE
        if (ch == ',' || ch == ';') {
            kitty->val_buffer[kitty->val_len] = '\0';
            KTerm_ParseKittyPair(session);
            kitty->state = 0; // Back to KEY
            kitty->key_len = 0;

            if (ch == ';') {
                kitty->state = 2; // PAYLOAD
                KTerm_PrepareKittyUpload(term, session);
            }
        } else {
            if (kitty->val_len < 127) kitty->val_buffer[kitty->val_len++] = ch;
        }
    } else if (kitty->state == 2) { // PAYLOAD (Base64)
        int val = -1;
        if (ch >= 'A' && ch <= 'Z') val = ch - 'A';
        else if (ch >= 'a' && ch <= 'z') val = ch - 'a' + 26;
        else if (ch >= '0' && ch <= '9') val = ch - '0' + 52;
        else if (ch == '+') val = 62;
        else if (ch == '/') val = 63;

        if (val != -1) {
            kitty->b64_accumulator = (kitty->b64_accumulator << 6) | val;
            kitty->b64_bits += 6;

            if (kitty->b64_bits >= 8) {
                kitty->b64_bits -= 8;
                unsigned char byte = (kitty->b64_accumulator >> kitty->b64_bits) & 0xFF;

                if (kitty->active_upload && kitty->active_upload->frame_count > 0) {
                    KittyFrame* frame = &kitty->active_upload->frames[kitty->active_upload->frame_count - 1];
                    if (frame->data) {
                        if (frame->size >= frame->capacity) {
                            size_t new_cap = frame->capacity * 2;
                            if (kitty->current_memory_usage + (new_cap - frame->capacity) <= KTERM_KITTY_MEMORY_LIMIT) {
                                unsigned char* new_data = realloc(frame->data, new_cap);
                                if (new_data) {
                                    kitty->current_memory_usage += (new_cap - frame->capacity);
                                    frame->data = new_data;
                                    frame->capacity = new_cap;
                                } else {
                                    // Realloc failed, stop uploading
                                    kitty->active_upload = NULL;
                                }
                            } else {
                                 // Limit exceeded
                                 if (session->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Kitty: Memory limit exceeded during upload");
                                 kitty->active_upload = NULL;
                            }
                        }

                        if (kitty->active_upload && frame->size < frame->capacity) {
                            frame->data[frame->size++] = byte;
                        }
                    }
                }
            }
        }
    }
}

void KTerm_ExecuteKittyCommand(KTerm* term, KTermSession* session) {
    KittyGraphics* kitty = &session->kitty;

    // Chunked Transmission logic
    kitty->continuing = (kitty->cmd.medium == 1);
    if (kitty->active_upload) {
        kitty->active_upload->complete = !kitty->continuing;
    }

    if (kitty->cmd.action == 't' || kitty->cmd.action == 'T' || kitty->cmd.action == 'p') {
        // Placement Logic (Update x, y, z for ID)
        if (kitty->images) {
            for(int i=0; i<kitty->image_count; i++) {
                if (kitty->images[i].id == kitty->cmd.id) {
                    // Found image, update placement if provided
                    if (kitty->cmd.has_x) kitty->images[i].x = kitty->cmd.x;
                    if (kitty->cmd.has_y) kitty->images[i].y = kitty->cmd.y;
                    if (kitty->cmd.z_index != 0) kitty->images[i].z_index = kitty->cmd.z_index;

                    // a=T or a=p makes it visible. a=t might be just upload.
                    if (kitty->cmd.action == 'T' || kitty->cmd.action == 'p') {
                        kitty->images[i].visible = true;
                        // On placement, re-anchor to current screen head
                        kitty->images[i].start_row = session->screen_head;
                    }
                    break;
                }
            }
        }
    }

    if (kitty->cmd.action == 't' || kitty->cmd.action == 'T') {
        if (session->options.debug_sequences) {
            char msg[128];
            snprintf(msg, sizeof(msg), "Kitty Image Transmitted: ID=%u", kitty->cmd.id);
            KTerm_LogUnsupportedSequence(term, msg);
        }
    } else if (kitty->cmd.action == 'd') {
        if (kitty->cmd.delete_action == 'a') {
             if (kitty->images) {
                for (int i = 0; i < kitty->image_count; i++) {
                    if (kitty->images[i].frames) {
                        for (int f = 0; f < kitty->images[i].frame_count; f++) {
                            if (kitty->images[i].frames[f].data) free(kitty->images[i].frames[f].data);
                            if (kitty->images[i].frames[f].texture.id != 0) KTerm_DestroyTexture(&kitty->images[i].frames[f].texture);
                        }
                        free(kitty->images[i].frames);
                    }
                }
                free(kitty->images);
                kitty->images = NULL;
                kitty->image_count = 0;
                kitty->image_capacity = 0;
                kitty->active_upload = NULL;
                kitty->current_memory_usage = 0; // Reset usage
            }
            if (session->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Kitty: Deleted All Images");
        } else if (kitty->cmd.delete_action == 'i') {
            for (int i = 0; i < kitty->image_count; i++) {
                if (kitty->images[i].id == kitty->cmd.id) {
                    if (kitty->images[i].frames) {
                        for (int f = 0; f < kitty->images[i].frame_count; f++) {
                            if (kitty->images[i].frames[f].data) {
                                if (kitty->current_memory_usage >= kitty->images[i].frames[f].capacity) {
                                    kitty->current_memory_usage -= kitty->images[i].frames[f].capacity;
                                } else {
                                    kitty->current_memory_usage = 0; // Should not happen
                                }
                                free(kitty->images[i].frames[f].data);
                            }
                            if (kitty->images[i].frames[f].texture.id != 0) KTerm_DestroyTexture(&kitty->images[i].frames[f].texture);
                        }
                        free(kitty->images[i].frames);
                    }
                    memmove(&kitty->images[i], &kitty->images[i+1], (kitty->image_count - i - 1) * sizeof(KittyImageBuffer));
                    kitty->image_count--;
                    break;
                }
            }
             if (session->options.debug_sequences) {
                char msg[64];
                snprintf(msg, sizeof(msg), "Kitty: Deleted Image ID=%u", kitty->cmd.id);
                KTerm_LogUnsupportedSequence(term, msg);
            }
        }
    } else if (kitty->cmd.action == 'q') {
        if (session->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Kitty: Query received");
    }
}

void KTerm_ProcessVT52Char(KTerm* term, KTermSession* session, unsigned char ch) {
    static bool expect_param = false;
    static char vt52_command = 0;

    if (!expect_param) {
        switch (ch) {
            case 'A': // Cursor up
                if (session->cursor.y > 0) session->cursor.y--;
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case 'B': // Cursor down
                if (session->cursor.y < term->height - 1) session->cursor.y++;
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case 'C': // Cursor right
                if (session->cursor.x < term->width - 1) session->cursor.x++;
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case 'D': // Cursor left
                if (session->cursor.x > 0) session->cursor.x--;
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case 'H': // Home cursor
                session->cursor.x = 0;
                session->cursor.y = 0;
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case 'I': // Reverse line feed
                session->cursor.y--;
                if (session->cursor.y < 0) {
                    session->cursor.y = 0;
                    KTerm_ScrollDownRegion_Internal(term, session, 0, term->height - 1, 1);
                }
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case 'J': // Clear to end of screen
                // Clear from cursor to end of line
                for (int x = session->cursor.x; x < term->width; x++) {
                    KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, session->cursor.y, x));
                }
                // Clear remaining lines
                for (int y = session->cursor.y + 1; y < term->height; y++) {
                    for (int x = 0; x < term->width; x++) {
                        KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, y, x));
                    }
                }
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case 'K': // Clear to end of line
                for (int x = session->cursor.x; x < term->width; x++) {
                    KTerm_ClearCell_Internal(session, GetActiveScreenCell(session, session->cursor.y, x));
                }
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case 'Y': // Direct cursor address
                vt52_command = 'Y';
                expect_param = true;
                session->escape_pos = 0;
                break;

            case 'Z': // Identify
                KTerm_QueueResponse(term, "\x1B/Z");
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case '=': // Enter alternate keypad mode
                session->input.keypad_application_mode = true;
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case '>': // Exit alternate keypad mode
                session->input.keypad_application_mode = false;
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case '<': // Enter ANSI mode
                session->parse_state = VT_PARSE_NORMAL;
                session->dec_modes.vt52_mode = false;
                break;

            case 'F': // Enter graphics mode
                session->charset.gl = &session->charset.g1; // Use G1 (DEC special)
                session->parse_state = VT_PARSE_NORMAL;
                break;

            case 'G': // Exit graphics mode
                session->charset.gl = &session->charset.g0; // Use G0 (ASCII)
                session->parse_state = VT_PARSE_NORMAL;
                break;

            default:
                // Unknown VT52 command
                session->parse_state = VT_PARSE_NORMAL;
                if (session->options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown VT52 command: %c", ch);
                    KTerm_LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    } else {
        // Handle parameterized commands
        if (vt52_command == 'Y') {
            if (session->escape_pos == 0) {
                // First parameter: row
                session->escape_buffer[0] = ch;
                session->escape_pos = 1;
            } else {
                // Second parameter: column
                int row = session->escape_buffer[0] - 32; // VT52 uses offset of 32
                int col = ch - 32;

                // Clamp to valid range
                session->cursor.y = (row < 0) ? 0 : (row >= term->height) ? term->height - 1 : row;
                session->cursor.x = (col < 0) ? 0 : (col >= term->width) ? term->width - 1 : col;

                expect_param = false;
                session->parse_state = VT_PARSE_NORMAL;
            }
        }
    }
}
// =============================================================================
// SIXEL GRAPHICS SUPPORT (Basic Implementation)
// =============================================================================

static float KTerm_HueToRGB(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f/2.0f) return q;
    if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
    return p;
}

static void KTerm_HLS2RGB(int h, int l, int s, unsigned char* r, unsigned char* g, unsigned char* b) {
    // H: 0-360, L: 0-100, S: 0-100
    float H = (float)h;
    float L = (float)l / 100.0f;
    float S = (float)s / 100.0f;

    if (S == 0.0f) {
        *r = *g = *b = (unsigned char)(L * 255.0f);
    } else {
        float q = (L < 0.5f) ? (L * (1.0f + S)) : (L + S - L * S);
        float p = 2.0f * L - q;

        float fr = KTerm_HueToRGB(p, q, H / 360.0f + 1.0f/3.0f);
        float fg = KTerm_HueToRGB(p, q, H / 360.0f);
        float fb = KTerm_HueToRGB(p, q, H / 360.0f - 1.0f/3.0f);

        *r = (unsigned char)(fr * 255.0f);
        *g = (unsigned char)(fg * 255.0f);
        *b = (unsigned char)(fb * 255.0f);
    }
}

void KTerm_ProcessSixelChar(KTerm* term, KTermSession* session, unsigned char ch) {
    (void)term;
    // 1. Check for digits across all states that consume them
    if (isdigit(ch)) {
        if (session->sixel.parse_state == SIXEL_STATE_REPEAT) {
            session->sixel.repeat_count = session->sixel.repeat_count * 10 + (ch - '0');
            return;
        } else if (session->sixel.parse_state == SIXEL_STATE_COLOR ||
                   session->sixel.parse_state == SIXEL_STATE_RASTER) {
            int idx = session->sixel.param_buffer_idx;
            if (idx < 8) {
                session->sixel.param_buffer[idx] = session->sixel.param_buffer[idx] * 10 + (ch - '0');
            }
            return;
        }
    }

    // 2. Handle Separator ';'
    if (ch == ';') {
        if (session->sixel.parse_state == SIXEL_STATE_COLOR ||
            session->sixel.parse_state == SIXEL_STATE_RASTER) {
            if (session->sixel.param_buffer_idx < 7) {
                session->sixel.param_buffer_idx++;
                session->sixel.param_buffer[session->sixel.param_buffer_idx] = 0;
            }
            return;
        }
    }

    // 3. Command Processing
    // If we are in a parameter state but receive a command char, finalize the previous command implicitly.
    if (session->sixel.parse_state == SIXEL_STATE_COLOR) {
        if (ch != '#' && !isdigit(ch) && ch != ';') {
            // Finalize KTermColor Command
            // # Pc ; Pu ; Px ; Py ; Pz (Define) OR # Pc (Select)
            if (session->sixel.param_buffer_idx >= 4) {
                // KTermColor Definition
                // Param 0: Index (Pc)
                // Param 1: Type (Pu) - 1=HLS, 2=RGB
                // Param 2,3,4: Components
                int idx = session->sixel.param_buffer[0];
                int type = session->sixel.param_buffer[1];
                int c1 = session->sixel.param_buffer[2];
                int c2 = session->sixel.param_buffer[3];
                int c3 = session->sixel.param_buffer[4];

                if (idx >= 0 && idx < 256) {
                    if (type == 2) { // RGB (0-100)
                        session->sixel.palette[idx] = (RGB_KTermColor){
                            (unsigned char)((c1 * 255) / 100),
                            (unsigned char)((c2 * 255) / 100),
                            (unsigned char)((c3 * 255) / 100),
                            255
                        };
                    } else if (type == 1) { // HLS (0-360, 0-100, 0-100)
                        unsigned char r, g, b;
                        KTerm_HLS2RGB(c1, c2, c3, &r, &g, &b);
                        session->sixel.palette[idx] = (RGB_KTermColor){ r, g, b, 255 };
                    }
                    session->sixel.color_index = idx; // Auto-select? Usually yes.
                }
            } else {
                // KTermColor Selection # Pc
                int idx = session->sixel.param_buffer[0];
                if (idx >= 0 && idx < 256) {
                    session->sixel.color_index = idx;
                }
            }
            session->sixel.parse_state = SIXEL_STATE_NORMAL;
        }
    } else if (session->sixel.parse_state == SIXEL_STATE_RASTER) {
        // Finalize Raster Attributes " Pan ; Pad ; Ph ; Pv
        // Just reset state for now, we don't resize based on this yet.
        session->sixel.parse_state = SIXEL_STATE_NORMAL;
    }

    switch (ch) {
        case '"': // Raster attributes
            session->sixel.parse_state = SIXEL_STATE_RASTER;
            session->sixel.param_buffer_idx = 0;
            memset(session->sixel.param_buffer, 0, sizeof(session->sixel.param_buffer));
            break;
        case '#': // KTermColor introducer
            session->sixel.parse_state = SIXEL_STATE_COLOR;
            session->sixel.param_buffer_idx = 0;
            memset(session->sixel.param_buffer, 0, sizeof(session->sixel.param_buffer));
            break;
        case '!': // Repeat introducer
            session->sixel.parse_state = SIXEL_STATE_REPEAT;
            session->sixel.repeat_count = 0;
            break;
        case '$': // Carriage return
            session->sixel.pos_x = 0;
            session->sixel.parse_state = SIXEL_STATE_NORMAL;
            break;
        case '-': // New line
            session->sixel.pos_x = 0;
            session->sixel.pos_y += 6;
            session->sixel.parse_state = SIXEL_STATE_NORMAL;
            break;
        case '\x1B': // Escape character - signals the start of the String Terminator (ST)
             session->parse_state = PARSE_SIXEL_ST;
             return;
        default:
            if (ch >= '?' && ch <= '~') {
                int sixel_val = ch - '?';
                int repeat = 1;

                if (session->sixel.parse_state == SIXEL_STATE_REPEAT) {
                    if (session->sixel.repeat_count > 0) repeat = session->sixel.repeat_count;
                    session->sixel.parse_state = SIXEL_STATE_NORMAL;
                    session->sixel.repeat_count = 0;
                }

                for (int r = 0; r < repeat; r++) {
                    // Buffer Growth Logic
                    if (session->sixel.strip_count >= session->sixel.strip_capacity) {
                        size_t new_cap = session->sixel.strip_capacity * 2;
                        if (new_cap == 0) new_cap = 65536; // Fallback
                        GPUSixelStrip* new_strips = realloc(session->sixel.strips, new_cap * sizeof(GPUSixelStrip));
                        if (new_strips) {
                            session->sixel.strips = new_strips;
                            session->sixel.strip_capacity = new_cap;
                        } else {
                            break; // OOM, stop drawing
                        }
                    }

                    if (session->sixel.strip_count < session->sixel.strip_capacity) {
                        GPUSixelStrip* strip = &session->sixel.strips[session->sixel.strip_count++];
                        strip->x = session->sixel.pos_x + r;
                        strip->y = session->sixel.pos_y; // Top of the 6-pixel column
                        strip->pattern = sixel_val; // 6 bits
                        strip->color_index = session->sixel.color_index;
                    }
                }
                session->sixel.pos_x += repeat;
                if (session->sixel.pos_x > session->sixel.max_x) {
                    session->sixel.max_x = session->sixel.pos_x;
                }
                if (session->sixel.pos_y + 6 > session->sixel.max_y) {
                    session->sixel.max_y = session->sixel.pos_y + 6;
                }
            }
            break;
    }
}

void KTerm_InitSixelGraphics(KTerm* term) {
    GET_SESSION(term)->sixel.active = false;
    if (GET_SESSION(term)->sixel.data) {
        free(GET_SESSION(term)->sixel.data);
    }
    GET_SESSION(term)->sixel.data = NULL;
    GET_SESSION(term)->sixel.width = 0;
    GET_SESSION(term)->sixel.height = 0;
    GET_SESSION(term)->sixel.x = 0;
    GET_SESSION(term)->sixel.y = 0;

    if (GET_SESSION(term)->sixel.strips) {
        free(GET_SESSION(term)->sixel.strips);
    }
    GET_SESSION(term)->sixel.strips = NULL;
    GET_SESSION(term)->sixel.strip_count = 0;
    GET_SESSION(term)->sixel.strip_capacity = 0;

    // Initialize standard palette (using global terminal palette as default)
    for (int i = 0; i < 256; i++) {
        GET_SESSION(term)->sixel.palette[i] = term->color_palette[i];
    }
    GET_SESSION(term)->sixel.parse_state = SIXEL_STATE_NORMAL;
    GET_SESSION(term)->sixel.param_buffer_idx = 0;
    memset(GET_SESSION(term)->sixel.param_buffer, 0, sizeof(GET_SESSION(term)->sixel.param_buffer));

    // Initialize scrolling state from DEC Mode 80
    // DECSDM (80): true=No Scroll, false=Scroll
    GET_SESSION(term)->sixel.scrolling = !GET_SESSION(term)->dec_modes.sixel_scrolling_mode;
}

void KTerm_ProcessSixelData(KTerm* term, KTermSession* session, const char* data, size_t length) {
    // Basic sixel processing - this is a complex format
    // This implementation provides framework for sixel support

    if (!session->conformance.features.sixel_graphics) {
        KTerm_LogUnsupportedSequence(term, "Sixel graphics require support enabled");
        return;
    }

    // Allocate sixel staging buffer
    if (!session->sixel.strips) {
        session->sixel.strip_capacity = 65536;
        session->sixel.strips = (GPUSixelStrip*)calloc(session->sixel.strip_capacity, sizeof(GPUSixelStrip));
    }
    session->sixel.strip_count = 0; // Reset for new image? Or append? Standard DCS q usually starts new.

    session->sixel.active = true;
    session->sixel.x = session->cursor.x * term->char_width;
    session->sixel.y = session->cursor.y * term->char_height;

    // Initialize internal sixel state for parsing
    session->sixel.pos_x = 0;
    session->sixel.pos_y = 0;
    session->sixel.max_x = 0;
    session->sixel.max_y = 0;
    session->sixel.color_index = 0;
    session->sixel.repeat_count = 1;

    // Process the sixel data stream
    for (size_t i = 0; i < length; i++) {
        KTerm_ProcessSixelChar(term, session, data[i]);
    }

    session->sixel.dirty = true; // Mark for upload
}

void KTerm_DrawSixelGraphics(KTerm* term) {
    if (!GET_SESSION(term)->conformance.features.sixel_graphics || !GET_SESSION(term)->sixel.active) return;
    // Just mark dirty, real work happens in KTerm_Draw
    GET_SESSION(term)->sixel.dirty = true;
}

// =============================================================================
// RECTANGULAR OPERATIONS (VT420)
// =============================================================================

void KTerm_ExecuteRectangularOps(KTerm* term) {
    // CSI Pt ; Pl ; Pb ; Pr $ v - Copy rectangular area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        KTerm_LogUnsupportedSequence(term, "Rectangular operations require support enabled");
        return;
    }

    // CSI Pts ; Pls ; Pbs ; Prs ; Pps ; Ptd ; Pld ; Ppd $ v
    int top = KTerm_GetCSIParam(term, 0, 1) - 1;
    int left = KTerm_GetCSIParam(term, 1, 1) - 1;
    int bottom = KTerm_GetCSIParam(term, 2, term->height) - 1;
    int right = KTerm_GetCSIParam(term, 3, term->width) - 1;
    // Pps (source page) ignored
    int dest_top = KTerm_GetCSIParam(term, 5, 1) - 1;
    int dest_left = KTerm_GetCSIParam(term, 6, 1) - 1;
    // Ppd (dest page) ignored

    // Validate rectangle
    if (top >= 0 && left >= 0 && bottom >= top && right >= left &&
        bottom < term->height && right < term->width) {

        VTRectangle rect = {top, left, bottom, right, true};
        KTerm_CopyRectangle(term, rect, dest_left, dest_top);
    }
}

void KTerm_ExecuteRectangularOps2(KTerm* term) {
    // CSI Pt ; Pl ; Pb ; Pr $ w - Request checksum of rectangular area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        KTerm_LogUnsupportedSequence(term, "Rectangular operations require support enabled");
        return;
    }

    // Calculate checksum and respond
    // CSI Pid ; Pp ; Pt ; Pl ; Pb ; Pr $ w
    int pid = KTerm_GetCSIParam(term, 0, 1);
    // int page = KTerm_GetCSIParam(term, 1, 1); // Ignored
    int top = KTerm_GetCSIParam(term, 2, 1) - 1;
    int left = KTerm_GetCSIParam(term, 3, 1) - 1;
    int bottom = KTerm_GetCSIParam(term, 4, term->height) - 1;
    int right = KTerm_GetCSIParam(term, 5, term->width) - 1;

    // Validate
    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= term->height) bottom = term->height - 1;
    if (right >= term->width) right = term->width - 1;

    unsigned int checksum = 0;
    if (top <= bottom && left <= right) {
        checksum = KTerm_CalculateRectChecksum(term, top, left, bottom, right);
    }

    char response[32];
    snprintf(response, sizeof(response), "\x1BP%d!~%04X\x1B\\", pid, checksum & 0xFFFF);
    KTerm_QueueResponse(term, response);
}

void KTerm_CopyRectangle(KTerm* term, VTRectangle src, int dest_x, int dest_y) {
    int width = src.right - src.left + 1;
    int height = src.bottom - src.top + 1;

    // Allocate temporary buffer for copy
    EnhancedTermChar* temp = malloc(width * height * sizeof(EnhancedTermChar));
    if (!temp) return;

    // Copy source to temp buffer
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (src.top + y < term->height && src.left + x < term->width) {
                temp[y * width + x] = *GetActiveScreenCell(GET_SESSION(term), src.top + y, src.left + x);
            }
        }
    }

    // Copy from temp buffer to destination
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dst_y = dest_y + y;
            int dst_x = dest_x + x;

            if (dst_y >= 0 && dst_y < term->height && dst_x >= 0 && dst_x < term->width) {
                *GetActiveScreenCell(GET_SESSION(term), dst_y, dst_x) = temp[y * width + x];
                GetActiveScreenCell(GET_SESSION(term), dst_y, dst_x)->flags |= KTERM_FLAG_DIRTY;
            }
        }
        if (dest_y + y >= 0 && dest_y + y < term->height) {
            GET_SESSION(term)->row_dirty[dest_y + y] = true;
        }
    }

    free(temp);
}

// =============================================================================
// COMPLETE TERMINAL TESTING FRAMEWORK
// =============================================================================

// Test helper functions
void KTerm_TestCursorMovement(KTerm* term) {
    KTerm_WriteString(term, "\x1B[2J\x1B[H"); // Clear screen, home cursor
    KTerm_WriteString(term, "VT Cursor Movement Test\n");
    KTerm_WriteString(term, "Testing basic cursor operations...\n\n");

    // Test cursor positioning
    KTerm_WriteString(term, "\x1B[5;10HPosition test");
    KTerm_WriteString(term, "\x1B[10;1H");

    // Test cursor movement
    KTerm_WriteString(term, "Moving: ");
    KTerm_WriteString(term, "\x1B[5CRIGHT ");
    KTerm_WriteString(term, "\x1B[3DBACK ");
    KTerm_WriteString(term, "\x1B[2AUP ");
    KTerm_WriteString(term, "\x1B[1BDOWN\n");

    // Test save/restore
    KTerm_WriteString(term, "\x1B[s"); // Save cursor
    KTerm_WriteString(term, "\x1B[15;20HTemp position");
    KTerm_WriteString(term, "\x1B[u"); // Restore cursor
    KTerm_WriteString(term, "Back to saved position\n");

    KTerm_WriteString(term, "\nCursor test complete.\n");
}

void KTerm_TestKTermColors(KTerm* term) {
    KTerm_WriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    KTerm_WriteString(term, "VT Color Test\n\n");

    // Test basic 16 colors
    KTerm_WriteString(term, "Basic 16 colors:\n");
    for (int i = 0; i < 8; i++) {
        KTerm_WriteFormat(term, "\x1B[%dm Color %d \x1B[0m", 30 + i, i);
        KTerm_WriteFormat(term, "\x1B[%dm Bright %d \x1B[0m\n", 90 + i, i + 8);
    }

    // Test 256 colors (sample)
    KTerm_WriteString(term, "\n256-color sample:\n");
    for (int i = 16; i < 32; i++) {
        KTerm_WriteFormat(term, "\x1B[38;5;%dm███\x1B[0m", i);
    }
    KTerm_WriteString(term, "\n");

    // Test true color
    KTerm_WriteString(term, "\nTrue color gradient:\n");
    for (int i = 0; i < 24; i++) {
        int r = (i * 255) / 23;
        KTerm_WriteFormat(term, "\x1B[38;2;%d;0;0m█\x1B[0m", r);
    }
    KTerm_WriteString(term, "\n\nColor test complete.\n");
}

void KTerm_TestCharacterSets(KTerm* term) {
    KTerm_WriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    KTerm_WriteString(term, "VT Character Set Test\n\n");

    // Test DEC Special Graphics
    KTerm_WriteString(term, "DEC Special Graphics:\n");
    KTerm_WriteString(term, "\x1B(0"); // Select DEC special
    KTerm_WriteString(term, "lqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk\n");
    KTerm_WriteString(term, "x                             x\n");
    KTerm_WriteString(term, "x    DEC Line Drawing Test    x\n");
    KTerm_WriteString(term, "x                             x\n");
    KTerm_WriteString(term, "mqqqqqqqqqqwqqqqqqqqqqqqqqqqqj\n");
    KTerm_WriteString(term, "             x\n");
    KTerm_WriteString(term, "             x\n");
    KTerm_WriteString(term, "             v\n");
    KTerm_WriteString(term, "\x1B(B"); // Back to ASCII

    KTerm_WriteString(term, "\nASCII mode restored.\n");
    KTerm_WriteString(term, "Character set test complete.\n");
}

void KTerm_TestMouseTracking(KTerm* term) {
    KTerm_WriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    KTerm_WriteString(term, "VT Mouse Tracking Test\n\n");

    KTerm_WriteString(term, "Enabling mouse tracking...\n");
    KTerm_WriteString(term, "\x1B[?1000h"); // Enable mouse tracking

    KTerm_WriteString(term, "Click anywhere to test mouse reporting.\n");
    KTerm_WriteString(term, "Mouse coordinates will be reported.\n");
    KTerm_WriteString(term, "Press ESC to disable mouse tracking.\n\n");

    // Mouse tracking will be handled by the input system
    // Results will appear as the user interacts
}

void KTerm_TestModes(KTerm* term) {
    KTerm_WriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    KTerm_WriteString(term, "VT KTerm Modes Test\n\n");

    // Test insert mode
    KTerm_WriteString(term, "Testing insert mode:\n");
    KTerm_WriteString(term, "Original: ABCDEF\n");
    KTerm_WriteString(term, "ABCDEF\x1B[4D\x1B[4h***\x1B[4l");
    KTerm_WriteString(term, "\nAfter insert: AB***CDEF\n\n");

    // Test alternate screen
    KTerm_WriteString(term, "Testing alternate screen buffer...\n");
    KTerm_WriteString(term, "Switching to alternate screen in 2 seconds...\n");
    // Would need timing mechanism for full demo

    KTerm_WriteString(term, "\nMode test complete.\n");
}

void KTerm_RunAllTests(KTerm* term) {
    KTerm_WriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    KTerm_WriteString(term, "Running Complete VT Test Suite\n");
    KTerm_WriteString(term, "==============================\n\n");

    KTerm_TestCursorMovement(term);
    KTerm_WriteString(term, "\nPress any key to continue...\n");
    // Would wait for input in full implementation

    KTerm_TestKTermColors(term);
    KTerm_WriteString(term, "\nPress any key to continue...\n");

    KTerm_TestCharacterSets(term);
    KTerm_WriteString(term, "\nPress any key to continue...\n");

    KTerm_TestModes(term);

    KTerm_WriteString(term, "\n\nAll tests completed!\n");
    KTerm_ShowInfo(term);
}

void KTerm_RunTest(KTerm* term, const char* test_name) {
    if (strcmp(test_name, "cursor") == 0) {
        KTerm_TestCursorMovement(term);
    } else if (strcmp(test_name, "colors") == 0) {
        KTerm_TestKTermColors(term);
    } else if (strcmp(test_name, "charset") == 0) {
        KTerm_TestCharacterSets(term);
    } else if (strcmp(test_name, "mouse") == 0) {
        KTerm_TestMouseTracking(term);
    } else if (strcmp(test_name, "modes") == 0) {
        KTerm_TestModes(term);
    } else if (strcmp(test_name, "all") == 0) {
        KTerm_RunAllTests(term);
    } else {
        KTerm_WriteFormat(term, "Unknown test: %s\n", test_name);
        KTerm_WriteString(term, "Available tests: cursor, colors, charset, mouse, modes, all\n");
    }
}

void KTerm_ShowInfo(KTerm* term) {
    KTerm_WriteString(term, "\n");
    KTerm_WriteString(term, "KTerm Information\n");
    KTerm_WriteString(term, "===================\n");
    KTerm_WriteFormat(term, "KTerm Type: %s\n", GET_SESSION(term)->title.terminal_name);
    KTerm_WriteFormat(term, "VT Level: %d\n", GET_SESSION(term)->conformance.level);
    KTerm_WriteFormat(term, "Primary DA: %s\n", GET_SESSION(term)->device_attributes);
    KTerm_WriteFormat(term, "Secondary DA: %s\n", GET_SESSION(term)->secondary_attributes);

    KTerm_WriteString(term, "\nSupported Features:\n");
    KTerm_WriteFormat(term, "- VT52 Mode: %s\n", GET_SESSION(term)->conformance.features.vt52_mode ? "Yes" : "No");
    KTerm_WriteFormat(term, "- VT100 Mode: %s\n", GET_SESSION(term)->conformance.features.vt100_mode ? "Yes" : "No");
    KTerm_WriteFormat(term, "- VT220 Mode: %s\n", GET_SESSION(term)->conformance.features.vt220_mode ? "Yes" : "No");
    KTerm_WriteFormat(term, "- VT320 Mode: %s\n", GET_SESSION(term)->conformance.features.vt320_mode ? "Yes" : "No");
    KTerm_WriteFormat(term, "- VT420 Mode: %s\n", GET_SESSION(term)->conformance.features.vt420_mode ? "Yes" : "No");
    KTerm_WriteFormat(term, "- VT520 Mode: %s\n", GET_SESSION(term)->conformance.features.vt520_mode ? "Yes" : "No");
    KTerm_WriteFormat(term, "- xterm Mode: %s\n", GET_SESSION(term)->conformance.features.xterm_mode ? "Yes" : "No");
    KTerm_WriteFormat(term, "- Sixel Graphics: %s\n", GET_SESSION(term)->conformance.features.sixel_graphics ? "Yes" : "No");
    KTerm_WriteFormat(term, "- ReGIS Graphics: %s\n", GET_SESSION(term)->conformance.features.regis_graphics ? "Yes" : "No");
    KTerm_WriteFormat(term, "- Rectangular Ops: %s\n", GET_SESSION(term)->conformance.features.rectangular_operations ? "Yes" : "No");
    KTerm_WriteFormat(term, "- Soft Fonts: %s\n", GET_SESSION(term)->conformance.features.soft_fonts ? "Yes" : "No");
    KTerm_WriteFormat(term, "- NRCS: %s\n", GET_SESSION(term)->conformance.features.national_charsets ? "Yes" : "No");
    KTerm_WriteFormat(term, "- User Defined Keys: %s\n", GET_SESSION(term)->conformance.features.user_defined_keys ? "Yes" : "No");
    KTerm_WriteFormat(term, "- Mouse Tracking: %s\n", GET_SESSION(term)->conformance.features.mouse_tracking ? "Yes" : "No");
    KTerm_WriteFormat(term, "- True Color: %s\n", GET_SESSION(term)->conformance.features.true_color ? "Yes" : "No");
    KTerm_WriteFormat(term, "- Locator: %s\n", GET_SESSION(term)->conformance.features.locator ? "Yes" : "No");
    KTerm_WriteFormat(term, "- Multi-Session: %s\n", GET_SESSION(term)->conformance.features.multi_session_mode ? "Yes" : "No");
    KTerm_WriteFormat(term, "- Selective Erase: %s\n", GET_SESSION(term)->conformance.features.selective_erase ? "Yes" : "No");
    KTerm_WriteFormat(term, "- Left/Right Margin: %s\n", GET_SESSION(term)->conformance.features.left_right_margin ? "Yes" : "No");

    KTerm_WriteString(term, "\nCurrent Settings:\n");
    KTerm_WriteFormat(term, "- Cursor Keys: %s\n", GET_SESSION(term)->dec_modes.application_cursor_keys ? "Application" : "Normal");
    KTerm_WriteFormat(term, "- Keypad: %s\n", GET_SESSION(term)->input.keypad_application_mode ? "Application" : "Numeric");
    KTerm_WriteFormat(term, "- Auto Wrap: %s\n", GET_SESSION(term)->dec_modes.auto_wrap_mode ? "On" : "Off");
    KTerm_WriteFormat(term, "- Origin Mode: %s\n", GET_SESSION(term)->dec_modes.origin_mode ? "On" : "Off");
    KTerm_WriteFormat(term, "- Insert Mode: %s\n", GET_SESSION(term)->dec_modes.insert_mode ? "On" : "Off");

    KTerm_WriteFormat(term, "\nScrolling Region: %d-%d\n",
                       GET_SESSION(term)->scroll_top + 1, GET_SESSION(term)->scroll_bottom + 1);
    KTerm_WriteFormat(term, "Margins: %d-%d\n",
                       GET_SESSION(term)->left_margin + 1, GET_SESSION(term)->right_margin + 1);

    KTerm_WriteString(term, "\nStatistics:\n");
    KTermStatus status = KTerm_GetStatus(term);
    KTerm_WriteFormat(term, "- Pipeline Usage: %zu/%d\n", status.pipeline_usage, (int)sizeof(GET_SESSION(term)->input_pipeline));
    KTerm_WriteFormat(term, "- Key Buffer: %zu\n", status.key_usage);
    KTerm_WriteFormat(term, "- Unsupported Sequences: %d\n", GET_SESSION(term)->conformance.compliance.unsupported_sequences);

    if (GET_SESSION(term)->conformance.compliance.last_unsupported[0]) {
        KTerm_WriteFormat(term, "- Last Unsupported: %s\n", GET_SESSION(term)->conformance.compliance.last_unsupported);
    }
}

// =============================================================================
// FINAL API IMPLEMENTATIONS (Scripting, Core Loop, Lifecycle, etc.)
// This section includes high-level scripting utilities, functions for managing
// the terminal's core update and drawing loop, VT level configuration,
// processed keyboard event retrieval, debugging, and lifecycle management.
// These APIs are key to integrating the terminal into a host application
// and controlling its primary behaviors and features.
// =============================================================================

// --- Scripting API Functions ---
// These functions provide convenient wrappers for the hosting application to
// directly output text and simple commands to the terminal screen. They are
// primarily for local display purposes by the application embedding the terminal,
// simplifying tasks like showing status messages or basic UI elements.

/**
 * @brief Outputs a single character to the terminal's input pipeline.
 * Part of the scripting API for easy terminal manipulation by the hosting application.
 * This is a convenience wrapper around KTerm_WriteChar(term).
 * @param ch The character to output.
 */
void KTerm_Script_PutChar(KTerm* term, unsigned char ch) {
    KTerm_WriteChar(term, ch);
}

/**
 * @brief Prints a null-terminated string to the terminal's input pipeline.
 * Part of the scripting API. Convenience wrapper around KTerm_WriteString(term).
 * Useful for displaying messages from the hosting application on the term->
 * @param text The string to print.
 */
void KTerm_Script_Print(KTerm* term, const char* text) {
    KTerm_WriteString(term, text);
}

/**
 * @brief Prints a formatted string to the terminal's input pipeline.
 * Part of the scripting API. Convenience wrapper around KTerm_WriteFormat(term).
 * Allows for dynamic string construction for display by the hosting application.
 * @param format The printf-style format string.
 * @param ... Additional arguments for the format string.
 */
void KTerm_Script_Printf(KTerm* term, const char* format, ...) {
    char buffer[1024]; // Note: For very long formatted strings, consider dynamic allocation or a larger buffer.
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    KTerm_WriteString(term, buffer);
}

/**
 * @brief Clears the terminal screen and homes the cursor.
 * Part of the scripting API. This sends the standard escape sequences:
 * "ESC[2J" (Erase Display: entire screen) and "ESC[H" (Cursor Home: top-left).
 */
void KTerm_Script_Cls(KTerm* term) {
    KTerm_WriteString(term, "\x1B[2J\x1B[H");
}

/**
 * @brief Sets basic ANSI foreground and background colors for subsequent text output via the Scripting API.
 * Part of the scripting API.
 * This sends SGR (Select Graphic Rendition) escape sequences.
 * @param fg Foreground color index (0-7 for standard, 8-15 for bright).
 * @param bg Background color index (0-7 for standard, 8-15 for bright).
 */
void KTerm_Script_SetKTermColor(KTerm* term, int fg, int bg) {
    // Ensure fg/bg are within basic ANSI range 0-7 for 30-37/40-47
    // or 8-15 for 90-97/100-107 (bright versions)
    char color_seq[32];
    if (fg >= 0 && fg <= 15 && bg >=0 && bg <= 15) {
        snprintf(color_seq, sizeof(color_seq), "\x1B[%d;%dm",
            (fg < 8 ? 30+fg : 90+(fg-8)), // Map 0-7 to 30-37, 8-15 to 90-97
            (bg < 8 ? 40+bg : 100+(bg-8)) ); // Map 0-7 to 40-47, 8-15 to 100-107
    } else { // Fallback or invalid input
        snprintf(color_seq, sizeof(color_seq), "\x1B[0m"); // Reset to default colors
    }
    KTerm_WriteString(term, color_seq);
}

// --- VT Compliance Level Management ---

/**
 * @brief Sets the terminal's VT compatibility level (e.g., VT100, VT220, XTERM).
 * This is a cornerstone for controlling the terminal's behavior. Changing the level:
 *  - Modifies which escape sequences the terminal recognizes and processes.
 *  - Alters the strings returned for Device Attribute (DA) requests (e.g., CSI c).
 *  - Enables or disables specific features associated with that level, such as:
 *    - Sixel graphics (typically VT240/VT3xx+ or XTERM).
 *    - Advanced mouse tracking modes (VT200+ or XTERM).
 *    - National Replacement Character Sets (NRCS), DEC Special Graphics.
 *    - Rectangular area operations (VT420+).
 *    - User-Defined Keys (DECUDK, VT320+).
 *    - Soft Fonts (DECDLD, VT220+).
 *  - Updates internal feature flags in `GET_SESSION(term)->conformance.features`.
 * The library aims for compatibility with VT52, VT100, VT220, VT320, VT420, and xterm standards.
 *
 * @param level The desired VTLevel (e.g., VT_LEVEL_100, VT_LEVEL_XTERM).
 * @see VTLevel enum for available levels.
 * @see GET_SESSION(term)->h header documentation for a full list of KEY FEATURES and their typical VT level requirements.
 */
// Statically define the feature sets for each VT level for easy lookup.
typedef struct {
    VTLevel level;
    VTFeatures features;
} VTLevelFeatureMapping;

static const VTLevelFeatureMapping vt_level_mappings[] = {
    { VT_LEVEL_52, { .vt52_mode = true, .max_session_count = 1 } },
    { VT_LEVEL_100, { .vt100_mode = true, .national_charsets = true, .max_session_count = 1 } },
    { VT_LEVEL_102, { .vt100_mode = true, .vt102_mode = true, .national_charsets = true, .max_session_count = 1 } },
    { VT_LEVEL_132, { .vt100_mode = true, .vt102_mode = true, .vt132_mode = true, .national_charsets = true, .max_session_count = 1 } },
    { VT_LEVEL_220, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .max_session_count = 1 } },
    { VT_LEVEL_320, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .max_session_count = 1 } },
    { VT_LEVEL_340, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true, .regis_graphics = true, .multi_session_mode = true, .locator = true, .max_session_count = 2 } },
    { VT_LEVEL_420, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .rectangular_operations = true, .selective_erase = true, .multi_session_mode = true, .locator = true, .left_right_margin = true, .max_session_count = 2 } },
    { VT_LEVEL_510, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt510_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .rectangular_operations = true, .selective_erase = true, .locator = true, .left_right_margin = true, .max_session_count = 2 } },
    { VT_LEVEL_520, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt510_mode = true, .vt520_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .rectangular_operations = true, .selective_erase = true, .locator = true, .multi_session_mode = true, .left_right_margin = true, .max_session_count = 4 } },
    { VT_LEVEL_525, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt510_mode = true, .vt520_mode = true, .vt525_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true, .regis_graphics = true, .rectangular_operations = true, .selective_erase = true, .locator = true, .true_color = true, .multi_session_mode = true, .left_right_margin = true, .max_session_count = 4 } },
    { VT_LEVEL_XTERM, {
        .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt520_mode = true, .xterm_mode = true,
        .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true, .regis_graphics = true,
        .rectangular_operations = true, .selective_erase = true, .locator = true, .true_color = true,
        .mouse_tracking = true, .alternate_screen = true, .window_manipulation = true, .left_right_margin = true, .max_session_count = 1
    }},
    { VT_LEVEL_K95, { .k95_mode = true, .max_session_count = 1 } },
    { VT_LEVEL_TT, { .tt_mode = true, .max_session_count = 1 } },
    { VT_LEVEL_PUTTY, { .putty_mode = true, .max_session_count = 1 } },
    { VT_LEVEL_ANSI_SYS, { .vt100_mode = true, .national_charsets = false, .max_session_count = 1 } },
};

void KTerm_SetLevel(KTerm* term, VTLevel level) {
    bool level_found = false;
    for (size_t i = 0; i < sizeof(vt_level_mappings) / sizeof(vt_level_mappings[0]); i++) {
        if (vt_level_mappings[i].level == level) {
            GET_SESSION(term)->conformance.features = vt_level_mappings[i].features;
            level_found = true;
            break;
        }
    }

    if (!level_found) {
        // Default to a safe baseline if unknown
        GET_SESSION(term)->conformance.features = vt_level_mappings[0].features;
    }

    GET_SESSION(term)->conformance.level = level;

    // Update Answerback string based on level
    if (level == VT_LEVEL_ANSI_SYS) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "ANSI.SYS");
        // Force IBM Font
        KTerm_SetFont(term, "IBM");
        // Enforce authentic CGA palette (using the standard definitions)
        for (int i = 0; i < 16; i++) {
            term->color_palette[i] = (RGB_KTermColor){ ansi_colors[i].r, ansi_colors[i].g, ansi_colors[i].b, 255 };
        }
    } else if (level == VT_LEVEL_XTERM) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm xterm");
    } else if (level >= VT_LEVEL_525) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm VT525");
    } else if (level >= VT_LEVEL_520) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm VT520");
    } else if (level >= VT_LEVEL_420) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm VT420");
    } else if (level >= VT_LEVEL_340) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm VT340");
    } else if (level >= VT_LEVEL_320) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm VT320");
    } else if (level >= VT_LEVEL_220) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm VT220");
    } else if (level >= VT_LEVEL_102) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm VT102");
    } else if (level >= VT_LEVEL_100) {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm VT100");
    } else {
        snprintf(GET_SESSION(term)->answerback_buffer, MAX_COMMAND_BUFFER, "kterm VT52");
    }

    // Update Device Attribute strings based on the level.
    if (level == VT_LEVEL_ANSI_SYS) {
        // ANSI.SYS does not typically respond to DA
        GET_SESSION(term)->device_attributes[0] = '\0';
        GET_SESSION(term)->secondary_attributes[0] = '\0';
        GET_SESSION(term)->tertiary_attributes[0] = '\0';
    } else if (level == VT_LEVEL_XTERM) {
        strcpy(GET_SESSION(term)->device_attributes, "\x1B[?41;1;2;6;7;8;9;15;18;21;22c");
        strcpy(GET_SESSION(term)->secondary_attributes, "\x1B[>41;400;0c");
        strcpy(GET_SESSION(term)->tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_525) {
        strcpy(GET_SESSION(term)->device_attributes, "\x1B[?65;1;2;6;7;8;9;15;18;21;22;28;29c");
        strcpy(GET_SESSION(term)->secondary_attributes, "\x1B[>52;10;0c");
        strcpy(GET_SESSION(term)->tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_520) {
        strcpy(GET_SESSION(term)->device_attributes, "\x1B[?65;1;2;6;7;8;9;15;18;21;22;28;29c");
        strcpy(GET_SESSION(term)->secondary_attributes, "\x1B[>52;10;0c");
        strcpy(GET_SESSION(term)->tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_420) {
        strcpy(GET_SESSION(term)->device_attributes, "\x1B[?64;1;2;6;7;8;9;15;18;21;22;28;29c");
        strcpy(GET_SESSION(term)->secondary_attributes, "\x1B[>41;10;0c");
        strcpy(GET_SESSION(term)->tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_340 || level >= VT_LEVEL_320) {
        strcpy(GET_SESSION(term)->device_attributes, "\x1B[?63;1;2;6;7;8;9;15;18;21c");
        strcpy(GET_SESSION(term)->secondary_attributes, "\x1B[>24;10;0c");
        strcpy(GET_SESSION(term)->tertiary_attributes, "");
    } else if (level >= VT_LEVEL_220) {
        strcpy(GET_SESSION(term)->device_attributes, "\x1B[?62;1;2;6;7;8;9;15c");
        strcpy(GET_SESSION(term)->secondary_attributes, "\x1B[>1;10;0c");
        strcpy(GET_SESSION(term)->tertiary_attributes, "");
    } else if (level >= VT_LEVEL_102) {
        strcpy(GET_SESSION(term)->device_attributes, "\x1B[?6c");
        strcpy(GET_SESSION(term)->secondary_attributes, "\x1B[>0;95;0c");
        strcpy(GET_SESSION(term)->tertiary_attributes, "");
    } else if (level >= VT_LEVEL_100) {
        strcpy(GET_SESSION(term)->device_attributes, "\x1B[?1;2c");
        strcpy(GET_SESSION(term)->secondary_attributes, "\x1B[>0;95;0c");
        strcpy(GET_SESSION(term)->tertiary_attributes, "");
    } else { // VT52
        strcpy(GET_SESSION(term)->device_attributes, "\x1B/Z");
        GET_SESSION(term)->secondary_attributes[0] = '\0';
        GET_SESSION(term)->tertiary_attributes[0] = '\0';
    }
}

VTLevel KTerm_GetLevel(KTerm* term) {
    return GET_SESSION(term)->conformance.level;
}

// --- Keyboard Input ---

/**
 * @brief Retrieves a fully processed keyboard event from the terminal's internal buffer.
 * The application hosting the terminal should call this function repeatedly (e.g., in its
 * main loop after `KTerm_UpdateKeyboard(term)`) to obtain keyboard input.
 *
 * The `VTKeyboard` system, updated by `KTerm_UpdateKeyboard(term)`, translates raw Platform key
 * presses into appropriate VT sequences or characters. This processing considers:
 *  - Modifier keys (Shift, Ctrl, Alt/Meta).
 *  - KTerm modes such as:
 *    - Application Cursor Keys (DECCKM): e.g., Up Arrow sends `ESC O A` instead of `ESC [ A`.
 *    - Application Keypad Mode (DECKPAM/DECKPNM): Numeric keypad keys send special sequences.
 *  - User-Defined Keys (DECUDK), if programmed.
 *
 * The `event->sequence` field of the returned `VTKeyEvent` struct contains the byte
 * sequence that should be transmitted to the connected PTY, host application, or
 * further processed locally.
 *
 * @param event Pointer to a `VTKeyEvent` structure that will be filled with the event data.
 * @return `true` if a key event was retrieved from the buffer, `false` if the buffer is empty.
 * @see KTerm_UpdateKeyboard(term) which captures Platform input and populates the event buffer.
 * @see VTKeyEvent struct for details on the event data fields.
 * @note The terminal platform provides robust keyboard translation, ensuring that applications
 *       running within the terminal receive the correct input sequences based on active modes.
 */

// --- Debugging ---

/**
 * @brief Enables or disables the terminal's debug mode for diagnostics.
 * When enabled, the terminal provides more verbose logging, which can be invaluable
 * for development and troubleshooting. This typically includes:
 *  - Logging of unrecognized or unsupported escape sequences.
 *  - Notifications about partially implemented features being invoked.
 *  - Potentially, details on internal state changes or parsing decisions.
 *
 * Debug output is usually directed via the `ResponseCallback`. If no callback is set,
 * the behavior might default to standard console output or be suppressed, depending
 * on the library's internal configuration.
 *
 * Enabling debug mode usually activates related flags like:
 *  - `GET_SESSION(term)->options.log_unsupported`: Specifically for unsupported sequences.
 *  - `GET_SESSION(term)->options.conformance_checking`: For stricter checks against VT standards.
 *  - `GET_SESSION(term)->status.debugging`: A general flag indicating debug activity.
 *
 * This capability allows developers to understand the terminal's interaction with
 * host applications and identify issues in escape sequence processing.
 *
 * @param enable `true` to enable debug mode, `false` to disable.
 */
void KTerm_EnableDebug(KTerm* term, bool enable) {
    GET_SESSION(term)->options.debug_sequences = enable;
    GET_SESSION(term)->options.log_unsupported = enable;
    GET_SESSION(term)->options.conformance_checking = enable;
    GET_SESSION(term)->status.debugging = enable; // General debugging flag for other parts of the library
}

// --- Core KTerm Loop Functions ---

/**
 * @brief Updates the terminal's internal state and processes incoming data.
 *
 * Called once per frame in the main loop, this function drives the terminal emulation by:
 * - **Processing Input**: Consumes characters from `GET_SESSION(term)->input_pipeline` via `KTerm_ProcessEvents(term)`, parsing VT52/xterm sequences with `KTerm_ProcessChar(term)`.
 * - **Handling Input Devices**: Updates keyboard (`KTerm_UpdateKeyboard(term)`) and mouse (`KTerm_UpdateMouse(term)`) states.
 * - **Auto-Printing**: Queues lines for printing when `GET_SESSION(term)->auto_print_enabled` and a newline occurs.
 * - **Managing Timers**: Updates cursor blink, text blink, and visual bell timers for visual effects.
 * - **Flushing Responses**: Sends queued responses (e.g., DSR, DA, focus events) via `term->response_callback`.
 * - **Rendering**: Draws the terminal display with `KTerm_Draw(term)`, including the custom mouse cursor.
 *
 * Performance is tuned via `GET_SESSION(term)->VTperformance` (e.g., `chars_per_frame`, `time_budget`) to balance responsiveness and throughput.
 *
 * @see KTerm_ProcessEvents(term) for input processing details.
 * @see KTerm_UpdateKeyboard(term) for keyboard handling.
 * @see KTerm_UpdateMouse(term) for mouse and focus event handling.
 * @see KTerm_Draw(term) for rendering details.
 * @see KTerm_QueueResponse(term) for response queuing.
 */
void KTerm_Update(KTerm* term) {
    term->pending_session_switch = -1; // Reset pending switch
    int saved_session = term->active_session;

    // Process all sessions
    for (int i = 0; i < MAX_SESSIONS; i++) {
        KTermSession* session = &term->sessions[i];

        // Process input from the pipeline
        KTerm_ProcessEventsInternal(term, session);

        // Update timers and bells for this session
        if (session->cursor.blink_enabled && session->dec_modes.cursor_visible) {
            session->cursor.blink_state = KTerm_TimerGetOscillator(30); // Slot 30 (~250ms)
        } else {
            session->cursor.blink_state = true;
        }
        // Update Blink States: Bit 0 = Fast, Bit 1 = Slow, Bit 2 = Background
        bool fast_blink = KTerm_TimerGetOscillator(session->fast_blink_rate);
        bool slow_blink = KTerm_TimerGetOscillator(session->slow_blink_rate);
        bool bg_blink = KTerm_TimerGetOscillator(session->bg_blink_rate);
        session->text_blink_state = (fast_blink ? 1 : 0) | (slow_blink ? 2 : 0) | (bg_blink ? 4 : 0);

        if (session->visual_bell_timer > 0) {
            session->visual_bell_timer -= KTerm_GetFrameTime();
            if (session->visual_bell_timer < 0) session->visual_bell_timer = 0;
        }

        // Animation Logic
        if (session->kitty.images) {
            float dt = KTerm_GetFrameTime();
            for (int k=0; k<session->kitty.image_count; k++) {
                KittyImageBuffer* img = &session->kitty.images[k];
                if (img->frame_count > 1 && img->visible && img->complete) {
                    img->frame_timer += (dt * 1000.0);
                    int delay = img->frames[img->current_frame].delay_ms;
                    if (delay <= 0) delay = 40; // Default min delay

                    while (img->frame_timer >= delay) {
                        img->frame_timer -= delay;
                        img->current_frame = (img->current_frame + 1) % img->frame_count;
                        delay = img->frames[img->current_frame].delay_ms;
                        if (delay <= 0) delay = 40;
                    }
                }
            }
        }

        // Flush responses
        if (session->response_length > 0 && term->response_callback) {
            term->response_callback(term, session->answerback_buffer, session->response_length);
            session->response_length = 0;
        }
    }

    // Restore active session for input handling, unless a switch occurred
    if (term->pending_session_switch != -1) {
        term->active_session = term->pending_session_switch;
    } else {
        term->active_session = saved_session;
    }

    // Process Input Buffer
    KTermSession* session = GET_SESSION(term);
    while (session->input.buffer_count > 0) {
        KTermEvent* event = &session->input.buffer[session->input.buffer_tail];

        // 1. Send Sequence to Host
        if (event->sequence[0] != '\0') {
            KTerm_QueueResponse(term, event->sequence);

            // 2. Local Echo
            if (session->dec_modes.local_echo) {
                KTerm_WriteString(term, event->sequence);
            }

            // 3. Visual Bell Trigger
            if (event->sequence[0] == 0x07) {
                session->visual_bell_timer = 0.2f;
            }
        }

        session->input.buffer_tail = (session->input.buffer_tail + 1) % KEY_EVENT_BUFFER_SIZE;
        session->input.buffer_count--;
    }

    // Auto-print (Active session only for now, or loop?)
    // Let's assume auto-print works for active session interaction.
    if (GET_SESSION(term)->printer_available && GET_SESSION(term)->auto_print_enabled) {
        if (GET_SESSION(term)->cursor.y > GET_SESSION(term)->last_cursor_y && GET_SESSION(term)->last_cursor_y >= 0) {
            // Queue the previous line for printing
            char print_buffer[term->width + 2];
            size_t pos = 0;
            for (int x = 0; x < term->width && pos < term->width; x++) {
                EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), GET_SESSION(term)->last_cursor_y, x);
                print_buffer[pos++] = GetPrintableChar(cell->ch, &GET_SESSION(term)->charset);
            }
            if (pos < term->width + 1) {
                print_buffer[pos++] = '\n';
                print_buffer[pos] = '\0';
                KTerm_QueueResponse(term, print_buffer);
            }
        }
        GET_SESSION(term)->last_cursor_y = GET_SESSION(term)->cursor.y;
    }

    KTerm_Draw(term);
}


/**
 * @brief Renders the current visual state of the terminal to the Platform window.
 * This function must be called once per frame, within KTerm_BeginFrame()`
 * and `KTerm_EndFrame()` block. It translates the terminal's internal model into a
 * graphical representation.
 *
 * Key rendering capabilities and processes:
 *  -   **Character Grid**: Iterates through the active screen buffer (primary or alternate),
 *      drawing each `EnhancedTermChar`.
 *  -   **KTermColor Resolution**: Handles various color modes for foreground and background:
 *      - Standard 16 ANSI colors.
 *      - 256-color indexed palette.
 *      - 24-bit True KTermColor (RGB).
 *  -   **Text Attributes**: Applies a rich set of visual attributes:
 *      - Bold (typically by using brighter ANSI colors or a bold font variant).
 *      - Faint (dimmed text).
 *      - Italic (if supported by font or simulated).
 *      - Underline (single and double).
 *      - Blink (text alternates visibility based on `GET_SESSION(term)->text_blink_state`).
 *      - Reverse video (swaps foreground and background colors, per cell or screen-wide).
 *      - Strikethrough.
 *      - Overline.
 *      - Concealed text (rendered as spaces or not at all).
 *  -   **Glyph Rendering**: Uses a pre-rendered bitmap font (`font_texture`), typically
 *      CP437-based for broad character support within that range. Unicode characters
 *      outside this basic set might be mapped to '?' or require a more advanced font system.
 *  -   **Cursor**: Draws the cursor according to its current `EnhancedCursor` properties:
 *      - Shape: `CURSOR_BLOCK`, `CURSOR_UNDERLINE`, `CURSOR_BAR`.
 *      - Blink: Synchronized with `GET_SESSION(term)->cursor.blink_state`.
 *      - Visibility: Honors `GET_SESSION(term)->cursor.visible`.
 *      - KTermColor: Uses `GET_SESSION(term)->cursor.color`.
 *  -   **Sixel Graphics**: If `GET_SESSION(term)->sixel.active` is true, Sixel graphics data is
 *      rendered, typically overlaid on the text grid.
 *  -   **Visual Bell**: If `GET_SESSION(term)->visual_bell_timer` is active, a visual flash effect
 *      may be rendered.
 *
 * The terminal provides a faithful visual emulation, leveraging the Platform for efficient
 * 2D rendering.
 *
 * @see EnhancedTermChar for the structure defining each character cell's properties.
 * @see EnhancedCursor for cursor attributes.
 * @see SixelGraphics for Sixel display state.
 * @see KTerm_Init(term) where `font_texture` is created.
 */
// =============================================================================
// BIDI (BIDIRECTIONAL) TEXT SUPPORT
// =============================================================================

// BiDi Types
#define BIDI_L  0 // Left-to-Right
#define BIDI_R  1 // Right-to-Left
#define BIDI_N  2 // Neutral (Numbers, Punctuation) - Simplified

// Helper: Check if character is RTL (Hebrew/Arabic ranges)
static bool IsRTL(uint32_t ch) {
    // Hebrew: 0590-05FF
    if (ch >= 0x0590 && ch <= 0x05FF) return true;
    // Arabic: 0600-06FF
    if (ch >= 0x0600 && ch <= 0x06FF) return true;
    // Arabic Supplement: 0750-077F
    if (ch >= 0x0750 && ch <= 0x077F) return true;
    // Arabic Extended-A: 08A0-08FF
    if (ch >= 0x08A0 && ch <= 0x08FF) return true;
    // FB50-FDFF: Arabic Presentation Forms-A
    if (ch >= 0xFB50 && ch <= 0xFDFF) return true;
    // FE70-FEFF: Arabic Presentation Forms-B
    if (ch >= 0xFE70 && ch <= 0xFEFF) return true;

    return false;
}

// Helper: Get Simplified BiDi Type
static int GetBiDiType(uint32_t ch) {
    if (IsRTL(ch)) return BIDI_R;

    // Digits (0-9) are treated as LTR to prevent reversal (e.g. 123 -> 321)
    if (ch >= '0' && ch <= '9') return BIDI_L;

    // Weak/Neutral characters (Simplified)
    // Spaces, Punctuation
    if (ch < 0x41) return BIDI_N; // Space, Punctuation
    if ((ch >= 0x5B && ch <= 0x60) || (ch >= 0x7B && ch <= 0x7E)) return BIDI_N; // brackets, etc.

    // Default to Left-to-Right (Latin, Cyrillic, etc.)
    return BIDI_L;
}

// Helper: Mirror characters (Parentheses, Brackets)
static uint32_t GetMirroredChar(uint32_t ch) {
    switch (ch) {
        case '(': return ')';
        case ')': return '(';
        case '[': return ']';
        case ']': return '[';
        case '{': return '}';
        case '}': return '{';
        case '<': return '>';
        case '>': return '<';
        default: return ch;
    }
}

// Helper: Reverse a run of characters in place
static void ReverseRun(EnhancedTermChar* row, int start, int end) {
    while (start < end) {
        EnhancedTermChar temp = row[start];
        row[start] = row[end];
        row[end] = temp;

        // Apply mirroring when reversing
        row[start].ch = GetMirroredChar(row[start].ch);
        row[end].ch = GetMirroredChar(row[end].ch);

        start++;
        end--;
    }
    // Handle middle char if any
    if (start == end) {
        row[start].ch = GetMirroredChar(row[start].ch);
    }
}

// Main Reordering Algorithm (Visual Reordering)
// Note: This internal implementation is used because fribidi is unavailable.
static void BiDiReorderRow(KTermSession* session, EnhancedTermChar* row, int width) {
    // Only reorder if BDSM is enabled
    if (!session->dec_modes.bidi_mode) return;

    // Use stack buffer for types (safe size for max terminal width)
    int types[512];
    int effective_width = (width < 512) ? width : 512;

    // 1. Determine base direction (Assume LTR for now)
    // First pass: Classify
    for (int i = 0; i < effective_width; i++) {
        types[i] = GetBiDiType(row[i].ch);
    }

    // Second pass: Resolve Neutrals
    int last_strong = BIDI_L; // Base direction
    for (int i = 0; i < effective_width; i++) {
        if (types[i] != BIDI_N) {
            last_strong = types[i];
        } else {
            // Look ahead for next strong
            int next_strong = BIDI_L; // Default to base if end of line
            for (int j = i + 1; j < effective_width; j++) {
                if (types[j] != BIDI_N) {
                    next_strong = types[j];
                    break;
                }
            }

            // Resolve neutral
            if (last_strong == next_strong) {
                types[i] = last_strong;
            } else {
                types[i] = BIDI_L; // Base direction (L)
            }
        }
    }

    // Third pass: Identify and Reverse R runs
    int run_start = -1;
    for (int i = 0; i < effective_width; i++) {
        if (types[i] == BIDI_R) {
            if (run_start == -1) run_start = i;
        } else {
            if (run_start != -1) {
                ReverseRun(row, run_start, i - 1);
                run_start = -1;
            }
        }
    }
    if (run_start != -1) {
        ReverseRun(row, run_start, effective_width - 1);
    }
}

// Updated Helper: Update a specific row segment for a pane
static void KTerm_UpdatePaneRow(KTerm* term, KTermSession* source_session, int global_x, int global_y, int width, int source_y) {
    if (source_y >= source_session->rows || source_y < 0) return;

    // --- BiDi Processing (Visual Reordering) ---
    // We copy the row to a temporary buffer to reorder it for display
    // without modifying the logical screen buffer.

    // Use scratch buffer if available and large enough
    EnhancedTermChar* temp_row = NULL;
    bool using_scratch = false;

    if (term->row_scratch_buffer && width <= term->width) {
        temp_row = term->row_scratch_buffer;
        using_scratch = true;
    } else {
        temp_row = (EnhancedTermChar*)malloc(width * sizeof(EnhancedTermChar));
    }

    if (!temp_row) return;

    EnhancedTermChar* src_row_ptr = GetScreenRow(source_session, source_y);

    // Copy only valid columns (min of session width and pane width)
    int copy_width = (width < source_session->cols) ? width : source_session->cols;
    memcpy(temp_row, src_row_ptr, copy_width * sizeof(EnhancedTermChar));

    // Fill remaining if pane is wider than session (should not happen with correct resize logic)
    if (width > copy_width) {
        memset(temp_row + copy_width, 0, (width - copy_width) * sizeof(EnhancedTermChar));
    }

    bool has_rtl = false;
    for (int x = 0; x < width; x++) {
        if (IsRTL(temp_row[x].ch)) {
            has_rtl = true;
            break;
        }
    }

    if (has_rtl) {
        BiDiReorderRow(source_session, temp_row, width);
    }
    // -------------------------------------------

    for (int x = 0; x < width; x++) {
        EnhancedTermChar* cell = &temp_row[x];

        // Calculate offset in global staging buffer
        int gx = global_x + x;
        int gy = global_y;

        // Bounds check against global terminal dimensions
        if (gx < 0 || gx >= term->width || gy < 0 || gy >= term->height) continue;

        size_t offset = gy * term->width + gx;
        GPUCell* gpu_cell = &term->gpu_staging_buffer[offset];

        // Dynamic Glyph Mapping
        uint32_t char_code;
        if (cell->ch < 256) {
            char_code = cell->ch; // Base set
        } else {
            char_code = KTerm_AllocateGlyph(term, cell->ch);
        }
        gpu_cell->char_code = char_code;

        // Update LRU if it's a dynamic glyph
        if (char_code >= 256 && char_code != '?') {
            term->glyph_last_used[char_code] = term->frame_count;
        }

        KTermColor fg = {255, 255, 255, 255};
        if (cell->fg_color.color_mode == 0) {
             if (cell->fg_color.value.index < 16) fg = ansi_colors[cell->fg_color.value.index];
             else {
                 RGB_KTermColor c = term->color_palette[cell->fg_color.value.index];
                 fg = (KTermColor){c.r, c.g, c.b, 255};
             }
        } else {
            fg = (KTermColor){cell->fg_color.value.rgb.r, cell->fg_color.value.rgb.g, cell->fg_color.value.rgb.b, 255};
        }
        gpu_cell->fg_color = (uint32_t)fg.r | ((uint32_t)fg.g << 8) | ((uint32_t)fg.b << 16) | ((uint32_t)fg.a << 24);

        KTermColor bg = {0, 0, 0, 255};
        if (cell->bg_color.color_mode == 0) {
             if (cell->bg_color.value.index < 16) {
                 bg = ansi_colors[cell->bg_color.value.index];
                 if (cell->bg_color.value.index == 0) bg.a = 0; // Make standard black transparent for compositing
             }
             else {
                 RGB_KTermColor c = term->color_palette[cell->bg_color.value.index];
                 bg = (KTermColor){c.r, c.g, c.b, 255};
             }
        } else {
            bg = (KTermColor){cell->bg_color.value.rgb.r, cell->bg_color.value.rgb.g, cell->bg_color.value.rgb.b, 255};
        }
        gpu_cell->bg_color = (uint32_t)bg.r | ((uint32_t)bg.g << 8) | ((uint32_t)bg.b << 16) | ((uint32_t)bg.a << 24);

        KTermColor ul = fg;
        if (cell->ul_color.color_mode != 2) {
             if (cell->ul_color.color_mode == 0) {
                 if (cell->ul_color.value.index < 16) ul = ansi_colors[cell->ul_color.value.index];
                 else {
                     RGB_KTermColor c = term->color_palette[cell->ul_color.value.index];
                     ul = (KTermColor){c.r, c.g, c.b, 255};
                 }
             } else {
                 ul = (KTermColor){cell->ul_color.value.rgb.r, cell->ul_color.value.rgb.g, cell->ul_color.value.rgb.b, 255};
             }
        }
        gpu_cell->ul_color = (uint32_t)ul.r | ((uint32_t)ul.g << 8) | ((uint32_t)ul.b << 16) | ((uint32_t)ul.a << 24);

        KTermColor st = fg;
        if (cell->st_color.color_mode != 2) {
             if (cell->st_color.color_mode == 0) {
                 if (cell->st_color.value.index < 16) st = ansi_colors[cell->st_color.value.index];
                 else {
                     RGB_KTermColor c = term->color_palette[cell->st_color.value.index];
                     st = (KTermColor){c.r, c.g, c.b, 255};
                 }
             } else {
                 st = (KTermColor){cell->st_color.value.rgb.r, cell->st_color.value.rgb.g, cell->st_color.value.rgb.b, 255};
             }
        }
        gpu_cell->st_color = (uint32_t)st.r | ((uint32_t)st.g << 8) | ((uint32_t)st.b << 16) | ((uint32_t)st.a << 24);

        gpu_cell->flags = cell->flags & 0xFFFF; // Copy shared visual attributes
        if (source_session->dec_modes.reverse_video) {
            gpu_cell->flags ^= KTERM_ATTR_REVERSE;
        }
        if (source_session->grid_enabled) {
            gpu_cell->flags |= KTERM_ATTR_GRID;
        }
    }

    if (!using_scratch) free(temp_row);

    // Mark row as clean in session
    source_session->row_dirty[source_y] = false;
}

static bool RecursiveUpdateSSBO(KTerm* term, KTermPane* pane) {
    if (!pane) return false;
    bool any_update = false;

    if (pane->type == PANE_LEAF) {
        if (pane->session_index >= 0 && pane->session_index < MAX_SESSIONS) {
            KTermSession* session = &term->sessions[pane->session_index];
            if (session->session_open) {
                // Iterate over visible rows of the pane
                for (int y = 0; y < pane->height; y++) {
                    // Check if this row is dirty in the session
                    // Note: session rows should match pane height
                    if (y < session->rows && session->row_dirty[y]) {
                        KTerm_UpdatePaneRow(term, session, pane->x, pane->y + y, pane->width, y);
                        any_update = true;
                    }
                }
            }
        }
    } else {
        if (RecursiveUpdateSSBO(term, pane->child_a)) any_update = true;
        if (RecursiveUpdateSSBO(term, pane->child_b)) any_update = true;
    }
    return any_update;
}

void KTerm_UpdateSSBO(KTerm* term) {
    if (!term->terminal_buffer.id || !term->gpu_staging_buffer) return;

    // Update global LRU clock
    term->frame_count++;

    bool any_upload_needed = false;

    // Use recursive layout update
    if (term->layout_root) {
        any_upload_needed = RecursiveUpdateSSBO(term, term->layout_root);
    } else {
        // Fallback for no layout (legacy single session?)
        if (term->active_session >= 0) {
            KTermSession* s = GET_SESSION(term);
            for(int y=0; y<term->height; y++) {
                 if (y < s->rows && s->row_dirty[y]) {
                     KTerm_UpdatePaneRow(term, s, 0, y, term->width, y);
                     any_upload_needed = true;
                 }
            }
        }
    }

    if (any_upload_needed) {
        size_t required_size = term->width * term->height * sizeof(GPUCell);
        KTerm_UpdateBuffer(term->terminal_buffer, 0, required_size, term->gpu_staging_buffer);
    }
}

// New API functions


void KTerm_Draw(KTerm* term) {
    if (!term->compute_initialized) return;

    // Handle Soft Font Update
    if (GET_SESSION(term)->soft_font.dirty || term->font_atlas_dirty) {
        if (term->font_atlas_pixels) {
            KTermImage img = {0};
            img.width = term->atlas_width;
            img.height = term->atlas_height;
            img.channels = 4;
            img.data = term->font_atlas_pixels; // Pointer alias, don't free

            // Re-upload full texture (Safe Pattern: Create New -> Check -> Swap -> Destroy Old)
            KTermTexture new_texture = {0};
            KTerm_CreateTexture(img, false, &new_texture);

            if (new_texture.id != 0) {
                if (term->font_texture.generation != 0) KTerm_DestroyTexture(&term->font_texture);
                term->font_texture = new_texture;
            } else {
                 if (GET_SESSION(term)->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Font texture creation failed");
            }
        }
        GET_SESSION(term)->soft_font.dirty = false;
        term->font_atlas_dirty = false;
    }

    // Handle Sixel Texture Creation/Upload (Compute Shader)
    if (GET_SESSION(term)->sixel.active && GET_SESSION(term)->sixel.strips) {
        // Calculate Y Offset for scrolling
        int y_shift = 0;
        if (GET_SESSION(term)->sixel.scrolling) {
            // How many rows have we scrolled since image started?
            // current head - start head
            // screen_head moves down (increments) as we add new lines at bottom?
            // Actually, screen_head is top of ring buffer.
            // Calculate ring buffer distance (How many rows have we scrolled?)
            // screen_head only moves forward (incrementing index).
            // distance = (current_head - start_head + buffer_height) % buffer_height
            int height = GET_SESSION(term)->buffer_height;
            int dist = (GET_SESSION(term)->screen_head - GET_SESSION(term)->sixel.logical_start_row);

            // Normalize to positive range [0, height-1]
            if (dist < 0) dist += height;
            dist %= height;

            // Shift amount (pixels moving UP) = dist * char_height.
            // Plus view_offset (scrolling back moves content DOWN).
            y_shift = (dist * term->char_height) - (GET_SESSION(term)->view_offset * term->char_height);
        }

        // 1. Check if dirty (new data)
        // Logic to trigger texture update (dispatch compute shader)
        bool offset_changed = (y_shift != GET_SESSION(term)->sixel.last_y_shift);
        if (offset_changed) GET_SESSION(term)->sixel.last_y_shift = y_shift;

        bool needs_dispatch = GET_SESSION(term)->sixel.dirty || offset_changed;

        if (needs_dispatch) {
            // 1. Upload Buffers only if dirty (content changed)
            if (GET_SESSION(term)->sixel.dirty) {
                // Upload Strips
                if (GET_SESSION(term)->sixel.strip_count > 0) {
                    KTerm_UpdateBuffer(term->sixel_buffer, 0, GET_SESSION(term)->sixel.strip_count * sizeof(GPUSixelStrip), GET_SESSION(term)->sixel.strips);
                }

                // Repack Palette safely
                uint32_t packed_palette[256];
                for(int i=0; i<256; i++) {
                    RGB_KTermColor c = GET_SESSION(term)->sixel.palette[i];
                    packed_palette[i] = (uint32_t)c.r | ((uint32_t)c.g << 8) | ((uint32_t)c.b << 16) | ((uint32_t)c.a << 24);
                }
                KTerm_UpdateBuffer(term->sixel_palette_buffer, 0, 256 * sizeof(uint32_t), packed_palette);
            }

            // 2. Dispatch Compute Shader to render to texture
            // FORCE RECREATE TEXTURE TO CLEAR IT (Essential for ytop/lsix to prevent smearing)

            KTermImage img = {0};
            // CreateImage typically returns zeroed buffer
            KTerm_CreateImage(GET_SESSION(term)->sixel.width, GET_SESSION(term)->sixel.height, 4, &img);
            if (img.data) memset(img.data, 0, GET_SESSION(term)->sixel.width * GET_SESSION(term)->sixel.height * 4); // Ensure zeroed

            KTermTexture new_sixel_tex = {0};
            KTerm_CreateTextureEx(img, false, KTERM_TEXTURE_USAGE_SAMPLED | KTERM_TEXTURE_USAGE_STORAGE | KTERM_TEXTURE_USAGE_TRANSFER_DST, &new_sixel_tex);

            if (new_sixel_tex.id != 0) {
                if (term->sixel_texture.generation != 0) KTerm_DestroyTexture(&term->sixel_texture);
                term->sixel_texture = new_sixel_tex;
            } else {
                if (GET_SESSION(term)->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Sixel texture creation failed");
            }

            KTerm_UnloadImage(img);

            if (KTerm_AcquireFrameCommandBuffer()) {
                KTermCommandBuffer cmd = KTerm_GetCommandBuffer();
                if (KTerm_CmdBindPipeline(cmd, term->sixel_pipeline) != KTERM_SUCCESS ||
                    KTerm_CmdBindTexture(cmd, 0, term->sixel_texture) != KTERM_SUCCESS) {
                    if (GET_SESSION(term)->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Sixel compute bind failed");
                } else {
                    // Push Constants
                    KTermPushConstants pc = {0};
                    pc.screen_size = (KTermVector2){{(float)GET_SESSION(term)->sixel.width, (float)GET_SESSION(term)->sixel.height}};
                    pc.vector_count = GET_SESSION(term)->sixel.strip_count;
                    pc.vector_buffer_addr = KTerm_GetBufferAddress(term->sixel_buffer); // Reusing field for sixel buffer
                    pc.terminal_buffer_addr = KTerm_GetBufferAddress(term->sixel_palette_buffer); // Reusing field for palette
                    pc.sixel_y_offset = y_shift;

                    if (KTerm_CmdSetPushConstant(cmd, 0, &pc, sizeof(pc)) != KTERM_SUCCESS) {
                        if (GET_SESSION(term)->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Sixel push constant failed");
                    } else {
                        if (KTerm_CmdDispatch(cmd, (GET_SESSION(term)->sixel.strip_count + 63) / 64, 1, 1) != KTERM_SUCCESS) {
                             if (GET_SESSION(term)->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Sixel dispatch failed");
                        }
                        KTerm_CmdPipelineBarrier(cmd, KTERM_BARRIER_COMPUTE_SHADER_WRITE, KTERM_BARRIER_COMPUTE_SHADER_READ);
                    }
                }
            }
            GET_SESSION(term)->sixel.dirty = false;
        }
    }

    KTerm_UpdateSSBO(term);

    if (KTerm_AcquireFrameCommandBuffer()) {
        KTermCommandBuffer cmd = KTerm_GetCommandBuffer();

        // --- Vector Layer Management ---
        if (term->vector_clear_request) {
            // Clear the persistent vector layer
            KTermImage clear_img = {0};
            if (KTerm_CreateImage(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 4, &clear_img) == KTERM_SUCCESS) {
                memset(clear_img.data, 0, DEFAULT_WINDOW_WIDTH * DEFAULT_WINDOW_HEIGHT * 4);
                if (term->vector_layer_texture.generation != 0) KTerm_DestroyTexture(&term->vector_layer_texture);
                KTerm_CreateTextureEx(clear_img, false, KTERM_TEXTURE_USAGE_SAMPLED | KTERM_TEXTURE_USAGE_STORAGE | KTERM_TEXTURE_USAGE_TRANSFER_DST, &term->vector_layer_texture);
                KTerm_UnloadImage(clear_img);
            }
            term->vector_clear_request = false;
        }

        // --- 1. Clear Screen (using 1x1 opaque black texture blit) ---
        if (term->texture_blit_pipeline.id != 0 && term->clear_texture.id != 0) {
            if (KTerm_CmdBindPipeline(cmd, term->texture_blit_pipeline) == KTERM_SUCCESS &&
                KTerm_CmdBindTexture(cmd, 1, term->output_texture) == KTERM_SUCCESS) {

                struct {
                    int dst_x, dst_y;
                    int src_w, src_h;
                    uint64_t handle;
                    uint64_t _padding;
                    int clip_x, clip_y, clip_mx, clip_my;
                } blit_pc;

                blit_pc.dst_x = 0;
                blit_pc.dst_y = 0;
                blit_pc.src_w = term->width * term->char_width * DEFAULT_WINDOW_SCALE; // Blit over full window area
                blit_pc.src_h = term->height * term->char_height * DEFAULT_WINDOW_SCALE;
                blit_pc.handle = KTerm_GetTextureHandle(term->clear_texture);
                blit_pc._padding = 0;
                blit_pc.clip_x = 0;
                blit_pc.clip_y = 0;
                blit_pc.clip_mx = blit_pc.src_w;
                blit_pc.clip_my = blit_pc.src_h;

                KTerm_CmdSetPushConstant(cmd, 0, &blit_pc, sizeof(blit_pc));
                // Dispatch full screen coverage
                KTerm_CmdDispatch(cmd, (blit_pc.src_w + 15) / 16, (blit_pc.src_h + 15) / 16, 1);
                KTerm_CmdPipelineBarrier(cmd, KTERM_BARRIER_COMPUTE_SHADER_WRITE, KTERM_BARRIER_COMPUTE_SHADER_READ);
            }
        }

        // --- 2. Kitty Graphics Overlay (Z < 0: Background) ---
        if (term->texture_blit_pipeline.id != 0) {
            for (int i = 0; i < MAX_SESSIONS; i++) {
                KTermSession* session = &term->sessions[i];
                if (!session->session_open || !session->kitty.images) continue;

                // Find pane clipping
                KTermPane* pane = NULL;
                KTermPane* stack[32];
                int stack_top = 0;
                if (term->layout_root) stack[stack_top++] = term->layout_root;
                while (stack_top > 0) {
                    KTermPane* p = stack[--stack_top];
                    if (p->type == PANE_LEAF) {
                        if (p->session_index == i) { pane = p; break; }
                    } else {
                        if (p->child_b) stack[stack_top++] = p->child_b;
                        if (p->child_a) stack[stack_top++] = p->child_a;
                    }
                }
                if (!pane) continue;

                for (int k = 0; k < session->kitty.image_count; k++) {
                    KittyImageBuffer* img = &session->kitty.images[k];
                    // Skip if Z >= 0
                    if (img->z_index >= 0 || !img->visible || !img->frames || img->frame_count == 0 || !img->complete) continue;

                    if (img->current_frame >= img->frame_count) img->current_frame = 0;
                    KittyFrame* frame = &img->frames[img->current_frame];

                    if (frame->texture.id == 0) {
                        KTermImage kimg = {0};
                        kimg.width = frame->width;
                        kimg.height = frame->height;
                        kimg.channels = 4;
                        kimg.data = frame->data;
                        KTerm_CreateTextureEx(kimg, false, KTERM_TEXTURE_USAGE_SAMPLED, &frame->texture);
                    }
                    if (frame->texture.id == 0) continue;

                    int dist = (session->screen_head - img->start_row + session->buffer_height) % session->buffer_height;
                    int y_shift = (dist * term->char_height) - (session->view_offset * term->char_height);
                    int draw_x = (pane->x * term->char_width) + img->x;
                    int draw_y = (pane->y * term->char_height) + img->y - y_shift;
                    int clip_min_x = pane->x * term->char_width;
                    int clip_min_y = pane->y * term->char_height;
                    int clip_max_x = clip_min_x + pane->width * term->char_width - 1;
                    int clip_max_y = clip_min_y + pane->height * term->char_height - 1;

                    if (KTerm_CmdBindPipeline(cmd, term->texture_blit_pipeline) == KTERM_SUCCESS &&
                        KTerm_CmdBindTexture(cmd, 1, term->output_texture) == KTERM_SUCCESS) {
                         struct { int dst_x, dst_y; int src_w, src_h; uint64_t handle; uint64_t _padding; int clip_x, clip_y, clip_mx, clip_my; } blit_pc;
                         blit_pc.dst_x = draw_x; blit_pc.dst_y = draw_y;
                         blit_pc.src_w = frame->width; blit_pc.src_h = frame->height;
                         blit_pc.handle = KTerm_GetTextureHandle(frame->texture); blit_pc._padding = 0;
                         blit_pc.clip_x = clip_min_x; blit_pc.clip_y = clip_min_y;
                         blit_pc.clip_mx = clip_max_x; blit_pc.clip_my = clip_max_y;
                         KTerm_CmdSetPushConstant(cmd, 0, &blit_pc, sizeof(blit_pc));
                         KTerm_CmdDispatch(cmd, (frame->width + 15) / 16, (frame->height + 15) / 16, 1);
                         KTerm_CmdPipelineBarrier(cmd, KTERM_BARRIER_COMPUTE_SHADER_WRITE, KTERM_BARRIER_COMPUTE_SHADER_READ);
                    }
                }
            }
        }

        // --- 3. Terminal Text (Composite over Background) ---
        if (KTerm_CmdBindPipeline(cmd, term->compute_pipeline) == KTERM_SUCCESS &&
            KTerm_CmdBindTexture(cmd, 1, term->output_texture) == KTERM_SUCCESS) {

            KTermPushConstants pc = {0};
            pc.terminal_buffer_addr = KTerm_GetBufferAddress(term->terminal_buffer);
            pc.font_texture_handle = KTerm_GetTextureHandle(term->font_texture);
            if (GET_SESSION(term)->sixel.active && term->sixel_texture.generation != 0) {
                pc.sixel_texture_handle = KTerm_GetTextureHandle(term->sixel_texture);
            } else {
                pc.sixel_texture_handle = KTerm_GetTextureHandle(term->dummy_sixel_texture);
            }
            pc.vector_texture_handle = KTerm_GetTextureHandle(term->vector_layer_texture);
            pc.atlas_cols = term->atlas_cols;
            pc.screen_size = (KTermVector2){{(float)(term->width * term->char_width * DEFAULT_WINDOW_SCALE), (float)(term->height * term->char_height * DEFAULT_WINDOW_SCALE)}};
            int char_w = term->char_width;
            int char_h = term->char_height;
            if (GET_SESSION(term)->soft_font.active) {
                char_w = GET_SESSION(term)->soft_font.char_width;
                char_h = GET_SESSION(term)->soft_font.char_height;
            }
            pc.char_size = (KTermVector2){{(float)char_w, (float)char_h}};
            pc.grid_size = (KTermVector2){{(float)term->width, (float)term->height}};
            pc.time = (float)KTerm_TimerGetTime();

            uint32_t cursor_idx = 0xFFFFFFFF;
            KTermSession* focused_session = NULL;
            if (term->focused_pane && term->focused_pane->type == PANE_LEAF) {
                if (term->focused_pane->session_index >= 0) focused_session = &term->sessions[term->focused_pane->session_index];
            }
            if (!focused_session) focused_session = GET_SESSION(term);

            if (focused_session && focused_session->session_open && focused_session->cursor.visible) {
                int origin_x = term->focused_pane ? term->focused_pane->x : 0;
                int origin_y = term->focused_pane ? term->focused_pane->y : 0;
                int gx = origin_x + focused_session->cursor.x;
                int gy = origin_y + focused_session->cursor.y;
                if (gx >= 0 && gx < term->width && gy >= 0 && gy < term->height) cursor_idx = gy * term->width + gx;
            }
            pc.cursor_index = cursor_idx;

            if (focused_session && focused_session->mouse.enabled && focused_session->mouse.cursor_x > 0) {
                 int mx = focused_session->mouse.cursor_x - 1;
                 int my = focused_session->mouse.cursor_y - 1;
                 if (term->focused_pane) { mx += term->focused_pane->x; my += term->focused_pane->y; }
                 if (mx >= 0 && mx < term->width && my >= 0 && my < term->height) pc.mouse_cursor_index = my * term->width + mx;
                 else pc.mouse_cursor_index = 0xFFFFFFFF;
            } else pc.mouse_cursor_index = 0xFFFFFFFF;

            pc.cursor_blink_state = focused_session ? (focused_session->cursor.blink_state ? 1 : 0) : 0;
            pc.text_blink_state = focused_session ? focused_session->text_blink_state : 0;

            if (GET_SESSION(term)->selection.active) {
                 uint32_t start_idx = GET_SESSION(term)->selection.start_y * term->width + GET_SESSION(term)->selection.start_x;
                 uint32_t end_idx = GET_SESSION(term)->selection.end_y * term->width + GET_SESSION(term)->selection.end_x;
                 if (start_idx > end_idx) { uint32_t t = start_idx; start_idx = end_idx; end_idx = t; }
                 pc.sel_start = start_idx; pc.sel_end = end_idx; pc.sel_active = 1;
            }
            pc.scanline_intensity = term->visual_effects.scanline_intensity;
            pc.crt_curvature = term->visual_effects.curvature;
            if (GET_SESSION(term)->visual_bell_timer > 0.0) {
                float intensity = (float)(GET_SESSION(term)->visual_bell_timer / 0.2);
                if (intensity > 1.0f) intensity = 1.0f; else if (intensity < 0.0f) intensity = 0.0f;
                pc.visual_bell_intensity = intensity;
            }

            if (focused_session) {
                RGB_KTermColor gc = focused_session->grid_color;
                pc.grid_color = (uint32_t)gc.r | ((uint32_t)gc.g << 8) | ((uint32_t)gc.b << 16) | ((uint32_t)gc.a << 24);
                pc.conceal_char_code = focused_session->conceal_char_code;
            }

            KTerm_CmdSetPushConstant(cmd, 0, &pc, sizeof(pc));
            KTerm_CmdDispatch(cmd, term->width, term->height, 1);
            KTerm_CmdPipelineBarrier(cmd, KTERM_BARRIER_COMPUTE_SHADER_WRITE, KTERM_BARRIER_COMPUTE_SHADER_READ);
        }

        // --- 4. Kitty Graphics Overlay (Z >= 0: Foreground) ---
        if (term->texture_blit_pipeline.id != 0) {
            for (int i = 0; i < MAX_SESSIONS; i++) {
                KTermSession* session = &term->sessions[i];
                if (!session->session_open || !session->kitty.images) continue;

                KTermPane* pane = NULL;
                KTermPane* stack[32];
                int stack_top = 0;
                if (term->layout_root) stack[stack_top++] = term->layout_root;
                while (stack_top > 0) {
                    KTermPane* p = stack[--stack_top];
                    if (p->type == PANE_LEAF) {
                        if (p->session_index == i) { pane = p; break; }
                    } else {
                        if (p->child_b) stack[stack_top++] = p->child_b;
                        if (p->child_a) stack[stack_top++] = p->child_a;
                    }
                }
                if (!pane) continue;

                for (int k = 0; k < session->kitty.image_count; k++) {
                    KittyImageBuffer* img = &session->kitty.images[k];
                    // Skip if Z < 0
                    if (img->z_index < 0 || !img->visible || !img->frames || img->frame_count == 0 || !img->complete) continue;

                    if (img->current_frame >= img->frame_count) img->current_frame = 0;
                    KittyFrame* frame = &img->frames[img->current_frame];

                    if (frame->texture.id == 0) {
                        KTermImage kimg = {0};
                        kimg.width = frame->width;
                        kimg.height = frame->height;
                        kimg.channels = 4;
                        kimg.data = frame->data;
                        KTerm_CreateTextureEx(kimg, false, KTERM_TEXTURE_USAGE_SAMPLED, &frame->texture);
                    }
                    if (frame->texture.id == 0) continue;

                    int dist = (session->screen_head - img->start_row + session->buffer_height) % session->buffer_height;
                    int y_shift = (dist * term->char_height) - (session->view_offset * term->char_height);
                    int draw_x = (pane->x * term->char_width) + img->x;
                    int draw_y = (pane->y * term->char_height) + img->y - y_shift;
                    int clip_min_x = pane->x * term->char_width;
                    int clip_min_y = pane->y * term->char_height;
                    int clip_max_x = clip_min_x + pane->width * term->char_width - 1;
                    int clip_max_y = clip_min_y + pane->height * term->char_height - 1;

                    if (KTerm_CmdBindPipeline(cmd, term->texture_blit_pipeline) == KTERM_SUCCESS &&
                        KTerm_CmdBindTexture(cmd, 1, term->output_texture) == KTERM_SUCCESS) {
                         struct { int dst_x, dst_y; int src_w, src_h; uint64_t handle; uint64_t _padding; int clip_x, clip_y, clip_mx, clip_my; } blit_pc;
                         blit_pc.dst_x = draw_x; blit_pc.dst_y = draw_y;
                         blit_pc.src_w = frame->width; blit_pc.src_h = frame->height;
                         blit_pc.handle = KTerm_GetTextureHandle(frame->texture); blit_pc._padding = 0;
                         blit_pc.clip_x = clip_min_x; blit_pc.clip_y = clip_min_y;
                         blit_pc.clip_mx = clip_max_x; blit_pc.clip_my = clip_max_y;
                         KTerm_CmdSetPushConstant(cmd, 0, &blit_pc, sizeof(blit_pc));
                         KTerm_CmdDispatch(cmd, (frame->width + 15) / 16, (frame->height + 15) / 16, 1);
                         KTerm_CmdPipelineBarrier(cmd, KTERM_BARRIER_COMPUTE_SHADER_WRITE, KTERM_BARRIER_COMPUTE_SHADER_READ);
                    }
                }
            }
        }

        // --- 5. Vector Drawing Pass ---
        if (term->vector_count > 0) {
            KTerm_UpdateBuffer(term->vector_buffer, 0, term->vector_count * sizeof(GPUVectorLine), term->vector_staging_buffer);
            if (KTerm_CmdBindPipeline(cmd, term->vector_pipeline) == KTERM_SUCCESS &&
                KTerm_CmdBindTexture(cmd, 1, term->vector_layer_texture) == KTERM_SUCCESS) {

                KTermPushConstants pc = {0};
                pc.vector_count = term->vector_count;
                pc.vector_buffer_addr = KTerm_GetBufferAddress(term->vector_buffer);
                KTerm_CmdSetPushConstant(cmd, 0, &pc, sizeof(pc));
                KTerm_CmdDispatch(cmd, (term->vector_count + 63) / 64, 1, 1);
                KTerm_CmdPipelineBarrier(cmd, KTERM_BARRIER_COMPUTE_SHADER_WRITE, KTERM_BARRIER_COMPUTE_SHADER_READ);
            }
            term->vector_count = 0;
        }

        KTerm_CmdPipelineBarrier(cmd, KTERM_BARRIER_COMPUTE_SHADER_WRITE, KTERM_BARRIER_TRANSFER_READ);
        if (KTerm_CmdPresent(cmd, term->output_texture) != KTERM_SUCCESS) {
             if (GET_SESSION(term)->options.debug_sequences) KTerm_LogUnsupportedSequence(term, "Present failed");
        }
    }

        KTerm_EndFrame();
    }


// --- Lifecycle Management ---

/**
 * @brief Cleans up all resources allocated by the terminal library.
 * This function must be called when the application is shutting down, typically
 * after the main loop has exited and before closing the Platform window (if Platform
 * is managed by the application).
 *
 * Its responsibilities include deallocating:
 *  - The main font texture (`font_texture`) loaded into GPU memory.
 *  - Memory used for storing sequences of programmable keys (`GET_SESSION(term)->programmable_keys`).
 *  - The Sixel graphics data buffer (`GET_SESSION(term)->sixel.data`) if it was allocated.
 *  - The buffer for bracketed paste data (`GET_SESSION(term)->bracketed_paste.buffer`) if used.
 *
 * It also ensures the input pipeline is cleared. Proper cleanup prevents memory leaks
 * and releases GPU resources.
 */
void KTerm_Cleanup(KTerm* term) {
    // Free LRU Cache
    if (term->glyph_map) { free(term->glyph_map); term->glyph_map = NULL; }
    if (term->glyph_last_used) { free(term->glyph_last_used); term->glyph_last_used = NULL; }
    if (term->atlas_to_codepoint) { free(term->atlas_to_codepoint); term->atlas_to_codepoint = NULL; }
    if (term->font_atlas_pixels) { free(term->font_atlas_pixels); term->font_atlas_pixels = NULL; }

    if (term->font_texture.generation != 0) KTerm_DestroyTexture(&term->font_texture);
    if (term->output_texture.generation != 0) KTerm_DestroyTexture(&term->output_texture);
    if (term->sixel_texture.generation != 0) KTerm_DestroyTexture(&term->sixel_texture);
    if (term->dummy_sixel_texture.generation != 0) KTerm_DestroyTexture(&term->dummy_sixel_texture);
    if (term->clear_texture.generation != 0) KTerm_DestroyTexture(&term->clear_texture);
    if (term->terminal_buffer.id != 0) KTerm_DestroyBuffer(&term->terminal_buffer);
    if (term->compute_pipeline.id != 0) KTerm_DestroyPipeline(&term->compute_pipeline);
    if (term->texture_blit_pipeline.id != 0) KTerm_DestroyPipeline(&term->texture_blit_pipeline);

    if (term->gpu_staging_buffer) {
        free(term->gpu_staging_buffer);
        term->gpu_staging_buffer = NULL;
    }

    // Free session buffers
    for (int i = 0; i < MAX_SESSIONS; i++) {
        KTermSession* session = &term->sessions[i];
        if (session->screen_buffer) {
            free(session->screen_buffer);
            session->screen_buffer = NULL;
        }
        if (session->alt_buffer) {
            free(session->alt_buffer);
            session->alt_buffer = NULL;
        }

        if (session->tab_stops.stops) {
            free(session->tab_stops.stops);
            session->tab_stops.stops = NULL;
        }

        // Free Kitty Graphics resources per session
        if (session->kitty.images) {
            for (int k = 0; k < session->kitty.image_count; k++) {
                if (session->kitty.images[k].frames) {
                    for (int f = 0; f < session->kitty.images[k].frame_count; f++) {
                        if (session->kitty.images[k].frames[f].data) {
                            free(session->kitty.images[k].frames[f].data);
                        }
                        if (session->kitty.images[k].frames[f].texture.id != 0) {
                            KTerm_DestroyTexture(&session->kitty.images[k].frames[f].texture);
                        }
                    }
                    free(session->kitty.images[k].frames);
                }
            }
            free(session->kitty.images);
            session->kitty.images = NULL;
        }
        session->kitty.current_memory_usage = 0;
        session->kitty.active_upload = NULL;
    }

    // Free Vector Engine resources
    if (term->vector_buffer.id != 0) KTerm_DestroyBuffer(&term->vector_buffer);
    if (term->vector_pipeline.id != 0) KTerm_DestroyPipeline(&term->vector_pipeline);
    if (term->vector_staging_buffer) {
        free(term->vector_staging_buffer);
        term->vector_staging_buffer = NULL;
    }

    // Free memory for programmable key sequences
    for (size_t i = 0; i < GET_SESSION(term)->programmable_keys.count; i++) {
        if (GET_SESSION(term)->programmable_keys.keys[i].sequence) {
            free(GET_SESSION(term)->programmable_keys.keys[i].sequence);
            GET_SESSION(term)->programmable_keys.keys[i].sequence = NULL;
        }
    }
    if (GET_SESSION(term)->programmable_keys.keys) {
        free(GET_SESSION(term)->programmable_keys.keys);
        GET_SESSION(term)->programmable_keys.keys = NULL;
    }
    GET_SESSION(term)->programmable_keys.count = 0;
    GET_SESSION(term)->programmable_keys.capacity = 0;

    // Free Sixel graphics data buffer
    if (GET_SESSION(term)->sixel.data) {
        free(GET_SESSION(term)->sixel.data);
        GET_SESSION(term)->sixel.data = NULL;
    }
    // Note: active_upload points to one of the images or is NULL, so no separate free needed unless we support partials differently

    // Free bracketed paste buffer
    if (GET_SESSION(term)->bracketed_paste.buffer) {
        free(GET_SESSION(term)->bracketed_paste.buffer);
        GET_SESSION(term)->bracketed_paste.buffer = NULL;
    }

    // Free ReGIS Macros
    for (int i = 0; i < 26; i++) {
        if (term->regis.macros[i]) {
            free(term->regis.macros[i]);
            term->regis.macros[i] = NULL;
        }
    }
    if (term->regis.macro_buffer) {
        free(term->regis.macro_buffer);
        term->regis.macro_buffer = NULL;
    }

    KTerm_ClearEvents(term); // Ensure input pipeline is empty and reset
}

bool KTerm_InitDisplay(KTerm* term) {
    (void)term;
    // Create a virtual display for the terminal
    int vd_id;
    if (KTerm_CreateVirtualDisplay((KTermVector2){{(float)DEFAULT_WINDOW_WIDTH, (float)DEFAULT_WINDOW_HEIGHT}}, 1.0, 0, KTERM_SCALING_INTEGER, KTERM_BLEND_ALPHA, &vd_id) != KTERM_SUCCESS) {
        return false;
    }

    // Set the virtual display as renderable
    // SituationSetVirtualDisplayActive(1, true); // Not part of current API

    return true;
}

/*// Response callback to handle all terminal output (keyboard, mouse, focus events, DSR)
static void HandleKTermResponse(const char* response, int length) {
    // Print response for debugging
    printf("KTerm response: ");
    for (int i = 0; i < length; i++) {
        printf("0x%02X ", (unsigned char)response[i]);
    }
    printf("\n");

    // Echo input back to terminal (simulates PTY)
    KTerm_WriteString(term, response);
}
int main(void) {
    // Initialize Platform window
    KTermInitInfo init_info = { .window_width = DEFAULT_WINDOW_WIDTH, .window_height = DEFAULT_WINDOW_HEIGHT, .window_title = "Terminal", .initial_active_window_flags = KTERM_WINDOW_STATE_RESIZABLE };
    KTerm_Platform_Init(0, NULL, &init_info);
    KTerm_SetTargetFPS(60);

    // Initialize terminal state
    KTerm_Init(term);

    // Set response callback
    KTerm_SetResponseCallback(term, HandleKTermResponse);

    // Configure initial settings
    KTerm_EnableDebug(term, true); // Enable diagnostics
    KTerm_SetPipelineTargetFPS(term, 60); // Match pipeline to FPS

    // Send initial text
    KTerm_WriteString(term, "Welcome to K-Term!\n");
    KTerm_WriteString(term, "\x1B[32mGreen text\x1B[0m | \x1B[1mBold text\x1B[0m\n");
    KTerm_WriteString(term, "Type to interact. Try mouse modes:\n");
    KTerm_WriteString(term, "- \x1B[?1000h: VT200\n");
    KTerm_WriteString(term, "- \x1B[?1006h: SGR\n");
    KTerm_WriteString(term, "- \x1B[?1015h: URXVT\n");
    KTerm_WriteString(term, "- \x1B[?1016h: Pixel\n");
    KTerm_WriteString(term, "Focus window for \x1B[?1004h events.\n");

    // Enable mouse features
    KTerm_WriteString(term, "\x1B[?1004h"); // Focus In/Out
    KTerm_WriteString(term, "\x1B[?1000h"); // VT200
    KTerm_WriteString(term, "\x1B[?1006h"); // SGR

    while (!WindowShouldClose()) {
        // Update and render terminal (all input reported via HandleKTermResponse)
        KTerm_Update(term);

        // Render frame
        KTerm_BeginFrame();
            DrawFPS(10, 10); // Optional GUI element
        KTerm_EndFrame();
    }

    // Cleanup resources
    KTerm_Cleanup(term);
    KTerm_Platform_Shutdown();

    return 0;
}
*/


bool KTerm_InitSession(KTerm* term, int index) {
    KTermSession* session = &term->sessions[index];

    session->last_cursor_y = -1;

    // Initialize dimensions if not already set (defaults)
    if (session->cols == 0) session->cols = (term->width > 0) ? term->width : DEFAULT_TERM_WIDTH;
    if (session->rows == 0) session->rows = (term->height > 0) ? term->height : DEFAULT_TERM_HEIGHT;

    // Initialize session defaults
    EnhancedTermChar default_char = {
        .ch = ' ',
        .fg_color = {.color_mode = 0, .value.index = COLOR_WHITE},
        .bg_color = {.color_mode = 0, .value.index = COLOR_BLACK},
        .flags = KTERM_FLAG_DIRTY
    };

    // Initialize Ring Buffer
    // Primary buffer includes scrollback
    session->buffer_height = session->rows + MAX_SCROLLBACK_LINES;
    session->screen_head = 0;
    session->alt_screen_head = 0;
    session->view_offset = 0;
    session->saved_view_offset = 0;

    if (session->screen_buffer) free(session->screen_buffer);
    session->screen_buffer = (EnhancedTermChar*)calloc(session->buffer_height * session->cols, sizeof(EnhancedTermChar));
    if (!session->screen_buffer) return false;

    if (session->alt_buffer) free(session->alt_buffer);
    // Alt buffer is typically fixed size (no scrollback)
    session->alt_buffer = (EnhancedTermChar*)calloc(session->rows * session->cols, sizeof(EnhancedTermChar));
    if (!session->alt_buffer) {
        free(session->screen_buffer);
        session->screen_buffer = NULL;
        return false;
    }

    // Fill with default char
    for (int i = 0; i < session->buffer_height * session->cols; i++) {
        session->screen_buffer[i] = default_char;
    }

    // Alt buffer is smaller
    for (int i = 0; i < session->rows * session->cols; i++) {
        session->alt_buffer[i] = default_char;
    }

    // Initialize dirty rows for viewport
    if (session->row_dirty) free(session->row_dirty);
    session->row_dirty = (bool*)calloc(session->rows, sizeof(bool));
    if (!session->row_dirty) {
        free(session->screen_buffer);
        session->screen_buffer = NULL;
        free(session->alt_buffer);
        session->alt_buffer = NULL;
        return false;
    }
    for (int y = 0; y < session->rows; y++) {
        session->row_dirty[y] = true;
    }

    session->selection.active = false;
    session->selection.dragging = false;
    session->selection.start_x = -1;
    session->selection.start_y = -1;
    session->selection.end_x = -1;
    session->selection.end_y = -1;

    // Initialize mouse state
    session->mouse.enabled = true;
    session->mouse.mode = MOUSE_TRACKING_OFF;
    session->mouse.buttons[0] = session->mouse.buttons[1] = session->mouse.buttons[2] = false;
    session->mouse.last_x = session->mouse.last_y = 0;
    session->mouse.last_pixel_x = session->mouse.last_pixel_y = 0;
    session->mouse.focused = false;
    session->mouse.focus_tracking = false;
    session->mouse.sgr_mode = false;
    session->mouse.cursor_x = -1;
    session->mouse.cursor_y = -1;

    session->cursor.visible = true;
    session->cursor.blink_enabled = true;
    session->cursor.blink_state = true;
    session->cursor.blink_timer = 0.0f;
    session->cursor.x = session->cursor.y = 0;
    session->cursor.color.color_mode = 0;
    session->cursor.color.value.index = 7; // White
    session->cursor.shape = CURSOR_BLOCK;

    session->text_blink_state = 1; // Default ON
    session->text_blink_timer = 0.0f;
    session->fast_blink_rate = 30; // Slot 30 = 249.7ms (~4Hz)
    session->slow_blink_rate = 35; // Slot 35 = 558.5ms (~1.8Hz)
    session->bg_blink_rate = 35;   // Slot 35 = 558.5ms (~1.8Hz)
    session->visual_bell_timer = 0.0f;
    session->response_length = 0;
    session->response_enabled = true;
    session->parse_state = VT_PARSE_NORMAL;
    session->left_margin = 0;
    session->right_margin = term->width - 1;
    session->scroll_top = 0;
    session->scroll_bottom = term->height - 1;

    session->dec_modes.application_cursor_keys = false;
    session->dec_modes.origin_mode = false;
    session->dec_modes.auto_wrap_mode = true;
    session->dec_modes.cursor_visible = true;
    session->dec_modes.alternate_screen = false;
    session->dec_modes.insert_mode = false;
    session->dec_modes.new_line_mode = false;
    session->dec_modes.column_mode_132 = false;
    session->dec_modes.local_echo = false;
    session->dec_modes.vt52_mode = false;
    session->dec_modes.backarrow_key_mode = true; // Default to BS (match input.backarrow_sends_bs)
    session->dec_modes.sixel_scrolling_mode = false; // Default: Scrolling Enabled
    session->dec_modes.edit_mode = false;
    session->dec_modes.sixel_cursor_mode = false;
    session->dec_modes.checksum_reporting = true; // Default: Enabled
    session->dec_modes.print_form_feed = false;
    session->dec_modes.print_extent = false;
    session->dec_modes.allow_80_132_mode = false;
    session->dec_modes.alt_cursor_save = false;

    session->ansi_modes.insert_replace = false;
    session->ansi_modes.line_feed_new_line = true;

    session->soft_font.active = false;
    session->soft_font.dirty = false;
    session->soft_font.char_width = 8;
    session->soft_font.char_height = 16;

    // Grid defaults
    session->grid_enabled = false;
    session->grid_color = (RGB_KTermColor){255, 255, 255, 255}; // Default White
    session->conceal_char_code = 0;

    // Reset attributes manually as KTerm_ResetAllAttributes depends on (*GET_SESSION(term))
    session->current_fg.color_mode = 0; session->current_fg.value.index = COLOR_WHITE;
    session->current_bg.color_mode = 0; session->current_bg.value.index = COLOR_BLACK;
    session->current_ul_color.color_mode = 2; // Default/Inherit
    session->current_st_color.color_mode = 2; // Default/Inherit
    session->current_attributes = 0;

    session->bracketed_paste.enabled = false;
    session->bracketed_paste.active = false;
    session->bracketed_paste.buffer = NULL;

    session->programmable_keys.keys = NULL;
    session->programmable_keys.count = 0;
    session->programmable_keys.capacity = 0;

    snprintf(session->title.terminal_name, sizeof(session->title.terminal_name), "Session %d", index + 1);
    snprintf(session->title.window_title, sizeof(session->title.window_title), "KTerm Session %d", index + 1);
    snprintf(session->title.icon_title, sizeof(session->title.icon_title), "Term %d", index + 1);

    session->input_pipeline_length = 0; // Fix: was missing, implicitly 0
    session->pipeline_head = 0;
    session->pipeline_tail = 0;
    session->pipeline_count = 0;
    session->pipeline_overflow = false;

    session->VTperformance.chars_per_frame = 200;
    session->VTperformance.target_frame_time = 1.0 / 60.0;
    session->VTperformance.time_budget = session->VTperformance.target_frame_time * 0.5;
    session->VTperformance.avg_process_time = 0.000001;
    session->VTperformance.burst_mode = false;
    session->VTperformance.burst_threshold = 8192; // Approx half of 16384
    session->VTperformance.adaptive_processing = true;

    session->parse_state = VT_PARSE_NORMAL;
    session->escape_pos = 0;
    session->param_count = 0;

    session->options.conformance_checking = true;
    session->options.vttest_mode = false;
    session->options.debug_sequences = false;
    session->options.log_unsupported = true;

    session->session_open = true;
    session->echo_enabled = true;
    session->input_enabled = true;
    session->password_mode = false;
    session->raw_mode = false;
    session->paused = false;

    session->printer_available = false;
    session->auto_print_enabled = false;
    session->printer_controller_enabled = false;
    session->locator_events.report_button_down = false;
    session->locator_events.report_button_up = false;
    session->locator_events.report_on_request_only = true;
    session->locator_enabled = false;
    session->programmable_keys.udk_locked = false;

    session->macro_space.used = 0;
    session->macro_space.total = 4096;

    session->printer_buf_len = 0;
    memset(session->printer_buffer, 0, sizeof(session->printer_buffer));

    // Initial answerback (will be updated by SetLevel)
    session->answerback_buffer[0] = '\0';

    // Init charsets, tabs, keyboard
    // We can reuse the helper functions if they operate on (*GET_SESSION(term)) and we switch context
    return true;
}

void KTerm_SetResponseEnabled(KTerm* term, int session_index, bool enable) {
    if (session_index >= 0 && session_index < MAX_SESSIONS) {
        term->sessions[session_index].response_enabled = enable;
    }
}

void KTerm_SetActiveSession(KTerm* term, int index) {
    if (index >= 0 && index < MAX_SESSIONS) {
        // Only switch if actually changing
        if (term->active_session != index) {
            term->active_session = index;
            term->pending_session_switch = index;

            // Force redraw of the newly active session
            KTermSession* new_session = &term->sessions[index];
            for(int y = 0; y < term->height; y++) {
                if (y < new_session->rows) {
                    new_session->row_dirty[y] = true;
                }
            }

            // Flag font atlas as dirty to ensure correct font is loaded for this session
            term->font_atlas_dirty = true;

            // Update window title immediately
            if (term->title_callback) {
                term->title_callback(term, new_session->title.window_title, false);
            }
            KTerm_SetWindowTitlePlatform(new_session->title.window_title);
        }
    }
}


void KTerm_SetSplitScreen(KTerm* term, bool active, int row, int top_idx, int bot_idx) {
    term->split_screen_active = active;
    if (active) {
        term->split_row = row;
        if (top_idx >= 0 && top_idx < MAX_SESSIONS) term->session_top = top_idx;
        if (bot_idx >= 0 && bot_idx < MAX_SESSIONS) term->session_bottom = bot_idx;

        // Invalidate both sessions to force redraw
        for(int y=0; y<term->height; y++) {
            term->sessions[term->session_top].row_dirty[y] = true;
            term->sessions[term->session_bottom].row_dirty[y] = true;
        }
    } else {
        // Invalidate active session
         for(int y=0; y<term->height; y++) {
            term->sessions[term->active_session].row_dirty[y] = true;
        }
    }
}


void KTerm_WriteCharToSession(KTerm* term, int session_index, unsigned char ch) {
    if (session_index >= 0 && session_index < MAX_SESSIONS) {
        KTerm_WriteCharToSessionInternal(term, &term->sessions[session_index], ch);
    }
}

// Helper to resize a specific session
static void KTerm_ResizeSession(KTerm* term, int session_index, int cols, int rows) {
    if (session_index < 0 || session_index >= MAX_SESSIONS) return;
    KTermSession* session = &term->sessions[session_index];

    // Only resize if dimensions changed
    if (session->cols == cols && session->rows == rows) return;

    int old_cols = session->cols;
    int old_rows = session->rows;

    // Calculate new dimensions
    int new_buffer_height = rows + MAX_SCROLLBACK_LINES;

    // --- Screen Buffer Resize & Content Preservation (Viewport) ---
    EnhancedTermChar* new_screen_buffer = (EnhancedTermChar*)calloc(new_buffer_height * cols, sizeof(EnhancedTermChar));
    if (!new_screen_buffer) return;

    // Allocate new aux buffers before committing changes to avoid partial failure
    bool* new_row_dirty = (bool*)calloc(rows, sizeof(bool));
    if (!new_row_dirty) {
        free(new_screen_buffer);
        return;
    }

    EnhancedTermChar* new_alt_buffer = (EnhancedTermChar*)calloc(rows * cols, sizeof(EnhancedTermChar));
    if (!new_alt_buffer) {
        free(new_screen_buffer);
        free(new_row_dirty);
        return;
    }

    // Default char for initialization
    EnhancedTermChar default_char = {
        .ch = ' ',
        .fg_color = {.color_mode = 0, .value.index = COLOR_WHITE},
        .bg_color = {.color_mode = 0, .value.index = COLOR_BLACK},
        .flags = KTERM_FLAG_DIRTY
    };

    // Initialize new buffer
    for (int k = 0; k < new_buffer_height * cols; k++) new_screen_buffer[k] = default_char;

    // Copy visible viewport AND history from old buffer
    int copy_rows = (old_rows < rows) ? old_rows : rows;
    int copy_cols = (old_cols < cols) ? old_cols : cols;

    // Iterate from oldest history line to bottom of visible viewport
    for (int y = -MAX_SCROLLBACK_LINES; y < copy_rows; y++) {
        // Get pointer to source row in ring buffer (relative to active head)
        // GetActiveScreenRow handles wrapping and negative indices correctly
        EnhancedTermChar* src_row_ptr = GetActiveScreenRow(session, y);

        // Calculate Destination Row Index in new linear buffer (Head=0)
        // Viewport (y>=0) -> [0, copy_rows-1]
        // History (y<0)   -> [new_buffer_height + y]
        int dst_row_idx;
        if (y >= 0) dst_row_idx = y;
        else dst_row_idx = new_buffer_height + y;

        // Safety check
        if (dst_row_idx >= 0 && dst_row_idx < new_buffer_height) {
            EnhancedTermChar* dst_row_ptr = &new_screen_buffer[dst_row_idx * cols];

            // Copy row content (up to min width)
            for (int x = 0; x < copy_cols; x++) {
                dst_row_ptr[x] = src_row_ptr[x];
                dst_row_ptr[x].flags |= KTERM_FLAG_DIRTY; // Force redraw
            }
        }
    }

    // Commit changes
    if (session->screen_buffer) free(session->screen_buffer);
    session->screen_buffer = new_screen_buffer;

    if (session->row_dirty) free(session->row_dirty);
    session->row_dirty = new_row_dirty;
    for (int r = 0; r < rows; r++) session->row_dirty[r] = true;

    if (session->alt_buffer) free(session->alt_buffer);
    session->alt_buffer = new_alt_buffer;
    for (int k = 0; k < rows * cols; k++) session->alt_buffer[k] = default_char;
    session->alt_screen_head = 0;

    session->cols = cols;
    session->rows = rows;
    session->buffer_height = new_buffer_height;

    // Reset ring buffer state
    session->screen_head = 0;
    session->view_offset = 0;
    session->saved_view_offset = 0;

    // Clamp cursor
    if (session->cursor.x >= cols) session->cursor.x = cols - 1;
    if (session->cursor.y >= rows) session->cursor.y = rows - 1;

    // Reset margins
    session->left_margin = 0;
    session->right_margin = cols - 1;
    session->scroll_top = 0;
    session->scroll_bottom = rows - 1;

    // --- Tab Stops Resize ---
    if (cols > session->tab_stops.capacity) {
        int old_capacity = session->tab_stops.capacity;
        int new_capacity = cols;
        bool* new_stops = (bool*)realloc(session->tab_stops.stops, new_capacity * sizeof(bool));
        if (new_stops) {
            session->tab_stops.stops = new_stops;
            session->tab_stops.capacity = new_capacity;

            // Initialize new space with default tab stops
            for (int i = old_capacity; i < new_capacity; i++) {
                if ((i % session->tab_stops.default_width) == 0 && i != 0) {
                     session->tab_stops.stops[i] = true;
                     session->tab_stops.count++;
                } else {
                     session->tab_stops.stops[i] = false;
                }
            }
        }
    }

    if (term->session_resize_callback) {
        term->session_resize_callback(term, session_index, cols, rows);
    }
}

// Recursive Layout Calculation
static void KTerm_RecalculateLayout(KTerm* term, KTermPane* pane, int x, int y, int w, int h) {
    if (!pane) return;

    pane->x = x;
    pane->y = y;
    pane->width = w;
    pane->height = h;

    if (pane->type == PANE_LEAF) {
        if (pane->session_index >= 0) {
            KTerm_ResizeSession(term, pane->session_index, w, h);
        }
    } else {
        int size_a, size_b;

        if (pane->type == PANE_SPLIT_HORIZONTAL) {
            // Split Horizontally (Left/Right)
            size_a = (int)(w * pane->split_ratio);
            size_b = w - size_a;

            // Recurse
            KTerm_RecalculateLayout(term, pane->child_a, x, y, size_a, h);
            KTerm_RecalculateLayout(term, pane->child_b, x + size_a, y, size_b, h);
        } else {
            // Split Vertically (Top/Bottom)
            size_a = (int)(h * pane->split_ratio);
            size_b = h - size_a;

            // Recurse
            KTerm_RecalculateLayout(term, pane->child_a, x, y, w, size_a);
            KTerm_RecalculateLayout(term, pane->child_b, x, y + size_a, w, size_b);
        }
    }
}

KTermPane* KTerm_SplitPane(KTerm* term, KTermPane* target_pane, KTermPaneType split_type, float ratio) {
    if (!target_pane || target_pane->type != PANE_LEAF) return NULL;
    if (split_type == PANE_LEAF) return NULL;

    // Find a free session for the new pane
    int new_session_idx = -1;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!term->sessions[i].session_open) {
             new_session_idx = i;
             break;
        }
    }

    if (new_session_idx == -1) return NULL;

    // Create new child panes
    KTermPane* child_a = (KTermPane*)calloc(1, sizeof(KTermPane));
    KTermPane* child_b = (KTermPane*)calloc(1, sizeof(KTermPane));

    if (!child_a || !child_b) {
        if (child_a) free(child_a);
        if (child_b) free(child_b);
        return NULL;
    }

    // Configure Child A (Existing content)
    child_a->type = PANE_LEAF;
    child_a->session_index = target_pane->session_index;
    child_a->parent = target_pane;

    // Configure Child B (New content)
    child_b->type = PANE_LEAF;
    child_b->session_index = new_session_idx;
    child_b->parent = target_pane;

    // Initialize the new session
    if (!KTerm_InitSession(term, new_session_idx)) {
        free(child_a);
        free(child_b);
        return NULL;
    }

    // Convert Target Pane to Split
    target_pane->type = split_type;
    target_pane->child_a = child_a;
    target_pane->child_b = child_b;
    target_pane->split_ratio = ratio;
    target_pane->session_index = -1; // No longer a leaf

    // Trigger Layout Update
    if (term->layout_root) {
        KTerm_RecalculateLayout(term, term->layout_root, 0, 0, term->width, term->height);
    }

    return child_b;
}

// Resizes the terminal grid and window texture
// Note: This operation destroys and recreates GPU resources, so it might be slow.
void KTerm_Resize(KTerm* term, int cols, int rows) {
    if (cols < 1 || rows < 1) return;

    bool global_dim_changed = (cols != term->width || rows != term->height);

    // 1. Update Global Dimensions
    term->width = cols;
    term->height = rows;

    // 2. Resize Layout Tree (Recalculate dimensions for all panes)
    if (term->layout_root) {
        KTerm_RecalculateLayout(term, term->layout_root, 0, 0, cols, rows);
    } else {
        // Fallback for initialization or if tree is missing (should verify)
        // Resize all active sessions to full size (legacy behavior)
        for(int i=0; i<MAX_SESSIONS; i++) {
             KTerm_ResizeSession(term, i, cols, rows);
        }
    }

    // 3. Recreate GPU Resources
    if (term->compute_initialized && global_dim_changed) {
        if (term->terminal_buffer.id != 0) KTerm_DestroyBuffer(&term->terminal_buffer);
        if (term->output_texture.generation != 0) KTerm_DestroyTexture(&term->output_texture);
        // Don't free gpu_staging_buffer here, we will realloc it below

        size_t buffer_size = cols * rows * sizeof(GPUCell);
        KTerm_CreateBuffer(buffer_size, NULL, KTERM_BUFFER_USAGE_STORAGE_BUFFER | KTERM_BUFFER_USAGE_TRANSFER_DST, &term->terminal_buffer);

        int win_width = cols * term->char_width * DEFAULT_WINDOW_SCALE;
        int win_height = rows * term->char_height * DEFAULT_WINDOW_SCALE;
        KTermImage empty_img = {0};
        KTerm_CreateImage(win_width, win_height, 4, &empty_img);
        KTerm_CreateTextureEx(empty_img, false, KTERM_TEXTURE_USAGE_SAMPLED | KTERM_TEXTURE_USAGE_STORAGE | KTERM_TEXTURE_USAGE_TRANSFER_SRC, &term->output_texture);
        KTerm_UnloadImage(empty_img);

        // Optimization: Use realloc to reuse memory if possible, avoiding frequent free/alloc
        size_t new_size = cols * rows * sizeof(GPUCell);
        void* new_ptr = realloc(term->gpu_staging_buffer, new_size);
        if (new_ptr) {
            term->gpu_staging_buffer = (GPUCell*)new_ptr;
            memset(term->gpu_staging_buffer, 0, new_size);
        } else {
            if (term->gpu_staging_buffer) free(term->gpu_staging_buffer);
            term->gpu_staging_buffer = NULL;
        }

        // Resize scratch buffer
        void* new_scratch = realloc(term->row_scratch_buffer, cols * sizeof(EnhancedTermChar));
        if (new_scratch) {
            term->row_scratch_buffer = (EnhancedTermChar*)new_scratch;
        } else {
            if (term->row_scratch_buffer) free(term->row_scratch_buffer);
            term->row_scratch_buffer = NULL;
        }

        if (term->vector_layer_texture.generation != 0) KTerm_DestroyTexture(&term->vector_layer_texture);
        // Note: Vector layer content is lost on resize. Preserving it would require a texture copy,
        // but the current Situation adapter does not expose a suitable compute layout or blit function for this.
        KTermImage vec_img = {0};
        KTerm_CreateImage(win_width, win_height, 4, &vec_img);
        memset(vec_img.data, 0, win_width * win_height * 4);
        KTerm_CreateTextureEx(vec_img, false, KTERM_TEXTURE_USAGE_SAMPLED | KTERM_TEXTURE_USAGE_STORAGE | KTERM_TEXTURE_USAGE_TRANSFER_DST, &term->vector_layer_texture);
        KTerm_UnloadImage(vec_img);
    }

    // Update Split Row if needed
    if (term->split_screen_active) {
        if (term->split_row >= rows) term->split_row = rows / 2;
    } else {
        term->split_row = rows / 2;
    }
}

KTermStatus KTerm_GetStatus(KTerm* term) {
    KTermStatus status = {0};
    int head = atomic_load_explicit(&GET_SESSION(term)->pipeline_head, memory_order_relaxed);
    int tail = atomic_load_explicit(&GET_SESSION(term)->pipeline_tail, memory_order_relaxed);
    status.pipeline_usage = (head - tail + sizeof(GET_SESSION(term)->input_pipeline)) % sizeof(GET_SESSION(term)->input_pipeline);
    status.key_usage = GET_SESSION(term)->input.buffer_count;
    status.overflow_detected = atomic_load_explicit(&GET_SESSION(term)->pipeline_overflow, memory_order_relaxed);
    status.avg_process_time = GET_SESSION(term)->VTperformance.avg_process_time;
    return status;
}

void KTerm_SetKeyboardMode(KTerm* term, const char* mode, bool enable) {
    if (strcmp(mode, "application_cursor") == 0) {
        GET_SESSION(term)->dec_modes.application_cursor_keys = enable;
    } else if (strcmp(mode, "keypad_application") == 0) {
        GET_SESSION(term)->input.keypad_application_mode = enable;
    } else if (strcmp(mode, "keypad_numeric") == 0) {
        GET_SESSION(term)->input.keypad_application_mode = !enable;
    }
}

void KTerm_DefineFunctionKey(KTerm* term, int key_num, const char* sequence) {
    if (key_num >= 1 && key_num <= 24) {
        strncpy(GET_SESSION(term)->input.function_keys[key_num - 1], sequence, 31);
        GET_SESSION(term)->input.function_keys[key_num - 1][31] = '\0';
    }
}

void KTerm_QueueInputEvent(KTerm* term, KTermEvent event) {
    // Multiplexer Input Interceptor
    if (event.ctrl && event.key_code == term->mux_input.prefix_key_code) {
        term->mux_input.active = true;
        return;
    }

    if (term->mux_input.active) {
        term->mux_input.active = false; // Reset state

        KTermPane* current = term->focused_pane;
        if (!current) current = term->layout_root;

        if (event.key_code == '"') {
            // Split Vertically (Top/Bottom)
            // Logic: Split current pane
            if (current->type == PANE_LEAF) {
                KTermPane* new_pane = KTerm_SplitPane(term, current, PANE_SPLIT_VERTICAL, 0.5f);
                if (new_pane) {
                    term->focused_pane = new_pane;
                    if (new_pane->session_index >= 0) KTerm_SetActiveSession(term, new_pane->session_index);
                }
            }
        } else if (event.key_code == '%') {
            // Split Horizontally (Left/Right)
             if (current->type == PANE_LEAF) {
                KTermPane* new_pane = KTerm_SplitPane(term, current, PANE_SPLIT_HORIZONTAL, 0.5f);
                if (new_pane) {
                    term->focused_pane = new_pane;
                    if (new_pane->session_index >= 0) KTerm_SetActiveSession(term, new_pane->session_index);
                }
            }
        } else if (event.key_code == 'x') {
            // Close pane (Not fully implemented in API yet, requires merging panes)
            // KTerm_ClosePane(term, current);
        } else if (event.key_code == 'o' || event.key_code == 'n' ||
                   (event.sequence[0] == '\x1B' && (
                        event.sequence[1] == '[' || event.sequence[1] == 'O'
                   ) && (
                        event.sequence[2] == 'A' || event.sequence[2] == 'B' ||
                        event.sequence[2] == 'C' || event.sequence[2] == 'D'
                   ))) {
            // Cycle focus (Next) - Arrows also map to Next for basic cycling in Phase 5
            // DFS traversal to find next leaf
            KTermPane* stack[32];
            int top = 0;
            if (term->layout_root) stack[top++] = term->layout_root;

            bool found_current = false;
            KTermPane* next_focus = NULL;
            KTermPane* first_leaf = NULL;

            while(top > 0) {
                KTermPane* p = stack[--top];
                if (p->type == PANE_LEAF) {
                    if (!first_leaf) first_leaf = p;
                    if (found_current) {
                        next_focus = p;
                        break;
                    }
                    if (p == current) found_current = true;
                } else {
                    if (p->child_b) stack[top++] = p->child_b; // Push right first so we pop left
                    if (p->child_a) stack[top++] = p->child_a;
                }
            }
            if (!next_focus) next_focus = first_leaf; // Wrap around

            if (next_focus) {
                term->focused_pane = next_focus;
                if (next_focus->session_index >= 0) KTerm_SetActiveSession(term, next_focus->session_index);
            }
        }
        return; // Consume command
    }

    KTermSession* session;

    // Route input to the focused pane's session if available
    if (term->focused_pane && term->focused_pane->type == PANE_LEAF && term->focused_pane->session_index >= 0) {
        session = &term->sessions[term->focused_pane->session_index];
    } else {
        // Fallback to legacy active session
        session = GET_SESSION(term);
    }

    if (session->input.buffer_count < KEY_EVENT_BUFFER_SIZE) {
        session->input.buffer[session->input.buffer_head] = event;
        session->input.buffer_head = (session->input.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
        session->input.buffer_count++;
        session->input.total_events++;
    } else {
        session->input.dropped_events++;
    }
}

bool KTerm_GetKey(KTerm* term, KTermEvent* event) {
    if (!event) return false;
    // Just peek? Or remove? Traditionally GetKey removes.
    // But KTerm_Update is already consuming the buffer for processing sequences.
    // If the app calls GetKey, it probably wants to intercept events *before* they are processed?
    // Or does it want to see processed events?
    // If KTerm_Update consumes the buffer, then GetKey will likely return nothing unless called *before* Update.
    // Let's implement peek/copy behavior for inspection, or dequeue if intended for consuming.
    // Given the architecture "push to buffer -> buffer gets processed", GetKey might be for debugging or alternate handling.
    // Let's implement standard dequeue from the same buffer. If App consumes it, KTerm_Update won't see it.
    KTermSession* session = GET_SESSION(term);
    if (session->input.buffer_count == 0) return false;

    *event = session->input.buffer[session->input.buffer_tail];
    session->input.buffer_tail = (session->input.buffer_tail + 1) % KEY_EVENT_BUFFER_SIZE;
    session->input.buffer_count--;
    return true;
}

#endif // KTERM_IMPLEMENTATION


#endif // KTERM_H
