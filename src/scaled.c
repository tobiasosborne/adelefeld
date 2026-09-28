/* adelefeld/scaled.c: the scaled-residue policy of SPEC 4.4 (work package 1.7, second part).

   Ground truth: docs/proofs/policies.md section 2 (Definition 4, line 107 to Corollary 12,
   line 241) and docs/proofs/precision.md Proposition 5 (line 94) and Proposition 6 (line 106);
   docs/conventions.md 5.4 (data and predicate), 4.6 (the shared-pointer rule, closure edit E1),
   3.2 (statuses), 4.1 (aliasing), 4.3 (outputs untouched on a failure); docs/SPEC.md 4.4
   ("Context compatibility for scaled values"); docs/reviews/m0-gate/closure.md C5 (the
   conversions report their loss).  Reference: tests/ref/adfref/policies.py.

   A value (conventions 5.4) is s (u + K Zhat) = s u + s K Zhat with s > 0 rational and
   0 <= u < K (exact = 0), or the exact rational s with u = 0 (exact = 1).  K is the modulus of
   the borrowed context; only adf_modctx_get_modulus is used on a context (its layout is not
   visible here).  The data (s, u) are determined by the set (policies Lemma 5, line 113), so
   every operation writes the unique data of its result set and no normalisation step exists.

   The rational gcd used throughout is the non-negative generator of the subgroup of Q generated
   by its arguments (docs/proofs/precision.md:9-11).  FLINT's fmpq_gcd computes exactly this:
   the canonicalisation of gcd(p s, q r)/(q s) for a = p/q, b = r/s
   (refs/src/flint-3.0.1/fmpq.rst:510-522), and fmpq_gcd_cofactors also returns the integer
   cofactors a/g and b/g (refs/src/flint-3.0.1/fmpq.rst:524-528; it requires canonical inputs,
   which conventions 5.1 and 5.4 guarantee).

   Aliasing (conventions 4.1): an output may be the same object as an input.  Every function
   that can fail computes into a private temporary and swaps it into the output last, so the
   output is untouched unless the function returns ADF_OK (conventions 4.3) and every permitted
   aliasing is safe.

   adf_scaled_set_fball reads its input through adf_fball_get_fmpz3, the canonical triple of the
   set, so a local input is converted as its set (the refusal of local inputs that this file had
   before work package 1.8 existed is removed; test set_fball_converts_a_local_value_as_its_set). */

#include "adelefeld/scaled.h"

#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/flint.h>

/* --------------------------------------------------------------- helpers */

/* g = gcd(a, b) as rationals, abar = a/g and bbar = b/g (integers): fmpq_gcd_cofactors. */
static void
sc_gcd_cofactors(fmpq_t g, fmpz_t abar, fmpz_t bbar, const fmpq_t a, const fmpq_t b)
{
    fmpq_gcd_cofactors(g, abar, bbar, a, b);
}

/* -------------------------------------------------------- adf_scaled_init, clear */

/* adf_scaled_init: the exact 0 in ctx (conventions 2.3, 5.4).  Never fails. */
void
adf_scaled_init(adf_scaled_t x, const adf_modctx_struct * ctx)
{
    fmpq_init(x->s);
    fmpq_zero(x->s);
    fmpz_init(x->u);
    fmpz_zero(x->u);
    x->mctx = ctx;
    x->exact = 1;
}

/* adf_scaled_clear: releases s and u; never touches the context (conventions 2.3, 4.2). */
void
adf_scaled_clear(adf_scaled_t x)
{
    fmpq_clear(x->s);
    fmpz_clear(x->u);
}

/* ------------------------------------------------------------ adf_scaled_set, swap */

/* adf_scaled_set: y = x, the context pointer included (conventions 2.3).  y may be x. */
void
adf_scaled_set(adf_scaled_t y, const adf_scaled_t x)
{
    fmpq_set(y->s, x->s);
    fmpz_set(y->u, x->u);
    y->mctx = x->mctx;
    y->exact = x->exact;
}

