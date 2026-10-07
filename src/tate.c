/* src/tate.c: the global Tate test vector and the global Tate integral in Re(s) > 1, slice 5c of docs/api-5.md
   (include/adelefeld/tate.h; statements, proofs and decisions in docs/api-5b.md "Slice 5c").
   Sources, read before the code:
   - docs/api-5.md:9-46 (section 1: values, scope, the order of the checks, D2), :121-165 (section 3: P11's value,
     D3, the vector, the balanced change A = 1/C, the dual coefficients W_chi conj(chi(n)) n^e from the actual
     transforms, the two comment blocks), :183-228 (section 4: P13's split, a0 = pi/C, E_n and E_t of P15 with
     both branches of L14, the searches N = 0, 1, 2, 4, ... and R = 1, 2, 4, ... to epsilon/32, the I budget
     epsilon = 2^-bits/max(1, upper|C^-z|), D4's panels [1, 2], [2, 4], ..., the Taylor recurrence, the majorant
     B and the panel error 2 h B q^(J+1)/(1 - q) <= epsilon/(64 panels), the width check and the retry rule),
     :286-300 (T3), :313-325 (T5);
   - docs/proofs/analysis.md:213-229 (Lemma 6, S_j(c, T) with beta = 0), :452-495 (Proposition 11: I_chi =
     pi^-z Gamma(z) L(s, chi), Lambda = C^z I), :564-626 (Proposition 13 step 3: Lambda = integral_1^inf V_chi(t)
     t^(z-1) dt + W_chi integral_1^inf V_conj(chi)(t) t^(z'-1) dt + delta [1/(s-1) - 1/s], z' = (1-s+e)/2),
     :627-651 (Lemma 14: J_bound, both branches), :652-732 (Proposition 15: E_sum, E_integral);
   - docs/conventions.md 6.5 (the test vector: f[j] = chi(j) on Zhat, x^e exp(-pi x^2) at infinity);
   - refs/src/flint-3.0.1/arb.rst:6-12 (a ball result contains the exact operation on every input point), :430-443
     (arb_get_ubound_arf, arb_get_lbound_arf: bounds rounded outward), :705-718 (arb_le, arb_gt: the comparison
     holds for all points); acb.rst:6-17 (a complex ball is a rectangle).
   Reused, not reimplemented: adf_char_chi (the phases), adf_ffun_set_acb_vec, adf_rfun_set_terms (the setters),
   adf_ffun_fourier and adf_rfun_fourier (the dual coefficients). The Lemma 6 kernel of src/poisson.c
   (pn_series) is static and cannot be called from here; tt_series below is the beta = 0 case of the same rule
   (a finding of the lane report). Every result is built in temporaries and committed after the last check. */
#include <adelefeld.h>
#include "invariants.h"
#include <string.h>

#ifdef ADF_CHECK_INVARIANTS
static ADF_INV_NOINLINE void tt_inv(int ok, const char * fn, const char * arg, const char * type)
{
    if (!ok) adf_inv_fail(fn, arg, type);
}
#define TT_INV(c, arg, type) tt_inv((c), __func__, (arg), (type))
#else
#define TT_INV(c, arg, type) ((void) 0)
#endif

/* ------------------------------------------------------------------ the vector (api-5.md:132-133, :152-156) */

/* Into fresh outputs: f = (D = 1, M = C, f[j] = chi(j)), phi = x^e exp(-pi A x^2) with A = 1 (tate.h). The value
   of chi(j) is adf_char_chi's: the exact 0 on nonunits, exact cardinal phases, a ball otherwise; for C = 1 every
   phase is 0, so f[0] = 1 (analysis L8:300). UNSUPPORTED of the character setup is NOT_DETERMINED (api-5.md:37). */
static int
tt_vector(adf_rfun_t phi, adf_ffun_t f, const adf_char_t chi, const arb_t A, slong p)
{
    ulong C = chi->q, j;
    acb_ptr v = _acb_vec_init((slong) C);
    adf_rterm_struct t;
    fmpz_t a;
    int st = ADF_OK;

    fmpz_init(a);
    for (j = 0; j < C && st == ADF_OK; j++)
    {
        fmpz_set_ui(a, j);
        st = adf_char_chi(v + j, chi, a, p);
    }
    if (st == ADF_UNSUPPORTED)
        st = ADF_NOT_DETERMINED;
    if (st == ADF_OK)
        st = adf_ffun_set_acb_vec(f, 1, C, v, (slong) C);
    acb_poly_init(t.P);
    acb_init(t.A);
    acb_init(t.B);
    acb_init(t.C);
    acb_poly_set_coeff_si(t.P, chi->parity, 1);           /* P = x^e */
    acb_set_arb(t.A, A);
    if (st == ADF_OK)
        st = adf_rfun_set_terms(phi, &t, 1);
    if (st == ADF_DOMAIN)                                 /* Re(A) not certified positive at a low precision */
        st = ADF_NOT_DETERMINED;
    acb_poly_clear(t.P);
    acb_clear(t.A);
    acb_clear(t.B);
    acb_clear(t.C);
    _acb_vec_clear(v, (slong) C);
    fmpz_clear(a);
    return st;
}

