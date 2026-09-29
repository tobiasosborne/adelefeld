/* tests/test_idele.c: adf_idele (milestone 2, slice 1, lane i-slice1; include/adelefeld/idele.h;
   docs/api-2.md 1.3 Statements D and E; docs/proofs/ideles.md P3, P10, P11; SPEC 5 "Sign preservation").

   The oracles:
   1. Exact rational end points. For a real ball [m +- rho] that excludes 0 the set of absolute values is
      [abs(m) - rho, abs(m) + rho], exactly; the absolute values of the products (inverses) of two such
      sets form [L, H] with L, H exact rationals, computed here with fmpq from the midpoints and radii
      (arf_get_fmpq, arf_set_mag: exact). A result must contain sign * L and sign * H (so the whole
      interval, as a ball is an interval), must exclude 0, and must have the sign of the product.
   2. tests/ref/vectors/i-slice1/idele.jsonl, written by lanes/i-slice1/gen_vectors.py from ref_real_* and
      ref_idele_* of proto/ideles_checks.py. It gives for each case the status, which the reference
      computes exactly (Statement E6: the status depends on the correctly rounded end points only), the
      rounded end points lo, hi that the result must contain as well, the exact end points L, H, the
      content and the unit. Every line is run.
   3. The unit cosets of the results against adf_ucoset_mul and adf_ucoset_inv, whose own test is
      tests/test_ucoset.c (enumeration). */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <flint/arb.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>

#include <adelefeld/idele.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* ---- helpers ---- */

/* The ball [m1 2^e1 +- m2 2^e2], m2 < 2^30, exactly (checked). */
static void
arb_set_exact(arb_t x, const fmpz_t m1, const fmpz_t e1, ulong m2, slong e2)
{
    arf_t t, u;
    arf_init(t);
    arf_init(u);
    arf_set_fmpz_2exp(arb_midref(x), m1, e1);
    mag_set_ui_2exp_si(arb_radref(x), m2, e2);
    arf_set_mag(t, arb_radref(x));
    arf_set_ui(u, m2);
    arf_mul_2exp_si(u, u, e2);
    if (!arf_equal(t, u))
    {
        printf("arb_set_exact: the radius %lu 2^%ld is not held exactly by a mag\n", (unsigned long) m2, (long) e2);
        abort();
    }
    arf_clear(t);
    arf_clear(u);
}

static void
arb_set_si_si(arb_t x, slong m, slong em, ulong r, slong er)
{
    fmpz_t a, b;
    fmpz_init_set_si(a, m);
    fmpz_init_set_si(b, em);
    arb_set_exact(x, a, b, r, er);
    fmpz_clear(a);
    fmpz_clear(b);
}

/* The exact end points of abs(X): [abs(m) - rho, abs(m) + rho] as rationals. */
static void
abs_ends(fmpq_t lo, fmpq_t hi, const arb_t x)
{
    fmpq_t m, r;
    arf_t t;
    fmpq_init(m);
    fmpq_init(r);
    arf_init(t);
    arf_get_fmpq(m, arb_midref(x));
    fmpq_abs(m, m);
    arf_set_mag(t, arb_radref(x));
    arf_get_fmpq(r, t);
    fmpq_sub(lo, m, r);
    fmpq_add(hi, m, r);
    fmpq_clear(m);
    fmpq_clear(r);
    arf_clear(t);
}

static int
arb_sign(const arb_t x)
{
    return arf_sgn(arb_midref(x));
}

/* 1 if z contains s * [L, H] and not 0, and the sign of z is s. */
static int
encloses(const arb_t z, const fmpq_t L, const fmpq_t H, int s)
{
    fmpq_t a, b;
    int ok;
    fmpq_init(a);
    fmpq_init(b);
    fmpq_set(a, L);
    fmpq_set(b, H);
    if (s < 0)
    {
        fmpq_neg(a, a);
        fmpq_neg(b, b);
    }
    ok = arb_contains_fmpq(z, a) && arb_contains_fmpq(z, b) && arb_is_nonzero(z) && arb_sign(z) == s
         && arb_is_finite(z);
    fmpq_clear(a);
    fmpq_clear(b);
    return ok;
}

/* The oracle of the real part of a product: sign and exact ends. */
static int
check_mul_real(const arb_t z, const arb_t x, const arb_t y)
{
    fmpq_t lx, hx, ly, hy;
    int ok;
    fmpq_init(lx);
    fmpq_init(hx);
    fmpq_init(ly);
    fmpq_init(hy);
    abs_ends(lx, hx, x);
    abs_ends(ly, hy, y);
    fmpq_mul(lx, lx, ly);
    fmpq_mul(hx, hx, hy);
    ok = encloses(z, lx, hx, arb_sign(x) * arb_sign(y));
    fmpq_clear(lx);
    fmpq_clear(hx);
    fmpq_clear(ly);
    fmpq_clear(hy);
    return ok;
}

static int
check_inv_real(const arb_t z, const arb_t x)
{
    fmpq_t lx, hx;
    int ok;
    fmpq_init(lx);
    fmpq_init(hx);
    abs_ends(lx, hx, x);
    fmpq_inv(lx, lx);
    fmpq_inv(hx, hx);
    ok = encloses(z, hx, lx, arb_sign(x));
    fmpq_clear(lx);
    fmpq_clear(hx);
    return ok;
}

