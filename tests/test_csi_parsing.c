#define KTERM_IMPLEMENTATION
#define KTERM_TESTING
#include "../kterm.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_basic_parsing(KTerm* term) {
    int params[MAX_ESCAPE_PARAMS];
    int count = KTerm_ParseCSIParams(term, "10;20", params, MAX_ESCAPE_PARAMS);

    assert(count == 2);
    assert(params[0] == 10);
    assert(params[1] == 20);
    printf("PASS: Basic Parsing\n");
}

void test_defaults(KTerm* term) {
    int params[MAX_ESCAPE_PARAMS];
    // Leading empty
    int count = KTerm_ParseCSIParams(term, ";20", params, MAX_ESCAPE_PARAMS);
    assert(count == 2);
    assert(params[0] == 0);
    assert(params[1] == 20);

    // Trailing empty
    count = KTerm_ParseCSIParams(term, "10;", params, MAX_ESCAPE_PARAMS);
    assert(count == 2);
    assert(params[0] == 10);
    assert(params[1] == 0);

    // Middle empty
    count = KTerm_ParseCSIParams(term, "10;;30", params, MAX_ESCAPE_PARAMS);
    assert(count == 3);
    assert(params[0] == 10);
    assert(params[1] == 0);
    assert(params[2] == 30);

    printf("PASS: Defaults\n");
}

void test_subparams(KTerm* term) {
    int params[MAX_ESCAPE_PARAMS];
    // Flattened subparams
    int count = KTerm_ParseCSIParams(term, "38:2:10:20:30", params, MAX_ESCAPE_PARAMS);
    assert(count == 5);
    assert(params[0] == 38);
    assert(params[1] == 2);
    assert(params[2] == 10);
    assert(params[3] == 20);
    assert(params[4] == 30);

    // Verify separators in session
    KTermSession* session = GET_SESSION(term);
    assert(session->escape_separators[0] == ':');
    assert(session->escape_separators[1] == ':');
    assert(session->escape_separators[2] == ':');
    assert(session->escape_separators[3] == ':');
    assert(session->escape_separators[4] == 0); // Last has no separator following it usually, or handled differently?
    // Current implementation:
    // if (sep == ';' || sep == ':') { escape_separators[i] = sep; consume; } else { escape_separators[i] = 0; }
    // So 38: -> sep=':' -> stored.
    // 30 -> sep=0 -> stored.

    printf("PASS: Subparams\n");
}

void test_garbage(KTerm* term) {
    int params[MAX_ESCAPE_PARAMS];
    // "10;foo;20" -> "10;0;20" ?
    // Stream_ReadInt checks isdigit. If not digit, returns false.
    // If Stream_ReadInt returns false, we set 0.
    // But we need to skip the garbage?
    // Stream_ReadInt does NOT consume garbage if it fails?
    // "foo" -> 'f' is not digit. Peek is 'f'. ReadInt returns false.
    // Value set to 0.
    // Next char is 'f'.
    // Next check: if (sep == ';' || sep == ':'). 'f' is not.
    // Else break.
    // So parsing STOPS at garbage?

    // "10;foo;20"
    // 1. Read 10. Separator ';'. Consume ';'. Count=1.
    // 2. ReadInt "foo". 'f' not digit. Returns false. Value=0. Params[1]=0.
    // 3. Improvement: Consume "foo".
    // 4. Peek ';'. Consume ';'.
    // 5. ReadInt 20. Count=3.

    int count = KTerm_ParseCSIParams(term, "10;foo;20", params, MAX_ESCAPE_PARAMS);
    assert(count == 3);
    assert(params[0] == 10);
    assert(params[1] == 0);
    assert(params[2] == 20);

    printf("PASS: Garbage Handling\n");
}

void test_overflow(KTerm* term) {
    // Generate long string
    char buf[1024] = "1";
    for(int i=0; i<50; i++) {
        strcat(buf, ";1");
    }
    int params[MAX_ESCAPE_PARAMS];
    int count = KTerm_ParseCSIParams(term, buf, params, MAX_ESCAPE_PARAMS);
    assert(count == MAX_ESCAPE_PARAMS);
    printf("PASS: Overflow Protection\n");
}

int main() {
    KTermConfig config = {0};
    config.width = 80; config.height = 25;
    KTerm* term = KTerm_Create(config);

    test_basic_parsing(term);
    test_defaults(term);
    test_subparams(term);
    test_garbage(term);
    test_overflow(term);

    KTerm_Destroy(term);
    return 0;
}
