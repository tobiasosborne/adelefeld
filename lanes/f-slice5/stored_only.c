#define ADF_TEST_NO_MAIN
#include "../../tests/test_lfunc.c"
int main(void) {
    adf_test_current = "stored_before_optimisation";
    adf_test_fn_stored_before_optimisation();
    printf("stored checks=%lu failures=%lu\n",adf_test_checks,adf_test_failures);
    flint_cleanup();
    return adf_test_failures ? 1 : 0;
}
