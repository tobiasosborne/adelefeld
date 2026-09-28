/* tools/mutate/example/src/example.h: the three declarations of the example. */

#ifndef ADF_EXAMPLE_H
#define ADF_EXAMPLE_H

/* 1 if a + h Zhat and b + h Zhat are the same set, 0 otherwise (SPEC 4.2). */
long adf_example_equal(long a, long h, long b);

/* gcd(n, m) for n, m >= 0. */
long adf_example_gcd(long n, long m);

/* The radius of the sum of n Zhat and m Zhat, which is gcd(n, m) (SPEC 4.3). */
long adf_example_sum_radius(long n, long m);

#endif /* ADF_EXAMPLE_H */
