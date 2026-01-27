#define KTERM_IMPLEMENTATION
#define KTERM_IO_SIT_IMPLEMENTATION
#define KTERM_TESTING
#include "../kt_io_sit.h"

int main() {
    KTermConfig config = {0};
    KTerm* term = KTerm_Create(config);
    if (!term) return 1;

    KTermSit_ProcessInput(term);

    KTerm_Destroy(term);
    printf("IO Adapter Test Passed (Compilation & Basic Linkage)\n");
    return 0;
}