/* adf_scaled_swap: exchanges contents and context pointers; O(1) (conventions 2.3). */
void
adf_scaled_swap(adf_scaled_t x, adf_scaled_t y)
{
    const adf_modctx_struct * m;
    int e;

    fmpq_swap(x->s, y->s);
    fmpz_swap(x->u, y->u);
    m = x->mctx;
    x->mctx = y->mctx;
    y->mctx = m;
    e = x->exact;
    x->exact = y->exact;
    y->exact = e;
}

/* --------------------------------------------------------- the predicate and queries */

/* adf_scaled_is_canonical: the predicate of conventions 5.4; never aborts.  The comparison
   0 <= u < K reads the modulus of the context (adf_modctx_get_modulus). */
int
adf_scaled_is_canonical(const adf_scaled_t x)
{
    fmpz_t K;
    int ok;

    if (x->mctx == NULL)
        return 0;
    if (x->exact != 0 && x->exact != 1)
        return 0;
    if (!fmpq_is_canonical(x->s))
        return 0;
    if (x->exact == 1)
        return fmpz_is_zero(x->u);
    if (fmpq_sgn(x->s) <= 0)
        return 0;
    fmpz_init(K);
    adf_modctx_get_modulus(K, x->mctx);
    ok = (fmpz_sgn(x->u) >= 0) && (fmpz_cmp(x->u, K) < 0);
    fmpz_clear(K);
    return ok;
}

/* adf_scaled_identical: same context pointer, same exact tag, equal s and u (conventions 2.1).
   Representation identity; for equal sets of one context it coincides with equality of the data
   (policies Lemma 5, line 113). */
int
adf_scaled_identical(const adf_scaled_t x, const adf_scaled_t y)
{
    return x->mctx == y->mctx && x->exact == y->exact && fmpq_equal(x->s, y->s) &&
           fmpz_equal(x->u, y->u);
}

const adf_modctx_struct *
adf_scaled_context(const adf_scaled_t x)
{
    return x->mctx;
}

int
adf_scaled_is_exact(const adf_scaled_t x)
{
    return x->exact;
}

/* ---------------------------------------------------------------- adf_scaled_set_rat */

/* adf_scaled_set_rat: the exact value q in ctx (policies Definition 4, line 107).  Never
   fails. */
void
adf_scaled_set_rat(adf_scaled_t y, const adf_rat_t q, const adf_modctx_struct * ctx)
{
    fmpq_set(y->s, q->q);
    fmpz_zero(y->u);
    y->mctx = ctx;
    y->exact = 1;
}

/* ------------------------------------------------------------- adf_scaled_set_fball */

/* adf_scaled_set_fball: the best scaled enclosure of the tight ball x = c + R Zhat in ctx of
   modulus K (policies Proposition 7, line 139; Lemma 6, line 124; conventions 5.4 row
   "conversion from a tight ball"; closure C5).  R > 0: s* = gcd(c, R/K), u* = (c/s*) mod K;
   *lost = 1 exactly when c K/R is not an integer, that is when the set changes (Lemma 6).
   R = 0: the exact c with *lost = 0 (closure C5).  lost may be NULL.  Status: ADF_OK always.
   A local input is read through adf_fball_get_fmpz3, which gives the canonical triple of its
   set (fball.h; policies Proposition 24.1), so both backends are converted as sets. */
