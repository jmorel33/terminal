#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../tests/mock_situation.h"
#include "../kterm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Helper: Check cell content
static void check_cell(KTerm* term, int y, int x, char expected_char, const char* message) {
    KTermSession* session = GET_SESSION(term);
    EnhancedTermChar* cell = GetScreenCell(session, y, x);
    if (!cell) {
        printf("FAIL: %s - Cell (%d,%d) is invalid (NULL)\n", message, y, x);
        exit(1);
    }
    if (cell->ch != expected_char) {
        printf("FAIL: %s - Cell (%d,%d) expected '%c' (0x%02X), got '%c' (0x%02X)\n",
               message, y, x, expected_char, expected_char, (char)cell->ch, cell->ch);
        exit(1);
    }
}

// Helper: Set cell content and protection
static void set_cell_protected(KTerm* term, int y, int x, char ch, bool protected) {
    EnhancedTermChar* cell = GetScreenCell(GET_SESSION(term), y, x);
    if (cell) {
        cell->ch = ch;
        if (protected) {
            cell->flags |= KTERM_ATTR_PROTECTED;
        } else {
            cell->flags &= ~KTERM_ATTR_PROTECTED;
        }
    }
}

static void reset_term(KTerm* term) {
    KTerm_WriteString(term, "\x1B[2J\x1B[H"); // Clear and Home
    KTerm_ProcessEvents(term);
    // Reset protection attributes on all cells just in case
    KTermSession* session = GET_SESSION(term);
    for(int i=0; i<session->rows*session->cols; i++) {
        session->screen_buffer[i].flags &= ~KTERM_ATTR_PROTECTED;
    }
    // Reset current attributes
    session->current_attributes = 0;
}

void test_ich_protection(KTerm* term) {
    printf("Testing ICH (Insert Character) Protection...\n");
    reset_term(term);

    // Setup: "ABCDE" with 'C' protected
    KTerm_WriteString(term, "ABCDE");
    KTerm_ProcessEvents(term);
    set_cell_protected(term, 0, 2, 'C', true); // Protect 'C' at index 2

    // Move to start and Insert 1 Char
    KTerm_WriteString(term, "\x1B[G\x1B[@"); // CHA 1, ICH 1
    KTerm_ProcessEvents(term);

    // Expectation: Operation ignored because 'C' is protected in the line
    // If ignored: "ABCDE"
    // If failed: " ABCDE" (shifted)
    check_cell(term, 0, 0, 'A', "ICH: Index 0 should be 'A'");
    check_cell(term, 0, 1, 'B', "ICH: Index 1 should be 'B'");
    check_cell(term, 0, 2, 'C', "ICH: Index 2 should be 'C'");
    printf("PASS: ICH ignored protected line.\n");
}

void test_dch_protection(KTerm* term) {
    printf("Testing DCH (Delete Character) Protection...\n");
    reset_term(term);

    // Setup: "ABCDE" with 'C' protected
    KTerm_WriteString(term, "ABCDE");
    KTerm_ProcessEvents(term);
    set_cell_protected(term, 0, 2, 'C', true); // Protect 'C'

    // Move to start and Delete 1 Char
    KTerm_WriteString(term, "\x1B[G\x1B[P"); // CHA 1, DCH 1
    KTerm_ProcessEvents(term);

    // Expectation: Operation ignored
    check_cell(term, 0, 0, 'A', "DCH: Index 0 should be 'A'");
    check_cell(term, 0, 2, 'C', "DCH: Index 2 should be 'C'");
    printf("PASS: DCH ignored protected line.\n");
}

void test_il_protection(KTerm* term) {
    printf("Testing IL (Insert Line) Protection...\n");
    reset_term(term);

    // Setup:
    // Row 0: "Line 1"
    // Row 1: "Line 2" (Protected char here)
    // Row 2: "Line 3"
    KTerm_WriteString(term, "Line 1\r\nLine 2\r\nLine 3");
    KTerm_ProcessEvents(term);
    set_cell_protected(term, 1, 0, 'L', true); // Protect 'L' on Row 1

    // Move to Row 0 and Insert Line
    KTerm_WriteString(term, "\x1B[H\x1B[L"); // CUP 1,1; IL
    KTerm_ProcessEvents(term);

    // Expectation: Ignored because Row 1 (in scrolling region) has protected char
    check_cell(term, 0, 0, 'L', "IL: Row 0 should be 'Line 1'");
    check_cell(term, 1, 0, 'L', "IL: Row 1 should be 'Line 2'");
    printf("PASS: IL ignored protected scrolling region.\n");
}

void test_dl_protection(KTerm* term) {
    printf("Testing DL (Delete Line) Protection...\n");
    reset_term(term);

    // Setup: same
    KTerm_WriteString(term, "Line 1\r\nLine 2\r\nLine 3");
    KTerm_ProcessEvents(term);
    set_cell_protected(term, 1, 0, 'L', true); // Protect 'L' on Row 1

    // Move to Row 0 and Delete Line
    KTerm_WriteString(term, "\x1B[H\x1B[M"); // CUP 1,1; DL
    KTerm_ProcessEvents(term);

    // Expectation: Ignored
    check_cell(term, 0, 0, 'L', "DL: Row 0 should be 'Line 1'");
    check_cell(term, 1, 0, 'L', "DL: Row 1 should be 'Line 2'");
    printf("PASS: DL ignored protected scrolling region.\n");
}

