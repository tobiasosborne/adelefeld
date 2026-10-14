/* localfactor.c: the local zeta factor of the trivial character at one place (WP 1F.9; lane f-slice14;
   include/adelefeld/localfactor.h; decision N-D20).

   The procedure is the decision rule Z4 of docs/design/local-zeta.md:107-170, with its proof of soundness at
   :151-170; the statements are docs/api-1f9.md Y16 (value and certificate) and Y17 (statuses, outputs,
   aliasing, limits, cost). Contract: docs/SPEC.md 9.3.7 (line 703); docs/conventions.md:944-951 (the pole
   rule), :223 (statuses), 4.4 (a non-finite result is never stored, CV-08). Sources: the finite factor
   refs/src/tate-poonen/notes.txt:1733, the real factor :1014-1016, Gamma's poles and absence of zeros :62-64.

   FLINT 3.0.1 functions used (refs/src/flint-3.0.1/): acb.rst: acb_set_round 130, acb_add_error_mag 144,
   acb_get_mid 151, acb_is_finite 224, acb_contains_zero 306, acb_inv 509, acb_div 521, acb_exp 658,
   acb_expm1 671, acb_rising_ui 871, acb_gamma 893; arf.rst: arf_floor 301, arf_cmp_si 332, arf_sub 574;
   mag.rst: mag_hypot 399, mag_expinv 442, mag_expm1 450, mag_mul_lower 299.

   Every computation is made in temporaries; y is written once, by a swap, after the status is OK, so y may be
   s and y is untouched on every other status. */

#include <adelefeld.h>
#include <adelefeld/localfactor.h>
#include <flint/arb_hypgeom.h>
#include <flint/ulong_extras.h>
#include "invariants.h"

#ifdef ADF_CHECK_INVARIANTS
/* A valid handle is the archimedean place or a certified prime (place.h; place.c). A forged handle is a
   precondition violation (conventions 4.4). */
