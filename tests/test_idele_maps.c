/* tests/test_idele_maps.c: the maps of an idele of slice 2 (lane i-slice2; include/adelefeld/idele.h:
   adf_idele_mul_rat, adf_idele_valuation_at, adf_idele_abs_at, adf_idele_abs_inf, adf_idele_norm;
   docs/api-2.md 2.3, Statements F, H, I; docs/proofs/ideles.md P3, P14).

   The oracles:
   1. Exact rational arithmetic (fmpq). For a real ball [m +- rho] that excludes 0 the set of absolute values
      is [abs(m) - rho, abs(m) + rho] exactly; times abs(q), or divided by r, it is [L, H] with L, H exact
      rationals. A result must contain sign * L and sign * H, exclude 0 and have the sign of the product.
   2. Valuations by a loop of exact divisibility tests (fmpz_divisible, one power of p at a time), not by
      fmpz_remove, which the library uses; the absolute value as an exact rational p^(-v); the product
      formula: the product of |r|_p over the primes of r is exactly 1/r, and the norm of the idele of a
      rational contains 1, exactly 1 when its real ball is exact.
   3. tests/ref/vectors/i-slice2/idele_maps.jsonl, written by lanes/i-slice2/gen_vectors.py from part 3 of
      proto/ideles_checks.py (checked there against exact points and trial division). It gives the status,
      which the reference computes exactly (the status depends on the correctly rounded end points only,
      Statement F.4), the rounded end points lo, hi, the exact end points L, H, the content and the unit.
      Every line is run. */

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
        printf("arb_set_exact: the radius is not held exactly by a mag\n");
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

/* The exact end points of abs(X). */
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

/* 1 if z contains s * [L, H] and not 0, is finite, and has the sign s. */
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
    ok = arb_contains_fmpq(z, a) && arb_contains_fmpq(z, b) && arb_is_nonzero(z) && arb_is_finite(z)
         && arf_sgn(arb_midref(z)) == s;
    fmpq_clear(a);
    fmpq_clear(b);
    return ok;
}