static void
idele_set_parts_si(adf_idele_t x, const arb_t inf, slong rn, slong rd, slong c, slong N)
{
    fmpq_t r;
    adf_ucoset_t u;
    fmpz_t fc, fN;
    fmpq_init(r);
    adf_ucoset_init(u);
    fmpz_init_set_si(fc, c);
    fmpz_init_set_si(fN, N);
    fmpq_set_si(r, rn, (ulong) rd);
    if (adf_ucoset_set_fmpz2(u, fc, fN) != ADF_OK || adf_idele_set_parts(x, inf, r, u) != ADF_OK)
    {
        printf("idele_set_parts_si: refused\n");
        abort();
    }
    fmpq_clear(r);
    adf_ucoset_clear(u);
    fmpz_clear(fc);
    fmpz_clear(fN);
}

static void
rat_set_si(adf_rat_t q, slong n, slong d)
{
    fmpq_set_si(q->q, n, (ulong) d);
}

/* 1 if the content of x is n/d and its stored unit is (c, N). */
static int
finite_is(const adf_idele_t x, slong n, slong d, slong c, slong N)
{
    fmpq_t r;
    adf_ucoset_t u;
    int ok;
    fmpq_init(r);
    adf_ucoset_init(u);
    adf_idele_content(r, x);
    adf_idele_get_unit(u, x);
    ok = fmpz_equal_si(fmpq_numref(r), n) && fmpz_equal_si(fmpq_denref(r), d) && fmpz_equal_si(u->c, c)
         && fmpz_equal_si(u->N, N);
    fmpq_clear(r);
    adf_ucoset_clear(u);
    return ok;
}

/* ---- JSON helpers ---- */

static const jsonl_value *
field(const jsonl_value * rec, const char * key)
{
    const jsonl_value * v;
    jsonl_error_t err;
    if (!jsonl_field(rec, key, &v, &err))
    {
        printf("field %s: %s\n", key, jsonl_error_message(&err));
        abort();
    }
    return v;
}

static void
read_fmpz_at(fmpz_t z, const jsonl_value * arr, size_t i)
{
    jsonl_error_t err;
    const char * t;
    if (!jsonl_int_text_or_string(jsonl_at(arr, i, &err), &t, &err) || fmpz_set_str(z, t, 10) != 0)
        abort();
}

static slong
read_si(const jsonl_value * v)
{
    fmpz_t z;
    jsonl_error_t err;
    const char * t;
    slong s;
    if (!jsonl_int_text_or_string(v, &t, &err))
        abort();
    fmpz_init(z);
    fmpz_set_str(z, t, 10);
    s = fmpz_get_si(z);
    fmpz_clear(z);
    return s;
}

static void
read_fmpq(fmpq_t q, const jsonl_value * v)
{
    read_fmpz_at(fmpq_numref(q), v, 0);
    read_fmpz_at(fmpq_denref(q), v, 1);
}

static void
read_arf(arf_t x, const jsonl_value * v)
{
    fmpz_t m, e;
    fmpz_init(m);
    fmpz_init(e);
    read_fmpz_at(m, v, 0);
    read_fmpz_at(e, v, 1);
    arf_set_fmpz_2exp(x, m, e);
    fmpz_clear(m);
    fmpz_clear(e);
}

static void
read_coset(adf_ucoset_t u, const jsonl_value * v)
{
    read_fmpz_at(u->c, v, 0);
    read_fmpz_at(u->N, v, 1);
}

static void
read_idele(adf_idele_t x, const jsonl_value * v)
{
    fmpz_t m1, e1, m2, e2;
    adf_ucoset_t u;
    fmpq_t r;
    arb_t inf;
    fmpz_init(m1);
    fmpz_init(e1);
    fmpz_init(m2);
    fmpz_init(e2);
    adf_ucoset_init(u);
    fmpq_init(r);
    arb_init(inf);
    read_fmpz_at(m1, field(v, "mid"), 0);
    read_fmpz_at(e1, field(v, "mid"), 1);
    read_fmpz_at(m2, field(v, "rad"), 0);
    read_fmpz_at(e2, field(v, "rad"), 1);
    arb_set_exact(inf, m1, e1, fmpz_get_ui(m2), fmpz_get_si(e2));
    read_fmpq(r, field(v, "r"));
    read_coset(u, field(v, "u"));
    if (!adf_ucoset_is_canonical(u) || adf_idele_set_parts(x, inf, r, u) != ADF_OK || !adf_idele_is_canonical(x))
    {
        printf("read_idele: the input is not an idele\n");
        abort();
    }
    fmpz_clear(m1);
    fmpz_clear(e1);
    fmpz_clear(m2);
    fmpz_clear(e2);
    adf_ucoset_clear(u);
    fmpq_clear(r);
    arb_clear(inf);
}

static int
status_of(const char * s)
{
    if (strcmp(s, "OK") == 0)
        return ADF_OK;
    if (strcmp(s, "NOT_DETERMINED") == 0)
        return ADF_NOT_DETERMINED;
    if (strcmp(s, "NOT_UNIT") == 0)
        return ADF_NOT_UNIT;
    if (strcmp(s, "DOMAIN") == 0)
        return ADF_DOMAIN;
    abort();
}

static const char *
str_of(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    size_t len;
    const char * s = jsonl_string(field(rec, key), &len, &err);
    if (s == NULL)
        abort();
    return s;
}

/* ---- layout and life cycle ---- */

