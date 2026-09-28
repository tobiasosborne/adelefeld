/* tests/test_recon_limit.c: adf_adele_reconstruct on real balls of extreme binary exponent.

   Contract: include/adelefeld/recon.h, decision M1-D3 (docs/SPEC.md section 15, row M1-D3):
   adf_adele_reconstruct returns ADF_LIMIT when the midpoint or the radius of the arb is not
   zero and its binary exponent (arf.h, ARF_EXP; mag.h, MAG_EXP) is above ADF_RECON_EXP_MAX =
   2^20 in absolute value, the test being made before any integer of that size is built; on
   every status other than ADF_OK the output is untouched (conventions 4.3). The set statement
   and the three statuses are those of conventions 6.8 (CV-51, D3) and docs/proofs/quotient.md
   Proposition 11, tested in tests/test_recon.c; this file is about the boundary of the limit
   and about the balls of the review of reviewer `arith` (docs/reviews/m1/arith/review.md, R1
   and R2), for which the answers of the set statement are ADF_NO_SOLUTION and ADF_OK and
   where the function answered ADF_OK with a value outside the ball, or aborted in GMP.

   The exponents of the test balls are the ones the guard reads, so every builder checks the
   exponent it produced. The representation: the arf value v is stored with the exponent e of
   v = m 2^e, 0.5 <= |m| < 1 (refs/src/flint-3.0.1/arf.rst:14-20: "the unique integer e such
   that x 2^y = m 2^e where 0.5 <= |m| < 1. The internal representation of an arf_t stores
   the exponent in the latter format"). Measured on this machine
   (lanes/m1-repair-recon/probe_exp.out): the arf value m 2^e (m odd) is stored with
   ARF_EXP = e + bits(m), so the arf of the value 2^(E-1) has ARF_EXP = E, and
   mag_set_ui_2exp_si(r, 1, y) stores the value 2^y with MAG_EXP = y + 1, so the mag of the
   value 2^(R-1) has MAG_EXP = R. [source pending: the C code of mag_set_ui_2exp_si and of
   arb_get_interval_fmpz_2exp, which is not on disk; on this machine only the declarations,
   /usr/include/flint/mag.h:615 and arb.h:346, and the measurements quoted here.] With
   E = R = 1048576 the two integers of arb_get_interval_fmpz_2exp have 1048577 bits at most
   (probe_exp.out), so the whole file runs in a few milliseconds, and under
   `ulimit -v 2000000` (lanes/m1-repair-recon/run_limit.sh).

   Numbers of the expectations below, with E = ADF_RECON_EXP_MAX = 2^20:
       P = 2^1048575 = 2^(E-1)   (the ball of ARF_EXP = E and mantissa 1 is the point P)
       Q = 2^1048574 = 2^(E-2)   (the ball of MAG_EXP = E has the radius P; of MAG_EXP = E-1
                                  the radius Q)
   Every expectation is worked out in the comment of its test. The adele is built and cleared
   field by field (arb_init, adf_fball_init). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <flint/arb.h>
#include <flint/arf.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include <flint/mag.h>

#include <adelefeld/adele.h>
#include <adelefeld/recon.h>

#include "test_runner.h"

/* The bound of decision M1-D3, as a signed word. */
#define EXP_MAX ((slong) ADF_RECON_EXP_MAX)

/* --------------------------------------------------------------- builders */

/* set_z(z, s): the integer of the text s, which is a decimal integer, the power of two
   "2^k" (k decimal, up to 1048576 here), or a multiple "m*2^k" of one; a leading "-" negates.
   The powers of two have up to 1048577 bits, which is why they are not written out. */