/* Oracle 1: z encloses abs(X) * f (f > 0 exact) with the sign s. */
static int
encloses_scaled(const arb_t z, const arb_t x, const fmpq_t f, int s)
{
    fmpq_t L, H;
    int ok;
    fmpq_init(L);
    fmpq_init(H);
    abs_ends(L, H, x);
    fmpq_mul(L, L, f);
    fmpq_mul(H, H, f);
    ok = encloses(z, L, H, s);
    fmpq_clear(L);
    fmpq_clear(H);
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

/* x = (inf, r, u) with a content given as an fmpq. */
static void
idele_set_parts_q(adf_idele_t x, const arb_t inf, const fmpq_t r, slong c, slong N)
{
    adf_ucoset_t u;
    fmpz_t fc, fN;
    adf_ucoset_init(u);
    fmpz_init_set_si(fc, c);
    fmpz_init_set_si(fN, N);
    if (adf_ucoset_set_fmpz2(u, fc, fN) != ADF_OK || adf_idele_set_parts(x, inf, r, u) != ADF_OK)
    {
        printf("idele_set_parts_q: refused\n");
        abort();
    }
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
    return fmpz_equal_si(fmpq_numref(x->r), n) && fmpz_equal_si(fmpq_denref(x->r), d) && fmpz_equal_si(x->u.c, c)
           && fmpz_equal_si(x->u.N, N);
}

static adf_place_t
place_of(ulong p)
{
    adf_place_t w;
    if (p == 0)
        return adf_place_inf();
    if (adf_place_prime(&w, p) != ADF_OK)
    {
        printf("place_of: %lu is not prime\n", (unsigned long) p);
        abort();
    }
    return w;
}

/* Oracle 2: v_p(n/d) by one divisibility test at a time. */
static slong
naive_val(const fmpq_t r, ulong p)
{
    fmpz_t a, pp;
    slong v = 0;
    fmpz_init(a);
    fmpz_init_set_ui(pp, p);
    fmpz_abs(a, fmpq_numref(r));
    while (!fmpz_is_zero(a) && fmpz_divisible(a, pp))
    {
        fmpz_divexact(a, a, pp);
        v++;
    }
    fmpz_set(a, fmpq_denref(r));
    while (fmpz_divisible(a, pp))
    {
        fmpz_divexact(a, a, pp);
        v--;
    }
    fmpz_clear(a);
    fmpz_clear(pp);
    return v;
}

/* a = p^(-v) exactly. */
static void
pow_neg(fmpq_t a, ulong p, slong v)
{
    fmpz_t t;
    fmpz_init_set_ui(t, p);
    fmpz_pow_ui(t, t, (ulong) (v < 0 ? -v : v));
    if (v >= 0)
    {
        fmpz_one(fmpq_numref(a));
        fmpz_set(fmpq_denref(a), t);
    }
    else
    {
        fmpz_set(fmpq_numref(a), t);
        fmpz_one(fmpq_denref(a));
    }
    fmpz_clear(t);
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
read_fmpz(fmpz_t z, const jsonl_value * v)
{
    jsonl_error_t err;
    const char * t;
    if (!jsonl_int_text_or_string(v, &t, &err) || fmpz_set_str(z, t, 10) != 0)
        abort();
}

static void
read_fmpz_at(fmpz_t z, const jsonl_value * arr, size_t i)
{
    jsonl_error_t err;
    read_fmpz(z, jsonl_at(arr, i, &err));
}

static slong
read_si(const jsonl_value * v)
{
    fmpz_t z;
    slong s;
    fmpz_init(z);
    read_fmpz(z, v);
    if (!fmpz_fits_si(z))
        abort();
    s = fmpz_get_si(z);
    fmpz_clear(z);
    return s;
}

static ulong
read_ui(const jsonl_value * v)
{
    fmpz_t z;
    ulong s;
    fmpz_init(z);
    read_fmpz(z, v);
    if (fmpz_sgn(z) < 0 || !fmpz_abs_fits_ui(z))
        abort();
    s = fmpz_get_ui(z);
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
read_ball(arb_t b, const jsonl_value * rec)
{
    fmpz_t m1, e1, m2, e2;
    fmpz_init(m1);
    fmpz_init(e1);
    fmpz_init(m2);
    fmpz_init(e2);
    read_fmpz_at(m1, field(rec, "mid"), 0);
    read_fmpz_at(e1, field(rec, "mid"), 1);
    read_fmpz_at(m2, field(rec, "rad"), 0);
    read_fmpz_at(e2, field(rec, "rad"), 1);
    arb_set_exact(b, m1, e1, fmpz_get_ui(m2), fmpz_get_si(e2));
    fmpz_clear(m1);
    fmpz_clear(e1);
    fmpz_clear(m2);
    fmpz_clear(e2);
}

static void
read_idele(adf_idele_t x, const jsonl_value * v)
{
    adf_ucoset_t u;
    fmpq_t r;
    arb_t inf;
    adf_ucoset_init(u);
    fmpq_init(r);
    arb_init(inf);
    read_ball(inf, v);
    read_fmpq(r, field(v, "r"));
    read_fmpz_at(u->c, field(v, "u"), 0);
    read_fmpz_at(u->N, field(v, "u"), 1);
    if (!adf_ucoset_is_canonical(u) || adf_idele_set_parts(x, inf, r, u) != ADF_OK || !adf_idele_is_canonical(x))
    {
        printf("read_idele: the input is not an idele\n");
        abort();
    }
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

/* ---- valuations and absolute values ---- */

ADF_TEST(valuations_of_the_idele_of_minus_6_over_35)
{
    static const ulong ps[] = { 2, 3, 5, 7, 11, 13, UWORD(18446744073709551557) };
    static const slong vs[] = { 1, 1, -1, -1, 0, 0, 0 };
    adf_idele_t x;
    adf_rat_t q, a, keep;
    slong v;
    size_t i;

    adf_idele_init(x);
    adf_rat_init(q);
    adf_rat_init(a);
    adf_rat_init(keep);
    rat_set_si(q, -6, 35);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    for (i = 0; i < sizeof(ps) / sizeof(ps[0]); i++)
    {
        fmpq_t want;
        fmpq_init(want);
        v = WORD_MIN;
        ADF_CHECK_MSG(adf_idele_valuation_at(&v, x, place_of(ps[i])) == ADF_OK && v == vs[i], "p = %lu",
                      (unsigned long) ps[i]);
        ADF_CHECK(adf_idele_abs_at(a, x, place_of(ps[i])) == ADF_OK);
        pow_neg(want, ps[i], vs[i]);
        ADF_CHECK_MSG(fmpq_equal(a->q, want) && adf_rat_is_canonical(a), "p = %lu", (unsigned long) ps[i]);
        fmpq_clear(want);
    }
    /* the archimedean place: DOMAIN, outputs untouched */
    v = 12345;
    ADF_CHECK(adf_idele_valuation_at(&v, x, adf_place_inf()) == ADF_DOMAIN && v == 12345);
    rat_set_si(a, 17, 3);
    adf_rat_set(keep, a);
    ADF_CHECK(adf_idele_abs_at(a, x, adf_place_inf()) == ADF_DOMAIN && adf_rat_identical(a, keep));
    adf_rat_clear(q);
    adf_rat_clear(a);
    adf_rat_clear(keep);
    adf_idele_clear(x);
}

ADF_TEST(valuation_by_divisibility_and_the_product_formula)
{
    /* r = 2^5 3^-2 7^3 / (11 * 101^2) * 13 and friends; the product of |r|_p over the primes that divide the
       numerator or the denominator is 1/r exactly (ideles.md P14.2) */
    static const ulong ps[] = { 2, 3, 5, 7, 11, 13, 17, 101, 65537 };
    adf_idele_t x;
    adf_rat_t a;
    arb_t inf;
    fmpq_t r, prod, want;
    slong v, k;
    size_t i;

    adf_idele_init(x);
    adf_rat_init(a);
    arb_init(inf);
    fmpq_init(r);
    fmpq_init(prod);
    fmpq_init(want);
    arb_set_si_si(inf, -7, -3, 1, -10);
    for (k = 0; k < 40; k++)
    {
        fmpz_t n, d, t;
        fmpz_init_set_ui(n, 1);
        fmpz_init_set_ui(d, 1);
        fmpz_init(t);
        for (i = 0; i < sizeof(ps) / sizeof(ps[0]); i++)
        {
            slong e = (slong) ((k * 7 + (slong) i * 13) % 9) - 4;         /* -4 .. 4 */
            fmpz_set_ui(t, ps[i]);
            fmpz_pow_ui(t, t, (ulong) (e < 0 ? -e : e));
            if (e >= 0)
                fmpz_mul(n, n, t);
            else
                fmpz_mul(d, d, t);
        }
        fmpq_set_fmpz_frac(r, n, d);
        idele_set_parts_q(x, inf, r, 5, 12);
        fmpq_one(prod);
        for (i = 0; i < sizeof(ps) / sizeof(ps[0]); i++)
        {
            int okv;
            v = WORD_MIN;
            okv = adf_idele_valuation_at(&v, x, place_of(ps[i])) == ADF_OK && v == naive_val(r, ps[i]);
            ADF_CHECK(okv);
            ADF_CHECK(adf_idele_abs_at(a, x, place_of(ps[i])) == ADF_OK);
            pow_neg(want, ps[i], naive_val(r, ps[i]));
            ADF_CHECK(fmpq_equal(a->q, want));
            fmpq_mul(prod, prod, a->q);
        }
        fmpq_inv(want, r);
        ADF_CHECK_MSG(fmpq_equal(prod, want), "k = %ld", (long) k);
        fmpz_clear(n);
        fmpz_clear(d);
        fmpz_clear(t);
    }
    fmpq_clear(r);
    fmpq_clear(prod);
    fmpq_clear(want);
    arb_clear(inf);
    adf_rat_clear(a);
    adf_idele_clear(x);
}

ADF_TEST(valuations_of_huge_contents)
{
    /* r = (2^61 - 1)^40 * 3^1000 / (2^3000 * (2^64 - 59)^7): thousands of bits */
    adf_idele_t x;
    adf_rat_t a;
    arb_t inf;
    fmpq_t r, want;
    fmpz_t t, n, d;
    slong v;
    const ulong P61 = (UWORD(1) << 61) - 1, P64 = UWORD(18446744073709551557);

    adf_idele_init(x);
    adf_rat_init(a);
    arb_init(inf);
    fmpq_init(r);
    fmpq_init(want);
    fmpz_init(t);
    fmpz_init(n);
    fmpz_init(d);
    fmpz_set_ui(n, P61);
    fmpz_pow_ui(n, n, 40);
    fmpz_set_ui(t, 3);
    fmpz_pow_ui(t, t, 1000);
    fmpz_mul(n, n, t);
    fmpz_set_ui(d, P64);
    fmpz_pow_ui(d, d, 7);
    fmpz_mul_2exp(d, d, 3000);
    fmpq_set_fmpz_frac(r, n, d);
    arb_set_si(inf, 3);
    idele_set_parts_q(x, inf, r, 1, 0);
    ADF_CHECK(adf_idele_valuation_at(&v, x, place_of(P61)) == ADF_OK && v == 40);
    ADF_CHECK(adf_idele_valuation_at(&v, x, place_of(3)) == ADF_OK && v == 1000);
    ADF_CHECK(adf_idele_valuation_at(&v, x, place_of(2)) == ADF_OK && v == -3000);
    ADF_CHECK(adf_idele_valuation_at(&v, x, place_of(P64)) == ADF_OK && v == -7);
    ADF_CHECK(adf_idele_valuation_at(&v, x, place_of(5)) == ADF_OK && v == 0);
    ADF_CHECK(adf_idele_abs_at(a, x, place_of(2)) == ADF_OK);
    pow_neg(want, 2, -3000);
    ADF_CHECK(fmpq_equal(a->q, want));
    ADF_CHECK(adf_idele_abs_at(a, x, place_of(P61)) == ADF_OK);
    pow_neg(want, P61, 40);
    ADF_CHECK(fmpq_equal(a->q, want));
    ADF_CHECK(adf_idele_abs_at(a, x, place_of(P64)) == ADF_OK);
    pow_neg(want, P64, -7);
    ADF_CHECK(fmpq_equal(a->q, want));
    fmpz_clear(t);
    fmpz_clear(n);
    fmpz_clear(d);
    fmpq_clear(r);
    fmpq_clear(want);
    arb_clear(inf);
    adf_rat_clear(a);
    adf_idele_clear(x);
}

ADF_TEST(abs_inf_is_the_exact_absolute_ball)
{
    adf_idele_t x;
    arb_t inf, a;

    adf_idele_init(x);
    arb_init(inf);
    arb_init(a);
    arb_set_si_si(inf, -13, -2, 7, -5);                   /* -3.25 +- 7/32 */
    idele_set_parts_si(x, inf, 3, 2, 5, 12);
    adf_idele_abs_inf(a, x);
    arb_neg(inf, inf);
    ADF_CHECK(arb_equal(a, inf) && arb_is_positive(a));
    arb_set_si_si(inf, 13, -2, 7, -5);
    idele_set_parts_si(x, inf, 3, 2, 5, 12);
    adf_idele_abs_inf(a, x);
    ADF_CHECK(arb_equal(a, inf));
    /* the exact idele 1 */
    adf_idele_init(x);
    adf_idele_abs_inf(a, x);
    ADF_CHECK(arb_is_one(a));
    arb_clear(inf);
    arb_clear(a);
    adf_idele_clear(x);
}

/* ---- the norm ---- */

ADF_TEST(norm_of_the_idele_of_a_rational_is_one)
{
    /* product formula (ideles.md P14.3): the norm of the idele of q contains 1; it is the exact 1 when the
       real ball is the exact q (q dyadic of at most p bits); negative rationals too.
       The status: OK is promised (Statement E6) when hi < 2^(p-1) lo. The real ball of set_rat lies in
       abs(q) [1 - 2^(2-p), 1 + 2^(2-p)] (E4 and B3: adjacent p-bit ends, a radius of at most their distance
       up to the mag rounding), and the norm rounds each end twice more, by a factor 1 +- 2^(1-p) each; so
       hi/lo <= (1 + 2^(2-p))/(1 - 2^(2-p)) ((1 + 2^(1-p))/(1 - 2^(1-p)))^2, which is 2.75 < 2^3 at p = 4
       and smaller above. At p = 2 and 3 NOT_DETERMINED is allowed: at p = 2 the ball of 5/8 is
       [1/2 +- (2^29 + 1) 2^-31] (the mag radius is a little above 1/4) and the norm ends are 1/4 and 2. */
    const slong p_ok = 4;
    static const slong qs[][2] = { { 1, 1 }, { -1, 1 }, { -3, 4 }, { 5, 8 }, { -6, 35 }, { 1, 3 },
                                   { -1000001, 7 }, { 7, 1024 }, { -12345, 1 } };
    adf_idele_t x;
    adf_rat_t q;
    arb_t t;
    size_t i;
    slong p;
    int n_exact = 0, n_nd = 0;

    adf_idele_init(x);
    adf_rat_init(q);
    arb_init(t);
    for (i = 0; i < sizeof(qs) / sizeof(qs[0]); i++)
        for (p = 0; p <= 131; p += (p < 5 ? 1 : 13))
        {
            rat_set_si(q, qs[i][0], qs[i][1]);
            int st;
            ADF_CHECK(adf_idele_set_rat(x, q, p) == ADF_OK);
            arb_set_si(t, 99);
            st = adf_idele_norm(t, x, p);
            if (st == ADF_NOT_DETERMINED)
            {
                ADF_CHECK_MSG(p < p_ok && arb_equal_si(t, 99), "q = %ld/%ld, p = %ld", (long) qs[i][0],
                              (long) qs[i][1], (long) p);
                n_nd++;
                continue;
            }
            ADF_CHECK_MSG(st == ADF_OK && arb_contains_si(t, 1) && arb_is_positive(t),
                          "q = %ld/%ld, p = %ld", (long) qs[i][0], (long) qs[i][1], (long) p);
            if (arb_is_exact(x->inf))
            {
                ADF_CHECK_MSG(arb_is_one(t), "q = %ld/%ld, p = %ld", (long) qs[i][0], (long) qs[i][1], (long) p);
                n_exact++;
            }
        }
    printf("norm of the idele of a rational: %d exact 1, %d NOT_DETERMINED (p < %ld)\n", n_exact, n_nd,
           (long) p_ok);
    ADF_CHECK(n_exact > 20);
    /* a huge rational: (2^3000 + 1) / 3^1200, negative */
    fmpz_one(fmpq_numref(q->q));
    fmpz_mul_2exp(fmpq_numref(q->q), fmpq_numref(q->q), 3000);
    fmpz_add_ui(fmpq_numref(q->q), fmpq_numref(q->q), 1);
    fmpz_set_ui(fmpq_denref(q->q), 3);
    fmpz_pow_ui(fmpq_denref(q->q), fmpq_denref(q->q), 1200);
    fmpq_neg(q->q, q->q);
    ADF_CHECK(adf_idele_set_rat(x, q, 200) == ADF_OK);
    ADF_CHECK(adf_idele_norm(t, x, 200) == ADF_OK && arb_contains_si(t, 1) && arb_rel_accuracy_bits(t) >= 190);
    /* a dyadic of 3000 bits at prec 3001: exact, norm exactly 1 */
    fmpz_one(fmpq_numref(q->q));
    fmpz_mul_2exp(fmpq_numref(q->q), fmpq_numref(q->q), 2999);
    fmpz_add_ui(fmpq_numref(q->q), fmpq_numref(q->q), 1);
    fmpz_set_ui(fmpq_denref(q->q), 1);
    fmpz_mul_2exp(fmpq_denref(q->q), fmpq_denref(q->q), 5000);
    ADF_CHECK(adf_idele_set_rat(x, q, 3001) == ADF_OK && arb_is_exact(x->inf));
    ADF_CHECK(adf_idele_norm(t, x, 3001) == ADF_OK && arb_is_one(t));
    ADF_CHECK(adf_idele_norm(t, x, 64) == ADF_OK && arb_contains_si(t, 1) && !arb_is_exact(t));
    arb_clear(t);
    adf_rat_clear(q);
    adf_idele_clear(x);
}

ADF_TEST(norm_hand_cases_and_not_determined)
{
    adf_idele_t x;
    arb_t inf, t, keep;
    fmpq_t f;

    adf_idele_init(x);
    arb_init(inf);
    arb_init(t);
    arb_init(keep);
    fmpq_init(f);
    /* (6 ; 3/2 [5 mod 12]): norm 4 exactly; (-6 ; ...) the same */
    arb_set_si(inf, 6);
    idele_set_parts_si(x, inf, 3, 2, 5, 12);
    ADF_CHECK(adf_idele_norm(t, x, 64) == ADF_OK && arb_is_exact(t) && arb_equal_si(t, 4));
    arb_set_si(inf, -6);
    idele_set_parts_si(x, inf, 3, 2, 5, 12);
    ADF_CHECK(adf_idele_norm(t, x, 64) == ADF_OK && arb_equal_si(t, 4));
    /* 4 needs 1 bit: exact at prec 2 (and at prec 0, 1: taken as 2) */
    ADF_CHECK(adf_idele_norm(t, x, 0) == ADF_OK && arb_equal_si(t, 4));
    /* (-1 ; 3 [1]): 1/3, never exact */
    arb_set_si(inf, -1);
    idele_set_parts_si(x, inf, 3, 1, 1, 0);
    fmpq_set_si(f, 1, 3);
    ADF_CHECK(adf_idele_norm(t, x, 64) == ADF_OK && arb_contains_fmpq(t, f) && !arb_is_exact(t)
              && arb_is_positive(t));
    /* a wide negative ball: -2.5 +- 0.25, content 3/2: [1.5, 11/6] */
    arb_set_si_si(inf, -5, -1, 1, -2);
    idele_set_parts_si(x, inf, 3, 2, 5, 36);
    fmpq_set_si(f, 2, 3);
    ADF_CHECK(adf_idele_norm(t, x, 30) == ADF_OK && encloses_scaled(t, inf, f, 1));
    /* 1 +- (1 - 2^-30): the ends of abs(X) are 2^-30 and 2 - 2^-30, about 2^31 apart: NOT_DETERMINED at
       p = 16, t untouched; OK at 64 */
    arb_set_si_si(inf, 1, 0, (UWORD(1) << 30) - 1, -30);
    idele_set_parts_si(x, inf, 7, 5, 1, 1);
    arb_set_si(keep, 9);
    arb_set(t, keep);
    ADF_CHECK(adf_idele_norm(t, x, 16) == ADF_NOT_DETERMINED && arb_equal(t, keep));
    fmpq_set_si(f, 5, 7);
    ADF_CHECK(adf_idele_norm(t, x, 64) == ADF_OK && encloses_scaled(t, inf, f, 1));
    fmpq_clear(f);
    arb_clear(inf);
    arb_clear(t);
    arb_clear(keep);
    adf_idele_clear(x);
}

/* ---- an idele times an exact rational ---- */

ADF_TEST(mul_rat_hand_cases_statuses_and_aliasing)
{
    adf_idele_t x, z, keep, a;
    adf_rat_t q;
    arb_t inf;
    fmpq_t f;

    adf_idele_init(x);
    adf_idele_init(z);
    adf_idele_init(keep);
    adf_idele_init(a);
    adf_rat_init(q);
    arb_init(inf);
    fmpq_init(f);
    /* (-2.5 +- 0.25 ; 3/2 [5 mod 36]) * (-4/9) = (1.111 +- ... ; 2/3 [31 mod 36]) */
    arb_set_si_si(inf, -5, -1, 1, -2);
    idele_set_parts_si(x, inf, 3, 2, 5, 36);
    rat_set_si(q, -4, 9);
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 64) == ADF_OK);
    fmpq_set_si(f, 4, 9);
    ADF_CHECK(finite_is(z, 2, 3, 31, 36) && encloses_scaled(z->inf, x->inf, f, 1) && adf_idele_is_canonical(z));
    /* positive q: the unit is kept, normal form: [5 mod 6] * 3 = content * 3, unit [2 mod 3] */
    arb_set_si(inf, 7);
    idele_set_parts_si(x, inf, 1, 5, 5, 6);
    rat_set_si(q, 3, 1);
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 64) == ADF_OK && finite_is(z, 3, 5, 2, 3) && arb_equal_si(z->inf, 21));
    /* exact: 7 * (-3) = -21 exactly at prec 5, a ball at prec 4 */
    rat_set_si(q, -3, 1);
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 5) == ADF_OK && arb_is_exact(z->inf) && arb_equal_si(z->inf, -21));
    ADF_CHECK(finite_is(z, 3, 5, 1, 3));
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 4) == ADF_OK && !arb_is_exact(z->inf) && arb_contains_si(z->inf, -21)
              && arb_is_negative(z->inf));
    /* the exact unit: [-1] * (-1) = [1] */
    idele_set_parts_si(x, inf, 1, 5, -1, 0);
    rat_set_si(q, -1, 1);
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 64) == ADF_OK && finite_is(z, 1, 5, 1, 0) && arb_equal_si(z->inf, -7));
    /* q = 0: NOT_UNIT, z untouched, also aliased */
    adf_idele_set(keep, z);
    fmpq_zero(q->q);
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 64) == ADF_NOT_UNIT && adf_idele_identical(z, keep));
    adf_idele_set(a, x);
    ADF_CHECK(adf_idele_mul_rat(a, a, q, 64) == ADF_NOT_UNIT && adf_idele_identical(a, x));
    /* NOT_DETERMINED: x = 1 +- (1 - 2^-30), times 1/3: the ends are 2^31 apart at p = 16; z untouched */
    arb_set_si_si(inf, 1, 0, (UWORD(1) << 30) - 1, -30);
    idele_set_parts_si(x, inf, 1, 1, 1, 1);
    rat_set_si(q, 1, 3);
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 16) == ADF_NOT_DETERMINED && adf_idele_identical(z, keep));
    adf_idele_set(a, x);
    ADF_CHECK(adf_idele_mul_rat(a, a, q, 16) == ADF_NOT_DETERMINED && adf_idele_identical(a, x));
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 64) == ADF_OK);
    fmpq_set_si(f, 1, 3);
    ADF_CHECK(encloses_scaled(z->inf, x->inf, f, 1) && finite_is(z, 1, 3, 1, 1));
    /* aliasing z = x on OK: the same result */
    adf_idele_set(a, x);
    ADF_CHECK(adf_idele_mul_rat(a, a, q, 64) == ADF_OK && adf_idele_identical(a, z));
    fmpq_clear(f);
    arb_clear(inf);
    adf_rat_clear(q);
    adf_idele_clear(x);
    adf_idele_clear(z);
    adf_idele_clear(keep);
    adf_idele_clear(a);
}

