/* tools/mutate/example/tests/test_strong.c: the strong test of src/example.c.

   The claims of example.c are

       adf_example_equal(a, h, b) = 1  if and only if  a = b, or h > 0 and h divides a - b
       adf_example_gcd(n, m)         = gcd(n, m) for n, m >= 0
       adf_example_sum_radius(n, m)  = gcd(n, m)

   and the test states them by enumeration over every small case: |a|, |b|, h <= 8 for the
   first, and 0 <= n, m <= 12 for the other two, with the expected greatest common divisor
   computed by the test itself from the definition (the smaller number, then repeatedly the
   remainder). Both arguments of every call are enumerated, so exchanging them is caught, and
   every step of the loop of the remainder is used, so removing an assignment from it is
   caught.

   An identity that a constant function satisfies does not count: adf_example_equal(1, 4, 5) is
   1 and adf_example_equal(1, 4, 4) is 0, and the second test below says so. */

#include <stdio.h>

#include "example.h"
#include "test_runner.h"

#define RANGE 8
#define MODULI 12

/* gcd(n, m) from the definition, written here so that the test does not use the function it
   tests: the remainder of n by m, then of m by that, until the remainder is 0. */

static long
reference_gcd(long n, long m)
{
    if (n < 0 || m < 0)
        return -1; /* the claim is only for n, m >= 0 */
    while (m != 0)
    {
        long t = n % m;
        n = m;
        m = t;
    }
    return n;
}

ADF_TEST(equal_is_the_congruence_of_the_two_centres)
{
    long a;
    long b;
    long h;

    for (h = 0; h <= RANGE; h++)
    {
        for (a = -RANGE; a <= RANGE; a++)
        {
            for (b = -RANGE; b <= RANGE; b++)
            {
                int want = (a == b) || (h > 0 && (a - b) % h == 0);
                int got = adf_example_equal(a, h, b) == 1;

                ADF_CHECK_MSG(got == want, "equal(%ld, %ld, %ld) = %d, expected %d", a, h, b, got,
                              want);
            }
        }
    }
}

ADF_TEST(equal_is_not_a_constant_function)
{
    /* A zero radius compares the centres only, a positive radius compares them modulo h. */

    ADF_CHECK(adf_example_equal(1, 4, 5) == 1);
    ADF_CHECK(adf_example_equal(0, 4, 4) == 1);
    ADF_CHECK(adf_example_equal(1, 4, 4) == 0);
    ADF_CHECK(adf_example_equal(0, 4, 5) == 0);
    ADF_CHECK(adf_example_equal(-4, 0, 4) == 0);
    ADF_CHECK(adf_example_equal(-4, 4, 0) == 1);
}

ADF_TEST(gcd_is_the_greatest_common_divisor)
{
    long n;
    long m;

    for (n = 0; n <= MODULI; n++)
    {
        for (m = 0; m <= MODULI; m++)
        {
            long want = reference_gcd(n, m);

            ADF_CHECK_MSG(adf_example_gcd(n, m) == want, "gcd(%ld, %ld) = %ld, expected %ld", n, m,
                          adf_example_gcd(n, m), want);
            ADF_CHECK_MSG(adf_example_sum_radius(n, m) == want,
                          "sum_radius(%ld, %ld) = %ld, expected %ld", n, m,
                          adf_example_sum_radius(n, m), want);
        }
    }
}