int
adf_tate_vector(adf_rfun_t phi, adf_ffun_t f, const adf_char_t chi, slong prec)
{
    adf_rfun_t ph;
    adf_ffun_t ff;
    arb_t one;
    int st;

    if (prec > ADF_REAL_PREC_MAX || chi->q > ADF_CHAR_MOD_MAX)
        return ADF_LIMIT;
    TT_INV(adf_char_is_canonical(chi), "chi", "adf_char");
    TT_INV((const void *) phi != (const void *) f, "phi", "output distinct from f");
    adf_rfun_init(ph);
    adf_ffun_init(ff);
    arb_init(one);
    arb_one(one);
    st = tt_vector(ph, ff, chi, one, FLINT_MAX(prec, 2));
    if (st == ADF_OK)
    {
        adf_rfun_swap(phi, ph);
        adf_ffun_swap(f, ff);
    }
    adf_rfun_clear(ph);
    adf_ffun_clear(ff);
    arb_clear(one);
    return st;
}

/* ------------------------------------------------------------------ the integral: constants and work (D2) */

/* bits in [0, 2^21] (api-5.md:159). */
#define TT_BITS_MAX ((slong) 2097152)
/* The precision of the bounds (Lemma 6, Lemma 14, T3's majorant): they are upper bounds at any precision. */
#define TT_TP 64
/* A doubling from a precision at least this that does not halve the largest diameter stops the retries
   (api-5.md:224-226). */
#define TT_STALL_PREC 64

/* Charges n work units; 0 (nothing charged) if the total would pass ADF_TATE_WORK_MAX (api-5.md:41-44). */
static int
tt_charge(ulong * w, ulong n)
{
    if (n > (ulong) ADF_TATE_WORK_MAX - *w)
        return 0;
    *w += n;
    return 1;
}

/* ------------------------------------------------------------------ Lemma 6 and Lemma 14 */

/* S >= sum_(n > N) n^j exp(-c n^2) for an exact c > 0 (analysis Lemma 6 with beta = 0,
   docs/proofs/analysis.md:215-221): K from N + 1; while the upper bound of rho_j(c, K) = exp(j/K - c (2K + 1)) is
   not <= 1/2 the term K^j exp(-c K^2) joins the prefix and K grows; at the first certified K the geometric bound
   term/(1 - rho) is added. The bound decreases in c, so a lower bound of c bounds every c of a ball. One work
   unit per K, the last included. ADF_OK or ADF_LIMIT. */
static int
tt_series(arb_t S, ulong j, const arb_t c, ulong N, ulong * w)
{
    arb_t prefix, rho, term, t;
    arf_t b;
    ulong K = N + 1;
    int st = ADF_OK;

    arb_init(prefix);
    arb_init(rho);
    arb_init(term);
    arb_init(t);
    arf_init(b);
    for (;;)
    {
        if (!tt_charge(w, 1))
        {
            st = ADF_LIMIT;
            break;
        }
        arb_set_ui(t, j);
        arb_div_ui(t, t, K, TT_TP);
        arb_mul_ui(rho, c, 2 * K + 1, TT_TP);
        arb_sub(t, t, rho, TT_TP);
        arb_exp(rho, t, TT_TP);                           /* rho_j(c, K) */
        arb_mul_ui(t, c, K, TT_TP);
        arb_mul_ui(t, t, K, TT_TP);
        arb_neg(t, t);
        arb_exp(term, t, TT_TP);
        if (j)
            arb_mul_ui(term, term, K, TT_TP);             /* K^j exp(-c K^2), j in {0, 1} */
        arb_get_ubound_arf(b, rho, TT_TP);
        if (arf_is_finite(b) && arf_cmp_2exp_si(b, -1) <= 0)
        {
            arb_sub_ui(t, rho, 1, TT_TP);
            arb_neg(t, t);
            arb_div(term, term, t, TT_TP);
            arb_add(S, prefix, term, TT_TP);
            break;
        }
        arb_add(prefix, prefix, term, TT_TP);
        K++;
    }
    arb_clear(prefix);
    arb_clear(rho);
    arb_clear(term);
    arb_clear(t);
    arf_clear(b);
    return st;
}

