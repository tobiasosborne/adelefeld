#define ADF_TEST_NO_MAIN
#include "tests/test_dump_limits.c"
int main(void) { adf_test_current = "qclass_exponent_bound_is_a_limit_of_stage_four";
adf_test_fn_qclass_exponent_bound_is_a_limit_of_stage_four();
printf("selected_tests=1 checks=%lu failures=%lu\n", adf_test_checks, adf_test_failures);
flint_cleanup_master(); return adf_test_failures != 0; }
