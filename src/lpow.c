/* src/lpow.c: rational powers and powers of principal units on a local ball at one prime (lane f-slice9, WP 1F.6;
   include/adelefeld/lpow.h; SPEC 9.3.4 items 2 and 3).

   Sources: docs/proofs/functions.md Lemma 9 (line 265: exp and log are inverse isometries on p^c Z_p and
   1 + p^c Z_p), Proposition 11 (line 336: Log, and log(u0 + p^A Z_p) = log(u0) + p^A Z_p for A >= c), Proposition 13
   (line 410), Proposition 15 (line 463), Proposition 17 (line 577: "reduce the fraction first and state the
   branch"; u^s = exp(s log u); w^(s mod 2) exp(s log u) at 2), Proposition 18 (line 616: the exact image
   exp(s0 ell) + p^R Z_p, R = min(A + beta, B + alpha, A + B)); docs/api-1f5.md R2 (the image of a root branch),
   docs/api-1f.md L12 (the image of an integer power); and the statements P1 to P8 of docs/api-1f6.md, which prove the
   composition, the 2-adic hull, the precisions and the limits used below. FLINT: n_gcd
   (refs/src/flint-3.0.1/ulong_extras.rst:404).

   Every result is built in a temporary and copied to y at the end, so y may alias any input and a status leaves y
   untouched. No power of p is formed here: lroot.h, lfunc.h and lball.h form them and test their limits. */

#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include "invariants.h"

/* INF marks an absent term (an exact input) and saturates every exponent sum; it is far above ADF_LBALL_EXP_MAX
   (2^60), and every sum below has operands of at most 2^62 in absolute value, so no slong overflows. */
#define INF ((slong) 1 << 62)

static void check_input(const adf_lball_t x, const char *fn, const char *arg)
{
#ifdef ADF_CHECK_INVARIANTS
    if (!adf_lball_is_canonical(x)) adf_inv_fail(fn, arg, "adf_lball");
#else
    (void) x; (void) fn; (void) arg;
#endif
}

static int exponent_ok(slong a) { return a >= -ADF_LBALL_EXP_MAX && a <= ADF_LBALL_EXP_MAX; }
static int in_bounds(const adf_lball_t x) { return exponent_ok(x->v) && exponent_ok(x->N); }

/* a clamped to [-INF, INF] */
static slong clamp(slong a) { return a > INF ? INF : (a < -INF ? -INF : a); }
/* a + b with INF absorbing; a, b in [-INF, INF] */
static slong add_inf(slong a, slong b) { return (a >= INF || b >= INF) ? INF : clamp(a + b); }
static slong min2(slong a, slong b) { return a < b ? a : b; }

/* e j clamped to [-INF, INF]: |j| <= 2^60, e any slong. */
static slong mul_clamp(slong e, slong j)
{
    ulong ae = e < 0 ? -(ulong) e : (ulong) e, aj = j < 0 ? -(ulong) j : (ulong) j;
    int neg = (e < 0) != (j < 0);
    if (ae == 0 || aj == 0) return 0;
    if (ae > (ulong) INF / aj) return neg ? -INF : INF;
    return neg ? -(slong) (ae * aj) : (slong) (ae * aj);
}

static slong vp_ulong(ulong n, ulong p)
{
    slong s = 0;
    while (n % p == 0) { n /= p; s++; }
    return s;
}

/* res = the ball p^K Z_p around 0 (canonical for every K). */
static void zero_ball(adf_lball_t res, ulong p, slong K)
{
    res->p = p; fmpq_zero(res->u); res->v = 0; res->N = K; res->exact = 0;
}

/* ------------------------------------------------------------------------------------------------ rational powers */

/* The degree n' >= 2 (P2, P3 of docs/api-1f6.md): the root of x on the branch seed (lroot.h, R2: the image of a guarded
   ball is b + p^E Z_p), then its e'-th power (L12). The image of the composite is b^e' + p^E' Z_p,
   E' = e' j + (M - m) - v_p(n') + v_p(e') (P2), the result the ball at K = min(N, E') (N for an exact x). The root is
   asked at the relative precision rel_r = max(1, K - e' j - v_p(e')), which P3 proves enough: the power then has
   relative precision >= K - e' j. */
