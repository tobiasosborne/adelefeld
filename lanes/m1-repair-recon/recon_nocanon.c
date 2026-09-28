/* lanes/m1-repair-recon/recon_nocanon.c: a copy of src/recon.c with the single mutant of the
   mutation run (src/recon.c:143 drop_call fmpq_canonicalise(q), survivor in
   lanes/m1-repair-recon/mutate-recon.log) applied, so that the comparison of
   docs/reviews/m1/arith/checks/recon_mut111.out can be repeated on the repaired file.

   adelefeld/recon.c: rational reconstruction from a full adelic ball.

   Work package 1.6 (docs/PLAN.md section 6). The contract is the comment block of
   include/adelefeld/recon.h: docs/SPEC.md 9.2 first item, docs/proofs/quotient.md Proposition 11
   (line 257), docs/conventions.md 6.8 (DECISION CV-51, D3: the real interval is closed), 3.2 (the
   statuses of the class "Reconstruction and solvers") and 4.3 (the output is untouched on a
   status other than ADF_OK). The oracle of the reference is tests/ref/adfref/recon.py,
   function reconstruct, whose vectors are tests/ref/vectors/recon.jsonl.

   The set statement, written out. Let the finite ball be the set a + N Zhat, with a = A/d the
   stored centre and N = H/d >= 0 the stored radius, and let [lo, hi] be the closed real interval.

   1. N = 0. The finite ball is the single rational a (SPEC 4.1: a ball of radius 0 is a single
      rational). There is one candidate if lo <= a <= hi, and none otherwise.
   2. N > 0. A rational q lies in a + N Zhat exactly when q - a lies in N Zhat meet Q = N Z
      (docs/proofs/precision.md Lemma 1, quoted in quotient.md P11, step 1), that is when
      q = a + N k for an integer k. The candidates are therefore the a + N k with
      lo <= a + N k <= hi. Since N > 0, dividing by N keeps the order, so the integers k that
      qualify are exactly

          ceil((lo - a)/N) <= k <= floor((hi - a)/N).

      The number of them is floor((hi - a)/N) - ceil((lo - a)/N) + 1 when the lower bound does not
      exceed the upper bound, and 0 otherwise: quotient.md P11, first item. Hence: exactly one
      candidate (the two bounds equal) gives ADF_OK, no candidate (the lower bound above the
      upper) gives ADF_NO_SOLUTION, and two or more (the lower bound strictly below the upper)
      gives ADF_NOT_UNIQUE. In particular an interval of width less than N has at most one
      candidate and an interval of width at least N has at least one, which are the last two
      clauses of P11, item 1; both follow from the same count, since two distinct candidates
      differ by at least N and any interval of length N contains one lattice point.
   3. lo > hi is the empty interval: no candidate, whatever the ball is.
   The floor and the ceiling are the integer divisions of a fraction in lowest terms with a
   positive denominator: fmpz_fdiv_q rounds towards -infinity and fmpz_cdiv_q towards +infinity
   (/usr/include/flint/fmpz.h:472-473). Nothing is enumerated, so the number of candidates is
   never computed as a count and never overflows: the cost is a constant number of divisions.

   The closed interval of a real ball. For the adele call the interval is [mid - rad, mid + rad]
   of the arb, whose end points are exact dyadic numbers (conventions 6.8). They are read with
   arb_get_interval_fmpz_2exp, which computes the exact interval of the ball in the form
   x = [A, B] * 2^exp (refs/src/flint-3.0.1/arb.rst:461-466); the two integers are then turned
   into rationals by a shift of exp. That function aborts on an infinite or NaN ball
   (arb.rst:468-470), so the predicate of conventions 5.5, arb_is_finite (arb.rst:606-609), is
   checked first; a ball that fails it is outside the contract and is answered ADF_DOMAIN.

   The size bound of decision M1-D3. The same passage of arb.rst (lines 472-477) warns that the
   function "will allocate a huge amount of memory to store the result if the exponent
   difference is huge ... It is recommended to check that the midpoint and radius of x both are
   within a reasonable range before calling this method". The check is made here, on the two
   exponents themselves, before the call, and the status is ADF_LIMIT, which conventions 3.2
   allows for reconstruction ("a size bound of an algorithm"): ADF_LIMIT when the midpoint or
   the radius is not zero and its binary exponent (arf.h, ARF_EXP; mag.h, MAG_EXP) is above
   ADF_RECON_EXP_MAX in absolute value, the value fixed by recon.h from M1-D3. A zero is
   exempt, since arf_is_zero and mag_is_zero are the predicates of the two zero values
   (arf.h:235-238, mag.h:229-233), and the exponent of a zero is 0 (arf.h:81 ARF_EXP_ZERO,
   mag.h:212 mag_zero) anyway. The two exponents of a finite ball are integers of any size
   (arf.rst:39: "Since exponents are bignums, overflow or underflow cannot occur"), so they
   are read with fmpz_cmp_si and never as machine words; the cost of the check is the size of
   those two integers only. With the two exponents within ADF_RECON_EXP_MAX, the exact end
   points need integers of at most about ADF_RECON_EXP_MAX + 2 bits (measured:
   lanes/m1-repair-recon/probe_exp.out, [source pending: the code of
   arb_get_interval_fmpz_2exp, which is not on disk]), which is what the header promises.

   Aliasing (conventions 4.1(1)): the output of adf_fball_reconstruct is an adf_rat and may be
   the same object as lo or hi, so every input is read into a temporary before the output is
   written, and the output is written only on ADF_OK. The finite ball x is of another type than
   the output and aliases no input (recon.h, "Aliasing").

   The local backend (adf_fball, backend ADF_LOCAL, work package 1.8) exists (src/fball_local.c,
   fball.h "Backends"), and the ball of adf_adele_reconstruct may therefore be local. The
   triple is read through adf_fball_get_fmpz3 alone, the accessor of fball.h that documents the
   canonical global triple of the set for a local value as well (fball.h:155-160: "For a local
   value this is (A0/g, K/g, d/g)"), and the centre and the radius are formed from that triple
   with adf_rat_set_fmpz2. The two other accessors of fball.h are not used: adf_fball_get_center
   and adf_fball_get_radius read the stored fields A/d and H/d, and the radius of a local value
   is K/d, not the stored H/d (fball.h:162-169, conventions 5.3). The set of candidates does
   not depend on the representative of the centre: conventions 5.2 says that any element of
   a + N Z is an equally valid centre of the same set, so the local and the global form of one
   ball give the same answer. The cost of the accessor is a copy for the global backend and, as
   its header says, a CRT recombination and a gcd for a local one. */

