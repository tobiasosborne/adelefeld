/* tests/test_recon.c: adf_fball_reconstruct and adf_adele_reconstruct.

   The contract is include/adelefeld/recon.h: docs/SPEC.md 9.2 first item, docs/proofs/quotient.md
   Proposition 11 (line 257), docs/conventions.md 6.8 (DECISION CV-51, D3: the real interval is
   closed), 3.2 (the statuses) and 4.3 (every output is untouched on a status other than ADF_OK).

   The oracle of the tests below is written here and never calls the code under test: either an
   enumeration of the candidates a + N k over a range of k, compared one by one with the two end
   points, or the formula of Proposition 11 computed with fmpq and with the integer divisions
   fmpz_cdiv_q (towards +infinity) and fmpz_fdiv_q (towards -infinity) of a fraction in lowest
   terms with a positive denominator. The vector file tests/ref/vectors/recon.jsonl is read
   completely by tests/test_recon_vectors.c.

   The adele is built and cleared field by field (arb_init, adf_fball_init): the functions of
   adelefeld/adele.h are written by another lane at the same time and are not called here. */

#ifdef ADF_CHECK_INVARIANTS
#define _POSIX_C_SOURCE 200809L   /* fork, pipe: the test of M1-D11 below */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <flint/arb.h>
#include <flint/arf.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include <flint/mag.h>

#include <adelefeld/adele.h>
#include <adelefeld/recon.h>

#include "test_runner.h"

/* --------------------------------------------------------------- builders */

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

/* q = the rational of the fmpq q, through adf_rat_set_fmpq (canonical, ADF_OK). */
static void
set_rat_from_fmpq(adf_rat_t q, const fmpq_t v)
{
    ADF_CHECK(adf_rat_set_fmpq(q, v) == ADF_OK);
}

/* x = (A + H Zhat)/d, from a raw triple. */
static void
set_ball(adf_fball_t x, slong A, slong H, slong d)
{
    fmpz_t a, h, dd;

    fmpz_init_set_si(a, A);
    fmpz_init_set_si(h, H);
    fmpz_init_set_si(dd, d);
    ADF_CHECK(adf_fball_set_fmpz3(x, a, h, dd) == ADF_OK);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(dd);
}

/* A deterministic integer of `bits` bits: the top bit and every bit whose index is congruent to
   `seed` modulo 3 are set. Used for the operands of 4096 bits. */
static void
big_fmpz(fmpz_t z, slong bits, ulong seed)
{
    slong i;

    fmpz_zero(z);
    for (i = 0; i < bits; i++)
        if (((ulong) i + seed) % 3 == 0)
            fmpz_setbit(z, (ulong) i);
}

/* The exact interval [lo, hi] of a real ball, read through arf: arb_get_interval_arf at `prec`
   bits and arf_get_fmpq on the two end points. Independent of the arb_get_interval_fmpz_2exp
   route of src/recon.c.
   [source pending: the entries of refs/src/flint-3.0.1/doc/source/arb.rst and arf.rst for
   arb_get_interval_arf and arf_get_fmpq; of the two, only arb.rst is on this machine. The
   declarations are /usr/include/flint/arb.h:355 and /usr/include/flint/arf.h:1125, and 5000
   round trips of arf_get_fmpq on dyadic arf values of up to 100 bits returned the exact
   rational every time (lanes/m1-recon/report.md).] */
static void
real_interval(fmpq_t lo, fmpq_t hi, const arb_t r, slong prec)
{
    arf_t al, ah;

    arf_init(al);
    arf_init(ah);
    arb_get_interval_arf(al, ah, r, prec);
    arf_get_fmpq(lo, al);
    arf_get_fmpq(hi, ah);
    arf_clear(al);
    arf_clear(ah);
}

/* The real ball [mid - rad, mid + rad] with mid = mn * 2^me and rad = 2^re, exactly.
   arb_set_fmpz_2exp writes the midpoint without rounding and the radius zero
   (/usr/include/flint/arb.h:187); the radius is then a mag of one limb
   (mag_set_fmpz_2exp_fmpz, /usr/include/flint/mag.h:479), which is exact, and the two
   accessors are the macros of /usr/include/flint/arb.h:38-39. Both end points are then dyadic,
   so the ball is exact; every adele built this way is checked against the interval it was asked
   for, so nothing here is assumed silently. */
static void
set_real(arb_t r, const fmpz_t mn, const fmpz_t me, slong re)
{
    fmpz_t one, ex;

    fmpz_init_set_ui(one, 1);
    fmpz_init_set_si(ex, re);
    arb_set_fmpz_2exp(r, mn, me);
    mag_set_fmpz_2exp_fmpz(arb_radref(r), one, ex);
    fmpz_clear(one);
    fmpz_clear(ex);
}

/* mid - rad and mid + rad as exact rationals, from the same data as set_real. */
static void
real_end_points(fmpq_t lo, fmpq_t hi, const fmpz_t mn, const fmpz_t me, slong re)
{
    fmpq_t mid, rad;

    fmpq_init(mid);
    fmpq_init(rad);
    fmpz_set(fmpq_numref(mid), mn);
    fmpz_one(fmpq_denref(mid));
    if (fmpz_sgn(me) >= 0)
        fmpz_mul_2exp(fmpq_numref(mid), fmpq_numref(mid), (ulong) fmpz_get_ui(me));
    else
        fmpz_mul_2exp(fmpq_denref(mid), fmpq_denref(mid), (ulong) (-fmpz_get_si(me)));
    fmpq_canonicalise(mid);
    fmpq_set_si(rad, 1, 1);
    if (re >= 0)
        fmpz_mul_2exp(fmpq_numref(rad), fmpq_numref(rad), (ulong) re);
    else
        fmpz_mul_2exp(fmpq_denref(rad), fmpq_denref(rad), (ulong) (-re));
    fmpq_canonicalise(rad);
    fmpq_sub(lo, mid, rad);
    fmpq_add(hi, mid, rad);
    fmpq_clear(mid);
    fmpq_clear(rad);
}

/* The two oracles. */

/* expected_status(out, a, N, lo, hi): the candidate set of quotient.md Proposition 11 by its
   formula, with fmpq and integer floor and ceiling division. Returns 0 for no candidate, 1 for
   exactly one (written to out) and 2 for several. */
static int
expected_status(fmpq_t out, const fmpq_t a, const fmpq_t N, const fmpq_t lo, const fmpq_t hi)
{
    fmpq_t t, c;
    fmpz_t kmin, kmax;
    int found;

    if (fmpq_cmp(lo, hi) > 0)
        return 0;
    if (fmpq_is_zero(N))
    {
        if (fmpq_cmp(lo, a) <= 0 && fmpq_cmp(a, hi) <= 0)
        {
            fmpq_set(out, a);
            return 1;
        }
        return 0;
    }
    fmpq_init(t);
    fmpq_init(c);
    fmpz_init(kmin);
    fmpz_init(kmax);
    fmpq_sub(t, lo, a);
    fmpq_div(t, t, N);
    fmpz_cdiv_q(kmin, fmpq_numref(t), fmpq_denref(t));
    fmpq_sub(t, hi, a);
    fmpq_div(t, t, N);
    fmpz_fdiv_q(kmax, fmpq_numref(t), fmpq_denref(t));
    found = 0;
    if (fmpz_cmp(kmin, kmax) <= 0)
    {
        fmpz_set(fmpq_numref(c), kmin);
        fmpz_one(fmpq_denref(c));
        fmpq_mul(c, c, N);
        fmpq_add(c, c, a);
        fmpq_canonicalise(c);
        if (fmpz_equal(kmin, kmax))
        {
            fmpq_set(out, c);
            found = 1;
        }
        else
        {
            found = 2;
        }
    }
    fmpq_clear(c);
    fmpq_clear(t);
    fmpz_clear(kmin);
    fmpz_clear(kmax);
    return found;
}