ADF_TEST(layout_is_80_bytes)
{
    ADF_CHECK(adf_sizeof_idele() == 80);
    ADF_CHECK(adf_alignof_idele() == 8);
    ADF_CHECK(offsetof(adf_idele_struct, inf) == 0);
    ADF_CHECK(offsetof(adf_idele_struct, r) == 48);
    ADF_CHECK(offsetof(adf_idele_struct, u) == 64);
}

ADF_TEST(init_is_the_exact_idele_one)
{
    adf_idele_t x;
    adf_idele_init(x);
    ADF_CHECK(adf_idele_is_canonical(x));
    ADF_CHECK(arb_is_one(x->inf));
    ADF_CHECK(fmpq_is_one(x->r));
    ADF_CHECK(fmpz_is_one(x->u.c) && fmpz_is_zero(x->u.N));
    adf_idele_clear(x);
}

ADF_TEST(is_canonical_is_the_predicate_of_conventions_5_7)
{
    adf_idele_t x;
    adf_idele_init(x);
    arb_set_si_si(x->inf, 1, 0, 1, -1);                   /* 1 +- 1/2 */
    ADF_CHECK(adf_idele_is_canonical(x));
    arb_set_si_si(x->inf, 1, 0, 1, 0);                    /* 1 +- 1 contains 0 */
    ADF_CHECK(!adf_idele_is_canonical(x));
    arb_zero(x->inf);
    ADF_CHECK(!adf_idele_is_canonical(x));
    arb_pos_inf(x->inf);
    ADF_CHECK(!adf_idele_is_canonical(x));
    arb_indeterminate(x->inf);
    ADF_CHECK(!adf_idele_is_canonical(x));
    arb_set_si(x->inf, -7);
    ADF_CHECK(adf_idele_is_canonical(x));
    fmpz_set_si(fmpq_numref(x->r), 6);
    fmpz_set_si(fmpq_denref(x->r), 4);                    /* not canonical */
    ADF_CHECK(!adf_idele_is_canonical(x));
    fmpz_set_si(fmpq_denref(x->r), 0);                    /* zero denominator: 0, no abort */
    ADF_CHECK(!adf_idele_is_canonical(x));
    fmpq_set_si(x->r, -1, 2);
    ADF_CHECK(!adf_idele_is_canonical(x));
    fmpq_zero(x->r);
    ADF_CHECK(!adf_idele_is_canonical(x));
    fmpq_set_si(x->r, 3, 2);
    ADF_CHECK(adf_idele_is_canonical(x));
    fmpz_set_si(x->u.c, 2);
    fmpz_set_si(x->u.N, 4);                               /* not a unit coset */
    ADF_CHECK(!adf_idele_is_canonical(x));
    fmpz_set_si(x->u.c, 5);
    fmpz_set_si(x->u.N, 6);                               /* canonical, not normal: allowed */
    ADF_CHECK(adf_idele_is_canonical(x));
    adf_idele_clear(x);
}

ADF_TEST(set_parts_and_its_refusals)
{
    adf_idele_t x, keep;
    arb_t inf;
    fmpq_t r, g;
    adf_ucoset_t u, gu;
    fmpz_t c, N;

    adf_idele_init(x);
    adf_idele_init(keep);
    arb_init(inf);
    fmpq_init(r);
    fmpq_init(g);
    adf_ucoset_init(u);
    adf_ucoset_init(gu);
    fmpz_init_set_si(c, 5);
    fmpz_init_set_si(N, 6);
    ADF_CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);

    /* 6/4 is canonicalised to 3/2; the unit (5, 6) is kept as supplied (CV-17) */
    arb_set_si_si(inf, 5, -1, 1, -30);                    /* 2.5 +- 2^-30 */
    fmpz_set_si(fmpq_numref(r), 6);
    fmpz_set_si(fmpq_denref(r), 4);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_OK);
    ADF_CHECK(adf_idele_is_canonical(x) && finite_is(x, 3, 2, 5, 6));
    ADF_CHECK(arb_equal(x->inf, inf));
    adf_idele_get_real(inf, x);
    ADF_CHECK(arb_equal(x->inf, inf));
    adf_idele_content(g, x);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(g), 3) && fmpz_equal_si(fmpq_denref(g), 2));
    adf_idele_get_unit(gu, x);
    ADF_CHECK(adf_ucoset_identical(gu, u));
    /* a negative denominator is canonicalised too: 3/(-2) is -3/2 <= 0, refused below; -3/-2 = 3/2 */
    fmpz_set_si(fmpq_numref(r), -3);
    fmpz_set_si(fmpq_denref(r), -2);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_OK && finite_is(x, 3, 2, 5, 6));

    adf_idele_set(keep, x);
    /* refusals, x untouched */
    fmpq_set_si(r, 1, 1);
    arb_zero(inf);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_DOMAIN && adf_idele_identical(x, keep));
    arb_set_si_si(inf, 1, 0, 1, 0);                       /* 1 +- 1 */
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_DOMAIN && adf_idele_identical(x, keep));
    arb_set_si_si(inf, 1, 0, 2, 0);                       /* 1 +- 2 */
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_DOMAIN && adf_idele_identical(x, keep));
    arb_pos_inf(inf);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_DOMAIN && adf_idele_identical(x, keep));
    arb_indeterminate(inf);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_DOMAIN && adf_idele_identical(x, keep));
    arb_set_si(inf, 1);
    fmpq_zero(r);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_DOMAIN && adf_idele_identical(x, keep));
    fmpq_set_si(r, -1, 7);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_DOMAIN && adf_idele_identical(x, keep));
    fmpz_set_si(fmpq_numref(r), 3);
    fmpz_set_si(fmpq_denref(r), -2);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_DOMAIN && adf_idele_identical(x, keep));
    fmpz_set_si(fmpq_numref(r), 1);
    fmpz_set_si(fmpq_denref(r), 0);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_DOMAIN && adf_idele_identical(x, keep));
    /* identical and swap */
    adf_idele_init(keep);
    ADF_CHECK(!adf_idele_identical(x, keep));
    adf_idele_swap(x, keep);
    ADF_CHECK(finite_is(keep, 3, 2, 5, 6) && finite_is(x, 1, 1, 1, 0));
    adf_idele_set(x, x);
    ADF_CHECK(finite_is(x, 1, 1, 1, 0) && arb_is_one(x->inf));

    fmpz_clear(c);
    fmpz_clear(N);
    arb_clear(inf);
    fmpq_clear(r);
    fmpq_clear(g);
    adf_ucoset_clear(u);
    adf_ucoset_clear(gu);
    adf_idele_clear(x);
    adf_idele_clear(keep);
}

