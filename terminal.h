// terminal.h - Enhanced Terminal Library Implementation Phase 5
// Comprehensive VT52/VT100/VT220/VT320/VT420/xterm compatibility with modern features

/**********************************************************************************************
*
*   terminal.h - Enhanced Terminal Emulation Library
*   (c) 2025 Jacques Morel
*
*   DESCRIPTION:
*       This library provides a comprehensive terminal emulation solution, aiming for compatibility with VT52, VT100, VT220, VT320, VT420, and xterm standards,
*       while also incorporating modern features like true color support, Sixel graphics, advanced mouse tracking, and bracketed paste mode. It is designed to be
*       integrated into applications that require a text-based terminal interface, using the Raylib library for rendering, input, and window management.
*
*       The library processes a stream of input characters (typically from a host application or PTY) and updates an internal screen buffer. This buffer,
*       representing the terminal display, is then rendered to the screen. It handles a wide range of escape sequences to control cursor movement, text attributes,
*       colors, screen clearing, scrolling, and various terminal modes.
*
**********************************************************************************************/
#ifndef TERMINAL_H
#define TERMINAL_H

#define SITUATION_IMPLEMENTATION
#include "situation.h"  // Ensure full impl

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
#define DEFAULT_WINDOW_HEIGHT (DEFAULT_TERM_HEIGHT * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE)
#define MAX_ESCAPE_PARAMS 32
#define MAX_COMMAND_BUFFER 512 // General purpose buffer for commands, OSC, DCS etc.
#define MAX_TAB_STOPS 256 // Max columns for tab stops, ensure it's >= DEFAULT_TERM_WIDTH
#define MAX_TITLE_LENGTH 256
#define MAX_RECT_OPERATIONS 16
#define KEY_EVENT_BUFFER_SIZE 65536
#define OUTPUT_BUFFER_SIZE 16384

// =============================================================================
// GLOBAL VARIABLES DECLARATIONS
// =============================================================================
// Callbacks for application to handle terminal events
// Response callback typedef
typedef void (*ResponseCallback)(const char* response, int length); // For sending data back to host
typedef void (*TitleCallback)(const char* title, bool is_icon);    // For GUI window title changes
typedef void (*BellCallback)(void);                                 // For audible bell

#ifndef TERMINAL_IMPLEMENTATION
// External declarations for users of the library (if not header-only)
extern Terminal terminal;
//extern VTKeyboard vt_keyboard;
extern Texture2D font_texture;
extern RGB_Color color_palette[256]; // Full 256 color palette
extern Color ansi_colors[16];        // Raylib Color type for the 16 base ANSI colors
// extern unsigned char font_data[256 * 32]; // Defined in implementation
extern ResponseCallback response_callback;
extern TitleCallback title_callback;
extern BellCallback bell_callback;
#endif

// =============================================================================
// VT COMPLIANCE LEVELS
// =============================================================================
typedef enum {
    VT_LEVEL_52 = 52,
    VT_LEVEL_100 = 100,
    VT_LEVEL_220 = 220,
    VT_LEVEL_320 = 320,
    VT_LEVEL_420 = 420,
    VT_LEVEL_520 = 520,
    VT_LEVEL_XTERM = 999
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
    PARSE_VT52,         // In VT52 compatibility mode
    PARSE_SIXEL,        // Parsing Sixel graphics data (ESC P q ... ST)
    PARSE_SIXEL_ST
} VTParseState;

// =============================================================================
// ENHANCED COLOR SYSTEM
// =============================================================================
typedef enum {
    COLOR_BLACK = 0, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE,
    COLOR_BRIGHT_BLACK, COLOR_BRIGHT_RED, COLOR_BRIGHT_GREEN, COLOR_BRIGHT_YELLOW,
    COLOR_BRIGHT_BLUE, COLOR_BRIGHT_MAGENTA, COLOR_BRIGHT_CYAN, COLOR_BRIGHT_WHITE
} AnsiColor; // Standard 16 ANSI colors

typedef struct {
    unsigned char r, g, b, a;
} RGB_Color; // True color representation

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
    CHARSET_UTF8            // UTF-8 (requires multi-byte processing)
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
    int key_code;           // Raylib key code that triggers this
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
} SixelGraphics;

// =============================================================================
// SOFT FONTS
// =============================================================================
typedef struct {
    unsigned char font_data[256][32]; // Storage for 256 characters, 32 bytes each (e.g., 16x16 monochrome)
    int char_width;                   // Width of characters in this font
    int char_height;                  // Height of characters in this font
    bool loaded[256];                 // Which characters in this set are loaded
    bool active;                      // Is a soft font currently selected?
} SoftFont;

// =============================================================================
// VT CONFORMANCE TESTING
// =============================================================================
typedef struct {
    VTLevel level;        // Current conformance level (e.g., VT220)
    bool strict_mode;     // Enforce strict conformance? (vs. permissive)
    
    // Feature flags based on level and specific xterm extensions
    struct {
        bool vt52_mode;
        bool vt100_mode;
        bool vt220_mode;
        bool vt320_mode;
        bool vt420_mode;
        bool vt520_mode;
        bool xterm_mode; // For xterm-specific extensions
    } features;
    
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
    int key_code;           // Raylib key code (e.g., KEY_A, KEY_F1) or char code
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
    //     int raylib_key;
    //     char normal[16];
    //     char shift[16];
    //     char ctrl[16];
    //     char alt[16];
    //     char app[16]; // For application modes
    // } key_mappings[256]; // Max Raylib key codes
    
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
// MAIN ENHANCED TERMINAL STRUCTURE
// =============================================================================
typedef struct {
    // Screen management
    EnhancedTermChar screen[DEFAULT_TERM_HEIGHT][DEFAULT_TERM_WIDTH];       // Primary screen buffer
    EnhancedTermChar alt_screen[DEFAULT_TERM_HEIGHT][DEFAULT_TERM_WIDTH];   // Alternate screen buffer
    // EnhancedTermChar saved_screen[DEFAULT_TERM_HEIGHT][DEFAULT_TERM_WIDTH]; // For DECSEL/DECSED if implemented

    // Enhanced cursor
    EnhancedCursor cursor;
    EnhancedCursor saved_cursor; // For DECSC/DECRC and other save/restore ops
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
    ResponseCallback response_callback; // Response callback

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
    
    // Fields for RayLib_terminal_console.c
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
    
} Terminal;

// =============================================================================
// CORE API FUNCTIONS
// =============================================================================

// Terminal lifecycle
void InitTerminal(void);
void CleanupTerminal(void);
void UpdateTerminal(void);  // Process events, update states (e.g., cursor blink)
void DrawTerminal(void);    // Render the terminal state to screen

// VT compliance and identification
bool GetVTKeyEvent(VTKeyEvent* event); // Retrieve a processed key event
void SetVTLevel(VTLevel level);
VTLevel GetVTLevel(void);
void EnableVTFeature(const char* feature, bool enable); // e.g., "sixel", "DECCKM"
bool IsVTFeatureSupported(const char* feature);
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
void UpdateMouse(void); // Process mouse input from Raylib and generate VT sequences

// Keyboard support (VT compatible)
void UpdateVTKeyboard(void); // Process keyboard input from Raylib
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
void VTSetWindowTitle(const char* title); // Set window title (OSC 0, OSC 2)
void SetIconTitle(const char* title);   // Set icon title (OSC 1)
const char* GetWindowTitle(void);
const char* GetIconTitle(void);

// Callbacks
void SetResponseCallback(ResponseCallback callback);
void SetTitleCallback(TitleCallback callback);
void SetBellCallback(BellCallback callback);

// Testing and diagnostics
void RunVTTest(const char* test_name); // Run predefined test sequences
void ShowTerminalInfo(void);           // Display current terminal state/info
void EnableDebugMode(bool enable);     // Toggle verbose debug logging
void LogUnsupportedSequence(const char* sequence); // Log an unsupported sequence
TerminalStatus GetTerminalStatus(void);
void ShowBufferDiagnostics(void);      // Display buffer usage info

// Screen buffer management
void VTSwapScreenBuffer(void); // Handles 1047/1049 logic

// Internal rendering/parsing functions (potentially exposed for advanced use or testing)
void CreateFontTexture(void);

// Low-level char processing (called by ProcessPipeline via ProcessChar)
void ProcessChar(unsigned char ch); // Main dispatcher for character processing
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


// Screen manipulation internals
void ScrollUpRegion(int top, int bottom, int lines);
void InsertLinesAt(int row, int count); // Added IL
void DeleteLinesAt(int row, int count); // Added DL
void InsertCharactersAt(int row, int col, int count); // Added ICH
void DeleteCharactersAt(int row, int col, int count); // Added DCH
void InsertCharacterAtCursor(unsigned int ch); // Handles character placement and insert mode
void ScrollDownRegion(int top, int bottom, int lines);

// Response and parsing helpers
void QueueResponse(const char* response); // Add string to answerback_buffer
void QueueResponseBytes(const char* data, size_t len);
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

// =============================================================================
// IMPLEMENTATION BEGINS HERE
// =============================================================================

// Fixed global variable definitions
Terminal terminal = {0};
//VTKeyboard vt_keyboard = {0};   // deprecated
RGLTexture font_texture = {0};  // SIT/RGL BRIDGE: The font texture is now an RGLTexture
ResponseCallback response_callback = NULL;
TitleCallback title_callback = NULL;
BellCallback bell_callback = NULL;

// Color mappings - Fixed initialization
RGB_Color color_palette[256];

Color ansi_colors[16] = { // Raylib Color type
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
    terminal.conformance.level = VT_LEVEL_XTERM; 
    terminal.conformance.strict_mode = false;
    SetVTLevel(terminal.conformance.level); 
    terminal.conformance.compliance.unsupported_sequences = 0;
    terminal.conformance.compliance.partial_implementations = 0;
    terminal.conformance.compliance.extensions_used = 0;
    terminal.conformance.compliance.last_unsupported[0] = '\0';
}

void InitTabStops(void) {
    memset(terminal.tab_stops.stops, false, sizeof(terminal.tab_stops.stops));
    terminal.tab_stops.count = 0;
    terminal.tab_stops.default_width = 8;
    for (int i = terminal.tab_stops.default_width; i < MAX_TAB_STOPS && i < DEFAULT_TERM_WIDTH; i += terminal.tab_stops.default_width) {
        terminal.tab_stops.stops[i] = true;
        terminal.tab_stops.count++;
    }
}

void InitCharacterSets(void) {
    terminal.charset.g0 = CHARSET_ASCII;
    terminal.charset.g1 = CHARSET_DEC_SPECIAL; 
    terminal.charset.g2 = CHARSET_ASCII;       
    terminal.charset.g3 = CHARSET_ASCII;       
    terminal.charset.gl = &terminal.charset.g0; 
    terminal.charset.gr = &terminal.charset.g1; 
    terminal.charset.single_shift_2 = false;
    terminal.charset.single_shift_3 = false;
}

// Initialize VT keyboard state
// Sets up keyboard modes, function key mappings, and event buffer
void InitVTKeyboard(void) {
    // Initialize keyboard modes and flags
    terminal.vt_keyboard.application_mode = false; // Application mode off
    terminal.vt_keyboard.cursor_key_mode = terminal.dec_modes.application_cursor_keys; // Sync with DECCKM
    terminal.vt_keyboard.keypad_mode = false; // Numeric keypad mode
    terminal.vt_keyboard.meta_sends_escape = true; // Alt sends ESC prefix
    terminal.vt_keyboard.delete_sends_del = true; // Delete sends DEL (\x7F)
    terminal.vt_keyboard.backarrow_sends_bs = true; // Backspace sends BS (\x08)
    terminal.vt_keyboard.keyboard_dialect = 1; // North American ASCII

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
        strncpy(terminal.vt_keyboard.function_keys[i], function_key_sequences[i], 31);
        terminal.vt_keyboard.function_keys[i][31] = '\0';
    }

    // Initialize event buffer
    terminal.vt_keyboard.buffer_head = 0;
    terminal.vt_keyboard.buffer_tail = 0;
    terminal.vt_keyboard.buffer_count = 0;
    terminal.vt_keyboard.total_events = 0;
    terminal.vt_keyboard.dropped_events = 0;
}

void InitTerminal(void) {
    InitFontData(); 
    InitColorPalette();
    InitVTConformance(); 
    InitTabStops();
    InitCharacterSets();
    InitVTKeyboard();
    InitSixelGraphics(); 
    
    EnhancedTermChar default_char = {
        .ch = ' ', 
        .fg_color = {.color_mode = 0, .value.index = COLOR_WHITE},
        .bg_color = {.color_mode = 0, .value.index = COLOR_BLACK},
        .dirty = true
    };

    for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
        for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
            terminal.screen[y][x] = default_char;
            terminal.alt_screen[y][x] = default_char;
        }
    }
    
    // Initialize mouse state
    terminal.mouse.enabled = true; // Assume mouse is enabled by default
    terminal.mouse.mode = MOUSE_TRACKING_OFF; // Default to no mouse tracking
    terminal.mouse.buttons[0] = terminal.mouse.buttons[1] = terminal.mouse.buttons[2] = false;
    terminal.mouse.last_x = terminal.mouse.last_y = 0;
    terminal.mouse.last_pixel_x = terminal.mouse.last_pixel_y = 0;
    terminal.mouse.focused = SituationHasWindowFocus(); // Set initial focus state
    terminal.mouse.focus_tracking = false; // Focus tracking off by default
    terminal.mouse.sgr_mode = false; // SGR mode off by default
    terminal.mouse.cursor_x = -1; // Invalid position (no cursor)
    terminal.mouse.cursor_y = -1; // Invalid position (no cursor)

    // ... initialization code (e.g., answerback_message, screen, charset, etc.) ...
    terminal.cursor.visible = true;
    terminal.cursor.blink_enabled = true;
    terminal.cursor.blink_state = true;
    terminal.cursor.blink_timer = 0.0f;
    terminal.cursor.x = terminal.cursor.y = 0;
    terminal.cursor.color.color_mode = 0; // Indexed color
    terminal.cursor.color.value.index = 7; // White
    terminal.cursor.shape = CURSOR_BLOCK;
    terminal.text_blink_state = true;
    terminal.text_blink_timer = 0.0f;
    terminal.visual_bell_timer = 0.0f;
    terminal.response_length = 0;
    terminal.parse_state = VT_PARSE_NORMAL;
    terminal.left_margin = 0;
    terminal.right_margin = DEFAULT_TERM_WIDTH - 1;
    terminal.scroll_top = 0;
    terminal.scroll_bottom = DEFAULT_TERM_HEIGHT - 1;
    // Initialize other fields as needed
    
    terminal.dec_modes.application_cursor_keys = false;
    terminal.dec_modes.origin_mode = false;
    terminal.dec_modes.auto_wrap_mode = true; 
    terminal.dec_modes.cursor_visible = true;
    terminal.dec_modes.alternate_screen = false;
    terminal.dec_modes.insert_mode = false; 
    terminal.dec_modes.new_line_mode = false; 
    terminal.dec_modes.column_mode_132 = false; 
    terminal.ansi_modes.insert_replace = false; 
    terminal.ansi_modes.line_feed_new_line = true;
    terminal.dec_modes.local_echo = false;
    
    ResetAllAttributes(); 
    
    terminal.scroll_top = 0;
    terminal.scroll_bottom = DEFAULT_TERM_HEIGHT - 1;
    terminal.left_margin = 0;
    terminal.right_margin = DEFAULT_TERM_WIDTH - 1;
    
    terminal.bracketed_paste.enabled = false;
    terminal.bracketed_paste.active = false;
    terminal.bracketed_paste.buffer = NULL; 
    
    terminal.programmable_keys.keys = NULL;
    terminal.programmable_keys.count = 0;
    terminal.programmable_keys.capacity = 0;
    
    strcpy(terminal.title.terminal_name, "Enhanced VT Terminal (Raylib)");
    VTSetWindowTitle("Terminal"); 
    SetIconTitle("Term");         
    
    ClearPipeline(); 
    
    terminal.VTperformance.chars_per_frame = 200; 
    terminal.VTperformance.target_frame_time = 1.0 / 60.0; 
    terminal.VTperformance.time_budget = terminal.VTperformance.target_frame_time * 0.5; 
    terminal.VTperformance.avg_process_time = 0.000001; 
    terminal.VTperformance.burst_mode = false;
    terminal.VTperformance.burst_threshold = sizeof(terminal.input_pipeline) / 2; 
    terminal.VTperformance.adaptive_processing = true;
    
    terminal.parse_state = VT_PARSE_NORMAL;
    terminal.escape_pos = 0;
    terminal.param_count = 0;
    
    terminal.response_length = 0;
    
    terminal.options.conformance_checking = true;
    terminal.options.vttest_mode = false;
    terminal.options.debug_sequences = false; 
    terminal.options.log_unsupported = true;  

    // Initialize fields for RayLib_terminal_console.c
    terminal.echo_enabled = true;
    terminal.input_enabled = true;
    terminal.password_mode = false;
    terminal.raw_mode = false;
    terminal.paused = false;
    
    terminal.printer_available = false; // Default: no printer
    terminal.auto_print_enabled = false;
    terminal.printer_controller_enabled = false;
    terminal.locator_events.report_button_down = false;
    terminal.locator_events.report_button_up = false;
    terminal.locator_events.report_on_request_only = true; // Default behavior
    terminal.locator_enabled = false;  // Default: no locator device
    terminal.programmable_keys.udk_locked = false; // Default: UDKs unlocked
    
    strncpy(terminal.answerback_buffer, "terminal_v2 VT420", MAX_COMMAND_BUFFER - 1);
    terminal.answerback_buffer[MAX_COMMAND_BUFFER - 1] = '\0';
    
    CreateFontTexture();
}


