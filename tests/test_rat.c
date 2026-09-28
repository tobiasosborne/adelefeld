/* tests/test_rat.c: adf_rat, the exact global rational (conventions 5.1, 2.3, 3.2, 4.1, 4.3;
   SPEC 4.1, "Exact rationals are a separate type").

   Two parts. The first reads every vector of tests/ref/vectors/m1-rat/ completely; the files
   were written by lanes/m1-rat/gen_rat_vectors.py from the Python reference. The second covers
   what the vectors do not: the life cycle, the aliasing of every permitted combination of
   arguments, the state of the outputs after a status, and operands of thousands of bits.

   The inputs of the vector part are built with FLINT, not with the functions under test. */

#include <adelefeld.h>
#include <string.h>

#include "support/jsonl.h"
#include "test_runner.h"

#define VDIR "tests/ref/vectors/m1-rat/"

/* ---- helpers ---- */

/* One integer of the record, by key, read with FLINT. */

static int
read_fmpz_key(const jsonl_value *rec, const char *key, fmpz_t out, jsonl_error_t *err)
{
    const jsonl_value *v;

    if (jsonl_field(rec, key, &v, err) != 1)
        return 0;
    return fmpz_set_str(out, jsonl_int_text(v, err), 10) == 0;
}

/* The same, as a slong. */

static slong
read_slong_key(const jsonl_value *rec, const char *key, jsonl_error_t *err)
{
    fmpz_t z;
    slong v;

    fmpz_init(z);
    ADF_CHECK(read_fmpz_key(rec, key, z, err) == 1);
    ADF_CHECK_MSG(fmpz_fits_si(z), "the key \"%s\" of the record does not fit a slong", key);
    v = fmpz_get_si(z);
    fmpz_clear(z);
    return v;
}

/* The rational of the vector format {"num": n, "den": d}, read with FLINT only. */

static int
read_rat(const jsonl_value *v, fmpq_t out, jsonl_error_t *err)
{
    const jsonl_value *num, *den;
    fmpz_t n, d;
    int ok;

    if (jsonl_field(v, "num", &num, err) != 1 || jsonl_field(v, "den", &den, err) != 1)
        return 0;
    fmpz_init(n);
    fmpz_init(d);
    ok = fmpz_set_str(n, jsonl_int_text(num, err), 10) == 0
         && fmpz_set_str(d, jsonl_int_text(den, err), 10) == 0;
    if (ok)
        fmpq_set_fmpz_frac(out, n, d);
    fmpz_clear(n);
    fmpz_clear(d);
    return ok;
}

/* The status a vector names, as a code, through the name table of conventions 3.1. The vector
   files spell a status the way the golden files do, without the prefix ADF_. */

static int
status_code(const char *file, const char *name)
{
    int s;

    for (s = 0; s < ADF_STATUS_COUNT; s++)
        if (strcmp(adf_status_str(s), name) == 0)
            return s;
    ADF_CHECK_MSG(0, "%s: the vector names the status \"%s\", which is not one of the %d codes",
                  file, name, ADF_STATUS_COUNT);
    return -1;
}

/* x = n/d, built with FLINT and not with the functions under test. */

static void
set_frac(adf_rat_t x, slong n, slong d)
{
    fmpz_t a, b;

    fmpz_init_set_si(a, n);
    fmpz_init_set_si(b, d);
    fmpq_set_fmpz_frac(x->q, a, b);
    fmpz_clear(a);
    fmpz_clear(b);
}

/* The sentinel that a function which must not write its output leaves behind: 7/3, which is
   neither 0 nor 1, so that a written output is always seen. */

static void
set_sentinel(adf_rat_t x)
{
    fmpq_t s;

    fmpq_init(s);
    fmpq_set_si(s, 7, 3);
    fmpq_set(x->q, s);
    fmpq_clear(s);
}

/* Is x still exactly 7/3? */

static int
is_sentinel(const adf_rat_t x)
{
    fmpq_t s;
    int equal;

    fmpq_init(s);
    fmpq_set_si(s, 7, 3);
    equal = fmpq_equal(x->q, s);
    fmpq_clear(s);
    return equal;
}

/* ---- the vectors ---- */

/* Run one record. qa and qb are the two operands as FLINT values (b is unused by the unary
   operations), want the expected rational, iwant the expected integer (the op sgn), flag the
   expected boolean (the predicates, -1 when the record has no such field), status the expected
   code or -1 when the record expects ADF_OK. file and line name the record in a message. */