#define ADF_INV_PLACE(v) do { if (!adf_place_is_archimedean(v) && !n_is_prime(adf_place_prime_get(v))) \
    adf_inv_fail(__func__, #v, "adf_place_t"); } while (0)
#else
#define ADF_INV_PLACE(v) ((void) 0)
#endif

static int
fail(int st, adf_place_t *where, adf_place_t v)
{
    if (where != NULL) *where = v;
    return st;
}

/* Z4, finite place procedure (local-zeta.md:113-128), with w = prec + 32 working bits and prec >= 2.
   Writes res only on OK. */
static int
finite_factor(acb_t res, const acb_t s, ulong p, slong prec)
{
    slong w = prec + 32;
    arb_t a, b;
    acb_t m, bm, t0, d0, direct, out;
    mag_t R, al, xl, t, e1, e2, E;
    int neg, st = ADF_NOT_DETERMINED;

    /* Step 1: the exact zero is the only exact dyadic pole (Z3, :86-105). */
    if (acb_is_zero(s)) return ADF_DOMAIN;

    arb_init(a); arb_init(b);
    acb_init(m); acb_init(bm); acb_init(t0); acb_init(d0); acb_init(direct); acb_init(out);
    mag_init(R); mag_init(al); mag_init(xl); mag_init(t); mag_init(e1); mag_init(e2); mag_init(E);

    /* Step 2: the stable sign from the exact midpoint: b = -log p if Re(m) >= 0, b = log p otherwise, so that
       |exp(b m)| = exp(-log p |Re m|) <= 1. T0 encloses exp(b m), D0 encloses 1 - exp(b m) = -expm1(b m),
       intersected with 1 - T0 (both enclose the same number; the direct one is kept if expm1 is not finite).
       The uncertainty of log p is in the arb a and enters both calls. */
    neg = arf_sgn(arb_midref(acb_realref(s))) < 0;
    arb_log_ui(a, p, w);
    if (neg) arb_set(b, a);
    else arb_neg(b, a);
    acb_get_mid(m, s);
    acb_mul_arb(bm, m, b, w);
    acb_exp(t0, bm, w);
    acb_expm1(d0, bm, w);
    acb_neg(d0, d0);
    acb_neg(direct, t0);
    acb_add_ui(direct, direct, 1, w);
    if (acb_is_finite(d0))
    {
        arb_t c;
        arb_init(c);
        if (arb_intersection(c, acb_realref(d0), acb_realref(direct), w)) arb_swap(acb_realref(d0), c);
        if (arb_intersection(c, acb_imagref(d0), acb_imagref(direct), w)) arb_swap(acb_imagref(d0), c);
        arb_clear(c);
    }
    else
        acb_set(d0, direct);

    /* Step 3: R+ >= R = sqrt(rx^2 + ry^2) (the disk cover radius, not max(rx, ry): :40-41), and
       E+ >= exp(-a |x|) (exp(a R+) - 1): a lower bound of a |x| in the first factor, upper bounds of a and R
       in the second. Proof step 2 (:154-157): |exp(b t) - exp(b m)| <= E for every t of the rectangle; adding
       E+ to both component radii encloses the disk of possible changes. */
    mag_hypot(R, arb_radref(acb_realref(s)), arb_radref(acb_imagref(s)));
    arb_get_mag_lower(al, a);
    arf_get_mag_lower(xl, arb_midref(acb_realref(s)));
    mag_mul_lower(t, al, xl);
    mag_expinv(e1, t);
    arb_get_mag(t, a);
    mag_mul(t, t, R);
    mag_expm1(e2, t);
    mag_mul(E, e1, e2);

    /* Step 4: a non-finite bound, or a denominator that contains 0, is NOT_DETERMINED. If the rectangle
       contains a pole s0, exp(b s0) = 1 and 0 lies in D (proof step 3, :158-159). */
    if (!mag_is_finite(E) || !acb_is_finite(t0) || !acb_is_finite(d0)) goto done;
    acb_add_error_mag(t0, E);
    acb_add_error_mag(d0, E);
    if (acb_contains_zero(d0)) goto done;

    /* Step 5: Y = 1/D for b = -log p; Y = -T/D for b = log p, since -exp(a t)/(1 - exp(a t)) =
       1/(1 - exp(-a t)) (proof step 4, :160-161). Finite check, outward rounding to prec, finite check. */
    if (neg)
    {
        acb_div(out, t0, d0, w);
        acb_neg(out, out);
    }
    else
        acb_inv(out, d0, w);
    if (!acb_is_finite(out)) goto done;
    acb_set_round(out, out, prec);
    if (!acb_is_finite(out)) goto done;
    acb_swap(res, out);
    st = ADF_OK;
done:
    arb_clear(a); arb_clear(b);
    acb_clear(m); acb_clear(bm); acb_clear(t0); acb_clear(d0); acb_clear(direct); acb_clear(out);
    mag_clear(R); mag_clear(al); mag_clear(xl); mag_clear(t); mag_clear(e1); mag_clear(e2); mag_clear(E);
    return st;
}

/* The candidate of Z4 real steps 3-4 (:137-143) on z = s/2 at w bits, with the prefactor exp(-z log pi):
   Gamma(z) directly (refs/src/flint-3.0.1/acb.rst:893-896). For 0 < n <= ADF_LOCAL_ZETA_SHIFT_MAX also
   intersect with Gamma(z + n) / (z (z+1) ... (z+n-1)); this sharpens finite but loose rectangles near poles.
   The identity follows by iterating Gamma(z+1) = z Gamma(z) (refs/src/tate-poonen/notes.txt:61).
   A failed direct value uses the recurrence fallback; only then does n > the bound give LIMIT.
   n is the caller's shift with min Re(z + n) >= 1. Returns OK with a finite y, NOT_DETERMINED or LIMIT. */
static int
real_candidate(acb_t y, const acb_t z, const arb_t logpi, slong n, slong w)
{
    acb_t pref, g, q;
    int st = ADF_OK;
    acb_init(pref);
    acb_init(g);
    acb_init(q);
    acb_mul_arb(pref, z, logpi, w);
    acb_neg(pref, pref);
    acb_exp(pref, pref, w);
    acb_gamma(g, z, w);
    if (!acb_is_finite(g) || (n > 0 && n <= ADF_LOCAL_ZETA_SHIFT_MAX))
    {
        if (n > ADF_LOCAL_ZETA_SHIFT_MAX) st = ADF_LIMIT;
        else
        {
            /* Q = z (z+1) ... (z+n-1) as a loop of products, as the design states it (:142). Not acb_rising_ui:
               on FLINT 3.0.1 it expands the product (rectangular splitting) and, for z = -2 +/- 0.05 +
               i (0.8 +/- 0.05), n = 6, returns a ball of radius about 110 that contains 0 (probe
               lanes/f-slice14/probes/rec_probe.c), where the loop gives a ball that excludes 0. */
            acb_t f;
            acb_init(f);
            acb_one(q);
            for (slong j = 0; j < n; j++)
            {
                acb_add_ui(f, z, (ulong) j, w);
                acb_mul(q, q, f, w);
            }
            acb_add_ui(f, z, (ulong) n, w);
            acb_gamma(f, f, w);
            /* Pole geometry was checked before this call. A zero-containing product can still be an
               interval overestimate: retain a finite direct candidate, otherwise refuse certification. */
            if (acb_contains_zero(q)) acb_indeterminate(f);
            else acb_div(f, f, q, w);
            if (!acb_is_finite(g)) acb_set(g, f);
            else if (acb_is_finite(f))
            {
                arb_t c;
                arb_init(c);
                if (arb_intersection(c, acb_realref(g), acb_realref(f), w)) arb_swap(acb_realref(g), c);
                if (arb_intersection(c, acb_imagref(g), acb_imagref(f), w)) arb_swap(acb_imagref(g), c);
                arb_clear(c);
            }
            acb_clear(f);
        }
    }
    if (st == ADF_OK)
    {
        acb_mul(g, g, pref, w);
        if (acb_is_finite(g)) acb_swap(y, g);
        else st = ADF_NOT_DETERMINED;
    }
    acb_clear(pref);
    acb_clear(g);
    acb_clear(q);
    return st;
}

/* The shift n >= 0 with lo + n >= 1 for a lower bound lo of min Re(z): n = 1 - floor(lo) for lo < 1, else 0.
   Any lo below -ADF_LOCAL_ZETA_SHIFT_MAX gives ADF_LOCAL_ZETA_SHIFT_MAX + 1 (only "too many" matters). */
static slong
shift_of(const arf_t lo)
{
    arf_t f;
    slong n;
    if (arf_cmp_si(lo, 1) >= 0) return 0;
    if (arf_cmp_si(lo, -ADF_LOCAL_ZETA_SHIFT_MAX) < 0) return ADF_LOCAL_ZETA_SHIFT_MAX + 1;
    arf_init(f);
    arf_floor(f, lo);
    n = 1 - arf_get_si(f, ARF_RND_FLOOR);
    arf_clear(f);
    return n;
}

/* Z6 (local-zeta.md:212-240), sharpened by docs/api-1f9.md Y16's numbered weighted-integral proof:
   an upper bound B of |L_inf'| on s, z = s/2, lo <= min Re(z), hi >= max Re(z),
   1 <= lo + n, hi + n <= 64. B = pi^(-min Re(s)/2) [M1 + M0 sum 1/delta_j] / (2 P).
   A = min Re(z + n) >= lo + n, M0 = 1/A + (ceil(B0) - 1)!, M1 = 1/A^2 + ceil(B0)!,
   B0 = max Re(z + n) <= hi + n, delta_j = dist(z, -j) >= the hypotenuse of the two component distances, P their
   product. Every quantity is rounded in the direction that enlarges B: lower bounds for A, delta_j and P, upper
   bounds for the rest. B is infinite if a delta_j bound is 0. */
static void
real_derivative_bound(mag_t B, mag_t previous, const acb_t z, const arf_t lo, const arf_t hi,
                      const arb_t logpi, const acb_t candidate, slong n,
                      slong w)
{
    arf_t t;
    arb_t c;
    mag_t inva, m0, m1, f, sum, oldsum, lp, P, dx, dy, d;
    slong cb, j;
    arf_init(t);
    arb_init(c);
    mag_init(inva); mag_init(m0); mag_init(m1); mag_init(f); mag_init(sum); mag_init(P);
    mag_init(oldsum); mag_init(lp);
    mag_init(dx); mag_init(dy); mag_init(d);

    arf_add_si(t, lo, n, w, ARF_RND_FLOOR);         /* A >= t >= 1 */
    arf_get_mag_lower(f, t);
    mag_inv(inva, f);                                /* >= 1/A */
    arf_add_si(t, hi, n, w, ARF_RND_CEIL);           /* B0 <= t <= 64 */
    arf_ceil(t, t);
    cb = arf_get_si(t, ARF_RND_CEIL);                /* ceil(B0) <= cb, 1 <= cb <= 64 */
    mag_fac_ui(f, (ulong) (cb - 1));
    mag_add(m0, inva, f);
    mag_mul(m1, inva, inva);
    mag_fac_ui(f, (ulong) cb);
    mag_add(m1, m1, f);

    mag_one(P);
    mag_zero(sum);
    arb_get_mag_lower(dy, acb_imagref(z));           /* distance of Im(z) from 0 */
    for (j = 0; j < n; j++)
    {
        arb_add_si(c, acb_realref(z), j, w);
        arb_get_mag_lower(dx, c);                    /* distance of Re(z) from -j */
        mag_mul_lower(d, dx, dx);
        mag_mul_lower(f, dy, dy);
        mag_add_lower(d, d, f);
        mag_sqrt_lower(d, d);
        mag_mul_lower(P, P, d);
        mag_inv(f, d);                               /* infinite for d = 0 */
        mag_add(sum, sum, f);
    }
    /* Retain the previous certificate only to guard the final representation's containment.
       Outward midpoint/radius rounding of nested intersections need not itself be nested (Y16). */
    mag_set(oldsum, sum);
    arb_get_mag(lp, logpi);
    mag_add(oldsum, oldsum, lp);
    mag_mul(oldsum, oldsum, m0);
    mag_add(oldsum, oldsum, m1);
    mag_div(oldsum, oldsum, P);
    mag_mul_2exp_si(oldsum, oldsum, -1);
    /* Y16: M1 already bounds |Gamma'(z+n) - log(pi) Gamma(z+n)|. */
    mag_mul(sum, sum, m0);
    mag_add(sum, sum, m1);
    mag_div(sum, sum, P);
    mag_mul_2exp_si(sum, sum, -1);
    /* pi^(-min Re(s)/2) = exp(-min Re(z) log pi) <= exp(-lo log pi) */
    arb_set_arf(c, lo);
    arb_mul(c, c, logpi, w);
    arb_neg(c, c);
    arb_exp(c, c, w);
    arb_get_mag(f, c);
    mag_mul(B, sum, f);
    mag_mul(previous, oldsum, f);

    /* On this pole-free rectangle L'(s) = L(s) (digamma(s/2) - log(pi)) / 2.
       candidate encloses L on the whole rectangle, so its magnitude times that of the second factor
       bounds the derivative. Take the smaller of two certified bounds; a nonfinite digamma
       leaves Z6's bound unchanged. refs/src/flint-3.0.1/acb.rst:918-920 defines digamma. */
    {
        acb_t d;
        mag_t image, sharp;
        acb_init(d); mag_init(image); mag_init(sharp);
        acb_digamma(d, z, w);
        acb_sub_arb(d, d, logpi, w);
        acb_get_mag(sharp, d); acb_get_mag(image, candidate);
        mag_mul(sharp, sharp, image); mag_mul_2exp_si(sharp, sharp, -1);
        mag_min(B, B, sharp);
        acb_clear(d); mag_clear(image); mag_clear(sharp);
    }

    arf_clear(t);
    arb_clear(c);
    mag_clear(inva); mag_clear(m0); mag_clear(m1); mag_clear(f); mag_clear(sum); mag_clear(P);
    mag_clear(oldsum); mag_clear(lp);
    mag_clear(dx); mag_clear(dy); mag_clear(d);
}

/* Z4, real place procedure (local-zeta.md:130-149), w = prec + 32 working bits. Writes res only on OK. */
static int
real_factor(acb_t res, const acb_t s, slong prec)
{
    slong w = prec + 32, n;
    const arf_struct *x = arb_midref(acb_realref(s));
    arf_t lo, hi, h;
    arb_t logpi;
    acb_t z, cand, zm, ym, previous, oldym;
    mag_t R, B, oldB;
    int st, have_previous = 0;

    /* Step 1: an exact non-positive even integer with imaginary part exactly 0 is an exact pole (Z2). */
    if (acb_is_exact(s) && arb_is_zero(acb_imagref(s)) && arf_sgn(x) <= 0 && arf_is_int_2exp_si(x, 1))
        return ADF_DOMAIN;

    arf_init(lo); arf_init(hi); arf_init(h);
    arb_init(logpi);
    acb_init(z); acb_init(cand); acb_init(zm); acb_init(ym);
    acb_init(previous); acb_init(oldym);
    mag_init(R); mag_init(B); mag_init(oldB);

    /* Step 2: the closed rectangle s/2 against the non-positive integers, with outward endpoints of Re(s/2)
       at w bits (lo rounded down, hi rounded up; halving is exact). The poles are real, so an imaginary interval
       that excludes 0 excludes them. Otherwise [lo, min(hi, 0)] contains an integer iff floor(min(hi, 0)) >= lo
       (endpoints count: closed). Rounding can only enlarge the interval, so a refusal may be caused by it, an
       acceptance never is. arf_floor and arf_cmp work for every exponent (arf.rst:39). */
    arb_get_lbound_arf(lo, acb_realref(s), w);
    arb_get_ubound_arf(hi, acb_realref(s), w);
    arf_mul_2exp_si(lo, lo, -1);
    arf_mul_2exp_si(hi, hi, -1);
    if (arb_contains_zero(acb_imagref(s)) && arf_sgn(lo) <= 0)
    {
        if (arf_sgn(hi) > 0) arf_zero(h);
        else arf_floor(h, hi);
        if (arf_cmp(h, lo) >= 0)
        {
            st = ADF_NOT_DETERMINED;
            goto done;
        }
    }

    /* Steps 3-4: the candidate; n from the outward lower end. */
    arb_const_pi(logpi, w);
    arb_log(logpi, logpi, w);
    acb_mul_2exp_si(z, s, -1);
    n = shift_of(lo);
    st = real_candidate(cand, z, logpi, n, w);
    if (st != ADF_OK) goto done;

    /* Step 5: the midpoint refinement for a ball of positive radius, n <= 64 and max Re(z + n) <= 64. The
       midpoint lies in the pole-free rectangle; its enclosure enlarged by R B (Z6 step 1: |L(t) - L(m)| <= R B
       on the convex rectangle) contains every value, as the candidate does; so does their intersection (proof
       step 5, :165-166). A non-finite midpoint value or bound leaves the candidate as it is. */
    arf_add_si(h, hi, n, w, ARF_RND_CEIL);
    if (!acb_is_exact(s) && n <= ADF_LOCAL_ZETA_SHIFT_MAX && arf_cmp_si(h, 64) <= 0)
    {
        arf_t ml;
        arf_init(ml);
        acb_get_mid(zm, z);
        arf_set(ml, arb_midref(acb_realref(zm)));
        if (real_candidate(ym, zm, logpi, shift_of(ml), w) == ADF_OK)
        {
            mag_hypot(R, arb_radref(acb_realref(s)), arb_radref(acb_imagref(s)));
            real_derivative_bound(B, oldB, z, lo, hi, logpi, cand, n, w);
            mag_mul(B, B, R);
            if (mag_is_finite(B))
            {
                arb_t c;
                arb_init(c);
                acb_set(previous, cand);
                acb_set(oldym, ym);
                mag_mul(oldB, oldB, R);
                if (mag_is_finite(oldB))
                {
                    acb_add_error_mag(oldym, oldB);
                    if (arb_intersection(c, acb_realref(previous), acb_realref(oldym), w))
                        arb_swap(acb_realref(previous), c);
                    if (arb_intersection(c, acb_imagref(previous), acb_imagref(oldym), w))
                        arb_swap(acb_imagref(previous), c);
                }
                have_previous = 1;
                acb_add_error_mag(ym, B);
                if (arb_intersection(c, acb_realref(cand), acb_realref(ym), w)) arb_swap(acb_realref(cand), c);
                if (arb_intersection(c, acb_imagref(cand), acb_imagref(ym), w)) arb_swap(acb_imagref(cand), c);
                arb_clear(c);
            }
        }
        arf_clear(ml);
    }

    /* Outward rounding to prec, finite check (twice, :144). */
    acb_set_round(cand, cand, prec);
    if (have_previous)
    {
        acb_set_round(previous, previous, prec);
        if (!acb_contains(previous, cand)) acb_set(cand, previous);
    }
    if (!acb_is_finite(cand)) st = ADF_NOT_DETERMINED;
    else acb_swap(res, cand);
done:
    arf_clear(lo); arf_clear(hi); arf_clear(h);
    arb_clear(logpi);
    acb_clear(z); acb_clear(cand); acb_clear(zm); acb_clear(ym);
    acb_clear(previous); acb_clear(oldym);
    mag_clear(R); mag_clear(B); mag_clear(oldB);
    return st;
}

/* docs/api-1f9.md Y16, Y17. */
int
adf_local_zeta_factor_at(acb_t y, adf_place_t *where, const acb_t s, adf_place_t v, slong prec)
{
    acb_t t;
    int st;

    /* LIMIT from prec alone, before every other check (localfactor.h; rfunc.h). */
    if (prec > ADF_REAL_PREC_MAX) return fail(ADF_LIMIT, where, v);
    ADF_INV_PLACE(v);
    if (prec < 2) prec = 2;
    /* A non-finite raw input is DOMAIN (conventions 4.4, a courtesy to raw acb callers). */
    if (!acb_is_finite(s)) return fail(ADF_DOMAIN, where, v);

    acb_init(t);
    if (adf_place_is_archimedean(v)) st = real_factor(t, s, prec);
    else st = finite_factor(t, s, adf_place_prime_get(v), prec);
    if (st == ADF_OK) acb_swap(y, t);
    acb_clear(t);
    return st == ADF_OK ? ADF_OK : fail(st, where, v);
}

/* Slice 5a: api-5.md:57-108; analysis.md P9:354-365 (integral, not gamma).
   Recognize exact finite poles s=k, alpha=p^k. The optional exact power is
   bounded by D2's 2^20 integer bits; no conversion of an unbounded s to slong.
   Negative powers are dyadic only at p=2, since raw exact acb values are dyadic. */
static int
tate_exact_finite_pole(const acb_t s, const acb_t alpha, ulong p)
{
    slong k, bound = 1048576 / FLINT_BIT_COUNT(p);
    fmpz_t power;
    int pole;
    if (!acb_is_exact(s) || !acb_is_exact(alpha) ||
        !arb_is_zero(acb_imagref(s)) || !arb_is_zero(acb_imagref(alpha)) ||
        !arf_is_int(arb_midref(acb_realref(s)))) return 0;
    if (arf_cmp_si(arb_midref(acb_realref(s)), -bound) < 0 ||
        arf_cmp_si(arb_midref(acb_realref(s)), bound) > 0) return 0;
    k = arf_get_si(arb_midref(acb_realref(s)), ARF_RND_NEAR);
    if (k < 0)
        return p == 2 && arf_cmp_2exp_si(arb_midref(acb_realref(alpha)), k) == 0;
    fmpz_init(power);
    fmpz_set_ui(power, p);
    fmpz_pow_ui(power, power, (ulong) k);
    pole = arb_contains_fmpz(acb_realref(alpha), power);
    fmpz_clear(power);
    return pole;
}

/* General geometric factor. Direct ball exp/multiply/subtract encloses the
   whole rectangle (acb.rst:6-17,658-669). Zero exclusion certifies inversion;
   failed exclusion is never evidence of an exact pole. No phase or G_minus
   occurs: that is the transformed vector's gamma factor (P9:343-347). */
static int
tate_finite_factor(acb_t z, const acb_t s, ulong p, const acb_t alpha, slong prec)
{
    acb_t t, denominator;
    arb_t logp;
    slong w = prec > ADF_REAL_PREC_MAX-32 ? ADF_REAL_PREC_MAX : prec+32;
    int st = ADF_NOT_DETERMINED;
    if (tate_exact_finite_pole(s, alpha, p)) return ADF_DOMAIN;
    acb_init(t); acb_init(denominator); arb_init(logp);
    arb_log_ui(logp, p, w);
    acb_mul_arb(t, s, logp, w);
    acb_neg(t, t);
    acb_exp(t, t, w);
    acb_mul(t, alpha, t, w);
    acb_neg(denominator, t);
    acb_add_ui(denominator, denominator, 1, w);
    if (!acb_is_finite(denominator) || acb_contains_zero(denominator)) goto done;
    acb_inv(t, denominator, w);
    if (!acb_is_finite(t)) goto done;
    acb_set_round(t, t, prec);
    if (!acb_is_finite(t)) goto done;
    acb_swap(z, t);
    st = ADF_OK;
done:
    acb_clear(t); acb_clear(denominator); arb_clear(logp);
    return st;
}

/* Exact odd real poles can be much larger than the working precision. Test
   the original dyadic integer's lowest bit before rounding the shift s+1. */
static int
tate_exact_odd_pole(const acb_t s)
{
    fmpz_t bottom;
    int pole;
    if (!acb_is_exact(s) || !arb_is_zero(acb_imagref(s)) ||
        !arf_is_int(arb_midref(acb_realref(s))) ||
        arf_sgn(arb_midref(acb_realref(s))) >= 0) return 0;
    fmpz_init(bottom);
    arf_bot(bottom, arb_midref(acb_realref(s)));
    pole = fmpz_is_zero(bottom);
    fmpz_clear(bottom);
    return pole;
}

/* Standard local integral, api-5.md section 2; analysis.md P9:331-400,
   P10:401-451, P11:452-495; conventions 6.4/6.5. In particular P9 step 1
   cancels the inverse unit character exactly and gives unit volume 1.
   At infinity reuse local-zeta.md Z4 and its 64-factor recurrence certificate.
   Gamma poles: refs/src/tate-poonen/notes.txt:62-64; evaluation contract:
   refs/src/flint-3.0.1/acb.rst:893-902. No character group setup outside INV. */
int
adf_local_tate_at(acb_t z, adf_place_t *where, const acb_t s, adf_place_t v,
                  const acb_t alpha, const adf_char_t eta0, slong prec)
{
    acb_t t, shifted;
    ulong q, p;
    int st;
    /* HEADER-FINDING: api-5:33 caps guard bits, while :80 requires exact
       delegation to the existing factor, whose contract uses prec+32 even
       at the cap. Preserve delegation; clip guards only on the new path. */
    if (prec > ADF_REAL_PREC_MAX) return fail(ADF_LIMIT, where, v);
    if (eta0->q > ADF_CHAR_MOD_MAX) return fail(ADF_LIMIT, where, v);
    ADF_INV_PLACE(v);
#ifdef ADF_CHECK_INVARIANTS
    if (!adf_char_is_canonical(eta0)) adf_inv_fail(__func__, "eta0", "adf_char");
    if (z == eta0->s || s == eta0->s || alpha == eta0->s)
        adf_inv_fail(__func__, "raw arguments", "independent acb input/output");
#endif
    if (prec < 2) prec = 2;
    if (!acb_is_finite(s)) return fail(ADF_DOMAIN, where, v);
    acb_init(t); acb_init(shifted);
    if (adf_place_is_archimedean(v))
    {
        slong w = prec > ADF_REAL_PREC_MAX-32 ? ADF_REAL_PREC_MAX : prec+32;
        if (eta0->parity && tate_exact_odd_pole(s)) st = ADF_DOMAIN;
        else if (!eta0->parity)
            st = adf_local_zeta_factor_at(t, NULL, s, v, prec);
        else
        {
            acb_add_ui(shifted, s, 1, w);
            st = adf_local_zeta_factor_at(t, NULL, shifted, v, prec);
        }
    }
    else
    {
        p = adf_place_prime_get(v);
        q = eta0->q;
        if (q == 0 || p < 2) { st = ADF_DOMAIN; goto done; }
        while (q > 1 && q % p == 0) q /= p;
        if (q != 1) { st = ADF_DOMAIN; goto done; }
        if (!acb_is_finite(alpha) || acb_is_zero(alpha)) { st = ADF_DOMAIN; goto done; }
        if (acb_contains_zero(alpha)) { st = ADF_NOT_DETERMINED; goto done; }
        if (eta0->q > 1)
        {
            acb_one(t);
            st = ADF_OK;
        }
        else if (acb_is_one(alpha))
            st = adf_local_zeta_factor_at(t, NULL, s, v, prec);
        else
            st = tate_finite_factor(t, s, p, alpha, prec);
    }
done:
    if (st == ADF_OK) acb_swap(z, t);
    acb_clear(t); acb_clear(shifted);
    return st == ADF_OK ? ADF_OK : fail(st, where, v);
}

/* Slice 5b: the local gamma and epsilon factors, docs/api-5.md:48-119 (design section 2), :84-98 (poles and
   certificates); docs/proofs/analysis.md P9:331-400, P10:401-451; conventions 6.4; statements docs/api-5a.md
   "Slice 5b". The entry checks, the conductor validation and the alpha checks are those of adf_local_tate_at
   above, in the same order (api-5.md:26-37; api-5a.md "Status, atomicity and resources"). */
#ifdef ADF_CHECK_INVARIANTS
#define ADF_INV_CONSTANT(z, s, alpha, eta0) do { \
    if (!adf_char_is_canonical(eta0)) adf_inv_fail(__func__, "eta0", "adf_char"); \
    if ((z) == (eta0)->s || (s) == (eta0)->s || (alpha) == (eta0)->s) \
        adf_inv_fail(__func__, "raw arguments", "independent acb input/output"); } while (0)
