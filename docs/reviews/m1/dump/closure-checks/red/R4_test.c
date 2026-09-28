#define ADF_TEST_NO_MAIN
#include "tests/test_dump.c"
int main(void) { adf_test_current = "header_before_the_whitespace_of_the_body";
adf_test_fn_header_before_the_whitespace_of_the_body();
printf("selected_tests=1 checks=%lu failures=%lu\n", adf_test_checks, adf_test_failures);
flint_cleanup_master(); return adf_test_failures != 0; }