static void
set_z(fmpz_t z, const char * s)
{
    fmpz_t m;
    const char * p = s;
    const char * star;
    int neg = 0;

    ADF_CHECK(s[0] != 0);
    if (*p == '-')
    {
        neg = 1;
        p++;
    }
    fmpz_init(m);
    star = strchr(p, '*');
    if (star == NULL)
    {
        if (p[0] == '2' && p[1] == '^')
        {
            long k;

            ADF_CHECK_MSG(sscanf(p, "2^%ld", &k) == 1, "cannot read the power of two %s", s);
            fmpz_one(m);
            fmpz_mul_2exp(m, m, (ulong) k);
        }
        else
        {
            ADF_CHECK_MSG(fmpz_set_str(m, p, 10) == 0, "cannot read the integer %s", s);
        }
    }
    else
    {
        char head[64];
        long k;

        ADF_CHECK_MSG((size_t) (star - p) < sizeof(head), "the mantissa of %s is too long", s);
        memcpy(head, p, (size_t) (star - p));
        head[star - p] = 0;
        ADF_CHECK_MSG(fmpz_set_str(m, head, 10) == 0, "cannot read the mantissa of %s", s);
        ADF_CHECK_MSG(sscanf(star + 1, "2^%ld", &k) == 1, "cannot read the power of two in %s",
                      s);
        fmpz_mul_2exp(m, m, (ulong) k);
    }
    if (neg)
        fmpz_neg(z, m);
    else
        fmpz_set(z, m);
    fmpz_clear(m);
}

/* q = n/d through the public constructor of rat.h (ADF_OK, q written). */
static void
set_rat(adf_rat_t q, slong n, slong d)
{
    fmpz_t num, den;

    fmpz_init_set_si(num, n);
    fmpz_init_set_si(den, d);
    ADF_CHECK(adf_rat_set_fmpz2(q, num, den) == ADF_OK);
    fmpz_clear(num);
    fmpz_clear(den);
}

/* The midpoint of r becomes the value man * 2^(exp - bits(man)), whose ARF_EXP is exp: the
   exponent the guard of decision M1-D3 reads. A zero midpoint keeps ARF_EXP = ARF_EXP_ZERO = 0
   (arf.h:81, arf.h:87; arf_set_fmpz_2exp changes nothing for a zero, arf.h:694-698), so it is
   only built with exp = 0. The exponent is checked, so a builder that did not keep its
   promise could not make a boundary test look like a boundary test. */
static void
set_mid_exp(arb_t r, slong man, slong exp)
{
    fmpz_t m, e;

    fmpz_init_set_si(m, man);
    fmpz_init_set_si(e, exp - (slong) fmpz_bits(m));
    arf_set_fmpz_2exp(arb_midref(r), m, e);
    if (man == 0)
        ADF_CHECK_MSG(arf_is_zero(arb_midref(r)), "a zero mantissa did not give a zero arf");
    ADF_CHECK_MSG(fmpz_equal_si(ARF_EXPREF(arb_midref(r)), exp),
                  "the midpoint was built with an ARF_EXP other than the one asked for");
    fmpz_clear(m);
    fmpz_clear(e);
}

/* The radius of r becomes the value 2^(exp-1), whose MAG_EXP is exp. */
static void
set_rad_exp(arb_t r, slong exp)
{
    mag_set_ui_2exp_si(arb_radref(r), 1, exp - 1);
    ADF_CHECK_MSG(fmpz_equal_si(MAG_EXPREF(arb_radref(r)), exp),
                  "the radius was built with a MAG_EXP other than the one asked for");
}

static void
adele_init_fields(adf_adele_t x)
{
    arb_init(x->inf);
    adf_fball_init(&x->fin);
}

static void
adele_clear_fields(adf_adele_t x)
{
    arb_clear(x->inf);
    adf_fball_clear(&x->fin);
}

/* The adele with the real ball [mid - rad, mid + rad], where the midpoint has ARF_EXP = E and
   the radius has MAG_EXP = R, and the finite ball (A + H Zhat)/d read with set_z. R = 0 is
   the radius zero (mag_zero, mag.h:212, MAG_EXP = 0); a radius of MAG_EXP = 0 that is not
   zero is the value 2^-1, which no case below asks for, since the guard exempts the zero mag
   and reads no other exponent for it. */
static void
adele_build_exp(adf_adele_t x, slong man, slong E, slong R, const char * A, const char * H,
                const char * d)
{
    fmpz_t a, h, dd;

    adele_init_fields(x);
    set_mid_exp(x->inf, man, E);
    if (R == 0)
        mag_zero(arb_radref(x->inf));
    else
        set_rad_exp(x->inf, R);
    fmpz_init(a);
    fmpz_init(h);
    fmpz_init(dd);
    set_z(a, A);
    set_z(h, H);
    set_z(dd, d);
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, a, h, dd) == ADF_OK);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(dd);
    ADF_CHECK_MSG(adf_adele_is_canonical(x), "the adele of the test is not canonical");
}