#else
#define ADF_INV_CONSTANT(z, s, alpha, eta0) ((void) 0)
#endif

/* The guard policy of the new finite path of slice 5a: prec + extra bits, clipped to the precision cap. */
static slong
constant_work(slong prec, slong extra)
{
    return prec > ADF_REAL_PREC_MAX - extra ? ADF_REAL_PREC_MAX : prec + extra;
}

/* At p: the conductor exponent a of eta0 by exact word divisions (conductor 1 or p^a, else DOMAIN; a canonical
   character is primitive, so p = 2 never has a = 1), then alpha: nonfinite or exact 0 DOMAIN, a rectangle
   containing 0 NOT_DETERMINED. The same checks and order as adf_local_tate_at. */
static int
constant_prime(ulong *a, ulong p, const acb_t alpha, const adf_char_t eta0)
{
    ulong q = eta0->q, k = 0;
    if (q == 0 || p < 2) return ADF_DOMAIN;
    while (q > 1 && q % p == 0) { q /= p; k++; }
    if (q != 1) return ADF_DOMAIN;
    if (!acb_is_finite(alpha) || acb_is_zero(alpha)) return ADF_DOMAIN;
    if (acb_contains_zero(alpha)) return ADF_NOT_DETERMINED;
    *a = k;
    return ADF_OK;
}

