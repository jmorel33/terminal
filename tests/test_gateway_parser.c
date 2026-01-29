#define KTERM_TESTING
// Minimal includes to test primitives
#include "../kt_parser.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_read_identifier() {
    const char* input = "  MyIdentifier123  Next";
    StreamScanner s = { .ptr = input, .len = strlen(input), .pos = 0 };
    char buf[64];

    assert(Stream_ReadIdentifier(&s, buf, sizeof(buf)) == true);
    assert(strcmp(buf, "MyIdentifier123") == 0);

    assert(Stream_ReadIdentifier(&s, buf, sizeof(buf)) == true);
    assert(strcmp(buf, "Next") == 0);

    assert(Stream_ReadIdentifier(&s, buf, sizeof(buf)) == false);
    printf("PASS: Read Identifier\n");
}

void test_read_bool() {
    const char* input = "  ON off TRUE false 1 0 invalid";
    StreamScanner s = { .ptr = input, .len = strlen(input), .pos = 0 };
    bool val;

    assert(Stream_ReadBool(&s, &val) == true); assert(val == true);
    assert(Stream_ReadBool(&s, &val) == true); assert(val == false);
    assert(Stream_ReadBool(&s, &val) == true); assert(val == true);
    assert(Stream_ReadBool(&s, &val) == true); assert(val == false);
    assert(Stream_ReadBool(&s, &val) == true); assert(val == true);
    assert(Stream_ReadBool(&s, &val) == true); assert(val == false);

    size_t pos_before = s.pos;
    assert(Stream_ReadBool(&s, &val) == false);
    // Stream_ReadBool skips whitespace even on failure, like ReadInt/ReadFloat
    // So pos should have advanced by 1 (the space)
    assert(s.pos > pos_before);
    assert(s.ptr[s.pos] == 'i'); // Should be at 'invalid'
    printf("PASS: Read Bool\n");
}

void test_match_token() {
    const char* input = "  SET PIPE  ";
    StreamScanner s = { .ptr = input, .len = strlen(input), .pos = 0 };

    assert(Stream_MatchToken(&s, "SET") == true);
    assert(Stream_MatchToken(&s, "PIPE") == true);

    s.pos = 0;
    assert(Stream_MatchToken(&s, "PIPE") == false); // Expect SET

    // Case insensitive
    s.pos = 0;
    assert(Stream_MatchToken(&s, "set") == true);
    printf("PASS: Match Token\n");
}

void test_peek_identifier() {
    const char* input = "  PeekMe";
    StreamScanner s = { .ptr = input, .len = strlen(input), .pos = 0 };
    char buf[64];

    assert(Stream_PeekIdentifier(&s, buf, sizeof(buf)) == true);
    assert(strcmp(buf, "PeekMe") == 0);
    // Should verify pos didn't move
    assert(s.pos == 0); // 0 because we skip whitespace inside PeekIdentifier but we restore original pos?
    // Wait, Stream_PeekIdentifier:
    // size_t original_pos = scanner->pos;
    // bool result = Stream_ReadIdentifier(scanner, buffer, max_len);
    // scanner->pos = original_pos;
    // So pos is restored to 0 (including whitespace).

    // Now read it
    assert(Stream_ReadIdentifier(&s, buf, sizeof(buf)) == true);
    assert(strcmp(buf, "PeekMe") == 0);
    // Stream_ReadIdentifier skips whitespace (2 chars) + "PeekMe" (6 chars) = 8
    assert(s.pos == 8);
    printf("PASS: Peek Identifier\n");
}

int main() {
    test_read_identifier();
    test_read_bool();
    test_match_token();
    test_peek_identifier();

    printf("All parser primitive tests passed.\n");
    return 0;
}
