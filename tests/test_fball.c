/* tests/test_fball.c: adf_fball, global backend, beyond the vector files.

   Covers the rows of docs/SPEC.md 4.2 and 4.3 literally; enclosure by enumeration on small
   radii; the tightness witnesses of docs/proofs/precision.md Proposition 2 (line 48) and the
   valuation formula at each prime; canonical uniqueness (docs/proofs/policies.md Summary 26,
   proof of the canonical column, line 564); aliasing of arguments (conventions 4.1, closure
   CV-05); operands of 4096 bits; the accessors and the domain cases.

   The place functions of lane m1-rat are not in this worktree. The weak definitions below are
   the natural encoding (archimedean opaque = 0, a prime p at opaque = p), and let
   adf_fball_prec_at be exercised here; after the lanes are merged the real definitions of
   adf_place_* are used. */

#include <stdio.h>
#include <string.h>

#include <adelefeld/fball.h>

#include "test_runner.h"

__attribute__((weak)) adf_place_t
adf_place_inf(void)
{
    adf_place_t v;
    v.opaque = 0;
    return v;
}

__attribute__((weak)) int
adf_place_prime(adf_place_t * v, ulong p)
{
    if (p < 2)
        return ADF_DOMAIN;
    v->opaque = p;
    return ADF_OK;
}

__attribute__((weak)) int
adf_place_is_archimedean(adf_place_t v)
{
    return v.opaque == 0;
}

__attribute__((weak)) ulong
adf_place_prime_get(adf_place_t v)
{
    return v.opaque;
}

/* --------------------------------------------------------------- helpers */

static void
mkball_si(adf_fball_t x, slong A, slong H, slong d)
{
    fmpz_t a, h, dd;
    fmpz_init_set_si(a, A);
    fmpz_init_set_si(h, H);
    fmpz_init_set_si(dd, d);
    ADF_CHECK(adf_fball_set_fmpz3(x, a, h, dd) == ADF_OK);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(dd);
}

static int
eqball_si(const adf_fball_t x, slong A, slong H, slong d)
{
    return fmpz_equal_si(x->A, A) && fmpz_equal_si(x->H, H) && fmpz_equal_si(x->d, d);
}

/* The ball x contains the rational p/q, through the public membership function. */
static int
contains_pq(const adf_fball_t x, slong p, slong q)
{
    adf_rat_struct r;
    fmpz_t n, d;
    int res;

    fmpq_init(r.q);
    fmpz_init_set_si(n, p);
    fmpz_init_set_si(d, q);
    fmpq_set_fmpz_frac(r.q, n, d);
    res = adf_fball_contains_rat(x, &r);
    fmpz_clear(n);
    fmpz_clear(d);
    fmpq_clear(r.q);
    return res;
}

static void
get_center_fmpq(fmpq_t c, const adf_fball_t x)
{
    adf_rat_struct r;
    fmpq_init(r.q);
    adf_fball_get_center(&r, x);
    fmpq_set(c, r.q);
    fmpq_clear(r.q);
}

static void
get_radius_fmpq(fmpq_t r, const adf_fball_t x)
{
    adf_rat_struct s;
    fmpq_init(s.q);
    adf_fball_get_radius(&s, x);
    fmpq_set(r, s.q);
    fmpq_clear(s.q);
}

/* Give x the fields of a local-shaped value. The context pointer is never dereferenced; only
   the structural parts of predicate L are exercised. The caller frees x with
   adf_fball_clear, which releases the residue array. */
static void
make_local_shaped(adf_fball_t x, slong A, slong H, slong d, const void * fake_mctx)
{
    fmpz_set_si(x->A, A);
    fmpz_set_si(x->H, H);
    fmpz_set_si(x->d, d);
    x->backend = ADF_LOCAL;
    x->mctx = (const adf_modctx_struct *) fake_mctx;
    x->res = (ulong *) flint_malloc(2 * sizeof(ulong));
    x->res[0] = 0;
    x->res[1] = 1;
}

/* v_p of a nonzero rational, p a prime. */
static long
vp_fmpq(const fmpq_t q, ulong p)
{
    fmpz_t fp, t;
    long e;

    fmpz_init(fp);
    fmpz_init(t);
    fmpz_set_ui(fp, p);
    e = (long) fmpz_remove(t, fmpq_numref(q), fp);
    e -= (long) fmpz_remove(t, fmpq_denref(q), fp);
    fmpz_clear(fp);
    fmpz_clear(t);
    return e;
}

/* ------------------------------------------------- the rows of SPEC 4.3 */

