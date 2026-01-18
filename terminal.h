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
// Forward declaration
typedef struct Terminal_T Terminal;

// Response callback typedef
typedef void (*ResponseCallback)(Terminal* term, const char* response, int length); // For sending data back to host
typedef void (*PrinterCallback)(Terminal* term, const char* data, size_t length);   // For Printer Controller Mode
typedef void (*TitleCallback)(Terminal* term, const char* title, bool is_icon);    // For GUI window title changes
typedef void (*BellCallback)(Terminal* term);                                 // For audible bell
typedef void (*NotificationCallback)(Terminal* term, const char* message);          // For sending notifications (OSC 9)
typedef void (*GatewayCallback)(Terminal* term, const char* class_id, const char* id, const char* command, const char* params); // Gateway Protocol

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
//extern VTKeyboard vt_keyboard;
// extern Texture2D font_texture; // Moved to struct
// extern RGB_Color color_palette[256]; // Moved to struct
extern Color ansi_colors[16];        // Situation Color type for the 16 base ANSI colors
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
    bool bidi_mode;                 // BDSM (CSI ? 8246 h/l) - Bi-Directional Support Mode
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

// --- Shader Code ---
static const char* terminal_shader_body =
"\n"
"vec4 UnpackColor(uint c) {\n"
"    return vec4(float(c & 0xFF), float((c >> 8) & 0xFF), float((c >> 16) & 0xFF), float((c >> 24) & 0xFF)) / 255.0;\n"
"}\n"
"\n"
"void main() {\n"
"    // Bindless Accessors\n"
"    TerminalBuffer terminal_data = TerminalBuffer(pc.terminal_buffer_addr);\n"
"    sampler2D font_texture = sampler2D(pc.font_texture_handle);\n"
"    sampler2D sixel_texture = sampler2D(pc.sixel_texture_handle);\n"
"\n"
"    uvec2 pixel_coords = gl_GlobalInvocationID.xy;\n"
"    if (pixel_coords.x >= uint(pc.screen_size.x) || pixel_coords.y >= uint(pc.screen_size.y)) return;\n"
"\n"
"    vec2 uv_screen = vec2(pixel_coords) / pc.screen_size;\n"
"\n"
"    // CRT Curvature Effect\n"
"    if (pc.crt_curvature > 0.0) {\n"
"        vec2 d = abs(uv_screen - 0.5);\n"
"        d = pow(d, vec2(2.0));\n"
"        uv_screen -= 0.5;\n"
"        uv_screen *= 1.0 + dot(d, d) * pc.crt_curvature;\n"
"        uv_screen += 0.5;\n"
"        if (uv_screen.x < 0.0 || uv_screen.x > 1.0 || uv_screen.y < 0.0 || uv_screen.y > 1.0) {\n"
"            imageStore(output_image, ivec2(pixel_coords), vec4(0.0));\n"
"            return;\n"
"        }\n"
"    }\n"
"\n"
"    // Sixel Overlay Sampling (using possibly distorted UV)\n"
"    vec4 sixel_color = texture(sixel_texture, uv_screen);\n"
"\n"
"    // Re-calculate cell coordinates based on distorted UV or original pixel coords\n"
"    // If CRT is on, we should sample based on distorted UV to map screen to terminal grid\n"
"    uvec2 sample_coords = uvec2(uv_screen * pc.screen_size);\n"
"    \n"
"    uint cell_x = sample_coords.x / uint(pc.char_size.x);\n"
"    uint cell_y = sample_coords.y / uint(pc.char_size.y);\n"
"    uint row_start = cell_y * uint(pc.grid_size.x);\n"
"\n"
"    if (row_start >= terminal_data.cells.length()) return;\n"
"\n"
"    // Check line attributes from the first cell of the row\n"
"    uint line_flags = terminal_data.cells[row_start].flags;\n"
"    bool is_dw = (line_flags & (1 << 7)) != 0;\n"
"    bool is_dh_top = (line_flags & (1 << 8)) != 0;\n"
"    bool is_dh_bot = (line_flags & (1 << 9)) != 0;\n"
"\n"
"    uint eff_cell_x = cell_x;\n"
"    uint in_char_x = sample_coords.x % uint(pc.char_size.x);\n"
"    if (is_dw) {\n"
"        eff_cell_x = cell_x / 2;\n"
"        in_char_x = (sample_coords.x % (uint(pc.char_size.x) * 2)) / 2;\n"
"    }\n"
"\n"
"    uint cell_index = row_start + eff_cell_x;\n"
"    if (cell_index >= terminal_data.cells.length()) return;\n"
"\n"
"    GPUCell cell = terminal_data.cells[cell_index];\n"
"    vec4 fg = UnpackColor(cell.fg_color);\n"
"    vec4 bg = UnpackColor(cell.bg_color);\n"
"    uint flags = cell.flags;\n"
"\n"
"    if ((flags & (1 << 5)) != 0) { vec4 t=fg; fg=bg; bg=t; }\n"
"\n"
"    // Mouse Selection Highlight\n"
"    if (pc.sel_active != 0) {\n"
"        uint s = min(pc.sel_start, pc.sel_end);\n"
"        uint e = max(pc.sel_start, pc.sel_end);\n"
"        if (cell_index >= s && cell_index <= e) {\n"
"             // Invert colors for selection\n"
"             fg = vec4(1.0) - fg;\n"
"             bg = vec4(1.0) - bg;\n"
"             fg.a = 1.0; bg.a = 1.0;\n"
"        }\n"
"    }\n"
"\n"
"    if (cell_index == pc.cursor_index && pc.cursor_blink_state != 0) {\n"
"        vec4 t=fg; fg=bg; bg=t;\n"
"    }\n"
"\n"
"    if (cell_index == pc.mouse_cursor_index) {\n"
"        if (in_char_x == 0 || in_char_x == uint(pc.char_size.x) - 1 || \n"
"            (sample_coords.y % uint(pc.char_size.y)) == 0 || \n"
"            (sample_coords.y % uint(pc.char_size.y)) == uint(pc.char_size.y) - 1) {\n"
"             vec4 t=fg; fg=bg; bg=t;\n"
"        }\n"
"    }\n"
"\n"
"    uint char_code = cell.char_code;\n"
"    uint glyph_col = char_code % pc.atlas_cols;\n"
"    uint glyph_row = char_code / pc.atlas_cols;\n"
"    \n"
"    uint in_char_y = sample_coords.y % uint(pc.char_size.y);\n"
"    float u_pixel = float(in_char_x);\n"
"    float v_pixel = float(in_char_y);\n"
"    \n"
"    if (is_dh_top || is_dh_bot) {\n"
"        v_pixel = (v_pixel * 0.5) + (is_dh_bot ? (pc.char_size.y * 0.5) : 0.0);\n"
"    }\n"
"\n"
"    ivec2 tex_size = textureSize(font_texture, 0);\n"
"    vec2 uv = vec2(float(glyph_col * pc.char_size.x + u_pixel) / float(tex_size.x),\n"
"                   float(glyph_row * pc.char_size.y + v_pixel) / float(tex_size.y));\n"
"\n"
"    float font_val = texture(font_texture, uv).r;\n"
"\n"
"    // Underline\n"
"    if ((flags & (1 << 3)) != 0 && in_char_y == uint(pc.char_size.y) - 1) font_val = 1.0;\n"
"    // Strike\n"
"    if ((flags & (1 << 6)) != 0 && in_char_y == uint(pc.char_size.y) / 2) font_val = 1.0;\n"
"\n"
"    vec4 pixel_color = mix(bg, fg, font_val);\n"
"\n"
"    if ((flags & (1 << 4)) != 0 && pc.text_blink_state == 0) {\n"
"       pixel_color = bg;\n"
"    }\n"
"\n"
"    if ((flags & (1 << 10)) != 0) {\n"
"       pixel_color = bg;\n"
"    }\n"
"\n"
"    // Sixel Blend\n"
"    pixel_color = mix(pixel_color, sixel_color, sixel_color.a);\n"
"\n"
"    // Vector Graphics Overlay (Storage Tube Glow)\n"
"    if (pc.vector_texture_handle != 0) {\n"
"        sampler2D vector_tex = sampler2D(pc.vector_texture_handle);\n"
"        vec4 vec_col = texture(vector_tex, uv_screen);\n"
"        // Additive blending for CRT glow effect\n"
"        pixel_color += vec_col;\n"
"    }\n"
"\n"
"    // Scanlines & Vignette (Retro Effects)\n"
"    if (pc.scanline_intensity > 0.0) {\n"
"        float scanline = sin(uv_screen.y * pc.screen_size.y * 3.14159);\n"
"        pixel_color.rgb *= (1.0 - pc.scanline_intensity) + pc.scanline_intensity * (0.5 + 0.5 * scanline);\n"
"    }\n"
"    if (pc.crt_curvature > 0.0) {\n"
"        vec2 d = abs(uv_screen - 0.5) * 2.0;\n"
"        d = pow(d, vec2(2.0));\n"
"        float vig = 1.0 - dot(d, d) * 0.1;\n"
"                pixel_color.rgb *= vig;\n"
"    }\n"
"\n"
    "    // Visual Bell Flash\n"
    "    if (pc.visual_bell_intensity > 0.0) {\n"
    "        pixel_color = mix(pixel_color, vec4(1.0), pc.visual_bell_intensity);\n"
    "    }\n"
    "\n"
    "    imageStore(output_image, ivec2(pixel_coords), pixel_color);\n"
    "}\n";

static const char* vector_shader_body =
"\n"
"vec4 UnpackColor(uint c) {\n"
"    return vec4(float(c & 0xFF), float((c >> 8) & 0xFF), float((c >> 16) & 0xFF), float((c >> 24) & 0xFF)) / 255.0;\n"
"}\n"
"\n"
"void main() {\n"
"    uint idx = gl_GlobalInvocationID.x;\n"
"    if (idx >= pc.vector_count) return;\n"
"\n"
"    // Bindless Buffer Access\n"
"    VectorBuffer lines = VectorBuffer(pc.vector_buffer_addr);\n"
"\n"
"    GPUVectorLine line = lines.data[idx];\n"
"    vec2 p0 = line.start * pc.screen_size;\n"
"    vec2 p1 = line.end * pc.screen_size;\n"
"    vec4 color = UnpackColor(line.color);\n"
"    color.a *= line.intensity;\n"
"\n"
"    int x0 = int(p0.x); int y0 = int(p0.y);\n"
"    int x1 = int(p1.x); int y1 = int(p1.y);\n"
"    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;\n"
"    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;\n"
"    int err = dx + dy, e2;\n"
"\n"
"    // Bresenham Loop\n"
"    for (;;) {\n"
"        if (x0 >= 0 && x0 < int(pc.screen_size.x) && y0 >= 0 && y0 < int(pc.screen_size.y)) {\n"
"            vec4 bg = imageLoad(output_image, ivec2(x0, y0));\n"
"            vec4 result = bg;\n"
"            if (line.mode == 0) {\n"
"                 // Additive 'Glow' Blending\n"
"                 result = bg + (color * color.a);\n"
"            } else if (line.mode == 1) {\n"
"                 // Replace\n"
"                 result = vec4(color.rgb, 1.0);\n"
"            } else if (line.mode == 2) {\n"
"                 // Erase (Draw Black)\n"
"                 result = vec4(0.0, 0.0, 0.0, 0.0);\n"
"            } else if (line.mode == 3) {\n"
"                 // XOR / Complement (Invert)\n"
"                 result = vec4(1.0 - bg.rgb, 1.0);\n"
"            }\n"
"            imageStore(output_image, ivec2(x0, y0), result);\n"
"        }\n"
"        if (x0 == x1 && y0 == y1) break;\n"
"        e2 = 2 * err;\n"
"        if (e2 >= dy) { err += dy; x0 += sx; }\n"
"        if (e2 <= dx) { err += dx; y0 += sy; }\n"
"    }\n"
"}\n";

static const char* sixel_shader_body =
"\n"
"vec4 UnpackColor(uint c) {\n"
"    return vec4(float(c & 0xFF), float((c >> 8) & 0xFF), float((c >> 16) & 0xFF), float((c >> 24) & 0xFF)) / 255.0;\n"
"}\n"
"\n"
"void main() {\n"
"    uint idx = gl_GlobalInvocationID.x;\n"
"    if (idx >= pc.vector_count) return;\n"
"\n"
"    // Bindless Buffer Access\n"
"    SixelBuffer strips = SixelBuffer(pc.vector_buffer_addr);\n"
"    PaletteBuffer palette = PaletteBuffer(pc.terminal_buffer_addr);\n"
"\n"
"    GPUSixelStrip strip = strips.data[idx];\n"
"    uint color_val = palette.colors[strip.color_index];\n"
"    vec4 color = UnpackColor(color_val);\n"
"\n"
"    // Write 6 pixels\n"
"    for (int i = 0; i < 6; i++) {\n"
"        if ((strip.pattern & (1 << i)) != 0) {\n"
"            int x = int(strip.x);\n"
    "            int y = int(strip.y) + i - pc.sixel_y_offset;\n"
    "            if (x < int(pc.screen_size.x) && y >= 0 && y < int(pc.screen_size.y)) {\n"
    "                imageStore(output_image, ivec2(x, y), color);\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "}\n";


#if defined(SITUATION_USE_VULKAN)
    // --- VULKAN DEFINITIONS ---
    static const char* terminal_compute_preamble =
    "#version 460\n"
    "#define VULKAN_BACKEND\n"
    "#extension GL_EXT_buffer_reference : require\n"
    "#extension GL_EXT_scalar_block_layout : require\n"
    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require\n"
    "#extension GL_ARB_bindless_texture : require\n"
    "layout(local_size_x = 8, local_size_y = 16, local_size_z = 1) in;\n"
    "struct GPUCell { uint char_code; uint fg_color; uint bg_color; uint flags; };\n"
    "layout(buffer_reference, scalar) buffer TerminalBuffer { GPUCell cells[]; };\n"
    "layout(set = 1, binding = 0, rgba8) writeonly uniform image2D output_image;\n"
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
    "    float visual_bell_intensity;\n"
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
    "struct GPUCell { uint char_code; uint fg_color; uint bg_color; uint flags; };\n"
    "layout(buffer_reference, scalar) buffer TerminalBuffer { GPUCell cells[]; };\n"
    "layout(binding = 1, rgba8) writeonly uniform image2D output_image;\n"
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
    "} pc;\n";
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
    int sixel_y_offset; // For scrolling Sixel images
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

    int cols; // Terminal width in columns
    int rows; // Terminal height in rows (viewport)

    bool* row_dirty; // Tracks dirty state of the VIEWPORT rows (0..rows-1)
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
    unsigned char input_pipeline[65536]; // Buffer for incoming data from host (Increased to 64KB)
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

    void* user_data; // User data for callbacks and application state

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

    return &session->screen_buffer[actual_index * session->cols];
}

static inline EnhancedTermChar* GetScreenCell(TerminalSession* session, int y, int x) {
    if (x < 0 || x >= session->cols) return NULL; // Basic safety
    return &GetScreenRow(session, y)[x];
}

static inline EnhancedTermChar* GetActiveScreenRow(TerminalSession* session, int row) {
    // Access logical row 'row' relative to the ACTIVE screen top (ignoring view_offset).
    // This is used for emulation commands that modify the screen state (insert, delete, scroll).

    int logical_row_idx = session->screen_head + row; // No view_offset subtraction

    // Handle wrap-around
    int actual_index = logical_row_idx % session->buffer_height;
    if (actual_index < 0) actual_index += session->buffer_height;

    return &session->screen_buffer[actual_index * session->cols];
}

static inline EnhancedTermChar* GetActiveScreenCell(TerminalSession* session, int y, int x) {
    if (x < 0 || x >= session->cols) return NULL;
    return &GetActiveScreenRow(session, y)[x];
}

typedef struct Terminal_T {
    TerminalSession sessions[MAX_SESSIONS];
    int width; // Global Width (Columns)
    int height; // Global Height (Rows)
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
        int extra_byte; // Buffered Extra Byte for 12-bit coordinates
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
    uint16_t* glyph_map; // Map Unicode Codepoint to Atlas Index
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
    GatewayCallback gateway_callback; // Callback for Gateway Protocol
    TitleCallback title_callback;
    BellCallback bell_callback;

    RGB_Color color_palette[256];
    uint32_t charset_lut[32][128];
} Terminal;

// =============================================================================
// CORE API FUNCTIONS
// =============================================================================

typedef struct {
    int width;
    int height;
    ResponseCallback response_callback;
} TerminalConfig;

Terminal* Terminal_Create(TerminalConfig config);
void Terminal_Destroy(Terminal* term);

// Session Management
void SetActiveSession(Terminal* term, int index);
void SetSplitScreen(Terminal* term, bool active, int row, int top_idx, int bot_idx);
void PipelineWriteCharToSession(Terminal* term, int session_index, unsigned char ch);
void InitSession(Terminal* term, int index);

// Terminal lifecycle
void InitTerminal(Terminal* term);
void CleanupTerminal(Terminal* term);
void UpdateTerminal(Terminal* term);  // Process events, update states (e.g., cursor blink)
void DrawTerminal(Terminal* term);    // Render the terminal state to screen

// VT compliance and identification
bool GetVTKeyEvent(Terminal* term, VTKeyEvent* event); // Retrieve a processed key event
void SetVTLevel(Terminal* term, VTLevel level);
VTLevel GetVTLevel(Terminal* term);
// void EnableVTFeature(const char* feature, bool enable); // e.g., "sixel", "DECCKM" - Deprecated by SetVTLevel
// bool IsVTFeatureSupported(const char* feature); - Deprecated by direct struct access
void GetDeviceAttributes(Terminal* term, char* primary, char* secondary, size_t buffer_size);

// Enhanced pipeline management (for host input)
bool PipelineWriteChar(Terminal* term, unsigned char ch);
bool PipelineWriteString(Terminal* term, const char* str);
bool PipelineWriteFormat(Terminal* term, const char* format, ...);
// bool PipelineWriteUTF8(const char* utf8_str); // Requires UTF-8 decoding logic
void ProcessPipeline(Terminal* term); // Process characters from the pipeline
void ClearPipeline(Terminal* term);
int GetPipelineCount(Terminal* term);
bool IsPipelineOverflow(Terminal* term);

// Performance management
void SetPipelineTargetFPS(Terminal* term, int fps);    // Helps tune processing budget
void SetPipelineTimeBudget(Terminal* term, double pct); // Percentage of frame time for pipeline

// Mouse support (enhanced)
void SetMouseTracking(Terminal* term, MouseTrackingMode mode); // Explicitly set a mouse mode
void EnableMouseFeature(Terminal* term, const char* feature, bool enable); // e.g., "focus", "sgr"
void UpdateMouse(Terminal* term); // Process mouse input from Situation and generate VT sequences

// Keyboard support (VT compatible)
void UpdateVTKeyboard(Terminal* term); // Process keyboard input from Situation
void UpdateKeyboard(Terminal* term);  // Alias for compatibility
bool GetKeyEvent(Terminal* term, KeyEvent* event);  // Alias for compatibility
void SetKeyboardMode(Terminal* term, const char* mode, bool enable); // "application_cursor", "keypad_numeric"
void DefineFunctionKey(Terminal* term, int key_num, const char* sequence); // Program F1-F24

// Terminal control and modes
void SetTerminalMode(Terminal* term, const char* mode, bool enable); // Generic mode setting by name
void SetCursorShape(Terminal* term, CursorShape shape);
void SetCursorColor(Terminal* term, ExtendedColor color);

// Character sets and encoding
void SelectCharacterSet(Terminal* term, int gset, CharacterSet charset); // Designate G0-G3
void SetCharacterSet(Terminal* term, CharacterSet charset); // Set current GL (usually G0)
unsigned int TranslateCharacter(Terminal* term, unsigned char ch, CharsetState* state); // Translate based on active CS

// Tab stops
void SetTabStop(Terminal* term, int column);
void ClearTabStop(Terminal* term, int column);
void ClearAllTabStops(Terminal* term);
int NextTabStop(Terminal* term, int current_column);
int PreviousTabStop(Terminal* term, int current_column); // Added for CBT

// Bracketed paste
void EnableBracketedPaste(Terminal* term, bool enable); // Enable/disable CSI ? 2004 h/l
bool IsBracketedPasteActive(Terminal* term);
void ProcessPasteData(Terminal* term, const char* data, size_t length); // Handle pasted data

// Rectangular operations (VT420+)
void DefineRectangle(Terminal* term, int top, int left, int bottom, int right); // (DECSERA, DECFRA, DECCRA)
void ExecuteRectangularOperation(Terminal* term, RectOperation op, const EnhancedTermChar* fill_char);
void CopyRectangle(Terminal* term, VTRectangle src, int dest_x, int dest_y);
void ExecuteRectangularOps(Terminal* term);  // DECCRA Implementation
void ExecuteRectangularOps2(Terminal* term); // DECRQCRA Implementation

// Sixel graphics
void InitSixelGraphics(Terminal* term);
void ProcessSixelData(Terminal* term, const char* data, size_t length); // Process raw Sixel string
void DrawSixelGraphics(Terminal* term); // Render current Sixel image

// Soft fonts
void LoadSoftFont(Terminal* term, const unsigned char* font_data, int char_start, int char_count); // DECDLD
void SelectSoftFont(Terminal* term, bool enable); // Enable/disable use of loaded soft font

// Title management
void VTSituationSetWindowTitle(Terminal* term, const char* title); // Set window title (OSC 0, OSC 2)
void SetIconTitle(Terminal* term, const char* title);   // Set icon title (OSC 1)
const char* GetWindowTitle(Terminal* term);
const char* GetIconTitle(Terminal* term);

// Callbacks
void SetResponseCallback(Terminal* term, ResponseCallback callback);
void SetPrinterCallback(Terminal* term, PrinterCallback callback);
void SetTitleCallback(Terminal* term, TitleCallback callback);
void SetBellCallback(Terminal* term, BellCallback callback);
void SetNotificationCallback(Terminal* term, NotificationCallback callback);
void SetGatewayCallback(Terminal* term, GatewayCallback callback);

// Testing and diagnostics
void RunVTTest(Terminal* term, const char* test_name); // Run predefined test sequences
void ShowTerminalInfo(Terminal* term);           // Display current terminal state/info
void EnableDebugMode(Terminal* term, bool enable);     // Toggle verbose debug logging
void LogUnsupportedSequence(Terminal* term, const char* sequence); // Log an unsupported sequence
TerminalStatus GetTerminalStatus(Terminal* term);
void ShowBufferDiagnostics(Terminal* term);      // Display buffer usage info

// Screen buffer management
void VTSwapScreenBuffer(Terminal* term); // Handles 1047/1049 logic

void LoadTerminalFont(Terminal* term, const char* filepath);

// Helper to allocate a glyph index in the dynamic atlas for any Unicode codepoint
uint32_t AllocateGlyph(Terminal* term, uint32_t codepoint);

// Resize the terminal grid and window texture
void ResizeTerminal(Terminal* term, int cols, int rows);

// Internal rendering/parsing functions (potentially exposed for advanced use or testing)
void CreateFontTexture(Terminal* term);

// Internal helper forward declaration
void InitTerminalCompute(Terminal* term);

// Low-level char processing (called by ProcessPipeline via ProcessChar)
void ProcessChar(Terminal* term, unsigned char ch); // Main dispatcher for character processing
void ProcessPrinterControllerChar(Terminal* term, unsigned char ch); // Handle Printer Controller Mode
void ProcessNormalChar(Terminal* term, unsigned char ch);
void ProcessEscapeChar(Terminal* term, unsigned char ch);
void ProcessCSIChar(Terminal* term, unsigned char ch);
void ProcessOSCChar(Terminal* term, unsigned char ch);
void ProcessDCSChar(Terminal* term, unsigned char ch);
void ProcessAPCChar(Terminal* term, unsigned char ch);
void ProcessPMChar(Terminal* term, unsigned char ch);
void ProcessSOSChar(Terminal* term, unsigned char ch);
void ProcessVT52Char(Terminal* term, unsigned char ch);
void ProcessSixelChar(Terminal* term, unsigned char ch);
void ProcessSixelSTChar(Terminal* term, unsigned char ch);
void ProcessControlChar(Terminal* term, unsigned char ch);
//void ProcessStringTerminator(Terminal* term, unsigned char ch);
void ProcessCharsetCommand(Terminal* term, unsigned char ch);
void ProcessHashChar(Terminal* term, unsigned char ch);
void ProcessPercentChar(Terminal* term, unsigned char ch);


// Screen manipulation internals
void ScrollUpRegion(Terminal* term, int top, int bottom, int lines);
void InsertLinesAt(Terminal* term, int row, int count); // Added IL
void DeleteLinesAt(Terminal* term, int row, int count); // Added DL
void InsertCharactersAt(Terminal* term, int row, int col, int count); // Added ICH
void DeleteCharactersAt(Terminal* term, int row, int col, int count); // Added DCH
void InsertCharacterAtCursor(Terminal* term, unsigned int ch); // Handles character placement and insert mode
void ScrollDownRegion(Terminal* term, int top, int bottom, int lines);

void ExecuteSaveCursor(Terminal* term);
void ExecuteRestoreCursor(Terminal* term);

// Response and parsing helpers
void QueueResponse(Terminal* term, const char* response); // Add string to answerback_buffer
void QueueResponseBytes(Terminal* term, const char* data, size_t len);
static void ParseGatewayCommand(Terminal* term, const char* data, size_t len); // Gateway Protocol Parser
int ParseCSIParams(Terminal* term, const char* params, int* out_params, int max_params); // Parses CSI parameter string into escape_params
int GetCSIParam(Terminal* term, int index, int default_value); // Gets a parsed CSI parameter
void ExecuteCSICommand(Terminal* term, unsigned char command);
void ExecuteOSCCommand(Terminal* term);
void ExecuteDCSCommand(Terminal* term);
void ExecuteAPCCommand(Terminal* term);
void ExecutePMCommand(Terminal* term);
void ExecuteSOSCommand(Terminal* term);
void ExecuteDCSAnswerback(Terminal* term);

// Cell and attribute helpers
void ClearCell(Terminal* term, EnhancedTermChar* cell); // Clears a cell with current attributes
void ResetAllAttributes(Terminal* term);          // Resets current text attributes to default

// Character set translation helpers
unsigned int TranslateDECSpecial(Terminal* term, unsigned char ch);
unsigned int TranslateDECMultinational(Terminal* term, unsigned char ch);

// Keyboard sequence generation helpers
void GenerateVTSequence(Terminal* term, VTKeyEvent* event);
void HandleControlKey(Terminal* term, VTKeyEvent* event);
void HandleAltKey(Terminal* term, VTKeyEvent* event);

// Scripting API functions
void Script_PutChar(Terminal* term, unsigned char ch);
void Script_Print(Terminal* term, const char* text);
void Script_Printf(Terminal* term, const char* format, ...);
void Script_Cls(Terminal* term);
void Script_SetColor(Terminal* term, int fg, int bg);

#ifdef TERMINAL_IMPLEMENTATION
#define GET_SESSION(term) (&(term)->sessions[(term)->active_session])


// =============================================================================
// IMPLEMENTATION BEGINS HERE
// =============================================================================

// Fixed global variable definitions
//VTKeyboard vt_keyboard = {0};   // deprecated
// RGLTexture font_texture = {0};  // Moved to terminal struct
// Callbacks moved to struct

// Color mappings - Fixed initialization
// RGB_Color color_palette[256]; // Moved to struct

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
void InitFontData(Terminal* term); // In case it's used elsewhere, though font_data is static

#include "font_data.h"


// Extended font data with larger character matrix for better rendering
/*unsigned char cp437_font__8x16[256 * 16 * 2] = {
    // 0x00 - NULL (empty)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};*/

// static uint32_t charset_lut[32][128]; // Moved to struct

void InitCharacterSetLUT(Terminal* term) {
    // 1. Initialize all to ASCII identity first
    for (int s = 0; s < 32; s++) {
        for (int c = 0; c < 128; c++) {
            term->charset_lut[s][c] = c;
        }
    }

    // 2. DEC Special Graphics
    for (int c = 0; c < 128; c++) {
        term->charset_lut[CHARSET_DEC_SPECIAL][c] = TranslateDECSpecial(term, c);
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

void InitFontData(Terminal* term) {
    // This function is currently empty.
    // The font_data array is initialized statically.
    // If font_data needed dynamic initialization or loading from a file,
    // it would happen here.
}

// =============================================================================
// REST OF THE IMPLEMENTATION
// =============================================================================

void InitColorPalette(Terminal* term) {
    for (int i = 0; i < 16; i++) {
        term->color_palette[i] = (RGB_Color){ ansi_colors[i].r, ansi_colors[i].g, ansi_colors[i].b, 255 };
    }
    int index = 16;
    for (int r = 0; r < 6; r++) {
        for (int g = 0; g < 6; g++) {
            for (int b = 0; b < 6; b++) {
                term->color_palette[index++] = (RGB_Color){
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
        term->color_palette[232 + i] = (RGB_Color){gray, gray, gray, 255};
    }
}

void InitVTConformance(Terminal* term) {
    GET_SESSION(term)->conformance.level = VT_LEVEL_XTERM;
    GET_SESSION(term)->conformance.strict_mode = false;
    SetVTLevel(term, GET_SESSION(term)->conformance.level);
    GET_SESSION(term)->conformance.compliance.unsupported_sequences = 0;
    GET_SESSION(term)->conformance.compliance.partial_implementations = 0;
    GET_SESSION(term)->conformance.compliance.extensions_used = 0;
    GET_SESSION(term)->conformance.compliance.last_unsupported[0] = '\0';
}

void InitTabStops(Terminal* term) {
    memset(GET_SESSION(term)->tab_stops.stops, false, sizeof(GET_SESSION(term)->tab_stops.stops));
    GET_SESSION(term)->tab_stops.count = 0;
    GET_SESSION(term)->tab_stops.default_width = 8;
    for (int i = GET_SESSION(term)->tab_stops.default_width; i < MAX_TAB_STOPS && i < DEFAULT_TERM_WIDTH; i += GET_SESSION(term)->tab_stops.default_width) {
        GET_SESSION(term)->tab_stops.stops[i] = true;
        GET_SESSION(term)->tab_stops.count++;
    }
}

void InitCharacterSets(Terminal* term) {
    GET_SESSION(term)->charset.g0 = CHARSET_ASCII;
    GET_SESSION(term)->charset.g1 = CHARSET_DEC_SPECIAL;
    GET_SESSION(term)->charset.g2 = CHARSET_ASCII;
    GET_SESSION(term)->charset.g3 = CHARSET_ASCII;
    GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g0;
    GET_SESSION(term)->charset.gr = &GET_SESSION(term)->charset.g1;
    GET_SESSION(term)->charset.single_shift_2 = false;
    GET_SESSION(term)->charset.single_shift_3 = false;
}

// Initialize VT keyboard state
// Sets up keyboard modes, function key mappings, and event buffer
void InitVTKeyboard(Terminal* term) {
    // Initialize keyboard modes and flags
    GET_SESSION(term)->vt_keyboard.application_mode = false; // Application mode off
    GET_SESSION(term)->vt_keyboard.cursor_key_mode = GET_SESSION(term)->dec_modes.application_cursor_keys; // Sync with DECCKM
    GET_SESSION(term)->vt_keyboard.keypad_mode = false; // Numeric keypad mode
    GET_SESSION(term)->vt_keyboard.meta_sends_escape = true; // Alt sends ESC prefix
    GET_SESSION(term)->vt_keyboard.delete_sends_del = true; // Delete sends DEL (\x7F)
    GET_SESSION(term)->vt_keyboard.backarrow_sends_bs = true; // Backspace sends BS (\x08)
    GET_SESSION(term)->vt_keyboard.keyboard_dialect = 1; // North American ASCII

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
        strncpy(GET_SESSION(term)->vt_keyboard.function_keys[i], function_key_sequences[i], 31);
        GET_SESSION(term)->vt_keyboard.function_keys[i][31] = '\0';
    }

    // Initialize event buffer
    GET_SESSION(term)->vt_keyboard.buffer_head = 0;
    GET_SESSION(term)->vt_keyboard.buffer_tail = 0;
    GET_SESSION(term)->vt_keyboard.buffer_count = 0;
    GET_SESSION(term)->vt_keyboard.total_events = 0;
    GET_SESSION(term)->vt_keyboard.dropped_events = 0;
}

Terminal* Terminal_Create(TerminalConfig config) {
    Terminal* term = (Terminal*)calloc(1, sizeof(Terminal));
    if (!term) return NULL;

    // Apply config
    if (config.width > 0) term->width = config.width;
    else term->width = DEFAULT_TERM_WIDTH;

    if (config.height > 0) term->height = config.height;
    else term->height = DEFAULT_TERM_HEIGHT;

    term->response_callback = config.response_callback;

    InitTerminal(term);
    return term;
}

void Terminal_Destroy(Terminal* term) {
    if (!term) return;
    CleanupTerminal(term);
    free(term);
}

void InitTerminal(Terminal* term) {
    InitFontData(term);
    InitColorPalette(term);

    // Init global members
    if (term->width == 0) term->width = DEFAULT_TERM_WIDTH;
    if (term->height == 0) term->height = DEFAULT_TERM_HEIGHT;
    term->active_session = 0;
    term->pending_session_switch = -1;
    term->split_screen_active = false;
    term->split_row = term->height / 2;
    term->session_top = 0;
    term->session_bottom = 1;
    term->visual_effects.curvature = 0.0f;
    term->visual_effects.scanline_intensity = 0.0f;
    term->tektronix.extra_byte = -1;

    // Init sessions
    for (int i = 0; i < MAX_SESSIONS; i++) {
        InitSession(term, i);

        // Context switch to use existing helper functions
        int saved = term->active_session;
        term->active_session = i;

        InitVTConformance(term);
        InitTabStops(term);
        InitCharacterSets(term);
        InitVTKeyboard(term);
        InitSixelGraphics(term);

        term->active_session = saved;
    }
    term->active_session = 0;

    InitCharacterSetLUT(term);

    // Allocate full Unicode map
    if (term->glyph_map) free(term->glyph_map);
    term->glyph_map = (uint16_t*)calloc(0x110000, sizeof(uint16_t));

    // Initialize Dynamic Atlas dimensions before creation
    term->atlas_width = 1024;
    term->atlas_height = 1024;
    term->atlas_cols = 128;

    // Allocate LRU Cache
    size_t capacity = (term->atlas_width / DEFAULT_CHAR_WIDTH) * (term->atlas_height / DEFAULT_CHAR_HEIGHT);
    term->glyph_last_used = (uint64_t*)calloc(capacity, sizeof(uint64_t));
    term->atlas_to_codepoint = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    term->frame_count = 0;

    CreateFontTexture(term);
    InitTerminalCompute(term);
}



// String terminator handler for ESC P, ESC _, ESC ^, ESC X
void ProcessStringTerminator(Terminal* term, unsigned char ch) {
    // Expects ST (ESC \) to terminate.
    // Current char `ch` is the char after ESC. So we need to see `\`
    if (ch == '\\') { // ESC \ (ST - String Terminator)
        // Execute the command that was buffered
        switch(GET_SESSION(term)->parse_state) { // The state *before* it became PARSE_STRING_TERMINATOR
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
            // So, if ch == '\', the DCS string is terminated. We should call ExecuteDCSCommand(term).
            // This implies this function needs to know *which* string type was being parsed.
            // A temporary variable holding the "parent_parse_state" would be better.
            // For now, let's assume the specific handlers (ProcessOSCChar, ProcessDCSChar) already called their Execute function
            // upon detecting the ST sequence starting (ESC then \).
            // This function just resets state.
        }
        GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
        GET_SESSION(term)->escape_pos = 0; // Clear buffer after command execution
    } else {
        // Not a valid ST, could be another ESC sequence.
        // Re-process 'ch' as start of new escape sequence.
        GET_SESSION(term)->parse_state = VT_PARSE_ESCAPE; // Go to escape state
        ProcessEscapeChar(term, ch); // Process the char that broke ST
    }
}

void ProcessCharsetCommand(Terminal* term, unsigned char ch) {
    GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos++] = ch;

    if (GET_SESSION(term)->escape_pos >= 2) {
        char designator = GET_SESSION(term)->escape_buffer[0];
        char charset_char = GET_SESSION(term)->escape_buffer[1];
        CharacterSet selected_cs = CHARSET_ASCII;

        switch (charset_char) {
            case 'A': selected_cs = CHARSET_UK; break;
            case 'B': selected_cs = CHARSET_ASCII; break;
            case '0': selected_cs = CHARSET_DEC_SPECIAL; break;
            case '1':
            case '2':
                if (GET_SESSION(term)->options.debug_sequences) {
                    LogUnsupportedSequence(term, "DEC Alternate Character ROM not fully supported, using ASCII/DEC Special");
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
                if (GET_SESSION(term)->options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown charset char: %c for designator %c", charset_char, designator);
                    LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }

        switch (designator) {
            case '(': GET_SESSION(term)->charset.g0 = selected_cs; break;
            case ')': GET_SESSION(term)->charset.g1 = selected_cs; break;
            case '*': GET_SESSION(term)->charset.g2 = selected_cs; break;
            case '+': GET_SESSION(term)->charset.g3 = selected_cs; break;
        }

        GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
        GET_SESSION(term)->escape_pos = 0;
    }
}

// Stubs for APC/PM/SOS command execution
void ExecuteAPCCommand(Terminal* term) {
    if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "APC sequence executed (no-op)");
    // GET_SESSION(term)->escape_buffer contains the APC string data.
}
void ExecutePMCommand(Terminal* term) {
    if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "PM sequence executed (no-op)");
    // GET_SESSION(term)->escape_buffer contains the PM string data.
}
void ExecuteSOSCommand(Terminal* term) {
    if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "SOS sequence executed (no-op)");
    // GET_SESSION(term)->escape_buffer contains the SOS string data.
}

// Generic string processor for APC, PM, SOS
void ProcessGenericStringChar(Terminal* term, unsigned char ch, VTParseState next_state_on_escape, void (*execute_command_func)()) {
    if (GET_SESSION(term)->escape_pos < sizeof(GET_SESSION(term)->escape_buffer) - 1) {
        GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos++] = ch;

        if (ch == '\\' && GET_SESSION(term)->escape_pos >= 2 && GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 2] == '\x1B') { // ST (ESC \)
            GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 2] = '\0'; // Null-terminate before ESC of ST
            if (execute_command_func) execute_command_func();
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            GET_SESSION(term)->escape_pos = 0;
        }
        // BEL is not a standard terminator for these, ST is.
    } else { // Buffer overflow
        GET_SESSION(term)->escape_buffer[sizeof(GET_SESSION(term)->escape_buffer) - 1] = '\0';
        if (execute_command_func) execute_command_func();
        GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
        GET_SESSION(term)->escape_pos = 0;
        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "String sequence (type %d) too long, truncated", (int)GET_SESSION(term)->parse_state); // Log current state
        LogUnsupportedSequence(term, log_msg);
    }
}