int
adf_scaled_set_fball(adf_scaled_t y, int * lost, const adf_fball_t x, const adf_modctx_struct * ctx)
{
    fmpz_t A, H, d, K, t, abar, bbar;
    fmpq_t c, rdivk, g;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(K);
    fmpz_init(t);
    fmpz_init(abar);
    fmpz_init(bbar);
    fmpq_init(c);
    fmpq_init(rdivk);
    fmpq_init(g);
    adf_fball_get_fmpz3(A, H, d, x);      /* the canonical triple (adf_fball.h) */
    adf_modctx_get_modulus(K, ctx);
    fmpq_set_fmpz_frac(c, A, d);          /* the centre c = A/d */
    if (fmpz_is_zero(H))
    {
        /* the exact point (closure C5): exact = 1, s = c, u = 0, *lost = 0 */
        fmpq_set(y->s, c);
        fmpz_zero(y->u);
        y->mctx = ctx;
        y->exact = 1;
        if (lost != NULL)
            *lost = 0;
    }
    else
    {
        /* s* = gcd(c, R/K) = gcd(A/d, H/(d K)); u* = (c/s*) mod K (Proposition 7) */
        fmpz_mul(t, d, K);
        fmpq_set_fmpz_frac(rdivk, H, t);
        sc_gcd_cofactors(g, abar, bbar, c, rdivk); /* abar = c/s*, the residue before mod K */
        fmpz_fdiv_r(t, abar, K);
        fmpq_set(y->s, g);
        fmpz_set(y->u, t);
        y->mctx = ctx;
        y->exact = 0;
        /* *lost = 1 exactly when c K/R = A K/H is not an integer */
        if (lost != NULL)
        {
            fmpz_mul(t, A, K);
            *lost = fmpz_divisible(t, H) ? 0 : 1;
        }
    }
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(K);
    fmpz_clear(t);
    fmpz_clear(abar);
    fmpz_clear(bbar);
    fmpq_clear(c);
    fmpq_clear(rdivk);
    fmpq_clear(g);
    return ADF_OK;
}

/* ----------------------------------------------------------- adf_scaled_set_context */

/* adf_scaled_set_context: closure finding C5 (conventions 5.4), formula policies
   Proposition 11 (line 218): for x = s (u + K Zhat) and ctx of modulus K',
   s' = s gcd(u, K/K'), u' = (s u/s') mod K'; *lost = 1 exactly when the set changes, that is
   when u K'/K is not an integer (item 2), always lossless when K divides K'.  An exact input
   keeps its rational s with exact = 1, u = 0 and *lost = 0 (C5).  y may alias x.  Status:
   ADF_OK always (conventions 3.2 row "Conversion between scaled contexts"). */
int
adf_scaled_set_context(adf_scaled_t y, int * lost, const adf_scaled_t x, const adf_modctx_struct * ctx)
{
    fmpz_t K, Kp, t, ubar;
    fmpq_t kk, uq, g, s2;

    if (x->exact == 1)
    {
        fmpq_set(y->s, x->s);
        fmpz_zero(y->u);
        y->mctx = ctx;
        y->exact = 1;
        if (lost != NULL)
            *lost = 0;
        return ADF_OK;
    }

    fmpz_init(K);
    fmpz_init(Kp);
    fmpz_init(t);
    fmpz_init(ubar);
    fmpq_init(kk);
    fmpq_init(uq);
    fmpq_init(g);
    fmpq_init(s2);
    adf_modctx_get_modulus(K, x->mctx);
    adf_modctx_get_modulus(Kp, ctx);
    fmpq_set_fmpz_frac(kk, K, Kp);            /* K/K' */
    fmpq_set_fmpz(uq, x->u);                  /* u */
    fmpq_gcd_cofactors(g, ubar, t, uq, kk);   /* g = gcd(u, K/K'), ubar = u/g */
    fmpq_mul(s2, x->s, g);                    /* s' = s gcd(u, K/K') (Proposition 11) */
    fmpz_fdiv_r(t, ubar, Kp);                 /* u' = (s u/s') mod K' = (u/g) mod K' */
    fmpq_set(y->s, s2);
    fmpz_set(y->u, t);
    y->mctx = ctx;
    y->exact = 0;
    /* *lost = 1 exactly when u K'/K is not an integer (Proposition 11(2)) */
    if (lost != NULL)
    {
        fmpz_mul(t, x->u, Kp);
        *lost = fmpz_divisible(t, K) ? 0 : 1;
    }
    fmpz_clear(K);
    fmpz_clear(Kp);
    fmpz_clear(t);
    fmpz_clear(ubar);
    fmpq_clear(kk);
    fmpq_clear(uq);
    fmpq_clear(g);
    fmpq_clear(s2);
    return ADF_OK;
}

