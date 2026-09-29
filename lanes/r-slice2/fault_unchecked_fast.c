/* roots_real.c: the candidates of Algorithm RR (the real roots) by an exact isolation and refinement (lane
   r-slice1, issue adf-8di). The design, its statements and their proofs: docs/design/real-roots.md
   (Lemma R1, Algorithm D with Propositions R2 and R3, Algorithm F with Proposition R4, Proposition R5). The
   reference is proto/real_isolation.py; the tests are tests/test_roots_real_isolate.c.

   The caller is adf_roots_real (src/roots.c). It passes the normalised polynomial g of its input (squarefree,
   primitive, positive leading coefficient, degree >= 1) and keeps the certificate of Algorithm RR: FLINT's
   count, the exact signs of solvers P3.8 at the exact end points of every ball, the order, the accuracy of
   S-D19 (real_finish in src/roots.c). Nothing computed here is trusted by that certificate.

   Sources (refs/src/), read before the code:
     [SM] sagraloff-mehlhorn/tex/arxivfinal.tex:547 to 553: Descartes' rule of signs for an interval I = (a, b):
          P_I(x) = (x + 1)^n P((a x + b) / (x + 1)), v_I the sign variations of its coefficients (zeros not
          considered, footnote at :547), v_I >= m_I and v_I = m_I modulo 2, so v_I = m_I when v_I <= 1;
          :569: "each interval I of width w(I) < sigma_P / 2 yields v_I = 0 or v_I = 1";
          :571 to 575: v(P, I_1) + v(P, I_2) <= v(P, I) for disjoint subintervals I_1, I_2 of I.
     [KS] kerber-sagraloff/tex/arxiv.tex:278 to 298: Algorithm Eqir, one step of the quadratic interval
          refinement; :337 to 341: N squared after a successful step, sqrt(N) after a failing one, a bisection
          and N = 4 when N drops to 2, N = 4 at the start.
     FLINT 3.0.1 (the .rst files under flint-3.0.1, the sources under flint-src-3.0.1):
          _fmpz_poly_taylor_shift, fmpz_poly.rst:2492 to 2494 (in place, composes with x + c; taylor_shift.c:15
          to 21); _fmpz_poly_scale_2exp, fmpz_poly.rst:578 to 581, and its source scale_2exp.c:16 to 63
          (coefficient i times 2^(k i) for k > 0, times 2^((-k)(n - i)) for k < 0, then divided by the 2-content:
          a positive multiple of p(2^k X)); _fmpz_poly_remove_content_2exp, fmpz_poly.rst:570 to 575;
          _fmpz_poly_set_length, fmpz_poly.rst:144 to 147 (demotes the coefficients beyond the new length);
          _fmpz_poly_reverse, fmpz_poly.rst:259 to 264 (aliasing allowed);
          _fmpz_poly_div_root, fmpz_poly.rst:1814 to 1819 (quotient by x - c, aliasing allowed; div_root.c:16 to
          40); fmpz_poly_evaluate_fmpz, fmpz_poly.rst:2230; fmpz_poly_derivative, fmpz_poly.rst:2176;
          arf_set_fmpz_2exp, arf.rst:227 to 229 (m 2^e, exact); arf_cmp, arf.rst:330; mag_set_ui_2exp_si,
          mag.rst:149 to 151 (an upper bound of 1 2^y, exact since 1 fits the mantissa); mag_zero, mag.rst:87.

   Every decision is an exact sign, an exact grid integer or a comparison of exact dyadic numbers.
   Arb filters certify the same sign or grid integer; uncertainty falls back to integer arithmetic. Exponents are slong; the
   products that give shifts are checked (S-D18). A cell is (c, k): the open interval (c 2^k, (c + 1) 2^k); a
   point is (c, k): the number c 2^k. No cell with k < 2 - ADF_ROOTS_BITS_MAX is formed: the function returns
   ADF_LIMIT instead (then the ball of the cell, of radius 2^(k-1), would not be of admissible size). */

#include <flint/fmpz_vec.h>
#include <flint/arf.h>
#include <flint/mag.h>

#include <adelefeld.h>

#if defined(__GNUC__) || defined(__clang__)
#define ADF_ROOTS_HIDDEN __attribute__((visibility("hidden")))
#else
#define ADF_ROOTS_HIDDEN
#endif

ADF_ROOTS_HIDDEN int adf_roots_real_isolate(arb_ptr cand, slong * m, const fmpz_poly_t g, slong prec);
ADF_ROOTS_HIDDEN int adf_roots_real_isolate_counted(arb_ptr cand, slong * m, const fmpz_poly_t g,
                                                 slong prec, slong count);

/* the least exponent of a cell (see above) */
#define KMIN (2 - (slong) ADF_ROOTS_BITS_MAX)

/* *r = x * y for x, y >= 0; 0 if the product exceeds WORD_MAX */
static int
mul_checked_nonneg(slong * r, slong x, slong y)
{
    if (x != 0 && y > WORD_MAX / x)
        return 0;
    *r = x * y;
    return 1;
}

/* ---- the items: cells and points ---- */

typedef struct
{
    fmpz * c;
    slong * k;
    int * point;                /* 1: the point c 2^k; 0: the cell (c 2^k, (c + 1) 2^k) */
    slong n, alloc;
} items_t;

static void
items_init(items_t * it, slong alloc)
{
    it->alloc = alloc > 0 ? alloc : 1;
    it->c = _fmpz_vec_init(it->alloc);
    it->k = flint_malloc(it->alloc * sizeof(slong));
    it->point = flint_malloc(it->alloc * sizeof(int));
    it->n = 0;
}

