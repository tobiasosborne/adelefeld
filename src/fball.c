/* adelefeld/fball.c: adf_fball, the finite ball (A + H Zhat)/d, tight policy, global backend.

   Work package 1.2 (docs/PLAN.md section 6): the tight global backend. Meaning of a value
   (docs/SPEC.md 4.1): the set (A + H Zhat)/d, centre a = A/d, radius N = H/d, H >= 0, d > 0.
   Canonical form (conventions 5.2, predicate G): backend = ADF_GLOBAL, mctx = NULL, res = NULL,
   d > 0, H >= 0 and (H > 0: 0 <= A < H and gcd(A, H, d) = 1; H = 0: gcd(A, d) = 1).

   The tight rules are docs/proofs/precision.md Proposition 1 (sum, line 27) and Proposition 2
   (product, line 34); the predicates are Proposition 3 (line 59); membership of a rational is
   Lemma 1 (line 13). The tight result is the smallest ball containing the result set
   (docs/proofs/policies.md Lemma 1, line 43). The canonical triple is computed as in
   docs/proofs/policies.md Summary 26, proof of the canonical column (line 564), and
   docs/conventions.md Lemma 5.2.

   Local backend (ADF_LOCAL) is work package 1.8. This file implements predicate G only. A
   function that receives a local value follows the header where the header fixes a result that
   needs no block access, and otherwise does not write its output. This is recorded as a
   HEADER-FINDING in lanes/m1-fball/report.md.

   Aliasing (conventions 4.1): an output may be the same object as any input of the same type.
   Every operation reads its inputs into temporaries and writes the output last. */

#include "adelefeld/fball.h"

#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/flint.h>

/* The place functions of lane m1-rat are not in this worktree. They are declared in
   adelefeld/place.h; a weak reference lets this file compile and link alone and binds to the
   real functions after the lanes are merged. prec_at checks for NULL. */
#pragma weak adf_place_is_archimedean
#pragma weak adf_place_prime_get

/* --------------------------------------------------------------- helpers */

/* Canonicalise a raw triple in place, d != 0, H >= 0. Source: docs/proofs/policies.md
   Summary 26, proof of the canonical column (line 564): make d > 0; for H = 0 divide A and d
   by gcd(A, d); for H > 0 divide A, H, d by gcd(A, H, d), then reduce A into [0, H). */
static void
fb_canon(fmpz_t A, fmpz_t H, fmpz_t d)
{
    fmpz_t g;

    if (fmpz_sgn(d) == -1)
    {
        fmpz_neg(A, A);
        fmpz_neg(d, d);
    }

    fmpz_init(g);
    if (fmpz_is_zero(H))
    {
        fmpz_gcd(g, A, d);
        fmpz_divexact(A, A, g);
        fmpz_divexact(d, d, g);
    }
    else
    {
        fmpz_gcd3(g, A, H, d);
        fmpz_divexact(A, A, g);
        fmpz_divexact(H, H, g);
        fmpz_divexact(d, d, g);
        fmpz_mod(A, A, H);
    }
    fmpz_clear(g);
}

/* Write the canonical triple (A, H, d) into x and return x to the global backend. A, H, d are
   borrowed temporaries (they may equal x->A etc.); every value is copied. The residue array of
   a previous local value is released, so the output satisfies G. */
static void
fb_store(adf_fball_t x, const fmpz_t A, const fmpz_t H, const fmpz_t d)
{
    flint_free(x->res);
    x->res = NULL;
    fmpz_set(x->A, A);
    fmpz_set(x->H, H);
    fmpz_set(x->d, d);
    x->backend = ADF_GLOBAL;
    x->mctx = NULL;
}

/* x = c + r Zhat with the canonical triple, c and r fmpq, r >= 0. This is
   Fball.from_center_radius (tests/ref/adfref/fball.py): a common denominator of c and r,
   then canon. */