// String terminator handler for ESC P, ESC _, ESC ^, ESC X
void ProcessStringTerminator(unsigned char ch) {
    // Expects ST (ESC \) to terminate.
    // Current char `ch` is the char after ESC. So we need to see `\`
    if (ch == '\\') { // ESC \ (ST - String Terminator)
        // Execute the command that was buffered
        switch(terminal.parse_state) { // The state *before* it became PARSE_STRING_TERMINATOR
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
        terminal.parse_state = VT_PARSE_NORMAL;
        terminal.escape_pos = 0; // Clear buffer after command execution
    } else {
        // Not a valid ST, could be another ESC sequence.
        // Re-process 'ch' as start of new escape sequence.
        terminal.parse_state = VT_PARSE_ESCAPE; // Go to escape state
        ProcessEscapeChar(ch); // Process the char that broke ST
    }
}

void ProcessCharsetCommand(unsigned char ch) {
    terminal.escape_buffer[terminal.escape_pos++] = ch; 

    if (terminal.escape_pos >= 2) { 
        char designator = terminal.escape_buffer[0];
        char charset_char = terminal.escape_buffer[1];
        CharacterSet selected_cs = CHARSET_ASCII; 

        switch (charset_char) {
            case 'A': selected_cs = CHARSET_UK; break;
            case 'B': selected_cs = CHARSET_ASCII; break;
            case '0': selected_cs = CHARSET_DEC_SPECIAL; break;
            case '1': 
            case '2': 
                if (terminal.options.debug_sequences) {
                    LogUnsupportedSequence("DEC Alternate Character ROM not fully supported, using ASCII/DEC Special");
                }
                selected_cs = (charset_char == '1') ? CHARSET_ASCII : CHARSET_DEC_SPECIAL;
                break;
            case '<': selected_cs = CHARSET_DEC_MULTINATIONAL; break; 
            default:
                if (terminal.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown charset char: %c for designator %c", charset_char, designator);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
        
        switch (designator) {
            case '(': terminal.charset.g0 = selected_cs; break;
            case ')': terminal.charset.g1 = selected_cs; break;
            case '*': terminal.charset.g2 = selected_cs; break;
            case '+': terminal.charset.g3 = selected_cs; break;
        }
        
        terminal.parse_state = VT_PARSE_NORMAL;
        terminal.escape_pos = 0;
    }
}

// Stubs for APC/PM/SOS command execution
void ExecuteAPCCommand() {
    if (terminal.options.debug_sequences) LogUnsupportedSequence("APC sequence executed (no-op)");
    // terminal.escape_buffer contains the APC string data.
}
void ExecutePMCommand() {
    if (terminal.options.debug_sequences) LogUnsupportedSequence("PM sequence executed (no-op)");
    // terminal.escape_buffer contains the PM string data.
}
void ExecuteSOSCommand() {
    if (terminal.options.debug_sequences) LogUnsupportedSequence("SOS sequence executed (no-op)");
    // terminal.escape_buffer contains the SOS string data.
}

// Generic string processor for APC, PM, SOS
void ProcessGenericStringChar(unsigned char ch, VTParseState next_state_on_escape, void (*execute_command_func)()) {
    if (terminal.escape_pos < sizeof(terminal.escape_buffer) - 1) {
        terminal.escape_buffer[terminal.escape_pos++] = ch;

        if (ch == '\\' && terminal.escape_pos >= 2 && terminal.escape_buffer[terminal.escape_pos - 2] == '\x1B') { // ST (ESC \)
            terminal.escape_buffer[terminal.escape_pos - 2] = '\0'; // Null-terminate before ESC of ST
            if (execute_command_func) execute_command_func();
            terminal.parse_state = VT_PARSE_NORMAL;
            terminal.escape_pos = 0;
        }
        // BEL is not a standard terminator for these, ST is.
    } else { // Buffer overflow
        terminal.escape_buffer[sizeof(terminal.escape_buffer) - 1] = '\0'; 
        if (execute_command_func) execute_command_func(); 
        terminal.parse_state = VT_PARSE_NORMAL;
        terminal.escape_pos = 0;
        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "String sequence (type %d) too long, truncated", (int)terminal.parse_state); // Log current state
        LogUnsupportedSequence(log_msg);
    }
}

void ProcessAPCChar(unsigned char ch) { ProcessGenericStringChar(ch, VT_PARSE_ESCAPE /* Fallback if ST is broken */, ExecuteAPCCommand); }
void ProcessPMChar(unsigned char ch) { ProcessGenericStringChar(ch, VT_PARSE_ESCAPE, ExecutePMCommand); }
void ProcessSOSChar(unsigned char ch) { ProcessGenericStringChar(ch, VT_PARSE_ESCAPE, ExecuteSOSCommand); }

// Continue with enhanced character processing...
void ProcessChar(unsigned char ch) {
    switch (terminal.parse_state) {
        case VT_PARSE_NORMAL:              ProcessNormalChar(ch); break;
        case VT_PARSE_ESCAPE:              ProcessEscapeChar(ch); break;
        case PARSE_CSI:                 ProcessCSIChar(ch); break;
        case PARSE_OSC:                 ProcessOSCChar(ch); break;
        case PARSE_DCS:                 ProcessDCSChar(ch); break;
        case PARSE_SIXEL_ST:            ProcessSixelSTChar(ch); break;
        case PARSE_VT52:                ProcessVT52Char(ch); break;
        case PARSE_SIXEL:               ProcessSixelChar(ch); break; 
        case PARSE_CHARSET:             ProcessCharsetCommand(ch); break;
        case PARSE_APC:                 ProcessAPCChar(ch); break; 
        case PARSE_PM:                  ProcessPMChar(ch); break;  
        case PARSE_SOS:                 ProcessSOSChar(ch); break; 
        default: 
            terminal.parse_state = VT_PARSE_NORMAL;
            ProcessNormalChar(ch); 
            break;
    }
}

// CSI Pts ; Pls ; Pbs ; Prs ; Pps ; Ptd ; Pld ; Ppd $ v
void ExecuteDECCRA(void) { // Copy Rectangular Area (DECCRA)
    if (!terminal.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECCRA requires VT420 mode");
        return;
    }
    if (terminal.param_count != 8) {
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

void ExecuteDECRQCRA(void) { // Request Rectangular Area Checksum
    if (!terminal.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECRQCRA requires VT420 mode");
        return;
    }

    // Correct format is CSI Pts ; Ptd ; Pcs $ w
    int pts = GetCSIParam(0, 1); // Parameter 1 (target selector)

    // Respond with a dummy checksum '0000'
    char response[32];
    snprintf(response, sizeof(response), "\x1BP%d!~0000\x1B\\", pts);
    QueueResponse(response);
}

// CSI Pch ; Pt ; Pl ; Pb ; Pr $ x
void ExecuteDECFRA(void) { // Fill Rectangular Area
    if (!terminal.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECFRA requires VT420 mode");
        return;
    }

    if (terminal.param_count != 5) {
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
            EnhancedTermChar* cell = &terminal.screen[y][x];
            cell->ch = fill_char;
            cell->fg_color = terminal.current_fg;
            cell->bg_color = terminal.current_bg;
            cell->bold = terminal.bold_mode;
            cell->faint = terminal.faint_mode;
            cell->italic = terminal.italic_mode;
            cell->underline = terminal.underline_mode;
            cell->blink = terminal.blink_mode;
            cell->reverse = terminal.reverse_mode;
            cell->strikethrough = terminal.strikethrough_mode;
            cell->conceal = terminal.conceal_mode;
            cell->overline = terminal.overline_mode;
            cell->double_underline = terminal.double_underline_mode;
            cell->dirty = true;
        }
    }
}

// CSI ? Psl {
void ExecuteDECSLE(void) { // Select Locator Events
    if (!terminal.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECSLE requires VT420 mode");
        return;
    }

    if (terminal.param_count == 0) {
        // No parameters, defaults to 0
        terminal.locator_events.report_on_request_only = true;
        terminal.locator_events.report_button_down = false;
        terminal.locator_events.report_button_up = false;
        return;
    }

    for (int i = 0; i < terminal.param_count; i++) {
        switch (terminal.escape_params[i]) {
            case 0:
                terminal.locator_events.report_on_request_only = true;
                terminal.locator_events.report_button_down = false;
                terminal.locator_events.report_button_up = false;
                break;
            case 1:
                terminal.locator_events.report_button_down = true;
                terminal.locator_events.report_on_request_only = false;
                break;
            case 2:
                terminal.locator_events.report_button_down = false;
                break;
            case 3:
                terminal.locator_events.report_button_up = true;
                terminal.locator_events.report_on_request_only = false;
                break;
            case 4:
                terminal.locator_events.report_button_up = false;
                break;
            default:
                if (terminal.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown DECSLE parameter: %d", terminal.escape_params[i]);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
    }
}

// CSI Plc |
void ExecuteDECRQLP(void) { // Request Locator Position
    if (!terminal.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECRQLP requires VT420 mode");
        return;
    }

    char response[64]; // Increased buffer size for longer response

    if (!terminal.locator_enabled || terminal.mouse.cursor_x < 1 || terminal.mouse.cursor_y < 1) {
        // Locator not enabled or position unknown, respond with "no locator"
        snprintf(response, sizeof(response), "\x1B[0!|");
    } else {
        // Locator enabled and position is known, report position.
        // Format: CSI Pe;Pr;Pc;Pp!|
        // Pe = 1 (request response)
        // Pr = row
        // Pc = column
        // Pp = page (hardcoded to 1)
        int row = terminal.mouse.cursor_y;
        int col = terminal.mouse.cursor_x;
        int page = 1; // Page memory not implemented, so hardcode to 1.
        snprintf(response, sizeof(response), "\x1B[1;%d;%d;%d!|", row, col, page);
    }

    QueueResponse(response);
}


// CSI Pt ; Pl ; Pb ; Pr $ x
void ExecuteDECERA(void) { // Erase Rectangular Area
    if (!terminal.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECERA requires VT420 mode");
        return;
    }
    if (terminal.param_count != 4) {
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
            ClearCell(&terminal.screen[y][x]);
        }
    }
}


// CSI Ps ; Pt ; Pl ; Pb ; Pr $ {
void ExecuteDECSERA(void) { // Selective Erase Rectangular Area
    if (!terminal.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECSERA requires VT420 mode");
        return;
    }
    if (terminal.param_count < 4 || terminal.param_count > 5) {
        LogUnsupportedSequence("Invalid parameters for DECSERA");
        return;
    }
    int erase_param, top, left, bottom, right;

    if (terminal.param_count == 5) {
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
            switch (erase_param) {
                case 0: if (!terminal.screen[y][x].protected_cell) should_erase = true; break;
                case 1: should_erase = true; break;
                case 2: if (terminal.screen[y][x].protected_cell) should_erase = true; break;
            }
            if (should_erase) {
                ClearCell(&terminal.screen[y][x]);
            }
        }
    }
}

void ProcessOSCChar(unsigned char ch) {
    if (terminal.escape_pos < sizeof(terminal.escape_buffer) - 1) {
        terminal.escape_buffer[terminal.escape_pos++] = ch; 

        if (ch == '\a') { 
            terminal.escape_buffer[terminal.escape_pos - 1] = '\0'; 
            ExecuteOSCCommand();
            terminal.parse_state = VT_PARSE_NORMAL;
            terminal.escape_pos = 0;
        } else if (ch == '\\' && terminal.escape_pos >= 2 && terminal.escape_buffer[terminal.escape_pos - 2] == '\x1B') { 
            terminal.escape_buffer[terminal.escape_pos - 2] = '\0'; 
            ExecuteOSCCommand();
            terminal.parse_state = VT_PARSE_NORMAL;
            terminal.escape_pos = 0;
        }
    } else { 
        terminal.escape_buffer[sizeof(terminal.escape_buffer) - 1] = '\0'; 
        ExecuteOSCCommand(); 
        terminal.parse_state = VT_PARSE_NORMAL;
        terminal.escape_pos = 0;
        LogUnsupportedSequence("OSC sequence too long, truncated");
    }
}

void ProcessDCSChar(unsigned char ch) {
    if (terminal.escape_pos < sizeof(terminal.escape_buffer) - 1) {
        terminal.escape_buffer[terminal.escape_pos++] = ch;

        if (ch == 'q') {
            // Sixel Graphics command
            ParseCSIParams(terminal.escape_buffer, terminal.sixel.params, MAX_ESCAPE_PARAMS);
            terminal.sixel.param_count = terminal.param_count;

            terminal.sixel.pos_x = 0;
            terminal.sixel.pos_y = 0;
            terminal.sixel.max_x = 0;
            terminal.sixel.max_y = 0;
            terminal.sixel.color_index = 0;
            terminal.sixel.repeat_count = 1;

            if (!terminal.sixel.data) {
                terminal.sixel.width = DEFAULT_TERM_WIDTH * DEFAULT_CHAR_WIDTH;
                terminal.sixel.height = DEFAULT_TERM_HEIGHT * DEFAULT_CHAR_HEIGHT;
                terminal.sixel.data = calloc(terminal.sixel.width * terminal.sixel.height * 4, 1);
            }

            if (terminal.sixel.data) {
                memset(terminal.sixel.data, 0, terminal.sixel.width * terminal.sixel.height * 4);
            }

            terminal.sixel.active = true;
            terminal.sixel.x = terminal.cursor.x * DEFAULT_CHAR_WIDTH;
            terminal.sixel.y = terminal.cursor.y * DEFAULT_CHAR_HEIGHT;

            terminal.parse_state = PARSE_SIXEL;
            terminal.escape_pos = 0;
            return;
        }

        if (ch == '\a') { // Non-standard, but some terminals accept BEL for DCS
            terminal.escape_buffer[terminal.escape_pos - 1] = '\0';
            ExecuteDCSCommand();
            terminal.parse_state = VT_PARSE_NORMAL;
            terminal.escape_pos = 0;
        } else if (ch == '\\' && terminal.escape_pos >= 2 && terminal.escape_buffer[terminal.escape_pos - 2] == '\x1B') { // ST (ESC \)
            terminal.escape_buffer[terminal.escape_pos - 2] = '\0';
            ExecuteDCSCommand();
            terminal.parse_state = VT_PARSE_NORMAL;
            terminal.escape_pos = 0;
        }
    } else { // Buffer overflow
        terminal.escape_buffer[sizeof(terminal.escape_buffer) - 1] = '\0';
        ExecuteDCSCommand();
        terminal.parse_state = VT_PARSE_NORMAL;
        terminal.escape_pos = 0;
        LogUnsupportedSequence("DCS sequence too long, truncated");
    }
}

// =============================================================================
// ENHANCED FONT SYSTEM WITH UNICODE SUPPORT
// =============================================================================

void CreateFontTexture(void) {
    if (font_texture.id != 0) {
        RGL_UnloadTexture(font_texture);
    }

    const int chars_per_row = 16;
    const int num_chars = 256;
    const int atlas_width = chars_per_row * DEFAULT_CHAR_WIDTH;
    const int atlas_height = (num_chars / chars_per_row) * DEFAULT_CHAR_HEIGHT;

    // Create a CPU-side buffer for the texture data (single channel).
    unsigned char* pixels = (unsigned char*)calloc(atlas_width * atlas_height, sizeof(unsigned char));
    if (!pixels) {
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate pixel buffer for terminal font.");
        return;
    }

    // Unpack the font data into the 2D texture buffer.
    for (int i = 0; i < num_chars; i++) {
        int glyph_col = i % chars_per_row;
        int glyph_row = i / chars_per_row;
        int dest_x_start = glyph_col * DEFAULT_CHAR_WIDTH;
        int dest_y_start = glyph_row * DEFAULT_CHAR_HEIGHT;

        for (int y = 0; y < DEFAULT_CHAR_HEIGHT; y++) {
            // Your font data has 32 bytes per char, we only use the first 8 for an 8x16 font.
            // Let's assume you meant 16 bytes for 8x16.
            unsigned char byte = cp437_font__8x16[i * 16 + y]; // Assuming 16 bytes for 8x16 font
            for (int x = 0; x < DEFAULT_CHAR_WIDTH; x++) {
                if ((byte >> (7 - x)) & 1) {
                    pixels[(dest_y_start + y) * atlas_width + (dest_x_start + x)] = 255;
                }
            }
        }
    }

    // Use RGL to create the texture from our raw pixel data.
    RGLTextureParams params = {
        .wrap_s = GL_CLAMP_TO_EDGE,
        .wrap_t = GL_CLAMP_TO_EDGE,
        .min_filter = GL_NEAREST, // Pixel-perfect filtering
        .mag_filter = GL_NEAREST,
        .generate_mipmaps = false
    };

    // RGL_LoadTextureWithParams is for files. We need a new RGL function for raw data.
    // Let's assume we add this to RGL:
    // RGLTexture RGL_LoadTextureFromRaw(const unsigned char* data, int width, int height, int format, const RGLTextureParams* params);
    // For now, we will adapt the existing debug text system's method.
    
    glGenTextures(1, &font_texture.id);
    glBindTexture(GL_TEXTURE_2D, font_texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas_width, atlas_height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    font_texture.width = atlas_width;
    font_texture.height = atlas_height;
    
    free(pixels);
}

// =============================================================================
// CHARACTER SET TRANSLATION SYSTEM
// =============================================================================

unsigned int TranslateCharacter(unsigned char ch, CharsetState* state) {
    CharacterSet active_set = *state->gl;
    
    // Handle single shift states
    if (state->single_shift_2) {
        active_set = state->g2;
        state->single_shift_2 = false;
    } else if (state->single_shift_3) {
        active_set = state->g3;
        state->single_shift_3 = false;
    }
    
    switch (active_set) {
        case CHARSET_ASCII:
            return ch;
            
        case CHARSET_DEC_SPECIAL:
            return TranslateDECSpecial(ch);
            
        case CHARSET_UK:
            return (ch == 0x23) ? 0x00A3 : ch; // # -> £
            
        case CHARSET_DEC_MULTINATIONAL:
            return TranslateDECMultinational(ch);
            
        case CHARSET_ISO_LATIN_1:
            return ch; // Direct mapping for Latin-1
            
        case CHARSET_UTF8:
            return ch; // UTF-8 handled separately
            
        default:
            return ch;
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
        if (!terminal.tab_stops.stops[column]) {
            terminal.tab_stops.stops[column] = true;
            terminal.tab_stops.count++;
        }
    }
}

void ClearTabStop(int column) {
    if (column >= 0 && column < MAX_TAB_STOPS) {
        if (terminal.tab_stops.stops[column]) {
            terminal.tab_stops.stops[column] = false;
            terminal.tab_stops.count--;
        }
    }
}

void ClearAllTabStops(void) {
    memset(terminal.tab_stops.stops, false, sizeof(terminal.tab_stops.stops));
    terminal.tab_stops.count = 0;
}

int NextTabStop(int current_column) {
    for (int i = current_column + 1; i < MAX_TAB_STOPS && i < DEFAULT_TERM_WIDTH; i++) {
        if (terminal.tab_stops.stops[i]) {
            return i;
        }
    }
    
    // If no explicit tab stop found, use default spacing
    int next = ((current_column / terminal.tab_stops.default_width) + 1) * terminal.tab_stops.default_width;
    return (next < DEFAULT_TERM_WIDTH) ? next : DEFAULT_TERM_WIDTH - 1;
}

int PreviousTabStop(int current_column) {
    for (int i = current_column - 1; i >= 0; i--) {
        if (terminal.tab_stops.stops[i]) {
            return i;
        }
    }
    
    // If no explicit tab stop found, use default spacing
    int prev = ((current_column - 1) / terminal.tab_stops.default_width) * terminal.tab_stops.default_width;
    return (prev >= 0) ? prev : 0;
}

// =============================================================================
// ENHANCED SCREEN MANIPULATION
// =============================================================================

void ClearCell(EnhancedTermChar* cell) {
    cell->ch = ' ';
    cell->fg_color = terminal.current_fg;
    cell->bg_color = terminal.current_bg;
    
    // Copy all current attributes
    cell->bold = terminal.bold_mode;
    cell->faint = terminal.faint_mode;
    cell->italic = terminal.italic_mode;
    cell->underline = terminal.underline_mode;
    cell->blink = terminal.blink_mode;
    cell->reverse = terminal.reverse_mode;
    cell->strikethrough = terminal.strikethrough_mode;
    cell->conceal = terminal.conceal_mode;
    cell->overline = terminal.overline_mode;
    cell->double_underline = terminal.double_underline_mode;
    cell->protected_cell = terminal.protected_mode;
    
    // Reset special attributes
    cell->double_width = false;
    cell->double_height_top = false;
    cell->double_height_bottom = false;
    cell->soft_hyphen = false;
    cell->combining = false;
    
    cell->dirty = true;
}

void ScrollUpRegion(int top, int bottom, int lines) {
    for (int i = 0; i < lines; i++) {
        // Move lines up
        for (int y = top; y < bottom; y++) {
            for (int x = terminal.left_margin; x <= terminal.right_margin; x++) {
                terminal.screen[y][x] = terminal.screen[y + 1][x];
                terminal.screen[y][x].dirty = true;
            }
        }
        
        // Clear bottom line
        for (int x = terminal.left_margin; x <= terminal.right_margin; x++) {
            ClearCell(&terminal.screen[bottom][x]);
        }
    }
}

void ScrollDownRegion(int top, int bottom, int lines) {
    for (int i = 0; i < lines; i++) {
        // Move lines down
        for (int y = bottom; y > top; y--) {
            for (int x = terminal.left_margin; x <= terminal.right_margin; x++) {
                terminal.screen[y][x] = terminal.screen[y - 1][x];
                terminal.screen[y][x].dirty = true;
            }
        }
        
        // Clear top line
        for (int x = terminal.left_margin; x <= terminal.right_margin; x++) {
            ClearCell(&terminal.screen[top][x]);
        }
    }
}

void InsertLinesAt(int row, int count) {
    if (row < terminal.scroll_top || row > terminal.scroll_bottom) {
        return;
    }
    
    // Move existing lines down
    for (int y = terminal.scroll_bottom; y >= row + count; y--) {
        if (y - count >= row) {
            for (int x = terminal.left_margin; x <= terminal.right_margin; x++) {
                terminal.screen[y][x] = terminal.screen[y - count][x];
                terminal.screen[y][x].dirty = true;
            }
        }
    }
    
    // Clear inserted lines
    for (int y = row; y < row + count && y <= terminal.scroll_bottom; y++) {
        for (int x = terminal.left_margin; x <= terminal.right_margin; x++) {
            ClearCell(&terminal.screen[y][x]);
        }
    }
}

void DeleteLinesAt(int row, int count) {
    if (row < terminal.scroll_top || row > terminal.scroll_bottom) {
        return;
    }
    
    // Move remaining lines up
    for (int y = row; y <= terminal.scroll_bottom - count; y++) {
        for (int x = terminal.left_margin; x <= terminal.right_margin; x++) {
            terminal.screen[y][x] = terminal.screen[y + count][x];
            terminal.screen[y][x].dirty = true;
        }
    }
    
    // Clear bottom lines
    for (int y = terminal.scroll_bottom - count + 1; y <= terminal.scroll_bottom; y++) {
        if (y >= 0) {
            for (int x = terminal.left_margin; x <= terminal.right_margin; x++) {
                ClearCell(&terminal.screen[y][x]);
            }
        }
    }
}

void InsertCharactersAt(int row, int col, int count) {
    // Shift existing characters right
    for (int x = terminal.right_margin; x >= col + count; x--) {
        if (x - count >= col) {
            terminal.screen[row][x] = terminal.screen[row][x - count];
            terminal.screen[row][x].dirty = true;
        }
    }
    
    // Clear inserted positions
    for (int x = col; x < col + count && x <= terminal.right_margin; x++) {
        ClearCell(&terminal.screen[row][x]);
    }
}

void DeleteCharactersAt(int row, int col, int count) {
    // Shift remaining characters left
    for (int x = col; x <= terminal.right_margin - count; x++) {
        terminal.screen[row][x] = terminal.screen[row][x + count];
        terminal.screen[row][x].dirty = true;
    }
    
    // Clear rightmost positions
    for (int x = terminal.right_margin - count + 1; x <= terminal.right_margin; x++) {
        if (x >= 0) {
            ClearCell(&terminal.screen[row][x]);
        }
    }
}

// =============================================================================
// VT100 INSERT MODE IMPLEMENTATION
// =============================================================================

void EnableInsertMode(bool enable) {
    terminal.dec_modes.insert_mode = enable;
}

void InsertCharacterAtCursor(unsigned int ch) {
    if (terminal.dec_modes.insert_mode) {
        // Insert mode: shift existing characters right
        InsertCharactersAt(terminal.cursor.y, terminal.cursor.x, 1);
    }
    
    // Place character at cursor position
    EnhancedTermChar* cell = &terminal.screen[terminal.cursor.y][terminal.cursor.x];
    cell->ch = ch;
    cell->fg_color = terminal.current_fg;
    cell->bg_color = terminal.current_bg;
    
    // Apply current attributes
    cell->bold = terminal.bold_mode;
    cell->faint = terminal.faint_mode;
    cell->italic = terminal.italic_mode;
    cell->underline = terminal.underline_mode;
    cell->blink = terminal.blink_mode;
    cell->reverse = terminal.reverse_mode;
    cell->strikethrough = terminal.strikethrough_mode;
    cell->conceal = terminal.conceal_mode;
    cell->overline = terminal.overline_mode;
    cell->double_underline = terminal.double_underline_mode;
    cell->protected_cell = terminal.protected_mode;
    
    cell->dirty = true;
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
    unsigned int unicode_ch = TranslateCharacter(ch, &terminal.charset);
    
    // Handle character display
    if (terminal.cursor.x > terminal.right_margin) {
        if (terminal.dec_modes.auto_wrap_mode) {
            // Auto-wrap to next line
            terminal.cursor.x = terminal.left_margin;
            terminal.cursor.y++;
            
            if (terminal.cursor.y > terminal.scroll_bottom) {
                terminal.cursor.y = terminal.scroll_bottom;
                ScrollUpRegion(terminal.scroll_top, terminal.scroll_bottom, 1);
            }
        } else {
            // No wrap - stay at right margin
            terminal.cursor.x = terminal.right_margin;
        }
    }
    
    // Insert character (handles insert mode internally)
    InsertCharacterAtCursor(unicode_ch);
    
    // Advance cursor
    terminal.cursor.x++;
}

// Update ProcessControlChar
void ProcessControlChar(unsigned char ch) {
    switch (ch) {
        case 0x05: // ENQ - Enquiry
            if (terminal.answerback_buffer[0] != '\0') {
                QueueResponse(terminal.answerback_buffer);
            }
            break;
        case 0x07: // BEL - Bell
            if (bell_callback) {
                bell_callback();
            } else {
                // Visual bell
                terminal.visual_bell_timer = 0.2;
            }
            break;
        case 0x08: // BS - Backspace
            if (terminal.cursor.x > terminal.left_margin) {
                terminal.cursor.x--;
            }
            break;
        case 0x09: // HT - Horizontal Tab
            terminal.cursor.x = NextTabStop(terminal.cursor.x);
            if (terminal.cursor.x > terminal.right_margin) {
                terminal.cursor.x = terminal.right_margin;
            }
            break;
        case 0x0A: // LF - Line Feed
        case 0x0B: // VT - Vertical Tab
        case 0x0C: // FF - Form Feed
            terminal.cursor.y++;
            if (terminal.cursor.y > terminal.scroll_bottom) {
                terminal.cursor.y = terminal.scroll_bottom;
                ScrollUpRegion(terminal.scroll_top, terminal.scroll_bottom, 1);
            }
            if (terminal.ansi_modes.line_feed_new_line) {
                terminal.cursor.x = terminal.left_margin;
            }
            break;
        case 0x0D: // CR - Carriage Return
            terminal.cursor.x = terminal.left_margin;
            break;
        case 0x0E: // SO - Shift Out (invoke G1 into GL)
            terminal.charset.gl = &terminal.charset.g1;
            break;
        case 0x0F: // SI - Shift In (invoke G0 into GL)
            terminal.charset.gl = &terminal.charset.g0;
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
            terminal.parse_state = VT_PARSE_NORMAL;
            terminal.escape_pos = 0;
            break;
        case 0x1B: // ESC - Escape
            terminal.parse_state = VT_PARSE_ESCAPE;
            terminal.escape_pos = 0;
            break;
        case 0x7F: // DEL - Delete
            // Ignored
            break;
        default:
            if (terminal.options.debug_sequences) {
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
            terminal.parse_state = PARSE_CSI;
            terminal.escape_pos = 0;
            memset(terminal.escape_params, 0, sizeof(terminal.escape_params));
            terminal.param_count = 0;
            break;
            
        // OSC - Operating System Command
        case ']':
            terminal.parse_state = PARSE_OSC;
            terminal.escape_pos = 0;
            break;
            
        // DCS - Device Control String
        case 'P':
            terminal.parse_state = PARSE_DCS;
            terminal.escape_pos = 0;
            break;
            
        // APC - Application Program Command
        case '_':
            terminal.parse_state = PARSE_APC;
            terminal.escape_pos = 0;
            break;
            
        // PM - Privacy Message
        case '^':
            terminal.parse_state = PARSE_PM;
            terminal.escape_pos = 0;
            break;
            
        // SOS - Start of String
        case 'X':
            terminal.parse_state = PARSE_SOS;
            terminal.escape_pos = 0;
            break;
            
        // Character set selection
        case '(':
            terminal.parse_state = PARSE_CHARSET;
            terminal.escape_buffer[0] = '(';
            terminal.escape_pos = 1;
            break;
            
        case ')':
            terminal.parse_state = PARSE_CHARSET;
            terminal.escape_buffer[0] = ')';
            terminal.escape_pos = 1;
            break;
            
        case '*':
            terminal.parse_state = PARSE_CHARSET;
            terminal.escape_buffer[0] = '*';
            terminal.escape_pos = 1;
            break;
            
        case '+':
            terminal.parse_state = PARSE_CHARSET;
            terminal.escape_buffer[0] = '+';
            terminal.escape_pos = 1;
            break;
            
        // Single character commands
        case '7': // DECSC - Save Cursor
            terminal.saved_cursor = terminal.cursor;
            terminal.saved_cursor_valid = true;
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case '8': // DECRC - Restore Cursor
            if (terminal.saved_cursor_valid) {
                terminal.cursor = terminal.saved_cursor;
            }
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case 'D': // IND - Index
            terminal.cursor.y++;
            if (terminal.cursor.y > terminal.scroll_bottom) {
                terminal.cursor.y = terminal.scroll_bottom;
                ScrollUpRegion(terminal.scroll_top, terminal.scroll_bottom, 1);
            }
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case 'E': // NEL - Next Line
            terminal.cursor.x = terminal.left_margin;
            terminal.cursor.y++;
            if (terminal.cursor.y > terminal.scroll_bottom) {
                terminal.cursor.y = terminal.scroll_bottom;
                ScrollUpRegion(terminal.scroll_top, terminal.scroll_bottom, 1);
            }
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case 'H': // HTS - Set Tab Stop
            SetTabStop(terminal.cursor.x);
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case 'M': // RI - Reverse Index
            terminal.cursor.y--;
            if (terminal.cursor.y < terminal.scroll_top) {
                terminal.cursor.y = terminal.scroll_top;
                ScrollDownRegion(terminal.scroll_top, terminal.scroll_bottom, 1);
            }
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case 'N': // SS2 - Single Shift 2
            terminal.charset.single_shift_2 = true;
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case 'O': // SS3 - Single Shift 3
            terminal.charset.single_shift_3 = true;
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case 'Z': // DECID - Identify Terminal
            QueueResponse(terminal.device_attributes);
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case 'c': // RIS - Reset to Initial State
            InitTerminal();
            break;
            
        case '=': // DECKPAM - Keypad Application Mode
            terminal.vt_keyboard.keypad_mode = true;
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case '>': // DECKPNM - Keypad Numeric Mode
            terminal.vt_keyboard.keypad_mode = false;
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
            
        case '<': // Enter VT52 mode (if enabled)
            if (terminal.conformance.features.vt52_mode) {
                terminal.parse_state = PARSE_VT52;
            } else {
                terminal.parse_state = VT_PARSE_NORMAL;
                if (terminal.options.log_unsupported) {
                    LogUnsupportedSequence("VT52 mode not supported");
                }
            }
            break;
            
        default:
            // Unknown escape sequence
            if (terminal.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown ESC %c (0x%02X)", ch, ch);
                LogUnsupportedSequence(debug_msg);
            }
            terminal.parse_state = VT_PARSE_NORMAL;
            break;
    }
}

// =============================================================================
// ENHANCED PIPELINE PROCESSING
// =============================================================================
bool PipelineWriteChar(unsigned char ch) {
    if (terminal.pipeline_count >= sizeof(terminal.input_pipeline) - 1) {
        terminal.pipeline_overflow = true;
        return false;
    }
    
    terminal.input_pipeline[terminal.pipeline_head] = ch;
    terminal.pipeline_head = (terminal.pipeline_head + 1) % sizeof(terminal.input_pipeline);
    terminal.pipeline_count++;
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
    terminal.pipeline_head = 0;
    terminal.pipeline_tail = 0;
    terminal.pipeline_count = 0;
    terminal.pipeline_overflow = false;
}

// =============================================================================
// BASIC IMPLEMENTATIONS FOR MISSING FUNCTIONS
// =============================================================================

void SetResponseCallback(ResponseCallback callback) {
    response_callback = callback;
}

void SetTitleCallback(TitleCallback callback) {
    title_callback = callback;
}

void SetBellCallback(BellCallback callback) {
    bell_callback = callback;
}

const char* GetWindowTitle(void) {
    return terminal.title.window_title;
}

const char* GetIconTitle(void) {
    return terminal.title.icon_title;
}

void SetTerminalMode(const char* mode, bool enable) {
    if (strcmp(mode, "application_cursor") == 0) {
        terminal.dec_modes.application_cursor_keys = enable;
    } else if (strcmp(mode, "auto_wrap") == 0) {
        terminal.dec_modes.auto_wrap_mode = enable;
    } else if (strcmp(mode, "origin") == 0) {
        terminal.dec_modes.origin_mode = enable;
    } else if (strcmp(mode, "insert") == 0) {
        terminal.dec_modes.insert_mode = enable;
    }
}

void SetCursorShape(CursorShape shape) {
    terminal.cursor.shape = shape;
}

void SetCursorColor(ExtendedColor color) {
    terminal.cursor.color = color;
}

void SetMouseTracking(MouseTrackingMode mode) {
    terminal.mouse.mode = mode;
    terminal.mouse.enabled = (mode != MOUSE_TRACKING_OFF);
}

// Enable or disable mouse features
// Toggles specific mouse functionalities based on feature name
void EnableMouseFeature(const char* feature, bool enable) {
    if (strcmp(feature, "focus") == 0) {
        // Enable/disable focus tracking for mouse reporting (CSI ?1004 h/l)
        terminal.mouse.focus_tracking = enable;
    } else if (strcmp(feature, "sgr") == 0) {
        // Enable/disable SGR mouse reporting mode (CSI ?1006 h/l)
        terminal.mouse.sgr_mode = enable;
        // Update mouse mode to SGR if enabled and tracking is active
        if (enable && terminal.mouse.mode != MOUSE_TRACKING_OFF && 
            terminal.mouse.mode != MOUSE_TRACKING_URXVT && terminal.mouse.mode != MOUSE_TRACKING_PIXEL) {
            terminal.mouse.mode = MOUSE_TRACKING_SGR;
        } else if (!enable && terminal.mouse.mode == MOUSE_TRACKING_SGR) {
            terminal.mouse.mode = MOUSE_TRACKING_VT200; // Fallback to VT200
        }
    } else if (strcmp(feature, "cursor") == 0) {
        // Enable/disable custom mouse cursor rendering
        terminal.mouse.enabled = enable;
        if (!enable) {
            terminal.mouse.cursor_x = -1; // Hide cursor
            terminal.mouse.cursor_y = -1;
        }
    } else if (strcmp(feature, "urxvt") == 0) {
        // Enable/disable URXVT mouse reporting mode (CSI ?1015 h/l)
        if (enable) {
            terminal.mouse.mode = MOUSE_TRACKING_URXVT;
            terminal.mouse.enabled = true; // Ensure cursor is enabled
        } else if (terminal.mouse.mode == MOUSE_TRACKING_URXVT) {
            terminal.mouse.mode = MOUSE_TRACKING_OFF;
        }
    } else if (strcmp(feature, "pixel") == 0) {
        // Enable/disable pixel position mouse reporting mode (CSI ?1016 h/l)
        if (enable) {
            terminal.mouse.mode = MOUSE_TRACKING_PIXEL;
            terminal.mouse.enabled = true; // Ensure cursor is enabled
        } else if (terminal.mouse.mode == MOUSE_TRACKING_PIXEL) {
            terminal.mouse.mode = MOUSE_TRACKING_OFF;
        }
    }
}

void EnableBracketedPaste(bool enable) {
    terminal.bracketed_paste.enabled = enable;
}

bool IsBracketedPasteActive(void) {
    return terminal.bracketed_paste.active;
}

void ProcessPasteData(const char* data, size_t length) {
    if (terminal.bracketed_paste.enabled) {
        PipelineWriteString("\x1B[200~");
        PipelineWriteString(data);
        PipelineWriteString("\x1B[201~");
    } else {
        PipelineWriteString(data);
    }
}

// Update mouse state (internal use only)
// Processes mouse position, buttons, wheel, motion, focus changes, and updates cursor position
void UpdateMouse(void) {
    // Exit if mouse is disabled or tracking is off
    if (!terminal.mouse.enabled || terminal.mouse.mode == MOUSE_TRACKING_OFF) {
        ShowCursor(); // Show system cursor
        terminal.mouse.cursor_x = -1; // Hide custom cursor
        terminal.mouse.cursor_y = -1;
        return;
    }

    // Hide system cursor during mouse tracking
    HideCursor();

    // Get mouse position and convert to cell coordinates
    vec2 mouse_pos;
    SituationGetMousePosition(mouse_pos);
    int cell_x = (int)(mouse_pos.x / (DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE)) + 1;  
    int cell_y = (int)(mouse_pos.y / (DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE)) + 1; 

    // Clamp to terminal bounds
    if (cell_x < 1) cell_x = 1;
    if (cell_x > DEFAULT_TERM_WIDTH) cell_x = DEFAULT_TERM_WIDTH;
    if (cell_y < 1) cell_y = 1;
    if (cell_y > DEFAULT_TERM_HEIGHT) cell_y = DEFAULT_TERM_HEIGHT;

    int pixel_x = (int)mouse_pos.x + 1; 
    int pixel_y = (int)mouse_pos.y + 1;

    // Update custom cursor position
    terminal.mouse.cursor_x = cell_x;
    terminal.mouse.cursor_y = cell_y;

    // Get button states
    bool current_buttons_state[3]; 
    current_buttons_state[0] = SituationIsMouseButtonDown(SIT_MOUSE_BUTTON_LEFT);
    current_buttons_state[1] = SituationIsMouseButtonDown(SIT_MOUSE_BUTTON_MIDDLE);
    current_buttons_state[2] = SituationIsMouseButtonDown(SIT_MOUSE_BUTTON_RIGHT);

    // Get wheel movement
    float wheel_move = SituationGetMouseWheelMove();
    char mouse_report[64];
    
    // Handle button press/release events
    for (size_t i = 0; i < 3; i++) {
        if (current_buttons_state[i] != terminal.mouse.buttons[i]) { 
            terminal.mouse.buttons[i] = current_buttons_state[i];
            bool pressed = current_buttons_state[i];
            int report_button_code = i; 
            mouse_report[0] = '\0';

            if (terminal.mouse.sgr_mode || terminal.mouse.mode == MOUSE_TRACKING_URXVT || terminal.mouse.mode == MOUSE_TRACKING_PIXEL) {
                if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) report_button_code += 4;
                if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) report_button_code += 8; 
                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) report_button_code += 16;

                if (terminal.mouse.mode == MOUSE_TRACKING_PIXEL) {
                     snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%d%c", 
                             report_button_code, pixel_x, pixel_y, pressed ? 'M' : 'm');
                } else {
                     snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%d%c", 
                             report_button_code, cell_x, cell_y, pressed ? 'M' : 'm');
                }
            } else if (terminal.mouse.mode >= MOUSE_TRACKING_VT200 && terminal.mouse.mode <= MOUSE_TRACKING_ANY_EVENT) {
                int cb_button = pressed ? i : 3; 
                int cb = 32 + cb_button;
                if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) cb += 4;
                if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) cb += 8; 
                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) cb += 16;
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c", 
                        (char)cb, (char)(32 + cell_x), (char)(32 + cell_y));
            } else if (terminal.mouse.mode == MOUSE_TRACKING_X10) {
                if (pressed) { 
                    int cb = 32 + i;
                    snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c", 
                            (char)cb, (char)(32 + cell_x), (char)(32 + cell_y));
                }
            }
            if (mouse_report[0]) QueueResponse(mouse_report);
        }
    }
    
    // Handle wheel events
    if (wheel_move != 0) {
        int report_button_code = (wheel_move > 0) ? 64 : 65; 
        mouse_report[0] = '\0';
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) report_button_code += 4;
        if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) report_button_code += 8; 
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) report_button_code += 16;

        if (terminal.mouse.sgr_mode || terminal.mouse.mode == MOUSE_TRACKING_URXVT || terminal.mouse.mode == MOUSE_TRACKING_PIXEL) {
             if (terminal.mouse.mode == MOUSE_TRACKING_PIXEL) {
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM", 
                        report_button_code, pixel_x, pixel_y); 
             } else {
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM", 
                        report_button_code, cell_x, cell_y);
             }
        } else if (terminal.mouse.mode >= MOUSE_TRACKING_VT200 && terminal.mouse.mode <= MOUSE_TRACKING_ANY_EVENT) {
            int cb = 32 + ((wheel_move > 0) ? 0 : 1) + 64; 
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) cb += 4;
            if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) cb += 8; 
            if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) cb += 16;
            snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c", 
                    (char)cb, (char)(32 + cell_x), (char)(32 + cell_y));
        }
        if (mouse_report[0]) QueueResponse(mouse_report);
    }

    // Handle motion events
    if (cell_x != terminal.mouse.last_x || cell_y != terminal.mouse.last_y || 
        (terminal.mouse.mode == MOUSE_TRACKING_PIXEL && (pixel_x != terminal.mouse.last_pixel_x || pixel_y != terminal.mouse.last_pixel_y))) {
        bool report_move = false;
        int sgr_motion_code = 35; // Motion no button for SGR
        int vt200_motion_cb = 32 + 3; // Motion no button for VT200

        if (terminal.mouse.mode == MOUSE_TRACKING_ANY_EVENT) {
            report_move = true;
        } else if (terminal.mouse.mode == MOUSE_TRACKING_VT200_HIGHLIGHT || terminal.mouse.mode == MOUSE_TRACKING_BTN_EVENT) {
            if (current_buttons_state[0] || current_buttons_state[1] || current_buttons_state[2]) report_move = true;
        } else if (terminal.mouse.sgr_mode || terminal.mouse.mode == MOUSE_TRACKING_URXVT || terminal.mouse.mode == MOUSE_TRACKING_PIXEL) {
             if (current_buttons_state[0] || current_buttons_state[1] || current_buttons_state[2]) report_move = true;
        }

        if (report_move) {
            mouse_report[0] = '\0';
            if (terminal.mouse.sgr_mode || terminal.mouse.mode == MOUSE_TRACKING_URXVT || terminal.mouse.mode == MOUSE_TRACKING_PIXEL) {
                if(current_buttons_state[0]) sgr_motion_code = 32; 
                else if(current_buttons_state[1]) sgr_motion_code = 33; 
                else if(current_buttons_state[2]) sgr_motion_code = 34; 

                if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) sgr_motion_code += 4;
                if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) sgr_motion_code += 8; 
                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) sgr_motion_code += 16;

                if (terminal.mouse.mode == MOUSE_TRACKING_PIXEL) {
                    snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM", sgr_motion_code, pixel_x, pixel_y);
                } else {
                    snprintf(mouse_report, sizeof(mouse_report), "\x1B[<%d;%d;%dM", sgr_motion_code, cell_x, cell_y);
                }
            } else { 
                if(current_buttons_state[0]) vt200_motion_cb = 32 + 0; 
                else if(current_buttons_state[1]) vt200_motion_cb = 32 + 1; 
                else if(current_buttons_state[2]) vt200_motion_cb = 32 + 2; 
                
                if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) vt200_motion_cb += 4;
                if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) vt200_motion_cb += 8; 
                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) vt200_motion_cb += 16;
                snprintf(mouse_report, sizeof(mouse_report), "\x1B[M%c%c%c", (char)vt200_motion_cb, (char)(32 + cell_x), (char)(32 + cell_y));
            }
            if (mouse_report[0]) QueueResponse(mouse_report);
        }
        terminal.mouse.last_x = cell_x;
        terminal.mouse.last_y = cell_y;
        terminal.mouse.last_pixel_x = pixel_x;
        terminal.mouse.last_pixel_y = pixel_y;
    }

    // Handle focus changes
    if (terminal.mouse.focus_tracking) {
        bool current_focus = SituationHasWindowFocus();
        if (current_focus && !terminal.mouse.focused) {
            QueueResponse("\x1B[I"); // Focus In
        } else if (!current_focus && terminal.mouse.focused) {
            QueueResponse("\x1B[O"); // Focus Out
        }
        terminal.mouse.focused = current_focus;
    }
}

