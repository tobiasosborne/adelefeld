/* src/rfun.c: real test functions, slice 4d of docs/api-4.md (include/adelefeld/rfun.h, docs/api-4b.md).
   Sources, read before the code:
   - docs/api-4.md section 1 (layout, common contract, caps D1), 5 (closure formulas, sum and product
     order, exact trailing zeros, rational q and h as exact integer and exact denominator, idele
     dilation by interval squaring, NOT_DETERMINED when Re(A') > 0 is lost), 6 (real evaluation);
   - docs/proofs/analysis.md:174-212 (Proposition 5): translation P(x-a), A, B+2 pi A a, C-Ba-pi A a^2;
     dilation P(hx), A h^2, B h, C; products multiply P and add A, B, C;
   - docs/conventions.md 5.12 (predicate: normalized P with finite coefficients, no exact-zero last
     coefficient, finite A, B, C, arb_is_positive(Re A)), 3.2 row "Integrals, Poisson summation";
   - refs/src/flint-3.0.1/acb_poly.rst:42-54 (fit_length, _set_length, _normalise strips coefficients
     identical to zero), :180 (acb_poly_equal), :344-347 (acb_poly_mul), :446-456 (evaluation);
     acb.rst:216-245 (acb_is_zero, acb_is_finite, acb_equal), :417 (acb_conj), :457-459 (acb_mul_fmpz,
     acb_mul_arb), :517 (acb_div_fmpz), :658-661 (acb_exp); arb.rst:6-12 (a result contains the exact
     operation on every input point), :425-433 (absolute bounds rounded outward), :481-485
     (arb_set_interval_arf), :639 (arb_is_positive); arf.rst:590 (arf_mul with a rounding mode).
   Every result is built in a fresh term array and committed only after the last check. */
#include <adelefeld.h>
#include "invariants.h"
#include <string.h>