static void
run_record(const char *file, unsigned long line, const jsonl_value *rec, const char *op,
           const fmpq_t qa, const fmpq_t qb, const fmpq_t want, slong iwant, int flag, int status)
{
    adf_rat_t a, b, z;
    int s;

    adf_rat_init(a);
    adf_rat_init(b);
    adf_rat_init(z);
    fmpq_set(a->q, qa);
    fmpq_set(b->q, qb);

    if (strcmp(op, "set_fmpz2") == 0)
    {
        fmpz_t num, den;

        fmpz_init(num);
        fmpz_init(den);
        ADF_CHECK(read_fmpz_key(rec, "num", num, NULL) == 1);
        ADF_CHECK(read_fmpz_key(rec, "den", den, NULL) == 1);
        set_sentinel(z);
        s = adf_rat_set_fmpz2(z, num, den);
        if (status >= 0)
        {
            ADF_CHECK_MSG(s == status, "%s line %lu: adf_rat_set_fmpz2 gave %s, the vector says %s",
                          file, line, adf_status_str(s), adf_status_str(status));
            ADF_CHECK_MSG(is_sentinel(z), "%s line %lu: the output of a refused adf_rat_set_fmpz2 "
                                         "was written", file, line);
        }
        else
        {
            ADF_CHECK_MSG(s == ADF_OK, "%s line %lu: adf_rat_set_fmpz2 gave %s", file, line,
                          adf_status_str(s));
            ADF_CHECK_MSG(fmpq_equal(z->q, want),
                          "%s line %lu: adf_rat_set_fmpz2 gave the wrong value", file, line);
        }
        ADF_CHECK(adf_rat_is_canonical(z) == 1);
        fmpz_clear(num);
        fmpz_clear(den);
    }
    else if (strcmp(op, "set_fmpq") == 0)
    {
        fmpq_t raw;
        fmpz_t num, den;

        fmpq_init(raw);
        fmpz_init(num);
        fmpz_init(den);
        ADF_CHECK(read_fmpz_key(rec, "num", num, NULL) == 1);
        ADF_CHECK(read_fmpz_key(rec, "den", den, NULL) == 1);
        fmpz_set(fmpq_numref(raw), num);
        fmpz_set(fmpq_denref(raw), den);
        set_sentinel(z);
        s = adf_rat_set_fmpq(z, raw);
        if (status >= 0)
        {
            ADF_CHECK_MSG(s == status, "%s line %lu: adf_rat_set_fmpq gave %s, the vector says %s",
                          file, line, adf_status_str(s), adf_status_str(status));
            ADF_CHECK_MSG(is_sentinel(z), "%s line %lu: the output of a refused adf_rat_set_fmpq "
                                         "was written", file, line);
        }
        else
        {
            ADF_CHECK_MSG(s == ADF_OK, "%s line %lu: adf_rat_set_fmpq gave %s", file, line,
                          adf_status_str(s));
            ADF_CHECK_MSG(fmpq_equal(z->q, want),
                          "%s line %lu: adf_rat_set_fmpq gave the wrong value", file, line);
        }
        ADF_CHECK(adf_rat_is_canonical(z) == 1);
        fmpz_clear(num);
        fmpz_clear(den);
        fmpq_clear(raw);
    }
    else if (strcmp(op, "set_si") == 0)
    {
        slong n = read_slong_key(rec, "n", NULL);

        adf_rat_set_si(z, n);
        ADF_CHECK_MSG(fmpq_equal(z->q, want), "%s line %lu: adf_rat_set_si gave the wrong value",
                      file, line);
        ADF_CHECK(adf_rat_is_canonical(z) == 1);
        if (n != WORD_MIN)
        {
            adf_rat_set_si(z, -n);
            ADF_CHECK_MSG(fmpq_equal(z->q, want) == (n == 0),
                          "%s line %lu: adf_rat_set_si gave the wrong value for -n", file, line);
            ADF_CHECK(adf_rat_is_canonical(z) == 1);
        }
    }
    else if (strcmp(op, "add") == 0 || strcmp(op, "sub") == 0 || strcmp(op, "mul") == 0
             || strcmp(op, "div") == 0)
    {
        /* Every permitted aliasing of conventions 4.1, each from fresh operands: c as a third
           object, c as the first input, c as the second input, and c as all three at once. */
        adf_rat_t c;
        fmpq_t e;                      /* the value of the operation with all three the same */

        adf_rat_init(c);
        fmpq_init(e);
        fmpq_set(e, qa);
        if (strcmp(op, "add") == 0)
            fmpq_add(e, qa, qa);
        else if (strcmp(op, "sub") == 0)
            fmpq_sub(e, qa, qa);
        else if (strcmp(op, "mul") == 0)
            fmpq_mul(e, qa, qa);
        else if (status < 0)
            fmpq_div(e, qb, qb);        /* the record has a non-zero divisor */

        if (status >= 0)
        {
            /* Only adf_rat_div has a status in this branch: a divisor that is the exact 0. */
            set_sentinel(z);
            ADF_CHECK_MSG(adf_rat_div(z, a, b) == status,
                          "%s line %lu: adf_rat_div by the exact zero gave a status other than %s",
                          file, line, adf_status_str(status));
            ADF_CHECK_MSG(is_sentinel(z),
                          "%s line %lu: the output of a refused adf_rat_div was written", file,
                          line);
            set_sentinel(c);
            ADF_CHECK(adf_rat_div(c, c, b) == status);
            ADF_CHECK_MSG(is_sentinel(c),
                          "%s line %lu: the aliased output of a refused adf_rat_div was written",
                          file, line);
            fmpq_zero(c->q);
            ADF_CHECK(adf_rat_div(c, a, c) == status);
            ADF_CHECK_MSG(fmpq_is_zero(c->q),
                          "%s line %lu: the aliased divisor of a refused adf_rat_div was written",
                          file, line);
        }
        else if (strcmp(op, "add") == 0)
        {
            adf_rat_add(z, a, b);
            ADF_CHECK_MSG(fmpq_equal(z->q, want), "%s line %lu: adf_rat_add gave the wrong value",
                          file, line);
            fmpq_set(c->q, qa);
            adf_rat_add(c, c, b);
            ADF_CHECK_MSG(fmpq_equal(c->q, want),
                          "%s line %lu: adf_rat_add(x, x, y) gave the wrong value", file, line);
            fmpq_set(c->q, qb);
            adf_rat_add(c, a, c);
            ADF_CHECK_MSG(fmpq_equal(c->q, want),
                          "%s line %lu: adf_rat_add(x, y, y) gave the wrong value", file, line);
            fmpq_set(c->q, qa);
            adf_rat_add(c, c, c);
            ADF_CHECK_MSG(fmpq_equal(c->q, e),
                          "%s line %lu: adf_rat_add(x, x, x) gave the wrong value", file, line);
        }
        else if (strcmp(op, "sub") == 0)
        {
            adf_rat_sub(z, a, b);
            ADF_CHECK_MSG(fmpq_equal(z->q, want), "%s line %lu: adf_rat_sub gave the wrong value",
                          file, line);
            fmpq_set(c->q, qa);
            adf_rat_sub(c, c, b);
            ADF_CHECK_MSG(fmpq_equal(c->q, want),
                          "%s line %lu: adf_rat_sub(x, x, y) gave the wrong value", file, line);
            fmpq_set(c->q, qb);
            adf_rat_sub(c, a, c);
            ADF_CHECK_MSG(fmpq_equal(c->q, want),
                          "%s line %lu: adf_rat_sub(x, y, y) gave the wrong value", file, line);
            fmpq_set(c->q, qa);
            adf_rat_sub(c, c, c);
            ADF_CHECK_MSG(fmpq_equal(c->q, e),
                          "%s line %lu: adf_rat_sub(x, x, x) gave the wrong value", file, line);
        }
        else if (strcmp(op, "mul") == 0)
        {
            adf_rat_mul(z, a, b);
            ADF_CHECK_MSG(fmpq_equal(z->q, want), "%s line %lu: adf_rat_mul gave the wrong value",
                          file, line);
            fmpq_set(c->q, qa);
            adf_rat_mul(c, c, b);
            ADF_CHECK_MSG(fmpq_equal(c->q, want),
                          "%s line %lu: adf_rat_mul(x, x, y) gave the wrong value", file, line);
            fmpq_set(c->q, qb);
            adf_rat_mul(c, a, c);
            ADF_CHECK_MSG(fmpq_equal(c->q, want),
                          "%s line %lu: adf_rat_mul(x, y, y) gave the wrong value", file, line);
            fmpq_set(c->q, qa);
            adf_rat_mul(c, c, c);
            ADF_CHECK_MSG(fmpq_equal(c->q, e),
                          "%s line %lu: adf_rat_mul(x, x, x) gave the wrong value", file, line);
        }
        else
        {
            ADF_CHECK_MSG(adf_rat_div(z, a, b) == ADF_OK, "%s line %lu: adf_rat_div failed",
                          file, line);
            ADF_CHECK_MSG(fmpq_equal(z->q, want), "%s line %lu: adf_rat_div gave the wrong value",
                          file, line);
            fmpq_set(c->q, qa);
            adf_rat_div(c, c, b);
            ADF_CHECK_MSG(fmpq_equal(c->q, want),
                          "%s line %lu: adf_rat_div(x, x, y) gave the wrong value", file, line);
            fmpq_set(c->q, qb);
            adf_rat_div(c, a, c);
            ADF_CHECK_MSG(fmpq_equal(c->q, want),
                          "%s line %lu: adf_rat_div(x, y, y) gave the wrong value", file, line);
            fmpq_set(c->q, qb);
            adf_rat_div(c, c, c);
            ADF_CHECK_MSG(fmpq_equal(c->q, e),
                          "%s line %lu: adf_rat_div(x, x, x) gave the wrong value", file, line);
        }
        ADF_CHECK(adf_rat_is_canonical(a) == 1);
        ADF_CHECK(adf_rat_is_canonical(b) == 1);
        ADF_CHECK(adf_rat_is_canonical(c) == 1);
        ADF_CHECK(adf_rat_is_canonical(z) == 1);
        fmpq_clear(e);
        adf_rat_clear(c);
    }
    else if (strcmp(op, "neg") == 0)
    {
        fmpq_t e;

        fmpq_init(e);
        fmpq_neg(e, qa);
        adf_rat_neg(z, a);
        ADF_CHECK_MSG(fmpq_equal(z->q, e), "%s line %lu: adf_rat_neg gave the wrong value", file,
                      line);
        adf_rat_neg(a, a);
        ADF_CHECK_MSG(fmpq_equal(a->q, e), "%s line %lu: adf_rat_neg(x, x) gave the wrong value",
                      file, line);
        ADF_CHECK(fmpq_is_canonical(a->q) == 1);
        ADF_CHECK(fmpq_is_canonical(z->q) == 1);
        fmpq_clear(e);
    }
    else if (strcmp(op, "inv") == 0)
    {
        set_sentinel(z);
        if (status >= 0)
        {
            ADF_CHECK_MSG(adf_rat_inv(z, a) == status,
                          "%s line %lu: adf_rat_inv of the exact zero gave a status other than %s",
                          file, line, adf_status_str(status));
            ADF_CHECK_MSG(is_sentinel(z), "%s line %lu: the output of a refused adf_rat_inv was "
                                         "written", file, line);
            /* The output is the input here, so the claim is that x is still its own value. */
            ADF_CHECK_MSG(adf_rat_inv(a, a) == status,
                          "%s line %lu: the refused adf_rat_inv(x, x) gave another status", file,
                          line);
            ADF_CHECK_MSG(fmpq_equal(a->q, qa),
                          "%s line %lu: the aliased output of a refused adf_rat_inv was written",
                          file, line);
        }
        else
        {
            ADF_CHECK_MSG(adf_rat_inv(z, a) == ADF_OK, "%s line %lu: adf_rat_inv failed", file,
                          line);
            ADF_CHECK_MSG(fmpq_equal(z->q, want),
                          "%s line %lu: adf_rat_inv gave the wrong value", file, line);
            adf_rat_inv(a, a);
            ADF_CHECK_MSG(fmpq_equal(a->q, want),
                          "%s line %lu: adf_rat_inv(x, x) gave the wrong value", file, line);
        }
        ADF_CHECK(adf_rat_is_canonical(z) == 1);
    }
    else if (strcmp(op, "sgn") == 0)
    {
        slong got = adf_rat_sgn(a);

        ADF_CHECK_MSG(got == iwant, "%s line %lu: the sign is not the sign of the value", file,
                      line);
        ADF_CHECK(got == -1 || got == 0 || got == 1);
    }
    else if (strcmp(op, "equal") == 0)
    {
        ADF_CHECK_MSG(adf_rat_equal(a, b) == flag, "%s line %lu: adf_rat_equal disagrees with the "
                                                    "vector", file, line);
        ADF_CHECK_MSG(adf_rat_equal(a, a) == 1 && adf_rat_equal(b, b) == 1,
                      "%s line %lu: a value is not equal to itself", file, line);
    }
    else if (strcmp(op, "identical") == 0)
    {
        ADF_CHECK_MSG(adf_rat_identical(a, b) == flag,
                      "%s line %lu: adf_rat_identical disagrees with the vector", file, line);
        ADF_CHECK_MSG(adf_rat_identical(a, a) == 1 && adf_rat_identical(b, b) == 1,
                      "%s line %lu: a value is not identical with itself", file, line);
    }
    else if (strcmp(op, "is_zero") == 0)
    {
        ADF_CHECK_MSG(adf_rat_is_zero(a) == flag,
                      "%s line %lu: adf_rat_is_zero disagrees with the vector", file, line);
        ADF_CHECK_MSG(adf_rat_is_zero(b) == fmpq_is_zero(qb),
                      "%s line %lu: adf_rat_is_zero is not the test of the value", file, line);
    }
    else
    {
        ADF_CHECK_MSG(0, "%s line %lu: unknown op \"%s\"", file, line, op);
    }

    adf_rat_clear(a);
    adf_rat_clear(b);
    adf_rat_clear(z);
}