/* enumerate_candidates(list, max, a, N, lo, hi): every a + N k with k in [-64, 64] that lies in
   the closed interval, found by comparing rationals. No floor and no ceiling: this is the
   enumeration against which Proposition 11 is checked. Returns the number found, and writes the
   first `max` of them to list. */
static int
enumerate_candidates(fmpq_t * list, int max, const fmpq_t a, const fmpq_t N, const fmpq_t lo,
                     const fmpq_t hi)
{
    fmpq_t k, c;
    slong i;
    int n = 0;

    /* A ball of radius 0 is the point a: a + N k is a for every k, and the candidate is counted
       once. */
    if (fmpq_is_zero(N))
    {
        if (fmpq_cmp(lo, a) <= 0 && fmpq_cmp(a, hi) <= 0)
        {
            if (max > 0)
                fmpq_set(list[0], a);
            return 1;
        }
        return 0;
    }
    fmpq_init(k);
    fmpq_init(c);
    for (i = -64; i <= 64; i++)
    {
        fmpq_set_si(k, i, 1);
        fmpq_mul(c, k, N);
        fmpq_add(c, c, a);
        fmpq_canonicalise(c);
        if (fmpq_cmp(lo, c) <= 0 && fmpq_cmp(c, hi) <= 0)
        {
            if (n < max)
                fmpq_set(list[n], c);
            n++;
        }
    }
    fmpq_clear(k);
    fmpq_clear(c);
    return n;
}

/* ------------------------------------------------------------- the drivers */

/* Run adf_fball_reconstruct on the ball x with the closed interval [ln/ld, hn/hd] and check the
   status, the value and the state of the output. On a status other than ADF_OK the output must
   still hold the marker (conventions 4.3); no candidate of the test cases below is the marker. */
static void
run_fball(const adf_fball_t x, slong ln, slong ld, slong hn, slong hd, int want, slong qn,
          slong qd, const char * what)
{
    adf_rat_t lo, hi, q, mark, expect;
    int got;

    adf_rat_init(lo);
    adf_rat_init(hi);
    adf_rat_init(q);
    adf_rat_init(mark);
    adf_rat_init(expect);
    set_rat(lo, ln, ld);
    set_rat(hi, hn, hd);
    set_rat(mark, -99, 7);
    set_rat(q, -99, 7);
    set_rat(expect, qn, qd);
    got = adf_fball_reconstruct(q, x, lo, hi);
    ADF_CHECK_MSG(got == want, "%s: the status is %s, expected %s", what, adf_status_str(got),
                  adf_status_str(want));
    if (want == ADF_OK)
    {
        ADF_CHECK_MSG(adf_rat_equal(q, expect) == 1, "%s: the candidate is not %ld/%ld", what, qn,
                      qd);
        ADF_CHECK(adf_rat_is_canonical(q));
    }
    else
    {
        ADF_CHECK_MSG(adf_rat_identical(q, mark) == 1, "%s: the output was written on %s", what,
                      adf_status_str(got));
    }
    adf_rat_clear(lo);
    adf_rat_clear(hi);
    adf_rat_clear(q);
    adf_rat_clear(mark);
    adf_rat_clear(expect);
}

/* An adele built and cleared field by field; see the comment at the head of the file. */
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

/* The adele ([mid - rad, mid + rad]) x (A + H Zhat)/d with mid = mn * 2^me, rad = 2^re, built
   exactly, and checked: the exact interval read back from the arb must be mid - rad, mid + rad. */
static void
adele_build(adf_adele_t x, const fmpz_t mn, const fmpz_t me, slong re, const fmpz_t A,
            const fmpz_t H, const fmpz_t d)
{
    fmpq_t lo, hi, glo, ghi;

    adele_init_fields(x);
    set_real(x->inf, mn, me, re);
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, A, H, d) == ADF_OK);
    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_init(glo);
    fmpq_init(ghi);
    real_end_points(lo, hi, mn, me, re);
    real_interval(glo, ghi, x->inf, 8192);
    ADF_CHECK_MSG(fmpq_equal(lo, glo) && fmpq_equal(hi, ghi),
                  "the real ball of the test is not the closed interval it asked for");
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpq_clear(glo);
    fmpq_clear(ghi);
}

/* adele_build with the end points and the triple given as slong. */
static void
adele_build_small(adf_adele_t x, slong mn, slong me, slong re, slong A, slong H, slong d)
{
    fmpz_t m, e, a, h, dd;

    fmpz_init_set_si(m, mn);
    fmpz_init_set_si(e, me);
    fmpz_init_set_si(a, A);
    fmpz_init_set_si(h, H);
    fmpz_init_set_si(dd, d);
    adele_build(x, m, e, re, a, h, dd);
    fmpz_clear(m);
    fmpz_clear(e);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(dd);
}

/* Run adf_adele_reconstruct on x, with the marker in the output; check the status and that the
   output was not written unless the status is ADF_OK. Returns the status. */
static int
run_adele(const adf_adele_t x, int want, const char * what)
{
    adf_rat_t q, mark;
    int got;

    adf_rat_init(q);
    adf_rat_init(mark);
    set_rat(mark, -99, 7);
    set_rat(q, -99, 7);
    got = adf_adele_reconstruct(q, x);
    ADF_CHECK_MSG(got == want, "%s: the status is %s, expected %s", what, adf_status_str(got),
                  adf_status_str(want));
    if (want != ADF_OK)
        ADF_CHECK_MSG(adf_rat_identical(q, mark) == 1, "%s: the output was written on %s", what,
                      adf_status_str(got));
    adf_rat_clear(q);
    adf_rat_clear(mark);
    return got;
}

/* -------------------------------------------------- adf_fball_reconstruct */

ADF_TEST(one_candidate_of_a_ball_of_radius_one_sixth)
{
    adf_fball_t x;

    /* The canonical triple (0, 1, 6) is the set (1/6) Zhat: centre 0, radius 1/6. */
    adf_fball_init(x);
    set_ball(x, 0, 1, 6);
    run_fball(x, 0, 1, 0, 1, ADF_OK, 0, 1, "the point 0 of the progression (1/6) Z");
    run_fball(x, 1, 2, 1, 2, ADF_OK, 1, 2, "the point 1/2 of the progression (1/6) Z");
    run_fball(x, 1, 6, 1, 6, ADF_OK, 1, 6, "the point 1/6 of the progression (1/6) Z");
    run_fball(x, 7, 6, 7, 6, ADF_OK, 7, 6, "the point 7/6 of the progression (1/6) Z");
    run_fball(x, -1, 6, -1, 6, ADF_OK, -1, 6, "the point -1/6 of the progression (1/6) Z");
    run_fball(x, 1, 2, 7, 12, ADF_OK, 1, 2, "1/2 at the left end of [1/2, 7/12]");
    run_fball(x, 5, 12, 1, 2, ADF_OK, 1, 2, "1/2 at the right end of [5/12, 1/2]");
    run_fball(x, 1, 3, 5, 12, ADF_OK, 1, 3, "the only candidate of [1/3, 5/12] is 1/3");
    run_fball(x, 1, 2, 2, 3, ADF_NOT_UNIQUE, 0, 1, "[1/2, 2/3] has the candidates 1/2 and 2/3");
    adf_fball_clear(x);
}