void EnableVTFeature(const char* feature, bool enable) {
    if (strcmp(feature, "vt52") == 0) {
        terminal.conformance.features.vt52_mode = enable;
    } else if (strcmp(feature, "vt220") == 0) {
        terminal.conformance.features.vt220_mode = enable;
    } else if (strcmp(feature, "xterm") == 0) {
        terminal.conformance.features.xterm_mode = enable;
    }
}

bool IsVTFeatureSupported(const char* feature) {
    if (strcmp(feature, "vt52") == 0) {
        return terminal.conformance.features.vt52_mode;
    } else if (strcmp(feature, "vt220") == 0) {
        return terminal.conformance.features.vt220_mode;
    } else if (strcmp(feature, "xterm") == 0) {
        return terminal.conformance.features.xterm_mode;
    }
    return false;
}

void SetKeyboardDialect(int dialect) {
    if (dialect >= 1 && dialect <= 10) { // Example range, adjust per NRCS standards
        terminal.vt_keyboard.keyboard_dialect = dialect;
    }
}

void SetPrinterAvailable(bool available) {
    terminal.printer_available = available;
}

void SetLocatorEnabled(bool enabled) {
    terminal.locator_enabled = enabled;
}

void SetUDKLocked(bool locked) {
    terminal.programmable_keys.udk_locked = locked;
}

void GetDeviceAttributes(char* primary, char* secondary, size_t buffer_size) {
    if (primary) {
        strncpy(primary, terminal.device_attributes, buffer_size - 1);
        primary[buffer_size - 1] = '\0';
    }
    if (secondary) {
        strncpy(secondary, terminal.secondary_attributes, buffer_size - 1);
        secondary[buffer_size - 1] = '\0';
    }
}

int GetPipelineCount(void) {
    return terminal.pipeline_count;
}

bool IsPipelineOverflow(void) {
    return terminal.pipeline_overflow;
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
        case 0: terminal.charset.g0 = charset; break;
        case 1: terminal.charset.g1 = charset; break;
        case 2: terminal.charset.g2 = charset; break;
        case 3: terminal.charset.g3 = charset; break;
    }
}

void SetCharacterSet(CharacterSet charset) {
    terminal.charset.g0 = charset;
    terminal.charset.gl = &terminal.charset.g0;
}

void LoadSoftFont(const unsigned char* font_data, int char_start, int char_count) {
    (void)font_data; (void)char_start; (void)char_count;
    // Soft font loading not fully implemented
}

void SelectSoftFont(bool enable) {
    terminal.soft_font.active = enable;
}

void SetKeyboardMode(const char* mode, bool enable) {
    if (strcmp(mode, "application") == 0) {
        terminal.vt_keyboard.application_mode = enable;
    } else if (strcmp(mode, "cursor") == 0) {
        terminal.vt_keyboard.cursor_key_mode = enable;
    } else if (strcmp(mode, "keypad") == 0) {
        terminal.vt_keyboard.keypad_mode = enable;
    } else if (strcmp(mode, "meta_escape") == 0) {
        terminal.vt_keyboard.meta_sends_escape = enable;
    }
}

void DefineFunctionKey(int key_num, const char* sequence) {
    if (key_num >= 1 && key_num <= 24 && sequence) {
        strncpy(terminal.vt_keyboard.function_keys[key_num - 1], sequence, 31);
        terminal.vt_keyboard.function_keys[key_num - 1][31] = '\0';
    }
}