/* ------------------------------------------------------------- adf_scaled_get_fball */

/* adf_scaled_get_fball: the set of x as a global canonical adf_fball (conventions 9.4 row
   "scaled value"): s u + s K Zhat, or the exact s.  Exact as a set; never fails. */
void
adf_scaled_get_fball(adf_fball_t y, const adf_scaled_t x)
{
    adf_rat_t c, r;
    fmpz_t K;

    if (x->exact == 1)
    {
        adf_rat_init(c);
        fmpq_set(c->q, x->s);
        adf_fball_set_rat(y, c);
        adf_rat_clear(c);
        return;
    }
    fmpz_init(K);
    adf_modctx_get_modulus(K, x->mctx);
    adf_rat_init(c);
    adf_rat_init(r);
    fmpq_mul_fmpz(c->q, x->s, x->u);   /* centre s u */
    fmpq_mul_fmpz(r->q, x->s, K);      /* radius s K */
    adf_fball_set_center_radius(y, c, r);
    adf_rat_clear(c);
    adf_rat_clear(r);
    fmpz_clear(K);
}

/* ------------------------------------------------------------------ adf_scaled_add */

/* adf_scaled_add: both scaled: g = gcd(s, t), A = s/g, B = t/g, z = g ((A u + B v) mod K +
   K Zhat), the tight sum (precision.md Proposition 5(1), line 99; SPEC 4.4).  One exact
   operand q: the best scaled enclosure of the tight sum (policies Proposition 8, line 159):
   q = 0 gives x unchanged, else g = gcd(s, q) and residue (q/g + (s/g) u) mod K.  Both exact:
   the exact sum with the tag kept (policies Definition 4).  Status: ADF_OK; ADF_DOMAIN on
   different context pointers, z untouched (conventions 4.6, closure E1).  Cost: one rational
   gcd and one reduction modulo K. */
int
adf_scaled_add(adf_scaled_t z, const adf_scaled_t x, const adf_scaled_t y)
{
    adf_scaled_struct t;
    fmpz_t K, r, ta, tb;
    fmpq_t g;

    if (x->mctx != y->mctx)
        return ADF_DOMAIN; /* the check precedes every write (SPEC 4.4) */

    adf_scaled_init(&t, x->mctx);
    fmpz_init(K);
    fmpz_init(r);
    fmpz_init(ta);
    fmpz_init(tb);
    fmpq_init(g);
    adf_modctx_get_modulus(K, x->mctx);

    if (x->exact == 1 && y->exact == 1)
    {
        t.exact = 1;
        fmpq_add(t.s, x->s, y->s);
    }
    else if (x->exact == 0 && y->exact == 0)
    {
        sc_gcd_cofactors(g, ta, tb, x->s, y->s);  /* g, A = s/g, B = t/g */
        fmpz_mul(r, ta, x->u);
        fmpz_mul(ta, tb, y->u);
        fmpz_add(r, r, ta);
        fmpz_fdiv_r(r, r, K);
        t.exact = 0;
        fmpq_set(t.s, g);
        fmpz_set(t.u, r);
    }
    else
    {
        /* one exact operand q and one scaled value w (policies Proposition 8) */
        const adf_scaled_struct * w = (x->exact == 0) ? x : y;
        const fmpq * q = (x->exact == 0) ? y->s : x->s;

        if (fmpq_is_zero(q))
        {
            t.exact = 0;
            fmpq_set(t.s, w->s);
            fmpz_set(t.u, w->u);
        }
        else
        {
            sc_gcd_cofactors(g, ta, tb, w->s, q); /* g = gcd(s, q), ta = s/g, tb = q/g */
            fmpz_mul(r, ta, w->u);
            fmpz_add(r, r, tb);                   /* q/g + (s/g) u */
            fmpz_fdiv_r(r, r, K);
            t.exact = 0;
            fmpq_set(t.s, g);
            fmpz_set(t.u, r);
        }
    }
    adf_scaled_swap(z, &t);
    adf_scaled_clear(&t);
    fmpz_clear(K);
    fmpz_clear(r);
    fmpz_clear(ta);
    fmpz_clear(tb);
    fmpq_clear(g);
    return ADF_OK;
}

