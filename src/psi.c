/* Tate psi, slice 3.2-a. Ground truth: refs/src/tate-poonen/notes.txt:693-700;
   transform conjugation is separately fixed at :733-740. See conventions 6.1:844,876-887,
   docs/proofs/analysis.md Lemma 2:76-94, docs/api-3.md Q4 and 3.3. */
#include "adelefeld/psi.h"
#include "invariants.h"

static int psi_size(const fmpq_t q)
{
    return fmpz_bits(fmpq_numref(q)) <= ADF_QCLASS_BITS_MAX &&
           fmpz_bits(fmpq_denref(q)) <= ADF_QCLASS_BITS_MAX;
}
static void psi_mod1(fmpq_t q)
{
    fmpz_mod(fmpq_numref(q), fmpq_numref(q), fmpq_denref(q));
    fmpq_canonicalise(q);
}
/* The raw local H bounds the CRT representative. Check before CRT or gcd products. */
static int psi_finite(fmpq_t a, fmpq_t N, const adf_fball_t x)
{
    adf_rat_t c, n; int ok;
    if (fmpz_bits(x->A) > ADF_QCLASS_BITS_MAX ||
        fmpz_bits(x->H) > ADF_QCLASS_BITS_MAX ||
        fmpz_bits(x->d) > ADF_QCLASS_BITS_MAX) return 0;
    adf_rat_init(c); adf_rat_init(n);
    adf_fball_get_center(c, x); adf_fball_get_radius(n, x);
    ok = psi_size(c->q) && psi_size(n->q);
    if (ok) { fmpq_set(a, c->q); fmpq_set(N, n->q); }
    adf_rat_clear(c); adf_rat_clear(n); return ok;
}
/* analysis L2:78-91: the finite image is a singleton precisely for integer N. */
int adf_fball_psi_tate_phase(fmpq_t theta, const adf_fball_t x)
{
    fmpq_t a, N; int st = ADF_LIMIT;
    ADF_INV_FBALL(x); fmpq_init(a); fmpq_init(N);
    if (!psi_finite(a, N, x)) goto done;
    st = ADF_NOT_DETERMINED;
    if (!fmpz_is_one(fmpq_denref(N))) goto done;
    psi_mod1(a); fmpq_swap(theta, a); st = ADF_OK;
done:
    fmpq_clear(a); fmpq_clear(N); return st;
}
/* Before converting a stored dyadic, bound both its exponent and the projected fraction.
   refs/src/flint-3.0.1/arf.rst:310-315: conversion allocates according to the exponent. */
static int psi_arf_bound(arf_srcptr a)
{
    slong e, b;
    if (arf_is_zero(a)) return 1;
    if (fmpz_cmp_si(ARF_EXPREF(a), -ADF_QCLASS_EXP_MAX) < 0 ||
        fmpz_cmp_si(ARF_EXPREF(a), ADF_QCLASS_EXP_MAX) > 0) return 0;
    e = fmpz_get_si(ARF_EXPREF(a)); b = arf_bits(a);
    return b <= ADF_QCLASS_BITS_MAX && FLINT_MAX(e, b) <= ADF_QCLASS_BITS_MAX &&
           (e >= b ? 1 : b-e+1) <= ADF_QCLASS_BITS_MAX;
}
/* Conservative intermediate bounds BEFORE cross-products; equal denominators avoid a
   fictitious squared denominator. Zero operands need no product or carry. */
static int psi_sub(fmpq_t z, const fmpq_t x, const fmpq_t y)
{
    flint_bitcnt_t xn = fmpz_bits(fmpq_numref(x)), xd = fmpz_bits(fmpq_denref(x));
    flint_bitcnt_t yn = fmpz_bits(fmpq_numref(y)), yd = fmpz_bits(fmpq_denref(y));
    if (fmpq_is_zero(y)) { fmpq_set(z, x); return psi_size(z); }
    if (fmpq_is_zero(x)) { fmpq_neg(z, y); return psi_size(z); }
    if (fmpz_equal(fmpq_denref(x), fmpq_denref(y))) {
        if (FLINT_MAX(xn, yn)+1 > ADF_QCLASS_BITS_MAX) return 0;
    } else if (FLINT_MAX(xn+yd, yn+xd)+1 > ADF_QCLASS_BITS_MAX ||
               xd+yd > ADF_QCLASS_BITS_MAX) return 0;
    fmpq_sub(z, x, y); return psi_size(z);
}
/* Read the stored real ball exactly. Reduce large centres BEFORE numerical conversion.
   analysis L2:87-91 makes finite psi_f(a)=E(a), so b=(a-m) mod 1. */