/* J >= integral_R^infinity t^r exp(-b t) dt for exact r, exact b > 0 and R >= 1 (analysis Lemma 14,
   docs/proofs/analysis.md:630-639, both branches): if b R >= 2 max(r, 0) (decided exactly), 2 R^r exp(-b R)/b;
   otherwise r > 0, R0 = 2r/b, t* = min(max(r/b, R), R0) and (R0 - R) t*^r exp(-b t*) + 2 R0^r exp(-b R0)/b.
   The balls contain the exact values (arb.rst:6-12); the caller takes their upper bounds. The bound increases in
   r and decreases in b for t >= 1, so an upper r and a lower b bound every point of a ball. */
static void
tt_jbound(arb_t J, const arf_t r, const arf_t b, ulong R)
{
    arb_t rr, bb, RR, R0, ts, t, u;
    arf_t bR, rp;

    arb_init(rr);
    arb_init(bb);
    arb_init(RR);
    arb_init(R0);
    arb_init(ts);
    arb_init(t);
    arb_init(u);
    arf_init(bR);
    arf_init(rp);
    arb_set_arf(rr, r);
    arb_set_arf(bb, b);
    arb_set_ui(RR, R);
    arf_mul_ui(bR, b, R, ARF_PREC_EXACT, ARF_RND_DOWN);
    if (arf_sgn(r) > 0)
        arf_mul_2exp_si(rp, r, 1);                        /* 2 max(r, 0) */
    if (arf_cmp(bR, rp) >= 0)
    {
        arb_pow(t, RR, rr, TT_TP);
        arb_mul(u, bb, RR, TT_TP);
        arb_neg(u, u);
        arb_exp(u, u, TT_TP);
        arb_mul(J, t, u, TT_TP);
        arb_mul_2exp_si(J, J, 1);
        arb_div(J, J, bb, TT_TP);
    }
    else
    {
        arb_div(ts, rr, bb, TT_TP);                       /* r/b */
        arb_mul_2exp_si(R0, ts, 1);                       /* R0 = 2r/b */
        arb_max(ts, ts, RR, TT_TP);
        arb_min(ts, ts, R0, TT_TP);                       /* t* */
        arb_pow(t, ts, rr, TT_TP);
        arb_mul(u, bb, ts, TT_TP);
        arb_neg(u, u);
        arb_exp(u, u, TT_TP);
        arb_mul(t, t, u, TT_TP);
        arb_sub(u, R0, RR, TT_TP);
        arb_mul(J, t, u, TT_TP);                          /* (R0 - R) t*^r exp(-b t*) */
        arb_pow(t, R0, rr, TT_TP);
        arb_mul(u, bb, R0, TT_TP);
        arb_neg(u, u);
        arb_exp(u, u, TT_TP);
        arb_mul(t, t, u, TT_TP);
        arb_mul_2exp_si(t, t, 1);
        arb_div(t, t, bb, TT_TP);
        arb_add(J, J, t, TT_TP);
    }
    arb_clear(rr);
    arb_clear(bb);
    arb_clear(RR);
    arb_clear(R0);
    arb_clear(ts);
    arb_clear(t);
    arb_clear(u);
    arf_clear(bR);
    arf_clear(rp);
}

/* E = exp(a0) S (J(r1, b, R) + J(r2, b, R)): P15's E_sum (R = 1, S = S_e(a0, N)) or E_integral (S = S_e(a0, 0))
   for the two Mellin exponents z and z' (analysis :656-661; api-5.md:196-204). ea0 encloses exp(a0). */
static void
tt_tail(arb_t E, const arb_t ea0, const arb_t S, const arf_t r1, const arf_t r2, const arf_t b, ulong R)
{
    arb_t t;

    arb_init(t);
    tt_jbound(E, r1, b, R);
    tt_jbound(t, r2, b, R);
    arb_add(E, E, t, TT_TP);
    arb_mul(E, E, S, TT_TP);
    arb_mul(E, E, ea0, TT_TP);
    arb_clear(t);
}

/* 1 iff the upper bound of x is <= y (exact). */
static int
tt_le(const arb_t x, const arf_t y)
{
    arf_t u;
    int ok;

    arf_init(u);
    arb_get_ubound_arf(u, x, TT_TP);
    ok = arf_is_finite(u) && arf_cmp(u, y) <= 0;
    arf_clear(u);
    return ok;
}

/* ------------------------------------------------------------------ T3: one Mellin integral on [1, R] */

