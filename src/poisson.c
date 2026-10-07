/* src/poisson.c: Poisson summation with certified tails, slice 4g of docs/api-4.md (include/adelefeld/tensor.h,
   adf_tensor_poisson; statements and proofs in docs/api-4c.md "Slice 4g"). A new file: src/tensor.c (684 lines)
   would pass about 1000 lines with this code appended.
   Sources, read before the code:
   - docs/api-4.md:297-345 (section 7, statement P1 steps 1-5, the comment block of adf_tensor_poisson), :20-41
     (section 1: common contract, caps D1), :426-428 (section 9 item 7), :430-449 (section 10, the theta witness);
   - docs/proofs/analysis.md:213-259 (Lemma 6: K = floor(T) + 1, rho_j(alpha, beta, K) = exp(j/K - alpha (2K + 1)
     + beta), first term K^j exp(-alpha K^2 + beta K), the prefix for floor(T) + 1 <= n < K0 and the geometric
     bound at K0; P(a + h n) = sum_j q_j n^j, alpha = pi Re(A h^2), beta = |Re(h (B - 2 pi A a))|,
     C' = C + B a - pi A a^2, B_phi(a, h, T) = 2 exp(Re(C')) sum_j |q_j| S_j(alpha, beta, T) bounds
     sum_(|n| > T) |phi(a + h n)|), :260-294 (Proposition 7: left = sum_j f_j sum_n phi(j/D + M n), right =
     sum_n g_(n mod DM) phihat(n/M), the truncation bounds sum_j |f_j| B_phi(j/D, M, T) and
     max_k |g_k| B_phihat(0, 1/M, T));
   - refs/src/flint-3.0.1/acb_poly.rst:391-395 (acb_poly_taylor_shift: g = f(x + c)); arb.rst:6-12 (a result
     contains the exact operation on every input point), :435-443 (arb_get_ubound_arf, arb_get_lbound_arf: bounds
     rounded outward); acb.rst:142-149 (acb_add_error_mag: adds to the radii of both parts), :290-292
     (acb_get_mag: an upper bound of |x|).
   Reused: adf_ffun_fourier (slice 4a), adf_rfun_fourier and adf_rfun_eval (slices 4d, 4e); no term evaluation is
   written here. Every result is built in temporaries and committed after the last check. */
#include <adelefeld.h>
#include "invariants.h"

#ifdef ADF_CHECK_INVARIANTS
static ADF_INV_NOINLINE void pn_inv(int ok, const char * fn, const char * arg, const char * type)
{
    if (!ok) adf_inv_fail(fn, arg, type);
}
#define PN_INV(c, arg, type) pn_inv((c), __func__, (arg), (type))
#else
#define PN_INV(c, arg, type) ((void) 0)
#endif

/* bits in [0, 2^21] (the comment block of api-4.md:335-336). */
#define PN_BITS_MAX ((slong) 2097152)
/* The precision of the tail bounds: they are upper bounds, any precision gives a valid one; 64 bits keep them
   within a factor 1 + 2^-50 or so of Lemma 6's value. */
#define PN_TP 64
/* A doubling from a precision at least this that does not halve the largest diameter stops the retries. */
#define PN_STALL_PREC 64

/* ------------------------------------------------------------------ D1 */

/* Charges n work units; 0 (and nothing charged) if the total would pass ADF_TENSOR_WORK_MAX. */
static int
pn_charge(ulong * w, ulong n)
{
    if (n > (ulong) ADF_TENSOR_WORK_MAX - *w)
        return 0;
    *w += n;
    return 1;
}

/* The units of one evaluation of phi at a point: per term the Horner multiply-adds and the exponential,
   at least 1. */
static ulong
pn_eval_units(const adf_rfun_t phi)
{
    ulong u = 0;
    slong i;

    for (i = 0; i < phi->len; i++)
        u += (ulong) acb_poly_length(phi->term[i].P) + 1;
    return FLINT_MAX(u, 1);
}

/* The units of adf_rfun_fourier: 2 L^2 - L + 1 per term of length L > 0 (src/rfun.c rf_fourier_work). */
static ulong
pn_fourier_units(const adf_rfun_t phi)
{
    ulong u = 0, L;
    slong i;

    for (i = 0; i < phi->len; i++)
    {
        L = (ulong) acb_poly_length(phi->term[i].P);
        u += L == 0 ? 0 : 2 * L * L - L + 1;
    }
    return u;
}