ADF_TEST(the_end_points_of_the_closed_interval_are_candidates)
{
    adf_fball_t x;

    /* M0-D3, conventions 6.8: the interval is closed, so an end point may be the answer. */
    adf_fball_init(x);
    set_ball(x, 0, 1, 1);
    run_fball(x, 0, 1, 0, 1, ADF_OK, 0, 1, "0 at both end points of [0, 0]");
    run_fball(x, -2, 3, 0, 1, ADF_OK, 0, 1, "0 at the right end of [-2/3, 0]");
    run_fball(x, 0, 1, 1, 3, ADF_OK, 0, 1, "0 at the left end of [0, 1/3]");
    run_fball(x, 1, 1, 1, 1, ADF_OK, 1, 1, "1 at both end points of [1, 1]");
    run_fball(x, -3, 1, -5, 2, ADF_OK, -3, 1, "-3 at the left end of [-3, -5/2]");
    run_fball(x, -3, 1, -3, 1, ADF_OK, -3, 1, "-3 at both end points of [-3, -3]");
    adf_fball_clear(x);

    /* (1, 2, 3) is the set 1/3 + (2/3) Zhat; 1/3 is an end point of the interval below and the
       only candidate. */
    adf_fball_init(x);
    set_ball(x, 1, 2, 3);
    run_fball(x, 1, 3, 1, 3, ADF_OK, 1, 3, "1/3 at both end points of [1/3, 1/3]");
    run_fball(x, 1, 3, 1, 2, ADF_OK, 1, 3, "1/3 at the left end of [1/3, 1/2]");
    run_fball(x, 0, 1, 1, 3, ADF_OK, 1, 3, "1/3 at the right end of [0, 1/3]");
    adf_fball_clear(x);
}

ADF_TEST(an_interval_of_the_width_of_the_radius)
{
    adf_fball_t x;

    /* An interval of width N has at least one candidate (P11, item 2). Whether it has one or two
       is decided by the position of the interval: [1/2, 3/2] and [1/3, 4/3] hold the single
       candidate 1, while [0, 1] holds the two candidates 0 and 1. */
    adf_fball_init(x);
    set_ball(x, 0, 1, 1);
    run_fball(x, 1, 2, 3, 2, ADF_OK, 1, 1, "width 1 = N on [1/2, 3/2]");
    run_fball(x, 1, 3, 4, 3, ADF_OK, 1, 1, "width 1 = N on [1/3, 4/3]");
    run_fball(x, 0, 1, 1, 1, ADF_NOT_UNIQUE, 0, 1, "width 1 = N on [0, 1]");
    run_fball(x, -1, 1, 0, 1, ADF_NOT_UNIQUE, 0, 1, "width 1 = N on [-1, 0]");
    run_fball(x, 0, 1, 2, 1, ADF_NOT_UNIQUE, 0, 1, "width 2 on [0, 2]");
    adf_fball_clear(x);

    adf_fball_init(x);
    set_ball(x, 1, 2, 3);
    run_fball(x, 0, 1, 2, 3, ADF_OK, 1, 3, "width 2/3 = N on [0, 2/3]");
    run_fball(x, 1, 3, 1, 1, ADF_NOT_UNIQUE, 0, 1, "width 2/3 = N on [1/3, 1]");
    adf_fball_clear(x);
}

ADF_TEST(an_interval_just_below_the_width_of_the_radius)
{
    adf_fball_t x;

    adf_fball_init(x);
    set_ball(x, 0, 1, 1);
    run_fball(x, 1, 2, 5, 4, ADF_OK, 1, 1, "width 3/4 on [1/2, 5/4]");
    run_fball(x, 1, 4, 1, 1, ADF_OK, 1, 1, "width 3/4 on [1/4, 1]");
    run_fball(x, 0, 1, 3, 4, ADF_OK, 0, 1, "width 3/4 on [0, 3/4]");
    run_fball(x, 1, 4, 3, 4, ADF_NO_SOLUTION, 0, 1, "width 1/2 on [1/4, 3/4]");
    run_fball(x, -1, 2, -1, 4, ADF_NO_SOLUTION, 0, 1, "width 1/4 on [-1/2, -1/4]");
    adf_fball_clear(x);

    adf_fball_init(x);
    set_ball(x, 1, 2, 3);
    run_fball(x, 0, 1, 7, 12, ADF_OK, 1, 3, "width 7/12 < 2/3 on [0, 7/12]");
    run_fball(x, 1, 12, 5, 12, ADF_OK, 1, 3, "width 1/3 on [1/12, 5/12] holds 1/3");
    adf_fball_clear(x);
}

ADF_TEST(no_candidate_gives_no_solution)
{
    adf_fball_t x;

    adf_fball_init(x);
    set_ball(x, 0, 1, 1);
    run_fball(x, 1, 4, 3, 4, ADF_NO_SOLUTION, 0, 1, "[1/4, 3/4] meets no integer");
    run_fball(x, 5, 4, 7, 4, ADF_NO_SOLUTION, 0, 1, "[5/4, 7/4] meets no integer");
    run_fball(x, -7, 4, -5, 4, ADF_NO_SOLUTION, 0, 1, "[-7/4, -5/4] meets no integer");
    run_fball(x, 3, 2, 5, 2, ADF_OK, 2, 1, "[3/2, 5/2] holds the single integer 2");
    adf_fball_clear(x);

    adf_fball_init(x);
    set_ball(x, 0, 1, 6);
    run_fball(x, 5, 12, 5, 12, ADF_NO_SOLUTION, 0, 1, "5/12 is not a multiple of 1/6");
    run_fball(x, 7, 12, 7, 12, ADF_NO_SOLUTION, 0, 1, "7/12 is not a multiple of 1/6");
    run_fball(x, 1, 4, 5, 12, ADF_OK, 1, 3, "[1/4, 5/12] holds the single multiple 1/3");
    run_fball(x, 5, 24, 7, 24, ADF_NO_SOLUTION, 0, 1, "[5/24, 7/24] meets no multiple of 1/6");
    adf_fball_clear(x);
}

ADF_TEST(an_empty_interval_gives_no_solution)
{
    adf_fball_t x;

    /* lo > hi is the empty interval: there is no candidate, whatever the ball is. */
    adf_fball_init(x);
    set_ball(x, 0, 1, 6);
    run_fball(x, 1, 2, 1, 3, ADF_NO_SOLUTION, 0, 1, "[1/2, 1/3] is empty");
    run_fball(x, 1, 1, 0, 1, ADF_NO_SOLUTION, 0, 1, "[1, 0] is empty");
    adf_fball_clear(x);

    adf_fball_init(x);
    set_ball(x, 3, 0, 7);
    run_fball(x, 1, 2, 1, 3, ADF_NO_SOLUTION, 0, 1, "[1/2, 1/3] is empty for a point");
    adf_fball_clear(x);
}