ADF_TEST(spec_4_3_rows)
{
    adf_fball_t x, y, z;
    adf_rat_struct q;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(q.q);

    /* (3 mod 12) + (5 mod 18) = 2 mod 6 */
    mkball_si(x, 3, 12, 1);
    mkball_si(y, 5, 18, 1);
    adf_fball_add(z, x, y);
    ADF_CHECK(eqball_si(z, 2, 6, 1));

    /* (3 mod 12) * (5 mod 18) = 3 mod 6 */
    adf_fball_mul(z, x, y);
    ADF_CHECK(eqball_si(z, 3, 6, 1));

    /* 12 * (5 mod 18) = 60 mod 216 */
    fmpq_set_si(q.q, 12, 1);
    adf_fball_mul_rat(z, y, &q);
    ADF_CHECK(eqball_si(z, 60, 216, 1));

    /* (1/3) * (5 mod 18) = 5/3 mod 6 */
    fmpq_set_si(q.q, 1, 3);
    adf_fball_mul_rat(z, y, &q);
    ADF_CHECK(eqball_si(z, 5, 18, 3));

    /* (1/2 mod 8) * (2/3 mod 9) = 0 mod 1/6 */
    mkball_si(x, 1, 16, 2);    /* centre 1/2, radius 8 */
    mkball_si(y, 2, 27, 3);    /* centre 2/3, radius 9 */
    adf_fball_mul(z, x, y);
    ADF_CHECK(eqball_si(z, 0, 1, 6));

    fmpq_clear(q.q);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
}

/* ------------------------------------------------- the rows of SPEC 4.2 */

ADF_TEST(spec_4_2_rows)
{
    adf_fball_t a, b, c, d, e;

    adf_fball_init(a);
    adf_fball_init(b);
    adf_fball_init(c);
    adf_fball_init(d);
    adf_fball_init(e);
    mkball_si(a, 0, 2, 1);   /* 0 mod 2 */
    mkball_si(b, 0, 1, 1);   /* 0 mod 1 */
    mkball_si(c, 1, 2, 1);   /* 1 mod 2 */

    ADF_CHECK(adf_fball_overlaps(a, b));
    ADF_CHECK(adf_fball_overlaps(b, c));
    ADF_CHECK(!adf_fball_overlaps(a, c));   /* overlap is not transitive */

    mkball_si(d, 2, 2, 1);                  /* 2 mod 2 = 0 mod 2 */
    ADF_CHECK(adf_fball_equal_set(a, d));
    ADF_CHECK(!adf_fball_equal_set(a, b));

    mkball_si(e, 0, 4, 1);                  /* 0 mod 4 inside 0 mod 2 */
    ADF_CHECK(adf_fball_contains(e, a));
    ADF_CHECK(!adf_fball_contains(a, e));

    adf_fball_clear(a);
    adf_fball_clear(b);
    adf_fball_clear(c);
    adf_fball_clear(d);
    adf_fball_clear(e);
}

ADF_TEST(radius_zero_is_a_point)
{
    adf_fball_t p, q, b;

    adf_fball_init(p);
    adf_fball_init(q);
    adf_fball_init(b);
    mkball_si(p, 3, 0, 1);
    mkball_si(q, 3, 0, 1);
    mkball_si(b, 3, 12, 1);

    ADF_CHECK(adf_fball_is_exact(p));
    ADF_CHECK(!adf_fball_is_exact(b));
    ADF_CHECK(adf_fball_contains(p, b));    /* the point is inside the ball */
    ADF_CHECK(!adf_fball_contains(b, p));   /* a positive ball is never inside a point */
    ADF_CHECK(adf_fball_equal_set(p, q));
    ADF_CHECK(adf_fball_overlaps(p, b));
    ADF_CHECK(adf_fball_compare(p, q) == ADF_CMP_EQUAL);
    ADF_CHECK(contains_pq(p, 3, 1));
    ADF_CHECK(!contains_pq(p, 4, 1));
    ADF_CHECK(contains_pq(b, 3, 1));
    ADF_CHECK(!contains_pq(b, 4, 1));   /* (4-3)/12 is not an integer */

    adf_fball_clear(p);
    adf_fball_clear(q);
    adf_fball_clear(b);
}

/* ----------------------------------------------------------- aliasing */

ADF_TEST(aliasing_binary)
{
    adf_fball_t x, y, z, e, two;
    adf_rat_struct q;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    adf_fball_init(e);
    adf_fball_init(two);
    fmpq_init(q.q);
    mkball_si(x, 3, 12, 1);
    mkball_si(y, 5, 18, 1);

    /* add, all three aliasing patterns */
    adf_fball_add(e, x, y);
    adf_fball_set(z, x);
    adf_fball_add(z, z, y);
    ADF_CHECK(adf_fball_identical(z, e));
    adf_fball_set(z, y);
    adf_fball_add(z, x, z);
    ADF_CHECK(adf_fball_identical(z, e));
    adf_fball_set(z, x);
    adf_fball_add(z, z, z);
    adf_fball_add(two, x, x);
    ADF_CHECK(adf_fball_identical(z, two));

    /* sub */
    adf_fball_sub(e, x, y);
    adf_fball_set(z, x);
    adf_fball_sub(z, z, y);
    ADF_CHECK(adf_fball_identical(z, e));

    /* mul */
    adf_fball_mul(e, x, y);
    adf_fball_set(z, x);
    adf_fball_mul(z, z, y);
    ADF_CHECK(adf_fball_identical(z, e));
    adf_fball_set(z, y);
    adf_fball_mul(z, x, z);
    ADF_CHECK(adf_fball_identical(z, e));
    adf_fball_set(z, x);
    adf_fball_mul(z, z, z);
    adf_fball_mul(two, x, x);
    ADF_CHECK(adf_fball_identical(z, two));

    /* neg and scalar multiplication */
    adf_fball_set(z, x);
    adf_fball_neg(z, z);
    adf_fball_neg(e, x);
    ADF_CHECK(adf_fball_identical(z, e));

    fmpq_set_si(q.q, 7, 5);
    adf_fball_set(z, x);
    adf_fball_mul_rat(z, z, &q);
    adf_fball_mul_rat(e, x, &q);
    ADF_CHECK(adf_fball_identical(z, e));

    ADF_CHECK(adf_fball_div_rat(z, x, &q) == ADF_OK);
    adf_fball_div_rat(e, x, &q);
    ADF_CHECK(adf_fball_identical(z, e));

    fmpq_clear(q.q);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    adf_fball_clear(e);
    adf_fball_clear(two);
}