static int powrat_branch(adf_lball_t y, const adf_lball_t x, slong e1, ulong n1, ulong seed, slong N)
{
    adf_lball_t r, P, res;
    fmpz_t c;
    ulong p = x->p, am;
    slong j = 0, ej = 0, K = clamp(N), Nr = 0, s, ve, rel;
    int st, divisible;

    if (fmpq_is_zero(x->u))
    {
        /* the exact 0 (root 0 for the seed 0, then 0^e': 0, or NOT_UNIT for e' < 0) or a ball around 0 (NOT_DETERMINED
           from lroot.h): the composition of the two functions decides (P1) */
        adf_lball_init(r);
        st = adf_lball_root_seed(r, x, n1, seed, 0);
        if (st == ADF_OK) st = adf_lball_pow_si(y, r, e1);
        adf_lball_clear(r);
        return st;
    }
    am = x->v < 0 ? -(ulong) x->v : (ulong) x->v;
    divisible = am % n1 == 0;
    s = vp_ulong(n1, p);
    ve = vp_ulong(e1 < 0 ? -(ulong) e1 : (ulong) e1, p);
    if (divisible)
    {
        j = x->v / (slong) n1;                      /* n' <= |m| <= 2^60 when m != 0 (R6 step 1) */
        ej = mul_clamp(e1, j);
        if (!x->exact)                              /* E' = e' j + (M - m) - s + v_p(e'), P2 */
            K = min2(K, add_inf(ej, clamp(x->N - x->v - s + ve)));
        /* the root precision of P3; for K <= e' j the root is asked only for its status and rationality */
        rel = add_inf(K, -ej) - ve;
        Nr = clamp(K <= ej ? j + 1 : j + (rel < 1 ? 1 : rel));
    }
    adf_lball_init(r); adf_lball_init(P); adf_lball_init(res); fmpz_init(c);
    st = adf_lball_root_seed(r, x, n1, seed, Nr);
    if (st == ADF_OK && !divisible) st = ADF_NOT_DETERMINED;    /* not reached: lroot.h refuses n' not dividing m */
    if (st != ADF_OK) goto done;
    if (r->exact)
    {
        /* a rational branch of an exact x: the exact power (P1: b^e' is rational exactly when b is) */
        st = adf_lball_pow_si(res, r, e1);
        goto done;
    }
    if (!exponent_ok(K)) { st = ADF_LIMIT; goto done; }
    if (K <= ej)
    {
        zero_ball(res, p, K);                       /* every image point has valuation e' j >= K (P2) */
        goto done;
    }
    if (!exponent_ok(ej)) { st = ADF_LIMIT; goto done; }    /* the valuation of the result */
    st = adf_lball_pow_si(P, r, e1);
    if (st != ADF_OK) goto done;
    if (P->N == K) { adf_lball_swap(res, P); goto done; }  /* already the ball at K: no reduction, no power */
    if (fmpq_is_one(P->u)) fmpz_one(c);                      /* the unit 1 modulo every p^(K - e' j) */
    else st = adf_lball_unit_mod(c, P, K - ej);              /* P3: K - e' j <= the relative precision of P */
    if (st != ADF_OK) goto done;
    res->p = p; res->v = ej; res->N = K; res->exact = 0;
    fmpq_set_fmpz(res->u, c);
done:
    if (st == ADF_OK) adf_lball_set(y, res);
    adf_lball_clear(r); adf_lball_clear(P); adf_lball_clear(res); fmpz_clear(c);
    return st;
}

/* SPEC 9.3.4 item 2; functions.md:577 (Proposition 17: "reduce the fraction first and state the branch"); P1. */
int adf_lball_powrat(adf_lball_t y, const adf_lball_t x, slong e, ulong n, ulong seed, slong N)
{
    ulong ae, g, n1;
    slong e1;
    check_input(x, __func__, "x");
    if (!in_bounds(x)) return ADF_LIMIT;
    if (n == 0) return ADF_DOMAIN;
    ae = e < 0 ? -(ulong) e : (ulong) e;
    g = n_gcd(ae, n);                               /* gcd(0, n) = n: e = 0 gives 0/1 */
    n1 = n / g;
    e1 = g == 1 ? e : (e < 0 ? -(slong) (ae / g) : (slong) (ae / g));   /* |e| / g < 2^63 for g >= 2 */
    if (n1 == 1) return adf_lball_pow_si(y, x, e1);  /* degree 1 is the identity (lroot.h); then L12 */
    return powrat_branch(y, x, e1, n1, seed, N);
}

/* ------------------------------------------------------------------------------------- powers of principal units */