static void
fb_set_cr(adf_fball_t x, const fmpq * c, const fmpq * r)
{
    fmpz_t A, H, d, t;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(t);

    fmpz_lcm(d, fmpq_denref(c), fmpq_denref(r));
    fmpz_divexact(t, d, fmpq_denref(c));
    fmpz_mul(A, fmpq_numref(c), t);
    fmpz_divexact(t, d, fmpq_denref(r));
    fmpz_mul(H, fmpq_numref(r), t);
    fb_canon(A, H, d);
    fb_store(x, A, H, d);

    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(t);
}

/* The centre a = A/d as a canonical fmpq. */
static void
fb_center(fmpq_t a, const adf_fball_t x)
{
    fmpq_set_fmpz_frac(a, x->A, x->d);
}

/* The radius N = H/d as a canonical fmpq. */
static void
fb_radius(fmpq_t N, const adf_fball_t x)
{
    fmpq_set_fmpz_frac(N, x->H, x->d);
}

/* 1 if the rational q has no prime in its denominator (docs/proofs/precision.md Lemma 1,
   line 13: R Zhat meets Q in R Z). */
static int
fb_is_integer(const fmpq * q)
{
    return fmpz_is_one(fmpq_denref(q));
}

/* ----------------------------------------------------------- life cycle */

void
adf_fball_init(adf_fball_t x)
{
    fmpz_init(x->A);
    fmpz_init(x->H);
    fmpz_init(x->d);
    fmpz_zero(x->A);
    fmpz_zero(x->H);
    fmpz_one(x->d);
    x->backend = ADF_GLOBAL;
    x->mctx = NULL;
    x->res = NULL;
}

void
adf_fball_clear(adf_fball_t x)
{
    fmpz_clear(x->A);
    fmpz_clear(x->H);
    fmpz_clear(x->d);
    flint_free(x->res);
    x->res = NULL;
    x->mctx = NULL;
    x->backend = ADF_GLOBAL;
}

/* Copy a global value. A local value cannot be copied without reading the block list of its
   context, which this file does not do (HEADER-FINDING); y is then left untouched. */
void
adf_fball_set(adf_fball_t y, const adf_fball_t x)
{
    if (x->backend != ADF_GLOBAL)
        return;
    flint_free(y->res);
    y->res = NULL;
    fmpz_set(y->A, x->A);
    fmpz_set(y->H, x->H);
    fmpz_set(y->d, x->d);
    y->backend = ADF_GLOBAL;
    y->mctx = NULL;
}

void
adf_fball_swap(adf_fball_t x, adf_fball_t y)
{
    int ib;
    const adf_modctx_struct * im;

    fmpz_swap(x->A, y->A);
    fmpz_swap(x->H, y->H);
    fmpz_swap(x->d, y->d);
    ib = x->backend;
    x->backend = y->backend;
    y->backend = ib;
    im = x->mctx;
    x->mctx = y->mctx;
    y->mctx = im;
    {
        ulong * r = x->res;
        x->res = y->res;
        y->res = r;
    }
}

int
adf_fball_is_canonical(const adf_fball_t x)
{
    fmpz_t g;
    int ok;

    if (x->backend == ADF_GLOBAL)
    {
        if (x->mctx != NULL || x->res != NULL)
            return 0;
        if (fmpz_sgn(x->d) <= 0 || fmpz_sgn(x->H) < 0)
            return 0;
        fmpz_init(g);
        if (fmpz_is_zero(x->H))
        {
            fmpz_gcd(g, x->A, x->d);
            ok = fmpz_is_one(g);
        }
        else
        {
            if (fmpz_sgn(x->A) < 0 || fmpz_cmp(x->A, x->H) >= 0)
                ok = 0;
            else
            {
                fmpz_gcd3(g, x->A, x->H, x->d);
                ok = fmpz_is_one(g);
            }
        }
        fmpz_clear(g);
        return ok;
    }

    /* Predicate L also needs the blocks of mctx; they are not readable here (HEADER-FINDING).
       The structural part is checked, the residue ranges and H = K are not. */
    if (x->mctx == NULL || x->res == NULL)
        return 0;
    if (!fmpz_is_zero(x->A))
        return 0;
    if (fmpz_sgn(x->d) < 1 || fmpz_sgn(x->H) < 1)
        return 0;
    return 1;
}

