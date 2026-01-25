#ifndef KT_CP437_TABLES_H
#define KT_CP437_TABLES_H

#ifdef __cplusplus
extern "C" {
#endif

// CP437 Line Drawing Constants
typedef enum {
    CP437_LIGHT_VERTICAL = 0xB3,
    CP437_LIGHT_VERTICAL_AND_LEFT = 0xB4,
    CP437_VERTICAL_SINGLE_AND_LEFT_DOUBLE = 0xB5,
    CP437_VERTICAL_DOUBLE_AND_LEFT_SINGLE = 0xB6,
    CP437_DOWN_DOUBLE_AND_LEFT_SINGLE = 0xB7,
    CP437_DOWN_SINGLE_AND_LEFT_DOUBLE = 0xB8,
    CP437_DOUBLE_VERTICAL_AND_LEFT = 0xB9,
    CP437_DOUBLE_VERTICAL = 0xBA,
    CP437_DOUBLE_DOWN_AND_LEFT = 0xBB,
    CP437_DOUBLE_UP_AND_LEFT = 0xBC,
    CP437_UP_DOUBLE_AND_LEFT_SINGLE = 0xBD,
    CP437_UP_SINGLE_AND_LEFT_DOUBLE = 0xBE,
    CP437_LIGHT_DOWN_AND_LEFT = 0xBF,
    CP437_LIGHT_UP_AND_RIGHT = 0xC0,
    CP437_LIGHT_UP_AND_HORIZONTAL = 0xC1,
    CP437_LIGHT_DOWN_AND_HORIZONTAL = 0xC2,
    CP437_LIGHT_VERTICAL_AND_RIGHT = 0xC3,
    CP437_LIGHT_HORIZONTAL = 0xC4,
    CP437_LIGHT_VERTICAL_AND_HORIZONTAL = 0xC5,
    CP437_VERTICAL_SINGLE_AND_RIGHT_DOUBLE = 0xC6,
    CP437_VERTICAL_DOUBLE_AND_RIGHT_SINGLE = 0xC7,
    CP437_DOUBLE_UP_AND_RIGHT = 0xC8,
    CP437_DOUBLE_DOWN_AND_RIGHT = 0xC9,
    CP437_DOUBLE_UP_AND_HORIZONTAL = 0xCA,
    CP437_DOUBLE_DOWN_AND_HORIZONTAL = 0xCB,
    CP437_DOUBLE_VERTICAL_AND_RIGHT = 0xCC,
    CP437_DOUBLE_HORIZONTAL = 0xCD,
    CP437_DOUBLE_VERTICAL_AND_HORIZONTAL = 0xCE,
    CP437_UP_SINGLE_AND_HORIZONTAL_DOUBLE = 0xCF,
    CP437_UP_DOUBLE_AND_HORIZONTAL_SINGLE = 0xD0,
    CP437_DOWN_SINGLE_AND_HORIZONTAL_DOUBLE = 0xD1,
    CP437_DOWN_DOUBLE_AND_HORIZONTAL_SINGLE = 0xD2,
    CP437_UP_DOUBLE_AND_RIGHT_SINGLE = 0xD3,
    CP437_UP_SINGLE_AND_RIGHT_DOUBLE = 0xD4,
    CP437_DOWN_SINGLE_AND_RIGHT_DOUBLE = 0xD5,
    CP437_DOWN_DOUBLE_AND_RIGHT_SINGLE = 0xD6,
    CP437_VERTICAL_DOUBLE_AND_HORIZONTAL_SINGLE = 0xD7,
    CP437_VERTICAL_SINGLE_AND_HORIZONTAL_DOUBLE = 0xD8,
    CP437_LIGHT_UP_AND_LEFT = 0xD9,
    CP437_LIGHT_DOWN_AND_RIGHT = 0xDA,
} CP437LineChar;

// Topology Masks (U=1, D=2, L=4, R=8)
#define CP437_MASK_U 1
#define CP437_MASK_D 2
#define CP437_MASK_L 4
#define CP437_MASK_R 8

/**
 * CP437_LineDrawing Lookup Table
 * ==============================
 * This 3D array provides a comprehensive mapping from connectivity and style
 * to the corresponding CP437 box drawing character.
 *
 * Usage:
 *   unsigned char c = CP437_LineDrawing[mask][v_style][h_style];
 *
 * Dimensions:
 *   [16]: Connectivity Mask (0-15). Bitwise OR of:
 *         - CP437_MASK_U (1): Connects Up
 *         - CP437_MASK_D (2): Connects Down
 *         - CP437_MASK_L (4): Connects Left
 *         - CP437_MASK_R (8): Connects Right
 *
 *   [2]:  Vertical Line Style
 *         - 0: Single (Light) Line
 *         - 1: Double Line
 *
 *   [2]:  Horizontal Line Style
 *         - 0: Single (Light) Line
 *         - 1: Double Line
 *
 * Notes:
 *   - Invalid combinations (e.g. mask=0) return 0x00.
 *   - Mixed styles (e.g. Single Vertical, Double Horizontal) are fully supported.
 */
static const unsigned char CP437_LineDrawing[16][2][2] = {
    // Mask 0 (U=False, D=False, L=False, R=False)
    {
        {0x00, 0x00},
        {0x00, 0x00},
    },
    // Mask 1 (U=True, D=False, L=False, R=False)
    {
        {0x00, 0x00},
        {0x00, 0x00},
    },
    // Mask 2 (U=False, D=True, L=False, R=False)
    {
        {0x00, 0x00},
        {0x00, 0x00},
    },
    // Mask 3 (U=True, D=True, L=False, R=False)
    {
        {0xB3, 0xB3},
        {0xBA, 0xBA},
    },
    // Mask 4 (U=False, D=False, L=True, R=False)
    {
        {0x00, 0x00},
        {0x00, 0x00},
    },
    // Mask 5 (U=True, D=False, L=True, R=False)
    {
        {0xD9, 0xBE},
        {0xBD, 0xBC},
    },
    // Mask 6 (U=False, D=True, L=True, R=False)
    {
        {0xBF, 0xB8},
        {0xB7, 0xBB},
    },
    // Mask 7 (U=True, D=True, L=True, R=False)
    {
        {0xB4, 0xB5},
        {0xB6, 0xB9},
    },
    // Mask 8 (U=False, D=False, L=False, R=True)
    {
        {0x00, 0x00},
        {0x00, 0x00},
    },
    // Mask 9 (U=True, D=False, L=False, R=True)
    {
        {0xC0, 0xD4},
        {0xD3, 0xC8},
    },
    // Mask 10 (U=False, D=True, L=False, R=True)
    {
        {0xDA, 0xD5},
        {0xD6, 0xC9},
    },
    // Mask 11 (U=True, D=True, L=False, R=True)
    {
        {0xC3, 0xC6},
        {0xC7, 0xCC},
    },
    // Mask 12 (U=False, D=False, L=True, R=True)
    {
        {0xC4, 0xCD},
        {0xC4, 0xCD},
    },
    // Mask 13 (U=True, D=False, L=True, R=True)
    {
        {0xC1, 0xCF},
        {0xD0, 0xCA},
    },
    // Mask 14 (U=False, D=True, L=True, R=True)
    {
        {0xC2, 0xD1},
        {0xD2, 0xCB},
    },
    // Mask 15 (U=True, D=True, L=True, R=True)
    {
        {0xC5, 0xD8},
        {0xD7, 0xCE},
    },
};

/**
 * Helper function to retrieve a CP437 box drawing character.
 *
 * @param up    True if connected Up
 * @param down  True if connected Down
 * @param left  True if connected Left
 * @param right True if connected Right
 * @param v_double True for Double Vertical lines, False for Single
 * @param h_double True for Double Horizontal lines, False for Single
 * @return The CP437 character code, or 0x00 if no valid character exists.
 */
static inline unsigned char KTerm_GetBoxDrawingChar(int up, int down, int left, int right, int v_double, int h_double) {
    int mask = (up ? CP437_MASK_U : 0) |
               (down ? CP437_MASK_D : 0) |
               (left ? CP437_MASK_L : 0) |
               (right ? CP437_MASK_R : 0);
    int v = v_double ? 1 : 0;
    int h = h_double ? 1 : 0;
    return CP437_LineDrawing[mask][v][h];
}

// Line Theme Structure
typedef struct {
    unsigned char v_line;
    unsigned char h_line;
    unsigned char corner_tl;
    unsigned char corner_tr;
    unsigned char corner_bl;
    unsigned char corner_br;
    unsigned char tee_left;
    unsigned char tee_right;
    unsigned char tee_top;
    unsigned char tee_bottom;
    unsigned char cross;

    // Mixed Style T-Junctions & Crosses
    unsigned char tee_left_v_single_h_double;
    unsigned char tee_left_v_double_h_single;
    unsigned char tee_right_v_single_h_double;
    unsigned char tee_right_v_double_h_single;
    unsigned char tee_top_v_single_h_double;
    unsigned char tee_top_v_double_h_single;
    unsigned char tee_bottom_v_single_h_double;
    unsigned char tee_bottom_v_double_h_single;
    unsigned char cross_v_single_h_double;
    unsigned char cross_v_double_h_single;
} CP437LineTheme;

// Predefined Regular Theme
static const CP437LineTheme CP437_THEME_REGULAR = {
    .v_line = CP437_LIGHT_VERTICAL,
    .h_line = CP437_LIGHT_HORIZONTAL,
    .corner_tl = CP437_LIGHT_DOWN_AND_RIGHT,
    .corner_tr = CP437_LIGHT_DOWN_AND_LEFT,
    .corner_bl = CP437_LIGHT_UP_AND_RIGHT,
    .corner_br = CP437_LIGHT_UP_AND_LEFT,
    .tee_left = CP437_LIGHT_VERTICAL_AND_RIGHT,
    .tee_right = CP437_LIGHT_VERTICAL_AND_LEFT,
    .tee_top = CP437_LIGHT_DOWN_AND_HORIZONTAL,
    .tee_bottom = CP437_LIGHT_UP_AND_HORIZONTAL,
    .cross = CP437_LIGHT_VERTICAL_AND_HORIZONTAL,

    // Mixed
    .tee_left_v_single_h_double = CP437_VERTICAL_SINGLE_AND_RIGHT_DOUBLE,
    .tee_left_v_double_h_single = CP437_VERTICAL_DOUBLE_AND_RIGHT_SINGLE,
    .tee_right_v_single_h_double = CP437_VERTICAL_SINGLE_AND_LEFT_DOUBLE,
    .tee_right_v_double_h_single = CP437_VERTICAL_DOUBLE_AND_LEFT_SINGLE,
    .tee_top_v_single_h_double = CP437_DOWN_SINGLE_AND_HORIZONTAL_DOUBLE,
    .tee_top_v_double_h_single = CP437_DOWN_DOUBLE_AND_HORIZONTAL_SINGLE,
    .tee_bottom_v_single_h_double = CP437_UP_SINGLE_AND_HORIZONTAL_DOUBLE,
    .tee_bottom_v_double_h_single = CP437_UP_DOUBLE_AND_HORIZONTAL_SINGLE,
    .cross_v_single_h_double = CP437_VERTICAL_SINGLE_AND_HORIZONTAL_DOUBLE,
    .cross_v_double_h_single = CP437_VERTICAL_DOUBLE_AND_HORIZONTAL_SINGLE
};

// Predefined Bold Theme
static const CP437LineTheme CP437_THEME_BOLD = {
    .v_line = CP437_DOUBLE_VERTICAL,
    .h_line = CP437_DOUBLE_HORIZONTAL,
    .corner_tl = CP437_DOUBLE_DOWN_AND_RIGHT,
    .corner_tr = CP437_DOUBLE_DOWN_AND_LEFT,
    .corner_bl = CP437_DOUBLE_UP_AND_RIGHT,
    .corner_br = CP437_DOUBLE_UP_AND_LEFT,
    .tee_left = CP437_DOUBLE_VERTICAL_AND_RIGHT,
    .tee_right = CP437_DOUBLE_VERTICAL_AND_LEFT,
    .tee_top = CP437_DOUBLE_DOWN_AND_HORIZONTAL,
    .tee_bottom = CP437_DOUBLE_UP_AND_HORIZONTAL,
    .cross = CP437_DOUBLE_VERTICAL_AND_HORIZONTAL,

    // Mixed
    .tee_left_v_single_h_double = CP437_VERTICAL_SINGLE_AND_RIGHT_DOUBLE,
    .tee_left_v_double_h_single = CP437_VERTICAL_DOUBLE_AND_RIGHT_SINGLE,
    .tee_right_v_single_h_double = CP437_VERTICAL_SINGLE_AND_LEFT_DOUBLE,
    .tee_right_v_double_h_single = CP437_VERTICAL_DOUBLE_AND_LEFT_SINGLE,
    .tee_top_v_single_h_double = CP437_DOWN_SINGLE_AND_HORIZONTAL_DOUBLE,
    .tee_top_v_double_h_single = CP437_DOWN_DOUBLE_AND_HORIZONTAL_SINGLE,
    .tee_bottom_v_single_h_double = CP437_UP_SINGLE_AND_HORIZONTAL_DOUBLE,
    .tee_bottom_v_double_h_single = CP437_UP_DOUBLE_AND_HORIZONTAL_SINGLE,
    .cross_v_single_h_double = CP437_VERTICAL_SINGLE_AND_HORIZONTAL_DOUBLE,
    .cross_v_double_h_single = CP437_VERTICAL_DOUBLE_AND_HORIZONTAL_SINGLE
};

#ifdef __cplusplus
}
#endif

#endif // KT_CP437_TABLES_H