/* 1 inside the set D, 0 meeting it and its complement, -1 disjoint from it (F1 of api-1f4.md; L8 of api-1f.md). */
static int where_in(const adf_lball_t x, const adf_lball_t D)
{
    if (adf_lball_contains(x, D)) return 1;
    return adf_lball_overlaps(x, D) ? 0 : -1;
}

/* res = the exact rational sign (1 or -1), or the ball sign + p^K Z_p (lball.h add: exact + ball). */
static int signed_one(adf_lball_t res, ulong p, slong sign, slong K, int exact)
{
    adf_lball_t one, zb;
    int st;
    adf_lball_init(one); adf_lball_init(zb);
    one->p = p; fmpq_set_si(one->u, sign, 1); one->v = 0; one->N = 0; one->exact = 1;
    if (exact) { adf_lball_swap(res, one); st = ADF_OK; }
    else if (!exponent_ok(K)) st = ADF_LIMIT;
    else { zero_ball(zb, p, K); st = adf_lball_add(res, one, zb); }
    adf_lball_clear(one); adf_lball_clear(zb);
    return st;
}

/* Proposition 18 (functions.md:616) and P5, P6 of api-1f6.md, after the domain and the exact 1 are settled:
   u0, A (INF exact), s0, B (INF exact), beta = v(s0) (INF for s0 = 0), w0 the sign of u0 (1 at odd p), sign =
   w0^(s0 mod 2) (+1 or -1). alpha = v(log(w0 u0)) = v(w0 u0 - 1), because log is an isometry on 1 + p^c Z_p
   (Proposition 11 line 336, Lemma 9 item 5; P6): one exact subtraction, no logarithm and no power. The centre
   exp(s0 ell) mod p^K needs ell to p^max(K - beta, c) (P6). */
static int principal(adf_lball_t res, const adf_lball_t u, const adf_lball_t s, slong A, slong B, slong beta,
                     slong w0, slong sign, slong Nc)
{
    adf_lball_t uc, sc, l, t, z, zb;
    ulong p = u->p;
    slong c = p == 2 ? 2 : 1, K, R = INF;
    int st = ADF_OK;
    adf_lball_init(uc); adf_lball_init(sc); adf_lball_init(l); adf_lball_init(t); adf_lball_init(z);
    adf_lball_init(zb);
    /* the exact centres: u0 = the canonical centre (a unit, v = 0), s0 = p^v u */
    uc->p = p; fmpq_set(uc->u, u->u); uc->v = u->v; uc->N = 0; uc->exact = 1;
    sc->p = p; fmpq_set(sc->u, s->u); sc->v = fmpq_is_zero(s->u) ? 0 : s->v; sc->N = 0; sc->exact = 1;
    if (A >= INF && B >= INF)
        K = Nc;                                     /* both exact: the ball at N (as exp of an exact input) */
    else
    {
        if (A < INF) R = min2(R, add_inf(A, beta));
        if (A < INF && B < INF) R = min2(R, add_inf(A, B));
        if (B < INF)
        {
            /* alpha = v(w0 u0 - 1), INF for w0 u0 = 1 (P6); both operands exact units, so the difference is exact */
            adf_lball_t one;
            adf_lball_init(one);
            one->p = p; fmpq_set_si(one->u, w0, 1); one->v = 0; one->N = 0; one->exact = 1;
            st = adf_lball_sub(l, uc, one);
            adf_lball_clear(one);
            if (st != ADF_OK) goto done;
            R = min2(R, add_inf(B, fmpq_is_zero(l->u) ? INF : l->v));
        }
        K = min2(Nc, R);
    }
    if (!exponent_ok(K)) { st = ADF_LIMIT; goto done; }
    if (K <= c || fmpq_is_zero(s->u))
    {
        /* exp(s0 ell) = 1 modulo p^K: s0 ell lies in p^c Z_p (Lemma 9 item 5), or s0 = 0 */
        st = signed_one(res, p, sign, K, 0);
        goto done;
    }
    st = adf_lball_Log(l, uc, K - beta > c ? K - beta : c);   /* ell + p^P Z_p, P = max(K - beta, c) */
    if (st == ADF_OK) st = adf_lball_mul(t, l, sc);          /* s0 ell + p^(P + beta) Z_p, inside p^c Z_p */
    if (st == ADF_OK) st = adf_lball_exp(z, t, K);           /* exponent min(K, P + beta) = K, or exact 1 */
    if (st == ADF_OK && sign < 0) st = adf_lball_neg(z, z);
    if (st == ADF_OK)
    {
        zero_ball(zb, p, K);
        st = adf_lball_add(res, z, zb);                      /* an exact 1 becomes the ball at K */
    }
done:
    adf_lball_clear(uc); adf_lball_clear(sc); adf_lball_clear(l); adf_lball_clear(t); adf_lball_clear(z);
    adf_lball_clear(zb);
    return st;
}