int
adf_fball_identical(const adf_fball_t x, const adf_fball_t y)
{
    if (x->backend != y->backend || x->mctx != y->mctx)
        return 0;
    if (!fmpz_equal(x->A, y->A) || !fmpz_equal(x->H, y->H) || !fmpz_equal(x->d, y->d))
        return 0;
    /* For a local value the residue arrays are also compared by the header. Reading them
       needs the block count of the context (HEADER-FINDING); the fields above are compared. */
    return 1;
}

/* ---------------------------------------------------------- constructors */

void
adf_fball_zero(adf_fball_t x)
{
    fmpz_zero(x->A);
    fmpz_zero(x->H);
    fmpz_one(x->d);
    x->backend = ADF_GLOBAL;
    x->mctx = NULL;
    flint_free(x->res);
    x->res = NULL;
}

void
adf_fball_one(adf_fball_t x)
{
    fmpz_one(x->A);
    fmpz_zero(x->H);
    fmpz_one(x->d);
    x->backend = ADF_GLOBAL;
    x->mctx = NULL;
    flint_free(x->res);
    x->res = NULL;
}

void
adf_fball_set_si(adf_fball_t x, slong n)
{
    fmpz_set_si(x->A, n);
    fmpz_zero(x->H);
    fmpz_one(x->d);
    x->backend = ADF_GLOBAL;
    x->mctx = NULL;
    flint_free(x->res);
    x->res = NULL;
}

void
adf_fball_set_fmpz(adf_fball_t x, const fmpz_t n)
{
    fmpz_set(x->A, n);
    fmpz_zero(x->H);
    fmpz_one(x->d);
    x->backend = ADF_GLOBAL;
    x->mctx = NULL;
    flint_free(x->res);
    x->res = NULL;
}

void
adf_fball_set_rat(adf_fball_t x, const adf_rat_t q)
{
    fmpz_set(x->A, fmpq_numref(q->q));
    fmpz_zero(x->H);
    fmpz_set(x->d, fmpq_denref(q->q));
    x->backend = ADF_GLOBAL;
    x->mctx = NULL;
    flint_free(x->res);
    x->res = NULL;
}

int
adf_fball_set_fmpz3(adf_fball_t x, const fmpz_t A, const fmpz_t H, const fmpz_t d)
{
    fmpz_t tA, tH, td;

    if (fmpz_is_zero(d) || fmpz_sgn(H) < 0)
        return ADF_DOMAIN;

    fmpz_init(tA);
    fmpz_init(tH);
    fmpz_init(td);
    fmpz_set(tA, A);
    fmpz_set(tH, H);
    fmpz_set(td, d);
    fb_canon(tA, tH, td);
    fb_store(x, tA, tH, td);
    fmpz_clear(tA);
    fmpz_clear(tH);
    fmpz_clear(td);
    return ADF_OK;
}

int
adf_fball_set_center_radius(adf_fball_t x, const adf_rat_t c, const adf_rat_t N)
{
    if (fmpq_sgn(N->q) < 0)
        return ADF_DOMAIN;
    fb_set_cr(x, c->q, N->q);
    return ADF_OK;
}

int
adf_fball_canonicalise(adf_fball_t x)
{
    fmpz_t A, H, d;

    if (x->backend == ADF_LOCAL)
        return ADF_OK;

    if (fmpz_is_zero(x->d) || fmpz_sgn(x->H) < 0)
        return ADF_DOMAIN;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_set(A, x->A);
    fmpz_set(H, x->H);
    fmpz_set(d, x->d);
    fb_canon(A, H, d);
    fb_store(x, A, H, d);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    return ADF_OK;
}

