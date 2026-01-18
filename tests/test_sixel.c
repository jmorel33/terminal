#include <stdio.h>
#include <assert.h>
#include <string.h>

#define TERMINAL_TESTING
#define TERMINAL_IMPLEMENTATION
#include "terminal.h"

static Terminal* term = NULL;

// Mock main for standalone testing
int main() {
    printf("Initializing Terminal...\n");

    TerminalConfig config = {0};
    term = Terminal_Create(config);


    printf("Setting up Session...\n");
    // Ensure we are in a mode that supports Sixel (e.g., VT340 or xterm)
    SetVTLevel(term, VT_LEVEL_340);
    assert(GET_SESSION(term)->conformance.features.sixel_graphics);

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
    ProcessChar(term, '\x1B');
    ProcessChar(term, 'P');
    ProcessChar(term, 'q');

    // At this point we are in PARSE_SIXEL state?
    // Let's check logic in ProcessDCSChar.
    // It says: if (ch == 'q') ... GET_SESSION(term)->parse_state = PARSE_SIXEL;

    assert(GET_SESSION(term)->parse_state == PARSE_SIXEL);
    printf("State is PARSE_SIXEL.\n");

    const char* sixel_payload = "\"1;1;10;10#0!10?";
    for (int i = 0; sixel_payload[i]; i++) {
        ProcessChar(term, sixel_payload[i]);
    }

    // Check if strips are generated
    // !10? means repeat '?' (63) 10 times.
    // So we should have 10 strips.
    // Wait, logic says: for (int r = 0; r < repeat; r++) ... add strip.

    printf("Strip count: %zu\n", GET_SESSION(term)->sixel.strip_count);
    assert(GET_SESSION(term)->sixel.strip_count == 10);
    assert(GET_SESSION(term)->sixel.strips[0].pattern == ('?' - '?')); // '?' is 63. Wait, sixel_val = ch - '?'. ? - ? = 0.
    // '?' is 63 in ASCII.
    // Logic: int sixel_val = ch - '?';
    // If ch is '?', val is 0.
    // Let's use '~' (126). 126 - 63 = 63.

    // Retry with '~' for all bits set
    // Reset
    InitSixelGraphics(term);

    // Simulate Sixel sequence start to ensure allocation happens
    ProcessChar(term, '\x1B');
    ProcessChar(term, 'P');
    ProcessChar(term, 'q');

    // GET_SESSION(term)->parse_state = PARSE_SIXEL; // Handled by ProcessChar(term, 'q')

    // Send !5~
    ProcessChar(term, '!');
    ProcessChar(term, '5');
    ProcessChar(term, '~');

    printf("Strip count after reset: %zu\n", GET_SESSION(term)->sixel.strip_count);
    assert(GET_SESSION(term)->sixel.strip_count == 5);
    assert(GET_SESSION(term)->sixel.strips[0].pattern == ('~' - '?')); // 63
    assert(GET_SESSION(term)->sixel.strips[0].color_index == 0);

    // Test Color Change
    // #1 (Select Color 1)
    ProcessChar(term, '#');
    ProcessChar(term, '1');
    // Then a char
    ProcessChar(term, '~'); // 1 strip

    assert(GET_SESSION(term)->sixel.strip_count == 6);
    assert(GET_SESSION(term)->sixel.strips[5].color_index == 1);

    // Terminate
    ProcessChar(term, '\x1B'); // ESC
    assert(GET_SESSION(term)->parse_state == PARSE_SIXEL_ST);
    ProcessChar(term, '\\'); // ST
    assert(GET_SESSION(term)->parse_state == VT_PARSE_NORMAL);

    // Verify Dirty Flag
    assert(GET_SESSION(term)->sixel.dirty == true);

    printf("Sixel Parsing Verified.\n");

    // Now verify DrawSixelGraphics (upload)
    // DrawTerminal calls UpdateTerminalSSBO and Sixel upload logic
    // We mock SituationUpdateBuffer to check calls?
    // mock_situation.h stub does nothing, but we can assume if code compiles and logic runs, it calls it.
    // We can add a print in mock_situation if we really want, but for now logic verification is key.

    DrawTerminal(term);

    // After Draw, dirty should be false
    assert(GET_SESSION(term)->sixel.dirty == false);

    printf("Sixel Draw Logic Verified.\n");

    CleanupTerminal(term);
    printf("Test Passed.\n");
    return 0;
}
