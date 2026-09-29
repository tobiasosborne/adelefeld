/* src/idele.c: adf_idele, product, inverse and the idele of a rational (milestone 2, slice 1, lane
   i-slice1); an exact rational factor, valuations, absolute values and the norm (slice 2, lane i-slice2).

   Contract: include/adelefeld/idele.h. Sources, read before the code was written:
   - docs/conventions.md 5.7 (struct, predicate, init, the idele of a rational, the content, the result sign
     check of gate finding G5), 3.2, 4.1, 4.3, 4.4; docs/SPEC.md 5 ("Sign preservation"; the example
     x = y = 1 +/- (1 - 2^-30)); decision M1-D4 (prec below 2 is 2); D2-2 of the brief (the real kernel works
     by end points);
   - docs/proofs/ideles.md Proposition 3 (line 67; P3.4 at line 80), Propositions 10, 11;
   - docs/api-2.md 1.3, Statements D (the set of a value, product, inverse, rational) and E (the kernel);
   - FLINT 3.0.1: correct rounding of arf, refs/src/flint-3.0.1/arf.rst:24-39; ARF_PREC_EXACT and when it is
     safe, arf.rst:92-112; arf_get_mag (an upper bound), arf.rst:403; arf_set_mag (exact), arf.rst:411;
     arf_add, arf_sub, arf_mul, arf_div, arf_fmpz_div_fmpz, arf.rst:560, 574, 590, 662, 676; the mag
     mantissa of 30 bits and its bounds, mag.rst:7, 12-15; a ball is [m - r, m + r], arb.rst:7;
     arb_is_nonzero, arb_is_finite, arb.rst:597, 606.
   Reference: proto/ideles_checks.py, ref_real_*, kernel_B, ref_idele_* (part 2).

   Slice 2 (lane i-slice2): the kernel is shared with src/idclass.c through src/idele_internal.h (hidden
   symbols); adf_idele_ends_scale (docs/api-2.md Statement F), adf_idele_mul_rat (Statement H),
   adf_idele_valuation_at, adf_idele_abs_at, adf_idele_abs_inf, adf_idele_norm (docs/proofs/ideles.md P14,
   line 343; Statement I); fmpz_remove, refs/src/flint-3.0.1/fmpz.rst:1132-1149; arf_mul_fmpz, arf_div_fmpz,
   arf.rst:598, :672. The limit ADF_IDELE_PREC_MAX of every function with a prec (decision of the orchestrator,
   2026-09-30, after lane i-review1). Reference: ref_real_scale, ref_idele_mul_rat, ref_idele_norm,
   ref_idele_valuation_at, ref_idele_abs_at (part 3). */

#include <stdio.h>

#include <adelefeld.h>

#include "invariants.h"
#include "idele_internal.h"

