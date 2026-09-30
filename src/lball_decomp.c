/* lball_decomp.c: the rest of work package 1F.3 (lane f-slice3; include/adelefeld/lball.h, section "slice 1F.3-b"):
   adf_lball_teichmuller, adf_lball_decompose_teich, adf_lball_frac, adf_lball_unit_mod. (adf_lball_pow_si is in
   lball.c: it needs the reduction lb_make.)

   Sources, read before the code (CLAUDE.md rules 3 and 4):
     docs/proofs/functions.md Lemma 3 item 2 (statement line 58, proof line 72): a simple root of f in Z_p[T] modulo
     p lifts uniquely; Proposition 4 (line 92) with steps 1 and 2 (lines 102 to 113): the decomposition x = p^m w u,
     w the root of T^(p-1) - 1 with the residue of x/p^m (odd p), w = +-1 by x/p^m modulo 4 (p = 2); Proposition 19
     (line 656): {x}_p = a/p^k with a = p^k x modulo p^k, k = -v_p(x);
     docs/api-1f.md L9 to L13 (Newton doubling, the split of a ball, the fractional part, the unit modulo p^k, the
     powers). FLINT functions used, from refs/src/flint-3.0.1/fmpz.rst: fmpz_powm_ui, fmpz_invmod (as in lball.c),
     fmpz_mod, fmpz_fdiv_ui.

   Every function computes into temporaries and writes its outputs only on ADF_OK (conventions 4.3), so aliasing of
   an output with an input is free. The values written are canonical by construction (the checks are the tests). */

#include <limits.h>

#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include "invariants.h"