ADF_TEST(mul_rat_huge_operands_and_prec_below_2)
{
    adf_idele_t x, z, w;
    adf_rat_t q;
    arb_t inf;
    fmpq_t f;
    fmpz_t m, e;

    adf_idele_init(x);
    adf_idele_init(z);
    adf_idele_init(w);
    adf_rat_init(q);
    arb_init(inf);
    fmpq_init(f);
    fmpz_init(m);
    fmpz_init(e);
    /* q = -(3^2000 + 2) / 2^5000 (odd numerator: q is canonical) on a ball of exponent 3000 */
    fmpz_set_ui(fmpq_numref(q->q), 3);
    fmpz_pow_ui(fmpq_numref(q->q), fmpq_numref(q->q), 2000);
    fmpz_add_ui(fmpq_numref(q->q), fmpq_numref(q->q), 2);
    ADF_CHECK(adf_rat_is_canonical(q));
    fmpz_neg(fmpq_numref(q->q), fmpq_numref(q->q));
    fmpz_one(fmpq_denref(q->q));
    fmpz_mul_2exp(fmpq_denref(q->q), fmpq_denref(q->q), 5000);
    fmpz_set_si(m, 12345);
    fmpz_set_si(e, 3000);
    arb_set_exact(inf, m, e, 3, 2990);
    idele_set_parts_si(x, inf, 5, 7, 3, 4);
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 128) == ADF_OK);
    fmpq_abs(f, q->q);
    ADF_CHECK(encloses_scaled(z->inf, x->inf, f, -1) && adf_idele_is_canonical(z));
    fmpq_mul_si(f, f, 5);
    fmpz_set_si(m, 7);
    fmpq_div_fmpz(f, f, m);
    ADF_CHECK(fmpq_equal(z->r, f) && fmpz_equal_si(z->u.c, 1) && fmpz_equal_si(z->u.N, 4));
    /* prec 0, 1 and negative are prec 2 */
    ADF_CHECK(adf_idele_mul_rat(w, x, q, 2) == ADF_OK);
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 1) == ADF_OK && adf_idele_identical(z, w));
    ADF_CHECK(adf_idele_mul_rat(z, x, q, 0) == ADF_OK && adf_idele_identical(z, w));
    ADF_CHECK(adf_idele_mul_rat(z, x, q, WORD_MIN) == ADF_OK && adf_idele_identical(z, w));
    ADF_CHECK(adf_idele_norm(inf, x, 2) == ADF_OK);
    ADF_CHECK(adf_idele_norm(w->inf, x, -3) == ADF_OK && arb_equal(w->inf, inf));
    fmpz_clear(m);
    fmpz_clear(e);
    fmpq_clear(f);
    arb_clear(inf);
    adf_rat_clear(q);
    adf_idele_clear(x);
    adf_idele_clear(z);
    adf_idele_clear(w);
}