#include "adelefeld/recon.h"

#include <flint/arb.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>

/* floor and ceiling of a rational, the denominator of which is positive. */
static void
fmpq_floor_fmpz(fmpz_t r, const fmpq_t x)
{
    fmpz_fdiv_q(r, fmpq_numref(x), fmpq_denref(x));
}

static void
fmpq_ceil_fmpz(fmpz_t r, const fmpq_t x)
{
    fmpz_cdiv_q(r, fmpq_numref(x), fmpq_denref(x));
}

/* q = mn * 2^exp, exactly, as a rational, provided |exp| <= max_exp; the power of two goes into
   the numerator or into the denominator, so that no fmpq has to be built and then shifted, and
   for exp = 0 both branches give mn/1. Returns 1 and writes q, or returns 0 and leaves q
   untouched.

   The magnitude of exp is computed with fmpz_abs and is read as a machine word only after
   fmpz_cmp_si has shown that it is at most max_exp, so no fmpz_get_ui and no fmpz_get_si is
   applied to an exponent that was not first shown to fit, and no slong is ever negated (the
   negation of WORD_MIN is not representable, and the review of reviewer `arith` reports it for
   the exponent -2^63, docs/reviews/m1/arith/review.md R1). fmpq_canonicalise is called at the
   end because every function of the fmpq module assumes canonical input and produces canonical
   output (refs/src/flint-3.0.1/fmpq.rst:21-26: "all functions in the fmpq module assume that
   inputs are in canonical form, and produce outputs in canonical form"), and the numerator or
   the denominator written here is a shift of mn, which need not be odd. */