/* --------------------------------------------------- canonical form */

ADF_TEST(canonical_form)
{
    adf_fball_t x, y, z, a;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    adf_fball_init(a);

    /* (2 + 4 Zhat)/2 = (1 + 2 Zhat)/1: the canonical triple is unchanged by the raw data. */
    mkball_si(x, 2, 4, 2);
    mkball_si(y, 1, 2, 1);
    ADF_CHECK(adf_fball_identical(x, y));
    ADF_CHECK(eqball_si(x, 1, 2, 1));

    /* (-1 + 2 Zhat)/1 = (1 + 2 Zhat)/1. */
    mkball_si(x, -1, 2, 1);
    ADF_CHECK(eqball_si(x, 1, 2, 1));

    /* Summary 26: (9, 4, 1) is the raw square; the canonical triple is (1, 4, 1). */
    mkball_si(x, 9, 4, 1);
    ADF_CHECK(eqball_si(x, 1, 4, 1));

    /* (0 + 27 Zhat)/9 = (0 + 3 Zhat)/1. */
    mkball_si(x, 0, 27, 9);
    ADF_CHECK(eqball_si(x, 0, 3, 1));

    /* H = 0: lowest terms. */
    mkball_si(x, 0, 0, 5);
    ADF_CHECK(eqball_si(x, 0, 0, 1));
    mkball_si(x, -6, 0, 8);
    ADF_CHECK(eqball_si(x, -3, 0, 4));

    /* A negative denominator changes only the sign of A. */
    mkball_si(x, 3, 0, -4);
    ADF_CHECK(eqball_si(x, -3, 0, 4));

    /* The same set from different inputs gives identical fields, field by field. */
    mkball_si(x, 2, 6, 4);      /* (2 + 6 Zhat)/4 = (1 + 3 Zhat)/2 */
    mkball_si(y, 7, 3, 2);      /* (7 + 3 Zhat)/2 = (1 + 3 Zhat)/2 */
    ADF_CHECK(fmpz_equal(x->A, y->A) && fmpz_equal(x->H, y->H) && fmpz_equal(x->d, y->d));

    /* set_fmpz3 leaves the output untouched on ADF_DOMAIN. */
    mkball_si(x, 11, 5, 1);
    adf_fball_set(z, x);
    {
        fmpz_t A, H, d;
        fmpz_init_set_si(A, 1);
        fmpz_init_set_si(H, 2);
        fmpz_init_set_si(d, 0);
        ADF_CHECK(adf_fball_set_fmpz3(z, A, H, d) == ADF_DOMAIN);
        ADF_CHECK(adf_fball_identical(z, x));
        fmpz_set_si(d, 1);
        fmpz_set_si(H, -2);
        ADF_CHECK(adf_fball_set_fmpz3(z, A, H, d) == ADF_DOMAIN);
        ADF_CHECK(adf_fball_identical(z, x));
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
    }

    /* canonicalise on an already canonical global value is ADF_OK and changes nothing. */
    mkball_si(a, 0, 6, 1);
    ADF_CHECK(adf_fball_canonicalise(a) == ADF_OK);
    ADF_CHECK(eqball_si(a, 0, 6, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    adf_fball_clear(a);
}

/* --------------------------------------------- is_canonical, identical */

ADF_TEST(canonical_predicate)
{
    adf_fball_t x, y;

    adf_fball_init(x);
    adf_fball_init(y);

    mkball_si(x, 0, 0, 1);
    ADF_CHECK(adf_fball_is_canonical(x));
    mkball_si(x, 5, 0, 1);
    ADF_CHECK(adf_fball_is_canonical(x));
    mkball_si(x, -3, 0, 4);
    ADF_CHECK(adf_fball_is_canonical(x));
    mkball_si(x, 0, 2, 1);
    ADF_CHECK(adf_fball_is_canonical(x));
    mkball_si(x, 1, 2, 2);
    ADF_CHECK(adf_fball_is_canonical(x));

    /* A value with a shared factor between centre, radius and denominator is not canonical
       as raw data. */
    fmpz_set_si(x->A, 2);
    fmpz_set_si(x->H, 2);
    fmpz_set_si(x->d, 2);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->A, 0);
    fmpz_set_si(x->H, 0);
    fmpz_set_si(x->d, 2);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->A, 5);
    fmpz_set_si(x->H, 2);
    fmpz_set_si(x->d, 1);
    ADF_CHECK(!adf_fball_is_canonical(x));   /* A is not in [0, H) */

    /* A == H is not allowed either. */
    fmpz_set_si(x->A, 2);
    fmpz_set_si(x->H, 2);
    fmpz_set_si(x->d, 1);
    ADF_CHECK(!adf_fball_is_canonical(x));

    /* A non-NULL mctx or res with backend GLOBAL fails predicate G. */
    x->mctx = (const adf_modctx_struct *) (const void *) &x;
    fmpz_set_si(x->A, 0);
    fmpz_set_si(x->H, 2);
    fmpz_set_si(x->d, 1);
    ADF_CHECK(!adf_fball_is_canonical(x));
    x->mctx = NULL;
    x->res = (ulong *) flint_malloc(sizeof(ulong));
    ADF_CHECK(!adf_fball_is_canonical(x));
    flint_free(x->res);
    x->res = NULL;

    /* d = 0 and H < 0 are not canonical. */
    fmpz_set_si(x->A, 0);
    fmpz_set_si(x->H, 1);
    fmpz_set_si(x->d, 0);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->A, 0);
    fmpz_set_si(x->H, -1);
    fmpz_set_si(x->d, 1);
    ADF_CHECK(!adf_fball_is_canonical(x));

    /* identical: representation identity. For canonical global values equal_set and identical
       agree (conventions Lemma 5.2). */
    mkball_si(x, 1, 2, 1);
    mkball_si(y, 0, 2, 1);
    ADF_CHECK(!adf_fball_identical(x, y));
    ADF_CHECK(!adf_fball_equal_set(x, y));
    adf_fball_set(y, x);
    ADF_CHECK(adf_fball_identical(x, y));
    ADF_CHECK(adf_fball_equal_set(x, y));

    adf_fball_clear(x);
    adf_fball_clear(y);
}