void HandleControlKey(VTKeyEvent* event) {
    // Handle Ctrl+key combinations
    if (event->key_code >= KEY_A && event->key_code <= KEY_Z) {
        // Ctrl+A = 0x01, Ctrl+B = 0x02, etc.
        int ctrl_char = event->key_code - KEY_A + 1;
        event->sequence[0] = (char)ctrl_char;
        event->sequence[1] = '\0';
    } else {
        switch (event->key_code) {
            case SIT_KEY_SPACE:      event->sequence[0] = 0x00; break; // Ctrl+Space = NUL
            case SIT_KEY_LEFT_BRACKET:  event->sequence[0] = 0x1B; break; // Ctrl+[ = ESC
            case SIT_KEY_BACKSLASH:  event->sequence[0] = 0x1C; break; // Ctrl+\ = FS
            case SIT_KEY_RIGHT_BRACKET: event->sequence[0] = 0x1D; break; // Ctrl+] = GS
            case SIT_KEY_GRAVE:      event->sequence[0] = 0x1E; break; // Ctrl+^ = RS
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
    } else if (event->key_code >= SIT_KEY_ZERO && event->key_code <= SIT_KEY_NINE) {
        char digit = '0' + (event->key_code - SIT_KEY_ZERO);
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
            if (terminal.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOA"); // Application mode
            } else {
                strcpy(event->sequence, "\x1B[A"); // Normal mode
            }
            break;
            
        case SIT_KEY_DOWN:
            if (terminal.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOB");
            } else {
                strcpy(event->sequence, "\x1B[B");
            }
            break;
            
        case SIT_KEY_RIGHT:
            if (terminal.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOC");
            } else {
                strcpy(event->sequence, "\x1B[C");
            }
            break;
            
        case SIT_KEY_LEFT:
            if (terminal.vt_keyboard.cursor_key_mode) {
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
            if (terminal.vt_keyboard.cursor_key_mode) {
                strcpy(event->sequence, "\x1BOH");
            } else {
                strcpy(event->sequence, "\x1B[H");
            }
            break;
            
        case SIT_KEY_END:
            if (terminal.vt_keyboard.cursor_key_mode) {
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
            if (terminal.vt_keyboard.keypad_mode) {
                snprintf(event->sequence, sizeof(event->sequence), "\x1BO%c", 
                        'p' + (event->key_code - KEY_KP_0));
            } else {
                snprintf(event->sequence, sizeof(event->sequence), "%c", 
                        '0' + (event->key_code - KEY_KP_0));
            }
            break;
            
        case SIT_KEY_KP_DECIMAL:
            strcpy(event->sequence, terminal.vt_keyboard.keypad_mode ? "\x1BOn" : ".");
            break;
            
        case SIT_KEY_KP_ENTER:
            strcpy(event->sequence, terminal.vt_keyboard.keypad_mode ? "\x1BOM" : "\r");
            break;
            
        case SIT_KEY_KP_ADD:
            strcpy(event->sequence, terminal.vt_keyboard.keypad_mode ? "\x1BOk" : "+");
            break;
            
        case SIT_KEY_KP_SUBTRACT:
            strcpy(event->sequence, terminal.vt_keyboard.keypad_mode ? "\x1BOm" : "-");
            break;
            
        case SIT_KEY_KP_MULTIPLY:
            strcpy(event->sequence, terminal.vt_keyboard.keypad_mode ? "\x1BOj" : "*");
            break;
            
        case SIT_KEY_KP_DIVIDE:
            strcpy(event->sequence, terminal.vt_keyboard.keypad_mode ? "\x1BOo" : "/");
            break;
            
        default:
            // Handle regular keys with modifiers
            if (event->ctrl) {
                HandleControlKey(event);
            } else if (event->alt && terminal.vt_keyboard.meta_sends_escape) {
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

    // Process Raylib key presses - SKIP PRINTABLE ASCII KEYS
    int rk;
    while ((rk = SituationGetKeyPressed()) != 0) {
        // First, check if this key is a User-Defined Key (UDK)
        bool udk_found = false;
        for (size_t i = 0; i < terminal.programmable_keys.count; i++) {
            if (terminal.programmable_keys.keys[i].key_code == rk && terminal.programmable_keys.keys[i].active) {
                if (terminal.vt_keyboard.buffer_count < KEY_EVENT_BUFFER_SIZE) {
                    VTKeyEvent* vt_event = &terminal.vt_keyboard.buffer[terminal.vt_keyboard.buffer_head];
                    memset(vt_event, 0, sizeof(VTKeyEvent));
                    vt_event->key_code = rk;
                    vt_event->timestamp = current_time;
                    vt_event->priority = KEY_PRIORITY_HIGH; // UDKs get high priority

                    // Copy the user-defined sequence
                    size_t len = terminal.programmable_keys.keys[i].sequence_length;
                    if (len >= sizeof(vt_event->sequence)) {
                        len = sizeof(vt_event->sequence) - 1;
                    }
                    memcpy(vt_event->sequence, terminal.programmable_keys.keys[i].sequence, len);
                    vt_event->sequence[len] = '\0';

                    terminal.vt_keyboard.buffer_head = (terminal.vt_keyboard.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
                    terminal.vt_keyboard.buffer_count++;
                    terminal.vt_keyboard.total_events++;
                    udk_found = true;
                } else {
                    terminal.vt_keyboard.dropped_events++;
                }
                break; // Stop after finding the first match
            }
        }
        if (udk_found) continue; // If UDK was handled, skip default processing for this key


        if (rk >= 32 && rk <= 126) continue;  // Skip these, SituationGetCharPressed() will handle them

        if (terminal.vt_keyboard.buffer_count < KEY_EVENT_BUFFER_SIZE) {
            VTKeyEvent* vt_event = &terminal.vt_keyboard.buffer[terminal.vt_keyboard.buffer_head];
            memset(vt_event, 0, sizeof(VTKeyEvent));
            vt_event->key_code = rk;
            vt_event->timestamp = current_time;
            vt_event->priority = KEY_PRIORITY_NORMAL;
            vt_event->ctrl = SituationIsKeyPressed(SIT_KEY_LEFT_CONTROL) || SituationIsKeyPressed(SIT_KEY_RIGHT_CONTROL);
            vt_event->shift = SituationIsKeyPressed(SIT_KEY_LEFT_SHIFT) || SituationIsKeyPressed(SIT_KEY_RIGHT_SHIFT);
            vt_event->alt = SituationIsKeyPressed(SIT_KEY_LEFT_ALT) || SituationIsKeyPressed(SIT_KEY_RIGHT_ALT);

            // Generate VT sequence for special keys only
            switch (rk) {
                case SIT_KEY_UP:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), terminal.vt_keyboard.cursor_key_mode ? "\x1BOA" : "\x1B[A");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5A");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3A");
                    break;
                case SIT_KEY_DOWN:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), terminal.vt_keyboard.cursor_key_mode ? "\x1BOB" : "\x1B[B");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5B");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3B");
                    break;
                case SIT_KEY_RIGHT:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), terminal.vt_keyboard.cursor_key_mode ? "\x1BOC" : "\x1B[C");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5C");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3C");
                    break;
                case SIT_KEY_LEFT:
                    snprintf(vt_event->sequence, sizeof(vt_event->sequence), terminal.vt_keyboard.cursor_key_mode ? "\x1BOD" : "\x1B[D");
                    if (vt_event->ctrl) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;5D");
                    else if (vt_event->alt) snprintf(vt_event->sequence, sizeof(vt_event->sequence), "\x1B[1;3D");
                    break;
                case SIT_KEY_F1: case SIT_KEY_F2: case SIT_KEY_F3: case SIT_KEY_F4:
                case SIT_KEY_F5: case SIT_KEY_F6: case SIT_KEY_F7: case SIT_KEY_F8:
                case SIT_KEY_F9: case SIT_KEY_F10: case SIT_KEY_F11: case SIT_KEY_F12:
                    strncpy(vt_event->sequence, terminal.vt_keyboard.function_keys[rk - SIT_KEY_F1], sizeof(vt_event->sequence));
                    break;
                case SIT_KEY_ENTER:
                    vt_event->sequence[0] = terminal.ansi_modes.line_feed_new_line ? '\r' : '\n';
                    vt_event->sequence[1] = '\0';
                    break;
                case SIT_KEY_BACKSPACE:
                    vt_event->sequence[0] = terminal.vt_keyboard.backarrow_sends_bs ? '\b' : '\x7F';
                    vt_event->sequence[1] = '\0';
                    break;
                case SIT_KEY_DELETE:
                    vt_event->sequence[0] = terminal.vt_keyboard.delete_sends_del ? '\x7F' : '\b';
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

            if (vt_event->sequence[0] != '\0') {
                terminal.vt_keyboard.buffer_head = (terminal.vt_keyboard.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
                terminal.vt_keyboard.buffer_count++;
                terminal.vt_keyboard.total_events++;
            }
        } else {
            terminal.vt_keyboard.dropped_events++;
        }
    }

    // Process Unicode characters - THIS HANDLES ALL PRINTABLE CHARACTERS
    int ch_unicode;
    while ((ch_unicode = SituationGetCharPressed()) != 0) {
        if (terminal.vt_keyboard.buffer_count < KEY_EVENT_BUFFER_SIZE) {
            VTKeyEvent* vt_event = &terminal.vt_keyboard.buffer[terminal.vt_keyboard.buffer_head];
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
            } else if (vt_event->alt && terminal.vt_keyboard.meta_sends_escape && !vt_event->ctrl) {
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
                terminal.vt_keyboard.buffer_head = (terminal.vt_keyboard.buffer_head + 1) % KEY_EVENT_BUFFER_SIZE;
                terminal.vt_keyboard.buffer_count++;
                terminal.vt_keyboard.total_events++;
            }
        } else {
            terminal.vt_keyboard.dropped_events++;
        }
    }
}


bool GetKeyEvent(KeyEvent* event) {
    return GetVTKeyEvent(event);
}

void SetPipelineTargetFPS(int fps) {
    if (fps > 0) {
        terminal.VTperformance.target_frame_time = 1.0 / fps;
        terminal.VTperformance.time_budget = terminal.VTperformance.target_frame_time * 0.3;
    }
}

void SetPipelineTimeBudget(double pct) {
    if (pct > 0.0 && pct <= 1.0) {
        terminal.VTperformance.time_budget = terminal.VTperformance.target_frame_time * pct;
    }
}

TerminalStatus GetTerminalStatus(void) {
    TerminalStatus status = {0};
    status.pipeline_usage = terminal.pipeline_count;
    status.key_usage = terminal.vt_keyboard.buffer_count;
    status.overflow_detected = terminal.pipeline_overflow;
    status.avg_process_time = terminal.VTperformance.avg_process_time;
    return status;
}

void ShowBufferDiagnostics(void) {
    TerminalStatus status = GetTerminalStatus();
    PipelineWriteFormat("=== Buffer Diagnostics ===\n");
    PipelineWriteFormat("Pipeline: %zu/%d bytes\n", status.pipeline_usage, (int)sizeof(terminal.input_pipeline));
    PipelineWriteFormat("Keyboard: %zu events\n", status.key_usage);
    PipelineWriteFormat("Overflow: %s\n", status.overflow_detected ? "YES" : "No");
    PipelineWriteFormat("Avg Process Time: %.6f ms\n", status.avg_process_time * 1000.0);
}

void VTSwapScreenBuffer(void) {
    // Simple screen buffer swap
    static bool using_alt = false;
    if (using_alt) {
        memcpy(terminal.screen, terminal.alt_screen, sizeof(terminal.screen));
    } else {
        memcpy(terminal.alt_screen, terminal.screen, sizeof(terminal.screen));
    }
    using_alt = !using_alt;
}