/* ---- the precision limit (orchestrator, 2026-09-30, after lane i-review1: 1/3 at prec LONG_MAX crashed) ---- */

ADF_TEST(prec_above_the_limit_is_LIMIT_and_untouched)
{
    static const slong big[] = { ADF_IDELE_PREC_MAX + 1, WORD(1) << 40, WORD_MAX };
    adf_idele_t x, y, z, keep;
    adf_rat_t q, zero;
    arb_t t, tkeep;
    size_t i;

    adf_idele_init(x);
    adf_idele_init(y);
    adf_idele_init(z);
    adf_idele_init(keep);
    adf_rat_init(q);
    adf_rat_init(zero);
    arb_init(t);
    arb_init(tkeep);
    ADF_CHECK(ADF_IDELE_PREC_MAX == 2097152);
    rat_set_si(q, 1, 3);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    rat_set_si(q, -5, 7);
    ADF_CHECK(adf_idele_set_rat(y, q, 64) == ADF_OK);
    rat_set_si(q, 1, 3);
    arb_set_si(z->inf, 9);
    adf_idele_set(keep, z);
    arb_set_si(tkeep, 11);
    arb_set(t, tkeep);
    for (i = 0; i < sizeof(big) / sizeof(big[0]); i++)
    {
        slong p = big[i];
        ADF_CHECK_MSG(adf_idele_set_rat(z, q, p) == ADF_LIMIT && adf_idele_identical(z, keep), "prec %ld",
                      (long) p);
        ADF_CHECK_MSG(adf_idele_set_rat(z, zero, p) == ADF_LIMIT && adf_idele_identical(z, keep), "prec %ld",
                      (long) p);
        ADF_CHECK_MSG(adf_idele_mul(z, x, y, p) == ADF_LIMIT && adf_idele_identical(z, keep), "prec %ld", (long) p);
        ADF_CHECK_MSG(adf_idele_inv(z, x, p) == ADF_LIMIT && adf_idele_identical(z, keep), "prec %ld", (long) p);
        ADF_CHECK_MSG(adf_idele_mul_rat(z, x, q, p) == ADF_LIMIT && adf_idele_identical(z, keep), "prec %ld",
                      (long) p);
        ADF_CHECK_MSG(adf_idele_mul_rat(z, x, zero, p) == ADF_LIMIT && adf_idele_identical(z, keep), "prec %ld",
                      (long) p);
        ADF_CHECK_MSG(adf_idele_norm(t, x, p) == ADF_LIMIT && arb_equal(t, tkeep), "prec %ld", (long) p);
        /* aliased */
        adf_idele_set(z, x);
        ADF_CHECK(adf_idele_inv(z, z, p) == ADF_LIMIT && adf_idele_identical(z, x));
        ADF_CHECK(adf_idele_mul(z, z, z, p) == ADF_LIMIT && adf_idele_identical(z, x));
        ADF_CHECK(adf_idele_mul_rat(z, z, q, p) == ADF_LIMIT && adf_idele_identical(z, x));
        adf_idele_set(z, keep);
    }
    /* at the limit itself: OK */
    ADF_CHECK(adf_idele_set_rat(z, q, ADF_IDELE_PREC_MAX) == ADF_OK && arb_contains_fmpq(z->inf, q->q));
    ADF_CHECK(arb_rel_accuracy_bits(z->inf) >= ADF_IDELE_PREC_MAX - 2);
    ADF_CHECK(adf_idele_inv(z, x, ADF_IDELE_PREC_MAX) == ADF_OK && arb_contains_si(z->inf, 3));
    ADF_CHECK(adf_idele_mul(z, x, y, ADF_IDELE_PREC_MAX) == ADF_OK);
    ADF_CHECK(adf_idele_mul_rat(z, x, q, ADF_IDELE_PREC_MAX) == ADF_OK);
    ADF_CHECK(adf_idele_norm(t, x, ADF_IDELE_PREC_MAX) == ADF_OK && arb_contains_si(t, 1));
    arb_clear(t);
    arb_clear(tkeep);
    adf_rat_clear(q);
    adf_rat_clear(zero);
    adf_idele_clear(x);
    adf_idele_clear(y);
    adf_idele_clear(z);
    adf_idele_clear(keep);
}