#ifdef ADF_CHECK_INVARIANTS
#define ADF_INV_RFUN(x) \
    do { if (!adf_rfun_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_rfun"); } while (0)
#define ADF_INV_IDELE_R(x) \
    do { if (!adf_idele_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idele"); } while (0)
#else
#define ADF_INV_RFUN(x) ((void) 0)
#define ADF_INV_IDELE_R(x) ((void) 0)
#endif

/* ------------------------------------------------------------------ term arrays */

static void
rf_term_init(adf_rterm_struct * t)
{
    acb_poly_init(t->P);
    acb_init(t->A);
    acb_init(t->B);
    acb_init(t->C);
}

static void
rf_term_clear(adf_rterm_struct * t)
{
    acb_poly_clear(t->P);
    acb_clear(t->A);
    acb_clear(t->B);
    acb_clear(t->C);
}

/* n initialized terms in owned storage (flint_malloc), NULL for n = 0. */
static adf_rterm_struct *
rf_alloc(slong n)
{
    adf_rterm_struct * t;
    slong i;

    if (n <= 0)
        return NULL;
    t = (adf_rterm_struct *) flint_malloc((size_t) n * sizeof(adf_rterm_struct));
    for (i = 0; i < n; i++)
        rf_term_init(t + i);
    return t;
}

static void
rf_free(adf_rterm_struct * t, slong n)
{
    slong i;

    for (i = 0; i < n; i++)
        rf_term_clear(t + i);
    flint_free(t);
}

/* y takes the n terms t; its old terms are released. */
static void
rf_commit(adf_rfun_t y, adf_rterm_struct * t, slong n)
{
    rf_free(y->term, y->len);
    y->term = t;
    y->len = n;
}

static void
rf_term_set(adf_rterm_struct * y, const adf_rterm_struct * x)
{
    acb_poly_set(y->P, x->P);
    acb_set(y->A, x->A);
    acb_set(y->B, x->B);
    acb_set(y->C, x->C);
}

/* The predicate of one term (conventions 5.12). */
static int
rf_term_ok(const adf_rterm_struct * t)
{
    slong j, n = t->P->length;

    if (n < 0 || (n > 0 && t->P->coeffs == NULL))
        return 0;
    for (j = 0; j < n; j++)
        if (!acb_is_finite(t->P->coeffs + j))
            return 0;
    if (n > 0 && acb_is_zero(t->P->coeffs + n - 1))
        return 0;
    return acb_is_finite(t->A) && acb_is_finite(t->B) && acb_is_finite(t->C)
           && arb_is_positive(acb_realref(t->A));
}

/* The result checks of a term: finite (else NOT_DETERMINED, api-4.md section 1) and Re(A) > 0 certified
   (api-4.md section 5, "If rounding loses Re(A') > 0, return NOT_DETERMINED"). The polynomial is
   normalized by construction (acb_poly_mul and _acb_poly_normalise). */
static int
rf_result_ok(const adf_rterm_struct * t)
{
    return rf_term_ok(t);
}

/* The total number of coefficients of x, saturated above ADF_RFUN_COEFFS_MAX. */
static slong
rf_coeffs(const adf_rfun_t x)
{
    slong i, s = 0;

    for (i = 0; i < x->len; i++)
    {
        s += acb_poly_length(x->term[i].P);
        if (s > ADF_RFUN_COEFFS_MAX)
            return ADF_RFUN_COEFFS_MAX + 1;
    }
    return s;
}

/* D1: an input within the caps of terms and coefficients. */
static int
rf_input_ok(const adf_rfun_t x)
{
    return x->len <= ADF_RFUN_TERMS_MAX && rf_coeffs(x) <= ADF_RFUN_COEFFS_MAX;
}

/* D1: an exact rational whose numerator and denominator can be squared within ADF_RFUN_BITS_MAX bits. */
static int
rf_rat_ok(const fmpq_t q)
{
    return 2 * (slong) fmpz_bits(fmpq_numref(q)) <= ADF_RFUN_BITS_MAX
           && 2 * (slong) fmpz_bits(fmpq_denref(q)) <= ADF_RFUN_BITS_MAX;
}

/* ------------------------------------------------------------------ lifecycle */

void
adf_rfun_init(adf_rfun_t x)
{
    x->len = 0;
    x->term = NULL;
}

void
adf_rfun_clear(adf_rfun_t x)
{
    rf_free(x->term, x->len);
    x->term = NULL;
    x->len = 0;
}

void
adf_rfun_set(adf_rfun_t y, const adf_rfun_t x)
{
    adf_rterm_struct * t;
    slong i;

    if (y == x)
        return;
    t = rf_alloc(x->len);
    for (i = 0; i < x->len; i++)
        rf_term_set(t + i, x->term + i);
    rf_commit(y, t, x->len);
}

void
adf_rfun_swap(adf_rfun_t x, adf_rfun_t y)
{
    adf_rfun_struct t = *x;

    *x = *y;
    *y = t;
}

int
adf_rfun_is_canonical(const adf_rfun_t x)
{
    slong i;

    if (x->len < 0 || (x->len > 0 && x->term == NULL))
        return 0;
    for (i = 0; i < x->len; i++)
        if (!rf_term_ok(x->term + i))
            return 0;
    return 1;
}

int
adf_rfun_identical(const adf_rfun_t x, const adf_rfun_t y)
{
    slong i;

    if (x->len != y->len)
        return 0;
    for (i = 0; i < x->len; i++)
    {
        const adf_rterm_struct * a = x->term + i, * b = y->term + i;
        if (!acb_poly_equal(a->P, b->P) || !acb_equal(a->A, b->A) || !acb_equal(a->B, b->B)
            || !acb_equal(a->C, b->C))
            return 0;
    }
    return 1;
}

/* api-4.md section 1: LIMIT first (the count before any term is read, then the coefficients),
   then DOMAIN; a deep copy, no rounding. The copy is made before the old terms are released. */
int
adf_rfun_set_terms(adf_rfun_t y, const adf_rterm_struct * terms, slong n)
{
    adf_rterm_struct * t;
    slong i, s = 0;

    if (n > ADF_RFUN_TERMS_MAX)
        return ADF_LIMIT;
    if (n < 0 || (n > 0 && terms == NULL))
        return ADF_DOMAIN;
    for (i = 0; i < n; i++)
    {
        s += FLINT_MAX(terms[i].P->length, 0);
        if (s > ADF_RFUN_COEFFS_MAX)
            return ADF_LIMIT;
    }
    for (i = 0; i < n; i++)
        if (!rf_term_ok(terms + i))
            return ADF_DOMAIN;
    t = rf_alloc(n);
    for (i = 0; i < n; i++)
        rf_term_set(t + i, terms + i);
    rf_commit(y, t, n);
    return ADF_OK;
}

/* ------------------------------------------------------------------ algebra (api-4.md section 5) */

/* Sum: the terms of x, then those of y, copied exactly (no rounding: prec is only checked). */
int
adf_rfun_add(adf_rfun_t z, const adf_rfun_t x, const adf_rfun_t y, slong prec)
{
    adf_rterm_struct * t;
    slong i, n;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(x);
    ADF_INV_RFUN(y);
    if (!rf_input_ok(x) || !rf_input_ok(y) || x->len + y->len > ADF_RFUN_TERMS_MAX
        || rf_coeffs(x) + rf_coeffs(y) > ADF_RFUN_COEFFS_MAX)
        return ADF_LIMIT;
    n = x->len + y->len;
    t = rf_alloc(n);
    for (i = 0; i < x->len; i++)
        rf_term_set(t + i, x->term + i);
    for (i = 0; i < y->len; i++)
        rf_term_set(t + x->len + i, y->term + i);
    rf_commit(z, t, n);
    return ADF_OK;
}

/* Product (analysis P5): the pairs (i, j) in lexicographic order, P_i Q_j, A_i + A'_j, B_i + B'_j,
   C_i + C'_j. Caps: len(x) len(y) terms; sum over pairs of len P + len Q - 1 coefficients (0 for a zero
   factor); work sum over pairs of len P len Q = (total of x) (total of y). */
int
adf_rfun_mul(adf_rfun_t z, const adf_rfun_t x, const adf_rfun_t y, slong prec)
{
    adf_rterm_struct * t;
    slong i, j, k, n, p = FLINT_MAX(prec, 2), sx, sy, nx = 0, ny = 0;
    ulong coeffs;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(x);
    ADF_INV_RFUN(y);
    if (!rf_input_ok(x) || !rf_input_ok(y))
        return ADF_LIMIT;
    if (x->len > 0 && y->len > ADF_RFUN_TERMS_MAX / x->len)
        return ADF_LIMIT;
    sx = rf_coeffs(x);
    sy = rf_coeffs(y);
    for (i = 0; i < x->len; i++)
        nx += acb_poly_length(x->term[i].P) > 0;
    for (j = 0; j < y->len; j++)
        ny += acb_poly_length(y->term[j].P) > 0;
    coeffs = (ulong) nx * (ulong) sy + (ulong) ny * (ulong) sx - (ulong) nx * (ulong) ny;
    if (coeffs > (ulong) ADF_RFUN_COEFFS_MAX || (ulong) sx * (ulong) sy > (ulong) ADF_RFUN_WORK_MAX)
        return ADF_LIMIT;
    n = x->len * y->len;
    t = rf_alloc(n);
    for (i = 0, k = 0; i < x->len; i++)
        for (j = 0; j < y->len; j++, k++)
        {
            const adf_rterm_struct * a = x->term + i, * b = y->term + j;
            acb_poly_mul(t[k].P, a->P, b->P, p);
            acb_add(t[k].A, a->A, b->A, p);
            acb_add(t[k].B, a->B, b->B, p);
            acb_add(t[k].C, a->C, b->C, p);
            if (!rf_result_ok(t + k))
            {
                rf_free(t, n);
                return ADF_NOT_DETERMINED;
            }
        }
    rf_commit(z, t, n);
    return ADF_OK;
}

/* Translation by q = num/den (analysis P5): P(x - q), A, B + 2 pi A q, C - B q - pi A q^2. Every
   product with q is a multiplication by num followed by a division by den (api-4.md section 5); the
   shift P(x - q) is the Horner scheme a_j += c a_(j+1) for i = 0..n-2, j = n-2..i with c = -q, whose
   multiply-adds are n (n - 1)/2 per term (the work charged). A is copied exactly. */
int
adf_rfun_translate_rat(adf_rfun_t y, const adf_rfun_t x, const adf_rat_t q, slong prec)
{
    adf_rterm_struct * t;
    slong i, j, k, L, p = FLINT_MAX(prec, 2);
    ulong work = 0;
    const fmpz * num = fmpq_numref(q->q), * den = fmpq_denref(q->q);
    fmpz_t mnum, twonum, num2, den2;
    acb_t pA, u;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(x);
    ADF_INV_RAT(q);
    if (!rf_input_ok(x) || !rf_rat_ok(q->q))
        return ADF_LIMIT;
    for (i = 0; i < x->len; i++)
    {
        ulong l = (ulong) acb_poly_length(x->term[i].P);
        work += l > 0 ? l * (l - 1) / 2 : 0;
        if (work > (ulong) ADF_RFUN_WORK_MAX)
            return ADF_LIMIT;
    }
    fmpz_init(mnum);
    fmpz_init(twonum);
    fmpz_init(num2);
    fmpz_init(den2);
    acb_init(pA);
    acb_init(u);
    fmpz_neg(mnum, num);
    fmpz_mul_2exp(twonum, num, 1);
    fmpz_mul(num2, num, num);
    fmpz_mul(den2, den, den);
    t = rf_alloc(x->len);
    for (i = 0; i < x->len; i++)
    {
        const adf_rterm_struct * a = x->term + i;
        adf_rterm_struct * r = t + i;

        acb_set(r->A, a->A);
        acb_const_pi(pA, p);
        acb_mul(pA, pA, a->A, p);
        /* B + 2 pi A q */
        acb_mul_fmpz(u, pA, twonum, p);
        acb_div_fmpz(u, u, den, p);
        acb_add(r->B, a->B, u, p);
        /* C - B q - pi A q^2 */
        acb_mul_fmpz(u, a->B, num, p);
        acb_div_fmpz(u, u, den, p);
        acb_sub(r->C, a->C, u, p);
        acb_mul_fmpz(u, pA, num2, p);
        acb_div_fmpz(u, u, den2, p);
        acb_sub(r->C, r->C, u, p);
        /* P(x - q) */
        acb_poly_set(r->P, a->P);
        L = acb_poly_length(r->P);
        for (k = 0; k + 1 < L; k++)
            for (j = L - 2; j >= k; j--)
            {
                acb_mul_fmpz(u, r->P->coeffs + j + 1, mnum, p);
                acb_div_fmpz(u, u, den, p);
                acb_add(r->P->coeffs + j, r->P->coeffs + j, u, p);
            }
        _acb_poly_normalise(r->P);
        if (!rf_result_ok(r))
        {
            rf_free(t, x->len);
            t = NULL;
            break;
        }
    }
    fmpz_clear(mnum);
    fmpz_clear(twonum);
    fmpz_clear(num2);
    fmpz_clear(den2);
    acb_clear(pA);
    acb_clear(u);
    if (t == NULL && x->len > 0)
        return ADF_NOT_DETERMINED;
    rf_commit(y, t, x->len);
    return ADF_OK;
}

/* Dilation by h = num/den != 0 (analysis P5): P(h x), A h^2, B h, C. A h^2 = (A num^2)/den^2; B h =
   (B num)/den; the coefficient p_j h^j = p_j w_j with w_0 = 1, w_j = (w_(j-1) num)/den. */
int
adf_rfun_dilate_rat(adf_rfun_t y, const adf_rfun_t x, const adf_rat_t h, slong prec)
{
    adf_rterm_struct * t;
    slong i, j, p = FLINT_MAX(prec, 2);
    const fmpz * num = fmpq_numref(h->q), * den = fmpq_denref(h->q);
    fmpz_t num2, den2;
    acb_t w;
    int st = ADF_OK;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(x);
    ADF_INV_RAT(h);
    if (!rf_input_ok(x) || !rf_rat_ok(h->q))
        return ADF_LIMIT;
    if (fmpz_is_zero(num))
        return ADF_DOMAIN;
    fmpz_init(num2);
    fmpz_init(den2);
    acb_init(w);
    fmpz_mul(num2, num, num);
    fmpz_mul(den2, den, den);
    t = rf_alloc(x->len);
    for (i = 0; i < x->len && st == ADF_OK; i++)
    {
        const adf_rterm_struct * a = x->term + i;
        adf_rterm_struct * r = t + i;
        slong L = acb_poly_length(a->P);

        acb_mul_fmpz(r->A, a->A, num2, p);
        acb_div_fmpz(r->A, r->A, den2, p);
        acb_mul_fmpz(r->B, a->B, num, p);
        acb_div_fmpz(r->B, r->B, den, p);
        acb_set(r->C, a->C);
        acb_poly_fit_length(r->P, L);
        acb_one(w);
        for (j = 0; j < L; j++)
        {
            acb_mul(r->P->coeffs + j, a->P->coeffs + j, w, p);
            acb_mul_fmpz(w, w, num, p);
            acb_div_fmpz(w, w, den, p);
        }
        _acb_poly_set_length(r->P, L);
        _acb_poly_normalise(r->P);
        if (!rf_result_ok(r))
            st = ADF_NOT_DETERMINED;
    }
    fmpz_clear(num2);
    fmpz_clear(den2);
    acb_clear(w);
    if (st != ADF_OK)
    {
        rf_free(t, x->len);
        return st;
    }
    rf_commit(y, t, x->len);
    return ADF_OK;
}

/* s = { v^2 : v in x } by interval squaring (api-4.md section 5): [l^2, u^2] with l, u the lower and upper
   bounds of |x|, rounded outward (arb.rst:425-433; arf.rst:590). l = 0 when x contains 0. */
static void
rf_interval_sqr(arb_t s, const arb_t x, slong p)
{
    arf_t l, u;

    arf_init(l);
    arf_init(u);
    arb_get_abs_lbound_arf(l, x, p);
    arb_get_abs_ubound_arf(u, x, p);
    arf_mul(l, l, l, p, ARF_RND_FLOOR);
    arf_mul(u, u, u, p, ARF_RND_CEIL);
    arb_set_interval_arf(s, l, u, p);
    arf_clear(l);
    arf_clear(u);
}

/* Dilation by the real component h = a->inf of an idele (api-4.md section 5): A h^2 with h^2 by interval
   squaring, B h, p_j h^j with h^j = h h ... h (each a ball product; h excludes 0, conventions 5.7). */
int
adf_rfun_dilate_idele(adf_rfun_t y, const adf_rfun_t x, const adf_idele_t a, slong prec)
{
    adf_rterm_struct * t;
    slong i, j, p = FLINT_MAX(prec, 2);
    arb_t h2, w;
    int st = ADF_OK;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(x);
    ADF_INV_IDELE_R(a);
    if (!rf_input_ok(x))
        return ADF_LIMIT;
    arb_init(h2);
    arb_init(w);
    rf_interval_sqr(h2, a->inf, p);
    t = rf_alloc(x->len);
    for (i = 0; i < x->len && st == ADF_OK; i++)
    {
        const adf_rterm_struct * b = x->term + i;
        adf_rterm_struct * r = t + i;
        slong L = acb_poly_length(b->P);

        acb_mul_arb(r->A, b->A, h2, p);
        acb_mul_arb(r->B, b->B, a->inf, p);
        acb_set(r->C, b->C);
        acb_poly_fit_length(r->P, L);
        arb_one(w);
        for (j = 0; j < L; j++)
        {
            acb_mul_arb(r->P->coeffs + j, b->P->coeffs + j, w, p);
            arb_mul(w, w, a->inf, p);
        }
        _acb_poly_set_length(r->P, L);
        _acb_poly_normalise(r->P);
        if (!rf_result_ok(r))
            st = ADF_NOT_DETERMINED;
    }
    arb_clear(h2);
    arb_clear(w);
    if (st != ADF_OK)
    {
        rf_free(t, x->len);
        return st;
    }
    rf_commit(y, t, x->len);
    return ADF_OK;
}

/* Reflection x(-t): odd coefficients and B change sign (exact negations). */
int
adf_rfun_reflect(adf_rfun_t y, const adf_rfun_t x)
{
    adf_rterm_struct * t;
    slong i, j;

    ADF_INV_RFUN(x);
    if (!rf_input_ok(x))
        return ADF_LIMIT;
    t = rf_alloc(x->len);
    for (i = 0; i < x->len; i++)
    {
        rf_term_set(t + i, x->term + i);
        for (j = 1; j < acb_poly_length(t[i].P); j += 2)
            acb_neg(t[i].P->coeffs + j, t[i].P->coeffs + j);
        acb_neg(t[i].B, t[i].B);
    }
    rf_commit(y, t, x->len);
    return ADF_OK;
}

/* Conjugation: every coefficient and A, B, C conjugated (exact). Re(A) is unchanged. */
int
adf_rfun_conj(adf_rfun_t y, const adf_rfun_t x)
{
    adf_rterm_struct * t;
    slong i, j;

    ADF_INV_RFUN(x);
    if (!rf_input_ok(x))
        return ADF_LIMIT;
    t = rf_alloc(x->len);
    for (i = 0; i < x->len; i++)
    {
        rf_term_set(t + i, x->term + i);
        for (j = 0; j < acb_poly_length(t[i].P); j++)
            acb_conj(t[i].P->coeffs + j, t[i].P->coeffs + j);
        acb_conj(t[i].A, t[i].A);
        acb_conj(t[i].B, t[i].B);
        acb_conj(t[i].C, t[i].C);
    }
    rf_commit(y, t, x->len);
    return ADF_OK;
}

/* ------------------------------------------------------------------ evaluation (api-4.md section 6) */

/* z = sum of P(x) exp(-pi A x^2 + B x + C) over the terms, x a real ball: Horner for P (acb_poly.rst:446),
   x^2 by interval squaring, acb_exp. Each operation encloses its exact result on all input points
   (arb.rst:6-12), so z encloses phi(t) for every t in x and every member of the parameter balls.
   A term with the zero polynomial contributes the exact 0 and is skipped. */
int
adf_rfun_eval(acb_t z, const adf_rfun_t phi, const arb_t x, slong prec)
{
    slong i, p = FLINT_MAX(prec, 2);
    acb_t sum, xc, v, e, u;
    arb_t x2, pi;
    int st = ADF_OK;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(phi);
    if (!rf_input_ok(phi))
        return ADF_LIMIT;
    if (!arb_is_finite(x))
        return ADF_DOMAIN;
    acb_init(sum);
    acb_init(xc);
    acb_init(v);
    acb_init(e);
    acb_init(u);
    arb_init(x2);
    arb_init(pi);
    acb_set_arb(xc, x);
    rf_interval_sqr(x2, x, p);
    arb_const_pi(pi, p);
    for (i = 0; i < phi->len; i++)
    {
        const adf_rterm_struct * a = phi->term + i;

        if (acb_poly_length(a->P) == 0)
            continue;
        acb_poly_evaluate_horner(v, a->P, xc, p);
        acb_mul_arb(e, a->A, x2, p);
        acb_mul_arb(e, e, pi, p);
        acb_sub(e, a->C, e, p);
        acb_mul_arb(u, a->B, x, p);
        acb_add(e, e, u, p);
        acb_exp(e, e, p);
        acb_mul(v, v, e, p);
        acb_add(sum, sum, v, p);
    }
    if (!acb_is_finite(sum))
        st = ADF_NOT_DETERMINED;
    else
        acb_swap(z, sum);
    acb_clear(sum);
    acb_clear(xc);
    acb_clear(v);
    acb_clear(e);
    acb_clear(u);
    arb_clear(x2);
    arb_clear(pi);
    return st;
}

/* ------------------------------------------------------------------ slice 4e: derivative, transform, integrals */
/* Sources, read before the code (lane f4-slice3; statements in docs/api-4b.md "Slice 4e"):
   - docs/api-4.md section 5 (derivative P' + (B - 2 pi A x) P, A, B, C unchanged; R1 and R2), section 6 (the
     integral A^(-1/2) exp(C + B^2/(4 pi A)) sum_j p_j H_j(B), the norm as the integral of phi conj(phi) with
     every cross term, clipped to [0, infinity) after the enclosure), section 1 (statuses, caps D1);
   - docs/proofs/analysis.md:174-212 (Proposition 5): H_0 = 1, H_(j+1) = H_j' + z H_j/(2 pi A),
     F phi(y) = A^(-1/2) exp(C + z^2/(4 pi A)) sum_j p_j H_j(z), z = B + 2 pi i y, the root positive for A > 0;
   - docs/conventions.md 6.1: F f(y) = integral f(x) conj(psi(x y)) dx, psi_inf(x) = E(-x): the kernel is
     exp(+2 pi i x y);
   - refs/src/flint-3.0.1/acb.rst:590-615 (acb_sqrt: sqrt(a + bi) = u/2 + ib/u, u = sqrt(2(|a + bi| + a)), the
     root with positive real part on Re > 0; acb_sqrt_analytic with analytic = 1 gives a NaN-containing result
     if z touches the branch cut), :445-449 (acb_mul_onei), :509 (acb_inv), :658-661 (acb_exp);
     acb_poly.rst:140-146 (shift_left), :250-253 (add), :283-289 (scalar_mul), :563-569 (derivative);
     arb.rst:417-423 (arb_nonnegative_part). */

/* A^(-1/2) with the root of R2: acb_sqrt_analytic (analytic = 1) then acb_inv. 1 if the root is finite with a
   certified positive real part (R2 steps 1-2: the chosen root of a canonical A lies in the right half-plane). */
static int
rf_amplitude(acb_t r, const acb_t A, slong p)
{
    acb_sqrt_analytic(r, A, 1, p);
    if (!acb_is_finite(r) || !arb_is_positive(acb_realref(r)))
        return 0;
    acb_inv(r, r, p);
    return acb_is_finite(r);
}

/* The work units of the transform of a term of length L (one per multiply-add, docs/api-4b.md 4e statement 2):
   the recurrence step j -> j + 1 costs 3 j + 2 (j for G_j', j + 1 for each of the two other products), the
   sum p_j G_j costs j + 1 for each j, the amplitude L; in all 2 L^2 - L + 1, and 0 for L = 0. */
static ulong
rf_fourier_work(ulong L)
{
    return L == 0 ? 0 : 2 * L * L - L + 1;
}

/* One term of R1: A' = 1/A, B' = i B/A, C' = C + B^2/(4 pi A) and Q(y) = A^(-1/2) sum_j p_j G_j(y), where
   G_j(y) = H_j(B + 2 pi i y). With w = 1/(2 pi A), H_j'(z) = G_j'(y)/(2 pi i) and z w = w B + (i/A) y, the
   recurrence of P5 reads G_0 = 1, G_(j+1) = G_j'/(2 pi i) + w B G_j + (i/A) y G_j. Returns 0 when the root is
   not certified (R2) or a result fails the predicate (Re(1/A) > 0, finite), checked by the caller. */
static int
rf_fourier_term(adf_rterm_struct * r, const adf_rterm_struct * a, slong p)
{
    acb_poly_t G, Gn, T, S;
    acb_t inv, w, wB, c1, iinv, root;
    slong j, L = acb_poly_length(a->P);
    int ok = 1;

    acb_poly_init(G);
    acb_poly_init(Gn);
    acb_poly_init(T);
    acb_poly_init(S);
    acb_init(inv);
    acb_init(w);
    acb_init(wB);
    acb_init(c1);
    acb_init(iinv);
    acb_init(root);
    acb_inv(inv, a->A, p);
    acb_set(r->A, inv);                                   /* A' = 1/A */
    acb_mul(r->B, a->B, inv, p);
    acb_mul_onei(r->B, r->B);                             /* B' = i B/A */
    acb_const_pi(c1, p);
    acb_mul_2exp_si(c1, c1, 1);                           /* 2 pi */
    acb_div(w, inv, c1, p);                               /* w = 1/(2 pi A) */
    acb_sqr(wB, a->B, p);
    acb_mul(wB, wB, w, p);
    acb_mul_2exp_si(wB, wB, -1);
    acb_add(r->C, a->C, wB, p);                           /* C' = C + B^2 w/2 = C + B^2/(4 pi A) */
    if (L > 0)
    {
        acb_inv(c1, c1, p);
        acb_div_onei(c1, c1);                             /* 1/(2 pi i) */
        acb_mul(wB, w, a->B, p);                          /* w B */
        acb_mul_onei(iinv, inv);                          /* 2 pi i w = i/A */
        ok = rf_amplitude(root, a->A, p);
        acb_poly_one(G);
        acb_poly_scalar_mul(S, G, a->P->coeffs, p);       /* p_0 G_0 */
        for (j = 0; j + 1 < L && ok; j++)
        {
            acb_poly_derivative(T, G, p);
            acb_poly_scalar_mul(T, T, c1, p);             /* G_j'/(2 pi i) */
            acb_poly_scalar_mul(Gn, G, wB, p);
            acb_poly_add(Gn, Gn, T, p);
            acb_poly_shift_left(T, G, 1);
            acb_poly_scalar_mul(T, T, iinv, p);           /* (i/A) y G_j */
            acb_poly_add(G, Gn, T, p);
            acb_poly_scalar_mul(T, G, a->P->coeffs + j + 1, p);
            acb_poly_add(S, S, T, p);
        }
        acb_poly_scalar_mul(r->P, S, root, p);            /* the amplitude belongs in Q (R1 step 2) */
    }
    else
        acb_poly_zero(r->P);
    acb_poly_clear(G);
    acb_poly_clear(Gn);
    acb_poly_clear(T);
    acb_poly_clear(S);
    acb_clear(inv);
    acb_clear(w);
    acb_clear(wB);
    acb_clear(c1);
    acb_clear(iinv);
    acb_clear(root);
    return ok;
}

int
adf_rfun_fourier(adf_rfun_t y, const adf_rfun_t x, slong prec)
{
    adf_rterm_struct * t;
    slong i, p = FLINT_MAX(prec, 2);
    ulong work = 0;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(x);
    if (!rf_input_ok(x))
        return ADF_LIMIT;
    for (i = 0; i < x->len; i++)
    {
        work += rf_fourier_work((ulong) acb_poly_length(x->term[i].P));
        if (work > (ulong) ADF_RFUN_WORK_MAX)
            return ADF_LIMIT;
    }
    t = rf_alloc(x->len);
    for (i = 0; i < x->len; i++)
        if (!rf_fourier_term(t + i, x->term + i, p) || !rf_result_ok(t + i))
        {
            rf_free(t, x->len);
            return ADF_NOT_DETERMINED;
        }
    rf_commit(y, t, x->len);
    return ADF_OK;
}

/* Derivative (api-4.md section 5): the coefficient k of P' + (B - 2 pi A x) P is
   (k + 1) p_(k+1) + B p_k - 2 pi A p_(k-1). A zero P stays zero; a nonzero P grows by one coefficient. */
int
adf_rfun_derivative(adf_rfun_t y, const adf_rfun_t x, slong prec)
{
    adf_rterm_struct * t;
    slong i, p = FLINT_MAX(prec, 2), coeffs = 0;
    acb_poly_t T;
    acb_t pA;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(x);
    if (!rf_input_ok(x))
        return ADF_LIMIT;
    for (i = 0; i < x->len; i++)
    {
        slong L = acb_poly_length(x->term[i].P);
        coeffs += L > 0 ? L + 1 : 0;
        if (coeffs > ADF_RFUN_COEFFS_MAX)
            return ADF_LIMIT;
    }
    acb_poly_init(T);
    acb_init(pA);
    t = rf_alloc(x->len);
    for (i = 0; i < x->len; i++)
    {
        const adf_rterm_struct * a = x->term + i;
        adf_rterm_struct * r = t + i;

        acb_set(r->A, a->A);
        acb_set(r->B, a->B);
        acb_set(r->C, a->C);
        acb_const_pi(pA, p);
        acb_mul(pA, pA, a->A, p);
        acb_mul_2exp_si(pA, pA, 1);                       /* 2 pi A */
        acb_poly_derivative(r->P, a->P, p);
        acb_poly_scalar_mul(T, a->P, a->B, p);
        acb_poly_add(r->P, r->P, T, p);
        acb_poly_shift_left(T, a->P, 1);
        acb_poly_scalar_mul(T, T, pA, p);
        acb_poly_sub(r->P, r->P, T, p);
        if (!rf_result_ok(r))
        {
            rf_free(t, x->len);
            t = NULL;
            break;
        }
    }
    acb_poly_clear(T);
    acb_clear(pA);
    if (t == NULL && x->len > 0)
        return ADF_NOT_DETERMINED;
    rf_commit(y, t, x->len);
    return ADF_OK;
}

/* The integral of one term with P != 0 (docs/api-4b.md 4e statement 3): with w = 1/(2 pi A),
   M_j = integral x^j exp(-pi A x^2 + B x + C) dx = A^(-1/2) exp(C + B^2 w/2) h_j, h_0 = 1, h_1 = w B,
   h_(j+1) = w (j h_(j-1) + B h_j). Adds the value to z. Returns 0 if the root is not certified. */
static int
rf_integral_term(acb_t z, const adf_rterm_struct * a, slong p)
{
    acb_t w, h0, h1, h2, s, e;
    slong j, L = acb_poly_length(a->P);
    int ok;

    acb_init(w);
    acb_init(h0);
    acb_init(h1);
    acb_init(h2);
    acb_init(s);
    acb_init(e);
    acb_const_pi(w, p);
    acb_mul_2exp_si(w, w, 1);
    acb_mul(w, w, a->A, p);
    acb_inv(w, w, p);                                     /* w = 1/(2 pi A) */
    acb_one(h0);
    acb_mul(h1, w, a->B, p);
    acb_set(s, a->P->coeffs);
    for (j = 1; j < L; j++)
    {
        acb_addmul(s, a->P->coeffs + j, h1, p);           /* s = sum_(k <= j) p_k h_k */
        acb_mul_si(h2, h0, j, p);
        acb_addmul(h2, a->B, h1, p);
        acb_mul(h2, h2, w, p);                            /* h_(j+1) = w (j h_(j-1) + B h_j) */
        acb_swap(h0, h1);
        acb_swap(h1, h2);
    }
    acb_sqr(e, a->B, p);
    acb_mul(e, e, w, p);
    acb_mul_2exp_si(e, e, -1);
    acb_add(e, e, a->C, p);
    acb_exp(e, e, p);                                     /* exp(C + B^2/(4 pi A)) */
    ok = rf_amplitude(h0, a->A, p);
    acb_mul(e, e, h0, p);
    acb_mul(e, e, s, p);
    acb_add(z, z, e, p);
    acb_clear(w);
    acb_clear(h0);
    acb_clear(h1);
    acb_clear(h2);
    acb_clear(s);
    acb_clear(e);
    return ok;
}

int
adf_rfun_integral(acb_t z, const adf_rfun_t phi, slong prec)
{
    slong i, p = FLINT_MAX(prec, 2);
    acb_t sum;
    int ok = 1;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(phi);
    if (!rf_input_ok(phi))
        return ADF_LIMIT;
    acb_init(sum);
    for (i = 0; i < phi->len && ok; i++)
        if (acb_poly_length(phi->term[i].P) > 0)          /* a zero P contributes the exact 0 */
            ok = rf_integral_term(sum, phi->term + i, p);
    ok = ok && acb_is_finite(sum);
    if (ok)
        acb_swap(z, sum);
    acb_clear(sum);
    return ok ? ADF_OK : ADF_NOT_DETERMINED;
}

/* The squared L2 norm: the integral of phi conj(phi), the product of slice 4d with every ordered pair (k, l),
   so every cross term enters (api-4.md section 6). The value is real for members; its enclosure is the real
   part, intersected with [0, infinity) (arb.rst:417-423). A provably negative enclosure would be an internal
   defect (api-4.md section 6); it cannot happen, since the enclosure contains the nonnegative true value. */
int
adf_rfun_norm2(arb_t z, const adf_rfun_t phi, slong prec)
{
    adf_rfun_t c, prod;
    acb_t v;
    int st;

    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    ADF_INV_RFUN(phi);
    if (!rf_input_ok(phi))
        return ADF_LIMIT;
    adf_rfun_init(c);
    adf_rfun_init(prod);
    acb_init(v);
    st = adf_rfun_conj(c, phi);
    if (st == ADF_OK)
        st = adf_rfun_mul(prod, phi, c, prec);
    if (st == ADF_OK)
        st = adf_rfun_integral(v, prod, prec);
    if (st == ADF_OK)
    {
        if (arb_is_negative(acb_realref(v)))
            flint_abort();                                /* internal defect, see above */
        arb_nonnegative_part(z, acb_realref(v));
    }
    adf_rfun_clear(c);
    adf_rfun_clear(prod);
    acb_clear(v);
    return st;
}