/* ------------------------------------- enclosure by enumeration */

ADF_TEST(enclosure_by_enumeration)
{
    adf_fball_t x, y, s, p;
    adf_rat_struct r;
    long k, j;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(s);
    adf_fball_init(p);
    fmpq_init(r.q);

    mkball_si(x, 3, 12, 1);   /* members 3 + 12 k */
    mkball_si(y, 5, 18, 1);   /* members 5 + 18 j */
    adf_fball_add(s, x, y);
    adf_fball_mul(p, x, y);

    for (k = -3; k <= 3; k++)
    {
        for (j = -3; j <= 3; j++)
        {
            long mx = 3 + 12 * k;
            long my = 5 + 18 * j;

            ADF_CHECK_MSG(contains_pq(s, mx + my, 1), "sum member %ld + %ld", mx, my);
            ADF_CHECK_MSG(contains_pq(p, mx * my, 1), "product member %ld * %ld", mx, my);
        }
    }

    /* A fractional-radius case: (1/2 + 2 Zhat) members 1/2 + 2 k = (1 + 4 k)/2. */
    mkball_si(x, 1, 4, 2);
    mkball_si(y, 5, 6, 1);    /* members 5 + 6 j */
    adf_fball_add(s, x, y);
    for (k = -2; k <= 2; k++)
    {
        for (j = -2; j <= 2; j++)
        {
            /* sum = (1 + 4k)/2 + (5 + 6j) = (11 + 8k + 12j)/2 */
            fmpq_set_si(r.q, 11 + 8 * k + 12 * j, 2);
            ADF_CHECK(adf_fball_contains_rat(s, &r));
        }
    }

    fmpq_clear(r.q);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(s);
    adf_fball_clear(p);
}

/* --------------------------------------------- tightness of the product */

ADF_TEST(tightness_witnesses)
{
    adf_fball_t x, y, z;
    fmpq_t a, N, b, M, G, R, t1, t2, t3, val, ab;
    long k;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(a);
    fmpq_init(N);
    fmpq_init(b);
    fmpq_init(M);
    fmpq_init(G);
    fmpq_init(R);
    fmpq_init(t1);
    fmpq_init(t2);
    fmpq_init(t3);
    fmpq_init(val);
    fmpq_init(ab);

    mkball_si(x, 3, 12, 1);
    mkball_si(y, 5, 18, 1);
    adf_fball_mul(z, x, y);

    get_center_fmpq(a, x);
    get_radius_fmpq(N, x);
    get_center_fmpq(b, y);
    get_radius_fmpq(M, y);
    fmpq_mul(ab, a, b);
    fmpq_mul(t1, a, M);
    fmpq_mul(t2, b, N);
    fmpq_mul(t3, N, M);
    fmpq_gcd(G, t1, t2);
    fmpq_gcd(G, G, t3);

    /* The radius of the result equals the gcd of the four witness differences. */
    get_radius_fmpq(R, z);
    ADF_CHECK(fmpq_equal(R, G));

    /* The four products of the tightness proof (u, v in {0,1}) lie in the result. */
    for (k = 0; k < 4; k++)
    {
        int u = (k == 1 || k == 3);
        int v = (k == 2 || k == 3);
        adf_rat_struct rr;

        fmpq_set(val, ab);
        if (u)
            fmpq_add(val, val, t1);
        if (v)
            fmpq_add(val, val, t2);
        if (u && v)
            fmpq_add(val, val, t3);
        fmpq_init(rr.q);
        fmpq_set(rr.q, val);
        ADF_CHECK_MSG(adf_fball_contains_rat(z, &rr),
                      "witness u = %d, v = %d is not in the product", u, v);
        fmpq_clear(rr.q);
    }

    fmpq_clear(a);
    fmpq_clear(N);
    fmpq_clear(b);
    fmpq_clear(M);
    fmpq_clear(G);
    fmpq_clear(R);
    fmpq_clear(t1);
    fmpq_clear(t2);
    fmpq_clear(t3);
    fmpq_clear(val);
    fmpq_clear(ab);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
}