ADF_TEST(an_exact_finite_ball_gives_its_centre)
{
    adf_fball_t x;

    /* N = 0: the finite ball is the point a (P11, second item). */
    adf_fball_init(x);
    set_ball(x, 3, 0, 7);
    run_fball(x, 3, 7, 3, 7, ADF_OK, 3, 7, "3/7 at both end points");
    run_fball(x, 0, 1, 1, 1, ADF_OK, 3, 7, "3/7 inside [0, 1]");
    run_fball(x, 3, 7, 1, 1, ADF_OK, 3, 7, "3/7 at the left end point");
    run_fball(x, 0, 1, 3, 7, ADF_OK, 3, 7, "3/7 at the right end point");
    run_fball(x, 0, 1, 1, 2, ADF_OK, 3, 7, "3/7 inside [0, 1/2]");
    run_fball(x, 1, 2, 1, 1, ADF_NO_SOLUTION, 0, 1, "3/7 above [1/2, 1]");
    run_fball(x, 0, 1, 2, 5, ADF_NO_SOLUTION, 0, 1, "3/7 above [0, 2/5]");
    run_fball(x, 4, 7, 1, 1, ADF_NO_SOLUTION, 0, 1, "3/7 below [4/7, 1]");
    adf_fball_clear(x);

    adf_fball_init(x);
    set_ball(x, -5, 0, 6);
    run_fball(x, -1, 1, 0, 1, ADF_OK, -5, 6, "-5/6 inside [-1, 0]");
    run_fball(x, -5, 6, -5, 6, ADF_OK, -5, 6, "-5/6 at both end points");
    run_fball(x, 0, 1, 1, 1, ADF_NO_SOLUTION, 0, 1, "-5/6 below [0, 1]");
    run_fball(x, -1, 1, -1, 2, ADF_OK, -5, 6, "-5/6 inside [-1, -1/2]");
    run_fball(x, -5, 6, -1, 2, ADF_OK, -5, 6, "-5/6 at the left end of [-5/6, -1/2]");
    adf_fball_clear(x);

    adf_fball_init(x);
    set_ball(x, 0, 0, 1);
    run_fball(x, 0, 1, 0, 1, ADF_OK, 0, 1, "0 at both end points");
    run_fball(x, 0, 1, 1, 1, ADF_OK, 0, 1, "0 at the left end point");
    run_fball(x, -1, 1, 0, 1, ADF_OK, 0, 1, "0 at the right end point");
    run_fball(x, -1, 1, -1, 1, ADF_NO_SOLUTION, 0, 1, "[-1, -1] does not hold 0");
    adf_fball_clear(x);
}

ADF_TEST(negative_centres)
{
    adf_fball_t x;

    /* The canonical triple (1, 2, 3) is the set 1/3 + (2/3) Zhat, which is also
       -1/3 + (2/3) Zhat: the stored centre is in [0, N) while the candidates reach the
       negatives. */
    adf_fball_init(x);
    set_ball(x, 1, 2, 3);
    run_fball(x, -1, 3, -1, 3, ADF_OK, -1, 3, "-1/3 is a candidate");
    run_fball(x, -5, 3, -5, 3, ADF_OK, -5, 3, "-5/3 is a candidate");
    run_fball(x, -1, 3, 1, 3, ADF_NOT_UNIQUE, 0, 1, "[-1/3, 1/3] has the two candidates");
    run_fball(x, -1, 1, -1, 3, ADF_NOT_UNIQUE, 0, 1, "[-1, -1/3] has two candidates");
    run_fball(x, 1, 1, 1, 1, ADF_OK, 1, 1, "1 is a candidate");
    adf_fball_clear(x);

    /* (1, 3, 2) is 1/2 + (3/2) Zhat. */
    adf_fball_init(x);
    set_ball(x, 1, 3, 2);
    run_fball(x, -1, 1, -1, 1, ADF_OK, -1, 1, "-1 is a candidate of 1/2 + (3/2) Z");
    run_fball(x, 2, 1, 2, 1, ADF_OK, 2, 1, "2 is a candidate of 1/2 + (3/2) Z");
    run_fball(x, -1, 1, 1, 2, ADF_NOT_UNIQUE, 0, 1, "[-1, 1/2] has the candidates -1 and 1/2");
    adf_fball_clear(x);

    /* (0, 1, 4) is (1/4) Zhat. */
    adf_fball_init(x);
    set_ball(x, 0, 1, 4);
    run_fball(x, -1, 4, -1, 4, ADF_OK, -1, 4, "-1/4 is a candidate");
    run_fball(x, -1, 2, -3, 8, ADF_OK, -1, 2, "-1/2 at the left end of [-1/2, -3/8]");
    run_fball(x, -3, 8, -1, 4, ADF_OK, -1, 4, "-1/4 at the right end of [-3/8, -1/4]");
    adf_fball_clear(x);
}

ADF_TEST(a_radius_with_a_denominator)
{
    adf_fball_t x;

    /* (3, 5, 6): centre 1/2, radius 5/6. */
    adf_fball_init(x);
    set_ball(x, 3, 5, 6);
    run_fball(x, 1, 2, 1, 2, ADF_OK, 1, 2, "the centre 1/2 is a candidate");
    run_fball(x, 4, 3, 4, 3, ADF_OK, 4, 3, "1/2 + 5/6 = 4/3 is a candidate");
    run_fball(x, -1, 3, -1, 3, ADF_OK, -1, 3, "1/2 - 5/6 = -1/3 is a candidate");
    run_fball(x, 13, 6, 13, 6, ADF_OK, 13, 6, "1/2 + 2 * 5/6 = 13/6 is a candidate");
    run_fball(x, 0, 1, 2, 3, ADF_OK, 1, 2, "the only candidate of [0, 2/3] is 1/2");
    run_fball(x, 0, 1, 1, 3, ADF_NO_SOLUTION, 0, 1, "[0, 1/3] holds no candidate");
    adf_fball_clear(x);

    /* (1, 7, 3): centre 1/3, radius 7/3. */
    adf_fball_init(x);
    set_ball(x, 1, 7, 3);
    run_fball(x, 8, 3, 8, 3, ADF_OK, 8, 3, "1/3 + 7/3 = 8/3 is a candidate");
    run_fball(x, -2, 1, -2, 1, ADF_OK, -2, 1, "1/3 - 7/3 = -2 is a candidate");
    run_fball(x, 1, 1, 1, 1, ADF_NO_SOLUTION, 0, 1, "[1, 1] holds no candidate");
    adf_fball_clear(x);

    /* (1, 4, 3): centre 1/3, radius 4/3. */
    adf_fball_init(x);
    set_ball(x, 1, 4, 3);
    run_fball(x, 5, 3, 5, 3, ADF_OK, 5, 3, "1/3 + 4/3 = 5/3 is a candidate");
    run_fball(x, -1, 1, -1, 1, ADF_OK, -1, 1, "1/3 - 4/3 = -1 is a candidate");
    run_fball(x, 0, 1, 1, 3, ADF_OK, 1, 3, "the only candidate of [0, 1/3] is 1/3");
    run_fball(x, 0, 1, 1, 6, ADF_NO_SOLUTION, 0, 1, "[0, 1/6] holds no candidate");
    adf_fball_clear(x);
}

ADF_TEST(several_candidates_give_not_unique)
{
    adf_fball_t x;

    adf_fball_init(x);
    set_ball(x, 0, 1, 6);
    run_fball(x, 0, 1, 1, 1, ADF_NOT_UNIQUE, 0, 1, "[0, 1] has 7 candidates");
    run_fball(x, 0, 1, 1, 2, ADF_NOT_UNIQUE, 0, 1, "[0, 1/2] has 4 candidates");
    run_fball(x, 0, 1, 5, 12, ADF_NOT_UNIQUE, 0, 1, "[0, 5/12] has 3 candidates");
    run_fball(x, -1, 3, 1, 3, ADF_NOT_UNIQUE, 0, 1, "[-1/3, 1/3] has 2 candidates");
    run_fball(x, -1, 1, 1, 1, ADF_NOT_UNIQUE, 0, 1, "[-1, 1] has 13 candidates");
    adf_fball_clear(x);

    adf_fball_init(x);
    set_ball(x, 0, 1, 1);
    run_fball(x, 0, 1, 100, 1, ADF_NOT_UNIQUE, 0, 1, "[0, 100] has 101 candidates");
    adf_fball_clear(x);
}