static int psi_read(fmpq_t b, fmpq_t r, fmpq_t N, const adf_adele_t x)
{
    fmpq_t m, a; arf_t rad; int ok = 0;
    fmpq_init(m); fmpq_init(a); arf_init(rad);
    arf_set_mag(rad, arb_radref(x->inf));
    if (!psi_arf_bound(arb_midref(x->inf)) || !psi_arf_bound(rad)) goto done;
    if (!psi_finite(a, N, &x->fin)) goto done;
    arf_get_fmpq(m, arb_midref(x->inf)); arf_get_fmpq(r, rad);
    psi_mod1(a); psi_mod1(m);
    if (!psi_sub(b, a, m)) goto done;
    psi_mod1(b); ok = 1;
done:
    fmpq_clear(m); fmpq_clear(a); arf_clear(rad); return ok;
}
/* analysis L2:80 and api-3.md Q4 step 7. The image is singleton iff r=0 and B=1. */
int adf_adele_psi_tate_phase(fmpq_t theta, const adf_adele_t x)
{
    fmpq_t b, r, N; int st = ADF_LIMIT;
    ADF_INV_ADELE(x);
    if (!arb_is_finite(x->inf)) return ADF_DOMAIN;
    fmpq_init(b); fmpq_init(r); fmpq_init(N);
    if (!psi_read(b, r, N, x)) goto done;
    st = ADF_NOT_DETERMINED;
    if (!fmpq_is_zero(r) || !fmpz_is_one(fmpq_denref(N))) goto done;
    fmpq_swap(theta, b); st = ADF_OK;
done:
    fmpq_clear(b); fmpq_clear(r); fmpq_clear(N); return st;
}
/* Q1's second form, api-3.md 3.3 (repair R2). Exact dyadic endpoint arithmetic,
   unrestricted midpoint rounded to p bits; RU30 then one successor on nonzero radius.
   refs/src/flint-3.0.1/arf.rst:24-35,662-681 describes directed arf rounding;
   mag.rst:6-15 allows extra ulps in general conversions, hence direct mag construction. */
static void psi_round(arb_t z, const arf_t lo, const arf_t hi, slong p)
{
    arf_t m, r, t; fmpz_t man, exp; ulong v;
    arf_init(m); arf_init(r); arf_init(t); fmpz_init(man); fmpz_init(exp);
    arf_add(m, lo, hi, ARF_PREC_EXACT, ARF_RND_NEAR); arf_mul_2exp_si(m, m, -1);
    arf_set_round(arb_midref(z), m, p, ARF_RND_NEAR);
    arf_sub(r, arb_midref(z), lo, ARF_PREC_EXACT, ARF_RND_NEAR);
    arf_sub(t, hi, arb_midref(z), ARF_PREC_EXACT, ARF_RND_NEAR);
    if (arf_cmp(t, r) > 0) arf_set(r, t);
    if (arf_is_zero(r)) mag_zero(arb_radref(z));
    else {
        arf_set_round(r, r, 30, ARF_RND_CEIL);
        arf_get_fmpz_2exp(man, exp, r);
        fmpz_mul_2exp(man, man, 30-fmpz_bits(man));
        v = fmpz_get_ui(man)+1;
        fmpz_set(MAG_EXPREF(arb_radref(z)), ARF_EXPREF(r));
        if (v == (UWORD(1) << 30)) {
            v >>= 1; fmpz_add_ui(MAG_EXPREF(arb_radref(z)), MAG_EXPREF(arb_radref(z)), 1);
        }
        MAG_MAN(arb_radref(z)) = v;
    }
    arf_clear(m); arf_clear(r); arf_clear(t); fmpz_clear(man); fmpz_clear(exp);
}
/* Certified interval width <= epsilon. Clipping is safe since sine/cosine lie in [-1,1]. */
static int psi_certified(const arb_t x, slong p)
{
    return arb_is_finite(x) && mag_cmp_2exp_si(arb_radref(x), -p-1) <= 0;
}
static void psi_bounds(arf_t lo, arf_t hi, const arb_t x)
{
    arb_get_interval_arf(lo, hi, x, ARF_PREC_EXACT);
    if (arf_cmp_si(lo, -1) < 0) arf_set_si(lo, -1);
    if (arf_cmp_si(hi, 1) > 0) arf_one(hi);
}
/* api-3.md 3.2,3.3: rational argument reduction in arb preserves a small phase near a root.
   refs/src/flint-3.0.1/arb.rst:1125-1138. A raw theta is a canonical-input precondition. */