/* Read one vector file completely, or say why not. */

static void
run_file(const char *name)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    size_t i;

    if (jsonl_open(name, &f, &err) != 1)
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    ADF_CHECK_MSG(jsonl_count(f) > 0, "%s: the file has no records", name);

    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        const jsonl_value *v;
        const char *op;
        fmpq_t qa, qb, want;
        slong iwant = 0;
        int flag = -1;
        int status = -1;

        /* The op is looked up by name: the keys of a record are in file order, not in the order
           the generator wrote them (it sorts them). */
        ADF_CHECK(jsonl_field(rec, "op", &v, &err) == 1);
        op = jsonl_string(v, NULL, &err);
        ADF_CHECK_MSG(op != NULL, "%s line %lu: the record has no op", name, jsonl_line_of(rec));
        if (op == NULL)
            break;

        fmpq_init(qa);
        fmpq_init(qb);
        fmpq_init(want);
        fmpq_set_si(qa, 0, 1);
        fmpq_set_si(qb, 1, 1);
        fmpq_set_si(want, 0, 1);

        if (jsonl_field(rec, "a", &v, &err) == 1)
            ADF_CHECK(read_rat(v, qa, &err));
        if (jsonl_field(rec, "b", &v, &err) == 1)
            ADF_CHECK(read_rat(v, qb, &err));
        if (jsonl_field(rec, "status", &v, &err) == 1)
            status = status_code(name, jsonl_string(v, NULL, &err));
        if (jsonl_field(rec, "result", &v, &err) == 1)
        {
            if (strcmp(op, "sgn") == 0)
                iwant = read_slong_key(rec, "result", &err);
            else if (strcmp(op, "equal") == 0 || strcmp(op, "identical") == 0
                     || strcmp(op, "is_zero") == 0)
            {
                ADF_CHECK(jsonl_bool(v, &flag, &err) == 1);
            }
            else
                ADF_CHECK(read_rat(v, want, &err));
        }

        run_record(name, jsonl_line_of(rec), rec, op, qa, qb, want, iwant, flag, status);
        fmpq_clear(qa);
        fmpq_clear(qb);
        fmpq_clear(want);
    }
    jsonl_close(f);
}