/* val encloses sum_(n=1)^N coef[n] times the polynomial part of integral_1^R exp(-a0 n^2 t) t^(zz-1) dt by the
   composite Taylor rule of D4 (api-5.md:207-220, T3 :286-300), and qerr bounds the total panel remainder, for
   every zz of the ball and every true coefficient of modulus <= n^e. R = 2^K; the panels are [2^k, 2^(k+1)],
   k < K, with m = 3 2^(k-1), h = 2^(k-1), d = m/2, q = h/d = 2/3; the majorant
   B = sum_n n^e exp(-a0 n^2 (m-d)) max((m-d)^r, (m+d)^r) exp(pi |Im zz|/2), r = Re(zz) - 1 (a ball: every r of
   the rectangle), the panel error 2h B q^(J+1)/(1-q) = 6 h B (2/3)^(J+1) <= target/K with J the least such.
   The coefficients: c_-1 = 0, c_0 = exp(-b m) m^(zz-1), c_(k+1) = ((zz-1-b m-k) c_k - b c_(k-1))/(m (k+1)),
   b = a0 n^2, and the integral 2h sum_(2j <= J) c_(2j) h^(2j)/(2j+1). A coefficient that is the exact 0 is
   skipped (its term is 0). (J+1) work units per nonzero coefficient and panel, charged before the panel is
   computed. deg, if not NULL, receives J of each panel (at most 64 panels). ADF_OK or ADF_LIMIT. */