static void
items_clear(items_t * it)
{
    _fmpz_vec_clear(it->c, it->alloc);
    flint_free(it->k);
    flint_free(it->point);
}

/* appends (c, k); the number of items is at most the degree of g (each holds a different root), so the caller
   sizes the arrays once and this never grows them */
static void
items_push(items_t * it, const fmpz_t c, slong k, int point)
{
    if (it->n >= it->alloc)
        flint_abort();                          /* more items than roots: a defect of this file (S-D20) */
    fmpz_set(it->c + it->n, c);
    it->k[it->n] = k;
    it->point[it->n] = point;
    it->n++;
}

/* ---- exact values at dyadic points ---- */

/* r = 2^(d max(0, -e)) g(m 2^e), d = deg g >= 0: an integer with the sign of g(m 2^e). For e >= 0 the value
   g(m 2^e) itself (fmpz_poly_evaluate_fmpz); for e < 0, with t = -e, the homogeneous Horner rule
   r = r m + g_i 2^(t (d - i)), i = d - 1, ..., 0, from r = g_d (as real_sign_at, src/roots.c). Two points
   with the same e get the same factor, so their values may be compared. t <= ADF_ROOTS_BITS_MAX - 2 here. */
static void
poly_value_2exp(fmpz_t r, const fmpz_poly_t g, const fmpz_t m, slong e)
{
    slong i, d = fmpz_poly_degree(g), t;
    fmpz_t x;

    fmpz_init(x);
    if (e >= 0)
    {
        fmpz_mul_2exp(x, m, (ulong) e);
        fmpz_poly_evaluate_fmpz(r, g, x);
    }
    else
    {
        t = -e;
        if (d > 0 && (ulong) d > (ulong) WORD_MAX / (ulong) t)
            flint_abort();                      /* t d bits: no polynomial of that size can exist */
        fmpz_set(r, g->coeffs + d);
        for (i = d - 1; i >= 0; i--)
        {
            fmpz_mul(r, r, m);
            fmpz_mul_2exp(x, g->coeffs + i, (ulong) t * (ulong) (d - i));
            fmpz_add(r, r, x);
        }
    }
    fmpz_clear(x);
}

/* Values in a fixed local coordinate. q(X) is a positive multiple of
   g((origin + X) 2^scale). All refinement points have e <= scale.
   The common multiplier cancels in the secant ratio; signs do not change.
   Design R8 proves that this changes no refinement decision or ball. */
typedef struct
{
    fmpz_poly_t q;
    fmpz_t origin;
    slong scale;
} local_eval;

static void
value_2exp(fmpz_t r, const local_eval * g, const fmpz_t m, slong e)
{
    fmpz_t x;
    fmpz_init(x);
    fmpz_mul_2exp(x, g->origin, (ulong) (g->scale - e));
    fmpz_sub(x, m, x);
    poly_value_2exp(r, g->q, x, e - g->scale);
    fmpz_clear(x);
}

/* Arb is only a filter for exact decisions. An uncertain interval falls back
   to the integer computation. refs/src/flint-3.0.1/arb.rst:155-165, :531-553,
   :639-650, :735-737 describe rounding, unique integers, signs and absolute value. */
static void
local_ball(arb_t r, const local_eval * g, const fmpz_t m, slong e, slong prec)
{
    fmpz_t x, exp;
    arb_t a, t;
    slong i;
    fmpz_init(x); fmpz_init(exp);
    arb_init(a); arb_init(t);
    fmpz_mul_2exp(x, g->origin, (ulong) (g->scale - e));
    fmpz_sub(x, m, x);
    fmpz_set_si(exp, e - g->scale);
    arb_set_round_fmpz_2exp(a, x, exp, prec);
    arb_zero(r);
    for (i = fmpz_poly_degree(g->q); i >= 0; i--)
    {
        arb_mul(r, r, a, prec);
        arb_set_round_fmpz(t, g->q->coeffs + i, prec);
        arb_add(r, r, t, prec);
    }
    arb_clear(a); arb_clear(t); fmpz_clear(x); fmpz_clear(exp);
}

static int
secant_grid(fmpz_t t, const local_eval * g, const fmpz_t a, const fmpz_t b, slong e, slong jj)
{
    arb_t fa, fb, r;
    int ok;
    slong prec = jj <= WORD_MAX - 64 ? jj + 64 : WORD_MAX;
    if (fmpz_poly_degree(g->q) < 8)
        return 0;
    arb_init(fa); arb_init(fb); arb_init(r);
    local_ball(fa, g, a, e, prec); local_ball(fb, g, b, e, prec);
    arb_abs(fa, fa); arb_abs(fb, fb);
    arb_add(r, fa, fb, prec);
    arb_div(r, fa, r, prec);
    arb_mul_2exp_si(r, r, jj + 1);
    arb_add_ui(r, r, 1, prec); arb_mul_2exp_si(r, r, -1);
    arb_floor(r, r, prec);
    ok = arb_get_unique_fmpz(t, r);
    arb_clear(fa); arb_clear(fb); arb_clear(r);
    return ok;
}

static int
sign_2exp(const local_eval * g, const fmpz_t m, slong e)
{
    fmpz_t r;
    arb_t ball;
    int s;
    if (fmpz_poly_degree(g->q) >= 8)
    {
        arb_init(ball);
        local_ball(ball, g, m, e, 64);
        s = arb_is_positive(ball) ? 1 : arb_is_negative(ball) ? -1 : 0;
        arb_clear(ball);
        if (s != 0)
            return s;
    }
    fmpz_init(r);
    value_2exp(r, g, m, e);
    s = fmpz_sgn(r);
    fmpz_clear(r);
    return s;
}