/* ---- oracle 3: the vectors ---- */

ADF_TEST(vectors_of_the_reference)
{
    jsonl_file * f;
    jsonl_error_t err;
    size_t i, j, n_mul = 0, n_norm = 0, n_val = 0, n_places = 0, n_inf = 0, n_nd = 0, n_exact = 0;
    adf_idele_t x, z, keep;
    adf_rat_t q, a;
    adf_ucoset_t u;
    fmpq_t r, L, H;
    arf_t lo, hi;
    arb_t t, tkeep;

    if (!jsonl_open("tests/ref/vectors/i-slice2/idele_maps.jsonl", &f, &err))
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    adf_idele_init(x);
    adf_idele_init(z);
    adf_idele_init(keep);
    adf_rat_init(q);
    adf_rat_init(a);
    adf_ucoset_init(u);
    fmpq_init(r);
    fmpq_init(L);
    fmpq_init(H);
    arf_init(lo);
    arf_init(hi);
    arb_init(t);
    arb_init(tkeep);
    arb_set_si(keep->inf, 3);
    arb_set_si(tkeep, 17);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * op = str_of(rec, "op");

        read_idele(x, field(rec, "x"));
        if (strcmp(op, "valuation") == 0)
        {
            const jsonl_value * at = field(rec, "at");
            for (j = 0; j < jsonl_size(at); j++)
            {
                const jsonl_value * e = jsonl_at(at, j, &err);
                ulong p = read_ui(field(e, "place"));
                int want = status_of(str_of(e, "status"));
                slong v = -777;
                rat_set_si(a, -5, 3);
                ADF_CHECK_MSG(adf_idele_valuation_at(&v, x, place_of(p)) == want, "line %zu", i + 1);
                ADF_CHECK_MSG(adf_idele_abs_at(a, x, place_of(p)) == want, "line %zu", i + 1);
                if (want == ADF_OK)
                {
                    read_fmpq(r, field(e, "a"));
                    ADF_CHECK_MSG(v == read_si(field(e, "v")) && v == naive_val(x->r, p) && fmpq_equal(a->q, r),
                                  "line %zu, p = %lu", i + 1, (unsigned long) p);
                }
                else
                    ADF_CHECK_MSG(v == -777 && fmpz_equal_si(fmpq_numref(a->q), -5), "line %zu", i + 1);
                n_places++;
            }
            n_val++;
            continue;
        }
        if (strcmp(op, "abs_inf") == 0)
        {
            arb_t w;
            arb_init(w);
            read_ball(w, rec);
            adf_idele_abs_inf(t, x);
            ADF_CHECK_MSG(arb_equal(t, w), "line %zu", i + 1);
            arb_clear(w);
            n_inf++;
            continue;
        }
        {
            slong prec = read_si(field(rec, "prec"));
            int want = status_of(str_of(rec, "status")), st;
            arb_ptr res;
            if (strcmp(op, "mul_rat") == 0)
            {
                read_fmpq(q->q, field(rec, "q"));
                adf_idele_set(z, keep);
                st = adf_idele_mul_rat(z, x, q, prec);
                res = z->inf;
                n_mul++;
            }
            else if (strcmp(op, "norm") == 0)
            {
                arb_set(t, tkeep);
                st = adf_idele_norm(t, x, prec);
                res = t;
                n_norm++;
            }
            else
            {
                ADF_CHECK_MSG(0, "unknown op %s on line %zu", op, i + 1);
                continue;
            }
            ADF_CHECK_MSG(st == want, "line %zu: status %d, want %d", i + 1, st, want);
            if (st != ADF_OK || want != ADF_OK)
            {
                if (strcmp(op, "mul_rat") == 0)
                    ADF_CHECK_MSG(adf_idele_identical(z, keep), "line %zu: output touched", i + 1);
                else
                    ADF_CHECK_MSG(arb_equal(t, tkeep), "line %zu: output touched", i + 1);
                n_nd += (want == ADF_NOT_DETERMINED);
                continue;
            }
            {
                slong s = read_si(field(rec, "sign"));
                read_arf(lo, field(rec, "lo"));
                read_arf(hi, field(rec, "hi"));
                read_fmpq(L, field(rec, "L"));
                read_fmpq(H, field(rec, "H"));
                ADF_CHECK_MSG(encloses(res, L, H, (int) s), "line %zu", i + 1);
                if (s < 0)
                {
                    arf_neg(lo, lo);
                    arf_neg(hi, hi);
                }
                ADF_CHECK_MSG(arb_contains_arf(res, lo) && arb_contains_arf(res, hi), "line %zu", i + 1);
                if (arf_equal(lo, hi))
                {
                    ADF_CHECK_MSG(arb_is_exact(res) && arf_equal(arb_midref(res), lo), "line %zu", i + 1);
                    n_exact++;
                }
                if (strcmp(op, "mul_rat") == 0)
                {
                    read_fmpq(r, field(rec, "r"));
                    read_fmpz_at(u->c, field(rec, "u"), 0);
                    read_fmpz_at(u->N, field(rec, "u"), 1);
                    ADF_CHECK_MSG(adf_idele_is_canonical(z) && fmpq_equal(z->r, r)
                                  && adf_ucoset_identical(&z->u, u), "line %zu", i + 1);
                    /* aliasing: the output is the input */
                    ADF_CHECK(adf_idele_mul_rat(x, x, q, prec) == ADF_OK && adf_idele_identical(x, z));
                }
                else
                    ADF_CHECK_MSG(arb_is_positive(t) && arb_is_finite(t), "line %zu", i + 1);
            }
        }
    }
    printf("vectors: %zu lines: %zu mul_rat, %zu norm, %zu valuation lines (%zu places), %zu abs_inf; "
           "%zu NOT_DETERMINED, %zu exact results\n", jsonl_count(f), n_mul, n_norm, n_val, n_places, n_inf, n_nd,
           n_exact);
    ADF_CHECK(n_mul + n_norm + n_val + n_inf == jsonl_count(f) && n_nd > 50 && n_places > 1000);
    jsonl_close(f);
    arf_clear(lo);
    arf_clear(hi);
    arb_clear(t);
    arb_clear(tkeep);
    fmpq_clear(r);
    fmpq_clear(L);
    fmpq_clear(H);
    adf_ucoset_clear(u);
    adf_rat_clear(q);
    adf_rat_clear(a);
    adf_idele_clear(x);
    adf_idele_clear(z);
    adf_idele_clear(keep);
}