static int
tt_side(acb_t val, arb_t qerr, const acb_t zz, acb_srcptr coef, ulong N, int e, const arb_t a0, ulong R,
        const arf_t target, slong w, ulong * work, ulong * deg)
{
    acb_t zm1, base, prev, cur, nxt, t, integral;
    arb_t r, B, P, x, y, err, two3, b, bm, lm, expim, a0t;
    arf_t tp;
    ulong K = 0, k, n, J, nz = 0;
    int st = ADF_OK;

    acb_zero(val);
    arb_zero(qerr);
    while ((UWORD(1) << K) < R)
        K++;
    if (K == 0)
        return ADF_OK;
    acb_init(zm1);
    acb_init(base);
    acb_init(prev);
    acb_init(cur);
    acb_init(nxt);
    acb_init(t);
    acb_init(integral);
    arb_init(r);
    arb_init(B);
    arb_init(P);
    arb_init(x);
    arb_init(y);
    arb_init(err);
    arb_init(two3);
    arb_init(b);
    arb_init(bm);
    arb_init(lm);
    arb_init(expim);
    arb_init(a0t);
    arf_init(tp);
    for (n = 1; n <= N; n++)
        nz += !acb_is_zero(coef + n);
    arf_div_ui(tp, target, K, TT_TP, ARF_RND_DOWN);       /* target/(number of panels) */
    acb_sub_ui(zm1, zz, 1, w);
    arb_set(r, acb_realref(zm1));
    arb_set_round(a0t, a0, TT_TP);
    arb_abs(x, acb_imagref(zz));
    arb_get_ubound_arf(arb_midref(expim), x, TT_TP);
    arb_const_pi(x, TT_TP);
    arb_mul(expim, expim, x, TT_TP);
    arb_mul_2exp_si(expim, expim, -1);
    arb_exp(expim, expim, TT_TP);                         /* exp(pi upper|Im zz|/2) */
    arb_set_ui(two3, 2);
    arb_div_ui(two3, two3, 3, TT_TP);
    for (k = 0; k < K && st == ADF_OK; k++)
    {
        slong ke = (slong) k - 1;                         /* h = 2^ke, m = 3 2^ke */
        /* the majorant B on |u| = d: m - d = 3 2^(k-2), m + d = 9 2^(k-2) */
        arb_set_ui(x, 3);
        arb_mul_2exp_si(x, x, (slong) k - 2);
        arb_pow(y, x, r, TT_TP);
        arb_get_ubound_arf(arb_midref(P), y, TT_TP);
        mag_zero(arb_radref(P));
        arb_set_ui(y, 9);
        arb_mul_2exp_si(y, y, (slong) k - 2);
        arb_pow(y, y, r, TT_TP);
        arb_max(P, P, y, TT_TP);                          /* max((m-d)^r, (m+d)^r) */
        arb_zero(B);
        for (n = 1; n <= N; n++)
        {
            arb_mul_ui(y, a0t, n * n, TT_TP);
            arb_mul(y, y, x, TT_TP);
            arb_neg(y, y);
            arb_exp(y, y, TT_TP);
            if (e)
                arb_mul_ui(y, y, n, TT_TP);
            arb_add(B, B, y, TT_TP);
        }
        arb_mul(B, B, P, TT_TP);
        arb_mul(B, B, expim, TT_TP);
        arb_mul_ui(err, B, 6, TT_TP);
        arb_mul_2exp_si(err, err, ke);                    /* 6 h B */
        arb_mul(err, err, two3, TT_TP);                   /* J = 0 */
        J = 0;
        if (nz == 0)                                      /* every coefficient is the exact 0: no term */
            arb_zero(err);
        else if (!arb_is_finite(err))
            st = ADF_NOT_DETERMINED;
        while (st == ADF_OK && !tt_le(err, tp))
        {
            J++;
            if (J + 1 > ((ulong) ADF_TATE_WORK_MAX - *work) / nz)
                st = ADF_LIMIT;
            arb_mul(err, err, two3, TT_TP);
        }
        if (st == ADF_OK && !tt_charge(work, (J + 1) * nz))
            st = ADF_LIMIT;
        if (st != ADF_OK)
            break;
        arb_add(qerr, qerr, err, TT_TP);
        if (deg != NULL)
            deg[k] = J;
        /* log m = log(3 2^ke) */
        arb_set_ui(lm, 3);
        arb_mul_2exp_si(lm, lm, ke);
        arb_log(lm, lm, w);
        for (n = 1; n <= N; n++)
        {
            ulong i;
            if (acb_is_zero(coef + n))
                continue;
            arb_mul_ui(b, a0, n * n, w);                  /* b_n = a0 n^2 */
            arb_mul_ui(bm, b, 3, w);
            arb_mul_2exp_si(bm, bm, ke);                  /* b m */
            acb_mul_arb(cur, zm1, lm, w);
            arb_sub(acb_realref(cur), acb_realref(cur), bm, w);
            acb_exp(cur, cur, w);                         /* c_0 = exp(-b m) m^(zz-1) */
            acb_zero(prev);
            acb_mul_2exp_si(integral, cur, ke + 1);       /* 2 h c_0 */
            acb_set(base, zm1);
            arb_sub(acb_realref(base), acb_realref(base), bm, w);
            for (i = 0; i < J; i++)
            {
                acb_sub_ui(t, base, i, w);
                acb_mul(nxt, t, cur, w);
                acb_mul_arb(t, prev, b, w);
                acb_sub(nxt, nxt, t, w);
                acb_div_ui(nxt, nxt, 3 * (i + 1), w);
                acb_mul_2exp_si(nxt, nxt, -ke);           /* / (m (i + 1)) */
                acb_swap(prev, cur);
                acb_swap(cur, nxt);                       /* cur = c_(i+1) */
                if ((i + 1) % 2 == 0)
                {
                    acb_mul_2exp_si(t, cur, ke * (slong) (i + 2) + 1);
                    acb_div_ui(t, t, i + 2, w);           /* 2 c_(i+1) h^(i+2)/(i+2) */
                    acb_add(integral, integral, t, w);
                }
            }
            acb_addmul(val, coef + n, integral, w);
        }
    }
    acb_clear(zm1);
    acb_clear(base);
    acb_clear(prev);
    acb_clear(cur);
    acb_clear(nxt);
    acb_clear(t);
    acb_clear(integral);
    arb_clear(r);
    arb_clear(B);
    arb_clear(P);
    arb_clear(x);
    arb_clear(y);
    arb_clear(err);
    arb_clear(two3);
    arb_clear(b);
    arb_clear(bm);
    arb_clear(lm);
    arb_clear(expim);
    arb_clear(a0t);
    arf_clear(tp);
    return st;
}

/* ------------------------------------------------------------------ one attempt at the working precision w */

/* The certificate of one attempt, for the hidden test hook adf_tate_cutoffs: N, R and each panel degree J of the
   two sides (forward, dual). */
typedef struct
{
    ulong N, R, K, deg[2][64];
} tt_trace;

/* out encloses I_chi(s) for every s of the ball (or the attempt fails): analysis P13 step 3 (:589-598) on the
   balanced vector, then the factor C^-z (P11 :463-466). Statuses: OK (out written, width not yet checked),
   LIMIT (D2), NOT_DETERMINED (a dependency did not certify at this precision: the caller retries). */
