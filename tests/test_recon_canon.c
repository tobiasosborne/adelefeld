/* tests/test_recon_canon.c: the canonical form of the bounds of src/recon.c.

   The statement under test is `fmpq_canonicalise(q);` at the end of the static function
   fmpq_set_dyadic (src/recon.c:141 today, the line 111 of the report of lanes/m1-testgaps
   section 8.3). TJO decided to keep it (progress of this lane) and to pin it with a test.

   Why a test has to look inside: the public result of adf_adele_reconstruct does not depend on the
   stored form of the bounds. The lane that measured that (lanes/m1-testgaps/report.md, section
   8.4) ran 10584 balls through the public function with and without the line and got the same
   hash of every status and every returned rational, and the reason is in the code: the bounds are
   read by fmpq_cmp (src/recon.c:180 and 208), by adf_rat_sub and adf_rat_div (src/recon.c:215-218)
   and by fmpq_ceil_fmpz and fmpq_floor_fmpz (src/recon.c:219-220), which are fmpz_cdiv_q and
   fmpz_fdiv_q and are homogeneous, so a numerator and a denominator with a common factor give the
   same integer; and the value that is written out is built from `a`, `N` and `kmin`, which are
   canonical whatever the bounds are. A test of the public result is therefore green with and
   without the line; the last test of this file is that test, and it says so.

   The line is kept because the stored form is a contract of the fmpq module: every function of it
   assumes canonical input and produces canonical output (refs/src/flint-3.0.1/fmpq.rst:21-26), and
   the comment block of fmpq_set_dyadic (src/recon.c:104-115) gives that as the reason for the call.
   The bound is a rational of the form mn * 2^exp: for exp >= 0 the branch writes the numerator
   mn * 2^|exp| over the denominator 1, which is canonical; for exp < 0 it writes mn over
   2^|exp|, which is not canonical whenever mn is even. So the line is the only thing that makes
   the second branch canonical, and the tests below are about that branch.

   The file includes src/recon.c, as tests/test_common.c:22 includes src/common.c, so that the
   static fmpq_set_dyadic can be called; the library is built without the test and the two
   reconstruct functions of this file are the ones of the same source text, compiled here. Nothing
   else of the tree is changed, and the library archive is not modified: the linker does not pull
   the member src/recon.o, because every symbol it exports is defined in this file. */

#include "../src/recon.c"

#include <stdio.h>
#include <string.h>

#include "test_runner.h"

/* A decimal string of an fmpq for the message of a check: eight buffers of 96 bytes, no
   allocation, and only the message of a failed check is formatted. */
static const char *
qs(const fmpq_t q)
{
    static char buf[8][96];
    static int slot = 0;
    char *b = buf[slot];

    slot = (slot + 1) % 8;
    _fmpq_get_str(b, 10, fmpq_numref(q), fmpq_denref(q));
    return b;
}

/* A decimal string of an fmpz, for the message of a check. */
static const char *
zs(const fmpz_t z)
{
    static char buf[4][96];
    static int slot = 0;
    char *b = buf[slot];

    slot = (slot + 1) % 4;
    fmpz_get_str(b, 10, z);
    return b;
}

/* The pair (num, den) as it is stored, without any reduction: fmpq_set_si and the other
   constructors of FLINT reduce, and the test needs a value that is not in canonical form. */
static void
set_rawnum_den(fmpq_t q, slong num, slong den)
{
    fmpz_set_si(fmpq_numref(q), num);
    fmpz_set_si(fmpq_denref(q), den);
}

/* Is the fmpq in canonical form, that is gcd(|num|, den) = 1 with den > 0? The test computes the
   gcd itself and does not ask the library, because the library function fmpq_canonicalise is the
   statement under test. den > 0 is the other half of the canonical form (fmpq.rst:68-80). */
static int
is_canonical(const fmpq_t q)
{
    fmpz_t g;
    int ok;

    if (fmpz_sgn(fmpq_denref(q)) <= 0)
        return 0;
    fmpz_init(g);
    fmpz_gcd(g, fmpq_numref(q), fmpq_denref(q));
    ok = fmpz_is_one(g);
    fmpz_clear(g);
    return ok;
}

/* The value mn * 2^exp as an exact rational, computed here with fmpz, so that the expected value
   of a test does not come from the function under test. */