ADF_TEST(the_output_may_be_one_of_the_interval_end_points)
{
    adf_rat_t lo, hi, q, want;
    adf_fball_t x;
    int st;

    /* conventions 4.1(1): the output may be the same object as an input of the same type. */
    adf_fball_init(x);
    set_ball(x, 0, 1, 6);
    adf_rat_init(lo);
    adf_rat_init(hi);
    adf_rat_init(q);
    adf_rat_init(want);
    set_rat(lo, 5, 12);
    set_rat(hi, 1, 2);
    set_rat(want, 1, 2);

    set_rat(q, 5, 12);
    st = adf_fball_reconstruct(q, x, lo, hi);
    ADF_CHECK_MSG(st == ADF_OK, "q = lo: the status is %s", adf_status_str(st));
    ADF_CHECK_MSG(adf_rat_equal(q, want) == 1, "q = lo: the candidate is not the end point");

    set_rat(q, 1, 2);
    st = adf_fball_reconstruct(q, x, lo, hi);
    ADF_CHECK_MSG(st == ADF_OK, "q = hi: the status is %s", adf_status_str(st));
    ADF_CHECK_MSG(adf_rat_equal(q, want) == 1, "q = hi: the candidate is not the end point");

    /* A failure with the output equal to an input leaves the output as it was. */
    set_rat(hi, 1, 1);
    adf_rat_set(q, lo);
    adf_rat_set(want, lo);
    st = adf_fball_reconstruct(q, x, lo, hi);
    ADF_CHECK_MSG(st == ADF_NOT_UNIQUE, "q = lo with [5/12, 1]: the status is %s",
                  adf_status_str(st));
    ADF_CHECK_MSG(adf_rat_identical(q, want) == 1, "q = lo with [5/12, 1]: the output was written");

    /* lo and hi may be the same object, and the output may be that object. */
    set_rat(q, 5, 12);
    adf_rat_set(want, q);
    st = adf_fball_reconstruct(q, x, q, q);
    ADF_CHECK_MSG(st == ADF_NO_SOLUTION, "lo = hi = q = 5/12: the status is %s",
                  adf_status_str(st));
    ADF_CHECK_MSG(adf_rat_identical(q, want) == 1, "lo = hi = q = 5/12: the output was written");
    ADF_CHECK(adf_rat_is_canonical(q));

    adf_rat_clear(lo);
    adf_rat_clear(hi);
    adf_rat_clear(q);
    adf_rat_clear(want);
    adf_fball_clear(x);
}

ADF_TEST(operands_of_4096_bits)
{
    adf_fball_t x;
    adf_rat_t a, N, lo, hi, q, mark;
    fmpz_t A, H, d;
    fmpq_t fa, fN, flo, fhi, out, one_q, half_q;
    int i;
    static const char *const what[7] = {
        "the centre in the interval of width 0", "the interval from the centre to one radius",
        "one radius above the centre", "one radius below the centre",
        "a window of width 1/2 that holds no multiple of the radius",
        "the interval from the centre to two radii",
        "the point at half a radius above the centre"};

    adf_fball_init(x);
    adf_rat_init(a);
    adf_rat_init(N);
    adf_rat_init(lo);
    adf_rat_init(hi);
    adf_rat_init(q);
    adf_rat_init(mark);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpq_init(fa);
    fmpq_init(fN);
    fmpq_init(flo);
    fmpq_init(fhi);
    fmpq_init(out);
    fmpq_init(one_q);
    fmpq_init(half_q);
    fmpq_set_si(one_q, 1, 1);
    fmpq_set_si(half_q, 1, 2);

    big_fmpz(A, 4096, 1);
    big_fmpz(H, 4096, 2);
    big_fmpz(d, 4096, 0);
    ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(x));
    adf_fball_get_center(a, x);
    adf_fball_get_radius(N, x);
    fmpq_set(fa, a->q);
    fmpq_set(fN, N->q);
    ADF_CHECK_MSG(fmpz_cmp_ui(fmpq_denref(fN), 0) > 0, "the radius is in lowest terms");
    ADF_CHECK_MSG(!fmpq_is_zero(fN), "the radius of the 4096 bit ball is not 0");
    ADF_CHECK_MSG(fmpz_bits(fmpq_numref(fN)) > 4000 || fmpz_bits(fmpq_denref(fN)) > 4000,
                  "the radius of the 4096 bit ball has a numerator or a denominator of that size");

    for (i = 0; i < 7; i++)
    {
        int found, got;
        const char * w = what[i];

        fmpq_set(flo, fa);
        fmpq_set(fhi, fa);
        if (i == 1)
            fmpq_add(fhi, fa, fN);
        else if (i == 2)
        {
            fmpq_add(flo, fa, fN);
            fmpq_add(fhi, fa, fN);
        }
        else if (i == 3)
        {
            fmpq_sub(flo, fa, fN);
            fmpq_sub(fhi, fa, fN);
        }
        else if (i == 4)
        {
            fmpq_add(flo, fa, one_q);
            fmpq_add(fhi, flo, half_q);
        }
        else if (i == 5)
        {
            fmpq_add(flo, fa, fN);
            fmpq_add(flo, flo, fN);
            fmpq_add(fhi, flo, fN);
        }
        else
        {
            fmpq_add(flo, fa, fN);
            fmpq_add(flo, flo, one_q);
            fmpq_div_2exp(flo, flo, 1);
        }
        fmpq_canonicalise(flo);
        fmpq_canonicalise(fhi);
        found = expected_status(out, fa, fN, flo, fhi);
        set_rat_from_fmpq(lo, flo);
        set_rat_from_fmpq(hi, fhi);
        set_rat(mark, -99, 7);
        set_rat(q, -99, 7);
        got = adf_fball_reconstruct(q, x, lo, hi);
        ADF_CHECK_MSG(got == (found == 0 ? ADF_NO_SOLUTION
                                         : (found == 1 ? ADF_OK : ADF_NOT_UNIQUE)),
                      "4096 bits, %s: the status is %s, expected %s", w, adf_status_str(got),
                      adf_status_str(found == 0 ? ADF_NO_SOLUTION
                                                : (found == 1 ? ADF_OK : ADF_NOT_UNIQUE)));
        if (found == 1)
        {
            adf_rat_t want;

            adf_rat_init(want);
            set_rat_from_fmpq(want, out);
            ADF_CHECK_MSG(adf_rat_equal(q, want) == 1, "4096 bits, %s: the candidate is not the "
                                                         "one of the formula",
                          w);
            ADF_CHECK(adf_rat_is_canonical(q));
            adf_rat_clear(want);
        }
        else
        {
            ADF_CHECK_MSG(adf_rat_identical(q, mark) == 1, "4096 bits, %s: the output was written",
                          w);
        }
    }

    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpq_clear(fa);
    fmpq_clear(fN);
    fmpq_clear(flo);
    fmpq_clear(fhi);
    fmpq_clear(out);
    fmpq_clear(one_q);
    fmpq_clear(half_q);
    adf_rat_clear(a);
    adf_rat_clear(N);
    adf_rat_clear(lo);
    adf_rat_clear(hi);
    adf_rat_clear(q);
    adf_rat_clear(mark);
    adf_fball_clear(x);
}

