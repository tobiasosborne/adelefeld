/* The full lpow program exceeds the Valgrind time bound. Check the two new tests
   without changing any assertion, then release documented FLINT caches.
   refs/src/flint-3.0.1/memory.rst:29-44. Full programs pass natively and under ASAN/UBSAN. */
#define ADF_TEST_NO_MAIN
#include "../../tests/test_lpow.c"

int main(void)
{
    unsigned long ran = 0;
    for (adf_test *t = adf_test_head; t != NULL; t = t->next)
    {
        if (strcmp(t->name, "repair6_powunit_integer_witnesses") &&
            strcmp(t->name, "repair6_powrat_hensel_centres")) continue;
        adf_test_current = t->name;
        t->fn();
        ran++;
    }
    printf("%lu selected tests, %lu checks, %lu failed checks\n", ran, adf_test_checks, adf_test_failures);
    int status = adf_test_failures != 0 || ran != 2;
    flint_cleanup_master();
    return status;
}