/* ------------------------------------------------------------- accessors */

int
adf_fball_is_exact(const adf_fball_t x)
{
    return fmpz_is_zero(x->H);
}

/* A local canonical triple needs CRT recombination from the blocks of mctx (docs/proofs/
   policies.md Lemma 18, line 331); this file does not read the context (HEADER-FINDING), so a
   local value leaves A, H, d untouched. */
void
adf_fball_get_fmpz3(fmpz_t A, fmpz_t H, fmpz_t d, const adf_fball_t x)
{
    if (x->backend != ADF_GLOBAL)
        return;
    fmpz_set(A, x->A);
    fmpz_set(H, x->H);
    fmpz_set(d, x->d);
}

void
adf_fball_get_center(adf_rat_t c, const adf_fball_t x)
{
    if (x->backend != ADF_GLOBAL)
        return;
    fmpq_set_fmpz_frac(c->q, x->A, x->d);
}

void
adf_fball_get_radius(adf_rat_t N, const adf_fball_t x)
{
    fmpq_set_fmpz_frac(N->q, x->H, x->d);
}

void
adf_fball_get_den(fmpz_t d, const adf_fball_t x)
{
    fmpz_set(d, x->d);
}

int
adf_fball_prec_at(slong * e, const adf_fball_t x, adf_place_t v)
{
    fmpz_t p, t;
    slong vH, vd;

    if (adf_place_is_archimedean == NULL)
        return ADF_UNSUPPORTED;
    if (adf_place_prime_get == NULL)
        return ADF_UNSUPPORTED;
    if (adf_place_is_archimedean(v))
        return ADF_DOMAIN;
    if (fmpz_is_zero(x->H))
        return ADF_DOMAIN;

    fmpz_init(p);
    fmpz_init(t);
    fmpz_set_ui(p, adf_place_prime_get(v));
    vH = fmpz_remove(t, x->H, p);
    vd = fmpz_remove(t, x->d, p);
    fmpz_clear(p);
    fmpz_clear(t);

    *e = vH - vd;
    return ADF_OK;
}

void
adf_fball_haar_volume(adf_rat_t vol, const adf_fball_t x)
{
    if (fmpz_is_zero(x->H))
        fmpq_zero(vol->q);
    else
        fmpq_set_fmpz_frac(vol->q, x->d, x->H);
}

/* ------------------------------------------------------ tight arithmetic */

void
adf_fball_add(adf_fball_t z, const adf_fball_t x, const adf_fball_t y)
{
    fmpq_t a, b, N, M, c, r;

    if (x->backend != ADF_GLOBAL || y->backend != ADF_GLOBAL)
        return;
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(N);
    fmpq_init(M);
    fmpq_init(c);
    fmpq_init(r);
    fb_center(a, x);
    fb_radius(N, x);
    fb_center(b, y);
    fb_radius(M, y);
    fmpq_add(c, a, b);
    fmpq_gcd(r, N, M);
    fb_set_cr(z, c, r);
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    fmpq_clear(c);
    fmpq_clear(r);
}

void
adf_fball_sub(adf_fball_t z, const adf_fball_t x, const adf_fball_t y)
{
    fmpq_t a, b, N, M, c, r;

    if (x->backend != ADF_GLOBAL || y->backend != ADF_GLOBAL)
        return;
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(N);
    fmpq_init(M);
    fmpq_init(c);
    fmpq_init(r);
    fb_center(a, x);
    fb_radius(N, x);
    fb_center(b, y);
    fb_radius(M, y);
    fmpq_sub(c, a, b);
    fmpq_gcd(r, N, M);
    fb_set_cr(z, c, r);
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    fmpq_clear(c);
    fmpq_clear(r);
}

