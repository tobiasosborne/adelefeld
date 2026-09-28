/* tests/test_adele_prec.c: the precision argument of adelefeld/adele.h, the finite half of the
   two swaps, and the status of the projection to the real place.

   The claim of this file, for every operation that takes a prec (adf_adele_add, adf_adele_sub,
   adf_adele_mul, adf_adele_add_rat, adf_adele_mul_rat, adf_adele_div_rat, adf_adele_set_rat and
   the seven adf_cadele counterparts, conventions 2.2: the real coordinate is computed at prec
   bits): with operands that are exact at 200 bits and whose exact result is not representable at
   2 bits, the result at prec = 200 has a radius below 2^-150 times the midpoint, and the result
   at prec = 2 still contains the exact value. A mutant that passes 2 where prec stands (the
   `prec` mutation of tools/mutate/mutate.py, lines 256, 259, 283, 285, 318, 319, 489, 492, 512,
   514, 543, 544 of src/adele.c) gives a radius of the order of 2^-3 at the first check and is
   killed there.

   The finite coordinate of an adele does not depend on prec (adele.h, "the finite coordinate
   does not depend on prec"); the tests of that are tests/test_adele.c. This file is about prec.

   The two swaps: adf_adele_swap and adf_cadele_swap exchange the whole value, so the finite half
   is exchanged too (the `drop_call` mutation of the adf_fball_swap inside them, src/adele.c lines
   82 and 360). The test gives the two values equal real parts, so that the real coordinate alone
   cannot see whether the finite halves moved.

   The status: adf_adele_get_arb_at at the archimedean place returns ADF_OK (adele.h, "Status:
   ADF_OK, r written"), the `status` mutation of line 179 of src/adele.c. */

#include <stdio.h>

#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>

#include <adelefeld/adele.h>

#include "test_runner.h"

/* --------------------------------------------------------------- helpers */

/* 1 if the radius of x, multiplied by 2^bits, is strictly below the absolute value of its
   midpoint. For a nonzero midpoint this is "the relative radius is below 2^-bits". */
static int
rel_radius_below(const arb_t x, slong bits)
{
    arf_t r, m;
    int ok;

    arf_init(r);
    arf_init(m);
    arf_set_mag(r, arb_radref(x));
    arf_mul_2exp_si(r, r, bits);
    arf_abs(m, arb_midref(x));
    ok = arf_cmp(r, m) < 0;
    arf_clear(r);
    arf_clear(m);
    return ok;
}

/* The two claims of this file for one real coordinate: the radius is below 2^-150 times the
   midpoint, and the ball contains the exact value. */
static void
check_precise(const arb_t r, const fmpq_t ex, const char * what)
{
    ADF_CHECK_MSG(rel_radius_below(r, 150),
                  "%s: the radius is not below 2^-150 times the midpoint (%ld bits of relative "
                  "error)", what, (long) arb_rel_error_bits(r));
    ADF_CHECK_MSG(arb_contains_fmpq(r, ex), "%s: the result does not contain the exact value",
                  what);
}

/* The same two claims for a complex coordinate. An acb is the product of the two real intervals
   (acb.h, "An acb_t represents a ball over the complex numbers ... [x1 +/- y1] + i [x2 +/- y2]"),
   so the exact value er + ei i lies in z exactly when er lies in the real interval and ei lies
   in the imaginary interval; both are decided with arb_contains_fmpq on the exact rationals. */
static void
check_precise_acb(const acb_t z, const fmpq_t er, const fmpq_t ei, const char * what,
                  int imag_mid_nonzero)
{
    ADF_CHECK_MSG(rel_radius_below(acb_realref(z), 150),
                  "%s: the real radius is not below 2^-150 times the real midpoint (%ld bits of "
                  "relative error)", what, (long) arb_rel_error_bits(acb_realref(z)));
    ADF_CHECK_MSG(arb_contains_fmpq(acb_realref(z), er),
                  "%s: the real interval does not contain the exact real value", what);
    ADF_CHECK_MSG(arb_contains_fmpq(acb_imagref(z), ei),
                  "%s: the imaginary interval does not contain the exact imaginary value", what);
    if (imag_mid_nonzero)
        ADF_CHECK_MSG(rel_radius_below(acb_imagref(z), 150),
                      "%s: the imaginary radius is not below 2^-150 times the imaginary midpoint "
                      "(%ld bits of relative error)", what,
                      (long) arb_rel_error_bits(acb_imagref(z)));
}