ADF_TEST(the_vectors_of_the_constructors_from_raw_data)
{
    run_file(VDIR "rat_set.jsonl");
}

ADF_TEST(the_vectors_of_adf_rat_set_si)
{
    run_file(VDIR "rat_set_si.jsonl");
}

ADF_TEST(the_vectors_of_the_sum)
{
    run_file(VDIR "rat_add.jsonl");
}

ADF_TEST(the_vectors_of_the_difference)
{
    run_file(VDIR "rat_sub.jsonl");
}

ADF_TEST(the_vectors_of_the_product)
{
    run_file(VDIR "rat_mul.jsonl");
}

ADF_TEST(the_vectors_of_the_negation)
{
    run_file(VDIR "rat_neg.jsonl");
}

ADF_TEST(the_vectors_of_the_quotient)
{
    run_file(VDIR "rat_div.jsonl");
}

ADF_TEST(the_vectors_of_the_inverse)
{
    run_file(VDIR "rat_inv.jsonl");
}

ADF_TEST(the_vectors_of_the_predicates)
{
    run_file(VDIR "rat_equal.jsonl");
    run_file(VDIR "rat_identical.jsonl");
    run_file(VDIR "rat_is_zero.jsonl");
    run_file(VDIR "rat_sgn.jsonl");
}