ADF_TEST(the_enumeration_of_candidates_agrees_with_the_function)
{
    /* For every raw triple (A, H, d) and every pair of end points of the grid below, the
       candidates of the closed interval are enumerated one by one, and the status of the
       function must be ADF_OK exactly when the enumeration finds one, ADF_NOT_UNIQUE when it
       finds two or more, and ADF_NO_SOLUTION when it finds none. Every interval of the grid has
       |lo|, |hi| <= 4 and every radius N is at least 1/12, so all the candidates of the
       enumeration (k in [-64, 64]) are found. */
    static const slong ends[12][2] = {{0, 1}, {1, 2},  {1, 1},  {3, 2}, {2, 1},  {-1, 2},
                                      {-1, 1}, {1, 6},  {1, 3},  {2, 3}, {5, 12}, {-1, 3}};
    fmpq_t list[4];
    adf_fball_t x;
    adf_rat_t lo, hi, q;
    slong A, H, d, i, j;
    int k;

    adf_fball_init(x);
    adf_rat_init(lo);
    adf_rat_init(hi);
    adf_rat_init(q);
    for (k = 0; k < 4; k++)
        fmpq_init(list[k]);

    for (A = -3; A <= 3; A++)
        for (H = 0; H <= 4; H++)
            for (d = 1; d <= 3; d++)
            {
                adf_rat_t a, N;
                fmpq_t fa, fN, flo, fhi;
                int n;

                set_ball(x, A, H, d);
                ADF_CHECK(adf_fball_is_canonical(x));
                adf_rat_init(a);
                adf_rat_init(N);
                adf_fball_get_center(a, x);
                adf_fball_get_radius(N, x);
                fmpq_init(fa);
                fmpq_init(fN);
                fmpq_init(flo);
                fmpq_init(fhi);
                fmpq_set(fa, a->q);
                fmpq_set(fN, N->q);
                for (i = 0; i < 12; i++)
                    for (j = 0; j < 12; j++)
                    {
                        fmpq_set_si(flo, ends[i][0], ends[i][1]);
                        fmpq_set_si(fhi, ends[j][0], ends[j][1]);
                        n = enumerate_candidates(list, 4, fa, fN, flo, fhi);
                        set_rat_from_fmpq(lo, flo);
                        set_rat_from_fmpq(hi, fhi);
                        ADF_CHECK_MSG(
                            adf_fball_reconstruct(q, x, lo, hi) ==
                                (n == 0 ? ADF_NO_SOLUTION : (n == 1 ? ADF_OK : ADF_NOT_UNIQUE)),
                            "the ball (A, H, d) = (%ld, %ld, %ld) on the interval "
                            "[%ld/%ld, %ld/%ld]: the enumeration found %d candidates",
                            A, H, d, ends[i][0], ends[i][1], ends[j][0], ends[j][1], n);
                        if (n == 1)
                        {
                            adf_rat_t want;

                            adf_rat_init(want);
                            set_rat_from_fmpq(want, list[0]);
                            ADF_CHECK_MSG(adf_rat_equal(q, want) == 1,
                                          "the ball (A, H, d) = (%ld, %ld, %ld) on the interval "
                                          "[%ld/%ld, %ld/%ld]: the candidate is not the one of "
                                          "the enumeration",
                                          A, H, d, ends[i][0], ends[i][1], ends[j][0], ends[j][1]);
                            adf_rat_clear(want);
                        }
                    }
                fmpq_clear(fa);
                fmpq_clear(fN);
                fmpq_clear(flo);
                fmpq_clear(fhi);
                adf_rat_clear(a);
                adf_rat_clear(N);
            }

    adf_rat_clear(lo);
    adf_rat_clear(hi);
    adf_rat_clear(q);
    adf_fball_clear(x);
    for (k = 0; k < 4; k++)
        fmpq_clear(list[k]);
}

/* ------------------------------------------------ adf_adele_reconstruct */

ADF_TEST(an_adele_with_a_real_ball_of_radius_zero)
{
    adf_adele_t x;
    adf_rat_t q, want;
    fmpq_t v;

    /* arb_set_fmpq of a dyadic number at 64 bits gives a real ball of radius 0, so the closed
       interval of the adele is the single point and the answer is the only candidate of the
       progression that is that point. */
    fmpq_init(v);
    adf_rat_init(q);
    adf_rat_init(want);

    adele_init_fields(x);
    set_ball(&x->fin, 0, 1, 6);
    fmpq_set_si(v, 1, 2);
    arb_set_fmpq(x->inf, v, 64);
    ADF_CHECK(arb_is_exact(x->inf));
    set_rat(q, -99, 7);
    ADF_CHECK(adf_adele_reconstruct(q, x) == ADF_OK);
    set_rat(want, 1, 2);
    ADF_CHECK_MSG(adf_rat_equal(q, want) == 1,
                  "the real ball of radius 0 at 1/2: the candidate is not 1/2");
    ADF_CHECK(adf_rat_is_canonical(q));
    adele_clear_fields(x);

    /* The point 3/8 is not a multiple of 1/6: no candidate. */
    adele_init_fields(x);
    set_ball(&x->fin, 0, 1, 6);
    fmpq_set_si(v, 3, 8);
    arb_set_fmpq(x->inf, v, 64);
    ADF_CHECK(arb_is_exact(x->inf));
    ADF_CHECK(run_adele(x, ADF_NO_SOLUTION, "the point 3/8 against (1/6) Zhat") ==
              ADF_NO_SOLUTION);
    adele_clear_fields(x);

    /* The point 0 is a candidate of every progression with centre 0. */
    adele_init_fields(x);
    set_ball(&x->fin, 0, 1, 1);
    fmpq_set_si(v, 0, 1);
    arb_set_fmpq(x->inf, v, 64);
    ADF_CHECK(arb_is_exact(x->inf));
    set_rat(q, -99, 7);
    ADF_CHECK(adf_adele_reconstruct(q, x) == ADF_OK);
    set_rat(want, 0, 1);
    ADF_CHECK_MSG(adf_rat_equal(q, want) == 1,
                  "the real ball of radius 0 at 0: the candidate is not 0");
    adele_clear_fields(x);

    /* The real ball of 1/3 at 64 bits has a positive radius, so its interval is a small
       neighbourhood of 1/3, and it meets no integer. */
    adele_init_fields(x);
    set_ball(&x->fin, 0, 1, 1);
    fmpq_set_si(v, 1, 3);
    arb_set_fmpq(x->inf, v, 64);
    ADF_CHECK(!arb_is_exact(x->inf));
    ADF_CHECK(run_adele(x, ADF_NO_SOLUTION, "the real ball of 1/3 at 64 bits against Zhat") ==
              ADF_NO_SOLUTION);
    adele_clear_fields(x);

    adf_rat_clear(q);
    adf_rat_clear(want);
    fmpq_clear(v);
}

ADF_TEST(the_end_points_of_the_real_interval_are_candidates)
{
    adf_adele_t x;

    /* The real ball of mid = 9/16 and radius 1/16 is [1/2, 5/8]; the only candidate of the
       progression (1/6) Z in it is 1/2, which is the left end point. */
    adele_build_small(x, 9, -4, -4, 0, 1, 6);
    run_adele(x, ADF_OK, "[1/2, 5/8] against (1/6) Zhat");
    adele_clear_fields(x);

    /* mid = 7/16, radius 1/16: the interval is [3/8, 1/2] and 1/2 is the right end point. */
    adele_build_small(x, 7, -4, -4, 0, 1, 6);
    run_adele(x, ADF_OK, "[3/8, 1/2] against (1/6) Zhat");
    adele_clear_fields(x);

    /* [1/2, 1] holds seven candidates. */
    adele_build_small(x, 3, -1, -1, 0, 1, 6);
    run_adele(x, ADF_NOT_UNIQUE, "[1/2, 1] against (1/6) Zhat");
    adele_clear_fields(x);

    /* [1/2, 9/16] holds the single candidate 1/2, at the left end point. */
    adele_build_small(x, 17, -5, -5, 0, 1, 6);
    run_adele(x, ADF_OK, "[1/2, 9/16] against (1/6) Zhat");
    adele_clear_fields(x);

    /* [1/4, 1/2] holds the two candidates 1/3 and 1/2, one of them at the right end point. */
    adele_build_small(x, 3, -3, -3, 0, 1, 6);
    run_adele(x, ADF_NOT_UNIQUE, "[1/4, 1/2] against (1/6) Zhat");
    adele_clear_fields(x);

    /* [1/2, 3/4] holds the two candidates 1/2 and 2/3, one of them at the left end point. */
    adele_build_small(x, 5, -3, -3, 0, 1, 6);
    run_adele(x, ADF_NOT_UNIQUE, "[1/2, 3/4] against (1/6) Zhat");
    adele_clear_fields(x);

    /* A real ball of radius 0 at 1/8, which is not a candidate of (1/6) Zhat. */
    adele_build_small(x, 1, -3, -100, 0, 1, 6);
    run_adele(x, ADF_NO_SOLUTION, "the point 1/8 against (1/6) Zhat");
    adele_clear_fields(x);

    /* An exact real ball of radius 0: [1/2, 1/2] holds the candidate 1/2. */
    adele_build_small(x, 1, -1, -100, 0, 1, 6);
    run_adele(x, ADF_OK, "the point 1/2 against (1/6) Zhat");
    adele_clear_fields(x);
}