static void
dyadic_value(fmpq_t want, const fmpz_t mn, const slong exp)
{
    fmpz_t t, u, one;

    fmpz_init(t);
    fmpz_init(u);
    fmpz_init(one);
    fmpz_one(one);
    fmpz_set(t, mn);
    if (exp >= 0)
    {
        fmpz_mul_2exp(t, t, (ulong) exp);
        fmpq_set_fmpz_frac(want, t, one);
    }
    else
    {
        /* the value is mn / 2^|exp|, and the numerator stays mn */
        fmpz_one(u);
        fmpz_mul_2exp(u, u, (ulong) (-exp));
        fmpq_set_fmpz_frac(want, t, u);
    }
    fmpz_clear(t);
    fmpz_clear(u);
    fmpz_clear(one);
    fmpq_canonicalise(want);
}

/* ---- 1. the value of the bound ---- */

ADF_TEST(the_bound_is_the_value_mn_times_two_to_the_exp)
{
    /* exp >= 0, the branch that writes the numerator only, and exp < 0, the branch that writes
       the denominator. */
    static const struct
    {
        const char *mn;
        slong exp;
    } cases[] = {
        { "1", 0 }, { "1", 1 }, { "1", 64 }, { "3", 5 }, { "-7", 3 }, { "0", 0 }, { "0", 4 },
        { "5", -1 }, { "6", -1 }, { "12", -3 }, { "-12", -3 }, { "0", -1 }, { "1024", -10 },
        { "1", -1 }, { "1", -64 },
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        fmpq_t q, want;
        fmpz_t mn, exp;

        fmpq_init(q);
        fmpq_init(want);
        fmpz_init(mn);
        fmpz_set_str(mn, cases[i].mn, 10);
        fmpz_init_set_si(exp, cases[i].exp);
        dyadic_value(want, mn, cases[i].exp);
        ADF_CHECK_MSG(fmpq_set_dyadic(q, mn, exp, 1000) == 1, "the case %lu (%s * 2^%ld) was refused",
                      (unsigned long) i + 1, cases[i].mn, (long) cases[i].exp);
        ADF_CHECK_MSG(fmpq_equal(q, want), "the case %lu (%s * 2^%ld) is %s, not %s",
                      (unsigned long) i + 1, cases[i].mn, (long) cases[i].exp, qs(q),
                      qs(want));
        fmpz_clear(mn);
        fmpz_clear(exp);
        fmpq_clear(q);
        fmpq_clear(want);
    }
}

/* ---- 2. the canonical form, which is the statement under test ---- */

ADF_TEST(the_bound_is_in_canonical_form)
{
    /* Every case of the table above, and the check is on the stored form: gcd(num, den) = 1 and
       den > 0. For mn = 6 and exp = -1 the branch writes 6/2, which the line reduces to 3/1. */
    static const struct
    {
        const char *mn;
        slong exp;
        const char *why;
    } cases[] = {
        { "6", -1, "6/2 is reduced to 3/1" },   { "12", -3, "12/8 is reduced to 3/2" },
        { "0", -1, "0/2 is reduced to 0/1" },  { "1024", -10, "1024/1024 is reduced to 1/1" },
        { "-12", -3, "the sign stays in the numerator" }, { "2", -1, "2/2 is reduced to 1/1" },
        { "18", -2, "18/4 is reduced to 9/2" }, { "1", -1, "already in lowest terms" },
        { "5", -1, "already in lowest terms" }, { "6", 0, "the exponent 0 is the value itself" },
        { "6", 3, "denominator 1, already canonical" },
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        fmpq_t q;
        fmpz_t mn, exp, g;

        fmpq_init(q);
        fmpz_init(mn);
        fmpz_set_str(mn, cases[i].mn, 10);
        fmpz_init_set_si(exp, cases[i].exp);
        ADF_CHECK(fmpq_set_dyadic(q, mn, exp, 1000) == 1);
        fmpz_init(g);
        fmpz_gcd(g, fmpq_numref(q), fmpq_denref(q));
        ADF_CHECK_MSG(fmpz_is_one(g), "%s * 2^%ld is stored as %s, whose numerator and "
                                   "denominator have the gcd %s, not 1 (%s)", cases[i].mn,
                      (long) cases[i].exp, qs(q), zs(g), cases[i].why);
        ADF_CHECK_MSG(fmpz_sgn(fmpq_denref(q)) > 0, "the denominator of %s * 2^%ld is not positive",
                      cases[i].mn, (long) cases[i].exp);
        ADF_CHECK_MSG(is_canonical(q), "the bound %s * 2^%ld is not canonical (%s)", cases[i].mn,
                      (long) cases[i].exp, cases[i].why);
        fmpz_clear(g);
        fmpz_clear(mn);
        fmpz_clear(exp);
        fmpq_clear(q);
    }
}

