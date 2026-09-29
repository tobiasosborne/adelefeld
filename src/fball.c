/* adelefeld/fball.c: adf_fball, the finite ball (A + H Zhat)/d, tight policy, both backends.

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

   Local backend (ADF_LOCAL, work package 1.8; lane m1-local). A local value (d; r_1..r_k) of a
   context with blocks q_i and modulus K is the set (A + K Zhat)/d, A any integer with
   A = r_i mod q_i (docs/proofs/policies.md Definition 16, line 305; Lemma 17, line 312); it is
   stored raw, as predicate L of fball.h and docs/conventions.md 5.3 say (CV-55). The rules here:
   - Operations that conventions 5.3 keeps local, when both local inputs share one context
     pointer: negation (policies P21.1, line 390), sum and difference (P21.2, line 391), the
     product when h = 1 blockwise or when h divides d e (P22, line 407), the product with an
     exact scalar m/n when |m| divides d (P23, line 443). They work on the residues and multiply
     or divide the denominators only as integers (policies P25.4, line 508; no inverse modulo a
     block is computed in this file).
   - Every other case (a global input, two context pointers, a product whose tight set needs other
     blocks) is computed on the canonical global forms and gives a global result (the implicit
     fallback of conventions 4.6 and 5.3; gate finding G1). The global form of a local value is
     adf_fball_set_global of src/fball_local.c (recombination and canonical cancellation,
     policies Lemma 17.3, P24.1).
   - Predicates, comparisons and accessors go through the canonical triple (policies P24.4,
     line 470; conventions 5.3), never through raw residues.
   The global code below is that of lane m1-fball, unchanged for global inputs.

   Aliasing (conventions 4.1): an output may be the same object as any input of the same type.
   Every operation reads its inputs into temporaries and writes the output last; the residue
   loops read res[i] of the inputs before they write res[i] of the output, which may be the same
   array. */

#include "adelefeld/fball.h"
#include "adelefeld/modctx.h"

#include <string.h>

#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/flint.h>
#include <flint/ulong_extras.h>
#include "invariants.h"

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
    ADF_INV_RELEASE(x->mctx);
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

/* ------------------------------------------------- helpers, local backend */

/* The value to compute with: x itself when it is global; for a local x, its canonical global form
   written into tmp (initialised by the caller, who clears it). docs/conventions.md 5.3:
   "Comparison and printing go through the canonical triple". */
static const adf_fball_struct *
fb_glob(adf_fball_struct * tmp, const adf_fball_struct * x)
{
    if (x->backend != ADF_LOCAL)
        return x;
    adf_fball_set_global(tmp, x);
    return tmp;
}

/* 1 if x and y are local values of one context pointer (conventions 4.6: another pointer gives a
   global result even when the blocks agree).
   HEADER-FINDING: fball.h keeps a result local "when all local inputs share one context pointer";
   read literally, a sum of one local and one global input would qualify. The table of
   conventions 5.3 (negation, sum, product of two local values; exact scalar) and policies P21 to
   P23 are stated for two local inputs only, so a binary operation with one global input gives a
   global result here, even when the result set would be representable in the context. */
static int
fb_same_local(const adf_fball_t x, const adf_fball_t y)
{
    return x->backend == ADF_LOCAL && y->backend == ADF_LOCAL && x->mctx == y->mctx;
}

/* The residue array that z will hold as a local value of ctx: z's own array if z is already a
   local value of ctx (then it may be the array of an input, which the caller reads index by index
   before writing), else a resized array of k words. */
static ulong *
fb_local_res(adf_fball_t z, const adf_modctx_struct * ctx)
{
    if (!(z->backend == ADF_LOCAL && z->mctx == ctx))
        z->res = (ulong *) flint_realloc(z->res, adf_modctx_nblocks(ctx) * sizeof(ulong));
    return z->res;
}

/* Complete z as the local value (d; z->res) of ctx: A = 0, H = K (predicate L). d may be a
   temporary or z->d. */
static void
fb_local_finish(adf_fball_t z, const adf_modctx_struct * ctx, const fmpz_t d)
{
    fmpz_set(z->d, d);
    fmpz_zero(z->A);
    adf_modctx_get_modulus(z->H, ctx);
    z->backend = ADF_LOCAL;
    ADF_INV_RETARGET(z->mctx, ctx);
    z->mctx = ctx;
}

