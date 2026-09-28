/* tests/test_fball_vectors.c: adf_fball against the JSON-lines vector oracle.

   Every file of tests/ref/vectors/ that concerns the finite ball is run completely:
   canonical, add, sub, neg, mul, scale, predicates, compare, membership
   (tests/ref/README.md; tests/README.md). The vectors are the oracle of work package 1.1,
   written from docs/proofs/precision.md; the C code does not read the Python reference. */

#include <stdio.h>
#include <string.h>

#include <adelefeld/fball.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* Read the integer in the JSON value into v (which is already initialised). */
static int
json_fmpz(fmpz_t v, const jsonl_value * j, jsonl_error_t * err)
{
    const char * text = jsonl_int_text(j, err);
    if (text == NULL)
        return 0;
    return fmpz_set_str(v, text, 10) == 0;
}

/* Build the ball {A,H,d} of a JSON object of the vector format, canonicalised. */
static int
json_ball(adf_fball_t x, const jsonl_value * o, jsonl_error_t * err)
{
    const jsonl_value *jA, *jH, *jd;
    fmpz_t A, H, d;
    int ok;

    if (!jsonl_field(o, "A", &jA, err) || !jsonl_field(o, "H", &jH, err) ||
        !jsonl_field(o, "d", &jd, err))
        return 0;
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    ok = json_fmpz(A, jA, err) && json_fmpz(H, jH, err) && json_fmpz(d, jd, err) &&
         adf_fball_set_fmpz3(x, A, H, d) == ADF_OK;
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    return ok;
}

/* Build the rational {num,den} of the vector format. */
static int
json_rat(adf_rat_struct * q, const jsonl_value * o, jsonl_error_t * err)
{
    const jsonl_value *jn, *jd;
    fmpz_t n, d;
    int ok;

    if (!jsonl_field(o, "num", &jn, err) || !jsonl_field(o, "den", &jd, err))
        return 0;
    fmpz_init(n);
    fmpz_init(d);
    ok = json_fmpz(n, jn, err) && json_fmpz(d, jd, err);
    if (ok)
        fmpq_set_fmpz_frac(q->q, n, d);
    fmpz_clear(n);
    fmpz_clear(d);
    return ok;
}

/* Compare the fields of x with the canonical triple {A,H,d} of a JSON object. */
static int
ball_equals_json(const adf_fball_t x, const jsonl_value * o, jsonl_error_t * err)
{
    const jsonl_value *jA, *jH, *jd;
    fmpz_t A, H, d;
    int ok;

    if (!jsonl_field(o, "A", &jA, err) || !jsonl_field(o, "H", &jH, err) ||
        !jsonl_field(o, "d", &jd, err))
        return 0;
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    ok = json_fmpz(A, jA, err) && json_fmpz(H, jH, err) && json_fmpz(d, jd, err) &&
         fmpz_equal(x->A, A) && fmpz_equal(x->H, H) && fmpz_equal(x->d, d);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    return ok;
}

/* Compare the fields of x with the canonical triple [A,H,d] of a JSON array. */
static int
ball_equals_array(const adf_fball_t x, const jsonl_value * arr, jsonl_error_t * err)
{
    const jsonl_value * jA = jsonl_at(arr, 0, err);
    const jsonl_value * jH = jsonl_at(arr, 1, err);
    const jsonl_value * jd = jsonl_at(arr, 2, err);
    fmpz_t A, H, d;
    int ok;

    if (jA == NULL || jH == NULL || jd == NULL)
        return 0;
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    ok = json_fmpz(A, jA, err) && json_fmpz(H, jH, err) && json_fmpz(d, jd, err) &&
         fmpz_equal(x->A, A) && fmpz_equal(x->H, H) && fmpz_equal(x->d, d);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    return ok;
}

/* ------------------------------------------------------------- canonical */

ADF_TEST(vectors_canonical)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/canonical.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * input, * jA, * jH, * jd, * result;
        fmpz_t A, H, d;
        adf_fball_t x;

        ADF_CHECK(jsonl_field(rec, "input", &input, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "result", &result, &err) == 1);
        jA = jsonl_at(input, 0, &err);
        jH = jsonl_at(input, 1, &err);
        jd = jsonl_at(input, 2, &err);
        ADF_CHECK(jA != NULL && jH != NULL && jd != NULL);
        fmpz_init(A);
        fmpz_init(H);
        fmpz_init(d);
        ADF_CHECK(json_fmpz(A, jA, &err));
        ADF_CHECK(json_fmpz(H, jH, &err));
        ADF_CHECK(json_fmpz(d, jd, &err));
        adf_fball_init(x);
        if (fmpz_is_zero(d) || fmpz_sgn(H) < 0)
        {
            ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_DOMAIN);
        }
        else
        {
            ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
            ADF_CHECK(adf_fball_is_canonical(x));
            ADF_CHECK_MSG(ball_equals_array(x, result, &err), "canonical line %lu",
                          jsonl_line_of(rec));
        }
        adf_fball_clear(x);
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
    }
    jsonl_close(f);
}

/* ---------------------------------------------------- binary operations */

static void
run_binary(const char * path, void (*op)(adf_fball_t, const adf_fball_t,
                                         const adf_fball_t))
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open(path, &f, &err) == 1, "%s", jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * ja, * jb, * result;
        adf_fball_t x, y, z;

        ADF_CHECK(jsonl_field(rec, "a", &ja, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "b", &jb, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "result", &result, &err) == 1);
        adf_fball_init(x);
        adf_fball_init(y);
        adf_fball_init(z);
        ADF_CHECK(json_ball(x, ja, &err));
        ADF_CHECK(json_ball(y, jb, &err));
        op(z, x, y);
        ADF_CHECK(adf_fball_is_canonical(z));
        ADF_CHECK_MSG(ball_equals_json(z, result, &err), "%s line %lu", path,
                      jsonl_line_of(rec));
        adf_fball_clear(x);
        adf_fball_clear(y);
        adf_fball_clear(z);
    }
    jsonl_close(f);
}

