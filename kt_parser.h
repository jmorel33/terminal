#ifndef KT_PARSER_H
#define KT_PARSER_H

#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    KT_TOK_EOF = 0,
    KT_TOK_ERROR,
    KT_TOK_IDENTIFIER,  // e.g., CREATE, TYPE, X, Y
    KT_TOK_STRING,      // e.g., "Hello World"
    KT_TOK_NUMBER,      // e.g., 10, -5.5, 0xFF
    KT_TOK_EQUALS,      // =
    KT_TOK_SEMICOLON,   // ;
    KT_TOK_COMMA,       // ,
    KT_TOK_UNKNOWN      // ?
} KTermTokenType;

typedef struct {
    KTermTokenType type;
    const char* start;  // Pointer into the source string (zero-copy)
    int length;
    union {
        int i;
        float f;
    } value;            // Pre-parsed numeric value
} KTermToken;

typedef struct {
    const char* src;
    size_t pos;
} KTermLexer;

// Initialize the lexer
static inline void KTerm_LexerInit(KTermLexer* lexer, const char* input) {
    lexer->src = input;
    lexer->pos = 0;
}

// Get the next token
static inline KTermToken KTerm_LexerNext(KTermLexer* lexer) {
    KTermToken token = {0};
    const char* p = lexer->src + lexer->pos;

    // 1. Skip Whitespace
    while (*p && isspace((unsigned char)*p)) p++;

    token.start = p;

    // 2. Check End of String
    if (*p == '\0') {
        token.type = KT_TOK_EOF;
        lexer->pos = p - lexer->src;
        return token;
    }

    // 3. Operators
    if (*p == '=') { token.type = KT_TOK_EQUALS;    token.length = 1; p++; }
    else if (*p == ';') { token.type = KT_TOK_SEMICOLON; token.length = 1; p++; }
    else if (*p == ',') { token.type = KT_TOK_COMMA;     token.length = 1; p++; }

    // 4. Strings ("..." or '...')
    else if (*p == '"' || *p == '\'') {
        char quote = *p++; // Remember quote type and skip it
        token.start = p;   // Content starts after quote
        token.type = KT_TOK_STRING;
        while (*p && *p != quote) {
            // Handle escape? if (*p == '\\' && *(p+1)) p++;
            p++;
        }
        token.length = (int)(p - token.start);
        if (*p == quote) p++; // Skip closing quote
    }

    // 5. Numbers (Integer, Float, Hex)
    else if (isdigit((unsigned char)*p) || (*p == '-' && isdigit((unsigned char)*(p+1)))) {
        token.type = KT_TOK_NUMBER;
        char* endptr;
        // Check for Hex
        if (*p == '0' && (*(p+1) == 'x' || *(p+1) == 'X')) {
            token.value.i = (int)strtol(p, &endptr, 16);
            token.value.f = (float)token.value.i;
        } else {
            token.value.f = strtof(p, &endptr);
            token.value.i = (int)token.value.f;
        }
        token.length = (int)(endptr - p);
        p = endptr;
    }

    // 6. Identifiers (Alpha + _ + # for colors)
    else if (isalpha((unsigned char)*p) || *p == '_' || *p == '#') {
        token.type = KT_TOK_IDENTIFIER;
        while (isalnum((unsigned char)*p) || *p == '_' || *p == '#') p++;
        token.length = (int)(p - token.start);
    }

    // 7. Unknown
    else {
        token.type = KT_TOK_UNKNOWN;
        token.length = 1;
        p++;
    }

    lexer->pos = p - lexer->src;
    return token;
}

// Helper to check if identifier matches string
static inline bool KTerm_TokenIs(KTermToken t, const char* str) {
    if (t.type != KT_TOK_IDENTIFIER) return false;
    if (strlen(str) != (size_t)t.length) return false;
    return strncmp(t.start, str, t.length) == 0;
}

// =============================================================================
// STREAM SCANNER (Centralized)
// =============================================================================

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

static inline void Stream_SkipWhitespace(StreamScanner* scanner) {
    while (scanner->pos < scanner->len && isspace((unsigned char)Stream_Peek(scanner))) {
        Stream_Consume(scanner);
    }
}

static inline bool Stream_Expect(StreamScanner* scanner, char expected) {
    Stream_SkipWhitespace(scanner);
    if (Stream_Peek(scanner) == expected) {
        Stream_Consume(scanner);
        return true;
    }
    return false;
}

static inline bool Stream_ReadInt(StreamScanner* scanner, int* out_val) {
    Stream_SkipWhitespace(scanner);
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

static inline bool Stream_ReadHex(StreamScanner* scanner, unsigned int* out_val) {
    Stream_SkipWhitespace(scanner);
    if (scanner->pos >= scanner->len) return false;

    // Optional 0x prefix
    if (scanner->pos + 1 < scanner->len && Stream_Peek(scanner) == '0' &&
        (scanner->ptr[scanner->pos+1] == 'x' || scanner->ptr[scanner->pos+1] == 'X')) {
        Stream_Consume(scanner);
        Stream_Consume(scanner);
    }

    if (!isxdigit((unsigned char)Stream_Peek(scanner))) return false;

    unsigned int val = 0;
    while (scanner->pos < scanner->len && isxdigit((unsigned char)Stream_Peek(scanner))) {
        char c = Stream_Consume(scanner);
        int d = 0;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        val = (val << 4) | d;
    }
    *out_val = val;
    return true;
}

static inline bool Stream_ReadFloat(StreamScanner* scanner, float* out_val) {
    Stream_SkipWhitespace(scanner);
    if (scanner->pos >= scanner->len) return false;
    char* endptr;
    *out_val = strtof(scanner->ptr + scanner->pos, &endptr);
    if (endptr == scanner->ptr + scanner->pos) return false;
    scanner->pos = endptr - scanner->ptr;
    return true;
}

#endif // KT_PARSER_H
