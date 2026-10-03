/* refs/src/flint-3.0.1/memory.rst:29-44: free FLINT's caches once before exit.
   All original tests, assertions and CPU guards run unchanged before this cleanup. */
#include <flint/flint.h>

int repair6_tests_main(void);

int main(void)
{
    int status = repair6_tests_main();
    flint_cleanup_master();
    return status;
}