ADF_TEST(vectors_add)
{
    run_binary("tests/ref/vectors/add.jsonl", adf_fball_add);
}

ADF_TEST(vectors_sub)
{
    run_binary("tests/ref/vectors/sub.jsonl", adf_fball_sub);
}

ADF_TEST(vectors_mul)
{
    run_binary("tests/ref/vectors/mul.jsonl", adf_fball_mul);
}

/* -------------------------------------------------------- unary operations */

ADF_TEST(vectors_neg)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/neg.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * ja, * result;
        adf_fball_t x, y;

        ADF_CHECK(jsonl_field(rec, "a", &ja, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "result", &result, &err) == 1);
        adf_fball_init(x);
        adf_fball_init(y);
        ADF_CHECK(json_ball(x, ja, &err));
        adf_fball_neg(y, x);
        ADF_CHECK(adf_fball_is_canonical(y));
        ADF_CHECK_MSG(ball_equals_json(y, result, &err), "neg line %lu",
                      jsonl_line_of(rec));
        adf_fball_clear(x);
        adf_fball_clear(y);
    }
    jsonl_close(f);
}

ADF_TEST(vectors_scale)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/scale.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * ja, * jq, * result;
        adf_fball_t x, y;
        adf_rat_struct q;

        ADF_CHECK(jsonl_field(rec, "a", &ja, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "q", &jq, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "result", &result, &err) == 1);
        adf_fball_init(x);
        adf_fball_init(y);
        fmpq_init(q.q);
        ADF_CHECK(json_ball(x, ja, &err));
        ADF_CHECK(json_rat(&q, jq, &err));
        adf_fball_mul_rat(y, x, &q);
        ADF_CHECK(adf_fball_is_canonical(y));
        ADF_CHECK_MSG(ball_equals_json(y, result, &err), "scale line %lu",
                      jsonl_line_of(rec));
        fmpq_clear(q.q);
        adf_fball_clear(x);
        adf_fball_clear(y);
    }
    jsonl_close(f);
}

/* ------------------------------------------------------------- predicates */

ADF_TEST(vectors_predicates)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/predicates.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * ja, * jb, * result, * opv;
        const char * op;
        size_t oplen;
        int want, got = -1;
        adf_fball_t x, y;

        ADF_CHECK(jsonl_field(rec, "a", &ja, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "b", &jb, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "result", &result, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "op", &opv, &err) == 1);
        op = jsonl_string(opv, &oplen, &err);
        ADF_CHECK(op != NULL);
        ADF_CHECK(jsonl_bool(result, &want, &err));
        adf_fball_init(x);
        adf_fball_init(y);
        ADF_CHECK(json_ball(x, ja, &err));
        ADF_CHECK(json_ball(y, jb, &err));
        if (op != NULL && strcmp(op, "equal_set") == 0)
            got = adf_fball_equal_set(x, y);
        else if (op != NULL && strcmp(op, "overlaps") == 0)
            got = adf_fball_overlaps(x, y);
        else if (op != NULL && strcmp(op, "contains") == 0)
            got = adf_fball_contains(x, y);
        else
            ADF_CHECK_MSG(0, "unknown predicate %s", op == NULL ? "(null)" : op);
        ADF_CHECK_MSG(got == want, "%s line %lu: got %d want %d",
                      op == NULL ? "(null)" : op, jsonl_line_of(rec), got, want);
        adf_fball_clear(x);
        adf_fball_clear(y);
    }
    jsonl_close(f);
}

ADF_TEST(vectors_compare)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/compare.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * ja, * jb, * result;
        fmpz_t want;
        adf_fball_t x, y;
        int got;

        ADF_CHECK(jsonl_field(rec, "a", &ja, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "b", &jb, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "result", &result, &err) == 1);
        fmpz_init(want);
        ADF_CHECK(json_fmpz(want, result, &err));
        adf_fball_init(x);
        adf_fball_init(y);
        ADF_CHECK(json_ball(x, ja, &err));
        ADF_CHECK(json_ball(y, jb, &err));
        got = adf_fball_compare(x, y);
        ADF_CHECK_MSG(got == (int) fmpz_get_si(want), "compare line %lu: got %d want %ld",
                      jsonl_line_of(rec), got, (long) fmpz_get_si(want));
        fmpz_clear(want);
        adf_fball_clear(x);
        adf_fball_clear(y);
    }
    jsonl_close(f);
}

ADF_TEST(vectors_membership)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/membership.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * jball, * jx, * result;
        adf_fball_t x;
        adf_rat_struct q;
        int want, got;

        ADF_CHECK(jsonl_field(rec, "ball", &jball, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "x", &jx, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "result", &result, &err) == 1);
        ADF_CHECK(jsonl_bool(result, &want, &err));
        adf_fball_init(x);
        fmpq_init(q.q);
        ADF_CHECK(json_ball(x, jball, &err));
        ADF_CHECK(json_rat(&q, jx, &err));
        got = adf_fball_contains_rat(x, &q);
        ADF_CHECK_MSG(got == want, "membership line %lu: got %d want %d",
                      jsonl_line_of(rec), got, want);
        fmpq_clear(q.q);
        adf_fball_clear(x);
    }
    jsonl_close(f);
}