/* ---- the idele of a rational ---- */

ADF_TEST(set_rat_negative_exact_and_zero)
{
    adf_idele_t x, keep;
    adf_rat_t q;
    arb_t t;
    slong p;

    adf_idele_init(x);
    adf_idele_init(keep);
    adf_rat_init(q);
    arb_init(t);

    /* -3/2: dyadic, exact at every prec >= 2 (two bits) */
    rat_set_si(q, -3, 2);
    for (p = 0; p <= 4; p++)
    {
        ADF_CHECK(adf_idele_set_rat(x, q, p) == ADF_OK);
        ADF_CHECK(adf_idele_is_canonical(x) && finite_is(x, 3, 2, -1, 0));
        ADF_CHECK(arb_is_exact(x->inf) && arb_contains_fmpq(x->inf, q->q));
    }
    /* 5 needs 3 bits: at prec 2 a ball around 5, at prec 3 exact; prec 0 and 1 are prec 2 (M1-D4) */
    rat_set_si(q, 5, 1);
    ADF_CHECK(adf_idele_set_rat(x, q, 2) == ADF_OK);
    ADF_CHECK(!arb_is_exact(x->inf) && arb_contains_fmpq(x->inf, q->q) && arb_is_positive(x->inf));
    ADF_CHECK(finite_is(x, 5, 1, 1, 0));
    adf_idele_set(keep, x);
    ADF_CHECK(adf_idele_set_rat(x, q, 0) == ADF_OK && adf_idele_identical(x, keep));
    ADF_CHECK(adf_idele_set_rat(x, q, 1) == ADF_OK && adf_idele_identical(x, keep));
    ADF_CHECK(adf_idele_set_rat(x, q, -5) == ADF_OK && adf_idele_identical(x, keep));
    ADF_CHECK(adf_idele_set_rat(x, q, 3) == ADF_OK && arb_is_exact(x->inf));
    /* -1/3: never exact, negative ball, unit [-1], content 1/3 */
    rat_set_si(q, -1, 3);
    for (p = 0; p <= 200; p += 7)
    {
        ADF_CHECK(adf_idele_set_rat(x, q, p) == ADF_OK);
        ADF_CHECK(arb_contains_fmpq(x->inf, q->q) && arb_is_negative(x->inf) && finite_is(x, 1, 3, -1, 0));
    }
    /* q = 0: NOT_UNIT, x untouched */
    adf_idele_set(keep, x);
    fmpq_zero(q->q);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_NOT_UNIT && adf_idele_identical(x, keep));
    /* a huge rational: (2^3000 + 1)/3^1200 */
    fmpz_one(fmpq_numref(q->q));
    fmpz_mul_2exp(fmpq_numref(q->q), fmpq_numref(q->q), 3000);
    fmpz_add_ui(fmpq_numref(q->q), fmpq_numref(q->q), 1);
    fmpz_set_ui(fmpq_denref(q->q), 3);
    fmpz_pow_ui(fmpq_denref(q->q), fmpq_denref(q->q), 1200);
    fmpq_neg(q->q, q->q);
    ADF_CHECK(adf_idele_set_rat(x, q, 53) == ADF_OK);
    ADF_CHECK(arb_contains_fmpq(x->inf, q->q) && arb_is_negative(x->inf));
    ADF_CHECK(arb_rel_accuracy_bits(x->inf) >= 50);
    fmpq_neg(q->q, q->q);
    ADF_CHECK(fmpq_equal(x->r, q->q) && fmpz_equal_si(x->u.c, -1) && fmpz_is_zero(x->u.N));
    /* a dyadic of many bits with a huge exponent: 3 2^-100000, exact at prec 2 */
    fmpq_set_si(q->q, 3, 1);
    fmpz_one(fmpq_denref(q->q));
    fmpz_mul_2exp(fmpq_denref(q->q), fmpq_denref(q->q), 100000);
    ADF_CHECK(adf_idele_set_rat(x, q, 2) == ADF_OK && arb_is_exact(x->inf) && arb_contains_fmpq(x->inf, q->q));

    arb_clear(t);
    adf_rat_clear(q);
    adf_idele_clear(x);
    adf_idele_clear(keep);
}

/* ---- the example of SPEC 5: the kernel keeps the sign where arb_mul does not ---- */