/* P9 (analysis.md:344), a = 0: gamma_p = (1 - alpha T)/(1 - alpha^-1 p^(s-1)), T = exp(-s log p), the real log.
   Multiplying numerator and denominator by alpha != 0 (certified by the caller for every alpha of the rectangle)
   gives alpha (1 - alpha T)/(alpha - p^(s-1)): the same function, whose denominator vanishes exactly at gamma's
   own poles alpha^-1 p^(s-1) = 1 (api-5.md:84-86). Each ball operation encloses all values on the rectangles
   (acb.rst:6-17; exp :658-660), so the quotient encloses gamma once the denominator excludes 0 (api-5.md:90-93:
   "enclose it directly"; the optional midpoint refinement is not used). Exact poles: s = k exact, alpha = p^(k-1)
   exactly, the dual of 5a's test (api-5.md:94-95), by 5a's recognizer applied to the exact s - 1. A computed
   zero-containing denominator is NOT_DETERMINED, never DOMAIN. The integral's poles alpha T = 1 are zeros of the
   numerator; numerator and denominator never vanish together (their exponential products equal 1/p). */
static int
gamma_unramified(acb_t res, const acb_t s, ulong p, const acb_t alpha, slong prec)
{
    slong w = constant_work(prec, 32);
    acb_t s1, t, num, den;
    arb_t logp;
    int st = ADF_NOT_DETERMINED;
    acb_init(s1);
    /* s - 1 is exact whenever s is an exact integer inside the recognizer's bound (at most 22 bits < w) */
    acb_sub_ui(s1, s, 1, w);
    if (tate_exact_finite_pole(s1, alpha, p)) { acb_clear(s1); return ADF_DOMAIN; }
    acb_init(t); acb_init(num); acb_init(den); arb_init(logp);
    arb_log_ui(logp, p, w);
    acb_mul_arb(t, s, logp, w);
    acb_neg(t, t);
    acb_exp(t, t, w);                       /* T = exp(-s log p) */
    acb_mul(num, alpha, t, w);
    acb_neg(num, num);
    acb_add_ui(num, num, 1, w);             /* 1 - alpha T */
    acb_mul_arb(t, s1, logp, w);
    acb_exp(t, t, w);                       /* p^(s-1) = exp((s-1) log p) */
    acb_sub(den, alpha, t, w);              /* alpha - p^(s-1) */
    if (!acb_is_finite(den) || acb_contains_zero(den)) goto done;
    acb_mul(num, num, alpha, w);
    acb_div(num, num, den, w);
    if (!acb_is_finite(num)) goto done;
    acb_set_round(num, num, prec);
    if (!acb_is_finite(num)) goto done;
    acb_swap(res, num);
    st = ADF_OK;
done:
    acb_clear(s1); acb_clear(t); acb_clear(num); acb_clear(den); arb_clear(logp);
    return st;
}