/* ---- Lemma R1: a bound 2^K on the absolute values of the complex roots ---- */

/* 1 if |h_n| 2^(K n) > sum_(i < n) |h_i| 2^(K i) (for K < 0 both sides times 2^(-K n)); -1 if a shift overflows */
static int
bound_holds(const fmpz_poly_t h, slong K)
{
    slong n = fmpz_poly_degree(h), i, a = K >= 0 ? K : -K, s;
    fmpz_t lhs, rhs, t;
    int r = 1;

    if (!mul_checked_nonneg(&s, a, n))
        return -1;
    fmpz_init(lhs);
    fmpz_init(rhs);
    fmpz_init(t);
    fmpz_abs(lhs, h->coeffs + n);
    if (K >= 0)
        fmpz_mul_2exp(lhs, lhs, (ulong) s);
    for (i = 0; i < n; i++)
    {
        fmpz_abs(t, h->coeffs + i);
        fmpz_mul_2exp(t, t, (ulong) (K >= 0 ? a * i : a * (n - i)));
        fmpz_add(rhs, rhs, t);
    }
    r = fmpz_cmp(lhs, rhs) > 0;
    fmpz_clear(lhs);
    fmpz_clear(rhs);
    fmpz_clear(t);
    return r;
}

/* *K = the least integer with bound_holds (design Lemma R1), for h of degree n >= 1 with h(0) != 0. The test
   fails at -bits(h_n) and holds at bits(max_(i<n) |h_i|) + 1, and it is monotone in K; bisection between the
   two. 0 if a shift overflows (then the caller returns ADF_LIMIT). */
static int
root_bound_exp(slong * K, const fmpz_poly_t h)
{
    slong n = fmpz_poly_degree(h), lo, hi, mid;
    int r;

    lo = -(slong) fmpz_bits(h->coeffs + n);
    hi = FLINT_ABS(_fmpz_vec_max_bits(h->coeffs, n)) + 1;
    while (hi - lo > 1)
    {
        mid = lo + (hi - lo) / 2;
        r = bound_holds(h, mid);
        if (r < 0)
            return 0;
        if (r)
            hi = mid;
        else
            lo = mid;
    }
    if (bound_holds(h, hi) < 0)
        return 0;                               /* the product hi n is formed by the caller: it must fit */
    *K = hi;
    return 1;
}

/* ---- Algorithm D: bisection with Descartes' rule of signs ---- */

/* v_(0,1)(q) of [SM]:549 to 552 for I = (0, 1): the sign variations of (x + 1)^n q(1 / (x + 1)), which is the
   reverse of q composed with x + 1 (the map x -> 1 / (x + 1), that is (a x + b) / (x + 1) with a = 0, b = 1).
   Zeros are not counted ([SM]:547, footnote). Counts up to 2: only 0, 1 and "2 or more" are used. */
static slong
var01(const fmpz_poly_t q, fmpz * tmp)
{
    slong n = fmpz_poly_length(q), i, v = 0;
    int s, last = 0;
    fmpz_t one;

    fmpz_init_set_ui(one, 1);
    _fmpz_poly_reverse(tmp, q->coeffs, n, n);
    _fmpz_poly_taylor_shift(tmp, one, n);
    for (i = 0; i < n && v < 2; i++)
    {
        s = fmpz_sgn(tmp + i);
        if (s != 0 && last != 0 && s != last)
            v++;
        if (s != 0)
            last = s;
    }
    fmpz_clear(one);
    return v;
}

/* Endpoint contraction, design Proposition R6. q is the polynomial on (0,1).
   Only skip a chain if the retained cell still has v >= 2, its new endpoint is clean,
   and the entire discarded interval has v = 0. All skipped siblings then have v = 0
   by refs/src/sagraloff-mehlhorn/tex/arxivfinal.tex:571-575. No status is decided here. */
static int
contract_edge(fmpz_poly_t out, const fmpz_poly_t q, fmpz * tmp, slong jump, int hi)
{
    fmpz_poly_t base, outside;
    fmpz_t w, power, one, endpoint;
    slong i, len = fmpz_poly_length(q);
    int ok = 0;

    fmpz_poly_init(base); fmpz_poly_init(outside);
    fmpz_init(w); fmpz_init(power); fmpz_init_set_ui(one, 1); fmpz_init(endpoint);
    fmpz_one(w); fmpz_mul_2exp(w, w, (ulong) jump); fmpz_sub_ui(w, w, 1);
    fmpz_poly_set(base, q);
    _fmpz_poly_scale_2exp(base->coeffs, len, -jump);
    fmpz_poly_set(out, base);
    if (hi)
        _fmpz_poly_taylor_shift(out->coeffs, w, len);
    /* The new endpoint must not be a root: no skipped midpoint may be a root. */
    if (hi)
        fmpz_set(endpoint, out->coeffs);
    else
        fmpz_poly_evaluate_fmpz(endpoint, out, one);
    if (fmpz_is_zero(endpoint) || var01(out, tmp) < 2)
        goto done;
    fmpz_poly_set(outside, base);
    if (!hi)
        _fmpz_poly_taylor_shift(outside->coeffs, one, len);
    /* outside = base(w X) for hi, base(1 + w X) for lo. */
    fmpz_one(power);
    for (i = 1; i < len; i++)
    {
        fmpz_mul(power, power, w);
        fmpz_mul(outside->coeffs + i, outside->coeffs + i, power);
    }
    ok = 1;
    if (ok)
        _fmpz_poly_remove_content_2exp(out->coeffs, len);
done:
    fmpz_clear(w); fmpz_clear(power); fmpz_clear(one); fmpz_clear(endpoint);
    fmpz_poly_clear(base); fmpz_poly_clear(outside);
    return ok;
}