ADF_TEST(spec5_sign_preservation)
{
    adf_idele_t x, z, keep;
    arb_t t;
    fmpq_t L, H;
    slong p;
    static const slong precs[] = { 64, 128, 1024, 100000 };
    size_t i;

    adf_idele_init(x);
    adf_idele_init(z);
    adf_idele_init(keep);
    arb_init(t);
    fmpq_init(L);
    fmpq_init(H);
    /* x = 1 +- (1 - 2^-30), exact: the radius (2^30 - 1) 2^-30 has 30 bits */
    arb_set_si_si(t, 1, 0, (UWORD(1) << 30) - 1, -30);
    idele_set_parts_si(x, t, 1, 1, 1, 0);
    ADF_CHECK(arb_is_nonzero(x->inf));
    /* the ball product of arb contains 0 at every precision (lane d-ideles, finding 3) */
    for (i = 0; i < sizeof(precs) / sizeof(precs[0]); i++)
    {
        arb_mul(t, x->inf, x->inf, precs[i]);
        ADF_CHECK_MSG(arb_contains_zero(t), "arb_mul at %ld", (long) precs[i]);
    }
    /* the product set is [2^-60, (2 - 2^-30)^2] */
    fmpq_set_si(L, 1, 1);
    fmpz_mul_2exp(fmpq_denref(L), fmpq_denref(L), 60);
    fmpq_set_si(H, (WORD(1) << 31) - 1, UWORD(1) << 30);
    fmpq_mul(H, H, H);
    /* the reference: NOT_DETERMINED for p <= 60, OK for 61 <= p (proto/ideles_checks.py) */
    for (p = 2; p <= 140; p++)
    {
        adf_idele_set(z, keep);
        int st = adf_idele_mul(z, x, x, p);
        if (p <= 60)
            ADF_CHECK_MSG(st == ADF_NOT_DETERMINED && adf_idele_identical(z, keep), "prec %ld", (long) p);
        else
        {
            ADF_CHECK_MSG(st == ADF_OK, "prec %ld", (long) p);
            ADF_CHECK_MSG(encloses(z->inf, L, H, 1) && adf_idele_is_canonical(z), "prec %ld", (long) p);
        }
    }
    ADF_CHECK(adf_idele_mul(z, x, x, 100000) == ADF_OK && encloses(z->inf, L, H, 1));
    /* the same with signs: (-x) * x is negative, (-x) * (-x) positive */
    adf_idele_set(keep, x);
    arb_neg(x->inf, x->inf);
    adf_ucoset_minus_one(&x->u);
    ADF_CHECK(adf_idele_mul(z, x, keep, 128) == ADF_OK && encloses(z->inf, L, H, -1));
    ADF_CHECK(finite_is(z, 1, 1, -1, 0));
    ADF_CHECK(adf_idele_mul(z, x, x, 128) == ADF_OK && encloses(z->inf, L, H, 1) && finite_is(z, 1, 1, 1, 0));
    fmpq_clear(L);
    fmpq_clear(H);
    arb_clear(t);
    adf_idele_clear(x);
    adf_idele_clear(z);
    adf_idele_clear(keep);
}

/* ---- product and inverse: hand cases ---- */

ADF_TEST(product_of_the_ideles_of_rationals)
{
    adf_idele_t x, y, z;
    adf_rat_t q;
    fmpq_t one;

    adf_idele_init(x);
    adf_idele_init(y);
    adf_idele_init(z);
    adf_rat_init(q);
    fmpq_init(one);
    fmpq_one(one);
    /* (-3/2) * (-2/3) = 1: content 1, unit [1], a real ball around 1, positive */
    rat_set_si(q, -3, 2);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    rat_set_si(q, -2, 3);
    ADF_CHECK(adf_idele_set_rat(y, q, 64) == ADF_OK);
    ADF_CHECK(adf_idele_mul(z, x, y, 64) == ADF_OK);
    ADF_CHECK(finite_is(z, 1, 1, 1, 0) && arb_contains_fmpq(z->inf, one) && arb_is_positive(z->inf));
    ADF_CHECK(check_mul_real(z->inf, x->inf, y->inf));
    /* 6 * (-5/7) = -30/7: content 30/7, unit [-1] */
    rat_set_si(q, 6, 1);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    rat_set_si(q, -5, 7);
    ADF_CHECK(adf_idele_set_rat(y, q, 64) == ADF_OK);
    ADF_CHECK(adf_idele_mul(z, x, y, 64) == ADF_OK);
    rat_set_si(q, -30, 7);
    ADF_CHECK(finite_is(z, 30, 7, -1, 0) && arb_contains_fmpq(z->inf, q->q) && arb_is_negative(z->inf));
    /* exact inputs whose product fits: 3 * (-5) = -15 exactly at prec 4, a ball at prec 3 */
    rat_set_si(q, 3, 1);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    rat_set_si(q, -5, 1);
    ADF_CHECK(adf_idele_set_rat(y, q, 64) == ADF_OK);
    ADF_CHECK(adf_idele_mul(z, x, y, 4) == ADF_OK && arb_is_exact(z->inf) && arb_equal_si(z->inf, -15));
    ADF_CHECK(adf_idele_mul(z, x, y, 3) == ADF_OK && !arb_is_exact(z->inf) && arb_contains_si(z->inf, -15)
              && arb_is_negative(z->inf));
    /* the inverse of -5: -1/5, content 1/5, unit [-1] */
    ADF_CHECK(adf_idele_inv(z, y, 64) == ADF_OK);
    rat_set_si(q, -1, 5);
    ADF_CHECK(finite_is(z, 1, 5, -1, 0) && arb_contains_fmpq(z->inf, q->q) && check_inv_real(z->inf, y->inf));
    /* the inverse of -1/4 is -4 exactly */
    rat_set_si(q, -1, 4);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    ADF_CHECK(adf_idele_inv(z, x, 2) == ADF_OK && arb_is_exact(z->inf) && arb_equal_si(z->inf, -4));
    ADF_CHECK(finite_is(z, 4, 1, -1, 0));
    fmpq_clear(one);
    adf_rat_clear(q);
    adf_idele_clear(x);
    adf_idele_clear(y);
    adf_idele_clear(z);
}

