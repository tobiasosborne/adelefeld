/* lfunc.c: exp, log and the Iwasawa Log on a local ball (milestone 1F.4, first slice; include/adelefeld/lfunc.h).

   Sources. docs/proofs/functions.md: Definition 1 (line 11, the series at lines 22 and 27), Lemma 5 (line 117:
   v_p(k!) = sum floor(k/p^j), line 121), Proposition 6 (line 140: the domains), Proposition 7 (line 165: the count
   K = max(1, ceildiv((p-1)n - 1, (p-1)v - 1)) of exp, lines 170-172), Proposition 7b (line 198: J = the least k >= 1
   with k v - e(k) >= n, T = J - 1, lines 202-203; step 2, line 219: J <= max(1, ceildiv(2n, 2v - 1))),
   Proposition 8 (line 227: W = max(v, n + D), line 231; D = v_p(L!) for exp, floor(log_p L) for log, lines 233-235;
   the division of a term by p^e and by the inverse of its unit part, lines 237-240), Lemma 9 (line 265; item 5,
   lines 291-294: v(exp(x) - 1) = v(x) and v(log(1 + z)) = v(z) for arguments of valuation >= c), Proposition 10
   (line 299: exp maps a + p^N Z_p onto exp(a) + p^N Z_p), Proposition 11 (line 336: the images of log and Log, lines
   345-352; Log = log on 1 + p Z_p, step 3, line 363), Proposition 12 (step 4, lines 402-405: Log(x) is in p^c Z_p).
   docs/api-1f.md L0 (the canonical centre) and L8 (the predicates, used for the domain test). docs/api-1f4.md F1 to
   F7: the statements that this file adds (the domain test, the exact values, Log without the root of unity, exp by
   one common denominator, the lower bound of the valuation in the sum of log, the precision of a ball result, the
   limits). FLINT, refs/src/flint-3.0.1/fmpz.rst: fmpz_divexact (852-859), fmpz_mod (880-883: the remainder is
   non-negative), fmpz_pow_ui and fmpz_ui_pow_ui (913-916), fmpz_powm_ui (923-930), fmpz_remove (1142-1150),
   fmpz_invmod (1154-1160).

   Inputs are reduced modulo p^W. Small log sums use decreasing term moduli (F8). Larger log sums
   factor into short principal units and use exact binary splitting (F9). Results are reduced modulo p^K.
   A function computes into a temporary and moves it into the output only on ADF_OK (conventions 4.3),
   so y may be x and a status leaves y untouched. */

#include <limits.h>
#include <flint/ulong_extras.h>

#include <adelefeld.h>
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

enum { F_EXP, F_LOG };

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

/* 1 if p^k, k >= 0, has at most BITS_MAX bits (p^k has at most k bits(p) bits). */
static int
pow_ok(ulong p, slong k)
{
    return k >= 0 && k <= BITS_MAX / (slong) FLINT_BIT_COUNT(p);
}

/* e(k): the largest e with p^e <= k, for k >= 1 (Proposition 7b). No overflow: t <= k / p before t *= p. */
static slong
floor_log(ulong k, ulong p)
{
    slong e = 0;
    ulong t = 1;
    while (t <= k / p)
    {
        t *= p;
        e++;
    }
    return e;
}

/* v_p(k!) = sum_{j >= 1} floor(k / p^j) (Lemma 5, line 121). */
static slong
val_fac(ulong k, ulong p)
{
    slong d = 0;
    while (k >= p)
    {
        k /= p;
        d += (slong) k;
    }
    return d;
}

/* Proposition 7b: T = J - 1, J the least k >= 1 with k v - e(k) >= n, for n >= 1 and v >= 1. The quantity
   k v - e(k) is nondecreasing in k (step 1, line 215), and J <= hi = max(1, ceildiv(2n, 2v - 1)) (step 2), so J is
   found by bisection on [1, hi]. n <= 2^26 here (pow_ok(p, n) holds), so hi and k v fit in a word. */