/* ------------------------------------------------------------------ adf_scaled_sub */

/* adf_scaled_sub: z = x + (-y) with adf_scaled_neg (conventions 5.4 operations table).
   Statuses as adf_scaled_add: ADF_OK; ADF_DOMAIN on different context pointers, z untouched. */
int
adf_scaled_sub(adf_scaled_t z, const adf_scaled_t x, const adf_scaled_t y)
{
    adf_scaled_struct t;
    int status;

    if (x->mctx != y->mctx)
        return ADF_DOMAIN; /* the check precedes every write (SPEC 4.4) */
    adf_scaled_init(&t, x->mctx);
    adf_scaled_neg(&t, y);
    status = adf_scaled_add(z, x, &t);
    adf_scaled_clear(&t);
    return status;
}

/* ------------------------------------------------------------------ adf_scaled_mul */

/* adf_scaled_mul: the default product (DECISION CV-48, D5): both scaled: z = s t ((u v mod K)
   + K Zhat), an enclosure of the tight product that loses the factor h = gcd(u, v, K)
   (precision.md Proposition 5(2), line 100; policies Proposition 10.2, line 199).  One exact
   operand q: q x by policies Proposition 9 (line 184), exact as a set; q = 0 gives the exact 0.
   Both exact: the exact product.  Status: ADF_OK; ADF_DOMAIN on different context pointers. */
int
adf_scaled_mul(adf_scaled_t z, const adf_scaled_t x, const adf_scaled_t y)
{
    adf_scaled_struct t;
    fmpz_t K, r;

    if (x->mctx != y->mctx)
        return ADF_DOMAIN; /* the check precedes every write (SPEC 4.4) */

    adf_scaled_init(&t, x->mctx);
    fmpz_init(K);
    fmpz_init(r);
    adf_modctx_get_modulus(K, x->mctx);

    if (x->exact == 1 && y->exact == 1)
    {
        t.exact = 1;
        fmpq_mul(t.s, x->s, y->s);
    }
    else if (x->exact == 0 && y->exact == 0)
    {
        fmpz_mul(r, x->u, y->u);
        fmpz_fdiv_r(r, r, K);
        t.exact = 0;
        fmpq_mul(t.s, x->s, y->s);
        fmpz_set(t.u, r);
    }
    else
    {
        /* one exact factor q: |q| s ((sign(q) u mod K) + K Zhat), exact as a set (P9) */
        const adf_scaled_struct * w = (x->exact == 0) ? x : y;
        const fmpq * q = (x->exact == 0) ? y->s : x->s;

        if (fmpq_is_zero(q))
        {
            t.exact = 1; /* the exact 0 (precision.md Proposition 6(2)) */
            fmpq_zero(t.s);
        }
        else
        {
            fmpq_t aq;
            fmpq_init(aq);
            fmpq_abs(aq, q);
            fmpq_mul(t.s, aq, w->s);
            fmpq_clear(aq);
            fmpz_set(r, w->u);
            if (fmpq_sgn(q) < 0)
                fmpz_neg(r, r);
            fmpz_fdiv_r(r, r, K);
            t.exact = 0;
            fmpz_set(t.u, r);
        }
    }
    adf_scaled_swap(z, &t);
    adf_scaled_clear(&t);
    fmpz_clear(K);
    fmpz_clear(r);
    return ADF_OK;
}