/* The preflight sizes (D1): L <= ADF_FFUN_ITEMS_MAX and L^2 <= ADF_TENSOR_WORK_MAX (the finite transform alone
   charges L^2), the rfun caps. */
static int
pn_sizes_ok(const adf_rfun_t phi, const adf_ffun_t f)
{
    slong i, s = 0;
    ulong L;

    if (f->M == 0 || f->D > ADF_FFUN_ITEMS_MAX / f->M)
        return 0;
    L = f->D * f->M;
    if (L > 1024)                                         /* 1024^2 = 2^20 */
        return 0;
    if (phi->len > ADF_RFUN_TERMS_MAX)
        return 0;
    for (i = 0; i < phi->len; i++)
    {
        s += acb_poly_length(phi->term[i].P);
        if (s > ADF_RFUN_COEFFS_MAX)
            return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ the tail kernel (Lemma 6) */

/* S = an upper bound of sum_(n > N) n^j exp(-alpha n^2 + beta n) for exact alpha > 0, beta >= 0 (analysis
   Lemma 6, docs/proofs/analysis.md:217-229): K runs from N + 1; while the upper bound of rho_j(alpha, beta, K)
   is not <= 1/2 the term K^j exp(-alpha K^2 + beta K) is added to the prefix and K is incremented; at the first K
   with rho certified <= 1/2 the geometric bound term/(1 - rho) is added (step 1 of the proof: every later ratio
   is at most rho). One work unit per K, the last included (P1 step 1: "charge prefix iterations too").
   Preflight: rho <= 1/2 needs alpha (2K + 1) >= beta + log 2 + j/K >= beta + log 2, so at least
   ((beta + log 2)/alpha - 1)/2 - (N + 1) iterations come first; more than the remaining budget is LIMIT at once.
   Returns ADF_OK or ADF_LIMIT. */
static int
pn_series(arb_t S, ulong j, const arb_t alpha, const arb_t beta, ulong N, ulong * w)
{
    arb_t prefix, rho, term, t, u;
    arf_t b;
    ulong K = N + 1;
    int st = ADF_OK;

    arb_init(prefix);
    arb_init(rho);
    arb_init(term);
    arb_init(t);
    arb_init(u);
    arf_init(b);
    arb_const_log2(t, PN_TP);
    arb_add(t, t, beta, PN_TP);
    arb_div(t, t, alpha, PN_TP);
    arb_sub_ui(t, t, 1, PN_TP);
    arb_mul_2exp_si(t, t, -1);
    arb_sub_ui(t, t, K, PN_TP);
    arb_get_lbound_arf(b, t, PN_TP);
    if (!arf_is_finite(b) || arf_cmp_ui(b, (ulong) ADF_TENSOR_WORK_MAX - *w) > 0)
        st = ADF_LIMIT;
    while (st == ADF_OK)
    {
        if (!pn_charge(w, 1))
        {
            st = ADF_LIMIT;
            break;
        }
        arb_set_ui(t, j);
        arb_div_ui(t, t, K, PN_TP);
        arb_mul_ui(u, alpha, 2 * K + 1, PN_TP);
        arb_sub(t, t, u, PN_TP);
        arb_add(t, t, beta, PN_TP);
        arb_exp(rho, t, PN_TP);                           /* rho_j(alpha, beta, K) */
        arb_set_ui(term, K);
        arb_pow_ui(term, term, j, PN_TP);
        arb_mul_ui(u, alpha, K, PN_TP);
        arb_mul_ui(u, u, K, PN_TP);
        arb_neg(u, u);
        arb_addmul_ui(u, beta, K, PN_TP);
        arb_exp(u, u, PN_TP);
        arb_mul(term, term, u, PN_TP);                    /* K^j exp(-alpha K^2 + beta K) */
        arb_get_ubound_arf(b, rho, PN_TP);
        if (arf_is_finite(b) && arf_cmp_2exp_si(b, -1) <= 0)
        {
            arb_sub_ui(u, rho, 1, PN_TP);
            arb_neg(u, u);
            arb_div(term, term, u, PN_TP);                /* term/(1 - rho), 1 - rho >= 1/2 */
            arb_add(S, prefix, term, PN_TP);
            break;
        }
        arb_add(prefix, prefix, term, PN_TP);
        K++;
    }
    arb_clear(prefix);
    arb_clear(rho);
    arb_clear(term);
    arb_clear(t);
    arb_clear(u);
    arf_clear(b);
    return st;
}

/* B = an upper bound of B_phi(a, h, N) = 2 exp(gamma) sum_j upper(|q_j|) S_j(alpha, beta, N) for one term t of
   phi (Lemma 6, analysis.md:230-246; P1 step 1): P(a + h n) = sum_j q_j n^j by a Taylor shift by a
   (acb_poly.rst:391-395) and q_j = Q_j h^j; alpha = pi lower(Re(A h^2)), beta = upper(|Re(h (B - 2 pi A a))|),
   gamma = upper(Re(C + B a - pi A a^2)), each at the precision pp and rounded outward to PN_TP bits. The series
   is decreasing in alpha and increasing in beta, so the outward bounds give an upper bound for every member of
   the parameter balls. A zero polynomial gives 0; a q_j with |q_j| = 0 is skipped. Charges the shift (len^2)
   and the scaling (len). ADF_OK, ADF_LIMIT, ADF_NOT_DETERMINED (alpha not certified positive, nonfinite). */
static int
pn_lattice(arb_t B, const adf_rterm_struct * t, const arb_t a, const arb_t h, ulong N, ulong * w, slong pp)
{
    slong len = acb_poly_length(t->P), j;
    acb_poly_t Q;
    acb_t z, y, ac;
    arb_t pi, alpha, beta, hp, S, qa, sum;
    arf_t b;
    mag_t m;
    int st = ADF_OK;

    if (len == 0)
    {
        arb_zero(B);
        return ADF_OK;
    }
    if (!pn_charge(w, (ulong) len * (ulong) len + (ulong) len))
        return ADF_LIMIT;
    acb_poly_init(Q);
    acb_init(z);
    acb_init(y);
    acb_init(ac);
    arb_init(pi);
    arb_init(alpha);
    arb_init(beta);
    arb_init(hp);
    arb_init(S);
    arb_init(qa);
    arb_init(sum);
    arf_init(b);
    mag_init(m);
    arb_const_pi(pi, pp);
    acb_set_arb(ac, a);
    acb_poly_taylor_shift(Q, t->P, ac, pp);              /* Q(x) = P(x + a) */
    acb_mul_arb(z, t->A, h, pp);
    acb_mul_arb(z, z, h, pp);
    arb_mul(alpha, pi, acb_realref(z), pp);              /* pi Re(A h^2) */
    arb_get_lbound_arf(b, alpha, PN_TP);
    if (!arf_is_finite(b) || arf_sgn(b) <= 0)
        st = ADF_NOT_DETERMINED;
    arb_set_arf(alpha, b);
    acb_mul_arb(z, t->A, a, pp);
    acb_mul_arb(z, z, pi, pp);
    acb_mul_2exp_si(z, z, 1);
    acb_sub(z, t->B, z, pp);
    acb_mul_arb(z, z, h, pp);                             /* h (B - 2 pi A a) */
    arb_abs(beta, acb_realref(z));
    arb_get_ubound_arf(b, beta, PN_TP);
    arb_set_arf(beta, b);
    acb_mul_arb(z, t->B, a, pp);
    acb_add(z, z, t->C, pp);
    acb_mul_arb(y, t->A, a, pp);
    acb_mul_arb(y, y, a, pp);
    acb_mul_arb(y, y, pi, pp);
    acb_sub(z, z, y, pp);                                 /* C' = C + B a - pi A a^2 */
    arb_get_ubound_arf(b, acb_realref(z), PN_TP);
    if (!arf_is_finite(b) || !arb_is_finite(beta))
        st = ADF_NOT_DETERMINED;
    arb_one(hp);
    for (j = 0; j < len && st == ADF_OK; j++)
    {
        acb_mul_arb(y, Q->coeffs + j, hp, pp);           /* q_j = Q_j h^j */
        arb_mul(hp, hp, h, pp);
        acb_get_mag(m, y);
        if (mag_is_zero(m))
            continue;
        if (mag_is_inf(m))
        {
            st = ADF_NOT_DETERMINED;
            break;
        }
        st = pn_series(S, (ulong) j, alpha, beta, N, w);
        if (st != ADF_OK)
            break;
        arf_set_mag(arb_midref(qa), m);
        mag_zero(arb_radref(qa));
        arb_addmul(sum, qa, S, PN_TP);
    }
    if (st == ADF_OK)
    {
        arb_set_arf(qa, b);
        arb_exp(qa, qa, PN_TP);                           /* exp(gamma) */
        arb_mul(B, sum, qa, PN_TP);
        arb_mul_2exp_si(B, B, 1);                         /* the two sides n > N and n < -N */
        if (!arb_is_finite(B))
            st = ADF_NOT_DETERMINED;
    }
    acb_poly_clear(Q);
    acb_clear(z);
    acb_clear(y);
    acb_clear(ac);
    arb_clear(pi);
    arb_clear(alpha);
    arb_clear(beta);
    arb_clear(hp);
    arb_clear(S);
    arb_clear(qa);
    arb_clear(sum);
    arf_clear(b);
    mag_clear(m);
    return st;
}

/* E = E_L(N) = sum_j upper(|f[j]|) sum_terms B_phi(j/D, M, N) (P1 step 3; analysis.md:279). */
static int
pn_tail_left(arb_t E, const adf_rfun_t phi, const adf_ffun_t f, ulong N, ulong * w, slong pp)
{
    ulong j, L = f->D * f->M;
    slong i;
    arb_t a, h, B, s, fa;
    fmpq_t q;
    mag_t m;
    int st = ADF_OK;

    arb_init(a);
    arb_init(h);
    arb_init(B);
    arb_init(s);
    arb_init(fa);
    fmpq_init(q);
    mag_init(m);
    arb_zero(E);
    arb_set_ui(h, f->M);
    for (j = 0; j < L && st == ADF_OK; j++)
    {
        acb_get_mag(m, f->f + j);
        if (mag_is_zero(m))
            continue;
        fmpq_set_si(q, (slong) j, f->D);
        arb_set_fmpq(a, q, pp);
        arb_zero(s);
        for (i = 0; i < phi->len && st == ADF_OK; i++)
        {
            st = pn_lattice(B, phi->term + i, a, h, N, w, pp);
            arb_add(s, s, B, PN_TP);
        }
        arf_set_mag(arb_midref(fa), m);
        mag_zero(arb_radref(fa));
        arb_addmul(E, fa, s, PN_TP);
    }
    arb_clear(a);
    arb_clear(h);
    arb_clear(B);
    arb_clear(s);
    arb_clear(fa);
    fmpq_clear(q);
    mag_clear(m);
    return st;
}

/* E = E_R(N) = max_k upper(|g[k]|) sum_terms B_phihat(0, 1/M, N) (P1 step 3; analysis.md:280); gmax is the max. */
static int
pn_tail_right(arb_t E, const adf_rfun_t hat, const mag_t gmax, ulong M, ulong N, ulong * w, slong pp)
{
    slong i;
    arb_t a, h, B;
    int st = ADF_OK;

    arb_init(a);
    arb_init(h);
    arb_init(B);
    arb_zero(E);
    if (!mag_is_zero(gmax))
    {
        arb_set_ui(h, M);
        arb_inv(h, h, pp);
        for (i = 0; i < hat->len && st == ADF_OK; i++)
        {
            st = pn_lattice(B, hat->term + i, a, h, N, w, pp);
            arb_add(E, E, B, PN_TP);
        }
        arf_set_mag(arb_midref(a), gmax);
        mag_zero(arb_radref(a));
        arb_mul(E, E, a, PN_TP);
    }
    arb_clear(a);
    arb_clear(h);
    arb_clear(B);
    return st;
}

/* ------------------------------------------------------------------ one attempt at the precision p */

/* The cutoff search of P1 step 3 for one side: N = 0, 1, 2, 4, ... until the tail E(N) has an upper bound
   <= 2^-(bits+3) = epsilon/8. Before each step (D1) the evaluation work at that N, npts (2N + 1) units, must fit
   the remaining budget (a later step needs more). left = 1: E_L with (phi, f); left = 0: E_R with (hat, gmax). */
static int
pn_search(ulong * Nout, arb_t E, int left, const adf_rfun_t fn, const adf_ffun_t f, const mag_t gmax, ulong npts,
          slong bits, ulong * w, slong pp)
{
    ulong N = 0;
    arf_t b;
    int st;

    arf_init(b);
    for (;;)
    {
        if (npts > 0 && (2 * N + 1) > ((ulong) ADF_TENSOR_WORK_MAX - *w) / npts)
        {
            st = ADF_LIMIT;
            break;
        }
        st = left ? pn_tail_left(E, fn, f, N, w, pp) : pn_tail_right(E, fn, gmax, f->M, N, w, pp);
        if (st != ADF_OK)
            break;
        arb_get_ubound_arf(b, E, PN_TP);
        if (arf_cmp_2exp_si(b, -bits - 3) <= 0)
        {
            *Nout = N;
            break;
        }
        N = N == 0 ? 1 : 2 * N;
    }
    arf_clear(b);
    return st;
}

/* One attempt at the working precision p: the two transforms, the two searches, the two sums with their tails
   added to both coordinate radii (P1 steps 1-4). left uses only (phi, f), right only (hat, g). OK (the width is
   checked by the caller), LIMIT, NOT_DETERMINED (a certificate or a value failed at this precision). */
static int
pn_attempt(acb_t lsum, acb_t rsum, ulong * nl, ulong * nr, const adf_rfun_t phi, const adf_ffun_t f, slong bits,
           slong p, ulong * w)
{
    adf_ffun_t g;
    adf_rfun_t hat;
    acb_t v, s;
    arb_t x, EL, ER;
    fmpq_t q;
    mag_t gmax, m;
    ulong L = f->D * f->M, j, nnz = 0, ul = pn_eval_units(phi), ur;
    slong n, pp = FLINT_MAX(p, PN_TP);
    int st = ADF_OK;

    adf_ffun_init(g);
    adf_rfun_init(hat);
    acb_init(v);
    acb_init(s);
    arb_init(x);
    arb_init(EL);
    arb_init(ER);
    fmpq_init(q);
    mag_init(gmax);
    mag_init(m);
    if (!pn_charge(w, L * L) || !pn_charge(w, pn_fourier_units(phi)))
        st = ADF_LIMIT;
    if (st == ADF_OK)
        st = adf_ffun_fourier(g, f, p);
    if (st == ADF_OK)
        st = adf_rfun_fourier(hat, phi, p);
    ur = pn_eval_units(hat);
    for (j = 0; j < L && st == ADF_OK; j++)
    {
        nnz += !acb_is_zero(f->f + j);
        acb_get_mag(m, g->f + j);
        mag_max(gmax, gmax, m);
    }
    if (st == ADF_OK)
        st = pn_search(nl, EL, 1, phi, f, gmax, nnz * ul, bits, w, pp);
    if (st == ADF_OK)
        st = pn_search(nr, ER, 0, hat, f, gmax, ur, bits, w, pp);
    /* left = sum_j f[j] sum_(|n| <= NL) phi(j/D + M n); an exact zero f[j] contributes the exact 0 */
    acb_zero(lsum);
    for (j = 0; j < L && st == ADF_OK; j++)
    {
        if (acb_is_zero(f->f + j))
            continue;
        acb_zero(s);
        for (n = -(slong) *nl; n <= (slong) *nl && st == ADF_OK; n++)
        {
            st = pn_charge(w, ul) ? ADF_OK : ADF_LIMIT;
            fmpq_set_si(q, (slong) j + (slong) L * n, f->D);
            arb_set_fmpq(x, q, p);
            if (st == ADF_OK && adf_rfun_eval(v, phi, x, p) != ADF_OK)
                st = ADF_NOT_DETERMINED;
            acb_add(s, s, v, p);
        }
        acb_addmul(lsum, f->f + j, s, p);
    }
    /* right = sum_(|n| <= NR) g[n mod L] phihat(n/M) */
    acb_zero(rsum);
    for (n = -(slong) *nr; st == ADF_OK && n <= (slong) *nr; n++)
    {
        st = pn_charge(w, ur) ? ADF_OK : ADF_LIMIT;
        fmpq_set_si(q, n, f->M);
        arb_set_fmpq(x, q, p);
        if (st == ADF_OK && adf_rfun_eval(v, hat, x, p) != ADF_OK)
            st = ADF_NOT_DETERMINED;
        acb_addmul(rsum, g->f + (ulong) (((n % (slong) L) + (slong) L) % (slong) L), v, p);
    }
    if (st == ADF_OK)
    {
        arb_get_mag(m, EL);
        acb_add_error_mag(lsum, m);                       /* P1 step 4: both coordinates */
        arb_get_mag(m, ER);
        acb_add_error_mag(rsum, m);
        if (!acb_is_finite(lsum) || !acb_is_finite(rsum))
            st = ADF_NOT_DETERMINED;
    }
    adf_ffun_clear(g);
    adf_rfun_clear(hat);
    acb_clear(v);
    acb_clear(s);
    arb_clear(x);
    arb_clear(EL);
    arb_clear(ER);
    fmpq_clear(q);
    mag_clear(gmax);
    mag_clear(m);
    return st;
}

/* d = the largest coordinate radius of the two balls (the diameter is 2 d). */
static void
pn_maxrad(mag_t d, const acb_t l, const acb_t r)
{
    mag_max(d, arb_radref(acb_realref(l)), arb_radref(acb_imagref(l)));
    mag_max(d, d, arb_radref(acb_realref(r)));
    mag_max(d, d, arb_radref(acb_imagref(r)));
}

/* P1 (docs/api-4.md:303-338; docs/api-4c.md "Slice 4g"): attempts at p = max(prec, 2), 2p, 4p, ... up to
   ADF_REAL_PREC_MAX while the width target or a certificate fails (step 4); a failed attempt has the diameter
   +infinity. A doubling from a precision >= PN_STALL_PREC that does not halve the largest diameter stops with
   NOT_DETERMINED (step 5: the input radii hold the width; docs/api-4c.md, decisions). The work counter runs over
   all attempts. */
int
adf_tensor_poisson(acb_t left, acb_t right, ulong * NL, ulong * NR, const adf_rfun_t phi, const adf_ffun_t f,
                   slong bits, slong prec)
{
    acb_t l, r;
    mag_t d, prev;
    ulong w = 0, nl = 0, nr = 0;
    slong p = FLINT_MAX(prec, 2), pprev = 0;
    int st;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    if (!pn_sizes_ok(phi, f))
        return ADF_LIMIT;
    if (bits < 0 || bits > PN_BITS_MAX)
        return ADF_DOMAIN;
    PN_INV(adf_rfun_is_canonical(phi), "phi", "adf_rfun");
    PN_INV(adf_ffun_is_canonical(f), "f", "adf_ffun");
    PN_INV(left != right, "right", "output distinct from left");
    PN_INV(NL != NULL && NR != NULL && NL != NR, "NR", "cutoff pointer distinct from NL");
    acb_init(l);
    acb_init(r);
    mag_init(d);
    mag_init(prev);
    for (;;)
    {
        st = pn_attempt(l, r, &nl, &nr, phi, f, bits, p, &w);
        if (st == ADF_LIMIT)
            break;
        if (st == ADF_OK)
        {
            pn_maxrad(d, l, r);
            if (mag_cmp_2exp_si(d, -bits - 1) <= 0)
                break;                                    /* every diameter 2 d <= 2^-bits */
        }
        else
            mag_inf(d);
        st = ADF_NOT_DETERMINED;
        if (p >= ADF_REAL_PREC_MAX)
            break;
        if (pprev >= PN_STALL_PREC)
        {
            mag_mul_2exp_si(prev, prev, -1);
            if (mag_is_inf(d) || mag_cmp(d, prev) > 0)
                break;                                    /* the doubling did not halve the diameter */
        }
        mag_set(prev, d);
        pprev = p;
        p = p > ADF_REAL_PREC_MAX / 2 ? ADF_REAL_PREC_MAX : 2 * p;
    }
    if (st == ADF_OK)
    {
        acb_swap(left, l);
        acb_swap(right, r);
        *NL = nl;
        *NR = nr;
    }
    acb_clear(l);
    acb_clear(r);
    mag_clear(d);
    mag_clear(prev);
    return st;
}