void ProcessAPCChar(Terminal* term, unsigned char ch) { ProcessGenericStringChar(term, ch, VT_PARSE_ESCAPE /* Fallback if ST is broken */, ExecuteAPCCommand); }
void ProcessPMChar(Terminal* term, unsigned char ch) { ProcessGenericStringChar(term, ch, VT_PARSE_ESCAPE, ExecutePMCommand); }
void ProcessSOSChar(Terminal* term, unsigned char ch) { ProcessGenericStringChar(term, ch, VT_PARSE_ESCAPE, ExecuteSOSCommand); }

// Internal helper forward declaration
static void ProcessTektronixChar(Terminal* term, unsigned char ch);
static void ProcessReGISChar(Terminal* term, unsigned char ch);

// Process character when in Printer Controller Mode (pass-through)
void ProcessPrinterControllerChar(Terminal* term, unsigned char ch) {
    // We must detect the exit sequence: CSI 4 i
    // CSI can be 7-bit (\x1B [) or 8-bit (\x9B)
    // Exit sequence:
    // 7-bit: ESC [ 4 i
    // 8-bit: CSI 4 i (\x9B 4 i)

    // Ensure session-specific buffering
    // Add to buffer
    if (GET_SESSION(term)->printer_buf_len < 7) {
        GET_SESSION(term)->printer_buffer[GET_SESSION(term)->printer_buf_len++] = ch;
    } else {
        // Buffer full, flush oldest and shift
        if (term->printer_callback) {
            term->printer_callback(term, (const char*)&GET_SESSION(term)->printer_buffer[0], 1);
        }
        memmove(GET_SESSION(term)->printer_buffer, GET_SESSION(term)->printer_buffer + 1, --GET_SESSION(term)->printer_buf_len);
        GET_SESSION(term)->printer_buffer[GET_SESSION(term)->printer_buf_len++] = ch;
    }
    GET_SESSION(term)->printer_buffer[GET_SESSION(term)->printer_buf_len] = '\0';

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

    while (GET_SESSION(term)->printer_buf_len > 0) {
        // Check 7-bit
        bool match1 = true;
        for (int i = 0; i < GET_SESSION(term)->printer_buf_len; i++) {
            if (i >= 4 || GET_SESSION(term)->printer_buffer[i] != (unsigned char)seq1[i]) {
                match1 = false;
                break;
            }
        }
        if (match1 && GET_SESSION(term)->printer_buf_len == 4) {
            GET_SESSION(term)->printer_controller_enabled = false;
            GET_SESSION(term)->printer_buf_len = 0;
            return;
        }

        // Check 8-bit
        bool match2 = true;
        for (int i = 0; i < GET_SESSION(term)->printer_buf_len; i++) {
            if (i >= 3 || GET_SESSION(term)->printer_buffer[i] != (unsigned char)seq2[i]) {
                match2 = false;
                break;
            }
        }
        if (match2 && GET_SESSION(term)->printer_buf_len == 3) {
            GET_SESSION(term)->printer_controller_enabled = false;
            GET_SESSION(term)->printer_buf_len = 0;
            return;
        }

        if (match1 || match2) {
            // It's a valid prefix, wait for more data
            return;
        }

        // Mismatch: Flush the first character and retry
        if (term->printer_callback) {
            term->printer_callback(term, (const char*)&GET_SESSION(term)->printer_buffer[0], 1);
        }
        memmove(GET_SESSION(term)->printer_buffer, GET_SESSION(term)->printer_buffer + 1, --GET_SESSION(term)->printer_buf_len);
    }
}

// Continue with enhanced character processing...
void ProcessChar(Terminal* term, unsigned char ch) {
    if (GET_SESSION(term)->printer_controller_enabled) {
        ProcessPrinterControllerChar(term, ch);
        return;
    }

    switch (GET_SESSION(term)->parse_state) {
        case VT_PARSE_NORMAL:              ProcessNormalChar(term, ch); break;
        case VT_PARSE_ESCAPE:              ProcessEscapeChar(term, ch); break;
        case PARSE_CSI:                 ProcessCSIChar(term, ch); break;
        case PARSE_OSC:                 ProcessOSCChar(term, ch); break;
        case PARSE_DCS:                 ProcessDCSChar(term, ch); break;
        case PARSE_SIXEL_ST:            ProcessSixelSTChar(term, ch); break;
        case PARSE_VT52:                ProcessVT52Char(term, ch); break;
        case PARSE_TEKTRONIX:           ProcessTektronixChar(term, ch); break;
        case PARSE_REGIS:               ProcessReGISChar(term, ch); break;
        case PARSE_SIXEL:               ProcessSixelChar(term, ch); break;
        case PARSE_CHARSET:             ProcessCharsetCommand(term, ch); break;
        case PARSE_HASH:                ProcessHashChar(term, ch); break;
        case PARSE_PERCENT:             ProcessPercentChar(term, ch); break;
        case PARSE_APC:                 ProcessAPCChar(term, ch); break;
        case PARSE_PM:                  ProcessPMChar(term, ch); break;
        case PARSE_SOS:                 ProcessSOSChar(term, ch); break;
        default:
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            ProcessNormalChar(term, ch);
            break;
    }
}

// CSI Pts ; Pls ; Pbs ; Prs ; Pps ; Ptd ; Pld ; Ppd $ v
void ExecuteDECCRA(Terminal* term) { // Copy Rectangular Area (DECCRA)
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        LogUnsupportedSequence(term, "DECCRA requires rectangular operations support");
        return;
    }
    if (GET_SESSION(term)->param_count != 8) {
        LogUnsupportedSequence(term, "Invalid parameters for DECCRA");
        return;
    }
    int top = GetCSIParam(term, 0, 1) - 1;
    int left = GetCSIParam(term, 1, 1) - 1;
    int bottom = GetCSIParam(term, 2, 1) - 1;
    int right = GetCSIParam(term, 3, 1) - 1;
    // Ps4 is source page, not supported.
    int dest_top = GetCSIParam(term, 5, 1) - 1;
    int dest_left = GetCSIParam(term, 6, 1) - 1;
    // Ps8 is destination page, not supported.

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;
    if (top > bottom || left > right) return;

    VTRectangle rect = {top, left, bottom, right, true};
    CopyRectangle(term, rect, dest_left, dest_top);
}

static unsigned int CalculateRectChecksum(Terminal* term, int top, int left, int bottom, int right) {
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

void ExecuteDECRQCRA(Terminal* term) { // Request Rectangular Area Checksum
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        LogUnsupportedSequence(term, "DECRQCRA requires rectangular operations support");
        return;
    }

    // CSI Pid ; Pp ; Pt ; Pl ; Pb ; Pr $ w
    int pid = GetCSIParam(term, 0, 1);
    // int page = GetCSIParam(term, 1, 1); // Ignored
    int top = GetCSIParam(term, 2, 1) - 1;
    int left = GetCSIParam(term, 3, 1) - 1;
    int bottom = GetCSIParam(term, 4, DEFAULT_TERM_HEIGHT) - 1;
    int right = GetCSIParam(term, 5, DEFAULT_TERM_WIDTH) - 1;

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;

    unsigned int checksum = 0;
    if (top <= bottom && left <= right) {
        checksum = CalculateRectChecksum(term, top, left, bottom, right);
    }

    // Response: DCS Pid ! ~ Checksum ST
    char response[32];
    snprintf(response, sizeof(response), "\x1BP%d!~%04X\x1B\\", pid, checksum & 0xFFFF);
    QueueResponse(term, response);
}

// CSI Pch ; Pt ; Pl ; Pb ; Pr $ x
void ExecuteDECFRA(Terminal* term) { // Fill Rectangular Area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        LogUnsupportedSequence(term, "DECFRA requires rectangular operations support");
        return;
    }

    if (GET_SESSION(term)->param_count != 5) {
        LogUnsupportedSequence(term, "Invalid parameters for DECFRA");
        return;
    }

    int char_code = GetCSIParam(term, 0, ' ');
    int top = GetCSIParam(term, 1, 1) - 1;
    int left = GetCSIParam(term, 2, 1) - 1;
    int bottom = GetCSIParam(term, 3, 1) - 1;
    int right = GetCSIParam(term, 4, 1) - 1;

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;
    if (top > bottom || left > right) return;

    unsigned int fill_char = (unsigned int)char_code;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
            cell->ch = fill_char;
            cell->fg_color = GET_SESSION(term)->current_fg;
            cell->bg_color = GET_SESSION(term)->current_bg;
            cell->bold = GET_SESSION(term)->bold_mode;
            cell->faint = GET_SESSION(term)->faint_mode;
            cell->italic = GET_SESSION(term)->italic_mode;
            cell->underline = GET_SESSION(term)->underline_mode;
            cell->blink = GET_SESSION(term)->blink_mode;
            cell->reverse = GET_SESSION(term)->reverse_mode;
            cell->strikethrough = GET_SESSION(term)->strikethrough_mode;
            cell->conceal = GET_SESSION(term)->conceal_mode;
            cell->overline = GET_SESSION(term)->overline_mode;
            cell->double_underline = GET_SESSION(term)->double_underline_mode;
            cell->dirty = true;
        }
        GET_SESSION(term)->row_dirty[y] = true;
    }
}

// CSI ? Psl {
void ExecuteDECSLE(Terminal* term) { // Select Locator Events
    if (!GET_SESSION(term)->conformance.features.locator) {
        LogUnsupportedSequence(term, "DECSLE requires locator support");
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
                    LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    }
}

void ExecuteDECSASD(Terminal* term) {
    // CSI Ps $ }
    // Select Active Status Display
    // 0 = Main Display, 1 = Status Line
    int mode = GetCSIParam(term, 0, 0);
    if (mode == 0 || mode == 1) {
        GET_SESSION(term)->active_display = mode;
    }
}

void ExecuteDECSSDT(Terminal* term) {
    // CSI Ps $ ~
    // Select Split Definition Type
    // 0 = No Split, 1 = Horizontal Split
    if (!GET_SESSION(term)->conformance.features.multi_session_mode) {
        LogUnsupportedSequence(term, "DECSSDT requires multi-session support");
        return;
    }

    int mode = GetCSIParam(term, 0, 0);
    if (mode == 0) {
        SetSplitScreen(term, false, 0, 0, 0);
    } else if (mode == 1) {
        // Default split: Center, Session 0 Top, Session 1 Bottom
        // Future: Support parameterized split points
        SetSplitScreen(term, true, DEFAULT_TERM_HEIGHT / 2, 0, 1);
    } else {
        if (GET_SESSION(term)->options.debug_sequences) {
            char msg[64];
            snprintf(msg, sizeof(msg), "DECSSDT mode %d not supported", mode);
            LogUnsupportedSequence(term, msg);
        }
    }
}

// CSI Plc |
void ExecuteDECRQLP(Terminal* term) { // Request Locator Position
    if (!GET_SESSION(term)->conformance.features.locator) {
        LogUnsupportedSequence(term, "DECRQLP requires locator support");
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

    QueueResponse(term, response);
}


// CSI Pt ; Pl ; Pb ; Pr $ x
void ExecuteDECERA(Terminal* term) { // Erase Rectangular Area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        LogUnsupportedSequence(term, "DECERA requires rectangular operations support");
        return;
    }
    if (GET_SESSION(term)->param_count != 4) {
        LogUnsupportedSequence(term, "Invalid parameters for DECERA");
        return;
    }
    int top = GetCSIParam(term, 0, 1) - 1;
    int left = GetCSIParam(term, 1, 1) - 1;
    int bottom = GetCSIParam(term, 2, 1) - 1;
    int right = GetCSIParam(term, 3, 1) - 1;

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;
    if (top > bottom || left > right) return;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            ClearCell(term, GetActiveScreenCell(GET_SESSION(term), y, x));
        }
        GET_SESSION(term)->row_dirty[y] = true;
    }
}


// CSI Ps ; Pt ; Pl ; Pb ; Pr $ {
void ExecuteDECSERA(Terminal* term) { // Selective Erase Rectangular Area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        LogUnsupportedSequence(term, "DECSERA requires rectangular operations support");
        return;
    }
    if (GET_SESSION(term)->param_count < 4 || GET_SESSION(term)->param_count > 5) {
        LogUnsupportedSequence(term, "Invalid parameters for DECSERA");
        return;
    }
    int erase_param, top, left, bottom, right;

    if (GET_SESSION(term)->param_count == 5) {
        erase_param = GetCSIParam(term, 0, 0);
        top = GetCSIParam(term, 1, 1) - 1;
        left = GetCSIParam(term, 2, 1) - 1;
        bottom = GetCSIParam(term, 3, 1) - 1;
        right = GetCSIParam(term, 4, 1) - 1;
    } else { // param_count == 4
        erase_param = 0; // Default when Ps is omitted
        top = GetCSIParam(term, 0, 1) - 1;
        left = GetCSIParam(term, 1, 1) - 1;
        bottom = GetCSIParam(term, 2, 1) - 1;
        right = GetCSIParam(term, 3, 1) - 1;
    }

    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;
    if (top > bottom || left > right) return;

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            bool should_erase = false;
            EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
            switch (erase_param) {
                case 0: if (!cell->protected_cell) should_erase = true; break;
                case 1: should_erase = true; break;
                case 2: if (cell->protected_cell) should_erase = true; break;
            }
            if (should_erase) {
                ClearCell(term, cell);
            }
        }
        GET_SESSION(term)->row_dirty[y] = true;
    }
}

void ProcessOSCChar(Terminal* term, unsigned char ch) {
    if (GET_SESSION(term)->escape_pos < sizeof(GET_SESSION(term)->escape_buffer) - 1) {
        GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos++] = ch;

        if (ch == '\a') {
            GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 1] = '\0';
            ExecuteOSCCommand(term);
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            GET_SESSION(term)->escape_pos = 0;
        } else if (ch == '\\' && GET_SESSION(term)->escape_pos >= 2 && GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 2] == '\x1B') {
            GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 2] = '\0';
            ExecuteOSCCommand(term);
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            GET_SESSION(term)->escape_pos = 0;
        }
    } else {
        GET_SESSION(term)->escape_buffer[sizeof(GET_SESSION(term)->escape_buffer) - 1] = '\0';
        ExecuteOSCCommand(term);
        GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
        GET_SESSION(term)->escape_pos = 0;
        LogUnsupportedSequence(term, "OSC sequence too long, truncated");
    }
}

void ProcessDCSChar(Terminal* term, unsigned char ch) {
    if (GET_SESSION(term)->escape_pos < sizeof(GET_SESSION(term)->escape_buffer) - 1) {
        GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos++] = ch;

        // Ensure this is not DECRQSS ($q)
        bool is_decrqss = (GET_SESSION(term)->escape_pos >= 2 && GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 2] == '$');

        if (ch == 'q' && GET_SESSION(term)->conformance.features.sixel_graphics && !is_decrqss) {
            // Sixel Graphics command
            ParseCSIParams(term, GET_SESSION(term)->escape_buffer, GET_SESSION(term)->sixel.params, MAX_ESCAPE_PARAMS);
            GET_SESSION(term)->sixel.param_count = GET_SESSION(term)->param_count;

            GET_SESSION(term)->sixel.pos_x = 0;
            GET_SESSION(term)->sixel.pos_y = 0;
            GET_SESSION(term)->sixel.max_x = 0;
            GET_SESSION(term)->sixel.max_y = 0;
            GET_SESSION(term)->sixel.color_index = 0;
            GET_SESSION(term)->sixel.repeat_count = 0;

            // Parse P2 for background transparency (0=Device Default, 1=Transparent, 2=Opaque)
            int p2 = (GET_SESSION(term)->param_count >= 2) ? GET_SESSION(term)->sixel.params[1] : 0;
            GET_SESSION(term)->sixel.transparent_bg = (p2 == 1);

            if (!GET_SESSION(term)->sixel.data) {
                GET_SESSION(term)->sixel.width = DEFAULT_TERM_WIDTH * DEFAULT_CHAR_WIDTH;
                GET_SESSION(term)->sixel.height = DEFAULT_TERM_HEIGHT * DEFAULT_CHAR_HEIGHT;
                GET_SESSION(term)->sixel.data = calloc(GET_SESSION(term)->sixel.width * GET_SESSION(term)->sixel.height * 4, 1);
            }

            if (GET_SESSION(term)->sixel.data) {
                memset(GET_SESSION(term)->sixel.data, 0, GET_SESSION(term)->sixel.width * GET_SESSION(term)->sixel.height * 4);
            }

            if (!GET_SESSION(term)->sixel.strips) {
                GET_SESSION(term)->sixel.strip_capacity = 65536;
                GET_SESSION(term)->sixel.strips = (GPUSixelStrip*)calloc(GET_SESSION(term)->sixel.strip_capacity, sizeof(GPUSixelStrip));
            }
            GET_SESSION(term)->sixel.strip_count = 0;

            GET_SESSION(term)->sixel.active = true;
            GET_SESSION(term)->sixel.scrolling = true; // Default Sixel behavior scrolls
            GET_SESSION(term)->sixel.logical_start_row = GET_SESSION(term)->screen_head;
            GET_SESSION(term)->sixel.x = GET_SESSION(term)->cursor.x * DEFAULT_CHAR_WIDTH;
            GET_SESSION(term)->sixel.y = GET_SESSION(term)->cursor.y * DEFAULT_CHAR_HEIGHT;

            GET_SESSION(term)->parse_state = PARSE_SIXEL;
            GET_SESSION(term)->escape_pos = 0;
            return;
        }

        if (ch == 'p' && GET_SESSION(term)->conformance.features.regis_graphics) {
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

            GET_SESSION(term)->parse_state = PARSE_REGIS;
            GET_SESSION(term)->escape_pos = 0;
            return;
        }

        if (ch == '\a') { // Non-standard, but some terminals accept BEL for DCS
            GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 1] = '\0';
            ExecuteDCSCommand(term);
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            GET_SESSION(term)->escape_pos = 0;
        } else if (ch == '\\' && GET_SESSION(term)->escape_pos >= 2 && GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 2] == '\x1B') { // ST (ESC \)
            GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 2] = '\0';
            ExecuteDCSCommand(term);
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            GET_SESSION(term)->escape_pos = 0;
        }
    } else { // Buffer overflow
        GET_SESSION(term)->escape_buffer[sizeof(GET_SESSION(term)->escape_buffer) - 1] = '\0';
        ExecuteDCSCommand(term);
        GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
        GET_SESSION(term)->escape_pos = 0;
        LogUnsupportedSequence(term, "DCS sequence too long, truncated");
    }
}

// =============================================================================
// ENHANCED FONT SYSTEM WITH UNICODE SUPPORT
// =============================================================================