/* -------------------------------------------------------------- adf_scaled_mul_tight */

/* adf_scaled_mul_tight: both scaled: z = (s t h) (((u v / h) mod K) + K Zhat),
   h = gcd(u, v, K), equal to the tight product (policies Proposition 10.3, line 200; proof
   line 206).  Exact operands as in adf_scaled_mul.  The shared-pointer check applies, exact
   operands included (conventions 4.6, closure E1).  Status: ADF_OK; ADF_DOMAIN on different
   context pointers.  Cost: one integer gcd at the size of K and one exact division. */
int
adf_scaled_mul_tight(adf_scaled_t z, const adf_scaled_t x, const adf_scaled_t y)
{
    adf_scaled_struct t;
    fmpz_t K, h, r;

    if (x->mctx != y->mctx)
        return ADF_DOMAIN; /* the check precedes every write (SPEC 4.4) */

    adf_scaled_init(&t, x->mctx);
    fmpz_init(K);
    fmpz_init(h);
    fmpz_init(r);
    adf_modctx_get_modulus(K, x->mctx);

    if (x->exact == 1 && y->exact == 1)
    {
        t.exact = 1;
        fmpq_mul(t.s, x->s, y->s);
    }
    else if (x->exact == 0 && y->exact == 0)
    {
        fmpz_gcd3(h, x->u, y->u, K);        /* h = gcd(u, v, K) >= 1 */
        fmpz_mul(r, x->u, y->u);
        fmpz_divexact(r, r, h);             /* h divides u, so it divides u v */
        fmpz_fdiv_r(r, r, K);
        t.exact = 0;
        fmpq_mul(t.s, x->s, y->s);
        fmpq_mul_fmpz(t.s, t.s, h);         /* scale s t h (Proposition 10.3) */
        fmpz_set(t.u, r);
    }
    else
    {
        /* one exact factor, as in adf_scaled_mul (policies Proposition 9) */
        const adf_scaled_struct * w = (x->exact == 0) ? x : y;
        const fmpq * q = (x->exact == 0) ? y->s : x->s;

        if (fmpq_is_zero(q))
        {
            t.exact = 1;
            fmpq_zero(t.s);
        }
        else
        {
            fmpq_t aq;
            fmpq_init(aq);
            fmpq_abs(aq, q);
            fmpq_mul(t.s, aq, w->s);
            fmpq_clear(aq);
            fmpz_set(r, w->u);
            if (fmpq_sgn(q) < 0)
                fmpz_neg(r, r);
            fmpz_fdiv_r(r, r, K);
            t.exact = 0;
            fmpz_set(t.u, r);
        }
    }
    adf_scaled_swap(z, &t);
    adf_scaled_clear(&t);
    fmpz_clear(K);
    fmpz_clear(h);
    fmpz_clear(r);
    return ADF_OK;
}

/* ------------------------------------------------------------------ adf_scaled_neg */

/* adf_scaled_neg: y = -x: s ((-u) mod K + K Zhat), or the exact -s.  Exact as a set
   (tests/ref/adfref/policies.py scaled_neg; policies Proposition 9 with q = -1).  Borrows x's
   context.  Never fails. */
void
adf_scaled_neg(adf_scaled_t y, const adf_scaled_t x)
{
    if (x->exact == 1)
    {
        fmpq_neg(y->s, x->s);
        fmpz_zero(y->u);
        y->mctx = x->mctx;
        y->exact = 1;
    }
    else
    {
        fmpz_t K;

        fmpz_init(K);
        adf_modctx_get_modulus(K, x->mctx);
        fmpq_set(y->s, x->s);
        fmpz_neg(y->u, x->u);
        fmpz_fdiv_r(y->u, y->u, K);
        y->mctx = x->mctx;
        y->exact = 0;
        fmpz_clear(K);
    }
}

/* --------------------------------------------------------------- adf_scaled_mul_rat */

