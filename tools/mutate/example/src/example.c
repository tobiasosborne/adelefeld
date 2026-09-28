/* tools/mutate/example/src/example.c: the example the mutation tool is tested on.

   Two small pieces of the mathematics of milestone 1, in the shape the tool needs: a loop
   with a guard, an assignment, a remainder, a comparison, a call with two arguments and a
   constant, so that every kind of mutation the tool makes has something to change here.

   The claims are the two of docs/SPEC.md 4.2 and 4.3 that need nothing but integers:

       adf_example_equal(a, h, b) = 1  if and only if  a = b, or h > 0 and h divides a - b
       adf_example_gcd(n, m)         = gcd(n, m) for n, m >= 0

   The first is the equal_set predicate for two finite balls with denominator 1: the ball
   a + H Zhat is the single point a when H = 0, and otherwise the residue class of a modulo H.
   The second is the radius of the sum of a + N Zhat and b + M Zhat, which is gcd(N, M); it is
   given its own name so that the tool has a call with two arguments to exchange, and the test
   checks it against the enumeration. */

#include "example.h"

long
adf_example_equal(long a, long h, long b)
{
    if (h == 0)
        return a == b;
    return (a - b) % h == 0;
}

long
adf_example_gcd(long n, long m)
{
    while (m != 0)
    {
        long t = n % m;
        n = m;
        m = t;
    }
    return n;
}

long
adf_example_sum_radius(long n, long m)
{
    return adf_example_gcd(n, m);
}

/* lcm(n, m) = n / gcd(n, m) * m for n, m >= 0 (the domain of adf_example_gcd itself); g = 0
   only at n = m = 0. The call to adf_example_gcd is not a tail call (its result is bound to g
   first), so the gcd_lcm mutant that renames it to adf_example_lcm recurses but does not loop
   forever: every call still pushes a frame, and the recursion overflows the stack in a
   fraction of a second on any input (verified: tools/mutate/selftest.py's strong run kills it,
   well inside the tool's --timeout). */
long
adf_example_lcm(long n, long m)
{
    long g = adf_example_gcd(n, m);

    if (g == 0)
        return 0;
    return (n / g) * m;
}

int
adf_example_status(long n)
{
    if (n < 0)
        return ADF_DOMAIN;
    if (n == 0)
        return ADF_UNSUPPORTED;
    return ADF_OK;
}

void
adf_example_box_init(adf_example_box * b)
{
    b->v = 0;
}

void
adf_example_box_clear(adf_example_box * b)
{
    b->v = 0;
}

void
adf_example_box_set(adf_example_box * b, long v)
{
    b->v = v;
}

long
adf_example_box_get(const adf_example_box * b)
{
    return b->v;
}

void
adf_example_box_add(adf_example_box * out, const adf_example_box * a, const adf_example_box * b)
{
    out->v = a->v + b->v;
}

void
adf_example_box_sub(adf_example_box * out, const adf_example_box * a, const adf_example_box * b)
{
    out->v = a->v - b->v;
}

long
adf_example_box_sum(long x, long y)
{
    adf_example_box a, b, out;

    adf_example_box_init(&a);
    adf_example_box_init(&b);
    adf_example_box_init(&out);
    adf_example_box_set(&a, x);
    adf_example_box_set(&b, y);
    adf_example_box_add(&out, &a, &b);
    return adf_example_box_get(&out);
}

long
adf_example_box_diff(long x, long y)
{
    adf_example_box a, b, out;

    adf_example_box_init(&a);
    adf_example_box_init(&b);
    adf_example_box_init(&out);
    adf_example_box_set(&a, x);
    adf_example_box_set(&b, y);
    adf_example_box_sub(&out, &a, &b);
    return adf_example_box_get(&out);
}

/* value rounded down towards zero to a multiple of unit (unit == 0: value unchanged, since
   dividing by it would not be; a negative unit still scales, C truncating division towards
   zero either way). Guarding on `== 0` rather than `<= 0` is deliberate: `<= 0` would leave a
   zero_one mutant of its literal (`<= 1`) with nothing to catch it, since scaling by the unit 1
   is already a no-op on the branch it would otherwise take (adf-obp self-test finding); `== 0`
   turns the same mutant into a division by zero instead, which crashes and so is killed. */
static long
adf_example_scale(long value, long unit)
{
    if (unit == 0)
        return value;
    return (value / unit) * unit;
}

long
adf_example_round(long value, long prec)
{
    return adf_example_scale(value, prec);
}
