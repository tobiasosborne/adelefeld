/* tests/test_fball_overwrite.c: every constructor and setter of adelefeld/fball.h on a value that
   already holds other data.

   The claim of this file: a constructor of fball.h writes every field of the output, that is A, H,
   d, backend, mctx and res, and leaves a value that satisfies predicate G of docs/conventions.md
   5.2 (adf_fball_is_canonical). "Already holds other data" is a ball with a large A, H > 0 and
   d > 1, so that every field of every expected result differs from the field the value held
   before: an implementation that leaves one field of the earlier value behind is caught.

   The lines of src/fball.c this is written for are the constructors adf_fball_zero (line 256),
   adf_fball_one (268), adf_fball_set_si (280), adf_fball_set_fmpz (292), adf_fball_set_rat
   (304), adf_fball_set (163) and the helper fb_store (72) reached by adf_fball_set_fmpz3 (316),
   adf_fball_set_center_radius (336) and adf_fball_canonicalise (350). Dropping one statement of
   one of them (the `drop_call` mutation of tools/mutate/mutate.py) leaves the old value in that
   field; the check below reads the field, so the mutant is killed.

   Not covered here: the `flint_free(x->res)` of fb_store (line 75) and of adf_fball_set (168),
   adf_fball_clear (155) and of the five constructors (lines 264, 276, 288, 300, 312). A drop of
   those is a leak of the residue array of a local value, and a local value can only be built by
   the local backend of work package 1.8, which does not exist yet. This file does not build one by
   hand. */

#include <stdio.h>
#include <string.h>

#include <adelefeld/fball.h>

#include "test_runner.h"

/* --------------------------------------------------------------- helpers */

/* x = (2^200 + 2^210 Zhat)/3^40: a large A, H > 0, d > 1, all three already canonical, since
   2^200 < 2^210 and gcd(2^200, 2^210, 3^40) = 1. */
