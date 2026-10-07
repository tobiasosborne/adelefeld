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
   Gamma(z) directly; if that is not finite and n <= ADF_LOCAL_ZETA_SHIFT_MAX, Gamma(z + n) / (z (z+1) ...
   (z+n-1)), which is Gamma(z) by the recurrence (Z2 step 4, :75-77; notes.txt:58); n > the bound is LIMIT.
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
    if (!acb_is_finite(g))
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
            acb_clear(f);
            acb_add_ui(g, z, (ulong) n, w);
            acb_gamma(g, g, w);
            acb_div(g, g, q, w);
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
                      const arb_t logpi, slong n,
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
            real_derivative_bound(B, oldB, z, lo, hi, logpi, n, w);
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