void CreateFontTexture(Terminal* term) {
    if (term->font_texture.generation != 0) {
        SituationDestroyTexture(&term->font_texture);
    }

    const int chars_per_row = term->atlas_cols > 0 ? term->atlas_cols : 16;
    const int num_chars_base = 256;

    // Allocate persistent CPU buffer if not present
    if (!term->font_atlas_pixels) {
        term->font_atlas_pixels = calloc(term->atlas_width * term->atlas_height * 4, 1);
        if (!term->font_atlas_pixels) return;
        term->next_atlas_index = 256; // Start dynamic allocation after base set
    }

    unsigned char* pixels = term->font_atlas_pixels;

    int char_w = DEFAULT_CHAR_WIDTH;
    int char_h = DEFAULT_CHAR_HEIGHT;
    if (GET_SESSION(term)->soft_font.active) {
        char_w = GET_SESSION(term)->soft_font.char_width;
        char_h = GET_SESSION(term)->soft_font.char_height;
    }
    int dynamic_chars_per_row = term->atlas_width / char_w;

    // Unpack the font data (Base 256 chars)
    for (int i = 0; i < num_chars_base; i++) {
        int glyph_col = i % dynamic_chars_per_row;
        int glyph_row = i / dynamic_chars_per_row;
        int dest_x_start = glyph_col * char_w;
        int dest_y_start = glyph_row * char_h;

        for (int y = 0; y < char_h; y++) {
            unsigned char byte;
            if (GET_SESSION(term)->soft_font.active && GET_SESSION(term)->soft_font.loaded[i]) {
                byte = GET_SESSION(term)->soft_font.font_data[i][y];
            } else {
                if (y < 16) byte = cp437_font__8x16[i * 16 + y];
                else byte = 0;
            }

            for (int x = 0; x < char_w; x++) {
                int px_idx = ((dest_y_start + y) * term->atlas_width + (dest_x_start + x)) * 4;
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
    img.width = term->atlas_width;
    img.height = term->atlas_height;
    img.channels = 4;
    img.data = pixels;

    if (term->font_texture.generation != 0) SituationDestroyTexture(&term->font_texture);
    SituationCreateTexture(img, false, &term->font_texture);
    // Don't unload image data as it points to persistent buffer
}

void InitTerminalCompute(Terminal* term) {
    if (term->compute_initialized) return;

    // 1. Create SSBO
    size_t buffer_size = term->width * term->height * sizeof(GPUCell);
    SituationCreateBuffer(buffer_size, NULL, SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_TRANSFER_DST, &term->terminal_buffer);

    // 2. Create Storage Image (Output)
    SituationImage empty_img = {0};
    // Use current dimensions
    int win_width = term->width * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE;
    int win_height = term->height * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE;

    SituationCreateImage(win_width, win_height, 4, &empty_img); // RGBA
    // We can init to black if we want, but compute will overwrite.
    SituationCreateTextureEx(empty_img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_SRC, &term->output_texture);
    SituationUnloadImage(empty_img);

    // 3. Create Compute Pipeline
    {
        size_t l1 = strlen(terminal_compute_preamble);
        size_t l2 = strlen(terminal_shader_body);
        char* src = (char*)malloc(l1 + l2 + 1);
        if (src) {
            strcpy(src, terminal_compute_preamble);
            strcat(src, terminal_shader_body);
            SituationCreateComputePipelineFromMemory(src, SIT_COMPUTE_LAYOUT_TERMINAL, &term->compute_pipeline);
            free(src);
        }
    }

    // Create Dummy Sixel Texture (1x1 transparent)
    SituationImage dummy_img = {0};
    if (SituationCreateImage(1, 1, 4, &dummy_img) == SITUATION_SUCCESS) {
        memset(dummy_img.data, 0, 4); // Clear to transparent
        SituationCreateTextureEx(dummy_img, false, SITUATION_TEXTURE_USAGE_SAMPLED, &term->dummy_sixel_texture);
        SituationUnloadImage(dummy_img);
    }

    term->gpu_staging_buffer = (GPUCell*)calloc(term->width * term->height, sizeof(GPUCell));

    // 4. Init Vector Engine (Storage Tube Architecture)
    term->vector_capacity = 65536; // Max new lines per frame
    SituationCreateBuffer(term->vector_capacity * sizeof(GPUVectorLine), NULL, SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_TRANSFER_DST, &term->vector_buffer);
    term->vector_staging_buffer = (GPUVectorLine*)calloc(term->vector_capacity, sizeof(GPUVectorLine));

    // Create Persistent Vector Layer Texture (Storage Tube Surface)
    SituationImage vec_img = {0};
    SituationCreateImage(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 4, &vec_img);
    memset(vec_img.data, 0, DEFAULT_WINDOW_WIDTH * DEFAULT_WINDOW_HEIGHT * 4); // Clear to transparent black
    SituationCreateTextureEx(vec_img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_DST, &term->vector_layer_texture);
    SituationUnloadImage(vec_img);

    // Create Vector Pipeline
    {
        size_t l1 = strlen(vector_compute_preamble);
        size_t l2 = strlen(vector_shader_body);
        char* src = (char*)malloc(l1 + l2 + 1);
        if (src) {
            strcpy(src, vector_compute_preamble);
            strcat(src, vector_shader_body);
            SituationCreateComputePipelineFromMemory(src, SIT_COMPUTE_LAYOUT_VECTOR, &term->vector_pipeline);
            free(src);
        }
    }

    // 5. Init Sixel Engine
    SituationCreateBuffer(65536 * sizeof(GPUSixelStrip), NULL, SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_TRANSFER_DST, &term->sixel_buffer);
    SituationCreateBuffer(256 * sizeof(uint32_t), NULL, SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_TRANSFER_DST, &term->sixel_palette_buffer);
        {
        size_t l1 = strlen(sixel_compute_preamble);
        size_t l2 = strlen(sixel_shader_body);
        char* src = (char*)malloc(l1 + l2 + 1);
        if (src) {
            strcpy(src, sixel_compute_preamble);
            strcat(src, sixel_shader_body);
            SituationCreateComputePipelineFromMemory(src, SIT_COMPUTE_LAYOUT_SIXEL, &term->sixel_pipeline);
            free(src);
        }
    }

    term->compute_initialized = true;
}

// =============================================================================
// CHARACTER SET TRANSLATION SYSTEM
// =============================================================================

unsigned int TranslateCharacter(Terminal* term, unsigned char ch, CharsetState* state) {
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
static void RenderGlyphToAtlas(Terminal* term, uint32_t codepoint, uint32_t idx) {
    int col = idx % term->atlas_cols;
    int row = idx / term->atlas_cols;
    int x_start = col * DEFAULT_CHAR_WIDTH;
    int y_start = row * DEFAULT_CHAR_HEIGHT;

    if (term->ttf.loaded) {
        // TTF Rendering
        int advance, lsb, x0, y0, x1, y1;
        stbtt_GetCodepointHMetrics(&term->ttf.info, codepoint, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&term->ttf.info, codepoint, term->ttf.scale, term->ttf.scale, &x0, &y0, &x1, &y1);

        // Center horizontally
        // x0 is usually small negative/positive bearing. Width of glyph is x1-x0.
        int gw = x1 - x0;
        int x_off = (DEFAULT_CHAR_WIDTH - gw) / 2;

        // Better approach:
        int w, h, xoff, yoff;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&term->ttf.info, 0, term->ttf.scale, codepoint, &w, &h, &xoff, &yoff);

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
        }
    } else {
        // Fallback: Hex Box
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

void LoadTerminalFont(Terminal* term, const char* filepath) {
    unsigned int size;
    unsigned char* buffer = NULL;
    if (SituationLoadFileData(filepath, &size, &buffer) != SITUATION_SUCCESS || !buffer) {
        if (term->response_callback) term->response_callback(term, "Font load failed\r\n", 18);
        return;
    }

    if (term->ttf.file_buffer) SIT_FREE(term->ttf.file_buffer);
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
uint32_t AllocateGlyph(Terminal* term, uint32_t codepoint) {
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
        // Atlas full. Use LRU Eviction.
        uint32_t lru_index = 0;
        uint64_t min_frame = 0xFFFFFFFFFFFFFFFF;

        // Scan for the oldest entry. Start from 256 to protect base set.
        // Optimization: We could use a min-heap or linked list, but linear scan is acceptable for this size/frequency.
        for (uint32_t i = 256; i < capacity; i++) {
            if (term->glyph_last_used[i] < min_frame) {
                min_frame = term->glyph_last_used[i];
                lru_index = i;
            }
        }

        // Evict
        if (lru_index >= 256) {
            uint32_t old_codepoint = term->atlas_to_codepoint[lru_index];
            if (old_codepoint < 65536) {
                term->glyph_map[old_codepoint] = 0; // Clear from map
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
uint32_t MapUnicodeToAtlas(Terminal* term, uint32_t codepoint) {
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
    return AllocateGlyph(term, codepoint);
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

unsigned int TranslateDECSpecial(Terminal* term, unsigned char ch) {
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

unsigned int TranslateDECMultinational(Terminal* term, unsigned char ch) {
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

void SetTabStop(Terminal* term, int column) {
    if (column >= 0 && column < MAX_TAB_STOPS && column < DEFAULT_TERM_WIDTH) {
        if (!GET_SESSION(term)->tab_stops.stops[column]) {
            GET_SESSION(term)->tab_stops.stops[column] = true;
            GET_SESSION(term)->tab_stops.count++;
        }
    }
}

void ClearTabStop(Terminal* term, int column) {
    if (column >= 0 && column < MAX_TAB_STOPS) {
        if (GET_SESSION(term)->tab_stops.stops[column]) {
            GET_SESSION(term)->tab_stops.stops[column] = false;
            GET_SESSION(term)->tab_stops.count--;
        }
    }
}

void ClearAllTabStops(Terminal* term) {
    memset(GET_SESSION(term)->tab_stops.stops, false, sizeof(GET_SESSION(term)->tab_stops.stops));
    GET_SESSION(term)->tab_stops.count = 0;
}

int NextTabStop(Terminal* term, int current_column) {
    for (int i = current_column + 1; i < MAX_TAB_STOPS && i < DEFAULT_TERM_WIDTH; i++) {
        if (GET_SESSION(term)->tab_stops.stops[i]) {
            return i;
        }
    }

    // If no explicit tab stop found, use default spacing
    int next = ((current_column / GET_SESSION(term)->tab_stops.default_width) + 1) * GET_SESSION(term)->tab_stops.default_width;
    return (next < DEFAULT_TERM_WIDTH) ? next : DEFAULT_TERM_WIDTH - 1;
}

int PreviousTabStop(Terminal* term, int current_column) {
    for (int i = current_column - 1; i >= 0; i--) {
        if (GET_SESSION(term)->tab_stops.stops[i]) {
            return i;
        }
    }

    // If no explicit tab stop found, use default spacing
    int prev = ((current_column - 1) / GET_SESSION(term)->tab_stops.default_width) * GET_SESSION(term)->tab_stops.default_width;
    return (prev >= 0) ? prev : 0;
}

// =============================================================================
// ENHANCED SCREEN MANIPULATION
// =============================================================================

void ClearCell(Terminal* term, EnhancedTermChar* cell) {
    cell->ch = ' ';
    cell->fg_color = GET_SESSION(term)->current_fg;
    cell->bg_color = GET_SESSION(term)->current_bg;

    // Copy all current attributes
    cell->bold = GET_SESSION(term)->bold_mode;
    cell->faint = GET_SESSION(term)->faint_mode;
    cell->italic = GET_SESSION(term)->italic_mode;
    cell->underline = GET_SESSION(term)->underline_mode;
    cell->blink = GET_SESSION(term)->blink_mode;
    cell->reverse = GET_SESSION(term)->reverse_mode;
    cell->strikethrough = GET_SESSION(term)->strikethrough_mode;
    cell->conceal = GET_SESSION(term)->conceal_mode;
    cell->overline = GET_SESSION(term)->overline_mode;
    cell->double_underline = GET_SESSION(term)->double_underline_mode;
    cell->protected_cell = GET_SESSION(term)->protected_mode;

    // Reset special attributes
    cell->double_width = false;
    cell->double_height_top = false;
    cell->double_height_bottom = false;
    cell->soft_hyphen = false;
    cell->combining = false;

    cell->dirty = true;
}

void ScrollUpRegion(Terminal* term, int top, int bottom, int lines) {
    // Check for full screen scroll (Top to Bottom, Full Width)
    // This allows optimization via Ring Buffer pointer arithmetic.
    if (top == 0 && bottom == DEFAULT_TERM_HEIGHT - 1 &&
        GET_SESSION(term)->left_margin == 0 && GET_SESSION(term)->right_margin == DEFAULT_TERM_WIDTH - 1)
    {
        for (int i = 0; i < lines; i++) {
            // Increment head (scrolling down in memory, visually up)
            GET_SESSION(term)->screen_head = (GET_SESSION(term)->screen_head + 1) % GET_SESSION(term)->buffer_height;

            // Adjust view_offset to keep historical view stable if user is looking back
            if (GET_SESSION(term)->view_offset > 0) {
                 GET_SESSION(term)->view_offset++;
                 // Cap at buffer limits
                 int max_offset = GET_SESSION(term)->buffer_height - DEFAULT_TERM_HEIGHT;
                 if (GET_SESSION(term)->view_offset > max_offset) GET_SESSION(term)->view_offset = max_offset;
            }

            // Clear the new bottom line (logical row 'bottom')
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                ClearCell(term, GetActiveScreenCell(GET_SESSION(term), bottom, x));
            }
        }
        // Invalidate all viewport rows because the data under them has shifted
        for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
            GET_SESSION(term)->row_dirty[y] = true;
        }
        return;
    }

    // Partial Scroll (Fallback to copy) - Strictly NO head manipulation
    for (int i = 0; i < lines; i++) {
        // Move lines up within the region
        for (int y = top; y < bottom; y++) {
            for (int x = GET_SESSION(term)->left_margin; x <= GET_SESSION(term)->right_margin; x++) {
                *GetActiveScreenCell(GET_SESSION(term), y, x) = *GetActiveScreenCell(GET_SESSION(term), y + 1, x);
                GetActiveScreenCell(GET_SESSION(term), y, x)->dirty = true;
            }
            GET_SESSION(term)->row_dirty[y] = true;
        }

        // Clear bottom line of the region
        for (int x = GET_SESSION(term)->left_margin; x <= GET_SESSION(term)->right_margin; x++) {
            ClearCell(term, GetActiveScreenCell(GET_SESSION(term), bottom, x));
        }
        GET_SESSION(term)->row_dirty[bottom] = true;
    }
}

void ScrollDownRegion(Terminal* term, int top, int bottom, int lines) {
    for (int i = 0; i < lines; i++) {
        // Move lines down
        for (int y = bottom; y > top; y--) {
            for (int x = GET_SESSION(term)->left_margin; x <= GET_SESSION(term)->right_margin; x++) {
                *GetActiveScreenCell(GET_SESSION(term), y, x) = *GetActiveScreenCell(GET_SESSION(term), y - 1, x);
                GetActiveScreenCell(GET_SESSION(term), y, x)->dirty = true;
            }
            GET_SESSION(term)->row_dirty[y] = true;
        }

        // Clear top line
        for (int x = GET_SESSION(term)->left_margin; x <= GET_SESSION(term)->right_margin; x++) {
            ClearCell(term, GetActiveScreenCell(GET_SESSION(term), top, x));
        }
        GET_SESSION(term)->row_dirty[top] = true;
    }
}

void InsertLinesAt(Terminal* term, int row, int count) {
    if (row < GET_SESSION(term)->scroll_top || row > GET_SESSION(term)->scroll_bottom) {
        return;
    }

    // Move existing lines down
    for (int y = GET_SESSION(term)->scroll_bottom; y >= row + count; y--) {
        if (y - count >= row) {
            for (int x = GET_SESSION(term)->left_margin; x <= GET_SESSION(term)->right_margin; x++) {
                *GetActiveScreenCell(GET_SESSION(term), y, x) = *GetActiveScreenCell(GET_SESSION(term), y - count, x);
                GetActiveScreenCell(GET_SESSION(term), y, x)->dirty = true;
            }
            GET_SESSION(term)->row_dirty[y] = true;
        }
    }

    // Clear inserted lines
    for (int y = row; y < row + count && y <= GET_SESSION(term)->scroll_bottom; y++) {
        for (int x = GET_SESSION(term)->left_margin; x <= GET_SESSION(term)->right_margin; x++) {
            ClearCell(term, GetActiveScreenCell(GET_SESSION(term), y, x));
        }
        GET_SESSION(term)->row_dirty[y] = true;
    }
}

void DeleteLinesAt(Terminal* term, int row, int count) {
    if (row < GET_SESSION(term)->scroll_top || row > GET_SESSION(term)->scroll_bottom) {
        return;
    }

    // Move remaining lines up
    for (int y = row; y <= GET_SESSION(term)->scroll_bottom - count; y++) {
        for (int x = GET_SESSION(term)->left_margin; x <= GET_SESSION(term)->right_margin; x++) {
            *GetActiveScreenCell(GET_SESSION(term), y, x) = *GetActiveScreenCell(GET_SESSION(term), y + count, x);
            GetActiveScreenCell(GET_SESSION(term), y, x)->dirty = true;
        }
        GET_SESSION(term)->row_dirty[y] = true;
    }

    // Clear bottom lines
    for (int y = GET_SESSION(term)->scroll_bottom - count + 1; y <= GET_SESSION(term)->scroll_bottom; y++) {
        if (y >= 0) {
            for (int x = GET_SESSION(term)->left_margin; x <= GET_SESSION(term)->right_margin; x++) {
                ClearCell(term, GetActiveScreenCell(GET_SESSION(term), y, x));
            }
            GET_SESSION(term)->row_dirty[y] = true;
        }
    }
}

void InsertCharactersAt(Terminal* term, int row, int col, int count) {
    // Shift existing characters right
    for (int x = GET_SESSION(term)->right_margin; x >= col + count; x--) {
        if (x - count >= col) {
            *GetActiveScreenCell(GET_SESSION(term), row, x) = *GetActiveScreenCell(GET_SESSION(term), row, x - count);
            GetActiveScreenCell(GET_SESSION(term), row, x)->dirty = true;
        }
    }

    // Clear inserted positions
    for (int x = col; x < col + count && x <= GET_SESSION(term)->right_margin; x++) {
        ClearCell(term, GetActiveScreenCell(GET_SESSION(term), row, x));
    }
    GET_SESSION(term)->row_dirty[row] = true;
}

void DeleteCharactersAt(Terminal* term, int row, int col, int count) {
    // Shift remaining characters left
    for (int x = col; x <= GET_SESSION(term)->right_margin - count; x++) {
        *GetActiveScreenCell(GET_SESSION(term), row, x) = *GetActiveScreenCell(GET_SESSION(term), row, x + count);
        GetActiveScreenCell(GET_SESSION(term), row, x)->dirty = true;
    }

    // Clear rightmost positions
    for (int x = GET_SESSION(term)->right_margin - count + 1; x <= GET_SESSION(term)->right_margin; x++) {
        if (x >= 0) {
            ClearCell(term, GetActiveScreenCell(GET_SESSION(term), row, x));
        }
    }
    GET_SESSION(term)->row_dirty[row] = true;
}

// =============================================================================
// VT100 INSERT MODE IMPLEMENTATION
// =============================================================================

void EnableInsertMode(Terminal* term, bool enable) {
    GET_SESSION(term)->dec_modes.insert_mode = enable;
}

void InsertCharacterAtCursor(Terminal* term, unsigned int ch) {
    if (GET_SESSION(term)->dec_modes.insert_mode) {
        // Insert mode: shift existing characters right
        InsertCharactersAt(term, GET_SESSION(term)->cursor.y, GET_SESSION(term)->cursor.x, 1);
    }

    // Place character at cursor position
    EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, GET_SESSION(term)->cursor.x);
    cell->ch = ch;
    cell->fg_color = GET_SESSION(term)->current_fg;
    cell->bg_color = GET_SESSION(term)->current_bg;

    // Apply current attributes
    cell->bold = GET_SESSION(term)->bold_mode;
    cell->faint = GET_SESSION(term)->faint_mode;
    cell->italic = GET_SESSION(term)->italic_mode;
    cell->underline = GET_SESSION(term)->underline_mode;
    cell->blink = GET_SESSION(term)->blink_mode;
    cell->reverse = GET_SESSION(term)->reverse_mode;
    cell->strikethrough = GET_SESSION(term)->strikethrough_mode;
    cell->conceal = GET_SESSION(term)->conceal_mode;
    cell->overline = GET_SESSION(term)->overline_mode;
    cell->double_underline = GET_SESSION(term)->double_underline_mode;
    cell->protected_cell = GET_SESSION(term)->protected_mode;

    cell->dirty = true;
    GET_SESSION(term)->row_dirty[GET_SESSION(term)->cursor.y] = true;

    // Track last printed character for REP command
    GET_SESSION(term)->last_char = ch;
}

// =============================================================================
// COMPREHENSIVE CHARACTER PROCESSING
// =============================================================================

void ProcessNormalChar(Terminal* term, unsigned char ch) {
    // Handle control characters first
    if (ch < 32) {
        ProcessControlChar(term, ch);
        return;
    }

    // Translate character through active character set
    unsigned int unicode_ch = TranslateCharacter(term, ch, &GET_SESSION(term)->charset);

    // Handle UTF-8 decoding if enabled
    if (*GET_SESSION(term)->charset.gl == CHARSET_UTF8) {
        if (GET_SESSION(term)->utf8.bytes_remaining == 0) {
            if (ch < 0x80) {
                // 1-byte sequence (ASCII)
                unicode_ch = ch;
            } else if ((ch & 0xE0) == 0xC0) {
                // 2-byte sequence
                GET_SESSION(term)->utf8.codepoint = ch & 0x1F;
                GET_SESSION(term)->utf8.bytes_remaining = 1;
                return;
            } else if ((ch & 0xF0) == 0xE0) {
                // 3-byte sequence
                GET_SESSION(term)->utf8.codepoint = ch & 0x0F;
                GET_SESSION(term)->utf8.bytes_remaining = 2;
                return;
            } else if ((ch & 0xF8) == 0xF0) {
                // 4-byte sequence
                GET_SESSION(term)->utf8.codepoint = ch & 0x07;
                GET_SESSION(term)->utf8.bytes_remaining = 3;
                return;
            } else {
                // Invalid start byte
                unicode_ch = 0xFFFD;
                InsertCharacterAtCursor(term, unicode_ch);
                GET_SESSION(term)->cursor.x++;
                return;
            }
        } else {
            // Continuation byte
            if ((ch & 0xC0) == 0x80) {
                GET_SESSION(term)->utf8.codepoint = (GET_SESSION(term)->utf8.codepoint << 6) | (ch & 0x3F);
                GET_SESSION(term)->utf8.bytes_remaining--;
                if (GET_SESSION(term)->utf8.bytes_remaining > 0) {
                    return;
                }
                // Sequence complete
                unicode_ch = GET_SESSION(term)->utf8.codepoint;

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
                InsertCharacterAtCursor(term, unicode_ch);
                GET_SESSION(term)->cursor.x++;

                // Reset and try to recover
                GET_SESSION(term)->utf8.bytes_remaining = 0;
                GET_SESSION(term)->utf8.codepoint = 0;
                // If the byte itself is a valid start byte or ASCII, we should process it.
                // Re-evaluate current char as if state was 0.
                if (ch < 0x80) {
                    unicode_ch = ch;
                } else if ((ch & 0xE0) == 0xC0) {
                    GET_SESSION(term)->utf8.codepoint = ch & 0x1F;
                    GET_SESSION(term)->utf8.bytes_remaining = 1;
                    return;
                } else if ((ch & 0xF0) == 0xE0) {
                    GET_SESSION(term)->utf8.codepoint = ch & 0x0F;
                    GET_SESSION(term)->utf8.bytes_remaining = 2;
                    return;
                } else if ((ch & 0xF8) == 0xF0) {
                    GET_SESSION(term)->utf8.codepoint = ch & 0x07;
                    GET_SESSION(term)->utf8.bytes_remaining = 3;
                    return;
                } else {
                    return; // Invalid start byte
                }
            }
        }
    }

    // Handle character display
    if (GET_SESSION(term)->cursor.x > GET_SESSION(term)->right_margin) {
        if (GET_SESSION(term)->dec_modes.auto_wrap_mode) {
            // Auto-wrap to next line
            GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
            GET_SESSION(term)->cursor.y++;

            if (GET_SESSION(term)->cursor.y > GET_SESSION(term)->scroll_bottom) {
                GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_bottom;
                ScrollUpRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, 1);
            }
        } else {
            // No wrap - stay at right margin
            GET_SESSION(term)->cursor.x = GET_SESSION(term)->right_margin;
        }
    }

    // Insert character (handles insert mode internally)
    InsertCharacterAtCursor(term, unicode_ch);

    // Advance cursor
    GET_SESSION(term)->cursor.x++;
}

// Update ProcessControlChar
void ProcessControlChar(Terminal* term, unsigned char ch) {
    switch (ch) {
        case 0x05: // ENQ - Enquiry
            if (GET_SESSION(term)->answerback_buffer[0] != '\0') {
                QueueResponse(term, GET_SESSION(term)->answerback_buffer);
            }
            break;
        case 0x07: // BEL - Bell
            if (term->bell_callback) {
                term->bell_callback(term);
            } else {
                // Visual bell
                GET_SESSION(term)->visual_bell_timer = 0.2;
            }
            break;
        case 0x08: // BS - Backspace
            if (GET_SESSION(term)->cursor.x > GET_SESSION(term)->left_margin) {
                GET_SESSION(term)->cursor.x--;
            }
            break;
        case 0x09: // HT - Horizontal Tab
            GET_SESSION(term)->cursor.x = NextTabStop(term, GET_SESSION(term)->cursor.x);
            if (GET_SESSION(term)->cursor.x > GET_SESSION(term)->right_margin) {
                GET_SESSION(term)->cursor.x = GET_SESSION(term)->right_margin;
            }
            break;
        case 0x0A: // LF - Line Feed
        case 0x0B: // VT - Vertical Tab
        case 0x0C: // FF - Form Feed
            GET_SESSION(term)->cursor.y++;
            if (GET_SESSION(term)->cursor.y > GET_SESSION(term)->scroll_bottom) {
                GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_bottom;
                ScrollUpRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, 1);
            }
            if (GET_SESSION(term)->ansi_modes.line_feed_new_line) {
                GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
            }
            break;
        case 0x0D: // CR - Carriage Return
            GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
            break;
        case 0x0E: // SO - Shift Out (invoke G1 into GL)
            GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g1;
            break;
        case 0x0F: // SI - Shift In (invoke G0 into GL)
            GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g0;
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
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            GET_SESSION(term)->escape_pos = 0;
            break;
        case 0x1B: // ESC - Escape
            GET_SESSION(term)->parse_state = VT_PARSE_ESCAPE;
            GET_SESSION(term)->escape_pos = 0;
            break;
        case 0x7F: // DEL - Delete
            // Ignored
            break;
        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown control char: 0x%02X", ch);
                LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}


// =============================================================================
// ENHANCED ESCAPE SEQUENCE PROCESSING
// =============================================================================

void ProcessEscapeChar(Terminal* term, unsigned char ch) {
    switch (ch) {
        // CSI - Control Sequence Introducer
        case '[':
            GET_SESSION(term)->parse_state = PARSE_CSI;
            GET_SESSION(term)->escape_pos = 0;
            memset(GET_SESSION(term)->escape_params, 0, sizeof(GET_SESSION(term)->escape_params));
            GET_SESSION(term)->param_count = 0;
            break;

        // OSC - Operating System Command
        case ']':
            GET_SESSION(term)->parse_state = PARSE_OSC;
            GET_SESSION(term)->escape_pos = 0;
            break;

        // DCS - Device Control String
        case 'P':
            GET_SESSION(term)->parse_state = PARSE_DCS;
            GET_SESSION(term)->escape_pos = 0;
            break;

        // APC - Application Program Command
        case '_':
            GET_SESSION(term)->parse_state = PARSE_APC;
            GET_SESSION(term)->escape_pos = 0;
            break;

        // PM - Privacy Message
        case '^':
            GET_SESSION(term)->parse_state = PARSE_PM;
            GET_SESSION(term)->escape_pos = 0;
            break;

        // SOS - Start of String
        case 'X':
            GET_SESSION(term)->parse_state = PARSE_SOS;
            GET_SESSION(term)->escape_pos = 0;
            break;

        // Character set selection
        case '(':
            GET_SESSION(term)->parse_state = PARSE_CHARSET;
            GET_SESSION(term)->escape_buffer[0] = '(';
            GET_SESSION(term)->escape_pos = 1;
            break;

        case ')':
            GET_SESSION(term)->parse_state = PARSE_CHARSET;
            GET_SESSION(term)->escape_buffer[0] = ')';
            GET_SESSION(term)->escape_pos = 1;
            break;

        case '*':
            GET_SESSION(term)->parse_state = PARSE_CHARSET;
            GET_SESSION(term)->escape_buffer[0] = '*';
            GET_SESSION(term)->escape_pos = 1;
            break;

        case '+':
            GET_SESSION(term)->parse_state = PARSE_CHARSET;
            GET_SESSION(term)->escape_buffer[0] = '+';
            GET_SESSION(term)->escape_pos = 1;
            break;

        // Locking Shifts (ISO 2022)
        case 'n': // LS2 (GL = G2)
            GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g2;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;
        case 'o': // LS3 (GL = G3)
            GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g3;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;
        case '~': // LS1R (GR = G1)
            GET_SESSION(term)->charset.gr = &GET_SESSION(term)->charset.g1;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;
        case '}': // LS2R (GR = G2)
            GET_SESSION(term)->charset.gr = &GET_SESSION(term)->charset.g2;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;
        case '|': // LS3R (GR = G3)
            GET_SESSION(term)->charset.gr = &GET_SESSION(term)->charset.g3;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        // Single character commands
        case '7': // DECSC - Save Cursor
            ExecuteSaveCursor(term);
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case '8': // DECRC - Restore Cursor
            ExecuteRestoreCursor(term);
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case '#': // DEC Line Attributes
            GET_SESSION(term)->parse_state = PARSE_HASH;
            break;

        case '%': // Select Character Set
            GET_SESSION(term)->parse_state = PARSE_PERCENT;
            break;

        case 'D': // IND - Index
            GET_SESSION(term)->cursor.y++;
            if (GET_SESSION(term)->cursor.y > GET_SESSION(term)->scroll_bottom) {
                GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_bottom;
                ScrollUpRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, 1);
            }
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case 'E': // NEL - Next Line
            GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
            GET_SESSION(term)->cursor.y++;
            if (GET_SESSION(term)->cursor.y > GET_SESSION(term)->scroll_bottom) {
                GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_bottom;
                ScrollUpRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, 1);
            }
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case 'H': // HTS - Set Tab Stop
            SetTabStop(term, GET_SESSION(term)->cursor.x);
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case 'M': // RI - Reverse Index
            GET_SESSION(term)->cursor.y--;
            if (GET_SESSION(term)->cursor.y < GET_SESSION(term)->scroll_top) {
                GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_top;
                ScrollDownRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, 1);
            }
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        // case 'n': // LS2 - Locking Shift 2 (Handled above as LS2)
        // case 'o': // LS3 - Locking Shift 3 (Handled above as LS3)

        case 'N': // SS2 - Single Shift 2
            GET_SESSION(term)->charset.single_shift_2 = true;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case 'O': // SS3 - Single Shift 3
            GET_SESSION(term)->charset.single_shift_3 = true;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case 'Z': // DECID - Identify Terminal
            QueueResponse(term, GET_SESSION(term)->device_attributes);
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case 'c': // RIS - Reset to Initial State
            InitTerminal(term);
            break;

        case '=': // DECKPAM - Keypad Application Mode
            GET_SESSION(term)->vt_keyboard.keypad_mode = true;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case '>': // DECKPNM - Keypad Numeric Mode
            GET_SESSION(term)->vt_keyboard.keypad_mode = false;
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;

        case '<': // Enter VT52 mode (if enabled)
            if (GET_SESSION(term)->conformance.features.vt52_mode) {
                GET_SESSION(term)->parse_state = PARSE_VT52;
            } else {
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                if (GET_SESSION(term)->options.log_unsupported) {
                    LogUnsupportedSequence(term, "VT52 mode not supported");
                }
            }
            break;

        default:
            // Unknown escape sequence
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC %c (0x%02X)", ch, ch);
                LogUnsupportedSequence(term, debug_msg);
            }
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            break;
    }
}

// =============================================================================
// ENHANCED PIPELINE PROCESSING
// =============================================================================
bool PipelineWriteChar(Terminal* term, unsigned char ch) {
    if (GET_SESSION(term)->pipeline_count >= sizeof(GET_SESSION(term)->input_pipeline) - 1) {
        GET_SESSION(term)->pipeline_overflow = true;
        return false;
    }

    GET_SESSION(term)->input_pipeline[GET_SESSION(term)->pipeline_head] = ch;
    GET_SESSION(term)->pipeline_head = (GET_SESSION(term)->pipeline_head + 1) % sizeof(GET_SESSION(term)->input_pipeline);
    GET_SESSION(term)->pipeline_count++;
    return true;
}

bool PipelineWriteString(Terminal* term, const char* str) {
    if (!str) return false;

    while (*str) {
        if (!PipelineWriteChar(term, *str)) {
            return false;
        }
        str++;
    }
    return true;
}

bool PipelineWriteFormat(Terminal* term, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return PipelineWriteString(term, buffer);
}

void ClearPipeline(Terminal* term) {
    GET_SESSION(term)->pipeline_head = 0;
    GET_SESSION(term)->pipeline_tail = 0;
    GET_SESSION(term)->pipeline_count = 0;
    GET_SESSION(term)->pipeline_overflow = false;
}

// =============================================================================
// BASIC IMPLEMENTATIONS FOR MISSING FUNCTIONS
// =============================================================================

void SetResponseCallback(Terminal* term, ResponseCallback callback) {
    term->response_callback = callback;
}

void SetPrinterCallback(Terminal* term, PrinterCallback callback) {
    term->printer_callback = callback;
}

void SetTitleCallback(Terminal* term, TitleCallback callback) {
    term->title_callback = callback;
}

void SetBellCallback(Terminal* term, BellCallback callback) {
    term->bell_callback = callback;
}

void SetNotificationCallback(Terminal* term, NotificationCallback callback) {
    term->notification_callback = callback;
    term->notification_callback = callback;
}

void SetGatewayCallback(Terminal* term, GatewayCallback callback) {
    term->gateway_callback = callback;
}

const char* GetWindowTitle(Terminal* term) {
    return GET_SESSION(term)->title.window_title;
}

const char* GetIconTitle(Terminal* term) {
    return GET_SESSION(term)->title.icon_title;
}

void SetTerminalMode(Terminal* term, const char* mode, bool enable) {
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

void SetCursorShape(Terminal* term, CursorShape shape) {
    GET_SESSION(term)->cursor.shape = shape;
}

void SetCursorColor(Terminal* term, ExtendedColor color) {
    GET_SESSION(term)->cursor.color = color;
}

void SetMouseTracking(Terminal* term, MouseTrackingMode mode) {
    GET_SESSION(term)->mouse.mode = mode;
    GET_SESSION(term)->mouse.enabled = (mode != MOUSE_TRACKING_OFF);
}

// Enable or disable mouse features
// Toggles specific mouse functionalities based on feature name
void EnableMouseFeature(Terminal* term, const char* feature, bool enable) {
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

void EnableBracketedPaste(Terminal* term, bool enable) {
    GET_SESSION(term)->bracketed_paste.enabled = enable;
}

bool IsBracketedPasteActive(Terminal* term) {
    return GET_SESSION(term)->bracketed_paste.active;
}

void ProcessPasteData(Terminal* term, const char* data, size_t length) {
    if (GET_SESSION(term)->bracketed_paste.enabled) {
        PipelineWriteString(term, "\x1B[200~");
        PipelineWriteString(term, data);
        PipelineWriteString(term, "\x1B[201~");
    } else {
        PipelineWriteString(term, data);
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

void CopySelectionToClipboard(Terminal* term) {
    if (!GET_SESSION(term)->selection.active) return;

    int start_y = GET_SESSION(term)->selection.start_y;
    int start_x = GET_SESSION(term)->selection.start_x;
    int end_y = GET_SESSION(term)->selection.end_y;
    int end_x = GET_SESSION(term)->selection.end_x;

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

        EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), cy, cx);
        if (cell->ch) {
            buf_idx += EncodeUTF8(cell->ch, &text_buf[buf_idx]);
        }
    }
    text_buf[buf_idx] = '\0';
    SituationSetClipboardText(text_buf);
    free(text_buf);
}

// Update mouse state (internal use only)
// Processes mouse position, buttons, wheel, motion, focus changes, and updates cursor position
void UpdateMouse(Terminal* term) {
    // 1. Get Global Mouse Position
    Vector2 mouse_pos = SituationGetMousePosition();
    int global_cell_x = (int)(mouse_pos.x / (DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE)); // 0-based
    int global_cell_y = (int)(mouse_pos.y / (DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE)); // 0-based

    // 2. Identify Target Session and Transform Coordinates
    int target_session_idx = term->active_session;
    int local_cell_y = global_cell_y;
    int local_pixel_y = (int)mouse_pos.y + 1; // 1-based

    if (term->split_screen_active) {
        if (global_cell_y <= term->split_row) {
            target_session_idx = term->session_top;
            local_cell_y = global_cell_y;
            // local_pixel_y remains same
        } else {
            target_session_idx = term->session_bottom;
            local_cell_y = global_cell_y - (term->split_row + 1);
            local_pixel_y = (int)mouse_pos.y - ((term->split_row + 1) * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE) + 1;
        }
    }

    // 3. Handle Focus on Click
    if (SituationIsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (term->active_session != target_session_idx) {
            SetActiveSession(term, target_session_idx);
        }
    }

    // 4. Temporarily switch context to target session for logic execution
    int saved_session = term->active_session;
    term->active_session = target_session_idx;

    // --- Begin Session-Specific Logic ---

    // Clamp Coordinates (Session-Relative)
    if (global_cell_x < 0) global_cell_x = 0; if (global_cell_x >= DEFAULT_TERM_WIDTH) global_cell_x = DEFAULT_TERM_WIDTH - 1;
    if (local_cell_y < 0) local_cell_y = 0; if (local_cell_y >= DEFAULT_TERM_HEIGHT) local_cell_y = DEFAULT_TERM_HEIGHT - 1;

    // Handle Mouse Wheel Scrolling
    float wheel = SituationGetMouseWheelMove();
    if (wheel != 0) {
        if (GET_SESSION(term)->dec_modes.alternate_screen) {
            // Send Arrow Keys in Alternate Screen Mode (3 lines per step)
            // Typically Wheel Up -> Arrow Up (Scrolls content down to see up) -> \x1B[A
            // Wheel Down -> Arrow Down -> \x1B[B
            int lines = 3;
            // Positive wheel = Up (scroll back/up). Negative = Down.
            const char* seq = (wheel > 0) ? (GET_SESSION(term)->vt_keyboard.cursor_key_mode ? "\x1BOA" : "\x1B[A")
                                          : (GET_SESSION(term)->vt_keyboard.cursor_key_mode ? "\x1BOB" : "\x1B[B");
            for(int i=0; i<lines; i++) QueueResponse(term, seq);
        } else {
            // Scroll History in Primary Screen Mode
            // Wheel Up (Positive) -> Increase view_offset (Look back)
            // Wheel Down (Negative) -> Decrease view_offset (Look forward)
            int scroll_amount = (int)(wheel * 3.0f);
            GET_SESSION(term)->view_offset += scroll_amount;

            // Clamp
            if (GET_SESSION(term)->view_offset < 0) GET_SESSION(term)->view_offset = 0;
            int max_offset = GET_SESSION(term)->buffer_height - DEFAULT_TERM_HEIGHT;
            if (GET_SESSION(term)->view_offset > max_offset) GET_SESSION(term)->view_offset = max_offset;

            // Invalidate screen
            for(int i=0; i<DEFAULT_TERM_HEIGHT; i++) GET_SESSION(term)->row_dirty[i] = true;
        }
    }

    // Handle Selection Logic (Left Click Drag) - Using Local Coords
    // Note: Cross-session selection is not supported in this simple model.
    if (SituationIsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        GET_SESSION(term)->selection.active = true;
        GET_SESSION(term)->selection.dragging = true;
        GET_SESSION(term)->selection.start_x = global_cell_x;
        GET_SESSION(term)->selection.start_y = local_cell_y;
        GET_SESSION(term)->selection.end_x = global_cell_x;
        GET_SESSION(term)->selection.end_y = local_cell_y;
    } else if (SituationIsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && GET_SESSION(term)->selection.dragging) {
        GET_SESSION(term)->selection.end_x = global_cell_x;
        GET_SESSION(term)->selection.end_y = local_cell_y;
    } else if (SituationIsMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT) && GET_SESSION(term)->selection.dragging) {
        GET_SESSION(term)->selection.dragging = false;
        CopySelectionToClipboard(term);
    }

    // Exit if mouse tracking feature is not supported
    if (!GET_SESSION(term)->conformance.features.mouse_tracking) {
        term->active_session = saved_session; // Restore
        return;
    }

    // Exit if mouse is disabled or tracking is off
    if (!GET_SESSION(term)->mouse.enabled || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_OFF) {
        SituationShowCursor(); // Show system cursor
        GET_SESSION(term)->mouse.cursor_x = -1; // Hide custom cursor
        GET_SESSION(term)->mouse.cursor_y = -1;
        term->active_session = saved_session; // Restore
        return;
    }

    // Hide system cursor during mouse tracking
    SituationHideCursor();

    int pixel_x = (int)mouse_pos.x + 1; // Global X matches Local X for now (no split vertical)

    // Update custom cursor position (1-based for VT)
    GET_SESSION(term)->mouse.cursor_x = global_cell_x + 1;
    GET_SESSION(term)->mouse.cursor_y = local_cell_y + 1;

    // Clamp coordinates for reporting
    if (global_cell_x > 223 && !GET_SESSION(term)->mouse.sgr_mode &&
        GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_SGR &&
        GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_URXVT &&
        GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_PIXEL) {
        // Clamp to 223 if not in extended mode (prevent overflow in legacy encoding)
        // 255 - 32 = 223
        global_cell_x = 223;
    }
    // Vertical is also limited
    if (local_cell_y > 223 && !GET_SESSION(term)->mouse.sgr_mode &&
        GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_SGR &&
        GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_URXVT &&
        GET_SESSION(term)->mouse.mode != MOUSE_TRACKING_PIXEL) {
        local_cell_y = 223;
    }

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
        if (current_buttons_state[i] != GET_SESSION(term)->mouse.buttons[i]) {
            GET_SESSION(term)->mouse.buttons[i] = current_buttons_state[i];
            bool pressed = current_buttons_state[i];
            int report_button_code = i;
            mouse_report[0] = '\0';

            if (GET_SESSION(term)->mouse.sgr_mode || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_URXVT || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_PIXEL) {
                if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) report_button_code += 4;
                if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) report_button_code += 8;
                if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) report_button_code += 16;

                if (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_PIXEL) {
                     snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%d%c",
                             report_button_code, pixel_x, local_pixel_y, pressed ? 'M' : 'm');
                } else {
                     snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%d%c",
                             report_button_code, global_cell_x + 1, local_cell_y + 1, pressed ? 'M' : 'm');
                }
            } else if (GET_SESSION(term)->mouse.mode >= MOUSE_TRACKING_VT200 && GET_SESSION(term)->mouse.mode <= MOUSE_TRACKING_ANY_EVENT) {
                int cb_button = pressed ? i : 3;
                int cb = 32 + cb_button;
                if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) cb += 4;
                if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) cb += 8;
                if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) cb += 16;
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c",
                        (char)cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
            } else if (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_X10) {
                if (pressed) {
                    int cb = 32 + i;
                    snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c",
                            (char)cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
                }
            }
            if (mouse_report[0]) QueueResponse(term, mouse_report);
        }
    }

    // Handle wheel events
    if (wheel_move != 0) {
        int report_button_code = (wheel_move > 0) ? 64 : 65;
        mouse_report[0] = '\0';
        if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) report_button_code += 4;
        if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) report_button_code += 8;
        if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) report_button_code += 16;

        if (GET_SESSION(term)->mouse.sgr_mode || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_URXVT || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_PIXEL) {
             if (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_PIXEL) {
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM",
                        report_button_code, pixel_x, local_pixel_y);
             } else {
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM",
                            report_button_code, global_cell_x + 1, local_cell_y + 1);
             }
        } else if (GET_SESSION(term)->mouse.mode >= MOUSE_TRACKING_VT200 && GET_SESSION(term)->mouse.mode <= MOUSE_TRACKING_ANY_EVENT) {
            int cb = 32 + ((wheel_move > 0) ? 0 : 1) + 64;
            if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) cb += 4;
            if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) cb += 8;
            if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) cb += 16;
            snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c",
                    (char)cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
        }
        if (mouse_report[0]) QueueResponse(term, mouse_report);
    }

    // Handle motion events
    if (global_cell_x != GET_SESSION(term)->mouse.last_x || local_cell_y != GET_SESSION(term)->mouse.last_y ||
        (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_PIXEL && (pixel_x != GET_SESSION(term)->mouse.last_pixel_x || local_pixel_y != GET_SESSION(term)->mouse.last_pixel_y))) {
        bool report_move = false;
        int sgr_motion_code = 35; // Motion no button for SGR
        int vt200_motion_cb = 32 + 3; // Motion no button for VT200

        if (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_ANY_EVENT) {
            report_move = true;
        } else if (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_VT200_HIGHLIGHT || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_BTN_EVENT) {
            if (current_buttons_state[0] || current_buttons_state[1] || current_buttons_state[2]) report_move = true;
        } else if (GET_SESSION(term)->mouse.sgr_mode || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_URXVT || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_PIXEL) {
             if (current_buttons_state[0] || current_buttons_state[1] || current_buttons_state[2]) report_move = true;
        }

        if (report_move) {
            mouse_report[0] = '\0';
            if (GET_SESSION(term)->mouse.sgr_mode || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_URXVT || GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_PIXEL) {
                if(current_buttons_state[0]) sgr_motion_code = 32;
                else if(current_buttons_state[1]) sgr_motion_code = 33;
                else if(current_buttons_state[2]) sgr_motion_code = 34;

                if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) sgr_motion_code += 4;
                if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) sgr_motion_code += 8;
                if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) sgr_motion_code += 16;

                if (GET_SESSION(term)->mouse.mode == MOUSE_TRACKING_PIXEL) {
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
            if (mouse_report[0]) QueueResponse(term, mouse_report);
        }
        GET_SESSION(term)->mouse.last_x = global_cell_x;
        GET_SESSION(term)->mouse.last_y = local_cell_y;
        GET_SESSION(term)->mouse.last_pixel_x = pixel_x;
        GET_SESSION(term)->mouse.last_pixel_y = local_pixel_y;
    }

    // Restore original active session context
    term->active_session = saved_session;

    // Handle focus changes (global or session specific? Focus usually window based)
    // We should probably report focus to the active session.
    // Since we restored session, we use (*GET_SESSION(term)) macro which now points to saved_session.

    if (GET_SESSION(term)->mouse.focus_tracking) {
        bool current_focus = SituationHasWindowFocus();
        if (current_focus && !GET_SESSION(term)->mouse.focused) {
            QueueResponse(term, "\x1B[I"); // Focus In
        } else if (!current_focus && GET_SESSION(term)->mouse.focused) {
            QueueResponse(term, "\x1B[O"); // Focus Out
        }
        GET_SESSION(term)->mouse.focused = current_focus;
    }
}