static int
tt_attempt(acb_t out, const adf_char_t chi, const acb_t s, slong bits, slong w, ulong * work, tt_trace * tr)
{
    ulong C = chi->q, N, R, n;
    int e = chi->parity, st;
    adf_rfun_t phi, hat;
    adf_ffun_t f, g;
    acb_t zz, zd, cz, lam, t, y;
    acb_ptr an = NULL, bn = NULL;
    arb_t A, a0, a0lo, ea0, S, En, Et, el, er;
    arf_t eps, tg, r1, r2, lo;
    mag_t m;
    int written = 0;

    adf_rfun_init(phi);
    adf_rfun_init(hat);
    adf_ffun_init(f);
    adf_ffun_init(g);
    acb_init(zz);
    acb_init(zd);
    acb_init(cz);
    acb_init(lam);
    acb_init(t);
    acb_init(y);
    arb_init(A);
    arb_init(a0);
    arb_init(a0lo);
    arb_init(ea0);
    arb_init(S);
    arb_init(En);
    arb_init(Et);
    arb_init(el);
    arb_init(er);
    arf_init(eps);
    arf_init(tg);
    arf_init(r1);
    arf_init(r2);
    arf_init(lo);
    mag_init(m);
    /* the balanced vector: f of conventions 6.5, P = x^e, A = 1/C (api-5.md:132-137); C phase entries */
    st = tt_charge(work, C) ? ADF_OK : ADF_LIMIT;
    if (st == ADF_OK)
    {
        arb_one(A);
        arb_div_ui(A, A, C, w);
        st = tt_vector(phi, f, chi, A, w);
    }
    /* its transforms: the dual coefficients (api-5.md:133-137); C^2 multiply-adds, 2 L^2 - L + 1 for the term */
    if (st == ADF_OK)
        st = tt_charge(work, C * C + 2 * (ulong) (e + 1) * (ulong) (e + 1) - (ulong) e) ? ADF_OK : ADF_LIMIT;
    if (st == ADF_OK)
        st = adf_ffun_fourier(g, f, w);
    if (st == ADF_OK)
        st = adf_rfun_fourier(hat, phi, w);
    if (st != ADF_OK)
        goto done;
    /* z = (s + e)/2, z' = (1 - s + e)/2 */
    acb_add_si(zz, s, e, w);
    acb_mul_2exp_si(zz, zz, -1);
    acb_neg(zd, s);
    acb_add_si(zd, zd, 1 + e, w);
    acb_mul_2exp_si(zd, zd, -1);
    /* the budget epsilon = 2^-bits/max(1, upper|C^-z|) (api-5.md:205) */
    arf_one(eps);
    arf_mul_2exp_si(eps, eps, -bits);
    if (C > 1)
    {
        arb_log_ui(acb_realref(t), C, w);
        arb_zero(acb_imagref(t));
        acb_mul(cz, zz, t, w);
        acb_neg(cz, cz);
        acb_exp(cz, cz, w);                               /* C^-z */
        acb_get_mag(m, cz);
        if (!mag_is_finite(m))
        {
            st = ADF_NOT_DETERMINED;
            goto done;
        }
        if (mag_cmp_2exp_si(m, 0) > 0)
        {
            arf_set_mag(lo, m);
            arf_div(eps, eps, lo, TT_TP, ARF_RND_DOWN);
        }
    }
    /* a0 = pi/C; its lower bound for the series and the exponential integrals, exp(a0) */
    arb_const_pi(a0, w);
    arb_div_ui(a0, a0, C, w);
    arb_get_lbound_arf(lo, a0, TT_TP);
    arb_set_arf(a0lo, lo);
    arb_exp(ea0, a0, TT_TP);
    /* upper real exponents r = upper(Re z) - 1 and upper(Re z') - 1 */
    arb_get_ubound_arf(r1, acb_realref(zz), TT_TP);
    arf_sub_ui(r1, r1, 1, TT_TP, ARF_RND_CEIL);
    arb_get_ubound_arf(r2, acb_realref(zd), TT_TP);
    arf_sub_ui(r2, r2, 1, TT_TP, ARF_RND_CEIL);
    if (!arf_is_finite(r1) || !arf_is_finite(r2) || arf_sgn(lo) <= 0)
    {
        st = ADF_NOT_DETERMINED;
        goto done;
    }
    /* N from 0, 1, 2, 4, ... with E_n(z, N) + E_n(z', N) <= epsilon/32 (api-5.md:202-203) */
    arf_mul_2exp_si(tg, eps, -5);
    for (N = 0;; N = N ? 2 * N : 1)
    {
        if (!tt_charge(work, 1) || (st = tt_series(S, (ulong) e, a0lo, N, work)) != ADF_OK)
        {
            st = ADF_LIMIT;
            goto done;
        }
        tt_tail(En, ea0, S, r1, r2, lo, 1);
        if (tt_le(En, tg))
            break;
    }
    /* R from 1, 2, 4, ... with E_t(z, R) + E_t(z', R) <= epsilon/32 */
    if ((st = tt_series(S, (ulong) e, a0lo, 0, work)) != ADF_OK)
        goto done;
    for (R = 1;; R *= 2)
    {
        if (!tt_charge(work, 1) || R > (UWORD(1) << 40))
        {
            st = ADF_LIMIT;
            goto done;
        }
        tt_tail(Et, ea0, S, r1, r2, lo, R);
        if (tt_le(Et, tg))
            break;
    }
    /* the coefficients: chi(n) n^e from the vector, W_chi conj(chi(n)) n^e as g[n mod C] Q(n/C) from the two
       transforms (api-5.md:136-138; P13 step 5 :613-618); 2 N entries */
    if (!tt_charge(work, 2 * N))
    {
        st = ADF_LIMIT;
        goto done;
    }
    an = _acb_vec_init((slong) N + 1);
    bn = _acb_vec_init((slong) N + 1);
    for (n = 1; n <= N; n++)
    {
        acb_mul_ui(an + n, f->f + n % C, e ? n : 1, w);
        acb_set_ui(y, n);
        acb_div_ui(y, y, C, w);
        acb_poly_evaluate(t, hat->term[0].P, y, w);
        acb_mul(bn + n, t, g->f + n % C, w);
    }
    /* the two Mellin integrals on [1, R], each with remainder target epsilon/64 */
    arf_mul_2exp_si(tg, eps, -6);
    if (tr != NULL)
    {
        tr->N = N;
        tr->R = R;
        for (tr->K = 0; (UWORD(1) << tr->K) < R; tr->K++)
            ;
    }
    st = tt_side(lam, el, zz, an, N, e, a0, R, tg, w, work, tr == NULL ? NULL : tr->deg[0]);
    if (st == ADF_OK)
        st = tt_side(t, er, zd, bn, N, e, a0, R, tg, w, work, tr == NULL ? NULL : tr->deg[1]);
    if (st != ADF_OK)
        goto done;
    acb_add(lam, lam, t, w);
    if (C == 1)
    {   /* delta [1/(s-1) - 1/s] (P13 step 3) */
        acb_sub_ui(t, s, 1, w);
        acb_inv(t, t, w);
        acb_add(lam, lam, t, w);
        acb_inv(t, s, w);
        acb_sub(lam, lam, t, w);
    }
    /* the omitted regions and the quadrature remainders, to both coordinate radii (api-5.md:218) */
    arb_add(En, En, Et, TT_TP);
    arb_add(En, En, el, TT_TP);
    arb_add(En, En, er, TT_TP);
    arb_get_ubound_arf(lo, En, TT_TP);
    acb_add_error_arf(lam, lo);
    /* I = C^-z Lambda (P11) */
    if (C > 1)
        acb_mul(lam, lam, cz, w);
    acb_swap(out, lam);
    written = 1;
done:
    if (st == ADF_OK && !written)                         /* OK only with the output written */
        st = ADF_NOT_DETERMINED;
    if (an != NULL)
        _acb_vec_clear(an, (slong) N + 1);
    if (bn != NULL)
        _acb_vec_clear(bn, (slong) N + 1);
    adf_rfun_clear(phi);
    adf_rfun_clear(hat);
    adf_ffun_clear(f);
    adf_ffun_clear(g);
    acb_clear(zz);
    acb_clear(zd);
    acb_clear(cz);
    acb_clear(lam);
    acb_clear(t);
    acb_clear(y);
    arb_clear(A);
    arb_clear(a0);
    arb_clear(a0lo);
    arb_clear(ea0);
    arb_clear(S);
    arb_clear(En);
    arb_clear(Et);
    arb_clear(el);
    arb_clear(er);
    arf_clear(eps);
    arf_clear(tg);
    arf_clear(r1);
    arf_clear(r2);
    arf_clear(lo);
    mag_clear(m);
    return st;
}