ADF_TEST(the_point_one_is_in_x_times_x_inverse)
{
    adf_idele_t x, y, z;
    arb_t t;
    fmpq_t one;
    adf_ucoset_t e;

    adf_idele_init(x);
    adf_idele_init(y);
    adf_idele_init(z);
    arb_init(t);
    fmpq_init(one);
    adf_ucoset_init(e);
    fmpq_one(one);
    /* x = (-3 +- 1 ; 3/2 * [5 mod 12]) */
    arb_set_si_si(t, -3, 0, 1, 0);
    idele_set_parts_si(x, t, 3, 2, 5, 12);
    ADF_CHECK(adf_idele_inv(y, x, 64) == ADF_OK);
    ADF_CHECK(finite_is(y, 2, 3, 5, 12) && arb_is_negative(y->inf) && check_inv_real(y->inf, x->inf));
    ADF_CHECK(adf_idele_mul(z, x, y, 64) == ADF_OK);
    ADF_CHECK(finite_is(z, 1, 1, 1, 12));                 /* U(12): contains 1, is not the exact 1 */
    ADF_CHECK(arb_contains_fmpq(z->inf, one) && !arb_is_exact(z->inf) && arb_is_positive(z->inf));
    ADF_CHECK(adf_ucoset_contains(e, &z->u));
    /* with the modulus 6 (not normal): the results are in normal form, U(3) */
    arb_set_si_si(t, 7, -2, 1, -3);                       /* 1.75 +- 0.125 */
    idele_set_parts_si(x, t, 5, 9, 5, 6);
    ADF_CHECK(adf_idele_inv(y, x, 30) == ADF_OK && finite_is(y, 9, 5, 2, 3));
    ADF_CHECK(adf_idele_mul(z, x, y, 30) == ADF_OK && finite_is(z, 1, 1, 1, 3));
    ADF_CHECK(arb_contains_fmpq(z->inf, one) && adf_ucoset_contains(e, &z->u));
    /* mixed moduli: [5 mod 12] * [7 mod 18] = [2 mod 3] */
    idele_set_parts_si(x, t, 5, 9, 5, 12);
    idele_set_parts_si(y, t, 2, 1, 7, 18);
    ADF_CHECK(adf_idele_mul(z, x, y, 30) == ADF_OK && finite_is(z, 10, 9, 2, 3));
    ADF_CHECK(check_mul_real(z->inf, x->inf, y->inf));
    adf_ucoset_clear(e);
    fmpq_clear(one);
    arb_clear(t);
    adf_idele_clear(x);
    adf_idele_clear(y);
    adf_idele_clear(z);
}

ADF_TEST(not_determined_leaves_the_output_untouched)
{
    adf_idele_t x, y, z, keep;
    arb_t t;

    adf_idele_init(x);
    adf_idele_init(y);
    adf_idele_init(z);
    adf_idele_init(keep);
    arb_init(t);
    /* x = 1 +- (1 - 2^-29): |x| in [2^-29, 2 - 2^-29]; the ends of 1/|x| and of x^2 are 2^29 apart or more */
    arb_set_si_si(t, 1, 0, (UWORD(1) << 29) - 1, -29);
    idele_set_parts_si(x, t, 7, 3, 5, 12);
    arb_set_si(t, 3);
    idele_set_parts_si(z, t, 1, 1, 1, 1);
    adf_idele_set(keep, z);
    ADF_CHECK(adf_idele_inv(z, x, 16) == ADF_NOT_DETERMINED && adf_idele_identical(z, keep));
    ADF_CHECK(adf_idele_mul(z, x, x, 16) == ADF_NOT_DETERMINED && adf_idele_identical(z, keep));
    ADF_CHECK(adf_idele_inv(z, x, 64) == ADF_OK && check_inv_real(z->inf, x->inf));
    /* aliased output, NOT_DETERMINED: x untouched */
    adf_idele_set(keep, x);
    ADF_CHECK(adf_idele_inv(x, x, 16) == ADF_NOT_DETERMINED && adf_idele_identical(x, keep));
    ADF_CHECK(adf_idele_mul(x, x, x, 16) == ADF_NOT_DETERMINED && adf_idele_identical(x, keep));
    adf_idele_set(y, x);
    ADF_CHECK(adf_idele_mul(y, x, y, 16) == ADF_NOT_DETERMINED && adf_idele_identical(y, keep));
    arb_clear(t);
    adf_idele_clear(x);
    adf_idele_clear(y);
    adf_idele_clear(z);
    adf_idele_clear(keep);
}