ADF_TEST(a_bound_that_is_not_canonical_on_entry_comes_out_canonical)
{
    /* The two callers of the helper pass a value that adf_rat_init has just set to 0/1, so the
       entry form is canonical; the helper must not rely on that. The entry state is set here to a
       value with a common factor, and the result must be canonical whatever it was. This test is
       red without the line as well: 6/1 with exp = -1 would be stored as 6/2. */
    fmpq_t q;
    fmpz_t mn, exp;

    fmpz_init_set_si(mn, 6);
    fmpz_init_set_si(exp, -1);
    fmpq_init(q);

    set_rawnum_den(q, 4, 2);
    ADF_CHECK(!is_canonical(q));
    ADF_CHECK(fmpq_set_dyadic(q, mn, exp, 1000) == 1);
    ADF_CHECK_MSG(is_canonical(q), "6 * 2^-1 into the value 4/2 is stored as %s",
                  qs(q));
    ADF_CHECK(fmpq_equal_si(q, 3));

    set_rawnum_den(q, -8, 4);
    ADF_CHECK(!is_canonical(q));
    ADF_CHECK(fmpq_set_dyadic(q, mn, exp, 1000) == 1);
    ADF_CHECK_MSG(is_canonical(q), "6 * 2^-1 into the value -8/4 is stored as %s",
                  qs(q));
    ADF_CHECK(fmpq_equal_si(q, 3));

    set_rawnum_den(q, 5, 1);
    ADF_CHECK(is_canonical(q));
    ADF_CHECK(fmpq_set_dyadic(q, mn, exp, 1000) == 1);
    ADF_CHECK_MSG(is_canonical(q), "6 * 2^-1 into the value 5/1 is stored as %s",
                  qs(q));
    ADF_CHECK(fmpq_equal_si(q, 3));

    fmpz_clear(mn);
    fmpz_clear(exp);
    fmpq_clear(q);
}

/* ---- 3. the refusal leaves the bound untouched ---- */

ADF_TEST(the_refusal_of_an_exponent_beyond_the_bound_leaves_it_untouched)
{
    /* fmpq_set_dyadic returns 0 and writes nothing when |exp| > max_exp (src/recon.c:124-126).
       The comparison of the form is a comparison of the whole rational, so the untouched rule is
       checked on the stored pair and not only on the value. */
    static const slong exps[4] = { 1001, -1001, 5000, -5000 };
    size_t i;

    for (i = 0; i < 4; i++)
    {
        fmpq_t q, before;
        fmpz_t mn, exp;

        fmpq_init(q);
        fmpq_init(before);
        fmpz_init_set_si(mn, 6);
        fmpz_init_set_si(exp, exps[i]);
        set_rawnum_den(q, 4, 2);
        fmpq_set(before, q);
        ADF_CHECK_MSG(fmpq_set_dyadic(q, mn, exp, 1000) == 0, "the exponent %ld was not refused",
                      (long) exps[i]);
        ADF_CHECK_MSG(fmpz_equal(fmpq_numref(q), fmpq_numref(before))
                          && fmpz_equal(fmpq_denref(q), fmpq_denref(before)),
                      "the exponent %ld was refused but the bound was written", (long) exps[i]);
        fmpz_clear(mn);
        fmpz_clear(exp);
        fmpq_clear(q);
        fmpq_clear(before);
    }
    /* The bound itself: |exp| = max_exp is inside, |exp| = max_exp + 1 is outside. */
    {
        fmpq_t q;
        fmpz_t mn, exp;

        fmpq_init(q);
        fmpz_init_set_si(mn, 3);
        fmpz_init_set_si(exp, 1000);
        ADF_CHECK(fmpq_set_dyadic(q, mn, exp, 1000) == 1);
        fmpz_set_si(exp, -1000);
        ADF_CHECK(fmpq_set_dyadic(q, mn, exp, 1000) == 1);
        fmpz_set_si(exp, 1001);
        ADF_CHECK(fmpq_set_dyadic(q, mn, exp, 1000) == 0);
        fmpz_clear(mn);
        fmpz_clear(exp);
        fmpq_clear(q);
    }
}