/* G_minus(eta0^-1) = sum_(u mod q, p not | u) eta0(u)^-1 E(-u/q), q = p^a (P9:345-346; conventions 6.4: the
   NEGATIVE kernel, not tau). As api-5.md:66-69 prescribes: the exact phase theta of eta0(u) from
   adf_char_chi_phase, negated, plus -u/q, reduced into [0, 1) exactly; adf_phase_get_acb encloses E of it;
   acb_add sums with outward rounding (arb.rst:6-12). The positive Gauss sum is not called. A character setup
   failure (UNSUPPORTED) leaves the value uncertified: NOT_DETERMINED (api-5.md:37-38). */
static int
gminus(acb_t g, const adf_char_t eta0, ulong p, slong w)
{
    fmpz_t u;
    fmpq_t theta, add;
    acb_t term, sum;
    int zero, st = ADF_OK;
    fmpz_init(u); fmpq_init(theta); fmpq_init(add); acb_init(term); acb_init(sum);
    for (ulong m = 1; m < eta0->q && st == ADF_OK; m++)
    {
        if (m % p == 0) continue;
        fmpz_set_ui(u, m);
        st = adf_char_chi_phase(&zero, theta, eta0, u);
        if (st != ADF_OK || zero) { st = st == ADF_LIMIT ? ADF_LIMIT : ADF_NOT_DETERMINED; break; }
        fmpq_set_ui(add, m, eta0->q);
        fmpq_add(theta, theta, add);
        fmpq_neg(theta, theta);              /* in (-2, 0] */
        while (fmpq_sgn(theta) < 0) fmpq_add_ui(theta, theta, 1);
        st = adf_phase_get_acb(term, theta, w);
        if (st != ADF_OK) { st = st == ADF_LIMIT ? ADF_LIMIT : ADF_NOT_DETERMINED; break; }
        acb_add(sum, sum, term, w);
    }
    if (st == ADF_OK) acb_swap(g, sum);
    fmpz_clear(u); fmpq_clear(theta); fmpq_clear(add); acb_clear(term); acb_clear(sum);
    return st;
}