ADF_TEST(the_adele_agrees_with_the_finite_ball_call_on_the_exact_interval)
{
    /* For a set of real balls, the exact interval of the arb is read through arf, and
       adf_adele_reconstruct must give the same status and the same candidate as
       adf_fball_reconstruct applied to the same finite ball and to those two end points. An
       inexact real ball is in the set: 1/3 at 64 bits. */
    static const char *const what[4] = {"the real ball [1/2, 3/4]", "the real ball of 1/3 at 64 bits",
                                        "the real ball of 7/5 at 32 bits",
                                        "the real ball of 1/7 at 200 bits"};
    adf_adele_t x;
    adf_rat_t q, r, mark;
    fmpq_t v, lo, hi, fa, fN, out;
    int i;

    adf_rat_init(q);
    adf_rat_init(r);
    adf_rat_init(mark);
    fmpq_init(v);
    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_init(fa);
    fmpq_init(fN);
    fmpq_init(out);
    set_rat(mark, -99, 7);

    for (i = 0; i < 4; i++)
    {
        adf_rat_t a, N, lo_r, hi_r;
        int found, got, want;

        adele_init_fields(x);
        set_ball(&x->fin, 3, 5, 6);
        fmpq_set_si(v, 1, 1);
        if (i == 0)
        {
            /* [1/2, 3/4] exactly: mid = 5/8, radius 1/8. */
            fmpz_t m, e;

            fmpz_init_set_si(m, 5);
            fmpz_init_set_si(e, -3);
            set_real(x->inf, m, e, -3);
            fmpz_clear(m);
            fmpz_clear(e);
        }
        else if (i == 1)
        {
            fmpq_set_si(v, 1, 3);
            arb_set_fmpq(x->inf, v, 64);
        }
        else if (i == 2)
        {
            fmpq_set_si(v, 7, 5);
            arb_set_fmpq(x->inf, v, 32);
        }
        else
        {
            fmpq_set_si(v, 1, 7);
            arb_set_fmpq(x->inf, v, 200);
        }
        ADF_CHECK(arb_is_finite(x->inf));
        real_interval(lo, hi, x->inf, 4096);
        adf_rat_init(a);
        adf_rat_init(N);
        adf_rat_init(lo_r);
        adf_rat_init(hi_r);
        adf_fball_get_center(a, &x->fin);
        adf_fball_get_radius(N, &x->fin);
        fmpq_set(fa, a->q);
        fmpq_set(fN, N->q);
        found = expected_status(out, fa, fN, lo, hi);
        want = (found == 0 ? ADF_NO_SOLUTION : (found == 1 ? ADF_OK : ADF_NOT_UNIQUE));
        set_rat_from_fmpq(lo_r, lo);
        set_rat_from_fmpq(hi_r, hi);
        set_rat(r, -99, 7);
        got = adf_fball_reconstruct(r, &x->fin, lo_r, hi_r);
        ADF_CHECK_MSG(got == want, "%s: the finite ball call returns %s, expected %s", what[i],
                      adf_status_str(got), adf_status_str(want));
        set_rat(q, -99, 7);
        got = adf_adele_reconstruct(q, x);
        ADF_CHECK_MSG(got == want, "%s: the adele call returns %s, expected %s", what[i],
                      adf_status_str(got), adf_status_str(want));
        if (want == ADF_OK)
        {
            adf_rat_t w;

            adf_rat_init(w);
            set_rat_from_fmpq(w, out);
            ADF_CHECK_MSG(adf_rat_equal(q, w) == 1,
                          "%s: the candidate of the adele call is not the candidate of the "
                          "formula",
                          what[i]);
            ADF_CHECK(adf_rat_is_canonical(q));
            adf_rat_clear(w);
        }
        else
        {
            ADF_CHECK_MSG(adf_rat_identical(q, mark) == 1, "%s: the adele call wrote its output "
                                                            "on %s",
                          what[i], adf_status_str(got));
            ADF_CHECK_MSG(adf_rat_identical(r, mark) == 1, "%s: the finite ball call wrote its "
                                                            "output on %s",
                          what[i], adf_status_str(want));
        }
        adf_rat_clear(a);
        adf_rat_clear(N);
        adf_rat_clear(lo_r);
        adf_rat_clear(hi_r);
        adele_clear_fields(x);
    }

    adf_rat_clear(q);
    adf_rat_clear(r);
    adf_rat_clear(mark);
    fmpq_clear(v);
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpq_clear(fa);
    fmpq_clear(fN);
    fmpq_clear(out);
}

ADF_TEST(an_adele_with_an_exact_finite_ball)
{
    adf_adele_t x;
    adf_rat_t q, want;
    fmpq_t v;
    arb_t y;

    adf_rat_init(q);
    adf_rat_init(want);
    fmpq_init(v);
    arb_init(y);

    /* The finite ball is the point 3/7; the real ball is [0, 1/2], which holds it. */
    adele_init_fields(x);
    set_ball(&x->fin, 3, 0, 7);
    fmpq_set_si(v, 0, 1);
    arb_set_fmpq(x->inf, v, 64);
    fmpq_set_si(v, 1, 2);
    arb_set_fmpq(y, v, 64);
    arb_union(x->inf, x->inf, y, 64);
    ADF_CHECK(run_adele(x, ADF_OK, "[0, 1/2] against the point 3/7") == ADF_OK);
    ADF_CHECK(adf_adele_reconstruct(q, x) == ADF_OK);
    set_rat(want, 3, 7);
    ADF_CHECK_MSG(adf_rat_equal(q, want) == 1, "[0, 1/2] against the point 3/7: the candidate is "
                                               "not 3/7");
    adele_clear_fields(x);

    /* [1/2, 1] does not hold 3/7. */
    adele_init_fields(x);
    set_ball(&x->fin, 3, 0, 7);
    fmpq_set_si(v, 1, 2);
    arb_set_fmpq(x->inf, v, 64);
    fmpq_set_si(v, 1, 1);
    arb_set_fmpq(y, v, 64);
    arb_union(x->inf, x->inf, y, 64);
    set_rat(q, -99, 7);
    set_rat(want, -99, 7);
    ADF_CHECK(run_adele(x, ADF_NO_SOLUTION, "[1/2, 1] against the point 3/7") ==
              ADF_NO_SOLUTION);
    ADF_CHECK_MSG(adf_rat_identical(q, want) == 1,
                  "[1/2, 1] against the point 3/7: the output was written");
    adele_clear_fields(x);

    arb_clear(y);
    adf_rat_clear(q);
    adf_rat_clear(want);
    fmpq_clear(v);
}