typedef struct
{
    fmpz_poly_t q;
    fmpz_t c;
    slong k, jump;
} dnode;

/* the positive roots of h (squarefree, degree >= 1, h(0) != 0, positive leading coefficient not needed) as
   items: points (c, k) with c 2^k a root, cells (c, k) with exactly one root; every positive root in exactly one
   of them (design Proposition R2). ADF_OK, or ADF_LIMIT for a cell below KMIN or an overflow of an exponent. */
static int
isolate_positive(items_t * it, const fmpz_poly_t h, slong count)
{
    slong n = fmpz_poly_degree(h), K, top = 0, alloc = 0, i, s, len;
    dnode * st = NULL;
    fmpz * tmp;
    fmpz_poly_t left, right;
    fmpz_t c, one;
    int status = ADF_OK;

    if (it->n >= count)
        return ADF_OK;
    if (!root_bound_exp(&K, h))
        return ADF_LIMIT;
    /* a larger K is a bound as well: the root cell is never below KMIN (h may have tiny roots that are not
       real; if one is real, the tree meets KMIN below and returns ADF_LIMIT there) */
    K = FLINT_MAX(K, KMIN);
    tmp = _fmpz_vec_init(n + 1);
    fmpz_poly_init(left);
    fmpz_poly_init(right);
    fmpz_init(c);
    fmpz_init_set_ui(one, 1);
    /* the root node: a positive multiple of h(2^K X) on the cell (0, K) = (0, 2^K), which holds every positive
       root (Lemma R1); q(0) != 0 and q(1) = h(2^K) != 0 */
    alloc = 4;
    st = flint_malloc(alloc * sizeof(dnode));
    for (i = 0; i < alloc; i++)
    {
        fmpz_poly_init(st[i].q);
        fmpz_init(st[i].c);
    }
    fmpz_poly_set(st[0].q, h);
    _fmpz_poly_scale_2exp(st[0].q->coeffs, n + 1, K);
    fmpz_zero(st[0].c);
    st[0].k = K;
    st[0].jump = 2;
    s = var01(st[0].q, tmp);
    if (s == 1)
        items_push(it, st[0].c, K, 0);
    else if (s >= 2)
        top = 1;
    /* invariant: every node on the stack has v >= 2, its polynomial q has q(0) != 0 and q(1) != 0, and the
       roots of q in (0, 1) are the images of the roots of h in its cell under x -> (x - c 2^k) / 2^k */
    while (top > 0 && status == ADF_OK && it->n < count)
    {
        dnode * p = st + top - 1;
        slong k = p->k, next_jump;

        if (k - 1 < KMIN)
        {
            status = ADF_LIMIT;
            break;
        }
        /* Geometric trials towards either end. Failure leaves p unchanged and
           falls back to the original bisection. A child inherits half the trial. */
        {
            slong jump = FLINT_MIN(p->jump, k - KMIN);
            int hi, accepted = 0;
            if (jump >= 2)
                for (hi = 0; hi < 2 && !accepted; hi++)
                    if (contract_edge(left, p->q, tmp, jump, hi))
                    {
                        fmpz_mul_2exp(p->c, p->c, (ulong) jump);
                        if (hi)
                        {
                            fmpz_one(c); fmpz_mul_2exp(c, c, (ulong) jump);
                            fmpz_sub_ui(c, c, 1); fmpz_add(p->c, p->c, c);
                        }
                        fmpz_poly_swap(p->q, left);
                        p->k -= jump;
                        p->jump = jump <= WORD_MAX / 2 ? 2 * jump : jump;
                        accepted = 1;
                    }
            if (accepted)
                continue;
            p->jump = FLINT_MAX(2, p->jump / 2);
        }
        next_jump = p->jump;
        len = fmpz_poly_length(p->q);
        /* left = 2^(len-1) q(X / 2) / 2^z: the cell (2c, k - 1); right = left(X + 1): the cell (2c + 1, k - 1) */
        fmpz_poly_set(left, p->q);
        _fmpz_poly_scale_2exp(left->coeffs, len, -1);
        fmpz_poly_set(right, left);
        _fmpz_poly_taylor_shift(right->coeffs, one, len);
        fmpz_mul_2exp(c, p->c, 1);              /* c = 2 c_parent */
        top--;                                  /* p is consumed; its slot is reused below */
        if (fmpz_is_zero(right->coeffs + 0))
        {
            /* q(1/2) = 0: the midpoint (2c + 1) 2^(k-1) is a root; divided out of both halves (h is squarefree,
               so it is a simple root and neither half has it again) */
            fmpz_add_ui(c, c, 1);
            items_push(it, c, k - 1, 1);
            fmpz_sub_ui(c, c, 1);
            fmpz_poly_shift_right(right, right, 1);
            _fmpz_poly_div_root(left->coeffs, left->coeffs, len, one);
            _fmpz_poly_set_length(left, len - 1);
            _fmpz_poly_normalise(left);
        }
        /* the two children, left first: v = 0 dropped, v = 1 a cell, v >= 2 pushed */
        for (i = 0; i < 2 && it->n < count; i++)
        {
            fmpz_poly_struct * q = i == 0 ? left : right;

            if (fmpz_poly_degree(q) < 1)
                continue;                       /* a nonzero constant: no root */
            _fmpz_poly_remove_content_2exp(q->coeffs, fmpz_poly_length(q));
            if (i == 1)
                fmpz_add_ui(c, c, 1);           /* 2c + 1 for the right half */
            s = var01(q, tmp);
            if (s == 1)
                items_push(it, c, k - 1, 0);
            else if (s >= 2)
            {
                if (top >= alloc)
                {
                    slong j, na = 2 * alloc;
                    st = flint_realloc(st, na * sizeof(dnode));
                    for (j = alloc; j < na; j++)
                    {
                        fmpz_poly_init(st[j].q);
                        fmpz_init(st[j].c);
                    }
                    alloc = na;
                }
                fmpz_poly_swap(st[top].q, q);
                fmpz_set(st[top].c, c);
                st[top].k = k - 1;
                st[top].jump = next_jump;
                top++;
            }
        }
    }
    for (i = 0; i < alloc; i++)
    {
        fmpz_poly_clear(st[i].q);
        fmpz_clear(st[i].c);
    }
    flint_free(st);
    _fmpz_vec_clear(tmp, n + 1);
    fmpz_poly_clear(left);
    fmpz_poly_clear(right);
    fmpz_clear(c);
    fmpz_clear(one);
    return status;
}