int adf_phase_get_acb(acb_t z, const fmpq_t theta, slong prec)
{
    slong p = FLINT_MAX(prec, 2), work; int st = ADF_NOT_DETERMINED;
    fmpq_t twice; acb_t out; arf_t lo, hi;
    if (prec > ADF_REAL_PREC_MAX) return ADF_LIMIT;
#ifdef ADF_CHECK_INVARIANTS
    if (!fmpq_is_canonical(theta) || fmpq_sgn(theta) < 0 || fmpq_cmp_si(theta, 1) >= 0)
        adf_inv_fail(__func__, "theta", "phase in [0,1)");
#endif
    if (!psi_size(theta)) return ADF_LIMIT;
    /* Exact cardinal phases are complex integers, independent of requested p. */
    if (fmpz_equal_ui(fmpq_denref(theta), 1)) { acb_one(z); return ADF_OK; }
    if (fmpz_equal_ui(fmpq_denref(theta), 2)) { acb_set_si(z, -1); return ADF_OK; }
    if (fmpz_equal_ui(fmpq_denref(theta), 4)) {
        acb_zero(z); arb_set_si(acb_imagref(z), fmpz_equal_ui(fmpq_numref(theta), 1) ? 1 : -1);
        return ADF_OK;
    }
    if (!fmpz_is_even(fmpq_denref(theta)) &&
        fmpz_bits(fmpq_numref(theta))+1 > ADF_QCLASS_BITS_MAX) return ADF_LIMIT;
    fmpq_init(twice); acb_init(out); arf_init(lo); arf_init(hi);
    fmpq_mul_2exp(twice, theta, 1);
    work = FLINT_MIN(p+32, ADF_REAL_PREC_MAX);
    for (;;) {
        arb_sin_cos_pi_fmpq(acb_imagref(out), acb_realref(out), twice, work);
        if (psi_certified(acb_realref(out), p) && psi_certified(acb_imagref(out), p)) {
            psi_bounds(lo, hi, acb_realref(out)); psi_round(acb_realref(out), lo, hi, p);
            psi_bounds(lo, hi, acb_imagref(out)); psi_round(acb_imagref(out), lo, hi, p);
            acb_swap(z, out); st = ADF_OK; break;
        }
        if (work == ADF_REAL_PREC_MAX) break;
        work = FLINT_MIN(2*work, ADF_REAL_PREC_MAX);
    }
    fmpq_clear(twice); acb_clear(out); arf_clear(lo); arf_clear(hi); return st;
}
/* Q4 d(t)=max(0,dist(B(t-b),Z)/B-r). Only four distances, never enumerate B roots.
   All modular operations are exact, so even a 2000-bit centre loses no small phase. */
static int psi_distance(fmpq_t d, const fmpq_t b, const fmpq_t r, const fmpz_t B, ulong quarter)
{
    fmpq_t t, u, one; int ok = 0;
    fmpq_init(t); fmpq_init(u); fmpq_init(one);
    fmpq_set_ui(t, quarter, 4); fmpq_one(one);
    if (!psi_sub(u, t, b)) goto done;
    psi_mod1(u);
    if (!fmpq_is_zero(u) && !fmpz_is_one(B)) {
        if (fmpz_bits(fmpq_numref(u))+fmpz_bits(B) > ADF_QCLASS_BITS_MAX) goto done;
        fmpq_mul_fmpz(u, u, B); psi_mod1(u);
    }
    if (!psi_sub(t, one, u)) goto done;
    if (fmpq_cmp(t, u) < 0) fmpq_set(u, t);
    if (!fmpq_is_zero(u) && !fmpz_is_one(B)) {
        if (fmpz_bits(fmpq_denref(u))+fmpz_bits(B) > ADF_QCLASS_BITS_MAX) goto done;
        fmpq_div_fmpz(u, u, B);
    }
    if (!psi_sub(d, u, r)) goto done;
    if (fmpq_sgn(d) < 0) fmpq_zero(d);
    ok = 1;
done:
    fmpq_clear(t); fmpq_clear(u); fmpq_clear(one); return ok;
}
/* Section 3.3 certificate: cosine interval width <=2^-p; exact cardinal distances.
   refs/src/flint-3.0.1/arb.rst:1125-1138 reduces the rational argument internally. */