ADF_TEST(aliasing_of_mul_and_inv)
{
    adf_idele_t x, y, z, a, b;
    arb_t t;
    slong i;

    adf_idele_init(x);
    adf_idele_init(y);
    adf_idele_init(z);
    adf_idele_init(a);
    adf_idele_init(b);
    arb_init(t);
    for (i = 0; i < 8; i++)
    {
        arb_set_si_si(t, (i & 1) ? -3 - i : 5 + 2 * i, -1, (UWORD(1) << (i + 3)) - 1, -(i + 4));
        idele_set_parts_si(x, t, 3 + i, 2, (i & 2) ? 5 : 7, 12);
        arb_set_si_si(t, (i & 4) ? 11 : -13, -2, 5, -8);
        idele_set_parts_si(y, t, 5, 7 + i, 5, (i & 1) ? 18 : 9);
        ADF_CHECK(adf_idele_mul(z, x, y, 53) == ADF_OK);
        adf_idele_set(a, x);
        adf_idele_set(b, y);
        ADF_CHECK(adf_idele_mul(a, a, b, 53) == ADF_OK && adf_idele_identical(a, z));
        adf_idele_set(a, x);
        ADF_CHECK(adf_idele_mul(b, a, b, 53) == ADF_OK && adf_idele_identical(b, z));
        ADF_CHECK(adf_idele_mul(z, x, x, 53) == ADF_OK);
        adf_idele_set(a, x);
        ADF_CHECK(adf_idele_mul(a, a, a, 53) == ADF_OK && adf_idele_identical(a, z));
        ADF_CHECK(check_mul_real(z->inf, x->inf, x->inf));
        ADF_CHECK(adf_idele_inv(z, x, 53) == ADF_OK);
        adf_idele_set(a, x);
        ADF_CHECK(adf_idele_inv(a, a, 53) == ADF_OK && adf_idele_identical(a, z));
    }
    arb_clear(t);
    adf_idele_clear(x);
    adf_idele_clear(y);
    adf_idele_clear(z);
    adf_idele_clear(a);
    adf_idele_clear(b);
}

ADF_TEST(prec_below_2_is_2_and_huge_exponents)
{
    adf_idele_t x, y, z, w;
    arb_t t;
    fmpz_t m, e;
    arf_t want;

    adf_idele_init(x);
    adf_idele_init(y);
    adf_idele_init(z);
    adf_idele_init(w);
    arb_init(t);
    fmpz_init(m);
    fmpz_init(e);
    arf_init(want);
    arb_set_si_si(t, 7, -3, 1, -5);
    idele_set_parts_si(x, t, 1, 1, 1, 1);
    arb_set_si_si(t, -5, 0, 1, -2);
    idele_set_parts_si(y, t, 1, 1, 1, 1);
    ADF_CHECK(adf_idele_mul(w, x, y, 2) == ADF_OK);
    ADF_CHECK(adf_idele_mul(z, x, y, 1) == ADF_OK && adf_idele_identical(z, w));
    ADF_CHECK(adf_idele_mul(z, x, y, 0) == ADF_OK && adf_idele_identical(z, w));
    ADF_CHECK(adf_idele_mul(z, x, y, WORD_MIN) == ADF_OK && adf_idele_identical(z, w));
    ADF_CHECK(adf_idele_inv(w, y, 2) == ADF_OK && adf_idele_inv(z, y, -1) == ADF_OK && adf_idele_identical(z, w));
    /* exponents beyond a word are FLINT's: 3 2^(10^21) squared is 9 2^(2 10^21), exactly at prec 4 */
    fmpz_set_si(m, 3);
    fmpz_set_si(e, 1000000000);
    fmpz_mul_ui(e, e, 1000000000000);                      /* 10^21: not a word */
    arb_set_exact(t, m, e, 0, 0);
    idele_set_parts_si(x, t, 1, 1, 1, 1);
    ADF_CHECK(adf_idele_mul(z, x, x, 4) == ADF_OK && arb_is_exact(z->inf));
    fmpz_set_si(m, 9);
    fmpz_mul_ui(e, e, 2);
    arf_set_fmpz_2exp(want, m, e);
    ADF_CHECK(arf_equal(arb_midref(z->inf), want));
    /* the inverse 2^(-10^21)/3: z * 3 * 2^(10^21) must contain 1 (arf_get_fmpq cannot hold this exponent,
       arf.rst:310-314, so the exact ends of check_inv_real are not used here) */
    ADF_CHECK(adf_idele_inv(z, x, 64) == ADF_OK && arb_is_positive(z->inf));
    fmpz_fdiv_q_2exp(e, e, 1);
    arb_mul_ui(t, z->inf, 3, 1000);
    arb_mul_2exp_fmpz(t, t, e);
    ADF_CHECK(arb_contains_si(t, 1) && arb_rel_accuracy_bits(z->inf) >= 60);
    fmpz_mul_ui(e, e, 2);
    /* a wide ball with a huge exponent: 2^(10^21) (1 +- (1 - 2^-20)) */
    fmpz_set_si(m, 1);
    fmpz_fdiv_q_2exp(e, e, 1);
    arb_set_exact(t, m, e, 0, 0);
    mag_set_ui_2exp_si(arb_radref(t), (UWORD(1) << 20) - 1, -20);
    mag_mul_2exp_fmpz(arb_radref(t), arb_radref(t), e);
    idele_set_parts_si(x, t, 1, 1, 1, 1);
    ADF_CHECK(adf_idele_mul(z, x, x, 64) == ADF_OK && arb_is_positive(z->inf));
    ADF_CHECK(adf_idele_inv(z, x, 64) == ADF_OK && arb_is_positive(z->inf));
    ADF_CHECK(adf_idele_mul(z, x, x, 30) == ADF_NOT_DETERMINED);
    arf_clear(want);
    fmpz_clear(m);
    fmpz_clear(e);
    arb_clear(t);
    adf_idele_clear(x);
    adf_idele_clear(y);
    adf_idele_clear(z);
    adf_idele_clear(w);
}