/* ---- Algorithm F: refinement of one cell ---- */

/* 1 if c 2^k > floor */
static int
above(const fmpz_t c, slong k, const arf_t floor)
{
    arf_t x;
    fmpz_t e;
    int r;

    arf_init(x);
    fmpz_init_set_si(e, k);
    arf_set_fmpz_2exp(x, c, e);
    r = arf_cmp(x, floor) > 0;
    arf_clear(x);
    fmpz_clear(e);
    return r;
}

/* Step G of Algorithm F (design Proposition R4(2)). The cell (c, k) holds exactly one root r of g and has an
   anchor: an end point p that is 0 (the cell touches 0: its accuracy is -1 at every bisection until it no longer
   does) or a root of g (the end is not clean). Bisection needs about k - log2 |r - p| steps to leave p behind.
   Instead, with sigma = +1 (p the left end) or -1 (p the right end) and v0 the sign of g on the open segment
   between p and r (sign g(p), or sigma sign g'(p) when g(p) = 0: a simple root), the predicate
   P(t): sign g(p + sigma 2^t) = v0 holds exactly for 2^t < |r - p| (one simple root in the cell and no other
   root between p and the far end, so g has the sign v0 between p and r and -v0 between r and the far end).
   T = floor(log2 |r - p|) <= k - 1 is found by galloping (t = k - 1, k - 2, k - 4, ...) and then bisection of
   the exponent; a zero met on the way is r itself. The new cell is (p + 2^T, p + 2^(T+1)) or
   (p - 2^(T+1), p - 2^T), of exponent T (p is a multiple of 2^k, so of 2^T); its end next to p has the sign
   v0, its far end the sign -v0 (or it is the old far end, when T = k - 1). ADF_LIMIT when |r - p| < 2^KMIN
   (P(KMIN) fails): no cell of exponent >= KMIN separates r from p. Deterministic: it depends on g and the
   cell only. anchor_hi = 0: p = c 2^k; anchor_hi = 1: p = (c + 1) 2^k. */
static int
gallop(fmpz_t c, slong * k, int * point, int * lo_clean, int * hi_clean, int * s_lo, int anchor_hi,
       const local_eval * g, const local_eval * dg)
{
    slong lo_t, hi_t, t, step, mid;
    int sigma = anchor_hi ? -1 : 1, v0, s = 0, found = 0;
    fmpz_t pc, m;

    fmpz_init(pc);
    fmpz_init(m);
    fmpz_add_ui(pc, c, anchor_hi ? 1 : 0);     /* p = pc 2^k */
    v0 = sign_2exp(g, pc, *k);
    if (v0 == 0)
        v0 = sigma * sign_2exp(dg, pc, *k);
    /* galloping: hi_t is an exponent with 2^hi_t > |r - p| (at first the width 2^k), lo_t one with
       2^lo_t < |r - p| */
    hi_t = *k;
    lo_t = WORD_MIN;
    for (step = 1; ; step = step <= (WORD_MAX / 2) ? 2 * step : step)
    {
        t = (*k - KMIN < step) ? KMIN : *k - step;
        if (t >= hi_t)
            t = hi_t - 1;
        if (t < KMIN)
            break;                              /* only when hi_t = KMIN */
        fmpz_mul_2exp(m, pc, (ulong) (*k - t));
        if (sigma > 0)
            fmpz_add_ui(m, m, 1);
        else
            fmpz_sub_ui(m, m, 1);               /* m 2^t = p + sigma 2^t */
        s = sign_2exp(g, m, t);
        if (s == 0 || s == v0)
        {
            found = s == 0;
            lo_t = t;
            break;
        }
        hi_t = t;
        if (t == KMIN)
            break;
    }
    if (lo_t == WORD_MIN)
    {
        fmpz_clear(pc);
        fmpz_clear(m);
        return ADF_LIMIT;                       /* P(t) fails down to KMIN: |r - p| < 2^KMIN */
    }
    /* bisection of the exponent between lo_t (P holds) and hi_t (P fails, or the width) */
    while (!found && hi_t - lo_t > 1)
    {
        mid = lo_t + (hi_t - lo_t) / 2;
        fmpz_mul_2exp(m, pc, (ulong) (*k - mid));
        if (sigma > 0)
            fmpz_add_ui(m, m, 1);
        else
            fmpz_sub_ui(m, m, 1);
        s = sign_2exp(g, m, mid);
        if (s == 0 || s == v0)
        {
            found = s == 0;
            lo_t = mid;
        }
        else
            hi_t = mid;
    }
    /* m 2^lo_t = p + sigma 2^lo_t, formed again for lo_t */
    fmpz_mul_2exp(m, pc, (ulong) (*k - lo_t));
    if (found)
    {
        if (sigma > 0)
            fmpz_add_ui(c, m, 1);
        else
            fmpz_sub_ui(c, m, 1);               /* the root p + sigma 2^T */
        *point = 1;
    }
    else if (sigma > 0)
    {
        /* (p + 2^T, p + 2^(T+1)): left end sign v0; right end sign -v0, or the old right end if T = k - 1 */
        if (lo_t < *k - 1)
            *hi_clean = 1;
        fmpz_add_ui(c, m, 1);
        *lo_clean = 1;
        *s_lo = v0;
    }
    else
    {
        /* (p - 2^(T+1), p - 2^T): right end sign v0; left end sign -v0, or the old left end if T = k - 1 */
        if (lo_t < *k - 1)
        {
            *lo_clean = 1;
            *s_lo = -v0;
        }
        fmpz_sub_ui(c, m, 2);
        *hi_clean = 1;
    }
    *k = lo_t;
    fmpz_clear(pc);
    fmpz_clear(m);
    return ADF_OK;
}

