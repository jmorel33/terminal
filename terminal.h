// terminal.h - Enhanced Terminal Library Implementation v1.5
// Comprehensive VT52/VT100/VT220/VT320/VT420/xterm compatibility with modern features

/**********************************************************************************************
*
*   terminal.h - Enhanced Terminal Emulation Library
*   (c) 2025 Jacques Morel
*
*   DESCRIPTION:
*       This library provides a comprehensive terminal emulation solution, aiming for compatibility with VT52, VT100, VT220, VT320, VT420, and xterm standards,
*       while also incorporating modern features like true color support, Sixel graphics, advanced mouse tracking, and bracketed paste mode. It is designed to be
*       integrated into applications that require a text-based terminal interface, using the Situation library for rendering, input, and window management.
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
*         - Session Management: Multi-session support (up to 3 sessions) mimicking VT520.
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
*         - Full integration with the Situation library for robust resource management and windowing.
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
#ifndef TERMINAL_H
#define TERMINAL_H

#ifdef TERMINAL_TESTING
#include "mock_situation.h"
#else
#include "situation.h"
#endif

#ifdef SITUATION_IMPLEMENTATION
  #ifdef STB_TRUETYPE_IMPLEMENTATION
    #undef STB_TRUETYPE_IMPLEMENTATION
  #endif
#endif

#ifdef TERMINAL_IMPLEMENTATION
  #if !defined(SITUATION_IMPLEMENTATION)
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

// =============================================================================
// TERMINAL CONFIGURATION CONSTANTS
// =============================================================================
#define DEFAULT_TERM_WIDTH 132
#define DEFAULT_TERM_HEIGHT 50
#define DEFAULT_CHAR_WIDTH 8
#define DEFAULT_CHAR_HEIGHT 16
#define DEFAULT_WINDOW_SCALE 1 // Scale factor for the window and font rendering
#define DEFAULT_WINDOW_WIDTH (DEFAULT_TERM_WIDTH * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE)
#define MAX_SESSIONS 3
#define DEFAULT_WINDOW_HEIGHT (DEFAULT_TERM_HEIGHT * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE)
#define MAX_ESCAPE_PARAMS 32
#define MAX_COMMAND_BUFFER 512 // General purpose buffer for commands, OSC, DCS etc.
#define MAX_TAB_STOPS 256 // Max columns for tab stops, ensure it's >= DEFAULT_TERM_WIDTH
#define MAX_TITLE_LENGTH 256
#define MAX_RECT_OPERATIONS 16
#define KEY_EVENT_BUFFER_SIZE 65536
#define OUTPUT_BUFFER_SIZE 16384
#define MAX_SCROLLBACK_LINES 1000

// =============================================================================
// GLOBAL VARIABLES DECLARATIONS
// =============================================================================
// Callbacks for application to handle terminal events
// Response callback typedef
typedef void (*ResponseCallback)(const char* response, int length); // For sending data back to host
typedef void (*PrinterCallback)(const char* data, size_t length);   // For Printer Controller Mode
typedef void (*TitleCallback)(const char* title, bool is_icon);    // For GUI window title changes
typedef void (*BellCallback)(void);                                 // For audible bell
typedef void (*NotificationCallback)(const char* message);          // For sending notifications (OSC 9)

// Forward declaration
typedef struct Terminal_T Terminal;

// =============================================================================
// ENHANCED COLOR SYSTEM
// =============================================================================
typedef enum {
    COLOR_BLACK = 0, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE,
    COLOR_BRIGHT_BLACK, COLOR_BRIGHT_RED, COLOR_BRIGHT_GREEN, COLOR_BRIGHT_YELLOW,
    COLOR_BRIGHT_BLUE, COLOR_BRIGHT_MAGENTA, COLOR_BRIGHT_CYAN, COLOR_BRIGHT_WHITE
} AnsiColor; // Standard 16 ANSI colors

typedef struct RGB_Color_T {
    unsigned char r, g, b, a;
} RGB_Color; // True color representation

#ifndef TERMINAL_IMPLEMENTATION
// External declarations for users of the library (if not header-only)
extern struct Terminal_T terminal;
//extern VTKeyboard vt_keyboard;
// extern Texture2D font_texture; // Moved to struct
extern RGB_Color color_palette[256]; // Full 256 color palette
extern Color ansi_colors[16];        // Situation Color type for the 16 base ANSI colors
// extern unsigned char font_data[256 * 32]; // Defined in implementation
extern ResponseCallback response_callback;
extern TitleCallback title_callback;
extern BellCallback bell_callback;
extern NotificationCallback notification_callback;
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
    VT_LEVEL_COUNT = 14 // Update this if more levels are added
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
    PARSE_REGIS         // ReGIS graphics mode (ESC P p ... ST)
} VTParseState;

// Extended color support
typedef struct {
    int color_mode;          // 0=indexed (palette), 1=RGB
    union {
        int index;           // 0-255 palette index
        RGB_Color rgb;       // True color
    } value;
} ExtendedColor;

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
    ExtendedColor color;    // Cursor color (often inverse of cell or specific color)
} EnhancedCursor;

// =============================================================================
// TAB STOP MANAGEMENT
// =============================================================================
typedef struct {
    bool stops[MAX_TAB_STOPS]; // Array indicating tab stop presence at each column
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
// ENHANCED TERMINAL CHARACTER
// =============================================================================
typedef struct {
    unsigned int ch;             // Unicode codepoint (or ASCII/charset specific value)
    ExtendedColor fg_color;
    ExtendedColor bg_color;

    // Text attributes
    bool bold;
    bool faint;                 // DEC-specific or ECMA-48
    bool italic;
    bool underline;
    bool blink;
    bool reverse;               // Swaps fg/bg
    bool strikethrough;
    bool conceal;               // Hidden text (rarely used, renders as space)
    bool overline;              // xterm extension
    bool double_underline;      // ECMA-48
    bool double_width;          // DECDWL - character takes two cells wide
    bool double_height_top;     // DECDHL - top half of double-height char
    bool double_height_bottom;  // DECDHL - bottom half of double-height char

    // VT specific attributes
    bool protected_cell;         // DECSCA - protected from erasure (partial impl.)
    bool soft_hyphen;            // Character is a soft hyphen

    // Rendering hints
    bool dirty;                  // Cell needs redraw
    bool combining;              // Unicode combining character (affects rendering)
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
    RGB_Color palette[256];
    int parse_state; // 0=Normal, 1=Repeat, 2=Color, 3=Raster
    int param_buffer[8]; // For color definitions #Pc;Pu;Px;Py;Pz etc.
    int param_buffer_idx;
    GPUSixelStrip* strips;
    size_t strip_count;
    size_t strip_capacity;
} SixelGraphics;

#define SIXEL_STATE_NORMAL 0
#define SIXEL_STATE_REPEAT 1
#define SIXEL_STATE_COLOR  2
#define SIXEL_STATE_RASTER 3

// =============================================================================
// SOFT FONTS
// =============================================================================
typedef struct {
    unsigned char font_data[256][32]; // Storage for 256 characters, 32 bytes each (e.g., 16x16 monochrome)
    int char_width;                   // Width of characters in this font
    int char_height;                  // Height of characters in this font
    bool loaded[256];                 // Which characters in this set are loaded
    bool active;                      // Is a soft font currently selected?
    bool dirty;                       // Font data has changed and needs upload
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
    int key_code;           // Situation key code (e.g., SIT_KEY_A, KEY_F1) or char code
    bool ctrl, shift, alt, meta; // Modifier states
    bool is_repeat;         // True if this is an auto-repeated key event
    bool is_extended;       // True for extended keys (e.g., numpad vs main keys if distinct)
    KeyPriority priority;   // Priority of this event
    double timestamp;       // Time of event
    char sequence[32];      // Generated escape sequence or character(s) to send
} VTKeyEvent;

// Define KeyEvent as alias for compatibility if needed for older API parts
typedef VTKeyEvent KeyEvent;

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

    // Key mapping table (example, might not be fully used if GenerateVTSequence is comprehensive)
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
} TerminalStatus;

// =============================================================================
// TERMINAL COMPUTE SHADER & GPU STRUCTURES
// =============================================================================

typedef struct {
    uint32_t char_code;
    uint32_t fg_color;
    uint32_t bg_color;
    uint32_t flags;
} GPUCell;

typedef struct {
    float x0, y0; // Normalized Device Coordinates (0.0 - 1.0)
    float x1, y1; // Normalized Device Coordinates
    uint32_t color; // Packed RGBA
    float intensity; // 1.0 = fresh beam, < 1.0 = decaying
    uint32_t mode;   // 0=Additive, 1=Replace, 2=Erase, 3=XOR
    float padding;   // Align to 16 bytes for std430
} GPUVectorLine;

// 1. TERMINAL LOGIC BODY
#define TERMINAL_SHADER_BODY \
"\n" \
"vec4 UnpackColor(uint c) {\n" \
"    return vec4(float(c & 0xFF), float((c >> 8) & 0xFF), float((c >> 16) & 0xFF), float((c >> 24) & 0xFF)) / 255.0;\n" \
"}\n" \
"\n" \
"void main() {\n" \
"    // Bindless Accessors\n" \
"    TerminalBuffer terminal_data = TerminalBuffer(pc.terminal_buffer_addr);\n" \
"    sampler2D font_texture = sampler2D(pc.font_texture_handle);\n" \
"    sampler2D sixel_texture = sampler2D(pc.sixel_texture_handle);\n" \
"\n" \
"    uvec2 pixel_coords = gl_GlobalInvocationID.xy;\n" \
"    if (pixel_coords.x >= uint(pc.screen_size.x) || pixel_coords.y >= uint(pc.screen_size.y)) return;\n" \
"\n" \
"    vec2 uv_screen = vec2(pixel_coords) / pc.screen_size;\n" \
"\n" \
"    // CRT Curvature Effect\n" \
"    if (pc.crt_curvature > 0.0) {\n" \
"        vec2 d = abs(uv_screen - 0.5);\n" \
"        d = pow(d, vec2(2.0));\n" \
"        uv_screen -= 0.5;\n" \
"        uv_screen *= 1.0 + dot(d, d) * pc.crt_curvature;\n" \
"        uv_screen += 0.5;\n" \
"        if (uv_screen.x < 0.0 || uv_screen.x > 1.0 || uv_screen.y < 0.0 || uv_screen.y > 1.0) {\n" \
"            imageStore(output_image, ivec2(pixel_coords), vec4(0.0));\n" \
"            return;\n" \
"        }\n" \
"    }\n" \
"\n" \
"    // Sixel Overlay Sampling (using possibly distorted UV)\n" \
"    vec4 sixel_color = texture(sixel_texture, uv_screen);\n" \
"\n" \
"    // Re-calculate cell coordinates based on distorted UV or original pixel coords\n" \
"    // If CRT is on, we should sample based on distorted UV to map screen to terminal grid\n" \
"    uvec2 sample_coords = uvec2(uv_screen * pc.screen_size);\n" \
"    \n" \
"    uint cell_x = sample_coords.x / uint(pc.char_size.x);\n" \
"    uint cell_y = sample_coords.y / uint(pc.char_size.y);\n" \
"    uint row_start = cell_y * uint(pc.grid_size.x);\n" \
"\n" \
"    if (row_start >= terminal_data.cells.length()) return;\n" \
"\n" \
"    // Check line attributes from the first cell of the row\n" \
"    uint line_flags = terminal_data.cells[row_start].flags;\n" \
"    bool is_dw = (line_flags & (1 << 7)) != 0;\n" \
"    bool is_dh_top = (line_flags & (1 << 8)) != 0;\n" \
"    bool is_dh_bot = (line_flags & (1 << 9)) != 0;\n" \
"\n" \
"    uint eff_cell_x = cell_x;\n" \
"    uint in_char_x = sample_coords.x % uint(pc.char_size.x);\n" \
"    if (is_dw) {\n" \
"        eff_cell_x = cell_x / 2;\n" \
"        in_char_x = (sample_coords.x % (uint(pc.char_size.x) * 2)) / 2;\n" \
"    }\n" \
"\n" \
"    uint cell_index = row_start + eff_cell_x;\n" \
"    if (cell_index >= terminal_data.cells.length()) return;\n" \
"\n" \
"    GPUCell cell = terminal_data.cells[cell_index];\n" \
"    vec4 fg = UnpackColor(cell.fg_color);\n" \
"    vec4 bg = UnpackColor(cell.bg_color);\n" \
"    uint flags = cell.flags;\n" \
"\n" \
"    if ((flags & (1 << 5)) != 0) { vec4 t=fg; fg=bg; bg=t; }\n" \
"\n" \
"    // Mouse Selection Highlight\n" \
"    if (pc.sel_active != 0) {\n" \
"        uint s = min(pc.sel_start, pc.sel_end);\n" \
"        uint e = max(pc.sel_start, pc.sel_end);\n" \
"        if (cell_index >= s && cell_index <= e) {\n" \
"             // Invert colors for selection\n" \
"             fg = vec4(1.0) - fg;\n" \
"             bg = vec4(1.0) - bg;\n" \
"             fg.a = 1.0; bg.a = 1.0;\n" \
"        }\n" \
"    }\n" \
"\n" \
"    if (cell_index == pc.cursor_index && pc.cursor_blink_state != 0) {\n" \
"        vec4 t=fg; fg=bg; bg=t;\n" \
"    }\n" \
"\n" \
"    if (cell_index == pc.mouse_cursor_index) {\n" \
"        if (in_char_x == 0 || in_char_x == uint(pc.char_size.x) - 1 || \n" \
"            (sample_coords.y % uint(pc.char_size.y)) == 0 || \n" \
"            (sample_coords.y % uint(pc.char_size.y)) == uint(pc.char_size.y) - 1) {\n" \
"             vec4 t=fg; fg=bg; bg=t;\n" \
"        }\n" \
"    }\n" \
"\n" \
"    uint char_code = cell.char_code;\n" \
"    uint glyph_col = char_code % pc.atlas_cols;\n" \
"    uint glyph_row = char_code / pc.atlas_cols;\n" \
"    \n" \
"    uint in_char_y = sample_coords.y % uint(pc.char_size.y);\n" \
"    float u_pixel = float(in_char_x);\n" \
"    float v_pixel = float(in_char_y);\n" \
"    \n" \
"    if (is_dh_top || is_dh_bot) {\n" \
"        v_pixel = (v_pixel * 0.5) + (is_dh_bot ? (pc.char_size.y * 0.5) : 0.0);\n" \
"    }\n" \
"\n" \
"    ivec2 tex_size = textureSize(font_texture, 0);\n" \
"    vec2 uv = vec2(float(glyph_col * pc.char_size.x + u_pixel) / float(tex_size.x),\n" \
"                   float(glyph_row * pc.char_size.y + v_pixel) / float(tex_size.y));\n" \
"\n" \
"    float font_val = texture(font_texture, uv).r;\n" \
"\n" \
"    // Underline\n" \
"    if ((flags & (1 << 3)) != 0 && in_char_y == uint(pc.char_size.y) - 1) font_val = 1.0;\n" \
"    // Strike\n" \
"    if ((flags & (1 << 6)) != 0 && in_char_y == uint(pc.char_size.y) / 2) font_val = 1.0;\n" \
"\n" \
"    vec4 pixel_color = mix(bg, fg, font_val);\n" \
"\n" \
"    if ((flags & (1 << 4)) != 0 && pc.text_blink_state == 0) {\n" \
"       pixel_color = bg;\n" \
"    }\n" \
"\n" \
"    if ((flags & (1 << 10)) != 0) {\n" \
"       pixel_color = bg;\n" \
"    }\n" \
"\n" \
"    // Sixel Blend\n" \
"    pixel_color = mix(pixel_color, sixel_color, sixel_color.a);\n" \
"\n" \
"    // Vector Graphics Overlay (Storage Tube Glow)\n" \
"    if (pc.vector_texture_handle != 0) {\n" \
"        sampler2D vector_tex = sampler2D(pc.vector_texture_handle);\n" \
"        vec4 vec_col = texture(vector_tex, uv_screen);\n" \
"        // Additive blending for CRT glow effect\n" \
"        pixel_color += vec_col;\n" \
"    }\n" \
"\n" \
"    // Scanlines & Vignette (Retro Effects)\n" \
"    if (pc.scanline_intensity > 0.0) {\n" \
"        float scanline = sin(uv_screen.y * pc.screen_size.y * 3.14159);\n" \
"        pixel_color.rgb *= (1.0 - pc.scanline_intensity) + pc.scanline_intensity * (0.5 + 0.5 * scanline);\n" \
"    }\n" \
"    if (pc.crt_curvature > 0.0) {\n" \
"        vec2 d = abs(uv_screen - 0.5) * 2.0;\n" \
"        d = pow(d, vec2(2.0));\n" \
"        float vig = 1.0 - dot(d, d) * 0.1;\n" \
"        pixel_color.rgb *= vig;\n" \
"    }\n" \
"\n" \
    "    // Visual Bell Flash\n" \
    "    if (pc.visual_bell_intensity > 0.0) {\n" \
    "        pixel_color = mix(pixel_color, vec4(1.0), pc.visual_bell_intensity);\n" \
    "    }\n" \
    "\n" \
"    imageStore(output_image, ivec2(pixel_coords), pixel_color);\n" \
"}\n"

// 2. VECTOR LOGIC BODY
#define VECTOR_SHADER_BODY \
"\n" \
"vec4 UnpackColor(uint c) {\n" \
"    return vec4(float(c & 0xFF), float((c >> 8) & 0xFF), float((c >> 16) & 0xFF), float((c >> 24) & 0xFF)) / 255.0;\n" \
"}\n" \
"\n" \
"void main() {\n" \
"    uint idx = gl_GlobalInvocationID.x;\n" \
"    if (idx >= pc.vector_count) return;\n" \
"\n" \
"    // Bindless Buffer Access\n" \
"    VectorBuffer lines = VectorBuffer(pc.vector_buffer_addr);\n" \
"\n" \
"    GPUVectorLine line = lines.data[idx];\n" \
"    vec2 p0 = line.start * pc.screen_size;\n" \
"    vec2 p1 = line.end * pc.screen_size;\n" \
"    vec4 color = UnpackColor(line.color);\n" \
"    color.a *= line.intensity;\n" \
"\n" \
"    int x0 = int(p0.x); int y0 = int(p0.y);\n" \
"    int x1 = int(p1.x); int y1 = int(p1.y);\n" \
"    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;\n" \
"    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;\n" \
"    int err = dx + dy, e2;\n" \
"\n" \
"    // Bresenham Loop\n" \
"    for (;;) {\n" \
"        if (x0 >= 0 && x0 < int(pc.screen_size.x) && y0 >= 0 && y0 < int(pc.screen_size.y)) {\n" \
"            vec4 bg = imageLoad(output_image, ivec2(x0, y0));\n" \
"            vec4 result = bg;\n" \
"            if (line.mode == 0) {\n" \
"                 // Additive 'Glow' Blending\n" \
"                 result = bg + (color * color.a);\n" \
"            } else if (line.mode == 1) {\n" \
"                 // Replace\n" \
"                 result = vec4(color.rgb, 1.0);\n" \
"            } else if (line.mode == 2) {\n" \
"                 // Erase (Draw Black)\n" \
"                 result = vec4(0.0, 0.0, 0.0, 0.0);\n" \
"            } else if (line.mode == 3) {\n" \
"                 // XOR / Complement (Invert)\n" \
"                 result = vec4(1.0 - bg.rgb, 1.0);\n" \
"            }\n" \
"            imageStore(output_image, ivec2(x0, y0), result);\n" \
"        }\n" \
"        if (x0 == x1 && y0 == y1) break;\n" \
"        e2 = 2 * err;\n" \
"        if (e2 >= dy) { err += dy; x0 += sx; }\n" \
"        if (e2 <= dx) { err += dx; y0 += sy; }\n" \
"    }\n" \
"}\n"

#define SIXEL_SHADER_BODY \
"\n" \
"vec4 UnpackColor(uint c) {\n" \
"    return vec4(float(c & 0xFF), float((c >> 8) & 0xFF), float((c >> 16) & 0xFF), float((c >> 24) & 0xFF)) / 255.0;\n" \
"}\n" \
"\n" \
"void main() {\n" \
"    uint idx = gl_GlobalInvocationID.x;\n" \
"    if (idx >= pc.vector_count) return;\n" \
"\n" \
"    // Bindless Buffer Access\n" \
"    SixelBuffer strips = SixelBuffer(pc.vector_buffer_addr);\n" \
"    PaletteBuffer palette = PaletteBuffer(pc.terminal_buffer_addr);\n" \
"\n" \
"    GPUSixelStrip strip = strips.data[idx];\n" \
"    uint color_val = palette.colors[strip.color_index];\n" \
"    vec4 color = UnpackColor(color_val);\n" \
"\n" \
"    // Write 6 pixels\n" \
"    for (int i = 0; i < 6; i++) {\n" \
"        if ((strip.pattern & (1 << i)) != 0) {\n" \
"            int x = int(strip.x);\n" \
"            int y = int(strip.y) + i;\n" \
"            if (x < int(pc.screen_size.x) && y < int(pc.screen_size.y)) {\n" \
"                imageStore(output_image, ivec2(x, y), color);\n" \
"            }\n" \
"        }\n" \
"    }\n" \
"}\n"

#if defined(SITUATION_USE_VULKAN)

    // --- VULKAN DEFINITIONS ---
    // Note: GL_ARB_bindless_texture enables casting uint64_t to sampler2D
    #define TERMINAL_COMPUTE_SHADER_SRC \
    "#version 460\n" \
    "#define VULKAN_BACKEND\n" \
    "#extension GL_EXT_buffer_reference : require\n" \
    "#extension GL_EXT_scalar_block_layout : require\n" \
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n" \
    "#extension GL_ARB_bindless_texture : require\n" \
    "layout(local_size_x = 8, local_size_y = 16, local_size_z = 1) in;\n" \
    "struct GPUCell { uint char_code; uint fg_color; uint bg_color; uint flags; };\n" \
    "layout(buffer_reference, scalar) buffer TerminalBuffer { GPUCell cells[]; };\n" \
    "layout(set = 1, binding = 0, rgba8) writeonly uniform image2D output_image;\n" \
    "layout(push_constant) uniform PushConstants {\n" \
    "    vec2 screen_size;\n" \
    "    vec2 char_size;\n" \
    "    vec2 grid_size;\n" \
    "    float time;\n" \
    "    uint cursor_index;\n" \
    "    uint cursor_blink_state;\n" \
    "    uint text_blink_state;\n" \
    "    uint sel_start;\n" \
    "    uint sel_end;\n" \
    "    uint sel_active;\n" \
    "    float scanline_intensity;\n" \
    "    float crt_curvature;\n" \
    "    uint mouse_cursor_index;\n" \
    "    uint64_t terminal_buffer_addr;\n" \
    "    uint64_t vector_buffer_addr;\n" \
    "    uint64_t font_texture_handle;\n" \
    "    uint64_t sixel_texture_handle;\n" \
    "    uint64_t vector_texture_handle;\n" \
    "    uint atlas_cols;\n" \
    "    float visual_bell_intensity;\n" \
    "} pc;\n" \
    TERMINAL_SHADER_BODY

    #define VECTOR_COMPUTE_SHADER_SRC \
    "#version 460\n" \
    "#extension GL_EXT_buffer_reference : require\n" \
    "#extension GL_EXT_scalar_block_layout : require\n" \
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n" \
    "layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n" \
    "struct GPUVectorLine { vec2 start; vec2 end; uint color; float intensity; uint mode; float _pad; };\n" \
    "layout(buffer_reference, scalar) buffer VectorBuffer { GPUVectorLine data[]; };\n" \
    "layout(set = 1, binding = 0, rgba8) uniform image2D output_image;\n" \
    "layout(push_constant) uniform PushConstants {\n" \
    "    vec2 screen_size;\n" \
    "    vec2 char_size;\n" \
    "    vec2 grid_size;\n" \
    "    float time;\n" \
    "    uint cursor_index;\n" \
    "    uint cursor_blink_state;\n" \
    "    uint text_blink_state;\n" \
    "    uint sel_start;\n" \
    "    uint sel_end;\n" \
    "    uint sel_active;\n" \
    "    float scanline_intensity;\n" \
    "    float crt_curvature;\n" \
    "    uint mouse_cursor_index;\n" \
    "    uint64_t terminal_buffer_addr;\n" \
    "    uint64_t vector_buffer_addr;\n" \
    "    uint64_t font_texture_handle;\n" \
    "    uint64_t sixel_texture_handle;\n" \
    "    uint vector_count;\n" \
    "    float visual_bell_intensity;\n" \
    "} pc;\n" \
    VECTOR_SHADER_BODY

    #define SIXEL_COMPUTE_SHADER_SRC \
    "#version 460\n" \
    "#define VULKAN_BACKEND\n" \
    "#extension GL_EXT_buffer_reference : require\n" \
    "#extension GL_EXT_scalar_block_layout : require\n" \
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n" \
    "#extension GL_ARB_bindless_texture : require\n" \
    "layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n" \
    "struct GPUSixelStrip { uint x; uint y; uint pattern; uint color_index; };\n" \
    "layout(buffer_reference, scalar) buffer SixelBuffer { GPUSixelStrip data[]; };\n" \
    "layout(buffer_reference, scalar) buffer PaletteBuffer { uint colors[]; };\n" \
    "layout(set = 1, binding = 0, rgba8) uniform image2D output_image;\n" \
    "layout(push_constant) uniform PushConstants {\n" \
    "    vec2 screen_size;\n" \
    "    vec2 char_size;\n" \
    "    vec2 grid_size;\n" \
    "    float time;\n" \
    "    uint cursor_index;\n" \
    "    uint cursor_blink_state;\n" \
    "    uint text_blink_state;\n" \
    "    uint sel_start;\n" \
    "    uint sel_end;\n" \
    "    uint sel_active;\n" \
    "    float scanline_intensity;\n" \
    "    float crt_curvature;\n" \
    "    uint mouse_cursor_index;\n" \
    "    uint64_t terminal_buffer_addr;\n" \
    "    uint64_t vector_buffer_addr;\n" \
    "    uint64_t font_texture_handle;\n" \
    "    uint64_t sixel_texture_handle;\n" \
    "    uint64_t vector_texture_handle;\n" \
    "    uint atlas_cols;\n" \
    "    uint vector_count;\n" \
    "    float visual_bell_intensity;\n" \
    "} pc;\n" \
    SIXEL_SHADER_BODY

#elif defined(SITUATION_USE_OPENGL)

    // --- OPENGL DEFINITIONS ---
    #define TERMINAL_COMPUTE_SHADER_SRC \
    "#version 460\n" \
    "#extension GL_EXT_buffer_reference : require\n" \
    "#extension GL_EXT_scalar_block_layout : require\n" \
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n" \
    "#extension GL_ARB_bindless_texture : require\n" \
    "layout(local_size_x = 8, local_size_y = 16, local_size_z = 1) in;\n" \
    "struct GPUCell { uint char_code; uint fg_color; uint bg_color; uint flags; };\n" \
    "layout(buffer_reference, scalar) buffer TerminalBuffer { GPUCell cells[]; };\n" \
    "layout(binding = 1, rgba8) writeonly uniform image2D output_image;\n" \
    "layout(scalar, binding = 0) uniform PushConstants {\n" \
    "    vec2 screen_size;\n" \
    "    vec2 char_size;\n" \
    "    vec2 grid_size;\n" \
    "    float time;\n" \
    "    uint cursor_index;\n" \
    "    uint cursor_blink_state;\n" \
    "    uint text_blink_state;\n" \
    "    uint sel_start;\n" \
    "    uint sel_end;\n" \
    "    uint sel_active;\n" \
    "    float scanline_intensity;\n" \
    "    float crt_curvature;\n" \
    "    uint mouse_cursor_index;\n" \
    "    uint64_t terminal_buffer_addr;\n" \
    "    uint64_t vector_buffer_addr;\n" \
    "    uint64_t font_texture_handle;\n" \
    "    uint64_t sixel_texture_handle;\n" \
    "    uint64_t vector_texture_handle;\n" \
    "    uint atlas_cols;\n" \
    "    uint vector_count;\n" \
    "    float visual_bell_intensity;\n" \
    "} pc;\n" \
    TERMINAL_SHADER_BODY

    #define VECTOR_COMPUTE_SHADER_SRC \
    "#version 460\n" \
    "#extension GL_EXT_buffer_reference : require\n" \
    "#extension GL_EXT_scalar_block_layout : require\n" \
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n" \
    "layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n" \
    "struct GPUVectorLine { vec2 start; vec2 end; uint color; float intensity; uint mode; float _pad; };\n" \
    "layout(buffer_reference, scalar) buffer VectorBuffer { GPUVectorLine data[]; };\n" \
    "layout(binding = 1, rgba8) uniform image2D output_image;\n" \
    "layout(scalar, binding = 0) uniform PushConstants {\n" \
    "    vec2 screen_size;\n" \
    "    vec2 char_size;\n" \
    "    vec2 grid_size;\n" \
    "    float time;\n" \
    "    uint cursor_index;\n" \
    "    uint cursor_blink_state;\n" \
    "    uint text_blink_state;\n" \
    "    uint sel_start;\n" \
    "    uint sel_end;\n" \
    "    uint sel_active;\n" \
    "    float scanline_intensity;\n" \
    "    float crt_curvature;\n" \
    "    uint mouse_cursor_index;\n" \
    "    uint64_t terminal_buffer_addr;\n" \
    "    uint64_t vector_buffer_addr;\n" \
    "    uint64_t font_texture_handle;\n" \
    "    uint64_t sixel_texture_handle;\n" \
    "    uint64_t vector_texture_handle;\n" \
    "    uint atlas_cols;\n" \
    "    uint vector_count;\n" \
    "    float visual_bell_intensity;\n" \
    "} pc;\n" \
    VECTOR_SHADER_BODY

    #define SIXEL_COMPUTE_SHADER_SRC \
    "#version 460\n" \
    "#extension GL_EXT_buffer_reference : require\n" \
    "#extension GL_EXT_scalar_block_layout : require\n" \
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n" \
    "#extension GL_ARB_bindless_texture : require\n" \
    "layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n" \
    "struct GPUSixelStrip { uint x; uint y; uint pattern; uint color_index; };\n" \
    "layout(buffer_reference, scalar) buffer SixelBuffer { GPUSixelStrip data[]; };\n" \
    "layout(buffer_reference, scalar) buffer PaletteBuffer { uint colors[]; };\n" \
    "layout(binding = 1, rgba8) uniform image2D output_image;\n" \
    "layout(scalar, binding = 0) uniform PushConstants {\n" \
    "    vec2 screen_size;\n" \
    "    vec2 char_size;\n" \
    "    vec2 grid_size;\n" \
    "    float time;\n" \
    "    uint cursor_index;\n" \
    "    uint cursor_blink_state;\n" \
    "    uint text_blink_state;\n" \
    "    uint sel_start;\n" \
    "    uint sel_end;\n" \
    "    uint sel_active;\n" \
    "    float scanline_intensity;\n" \
    "    float crt_curvature;\n" \
    "    uint mouse_cursor_index;\n" \
    "    uint64_t terminal_buffer_addr;\n" \
    "    uint64_t vector_buffer_addr;\n" \
    "    uint64_t font_texture_handle;\n" \
    "    uint64_t sixel_texture_handle;\n" \
    "    uint64_t vector_texture_handle;\n" \
    "    uint atlas_cols;\n" \
    "    uint vector_count;\n" \
    "    float visual_bell_intensity;\n" \
    "} pc;\n" \
    SIXEL_SHADER_BODY

#endif

typedef struct {
    Vector2 screen_size;
    Vector2 char_size;
    Vector2 grid_size;
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
} TerminalPushConstants;

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
    ExtendedColor fg_color;
    ExtendedColor bg_color;
    bool bold_mode, faint_mode, italic_mode;
    bool underline_mode, blink_mode, reverse_mode;
    bool strikethrough_mode, conceal_mode, overline_mode;
    bool double_underline_mode;
    bool protected_mode;

    // Charset
    CharsetState charset;
} SavedCursorState;

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
    // EnhancedTermChar screen[DEFAULT_TERM_HEIGHT][DEFAULT_TERM_WIDTH];
    // EnhancedTermChar alt_screen[DEFAULT_TERM_HEIGHT][DEFAULT_TERM_WIDTH];

    bool row_dirty[DEFAULT_TERM_HEIGHT]; // Tracks dirty state of the VIEWPORT rows (0..HEIGHT-1)
    // EnhancedTermChar saved_screen[DEFAULT_TERM_HEIGHT][DEFAULT_TERM_WIDTH]; // For DECSEL/DECSED if implemented

    // Enhanced cursor
    EnhancedCursor cursor;
    SavedCursorState saved_cursor; // For DECSC/DECRC and other save/restore ops
    bool saved_cursor_valid;

    // Terminal identification & conformance
    VTConformance conformance;
    char device_attributes[128];    // Primary DA string (e.g., CSI c)
    char secondary_attributes[128]; // Secondary DA string (e.g., CSI > c)

    // Mode management
    DECModes dec_modes;
    ANSIModes ansi_modes;

    // Current character attributes for new text
    ExtendedColor current_fg;
    ExtendedColor current_bg;
    bool bold_mode, faint_mode, italic_mode;
    bool underline_mode, blink_mode, reverse_mode;
    bool strikethrough_mode, conceal_mode, overline_mode;
    bool double_underline_mode;
    bool protected_mode; // For DECSCA
    bool text_blink_state;      // Current on/off state for text blinking
    double text_blink_timer;    // Timer for text blink interval

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
    unsigned char input_pipeline[16384]; // Buffer for incoming data from host
    int input_pipeline_length; // Input pipeline length0
    int pipeline_head, pipeline_tail, pipeline_count;
    bool pipeline_overflow;

    struct {
        bool cursor_key_mode; // DECCKM mode
        bool application_mode; // Application mode
        bool keypad_mode; // Keypad application mode
        bool meta_sends_escape; // Alt sends ESC prefix
        bool delete_sends_del; // Delete sends DEL (\x7F)
        bool backarrow_sends_bs; // Backspace sends BS (\x08)
        int keyboard_dialect; // Keyboard dialect (e.g., 1 for North American ASCII)
        VTKeyEvent buffer[KEY_EVENT_BUFFER_SIZE]; // Key event buffer
        int buffer_head; // Write position
        int buffer_tail; // Read position
        int buffer_count; // Number of queued events
        int total_events; // Total events processed
        int dropped_events; // Dropped events due to overflow
        char function_keys[24][32]; // Function key sequences (F1–F24)
    } vt_keyboard; // Keyboard state

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

    // ANSI parsing state (enhanced)
    VTParseState parse_state;
    char escape_buffer[MAX_COMMAND_BUFFER]; // Buffer for parameters of ESC, CSI, OSC, DCS etc.
    int escape_pos;
    int escape_params[MAX_ESCAPE_PARAMS];   // Parsed numeric parameters for CSI
    int param_count;

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

    // Fields for terminal_console.c
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

} TerminalSession;

// Helper functions for Ring Buffer Access
static inline EnhancedTermChar* GetScreenRow(TerminalSession* session, int row) {
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

    return &session->screen_buffer[actual_index * DEFAULT_TERM_WIDTH];
}

static inline EnhancedTermChar* GetScreenCell(TerminalSession* session, int y, int x) {
    if (x < 0 || x >= DEFAULT_TERM_WIDTH) return NULL; // Basic safety
    return &GetScreenRow(session, y)[x];
}

static inline EnhancedTermChar* GetActiveScreenRow(TerminalSession* session, int row) {
    // Access logical row 'row' relative to the ACTIVE screen top (ignoring view_offset).
    // This is used for emulation commands that modify the screen state (insert, delete, scroll).

    int logical_row_idx = session->screen_head + row; // No view_offset subtraction

    // Handle wrap-around
    int actual_index = logical_row_idx % session->buffer_height;
    if (actual_index < 0) actual_index += session->buffer_height;

    return &session->screen_buffer[actual_index * DEFAULT_TERM_WIDTH];
}

static inline EnhancedTermChar* GetActiveScreenCell(TerminalSession* session, int y, int x) {
    if (x < 0 || x >= DEFAULT_TERM_WIDTH) return NULL;
    return &GetActiveScreenRow(session, y)[x];
}

typedef struct Terminal_T {
    TerminalSession sessions[MAX_SESSIONS];
    int active_session;
    int pending_session_switch; // For session switching during update loop
    bool split_screen_active;
    int split_row;
    int session_top;
    int session_bottom;
    ResponseCallback response_callback; // Response callback
    SituationComputePipeline compute_pipeline;
    SituationBuffer terminal_buffer; // SSBO
    SituationTexture output_texture; // Storage Image
    SituationTexture font_texture;   // Font Atlas
    SituationTexture sixel_texture;  // Sixel Graphics
    SituationTexture dummy_sixel_texture; // Fallback 1x1 transparent texture
    GPUCell* gpu_staging_buffer;
    bool compute_initialized;

    // Vector Engine (Tektronix)
    SituationBuffer vector_buffer;
    SituationTexture vector_layer_texture;
    SituationComputePipeline vector_pipeline;
    uint32_t vector_count;
    GPUVectorLine* vector_staging_buffer;
    size_t vector_capacity;

    // Sixel Engine (Compute Shader)
    SituationBuffer sixel_buffer;
    SituationBuffer sixel_palette_buffer;
    SituationComputePipeline sixel_pipeline;

    // Tektronix Parser State
    struct {
        int state; // 0=Alpha, 1=Graph
        int sub_state; // 0=HiY, 1=LoY, 2=HiX, 3=LoX
        int x, y; // Current beam position (0-4095)
        int holding_x, holding_y; // Holding registers
        bool pen_down; // True=Draw, False=Move
    } tektronix;

    // ReGIS Parser State
    struct {
        int state; // 0=Command, 1=Values, 2=Options, 3=Text String
        int x, y; // Current beam position (0-799, 0-479)
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
    uint16_t glyph_map[65536]; // Map Unicode BMP to Atlas Index
    uint32_t next_atlas_index;
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

    PrinterCallback printer_callback; // Callback for Printer Controller Mode
} Terminal;

// =============================================================================
// CORE API FUNCTIONS
// =============================================================================


// Session Management
void SetActiveSession(int index);
void SetSplitScreen(bool active, int row, int top_idx, int bot_idx);
void PipelineWriteCharToSession(int session_index, unsigned char ch);
void InitSession(int index);

// Terminal lifecycle
void InitTerminal(void);
void CleanupTerminal(void);
void UpdateTerminal(void);  // Process events, update states (e.g., cursor blink)
void DrawTerminal(void);    // Render the terminal state to screen

// VT compliance and identification
bool GetVTKeyEvent(VTKeyEvent* event); // Retrieve a processed key event
void SetVTLevel(VTLevel level);
VTLevel GetVTLevel(void);
// void EnableVTFeature(const char* feature, bool enable); // e.g., "sixel", "DECCKM" - Deprecated by SetVTLevel
// bool IsVTFeatureSupported(const char* feature); - Deprecated by direct struct access
void GetDeviceAttributes(char* primary, char* secondary, size_t buffer_size);

// Enhanced pipeline management (for host input)
bool PipelineWriteChar(unsigned char ch);
bool PipelineWriteString(const char* str);
bool PipelineWriteFormat(const char* format, ...);
// bool PipelineWriteUTF8(const char* utf8_str); // Requires UTF-8 decoding logic
void ProcessPipeline(void); // Process characters from the pipeline
void ClearPipeline(void);
int GetPipelineCount(void);
bool IsPipelineOverflow(void);

// Performance management
void SetPipelineTargetFPS(int fps);    // Helps tune processing budget
void SetPipelineTimeBudget(double pct); // Percentage of frame time for pipeline

// Mouse support (enhanced)
void SetMouseTracking(MouseTrackingMode mode); // Explicitly set a mouse mode
void EnableMouseFeature(const char* feature, bool enable); // e.g., "focus", "sgr"
void UpdateMouse(void); // Process mouse input from Situation and generate VT sequences

// Keyboard support (VT compatible)
void UpdateVTKeyboard(void); // Process keyboard input from Situation
void UpdateKeyboard(void);  // Alias for compatibility
bool GetKeyEvent(KeyEvent* event);  // Alias for compatibility
void SetKeyboardMode(const char* mode, bool enable); // "application_cursor", "keypad_numeric"
void DefineFunctionKey(int key_num, const char* sequence); // Program F1-F24

// Terminal control and modes
void SetTerminalMode(const char* mode, bool enable); // Generic mode setting by name
void SetCursorShape(CursorShape shape);
void SetCursorColor(ExtendedColor color);

// Character sets and encoding
void SelectCharacterSet(int gset, CharacterSet charset); // Designate G0-G3
void SetCharacterSet(CharacterSet charset); // Set current GL (usually G0)
unsigned int TranslateCharacter(unsigned char ch, CharsetState* state); // Translate based on active CS

// Tab stops
void SetTabStop(int column);
void ClearTabStop(int column);
void ClearAllTabStops(void);
int NextTabStop(int current_column);
int PreviousTabStop(int current_column); // Added for CBT

// Bracketed paste
void EnableBracketedPaste(bool enable); // Enable/disable CSI ? 2004 h/l
bool IsBracketedPasteActive(void);
void ProcessPasteData(const char* data, size_t length); // Handle pasted data

// Rectangular operations (VT420+)
void DefineRectangle(int top, int left, int bottom, int right); // (DECSERA, DECFRA, DECCRA)
void ExecuteRectangularOperation(RectOperation op, const EnhancedTermChar* fill_char);
void CopyRectangle(VTRectangle src, int dest_x, int dest_y);
void ExecuteRectangularOps(void);  // Placeholder for DECCRA
void ExecuteRectangularOps2(void); // Placeholder for DECRQCRA

// Sixel graphics
void InitSixelGraphics(void);
void ProcessSixelData(const char* data, size_t length); // Process raw Sixel string
void DrawSixelGraphics(void); // Render current Sixel image

// Soft fonts
void LoadSoftFont(const unsigned char* font_data, int char_start, int char_count); // DECDLD
void SelectSoftFont(bool enable); // Enable/disable use of loaded soft font

// Title management
void VTSituationSetWindowTitle(const char* title); // Set window title (OSC 0, OSC 2)
void SetIconTitle(const char* title);   // Set icon title (OSC 1)
const char* GetWindowTitle(void);
const char* GetIconTitle(void);

// Callbacks
void SetResponseCallback(ResponseCallback callback);
void SetPrinterCallback(PrinterCallback callback);
void SetTitleCallback(TitleCallback callback);
void SetBellCallback(BellCallback callback);
void SetNotificationCallback(NotificationCallback callback);

// Testing and diagnostics
void RunVTTest(const char* test_name); // Run predefined test sequences
void ShowTerminalInfo(void);           // Display current terminal state/info
void EnableDebugMode(bool enable);     // Toggle verbose debug logging
void LogUnsupportedSequence(const char* sequence); // Log an unsupported sequence
TerminalStatus GetTerminalStatus(void);
void ShowBufferDiagnostics(void);      // Display buffer usage info

// Screen buffer management
void VTSwapScreenBuffer(void); // Handles 1047/1049 logic

void LoadTerminalFont(const char* filepath);

// Helper to allocate a glyph index in the dynamic atlas for any Unicode codepoint
uint32_t AllocateGlyph(uint32_t codepoint);

// Internal rendering/parsing functions (potentially exposed for advanced use or testing)
void CreateFontTexture(void);

// Internal helper forward declaration
void InitTerminalCompute(void);

// Low-level char processing (called by ProcessPipeline via ProcessChar)
void ProcessChar(unsigned char ch); // Main dispatcher for character processing
void ProcessPrinterControllerChar(unsigned char ch); // Handle Printer Controller Mode
void ProcessNormalChar(unsigned char ch);
void ProcessEscapeChar(unsigned char ch);
void ProcessCSIChar(unsigned char ch);
void ProcessOSCChar(unsigned char ch);
void ProcessDCSChar(unsigned char ch);
void ProcessAPCChar(unsigned char ch);
void ProcessPMChar(unsigned char ch);
void ProcessSOSChar(unsigned char ch);
void ProcessVT52Char(unsigned char ch);
void ProcessSixelChar(unsigned char ch);
void ProcessSixelSTChar(unsigned char ch);
void ProcessControlChar(unsigned char ch);
//void ProcessStringTerminator(unsigned char ch);
void ProcessCharsetCommand(unsigned char ch);
void ProcessHashChar(unsigned char ch);
void ProcessPercentChar(unsigned char ch);


// Screen manipulation internals
void ScrollUpRegion(int top, int bottom, int lines);
void InsertLinesAt(int row, int count); // Added IL
void DeleteLinesAt(int row, int count); // Added DL
void InsertCharactersAt(int row, int col, int count); // Added ICH
void DeleteCharactersAt(int row, int col, int count); // Added DCH
void InsertCharacterAtCursor(unsigned int ch); // Handles character placement and insert mode
void ScrollDownRegion(int top, int bottom, int lines);

void ExecuteSaveCursor(void);
void ExecuteRestoreCursor(void);

// Response and parsing helpers
void QueueResponse(const char* response); // Add string to answerback_buffer
void QueueResponseBytes(const char* data, size_t len);
static void ParseGatewayCommand(const char* data, size_t len); // Gateway Protocol Parser
int ParseCSIParams(const char* params, int* out_params, int max_params); // Parses CSI parameter string into escape_params
int GetCSIParam(int index, int default_value); // Gets a parsed CSI parameter
void ExecuteCSICommand(unsigned char command);
void ExecuteOSCCommand(void);
void ExecuteDCSCommand(void);
void ExecuteAPCCommand(void);
void ExecutePMCommand(void);
void ExecuteSOSCommand(void);
void ExecuteDCSAnswerback(void);

// Cell and attribute helpers
void ClearCell(EnhancedTermChar* cell); // Clears a cell with current attributes
void ResetAllAttributes(void);          // Resets current text attributes to default

// Character set translation helpers
unsigned int TranslateDECSpecial(unsigned char ch);
unsigned int TranslateDECMultinational(unsigned char ch);

// Keyboard sequence generation helpers
void GenerateVTSequence(VTKeyEvent* event);
void HandleControlKey(VTKeyEvent* event);
void HandleAltKey(VTKeyEvent* event);

// Scripting API functions
void Script_PutChar(unsigned char ch);
void Script_Print(const char* text);
void Script_Printf(const char* format, ...);
void Script_Cls(void);
void Script_SetColor(int fg, int bg);

#ifdef TERMINAL_IMPLEMENTATION
#define ACTIVE_SESSION (terminal.sessions[terminal.active_session])


// =============================================================================
// IMPLEMENTATION BEGINS HERE
// =============================================================================

// Fixed global variable definitions
Terminal terminal = {0};
//VTKeyboard vt_keyboard = {0};   // deprecated
// RGLTexture font_texture = {0};  // Moved to terminal struct
ResponseCallback response_callback = NULL;
TitleCallback title_callback = NULL;
BellCallback bell_callback = NULL;
NotificationCallback notification_callback = NULL;

// Color mappings - Fixed initialization
RGB_Color color_palette[256];

Color ansi_colors[16] = { // Situation Color type
    {  0,   0,   0, 255}, // Black
    {170,   0,   0, 255}, // Red
    {  0, 170,   0, 255}, // Green
    {170,  85,   0, 255}, // Yellow (often brown)
    {  0,   0, 170, 255}, // Blue
    {170,   0, 170, 255}, // Magenta
    {  0, 170, 170, 255}, // Cyan
    {170, 170, 170, 255}, // White (light gray)
    { 85,  85,  85, 255}, // Bright Black (dark gray)
    {255,  85,  85, 255}, // Bright Red
    { 85, 255,  85, 255}, // Bright Green
    {255, 255,  85, 255}, // Bright Yellow
    { 85,  85, 255, 255}, // Bright Blue
    {255,  85, 255, 255}, // Bright Magenta
    { 85, 255, 255, 255}, // Bright Cyan
    {255, 255, 255, 255}  // Bright White
};

// Add missing function declaration
void InitFontData(void); // In case it's used elsewhere, though font_data is static

#include "font_data.h"


// Extended font data with larger character matrix for better rendering
/*unsigned char cp437_font__8x16[256 * 16 * 2] = {
    // 0x00 - NULL (empty)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};*/

static uint32_t charset_lut[32][128]; // Lookup table for 7-bit charset translations

void InitCharacterSetLUT(void) {
    // 1. Initialize all to ASCII identity first
    for (int s = 0; s < 32; s++) {
        for (int c = 0; c < 128; c++) {
            charset_lut[s][c] = c;
        }
    }

    // 2. DEC Special Graphics
    for (int c = 0; c < 128; c++) {
        charset_lut[CHARSET_DEC_SPECIAL][c] = TranslateDECSpecial(c);
    }

    // 3. National Replacement Character Sets (NRCS)
    // UK
    charset_lut[CHARSET_UK]['#'] = 0x00A3; // £

    // Dutch
    charset_lut[CHARSET_DUTCH]['#'] = 0x00A3; // £
    charset_lut[CHARSET_DUTCH]['@'] = 0x00BE; // ¾
    charset_lut[CHARSET_DUTCH]['['] = 0x0133; // ij (digraph) - approximated as ĳ
    charset_lut[CHARSET_DUTCH]['\\'] = 0x00BD; // ½
    charset_lut[CHARSET_DUTCH][']'] = 0x007C; // |
    charset_lut[CHARSET_DUTCH]['{'] = 0x00A8; // ¨
    charset_lut[CHARSET_DUTCH]['|'] = 0x0192; // f (florin)
    charset_lut[CHARSET_DUTCH]['}'] = 0x00BC; // ¼
    charset_lut[CHARSET_DUTCH]['~'] = 0x00B4; // ´

    // Finnish
    charset_lut[CHARSET_FINNISH]['['] = 0x00C4; // Ä
    charset_lut[CHARSET_FINNISH]['\\'] = 0x00D6; // Ö
    charset_lut[CHARSET_FINNISH][']'] = 0x00C5; // Å
    charset_lut[CHARSET_FINNISH]['^'] = 0x00DC; // Ü
    charset_lut[CHARSET_FINNISH]['`'] = 0x00E9; // é
    charset_lut[CHARSET_FINNISH]['{'] = 0x00E4; // ä
    charset_lut[CHARSET_FINNISH]['|'] = 0x00F6; // ö
    charset_lut[CHARSET_FINNISH]['}'] = 0x00E5; // å
    charset_lut[CHARSET_FINNISH]['~'] = 0x00FC; // ü

    // French
    charset_lut[CHARSET_FRENCH]['#'] = 0x00A3; // £
    charset_lut[CHARSET_FRENCH]['@'] = 0x00E0; // à
    charset_lut[CHARSET_FRENCH]['['] = 0x00B0; // °
    charset_lut[CHARSET_FRENCH]['\\'] = 0x00E7; // ç
    charset_lut[CHARSET_FRENCH][']'] = 0x00A7; // §
    charset_lut[CHARSET_FRENCH]['{'] = 0x00E9; // é
    charset_lut[CHARSET_FRENCH]['|'] = 0x00F9; // ù
    charset_lut[CHARSET_FRENCH]['}'] = 0x00E8; // è
    charset_lut[CHARSET_FRENCH]['~'] = 0x00A8; // ¨

    // French Canadian (Similar to French but with subtle differences in some standards, using VT standard map)
    charset_lut[CHARSET_FRENCH_CANADIAN]['@'] = 0x00E0; // à
    charset_lut[CHARSET_FRENCH_CANADIAN]['['] = 0x00E2; // â
    charset_lut[CHARSET_FRENCH_CANADIAN]['\\'] = 0x00E7; // ç
    charset_lut[CHARSET_FRENCH_CANADIAN][']'] = 0x00EA; // ê
    charset_lut[CHARSET_FRENCH_CANADIAN]['^'] = 0x00EE; // î
    charset_lut[CHARSET_FRENCH_CANADIAN]['`'] = 0x00F4; // ô
    charset_lut[CHARSET_FRENCH_CANADIAN]['{'] = 0x00E9; // é
    charset_lut[CHARSET_FRENCH_CANADIAN]['|'] = 0x00F9; // ù
    charset_lut[CHARSET_FRENCH_CANADIAN]['}'] = 0x00E8; // è
    charset_lut[CHARSET_FRENCH_CANADIAN]['~'] = 0x00FB; // û

    // German
    charset_lut[CHARSET_GERMAN]['@'] = 0x00A7; // §
    charset_lut[CHARSET_GERMAN]['['] = 0x00C4; // Ä
    charset_lut[CHARSET_GERMAN]['\\'] = 0x00D6; // Ö
    charset_lut[CHARSET_GERMAN][']'] = 0x00DC; // Ü
    charset_lut[CHARSET_GERMAN]['{'] = 0x00E4; // ä
    charset_lut[CHARSET_GERMAN]['|'] = 0x00F6; // ö
    charset_lut[CHARSET_GERMAN]['}'] = 0x00FC; // ü
    charset_lut[CHARSET_GERMAN]['~'] = 0x00DF; // ß

    // Italian
    charset_lut[CHARSET_ITALIAN]['#'] = 0x00A3; // £
    charset_lut[CHARSET_ITALIAN]['@'] = 0x00A7; // §
    charset_lut[CHARSET_ITALIAN]['['] = 0x00B0; // °
    charset_lut[CHARSET_ITALIAN]['\\'] = 0x00E7; // ç
    charset_lut[CHARSET_ITALIAN][']'] = 0x00E9; // é
    charset_lut[CHARSET_ITALIAN]['`'] = 0x00F9; // ù
    charset_lut[CHARSET_ITALIAN]['{'] = 0x00E0; // à
    charset_lut[CHARSET_ITALIAN]['|'] = 0x00F2; // ò
    charset_lut[CHARSET_ITALIAN]['}'] = 0x00E8; // è
    charset_lut[CHARSET_ITALIAN]['~'] = 0x00EC; // ì

    // Norwegian/Danish
    // Note: There are two variants (E and 6), usually identical.
    charset_lut[CHARSET_NORWEGIAN_DANISH]['@'] = 0x00C4; // Ä
    charset_lut[CHARSET_NORWEGIAN_DANISH]['['] = 0x00C6; // Æ
    charset_lut[CHARSET_NORWEGIAN_DANISH]['\\'] = 0x00D8; // Ø
    charset_lut[CHARSET_NORWEGIAN_DANISH][']'] = 0x00C5; // Å
    charset_lut[CHARSET_NORWEGIAN_DANISH]['^'] = 0x00DC; // Ü
    charset_lut[CHARSET_NORWEGIAN_DANISH]['`'] = 0x00E4; // ä
    charset_lut[CHARSET_NORWEGIAN_DANISH]['{'] = 0x00E6; // æ
    charset_lut[CHARSET_NORWEGIAN_DANISH]['|'] = 0x00F8; // ø
    charset_lut[CHARSET_NORWEGIAN_DANISH]['}'] = 0x00E5; // å
    charset_lut[CHARSET_NORWEGIAN_DANISH]['~'] = 0x00FC; // ü

    // Spanish
    charset_lut[CHARSET_SPANISH]['#'] = 0x00A3; // £
    charset_lut[CHARSET_SPANISH]['@'] = 0x00A7; // §
    charset_lut[CHARSET_SPANISH]['['] = 0x00A1; // ¡
    charset_lut[CHARSET_SPANISH]['\\'] = 0x00D1; // Ñ
    charset_lut[CHARSET_SPANISH][']'] = 0x00BF; // ¿
    charset_lut[CHARSET_SPANISH]['{'] = 0x00B0; // °
    charset_lut[CHARSET_SPANISH]['|'] = 0x00F1; // ñ
    charset_lut[CHARSET_SPANISH]['}'] = 0x00E7; // ç

    // Swedish
    charset_lut[CHARSET_SWEDISH]['@'] = 0x00C9; // É
    charset_lut[CHARSET_SWEDISH]['['] = 0x00C4; // Ä
    charset_lut[CHARSET_SWEDISH]['\\'] = 0x00D6; // Ö
    charset_lut[CHARSET_SWEDISH][']'] = 0x00C5; // Å
    charset_lut[CHARSET_SWEDISH]['^'] = 0x00DC; // Ü
    charset_lut[CHARSET_SWEDISH]['`'] = 0x00E9; // é
    charset_lut[CHARSET_SWEDISH]['{'] = 0x00E4; // ä
    charset_lut[CHARSET_SWEDISH]['|'] = 0x00F6; // ö
    charset_lut[CHARSET_SWEDISH]['}'] = 0x00E5; // å
    charset_lut[CHARSET_SWEDISH]['~'] = 0x00FC; // ü

    // Swiss
    charset_lut[CHARSET_SWISS]['#'] = 0x00F9; // ù
    charset_lut[CHARSET_SWISS]['@'] = 0x00E0; // à
    charset_lut[CHARSET_SWISS]['['] = 0x00E9; // é
    charset_lut[CHARSET_SWISS]['\\'] = 0x00E7; // ç
    charset_lut[CHARSET_SWISS][']'] = 0x00EA; // ê
    charset_lut[CHARSET_SWISS]['^'] = 0x00EE; // î
    charset_lut[CHARSET_SWISS]['_'] = 0x00E8; // è
    charset_lut[CHARSET_SWISS]['`'] = 0x00F4; // ô
    charset_lut[CHARSET_SWISS]['{'] = 0x00E4; // ä
    charset_lut[CHARSET_SWISS]['|'] = 0x00F6; // ö
    charset_lut[CHARSET_SWISS]['}'] = 0x00FC; // ü
    charset_lut[CHARSET_SWISS]['~'] = 0x00FB; // û
}

void InitFontData(void) {
    // This function is currently empty.
    // The font_data array is initialized statically.
    // If font_data needed dynamic initialization or loading from a file,
    // it would happen here.
}

// =============================================================================
// REST OF THE IMPLEMENTATION
// =============================================================================

void InitColorPalette(void) {
    for (int i = 0; i < 16; i++) {
        color_palette[i] = (RGB_Color){ ansi_colors[i].r, ansi_colors[i].g, ansi_colors[i].b, 255 };
    }
    int index = 16;
    for (int r = 0; r < 6; r++) {
        for (int g = 0; g < 6; g++) {
            for (int b = 0; b < 6; b++) {
                color_palette[index++] = (RGB_Color){
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
        color_palette[232 + i] = (RGB_Color){gray, gray, gray, 255};
    }
}

void InitVTConformance(void) {
    ACTIVE_SESSION.conformance.level = VT_LEVEL_XTERM;
    ACTIVE_SESSION.conformance.strict_mode = false;
    SetVTLevel(ACTIVE_SESSION.conformance.level);
    ACTIVE_SESSION.conformance.compliance.unsupported_sequences = 0;
    ACTIVE_SESSION.conformance.compliance.partial_implementations = 0;
    ACTIVE_SESSION.conformance.compliance.extensions_used = 0;
    ACTIVE_SESSION.conformance.compliance.last_unsupported[0] = '\0';
}

void InitTabStops(void) {
    memset(ACTIVE_SESSION.tab_stops.stops, false, sizeof(ACTIVE_SESSION.tab_stops.stops));
    ACTIVE_SESSION.tab_stops.count = 0;
    ACTIVE_SESSION.tab_stops.default_width = 8;
    for (int i = ACTIVE_SESSION.tab_stops.default_width; i < MAX_TAB_STOPS && i < DEFAULT_TERM_WIDTH; i += ACTIVE_SESSION.tab_stops.default_width) {
        ACTIVE_SESSION.tab_stops.stops[i] = true;
        ACTIVE_SESSION.tab_stops.count++;
    }
}

void InitCharacterSets(void) {
    ACTIVE_SESSION.charset.g0 = CHARSET_ASCII;
    ACTIVE_SESSION.charset.g1 = CHARSET_DEC_SPECIAL;
    ACTIVE_SESSION.charset.g2 = CHARSET_ASCII;
    ACTIVE_SESSION.charset.g3 = CHARSET_ASCII;
    ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g0;
    ACTIVE_SESSION.charset.gr = &ACTIVE_SESSION.charset.g1;
    ACTIVE_SESSION.charset.single_shift_2 = false;
    ACTIVE_SESSION.charset.single_shift_3 = false;
}

// Initialize VT keyboard state
// Sets up keyboard modes, function key mappings, and event buffer
void InitVTKeyboard(void) {
    // Initialize keyboard modes and flags
    ACTIVE_SESSION.vt_keyboard.application_mode = false; // Application mode off
    ACTIVE_SESSION.vt_keyboard.cursor_key_mode = ACTIVE_SESSION.dec_modes.application_cursor_keys; // Sync with DECCKM
    ACTIVE_SESSION.vt_keyboard.keypad_mode = false; // Numeric keypad mode
    ACTIVE_SESSION.vt_keyboard.meta_sends_escape = true; // Alt sends ESC prefix
    ACTIVE_SESSION.vt_keyboard.delete_sends_del = true; // Delete sends DEL (\x7F)
    ACTIVE_SESSION.vt_keyboard.backarrow_sends_bs = true; // Backspace sends BS (\x08)
    ACTIVE_SESSION.vt_keyboard.keyboard_dialect = 1; // North American ASCII

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
        strncpy(ACTIVE_SESSION.vt_keyboard.function_keys[i], function_key_sequences[i], 31);
        ACTIVE_SESSION.vt_keyboard.function_keys[i][31] = '\0';
    }

    // Initialize event buffer
    ACTIVE_SESSION.vt_keyboard.buffer_head = 0;
    ACTIVE_SESSION.vt_keyboard.buffer_tail = 0;
    ACTIVE_SESSION.vt_keyboard.buffer_count = 0;
    ACTIVE_SESSION.vt_keyboard.total_events = 0;
    ACTIVE_SESSION.vt_keyboard.dropped_events = 0;
}

void InitTerminal(void) {
    InitFontData();
    InitColorPalette();

    // Init global members
    terminal.active_session = 0;
    terminal.pending_session_switch = -1;
    terminal.split_screen_active = false;
    terminal.split_row = DEFAULT_TERM_HEIGHT / 2;
    terminal.session_top = 0;
    terminal.session_bottom = 1;
    terminal.visual_effects.curvature = 0.0f;
    terminal.visual_effects.scanline_intensity = 0.0f;

    // Init sessions
    for (int i = 0; i < MAX_SESSIONS; i++) {
        InitSession(i);

        // Context switch to use existing helper functions
        int saved = terminal.active_session;
        terminal.active_session = i;

        InitVTConformance();
        InitTabStops();
        InitCharacterSets();
        InitVTKeyboard();
        InitSixelGraphics();

        terminal.active_session = saved;
    }
    terminal.active_session = 0;

    InitCharacterSetLUT();

    // Initialize Dynamic Atlas dimensions before creation
    terminal.atlas_width = 1024;
    terminal.atlas_height = 1024;
    terminal.atlas_cols = 128;

    // Allocate LRU Cache
    size_t capacity = (terminal.atlas_width / DEFAULT_CHAR_WIDTH) * (terminal.atlas_height / DEFAULT_CHAR_HEIGHT);
    terminal.glyph_last_used = (uint64_t*)calloc(capacity, sizeof(uint64_t));
    terminal.atlas_to_codepoint = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    terminal.frame_count = 0;

    CreateFontTexture();
    InitTerminalCompute();
}



// String terminator handler for ESC P, ESC _, ESC ^, ESC X
void ProcessStringTerminator(unsigned char ch) {
    // Expects ST (ESC \) to terminate.
    // Current char `ch` is the char after ESC. So we need to see `\`
    if (ch == '\\') { // ESC \ (ST - String Terminator)
        // Execute the command that was buffered
        switch(ACTIVE_SESSION.parse_state) { // The state *before* it became PARSE_STRING_TERMINATOR
            // This logic is a bit tricky, original state should be stored temporarily if needed
            // Or, just assume it's one of DCS/OSC/APC etc. and execute its specific command.
            // For now, this state means we've seen ESC, now we see '\', so terminate.
            // The actual command execution (ExecuteDCSCommand, etc.) should happen *before*
            // setting state to PARSE_STRING_TERMINATOR, when the initial ESC P etc. is seen,
            // and data is buffered. The ST just finalizes it.
            // This function is simple: it just resets parse state.
            // The content specific execution should have happened inside the specific parser (OSC, DCS etc.) when ST is received.
            // Example: ProcessOSCChar or ProcessDCSChar would see ESC, then if next is '\', call ExecuteOSCCommand and then reset state.
            // This current structure is: state becomes PARSE_STRING_TERMINATOR, then THIS function is called with '\'.
            // This might be too simplistic.
            // Correct approach: DCS/OSC/etc. parsers buffer data. When they see an ESC that might be part of ST,
            // they might change state to PARSE_STRING_TERMINATOR. Then if this function gets '\', it means ST is complete.
            // The ExecuteXYZCommand should be called from the respective ProcessXYZChar functions upon receiving ST.

            // Given the current flow (ProcessChar -> Process[State]Char):
            // If ProcessDCSChar saw an ESC, it set state to PARSE_STRING_TERMINATOR.
            // Now ProcessChar calls this function with the char *after* that ESC (i.e. '\').
            // So, if ch == '\', the DCS string is terminated. We should call ExecuteDCSCommand().
            // This implies this function needs to know *which* string type was being parsed.
            // A temporary variable holding the "parent_parse_state" would be better.
            // For now, let's assume the specific handlers (ProcessOSCChar, ProcessDCSChar) already called their Execute function
            // upon detecting the ST sequence starting (ESC then \).
            // This function just resets state.
        }
        ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
        ACTIVE_SESSION.escape_pos = 0; // Clear buffer after command execution
    } else {
        // Not a valid ST, could be another ESC sequence.
        // Re-process 'ch' as start of new escape sequence.
        ACTIVE_SESSION.parse_state = VT_PARSE_ESCAPE; // Go to escape state
        ProcessEscapeChar(ch); // Process the char that broke ST
    }
}

void ProcessCharsetCommand(unsigned char ch) {
    ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos++] = ch;

    if (ACTIVE_SESSION.escape_pos >= 2) {
        char designator = ACTIVE_SESSION.escape_buffer[0];
        char charset_char = ACTIVE_SESSION.escape_buffer[1];
        CharacterSet selected_cs = CHARSET_ASCII;

        switch (charset_char) {
            case 'A': selected_cs = CHARSET_UK; break;
            case 'B': selected_cs = CHARSET_ASCII; break;
            case '0': selected_cs = CHARSET_DEC_SPECIAL; break;
            case '1':
            case '2':
                if (ACTIVE_SESSION.options.debug_sequences) {
                    LogUnsupportedSequence("DEC Alternate Character ROM not fully supported, using ASCII/DEC Special");
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
                if (ACTIVE_SESSION.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown charset char: %c for designator %c", charset_char, designator);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }

        switch (designator) {
            case '(': ACTIVE_SESSION.charset.g0 = selected_cs; break;
            case ')': ACTIVE_SESSION.charset.g1 = selected_cs; break;
            case '*': ACTIVE_SESSION.charset.g2 = selected_cs; break;
            case '+': ACTIVE_SESSION.charset.g3 = selected_cs; break;
        }

        ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
        ACTIVE_SESSION.escape_pos = 0;
    }
}

// Stubs for APC/PM/SOS command execution
void ExecuteAPCCommand() {
    if (ACTIVE_SESSION.options.debug_sequences) LogUnsupportedSequence("APC sequence executed (no-op)");
    // ACTIVE_SESSION.escape_buffer contains the APC string data.
}
void ExecutePMCommand() {
    if (ACTIVE_SESSION.options.debug_sequences) LogUnsupportedSequence("PM sequence executed (no-op)");
    // ACTIVE_SESSION.escape_buffer contains the PM string data.
}
void ExecuteSOSCommand() {
    if (ACTIVE_SESSION.options.debug_sequences) LogUnsupportedSequence("SOS sequence executed (no-op)");
    // ACTIVE_SESSION.escape_buffer contains the SOS string data.
}

// Generic string processor for APC, PM, SOS
void ProcessGenericStringChar(unsigned char ch, VTParseState next_state_on_escape, void (*execute_command_func)()) {
    if (ACTIVE_SESSION.escape_pos < sizeof(ACTIVE_SESSION.escape_buffer) - 1) {
        ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos++] = ch;

        if (ch == '\\' && ACTIVE_SESSION.escape_pos >= 2 && ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 2] == '\x1B') { // ST (ESC \)
            ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 2] = '\0'; // Null-terminate before ESC of ST
            if (execute_command_func) execute_command_func();
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            ACTIVE_SESSION.escape_pos = 0;
        }
        // BEL is not a standard terminator for these, ST is.
    } else { // Buffer overflow
        ACTIVE_SESSION.escape_buffer[sizeof(ACTIVE_SESSION.escape_buffer) - 1] = '\0';
        if (execute_command_func) execute_command_func();
        ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
        ACTIVE_SESSION.escape_pos = 0;
        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "String sequence (type %d) too long, truncated", (int)ACTIVE_SESSION.parse_state); // Log current state
        LogUnsupportedSequence(log_msg);
    }
}

void ProcessAPCChar(unsigned char ch) { ProcessGenericStringChar(ch, VT_PARSE_ESCAPE /* Fallback if ST is broken */, ExecuteAPCCommand); }
void ProcessPMChar(unsigned char ch) { ProcessGenericStringChar(ch, VT_PARSE_ESCAPE, ExecutePMCommand); }
void ProcessSOSChar(unsigned char ch) { ProcessGenericStringChar(ch, VT_PARSE_ESCAPE, ExecuteSOSCommand); }

// Internal helper forward declaration
static void ProcessTektronixChar(unsigned char ch);
static void ProcessReGISChar(unsigned char ch);

// Process character when in Printer Controller Mode (pass-through)
void ProcessPrinterControllerChar(unsigned char ch) {
    // We must detect the exit sequence: CSI 4 i
    // CSI can be 7-bit (\x1B [) or 8-bit (\x9B)
    // Exit sequence:
    // 7-bit: ESC [ 4 i
    // 8-bit: CSI 4 i (\x9B 4 i)

    // Ensure session-specific buffering
    // Add to buffer
    if (ACTIVE_SESSION.printer_buf_len < 7) {
        ACTIVE_SESSION.printer_buffer[ACTIVE_SESSION.printer_buf_len++] = ch;
    } else {
        // Buffer full, flush oldest and shift
        if (terminal.printer_callback) {
            terminal.printer_callback((const char*)&ACTIVE_SESSION.printer_buffer[0], 1);
        }
        memmove(ACTIVE_SESSION.printer_buffer, ACTIVE_SESSION.printer_buffer + 1, --ACTIVE_SESSION.printer_buf_len);
        ACTIVE_SESSION.printer_buffer[ACTIVE_SESSION.printer_buf_len++] = ch;
    }
    ACTIVE_SESSION.printer_buffer[ACTIVE_SESSION.printer_buf_len] = '\0';

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

    while (ACTIVE_SESSION.printer_buf_len > 0) {
        // Check 7-bit
        bool match1 = true;
        for (int i = 0; i < ACTIVE_SESSION.printer_buf_len; i++) {
            if (i >= 4 || ACTIVE_SESSION.printer_buffer[i] != (unsigned char)seq1[i]) {
                match1 = false;
                break;
            }
        }
        if (match1 && ACTIVE_SESSION.printer_buf_len == 4) {
            ACTIVE_SESSION.printer_controller_enabled = false;
            ACTIVE_SESSION.printer_buf_len = 0;
            return;
        }

        // Check 8-bit
        bool match2 = true;
        for (int i = 0; i < ACTIVE_SESSION.printer_buf_len; i++) {
            if (i >= 3 || ACTIVE_SESSION.printer_buffer[i] != (unsigned char)seq2[i]) {
                match2 = false;
                break;
            }
        }
        if (match2 && ACTIVE_SESSION.printer_buf_len == 3) {
            ACTIVE_SESSION.printer_controller_enabled = false;
            ACTIVE_SESSION.printer_buf_len = 0;
            return;
        }

        if (match1 || match2) {
            // It's a valid prefix, wait for more data
            return;
        }

        // Mismatch: Flush the first character and retry
        if (terminal.printer_callback) {
            terminal.printer_callback((const char*)&ACTIVE_SESSION.printer_buffer[0], 1);
        }
        memmove(ACTIVE_SESSION.printer_buffer, ACTIVE_SESSION.printer_buffer + 1, --ACTIVE_SESSION.printer_buf_len);
    }
}

// Continue with enhanced character processing...
void ProcessChar(unsigned char ch) {
    if (ACTIVE_SESSION.printer_controller_enabled) {
        ProcessPrinterControllerChar(ch);
        return;
    }

    switch (ACTIVE_SESSION.parse_state) {
        case VT_PARSE_NORMAL:              ProcessNormalChar(ch); break;
        case VT_PARSE_ESCAPE:              ProcessEscapeChar(ch); break;
        case PARSE_CSI:                 ProcessCSIChar(ch); break;
        case PARSE_OSC:                 ProcessOSCChar(ch); break;
        case PARSE_DCS:                 ProcessDCSChar(ch); break;
        case PARSE_SIXEL_ST:            ProcessSixelSTChar(ch); break;
        case PARSE_VT52:                ProcessVT52Char(ch); break;
        case PARSE_TEKTRONIX:           ProcessTektronixChar(ch); break;
        case PARSE_REGIS:               ProcessReGISChar(ch); break;
        case PARSE_SIXEL:               ProcessSixelChar(ch); break;
        case PARSE_CHARSET:             ProcessCharsetCommand(ch); break;
        case PARSE_HASH:                ProcessHashChar(ch); break;
        case PARSE_PERCENT:             ProcessPercentChar(ch); break;
        case PARSE_APC:                 ProcessAPCChar(ch); break;
        case PARSE_PM:                  ProcessPMChar(ch); break;
        case PARSE_SOS:                 ProcessSOSChar(ch); break;
        default:
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            ProcessNormalChar(ch);
            break;
    }
}

// CSI Pts ; Pls ; Pbs ; Prs ; Pps ; Ptd ; Pld ; Ppd $ v
void ExecuteDECCRA(void) { // Copy Rectangular Area (DECCRA)
    if (!ACTIVE_SESSION.conformance.features.rectangular_operations) {
        LogUnsupportedSequence("DECCRA requires rectangular operations support");
        return;
    }
    if (ACTIVE_SESSION.param_count != 8) {
        LogUnsupportedSequence("Invalid parameters for DECCRA");
        return;
    }
    int top = GetCSIParam(0, 1) - 1;
    int left = GetCSIParam(1, 1) - 1;
    int bottom = GetCSIParam(2, 1) - 1;
    int right = GetCSIParam(3, 1) - 1;
    // Ps4 is source page, not supported.
    int dest_top = GetCSIParam(5, 1) - 1;
    int dest_left = GetCSIParam(6, 1) - 1;
    // Ps8 is destination page, not supported.

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;
    if (top > bottom || left > right) return;

    VTRectangle rect = {top, left, bottom, right, true};
    CopyRectangle(rect, dest_left, dest_top);
}

static unsigned int CalculateRectChecksum(int top, int left, int bottom, int right) {
    unsigned int checksum = 0;
    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, y, x);
            if (cell) {
                checksum += cell->ch;
            }
        }
    }
    return checksum;
}

void ExecuteDECRQCRA(void) { // Request Rectangular Area Checksum
    if (!ACTIVE_SESSION.conformance.features.rectangular_operations) {
        LogUnsupportedSequence("DECRQCRA requires rectangular operations support");
        return;
    }

    // CSI Pid ; Pp ; Pt ; Pl ; Pb ; Pr $ w
    int pid = GetCSIParam(0, 1);
    // int page = GetCSIParam(1, 1); // Ignored
    int top = GetCSIParam(2, 1) - 1;
    int left = GetCSIParam(3, 1) - 1;
    int bottom = GetCSIParam(4, DEFAULT_TERM_HEIGHT) - 1;
    int right = GetCSIParam(5, DEFAULT_TERM_WIDTH) - 1;

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;

    unsigned int checksum = 0;
    if (top <= bottom && left <= right) {
        checksum = CalculateRectChecksum(top, left, bottom, right);
    }

    // Response: DCS Pid ! ~ Checksum ST
    char response[32];
    snprintf(response, sizeof(response), "\x1BP%d!~%04X\x1B\\", pid, checksum & 0xFFFF);
    QueueResponse(response);
}

// CSI Pch ; Pt ; Pl ; Pb ; Pr $ x
void ExecuteDECFRA(void) { // Fill Rectangular Area
    if (!ACTIVE_SESSION.conformance.features.rectangular_operations) {
        LogUnsupportedSequence("DECFRA requires rectangular operations support");
        return;
    }

    if (ACTIVE_SESSION.param_count != 5) {
        LogUnsupportedSequence("Invalid parameters for DECFRA");
        return;
    }

    int char_code = GetCSIParam(0, ' ');
    int top = GetCSIParam(1, 1) - 1;
    int left = GetCSIParam(2, 1) - 1;
    int bottom = GetCSIParam(3, 1) - 1;
    int right = GetCSIParam(4, 1) - 1;

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;
    if (top > bottom || left > right) return;

    unsigned int fill_char = (unsigned int)char_code;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, y, x);
            cell->ch = fill_char;
            cell->fg_color = ACTIVE_SESSION.current_fg;
            cell->bg_color = ACTIVE_SESSION.current_bg;
            cell->bold = ACTIVE_SESSION.bold_mode;
            cell->faint = ACTIVE_SESSION.faint_mode;
            cell->italic = ACTIVE_SESSION.italic_mode;
            cell->underline = ACTIVE_SESSION.underline_mode;
            cell->blink = ACTIVE_SESSION.blink_mode;
            cell->reverse = ACTIVE_SESSION.reverse_mode;
            cell->strikethrough = ACTIVE_SESSION.strikethrough_mode;
            cell->conceal = ACTIVE_SESSION.conceal_mode;
            cell->overline = ACTIVE_SESSION.overline_mode;
            cell->double_underline = ACTIVE_SESSION.double_underline_mode;
            cell->dirty = true;
        }
        ACTIVE_SESSION.row_dirty[y] = true;
    }
}

// CSI ? Psl {
void ExecuteDECSLE(void) { // Select Locator Events
    if (!ACTIVE_SESSION.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECSLE requires VT420 mode");
        return;
    }

    // Ensure session-specific locator events are updated
    if (ACTIVE_SESSION.param_count == 0) {
        // No parameters, defaults to 0
        ACTIVE_SESSION.locator_events.report_on_request_only = true;
        ACTIVE_SESSION.locator_events.report_button_down = false;
        ACTIVE_SESSION.locator_events.report_button_up = false;
        return;
    }

    for (int i = 0; i < ACTIVE_SESSION.param_count; i++) {
        switch (ACTIVE_SESSION.escape_params[i]) {
            case 0:
                ACTIVE_SESSION.locator_events.report_on_request_only = true;
                ACTIVE_SESSION.locator_events.report_button_down = false;
                ACTIVE_SESSION.locator_events.report_button_up = false;
                break;
            case 1:
                ACTIVE_SESSION.locator_events.report_button_down = true;
                ACTIVE_SESSION.locator_events.report_on_request_only = false;
                break;
            case 2:
                ACTIVE_SESSION.locator_events.report_button_down = false;
                break;
            case 3:
                ACTIVE_SESSION.locator_events.report_button_up = true;
                ACTIVE_SESSION.locator_events.report_on_request_only = false;
                break;
            case 4:
                ACTIVE_SESSION.locator_events.report_button_up = false;
                break;
            default:
                if (ACTIVE_SESSION.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown DECSLE parameter: %d", ACTIVE_SESSION.escape_params[i]);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
    }
}

void ExecuteDECSASD(void) {
    // CSI Ps $ }
    // Select Active Status Display
    // 0 = Main Display, 1 = Status Line
    int mode = GetCSIParam(0, 0);
    if (mode == 0 || mode == 1) {
        ACTIVE_SESSION.active_display = mode;
    }
}

void ExecuteDECSSDT(void) {
    // CSI Ps $ ~
    // Select Split Definition Type
    // 0 = No Split, 1 = Horizontal Split
    int mode = GetCSIParam(0, 0);
    if (mode == 0) {
        SetSplitScreen(false, 0, 0, 0);
    } else if (mode == 1) {
        // Default split: Center, Session 0 Top, Session 1 Bottom
        // Future: Support parameterized split points
        SetSplitScreen(true, DEFAULT_TERM_HEIGHT / 2, 0, 1);
    } else {
        if (ACTIVE_SESSION.options.debug_sequences) {
            char msg[64];
            snprintf(msg, sizeof(msg), "DECSSDT mode %d not supported", mode);
            LogUnsupportedSequence(msg);
        }
    }
}

// CSI Plc |
void ExecuteDECRQLP(void) { // Request Locator Position
    if (!ACTIVE_SESSION.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECRQLP requires VT420 mode");
        return;
    }

    char response[64]; // Increased buffer size for longer response

    if (!ACTIVE_SESSION.locator_enabled || ACTIVE_SESSION.mouse.cursor_x < 1 || ACTIVE_SESSION.mouse.cursor_y < 1) {
        // Locator not enabled or position unknown, respond with "no locator"
        snprintf(response, sizeof(response), "\x1B[0!|");
    } else {
        // Locator enabled and position is known, report position.
        // Format: CSI Pe;Pr;Pc;Pp!|
        // Pe = 1 (request response)
        // Pr = row
        // Pc = column
        // Pp = page (hardcoded to 1)
        int row = ACTIVE_SESSION.mouse.cursor_y;
        int col = ACTIVE_SESSION.mouse.cursor_x;

        if (terminal.split_screen_active && terminal.active_session == terminal.session_bottom) {
            row -= (terminal.split_row + 1);
        }

        int page = 1; // Page memory not implemented, so hardcode to 1.
        snprintf(response, sizeof(response), "\x1B[1;%d;%d;%d!|", row, col, page);
    }

    QueueResponse(response);
}


// CSI Pt ; Pl ; Pb ; Pr $ x
void ExecuteDECERA(void) { // Erase Rectangular Area
    if (!ACTIVE_SESSION.conformance.features.rectangular_operations) {
        LogUnsupportedSequence("DECERA requires rectangular operations support");
        return;
    }
    if (ACTIVE_SESSION.param_count != 4) {
        LogUnsupportedSequence("Invalid parameters for DECERA");
        return;
    }
    int top = GetCSIParam(0, 1) - 1;
    int left = GetCSIParam(1, 1) - 1;
    int bottom = GetCSIParam(2, 1) - 1;
    int right = GetCSIParam(3, 1) - 1;

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;
    if (top > bottom || left > right) return;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, y, x));
        }
        ACTIVE_SESSION.row_dirty[y] = true;
    }
}


// CSI Ps ; Pt ; Pl ; Pb ; Pr $ {
void ExecuteDECSERA(void) { // Selective Erase Rectangular Area
    if (!ACTIVE_SESSION.conformance.features.rectangular_operations) {
        LogUnsupportedSequence("DECSERA requires rectangular operations support");
        return;
    }
    if (ACTIVE_SESSION.param_count < 4 || ACTIVE_SESSION.param_count > 5) {
        LogUnsupportedSequence("Invalid parameters for DECSERA");
        return;
    }
    int erase_param, top, left, bottom, right;

    if (ACTIVE_SESSION.param_count == 5) {
        erase_param = GetCSIParam(0, 0);
        top = GetCSIParam(1, 1) - 1;
        left = GetCSIParam(2, 1) - 1;
        bottom = GetCSIParam(3, 1) - 1;
        right = GetCSIParam(4, 1) - 1;
    } else { // param_count == 4
        erase_param = 0; // Default when Ps is omitted
        top = GetCSIParam(0, 1) - 1;
        left = GetCSIParam(1, 1) - 1;
        bottom = GetCSIParam(2, 1) - 1;
        right = GetCSIParam(3, 1) - 1;
    }

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;
    if (top > bottom || left > right) return;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            bool should_erase = false;
            EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, y, x);
            switch (erase_param) {
                case 0: if (!cell->protected_cell) should_erase = true; break;
                case 1: should_erase = true; break;
                case 2: if (cell->protected_cell) should_erase = true; break;
            }
            if (should_erase) {
                ClearCell(cell);
            }
        }
        ACTIVE_SESSION.row_dirty[y] = true;
    }
}

void ProcessOSCChar(unsigned char ch) {
    if (ACTIVE_SESSION.escape_pos < sizeof(ACTIVE_SESSION.escape_buffer) - 1) {
        ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos++] = ch;

        if (ch == '\a') {
            ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 1] = '\0';
            ExecuteOSCCommand();
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            ACTIVE_SESSION.escape_pos = 0;
        } else if (ch == '\\' && ACTIVE_SESSION.escape_pos >= 2 && ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 2] == '\x1B') {
            ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 2] = '\0';
            ExecuteOSCCommand();
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            ACTIVE_SESSION.escape_pos = 0;
        }
    } else {
        ACTIVE_SESSION.escape_buffer[sizeof(ACTIVE_SESSION.escape_buffer) - 1] = '\0';
        ExecuteOSCCommand();
        ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
        ACTIVE_SESSION.escape_pos = 0;
        LogUnsupportedSequence("OSC sequence too long, truncated");
    }
}

void ProcessDCSChar(unsigned char ch) {
    if (ACTIVE_SESSION.escape_pos < sizeof(ACTIVE_SESSION.escape_buffer) - 1) {
        ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos++] = ch;

        if (ch == 'q' && ACTIVE_SESSION.conformance.features.sixel_graphics) {
            // Sixel Graphics command
            ParseCSIParams(ACTIVE_SESSION.escape_buffer, ACTIVE_SESSION.sixel.params, MAX_ESCAPE_PARAMS);
            ACTIVE_SESSION.sixel.param_count = ACTIVE_SESSION.param_count;

            ACTIVE_SESSION.sixel.pos_x = 0;
            ACTIVE_SESSION.sixel.pos_y = 0;
            ACTIVE_SESSION.sixel.max_x = 0;
            ACTIVE_SESSION.sixel.max_y = 0;
            ACTIVE_SESSION.sixel.color_index = 0;
            ACTIVE_SESSION.sixel.repeat_count = 0;

            if (!ACTIVE_SESSION.sixel.data) {
                ACTIVE_SESSION.sixel.width = DEFAULT_TERM_WIDTH * DEFAULT_CHAR_WIDTH;
                ACTIVE_SESSION.sixel.height = DEFAULT_TERM_HEIGHT * DEFAULT_CHAR_HEIGHT;
                ACTIVE_SESSION.sixel.data = calloc(ACTIVE_SESSION.sixel.width * ACTIVE_SESSION.sixel.height * 4, 1);
            }

            if (ACTIVE_SESSION.sixel.data) {
                memset(ACTIVE_SESSION.sixel.data, 0, ACTIVE_SESSION.sixel.width * ACTIVE_SESSION.sixel.height * 4);
            }

            if (!ACTIVE_SESSION.sixel.strips) {
                ACTIVE_SESSION.sixel.strip_capacity = 65536;
                ACTIVE_SESSION.sixel.strips = (GPUSixelStrip*)calloc(ACTIVE_SESSION.sixel.strip_capacity, sizeof(GPUSixelStrip));
            }
            ACTIVE_SESSION.sixel.strip_count = 0;

            ACTIVE_SESSION.sixel.active = true;
            ACTIVE_SESSION.sixel.x = ACTIVE_SESSION.cursor.x * DEFAULT_CHAR_WIDTH;
            ACTIVE_SESSION.sixel.y = ACTIVE_SESSION.cursor.y * DEFAULT_CHAR_HEIGHT;

            ACTIVE_SESSION.parse_state = PARSE_SIXEL;
            ACTIVE_SESSION.escape_pos = 0;
            return;
        }

        if (ch == 'p') {
            // ReGIS (Remote Graphics Instruction Set)
            // Initialize ReGIS state
            terminal.regis.state = 0; // Expecting command
            terminal.regis.command = 0;
            terminal.regis.x = 0;
            terminal.regis.y = 0;
            terminal.regis.color = 0xFFFFFFFF; // White
            terminal.regis.write_mode = 0; // Default to Overlay/Additive
            terminal.regis.param_count = 0;
            terminal.regis.has_comma = false;
            terminal.regis.has_bracket = false;

            ACTIVE_SESSION.parse_state = PARSE_REGIS;
            ACTIVE_SESSION.escape_pos = 0;
            return;
        }

        if (ch == '\a') { // Non-standard, but some terminals accept BEL for DCS
            ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 1] = '\0';
            ExecuteDCSCommand();
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            ACTIVE_SESSION.escape_pos = 0;
        } else if (ch == '\\' && ACTIVE_SESSION.escape_pos >= 2 && ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 2] == '\x1B') { // ST (ESC \)
            ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 2] = '\0';
            ExecuteDCSCommand();
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            ACTIVE_SESSION.escape_pos = 0;
        }
    } else { // Buffer overflow
        ACTIVE_SESSION.escape_buffer[sizeof(ACTIVE_SESSION.escape_buffer) - 1] = '\0';
        ExecuteDCSCommand();
        ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
        ACTIVE_SESSION.escape_pos = 0;
        LogUnsupportedSequence("DCS sequence too long, truncated");
    }
}

// =============================================================================
// ENHANCED FONT SYSTEM WITH UNICODE SUPPORT
// =============================================================================

void CreateFontTexture(void) {
    if (terminal.font_texture.generation != 0) {
        SituationDestroyTexture(&terminal.font_texture);
    }

    const int chars_per_row = terminal.atlas_cols > 0 ? terminal.atlas_cols : 16;
    const int num_chars_base = 256;

    // Allocate persistent CPU buffer if not present
    if (!terminal.font_atlas_pixels) {
        terminal.font_atlas_pixels = calloc(terminal.atlas_width * terminal.atlas_height * 4, 1);
        if (!terminal.font_atlas_pixels) return;
        terminal.next_atlas_index = 256; // Start dynamic allocation after base set
    }

    unsigned char* pixels = terminal.font_atlas_pixels;

    int char_w = DEFAULT_CHAR_WIDTH;
    int char_h = DEFAULT_CHAR_HEIGHT;
    if (ACTIVE_SESSION.soft_font.active) {
        char_w = ACTIVE_SESSION.soft_font.char_width;
        char_h = ACTIVE_SESSION.soft_font.char_height;
    }
    int dynamic_chars_per_row = terminal.atlas_width / char_w;

    // Unpack the font data (Base 256 chars)
    for (int i = 0; i < num_chars_base; i++) {
        int glyph_col = i % dynamic_chars_per_row;
        int glyph_row = i / dynamic_chars_per_row;
        int dest_x_start = glyph_col * char_w;
        int dest_y_start = glyph_row * char_h;

        for (int y = 0; y < char_h; y++) {
            unsigned char byte;
            if (ACTIVE_SESSION.soft_font.active && ACTIVE_SESSION.soft_font.loaded[i]) {
                byte = ACTIVE_SESSION.soft_font.font_data[i][y];
            } else {
                if (y < 16) byte = cp437_font__8x16[i * 16 + y];
                else byte = 0;
            }

            for (int x = 0; x < char_w; x++) {
                int px_idx = ((dest_y_start + y) * terminal.atlas_width + (dest_x_start + x)) * 4;
                if ((byte >> (7 - x)) & 1) {
                    pixels[px_idx + 0] = 255;
                    pixels[px_idx + 1] = 255;
                    pixels[px_idx + 2] = 255;
                    pixels[px_idx + 3] = 255;
                } else {
                    pixels[px_idx + 0] = 0;
                    pixels[px_idx + 1] = 0;
                    pixels[px_idx + 2] = 0;
                    pixels[px_idx + 3] = 0;
                }
            }
        }
    }

    // Create GPU Texture
    SituationImage img = {0};
    img.width = terminal.atlas_width;
    img.height = terminal.atlas_height;
    img.channels = 4;
    img.data = pixels;

    if (terminal.font_texture.generation != 0) SituationDestroyTexture(&terminal.font_texture);
    SituationCreateTexture(img, false, &terminal.font_texture);
    // Don't unload image data as it points to persistent buffer
}

void InitTerminalCompute(void) {
    if (terminal.compute_initialized) return;

    // 1. Create SSBO
    size_t buffer_size = DEFAULT_TERM_WIDTH * DEFAULT_TERM_HEIGHT * sizeof(GPUCell);
    SituationCreateBuffer(buffer_size, NULL, SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_TRANSFER_DST, &terminal.terminal_buffer);

    // 2. Create Storage Image (Output)
    SituationImage empty_img = {0};
    SituationCreateImage(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 4, &empty_img); // RGBA
    // We can init to black if we want, but compute will overwrite.
    SituationCreateTextureEx(empty_img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_SRC, &terminal.output_texture);
    SituationUnloadImage(empty_img);

    // 3. Create Compute Pipeline
    SituationCreateComputePipelineFromMemory(TERMINAL_COMPUTE_SHADER_SRC, SIT_COMPUTE_LAYOUT_TERMINAL, &terminal.compute_pipeline);

    // Create Dummy Sixel Texture (1x1 transparent)
    SituationImage dummy_img = {0};
    if (SituationCreateImage(1, 1, 4, &dummy_img) == SITUATION_SUCCESS) {
        memset(dummy_img.data, 0, 4); // Clear to transparent
        SituationCreateTextureEx(dummy_img, false, SITUATION_TEXTURE_USAGE_SAMPLED, &terminal.dummy_sixel_texture);
        SituationUnloadImage(dummy_img);
    }

    terminal.gpu_staging_buffer = (GPUCell*)calloc(DEFAULT_TERM_WIDTH * DEFAULT_TERM_HEIGHT, sizeof(GPUCell));

    // 4. Init Vector Engine (Storage Tube Architecture)
    terminal.vector_capacity = 65536; // Max new lines per frame
    SituationCreateBuffer(terminal.vector_capacity * sizeof(GPUVectorLine), NULL, SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_TRANSFER_DST, &terminal.vector_buffer);
    terminal.vector_staging_buffer = (GPUVectorLine*)calloc(terminal.vector_capacity, sizeof(GPUVectorLine));

    // Create Persistent Vector Layer Texture (Storage Tube Surface)
    SituationImage vec_img = {0};
    SituationCreateImage(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 4, &vec_img);
    memset(vec_img.data, 0, DEFAULT_WINDOW_WIDTH * DEFAULT_WINDOW_HEIGHT * 4); // Clear to transparent black
    SituationCreateTextureEx(vec_img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_DST, &terminal.vector_layer_texture);
    SituationUnloadImage(vec_img);

    // Create Vector Pipeline
    SituationCreateComputePipelineFromMemory(VECTOR_COMPUTE_SHADER_SRC, SIT_COMPUTE_LAYOUT_VECTOR, &terminal.vector_pipeline);

    // 5. Init Sixel Engine
    SituationCreateBuffer(65536 * sizeof(GPUSixelStrip), NULL, SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_TRANSFER_DST, &terminal.sixel_buffer);
    SituationCreateBuffer(256 * sizeof(uint32_t), NULL, SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_TRANSFER_DST, &terminal.sixel_palette_buffer);
    SituationCreateComputePipelineFromMemory(SIXEL_COMPUTE_SHADER_SRC, SIT_COMPUTE_LAYOUT_SIXEL, &terminal.sixel_pipeline);

    terminal.compute_initialized = true;
}

// =============================================================================
// CHARACTER SET TRANSLATION SYSTEM
// =============================================================================

unsigned int TranslateCharacter(unsigned char ch, CharsetState* state) {
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
            return charset_lut[active_set][seven_bit];
        }
        return ch;
    } else {
        // Low-bit characters (GL)
        if (active_set < CHARSET_COUNT) {
            return charset_lut[active_set][ch];
        }
    }

    return ch;
}

// Render a glyph from TTF or fallback
static void RenderGlyphToAtlas(uint32_t codepoint, uint32_t idx) {
    int col = idx % terminal.atlas_cols;
    int row = idx / terminal.atlas_cols;
    int x_start = col * DEFAULT_CHAR_WIDTH;
    int y_start = row * DEFAULT_CHAR_HEIGHT;

    if (terminal.ttf.loaded) {
        // TTF Rendering
        int advance, lsb, x0, y0, x1, y1;
        stbtt_GetCodepointHMetrics(&terminal.ttf.info, codepoint, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&terminal.ttf.info, codepoint, terminal.ttf.scale, terminal.ttf.scale, &x0, &y0, &x1, &y1);

        // Center horizontally
        // x0 is usually small negative/positive bearing. Width of glyph is x1-x0.
        int gw = x1 - x0;
        int x_off = (DEFAULT_CHAR_WIDTH - gw) / 2;

        // Better approach:
        int w, h, xoff, yoff;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&terminal.ttf.info, 0, terminal.ttf.scale, codepoint, &w, &h, &xoff, &yoff);

        if (bitmap) {
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    int px = x + (DEFAULT_CHAR_WIDTH - w)/2; // Simple center X
                    int py = y + terminal.ttf.baseline + yoff; // yoff is negative (distance from baseline up)

                    if (px >= 0 && px < DEFAULT_CHAR_WIDTH && py >= 0 && py < DEFAULT_CHAR_HEIGHT) {
                        int val = bitmap[y * w + x];
                        int px_idx = ((y_start + py) * terminal.atlas_width + (x_start + px)) * 4;
                        terminal.font_atlas_pixels[px_idx+0] = 255;
                        terminal.font_atlas_pixels[px_idx+1] = 255;
                        terminal.font_atlas_pixels[px_idx+2] = 255;
                        terminal.font_atlas_pixels[px_idx+3] = val; // Use alpha
                    }
                }
            }
            stbtt_FreeBitmap(bitmap, NULL);
        }
    } else {
        // Fallback: Hex Box
        for (int y = 0; y < DEFAULT_CHAR_HEIGHT; y++) {
            for (int x = 0; x < DEFAULT_CHAR_WIDTH; x++) {
                bool on = false;
                if (x == 0 || x == DEFAULT_CHAR_WIDTH-1 || y == 0 || y == DEFAULT_CHAR_HEIGHT-1) on = true;
                if (x == DEFAULT_CHAR_WIDTH/2 && y == DEFAULT_CHAR_HEIGHT/2) on = true; // Dot

                int px_idx = ((y_start + y) * terminal.atlas_width + (x_start + x)) * 4;
                unsigned char val = on ? 255 : 0;
                terminal.font_atlas_pixels[px_idx+0] = val;
                terminal.font_atlas_pixels[px_idx+1] = val;
                terminal.font_atlas_pixels[px_idx+2] = val;
                terminal.font_atlas_pixels[px_idx+3] = val;
            }
        }
    }
}

void LoadTerminalFont(const char* filepath) {
    unsigned int size;
    unsigned char* buffer = NULL;
    if (SituationLoadFileData(filepath, &size, &buffer) != SITUATION_SUCCESS || !buffer) {
        if (terminal.response_callback) terminal.response_callback("Font load failed\r\n", 18);
        return;
    }

    if (terminal.ttf.file_buffer) SIT_FREE(terminal.ttf.file_buffer);
    terminal.ttf.file_buffer = buffer;

    if (!stbtt_InitFont(&terminal.ttf.info, buffer, 0)) {
        if (terminal.response_callback) terminal.response_callback("Font init failed\r\n", 18);
        return;
    }

    terminal.ttf.scale = stbtt_ScaleForPixelHeight(&terminal.ttf.info, (float)DEFAULT_CHAR_HEIGHT * 0.8f); // 80% height to leave room
    stbtt_GetFontVMetrics(&terminal.ttf.info, &terminal.ttf.ascent, &terminal.ttf.descent, &terminal.ttf.line_gap);

    // Calculate baseline
    int pixel_height = (int)((terminal.ttf.ascent - terminal.ttf.descent) * terminal.ttf.scale);
    int y_adjust = (DEFAULT_CHAR_HEIGHT - pixel_height) / 2;
    terminal.ttf.baseline = (int)(terminal.ttf.ascent * terminal.ttf.scale) + y_adjust;

    terminal.ttf.loaded = true;
}

// Helper to allocate a glyph index in the dynamic atlas for any Unicode codepoint
uint32_t AllocateGlyph(uint32_t codepoint) {
    // Limit to BMP (Basic Multilingual Plane) for now as our map is 64K
    if (codepoint >= 65536) {
        return '?'; // Return safe fallback to prevent infinite allocation
    }

    // Check if already mapped
    if (terminal.glyph_map[codepoint] != 0) {
        return terminal.glyph_map[codepoint];
    }

    // Check capacity
    uint32_t capacity = (terminal.atlas_width / DEFAULT_CHAR_WIDTH) * (terminal.atlas_height / DEFAULT_CHAR_HEIGHT);
    if (terminal.next_atlas_index >= capacity) {
        // Atlas full. Use LRU Eviction.
        uint32_t lru_index = 0;
        uint64_t min_frame = 0xFFFFFFFFFFFFFFFF;

        // Scan for the oldest entry. Start from 256 to protect base set.
        // Optimization: We could use a min-heap or linked list, but linear scan is acceptable for this size/frequency.
        for (uint32_t i = 256; i < capacity; i++) {
            if (terminal.glyph_last_used[i] < min_frame) {
                min_frame = terminal.glyph_last_used[i];
                lru_index = i;
            }
        }

        // Evict
        if (lru_index >= 256) {
            uint32_t old_codepoint = terminal.atlas_to_codepoint[lru_index];
            if (old_codepoint < 65536) {
                terminal.glyph_map[old_codepoint] = 0; // Clear from map
            }

            // Reuse this index
            terminal.glyph_map[codepoint] = (uint16_t)lru_index;
            terminal.atlas_to_codepoint[lru_index] = codepoint;
            terminal.glyph_last_used[lru_index] = terminal.frame_count; // Touch

            RenderGlyphToAtlas(codepoint, lru_index);
            terminal.font_atlas_dirty = true;
            return lru_index;
        } else {
            return '?'; // Should not happen if capacity > 256
        }
    }

    uint32_t idx = terminal.next_atlas_index++;
    terminal.glyph_map[codepoint] = (uint16_t)idx;
    terminal.atlas_to_codepoint[idx] = codepoint;
    terminal.glyph_last_used[idx] = terminal.frame_count;

    RenderGlyphToAtlas(codepoint, idx);

    terminal.font_atlas_dirty = true;
    return idx;
}

// Helper to map Unicode codepoints to Dynamic Atlas indices
uint32_t MapUnicodeToAtlas(uint32_t codepoint) {
    if (codepoint < 256) {
        // Direct mapping for CP437 range (pre-loaded)
        // Wait, MapUnicodeToCP437 handles remapping.
        // We should reuse that logic to map Unicode -> CP437 index for known chars.
        // But now we use that index directly as Atlas Index.
        // If it returns '?', we might want to allocate a new one if it's truly unknown?
        // But CP437 map returns valid 0-255 indices for many unicode chars.
        // So we keep using it for the base set.
        return codepoint;
    }

    // Check if we have a CP437 mapping first (legacy)
    // Actually, AllocateGlyph handles arbitrary unicode.
    // We should try to allocate if not found.
    return AllocateGlyph(codepoint);
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

unsigned int TranslateDECSpecial(unsigned char ch) {
    // DEC Special Character Set translation
    switch (ch) {
        case 0x5F: return 0x00A0; // Non-breaking space
        case 0x60: return 0x25C6; // Diamond
        case 0x61: return 0x2592; // Checkerboard
        case 0x62: return 0x2409; // HT symbol
        case 0x63: return 0x240C; // FF symbol
        case 0x64: return 0x240D; // CR symbol
        case 0x65: return 0x240A; // LF symbol
        case 0x66: return 0x00B0; // Degree symbol
        case 0x67: return 0x00B1; // Plus-minus
        case 0x68: return 0x2424; // NL symbol
        case 0x69: return 0x240B; // VT symbol
        case 0x6A: return 0x2518; // Lower right corner
        case 0x6B: return 0x2510; // Upper right corner
        case 0x6C: return 0x250C; // Upper left corner
        case 0x6D: return 0x2514; // Lower left corner
        case 0x6E: return 0x253C; // Cross
        case 0x6F: return 0x23BA; // Horizontal line - scan 1
        case 0x70: return 0x23BB; // Horizontal line - scan 3
        case 0x71: return 0x2500; // Horizontal line
        case 0x72: return 0x23BC; // Horizontal line - scan 7
        case 0x73: return 0x23BD; // Horizontal line - scan 9
        case 0x74: return 0x251C; // Left tee
        case 0x75: return 0x2524; // Right tee
        case 0x76: return 0x2534; // Bottom tee
        case 0x77: return 0x252C; // Top tee
        case 0x78: return 0x2502; // Vertical line
        case 0x79: return 0x2264; // Less than or equal
        case 0x7A: return 0x2265; // Greater than or equal
        case 0x7B: return 0x03C0; // Pi
        case 0x7C: return 0x2260; // Not equal
        case 0x7D: return 0x00A3; // Pound sterling
        case 0x7E: return 0x00B7; // Middle dot
        default: return ch;
    }
}

unsigned int TranslateDECMultinational(unsigned char ch) {
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

void SetTabStop(int column) {
    if (column >= 0 && column < MAX_TAB_STOPS && column < DEFAULT_TERM_WIDTH) {
        if (!ACTIVE_SESSION.tab_stops.stops[column]) {
            ACTIVE_SESSION.tab_stops.stops[column] = true;
            ACTIVE_SESSION.tab_stops.count++;
        }
    }
}

void ClearTabStop(int column) {
    if (column >= 0 && column < MAX_TAB_STOPS) {
        if (ACTIVE_SESSION.tab_stops.stops[column]) {
            ACTIVE_SESSION.tab_stops.stops[column] = false;
            ACTIVE_SESSION.tab_stops.count--;
        }
    }
}

void ClearAllTabStops(void) {
    memset(ACTIVE_SESSION.tab_stops.stops, false, sizeof(ACTIVE_SESSION.tab_stops.stops));
    ACTIVE_SESSION.tab_stops.count = 0;
}

int NextTabStop(int current_column) {
    for (int i = current_column + 1; i < MAX_TAB_STOPS && i < DEFAULT_TERM_WIDTH; i++) {
        if (ACTIVE_SESSION.tab_stops.stops[i]) {
            return i;
        }
    }

    // If no explicit tab stop found, use default spacing
    int next = ((current_column / ACTIVE_SESSION.tab_stops.default_width) + 1) * ACTIVE_SESSION.tab_stops.default_width;
    return (next < DEFAULT_TERM_WIDTH) ? next : DEFAULT_TERM_WIDTH - 1;
}

int PreviousTabStop(int current_column) {
    for (int i = current_column - 1; i >= 0; i--) {
        if (ACTIVE_SESSION.tab_stops.stops[i]) {
            return i;
        }
    }

    // If no explicit tab stop found, use default spacing
    int prev = ((current_column - 1) / ACTIVE_SESSION.tab_stops.default_width) * ACTIVE_SESSION.tab_stops.default_width;
    return (prev >= 0) ? prev : 0;
}

// =============================================================================
// ENHANCED SCREEN MANIPULATION
// =============================================================================

void ClearCell(EnhancedTermChar* cell) {
    cell->ch = ' ';
    cell->fg_color = ACTIVE_SESSION.current_fg;
    cell->bg_color = ACTIVE_SESSION.current_bg;

    // Copy all current attributes
    cell->bold = ACTIVE_SESSION.bold_mode;
    cell->faint = ACTIVE_SESSION.faint_mode;
    cell->italic = ACTIVE_SESSION.italic_mode;
    cell->underline = ACTIVE_SESSION.underline_mode;
    cell->blink = ACTIVE_SESSION.blink_mode;
    cell->reverse = ACTIVE_SESSION.reverse_mode;
    cell->strikethrough = ACTIVE_SESSION.strikethrough_mode;
    cell->conceal = ACTIVE_SESSION.conceal_mode;
    cell->overline = ACTIVE_SESSION.overline_mode;
    cell->double_underline = ACTIVE_SESSION.double_underline_mode;
    cell->protected_cell = ACTIVE_SESSION.protected_mode;

    // Reset special attributes
    cell->double_width = false;
    cell->double_height_top = false;
    cell->double_height_bottom = false;
    cell->soft_hyphen = false;
    cell->combining = false;

    cell->dirty = true;
}

void ScrollUpRegion(int top, int bottom, int lines) {
    // Check for full screen scroll (Top to Bottom, Full Width)
    // This allows optimization via Ring Buffer pointer arithmetic.
    if (top == 0 && bottom == DEFAULT_TERM_HEIGHT - 1 &&
        ACTIVE_SESSION.left_margin == 0 && ACTIVE_SESSION.right_margin == DEFAULT_TERM_WIDTH - 1)
    {
        for (int i = 0; i < lines; i++) {
            // Increment head (scrolling down in memory, visually up)
            ACTIVE_SESSION.screen_head = (ACTIVE_SESSION.screen_head + 1) % ACTIVE_SESSION.buffer_height;

            // Adjust view_offset to keep historical view stable if user is looking back
            if (ACTIVE_SESSION.view_offset > 0) {
                 ACTIVE_SESSION.view_offset++;
                 // Cap at buffer limits
                 int max_offset = ACTIVE_SESSION.buffer_height - DEFAULT_TERM_HEIGHT;
                 if (ACTIVE_SESSION.view_offset > max_offset) ACTIVE_SESSION.view_offset = max_offset;
            }

            // Clear the new bottom line (logical row 'bottom')
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, bottom, x));
            }
        }
        // Invalidate all viewport rows because the data under them has shifted
        for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
            ACTIVE_SESSION.row_dirty[y] = true;
        }
        return;
    }

    // Partial Scroll (Fallback to copy) - Strictly NO head manipulation
    for (int i = 0; i < lines; i++) {
        // Move lines up within the region
        for (int y = top; y < bottom; y++) {
            for (int x = ACTIVE_SESSION.left_margin; x <= ACTIVE_SESSION.right_margin; x++) {
                *GetActiveScreenCell(&ACTIVE_SESSION, y, x) = *GetActiveScreenCell(&ACTIVE_SESSION, y + 1, x);
                GetActiveScreenCell(&ACTIVE_SESSION, y, x)->dirty = true;
            }
            ACTIVE_SESSION.row_dirty[y] = true;
        }

        // Clear bottom line of the region
        for (int x = ACTIVE_SESSION.left_margin; x <= ACTIVE_SESSION.right_margin; x++) {
            ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, bottom, x));
        }
        ACTIVE_SESSION.row_dirty[bottom] = true;
    }
}

void ScrollDownRegion(int top, int bottom, int lines) {
    for (int i = 0; i < lines; i++) {
        // Move lines down
        for (int y = bottom; y > top; y--) {
            for (int x = ACTIVE_SESSION.left_margin; x <= ACTIVE_SESSION.right_margin; x++) {
                *GetActiveScreenCell(&ACTIVE_SESSION, y, x) = *GetActiveScreenCell(&ACTIVE_SESSION, y - 1, x);
                GetActiveScreenCell(&ACTIVE_SESSION, y, x)->dirty = true;
            }
            ACTIVE_SESSION.row_dirty[y] = true;
        }

        // Clear top line
        for (int x = ACTIVE_SESSION.left_margin; x <= ACTIVE_SESSION.right_margin; x++) {
            ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, top, x));
        }
        ACTIVE_SESSION.row_dirty[top] = true;
    }
}

void InsertLinesAt(int row, int count) {
    if (row < ACTIVE_SESSION.scroll_top || row > ACTIVE_SESSION.scroll_bottom) {
        return;
    }

    // Move existing lines down
    for (int y = ACTIVE_SESSION.scroll_bottom; y >= row + count; y--) {
        if (y - count >= row) {
            for (int x = ACTIVE_SESSION.left_margin; x <= ACTIVE_SESSION.right_margin; x++) {
                *GetActiveScreenCell(&ACTIVE_SESSION, y, x) = *GetActiveScreenCell(&ACTIVE_SESSION, y - count, x);
                GetActiveScreenCell(&ACTIVE_SESSION, y, x)->dirty = true;
            }
            ACTIVE_SESSION.row_dirty[y] = true;
        }
    }

    // Clear inserted lines
    for (int y = row; y < row + count && y <= ACTIVE_SESSION.scroll_bottom; y++) {
        for (int x = ACTIVE_SESSION.left_margin; x <= ACTIVE_SESSION.right_margin; x++) {
            ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, y, x));
        }
        ACTIVE_SESSION.row_dirty[y] = true;
    }
}

void DeleteLinesAt(int row, int count) {
    if (row < ACTIVE_SESSION.scroll_top || row > ACTIVE_SESSION.scroll_bottom) {
        return;
    }

    // Move remaining lines up
    for (int y = row; y <= ACTIVE_SESSION.scroll_bottom - count; y++) {
        for (int x = ACTIVE_SESSION.left_margin; x <= ACTIVE_SESSION.right_margin; x++) {
            *GetActiveScreenCell(&ACTIVE_SESSION, y, x) = *GetActiveScreenCell(&ACTIVE_SESSION, y + count, x);
            GetActiveScreenCell(&ACTIVE_SESSION, y, x)->dirty = true;
        }
        ACTIVE_SESSION.row_dirty[y] = true;
    }

    // Clear bottom lines
    for (int y = ACTIVE_SESSION.scroll_bottom - count + 1; y <= ACTIVE_SESSION.scroll_bottom; y++) {
        if (y >= 0) {
            for (int x = ACTIVE_SESSION.left_margin; x <= ACTIVE_SESSION.right_margin; x++) {
                ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, y, x));
            }
            ACTIVE_SESSION.row_dirty[y] = true;
        }
    }
}

void InsertCharactersAt(int row, int col, int count) {
    // Shift existing characters right
    for (int x = ACTIVE_SESSION.right_margin; x >= col + count; x--) {
        if (x - count >= col) {
            *GetActiveScreenCell(&ACTIVE_SESSION, row, x) = *GetActiveScreenCell(&ACTIVE_SESSION, row, x - count);
            GetActiveScreenCell(&ACTIVE_SESSION, row, x)->dirty = true;
        }
    }

    // Clear inserted positions
    for (int x = col; x < col + count && x <= ACTIVE_SESSION.right_margin; x++) {
        ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, row, x));
    }
    ACTIVE_SESSION.row_dirty[row] = true;
}

void DeleteCharactersAt(int row, int col, int count) {
    // Shift remaining characters left
    for (int x = col; x <= ACTIVE_SESSION.right_margin - count; x++) {
        *GetActiveScreenCell(&ACTIVE_SESSION, row, x) = *GetActiveScreenCell(&ACTIVE_SESSION, row, x + count);
        GetActiveScreenCell(&ACTIVE_SESSION, row, x)->dirty = true;
    }

    // Clear rightmost positions
    for (int x = ACTIVE_SESSION.right_margin - count + 1; x <= ACTIVE_SESSION.right_margin; x++) {
        if (x >= 0) {
            ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, row, x));
        }
    }
    ACTIVE_SESSION.row_dirty[row] = true;
}

// =============================================================================
// VT100 INSERT MODE IMPLEMENTATION
// =============================================================================

void EnableInsertMode(bool enable) {
    ACTIVE_SESSION.dec_modes.insert_mode = enable;
}

void InsertCharacterAtCursor(unsigned int ch) {
    if (ACTIVE_SESSION.dec_modes.insert_mode) {
        // Insert mode: shift existing characters right
        InsertCharactersAt(ACTIVE_SESSION.cursor.y, ACTIVE_SESSION.cursor.x, 1);
    }

    // Place character at cursor position
    EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, ACTIVE_SESSION.cursor.x);
    cell->ch = ch;
    cell->fg_color = ACTIVE_SESSION.current_fg;
    cell->bg_color = ACTIVE_SESSION.current_bg;

    // Apply current attributes
    cell->bold = ACTIVE_SESSION.bold_mode;
    cell->faint = ACTIVE_SESSION.faint_mode;
    cell->italic = ACTIVE_SESSION.italic_mode;
    cell->underline = ACTIVE_SESSION.underline_mode;
    cell->blink = ACTIVE_SESSION.blink_mode;
    cell->reverse = ACTIVE_SESSION.reverse_mode;
    cell->strikethrough = ACTIVE_SESSION.strikethrough_mode;
    cell->conceal = ACTIVE_SESSION.conceal_mode;
    cell->overline = ACTIVE_SESSION.overline_mode;
    cell->double_underline = ACTIVE_SESSION.double_underline_mode;
    cell->protected_cell = ACTIVE_SESSION.protected_mode;

    cell->dirty = true;
    ACTIVE_SESSION.row_dirty[ACTIVE_SESSION.cursor.y] = true;

    // Track last printed character for REP command
    ACTIVE_SESSION.last_char = ch;
}

// =============================================================================
// COMPREHENSIVE CHARACTER PROCESSING
// =============================================================================

void ProcessNormalChar(unsigned char ch) {
    // Handle control characters first
    if (ch < 32) {
        ProcessControlChar(ch);
        return;
    }

    // Translate character through active character set
    unsigned int unicode_ch = TranslateCharacter(ch, &ACTIVE_SESSION.charset);

    // Handle UTF-8 decoding if enabled
    if (*ACTIVE_SESSION.charset.gl == CHARSET_UTF8) {
        if (ACTIVE_SESSION.utf8.bytes_remaining == 0) {
            if (ch < 0x80) {
                // 1-byte sequence (ASCII)
                unicode_ch = ch;
            } else if ((ch & 0xE0) == 0xC0) {
                // 2-byte sequence
                ACTIVE_SESSION.utf8.codepoint = ch & 0x1F;
                ACTIVE_SESSION.utf8.bytes_remaining = 1;
                return;
            } else if ((ch & 0xF0) == 0xE0) {
                // 3-byte sequence
                ACTIVE_SESSION.utf8.codepoint = ch & 0x0F;
                ACTIVE_SESSION.utf8.bytes_remaining = 2;
                return;
            } else if ((ch & 0xF8) == 0xF0) {
                // 4-byte sequence
                ACTIVE_SESSION.utf8.codepoint = ch & 0x07;
                ACTIVE_SESSION.utf8.bytes_remaining = 3;
                return;
            } else {
                // Invalid start byte
                unicode_ch = 0xFFFD;
                InsertCharacterAtCursor(unicode_ch);
                ACTIVE_SESSION.cursor.x++;
                return;
            }
        } else {
            // Continuation byte
            if ((ch & 0xC0) == 0x80) {
                ACTIVE_SESSION.utf8.codepoint = (ACTIVE_SESSION.utf8.codepoint << 6) | (ch & 0x3F);
                ACTIVE_SESSION.utf8.bytes_remaining--;
                if (ACTIVE_SESSION.utf8.bytes_remaining > 0) {
                    return;
                }
                // Sequence complete
                unicode_ch = ACTIVE_SESSION.utf8.codepoint;

                // Attempt to map to CP437 for pixel-perfect box drawing, but preserve Unicode if not found
                uint8_t cp437 = MapUnicodeToCP437(unicode_ch);
                if (cp437 != '?' || unicode_ch == '?') {
                    unicode_ch = cp437;
                }
                // If cp437 is '?' but input wasn't '?', we keep the original unicode_ch.
                // This allows dynamic glyph allocation for all other Unicode chars.
            } else {
                // Invalid continuation byte
                // Emit replacement character for the failed sequence
                unicode_ch = 0xFFFD;
                InsertCharacterAtCursor(unicode_ch);
                ACTIVE_SESSION.cursor.x++;

                // Reset and try to recover
                ACTIVE_SESSION.utf8.bytes_remaining = 0;
                ACTIVE_SESSION.utf8.codepoint = 0;
                // If the byte itself is a valid start byte or ASCII, we should process it.
                // Re-evaluate current char as if state was 0.
                if (ch < 0x80) {
                    unicode_ch = ch;
                } else if ((ch & 0xE0) == 0xC0) {
                    ACTIVE_SESSION.utf8.codepoint = ch & 0x1F;
                    ACTIVE_SESSION.utf8.bytes_remaining = 1;
                    return;
                } else if ((ch & 0xF0) == 0xE0) {
                    ACTIVE_SESSION.utf8.codepoint = ch & 0x0F;
                    ACTIVE_SESSION.utf8.bytes_remaining = 2;
                    return;
                } else if ((ch & 0xF8) == 0xF0) {
                    ACTIVE_SESSION.utf8.codepoint = ch & 0x07;
                    ACTIVE_SESSION.utf8.bytes_remaining = 3;
                    return;
                } else {
                    return; // Invalid start byte
                }
            }
        }
    }

    // Handle character display
    if (ACTIVE_SESSION.cursor.x > ACTIVE_SESSION.right_margin) {
        if (ACTIVE_SESSION.dec_modes.auto_wrap_mode) {
            // Auto-wrap to next line
            ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
            ACTIVE_SESSION.cursor.y++;

            if (ACTIVE_SESSION.cursor.y > ACTIVE_SESSION.scroll_bottom) {
                ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_bottom;
                ScrollUpRegion(ACTIVE_SESSION.scroll_top, ACTIVE_SESSION.scroll_bottom, 1);
            }
        } else {
            // No wrap - stay at right margin
            ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.right_margin;
        }
    }

    // Insert character (handles insert mode internally)
    InsertCharacterAtCursor(unicode_ch);

    // Advance cursor
    ACTIVE_SESSION.cursor.x++;
}

// Update ProcessControlChar
void ProcessControlChar(unsigned char ch) {
    switch (ch) {
        case 0x05: // ENQ - Enquiry
            if (ACTIVE_SESSION.answerback_buffer[0] != '\0') {
                QueueResponse(ACTIVE_SESSION.answerback_buffer);
            }
            break;
        case 0x07: // BEL - Bell
            if (bell_callback) {
                bell_callback();
            } else {
                // Visual bell
                ACTIVE_SESSION.visual_bell_timer = 0.2;
            }
            break;
        case 0x08: // BS - Backspace
            if (ACTIVE_SESSION.cursor.x > ACTIVE_SESSION.left_margin) {
                ACTIVE_SESSION.cursor.x--;
            }
            break;
        case 0x09: // HT - Horizontal Tab
            ACTIVE_SESSION.cursor.x = NextTabStop(ACTIVE_SESSION.cursor.x);
            if (ACTIVE_SESSION.cursor.x > ACTIVE_SESSION.right_margin) {
                ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.right_margin;
            }
            break;
        case 0x0A: // LF - Line Feed
        case 0x0B: // VT - Vertical Tab
        case 0x0C: // FF - Form Feed
            ACTIVE_SESSION.cursor.y++;
            if (ACTIVE_SESSION.cursor.y > ACTIVE_SESSION.scroll_bottom) {
                ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_bottom;
                ScrollUpRegion(ACTIVE_SESSION.scroll_top, ACTIVE_SESSION.scroll_bottom, 1);
            }
            if (ACTIVE_SESSION.ansi_modes.line_feed_new_line) {
                ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
            }
            break;
        case 0x0D: // CR - Carriage Return
            ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
            break;
        case 0x0E: // SO - Shift Out (invoke G1 into GL)
            ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g1;
            break;
        case 0x0F: // SI - Shift In (invoke G0 into GL)
            ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g0;
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
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            ACTIVE_SESSION.escape_pos = 0;
            break;
        case 0x1B: // ESC - Escape
            ACTIVE_SESSION.parse_state = VT_PARSE_ESCAPE;
            ACTIVE_SESSION.escape_pos = 0;
            break;
        case 0x7F: // DEL - Delete
            // Ignored
            break;
        default:
            if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown control char: 0x%02X", ch);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }
}


// =============================================================================
// ENHANCED ESCAPE SEQUENCE PROCESSING
// =============================================================================

void ProcessEscapeChar(unsigned char ch) {
    switch (ch) {
        // CSI - Control Sequence Introducer
        case '[':
            ACTIVE_SESSION.parse_state = PARSE_CSI;
            ACTIVE_SESSION.escape_pos = 0;
            memset(ACTIVE_SESSION.escape_params, 0, sizeof(ACTIVE_SESSION.escape_params));
            ACTIVE_SESSION.param_count = 0;
            break;

        // OSC - Operating System Command
        case ']':
            ACTIVE_SESSION.parse_state = PARSE_OSC;
            ACTIVE_SESSION.escape_pos = 0;
            break;

        // DCS - Device Control String
        case 'P':
            ACTIVE_SESSION.parse_state = PARSE_DCS;
            ACTIVE_SESSION.escape_pos = 0;
            break;

        // APC - Application Program Command
        case '_':
            ACTIVE_SESSION.parse_state = PARSE_APC;
            ACTIVE_SESSION.escape_pos = 0;
            break;

        // PM - Privacy Message
        case '^':
            ACTIVE_SESSION.parse_state = PARSE_PM;
            ACTIVE_SESSION.escape_pos = 0;
            break;

        // SOS - Start of String
        case 'X':
            ACTIVE_SESSION.parse_state = PARSE_SOS;
            ACTIVE_SESSION.escape_pos = 0;
            break;

        // Character set selection
        case '(':
            ACTIVE_SESSION.parse_state = PARSE_CHARSET;
            ACTIVE_SESSION.escape_buffer[0] = '(';
            ACTIVE_SESSION.escape_pos = 1;
            break;

        case ')':
            ACTIVE_SESSION.parse_state = PARSE_CHARSET;
            ACTIVE_SESSION.escape_buffer[0] = ')';
            ACTIVE_SESSION.escape_pos = 1;
            break;

        case '*':
            ACTIVE_SESSION.parse_state = PARSE_CHARSET;
            ACTIVE_SESSION.escape_buffer[0] = '*';
            ACTIVE_SESSION.escape_pos = 1;
            break;

        case '+':
            ACTIVE_SESSION.parse_state = PARSE_CHARSET;
            ACTIVE_SESSION.escape_buffer[0] = '+';
            ACTIVE_SESSION.escape_pos = 1;
            break;

        // Locking Shifts (ISO 2022)
        case 'n': // LS2 (GL = G2)
            ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g2;
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;
        case 'o': // LS3 (GL = G3)
            ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g3;
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;
        case '~': // LS1R (GR = G1)
            ACTIVE_SESSION.charset.gr = &ACTIVE_SESSION.charset.g1;
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;
        case '}': // LS2R (GR = G2)
            ACTIVE_SESSION.charset.gr = &ACTIVE_SESSION.charset.g2;
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;
        case '|': // LS3R (GR = G3)
            ACTIVE_SESSION.charset.gr = &ACTIVE_SESSION.charset.g3;
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        // Single character commands
        case '7': // DECSC - Save Cursor
            ExecuteSaveCursor();
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case '8': // DECRC - Restore Cursor
            ExecuteRestoreCursor();
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case '#': // DEC Line Attributes
            ACTIVE_SESSION.parse_state = PARSE_HASH;
            break;

        case '%': // Select Character Set
            ACTIVE_SESSION.parse_state = PARSE_PERCENT;
            break;

        case 'D': // IND - Index
            ACTIVE_SESSION.cursor.y++;
            if (ACTIVE_SESSION.cursor.y > ACTIVE_SESSION.scroll_bottom) {
                ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_bottom;
                ScrollUpRegion(ACTIVE_SESSION.scroll_top, ACTIVE_SESSION.scroll_bottom, 1);
            }
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case 'E': // NEL - Next Line
            ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
            ACTIVE_SESSION.cursor.y++;
            if (ACTIVE_SESSION.cursor.y > ACTIVE_SESSION.scroll_bottom) {
                ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_bottom;
                ScrollUpRegion(ACTIVE_SESSION.scroll_top, ACTIVE_SESSION.scroll_bottom, 1);
            }
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case 'H': // HTS - Set Tab Stop
            SetTabStop(ACTIVE_SESSION.cursor.x);
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case 'M': // RI - Reverse Index
            ACTIVE_SESSION.cursor.y--;
            if (ACTIVE_SESSION.cursor.y < ACTIVE_SESSION.scroll_top) {
                ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_top;
                ScrollDownRegion(ACTIVE_SESSION.scroll_top, ACTIVE_SESSION.scroll_bottom, 1);
            }
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        // case 'n': // LS2 - Locking Shift 2 (Handled above as LS2)
        // case 'o': // LS3 - Locking Shift 3 (Handled above as LS3)

        case 'N': // SS2 - Single Shift 2
            ACTIVE_SESSION.charset.single_shift_2 = true;
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case 'O': // SS3 - Single Shift 3
            ACTIVE_SESSION.charset.single_shift_3 = true;
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case 'Z': // DECID - Identify Terminal
            QueueResponse(ACTIVE_SESSION.device_attributes);
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case 'c': // RIS - Reset to Initial State
            InitTerminal();
            break;

        case '=': // DECKPAM - Keypad Application Mode
            ACTIVE_SESSION.vt_keyboard.keypad_mode = true;
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case '>': // DECKPNM - Keypad Numeric Mode
            ACTIVE_SESSION.vt_keyboard.keypad_mode = false;
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;

        case '<': // Enter VT52 mode (if enabled)
            if (ACTIVE_SESSION.conformance.features.vt52_mode) {
                ACTIVE_SESSION.parse_state = PARSE_VT52;
            } else {
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                if (ACTIVE_SESSION.options.log_unsupported) {
                    LogUnsupportedSequence("VT52 mode not supported");
                }
            }
            break;

        default:
            // Unknown escape sequence
            if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC %c (0x%02X)", ch, ch);
                LogUnsupportedSequence(debug_msg);
            }
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            break;
    }
}

// =============================================================================
// ENHANCED PIPELINE PROCESSING
// =============================================================================
bool PipelineWriteChar(unsigned char ch) {
    if (ACTIVE_SESSION.pipeline_count >= sizeof(ACTIVE_SESSION.input_pipeline) - 1) {
        ACTIVE_SESSION.pipeline_overflow = true;
        return false;
    }

    ACTIVE_SESSION.input_pipeline[ACTIVE_SESSION.pipeline_head] = ch;
    ACTIVE_SESSION.pipeline_head = (ACTIVE_SESSION.pipeline_head + 1) % sizeof(ACTIVE_SESSION.input_pipeline);
    ACTIVE_SESSION.pipeline_count++;
    return true;
}

bool PipelineWriteString(const char* str) {
    if (!str) return false;

    while (*str) {
        if (!PipelineWriteChar(*str)) {
            return false;
        }
        str++;
    }
    return true;
}

bool PipelineWriteFormat(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return PipelineWriteString(buffer);
}

void ClearPipeline(void) {
    ACTIVE_SESSION.pipeline_head = 0;
    ACTIVE_SESSION.pipeline_tail = 0;
    ACTIVE_SESSION.pipeline_count = 0;
    ACTIVE_SESSION.pipeline_overflow = false;
}

// =============================================================================
// BASIC IMPLEMENTATIONS FOR MISSING FUNCTIONS
// =============================================================================

void SetResponseCallback(ResponseCallback callback) {
    terminal.response_callback = callback;
}

void SetPrinterCallback(PrinterCallback callback) {
    terminal.printer_callback = callback;
}

void SetTitleCallback(TitleCallback callback) {
    title_callback = callback;
}

void SetBellCallback(BellCallback callback) {
    bell_callback = callback;
}

void SetNotificationCallback(NotificationCallback callback) {
    notification_callback = callback;
    terminal.notification_callback = callback;
}

const char* GetWindowTitle(void) {
    return ACTIVE_SESSION.title.window_title;
}

const char* GetIconTitle(void) {
    return ACTIVE_SESSION.title.icon_title;
}

void SetTerminalMode(const char* mode, bool enable) {
    if (strcmp(mode, "application_cursor") == 0) {
        ACTIVE_SESSION.dec_modes.application_cursor_keys = enable;
    } else if (strcmp(mode, "auto_wrap") == 0) {
        ACTIVE_SESSION.dec_modes.auto_wrap_mode = enable;
    } else if (strcmp(mode, "origin") == 0) {
        ACTIVE_SESSION.dec_modes.origin_mode = enable;
    } else if (strcmp(mode, "insert") == 0) {
        ACTIVE_SESSION.dec_modes.insert_mode = enable;
    }
}

void SetCursorShape(CursorShape shape) {
    ACTIVE_SESSION.cursor.shape = shape;
}

void SetCursorColor(ExtendedColor color) {
    ACTIVE_SESSION.cursor.color = color;
}

void SetMouseTracking(MouseTrackingMode mode) {
    ACTIVE_SESSION.mouse.mode = mode;
    ACTIVE_SESSION.mouse.enabled = (mode != MOUSE_TRACKING_OFF);
}

// Enable or disable mouse features
// Toggles specific mouse functionalities based on feature name
void EnableMouseFeature(const char* feature, bool enable) {
    if (strcmp(feature, "focus") == 0) {
        // Enable/disable focus tracking for mouse reporting (CSI ?1004 h/l)
        ACTIVE_SESSION.mouse.focus_tracking = enable;
    } else if (strcmp(feature, "sgr") == 0) {
        // Enable/disable SGR mouse reporting mode (CSI ?1006 h/l)
        ACTIVE_SESSION.mouse.sgr_mode = enable;
        // Update mouse mode to SGR if enabled and tracking is active
        if (enable && ACTIVE_SESSION.mouse.mode != MOUSE_TRACKING_OFF &&
            ACTIVE_SESSION.mouse.mode != MOUSE_TRACKING_URXVT && ACTIVE_SESSION.mouse.mode != MOUSE_TRACKING_PIXEL) {
            ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_SGR;
        } else if (!enable && ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_SGR) {
            ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_VT200; // Fallback to VT200
        }
    } else if (strcmp(feature, "cursor") == 0) {
        // Enable/disable custom mouse cursor rendering
        ACTIVE_SESSION.mouse.enabled = enable;
        if (!enable) {
            ACTIVE_SESSION.mouse.cursor_x = -1; // Hide cursor
            ACTIVE_SESSION.mouse.cursor_y = -1;
        }
    } else if (strcmp(feature, "urxvt") == 0) {
        // Enable/disable URXVT mouse reporting mode (CSI ?1015 h/l)
        if (enable) {
            ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_URXVT;
            ACTIVE_SESSION.mouse.enabled = true; // Ensure cursor is enabled
        } else if (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_URXVT) {
            ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_OFF;
        }
    } else if (strcmp(feature, "pixel") == 0) {
        // Enable/disable pixel position mouse reporting mode (CSI ?1016 h/l)
        if (enable) {
            ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_PIXEL;
            ACTIVE_SESSION.mouse.enabled = true; // Ensure cursor is enabled
        } else if (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_PIXEL) {
            ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_OFF;
        }
    }
}

void EnableBracketedPaste(bool enable) {
    ACTIVE_SESSION.bracketed_paste.enabled = enable;
}

bool IsBracketedPasteActive(void) {
    return ACTIVE_SESSION.bracketed_paste.active;
}

void ProcessPasteData(const char* data, size_t length) {
    if (ACTIVE_SESSION.bracketed_paste.enabled) {
        PipelineWriteString("\x1B[200~");
        PipelineWriteString(data);
        PipelineWriteString("\x1B[201~");
    } else {
        PipelineWriteString(data);
    }
}

void CopySelectionToClipboard(void) {
    if (!ACTIVE_SESSION.selection.active) return;

    int start_y = ACTIVE_SESSION.selection.start_y;
    int start_x = ACTIVE_SESSION.selection.start_x;
    int end_y = ACTIVE_SESSION.selection.end_y;
    int end_x = ACTIVE_SESSION.selection.end_x;

    uint32_t s_idx = start_y * DEFAULT_TERM_WIDTH + start_x;
    uint32_t e_idx = end_y * DEFAULT_TERM_WIDTH + end_x;

    if (s_idx > e_idx) { uint32_t t = s_idx; s_idx = e_idx; e_idx = t; }

    size_t char_count = (e_idx - s_idx) + 1 + (DEFAULT_TERM_HEIGHT * 2);
    char* text_buf = calloc(char_count * 4, 1); // UTF-8 safety
    if (!text_buf) return;
    size_t buf_idx = 0;

    int last_y = -1;
    for (uint32_t i = s_idx; i <= e_idx; i++) {
        int cy = i / DEFAULT_TERM_WIDTH;
        int cx = i % DEFAULT_TERM_WIDTH;

        if (last_y != -1 && cy != last_y) {
            text_buf[buf_idx++] = '\n';
        }
        last_y = cy;

        EnhancedTermChar* cell = GetScreenCell(&ACTIVE_SESSION, cy, cx);
        if (cell->ch) {
            if (cell->ch < 128) text_buf[buf_idx++] = (char)cell->ch;
            else {
               // Basic placeholder for now, ideally real UTF-8 encoding
               text_buf[buf_idx++] = '?';
            }
        }
    }
    text_buf[buf_idx] = '\0';
    SituationSetClipboardText(text_buf);
    free(text_buf);
}

// Update mouse state (internal use only)
// Processes mouse position, buttons, wheel, motion, focus changes, and updates cursor position
void UpdateMouse(void) {
    // 1. Get Global Mouse Position
    Vector2 mouse_pos = SituationGetMousePosition();
    int global_cell_x = (int)(mouse_pos.x / (DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE)); // 0-based
    int global_cell_y = (int)(mouse_pos.y / (DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE)); // 0-based

    // 2. Identify Target Session and Transform Coordinates
    int target_session_idx = terminal.active_session;
    int local_cell_y = global_cell_y;
    int local_pixel_y = (int)mouse_pos.y + 1; // 1-based

    if (terminal.split_screen_active) {
        if (global_cell_y <= terminal.split_row) {
            target_session_idx = terminal.session_top;
            local_cell_y = global_cell_y;
            // local_pixel_y remains same
        } else {
            target_session_idx = terminal.session_bottom;
            local_cell_y = global_cell_y - (terminal.split_row + 1);
            local_pixel_y = (int)mouse_pos.y - ((terminal.split_row + 1) * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE) + 1;
        }
    }

    // 3. Handle Focus on Click
    if (SituationIsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (terminal.active_session != target_session_idx) {
            SetActiveSession(target_session_idx);
        }
    }

    // 4. Temporarily switch context to target session for logic execution
    int saved_session = terminal.active_session;
    terminal.active_session = target_session_idx;

    // --- Begin Session-Specific Logic ---

    // Clamp Coordinates (Session-Relative)
    if (global_cell_x < 0) global_cell_x = 0; if (global_cell_x >= DEFAULT_TERM_WIDTH) global_cell_x = DEFAULT_TERM_WIDTH - 1;
    if (local_cell_y < 0) local_cell_y = 0; if (local_cell_y >= DEFAULT_TERM_HEIGHT) local_cell_y = DEFAULT_TERM_HEIGHT - 1;

    // Handle Mouse Wheel Scrolling
    float wheel = SituationGetMouseWheelMove();
    if (wheel != 0) {
        if (ACTIVE_SESSION.dec_modes.alternate_screen) {
            // Send Arrow Keys in Alternate Screen Mode (3 lines per step)
            // Typically Wheel Up -> Arrow Up (Scrolls content down to see up) -> \x1B[A
            // Wheel Down -> Arrow Down -> \x1B[B
            int lines = 3;
            // Positive wheel = Up (scroll back/up). Negative = Down.
            const char* seq = (wheel > 0) ? (ACTIVE_SESSION.vt_keyboard.cursor_key_mode ? "\x1BOA" : "\x1B[A")
                                          : (ACTIVE_SESSION.vt_keyboard.cursor_key_mode ? "\x1BOB" : "\x1B[B");
            for(int i=0; i<lines; i++) QueueResponse(seq);
        } else {
            // Scroll History in Primary Screen Mode
            // Wheel Up (Positive) -> Increase view_offset (Look back)
            // Wheel Down (Negative) -> Decrease view_offset (Look forward)
            int scroll_amount = (int)(wheel * 3.0f);
            ACTIVE_SESSION.view_offset += scroll_amount;

            // Clamp
            if (ACTIVE_SESSION.view_offset < 0) ACTIVE_SESSION.view_offset = 0;
            int max_offset = ACTIVE_SESSION.buffer_height - DEFAULT_TERM_HEIGHT;
            if (ACTIVE_SESSION.view_offset > max_offset) ACTIVE_SESSION.view_offset = max_offset;

            // Invalidate screen
            for(int i=0; i<DEFAULT_TERM_HEIGHT; i++) ACTIVE_SESSION.row_dirty[i] = true;
        }
    }

    // Handle Selection Logic (Left Click Drag) - Using Local Coords
    // Note: Cross-session selection is not supported in this simple model.
    if (SituationIsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        ACTIVE_SESSION.selection.active = true;
        ACTIVE_SESSION.selection.dragging = true;
        ACTIVE_SESSION.selection.start_x = global_cell_x;
        ACTIVE_SESSION.selection.start_y = local_cell_y;
        ACTIVE_SESSION.selection.end_x = global_cell_x;
        ACTIVE_SESSION.selection.end_y = local_cell_y;
    } else if (SituationIsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && ACTIVE_SESSION.selection.dragging) {
        ACTIVE_SESSION.selection.end_x = global_cell_x;
        ACTIVE_SESSION.selection.end_y = local_cell_y;
    } else if (SituationIsMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT) && ACTIVE_SESSION.selection.dragging) {
        ACTIVE_SESSION.selection.dragging = false;
        CopySelectionToClipboard();
    }

    // Exit if mouse tracking feature is not supported
    if (!ACTIVE_SESSION.conformance.features.mouse_tracking) {
        terminal.active_session = saved_session; // Restore
        return;
    }

    // Exit if mouse is disabled or tracking is off
    if (!ACTIVE_SESSION.mouse.enabled || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_OFF) {
        SituationShowCursor(); // Show system cursor
        ACTIVE_SESSION.mouse.cursor_x = -1; // Hide custom cursor
        ACTIVE_SESSION.mouse.cursor_y = -1;
        terminal.active_session = saved_session; // Restore
        return;
    }

    // Hide system cursor during mouse tracking
    SituationHideCursor();

    int pixel_x = (int)mouse_pos.x + 1; // Global X matches Local X for now (no split vertical)

    // Update custom cursor position (1-based for VT)
    ACTIVE_SESSION.mouse.cursor_x = global_cell_x + 1;
    ACTIVE_SESSION.mouse.cursor_y = local_cell_y + 1;

    // Get button states
    bool current_buttons_state[3];
    current_buttons_state[0] = SituationIsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
    current_buttons_state[1] = SituationIsMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE);
    current_buttons_state[2] = SituationIsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);

    // Get wheel movement
    float wheel_move = SituationGetMouseWheelMove();
    char mouse_report[64];

    // Handle button press/release events
    for (size_t i = 0; i < 3; i++) {
        if (current_buttons_state[i] != ACTIVE_SESSION.mouse.buttons[i]) {
            ACTIVE_SESSION.mouse.buttons[i] = current_buttons_state[i];
            bool pressed = current_buttons_state[i];
            int report_button_code = i;
            mouse_report[0] = '\0';

            if (ACTIVE_SESSION.mouse.sgr_mode || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_URXVT || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_PIXEL) {
                if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) report_button_code += 4;
                if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) report_button_code += 8;
                if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) report_button_code += 16;

                if (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_PIXEL) {
                     snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%d%c",
                             report_button_code, pixel_x, local_pixel_y, pressed ? 'M' : 'm');
                } else {
                     snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%d%c",
                             report_button_code, global_cell_x + 1, local_cell_y + 1, pressed ? 'M' : 'm');
                }
            } else if (ACTIVE_SESSION.mouse.mode >= MOUSE_TRACKING_VT200 && ACTIVE_SESSION.mouse.mode <= MOUSE_TRACKING_ANY_EVENT) {
                int cb_button = pressed ? i : 3;
                int cb = 32 + cb_button;
                if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) cb += 4;
                if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) cb += 8;
                if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) cb += 16;
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c",
                        (char)cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
            } else if (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_X10) {
                if (pressed) {
                    int cb = 32 + i;
                    snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c",
                            (char)cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
                }
            }
            if (mouse_report[0]) QueueResponse(mouse_report);
        }
    }

    // Handle wheel events
    if (wheel_move != 0) {
        int report_button_code = (wheel_move > 0) ? 64 : 65;
        mouse_report[0] = '\0';
        if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) report_button_code += 4;
        if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) report_button_code += 8;
        if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) report_button_code += 16;

        if (ACTIVE_SESSION.mouse.sgr_mode || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_URXVT || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_PIXEL) {
             if (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_PIXEL) {
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM",
                        report_button_code, pixel_x, local_pixel_y);
             } else {
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM",
                            report_button_code, global_cell_x + 1, local_cell_y + 1);
             }
        } else if (ACTIVE_SESSION.mouse.mode >= MOUSE_TRACKING_VT200 && ACTIVE_SESSION.mouse.mode <= MOUSE_TRACKING_ANY_EVENT) {
            int cb = 32 + ((wheel_move > 0) ? 0 : 1) + 64;
            if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) cb += 4;
            if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) cb += 8;
            if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) cb += 16;
            snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c",
                    (char)cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
        }
        if (mouse_report[0]) QueueResponse(mouse_report);
    }

    // Handle motion events
    if (global_cell_x != ACTIVE_SESSION.mouse.last_x || local_cell_y != ACTIVE_SESSION.mouse.last_y ||
        (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_PIXEL && (pixel_x != ACTIVE_SESSION.mouse.last_pixel_x || local_pixel_y != ACTIVE_SESSION.mouse.last_pixel_y))) {
        bool report_move = false;
        int sgr_motion_code = 35; // Motion no button for SGR
        int vt200_motion_cb = 32 + 3; // Motion no button for VT200

        if (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_ANY_EVENT) {
            report_move = true;
        } else if (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_VT200_HIGHLIGHT || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_BTN_EVENT) {
            if (current_buttons_state[0] || current_buttons_state[1] || current_buttons_state[2]) report_move = true;
        } else if (ACTIVE_SESSION.mouse.sgr_mode || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_URXVT || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_PIXEL) {
             if (current_buttons_state[0] || current_buttons_state[1] || current_buttons_state[2]) report_move = true;
        }

        if (report_move) {
            mouse_report[0] = '\0';
            if (ACTIVE_SESSION.mouse.sgr_mode || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_URXVT || ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_PIXEL) {
                if(current_buttons_state[0]) sgr_motion_code = 32;
                else if(current_buttons_state[1]) sgr_motion_code = 33;
                else if(current_buttons_state[2]) sgr_motion_code = 34;

                if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) sgr_motion_code += 4;
                if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) sgr_motion_code += 8;
                if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) sgr_motion_code += 16;

                if (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_PIXEL) {
                    snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM", sgr_motion_code, pixel_x, local_pixel_y);
                } else {
                    snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM", sgr_motion_code, global_cell_x + 1, local_cell_y + 1);
                }
            } else {
                if(current_buttons_state[0]) vt200_motion_cb = 32 + 0;
                else if(current_buttons_state[1]) vt200_motion_cb = 32 + 1;
                else if(current_buttons_state[2]) vt200_motion_cb = 32 + 2;

                if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) vt200_motion_cb += 4;
                if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) vt200_motion_cb += 8;
                if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) vt200_motion_cb += 16;
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c", (char)vt200_motion_cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
            }
            if (mouse_report[0]) QueueResponse(mouse_report);
        }
        ACTIVE_SESSION.mouse.last_x = global_cell_x;
        ACTIVE_SESSION.mouse.last_y = local_cell_y;
        ACTIVE_SESSION.mouse.last_pixel_x = pixel_x;
        ACTIVE_SESSION.mouse.last_pixel_y = local_pixel_y;
    }

    // Restore original active session context
    terminal.active_session = saved_session;

    // Handle focus changes (global or session specific? Focus usually window based)
    // We should probably report focus to the active session.
    // Since we restored session, we use ACTIVE_SESSION macro which now points to saved_session.

    if (ACTIVE_SESSION.mouse.focus_tracking) {
        bool current_focus = SituationHasWindowFocus();
        if (current_focus && !ACTIVE_SESSION.mouse.focused) {
            QueueResponse("\x1B[I"); // Focus In
        } else if (!current_focus && ACTIVE_SESSION.mouse.focused) {
            QueueResponse("\x1B[O"); // Focus Out
        }
        ACTIVE_SESSION.mouse.focused = current_focus;
    }
}



void SetKeyboardDialect(int dialect) {
    if (dialect >= 1 && dialect <= 10) { // Example range, adjust per NRCS standards
        ACTIVE_SESSION.vt_keyboard.keyboard_dialect = dialect;
    }
}

void SetPrinterAvailable(bool available) {
    ACTIVE_SESSION.printer_available = available;
}

void SetLocatorEnabled(bool enabled) {
    ACTIVE_SESSION.locator_enabled = enabled;
}

void SetUDKLocked(bool locked) {
    ACTIVE_SESSION.programmable_keys.udk_locked = locked;
}

void GetDeviceAttributes(char* primary, char* secondary, size_t buffer_size) {
    if (primary) {
        strncpy(primary, ACTIVE_SESSION.device_attributes, buffer_size - 1);
        primary[buffer_size - 1] = '\0';
    }
    if (secondary) {
        strncpy(secondary, ACTIVE_SESSION.secondary_attributes, buffer_size - 1);
        secondary[buffer_size - 1] = '\0';
    }
}

int GetPipelineCount(void) {
    return ACTIVE_SESSION.pipeline_count;
}

bool IsPipelineOverflow(void) {
    return ACTIVE_SESSION.pipeline_overflow;
}

// Fix the stubs
void DefineRectangle(int top, int left, int bottom, int right) {
    // Store rectangle definition for later operations
    (void)top; (void)left; (void)bottom; (void)right;
}

void ExecuteRectangularOperation(RectOperation op, const EnhancedTermChar* fill_char) {
    (void)op; (void)fill_char;
}

void SelectCharacterSet(int gset, CharacterSet charset) {
    switch (gset) {
        case 0: ACTIVE_SESSION.charset.g0 = charset; break;
        case 1: ACTIVE_SESSION.charset.g1 = charset; break;
        case 2: ACTIVE_SESSION.charset.g2 = charset; break;
        case 3: ACTIVE_SESSION.charset.g3 = charset; break;
    }
}

void SetCharacterSet(CharacterSet charset) {
    ACTIVE_SESSION.charset.g0 = charset;
    ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g0;
}

void LoadSoftFont(const unsigned char* font_data, int char_start, int char_count) {
    (void)font_data; (void)char_start; (void)char_count;
    // Soft font loading not fully implemented
}

void SelectSoftFont(bool enable) {
    ACTIVE_SESSION.soft_font.active = enable;
}

void SetKeyboardMode(const char* mode, bool enable) {
    if (strcmp(mode, "application") == 0) {
        ACTIVE_SESSION.vt_keyboard.application_mode = enable;
    } else if (strcmp(mode, "cursor") == 0) {
        ACTIVE_SESSION.vt_keyboard.cursor_key_mode = enable;
    } else if (strcmp(mode, "keypad") == 0) {
        ACTIVE_SESSION.vt_keyboard.keypad_mode = enable;
    } else if (strcmp(mode, "meta_escape") == 0) {
        ACTIVE_SESSION.vt_keyboard.meta_sends_escape = enable;
    }
}

void DefineFunctionKey(int key_num, const char* sequence) {
    if (key_num >= 1 && key_num <= 24 && sequence) {
        strncpy(ACTIVE_SESSION.vt_keyboard.function_keys[key_num - 1], sequence, 31);
        ACTIVE_SESSION.vt_keyboard.function_keys[key_num - 1][31] = '\0';
    }
}


void HandleControlKey(VTKeyEvent* event) {
    // Handle Ctrl+key combinations
    if (event->key_code >= SIT_KEY_A && event->key_code <= SIT_KEY_Z) {
        // Ctrl+A = 0x01, Ctrl+B = 0x02, etc.
        int ctrl_char = event->key_code - SIT_KEY_A + 1;
        event->sequence[0] = (char)ctrl_char;
        event->sequence[1] = '\0';
    } else {
        switch (event->key_code) {
            case SIT_KEY_SPACE:      event->sequence[0] = 0x00; break; // Ctrl+Space = NUL
            case SIT_KEY_LEFT_BRACKET:  event->sequence[0] = 0x1B; break; // Ctrl+[ = ESC
            case SIT_KEY_BACKSLASH:  event->sequence[0] = 0x1C; break; // Ctrl+\ = FS
            case SIT_KEY_RIGHT_BRACKET: event->sequence[0] = 0x1D; break; // Ctrl+] = GS
            case SIT_KEY_GRAVE_ACCENT:      event->sequence[0] = 0x1E; break; // Ctrl+^ = RS
            case SIT_KEY_MINUS:      event->sequence[0] = 0x1F; break; // Ctrl+_ = US
            default:
                event->sequence[0] = '\0';
                break;
        }

        if (event->sequence[0] != '\0') {
            event->sequence[1] = '\0';
        }
    }
}

void HandleAltKey(VTKeyEvent* event) {
    // Alt+key sends ESC followed by the key
    if (event->key_code >= SIT_KEY_A && event->key_code <= SIT_KEY_Z) {
        char letter = 'a' + (event->key_code - SIT_KEY_A);
        if (event->shift) {
            letter = 'A' + (event->key_code - SIT_KEY_A);
        }
        snprintf(event->sequence, sizeof(event->sequence), "\x1B%c", letter);
    } else if (event->key_code >= SIT_KEY_0 && event->key_code <= SIT_KEY_9) {
        char digit = '0' + (event->key_code - SIT_KEY_0);
        snprintf(event->sequence, sizeof(event->sequence), "\x1B%c", digit);
    } else {
        // For other keys, just send ESC followed by the normal character
        event->sequence[0] = '\0'; // Will be handled elsewhere
    }
}

void GenerateVTSequence(VTKeyEvent* event) {
    // Clear sequence
    memset(event->sequence, 0, sizeof(event->sequence));

    // Handle special keys first
    switch (event->key_code) {
        // Arrow keys
        case SIT_KEY_UP:
            if (ACTIVE_SESSION.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOA"); // Application mode
            } else {
                strcpy(event->sequence, "\x1B[A"); // Normal mode
            }
            break;

        case SIT_KEY_DOWN:
            if (ACTIVE_SESSION.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOB");
            } else {
                strcpy(event->sequence, "\x1B[B");
            }
            break;

        case SIT_KEY_RIGHT:
            if (ACTIVE_SESSION.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOC");
            } else {
                strcpy(event->sequence, "\x1B[C");
            }
            break;

        case SIT_KEY_LEFT:
            if (ACTIVE_SESSION.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOD");
            } else {
                strcpy(event->sequence, "\x1B[D");
            }
            break;

        // Function keys
        case SIT_KEY_F1:  strcpy(event->sequence, "\x1BOP"); break;
        case SIT_KEY_F2:  strcpy(event->sequence, "\x1BOQ"); break;
        case SIT_KEY_F3:  strcpy(event->sequence, "\x1BOR"); break;
        case SIT_KEY_F4:  strcpy(event->sequence, "\x1BOS"); break;
        case SIT_KEY_F5:  strcpy(event->sequence, "\x1B[15~"); break;
        case SIT_KEY_F6:  strcpy(event->sequence, "\x1B[17~"); break;
        case SIT_KEY_F7:  strcpy(event->sequence, "\x1B[18~"); break;
        case SIT_KEY_F8:  strcpy(event->sequence, "\x1B[19~"); break;
        case SIT_KEY_F9:  strcpy(event->sequence, "\x1B[20~"); break;
        case SIT_KEY_F10: strcpy(event->sequence, "\x1B[21~"); break;
        case SIT_KEY_F11: strcpy(event->sequence, "\x1B[23~"); break;
        case SIT_KEY_F12: strcpy(event->sequence, "\x1B[24~"); break;

        // Home/End
        case SIT_KEY_HOME:
            if (ACTIVE_SESSION.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOH");
            } else {
                strcpy(event->sequence, "\x1B[H");
            }
            break;

        case SIT_KEY_END:
            if (ACTIVE_SESSION.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOF");
            } else {
                strcpy(event->sequence, "\x1B[F");
            }
            break;

        // Page Up/Down
        case SIT_KEY_PAGE_UP:   strcpy(event->sequence, "\x1B[5~"); break;
        case SIT_KEY_PAGE_DOWN: strcpy(event->sequence, "\x1B[6~"); break;

        // Insert/Delete
        case SIT_KEY_INSERT: strcpy(event->sequence, "\x1B[2~"); break;
        case SIT_KEY_DELETE: strcpy(event->sequence, "\x1B[3~"); break;

        // Control characters
        case SIT_KEY_ENTER:     strcpy(event->sequence, "\r"); break;
        case SIT_KEY_TAB:       strcpy(event->sequence, "\t"); break;
        case SIT_KEY_BACKSPACE: strcpy(event->sequence, "\b"); break;
        case SIT_KEY_ESCAPE:    strcpy(event->sequence, "\x1B"); break;

        // Keypad (when in application mode)
        case SIT_KEY_KP_0: case SIT_KEY_KP_1: case SIT_KEY_KP_2: case SIT_KEY_KP_3: case SIT_KEY_KP_4:
        case SIT_KEY_KP_5: case SIT_KEY_KP_6: case SIT_KEY_KP_7: case SIT_KEY_KP_8: case SIT_KEY_KP_9:
            if (ACTIVE_SESSION.vt_keyboard.keypad_mode) {
                snprintf(event->sequence, sizeof(event->sequence), "\x1BO%c",
                        'p' + (event->key_code - SIT_KEY_KP_0));
            } else {
                snprintf(event->sequence, sizeof(event->sequence), "%c",
                        '0' + (event->key_code - SIT_KEY_KP_0));
            }
            break;

        case SIT_KEY_KP_DECIMAL:
            strcpy(event->sequence, ACTIVE_SESSION.vt_keyboard.keypad_mode ? "\x1BOn" : ".");
            break;

        case SIT_KEY_KP_ENTER:
            strcpy(event->sequence, ACTIVE_SESSION.vt_keyboard.keypad_mode ? "\x1BOM" : "\r");
            break;

        case SIT_KEY_KP_ADD:
            strcpy(event->sequence, ACTIVE_SESSION.vt_keyboard.keypad_mode ? "\x1BOk" : "+");
            break;

        case SIT_KEY_KP_SUBTRACT:
            strcpy(event->sequence, ACTIVE_SESSION.vt_keyboard.keypad_mode ? "\x1BOm" : "-");
            break;

        case SIT_KEY_KP_MULTIPLY:
            strcpy(event->sequence, ACTIVE_SESSION.vt_keyboard.keypad_mode ? "\x1BOj" : "*");
            break;

        case SIT_KEY_KP_DIVIDE:
            strcpy(event->sequence, ACTIVE_SESSION.vt_keyboard.keypad_mode ? "\x1BOo" : "/");
            break;

        default:
            // Handle regular keys with modifiers
            if (event->ctrl) {
                HandleControlKey(event);
            } else if (event->alt && ACTIVE_SESSION.vt_keyboard.meta_sends_escape) {
                HandleAltKey(event);
            } else {
                // Regular character - will be handled by GetCharPressed
                event->sequence[0] = '\0';
            }
            break;
    }
}

// Internal function to process keyboard input and enqueue events

// Internal function to process keyboard input and enqueue events
void UpdateVTKeyboard(void) {
    double current_time = SituationTimerGetTime();

    // Process Situation key presses - SKIP PRINTABLE ASCII KEYS
    int rk;
    while ((rk = SituationGetKeyPressed()) != 0) {
        // First, check if this key is a User-Defined Key (UDK)
        bool udk_found = false;
        for (size_t i = 0; i < ACTIVE_SESSION.programmable_keys.count; i++) {
            if (ACTIVE_SESSION.programmable_keys.keys[i].key_code == rk && ACTIVE_SESSION.programmable_keys.keys[i].active) {
                if (ACTIVE_SESSION.vt_keyboard.buffer_count < KEY_EVENT_BUFFER_SIZE) {
                    VTKeyEvent* vt_event = &ACTIVE_SESSION.vt_keyboard.buffer[ACTIVE_SESSION.vt_keyboard.buffer_head];
                    memset(vt_event, 0, sizeof(VTKeyEvent));
                    vt_event->key_code = rk;
                    vt_event->timestamp = current_time;
                    vt_event->priority = KEY_PRIORITY_HIGH; // UDKs get high priority

                    // Copy the user-defined sequence
                    size_t len = ACTIVE_SESSION.programmable_keys.keys[i].sequence_length;
                    if (len >= sizeof(vt_event->sequence)) {
                        len = sizeof(vt_event->sequence) - 1;
                    }
                    memcpy(vt_event->sequence, ACTIVE_SESSION.programmable_keys.keys[i].sequence, len);
                    vt_event->sequence[len] = '\0';

                    ACTIVE_SESSION.vt_keyboard.buffer_head = (ACTIVE_SESSION.vt_keyboard.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
                    ACTIVE_SESSION.vt_keyboard.buffer_count++;
                    ACTIVE_SESSION.vt_keyboard.total_events++;
                    udk_found = true;
                } else {
                    ACTIVE_SESSION.vt_keyboard.dropped_events++;
                }
                break; // Stop after finding the first match
            }
        }
        if (udk_found) continue; // If UDK was handled, skip default processing for this key

        // Determine modifier state correctly (IsKeyDown for held keys)
        bool ctrl = SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL);
        bool alt = SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT);

        // Skip printable ASCII (32-126) because they are handled by GetCharPressed
        // BUT we must allow them if CTRL or ALT is pressed, as they generate sequences (e.g. Ctrl+C)
        if (rk >= 32 && rk <= 126 && !ctrl && !alt) continue;

        if (ACTIVE_SESSION.vt_keyboard.buffer_count < KEY_EVENT_BUFFER_SIZE) {
            VTKeyEvent* vt_event = &ACTIVE_SESSION.vt_keyboard.buffer[ACTIVE_SESSION.vt_keyboard.buffer_head];
            memset(vt_event, 0, sizeof(VTKeyEvent));
            vt_event->key_code = rk;
            vt_event->timestamp = current_time;
            vt_event->priority = KEY_PRIORITY_NORMAL;
            vt_event->ctrl = ctrl;
            vt_event->shift = SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT);
            vt_event->alt = alt;

            // Special handling for printable keys with modifiers
            if (rk >= 32 && rk <= 126) {
                if (ctrl) HandleControlKey(vt_event);
                else if (alt) HandleAltKey(vt_event);
            }
            else {
                // Handle Scrollback (Shift + PageUp/Down) - Local Action, No Sequence
                if (vt_event->shift && (rk == SIT_KEY_PAGE_UP || rk == SIT_KEY_PAGE_DOWN)) {
                    if (rk == SIT_KEY_PAGE_UP) {
                        ACTIVE_SESSION.view_offset += DEFAULT_TERM_HEIGHT / 2;
                    } else {
                        ACTIVE_SESSION.view_offset -= DEFAULT_TERM_HEIGHT / 2;
                    }

                    // Clamp view_offset
                    if (ACTIVE_SESSION.view_offset < 0) ACTIVE_SESSION.view_offset = 0;
                    int max_offset = ACTIVE_SESSION.buffer_height - DEFAULT_TERM_HEIGHT;
                    if (ACTIVE_SESSION.view_offset > max_offset) ACTIVE_SESSION.view_offset = max_offset;

                    // Invalidate screen to redraw with new offset
                    for (int i=0; i<DEFAULT_TERM_HEIGHT; i++) ACTIVE_SESSION.row_dirty[i] = true;

                    // Do NOT increment buffer_count, we consumed the event locally.
                    continue;
                }

                // Generate VT sequence for special keys only
                switch (rk) {
                case SIT_KEY_UP:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), ACTIVE_SESSION.vt_keyboard.cursor_key_mode ? "\x1BOA" : "\x1B[A");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5A");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3A");
                    break;
                case SIT_KEY_DOWN:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), ACTIVE_SESSION.vt_keyboard.cursor_key_mode ? "\x1BOB" : "\x1B[B");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5B");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3B");
                    break;
                case SIT_KEY_RIGHT:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), ACTIVE_SESSION.vt_keyboard.cursor_key_mode ? "\x1BOC" : "\x1B[C");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5C");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3C");
                    break;
                case SIT_KEY_LEFT:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), ACTIVE_SESSION.vt_keyboard.cursor_key_mode ? "\x1BOD" : "\x1B[D");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5D");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3D");
                    break;
                case SIT_KEY_F1: case SIT_KEY_F2: case SIT_KEY_F3: case SIT_KEY_F4:
                case SIT_KEY_F5: case SIT_KEY_F6: case SIT_KEY_F7: case SIT_KEY_F8:
                case SIT_KEY_F9: case SIT_KEY_F10: case SIT_KEY_F11: case SIT_KEY_F12:
                    strncpy(vt_event->sequence, ACTIVE_SESSION.vt_keyboard.function_keys[rk - SIT_KEY_F1], sizeof(vt_event->sequence));
                    break;
                case SIT_KEY_ENTER:
                    vt_event->sequence[0] = ACTIVE_SESSION.ansi_modes.line_feed_new_line ? '\r' : '\n';
                    vt_event->sequence[1] = '\0';
                    break;
                case SIT_KEY_BACKSPACE:
                    vt_event->sequence[0] = ACTIVE_SESSION.vt_keyboard.backarrow_sends_bs ? '\b' : '\x7F';
                    vt_event->sequence[1] = '\0';
                    break;
                case SIT_KEY_DELETE:
                    vt_event->sequence[0] = ACTIVE_SESSION.vt_keyboard.delete_sends_del ? '\x7F' : '\b';
                    vt_event->sequence[1] = '\0';
                    break;
                case SIT_KEY_TAB:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\t");
                    break;
                case SIT_KEY_ESCAPE:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B");
                    break;
                    default:
                        continue; // Ignore unhandled special keys
                }
            }

            if (vt_event->sequence[0] != '\0') {
                ACTIVE_SESSION.vt_keyboard.buffer_head = (ACTIVE_SESSION.vt_keyboard.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
                ACTIVE_SESSION.vt_keyboard.buffer_count++;
                ACTIVE_SESSION.vt_keyboard.total_events++;
            }
        } else {
            ACTIVE_SESSION.vt_keyboard.dropped_events++;
        }
    }

    // Process Unicode characters - THIS HANDLES ALL PRINTABLE CHARACTERS
    int ch_unicode;
    while ((ch_unicode = SituationGetCharPressed()) != 0) {
        if (ACTIVE_SESSION.vt_keyboard.buffer_count < KEY_EVENT_BUFFER_SIZE) {
            VTKeyEvent* vt_event = &ACTIVE_SESSION.vt_keyboard.buffer[ACTIVE_SESSION.vt_keyboard.buffer_head];
            memset(vt_event, 0, sizeof(VTKeyEvent));
            vt_event->key_code = ch_unicode;
            vt_event->timestamp = current_time;
            vt_event->priority = KEY_PRIORITY_NORMAL;
            vt_event->is_repeat = false; // SituationGetCharPressed() doesn't distinguish repeats
            vt_event->ctrl = SituationIsKeyPressed(SIT_KEY_LEFT_CONTROL) || SituationIsKeyPressed(SIT_KEY_RIGHT_CONTROL);
            vt_event->alt = SituationIsKeyPressed(SIT_KEY_LEFT_ALT) || SituationIsKeyPressed(SIT_KEY_RIGHT_ALT);
            vt_event->shift = SituationIsKeyPressed(SIT_KEY_LEFT_SHIFT) || SituationIsKeyPressed(SIT_KEY_RIGHT_SHIFT);

            // Handle Ctrl key combinations
            if (vt_event->ctrl && ch_unicode >= 1 && ch_unicode <= 26) {
                // Already a control character
                vt_event->sequence[0] = (char)ch_unicode;
                vt_event->sequence[1] = '\0';
            } else if (vt_event->ctrl && ch_unicode >= 'a' && ch_unicode <= 'z') {
                // Convert to control character
                vt_event->sequence[0] = (char)(ch_unicode - 'a' + 1);
                vt_event->sequence[1] = '\0';
            } else if (vt_event->ctrl && ch_unicode >= 'A' && ch_unicode <= 'Z') {
                // Convert to control character
                vt_event->sequence[0] = (char)(ch_unicode - 'A' + 1);
                vt_event->sequence[1] = '\0';
            } else if (vt_event->alt && ACTIVE_SESSION.vt_keyboard.meta_sends_escape && !vt_event->ctrl) {
                // Alt+key sends ESC prefix
                vt_event->sequence[0] = '';
                if (ch_unicode < 128) {
                    vt_event->sequence[1] = (char)ch_unicode;
                    vt_event->sequence[2] = '\0';
                } else {
                    // UTF-8 encoding for unicode
                    if (ch_unicode < 0x80) { // 1-byte
                        vt_event->sequence[1] = (char)ch_unicode;
                        vt_event->sequence[2] = '\0';
                    } else if (ch_unicode < 0x800) { // 2-byte
                        vt_event->sequence[1] = (char)(0xC0 | (ch_unicode >> 6));
                        vt_event->sequence[2] = (char)(0x80 | (ch_unicode & 0x3F));
                        vt_event->sequence[3] = '\0';
                    } else if (ch_unicode < 0x10000) { // 3-byte
                        vt_event->sequence[1] = (char)(0xE0 | (ch_unicode >> 12));
                        vt_event->sequence[2] = (char)(0x80 | ((ch_unicode >> 6) & 0x3F));
                        vt_event->sequence[3] = (char)(0x80 | (ch_unicode & 0x3F));
                        vt_event->sequence[4] = '\0';
                    } else { // 4-byte
                        vt_event->sequence[1] = (char)(0xF0 | (ch_unicode >> 18));
                        vt_event->sequence[2] = (char)(0x80 | ((ch_unicode >> 12) & 0x3F));
                        vt_event->sequence[3] = (char)(0x80 | ((ch_unicode >> 6) & 0x3F));
                        vt_event->sequence[4] = (char)(0x80 | (ch_unicode & 0x3F));
                        vt_event->sequence[5] = '\0';
                    }
                }
            } else {
                // Normal character - encode as UTF-8
                if (ch_unicode < 0x80) { // 1-byte
                    vt_event->sequence[0] = (char)ch_unicode;
                    vt_event->sequence[1] = '\0';
                } else if (ch_unicode < 0x800) { // 2-byte
                    vt_event->sequence[0] = (char)(0xC0 | (ch_unicode >> 6));
                    vt_event->sequence[1] = (char)(0x80 | (ch_unicode & 0x3F));
                    vt_event->sequence[2] = '\0';
                } else if (ch_unicode < 0x10000) { // 3-byte
                    vt_event->sequence[0] = (char)(0xE0 | (ch_unicode >> 12));
                    vt_event->sequence[1] = (char)(0x80 | ((ch_unicode >> 6) & 0x3F));
                    vt_event->sequence[2] = (char)(0x80 | (ch_unicode & 0x3F));
                    vt_event->sequence[3] = '\0';
                } else { // 4-byte
                    vt_event->sequence[0] = (char)(0xF0 | (ch_unicode >> 18));
                    vt_event->sequence[1] = (char)(0x80 | ((ch_unicode >> 12) & 0x3F));
                    vt_event->sequence[2] = (char)(0x80 | ((ch_unicode >> 6) & 0x3F));
                    vt_event->sequence[3] = (char)(0x80 | (ch_unicode & 0x3F));
                    vt_event->sequence[4] = '\0';
                }
            }

            if (vt_event->sequence[0] != '\0') {
                ACTIVE_SESSION.vt_keyboard.buffer_head = (ACTIVE_SESSION.vt_keyboard.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
                ACTIVE_SESSION.vt_keyboard.buffer_count++;
                ACTIVE_SESSION.vt_keyboard.total_events++;
            }
        } else {
            ACTIVE_SESSION.vt_keyboard.dropped_events++;
        }
    }
}


bool GetKeyEvent(KeyEvent* event) {
    return GetVTKeyEvent(event);
}

void SetPipelineTargetFPS(int fps) {
    if (fps > 0) {
        ACTIVE_SESSION.VTperformance.target_frame_time = 1.0 / fps;
        ACTIVE_SESSION.VTperformance.time_budget = ACTIVE_SESSION.VTperformance.target_frame_time * 0.3;
    }
}

void SetPipelineTimeBudget(double pct) {
    if (pct > 0.0 && pct <= 1.0) {
        ACTIVE_SESSION.VTperformance.time_budget = ACTIVE_SESSION.VTperformance.target_frame_time * pct;
    }
}

TerminalStatus GetTerminalStatus(void) {
    TerminalStatus status = {0};
    status.pipeline_usage = ACTIVE_SESSION.pipeline_count;
    status.key_usage = ACTIVE_SESSION.vt_keyboard.buffer_count;
    status.overflow_detected = ACTIVE_SESSION.pipeline_overflow;
    status.avg_process_time = ACTIVE_SESSION.VTperformance.avg_process_time;
    return status;
}

void ShowBufferDiagnostics(void) {
    TerminalStatus status = GetTerminalStatus();
    PipelineWriteFormat("=== Buffer Diagnostics ===\n");
    PipelineWriteFormat("Pipeline: %zu/%d bytes\n", status.pipeline_usage, (int)sizeof(ACTIVE_SESSION.input_pipeline));
    PipelineWriteFormat("Keyboard: %zu events\n", status.key_usage);
    PipelineWriteFormat("Overflow: %s\n", status.overflow_detected ? "YES" : "No");
    PipelineWriteFormat("Avg Process Time: %.6f ms\n", status.avg_process_time * 1000.0);
}

void VTSwapScreenBuffer(void) {
    // Swap pointers
    EnhancedTermChar* temp_buf = ACTIVE_SESSION.screen_buffer;
    ACTIVE_SESSION.screen_buffer = ACTIVE_SESSION.alt_buffer;
    ACTIVE_SESSION.alt_buffer = temp_buf;

    // Swap dimensions/metadata
    // For now, only buffer_height differs (Main has scrollback, Alt usually doesn't).
    // But since we allocate Alt buffer with DEFAULT_TERM_HEIGHT, we must handle this.
    // However, if we swap, the "active" buffer height must reflect what we are pointing to.

    // We didn't add "alt_buffer_height" to the struct, assuming Alt is always screen size.
    // But if we swap, ACTIVE_SESSION.screen_buffer now points to a small buffer.
    // So ACTIVE_SESSION.buffer_height must be updated.

    // Problem: We need to store the height of the buffer currently in 'alt_buffer' so we can restore it.
    // Let's assume standard behavior:
    // Main Buffer: Has scrollback (Large).
    // Alt Buffer: No scrollback (Screen Height).

    // Swap heads
    int temp_head = ACTIVE_SESSION.screen_head;
    ACTIVE_SESSION.screen_head = ACTIVE_SESSION.alt_screen_head;
    ACTIVE_SESSION.alt_screen_head = temp_head;

    if (ACTIVE_SESSION.dec_modes.alternate_screen) {
        // Switching BACK to Main Screen
        ACTIVE_SESSION.buffer_height = DEFAULT_TERM_HEIGHT + MAX_SCROLLBACK_LINES;
        ACTIVE_SESSION.dec_modes.alternate_screen = false;

        // Restore view offset (if we want to restore scroll position, otherwise 0)
        // For now, reset to 0 (bottom) is safest and standard behavior.
        // Or restore saved? Let's restore saved for better UX.
        ACTIVE_SESSION.view_offset = ACTIVE_SESSION.saved_view_offset;
    } else {
        // Switching TO Alternate Screen
        ACTIVE_SESSION.buffer_height = DEFAULT_TERM_HEIGHT;
        ACTIVE_SESSION.dec_modes.alternate_screen = true;

        // Save current offset and reset view for Alt screen (which has no scrollback)
        ACTIVE_SESSION.saved_view_offset = ACTIVE_SESSION.view_offset;
        ACTIVE_SESSION.view_offset = 0;
    }

    // Force full redraw
    for (int i=0; i<DEFAULT_TERM_HEIGHT; i++) {
        ACTIVE_SESSION.row_dirty[i] = true;
    }
}

void ProcessPipeline(void) {
    if (ACTIVE_SESSION.pipeline_count == 0) {
        return;
    }

    double start_time = SituationTimerGetTime();
    int chars_processed = 0;
    int target_chars = ACTIVE_SESSION.VTperformance.chars_per_frame;

    // Adaptive processing based on buffer level
    if (ACTIVE_SESSION.pipeline_count > ACTIVE_SESSION.VTperformance.burst_threshold) {
        target_chars *= 2; // Burst mode
        ACTIVE_SESSION.VTperformance.burst_mode = true;
    } else if (ACTIVE_SESSION.pipeline_count < target_chars) {
        target_chars = ACTIVE_SESSION.pipeline_count; // Process all remaining
        ACTIVE_SESSION.VTperformance.burst_mode = false;
    }

    while (chars_processed < target_chars && ACTIVE_SESSION.pipeline_count > 0) {
        // Check time budget
        if (SituationTimerGetTime() - start_time > ACTIVE_SESSION.VTperformance.time_budget) {
            break;
        }

        unsigned char ch = ACTIVE_SESSION.input_pipeline[ACTIVE_SESSION.pipeline_tail];
        ACTIVE_SESSION.pipeline_tail = (ACTIVE_SESSION.pipeline_tail + 1) % sizeof(ACTIVE_SESSION.input_pipeline);
        ACTIVE_SESSION.pipeline_count--;

        ProcessChar(ch);
        chars_processed++;
    }

    // Update performance metrics
    if (chars_processed > 0) {
        double total_time = SituationTimerGetTime() - start_time;
        double time_per_char = total_time / chars_processed;
        ACTIVE_SESSION.VTperformance.avg_process_time =
            ACTIVE_SESSION.VTperformance.avg_process_time * 0.9 + time_per_char * 0.1;
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void LogUnsupportedSequence(const char* sequence) {
    if (!ACTIVE_SESSION.options.log_unsupported) return;

    ACTIVE_SESSION.conformance.compliance.unsupported_sequences++;

    // Use strcpy instead of strncpy to avoid truncation warnings
    size_t len = strlen(sequence);
    if (len >= sizeof(ACTIVE_SESSION.conformance.compliance.last_unsupported)) {
        len = sizeof(ACTIVE_SESSION.conformance.compliance.last_unsupported) - 1;
    }
    memcpy(ACTIVE_SESSION.conformance.compliance.last_unsupported, sequence, len);
    ACTIVE_SESSION.conformance.compliance.last_unsupported[len] = '\0';

    if (ACTIVE_SESSION.options.debug_sequences) {
        char debug_msg[128];
        snprintf(debug_msg, sizeof(debug_msg),
                "Unsupported: %s (total: %d)\n",
                sequence, ACTIVE_SESSION.conformance.compliance.unsupported_sequences);

        if (response_callback) {
            response_callback(debug_msg, strlen(debug_msg));
        }
    }
}

// =============================================================================
// PARAMETER PARSING UTILITIES
// =============================================================================

int ParseCSIParams(const char* params, int* out_params, int max_params) {
    ACTIVE_SESSION.param_count = 0;
    memset(ACTIVE_SESSION.escape_params, 0, sizeof(ACTIVE_SESSION.escape_params));

    if (!params || strlen(params) == 0) {
        return 0;
    }

    const char* parse_start = params;
    if (params[0] == '?') {
        parse_start = params + 1;
    }

    if (strlen(parse_start) == 0) {
        return 0;
    }

    char param_buffer[512];
    strncpy(param_buffer, parse_start, sizeof(param_buffer) - 1);
    param_buffer[sizeof(param_buffer) - 1] = '\0';

    char* saveptr;
    char* token = strtok_r(param_buffer, ";", &saveptr);

    while (token != NULL && ACTIVE_SESSION.param_count < max_params) {
        if (strlen(token) == 0) {
            ACTIVE_SESSION.escape_params[ACTIVE_SESSION.param_count] = 0;
        } else {
            int value = atoi(token);
            ACTIVE_SESSION.escape_params[ACTIVE_SESSION.param_count] = (value >= 0) ? value : 0;
        }
        if (out_params) {
            out_params[ACTIVE_SESSION.param_count] = ACTIVE_SESSION.escape_params[ACTIVE_SESSION.param_count];
        }
        ACTIVE_SESSION.param_count++;
        token = strtok_r(NULL, ";", &saveptr);
    }
    return ACTIVE_SESSION.param_count;
}

static void ClearCSIParams(void) {
    ACTIVE_SESSION.escape_buffer[0] = '\0';
    ACTIVE_SESSION.escape_pos = 0;
    ACTIVE_SESSION.param_count = 0;
    memset(ACTIVE_SESSION.escape_params, 0, sizeof(ACTIVE_SESSION.escape_params));
}

void ProcessSixelSTChar(unsigned char ch) {
    if (ch == '\\') { // This is ST
        ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
        // Finalize sixel image size
        ACTIVE_SESSION.sixel.width = ACTIVE_SESSION.sixel.max_x;
        ACTIVE_SESSION.sixel.height = ACTIVE_SESSION.sixel.max_y;
    } else {
        // Not an ST, go back to parsing sixel data
        ACTIVE_SESSION.parse_state = PARSE_SIXEL;
        ProcessSixelChar('\x1B'); // Process the ESC that led here
        ProcessSixelChar(ch);     // Process the current char
    }
}

int GetCSIParam(int index, int default_value) {
    if (index >= 0 && index < ACTIVE_SESSION.param_count) {
        return (ACTIVE_SESSION.escape_params[index] == 0) ? default_value : ACTIVE_SESSION.escape_params[index];
    }
    return default_value;
}


// =============================================================================
// CURSOR MOVEMENT IMPLEMENTATIONS
// =============================================================================

void ExecuteCUU(void) { // Cursor Up
    int n = GetCSIParam(0, 1);
    int new_y = ACTIVE_SESSION.cursor.y - n;

    if (ACTIVE_SESSION.dec_modes.origin_mode) {
        ACTIVE_SESSION.cursor.y = (new_y < ACTIVE_SESSION.scroll_top) ? ACTIVE_SESSION.scroll_top : new_y;
    } else {
        ACTIVE_SESSION.cursor.y = (new_y < 0) ? 0 : new_y;
    }
}

void ExecuteCUD(void) { // Cursor Down
    int n = GetCSIParam(0, 1);
    int new_y = ACTIVE_SESSION.cursor.y + n;

    if (ACTIVE_SESSION.dec_modes.origin_mode) {
        ACTIVE_SESSION.cursor.y = (new_y > ACTIVE_SESSION.scroll_bottom) ? ACTIVE_SESSION.scroll_bottom : new_y;
    } else {
        ACTIVE_SESSION.cursor.y = (new_y >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : new_y;
    }
}

void ExecuteCUF(void) { // Cursor Forward
    int n = GetCSIParam(0, 1);
    ACTIVE_SESSION.cursor.x = (ACTIVE_SESSION.cursor.x + n >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : ACTIVE_SESSION.cursor.x + n;
}

void ExecuteCUB(void) { // Cursor Back
    int n = GetCSIParam(0, 1);
    ACTIVE_SESSION.cursor.x = (ACTIVE_SESSION.cursor.x - n < 0) ? 0 : ACTIVE_SESSION.cursor.x - n;
}

void ExecuteCNL(void) { // Cursor Next Line
    int n = GetCSIParam(0, 1);
    ACTIVE_SESSION.cursor.y = (ACTIVE_SESSION.cursor.y + n >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : ACTIVE_SESSION.cursor.y + n;
    ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
}

void ExecuteCPL(void) { // Cursor Previous Line
    int n = GetCSIParam(0, 1);
    ACTIVE_SESSION.cursor.y = (ACTIVE_SESSION.cursor.y - n < 0) ? 0 : ACTIVE_SESSION.cursor.y - n;
    ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
}

void ExecuteCHA(void) { // Cursor Horizontal Absolute
    int n = GetCSIParam(0, 1) - 1; // Convert to 0-based
    ACTIVE_SESSION.cursor.x = (n < 0) ? 0 : (n >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : n;
}

void ExecuteCUP(void) { // Cursor Position
    int row = GetCSIParam(0, 1) - 1; // Convert to 0-based
    int col = GetCSIParam(1, 1) - 1;

    if (ACTIVE_SESSION.dec_modes.origin_mode) {
        row += ACTIVE_SESSION.scroll_top;
        col += ACTIVE_SESSION.left_margin;
    }

    ACTIVE_SESSION.cursor.y = (row < 0) ? 0 : (row >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : row;
    ACTIVE_SESSION.cursor.x = (col < 0) ? 0 : (col >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : col;

    // Clamp to scrolling region if in origin mode
    if (ACTIVE_SESSION.dec_modes.origin_mode) {
        if (ACTIVE_SESSION.cursor.y < ACTIVE_SESSION.scroll_top) ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_top;
        if (ACTIVE_SESSION.cursor.y > ACTIVE_SESSION.scroll_bottom) ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_bottom;
        if (ACTIVE_SESSION.cursor.x < ACTIVE_SESSION.left_margin) ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
        if (ACTIVE_SESSION.cursor.x > ACTIVE_SESSION.right_margin) ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.right_margin;
    }
}

void ExecuteVPA(void) { // Vertical Position Absolute
    int n = GetCSIParam(0, 1) - 1; // Convert to 0-based

    if (ACTIVE_SESSION.dec_modes.origin_mode) {
        n += ACTIVE_SESSION.scroll_top;
        ACTIVE_SESSION.cursor.y = (n < ACTIVE_SESSION.scroll_top) ? ACTIVE_SESSION.scroll_top :
                           (n > ACTIVE_SESSION.scroll_bottom) ? ACTIVE_SESSION.scroll_bottom : n;
    } else {
        ACTIVE_SESSION.cursor.y = (n < 0) ? 0 : (n >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : n;
    }
}

// =============================================================================
// ERASING IMPLEMENTATIONS
// =============================================================================

void ExecuteED(bool private_mode) { // Erase in Display
    int n = GetCSIParam(0, 0);

    switch (n) {
        case 0: // Clear from cursor to end of screen
            // Clear current line from cursor
            for (int x = ACTIVE_SESSION.cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(cell);
            }
            // Clear remaining lines
            for (int y = ACTIVE_SESSION.cursor.y + 1; y < DEFAULT_TERM_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, y, x);
                    if (private_mode && cell->protected_cell) continue;
                    ClearCell(cell);
                }
            }
            break;

        case 1: // Clear from beginning of screen to cursor
            // Clear lines before cursor
            for (int y = 0; y < ACTIVE_SESSION.cursor.y; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, y, x);
                    if (private_mode && cell->protected_cell) continue;
                    ClearCell(cell);
                }
            }
            // Clear current line up to cursor
            for (int x = 0; x <= ACTIVE_SESSION.cursor.x; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(cell);
            }
            break;

        case 2: // Clear entire screen
        case 3: // Clear entire screen and scrollback (xterm extension)
            for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, y, x);
                    if (private_mode && cell->protected_cell) continue;
                    ClearCell(cell);
                }
            }
            break;

        default:
            LogUnsupportedSequence("Unknown ED parameter");
            break;
    }
}

void ExecuteEL(bool private_mode) { // Erase in Line
    int n = GetCSIParam(0, 0);

    switch (n) {
        case 0: // Clear from cursor to end of line
            for (int x = ACTIVE_SESSION.cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(cell);
            }
            break;

        case 1: // Clear from beginning of line to cursor
            for (int x = 0; x <= ACTIVE_SESSION.cursor.x; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(cell);
            }
            break;

        case 2: // Clear entire line
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(cell);
            }
            break;

        default:
            LogUnsupportedSequence("Unknown EL parameter");
            break;
    }
}

void ExecuteECH(void) { // Erase Character
    int n = GetCSIParam(0, 1);

    for (int i = 0; i < n && ACTIVE_SESSION.cursor.x + i < DEFAULT_TERM_WIDTH; i++) {
        ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, ACTIVE_SESSION.cursor.x + i));
    }
}

// =============================================================================
// INSERTION AND DELETION IMPLEMENTATIONS
// =============================================================================

void ExecuteIL(void) { // Insert Line
    int n = GetCSIParam(0, 1);
    InsertLinesAt(ACTIVE_SESSION.cursor.y, n);
}

void ExecuteDL(void) { // Delete Line
    int n = GetCSIParam(0, 1);
    DeleteLinesAt(ACTIVE_SESSION.cursor.y, n);
}

void ExecuteICH(void) { // Insert Character
    int n = GetCSIParam(0, 1);
    InsertCharactersAt(ACTIVE_SESSION.cursor.y, ACTIVE_SESSION.cursor.x, n);
}

void ExecuteDCH(void) { // Delete Character
    int n = GetCSIParam(0, 1);
    DeleteCharactersAt(ACTIVE_SESSION.cursor.y, ACTIVE_SESSION.cursor.x, n);
}

void ExecuteREP(void) { // Repeat Preceding Graphic Character
    int n = GetCSIParam(0, 1);
    if (n < 1) n = 1;
    if (ACTIVE_SESSION.last_char > 0) {
        for (int i = 0; i < n; i++) {
            if (ACTIVE_SESSION.cursor.x > ACTIVE_SESSION.right_margin) {
                if (ACTIVE_SESSION.dec_modes.auto_wrap_mode) {
                    ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
                    ACTIVE_SESSION.cursor.y++;
                    if (ACTIVE_SESSION.cursor.y > ACTIVE_SESSION.scroll_bottom) {
                        ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_bottom;
                        ScrollUpRegion(ACTIVE_SESSION.scroll_top, ACTIVE_SESSION.scroll_bottom, 1);
                    }
                } else {
                    ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.right_margin;
                }
            }
            InsertCharacterAtCursor(ACTIVE_SESSION.last_char);
            ACTIVE_SESSION.cursor.x++;
        }
    }
}

// =============================================================================
// SCROLLING IMPLEMENTATIONS
// =============================================================================

void ExecuteSU(void) { // Scroll Up
    int n = GetCSIParam(0, 1);
    ScrollUpRegion(ACTIVE_SESSION.scroll_top, ACTIVE_SESSION.scroll_bottom, n);
}

void ExecuteSD(void) { // Scroll Down
    int n = GetCSIParam(0, 1);
    ScrollDownRegion(ACTIVE_SESSION.scroll_top, ACTIVE_SESSION.scroll_bottom, n);
}

// =============================================================================
// ENHANCED SGR (SELECT GRAPHIC RENDITION) IMPLEMENTATION
// =============================================================================

int ProcessExtendedColor(ExtendedColor* color, int param_index) {
    int consumed = 0;

    if (param_index + 1 < ACTIVE_SESSION.param_count) {
        int color_type = ACTIVE_SESSION.escape_params[param_index + 1];

        if (color_type == 5 && param_index + 2 < ACTIVE_SESSION.param_count) {
            // 256-color mode: ESC[38;5;n or ESC[48;5;n
            int color_index = ACTIVE_SESSION.escape_params[param_index + 2];
            if (color_index >= 0 && color_index < 256) {
                color->color_mode = 0;
                color->value.index = color_index;
            }
            consumed = 2;

        } else if (color_type == 2 && param_index + 4 < ACTIVE_SESSION.param_count) {
            // True color mode: ESC[38;2;r;g;b or ESC[48;2;r;g;b
            int r = ACTIVE_SESSION.escape_params[param_index + 2] & 0xFF;
            int g = ACTIVE_SESSION.escape_params[param_index + 3] & 0xFF;
            int b = ACTIVE_SESSION.escape_params[param_index + 4] & 0xFF;

            color->color_mode = 1;
            color->value.rgb = (RGB_Color){r, g, b, 255};
            consumed = 4;
        }
    }

    return consumed;
}

void ResetAllAttributes(void) {
    ACTIVE_SESSION.current_fg.color_mode = 0;
    ACTIVE_SESSION.current_fg.value.index = COLOR_WHITE;
    ACTIVE_SESSION.current_bg.color_mode = 0;
    ACTIVE_SESSION.current_bg.value.index = COLOR_BLACK;

    ACTIVE_SESSION.bold_mode = false;
    ACTIVE_SESSION.faint_mode = false;
    ACTIVE_SESSION.italic_mode = false;
    ACTIVE_SESSION.underline_mode = false;
    ACTIVE_SESSION.blink_mode = false;
    ACTIVE_SESSION.reverse_mode = false;
    ACTIVE_SESSION.strikethrough_mode = false;
    ACTIVE_SESSION.conceal_mode = false;
    ACTIVE_SESSION.overline_mode = false;
    ACTIVE_SESSION.double_underline_mode = false;
    ACTIVE_SESSION.protected_mode = false;
}

void ExecuteSGR(void) {
    if (ACTIVE_SESSION.param_count == 0) {
        // Reset all attributes
        ResetAllAttributes();
        return;
    }

    for (int i = 0; i < ACTIVE_SESSION.param_count; i++) {
        int param = ACTIVE_SESSION.escape_params[i];

        switch (param) {
            case 0: // Reset all
                ResetAllAttributes();
                break;

            // Intensity
            case 1: ACTIVE_SESSION.bold_mode = true; break;
            case 2: ACTIVE_SESSION.faint_mode = true; break;
            case 22: ACTIVE_SESSION.bold_mode = ACTIVE_SESSION.faint_mode = false; break;

            // Style
            case 3: ACTIVE_SESSION.italic_mode = true; break;
            case 23: ACTIVE_SESSION.italic_mode = false; break;

            case 4: ACTIVE_SESSION.underline_mode = true; break;
            case 21: ACTIVE_SESSION.double_underline_mode = true; break;
            case 24: ACTIVE_SESSION.underline_mode = ACTIVE_SESSION.double_underline_mode = false; break;

            case 5: case 6: ACTIVE_SESSION.blink_mode = true; break;
            case 25: ACTIVE_SESSION.blink_mode = false; break;

            case 7: ACTIVE_SESSION.reverse_mode = true; break;
            case 27: ACTIVE_SESSION.reverse_mode = false; break;

            case 8: ACTIVE_SESSION.conceal_mode = true; break;
            case 28: ACTIVE_SESSION.conceal_mode = false; break;

            case 9: ACTIVE_SESSION.strikethrough_mode = true; break;
            case 29: ACTIVE_SESSION.strikethrough_mode = false; break;

            case 53: ACTIVE_SESSION.overline_mode = true; break;
            case 55: ACTIVE_SESSION.overline_mode = false; break;

            // Standard colors (30-37, 40-47)
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                ACTIVE_SESSION.current_fg.color_mode = 0;
                ACTIVE_SESSION.current_fg.value.index = param - 30;
                break;

            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                ACTIVE_SESSION.current_bg.color_mode = 0;
                ACTIVE_SESSION.current_bg.value.index = param - 40;
                break;

            // Bright colors (90-97, 100-107)
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                ACTIVE_SESSION.current_fg.color_mode = 0;
                ACTIVE_SESSION.current_fg.value.index = param - 90 + 8;
                break;

            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                ACTIVE_SESSION.current_bg.color_mode = 0;
                ACTIVE_SESSION.current_bg.value.index = param - 100 + 8;
                break;

            // Extended colors
            case 38: // Set foreground color
                i += ProcessExtendedColor(&ACTIVE_SESSION.current_fg, i);
                break;

            case 48: // Set background color
                i += ProcessExtendedColor(&ACTIVE_SESSION.current_bg, i);
                break;

            // Default colors
            case 39:
                ACTIVE_SESSION.current_fg.color_mode = 0;
                ACTIVE_SESSION.current_fg.value.index = COLOR_WHITE;
                break;

            case 49:
                ACTIVE_SESSION.current_bg.color_mode = 0;
                ACTIVE_SESSION.current_bg.value.index = COLOR_BLACK;
                break;

            default:
                if (ACTIVE_SESSION.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown SGR parameter: %d", param);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
    }
}

// =============================================================================
// TERMINAL MODE IMPLEMENTATIONS
// =============================================================================

// Helper function to compute screen buffer checksum (for CSI ?63 n)
static uint32_t ComputeScreenChecksum(int page) {
    uint32_t checksum = 0;
    // Simple CRC16-like checksum for screen buffer
    for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
        for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
            EnhancedTermChar *cell = GetScreenCell(&ACTIVE_SESSION, y, x);
            checksum += cell->ch;
            checksum += (cell->fg_color.color_mode == 0 ? cell->fg_color.value.index : (cell->fg_color.value.rgb.r << 16 | cell->fg_color.value.rgb.g << 8 | cell->fg_color.value.rgb.b));
            checksum += (cell->bg_color.color_mode == 0 ? cell->bg_color.value.index : (cell->bg_color.value.rgb.r << 16 | cell->bg_color.value.rgb.g << 8 | cell->bg_color.value.rgb.b));
            checksum = (checksum >> 16) + (checksum & 0xFFFF);
        }
    }
    return checksum & 0xFFFF;
}

void SwitchScreenBuffer(bool to_alternate) {
    if (!ACTIVE_SESSION.conformance.features.alternate_screen) {
        LogUnsupportedSequence("Alternate screen not supported");
        return;
    }

    // In new Ring Buffer architecture, we swap buffers rather than copy.
    VTSwapScreenBuffer();
    // VTSwapScreenBuffer handles logic if implemented correctly.
    // However, this function `SwitchScreenBuffer` seems to enforce explicit "to_alternate" direction.
    // We should implement it using pointers.

    if (to_alternate && !ACTIVE_SESSION.dec_modes.alternate_screen) {
        VTSwapScreenBuffer(); // Swaps to alt
    } else if (!to_alternate && ACTIVE_SESSION.dec_modes.alternate_screen) {
        VTSwapScreenBuffer(); // Swaps back to main
    }
}

// Set terminal modes internally
// Configures DEC private modes (CSI ? Pm h/l) and ANSI modes (CSI Pm h/l)
static void SetTerminalModeInternal(int mode, bool enable, bool private_mode) {
    if (private_mode) {
        // DEC Private Modes
        switch (mode) {
            case 1: // DECCKM - Cursor Key Mode
                // Enable/disable application cursor keys
                ACTIVE_SESSION.dec_modes.application_cursor_keys = enable;
                ACTIVE_SESSION.vt_keyboard.cursor_key_mode = enable;
                break;

            case 2: // DECANM - ANSI Mode
                // Switch between VT52 and ANSI mode
                if (!enable && ACTIVE_SESSION.conformance.features.vt52_mode) {
                    ACTIVE_SESSION.parse_state = PARSE_VT52;
                }
                break;

            case 3: // DECCOLM - Column Mode
                // Set 132-column mode
                // Note: Actual window resizing is host-dependent and not handled here.
                // Standard requires clearing screen, resetting margins, and homing cursor.
                if (ACTIVE_SESSION.dec_modes.column_mode_132 != enable) {
                    ACTIVE_SESSION.dec_modes.column_mode_132 = enable;

                    // 1. Clear Screen
                    for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                        for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                            ClearCell(GetScreenCell(&ACTIVE_SESSION, y, x));
                        }
                        ACTIVE_SESSION.row_dirty[y] = true;
                    }

                    // 2. Reset Margins
                    ACTIVE_SESSION.scroll_top = 0;
                    ACTIVE_SESSION.scroll_bottom = DEFAULT_TERM_HEIGHT - 1;
                    ACTIVE_SESSION.left_margin = 0;
                    // Set right margin (132 columns = index 131, 80 columns = index 79)
                    ACTIVE_SESSION.right_margin = enable ? 131 : 79;
                    // Safety clamp if DEFAULT_TERM_WIDTH < 132
                    if (ACTIVE_SESSION.right_margin >= DEFAULT_TERM_WIDTH) ACTIVE_SESSION.right_margin = DEFAULT_TERM_WIDTH - 1;

                    // 3. Home Cursor
                    ACTIVE_SESSION.cursor.x = 0;
                    ACTIVE_SESSION.cursor.y = 0;
                }
                break;

            case 4: // DECSCLM - Scrolling Mode
                // Enable/disable smooth scrolling
                ACTIVE_SESSION.dec_modes.smooth_scroll = enable;
                break;

            case 5: // DECSCNM - Screen Mode
                // Enable/disable reverse video
                ACTIVE_SESSION.dec_modes.reverse_video = enable;
                break;

            case 6: // DECOM - Origin Mode
                // Enable/disable origin mode, adjust cursor position
                ACTIVE_SESSION.dec_modes.origin_mode = enable;
                if (enable) {
                    ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
                    ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_top;
                } else {
                    ACTIVE_SESSION.cursor.x = 0;
                    ACTIVE_SESSION.cursor.y = 0;
                }
                break;

            case 7: // DECAWM - Auto Wrap Mode
                // Enable/disable auto wrap
                ACTIVE_SESSION.dec_modes.auto_wrap_mode = enable;
                break;

            case 8: // DECARM - Auto Repeat Mode
                // Enable/disable auto repeat keys
                ACTIVE_SESSION.dec_modes.auto_repeat_keys = enable;
                break;

            case 9: // X10 Mouse Tracking
                // Enable/disable X10 mouse tracking
                ACTIVE_SESSION.mouse.mode = enable ? MOUSE_TRACKING_X10 : MOUSE_TRACKING_OFF;
                ACTIVE_SESSION.mouse.enabled = enable;
                break;

            case 12: // DECSET 12 - Local Echo / Send/Receive
                // Enable/disable local echo mode
                ACTIVE_SESSION.dec_modes.local_echo = enable;
                break;

            case 25: // DECTCEM - Text Cursor Enable Mode
                // Enable/disable text cursor visibility
                ACTIVE_SESSION.dec_modes.cursor_visible = enable;
                ACTIVE_SESSION.cursor.visible = enable;
                break;

            case 38: // DECTEK - Tektronix Mode
                if (enable) {
                    ACTIVE_SESSION.parse_state = PARSE_TEKTRONIX;
                    terminal.tektronix.state = 0; // Alpha
                    terminal.tektronix.x = 0;
                    terminal.tektronix.y = 0;
                    terminal.tektronix.pen_down = false;
                    terminal.vector_count = 0; // Clear screen on entry
                } else {
                    ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                }
                break;

            case 40: // Allow 80/132 Column Mode
                // Placeholder for column mode switching (resize not implemented)
                break;

            case 47: // Alternate Screen Buffer
            case 1047: // Alternate Screen Buffer (xterm)
                // Switch between main and alternate screen buffers
                SwitchScreenBuffer(enable);
                break;

            case 1048: // Save/Restore Cursor
                // Save or restore cursor state
                if (enable) {
                    ExecuteSaveCursor();
                } else {
                    ExecuteRestoreCursor();
                }
                break;

            case 1049: // Alternate Screen + Save/Restore Cursor
                // Save/restore cursor and switch screen buffer
                if (enable) {
                    ExecuteSaveCursor();
                    SwitchScreenBuffer(true);
                    ExecuteED(false); // Clear screen
                    ACTIVE_SESSION.cursor.x = 0;
                    ACTIVE_SESSION.cursor.y = 0;
                } else {
                    SwitchScreenBuffer(false);
                    ExecuteRestoreCursor();
                }
                break;

            case 1000: // VT200 Mouse Tracking
                // Enable/disable VT200 mouse tracking
                ACTIVE_SESSION.mouse.mode = enable ? (ACTIVE_SESSION.mouse.sgr_mode ? MOUSE_TRACKING_SGR : MOUSE_TRACKING_VT200) : MOUSE_TRACKING_OFF;
                ACTIVE_SESSION.mouse.enabled = enable;
                break;

            case 1001: // VT200 Highlight Mouse Tracking
                // Enable/disable VT200 highlight tracking
                ACTIVE_SESSION.mouse.mode = enable ? MOUSE_TRACKING_VT200_HIGHLIGHT : MOUSE_TRACKING_OFF;
                ACTIVE_SESSION.mouse.enabled = enable;
                break;

            case 1002: // Button Event Mouse Tracking
                // Enable/disable button-event tracking
                ACTIVE_SESSION.mouse.mode = enable ? MOUSE_TRACKING_BTN_EVENT : MOUSE_TRACKING_OFF;
                ACTIVE_SESSION.mouse.enabled = enable;
                break;

            case 1003: // Any Event Mouse Tracking
                // Enable/disable any-event tracking
                ACTIVE_SESSION.mouse.mode = enable ? MOUSE_TRACKING_ANY_EVENT : MOUSE_TRACKING_OFF;
                ACTIVE_SESSION.mouse.enabled = enable;
                break;

            case 1004: // Focus In/Out Events
                // Enable/disable focus tracking
                ACTIVE_SESSION.mouse.focus_tracking = enable;
                break;

            case 1005: // UTF-8 Mouse Mode
                // Placeholder for UTF-8 mouse encoding (not implemented)
                break;

            case 1006: // SGR Mouse Mode
                // Enable/disable SGR mouse reporting
                ACTIVE_SESSION.mouse.sgr_mode = enable;
                if (enable && ACTIVE_SESSION.mouse.mode != MOUSE_TRACKING_OFF &&
                    ACTIVE_SESSION.mouse.mode != MOUSE_TRACKING_URXVT && ACTIVE_SESSION.mouse.mode != MOUSE_TRACKING_PIXEL) {
                    ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_SGR;
                } else if (!enable && ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_SGR) {
                    ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_VT200;
                }
                break;

            case 1015: // URXVT Mouse Mode
                // Enable/disable URXVT mouse reporting
                ACTIVE_SESSION.mouse.mode = enable ? MOUSE_TRACKING_URXVT : MOUSE_TRACKING_OFF;
                ACTIVE_SESSION.mouse.enabled = enable;
                break;

            case 1016: // Pixel Position Mouse Mode
                // Enable/disable pixel position mouse reporting
                ACTIVE_SESSION.mouse.mode = enable ? MOUSE_TRACKING_PIXEL : MOUSE_TRACKING_OFF;
                ACTIVE_SESSION.mouse.enabled = enable;
                break;

            case 2004: // Bracketed Paste Mode
                // Enable/disable bracketed paste
                ACTIVE_SESSION.bracketed_paste.enabled = enable;
                break;

            default:
                // Log unsupported DEC modes
                if (ACTIVE_SESSION.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown DEC mode: %d", mode);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
    } else {
        // ANSI Modes
        switch (mode) {
            case 4: // IRM - Insert/Replace Mode
                // Enable/disable insert mode
                ACTIVE_SESSION.dec_modes.insert_mode = enable;
                break;

            case 20: // LNM - Line Feed/New Line Mode
                // Enable/disable line feed/new line mode
                ACTIVE_SESSION.ansi_modes.line_feed_new_line = enable;
                break;

            default:
                // Log unsupported ANSI modes
                if (ACTIVE_SESSION.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown ANSI mode: %d", mode);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
    }
}

// Set terminal modes (CSI Pm h or CSI ? Pm h)
// Enables specified modes, including mouse tracking and focus reporting
static void ExecuteSM(bool private_mode) {
    // Iterate through parsed parameters from the CSI sequence
    for (int i = 0; i < ACTIVE_SESSION.param_count; i++) {
        int mode = ACTIVE_SESSION.escape_params[i];
        if (private_mode) {
            switch (mode) {
                case 1000: // VT200 mouse tracking
                    EnableMouseFeature("cursor", true);
                    ACTIVE_SESSION.mouse.mode = ACTIVE_SESSION.mouse.sgr_mode ? MOUSE_TRACKING_SGR : MOUSE_TRACKING_VT200;
                    break;
                case 1002: // Button-event mouse tracking
                    EnableMouseFeature("cursor", true);
                    ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_BTN_EVENT;
                    break;
                case 1003: // Any-event mouse tracking
                    EnableMouseFeature("cursor", true);
                    ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_ANY_EVENT;
                    break;
                case 1004: // Focus tracking
                    EnableMouseFeature("focus", true);
                    break;
                case 1006: // SGR mouse reporting
                    EnableMouseFeature("sgr", true);
                    break;
                case 1015: // URXVT mouse reporting
                    EnableMouseFeature("urxvt", true);
                    break;
                case 1016: // Pixel position mouse reporting
                    EnableMouseFeature("pixel", true);
                    break;
                case 64: // DECSCCM - Multi-Session Support (Private mode 64 typically page/session stuff)
                         // VT520 DECSCCM (Select Cursor Control Mode) is 64 but this context is ? 64.
                         // Standard VT520 doesn't strictly document ?64 as Multi-Session enable, but used here for protocol.
                    // Enable multi-session switching capability
                    ACTIVE_SESSION.conformance.features.multi_session_mode = true;
                    break;
                default:
                    // Delegate other private modes to SetTerminalModeInternal
                    SetTerminalModeInternal(mode, true, private_mode);
                    break;
            }
        } else {
            // Delegate ANSI modes to SetTerminalModeInternal
            SetTerminalModeInternal(mode, true, private_mode);
        }
    }
}

// Reset terminal modes (CSI Pm l or CSI ? Pm l)
// Disables specified modes, including mouse tracking and focus reporting
static void ExecuteRM(bool private_mode) {
    // Iterate through parsed parameters from the CSI sequence
    for (int i = 0; i < ACTIVE_SESSION.param_count; i++) {
        int mode = ACTIVE_SESSION.escape_params[i];
        if (private_mode) {
            switch (mode) {
                case 1000: // VT200 mouse tracking
                case 1002: // Button-event mouse tracking
                case 1003: // Any-event mouse tracking
                case 1015: // URXVT mouse reporting
                case 1016: // Pixel position mouse reporting
                    EnableMouseFeature("cursor", false);
                    ACTIVE_SESSION.mouse.mode = MOUSE_TRACKING_OFF;
                    break;
                case 1004: // Focus tracking
                    EnableMouseFeature("focus", false);
                    break;
                case 1006: // SGR mouse reporting
                    EnableMouseFeature("sgr", false);
                    break;
                case 64: // Disable Multi-Session Support
                    ACTIVE_SESSION.conformance.features.multi_session_mode = false;
                    // If disabling, we should probably switch back to Session 1?
                    if (terminal.active_session != 0) {
                        SetActiveSession(0);
                    }
                    break;
                default:
                    // Delegate other private modes to SetTerminalModeInternal
                    SetTerminalModeInternal(mode, false, private_mode);
                    break;
            }
        } else {
            // Delegate ANSI modes to SetTerminalModeInternal
            SetTerminalModeInternal(mode, false, private_mode);
        }
    }
}

// Continue with device attributes and other implementations...

void ExecuteDA(bool private_mode) {
    char introducer = private_mode ? ACTIVE_SESSION.escape_buffer[0] : 0;
    if (introducer == '>') {
        QueueResponse(ACTIVE_SESSION.secondary_attributes);
    } else if (introducer == '=') {
        QueueResponse(ACTIVE_SESSION.tertiary_attributes);
    } else {
        QueueResponse(ACTIVE_SESSION.device_attributes);
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
static void SendToPrinter(const char* data, size_t length) {
    if (terminal.printer_callback) {
        terminal.printer_callback(data, length);
    } else {
        // Fallback: If no printer callback, maybe log or ignore?
        // Standard behavior might be to do nothing if no printer attached.
        if (ACTIVE_SESSION.options.debug_sequences) {
            fprintf(stderr, "MC: Print requested but no printer callback set (len=%zu)\n", length);
        }
    }
}

// Updated ExecuteMC
static void ExecuteMC(void) {
    bool private_mode = (ACTIVE_SESSION.escape_buffer[0] == '?');
    int params[MAX_ESCAPE_PARAMS];
    ParseCSIParams(ACTIVE_SESSION.escape_buffer, params, MAX_ESCAPE_PARAMS);
    int pi = (ACTIVE_SESSION.param_count > 0) ? ACTIVE_SESSION.escape_params[0] : 0;

    if (!ACTIVE_SESSION.printer_available) {
        LogUnsupportedSequence("MC: No printer available");
        return;
    }

    if (!private_mode) {
        switch (pi) {
            case 0: // Print entire screen
            {
                char print_buffer[DEFAULT_TERM_WIDTH * DEFAULT_TERM_HEIGHT + DEFAULT_TERM_HEIGHT + 1];
                size_t pos = 0;
                for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                    for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                        EnhancedTermChar* cell = GetScreenCell(&ACTIVE_SESSION, y, x);
                        if (pos < sizeof(print_buffer) - 2) {
                            print_buffer[pos++] = GetPrintableChar(cell->ch, &ACTIVE_SESSION.charset);
                        }
                    }
                    if (pos < sizeof(print_buffer) - 2) {
                        print_buffer[pos++] = '\n';
                    }
                }
                print_buffer[pos] = '\0';
                SendToPrinter(print_buffer, pos);
                if (ACTIVE_SESSION.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Print screen completed");
                }
                break;
            }
            case 1: // Print current line
            {
                char print_buffer[DEFAULT_TERM_WIDTH + 2];
                size_t pos = 0;
                int y = ACTIVE_SESSION.cursor.y;
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetScreenCell(&ACTIVE_SESSION, y, x);
                    if (pos < sizeof(print_buffer) - 2) {
                        print_buffer[pos++] = GetPrintableChar(cell->ch, &ACTIVE_SESSION.charset);
                    }
                }
                print_buffer[pos++] = '\n';
                print_buffer[pos] = '\0';
                SendToPrinter(print_buffer, pos);
                if (ACTIVE_SESSION.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Print line completed");
                }
                break;
            }
            case 4: // Disable auto-print
                ACTIVE_SESSION.auto_print_enabled = false;
                if (ACTIVE_SESSION.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Auto-print disabled");
                }
                break;
            case 5: // Enable auto-print
                ACTIVE_SESSION.auto_print_enabled = true;
                if (ACTIVE_SESSION.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Auto-print enabled");
                }
                break;
            default:
                if (ACTIVE_SESSION.options.log_unsupported) {
                    snprintf(ACTIVE_SESSION.conformance.compliance.last_unsupported,
                             sizeof(ACTIVE_SESSION.conformance.compliance.last_unsupported),
                             "CSI %d i", pi);
                    ACTIVE_SESSION.conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    } else {
        switch (pi) {
            case 4: // Disable printer controller mode
                ACTIVE_SESSION.printer_controller_enabled = false;
                if (ACTIVE_SESSION.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Printer controller disabled");
                }
                break;
            case 5: // Enable printer controller mode
                ACTIVE_SESSION.printer_controller_enabled = true;
                if (ACTIVE_SESSION.options.debug_sequences) {
                }
                break;
            case 9: // Print Screen (DEC specific private parameter for same action as CSI 0 i)
            {
                char print_buffer[DEFAULT_TERM_WIDTH * DEFAULT_TERM_HEIGHT + DEFAULT_TERM_HEIGHT + 1];
                size_t pos = 0;
                for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                    for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                        EnhancedTermChar* cell = GetScreenCell(&ACTIVE_SESSION, y, x);
                        if (pos < sizeof(print_buffer) - 2) {
                            print_buffer[pos++] = GetPrintableChar(cell->ch, &ACTIVE_SESSION.charset);
                        }
                    }
                    if (pos < sizeof(print_buffer) - 2) {
                        print_buffer[pos++] = '\n';
                    }
                }
                print_buffer[pos] = '\0';
                SendToPrinter(print_buffer, pos);
                if (ACTIVE_SESSION.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Print screen (DEC) completed");
                }
                break;
            }
            default:
                if (ACTIVE_SESSION.options.log_unsupported) {
                    snprintf(ACTIVE_SESSION.conformance.compliance.last_unsupported,
                             sizeof(ACTIVE_SESSION.conformance.compliance.last_unsupported),
                             "CSI ?%d i", pi);
                    ACTIVE_SESSION.conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    }
}

// Enhanced QueueResponse
void QueueResponse(const char* response) {
    size_t len = strlen(response);
    // Leave space for null terminator
    if (ACTIVE_SESSION.response_length + len >= sizeof(ACTIVE_SESSION.answerback_buffer) - 1) {
        // Flush existing responses
        if (terminal.response_callback && ACTIVE_SESSION.response_length > 0) {
            terminal.response_callback(ACTIVE_SESSION.answerback_buffer, ACTIVE_SESSION.response_length);
            ACTIVE_SESSION.response_length = 0;
        }
        // If response is still too large, log and truncate
        if (len >= sizeof(ACTIVE_SESSION.answerback_buffer) - 1) {
            if (ACTIVE_SESSION.options.debug_sequences) {
                fprintf(stderr, "QueueResponse: Response too large (%zu bytes)\n", len);
            }
            len = sizeof(ACTIVE_SESSION.answerback_buffer) - 1;
        }
    }

    if (len > 0) {
        memcpy(ACTIVE_SESSION.answerback_buffer + ACTIVE_SESSION.response_length, response, len);
        ACTIVE_SESSION.response_length += len;
        ACTIVE_SESSION.answerback_buffer[ACTIVE_SESSION.response_length] = '\0'; // Ensure null-termination
    }
}

void QueueResponseBytes(const char* data, size_t len) {
    if (ACTIVE_SESSION.response_length + len >= sizeof(ACTIVE_SESSION.answerback_buffer)) {
        if (terminal.response_callback && ACTIVE_SESSION.response_length > 0) {
            terminal.response_callback(ACTIVE_SESSION.answerback_buffer, ACTIVE_SESSION.response_length);
            ACTIVE_SESSION.response_length = 0;
        }
        if (len >= sizeof(ACTIVE_SESSION.answerback_buffer)) {
            if (ACTIVE_SESSION.options.debug_sequences) {
                fprintf(stderr, "QueueResponseBytes: Response too large (%zu bytes)\n", len);
            }
            len = sizeof(ACTIVE_SESSION.answerback_buffer) -1;
        }
    }

    if (len > 0) {
        memcpy(ACTIVE_SESSION.answerback_buffer + ACTIVE_SESSION.response_length, data, len);
        ACTIVE_SESSION.response_length += len;
    }
}

// Enhanced ExecuteDSR function with new handlers
static void ExecuteDSR(void) {
    bool private_mode = (ACTIVE_SESSION.escape_buffer[0] == '?');
    int params[MAX_ESCAPE_PARAMS];
    ParseCSIParams(ACTIVE_SESSION.escape_buffer, params, MAX_ESCAPE_PARAMS);
    int command = (ACTIVE_SESSION.param_count > 0) ? ACTIVE_SESSION.escape_params[0] : 0;

    if (!private_mode) {
        switch (command) {
            case 5: QueueResponse("\x1B[0n"); break;
            case 6: {
                int row = ACTIVE_SESSION.cursor.y + 1;
                int col = ACTIVE_SESSION.cursor.x + 1;
                if (ACTIVE_SESSION.dec_modes.origin_mode) {
                    row = ACTIVE_SESSION.cursor.y - ACTIVE_SESSION.scroll_top + 1;
                    col = ACTIVE_SESSION.cursor.x - ACTIVE_SESSION.left_margin + 1;
                }
                char response[32];
                snprintf(response, sizeof(response), "\x1B[%d;%dR", row, col);
                QueueResponse(response);
                break;
            }
            default:
                if (ACTIVE_SESSION.options.log_unsupported) {
                    snprintf(ACTIVE_SESSION.conformance.compliance.last_unsupported,
                             sizeof(ACTIVE_SESSION.conformance.compliance.last_unsupported),
                             "CSI %dn", command);
                    ACTIVE_SESSION.conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    } else {
        switch (command) {
            case 15: QueueResponse(ACTIVE_SESSION.printer_available ? "\x1B[?10n" : "\x1B[?13n"); break;
            case 25: QueueResponse(ACTIVE_SESSION.programmable_keys.udk_locked ? "\x1B[?21n" : "\x1B[?20n"); break;
            case 26: {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?27;%dn", ACTIVE_SESSION.vt_keyboard.keyboard_dialect);
                QueueResponse(response);
                break;
            }
            case 27: // Locator Type (DECREPTPARM)
                QueueResponse("\x1B[?27;0n"); // No locator
                break;
            case 53: QueueResponse(ACTIVE_SESSION.locator_enabled ? "\x1B[?53n" : "\x1B[?50n"); break;
            case 55: QueueResponse("\x1B[?57;0n"); break;
            case 56: QueueResponse("\x1B[?56;0n"); break;
            case 62: {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?62;%zu;%zun",
                         ACTIVE_SESSION.macro_space.used, ACTIVE_SESSION.macro_space.total);
                QueueResponse(response);
                break;
            }
            case 63: {
                int page = (ACTIVE_SESSION.param_count > 1) ? ACTIVE_SESSION.escape_params[1] : 1;
                ACTIVE_SESSION.checksum.last_checksum = ComputeScreenChecksum(page);
                char response[64];
                snprintf(response, sizeof(response), "\x1B[?63;%d;%d;%04Xn",
                         page, ACTIVE_SESSION.checksum.algorithm, ACTIVE_SESSION.checksum.last_checksum);
                QueueResponse(response);
                break;
            }
            case 75: QueueResponse("\x1B[?75;0n"); break;
            case 12: { // DECRSN - Report Session Number
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?12;%dn", terminal.active_session + 1);
                QueueResponse(response);
                break;
            }
            case 21: { // DECRS - Report Session Status
                char buf[64];
                strcpy(buf, "\x1BP$p"); // DCS $ p
                bool first = true;
                for(int i=0; i<MAX_SESSIONS; i++) {
                    if (terminal.sessions[i].session_open) {
                        if (!first) strcat(buf, ";");
                        char num[4];
                        snprintf(num, sizeof(num), "%d", i+1);
                        strcat(buf, num);
                        first = false;
                    }
                }
                strcat(buf, "\x1B\\"); // ST
                QueueResponse(buf);
                break;
            }
            default:
                if (ACTIVE_SESSION.options.log_unsupported) {
                    snprintf(ACTIVE_SESSION.conformance.compliance.last_unsupported,
                             sizeof(ACTIVE_SESSION.conformance.compliance.last_unsupported),
                             "CSI ?%dn", command);
                    ACTIVE_SESSION.conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    }
}

void ExecuteDECSTBM(void) { // Set Top and Bottom Margins
    int top = GetCSIParam(0, 1) - 1;    // Convert to 0-based
    int bottom = GetCSIParam(1, DEFAULT_TERM_HEIGHT) - 1;

    // Validate parameters
    if (top >= 0 && top < DEFAULT_TERM_HEIGHT && bottom >= top && bottom < DEFAULT_TERM_HEIGHT) {
        ACTIVE_SESSION.scroll_top = top;
        ACTIVE_SESSION.scroll_bottom = bottom;

        // Move cursor to home position
        if (ACTIVE_SESSION.dec_modes.origin_mode) {
            ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
            ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_top;
        } else {
            ACTIVE_SESSION.cursor.x = 0;
            ACTIVE_SESSION.cursor.y = 0;
        }
    }
}

void ExecuteDECSLRM(void) { // Set Left and Right Margins (VT420)
    if (!ACTIVE_SESSION.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECSLRM requires VT420 mode");
        return;
    }

    int left = GetCSIParam(0, 1) - 1;    // Convert to 0-based
    int right = GetCSIParam(1, DEFAULT_TERM_WIDTH) - 1;

    // Validate parameters
    if (left >= 0 && left < DEFAULT_TERM_WIDTH && right >= left && right < DEFAULT_TERM_WIDTH) {
        ACTIVE_SESSION.left_margin = left;
        ACTIVE_SESSION.right_margin = right;

        // Move cursor to home position
        if (ACTIVE_SESSION.dec_modes.origin_mode) {
            ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.left_margin;
            ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.scroll_top;
        } else {
            ACTIVE_SESSION.cursor.x = 0;
            ACTIVE_SESSION.cursor.y = 0;
        }
    }
}

// Updated ExecuteDECRQPSR
static void ExecuteDECRQPSR(void) {
    int params[MAX_ESCAPE_PARAMS];
    ParseCSIParams(ACTIVE_SESSION.escape_buffer, params, MAX_ESCAPE_PARAMS);
    int pfn = (ACTIVE_SESSION.param_count > 0) ? ACTIVE_SESSION.escape_params[0] : 0;

    char response[64];
    switch (pfn) {
        case 1: // Sixel geometry
            snprintf(response, sizeof(response), "DCS 2 $u %d ; %d;%d;%d;%d ST",
                     ACTIVE_SESSION.conformance.level, ACTIVE_SESSION.sixel.x, ACTIVE_SESSION.sixel.y,
                     ACTIVE_SESSION.sixel.width, ACTIVE_SESSION.sixel.height);
            QueueResponse(response);
            break;
        case 2: // Sixel color palette
            for (int i = 0; i < 256; i++) {
                RGB_Color c = color_palette[i];
                snprintf(response, sizeof(response), "DCS 1 $u #%d;%d;%d;%d ST",
                         i, c.r, c.g, c.b);
                QueueResponse(response);
            }
            break;
        case 3: // ReGIS (unsupported)
            if (ACTIVE_SESSION.options.log_unsupported) {
                snprintf(ACTIVE_SESSION.conformance.compliance.last_unsupported,
                         sizeof(ACTIVE_SESSION.conformance.compliance.last_unsupported),
                         "CSI %d $ u (ReGIS unsupported)", pfn);
                ACTIVE_SESSION.conformance.compliance.unsupported_sequences++;
            }
            break;
        default:
            if (ACTIVE_SESSION.options.log_unsupported) {
                snprintf(ACTIVE_SESSION.conformance.compliance.last_unsupported,
                         sizeof(ACTIVE_SESSION.conformance.compliance.last_unsupported),
                         "CSI %d $ u", pfn);
                ACTIVE_SESSION.conformance.compliance.unsupported_sequences++;
            }
            break;
    }
}

void ExecuteDECLL(void) { // Load LEDs
    int n = GetCSIParam(0, 0);

    // DECLL - Load LEDs (VT220+ feature)
    // Parameters: 0=all off, 1=LED1 on, 2=LED2 on, 4=LED3 on, 8=LED4 on
    // Modern terminals don't have LEDs, so this is mostly ignored

    if (ACTIVE_SESSION.options.debug_sequences) {
        char debug_msg[64];
        snprintf(debug_msg, sizeof(debug_msg), "DECLL: LED state %d", n);
        LogUnsupportedSequence(debug_msg);
    }

    // Could be used for visual indicators in a GUI implementation
    // For now, just acknowledge the command
}

void ExecuteDECSTR(void) { // Soft Terminal Reset
    // Reset terminal to power-on defaults but keep communication settings

    // Reset display modes
    ACTIVE_SESSION.dec_modes.cursor_visible = true;
    ACTIVE_SESSION.dec_modes.auto_wrap_mode = true;
    ACTIVE_SESSION.dec_modes.origin_mode = false;
    ACTIVE_SESSION.dec_modes.insert_mode = false;
    ACTIVE_SESSION.dec_modes.application_cursor_keys = false;

    // Reset character attributes
    ResetAllAttributes();

    // Reset scrolling region
    ACTIVE_SESSION.scroll_top = 0;
    ACTIVE_SESSION.scroll_bottom = DEFAULT_TERM_HEIGHT - 1;
    ACTIVE_SESSION.left_margin = 0;
    ACTIVE_SESSION.right_margin = DEFAULT_TERM_WIDTH - 1;

    // Reset character sets
    InitCharacterSets();

    // Reset tab stops
    InitTabStops();

    // Home cursor
    ACTIVE_SESSION.cursor.x = 0;
    ACTIVE_SESSION.cursor.y = 0;

    // Clear saved cursor
    ACTIVE_SESSION.saved_cursor_valid = false;

    InitColorPalette();
    InitSixelGraphics();

    if (ACTIVE_SESSION.options.debug_sequences) {
        LogUnsupportedSequence("DECSTR: Soft terminal reset");
    }
}

void ExecuteDECSCL(void) { // Select Conformance Level
    int level = GetCSIParam(0, 61);
    int c1_control = GetCSIParam(1, 0);
    (void)c1_control;  // Mark as intentionally unused

    // Set conformance level based on parameter
    switch (level) {
        case 61: SetVTLevel(VT_LEVEL_100); break;
        case 62: SetVTLevel(VT_LEVEL_220); break;
        case 63: SetVTLevel(VT_LEVEL_320); break;
        case 64: SetVTLevel(VT_LEVEL_420); break;
        default:
            if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown conformance level: %d", level);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }

    // c1_control: 0=8-bit controls, 1=7-bit controls, 2=8-bit controls
    // Modern implementations typically use 7-bit controls
}

// Enhanced ExecuteDECRQM
void ExecuteDECRQM(void) { // Request Mode
    int mode = GetCSIParam(0, 0);
    bool private_mode = (ACTIVE_SESSION.escape_buffer[0] == '?');

    char response[32];
    int mode_state = 0; // 0=not recognized, 1=set, 2=reset, 3=permanently set, 4=permanently reset

    if (private_mode) {
        // Check DEC private modes
        switch (mode) {
            case 1: // DECCKM (Application Cursor Keys)
                mode_state = ACTIVE_SESSION.dec_modes.application_cursor_keys ? 1 : 2;
                break;
            case 3: // DECCOLM (132 Column Mode)
                mode_state = ACTIVE_SESSION.dec_modes.column_mode_132 ? 1 : 2;
                break;
            case 4: // DECSCLM (Smooth Scroll)
                mode_state = ACTIVE_SESSION.dec_modes.smooth_scroll ? 1 : 2;
                break;
            case 5: // DECSCNM (Reverse Video)
                mode_state = ACTIVE_SESSION.dec_modes.reverse_video ? 1 : 2;
                break;
            case 6: // DECOM (Origin Mode)
                mode_state = ACTIVE_SESSION.dec_modes.origin_mode ? 1 : 2;
                break;
            case 7: // DECAWM (Auto Wrap Mode)
                mode_state = ACTIVE_SESSION.dec_modes.auto_wrap_mode ? 1 : 2;
                break;
            case 8: // DECARM (Auto Repeat Keys)
                mode_state = ACTIVE_SESSION.dec_modes.auto_repeat_keys ? 1 : 2;
                break;
            case 9: // X10 Mouse
                mode_state = ACTIVE_SESSION.dec_modes.x10_mouse ? 1 : 2;
                break;
            case 10: // Show Toolbar
                mode_state = ACTIVE_SESSION.dec_modes.show_toolbar ? 1 : 4; // Permanently reset
                break;
            case 12: // Blinking Cursor
                mode_state = ACTIVE_SESSION.dec_modes.blink_cursor ? 1 : 2;
                break;
            case 18: // DECPFF (Print Form Feed)
                mode_state = ACTIVE_SESSION.dec_modes.print_form_feed ? 1 : 2;
                break;
            case 19: // Print Extent
                mode_state = ACTIVE_SESSION.dec_modes.print_extent ? 1 : 2;
                break;
            case 25: // DECTCEM (Cursor Visible)
                mode_state = ACTIVE_SESSION.dec_modes.cursor_visible ? 1 : 2;
                break;
            case 38: // Tektronix Mode
                mode_state = (ACTIVE_SESSION.parse_state == PARSE_TEKTRONIX) ? 1 : 2;
                break;
            case 47: // Alternate Screen
            case 1047:
            case 1049:
                mode_state = ACTIVE_SESSION.dec_modes.alternate_screen ? 1 : 2;
                break;
            case 1000: // VT200 Mouse
                mode_state = (ACTIVE_SESSION.mouse.mode == MOUSE_TRACKING_VT200) ? 1 : 2;
                break;
            case 2004: // Bracketed Paste
                mode_state = ACTIVE_SESSION.bracketed_paste.enabled ? 1 : 2;
                break;
            case 61: // DECSCL VT100
                mode_state = (ACTIVE_SESSION.conformance.level == VT_LEVEL_100) ? 1 : 2;
                break;
            case 62: // DECSCL VT220
                mode_state = (ACTIVE_SESSION.conformance.level == VT_LEVEL_220) ? 1 : 2;
                break;
            case 63: // DECSCL VT320
                mode_state = (ACTIVE_SESSION.conformance.level == VT_LEVEL_520) ? 1 : 2;
                break;
            case 64: // DECSCL VT420
                mode_state = (ACTIVE_SESSION.conformance.level == VT_LEVEL_420) ? 1 : 2;
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
                mode_state = ACTIVE_SESSION.ansi_modes.insert_replace ? 1 : 2;
                break;
            case 20: // LNM (Line Feed/New Line Mode)
                mode_state = ACTIVE_SESSION.ansi_modes.line_feed_new_line ? 1 : 3; // Permanently set
                break;
            default:
                mode_state = 0;
                break;
        }
        snprintf(response, sizeof(response), "\x1B[%d;%d$y", mode, mode_state);
    }

    QueueResponse(response);
}

void ExecuteDECSCUSR(void) { // Set Cursor Style
    int style = GetCSIParam(0, 1);

    switch (style) {
        case 0: case 1: // Default or blinking block
            ACTIVE_SESSION.cursor.shape = CURSOR_BLOCK_BLINK;
            ACTIVE_SESSION.cursor.blink_enabled = true;
            break;
        case 2: // Steady block
            ACTIVE_SESSION.cursor.shape = CURSOR_BLOCK;
            ACTIVE_SESSION.cursor.blink_enabled = false;
            break;
        case 3: // Blinking underline
            ACTIVE_SESSION.cursor.shape = CURSOR_UNDERLINE_BLINK;
            ACTIVE_SESSION.cursor.blink_enabled = true;
            break;
        case 4: // Steady underline
            ACTIVE_SESSION.cursor.shape = CURSOR_UNDERLINE;
            ACTIVE_SESSION.cursor.blink_enabled = false;
            break;
        case 5: // Blinking bar
            ACTIVE_SESSION.cursor.shape = CURSOR_BAR_BLINK;
            ACTIVE_SESSION.cursor.blink_enabled = true;
            break;
        case 6: // Steady bar
            ACTIVE_SESSION.cursor.shape = CURSOR_BAR;
            ACTIVE_SESSION.cursor.blink_enabled = false;
            break;
        default:
            if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown cursor style: %d", style);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }
}

void ExecuteCSI_P(void) { // Various P commands
    // This handles CSI sequences ending in 'p' with different intermediates
    char* params = ACTIVE_SESSION.escape_buffer;

    if (strstr(params, "!") != NULL) {
        // DECSTR - Soft Terminal Reset
        ExecuteDECSTR();
    } else if (strstr(params, "\"") != NULL) {
        // DECSCL - Select Conformance Level
        ExecuteDECSCL();
    } else if (strstr(params, "$") != NULL) {
        // DECRQM - Request Mode
        ExecuteDECRQM();
    } else if (strstr(params, " ") != NULL) {
        // Set cursor style (DECSCUSR)
        ExecuteDECSCUSR();
    } else {
        // Unknown p command
        if (ACTIVE_SESSION.options.debug_sequences) {
            char debug_msg[MAX_COMMAND_BUFFER + 64];
            snprintf(debug_msg, sizeof(debug_msg), "Unknown CSI p command: %s", params);
            LogUnsupportedSequence(debug_msg);
        }
    }
}

void ExecuteDECSCA(void) { // Select Character Protection Attribute
    // CSI Ps " q
    // Ps = 0, 2 -> Not protected
    // Ps = 1 -> Protected
    int ps = GetCSIParam(0, 0);
    if (ps == 1) {
        ACTIVE_SESSION.protected_mode = true;
    } else {
        ACTIVE_SESSION.protected_mode = false;
    }
}


void ExecuteWindowOps(void) { // Window manipulation (xterm extension)
    int operation = GetCSIParam(0, 0);

    switch (operation) {
        case 1: // De-iconify window (Restore)
            SituationRestoreWindow();
            break;
        case 2: // Iconify window (Minimize)
            SituationMinimizeWindow();
            break;
        case 3: // Move window to position (in pixels)
            {
                int x = GetCSIParam(1, 0);
                int y = GetCSIParam(2, 0);
                SituationSetWindowPosition(x, y);
            }
            break;
        case 4: // Resize window (in pixels)
            {
                int height = GetCSIParam(1, DEFAULT_WINDOW_HEIGHT);
                int width = GetCSIParam(2, DEFAULT_WINDOW_WIDTH);
                SituationSetWindowSize(width, height);
            }
            break;
        case 5: // Raise window (Bring to front)
            SituationSetWindowFocused(); // Closest approximation
            break;
        case 6: // Lower window
            // Not directly supported by Situation/GLFW easily
            if (ACTIVE_SESSION.options.debug_sequences) LogUnsupportedSequence("Window lower not supported");
            break;
        case 7: // Refresh window
            // Handled automatically by game loop
            break;
        case 8: // Resize text area (in chars)
            {
                int rows = GetCSIParam(1, DEFAULT_TERM_HEIGHT);
                int cols = GetCSIParam(2, DEFAULT_TERM_WIDTH);
                int width = cols * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE;
                int height = rows * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE;
                SituationSetWindowSize(width, height);
            }
            break;

        case 9: // Maximize/restore window
            if (GetCSIParam(1, 0) == 1) SituationMaximizeWindow();
            else SituationRestoreWindow();
            break;
        case 10: // Full-screen toggle
            if (GetCSIParam(1, 0) == 1) {
                if (!SituationIsWindowFullscreen()) SituationToggleFullscreen();
            } else {
                if (SituationIsWindowFullscreen()) SituationToggleFullscreen();
            }
            break;

        case 11: // Report window state
            QueueResponse("\x1B[1t"); // Always report "not iconified"
            break;

        case 13: // Report window position
        case 14: // Report window size in pixels
        case 18: // Report text area size
            {
                char response[32];
                if (operation == 18) {
                    snprintf(response, sizeof(response), "\x1B[8;%d;%dt", DEFAULT_TERM_HEIGHT, DEFAULT_TERM_WIDTH);
                } else {
                    snprintf(response, sizeof(response), "\x1B[3;%d;%dt", 100, 100); // Dummy values
                }
                QueueResponse(response);
            }
            break;

        case 19: // Report screen size
            {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[9;%d;%dt",
                        SituationGetScreenHeight() / DEFAULT_CHAR_HEIGHT, SituationGetScreenWidth() / DEFAULT_CHAR_WIDTH);
                QueueResponse(response);
            }
            break;

        case 20: // Report icon label
            {
                char response[MAX_TITLE_LENGTH + 32];
                snprintf(response, sizeof(response), "\x1B]L%s\x1B\\", ACTIVE_SESSION.title.icon_title);
                QueueResponse(response);
            }
            break;

        case 21: // Report window title
            {
                char response[MAX_TITLE_LENGTH + 32];
                snprintf(response, sizeof(response), "\x1B]l%s\x1B\\", ACTIVE_SESSION.title.window_title);
                QueueResponse(response);
            }
            break;

        default:
            if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown window operation: %d", operation);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }
}

void ExecuteSaveCursor(void) {
    ACTIVE_SESSION.saved_cursor.x = ACTIVE_SESSION.cursor.x;
    ACTIVE_SESSION.saved_cursor.y = ACTIVE_SESSION.cursor.y;

    // Modes
    ACTIVE_SESSION.saved_cursor.origin_mode = ACTIVE_SESSION.dec_modes.origin_mode;
    ACTIVE_SESSION.saved_cursor.auto_wrap_mode = ACTIVE_SESSION.dec_modes.auto_wrap_mode;

    // Attributes
    ACTIVE_SESSION.saved_cursor.fg_color = ACTIVE_SESSION.current_fg;
    ACTIVE_SESSION.saved_cursor.bg_color = ACTIVE_SESSION.current_bg;
    ACTIVE_SESSION.saved_cursor.bold_mode = ACTIVE_SESSION.bold_mode;
    ACTIVE_SESSION.saved_cursor.faint_mode = ACTIVE_SESSION.faint_mode;
    ACTIVE_SESSION.saved_cursor.italic_mode = ACTIVE_SESSION.italic_mode;
    ACTIVE_SESSION.saved_cursor.underline_mode = ACTIVE_SESSION.underline_mode;
    ACTIVE_SESSION.saved_cursor.blink_mode = ACTIVE_SESSION.blink_mode;
    ACTIVE_SESSION.saved_cursor.reverse_mode = ACTIVE_SESSION.reverse_mode;
    ACTIVE_SESSION.saved_cursor.strikethrough_mode = ACTIVE_SESSION.strikethrough_mode;
    ACTIVE_SESSION.saved_cursor.conceal_mode = ACTIVE_SESSION.conceal_mode;
    ACTIVE_SESSION.saved_cursor.overline_mode = ACTIVE_SESSION.overline_mode;
    ACTIVE_SESSION.saved_cursor.double_underline_mode = ACTIVE_SESSION.double_underline_mode;
    ACTIVE_SESSION.saved_cursor.protected_mode = ACTIVE_SESSION.protected_mode;

    // Charset
    // Direct copy is safe as CharsetState contains values, and pointers point to
    // fields within the CharsetState struct (or rather, the session's charset struct).
    // However, the pointers gl/gr in SavedCursorState will point to ACTIVE_SESSION.charset.gX,
    // which is what we want for state restoration?
    // Actually, no. If we restore, we copy saved->active.
    // So saved.gl must point to saved.gX? No.
    // The GL/GR pointers indicate WHICH slot (G0-G3) is active.
    // They point to addresses.
    // If we simply copy, saved.gl points to ACTIVE_SESSION.charset.gX.
    // This is correct. Because 'gl' just tells us "we are using G0" (by address).
    // But wait, if we want to save "G0 is currently mapped to GL", saving the pointer &active.g0 is correct.
    // But we also need to save the *content* of G0 (e.g. ASCII vs UK).
    // memcpy copies the content of g0..g3.
    // So:
    // 1. Content of G0..G3 is saved.
    // 2. Pointer GL points to Active's G0..G3.
    // This is weird. saved.gl pointing to active.g0 means saved state depends on active address?
    // If we restore: ACTIVE = SAVED.
    // ACTIVE.gl will point to ACTIVE.g0 (because SAVED.gl pointed there).
    // This assumes the pointers don't change relative to the struct base?
    // They are absolute pointers.
    // So if I say ACTIVE.saved_cursor = ...
    // ACTIVE.saved_cursor.gl = &ACTIVE.charset.g0.
    // Later: ACTIVE.charset = ACTIVE.saved_cursor.charset.
    // ACTIVE.charset.gl = &ACTIVE.charset.g0.
    // This is correct.
    ACTIVE_SESSION.saved_cursor.charset = ACTIVE_SESSION.charset;

    ACTIVE_SESSION.saved_cursor_valid = true;
}

void ExecuteRestoreCursor(void) { // Restore cursor (non-ANSI.SYS)
    // This is the VT terminal version, not ANSI.SYS
    // Restores cursor from per-session saved state
    if (ACTIVE_SESSION.saved_cursor_valid) {
        // Restore Position
        ACTIVE_SESSION.cursor.x = ACTIVE_SESSION.saved_cursor.x;
        ACTIVE_SESSION.cursor.y = ACTIVE_SESSION.saved_cursor.y;

        // Restore Modes
        ACTIVE_SESSION.dec_modes.origin_mode = ACTIVE_SESSION.saved_cursor.origin_mode;
        ACTIVE_SESSION.dec_modes.auto_wrap_mode = ACTIVE_SESSION.saved_cursor.auto_wrap_mode;

        // Restore Attributes
        ACTIVE_SESSION.current_fg = ACTIVE_SESSION.saved_cursor.fg_color;
        ACTIVE_SESSION.current_bg = ACTIVE_SESSION.saved_cursor.bg_color;
        ACTIVE_SESSION.bold_mode = ACTIVE_SESSION.saved_cursor.bold_mode;
        ACTIVE_SESSION.faint_mode = ACTIVE_SESSION.saved_cursor.faint_mode;
        ACTIVE_SESSION.italic_mode = ACTIVE_SESSION.saved_cursor.italic_mode;
        ACTIVE_SESSION.underline_mode = ACTIVE_SESSION.saved_cursor.underline_mode;
        ACTIVE_SESSION.blink_mode = ACTIVE_SESSION.saved_cursor.blink_mode;
        ACTIVE_SESSION.reverse_mode = ACTIVE_SESSION.saved_cursor.reverse_mode;
        ACTIVE_SESSION.strikethrough_mode = ACTIVE_SESSION.saved_cursor.strikethrough_mode;
        ACTIVE_SESSION.conceal_mode = ACTIVE_SESSION.saved_cursor.conceal_mode;
        ACTIVE_SESSION.overline_mode = ACTIVE_SESSION.saved_cursor.overline_mode;
        ACTIVE_SESSION.double_underline_mode = ACTIVE_SESSION.saved_cursor.double_underline_mode;
        ACTIVE_SESSION.protected_mode = ACTIVE_SESSION.saved_cursor.protected_mode;

        // Restore Charset
        ACTIVE_SESSION.charset = ACTIVE_SESSION.saved_cursor.charset;

        // Fixup pointers if they point to the saved struct (unlikely with simple copy logic above,
        // but let's ensure they point to the ACTIVE struct slots)
        // If saved.gl pointed to &ACTIVE_SESSION.charset.g0, then restored gl points there too.
        // We are good.
    }
}

void ExecuteDECREQTPARM(void) { // Request Terminal Parameters
    int parm = GetCSIParam(0, 0);

    char response[32];
    // Report terminal parameters
    // Format: CSI sol ; par ; nbits ; xspeed ; rspeed ; clkmul ; flags x
    // Simplified response with standard values
    snprintf(response, sizeof(response), "\x1B[%d;1;1;120;120;1;0x", parm + 2);
    QueueResponse(response);
}

void ExecuteDECTST(void) { // Invoke Confidence Test
    int test = GetCSIParam(0, 0);

    switch (test) {
        case 1: // Power-up self test
        case 2: // Data loop back test
        case 3: // EIA loop back test
        case 4: // Printer port loop back test
            // These tests would require hardware access
            // For software terminal, just acknowledge
            if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "DECTST test %d - not applicable", test);
                LogUnsupportedSequence(debug_msg);
            }
            break;

        default:
            if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown DECTST test: %d", test);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }
}

void ExecuteDECVERP(void) { // Verify Parity
    // DECVERP - Enable/disable parity verification
    // Not applicable to software terminals
    if (ACTIVE_SESSION.options.debug_sequences) {
        LogUnsupportedSequence("DECVERP - parity verification not applicable");
    }
}

// =============================================================================
// COMPREHENSIVE CSI SEQUENCE PROCESSING
// =============================================================================

// Complete the missing API functions from earlier phases
void ExecuteTBC(void) { // Tab Clear
    int n = GetCSIParam(0, 0);

    switch (n) {
        case 0: // Clear tab stop at current column
            ClearTabStop(ACTIVE_SESSION.cursor.x);
            break;
        case 3: // Clear all tab stops
            ClearAllTabStops();
            break;
    }
}

void ExecuteCTC(void) { // Cursor Tabulation Control
    int n = GetCSIParam(0, 0);

    switch (n) {
        case 0: // Set tab stop at current column
            SetTabStop(ACTIVE_SESSION.cursor.x);
            break;
        case 2: // Clear tab stop at current column
            ClearTabStop(ACTIVE_SESSION.cursor.x);
            break;
        case 5: // Clear all tab stops
            ClearAllTabStops();
            break;
    }
}

void ExecuteDECSN(void) {
    int session_id = GetCSIParam(0, 0);
    // If param is omitted (0 returned by GetCSIParam if 0 is default), VT520 DECSN usually defaults to 1.
    if (session_id == 0) session_id = 1;

    if (session_id >= 1 && session_id <= MAX_SESSIONS) {
        // Respect Multi-Session Mode Lock
        if (!ACTIVE_SESSION.conformance.features.multi_session_mode) {
            // If disabled, ignore request (or log it)
            if (ACTIVE_SESSION.options.debug_sequences) {
                char msg[64];
                snprintf(msg, sizeof(msg), "DECSN %d ignored: Multi-session mode disabled", session_id);
                LogUnsupportedSequence(msg);
            }
            return;
        }

        if (terminal.sessions[session_id - 1].session_open) {
            SetActiveSession(session_id - 1);
        } else {
            if (ACTIVE_SESSION.options.debug_sequences) {
                char msg[64];
                snprintf(msg, sizeof(msg), "DECSN %d ignored: Session not open", session_id);
                LogUnsupportedSequence(msg);
            }
        }
    }
}

// New ExecuteCSI_Dollar for multi-byte CSI $ sequences
void ExecuteCSI_Dollar(void) {
    // This function is now the central dispatcher for sequences with a '$' intermediate.
    // It looks for the character *after* the '$'.
    char* dollar_ptr = strchr(ACTIVE_SESSION.escape_buffer, '$');
    if (dollar_ptr && *(dollar_ptr + 1) != '\0') {
        char final_char = *(dollar_ptr + 1);
        switch (final_char) {
            case 'v':
                ExecuteDECCRA();
                break;
            case 'w':
                ExecuteDECRQCRA();
                break;
            case 'x':
                // DECERA and DECFRA share the same sequence ending ($x),
                // but are distinguished by parameter count.
                if (ACTIVE_SESSION.param_count == 4) {
                    ExecuteDECERA();
                } else if (ACTIVE_SESSION.param_count == 5) {
                    ExecuteDECFRA();
                } else {
                    LogUnsupportedSequence("Invalid parameters for DECERA/DECFRA");
                }
                break;
            case '{':
                ExecuteDECSERA();
                break;
            case 'u':
                ExecuteDECRQPSR();
                break;
            case 'q':
                ExecuteDECRQM();
                break;
            default:
                if (ACTIVE_SESSION.options.debug_sequences) {
                    char debug_msg[128];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown CSI $ sequence with final char '%c'", final_char);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
    } else {
         if (ACTIVE_SESSION.options.debug_sequences) {
            char debug_msg[MAX_COMMAND_BUFFER + 64];
            snprintf(debug_msg, sizeof(debug_msg), "Malformed CSI $ sequence in buffer: %s", ACTIVE_SESSION.escape_buffer);
            LogUnsupportedSequence(debug_msg);
        }
    }
}

void ProcessCSIChar(unsigned char ch) {
    if (ACTIVE_SESSION.parse_state != PARSE_CSI) return;

    if (ch >= 0x40 && ch <= 0x7E) {
        // Parse parameters into ACTIVE_SESSION.escape_params
        ParseCSIParams(ACTIVE_SESSION.escape_buffer, NULL, MAX_ESCAPE_PARAMS);

        // Handle DECSCUSR (CSI Ps SP q)
        if (ch == 'q' && ACTIVE_SESSION.escape_pos >= 1 && ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 1] == ' ') {
            ExecuteDECSCUSR();
        } else {
            // Dispatch to ExecuteCSICommand
            ExecuteCSICommand(ch);
        }

        // Reset parser state
        ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
        ClearCSIParams();
    } else if (ch >= 0x20 && ch <= 0x3F) {
        // Accumulate intermediate characters (e.g., digits, ';', '?')
        if (ACTIVE_SESSION.escape_pos < MAX_COMMAND_BUFFER - 1) {
            ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos++] = ch;
            ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos] = '\0';
        } else {
            if (ACTIVE_SESSION.options.debug_sequences) {
                fprintf(stderr, "CSI escape buffer overflow\n");
            }
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            ClearCSIParams();
        }
    } else if (ch == '$') {
        // Handle multi-byte CSI sequences (e.g., CSI $ q, CSI $ u)
        if (ACTIVE_SESSION.escape_pos < MAX_COMMAND_BUFFER - 1) {
            ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos++] = ch;
            ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos] = '\0';
        } else {
            if (ACTIVE_SESSION.options.debug_sequences) {
                fprintf(stderr, "CSI escape buffer overflow\n");
            }
            ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            ClearCSIParams();
        }
    } else {
        // Invalid character
        if (ACTIVE_SESSION.options.debug_sequences) {
            snprintf(ACTIVE_SESSION.conformance.compliance.last_unsupported,
                     sizeof(ACTIVE_SESSION.conformance.compliance.last_unsupported),
                     "Invalid CSI char: 0x%02X", ch);
            ACTIVE_SESSION.conformance.compliance.unsupported_sequences++;
        }
        ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
        ClearCSIParams();
    }
}

// Updated ExecuteCSICommand
void ExecuteCSICommand(unsigned char command) {
    bool private_mode = (ACTIVE_SESSION.escape_buffer[0] == '?');

    // Statically initialized jump table using addresses of labels and designated initializers.
    // This table is constructed at compile time.
    static const void* const csi_dispatch_table[256] = {
        // Default for all 256 entries is L_CSI_UNSUPPORTED.
        // Then, specific characters override their entry.
        // This requires C99 or later for designated initializers.
        // GCC/Clang support this well.

        // Initialize all to unsupported first (conceptually)
        // The compiler will handle this if we only list the specified ones.
        // If an index is not specified, it's typically zero-initialized (NULL for pointers).
        // We need to handle NULL lookups or ensure all are L_CSI_UNSUPPORTED.
        // Let's explicitly initialize common ranges to UNSUPPORTED for clarity,
        // though just listing the active ones and having a NULL check is also fine.

        [0 ... 255] = &&L_CSI_UNSUPPORTED, // Default for all: jump to unsupported

        // C0 Controls (0x00 - 0x1F) - Should not typically be final CSI bytes, but for completeness
        // (these are usually handled by ProcessControlChar)
        // ... (can be left as L_CSI_UNSUPPORTED)

        // Printable ASCII & CSI Commands
        // Note: The labels must be defined within this function.

        ['A'] = &&L_CSI_A_CUU,          // Cursor Up
        ['B'] = &&L_CSI_B_CUD,          // Cursor Down
        ['C'] = &&L_CSI_C_CUF,          // Cursor Forward
        ['D'] = &&L_CSI_D_CUB,          // Cursor Back
        ['E'] = &&L_CSI_E_CNL,          // Cursor Next Line
        ['F'] = &&L_CSI_F_CPL,          // Cursor Previous Line
        ['G'] = &&L_CSI_G_CHA,          // Cursor Horizontal Absolute
        ['H'] = &&L_CSI_H_CUP,          // Cursor Position
        ['I'] = &&L_CSI_I_CHT,          // Cursor Horizontal Tabulation (Forward)
        ['J'] = &&L_CSI_J_ED,           // Erase in Display
        ['K'] = &&L_CSI_K_EL,           // Erase in Line
        ['L'] = &&L_CSI_L_IL,           // Insert Line(s)
        ['M'] = &&L_CSI_M_DL,           // Delete Line(s)
        // ['N'] (SS2), ['O'] (SS3) - Not CSI final bytes
        ['P'] = &&L_CSI_P_DCH,          // Delete Character(s)
        // ['Q'] - SDM (Select Display Mode) - Often not used/implemented
        ['S'] = &&L_CSI_S_SU,           // Scroll Up
        ['T'] = &&L_CSI_T_SD,           // Scroll Down (also DECRST/DECRSTS)
        // ['U'] - Not a standard CSI final byte
        // ['V'] - Not a standard CSI final byte
        ['W'] = &&L_CSI_W_CTC_etc,      // Cursor Tabulation Control (if private) / other
        ['X'] = &&L_CSI_X_ECH,          // Erase Character(s)
        // ['Y'] - Not a standard CSI final byte
        ['Z'] = &&L_CSI_Z_CBT,          // Cursor Backward Tabulation

        ['@'] = &&L_CSI_at_ASC,         // Insert Character(s)

        ['`'] = &&L_CSI_tick_HPA,       // Horizontal Position Absolute (same as G)
        ['a'] = &&L_CSI_a_HPR,          // Horizontal Position Relative
        ['b'] = &&L_CSI_b_REP,          // Repeat Preceding Character
        ['c'] = &&L_CSI_c_DA,           // Device Attributes
        ['d'] = &&L_CSI_d_VPA,          // Vertical Line Position Absolute
        ['e'] = &&L_CSI_e_VPR,          // Vertical Position Relative
        ['f'] = &&L_CSI_f_HVP,          // Horizontal and Vertical Position (same as H)
        ['g'] = &&L_CSI_g_TBC,          // Tabulation Clear
        ['h'] = &&L_CSI_h_SM,           // Set Mode
        ['i'] = &&L_CSI_i_MC,           // Media Copy
        ['j'] = &&L_CSI_j_HPB,          // Horizontal Position Backward (like CUB)
        ['k'] = &&L_CSI_k_VPB,          // Vertical Position Backward (like CUU)
        ['l'] = &&L_CSI_l_RM,           // Reset Mode
        ['m'] = &&L_CSI_m_SGR,          // Select Graphic Rendition
        ['n'] = &&L_CSI_n_DSR,          // Device Status Report
        ['o'] = &&L_CSI_o_VT420,        // VT420 specific (e.g., DECDMAC)
        ['p'] = &&L_CSI_p_DECSOFT_etc,  // Soft Key assignments, DECSTR, DECSCL, etc.
        ['q'] = &&L_CSI_q_DECLL_DECSCUSR, // Load LEDs (private) / Set Cursor Style
        ['r'] = &&L_CSI_r_DECSTBM,      // Set Top and Bottom Margins (Scrolling Region)
        ['s'] = &&L_CSI_s_SAVRES_CUR,   // Save Cursor (ANSI) / Set Left/Right Margins (private VT420)
        ['t'] = &&L_CSI_t_WINMAN,       // Window Manipulation (xterm) / DECSLPP
        ['u'] = &&L_CSI_u_RES_CUR,      // Restore Cursor (ANSI)
        ['v'] = &&L_CSI_v_RECTCOPY,     // Rectangular Area Copy (DECCRA - private)
        ['w'] = &&L_CSI_w_RECTCHKSUM,   // Request Rectangular Area Checksum (DECRQCRA - private)
        ['x'] = &&L_CSI_x_DECREQTPARM,  // Request Terminal Parameters
        ['y'] = &&L_CSI_y_DECTST,       // Invoke Confidence Test
        ['z'] = &&L_CSI_z_DECVERP,      // Verify Parity (private)

        ['{'] = &&L_CSI_LSBrace_DECSLE, // Select Locator Events (private)
        ['|'] = &&L_CSI_Pipe_DECRQLP,   // Request Locator Position (private)
        ['}'] = &&L_CSI_RSBrace_VT420,  // VT420 specific (e.g., DECELR, DECERA)
        ['~'] = &&L_CSI_Tilde_VT420,    // VT420 specific (e.g., DECSERA, DECFRA)
        ['$'] = &&L_CSI_dollar_multi,
        ['P'] = &&L_CSI_P_DCS           // Device Control String
    };

    // --- Special handling for sequences with intermediate characters BEFORE dispatch ---
    // This is tricky with a simple final-char dispatch. The logic for 'q' is one attempt.
    if (command == 'q' && ACTIVE_SESSION.escape_pos >= 1 && ACTIVE_SESSION.escape_buffer[ACTIVE_SESSION.escape_pos - 1] == ' ') {
        // This is DECSCUSR: CSI Ps SP q.
        // ParseCSIParams was already called on the whole buffer by ProcessCSIChar.
        // We need to ensure it correctly parsed parameters *before* " SP q".
        // If GetCSIParam correctly extracts numbers before non-numeric parts, this might work.
        goto L_CSI_SP_q_DECSCUSR; // Specific handler for DECSCUSR via space
    }

    // Handle DCS sequences (e.g., DCS 0 ; 0 $ t <message> ST)
    if (command == 'P') {
        if (strstr(ACTIVE_SESSION.escape_buffer, "$t")) {
            ExecuteDCSAnswerback();
            goto L_CSI_END;
        } else {
            if (ACTIVE_SESSION.options.debug_sequences) {
                LogUnsupportedSequence("Unknown DCS sequence");
            }
        }
        goto L_CSI_END;
    }

    // Dispatch using the computed goto table
    const void* target_label = csi_dispatch_table[command];
    goto *target_label;

// --- Label Blocks for each CSI Command ---
// Format: LABEL_NAME: Action; // Comment (VT sequence if applicable)

L_CSI_A_CUU:          ExecuteCUU(); goto L_CSI_END;                      // CUU - Cursor Up (CSI Pn A)
L_CSI_B_CUD:          ExecuteCUD(); goto L_CSI_END;                      // CUD - Cursor Down (CSI Pn B)
L_CSI_C_CUF:          ExecuteCUF(); goto L_CSI_END;                      // CUF - Cursor Forward (CSI Pn C)
L_CSI_D_CUB:          ExecuteCUB(); goto L_CSI_END;                      // CUB - Cursor Back (CSI Pn D)
L_CSI_E_CNL:          ExecuteCNL(); goto L_CSI_END;                      // CNL - Cursor Next Line (CSI Pn E)
L_CSI_F_CPL:          ExecuteCPL(); goto L_CSI_END;                      // CPL - Cursor Previous Line (CSI Pn F)
L_CSI_G_CHA:          ExecuteCHA(); goto L_CSI_END;                      // CHA - Cursor Horizontal Absolute (CSI Pn G)
L_CSI_H_CUP:          ExecuteCUP(); goto L_CSI_END;                      // CUP - Cursor Position (CSI Pn ; Pn H)
L_CSI_f_HVP:          ExecuteCUP(); goto L_CSI_END;                      // HVP - Horizontal and Vertical Position (CSI Pn ; Pn f) (Same as CUP)
L_CSI_d_VPA:          ExecuteVPA(); goto L_CSI_END;                      // VPA - Vertical Line Position Absolute (CSI Pn d)
L_CSI_tick_HPA:       ExecuteCHA(); goto L_CSI_END;                      // HPA - Horizontal Position Absolute (CSI Pn `) (Same as CHA)
L_CSI_I_CHT:          { int n=GetCSIParam(0,1); while(n-->0) ACTIVE_SESSION.cursor.x = NextTabStop(ACTIVE_SESSION.cursor.x); if (ACTIVE_SESSION.cursor.x >= DEFAULT_TERM_WIDTH) ACTIVE_SESSION.cursor.x = DEFAULT_TERM_WIDTH -1; } goto L_CSI_END; // CHT - Cursor Horizontal Tab (CSI Pn I)
L_CSI_J_ED:           ExecuteED(private_mode);  goto L_CSI_END;          // ED  - Erase in Display (CSI Pn J) / DECSED (CSI ? Pn J)
L_CSI_K_EL:           ExecuteEL(private_mode);  goto L_CSI_END;          // EL  - Erase in Line (CSI Pn K) / DECSEL (CSI ? Pn K)
L_CSI_L_IL:           ExecuteIL();  goto L_CSI_END;                      // IL  - Insert Line(s) (CSI Pn L)
L_CSI_M_DL:           ExecuteDL();  goto L_CSI_END;                      // DL  - Delete Line(s) (CSI Pn M)
L_CSI_P_DCH:          ExecuteDCH(); goto L_CSI_END;                      // DCH - Delete Character(s) (CSI Pn P)
L_CSI_S_SU:           ExecuteSU();  goto L_CSI_END;                      // SU  - Scroll Up (CSI Pn S)
L_CSI_T_SD:           ExecuteSD();  goto L_CSI_END;                      // SD  - Scroll Down (CSI Pn T)
L_CSI_W_CTC_etc:      if(private_mode) ExecuteCTC(); else LogUnsupportedSequence("CSI W (non-private)"); goto L_CSI_END; // CTC - Cursor Tab Control (CSI ? Ps W)
L_CSI_X_ECH:          ExecuteECH(); goto L_CSI_END;                      // ECH - Erase Character(s) (CSI Pn X)
L_CSI_Z_CBT:          { int n=GetCSIParam(0,1); while(n-->0) ACTIVE_SESSION.cursor.x = PreviousTabStop(ACTIVE_SESSION.cursor.x); } goto L_CSI_END; // CBT - Cursor Backward Tab (CSI Pn Z)
L_CSI_at_ASC:         ExecuteICH(); goto L_CSI_END;                      // ICH - Insert Character(s) (CSI Pn @)
L_CSI_a_HPR:          { int n=GetCSIParam(0,1); ACTIVE_SESSION.cursor.x+=n; if(ACTIVE_SESSION.cursor.x<0)ACTIVE_SESSION.cursor.x=0; if(ACTIVE_SESSION.cursor.x>=DEFAULT_TERM_WIDTH)ACTIVE_SESSION.cursor.x=DEFAULT_TERM_WIDTH-1;} goto L_CSI_END; // HPR - Horizontal Position Relative (CSI Pn a)
L_CSI_b_REP:          ExecuteREP(); goto L_CSI_END;                      // REP - Repeat Preceding Graphic Character (CSI Pn b)
L_CSI_c_DA:           ExecuteDA(private_mode); goto L_CSI_END;           // DA  - Device Attributes (CSI Ps c or CSI ? Ps c)
L_CSI_e_VPR:          { int n=GetCSIParam(0,1); ACTIVE_SESSION.cursor.y+=n; if(ACTIVE_SESSION.cursor.y<0)ACTIVE_SESSION.cursor.y=0; if(ACTIVE_SESSION.cursor.y>=DEFAULT_TERM_HEIGHT)ACTIVE_SESSION.cursor.y=DEFAULT_TERM_HEIGHT-1;} goto L_CSI_END; // VPR - Vertical Position Relative (CSI Pn e)
L_CSI_g_TBC:          ExecuteTBC(); goto L_CSI_END;                      // TBC - Tabulation Clear (CSI Ps g)
L_CSI_h_SM:           ExecuteSM(private_mode); goto L_CSI_END;           // SM  - Set Mode (CSI ? Pm h or CSI Pm h)
L_CSI_i_MC:
            {
                int param = GetCSIParam(0, 0);
                if (private_mode) {
                     // CSI ? 4 i (Auto Print off), ? 5 i (Auto Print on)
                     if (param == 4) ACTIVE_SESSION.auto_print_enabled = false;
                     else if (param == 5) ACTIVE_SESSION.auto_print_enabled = true;
                } else {
                     // CSI 4 i (Printer Controller off), 5 i (Printer Controller on), 0 i (Print Screen)
                     if (param == 0) {
                         // Print Screen (Stub: Log or Trigger Callback)
                         if (ACTIVE_SESSION.options.debug_sequences) {
                             LogUnsupportedSequence("Print Screen requested (no printer)");
                         }
                     } else if (param == 4) {
                         ACTIVE_SESSION.printer_controller_enabled = false;
                     } else if (param == 5) {
                         ACTIVE_SESSION.printer_controller_enabled = true;
                     }
                }
            }
            goto L_CSI_END; // MC - Media Copy (CSI Pn i or CSI ? Pn i)
L_CSI_j_HPB:          ExecuteCUB(); goto L_CSI_END;                      // HPB - Horizontal Position Backward (like CUB) (CSI Pn j)
L_CSI_k_VPB:          ExecuteCUU(); goto L_CSI_END;                      // VPB - Vertical Position Backward (like CUU) (CSI Pn k)
L_CSI_l_RM:           ExecuteRM(private_mode); goto L_CSI_END;           // RM  - Reset Mode (CSI ? Pm l or CSI Pm l)
L_CSI_m_SGR:          ExecuteSGR(); goto L_CSI_END;                      // SGR - Select Graphic Rendition (CSI Pm m)
L_CSI_n_DSR:          ExecuteDSR(); goto L_CSI_END;                      // DSR - Device Status Report (CSI Ps n or CSI ? Ps n)
L_CSI_o_VT420:        if(ACTIVE_SESSION.options.debug_sequences) LogUnsupportedSequence("VT420 'o'"); goto L_CSI_END; // DECDMAC, etc. (CSI Pt;Pb;Pl;Pr;Pp;Pattr o)
L_CSI_p_DECSOFT_etc:  ExecuteCSI_P(); goto L_CSI_END;                   // Various 'p' suffixed: DECSTR, DECSCL, DECRQM, DECUDK (CSI ! p, CSI " p, etc.)
L_CSI_q_DECLL_DECSCUSR: if(strstr(ACTIVE_SESSION.escape_buffer, "\"")) ExecuteDECSCA(); else if(private_mode) ExecuteDECLL(); else ExecuteDECSCUSR(); goto L_CSI_END; // DECSCA / DECLL / DECSCUSR
L_CSI_r_DECSTBM:      if(!private_mode) ExecuteDECSTBM(); else LogUnsupportedSequence("CSI ? r invalid"); goto L_CSI_END; // DECSTBM - Set Top/Bottom Margins (CSI Pt ; Pb r)
// Save Cursor: uses per-session logic via ACTIVE_SESSION macro
L_CSI_s_SAVRES_CUR:   if(private_mode){if(ACTIVE_SESSION.conformance.features.vt420_mode) ExecuteDECSLRM(); else LogUnsupportedSequence("DECSLRM requires VT420");} else { ExecuteSaveCursor(); } goto L_CSI_END; // DECSLRM (private VT420+) / Save Cursor (ANSI.SYS) (CSI s / CSI ? Pl ; Pr s)
L_CSI_t_WINMAN:       ExecuteWindowOps(); goto L_CSI_END;                // Window Manipulation (xterm) / DECSLPP (Set lines per page) (CSI Ps t)
L_CSI_u_RES_CUR:      ExecuteRestoreCursor(); goto L_CSI_END;            // Restore Cursor (ANSI.SYS) (CSI u)
L_CSI_v_RECTCOPY:     if(strstr(ACTIVE_SESSION.escape_buffer, "$")) ExecuteDECCRA(); else if(private_mode) ExecuteRectangularOps(); else LogUnsupportedSequence("CSI v non-private invalid"); goto L_CSI_END; // DECCRA
L_CSI_w_RECTCHKSUM:   if(strstr(ACTIVE_SESSION.escape_buffer, "$")) ExecuteDECRQCRA(); else if(private_mode) ExecuteRectangularOps2(); else LogUnsupportedSequence("CSI w non-private invalid"); goto L_CSI_END; // DECRQCRA
L_CSI_x_DECREQTPARM:  if(strstr(ACTIVE_SESSION.escape_buffer, "$")) ExecuteDECFRA(); else ExecuteDECREQTPARM(); goto L_CSI_END;             // DECFRA / DECREQTPARM
L_CSI_y_DECTST:       ExecuteDECTST(); goto L_CSI_END;                   // DECTST
L_CSI_z_DECVERP:      if(strstr(ACTIVE_SESSION.escape_buffer, "$")) ExecuteDECERA(); else if(private_mode) ExecuteDECVERP(); else LogUnsupportedSequence("CSI z non-private invalid"); goto L_CSI_END; // DECERA / DECVERP
L_CSI_LSBrace_DECSLE: if(strstr(ACTIVE_SESSION.escape_buffer, "$")) ExecuteDECSERA(); else ExecuteDECSLE(); goto L_CSI_END; // DECSERA / DECSLE
L_CSI_Pipe_DECRQLP:   ExecuteDECRQLP(); goto L_CSI_END; // DECRQLP - Request Locator Position (CSI Plc |)
L_CSI_RSBrace_VT420:
    if (strstr(ACTIVE_SESSION.escape_buffer, "$")) {
        ExecuteDECSASD();
    } else {
        LogUnsupportedSequence("CSI } invalid");
    }
    goto L_CSI_END;
L_CSI_Tilde_VT420:
    if (strstr(ACTIVE_SESSION.escape_buffer, "!")) {
        ExecuteDECSN();
    } else if (strstr(ACTIVE_SESSION.escape_buffer, "$")) {
        ExecuteDECSSDT();
    } else {
        LogUnsupportedSequence("CSI ~ invalid");
    }
    goto L_CSI_END;
L_CSI_dollar_multi:   ExecuteCSI_Dollar(); goto L_CSI_END;              // Multi-byte CSI sequences (e.g., CSI $ q, CSI $ u)
L_CSI_P_DCS:          // Device Control String (e.g., DCS 0 ; 0 $ t <message> ST)
    if (strstr(ACTIVE_SESSION.escape_buffer, "$t")) {
        ExecuteDCSAnswerback();
    } else {
        if (ACTIVE_SESSION.options.debug_sequences) {
            LogUnsupportedSequence("Unknown DCS sequence");
        }
    }
    goto L_CSI_END;

L_CSI_SP_q_DECSCUSR:  ExecuteDECSCUSR(); goto L_CSI_END;                  // DECSCUSR specific handler for CSI Ps SP q

L_CSI_UNSUPPORTED:
    if (ACTIVE_SESSION.options.debug_sequences) {
        char debug_msg[128];
        snprintf(debug_msg, sizeof(debug_msg),
                 "Unknown CSI %s%c (0x%02X) [computed goto default]", private_mode ? "?" : "", command, command);
        LogUnsupportedSequence(debug_msg);
    }
    ACTIVE_SESSION.conformance.compliance.unsupported_sequences++;
    // Fall through to L_CSI_END

L_CSI_END:
    return; // Common exit point for the function.
}

// =============================================================================
// OSC (OPERATING SYSTEM COMMAND) PROCESSING
// =============================================================================


void VTSituationSetWindowTitle(const char* title) {
    strncpy(ACTIVE_SESSION.title.window_title, title, MAX_TITLE_LENGTH - 1);
    ACTIVE_SESSION.title.window_title[MAX_TITLE_LENGTH - 1] = '\0';
    ACTIVE_SESSION.title.title_changed = true;

    if (title_callback) {
        title_callback(ACTIVE_SESSION.title.window_title, false);
    }

    // Also set Situation window title
    SituationSetWindowTitle(ACTIVE_SESSION.title.window_title);
}

void SetIconTitle(const char* title) {
    strncpy(ACTIVE_SESSION.title.icon_title, title, MAX_TITLE_LENGTH - 1);
    ACTIVE_SESSION.title.icon_title[MAX_TITLE_LENGTH - 1] = '\0';
    ACTIVE_SESSION.title.icon_changed = true;

    if (title_callback) {
        title_callback(ACTIVE_SESSION.title.icon_title, true);
    }
}

void ResetForegroundColor(void) {
    ACTIVE_SESSION.current_fg.color_mode = 0;
    ACTIVE_SESSION.current_fg.value.index = COLOR_WHITE;
}

void ResetBackgroundColor(void) {
    ACTIVE_SESSION.current_bg.color_mode = 0;
    ACTIVE_SESSION.current_bg.value.index = COLOR_BLACK;
}

void ResetCursorColor(void) {
    ACTIVE_SESSION.cursor.color.color_mode = 0;
    ACTIVE_SESSION.cursor.color.value.index = COLOR_WHITE;
}

void ProcessColorCommand(const char* data) {
    // Format: color_index;rgb:rr/gg/bb or color_index;?
    char* semicolon = strchr(data, ';');
    if (!semicolon) return;

    int color_index = atoi(data);
    char* color_spec = semicolon + 1;

    if (color_spec[0] == '?') {
        // Query color
        char response[128];
        if (color_index >= 0 && color_index < 256) {
            RGB_Color c = color_palette[color_index];
            snprintf(response, sizeof(response), "\x1B]4;%d;rgb:%02x/%02x/%02x\x1B\\",
                    color_index, c.r, c.g, c.b);
            QueueResponse(response);
        }
    } else if (strncmp(color_spec, "rgb:", 4) == 0) {
        // Set color: rgb:rr/gg/bb
        unsigned int r, g, b;
        if (sscanf(color_spec + 4, "%02x/%02x/%02x", &r, &g, &b) == 3) {
            if (color_index >= 0 && color_index < 256) {
                color_palette[color_index] = (RGB_Color){r, g, b, 255};
            }
        }
    }
}

// Additional helper functions for OSC commands
void ResetColorPalette(const char* data) {
    if (!data || strlen(data) == 0) {
        // Reset entire palette
        InitColorPalette();
    } else {
        // Reset specific colors (comma-separated list)
        char* data_copy = strdup(data);
        char* token = strtok(data_copy, ";");

        while (token != NULL) {
            int color_index = atoi(token);
            if (color_index >= 0 && color_index < 16) {
                // Reset to default ANSI color
                color_palette[color_index] = (RGB_Color){
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

void ProcessForegroundColorCommand(const char* data) {
    if (data[0] == '?') {
        // Query foreground color
        char response[64];
        ExtendedColor fg = ACTIVE_SESSION.current_fg;
        if (fg.color_mode == 0 && fg.value.index < 16) {
            RGB_Color c = color_palette[fg.value.index];
            snprintf(response, sizeof(response), "\x1B]10;rgb:%02x/%02x/%02x\x1B\\", c.r, c.g, c.b);
        } else if (fg.color_mode == 1) {
            snprintf(response, sizeof(response), "\x1B]10;rgb:%02x/%02x/%02x\x1B\\",
                    fg.value.rgb.r, fg.value.rgb.g, fg.value.rgb.b);
        }
        QueueResponse(response);
    }
    // Setting foreground via OSC is less common, usually done via SGR
}

void ProcessBackgroundColorCommand(const char* data) {
    if (data[0] == '?') {
        // Query background color
        char response[64];
        ExtendedColor bg = ACTIVE_SESSION.current_bg;
        if (bg.color_mode == 0 && bg.value.index < 16) {
            RGB_Color c = color_palette[bg.value.index];
            snprintf(response, sizeof(response), "\x1B]11;rgb:%02x/%02x/%02x\x1B\\", c.r, c.g, c.b);
        } else if (bg.color_mode == 1) {
            snprintf(response, sizeof(response), "\x1B]11;rgb:%02x/%02x/%02x\x1B\\",
                    bg.value.rgb.r, bg.value.rgb.g, bg.value.rgb.b);
        }
        QueueResponse(response);
    }
}

void ProcessCursorColorCommand(const char* data) {
    if (data[0] == '?') {
        // Query cursor color
        char response[64];
        ExtendedColor cursor_color = ACTIVE_SESSION.cursor.color;
        if (cursor_color.color_mode == 0 && cursor_color.value.index < 16) {
            RGB_Color c = color_palette[cursor_color.value.index];
            snprintf(response, sizeof(response), "\x1B]12;rgb:%02x/%02x/%02x\x1B\\", c.r, c.g, c.b);
        } else if (cursor_color.color_mode == 1) {
            snprintf(response, sizeof(response), "\x1B]12;rgb:%02x/%02x/%02x\x1B\\",
                    cursor_color.value.rgb.r, cursor_color.value.rgb.g, cursor_color.value.rgb.b);
        }
        QueueResponse(response);
    }
}

void ProcessFontCommand(const char* data) {
    // Font selection - simplified implementation
    if (ACTIVE_SESSION.options.debug_sequences) {
        LogUnsupportedSequence("Font selection not fully implemented");
    }
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

void ProcessClipboardCommand(const char* data) {
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
        if (SituationGetClipboardText(&clipboard_text) == SITUATION_SUCCESS && clipboard_text) {
            size_t text_len = strlen(clipboard_text);
            size_t encoded_len = 4 * ((text_len + 2) / 3) + 1;
            char* encoded_data = malloc(encoded_len);
            if (encoded_data) {
                EncodeBase64((const unsigned char*)clipboard_text, text_len, encoded_data, encoded_len);
                char response_header[16];
                snprintf(response_header, sizeof(response_header), "\x1B]52;%c;", clipboard_selector);
                QueueResponse(response_header);
                QueueResponse(encoded_data);
                QueueResponse("\x1B\\");
                free(encoded_data);
            }
            SituationFreeString((char*)clipboard_text);
        } else {
            // Empty clipboard response
            char response[16];
            snprintf(response, sizeof(response), "\x1B]52;%c;\x1B\\", clipboard_selector);
            QueueResponse(response);
        }
    } else {
        // Set clipboard data (base64 encoded)
        // We only support setting the standard clipboard ('c' or '0')
        if (clipboard_selector == 'c' || clipboard_selector == '0') {
            size_t decoded_size = strlen(pd_str); // Upper bound
            unsigned char* decoded_data = malloc(decoded_size + 1);
            if (decoded_data) {
                DecodeBase64(pd_str, decoded_data, decoded_size + 1);
                SituationSetClipboardText((const char*)decoded_data);
                free(decoded_data);
            }
        }
    }

    free(data_copy);
}

void ExecuteOSCCommand(void) {
    char* params = ACTIVE_SESSION.escape_buffer;

    // Find the command number
    char* semicolon = strchr(params, ';');
    if (!semicolon) {
        LogUnsupportedSequence("Malformed OSC sequence");
        return;
    }

    *semicolon = '\0';
    int command = atoi(params);
    char* data = semicolon + 1;

    switch (command) {
        case 0: // Set window and icon title
        case 2: // Set window title
            VTSituationSetWindowTitle(data);
            break;

        case 1: // Set icon title
            SetIconTitle(data);
            break;

        case 9: // Notification
            if (terminal.notification_callback) {
                terminal.notification_callback(data);
            }
            break;

        case 4: // Set/query color palette
            ProcessColorCommand(data);
            break;

        case 10: // Query/set foreground color
            ProcessForegroundColorCommand(data);
            break;

        case 11: // Query/set background color
            ProcessBackgroundColorCommand(data);
            break;

        case 12: // Query/set cursor color
            ProcessCursorColorCommand(data);
            break;

        case 50: // Set font
            ProcessFontCommand(data);
            break;

        case 52: // Clipboard operations
            ProcessClipboardCommand(data);
            break;

        case 104: // Reset color palette
            ResetColorPalette(data);
            break;

        case 110: // Reset foreground color
            ResetForegroundColor();
            break;

        case 111: // Reset background color
            ResetBackgroundColor();
            break;

        case 112: // Reset cursor color
            ResetCursorColor();
            break;

        default:
            if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[128];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown OSC command: %d", command);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }
}

// =============================================================================
// DCS (DEVICE CONTROL STRING) PROCESSING
// =============================================================================

void ProcessTermcapRequest(const char* request) {
    // XTGETTCAP - Get terminal capability
    // This is an xterm extension for querying termcap/terminfo values

    char response[256];

    if (strcmp(request, "Co") == 0) {
        // Number of colors
        snprintf(response, sizeof(response), "\x1BP1+r436f=323536\x1B\\"); // "256" in hex
    } else if (strcmp(request, "lines") == 0) {
        // Number of lines
        char hex_lines[16];
        snprintf(hex_lines, sizeof(hex_lines), "%X", DEFAULT_TERM_HEIGHT);
        snprintf(response, sizeof(response), "\x1BP1+r6c696e6573=%s\x1B\\", hex_lines);
    } else if (strcmp(request, "cols") == 0) {
        // Number of columns
        char hex_cols[16];
        snprintf(hex_cols, sizeof(hex_cols), "%X", DEFAULT_TERM_WIDTH);
        snprintf(response, sizeof(response), "\x1BP1+r636f6c73=%s\x1B\\", hex_cols);
    } else {
        // Unknown capability
        snprintf(response, sizeof(response), "\x1BP0+r%s\x1B\\", request);
    }

    QueueResponse(response);
}

// Helper function to convert a single hex character to its integer value
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1; // Invalid hex char
}

void DefineUserKey(int key_code, const char* sequence, size_t sequence_len) {
    // Expand programmable keys array if needed
    if (ACTIVE_SESSION.programmable_keys.count >= ACTIVE_SESSION.programmable_keys.capacity) {
        size_t new_capacity = ACTIVE_SESSION.programmable_keys.capacity == 0 ? 16 :
                             ACTIVE_SESSION.programmable_keys.capacity * 2;

        ProgrammableKey* new_keys = realloc(ACTIVE_SESSION.programmable_keys.keys,
                                           new_capacity * sizeof(ProgrammableKey));
        if (!new_keys) return;

        ACTIVE_SESSION.programmable_keys.keys = new_keys;
        ACTIVE_SESSION.programmable_keys.capacity = new_capacity;
    }

    // Find existing key or add new one
    ProgrammableKey* key = NULL;
    for (size_t i = 0; i < ACTIVE_SESSION.programmable_keys.count; i++) {
        if (ACTIVE_SESSION.programmable_keys.keys[i].key_code == key_code) {
            key = &ACTIVE_SESSION.programmable_keys.keys[i];
            if (key->sequence) free(key->sequence); // Free old sequence
            break;
        }
    }

    if (!key) {
        key = &ACTIVE_SESSION.programmable_keys.keys[ACTIVE_SESSION.programmable_keys.count++];
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

void ProcessUserDefinedKeys(const char* data) {
    // Parse user defined key format: key/string;key/string;...
    // The string is a sequence of hexadecimal pairs.
    if (!ACTIVE_SESSION.conformance.features.user_defined_keys) {
        LogUnsupportedSequence("User defined keys require VT320 mode");
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
                LogUnsupportedSequence("Invalid hex string in DECUDK");
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
                DefineUserKey(key_code, decoded_sequence, decoded_len);
                free(decoded_sequence);
            }
        }
    }

    free(data_copy);
}

void ClearUserDefinedKeys(void) {
    for (size_t i = 0; i < ACTIVE_SESSION.programmable_keys.count; i++) {
        free(ACTIVE_SESSION.programmable_keys.keys[i].sequence);
    }
    ACTIVE_SESSION.programmable_keys.count = 0;
}

void ProcessSoftFontDownload(const char* data) {
    // DECDLD format: Pfn; Pcn; Pe; Pcm; w; h; ... {data}
    // Pfn: Font number (0 or 1)
    // Pcn: Starting character number
    // Pe: Erase control (0=erase all, 1=erase specific, 2=erase all)
    // Pcm: Character matrix size (0=15x12??, 1=13x8, 2=8x10, etc.) - We only support 8x16 effectively
    // w: Font width (1-80) - ignored, we force 8
    // h: Font height (1-24) - ignored, we force 16
    // data: Sixel-like encoded data

    if (!ACTIVE_SESSION.conformance.features.soft_fonts) {
        LogUnsupportedSequence("Soft fonts not supported");
        return;
    }

    char* data_copy = strdup(data);
    if (!data_copy) return;

    // Tokenize parameters using manual pointer arithmetic
    char* current_ptr = data_copy;
    char* token = NULL;
    int params[6] = {0};
    int param_idx = 0;

    // Parse up to 6 numeric parameters
    while (current_ptr && *current_ptr && param_idx < 6) {
        token = current_ptr;
        char* next_delim = strchr(current_ptr, ';');

        // Check if we hit the data start '{' in this segment
        char* brace = strchr(token, '{');
        if (brace && (!next_delim || brace < next_delim)) {
            // Found brace before next semicolon
            *brace = '\0';
            if (strlen(token) > 0) params[param_idx++] = atoi(token);
            // Move token pointer to start of data
            token = brace + 1;
            // Stop parameter parsing loop, 'token' now points to data
            current_ptr = NULL; // Signal to exit loop but keep token valid
            break;
        }

        if (next_delim) {
            *next_delim = '\0';
            current_ptr = next_delim + 1;
        } else {
            current_ptr = NULL;
        }

        params[param_idx++] = atoi(token);
    }

    // Update dimensions if provided
    if (param_idx >= 5) {
        int w = params[4];
        if (w > 0 && w <= 32) ACTIVE_SESSION.soft_font.char_width = w;
    }
    if (param_idx >= 6) {
        int h = params[5];
        if (h > 0 && h <= 32) ACTIVE_SESSION.soft_font.char_height = h;
    }

    // Parse sixel-encoded font data
    if (token) {
        int current_char = (param_idx >= 2) ? params[1] : 0;
        unsigned char* data_ptr = (unsigned char*)token;
        int sixel_row_base = 0; // 0, 6, 12...
        int current_col = 0;

        // Clear current char matrix before starting
        if (current_char < 256) {
            memset(ACTIVE_SESSION.soft_font.font_data[current_char], 0, 32);
        }

        while (*data_ptr != '\0') {
            unsigned char ch = *data_ptr;

            if (ch == '/' || ch == ';') {
                // End of character
                if (current_char < 256) {
                    ACTIVE_SESSION.soft_font.loaded[current_char] = true;
                }
                current_char++;
                if (current_char >= 256) break;

                // Reset for next char
                memset(ACTIVE_SESSION.soft_font.font_data[current_char], 0, 32);
                sixel_row_base = 0;
                current_col = 0;

            } else if (ch == '-') {
                // New sixel row (move down 6 pixels)
                sixel_row_base += 6;
                current_col = 0;

            } else if (ch >= 63 && ch <= 126) {
                // Sixel data byte
                if (current_char < 256 && current_col < 8) { // Assuming 8px width
                    int val = ch - 63;
                    // Map 6 vertical bits to the bitmap
                    for (int b = 0; b < 6; b++) {
                        int pixel_y = sixel_row_base + b;
                        if (pixel_y < 16) { // Limit to 16px height
                            if ((val >> b) & 1) {
                                // Set bit (7 - current_col) in row pixel_y
                                ACTIVE_SESSION.soft_font.font_data[current_char][pixel_y] |= (1 << (7 - current_col));
                            }
                        }
                    }
                    current_col++;
                }
            }
            // Ignore other chars (CR/LF/space)
            data_ptr++;
        }

        // Mark last char as loaded if we processed some data
        if (current_char < 256) {
            ACTIVE_SESSION.soft_font.loaded[current_char] = true;
        }

        ACTIVE_SESSION.soft_font.active = true;
        CreateFontTexture();

        if (ACTIVE_SESSION.options.debug_sequences) {
            LogUnsupportedSequence("Soft font downloaded and active");
        }
    }

    free(data_copy);
}

void ProcessStatusRequest(const char* request) {
    // DECRQSS - Request Status String
    char response[128];

    if (strcmp(request, "m") == 0) {
        // Request SGR status
        snprintf(response, sizeof(response), "\x1BPm%dm\x1B\\", 0); // Simplified
        QueueResponse(response);
    } else if (strcmp(request, "r") == 0) {
        // Request scrolling region
        snprintf(response, sizeof(response), "\x1BPr%d;%dr\x1B\\",
                ACTIVE_SESSION.scroll_top + 1, ACTIVE_SESSION.scroll_bottom + 1);
        QueueResponse(response);
    } else {
        // Unknown request
        snprintf(response, sizeof(response), "\x1BP0$r%s\x1B\\", request);
        QueueResponse(response);
    }
}

// New ExecuteDCSAnswerback for DCS 0 ; 0 $ t <message> ST
void ExecuteDCSAnswerback(void) {
    char* message_start = strstr(ACTIVE_SESSION.escape_buffer, "$t");
    if (message_start) {
        message_start += 2; // Skip "$t"
        char* message_end = strstr(message_start, "\x1B\\"); // Find ST
        if (message_end) {
            size_t length = message_end - message_start;
            if (length >= MAX_COMMAND_BUFFER) {
                length = MAX_COMMAND_BUFFER - 1; // Prevent overflow
            }
            strncpy(ACTIVE_SESSION.answerback_buffer, message_start, length);
            ACTIVE_SESSION.answerback_buffer[length] = '\0';
        } else if (ACTIVE_SESSION.options.debug_sequences) {
            LogUnsupportedSequence("Incomplete DCS $ t sequence");
        }
    } else if (ACTIVE_SESSION.options.debug_sequences) {
        LogUnsupportedSequence("Invalid DCS $ t sequence");
    }
}

static void ParseGatewayCommand(const char* data, size_t len) {
    if (!data || len == 0) return;

    // Basic Parsing of "CLASS;ID;CMD;..."
    // Example: MAT;1;SET;COLOR;RED
    char buffer[256]; // Temporary buffer for parsing tokens
    size_t pos = 0;

    // Extract Class (MAT, GEO, LOG, etc.)
    char class_token[4] = {0};
    while (pos < len && pos < 3 && isalpha(data[pos])) {
        class_token[pos] = data[pos];
        pos++;
    }

    if (pos < len && data[pos] == ';') pos++; // Skip separator

    if (ACTIVE_SESSION.options.debug_sequences) {
    }

    if (strcmp(class_token, "MAT") == 0) {
        // Material Resource
    } else if (strcmp(class_token, "GEO") == 0) {
        // Geometry Resource
    } else if (strcmp(class_token, "LOG") == 0) {
        // Logic/Shader Resource
    }
}

void ExecuteDCSCommand(void) {
    char* params = ACTIVE_SESSION.escape_buffer;

    if (strncmp(params, "1;1|", 4) == 0) {
        // DECUDK - User Defined Keys
        ProcessUserDefinedKeys(params + 4);
    } else if (strncmp(params, "0;1|", 4) == 0) {
        // DECUDK - Clear User Defined Keys
        ClearUserDefinedKeys();
    } else if (strncmp(params, "2;1|", 4) == 0) {
        // DECDLD - Download Soft Font (Variant?)
        ProcessSoftFontDownload(params + 4);
    } else if (strstr(params, "{") != NULL) {
        // Standard DECDLD - Download Soft Font (DCS ... { ...)
        // We pass the whole string, ProcessSoftFontDownload will handle tokenization
        ProcessSoftFontDownload(params);
    } else if (strncmp(params, "$q", 2) == 0) {
        // DECRQSS - Request Status String
        ProcessStatusRequest(params + 2);
    } else if (strncmp(params, "+q", 2) == 0) {
        // XTGETTCAP - Get Termcap
        ProcessTermcapRequest(params + 2);
    } else if (strncmp(params, "GATE", 4) == 0) {
        // Gateway Protocol
        // Format: DCS GATE <Class> ; <ID> ; <Command> ... ST
        // Skip "GATE" (4 bytes) and any immediate separator if present
        const char* payload = params + 4;
        if (*payload == ';') payload++;
        ParseGatewayCommand(payload, strlen(payload));
    } else {
        if (ACTIVE_SESSION.options.debug_sequences) {
            LogUnsupportedSequence("Unknown DCS command");
        }
    }
}

// =============================================================================
// VT52 COMPATIBILITY MODE
// =============================================================================

void ProcessHashChar(unsigned char ch) {
    // DEC Line Attributes (ESC # Pn)

    // These commands apply to the *entire line* containing the active position.
    // In a real hardware terminal, this changes the scan-out logic.
    // Here, we set flags on all characters in the current row.
    // Note: Using DEFAULT_TERM_WIDTH assumes fixed-width allocation, which matches
    // the current implementation of 'screen' in ACTIVE_SESSION.h. If dynamic resizing is
    // added, this should iterate up to ACTIVE_SESSION.width or similar.

    switch (ch) {
        case '3': // DECDHL - Double-height line, top half
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_height_top = true;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_height_bottom = false;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_width = true; // Usually implies double width too
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->dirty = true;
            }
            ACTIVE_SESSION.row_dirty[ACTIVE_SESSION.cursor.y] = true;
            break;

        case '4': // DECDHL - Double-height line, bottom half
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_height_top = false;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_height_bottom = true;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_width = true;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->dirty = true;
            }
            ACTIVE_SESSION.row_dirty[ACTIVE_SESSION.cursor.y] = true;
            break;

        case '5': // DECSWL - Single-width single-height line
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_height_top = false;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_height_bottom = false;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_width = false;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->dirty = true;
            }
            ACTIVE_SESSION.row_dirty[ACTIVE_SESSION.cursor.y] = true;
            break;

        case '6': // DECDWL - Double-width single-height line
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_height_top = false;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_height_bottom = false;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->double_width = true;
                GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x)->dirty = true;
            }
            ACTIVE_SESSION.row_dirty[ACTIVE_SESSION.cursor.y] = true;
            break;

        case '8': // DECALN - Screen Alignment Pattern
            // Fill screen with 'E'
            for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(&ACTIVE_SESSION, y, x);
                    cell->ch = 'E';
                    cell->fg_color = ACTIVE_SESSION.current_fg;
                    cell->bg_color = ACTIVE_SESSION.current_bg;
                    // Reset attributes
                    cell->bold = false;
                    cell->faint = false;
                    cell->italic = false;
                    cell->underline = false;
                    cell->blink = false;
                    cell->reverse = false;
                    cell->strikethrough = false;
                    cell->conceal = false;
                    cell->overline = false;
                    cell->double_underline = false;
                    cell->double_width = false;
                    cell->double_height_top = false;
                    cell->double_height_bottom = false;
                    cell->dirty = true;
                }
            }
            ACTIVE_SESSION.cursor.x = 0;
            ACTIVE_SESSION.cursor.y = 0;
            break;

        default:
            if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC # %c", ch);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }

    ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
}

void ProcessPercentChar(unsigned char ch) {
    // ISO 2022 Select Character Set (ESC % P)

    switch (ch) {
        case '@': // Select default (ISO 8859-1)
            ACTIVE_SESSION.charset.g0 = CHARSET_ISO_LATIN_1;
            ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g0;
            // Technically this selects the 'return to default' for ISO 2022.
            break;

        case 'G': // Select UTF-8 (ISO 2022 standard for UTF-8 level 1/2/3)
            ACTIVE_SESSION.charset.g0 = CHARSET_UTF8;
            ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g0;
            break;

        default:
             if (ACTIVE_SESSION.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC %% %c", ch);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }

    ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
}

static void ReGIS_DrawLine(int x0, int y0, int x1, int y1) {
    if (terminal.vector_count < terminal.vector_capacity) {
        GPUVectorLine* line = &terminal.vector_staging_buffer[terminal.vector_count];

        // Aspect Ratio Correction
        // Map 800x480 ReGIS space to the center of the 1056x800 window while maintaining aspect ratio.
        // ReGIS AR: 1.666 (5:3)
        // Window AR: 1.32
        // We are width-limited if we want to fill width, or height-limited if we want to fill height?
        // Actually, we want to maintain SQUARE pixels relative to the physical screen.
        // Assuming the physical pixels of the window are square.
        // To map 800x480 "logical units" to square pixels, we must scale uniformily.
        // 800 units wide, 480 units high.
        // If we map X: 0..800 -> 0..1056 pixels (Scale = 1.32)
        // Then Y must be scaled by 1.32: 480 * 1.32 = 633.6 pixels.
        // Window height is 800. So we have room. Centering vertically.

        float scale_factor = (float)(DEFAULT_TERM_WIDTH * DEFAULT_CHAR_WIDTH) / 800.0f; // 1.32
        float target_height = 480.0f * scale_factor; // 633.6
        float y_margin = ((float)(DEFAULT_TERM_HEIGHT * DEFAULT_CHAR_HEIGHT) - target_height) / 2.0f;

        // Normalize to UV space (0..1)
        float u0 = ((float)x0 * scale_factor) / (float)(DEFAULT_TERM_WIDTH * DEFAULT_CHAR_WIDTH); // Simplifies to x0/800.0f if scale_factor is width-based
        // Actually: u = (x * 1.32) / 1056 = x * (1056/800) / 1056 = x / 800.
        // So X mapping is strictly 0..1 for 0..800 input.

        float screen_h = (float)(DEFAULT_TERM_HEIGHT * DEFAULT_CHAR_HEIGHT);
        float v0_px = y_margin + ((float)y0 * scale_factor);
        float v1_px = y_margin + ((float)y1 * scale_factor);

        // Y is inverted in ReGIS (0=Top) vs OpenGL UV (0=Bottom usually? No, SITUATION/Vulkan UV 0=Top).
        // Let's assume UV 0,0 is Top-Left.
        // ReGIS 0,0 is Top-Left? Standard ReGIS P[0,0] is top left.
        // Our shader uses: vec2 p0 = line.start * pc.screen_size;
        // imageStore(ivec2(p0))
        // imageStore coordinates: 0,0 is usually Top-Left in Vulkan/Compute if we map that way?
        // Wait, OpenGL Compute imageStore 0,0 is Bottom-Left.
        // So we need to invert Y.

        float v0 = 1.0f - (v0_px / screen_h);
        float v1 = 1.0f - (v1_px / screen_h);

        line->x0 = (float)x0 / 800.0f;
        line->y0 = v0;
        line->x1 = (float)x1 / 800.0f;
        line->y1 = v1;

        line->color = terminal.regis.color;
        line->intensity = 1.0f;
        line->mode = terminal.regis.write_mode;
        terminal.vector_count++;
    }
}

static int ReGIS_CompareInt(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

static void ReGIS_FillPolygon(void) {
    if (terminal.regis.point_count < 3) {
        terminal.regis.point_count = 0;
        return;
    }

    // Scanline Fill Algorithm
    int min_y = 480, max_y = 0;
    for(int i=0; i<terminal.regis.point_count; i++) {
        if (terminal.regis.point_buffer[i].y < min_y) min_y = terminal.regis.point_buffer[i].y;
        if (terminal.regis.point_buffer[i].y > max_y) max_y = terminal.regis.point_buffer[i].y;
    }
    if (min_y < 0) min_y = 0;
    if (max_y > 479) max_y = 479;

    int nodes[64];
    for (int y = min_y; y <= max_y; y++) {
        int node_count = 0;
        int j = terminal.regis.point_count - 1;
        for (int i = 0; i < terminal.regis.point_count; i++) {
            int y1 = terminal.regis.point_buffer[i].y;
            int y2 = terminal.regis.point_buffer[j].y;
            int x1 = terminal.regis.point_buffer[i].x;
            int x2 = terminal.regis.point_buffer[j].x;

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
                int x_start = nodes[i] < 0 ? 0 : nodes[i];
                int x_end = nodes[i+1] > 799 ? 799 : nodes[i+1];
                if (x_start > 799) break;
                if (x_end < 0) continue;
                if (x_start < x_end) {
                     // Draw horizontal line span
                     ReGIS_DrawLine(x_start, y, x_end, y);
                }
            }
        }
    }
    terminal.regis.point_count = 0;
}

// Cubic B-Spline interpolation
static void ReGIS_EvalBSpline(int p0x, int p0y, int p1x, int p1y, int p2x, int p2y, int p3x, int p3y, float t, int* out_x, int* out_y) {
    float t2 = t * t;
    float t3 = t2 * t;
    float b0 = (-t3 + 3*t2 - 3*t + 1) / 6.0f;
    float b1 = (3*t3 - 6*t2 + 4) / 6.0f;
    float b2 = (-3*t3 + 3*t2 + 3*t + 1) / 6.0f;
    float b3 = t3 / 6.0f;

    *out_x = (int)(b0*p0x + b1*p1x + b2*p2x + b3*p3x);
    *out_y = (int)(b0*p0y + b1*p1y + b2*p2y + b3*p3y);
}

static void ExecuteReGISCommand(void) {
    if (terminal.regis.command == 0) return;
    if (!terminal.regis.data_pending && terminal.regis.command != 'S' && terminal.regis.command != 'W' && terminal.regis.command != 'F' && terminal.regis.command != 'R') return;

    int max_idx = terminal.regis.param_count;

    // --- P: Position ---
    if (terminal.regis.command == 'P') {
        for (int i = 0; i <= max_idx; i += 2) {
            int val_x = terminal.regis.params[i];
            bool rel_x = terminal.regis.params_relative[i];
            int val_y = (i + 1 <= max_idx) ? terminal.regis.params[i+1] : terminal.regis.y;

            if (i+1 > max_idx) val_y = terminal.regis.y; // Fallback if Y completely missing in pair

            bool rel_y = (i + 1 <= max_idx) ? terminal.regis.params_relative[i+1] : false;

            int target_x = rel_x ? (terminal.regis.x + val_x) : val_x;
            int target_y = rel_y ? (terminal.regis.y + val_y) : val_y;

            // Clamp
            if (target_x < 0) target_x = 0; if (target_x > 799) target_x = 799;
            if (target_y < 0) target_y = 0; if (target_y > 479) target_y = 479;

            terminal.regis.x = target_x;
            terminal.regis.y = target_y;

            terminal.regis.point_count = 0;
        }
    }
    // --- V: Vector (Line) ---
    else if (terminal.regis.command == 'V') {
        for (int i = 0; i <= max_idx; i += 2) {
            int val_x = terminal.regis.params[i];
            bool rel_x = terminal.regis.params_relative[i];
            int val_y = (i + 1 <= max_idx) ? terminal.regis.params[i+1] : terminal.regis.y;
            if (i+1 > max_idx && !rel_x) val_y = terminal.regis.y;

            bool rel_y = (i + 1 <= max_idx) ? terminal.regis.params_relative[i+1] : false;

            int target_x = rel_x ? (terminal.regis.x + val_x) : val_x;
            int target_y = rel_y ? (terminal.regis.y + val_y) : val_y;

            if (target_x < 0) target_x = 0; if (target_x > 799) target_x = 799;
            if (target_y < 0) target_y = 0; if (target_y > 479) target_y = 479;

            ReGIS_DrawLine(terminal.regis.x, terminal.regis.y, target_x, target_y);

            terminal.regis.x = target_x;
            terminal.regis.y = target_y;
        }
        terminal.regis.point_count = 0;
    }
    // --- F: Polygon Fill ---
    else if (terminal.regis.command == 'F') {
        // Collect points but don't draw immediately
        for (int i = 0; i <= max_idx; i += 2) {
            int val_x = terminal.regis.params[i];
            bool rel_x = terminal.regis.params_relative[i];
            int val_y = (i + 1 <= max_idx) ? terminal.regis.params[i+1] : terminal.regis.y;
            bool rel_y = (i + 1 <= max_idx) ? terminal.regis.params_relative[i+1] : false;

            int px = rel_x ? (terminal.regis.x + val_x) : val_x;
            int py = rel_y ? (terminal.regis.y + val_y) : val_y;

            if (px < 0) px = 0; if (px > 799) px = 799;
            if (py < 0) py = 0; if (py > 479) py = 479;

            if (terminal.regis.point_count < 64) {
                 if (terminal.regis.point_count == 0) {
                     // First point is usually current cursor if implied?
                     // Standard F command might imply current position as start.
                     // We'll add current pos if buffer empty?
                     // ReGIS usually: F(V(P[x,y]...))
                     // Our F implementation just collects points passed to it.
                     // If point_count is 0, we should probably add current cursor?
                     terminal.regis.point_buffer[0].x = terminal.regis.x;
                     terminal.regis.point_buffer[0].y = terminal.regis.y;
                     terminal.regis.point_count++;
                 }
                 terminal.regis.point_buffer[terminal.regis.point_count].x = px;
                 terminal.regis.point_buffer[terminal.regis.point_count].y = py;
                 terminal.regis.point_count++;
            }
            terminal.regis.x = px;
            terminal.regis.y = py;
        }
    }
    // --- C: Circle / Curve ---
    else if (terminal.regis.command == 'C') {
        if (terminal.regis.option_command == 'B') {
            // --- B-Spline ---
            for (int i = 0; i <= max_idx; i += 2) {
                int val_x = terminal.regis.params[i];
                bool rel_x = terminal.regis.params_relative[i];
                int val_y = (i + 1 <= max_idx) ? terminal.regis.params[i+1] : terminal.regis.y;
                bool rel_y = (i + 1 <= max_idx) ? terminal.regis.params_relative[i+1] : false;

                int px = rel_x ? (terminal.regis.x + val_x) : val_x;
                int py = rel_y ? (terminal.regis.y + val_y) : val_y;

                if (terminal.regis.point_count < 64) {
                    if (terminal.regis.point_count == 0) {
                        terminal.regis.point_buffer[0].x = terminal.regis.x;
                        terminal.regis.point_buffer[0].y = terminal.regis.y;
                        terminal.regis.point_count++;
                    }
                    terminal.regis.point_buffer[terminal.regis.point_count].x = px;
                    terminal.regis.point_buffer[terminal.regis.point_count].y = py;
                    terminal.regis.point_count++;
                }
                terminal.regis.x = px;
                terminal.regis.y = py;
            }

            if (terminal.regis.point_count >= 4) {
                for (int i = 0; i <= terminal.regis.point_count - 4; i++) {
                    int p0x = terminal.regis.point_buffer[i].x;   int p0y = terminal.regis.point_buffer[i].y;
                    int p1x = terminal.regis.point_buffer[i+1].x; int p1y = terminal.regis.point_buffer[i+1].y;
                    int p2x = terminal.regis.point_buffer[i+2].x; int p2y = terminal.regis.point_buffer[i+2].y;
                    int p3x = terminal.regis.point_buffer[i+3].x; int p3y = terminal.regis.point_buffer[i+3].y;

                    int seg_steps = 10;
                    int last_x = -1, last_y = -1;

                    for (int s=0; s<=seg_steps; s++) {
                        float t = (float)s / (float)seg_steps;
                        int tx, ty;
                        ReGIS_EvalBSpline(p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y, t, &tx, &ty);
                        if (last_x != -1) {
                            ReGIS_DrawLine(last_x, last_y, tx, ty);
                        }
                        last_x = tx;
                        last_y = ty;
                    }
                }
                int keep = 3;
                if (terminal.regis.point_count > keep) {
                    for(int k=0; k<keep; k++) {
                        terminal.regis.point_buffer[k] = terminal.regis.point_buffer[terminal.regis.point_count - keep + k];
                    }
                    terminal.regis.point_count = keep;
                }
            }
        }
        else if (terminal.regis.option_command == 'A') {
            // --- Arc ---
            if (max_idx >= 0) {
                int cx_val = terminal.regis.params[0];
                bool cx_rel = terminal.regis.params_relative[0];
                int cy_val = (1 <= max_idx) ? terminal.regis.params[1] : terminal.regis.y;
                bool cy_rel = (1 <= max_idx) ? terminal.regis.params_relative[1] : false;

                int cx = cx_rel ? (terminal.regis.x + cx_val) : cx_val;
                int cy = cy_rel ? (terminal.regis.y + cy_val) : cy_val;

                int sx = terminal.regis.x;
                int sy = terminal.regis.y;

                float dx = (float)(sx - cx);
                float dy = (float)(sy - cy);
                float radius = sqrtf(dx*dx + dy*dy);
                float start_angle = atan2f(dy, dx);

                float degrees = 0;
                if (max_idx >= 2) {
                    degrees = (float)terminal.regis.params[2];
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
                    ReGIS_DrawLine(last_x, last_y, nx, ny);
                    last_x = nx;
                    last_y = ny;
                }

                terminal.regis.x = last_x;
                terminal.regis.y = last_y;
            }
        }
        else {
            // --- Standard Circle ---
            for (int i = 0; i <= max_idx; i += 2) {
                 int val1 = terminal.regis.params[i];
                 bool rel1 = terminal.regis.params_relative[i];

                 int radius = 0;
                 if (i + 1 > max_idx) {
                     radius = val1;
                 } else {
                     int val2 = terminal.regis.params[i+1];
                     bool rel2 = terminal.regis.params_relative[i+1];
                     int px = rel1 ? (terminal.regis.x + val1) : val1;
                     int py = rel2 ? (terminal.regis.y + val2) : val2;
                     float dx = (float)(px - terminal.regis.x);
                     float dy = (float)(py - terminal.regis.y);
                     radius = (int)sqrtf(dx*dx + dy*dy);
                 }

                 int cx = terminal.regis.x;
                 int cy = terminal.regis.y;
                 int segments = 32;
                 float angle_step = 6.283185f / segments;
                 float ncx = (float)cx / 800.0f;
                 float ncy = (float)cy / 480.0f;
                 float nr_x = (float)radius / 800.0f;
                 float nr_y = (float)radius / 480.0f;

                 for (int j = 0; j < segments; j++) {
                    if (terminal.vector_count >= terminal.vector_capacity) break;
                    float a1 = j * angle_step;
                    float a2 = (j + 1) * angle_step;
                    float x1 = ncx + cosf(a1) * nr_x;
                    float y1 = ncy + sinf(a1) * nr_y;
                    float x2 = ncx + cosf(a2) * nr_x;
                    float y2 = ncy + sinf(a2) * nr_y;

                    GPUVectorLine* line = &terminal.vector_staging_buffer[terminal.vector_count];
                    line->x0 = x1;
                    line->y0 = 1.0f - y1;
                    line->x1 = x2;
                    line->y1 = 1.0f - y2;
                    line->color = terminal.regis.color;
                    line->intensity = 1.0f;
                    line->mode = terminal.regis.write_mode;
                    terminal.vector_count++;
                 }
            }
        }
    }
    // --- S: Screen Control ---
    else if (terminal.regis.command == 'S') {
        if (terminal.regis.option_command == 'E') {
             terminal.vector_count = 0;
             terminal.vector_clear_request = true;
        }
    }
    // --- W: Write Control ---
    else if (terminal.regis.command == 'W') {
        // Handle explicit Color Index selection W(I...)
        if (terminal.regis.option_command == 'I') {
             int color_idx = terminal.regis.params[0];
             if (color_idx >= 0 && color_idx < 16) {
                 Color c = ansi_colors[color_idx];
                 terminal.regis.color = (uint32_t)c.r | ((uint32_t)c.g << 8) | ((uint32_t)c.b << 16) | 0xFF000000;
             }
        }
        // Handle Writing Modes
        else if (terminal.regis.option_command == 'R') {
             terminal.regis.write_mode = 1; // Replace
        } else if (terminal.regis.option_command == 'E') {
             terminal.regis.write_mode = 2; // Erase
        } else if (terminal.regis.option_command == 'V') {
             terminal.regis.write_mode = 0; // Overlay (Additive)
        } else if (terminal.regis.option_command == 'C') {
             // W(C) is ambiguous: could be Complement or Color
             // If we have parameters (e.g. W(C1)), treat as Color (Legacy behavior).
             // If no parameters (e.g. W(C)), treat as Complement (XOR).

             if (terminal.regis.param_count > 0) {
                 // Likely Color Index W(C1)
                 int color_idx = terminal.regis.params[0];
                 if (color_idx >= 0 && color_idx < 16) {
                     Color c = ansi_colors[color_idx];
                     terminal.regis.color = (uint32_t)c.r | ((uint32_t)c.g << 8) | ((uint32_t)c.b << 16) | 0xFF000000;
                 }
             } else {
                 terminal.regis.write_mode = 3; // Complement (XOR)
             }
        }
    }
    // --- T: Text Attributes ---
    else if (terminal.regis.command == 'T') {
        if (terminal.regis.option_command == 'S') {
             // Size
             terminal.regis.text_size = (float)terminal.regis.params[0];
             if (terminal.regis.text_size <= 0) terminal.regis.text_size = 1;
        }
        if (terminal.regis.option_command == 'D') {
             // Direction (degrees)
             terminal.regis.text_angle = (float)terminal.regis.params[0] * 3.14159f / 180.0f;
        }
    }
    // --- L: Load Alphabet ---
    else if (terminal.regis.command == 'L') {
        // L command logic is primarily handled in ProcessReGISChar during string/hex parsing.
        // This block handles parameterized options like S (Size) if provided.
        if (terminal.regis.option_command == 'S') {
             // Character Cell Size (e.g. L(S1) or L(S[8,16]))
             int w = 8;
             int h = 16;
             if (terminal.regis.param_count >= 0) { // param_count is max index
                 // Check if it's an index or explicit size
                 if (terminal.regis.params[0] == 1) { w=8; h=16; }
                 else if (terminal.regis.params[0] == 0) { w=8; h=16; } // Default
                 else {
                     // Assume width
                     w = terminal.regis.params[0];
                     if (terminal.regis.param_count >= 1) h = terminal.regis.params[1];
                 }
             }
             ACTIVE_SESSION.soft_font.char_width = w;
             ACTIVE_SESSION.soft_font.char_height = h;
        } else if (terminal.regis.option_command == 'A') {
             // Alphabet selection L(A1)
             if (terminal.regis.param_count >= 0) {
                 int alpha = terminal.regis.params[0];
                 // We only really support loading into "soft font" slot (conceptually A1)
                 // A0 is typically the hardware ROM font.
                 // If L(A1) is used, we know subsequent string data targets the soft font.
                 // ProcessReGISChar logic implicitly targets soft font.
                 // Maybe we should warn if alpha != 1?
             }
        }
    }
    // --- R: Report ---
    else if (terminal.regis.command == 'R') {
         if (terminal.regis.option_command == 'P') {
             char buf[64];
             snprintf(buf, sizeof(buf), "\x1BP%d,%d\x1B\\", terminal.regis.x, terminal.regis.y);
             QueueResponse(buf);
         }
    }

    terminal.regis.data_pending = false;
}

static void ProcessReGISChar(unsigned char ch) {
    if (ch == 0x1B) { // ESC \ (ST)
        if (terminal.regis.command == 'F') ReGIS_FillPolygon(); // Flush pending fill
        if (terminal.regis.state == 1 || terminal.regis.state == 3) {
            ExecuteReGISCommand();
        }
        ACTIVE_SESSION.parse_state = VT_PARSE_ESCAPE;
        return;
    }

    if (terminal.regis.recording_macro) {
        if (ch == ';' && terminal.regis.macro_len > 0 && terminal.regis.macro_buffer[terminal.regis.macro_len-1] == '@') {
             // End of macro definition (@;)
             terminal.regis.macro_buffer[terminal.regis.macro_len-1] = '\0'; // Remove @
             terminal.regis.recording_macro = false;
             // Store macro in slot
             if (terminal.regis.macro_index >= 0 && terminal.regis.macro_index < 26) {
                 if (terminal.regis.macros[terminal.regis.macro_index]) free(terminal.regis.macros[terminal.regis.macro_index]);
                 terminal.regis.macros[terminal.regis.macro_index] = strdup(terminal.regis.macro_buffer);
             }
             if (terminal.regis.macro_buffer) { free(terminal.regis.macro_buffer); terminal.regis.macro_buffer = NULL; }
             return;
        }
        // Append
        if (!terminal.regis.macro_buffer) {
             terminal.regis.macro_cap = 1024;
             terminal.regis.macro_buffer = malloc(terminal.regis.macro_cap);
             terminal.regis.macro_len = 0;
        }
        if (terminal.regis.macro_len >= terminal.regis.macro_cap - 1) {
             terminal.regis.macro_cap *= 2;
             terminal.regis.macro_buffer = realloc(terminal.regis.macro_buffer, terminal.regis.macro_cap);
        }
        terminal.regis.macro_buffer[terminal.regis.macro_len++] = ch;
        terminal.regis.macro_buffer[terminal.regis.macro_len] = '\0';
        return;
    }

    if (terminal.regis.state == 3) { // Parsing Text String
        if (ch == terminal.regis.string_terminator) {
            terminal.regis.text_buffer[terminal.regis.text_pos] = '\0';

            if (terminal.regis.command == 'L') {
                // Load Alphabet Logic
                if (terminal.regis.option_command == 'A') {
                    // Set Alphabet Name
                    strncpy(terminal.regis.load.name, terminal.regis.text_buffer, 15);
                    terminal.regis.load.name[15] = '\0';
                    terminal.regis.option_command = 0; // Reset
                } else {
                    // Define Character
                    if (terminal.regis.text_pos > 0) {
                        terminal.regis.load.current_char = (unsigned char)terminal.regis.text_buffer[0];
                        terminal.regis.load.pattern_byte_idx = 0;
                        terminal.regis.load.hex_nibble = -1;
                        // Clear existing pattern for this char
                        memset(ACTIVE_SESSION.soft_font.font_data[terminal.regis.load.current_char], 0, 32);
                        ACTIVE_SESSION.soft_font.loaded[terminal.regis.load.current_char] = true;
                        ACTIVE_SESSION.soft_font.active = true;
                    }
                }
            } else {
                // Text Drawing with attributes
                float scale = (terminal.regis.text_size > 0) ? terminal.regis.text_size : 1.0f;
                // Base scale 1 = 8x16? ReGIS default size 1 is roughly 9x16 grid?
                // Existing code used scale 2.0f. Let's base it on that.
                scale *= 2.0f;

                float cos_a = cosf(terminal.regis.text_angle);
                float sin_a = sinf(terminal.regis.text_angle);

                int start_x = terminal.regis.x;
                int start_y = terminal.regis.y;

                const unsigned char* font_base = vga_perfect_8x8_font;
                bool use_soft_font = ACTIVE_SESSION.soft_font.active;

                for(int i=0; terminal.regis.text_buffer[i] != '\0'; i++) {
                    unsigned char c = (unsigned char)terminal.regis.text_buffer[i];

                    // Use dynamic height if soft font is active
                    int max_rows = use_soft_font ? ACTIVE_SESSION.soft_font.char_height : 16;
                    if (max_rows > 32) max_rows = 32;

                    for(int r=0; r<max_rows; r++) { // Iterate up to max_rows
                        unsigned char row = 0;
                        int height_limit = 8; // Default to 8 for VGA font

                        if (use_soft_font && ACTIVE_SESSION.soft_font.loaded[c]) {
                            row = ACTIVE_SESSION.soft_font.font_data[c][r];
                            height_limit = ACTIVE_SESSION.soft_font.char_height;
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

                                if (terminal.vector_count < terminal.vector_capacity) {
                                    GPUVectorLine* line = &terminal.vector_staging_buffer[terminal.vector_count];
                                    line->x0 = fx0 / 800.0f;
                                    line->y0 = 1.0f - (fy0 / 480.0f);
                                    line->x1 = fx1 / 800.0f;
                                    line->y1 = 1.0f - (fy1 / 480.0f);
                                    line->color = terminal.regis.color;
                                    line->intensity = 1.0f;
                                    line->mode = terminal.regis.write_mode;
                                    terminal.vector_count++;
                                 }
                                c_bit += len - 1;
                            }
                        }
                    }
                }
                // Update cursor position to end of string
                float total_width = terminal.regis.text_pos * 9 * scale;
                terminal.regis.x = start_x + (int)(total_width * cos_a);
                terminal.regis.y = start_y + (int)(total_width * sin_a);
            }

            terminal.regis.state = 1;
            terminal.regis.text_pos = 0;
        } else {
            if (terminal.regis.text_pos < 255) {
                terminal.regis.text_buffer[terminal.regis.text_pos++] = ch;
            }
        }
        return;
    }

    if (ch <= 0x20 || ch == 0x7F) return;

    if (terminal.regis.state == 0) { // Expecting Command
        if (ch == '@') {
            // Macro
            terminal.regis.command = '@';
            terminal.regis.state = 1;
            return;
        }
        if (isalpha(ch)) {
            terminal.regis.command = toupper(ch);
            terminal.regis.state = 1;
            terminal.regis.param_count = 0;
            terminal.regis.has_bracket = false;
            terminal.regis.has_paren = false;
            terminal.regis.point_count = 0; // Reset curve points on new command
            for(int i=0; i<16; i++) {
                terminal.regis.params[i] = 0;
                terminal.regis.params_relative[i] = false;
            }
        }
    } else if (terminal.regis.state == 1) { // Expecting Values/Options
        if (terminal.regis.command == '@') {
             if (ch == ':') {
                 // Definition
                 terminal.regis.option_command = ':'; // Flag next char as macro name
                 return;
             }
             if (terminal.regis.option_command == ':') {
                 // Macro Name
                 if (isalpha(ch)) {
                     terminal.regis.macro_index = toupper(ch) - 'A';
                     terminal.regis.recording_macro = true;
                     terminal.regis.macro_len = 0;
                     terminal.regis.option_command = 0;
                 }
                 return;
             }
             // Execute Macro
             if (isalpha(ch)) {
                 int idx = toupper(ch) - 'A';
                 if (idx >= 0 && idx < 26 && terminal.regis.macros[idx]) {
                     if (terminal.regis.recursion_depth < 16) {
                         terminal.regis.recursion_depth++;
                         // Push macro content to parser
                         const char* m = terminal.regis.macros[idx];
                         // Reset state to 0 for macro context?
                         // Macros usually contain full commands.
                         int saved_state = terminal.regis.state;
                         terminal.regis.state = 0;
                         for (int k=0; m[k]; k++) ProcessReGISChar(m[k]);
                         terminal.regis.state = saved_state;
                         terminal.regis.recursion_depth--;
                     } else {
                         if (ACTIVE_SESSION.options.debug_sequences) {
                             LogUnsupportedSequence("ReGIS Macro recursion depth exceeded");
                         }
                     }
                 }
                 terminal.regis.command = 0;
                 terminal.regis.state = 0;
             }
             return;
        }

        if (ch == '\'' || ch == '"') {
             if (terminal.regis.command == 'T' || terminal.regis.command == 'L') {
                 terminal.regis.state = 3;
                 terminal.regis.string_terminator = ch;
                 terminal.regis.text_pos = 0;
                 return;
             }
        }
        if (ch == '[') {
            terminal.regis.has_bracket = true;
            terminal.regis.has_comma = false;
            terminal.regis.parsing_val = false;
        } else if (ch == ']') {
            if (terminal.regis.parsing_val) {
                 terminal.regis.params[terminal.regis.param_count] = terminal.regis.current_sign * terminal.regis.current_val;
                 terminal.regis.params_relative[terminal.regis.param_count] = terminal.regis.val_is_relative;
            }
            terminal.regis.parsing_val = false;
            terminal.regis.has_bracket = false;
            ExecuteReGISCommand();
            terminal.regis.param_count = 0;
            for(int i=0; i<16; i++) {
                terminal.regis.params[i] = 0;
                terminal.regis.params_relative[i] = false;
            }
        } else if (ch == '(') {
            terminal.regis.has_paren = true;
            terminal.regis.parsing_val = false;
        } else if (ch == ')') {
            if (terminal.regis.parsing_val) {
                 terminal.regis.params[terminal.regis.param_count] = terminal.regis.current_sign * terminal.regis.current_val;
                 terminal.regis.params_relative[terminal.regis.param_count] = terminal.regis.val_is_relative;
            }
            terminal.regis.has_paren = false;
            terminal.regis.parsing_val = false;
            ExecuteReGISCommand();
            // Don't reset point count here, as options might modify curve mode
            // But we reset param count for next block
            terminal.regis.param_count = 0;
            for(int i=0; i<16; i++) {
                terminal.regis.params[i] = 0;
                terminal.regis.params_relative[i] = false;
            }
        } else if (terminal.regis.command == 'L' && isxdigit(ch)) {
            // Hex parsing for Load Alphabet
            // Assuming ReGIS "hex string" format (pairs of hex digits)
            int val = 0;
            if (ch >= '0' && ch <= '9') val = ch - '0';
            else if (ch >= 'A' && ch <= 'F') val = ch - 'A' + 10;
            else if (ch >= 'a' && ch <= 'f') val = ch - 'a' + 10;

            if (terminal.regis.load.hex_nibble == -1) {
                terminal.regis.load.hex_nibble = val;
            } else {
                int byte = (terminal.regis.load.hex_nibble << 4) | val;
                terminal.regis.load.hex_nibble = -1;

                if (terminal.regis.load.pattern_byte_idx < 32) {
                    ACTIVE_SESSION.soft_font.font_data[terminal.regis.load.current_char][terminal.regis.load.pattern_byte_idx++] = byte;
                }
            }
            // Defer texture update to DrawTerminal
            ACTIVE_SESSION.soft_font.dirty = true;

        } else if (isdigit(ch) || ch == '-' || ch == '+') {
            if (!terminal.regis.parsing_val) {
                terminal.regis.parsing_val = true;
                terminal.regis.current_val = 0;
                terminal.regis.current_sign = 1;
                terminal.regis.val_is_relative = false;
            }
            if (ch == '-') {
                terminal.regis.current_sign = -1;
                terminal.regis.val_is_relative = true;
            } else if (ch == '+') {
                terminal.regis.current_sign = 1;
                terminal.regis.val_is_relative = true;
            } else if (isdigit(ch)) {
               terminal.regis.current_val = terminal.regis.current_val * 10 + (ch - '0');
            }
            terminal.regis.params[terminal.regis.param_count] = terminal.regis.current_sign * terminal.regis.current_val;
            terminal.regis.params_relative[terminal.regis.param_count] = terminal.regis.val_is_relative;
            terminal.regis.data_pending = true;
        } else if (ch == ',') {
            if (terminal.regis.parsing_val) {
                terminal.regis.params[terminal.regis.param_count] = terminal.regis.current_sign * terminal.regis.current_val;
                terminal.regis.params_relative[terminal.regis.param_count] = terminal.regis.val_is_relative;
                terminal.regis.parsing_val = false;
            }
            if (terminal.regis.param_count < 15) {
                terminal.regis.param_count++;
                terminal.regis.params[terminal.regis.param_count] = 0;
                terminal.regis.params_relative[terminal.regis.param_count] = false;
            }
            terminal.regis.has_comma = true;
        } else if (isalpha(ch)) {
            if (terminal.regis.has_paren) {
                 terminal.regis.option_command = toupper(ch);
                 terminal.regis.param_count = 0;
                 terminal.regis.parsing_val = false;
            } else {
                if (terminal.regis.command == 'F') ReGIS_FillPolygon(); // Flush fill on new command
                ExecuteReGISCommand();
                terminal.regis.command = toupper(ch);
                terminal.regis.state = 1;
                terminal.regis.param_count = 0;
                terminal.regis.parsing_val = false;
                terminal.regis.point_count = 0; // Reset on new command
                for(int i=0; i<16; i++) {
                    terminal.regis.params[i] = 0;
                    terminal.regis.params_relative[i] = false;
                }
            }
        }
    }
}

static void ProcessTektronixChar(unsigned char ch) {
    // 1. Escape Sequence Escape
    if (ch == 0x1B) {
        // Switch to VT_PARSE_ESCAPE. Standard parser will handle the rest.
        // If it's ESC ETX (exit), the next char will be handled in ProcessEscapeChar
        // which resets state to NORMAL if sequence is unknown/invalid, effectively exiting Tek mode.
        // Or if it's ESC [ ? 38 l (exit), it will be handled by ProcessCSIChar -> ExecuteRM.
        ACTIVE_SESSION.parse_state = VT_PARSE_ESCAPE;
        return;
    }

    // 2. Control Codes
    if (ch == 0x1D) { // GS - Graph Mode
        terminal.tektronix.state = 1; // Graph
        terminal.tektronix.pen_down = false; // First coord is Dark (Move)
        return;
    }
    if (ch == 0x1F) { // US - Alpha Mode (Text)
        terminal.tektronix.state = 0; // Alpha
        return;
    }
    if (ch == 0x0C) { // FF - Clear Screen
        terminal.vector_count = 0;
        terminal.tektronix.pen_down = false; // Reset pen? Usually yes.
        return;
    }
    if (ch < 0x20) {
        // Other controls (CR, LF, BEL) might be relevant in Alpha mode.
        if (terminal.tektronix.state == 0) { // Alpha
             ProcessControlChar(ch);
        }
        return;
    }

    // 3. Alpha Mode Handling
    if (terminal.tektronix.state == 0) {
        // Just standard text processing? Or specialized Tek font?
        // Let's fallback to ProcessNormalChar for Alpha mode bytes to show something.
        ProcessNormalChar(ch);
        return;
    }

    // 4. Graph Mode Coordinate Parsing
    // Categories:
    // HiY: 0x20 - 0x3F (001xxxxx)
    // LoY: 0x60 - 0x7F (011xxxxx)
    // HiX: 0x20 - 0x3F (001xxxxx) - Context dependent
    // LoX: 0x40 - 0x5F (010xxxxx)

    int val = ch & 0x1F; // 5 bits

    if (ch >= 0x20 && ch <= 0x3F) {
        // HiY or HiX
        if (terminal.tektronix.sub_state == 1) { // Previous was LoY?
            // Interpret as HiX
            terminal.tektronix.holding_x = (terminal.tektronix.holding_x & 0x1F) | (val << 5);
            terminal.tektronix.sub_state = 2; // Seen HiX (clears LoY flag)
        } else {
            // Interpret as HiY
            terminal.tektronix.holding_y = (terminal.tektronix.holding_y & 0x1F) | (val << 5);
            terminal.tektronix.sub_state = 0; // Seen HiY
        }
    } else if (ch >= 0x60 && ch <= 0x7F) {
        // LoY
        terminal.tektronix.holding_y = (terminal.tektronix.holding_y & ~0x1F) | val;
        terminal.tektronix.sub_state = 1; // Flag: Next 0x20-3F is HiX
    } else if (ch >= 0x40 && ch <= 0x5F) {
        // LoX - Trigger
        terminal.tektronix.holding_x = (terminal.tektronix.holding_x & ~0x1F) | val;

        // DRAW
        if (terminal.tektronix.pen_down) {
            if (terminal.vector_count < terminal.vector_capacity) {
                GPUVectorLine* line = &terminal.vector_staging_buffer[terminal.vector_count];

                // Tektronix 4010/4014 is 4096x4096 addressable (12-bit)
                // However, 10-bit mode is 1024x1024.
                // 12-bit addressing needs Extra Byte (not implemented here, assuming 10-bit logic for now).
                // "10-bit coordinate (0-1023) or 12-bit (0-4095)".
                // Our parsing logic (Hi/Lo) gives 5+5=10 bits.
                // Max val = 1023.

                float norm_x1 = (float)terminal.tektronix.x / 1024.0f;
                float norm_y1 = (float)terminal.tektronix.y / 1024.0f;
                float norm_x2 = (float)terminal.tektronix.holding_x / 1024.0f;
                float norm_y2 = (float)terminal.tektronix.holding_y / 1024.0f;

                // Flip Y (Tektronix 0,0 is bottom-left)
                norm_y1 = 1.0f - norm_y1;
                norm_y2 = 1.0f - norm_y2;

                line->x0 = norm_x1;
                line->y0 = norm_y1;
                line->x1 = norm_x2;
                line->y1 = norm_y2;
                line->color = 0xFF00FF00; // Bright Green (ABGR: A=FF, B=00, G=FF, R=00)
                line->intensity = 1.0f;

                terminal.vector_count++;
            }
        }

        // Update Position
        terminal.tektronix.x = terminal.tektronix.holding_x;
        terminal.tektronix.y = terminal.tektronix.holding_y;
        terminal.tektronix.pen_down = true; // Subsequent coords will draw
        terminal.tektronix.sub_state = 0; // Reset sub-state
    }
}

void ProcessVT52Char(unsigned char ch) {
    static bool expect_param = false;
    static char vt52_command = 0;

    if (!expect_param) {
        switch (ch) {
            case 'A': // Cursor up
                if (ACTIVE_SESSION.cursor.y > 0) ACTIVE_SESSION.cursor.y--;
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case 'B': // Cursor down
                if (ACTIVE_SESSION.cursor.y < DEFAULT_TERM_HEIGHT - 1) ACTIVE_SESSION.cursor.y++;
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case 'C': // Cursor right
                if (ACTIVE_SESSION.cursor.x < DEFAULT_TERM_WIDTH - 1) ACTIVE_SESSION.cursor.x++;
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case 'D': // Cursor left
                if (ACTIVE_SESSION.cursor.x > 0) ACTIVE_SESSION.cursor.x--;
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case 'H': // Home cursor
                ACTIVE_SESSION.cursor.x = 0;
                ACTIVE_SESSION.cursor.y = 0;
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case 'I': // Reverse line feed
                ACTIVE_SESSION.cursor.y--;
                if (ACTIVE_SESSION.cursor.y < 0) {
                    ACTIVE_SESSION.cursor.y = 0;
                    ScrollDownRegion(0, DEFAULT_TERM_HEIGHT - 1, 1);
                }
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case 'J': // Clear to end of screen
                // Clear from cursor to end of line
                for (int x = ACTIVE_SESSION.cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                    ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x));
                }
                // Clear remaining lines
                for (int y = ACTIVE_SESSION.cursor.y + 1; y < DEFAULT_TERM_HEIGHT; y++) {
                    for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                        ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, y, x));
                    }
                }
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case 'K': // Clear to end of line
                for (int x = ACTIVE_SESSION.cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                    ClearCell(GetActiveScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.cursor.y, x));
                }
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case 'Y': // Direct cursor address
                vt52_command = 'Y';
                expect_param = true;
                ACTIVE_SESSION.escape_pos = 0;
                break;

            case 'Z': // Identify
                QueueResponse("\x1B/Z"); // VT52 identification
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case '=': // Enter alternate keypad mode
                ACTIVE_SESSION.vt_keyboard.keypad_mode = true;
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case '>': // Exit alternate keypad mode
                ACTIVE_SESSION.vt_keyboard.keypad_mode = false;
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case '<': // Enter ANSI mode
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                // Exit VT52 mode
                break;

            case 'F': // Enter graphics mode
                ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g1; // Use G1 (DEC special)
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            case 'G': // Exit graphics mode
                ACTIVE_SESSION.charset.gl = &ACTIVE_SESSION.charset.g0; // Use G0 (ASCII)
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                break;

            default:
                // Unknown VT52 command
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
                if (ACTIVE_SESSION.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown VT52 command: %c", ch);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
    } else {
        // Handle parameterized commands
        if (vt52_command == 'Y') {
            if (ACTIVE_SESSION.escape_pos == 0) {
                // First parameter: row
                ACTIVE_SESSION.escape_buffer[0] = ch;
                ACTIVE_SESSION.escape_pos = 1;
            } else {
                // Second parameter: column
                int row = ACTIVE_SESSION.escape_buffer[0] - 32; // VT52 uses offset of 32
                int col = ch - 32;

                // Clamp to valid range
                ACTIVE_SESSION.cursor.y = (row < 0) ? 0 : (row >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : row;
                ACTIVE_SESSION.cursor.x = (col < 0) ? 0 : (col >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : col;

                expect_param = false;
                ACTIVE_SESSION.parse_state = VT_PARSE_NORMAL;
            }
        }
    }
}
// =============================================================================
// SIXEL GRAPHICS SUPPORT (Basic Implementation)
// =============================================================================

void ProcessSixelChar(unsigned char ch) {
    // 1. Check for digits across all states that consume them
    if (isdigit(ch)) {
        if (ACTIVE_SESSION.sixel.parse_state == SIXEL_STATE_REPEAT) {
            ACTIVE_SESSION.sixel.repeat_count = ACTIVE_SESSION.sixel.repeat_count * 10 + (ch - '0');
            return;
        } else if (ACTIVE_SESSION.sixel.parse_state == SIXEL_STATE_COLOR ||
                   ACTIVE_SESSION.sixel.parse_state == SIXEL_STATE_RASTER) {
            int idx = ACTIVE_SESSION.sixel.param_buffer_idx;
            if (idx < 8) {
                ACTIVE_SESSION.sixel.param_buffer[idx] = ACTIVE_SESSION.sixel.param_buffer[idx] * 10 + (ch - '0');
            }
            return;
        }
    }

    // 2. Handle Separator ';'
    if (ch == ';') {
        if (ACTIVE_SESSION.sixel.parse_state == SIXEL_STATE_COLOR ||
            ACTIVE_SESSION.sixel.parse_state == SIXEL_STATE_RASTER) {
            if (ACTIVE_SESSION.sixel.param_buffer_idx < 7) {
                ACTIVE_SESSION.sixel.param_buffer_idx++;
                ACTIVE_SESSION.sixel.param_buffer[ACTIVE_SESSION.sixel.param_buffer_idx] = 0;
            }
            return;
        }
    }

    // 3. Command Processing
    // If we are in a parameter state but receive a command char, finalize the previous command implicitly.
    if (ACTIVE_SESSION.sixel.parse_state == SIXEL_STATE_COLOR) {
        if (ch != '#' && !isdigit(ch) && ch != ';') {
            // Finalize Color Command
            // # Pc ; Pu ; Px ; Py ; Pz (Define) OR # Pc (Select)
            if (ACTIVE_SESSION.sixel.param_buffer_idx >= 4) {
                // Color Definition
                // Param 0: Index (Pc)
                // Param 1: Type (Pu) - 1=HLS, 2=RGB
                // Param 2,3,4: Components
                int idx = ACTIVE_SESSION.sixel.param_buffer[0];
                int type = ACTIVE_SESSION.sixel.param_buffer[1];
                int c1 = ACTIVE_SESSION.sixel.param_buffer[2];
                int c2 = ACTIVE_SESSION.sixel.param_buffer[3];
                int c3 = ACTIVE_SESSION.sixel.param_buffer[4];

                if (idx >= 0 && idx < 256) {
                    if (type == 2) { // RGB (0-100)
                        ACTIVE_SESSION.sixel.palette[idx] = (RGB_Color){
                            (unsigned char)((c1 * 255) / 100),
                            (unsigned char)((c2 * 255) / 100),
                            (unsigned char)((c3 * 255) / 100),
                            255
                        };
                    } else if (type == 1) { // HLS (0-360, 0-100, 0-100)
                        // Simple HLS to RGB conversion could go here, for now mapping directly or ignoring
                        // HLS support requires a helper. Mapping Hue to RGB approx.
                        // Ideally strictly implement HLS.
                        // For this implementation, we will treat it as RGB for simplicity or leave as is.
                        // Standard says 2=RGB.
                    }
                    ACTIVE_SESSION.sixel.color_index = idx; // Auto-select? Usually yes.
                }
            } else {
                // Color Selection # Pc
                int idx = ACTIVE_SESSION.sixel.param_buffer[0];
                if (idx >= 0 && idx < 256) {
                    ACTIVE_SESSION.sixel.color_index = idx;
                }
            }
            ACTIVE_SESSION.sixel.parse_state = SIXEL_STATE_NORMAL;
        }
    } else if (ACTIVE_SESSION.sixel.parse_state == SIXEL_STATE_RASTER) {
        // Finalize Raster Attributes " Pan ; Pad ; Ph ; Pv
        // Just reset state for now, we don't resize based on this yet.
        ACTIVE_SESSION.sixel.parse_state = SIXEL_STATE_NORMAL;
    }

    switch (ch) {
        case '"': // Raster attributes
            ACTIVE_SESSION.sixel.parse_state = SIXEL_STATE_RASTER;
            ACTIVE_SESSION.sixel.param_buffer_idx = 0;
            memset(ACTIVE_SESSION.sixel.param_buffer, 0, sizeof(ACTIVE_SESSION.sixel.param_buffer));
            break;
        case '#': // Color introducer
            ACTIVE_SESSION.sixel.parse_state = SIXEL_STATE_COLOR;
            ACTIVE_SESSION.sixel.param_buffer_idx = 0;
            memset(ACTIVE_SESSION.sixel.param_buffer, 0, sizeof(ACTIVE_SESSION.sixel.param_buffer));
            break;
        case '!': // Repeat introducer
            ACTIVE_SESSION.sixel.parse_state = SIXEL_STATE_REPEAT;
            ACTIVE_SESSION.sixel.repeat_count = 0;
            break;
        case '$': // Carriage return
            ACTIVE_SESSION.sixel.pos_x = 0;
            ACTIVE_SESSION.sixel.parse_state = SIXEL_STATE_NORMAL;
            break;
        case '-': // New line
            ACTIVE_SESSION.sixel.pos_x = 0;
            ACTIVE_SESSION.sixel.pos_y += 6;
            ACTIVE_SESSION.sixel.parse_state = SIXEL_STATE_NORMAL;
            break;
        case '\x1B': // Escape character - signals the start of the String Terminator (ST)
             ACTIVE_SESSION.parse_state = PARSE_SIXEL_ST;
             return;
        default:
            if (ch >= '?' && ch <= '~') {
                int sixel_val = ch - '?';
                int repeat = 1;

                if (ACTIVE_SESSION.sixel.parse_state == SIXEL_STATE_REPEAT) {
                    if (ACTIVE_SESSION.sixel.repeat_count > 0) repeat = ACTIVE_SESSION.sixel.repeat_count;
                    ACTIVE_SESSION.sixel.parse_state = SIXEL_STATE_NORMAL;
                    ACTIVE_SESSION.sixel.repeat_count = 0;
                }

                for (int r = 0; r < repeat; r++) {
                    if (ACTIVE_SESSION.sixel.strip_count < ACTIVE_SESSION.sixel.strip_capacity) {
                        GPUSixelStrip* strip = &ACTIVE_SESSION.sixel.strips[ACTIVE_SESSION.sixel.strip_count++];
                        strip->x = ACTIVE_SESSION.sixel.pos_x + r;
                        strip->y = ACTIVE_SESSION.sixel.pos_y; // Top of the 6-pixel column
                        strip->pattern = sixel_val; // 6 bits
                        strip->color_index = ACTIVE_SESSION.sixel.color_index;
                    }
                }
                ACTIVE_SESSION.sixel.pos_x += repeat;
                if (ACTIVE_SESSION.sixel.pos_x > ACTIVE_SESSION.sixel.max_x) {
                    ACTIVE_SESSION.sixel.max_x = ACTIVE_SESSION.sixel.pos_x;
                }
                if (ACTIVE_SESSION.sixel.pos_y + 6 > ACTIVE_SESSION.sixel.max_y) {
                    ACTIVE_SESSION.sixel.max_y = ACTIVE_SESSION.sixel.pos_y + 6;
                }
            }
            break;
    }
}

void InitSixelGraphics(void) {
    ACTIVE_SESSION.sixel.active = false;
    if (ACTIVE_SESSION.sixel.data) {
        free(ACTIVE_SESSION.sixel.data);
    }
    ACTIVE_SESSION.sixel.data = NULL;
    ACTIVE_SESSION.sixel.width = 0;
    ACTIVE_SESSION.sixel.height = 0;
    ACTIVE_SESSION.sixel.x = 0;
    ACTIVE_SESSION.sixel.y = 0;

    if (ACTIVE_SESSION.sixel.strips) {
        free(ACTIVE_SESSION.sixel.strips);
    }
    ACTIVE_SESSION.sixel.strips = NULL;
    ACTIVE_SESSION.sixel.strip_count = 0;
    ACTIVE_SESSION.sixel.strip_capacity = 0;

    // Initialize standard palette (using global terminal palette as default)
    for (int i = 0; i < 256; i++) {
        ACTIVE_SESSION.sixel.palette[i] = color_palette[i];
    }
    ACTIVE_SESSION.sixel.parse_state = SIXEL_STATE_NORMAL;
    ACTIVE_SESSION.sixel.param_buffer_idx = 0;
    memset(ACTIVE_SESSION.sixel.param_buffer, 0, sizeof(ACTIVE_SESSION.sixel.param_buffer));
}

void ProcessSixelData(const char* data, size_t length) {
    // Basic sixel processing - this is a complex format
    // This implementation provides framework for sixel support

    if (!ACTIVE_SESSION.conformance.features.vt320_mode) {
        LogUnsupportedSequence("Sixel graphics require VT320+ mode");
        return;
    }

    // Allocate sixel staging buffer
    if (!ACTIVE_SESSION.sixel.strips) {
        ACTIVE_SESSION.sixel.strip_capacity = 65536;
        ACTIVE_SESSION.sixel.strips = (GPUSixelStrip*)calloc(ACTIVE_SESSION.sixel.strip_capacity, sizeof(GPUSixelStrip));
    }
    ACTIVE_SESSION.sixel.strip_count = 0; // Reset for new image? Or append? Standard DCS q usually starts new.

    ACTIVE_SESSION.sixel.active = true;
    ACTIVE_SESSION.sixel.x = ACTIVE_SESSION.cursor.x * DEFAULT_CHAR_WIDTH;
    ACTIVE_SESSION.sixel.y = ACTIVE_SESSION.cursor.y * DEFAULT_CHAR_HEIGHT;

    // Initialize internal sixel state for parsing
    ACTIVE_SESSION.sixel.pos_x = 0;
    ACTIVE_SESSION.sixel.pos_y = 0;
    ACTIVE_SESSION.sixel.max_x = 0;
    ACTIVE_SESSION.sixel.max_y = 0;
    ACTIVE_SESSION.sixel.color_index = 0;
    ACTIVE_SESSION.sixel.repeat_count = 1;

    // Process the sixel data stream
    for (size_t i = 0; i < length; i++) {
        ProcessSixelChar(data[i]);
    }

    ACTIVE_SESSION.sixel.dirty = true; // Mark for upload
}

void DrawSixelGraphics(void) {
    if (!ACTIVE_SESSION.conformance.features.sixel_graphics || !ACTIVE_SESSION.sixel.active) return;
    // Just mark dirty, real work happens in DrawTerminal
    ACTIVE_SESSION.sixel.dirty = true;
}

// =============================================================================
// RECTANGULAR OPERATIONS (VT420)
// =============================================================================

void ExecuteRectangularOps(void) {
    // CSI Pt ; Pl ; Pb ; Pr $ v - Copy rectangular area
    if (!ACTIVE_SESSION.conformance.features.vt420_mode) {
        LogUnsupportedSequence("Rectangular operations require VT420 mode");
        return;
    }

    int top = GetCSIParam(0, 1) - 1;
    int left = GetCSIParam(1, 1) - 1;
    int bottom = GetCSIParam(2, DEFAULT_TERM_HEIGHT) - 1;
    int right = GetCSIParam(3, DEFAULT_TERM_WIDTH) - 1;

    // Validate rectangle
    if (top >= 0 && left >= 0 && bottom >= top && right >= left &&
        bottom < DEFAULT_TERM_HEIGHT && right < DEFAULT_TERM_WIDTH) {

        VTRectangle rect = {top, left, bottom, right, true};
        CopyRectangle(rect, ACTIVE_SESSION.cursor.x, ACTIVE_SESSION.cursor.y);
    }
}

void ExecuteRectangularOps2(void) {
    // CSI Pt ; Pl ; Pb ; Pr $ w - Request checksum of rectangular area
    if (!ACTIVE_SESSION.conformance.features.vt420_mode) {
        LogUnsupportedSequence("Rectangular operations require VT420 mode");
        return;
    }

    // Calculate checksum and respond
    char response[32];
    snprintf(response, sizeof(response), "\x1BP%d!~0000\x1B\\", GetCSIParam(4, 0));
    QueueResponse(response);
}

void CopyRectangle(VTRectangle src, int dest_x, int dest_y) {
    int width = src.right - src.left + 1;
    int height = src.bottom - src.top + 1;

    // Allocate temporary buffer for copy
    EnhancedTermChar* temp = malloc(width * height * sizeof(EnhancedTermChar));
    if (!temp) return;

    // Copy source to temp buffer
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (src.top + y < DEFAULT_TERM_HEIGHT && src.left + x < DEFAULT_TERM_WIDTH) {
                temp[y * width + x] = *GetActiveScreenCell(&ACTIVE_SESSION, src.top + y, src.left + x);
            }
        }
    }

    // Copy from temp buffer to destination
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dst_y = dest_y + y;
            int dst_x = dest_x + x;

            if (dst_y >= 0 && dst_y < DEFAULT_TERM_HEIGHT && dst_x >= 0 && dst_x < DEFAULT_TERM_WIDTH) {
                *GetActiveScreenCell(&ACTIVE_SESSION, dst_y, dst_x) = temp[y * width + x];
                GetActiveScreenCell(&ACTIVE_SESSION, dst_y, dst_x)->dirty = true;
            }
        }
        if (dest_y + y >= 0 && dest_y + y < DEFAULT_TERM_HEIGHT) {
            ACTIVE_SESSION.row_dirty[dest_y + y] = true;
        }
    }

    free(temp);
}

// =============================================================================
// COMPLETE TERMINAL TESTING FRAMEWORK
// =============================================================================

// Test helper functions
void TestCursorMovement(void) {
    PipelineWriteString("\x1B[2J\x1B[H"); // Clear screen, home cursor
    PipelineWriteString("VT Cursor Movement Test\n");
    PipelineWriteString("Testing basic cursor operations...\n\n");

    // Test cursor positioning
    PipelineWriteString("\x1B[5;10HPosition test");
    PipelineWriteString("\x1B[10;1H");

    // Test cursor movement
    PipelineWriteString("Moving: ");
    PipelineWriteString("\x1B[5CRIGHT ");
    PipelineWriteString("\x1B[3DBACK ");
    PipelineWriteString("\x1B[2AUP ");
    PipelineWriteString("\x1B[1BDOWN\n");

    // Test save/restore
    PipelineWriteString("\x1B[s"); // Save cursor
    PipelineWriteString("\x1B[15;20HTemp position");
    PipelineWriteString("\x1B[u"); // Restore cursor
    PipelineWriteString("Back to saved position\n");

    PipelineWriteString("\nCursor test complete.\n");
}

void TestColors(void) {
    PipelineWriteString("\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString("VT Color Test\n\n");

    // Test basic 16 colors
    PipelineWriteString("Basic 16 colors:\n");
    for (int i = 0; i < 8; i++) {
        PipelineWriteFormat("\x1B[%dm Color %d \x1B[0m", 30 + i, i);
        PipelineWriteFormat("\x1B[%dm Bright %d \x1B[0m\n", 90 + i, i + 8);
    }

    // Test 256 colors (sample)
    PipelineWriteString("\n256-color sample:\n");
    for (int i = 16; i < 32; i++) {
        PipelineWriteFormat("\x1B[38;5;%dm███\x1B[0m", i);
    }
    PipelineWriteString("\n");

    // Test true color
    PipelineWriteString("\nTrue color gradient:\n");
    for (int i = 0; i < 24; i++) {
        int r = (i * 255) / 23;
        PipelineWriteFormat("\x1B[38;2;%d;0;0m█\x1B[0m", r);
    }
    PipelineWriteString("\n\nColor test complete.\n");
}

void TestCharacterSets(void) {
    PipelineWriteString("\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString("VT Character Set Test\n\n");

    // Test DEC Special Graphics
    PipelineWriteString("DEC Special Graphics:\n");
    PipelineWriteString("\x1B(0"); // Select DEC special
    PipelineWriteString("lqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk\n");
    PipelineWriteString("x                             x\n");
    PipelineWriteString("x    DEC Line Drawing Test    x\n");
    PipelineWriteString("x                             x\n");
    PipelineWriteString("mqqqqqqqqqqwqqqqqqqqqqqqqqqqqj\n");
    PipelineWriteString("             x\n");
    PipelineWriteString("             x\n");
    PipelineWriteString("             v\n");
    PipelineWriteString("\x1B(B"); // Back to ASCII

    PipelineWriteString("\nASCII mode restored.\n");
    PipelineWriteString("Character set test complete.\n");
}

void TestMouseTracking(void) {
    PipelineWriteString("\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString("VT Mouse Tracking Test\n\n");

    PipelineWriteString("Enabling mouse tracking...\n");
    PipelineWriteString("\x1B[?1000h"); // Enable mouse tracking

    PipelineWriteString("Click anywhere to test mouse reporting.\n");
    PipelineWriteString("Mouse coordinates will be reported.\n");
    PipelineWriteString("Press ESC to disable mouse tracking.\n\n");

    // Mouse tracking will be handled by the input system
    // Results will appear as the user interacts
}

void TestTerminalModes(void) {
    PipelineWriteString("\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString("VT Terminal Modes Test\n\n");

    // Test insert mode
    PipelineWriteString("Testing insert mode:\n");
    PipelineWriteString("Original: ABCDEF\n");
    PipelineWriteString("ABCDEF\x1B[4D\x1B[4h***\x1B[4l");
    PipelineWriteString("\nAfter insert: AB***CDEF\n\n");

    // Test alternate screen
    PipelineWriteString("Testing alternate screen buffer...\n");
    PipelineWriteString("Switching to alternate screen in 2 seconds...\n");
    // Would need timing mechanism for full demo

    PipelineWriteString("\nMode test complete.\n");
}

void RunAllTests(void) {
    PipelineWriteString("\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString("Running Complete VT Test Suite\n");
    PipelineWriteString("==============================\n\n");

    TestCursorMovement();
    PipelineWriteString("\nPress any key to continue...\n");
    // Would wait for input in full implementation

    TestColors();
    PipelineWriteString("\nPress any key to continue...\n");

    TestCharacterSets();
    PipelineWriteString("\nPress any key to continue...\n");

    TestTerminalModes();

    PipelineWriteString("\n\nAll tests completed!\n");
    ShowTerminalInfo();
}

void RunVTTest(const char* test_name) {
    if (strcmp(test_name, "cursor") == 0) {
        TestCursorMovement();
    } else if (strcmp(test_name, "colors") == 0) {
        TestColors();
    } else if (strcmp(test_name, "charset") == 0) {
        TestCharacterSets();
    } else if (strcmp(test_name, "mouse") == 0) {
        TestMouseTracking();
    } else if (strcmp(test_name, "modes") == 0) {
        TestTerminalModes();
    } else if (strcmp(test_name, "all") == 0) {
        RunAllTests();
    } else {
        PipelineWriteFormat("Unknown test: %s\n", test_name);
        PipelineWriteString("Available tests: cursor, colors, charset, mouse, modes, all\n");
    }
}

void ShowTerminalInfo(void) {
    PipelineWriteString("\n");
    PipelineWriteString("Terminal Information\n");
    PipelineWriteString("===================\n");
    PipelineWriteFormat("Terminal Type: %s\n", ACTIVE_SESSION.title.terminal_name);
    PipelineWriteFormat("VT Level: %d\n", ACTIVE_SESSION.conformance.level);
    PipelineWriteFormat("Primary DA: %s\n", ACTIVE_SESSION.device_attributes);
    PipelineWriteFormat("Secondary DA: %s\n", ACTIVE_SESSION.secondary_attributes);

    PipelineWriteString("\nSupported Features:\n");
    PipelineWriteFormat("- VT52 Mode: %s\n", ACTIVE_SESSION.conformance.features.vt52_mode ? "Yes" : "No");
    PipelineWriteFormat("- VT100 Mode: %s\n", ACTIVE_SESSION.conformance.features.vt100_mode ? "Yes" : "No");
    PipelineWriteFormat("- VT220 Mode: %s\n", ACTIVE_SESSION.conformance.features.vt220_mode ? "Yes" : "No");
    PipelineWriteFormat("- VT320 Mode: %s\n", ACTIVE_SESSION.conformance.features.vt320_mode ? "Yes" : "No");
    PipelineWriteFormat("- VT420 Mode: %s\n", ACTIVE_SESSION.conformance.features.vt420_mode ? "Yes" : "No");
    PipelineWriteFormat("- VT520 Mode: %s\n", ACTIVE_SESSION.conformance.features.vt520_mode ? "Yes" : "No");
    PipelineWriteFormat("- xterm Mode: %s\n", ACTIVE_SESSION.conformance.features.xterm_mode ? "Yes" : "No");

    PipelineWriteString("\nCurrent Settings:\n");
    PipelineWriteFormat("- Cursor Keys: %s\n", ACTIVE_SESSION.dec_modes.application_cursor_keys ? "Application" : "Normal");
    PipelineWriteFormat("- Keypad: %s\n", ACTIVE_SESSION.vt_keyboard.keypad_mode ? "Application" : "Numeric");
    PipelineWriteFormat("- Auto Wrap: %s\n", ACTIVE_SESSION.dec_modes.auto_wrap_mode ? "On" : "Off");
    PipelineWriteFormat("- Origin Mode: %s\n", ACTIVE_SESSION.dec_modes.origin_mode ? "On" : "Off");
    PipelineWriteFormat("- Insert Mode: %s\n", ACTIVE_SESSION.dec_modes.insert_mode ? "On" : "Off");

    PipelineWriteFormat("\nScrolling Region: %d-%d\n",
                       ACTIVE_SESSION.scroll_top + 1, ACTIVE_SESSION.scroll_bottom + 1);
    PipelineWriteFormat("Margins: %d-%d\n",
                       ACTIVE_SESSION.left_margin + 1, ACTIVE_SESSION.right_margin + 1);

    PipelineWriteString("\nStatistics:\n");
    TerminalStatus status = GetTerminalStatus();
    PipelineWriteFormat("- Pipeline Usage: %zu/%d\n", status.pipeline_usage, (int)sizeof(ACTIVE_SESSION.input_pipeline));
    PipelineWriteFormat("- Key Buffer: %zu\n", status.key_usage);
    PipelineWriteFormat("- Unsupported Sequences: %d\n", ACTIVE_SESSION.conformance.compliance.unsupported_sequences);

    if (ACTIVE_SESSION.conformance.compliance.last_unsupported[0]) {
        PipelineWriteFormat("- Last Unsupported: %s\n", ACTIVE_SESSION.conformance.compliance.last_unsupported);
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
 * This is a convenience wrapper around PipelineWriteChar().
 * @param ch The character to output.
 */
void Script_PutChar(unsigned char ch) {
    PipelineWriteChar(ch);
}

/**
 * @brief Prints a null-terminated string to the terminal's input pipeline.
 * Part of the scripting API. Convenience wrapper around PipelineWriteString().
 * Useful for displaying messages from the hosting application on the terminal.
 * @param text The string to print.
 */
void Script_Print(const char* text) {
    PipelineWriteString(text);
}

/**
 * @brief Prints a formatted string to the terminal's input pipeline.
 * Part of the scripting API. Convenience wrapper around PipelineWriteFormat().
 * Allows for dynamic string construction for display by the hosting application.
 * @param format The printf-style format string.
 * @param ... Additional arguments for the format string.
 */
void Script_Printf(const char* format, ...) {
    char buffer[1024]; // Note: For very long formatted strings, consider dynamic allocation or a larger buffer.
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    PipelineWriteString(buffer);
}

/**
 * @brief Clears the terminal screen and homes the cursor.
 * Part of the scripting API. This sends the standard escape sequences:
 * "ESC[2J" (Erase Display: entire screen) and "ESC[H" (Cursor Home: top-left).
 */
void Script_Cls(void) {
    PipelineWriteString("\x1B[2J\x1B[H");
}

/**
 * @brief Sets basic ANSI foreground and background colors for subsequent text output via the Scripting API.
 * Part of the scripting API.
 * This sends SGR (Select Graphic Rendition) escape sequences.
 * @param fg Foreground color index (0-7 for standard, 8-15 for bright).
 * @param bg Background color index (0-7 for standard, 8-15 for bright).
 */
void Script_SetColor(int fg, int bg) {
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
    PipelineWriteString(color_seq);
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
 *  - Updates internal feature flags in `ACTIVE_SESSION.conformance.features`.
 * The library aims for compatibility with VT52, VT100, VT220, VT320, VT420, and xterm standards.
 *
 * @param level The desired VTLevel (e.g., VT_LEVEL_100, VT_LEVEL_XTERM).
 * @see VTLevel enum for available levels.
 * @see ACTIVE_SESSION.h header documentation for a full list of KEY FEATURES and their typical VT level requirements.
 */
// Statically define the feature sets for each VT level for easy lookup.
typedef struct {
    VTLevel level;
    VTFeatures features;
} VTLevelFeatureMapping;

static const VTLevelFeatureMapping vt_level_mappings[] = {
    { VT_LEVEL_52, { .vt52_mode = true } },
    { VT_LEVEL_100, { .vt100_mode = true, .national_charsets = true } },
    { VT_LEVEL_102, { .vt100_mode = true, .vt102_mode = true, .national_charsets = true } },
    { VT_LEVEL_132, { .vt100_mode = true, .vt102_mode = true, .vt132_mode = true, .national_charsets = true } },
    { VT_LEVEL_220, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true } },
    { VT_LEVEL_320, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true } },
    { VT_LEVEL_340, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true } },
    { VT_LEVEL_420, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true, .rectangular_operations = true, .selective_erase = true } },
    { VT_LEVEL_510, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt510_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true, .rectangular_operations = true, .selective_erase = true } },
    { VT_LEVEL_520, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt510_mode = true, .vt520_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true, .rectangular_operations = true, .selective_erase = true, .locator = true, .multi_session_mode = true } },
    { VT_LEVEL_525, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt510_mode = true, .vt520_mode = true, .vt525_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true, .rectangular_operations = true, .selective_erase = true, .locator = true, .true_color = true, .multi_session_mode = true } },
    { VT_LEVEL_XTERM, {
        .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt520_mode = true, .xterm_mode = true,
        .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true,
        .rectangular_operations = true, .selective_erase = true, .locator = true, .true_color = true,
        .mouse_tracking = true, .alternate_screen = true, .window_manipulation = true
    }},
    { VT_LEVEL_K95, { .k95_mode = true } },
    { VT_LEVEL_TT, { .tt_mode = true } },
    { VT_LEVEL_PUTTY, { .putty_mode = true } },
};

void SetVTLevel(VTLevel level) {
    bool level_found = false;
    for (size_t i = 0; i < sizeof(vt_level_mappings) / sizeof(vt_level_mappings[0]); i++) {
        if (vt_level_mappings[i].level == level) {
            ACTIVE_SESSION.conformance.features = vt_level_mappings[i].features;
            level_found = true;
            break;
        }
    }

    if (!level_found) {
        // Default to a safe baseline if unknown
        ACTIVE_SESSION.conformance.features = vt_level_mappings[0].features;
    }

    ACTIVE_SESSION.conformance.level = level;

    // Update Device Attribute strings based on the level.
    if (level == VT_LEVEL_XTERM) {
        strcpy(ACTIVE_SESSION.device_attributes, "\x1B[?41;1;2;6;7;8;9;15;18;21;22c");
        strcpy(ACTIVE_SESSION.secondary_attributes, "\x1B[>41;400;0c");
        strcpy(ACTIVE_SESSION.tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_525) {
        strcpy(ACTIVE_SESSION.device_attributes, "\x1B[?65;1;2;6;7;8;9;15;18;21;22;28;29c");
        strcpy(ACTIVE_SESSION.secondary_attributes, "\x1B[>52;10;0c");
        strcpy(ACTIVE_SESSION.tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_520) {
        strcpy(ACTIVE_SESSION.device_attributes, "\x1B[?65;1;2;6;7;8;9;15;18;21;22;28;29c");
        strcpy(ACTIVE_SESSION.secondary_attributes, "\x1B[>52;10;0c");
        strcpy(ACTIVE_SESSION.tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_420) {
        strcpy(ACTIVE_SESSION.device_attributes, "\x1B[?64;1;2;6;7;8;9;15;18;21;22;28;29c");
        strcpy(ACTIVE_SESSION.secondary_attributes, "\x1B[>41;10;0c");
        strcpy(ACTIVE_SESSION.tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_340 || level >= VT_LEVEL_320) {
        strcpy(ACTIVE_SESSION.device_attributes, "\x1B[?63;1;2;6;7;8;9;15;18;21c");
        strcpy(ACTIVE_SESSION.secondary_attributes, "\x1B[>24;10;0c");
        strcpy(ACTIVE_SESSION.tertiary_attributes, "");
    } else if (level >= VT_LEVEL_220) {
        strcpy(ACTIVE_SESSION.device_attributes, "\x1B[?62;1;2;6;7;8;9;15c");
        strcpy(ACTIVE_SESSION.secondary_attributes, "\x1B[>1;10;0c");
        strcpy(ACTIVE_SESSION.tertiary_attributes, "");
    } else if (level >= VT_LEVEL_102) {
        strcpy(ACTIVE_SESSION.device_attributes, "\x1B[?6c");
        strcpy(ACTIVE_SESSION.secondary_attributes, "\x1B[>0;95;0c");
        strcpy(ACTIVE_SESSION.tertiary_attributes, "");
    } else if (level >= VT_LEVEL_100) {
        strcpy(ACTIVE_SESSION.device_attributes, "\x1B[?1;2c");
        strcpy(ACTIVE_SESSION.secondary_attributes, "\x1B[>0;95;0c");
        strcpy(ACTIVE_SESSION.tertiary_attributes, "");
    } else { // VT52
        strcpy(ACTIVE_SESSION.device_attributes, "\x1B/Z");
        ACTIVE_SESSION.secondary_attributes[0] = '\0';
        ACTIVE_SESSION.tertiary_attributes[0] = '\0';
    }
}

VTLevel GetVTLevel(void) {
    return ACTIVE_SESSION.conformance.level;
}

// --- Keyboard Input ---

/**
 * @brief Retrieves a fully processed keyboard event from the terminal's internal buffer.
 * The application hosting the terminal should call this function repeatedly (e.g., in its
 * main loop after `UpdateVTKeyboard()`) to obtain keyboard input.
 *
 * The `VTKeyboard` system, updated by `UpdateVTKeyboard()`, translates raw Situation key
 * presses into appropriate VT sequences or characters. This processing considers:
 *  - Modifier keys (Shift, Ctrl, Alt/Meta).
 *  - Terminal modes such as:
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
 * @see UpdateVTKeyboard() which captures Situation input and populates the event buffer.
 * @see VTKeyEvent struct for details on the event data fields.
 * @note The terminal platform provides robust keyboard translation, ensuring that applications
 *       running within the terminal receive the correct input sequences based on active modes.
 */
bool GetVTKeyEvent(VTKeyEvent* event) {
    if (!event || ACTIVE_SESSION.vt_keyboard.buffer_count == 0) {
        return false;
    }
    *event = ACTIVE_SESSION.vt_keyboard.buffer[ACTIVE_SESSION.vt_keyboard.buffer_tail];
    ACTIVE_SESSION.vt_keyboard.buffer_tail = (ACTIVE_SESSION.vt_keyboard.buffer_tail + 1) % 512; // Assuming vt_keyboard.buffer size is 512
    ACTIVE_SESSION.vt_keyboard.buffer_count--;
    return true;
}

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
 *  - `ACTIVE_SESSION.options.log_unsupported`: Specifically for unsupported sequences.
 *  - `ACTIVE_SESSION.options.conformance_checking`: For stricter checks against VT standards.
 *  - `ACTIVE_SESSION.status.debugging`: A general flag indicating debug activity.
 *
 * This capability allows developers to understand the terminal's interaction with
 * host applications and identify issues in escape sequence processing.
 *
 * @param enable `true` to enable debug mode, `false` to disable.
 */
void EnableDebugMode(bool enable) {
    ACTIVE_SESSION.options.debug_sequences = enable;
    ACTIVE_SESSION.options.log_unsupported = enable;
    ACTIVE_SESSION.options.conformance_checking = enable;
    ACTIVE_SESSION.status.debugging = enable; // General debugging flag for other parts of the library
}

// --- Core Terminal Loop Functions ---

/**
 * @brief Updates the terminal's internal state and processes incoming data.
 *
 * Called once per frame in the main loop, this function drives the terminal emulation by:
 * - **Processing Input**: Consumes characters from `ACTIVE_SESSION.input_pipeline` via `ProcessPipeline()`, parsing VT52/xterm sequences with `ProcessChar()`.
 * - **Handling Input Devices**: Updates keyboard (`UpdateVTKeyboard()`) and mouse (`UpdateMouse()`) states.
 * - **Auto-Printing**: Queues lines for printing when `ACTIVE_SESSION.auto_print_enabled` and a newline occurs.
 * - **Managing Timers**: Updates cursor blink, text blink, and visual bell timers for visual effects.
 * - **Flushing Responses**: Sends queued responses (e.g., DSR, DA, focus events) via `response_callback`.
 * - **Rendering**: Draws the terminal display with `DrawTerminal()`, including the custom mouse cursor.
 *
 * Performance is tuned via `ACTIVE_SESSION.VTperformance` (e.g., `chars_per_frame`, `time_budget`) to balance responsiveness and throughput.
 *
 * @see ProcessPipeline() for input processing details.
 * @see UpdateVTKeyboard() for keyboard handling.
 * @see UpdateMouse() for mouse and focus event handling.
 * @see DrawTerminal() for rendering details.
 * @see QueueResponse() for response queuing.
 */
void UpdateTerminal(void) {
    terminal.pending_session_switch = -1; // Reset pending switch
    int saved_session = terminal.active_session;

    // Process all sessions
    for (int i = 0; i < MAX_SESSIONS; i++) {
        terminal.active_session = i;

        // Process input from the pipeline
        ProcessPipeline();

        // Update timers and bells for this session
        if (ACTIVE_SESSION.cursor.blink_enabled && ACTIVE_SESSION.dec_modes.cursor_visible) {
            ACTIVE_SESSION.cursor.blink_state = SituationTimerGetOscillatorState(250);
        } else {
            ACTIVE_SESSION.cursor.blink_state = true;
        }
        ACTIVE_SESSION.text_blink_state = SituationTimerGetOscillatorState(255);

        if (ACTIVE_SESSION.visual_bell_timer > 0) {
            ACTIVE_SESSION.visual_bell_timer -= SituationGetFrameTime();
            if (ACTIVE_SESSION.visual_bell_timer < 0) ACTIVE_SESSION.visual_bell_timer = 0;
        }

        // Flush responses
        if (ACTIVE_SESSION.response_length > 0 && terminal.response_callback) {
            terminal.response_callback(ACTIVE_SESSION.answerback_buffer, ACTIVE_SESSION.response_length);
            ACTIVE_SESSION.response_length = 0;
        }
    }

    // Restore active session for input handling, unless a switch occurred
    if (terminal.pending_session_switch != -1) {
        terminal.active_session = terminal.pending_session_switch;
    } else {
        terminal.active_session = saved_session;
    }

    // Update input devices (Keyboard/Mouse) for the ACTIVE session only
    UpdateVTKeyboard();
    UpdateMouse();

    // Process queued keyboard events for ACTIVE session
    while (ACTIVE_SESSION.vt_keyboard.buffer_count > 0) {
        VTKeyEvent* event = &ACTIVE_SESSION.vt_keyboard.buffer[ACTIVE_SESSION.vt_keyboard.buffer_tail];
        if (event->sequence[0] != '\0') {
            QueueResponse(event->sequence);
            if (ACTIVE_SESSION.dec_modes.local_echo) {
                for (int i = 0; event->sequence[i] != '\0'; i++) {
                    PipelineWriteChar(event->sequence[i]);
                }
            }
            if (event->sequence[0] == 0x07) {
                ACTIVE_SESSION.visual_bell_timer = 0.2f;
            }
        }
        ACTIVE_SESSION.vt_keyboard.buffer_tail = (ACTIVE_SESSION.vt_keyboard.buffer_tail + 1) % KEY_EVENT_BUFFER_SIZE;
        ACTIVE_SESSION.vt_keyboard.buffer_count--;
    }

    // Auto-print (Active session only for now, or loop?)
    // Let's assume auto-print works for active session interaction.
    if (ACTIVE_SESSION.printer_available && ACTIVE_SESSION.auto_print_enabled) {
        if (ACTIVE_SESSION.cursor.y > ACTIVE_SESSION.last_cursor_y && ACTIVE_SESSION.last_cursor_y >= 0) {
            // Queue the previous line for printing
            char print_buffer[DEFAULT_TERM_WIDTH + 2];
            size_t pos = 0;
            for (int x = 0; x < DEFAULT_TERM_WIDTH && pos < DEFAULT_TERM_WIDTH; x++) {
                EnhancedTermChar* cell = GetScreenCell(&ACTIVE_SESSION, ACTIVE_SESSION.last_cursor_y, x);
                print_buffer[pos++] = GetPrintableChar(cell->ch, &ACTIVE_SESSION.charset);
            }
            if (pos < DEFAULT_TERM_WIDTH + 1) {
                print_buffer[pos++] = '\n';
                print_buffer[pos] = '\0';
                QueueResponse(print_buffer);
            }
        }
        ACTIVE_SESSION.last_cursor_y = ACTIVE_SESSION.cursor.y;
    }

    DrawTerminal();
}


/**
 * @brief Renders the current visual state of the terminal to the Situation window.
 * This function must be called once per frame, within SituationBeginFrame()`
 * and `SituationEndFrame()` block. It translates the terminal's internal model into a
 * graphical representation.
 *
 * Key rendering capabilities and processes:
 *  -   **Character Grid**: Iterates through the active screen buffer (primary or alternate),
 *      drawing each `EnhancedTermChar`.
 *  -   **Color Resolution**: Handles various color modes for foreground and background:
 *      - Standard 16 ANSI colors.
 *      - 256-color indexed palette.
 *      - 24-bit True Color (RGB).
 *  -   **Text Attributes**: Applies a rich set of visual attributes:
 *      - Bold (typically by using brighter ANSI colors or a bold font variant).
 *      - Faint (dimmed text).
 *      - Italic (if supported by font or simulated).
 *      - Underline (single and double).
 *      - Blink (text alternates visibility based on `ACTIVE_SESSION.text_blink_state`).
 *      - Reverse video (swaps foreground and background colors, per cell or screen-wide).
 *      - Strikethrough.
 *      - Overline.
 *      - Concealed text (rendered as spaces or not at all).
 *  -   **Glyph Rendering**: Uses a pre-rendered bitmap font (`font_texture`), typically
 *      CP437-based for broad character support within that range. Unicode characters
 *      outside this basic set might be mapped to '?' or require a more advanced font system.
 *  -   **Cursor**: Draws the cursor according to its current `EnhancedCursor` properties:
 *      - Shape: `CURSOR_BLOCK`, `CURSOR_UNDERLINE`, `CURSOR_BAR`.
 *      - Blink: Synchronized with `ACTIVE_SESSION.cursor.blink_state`.
 *      - Visibility: Honors `ACTIVE_SESSION.cursor.visible`.
 *      - Color: Uses `ACTIVE_SESSION.cursor.color`.
 *  -   **Sixel Graphics**: If `ACTIVE_SESSION.sixel.active` is true, Sixel graphics data is
 *      rendered, typically overlaid on the text grid.
 *  -   **Visual Bell**: If `ACTIVE_SESSION.visual_bell_timer` is active, a visual flash effect
 *      may be rendered.
 *
 * The terminal provides a faithful visual emulation, leveraging Situation for efficient
 * 2D rendering.
 *
 * @see EnhancedTermChar for the structure defining each character cell's properties.
 * @see EnhancedCursor for cursor attributes.
 * @see SixelGraphics for Sixel display state.
 * @see InitTerminal() where `font_texture` is created.
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
static void BiDiReorderRow(EnhancedTermChar* row, int width) {
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

static void UpdateTerminalRow(TerminalSession* source_session, int dest_y, int source_y) {
    // --- BiDi Processing (Visual Reordering) ---
    // We copy the row to a temporary buffer to reorder it for display
    // without modifying the logical screen buffer.
    EnhancedTermChar temp_row[DEFAULT_TERM_WIDTH];
    EnhancedTermChar* src_row_ptr = GetScreenRow(source_session, source_y);
    memcpy(temp_row, src_row_ptr, DEFAULT_TERM_WIDTH * sizeof(EnhancedTermChar));

    bool has_rtl = false;
    for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
        if (IsRTL(temp_row[x].ch)) {
            has_rtl = true;
            break;
        }
    }

    if (has_rtl) {
        BiDiReorderRow(temp_row, DEFAULT_TERM_WIDTH);
    }
    // -------------------------------------------

    for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
        EnhancedTermChar* cell = &temp_row[x];
        GPUCell* gpu_cell = &terminal.gpu_staging_buffer[dest_y * DEFAULT_TERM_WIDTH + x];

        // Dynamic Glyph Mapping
        uint32_t char_code;
        if (cell->ch < 256) {
            char_code = cell->ch; // Base set
        } else {
            char_code = AllocateGlyph(cell->ch);
        }
        gpu_cell->char_code = char_code;

        // Update LRU if it's a dynamic glyph
        if (char_code >= 256 && char_code != '?') {
            terminal.glyph_last_used[char_code] = terminal.frame_count;
        }

        Color fg = {255, 255, 255, 255};
        if (cell->fg_color.color_mode == 0) {
             if (cell->fg_color.value.index < 16) fg = ansi_colors[cell->fg_color.value.index];
             else {
                 RGB_Color c = color_palette[cell->fg_color.value.index];
                 fg = (Color){c.r, c.g, c.b, 255};
             }
        } else {
            fg = (Color){cell->fg_color.value.rgb.r, cell->fg_color.value.rgb.g, cell->fg_color.value.rgb.b, 255};
        }
        gpu_cell->fg_color = (uint32_t)fg.r | ((uint32_t)fg.g << 8) | ((uint32_t)fg.b << 16) | ((uint32_t)fg.a << 24);

        Color bg = {0, 0, 0, 255};
        if (cell->bg_color.color_mode == 0) {
             if (cell->bg_color.value.index < 16) bg = ansi_colors[cell->bg_color.value.index];
             else {
                 RGB_Color c = color_palette[cell->bg_color.value.index];
                 bg = (Color){c.r, c.g, c.b, 255};
             }
        } else {
            bg = (Color){cell->bg_color.value.rgb.r, cell->bg_color.value.rgb.g, cell->bg_color.value.rgb.b, 255};
        }
        gpu_cell->bg_color = (uint32_t)bg.r | ((uint32_t)bg.g << 8) | ((uint32_t)bg.b << 16) | ((uint32_t)bg.a << 24);

        gpu_cell->flags = 0;
        if (cell->bold) gpu_cell->flags |= GPU_ATTR_BOLD;
        if (cell->faint) gpu_cell->flags |= GPU_ATTR_FAINT;
        if (cell->italic) gpu_cell->flags |= GPU_ATTR_ITALIC;
        if (cell->underline) gpu_cell->flags |= GPU_ATTR_UNDERLINE;
        if (cell->blink) gpu_cell->flags |= GPU_ATTR_BLINK;
        if (cell->reverse ^ source_session->dec_modes.reverse_video) gpu_cell->flags |= GPU_ATTR_REVERSE;
        if (cell->strikethrough) gpu_cell->flags |= GPU_ATTR_STRIKE;
        if (cell->double_width) gpu_cell->flags |= GPU_ATTR_DOUBLE_WIDTH;
        if (cell->double_height_top) gpu_cell->flags |= GPU_ATTR_DOUBLE_HEIGHT_TOP;
        if (cell->double_height_bottom) gpu_cell->flags |= GPU_ATTR_DOUBLE_HEIGHT_BOT;
        if (cell->conceal) gpu_cell->flags |= GPU_ATTR_CONCEAL;
    }
    source_session->row_dirty[source_y] = false;
}

void UpdateTerminalSSBO(void) {
    if (!terminal.terminal_buffer.id || !terminal.gpu_staging_buffer) return;

    // Determine which sessions are visible
    bool split = terminal.split_screen_active;
    int top_idx = terminal.session_top;
    int bot_idx = terminal.session_bottom;
    int split_y = terminal.split_row;

    if (!split) {
        // Single session mode (active session)
        // Wait, if not split, we should probably show the active session?
        // Or strictly session_top? Let's use active_session for single view consistency.
        top_idx = terminal.active_session;
        split_y = DEFAULT_TERM_HEIGHT; // All rows from top_idx
    }

    size_t required_size = DEFAULT_TERM_WIDTH * DEFAULT_TERM_HEIGHT * sizeof(GPUCell);
    bool any_upload_needed = false;

    // Update global LRU clock
    terminal.frame_count++;

    // We reconstruct the whole buffer every frame if mixed?
    // Optimization: Check dirty flags.
    // Since we are compositing, we should probably just write to staging buffer always if dirty.

    for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
        TerminalSession* source_session;
        int source_y = y;

        if (y <= split_y) {
            source_session = &terminal.sessions[top_idx];
        } else {
            source_session = &terminal.sessions[bot_idx];
            // Do we map y to 0 for bottom session?
            // VT520 split screen usually shows two independent scrolling regions.
            // If we split at row 25. Row 26 is Row 0 of bottom session?
            // "Session 1 on the top half and Session 2 on the bottom half."
            // Usually implied distinct viewports.
            // Let's assume bottom session maps to y - (split_y + 1).
            source_y = y - (split_y + 1);
            if (source_y >= DEFAULT_TERM_HEIGHT) continue; // Out of bounds
        }

        // Check if row is dirty in source OR if we just switched layout?
        // For simplicity in this PR, we assume we update if source row is dirty.
        // But if we toggle split screen, we need full redraw.
        // The calling code doesn't set dirty on toggle.
        // We will just upload dirty rows.

        if (source_session->row_dirty[source_y]) {
            UpdateTerminalRow(source_session, y, source_y);
            any_upload_needed = true;
        }
    }

    // Only update buffer if data changed to save bandwidth
    if (any_upload_needed) {
        SituationUpdateBuffer(terminal.terminal_buffer, 0, required_size, terminal.gpu_staging_buffer);
    }
}

// New API functions


void DrawTerminal(void) {
    if (!terminal.compute_initialized) return;

    // Handle Soft Font Update
    if (ACTIVE_SESSION.soft_font.dirty || terminal.font_atlas_dirty) {
        if (terminal.font_atlas_pixels) {
            SituationImage img = {0};
            img.width = terminal.atlas_width;
            img.height = terminal.atlas_height;
            img.channels = 4;
            img.data = terminal.font_atlas_pixels; // Pointer alias, don't free

            // Re-upload full texture
            if (terminal.font_texture.generation != 0) SituationDestroyTexture(&terminal.font_texture);
            SituationCreateTexture(img, false, &terminal.font_texture);
        }
        ACTIVE_SESSION.soft_font.dirty = false;
        terminal.font_atlas_dirty = false;
    }

    // Handle Sixel Texture Creation/Upload (Compute Shader)
    if (ACTIVE_SESSION.sixel.active && ACTIVE_SESSION.sixel.strips) {
        // 1. Check if dirty (new data)
        if (ACTIVE_SESSION.sixel.dirty) {
            // Upload Strips
            if (ACTIVE_SESSION.sixel.strip_count > 0) {
                SituationUpdateBuffer(terminal.sixel_buffer, 0, ACTIVE_SESSION.sixel.strip_count * sizeof(GPUSixelStrip), ACTIVE_SESSION.sixel.strips);
            }

            // Repack Palette safely
            uint32_t packed_palette[256];
            for(int i=0; i<256; i++) {
                RGB_Color c = ACTIVE_SESSION.sixel.palette[i];
                packed_palette[i] = (uint32_t)c.r | ((uint32_t)c.g << 8) | ((uint32_t)c.b << 16) | ((uint32_t)c.a << 24);
            }
            SituationUpdateBuffer(terminal.sixel_palette_buffer, 0, 256 * sizeof(uint32_t), packed_palette);

            // 2. Dispatch Compute Shader to render to texture
            // FORCE RECREATE TEXTURE TO CLEAR IT (Essential for ytop/lsix to prevent smearing)
            if (terminal.sixel_texture.generation != 0) SituationDestroyTexture(&terminal.sixel_texture);

            SituationImage img = {0};
            // CreateImage typically returns zeroed buffer
            SituationCreateImage(ACTIVE_SESSION.sixel.width, ACTIVE_SESSION.sixel.height, 4, &img);
            if (img.data) memset(img.data, 0, ACTIVE_SESSION.sixel.width * ACTIVE_SESSION.sixel.height * 4); // Ensure zeroed

            SituationCreateTextureEx(img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_DST, &terminal.sixel_texture);
            SituationUnloadImage(img);

            if (SituationAcquireFrameCommandBuffer()) {
                SituationCommandBuffer cmd = SituationGetMainCommandBuffer();
                SituationCmdBindComputePipeline(cmd, terminal.sixel_pipeline);
                SituationCmdBindComputeTexture(cmd, 0, terminal.sixel_texture);

                // Push Constants
                TerminalPushConstants pc = {0};
                pc.screen_size = (Vector2){{(float)ACTIVE_SESSION.sixel.width, (float)ACTIVE_SESSION.sixel.height}};
                pc.vector_count = ACTIVE_SESSION.sixel.strip_count;
                pc.vector_buffer_addr = SituationGetBufferDeviceAddress(terminal.sixel_buffer); // Reusing field for sixel buffer
                pc.terminal_buffer_addr = SituationGetBufferDeviceAddress(terminal.sixel_palette_buffer); // Reusing field for palette

                SituationCmdSetPushConstant(cmd, 0, &pc, sizeof(pc));
                SituationCmdDispatch(cmd, (ACTIVE_SESSION.sixel.strip_count + 63) / 64, 1, 1);
                SituationCmdPipelineBarrier(cmd, SITUATION_BARRIER_COMPUTE_SHADER_WRITE, SITUATION_BARRIER_COMPUTE_SHADER_READ);
            }
            ACTIVE_SESSION.sixel.dirty = false;
        }
    }

    UpdateTerminalSSBO();

    if (SituationAcquireFrameCommandBuffer()) {
        SituationCommandBuffer cmd = SituationGetMainCommandBuffer();

        // --- Vector Layer Management (Storage Tube) ---
        if (terminal.vector_clear_request) {
            // Clear the persistent vector layer
            SituationImage clear_img = {0};
            if (SituationCreateImage(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 4, &clear_img) == SITUATION_SUCCESS) {
                memset(clear_img.data, 0, DEFAULT_WINDOW_WIDTH * DEFAULT_WINDOW_HEIGHT * 4);

                if (terminal.vector_layer_texture.generation != 0) {
                    SituationDestroyTexture(&terminal.vector_layer_texture);
                }

                SituationCreateTextureEx(clear_img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_DST, &terminal.vector_layer_texture);
                SituationUnloadImage(clear_img);
            }
            terminal.vector_clear_request = false;
        }

        SituationCmdBindComputePipeline(cmd, terminal.compute_pipeline);

        // Bindless: No Descriptor Sets for Buffers (BDA used)
        SituationCmdBindComputeTexture(cmd, 1, terminal.output_texture);

        TerminalPushConstants pc = {0};
        pc.terminal_buffer_addr = SituationGetBufferDeviceAddress(terminal.terminal_buffer);

        // Full Bindless (Both Backends)
        pc.font_texture_handle = SituationGetTextureHandle(terminal.font_texture);
        if (ACTIVE_SESSION.sixel.active && terminal.sixel_texture.generation != 0) {
            pc.sixel_texture_handle = SituationGetTextureHandle(terminal.sixel_texture);
        } else {
            pc.sixel_texture_handle = SituationGetTextureHandle(terminal.dummy_sixel_texture);
        }
        pc.vector_texture_handle = SituationGetTextureHandle(terminal.vector_layer_texture);
        pc.atlas_cols = terminal.atlas_cols;

        pc.screen_size = (Vector2){{(float)DEFAULT_WINDOW_WIDTH, (float)DEFAULT_WINDOW_HEIGHT}};

        int char_w = DEFAULT_CHAR_WIDTH;
        int char_h = DEFAULT_CHAR_HEIGHT;
        if (ACTIVE_SESSION.soft_font.active) {
            char_w = ACTIVE_SESSION.soft_font.char_width;
            char_h = ACTIVE_SESSION.soft_font.char_height;
        }
        pc.char_size = (Vector2){{(float)char_w, (float)char_h}};

        pc.grid_size = (Vector2){{(float)DEFAULT_TERM_WIDTH, (float)DEFAULT_TERM_HEIGHT}};
        pc.time = (float)SituationTimerGetTime();

        // Calculate visible cursor position
        int cursor_y_screen = -1;
        if (!terminal.split_screen_active) {
            // Single session: cursor is just session cursor Y
            if (terminal.active_session == terminal.session_top) { // Assuming single view uses top slot logic or just active
                 cursor_y_screen = ACTIVE_SESSION.cursor.y;
            } else {
                 // Should not happen if non-split uses active_session as top_idx
                 cursor_y_screen = ACTIVE_SESSION.cursor.y;
            }
        } else {
            // Split screen: check if active session is visible
            if (terminal.active_session == terminal.session_top) {
                if (ACTIVE_SESSION.cursor.y <= terminal.split_row) {
                    cursor_y_screen = ACTIVE_SESSION.cursor.y;
                }
            } else if (terminal.active_session == terminal.session_bottom) {
                // Bottom session starts visually at split_row + 1
                // Its internal row 0 maps to screen row split_row + 1
                // We need to check if cursor fits on screen
                int screen_y = ACTIVE_SESSION.cursor.y + (terminal.split_row + 1);
                if (screen_y < DEFAULT_TERM_HEIGHT) {
                    cursor_y_screen = screen_y;
                }
            }
        }

        if (cursor_y_screen >= 0) {
            pc.cursor_index = cursor_y_screen * DEFAULT_TERM_WIDTH + ACTIVE_SESSION.cursor.x;
        } else {
            pc.cursor_index = 0xFFFFFFFF; // Hide cursor
        }

        // Mouse Cursor
        if (ACTIVE_SESSION.mouse.enabled && ACTIVE_SESSION.mouse.cursor_x > 0) {
             int mx = ACTIVE_SESSION.mouse.cursor_x - 1;
             int my = ACTIVE_SESSION.mouse.cursor_y - 1;

             // Ensure coordinates are within valid bounds to prevent wrapping/overflow
             if (mx >= 0 && mx < DEFAULT_TERM_WIDTH && my >= 0 && my < DEFAULT_TERM_HEIGHT) {
                 pc.mouse_cursor_index = my * DEFAULT_TERM_WIDTH + mx;
             } else {
                 pc.mouse_cursor_index = 0xFFFFFFFF;
             }
        } else {
             pc.mouse_cursor_index = 0xFFFFFFFF;
        }

        pc.cursor_blink_state = ACTIVE_SESSION.cursor.blink_state ? 1 : 0;
        pc.text_blink_state = ACTIVE_SESSION.text_blink_state ? 1 : 0;

        if (ACTIVE_SESSION.selection.active) {
             uint32_t start_idx = ACTIVE_SESSION.selection.start_y * DEFAULT_TERM_WIDTH + ACTIVE_SESSION.selection.start_x;
             uint32_t end_idx = ACTIVE_SESSION.selection.end_y * DEFAULT_TERM_WIDTH + ACTIVE_SESSION.selection.end_x;
             if (start_idx > end_idx) { uint32_t t = start_idx; start_idx = end_idx; end_idx = t; }
             pc.sel_start = start_idx;
             pc.sel_end = end_idx;
             pc.sel_active = 1;
        }
        pc.scanline_intensity = terminal.visual_effects.scanline_intensity;
        pc.crt_curvature = terminal.visual_effects.curvature;

        // Visual Bell
        if (ACTIVE_SESSION.visual_bell_timer > 0.0) {
            // Map 0.2s -> 0.0s to 1.0 -> 0.0 intensity
            float intensity = (float)(ACTIVE_SESSION.visual_bell_timer / 0.2);
            if (intensity > 1.0f) intensity = 1.0f;
            if (intensity < 0.0f) intensity = 0.0f;
            pc.visual_bell_intensity = intensity;
        }

        SituationCmdSetPushConstant(cmd, 0, &pc, sizeof(pc));

        // Dispatch Text (and compositing) Pass
        SituationCmdDispatch(cmd, DEFAULT_TERM_WIDTH, DEFAULT_TERM_HEIGHT, 1);

        // --- Vector Drawing Pass (Storage Tube Accumulation) ---
        if (terminal.vector_count > 0) {
            // Update Vector Buffer with NEW lines
            SituationUpdateBuffer(terminal.vector_buffer, 0, terminal.vector_count * sizeof(GPUVectorLine), terminal.vector_staging_buffer);

            // Execute vector drawing after text pass. The updated vector texture will be composited in the NEXT frame's text pass.
            // This introduces a 1-frame latency for new vectors, which is acceptable for terminal emulation.

            SituationCmdBindComputePipeline(cmd, terminal.vector_pipeline);
            // Bind the Storage Texture
            SituationCmdBindComputeTexture(cmd, 1, terminal.vector_layer_texture); // Read-Write

            // Push Constants
            pc.vector_count = terminal.vector_count;
            pc.vector_buffer_addr = SituationGetBufferDeviceAddress(terminal.vector_buffer);

            SituationCmdSetPushConstant(cmd, 0, &pc, sizeof(pc));

            // Dispatch (64 threads per group)
            SituationCmdDispatch(cmd, (terminal.vector_count + 63) / 64, 1, 1);

            // Barrier for safety
            SituationCmdPipelineBarrier(cmd, SITUATION_BARRIER_COMPUTE_SHADER_WRITE, SITUATION_BARRIER_COMPUTE_SHADER_READ);

            // Reset vector count (Storage Tube behavior: only draw new lines once)
            terminal.vector_count = 0;
        }

        SituationCmdPipelineBarrier(cmd, SITUATION_BARRIER_COMPUTE_SHADER_WRITE, SITUATION_BARRIER_TRANSFER_READ);

        SituationCmdPresent(cmd, terminal.output_texture);

        SituationEndFrame();
    }
}


// --- Lifecycle Management ---

/**
 * @brief Cleans up all resources allocated by the terminal library.
 * This function must be called when the application is shutting down, typically
 * after the main loop has exited and before closing the Situation window (if Situation
 * is managed by the application).
 *
 * Its responsibilities include deallocating:
 *  - The main font texture (`font_texture`) loaded into GPU memory.
 *  - Memory used for storing sequences of programmable keys (`ACTIVE_SESSION.programmable_keys`).
 *  - The Sixel graphics data buffer (`ACTIVE_SESSION.sixel.data`) if it was allocated.
 *  - The buffer for bracketed paste data (`ACTIVE_SESSION.bracketed_paste.buffer`) if used.
 *
 * It also ensures the input pipeline is cleared. Proper cleanup prevents memory leaks
 * and releases GPU resources.
 */
void CleanupTerminal(void) {
    // Free LRU Cache
    if (terminal.glyph_last_used) { free(terminal.glyph_last_used); terminal.glyph_last_used = NULL; }
    if (terminal.atlas_to_codepoint) { free(terminal.atlas_to_codepoint); terminal.atlas_to_codepoint = NULL; }
    if (terminal.font_atlas_pixels) { free(terminal.font_atlas_pixels); terminal.font_atlas_pixels = NULL; }

    if (terminal.font_texture.generation != 0) SituationDestroyTexture(&terminal.font_texture);
    if (terminal.output_texture.generation != 0) SituationDestroyTexture(&terminal.output_texture);
    if (terminal.sixel_texture.generation != 0) SituationDestroyTexture(&terminal.sixel_texture);
    if (terminal.dummy_sixel_texture.generation != 0) SituationDestroyTexture(&terminal.dummy_sixel_texture);
    if (terminal.terminal_buffer.id != 0) SituationDestroyBuffer(&terminal.terminal_buffer);
    if (terminal.compute_pipeline.id != 0) SituationDestroyComputePipeline(&terminal.compute_pipeline);

    if (terminal.gpu_staging_buffer) {
        free(terminal.gpu_staging_buffer);
        terminal.gpu_staging_buffer = NULL;
    }

    // Free session buffers
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (terminal.sessions[i].screen_buffer) {
            free(terminal.sessions[i].screen_buffer);
            terminal.sessions[i].screen_buffer = NULL;
        }
        if (terminal.sessions[i].alt_buffer) {
            free(terminal.sessions[i].alt_buffer);
            terminal.sessions[i].alt_buffer = NULL;
        }
    }

    // Free Vector Engine resources
    if (terminal.vector_buffer.id != 0) SituationDestroyBuffer(&terminal.vector_buffer);
    if (terminal.vector_pipeline.id != 0) SituationDestroyComputePipeline(&terminal.vector_pipeline);
    if (terminal.vector_staging_buffer) {
        free(terminal.vector_staging_buffer);
        terminal.vector_staging_buffer = NULL;
    }

    // Free memory for programmable key sequences
    for (size_t i = 0; i < ACTIVE_SESSION.programmable_keys.count; i++) {
        if (ACTIVE_SESSION.programmable_keys.keys[i].sequence) {
            free(ACTIVE_SESSION.programmable_keys.keys[i].sequence);
            ACTIVE_SESSION.programmable_keys.keys[i].sequence = NULL;
        }
    }
    if (ACTIVE_SESSION.programmable_keys.keys) {
        free(ACTIVE_SESSION.programmable_keys.keys);
        ACTIVE_SESSION.programmable_keys.keys = NULL;
    }
    ACTIVE_SESSION.programmable_keys.count = 0;
    ACTIVE_SESSION.programmable_keys.capacity = 0;

    // Free Sixel graphics data buffer
    if (ACTIVE_SESSION.sixel.data) {
        free(ACTIVE_SESSION.sixel.data);
        ACTIVE_SESSION.sixel.data = NULL;
    }

    // Free bracketed paste buffer
    if (ACTIVE_SESSION.bracketed_paste.buffer) {
        free(ACTIVE_SESSION.bracketed_paste.buffer);
        ACTIVE_SESSION.bracketed_paste.buffer = NULL;
    }

    // Free ReGIS Macros
    for (int i = 0; i < 26; i++) {
        if (terminal.regis.macros[i]) {
            free(terminal.regis.macros[i]);
            terminal.regis.macros[i] = NULL;
        }
    }
    if (terminal.regis.macro_buffer) {
        free(terminal.regis.macro_buffer);
        terminal.regis.macro_buffer = NULL;
    }

    ClearPipeline(); // Ensure input pipeline is empty and reset
}

bool InitTerminalDisplay(void) {
    // Create a virtual display for the terminal
    int vd_id;
    if (SituationCreateVirtualDisplay((Vector2){{(float)DEFAULT_WINDOW_WIDTH, (float)DEFAULT_WINDOW_HEIGHT}}, 1.0, 0, SITUATION_SCALING_INTEGER, SITUATION_BLEND_ALPHA, &vd_id) != SITUATION_SUCCESS) {
        return false;
    }

    // Set the virtual display as renderable
    // SituationSetVirtualDisplayActive(1, true); // Not part of current API

    return true;
}

/*// Response callback to handle all terminal output (keyboard, mouse, focus events, DSR)
static void HandleTerminalResponse(const char* response, int length) {
    // Print response for debugging
    printf("Terminal response: ");
    for (int i = 0; i < length; i++) {
        printf("0x%02X ", (unsigned char)response[i]);
    }
    printf("\n");

    // Echo input back to terminal (simulates PTY)
    PipelineWriteString(response);
}
int main(void) {
    // Initialize Situation window
    SituationInitInfo init_info = { .window_width = DEFAULT_WINDOW_WIDTH, .window_height = DEFAULT_WINDOW_HEIGHT, .window_title = "Terminal", .initial_active_window_flags = SITUATION_WINDOW_STATE_RESIZABLE };
    SituationInit(0, NULL, &init_info);
    SituationSetTargetFPS(60);

    // Initialize terminal state
    InitTerminal();

    // Set response callback
    SetResponseCallback(HandleTerminalResponse);

    // Configure initial settings
    EnableDebugMode(true); // Enable diagnostics
    SetPipelineTargetFPS(60); // Match pipeline to FPS

    // Send initial text
    PipelineWriteString("Welcome to Enhanced VT Terminal!\n");
    PipelineWriteString("\x1B[32mGreen text\x1B[0m | \x1B[1mBold text\x1B[0m\n");
    PipelineWriteString("Type to interact. Try mouse modes:\n");
    PipelineWriteString("- \x1B[?1000h: VT200\n");
    PipelineWriteString("- \x1B[?1006h: SGR\n");
    PipelineWriteString("- \x1B[?1015h: URXVT\n");
    PipelineWriteString("- \x1B[?1016h: Pixel\n");
    PipelineWriteString("Focus window for \x1B[?1004h events.\n");

    // Enable mouse features
    PipelineWriteString("\x1B[?1004h"); // Focus In/Out
    PipelineWriteString("\x1B[?1000h"); // VT200
    PipelineWriteString("\x1B[?1006h"); // SGR

    while (!WindowShouldClose()) {
        // Update and render terminal (all input reported via HandleTerminalResponse)
        UpdateTerminal();

        // Render frame
        SituationBeginFrame();
            DrawFPS(10, 10); // Optional GUI element
        SituationEndFrame();
    }

    // Cleanup resources
    CleanupTerminal();
    SituationShutdown();

    return 0;
}
*/


void InitSession(int index) {
    TerminalSession* session = &terminal.sessions[index];

    session->last_cursor_y = -1;

    // Initialize session defaults
    EnhancedTermChar default_char = {
        .ch = ' ',
        .fg_color = {.color_mode = 0, .value.index = COLOR_WHITE},
        .bg_color = {.color_mode = 0, .value.index = COLOR_BLACK},
        .dirty = true
    };

    // Initialize Ring Buffer
    // Primary buffer includes scrollback
    session->buffer_height = DEFAULT_TERM_HEIGHT + MAX_SCROLLBACK_LINES;
    session->screen_head = 0;
    session->alt_screen_head = 0;
    session->view_offset = 0;
    session->saved_view_offset = 0;

    if (session->screen_buffer) free(session->screen_buffer);
    session->screen_buffer = (EnhancedTermChar*)calloc(session->buffer_height * DEFAULT_TERM_WIDTH, sizeof(EnhancedTermChar));

    if (session->alt_buffer) free(session->alt_buffer);
    // Alt buffer is typically fixed size (no scrollback), but for simplicity/consistency we could make it same size?
    // Usually alt buffer (vi/top) has no scrollback.
    session->alt_buffer = (EnhancedTermChar*)calloc(DEFAULT_TERM_HEIGHT * DEFAULT_TERM_WIDTH, sizeof(EnhancedTermChar));

    // Fill with default char
    for (int i = 0; i < session->buffer_height * DEFAULT_TERM_WIDTH; i++) {
        session->screen_buffer[i] = default_char;
    }

    // Alt buffer is smaller
    for (int i = 0; i < DEFAULT_TERM_HEIGHT * DEFAULT_TERM_WIDTH; i++) {
        session->alt_buffer[i] = default_char;
    }

    // Initialize dirty rows for viewport
    for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
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

    session->text_blink_state = true;
    session->text_blink_timer = 0.0f;
    session->visual_bell_timer = 0.0f;
    session->response_length = 0;
    session->parse_state = VT_PARSE_NORMAL;
    session->left_margin = 0;
    session->right_margin = DEFAULT_TERM_WIDTH - 1;
    session->scroll_top = 0;
    session->scroll_bottom = DEFAULT_TERM_HEIGHT - 1;

    session->dec_modes.application_cursor_keys = false;
    session->dec_modes.origin_mode = false;
    session->dec_modes.auto_wrap_mode = true;
    session->dec_modes.cursor_visible = true;
    session->dec_modes.alternate_screen = false;
    session->dec_modes.insert_mode = false;
    session->dec_modes.new_line_mode = false;
    session->dec_modes.column_mode_132 = false;
    session->dec_modes.local_echo = false;

    session->ansi_modes.insert_replace = false;
    session->ansi_modes.line_feed_new_line = true;

    session->soft_font.active = false;
    session->soft_font.dirty = false;
    session->soft_font.char_width = 8;
    session->soft_font.char_height = 16;

    // Reset attributes manually as ResetAllAttributes depends on ACTIVE_SESSION
    session->current_fg.color_mode = 0; session->current_fg.value.index = COLOR_WHITE;
    session->current_bg.color_mode = 0; session->current_bg.value.index = COLOR_BLACK;
    session->bold_mode = false;
    session->faint_mode = false;
    session->italic_mode = false;
    session->underline_mode = false;
    session->blink_mode = false;
    session->reverse_mode = false;
    session->strikethrough_mode = false;
    session->conceal_mode = false;
    session->overline_mode = false;
    session->double_underline_mode = false;
    session->protected_mode = false;

    session->bracketed_paste.enabled = false;
    session->bracketed_paste.active = false;
    session->bracketed_paste.buffer = NULL;

    session->programmable_keys.keys = NULL;
    session->programmable_keys.count = 0;
    session->programmable_keys.capacity = 0;

    snprintf(session->title.terminal_name, sizeof(session->title.terminal_name), "Session %d", index + 1);
    snprintf(session->title.window_title, sizeof(session->title.window_title), "Terminal Session %d", index + 1);
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

    strncpy(session->answerback_buffer, "terminal_v2 VT420", MAX_COMMAND_BUFFER - 1);
    session->answerback_buffer[MAX_COMMAND_BUFFER - 1] = '\0';

    // Init charsets, tabs, keyboard
    // We can reuse the helper functions if they operate on ACTIVE_SESSION and we switch context
}


void SetActiveSession(int index) {
    if (index >= 0 && index < MAX_SESSIONS) {
        terminal.active_session = index;
        terminal.pending_session_switch = index; // Queue session switch for UpdateTerminal
        // Invalidate screen to force redraw of the new active session
        for(int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
            terminal.sessions[terminal.active_session].row_dirty[y] = true;
        }

        // Update window title for the new session
        if (title_callback) {
            title_callback(terminal.sessions[index].title.window_title, false);
        }
        SituationSetWindowTitle(terminal.sessions[index].title.window_title);
    }
}


void SetSplitScreen(bool active, int row, int top_idx, int bot_idx) {
    terminal.split_screen_active = active;
    if (active) {
        terminal.split_row = row;
        if (top_idx >= 0 && top_idx < MAX_SESSIONS) terminal.session_top = top_idx;
        if (bot_idx >= 0 && bot_idx < MAX_SESSIONS) terminal.session_bottom = bot_idx;

        // Invalidate both sessions to force redraw
        for(int y=0; y<DEFAULT_TERM_HEIGHT; y++) {
            terminal.sessions[terminal.session_top].row_dirty[y] = true;
            terminal.sessions[terminal.session_bottom].row_dirty[y] = true;
        }
    } else {
        // Invalidate active session
         for(int y=0; y<DEFAULT_TERM_HEIGHT; y++) {
            terminal.sessions[terminal.active_session].row_dirty[y] = true;
        }
    }
}


void PipelineWriteCharToSession(int session_index, unsigned char ch) {
    if (session_index >= 0 && session_index < MAX_SESSIONS) {
        int saved = terminal.active_session;
        terminal.active_session = session_index;
        PipelineWriteChar(ch);
        terminal.active_session = saved;
    }
}


#endif // TERMINAL_IMPLEMENTATION


#endif // TERMINAL_H
