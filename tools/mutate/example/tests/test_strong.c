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

/* lcm(n, m) = n * m / gcd(n, m), and lcm(n, m) != gcd(n, m) for most pairs; this also kills the
   gcd_lcm mutant of adf_example_sum_radius's call to adf_example_gcd, since that mutant calls
   this function instead (tools/mutate/selftest.py: added for adf-obp). */
ADF_TEST(lcm_is_the_least_common_multiple)
{
    long n;
    long m;

    for (n = 0; n <= MODULI; n++)
    {
        for (m = 0; m <= MODULI; m++)
        {
            long g = reference_gcd(n, m);
            long want = (n == 0 || m == 0) ? 0 : (n / g) * m;

            ADF_CHECK_MSG(adf_example_lcm(n, m) == want, "lcm(%ld, %ld) = %ld, expected %ld", n, m,
                          adf_example_lcm(n, m), want);
        }
    }
}

/* adf_example_status(n): ADF_DOMAIN for n < 0, ADF_UNSUPPORTED for n == 0, ADF_OK for n > 0
   (added for adf-obp, the `status` kind). */
ADF_TEST(status_is_domain_unsupported_or_ok)
{
    long n;

    for (n = -RANGE; n <= RANGE; n++)
    {
        int want = (n < 0) ? ADF_DOMAIN : (n == 0) ? ADF_UNSUPPORTED : ADF_OK;

        ADF_CHECK_MSG(adf_example_status(n) == want, "status(%ld) = %d, expected %d", n,
                      adf_example_status(n), want);
    }
}

/* adf_example_box_init and adf_example_box_clear must each leave v at exactly 0: read v
   straight after init/clear, before anything else can overwrite it, so that a mutant of the
   literal 0 they assign (adf-obp: zero_one) is caught here and not hidden behind the
   adf_example_box_set that every other test calls right after init. */
ADF_TEST(box_init_and_clear_leave_v_at_zero)
{
    adf_example_box b;

    adf_example_box_init(&b);
    ADF_CHECK_MSG(adf_example_box_get(&b) == 0, "box_init: v = %ld, expected 0",
                  adf_example_box_get(&b));
    adf_example_box_set(&b, 5);
    adf_example_box_clear(&b);
    ADF_CHECK_MSG(adf_example_box_get(&b) == 0, "box_clear: v = %ld, expected 0",
                  adf_example_box_get(&b));
}

/* x + y and x - y through the box type (added for adf-obp: swap_args never touches the output
   argument of a FLINT-style call, call_swap, drop_call). Exchanging the two non-output
   arguments of adf_example_box_add computes the same sum; that one surviving mutant is excused
   in the EQUIVALENT text of tools/mutate/selftest.py, exactly as fball.c excuses its own
   commutative swaps in tools/mutate/equivalent.txt. Subtraction is not commutative, so its
   swap is caught here without an excuse. */
ADF_TEST(box_sum_and_diff_match_plus_and_minus)
{
    long x;
    long y;

    for (x = -RANGE; x <= RANGE; x++)
    {
        for (y = -RANGE; y <= RANGE; y++)
        {
            ADF_CHECK_MSG(adf_example_box_sum(x, y) == x + y,
                          "box_sum(%ld, %ld) = %ld, expected %ld", x, y,
                          adf_example_box_sum(x, y), x + y);
            ADF_CHECK_MSG(adf_example_box_diff(x, y) == x - y,
                          "box_diff(%ld, %ld) = %ld, expected %ld", x, y,
                          adf_example_box_diff(x, y), x - y);
        }
    }
}

/* value rounded down to a multiple of prec, prec == 0 leaving it unchanged, a negative prec
   still scaling (added for adf-obp: the `prec` kind mutates the argument of the inner call,
   not the parameter). */
ADF_TEST(round_matches_the_reference)
{
    long value;
    long prec;

    for (value = -RANGE; value <= RANGE; value++)
    {
        for (prec = -2; prec <= 5; prec++)
        {
            long want = (prec == 0) ? value : (value / prec) * prec;

            ADF_CHECK_MSG(adf_example_round(value, prec) == want,
                          "round(%ld, %ld) = %ld, expected %ld", value, prec,
                          adf_example_round(value, prec), want);
        }
    }
}