static int
fmpq_set_dyadic(fmpq_t q, const fmpz_t mn, const fmpz_t exp, slong max_exp)
{
    fmpz_t m;
    ulong sh;
    int ok;

    fmpz_init(m);
    fmpz_abs(m, exp);
    ok = (fmpz_cmp_si(m, max_exp) <= 0);
    sh = ok ? (ulong) fmpz_get_si(m) : 0;
    if (ok)
    {
        fmpz_t t;

        fmpz_init(t);
        if (fmpz_sgn(exp) >= 0)
        {
            fmpz_mul_2exp(t, mn, sh);
            fmpz_set(fmpq_numref(q), t);
            fmpz_one(fmpq_denref(q));
        }
        else
        {
            fmpz_one(t);
            fmpz_mul_2exp(t, t, sh);
            fmpz_set(fmpq_numref(q), mn);
            fmpz_set(fmpq_denref(q), t);
        }
        /* MUTANT: fmpq_canonicalise(q) removed */
        fmpz_clear(t);
    }
    fmpz_clear(m);
    return ok;
}

/* The exponents of the midpoint and of the radius of a finite real ball, both within
   ADF_RECON_EXP_MAX in absolute value, or one of them zero: 1. This is the test of decision
   M1-D3, made before any integer of the size of the end points is built, and it is the reason
   the call below is bounded. */
static int
arb_exponents_within_limit(const arb_t x)
{
    int mzero = arf_is_zero(arb_midref(x));
    int rzero = mag_is_zero(arb_radref(x));

    if (!mzero && (fmpz_cmp_si(ARF_EXPREF(arb_midref(x)), ADF_RECON_EXP_MAX) > 0
                   || fmpz_cmp_si(ARF_EXPREF(arb_midref(x)), -ADF_RECON_EXP_MAX) < 0))
        return 0;
    if (!rzero && (fmpz_cmp_si(MAG_EXPREF(arb_radref(x)), ADF_RECON_EXP_MAX) > 0
                   || fmpz_cmp_si(MAG_EXPREF(arb_radref(x)), -ADF_RECON_EXP_MAX) < 0))
        return 0;
    return 1;
}