/* Step Q of Algorithm F: one step of Algorithm Eqir ([KS]:278 to 298) on the cell (c, k), whose end points are
   not roots, s_lo the sign of g at its left end (so -s_lo at its right end), with N = 2^jj, jj >= 2,
   k - jj >= KMIN. The grid of the N + 1 points (a + i) 2^e, a = c 2^jj, e = k - jj, i = 0, ..., N; fa, fb the
   values at the two ends with the same factor 2^(d max(0, -e)) (value_2exp), so that
   t = round(N fa / (fa - fb)) = floor((2 N |fa| + D) / (2 D)), D = |fa| + |fb|, is the grid point nearest to
   the zero of the secant ([KS]:288; fa and fb have opposite signs, so 0 <= t <= N). Then the sign at
   m' = a + t, and at m' + 1 if it equals s_lo, or at m' - 1 if it equals -s_lo ([KS]:290 to 295). Returns 2 if
   a root is met at a grid point (then c 2^k is that root, *point = 1), 1 if the step succeeds (then (c, k) is
   the grid cell with a sign change, a subcell of the old cell with end points that are not roots), 0 if it
   fails (the cell is unchanged). */
static int
qir_step(fmpz_t c, slong * k, int * point, const local_eval * g, int s_lo, slong jj)
{
    fmpz_t a, b, fa, fb, D, t;
    slong e = *k - jj;
    int s, s2, r = 0;

    fmpz_init(a);
    fmpz_init(b);
    fmpz_init(fa);
    fmpz_init(fb);
    fmpz_init(D);
    fmpz_init(t);
    fmpz_mul_2exp(a, c, (ulong) jj);
    fmpz_one(b);
    fmpz_mul_2exp(b, b, (ulong) jj);            /* b = N */
    fmpz_add(b, a, b);                          /* b = a + N, the right end */
    if (!secant_grid(t, g, a, b, e, jj))
    {
        value_2exp(fa, g, a, e);
        value_2exp(fb, g, b, e);
        fmpz_abs(fa, fa);
        fmpz_abs(fb, fb);
        fmpz_add(D, fa, fb);
        fmpz_mul_2exp(t, fa, (ulong) jj + 1);       /* 2 N |fa| */
        fmpz_add(t, t, D);
        fmpz_mul_2exp(D, D, 1);
        fmpz_fdiv_q(t, t, D);                       /* t = floor((2 N |fa| + D) / (2 D)) */
    }
    fmpz_add(a, a, t);                          /* a = m' */
    fmpz_sub(b, b, a);                          /* b = N - t */
    s = sign_2exp(g, a, e);
    if (s == 0)
        r = 2;
    else if (s == s_lo && fmpz_sgn(b) > 0)
    {
        fmpz_add_ui(t, a, 1);
        s2 = sign_2exp(g, t, e);
        if (s2 == 0)
        {
            fmpz_set(a, t);
            r = 2;
        }
        else if (s2 == -s_lo)
            r = 1;                              /* the cell (m', e) */
    }
    else if (s == -s_lo && !fmpz_is_zero(t) && fmpz_sgn(t) > 0)
    {
        fmpz_sub_ui(t, a, 1);
        s2 = sign_2exp(g, t, e);
        if (s2 == 0)
        {
            fmpz_set(a, t);
            r = 2;
        }
        else if (s2 == s_lo)
        {
            fmpz_set(a, t);
            r = 1;                              /* the cell (m' - 1, e) */
        }
    }
    if (r != 0)
    {
        fmpz_set(c, a);
        *k = e;
        *point = r == 2;
    }
    fmpz_clear(a);
    fmpz_clear(b);
    fmpz_clear(fa);
    fmpz_clear(fb);
    fmpz_clear(D);
    fmpz_clear(t);
    return r;
}

/* the cell (c, k) holds exactly one root of g and no other root of g lies in its open interval (Proposition
   R2). Refines it in place: on return with *point = 1, c 2^k is the root; with *point = 0, (c, k) is a cell
   whose end points are not roots of g (so g has opposite signs there), whose left end is above floor (when
   has_floor), and whose ball has bits(|2c + 1|) - 2 >= need (design Proposition R4). The sequence of cells
   visited does not depend on need nor on floor, only the step at which it stops. ADF_LIMIT for a cell below
   KMIN. Algorithm F: at each step, G if the cell has an anchor, else the stop test, else Q while
   N > 2, else one bisection. */