/* SPEC 9.3.4 item 3; functions.md:577 (Proposition 17), :616 (Proposition 18); P4 to P6 of api-1f6.md. */
int adf_lball_powunit(adf_lball_t y, const adf_lball_t u, const adf_lball_t s, slong N)
{
    adf_lball_t D1, Zp, res;
    fmpz_t r;
    ulong p;
    slong A, B, beta, Nc = clamp(N), w0 = 1;
    int du, ds, st = ADF_OK, par, par_known, sign_known;
    check_input(u, __func__, "u");
    check_input(s, __func__, "s");
    if (u->p != s->p) return ADF_DOMAIN;
    if (!in_bounds(u) || !in_bounds(s)) return ADF_LIMIT;
    p = u->p;
    adf_lball_init(D1); adf_lball_init(Zp); adf_lball_init(res); fmpz_init(r);
    /* the domains as canonical balls: 1 + p Z_p and Z_p = 0 + p^0 Z_p (P4) */
    D1->p = p; fmpq_one(D1->u); D1->v = 0; D1->N = 1; D1->exact = 0;
    zero_ball(Zp, p, 0);
    du = where_in(u, D1);
    ds = where_in(s, Zp);
    if (du < 0 || ds < 0) { st = ADF_DOMAIN; goto done; }
    if (du == 0 || ds == 0) { st = ADF_NOT_DETERMINED; goto done; }
    A = u->exact ? INF : u->N;                      /* u inside 1 + p Z_p: v = 0 and N >= 1 */
    B = s->exact ? INF : s->N;                      /* s inside Z_p: N >= 0 */
    beta = fmpq_is_zero(s->u) ? INF : s->v;         /* the valuation of the CENTRE of s (Proposition 18) */
    /* the exact 1: an exact factor of s log u is 0 (Proposition 18, last sentence) */
    if ((s->exact && fmpq_is_zero(s->u)) || (u->exact && fmpq_is_one(u->u)))
    {
        st = signed_one(res, p, 1, 0, 1);
        goto done;
    }
    par_known = s->exact || B >= 1;
    par = !fmpq_is_zero(s->u) && beta == 0;         /* s0 odd; read only where par_known */
    sign_known = p != 2 || u->exact || A >= 2;
    if (p == 2)
    {
        if (sign_known)
        {
            st = adf_lball_unit_mod(r, u, 2);
            if (st != ADF_OK) goto done;
            w0 = fmpz_equal_si(r, 1) ? 1 : -1;      /* Proposition 4 step 2: w = u modulo 4 */
        }
        if (u->exact && fmpz_equal_si(fmpq_numref(u->u), -1) && fmpz_is_one(fmpq_denref(u->u)))
        {
            /* u = -1 exactly: u' = 1, log u' = 0 exactly; the value is (-1)^(s mod 2) (P5) */
            if (par_known) st = signed_one(res, p, par ? -1 : 1, 0, 1);
            else st = signed_one(res, p, 1, min2(Nc, 1), 0);      /* {1, -1}: the hull 1 + 2 Z_2 */
            goto done;
        }
        if (!sign_known)
        {
            /* u = 1 + 2 Z_2. s even: u' over 1 + 4 Z_2, the image 1 + 2^R Z_2, R = 2 + min(beta, B); else the
               hull 1 + 2 Z_2 (P5) */
            if (par_known && !par) st = signed_one(res, p, 1, min2(Nc, add_inf(2, min2(beta, B))), 0);
            else st = signed_one(res, p, 1, min2(Nc, 1), 0);
            goto done;
        }
        if (w0 == -1 && !par_known)
        {
            st = signed_one(res, p, 1, min2(Nc, 1), 0);           /* the hull 1 + 2 Z_2 (P5) */
            goto done;
        }
    }
    st = principal(res, u, s, A, B, beta, w0, (w0 == -1 && par) ? -1 : 1, Nc);
done:
    if (st == ADF_OK) adf_lball_set(y, res);
    adf_lball_clear(D1); adf_lball_clear(Zp); adf_lball_clear(res); fmpz_clear(r);
    return st;
}