/* run(what, x, want, num, den): adf_adele_reconstruct on x with the marker -99/7 in the
   output. The status must be want; on ADF_OK the output must hold the rational num/den (both
   read with set_z, NULL for a status other than ADF_OK), and on every other status the output
   must still hold the marker (conventions 4.3). No candidate of the test cases below is the
   marker. */
static int
run(const char * what, const adf_adele_t x, int want, const char * num, const char * den)
{
    adf_rat_t q, mark;
    fmpq_t w;
    int got;

    adf_rat_init(q);
    adf_rat_init(mark);
    set_rat(mark, -99, 7);
    set_rat(q, -99, 7);
    fmpq_init(w);
    if (want == ADF_OK)
    {
        fmpz_t n, d;

        fmpz_init(n);
        fmpz_init(d);
        set_z(n, num);
        set_z(d, den);
        fmpz_set(fmpq_numref(w), n);
        fmpz_set(fmpq_denref(w), d);
        fmpq_canonicalise(w);
        fmpz_clear(n);
        fmpz_clear(d);
    }
    got = adf_adele_reconstruct(q, x);
    ADF_CHECK_MSG(got == want, "%s: the status is %s, expected %s", what, adf_status_str(got),
                  adf_status_str(want));
    if (want == ADF_OK)
    {
        ADF_CHECK_MSG(fmpq_equal(q->q, w) == 1, "%s: the candidate is not %s/%s", what, num, den);
        ADF_CHECK_MSG(adf_rat_is_canonical(q), "%s: the candidate is not canonical", what);
    }
    else
    {
        ADF_CHECK_MSG(adf_rat_identical(q, mark) == 1, "%s: the output was written on %s", what,
                      adf_status_str(got));
    }
    fmpq_clear(w);
    adf_rat_clear(q);
    adf_rat_clear(mark);
    return got;
}

/* run_exp: the adele of adele_build_exp, the status want, and for ADF_OK the candidate
   num/den. */
static void
run_exp(const char * what, slong man, slong E, slong R, const char * A, const char * H,
        const char * d, int want, const char * num, const char * den)
{
    adf_adele_t x;

    adele_build_exp(x, man, E, R, A, H, d);
    run(what, x, want, num, den);
    adele_clear_fields(x);
}

/* run_point: the adele of the exact point with the midpoint of ARF_EXP = E (the radius zero). */
static void
run_point(const char * what, slong man, slong E, const char * A, const char * H, const char * d,
          int want, const char * num, const char * den)
{
    adf_adele_t x;

    adele_build_exp(x, man, E, 0, A, H, d);
    ADF_CHECK_MSG(arb_is_exact(x->inf), "%s: the real ball of the test is not exact", what);
    run(what, x, want, num, den);
    adele_clear_fields(x);
}

/* ------------------------------------------ the balls of the review R1, R2 */

/* The midpoint mn * 2^e of the real ball r, built with arb_mul_2exp_fmpz from the exact
   integer mn, where e is the decimal text es (docs/reviews/m1/arith/checks/recon_huge_exp.c).
   The function checks that the exponent of the result is outside
   [-ADF_RECON_EXP_MAX, ADF_RECON_EXP_MAX], so a test cannot pass because the exponent came
   out smaller than asked for. */
static void
set_real_huge(arb_t r, slong mn, const char * es)
{
    fmpz_t e;

    fmpz_init(e);
    ADF_CHECK_MSG(fmpz_set_str(e, es, 10) == 0, "cannot read the exponent %s", es);
    arb_set_si(r, mn);
    arb_mul_2exp_fmpz(r, r, e);
    ADF_CHECK_MSG(fmpz_cmp_si(ARF_EXPREF(arb_midref(r)), -EXP_MAX - 1) < 0
                      || fmpz_cmp_si(ARF_EXPREF(arb_midref(r)), EXP_MAX + 1) > 0,
                  "the exponent of the midpoint is not above the bound");
    ADF_CHECK(arb_is_exact(r));
    fmpz_clear(e);
}