static int
refine_local(fmpz_t c, slong * k, int * point, const local_eval * g, const local_eval * dg, const arf_t floor,
            int has_floor, slong need)
{
    fmpz_t x;
    slong j = 2;                                /* N = 2^j = 4 at the start ([KS]:337 to 338) */
    int s_lo, s, lo_clean, hi_clean, st, r;

    fmpz_init(x);
    *point = 0;
    s_lo = sign_2exp(g, c, *k);
    lo_clean = s_lo != 0;
    if (!lo_clean)
        s_lo = sign_2exp(dg, c, *k);            /* a simple root at lo: g has the sign of g'(lo) right of lo */
    fmpz_add_ui(x, c, 1);
    hi_clean = sign_2exp(g, x, *k) != 0;
    for (;;)
    {
        /* step G: a cell with an anchor, 0 or a root at an end (such a cell never stops the loop: a cell that
           touches 0 has accuracy -1 < need, one with a root at an end is not clean) */
        if (fmpz_is_zero(c) || fmpz_equal_si(c, -1) || !lo_clean || !hi_clean)
        {
            int anchor_hi = fmpz_is_zero(c) ? 0 : fmpz_equal_si(c, -1) ? 1 : lo_clean;
            st = gallop(c, k, point, &lo_clean, &hi_clean, &s_lo, anchor_hi, g, dg);
            if (st != ADF_OK || *point)
            {
                fmpz_clear(x);
                return st;
            }
            continue;                           /* the new cell may have an anchor again (its other end) */
        }
        fmpz_mul_2exp(x, c, 1);
        fmpz_add_ui(x, x, 1);                   /* x = 2c + 1 */
        if (lo_clean && hi_clean && (!has_floor || above(c, *k, floor)) && (slong) fmpz_bits(x) - 2 >= need)
            break;
        /* step Q: Eqir steps while both end points are clean, N = 2^j (at most as fine as KMIN allows) */
        if (lo_clean && hi_clean && j > 1)
        {
            slong jj = FLINT_MIN(j, *k - KMIN);
            if (jj >= 2)
            {
                r = qir_step(c, k, point, g, s_lo, jj);
                if (r == 2)
                    break;                      /* a root met at a grid point */
                if (r == 1)
                {
                    j = j <= WORD_MAX / 2 ? 2 * j : j;      /* successful: N becomes N^2 ([KS]:338 to 339) */
                    continue;
                }
            }
            j /= 2;                             /* failing: N becomes sqrt(N) ([KS]:339 to 340) */
            if (j > 1)
                continue;
        }
        /* one bisection: the midpoint x 2^(k-1); afterwards N = 4 ([KS]:340 to 341) */
        j = 2;
        if (*k - 1 < KMIN)
        {
            fmpz_clear(x);
            return ADF_LIMIT;
        }
        s = sign_2exp(g, x, *k - 1);
        (*k)--;
        if (s == 0)
        {
            fmpz_set(c, x);
            *point = 1;
            break;
        }
        if (s == -s_lo)
        {
            fmpz_mul_2exp(c, c, 1);             /* the root is left of the midpoint */
            hi_clean = 1;
        }
        else
        {
            fmpz_set(c, x);                     /* right of it; c = 2c + 1 */
            lo_clean = 1;
            s_lo = s;
        }
    }
    fmpz_clear(x);
    return ADF_OK;
}

/* Keep the original cell and stop coordinates. Only evaluation is translated. */
static int
refine_cell(fmpz_t c, slong * k, int * point, const fmpz_poly_t g, const fmpz_poly_t dg,
            const arf_t floor, int has_floor, slong need)
{
    local_eval f, df;
    int st;
    (void) dg;
    fmpz_poly_init(f.q); fmpz_poly_init(df.q);
    fmpz_init(f.origin); fmpz_init(df.origin);
    if (fmpz_poly_degree(g) >= 8)
        fmpz_set(f.origin, c);
    fmpz_set(df.origin, f.origin);
    f.scale = df.scale = *k;
    fmpz_poly_set(f.q, g);
    _fmpz_poly_scale_2exp(f.q->coeffs, fmpz_poly_length(f.q), *k);
    _fmpz_poly_taylor_shift(f.q->coeffs, f.origin, fmpz_poly_length(f.q));
    _fmpz_poly_remove_content_2exp(f.q->coeffs, fmpz_poly_length(f.q));
    fmpz_poly_derivative(df.q, f.q);
    st = refine_local(c, k, point, &f, &df, floor, has_floor, need);
    fmpz_clear(f.origin); fmpz_clear(df.origin);
    fmpz_poly_clear(f.q); fmpz_poly_clear(df.q);
    return st;
}

/* ---- the whole: Algorithm RR2 of the design without its final tests (those are real_finish of roots.c) ---- */

/* the right end of the closed set of item i: (c + 1) 2^k for a cell, c 2^k for a point */
static void
item_right(arf_t r, const items_t * it, slong i)
{
    fmpz_t c, e;

    fmpz_init(c);
    fmpz_init_set_si(e, it->k[i]);
    fmpz_add_ui(c, it->c + i, it->point[i] ? 0 : 1);
    arf_set_fmpz_2exp(r, c, e);
    fmpz_clear(c);
    fmpz_clear(e);
}

/* sorts the items by their left end c 2^k (insertion sort: at most deg g items); a point before a cell with
   the same left end */