/* ---- 4. the public result, which the line does not change ---- */

ADF_TEST(the_public_reconstruction_returns_a_canonical_rational)
{
    /* Green with the line and without it: the value of the public result is the same either way
       (the reason is in the comment of this file, the measurement is in
       lanes/m1-testgaps/report.md, section 8.4). It is here so that the form of the output of
       adf_adele_reconstruct is a statement of the tree and not only of the helper.

       Each row is a closed interval [lo, hi] of reals, the triple (A, H, d) of the finite part,
       and the one candidate a + N k that the interval admits (docs/proofs/quotient.md
       Proposition 11, line 257). The expected value is in the last column and is a value of the
       specification, not an output of the code. */
    static const struct
    {
        const char *lo;
        const char *hi;
        slong A, H, d;
        const char *want;
    } cases[] = {
        { "1.25", "1.75", 3, 2, 2, "3/2" },        /* a = 3/2, N = 1, k = 0 */
        { "0", "1", 1, 4, 4, "1/4" },              /* a = 1/4, N = 1, k = 0 */
        { "0.1", "0.3", 1, 4, 10, "1/10" },        /* a = 1/10, N = 2/5, k = 0 */
        { "1.5", "1.5", 3, 0, 2, "3/2" },          /* a point, N = 0 */
        { "1.05", "1.45", 5, 2, 4, "5/4" },        /* a = 5/4, N = 1/2, k = 0 */
        { "-0.8125", "-0.6875", -3, 2, 4, "-3/4" }, /* a = -3/4, N = 1/2, k = 0 */
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        adf_adele_t x;
        adf_rat_t q;
        arb_t alo, ahi;
        fmpq_t want;
        fmpz_t a, h, d;

        arb_init(x->inf);
        adf_fball_init(&x->fin);
        adf_rat_init(q);
        fmpq_init(want);
        arb_init(alo);
        arb_init(ahi);
        /* an exact decimal as a point ball, so that the end points are the decimals of the table
           and not the nearest binary doubles of a division */
        ADF_CHECK(arb_set_str(alo, cases[i].lo, 10) == 0);
        ADF_CHECK(arb_set_str(ahi, cases[i].hi, 10) == 0);
        arb_set_interval_arf(x->inf, arb_midref(alo), arb_midref(ahi), 128);
        fmpz_init_set_si(a, cases[i].A);
        fmpz_init_set_si(h, cases[i].H);
        fmpz_init_set_si(d, cases[i].d);
        ADF_CHECK(adf_fball_set_fmpz3(&x->fin, a, h, d) == ADF_OK);
        fmpz_clear(a);
        fmpz_clear(h);
        fmpz_clear(d);
        fmpq_set_str(want, cases[i].want, 10);

        ADF_CHECK_MSG(adf_adele_reconstruct(q, x) == ADF_OK, "the case %lu ([%s, %s], %ld, %ld, %ld) "
                                                            "was not reconstructed",
                      (unsigned long) i + 1, cases[i].lo, cases[i].hi, (long) cases[i].A,
                      (long) cases[i].H, (long) cases[i].d);
        ADF_CHECK_MSG(is_canonical(q->q), "the case %lu gives %s, which is not canonical",
                      (unsigned long) i + 1, qs(q->q));
        ADF_CHECK_MSG(fmpq_equal(q->q, want), "the case %lu gives %s, not %s", (unsigned long) i + 1,
                      qs(q->q), qs(want));
        /* The candidate lies in the interval of the ball: the promise of the proposition. */
        ADF_CHECK_MSG(arb_contains_fmpq(x->inf, q->q), "the case %lu gives %s, which is not in the "
                                                       "ball", (unsigned long) i + 1, qs(q->q));
        fmpq_clear(want);
        arb_clear(alo);
        arb_clear(ahi);
        adf_rat_clear(q);
        adf_fball_clear(&x->fin);
        arb_clear(x->inf);
    }
}