/* ---- what the vectors do not cover ---- */

ADF_TEST(the_life_cycle)
{
    adf_rat_t x;
    fmpq_t q;

    adf_rat_init(x);
    ADF_CHECK(adf_rat_is_canonical(x) == 1);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    ADF_CHECK(fmpz_is_zero(fmpq_numref(x->q)));
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));
    ADF_CHECK(adf_rat_sgn(x) == 0);
    ADF_CHECK(adf_rat_equal(x, x) == 1);
    ADF_CHECK(adf_rat_identical(x, x) == 1);

    adf_rat_set_si(x, -5);
    ADF_CHECK(adf_rat_is_canonical(x) == 1);
    ADF_CHECK(adf_rat_is_zero(x) == 0);
    ADF_CHECK(adf_rat_sgn(x) == -1);
    ADF_CHECK(adf_rat_equal(x, x) == 1);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(x->q), -5));
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));

    fmpq_init(q);
    adf_rat_get_fmpq(q, x);
    ADF_CHECK(fmpq_equal_si(q, -5));
    ADF_CHECK(fmpq_is_canonical(q) == 1);
    fmpq_clear(q);

    adf_rat_clear(x);
}

ADF_TEST(the_zero_and_the_one)
{
    adf_rat_t x, y;

    adf_rat_init(x);
    adf_rat_init(y);
    adf_rat_set_si(y, 17);

    adf_rat_zero(x);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    ADF_CHECK(adf_rat_sgn(x) == 0);
    ADF_CHECK(fmpz_is_zero(fmpq_numref(x->q)));
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));
    ADF_CHECK(adf_rat_equal(x, y) == 0);
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    adf_rat_one(x);
    ADF_CHECK(adf_rat_is_zero(x) == 0);
    ADF_CHECK(adf_rat_sgn(x) == 1);
    ADF_CHECK(fmpz_is_one(fmpq_numref(x->q)));
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));
    ADF_CHECK(adf_rat_equal(x, y) == 0);
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    adf_rat_zero(y);
    ADF_CHECK(adf_rat_equal(x, y) == 0);
    adf_rat_one(y);
    ADF_CHECK(adf_rat_equal(x, y) == 1);

    adf_rat_clear(x);
    adf_rat_clear(y);
}

ADF_TEST(the_zero_keeps_its_canonical_form_under_arithmetic)
{
    adf_rat_t x, z;

    adf_rat_init(x);
    adf_rat_init(z);
    adf_rat_zero(x);
    adf_rat_set_si(z, -3);

    adf_rat_add(x, x, z);
    ADF_CHECK(fmpq_equal_si(x->q, -3));
    adf_rat_sub(x, x, z);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    adf_rat_mul(x, x, z);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    adf_rat_neg(x, x);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    ADF_CHECK(fmpz_is_zero(fmpq_numref(x->q)));
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));

    /* The exact 0 is not invertible, and the refusal leaves x at the exact 0. */
    ADF_CHECK(adf_rat_div(x, z, x) == ADF_NOT_UNIT);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));
    ADF_CHECK(adf_rat_inv(x, x) == ADF_NOT_UNIT);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));

    /* With a non-zero divisor the exact operations are exact again. */
    set_frac(x, -3, 2);
    ADF_CHECK(adf_rat_inv(x, x) == ADF_OK);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(x->q), -2));
    ADF_CHECK(fmpz_equal_si(fmpq_denref(x->q), 3));
    ADF_CHECK(adf_rat_div(x, z, x) == ADF_OK);
    ADF_CHECK_MSG(fmpz_equal_si(fmpq_numref(x->q), 9) && fmpz_equal_si(fmpq_denref(x->q), 2),
                  "-3 divided by -2/3 is 9/2");
    set_frac(x, 3, 7);
    ADF_CHECK(adf_rat_div(x, x, z) == ADF_OK);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(x->q), -1));
    ADF_CHECK(fmpz_equal_si(fmpq_denref(x->q), 7));
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    adf_rat_clear(x);
    adf_rat_clear(z);
}

ADF_TEST(a_copy_may_be_its_own_source)
{
    adf_rat_t x, y;

    adf_rat_init(x);
    adf_rat_init(y);
    set_frac(y, 2, 3);
    adf_rat_set(x, y);
    ADF_CHECK(fmpq_equal(x->q, y->q));
    ADF_CHECK(adf_rat_identical(x, y) == 1);
    adf_rat_set(x, x);
    ADF_CHECK(fmpq_equal(x->q, y->q));
    ADF_CHECK(fmpz_equal_si(fmpq_numref(x->q), 2));
    ADF_CHECK(fmpz_equal_si(fmpq_denref(x->q), 3));
    ADF_CHECK(adf_rat_is_canonical(x) == 1);
    adf_rat_clear(x);
    adf_rat_clear(y);
}

ADF_TEST(a_swap_exchanges_the_values)
{
    adf_rat_t x, y;

    adf_rat_init(x);
    adf_rat_init(y);
    set_frac(x, 5, 2);
    adf_rat_set_si(y, -7);
    adf_rat_swap(x, y);
    ADF_CHECK(adf_rat_sgn(x) == -1);
    ADF_CHECK(fmpq_equal_si(x->q, -7));
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));
    ADF_CHECK(adf_rat_sgn(y) == 1);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(y->q), 5));
    ADF_CHECK(fmpz_equal_si(fmpq_denref(y->q), 2));
    adf_rat_swap(x, y);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(x->q), 5));
    ADF_CHECK(fmpz_equal_si(fmpq_denref(x->q), 2));
    ADF_CHECK(fmpq_equal_si(y->q, -7));
    adf_rat_clear(x);
    adf_rat_clear(y);
}