ADF_TEST(valuation_formula_at_each_prime)
{
    adf_fball_t x, y, z;
    fmpq_t a, N, b, M, G, R, v1, v2, v3;
    static const ulong primes[] = {2, 3, 5, 7, 11, 13, 17, 19};
    static const slong cases[][4] = {
        {3, 12, 5, 18},
        {-7, 24, 11, 30},
        {6, 45, -14, 20},
        {1, 8, 1, 9},
        {-3, 50, 7, 36}
    };
    size_t ci, pi;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(a);
    fmpq_init(N);
    fmpq_init(b);
    fmpq_init(M);
    fmpq_init(G);
    fmpq_init(R);
    fmpq_init(v1);
    fmpq_init(v2);
    fmpq_init(v3);

    for (ci = 0; ci < sizeof(cases) / sizeof(cases[0]); ci++)
    {
        mkball_si(x, cases[ci][0], cases[ci][1], 1);
        mkball_si(y, cases[ci][2], cases[ci][3], 1);
        adf_fball_mul(z, x, y);

        get_center_fmpq(a, x);
        get_radius_fmpq(N, x);
        get_center_fmpq(b, y);
        get_radius_fmpq(M, y);
        get_radius_fmpq(R, z);
        fmpq_mul(v1, a, M);
        fmpq_mul(v2, b, N);
        fmpq_mul(v3, N, M);
        fmpq_gcd(G, v1, v2);
        fmpq_gcd(G, G, v3);
        ADF_CHECK(fmpq_equal(R, G));

        for (pi = 0; pi < sizeof(primes) / sizeof(primes[0]); pi++)
        {
            long lhs = vp_fmpq(R, primes[pi]);
            long e1 = vp_fmpq(a, primes[pi]) + vp_fmpq(M, primes[pi]);
            long e2 = vp_fmpq(b, primes[pi]) + vp_fmpq(N, primes[pi]);
            long e3 = vp_fmpq(N, primes[pi]) + vp_fmpq(M, primes[pi]);
            long rhs = e1 < e2 ? e1 : e2;
            if (e3 < rhs)
                rhs = e3;
            ADF_CHECK_MSG(lhs == rhs, "case %lu prime %lu: v_p(G) = %ld, min = %ld",
                          (unsigned long) ci, (unsigned long) primes[pi], lhs, rhs);
        }
    }

    fmpq_clear(a);
    fmpq_clear(N);
    fmpq_clear(b);
    fmpq_clear(M);
    fmpq_clear(G);
    fmpq_clear(R);
    fmpq_clear(v1);
    fmpq_clear(v2);
    fmpq_clear(v3);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
}

/* ------------------------------------------------------------ accessors */

ADF_TEST(accessors)
{
    adf_fball_t x;
    adf_rat_struct c, r, vol;
    fmpz_t A, H, d;
    fmpq_t want;

    adf_fball_init(x);
    fmpq_init(c.q);
    fmpq_init(r.q);
    fmpq_init(vol.q);
    fmpq_init(want);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);

    mkball_si(x, 3, 12, 1);
    ADF_CHECK(!adf_fball_is_exact(x));
    fmpz_set_si(d, 0);
    adf_fball_get_den(d, x);
    ADF_CHECK(fmpz_equal_si(d, 1));
    adf_fball_get_center(&c, x);
    ADF_CHECK(fmpq_equal_si(c.q, 3));
    adf_fball_get_radius(&r, x);
    ADF_CHECK(fmpq_equal_si(r.q, 12));
    adf_fball_haar_volume(&vol, x);
    fmpq_set_si(want, 1, 12);
    ADF_CHECK(fmpq_equal(vol.q, want));

    adf_fball_get_fmpz3(A, H, d, x);
    ADF_CHECK(fmpz_equal_si(A, 3) && fmpz_equal_si(H, 12) && fmpz_equal_si(d, 1));

    /* exact rational 3/4 */
    mkball_si(x, 3, 0, 4);
    ADF_CHECK(adf_fball_is_exact(x));
    adf_fball_get_center(&c, x);
    fmpq_set_si(want, 3, 4);
    ADF_CHECK(fmpq_equal(c.q, want));
    adf_fball_get_radius(&r, x);
    ADF_CHECK(fmpq_is_zero(r.q));
    adf_fball_haar_volume(&vol, x);
    ADF_CHECK(fmpq_is_zero(vol.q));

    /* radius 1/2: (1 + 2 Zhat)/2 */
    mkball_si(x, 1, 2, 2);
    adf_fball_get_center(&c, x);
    fmpq_set_si(want, 1, 2);
    ADF_CHECK(fmpq_equal(c.q, want));
    adf_fball_get_radius(&r, x);
    fmpq_set_si(want, 1, 1);
    ADF_CHECK(fmpq_equal(r.q, want));
    adf_fball_haar_volume(&vol, x);
    ADF_CHECK(fmpq_equal_si(vol.q, 1));

    fmpq_clear(c.q);
    fmpq_clear(r.q);
    fmpq_clear(vol.q);
    fmpq_clear(want);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
}