/* The radius of the same kind, with mag_mul_2exp_fmpz (checks/recon_big_alloc.c, case 2), the
   midpoint being the exact 1. */
static void
set_real_huge_radius(arb_t r, const char * es)
{
    fmpz_t e;

    fmpz_init(e);
    ADF_CHECK_MSG(fmpz_set_str(e, es, 10) == 0, "cannot read the exponent %s", es);
    arb_set_si(r, 1);
    mag_one(arb_radref(r));
    mag_mul_2exp_fmpz(arb_radref(r), arb_radref(r), e);
    ADF_CHECK_MSG(fmpz_cmp_si(MAG_EXPREF(arb_radref(r)), -EXP_MAX - 1) < 0
                      || fmpz_cmp_si(MAG_EXPREF(arb_radref(r)), EXP_MAX + 1) > 0,
                  "the exponent of the radius is not above the bound");
    fmpz_clear(e);
}

/* run_huge: the adele with the real ball of the caller and the exact finite ball n/d. */
static void
run_huge(const char * what, const arb_t r, slong n, slong d, int want)
{
    adf_adele_t x;
    adf_rat_t f;

    adele_init_fields(x);
    arb_set(x->inf, r);
    adf_rat_init(f);
    set_rat(f, n, d);
    adf_fball_set_rat(&x->fin, f);
    ADF_CHECK(arb_is_finite(x->inf));
    ADF_CHECK(adf_adele_is_canonical(x));
    run(what, x, want, NULL, NULL);
    adf_rat_clear(f);
    adele_clear_fields(x);
}

ADF_TEST(the_real_balls_of_the_review_are_answered_limit)
{
    arb_t r;

    /* R1, case A: the exact real ball 3 * 2^(2^64+1) and the exact finite ball 6. The true
       interval is the single point 3 * 2^(2^64+1), which is not 6, so the set statement gives
       ADF_NO_SOLUTION; the review found ADF_OK with q = 6, because the exponent of the
       interval was truncated to its low limb. Decision M1-D3: ADF_LIMIT, the exponent of the
       midpoint being 2^64 + 2, above 2^20. */
    arb_init(r);
    set_real_huge(r, 3, "18446744073709551617");
    run_huge("3 * 2^(2^64+1) against 6", r, 6, 1, ADF_LIMIT);
    arb_clear(r);

    /* R1, case B: the exact real ball 2^-(2^64+3) against the exact 1/8, a point that is not
       1/8: ADF_NO_SOLUTION by the set statement, ADF_LIMIT by M1-D3. */
    arb_init(r);
    set_real_huge(r, 1, "-18446744073709551619");
    run_huge("2^-(2^64+3) against 1/8", r, 1, 8, ADF_LIMIT);
    arb_clear(r);

    /* R1, case B': the exact real ball 2^(2^64) against the exact 1, a point that is not 1. */
    arb_init(r);
    set_real_huge(r, 1, "18446744073709551616");
    run_huge("2^(2^64) against 1", r, 1, 1, ADF_LIMIT);
    arb_clear(r);

    /* R1, case D: the exponent -2^63, whose negation as a machine integer is undefined
       behaviour: the review's sanitizer build reports it at src/recon.c:106. */
    arb_init(r);
    set_real_huge(r, 1, "-9223372036854775808");
    run_huge("2^-(2^63) against 1", r, 1, 1, ADF_LIMIT);
    arb_clear(r);

    /* R2, case 1: the exact real ball 2^(2^36) against the exact 1. The point is not 1, so
       ADF_NO_SOLUTION is decidable, while the exact end points of the interval need 2^36
       bits, which the review's reproducer tried to allocate. */
    arb_init(r);
    set_real_huge(r, 1, "68719476736");
    run_huge("2^(2^36) against 1", r, 1, 1, ADF_LIMIT);
    arb_clear(r);

    /* R2, case 2: the real ball 1 +/- 2^(-2^36) against the exact 1. The interval holds 1, so
       the set statement gives ADF_OK with q = 1, but arb_get_interval_fmpz_2exp itself builds
       integers of 2^36 bits here (arb.rst:468-474 warns of exactly that). */
    arb_init(r);
    set_real_huge_radius(r, "-68719476736");
    run_huge("1 +/- 2^(-2^36) against 1", r, 1, 1, ADF_LIMIT);
    arb_clear(r);
}