/* Entry checks of the debug build (conventions 4.4, CV-09; src/invariants.h). */
#ifdef ADF_CHECK_INVARIANTS
#define ID_INV(x)                                                                                      \
    do { if (!adf_idele_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idele"); } while (0)
#define ID_INV_UC(x)                                                                                   \
    do { if (!adf_ucoset_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_ucoset"); } while (0)
#else
#define ID_INV(x) ((void) 0)
#define ID_INV_UC(x) ((void) 0)
#endif

/* ---- the real kernel (api-2.md Statement E; shared through src/idele_internal.h, lane i-slice2) ---- */

/* E1: for a finite ball x = [m +- rho] that excludes 0, l = RD_p(|m| - rho) and h = RU_p(|m| + rho); returns
   the sign of m, which is the sign of every point of x. l > 0 (E1). */
int
adf_idele_ends_of_abs(arf_t l, arf_t h, const arb_t x, slong p)
{
    arf_t a, r;
    int s = arf_sgn(arb_midref(x));

    arf_init(a);
    arf_init(r);
    arf_abs(a, arb_midref(x));
    arf_set_mag(r, arb_radref(x));
    arf_sub(l, a, r, p, ARF_RND_FLOOR);
    arf_add(h, a, r, p, ARF_RND_CEIL);
    arf_clear(a);
    arf_clear(r);
    return s;
}

/* A result that fails its own sign test is a defect of the library (decision i1-6 of docs/api-2.md 1.4, as
   S-D20): it aborts and is never returned. */
void
adf_idele_kernel_defect(const char * what)
{
    fprintf(stderr, "adelefeld: src/idele.c: the real kernel produced %s (a defect of the library)\n", what);
    fflush(stderr);
    flint_abort();
}

/* Kernel B (E5): z = a ball that contains sign * [lo, hi] and excludes 0, from 0 < lo <= hi with at most p
   bits each; or ADF_NOT_DETERMINED (B1), z untouched. */
int
adf_idele_ball_from_ends(arb_t z, const arf_t lo, const arf_t hi, int sign, slong p)
{
    fmpz_t gap;
    arf_t m, d1, d2;
    mag_t rho;
    int b1;

    if (arf_sgn(lo) <= 0 || arf_cmp(lo, hi) > 0)
        adf_idele_kernel_defect("end points that are not 0 < lo <= hi");

    /* B1: e(hi) - e(lo) > p */
    fmpz_init(gap);
    fmpz_sub(gap, ARF_EXPREF(hi), ARF_EXPREF(lo));
    b1 = fmpz_cmp_si(gap, p) > 0;
    fmpz_clear(gap);
    if (b1)
        return ADF_NOT_DETERMINED;

    /* B2: lo = hi, the exact ball */
    if (arf_equal(lo, hi))
    {
        arb_set_arf(z, lo);
        if (sign < 0)
            arb_neg(z, z);
        return ADF_OK;
    }

    arf_init(m);
    arf_init(d1);
    arf_init(d2);
    mag_init(rho);
    /* B3: m = RN_p((lo + hi)/2) (the halving is exact), rho >= max(hi - m, m - lo). The differences of two
       p-bit numbers whose exponents differ by at most p + 1 are exact at ARF_PREC_EXACT and fit in memory
       (arf.rst:92-112). */
    arf_add(m, lo, hi, p, ARF_RND_NEAR);
    arf_mul_2exp_si(m, m, -1);
    arf_sub(d1, hi, m, ARF_PREC_EXACT, ARF_RND_DOWN);
    arf_sub(d2, m, lo, ARF_PREC_EXACT, ARF_RND_DOWN);
    arf_max(d1, d1, d2);
    arf_get_mag(rho, d1);
    if (arf_cmpabs_mag(m, rho) <= 0)
    {
        /* B4: rho' >= (hi - lo)/2, m' = lo + rho' exactly (at most 2 p + 30 bits, E5) */
        arf_sub(d1, hi, lo, ARF_PREC_EXACT, ARF_RND_DOWN);
        arf_mul_2exp_si(d1, d1, -1);
        arf_get_mag(rho, d1);
        arf_set_mag(d2, rho);
        arf_add(m, lo, d2, ARF_PREC_EXACT, ARF_RND_DOWN);
    }
    if (sign < 0)
        arf_neg(m, m);
    arf_swap(arb_midref(z), m);
    mag_swap(arb_radref(z), rho);
    arf_clear(m);
    arf_clear(d1);
    arf_clear(d2);
    mag_clear(rho);
    if (!arb_is_finite(z) || !arb_is_nonzero(z) || arf_sgn(arb_midref(z)) != sign)
        adf_idele_kernel_defect("a ball without the required sign");
    return ADF_OK;
}

/* Statement F (api-2.md 2.3): lo = RD_p(l a / b), hi = RU_p(h a / b) for dyadic 0 < l <= h of at most p bits
   and integers a, b >= 1. l a is exact: at most p + bits(a) bits, so it fits in memory (arf.rst:91-112); the
   division by b is one correct rounding (arf.rst:672, :676-680, :24-39). */
void
adf_idele_ends_scale(arf_t lo, arf_t hi, const arf_t l, const arf_t h, const fmpz_t a, const fmpz_t b, slong p)
{
    arf_t t;
    arf_init(t);
    arf_mul_fmpz(t, l, a, ARF_PREC_EXACT, ARF_RND_DOWN);
    arf_div_fmpz(lo, t, b, p, ARF_RND_FLOOR);
    arf_mul_fmpz(t, h, a, ARF_PREC_EXACT, ARF_RND_DOWN);
    arf_div_fmpz(hi, t, b, p, ARF_RND_CEIL);
    arf_clear(t);
}

/* ---- life cycle ---- */

void
adf_idele_init(adf_idele_t x)
{
    arb_init(x->inf);
    arb_one(x->inf);
    fmpq_init(x->r);
    fmpq_one(x->r);
    adf_ucoset_init(&x->u);
}

void
adf_idele_clear(adf_idele_t x)
{
    arb_clear(x->inf);
    fmpq_clear(x->r);
    adf_ucoset_clear(&x->u);
}

void
adf_idele_set(adf_idele_t y, const adf_idele_t x)
{
    ID_INV(x);
    arb_set(y->inf, x->inf);
    fmpq_set(y->r, x->r);
    adf_ucoset_set(&y->u, &x->u);
}

void
adf_idele_swap(adf_idele_t x, adf_idele_t y)
{
    arb_swap(x->inf, y->inf);
    fmpq_swap(x->r, y->r);
    adf_ucoset_swap(&x->u, &y->u);
}

/* conventions 5.7: inf finite and not containing 0; r canonical (denominator > 0, gcd(num, den) = 1,
   fmpq.rst:87-90) and > 0; u canonical. The canonical form of r is tested here by its definition, so that a
   zero denominator gives 0 without any call of FLINT on it (and gcc 13 reports a false -Wstringop-overread
   for the inline fmpq_is_canonical at this place, as src/invariants.h notes for another predicate). */
int
adf_idele_is_canonical(const adf_idele_t x)
{
    fmpz_t g;
    int ok;

    if (!arb_is_finite(x->inf) || !arb_is_nonzero(x->inf))
        return 0;
    if (fmpz_sgn(fmpq_denref(x->r)) <= 0 || fmpz_sgn(fmpq_numref(x->r)) <= 0)
        return 0;
    fmpz_init(g);
    fmpz_gcd(g, fmpq_numref(x->r), fmpq_denref(x->r));
    ok = fmpz_is_one(g);
    fmpz_clear(g);
    return ok && adf_ucoset_is_canonical(&x->u);
}

int
adf_idele_identical(const adf_idele_t x, const adf_idele_t y)
{
    ID_INV(x);
    ID_INV(y);
    return arb_equal(x->inf, y->inf) && fmpq_equal(x->r, y->r) && adf_ucoset_identical(&x->u, &y->u);
}

/* ---- constructors ---- */

int
adf_idele_set_parts(adf_idele_t x, const arb_t inf, const fmpq_t r, const adf_ucoset_t u)
{
    fmpq_t t;
    int ok;

    ID_INV_UC(u);
    if (!arb_is_finite(inf) || !arb_is_nonzero(inf) || fmpz_is_zero(fmpq_denref(r)))
        return ADF_DOMAIN;
    fmpq_init(t);
    fmpq_set(t, r);
    fmpq_canonicalise(t);
    ok = fmpq_sgn(t) > 0;
    if (ok)
    {
        arb_set(x->inf, inf);
        fmpq_swap(x->r, t);
        adf_ucoset_set(&x->u, u);       /* the modulus as supplied (CV-17) */
    }
    fmpq_clear(t);
    return ok ? ADF_OK : ADF_DOMAIN;
}

/* The idele of q != 0 (conventions 5.7; ideles.md P3.4, line 80; api-2.md D.3): the real ball of kernel B on
   lo = RD_p(|q|), hi = RU_p(|q|) (E4), the content |q|, the exact unit [sign(q)]. */
int
adf_idele_set_rat(adf_idele_t x, const adf_rat_t q, slong prec)
{
    slong p = prec < 2 ? 2 : prec;
    arf_t lo, hi;
    fmpz_t n;
    arb_t t;
    int s;

    ADF_INV_RAT(q);
    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;                  /* before any allocation (the orchestrator, 2026-09-30) */
    if (fmpq_is_zero(q->q))
        return ADF_NOT_UNIT;
    s = fmpz_sgn(fmpq_numref(q->q));
    arf_init(lo);
    arf_init(hi);
    fmpz_init(n);
    arb_init(t);
    fmpz_abs(n, fmpq_numref(q->q));
    arf_fmpz_div_fmpz(lo, n, fmpq_denref(q->q), p, ARF_RND_FLOOR);
    arf_fmpz_div_fmpz(hi, n, fmpq_denref(q->q), p, ARF_RND_CEIL);
    if (adf_idele_ball_from_ends(t, lo, hi, s, p) != ADF_OK)
        adf_idele_kernel_defect("NOT_DETERMINED for a rational (impossible by Statement E4)");
    arb_swap(x->inf, t);
    fmpq_abs(x->r, q->q);
    if (s > 0)
        adf_ucoset_one(&x->u);
    else
        adf_ucoset_minus_one(&x->u);
    arf_clear(lo);
    arf_clear(hi);
    fmpz_clear(n);
    arb_clear(t);
    return ADF_OK;
}

/* ---- arithmetic ---- */

/* api-2.md D.1 with E2: lo = RD_p(l_x l_y), hi = RU_p(h_x h_y), the sign the product of the signs. The
   content and the unit are computed into temporaries, and z is written only on ADF_OK (conventions 4.3); z
   may be x or y. */
int
adf_idele_mul(adf_idele_t z, const adf_idele_t x, const adf_idele_t y, slong prec)
{
    slong p = prec < 2 ? 2 : prec;
    arf_t lx, hx, ly, hy, lo, hi;
    arb_t t;
    int sx, sy, st;

    ID_INV(x);
    ID_INV(y);
    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;
    arf_init(lx);
    arf_init(hx);
    arf_init(ly);
    arf_init(hy);
    arf_init(lo);
    arf_init(hi);
    arb_init(t);
    sx = adf_idele_ends_of_abs(lx, hx, x->inf, p);
    sy = adf_idele_ends_of_abs(ly, hy, y->inf, p);
    arf_mul(lo, lx, ly, p, ARF_RND_FLOOR);
    arf_mul(hi, hx, hy, p, ARF_RND_CEIL);
    st = adf_idele_ball_from_ends(t, lo, hi, sx * sy, p);
    if (st == ADF_OK)
    {
        fmpq_t r;
        adf_ucoset_t u;
        fmpq_init(r);
        adf_ucoset_init(u);
        fmpq_mul(r, x->r, y->r);
        adf_ucoset_mul(u, &x->u, &y->u);
        arb_swap(z->inf, t);
        fmpq_swap(z->r, r);
        adf_ucoset_swap(&z->u, u);
        fmpq_clear(r);
        adf_ucoset_clear(u);
    }
    arf_clear(lx);
    arf_clear(hx);
    arf_clear(ly);
    arf_clear(hy);
    arf_clear(lo);
    arf_clear(hi);
    arb_clear(t);
    return st;
}

/* api-2.md D.2 with E3: lo = RD_p(1/h_x), hi = RU_p(1/l_x), the sign of x. */
int
adf_idele_inv(adf_idele_t y, const adf_idele_t x, slong prec)
{
    slong p = prec < 2 ? 2 : prec;
    arf_t lx, hx, lo, hi;
    arb_t t;
    int sx, st;

    ID_INV(x);
    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;
    arf_init(lx);
    arf_init(hx);
    arf_init(lo);
    arf_init(hi);
    arb_init(t);
    sx = adf_idele_ends_of_abs(lx, hx, x->inf, p);
    arf_ui_div(lo, 1, hx, p, ARF_RND_FLOOR);
    arf_ui_div(hi, 1, lx, p, ARF_RND_CEIL);
    st = adf_idele_ball_from_ends(t, lo, hi, sx, p);
    if (st == ADF_OK)
    {
        fmpq_t r;
        adf_ucoset_t u;
        fmpq_init(r);
        adf_ucoset_init(u);
        fmpq_inv(r, x->r);
        adf_ucoset_inv(u, &x->u);
        arb_swap(y->inf, t);
        fmpq_swap(y->r, r);
        adf_ucoset_swap(&y->u, u);
        fmpq_clear(r);
        adf_ucoset_clear(u);
    }
    arf_clear(lx);
    arf_clear(hx);
    arf_clear(lo);
    arf_clear(hi);
    arb_clear(t);
    return st;
}

/* ---- slice 2 (lane i-slice2): an exact rational factor, valuations, absolute values, norm ---- */

/* Statement H (api-2.md 2.3): for x = (X, r, u) and q = n/d != 0, z = (Z, r |q|, u [sign q]) with Z kernel B
   on lo = RD_p(l_X |n| / d), hi = RU_p(h_X |n| / d) (Statement F; M1-D4: the integers of q) and the sign
   sign(X) sign(q). The unit is adf_ucoset_mul with the exact unit (ideles.md P10, api-2.md A.1), normal form.
   The limit first, then NOT_UNIT (conventions 3.3: the maximum), both before any allocation. */
int
adf_idele_mul_rat(adf_idele_t z, const adf_idele_t x, const adf_rat_t q, slong prec)
{
    slong p = prec < 2 ? 2 : prec;
    arf_t lx, hx, lo, hi;
    fmpz_t n;
    arb_t t;
    int sx, sq, st;

    ID_INV(x);
    ADF_INV_RAT(q);
    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;
    if (fmpq_is_zero(q->q))
        return ADF_NOT_UNIT;
    sq = fmpz_sgn(fmpq_numref(q->q));
    arf_init(lx);
    arf_init(hx);
    arf_init(lo);
    arf_init(hi);
    fmpz_init(n);
    arb_init(t);
    sx = adf_idele_ends_of_abs(lx, hx, x->inf, p);
    fmpz_abs(n, fmpq_numref(q->q));
    adf_idele_ends_scale(lo, hi, lx, hx, n, fmpq_denref(q->q), p);
    st = adf_idele_ball_from_ends(t, lo, hi, sx * sq, p);
    if (st == ADF_OK)
    {
        fmpq_t r;
        adf_ucoset_t e, u;
        fmpq_init(r);
        adf_ucoset_init(e);
        adf_ucoset_init(u);
        fmpq_abs(r, q->q);
        fmpq_mul(r, r, x->r);
        if (sq < 0)
            adf_ucoset_minus_one(e);
        adf_ucoset_mul(u, &x->u, e);
        arb_swap(z->inf, t);
        fmpq_swap(z->r, r);
        adf_ucoset_swap(&z->u, u);
        fmpq_clear(r);
        adf_ucoset_clear(e);
        adf_ucoset_clear(u);
    }
    arf_clear(lx);
    arf_clear(hx);
    arf_clear(lo);
    arf_clear(hi);
    fmpz_clear(n);
    arb_clear(t);
    return st;
}

/* v_p(r) = v_p(n) - v_p(d) by removal of p (ideles.md P14.1 and P14.4, lines 347, 351). fmpz_remove,
   refs/src/flint-3.0.1/fmpz.rst:1142-1149: "Remove all occurrences of the factor f > 1 from the integer op";
   its count is the return value, as for _fmpz_remove, fmpz.rst:1134, "Removes all factors f from x and
   returns the number of such" (the test checks it against one division at a time). n > 0 as r > 0.
   Returns 0 and leaves *v alone at the archimedean place. */
static int
valuation_of_content(slong * v, const adf_idele_t x, adf_place_t w)
{
    fmpz_t p, t;
    slong a, b;

    if (adf_place_is_archimedean(w))
        return 0;
    fmpz_init_set_ui(p, adf_place_prime_get(w));
    fmpz_init(t);
    a = fmpz_remove(t, fmpq_numref(x->r), p);
    b = fmpz_remove(t, fmpq_denref(x->r), p);
    fmpz_clear(p);
    fmpz_clear(t);
    *v = a - b;              /* no overflow: 0 <= a, b < 2^62 (each at most the bit length of an fmpz) */
    return 1;
}

int
adf_idele_valuation_at(slong * v, const adf_idele_t x, adf_place_t w)
{
    slong t;
    ID_INV(x);
    if (!valuation_of_content(&t, x, w))
        return ADF_DOMAIN;
    *v = t;
    return ADF_OK;
}

/* |x_p|_p = p^(-v) (ideles.md P14.1, line 347): 1/p^v for v >= 0, p^(-v) for v < 0; canonical, as p^k and 1
   are coprime. */
int
adf_idele_abs_at(adf_rat_t a, const adf_idele_t x, adf_place_t w)
{
    slong v;
    fmpz_t pk;

    ID_INV(x);
    if (!valuation_of_content(&v, x, w))
        return ADF_DOMAIN;
    fmpz_init_set_ui(pk, adf_place_prime_get(w));
    fmpz_pow_ui(pk, pk, (ulong) (v < 0 ? -v : v));
    if (v >= 0)
    {
        fmpz_one(fmpq_numref(a->q));
        fmpz_swap(fmpq_denref(a->q), pk);
    }
    else
    {
        fmpz_swap(fmpq_numref(a->q), pk);
        fmpz_one(fmpq_denref(a->q));
    }
    fmpz_clear(pk);
    return ADF_OK;
}

/* Statement I.3: |X| = [|m| +- rho] exactly. */
void
adf_idele_abs_inf(arb_t a, const adf_idele_t x)
{
    ID_INV(x);
    arf_abs(arb_midref(a), arb_midref(x->inf));
    mag_set(arb_radref(a), arb_radref(x->inf));
}

/* Statement I.4 with F: t = kernel B on lo = RD_p(l_X d / n), hi = RU_p(h_X d / n), r = n/d, sign +1; 1/r enters
   through its integers, exactly, before the rounding (PLAN 2.2). */
int
adf_idele_norm(arb_t t, const adf_idele_t x, slong prec)
{
    slong p = prec < 2 ? 2 : prec;
    arf_t lx, hx, lo, hi;
    arb_t b;
    int st;

    ID_INV(x);
    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;
    arf_init(lx);
    arf_init(hx);
    arf_init(lo);
    arf_init(hi);
    arb_init(b);
    adf_idele_ends_of_abs(lx, hx, x->inf, p);
    adf_idele_ends_scale(lo, hi, lx, hx, fmpq_denref(x->r), fmpq_numref(x->r), p);
    st = adf_idele_ball_from_ends(b, lo, hi, 1, p);
    if (st == ADF_OK)
        arb_swap(t, b);
    arf_clear(lx);
    arf_clear(hx);
    arf_clear(lo);
    arf_clear(hi);
    arb_clear(b);
    return st;
}

/* ---- accessors ---- */

void
adf_idele_get_real(arb_t r, const adf_idele_t x)
{
    ID_INV(x);
    arb_set(r, x->inf);
}

void
adf_idele_content(fmpq_t r, const adf_idele_t x)
{
    ID_INV(x);
    fmpq_set(r, x->r);
}

void
adf_idele_get_unit(adf_ucoset_t u, const adf_idele_t x)
{
    ID_INV(x);
    adf_ucoset_set(u, &x->u);
}