/* The same, at prec 2: the two intervals still contain the two exact values. */
static void
check_contains_acb(const acb_t z, const fmpq_t er, const fmpq_t ei, const char * what)
{
    ADF_CHECK_MSG(arb_contains_fmpq(acb_realref(z), er),
                  "%s: the real interval does not contain the exact real value", what);
    ADF_CHECK_MSG(arb_contains_fmpq(acb_imagref(z), ei),
                  "%s: the imaginary interval does not contain the exact imaginary value", what);
}

/* The two real parts of the operands, as exact rationals:
       X = (2^100 + 1)/2^100 = 1 + 2^-100,   Y = (2^91 + 1)/2^90 = 2 + 2^-90.
   X has 101 significant bits and Y has 92, so both are exact at 200 bits; neither is
   representable at 2 bits, and neither is the sum, the difference or the product of the two. */

static void
exact_x(fmpq_t q)
{
    fmpz_t n, d;

    fmpz_init(n);
    fmpz_init(d);
    fmpz_set_ui(n, 1);
    fmpz_mul_2exp(n, n, 100);
    fmpz_add_ui(n, n, 1);
    fmpz_set_ui(d, 1);
    fmpz_mul_2exp(d, d, 100);
    fmpq_set_fmpz_frac(q, n, d);
    fmpz_clear(n);
    fmpz_clear(d);
}

static void
exact_y(fmpq_t q)
{
    fmpz_t n, d;

    fmpz_init(n);
    fmpz_init(d);
    fmpz_set_ui(d, 1);
    fmpz_mul_2exp(d, d, 90);
    fmpz_mul_2exp(n, d, 1);
    fmpz_add_ui(n, n, 1);
    fmpq_set_fmpz_frac(q, n, d);
    fmpz_clear(n);
    fmpz_clear(d);
}

/* The exact rational 1/4, the imaginary part of the first operand, and -1/2, that of the
   second. Both are dyadic, so both are exact at every precision. */
static void
exact_i1(fmpq_t q)
{
    fmpq_set_si(q, 1, 4);
}

static void
exact_i2(fmpq_t q)
{
    fmpq_set_si(q, -1, 2);
}

static void
set_im(arb_t r, const fmpq_t q)
{
    arb_set_fmpq(r, q, 200);
    ADF_CHECK_MSG(arb_is_exact(r), "the imaginary part of an operand is not exact");
}

/* The real part of an operand: X for the first, Y for the second, both exact at 200 bits. */
static void
set_re_x(arb_t r)
{
    fmpq_t q;

    fmpq_init(q);
    exact_x(q);
    arb_set_fmpq(r, q, 200);
    ADF_CHECK_MSG(arb_is_exact(r), "the real part of X is not exact at 200 bits");
    fmpq_clear(q);
}

static void
set_re_y(arb_t r)
{
    fmpq_t q;

    fmpq_init(q);
    exact_y(q);
    arb_set_fmpq(r, q, 200);
    ADF_CHECK_MSG(arb_is_exact(r), "the real part of Y is not exact at 200 bits");
    fmpq_clear(q);
}

/* The finite part of the first operand: the exact ball 1/3. Of the second: (5 + 18 Zhat)/1. */
static void
set_fin_x(adf_fball_t f)
{
    adf_rat_t q;

    adf_rat_init(q);
    fmpq_set_si(q->q, 1, 3);
    adf_fball_set_rat(f, q);
    adf_rat_clear(q);
}