/* z = x + y (sub = 0) or x - y (sub = 1), x and y local at one context: policies P21.2
   (line 391), (L; (r_i L/d + s_i L/e) mod q_i), L = lcm(d, e); the difference with -s_i
   (P21.1). Exact and tight; the raw form keeps K. */
static void
fb_local_addsub(adf_fball_t z, const adf_fball_t x, const adf_fball_t y, int sub)
{
    const adf_modctx_struct * ctx = x->mctx;
    slong i, k = adf_modctx_nblocks(ctx);
    fmpz_t L, u, v;
    ulong * out;

    fmpz_init(L);
    fmpz_init(u);
    fmpz_init(v);
    fmpz_lcm(L, x->d, y->d);
    fmpz_divexact(u, L, x->d);
    fmpz_divexact(v, L, y->d);
    out = fb_local_res(z, ctx);
    for (i = 0; i < k; i++)
    {
        ulong q = adf_modctx_block(ctx, i);
        ulong qinv = n_preinvert_limb(q);
        ulong a = n_mulmod2_preinv(x->res[i], fmpz_fdiv_ui(u, q), q, qinv);
        ulong b = n_mulmod2_preinv(y->res[i], fmpz_fdiv_ui(v, q), q, qinv);
        out[i] = sub ? n_submod(a, b, q) : n_addmod(a, b, q);
    }
    fb_local_finish(z, ctx, L);
    fmpz_clear(L);
    fmpz_clear(u);
    fmpz_clear(v);
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
    ADF_INV_RELEASE(x->mctx);
    x->mctx = NULL;
    x->backend = ADF_GLOBAL;
}

/* y = a copy of x: same backend, context pointer and fields (conventions 2.3). For a local x the
   k residues are copied into an array owned by y (conventions 4.2). */
void
adf_fball_set(adf_fball_t y, const adf_fball_t x)
{
    ADF_INV_FBALL(x);
    if (x->backend == ADF_LOCAL)
    {
        slong k = adf_modctx_nblocks(x->mctx);
        if (y == x)
            return;
        y->res = (ulong *) flint_realloc(y->res, k * sizeof(ulong));
        memcpy(y->res, x->res, k * sizeof(ulong));
        fmpz_set(y->A, x->A);
        fmpz_set(y->H, x->H);
        fmpz_set(y->d, x->d);
        y->backend = ADF_LOCAL;
        ADF_INV_RETARGET(y->mctx, x->mctx);
        y->mctx = x->mctx;
        return;
    }
    flint_free(y->res);
    y->res = NULL;
    fmpz_set(y->A, x->A);
    fmpz_set(y->H, x->H);
    fmpz_set(y->d, x->d);
    y->backend = ADF_GLOBAL;
    ADF_INV_RELEASE(y->mctx);
    y->mctx = NULL;
}

void
adf_fball_swap(adf_fball_t x, adf_fball_t y)
{
    ADF_INV_FBALL(x);
    ADF_INV_FBALL(y);
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

    /* Predicate L (fball.h; conventions 5.3): mctx != NULL, k >= 1, H = K, A = 0, res != NULL,
       d >= 1, 0 <= res[i] < q_i. The pointers are checked before the context is read.
       HEADER-FINDING: "never aborts, whatever the fields hold" cannot cover a non-NULL mctx that
       does not point to a context; the blocks are read through it. */
    if (x->backend != ADF_LOCAL)
        return 0;
    if (x->mctx == NULL || x->res == NULL)
        return 0;
    if (!fmpz_is_zero(x->A))
        return 0;
    if (fmpz_sgn(x->d) < 1)
        return 0;
    {
        slong i, k = adf_modctx_nblocks(x->mctx);
        if (k < 1)
            return 0;
        fmpz_init(g);
        adf_modctx_get_modulus(g, x->mctx);
        ok = fmpz_equal(x->H, g);
        fmpz_clear(g);
        for (i = 0; ok && i < k; i++)
            ok = x->res[i] < adf_modctx_block(x->mctx, i);
        return ok;
    }
}