/* P9 (analysis.md:347), a > 0: gamma_p = alpha^a T^a G_minus(eta0^-1), T^a = exp(-a s log p). Entire in s: no
   pole test. acb_pow_ui and acb_exp enclose on the rectangles; the product of enclosures encloses. */
static int
gamma_ramified(acb_t res, const acb_t s, ulong p, ulong a, const acb_t alpha, const adf_char_t eta0, slong prec)
{
    slong w = constant_work(prec, 32 + FLINT_BIT_COUNT(eta0->q));
    acb_t g, t, out;
    arb_t logp;
    int st;
    acb_init(g); acb_init(t); acb_init(out); arb_init(logp);
    st = gminus(g, eta0, p, w);
    if (st == ADF_OK)
    {
        arb_log_ui(logp, p, w);
        acb_mul_arb(t, s, logp, w);
        acb_mul_ui(t, t, a, w);
        acb_neg(t, t);
        acb_exp(t, t, w);                   /* T^a = exp(-a s log p) */
        acb_pow_ui(out, alpha, a, w);       /* alpha^a */
        acb_mul(out, out, t, w);
        acb_mul(out, out, g, w);
        acb_set_round(out, out, prec);
        if (!acb_is_finite(out)) st = ADF_NOT_DETERMINED;
        else acb_swap(res, out);
    }
    acb_clear(g); acb_clear(t); acb_clear(out); arb_clear(logp);
    return st;
}

