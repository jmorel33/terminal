#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_unescape_utility() {
    char dest[256];

    // Basic
    KTerm_UnescapeString(dest, "abc", 3);
    assert(strcmp(dest, "abc") == 0);

    // Escaped Quote
    KTerm_UnescapeString(dest, "a\\\"b", 4);
    assert(strcmp(dest, "a\"b") == 0);

    // Escaped Backslash
    KTerm_UnescapeString(dest, "a\\\\b", 4);
    assert(strcmp(dest, "a\\b") == 0);

    // Escaped Newline
    KTerm_UnescapeString(dest, "a\\nb", 4);
    assert(strcmp(dest, "a\nb") == 0);

    printf("PASS: Unescape Utility\n");
}

void test_lexer_escapes() {
    KTermLexer lexer;
    KTermToken token;

    // Test 1: Escaped Quote
    KTerm_LexerInit(&lexer, "\"a\\\"b\"");
    token = KTerm_LexerNext(&lexer);
    assert(token.type == KT_TOK_STRING);
    // Raw content should include the escape
    assert(strncmp(token.start, "a\\\"b", token.length) == 0);
    assert(token.length == 4);

    // Test 2: Unterminated
    KTerm_LexerInit(&lexer, "\"abc");
    token = KTerm_LexerNext(&lexer);
    assert(token.type == KT_TOK_ERROR); // Expect ERROR for unterminated

    // Test 3: Backslash at end (effectively unterminated or invalid escape)
    KTerm_LexerInit(&lexer, "\"abc\\");
    token = KTerm_LexerNext(&lexer);
    assert(token.type == KT_TOK_ERROR);

    printf("PASS: Lexer Escapes\n");
}

int main() {
    test_unescape_utility();
    test_lexer_escapes();
    return 0;
}