#if defined(__GNUC__) || defined(__clang__)
#define TT_HIDDEN __attribute__((visibility("hidden")))
#else
#define TT_HIDDEN
#endif

/* Not part of the interface (hidden in a shared object; tests/test_exports.sh; as src/roots.c:49-53): tt_side
   with a0 = pi/C at max(prec, 2), for tests/test_tate.c (T3 against an independent integral). */
TT_HIDDEN int adf_tate_taylor_piece(acb_t val, arb_t qerr, const acb_t z, acb_srcptr coef, ulong N, int e, ulong C,
                                    ulong R, const arf_t target, slong prec);
TT_HIDDEN int
adf_tate_taylor_piece(acb_t val, arb_t qerr, const acb_t z, acb_srcptr coef, ulong N, int e, ulong C, ulong R,
                      const arf_t target, slong prec)
{
    arb_t a0;
    ulong work = 0;
    slong w = FLINT_MAX(prec, 2);
    int st;

    arb_init(a0);
    arb_const_pi(a0, w);
    arb_div_ui(a0, a0, C, w);
    st = tt_side(val, qerr, z, coef, N, e, a0, R, target, w, &work, NULL);
    arb_clear(a0);
    return st;
}

/* Not part of the interface (hidden, as above): one attempt at max(prec, 2) on a canonical chi and a finite s
   with Re(s) > 1, returning its status, the cutoffs N, R, the number K of panels and the panel degrees J of the
   forward (deg[0..K)) and dual (deg[K..2K)) sides, and the work charged. For tests/test_tate.c (the oracle's
   cutoffs). deg holds 128 entries. */