int
adf_fball_reconstruct(adf_rat_t q, const adf_fball_t x, const adf_rat_t lo, const adf_rat_t hi)
{
    adf_rat_t a, N, clo, chi, c;
    fmpz_t A, H, d, kmin, kmax;
    int status;

    /* Step 3: an empty interval has no candidate. */
    if (fmpq_cmp(lo->q, hi->q) > 0)
        return ADF_NO_SOLUTION;

    adf_rat_init(a);
    adf_rat_init(N);
    adf_rat_init(clo);
    adf_rat_init(chi);
    adf_rat_init(c);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(kmin);
    fmpz_init(kmax);

    /* The centre and the radius, from the canonical triple of the set, which the accessor of
       fball.h computes for a local value as well (the radius of a local value is K/d, not the
       stored H/d, which is 0 for a raw local value). */
    adf_fball_get_fmpz3(A, H, d, x);
    status = adf_rat_set_fmpz2(a, A, d);
    if (status != ADF_OK)
        goto done;
    status = adf_rat_set_fmpz2(N, H, d);
    if (status != ADF_OK)
        goto done;
    if (adf_rat_is_zero(N))
    {
        /* Step 1: the ball is the point a. */
        if (fmpq_cmp(lo->q, a->q) > 0 || fmpq_cmp(a->q, hi->q) > 0)
            status = ADF_NO_SOLUTION;
        else
            adf_rat_set(c, a);
    }
    else
    {
        /* Step 2: the candidates are a + N k for ceil((lo - a)/N) <= k <= floor((hi - a)/N). */
        adf_rat_sub(clo, lo, a);
        adf_rat_div(clo, clo, N);
        adf_rat_sub(chi, hi, a);
        adf_rat_div(chi, chi, N);
        fmpq_ceil_fmpz(kmin, clo->q);
        fmpq_floor_fmpz(kmax, chi->q);
        if (fmpz_cmp(kmin, kmax) > 0)
        {
            status = ADF_NO_SOLUTION;
        }
        else if (fmpz_cmp(kmin, kmax) < 0)
        {
            status = ADF_NOT_UNIQUE;
        }
        else
        {
            /* fmpz_set of kmin into a one-denominator fraction, then a + N k. */
            fmpz_set(fmpq_numref(c->q), kmin);
            fmpz_one(fmpq_denref(c->q));
            adf_rat_mul(c, c, N);
            adf_rat_add(c, c, a);
        }
    }

    /* conventions 4.3: the output is written only on ADF_OK. */
    if (status == ADF_OK)
        adf_rat_set(q, c);

done:
    adf_rat_clear(a);
    adf_rat_clear(N);
    adf_rat_clear(clo);
    adf_rat_clear(chi);
    adf_rat_clear(c);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(kmin);
    fmpz_clear(kmax);
    return status;
}

int
adf_adele_reconstruct(adf_rat_t q, const adf_adele_t x)
{
    adf_rat_t lo, hi;
    fmpz_t a, b, exp;
    int status;

    /* Outside the contract (conventions 4.4, CV-09) when the real ball is not finite; checked
       because arb_get_interval_fmpz_2exp aborts on such a ball (arb.rst:468-470). The check
       comes first, since the exponent of an infinite midpoint is ARF_EXP_POS_INF, which is
       above ADF_RECON_EXP_MAX and would answer ADF_LIMIT for a ball that is outside the
       contract. */
    if (!arb_is_finite(x->inf))
        return ADF_DOMAIN;

    /* Decision M1-D3: ADF_LIMIT before anything of the size of the exact end points is
       built. */
    if (!arb_exponents_within_limit(x->inf))
        return ADF_LIMIT;

    fmpz_init(a);
    fmpz_init(b);
    fmpz_init(exp);
    adf_rat_init(lo);
    adf_rat_init(hi);
    arb_get_interval_fmpz_2exp(a, b, exp, x->inf);
    /* The exponent that the call returns lies between the smaller of the two exponents of the
       ball, less one, and the larger of them (measured on 15 configurations, exponents from
       -2^20 to 2^20 and 5: lanes/m1-repair-recon/probe_exp.out), so the bound of the test
       above plus one covers it, and the check in fmpq_set_dyadic does not reject. That is a
       measurement, not a quotation: [source pending: the code of arb_get_interval_fmpz_2exp,
       which is not on disk]. The check is kept because it is the only place where the
       exponent is read as a machine word, and if it ever rejected, the answer would be
       ADF_LIMIT, which conventions 3.2 allows, so the failure would be a refusal and not a
       wrong answer. */
    status = ADF_LIMIT;
    if (fmpq_set_dyadic(lo->q, a, exp, ADF_RECON_EXP_MAX + 1)
        && fmpq_set_dyadic(hi->q, b, exp, ADF_RECON_EXP_MAX + 1))
        status = adf_fball_reconstruct(q, &x->fin, lo, hi);
    adf_rat_clear(lo);
    adf_rat_clear(hi);
    fmpz_clear(a);
    fmpz_clear(b);
    fmpz_clear(exp);
    return status;
}
