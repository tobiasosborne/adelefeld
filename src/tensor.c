/* src/tensor.c: evaluation and additive integrals of test functions, slice 4f of docs/api-4.md
   (include/adelefeld/tensor.h; statements and proofs in docs/api-4c.md "Slice 4f").
   Sources, read before the code:
   - docs/api-4.md:244-296 (section 6, statement E1 steps 1-5, the integrals and norms), :20-41 (section 1:
     common contract, caps D1), :423-425 (section 9 item 6), :400-403 (D3);
   - docs/proofs/analysis.md:150-173 (Proposition 4: integral f = (1/M) sum f_j, integral |f|^2 =
     (1/M) sum |f_j|^2 = (1/D) sum |g_k|^2);
   - docs/conventions.md 5.9 (adf_sball: arch tags, local components a_p + p^e Z_p and exact local points),
     6.2 (Z_p volume 1, N Zhat volume 1/N), 5.3 (local backend; the canonical triple by CRT);
   - include/adelefeld/fball.h:156-162 (adf_fball_get_fmpz3, the canonical global triple; CRT for local);
     include/adelefeld/lball.h:72-80 (the lball predicate: centre p^v u, u a unit at p or 0);
   - refs/src/flint-3.0.1/acb.rst:271-273 (acb_union: a rectangle containing both), :275-277
     (acb_get_abs_ubound_arf); arb.rst:6-12 (a result contains the exact operation on every input point),
     :417-423 (arb_nonnegative_part), :425-443 (upper and lower bounds rounded outward); arf.rst:403-409
     (arf_get_mag, arf_get_mag_lower), :429-432 (arf_mag_add_ulp); fmpz.rst:868
     (fmpz_divisible), :1142 (fmpz_remove).
   Every result is built in temporaries and swapped into z after the last check. */
#include <adelefeld.h>
#include "invariants.h"

#ifdef ADF_CHECK_INVARIANTS
static ADF_INV_NOINLINE void tn_inv(int ok, const char * fn, const char * arg, const char * type)
{
    if (!ok) adf_inv_fail(fn, arg, type);
}
#define TN_INV_FFUN(x) tn_inv(adf_ffun_is_canonical(x), __func__, #x, "adf_ffun")
#define TN_INV_RFUN(x) tn_inv(adf_rfun_is_canonical(x), __func__, #x, "adf_rfun")
#define TN_INV_SBALL(x) tn_inv(adf_sball_is_canonical(x), __func__, #x, "adf_sball")
#else
#define TN_INV_FFUN(x) ((void) 0)
#define TN_INV_RFUN(x) ((void) 0)
#define TN_INV_SBALL(x) ((void) 0)
#endif

/* D1 bits of a raw exact integer: room for the products A D, H D, M d D, j d (each factor below 2^62). */
#define TN_BITS_MAX (ADF_FFUN_BITS_MAX - 128)

/* ------------------------------------------------------------------ preflight (D1) */

/* L = D M <= ADF_FFUN_ITEMS_MAX, decided without forming D M (divide first, as src/ffun.c ffun_shape). */
static int
tn_items_ok(const adf_ffun_t f)
{
    return f->M == 0 || f->D <= ADF_FFUN_ITEMS_MAX / f->M;
}

static int
tn_bits_ok(const fmpz_t a)
{
    return (slong) fmpz_bits(a) <= TN_BITS_MAX;
}

/* The raw fields of a finite ball; for a local value H = K bounds the CRT lift A0 < K (fball.h:156-162). */
static int
tn_fball_ok(const adf_fball_t x)
{
    return tn_bits_ok(x->A) && tn_bits_ok(x->H) && tn_bits_ok(x->d);
}