void test_scroll_protection(KTerm* term) {
    printf("Testing Scroll (SU) Protection...\n");
    reset_term(term);

    // Setup
    KTerm_WriteString(term, "Line 1\r\nLine 2\r\nLine 3");
    KTerm_ProcessEvents(term);
    set_cell_protected(term, 1, 0, 'L', true);

    // Scroll Up
    KTerm_WriteString(term, "\x1B[S");
    KTerm_ProcessEvents(term);

    // Expectation: Ignored
    check_cell(term, 0, 0, 'L', "SU: Row 0 should be 'Line 1'");
    printf("PASS: SU ignored protected scrolling region.\n");
}

void test_replace_protection(KTerm* term) {
    printf("Testing Replace Mode Overwrite Protection...\n");
    reset_term(term);

    // Setup: "A" protected
    KTerm_WriteString(term, "A");
    KTerm_ProcessEvents(term);
    set_cell_protected(term, 0, 0, 'A', true);

    // Try to overwrite with 'B'
    KTerm_WriteString(term, "\x1B[H"); // Home
    KTerm_WriteString(term, "B");
    KTerm_ProcessEvents(term);

    check_cell(term, 0, 0, 'A', "Replace: Protected 'A' should not be overwritten by 'B'");
    printf("PASS: Replace mode respected protected cell.\n");
}

void test_insert_mode_typing_protection(KTerm* term) {
    printf("Testing Insert Mode Typing Protection...\n");
    reset_term(term);

    // Setup: "ABC" with 'C' protected
    KTerm_WriteString(term, "ABC");
    KTerm_ProcessEvents(term);
    set_cell_protected(term, 0, 2, 'C', true);

    // Enable Insert Mode
    KTerm_WriteString(term, "\x1B[4h");

    // Move to start and type 'X'
    KTerm_WriteString(term, "\x1B[H");
    KTerm_WriteString(term, "X");
    KTerm_ProcessEvents(term);

    // Expectation: 'X' is NOT inserted because line has protected char
    // If it was inserted: "XABC" (shifted)
    // If ignored: "ABC"
    // Note: KTerm currently shifts " ABC" if protection ignored? No, if protection respected, it does nothing.
    // But my bug analysis suggested it might overwrite 'A' if shift blocked?
    // "ABC" -> shift blocked -> overwrite 'A' with 'X' -> "XBC"?
    
    check_cell(term, 0, 0, 'A', "InsertTyping: 'A' preserved");
    check_cell(term, 0, 1, 'B', "InsertTyping: 'B' preserved");
    check_cell(term, 0, 2, 'C', "InsertTyping: 'C' preserved");
    printf("PASS: Insert mode typing respected protected line.\n");

    // Disable Insert Mode
    KTerm_WriteString(term, "\x1B[4l");
}

void test_insert_after_protected(KTerm* term) {
    printf("Testing Insert After Protected Cell...\n");
    reset_term(term);

    // Setup: "P U" (P protected at 0, U unprotected at 2)
    KTerm_WriteString(term, "P U");
    KTerm_ProcessEvents(term);
    set_cell_protected(term, 0, 0, 'P', true); // Protect 'P'

    // Move to index 2 (at 'U')
    KTerm_WriteString(term, "\x1B[H\x1B[2C"); // Home, Right 2

    // Insert Character (should shift 'U' right, 'P' stays)
    // Range affected: 2..RightMargin. 'P' at 0 is safe.
    KTerm_WriteString(term, "\x1B[@"); // ICH 1
    KTerm_ProcessEvents(term);

    check_cell(term, 0, 0, 'P', "P should remain at 0");
    check_cell(term, 0, 1, ' ', "Space inserted at 1? No, at 2.");
    // Wait, cursor at 2. Insert at 2. 
    // Old 2 ('U') moves to 3. 
    // New 2 is Space.
    // 0='P', 1=' ', 2=' ', 3='U' ?
    // Initial: "P U" -> 0='P', 1=' ', 2='U'.
    // Insert at 2: 2 becomes Space. Old 2 ('U') moves to 3.
    // Result: "P  U" -> 0='P', 1=' ', 2=' ', 3='U'.
    
    check_cell(term, 0, 2, ' ', "New Space at 2");
    check_cell(term, 0, 3, 'U', "U shifted to 3");
    
    printf("PASS: Insert after protected cell succeeded.\n");
}

int main(void) {
    KTermConfig config = { .width = 80, .height = 24 };
    KTerm* term = KTerm_Create(config);
    if (!term) { printf("Failed to create KTerm\n"); exit(1); }

    test_ich_protection(term);
    test_dch_protection(term);
    test_il_protection(term);
    test_dl_protection(term);
    test_scroll_protection(term);
    test_replace_protection(term);
    test_insert_mode_typing_protection(term);
    test_insert_after_protected(term);

    KTerm_Destroy(term);
    printf("All Protected Cells Tests Passed.\n");
    return 0;
}
