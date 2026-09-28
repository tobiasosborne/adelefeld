/* tools/mutate/example/tests/test_weak.c: a deliberately weak test of src/example.c.

   It checks the diagonal a = b of adf_example_equal, where a + H Zhat and b + H Zhat are equal
   for every H, and one value of adf_example_sum_radius. Nothing else: the way h enters the
   predicate, the way the loop of the remainder runs and the order of the two arguments of the
   gcd are not checked.

   The test is here to be caught. A mutant of any of those passes it, and
   `make mutate-selftest` runs the mutation tool over this example and requires that it finds
   such a survivor. The strong test of the same file is tests/test_strong.c, which leaves none.

   The declarations come from src/example.h and the runner from tests/test_runner.h, copied
   here by tools/mutate/selftest.py so that the example builds on its own. */

#include <stdio.h>

#include "example.h"
#include "test_runner.h"

ADF_TEST(equal_is_reflexive)
{
    long a;

    for (a = -8; a <= 8; a++)
    {
        long h;

        for (h = 0; h <= 8; h++)
            ADF_CHECK_MSG(adf_example_equal(a, h, a) == 1, "a = %ld, h = %ld", a, h);
    }
}

ADF_TEST(the_sum_of_6_and_12_zhat_has_radius_6)
{
    ADF_CHECK(adf_example_sum_radius(6, 12) == 6);
}