void SetKeyboardDialect(Terminal* term, int dialect) {
    if (dialect >= 1 && dialect <= 10) { // Example range, adjust per NRCS standards
        GET_SESSION(term)->vt_keyboard.keyboard_dialect = dialect;
    }
}

void SetPrinterAvailable(Terminal* term, bool available) {
    GET_SESSION(term)->printer_available = available;
}

void SetLocatorEnabled(Terminal* term, bool enabled) {
    GET_SESSION(term)->locator_enabled = enabled;
}

void SetUDKLocked(Terminal* term, bool locked) {
    GET_SESSION(term)->programmable_keys.udk_locked = locked;
}

void GetDeviceAttributes(Terminal* term, char* primary, char* secondary, size_t buffer_size) {
    if (primary) {
        strncpy(primary, GET_SESSION(term)->device_attributes, buffer_size - 1);
        primary[buffer_size - 1] = '\0';
    }
    if (secondary) {
        strncpy(secondary, GET_SESSION(term)->secondary_attributes, buffer_size - 1);
        secondary[buffer_size - 1] = '\0';
    }
}

int GetPipelineCount(Terminal* term) {
    return GET_SESSION(term)->pipeline_count;
}

bool IsPipelineOverflow(Terminal* term) {
    return GET_SESSION(term)->pipeline_overflow;
}

// Fix the stubs
void DefineRectangle(Terminal* term, int top, int left, int bottom, int right) {
    // Store rectangle definition for later operations
    (void)top; (void)left; (void)bottom; (void)right;
}

void ExecuteRectangularOperation(Terminal* term, RectOperation op, const EnhancedTermChar* fill_char) {
    (void)op; (void)fill_char;
}

void SelectCharacterSet(Terminal* term, int gset, CharacterSet charset) {
    switch (gset) {
        case 0: GET_SESSION(term)->charset.g0 = charset; break;
        case 1: GET_SESSION(term)->charset.g1 = charset; break;
        case 2: GET_SESSION(term)->charset.g2 = charset; break;
        case 3: GET_SESSION(term)->charset.g3 = charset; break;
    }
}

void SetCharacterSet(Terminal* term, CharacterSet charset) {
    GET_SESSION(term)->charset.g0 = charset;
    GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g0;
}

void LoadSoftFont(Terminal* term, const unsigned char* font_data, int char_start, int char_count) {
    (void)font_data; (void)char_start; (void)char_count;
    // Soft font loading not fully implemented
}

void SelectSoftFont(Terminal* term, bool enable) {
    GET_SESSION(term)->soft_font.active = enable;
}

void SetKeyboardMode(Terminal* term, const char* mode, bool enable) {
    if (strcmp(mode, "application") == 0) {
        GET_SESSION(term)->vt_keyboard.application_mode = enable;
    } else if (strcmp(mode, "cursor") == 0) {
        GET_SESSION(term)->vt_keyboard.cursor_key_mode = enable;
    } else if (strcmp(mode, "keypad") == 0) {
        GET_SESSION(term)->vt_keyboard.keypad_mode = enable;
    } else if (strcmp(mode, "meta_escape") == 0) {
        GET_SESSION(term)->vt_keyboard.meta_sends_escape = enable;
    }
}

void DefineFunctionKey(Terminal* term, int key_num, const char* sequence) {
    if (key_num >= 1 && key_num <= 24 && sequence) {
        strncpy(GET_SESSION(term)->vt_keyboard.function_keys[key_num - 1], sequence, 31);
        GET_SESSION(term)->vt_keyboard.function_keys[key_num - 1][31] = '\0';
    }
}


void HandleControlKey(Terminal* term, VTKeyEvent* event) {
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

void HandleAltKey(Terminal* term, VTKeyEvent* event) {
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

void GenerateVTSequence(Terminal* term, VTKeyEvent* event) {
    // Clear sequence
    memset(event->sequence, 0, sizeof(event->sequence));

    // Handle special keys first
    switch (event->key_code) {
        // Arrow keys
        case SIT_KEY_UP:
            if (GET_SESSION(term)->vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOA"); // Application mode
            } else {
                strcpy(event->sequence, "\x1B[A"); // Normal mode
            }
            break;

        case SIT_KEY_DOWN:
            if (GET_SESSION(term)->vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOB");
            } else {
                strcpy(event->sequence, "\x1B[B");
            }
            break;

        case SIT_KEY_RIGHT:
            if (GET_SESSION(term)->vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOC");
            } else {
                strcpy(event->sequence, "\x1B[C");
            }
            break;

        case SIT_KEY_LEFT:
            if (GET_SESSION(term)->vt_keyboard.cursor_key_mode) {
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
            if (GET_SESSION(term)->vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOH");
            } else {
                strcpy(event->sequence, "\x1B[H");
            }
            break;

        case SIT_KEY_END:
            if (GET_SESSION(term)->vt_keyboard.cursor_key_mode) {
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
            if (GET_SESSION(term)->vt_keyboard.keypad_mode) {
                snprintf(event->sequence, sizeof(event->sequence), "\x1BO%c",
                        'p' + (event->key_code - SIT_KEY_KP_0));
            } else {
                snprintf(event->sequence, sizeof(event->sequence), "%c",
                        '0' + (event->key_code - SIT_KEY_KP_0));
            }
            break;

        case SIT_KEY_KP_DECIMAL:
            strcpy(event->sequence, GET_SESSION(term)->vt_keyboard.keypad_mode ? "\x1BOn" : ".");
            break;

        case SIT_KEY_KP_ENTER:
            strcpy(event->sequence, GET_SESSION(term)->vt_keyboard.keypad_mode ? "\x1BOM" : "\r");
            break;

        case SIT_KEY_KP_ADD:
            strcpy(event->sequence, GET_SESSION(term)->vt_keyboard.keypad_mode ? "\x1BOk" : "+");
            break;

        case SIT_KEY_KP_SUBTRACT:
            strcpy(event->sequence, GET_SESSION(term)->vt_keyboard.keypad_mode ? "\x1BOm" : "-");
            break;

        case SIT_KEY_KP_MULTIPLY:
            strcpy(event->sequence, GET_SESSION(term)->vt_keyboard.keypad_mode ? "\x1BOj" : "*");
            break;

        case SIT_KEY_KP_DIVIDE:
            strcpy(event->sequence, GET_SESSION(term)->vt_keyboard.keypad_mode ? "\x1BOo" : "/");
            break;

        default:
            // Handle regular keys with modifiers
            if (event->ctrl) {
                HandleControlKey(term, event);
            } else if (event->alt && GET_SESSION(term)->vt_keyboard.meta_sends_escape) {
                HandleAltKey(term, event);
            } else {
                // Regular character - will be handled by GetCharPressed
                event->sequence[0] = '\0';
            }
            break;
    }
}

// Internal function to process keyboard input and enqueue events

// Internal function to process keyboard input and enqueue events
void UpdateVTKeyboard(Terminal* term) {
    double current_time = SituationTimerGetTime();

    // Process Situation key presses - SKIP PRINTABLE ASCII KEYS
    int rk;
    while ((rk = SituationGetKeyPressed()) != 0) {
        // First, check if this key is a User-Defined Key (UDK)
        bool udk_found = false;
        for (size_t i = 0; i < GET_SESSION(term)->programmable_keys.count; i++) {
            if (GET_SESSION(term)->programmable_keys.keys[i].key_code == rk && GET_SESSION(term)->programmable_keys.keys[i].active) {
                if (GET_SESSION(term)->vt_keyboard.buffer_count < KEY_EVENT_BUFFER_SIZE) {
                    VTKeyEvent* vt_event = &GET_SESSION(term)->vt_keyboard.buffer[GET_SESSION(term)->vt_keyboard.buffer_head];
                    memset(vt_event, 0, sizeof(VTKeyEvent));
                    vt_event->key_code = rk;
                    vt_event->timestamp = current_time;
                    vt_event->priority = KEY_PRIORITY_HIGH; // UDKs get high priority

                    // Copy the user-defined sequence
                    size_t len = GET_SESSION(term)->programmable_keys.keys[i].sequence_length;
                    if (len >= sizeof(vt_event->sequence)) {
                        len = sizeof(vt_event->sequence) - 1;
                    }
                    memcpy(vt_event->sequence, GET_SESSION(term)->programmable_keys.keys[i].sequence, len);
                    vt_event->sequence[len] = '\0';

                    GET_SESSION(term)->vt_keyboard.buffer_head = (GET_SESSION(term)->vt_keyboard.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
                    GET_SESSION(term)->vt_keyboard.buffer_count++;
                    GET_SESSION(term)->vt_keyboard.total_events++;
                    udk_found = true;
                } else {
                    GET_SESSION(term)->vt_keyboard.dropped_events++;
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

        if (GET_SESSION(term)->vt_keyboard.buffer_count < KEY_EVENT_BUFFER_SIZE) {
            VTKeyEvent* vt_event = &GET_SESSION(term)->vt_keyboard.buffer[GET_SESSION(term)->vt_keyboard.buffer_head];
            memset(vt_event, 0, sizeof(VTKeyEvent));
            vt_event->key_code = rk;
            vt_event->timestamp = current_time;
            vt_event->priority = KEY_PRIORITY_NORMAL;
            vt_event->ctrl = ctrl;
            vt_event->shift = SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT);
            vt_event->alt = alt;

            // Special handling for printable keys with modifiers
            if (rk >= 32 && rk <= 126) {
                if (ctrl) HandleControlKey(term, vt_event);
                else if (alt) HandleAltKey(term, vt_event);
            }
            else {
                // Handle Scrollback (Shift + PageUp/Down) - Local Action, No Sequence
                if (vt_event->shift && (rk == SIT_KEY_PAGE_UP || rk == SIT_KEY_PAGE_DOWN)) {
                    if (rk == SIT_KEY_PAGE_UP) {
                        GET_SESSION(term)->view_offset += DEFAULT_TERM_HEIGHT / 2;
                    } else {
                        GET_SESSION(term)->view_offset -= DEFAULT_TERM_HEIGHT / 2;
                    }

                    // Clamp view_offset
                    if (GET_SESSION(term)->view_offset < 0) GET_SESSION(term)->view_offset = 0;
                    int max_offset = GET_SESSION(term)->buffer_height - DEFAULT_TERM_HEIGHT;
                    if (GET_SESSION(term)->view_offset > max_offset) GET_SESSION(term)->view_offset = max_offset;

                    // Invalidate screen to redraw with new offset
                    for (int i=0; i<DEFAULT_TERM_HEIGHT; i++) GET_SESSION(term)->row_dirty[i] = true;

                    // Do NOT increment buffer_count, we consumed the event locally.
                    continue;
                }

                // Generate VT sequence for special keys only
                switch (rk) {
                case SIT_KEY_UP:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), GET_SESSION(term)->vt_keyboard.cursor_key_mode ? "\x1BOA" : "\x1B[A");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5A");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3A");
                    break;
                case SIT_KEY_DOWN:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), GET_SESSION(term)->vt_keyboard.cursor_key_mode ? "\x1BOB" : "\x1B[B");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5B");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3B");
                    break;
                case SIT_KEY_RIGHT:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), GET_SESSION(term)->vt_keyboard.cursor_key_mode ? "\x1BOC" : "\x1B[C");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5C");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3C");
                    break;
                case SIT_KEY_LEFT:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), GET_SESSION(term)->vt_keyboard.cursor_key_mode ? "\x1BOD" : "\x1B[D");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5D");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3D");
                    break;
                case SIT_KEY_F1: case SIT_KEY_F2: case SIT_KEY_F3: case SIT_KEY_F4:
                case SIT_KEY_F5: case SIT_KEY_F6: case SIT_KEY_F7: case SIT_KEY_F8:
                case SIT_KEY_F9: case SIT_KEY_F10: case SIT_KEY_F11: case SIT_KEY_F12:
                    strncpy(vt_event->sequence, GET_SESSION(term)->vt_keyboard.function_keys[rk - SIT_KEY_F1], sizeof(vt_event->sequence));
                    break;
                case SIT_KEY_ENTER:
                    vt_event->sequence[0] = GET_SESSION(term)->ansi_modes.line_feed_new_line ? '\r' : '\n';
                    vt_event->sequence[1] = '\0';
                    break;
                case SIT_KEY_BACKSPACE:
                    vt_event->sequence[0] = GET_SESSION(term)->vt_keyboard.backarrow_sends_bs ? '\b' : '\x7F';
                    vt_event->sequence[1] = '\0';
                    break;
                case SIT_KEY_DELETE:
                    vt_event->sequence[0] = GET_SESSION(term)->vt_keyboard.delete_sends_del ? '\x7F' : '\b';
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
                GET_SESSION(term)->vt_keyboard.buffer_head = (GET_SESSION(term)->vt_keyboard.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
                GET_SESSION(term)->vt_keyboard.buffer_count++;
                GET_SESSION(term)->vt_keyboard.total_events++;
            }
        } else {
            GET_SESSION(term)->vt_keyboard.dropped_events++;
        }
    }

    // Process Unicode characters - THIS HANDLES ALL PRINTABLE CHARACTERS
    int ch_unicode;
    while ((ch_unicode = SituationGetCharPressed()) != 0) {
        if (GET_SESSION(term)->vt_keyboard.buffer_count < KEY_EVENT_BUFFER_SIZE) {
            VTKeyEvent* vt_event = &GET_SESSION(term)->vt_keyboard.buffer[GET_SESSION(term)->vt_keyboard.buffer_head];
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
            } else if (vt_event->alt && GET_SESSION(term)->vt_keyboard.meta_sends_escape && !vt_event->ctrl) {
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
                GET_SESSION(term)->vt_keyboard.buffer_head = (GET_SESSION(term)->vt_keyboard.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
                GET_SESSION(term)->vt_keyboard.buffer_count++;
                GET_SESSION(term)->vt_keyboard.total_events++;
            }
        } else {
            GET_SESSION(term)->vt_keyboard.dropped_events++;
        }
    }
}


bool GetKeyEvent(Terminal* term, KeyEvent* event) {
    return GetVTKeyEvent(term, event);
}

void SetPipelineTargetFPS(Terminal* term, int fps) {
    if (fps > 0) {
        GET_SESSION(term)->VTperformance.target_frame_time = 1.0 / fps;
        GET_SESSION(term)->VTperformance.time_budget = GET_SESSION(term)->VTperformance.target_frame_time * 0.3;
    }
}

void SetPipelineTimeBudget(Terminal* term, double pct) {
    if (pct > 0.0 && pct <= 1.0) {
        GET_SESSION(term)->VTperformance.time_budget = GET_SESSION(term)->VTperformance.target_frame_time * pct;
    }
}

TerminalStatus GetTerminalStatus(Terminal* term) {
    TerminalStatus status = {0};
    status.pipeline_usage = GET_SESSION(term)->pipeline_count;
    status.key_usage = GET_SESSION(term)->vt_keyboard.buffer_count;
    status.overflow_detected = GET_SESSION(term)->pipeline_overflow;
    status.avg_process_time = GET_SESSION(term)->VTperformance.avg_process_time;
    return status;
}

void ShowBufferDiagnostics(Terminal* term) {
    TerminalStatus status = GetTerminalStatus(term);
    PipelineWriteFormat(term, "=== Buffer Diagnostics ===\n");
    PipelineWriteFormat(term, "Pipeline: %zu/%d bytes\n", status.pipeline_usage, (int)sizeof(GET_SESSION(term)->input_pipeline));
    PipelineWriteFormat(term, "Keyboard: %zu events\n", status.key_usage);
    PipelineWriteFormat(term, "Overflow: %s\n", status.overflow_detected ? "YES" : "No");
    PipelineWriteFormat(term, "Avg Process Time: %.6f ms\n", status.avg_process_time * 1000.0);
}

void VTSwapScreenBuffer(Terminal* term) {
    // Swap pointers
    EnhancedTermChar* temp_buf = GET_SESSION(term)->screen_buffer;
    GET_SESSION(term)->screen_buffer = GET_SESSION(term)->alt_buffer;
    GET_SESSION(term)->alt_buffer = temp_buf;

    // Swap dimensions/metadata
    // For now, only buffer_height differs (Main has scrollback, Alt usually doesn't).
    // But since we allocate Alt buffer with DEFAULT_TERM_HEIGHT, we must handle this.
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
        GET_SESSION(term)->buffer_height = DEFAULT_TERM_HEIGHT + MAX_SCROLLBACK_LINES;
        GET_SESSION(term)->dec_modes.alternate_screen = false;

        // Restore view offset (if we want to restore scroll position, otherwise 0)
        // For now, reset to 0 (bottom) is safest and standard behavior.
        // Or restore saved? Let's restore saved for better UX.
        GET_SESSION(term)->view_offset = GET_SESSION(term)->saved_view_offset;
    } else {
        // Switching TO Alternate Screen
        GET_SESSION(term)->buffer_height = DEFAULT_TERM_HEIGHT;
        GET_SESSION(term)->dec_modes.alternate_screen = true;

        // Save current offset and reset view for Alt screen (which has no scrollback)
        GET_SESSION(term)->saved_view_offset = GET_SESSION(term)->view_offset;
        GET_SESSION(term)->view_offset = 0;
    }

    // Force full redraw
    for (int i=0; i<DEFAULT_TERM_HEIGHT; i++) {
        GET_SESSION(term)->row_dirty[i] = true;
    }
}

void ProcessPipeline(Terminal* term) {
    if (GET_SESSION(term)->pipeline_count == 0) {
        return;
    }

    double start_time = SituationTimerGetTime();
    int chars_processed = 0;
    int target_chars = GET_SESSION(term)->VTperformance.chars_per_frame;

    // Adaptive processing based on buffer level
    if (GET_SESSION(term)->pipeline_count > GET_SESSION(term)->VTperformance.burst_threshold) {
        target_chars *= 2; // Burst mode
        GET_SESSION(term)->VTperformance.burst_mode = true;
    } else if (GET_SESSION(term)->pipeline_count < target_chars) {
        target_chars = GET_SESSION(term)->pipeline_count; // Process all remaining
        GET_SESSION(term)->VTperformance.burst_mode = false;
    }

    while (chars_processed < target_chars && GET_SESSION(term)->pipeline_count > 0) {
        // Check time budget
        if (SituationTimerGetTime() - start_time > GET_SESSION(term)->VTperformance.time_budget) {
            break;
        }

        unsigned char ch = GET_SESSION(term)->input_pipeline[GET_SESSION(term)->pipeline_tail];
        GET_SESSION(term)->pipeline_tail = (GET_SESSION(term)->pipeline_tail + 1) % sizeof(GET_SESSION(term)->input_pipeline);
        GET_SESSION(term)->pipeline_count--;

        ProcessChar(term, ch);
        chars_processed++;
    }

    // Update performance metrics
    if (chars_processed > 0) {
        double total_time = SituationTimerGetTime() - start_time;
        double time_per_char = total_time / chars_processed;
        GET_SESSION(term)->VTperformance.avg_process_time =
            GET_SESSION(term)->VTperformance.avg_process_time * 0.9 + time_per_char * 0.1;
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void LogUnsupportedSequence(Terminal* term, const char* sequence) {
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

int ParseCSIParams(Terminal* term, const char* params, int* out_params, int max_params) {
    GET_SESSION(term)->param_count = 0;
    memset(GET_SESSION(term)->escape_params, 0, sizeof(GET_SESSION(term)->escape_params));

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

    while (token != NULL && GET_SESSION(term)->param_count < max_params) {
        if (strlen(token) == 0) {
            GET_SESSION(term)->escape_params[GET_SESSION(term)->param_count] = 0;
        } else {
            int value = atoi(token);
            GET_SESSION(term)->escape_params[GET_SESSION(term)->param_count] = (value >= 0) ? value : 0;
        }
        if (out_params) {
            out_params[GET_SESSION(term)->param_count] = GET_SESSION(term)->escape_params[GET_SESSION(term)->param_count];
        }
        GET_SESSION(term)->param_count++;
        token = strtok_r(NULL, ";", &saveptr);
    }
    return GET_SESSION(term)->param_count;
}

static void ClearCSIParams(Terminal* term) {
    GET_SESSION(term)->escape_buffer[0] = '\0';
    GET_SESSION(term)->escape_pos = 0;
    GET_SESSION(term)->param_count = 0;
    memset(GET_SESSION(term)->escape_params, 0, sizeof(GET_SESSION(term)->escape_params));
}

void ProcessSixelSTChar(Terminal* term, unsigned char ch) {
    if (ch == '\\') { // This is ST
        GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
        // Finalize sixel image size
        GET_SESSION(term)->sixel.width = GET_SESSION(term)->sixel.max_x;
        GET_SESSION(term)->sixel.height = GET_SESSION(term)->sixel.max_y;
        GET_SESSION(term)->sixel.dirty = true;
    } else {
        // ESC was start of new sequence
        // We treat the current char 'ch' as the one following ESC.
        // e.g. ESC P -> ch='P'. ProcessEscapeChar(term, 'P') -> PARSE_DCS.
        ProcessEscapeChar(term, ch);
    }
}

int GetCSIParam(Terminal* term, int index, int default_value) {
    if (index >= 0 && index < GET_SESSION(term)->param_count) {
        return (GET_SESSION(term)->escape_params[index] == 0) ? default_value : GET_SESSION(term)->escape_params[index];
    }
    return default_value;
}


// =============================================================================
// CURSOR MOVEMENT IMPLEMENTATIONS
// =============================================================================

void ExecuteCUU(Terminal* term) { // Cursor Up
    int n = GetCSIParam(term, 0, 1);
    int new_y = GET_SESSION(term)->cursor.y - n;

    if (GET_SESSION(term)->dec_modes.origin_mode) {
        GET_SESSION(term)->cursor.y = (new_y < GET_SESSION(term)->scroll_top) ? GET_SESSION(term)->scroll_top : new_y;
    } else {
        GET_SESSION(term)->cursor.y = (new_y < 0) ? 0 : new_y;
    }
}

void ExecuteCUD(Terminal* term) { // Cursor Down
    int n = GetCSIParam(term, 0, 1);
    int new_y = GET_SESSION(term)->cursor.y + n;

    if (GET_SESSION(term)->dec_modes.origin_mode) {
        GET_SESSION(term)->cursor.y = (new_y > GET_SESSION(term)->scroll_bottom) ? GET_SESSION(term)->scroll_bottom : new_y;
    } else {
        GET_SESSION(term)->cursor.y = (new_y >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : new_y;
    }
}

void ExecuteCUF(Terminal* term) { // Cursor Forward
    int n = GetCSIParam(term, 0, 1);
    GET_SESSION(term)->cursor.x = (GET_SESSION(term)->cursor.x + n >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : GET_SESSION(term)->cursor.x + n;
}

void ExecuteCUB(Terminal* term) { // Cursor Back
    int n = GetCSIParam(term, 0, 1);
    GET_SESSION(term)->cursor.x = (GET_SESSION(term)->cursor.x - n < 0) ? 0 : GET_SESSION(term)->cursor.x - n;
}

void ExecuteCNL(Terminal* term) { // Cursor Next Line
    int n = GetCSIParam(term, 0, 1);
    GET_SESSION(term)->cursor.y = (GET_SESSION(term)->cursor.y + n >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : GET_SESSION(term)->cursor.y + n;
    GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
}

void ExecuteCPL(Terminal* term) { // Cursor Previous Line
    int n = GetCSIParam(term, 0, 1);
    GET_SESSION(term)->cursor.y = (GET_SESSION(term)->cursor.y - n < 0) ? 0 : GET_SESSION(term)->cursor.y - n;
    GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
}

void ExecuteCHA(Terminal* term) { // Cursor Horizontal Absolute
    int n = GetCSIParam(term, 0, 1) - 1; // Convert to 0-based
    GET_SESSION(term)->cursor.x = (n < 0) ? 0 : (n >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : n;
}

void ExecuteCUP(Terminal* term) { // Cursor Position
    int row = GetCSIParam(term, 0, 1) - 1; // Convert to 0-based
    int col = GetCSIParam(term, 1, 1) - 1;

    if (GET_SESSION(term)->dec_modes.origin_mode) {
        row += GET_SESSION(term)->scroll_top;
        col += GET_SESSION(term)->left_margin;
    }

    GET_SESSION(term)->cursor.y = (row < 0) ? 0 : (row >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : row;
    GET_SESSION(term)->cursor.x = (col < 0) ? 0 : (col >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : col;

    // Clamp to scrolling region if in origin mode
    if (GET_SESSION(term)->dec_modes.origin_mode) {
        if (GET_SESSION(term)->cursor.y < GET_SESSION(term)->scroll_top) GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_top;
        if (GET_SESSION(term)->cursor.y > GET_SESSION(term)->scroll_bottom) GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_bottom;
        if (GET_SESSION(term)->cursor.x < GET_SESSION(term)->left_margin) GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
        if (GET_SESSION(term)->cursor.x > GET_SESSION(term)->right_margin) GET_SESSION(term)->cursor.x = GET_SESSION(term)->right_margin;
    }
}

void ExecuteVPA(Terminal* term) { // Vertical Position Absolute
    int n = GetCSIParam(term, 0, 1) - 1; // Convert to 0-based

    if (GET_SESSION(term)->dec_modes.origin_mode) {
        n += GET_SESSION(term)->scroll_top;
        GET_SESSION(term)->cursor.y = (n < GET_SESSION(term)->scroll_top) ? GET_SESSION(term)->scroll_top :
                           (n > GET_SESSION(term)->scroll_bottom) ? GET_SESSION(term)->scroll_bottom : n;
    } else {
        GET_SESSION(term)->cursor.y = (n < 0) ? 0 : (n >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : n;
    }
}

// =============================================================================
// ERASING IMPLEMENTATIONS
// =============================================================================

void ExecuteED(Terminal* term, bool private_mode) { // Erase in Display
    int n = GetCSIParam(term, 0, 0);

    switch (n) {
        case 0: // Clear from cursor to end of screen
            // Clear current line from cursor
            for (int x = GET_SESSION(term)->cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(term, cell);
            }
            // Clear remaining lines
            for (int y = GET_SESSION(term)->cursor.y + 1; y < DEFAULT_TERM_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
                    if (private_mode && cell->protected_cell) continue;
                    ClearCell(term, cell);
                }
            }
            break;

        case 1: // Clear from beginning of screen to cursor
            // Clear lines before cursor
            for (int y = 0; y < GET_SESSION(term)->cursor.y; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
                    if (private_mode && cell->protected_cell) continue;
                    ClearCell(term, cell);
                }
            }
            // Clear current line up to cursor
            for (int x = 0; x <= GET_SESSION(term)->cursor.x; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(term, cell);
            }
            break;

        case 2: // Clear entire screen
        case 3: // Clear entire screen and scrollback (xterm extension)
            for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
                    if (private_mode && cell->protected_cell) continue;
                    ClearCell(term, cell);
                }
            }
            break;

        default:
            LogUnsupportedSequence(term, "Unknown ED parameter");
            break;
    }
}

void ExecuteEL(Terminal* term, bool private_mode) { // Erase in Line
    int n = GetCSIParam(term, 0, 0);

    switch (n) {
        case 0: // Clear from cursor to end of line
            for (int x = GET_SESSION(term)->cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(term, cell);
            }
            break;

        case 1: // Clear from beginning of line to cursor
            for (int x = 0; x <= GET_SESSION(term)->cursor.x; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(term, cell);
            }
            break;

        case 2: // Clear entire line
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x);
                if (private_mode && cell->protected_cell) continue;
                ClearCell(term, cell);
            }
            break;

        default:
            LogUnsupportedSequence(term, "Unknown EL parameter");
            break;
    }
}

void ExecuteECH(Terminal* term) { // Erase Character
    int n = GetCSIParam(term, 0, 1);

    for (int i = 0; i < n && GET_SESSION(term)->cursor.x + i < DEFAULT_TERM_WIDTH; i++) {
        ClearCell(term, GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, GET_SESSION(term)->cursor.x + i));
    }
}

// =============================================================================
// INSERTION AND DELETION IMPLEMENTATIONS
// =============================================================================

void ExecuteIL(Terminal* term) { // Insert Line
    int n = GetCSIParam(term, 0, 1);
    InsertLinesAt(term, GET_SESSION(term)->cursor.y, n);
}

void ExecuteDL(Terminal* term) { // Delete Line
    int n = GetCSIParam(term, 0, 1);
    DeleteLinesAt(term, GET_SESSION(term)->cursor.y, n);
}

void ExecuteICH(Terminal* term) { // Insert Character
    int n = GetCSIParam(term, 0, 1);
    InsertCharactersAt(term, GET_SESSION(term)->cursor.y, GET_SESSION(term)->cursor.x, n);
}

void ExecuteDCH(Terminal* term) { // Delete Character
    int n = GetCSIParam(term, 0, 1);
    DeleteCharactersAt(term, GET_SESSION(term)->cursor.y, GET_SESSION(term)->cursor.x, n);
}

void ExecuteREP(Terminal* term) { // Repeat Preceding Graphic Character
    int n = GetCSIParam(term, 0, 1);
    if (n < 1) n = 1;
    if (GET_SESSION(term)->last_char > 0) {
        for (int i = 0; i < n; i++) {
            if (GET_SESSION(term)->cursor.x > GET_SESSION(term)->right_margin) {
                if (GET_SESSION(term)->dec_modes.auto_wrap_mode) {
                    GET_SESSION(term)->cursor.x = GET_SESSION(term)->left_margin;
                    GET_SESSION(term)->cursor.y++;
                    if (GET_SESSION(term)->cursor.y > GET_SESSION(term)->scroll_bottom) {
                        GET_SESSION(term)->cursor.y = GET_SESSION(term)->scroll_bottom;
                        ScrollUpRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, 1);
                    }
                } else {
                    GET_SESSION(term)->cursor.x = GET_SESSION(term)->right_margin;
                }
            }
            InsertCharacterAtCursor(term, GET_SESSION(term)->last_char);
            GET_SESSION(term)->cursor.x++;
        }
    }
}

// =============================================================================
// SCROLLING IMPLEMENTATIONS
// =============================================================================

void ExecuteSU(Terminal* term) { // Scroll Up
    int n = GetCSIParam(term, 0, 1);
    ScrollUpRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, n);
}

void ExecuteSD(Terminal* term) { // Scroll Down
    int n = GetCSIParam(term, 0, 1);
    ScrollDownRegion(term, GET_SESSION(term)->scroll_top, GET_SESSION(term)->scroll_bottom, n);
}

// =============================================================================
// ENHANCED SGR (SELECT GRAPHIC RENDITION) IMPLEMENTATION
// =============================================================================

int ProcessExtendedColor(Terminal* term, ExtendedColor* color, int param_index) {
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
            color->value.rgb = (RGB_Color){r, g, b, 255};
            consumed = 4;
        }
    }

    return consumed;
}

void ResetAllAttributes(Terminal* term) {
    GET_SESSION(term)->current_fg.color_mode = 0;
    GET_SESSION(term)->current_fg.value.index = COLOR_WHITE;
    GET_SESSION(term)->current_bg.color_mode = 0;
    GET_SESSION(term)->current_bg.value.index = COLOR_BLACK;

    GET_SESSION(term)->bold_mode = false;
    GET_SESSION(term)->faint_mode = false;
    GET_SESSION(term)->italic_mode = false;
    GET_SESSION(term)->underline_mode = false;
    GET_SESSION(term)->blink_mode = false;
    GET_SESSION(term)->reverse_mode = false;
    GET_SESSION(term)->strikethrough_mode = false;
    GET_SESSION(term)->conceal_mode = false;
    GET_SESSION(term)->overline_mode = false;
    GET_SESSION(term)->double_underline_mode = false;
    GET_SESSION(term)->protected_mode = false;
}

void ExecuteSGR(Terminal* term) {
    if (GET_SESSION(term)->param_count == 0) {
        // Reset all attributes
        ResetAllAttributes(term);
        return;
    }

    for (int i = 0; i < GET_SESSION(term)->param_count; i++) {
        int param = GET_SESSION(term)->escape_params[i];

        switch (param) {
            case 0: // Reset all
                ResetAllAttributes(term);
                break;

            // Intensity
            case 1: GET_SESSION(term)->bold_mode = true; break;
            case 2: GET_SESSION(term)->faint_mode = true; break;
            case 22: GET_SESSION(term)->bold_mode = GET_SESSION(term)->faint_mode = false; break;

            // Style
            case 3: GET_SESSION(term)->italic_mode = true; break;
            case 23: GET_SESSION(term)->italic_mode = false; break;

            case 4: GET_SESSION(term)->underline_mode = true; break;
            case 21: GET_SESSION(term)->double_underline_mode = true; break;
            case 24: GET_SESSION(term)->underline_mode = GET_SESSION(term)->double_underline_mode = false; break;

            case 5: case 6: GET_SESSION(term)->blink_mode = true; break;
            case 25: GET_SESSION(term)->blink_mode = false; break;

            case 7: GET_SESSION(term)->reverse_mode = true; break;
            case 27: GET_SESSION(term)->reverse_mode = false; break;

            case 8: GET_SESSION(term)->conceal_mode = true; break;
            case 28: GET_SESSION(term)->conceal_mode = false; break;

            case 9: GET_SESSION(term)->strikethrough_mode = true; break;
            case 29: GET_SESSION(term)->strikethrough_mode = false; break;

            case 53: GET_SESSION(term)->overline_mode = true; break;
            case 55: GET_SESSION(term)->overline_mode = false; break;

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
                GET_SESSION(term)->current_fg.color_mode = 0;
                GET_SESSION(term)->current_fg.value.index = param - 90 + 8;
                break;

            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                GET_SESSION(term)->current_bg.color_mode = 0;
                GET_SESSION(term)->current_bg.value.index = param - 100 + 8;
                break;

            // Extended colors
            case 38: // Set foreground color
                i += ProcessExtendedColor(term, &GET_SESSION(term)->current_fg, i);
                break;

            case 48: // Set background color
                i += ProcessExtendedColor(term, &GET_SESSION(term)->current_bg, i);
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
                    LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    }
}

// =============================================================================
// TERMINAL MODE IMPLEMENTATIONS
// =============================================================================

// Helper function to compute screen buffer checksum (for CSI ?63 n)
static uint32_t ComputeScreenChecksum(Terminal* term, int page) {
    uint32_t checksum = 0;
    // Simple CRC16-like checksum for screen buffer
    for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
        for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
            EnhancedTermChar *cell = GetScreenCell(GET_SESSION(term), y, x);
            checksum += cell->ch;
            checksum += (cell->fg_color.color_mode == 0 ? cell->fg_color.value.index : (cell->fg_color.value.rgb.r << 16 | cell->fg_color.value.rgb.g << 8 | cell->fg_color.value.rgb.b));
            checksum += (cell->bg_color.color_mode == 0 ? cell->bg_color.value.index : (cell->bg_color.value.rgb.r << 16 | cell->bg_color.value.rgb.g << 8 | cell->bg_color.value.rgb.b));
            checksum = (checksum >> 16) + (checksum & 0xFFFF);
        }
    }
    return checksum & 0xFFFF;
}