#ifdef ADF_CHECK_INVARIANTS
#include <stdio.h>
#include <flint/flint.h>
#define ADF_INV_LBALL(x)                                                                                    \
    do                                                                                                      \
    {                                                                                                       \
        if (!adf_lball_is_canonical(x))                                                                     \
        {                                                                                                   \
            fprintf(stderr, "adelefeld: ADF_CHECK_INVARIANTS: %s: argument %s is not a canonical adf_lball\n", \
                    __func__, #x);                                                                          \
            fflush(stderr);                                                                                 \
            flint_abort();                                                                                  \
        }                                                                                                   \
    }                                                                                                       \
    while (0)
#else
#define ADF_INV_LBALL(x) ((void) 0)
#endif

#define EXP_MAX  ADF_LBALL_EXP_MAX
#define BITS_MAX ADF_LBALL_BITS_MAX

static int
exp_ok(slong a)
{
    return a >= -EXP_MAX && a <= EXP_MAX;
}

static int
in_bounds(const adf_lball_struct * x)
{
    return exp_ok(x->v) && exp_ok(x->N);
}

/* 1 if p^k, k >= 0, has at most BITS_MAX bits (p^k has at most k bits(p) bits). As in lball.c. */
static int
pow_ok(ulong p, slong k)
{
    return k >= 0 && k <= BITS_MAX / (slong) FLINT_BIT_COUNT(p);
}

static void
pow_p(fmpz_t out, ulong p, ulong k)
{
    fmpz_set_ui(out, p);
    fmpz_pow_ui(out, out, k);
}

/* The unchecked swap: an output that is overwritten is not checked (lball.h, common rules). */
static void
lb_swap(adf_lball_struct * x, adf_lball_struct * y)
{
    ulong p = x->p;
    slong v = x->v, N = x->N;
    int e = x->exact;
    x->p = y->p; x->v = y->v; x->N = y->N; x->exact = y->exact;
    y->p = p; y->v = v; y->N = N; y->exact = e;
    fmpq_swap(x->u, y->u);
}

/* The exact value +1 or -1 at p. */
static void
set_exact_pm1(adf_lball_struct * w, ulong p, int negative)
{
    w->p = p;
    fmpz_set_si(fmpq_numref(w->u), negative ? -1 : 1);
    fmpz_one(fmpq_denref(w->u));
    w->v = 0;
    w->N = 0;
    w->exact = 1;
}

/* The unit ball c + p^n Z_p (v = 0), c an integer in (0, p^n) prime to p. */
static void
set_unit_ball(adf_lball_struct * w, ulong p, fmpz_t c, slong n)
{
    w->p = p;
    fmpz_swap(fmpq_numref(w->u), c);
    fmpz_one(fmpq_denref(w->u));
    w->v = 0;
    w->N = n;
    w->exact = 0;
}

/* omega = the root of T^(p-1) - 1 in Z_p with omega = r modulo p, modulo p^K, in [0, p^K): p odd, 1 <= r < p, K >=
   1. Lemma 3 item 2 (line 72) lifts one digit at a time; here Newton's method with the precision doubled (api-1f.md
   L9): f(T) = T^(p-1) - 1, f'(T) = (p-1) T^(p-2), a unit at any T = r modulo p. If w is an integer with f(w) = 0
   modulo p^j then h = -f(w)/f'(w) lies in p^j Z_p and f(w + h) = h^2 G, G in Z[w, h], so w + h is a root modulo
   p^(2j), and it is w modulo p^j (so the same root: uniqueness of Lemma 3). The division is a modular inverse
   modulo p^(min(2j, K)). */
static void
teich_mod(fmpz_t w, ulong p, ulong r, ulong K)
{
    fmpz_t P, t, d, fw;
    ulong cur = 1;
    fmpz_init(P);
    fmpz_init(t);
    fmpz_init(d);
    fmpz_init(fw);
    fmpz_set_ui(w, r);
    while (cur < K)
    {
        ulong nxt = cur <= K / 2 ? 2 * cur : K;
        int ok;
        pow_p(P, p, nxt);
        fmpz_powm_ui(t, w, p - 2, P);               /* w^(p-2) */
        fmpz_mul(fw, t, w);                         /* w^(p-1) mod p^nxt after the reduction below */
        fmpz_sub_ui(fw, fw, 1);
        fmpz_mod(fw, fw, P);
        fmpz_mul_ui(d, t, p - 1);
        ok = fmpz_invmod(d, d, P);
        (void) ok;                                  /* the derivative is a unit */
        fmpz_mul(fw, fw, d);
        fmpz_sub(w, w, fw);
        fmpz_mod(w, w, P);
        cur = nxt;
    }
    fmpz_clear(P);
    fmpz_clear(t);
    fmpz_clear(d);
    fmpz_clear(fw);
}

int
adf_lball_teichmuller(adf_lball_t w, adf_place_t v, ulong r, slong prec)
{
    ulong p, rr;
    slong n = prec < 1 ? 1 : prec;
    if (adf_place_is_archimedean(v))
        return ADF_DOMAIN;
    p = adf_place_prime_get(v);
    rr = r % p;
    if (rr == 0)
        return ADF_DOMAIN;
    if (p == 2 || rr == 1 || rr == p - 1)
    {
        adf_lball_t res;
        adf_lball_init(res);
        set_exact_pm1(res, p, p != 2 && rr == p - 1);       /* Proposition 4 step 2: at 2 the representative is 1 */
        lb_swap(w, res);
        adf_lball_clear(res);
        return ADF_OK;
    }
    if (!pow_ok(p, n))
        return ADF_LIMIT;
    {
        adf_lball_t res;
        fmpz_t c;
        adf_lball_init(res);
        fmpz_init(c);
        teich_mod(c, p, rr, (ulong) n);
        set_unit_ball(res, p, c, n);
        lb_swap(w, res);
        adf_lball_clear(res);
        fmpz_clear(c);
    }
    return ADF_OK;
}

/* The residue of the unit part of x (non-zero centre) modulo p (odd p) or modulo 4 (p = 2), in [1, p) or {1, 3}.
   Ball: the integer u. Exact: a/b, a b^(-1) modulo p; modulo 4 the inverse of the odd b is b. */
static ulong
unit_residue(const adf_lball_struct * x)
{
    ulong p = x->p;
    if (p == 2)
    {
        ulong a = fmpz_fdiv_ui(fmpq_numref(x->u), 4), b = fmpz_fdiv_ui(fmpq_denref(x->u), 4);
        return (a * b) % 4;
    }
    {
        ulong a = fmpz_fdiv_ui(fmpq_numref(x->u), p), b = fmpz_fdiv_ui(fmpq_denref(x->u), p);
        return n_mulmod2(a, n_invmod(b, p), p);
    }
}

/* L10 (docs/api-1f.md). */
int
adf_lball_decompose_teich(slong * m, adf_lball_t w, ulong * index, adf_lball_t u, const adf_lball_t x, slong prec)
{
    adf_lball_t wres, ures;
    fmpz_t om, P, Pk, c, inv;
    ulong p, r, K;
    slong n = prec < 1 ? 1 : prec, k, mv;
    int exact, st = ADF_OK, rational;
    ADF_INV_LBALL(x);
    if (fmpq_is_zero(x->u))
        return x->exact ? ADF_DOMAIN : ADF_NOT_DETERMINED;
    p = x->p;
    exact = x->exact;
    /* the limits of N-D4 first: N - v is formed only for |v|, |N| <= EXP_MAX (review n-review1, C1) */
    if (!in_bounds(x) || !exp_ok(x->N - x->v))
        return ADF_LIMIT;
    if (!exact && p == 2 && x->N - x->v == 1)
        return ADF_NOT_DETERMINED;
    mv = x->v;
    k = exact ? 0 : x->N - x->v;
    r = unit_residue(x);
    rational = (p == 2 || r == 1 || r == p - 1);
    K = (ulong) (exact || n > k ? n : k);
    if (!rational && !pow_ok(p, (slong) K))
        return ADF_LIMIT;
    adf_lball_init(wres);
    adf_lball_init(ures);
    fmpz_init(om);
    fmpz_init(P);
    fmpz_init(Pk);
    fmpz_init(c);
    fmpz_init(inv);
    if (rational)
    {
        /* w = s = +-1: at p = 2, s = -1 iff the residue is 3; at odd p, s = -1 iff the residue is p - 1 */
        int neg = (p == 2) ? (r == 3) : (r == p - 1);
        set_exact_pm1(wres, p, neg);
        if (exact)
        {
            fmpq_set(ures->u, x->u);
            if (neg)
                fmpq_neg(ures->u, ures->u);
            ures->p = p;
            ures->v = 0;
            ures->N = 0;
            ures->exact = 1;
        }
        else if (!neg)
        {
            fmpz_set(c, fmpq_numref(x->u));
            set_unit_ball(ures, p, c, k);
        }
        else if (!pow_ok(p, k))
            st = ADF_LIMIT;                                 /* the centre p^k - u needs p^k */
        else
        {
            pow_p(Pk, p, (ulong) k);
            fmpz_sub(c, Pk, fmpq_numref(x->u));
            set_unit_ball(ures, p, c, k);
        }
    }
    else
    {
        slong nu = exact ? n : k;                           /* the precision of the principal unit */
        teich_mod(om, p, r, K);
        pow_p(P, p, K);
        {
            fmpz_t pn;
            fmpz_init(pn);
            pow_p(pn, p, (ulong) n);
            fmpz_mod(c, om, pn);
            fmpz_clear(pn);
        }
        set_unit_ball(wres, p, c, n);
        pow_p(Pk, p, (ulong) nu);
        fmpz_mod(om, om, Pk);                               /* omega modulo p^nu */
        if (exact)
        {
            /* u = (a/b) / omega modulo p^n = a (b omega)^(-1) */
            fmpz_mul(c, fmpq_denref(x->u), om);
            fmpz_mod(c, c, Pk);
            {
                int ok = fmpz_invmod(inv, c, Pk);
                (void) ok;
            }
            fmpz_mul(c, fmpq_numref(x->u), inv);
            fmpz_mod(c, c, Pk);
        }
        else
        {
            int ok = fmpz_invmod(inv, om, Pk);
            (void) ok;
            fmpz_mul(c, fmpq_numref(x->u), inv);
            fmpz_mod(c, c, Pk);
        }
        set_unit_ball(ures, p, c, nu);
    }
    if (st == ADF_OK)
    {
        lb_swap(w, wres);
        lb_swap(u, ures);
        *m = mv;
        *index = r;
    }
    adf_lball_clear(wres);
    adf_lball_clear(ures);
    fmpz_clear(om);
    fmpz_clear(P);
    fmpz_clear(Pk);
    fmpz_clear(c);
    fmpz_clear(inv);
    return st;
}

/* The unit part of x modulo p^k (k >= 1, p^k formed by the caller into Pk): a ball u mod p^k (u < p^(N - v) is not
   assumed below p^k), an exact a/b as a b^(-1). */
static void
unit_mod_pk(fmpz_t out, const adf_lball_struct * x, const fmpz_t Pk)
{
    if (x->exact)
    {
        int ok = fmpz_invmod(out, fmpq_denref(x->u), Pk);
        (void) ok;                                          /* den is prime to p */
        fmpz_mul(out, out, fmpq_numref(x->u));
        fmpz_mod(out, out, Pk);
    }
    else
        fmpz_mod(out, fmpq_numref(x->u), Pk);
}

/* L11 (docs/api-1f.md), Proposition 19 (docs/proofs/functions.md line 656). */
int
adf_lball_frac(adf_rat_t r, const adf_lball_t x)
{
    fmpz_t Pk, a;
    slong k;
    ADF_INV_LBALL(x);
    if (!in_bounds(x))
        return ADF_LIMIT;
    if (!x->exact && x->N < 0)
        return ADF_NOT_DETERMINED;              /* Proposition 19 step 3: constant exactly when N >= 0 */
    if (fmpq_is_zero(x->u) || x->v >= 0)
    {
        fmpq_zero(r->q);
        return ADF_OK;
    }
    k = -x->v;
    if (!pow_ok(x->p, k))
        return ADF_LIMIT;                       /* the denominator of the result is p^k */
    fmpz_init(Pk);
    fmpz_init(a);
    pow_p(Pk, x->p, (ulong) k);
    unit_mod_pk(a, x, Pk);                      /* p^k x = the unit part u, so a = u modulo p^k */
    fmpz_swap(fmpq_numref(r->q), a);
    fmpz_swap(fmpq_denref(r->q), Pk);           /* a is prime to p (u is): the fraction is in lowest terms */
    fmpz_clear(Pk);
    fmpz_clear(a);
    return ADF_OK;
}

/* L11: the unit part of x modulo p^k. */
int
adf_lball_unit_mod(fmpz_t out, const adf_lball_t x, slong k)
{
    fmpz_t Pk, a;
    ADF_INV_LBALL(x);
    if (fmpq_is_zero(x->u))
        return x->exact ? ADF_DOMAIN : ADF_NOT_DETERMINED;
    if (k < 0)
        return ADF_DOMAIN;
    if (!pow_ok(x->p, k))
        return ADF_LIMIT;
    if (!x->exact && (ulong) k > (ulong) x->N - (ulong) x->v)
        return ADF_NOT_DETERMINED;
    fmpz_init(Pk);
    fmpz_init(a);
    if (k == 0)
        fmpz_zero(a);
    else
    {
        pow_p(Pk, x->p, (ulong) k);
        unit_mod_pk(a, x, Pk);
    }
    fmpz_swap(out, a);
    fmpz_clear(Pk);
    fmpz_clear(a);
    return ADF_OK;
}
