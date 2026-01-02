#include <stdio.h>
#include <assert.h>
#include <string.h>

#define TERMINAL_TESTING
#define TERMINAL_IMPLEMENTATION
#include "terminal.h"

// Mock main for standalone testing
int main() {
    printf("Initializing Terminal...\n");
    InitTerminal();

    printf("Setting up Session...\n");
    // Ensure we are in a mode that supports Sixel (e.g., VT340 or xterm)
    SetVTLevel(VT_LEVEL_340);
    assert(ACTIVE_SESSION.conformance.features.sixel_graphics);

    printf("Testing Sixel Data Processing...\n");
    // Simple Sixel string: DCS q "1;1;10;10 #0!10? ST"
    // DCS q = Start Sixel
    // "1;1;10;10 = Raster Attributes (Pan/Pad/Ph/Pv) - handled but mostly ignored currently
    // #0 = Select Color 0
    // !10 = Repeat 10 times
    // ? = Pattern 63 (all bits set)
    // ST = String Terminator (ESC \)

    // We inject this via ProcessSixelData directly or via pipeline
    // Let's use ProcessSixelData as it is the core logic.
    // Wait, ProcessSixelData is called by ExecuteDCSCommand -> but ProcessDCSChar accumulates it?
    // Let's emulate the parser feeding data.

    // Simulate: ESC P q (DCS Sixel)
    ProcessChar('\x1B');
    ProcessChar('P');
    ProcessChar('q');

    // At this point we are in PARSE_SIXEL state?
    // Let's check logic in ProcessDCSChar.
    // It says: if (ch == 'q') ... ACTIVE_SESSION.parse_state = PARSE_SIXEL;

    assert(ACTIVE_SESSION.parse_state == PARSE_SIXEL);
    printf("State is PARSE_SIXEL.\n");

    const char* sixel_payload = "\"1;1;10;10#0!10?";
    for (int i = 0; sixel_payload[i]; i++) {
        ProcessChar(sixel_payload[i]);
    }

    // Check if strips are generated
    // !10? means repeat '?' (63) 10 times.
    // So we should have 10 strips.
    // Wait, logic says: for (int r = 0; r < repeat; r++) ... add strip.

    printf("Strip count: %zu\n", ACTIVE_SESSION.sixel.strip_count);
    assert(ACTIVE_SESSION.sixel.strip_count == 10);
    assert(ACTIVE_SESSION.sixel.strips[0].pattern == ('?' - '?')); // '?' is 63. Wait, sixel_val = ch - '?'. ? - ? = 0.
    // '?' is 63 in ASCII.
    // Logic: int sixel_val = ch - '?';
    // If ch is '?', val is 0.
    // Let's use '~' (126). 126 - 63 = 63.

    // Retry with '~' for all bits set
    // Reset
    InitSixelGraphics();
    ACTIVE_SESSION.parse_state = PARSE_SIXEL;

    // Send !5~
    ProcessChar('!');
    ProcessChar('5');
    ProcessChar('~');

    printf("Strip count after reset: %zu\n", ACTIVE_SESSION.sixel.strip_count);
    assert(ACTIVE_SESSION.sixel.strip_count == 5);
    assert(ACTIVE_SESSION.sixel.strips[0].pattern == ('~' - '?')); // 63
    assert(ACTIVE_SESSION.sixel.strips[0].color_index == 0);

    // Test Color Change
    // #1 (Select Color 1)
    ProcessChar('#');
    ProcessChar('1');
    // Then a char
    ProcessChar('~'); // 1 strip

    assert(ACTIVE_SESSION.sixel.strip_count == 6);
    assert(ACTIVE_SESSION.sixel.strips[5].color_index == 1);

    // Terminate
    ProcessChar('\x1B'); // ESC
    assert(ACTIVE_SESSION.parse_state == PARSE_SIXEL_ST);
    ProcessChar('\\'); // ST
    assert(ACTIVE_SESSION.parse_state == VT_PARSE_NORMAL);

    // Verify Dirty Flag
    assert(ACTIVE_SESSION.sixel.dirty == true);

    printf("Sixel Parsing Verified.\n");

    // Now verify DrawSixelGraphics (upload)
    // DrawTerminal calls UpdateTerminalSSBO and Sixel upload logic
    // We mock SituationUpdateBuffer to check calls?
    // mock_situation.h stub does nothing, but we can assume if code compiles and logic runs, it calls it.
    // We can add a print in mock_situation if we really want, but for now logic verification is key.

    DrawTerminal();

    // After Draw, dirty should be false
    assert(ACTIVE_SESSION.sixel.dirty == false);

    printf("Sixel Draw Logic Verified.\n");

    CleanupTerminal();
    printf("Test Passed.\n");
    return 0;
}