static slong
count_log(ulong p, slong n, slong v)
{
    slong lo = 1, hi;
    if (v >= n)
        return 0;                                  /* k = 1 already works: J = 1 */
    hi = (2 * n + (2 * v - 1) - 1) / (2 * v - 1);
    if (hi < 1)
        hi = 1;
    while (lo < hi)                               /* invariant: J in [lo, hi] */
    {
        slong mid = lo + (hi - lo) / 2;
        if (mid * v - floor_log((ulong) mid, p) >= n)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo - 1;
}

/* Proposition 7: the number K of terms of exp (degrees 0 .. K - 1) for v(x) >= w >= c and precision n >= 1:
   K = max(1, ceildiv((p-1) n - 1, (p-1) w - 1)). Formed in fmpz ((p - 1) n overflows a word for large p). */
static slong
count_exp(ulong p, slong n, slong w)
{
    fmpz_t a, b;
    slong K;
    fmpz_init_set_ui(a, p - 1);
    fmpz_init_set_ui(b, p - 1);
    fmpz_mul_si(a, a, n);
    fmpz_sub_ui(a, a, 1);
    fmpz_mul_si(b, b, w);
    fmpz_sub_ui(b, b, 1);                          /* (p-1) w - 1 >= 1 because w >= c */
    fmpz_cdiv_q(a, a, b);
    K = fmpz_cmp_si(a, 1) < 0 ? 1 : fmpz_get_si(a); /* at most 2n (api-1f4.md F7), and n <= 2^25: fits */
    fmpz_clear(a);
    fmpz_clear(b);
    return K;
}

/* The residue modulo P of the p-integral rational q (its denominator is prime to p, so invertible modulo P). */
static void
rat_mod(fmpz_t r, const fmpq_t q, const fmpz_t P)
{
    fmpz_t inv;
    fmpz_init(inv);
    fmpz_invmod(inv, fmpq_denref(q), P);
    fmpz_mul(r, fmpq_numref(q), inv);
    fmpz_mod(r, r, P);
    fmpz_clear(inv);
}

/* ------------------------------------------------------------------------------------------ the result forms */

static void
set_exact_small(adf_lball_struct * y, ulong p, slong value)
{
    y->p = p;
    fmpq_set_si(y->u, value, 1);
    y->v = 0;
    y->N = 0;
    y->exact = 1;
}

/* y = the ball r + p^K Z_p for an integer 0 <= r < p^K (K >= 1), or the ball p^K Z_p around 0 (r = 0 or K <= 0).
   The canonical form (conventions 5.8; api-1f.md L0): r = p^w u with p not dividing u, w < K, 0 < u < p^(K - w). */
static void
set_ball_residue(adf_lball_struct * y, ulong p, const fmpz_t r, slong K)
{
    y->p = p;
    y->N = K;
    y->exact = 0;
    fmpz_one(fmpq_denref(y->u));
    if (K <= 0 || fmpz_is_zero(r))
    {
        fmpz_zero(fmpq_numref(y->u));
        y->v = 0;
    }
    else
    {
        fmpz_t P;
        fmpz_init_set_ui(P, p);
        y->v = fmpz_remove(fmpq_numref(y->u), r, P);
        fmpz_clear(P);
    }
}

static void
set_ball_one(adf_lball_struct * y, ulong p, slong K)
{
    fmpz_t one;
    fmpz_init_set_ui(one, 1);
    set_ball_residue(y, p, one, K);
    fmpz_clear(one);
}

/* The output gets the temporary on OK; the temporary is cleared. No check of y: an output is not read. */
static int
finish(adf_lball_struct * y, adf_lball_struct * res, int status)
{
    if (status == ADF_OK)
    {
        ulong p = y->p;
        slong v = y->v, N = y->N;
        int e = y->exact;
        y->p = res->p; y->v = res->v; y->N = res->N; y->exact = res->exact;
        res->p = p; res->v = v; res->N = N; res->exact = e;
        fmpq_swap(y->u, res->u);
    }
    adf_lball_clear(res);
    return status;
}

/* ------------------------------------------------------------------------------------------- the domain (F1) */

/* The status of the domain test of exp (domain p^c Z_p) or log (domain 1 + p Z_p), by the predicates of lball.h
   (api-1f.md L8): x inside the domain ball: OK; x does not meet it: DOMAIN; else NOT_DETERMINED (conventions 3.1). */
static int
domain_status(const adf_lball_struct * x, int which)
{
    adf_lball_t D;
    int st;
    adf_lball_init(D);
    D->p = x->p;
    D->exact = 0;
    D->v = 0;
    if (which == F_EXP)
    {
        D->N = x->p == 2 ? 2 : 1;              /* p^c Z_p: the ball around 0 of exponent c */
    }
    else
    {
        D->N = 1;                              /* 1 + p Z_p: centre 1, exponent 1 */
        fmpq_one(D->u);
    }
    if (adf_lball_contains(x, D))
        st = ADF_OK;
    else if (adf_lball_overlaps(x, D))
        st = ADF_NOT_DETERMINED;
    else
        st = ADF_DOMAIN;
    adf_lball_clear(D);
    return st;
}

/* --------------------------------------------------------------------------------------------------------- exp */

/* F4: res = exp(p^w t) + p^K Z_p for the p-integral rational t (a unit, or an integer prime to p) with w >= c and
   c <= w < K, pow_ok(p, K). Returns OK or LIMIT (the working power p^W, W = K + D, is beyond the bound).
   Proposition 7 gives the count Kt of terms (degrees 0 .. L, L = Kt - 1); Proposition 8's D = v_p(L!); the sum
   sum_{i=0}^{L} x^i / i! = A_1 / L! with A_{L+1} = 1, F_{L+1} = 1, F_k = k F_{k+1}, A_k = F_k + x A_{k+1} (F4). */
static int
exp_centre(adf_lball_struct * res, ulong p, const fmpq_t t, slong w, slong K)
{
    slong L = count_exp(p, K, w) - 1, D, W, k;
    fmpz_t P, PD, PK, x, A, F, tmp;
    D = val_fac((ulong) L, p);
    W = K + D;
    if (!pow_ok(p, W))
        return ADF_LIMIT;
    fmpz_init(P); fmpz_init(PD); fmpz_init(PK); fmpz_init(x); fmpz_init(A); fmpz_init(F); fmpz_init(tmp);
    fmpz_ui_pow_ui(P, p, (ulong) W);
    fmpz_ui_pow_ui(PD, p, (ulong) D);
    fmpz_ui_pow_ui(PK, p, (ulong) K);
    /* x = p^w t modulo p^W; w < K <= W */
    rat_mod(x, t, P);
    fmpz_ui_pow_ui(tmp, p, (ulong) w);
    fmpz_mul(x, x, tmp);
    fmpz_mod(x, x, P);
    fmpz_one(A);
    fmpz_one(F);
    for (k = L; k >= 1; k--)
    {
        fmpz_mul_ui(F, F, (ulong) k);
        fmpz_mod(F, F, P);
        fmpz_mul(A, A, x);
        fmpz_add(A, A, F);
        fmpz_mod(A, A, P);
    }
    /* A = A_1, F = L! modulo p^W; both divisible by p^D (F4 (iii), (iv)) */
    fmpz_divexact(A, A, PD);
    fmpz_divexact(F, F, PD);
    fmpz_invmod(tmp, F, PK);                    /* F / p^D = the unit part of L! modulo p^K: invertible */
    fmpz_mul(A, A, tmp);
    fmpz_mod(A, A, PK);
    set_ball_residue(res, p, A, K);
    fmpz_clear(P); fmpz_clear(PD); fmpz_clear(PK); fmpz_clear(x); fmpz_clear(A); fmpz_clear(F); fmpz_clear(tmp);
    return ADF_OK;
}

int
adf_lball_exp(adf_lball_t y, const adf_lball_t x, slong N)
{
    adf_lball_t res;
    int st;
    slong K;
    ulong p = x->p;
    ADF_INV_LBALL(x);
    if (!in_bounds(x))
        return ADF_LIMIT;
    st = domain_status(x, F_EXP);
    if (st != ADF_OK)
        return st;
    adf_lball_init(res);
    if (x->exact && fmpq_is_zero(x->u))
    {
        set_exact_small(res, p, 1);            /* exp(0) = 1 (Definition 1) */
        return finish(y, res, ADF_OK);
    }
    /* F6: K = N for an exact x, min(N, M) for a ball of exponent M (Proposition 10) */
    K = (x->exact || N < x->N) ? N : x->N;
    if (!exp_ok(K))
        return finish(y, res, ADF_LIMIT);
    if (fmpq_is_zero(x->u) || K <= x->v)
    {
        /* the centre is 0, or v(exp(a) - 1) = v(a) >= K (Lemma 9, item 5): the centre of the result is 1 */
        set_ball_one(res, p, K);
        return finish(y, res, ADF_OK);
    }
    if (!pow_ok(p, K))
        return finish(y, res, ADF_LIMIT);
    st = exp_centre(res, p, x->u, x->v, K);
    return finish(y, res, st);
}

/* ------------------------------------------------------------------------------------------------ log and Log */

/* Retain the original loop when the entire working modulus is a tagged word.
   The small-N measurements show that setting up the shrinking powers costs more there. */
/* F5 and Proposition 8: S = sum_{k=1}^{T} (-1)^(k+1) z^k / k modulo p^K, from z_r = z modulo p^W, W >= K + e(T),
   v(z) >= vz >= 1, vz < K. Each term: z_r^k modulo p^W is divisible by p^e, e = v_p(k) (v(z_r^k) >= k vz > e, and
   W > e); it is divided by p^e and multiplied by the inverse of k / p^e modulo p^W. */
static void
log_sum_word(fmpz_t S, ulong p, const fmpz_t zr, slong T, const fmpz_t P, const fmpz_t PK)
{
    fmpz_t zk, term, inv, kk, pe;
    slong k;
    fmpz_init(zk); fmpz_init(term); fmpz_init(inv); fmpz_init(kk); fmpz_init(pe);
    fmpz_zero(S);
    fmpz_one(zk);
    for (k = 1; k <= T; k++)
    {
        ulong kp = (ulong) k;
        slong e = 0;
        fmpz_mul(zk, zk, zr);
        fmpz_mod(zk, zk, P);
        while (kp % p == 0)
        {
            kp /= p;
            e++;
        }
        fmpz_ui_pow_ui(pe, p, (ulong) e);
        fmpz_divexact(term, zk, pe);
        fmpz_set_ui(kk, kp);
        fmpz_invmod(inv, kk, P);
        fmpz_mul(term, term, inv);
        if (k % 2 == 1)
            fmpz_add(S, S, term);
        else
            fmpz_sub(S, S, term);
        fmpz_mod(S, S, P);
    }
    fmpz_mod(S, S, PK);
    fmpz_clear(zk); fmpz_clear(term); fmpz_clear(inv); fmpz_clear(kk); fmpz_clear(pe);
}

/* F8 (docs/api-1f4.md): z = p^vz b. The degree-k unit power needs only
   H_k = K - k vz + v_p(k) digits. Q uses the monotone upper bound K-k vz+floor_log(k,p),
   so reducing the previous unit power never loses a digit needed by a later term.
   The sum itself retains K digits. The original F7 working-power limit is checked by the caller. */
static void
log_sum(fmpz_t S, ulong p, const fmpz_t zr, slong vz, slong K, slong T, const fmpz_t PK)
{
    fmpz_t b, zk, term, pe, Q, Qt, scale, pv, tmp;
    slong k, previous = 0;
    fmpz_init(b); fmpz_init(zk); fmpz_init(term); fmpz_init(pe);
    fmpz_init(Q); fmpz_init(Qt); fmpz_init(scale); fmpz_init(pv); fmpz_init(tmp);
    fmpz_ui_pow_ui(pv, p, (ulong) vz);
    fmpz_divexact(b, zr, pv);
    fmpz_set(Q, PK); fmpz_one(scale); fmpz_one(zk); fmpz_zero(S);
    for (k = 1; k <= T; k++)
    {
        ulong kp = (ulong) k;
        slong e = 0, bound = floor_log((ulong) k, p), H;
        fmpz_ui_pow_ui(tmp, p, (ulong) (vz - (bound - previous)));
        fmpz_divexact(Q, Q, tmp);
        previous = bound;
        fmpz_mul(zk, zk, b); fmpz_mod(zk, zk, Q);
        fmpz_mul(scale, scale, pv);
        while (kp % p == 0) { kp /= p; e++; }
        H = K - k * vz + e;
        if (H <= 0) continue;
        fmpz_ui_pow_ui(pe, p, (ulong) e);
        fmpz_ui_pow_ui(tmp, p, (ulong) (bound - e));
        fmpz_divexact(Qt, Q, tmp);
        fmpz_mod(term, zk, Qt);
        /* F8: choose j in [0,kp) with term+j Qt divisible by kp.
           Only word inverses are used. refs/src/flint-3.0.1/ulong_extras.rst:477-483;
           fmpz.rst:845-859 (word remainder and exact division). kp <= T <= 2K <= 2^26. */
        if (kp != 1)
        {
            ulong r = fmpz_fdiv_ui(term, kp);
            ulong qi = n_invmod(fmpz_fdiv_ui(Qt, kp), kp);
            ulong j = n_mulmod2(r == 0 ? 0 : kp - r, qi, kp);
            fmpz_addmul_ui(term, Qt, j);
            fmpz_divexact_ui(term, term, kp);
        }
        fmpz_divexact(tmp, scale, pe); fmpz_mul(term, term, tmp);
        if (k % 2 == 1) fmpz_add(S, S, term); else fmpz_sub(S, S, term);
    }
    fmpz_mod(S, S, PK);
    fmpz_clear(b); fmpz_clear(zk); fmpz_clear(term); fmpz_clear(pe);
    fmpz_clear(Q); fmpz_clear(Qt); fmpz_clear(scale); fmpz_clear(pv); fmpz_clear(tmp);
}

/* F9: exact binary splitting of sum_{k=a}^{b-1} q^(k-a)/k.
   No precision is lost inside the tree. FLINT documents balanced chunks for exp at
   refs/src/flint-3.0.1/padic.rst:440-447 and rectangular log at :509-516.
   This tree and the factorisation below are proved independently in docs/api-1f4.md F9. */
static void
log_split(fmpz_t A, fmpz_t B, fmpz_t R, const fmpz_t q, ulong a, ulong b)
{
    if (b - a == 1)
    {
        fmpz_one(A); fmpz_set_ui(B, a); fmpz_set(R, q);
    }
    else
    {
        ulong m = a + (b - a) / 2;
        fmpz_t C, D, U, tmp;
        fmpz_init(C); fmpz_init(D); fmpz_init(U); fmpz_init(tmp);
        log_split(A, B, R, q, a, m);
        log_split(C, D, U, q, m, b);
        fmpz_mul(tmp, R, C); fmpz_mul(tmp, tmp, B);
        fmpz_mul(A, A, D); fmpz_add(A, A, tmp);
        fmpz_mul(B, B, D); fmpz_mul(R, R, U);
        fmpz_clear(C); fmpz_clear(D); fmpz_clear(U); fmpz_clear(tmp);
    }
}

/* Log of a short integral principal unit by an exact rational partial sum (F9). */
static void
log_short(fmpz_t S, ulong p, const fmpz_t z, slong vz, slong K, const fmpz_t PK)
{
    slong T = count_log(p, K, vz), D;
    fmpz_t A, B, R, q, pp, pe;
    fmpz_init(A); fmpz_init(B); fmpz_init(R); fmpz_init(q); fmpz_init_set_ui(pp,p); fmpz_init(pe);
    fmpz_neg(q, z);
    log_split(A, B, R, q, 1, (ulong) T + 1);
    fmpz_mul(A, A, z);
    D = fmpz_remove(B, B, pp);
    fmpz_ui_pow_ui(pe, p, (ulong) D);
    fmpz_divexact(A, A, pe);
    fmpz_invmod(B, B, PK); fmpz_mul(S, A, B); fmpz_mod(S, S, PK);
    fmpz_clear(A); fmpz_clear(B); fmpz_clear(R); fmpz_clear(q); fmpz_clear(pp); fmpz_clear(pe);
}

/* F9: successively remove the low 2*v digits of a unit that is 1 modulo p^v.
   Each remaining factor is 1 modulo p^(2*v). The last factor is taken modulo p^K.
   All factors lie in 1+p^c Z_p, so log adds and congruence modulo p^K is sufficient. */
static void
log_balanced(fmpz_t S, ulong p, const fmpz_t zr, slong vz, slong K, const fmpz_t PK)
{
    fmpz_t u, low, z, Q, inv, term, pp;
    slong v = vz;
    fmpz_init(u); fmpz_init(low); fmpz_init(z); fmpz_init(Q); fmpz_init(inv); fmpz_init(term);
    fmpz_init_set_ui(pp, p);
    fmpz_add_ui(u, zr, 1); fmpz_mod(u, u, PK); fmpz_zero(S);
    while (v < K)
    {
        slong m = v <= K / 2 ? 2 * v : K;
        fmpz_ui_pow_ui(Q, p, (ulong) m); fmpz_mod(low, u, Q);
        fmpz_sub_ui(z, low, 1);
        if (!fmpz_is_zero(z))
        {
            slong w = fmpz_remove(term, z, pp);
            log_short(term, p, z, w, K, PK);
            fmpz_add(S, S, term);
        }
        if (m == K) break;
        if (!fmpz_is_one(low))
        {
            /* u-low is divisible by p^m. Only K-m digits of low^(-1) are needed (F9). */
            fmpz_sub(term, u, low); fmpz_divexact(term, term, Q);
            fmpz_divexact(z, PK, Q);
            fmpz_invmod(inv, low, z); fmpz_mul(term, term, inv); fmpz_mod(term, term, z);
            fmpz_mul(u, term, Q); fmpz_add_ui(u, u, 1);
        }
        v = m;
    }
    fmpz_mod(S, S, PK);
    fmpz_clear(u); fmpz_clear(low); fmpz_clear(z); fmpz_clear(Q); fmpz_clear(inv); fmpz_clear(term);
    fmpz_clear(pp);
}

/* v_p of a nonzero rational. */
static slong
val_fmpq(const fmpq_t q, ulong p)
{
    fmpz_t P, r;
    slong a, b;
    fmpz_init_set_ui(P, p);
    fmpz_init(r);
    a = fmpz_remove(r, fmpq_numref(q), P);
    b = fmpz_remove(r, fmpq_denref(q), P);
    fmpz_clear(P);
    fmpz_clear(r);
    return a - b;
}

/* res = Log(p^m a) + p^K Z_p for the unit a of Z_p (a rational unit, or an integer prime to p), c < K,
   pow_ok(p, K), a != 1 and a != -1. F3: at p = 2 Log = log(s a), s = +-1 with s a = 1 modulo 4; at odd p
   Log = log(a) if a = 1 modulo p, else log(a^(p-1)) / (p - 1). Returns OK or LIMIT (p^W beyond the bound). */
static int
Log_centre(adf_lball_struct * res, ulong p, const fmpq_t a, slong K)
{
    fmpq_t z;
    fmpz_t P, PK, zr, t, S;
    slong vz, T, W;
    int power = 0, st = ADF_OK;
    fmpq_init(z);
    fmpz_init(P); fmpz_init(PK); fmpz_init(zr); fmpz_init(t); fmpz_init(S);
    fmpz_ui_pow_ui(PK, p, (ulong) K);
    if (p == 2)
    {
        /* s a = 1 modulo 4: numerator = denominator modulo 4 for s = 1 (both odd) */
        fmpz_sub(t, fmpq_numref(a), fmpq_denref(a));
        if (fmpz_fdiv_ui(t, 4) == 0)
            fmpq_sub_si(z, a, 1);
        else
        {
            fmpq_neg(z, a);
            fmpq_sub_si(z, z, 1);
        }
    }
    else
    {
        fmpz_sub(t, fmpq_numref(a), fmpq_denref(a));
        if (fmpz_fdiv_ui(t, p) == 0)
            fmpq_sub_si(z, a, 1);                   /* a = 1 modulo p: w = 1 (F3) */
        else
            power = 1;
    }
    if (!power)
    {
        /* z != 0 (a != +-1), v(z) >= c; if v(z) >= K then v(log(1 + z)) = v(z) >= K (Lemma 9, item 5) */
        vz = val_fmpq(z, p);
        if (vz >= K)
        {
            fmpz_zero(S);
            goto done;
        }
        T = count_log(p, K, vz);
        W = K + floor_log((ulong) (T > 0 ? T : 1), p);
        if (!pow_ok(p, W))
        {
            st = ADF_LIMIT;
            goto out;
        }
        fmpz_ui_pow_ui(P, p, (ulong) W);
        rat_mod(zr, z, P);
    }
    else
    {
        /* z = a^(p-1) - 1, v(z) >= 1: W from the count for the lower bound 1 (F5: T and e(T) do not increase
           when the bound on the valuation increases), then vz = min(v_p(z_r), W) */
        slong Tc = count_log(p, K, 1);
        W = K + floor_log((ulong) (Tc > 0 ? Tc : 1), p);
        if (!pow_ok(p, W))
        {
            st = ADF_LIMIT;
            goto out;
        }
        fmpz_ui_pow_ui(P, p, (ulong) W);
        rat_mod(t, a, P);
        fmpz_powm_ui(zr, t, p - 1, P);
        fmpz_sub_ui(zr, zr, 1);
        fmpz_mod(zr, zr, P);
        if (fmpz_is_zero(zr))
            vz = W;
        else
        {
            fmpz_t pp;
            fmpz_init_set_ui(pp, p);
            vz = fmpz_remove(t, zr, pp);
            fmpz_clear(pp);
            if (vz > W)
                vz = W;
        }
        if (vz >= K)
        {
            fmpz_zero(S);
            goto done;
        }
        T = count_log(p, K, vz);
    }
    if (W * (slong) FLINT_BIT_COUNT(p) <= FLINT_BITS - 2)
        log_sum_word(S, p, zr, T, P, PK);
    else if (K <= 64)
        log_sum(S, p, zr, vz, K, T, PK);
    else
        log_balanced(S, p, zr, vz, K, PK);
    if (power)
    {
        fmpz_set_ui(t, p - 1);
        fmpz_invmod(t, t, PK);                      /* p - 1 is a unit at odd p */
        fmpz_mul(S, S, t);
        fmpz_mod(S, S, PK);
    }
done:
    set_ball_residue(res, p, S, K);
out:
    fmpq_clear(z);
    fmpz_clear(P); fmpz_clear(PK); fmpz_clear(zr); fmpz_clear(t); fmpz_clear(S);
    return st;
}

/* Log of a canonical x != 0 (not a ball around 0), inputs within the bounds: the common part of log and Log. */
static int
Log_core(adf_lball_t y, const adf_lball_struct * x, slong N)
{
    adf_lball_t res;
    ulong p = x->p;
    slong c = p == 2 ? 2 : 1, K, E, r;
    adf_lball_init(res);
    if (x->exact)
    {
        /* F2: Log(+-p^m) = 0 exactly (the unit part is +-1: w = +-1, u = 1) */
        if (fmpz_is_one(fmpq_denref(x->u)) && fmpz_is_pm1(fmpq_numref(x->u)))
        {
            set_exact_small(res, p, 0);
            return finish(y, res, ADF_OK);
        }
        K = N;
    }
    else
    {
        /* F6, Proposition 11: r = M - m; E = r if r >= c, E = 2 if p = 2 and r = 1 */
        r = x->N - x->v;
        E = r >= c ? r : 2;
        K = N < E ? N : E;
    }
    if (!exp_ok(K))
        return finish(y, res, ADF_LIMIT);
    if (K <= c || fmpq_is_one(x->u))
    {
        /* Log(x) is in p^c Z_p (Proposition 12, step 4), or the unit part of the centre is 1 and Log of the centre
           is 0: the centre of the result is 0 */
        fmpz_t zero;
        fmpz_init(zero);
        set_ball_residue(res, p, zero, K);
        fmpz_clear(zero);
        return finish(y, res, ADF_OK);
    }
    if (!pow_ok(p, K))
        return finish(y, res, ADF_LIMIT);
    return finish(y, res, Log_centre(res, p, x->u, K));
}

int
adf_lball_log(adf_lball_t y, const adf_lball_t x, slong N)
{
    int st;
    ADF_INV_LBALL(x);
    if (!in_bounds(x))
        return ADF_LIMIT;
    st = domain_status(x, F_LOG);
    if (st != ADF_OK)
        return st;
    /* on 1 + p Z_p the series log is Log (Proposition 11, step 3); a ball inside has v = 0 and exponent M >= 1, so
       E of Log is E of log (lfunc.h). The exact -1 at p = 2 has the unit part -1: exact 0 (log(-1) = 0). */
    return Log_core(y, x, N);
}

int
adf_lball_Log(adf_lball_t y, const adf_lball_t x, slong N)
{
    ADF_INV_LBALL(x);
    if (!in_bounds(x))
        return ADF_LIMIT;
    if (fmpq_is_zero(x->u))
        return x->exact ? ADF_DOMAIN : ADF_NOT_DETERMINED;
    return Log_core(y, x, N);
}