ADF_TEST(a_copy_owns_its_own_limbs)
{
    /* The copy has its own memory: a later write through one leaves the other alone. */
    adf_rat_t x, y;
    fmpz_t n;
    char *text;

    adf_rat_init(x);
    adf_rat_init(y);
    fmpz_init(n);
    ADF_CHECK(fmpz_set_str(n, "123456789012345678901234567890", 10) == 0);
    adf_rat_set_fmpz(x, n);
    adf_rat_set(y, x);
    ADF_CHECK(fmpq_equal(x->q, y->q));
    ADF_CHECK(adf_rat_identical(x, y) == 1);

    /* Writing through x leaves y alone: the copy owns its own limbs. */
    ADF_CHECK(fmpz_set_str(n, "98765432109876543210987654321", 10) == 0);
    adf_rat_set_fmpz(x, n);
    ADF_CHECK_MSG(!fmpq_equal(x->q, y->q), "the copy shares its memory with the original");
    text = fmpq_get_str(NULL, 10, y->q);
    ADF_CHECK_MSG(strncmp(text, "123456789012345678901234567890", 30) == 0,
                  "the copy was changed through the original: %s", text);
    flint_free(text);
    fmpz_clear(n);
    adf_rat_clear(x);
    adf_rat_clear(y);
}

ADF_TEST(the_constructor_from_two_integers_reduces_and_moves_the_sign)
{
    adf_rat_t x;
    fmpz_t n, d;

    adf_rat_init(x);
    fmpz_init(n);
    fmpz_init(d);

    fmpz_set_si(n, 6);
    fmpz_set_si(d, 3);
    ADF_CHECK(adf_rat_set_fmpz2(x, n, d) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(x->q, 2));
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpz_set_si(d, -3);
    ADF_CHECK(adf_rat_set_fmpz2(x, n, d) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(x->q, -2));
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpz_set_si(n, -6);
    ADF_CHECK(adf_rat_set_fmpz2(x, n, d) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(x->q, 2));
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpz_set_si(d, 3);
    ADF_CHECK(adf_rat_set_fmpz2(x, n, d) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(x->q, -2));

    fmpz_zero(n);
    fmpz_set_si(d, -7);
    ADF_CHECK(adf_rat_set_fmpz2(x, n, d) == ADF_OK);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpz_clear(n);
    fmpz_clear(d);
    adf_rat_clear(x);
}

ADF_TEST(the_constructor_from_two_integers_admits_aliased_arguments)
{
    /* The header: "num and den may alias each other"; they are not of the output type. */
    adf_rat_t x;
    fmpz_t n;

    adf_rat_init(x);
    fmpz_init(n);

    fmpz_set_si(n, 4);
    ADF_CHECK(adf_rat_set_fmpz2(x, n, n) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(x->q, 1));
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpz_set_si(n, -4);
    ADF_CHECK(adf_rat_set_fmpz2(x, n, n) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(x->q, 1));
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpz_set_si(n, 7);
    ADF_CHECK(adf_rat_set_fmpz2(x, n, n) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(x->q, 1));
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));

    fmpz_clear(n);
    adf_rat_clear(x);
}

ADF_TEST(the_constructor_from_an_fmpq_canonicalises)
{
    adf_rat_t x;
    fmpq_t raw;

    adf_rat_init(x);
    fmpq_init(raw);

    fmpz_set_si(fmpq_numref(raw), 4);
    fmpz_set_si(fmpq_denref(raw), 8);
    ADF_CHECK_MSG(fmpq_is_canonical(raw) == 0, "4/8 is not in lowest terms");
    ADF_CHECK(adf_rat_set_fmpq(x, raw) == ADF_OK);
    ADF_CHECK_MSG(fmpz_equal_si(fmpq_numref(x->q), 1) && fmpz_equal_si(fmpq_denref(x->q), 2),
                  "4/8 is the rational 1/2");
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpz_set_si(fmpq_numref(raw), -4);
    fmpz_set_si(fmpq_denref(raw), 8);
    ADF_CHECK(adf_rat_set_fmpq(x, raw) == ADF_OK);
    ADF_CHECK_MSG(fmpz_equal_si(fmpq_numref(x->q), -1) && fmpz_equal_si(fmpq_denref(x->q), 2),
                  "-4/8 is the rational -1/2");
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpz_set_si(fmpq_numref(raw), -6);
    fmpz_set_si(fmpq_denref(raw), 4);
    ADF_CHECK(fmpq_is_canonical(raw) == 0);
    ADF_CHECK(adf_rat_set_fmpq(x, raw) == ADF_OK);
    ADF_CHECK_MSG(fmpz_equal_si(fmpq_numref(x->q), -3) && fmpz_equal_si(fmpq_denref(x->q), 2),
                  "-6/4 is the rational -3/2");
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpq_zero(raw);
    ADF_CHECK(adf_rat_set_fmpq(x, raw) == ADF_OK);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    ADF_CHECK(fmpz_is_one(fmpq_denref(x->q)));
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpq_clear(raw);
    adf_rat_clear(x);
}