/* One above the bound. Every case of the previous test is decidable without the exact end
   points, and the set statement has an answer for each of them, so ADF_LIMIT here can only
   come from the size bound of M1-D3, and the ball is finite and canonical. */
ADF_TEST(one_above_the_bound_gives_limit)
{
    /* the exact point 2^1048576, whose exponent is 1048577, against the exact 1: the point is
       not 1, so the set statement would give ADF_NO_SOLUTION. */
    run_point("2^1048576 against 1", 1, EXP_MAX + 1, "1", "0", "1", ADF_LIMIT, NULL, NULL);
    /* the exact point 2^-1048577, whose exponent is -1048576, is at the bound and is
       computed; the next two are one step further down. */
    run_point("2^-1048578 against 1", 1, -EXP_MAX - 1, "1", "0", "1", ADF_LIMIT, NULL, NULL);
    run_point("2^-1048579 against 1", 1, -EXP_MAX - 2, "1", "0", "1", ADF_LIMIT, NULL, NULL);
    /* the ball [1 - 2^1048576, 1 + 2^1048576] against the exact 1: 1 is in the interval, so
       the set statement would give ADF_OK with q = 1. */
    run_exp("1 +/- 2^1048576 against 1", 1, 1, EXP_MAX + 1, "1", "0", "1", ADF_LIMIT, NULL,
            NULL);
    /* the ball [1 - 2^-1048577, 1 + 2^-1048577] against the exact 1. */
    run_exp("1 +/- 2^-1048577 against 1", 1, 1, -EXP_MAX - 1, "1", "0", "1", ADF_LIMIT, NULL,
            NULL);
    /* the ball [1 - 2^-1048578, 1 + 2^-1048578] against the exact 1. */
    run_exp("1 +/- 2^-1048578 against 1", 1, 1, -EXP_MAX - 2, "1", "0", "1", ADF_LIMIT, NULL,
            NULL);
    /* the ball [0, 2^1048577], one step above the last case of the boundary test, against the
       exact 1: 1 is in the interval. */
    run_exp("[0, 2^1048577] against 1", 1, EXP_MAX + 1, EXP_MAX + 1, "1", "0", "1", ADF_LIMIT,
            NULL, NULL);
    /* the same ball against 1 + Zhat, where the set statement would give ADF_NOT_UNIQUE. */
    run_exp("[0, 2^1048577] against 1 + Zhat", 1, EXP_MAX + 1, EXP_MAX + 1, "1", "1", "1",
            ADF_LIMIT, NULL, NULL);
}

/* At the bound: the balls whose exponents are ADF_RECON_EXP_MAX in absolute value are
   computed, not refused. Every expectation is worked out here.

   A ball with ARF_EXP = E and mantissa 1 is the exact point P = 2^(E-1) = 2^1048575; a ball
   with MAG_EXP = E has the radius P and one with MAG_EXP = E-1 the radius Q = 2^1048574. The
   finite ball of the triple (A, H, d) is canonicalised by adf_fball_set_fmpz3, so
   (1, 1, 1) is Zhat, (1, 2, 1) is 1 + 2 Zhat, (0, P, 1) is P Zhat and (0, Q, 1) is Q Zhat. */