ADF_TEST(precision_at_a_place)
{
    adf_fball_t x;
    adf_place_t v;
    slong e;

    adf_fball_init(x);
    ADF_CHECK(adf_place_prime(&v, 2) == ADF_OK);

    mkball_si(x, 3, 12, 1);
    e = 999;
    ADF_CHECK(adf_fball_prec_at(&e, x, v) == ADF_OK);
    ADF_CHECK_MSG(e == 2, "v_2(12) = %ld", (long) e);

    mkball_si(x, 5, 18, 3);   /* radius 6, v_2(6) = 1 */
    ADF_CHECK(adf_fball_prec_at(&e, x, v) == ADF_OK);
    ADF_CHECK(e == 1);

    mkball_si(x, 1, 2, 2);    /* radius 1, v_2(1) = 0 */
    ADF_CHECK(adf_fball_prec_at(&e, x, v) == ADF_OK);
    ADF_CHECK(e == 0);

    /* exact: the precision is infinite, so ADF_DOMAIN. */
    mkball_si(x, 5, 0, 1);
    e = 999;
    ADF_CHECK(adf_fball_prec_at(&e, x, v) == ADF_DOMAIN);
    ADF_CHECK(e == 999);

    /* the archimedean place: ADF_DOMAIN. */
    mkball_si(x, 3, 12, 1);
    e = 999;
    ADF_CHECK(adf_fball_prec_at(&e, x, adf_place_inf()) == ADF_DOMAIN);
    ADF_CHECK(e == 999);

    /* a negative valuation: x = (1 + 4 Zhat)/8, radius 1/2, v_2 = -1. */
    mkball_si(x, 1, 4, 8);
    ADF_CHECK(adf_fball_prec_at(&e, x, v) == ADF_OK);
    ADF_CHECK_MSG(e == -1, "v_2(1/2) = %ld", (long) e);

    adf_fball_clear(x);
}

/* ------------------------------------------------------- div_rat status */

ADF_TEST(div_rat_status)
{
    adf_fball_t x, y, keep, prod;
    adf_rat_struct zero, q;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(keep);
    adf_fball_init(prod);
    fmpq_init(zero.q);
    fmpq_init(q.q);
    mkball_si(x, 3, 12, 1);
    mkball_si(keep, 7, 5, 2);
    fmpq_set_si(zero.q, 0, 1);
    fmpq_set_si(q.q, 2, 3);

    adf_fball_set(y, keep);
    ADF_CHECK(adf_fball_div_rat(y, x, &zero) == ADF_NOT_UNIT);
    ADF_CHECK(adf_fball_identical(y, keep));

    ADF_CHECK(adf_fball_div_rat(y, x, &q) == ADF_OK);
    adf_fball_mul_rat(prod, y, &q);
    ADF_CHECK(adf_fball_equal_set(prod, x));

    fmpq_clear(zero.q);
    fmpq_clear(q.q);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(keep);
    adf_fball_clear(prod);
}

/* --------------------------------------------------- 4096-bit operands */

ADF_TEST(big_operands_4096_bits)
{
    adf_fball_t x, y, z;
    fmpz_t A, H, d;
    fmpq_t N, M, G, R;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpq_init(N);
    fmpq_init(M);
    fmpq_init(G);
    fmpq_init(R);

    /* A, H, d all about 4096 bits. */
    fmpz_one(A);
    fmpz_mul_2exp(A, A, 4096);
    fmpz_add_ui(A, A, 12345);           /* 2^4096 + 12345 */
    fmpz_one(H);
    fmpz_mul_2exp(H, H, 4096);
    fmpz_add_ui(H, H, 67891);           /* 2^4096 + 67891 */
    fmpz_one(d);
    fmpz_mul_2exp(d, d, 4095);
    fmpz_add_ui(d, d, 13579);           /* 2^4095 + 13579 */

    ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(x));
    ADF_CHECK(fmpz_sizeinbase(x->A, 2) > 4000);
    ADF_CHECK(fmpz_sizeinbase(x->H, 2) > 4000);

    /* second operand, about 4096 bits */
    fmpz_one(A);
    fmpz_mul_2exp(A, A, 4096);
    fmpz_add_ui(A, A, 24680);
    fmpz_one(H);
    fmpz_mul_2exp(H, H, 4096);
    fmpz_add_ui(H, H, 97531);
    fmpz_one(d);
    fmpz_mul_2exp(d, d, 4095);
    fmpz_add_ui(d, d, 86420);
    ADF_CHECK(adf_fball_set_fmpz3(y, A, H, d) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(y));

    adf_fball_add(z, x, y);
    ADF_CHECK(adf_fball_is_canonical(z));
    adf_fball_mul(z, x, y);
    ADF_CHECK(adf_fball_is_canonical(z));
    adf_fball_mul(z, x, x);
    ADF_CHECK(adf_fball_is_canonical(z));

    /* the sum is the tight sum: the radius is the gcd of the two radii. */
    get_radius_fmpq(N, x);
    get_radius_fmpq(M, y);
    fmpq_gcd(G, N, M);
    adf_fball_add(z, x, y);
    get_radius_fmpq(R, z);
    ADF_CHECK(fmpq_equal(R, G));

    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpq_clear(N);
    fmpq_clear(M);
    fmpq_clear(G);
    fmpq_clear(R);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
}