void SwitchScreenBuffer(Terminal* term, bool to_alternate) {
    if (!GET_SESSION(term)->conformance.features.alternate_screen) {
        LogUnsupportedSequence(term, "Alternate screen not supported");
        return;
    }

    // In new Ring Buffer architecture, we swap buffers rather than copy.
    VTSwapScreenBuffer(term);
    // VTSwapScreenBuffer handles logic if implemented correctly.
    // However, this function `SwitchScreenBuffer` seems to enforce explicit "to_alternate" direction.
    // We should implement it using pointers.

    if (to_alternate && !GET_SESSION(term)->dec_modes.alternate_screen) {
        VTSwapScreenBuffer(term); // Swaps to alt
    } else if (!to_alternate && GET_SESSION(term)->dec_modes.alternate_screen) {
        VTSwapScreenBuffer(term); // Swaps back to main
    }
}

// Set terminal modes internally
// Configures DEC private modes (CSI ? Pm h/l) and ANSI modes (CSI Pm h/l)
static void SetTerminalModeInternal(Terminal* term, int mode, bool enable, bool private_mode) {
    if (private_mode) {
        // DEC Private Modes
        switch (mode) {
            case 1: // DECCKM - Cursor Key Mode
                // Enable/disable application cursor keys
                GET_SESSION(term)->dec_modes.application_cursor_keys = enable;
                GET_SESSION(term)->vt_keyboard.cursor_key_mode = enable;
                break;

            case 2: // DECANM - ANSI Mode
                // Switch between VT52 and ANSI mode
                if (!enable && GET_SESSION(term)->conformance.features.vt52_mode) {
                    GET_SESSION(term)->parse_state = PARSE_VT52;
                }
                break;

            case 3: // DECCOLM - Column Mode
                // Set 132-column mode
                // Note: Actual window resizing is host-dependent and not handled here.
                // Standard requires clearing screen, resetting margins, and homing cursor.
                if (GET_SESSION(term)->dec_modes.column_mode_132 != enable) {
                    GET_SESSION(term)->dec_modes.column_mode_132 = enable;

                    // 1. Clear Screen
                    for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                        for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                            ClearCell(term, GetScreenCell(GET_SESSION(term), y, x));
                        }
                        GET_SESSION(term)->row_dirty[y] = true;
                    }

                    // 2. Reset Margins
                    GET_SESSION(term)->scroll_top = 0;
                    GET_SESSION(term)->scroll_bottom = DEFAULT_TERM_HEIGHT - 1;
                    GET_SESSION(term)->left_margin = 0;
                    // Set right margin (132 columns = index 131, 80 columns = index 79)
                    GET_SESSION(term)->right_margin = enable ? 131 : 79;
                    // Safety clamp if DEFAULT_TERM_WIDTH < 132
                    if (GET_SESSION(term)->right_margin >= DEFAULT_TERM_WIDTH) GET_SESSION(term)->right_margin = DEFAULT_TERM_WIDTH - 1;

                    // 3. Home Cursor
                    GET_SESSION(term)->cursor.x = 0;
                    GET_SESSION(term)->cursor.y = 0;
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
                if (GET_SESSION(term)->options.debug_sequences) {
                    LogUnsupportedSequence(term, "80/132 Column Mode Switch Requested (Resize unsupported)");
                }
                break;

            case 47: // Alternate Screen Buffer
            case 1047: // Alternate Screen Buffer (xterm)
                // Switch between main and alternate screen buffers
                SwitchScreenBuffer(term, enable);
                break;

            case 1048: // Save/Restore Cursor
                // Save or restore cursor state
                if (enable) {
                    ExecuteSaveCursor(term);
                } else {
                    ExecuteRestoreCursor(term);
                }
                break;

            case 1049: // Alternate Screen + Save/Restore Cursor
                // Save/restore cursor and switch screen buffer
                if (enable) {
                    ExecuteSaveCursor(term);
                    SwitchScreenBuffer(term, true);
                    ExecuteED(term, false); // Clear screen
                    GET_SESSION(term)->cursor.x = 0;
                    GET_SESSION(term)->cursor.y = 0;
                } else {
                    SwitchScreenBuffer(term, false);
                    ExecuteRestoreCursor(term);
                }
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
                    LogUnsupportedSequence(term, debug_msg);
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

            default:
                // Log unsupported ANSI modes
                if (GET_SESSION(term)->options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown ANSI mode: %d", mode);
                    LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    }
}

// Set terminal modes (CSI Pm h or CSI ? Pm h)
// Enables specified modes, including mouse tracking and focus reporting
static void ExecuteSM(Terminal* term, bool private_mode) {
    // Iterate through parsed parameters from the CSI sequence
    for (int i = 0; i < GET_SESSION(term)->param_count; i++) {
        int mode = GET_SESSION(term)->escape_params[i];
        if (private_mode) {
            switch (mode) {
                case 1000: // VT200 mouse tracking
                    EnableMouseFeature(term, "cursor", true);
                    GET_SESSION(term)->mouse.mode = GET_SESSION(term)->mouse.sgr_mode ? MOUSE_TRACKING_SGR : MOUSE_TRACKING_VT200;
                    break;
                case 1002: // Button-event mouse tracking
                    EnableMouseFeature(term, "cursor", true);
                    GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_BTN_EVENT;
                    break;
                case 1003: // Any-event mouse tracking
                    EnableMouseFeature(term, "cursor", true);
                    GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_ANY_EVENT;
                    break;
                case 1004: // Focus tracking
                    EnableMouseFeature(term, "focus", true);
                    break;
                case 1006: // SGR mouse reporting
                    EnableMouseFeature(term, "sgr", true);
                    break;
                case 1015: // URXVT mouse reporting
                    EnableMouseFeature(term, "urxvt", true);
                    break;
                case 1016: // Pixel position mouse reporting
                    EnableMouseFeature(term, "pixel", true);
                    break;
                case 64: // DECSCCM - Multi-Session Support (Private mode 64 typically page/session stuff)
                         // VT520 DECSCCM (Select Cursor Control Mode) is 64 but this context is ? 64.
                         // Standard VT520 doesn't strictly document ?64 as Multi-Session enable, but used here for protocol.
                    // Enable multi-session switching capability
                    GET_SESSION(term)->conformance.features.multi_session_mode = true;
                    break;
                default:
                    // Delegate other private modes to SetTerminalModeInternal
                    SetTerminalModeInternal(term, mode, true, private_mode);
                    break;
            }
        } else {
            // Delegate ANSI modes to SetTerminalModeInternal
            SetTerminalModeInternal(term, mode, true, private_mode);
        }
    }
}

// Reset terminal modes (CSI Pm l or CSI ? Pm l)
// Disables specified modes, including mouse tracking and focus reporting
static void ExecuteRM(Terminal* term, bool private_mode) {
    // Iterate through parsed parameters from the CSI sequence
    for (int i = 0; i < GET_SESSION(term)->param_count; i++) {
        int mode = GET_SESSION(term)->escape_params[i];
        if (private_mode) {
            switch (mode) {
                case 1000: // VT200 mouse tracking
                case 1002: // Button-event mouse tracking
                case 1003: // Any-event mouse tracking
                case 1015: // URXVT mouse reporting
                case 1016: // Pixel position mouse reporting
                    EnableMouseFeature(term, "cursor", false);
                    GET_SESSION(term)->mouse.mode = MOUSE_TRACKING_OFF;
                    break;
                case 1004: // Focus tracking
                    EnableMouseFeature(term, "focus", false);
                    break;
                case 1006: // SGR mouse reporting
                    EnableMouseFeature(term, "sgr", false);
                    break;
                case 64: // Disable Multi-Session Support
                    GET_SESSION(term)->conformance.features.multi_session_mode = false;
                    // If disabling, we should probably switch back to Session 1?
                    if (term->active_session != 0) {
                        SetActiveSession(term, 0);
                    }
                    break;
                default:
                    // Delegate other private modes to SetTerminalModeInternal
                    SetTerminalModeInternal(term, mode, false, private_mode);
                    break;
            }
        } else {
            // Delegate ANSI modes to SetTerminalModeInternal
            SetTerminalModeInternal(term, mode, false, private_mode);
        }
    }
}

// Continue with device attributes and other implementations...

void ExecuteDA(Terminal* term, bool private_mode) {
    char introducer = private_mode ? GET_SESSION(term)->escape_buffer[0] : 0;
    if (introducer == '>') {
        QueueResponse(term, GET_SESSION(term)->secondary_attributes);
    } else if (introducer == '=') {
        QueueResponse(term, GET_SESSION(term)->tertiary_attributes);
    } else {
        QueueResponse(term, GET_SESSION(term)->device_attributes);
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
static void SendToPrinter(Terminal* term, const char* data, size_t length) {
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
static void ExecuteMC(Terminal* term) {
    bool private_mode = (GET_SESSION(term)->escape_buffer[0] == '?');
    int params[MAX_ESCAPE_PARAMS];
    ParseCSIParams(term, GET_SESSION(term)->escape_buffer, params, MAX_ESCAPE_PARAMS);
    int pi = (GET_SESSION(term)->param_count > 0) ? GET_SESSION(term)->escape_params[0] : 0;

    if (!GET_SESSION(term)->printer_available) {
        LogUnsupportedSequence(term, "MC: No printer available");
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
                        EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
                        if (pos < sizeof(print_buffer) - 2) {
                            print_buffer[pos++] = GetPrintableChar(cell->ch, &GET_SESSION(term)->charset);
                        }
                    }
                    if (pos < sizeof(print_buffer) - 2) {
                        print_buffer[pos++] = '\n';
                    }
                }
                print_buffer[pos] = '\0';
                SendToPrinter(term, print_buffer, pos);
                if (GET_SESSION(term)->options.debug_sequences) {
                    LogUnsupportedSequence(term, "MC: Print screen completed");
                }
                break;
            }
            case 1: // Print current line
            {
                char print_buffer[DEFAULT_TERM_WIDTH + 2];
                size_t pos = 0;
                int y = GET_SESSION(term)->cursor.y;
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
                    if (pos < sizeof(print_buffer) - 2) {
                        print_buffer[pos++] = GetPrintableChar(cell->ch, &GET_SESSION(term)->charset);
                    }
                }
                print_buffer[pos++] = '\n';
                print_buffer[pos] = '\0';
                SendToPrinter(term, print_buffer, pos);
                if (GET_SESSION(term)->options.debug_sequences) {
                    LogUnsupportedSequence(term, "MC: Print line completed");
                }
                break;
            }
            case 4: // Disable auto-print
                GET_SESSION(term)->auto_print_enabled = false;
                if (GET_SESSION(term)->options.debug_sequences) {
                    LogUnsupportedSequence(term, "MC: Auto-print disabled");
                }
                break;
            case 5: // Enable auto-print
                GET_SESSION(term)->auto_print_enabled = true;
                if (GET_SESSION(term)->options.debug_sequences) {
                    LogUnsupportedSequence(term, "MC: Auto-print enabled");
                }
                break;
            default:
                if (GET_SESSION(term)->options.log_unsupported) {
                    snprintf(GET_SESSION(term)->conformance.compliance.last_unsupported,
                             sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported),
                             "CSI %d i", pi);
                    GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    } else {
        switch (pi) {
            case 4: // Disable printer controller mode
                GET_SESSION(term)->printer_controller_enabled = false;
                if (GET_SESSION(term)->options.debug_sequences) {
                    LogUnsupportedSequence(term, "MC: Printer controller disabled");
                }
                break;
            case 5: // Enable printer controller mode
                GET_SESSION(term)->printer_controller_enabled = true;
                if (GET_SESSION(term)->options.debug_sequences) {
                }
                break;
            case 9: // Print Screen (DEC specific private parameter for same action as CSI 0 i)
            {
                char print_buffer[DEFAULT_TERM_WIDTH * DEFAULT_TERM_HEIGHT + DEFAULT_TERM_HEIGHT + 1];
                size_t pos = 0;
                for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                    for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                        EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
                        if (pos < sizeof(print_buffer) - 2) {
                            print_buffer[pos++] = GetPrintableChar(cell->ch, &GET_SESSION(term)->charset);
                        }
                    }
                    if (pos < sizeof(print_buffer) - 2) {
                        print_buffer[pos++] = '\n';
                    }
                }
                print_buffer[pos] = '\0';
                SendToPrinter(term, print_buffer, pos);
                if (GET_SESSION(term)->options.debug_sequences) {
                    LogUnsupportedSequence(term, "MC: Print screen (DEC) completed");
                }
                break;
            }
            default:
                if (GET_SESSION(term)->options.log_unsupported) {
                    snprintf(GET_SESSION(term)->conformance.compliance.last_unsupported,
                             sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported),
                             "CSI ?%d i", pi);
                    GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    }
}

