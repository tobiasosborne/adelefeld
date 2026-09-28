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