TT_HIDDEN int adf_tate_cutoffs(ulong * N, ulong * R, ulong * K, ulong * deg, ulong * work, const adf_char_t chi,
                               const acb_t s, slong bits, slong prec);
TT_HIDDEN int
adf_tate_cutoffs(ulong * N, ulong * R, ulong * K, ulong * deg, ulong * work, const adf_char_t chi, const acb_t s,
                 slong bits, slong prec)
{
    tt_trace tr;
    acb_t out;
    ulong k;
    int st;

    acb_init(out);
    memset(&tr, 0, sizeof tr);
    *work = 0;
    st = tt_attempt(out, chi, s, bits, FLINT_MAX(prec, 2), work, &tr);
    *N = tr.N;
    *R = tr.R;
    *K = tr.K;
    for (k = 0; k < tr.K && k < 64; k++)
    {
        deg[k] = tr.deg[0][k];
        deg[tr.K + k] = tr.deg[1][k];
    }
    acb_clear(out);
    return st;
}

/* 1 iff z is finite and each coordinate diameter is <= 2^-bits. */
static int
tt_narrow(const acb_t z, slong bits)
{
    return acb_is_finite(z) && mag_cmp_2exp_si(arb_radref(acb_realref(z)), -bits - 1) <= 0
        && mag_cmp_2exp_si(arb_radref(acb_imagref(z)), -bits - 1) <= 0;
}

int
adf_tate_integral(acb_t z, const adf_char_t chi, const acb_t s, slong bits, slong prec)
{
    acb_t s0, res;
    arb_t one;
    mag_t cur, prev, two;
    ulong work = 0;
    slong w, pw = 0;
    int st, have = 0;

    if (prec > ADF_REAL_PREC_MAX || chi->q > ADF_TATE_TRANSFORM_C_MAX)
        return ADF_LIMIT;
    if (bits < 0 || bits > TT_BITS_MAX)
        return ADF_DOMAIN;
    TT_INV(adf_char_is_canonical(chi), "chi", "adf_char");
    TT_INV(z != chi->s, "z", "output distinct from chi->s");
    if (!acb_is_finite(s))
        return ADF_DOMAIN;
    arb_init(one);
    arb_one(one);
    st = arb_le(acb_realref(s), one) ? ADF_DOMAIN : arb_gt(acb_realref(s), one) ? ADF_OK : ADF_NOT_DETERMINED;
    arb_clear(one);
    if (st != ADF_OK)
        return st;
    acb_init(s0);
    acb_init(res);
    mag_init(cur);
    mag_init(prev);
    mag_init(two);
    acb_set(s0, s);                                       /* z may be s */
    for (w = FLINT_MAX(prec, 2);; w = FLINT_MIN(2 * w, ADF_REAL_PREC_MAX))
    {
        st = tt_attempt(res, chi, s0, bits, w, &work, NULL);
        if (st == ADF_LIMIT)
            break;
        if (st == ADF_OK && tt_narrow(res, bits))
        {
            acb_swap(z, res);
            break;
        }
        /* a width or certificate failure: the retry rule of api-5.md:223-226 */
        if (st == ADF_OK && acb_is_finite(res))
            mag_max(cur, arb_radref(acb_realref(res)), arb_radref(acb_imagref(res)));
        else
            mag_inf(cur);
        st = ADF_NOT_DETERMINED;
        if (have && pw >= TT_STALL_PREC)
        {   /* halved iff finite and 2 cur <= prev */
            mag_mul_2exp_si(two, cur, 1);
            if (!mag_is_finite(cur) || mag_cmp(two, prev) > 0)
                break;
        }
        if (w >= ADF_REAL_PREC_MAX)
            break;
        mag_set(prev, cur);
        pw = w;
        have = 1;
    }
    acb_clear(s0);
    acb_clear(res);
    mag_clear(cur);
    mag_clear(prev);
    mag_clear(two);
    return st;
}