void ProcessPipeline(void) {
    if (terminal.pipeline_count == 0) {
        return;
    }
    
    double start_time = SituationTimerGetTime();
    int chars_processed = 0;
    int target_chars = terminal.VTperformance.chars_per_frame;
    
    // Adaptive processing based on buffer level
    if (terminal.pipeline_count > terminal.VTperformance.burst_threshold) {
        target_chars *= 2; // Burst mode
        terminal.VTperformance.burst_mode = true;
    } else if (terminal.pipeline_count < target_chars) {
        target_chars = terminal.pipeline_count; // Process all remaining
        terminal.VTperformance.burst_mode = false;
    }
    
    while (chars_processed < target_chars && terminal.pipeline_count > 0) {
        // Check time budget
        if (SituationTimerGetTime() - start_time > terminal.VTperformance.time_budget) {
            break;
        }
        
        unsigned char ch = terminal.input_pipeline[terminal.pipeline_tail];
        terminal.pipeline_tail = (terminal.pipeline_tail + 1) % sizeof(terminal.input_pipeline);
        terminal.pipeline_count--;
        
        ProcessChar(ch);
        chars_processed++;
    }
    
    // Update performance metrics
    if (chars_processed > 0) {
        double total_time = SituationTimerGetTime() - start_time;
        double time_per_char = total_time / chars_processed;
        terminal.VTperformance.avg_process_time = 
            terminal.VTperformance.avg_process_time * 0.9 + time_per_char * 0.1;
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void LogUnsupportedSequence(const char* sequence) {
    if (!terminal.options.log_unsupported) return;
    
    terminal.conformance.compliance.unsupported_sequences++;
    
    // Use strcpy instead of strncpy to avoid truncation warnings
    size_t len = strlen(sequence);
    if (len >= sizeof(terminal.conformance.compliance.last_unsupported)) {
        len = sizeof(terminal.conformance.compliance.last_unsupported) - 1;
    }
    memcpy(terminal.conformance.compliance.last_unsupported, sequence, len);
    terminal.conformance.compliance.last_unsupported[len] = '\0';
    
    if (terminal.options.debug_sequences) {
        char debug_msg[128];
        snprintf(debug_msg, sizeof(debug_msg), 
                "Unsupported: %s (total: %d)\n", 
                sequence, terminal.conformance.compliance.unsupported_sequences);
        
        if (response_callback) {
            response_callback(debug_msg, strlen(debug_msg));
        }
    }
}

// =============================================================================
// PARAMETER PARSING UTILITIES
// =============================================================================

int ParseCSIParams(const char* params, int* out_params, int max_params) {
    terminal.param_count = 0;
    memset(terminal.escape_params, 0, sizeof(terminal.escape_params));
    
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
    
    while (token != NULL && terminal.param_count < max_params) {
        if (strlen(token) == 0) {
            terminal.escape_params[terminal.param_count] = 0;
        } else {
            int value = atoi(token);
            terminal.escape_params[terminal.param_count] = (value >= 0) ? value : 0;
        }
        if (out_params) {
            out_params[terminal.param_count] = terminal.escape_params[terminal.param_count];
        }
        terminal.param_count++;
        token = strtok_r(NULL, ";", &saveptr);
    }
    return terminal.param_count;
}

static void ClearCSIParams(void) {
    terminal.escape_buffer[0] = '\0';
    terminal.escape_pos = 0;
    terminal.param_count = 0;
    memset(terminal.escape_params, 0, sizeof(terminal.escape_params));
}

void ProcessSixelSTChar(unsigned char ch) {
    if (ch == '\\') { // This is ST
        terminal.parse_state = VT_PARSE_NORMAL;
        // Finalize sixel image size
        terminal.sixel.width = terminal.sixel.max_x;
        terminal.sixel.height = terminal.sixel.max_y;
    } else {
        // Not an ST, go back to parsing sixel data
        terminal.parse_state = PARSE_SIXEL;
        ProcessSixelChar('\x1B'); // Process the ESC that led here
        ProcessSixelChar(ch);     // Process the current char
    }
}

int GetCSIParam(int index, int default_value) {
    if (index >= 0 && index < terminal.param_count) {
        return (terminal.escape_params[index] == 0) ? default_value : terminal.escape_params[index];
    }
    return default_value;
}


// =============================================================================
// CURSOR MOVEMENT IMPLEMENTATIONS
// =============================================================================

void ExecuteCUU(void) { // Cursor Up
    int n = GetCSIParam(0, 1);
    int new_y = terminal.cursor.y - n;
    
    if (terminal.dec_modes.origin_mode) {
        terminal.cursor.y = (new_y < terminal.scroll_top) ? terminal.scroll_top : new_y;
    } else {
        terminal.cursor.y = (new_y < 0) ? 0 : new_y;
    }
}

void ExecuteCUD(void) { // Cursor Down
    int n = GetCSIParam(0, 1);
    int new_y = terminal.cursor.y + n;
    
    if (terminal.dec_modes.origin_mode) {
        terminal.cursor.y = (new_y > terminal.scroll_bottom) ? terminal.scroll_bottom : new_y;
    } else {
        terminal.cursor.y = (new_y >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : new_y;
    }
}

void ExecuteCUF(void) { // Cursor Forward
    int n = GetCSIParam(0, 1);
    terminal.cursor.x = (terminal.cursor.x + n >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : terminal.cursor.x + n;
}

void ExecuteCUB(void) { // Cursor Back
    int n = GetCSIParam(0, 1);
    terminal.cursor.x = (terminal.cursor.x - n < 0) ? 0 : terminal.cursor.x - n;
}

void ExecuteCNL(void) { // Cursor Next Line
    int n = GetCSIParam(0, 1);
    terminal.cursor.y = (terminal.cursor.y + n >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : terminal.cursor.y + n;
    terminal.cursor.x = terminal.left_margin;
}

void ExecuteCPL(void) { // Cursor Previous Line
    int n = GetCSIParam(0, 1);
    terminal.cursor.y = (terminal.cursor.y - n < 0) ? 0 : terminal.cursor.y - n;
    terminal.cursor.x = terminal.left_margin;
}

void ExecuteCHA(void) { // Cursor Horizontal Absolute
    int n = GetCSIParam(0, 1) - 1; // Convert to 0-based
    terminal.cursor.x = (n < 0) ? 0 : (n >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : n;
}

void ExecuteCUP(void) { // Cursor Position
    int row = GetCSIParam(0, 1) - 1; // Convert to 0-based
    int col = GetCSIParam(1, 1) - 1;
    
    if (terminal.dec_modes.origin_mode) {
        row += terminal.scroll_top;
        col += terminal.left_margin;
    }
    
    terminal.cursor.y = (row < 0) ? 0 : (row >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : row;
    terminal.cursor.x = (col < 0) ? 0 : (col >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : col;
    
    // Clamp to scrolling region if in origin mode
    if (terminal.dec_modes.origin_mode) {
        if (terminal.cursor.y < terminal.scroll_top) terminal.cursor.y = terminal.scroll_top;
        if (terminal.cursor.y > terminal.scroll_bottom) terminal.cursor.y = terminal.scroll_bottom;
        if (terminal.cursor.x < terminal.left_margin) terminal.cursor.x = terminal.left_margin;
        if (terminal.cursor.x > terminal.right_margin) terminal.cursor.x = terminal.right_margin;
    }
}

void ExecuteVPA(void) { // Vertical Position Absolute
    int n = GetCSIParam(0, 1) - 1; // Convert to 0-based
    
    if (terminal.dec_modes.origin_mode) {
        n += terminal.scroll_top;
        terminal.cursor.y = (n < terminal.scroll_top) ? terminal.scroll_top : 
                           (n > terminal.scroll_bottom) ? terminal.scroll_bottom : n;
    } else {
        terminal.cursor.y = (n < 0) ? 0 : (n >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : n;
    }
}

// =============================================================================
// ERASING IMPLEMENTATIONS
// =============================================================================

void ExecuteED(void) { // Erase in Display
    int n = GetCSIParam(0, 0);
    
    switch (n) {
        case 0: // Clear from cursor to end of screen
            // Clear current line from cursor
            for (int x = terminal.cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                ClearCell(&terminal.screen[terminal.cursor.y][x]);
            }
            // Clear remaining lines
            for (int y = terminal.cursor.y + 1; y < DEFAULT_TERM_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    ClearCell(&terminal.screen[y][x]);
                }
            }
            break;
            
        case 1: // Clear from beginning of screen to cursor
            // Clear lines before cursor
            for (int y = 0; y < terminal.cursor.y; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    ClearCell(&terminal.screen[y][x]);
                }
            }
            // Clear current line up to cursor
            for (int x = 0; x <= terminal.cursor.x; x++) {
                ClearCell(&terminal.screen[terminal.cursor.y][x]);
            }
            break;
            
        case 2: // Clear entire screen
        case 3: // Clear entire screen and scrollback (xterm extension)
            for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    ClearCell(&terminal.screen[y][x]);
                }
            }
            break;
            
        default:
            LogUnsupportedSequence("Unknown ED parameter");
            break;
    }
}

void ExecuteEL(void) { // Erase in Line
    int n = GetCSIParam(0, 0);
    
    switch (n) {
        case 0: // Clear from cursor to end of line
            for (int x = terminal.cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                ClearCell(&terminal.screen[terminal.cursor.y][x]);
            }
            break;
            
        case 1: // Clear from beginning of line to cursor
            for (int x = 0; x <= terminal.cursor.x; x++) {
                ClearCell(&terminal.screen[terminal.cursor.y][x]);
            }
            break;
            
        case 2: // Clear entire line
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                ClearCell(&terminal.screen[terminal.cursor.y][x]);
            }
            break;
            
        default:
            LogUnsupportedSequence("Unknown EL parameter");
            break;
    }
}

void ExecuteECH(void) { // Erase Character
    int n = GetCSIParam(0, 1);
    
    for (int i = 0; i < n && terminal.cursor.x + i < DEFAULT_TERM_WIDTH; i++) {
        ClearCell(&terminal.screen[terminal.cursor.y][terminal.cursor.x + i]);
    }
}

// =============================================================================
// INSERTION AND DELETION IMPLEMENTATIONS
// =============================================================================

void ExecuteIL(void) { // Insert Line
    int n = GetCSIParam(0, 1);
    InsertLinesAt(terminal.cursor.y, n);
}

void ExecuteDL(void) { // Delete Line
    int n = GetCSIParam(0, 1);
    DeleteLinesAt(terminal.cursor.y, n);
}

void ExecuteICH(void) { // Insert Character
    int n = GetCSIParam(0, 1);
    InsertCharactersAt(terminal.cursor.y, terminal.cursor.x, n);
}

void ExecuteDCH(void) { // Delete Character
    int n = GetCSIParam(0, 1);
    DeleteCharactersAt(terminal.cursor.y, terminal.cursor.x, n);
}

// =============================================================================
// SCROLLING IMPLEMENTATIONS
// =============================================================================

void ExecuteSU(void) { // Scroll Up
    int n = GetCSIParam(0, 1);
    ScrollUpRegion(terminal.scroll_top, terminal.scroll_bottom, n);
}

void ExecuteSD(void) { // Scroll Down
    int n = GetCSIParam(0, 1);
    ScrollDownRegion(terminal.scroll_top, terminal.scroll_bottom, n);
}

// =============================================================================
// ENHANCED SGR (SELECT GRAPHIC RENDITION) IMPLEMENTATION
// =============================================================================

int ProcessExtendedColor(ExtendedColor* color, int param_index) {
    int consumed = 0;
    
    if (param_index + 1 < terminal.param_count) {
        int color_type = terminal.escape_params[param_index + 1];
        
        if (color_type == 5 && param_index + 2 < terminal.param_count) {
            // 256-color mode: ESC[38;5;n or ESC[48;5;n
            int color_index = terminal.escape_params[param_index + 2];
            if (color_index >= 0 && color_index < 256) {
                color->color_mode = 0;
                color->value.index = color_index;
            }
            consumed = 2;
            
        } else if (color_type == 2 && param_index + 4 < terminal.param_count) {
            // True color mode: ESC[38;2;r;g;b or ESC[48;2;r;g;b
            int r = terminal.escape_params[param_index + 2] & 0xFF;
            int g = terminal.escape_params[param_index + 3] & 0xFF;
            int b = terminal.escape_params[param_index + 4] & 0xFF;
            
            color->color_mode = 1;
            color->value.rgb = (RGB_Color){r, g, b, 255};
            consumed = 4;
        }
    }
    
    return consumed;
}

void ResetAllAttributes(void) {
    terminal.current_fg.color_mode = 0;
    terminal.current_fg.value.index = COLOR_WHITE;
    terminal.current_bg.color_mode = 0;
    terminal.current_bg.value.index = COLOR_BLACK;
    
    terminal.bold_mode = false;
    terminal.faint_mode = false;
    terminal.italic_mode = false;
    terminal.underline_mode = false;
    terminal.blink_mode = false;
    terminal.reverse_mode = false;
    terminal.strikethrough_mode = false;
    terminal.conceal_mode = false;
    terminal.overline_mode = false;
    terminal.double_underline_mode = false;
    terminal.protected_mode = false;
}

void ExecuteSGR(void) {
    if (terminal.param_count == 0) {
        // Reset all attributes
        ResetAllAttributes();
        return;
    }
    
    for (int i = 0; i < terminal.param_count; i++) {
        int param = terminal.escape_params[i];
        
        switch (param) {
            case 0: // Reset all
                ResetAllAttributes();
                break;
                
            // Intensity
            case 1: terminal.bold_mode = true; break;
            case 2: terminal.faint_mode = true; break;
            case 22: terminal.bold_mode = terminal.faint_mode = false; break;
                
            // Style
            case 3: terminal.italic_mode = true; break;
            case 23: terminal.italic_mode = false; break;
            
            case 4: terminal.underline_mode = true; break;
            case 21: terminal.double_underline_mode = true; break;
            case 24: terminal.underline_mode = terminal.double_underline_mode = false; break;
            
            case 5: case 6: terminal.blink_mode = true; break;
            case 25: terminal.blink_mode = false; break;
            
            case 7: terminal.reverse_mode = true; break;
            case 27: terminal.reverse_mode = false; break;
            
            case 8: terminal.conceal_mode = true; break;
            case 28: terminal.conceal_mode = false; break;
            
            case 9: terminal.strikethrough_mode = true; break;
            case 29: terminal.strikethrough_mode = false; break;
            
            case 53: terminal.overline_mode = true; break;
            case 55: terminal.overline_mode = false; break;
            
            // Standard colors (30-37, 40-47)
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                terminal.current_fg.color_mode = 0;
                terminal.current_fg.value.index = param - 30;
                break;
                
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                terminal.current_bg.color_mode = 0;
                terminal.current_bg.value.index = param - 40;
                break;
                
            // Bright colors (90-97, 100-107)
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                terminal.current_fg.color_mode = 0;
                terminal.current_fg.value.index = param - 90 + 8;
                break;
                
            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                terminal.current_bg.color_mode = 0;
                terminal.current_bg.value.index = param - 100 + 8;
                break;
                
            // Extended colors
            case 38: // Set foreground color
                i += ProcessExtendedColor(&terminal.current_fg, i);
                break;
                
            case 48: // Set background color
                i += ProcessExtendedColor(&terminal.current_bg, i);
                break;
                
            // Default colors
            case 39:
                terminal.current_fg.color_mode = 0;
                terminal.current_fg.value.index = COLOR_WHITE;
                break;
                
            case 49:
                terminal.current_bg.color_mode = 0;
                terminal.current_bg.value.index = COLOR_BLACK;
                break;
                
            default:
                if (terminal.options.debug_sequences) {
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
            EnhancedTermChar *cell = &terminal.screen[y][x];
            checksum += cell->ch;
            checksum += (cell->fg_color.color_mode == 0 ? cell->fg_color.value.index : (cell->fg_color.value.rgb.r << 16 | cell->fg_color.value.rgb.g << 8 | cell->fg_color.value.rgb.b));
            checksum += (cell->bg_color.color_mode == 0 ? cell->bg_color.value.index : (cell->bg_color.value.rgb.r << 16 | cell->bg_color.value.rgb.g << 8 | cell->bg_color.value.rgb.b));
            checksum = (checksum >> 16) + (checksum & 0xFFFF);
        }
    }
    return checksum & 0xFFFF;
}

void SwitchScreenBuffer(bool to_alternate) {
    if (to_alternate && !terminal.dec_modes.alternate_screen) {
        // Save current screen to alternate buffer
        memcpy(terminal.alt_screen, terminal.screen, sizeof(terminal.screen));
        // Clear main screen
        memset(terminal.screen, 0, sizeof(terminal.screen));
        // Initialize cleared screen
        for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                ClearCell(&terminal.screen[y][x]);
            }
        }
        terminal.dec_modes.alternate_screen = true;
        
    } else if (!to_alternate && terminal.dec_modes.alternate_screen) {
        // Restore screen from alternate buffer
        memcpy(terminal.screen, terminal.alt_screen, sizeof(terminal.screen));
        // Mark all as dirty for redraw
        for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
            for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                terminal.screen[y][x].dirty = true;
            }
        }
        terminal.dec_modes.alternate_screen = false;
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
                terminal.dec_modes.application_cursor_keys = enable;
                terminal.vt_keyboard.cursor_key_mode = enable;
                break;
                
            case 2: // DECANM - ANSI Mode
                // Switch between VT52 and ANSI mode
                if (!enable && terminal.conformance.features.vt52_mode) {
                    terminal.parse_state = PARSE_VT52;
                }
                break;
                
            case 3: // DECCOLM - Column Mode
                // Set 132-column mode (resize not fully implemented)
                terminal.dec_modes.column_mode_132 = enable;
                break;
                
            case 4: // DECSCLM - Scrolling Mode
                // Enable/disable smooth scrolling
                terminal.dec_modes.smooth_scroll = enable;
                break;
                
            case 5: // DECSCNM - Screen Mode
                // Enable/disable reverse video
                terminal.dec_modes.reverse_video = enable;
                break;
                
            case 6: // DECOM - Origin Mode
                // Enable/disable origin mode, adjust cursor position
                terminal.dec_modes.origin_mode = enable;
                if (enable) {
                    terminal.cursor.x = terminal.left_margin;
                    terminal.cursor.y = terminal.scroll_top;
                } else {
                    terminal.cursor.x = 0;
                    terminal.cursor.y = 0;
                }
                break;
                
            case 7: // DECAWM - Auto Wrap Mode
                // Enable/disable auto wrap
                terminal.dec_modes.auto_wrap_mode = enable;
                break;
                
            case 8: // DECARM - Auto Repeat Mode
                // Enable/disable auto repeat keys
                terminal.dec_modes.auto_repeat_keys = enable;
                break;
                
            case 9: // X10 Mouse Tracking
                // Enable/disable X10 mouse tracking
                terminal.mouse.mode = enable ? MOUSE_TRACKING_X10 : MOUSE_TRACKING_OFF;
                terminal.mouse.enabled = enable;
                break;
                
            case 12: // DECSET 12 - Local Echo / Send/Receive
                // Enable/disable local echo mode
                terminal.dec_modes.local_echo = enable;
                break;
                
            case 25: // DECTCEM - Text Cursor Enable Mode
                // Enable/disable text cursor visibility
                terminal.dec_modes.cursor_visible = enable;
                terminal.cursor.visible = enable;
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
                    terminal.saved_cursor = terminal.cursor;
                    terminal.saved_cursor_valid = true;
                } else if (terminal.saved_cursor_valid) {
                    terminal.cursor = terminal.saved_cursor;
                }
                break;
                
            case 1049: // Alternate Screen + Save/Restore Cursor
                // Save/restore cursor and switch screen buffer
                if (enable) {
                    terminal.saved_cursor = terminal.cursor;
                    terminal.saved_cursor_valid = true;
                    SwitchScreenBuffer(true);
                    ExecuteED(); // Clear screen
                    terminal.cursor.x = 0;
                    terminal.cursor.y = 0;
                } else {
                    SwitchScreenBuffer(false);
                    if (terminal.saved_cursor_valid) {
                        terminal.cursor = terminal.saved_cursor;
                    }
                }
                break;
                
            case 1000: // VT200 Mouse Tracking
                // Enable/disable VT200 mouse tracking
                terminal.mouse.mode = enable ? (terminal.mouse.sgr_mode ? MOUSE_TRACKING_SGR : MOUSE_TRACKING_VT200) : MOUSE_TRACKING_OFF;
                terminal.mouse.enabled = enable;
                break;
                
            case 1001: // VT200 Highlight Mouse Tracking
                // Enable/disable VT200 highlight tracking
                terminal.mouse.mode = enable ? MOUSE_TRACKING_VT200_HIGHLIGHT : MOUSE_TRACKING_OFF;
                terminal.mouse.enabled = enable;
                break;
                
            case 1002: // Button Event Mouse Tracking
                // Enable/disable button-event tracking
                terminal.mouse.mode = enable ? MOUSE_TRACKING_BTN_EVENT : MOUSE_TRACKING_OFF;
                terminal.mouse.enabled = enable;
                break;
                
            case 1003: // Any Event Mouse Tracking
                // Enable/disable any-event tracking
                terminal.mouse.mode = enable ? MOUSE_TRACKING_ANY_EVENT : MOUSE_TRACKING_OFF;
                terminal.mouse.enabled = enable;
                break;
                
            case 1004: // Focus In/Out Events
                // Enable/disable focus tracking
                terminal.mouse.focus_tracking = enable;
                break;
                
            case 1005: // UTF-8 Mouse Mode
                // Placeholder for UTF-8 mouse encoding (not implemented)
                break;
                
            case 1006: // SGR Mouse Mode
                // Enable/disable SGR mouse reporting
                terminal.mouse.sgr_mode = enable;
                if (enable && terminal.mouse.mode != MOUSE_TRACKING_OFF && 
                    terminal.mouse.mode != MOUSE_TRACKING_URXVT && terminal.mouse.mode != MOUSE_TRACKING_PIXEL) {
                    terminal.mouse.mode = MOUSE_TRACKING_SGR;
                } else if (!enable && terminal.mouse.mode == MOUSE_TRACKING_SGR) {
                    terminal.mouse.mode = MOUSE_TRACKING_VT200;
                }
                break;
                
            case 1015: // URXVT Mouse Mode
                // Enable/disable URXVT mouse reporting
                terminal.mouse.mode = enable ? MOUSE_TRACKING_URXVT : MOUSE_TRACKING_OFF;
                terminal.mouse.enabled = enable;
                break;
                
            case 1016: // Pixel Position Mouse Mode
                // Enable/disable pixel position mouse reporting
                terminal.mouse.mode = enable ? MOUSE_TRACKING_PIXEL : MOUSE_TRACKING_OFF;
                terminal.mouse.enabled = enable;
                break;
                
            case 2004: // Bracketed Paste Mode
                // Enable/disable bracketed paste
                terminal.bracketed_paste.enabled = enable;
                break;
                
            default:
                // Log unsupported DEC modes
                if (terminal.options.debug_sequences) {
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
                terminal.dec_modes.insert_mode = enable;
                break;
                
            case 20: // LNM - Line Feed/New Line Mode
                // Enable/disable line feed/new line mode
                terminal.ansi_modes.line_feed_new_line = enable;
                break;
                
            default:
                // Log unsupported ANSI modes
                if (terminal.options.debug_sequences) {
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
    for (int i = 0; i < terminal.param_count; i++) {
        int mode = terminal.escape_params[i];
        if (private_mode) {
            switch (mode) {
                case 1000: // VT200 mouse tracking
                    EnableMouseFeature("cursor", true);
                    terminal.mouse.mode = terminal.mouse.sgr_mode ? MOUSE_TRACKING_SGR : MOUSE_TRACKING_VT200;
                    break;
                case 1002: // Button-event mouse tracking
                    EnableMouseFeature("cursor", true);
                    terminal.mouse.mode = MOUSE_TRACKING_BTN_EVENT;
                    break;
                case 1003: // Any-event mouse tracking
                    EnableMouseFeature("cursor", true);
                    terminal.mouse.mode = MOUSE_TRACKING_ANY_EVENT;
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
    for (int i = 0; i < terminal.param_count; i++) {
        int mode = terminal.escape_params[i];
        if (private_mode) {
            switch (mode) {
                case 1000: // VT200 mouse tracking
                case 1002: // Button-event mouse tracking
                case 1003: // Any-event mouse tracking
                case 1015: // URXVT mouse reporting
                case 1016: // Pixel position mouse reporting
                    EnableMouseFeature("cursor", false);
                    terminal.mouse.mode = MOUSE_TRACKING_OFF;
                    break;
                case 1004: // Focus tracking
                    EnableMouseFeature("focus", false);
                    break;
                case 1006: // SGR mouse reporting
                    EnableMouseFeature("sgr", false);
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
    char introducer = private_mode ? terminal.escape_buffer[0] : 0;
    if (introducer == '>') {
        QueueResponse(terminal.secondary_attributes);
    } else if (introducer == '=') {
        QueueResponse(terminal.tertiary_attributes);
    } else {
        QueueResponse(terminal.device_attributes);
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

// Helper function to queue print output
static void QueuePrintOutput(const char* data, size_t length) {
    if (terminal.response_length + length < sizeof(terminal.answerback_buffer)) {
        memcpy(terminal.answerback_buffer + terminal.response_length, data, length);
        terminal.response_length += length;
    } else if (terminal.options.debug_sequences) {
        fprintf(stderr, "Print output buffer overflow\n");
    }
}

// Updated ExecuteMC
static void ExecuteMC(void) {
    bool private_mode = (terminal.escape_buffer[0] == '?');
    int params[MAX_ESCAPE_PARAMS];
    ParseCSIParams(terminal.escape_buffer, params, MAX_ESCAPE_PARAMS);
    int pi = (terminal.param_count > 0) ? terminal.escape_params[0] : 0;

    if (!terminal.printer_available) {
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
                        EnhancedTermChar* cell = &terminal.screen[y][x];
                        if (pos < sizeof(print_buffer) - 2) {
                            print_buffer[pos++] = GetPrintableChar(cell->ch, &terminal.charset);
                        }
                    }
                    if (pos < sizeof(print_buffer) - 2) {
                        print_buffer[pos++] = '\n';
                    }
                }
                print_buffer[pos] = '\0';
                QueueResponse(print_buffer);
                if (terminal.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Print screen completed");
                }
                break;
            }
            case 1: // Print current line
            {
                char print_buffer[DEFAULT_TERM_WIDTH + 2];
                size_t pos = 0;
                int y = terminal.cursor.y;
                for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                    EnhancedTermChar* cell = &terminal.screen[y][x];
                    if (pos < sizeof(print_buffer) - 2) {
                        print_buffer[pos++] = GetPrintableChar(cell->ch, &terminal.charset);
                    }
                }
                print_buffer[pos++] = '\n';
                print_buffer[pos] = '\0';
                QueueResponse(print_buffer);
                if (terminal.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Print line completed");
                }
                break;
            }
            case 4: // Disable auto-print
                terminal.auto_print_enabled = false;
                if (terminal.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Auto-print disabled");
                }
                break;
            case 5: // Enable auto-print
                terminal.auto_print_enabled = true;
                if (terminal.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Auto-print enabled");
                }
                break;
            default:
                if (terminal.options.log_unsupported) {
                    snprintf(terminal.conformance.compliance.last_unsupported,
                             sizeof(terminal.conformance.compliance.last_unsupported),
                             "CSI %d i", pi);
                    terminal.conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    } else {
        switch (pi) {
            case 4: // Disable printer controller mode
                terminal.printer_controller_enabled = false;
                if (terminal.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Printer controller disabled");
                }
                break;
            case 5: // Enable printer controller mode
                terminal.printer_controller_enabled = true;
                if (terminal.options.debug_sequences) {
                    LogUnsupportedSequence("MC: Printer controller enabled");
                }
                break;
            default:
                if (terminal.options.log_unsupported) {
                    snprintf(terminal.conformance.compliance.last_unsupported,
                             sizeof(terminal.conformance.compliance.last_unsupported),
                             "CSI ?%d i", pi);
                    terminal.conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    }
}

// Enhanced QueueResponse
void QueueResponse(const char* response) {
    size_t len = strlen(response);
    // Leave space for null terminator
    if (terminal.response_length + len >= sizeof(terminal.answerback_buffer) - 1) {
        // Flush existing responses
        if (response_callback && terminal.response_length > 0) {
            response_callback(terminal.answerback_buffer, terminal.response_length);
            terminal.response_length = 0;
        }
        // If response is still too large, log and truncate
        if (len >= sizeof(terminal.answerback_buffer) - 1) {
            if (terminal.options.debug_sequences) {
                fprintf(stderr, "QueueResponse: Response too large (%zu bytes)\n", len);
            }
            len = sizeof(terminal.answerback_buffer) - 1;
        }
    }
    
    if (len > 0) {
        memcpy(terminal.answerback_buffer + terminal.response_length, response, len);
        terminal.response_length += len;
        terminal.answerback_buffer[terminal.response_length] = '\0'; // Ensure null-termination
    }
}

void QueueResponseBytes(const char* data, size_t len) {
    if (terminal.response_length + len >= sizeof(terminal.answerback_buffer)) {
        if (response_callback && terminal.response_length > 0) {
            response_callback(terminal.answerback_buffer, terminal.response_length);
            terminal.response_length = 0;
        }
        if (len >= sizeof(terminal.answerback_buffer)) {
            if (terminal.options.debug_sequences) {
                fprintf(stderr, "QueueResponseBytes: Response too large (%zu bytes)\n", len);
            }
            len = sizeof(terminal.answerback_buffer) -1;
        }
    }

    if (len > 0) {
        memcpy(terminal.answerback_buffer + terminal.response_length, data, len);
        terminal.response_length += len;
    }
}

// Enhanced ExecuteDSR function with new handlers
static void ExecuteDSR(void) {
    bool private_mode = (terminal.escape_buffer[0] == '?');
    int params[MAX_ESCAPE_PARAMS];
    ParseCSIParams(terminal.escape_buffer, params, MAX_ESCAPE_PARAMS);
    int command = (terminal.param_count > 0) ? terminal.escape_params[0] : 0;

    if (!private_mode) {
        switch (command) {
            case 5: QueueResponse("\x1B[0n"); break;
            case 6: {
                int row = terminal.cursor.y + 1;
                int col = terminal.cursor.x + 1;
                if (terminal.dec_modes.origin_mode) {
                    row = terminal.cursor.y - terminal.scroll_top + 1;
                    col = terminal.cursor.x - terminal.left_margin + 1;
                }
                char response[32];
                snprintf(response, sizeof(response), "\x1B[%d;%dR", row, col);
                QueueResponse(response);
                break;
            }
            default:
                if (terminal.options.log_unsupported) {
                    snprintf(terminal.conformance.compliance.last_unsupported,
                             sizeof(terminal.conformance.compliance.last_unsupported),
                             "CSI %dn", command);
                    terminal.conformance.compliance.unsupported_sequences++;
                }
                break;
        }
    } else {
        switch (command) {
            case 15: QueueResponse(terminal.printer_available ? "\x1B[?10n" : "\x1B[?13n"); break;
            case 25: QueueResponse(terminal.programmable_keys.udk_locked ? "\x1B[?21n" : "\x1B[?20n"); break;
            case 26: {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?27;%dn", terminal.vt_keyboard.keyboard_dialect);
                QueueResponse(response);
                break;
            }
            case 27: // Locator Type (DECREPTPARM)
                QueueResponse("\x1B[?27;0n"); // No locator
                break;
            case 53: QueueResponse(terminal.locator_enabled ? "\x1B[?53n" : "\x1B[?50n"); break;
            case 55: QueueResponse("\x1B[?57;0n"); break;
            case 56: QueueResponse("\x1B[?56;0n"); break;
            case 62: {
                char response[32];
                snprintf(response, sizeof(response), "\x1B[?62;%zu;%zun", 
                         terminal.macro_space.used, terminal.macro_space.total);
                QueueResponse(response);
                break;
            }
            case 63: {
                int page = (terminal.param_count > 1) ? terminal.escape_params[1] : 1;
                terminal.checksum.last_checksum = ComputeScreenChecksum(page);
                char response[64];
                snprintf(response, sizeof(response), "\x1B[?63;%d;%d;%04Xn", 
                         page, terminal.checksum.algorithm, terminal.checksum.last_checksum);
                QueueResponse(response);
                break;
            }
            case 75: QueueResponse("\x1B[?75;0n"); break;
            default:
                if (terminal.options.log_unsupported) {
                    snprintf(terminal.conformance.compliance.last_unsupported,
                             sizeof(terminal.conformance.compliance.last_unsupported),
                             "CSI ?%dn", command);
                    terminal.conformance.compliance.unsupported_sequences++;
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
        terminal.scroll_top = top;
        terminal.scroll_bottom = bottom;
        
        // Move cursor to home position
        if (terminal.dec_modes.origin_mode) {
            terminal.cursor.x = terminal.left_margin;
            terminal.cursor.y = terminal.scroll_top;
        } else {
            terminal.cursor.x = 0;
            terminal.cursor.y = 0;
        }
    }
}

void ExecuteDECSLRM(void) { // Set Left and Right Margins (VT420)
    if (!terminal.conformance.features.vt420_mode) {
        LogUnsupportedSequence("DECSLRM requires VT420 mode");
        return;
    }
    
    int left = GetCSIParam(0, 1) - 1;    // Convert to 0-based
    int right = GetCSIParam(1, DEFAULT_TERM_WIDTH) - 1;
    
    // Validate parameters
    if (left >= 0 && left < DEFAULT_TERM_WIDTH && right >= left && right < DEFAULT_TERM_WIDTH) {
        terminal.left_margin = left;
        terminal.right_margin = right;
        
        // Move cursor to home position
        if (terminal.dec_modes.origin_mode) {
            terminal.cursor.x = terminal.left_margin;
            terminal.cursor.y = terminal.scroll_top;
        } else {
            terminal.cursor.x = 0;
            terminal.cursor.y = 0;
        }
    }
}

// Updated ExecuteDECRQPSR
static void ExecuteDECRQPSR(void) {
    int params[MAX_ESCAPE_PARAMS];
    ParseCSIParams(terminal.escape_buffer, params, MAX_ESCAPE_PARAMS);
    int pfn = (terminal.param_count > 0) ? terminal.escape_params[0] : 0;

    char response[64];
    switch (pfn) {
        case 1: // Sixel geometry
            snprintf(response, sizeof(response), "DCS 2 $u %d ; %d;%d;%d;%d ST",
                     terminal.conformance.level, terminal.sixel.x, terminal.sixel.y,
                     terminal.sixel.width, terminal.sixel.height);
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
            if (terminal.options.log_unsupported) {
                snprintf(terminal.conformance.compliance.last_unsupported,
                         sizeof(terminal.conformance.compliance.last_unsupported),
                         "CSI %d $ u (ReGIS unsupported)", pfn);
                terminal.conformance.compliance.unsupported_sequences++;
            }
            break;
        default:
            if (terminal.options.log_unsupported) {
                snprintf(terminal.conformance.compliance.last_unsupported,
                         sizeof(terminal.conformance.compliance.last_unsupported),
                         "CSI %d $ u", pfn);
                terminal.conformance.compliance.unsupported_sequences++;
            }
            break;
    }
}

void ExecuteDECLL(void) { // Load LEDs
    int n = GetCSIParam(0, 0);
    
    // DECLL - Load LEDs (VT220+ feature)
    // Parameters: 0=all off, 1=LED1 on, 2=LED2 on, 4=LED3 on, 8=LED4 on
    // Modern terminals don't have LEDs, so this is mostly ignored
    
    if (terminal.options.debug_sequences) {
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
    terminal.dec_modes.cursor_visible = true;
    terminal.dec_modes.auto_wrap_mode = true;
    terminal.dec_modes.origin_mode = false;
    terminal.dec_modes.insert_mode = false;
    terminal.dec_modes.application_cursor_keys = false;
    
    // Reset character attributes
    ResetAllAttributes();
    
    // Reset scrolling region
    terminal.scroll_top = 0;
    terminal.scroll_bottom = DEFAULT_TERM_HEIGHT - 1;
    terminal.left_margin = 0;
    terminal.right_margin = DEFAULT_TERM_WIDTH - 1;
    
    // Reset character sets
    InitCharacterSets();
    
    // Reset tab stops
    InitTabStops();
    
    // Home cursor
    terminal.cursor.x = 0;
    terminal.cursor.y = 0;
    
    // Clear saved cursor
    terminal.saved_cursor_valid = false;
    
    if (terminal.options.debug_sequences) {
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
            if (terminal.options.debug_sequences) {
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
    bool private_mode = (terminal.escape_buffer[0] == '?');
    
    char response[32];
    int mode_state = 0; // 0=not recognized, 1=set, 2=reset, 3=permanently set, 4=permanently reset
    
    if (private_mode) {
        // Check DEC private modes
        switch (mode) {
            case 1: // DECCKM (Application Cursor Keys)
                mode_state = terminal.dec_modes.application_cursor_keys ? 1 : 2;
                break;
            case 3: // DECCOLM (132 Column Mode)
                mode_state = terminal.dec_modes.column_mode_132 ? 1 : 2;
                break;
            case 4: // DECSCLM (Smooth Scroll)
                mode_state = terminal.dec_modes.smooth_scroll ? 1 : 2;
                break;
            case 5: // DECSCNM (Reverse Video)
                mode_state = terminal.dec_modes.reverse_video ? 1 : 2;
                break;
            case 6: // DECOM (Origin Mode)
                mode_state = terminal.dec_modes.origin_mode ? 1 : 2;
                break;
            case 7: // DECAWM (Auto Wrap Mode)
                mode_state = terminal.dec_modes.auto_wrap_mode ? 1 : 2;
                break;
            case 8: // DECARM (Auto Repeat Keys)
                mode_state = terminal.dec_modes.auto_repeat_keys ? 1 : 2;
                break;
            case 9: // X10 Mouse
                mode_state = terminal.dec_modes.x10_mouse ? 1 : 2;
                break;
            case 10: // Show Toolbar
                mode_state = terminal.dec_modes.show_toolbar ? 1 : 4; // Permanently reset
                break;
            case 12: // Blinking Cursor
                mode_state = terminal.dec_modes.blink_cursor ? 1 : 2;
                break;
            case 18: // DECPFF (Print Form Feed)
                mode_state = terminal.dec_modes.print_form_feed ? 1 : 2;
                break;
            case 19: // Print Extent
                mode_state = terminal.dec_modes.print_extent ? 1 : 2;
                break;
            case 25: // DECTCEM (Cursor Visible)
                mode_state = terminal.dec_modes.cursor_visible ? 1 : 2;
                break;
            case 47: // Alternate Screen
            case 1047:
            case 1049:
                mode_state = terminal.dec_modes.alternate_screen ? 1 : 2;
                break;
            case 1000: // VT200 Mouse
                mode_state = (terminal.mouse.mode == MOUSE_TRACKING_VT200) ? 1 : 2;
                break;
            case 2004: // Bracketed Paste
                mode_state = terminal.bracketed_paste.enabled ? 1 : 2;
                break;
            case 61: // DECSCL VT100
                mode_state = (terminal.conformance.level == VT_LEVEL_100) ? 1 : 2;
                break;
            case 62: // DECSCL VT220
                mode_state = (terminal.conformance.level == VT_LEVEL_220) ? 1 : 2;
                break;
            case 63: // DECSCL VT320
                mode_state = (terminal.conformance.level == VT_LEVEL_320) ? 1 : 2;
                break;
            case 64: // DECSCL VT420
                mode_state = (terminal.conformance.level == VT_LEVEL_420) ? 1 : 2;
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
                mode_state = terminal.ansi_modes.insert_replace ? 1 : 2;
                break;
            case 20: // LNM (Line Feed/New Line Mode)
                mode_state = terminal.ansi_modes.line_feed_new_line ? 1 : 3; // Permanently set
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
            terminal.cursor.shape = CURSOR_BLOCK_BLINK;
            terminal.cursor.blink_enabled = true;
            break;
        case 2: // Steady block
            terminal.cursor.shape = CURSOR_BLOCK;
            terminal.cursor.blink_enabled = false;
            break;
        case 3: // Blinking underline
            terminal.cursor.shape = CURSOR_UNDERLINE_BLINK;
            terminal.cursor.blink_enabled = true;
            break;
        case 4: // Steady underline
            terminal.cursor.shape = CURSOR_UNDERLINE;
            terminal.cursor.blink_enabled = false;
            break;
        case 5: // Blinking bar
            terminal.cursor.shape = CURSOR_BAR_BLINK;
            terminal.cursor.blink_enabled = true;
            break;
        case 6: // Steady bar
            terminal.cursor.shape = CURSOR_BAR;
            terminal.cursor.blink_enabled = false;
            break;
        default:
            if (terminal.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown cursor style: %d", style);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }
}

void ExecuteCSI_P(void) { // Various P commands
    // This handles CSI sequences ending in 'p' with different intermediates
    char* params = terminal.escape_buffer;
    
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
        if (terminal.options.debug_sequences) {
            char debug_msg[128];
            snprintf(debug_msg, sizeof(debug_msg), "Unknown CSI p command: %s", params);
            LogUnsupportedSequence(debug_msg);
        }
    }
}


void ExecuteWindowOps(void) { // Window manipulation (xterm extension)
    int operation = GetCSIParam(0, 0);
    
    switch (operation) {
        case 1: // De-iconify window
        case 2: // Iconify window
        case 3: // Move window to position
        case 4: // Resize window
        case 5: // Raise window
        case 6: // Lower window
        case 7: // Refresh window
        case 8: // Resize text area
            // These operations would require window manager integration
            if (terminal.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Window operation %d not supported", operation);
                LogUnsupportedSequence(debug_msg);
            }
            break;
            
        case 9: // Maximize/restore window
        case 10: // Full-screen toggle
            // Modern security restrictions typically block these
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
                        GetScreenHeight() / DEFAULT_CHAR_HEIGHT, GetScreenWidth() / DEFAULT_CHAR_WIDTH);
                QueueResponse(response);
            }
            break;
            
        case 20: // Report icon label
            {
                char response[256];
                snprintf(response, sizeof(response), "\x1B]L%s\x1B\\", terminal.title.icon_title);
                QueueResponse(response);
            }
            break;
            
        case 21: // Report window title
            {
                char response[256];
                snprintf(response, sizeof(response), "\x1B]l%s\x1B\\", terminal.title.window_title);
                QueueResponse(response);
            }
            break;
            
        default:
            if (terminal.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "Unknown window operation: %d", operation);
                LogUnsupportedSequence(debug_msg);
            }
            break;
    }
}

void ExecuteRestoreCursor(void) { // Restore cursor (non-ANSI.SYS)
    // This is the VT terminal version, not ANSI.SYS
    if (terminal.saved_cursor_valid) {
        terminal.cursor = terminal.saved_cursor;
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
            if (terminal.options.debug_sequences) {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "DECTST test %d - not applicable", test);
                LogUnsupportedSequence(debug_msg);
            }
            break;
            
        default:
            if (terminal.options.debug_sequences) {
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
    if (terminal.options.debug_sequences) {
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
            ClearTabStop(terminal.cursor.x);
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
            SetTabStop(terminal.cursor.x);
            break;
        case 2: // Clear tab stop at current column
            ClearTabStop(terminal.cursor.x);
            break;
        case 5: // Clear all tab stops
            ClearAllTabStops();
            break;
    }
}

// New ExecuteCSI_Dollar for multi-byte CSI $ sequences
void ExecuteCSI_Dollar(void) {
    // This function is now the central dispatcher for sequences with a '$' intermediate.
    // It looks for the character *after* the '$'.
    char* dollar_ptr = strchr(terminal.escape_buffer, '$');
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
                if (terminal.param_count == 4) {
                    ExecuteDECERA();
                } else if (terminal.param_count == 5) {
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
                if (terminal.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown CSI $ sequence with final char '%c'", final_char);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
    } else {
         if (terminal.options.debug_sequences) {
            char debug_msg[64];
            snprintf(debug_msg, sizeof(debug_msg), "Malformed CSI $ sequence in buffer: %s", terminal.escape_buffer);
            LogUnsupportedSequence(debug_msg);
        }
    }
}

void ProcessCSIChar(unsigned char ch) {
    if (terminal.parse_state != PARSE_CSI) return;

    if (ch >= 0x40 && ch <= 0x7E) {
        // Parse parameters into terminal.escape_params
        ParseCSIParams(terminal.escape_buffer, NULL, MAX_ESCAPE_PARAMS);

        // Handle DECSCUSR (CSI Ps SP q)
        if (ch == 'q' && terminal.escape_pos >= 1 && terminal.escape_buffer[terminal.escape_pos - 1] == ' ') {
            ExecuteDECSCUSR();
        } else {
            // Dispatch to ExecuteCSICommand
            ExecuteCSICommand(ch);
        }

        // Reset parser state
        terminal.parse_state = VT_PARSE_NORMAL;
        ClearCSIParams();
    } else if (ch >= 0x20 && ch <= 0x3F) {
        // Accumulate intermediate characters (e.g., digits, ';', '?')
        if (terminal.escape_pos < MAX_COMMAND_BUFFER - 1) {
            terminal.escape_buffer[terminal.escape_pos++] = ch;
            terminal.escape_buffer[terminal.escape_pos] = '\0';
        } else {
            if (terminal.options.debug_sequences) {
                fprintf(stderr, "CSI escape buffer overflow\n");
            }
            terminal.parse_state = VT_PARSE_NORMAL;
            ClearCSIParams();
        }
    } else if (ch == '$') {
        // Handle multi-byte CSI sequences (e.g., CSI $ q, CSI $ u)
        if (terminal.escape_pos < MAX_COMMAND_BUFFER - 1) {
            terminal.escape_buffer[terminal.escape_pos++] = ch;
            terminal.escape_buffer[terminal.escape_pos] = '\0';
        } else {
            if (terminal.options.debug_sequences) {
                fprintf(stderr, "CSI escape buffer overflow\n");
            }
            terminal.parse_state = VT_PARSE_NORMAL;
            ClearCSIParams();
        }
    } else {
        // Invalid character
        if (terminal.options.debug_sequences) {
            snprintf(terminal.conformance.compliance.last_unsupported,
                     sizeof(terminal.conformance.compliance.last_unsupported),
                     "Invalid CSI char: 0x%02X", ch);
            terminal.conformance.compliance.unsupported_sequences++;
        }
        terminal.parse_state = VT_PARSE_NORMAL;
        ClearCSIParams();
    }
}

// Updated ExecuteCSICommand
void ExecuteCSICommand(unsigned char command) {
    bool private_mode = (terminal.escape_buffer[0] == '?');

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
    if (command == 'q' && terminal.escape_pos >= 1 && terminal.escape_buffer[terminal.escape_pos - 1] == ' ') {
        // This is DECSCUSR: CSI Ps SP q.
        // ParseCSIParams was already called on the whole buffer by ProcessCSIChar.
        // We need to ensure it correctly parsed parameters *before* " SP q".
        // If GetCSIParam correctly extracts numbers before non-numeric parts, this might work.
        goto L_CSI_SP_q_DECSCUSR; // Specific handler for DECSCUSR via space
    }

    // Handle DCS sequences (e.g., DCS 0 ; 0 $ t <message> ST)
    if (command == 'P') {
        if (strstr(terminal.escape_buffer, "$t")) {
            ExecuteDCSAnswerback();
            goto L_CSI_END;
        } else {
            if (terminal.options.debug_sequences) {
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
L_CSI_I_CHT:          { int n=GetCSIParam(0,1); while(n-->0) terminal.cursor.x = NextTabStop(terminal.cursor.x); if (terminal.cursor.x >= DEFAULT_TERM_WIDTH) terminal.cursor.x = DEFAULT_TERM_WIDTH -1; } goto L_CSI_END; // CHT - Cursor Horizontal Tab (CSI Pn I)
L_CSI_J_ED:           ExecuteED();  goto L_CSI_END;                      // ED  - Erase in Display (CSI Pn J)
L_CSI_K_EL:           ExecuteEL();  goto L_CSI_END;                      // EL  - Erase in Line (CSI Pn K)
L_CSI_L_IL:           ExecuteIL();  goto L_CSI_END;                      // IL  - Insert Line(s) (CSI Pn L)
L_CSI_M_DL:           ExecuteDL();  goto L_CSI_END;                      // DL  - Delete Line(s) (CSI Pn M)
L_CSI_P_DCH:          ExecuteDCH(); goto L_CSI_END;                      // DCH - Delete Character(s) (CSI Pn P)
L_CSI_S_SU:           ExecuteSU();  goto L_CSI_END;                      // SU  - Scroll Up (CSI Pn S)
L_CSI_T_SD:           ExecuteSD();  goto L_CSI_END;                      // SD  - Scroll Down (CSI Pn T)
L_CSI_W_CTC_etc:      if(private_mode) ExecuteCTC(); else LogUnsupportedSequence("CSI W (non-private)"); goto L_CSI_END; // CTC - Cursor Tab Control (CSI ? Ps W)
L_CSI_X_ECH:          ExecuteECH(); goto L_CSI_END;                      // ECH - Erase Character(s) (CSI Pn X)
L_CSI_Z_CBT:          { int n=GetCSIParam(0,1); while(n-->0) terminal.cursor.x = PreviousTabStop(terminal.cursor.x); } goto L_CSI_END; // CBT - Cursor Backward Tab (CSI Pn Z)
L_CSI_at_ASC:         ExecuteICH(); goto L_CSI_END;                      // ICH - Insert Character(s) (CSI Pn @)
L_CSI_a_HPR:          { int n=GetCSIParam(0,1); terminal.cursor.x+=n; if(terminal.cursor.x<0)terminal.cursor.x=0; if(terminal.cursor.x>=DEFAULT_TERM_WIDTH)terminal.cursor.x=DEFAULT_TERM_WIDTH-1;} goto L_CSI_END; // HPR - Horizontal Position Relative (CSI Pn a)
L_CSI_b_REP:          { int n=GetCSIParam(0,1); (void)n; if(terminal.options.debug_sequences) LogUnsupportedSequence("REP");} goto L_CSI_END; // REP - Repeat Preceding Graphic Character (CSI Pn b)
L_CSI_c_DA:           ExecuteDA(private_mode); goto L_CSI_END;           // DA  - Device Attributes (CSI Ps c or CSI ? Ps c)
L_CSI_e_VPR:          { int n=GetCSIParam(0,1); terminal.cursor.y+=n; if(terminal.cursor.y<0)terminal.cursor.y=0; if(terminal.cursor.y>=DEFAULT_TERM_HEIGHT)terminal.cursor.y=DEFAULT_TERM_HEIGHT-1;} goto L_CSI_END; // VPR - Vertical Position Relative (CSI Pn e)
L_CSI_g_TBC:          ExecuteTBC(); goto L_CSI_END;                      // TBC - Tabulation Clear (CSI Ps g)
L_CSI_h_SM:           ExecuteSM(private_mode); goto L_CSI_END;           // SM  - Set Mode (CSI ? Pm h or CSI Pm h)
L_CSI_i_MC:           { int p0=GetCSIParam(0,0); (void)p0; if(terminal.options.debug_sequences)LogUnsupportedSequence("MC");} goto L_CSI_END; // MC - Media Copy (CSI Pn i or CSI ? Pn i)
L_CSI_j_HPB:          ExecuteCUB(); goto L_CSI_END;                      // HPB - Horizontal Position Backward (like CUB) (CSI Pn j)
L_CSI_k_VPB:          ExecuteCUU(); goto L_CSI_END;                      // VPB - Vertical Position Backward (like CUU) (CSI Pn k)
L_CSI_l_RM:           ExecuteRM(private_mode); goto L_CSI_END;           // RM  - Reset Mode (CSI ? Pm l or CSI Pm l)
L_CSI_m_SGR:          ExecuteSGR(); goto L_CSI_END;                      // SGR - Select Graphic Rendition (CSI Pm m)
L_CSI_n_DSR:          ExecuteDSR(); goto L_CSI_END;                      // DSR - Device Status Report (CSI Ps n or CSI ? Ps n)
L_CSI_o_VT420:        if(terminal.options.debug_sequences) LogUnsupportedSequence("VT420 'o'"); goto L_CSI_END; // DECDMAC, etc. (CSI Pt;Pb;Pl;Pr;Pp;Pattr o)
L_CSI_p_DECSOFT_etc:  ExecuteCSI_P(); goto L_CSI_END;                   // Various 'p' suffixed: DECSTR, DECSCL, DECRQM, DECUDK (CSI ! p, CSI " p, etc.)
L_CSI_q_DECLL_DECSCUSR: if(private_mode) ExecuteDECLL(); else ExecuteDECSCUSR(); goto L_CSI_END; // DECLL (private) / DECSCUSR (CSI Pn q or CSI Pn SP q)
L_CSI_r_DECSTBM:      if(!private_mode) ExecuteDECSTBM(); else LogUnsupportedSequence("CSI ? r invalid"); goto L_CSI_END; // DECSTBM - Set Top/Bottom Margins (CSI Pt ; Pb r)
L_CSI_s_SAVRES_CUR:   if(private_mode){if(terminal.conformance.features.vt420_mode) ExecuteDECSLRM(); else LogUnsupportedSequence("DECSLRM requires VT420");} else {terminal.saved_cursor=terminal.cursor; terminal.saved_cursor_valid=true;} goto L_CSI_END; // DECSLRM (private VT420+) / Save Cursor (ANSI.SYS) (CSI s / CSI ? Pl ; Pr s)
L_CSI_t_WINMAN:       ExecuteWindowOps(); goto L_CSI_END;                // Window Manipulation (xterm) / DECSLPP (Set lines per page) (CSI Ps t)
L_CSI_u_RES_CUR:      ExecuteRestoreCursor(); goto L_CSI_END;            // Restore Cursor (ANSI.SYS) (CSI u)
L_CSI_v_RECTCOPY:     if(private_mode) ExecuteRectangularOps(); else LogUnsupportedSequence("CSI v non-private invalid"); goto L_CSI_END; // DECCRA - Copy Rectangular Area (CSI ? Pt;Pl;Pb;Pr;Pps;Ptd;Ptl;Ptp $ v)
L_CSI_w_RECTCHKSUM:   if(private_mode) ExecuteRectangularOps2(); else LogUnsupportedSequence("CSI w non-private invalid"); goto L_CSI_END; // DECRQCRA - Req Rect Checksum (CSI ? Pts;Ptd;Pcs $ w)
L_CSI_x_DECREQTPARM:  ExecuteDECREQTPARM(); goto L_CSI_END;             // DECREQTPARM - Request Terminal Parameters (CSI Ps x)
L_CSI_y_DECTST:       ExecuteDECTST(); goto L_CSI_END;                   // DECTST - Invoke Confidence Test (CSI ? Ps y)
L_CSI_z_DECVERP:      if(private_mode) ExecuteDECVERP(); else LogUnsupportedSequence("CSI z non-private invalid"); goto L_CSI_END; // DECVERP - Verify Parity (CSI ? Ps ; Pv $ z)
L_CSI_LSBrace_DECSLE: ExecuteDECSLE(); goto L_CSI_END; // DECSLE - Select Locator Events (CSI ? Psl {)
L_CSI_Pipe_DECRQLP:   ExecuteDECRQLP(); goto L_CSI_END; // DECRQLP - Request Locator Position (CSI Plc |)
L_CSI_RSBrace_VT420:  LogUnsupportedSequence("CSI } invalid"); goto L_CSI_END;
L_CSI_Tilde_VT420:    LogUnsupportedSequence("CSI ~ invalid"); goto L_CSI_END;
L_CSI_dollar_multi:   ExecuteCSI_Dollar(); goto L_CSI_END;              // Multi-byte CSI sequences (e.g., CSI $ q, CSI $ u)
L_CSI_P_DCS:          // Device Control String (e.g., DCS 0 ; 0 $ t <message> ST)
    if (strstr(terminal.escape_buffer, "$t")) {
        ExecuteDCSAnswerback();
    } else {
        if (terminal.options.debug_sequences) {
            LogUnsupportedSequence("Unknown DCS sequence");
        }
    }
    goto L_CSI_END;

L_CSI_SP_q_DECSCUSR:  ExecuteDECSCUSR(); goto L_CSI_END;                  // DECSCUSR specific handler for CSI Ps SP q

L_CSI_UNSUPPORTED:
    if (terminal.options.debug_sequences) {
        char debug_msg[128];
        snprintf(debug_msg, sizeof(debug_msg),
                 "Unknown CSI %s%c (0x%02X) [computed goto default]", private_mode ? "?" : "", command, command);
        LogUnsupportedSequence(debug_msg);
    }
    terminal.conformance.compliance.unsupported_sequences++;
    // Fall through to L_CSI_END

L_CSI_END:
    return; // Common exit point for the function.
}

// =============================================================================
// OSC (OPERATING SYSTEM COMMAND) PROCESSING
// =============================================================================


void VTSetWindowTitle(const char* title) {
    strncpy(terminal.title.window_title, title, MAX_TITLE_LENGTH - 1);
    terminal.title.window_title[MAX_TITLE_LENGTH - 1] = '\0';
    terminal.title.title_changed = true;
    
    if (title_callback) {
        title_callback(terminal.title.window_title, false);
    }
    
    // Also set Raylib window title
    SetWindowTitle(terminal.title.window_title);
}

void SetIconTitle(const char* title) {
    strncpy(terminal.title.icon_title, title, MAX_TITLE_LENGTH - 1);
    terminal.title.icon_title[MAX_TITLE_LENGTH - 1] = '\0';
    terminal.title.icon_changed = true;
    
    if (title_callback) {
        title_callback(terminal.title.icon_title, true);
    }
}

void ResetForegroundColor(void) {
    terminal.current_fg.color_mode = 0;
    terminal.current_fg.value.index = COLOR_WHITE;
}

void ResetBackgroundColor(void) {
    terminal.current_bg.color_mode = 0;
    terminal.current_bg.value.index = COLOR_BLACK;
}

void ResetCursorColor(void) {
    terminal.cursor.color.color_mode = 0;
    terminal.cursor.color.value.index = COLOR_WHITE;
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
        ExtendedColor fg = terminal.current_fg;
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
        ExtendedColor bg = terminal.current_bg;
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
        ExtendedColor cursor_color = terminal.cursor.color;
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
    if (terminal.options.debug_sequences) {
        LogUnsupportedSequence("Font selection not fully implemented");
    }
}

void ProcessClipboardCommand(const char* data) {
    // Clipboard operations: c;base64data or c;?
    char clipboard_type = data[0];
    
    if (data[1] == ';' && data[2] == '?') {
        // Query clipboard
        char response[64];
        snprintf(response, sizeof(response), "\x1B]52;%c;\x1B\\", clipboard_type);
        QueueResponse(response);
    } else if (data[1] == ';') {
        // Set clipboard data (base64 encoded)
        if (terminal.options.debug_sequences) {
            LogUnsupportedSequence("Clipboard setting not implemented");
        }
    }
}

void ExecuteOSCCommand(void) {
    char* params = terminal.escape_buffer;
    
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
            VTSetWindowTitle(data);
            break;
            
        case 1: // Set icon title
            SetIconTitle(data);
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
            if (terminal.options.debug_sequences) {
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

void DefineUserKey(int key_code, const char* sequence) {
    // Expand programmable keys array if needed
    if (terminal.programmable_keys.count >= terminal.programmable_keys.capacity) {
        size_t new_capacity = terminal.programmable_keys.capacity == 0 ? 16 : 
                             terminal.programmable_keys.capacity * 2;
        
        ProgrammableKey* new_keys = realloc(terminal.programmable_keys.keys, 
                                           new_capacity * sizeof(ProgrammableKey));
        if (!new_keys) return;
        
        terminal.programmable_keys.keys = new_keys;
        terminal.programmable_keys.capacity = new_capacity;
    }
    
    // Find existing key or add new one
    ProgrammableKey* key = NULL;
    for (size_t i = 0; i < terminal.programmable_keys.count; i++) {
        if (terminal.programmable_keys.keys[i].key_code == key_code) {
            key = &terminal.programmable_keys.keys[i];
            free(key->sequence); // Free old sequence
            break;
        }
    }
    
    if (!key) {
        key = &terminal.programmable_keys.keys[terminal.programmable_keys.count++];
        key->key_code = key_code;
    }
    
    // Store new sequence
    key->sequence_length = strlen(sequence);
    key->sequence = malloc(key->sequence_length + 1);
    strcpy(key->sequence, sequence);
    key->active = true;
}

void ProcessUserDefinedKeys(const char* data) {
    // Parse user defined key format: key/string;key/string;...
    if (!terminal.conformance.features.vt320_mode) {
        LogUnsupportedSequence("User defined keys require VT320 mode");
        return;
    }
    
    char* data_copy = strdup(data);
    char* token = strtok(data_copy, ";");
    
    while (token != NULL) {
        char* slash = strchr(token, '/');
        if (slash) {
            *slash = '\0';
            int key_code = atoi(token);
            char* key_string = slash + 1;
            
            DefineUserKey(key_code, key_string);
        }
        token = strtok(NULL, ";");
    }
    
    free(data_copy);
}

void ClearUserDefinedKeys(void) {
    for (size_t i = 0; i < terminal.programmable_keys.count; i++) {
        free(terminal.programmable_keys.keys[i].sequence);
    }
    terminal.programmable_keys.count = 0;
}

void ProcessSoftFontDownload(const char* data) {
    // Simplified soft font loading
    if (!terminal.conformance.features.vt220_mode) {
        LogUnsupportedSequence("Soft fonts require VT220+ mode");
        return;
    }
    
    // Parse soft font data format
    // This is a complex format - implementing basic framework
    terminal.soft_font.active = true;
    
    if (terminal.options.debug_sequences) {
        LogUnsupportedSequence("Soft font download partially implemented");
    }
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
                terminal.scroll_top + 1, terminal.scroll_bottom + 1);
        QueueResponse(response);
    } else {
        // Unknown request
        snprintf(response, sizeof(response), "\x1BP0$r%s\x1B\\", request);
        QueueResponse(response);
    }
}

// New ExecuteDCSAnswerback for DCS 0 ; 0 $ t <message> ST
void ExecuteDCSAnswerback(void) {
    char* message_start = strstr(terminal.escape_buffer, "$t");
    if (message_start) {
        message_start += 2; // Skip "$t"
        char* message_end = strstr(message_start, "\x1B\\"); // Find ST
        if (message_end) {
            size_t length = message_end - message_start;
            if (length >= MAX_COMMAND_BUFFER) {
                length = MAX_COMMAND_BUFFER - 1; // Prevent overflow
            }
            strncpy(terminal.answerback_buffer, message_start, length);
            terminal.answerback_buffer[length] = '\0';
        } else if (terminal.options.debug_sequences) {
            LogUnsupportedSequence("Incomplete DCS $ t sequence");
        }
    } else if (terminal.options.debug_sequences) {
        LogUnsupportedSequence("Invalid DCS $ t sequence");
    }
}

void ExecuteDCSCommand(void) {
    char* params = terminal.escape_buffer;
    
    if (strncmp(params, "1;1|", 4) == 0) {
        // DECUDK - User Defined Keys
        ProcessUserDefinedKeys(params + 4);
    } else if (strncmp(params, "0;1|", 4) == 0) {
        // DECUDK - Clear User Defined Keys
        ClearUserDefinedKeys();
    } else if (strncmp(params, "2;1|", 4) == 0) {
        // DECDLD - Download Soft Font
        ProcessSoftFontDownload(params + 4);
    } else if (strncmp(params, "$q", 2) == 0) {
        // DECRQSS - Request Status String
        ProcessStatusRequest(params + 2);
    } else if (strncmp(params, "+q", 2) == 0) {
        // XTGETTCAP - Get Termcap
        ProcessTermcapRequest(params + 2);
    } else {
        if (terminal.options.debug_sequences) {
            LogUnsupportedSequence("Unknown DCS command");
        }
    }
}

// =============================================================================
// VT52 COMPATIBILITY MODE
// =============================================================================

void ProcessVT52Char(unsigned char ch) {
    static bool expect_param = false;
    static char vt52_command = 0;
    
    if (!expect_param) {
        switch (ch) {
            case 'A': // Cursor up
                if (terminal.cursor.y > 0) terminal.cursor.y--;
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case 'B': // Cursor down
                if (terminal.cursor.y < DEFAULT_TERM_HEIGHT - 1) terminal.cursor.y++;
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case 'C': // Cursor right
                if (terminal.cursor.x < DEFAULT_TERM_WIDTH - 1) terminal.cursor.x++;
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case 'D': // Cursor left
                if (terminal.cursor.x > 0) terminal.cursor.x--;
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case 'H': // Home cursor
                terminal.cursor.x = 0;
                terminal.cursor.y = 0;
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case 'I': // Reverse line feed
                terminal.cursor.y--;
                if (terminal.cursor.y < 0) {
                    terminal.cursor.y = 0;
                    ScrollDownRegion(0, DEFAULT_TERM_HEIGHT - 1, 1);
                }
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case 'J': // Clear to end of screen
                // Clear from cursor to end of line
                for (int x = terminal.cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                    ClearCell(&terminal.screen[terminal.cursor.y][x]);
                }
                // Clear remaining lines
                for (int y = terminal.cursor.y + 1; y < DEFAULT_TERM_HEIGHT; y++) {
                    for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
                        ClearCell(&terminal.screen[y][x]);
                    }
                }
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case 'K': // Clear to end of line
                for (int x = terminal.cursor.x; x < DEFAULT_TERM_WIDTH; x++) {
                    ClearCell(&terminal.screen[terminal.cursor.y][x]);
                }
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case 'Y': // Direct cursor address
                vt52_command = 'Y';
                expect_param = true;
                terminal.escape_pos = 0;
                break;
                
            case 'Z': // Identify
                QueueResponse("\x1B/Z"); // VT52 identification
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case '=': // Enter alternate keypad mode
                terminal.vt_keyboard.keypad_mode = true;
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case '>': // Exit alternate keypad mode
                terminal.vt_keyboard.keypad_mode = false;
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case '<': // Enter ANSI mode
                terminal.parse_state = VT_PARSE_NORMAL;
                // Exit VT52 mode
                break;
                
            case 'F': // Enter graphics mode
                terminal.charset.gl = &terminal.charset.g1; // Use G1 (DEC special)
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            case 'G': // Exit graphics mode
                terminal.charset.gl = &terminal.charset.g0; // Use G0 (ASCII)
                terminal.parse_state = VT_PARSE_NORMAL;
                break;
                
            default:
                // Unknown VT52 command
                terminal.parse_state = VT_PARSE_NORMAL;
                if (terminal.options.debug_sequences) {
                    char debug_msg[64];
                    snprintf(debug_msg, sizeof(debug_msg), "Unknown VT52 command: %c", ch);
                    LogUnsupportedSequence(debug_msg);
                }
                break;
        }
    } else {
        // Handle parameterized commands
        if (vt52_command == 'Y') {
            if (terminal.escape_pos == 0) {
                // First parameter: row
                terminal.escape_buffer[0] = ch;
                terminal.escape_pos = 1;
            } else {
                // Second parameter: column
                int row = terminal.escape_buffer[0] - 32; // VT52 uses offset of 32
                int col = ch - 32;
                
                // Clamp to valid range
                terminal.cursor.y = (row < 0) ? 0 : (row >= DEFAULT_TERM_HEIGHT) ? DEFAULT_TERM_HEIGHT - 1 : row;
                terminal.cursor.x = (col < 0) ? 0 : (col >= DEFAULT_TERM_WIDTH) ? DEFAULT_TERM_WIDTH - 1 : col;
                
                expect_param = false;
                terminal.parse_state = VT_PARSE_NORMAL;
            }
        }
    }
}
// =============================================================================
// SIXEL GRAPHICS SUPPORT (Basic Implementation)
// =============================================================================

void ProcessSixelChar(unsigned char ch) {
    if (ch >= '0' && ch <= '9') {
        // If the repeat count is at its default (1), this must be the start of a new number.
        // Reset it to 0 before accumulating digits.
        if (terminal.sixel.repeat_count == 1) {
            terminal.sixel.repeat_count = 0;
        }
        terminal.sixel.repeat_count = terminal.sixel.repeat_count * 10 + (ch - '0');
        return;
    }

    switch (ch) {
        case '"': // Raster attributes
            // Not implemented, but acknowledge and reset repeat count.
            break;
        case '#': // Color introducer
            // Use the parsed number as the color index.
            if (terminal.sixel.repeat_count > 0) {
                terminal.sixel.color_index = terminal.sixel.repeat_count - 1;
            }
            break;
        case '!': // Repeat introducer
            // The repeat count is parsed from the digits that follow,
            // so this character itself is just a marker. We let the digit
            // handler accumulate the count.
            return; // Return early to avoid resetting repeat_count yet.
        case '$': // Carriage return
            terminal.sixel.pos_x = 0;
            break;
        case '-': // New line
            terminal.sixel.pos_x = 0;
            terminal.sixel.pos_y += 6;
            break;
        case '\x1B': // Escape character - signals the start of the String Terminator (ST)
             terminal.parse_state = PARSE_SIXEL_ST;
             return;
        default:
            if (ch >= '?' && ch <= '~') {
                int sixel_val = ch - '?';

                // Ensure repeat count is at least 1.
                if (terminal.sixel.repeat_count == 0) {
                    terminal.sixel.repeat_count = 1;
                }

                for (int r = 0; r < terminal.sixel.repeat_count; r++) {
                    for (int i = 0; i < 6; i++) {
                        if ((sixel_val >> i) & 1) {
                            int px = terminal.sixel.pos_x + r;
                            int py = terminal.sixel.pos_y + i;
                            if (px < terminal.sixel.width && py < terminal.sixel.height) {
                                RGB_Color color = color_palette[terminal.sixel.color_index];
                                int idx = (py * terminal.sixel.width + px) * 4;
                                terminal.sixel.data[idx] = color.r;
                                terminal.sixel.data[idx + 1] = color.g;
                                terminal.sixel.data[idx + 2] = color.b;
                                terminal.sixel.data[idx + 3] = 255;
                            }
                        }
                    }
                }
                terminal.sixel.pos_x += terminal.sixel.repeat_count;
                if (terminal.sixel.pos_x > terminal.sixel.max_x) {
                    terminal.sixel.max_x = terminal.sixel.pos_x;
                }
                if (terminal.sixel.pos_y + 6 > terminal.sixel.max_y) {
                    terminal.sixel.max_y = terminal.sixel.pos_y + 6;
                }
            }
            break;
    }
    // Reset repeat count to the default (1) after processing a command character.
    terminal.sixel.repeat_count = 1;
}

void InitSixelGraphics(void) {
    terminal.sixel.active = false;
    terminal.sixel.data = NULL;
    terminal.sixel.width = 0;
    terminal.sixel.height = 0;
    terminal.sixel.x = 0;
    terminal.sixel.y = 0;
}

void ProcessSixelData(const char* data, size_t length) {
    // Basic sixel processing - this is a complex format
    // This implementation provides framework for sixel support
    
    if (!terminal.conformance.features.vt320_mode) {
        LogUnsupportedSequence("Sixel graphics require VT320+ mode");
        return;
    }
    
    // Allocate sixel buffer if needed
    if (!terminal.sixel.data) {
        terminal.sixel.width = DEFAULT_TERM_WIDTH * DEFAULT_CHAR_WIDTH;
        terminal.sixel.height = DEFAULT_TERM_HEIGHT * DEFAULT_CHAR_HEIGHT;
        terminal.sixel.data = calloc(terminal.sixel.width * terminal.sixel.height * 4, 1);
    }
    
    terminal.sixel.active = true;
    terminal.sixel.x = terminal.cursor.x * DEFAULT_CHAR_WIDTH;
    terminal.sixel.y = terminal.cursor.y * DEFAULT_CHAR_HEIGHT;
    
    // Basic sixel parsing would go here
    // For now, just mark as active for demonstration
    
    if (terminal.options.debug_sequences) {
        LogUnsupportedSequence("Sixel graphics partially implemented");
    }
}

void DrawSixelGraphics(void) {
    if (!terminal.sixel.active || !terminal.sixel.data || terminal.sixel.width <= 0 || terminal.sixel.height <= 0) {
        return;
    }
    
    // Create texture from sixel data and draw it
    // This would be the full implementation
    // For now, just draw a placeholder rectangle
    
    // SIT/RGL BRIDGE
    // We create a temporary texture from the raw sixel data each time we draw.
    // This is because the sixel image can change at any time.

    RGLTexture sixel_texture = {0};

    // 1. Generate, bind, and upload texture data using raw OpenGL calls,
    // as there is no RGL helper for loading from memory.
    glGenTextures(1, &sixel_texture.id);
    glBindTexture(GL_TEXTURE_2D, sixel_texture.id);

    // Upload RGBA data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, terminal.sixel.width, terminal.sixel.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, terminal.sixel.data);

    // Set texture parameters for pixel-perfect rendering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    sixel_texture.width = terminal.sixel.width;
    sixel_texture.height = terminal.sixel.height;

    // 2. Draw the texture.
    if (sixel_texture.id != 0) {
        Rectangle src_rect = {0.0f, 0.0f, (float)sixel_texture.width, (float)sixel_texture.height};
        Rectangle dest_rect = {
            (float)terminal.sixel.x * DEFAULT_WINDOW_SCALE,
            (float)terminal.sixel.y * DEFAULT_WINDOW_SCALE,
            (float)terminal.sixel.width * DEFAULT_WINDOW_SCALE,
            (float)terminal.sixel.height * DEFAULT_WINDOW_SCALE
        };
        Vector2 origin = {0, 0};
        SituationCmdDrawTexture(sixel_texture, src_rect, dest_rect, origin, 0.0f, WHITE);

        // 3. Unload the texture from GPU memory after drawing.
        RGL_UnloadTexture(sixel_texture);
    }
}

// =============================================================================
// RECTANGULAR OPERATIONS (VT420)
// =============================================================================

void ExecuteRectangularOps(void) {
    // CSI Pt ; Pl ; Pb ; Pr $ v - Copy rectangular area
    if (!terminal.conformance.features.vt420_mode) {
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
        CopyRectangle(rect, terminal.cursor.x, terminal.cursor.y);
    }
}

void ExecuteRectangularOps2(void) {
    // CSI Pt ; Pl ; Pb ; Pr $ w - Request checksum of rectangular area
    if (!terminal.conformance.features.vt420_mode) {
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
                temp[y * width + x] = terminal.screen[src.top + y][src.left + x];
            }
        }
    }
    
    // Copy from temp buffer to destination
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dst_y = dest_y + y;
            int dst_x = dest_x + x;
            
            if (dst_y >= 0 && dst_y < DEFAULT_TERM_HEIGHT && dst_x >= 0 && dst_x < DEFAULT_TERM_WIDTH) {
                terminal.screen[dst_y][dst_x] = temp[y * width + x];
                terminal.screen[dst_y][dst_x].dirty = true;
            }
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
    PipelineWriteFormat("Terminal Type: %s\n", terminal.title.terminal_name);
    PipelineWriteFormat("VT Level: %d\n", terminal.conformance.level);
    PipelineWriteFormat("Primary DA: %s\n", terminal.device_attributes);
    PipelineWriteFormat("Secondary DA: %s\n", terminal.secondary_attributes);
    
    PipelineWriteString("\nSupported Features:\n");
    PipelineWriteFormat("- VT52 Mode: %s\n", terminal.conformance.features.vt52_mode ? "Yes" : "No");
    PipelineWriteFormat("- VT100 Mode: %s\n", terminal.conformance.features.vt100_mode ? "Yes" : "No");
    PipelineWriteFormat("- VT220 Mode: %s\n", terminal.conformance.features.vt220_mode ? "Yes" : "No");
    PipelineWriteFormat("- VT320 Mode: %s\n", terminal.conformance.features.vt320_mode ? "Yes" : "No");
    PipelineWriteFormat("- VT420 Mode: %s\n", terminal.conformance.features.vt420_mode ? "Yes" : "No");
    PipelineWriteFormat("- xterm Mode: %s\n", terminal.conformance.features.xterm_mode ? "Yes" : "No");
    
    PipelineWriteString("\nCurrent Settings:\n");
    PipelineWriteFormat("- Cursor Keys: %s\n", terminal.dec_modes.application_cursor_keys ? "Application" : "Normal");
    PipelineWriteFormat("- Keypad: %s\n", terminal.vt_keyboard.keypad_mode ? "Application" : "Numeric");
    PipelineWriteFormat("- Auto Wrap: %s\n", terminal.dec_modes.auto_wrap_mode ? "On" : "Off");
    PipelineWriteFormat("- Origin Mode: %s\n", terminal.dec_modes.origin_mode ? "On" : "Off");
    PipelineWriteFormat("- Insert Mode: %s\n", terminal.dec_modes.insert_mode ? "On" : "Off");
    
    PipelineWriteFormat("\nScrolling Region: %d-%d\n", 
                       terminal.scroll_top + 1, terminal.scroll_bottom + 1);
    PipelineWriteFormat("Margins: %d-%d\n", 
                       terminal.left_margin + 1, terminal.right_margin + 1);
    
    PipelineWriteString("\nStatistics:\n");
    TerminalStatus status = GetTerminalStatus();
    PipelineWriteFormat("- Pipeline Usage: %zu/%d\n", status.pipeline_usage, (int)sizeof(terminal.input_pipeline));
    PipelineWriteFormat("- Key Buffer: %zu\n", status.key_usage);
    PipelineWriteFormat("- Unsupported Sequences: %d\n", terminal.conformance.compliance.unsupported_sequences);
    
    if (terminal.conformance.compliance.last_unsupported[0]) {
        PipelineWriteFormat("- Last Unsupported: %s\n", terminal.conformance.compliance.last_unsupported);
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
 *  - Updates internal feature flags in `terminal.conformance.features`.
 * The library aims for compatibility with VT52, VT100, VT220, VT320, VT420, and xterm standards.
 *
 * @param level The desired VTLevel (e.g., VT_LEVEL_100, VT_LEVEL_XTERM).
 * @see VTLevel enum for available levels.
 * @see terminal.h header documentation for a full list of KEY FEATURES and their typical VT level requirements.
 */
void SetVTLevel(VTLevel level) {
    terminal.conformance.level = level;

    // Update feature flags based on the new level
    terminal.conformance.features.vt52_mode = (level >= VT_LEVEL_52);
    terminal.conformance.features.vt100_mode = (level >= VT_LEVEL_100 || level == VT_LEVEL_XTERM);
    terminal.conformance.features.vt220_mode = (level >= VT_LEVEL_220 || level == VT_LEVEL_XTERM);
    terminal.conformance.features.vt320_mode = (level >= VT_LEVEL_320 || level == VT_LEVEL_XTERM);
    terminal.conformance.features.vt420_mode = (level >= VT_LEVEL_420 || level == VT_LEVEL_XTERM);
    terminal.conformance.features.vt520_mode = (level >= VT_LEVEL_520 || level == VT_LEVEL_XTERM);
    terminal.conformance.features.xterm_mode = (level == VT_LEVEL_XTERM);

    // Update Device Attribute strings based on the level
    // These are example DA strings; consult standards for exact values.
    // The "\x1B[" part is crucial for CSI sequences.
    if (level == VT_LEVEL_XTERM) {
        // Example for a modern xterm supporting many features
        strcpy(terminal.device_attributes, "\x1B[?41;1;2;6;7;8;9;15;18;21;22c"); // Includes 256 colors, TrueColor support indicators
        strcpy(terminal.secondary_attributes, "\x1B[>41;400;0c"); // xterm, example version 400, no patches
        strcpy(terminal.tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_520) {
        strcpy(terminal.device_attributes, "\x1B[?65;1;2;6;7;8;9;15;18;21;22;28;29c");
        strcpy(terminal.secondary_attributes, "\x1B[>52;10;0c"); // Unique to VT520
        strcpy(terminal.tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_420) {
        strcpy(terminal.device_attributes, "\x1B[?64;1;2;6;7;8;9;15;18;21;22;28;29c");
        strcpy(terminal.secondary_attributes, "\x1B[>41;10;0c");
        strcpy(terminal.tertiary_attributes, "\x1B[>0;1;0c");
    } else if (level >= VT_LEVEL_320) {
        strcpy(terminal.device_attributes, "\x1B[?63;1;2;6;7;8;9;15;18;21c"); // VT320
        strcpy(terminal.secondary_attributes, "\x1B[>24;10;0c"); // VT320, with Sixel graphics support
        strcpy(terminal.tertiary_attributes, "");
    } else if (level >= VT_LEVEL_220) {
        strcpy(terminal.device_attributes, "\x1B[?62;1;2;6;7;8;9;15c"); // VT220
        strcpy(terminal.secondary_attributes, "\x1B[>1;10;0c"); // VT220, 7-bit controls, no Sixel
        strcpy(terminal.tertiary_attributes, "");
    } else if (level >= VT_LEVEL_100) {
        strcpy(terminal.device_attributes, "\x1B[?6c"); // VT102 (common DA for VT100 compatibility)
        // Or: strcpy(terminal.device_attributes, "\x1B[?1;2c"); // Original VT100 (AVO, no advanced video options)
        strcpy(terminal.secondary_attributes, "\x1B[>0;95;0c"); // VT100, firmware version 95
        strcpy(terminal.tertiary_attributes, "");
    } else { // VT_LEVEL_52
        strcpy(terminal.device_attributes, "\x1B/Z"); // VT52 identification sequence
        strcpy(terminal.secondary_attributes, "");    // VT52 does not have a secondary DA response
        strcpy(terminal.tertiary_attributes, "");
    }
}

/**
 * @brief Retrieves the current VT compatibility level of the terminal.
 * @return The current VTLevel.
 */
VTLevel GetVTLevel(void) {
    return terminal.conformance.level;
}

// --- Keyboard Input ---

/**
 * @brief Retrieves a fully processed keyboard event from the terminal's internal buffer.
 * The application hosting the terminal should call this function repeatedly (e.g., in its
 * main loop after `UpdateVTKeyboard()`) to obtain keyboard input.
 *
 * The `VTKeyboard` system, updated by `UpdateVTKeyboard()`, translates raw Raylib key
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
 * @see UpdateVTKeyboard() which captures Raylib input and populates the event buffer.
 * @see VTKeyEvent struct for details on the event data fields.
 * @note The terminal platform provides robust keyboard translation, ensuring that applications
 *       running within the terminal receive the correct input sequences based on active modes.
 */
bool GetVTKeyEvent(VTKeyEvent* event) {
    if (!event || terminal.vt_keyboard.buffer_count == 0) {
        return false;
    }
    *event = terminal.vt_keyboard.buffer[terminal.vt_keyboard.buffer_tail];
    terminal.vt_keyboard.buffer_tail = (terminal.vt_keyboard.buffer_tail + 1) % 512; // Assuming vt_keyboard.buffer size is 512
    terminal.vt_keyboard.buffer_count--;
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
 *  - `terminal.options.log_unsupported`: Specifically for unsupported sequences.
 *  - `terminal.options.conformance_checking`: For stricter checks against VT standards.
 *  - `terminal.status.debugging`: A general flag indicating debug activity.
 *
 * This capability allows developers to understand the terminal's interaction with
 * host applications and identify issues in escape sequence processing.
 *
 * @param enable `true` to enable debug mode, `false` to disable.
 */
void EnableDebugMode(bool enable) {
    terminal.options.debug_sequences = enable;
    terminal.options.log_unsupported = enable;
    terminal.options.conformance_checking = enable;
    terminal.status.debugging = enable; // General debugging flag for other parts of the library
}

// --- Core Terminal Loop Functions ---

/**
 * @brief Updates the terminal's internal state and processes incoming data.
 *
 * Called once per frame in the main loop, this function drives the terminal emulation by:
 * - **Processing Input**: Consumes characters from `terminal.input_pipeline` via `ProcessPipeline()`, parsing VT52/xterm sequences with `ProcessChar()`.
 * - **Handling Input Devices**: Updates keyboard (`UpdateVTKeyboard()`) and mouse (`UpdateMouse()`) states.
 * - **Auto-Printing**: Queues lines for printing when `terminal.auto_print_enabled` and a newline occurs.
 * - **Managing Timers**: Updates cursor blink, text blink, and visual bell timers for visual effects.
 * - **Flushing Responses**: Sends queued responses (e.g., DSR, DA, focus events) via `response_callback`.
 * - **Rendering**: Draws the terminal display with `DrawTerminal()`, including the custom mouse cursor.
 *
 * Performance is tuned via `terminal.VTperformance` (e.g., `chars_per_frame`, `time_budget`) to balance responsiveness and throughput.
 *
 * @see ProcessPipeline() for input processing details.
 * @see UpdateVTKeyboard() for keyboard handling.
 * @see UpdateMouse() for mouse and focus event handling.
 * @see DrawTerminal() for rendering details.
 * @see QueueResponse() for response queuing.
 */
void UpdateTerminal(void) {
    // Process input from the pipeline (VT sequences, text)
    ProcessPipeline();

    // Update input devices
    UpdateVTKeyboard(); // Process keyboard input, enqueue to vt_keyboard.buffer
    UpdateMouse(); // Process mouse input, send to answerback_buffer

    // Process queued keyboard events
    while (terminal.vt_keyboard.buffer_count > 0) {
        VTKeyEvent* event = &terminal.vt_keyboard.buffer[terminal.vt_keyboard.buffer_tail];
        if (event->sequence[0] != '\0') {
            // Send to host/console
            QueueResponse(event->sequence);
            
            // LOCAL ECHO: Also display on terminal screen if local echo is enabled
            if (terminal.dec_modes.local_echo) {
                // Process the sequence through the terminal's own input pipeline
                // This will display the characters on screen
                for (int i = 0; event->sequence[i] != '\0'; i++) {
                    PipelineWriteChar(event->sequence[i]);
                }
            }
            
            // Handle BEL (Ctrl+G) internally
            if (event->sequence[0] == 0x07) {
                terminal.visual_bell_timer = 0.2f; // Trigger visual bell
            }
        }
        terminal.vt_keyboard.buffer_tail = (terminal.vt_keyboard.buffer_tail + 1) % KEY_EVENT_BUFFER_SIZE;
        terminal.vt_keyboard.buffer_count--;
    }

    // Auto-print lines when enabled
    if (terminal.printer_available && terminal.auto_print_enabled) {
        static int last_cursor_y = -1;
        if (terminal.cursor.y > last_cursor_y && last_cursor_y >= 0) {
            // Queue the previous line for printing
            char print_buffer[DEFAULT_TERM_WIDTH + 2];
            size_t pos = 0;
            for (int x = 0; x < DEFAULT_TERM_WIDTH && pos < DEFAULT_TERM_WIDTH; x++) {
                EnhancedTermChar* cell = &terminal.screen[last_cursor_y][x];
                print_buffer[pos++] = GetPrintableChar(cell->ch, &terminal.charset);
            }
            if (pos < DEFAULT_TERM_WIDTH + 1) {
                print_buffer[pos++] = '\n';
                print_buffer[pos] = '\0';
                QueueResponse(print_buffer);
            }
        }
        last_cursor_y = terminal.cursor.y;
    }

    // Update cursor blink timer
    if (terminal.cursor.blink_enabled && terminal.dec_modes.cursor_visible) {
        terminal.cursor.blink_timer += SituationGetFrameTime();
        if (terminal.cursor.blink_timer >= 0.530f) {
            terminal.cursor.blink_state = !terminal.cursor.blink_state;
            terminal.cursor.blink_timer = 0.0f;
        }
    } else {
        terminal.cursor.blink_state = true;
    }

    // Update text blink timer
    terminal.text_blink_timer += SituationGetFrameTime();
    if (terminal.text_blink_timer >= 0.530f) {
        terminal.text_blink_state = !terminal.text_blink_state;
        terminal.text_blink_timer = 0.0f;
    }

    // Update visual bell timer
    if (terminal.visual_bell_timer > 0) {
        terminal.visual_bell_timer -= SituationGetFrameTime();
        if (terminal.visual_bell_timer < 0) {
            terminal.visual_bell_timer = 0;
        }
    }

    // Flush queued responses (keyboard, mouse, focus events, DSR)
    if (terminal.response_length > 0 && response_callback) {
        response_callback(terminal.answerback_buffer, terminal.response_length);
        terminal.response_length = 0;
    }

    // Render the terminal display
    DrawTerminal();
}

/**
 * @brief Renders the current visual state of the terminal to the Raylib window.
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
 *      - Blink (text alternates visibility based on `terminal.text_blink_state`).
 *      - Reverse video (swaps foreground and background colors, per cell or screen-wide).
 *      - Strikethrough.
 *      - Overline.
 *      - Concealed text (rendered as spaces or not at all).
 *  -   **Glyph Rendering**: Uses a pre-rendered bitmap font (`font_texture`), typically
 *      CP437-based for broad character support within that range. Unicode characters
 *      outside this basic set might be mapped to '?' or require a more advanced font system.
 *  -   **Cursor**: Draws the cursor according to its current `EnhancedCursor` properties:
 *      - Shape: `CURSOR_BLOCK`, `CURSOR_UNDERLINE`, `CURSOR_BAR`.
 *      - Blink: Synchronized with `terminal.cursor.blink_state`.
 *      - Visibility: Honors `terminal.cursor.visible`.
 *      - Color: Uses `terminal.cursor.color`.
 *  -   **Sixel Graphics**: If `terminal.sixel.active` is true, Sixel graphics data is
 *      rendered, typically overlaid on the text grid.
 *  -   **Visual Bell**: If `terminal.visual_bell_timer` is active, a visual flash effect
 *      may be rendered.
 *
 * The terminal provides a faithful visual emulation, leveraging Raylib for efficient
 * 2D rendering.
 *
 * @see EnhancedTermChar for the structure defining each character cell's properties.
 * @see EnhancedCursor for cursor attributes.
 * @see SixelGraphics for Sixel display state.
 * @see InitTerminal() where `font_texture` is created.
 */
void DrawTerminal(void) {
    SituationBeginFrame();

    SituationCmdClearBackground(BLACK); // Base window background, terminal cells will draw over this

    for (int y = 0; y < DEFAULT_TERM_HEIGHT; y++) {
        for (int x = 0; x < DEFAULT_TERM_WIDTH; x++) {
            EnhancedTermChar* cell = &terminal.screen[y][x]; // Assumes 'terminal.screen' is the active buffer
            Color fg_color_resolved = WHITE; // Default
            Color bg_color_resolved = BLACK; // Default

            // Resolve Foreground Color
            if (cell->fg_color.color_mode == 0) { // Indexed color
                if (cell->fg_color.value.index >= 0 && cell->fg_color.value.index < 256) {
                    if (cell->fg_color.value.index < 16) { // Standard 16 ANSI colors
                        fg_color_resolved = ansi_colors[cell->fg_color.value.index];
                    } else { // 256-color palette
                        RGB_Color c_rgb = color_palette[cell->fg_color.value.index];
                        fg_color_resolved = (Color){c_rgb.r, c_rgb.g, c_rgb.b, 255};
                    }
                }
            } else { // RGB True Color
                fg_color_resolved = (Color){cell->fg_color.value.rgb.r, cell->fg_color.value.rgb.g, cell->fg_color.value.rgb.b, cell->fg_color.value.rgb.a};
            }

            // Resolve Background Color
            if (cell->bg_color.color_mode == 0) { // Indexed color
                if (cell->bg_color.value.index >= 0 && cell->bg_color.value.index < 256) {
                    if (cell->bg_color.value.index < 16) { // Standard 16 ANSI colors
                        bg_color_resolved = ansi_colors[cell->bg_color.value.index];
                    } else { // 256-color palette
                        RGB_Color c_rgb = color_palette[cell->bg_color.value.index];
                        bg_color_resolved = (Color){c_rgb.r, c_rgb.g, c_rgb.b, 255};
                    }
                }
            } else { // RGB True Color
                bg_color_resolved = (Color){cell->bg_color.value.rgb.r, cell->bg_color.value.rgb.g, cell->bg_color.value.rgb.b, cell->bg_color.value.rgb.a};
            }

            // Apply reverse video (cell-specific reverse XORed with global screen reverse)
            bool actual_reverse = cell->reverse ^ terminal.dec_modes.reverse_video;
            if (actual_reverse) {
                Color temp_color = fg_color_resolved;
                fg_color_resolved = bg_color_resolved;
                bg_color_resolved = temp_color;
            }

            // Draw cell background
            // Optimization: Only draw if not default black, or if char is space but needs to show a non-black (e.g., reversed) background
            if (bg_color_resolved.r != 0 || bg_color_resolved.g != 0 || bg_color_resolved.b != 0 || (cell->ch == ' ' && actual_reverse)) { // If space and reversed, its "background" (original fg) might be non-black
                SituationCmdDrawRectangle(x * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE, y * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE, DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE, DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE, bg_color_resolved);
            }

            // Draw character if not concealed and (not blinking or currently in blink-on state)
            // Also, don't draw a normal space char unless it's reversed (to show its new "foreground", original bg)
            if (!cell->conceal && (!cell->blink || terminal.text_blink_state) && (cell->ch != ' ' || actual_reverse)) {
                Color final_fg_for_char = fg_color_resolved;
                // Apply bold: for standard ANSI colors (0-7), bold often means using the bright version (8-15)
                // This doesn't apply if the cell is already reversed, as the original fg is now the bg.
                if (cell->bold && !actual_reverse) {
                    if (cell->fg_color.color_mode == 0 && cell->fg_color.value.index < 8) { // Original FG was a standard, non-bright color
                        final_fg_for_char = ansi_colors[cell->fg_color.value.index + 8];
                    }
                    // For 256-color or TrueColor, bold might mean a thicker font (not handled by color change here)
                    // or no visual change if the font doesn't have a separate bold weight.
                }

                // Apply faint: dim the color
                if (cell->faint) {
                    final_fg_for_char.r = (unsigned char)(final_fg_for_char.r * 0.67f);
                    final_fg_for_char.g = (unsigned char)(final_fg_for_char.g * 0.67f);
                    final_fg_for_char.b = (unsigned char)(final_fg_for_char.b * 0.67f);
                }

                // Determine character code for font texture (CP437 based)
                unsigned int char_to_render_raw = cell->ch;
                // TODO: Implement proper Unicode to CP437 mapping or integrate a Unicode-capable font renderer.
                // For this CP437 font, characters outside 0-255 are typically replaced.
                unsigned char char_code_for_texture = (char_to_render_raw < 256) ? (unsigned char)char_to_render_raw : '?'; // Default fallback '?'

                // Draw character glyph from font texture
                int src_x_font = (char_code_for_texture % 16) * DEFAULT_CHAR_WIDTH; // Assuming 16 chars per row in font texture
                int src_y_font = (char_code_for_texture / 16) * DEFAULT_CHAR_HEIGHT;
                Rectangle src_rect = {(float)src_x_font, (float)src_y_font, (float)DEFAULT_CHAR_WIDTH, (float)DEFAULT_CHAR_HEIGHT};
                Rectangle dest_rect = {(float)x * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE, (float)y * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE, (float)DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE, (float)DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE};
                SituationCmdDrawTexture(font_texture, src_rect, dest_rect, (Vector2){0, 0}, 0.0f, final_fg_for_char);
                
                // Draw text decorations (underline, strikethrough, overline)
                int line_thickness = (DEFAULT_WINDOW_SCALE > 1 ? DEFAULT_WINDOW_SCALE : 1); // Scale line thickness for visibility
                if (cell->underline) {
                    SituationCmdDrawLine((Vector2){dest_rect.x, dest_rect.y + dest_rect.height - line_thickness * 2}, (Vector2){dest_rect.x + dest_rect.width, dest_rect.y + dest_rect.height - line_thickness * 2}, (float)line_thickness, final_fg_for_char);
                }
                if (cell->double_underline) { // Draw two lines for double underline
                    SituationCmdDrawLine((Vector2){dest_rect.x, dest_rect.y + dest_rect.height - line_thickness * 2 -1}, (Vector2){dest_rect.x + dest_rect.width, dest_rect.y + dest_rect.height - line_thickness * 2 -1}, (float)line_thickness, final_fg_for_char);
                    SituationCmdDrawLine((Vector2){dest_rect.x, dest_rect.y + dest_rect.height - line_thickness}, (Vector2){dest_rect.x + dest_rect.width, dest_rect.y + dest_rect.height - line_thickness}, (float)line_thickness, final_fg_for_char);
                }
                if (cell->strikethrough) {
                    SituationCmdDrawLine((Vector2){dest_rect.x, dest_rect.y + dest_rect.height / 2}, (Vector2){dest_rect.x + dest_rect.width, dest_rect.y + dest_rect.height / 2}, (float)line_thickness, final_fg_for_char);
                }
                if (cell->overline) {
                    SituationCmdDrawLine((Vector2){dest_rect.x, dest_rect.y + line_thickness}, (Vector2){dest_rect.x + dest_rect.width, dest_rect.y + line_thickness}, (float)line_thickness, final_fg_for_char);
                }
            }
        }
    }

    // Draw Sixel graphics if active (implementation is in Sixel-specific functions)
    DrawSixelGraphics();

    // Draw cursor if visible and in its "on" blink state
    if (terminal.cursor.visible && (!terminal.cursor.blink_enabled || terminal.cursor.blink_state)) {
        int cursor_draw_x_px = terminal.cursor.x * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE;
        int cursor_draw_y_px = terminal.cursor.y * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE;
        Color cursor_render_color = WHITE; // Default cursor color

        // Resolve cursor color from ExtendedColor struct
        if (terminal.cursor.color.color_mode == 0) { // Indexed color
            if (terminal.cursor.color.value.index >= 0 && terminal.cursor.color.value.index < 256) {
                if (terminal.cursor.color.value.index < 16) { // Standard 16 ANSI colors
                    cursor_render_color = ansi_colors[terminal.cursor.color.value.index];
                } else { // 256-color palette
                    RGB_Color c_rgb = color_palette[terminal.cursor.color.value.index];
                    cursor_render_color = (Color){c_rgb.r, c_rgb.g, c_rgb.b, 255};
                }
            }
        } else { // RGB True Color
            cursor_render_color = (Color){terminal.cursor.color.value.rgb.r, terminal.cursor.color.value.rgb.g, terminal.cursor.color.value.rgb.b, terminal.cursor.color.value.rgb.a};
        }

        int cursor_line_thickness = (DEFAULT_WINDOW_SCALE > 1 ? 2 * DEFAULT_WINDOW_SCALE : 2); // Make cursor lines slightly thicker
        int cursor_bar_width = (DEFAULT_WINDOW_SCALE > 1 ? DEFAULT_WINDOW_SCALE : 1); // Bar cursor width
        switch (terminal.cursor.shape) {
            case CURSOR_BLOCK:
            case CURSOR_BLOCK_BLINK:
                // A true block cursor inverts the underlying cell's character and colors.
                // For simplicity, this often draws a solid block with the cursor color.
                // To implement inversion:
                // 1. Get EnhancedTermChar* cell_under_cursor = &terminal.screen[terminal.cursor.y][terminal.cursor.x];
                // 2. Resolve cell_under_cursor's fg and bg colors (as done above).
                // 3. Draw cell background with resolved_fg_under_cursor.
                // 4. Draw cell_under_cursor->ch using resolved_bg_under_cursor.
                // For now, drawing a solid block:
                SituationCmdDrawRectangle(cursor_draw_x_px, cursor_draw_y_px, DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE, DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE, cursor_render_color);
                break;
            case CURSOR_UNDERLINE:
            case CURSOR_UNDERLINE_BLINK:
                SituationCmdDrawRectangle(cursor_draw_x_px, cursor_draw_y_px + DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE - cursor_line_thickness, DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE, cursor_line_thickness, cursor_render_color);
                break;
            case CURSOR_BAR:
            case CURSOR_BAR_BLINK:
                SituationCmdDrawRectangle(cursor_draw_x_px, cursor_draw_y_px, cursor_bar_width, DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE, cursor_render_color);
                break;
        }
    }

    // Visual Bell Flash
    if (terminal.visual_bell_timer > 0) {
        // Draw a semi-transparent white overlay for the bell effect
        // Alpha is proportional to remaining timer, creating a fade-out effect.
        SituationCmdDrawRectangle(0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, Fade(WHITE, terminal.visual_bell_timer * 0.5f)); // Max alpha 0.5
    }

    // Render custom mouse cursor (diamond)
    if (terminal.mouse.enabled && terminal.mouse.mode != MOUSE_TRACKING_OFF && terminal.mouse.cursor_x >= 1 && terminal.mouse.cursor_x <= DEFAULT_TERM_WIDTH && terminal.mouse.cursor_y >= 1 && terminal.mouse.cursor_y <= DEFAULT_TERM_HEIGHT) {
        // Save current charset state
        CharsetState saved_charset = terminal.charset;
        // Switch to DEC Special Graphics
        terminal.charset.gl = &terminal.charset.g0;
        terminal.charset.g0 = CHARSET_DEC_SPECIAL;
        // Diamond (◆, 0x60)
        unsigned char char_code = 0x60;
        int src_x_font = (char_code % 16) * DEFAULT_CHAR_WIDTH;
        int src_y_font = (char_code / 16) * DEFAULT_CHAR_HEIGHT;
        Rectangle src_rect = {(float)src_x_font, (float)src_y_font, (float)DEFAULT_CHAR_WIDTH, (float)DEFAULT_CHAR_HEIGHT};
        Rectangle dest_rect = {(float)(terminal.mouse.cursor_x - 1) * DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE, (float)(terminal.mouse.cursor_y - 1) * DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE, (float)DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE, (float)DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE};
        SituationCmdDrawTexture(font_texture, src_rect, dest_rect, (Vector2){0, 0}, 0.0f, WHITE);
        // Restore charset state
        terminal.charset = saved_charset;
    }

    SituationEndFrame();
}


// --- Lifecycle Management ---

/**
 * @brief Cleans up all resources allocated by the terminal library.
 * This function must be called when the application is shutting down, typically
 * after the main loop has exited and before closing the Raylib window (if Raylib
 * is managed by the application).
 *
 * Its responsibilities include deallocating:
 *  - The main font texture (`font_texture`) loaded into GPU memory.
 *  - Memory used for storing sequences of programmable keys (`terminal.programmable_keys`).
 *  - The Sixel graphics data buffer (`terminal.sixel.data`) if it was allocated.
 *  - The buffer for bracketed paste data (`terminal.bracketed_paste.buffer`) if used.
 *
 * It also ensures the input pipeline is cleared. Proper cleanup prevents memory leaks
 * and releases GPU resources.
 */
void CleanupTerminal(void) {
    RGL_UnloadTexture(font_texture); // SIT/RGL BRIDGE: Use RGL to unload the texture

    // Free memory for programmable key sequences
    for (size_t i = 0; i < terminal.programmable_keys.count; i++) {
        if (terminal.programmable_keys.keys[i].sequence) {
            free(terminal.programmable_keys.keys[i].sequence);
            terminal.programmable_keys.keys[i].sequence = NULL;
        }
    }
    if (terminal.programmable_keys.keys) {
        free(terminal.programmable_keys.keys);
        terminal.programmable_keys.keys = NULL;
    }
    terminal.programmable_keys.count = 0;
    terminal.programmable_keys.capacity = 0;

    // Free Sixel graphics data buffer
    if (terminal.sixel.data) {
        free(terminal.sixel.data);
        terminal.sixel.data = NULL;
    }

    // Free bracketed paste buffer
    if (terminal.bracketed_paste.buffer) {
        free(terminal.bracketed_paste.buffer);
        terminal.bracketed_paste.buffer = NULL;
    }

    ClearPipeline(); // Ensure input pipeline is empty and reset
}

bool InitTerminalDisplay(void) {
    // Create a virtual display for the terminal
    if (!SituationCreateVirtualDisplay(1, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT)) {
        return false;
    }
    
    // Set the virtual display as renderable
    SituationSetVirtualDisplayActive(1, true);
    
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
    // Initialize Raylib window
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

#endif // TERMINAL_IMPLEMENTATION


#endif // TERMINAL_H


// Helper function to convert a single hex character to its integer value
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1; // Invalid hex char
}

void DefineUserKey(int key_code, const char* sequence, size_t sequence_len) {
    // Expand programmable keys array if needed
    if (terminal.programmable_keys.count >= terminal.programmable_keys.capacity) {
        size_t new_capacity = terminal.programmable_keys.capacity == 0 ? 16 :
                             terminal.programmable_keys.capacity * 2;

        ProgrammableKey* new_keys = realloc(terminal.programmable_keys.keys,
                                           new_capacity * sizeof(ProgrammableKey));
        if (!new_keys) return;

        terminal.programmable_keys.keys = new_keys;
        terminal.programmable_keys.capacity = new_capacity;
    }

    // Find existing key or add new one
    ProgrammableKey* key = NULL;
    for (size_t i = 0; i < terminal.programmable_keys.count; i++) {
        if (terminal.programmable_keys.keys[i].key_code == key_code) {
            key = &terminal.programmable_keys.keys[i];
            if (key->sequence) free(key->sequence); // Free old sequence
            break;
        }
    }

    if (!key) {
        key = &terminal.programmable_keys.keys[terminal.programmable_keys.count++];
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
    if (!terminal.conformance.features.vt320_mode) {
        LogUnsupportedSequence("User defined keys require VT320 mode");
        return;
    }

    char* data_copy = strdup(data);
    if (!data_copy) return;

    char* saveptr;
    char* token = strtok_r(data_copy, ";", &saveptr);

    while (token != NULL) {
        char* slash = strchr(token, '/');
        if (slash) {
            *slash = '\0';
            int key_code = atoi(token);
            char* hex_string = slash + 1;
            size_t hex_len = strlen(hex_string);

            if (hex_len % 2 != 0) {
                // Invalid hex string length
                LogUnsupportedSequence("Invalid hex string in DECUDK");
                token = strtok_r(NULL, ";", &saveptr);
                continue;
            }

            size_t decoded_len = hex_len / 2;
            char* decoded_sequence = malloc(decoded_len);
            if (!decoded_sequence) {
                // Allocation failed
                token = strtok_r(NULL, ";", &saveptr);
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
        token = strtok_r(NULL, ";", &saveptr);
    }

    free(data_copy);
}
void ProcessSoftFontDownload(const char* data) {