/* ------------------------------------------------------ constructors */

ADF_TEST(constructors)
{
    adf_fball_t x, y, c;
    adf_rat_struct q, neg, z;
    fmpz_t n;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(c);
    fmpq_init(q.q);
    fmpq_init(neg.q);
    fmpq_init(z.q);
    fmpz_init(n);

    adf_fball_zero(x);
    ADF_CHECK(eqball_si(x, 0, 0, 1));
    adf_fball_one(x);
    ADF_CHECK(eqball_si(x, 1, 0, 1));
    adf_fball_set_si(x, -7);
    ADF_CHECK(eqball_si(x, -7, 0, 1));
    fmpz_set_si(n, 123456789);
    adf_fball_set_fmpz(x, n);
    ADF_CHECK(fmpz_equal_si(x->A, 123456789) && fmpz_is_zero(x->H) && fmpz_is_one(x->d));

    fmpq_set_si(q.q, -3, 4);
    adf_fball_set_rat(x, &q);
    ADF_CHECK(eqball_si(x, -3, 0, 4));

    /* centre and radius; N < 0 is ADF_DOMAIN and leaves the output untouched. */
    fmpq_set_si(q.q, 1, 2);
    fmpq_set_si(neg.q, -1, 3);
    adf_fball_set_si(y, 11);
    ADF_CHECK(adf_fball_set_center_radius(y, &q, &neg) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(y, 11, 0, 1));
    fmpq_set_si(neg.q, 1, 3);
    ADF_CHECK(adf_fball_set_center_radius(y, &q, &neg) == ADF_OK);
    ADF_CHECK(eqball_si(y, 1, 2, 6));   /* 1/2 + (1/3) Zhat = (3 + 2 Zhat)/6 -> (1,2,6) */

    /* radius 0 gives the point */
    fmpq_set_si(z.q, 0, 1);
    ADF_CHECK(adf_fball_set_center_radius(c, &q, &z) == ADF_OK);
    ADF_CHECK(eqball_si(c, 1, 0, 2));

    fmpq_clear(q.q);
    fmpq_clear(neg.q);
    fmpq_clear(z.q);
    fmpz_clear(n);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(c);
}

/* ----------------------------------------------------------- swap/set */

ADF_TEST(swap_and_set)
{
    adf_fball_t x, y, a, b;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(a);
    adf_fball_init(b);
    mkball_si(x, 3, 12, 1);
    mkball_si(y, 5, 18, 1);
    adf_fball_set(a, x);
    adf_fball_set(b, y);

    adf_fball_swap(x, y);
    ADF_CHECK(adf_fball_identical(x, b));
    ADF_CHECK(adf_fball_identical(y, a));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(a);
    adf_fball_clear(b);
}

/* ----------------------------------------- canonicalise from raw fields */

ADF_TEST(canonicalise_raw_and_domains)
{
    adf_fball_t x, keep;
    char dummy;

    adf_fball_init(x);
    adf_fball_init(keep);

    /* A raw global triple that is not canonical: (2 + 4 Zhat)/2 -> (1 + 2 Zhat)/1. */
    fmpz_set_si(x->A, 2);
    fmpz_set_si(x->H, 4);
    fmpz_set_si(x->d, 2);
    x->backend = ADF_GLOBAL;
    x->mctx = NULL;
    x->res = NULL;
    ADF_CHECK(adf_fball_canonicalise(x) == ADF_OK);
    ADF_CHECK(eqball_si(x, 1, 2, 1));

    /* Raw exact (2, 0, 4) -> (1, 0, 2). This also pins the H = 0 side of the domain
       check (H = 0 is allowed; only H < 0 and d = 0 are ADF_DOMAIN). */
    fmpz_set_si(x->A, 2);
    fmpz_set_si(x->H, 0);
    fmpz_set_si(x->d, 4);
    ADF_CHECK(adf_fball_canonicalise(x) == ADF_OK);
    ADF_CHECK(eqball_si(x, 1, 0, 2));

    /* d = 0 is ADF_DOMAIN and the fields are untouched. */
    mkball_si(keep, 7, 5, 1);
    fmpz_set_si(x->A, 1);
    fmpz_set_si(x->H, 1);
    fmpz_set_si(x->d, 0);
    ADF_CHECK(adf_fball_canonicalise(x) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(x, 1, 1, 0));
    (void) keep;

    /* A local-shaped value is left as it is and returns ADF_OK, even when its fields would
       not be canonical as a global triple. */
    make_local_shaped(x, 2, 4, 2, &dummy);
    ADF_CHECK(adf_fball_canonicalise(x) == ADF_OK);
    ADF_CHECK(fmpz_equal_si(x->A, 2) && fmpz_equal_si(x->H, 4) && fmpz_equal_si(x->d, 2));

    adf_fball_clear(x);
    adf_fball_clear(keep);
}