static int psi_cos_upper(arf_t hi, const fmpq_t d, slong p)
{
    fmpq_t twice; arb_t c; arf_t lo; slong work; int st = ADF_NOT_DETERMINED;
    if (fmpq_is_zero(d)) { arf_one(hi); return ADF_OK; }
    if (fmpz_equal_ui(fmpq_denref(d), 4)) { arf_zero(hi); return ADF_OK; }
    if (fmpz_equal_ui(fmpq_denref(d), 2)) { arf_set_si(hi, -1); return ADF_OK; }
    if (!fmpz_is_even(fmpq_denref(d)) &&
        fmpz_bits(fmpq_numref(d))+1 > ADF_QCLASS_BITS_MAX) return ADF_LIMIT;
    fmpq_init(twice); arb_init(c); arf_init(lo);
    fmpq_mul_2exp(twice, d, 1); work = FLINT_MIN(p+32, ADF_REAL_PREC_MAX);
    for (;;) {
        arb_cos_pi_fmpq(c, twice, work);
        if (psi_certified(c, p)) { psi_bounds(lo, hi, c); st = ADF_OK; break; }
        if (work == ADF_REAL_PREC_MAX) break;
        work = FLINT_MIN(2*work, ADF_REAL_PREC_MAX);
    }
    fmpq_clear(twice); arb_clear(c); arf_clear(lo); return st;
}
/* analysis L2:78-91 supplies the image, Q4 proves its extrema; section 3.3 and
   Q1's second form enclose each extremum and bound the stored endpoint excess. */
static int psi_adele(acb_t z, const adf_adele_t x, slong prec, int strict)
{
    fmpq_t b, r, N, d[4]; arf_t upper[4], lo; acb_t out;
    const ulong targets[] = {0, 2, 1, 3};
    int st = ADF_LIMIT, j; slong p = FLINT_MAX(prec, 2);
    if (prec > ADF_REAL_PREC_MAX) return ADF_LIMIT;
    ADF_INV_ADELE(x);
    if (!arb_is_finite(x->inf)) return ADF_DOMAIN;
    fmpq_init(b); fmpq_init(r); fmpq_init(N); arf_init(lo); acb_init(out);
    for (j = 0; j < 4; j++) { fmpq_init(d[j]); arf_init(upper[j]); }
    if (!psi_read(b, r, N, x)) goto done;
    /* Exact work, including all projected distances, precedes strict ambiguity. */
    for (j = 0; j < 4; j++)
        if (!psi_distance(d[j], b, r, fmpq_denref(N), targets[j])) goto done;
    st = ADF_NOT_DETERMINED;
    if (strict && !fmpz_is_one(fmpq_denref(N))) goto done;
    for (j = 0; j < 4; j++) {
        st = psi_cos_upper(upper[j], d[j], p);
        if (st != ADF_OK) goto done;
    }
    arf_neg(lo, upper[1]); psi_round(acb_realref(out), lo, upper[0], p);
    arf_neg(lo, upper[3]); psi_round(acb_imagref(out), lo, upper[2], p);
    acb_swap(z, out); st = ADF_OK;
done:
    fmpq_clear(b); fmpq_clear(r); fmpq_clear(N); arf_clear(lo); acb_clear(out);
    for (j = 0; j < 4; j++) { fmpq_clear(d[j]); arf_clear(upper[j]); }
    return st;
}
int adf_adele_psi_tate(acb_t z, const adf_adele_t x, slong prec)
{
    if (prec > ADF_REAL_PREC_MAX) return ADF_LIMIT;
    ADF_INV_ADELE(x);
    return psi_adele(z, x, prec, 0);
}
/* CV-59, conventions:881-887: strict tests finite integrality, permits real uncertainty. */
int adf_adele_psi_tate_strict(acb_t z, const adf_adele_t x, slong prec)
{
    if (prec > ADF_REAL_PREC_MAX) return ADF_LIMIT;
    ADF_INV_ADELE(x);
    return psi_adele(z, x, prec, 1);
}