ADF_TEST(a_zero_denominator_is_outside_the_domain_and_writes_nothing)
{
    adf_rat_t x;
    fmpz_t n, d;
    fmpq_t raw;

    adf_rat_init(x);
    fmpq_init(raw);
    fmpz_init(n);
    fmpz_init(d);
    set_sentinel(x);

    fmpz_set_si(n, 3);
    fmpz_zero(d);
    ADF_CHECK_MSG(adf_rat_set_fmpz2(x, n, d) == ADF_DOMAIN, "a zero denominator is ADF_DOMAIN");
    ADF_CHECK_MSG(is_sentinel(x), "the output of adf_rat_set_fmpz2 with den = 0 was written");

    fmpz_zero(n);
    ADF_CHECK(adf_rat_set_fmpz2(x, n, d) == ADF_DOMAIN);
    ADF_CHECK(is_sentinel(x));

    /* The same through an fmpq whose denominator field is 0, which is not a value of fmpq. */
    fmpz_set_si(fmpq_numref(raw), 5);
    fmpz_zero(fmpq_denref(raw));
    ADF_CHECK_MSG(adf_rat_set_fmpq(x, raw) == ADF_DOMAIN,
                  "an fmpq with a zero denominator is ADF_DOMAIN");
    ADF_CHECK_MSG(is_sentinel(x), "the output of adf_rat_set_fmpq with den = 0 was written");
    fmpz_zero(fmpq_numref(raw));
    ADF_CHECK(adf_rat_set_fmpq(x, raw) == ADF_DOMAIN);
    ADF_CHECK(is_sentinel(x));

    fmpz_clear(n);
    fmpz_clear(d);
    fmpq_clear(raw);
    adf_rat_clear(x);
}

ADF_TEST(the_quotient_by_the_exact_zero_is_not_a_unit_and_writes_nothing)
{
    adf_rat_t x, y, z;

    adf_rat_init(x);
    adf_rat_init(y);
    adf_rat_init(z);
    set_frac(x, 3, 2);
    adf_rat_zero(y);
    set_sentinel(z);

    ADF_CHECK_MSG(adf_rat_div(z, x, y) == ADF_NOT_UNIT, "the exact zero is not invertible");
    ADF_CHECK_MSG(is_sentinel(z), "the output of a refused adf_rat_div was written");
    ADF_CHECK(adf_rat_is_canonical(z) == 1);

    /* The output may be one of the inputs, also on the refused path (conventions 4.1, 4.3). */
    set_sentinel(x);
    ADF_CHECK(adf_rat_div(x, x, y) == ADF_NOT_UNIT);
    ADF_CHECK_MSG(is_sentinel(x), "the aliased output of a refused adf_rat_div was written");
    adf_rat_zero(x);
    ADF_CHECK(adf_rat_div(x, x, y) == ADF_NOT_UNIT);
    ADF_CHECK(adf_rat_is_zero(x) == 1);

    /* A non-zero divisor never fails, not even of a zero dividend. */
    set_frac(x, 3, 2);
    ADF_CHECK(adf_rat_div(z, x, x) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(z->q, 1));
    ADF_CHECK(adf_rat_div(z, y, x) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(z->q, 0));
    ADF_CHECK(adf_rat_is_canonical(z) == 1);

    adf_rat_clear(x);
    adf_rat_clear(y);
    adf_rat_clear(z);
}

ADF_TEST(the_inverse_of_the_exact_zero_is_not_a_unit_and_writes_nothing)
{
    adf_rat_t x, y;

    adf_rat_init(x);
    adf_rat_init(y);
    adf_rat_zero(x);
    set_sentinel(y);

    ADF_CHECK_MSG(adf_rat_inv(y, x) == ADF_NOT_UNIT, "the exact zero has no inverse");
    ADF_CHECK_MSG(is_sentinel(y), "the output of a refused adf_rat_inv was written");
    /* The output is also the input here: the exact 0 must survive the refusal. */
    ADF_CHECK(adf_rat_inv(x, x) == ADF_NOT_UNIT);
    ADF_CHECK_MSG(adf_rat_is_zero(x) == 1 && fmpz_is_one(fmpq_denref(x->q)),
                  "the aliased output of a refused adf_rat_inv was written");

    /* One is its own inverse, and the inverse of an exact rational is exact. */
    adf_rat_one(x);
    ADF_CHECK(adf_rat_inv(x, x) == ADF_OK);
    ADF_CHECK(fmpq_equal_si(x->q, 1));
    adf_rat_set_si(x, -2);
    ADF_CHECK(adf_rat_inv(x, x) == ADF_OK);
    ADF_CHECK_MSG(fmpz_equal_si(fmpq_numref(x->q), -1) && fmpz_equal_si(fmpq_denref(x->q), 2),
                  "the inverse of the exact -2 is -1/2");
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    adf_rat_clear(x);
    adf_rat_clear(y);
}

ADF_TEST(the_sign_of_every_value)
{
    adf_rat_t x;

    adf_rat_init(x);
    adf_rat_set_si(x, 1);
    ADF_CHECK(adf_rat_sgn(x) == 1);
    adf_rat_set_si(x, 0);
    ADF_CHECK(adf_rat_sgn(x) == 0);
    adf_rat_set_si(x, -1);
    ADF_CHECK(adf_rat_sgn(x) == -1);
    adf_rat_set_si(x, 2);
    ADF_CHECK(adf_rat_sgn(x) == 1);
    adf_rat_set_si(x, -2);
    ADF_CHECK(adf_rat_sgn(x) == -1);
    adf_rat_clear(x);
}