void
adf_fball_neg(adf_fball_t y, const adf_fball_t x)
{
    fmpq_t a, N, c, r;

    if (x->backend != ADF_GLOBAL)
        return;
    fmpq_init(a);
    fmpq_init(N);
    fmpq_init(c);
    fmpq_init(r);
    fb_center(a, x);
    fb_radius(N, x);
    fmpq_neg(c, a);
    fmpq_set(r, N);
    fb_set_cr(y, c, r);
    fmpq_clear(a);
    fmpq_clear(N);
    fmpq_clear(c);
    fmpq_clear(r);
}

/* Tight product radius G = gcd(a M, b N, N M), centre a b (docs/proofs/precision.md
   Proposition 2, line 34). */
void
adf_fball_mul(adf_fball_t z, const adf_fball_t x, const adf_fball_t y)
{
    fmpq_t a, b, N, M, c, g, t1, t2, t3;

    if (x->backend != ADF_GLOBAL || y->backend != ADF_GLOBAL)
        return;
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(N);
    fmpq_init(M);
    fmpq_init(c);
    fmpq_init(g);
    fmpq_init(t1);
    fmpq_init(t2);
    fmpq_init(t3);
    fb_center(a, x);
    fb_radius(N, x);
    fb_center(b, y);
    fb_radius(M, y);
    fmpq_mul(c, a, b);
    fmpq_mul(t1, a, M);
    fmpq_mul(t2, b, N);
    fmpq_mul(t3, N, M);
    fmpq_gcd(g, t1, t2);
    fmpq_gcd(g, g, t3);
    fb_set_cr(z, c, g);
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    fmpq_clear(c);
    fmpq_clear(g);
    fmpq_clear(t1);
    fmpq_clear(t2);
    fmpq_clear(t3);
}

void
adf_fball_mul_rat(adf_fball_t y, const adf_fball_t x, const adf_rat_t q)
{
    fmpq_t a, N, c, r;

    if (x->backend != ADF_GLOBAL)
        return;
    fmpq_init(a);
    fmpq_init(N);
    fmpq_init(c);
    fmpq_init(r);
    fb_center(a, x);
    fb_radius(N, x);
    fmpq_mul(c, a, q->q);
    fmpq_abs(r, q->q);
    fmpq_mul(r, r, N);
    fb_set_cr(y, c, r);
    fmpq_clear(a);
    fmpq_clear(N);
    fmpq_clear(c);
    fmpq_clear(r);
}

int
adf_fball_div_rat(adf_fball_t y, const adf_fball_t x, const adf_rat_t q)
{
    fmpq_t a, N, c, r, qq;

    if (x->backend != ADF_GLOBAL)
        return ADF_UNSUPPORTED;
    if (fmpq_is_zero(q->q))
        return ADF_NOT_UNIT;

    fmpq_init(a);
    fmpq_init(N);
    fmpq_init(c);
    fmpq_init(r);
    fmpq_init(qq);
    fb_center(a, x);
    fb_radius(N, x);
    fmpq_inv(qq, q->q);
    fmpq_mul(c, a, qq);
    fmpq_abs(r, qq);
    fmpq_mul(r, r, N);
    fb_set_cr(y, c, r);
    fmpq_clear(a);
    fmpq_clear(N);
    fmpq_clear(c);
    fmpq_clear(r);
    fmpq_clear(qq);
    return ADF_OK;
}

/* -------------------------------------------------------- set predicates */

int
adf_fball_equal_set(const adf_fball_t x, const adf_fball_t y)
{
    fmpq_t a, b, N, M, diff, t;
    int res;

    if (x->backend != ADF_GLOBAL || y->backend != ADF_GLOBAL)
        return 0;

    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(N);
    fmpq_init(M);
    fmpq_init(diff);
    fmpq_init(t);
    fb_center(a, x);
    fb_center(b, y);
    fb_radius(N, x);
    fb_radius(M, y);
    if (fmpq_is_zero(N))
        res = fmpq_is_zero(M) && fmpq_equal(a, b);
    else if (fmpq_is_zero(M))
        res = 0;
    else if (!fmpq_equal(N, M))
        res = 0;
    else
    {
        fmpq_sub(diff, a, b);
        fmpq_div(t, diff, N);
        res = fb_is_integer(t);
    }
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    fmpq_clear(diff);
    fmpq_clear(t);
    return res;
}