ADF_TEST(an_adele_with_operands_of_4096_bits)
{
    /* The finite ball is (A + H Zhat)/d with A of 4096 bits, H = 2^3100 and d = 2^4000, so
       that the centre a = A * 2^-4000 is a dyadic number that can be the midpoint of a real
       ball and the radius is N = 2^-900. The real ball is built exactly and its exact interval
       is read back through arf before the call, so the three cases below are about the interval
       the function really sees. */
    adf_adele_t x;
    adf_rat_t q, mark, want, a, N;
    fmpz_t A, H, den, m, e, one;
    fmpq_t fa, fN, glo, ghi, out;
    int i;
    static const slong re[3] = {-929, -899, -902};

    adf_rat_init(q);
    adf_rat_init(mark);
    adf_rat_init(want);
    adf_rat_init(a);
    adf_rat_init(N);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(den);
    fmpz_init(m);
    fmpz_init(e);
    fmpz_init_set_ui(one, 1);
    fmpq_init(fa);
    fmpq_init(fN);
    fmpq_init(glo);
    fmpq_init(ghi);
    fmpq_init(out);

    big_fmpz(A, 4096, 1);
    fmpz_one(den);
    fmpz_mul_2exp(den, den, 4000);
    fmpz_one(H);
    fmpz_mul_2exp(H, H, 3100);

    for (i = 0; i < 3; i++)
    {
        int found, got;
        const char * w;

        if (i == 0)
        {
            /* The real ball of radius 2^-929 around the centre a: the next candidates are at
               distance 2^-900, so a is the only candidate of [a - 2^-929, a + 2^-929]. */
            fmpz_set(m, A);
            fmpz_set_si(e, -4000);
            w = "the centre in an interval of radius 2^-929";
        }
        else if (i == 1)
        {
            /* The same midpoint with radius 2^-899: the interval is wider than the radius of
               the ball, so it holds the three candidates a, a - 2^-900 and a + 2^-900. */
            fmpz_set(m, A);
            fmpz_set_si(e, -4000);
            w = "the centre in an interval of radius 2^-899";
        }
        else
        {
            /* The midpoint a + 2^-901, which is not a multiple of 2^-900 away from a, in the
               interval of radius 2^-902 around it: no candidate at all. */
            fmpz_set(m, A);
            fmpz_setbit(m, 3099);
            fmpz_set_si(e, -4000);
            w = "an interval of radius 2^-902 around a + 2^-901";
        }
        adele_init_fields(x);
        set_real(x->inf, m, e, re[i]);
        ADF_CHECK(adf_fball_set_fmpz3(&x->fin, A, H, den) == ADF_OK);
        adf_fball_get_center(a, &x->fin);
        adf_fball_get_radius(N, &x->fin);
        fmpq_set(fa, a->q);
        fmpq_set(fN, N->q);
        ADF_CHECK_MSG(fmpz_cmp_ui(fmpq_denref(fN), 0) > 0, "the radius is in lowest terms");
        /* The exact interval of the real ball, read through arf. */
        real_interval(glo, ghi, x->inf, 8192);
        found = expected_status(out, fa, fN, glo, ghi);
        ADF_CHECK_MSG(found == (i == 0 ? 1 : (i == 1 ? 2 : 0)),
                      "4096 bits, %s: the formula finds %d candidates", w, found);
        set_rat(mark, -99, 7);
        set_rat(q, -99, 7);
        got = adf_adele_reconstruct(q, x);
        ADF_CHECK_MSG(got == (found == 0 ? ADF_NO_SOLUTION
                                         : (found == 1 ? ADF_OK : ADF_NOT_UNIQUE)),
                      "4096 bits, %s: the status is %s", w, adf_status_str(got));
        if (found == 1)
        {
            set_rat_from_fmpq(want, out);
            ADF_CHECK_MSG(adf_rat_equal(q, want) == 1, "4096 bits, %s: the candidate is not the "
                                                        "one of the formula",
                          w);
            ADF_CHECK(adf_rat_is_canonical(q));
        }
        else
        {
            ADF_CHECK_MSG(adf_rat_identical(q, mark) == 1, "4096 bits, %s: the output was written",
                          w);
        }
        adele_clear_fields(x);
    }

    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(den);
    fmpz_clear(m);
    fmpz_clear(e);
    fmpz_clear(one);
    fmpq_clear(fa);
    fmpq_clear(fN);
    fmpq_clear(glo);
    fmpq_clear(ghi);
    fmpq_clear(out);
    adf_rat_clear(q);
    adf_rat_clear(mark);
    adf_rat_clear(want);
    adf_rat_clear(a);
    adf_rat_clear(N);
}

#ifdef ADF_CHECK_INVARIANTS
/* M1-D11: a child process calls adf_adele_reconstruct(q, x) and must die by SIGABRT with the line of
   src/invariants.h:55 ("adelefeld: ADF_CHECK_INVARIANTS: <function>: argument <arg> is not a
   canonical <type>") on stderr. Returns 1 if it does. Pattern of tests/test_invariants.c. */
static int
reconstruct_aborts(const adf_adele_t x, const char * want)
{
    int fd[2], st = 0;
    char err[512];
    size_t n = 0;
    pid_t pid;

    fflush(stdout);
    fflush(stderr);
    if (pipe(fd) != 0)
        abort();
    pid = fork();
    if (pid < 0)
        abort();
    if (pid == 0)
    {
        struct rlimit nocore = {0, 0};
        adf_rat_t q;

        setrlimit(RLIMIT_CORE, &nocore);   /* an abort must not write a core file */
        close(fd[0]);
        dup2(fd[1], 2);
        close(fd[1]);
        adf_rat_init(q);
        (void) adf_adele_reconstruct(q, x);
        _exit(0);
    }
    close(fd[1]);
    for (;;)
    {
        ssize_t r = read(fd[0], err + n, sizeof err - 1 - n);
        if (r <= 0)
            break;
        n += (size_t) r;
    }
    err[n] = 0;
    close(fd[0]);
    if (waitpid(pid, &st, 0) != pid)
        abort();
    return WIFSIGNALED(st) && WTERMSIG(st) == SIGABRT && strcmp(err, want) == 0;
}
#endif

ADF_TEST(an_adele_with_an_infinite_real_ball_is_rejected)
{
    adf_adele_t x;

    /* Outside the contract: conventions 5.5 requires arb_is_finite(x->inf). Without the flag the call
       is nevertheless answered instead of aborting inside FLINT, because
       arb_get_interval_fmpz_2exp aborts on an infinite or NaN ball
       (refs/src/flint-3.0.1/arb.rst:468-470). That answer is a courtesy of the release build and no
       promise (M1-D11: recon.h does not exempt a non-canonical adele); the test of it is compiled
       only without ADF_CHECK_INVARIANTS. With the flag the entry check comes first and the call
       aborts (src/invariants.h:53-57); the test of the same name requires that. */
    adele_init_fields(x);
    set_ball(&x->fin, 0, 1, 6);
    arb_pos_inf(x->inf);
    ADF_CHECK(!arb_is_finite(x->inf));
#ifndef ADF_CHECK_INVARIANTS
    run_adele(x, ADF_DOMAIN, "an infinite real ball");
#else
    ADF_CHECK(reconstruct_aborts(x, "adelefeld: ADF_CHECK_INVARIANTS: adf_adele_reconstruct: argument x is "
                                    "not a canonical adf_adele\n"));
#endif
    adele_clear_fields(x);
}