/* Whether s is an exact real integer; then *odd is its parity, read from the bottom bit of the dyadic integer
   (arf_bot), so that no large integer is rounded or converted (as tate_exact_odd_pole above). */
static int
exact_integer_parity(int *odd, const acb_t s)
{
    const arf_struct *x = arb_midref(acb_realref(s));
    fmpz_t bottom;
    if (!acb_is_exact(s) || !arb_is_zero(acb_imagref(s)) || !arf_is_int(x)) return 0;
    if (arf_is_zero(x)) { *odd = 0; return 1; }
    fmpz_init(bottom);
    arf_bot(bottom, x);
    *odd = fmpz_is_zero(bottom);
    fmpz_clear(bottom);
    return 1;
}

/* rgamma(z) on the rectangle z. acb_rgamma alone is a valid but loose enclosure near the poles of Gamma: for
   z = -2 +/- 2^-12 (both coordinates) FLINT 3.0.1 returns radius 0.038 where the variation is about 7e-4 (probe in
   lanes/t5-slice3/redgreen.md). Gamma(z + n) = z (z+1) ... (z+n-1) Gamma(z) (notes.txt:61, iterated) gives the
   identity of entire functions rgamma(z) = z (z+1) ... (z+n-1) rgamma(z + n). With n from the outward lower end
   of Re(z) as in Z4 (shift_of: min Re(z + n) >= 1), the product of balls, a loop as real_candidate's, times
   acb_rgamma at z + n encloses rgamma(z); no division occurs. For n > ADF_LOCAL_ZETA_SHIFT_MAX the direct
   acb_rgamma is used (still an enclosure: acb.rst:898-902); there is no LIMIT, rgamma being entire. */
static void
rgamma_shifted(acb_t y, const acb_t z, slong w)
{
    arf_t lo;
    slong n;
    acb_t q, f;
    arf_init(lo); acb_init(q); acb_init(f);
    arb_get_lbound_arf(lo, acb_realref(z), w);
    n = shift_of(lo);
    if (n > ADF_LOCAL_ZETA_SHIFT_MAX) n = 0;
    acb_one(q);
    for (slong j = 0; j < n; j++)
    {
        acb_add_ui(f, z, (ulong) j, w);
        acb_mul(q, q, f, w);
    }
    acb_add_ui(f, z, (ulong) n, w);
    acb_rgamma(f, f, w);
    acb_mul(y, q, f, w);
    arf_clear(lo); acb_clear(q); acb_clear(f);
}