// Enhanced QueueResponse
void QueueResponse(Terminal* term, const char* response) {
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
                fprintf(stderr, "QueueResponse: Response too large (%zu bytes)\n", len);
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

void QueueResponseBytes(Terminal* term, const char* data, size_t len) {
    if (GET_SESSION(term)->response_length + len >= sizeof(GET_SESSION(term)->answerback_buffer)) {
        if (term->response_callback && GET_SESSION(term)->response_length > 0) {
            term->response_callback(term, GET_SESSION(term)->answerback_buffer, GET_SESSION(term)->response_length);
            GET_SESSION(term)->response_length = 0;
        }
        if (len >= sizeof(GET_SESSION(term)->answerback_buffer)) {
            if (GET_SESSION(term)->options.debug_sequences) {
                fprintf(stderr, "QueueResponseBytes: Response too large (%zu bytes)\n", len);
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
static void ExecuteDSR(Terminal* term) {
    bool private_mode = (GET_SESSION(term)->escape_buffer[0] == '?');
    int params[MAX_ESCAPE_PARAMS];
    ParseCSIParams(term, GET_SESSION(term)->escape_buffer, params, MAX_ESCAPE_PARAMS);
    int command = (GET_SESSION(term)->param_count > 0) ? GET_SESSION(term)->escape_params[0] : 0;

    if (!private_mode) {
        switch (command) {
            case 5: QueueResponse(term, "\x1B[0n"); break;
            case 6: {
                int row = GET_SESSION(term)->cursor.y + 1;
                int col = GET_SESSION(term)->cursor.x + 1;
                if (GET_SESSION(term)->dec_modes.origin_mode) {
                    row = GET_SESSION(term)->cursor.y - GET_SESSION(term)->scroll_top + 1;
                    col = GET_SESSION(term)->cursor.x - GET_SESSION(term)->left_margin + 1;
                }
                char response[32];
                snprintf(response, sizeof(response), "\x1B[%d;%dR", row, col);
                QueueResponse(term, response);
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
            case 15: QueueResponse(term, GET_SESSION(term)->printer_available ? "\x1B[?10n" : "\x1B[?13n"); break;
            case 21: { // DECRS - Report Session Status
                // If multi-session is not supported/enabled, we might choose to ignore or report limited info.
                // VT520 spec says DECRS reports on sessions.
                // If the terminal doesn't support sessions, it shouldn't respond or should respond with just 1.
                if (!GET_SESSION(term)->conformance.features.multi_session_mode) {
                     if (GET_SESSION(term)->options.debug_sequences) {
                         LogUnsupportedSequence(term, "DECRS ignored: Multi-session mode disabled");
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
                QueueResponse(term, response);
                break;
            }
            case 25: QueueResponse(term, GET_SESSION(term)->programmable_keys.udk_locked ? "\x1B[?21n" : "\x1B[?20n"); break;
            case 26: {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?27;%dn", GET_SESSION(term)->vt_keyboard.keyboard_dialect);
                QueueResponse(term, response);
                break;
            }
            case 27: // Locator Type (DECREPTPARM)
                QueueResponse(term, "\x1B[?27;0n"); // No locator
                break;
            case 53: QueueResponse(term, GET_SESSION(term)->locator_enabled ? "\x1B[?53n" : "\x1B[?50n"); break;
            case 55: QueueResponse(term, "\x1B[?57;0n"); break;
            case 56: QueueResponse(term, "\x1B[?56;0n"); break;
            case 62: {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?62;%zu;%zun",
                         GET_SESSION(term)->macro_space.used, GET_SESSION(term)->macro_space.total);
                QueueResponse(term, response);
                break;
            }
            case 63: {
                int page = (GET_SESSION(term)->param_count > 1) ? GET_SESSION(term)->escape_params[1] : 1;
                GET_SESSION(term)->checksum.last_checksum = ComputeScreenChecksum(term, page);
                char response[64];
                snprintf(response, sizeof(response), "\x1B[?63;%d;%d;%04Xn",
                         page, GET_SESSION(term)->checksum.algorithm, GET_SESSION(term)->checksum.last_checksum);
                QueueResponse(term, response);
                break;
            }
            case 75: QueueResponse(term, "\x1B[?75;0n"); break;
            case 12: { // DECRSN - Report Session Number
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?12;%dn", term->active_session + 1);
                QueueResponse(term, response);
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

void ExecuteDECSTBM(Terminal* term) { // Set Top and Bottom Margins
    int top = GetCSIParam(term, 0, 1) - 1;    // Convert to 0-based
    int bottom = GetCSIParam(term, 1, DEFAULT_TERM_HEIGHT) - 1;

    // Validate parameters
    if (top >= 0 && top < DEFAULT_TERM_HEIGHT && bottom >= top && bottom < DEFAULT_TERM_HEIGHT) {
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

void ExecuteDECSLRM(Terminal* term) { // Set Left and Right Margins (VT420)
    if (!GET_SESSION(term)->conformance.features.vt420_mode) {
        LogUnsupportedSequence(term, "DECSLRM requires VT420 mode");
        return;
    }

    int left = GetCSIParam(term, 0, 1) - 1;    // Convert to 0-based
    int right = GetCSIParam(term, 1, DEFAULT_TERM_WIDTH) - 1;

    // Validate parameters
    if (left >= 0 && left < DEFAULT_TERM_WIDTH && right >= left && right < DEFAULT_TERM_WIDTH) {
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
static void ExecuteDECRQPSR(Terminal* term) {
    int params[MAX_ESCAPE_PARAMS];
    ParseCSIParams(term, GET_SESSION(term)->escape_buffer, params, MAX_ESCAPE_PARAMS);
    int pfn = (GET_SESSION(term)->param_count > 0) ? GET_SESSION(term)->escape_params[0] : 0;

    char response[64];
    switch (pfn) {
        case 1: // Sixel geometry
            snprintf(response, sizeof(response), "DCS 2 $u %d ; %d;%d;%d;%d ST",
                     GET_SESSION(term)->conformance.level, GET_SESSION(term)->sixel.x, GET_SESSION(term)->sixel.y,
                     GET_SESSION(term)->sixel.width, GET_SESSION(term)->sixel.height);
            QueueResponse(term, response);
            break;
        case 2: // Sixel color palette
            for (int i = 0; i < 256; i++) {
                RGB_Color c = term->color_palette[i];
                snprintf(response, sizeof(response), "DCS 1 $u #%d;%d;%d;%d ST",
                         i, c.r, c.g, c.b);
                QueueResponse(term, response);
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

void ExecuteDECLL(Terminal* term) { // Load LEDs
    int n = GetCSIParam(term, 0, 0);

    // DECLL - Load LEDs (VT220+ feature)
    // Parameters: 0=all off, 1=LED1 on, 2=LED2 on, 4=LED3 on, 8=LED4 on
    // Modern terminals don't have LEDs, so this is mostly ignored

    if (GET_SESSION(term)->options.debug_sequences) {
        char debug_msg[64];
        snprintf(debug_msg, sizeof(debug_msg), "DECLL: LED state %d", n);
        LogUnsupportedSequence(term, debug_msg);
    }

    // Could be used for visual indicators in a GUI implementation
    // For now, just acknowledge the command
}

void ExecuteDECSTR(Terminal* term) { // Soft Terminal Reset
    // Reset terminal to power-on defaults but keep communication settings

    // Reset display modes
    GET_SESSION(term)->dec_modes.cursor_visible = true;
    GET_SESSION(term)->dec_modes.auto_wrap_mode = true;
    GET_SESSION(term)->dec_modes.origin_mode = false;
    GET_SESSION(term)->dec_modes.insert_mode = false;
    GET_SESSION(term)->dec_modes.application_cursor_keys = false;

    // Reset character attributes
    ResetAllAttributes(term);

    // Reset scrolling region
    GET_SESSION(term)->scroll_top = 0;
    GET_SESSION(term)->scroll_bottom = DEFAULT_TERM_HEIGHT - 1;
    GET_SESSION(term)->left_margin = 0;
    GET_SESSION(term)->right_margin = DEFAULT_TERM_WIDTH - 1;

    // Reset character sets
    InitCharacterSets(term);

    // Reset tab stops
    InitTabStops(term);

    // Home cursor
    GET_SESSION(term)->cursor.x = 0;
    GET_SESSION(term)->cursor.y = 0;

    // Clear saved cursor
    GET_SESSION(term)->saved_cursor_valid = false;

    InitColorPalette(term);
    InitSixelGraphics(term);

    if (GET_SESSION(term)->options.debug_sequences) {
        LogUnsupportedSequence(term, "DECSTR: Soft terminal reset");
    }
}

void ExecuteDECSCL(Terminal* term) { // Select Conformance Level
    int level = GetCSIParam(term, 0, 61);
    int c1_control = GetCSIParam(term, 1, 0);
    (void)c1_control;  // Mark as intentionally unused

    // Set conformance level based on parameter
    switch (level) {
        case 61: SetVTLevel(term, VT_LEVEL_100); break;
        case 62: SetVTLevel(term, VT_LEVEL_220); break;
        case 63: SetVTLevel(term, VT_LEVEL_320); break;
        case 64: SetVTLevel(term, VT_LEVEL_420); break;
        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown conformance level: %d", level);
                LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }

    // c1_control: 0=8-bit controls, 1=7-bit controls, 2=8-bit controls
    // Modern implementations typically use 7-bit controls
}

// Enhanced ExecuteDECRQM
void ExecuteDECRQM(Terminal* term) { // Request Mode
    int mode = GetCSIParam(term, 0, 0);
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

    QueueResponse(term, response);
}

void ExecuteDECSCUSR(Terminal* term) { // Set Cursor Style
    int style = GetCSIParam(term, 0, 1);

    switch (style) {
        case 0: case 1: // Default or blinking block
            GET_SESSION(term)->cursor.shape = CURSOR_BLOCK_BLINK;
            GET_SESSION(term)->cursor.blink_enabled = true;
            break;
        case 2: // Steady block
            GET_SESSION(term)->cursor.shape = CURSOR_BLOCK;
            GET_SESSION(term)->cursor.blink_enabled = false;
            break;
        case 3: // Blinking underline
            GET_SESSION(term)->cursor.shape = CURSOR_UNDERLINE_BLINK;
            GET_SESSION(term)->cursor.blink_enabled = true;
            break;
        case 4: // Steady underline
            GET_SESSION(term)->cursor.shape = CURSOR_UNDERLINE;
            GET_SESSION(term)->cursor.blink_enabled = false;
            break;
        case 5: // Blinking bar
            GET_SESSION(term)->cursor.shape = CURSOR_BAR_BLINK;
            GET_SESSION(term)->cursor.blink_enabled = true;
            break;
        case 6: // Steady bar
            GET_SESSION(term)->cursor.shape = CURSOR_BAR;
            GET_SESSION(term)->cursor.blink_enabled = false;
            break;
        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown cursor style: %d", style);
                LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}

void ExecuteCSI_P(Terminal* term) { // Various P commands
    // This handles CSI sequences ending in 'p' with different intermediates
    char* params = GET_SESSION(term)->escape_buffer;

    if (strstr(params, "!") != NULL) {
        // DECSTR - Soft Terminal Reset
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
        // Unknown p command
        if (GET_SESSION(term)->options.debug_sequences) {
            char debug_msg[MAX_COMMAND_BUFFER + 64];
            snprintf(debug_msg, sizeof(debug_msg), "Unknown CSI p command: %s", params);
            LogUnsupportedSequence(term, debug_msg);
        }
    }
}

void ExecuteDECSCA(Terminal* term) { // Select Character Protection Attribute
    // CSI Ps " q
    // Ps = 0, 2 -> Not protected
    // Ps = 1 -> Protected
    int ps = GetCSIParam(term, 0, 0);
    if (ps == 1) {
        GET_SESSION(term)->protected_mode = true;
    } else {
        GET_SESSION(term)->protected_mode = false;
    }
}


void ExecuteWindowOps(Terminal* term) { // Window manipulation (xterm extension)
    int operation = GetCSIParam(term, 0, 0);

    switch (operation) {
        case 1: // De-iconify window (Restore)
            SituationRestoreWindow();
            break;
        case 2: // Iconify window (Minimize)
            SituationMinimizeWindow();
            break;
        case 3: // Move window to position (in pixels)
            {
                int x = GetCSIParam(term, 1, 0);
                int y = GetCSIParam(term, 2, 0);
                SituationSetWindowPosition(x, y);
            }
            break;
        case 4: // Resize window (in pixels)
            {
                int height = GetCSIParam(term, 1, DEFAULT_WINDOW_HEIGHT);
                int width = GetCSIParam(term, 2, DEFAULT_WINDOW_WIDTH);
                SituationSetWindowSize(width, height);
            }
            break;
        case 5: // Raise window (Bring to front)
            SituationSetWindowFocused(); // Closest approximation
            break;
        case 6: // Lower window
            // Not directly supported by Situation/GLFW easily
            if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Window lower not supported");
            break;
        case 7: // Refresh window
            // Handled automatically by game loop
            break;
        case 8: // Resize text area (in chars)
            {
                int rows = GetCSIParam(term, 1, DEFAULT_TERM_HEIGHT);
                int cols = GetCSIParam(term, 2, DEFAULT_TERM_WIDTH);
                int width = cols * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE;
                int height = rows * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE;
                SituationSetWindowSize(width, height);
            }
            break;

        case 9: // Maximize/restore window
            if (GetCSIParam(term, 1, 0) == 1) SituationMaximizeWindow();
            else SituationRestoreWindow();
            break;
        case 10: // Full-screen toggle
            if (GetCSIParam(term, 1, 0) == 1) {
                if (!SituationIsWindowFullscreen()) SituationToggleFullscreen();
            } else {
                if (SituationIsWindowFullscreen()) SituationToggleFullscreen();
            }
            break;

        case 11: // Report window state
            QueueResponse(term, "\x1B[1t"); // Always report "not iconified"
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
                QueueResponse(term, response);
            }
            break;

        case 19: // Report screen size
            {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[9;%d;%dt",
                        SituationGetScreenHeight() / DEFAULT_CHAR_HEIGHT, SituationGetScreenWidth() / DEFAULT_CHAR_WIDTH);
                QueueResponse(term, response);
            }
            break;

        case 20: // Report icon label
            {
                char response[MAX_TITLE_LENGTH + 32];
                snprintf(response, sizeof(response), "\x1B]L%s\x1B\\", GET_SESSION(term)->title.icon_title);
                QueueResponse(term, response);
            }
            break;

        case 21: // Report window title
            {
                char response[MAX_TITLE_LENGTH + 32];
                snprintf(response, sizeof(response), "\x1B]l%s\x1B\\", GET_SESSION(term)->title.window_title);
                QueueResponse(term, response);
            }
            break;

        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown window operation: %d", operation);
                LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}

void ExecuteSaveCursor(Terminal* term) {
    GET_SESSION(term)->saved_cursor.x = GET_SESSION(term)->cursor.x;
    GET_SESSION(term)->saved_cursor.y = GET_SESSION(term)->cursor.y;

    // Modes
    GET_SESSION(term)->saved_cursor.origin_mode = GET_SESSION(term)->dec_modes.origin_mode;
    GET_SESSION(term)->saved_cursor.auto_wrap_mode = GET_SESSION(term)->dec_modes.auto_wrap_mode;

    // Attributes
    GET_SESSION(term)->saved_cursor.fg_color = GET_SESSION(term)->current_fg;
    GET_SESSION(term)->saved_cursor.bg_color = GET_SESSION(term)->current_bg;
    GET_SESSION(term)->saved_cursor.bold_mode = GET_SESSION(term)->bold_mode;
    GET_SESSION(term)->saved_cursor.faint_mode = GET_SESSION(term)->faint_mode;
    GET_SESSION(term)->saved_cursor.italic_mode = GET_SESSION(term)->italic_mode;
    GET_SESSION(term)->saved_cursor.underline_mode = GET_SESSION(term)->underline_mode;
    GET_SESSION(term)->saved_cursor.blink_mode = GET_SESSION(term)->blink_mode;
    GET_SESSION(term)->saved_cursor.reverse_mode = GET_SESSION(term)->reverse_mode;
    GET_SESSION(term)->saved_cursor.strikethrough_mode = GET_SESSION(term)->strikethrough_mode;
    GET_SESSION(term)->saved_cursor.conceal_mode = GET_SESSION(term)->conceal_mode;
    GET_SESSION(term)->saved_cursor.overline_mode = GET_SESSION(term)->overline_mode;
    GET_SESSION(term)->saved_cursor.double_underline_mode = GET_SESSION(term)->double_underline_mode;
    GET_SESSION(term)->saved_cursor.protected_mode = GET_SESSION(term)->protected_mode;

    // Charset
    // Direct copy is safe as CharsetState contains values, and pointers point to
    // fields within the CharsetState struct (or rather, the session's charset struct).
    // However, the pointers gl/gr in SavedCursorState will point to GET_SESSION(term)->charset.gX,
    // which is what we want for state restoration?
    // Actually, no. If we restore, we copy saved->active.
    // So saved.gl must point to saved.gX? No.
    // The GL/GR pointers indicate WHICH slot (G0-G3) is active.
    // They point to addresses.
    // If we simply copy, saved.gl points to GET_SESSION(term)->charset.gX.
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
    GET_SESSION(term)->saved_cursor.charset = GET_SESSION(term)->charset;

    GET_SESSION(term)->saved_cursor_valid = true;
}

void ExecuteRestoreCursor(Terminal* term) { // Restore cursor (non-ANSI.SYS)
    // This is the VT terminal version, not ANSI.SYS
    // Restores cursor from per-session saved state
    if (GET_SESSION(term)->saved_cursor_valid) {
        // Restore Position
        GET_SESSION(term)->cursor.x = GET_SESSION(term)->saved_cursor.x;
        GET_SESSION(term)->cursor.y = GET_SESSION(term)->saved_cursor.y;

        // Restore Modes
        GET_SESSION(term)->dec_modes.origin_mode = GET_SESSION(term)->saved_cursor.origin_mode;
        GET_SESSION(term)->dec_modes.auto_wrap_mode = GET_SESSION(term)->saved_cursor.auto_wrap_mode;

        // Restore Attributes
        GET_SESSION(term)->current_fg = GET_SESSION(term)->saved_cursor.fg_color;
        GET_SESSION(term)->current_bg = GET_SESSION(term)->saved_cursor.bg_color;
        GET_SESSION(term)->bold_mode = GET_SESSION(term)->saved_cursor.bold_mode;
        GET_SESSION(term)->faint_mode = GET_SESSION(term)->saved_cursor.faint_mode;
        GET_SESSION(term)->italic_mode = GET_SESSION(term)->saved_cursor.italic_mode;
        GET_SESSION(term)->underline_mode = GET_SESSION(term)->saved_cursor.underline_mode;
        GET_SESSION(term)->blink_mode = GET_SESSION(term)->saved_cursor.blink_mode;
        GET_SESSION(term)->reverse_mode = GET_SESSION(term)->saved_cursor.reverse_mode;
        GET_SESSION(term)->strikethrough_mode = GET_SESSION(term)->saved_cursor.strikethrough_mode;
        GET_SESSION(term)->conceal_mode = GET_SESSION(term)->saved_cursor.conceal_mode;
        GET_SESSION(term)->overline_mode = GET_SESSION(term)->saved_cursor.overline_mode;
        GET_SESSION(term)->double_underline_mode = GET_SESSION(term)->saved_cursor.double_underline_mode;
        GET_SESSION(term)->protected_mode = GET_SESSION(term)->saved_cursor.protected_mode;

        // Restore Charset
        GET_SESSION(term)->charset = GET_SESSION(term)->saved_cursor.charset;

        // Fixup pointers if they point to the saved struct (unlikely with simple copy logic above,
        // but let's ensure they point to the ACTIVE struct slots)
        // If saved.gl pointed to &GET_SESSION(term)->charset.g0, then restored gl points there too.
        // We are good.
    }
}

void ExecuteDECREQTPARM(Terminal* term) { // Request Terminal Parameters
    int parm = GetCSIParam(term, 0, 0);

    char response[32];
    // Report terminal parameters
    // Format: CSI sol ; par ; nbits ; xspeed ; rspeed ; clkmul ; flags x
    // Simplified response with standard values
    snprintf(response, sizeof(response), "\x1B[%d;1;1;120;120;1;0x", parm + 2);
    QueueResponse(term, response);
}

void ExecuteDECTST(Terminal* term) { // Invoke Confidence Test
    int test = GetCSIParam(term, 0, 0);

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
                LogUnsupportedSequence(term, debug_msg);
            }
            break;

        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown DECTST test: %d", test);
                LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}

void ExecuteDECVERP(Terminal* term) { // Verify Parity
    // DECVERP - Enable/disable parity verification
    // Not applicable to software terminals
    if (GET_SESSION(term)->options.debug_sequences) {
        LogUnsupportedSequence(term, "DECVERP - parity verification not applicable");
    }
}

// =============================================================================
// COMPREHENSIVE CSI SEQUENCE PROCESSING
// =============================================================================

// Complete the missing API functions from earlier phases
void ExecuteTBC(Terminal* term) { // Tab Clear
    int n = GetCSIParam(term, 0, 0);

    switch (n) {
        case 0: // Clear tab stop at current column
            ClearTabStop(term, GET_SESSION(term)->cursor.x);
            break;
        case 3: // Clear all tab stops
            ClearAllTabStops(term);
            break;
    }
}

void ExecuteCTC(Terminal* term) { // Cursor Tabulation Control
    int n = GetCSIParam(term, 0, 0);

    switch (n) {
        case 0: // Set tab stop at current column
            SetTabStop(term, GET_SESSION(term)->cursor.x);
            break;
        case 2: // Clear tab stop at current column
            ClearTabStop(term, GET_SESSION(term)->cursor.x);
            break;
        case 5: // Clear all tab stops
            ClearAllTabStops(term);
            break;
    }
}

void ExecuteDECSN(Terminal* term) {
    int session_id = GetCSIParam(term, 0, 0);
    // If param is omitted (0 returned by GetCSIParam if 0 is default), VT520 DECSN usually defaults to 1.
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
                LogUnsupportedSequence(term, msg);
            }
            return;
        }

        if (term->sessions[session_id - 1].session_open) {
            SetActiveSession(term, session_id - 1);
        } else {
            if (GET_SESSION(term)->options.debug_sequences) {
                char msg[64];
                snprintf(msg, sizeof(msg), "DECSN %d ignored: Session not open", session_id);
                LogUnsupportedSequence(term, msg);
            }
        }
    }
}

// New ExecuteCSI_Dollar for multi-byte CSI $ sequences
void ExecuteCSI_Dollar(Terminal* term) {
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
                    LogUnsupportedSequence(term, "Invalid parameters for DECERA/DECFRA");
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
                    LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    } else {
         if (GET_SESSION(term)->options.debug_sequences) {
            char debug_msg[MAX_COMMAND_BUFFER + 64];
            snprintf(debug_msg, sizeof(debug_msg), "Malformed CSI $ sequence in buffer: %s", GET_SESSION(term)->escape_buffer);
            LogUnsupportedSequence(term, debug_msg);
        }
    }
}

void ProcessCSIChar(Terminal* term, unsigned char ch) {
    if (GET_SESSION(term)->parse_state != PARSE_CSI) return;

    if (ch >= 0x40 && ch <= 0x7E) {
        // Parse parameters into GET_SESSION(term)->escape_params
        ParseCSIParams(term, GET_SESSION(term)->escape_buffer, NULL, MAX_ESCAPE_PARAMS);

        // Handle DECSCUSR (CSI Ps SP q)
        if (ch == 'q' && GET_SESSION(term)->escape_pos >= 1 && GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos - 1] == ' ') {
            ExecuteDECSCUSR(term);
        } else {
            // Dispatch to ExecuteCSICommand
            ExecuteCSICommand(term, ch);
        }

        // Reset parser state
        GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
        ClearCSIParams(term);
    } else if (ch >= 0x20 && ch <= 0x3F) {
        // Accumulate intermediate characters (e.g., digits, ';', '?')
        if (GET_SESSION(term)->escape_pos < MAX_COMMAND_BUFFER - 1) {
            GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos++] = ch;
            GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos] = '\0';
        } else {
            if (GET_SESSION(term)->options.debug_sequences) {
                fprintf(stderr, "CSI escape buffer overflow\n");
            }
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            ClearCSIParams(term);
        }
    } else if (ch == '$') {
        // Handle multi-byte CSI sequences (e.g., CSI $ q, CSI $ u)
        if (GET_SESSION(term)->escape_pos < MAX_COMMAND_BUFFER - 1) {
            GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos++] = ch;
            GET_SESSION(term)->escape_buffer[GET_SESSION(term)->escape_pos] = '\0';
        } else {
            if (GET_SESSION(term)->options.debug_sequences) {
                fprintf(stderr, "CSI escape buffer overflow\n");
            }
            GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            ClearCSIParams(term);
        }
    } else {
        // Invalid character
        if (GET_SESSION(term)->options.debug_sequences) {
            snprintf(GET_SESSION(term)->conformance.compliance.last_unsupported,
                     sizeof(GET_SESSION(term)->conformance.compliance.last_unsupported),
                     "Invalid CSI char: 0x%02X", ch);
            GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
        }
        GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
        ClearCSIParams(term);
    }
}

// Updated ExecuteCSICommand
void ExecuteCSICommand(Terminal* term, unsigned char command) {
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
            { int n=GetCSIParam(term, 0,1); while(n-->0) GET_SESSION(term)->cursor.x = NextTabStop(term, GET_SESSION(term)->cursor.x); if (GET_SESSION(term)->cursor.x >= DEFAULT_TERM_WIDTH) GET_SESSION(term)->cursor.x = DEFAULT_TERM_WIDTH -1; }
            // CHT - Cursor Horizontal Tab (CSI Pn I)
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
            if(private_mode) ExecuteCTC(term); else LogUnsupportedSequence(term, "CSI W (non-private)");
            // CTC - Cursor Tab Control (CSI ? Ps W)
            break;
        case 'X': // L_CSI_X_ECH
            ExecuteECH(term);
            // ECH - Erase Character(s) (CSI Pn X)
            break;
        case 'Z': // L_CSI_Z_CBT
            { int n=GetCSIParam(term, 0,1); while(n-->0) GET_SESSION(term)->cursor.x = PreviousTabStop(term, GET_SESSION(term)->cursor.x); }
            // CBT - Cursor Backward Tab (CSI Pn Z)
            break;
        case '`': // L_CSI_tick_HPA
            ExecuteCHA(term);
            // HPA - Horizontal Position Absolute (CSI Pn `) (Same as CHA)
            break;
        case 'a': // L_CSI_a_HPR
            { int n=GetCSIParam(term, 0,1); GET_SESSION(term)->cursor.x+=n; if(GET_SESSION(term)->cursor.x<0)GET_SESSION(term)->cursor.x=0; if(GET_SESSION(term)->cursor.x>=DEFAULT_TERM_WIDTH)GET_SESSION(term)->cursor.x=DEFAULT_TERM_WIDTH-1;}
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
            { int n=GetCSIParam(term, 0,1); GET_SESSION(term)->cursor.y+=n; if(GET_SESSION(term)->cursor.y<0)GET_SESSION(term)->cursor.y=0; if(GET_SESSION(term)->cursor.y>=DEFAULT_TERM_HEIGHT)GET_SESSION(term)->cursor.y=DEFAULT_TERM_HEIGHT-1;}
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
            if(GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "VT420 'o'");
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
            if(!private_mode) ExecuteDECSTBM(term); else LogUnsupportedSequence(term, "CSI ? r invalid");
            // DECSTBM - Set Top/Bottom Margins (CSI Pt ; Pb r)
            break;
        case 's': // L_CSI_s_SAVRES_CUR
            if(private_mode){if(GET_SESSION(term)->conformance.features.vt420_mode) ExecuteDECSLRM(term); else LogUnsupportedSequence(term, "DECSLRM requires VT420");} else { ExecuteSaveCursor(term); }
            // DECSLRM (private VT420+) / Save Cursor (ANSI.SYS) (CSI s / CSI ? Pl ; Pr s)
            break;
        case 't': // L_CSI_t_WINMAN
            ExecuteWindowOps(term);
            // Window Manipulation (xterm) / DECSLPP (Set lines per page) (CSI Ps t)
            break;
        case 'u': // L_CSI_u_RES_CUR
            ExecuteRestoreCursor(term);
            // Restore Cursor (ANSI.SYS) (CSI u)
            break;
        case 'v': // L_CSI_v_RECTCOPY
            if(strstr(GET_SESSION(term)->escape_buffer, "$")) ExecuteDECCRA(term); else if(private_mode) ExecuteRectangularOps(term); else LogUnsupportedSequence(term, "CSI v non-private invalid");
            // DECCRA
            break;
        case 'w': // L_CSI_w_RECTCHKSUM
            if(strstr(GET_SESSION(term)->escape_buffer, "$")) ExecuteDECRQCRA(term); else if(private_mode) ExecuteRectangularOps2(term); else LogUnsupportedSequence(term, "CSI w non-private invalid");
            // DECRQCRA
            break;
        case 'x': // L_CSI_x_DECREQTPARM
            if(strstr(GET_SESSION(term)->escape_buffer, "$")) ExecuteDECFRA(term); else ExecuteDECREQTPARM(term);
            // DECFRA / DECREQTPARM
            break;
        case 'y': // L_CSI_y_DECTST
            ExecuteDECTST(term);
            // DECTST
            break;
        case 'z': // L_CSI_z_DECVERP
            if(strstr(GET_SESSION(term)->escape_buffer, "$")) ExecuteDECERA(term); else if(private_mode) ExecuteDECVERP(term); else LogUnsupportedSequence(term, "CSI z non-private invalid");
            // DECERA / DECVERP
            break;
        case '}': // L_CSI_RSBrace_VT420
            if (strstr(GET_SESSION(term)->escape_buffer, "$")) { ExecuteDECSASD(term); } else { LogUnsupportedSequence(term, "CSI } invalid"); }
            break;
        case '~': // L_CSI_Tilde_VT420
            if (strstr(GET_SESSION(term)->escape_buffer, "!")) { ExecuteDECSN(term); } else if (strstr(GET_SESSION(term)->escape_buffer, "$")) { ExecuteDECSSDT(term); } else { LogUnsupportedSequence(term, "CSI ~ invalid"); }
            break;

        case '{': // L_CSI_LSBrace_DECSLE
            if(strstr(GET_SESSION(term)->escape_buffer, "$")) ExecuteDECSERA(term); else ExecuteDECSLE(term);
            // DECSERA / DECSLE
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
                LogUnsupportedSequence(term, debug_msg);
            }
            GET_SESSION(term)->conformance.compliance.unsupported_sequences++;
            break;
    }
}

// =============================================================================
// OSC (OPERATING SYSTEM COMMAND) PROCESSING
// =============================================================================


void VTSituationSetWindowTitle(Terminal* term, const char* title) {
    strncpy(GET_SESSION(term)->title.window_title, title, MAX_TITLE_LENGTH - 1);
    GET_SESSION(term)->title.window_title[MAX_TITLE_LENGTH - 1] = '\0';
    GET_SESSION(term)->title.title_changed = true;

    if (term->title_callback) {
        term->title_callback(term, GET_SESSION(term)->title.window_title, false);
    }

    // Also set Situation window title
    SituationSetWindowTitle(GET_SESSION(term)->title.window_title);
}

void SetIconTitle(Terminal* term, const char* title) {
    strncpy(GET_SESSION(term)->title.icon_title, title, MAX_TITLE_LENGTH - 1);
    GET_SESSION(term)->title.icon_title[MAX_TITLE_LENGTH - 1] = '\0';
    GET_SESSION(term)->title.icon_changed = true;

    if (term->title_callback) {
        term->title_callback(term, GET_SESSION(term)->title.icon_title, true);
    }
}

void ResetForegroundColor(Terminal* term) {
    GET_SESSION(term)->current_fg.color_mode = 0;
    GET_SESSION(term)->current_fg.value.index = COLOR_WHITE;
}

void ResetBackgroundColor(Terminal* term) {
    GET_SESSION(term)->current_bg.color_mode = 0;
    GET_SESSION(term)->current_bg.value.index = COLOR_BLACK;
}

void ResetCursorColor(Terminal* term) {
    GET_SESSION(term)->cursor.color.color_mode = 0;
    GET_SESSION(term)->cursor.color.value.index = COLOR_WHITE;
}

void ProcessColorCommand(Terminal* term, const char* data) {
    // Format: color_index;rgb:rr/gg/bb or color_index;?
    char* semicolon = strchr(data, ';');
    if (!semicolon) return;

    int color_index = atoi(data);
    char* color_spec = semicolon + 1;

    if (color_spec[0] == '?') {
        // Query color
        char response[128];
        if (color_index >= 0 && color_index < 256) {
            RGB_Color c = term->color_palette[color_index];
            snprintf(response, sizeof(response), "\x1B]4;%d;rgb:%02x/%02x/%02x\x1B\\",
                    color_index, c.r, c.g, c.b);
            QueueResponse(term, response);
        }
    } else if (strncmp(color_spec, "rgb:", 4) == 0) {
        // Set color: rgb:rr/gg/bb
        unsigned int r, g, b;
        if (sscanf(color_spec + 4, "%02x/%02x/%02x", &r, &g, &b) == 3) {
            if (color_index >= 0 && color_index < 256) {
                term->color_palette[color_index] = (RGB_Color){r, g, b, 255};
            }
        }
    }
}

// Additional helper functions for OSC commands
void ResetColorPalette(Terminal* term, const char* data) {
    if (!data || strlen(data) == 0) {
        // Reset entire palette
        InitColorPalette(term);
    } else {
        // Reset specific colors (comma-separated list)
        char* data_copy = strdup(data);
        char* token = strtok(data_copy, ";");

        while (token != NULL) {
            int color_index = atoi(token);
            if (color_index >= 0 && color_index < 16) {
                // Reset to default ANSI color
                term->color_palette[color_index] = (RGB_Color){
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

void ProcessForegroundColorCommand(Terminal* term, const char* data) {
    if (data[0] == '?') {
        // Query foreground color
        char response[64];
        ExtendedColor fg = GET_SESSION(term)->current_fg;
        if (fg.color_mode == 0 && fg.value.index < 16) {
            RGB_Color c = term->color_palette[fg.value.index];
            snprintf(response, sizeof(response), "\x1B]10;rgb:%02x/%02x/%02x\x1B\\", c.r, c.g, c.b);
        } else if (fg.color_mode == 1) {
            snprintf(response, sizeof(response), "\x1B]10;rgb:%02x/%02x/%02x\x1B\\",
                    fg.value.rgb.r, fg.value.rgb.g, fg.value.rgb.b);
        }
        QueueResponse(term, response);
    }
    // Setting foreground via OSC is less common, usually done via SGR
}

void ProcessBackgroundColorCommand(Terminal* term, const char* data) {
    if (data[0] == '?') {
        // Query background color
        char response[64];
        ExtendedColor bg = GET_SESSION(term)->current_bg;
        if (bg.color_mode == 0 && bg.value.index < 16) {
            RGB_Color c = term->color_palette[bg.value.index];
            snprintf(response, sizeof(response), "\x1B]11;rgb:%02x/%02x/%02x\x1B\\", c.r, c.g, c.b);
        } else if (bg.color_mode == 1) {
            snprintf(response, sizeof(response), "\x1B]11;rgb:%02x/%02x/%02x\x1B\\",
                    bg.value.rgb.r, bg.value.rgb.g, bg.value.rgb.b);
        }
        QueueResponse(term, response);
    }
}

void ProcessCursorColorCommand(Terminal* term, const char* data) {
    if (data[0] == '?') {
        // Query cursor color
        char response[64];
        ExtendedColor cursor_color = GET_SESSION(term)->cursor.color;
        if (cursor_color.color_mode == 0 && cursor_color.value.index < 16) {
            RGB_Color c = term->color_palette[cursor_color.value.index];
            snprintf(response, sizeof(response), "\x1B]12;rgb:%02x/%02x/%02x\x1B\\", c.r, c.g, c.b);
        } else if (cursor_color.color_mode == 1) {
            snprintf(response, sizeof(response), "\x1B]12;rgb:%02x/%02x/%02x\x1B\\",
                    cursor_color.value.rgb.r, cursor_color.value.rgb.g, cursor_color.value.rgb.b);
        }
        QueueResponse(term, response);
    }
}

void ProcessFontCommand(Terminal* term, const char* data) {
    // Font selection - simplified implementation
    if (GET_SESSION(term)->options.debug_sequences) {
        LogUnsupportedSequence(term, "Font selection not fully implemented");
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

void ProcessClipboardCommand(Terminal* term, const char* data) {
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
                QueueResponse(term, response_header);
                QueueResponse(term, encoded_data);
                QueueResponse(term, "\x1B\\");
                free(encoded_data);
            }
            SituationFreeString((char*)clipboard_text);
        } else {
            // Empty clipboard response
            char response[16];
            snprintf(response, sizeof(response), "\x1B]52;%c;\x1B\\", clipboard_selector);
            QueueResponse(term, response);
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

void ExecuteOSCCommand(Terminal* term) {
    char* params = GET_SESSION(term)->escape_buffer;

    // Find the command number
    char* semicolon = strchr(params, ';');
    if (!semicolon) {
        LogUnsupportedSequence(term, "Malformed OSC sequence");
        return;
    }

    *semicolon = '\0';
    int command = atoi(params);
    char* data = semicolon + 1;

    switch (command) {
        case 0: // Set window and icon title
        case 2: // Set window title
            VTSituationSetWindowTitle(term, data);
            break;

        case 1: // Set icon title
            SetIconTitle(term, data);
            break;

        case 9: // Notification
            if (term->notification_callback) {
                term->notification_callback(term, data);
            }
            break;

        case 4: // Set/query color palette
            ProcessColorCommand(term, data);
            break;

        case 10: // Query/set foreground color
            ProcessForegroundColorCommand(term, data);
            break;

        case 11: // Query/set background color
            ProcessBackgroundColorCommand(term, data);
            break;

        case 12: // Query/set cursor color
            ProcessCursorColorCommand(term, data);
            break;

        case 50: // Set font
            ProcessFontCommand(term, data);
            break;

        case 52: // Clipboard operations
            ProcessClipboardCommand(term, data);
            break;

        case 104: // Reset color palette
            ResetColorPalette(term, data);
            break;

        case 110: // Reset foreground color
            ResetForegroundColor(term);
            break;

        case 111: // Reset background color
            ResetBackgroundColor(term);
            break;

        case 112: // Reset cursor color
            ResetCursorColor(term);
            break;

        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[128];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown OSC command: %d", command);
                LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }
}

// =============================================================================
// DCS (DEVICE CONTROL STRING) PROCESSING
// =============================================================================

void ProcessTermcapRequest(Terminal* term, const char* request) {
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

    QueueResponse(term, response);
}

// Helper function to convert a single hex character to its integer value
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1; // Invalid hex char
}

void DefineUserKey(Terminal* term, int key_code, const char* sequence, size_t sequence_len) {
    // Expand programmable keys array if needed
    if (GET_SESSION(term)->programmable_keys.count >= GET_SESSION(term)->programmable_keys.capacity) {
        size_t new_capacity = GET_SESSION(term)->programmable_keys.capacity == 0 ? 16 :
                             GET_SESSION(term)->programmable_keys.capacity * 2;

        ProgrammableKey* new_keys = realloc(GET_SESSION(term)->programmable_keys.keys,
                                           new_capacity * sizeof(ProgrammableKey));
        if (!new_keys) return;

        GET_SESSION(term)->programmable_keys.keys = new_keys;
        GET_SESSION(term)->programmable_keys.capacity = new_capacity;
    }

    // Find existing key or add new one
    ProgrammableKey* key = NULL;
    for (size_t i = 0; i < GET_SESSION(term)->programmable_keys.count; i++) {
        if (GET_SESSION(term)->programmable_keys.keys[i].key_code == key_code) {
            key = &GET_SESSION(term)->programmable_keys.keys[i];
            if (key->sequence) free(key->sequence); // Free old sequence
            break;
        }
    }

    if (!key) {
        key = &GET_SESSION(term)->programmable_keys.keys[GET_SESSION(term)->programmable_keys.count++];
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

void ProcessUserDefinedKeys(Terminal* term, const char* data) {
    // Parse user defined key format: key/string;key/string;...
    // The string is a sequence of hexadecimal pairs.
    if (!GET_SESSION(term)->conformance.features.user_defined_keys) {
        LogUnsupportedSequence(term, "User defined keys require VT320 mode");
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
                LogUnsupportedSequence(term, "Invalid hex string in DECUDK");
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
                DefineUserKey(term, key_code, decoded_sequence, decoded_len);
                free(decoded_sequence);
            }
        }
    }

    free(data_copy);
}

void ClearUserDefinedKeys(Terminal* term) {
    for (size_t i = 0; i < GET_SESSION(term)->programmable_keys.count; i++) {
        free(GET_SESSION(term)->programmable_keys.keys[i].sequence);
    }
    GET_SESSION(term)->programmable_keys.count = 0;
}

void ProcessSoftFontDownload(Terminal* term, const char* data) {
    // DECDLD format: Pfn; Pcn; Pe; Pcm; w; h; ... {data}
    // Pfn: Font number (0 or 1)
    // Pcn: Starting character number
    // Pe: Erase control (0=erase all, 1=erase specific, 2=erase all)
    // Pcm: Character matrix size (0=15x12??, 1=13x8, 2=8x10, etc.) - We only support 8x16 effectively
    // w: Font width (1-80) - ignored, we force 8
    // h: Font height (1-24) - ignored, we force 16
    // data: Sixel-like encoded data

    if (!GET_SESSION(term)->conformance.features.soft_fonts) {
        LogUnsupportedSequence(term, "Soft fonts not supported");
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
        if (w > 0 && w <= 32) GET_SESSION(term)->soft_font.char_width = w;
    }
    if (param_idx >= 6) {
        int h = params[5];
        if (h > 0 && h <= 32) GET_SESSION(term)->soft_font.char_height = h;
    }

    // Parse sixel-encoded font data
    if (token) {
        int current_char = (param_idx >= 2) ? params[1] : 0;
        unsigned char* data_ptr = (unsigned char*)token;
        int sixel_row_base = 0; // 0, 6, 12...
        int current_col = 0;

        // Clear current char matrix before starting
        if (current_char < 256) {
            memset(GET_SESSION(term)->soft_font.font_data[current_char], 0, 32);
        }

        while (*data_ptr != '\0') {
            unsigned char ch = *data_ptr;

            if (ch == '/' || ch == ';') {
                // End of character
                if (current_char < 256) {
                    GET_SESSION(term)->soft_font.loaded[current_char] = true;
                }
                current_char++;
                if (current_char >= 256) break;

                // Reset for next char
                memset(GET_SESSION(term)->soft_font.font_data[current_char], 0, 32);
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
                                GET_SESSION(term)->soft_font.font_data[current_char][pixel_y] |= (1 << (7 - current_col));
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
            GET_SESSION(term)->soft_font.loaded[current_char] = true;
        }

        GET_SESSION(term)->soft_font.active = true;
        CreateFontTexture(term);

        if (GET_SESSION(term)->options.debug_sequences) {
            LogUnsupportedSequence(term, "Soft font downloaded and active");
        }
    }

    free(data_copy);
}

void ProcessStatusRequest(Terminal* term, const char* request) {
    // DECRQSS - Request Status String
    char response[MAX_COMMAND_BUFFER];

    if (strcmp(request, "m") == 0) {
        // Request SGR status - Reconstruct SGR string
        char sgr[256];
        int len = 0;

        len += snprintf(sgr + len, sizeof(sgr) - len, "0"); // Reset first

        if (GET_SESSION(term)->bold_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";1");
        if (GET_SESSION(term)->faint_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";2");
        if (GET_SESSION(term)->italic_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";3");
        if (GET_SESSION(term)->underline_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";4");
        if (GET_SESSION(term)->blink_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";5");
        if (GET_SESSION(term)->reverse_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";7");
        if (GET_SESSION(term)->conceal_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";8");
        if (GET_SESSION(term)->strikethrough_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";9");
        if (GET_SESSION(term)->double_underline_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";21");
        if (GET_SESSION(term)->overline_mode) len += snprintf(sgr + len, sizeof(sgr) - len, ";53");

        // Foreground
        if (GET_SESSION(term)->current_fg.color_mode == 0) {
            int idx = GET_SESSION(term)->current_fg.value.index;
            if (idx != COLOR_WHITE) {
                if (idx < 8) len += snprintf(sgr + len, sizeof(sgr) - len, ";%d", 30 + idx);
                else if (idx < 16) len += snprintf(sgr + len, sizeof(sgr) - len, ";%d", 90 + (idx - 8));
                else len += snprintf(sgr + len, sizeof(sgr) - len, ";38;5;%d", idx);
            }
        } else {
             len += snprintf(sgr + len, sizeof(sgr) - len, ";38;2;%d;%d;%d",
                 GET_SESSION(term)->current_fg.value.rgb.r,
                 GET_SESSION(term)->current_fg.value.rgb.g,
                 GET_SESSION(term)->current_fg.value.rgb.b);
        }

        // Background
        if (GET_SESSION(term)->current_bg.color_mode == 0) {
            int idx = GET_SESSION(term)->current_bg.value.index;
            if (idx != COLOR_BLACK) {
                if (idx < 8) len += snprintf(sgr + len, sizeof(sgr) - len, ";%d", 40 + idx);
                else if (idx < 16) len += snprintf(sgr + len, sizeof(sgr) - len, ";%d", 100 + (idx - 8));
                else len += snprintf(sgr + len, sizeof(sgr) - len, ";48;5;%d", idx);
            }
        } else {
             len += snprintf(sgr + len, sizeof(sgr) - len, ";48;2;%d;%d;%d",
                 GET_SESSION(term)->current_bg.value.rgb.r,
                 GET_SESSION(term)->current_bg.value.rgb.g,
                 GET_SESSION(term)->current_bg.value.rgb.b);
        }

        snprintf(response, sizeof(response), "\x1BP1$r%sm\x1B\\", sgr);
        QueueResponse(term, response);
    } else if (strcmp(request, "r") == 0) {
        // Request scrolling region
        snprintf(response, sizeof(response), "\x1BP1$r%d;%dr\x1B\\",
                GET_SESSION(term)->scroll_top + 1, GET_SESSION(term)->scroll_bottom + 1);
        QueueResponse(term, response);
    } else {
        // Unknown request
        snprintf(response, sizeof(response), "\x1BP0$r%s\x1B\\", request);
        QueueResponse(term, response);
    }
}

// New ExecuteDCSAnswerback for DCS 0 ; 0 $ t <message> ST
void ExecuteDCSAnswerback(Terminal* term) {
    char* message_start = strstr(GET_SESSION(term)->escape_buffer, "$t");
    if (message_start) {
        message_start += 2; // Skip "$t"
        char* message_end = strstr(message_start, "\x1B\\"); // Find ST
        if (message_end) {
            size_t length = message_end - message_start;
            if (length >= MAX_COMMAND_BUFFER) {
                length = MAX_COMMAND_BUFFER - 1; // Prevent overflow
            }
            strncpy(GET_SESSION(term)->answerback_buffer, message_start, length);
            GET_SESSION(term)->answerback_buffer[length] = '\0';
        } else if (GET_SESSION(term)->options.debug_sequences) {
            LogUnsupportedSequence(term, "Incomplete DCS $ t sequence");
        }
    } else if (GET_SESSION(term)->options.debug_sequences) {
        LogUnsupportedSequence(term, "Invalid DCS $ t sequence");
    }
}

static void ParseGatewayCommand(Terminal* term, const char* data, size_t len) {
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
        if (term->gateway_callback) {
            term->gateway_callback(term, class_id, id, command, params ? params : "");
        }
    } else {
        if (GET_SESSION(term)->options.debug_sequences) {
            LogUnsupportedSequence(term, "Invalid Gateway Command Format");
        }
    }
}

void ExecuteDCSCommand(Terminal* term) {
    char* params = GET_SESSION(term)->escape_buffer;

    if (strncmp(params, "1;1|", 4) == 0) {
        // DECUDK - User Defined Keys
        ProcessUserDefinedKeys(term, params + 4);
    } else if (strncmp(params, "0;1|", 4) == 0) {
        // DECUDK - Clear User Defined Keys
        ClearUserDefinedKeys(term);
    } else if (strncmp(params, "2;1|", 4) == 0) {
        // DECDLD - Download Soft Font (Variant?)
        ProcessSoftFontDownload(term, params + 4);
    } else if (strstr(params, "{") != NULL) {
        // Standard DECDLD - Download Soft Font (DCS ... { ...)
        // We pass the whole string, ProcessSoftFontDownload will handle tokenization
        ProcessSoftFontDownload(term, params);
    } else if (strncmp(params, "$q", 2) == 0) {
        // DECRQSS - Request Status String
        ProcessStatusRequest(term, params + 2);
    } else if (strncmp(params, "+q", 2) == 0) {
        // XTGETTCAP - Get Termcap
        ProcessTermcapRequest(term, params + 2);
    } else if (strncmp(params, "GATE", 4) == 0) {
        // Gateway Protocol
        // Format: DCS GATE <Class> ; <ID> ; <Command> ... ST
        // Skip "GATE" (4 bytes) and any immediate separator if present
        const char* payload = params + 4;
        if (*payload == ';') payload++;
        ParseGatewayCommand(term, payload, strlen(payload));
    } else {
        if (GET_SESSION(term)->options.debug_sequences) {
            LogUnsupportedSequence(term, "Unknown DCS command");
        }
    }
}

// =============================================================================
// VT52 COMPATIBILITY MODE
// =============================================================================

void ProcessHashChar(Terminal* term, unsigned char ch) {
    // DEC Line Attributes (ESC # Pn)

    // These commands apply to the *entire line* containing the active position.
    // In a real hardware terminal, this changes the scan-out logic.
    // Here, we set flags on all characters in the current row.
    // Note: Using DEFAULT_TERM_WIDTH assumes fixed-width allocation, which matches
    // the current implementation of 'screen' in GET_SESSION(term)->h. If dynamic resizing is
    // added, this should iterate up to GET_SESSION(term)->width or similar.

    switch (ch) {
        case '3': // DECDHL - Double-height line, top half
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_height_top = true;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_height_bottom = false;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_width = true; // Usually implies double width too
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->dirty = true;
            }
            GET_SESSION(term)->row_dirty[GET_SESSION(term)->cursor.y] = true;
            break;

        case '4': // DECDHL - Double-height line, bottom half
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_height_top = false;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_height_bottom = true;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_width = true;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->dirty = true;
            }
            GET_SESSION(term)->row_dirty[GET_SESSION(term)->cursor.y] = true;
            break;

        case '5': // DECSWL - Single-width single-height line
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_height_top = false;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_height_bottom = false;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_width = false;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->dirty = true;
            }
            GET_SESSION(term)->row_dirty[GET_SESSION(term)->cursor.y] = true;
            break;

        case '6': // DECDWL - Double-width single-height line
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_height_top = false;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_height_bottom = false;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->double_width = true;
                GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x)->dirty = true;
            }
            GET_SESSION(term)->row_dirty[GET_SESSION(term)->cursor.y] = true;
            break;

        case '8': // DECALN - Screen Alignment Pattern
            // Fill screen with 'E'
            for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = GetActiveScreenCell(GET_SESSION(term), y, x);
                    cell->ch = 'E';
                    cell->fg_color = GET_SESSION(term)->current_fg;
                    cell->bg_color = GET_SESSION(term)->current_bg;
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
            GET_SESSION(term)->cursor.x = 0;
            GET_SESSION(term)->cursor.y = 0;
            break;

        default:
            if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC # %c", ch);
                LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }

    GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
}

void ProcessPercentChar(Terminal* term, unsigned char ch) {
    // ISO 2022 Select Character Set (ESC % P)

    switch (ch) {
        case '@': // Select default (ISO 8859-1)
            GET_SESSION(term)->charset.g0 = CHARSET_ISO_LATIN_1;
            GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g0;
            // Technically this selects the 'return to default' for ISO 2022.
            break;

        case 'G': // Select UTF-8 (ISO 2022 standard for UTF-8 level 1/2/3)
            GET_SESSION(term)->charset.g0 = CHARSET_UTF8;
            GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g0;
            break;

        default:
             if (GET_SESSION(term)->options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC %% %c", ch);
                LogUnsupportedSequence(term, debug_msg);
            }
            break;
    }

    GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
}

static void ReGIS_DrawLine(Terminal* term, int x0, int y0, int x1, int y1) {
    if (term->vector_count < term->vector_capacity) {
        GPUVectorLine* line = &term->vector_staging_buffer[term->vector_count];

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

        line->color = term->regis.color;
        line->intensity = 1.0f;
        line->mode = term->regis.write_mode;
        term->vector_count++;
    }
}

static int ReGIS_CompareInt(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

static void ReGIS_FillPolygon(Terminal* term) {
    if (term->regis.point_count < 3) {
        term->regis.point_count = 0;
        return;
    }

    // Scanline Fill Algorithm
    int min_y = 480, max_y = 0;
    for(int i=0; i<term->regis.point_count; i++) {
        if (term->regis.point_buffer[i].y < min_y) min_y = term->regis.point_buffer[i].y;
        if (term->regis.point_buffer[i].y > max_y) max_y = term->regis.point_buffer[i].y;
    }
    if (min_y < 0) min_y = 0;
    if (max_y > 479) max_y = 479;

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
                int x_start = nodes[i] < 0 ? 0 : nodes[i];
                int x_end = nodes[i+1] > 799 ? 799 : nodes[i+1];
                if (x_start > 799) break;
                if (x_end < 0) continue;
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
static void ReGIS_EvalBSpline(Terminal* term, int p0x, int p0y, int p1x, int p1y, int p2x, int p2y, int p3x, int p3y, float t, int* out_x, int* out_y) {
    float t2 = t * t;
    float t3 = t2 * t;
    float b0 = (-t3 + 3*t2 - 3*t + 1) / 6.0f;
    float b1 = (3*t3 - 6*t2 + 4) / 6.0f;
    float b2 = (-3*t3 + 3*t2 + 3*t + 1) / 6.0f;
    float b3 = t3 / 6.0f;

    *out_x = (int)(b0*p0x + b1*p1x + b2*p2x + b3*p3x);
    *out_y = (int)(b0*p0y + b1*p1y + b2*p2y + b3*p3y);
}

static void ExecuteReGISCommand(Terminal* term) {
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
            if (target_x < 0) target_x = 0; if (target_x > 799) target_x = 799;
            if (target_y < 0) target_y = 0; if (target_y > 479) target_y = 479;

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

            if (target_x < 0) target_x = 0; if (target_x > 799) target_x = 799;
            if (target_y < 0) target_y = 0; if (target_y > 479) target_y = 479;

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

            if (px < 0) px = 0; if (px > 799) px = 799;
            if (py < 0) py = 0; if (py > 479) py = 479;

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
                 float ncx = (float)cx / 800.0f;
                 float ncy = (float)cy / 480.0f;
                 float nr_x = (float)radius / 800.0f;
                 float nr_y = (float)radius / 480.0f;

                 for (int j = 0; j < segments; j++) {
                    if (term->vector_count >= term->vector_capacity) break;
                    float a1 = j * angle_step;
                    float a2 = (j + 1) * angle_step;
                    float x1 = ncx + cosf(a1) * nr_x;
                    float y1 = ncy + sinf(a1) * nr_y;
                    float x2 = ncx + cosf(a2) * nr_x;
                    float y2 = ncy + sinf(a2) * nr_y;

                    GPUVectorLine* line = &term->vector_staging_buffer[term->vector_count];
                    line->x0 = x1;
                    line->y0 = 1.0f - y1;
                    line->x1 = x2;
                    line->y1 = 1.0f - y2;
                    line->color = term->regis.color;
                    line->intensity = 1.0f;
                    line->mode = term->regis.write_mode;
                    term->vector_count++;
                 }
            }
        }
    }
    // --- S: Screen Control ---
    else if (term->regis.command == 'S') {
        if (term->regis.option_command == 'E') {
             term->vector_count = 0;
             term->vector_clear_request = true;
        }
    }
    // --- W: Write Control ---
    else if (term->regis.command == 'W') {
        // Handle explicit Color Index selection W(I...)
        if (term->regis.option_command == 'I') {
             int color_idx = term->regis.params[0];
             if (color_idx >= 0 && color_idx < 16) {
                 Color c = ansi_colors[color_idx];
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
             // W(C) is ambiguous: could be Complement or Color
             // If we have parameters (e.g. W(C1)), treat as Color (Legacy behavior).
             // If no parameters (e.g. W(C)), treat as Complement (XOR).

             if (term->regis.param_count > 0) {
                 // Likely Color Index W(C1)
                 int color_idx = term->regis.params[0];
                 if (color_idx >= 0 && color_idx < 16) {
                     Color c = ansi_colors[color_idx];
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
                 // If L(A1) is used, we know subsequent string data targets the soft font.
                 // ProcessReGISChar logic implicitly targets soft font.
                 // Maybe we should warn if alpha != 1?
             }
        }
    }
    // --- R: Report ---
    else if (term->regis.command == 'R') {
         if (term->regis.option_command == 'P') {
             char buf[64];
             snprintf(buf, sizeof(buf), "\x1BP%d,%d\x1B\\", term->regis.x, term->regis.y);
             QueueResponse(term, buf);
         }
    }

    term->regis.data_pending = false;
}

static void ProcessReGISChar(Terminal* term, unsigned char ch) {
    if (ch == 0x1B) { // ESC \ (ST)
        if (term->regis.command == 'F') ReGIS_FillPolygon(term); // Flush pending fill
        if (term->regis.state == 1 || term->regis.state == 3) {
            ExecuteReGISCommand(term);
        }
        GET_SESSION(term)->parse_state = VT_PARSE_ESCAPE;
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
        if (term->regis.macro_len >= term->regis.macro_cap - 1) {
             term->regis.macro_cap *= 2;
             term->regis.macro_buffer = realloc(term->regis.macro_buffer, term->regis.macro_cap);
        }
        term->regis.macro_buffer[term->regis.macro_len++] = ch;
        term->regis.macro_buffer[term->regis.macro_len] = '\0';
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
                        memset(GET_SESSION(term)->soft_font.font_data[term->regis.load.current_char], 0, 32);
                        GET_SESSION(term)->soft_font.loaded[term->regis.load.current_char] = true;
                        GET_SESSION(term)->soft_font.active = true;
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
                bool use_soft_font = GET_SESSION(term)->soft_font.active;

                for(int i=0; term->regis.text_buffer[i] != '\0'; i++) {
                    unsigned char c = (unsigned char)term->regis.text_buffer[i];

                    // Use dynamic height if soft font is active
                    int max_rows = use_soft_font ? GET_SESSION(term)->soft_font.char_height : 16;
                    if (max_rows > 32) max_rows = 32;

                    for(int r=0; r<max_rows; r++) { // Iterate up to max_rows
                        unsigned char row = 0;
                        int height_limit = 8; // Default to 8 for VGA font

                        if (use_soft_font && GET_SESSION(term)->soft_font.loaded[c]) {
                            row = GET_SESSION(term)->soft_font.font_data[c][r];
                            height_limit = GET_SESSION(term)->soft_font.char_height;
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
                                    line->x0 = fx0 / 800.0f;
                                    line->y0 = 1.0f - (fy0 / 480.0f);
                                    line->x1 = fx1 / 800.0f;
                                    line->y1 = 1.0f - (fy1 / 480.0f);
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
                         for (int k=0; m[k]; k++) ProcessReGISChar(term, m[k]);
                         term->regis.state = saved_state;
                         term->regis.recursion_depth--;
                     } else {
                         if (GET_SESSION(term)->options.debug_sequences) {
                             LogUnsupportedSequence(term, "ReGIS Macro recursion depth exceeded");
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
            ExecuteReGISCommand(term);
            term->regis.param_count = 0;
            for(int i=0; i<16; i++) {
                term->regis.params[i] = 0;
                term->regis.params_relative[i] = false;
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
            // Defer texture update to DrawTerminal
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
               term->regis.current_val = term->regis.current_val * 10 + (ch - '0');
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

static void ProcessTektronixChar(Terminal* term, unsigned char ch) {
    // 1. Escape Sequence Escape
    if (ch == 0x1B) {
        GET_SESSION(term)->parse_state = VT_PARSE_ESCAPE;
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
        if (term->tektronix.state == 0) ProcessControlChar(term, ch);
        return;
    }

    // 3. Alpha Mode Handling
    if (term->tektronix.state == 0) {
        ProcessNormalChar(term, ch);
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

void ProcessVT52Char(Terminal* term, unsigned char ch) {
    static bool expect_param = false;
    static char vt52_command = 0;

    if (!expect_param) {
        switch (ch) {
            case 'A': // Cursor up
                if (GET_SESSION(term)->cursor.y > 0) GET_SESSION(term)->cursor.y--;
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case 'B': // Cursor down
                if (GET_SESSION(term)->cursor.y < DEFAULT_TERM_HEIGHT - 1) GET_SESSION(term)->cursor.y++;
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case 'C': // Cursor right
                if (GET_SESSION(term)->cursor.x < DEFAULT_TERM_WIDTH - 1) GET_SESSION(term)->cursor.x++;
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case 'D': // Cursor left
                if (GET_SESSION(term)->cursor.x > 0) GET_SESSION(term)->cursor.x--;
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case 'H': // Home cursor
                GET_SESSION(term)->cursor.x = 0;
                GET_SESSION(term)->cursor.y = 0;
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case 'I': // Reverse line feed
                GET_SESSION(term)->cursor.y--;
                if (GET_SESSION(term)->cursor.y < 0) {
                    GET_SESSION(term)->cursor.y = 0;
                    ScrollDownRegion(term, 0, DEFAULT_TERM_HEIGHT - 1, 1);
                }
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case 'J': // Clear to end of screen
                // Clear from cursor to end of line
                for (int x = GET_SESSION(term)->cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                    ClearCell(term, GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x));
                }
                // Clear remaining lines
                for (int y = GET_SESSION(term)->cursor.y + 1; y < DEFAULT_TERM_HEIGHT; y++) {
                    for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                        ClearCell(term, GetActiveScreenCell(GET_SESSION(term), y, x));
                    }
                }
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case 'K': // Clear to end of line
                for (int x = GET_SESSION(term)->cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                    ClearCell(term, GetActiveScreenCell(GET_SESSION(term), GET_SESSION(term)->cursor.y, x));
                }
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case 'Y': // Direct cursor address
                vt52_command = 'Y';
                expect_param = true;
                GET_SESSION(term)->escape_pos = 0;
                break;

            case 'Z': // Identify
                QueueResponse(term, "\x1B/Z"); // VT52 identification
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case '=': // Enter alternate keypad mode
                GET_SESSION(term)->vt_keyboard.keypad_mode = true;
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case '>': // Exit alternate keypad mode
                GET_SESSION(term)->vt_keyboard.keypad_mode = false;
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case '<': // Enter ANSI mode
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                // Exit VT52 mode
                break;

            case 'F': // Enter graphics mode
                GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g1; // Use G1 (DEC special)
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            case 'G': // Exit graphics mode
                GET_SESSION(term)->charset.gl = &GET_SESSION(term)->charset.g0; // Use G0 (ASCII)
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                break;

            default:
                // Unknown VT52 command
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
                if (GET_SESSION(term)->options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown VT52 command: %c", ch);
                    LogUnsupportedSequence(term, debug_msg);
                }
                break;
        }
    } else {
        // Handle parameterized commands
        if (vt52_command == 'Y') {
            if (GET_SESSION(term)->escape_pos == 0) {
                // First parameter: row
                GET_SESSION(term)->escape_buffer[0] = ch;
                GET_SESSION(term)->escape_pos = 1;
            } else {
                // Second parameter: column
                int row = GET_SESSION(term)->escape_buffer[0] - 32; // VT52 uses offset of 32
                int col = ch - 32;

                // Clamp to valid range
                GET_SESSION(term)->cursor.y = (row < 0) ? 0 : (row >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : row;
                GET_SESSION(term)->cursor.x = (col < 0) ? 0 : (col >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : col;

                expect_param = false;
                GET_SESSION(term)->parse_state = VT_PARSE_NORMAL;
            }
        }
    }
}
// =============================================================================
// SIXEL GRAPHICS SUPPORT (Basic Implementation)
// =============================================================================

void ProcessSixelChar(Terminal* term, unsigned char ch) {
    // 1. Check for digits across all states that consume them
    if (isdigit(ch)) {
        if (GET_SESSION(term)->sixel.parse_state == SIXEL_STATE_REPEAT) {
            GET_SESSION(term)->sixel.repeat_count = GET_SESSION(term)->sixel.repeat_count * 10 + (ch - '0');
            return;
        } else if (GET_SESSION(term)->sixel.parse_state == SIXEL_STATE_COLOR ||
                   GET_SESSION(term)->sixel.parse_state == SIXEL_STATE_RASTER) {
            int idx = GET_SESSION(term)->sixel.param_buffer_idx;
            if (idx < 8) {
                GET_SESSION(term)->sixel.param_buffer[idx] = GET_SESSION(term)->sixel.param_buffer[idx] * 10 + (ch - '0');
            }
            return;
        }
    }

    // 2. Handle Separator ';'
    if (ch == ';') {
        if (GET_SESSION(term)->sixel.parse_state == SIXEL_STATE_COLOR ||
            GET_SESSION(term)->sixel.parse_state == SIXEL_STATE_RASTER) {
            if (GET_SESSION(term)->sixel.param_buffer_idx < 7) {
                GET_SESSION(term)->sixel.param_buffer_idx++;
                GET_SESSION(term)->sixel.param_buffer[GET_SESSION(term)->sixel.param_buffer_idx] = 0;
            }
            return;
        }
    }

    // 3. Command Processing
    // If we are in a parameter state but receive a command char, finalize the previous command implicitly.
    if (GET_SESSION(term)->sixel.parse_state == SIXEL_STATE_COLOR) {
        if (ch != '#' && !isdigit(ch) && ch != ';') {
            // Finalize Color Command
            // # Pc ; Pu ; Px ; Py ; Pz (Define) OR # Pc (Select)
            if (GET_SESSION(term)->sixel.param_buffer_idx >= 4) {
                // Color Definition
                // Param 0: Index (Pc)
                // Param 1: Type (Pu) - 1=HLS, 2=RGB
                // Param 2,3,4: Components
                int idx = GET_SESSION(term)->sixel.param_buffer[0];
                int type = GET_SESSION(term)->sixel.param_buffer[1];
                int c1 = GET_SESSION(term)->sixel.param_buffer[2];
                int c2 = GET_SESSION(term)->sixel.param_buffer[3];
                int c3 = GET_SESSION(term)->sixel.param_buffer[4];

                if (idx >= 0 && idx < 256) {
                    if (type == 2) { // RGB (0-100)
                        GET_SESSION(term)->sixel.palette[idx] = (RGB_Color){
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
                    GET_SESSION(term)->sixel.color_index = idx; // Auto-select? Usually yes.
                }
            } else {
                // Color Selection # Pc
                int idx = GET_SESSION(term)->sixel.param_buffer[0];
                if (idx >= 0 && idx < 256) {
                    GET_SESSION(term)->sixel.color_index = idx;
                }
            }
            GET_SESSION(term)->sixel.parse_state = SIXEL_STATE_NORMAL;
        }
    } else if (GET_SESSION(term)->sixel.parse_state == SIXEL_STATE_RASTER) {
        // Finalize Raster Attributes " Pan ; Pad ; Ph ; Pv
        // Just reset state for now, we don't resize based on this yet.
        GET_SESSION(term)->sixel.parse_state = SIXEL_STATE_NORMAL;
    }

    switch (ch) {
        case '"': // Raster attributes
            GET_SESSION(term)->sixel.parse_state = SIXEL_STATE_RASTER;
            GET_SESSION(term)->sixel.param_buffer_idx = 0;
            memset(GET_SESSION(term)->sixel.param_buffer, 0, sizeof(GET_SESSION(term)->sixel.param_buffer));
            break;
        case '#': // Color introducer
            GET_SESSION(term)->sixel.parse_state = SIXEL_STATE_COLOR;
            GET_SESSION(term)->sixel.param_buffer_idx = 0;
            memset(GET_SESSION(term)->sixel.param_buffer, 0, sizeof(GET_SESSION(term)->sixel.param_buffer));
            break;
        case '!': // Repeat introducer
            GET_SESSION(term)->sixel.parse_state = SIXEL_STATE_REPEAT;
            GET_SESSION(term)->sixel.repeat_count = 0;
            break;
        case '$': // Carriage return
            GET_SESSION(term)->sixel.pos_x = 0;
            GET_SESSION(term)->sixel.parse_state = SIXEL_STATE_NORMAL;
            break;
        case '-': // New line
            GET_SESSION(term)->sixel.pos_x = 0;
            GET_SESSION(term)->sixel.pos_y += 6;
            GET_SESSION(term)->sixel.parse_state = SIXEL_STATE_NORMAL;
            break;
        case '\x1B': // Escape character - signals the start of the String Terminator (ST)
             GET_SESSION(term)->parse_state = PARSE_SIXEL_ST;
             return;
        default:
            if (ch >= '?' && ch <= '~') {
                int sixel_val = ch - '?';
                int repeat = 1;

                if (GET_SESSION(term)->sixel.parse_state == SIXEL_STATE_REPEAT) {
                    if (GET_SESSION(term)->sixel.repeat_count > 0) repeat = GET_SESSION(term)->sixel.repeat_count;
                    GET_SESSION(term)->sixel.parse_state = SIXEL_STATE_NORMAL;
                    GET_SESSION(term)->sixel.repeat_count = 0;
                }

                for (int r = 0; r < repeat; r++) {
                    if (GET_SESSION(term)->sixel.strip_count < GET_SESSION(term)->sixel.strip_capacity) {
                        GPUSixelStrip* strip = &GET_SESSION(term)->sixel.strips[GET_SESSION(term)->sixel.strip_count++];
                        strip->x = GET_SESSION(term)->sixel.pos_x + r;
                        strip->y = GET_SESSION(term)->sixel.pos_y; // Top of the 6-pixel column
                        strip->pattern = sixel_val; // 6 bits
                        strip->color_index = GET_SESSION(term)->sixel.color_index;
                    }
                }
                GET_SESSION(term)->sixel.pos_x += repeat;
                if (GET_SESSION(term)->sixel.pos_x > GET_SESSION(term)->sixel.max_x) {
                    GET_SESSION(term)->sixel.max_x = GET_SESSION(term)->sixel.pos_x;
                }
                if (GET_SESSION(term)->sixel.pos_y + 6 > GET_SESSION(term)->sixel.max_y) {
                    GET_SESSION(term)->sixel.max_y = GET_SESSION(term)->sixel.pos_y + 6;
                }
            }
            break;
    }
}

void InitSixelGraphics(Terminal* term) {
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
}

void ProcessSixelData(Terminal* term, const char* data, size_t length) {
    // Basic sixel processing - this is a complex format
    // This implementation provides framework for sixel support

    if (!GET_SESSION(term)->conformance.features.sixel_graphics) {
        LogUnsupportedSequence(term, "Sixel graphics require support enabled");
        return;
    }

    // Allocate sixel staging buffer
    if (!GET_SESSION(term)->sixel.strips) {
        GET_SESSION(term)->sixel.strip_capacity = 65536;
        GET_SESSION(term)->sixel.strips = (GPUSixelStrip*)calloc(GET_SESSION(term)->sixel.strip_capacity, sizeof(GPUSixelStrip));
    }
    GET_SESSION(term)->sixel.strip_count = 0; // Reset for new image? Or append? Standard DCS q usually starts new.

    GET_SESSION(term)->sixel.active = true;
    GET_SESSION(term)->sixel.x = GET_SESSION(term)->cursor.x * DEFAULT_CHAR_WIDTH;
    GET_SESSION(term)->sixel.y = GET_SESSION(term)->cursor.y * DEFAULT_CHAR_HEIGHT;

    // Initialize internal sixel state for parsing
    GET_SESSION(term)->sixel.pos_x = 0;
    GET_SESSION(term)->sixel.pos_y = 0;
    GET_SESSION(term)->sixel.max_x = 0;
    GET_SESSION(term)->sixel.max_y = 0;
    GET_SESSION(term)->sixel.color_index = 0;
    GET_SESSION(term)->sixel.repeat_count = 1;

    // Process the sixel data stream
    for (size_t i = 0; i < length; i++) {
        ProcessSixelChar(term, data[i]);
    }

    GET_SESSION(term)->sixel.dirty = true; // Mark for upload
}

void DrawSixelGraphics(Terminal* term) {
    if (!GET_SESSION(term)->conformance.features.sixel_graphics || !GET_SESSION(term)->sixel.active) return;
    // Just mark dirty, real work happens in DrawTerminal
    GET_SESSION(term)->sixel.dirty = true;
}

// =============================================================================
// RECTANGULAR OPERATIONS (VT420)
// =============================================================================

void ExecuteRectangularOps(Terminal* term) {
    // CSI Pt ; Pl ; Pb ; Pr $ v - Copy rectangular area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        LogUnsupportedSequence(term, "Rectangular operations require support enabled");
        return;
    }

    // CSI Pts ; Pls ; Pbs ; Prs ; Pps ; Ptd ; Pld ; Ppd $ v
    int top = GetCSIParam(term, 0, 1) - 1;
    int left = GetCSIParam(term, 1, 1) - 1;
    int bottom = GetCSIParam(term, 2, DEFAULT_TERM_HEIGHT) - 1;
    int right = GetCSIParam(term, 3, DEFAULT_TERM_WIDTH) - 1;
    // Pps (source page) ignored
    int dest_top = GetCSIParam(term, 5, 1) - 1;
    int dest_left = GetCSIParam(term, 6, 1) - 1;
    // Ppd (dest page) ignored

    // Validate rectangle
    if (top >= 0 && left >= 0 && bottom >= top && right >= left &&
        bottom < DEFAULT_TERM_HEIGHT && right < DEFAULT_TERM_WIDTH) {

        VTRectangle rect = {top, left, bottom, right, true};
        CopyRectangle(term, rect, dest_left, dest_top);
    }
}

void ExecuteRectangularOps2(Terminal* term) {
    // CSI Pt ; Pl ; Pb ; Pr $ w - Request checksum of rectangular area
    if (!GET_SESSION(term)->conformance.features.rectangular_operations) {
        LogUnsupportedSequence(term, "Rectangular operations require support enabled");
        return;
    }

    // Calculate checksum and respond
    // CSI Pid ; Pp ; Pt ; Pl ; Pb ; Pr $ w
    int pid = GetCSIParam(term, 0, 1);
    // int page = GetCSIParam(term, 1, 1); // Ignored
    int top = GetCSIParam(term, 2, 1) - 1;
    int left = GetCSIParam(term, 3, 1) - 1;
    int bottom = GetCSIParam(term, 4, DEFAULT_TERM_HEIGHT) - 1;
    int right = GetCSIParam(term, 5, DEFAULT_TERM_WIDTH) - 1;

    // Validate
    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom >= DEFAULT_TERM_HEIGHT) bottom = DEFAULT_TERM_HEIGHT - 1;
    if (right >= DEFAULT_TERM_WIDTH) right = DEFAULT_TERM_WIDTH - 1;

    unsigned int checksum = 0;
    if (top <= bottom && left <= right) {
        checksum = CalculateRectChecksum(term, top, left, bottom, right);
    }

    char response[32];
    snprintf(response, sizeof(response), "\x1BP%d!~%04X\x1B\\", pid, checksum & 0xFFFF);
    QueueResponse(term, response);
}

void CopyRectangle(Terminal* term, VTRectangle src, int dest_x, int dest_y) {
    int width = src.right - src.left + 1;
    int height = src.bottom - src.top + 1;

    // Allocate temporary buffer for copy
    EnhancedTermChar* temp = malloc(width * height * sizeof(EnhancedTermChar));
    if (!temp) return;

    // Copy source to temp buffer
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (src.top + y < DEFAULT_TERM_HEIGHT && src.left + x < DEFAULT_TERM_WIDTH) {
                temp[y * width + x] = *GetActiveScreenCell(GET_SESSION(term), src.top + y, src.left + x);
            }
        }
    }

    // Copy from temp buffer to destination
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dst_y = dest_y + y;
            int dst_x = dest_x + x;

            if (dst_y >= 0 && dst_y < DEFAULT_TERM_HEIGHT && dst_x >= 0 && dst_x < DEFAULT_TERM_WIDTH) {
                *GetActiveScreenCell(GET_SESSION(term), dst_y, dst_x) = temp[y * width + x];
                GetActiveScreenCell(GET_SESSION(term), dst_y, dst_x)->dirty = true;
            }
        }
        if (dest_y + y >= 0 && dest_y + y < DEFAULT_TERM_HEIGHT) {
            GET_SESSION(term)->row_dirty[dest_y + y] = true;
        }
    }

    free(temp);
}

// =============================================================================
// COMPLETE TERMINAL TESTING FRAMEWORK
// =============================================================================

// Test helper functions
void TestCursorMovement(Terminal* term) {
    PipelineWriteString(term, "\x1B[2J\x1B[H"); // Clear screen, home cursor
    PipelineWriteString(term, "VT Cursor Movement Test\n");
    PipelineWriteString(term, "Testing basic cursor operations...\n\n");

    // Test cursor positioning
    PipelineWriteString(term, "\x1B[5;10HPosition test");
    PipelineWriteString(term, "\x1B[10;1H");

    // Test cursor movement
    PipelineWriteString(term, "Moving: ");
    PipelineWriteString(term, "\x1B[5CRIGHT ");
    PipelineWriteString(term, "\x1B[3DBACK ");
    PipelineWriteString(term, "\x1B[2AUP ");
    PipelineWriteString(term, "\x1B[1BDOWN\n");

    // Test save/restore
    PipelineWriteString(term, "\x1B[s"); // Save cursor
    PipelineWriteString(term, "\x1B[15;20HTemp position");
    PipelineWriteString(term, "\x1B[u"); // Restore cursor
    PipelineWriteString(term, "Back to saved position\n");

    PipelineWriteString(term, "\nCursor test complete.\n");
}

void TestColors(Terminal* term) {
    PipelineWriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString(term, "VT Color Test\n\n");

    // Test basic 16 colors
    PipelineWriteString(term, "Basic 16 colors:\n");
    for (int i = 0; i < 8; i++) {
        PipelineWriteFormat(term, "\x1B[%dm Color %d \x1B[0m", 30 + i, i);
        PipelineWriteFormat(term, "\x1B[%dm Bright %d \x1B[0m\n", 90 + i, i + 8);
    }

    // Test 256 colors (sample)
    PipelineWriteString(term, "\n256-color sample:\n");
    for (int i = 16; i < 32; i++) {
        PipelineWriteFormat(term, "\x1B[38;5;%dm███\x1B[0m", i);
    }
    PipelineWriteString(term, "\n");

    // Test true color
    PipelineWriteString(term, "\nTrue color gradient:\n");
    for (int i = 0; i < 24; i++) {
        int r = (i * 255) / 23;
        PipelineWriteFormat(term, "\x1B[38;2;%d;0;0m█\x1B[0m", r);
    }
    PipelineWriteString(term, "\n\nColor test complete.\n");
}

void TestCharacterSets(Terminal* term) {
    PipelineWriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString(term, "VT Character Set Test\n\n");

    // Test DEC Special Graphics
    PipelineWriteString(term, "DEC Special Graphics:\n");
    PipelineWriteString(term, "\x1B(0"); // Select DEC special
    PipelineWriteString(term, "lqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk\n");
    PipelineWriteString(term, "x                             x\n");
    PipelineWriteString(term, "x    DEC Line Drawing Test    x\n");
    PipelineWriteString(term, "x                             x\n");
    PipelineWriteString(term, "mqqqqqqqqqqwqqqqqqqqqqqqqqqqqj\n");
    PipelineWriteString(term, "             x\n");
    PipelineWriteString(term, "             x\n");
    PipelineWriteString(term, "             v\n");
    PipelineWriteString(term, "\x1B(B"); // Back to ASCII

    PipelineWriteString(term, "\nASCII mode restored.\n");
    PipelineWriteString(term, "Character set test complete.\n");
}

void TestMouseTracking(Terminal* term) {
    PipelineWriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString(term, "VT Mouse Tracking Test\n\n");

    PipelineWriteString(term, "Enabling mouse tracking...\n");
    PipelineWriteString(term, "\x1B[?1000h"); // Enable mouse tracking

    PipelineWriteString(term, "Click anywhere to test mouse reporting.\n");
    PipelineWriteString(term, "Mouse coordinates will be reported.\n");
    PipelineWriteString(term, "Press ESC to disable mouse tracking.\n\n");

    // Mouse tracking will be handled by the input system
    // Results will appear as the user interacts
}

void TestTerminalModes(Terminal* term) {
    PipelineWriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString(term, "VT Terminal Modes Test\n\n");

    // Test insert mode
    PipelineWriteString(term, "Testing insert mode:\n");
    PipelineWriteString(term, "Original: ABCDEF\n");
    PipelineWriteString(term, "ABCDEF\x1B[4D\x1B[4h***\x1B[4l");
    PipelineWriteString(term, "\nAfter insert: AB***CDEF\n\n");

    // Test alternate screen
    PipelineWriteString(term, "Testing alternate screen buffer...\n");
    PipelineWriteString(term, "Switching to alternate screen in 2 seconds...\n");
    // Would need timing mechanism for full demo

    PipelineWriteString(term, "\nMode test complete.\n");
}

void RunAllTests(Terminal* term) {
    PipelineWriteString(term, "\x1B[2J\x1B[H"); // Clear screen
    PipelineWriteString(term, "Running Complete VT Test Suite\n");
    PipelineWriteString(term, "==============================\n\n");

    TestCursorMovement(term);
    PipelineWriteString(term, "\nPress any key to continue...\n");
    // Would wait for input in full implementation

    TestColors(term);
    PipelineWriteString(term, "\nPress any key to continue...\n");

    TestCharacterSets(term);
    PipelineWriteString(term, "\nPress any key to continue...\n");

    TestTerminalModes(term);

    PipelineWriteString(term, "\n\nAll tests completed!\n");
    ShowTerminalInfo(term);
}

void RunVTTest(Terminal* term, const char* test_name) {
    if (strcmp(test_name, "cursor") == 0) {
        TestCursorMovement(term);
    } else if (strcmp(test_name, "colors") == 0) {
        TestColors(term);
    } else if (strcmp(test_name, "charset") == 0) {
        TestCharacterSets(term);
    } else if (strcmp(test_name, "mouse") == 0) {
        TestMouseTracking(term);
    } else if (strcmp(test_name, "modes") == 0) {
        TestTerminalModes(term);
    } else if (strcmp(test_name, "all") == 0) {
        RunAllTests(term);
    } else {
        PipelineWriteFormat(term, "Unknown test: %s\n", test_name);
        PipelineWriteString(term, "Available tests: cursor, colors, charset, mouse, modes, all\n");
    }
}

void ShowTerminalInfo(Terminal* term) {
    PipelineWriteString(term, "\n");
    PipelineWriteString(term, "Terminal Information\n");
    PipelineWriteString(term, "===================\n");
    PipelineWriteFormat(term, "Terminal Type: %s\n", GET_SESSION(term)->title.terminal_name);
    PipelineWriteFormat(term, "VT Level: %d\n", GET_SESSION(term)->conformance.level);
    PipelineWriteFormat(term, "Primary DA: %s\n", GET_SESSION(term)->device_attributes);
    PipelineWriteFormat(term, "Secondary DA: %s\n", GET_SESSION(term)->secondary_attributes);

    PipelineWriteString(term, "\nSupported Features:\n");
    PipelineWriteFormat(term, "- VT52 Mode: %s\n", GET_SESSION(term)->conformance.features.vt52_mode ? "Yes" : "No");
    PipelineWriteFormat(term, "- VT100 Mode: %s\n", GET_SESSION(term)->conformance.features.vt100_mode ? "Yes" : "No");
    PipelineWriteFormat(term, "- VT220 Mode: %s\n", GET_SESSION(term)->conformance.features.vt220_mode ? "Yes" : "No");
    PipelineWriteFormat(term, "- VT320 Mode: %s\n", GET_SESSION(term)->conformance.features.vt320_mode ? "Yes" : "No");
    PipelineWriteFormat(term, "- VT420 Mode: %s\n", GET_SESSION(term)->conformance.features.vt420_mode ? "Yes" : "No");
    PipelineWriteFormat(term, "- VT520 Mode: %s\n", GET_SESSION(term)->conformance.features.vt520_mode ? "Yes" : "No");
    PipelineWriteFormat(term, "- xterm Mode: %s\n", GET_SESSION(term)->conformance.features.xterm_mode ? "Yes" : "No");

    PipelineWriteString(term, "\nCurrent Settings:\n");
    PipelineWriteFormat(term, "- Cursor Keys: %s\n", GET_SESSION(term)->dec_modes.application_cursor_keys ? "Application" : "Normal");
    PipelineWriteFormat(term, "- Keypad: %s\n", GET_SESSION(term)->vt_keyboard.keypad_mode ? "Application" : "Numeric");
    PipelineWriteFormat(term, "- Auto Wrap: %s\n", GET_SESSION(term)->dec_modes.auto_wrap_mode ? "On" : "Off");
    PipelineWriteFormat(term, "- Origin Mode: %s\n", GET_SESSION(term)->dec_modes.origin_mode ? "On" : "Off");
    PipelineWriteFormat(term, "- Insert Mode: %s\n", GET_SESSION(term)->dec_modes.insert_mode ? "On" : "Off");

    PipelineWriteFormat(term, "\nScrolling Region: %d-%d\n",
                       GET_SESSION(term)->scroll_top + 1, GET_SESSION(term)->scroll_bottom + 1);
    PipelineWriteFormat(term, "Margins: %d-%d\n",
                       GET_SESSION(term)->left_margin + 1, GET_SESSION(term)->right_margin + 1);

    PipelineWriteString(term, "\nStatistics:\n");
    TerminalStatus status = GetTerminalStatus(term);
    PipelineWriteFormat(term, "- Pipeline Usage: %zu/%d\n", status.pipeline_usage, (int)sizeof(GET_SESSION(term)->input_pipeline));
    PipelineWriteFormat(term, "- Key Buffer: %zu\n", status.key_usage);
    PipelineWriteFormat(term, "- Unsupported Sequences: %d\n", GET_SESSION(term)->conformance.compliance.unsupported_sequences);

    if (GET_SESSION(term)->conformance.compliance.last_unsupported[0]) {
        PipelineWriteFormat(term, "- Last Unsupported: %s\n", GET_SESSION(term)->conformance.compliance.last_unsupported);
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
 * This is a convenience wrapper around PipelineWriteChar(term).
 * @param ch The character to output.
 */
void Script_PutChar(Terminal* term, unsigned char ch) {
    PipelineWriteChar(term, ch);
}

/**
 * @brief Prints a null-terminated string to the terminal's input pipeline.
 * Part of the scripting API. Convenience wrapper around PipelineWriteString(term).
 * Useful for displaying messages from the hosting application on the term->
 * @param text The string to print.
 */
void Script_Print(Terminal* term, const char* text) {
    PipelineWriteString(term, text);
}

/**
 * @brief Prints a formatted string to the terminal's input pipeline.
 * Part of the scripting API. Convenience wrapper around PipelineWriteFormat(term).
 * Allows for dynamic string construction for display by the hosting application.
 * @param format The printf-style format string.
 * @param ... Additional arguments for the format string.
 */
void Script_Printf(Terminal* term, const char* format, ...) {
    char buffer[1024]; // Note: For very long formatted strings, consider dynamic allocation or a larger buffer.
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    PipelineWriteString(term, buffer);
}

/**
 * @brief Clears the terminal screen and homes the cursor.
 * Part of the scripting API. This sends the standard escape sequences:
 * "ESC[2J" (Erase Display: entire screen) and "ESC[H" (Cursor Home: top-left).
 */
void Script_Cls(Terminal* term) {
    PipelineWriteString(term, "\x1B[2J\x1B[H");
}

/**
 * @brief Sets basic ANSI foreground and background colors for subsequent text output via the Scripting API.
 * Part of the scripting API.
 * This sends SGR (Select Graphic Rendition) escape sequences.
 * @param fg Foreground color index (0-7 for standard, 8-15 for bright).
 * @param bg Background color index (0-7 for standard, 8-15 for bright).
 */
void Script_SetColor(Terminal* term, int fg, int bg) {
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
    PipelineWriteString(term, color_seq);
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
    { VT_LEVEL_520, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt510_mode = true, .vt520_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .rectangular_operations = true, .selective_erase = true, .locator = true, .multi_session_mode = true, .left_right_margin = true, .max_session_count = 3 } },
    { VT_LEVEL_525, { .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt510_mode = true, .vt520_mode = true, .vt525_mode = true, .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true, .regis_graphics = true, .rectangular_operations = true, .selective_erase = true, .locator = true, .true_color = true, .multi_session_mode = true, .left_right_margin = true, .max_session_count = 3 } },
    { VT_LEVEL_XTERM, {
        .vt100_mode = true, .vt102_mode = true, .vt220_mode = true, .vt320_mode = true, .vt340_mode = true, .vt420_mode = true, .vt520_mode = true, .xterm_mode = true,
        .national_charsets = true, .soft_fonts = true, .user_defined_keys = true, .sixel_graphics = true, .regis_graphics = true,
        .rectangular_operations = true, .selective_erase = true, .locator = true, .true_color = true,
        .mouse_tracking = true, .alternate_screen = true, .window_manipulation = true, .left_right_margin = true, .max_session_count = 1
    }},
    { VT_LEVEL_K95, { .k95_mode = true, .max_session_count = 1 } },
    { VT_LEVEL_TT, { .tt_mode = true, .max_session_count = 1 } },
    { VT_LEVEL_PUTTY, { .putty_mode = true, .max_session_count = 1 } },
};

void SetVTLevel(Terminal* term, VTLevel level) {
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

    // Update Device Attribute strings based on the level.
    if (level == VT_LEVEL_XTERM) {
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

VTLevel GetVTLevel(Terminal* term) {
    return GET_SESSION(term)->conformance.level;
}

// --- Keyboard Input ---

/**
 * @brief Retrieves a fully processed keyboard event from the terminal's internal buffer.
 * The application hosting the terminal should call this function repeatedly (e.g., in its
 * main loop after `UpdateVTKeyboard(term)`) to obtain keyboard input.
 *
 * The `VTKeyboard` system, updated by `UpdateVTKeyboard(term)`, translates raw Situation key
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
 * @see UpdateVTKeyboard(term) which captures Situation input and populates the event buffer.
 * @see VTKeyEvent struct for details on the event data fields.
 * @note The terminal platform provides robust keyboard translation, ensuring that applications
 *       running within the terminal receive the correct input sequences based on active modes.
 */
bool GetVTKeyEvent(Terminal* term, VTKeyEvent* event) {
    if (!event || GET_SESSION(term)->vt_keyboard.buffer_count == 0) {
        return false;
    }
    *event = GET_SESSION(term)->vt_keyboard.buffer[GET_SESSION(term)->vt_keyboard.buffer_tail];
    GET_SESSION(term)->vt_keyboard.buffer_tail = (GET_SESSION(term)->vt_keyboard.buffer_tail + 1) % 512; // Assuming vt_keyboard.buffer size is 512
    GET_SESSION(term)->vt_keyboard.buffer_count--;
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
 *  - `GET_SESSION(term)->options.log_unsupported`: Specifically for unsupported sequences.
 *  - `GET_SESSION(term)->options.conformance_checking`: For stricter checks against VT standards.
 *  - `GET_SESSION(term)->status.debugging`: A general flag indicating debug activity.
 *
 * This capability allows developers to understand the terminal's interaction with
 * host applications and identify issues in escape sequence processing.
 *
 * @param enable `true` to enable debug mode, `false` to disable.
 */
void EnableDebugMode(Terminal* term, bool enable) {
    GET_SESSION(term)->options.debug_sequences = enable;
    GET_SESSION(term)->options.log_unsupported = enable;
    GET_SESSION(term)->options.conformance_checking = enable;
    GET_SESSION(term)->status.debugging = enable; // General debugging flag for other parts of the library
}

// --- Core Terminal Loop Functions ---

/**
 * @brief Updates the terminal's internal state and processes incoming data.
 *
 * Called once per frame in the main loop, this function drives the terminal emulation by:
 * - **Processing Input**: Consumes characters from `GET_SESSION(term)->input_pipeline` via `ProcessPipeline(term)`, parsing VT52/xterm sequences with `ProcessChar(term)`.
 * - **Handling Input Devices**: Updates keyboard (`UpdateVTKeyboard(term)`) and mouse (`UpdateMouse(term)`) states.
 * - **Auto-Printing**: Queues lines for printing when `GET_SESSION(term)->auto_print_enabled` and a newline occurs.
 * - **Managing Timers**: Updates cursor blink, text blink, and visual bell timers for visual effects.
 * - **Flushing Responses**: Sends queued responses (e.g., DSR, DA, focus events) via `term->response_callback`.
 * - **Rendering**: Draws the terminal display with `DrawTerminal(term)`, including the custom mouse cursor.
 *
 * Performance is tuned via `GET_SESSION(term)->VTperformance` (e.g., `chars_per_frame`, `time_budget`) to balance responsiveness and throughput.
 *
 * @see ProcessPipeline(term) for input processing details.
 * @see UpdateVTKeyboard(term) for keyboard handling.
 * @see UpdateMouse(term) for mouse and focus event handling.
 * @see DrawTerminal(term) for rendering details.
 * @see QueueResponse(term) for response queuing.
 */
void UpdateTerminal(Terminal* term) {
    term->pending_session_switch = -1; // Reset pending switch
    int saved_session = term->active_session;

    // Process all sessions
    for (int i = 0; i < MAX_SESSIONS; i++) {
        term->active_session = i;

        // Process input from the pipeline
        ProcessPipeline(term);

        // Update timers and bells for this session
        if (GET_SESSION(term)->cursor.blink_enabled && GET_SESSION(term)->dec_modes.cursor_visible) {
            GET_SESSION(term)->cursor.blink_state = SituationTimerGetOscillatorState(250);
        } else {
            GET_SESSION(term)->cursor.blink_state = true;
        }
        GET_SESSION(term)->text_blink_state = SituationTimerGetOscillatorState(255);

        if (GET_SESSION(term)->visual_bell_timer > 0) {
            GET_SESSION(term)->visual_bell_timer -= SituationGetFrameTime();
            if (GET_SESSION(term)->visual_bell_timer < 0) GET_SESSION(term)->visual_bell_timer = 0;
        }

        // Flush responses
        if (GET_SESSION(term)->response_length > 0 && term->response_callback) {
            term->response_callback(term, GET_SESSION(term)->answerback_buffer, GET_SESSION(term)->response_length);
            GET_SESSION(term)->response_length = 0;
        }
    }

    // Restore active session for input handling, unless a switch occurred
    if (term->pending_session_switch != -1) {
        term->active_session = term->pending_session_switch;
    } else {
        term->active_session = saved_session;
    }

    // Update input devices (Keyboard/Mouse) for the ACTIVE session only
    UpdateVTKeyboard(term);
    UpdateMouse(term);

    // Process queued keyboard events for ACTIVE session
    while (GET_SESSION(term)->vt_keyboard.buffer_count > 0) {
        VTKeyEvent* event = &GET_SESSION(term)->vt_keyboard.buffer[GET_SESSION(term)->vt_keyboard.buffer_tail];
        if (event->sequence[0] != '\0') {
            QueueResponse(term, event->sequence);
            if (GET_SESSION(term)->dec_modes.local_echo) {
                for (int i = 0; event->sequence[i] != '\0'; i++) {
                    PipelineWriteChar(term, event->sequence[i]);
                }
            }
            if (event->sequence[0] == 0x07) {
                GET_SESSION(term)->visual_bell_timer = 0.2f;
            }
        }
        GET_SESSION(term)->vt_keyboard.buffer_tail = (GET_SESSION(term)->vt_keyboard.buffer_tail + 1) % KEY_EVENT_BUFFER_SIZE;
        GET_SESSION(term)->vt_keyboard.buffer_count--;
    }

    // Auto-print (Active session only for now, or loop?)
    // Let's assume auto-print works for active session interaction.
    if (GET_SESSION(term)->printer_available && GET_SESSION(term)->auto_print_enabled) {
        if (GET_SESSION(term)->cursor.y > GET_SESSION(term)->last_cursor_y && GET_SESSION(term)->last_cursor_y >= 0) {
            // Queue the previous line for printing
            char print_buffer[DEFAULT_TERM_WIDTH + 2];
            size_t pos = 0;
            for (int x = 0; x < DEFAULT_TERM_WIDTH && pos < DEFAULT_TERM_WIDTH; x++) {
                EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), GET_SESSION(term)->last_cursor_y, x);
                print_buffer[pos++] = GetPrintableChar(cell->ch, &GET_SESSION(term)->charset);
            }
            if (pos < DEFAULT_TERM_WIDTH + 1) {
                print_buffer[pos++] = '\n';
                print_buffer[pos] = '\0';
                QueueResponse(term, print_buffer);
            }
        }
        GET_SESSION(term)->last_cursor_y = GET_SESSION(term)->cursor.y;
    }

    DrawTerminal(term);
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
 *      - Color: Uses `GET_SESSION(term)->cursor.color`.
 *  -   **Sixel Graphics**: If `GET_SESSION(term)->sixel.active` is true, Sixel graphics data is
 *      rendered, typically overlaid on the text grid.
 *  -   **Visual Bell**: If `GET_SESSION(term)->visual_bell_timer` is active, a visual flash effect
 *      may be rendered.
 *
 * The terminal provides a faithful visual emulation, leveraging Situation for efficient
 * 2D rendering.
 *
 * @see EnhancedTermChar for the structure defining each character cell's properties.
 * @see EnhancedCursor for cursor attributes.
 * @see SixelGraphics for Sixel display state.
 * @see InitTerminal(term) where `font_texture` is created.
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
static void BiDiReorderRow(TerminalSession* session, EnhancedTermChar* row, int width) {
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

static void UpdateTerminalRow(Terminal* term, TerminalSession* source_session, int dest_y, int source_y) {
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
        BiDiReorderRow(source_session, temp_row, DEFAULT_TERM_WIDTH);
    }
    // -------------------------------------------

    for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
        EnhancedTermChar* cell = &temp_row[x];
        // Bounds check for safety
        size_t offset = dest_y * DEFAULT_TERM_WIDTH + x;
        if (offset >= DEFAULT_TERM_WIDTH * DEFAULT_TERM_HEIGHT) continue;

        GPUCell* gpu_cell = &term->gpu_staging_buffer[offset];

        // Dynamic Glyph Mapping
        uint32_t char_code;
        if (cell->ch < 256) {
            char_code = cell->ch; // Base set
        } else {
            char_code = AllocateGlyph(term, cell->ch);
        }
        gpu_cell->char_code = char_code;

        // Update LRU if it's a dynamic glyph
        if (char_code >= 256 && char_code != '?') {
            term->glyph_last_used[char_code] = term->frame_count;
        }

        Color fg = {255, 255, 255, 255};
        if (cell->fg_color.color_mode == 0) {
             if (cell->fg_color.value.index < 16) fg = ansi_colors[cell->fg_color.value.index];
             else {
                 RGB_Color c = term->color_palette[cell->fg_color.value.index];
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
                 RGB_Color c = term->color_palette[cell->bg_color.value.index];
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

void UpdateTerminalSSBO(Terminal* term) {
    if (!term->terminal_buffer.id || !term->gpu_staging_buffer) return;

    // Determine which sessions are visible
    bool split = term->split_screen_active;
    int top_idx = term->session_top;
    int bot_idx = term->session_bottom;
    int split_y = term->split_row;

    // Clamp split_y to valid range to prevent OOB logic
    if (split_y < 0) split_y = 0;
    if (split_y >= DEFAULT_TERM_HEIGHT) split_y = DEFAULT_TERM_HEIGHT - 1;

    if (!split) {
        // Single session mode (active session)
        // Wait, if not split, we should probably show the active session?
        // Or strictly session_top? Let's use active_session for single view consistency.
        top_idx = term->active_session;
        split_y = DEFAULT_TERM_HEIGHT; // All rows from top_idx
    }

    size_t required_size = DEFAULT_TERM_WIDTH * DEFAULT_TERM_HEIGHT * sizeof(GPUCell);
    bool any_upload_needed = false;

    // Update global LRU clock
    term->frame_count++;

    // We reconstruct the whole buffer every frame if mixed?
    // Optimization: Check dirty flags.
    // Since we are compositing, we should probably just write to staging buffer always if dirty.

    for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
        TerminalSession* source_session;
        int source_y = y;

        if (y <= split_y) {
            source_session = &term->sessions[top_idx];
        } else {
            source_session = &term->sessions[bot_idx];
            // Do we map y to 0 for bottom session?
            // VT520 split screen usually shows two independent scrolling regions.
            // If we split at row 25. Row 26 is Row 0 of bottom session?
            // "Session 1 on the top half and Session 2 on the bottom half."
            // Usually implied distinct viewports.
            // Let's assume bottom session maps to y - (split_y + 1).
            source_y = y - (split_y + 1);
        }

        // Validate source_y before accessing arrays
        if (source_y < 0 || source_y >= DEFAULT_TERM_HEIGHT) continue;

        // Check if row is dirty in source OR if we just switched layout?
        // For simplicity in this PR, we assume we update if source row is dirty.
        // But if we toggle split screen, we need full redraw.
        // The calling code doesn't set dirty on toggle.
        // We will just upload dirty rows.

        if (source_session->row_dirty[source_y]) {
            UpdateTerminalRow(term, source_session, y, source_y);
            any_upload_needed = true;
        }
    }

    // Only update buffer if data changed to save bandwidth
    if (any_upload_needed) {
        SituationUpdateBuffer(term->terminal_buffer, 0, required_size, term->gpu_staging_buffer);
    }
}

// New API functions


void DrawTerminal(Terminal* term) {
    if (!term->compute_initialized) return;

    // Handle Soft Font Update
    if (GET_SESSION(term)->soft_font.dirty || term->font_atlas_dirty) {
        if (term->font_atlas_pixels) {
            SituationImage img = {0};
            img.width = term->atlas_width;
            img.height = term->atlas_height;
            img.channels = 4;
            img.data = term->font_atlas_pixels; // Pointer alias, don't free

            // Re-upload full texture (Safe Pattern: Create New -> Check -> Swap -> Destroy Old)
            SituationTexture new_texture = {0};
            SituationCreateTexture(img, false, &new_texture);

            if (new_texture.id != 0) {
                if (term->font_texture.generation != 0) SituationDestroyTexture(&term->font_texture);
                term->font_texture = new_texture;
            } else {
                 if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Font texture creation failed");
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
            // Calculate ring buffer distance
            int dist = (GET_SESSION(term)->screen_head - GET_SESSION(term)->sixel.logical_start_row);

            // Handle wrap-around
            if (dist < 0) {
                dist += GET_SESSION(term)->buffer_height;
            } else if (dist > GET_SESSION(term)->buffer_height / 2) {
                // Heuristic: If distance is huge positive, it might be a backward wrap (unlikely for history, but possible if head moved back?)
                // Actually, screen_head only moves forward. logical_start_row is fixed.
                // Distance should be positive (head >= start).
                // If head < start, it wrapped.
                // So (head - start + H) % H is correct.
            }
            // Ensure strictly positive modulo result
            dist = (dist + GET_SESSION(term)->buffer_height) % GET_SESSION(term)->buffer_height;

            // Shift amount (pixels moving UP) = dist * char_height.
            // Plus view_offset (scrolling back moves content DOWN).
            y_shift = (dist * DEFAULT_CHAR_HEIGHT) - (GET_SESSION(term)->view_offset * DEFAULT_CHAR_HEIGHT);
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
                    SituationUpdateBuffer(term->sixel_buffer, 0, GET_SESSION(term)->sixel.strip_count * sizeof(GPUSixelStrip), GET_SESSION(term)->sixel.strips);
                }

                // Repack Palette safely
                uint32_t packed_palette[256];
                for(int i=0; i<256; i++) {
                    RGB_Color c = GET_SESSION(term)->sixel.palette[i];
                    packed_palette[i] = (uint32_t)c.r | ((uint32_t)c.g << 8) | ((uint32_t)c.b << 16) | ((uint32_t)c.a << 24);
                }
                SituationUpdateBuffer(term->sixel_palette_buffer, 0, 256 * sizeof(uint32_t), packed_palette);
            }

            // 2. Dispatch Compute Shader to render to texture
            // FORCE RECREATE TEXTURE TO CLEAR IT (Essential for ytop/lsix to prevent smearing)

            SituationImage img = {0};
            // CreateImage typically returns zeroed buffer
            SituationCreateImage(GET_SESSION(term)->sixel.width, GET_SESSION(term)->sixel.height, 4, &img);
            if (img.data) memset(img.data, 0, GET_SESSION(term)->sixel.width * GET_SESSION(term)->sixel.height * 4); // Ensure zeroed

            SituationTexture new_sixel_tex = {0};
            SituationCreateTextureEx(img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_DST, &new_sixel_tex);

            if (new_sixel_tex.id != 0) {
                if (term->sixel_texture.generation != 0) SituationDestroyTexture(&term->sixel_texture);
                term->sixel_texture = new_sixel_tex;
            } else {
                if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Sixel texture creation failed");
            }

            SituationUnloadImage(img);

            if (SituationAcquireFrameCommandBuffer()) {
                SituationCommandBuffer cmd = SituationGetMainCommandBuffer();
                if (SituationCmdBindComputePipeline(cmd, term->sixel_pipeline) != SITUATION_SUCCESS ||
                    SituationCmdBindComputeTexture(cmd, 0, term->sixel_texture) != SITUATION_SUCCESS) {
                    if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Sixel compute bind failed");
                } else {
                    // Push Constants
                    TerminalPushConstants pc = {0};
                    pc.screen_size = (Vector2){{(float)GET_SESSION(term)->sixel.width, (float)GET_SESSION(term)->sixel.height}};
                    pc.vector_count = GET_SESSION(term)->sixel.strip_count;
                    pc.vector_buffer_addr = SituationGetBufferDeviceAddress(term->sixel_buffer); // Reusing field for sixel buffer
                    pc.terminal_buffer_addr = SituationGetBufferDeviceAddress(term->sixel_palette_buffer); // Reusing field for palette
                    pc.sixel_y_offset = y_shift;

                    if (SituationCmdSetPushConstant(cmd, 0, &pc, sizeof(pc)) != SITUATION_SUCCESS) {
                        if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Sixel push constant failed");
                    } else {
                        if (SituationCmdDispatch(cmd, (GET_SESSION(term)->sixel.strip_count + 63) / 64, 1, 1) != SITUATION_SUCCESS) {
                             if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Sixel dispatch failed");
                        }
                        SituationCmdPipelineBarrier(cmd, SITUATION_BARRIER_COMPUTE_SHADER_WRITE, SITUATION_BARRIER_COMPUTE_SHADER_READ);
                    }
                }
            }
            GET_SESSION(term)->sixel.dirty = false;
        }
    }

    UpdateTerminalSSBO(term);

    if (SituationAcquireFrameCommandBuffer()) {
        SituationCommandBuffer cmd = SituationGetMainCommandBuffer();

        // --- Vector Layer Management (Storage Tube) ---
        if (term->vector_clear_request) {
            // Clear the persistent vector layer
            SituationImage clear_img = {0};
            if (SituationCreateImage(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 4, &clear_img) == SITUATION_SUCCESS) {
                memset(clear_img.data, 0, DEFAULT_WINDOW_WIDTH * DEFAULT_WINDOW_HEIGHT * 4);

                if (term->vector_layer_texture.generation != 0) {
                    SituationDestroyTexture(&term->vector_layer_texture);
                }

                SituationCreateTextureEx(clear_img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_DST, &term->vector_layer_texture);

                if (term->vector_layer_texture.id == 0) {
                    if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Vector layer texture creation failed");
                }
                SituationUnloadImage(clear_img);
            }
            term->vector_clear_request = false;
        }

        if (SituationCmdBindComputePipeline(cmd, term->compute_pipeline) != SITUATION_SUCCESS ||
            SituationCmdBindComputeTexture(cmd, 1, term->output_texture) != SITUATION_SUCCESS) {
             if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Terminal compute bind failed");
        } else {
            TerminalPushConstants pc = {0};
            pc.terminal_buffer_addr = SituationGetBufferDeviceAddress(term->terminal_buffer);

            // Full Bindless (Both Backends)
            pc.font_texture_handle = SituationGetTextureHandle(term->font_texture);
            if (GET_SESSION(term)->sixel.active && term->sixel_texture.generation != 0) {
                pc.sixel_texture_handle = SituationGetTextureHandle(term->sixel_texture);
            } else {
                pc.sixel_texture_handle = SituationGetTextureHandle(term->dummy_sixel_texture);
            }
            pc.vector_texture_handle = SituationGetTextureHandle(term->vector_layer_texture);
            pc.atlas_cols = term->atlas_cols;

            pc.screen_size = (Vector2){{(float)DEFAULT_WINDOW_WIDTH, (float)DEFAULT_WINDOW_HEIGHT}};

            int char_w = DEFAULT_CHAR_WIDTH;
            int char_h = DEFAULT_CHAR_HEIGHT;
            if (GET_SESSION(term)->soft_font.active) {
                char_w = GET_SESSION(term)->soft_font.char_width;
                char_h = GET_SESSION(term)->soft_font.char_height;
            }
            pc.char_size = (Vector2){{(float)char_w, (float)char_h}};

            pc.grid_size = (Vector2){{(float)DEFAULT_TERM_WIDTH, (float)DEFAULT_TERM_HEIGHT}};
            pc.time = (float)SituationTimerGetTime();

            // Calculate visible cursor position
            int cursor_y_screen = -1;
            if (!term->split_screen_active) {
                // Single session: cursor is just session cursor Y
                if (term->active_session == term->session_top) { // Assuming single view uses top slot logic or just active
                     cursor_y_screen = GET_SESSION(term)->cursor.y;
                } else {
                     // Should not happen if non-split uses active_session as top_idx
                     cursor_y_screen = GET_SESSION(term)->cursor.y;
                }
            } else {
                // Split screen: check if active session is visible
                if (term->active_session == term->session_top) {
                    if (GET_SESSION(term)->cursor.y <= term->split_row) {
                        cursor_y_screen = GET_SESSION(term)->cursor.y;
                    }
                } else if (term->active_session == term->session_bottom) {
                    // Bottom session starts visually at split_row + 1
                    // Its internal row 0 maps to screen row split_row + 1
                    // We need to check if cursor fits on screen
                    int screen_y = GET_SESSION(term)->cursor.y + (term->split_row + 1);
                    if (screen_y < DEFAULT_TERM_HEIGHT) {
                        cursor_y_screen = screen_y;
                    }
                }
            }

            if (cursor_y_screen >= 0) {
                pc.cursor_index = cursor_y_screen * DEFAULT_TERM_WIDTH + GET_SESSION(term)->cursor.x;
            } else {
                pc.cursor_index = 0xFFFFFFFF; // Hide cursor
            }

            // Mouse Cursor
            if (GET_SESSION(term)->mouse.enabled && GET_SESSION(term)->mouse.cursor_x > 0) {
                 int mx = GET_SESSION(term)->mouse.cursor_x - 1;
                 int my = GET_SESSION(term)->mouse.cursor_y - 1;

                 // Ensure coordinates are within valid bounds to prevent wrapping/overflow
                 if (mx >= 0 && mx < DEFAULT_TERM_WIDTH && my >= 0 && my < DEFAULT_TERM_HEIGHT) {
                     pc.mouse_cursor_index = my * DEFAULT_TERM_WIDTH + mx;
                 } else {
                     pc.mouse_cursor_index = 0xFFFFFFFF;
                 }
            } else {
                 pc.mouse_cursor_index = 0xFFFFFFFF;
            }

            pc.cursor_blink_state = GET_SESSION(term)->cursor.blink_state ? 1 : 0;
            pc.text_blink_state = GET_SESSION(term)->text_blink_state ? 1 : 0;

            if (GET_SESSION(term)->selection.active) {
                 uint32_t start_idx = GET_SESSION(term)->selection.start_y * DEFAULT_TERM_WIDTH + GET_SESSION(term)->selection.start_x;
                 uint32_t end_idx = GET_SESSION(term)->selection.end_y * DEFAULT_TERM_WIDTH + GET_SESSION(term)->selection.end_x;
                 if (start_idx > end_idx) { uint32_t t = start_idx; start_idx = end_idx; end_idx = t; }
                 pc.sel_start = start_idx;
                 pc.sel_end = end_idx;
                 pc.sel_active = 1;
            }
            pc.scanline_intensity = term->visual_effects.scanline_intensity;
            pc.crt_curvature = term->visual_effects.curvature;

            // Visual Bell
            if (GET_SESSION(term)->visual_bell_timer > 0.0) {
                // Map 0.2s -> 0.0s to 1.0 -> 0.0 intensity
                float intensity = (float)(GET_SESSION(term)->visual_bell_timer / 0.2);
                if (intensity > 1.0f) intensity = 1.0f;
                if (intensity < 0.0f) intensity = 0.0f;
                pc.visual_bell_intensity = intensity;
            }

            if (SituationCmdSetPushConstant(cmd, 0, &pc, sizeof(pc)) != SITUATION_SUCCESS) {
                if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Terminal push constant failed");
            } else {
                if (SituationCmdDispatch(cmd, DEFAULT_TERM_WIDTH, DEFAULT_TERM_HEIGHT, 1) != SITUATION_SUCCESS) {
                    if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Terminal dispatch failed");
                }
            }

            // --- Vector Drawing Pass (Storage Tube Accumulation) ---
            if (term->vector_count > 0) {
                // Update Vector Buffer with NEW lines
                SituationUpdateBuffer(term->vector_buffer, 0, term->vector_count * sizeof(GPUVectorLine), term->vector_staging_buffer);

                // Execute vector drawing after text pass.
                if (SituationCmdBindComputePipeline(cmd, term->vector_pipeline) != SITUATION_SUCCESS ||
                    SituationCmdBindComputeTexture(cmd, 1, term->vector_layer_texture) != SITUATION_SUCCESS) {
                     if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Vector compute bind failed");
                } else {
                    // Push Constants
                    pc.vector_count = term->vector_count;
                    pc.vector_buffer_addr = SituationGetBufferDeviceAddress(term->vector_buffer);

                    if (SituationCmdSetPushConstant(cmd, 0, &pc, sizeof(pc)) != SITUATION_SUCCESS) {
                         if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Vector push constant failed");
                    } else {
                        // Dispatch (64 threads per group)
                        if (SituationCmdDispatch(cmd, (term->vector_count + 63) / 64, 1, 1) != SITUATION_SUCCESS) {
                             if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Vector dispatch failed");
                        }
                        SituationCmdPipelineBarrier(cmd, SITUATION_BARRIER_COMPUTE_SHADER_WRITE, SITUATION_BARRIER_COMPUTE_SHADER_READ);
                    }
                }
                // Reset vector count (Storage Tube behavior: only draw new lines once)
                term->vector_count = 0;
            }

            SituationCmdPipelineBarrier(cmd, SITUATION_BARRIER_COMPUTE_SHADER_WRITE, SITUATION_BARRIER_TRANSFER_READ);

            if (SituationCmdPresent(cmd, term->output_texture) != SITUATION_SUCCESS) {
                 if (GET_SESSION(term)->options.debug_sequences) LogUnsupportedSequence(term, "Present failed");
            }
        }

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
 *  - Memory used for storing sequences of programmable keys (`GET_SESSION(term)->programmable_keys`).
 *  - The Sixel graphics data buffer (`GET_SESSION(term)->sixel.data`) if it was allocated.
 *  - The buffer for bracketed paste data (`GET_SESSION(term)->bracketed_paste.buffer`) if used.
 *
 * It also ensures the input pipeline is cleared. Proper cleanup prevents memory leaks
 * and releases GPU resources.
 */
void CleanupTerminal(Terminal* term) {
    // Free LRU Cache
    if (term->glyph_map) { free(term->glyph_map); term->glyph_map = NULL; }
    if (term->glyph_last_used) { free(term->glyph_last_used); term->glyph_last_used = NULL; }
    if (term->atlas_to_codepoint) { free(term->atlas_to_codepoint); term->atlas_to_codepoint = NULL; }
    if (term->font_atlas_pixels) { free(term->font_atlas_pixels); term->font_atlas_pixels = NULL; }

    if (term->font_texture.generation != 0) SituationDestroyTexture(&term->font_texture);
    if (term->output_texture.generation != 0) SituationDestroyTexture(&term->output_texture);
    if (term->sixel_texture.generation != 0) SituationDestroyTexture(&term->sixel_texture);
    if (term->dummy_sixel_texture.generation != 0) SituationDestroyTexture(&term->dummy_sixel_texture);
    if (term->terminal_buffer.id != 0) SituationDestroyBuffer(&term->terminal_buffer);
    if (term->compute_pipeline.id != 0) SituationDestroyComputePipeline(&term->compute_pipeline);

    if (term->gpu_staging_buffer) {
        free(term->gpu_staging_buffer);
        term->gpu_staging_buffer = NULL;
    }

    // Free session buffers
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (term->sessions[i].screen_buffer) {
            free(term->sessions[i].screen_buffer);
            term->sessions[i].screen_buffer = NULL;
        }
        if (term->sessions[i].alt_buffer) {
            free(term->sessions[i].alt_buffer);
            term->sessions[i].alt_buffer = NULL;
        }
    }

    // Free Vector Engine resources
    if (term->vector_buffer.id != 0) SituationDestroyBuffer(&term->vector_buffer);
    if (term->vector_pipeline.id != 0) SituationDestroyComputePipeline(&term->vector_pipeline);
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

    ClearPipeline(term); // Ensure input pipeline is empty and reset
}

bool InitTerminalDisplay(Terminal* term) {
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
    PipelineWriteString(term, response);
}
int main(void) {
    // Initialize Situation window
    SituationInitInfo init_info = { .window_width = DEFAULT_WINDOW_WIDTH, .window_height = DEFAULT_WINDOW_HEIGHT, .window_title = "Terminal", .initial_active_window_flags = SITUATION_WINDOW_STATE_RESIZABLE };
    SituationInit(0, NULL, &init_info);
    SituationSetTargetFPS(60);

    // Initialize terminal state
    InitTerminal(term);

    // Set response callback
    SetResponseCallback(term, HandleTerminalResponse);

    // Configure initial settings
    EnableDebugMode(term, true); // Enable diagnostics
    SetPipelineTargetFPS(term, 60); // Match pipeline to FPS

    // Send initial text
    PipelineWriteString(term, "Welcome to Enhanced VT Terminal!\n");
    PipelineWriteString(term, "\x1B[32mGreen text\x1B[0m | \x1B[1mBold text\x1B[0m\n");
    PipelineWriteString(term, "Type to interact. Try mouse modes:\n");
    PipelineWriteString(term, "- \x1B[?1000h: VT200\n");
    PipelineWriteString(term, "- \x1B[?1006h: SGR\n");
    PipelineWriteString(term, "- \x1B[?1015h: URXVT\n");
    PipelineWriteString(term, "- \x1B[?1016h: Pixel\n");
    PipelineWriteString(term, "Focus window for \x1B[?1004h events.\n");

    // Enable mouse features
    PipelineWriteString(term, "\x1B[?1004h"); // Focus In/Out
    PipelineWriteString(term, "\x1B[?1000h"); // VT200
    PipelineWriteString(term, "\x1B[?1006h"); // SGR

    while (!WindowShouldClose()) {
        // Update and render terminal (all input reported via HandleTerminalResponse)
        UpdateTerminal(term);

        // Render frame
        SituationBeginFrame();
            DrawFPS(10, 10); // Optional GUI element
        SituationEndFrame();
    }

    // Cleanup resources
    CleanupTerminal(term);
    SituationShutdown();

    return 0;
}
*/


void InitSession(Terminal* term, int index) {
    TerminalSession* session = &term->sessions[index];

    session->last_cursor_y = -1;

    // Initialize dimensions if not already set (defaults)
    if (session->cols == 0) session->cols = DEFAULT_TERM_WIDTH;
    if (session->rows == 0) session->rows = DEFAULT_TERM_HEIGHT;

    // Initialize session defaults
    EnhancedTermChar default_char = {
        .ch = ' ',
        .fg_color = {.color_mode = 0, .value.index = COLOR_WHITE},
        .bg_color = {.color_mode = 0, .value.index = COLOR_BLACK},
        .dirty = true
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

    if (session->alt_buffer) free(session->alt_buffer);
    // Alt buffer is typically fixed size (no scrollback)
    session->alt_buffer = (EnhancedTermChar*)calloc(session->rows * session->cols, sizeof(EnhancedTermChar));

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

    // Reset attributes manually as ResetAllAttributes depends on (*GET_SESSION(term))
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
    // We can reuse the helper functions if they operate on (*GET_SESSION(term)) and we switch context
}


void SetActiveSession(Terminal* term, int index) {
    if (index >= 0 && index < MAX_SESSIONS) {
        term->active_session = index;
        term->pending_session_switch = index; // Queue session switch for UpdateTerminal
        // Invalidate screen to force redraw of the new active session
        for(int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
            term->sessions[term->active_session].row_dirty[y] = true;
        }

        // Update window title for the new session
        if (term->title_callback) {
            term->title_callback(term, term->sessions[index].title.window_title, false);
        }
        SituationSetWindowTitle(term->sessions[index].title.window_title);
    }
}


void SetSplitScreen(Terminal* term, bool active, int row, int top_idx, int bot_idx) {
    term->split_screen_active = active;
    if (active) {
        term->split_row = row;
        if (top_idx >= 0 && top_idx < MAX_SESSIONS) term->session_top = top_idx;
        if (bot_idx >= 0 && bot_idx < MAX_SESSIONS) term->session_bottom = bot_idx;

        // Invalidate both sessions to force redraw
        for(int y=0; y<DEFAULT_TERM_HEIGHT; y++) {
            term->sessions[term->session_top].row_dirty[y] = true;
            term->sessions[term->session_bottom].row_dirty[y] = true;
        }
    } else {
        // Invalidate active session
         for(int y=0; y<DEFAULT_TERM_HEIGHT; y++) {
            term->sessions[term->active_session].row_dirty[y] = true;
        }
    }
}


void PipelineWriteCharToSession(Terminal* term, int session_index, unsigned char ch) {
    if (session_index >= 0 && session_index < MAX_SESSIONS) {
        int saved = term->active_session;
        term->active_session = session_index;
        PipelineWriteChar(term, ch);
        term->active_session = saved;
    }
}

// Resizes the terminal grid and window texture
// Note: This operation destroys and recreates GPU resources, so it might be slow.
void ResizeTerminal(Terminal* term, int cols, int rows) {
    if (cols < 1 || rows < 1) return;
    if (cols == term->width && rows == term->height) return;

    // 1. Update Global Dimensions
    term->width = cols;
    term->height = rows;

    // 2. Resize Session Buffers
    for (int i = 0; i < MAX_SESSIONS; i++) {
        TerminalSession* session = &term->sessions[i];

        int old_cols = session->cols;
        int old_rows = session->rows;

        // Calculate new dimensions
        int new_buffer_height = rows + MAX_SCROLLBACK_LINES;

        // --- Screen Buffer Resize & Content Preservation (Viewport) ---
        EnhancedTermChar* new_screen_buffer = (EnhancedTermChar*)calloc(new_buffer_height * cols, sizeof(EnhancedTermChar));

        // Default char for initialization
        EnhancedTermChar default_char = {
            .ch = ' ',
            .fg_color = {.color_mode = 0, .value.index = COLOR_WHITE},
            .bg_color = {.color_mode = 0, .value.index = COLOR_BLACK},
            .dirty = true
        };

        // Initialize new buffer
        for (int k = 0; k < new_buffer_height * cols; k++) new_screen_buffer[k] = default_char;

        // Copy visible viewport from old buffer
        int copy_rows = (old_rows < rows) ? old_rows : rows;
        int copy_cols = (old_cols < cols) ? old_cols : cols;

        for (int y = 0; y < copy_rows; y++) {
            // Get pointer to source row in ring buffer (uses old session state)
            EnhancedTermChar* src_row_ptr = GetScreenRow(session, y);

            // Destination: Linear at top of new buffer
            EnhancedTermChar* dst_row_ptr = &new_screen_buffer[y * cols];

            // Copy row content
            for (int x = 0; x < copy_cols; x++) {
                dst_row_ptr[x] = src_row_ptr[x];
                dst_row_ptr[x].dirty = true; // Force redraw
            }
        }

        // Swap buffers and update dimensions
        if (session->screen_buffer) free(session->screen_buffer);
        session->screen_buffer = new_screen_buffer;

        session->cols = cols;
        session->rows = rows;
        session->buffer_height = new_buffer_height;

        // Reset ring buffer state
        session->screen_head = 0;
        session->view_offset = 0;
        session->saved_view_offset = 0;

        // Reallocate row_dirty
        if (session->row_dirty) free(session->row_dirty);
        session->row_dirty = (bool*)calloc(rows, sizeof(bool));
        for (int r = 0; r < rows; r++) session->row_dirty[r] = true;

        // Clamp cursor
        if (session->cursor.x >= cols) session->cursor.x = cols - 1;
        if (session->cursor.y >= rows) session->cursor.y = rows - 1;

        // Reset margins
        session->left_margin = 0;
        session->right_margin = cols - 1;
        session->scroll_top = 0;
        session->scroll_bottom = rows - 1;

        // --- Alt Buffer Resize ---
        if (session->alt_buffer) free(session->alt_buffer);
        session->alt_buffer = (EnhancedTermChar*)calloc(rows * cols, sizeof(EnhancedTermChar));
        for (int k = 0; k < rows * cols; k++) session->alt_buffer[k] = default_char;
        session->alt_screen_head = 0;
    }

    // 3. Recreate GPU Resources
    if (term->compute_initialized) {
        if (term->terminal_buffer.id != 0) SituationDestroyBuffer(&term->terminal_buffer);
        if (term->output_texture.generation != 0) SituationDestroyTexture(&term->output_texture);
        if (term->gpu_staging_buffer) free(term->gpu_staging_buffer);

        size_t buffer_size = cols * rows * sizeof(GPUCell);
        SituationCreateBuffer(buffer_size, NULL, SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_TRANSFER_DST, &term->terminal_buffer);

        int win_width = cols * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE;
        int win_height = rows * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE;
        SituationImage empty_img = {0};
        SituationCreateImage(win_width, win_height, 4, &empty_img);
        SituationCreateTextureEx(empty_img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_SRC, &term->output_texture);
        SituationUnloadImage(empty_img);

        term->gpu_staging_buffer = (GPUCell*)calloc(cols * rows, sizeof(GPUCell));

        if (term->vector_layer_texture.generation != 0) SituationDestroyTexture(&term->vector_layer_texture);
        SituationImage vec_img = {0};
        SituationCreateImage(win_width, win_height, 4, &vec_img);
        memset(vec_img.data, 0, win_width * win_height * 4);
        SituationCreateTextureEx(vec_img, false, SITUATION_TEXTURE_USAGE_SAMPLED | SITUATION_TEXTURE_USAGE_STORAGE | SITUATION_TEXTURE_USAGE_TRANSFER_DST, &term->vector_layer_texture);
        SituationUnloadImage(vec_img);
    }

    // Update Split Row if needed
    if (term->split_screen_active) {
        if (term->split_row >= rows) term->split_row = rows / 2;
    } else {
        term->split_row = rows / 2;
    }
}


#endif // TERMINAL_IMPLEMENTATION


#endif // TERMINAL_H