ADF_TEST(at_the_bound_the_ball_is_computed)
{
    /* the exact point P against the exact 1: P > 1, so the interval [P, P] holds no
       candidate. */
    run_point("2^1048575 against 1", 1, EXP_MAX, "1", "0", "1", ADF_NO_SOLUTION, NULL, NULL);

    /* the exact point P against Zhat: the candidates are the integers k with P <= k <= P,
       that is k = P, so ADF_OK with q = P = 1/2^1048575. */
    run_point("2^1048575 against Zhat", 1, EXP_MAX, "1", "1", "1", ADF_OK, "2^1048575", "1");

    /* the exact point P against 1 + 2 Zhat: 1 + 2k = P means 2k = 2^1048575 - 1, which is odd,
       so no integer k and no candidate. */
    run_point("2^1048575 against 1 + 2 Zhat", 1, EXP_MAX, "1", "2", "1", ADF_NO_SOLUTION, NULL,
              NULL);

    /* the exact point P against P Zhat: the candidates are 2^1048575 k in [P, P], that is
       k = 1, so ADF_OK with q = P. */
    run_point("2^1048575 against 2^1048575 Zhat", 1, EXP_MAX, "0", "2^1048575", "1", ADF_OK,
              "2^1048575", "1");

    /* the exact point P against Q Zhat: the candidates are 2^1048574 k in [P, P], that is
       k = 2, so ADF_OK with q = P. */
    run_point("2^1048575 against 2^1048574 Zhat", 1, EXP_MAX, "0", "2^1048574", "1", ADF_OK,
              "2^1048575", "1");

    /* the ball [P - Q, P + Q] = [Q, 3Q] against Q Zhat: the candidates are 2^1048574 k with
       1 <= k <= 3, that is k = 1, 2, 3, so ADF_NOT_UNIQUE. */
    run_exp("[2^1048574, 3*2^1048574] against 2^1048574 Zhat", 1, EXP_MAX, EXP_MAX - 1, "0",
            "2^1048574", "1", ADF_NOT_UNIQUE, NULL, NULL);

    /* the same ball against the exact 1: 1 < Q, so the interval holds no candidate. */
    run_exp("[2^1048574, 3*2^1048574] against 1", 1, EXP_MAX, EXP_MAX - 1, "1", "0", "1",
            ADF_NO_SOLUTION, NULL, NULL);

    /* the ball [0, 2P] against the exact 1: 1 is in the interval, so ADF_OK with q = 1. */
    run_exp("[0, 2^1048576] against 1", 1, EXP_MAX, EXP_MAX, "1", "0", "1", ADF_OK, "1", "1");

    /* the same ball against Zhat: the candidates are the integers k with 0 <= k <= 2P, of
       which there are 2P + 1, so ADF_NOT_UNIQUE. */
    run_exp("[0, 2^1048576] against Zhat", 1, EXP_MAX, EXP_MAX, "1", "1", "1", ADF_NOT_UNIQUE,
            NULL, NULL);

    /* the point 1 with the radius P = 2^1048575: the interval [1 - P, 1 + P]. Against the
       exact 1 the answer is ADF_OK, since 1 is in the interval, and so it is against the exact
       2, since 2 < 1 + P. Against Zhat the candidates are the k with -P <= k <= P, that is
       2P + 1 of them, so ADF_NOT_UNIQUE. */
    run_exp("1 +/- 2^1048575 against 1", 1, 1, EXP_MAX, "1", "0", "1", ADF_OK, "1", "1");
    run_exp("1 +/- 2^1048575 against 2", 1, 1, EXP_MAX, "2", "0", "1", ADF_OK, "2", "1");
    run_exp("1 +/- 2^1048575 against Zhat", 1, 1, EXP_MAX, "1", "1", "1", ADF_NOT_UNIQUE, NULL,
            NULL);

    /* the point 1 with the radius 2^-1048576, whose MAG_EXP is -E+1: the interval
       [1 - 2^-1048576, 1 + 2^-1048576]. Against the exact 1 the answer is ADF_OK; against the
       exact 2 no candidate; against Zhat the interval has the width 2^-1048575 < 1 = N, and
       the only candidate is 1, so ADF_OK with q = 1. */
    run_exp("1 +/- 2^-1048576 against 1", 1, 1, -EXP_MAX + 1, "1", "0", "1", ADF_OK, "1", "1");
    run_exp("1 +/- 2^-1048576 against 2", 1, 1, -EXP_MAX + 1, "2", "0", "1", ADF_NO_SOLUTION,
            NULL, NULL);
    run_exp("1 +/- 2^-1048576 against Zhat", 1, 1, -EXP_MAX + 1, "1", "1", "1", ADF_OK, "1",
            "1");

    /* the same three cases with the radius 2^-1048577, whose MAG_EXP is -E: the negative end
       of the bound is as much computed as the positive one. */
    run_exp("1 +/- 2^-1048577 against 1", 1, 1, -EXP_MAX, "1", "0", "1", ADF_OK, "1", "1");
    run_exp("1 +/- 2^-1048577 against 2", 1, 1, -EXP_MAX, "2", "0", "1", ADF_NO_SOLUTION, NULL,
            NULL);
    run_exp("1 +/- 2^-1048577 against Zhat", 1, 1, -EXP_MAX, "1", "1", "1", ADF_OK, "1", "1");

    /* the point 2^-1048577, whose ARF_EXP is -E, against the exact 1, against Zhat and
       against the progression 2^-1048577 + Zhat, which is the canonical triple
       (1, 2^1048577, 2^1048577) since the centre is A/d. The point is far from 1 and is not an
       integer, so the first two have no candidate, and the third has the single candidate
       2^-1048577, with k = 0. */
    run_point("2^-1048577 against 1", 1, -EXP_MAX, "1", "0", "1", ADF_NO_SOLUTION, NULL, NULL);
    run_point("2^-1048577 against Zhat", 1, -EXP_MAX, "1", "1", "1", ADF_NO_SOLUTION, NULL,
              NULL);
    run_point("2^-1048577 against 2^-1048577 + Zhat", 1, -EXP_MAX, "1", "2^1048577", "2^1048577",
              ADF_OK, "1", "2^1048577");

    /* the negative of the boundary point: -P has ARF_EXP = E as well. Against the exact -1 the
       interval [-P, -P] holds no candidate; against Zhat the single candidate -P, so ADF_OK
       with q = -P. */
    run_point("-2^1048575 against -1", -1, EXP_MAX, "-1", "0", "1", ADF_NO_SOLUTION, NULL, NULL);
    run_point("-2^1048575 against Zhat", -1, EXP_MAX, "-1", "1", "1", ADF_OK, "-2^1048575",
              "1");

    /* the point -P with the radius Q: the interval [-3Q, -Q] against Zhat holds many
       candidates, so ADF_NOT_UNIQUE. */
    run_exp("[-3*2^1048574, -2^1048574] against Zhat", -1, EXP_MAX, EXP_MAX - 1, "-1", "1", "1",
            ADF_NOT_UNIQUE, NULL, NULL);

    /* a two-bit mantissa at the bound: the midpoint 3 * 2^(E-2) = 3Q has ARF_EXP = E, and
       with the radius Q the interval is [2Q, 4Q] = [P, 2P]. Against P Zhat the candidates are
       2^1048575 k with 1 <= k <= 2, that is P and 2P, so ADF_NOT_UNIQUE; against the exact 1
       there is no candidate, since 1 < P. */
    run_exp("[2^1048575, 2^1048576] against 2^1048575 Zhat", 3, EXP_MAX, EXP_MAX - 1, "0",
            "2^1048575", "1", ADF_NOT_UNIQUE, NULL, NULL);
    run_exp("[2^1048575, 2^1048576] against 1", 3, EXP_MAX, EXP_MAX - 1, "1", "0", "1",
            ADF_NO_SOLUTION, NULL, NULL);
    /* the same point without a radius, [3Q, 3Q], against Q Zhat: the candidates are
       2^1048574 k = 3Q, that is k = 3, so ADF_OK with q = 3Q = 3/2^1048574. */
    run_point("3*2^1048574 against 2^1048574 Zhat", 3, EXP_MAX, "0", "2^1048574", "1", ADF_OK,
              "3*2^1048574", "1");

    /* a zero midpoint carries no exponent above the bound: its ARF_EXP is ARF_EXP_ZERO = 0
       (arf.h:81), and the ball [-P, P] with the radius of MAG_EXP = E is computed. Against
       the exact 0, against the exact 1 and against Zhat the answers are ADF_OK, ADF_OK and
       ADF_NOT_UNIQUE. */
    run_exp("[-2^1048575, 2^1048575] against 0", 0, 0, EXP_MAX, "0", "0", "1", ADF_OK, "0", "1");
    run_exp("[-2^1048575, 2^1048575] against 1", 0, 0, EXP_MAX, "1", "0", "1", ADF_OK, "1", "1");
    run_exp("[-2^1048575, 2^1048575] against Zhat", 0, 0, EXP_MAX, "1", "1", "1", ADF_NOT_UNIQUE,
            NULL, NULL);
}