/* P10 (analysis.md:406; api-5.md:78): gamma_inf = i^e pi^(s-1/2) Gamma((1-s+e)/2) rgamma((s+e)/2). With
   w = (1-s+e)/2 and z = (s+e)/2, -w + z = s - 1/2, so gamma_inf = i^e [pi^-w Gamma(w)] [pi^z rgamma(z)].
   The first factor is the local zeta factor at 1-s+e (localfactor.h, design local-zeta.md Z4): its exact pole
   test, integer geometry against 1-s+e = 0, -2, ... (that is s = 1+e+2k, gamma's own poles), Gamma certificate
   and 64-factor recurrence bound are inherited. The second factor is entire: rgamma_shifted (acb_rgamma "avoiding
   division by zero at the poles of the gamma function", acb.rst:898-902) and acb_exp enclose on the rectangle; no
   division by a nonfinite Gamma. Gamma has no zeros (notes.txt:62-64), so the zeros are exactly z = -k, that is
   s = -e-2k, and the poles exactly w = -k, s = 1+e+2k; the two sets are disjoint (1+2e+2k+2j is odd). An exact
   integer s is tested first, on the original dyadic (no rounding of a large s): a pole is DOMAIN, a zero is the
   exact 0. */
static int
gamma_real(acb_t res, const acb_t s, int e, slong prec)
{
    slong w = constant_work(prec, 32);
    acb_t num, den, t;
    arb_t logpi;
    int odd, st;
    const arf_struct *x = arb_midref(acb_realref(s));
    if (exact_integer_parity(&odd, s))
    {
        if (odd != e && arf_cmp_si(x, 1 + e) >= 0) return ADF_DOMAIN;   /* s = 1+e+2k */
        if (odd == e && arf_cmp_si(x, -e) <= 0) { acb_zero(res); return ADF_OK; }   /* s = -e-2k */
    }
    acb_init(num); acb_init(den); acb_init(t); arb_init(logpi);
    acb_neg(num, s);
    acb_add_ui(num, num, (ulong) (1 + e), w);       /* 1 - s + e */
    /* HEADER-FINDING (as slice 5a's): api-5.md:33 counts guards against the precision cap, while the delegated
       local zeta factor works at prec + 32 even at the cap (localfactor.h). The delegation is kept. */
    st = adf_local_zeta_factor_at(num, NULL, num, adf_place_inf(), prec);
    if (st != ADF_OK) goto done;
    acb_add_ui(den, s, (ulong) e, w);
    acb_mul_2exp_si(den, den, -1);                  /* z = (s + e)/2 */
    arb_const_pi(logpi, w);
    arb_log(logpi, logpi, w);
    acb_mul_arb(t, den, logpi, w);
    acb_exp(t, t, w);                               /* pi^z */
    rgamma_shifted(den, den, w);
    acb_mul(den, den, t, w);
    acb_mul(num, num, den, w);
    if (e) acb_mul_onei(num, num);
    acb_set_round(num, num, prec);
    if (!acb_is_finite(num)) st = ADF_NOT_DETERMINED;
    else acb_swap(res, num);
done:
    acb_clear(num); acb_clear(den); acb_clear(t); arb_clear(logpi);
    return st;
}

/* api-5.md:109-113; analysis.md P9:340-348, P10:406. */
int
adf_local_gamma_at(acb_t z, adf_place_t *where, const acb_t s, adf_place_t v,
                   const acb_t alpha, const adf_char_t eta0, slong prec)
{
    acb_t t;
    ulong a = 0;
    int st;
    if (prec > ADF_REAL_PREC_MAX) return fail(ADF_LIMIT, where, v);
    if (eta0->q > ADF_CHAR_MOD_MAX) return fail(ADF_LIMIT, where, v);
    ADF_INV_PLACE(v);
    ADF_INV_CONSTANT(z, s, alpha, eta0);
    if (prec < 2) prec = 2;
    if (!acb_is_finite(s)) return fail(ADF_DOMAIN, where, v);
    acb_init(t);
    if (adf_place_is_archimedean(v))
        st = gamma_real(t, s, eta0->parity, prec);
    else
    {
        ulong p = adf_place_prime_get(v);
        st = constant_prime(&a, p, alpha, eta0);
        if (st == ADF_OK)
            st = a == 0 ? gamma_unramified(t, s, p, alpha, prec) : gamma_ramified(t, s, p, a, alpha, eta0, prec);
    }
    if (st == ADF_OK) acb_swap(z, t);
    acb_clear(t);
    return st == ADF_OK ? ADF_OK : fail(st, where, v);
}

/* api-5.md:114-118; analysis.md P9:349-351, P10:410. */
int
adf_local_epsilon_at(acb_t z, adf_place_t *where, const acb_t s, adf_place_t v,
                     const acb_t alpha, const adf_char_t eta0, slong prec)
{
    acb_t t;
    ulong a = 0;
    int st;
    if (prec > ADF_REAL_PREC_MAX) return fail(ADF_LIMIT, where, v);
    if (eta0->q > ADF_CHAR_MOD_MAX) return fail(ADF_LIMIT, where, v);
    ADF_INV_PLACE(v);
    ADF_INV_CONSTANT(z, s, alpha, eta0);
    if (prec < 2) prec = 2;
    if (!acb_is_finite(s)) return fail(ADF_DOMAIN, where, v);
    acb_init(t);
    if (adf_place_is_archimedean(v))
    {
        /* epsilon_inf = i^e exactly, entire in s; alpha ignored (P10:410) */
        if (eta0->parity) acb_onei(t);
        else acb_one(t);
        st = ADF_OK;
    }
    else
    {
        ulong p = adf_place_prime_get(v);
        st = constant_prime(&a, p, alpha, eta0);
        /* a = 0: epsilon = gamma L_p(s)/L_p(1-s) = 1 exactly in its simplified entire form (P9:349-351), also at
           gamma's poles; no factor is divided. a > 0: epsilon = gamma, which is entire. */
        if (st == ADF_OK)
        {
            if (a == 0) acb_one(t);
            else st = gamma_ramified(t, s, p, a, alpha, eta0, prec);
        }
    }
    if (st == ADF_OK) acb_swap(z, t);
    acb_clear(t);
    return st == ADF_OK ? ADF_OK : fail(st, where, v);
}
