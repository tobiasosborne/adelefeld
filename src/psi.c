/* Tate psi, slice 3.2-a. Ground truth: refs/src/tate-poonen/notes.txt:693-700;
   transform conjugation is separately fixed at :733-740. See conventions 6.1:844,876-887,
   docs/proofs/analysis.md Lemma 2:76-94, docs/api-3.md Q4 and 3.3. */
#include "adelefeld/psi.h"
#include "invariants.h"
#include <flint/ulong_extras.h>

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
static int psi_phase_entry(fmpq_t theta, const adf_adele_struct *x)
{
    fmpq_t b, r, N; int st = ADF_LIMIT;
    if (!arb_is_finite(x->inf)) return ADF_DOMAIN;
    fmpq_init(b); fmpq_init(r); fmpq_init(N);
    if (!psi_read(b, r, N, x)) goto done;
    st = ADF_NOT_DETERMINED;
    if (!fmpq_is_zero(r) || !fmpz_is_one(fmpq_denref(N))) goto done;
    fmpq_swap(theta, b); st = ADF_OK;
done:
    fmpq_clear(b); fmpq_clear(r); fmpq_clear(N); return st;
}
int adf_adele_psi_tate_phase(fmpq_t theta, const adf_adele_t x)
{
    ADF_INV_ADELE(x);
    return psi_phase_entry(theta, x);
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
/* Q4's four exact distances for the image (b, r, B), in the order t = 0, 1/2, 1/4, 3/4. */
static int psi_dists(fmpq *d, const fmpq_t b, const fmpq_t r, const fmpz_t B)
{
    const ulong targets[] = {0, 2, 1, 3};
    int j;
    for (j = 0; j < 4; j++)
        if (!psi_distance(d + j, b, r, B, targets[j])) return 0;
    return 1;
}
/* Exact preflight of one stored adele: DOMAIN for a raw non-finite real ball, LIMIT for D3-2 bounds,
   else OK with the four distances and B = denominator(N). */
static int psi_exact(fmpq *d, fmpz_t B, const adf_adele_struct *x)
{
    fmpq_t b, r, N; int st = ADF_LIMIT;
    if (!arb_is_finite(x->inf)) return ADF_DOMAIN;
    fmpq_init(b); fmpq_init(r); fmpq_init(N);
    if (psi_read(b, r, N, x) && psi_dists(d, b, r, fmpq_denref(N))) {
        fmpz_set(B, fmpq_denref(N)); st = ADF_OK;
    }
    fmpq_clear(b); fmpq_clear(r); fmpq_clear(N); return st;
}
/* Four certified upper bounds cos(2 pi d_j) + at most 2^-p (section 3.3). */
static int psi_numeric(arf_struct *upper, const fmpq *d, slong p)
{
    int j, st;
    for (j = 0; j < 4; j++)
        if ((st = psi_cos_upper(upper + j, d + j, p)) != ADF_OK) return st;
    return ADF_OK;
}
/* Fold one image's bounds into the coordinate extrema of a union: real [-u1,u0], imag [-u3,u2]
   (Q4); api-3.md 3.3: "take the minimum lower bound and maximum upper bound over constituents". */
static void psi_fold(arf_struct *ext, const arf_struct *upper, int first)
{
    arf_t lo; arf_init(lo);
    arf_neg(lo, upper + 1);
    if (first || arf_cmp(lo, ext + 0) < 0) arf_set(ext + 0, lo);
    if (first || arf_cmp(upper + 0, ext + 1) > 0) arf_set(ext + 1, upper + 0);
    arf_neg(lo, upper + 3);
    if (first || arf_cmp(lo, ext + 2) < 0) arf_set(ext + 2, lo);
    if (first || arf_cmp(upper + 2, ext + 3) > 0) arf_set(ext + 3, upper + 2);
    arf_clear(lo);
}
/* Q1's second form on both coordinates, then the single write of z. */
static void psi_commit(acb_t z, const arf_struct *ext, slong p)
{
    acb_t out; acb_init(out);
    psi_round(acb_realref(out), ext + 0, ext + 1, p);
    psi_round(acb_imagref(out), ext + 2, ext + 3, p);
    acb_swap(z, out); acb_clear(out);
}
#define PSI_TEMPS(d, upper, ext) \
    do { int j_; for (j_ = 0; j_ < 4; j_++) { fmpq_init(d + j_); arf_init(upper + j_); arf_init(ext + j_); } } \
    while (0)
#define PSI_CLEAR(d, upper, ext) \
    do { int j_; for (j_ = 0; j_ < 4; j_++) { fmpq_clear(d + j_); arf_clear(upper + j_); arf_clear(ext + j_); } } \
    while (0)
/* analysis L2:78-91 supplies the image, Q4 proves its extrema; section 3.3 and
   Q1's second form enclose each extremum and bound the stored endpoint excess. */
static int psi_adele(acb_t z, const adf_adele_t x, slong prec, int strict)
{
    fmpq d[4]; arf_struct upper[4], ext[4]; fmpz_t B;
    int st; slong p = FLINT_MAX(prec, 2);
    if (prec > ADF_REAL_PREC_MAX) return ADF_LIMIT;
    ADF_INV_ADELE(x);
    if (!arb_is_finite(x->inf)) return ADF_DOMAIN;
    PSI_TEMPS(d, upper, ext); fmpz_init(B);
    /* Exact work, including all projected distances, precedes strict ambiguity. */
    if ((st = psi_exact(d, B, x)) != ADF_OK) goto done;
    st = ADF_NOT_DETERMINED;
    if (strict && !fmpz_is_one(B)) goto done;
    if ((st = psi_numeric(upper, d, p)) != ADF_OK) goto done;
    psi_fold(ext, upper, 1); psi_commit(z, ext, p);
done:
    PSI_CLEAR(d, upper, ext); fmpz_clear(B);
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

/* ---- Slices 3.2-b and 3.2-c (lane q-slice5). docs/api-3.md 3.2:370-424, D3-3:809-817, Q4:591-635;
   statements docs/api-3b.md "Slices 3.2-b and 3.2-c". ---- */

#ifdef ADF_CHECK_INVARIANTS
#define PSI_INV_QCLASS(x) do { if (!adf_qclass_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_qclass"); } \
    while (0)
#define PSI_INV_LBALL(x) do { if (!adf_lball_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_lball"); } \
    while (0)
/* A valid handle is the archimedean place or a certified prime (place.h; as src/localfactor.c:25-28). */
#define PSI_INV_PLACE(v) do { if (!adf_place_is_archimedean(v) && !n_is_prime(adf_place_prime_get(v))) \
    adf_inv_fail(__func__, #v, "adf_place_t"); } while (0)
#else
#define PSI_INV_QCLASS(x) ((void) 0)
#define PSI_INV_LBALL(x) ((void) 0)
#define PSI_INV_PLACE(v) ((void) 0)
#endif

/* The class character (api-3.md 3.2:370-389). psi descends to A/Q (analysis L2 step 4:92-94), so the
   image of the class is the union over the stored entries of the image of each entry. Pass 1: exact
   preflight of EVERY entry (LIMIT is the largest status, so it is final at once). D3-3: strict is
   NOT_DETERMINED if any entry has B > 1. Pass 2: certified bounds of every entry, folded into the
   coordinate extrema of the union (api-3.md 3.3), then one rounding per coordinate and one write.
   Statuses combine by maximum: a numerical LIMIT (psi_cos_upper's projected doubling) needs a distance
   with an odd denominator, which no PIECES entry has (its finite centre is an integer, d = 1, H integral,
   its real midpoint and radius dyadic, so every distance is dyadic); for one entry the first failure is
   the maximum. Hence stopping at the first failure of pass 2 returns the maximum (docs/api-3b.md). */
static int psi_class(acb_t z, const adf_qclass_t x, slong prec, int strict)
{
    fmpq d[4]; arf_struct upper[4], ext[4]; fmpz_t B;
    int st = ADF_OK, s, frac = 0; slong i, p = FLINT_MAX(prec, 2);
    if (prec > ADF_REAL_PREC_MAX) return ADF_LIMIT;
    PSI_INV_QCLASS(x);
    PSI_TEMPS(d, upper, ext); fmpz_init(B);
    for (i = 0; i < x->len; i++) {
        s = psi_exact(d, B, x->piece + i);
        if (s > st) st = s;
        if (st == ADF_LIMIT) goto done;
        if (s == ADF_OK && !fmpz_is_one(B)) frac = 1;
    }
    if (st != ADF_OK) goto done;
    if (strict && frac) { st = ADF_NOT_DETERMINED; goto done; }
    for (i = 0; i < x->len; i++) {
        (void) psi_exact(d, B, x->piece + i);
        if ((st = psi_numeric(upper, d, p)) != ADF_OK) goto done;
        psi_fold(ext, upper, i == 0);
    }
    psi_commit(z, ext, p);
done:
    PSI_CLEAR(d, upper, ext); fmpz_clear(B);
    return st;
}
int adf_qclass_psi_tate(acb_t z, const adf_qclass_t x, slong prec)
{
    return psi_class(z, x, prec, 0);
}
/* D3-3 (api-3.md:809-817): the strict finite-radius certificate of CV-59 on each stored entry. */
int adf_qclass_psi_tate_strict(acb_t z, const adf_qclass_t x, slong prec)
{
    return psi_class(z, x, prec, 1);
}
/* Q4 step 7: a union is a singleton iff every entry is and all agree. LIMIT first (maximum). */
int adf_qclass_psi_tate_phase(fmpq_t theta, const adf_qclass_t x)
{
    fmpq_t first, t; int st = ADF_OK, s, have = 0, agree = 1; slong i;
    PSI_INV_QCLASS(x);
    fmpq_init(first); fmpq_init(t);
    for (i = 0; i < x->len; i++) {
        s = psi_phase_entry(t, x->piece + i);
        if (s > st) st = s;
        if (st == ADF_LIMIT) break;
        if (s != ADF_OK) continue;
        if (!have) { fmpq_swap(first, t); have = 1; }
        else if (!fmpq_equal(first, t)) agree = 0;
    }
    if (st == ADF_OK && !agree) st = ADF_NOT_DETERMINED;
    if (st == ADF_OK) fmpq_swap(theta, first);
    fmpq_clear(first); fmpq_clear(t);
    return st;
}

/* P = p^k if it has at most ADF_QCLASS_BITS_MAX bits, else 0 without forming it when the lower bound
   k (bits(p)-1)+1 <= bits(p^k) (from p >= 2^(bits(p)-1)) already exceeds the bound. A formed power has at
   most k bits(p) <= 2 ADF_QCLASS_BITS_MAX + 64 bits. api-3.md 3.2:395 "Check power bit sizes before
   forming p^k". */
static int psi_ppow(fmpz_t P, ulong p, ulong k)
{
    ulong bp = FLINT_BIT_COUNT(p);
    if (k == 0) { fmpz_one(P); return 1; }
    if (k >= (ulong) ADF_QCLASS_BITS_MAX || (bp-1)*k+1 > (ulong) ADF_QCLASS_BITS_MAX) return 0;
    fmpz_set_ui(P, p); fmpz_pow_ui(P, P, k);
    return fmpz_bits(P) <= (flint_bitcnt_t) ADF_QCLASS_BITS_MAX;
}
/* fp_p(n/(M dd)) with M = p^h exactly the p-part of the denominator and p not dividing dd:
   (n dd^(-1) mod M)/M (api-3.md Q4 step 8:625-628, design 3.2:432-436). M = 1 gives 0. */
static void psi_fp(fmpq_t b, const fmpz_t n, const fmpz_t dd, const fmpz_t M)
{
    fmpz_t t;
    if (fmpz_is_one(M)) { fmpq_zero(b); return; }
    fmpz_init(t);
    if (!fmpz_invmod(t, dd, M)) flint_abort();     /* p does not divide dd: unreachable */
    fmpz_mul(t, t, n); fmpz_mod(t, t, M);
    fmpq_set_fmpz_frac(b, t, M);
    fmpz_clear(t);
}
/* The local image at p of the lball x = p^v u (+ p^N Z_p): base b = fp_p(p^v u) and order B = p^(-N) for a
   ball with N < 0, else 1 (Q4 step 8; analysis L2:85-86, conventions 6.1:840-844). The canonical centre lies
   in the ball, so its fp_p is a member of the image. 1 on success, 0 for LIMIT. */
static int psi_local_read(fmpq_t b, fmpz_t B, const adf_lball_t x)
{
    fmpz_t M; int ok = 0;
    if (x->v > ADF_LBALL_EXP_MAX || x->v < -ADF_LBALL_EXP_MAX ||
        x->N > ADF_LBALL_EXP_MAX || x->N < -ADF_LBALL_EXP_MAX) return 0;
    fmpz_init(M);
    if (x->v >= 0) fmpq_zero(b);               /* u is p-integral: fp_p = 0, no power formed */
    else {
        if (!psi_ppow(M, x->p, (ulong) -x->v)) goto done;
        psi_fp(b, fmpq_numref(x->u), fmpq_denref(x->u), M);
    }
    if (!x->exact && x->N < 0) { if (!psi_ppow(B, x->p, (ulong) -x->N)) goto done; }
    else fmpz_one(B);
    ok = 1;
done:
    fmpz_clear(M); return ok;
}
/* One image (b, r, B): exact distances (LIMIT), strict B > 1 (NOT_DETERMINED), numerics, one write. */
static int psi_image(acb_t z, const fmpq_t b, const fmpq_t r, const fmpz_t B, slong p, int strict)
{
    fmpq d[4]; arf_struct upper[4], ext[4]; int st = ADF_LIMIT;
    PSI_TEMPS(d, upper, ext);
    if (!psi_dists(d, b, r, B)) goto done;
    st = ADF_NOT_DETERMINED;
    if (strict && !fmpz_is_one(B)) goto done;
    if ((st = psi_numeric(upper, d, p)) != ADF_OK) goto done;
    psi_fold(ext, upper, 1); psi_commit(z, ext, p);
done:
    PSI_CLEAR(d, upper, ext);
    return st;
}
static int psi_lball(acb_t z, const adf_lball_t x, slong prec, int strict)
{
    fmpq_t b, r; fmpz_t B; int st = ADF_LIMIT;
    if (prec > ADF_REAL_PREC_MAX) return ADF_LIMIT;
    PSI_INV_LBALL(x);
    fmpq_init(b); fmpq_init(r); fmpz_init(B);
    if (psi_local_read(b, B, x)) st = psi_image(z, b, r, B, FLINT_MAX(prec, 2), strict);
    fmpq_clear(b); fmpq_clear(r); fmpz_clear(B);
    return st;
}
int adf_lball_psi_tate(acb_t z, const adf_lball_t x, slong prec)
{
    return psi_lball(z, x, prec, 0);
}
int adf_lball_psi_tate_strict(acb_t z, const adf_lball_t x, slong prec)
{
    return psi_lball(z, x, prec, 1);
}
/* Q4 steps 7-8: a singleton iff exact or N >= 0. */
int adf_lball_psi_tate_phase(fmpq_t theta, const adf_lball_t x)
{
    fmpq_t b; fmpz_t B; int st = ADF_LIMIT;
    PSI_INV_LBALL(x);
    fmpq_init(b); fmpz_init(B);
    if (psi_local_read(b, B, x)) {
        st = ADF_NOT_DETERMINED;
        if (fmpz_is_one(B)) { fmpq_swap(theta, b); st = ADF_OK; }
    }
    fmpq_clear(b); fmpz_clear(B);
    return st;
}

static int psi_fail(int st, adf_place_t *where, adf_place_t v)
{
    if (where != NULL) *where = v;
    return st;
}
/* psi_v on the projection of x (api-3.md 3.2:400-414). At infinity psi_inf(t) = E(-t) on [m-r, m+r]
   (conventions 6.1:844; refs/src/tate-poonen/notes.txt:693-694): base (-m) mod 1, radius r, B = 1.
   At p the projection of a + N Zhat is a + p^(v_p(N)) Z_p (lball.h adf_lball_set_fball, api-1f.md L1),
   psi_p = E(fp_p) (notes.txt:695-700): base fp_p(a), B = p^(-v_p(N)) if v_p(N) < 0, else 1. The powers of p
   are removed from den(a) and den(N), which psi_finite has bounded: M and B divide them. */
static int psi_at(acb_t z, adf_place_t *where, const adf_adele_t x, adf_place_t v, slong prec, int strict)
{
    fmpq_t a, N, b, r; fmpz_t B, M, dd; arf_t t; int st = ADF_LIMIT;
    if (prec > ADF_REAL_PREC_MAX) return psi_fail(ADF_LIMIT, where, v);
    ADF_INV_ADELE(x); PSI_INV_PLACE(v);
    if (!arb_is_finite(x->inf)) return psi_fail(ADF_DOMAIN, where, v);
    fmpq_init(a); fmpq_init(N); fmpq_init(b); fmpq_init(r);
    fmpz_init(B); fmpz_init(M); fmpz_init(dd); arf_init(t);
    fmpz_one(B);
    if (adf_place_is_archimedean(v)) {
        arf_set_mag(t, arb_radref(x->inf));
        if (!psi_arf_bound(arb_midref(x->inf)) || !psi_arf_bound(t)) goto done;
        arf_get_fmpq(b, arb_midref(x->inf)); arf_get_fmpq(r, t);
        fmpq_neg(b, b); psi_mod1(b);
        /* CV-59: strict tests the finite radius only; the real factor has none, and B = 1 here. */
    } else {
        fmpz_t pp; fmpz_init(pp); fmpz_set_ui(pp, adf_place_prime_get(v));
        if (!psi_finite(a, N, &x->fin)) { fmpz_clear(pp); goto done; }
        (void) fmpz_remove(dd, fmpq_denref(a), pp);
        fmpz_divexact(M, fmpq_denref(a), dd);
        psi_fp(b, fmpq_numref(a), dd, M);
        if (!fmpq_is_zero(N)) {
            (void) fmpz_remove(dd, fmpq_denref(N), pp);
            fmpz_divexact(B, fmpq_denref(N), dd);
        }
        fmpz_clear(pp);
    }
    st = psi_image(z, b, r, B, FLINT_MAX(prec, 2), strict);
done:
    fmpq_clear(a); fmpq_clear(N); fmpq_clear(b); fmpq_clear(r);
    fmpz_clear(B); fmpz_clear(M); fmpz_clear(dd); arf_clear(t);
    return st == ADF_OK ? ADF_OK : psi_fail(st, where, v);
}
int adf_adele_psi_tate_at(acb_t z, adf_place_t *where, const adf_adele_t x, adf_place_t v, slong prec)
{
    return psi_at(z, where, x, v, prec, 0);
}
int adf_adele_psi_tate_strict_at(acb_t z, adf_place_t *where, const adf_adele_t x, adf_place_t v, slong prec)
{
    return psi_at(z, where, x, v, prec, 1);
}