static void
set_fin_y(adf_fball_t f)
{
    fmpz_t A, H, d;

    fmpz_init_set_si(A, 5);
    fmpz_init_set_si(H, 18);
    fmpz_init_set_si(d, 1);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

static void
build_adele(adf_adele_t x)
{
    adf_fball_t f;
    arb_t r;

    arb_init(r);
    adf_fball_init(f);
    set_re_x(r);
    set_fin_x(f);
    ADF_CHECK(adf_adele_set_arb_fball(x, r, f) == ADF_OK);
    arb_clear(r);
    adf_fball_clear(f);
}

static void
build_adele_y(adf_adele_t y)
{
    adf_fball_t f;
    arb_t r;

    arb_init(r);
    adf_fball_init(f);
    set_re_y(r);
    set_fin_y(f);
    ADF_CHECK(adf_adele_set_arb_fball(y, r, f) == ADF_OK);
    arb_clear(r);
    adf_fball_clear(f);
}

static void
build_cadele(adf_cadele_t x)
{
    adf_fball_t f;
    acb_t z;
    fmpq_t q;

    acb_init(z);
    adf_fball_init(f);
    fmpq_init(q);
    set_re_x(acb_realref(z));
    exact_i1(q);
    set_im(acb_imagref(z), q);
    set_fin_x(f);
    ADF_CHECK(adf_cadele_set_acb_fball(x, z, f) == ADF_OK);
    fmpq_clear(q);
    acb_clear(z);
    adf_fball_clear(f);
}

static void
build_cadele_y(adf_cadele_t y)
{
    adf_fball_t f;
    acb_t z;
    fmpq_t q;

    acb_init(z);
    adf_fball_init(f);
    fmpq_init(q);
    set_re_y(acb_realref(z));
    exact_i2(q);
    set_im(acb_imagref(z), q);
    set_fin_y(f);
    ADF_CHECK(adf_cadele_set_acb_fball(y, z, f) == ADF_OK);
    fmpq_clear(q);
    acb_clear(z);
    adf_fball_clear(f);
}

/* ============================== adf_adele ============================== */

ADF_TEST(adele_add_computes_the_real_part_at_the_precision_asked_for)
{
    adf_adele_t x, y, z;
    fmpq_t a, b, ex;

    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    build_adele(x);
    build_adele_y(y);
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(ex);
    exact_x(a);
    exact_y(b);
    fmpq_add(ex, a, b);

    adf_adele_add(z, x, y, 200);
    check_precise(z->inf, ex, "adf_adele_add at prec 200");
    adf_adele_add(z, x, y, 2);
    ADF_CHECK_MSG(arb_contains_fmpq(z->inf, ex), "adf_adele_add at prec 2 lost the exact value");

    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(ex);
    adf_adele_clear(x);
    adf_adele_clear(y);
    adf_adele_clear(z);
}

ADF_TEST(adele_sub_computes_the_real_part_at_the_precision_asked_for)
{
    adf_adele_t x, y, z;
    fmpq_t a, b, ex;

    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    build_adele(x);
    build_adele_y(y);
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(ex);
    exact_x(a);
    exact_y(b);
    fmpq_sub(ex, a, b);

    adf_adele_sub(z, x, y, 200);
    check_precise(z->inf, ex, "adf_adele_sub at prec 200");
    adf_adele_sub(z, x, y, 2);
    ADF_CHECK_MSG(arb_contains_fmpq(z->inf, ex), "adf_adele_sub at prec 2 lost the exact value");

    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(ex);
    adf_adele_clear(x);
    adf_adele_clear(y);
    adf_adele_clear(z);
}

ADF_TEST(adele_mul_computes_the_real_part_at_the_precision_asked_for)
{
    adf_adele_t x, y, z;
    fmpq_t a, b, ex;

    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    build_adele(x);
    build_adele_y(y);
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(ex);
    exact_x(a);
    exact_y(b);
    fmpq_mul(ex, a, b);

    adf_adele_mul(z, x, y, 200);
    check_precise(z->inf, ex, "adf_adele_mul at prec 200");
    adf_adele_mul(z, x, y, 2);
    ADF_CHECK_MSG(arb_contains_fmpq(z->inf, ex), "adf_adele_mul at prec 2 lost the exact value");

    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(ex);
    adf_adele_clear(x);
    adf_adele_clear(y);
    adf_adele_clear(z);
}

ADF_TEST(adele_add_rat_computes_the_real_part_at_the_precision_asked_for)
{
    adf_adele_t x, z;
    adf_rat_t q;
    fmpq_t a, ex;

    adf_adele_init(x);
    adf_adele_init(z);
    adf_rat_init(q);
    build_adele(x);
    fmpq_set_si(q->q, 3, 7);
    fmpq_init(a);
    fmpq_init(ex);
    exact_x(a);
    fmpq_add(ex, a, q->q);

    adf_adele_add_rat(z, x, q, 200);
    check_precise(z->inf, ex, "adf_adele_add_rat at prec 200");
    adf_adele_add_rat(z, x, q, 2);
    ADF_CHECK_MSG(arb_contains_fmpq(z->inf, ex),
                  "adf_adele_add_rat at prec 2 lost the exact value");

    fmpq_clear(a);
    fmpq_clear(ex);
    adf_rat_clear(q);
    adf_adele_clear(x);
    adf_adele_clear(z);
}

ADF_TEST(adele_mul_rat_computes_the_real_part_at_the_precision_asked_for)
{
    adf_adele_t x, z;
    adf_rat_t q;
    fmpq_t a, ex;

    adf_adele_init(x);
    adf_adele_init(z);
    adf_rat_init(q);
    build_adele(x);
    fmpq_set_si(q->q, 3, 7);
    fmpq_init(a);
    fmpq_init(ex);
    exact_x(a);
    fmpq_mul(ex, a, q->q);

    adf_adele_mul_rat(z, x, q, 200);
    check_precise(z->inf, ex, "adf_adele_mul_rat at prec 200");
    adf_adele_mul_rat(z, x, q, 2);
    ADF_CHECK_MSG(arb_contains_fmpq(z->inf, ex),
                  "adf_adele_mul_rat at prec 2 lost the exact value");

    fmpq_clear(a);
    fmpq_clear(ex);
    adf_rat_clear(q);
    adf_adele_clear(x);
    adf_adele_clear(z);
}

ADF_TEST(adele_div_rat_computes_the_real_part_at_the_precision_asked_for)
{
    adf_adele_t x, z;
    adf_rat_t q;
    fmpq_t a, ex;

    adf_adele_init(x);
    adf_adele_init(z);
    adf_rat_init(q);
    build_adele(x);
    fmpq_set_si(q->q, 7, 3);
    fmpq_init(a);
    fmpq_init(ex);
    exact_x(a);
    fmpq_div(ex, a, q->q);

    ADF_CHECK(adf_adele_div_rat(z, x, q, 200) == ADF_OK);
    check_precise(z->inf, ex, "adf_adele_div_rat at prec 200");
    ADF_CHECK(adf_adele_div_rat(z, x, q, 2) == ADF_OK);
    ADF_CHECK_MSG(arb_contains_fmpq(z->inf, ex),
                  "adf_adele_div_rat at prec 2 lost the exact value");

    fmpq_clear(a);
    fmpq_clear(ex);
    adf_rat_clear(q);
    adf_adele_clear(x);
    adf_adele_clear(z);
}

ADF_TEST(adele_set_rat_converts_at_the_precision_asked_for)
{
    adf_adele_t y;
    adf_rat_t q;
    fmpq_t ex;

    adf_adele_init(y);
    adf_rat_init(q);
    fmpq_set_si(q->q, 3, 7);
    fmpq_init(ex);
    fmpq_set(ex, q->q);

    adf_adele_set_rat(y, q, 200);
    check_precise(y->inf, ex, "adf_adele_set_rat at prec 200");
    adf_adele_set_rat(y, q, 2);
    ADF_CHECK_MSG(arb_contains_fmpq(y->inf, ex),
                  "adf_adele_set_rat at prec 2 lost the exact value");

    fmpq_clear(ex);
    adf_rat_clear(q);
    adf_adele_clear(y);
}

/* ============================== adf_cadele ============================== */

ADF_TEST(cadele_add_computes_the_real_part_at_the_precision_asked_for)
{
    adf_cadele_t x, y, z;
    fmpq_t a, b, i1, i2, er, ei;
    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_cadele_init(z);
    build_cadele(x);
    build_cadele_y(y);
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(i1);
    fmpq_init(i2);
    fmpq_init(er);
    fmpq_init(ei);
    exact_x(a);
    exact_y(b);
    exact_i1(i1);
    exact_i2(i2);
    adf_cadele_add(z, x, y, 200);
        fmpq_add(er, a, b);
        fmpq_add(ei, i1, i2);
    ADF_CHECK_MSG(fmpq_is_zero(ei) == 0, "the exact imaginary part is zero, so the "
                   "imaginary relative radius is not compared");
    check_precise_acb(z->inf, er, ei, "adf_cadele_add at prec 200", 1);
        adf_cadele_add(z, x, y, 2);
    check_contains_acb(z->inf, er, ei, "adf_cadele_add at prec 2 lost the exact value");

    adf_cadele_clear(x);
    adf_cadele_clear(y);
    adf_cadele_clear(z);
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(i1);
    fmpq_clear(i2);
    fmpq_clear(er);
    fmpq_clear(ei);
}

ADF_TEST(cadele_sub_computes_the_real_part_at_the_precision_asked_for)
{
    adf_cadele_t x, y, z;
    fmpq_t a, b, i1, i2, er, ei;
    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_cadele_init(z);
    build_cadele(x);
    build_cadele_y(y);
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(i1);
    fmpq_init(i2);
    fmpq_init(er);
    fmpq_init(ei);
    exact_x(a);
    exact_y(b);
    exact_i1(i1);
    exact_i2(i2);
    adf_cadele_sub(z, x, y, 200);
        fmpq_sub(er, a, b);
        fmpq_sub(ei, i1, i2);
    ADF_CHECK_MSG(fmpq_is_zero(ei) == 0, "the exact imaginary part is zero, so the "
                   "imaginary relative radius is not compared");
    check_precise_acb(z->inf, er, ei, "adf_cadele_sub at prec 200", 1);
        adf_cadele_sub(z, x, y, 2);
    check_contains_acb(z->inf, er, ei, "adf_cadele_sub at prec 2 lost the exact value");

    adf_cadele_clear(x);
    adf_cadele_clear(y);
    adf_cadele_clear(z);
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(i1);
    fmpq_clear(i2);
    fmpq_clear(er);
    fmpq_clear(ei);
}

ADF_TEST(cadele_mul_computes_the_real_part_at_the_precision_asked_for)
{
    adf_cadele_t x, y, z;
    fmpq_t a, b, i1, i2, er, ei;
    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_cadele_init(z);
    build_cadele(x);
    build_cadele_y(y);
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(i1);
    fmpq_init(i2);
    fmpq_init(er);
    fmpq_init(ei);
    exact_x(a);
    exact_y(b);
    exact_i1(i1);
    exact_i2(i2);
    adf_cadele_mul(z, x, y, 200);
        fmpq_mul(er, a, b);
    fmpq_mul(ei, i1, i2);
    fmpq_sub(er, er, ei);
        fmpq_mul(ei, a, i2);
    fmpq_mul(i2, i1, b);
    fmpq_add(ei, ei, i2);
    ADF_CHECK_MSG(fmpq_is_zero(ei) == 0, "the exact imaginary part is zero, so the "
                   "imaginary relative radius is not compared");
    check_precise_acb(z->inf, er, ei, "adf_cadele_mul at prec 200", 1);
        adf_cadele_mul(z, x, y, 2);
    check_contains_acb(z->inf, er, ei, "adf_cadele_mul at prec 2 lost the exact value");

    adf_cadele_clear(x);
    adf_cadele_clear(y);
    adf_cadele_clear(z);
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(i1);
    fmpq_clear(i2);
    fmpq_clear(er);
    fmpq_clear(ei);
}

ADF_TEST(cadele_add_rat_computes_the_real_part_at_the_precision_asked_for)
{
    adf_cadele_t x, z;
    adf_rat_t q;
    fmpq_t a, i1, i2, er, ei;
    adf_cadele_init(x);
    adf_cadele_init(z);
    adf_rat_init(q);
    build_cadele(x);
    fmpq_init(a);
    fmpq_init(i1);
    fmpq_init(i2);
    fmpq_init(er);
    fmpq_init(ei);
    exact_x(a);
    exact_i1(i1);
    exact_i2(i2);
    fmpq_set_si(q->q, 3, 7);
    adf_cadele_add_rat(z, x, q, 200);
        fmpq_add(er, a, q->q);
        fmpq_set(ei, i1);
    ADF_CHECK_MSG(fmpq_is_zero(ei) == 0, "the exact imaginary part is zero, so the "
                   "imaginary relative radius is not compared");
    check_precise_acb(z->inf, er, ei, "adf_cadele_add_rat at prec 200", 1);
        adf_cadele_add_rat(z, x, q, 2);
    check_contains_acb(z->inf, er, ei, "adf_cadele_add_rat at prec 2 lost the exact value");

    adf_cadele_clear(x);
    adf_cadele_clear(z);
    adf_rat_clear(q);
    fmpq_clear(a);
    fmpq_clear(i1);
    fmpq_clear(i2);
    fmpq_clear(er);
    fmpq_clear(ei);
}

ADF_TEST(cadele_mul_rat_computes_the_real_part_at_the_precision_asked_for)
{
    adf_cadele_t x, z;
    adf_rat_t q;
    fmpq_t a, i1, i2, er, ei;
    adf_cadele_init(x);
    adf_cadele_init(z);
    adf_rat_init(q);
    build_cadele(x);
    fmpq_init(a);
    fmpq_init(i1);
    fmpq_init(i2);
    fmpq_init(er);
    fmpq_init(ei);
    exact_x(a);
    exact_i1(i1);
    exact_i2(i2);
    fmpq_set_si(q->q, 3, 7);
    adf_cadele_mul_rat(z, x, q, 200);
        fmpq_mul(er, a, q->q);
        fmpq_mul(ei, i1, q->q);
    ADF_CHECK_MSG(fmpq_is_zero(ei) == 0, "the exact imaginary part is zero, so the "
                   "imaginary relative radius is not compared");
    check_precise_acb(z->inf, er, ei, "adf_cadele_mul_rat at prec 200", 1);
        adf_cadele_mul_rat(z, x, q, 2);
    check_contains_acb(z->inf, er, ei, "adf_cadele_mul_rat at prec 2 lost the exact value");

    adf_cadele_clear(x);
    adf_cadele_clear(z);
    adf_rat_clear(q);
    fmpq_clear(a);
    fmpq_clear(i1);
    fmpq_clear(i2);
    fmpq_clear(er);
    fmpq_clear(ei);
}

ADF_TEST(cadele_div_rat_computes_the_real_part_at_the_precision_asked_for)
{
    adf_cadele_t x, z;
    adf_rat_t q;
    fmpq_t a, i1, i2, er, ei;
    adf_cadele_init(x);
    adf_cadele_init(z);
    adf_rat_init(q);
    build_cadele(x);
    fmpq_init(a);
    fmpq_init(i1);
    fmpq_init(i2);
    fmpq_init(er);
    fmpq_init(ei);
    exact_x(a);
    exact_i1(i1);
    exact_i2(i2);
    fmpq_set_si(q->q, 7, 3);
    ADF_CHECK(adf_cadele_div_rat(z, x, q, 200) == ADF_OK);
        fmpq_div(er, a, q->q);
        fmpq_div(ei, i1, q->q);
    ADF_CHECK_MSG(fmpq_is_zero(ei) == 0, "the exact imaginary part is zero, so the "
                   "imaginary relative radius is not compared");
    check_precise_acb(z->inf, er, ei, "adf_cadele_div_rat at prec 200", 1);
        ADF_CHECK(adf_cadele_div_rat(z, x, q, 2) == ADF_OK);
    check_contains_acb(z->inf, er, ei, "adf_cadele_div_rat at prec 2 lost the exact value");

    adf_cadele_clear(x);
    adf_cadele_clear(z);
    adf_rat_clear(q);
    fmpq_clear(a);
    fmpq_clear(i1);
    fmpq_clear(i2);
    fmpq_clear(er);
    fmpq_clear(ei);
}

ADF_TEST(cadele_set_rat_converts_at_the_precision_asked_for)
{
    adf_cadele_t y;
    adf_rat_t q;
    fmpq_t ex;

    adf_cadele_init(y);
    adf_rat_init(q);
    fmpq_set_si(q->q, 3, 7);
    fmpq_init(ex);
    fmpq_set(ex, q->q);

    adf_cadele_set_rat(y, q, 200);
    /* The imaginary part of adf_cadele_set_rat is the exact 0, so only the real part has a
       relative radius to compare. */
    ADF_CHECK_MSG(rel_radius_below(acb_realref(y->inf), 150),
                  "adf_cadele_set_rat at prec 200: the real radius is not below 2^-150 times the "
                  "real midpoint (%ld bits of relative error)",
                  (long) arb_rel_error_bits(acb_realref(y->inf)));
    ADF_CHECK_MSG(arb_is_exact(acb_imagref(y->inf)),
                  "the imaginary part of adf_cadele_set_rat is not the exact 0");
    ADF_CHECK_MSG(acb_contains_fmpq(y->inf, ex),
                  "adf_cadele_set_rat at prec 200 does not contain the exact value");
    adf_cadele_set_rat(y, q, 2);
    ADF_CHECK_MSG(acb_contains_fmpq(y->inf, ex),
                  "adf_cadele_set_rat at prec 2 lost the exact value");

    fmpq_clear(ex);
    adf_rat_clear(q);
    adf_cadele_clear(y);
}

/* ============================== the two swaps ============================== */

ADF_TEST(adele_swap_exchanges_the_finite_half)
{
    adf_adele_t x, y;
    adf_fball_t fx, fy;
    arb_t r;

    adf_adele_init(x);
    adf_adele_init(y);
    adf_fball_init(fx);
    adf_fball_init(fy);
    arb_init(r);
    build_adele(x);
    build_adele_y(y);
    /* Equal real parts, different finite parts: the real coordinate cannot see the exchange. */
    adf_adele_get_real(r, x);
    arb_set(y->inf, r);
    ADF_CHECK_MSG(arb_equal(x->inf, y->inf), "the two real parts are not equal before the swap");
    ADF_CHECK_MSG(adf_fball_identical(&x->fin, &y->fin) == 0,
                  "the two finite halves are equal before the swap");

    adf_adele_get_fin(fx, x);
    adf_adele_get_fin(fy, y);
    adf_adele_swap(x, y);
    ADF_CHECK_MSG(adf_fball_identical(&x->fin, fy),
                  "the finite half of x is not the one y held before the swap");
    ADF_CHECK_MSG(adf_fball_identical(&y->fin, fx),
                  "the finite half of y is not the one x held before the swap");
    ADF_CHECK_MSG(arb_equal(x->inf, y->inf), "the two real parts are no longer equal");

    arb_clear(r);
    adf_fball_clear(fx);
    adf_fball_clear(fy);
    adf_adele_clear(x);
    adf_adele_clear(y);
}

ADF_TEST(cadele_swap_exchanges_the_finite_half)
{
    adf_cadele_t x, y;
    adf_fball_t fx, fy;
    acb_t z;

    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_fball_init(fx);
    adf_fball_init(fy);
    acb_init(z);
    build_cadele(x);
    build_cadele_y(y);
    /* Equal complex parts, different finite parts: the complex coordinate cannot see the
       exchange. */
    adf_cadele_get_complex(z, x);
    acb_set(y->inf, z);
    ADF_CHECK_MSG(acb_equal(x->inf, y->inf), "the two complex parts are not equal before the swap");
    ADF_CHECK_MSG(adf_fball_identical(&x->fin, &y->fin) == 0,
                  "the two finite halves are equal before the swap");

    adf_cadele_get_fin(fx, x);
    adf_cadele_get_fin(fy, y);
    adf_cadele_swap(x, y);
    ADF_CHECK_MSG(adf_fball_identical(&x->fin, fy),
                  "the finite half of x is not the one y held before the swap");
    ADF_CHECK_MSG(adf_fball_identical(&y->fin, fx),
                  "the finite half of y is not the one x held before the swap");
    ADF_CHECK_MSG(acb_equal(x->inf, y->inf), "the two complex parts are no longer equal");

    acb_clear(z);
    adf_fball_clear(fx);
    adf_fball_clear(fy);
    adf_cadele_clear(x);
    adf_cadele_clear(y);
}

ADF_TEST(get_arb_at_the_real_place_returns_ADF_OK)
{
    adf_adele_t x;
    arb_t r;

    adf_adele_init(x);
    arb_init(r);
    build_adele(x);
    ADF_CHECK_MSG(adf_adele_get_arb_at(r, x, adf_place_inf()) == ADF_OK,
                  "the status at the archimedean place is not ADF_OK");
    ADF_CHECK_MSG(arb_equal(r, x->inf), "the coordinate at the real place is not the real part");
    arb_clear(r);
    adf_adele_clear(x);
}