/* adf_scaled_mul_rat: y = q x (policies Proposition 9, line 184): q != 0: |q| s ((sign(q) u
   mod K) + K Zhat), exact as a set; q = 0: the exact 0.  An exact x gives the exact q s.
   Borrows x's context.  Never fails. */
void
adf_scaled_mul_rat(adf_scaled_t y, const adf_scaled_t x, const adf_rat_t q)
{
    if (fmpq_is_zero(q->q))
    {
        fmpq_zero(y->s);
        fmpz_zero(y->u);
        y->mctx = x->mctx;
        y->exact = 1;
    }
    else if (x->exact == 1)
    {
        fmpq_mul(y->s, q->q, x->s);
        fmpz_zero(y->u);
        y->mctx = x->mctx;
        y->exact = 1;
    }
    else
    {
        fmpz_t K;
        fmpq_t aq;

        fmpz_init(K);
        fmpq_init(aq);
        adf_modctx_get_modulus(K, x->mctx);
        fmpq_abs(aq, q->q);
        fmpq_mul(y->s, aq, x->s);   /* aq first: y may alias x */
        fmpz_set(y->u, x->u);
        if (fmpq_sgn(q->q) < 0)
            fmpz_neg(y->u, y->u);
        fmpz_fdiv_r(y->u, y->u, K);
        y->mctx = x->mctx;
        y->exact = 0;
        fmpq_clear(aq);
        fmpz_clear(K);
    }
}

/* --------------------------------------------------------------- adf_scaled_add_rat */

/* adf_scaled_add_rat: y = x + q (policies Proposition 8, line 159): q = 0: y = x; else
   g = gcd(s, q), y = g ((q/g + (s/g) u) mod K + K Zhat), the best scaled enclosure of the
   tight sum; *lost = 1 exactly when s does not divide q (item 2 and 4: the result equals the
   tight sum exactly when s | q).  x exact: the exact sum with *lost = 0.  lost may be NULL.
   Borrows x's context.  Never fails. */
void
adf_scaled_add_rat(adf_scaled_t y, int * lost, const adf_scaled_t x, const adf_rat_t q)
{
    adf_scaled_struct t;
    fmpz_t K, r, ta, tb;
    fmpq_t g, quot;

    adf_scaled_init(&t, x->mctx);
    if (x->exact == 1)
    {
        t.exact = 1;
        fmpq_add(t.s, x->s, q->q);
        if (lost != NULL)
            *lost = 0;
    }
    else if (fmpq_is_zero(q->q))
    {
        t.exact = 0;
        fmpq_set(t.s, x->s);
        fmpz_set(t.u, x->u);
        if (lost != NULL)
            *lost = 0;
    }
    else
    {
        fmpz_init(K);
        fmpz_init(r);
        fmpz_init(ta);
        fmpz_init(tb);
        fmpq_init(g);
        fmpq_init(quot);
        adf_modctx_get_modulus(K, x->mctx);
        sc_gcd_cofactors(g, ta, tb, x->s, q->q); /* g = gcd(s, q), ta = s/g, tb = q/g */
        fmpz_mul(r, ta, x->u);
        fmpz_add(r, r, tb);                      /* q/g + (s/g) u */
        fmpz_fdiv_r(r, r, K);
        t.exact = 0;
        fmpq_set(t.s, g);
        fmpz_set(t.u, r);
        /* *lost = 1 exactly when s does not divide q, that is when q/s is not an integer */
        if (lost != NULL)
        {
            fmpq_div(quot, q->q, x->s);
            *lost = fmpz_is_one(fmpq_denref(quot)) ? 0 : 1;
        }
        fmpz_clear(K);
        fmpz_clear(r);
        fmpz_clear(ta);
        fmpz_clear(tb);
        fmpq_clear(g);
        fmpq_clear(quot);
    }
    adf_scaled_swap(y, &t);
    adf_scaled_clear(&t);
}
