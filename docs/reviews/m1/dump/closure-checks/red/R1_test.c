#define ADF_TEST_NO_MAIN
#include "tests/test_dump_limits.c"
int main(void) { adf_test_current = "block_cap_before_the_predicate_of_the_context";
adf_test_fn_block_cap_before_the_predicate_of_the_context();
printf("selected_tests=1 checks=%lu failures=%lu\n", adf_test_checks, adf_test_failures);
flint_cleanup_master(); return adf_test_failures != 0; }