/* The rfun caps of D1 (rfun.h): terms and total coefficients, as src/rfun.c rf_input_ok. */
static int
tn_rfun_ok(const adf_rfun_t phi)
{
    slong i, s = 0;

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

/* ------------------------------------------------------------------ the rectangular hull */

/* The hull of E1 step 2 (api-4.md:255) is the smallest rectangle containing the selected balls. It is formed as
   the rectangle of acb_union (acb.rst:271-273), formed over all the balls at once: per part, each lower
   endpoint mid - rad rounded down and each upper endpoint mid + rad rounded up at prec, their minimum and
   maximum, then one ball containing that interval (tn_set_interval); a single ball is copied. Chained pairwise
   acb_union of the installed FLINT 3.0.1 widens at each step (the union of 2 +/- 1 and 5 is
   (3 - 2^-30) +/- (2 + 2^-27), not 3 +/- 2: docs/api-4c.md, findings). */
typedef struct
{
    arf_struct lo[2], hi[2];
    int empty;
    slong n;                                              /* balls joined */
    const acb_struct * one;                               /* the first ball joined (NULL: the exact 0) */
} tn_box;

static void
tn_box_init(tn_box * b)
{
    int k;

    for (k = 0; k < 2; k++)
    {
        arf_init(b->lo + k);
        arf_init(b->hi + k);
    }
    b->empty = 1;
    b->n = 0;
    b->one = NULL;
}

static void
tn_box_clear(tn_box * b)
{
    int k;

    for (k = 0; k < 2; k++)
    {
        arf_clear(b->lo + k);
        arf_clear(b->hi + k);
    }
}

/* Joins the ball v (the exact 0 if v is NULL). */
static void
tn_box_join(tn_box * b, const acb_t v, slong p)
{
    arf_t l, h, r;
    int k;

    arf_init(l);
    arf_init(h);
    arf_init(r);
    for (k = 0; k < 2; k++)
    {
        if (v == NULL)
        {
            arf_zero(l);
            arf_zero(h);
        }
        else
        {
            const arb_struct * x = k ? acb_imagref(v) : acb_realref(v);

            arf_set_mag(r, arb_radref(x));
            arf_sub(l, arb_midref(x), r, p, ARF_RND_FLOOR);
            arf_add(h, arb_midref(x), r, p, ARF_RND_CEIL);
        }
        if (b->empty || arf_cmp(l, b->lo + k) < 0)
            arf_set(b->lo + k, l);
        if (b->empty || arf_cmp(h, b->hi + k) > 0)
            arf_set(b->hi + k, h);
    }
    if (b->n++ == 0)
        b->one = v;
    b->empty = 0;
    arf_clear(l);
    arf_clear(h);
    arf_clear(r);
}

/* x = a ball containing [a, b], a <= b: mid = a + b rounded at p, rad = b - a rounded up to a mag, plus the
   ulp of mid when the sum was inexact, both halved. The radius is exact when b - a fits a mag (30 bits):
   arb_set_interval_arf (arb.rst:481-486) of the installed FLINT 3.0.1 adds one mag ulp even then ([1, 5] gives
   3 +/- (2 + 2^-28.4)). Enclosure: |mid/2 - (a + b)/2| <= ulp/2 and rad/2 >= (b - a)/2 + ulp/2. */
static void
tn_set_interval(arb_t x, const arf_t a, const arf_t b, slong p)
{
    arf_t t, u;
    mag_t m;
    int inexact;

    arf_init(t);
    arf_init(u);
    mag_init(m);
    inexact = arf_sub(t, b, a, MAG_BITS, ARF_RND_UP);
    arf_get_mag_lower(m, t);
    arf_set_mag(u, m);
    if (inexact || !arf_equal(u, t))
        arf_get_mag(m, t);                                /* an upper bound (arf.rst:403-405) */
    inexact = arf_add(arb_midref(x), a, b, p, ARF_RND_DOWN);
    if (inexact)
        arf_mag_add_ulp(m, m, arb_midref(x), p);          /* arf.rst:429-432 */
    mag_swap(arb_radref(x), m);
    arb_mul_2exp_si(x, x, -1);
    arf_clear(t);
    arf_clear(u);
    mag_clear(m);
}

/* h = the hull; an empty hull is the exact 0 (E1 step 2); a single ball is copied exactly. */
static void
tn_box_get(acb_t h, const tn_box * b, slong p)
{
    if (b->n <= 1)
    {
        if (b->one == NULL)
            acb_zero(h);
        else
            acb_set(h, b->one);
        return;
    }
    tn_set_interval(acb_realref(h), b->lo, b->hi, p);
    tn_set_interval(acb_imagref(h), b->lo + 1, b->hi + 1, p);
}

/* ------------------------------------------------------------------ E1 steps 1-3 */

/* h = the hull of the values of f on the finite ball x (docs/api-4c.md, statement 1).
   Step 1: with the canonical triple (A, H, d), D (A + H Zhat)/d meets j + M D Zhat ... cleared of
   denominators, (A D + H D Zhat) meets (j d + M d D Zhat) iff g = gcd(H D, M d D) divides A D - j d
   (two integer cosets a + I, b + J meet iff a - b lies in I + J = gcd Z). H = 0: g = M d D (fmpz_gcd(0, n) = |n|).
   Step 2: x lies in (1/D) Zhat iff d | D A and d | D H; otherwise the exact 0 joins the hull.
   Step 3: the hull (tn_box) of the selected f[j] (and 0); nothing selected gives the exact 0. */
static void
tn_hull(acb_t h, const adf_ffun_t f, const adf_fball_t x, slong p)
{
    fmpz_t A, H, d, AD, g, t;
    ulong j, L = f->D * f->M;
    tn_box b;

    tn_box_init(&b);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(AD);
    fmpz_init(g);
    fmpz_init(t);
    adf_fball_get_fmpz3(A, H, d, x);
    fmpz_mul_ui(AD, A, f->D);
    fmpz_mul_ui(g, H, f->D);
    fmpz_mul_ui(t, d, f->M);
    fmpz_mul_ui(t, t, f->D);
    fmpz_gcd(g, g, t);                                    /* gcd(H D, M d D) > 0 */
    fmpz_mul_ui(t, H, f->D);
    if (!fmpz_divisible(AD, d) || !fmpz_divisible(t, d))
        tn_box_join(&b, NULL, p);                         /* outside (1/D) Zhat: the value 0 */
    for (j = 0; j < L; j++)
    {
        fmpz_mul_ui(t, d, j);
        fmpz_sub(t, AD, t);
        if (fmpz_divisible(t, g))
            tn_box_join(&b, f->f + j, p);
    }
    tn_box_get(h, &b, p);
    tn_box_clear(&b);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(AD);
    fmpz_clear(g);
    fmpz_clear(t);
}

int
adf_ffun_eval(acb_t z, const adf_ffun_t f, const adf_fball_t x, slong prec)
{
    acb_t h;
    int st;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    if (!tn_items_ok(f) || !tn_fball_ok(x))
        return ADF_LIMIT;
    TN_INV_FFUN(f);
    ADF_INV_FBALL(x);
    acb_init(h);
    tn_hull(h, f, x, FLINT_MAX(prec, 2));
    st = acb_is_finite(h) ? ADF_OK : ADF_NOT_DETERMINED;
    if (st == ADF_OK)
        acb_swap(z, h);
    acb_clear(h);
    return st;
}

/* phi(x_inf) f(x_f): every point value lies in the product of the two enclosures (arb.rst:6-12). */
int
adf_tensor_eval(acb_t z, const adf_rfun_t phi, const adf_ffun_t f, const adf_adele_t x, slong prec)
{
    acb_t h, r;
    slong p = FLINT_MAX(prec, 2);
    int st;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    if (!tn_items_ok(f) || !tn_fball_ok(&x->fin) || !tn_rfun_ok(phi))
        return ADF_LIMIT;
    TN_INV_RFUN(phi);
    TN_INV_FFUN(f);
    ADF_INV_ADELE(x);
    if (!arb_is_finite(x->inf))
        return ADF_DOMAIN;
    acb_init(h);
    acb_init(r);
    tn_hull(h, f, &x->fin, p);
    st = adf_rfun_eval(r, phi, x->inf, prec);
    if (st == ADF_OK)
    {
        acb_mul(r, r, h, p);
        if (!acb_is_finite(r))
            st = ADF_NOT_DETERMINED;
    }
    if (st == ADF_OK)
        acb_swap(z, r);
    acb_clear(h);
    acb_clear(r);
    return st;
}

/* ------------------------------------------------------------------ E1 steps 4-5 (D3) */

/* v_p(n) of a word n > 0. */
static slong
tn_val_ui(ulong n, ulong p)
{
    slong v = 0;

    while (n % p == 0)
    {
        n /= p;
        v++;
    }
    return v;
}

/* E1 step 4 at one supplied component l at the prime p: 1 iff j/D + M Zhat meets the component, that is iff
   v_p(j/D - c) >= min(e, v_p(M)) for a ball c + p^e Z_p and >= v_p(M) for an exact point c (docs/api-4c.md,
   statement 3). The centre is c = p^v u with u = 0 or v_p(u) = 0 (lball.h:72-80), so v_p(c) = v without forming
   p^v; v_p(j/D - c) = min(v_p(j/D), v) when the two differ, and only when they are equal (then |v| <= 63) the
   difference is formed exactly. j = 0 or c = 0: the valuation of the other term (+infinity for both). */
static int
tn_keep(ulong j, const adf_ffun_t f, const adf_lball_struct * l, ulong p, fmpq_t q, fmpz_t t)
{
    slong m = f->M % p ? 0 : tn_val_ui(f->M, p), thr, w, val;

    thr = (l->exact || l->N > m) ? m : l->N;
    if (j == 0 && fmpq_is_zero(l->u))
        return 1;
    if (j == 0)
        return l->v >= thr;
    w = tn_val_ui(j, p) - tn_val_ui(f->D, p);
    if (fmpq_is_zero(l->u))
        return w >= thr;
    if (w != l->v)
        return FLINT_MIN(w, l->v) >= thr;
    /* q = j/D - p^v u, with |v| = |w| <= 63 */
    fmpz_set_ui(t, p);
    fmpz_pow_ui(t, t, (ulong) (l->v >= 0 ? l->v : -l->v));
    fmpq_set(q, l->u);
    if (l->v >= 0)
        fmpz_mul(fmpq_numref(q), fmpq_numref(q), t);
    else
        fmpz_mul(fmpq_denref(q), fmpq_denref(q), t);
    fmpq_canonicalise(q);                                 /* q = c */
    fmpz_set_ui(t, f->D);
    fmpq_mul_fmpz(q, q, t);
    fmpz_set_ui(t, j);
    fmpq_sub_fmpz(q, q, t);                               /* q = D c - j = -D (j/D - c) */
    if (fmpq_is_zero(q))
        return 1;
    fmpz_set_ui(t, p);
    {
        fmpz_t r;
        fmpz_init(r);
        val = fmpz_remove(r, fmpq_numref(q), t) - fmpz_remove(r, fmpq_denref(q), t) - tn_val_ui(f->D, p);
        fmpz_clear(r);
    }
    return val >= thr;
}

/* h = the hull of the f[j] kept at every supplied prime, and of the exact 0 (always: docs/api-4c.md statement 3,
   an absent prime can put a completion outside (1/D) Zhat). */
static void
tn_partial_hull(acb_t h, const adf_ffun_t f, const adf_sball_t x, slong p)
{
    ulong j, L = f->D * f->M;
    slong i;
    fmpq_t q;
    fmpz_t t;

    tn_box b;

    tn_box_init(&b);
    tn_box_join(&b, NULL, p);
    fmpq_init(q);
    fmpz_init(t);
    for (j = 0; j < L; j++)
    {
        int keep = 1;

        for (i = 0; i < x->len && keep; i++)
            keep = tn_keep(j, f, x->loc + i, adf_place_prime_get(adf_lball_place(x->loc + i)), q, t);
        if (keep)
            tn_box_join(&b, f->f + j, p);
    }
    tn_box_get(h, &b, p);
    tn_box_clear(&b);
    fmpq_clear(q);
    fmpz_clear(t);
}

/* E1 step 5, arch NONE (docs/api-4c.md statement 4): r = [-R, R] + i [-R, R] with R the sum over the terms with
   P != 0 of exp(gamma + beta^2/(2 alpha)) sum_j upper(|p_j|) T_j, alpha = pi lower(Re A), beta = upper(|Re B|),
   gamma = upper(Re C), T_0 = 1, T_j = (j/(alpha e))^(j/2). For real t and every member,
   |phi(t)| <= sum |P(t)| exp(-alpha t^2 + beta |t| + gamma) and -alpha t^2 + beta |t| <= -alpha t^2/2 +
   beta^2/(2 alpha); max |t|^j exp(-alpha t^2/2) = T_j. R decreases in alpha and increases in beta, gamma and
   |p_j|, so the bounds of the members may replace the members. Returns 0 if alpha is not certified positive or
   R is not finite. */
static int
tn_bound(acb_t r, const adf_rfun_t phi, slong p)
{
    arb_t R, alpha, s, e, t, u;
    arf_t b;
    slong i, j;
    int ok = 1;

    arb_init(R);
    arb_init(alpha);
    arb_init(s);
    arb_init(e);
    arb_init(t);
    arb_init(u);
    arf_init(b);
    for (i = 0; i < phi->len && ok; i++)
    {
        const adf_rterm_struct * a = phi->term + i;
        slong n = acb_poly_length(a->P);

        if (n == 0)
            continue;
        arb_get_lbound_arf(b, acb_realref(a->A), p);
        arb_const_pi(alpha, p);
        arb_mul_arf(alpha, alpha, b, p);                  /* alpha = pi lower(Re A) */
        if (!arb_is_positive(alpha))
        {
            ok = 0;
            break;
        }
        arb_get_abs_ubound_arf(b, acb_realref(a->B), p);
        arb_set_arf(e, b);
        arb_sqr(e, e, p);
        arb_div(e, e, alpha, p);
        arb_mul_2exp_si(e, e, -1);                        /* beta^2/(2 alpha) */
        arb_get_ubound_arf(b, acb_realref(a->C), p);
        arb_add_arf(e, e, b, p);
        arb_exp(e, e, p);                                 /* exp(gamma + beta^2/(2 alpha)) */
        acb_get_abs_ubound_arf(b, a->P->coeffs, p);
        arb_set_arf(s, b);                                /* T_0 = 1 */
        for (j = 1; j < n; j++)
        {
            acb_get_abs_ubound_arf(b, a->P->coeffs + j, p);
            if (arf_is_zero(b))
                continue;
            arb_set_si(t, j);
            arb_div(t, t, alpha, p);
            arb_log(t, t, p);
            arb_sub_ui(t, t, 1, p);
            arb_mul_si(t, t, j, p);
            arb_mul_2exp_si(t, t, -1);
            arb_exp(t, t, p);                             /* T_j = exp((j/2) (log(j/alpha) - 1)) */
            arb_set_arf(u, b);
            arb_addmul(s, u, t, p);
        }
        arb_addmul(R, e, s, p);
    }
    if (ok)
    {
        arb_get_ubound_arf(b, R, p);
        ok = arf_is_finite(b);
    }
    if (ok)
    {
        acb_zero(r);
        arb_add_error_arf(acb_realref(r), b);
        arb_add_error_arf(acb_imagref(r), b);
    }
    arb_clear(R);
    arb_clear(alpha);
    arb_clear(s);
    arb_clear(e);
    arb_clear(t);
    arb_clear(u);
    arf_clear(b);
    return ok;
}

/* One work unit per index and supplied prime, at least one per index (D1; docs/api-4c.md, decisions). */
static int
tn_sball_ok(const adf_ffun_t f, const adf_sball_t x)
{
    ulong L = f->D * f->M;
    slong i;

    if (x->len > 1 && L > (ulong) ADF_TENSOR_WORK_MAX / (ulong) x->len)
        return 0;
    for (i = 0; i < x->len; i++)
        if (!tn_bits_ok(fmpq_numref(x->loc[i].u)) || !tn_bits_ok(fmpq_denref(x->loc[i].u)))
            return 0;
    return 1;
}

int
adf_tensor_eval_sball(acb_t z, const adf_rfun_t phi, const adf_ffun_t f, const adf_sball_t x, slong prec)
{
    acb_t h, r;
    slong p = FLINT_MAX(prec, 2);
    int st;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    if (!tn_items_ok(f) || !tn_rfun_ok(phi) || !tn_sball_ok(f, x))
        return ADF_LIMIT;
    TN_INV_RFUN(phi);
    TN_INV_FFUN(f);
    TN_INV_SBALL(x);
    if (x->arch == ADF_ARCH_COMPLEX)
        return ADF_DOMAIN;                                /* no complex-to-real coercion (E1 step 5) */
    acb_init(h);
    acb_init(r);
    tn_partial_hull(h, f, x, p);
    if (x->arch == ADF_ARCH_REAL)
        st = adf_rfun_eval(r, phi, acb_realref(x->inf), prec);
    else
        st = tn_bound(r, phi, p) ? ADF_OK : ADF_NOT_DETERMINED;
    if (st == ADF_OK)
    {
        acb_mul(r, r, h, p);
        if (!acb_is_finite(r))
            st = ADF_NOT_DETERMINED;
    }
    if (st == ADF_OK)
        acb_swap(z, r);
    acb_clear(h);
    acb_clear(r);
    return st;
}

/* ------------------------------------------------------------------ integrals and norms (analysis P4) */

/* (1/M) sum f[j] (analysis P4:157; docs/api-4c.md statement 5). */
static void
tn_integral(acb_t s, const adf_ffun_t f, slong p)
{
    ulong j, L = f->D * f->M;

    acb_zero(s);
    for (j = 0; j < L; j++)
        acb_add(s, s, f->f + j, p);
    acb_div_ui(s, s, f->M, p);
}

/* (1/M) sum |f[j]|^2 = (1/M) sum (Re^2 + Im^2) (analysis P4:158), not yet clipped. */
static void
tn_norm2(arb_t s, const adf_ffun_t f, slong p)
{
    ulong j, L = f->D * f->M;
    arb_t t;

    arb_init(t);
    arb_zero(s);
    for (j = 0; j < L; j++)
    {
        arb_sqr(t, acb_realref(f->f + j), p);
        arb_add(s, s, t, p);
        arb_sqr(t, acb_imagref(f->f + j), p);
        arb_add(s, s, t, p);
    }
    arb_div_ui(s, s, f->M, p);
    arb_clear(t);
}

/* The true value is nonnegative and lies in s; a provably negative s is an internal defect (api-4.md:269). */
static void
tn_clip(arb_t s)
{
    if (arb_is_negative(s))
        flint_abort();
    arb_nonnegative_part(s, s);
}

int
adf_ffun_integral(acb_t z, const adf_ffun_t f, slong prec)
{
    acb_t s;
    int st;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    if (!tn_items_ok(f))
        return ADF_LIMIT;
    TN_INV_FFUN(f);
    acb_init(s);
    tn_integral(s, f, FLINT_MAX(prec, 2));
    st = acb_is_finite(s) ? ADF_OK : ADF_NOT_DETERMINED;
    if (st == ADF_OK)
        acb_swap(z, s);
    acb_clear(s);
    return st;
}

int
adf_ffun_norm2(arb_t z, const adf_ffun_t f, slong prec)
{
    arb_t s;
    int st;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    if (!tn_items_ok(f))
        return ADF_LIMIT;
    TN_INV_FFUN(f);
    arb_init(s);
    tn_norm2(s, f, FLINT_MAX(prec, 2));
    st = arb_is_finite(s) ? ADF_OK : ADF_NOT_DETERMINED;
    if (st == ADF_OK)
    {
        tn_clip(s);
        arb_swap(z, s);
    }
    arb_clear(s);
    return st;
}

/* Fubini on the product measure: the integral of phi (x) f is the product of the integrals. */
int
adf_tensor_integral(acb_t z, const adf_rfun_t phi, const adf_ffun_t f, slong prec)
{
    acb_t s, r;
    slong p = FLINT_MAX(prec, 2);
    int st;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    if (!tn_items_ok(f) || !tn_rfun_ok(phi))
        return ADF_LIMIT;
    TN_INV_RFUN(phi);
    TN_INV_FFUN(f);
    acb_init(s);
    acb_init(r);
    tn_integral(s, f, p);
    st = adf_rfun_integral(r, phi, prec);
    if (st == ADF_OK)
    {
        acb_mul(r, r, s, p);
        if (!acb_is_finite(r))
            st = ADF_NOT_DETERMINED;
    }
    if (st == ADF_OK)
        acb_swap(z, r);
    acb_clear(s);
    acb_clear(r);
    return st;
}

/* |phi (x) f|^2 = |phi|^2 |f|^2 pointwise; Fubini gives the product of the two norms, clipped again. */
int
adf_tensor_norm2(arb_t z, const adf_rfun_t phi, const adf_ffun_t f, slong prec)
{
    arb_t s, r;
    slong p = FLINT_MAX(prec, 2);
    int st;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    if (!tn_items_ok(f) || !tn_rfun_ok(phi))
        return ADF_LIMIT;
    TN_INV_RFUN(phi);
    TN_INV_FFUN(f);
    arb_init(s);
    arb_init(r);
    tn_norm2(s, f, p);
    st = adf_rfun_norm2(r, phi, prec);
    if (st == ADF_OK)
    {
        tn_clip(s);
        arb_mul(r, r, s, p);
        if (!arb_is_finite(r))
            st = ADF_NOT_DETERMINED;
    }
    if (st == ADF_OK)
    {
        tn_clip(r);
        arb_swap(z, r);
    }
    arb_clear(s);
    arb_clear(r);
    return st;
}
