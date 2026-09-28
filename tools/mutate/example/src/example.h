/* tools/mutate/example/src/example.h: the declarations of the example.

   The first three (adf_example_equal, adf_example_gcd, adf_example_sum_radius) are the
   original example, for op, cmp, zero_one, drop_assign, negate_if and the 2-argument case of
   swap_args (a call with no separate output argument). Everything below was added for the
   self-test of adf-obp's fix (tools/mutate/selftest.py): a second gcd-shaped function so
   gcd_lcm has a declared name to rename to; a small FLINT-style "box" type (an output-first
   3-argument call) for the output-preserving swap_args, call_swap and drop_call; a status
   function for the status kind; and a prec-named argument for the prec kind. */

#ifndef ADF_EXAMPLE_H
#define ADF_EXAMPLE_H

#include "adelefeld/status.h"

/* 1 if a + h Zhat and b + h Zhat are the same set, 0 otherwise (SPEC 4.2). */
long adf_example_equal(long a, long h, long b);

/* gcd(n, m) for n, m >= 0. */
long adf_example_gcd(long n, long m);

/* The radius of the sum of n Zhat and m Zhat, which is gcd(n, m) (SPEC 4.3). */
long adf_example_sum_radius(long n, long m);

/* lcm(n, m) for n, m >= 0, lcm(0, m) = lcm(n, 0) = 0. Needed only so that gcd_lcm has a
   declared name to rename adf_example_gcd to (adf-obp: the present tool renames a call ending
   in `_gcd` to a bare `lcm`, which nothing declares). */
long adf_example_lcm(long n, long m);

/* ADF_OK for n > 0, ADF_UNSUPPORTED for n == 0, ADF_DOMAIN for n < 0: a status function for
   the `status` kind. */
int adf_example_status(long n);

/* A minimal FLINT-style value: one field, an output-first 3-argument add/sub pair, so that
   swap_args (never the output argument), call_swap (_add <-> _sub) and drop_call (a bare call
   statement, kept only when the name does not end in _init/_clear) each have something real to
   mutate and something real to catch it. */
typedef struct
{
    long v;
} adf_example_box;

void adf_example_box_init(adf_example_box * b);
void adf_example_box_clear(adf_example_box * b);
void adf_example_box_set(adf_example_box * b, long v);
long adf_example_box_get(const adf_example_box * b);
void adf_example_box_add(adf_example_box * out, const adf_example_box * a,
                         const adf_example_box * b);
void adf_example_box_sub(adf_example_box * out, const adf_example_box * a,
                         const adf_example_box * b);

/* x + y and x - y through the box type, exercised by the self-test. */
long adf_example_box_sum(long x, long y);
long adf_example_box_diff(long x, long y);

/* value rounded down to a multiple of prec (prec == 0: value unchanged). The call inside
   passes prec as a bare argument, for the `prec` kind. */
long adf_example_round(long value, long prec);

#endif /* ADF_EXAMPLE_H */