ADF_TEST(the_equality_of_two_values)
{
    adf_rat_t x, y;

    adf_rat_init(x);
    adf_rat_init(y);

    adf_rat_set_si(x, 2);
    adf_rat_set_si(y, 2);
    ADF_CHECK(adf_rat_equal(x, y) == 1);
    ADF_CHECK(adf_rat_identical(x, y) == 1);

    adf_rat_set_si(y, 3);
    ADF_CHECK(adf_rat_equal(x, y) == 0);
    ADF_CHECK(adf_rat_identical(x, y) == 0);

    adf_rat_one(y);
    ADF_CHECK(adf_rat_equal(x, y) == 0);
    ADF_CHECK(adf_rat_equal(x, x) == 1);
    ADF_CHECK(adf_rat_identical(x, x) == 1);

    adf_rat_zero(x);
    ADF_CHECK(adf_rat_equal(x, y) == 0);
    adf_rat_zero(y);
    ADF_CHECK(adf_rat_equal(x, y) == 1);
    ADF_CHECK(adf_rat_identical(x, y) == 1);
    ADF_CHECK(adf_rat_is_zero(x) == 1);
    ADF_CHECK(adf_rat_is_zero(y) == 1);

    adf_rat_clear(x);
    adf_rat_clear(y);
}

ADF_TEST(a_non_canonical_value_is_told_from_a_canonical_one)
{
    /* adf_rat_is_canonical is the predicate of conventions 5.1 and never aborts. */
    adf_rat_t x;

    adf_rat_init(x);
    adf_rat_set_si(x, 4);
    ADF_CHECK(adf_rat_is_canonical(x) == 1);

    fmpz_set_si(fmpq_numref(x->q), 8);
    fmpz_set_si(fmpq_denref(x->q), 2);   /* 8/2 is not in lowest terms */
    ADF_CHECK_MSG(adf_rat_is_canonical(x) == 0, "8/2 is not canonical");
    fmpq_canonicalise(x->q);
    ADF_CHECK(adf_rat_is_canonical(x) == 1);
    ADF_CHECK(fmpq_equal_si(x->q, 4));

    fmpz_set_si(fmpq_numref(x->q), -8);
    fmpz_set_si(fmpq_denref(x->q), -2);
    ADF_CHECK_MSG(adf_rat_is_canonical(x) == 0, "a negative denominator is not canonical");
    fmpq_canonicalise(x->q);
    ADF_CHECK(adf_rat_is_canonical(x) == 1);
    ADF_CHECK(fmpq_equal_si(x->q, 4));

    fmpz_zero(fmpq_denref(x->q));
    ADF_CHECK_MSG(adf_rat_is_canonical(x) == 0, "a zero denominator is not canonical");

    adf_rat_clear(x);
}

ADF_TEST(the_arithmetic_of_huge_operands_is_exact)
{
    /* A thousand-bit numerator and denominator, so that the gcds are not word-sized. */
    adf_rat_t x, y, z;
    char *text;
    int i;

    adf_rat_init(x);
    adf_rat_init(y);
    adf_rat_init(z);

    set_frac(x, 3, 1);
    for (i = 0; i < 10; i++)
        adf_rat_mul(x, x, x);
    text = fmpq_get_str(NULL, 10, x->q);
    ADF_CHECK_MSG(strlen(text) > 300, "the operand is not huge: %u digits",
                  (unsigned) strlen(text));
    flint_free(text);
    ADF_CHECK(adf_rat_is_canonical(x) == 1);
    ADF_CHECK_MSG(fmpz_is_one(fmpq_denref(x->q)), "3^1024 is an integer");
    adf_rat_set(y, x);
    ADF_CHECK(fmpq_equal(x->q, y->q));
    ADF_CHECK(adf_rat_identical(x, y) == 1);
    adf_rat_div(y, y, x);
    ADF_CHECK_MSG(fmpq_equal_si(y->q, 1), "x / x is the exact 1 for a huge x");

    /* (1/3) + (1/7) is 10/21, a result no rounding could give. */
    set_frac(x, 1, 3);
    set_frac(y, 1, 7);
    adf_rat_add(z, x, y);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(z->q), 10));
    ADF_CHECK(fmpz_equal_si(fmpq_denref(z->q), 21));
    ADF_CHECK(adf_rat_is_canonical(z) == 1);

    adf_rat_inv(z, x);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(z->q), 3));
    ADF_CHECK(fmpz_equal_si(fmpq_denref(z->q), 1));
    ADF_CHECK(adf_rat_is_canonical(z) == 1);

    adf_rat_mul(z, x, y);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(z->q), 1));
    ADF_CHECK(fmpz_equal_si(fmpq_denref(z->q), 21));
    ADF_CHECK(adf_rat_is_canonical(z) == 1);

    adf_rat_div(z, z, y);
    ADF_CHECK_MSG(fmpz_equal_si(fmpq_numref(z->q), 1) && fmpz_equal_si(fmpq_denref(z->q), 3),
                  "1/21 divided by 1/7 is 1/3");
    ADF_CHECK(adf_rat_is_canonical(z) == 1);

    /* The difference of two equal huge values is the exact zero. */
    adf_rat_sub(z, x, x);
    ADF_CHECK(adf_rat_is_zero(z) == 1);
    ADF_CHECK(fmpz_is_one(fmpq_denref(z->q)));

    adf_rat_clear(x);
    adf_rat_clear(y);
    adf_rat_clear(z);
}

ADF_TEST(the_layout_of_the_rational_is_the_documented_one)
{
    /* conventions 5.1 and 12.4: one fmpq, 16 bytes, alignment 8. */
    ADF_CHECK(adf_sizeof_rat() == 16);
    ADF_CHECK(adf_alignof_rat() == 8);
    ADF_CHECK(sizeof(adf_rat_struct) == 16);
}