/* ---- oracle 2: the vectors ---- */

ADF_TEST(vectors_of_the_reference)
{
    jsonl_file * f;
    jsonl_error_t err;
    size_t i, n_rat = 0, n_mul = 0, n_inv = 0, n_nd = 0, n_exact = 0;
    adf_idele_t x, y, z, keep;
    adf_rat_t q;
    adf_ucoset_t u;
    fmpq_t r, L, H;
    arf_t lo, hi;
    slong maxbits_excess = WORD_MIN;

    if (!jsonl_open("tests/ref/vectors/i-slice1/idele.jsonl", &f, &err))
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    adf_idele_init(x);
    adf_idele_init(y);
    adf_idele_init(z);
    adf_idele_init(keep);
    adf_rat_init(q);
    adf_ucoset_init(u);
    fmpq_init(r);
    fmpq_init(L);
    fmpq_init(H);
    arf_init(lo);
    arf_init(hi);
    arb_set_si(keep->inf, 3);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * op = str_of(rec, "op");
        slong prec = read_si(field(rec, "prec"));
        int want = status_of(str_of(rec, "status")), st;

        adf_idele_set(z, keep);
        if (strcmp(op, "set_rat") == 0)
        {
            read_fmpq(q->q, field(rec, "q"));
            st = adf_idele_set_rat(z, q, prec);
            n_rat++;
        }
        else if (strcmp(op, "mul") == 0)
        {
            read_idele(x, field(rec, "x"));
            read_idele(y, field(rec, "y"));
            st = adf_idele_mul(z, x, y, prec);
            if (st == ADF_OK)
                ADF_CHECK_MSG(check_mul_real(z->inf, x->inf, y->inf), "line %zu", i + 1);
            n_mul++;
        }
        else if (strcmp(op, "inv") == 0)
        {
            read_idele(x, field(rec, "x"));
            st = adf_idele_inv(z, x, prec);
            if (st == ADF_OK)
                ADF_CHECK_MSG(check_inv_real(z->inf, x->inf), "line %zu", i + 1);
            n_inv++;
        }
        else
        {
            ADF_CHECK_MSG(0, "unknown op %s on line %zu", op, i + 1);
            continue;
        }
        ADF_CHECK_MSG(st == want, "line %zu: status %d, want %d", i + 1, st, want);
        if (st != ADF_OK || want != ADF_OK)
        {
            ADF_CHECK_MSG(adf_idele_identical(z, keep), "line %zu: output touched", i + 1);
            n_nd += (want == ADF_NOT_DETERMINED);
            continue;
        }
        /* the result: canonical, the rounded ends and the exact ends inside, the sign, the size */
        {
            slong s = read_si(field(rec, "sign")), p = prec < 2 ? 2 : prec, excess;
            read_arf(lo, field(rec, "lo"));
            read_arf(hi, field(rec, "hi"));
            read_fmpq(L, field(rec, "L"));
            read_fmpq(H, field(rec, "H"));
            ADF_CHECK_MSG(adf_idele_is_canonical(z) && encloses(z->inf, L, H, (int) s), "line %zu", i + 1);
            if (s < 0)
            {
                arf_neg(lo, lo);
                arf_neg(hi, hi);
            }
            ADF_CHECK_MSG(arb_contains_arf(z->inf, lo) && arb_contains_arf(z->inf, hi), "line %zu", i + 1);
            if (arf_equal(lo, hi))
            {
                ADF_CHECK_MSG(arb_is_exact(z->inf) && arf_equal(arb_midref(z->inf), lo), "line %zu", i + 1);
                n_exact++;
            }
            excess = arf_bits(arb_midref(z->inf)) - (2 * p + 30);
            if (excess > maxbits_excess)
                maxbits_excess = excess;
            read_fmpq(r, field(rec, "r"));
            read_fmpz_at(u->c, field(rec, "u"), 0);
            read_fmpz_at(u->N, field(rec, "u"), 1);
            ADF_CHECK_MSG(fmpq_equal(z->r, r) && adf_ucoset_identical(&z->u, u), "line %zu", i + 1);
        }
        /* aliasing: the same call with the output equal to the first input */
        if (strcmp(op, "mul") == 0)
            ADF_CHECK(adf_idele_mul(x, x, y, prec) == ADF_OK && adf_idele_identical(x, z));
        else if (strcmp(op, "inv") == 0)
            ADF_CHECK(adf_idele_inv(x, x, prec) == ADF_OK && adf_idele_identical(x, z));
    }
    printf("vectors: %zu lines: %zu set_rat, %zu mul, %zu inv; %zu NOT_DETERMINED, %zu exact results; "
           "largest midpoint minus (2 p + 30) bits: %ld\n", jsonl_count(f), n_rat, n_mul, n_inv, n_nd, n_exact,
           (long) maxbits_excess);
    ADF_CHECK(maxbits_excess <= 0);
    ADF_CHECK(n_rat + n_mul + n_inv == jsonl_count(f) && n_nd > 100);
    jsonl_close(f);
    arf_clear(lo);
    arf_clear(hi);
    fmpq_clear(r);
    fmpq_clear(L);
    fmpq_clear(H);
    adf_ucoset_clear(u);
    adf_rat_clear(q);
    adf_idele_clear(x);
    adf_idele_clear(y);
    adf_idele_clear(z);
    adf_idele_clear(keep);
}