int
adf_fball_overlaps(const adf_fball_t x, const adf_fball_t y)
{
    fmpq_t a, b, N, M, g, diff, t;
    int res;

    if (x->backend != ADF_GLOBAL || y->backend != ADF_GLOBAL)
        return 0;

    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(N);
    fmpq_init(M);
    fmpq_init(g);
    fmpq_init(diff);
    fmpq_init(t);
    fb_center(a, x);
    fb_center(b, y);
    fb_radius(N, x);
    fb_radius(M, y);
    fmpq_gcd(g, N, M);
    if (fmpq_is_zero(g))
        res = fmpq_equal(a, b);
    else
    {
        fmpq_sub(diff, a, b);
        fmpq_div(t, diff, g);
        res = fb_is_integer(t);
    }
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    fmpq_clear(g);
    fmpq_clear(diff);
    fmpq_clear(t);
    return res;
}

int
adf_fball_contains(const adf_fball_t x, const adf_fball_t y)
{
    fmpq_t a, b, N, M, diff, t;
    int res;

    if (x->backend != ADF_GLOBAL || y->backend != ADF_GLOBAL)
        return 0;

    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(N);
    fmpq_init(M);
    fmpq_init(diff);
    fmpq_init(t);
    fb_center(a, x);
    fb_center(b, y);
    fb_radius(N, x);
    fb_radius(M, y);
    if (fmpq_is_zero(M))
        res = fmpq_is_zero(N) && fmpq_equal(a, b);
    else
    {
        fmpq_sub(diff, a, b);
        fmpq_div(t, diff, M);
        if (fmpq_is_zero(N))
            res = fb_is_integer(t);
        else
        {
            fmpq_div(t, N, M);
            res = fb_is_integer(t);
            fmpq_sub(diff, a, b);
            fmpq_div(t, diff, M);
            res = res && fb_is_integer(t);
        }
    }
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    fmpq_clear(diff);
    fmpq_clear(t);
    return res;
}

int
adf_fball_contains_rat(const adf_fball_t x, const adf_rat_t q)
{
    fmpq_t a, N, diff, t;
    int res;

    if (x->backend != ADF_GLOBAL)
        return 0;

    fmpq_init(a);
    fmpq_init(N);
    fmpq_init(diff);
    fmpq_init(t);
    if (fmpz_is_zero(x->H))
    {
        fb_center(a, x);
        res = fmpq_equal(q->q, a);
    }
    else
    {
        fb_center(a, x);
        fb_radius(N, x);
        fmpq_sub(diff, q->q, a);
        fmpq_div(t, diff, N);
        res = fb_is_integer(t);
    }
    fmpq_clear(a);
    fmpq_clear(N);
    fmpq_clear(diff);
    fmpq_clear(t);
    return res;
}

int
adf_fball_compare(const adf_fball_t x, const adf_fball_t y)
{
    fmpq_t a, b, N, M;
    int res;

    if (x->backend != ADF_GLOBAL)
        return ADF_CMP_DIFFERENT;
    if (y->backend != ADF_GLOBAL)
        return ADF_CMP_DIFFERENT;

    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(N);
    fmpq_init(M);
    fb_center(a, x);
    fb_center(b, y);
    fb_radius(N, x);
    fb_radius(M, y);
    if (fmpq_is_zero(N) && fmpq_is_zero(M))
        res = fmpq_equal(a, b) ? ADF_CMP_EQUAL : ADF_CMP_DIFFERENT;
    else if (!adf_fball_overlaps(x, y))
        res = ADF_CMP_DIFFERENT;
    else
        res = ADF_CMP_UNDECIDED;
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    return res;
}