int
adf_fball_identical(const adf_fball_t x, const adf_fball_t y)
{
    ADF_INV_FBALL(x);
    ADF_INV_FBALL(y);
    if (x->backend != y->backend || x->mctx != y->mctx)
        return 0;
    if (!fmpz_equal(x->A, y->A) || !fmpz_equal(x->H, y->H) || !fmpz_equal(x->d, y->d))
        return 0;
    /* A local value: also res[0..k-1] (fball.h; conventions 2.1, CV-02). Both share mctx here. */
    if (x->backend == ADF_LOCAL)
    {
        slong k = adf_modctx_nblocks(x->mctx);
        return memcmp(x->res, y->res, k * sizeof(ulong)) == 0;
    }
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
    ADF_INV_RELEASE(x->mctx);
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
    ADF_INV_RELEASE(x->mctx);
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
    ADF_INV_RELEASE(x->mctx);
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
    ADF_INV_RELEASE(x->mctx);
    x->mctx = NULL;
    flint_free(x->res);
    x->res = NULL;
}

void
adf_fball_set_rat(adf_fball_t x, const adf_rat_t q)
{
    ADF_INV_RAT(q);
    fmpz_set(x->A, fmpq_numref(q->q));
    fmpz_zero(x->H);
    fmpz_set(x->d, fmpq_denref(q->q));
    x->backend = ADF_GLOBAL;
    ADF_INV_RELEASE(x->mctx);
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
    ADF_INV_RAT(c);
    ADF_INV_RAT(N);
    if (fmpq_sgn(N->q) < 0)
        return ADF_DOMAIN;
    fb_set_cr(x, c->q, N->q);
    return ADF_OK;
}

int
adf_fball_canonicalise(adf_fball_t x)
{
    fmpz_t A, H, d;

#ifdef ADF_CHECK_INVARIANTS
    /* A global x is raw data by contract (fball.h:138-141) and is not checked; a local x "must
       already satisfy L" (fball.h:141-143) and is. */
    if (x->backend == ADF_LOCAL)
        ADF_INV_FBALL(x);
#endif
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
    ADF_INV_FBALL(x);
    return fmpz_is_zero(x->H);
}

/* The canonical triple of the set. For a local value: (A0/g, K/g, d/g), A0 the CRT lift in
   [0, K), g = gcd(A0, K, d) (fball.h; policies P24.1, line 457; Lemma 18, line 331), through
   adf_fball_set_global. */
void
adf_fball_get_fmpz3(fmpz_t A, fmpz_t H, fmpz_t d, const adf_fball_t x)
{
    ADF_INV_FBALL(x);
    adf_fball_t t;
    const adf_fball_struct * g;

    adf_fball_init(t);
    g = fb_glob(t, x);
    fmpz_set(A, g->A);
    fmpz_set(H, g->H);
    fmpz_set(d, g->d);
    adf_fball_clear(t);
}

/* The stored centre of the canonical triple (conventions 5.2), for either backend. */
void
adf_fball_get_center(adf_rat_t c, const adf_fball_t x)
{
    ADF_INV_FBALL(x);
    adf_fball_t t;
    const adf_fball_struct * g;

    adf_fball_init(t);
    g = fb_glob(t, x);
    fmpq_set_fmpz_frac(c->q, g->A, g->d);
    adf_fball_clear(t);
}

/* N = H/d in lowest terms. For a local value H = K and N = K/d is the radius of the set (policies
   Definition 16); fmpq_set_fmpz_frac cancels, so the raw fields give the canonical N. */
void
adf_fball_get_radius(adf_rat_t N, const adf_fball_t x)
{
    ADF_INV_FBALL(x);
    fmpq_set_fmpz_frac(N->q, x->H, x->d);
}

/* The denominator of the canonical triple. For a local value it is d/g, g = gcd(A0, K, d), not the
   raw d (policies P24: (2; 2) in (4) has the raw d = 2 and the canonical d = 1). */
void
adf_fball_get_den(fmpz_t d, const adf_fball_t x)
{
    ADF_INV_FBALL(x);
    adf_fball_t t;
    const adf_fball_struct * g;

    adf_fball_init(t);
    g = fb_glob(t, x);
    fmpz_set(d, g->d);
    adf_fball_clear(t);
}

/* v_p(N) = v_p(H) - v_p(d); for a local value H = K, and K/d is the radius of its set, so the
   raw fields give v_p of the canonical radius (the valuation of a quotient does not depend on
   the representation). */
