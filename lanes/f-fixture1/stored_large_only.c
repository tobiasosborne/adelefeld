/* Time and isolate exactly the new test, without changing its assertions. */
#define ADF_TEST_NO_MAIN
#include "../../tests/test_lfunc.c"
int main(void)
{
    adf_test_current = "stored_large_before_optimisation";
    adf_test_fn_stored_large_before_optimisation();
    printf("stored_large checks=%lu failures=%lu\n", adf_test_checks, adf_test_failures);
    flint_cleanup();
    return adf_test_failures ? 1 : 0;
}