static void
items_sort(items_t * it)
{
    arf_t a, b;
    fmpz_t e;
    slong i, j, tk;
    int tp;

    arf_init(a);
    arf_init(b);
    fmpz_init(e);
    for (i = 1; i < it->n; i++)
        for (j = i; j > 0; j--)
        {
            int cmp;
            fmpz_set_si(e, it->k[j - 1]);
            arf_set_fmpz_2exp(a, it->c + j - 1, e);
            fmpz_set_si(e, it->k[j]);
            arf_set_fmpz_2exp(b, it->c + j, e);
            cmp = arf_cmp(a, b);
            if (cmp < 0 || (cmp == 0 && it->point[j - 1]))
                break;
            fmpz_swap(it->c + j - 1, it->c + j);
            tk = it->k[j - 1];
            it->k[j - 1] = it->k[j];
            it->k[j] = tk;
            tp = it->point[j - 1];
            it->point[j - 1] = it->point[j];
            it->point[j] = tp;
        }
    arf_clear(a);
    arf_clear(b);
    fmpz_clear(e);
}

/* adf_roots_real_isolate(cand, m, g, prec): g the normalised polynomial of adf_roots_real (degree >= 1,
   squarefree, primitive, positive leading coefficient), prec >= 2, cand an initialised vector of at least
   deg g balls. On ADF_OK: *m balls in cand[0, m), in increasing order, each either exact (a root of g) or with
   exact end points lo < hi that are not roots, g(lo) g(hi) < 0 and exactly one root of g inside; hi_i <
   lo_(i+1); every real root of g in one of them; arb_rel_accuracy_bits >= prec or exact (design Proposition
   R5). The balls may be of any size: the caller tests their admissible size. ADF_LIMIT: a cell below KMIN,
   or an overflow of an exponent; *m and cand are then of no use (cand stays initialised). */
int
adf_roots_real_isolate_counted(arb_ptr cand, slong * m, const fmpz_poly_t g, slong prec, slong count)
{
    slong d = fmpz_poly_degree(g), i, k, need = FLINT_MAX(prec, 2);
    fmpz_poly_t h, dg;
    items_t it;
    arf_t floor;
    fmpz_t c, e;
    int st = ADF_OK, point;

    *m = 0;
    if (d < 1 || count == 0)
        return ADF_OK;
    fmpz_poly_init(h);
    fmpz_poly_init(dg);
    items_init(&it, d);
    arf_init(floor);
    fmpz_init(c);
    fmpz_init(e);
    /* the root 0: g is squarefree, so X divides it at most once (design, step Z) */
    fmpz_poly_set(h, g);
    if (fmpz_is_zero(g->coeffs + 0))
    {
        fmpz_zero(c);
        items_push(&it, c, 0, 1);
        fmpz_poly_shift_right(h, h, 1);
    }
    if (fmpz_poly_degree(h) >= 1)
    {
        /* the positive roots of h, then those of h(-X) mirrored: cell (c, k) -> (-c - 1, k), point c -> -c */
        st = isolate_positive(&it, h, count);
        if (st == ADF_OK)
        {
            slong first = it.n;
            for (i = 1; i < fmpz_poly_length(h); i += 2)
                fmpz_neg(h->coeffs + i, h->coeffs + i);
            st = isolate_positive(&it, h, count);
            for (i = first; i < it.n && st == ADF_OK; i++)
            {
                fmpz_neg(it.c + i, it.c + i);
                if (!it.point[i])
                    fmpz_sub_ui(it.c + i, it.c + i, 1);
            }
        }
    }
    if (st == ADF_OK)
    {
        items_sort(&it);
        fmpz_poly_derivative(dg, g);
        /* the refinement, in increasing order; the floor of item i is the right end of the closed set of item
           i - 1 as isolated (not as refined), so that no refinement depends on another one */
        for (i = 0; i < it.n && st == ADF_OK; i++)
        {
            if (!it.point[i])
            {
                if (i > 0)
                    item_right(floor, &it, i - 1);
                fmpz_set(c, it.c + i);
                k = it.k[i];
                st = refine_cell(c, &k, &point, g, dg, floor, i > 0, need);
                if (st != ADF_OK)
                    break;
            }
            else
            {
                fmpz_set(c, it.c + i);
                k = it.k[i];
                point = 1;
            }
            /* the ball: exact c 2^k, or the midpoint (2c + 1) 2^(k-1) and the radius 2^(k-1) */
            if (point)
            {
                fmpz_set_si(e, k);
                arf_set_fmpz_2exp(arb_midref(cand + i), c, e);
                mag_zero(arb_radref(cand + i));
            }
            else
            {
                fmpz_mul_2exp(c, c, 1);
                fmpz_add_ui(c, c, 1);
                fmpz_set_si(e, k - 1);
                arf_set_fmpz_2exp(arb_midref(cand + i), c, e);
                mag_set_ui_2exp_si(arb_radref(cand + i), 1, k - 1);
            }
        }
        if (st == ADF_OK)
            *m = it.n;
    }
    fmpz_poly_clear(h);
    fmpz_poly_clear(dg);
    items_clear(&it);
    arf_clear(floor);
    fmpz_clear(c);
    fmpz_clear(e);
    return st;
}

/* Compatibility entry for the direct candidate tests. The public caller passes its
   already computed trusted count to the counted entry (design Proposition R7). */
int
adf_roots_real_isolate(arb_ptr cand, slong * m, const fmpz_poly_t g, slong prec)
{
    slong count = fmpz_poly_degree(g) > 0 ? fmpz_poly_num_real_roots(g) : 0;
    return adf_roots_real_isolate_counted(cand, m, g, prec, count);
}