int
adf_fball_prec_at(slong * e, const adf_fball_t x, adf_place_t v)
{
    ADF_INV_FBALL(x);
    fmpz_t p, t;
    slong vH, vd;

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

/* 1/N = d/H; for a local value H = K and N = K/d is the radius of its set. */
void
adf_fball_haar_volume(adf_rat_t vol, const adf_fball_t x)
{
    ADF_INV_FBALL(x);
    if (fmpz_is_zero(x->H))
        fmpq_zero(vol->q);
    else
        fmpq_set_fmpz_frac(vol->q, x->d, x->H);
}

/* ------------------------------------------------------ tight arithmetic */

/* The global rules take the canonical global forms xg, yg (x, y themselves when global). */

void
adf_fball_add(adf_fball_t z, const adf_fball_t x, const adf_fball_t y)
{
    ADF_INV_FBALL(x);
    ADF_INV_FBALL(y);
    fmpq_t a, b, N, M, c, r;
    adf_fball_t tx, ty;
    const adf_fball_struct * xg;
    const adf_fball_struct * yg;

    /* Both local at one context pointer: the local sum (policies P21.2; conventions 5.3). */
    if (fb_same_local(x, y))
    {
        fb_local_addsub(z, x, y, 0);
        return;
    }
    adf_fball_init(tx);
    adf_fball_init(ty);
    xg = fb_glob(tx, x);
    yg = fb_glob(ty, y);
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(N);
    fmpq_init(M);
    fmpq_init(c);
    fmpq_init(r);
    fb_center(a, xg);
    fb_radius(N, xg);
    fb_center(b, yg);
    fb_radius(M, yg);
    fmpq_add(c, a, b);
    fmpq_gcd(r, N, M);
    fb_set_cr(z, c, r);
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    fmpq_clear(c);
    fmpq_clear(r);
    adf_fball_clear(tx);
    adf_fball_clear(ty);
}

void
adf_fball_sub(adf_fball_t z, const adf_fball_t x, const adf_fball_t y)
{
    ADF_INV_FBALL(x);
    ADF_INV_FBALL(y);
    fmpq_t a, b, N, M, c, r;
    adf_fball_t tx, ty;
    const adf_fball_struct * xg;
    const adf_fball_struct * yg;

    /* Both local at one context pointer: x + (-y), policies P21.1 and P21.2. */
    if (fb_same_local(x, y))
    {
        fb_local_addsub(z, x, y, 1);
        return;
    }
    adf_fball_init(tx);
    adf_fball_init(ty);
    xg = fb_glob(tx, x);
    yg = fb_glob(ty, y);
    fmpq_init(a);
    fmpq_init(b);
    fmpq_init(N);
    fmpq_init(M);
    fmpq_init(c);
    fmpq_init(r);
    fb_center(a, xg);
    fb_radius(N, xg);
    fb_center(b, yg);
    fb_radius(M, yg);
    fmpq_sub(c, a, b);
    fmpq_gcd(r, N, M);
    fb_set_cr(z, c, r);
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    fmpq_clear(c);
    fmpq_clear(r);
    adf_fball_clear(tx);
    adf_fball_clear(ty);
}

void
adf_fball_neg(adf_fball_t y, const adf_fball_t x)
{
    ADF_INV_FBALL(x);
    fmpq_t a, N, c, r;

    /* A local x: (d; -r_i mod q_i), the same context, exact (policies P21.1, line 390). */
    if (x->backend == ADF_LOCAL)
    {
        const adf_modctx_struct * ctx = x->mctx;
        slong i, k = adf_modctx_nblocks(ctx);
        ulong * out = fb_local_res(y, ctx);
        for (i = 0; i < k; i++)
            out[i] = n_negmod(x->res[i], adf_modctx_block(ctx, i));
        fb_local_finish(y, ctx, x->d);
        return;
    }
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
static void
fb_mul_global(adf_fball_t z, const adf_fball_struct * x, const adf_fball_struct * y)
{
    fmpq_t a, b, N, M, c, g, t1, t2, t3;

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

/* Local inputs at one context (policies P22, line 407; conventions 5.3 row "product"):
   h_i = gcd(r_i, s_i, q_i), h = product of h_i = gcd(A, B, K) (P22.1).
   - h = 1 (every h_i = 1): the blockwise product (d e; r_i s_i mod q_i) is the tight product
     (P22.2) and local.
   - h > 1: the tight product (A B + K h Zhat)/(d e) (P22.1) is computed on the global forms
     (precision.md Proposition 2 is the same tight ball) and then converted into the context by
     adf_fball_set_local, which succeeds exactly when the set lives in the context
     (Proposition 19), that is exactly when h divides d e (P22.3); then the result is the unique
     local value of that set (Lemma 17.2), which is the value of P22.3. Otherwise it stays global
     (the fallback of conventions 4.6, 5.3). The blockwise product with h > 1 is only an
     enclosure (P22.2) and is never returned. */
void
adf_fball_mul(adf_fball_t z, const adf_fball_t x, const adf_fball_t y)
{
    ADF_INV_FBALL(x);
    ADF_INV_FBALL(y);
    adf_fball_t tx, ty, t;
    const adf_fball_struct * xg;
    const adf_fball_struct * yg;

    if (fb_same_local(x, y))
    {
        const adf_modctx_struct * ctx = x->mctx;
        slong i, k = adf_modctx_nblocks(ctx);
        int h_is_one = 1;

        for (i = 0; i < k && h_is_one; i++)
            h_is_one = n_gcd(n_gcd(x->res[i], y->res[i]), adf_modctx_block(ctx, i)) == 1;
        if (h_is_one)
        {
            fmpz_t de;
            ulong * out;
            fmpz_init(de);
            fmpz_mul(de, x->d, y->d);
            out = fb_local_res(z, ctx);
            for (i = 0; i < k; i++)
            {
                ulong q = adf_modctx_block(ctx, i);
                out[i] = n_mulmod2_preinv(x->res[i], y->res[i], q, n_preinvert_limb(q));
            }
            fb_local_finish(z, ctx, de);
            fmpz_clear(de);
            return;
        }
        adf_fball_init(tx);
        adf_fball_init(ty);
        adf_fball_init(t);
        fb_mul_global(t, fb_glob(tx, x), fb_glob(ty, y));
        if (adf_fball_set_local(z, t, ctx) != ADF_OK)
            adf_fball_swap(z, t);
        adf_fball_clear(tx);
        adf_fball_clear(ty);
        adf_fball_clear(t);
        return;
    }
    adf_fball_init(tx);
    adf_fball_init(ty);
    xg = fb_glob(tx, x);
    yg = fb_glob(ty, y);
    fb_mul_global(z, xg, yg);
    adf_fball_clear(tx);
    adf_fball_clear(ty);
}

/* q x = q a + |q| N Zhat (fball.h). A local x and q = m/n with m != 0 and |m| dividing d: the
   local (n d/|m|; sign(m) r_i mod q_i) of the same context (policies P23, line 443; conventions
   5.3 row "exact scalar"); otherwise the global rule on the global form (q = 0: the exact 0). */
void
adf_fball_mul_rat(adf_fball_t y, const adf_fball_t x, const adf_rat_t q)
{
    ADF_INV_FBALL(x);
    ADF_INV_RAT(q);
    fmpq_t a, N, c, r;
    adf_fball_t tx;
    const adf_fball_struct * xg;

    if (x->backend == ADF_LOCAL && !fmpq_is_zero(q->q) && fmpz_divisible(x->d, fmpq_numref(q->q)))
    {
        const adf_modctx_struct * ctx = x->mctx;
        slong i, k = adf_modctx_nblocks(ctx);
        int neg = fmpz_sgn(fmpq_numref(q->q)) < 0;
        fmpz_t dn;
        ulong * out;

        fmpz_init(dn);
        fmpz_divexact(dn, x->d, fmpq_numref(q->q));
        fmpz_abs(dn, dn);
        fmpz_mul(dn, dn, fmpq_denref(q->q));
        out = fb_local_res(y, ctx);
        for (i = 0; i < k; i++)
            out[i] = neg ? n_negmod(x->res[i], adf_modctx_block(ctx, i)) : x->res[i];
        fb_local_finish(y, ctx, dn);
        fmpz_clear(dn);
        return;
    }
    adf_fball_init(tx);
    xg = fb_glob(tx, x);
    fmpq_init(a);
    fmpq_init(N);
    fmpq_init(c);
    fmpq_init(r);
    fb_center(a, xg);
    fb_radius(N, xg);
    fmpq_mul(c, a, q->q);
    fmpq_abs(r, q->q);
    fmpq_mul(r, r, N);
    fb_set_cr(y, c, r);
    fmpq_clear(a);
    fmpq_clear(N);
    fmpq_clear(c);
    fmpq_clear(r);
    adf_fball_clear(tx);
}

/* y = (1/q) x, as adf_fball_mul_rat with 1/q (fball.h); q = 0: ADF_NOT_UNIT, y untouched. For a
   global x this is the computation of lane m1-fball: centre a/q, radius N/|q|. */
int
adf_fball_div_rat(adf_fball_t y, const adf_fball_t x, const adf_rat_t q)
{
    ADF_INV_FBALL(x);
    ADF_INV_RAT(q);
    adf_rat_struct qq;

    if (fmpq_is_zero(q->q))
        return ADF_NOT_UNIT;
    fmpq_init(qq.q);
    fmpq_inv(qq.q, q->q);
    adf_fball_mul_rat(y, x, &qq);
    fmpq_clear(qq.q);
    return ADF_OK;
}

/* -------------------------------------------------------- set predicates */

/* Every predicate compares sets through the canonical triples (conventions 5.3; policies P24.4):
   a local input is replaced by its global form first. */

static int
fb_equal_set_g(const adf_fball_struct * x, const adf_fball_struct * y)
{
    fmpq_t a, b, N, M, diff, t;
    int res;

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
adf_fball_equal_set(const adf_fball_t x, const adf_fball_t y)
{
    ADF_INV_FBALL(x);
    ADF_INV_FBALL(y);
    adf_fball_t tx, ty;
    int res;

    adf_fball_init(tx);
    adf_fball_init(ty);
    res = fb_equal_set_g(fb_glob(tx, x), fb_glob(ty, y));
    adf_fball_clear(tx);
    adf_fball_clear(ty);
    return res;
}

static int
fb_overlaps_g(const adf_fball_struct * x, const adf_fball_struct * y)
{
    fmpq_t a, b, N, M, g, diff, t;
    int res;

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
adf_fball_overlaps(const adf_fball_t x, const adf_fball_t y)
{
    ADF_INV_FBALL(x);
    ADF_INV_FBALL(y);
    adf_fball_t tx, ty;
    int res;

    adf_fball_init(tx);
    adf_fball_init(ty);
    res = fb_overlaps_g(fb_glob(tx, x), fb_glob(ty, y));
    adf_fball_clear(tx);
    adf_fball_clear(ty);
    return res;
}

static int
fb_contains_g(const adf_fball_struct * x, const adf_fball_struct * y)
{
    fmpq_t a, b, N, M, diff, t;
    int res;

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
adf_fball_contains(const adf_fball_t x, const adf_fball_t y)
{
    ADF_INV_FBALL(x);
    ADF_INV_FBALL(y);
    adf_fball_t tx, ty;
    int res;

    adf_fball_init(tx);
    adf_fball_init(ty);
    res = fb_contains_g(fb_glob(tx, x), fb_glob(ty, y));
    adf_fball_clear(tx);
    adf_fball_clear(ty);
    return res;
}

int
adf_fball_contains_rat(const adf_fball_t x0, const adf_rat_t q)
{
    ADF_INV_FBALL_NAMED("x", x0);
    ADF_INV_RAT(q);
    fmpq_t a, N, diff, t;
    adf_fball_t tx;
    const adf_fball_struct * x;
    int res;

    adf_fball_init(tx);
    x = fb_glob(tx, x0);
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
    adf_fball_clear(tx);
    return res;
}

int
adf_fball_compare(const adf_fball_t x0, const adf_fball_t y0)
{
    ADF_INV_FBALL_NAMED("x", x0);
    ADF_INV_FBALL_NAMED("y", y0);
    fmpq_t a, b, N, M;
    adf_fball_t tx, ty;
    const adf_fball_struct * x;
    const adf_fball_struct * y;
    int res;

    adf_fball_init(tx);
    adf_fball_init(ty);
    x = fb_glob(tx, x0);
    y = fb_glob(ty, y0);
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
    else if (!fb_overlaps_g(x, y))
        res = ADF_CMP_DIFFERENT;
    else
        res = ADF_CMP_UNDECIDED;
    fmpq_clear(a);
    fmpq_clear(b);
    fmpq_clear(N);
    fmpq_clear(M);
    adf_fball_clear(tx);
    adf_fball_clear(ty);
    return res;
}