/* -------------------------------------------- local-shaped structural checks */

ADF_TEST(local_shaped_is_canonical)
{
    adf_fball_t x;
    char dummy;
    ulong * save;

    adf_fball_init(x);
    make_local_shaped(x, 0, 6, 1, &dummy);
    ADF_CHECK(adf_fball_is_canonical(x));

    fmpz_set_si(x->A, 1);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->A, 0);

    fmpz_set_si(x->d, 0);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->d, 1);

    fmpz_set_si(x->H, 0);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->H, 6);

    x->mctx = NULL;
    ADF_CHECK(!adf_fball_is_canonical(x));
    x->mctx = (const adf_modctx_struct *) (const void *) &dummy;

    save = x->res;
    x->res = NULL;
    ADF_CHECK(!adf_fball_is_canonical(x));
    x->res = save;

    adf_fball_clear(x);
}

ADF_TEST(identical_local_guard)
{
    adf_fball_t x, y;
    char d1, d2;

    adf_fball_init(x);
    adf_fball_init(y);
    make_local_shaped(x, 0, 6, 1, &d1);
    make_local_shaped(y, 0, 6, 1, &d2);

    /* same fields, different context pointers */
    ADF_CHECK(!adf_fball_identical(x, y));

    y->mctx = x->mctx;
    ADF_CHECK(adf_fball_identical(x, y));

    /* different backend */
    y->backend = ADF_GLOBAL;
    ADF_CHECK(!adf_fball_identical(x, y));

    adf_fball_clear(x);
    adf_fball_clear(y);
}

ADF_TEST(predicates_local_and_radius_edges)
{
    adf_fball_t x, y;
    char dummy;
    adf_rat_struct q;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(q.q);

    /* equal_set with radius 0: two different points are not equal; two equal points are. */
    mkball_si(x, 3, 0, 1);
    mkball_si(y, 4, 0, 1);
    ADF_CHECK(!adf_fball_equal_set(x, y));
    fmpq_set_si(q.q, 3, 1);
    ADF_CHECK(adf_fball_contains_rat(x, &q));
    ADF_CHECK(!adf_fball_contains_rat(y, &q));

    /* a point and a ball with a positive radius are not equal, in either order */
    mkball_si(y, 3, 12, 1);
    ADF_CHECK(!adf_fball_equal_set(x, y));
    ADF_CHECK(!adf_fball_equal_set(y, x));

    /* two positive radii that differ */
    mkball_si(x, 0, 2, 1);
    mkball_si(y, 0, 4, 1);
    ADF_CHECK(!adf_fball_equal_set(x, y));

    /* a local-valued operand makes the set predicates fail closed and compare DISJOINT */
    mkball_si(x, 3, 12, 1);
    make_local_shaped(y, 3, 12, 1, &dummy);
    ADF_CHECK(!adf_fball_equal_set(x, y));
    ADF_CHECK(!adf_fball_overlaps(x, y));
    ADF_CHECK(!adf_fball_contains(x, y));
    ADF_CHECK(adf_fball_compare(x, y) == ADF_CMP_DIFFERENT);
    fmpq_set_si(q.q, 3, 1);
    ADF_CHECK(!adf_fball_contains_rat(y, &q));

    fmpq_clear(q.q);
    adf_fball_clear(x);
    adf_fball_clear(y);
}

/* --------------------------------- local inputs to the tight arithmetic */

ADF_TEST(arithmetic_local_input_untouched)
{
    adf_fball_t x, y, z, keep;
    adf_rat_struct q;
    char dummy;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    adf_fball_init(keep);
    fmpq_init(q.q);
    mkball_si(x, 3, 12, 1);
    mkball_si(keep, 7, 5, 2);
    make_local_shaped(y, 3, 12, 1, &dummy);
    fmpq_set_si(q.q, 2, 3);
    adf_fball_set(z, keep);

    /* A local operand leaves the output untouched, in either position. */
    adf_fball_add(z, x, y);
    ADF_CHECK(adf_fball_identical(z, keep));
    adf_fball_add(z, y, x);
    ADF_CHECK(adf_fball_identical(z, keep));
    adf_fball_sub(z, x, y);
    ADF_CHECK(adf_fball_identical(z, keep));
    adf_fball_mul(z, x, y);
    ADF_CHECK(adf_fball_identical(z, keep));
    adf_fball_neg(z, y);
    ADF_CHECK(adf_fball_identical(z, keep));
    adf_fball_mul_rat(z, y, &q);
    ADF_CHECK(adf_fball_identical(z, keep));
    ADF_CHECK(adf_fball_div_rat(z, y, &q) == ADF_UNSUPPORTED);
    ADF_CHECK(adf_fball_identical(z, keep));

    fmpq_clear(q.q);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    adf_fball_clear(keep);
}