/* A zero in either field, and two zeros. Neither zero carries an exponent above the bound: the
   zero arf has ARF_EXP = ARF_EXP_ZERO = 0 (arf.h:81) and the zero mag has MAG_EXP = 0
   (mag.h:212), so the guard must exempt both. */
ADF_TEST(a_zero_midpoint_a_zero_radius_and_two_zeros)
{
    adf_adele_t x;
    fmpz_t zero, one, two, five, six, nine;

    fmpz_init(zero);
    fmpz_init_set_ui(one, 1);
    fmpz_init_set_ui(two, 2);
    fmpz_init_set_ui(five, 5);
    fmpz_init_set_ui(six, 6);
    fmpz_init_set_ui(nine, 9);

    /* the midpoint 0 and the radius 8: the interval [-8, 8]. */
    adele_init_fields(x);
    set_mid_exp(x->inf, 0, 0);
    mag_set_ui_2exp_si(arb_radref(x->inf), 1, 3);
    ADF_CHECK(arf_is_zero(arb_midref(x->inf)));
    ADF_CHECK(fmpz_equal_si(MAG_EXPREF(arb_radref(x->inf)), 4));

    /* the exact 0 is a candidate, since the interval is closed. */
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, zero, zero, one) == ADF_OK);
    run("[-8, 8] against 0", x, ADF_OK, "0", "1");
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, one, zero, one) == ADF_OK);
    run("[-8, 8] against 1", x, ADF_OK, "1", "1");
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, nine, zero, one) == ADF_OK);
    run("[-8, 8] against 9", x, ADF_NO_SOLUTION, NULL, NULL);
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, one, one, one) == ADF_OK);
    run("[-8, 8] against Zhat", x, ADF_NOT_UNIQUE, NULL, NULL);
    adele_clear_fields(x);

    /* the radius 0 and the midpoint 5: the interval is the single point 5. */
    adele_init_fields(x);
    set_mid_exp(x->inf, 5, 3);
    mag_zero(arb_radref(x->inf));
    ADF_CHECK(arb_is_exact(x->inf));
    ADF_CHECK(fmpz_equal_si(MAG_EXPREF(arb_radref(x->inf)), 0));
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, five, zero, one) == ADF_OK);
    run("[5, 5] against 5", x, ADF_OK, "5", "1");
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, six, zero, one) == ADF_OK);
    run("[5, 5] against 6", x, ADF_NO_SOLUTION, NULL, NULL);
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, five, one, one) == ADF_OK);
    run("[5, 5] against 5 + Zhat", x, ADF_OK, "5", "1");
    adele_clear_fields(x);

    /* both zero: the interval is the single point 0. */
    adele_init_fields(x);
    arf_zero(arb_midref(x->inf));
    mag_zero(arb_radref(x->inf));
    ADF_CHECK(arf_is_zero(arb_midref(x->inf)));
    ADF_CHECK(mag_is_zero(arb_radref(x->inf)));
    ADF_CHECK(arb_is_exact(x->inf));
    ADF_CHECK(arb_is_finite(x->inf));
    ADF_CHECK(adf_adele_is_canonical(x));
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, zero, zero, one) == ADF_OK);
    run("[0, 0] against 0", x, ADF_OK, "0", "1");
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, one, zero, one) == ADF_OK);
    run("[0, 0] against 1", x, ADF_NO_SOLUTION, NULL, NULL);
    /* 0 is a candidate of Zhat, with k = 0, and it is the only one. */
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, one, one, one) == ADF_OK);
    run("[0, 0] against Zhat", x, ADF_OK, "0", "1");
    /* 0 is a candidate of 2 Zhat, with k = 0. */
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, zero, two, one) == ADF_OK);
    run("[0, 0] against 2 Zhat", x, ADF_OK, "0", "1");
    /* 0 is a candidate of 1 + 2 Zhat only if 1 + 2k = 0 has an integer solution, which it has
       not, so there is no candidate. */
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, one, two, one) == ADF_OK);
    run("[0, 0] against 1 + 2 Zhat", x, ADF_NO_SOLUTION, NULL, NULL);
    adele_clear_fields(x);

    fmpz_clear(zero);
    fmpz_clear(one);
    fmpz_clear(two);
    fmpz_clear(five);
    fmpz_clear(six);
    fmpz_clear(nine);
}