static void
load_ball(adf_fball_t x)
{
    fmpz_t A, H, d;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_set_ui(A, 1);
    fmpz_mul_2exp(A, A, 200);
    fmpz_set_ui(H, 1);
    fmpz_mul_2exp(H, H, 210);
    fmpz_set_ui(d, 3);
    fmpz_pow_ui(d, d, 40);
    ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    ADF_CHECK_MSG(adf_fball_is_canonical(x), "the loaded value is not canonical");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

/* The whole state of x, as a string, for a failure message. */
static void
describe(char * buf, const adf_fball_t x)
{
    char a[128], h[128], d[128], * sa, * sh, * sd;

    sa = fmpz_get_str(NULL, 10, x->A);
    sh = fmpz_get_str(NULL, 10, x->H);
    sd = fmpz_get_str(NULL, 10, x->d);
    flint_sprintf(a, "%s", sa);
    flint_sprintf(h, "%s", sh);
    flint_sprintf(d, "%s", sd);
    flint_free(sa);
    flint_free(sh);
    flint_free(sd);
    flint_sprintf(buf, "(%s, %s, %s) backend %d mctx %s res %s canonical %d", a, h, d,
                   x->backend, x->mctx == NULL ? "NULL" : "set", x->res == NULL ? "NULL" : "set",
                   adf_fball_is_canonical(x));
}

/* Every field of x is the expected one, and x satisfies predicate G (conventions 5.2). The claim
   is about the fields, not about a function. */
static void
check_fields(const adf_fball_t x, const fmpz_t A, const fmpz_t H, const fmpz_t d,
             const char * what)
{
    char buf[512];
    int fields_match_the_expected_triple;

    fields_match_the_expected_triple = fmpz_equal(x->A, A) && fmpz_equal(x->H, H) &&
                                       fmpz_equal(x->d, d);
    fields_match_the_expected_triple = fields_match_the_expected_triple &&
                                       x->backend == ADF_GLOBAL && x->mctx == NULL &&
                                       x->res == NULL && adf_fball_is_canonical(x);
    describe(buf, x);
    ADF_CHECK_MSG(fields_match_the_expected_triple, "after %s the value is %s", what, buf);
}

/* The three integers every expected triple of this file is built from, as fmpz. */
static void
expect(fmpz_t A, fmpz_t H, fmpz_t d, slong a, slong h, slong dd)
{
    fmpz_init_set_si(A, a);
    fmpz_init_set_si(H, h);
    fmpz_init_set_si(d, dd);
}

/* ------------------------------------------------------------------ zero */

ADF_TEST(zero_writes_every_field_of_a_value_that_holds_other_data)
{
    adf_fball_t x;
    fmpz_t A, H, d;

    adf_fball_init(x);
    load_ball(x);
    expect(A, H, d, 0, 0, 1);
    adf_fball_zero(x);
    check_fields(x, A, H, d, "adf_fball_zero");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
}

/* ------------------------------------------------------------------- one */

ADF_TEST(one_writes_every_field_of_a_value_that_holds_other_data)
{
    adf_fball_t x;
    fmpz_t A, H, d;

    adf_fball_init(x);
    load_ball(x);
    expect(A, H, d, 1, 0, 1);
    adf_fball_one(x);
    check_fields(x, A, H, d, "adf_fball_one");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
}

/* ----------------------------------------------------------------- set_si */

ADF_TEST(set_si_writes_every_field_of_a_value_that_holds_other_data)
{
    adf_fball_t x;
    fmpz_t A, H, d;

    adf_fball_init(x);
    load_ball(x);
    expect(A, H, d, -5, 0, 1);
    adf_fball_set_si(x, -5);
    check_fields(x, A, H, d, "adf_fball_set_si(-5)");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
}

/* --------------------------------------------------------------- set_fmpz */

ADF_TEST(set_fmpz_writes_every_field_of_a_value_that_holds_other_data)
{
    adf_fball_t x;
    fmpz_t A, H, d, n;

    adf_fball_init(x);
    load_ball(x);
    fmpz_init(n);
    fmpz_set_ui(n, 1);
    fmpz_mul_2exp(n, n, 250);
    fmpz_add_ui(n, n, 7);
    fmpz_neg(n, n);
    expect(A, H, d, 0, 0, 1);
    fmpz_set(A, n);
    adf_fball_set_fmpz(x, n);
    check_fields(x, A, H, d, "adf_fball_set_fmpz(-(2^250 + 7))");
    fmpz_clear(n);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
}

/* ---------------------------------------------------------------- set_rat */

ADF_TEST(set_rat_writes_every_field_of_a_value_that_holds_other_data)
{
    adf_fball_t x;
    adf_rat_t q;
    fmpz_t A, H, d;

    adf_fball_init(x);
    adf_rat_init(q);
    load_ball(x);
    fmpq_set_si(q->q, -7, 11);
    expect(A, H, d, -7, 0, 11);
    adf_fball_set_rat(x, q);
    check_fields(x, A, H, d, "adf_fball_set_rat(-7/11)");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_rat_clear(q);
    adf_fball_clear(x);
}

/* -------------------------------------------------------------- set_fmpz3 */

ADF_TEST(set_fmpz3_writes_every_field_of_a_value_that_holds_other_data)
{
    adf_fball_t x;
    fmpz_t A, H, d, rA, rH, rd;

    adf_fball_init(x);
    load_ball(x);
    fmpz_init_set_si(rA, 35);
    fmpz_init_set_si(rH, 30);
    fmpz_init_set_si(rd, 6);
    /* (35 + 30 Zhat)/6 = (5 + 30 Zhat)/6, so the canonical triple is (5, 30, 6). */
    expect(A, H, d, 5, 30, 6);
    ADF_CHECK(adf_fball_set_fmpz3(x, rA, rH, rd) == ADF_OK);
    check_fields(x, A, H, d, "adf_fball_set_fmpz3(35, 30, 6)");
    fmpz_clear(rA);
    fmpz_clear(rH);
    fmpz_clear(rd);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
}

/* ------------------------------------------------------- set_center_radius */

ADF_TEST(set_center_radius_writes_every_field_of_a_value_that_holds_other_data)
{
    adf_fball_t x;
    adf_rat_t c, N;
    fmpz_t A, H, d;

    adf_fball_init(x);
    adf_rat_init(c);
    adf_rat_init(N);
    load_ball(x);
    fmpq_set_si(c->q, 1, 3);
    fmpq_set_si(N->q, 1, 2);
    /* 1/3 + (1/2) Zhat = (2 + 3 Zhat)/6, already canonical. */
    expect(A, H, d, 2, 3, 6);
    ADF_CHECK(adf_fball_set_center_radius(x, c, N) == ADF_OK);
    check_fields(x, A, H, d, "adf_fball_set_center_radius(1/3, 1/2)");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_rat_clear(c);
    adf_rat_clear(N);
    adf_fball_clear(x);
}

/* -------------------------------------------------------------------- set */

ADF_TEST(set_copies_every_field_over_a_value_that_holds_other_data)
{
    adf_fball_t x, y;
    fmpz_t A, H, d;

    adf_fball_init(x);
    adf_fball_init(y);
    load_ball(y);
    fmpz_init_set_si(A, 7);
    fmpz_init_set_si(H, 9);
    fmpz_init_set_si(d, 4);
    ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    adf_fball_set(y, x);
    check_fields(y, A, H, d, "adf_fball_set");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
    adf_fball_clear(y);
}

/* ----------------------------------------------------------- canonicalise */

ADF_TEST(canonicalise_writes_every_field_of_a_raw_triple_over_other_data)
{
    adf_fball_t x;
    fmpz_t A, H, d;

    adf_fball_init(x);
    load_ball(x);
    /* The raw triple (2 * 2^200 + 2^210 Zhat)/(2 * 3^40) is not canonical: its gcd is 2. After
       the reduction it is (2^200 + 2^205 Zhat)/3^40. A and d are the fields of the value that
       was there before, so only H tells the two apart; that is enough, because the reduction
       halves H. */
    fmpz_mul_2exp(x->A, x->A, 1);
    fmpz_mul_2exp(x->H, x->H, 1);
    fmpz_mul_2exp(x->d, x->d, 1);
    x->backend = ADF_GLOBAL;
    x->mctx = NULL;
    x->res = NULL;
    fmpz_init_set(A, x->A);
    fmpz_tdiv_q_2exp(A, A, 1);
    fmpz_init(H);
    fmpz_init_set(H, x->H);
    fmpz_tdiv_q_2exp(H, H, 1);
    fmpz_init(d);
    fmpz_init_set(d, x->d);
    fmpz_tdiv_q_2exp(d, d, 1);
    ADF_CHECK(adf_fball_canonicalise(x) == ADF_OK);
    check_fields(x, A, H, d, "adf_fball_canonicalise");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
}

/* ------------------------------------------------------------------- swap */

ADF_TEST(swap_moves_every_field)
{
    adf_fball_t x, y;
    fmpz_t A, H, d, B, K, e;

    adf_fball_init(x);
    adf_fball_init(y);
    load_ball(x);
    fmpz_init_set_si(A, 7);
    fmpz_init_set_si(H, 9);
    fmpz_init_set_si(d, 4);
    ADF_CHECK(adf_fball_set_fmpz3(y, A, H, d) == ADF_OK);
    fmpz_init(B);
    fmpz_set_ui(B, 1);
    fmpz_mul_2exp(B, B, 200);
    fmpz_init(K);
    fmpz_set_ui(K, 1);
    fmpz_mul_2exp(K, K, 210);
    fmpz_init(e);
    fmpz_set_ui(e, 3);
    fmpz_pow_ui(e, e, 40);
    adf_fball_swap(x, y);
    check_fields(x, A, H, d, "adf_fball_swap");
    check_fields(y, B, K, e, "adf_fball_swap");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(B);
    fmpz_clear(K);
    fmpz_clear(e);
    adf_fball_clear(x);
    adf_fball_clear(y);
}

/* Every constructor once more, each on the value that the previous constructor left, with the
   expected triple written out again: no constructor may depend on what the value held before. */
ADF_TEST(the_constructors_do_not_depend_on_the_earlier_value)
{
    adf_fball_t x;
    fmpz_t A, H, d, rA, rH, rd;
    adf_rat_t c, N;

    adf_fball_init(x);
    adf_rat_init(c);
    adf_rat_init(N);
    load_ball(x);
    fmpz_init_set_si(rA, 35);
    fmpz_init_set_si(rH, 30);
    fmpz_init_set_si(rd, 6);
    fmpq_set_si(c->q, 1, 3);
    fmpq_set_si(N->q, 1, 2);

    expect(A, H, d, 0, 0, 1);
    adf_fball_zero(x);
    check_fields(x, A, H, d, "adf_fball_zero after a loaded value");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);

    expect(A, H, d, 1, 0, 1);
    adf_fball_one(x);
    check_fields(x, A, H, d, "adf_fball_one after adf_fball_zero");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);

    expect(A, H, d, 5, 30, 6);
    ADF_CHECK(adf_fball_set_fmpz3(x, rA, rH, rd) == ADF_OK);
    check_fields(x, A, H, d, "adf_fball_set_fmpz3 after adf_fball_one");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);

    expect(A, H, d, 2, 3, 6);
    ADF_CHECK(adf_fball_set_center_radius(x, c, N) == ADF_OK);
    check_fields(x, A, H, d, "adf_fball_set_center_radius after a set_fmpz3");
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);

    fmpz_clear(rA);
    fmpz_clear(rH);
    fmpz_clear(rd);
    adf_rat_clear(c);
    adf_rat_clear(N);
    adf_fball_clear(x);
}
