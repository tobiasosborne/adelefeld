/* tests/test_lball.c: adf_lball, local balls at one prime (lane f-slice1; include/adelefeld/lball.h;
   docs/api-1f.md statements L0 to L8).

   Oracles.
   1. The vectors the files tests/ref/vectors/f-slice1/lball_*.jsonl, made by lanes/f-slice1/gen_vectors.py from the Python reference
      (proto/functions_checks.py, section f-slice1). Every line of every file is run.
   2. An enumeration written in this file, independent of the reference: for every pair of balls of a universe
      (p = 2, 3 exhaustive; p = 5, 7 a sample), for every choice of one point in each operand (c + p^N a, a in
      [0, p)), the exact result r of the point operation lies in the result ball, and the p classes
      r modulo p^(K + 1), K the exponent of the result, are all present. The first says "contains every result",
      the second "no ball of exponent K + 1 or more does". The points are FLINT fmpq values, the classes are
      compared with adf_lball_set_rat_ball and adf_lball_equal_set.
   3. Forged values for adf_lball_is_canonical, the statuses with the state of the outputs, aliasing, limits.

   What would make a case fail is stated at each test. */

#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include "support/jsonl.h"
#include "test_runner.h"

#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/resource.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <unistd.h>
#endif

/* ---------------------------------------------------------------------------------------------------- helpers */

static adf_place_t
place_of(ulong p)
{
    adf_place_t v;
    int st = adf_place_prime(&v, p);
    ADF_CHECK(st == ADF_OK);
    return v;
}

/* x = the value with the given canonical fields (no validation). */
static void
lb_fields(adf_lball_t x, ulong p, int exact, const fmpz_t un, const fmpz_t ud, slong v, slong N)
{
    x->p = p;
    fmpz_set(fmpq_numref(x->u), un);
    fmpz_set(fmpq_denref(x->u), ud);
    x->v = v;
    x->N = N;
    x->exact = exact;
}

static void
lb_fields_si(adf_lball_t x, ulong p, int exact, slong un, ulong ud, slong v, slong N)
{
    x->p = p;
    fmpz_set_si(fmpq_numref(x->u), un);
    fmpz_set_ui(fmpq_denref(x->u), ud);
    x->v = v;
    x->N = N;
    x->exact = exact;
}

static int
status_from_name(const char * s)
{
    int i;
    for (i = 0; i < ADF_STATUS_COUNT; i++)
        if (strcmp(adf_status_str(i), s) == 0)
            return i;
    return -1;
}

static void
fmpz_from_text(fmpz_t z, const char * t)
{
    int r = fmpz_set_str(z, t, 10);
    ADF_CHECK(r == 0);
}

static ulong
ulong_from_text(const char * t)
{
    return strtoul(t, NULL, 10);
}

static slong
slong_from_text(const char * t)
{
    return strtol(t, NULL, 10);
}

/* Reads a member of an object; NULL text on failure (with a check that fails). */
static const char *
member_int(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    const jsonl_value * v = NULL;
    const char * t = NULL;
    int ok = jsonl_field(rec, key, &v, &err);
    ADF_CHECK_MSG(ok == 1, "%s", jsonl_error_message(&err));
    if (!ok)
        return "0";
    t = jsonl_int_text(v, &err);
    ADF_CHECK_MSG(t != NULL, "%s", jsonl_error_message(&err));
    return t ? t : "0";
}

static const jsonl_value *
member(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    const jsonl_value * v = NULL;
    int ok = jsonl_field(rec, key, &v, &err);
    ADF_CHECK_MSG(ok == 1, "%s", jsonl_error_message(&err));
    return ok ? v : NULL;
}

static const char *
member_str(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    size_t len;
    const jsonl_value * v = member(rec, key);
    const char * s = v ? jsonl_string(v, &len, &err) : NULL;
    ADF_CHECK(s != NULL);
    return s ? s : "";
}

static int
member_bool(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    int b = 0;
    const jsonl_value * v = member(rec, key);
    ADF_CHECK(v != NULL && jsonl_bool(v, &b, &err) == 1);
    return b;
}

/* A value of a vector: {"p","exact","un","ud","v","N"}. x must be initialised. */
static void
lb_from_json(adf_lball_t x, const jsonl_value * o)
{
    fmpz_t un, ud;
    fmpz_init(un);
    fmpz_init(ud);
    fmpz_from_text(un, member_int(o, "un"));
    fmpz_from_text(ud, member_int(o, "ud"));
    lb_fields(x, ulong_from_text(member_int(o, "p")), (int) slong_from_text(member_int(o, "exact")), un, ud,
              slong_from_text(member_int(o, "v")), slong_from_text(member_int(o, "N")));
    fmpz_clear(un);
    fmpz_clear(ud);
}

static void
rat_from_json(adf_rat_t q, const jsonl_value * o)
{
    fmpz_from_text(fmpq_numref(q->q), member_int(o, "num"));
    fmpz_from_text(fmpq_denref(q->q), member_int(o, "den"));
}

/* A recognisable canonical value that a function must leave alone on a status. */
static void
sentinel(adf_lball_t x)
{
    lb_fields_si(x, 1000003, 0, 7, 1, 2, 5);
}

/* One point of a ball of the enumeration: c + p^N a as an fmpq; the exact value for an exact ball.
   Reads the fields only; the value of the centre is p^v u built here with fmpz_pow_ui (not with the library). */
static void
centre_of(fmpq_t c, const adf_lball_t x)
{
    fmpz_t pp;
    fmpq_t t;
    fmpz_init(pp);
    fmpq_init(t);
    fmpz_set_ui(pp, x->p);
    if (x->v >= 0)
    {
        fmpz_pow_ui(fmpq_numref(t), pp, (ulong) x->v);
        fmpz_one(fmpq_denref(t));
    }
    else
    {
        fmpz_one(fmpq_numref(t));
        fmpz_pow_ui(fmpq_denref(t), pp, (ulong) -x->v);
    }
    fmpq_mul(c, x->u, t);
    fmpz_clear(pp);
    fmpq_clear(t);
}

static void
ppow(fmpq_t out, ulong p, slong e)
{
    fmpz_t pp;
    fmpz_init(pp);
    fmpz_set_ui(pp, p);
    if (e >= 0)
    {
        fmpz_pow_ui(fmpq_numref(out), pp, (ulong) e);
        fmpz_one(fmpq_denref(out));
    }
    else
    {
        fmpz_one(fmpq_numref(out));
        fmpz_pow_ui(fmpq_denref(out), pp, (ulong) -e);
    }
    fmpz_clear(pp);
}

/* The a-th point of x: the value for exact x (a ignored), c + p^N a for a ball. */
static void
point_of(fmpq_t s, const adf_lball_t x, ulong a)
{
    fmpq_t pn;
    centre_of(s, x);
    if (!x->exact)
    {
        fmpq_init(pn);
        ppow(pn, x->p, x->N);
        fmpq_mul_ui(pn, pn, a);
        fmpq_add(s, s, pn);
        fmpq_clear(pn);
    }
}

/* z = the exact value r (an fmpq) at prime p, through the library. */
static void
exact_of(adf_lball_t z, ulong p, const fmpq_t r)
{
    adf_rat_t q;
    int st;
    adf_rat_init(q);
    fmpq_set(q->q, r);
    st = adf_lball_set_rat(z, place_of(p), q);
    ADF_CHECK(st == ADF_OK);
    adf_rat_clear(q);
}

static void
ball_of(adf_lball_t z, ulong p, const fmpq_t r, slong N)
{
    adf_rat_t q;
    int st;
    adf_rat_init(q);
    fmpq_set(q->q, r);
    st = adf_lball_set_rat_ball(z, place_of(p), q, N);
    ADF_CHECK(st == ADF_OK);
    adf_rat_clear(q);
}

/* ------------------------------------------------------------------------------------------------ life cycle */

ADF_TEST(layout_init_and_copy)
{
    adf_lball_t x, y;
    fmpz_t t;
    ADF_CHECK(adf_sizeof_lball() == 48 && adf_alignof_lball() == 8);
    ADF_CHECK(sizeof(adf_lball_t) == sizeof(adf_lball_struct));
    ADF_CHECK(offsetof(adf_lball_struct, p) == 0 && offsetof(adf_lball_struct, u) == 8);
    ADF_CHECK(offsetof(adf_lball_struct, v) == 24 && offsetof(adf_lball_struct, N) == 32);
    ADF_CHECK(offsetof(adf_lball_struct, exact) == 40);
    adf_lball_init(x);
    adf_lball_init(y);
    ADF_CHECK(x->p == 2 && x->exact == 1 && x->v == 0 && x->N == 0 && fmpq_is_zero(x->u));
    ADF_CHECK(adf_lball_is_canonical(x) && adf_lball_is_exact(x) && adf_lball_contains_zero(x));
    ADF_CHECK(adf_place_prime_get(adf_lball_place(x)) == 2);
    fmpz_init(t);
    fmpz_set_si(t, 4);
    lb_fields(y, 7, 0, t, t, -3, 2);      /* u = 4/4 = 1 as fmpq is not canonical; fix below */
    fmpq_canonicalise(y->u);
    ADF_CHECK(adf_lball_is_canonical(y));
    adf_lball_set(x, y);
    ADF_CHECK(adf_lball_identical(x, y) && adf_lball_equal_set(x, y));
    adf_lball_set(x, x);
    ADF_CHECK(adf_lball_identical(x, y));
    adf_lball_init(y);
    adf_lball_swap(x, y);
    ADF_CHECK(x->exact == 1 && y->exact == 0 && y->p == 7 && y->v == -3 && y->N == 2);
    fmpz_clear(t);
    adf_lball_clear(x);
    adf_lball_clear(y);
}

/* Each forged value violates exactly one clause of the predicate of conventions 5.8. */
ADF_TEST(is_canonical_rejects_each_clause)
{
    adf_lball_t x;
    int i;
    adf_lball_init(x);
    /* good ones first */
    lb_fields_si(x, 5, 0, 7, 1, 0, 2); ADF_CHECK(adf_lball_is_canonical(x));      /* 7 + 25 Z_5: 7 < 25 */
    lb_fields_si(x, 5, 0, 24, 1, 0, 2); ADF_CHECK(adf_lball_is_canonical(x));     /* u = p^k - 1 */
    lb_fields_si(x, 5, 0, 0, 1, 0, -4); ADF_CHECK(adf_lball_is_canonical(x));     /* O(5^-4) */
    lb_fields_si(x, 5, 1, 0, 1, 0, 0); ADF_CHECK(adf_lball_is_canonical(x));      /* exact 0 */
    lb_fields_si(x, 5, 1, 3, 4, 7, 0); ADF_CHECK(adf_lball_is_canonical(x));      /* exact 5^7 3/4 */
    lb_fields_si(x, 2, 0, 1, 1, -5, -4); ADF_CHECK(adf_lball_is_canonical(x));    /* 2^-5 + 2^-4 Z_2 */
    lb_fields_si(x, 18446744073709551557UL, 0, 1, 1, 0, 3); ADF_CHECK(adf_lball_is_canonical(x));  /* 2^64 - 59 */
    /* bad ones */
    lb_fields_si(x, 4, 0, 3, 1, 0, 2); ADF_CHECK(!adf_lball_is_canonical(x));     /* p composite */
    lb_fields_si(x, 1, 1, 0, 1, 0, 0); ADF_CHECK(!adf_lball_is_canonical(x));     /* p = 1 */
    lb_fields_si(x, 0, 1, 0, 1, 0, 0); ADF_CHECK(!adf_lball_is_canonical(x));     /* p = 0 */
    lb_fields_si(x, 5, 2, 0, 1, 0, 0); ADF_CHECK(!adf_lball_is_canonical(x));     /* exact not in {0,1} */
    lb_fields_si(x, 5, -1, 0, 1, 0, 0); ADF_CHECK(!adf_lball_is_canonical(x));
    lb_fields_si(x, 5, 1, 2, 4, 0, 0); ADF_CHECK(!adf_lball_is_canonical(x));     /* 2/4 not canonical */
    lb_fields_si(x, 5, 1, 3, 1, 0, 1); ADF_CHECK(!adf_lball_is_canonical(x));     /* exact with N != 0 */
    lb_fields_si(x, 5, 1, 5, 1, 0, 0); ADF_CHECK(!adf_lball_is_canonical(x));     /* p | numerator */
    lb_fields_si(x, 5, 1, 1, 5, 0, 0); ADF_CHECK(!adf_lball_is_canonical(x));     /* p | denominator */
    lb_fields_si(x, 5, 1, 0, 1, 1, 0); ADF_CHECK(!adf_lball_is_canonical(x));     /* exact 0 with v != 0 */
    lb_fields_si(x, 5, 0, 0, 1, 1, 3); ADF_CHECK(!adf_lball_is_canonical(x));     /* ball around 0 with v != 0 */
    lb_fields_si(x, 5, 0, 3, 2, 0, 3); ADF_CHECK(!adf_lball_is_canonical(x));     /* ball, u not an integer */
    lb_fields_si(x, 5, 0, 7, 1, 2, 2); ADF_CHECK(!adf_lball_is_canonical(x));     /* v = N */
    lb_fields_si(x, 5, 0, 7, 1, 3, 2); ADF_CHECK(!adf_lball_is_canonical(x));     /* v > N */
    lb_fields_si(x, 5, 0, 10, 1, 0, 2); ADF_CHECK(!adf_lball_is_canonical(x));    /* p | u */
    lb_fields_si(x, 5, 0, -7, 1, 0, 2); ADF_CHECK(!adf_lball_is_canonical(x));    /* u < 0 */
    lb_fields_si(x, 5, 0, 26, 1, 0, 2); ADF_CHECK(!adf_lball_is_canonical(x));    /* u > p^(N-v) */
    lb_fields_si(x, 5, 0, 25 + 1, 1, 0, 2); ADF_CHECK(!adf_lball_is_canonical(x));/* u = p^k + 1 */
    lb_fields_si(x, 2, 0, 5, 1, 0, 2); ADF_CHECK(!adf_lball_is_canonical(x));     /* 5 >= 2^2 */
    lb_fields_si(x, 2, 0, 3, 1, 0, 2); ADF_CHECK(adf_lball_is_canonical(x));      /* 3 < 4 */
    lb_fields_si(x, 2, 0, 4, 1, 0, 3); ADF_CHECK(!adf_lball_is_canonical(x));     /* p | u, even */
    /* extreme exponents: must answer, never abort */
    lb_fields_si(x, 5, 0, 1, 1, LONG_MIN, LONG_MAX); ADF_CHECK(adf_lball_is_canonical(x));   /* N - v = 2^64 - 1 */
    lb_fields_si(x, 5, 0, 1, 1, LONG_MAX, LONG_MIN); ADF_CHECK(!adf_lball_is_canonical(x));
    lb_fields_si(x, 5, 0, 1, 1, 0, LONG_MAX); ADF_CHECK(adf_lball_is_canonical(x));
    lb_fields_si(x, 5, 0, 1, 1, -1, LONG_MAX); ADF_CHECK(adf_lball_is_canonical(x));
    lb_fields_si(x, 5, 1, 1, 1, LONG_MIN, 0); ADF_CHECK(adf_lball_is_canonical(x));
    for (i = 0; i < 3; i++)   /* a large u with a small p^(N-v): compared, not formed */
    {
        fmpz_set_ui(fmpq_numref(x->u), 3);
        fmpz_mul_2exp(fmpq_numref(x->u), fmpq_numref(x->u), 198 + (ulong) i);
        fmpz_add_ui(fmpq_numref(x->u), fmpq_numref(x->u), 1);          /* 3 2^(198 + i) + 1, prime to 3 */
        fmpz_one(fmpq_denref(x->u));
        x->p = 3; x->exact = 0; x->v = 0; x->N = 100;
        ADF_CHECK(!adf_lball_is_canonical(x));                /* about 2^199.6 >= 3^100 (about 2^158.5) */
        x->N = 200 + 7;
        ADF_CHECK(adf_lball_is_canonical(x));                 /* < 3^207 */
    }
    adf_lball_clear(x);
}

/* -------------------------------------------------------------------------------------------- vector files */

/* Runs a vector file: `body(rec)` for every line. */
typedef void (*rec_fn)(const jsonl_value *);

static void
run_vectors(const char * name, rec_fn body, size_t min_lines)
{
    char path[256];
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;
    snprintf(path, sizeof path, "tests/ref/vectors/f-slice1/%s", name);
    ADF_CHECK_MSG(jsonl_open(path, &f, &err) == 1, "%s", jsonl_error_message(&err));
    if (f == NULL)
        return;
    ADF_CHECK_MSG(jsonl_count(f) >= min_lines, "%s has %lu lines", name, (unsigned long) jsonl_count(f));
    for (i = 0; i < jsonl_count(f); i++)
        body(jsonl_record(f, i));
    jsonl_close(f);
}

static void
construct_row(const jsonl_value * rec)
{
    const char * op = member_str(rec, "op");
    ulong p = ulong_from_text(member_int(rec, "p"));
    adf_lball_t x, want;
    adf_rat_t q;
    int st;
    adf_lball_init(x);
    adf_lball_init(want);
    adf_rat_init(q);
    rat_from_json(q, member(rec, "q"));
    lb_from_json(want, member(rec, "result"));
    if (strcmp(op, "set_rat") == 0)
        st = adf_lball_set_rat(x, place_of(p), q);
    else
        st = adf_lball_set_rat_ball(x, place_of(p), q, slong_from_text(member_int(rec, "N")));
    ADF_CHECK_MSG(st == ADF_OK, "%s line %lu: status %s", op, jsonl_line_of(rec), adf_status_str(st));
    ADF_CHECK_MSG(adf_lball_is_canonical(x), "%s line %lu not canonical", op, jsonl_line_of(rec));
    ADF_CHECK_MSG(adf_lball_identical(x, want), "%s line %lu: result differs", op, jsonl_line_of(rec));
    adf_rat_clear(q);
    adf_lball_clear(x);
    adf_lball_clear(want);
}

/* set_rat and set_rat_ball carry "q"; set_fball carries A, H, d. */
static void
construct_row_dispatch(const jsonl_value * rec)
{
    const char * op = member_str(rec, "op");
    if (strcmp(op, "set_fball") == 0)
    {
        ulong p = ulong_from_text(member_int(rec, "p"));
        adf_lball_t x, want;
        fmpz_t A, H, d;
        adf_fball_t f;
        int st;
        adf_lball_init(x);
        adf_lball_init(want);
        fmpz_init(A); fmpz_init(H); fmpz_init(d);
        adf_fball_init(f);
        fmpz_from_text(A, member_int(rec, "A"));
        fmpz_from_text(H, member_int(rec, "H"));
        fmpz_from_text(d, member_int(rec, "d"));
        ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
        lb_from_json(want, member(rec, "result"));
        st = adf_lball_set_fball(x, place_of(p), f);
        ADF_CHECK_MSG(st == ADF_OK, "set_fball line %lu: %s", jsonl_line_of(rec), adf_status_str(st));
        ADF_CHECK_MSG(adf_lball_is_canonical(x) && adf_lball_identical(x, want), "set_fball line %lu",
                      jsonl_line_of(rec));
        adf_fball_clear(f);
        fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
        adf_lball_clear(x);
        adf_lball_clear(want);
    }
    else
        construct_row(rec);
}

/* Fails if set_rat / set_rat_ball / set_fball give a result different from the reference: the reference reduces
   c modulo p^N with pow(den, -1, p^k), the library with fmpz_invmod; both give the unique centre of L0. */
ADF_TEST(vectors_construct)
{
    run_vectors("lball_construct.jsonl", construct_row_dispatch, 2000);
}

static void
binary_row(const jsonl_value * rec)
{
    const char * op = member_str(rec, "op");
    adf_lball_t x, y, z, want, s;
    int st, want_st = status_from_name(member_str(rec, "status"));
    int (*fn)(adf_lball_t, const adf_lball_t, const adf_lball_t) = NULL;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(want); adf_lball_init(s);
    lb_from_json(x, member(rec, "x"));
    lb_from_json(y, member(rec, "y"));
    ADF_CHECK_MSG(adf_lball_is_canonical(x) && adf_lball_is_canonical(y), "line %lu: input", jsonl_line_of(rec));
    if (strcmp(op, "add") == 0) fn = adf_lball_add;
    else if (strcmp(op, "sub") == 0) fn = adf_lball_sub;
    else if (strcmp(op, "mul") == 0) fn = adf_lball_mul;
    else fn = adf_lball_div;
    sentinel(z);
    sentinel(s);
    st = fn(z, x, y);
    ADF_CHECK_MSG(st == want_st, "%s line %lu: status %s, want %s", op, jsonl_line_of(rec), adf_status_str(st),
                  adf_status_str(want_st));
    if (want_st == ADF_OK)
    {
        lb_from_json(want, member(rec, "result"));
        ADF_CHECK_MSG(adf_lball_is_canonical(z), "%s line %lu: result not canonical", op, jsonl_line_of(rec));
        ADF_CHECK_MSG(adf_lball_identical(z, want), "%s line %lu: result differs", op, jsonl_line_of(rec));
    }
    else
    {
        ADF_CHECK_MSG(adf_lball_identical(z, s), "%s line %lu: output touched on a status", op, jsonl_line_of(rec));
    }
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(want); adf_lball_clear(s);
}

/* Fails if the rule for a sum, product, quotient is wrong at any of the 4500 recorded pairs: exponent of the
   result (min, or min of three terms), centre reduction, valuation of the products, the statuses of a quotient. */
ADF_TEST(vectors_binary)
{
    run_vectors("lball_binary.jsonl", binary_row, 4000);
}

static void
unary_row(const jsonl_value * rec)
{
    const char * op = member_str(rec, "op");
    adf_lball_t x, z, want, s;
    int st, want_st = status_from_name(member_str(rec, "status"));
    adf_lball_init(x); adf_lball_init(z); adf_lball_init(want); adf_lball_init(s);
    lb_from_json(x, member(rec, "x"));
    ADF_CHECK_MSG(adf_lball_is_canonical(x), "line %lu: input", jsonl_line_of(rec));
    sentinel(z);
    sentinel(s);
    if (strcmp(op, "neg") == 0 || strcmp(op, "inv") == 0)
    {
        st = strcmp(op, "neg") == 0 ? adf_lball_neg(z, x) : adf_lball_inv(z, x);
        ADF_CHECK_MSG(st == want_st, "%s line %lu: status %s, want %s", op, jsonl_line_of(rec), adf_status_str(st),
                      adf_status_str(want_st));
        if (want_st == ADF_OK)
        {
            lb_from_json(want, member(rec, "result"));
            ADF_CHECK_MSG(adf_lball_identical(z, want) && adf_lball_is_canonical(z), "%s line %lu: result",
                          op, jsonl_line_of(rec));
        }
        else
            ADF_CHECK_MSG(adf_lball_identical(z, s), "%s line %lu: output touched", op, jsonl_line_of(rec));
    }
    else if (strcmp(op, "valuation") == 0)
    {
        slong v = 1234;
        int is_inf = 77;
        st = adf_lball_valuation(&v, &is_inf, x);
        ADF_CHECK_MSG(st == want_st, "valuation line %lu: status %s", jsonl_line_of(rec), adf_status_str(st));
        if (want_st == ADF_OK)
            ADF_CHECK_MSG(v == slong_from_text(member_int(rec, "v")) &&
                          is_inf == (int) slong_from_text(member_int(rec, "is_inf")),
                          "valuation line %lu: %ld %d", jsonl_line_of(rec), (long) v, is_inf);
        else
            ADF_CHECK_MSG(v == 1234 && is_inf == 77, "valuation line %lu: output touched", jsonl_line_of(rec));
    }
    else if (strcmp(op, "abs") == 0)
    {
        adf_rat_t a, w;
        adf_rat_init(a); adf_rat_init(w);
        fmpq_set_si(a->q, 5, 7);
        st = adf_lball_abs(a, x);
        ADF_CHECK_MSG(st == want_st, "abs line %lu: status %s", jsonl_line_of(rec), adf_status_str(st));
        if (want_st == ADF_OK)
        {
            rat_from_json(w, member(rec, "result"));
            ADF_CHECK_MSG(fmpq_equal(a->q, w->q), "abs line %lu", jsonl_line_of(rec));
        }
        else
            ADF_CHECK_MSG(fmpz_equal_si(fmpq_numref(a->q), 5) && fmpz_equal_si(fmpq_denref(a->q), 7),
                          "abs line %lu: output touched", jsonl_line_of(rec));
        adf_rat_clear(a); adf_rat_clear(w);
    }
    else
    {
        slong m = 4321;
        st = adf_lball_decompose(&m, z, x);
        ADF_CHECK_MSG(st == want_st, "decompose line %lu: status %s", jsonl_line_of(rec), adf_status_str(st));
        if (want_st == ADF_OK)
        {
            lb_from_json(want, member(rec, "unit"));
            ADF_CHECK_MSG(m == slong_from_text(member_int(rec, "m")), "decompose line %lu: m = %ld",
                          jsonl_line_of(rec), (long) m);
            ADF_CHECK_MSG(adf_lball_identical(z, want) && adf_lball_is_canonical(z), "decompose line %lu: unit",
                          jsonl_line_of(rec));
        }
        else
            ADF_CHECK_MSG(m == 4321 && adf_lball_identical(z, s), "decompose line %lu: output touched",
                          jsonl_line_of(rec));
    }
    adf_lball_clear(x); adf_lball_clear(z); adf_lball_clear(want); adf_lball_clear(s);
}

ADF_TEST(vectors_unary)
{
    run_vectors("lball_unary.jsonl", unary_row, 5000);
}

static void
predicate_row(const jsonl_value * rec)
{
    const char * op = member_str(rec, "op");
    adf_lball_t x, y;
    int got = -1, want = member_bool(rec, "result");
    adf_lball_init(x); adf_lball_init(y);
    lb_from_json(x, member(rec, "x"));
    lb_from_json(y, member(rec, "y"));
    ADF_CHECK_MSG(adf_lball_is_canonical(x) && adf_lball_is_canonical(y), "line %lu: input", jsonl_line_of(rec));
    if (strcmp(op, "equal_set") == 0) got = adf_lball_equal_set(x, y);
    else if (strcmp(op, "overlaps") == 0) got = adf_lball_overlaps(x, y);
    else got = adf_lball_contains(x, y);
    ADF_CHECK_MSG(got == want, "%s line %lu: got %d, want %d", op, jsonl_line_of(rec), got, want);
    adf_lball_clear(x); adf_lball_clear(y);
}

ADF_TEST(vectors_predicates)
{
    run_vectors("lball_predicates.jsonl", predicate_row, 10000);
}

/* ------------------------------------------------------------------------------------------- enumeration */

typedef struct
{
    adf_lball_struct * v;
    size_t n, cap;
} universe_t;

static void
uni_push(universe_t * U, ulong p, int exact, slong un, ulong ud, slong v, slong N)
{
    if (U->n == U->cap)
    {
        U->cap = U->cap ? 2 * U->cap : 64;
        U->v = (adf_lball_struct *) realloc(U->v, U->cap * sizeof(adf_lball_struct));
    }
    fmpq_init(U->v[U->n].u);
    lb_fields_si(U->v + U->n, p, exact, un, ud, v, N);
    fmpq_canonicalise(U->v[U->n].u);
    ADF_CHECK(adf_lball_is_canonical(U->v + U->n));
    U->n++;
}

static void
uni_free(universe_t * U)
{
    size_t i;
    for (i = 0; i < U->n; i++)
        fmpq_clear(U->v[i].u);
    free(U->v);
    U->v = NULL;
    U->n = U->cap = 0;
}

/* All canonical balls with valuation in [-vmax, vmax], relative precision 1..kmax; balls around 0 with N in
   [-vmax, kmax]; the exact values 0, 1, -1, +-p^v, and +-3/2 * p^v when p is odd and 3/2 is a unit there. */
static void
uni_build(universe_t * U, ulong p, slong vmax, slong kmax)
{
    slong v, k, N;
    ulong u, pk;
    for (v = -vmax; v <= vmax; v++)
    {
        pk = 1;
        for (k = 1; k <= kmax; k++)
        {
            pk *= p;
            for (u = 1; u < pk; u++)
                if (u % p)
                    uni_push(U, p, 0, (slong) u, 1, v, v + k);
        }
    }
    for (N = -vmax; N <= kmax; N++)
        uni_push(U, p, 0, 0, 1, 0, N);
    uni_push(U, p, 1, 0, 1, 0, 0);
    for (v = -vmax; v <= vmax; v++)
    {
        uni_push(U, p, 1, 1, 1, v, 0);
        uni_push(U, p, 1, -1, 1, v, 0);
        if (p != 2 && p != 3)
            uni_push(U, p, 1, 3, 2, v, 0);
        else if (p == 3)
            uni_push(U, p, 1, 5, 2, v, 0);
        else
            uni_push(U, p, 1, 3, 5, v, 0);
    }
}

/* op: 0 add, 1 sub, 2 mul, 3 div, 4 neg, 5 inv. Returns 0 if a check failed. The expected status is decided
   here from the operands alone (a divisor that is the exact 0, or a ball around 0, has no inverse). */
static int
enum_case(int op, const adf_lball_struct * x, const adf_lball_struct * y)
{
    static const char * const names[] = {"add", "sub", "mul", "div", "neg", "inv"};
    ulong p = x->p, a, b, nclass = 0, i, na, nb;
    adf_lball_t z;
    int st, want_st = ADF_OK, good = 1;
    adf_lball_t * seen = (adf_lball_t *) malloc(p * sizeof(adf_lball_t));
    const adf_lball_struct * d = (op == 5 || op == 3) ? (op == 3 ? y : x) : NULL;
    fmpq_t s, t, r;

    if (d != NULL && fmpq_is_zero(d->u))
        want_st = d->exact ? ADF_NOT_UNIT : ADF_UNIT_NOT_CERTIFIED;
    adf_lball_init(z);
    sentinel(z);
    if (op == 0) st = adf_lball_add(z, x, y);
    else if (op == 1) st = adf_lball_sub(z, x, y);
    else if (op == 2) st = adf_lball_mul(z, x, y);
    else if (op == 3) st = adf_lball_div(z, x, y);
    else if (op == 4) st = adf_lball_neg(z, x);
    else st = adf_lball_inv(z, x);
    if (st != want_st)
    {
        ADF_CHECK_MSG(0, "%s p=%lu: status %s, want %s", names[op], p, adf_status_str(st), adf_status_str(want_st));
        adf_lball_clear(z);
        free(seen);
        return 0;
    }
    if (want_st != ADF_OK)
    {
        adf_lball_t sen;
        adf_lball_init(sen);
        sentinel(sen);
        ADF_CHECK(adf_lball_identical(z, sen));
        adf_lball_clear(sen);
        adf_lball_clear(z);
        free(seen);
        return 1;
    }
    ADF_CHECK(adf_lball_is_canonical(z));
    fmpq_init(s); fmpq_init(t); fmpq_init(r);
    na = x->exact ? 1 : p;
    nb = (op >= 4 || y->exact) ? 1 : p;
    for (a = 0; a < na; a++)
        for (b = 0; b < nb; b++)
        {
            adf_lball_t e;
            point_of(s, x, a);
            if (op < 4)
                point_of(t, y, b);
            if (op == 3 && fmpq_is_zero(t))
                continue;
            if (op == 5 && fmpq_is_zero(s))
                continue;
            if (op == 0) fmpq_add(r, s, t);
            else if (op == 1) fmpq_sub(r, s, t);
            else if (op == 2) fmpq_mul(r, s, t);
            else if (op == 3) fmpq_div(r, s, t);
            else if (op == 4) fmpq_neg(r, s);
            else fmpq_inv(r, s);
            adf_lball_init(e);
            exact_of(e, p, r);
            if (!adf_lball_contains(e, z))
            {
                ADF_CHECK_MSG(0, "%s p=%lu: a result point is outside the ball (x.v=%ld x.N=%ld y.v=%ld y.N=%ld)",
                              names[op], p, (long) x->v, (long) x->N, y ? (long) y->v : 0L, y ? (long) y->N : 0L);
                good = 0;
            }
            if (!z->exact)
            {
                adf_lball_t c;
                int dup = 0;
                adf_lball_init(c);
                ball_of(c, p, r, z->N + 1);
                for (i = 0; i < nclass; i++)
                    if (adf_lball_equal_set(seen[i], c))
                        dup = 1;
                if (!dup)
                {
                    adf_lball_init(seen[nclass]);
                    adf_lball_set(seen[nclass], c);
                    nclass++;
                }
                adf_lball_clear(c);
            }
            adf_lball_clear(e);
        }
    if (!z->exact && nclass != p)
    {
        ADF_CHECK_MSG(0, "%s p=%lu: %lu classes modulo p^(K+1), want p (not tight); x=(%ld,%ld) y=(%ld,%ld)",
                      names[op], p, nclass, (long) x->v, (long) x->N, y ? (long) y->v : 0L, y ? (long) y->N : 0L);
        good = 0;
    }
    for (i = 0; i < nclass; i++)
        adf_lball_clear(seen[i]);
    fmpq_clear(s); fmpq_clear(t); fmpq_clear(r);
    adf_lball_clear(z);
    free(seen);
    return good;
}

static void
enumerate_prime(ulong p, slong vmax, slong kmax, size_t sample, unsigned long seed)
{
    universe_t U = {NULL, 0, 0};
    size_t i, j, cases = 0, unary = 0;
    int op;
    uni_build(&U, p, vmax, kmax);
    srand((unsigned) seed);
    for (op = 0; op < 4; op++)
    {
        if (sample == 0)
        {
            for (i = 0; i < U.n; i++)
                for (j = 0; j < U.n; j++)
                {
                    enum_case(op, U.v + i, U.v + j);
                    cases++;
                }
        }
        else
        {
            size_t c;
            for (c = 0; c < sample; c++)
            {
                size_t a = (size_t) rand() % U.n;       /* two statements: the order of evaluation of the */
                size_t b = (size_t) rand() % U.n;       /* arguments is unspecified, and gcc and clang differ */
                enum_case(op, U.v + a, U.v + b);
                cases++;
            }
        }
    }
    for (i = 0; i < U.n; i++)
    {
        enum_case(4, U.v + i, NULL);
        enum_case(5, U.v + i, NULL);
        unary += 2;
    }
    printf("   enumeration p=%lu: %lu balls, %lu binary cases, %lu unary cases\n", p, (unsigned long) U.n,
           (unsigned long) cases, (unsigned long) unary);
    uni_free(&U);
}

/* Fails if a result excludes one point result (not an enclosure), or if the classes modulo p^(K + 1) of the point
   results are fewer than p (a smaller ball exists: not tight), for add, sub, mul, div, neg, inv on every pair of
   the universe. E.g. a product exponent min(v + N', v' + N) without N + N' fails on two balls around 0. */
ADF_TEST(enumeration_p2)
{
    enumerate_prime(2, 2, 3, 0, 1);
}

ADF_TEST(enumeration_p3)
{
    enumerate_prime(3, 1, 2, 0, 2);
}

ADF_TEST(enumeration_p5)
{
    enumerate_prime(5, 1, 2, 4000, 3);
}

ADF_TEST(enumeration_p7)
{
    enumerate_prime(7, 1, 2, 2000, 4);
}

/* ------------------------------------------------------------------- statuses, aliasing, limits, decomposition */

ADF_TEST(statuses_and_untouched_outputs)
{
    adf_lball_t zero, ballzero, one, pd, z, s, other;
    adf_place_t p5 = place_of(5), p3 = place_of(3);
    adf_rat_t q, a;
    slong v = 99, m = 98, N = 97;
    int inf_ = 96, st;
    adf_lball_init(zero); adf_lball_init(ballzero); adf_lball_init(one); adf_lball_init(pd);
    adf_lball_init(z); adf_lball_init(s); adf_lball_init(other);
    adf_rat_init(q); adf_rat_init(a);
    fmpq_set_si(q->q, 1, 3);
    ADF_CHECK(adf_lball_set_rat(one, p5, q) == ADF_OK);            /* 1/3 exact at 5 */
    adf_rat_zero(q);
    ADF_CHECK(adf_lball_set_rat(zero, p5, q) == ADF_OK);
    ADF_CHECK(adf_lball_set_rat_ball(ballzero, p5, q, 4) == ADF_OK);
    ADF_CHECK(zero->exact && !ballzero->exact && ballzero->N == 4 && fmpq_is_zero(ballzero->u));
    ADF_CHECK(!adf_lball_equal_set(zero, ballzero) && adf_lball_contains(zero, ballzero));
    ADF_CHECK(!adf_lball_contains(ballzero, zero));
    fmpq_set_si(q->q, 3, 1);
    ADF_CHECK(adf_lball_set_rat_ball(pd, p3, q, 3) == ADF_OK);      /* 3 + 27 Z_3: v = 1, unit 1 mod 9 */
    ADF_CHECK(pd->v == 1 && pd->N == 3 && fmpq_is_one(pd->u));

    sentinel(s);
    sentinel(z);
    /* the exact 0: NOT_UNIT for inv, div by it; DOMAIN for decompose; valuation infinity */
    ADF_CHECK(adf_lball_inv(z, zero) == ADF_NOT_UNIT && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_div(z, one, zero) == ADF_NOT_UNIT && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_decompose(&m, z, zero) == ADF_DOMAIN && m == 98 && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_valuation(&v, &inf_, zero) == ADF_OK && inf_ == 1 && v == 0);
    v = 99; inf_ = 96;
    /* a ball around 0 */
    ADF_CHECK(adf_lball_inv(z, ballzero) == ADF_UNIT_NOT_CERTIFIED && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_div(z, one, ballzero) == ADF_UNIT_NOT_CERTIFIED && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_valuation(&v, &inf_, ballzero) == ADF_NOT_DETERMINED && v == 99 && inf_ == 96);
    ADF_CHECK(adf_lball_decompose(&m, z, ballzero) == ADF_NOT_DETERMINED && m == 98 && adf_lball_identical(z, s));
    fmpq_set_si(a->q, 5, 7);
    ADF_CHECK(adf_lball_abs(a, ballzero) == ADF_NOT_DETERMINED && fmpz_equal_si(fmpq_numref(a->q), 5));
    ADF_CHECK(adf_lball_abs(a, zero) == ADF_OK && fmpq_is_zero(a->q));
    /* the accessors */
    ADF_CHECK(adf_lball_get_prec(&N, zero) == ADF_DOMAIN && N == 97);
    ADF_CHECK(adf_lball_get_prec(&N, ballzero) == ADF_OK && N == 4);
    ADF_CHECK(adf_lball_contains_zero(zero) && adf_lball_contains_zero(ballzero) && !adf_lball_contains_zero(one));
    ADF_CHECK(!adf_lball_contains_zero(pd) && !adf_lball_is_exact(pd) && adf_lball_is_exact(one));
    /* two primes */
    ADF_CHECK(adf_lball_add(z, one, pd) == ADF_DOMAIN && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_sub(z, one, pd) == ADF_DOMAIN && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_mul(z, one, pd) == ADF_DOMAIN && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_div(z, one, pd) == ADF_DOMAIN && adf_lball_identical(z, s));
    ADF_CHECK(!adf_lball_equal_set(one, pd) && !adf_lball_overlaps(one, pd) && !adf_lball_contains(one, pd));
    /* the archimedean place */
    adf_lball_set(other, s);
    ADF_CHECK(adf_lball_set_rat(z, adf_place_inf(), q) == ADF_DOMAIN && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_set_rat_ball(z, adf_place_inf(), q, 3) == ADF_DOMAIN && adf_lball_identical(z, s));
    {
        adf_fball_t f;
        adf_fball_init(f);
        ADF_CHECK(adf_lball_set_fball(z, adf_place_inf(), f) == ADF_DOMAIN && adf_lball_identical(z, s));
        adf_fball_clear(f);
    }
    /* valuation, abs and decompose of an ordinary ball and of an exact value */
    ADF_CHECK(adf_lball_valuation(&v, &inf_, pd) == ADF_OK && v == 1 && inf_ == 0);
    ADF_CHECK(adf_lball_abs(a, pd) == ADF_OK && fmpz_is_one(fmpq_numref(a->q)) && fmpz_equal_si(fmpq_denref(a->q), 3));
    ADF_CHECK(adf_lball_decompose(&m, z, pd) == ADF_OK && m == 1 && z->v == 0 && z->N == 2 && fmpq_is_one(z->u) &&
              !z->exact);
    st = adf_lball_decompose(&m, z, one);
    ADF_CHECK(st == ADF_OK && m == 0 && z->exact && fmpz_is_one(fmpq_numref(z->u)) &&
              fmpz_equal_si(fmpq_denref(z->u), 3));
    adf_lball_clear(zero); adf_lball_clear(ballzero); adf_lball_clear(one); adf_lball_clear(pd);
    adf_lball_clear(z); adf_lball_clear(s); adf_lball_clear(other);
    adf_rat_clear(q); adf_rat_clear(a);
}

/* every combination of aliasing of the binary and unary functions, at OK and at a status */
ADF_TEST(aliasing)
{
    universe_t U = {NULL, 0, 0};
    size_t i, j, checked = 0;
    uni_build(&U, 3, 1, 2);
    for (i = 0; i < U.n; i += 3)
        for (j = 0; j < U.n; j += 2)
        {
            int op;
            for (op = 0; op < 4; op++)
            {
                int (*fn)(adf_lball_t, const adf_lball_t, const adf_lball_t) =
                    op == 0 ? adf_lball_add : op == 1 ? adf_lball_sub : op == 2 ? adf_lball_mul : adf_lball_div;
                adf_lball_t ref, a, b, c, orig;
                int st, st2;
                adf_lball_init(ref); adf_lball_init(a); adf_lball_init(b); adf_lball_init(c); adf_lball_init(orig);
                sentinel(ref);
                st = fn(ref, U.v + i, U.v + j);
                /* (x, x, y) */
                adf_lball_set(a, U.v + i);
                st2 = fn(a, a, U.v + j);
                ADF_CHECK(st2 == st);
                ADF_CHECK(st == ADF_OK ? adf_lball_identical(a, ref) : adf_lball_identical(a, U.v + i));
                /* (y, x, y) */
                adf_lball_set(b, U.v + j);
                st2 = fn(b, U.v + i, b);
                ADF_CHECK(st2 == st);
                ADF_CHECK(st == ADF_OK ? adf_lball_identical(b, ref) : adf_lball_identical(b, U.v + j));
                /* (x, x, x) against the out-of-place x op x */
                adf_lball_set(orig, U.v + i);
                sentinel(c);
                st = fn(c, U.v + i, U.v + i);
                adf_lball_set(a, U.v + i);
                st2 = fn(a, a, a);
                ADF_CHECK(st2 == st);
                ADF_CHECK(st == ADF_OK ? adf_lball_identical(a, c) : adf_lball_identical(a, orig));
                checked++;
                adf_lball_clear(ref); adf_lball_clear(a); adf_lball_clear(b); adf_lball_clear(c);
                adf_lball_clear(orig);
            }
        }
    for (i = 0; i < U.n; i++)
    {
        adf_lball_t ref, a;
        int st, st2;
        slong m1, m2;
        adf_lball_init(ref); adf_lball_init(a);
        sentinel(ref);
        st = adf_lball_neg(ref, U.v + i);
        adf_lball_set(a, U.v + i);
        st2 = adf_lball_neg(a, a);
        ADF_CHECK(st == ADF_OK && st2 == ADF_OK && adf_lball_identical(a, ref));
        sentinel(ref);
        st = adf_lball_inv(ref, U.v + i);
        adf_lball_set(a, U.v + i);
        st2 = adf_lball_inv(a, a);
        ADF_CHECK(st2 == st);
        ADF_CHECK(st == ADF_OK ? adf_lball_identical(a, ref) : adf_lball_identical(a, U.v + i));
        sentinel(ref);
        st = adf_lball_decompose(&m1, ref, U.v + i);
        adf_lball_set(a, U.v + i);
        st2 = adf_lball_decompose(&m2, a, a);
        ADF_CHECK(st2 == st);
        ADF_CHECK(st == ADF_OK ? (adf_lball_identical(a, ref) && m1 == m2) : adf_lball_identical(a, U.v + i));
        adf_lball_clear(ref); adf_lball_clear(a);
        checked += 3;
    }
    printf("   aliasing: %lu cases\n", (unsigned long) checked);
    uni_free(&U);
}

/* The unit part of a decomposition has valuation 0, stored as v = 0 with u a unit or a ball of v = 0 (L6). The check
   reads the valuation through the library and the field: the wrong unit 5 with v = 1 (finding F6 of
   docs/reviews/f1/review-lball.md) fails both. The earlier assertion `v == 0 || (exact && u != 0)` accepted it. */
static int
unit_part_ok(const adf_lball_t unit)
{
    slong vu = 12345;
    int inf = 1;
    return adf_lball_is_canonical(unit) && unit->v == 0 && !fmpq_is_zero(unit->u) &&
           adf_lball_valuation(&vu, &inf, unit) == ADF_OK && vu == 0 && inf == 0;
}

ADF_TEST(unit_part_check_rejects_the_wrong_unit)
{
    adf_lball_t w;
    adf_lball_init(w);
    lb_fields_si(w, 5, 1, 1, 1, 1, 0);                /* exact 5: canonical, valuation 1, the reviewer's example */
    ADF_CHECK(adf_lball_is_canonical(w));
    ADF_CHECK(!unit_part_ok(w));
    lb_fields_si(w, 5, 1, 3, 2, 0, 0);                /* exact 3/2: a unit at 5 */
    ADF_CHECK(unit_part_ok(w));
    lb_fields_si(w, 5, 0, 7, 1, 0, 2);                /* 7 + 25 Z_5 */
    ADF_CHECK(unit_part_ok(w));
    lb_fields_si(w, 5, 0, 7, 1, 1, 3);                /* a ball of valuation 1 is not a unit part */
    ADF_CHECK(!unit_part_ok(w));
    lb_fields_si(w, 5, 1, 0, 1, 0, 0);                /* the exact 0 has no valuation 0 unit */
    ADF_CHECK(!unit_part_ok(w));
    adf_lball_clear(w);
}

/* x = p^m unit is recovered: exact p^m times the unit is x, for every value with a decomposition. */
ADF_TEST(decompose_roundtrip)
{
    ulong ps[3] = {2, 3, 5};
    int k;
    size_t i, n = 0;
    for (k = 0; k < 3; k++)
    {
        universe_t U = {NULL, 0, 0};
        uni_build(&U, ps[k], 2, 2);
        for (i = 0; i < U.n; i++)
        {
            adf_lball_t unit, pm, prod;
            fmpq_t c;
            slong m;
            int st;
            adf_lball_init(unit); adf_lball_init(pm); adf_lball_init(prod);
            fmpq_init(c);
            st = adf_lball_decompose(&m, unit, U.v + i);
            if (st == ADF_OK)
            {
                ppow(c, ps[k], m);
                exact_of(pm, ps[k], c);
                ADF_CHECK(adf_lball_mul(prod, pm, unit) == ADF_OK);
                ADF_CHECK(adf_lball_equal_set(prod, U.v + i));
                ADF_CHECK(unit_part_ok(unit));
                {
                    slong vx = 54321;
                    int infx = 1;
                    ADF_CHECK(adf_lball_valuation(&vx, &infx, U.v + i) == ADF_OK && infx == 0 && vx == m);
                }
                if (!unit->exact)
                    ADF_CHECK(unit->N >= 1 && unit->v == 0);
                n++;
            }
            fmpq_clear(c);
            adf_lball_clear(unit); adf_lball_clear(pm); adf_lball_clear(prod);
        }
        uni_free(&U);
    }
    ADF_CHECK(n > 100);
}

ADF_TEST(inverse_times_self_contains_one)
{
    /* a ball at p = 5 with relative precision 3000 (about 7000 bits): x * (1/x) contains 1, and contains
       exactly the ball 1 + 5^K Z_5 with K = N - 2v + v ... the relative precision N - v is kept */
    adf_lball_t x, y, prod, one;
    adf_rat_t q;
    fmpz_t big;
    fmpq_t c;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(prod); adf_lball_init(one);
    adf_rat_init(q);
    fmpz_init(big);
    fmpq_init(c);
    fmpz_set_ui(big, 5);
    fmpz_pow_ui(big, big, 3000);
    fmpz_sub_ui(big, big, 12346);             /* 5^3000 - 12346, prime to 5 */
    fmpz_set(fmpq_numref(q->q), big);
    fmpz_set_ui(fmpq_denref(q->q), 7);
    ADF_CHECK(adf_lball_set_rat_ball(x, place_of(5), q, 3000) == ADF_OK);    /* v = 0, relative precision 3000 */
    ADF_CHECK(x->v == 0 && x->N == 3000 && fmpz_bits(fmpq_numref(x->u)) > 6900);
    ADF_CHECK(adf_lball_inv(y, x) == ADF_OK);
    ADF_CHECK(y->v == 0 && y->N == 3000);
    ADF_CHECK(adf_lball_mul(prod, x, y) == ADF_OK);
    fmpq_one(c);
    exact_of(one, 5, c);
    ADF_CHECK(adf_lball_contains(one, prod));
    ADF_CHECK(prod->N == 3000 && prod->v == 0 && fmpq_is_one(prod->u));
    /* x / x contains 1 too */
    ADF_CHECK(adf_lball_div(prod, x, x) == ADF_OK && adf_lball_contains(one, prod) && prod->N == 3000);
    fmpq_clear(c);
    fmpz_clear(big);
    adf_rat_clear(q);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(prod); adf_lball_clear(one);
}

ADF_TEST(the_prime_2_64_minus_59)
{
    ulong p = 18446744073709551557UL;
    adf_lball_t x, y, z;
    adf_rat_t q;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z);
    adf_rat_init(q);
    fmpq_set_si(q->q, 1, 3);
    ADF_CHECK(adf_lball_set_rat_ball(x, place_of(p), q, 4) == ADF_OK);
    ADF_CHECK(adf_lball_is_canonical(x) && x->v == 0 && x->N == 4);
    ADF_CHECK(adf_lball_inv(y, x) == ADF_OK && fmpz_equal_si(fmpq_numref(y->u), 3));   /* 3 + p^4 Z_p */
    {
        adf_lball_t three;
        fmpq_t c;
        fmpq_init(c);
        fmpq_set_si(c, 3, 1);
        adf_lball_init(three);
        ball_of(three, p, c, 4);
        ADF_CHECK(adf_lball_equal_set(y, three));
        fmpq_clear(c);
        adf_lball_clear(three);
    }
    ADF_CHECK(adf_lball_mul(z, x, y) == ADF_OK && fmpq_is_one(z->u) && z->N == 4 && z->v == 0);
    ADF_CHECK(adf_lball_add(z, x, y) == ADF_OK && z->N == 4 && adf_lball_is_canonical(z));
    adf_rat_clear(q);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z);
}

/* the limits and extreme exponents: statuses before any allocation, and the cases that must work */
ADF_TEST(limits_and_extreme_exponents)
{
    adf_lball_t x, y, z, s, e;
    adf_rat_t q, r;
    slong big = ADF_LBALL_EXP_MAX;
    adf_place_t p5 = place_of(5);
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(s); adf_lball_init(e);
    adf_rat_init(q); adf_rat_init(r);
    sentinel(s);
    sentinel(z);
    fmpq_set_si(q->q, 1, 1);
    /* 1 + 5^(2^60) Z_5 is stored without forming 5^(2^60): u = 1, v = 0 */
    ADF_CHECK(adf_lball_set_rat_ball(x, p5, q, big) == ADF_OK);
    ADF_CHECK(x->N == big && x->v == 0 && fmpq_is_one(x->u) && adf_lball_is_canonical(x));
    ADF_CHECK(adf_lball_set_rat_ball(y, p5, q, -big) == ADF_OK && fmpq_is_zero(y->u) && y->N == -big);
    /* beyond the bound */
    ADF_CHECK(adf_lball_set_rat_ball(z, p5, q, big + 1) == ADF_LIMIT && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_set_rat_ball(z, p5, q, -big - 1) == ADF_LIMIT && adf_lball_identical(z, s));
    /* -1 needs p^k: k = 2^40 is beyond the bit bound, k = 1000 is not */
    fmpq_set_si(q->q, -1, 1);
    ADF_CHECK(adf_lball_set_rat_ball(z, p5, q, (slong) 1 << 40) == ADF_LIMIT && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_set_rat_ball(z, p5, q, 1000) == ADF_OK && z->N == 1000 && !fmpq_is_one(z->u));
    /* the same ball as a sum: 1 + 5^(2^60) with the exact 1 gives 2 + ... */
    fmpq_one(r->q);
    ADF_CHECK(adf_lball_set_rat(e, p5, r) == ADF_OK);
    ADF_CHECK(adf_lball_add(z, x, e) == ADF_OK && z->N == big && fmpz_equal_si(fmpq_numref(z->u), 2));
    ADF_CHECK(adf_lball_mul(z, x, x) == ADF_OK && z->N == big && fmpq_is_one(z->u) && z->v == 0);
    ADF_CHECK(adf_lball_inv(z, x) == ADF_OK && z->N == big && fmpq_is_one(z->u));
    /* 3 + O(5^(2^40)) has no cheap inverse: 1/3 needs 5^(2^40) */
    fmpq_set_si(q->q, 3, 1);
    ADF_CHECK(adf_lball_set_rat_ball(y, p5, q, (slong) 1 << 40) == ADF_OK && fmpz_equal_si(fmpq_numref(y->u), 3));
    sentinel(z);
    ADF_CHECK(adf_lball_inv(z, y) == ADF_LIMIT && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_div(z, x, y) == ADF_LIMIT && adf_lball_identical(z, s));
    /* forged inputs beyond the exponent bound */
    lb_fields_si(y, 5, 0, 1, 1, 0, big + 1);
    ADF_CHECK(adf_lball_is_canonical(y));
    ADF_CHECK(adf_lball_add(z, y, x) == ADF_LIMIT && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_mul(z, y, x) == ADF_LIMIT && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_neg(z, y) == ADF_LIMIT && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_inv(z, y) == ADF_LIMIT && adf_lball_identical(z, s));
    lb_fields_si(y, 5, 0, 1, 1, -big - 1, 0);
    ADF_CHECK(adf_lball_add(z, y, x) == ADF_LIMIT && adf_lball_identical(z, s));
    /* products and inverses whose exponent leaves the bound: N + N' */
    lb_fields_si(y, 5, 0, 0, 1, 0, big);
    ADF_CHECK(adf_lball_mul(z, y, y) == ADF_LIMIT && adf_lball_identical(z, s));   /* 2^61 */
    lb_fields_si(y, 5, 0, 1, 1, -big / 2 - 1, big);                                    /* inverse: N - 2v > 2^60 */
    ADF_CHECK(adf_lball_inv(z, y) == ADF_LIMIT && adf_lball_identical(z, s));
    /* an exact value 5^(2^40): the sum with an exact 1 needs 5^(2^40); the product with itself does not */
    lb_fields_si(y, 5, 1, 1, 1, (slong) 1 << 40, 0);
    ADF_CHECK(adf_lball_is_canonical(y));
    ADF_CHECK(adf_lball_add(z, y, e) == ADF_LIMIT && adf_lball_identical(z, s));
    ADF_CHECK(adf_lball_mul(z, y, y) == ADF_OK && z->exact && z->v == ((slong) 1 << 41) && fmpq_is_one(z->u));
    ADF_CHECK(adf_lball_inv(z, y) == ADF_OK && z->exact && z->v == -((slong) 1 << 40));
    /* an operand of valuation >= K is dropped from a sum: 7 + O(5^10) plus the exact 5^(2^40) is 7 + O(5^10), and the
       product is 7 5^(2^40) + O(5^(2^40 + 10)); neither forms 5^(2^40) */
    fmpq_set_si(q->q, 7, 1);
    ADF_CHECK(adf_lball_set_rat_ball(x, p5, q, 10) == ADF_OK);
    ADF_CHECK(adf_lball_add(z, x, y) == ADF_OK && adf_lball_identical(z, x));
    ADF_CHECK(adf_lball_add(z, y, x) == ADF_OK && adf_lball_identical(z, x));
    ADF_CHECK(adf_lball_sub(z, x, y) == ADF_OK && adf_lball_identical(z, x));
    ADF_CHECK(adf_lball_mul(z, x, y) == ADF_OK && !z->exact && z->v == ((slong) 1 << 40) && z->N == ((slong) 1 << 40) + 10 &&
              fmpz_equal_si(fmpq_numref(z->u), 7));
    sentinel(z);
    ADF_CHECK(adf_lball_abs(r, y) == ADF_LIMIT && fmpq_is_one(r->q));
    ADF_CHECK(adf_lball_get_center(r, y) == ADF_LIMIT && fmpq_is_one(r->q));
    ADF_CHECK(adf_lball_valuation(&big, &(int){0}, y) == ADF_OK && big == ((slong) 1 << 40));
    adf_rat_clear(q); adf_rat_clear(r);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(s); adf_lball_clear(e);
}

ADF_TEST(get_center_and_abs_values)
{
    adf_lball_t x;
    adf_rat_t q, c;
    adf_lball_init(x);
    adf_rat_init(q); adf_rat_init(c);
    fmpq_set_si(q->q, 7, 4);
    ADF_CHECK(adf_lball_set_rat_ball(x, place_of(2), q, 0) == ADF_OK);       /* v = -2, 7 + 4 * ... */
    ADF_CHECK(x->v == -2 && x->N == 0 && fmpz_equal_si(fmpq_numref(x->u), 3));
    ADF_CHECK(adf_lball_get_center(c, x) == ADF_OK && fmpz_equal_si(fmpq_numref(c->q), 3) &&
              fmpz_equal_si(fmpq_denref(c->q), 4));
    ADF_CHECK(adf_lball_abs(c, x) == ADF_OK && fmpz_equal_si(fmpq_numref(c->q), 4) && fmpz_is_one(fmpq_denref(c->q)));
    adf_rat_clear(q); adf_rat_clear(c);
    adf_lball_clear(x);
}

/* the projection of a local finite ball and of a global one give the same lball */
ADF_TEST(set_fball_local_backend)
{
    ulong blocks[3] = {4, 9, 5};
    adf_modctx_struct * ctx = NULL;
    adf_fball_t g, l;
    adf_lball_t a, b;
    fmpz_t A, H, d;
    ulong ps[3] = {2, 3, 5}, primes_other[2] = {7, 11};
    int i;
    ADF_CHECK(adf_modctx_new_blocks(&ctx, blocks, 3) == ADF_OK);
    if (ctx == NULL)
        return;
    adf_fball_init(g); adf_fball_init(l);
    adf_lball_init(a); adf_lball_init(b);
    fmpz_init_set_si(A, 47); fmpz_init_set_si(H, 180); fmpz_init_set_si(d, 7);
    ADF_CHECK(adf_fball_set_fmpz3(g, A, H, d) == ADF_OK);
    ADF_CHECK(adf_fball_set_local(l, g, ctx) == ADF_OK && adf_fball_is_local(l));
    for (i = 0; i < 3; i++)
    {
        ADF_CHECK(adf_lball_set_fball(a, place_of(ps[i]), g) == ADF_OK);
        ADF_CHECK(adf_lball_set_fball(b, place_of(ps[i]), l) == ADF_OK);
        ADF_CHECK(adf_lball_identical(a, b) && !a->exact);
        ADF_CHECK(a->N == (i == 0 ? 2 : i == 1 ? 2 : 1));                       /* v_p(180) - v_p(7) */
    }
    for (i = 0; i < 2; i++)                                                       /* Zhat at 7: a ball of N = 0 */
    {
        ADF_CHECK(adf_lball_set_fball(a, place_of(primes_other[i]), g) == ADF_OK);
        ADF_CHECK(a->N == (primes_other[i] == 7 ? -1 : 0));
    }
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
    adf_fball_clear(g); adf_fball_clear(l);
    adf_lball_clear(a); adf_lball_clear(b);
    adf_modctx_free(ctx);
}

ADF_TEST(set_fball_exact_and_fractional)
{
    adf_fball_t f;
    adf_lball_t x;
    fmpz_t A, H, d;
    adf_lball_init(x);
    adf_fball_init(f);
    fmpz_init_set_si(A, 5); fmpz_init_set_si(H, 0); fmpz_init_set_si(d, 12);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    ADF_CHECK(adf_lball_set_fball(x, place_of(2), f) == ADF_OK && x->exact && x->v == -2 &&
              fmpz_equal_si(fmpq_numref(x->u), 5) && fmpz_equal_si(fmpq_denref(x->u), 3));
    ADF_CHECK(adf_lball_set_fball(x, place_of(3), f) == ADF_OK && x->exact && x->v == -1 &&
              fmpz_equal_si(fmpq_numref(x->u), 5) && fmpz_equal_si(fmpq_denref(x->u), 4));
    ADF_CHECK(adf_lball_set_fball(x, place_of(5), f) == ADF_OK && x->exact && x->v == 1);
    /* (1 + (3/2) Zhat)... H = 3, d = 2: N = 3/2, precision at 3 is 1, at 2 is -1 */
    fmpz_set_si(A, 1); fmpz_set_si(H, 3); fmpz_set_si(d, 2);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    ADF_CHECK(adf_lball_set_fball(x, place_of(3), f) == ADF_OK && !x->exact && x->N == 1);
    ADF_CHECK(adf_lball_set_fball(x, place_of(2), f) == ADF_OK && !x->exact && x->N == -1);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
    adf_fball_clear(f);
    adf_lball_clear(x);
}

ADF_TEST(the_example_of_the_brief)
{
    /* 1/3 at p = 5 to precision 10: unit part 1/3 mod 5^10 = 6510417 ( 3 * 6510417 = 19531251 = 1 + 5^10 * 2 ) ...
       the test computes the centre from the definition: u in (0, 5^10), 3 u = 1 mod 5^10 */
    adf_lball_t x, y, z;
    adf_rat_t q;
    fmpz_t p10, t;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z);
    adf_rat_init(q);
    fmpz_init(p10); fmpz_init(t);
    fmpz_set_ui(p10, 5);
    fmpz_pow_ui(p10, p10, 10);
    fmpq_set_si(q->q, 1, 3);
    ADF_CHECK(adf_lball_set_rat_ball(x, place_of(5), q, 10) == ADF_OK);
    fmpz_mul_ui(t, fmpq_numref(x->u), 3);
    fmpz_mod(t, t, p10);
    ADF_CHECK(fmpz_is_one(t) && fmpz_cmp(fmpq_numref(x->u), p10) < 0 && fmpz_sgn(fmpq_numref(x->u)) > 0);
    ADF_CHECK(adf_lball_mul(z, x, x) == ADF_OK && z->N == 10 && z->v == 0);
    fmpz_mul_ui(t, fmpq_numref(z->u), 9);
    fmpz_mod(t, t, p10);
    ADF_CHECK(fmpz_is_one(t));                                            /* (1/3)^2 = 1/9 */
    ADF_CHECK(adf_lball_inv(y, x) == ADF_OK && y->N == 10 && fmpz_equal_si(fmpq_numref(y->u), 3));
    fmpz_clear(p10); fmpz_clear(t);
    adf_rat_clear(q);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z);
}

/* ------------------------------------------------------------- the limits apply to the result, never to a temporary */

/* Findings F1 to F3 of docs/reviews/f1/review-lball.md; the rule of the header and api-1f.md L4a: ADF_LIMIT only if an
   input or the RESULT is outside the limits (|v|, |N| <= ADF_LBALL_EXP_MAX; a centre that needs p^k with
   k bits(p) > ADF_LBALL_BITS_MAX), never because of an intermediate value. E = 2^60 is the exponent bound and
   p = 5 throughout. The expected values are worked out by hand in the comment at each call. */

/* z was set to the sentinel before the call that returned st. want_st == ADF_OK: z must be the canonical value
   (p = 5, exact, un/ud, v, N); else z must be untouched. */
static void
expect_res(const char * name, int st, int want_st, const adf_lball_t z, int exact, slong un, ulong ud, slong v,
           slong N)
{
    adf_lball_t want, sen;
    adf_lball_init(want);
    adf_lball_init(sen);
    sentinel(sen);
    lb_fields_si(want, 5, exact, un, ud, v, N);
    fmpq_canonicalise(want->u);
    ADF_CHECK_MSG(st == want_st, "%s: status %s, want %s", name, adf_status_str(st), adf_status_str(want_st));
    if (want_st == ADF_OK)
    {
        ADF_CHECK_MSG(adf_lball_is_canonical(want), "%s: the expected value is not canonical (test error)", name);
        ADF_CHECK_MSG(st != ADF_OK || (adf_lball_is_canonical(z) && adf_lball_identical(z, want)),
                      "%s: result differs from the expected value", name);
    }
    else
        ADF_CHECK_MSG(adf_lball_identical(z, sen), "%s: output touched on a status", name);
    adf_lball_clear(want);
    adf_lball_clear(sen);
}

/* F1. The inverse of an exact value has no precision, so the guard N - 2v does not apply to it. */
ADF_TEST(inv_of_an_exact_power_at_the_exponent_limit)
{
    slong E = ADF_LBALL_EXP_MAX;
    adf_lball_t x, z;
    int sign;
    adf_lball_init(x); adf_lball_init(z);
    for (sign = -1; sign <= 1; sign += 2)
    {
        /* 1 / 5^(sign E) = 5^(-sign E), exact */
        lb_fields_si(x, 5, 1, 1, 1, sign * E, 0);
        ADF_CHECK(adf_lball_is_canonical(x));
        sentinel(z);
        expect_res("inv 5^(+-E)", adf_lball_inv(z, x), ADF_OK, z, 1, 1, 1, -sign * E, 0);
        /* 1 / (3/2 5^(sign E)) = 2/3 5^(-sign E) */
        lb_fields_si(x, 5, 1, 3, 2, sign * E, 0);
        fmpq_canonicalise(x->u);
        sentinel(z);
        expect_res("inv 3/2 5^(+-E)", adf_lball_inv(z, x), ADF_OK, z, 1, 2, 3, -sign * E, 0);
        /* aliased */
        ADF_CHECK(adf_lball_inv(x, x) == ADF_OK && x->v == -sign * E && fmpz_equal_si(fmpq_numref(x->u), 2));
    }
    /* a ball whose inverse has N - 2v beyond the bound is still LIMIT: the RESULT is outside.
       5^(-E) + O(5^(-E + 1)): the inverse has N' = -E + 1 + 2E = E + 1 */
    lb_fields_si(x, 5, 0, 1, 1, -E, -E + 1);
    ADF_CHECK(adf_lball_is_canonical(x));
    sentinel(z);
    expect_res("inv ball, N' = E + 1", adf_lball_inv(z, x), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    /* 3 + O(5^E): the inverse centre 1/3 mod 5^E needs 5^E, a centre of 2^60 bits: the RESULT is too big */
    lb_fields_si(x, 5, 0, 3, 1, 0, E);
    sentinel(z);
    expect_res("inv 3 + O(5^E)", adf_lball_inv(z, x), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    /* 1 + O(5^E): the inverse is 1 + O(5^E), small */
    lb_fields_si(x, 5, 0, 1, 1, 0, E);
    sentinel(z);
    expect_res("inv 1 + O(5^E)", adf_lball_inv(z, x), ADF_OK, z, 0, 1, 1, 0, E);
    adf_lball_clear(x); adf_lball_clear(z);
}

/* F2. x - y is computed with the sign inside, no canonical temporary of -y. */
ADF_TEST(sub_of_small_results_at_the_exponent_limit)
{
    slong E = ADF_LBALL_EXP_MAX;
    adf_lball_t x, y, z;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z);
    /* (1 + 5^E a) - (1 + 5^E b) = 5^E (a - b): O(5^E) around 0 */
    lb_fields_si(x, 5, 0, 1, 1, 0, E);
    sentinel(z);
    expect_res("sub x - x", adf_lball_sub(z, x, x), ADF_OK, z, 0, 0, 1, 0, E);
    /* Z_5 - (1 + 5^E Z_5) = Z_5 */
    lb_fields_si(y, 5, 0, 0, 1, 0, 0);
    sentinel(z);
    expect_res("sub Z_5 - fine", adf_lball_sub(z, y, x), ADF_OK, z, 0, 0, 1, 0, 0);
    /* (2 + O(5^E)) - (1 + O(5^E)) = 1 + O(5^E) */
    lb_fields_si(y, 5, 0, 2, 1, 0, E);
    sentinel(z);
    expect_res("sub 2 - 1", adf_lball_sub(z, y, x), ADF_OK, z, 0, 1, 1, 0, E);
    /* aliased outputs */
    adf_lball_set(z, x);
    ADF_CHECK(adf_lball_sub(z, z, z) == ADF_OK && fmpq_is_zero(z->u) && z->N == E && !z->exact);
    /* at the negative end: 5^(-E) + O(5^(1 - E)) minus itself is O(5^(1 - E)) around 0 */
    lb_fields_si(x, 5, 0, 1, 1, -E, -E + 1);
    sentinel(z);
    expect_res("sub at v = -E", adf_lball_sub(z, x, x), ADF_OK, z, 0, 0, 1, 0, -E + 1);
    /* 0 - (5^(-E) + O(5^(1-E))): the exact 0 minus the ball is the ball -c: u = -1 mod 5 = 4 */
    lb_fields_si(y, 5, 1, 0, 1, 0, 0);
    sentinel(z);
    expect_res("sub 0 - ball", adf_lball_sub(z, y, x), ADF_OK, z, 0, 4, 1, -E, -E + 1);
    /* the RESULT needs a power: 0 - (1 + O(5^E)) = -1 + O(5^E), centre 5^E - 1: LIMIT, as neg */
    lb_fields_si(x, 5, 0, 1, 1, 0, E);
    sentinel(z);
    expect_res("sub 0 - (1 + O(5^E)) needs 5^E", adf_lball_sub(z, y, x), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    sentinel(z);
    expect_res("neg (1 + O(5^E)) needs 5^E", adf_lball_neg(z, x), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z);
}

/* F3. x / y is computed from the valuations and precisions first (api-1f.md L4a). */
ADF_TEST(div_of_small_results_at_the_exponent_limit)
{
    slong E = ADF_LBALL_EXP_MAX;
    adf_lball_t x, y, z;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z);
    /* 0 / (3 + O(5^E)): every point of y is a unit, so the quotient is the exact 0 */
    lb_fields_si(y, 5, 0, 3, 1, 0, E);
    lb_fields_si(x, 5, 1, 0, 1, 0, 0);
    sentinel(z);
    expect_res("div 0 / fine unit", adf_lball_div(z, x, y), ADF_OK, z, 1, 0, 1, 0, 0);
    /* Z_5 / (3 + O(5^E)) = Z_5 (K = N - w = 0) */
    lb_fields_si(x, 5, 0, 0, 1, 0, 0);
    sentinel(z);
    expect_res("div Z_5 / fine unit", adf_lball_div(z, x, y), ADF_OK, z, 0, 0, 1, 0, 0);
    /* (5^(-E) + Z_5) / (5^(-E) + Z_5) = 1 + O(5^E): K = min(E, E, 2E) = E, centre 1 */
    lb_fields_si(x, 5, 0, 1, 1, -E, 0);
    ADF_CHECK(adf_lball_is_canonical(x));
    sentinel(z);
    expect_res("div x / x at v = -E", adf_lball_div(z, x, x), ADF_OK, z, 0, 1, 1, 0, E);
    /* aliased */
    adf_lball_set(z, x);
    ADF_CHECK(adf_lball_div(z, z, z) == ADF_OK && fmpq_is_one(z->u) && z->N == E && z->v == 0);
    /* O(5^(-E)) / (3 + O(5^E)) = O(5^(-E)): K = min(-w + N, N + M - 2w) = min(-E, 0) = -E */
    lb_fields_si(x, 5, 0, 0, 1, 0, -E);
    sentinel(z);
    expect_res("div O(5^-E) / fine unit", adf_lball_div(z, x, y), ADF_OK, z, 0, 0, 1, 0, -E);
    /* (2 + O(5^3)) / 5^E exact = 2 5^(-E) + O(5^(3 - E)): u = 2, v = -E, N = 3 - E */
    lb_fields_si(x, 5, 0, 2, 1, 0, 3);
    lb_fields_si(y, 5, 1, 1, 1, E, 0);
    sentinel(z);
    expect_res("div ball / 5^E", adf_lball_div(z, x, y), ADF_OK, z, 0, 2, 1, -E, 3 - E);
    /* (2 + O(5^3)) / 5^(-E) has N = E + 3: the RESULT is outside the bound */
    lb_fields_si(y, 5, 1, 1, 1, -E, 0);
    sentinel(z);
    expect_res("div ball / 5^-E, N = E + 3", adf_lball_div(z, x, y), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    /* exact 5^E / exact 5^E = exact 1; 5^E / 5^(-E) = 5^(2E), the result is outside */
    lb_fields_si(x, 5, 1, 1, 1, E, 0);
    lb_fields_si(y, 5, 1, 1, 1, E, 0);
    sentinel(z);
    expect_res("div 5^E / 5^E", adf_lball_div(z, x, y), ADF_OK, z, 1, 1, 1, 0, 0);
    lb_fields_si(y, 5, 1, 1, 1, -E, 0);
    sentinel(z);
    expect_res("div 5^E / 5^-E", adf_lball_div(z, x, y), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    /* 5^(-E) exact / (1 + O(5^E)) = 5^(-E) + O(5^0): K = v + M = 0 */
    lb_fields_si(x, 5, 1, 1, 1, -E, 0);
    lb_fields_si(y, 5, 0, 1, 1, 0, E);
    sentinel(z);
    expect_res("div 5^-E / (1 + O(5^E))", adf_lball_div(z, x, y), ADF_OK, z, 0, 1, 1, -E, 0);
    /* 5^E exact / (1 + O(5^E)): K = E + E = 2E, outside */
    lb_fields_si(x, 5, 1, 1, 1, E, 0);
    sentinel(z);
    expect_res("div 5^E / (1 + O(5^E)), N = 2E", adf_lball_div(z, x, y), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    /* the RESULT needs a centre 1/3 mod 5^E: (1 + O(5^E)) / (3 + O(5^E)) is LIMIT, rightly */
    lb_fields_si(x, 5, 0, 1, 1, 0, E);
    lb_fields_si(y, 5, 0, 3, 1, 0, E);
    sentinel(z);
    expect_res("div (1 + O(5^E)) / (3 + O(5^E)) needs 5^E", adf_lball_div(z, x, y), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    /* the statuses of a divisor without inverse come first, as before */
    lb_fields_si(y, 5, 1, 0, 1, 0, 0);
    sentinel(z);
    expect_res("div by exact 0", adf_lball_div(z, x, y), ADF_NOT_UNIT, z, 0, 0, 1, 0, 0);
    lb_fields_si(y, 5, 0, 0, 1, 0, E);
    sentinel(z);
    expect_res("div by O(5^E)", adf_lball_div(z, x, y), ADF_UNIT_NOT_CERTIFIED, z, 0, 0, 1, 0, 0);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z);
}

/* add, mul, neg with inputs at the limits of both signs: the same rule (F1 to F3, "look for the same defect"). */
ADF_TEST(add_mul_neg_at_the_exponent_limits)
{
    slong E = ADF_LBALL_EXP_MAX;
    adf_lball_t x, y, z;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z);
    /* add: the operand 5^E of valuation >= K is dropped: (1 + O(5^E)) + 5^E = 1 + O(5^E) */
    lb_fields_si(x, 5, 0, 1, 1, 0, E);
    lb_fields_si(y, 5, 1, 1, 1, E, 0);
    sentinel(z);
    expect_res("add ball + 5^E", adf_lball_add(z, x, y), ADF_OK, z, 0, 1, 1, 0, E);
    /* (5^(-E) + O(5^(1-E))) + 5^E: 5^E is dropped */
    lb_fields_si(x, 5, 0, 1, 1, -E, -E + 1);
    sentinel(z);
    expect_res("add ball(-E) + 5^E", adf_lball_add(z, x, y), ADF_OK, z, 0, 1, 1, -E, -E + 1);
    /* exact 5^E + exact 5^(-E) = 5^(-E) (1 + 5^(2E)): the RESULT needs 5^(2E): LIMIT */
    lb_fields_si(x, 5, 1, 1, 1, -E, 0);
    sentinel(z);
    expect_res("add 5^-E + 5^E", adf_lball_add(z, x, y), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    /* exact 5^E + (-5^E) = exact 0 */
    lb_fields_si(x, 5, 1, -1, 1, E, 0);
    sentinel(z);
    expect_res("add 5^E - 5^E", adf_lball_add(z, x, y), ADF_OK, z, 1, 0, 1, 0, 0);
    sentinel(z);
    expect_res("sub 5^E - 5^E", adf_lball_sub(z, y, y), ADF_OK, z, 1, 0, 1, 0, 0);
    /* neg of an exact value at both ends is exact and small */
    lb_fields_si(x, 5, 1, 1, 1, -E, 0);
    sentinel(z);
    expect_res("neg 5^-E", adf_lball_neg(z, x), ADF_OK, z, 1, -1, 1, -E, 0);
    lb_fields_si(x, 5, 1, 1, 1, E, 0);
    sentinel(z);
    expect_res("neg 5^E", adf_lball_neg(z, x), ADF_OK, z, 1, -1, 1, E, 0);
    /* neg of a ball around 0 and of a ball at v = -E */
    lb_fields_si(x, 5, 0, 0, 1, 0, E);
    sentinel(z);
    expect_res("neg O(5^E)", adf_lball_neg(z, x), ADF_OK, z, 0, 0, 1, 0, E);
    lb_fields_si(x, 5, 0, 1, 1, -E, -E + 1);
    sentinel(z);
    expect_res("neg ball(-E)", adf_lball_neg(z, x), ADF_OK, z, 0, 4, 1, -E, -E + 1);
    /* mul: 5^E * 5^(-E) = 1; (1 + O(5^E)) * 5^(-E) = 5^(-E) + O(5^0); 5^E * 5^E is outside */
    lb_fields_si(x, 5, 1, 1, 1, E, 0);
    lb_fields_si(y, 5, 1, 1, 1, -E, 0);
    sentinel(z);
    expect_res("mul 5^E 5^-E", adf_lball_mul(z, x, y), ADF_OK, z, 1, 1, 1, 0, 0);
    sentinel(z);
    expect_res("mul 5^E 5^E", adf_lball_mul(z, x, x), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    sentinel(z);
    expect_res("mul 5^-E 5^-E", adf_lball_mul(z, y, y), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    lb_fields_si(x, 5, 0, 1, 1, 0, E);
    sentinel(z);
    expect_res("mul (1 + O(5^E)) 5^-E", adf_lball_mul(z, x, y), ADF_OK, z, 0, 1, 1, -E, 0);
    /* (5^(-E) + O(5^0)) squared: K = -E, centre valuation -2E, so the result is outside */
    lb_fields_si(x, 5, 0, 1, 1, -E, 0);
    sentinel(z);
    expect_res("mul ball(-E) ball(-E)", adf_lball_mul(z, x, x), ADF_LIMIT, z, 0, 0, 1, 0, 0);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z);
}

/* sub and div at mixed valuations agree with the composition they replace, wherever the composition works, and
   have its status where the divisor has no inverse. */
static void
composition_at_prime(ulong p, slong vmax, slong kmax)
{
    universe_t U = {NULL, 0, 0};
    size_t i, j, pairs = 0;
    uni_build(&U, p, vmax, kmax);
    for (i = 0; i < U.n; i++)
        for (j = 0; j < U.n; j++)
        {
            adf_lball_t m, r1, r2, d, a;
            int s1, s2;
            adf_lball_init(m); adf_lball_init(r1); adf_lball_init(r2); adf_lball_init(d); adf_lball_init(a);
            /* sub = add of the negation */
            ADF_CHECK(adf_lball_neg(m, U.v + j) == ADF_OK);
            s1 = adf_lball_add(r1, U.v + i, m);
            s2 = adf_lball_sub(r2, U.v + i, U.v + j);
            ADF_CHECK(s1 == s2 && (s1 != ADF_OK || adf_lball_identical(r1, r2)));
            /* div = mul by the inverse, for a divisor that is invertible */
            s2 = adf_lball_div(d, U.v + i, U.v + j);
            s1 = adf_lball_inv(m, U.v + j);
            if (s1 == ADF_OK)
            {
                s1 = adf_lball_mul(a, U.v + i, m);
                ADF_CHECK(s1 == ADF_OK && s2 == ADF_OK && adf_lball_identical(a, d));
            }
            else
                ADF_CHECK(s2 == s1);
            pairs++;
            adf_lball_clear(m); adf_lball_clear(r1); adf_lball_clear(r2); adf_lball_clear(d); adf_lball_clear(a);
        }
    printf("   composition p=%lu: %lu pairs\n", p, (unsigned long) pairs);
    uni_free(&U);
}

ADF_TEST(sub_and_div_agree_with_their_compositions)
{
    composition_at_prime(2, 2, 3);
    composition_at_prime(3, 2, 2);
    composition_at_prime(5, 1, 2);
}

#ifdef ADF_CHECK_INVARIANTS
/* ------------------------------------------------------------------------------------ the entry check (F4) */

/* conventions 4.4: with -DADF_CHECK_INVARIANTS every public function checks the predicate of each input on entry and
   calls flint_abort. The nine paths that had no check (finding F4): place, is_exact, contains_zero, get_prec, set,
   swap, identical (each argument), set_rat, set_rat_ball. Exempt: init (reads nothing), clear (must release a value
   whose fields were forged), is_canonical (the predicate never aborts, M1-D2), the layout queries (no argument),
   the OUTPUT argument of set and of every arithmetic function (it is overwritten). set_fball reads f through
   adf_fball_get_fmpz3, which checks f. Each case runs in a child process: it must end by SIGABRT with a line on
   stderr that names the function and the type; the canonical control must return normally and write nothing. */

enum { IC_PLACE, IC_EXACT, IC_ZERO, IC_PREC, IC_SET, IC_SWAP_X, IC_SWAP_Y, IC_IDENT_X, IC_IDENT_Y, IC_SET_RAT,
       IC_SET_RAT_BALL, IC_ADD, IC_SET_OUT, IC_COUNT };

static const char * const ic_name[IC_COUNT] = {"adf_lball_place", "adf_lball_is_exact", "adf_lball_contains_zero",
    "adf_lball_get_prec", "adf_lball_set", "adf_lball_swap", "adf_lball_swap", "adf_lball_identical",
    "adf_lball_identical", "adf_lball_set_rat", "adf_lball_set_rat_ball", "adf_lball_add", "adf_lball_set"};

/* forged = 1: the argument named by the case is not canonical (a composite p, or the fraction 2/4). IC_SET_OUT:
   the OUTPUT of set is forged and the call must return normally. */
static void
ic_child(int which, int forged)
{
    adf_lball_t x, y;
    adf_rat_t q;
    adf_place_t v = place_of(5);
    slong N = 0;
    adf_lball_init(x); adf_lball_init(y); adf_rat_init(q);
    lb_fields_si(x, 5, 0, 7, 1, 0, 2);                     /* 7 + 25 Z_5, canonical */
    lb_fields_si(y, 5, 0, 7, 1, 0, 2);
    fmpz_set_si(fmpq_numref(q->q), forged ? 2 : 1);        /* 2/4 is not canonical, 1/4 is */
    fmpz_set_si(fmpq_denref(q->q), 4);
    if (forged && (which <= IC_SET || which == IC_SWAP_X || which == IC_IDENT_X || which == IC_ADD))
        x->p = 4;
    if (forged && (which == IC_SWAP_Y || which == IC_IDENT_Y))
        y->p = 4;
    if (which == IC_SET_OUT)
        y->p = 4;                                          /* forged output: not read, no check */
    switch (which)
    {
    case IC_PLACE: (void) adf_lball_place(x); break;
    case IC_EXACT: (void) adf_lball_is_exact(x); break;
    case IC_ZERO: (void) adf_lball_contains_zero(x); break;
    case IC_PREC: (void) adf_lball_get_prec(&N, x); break;
    case IC_SET:
    case IC_SET_OUT: adf_lball_set(y, x); break;
    case IC_SWAP_X:
    case IC_SWAP_Y: adf_lball_swap(x, y); break;
    case IC_IDENT_X:
    case IC_IDENT_Y: (void) adf_lball_identical(x, y); break;
    case IC_SET_RAT: (void) adf_lball_set_rat(y, v, q); break;
    case IC_SET_RAT_BALL: (void) adf_lball_set_rat_ball(y, v, q, 3); break;
    case IC_ADD: (void) adf_lball_add(y, x, x); break;
    default: break;
    }
    x->p = 5;
    y->p = 5;
    adf_lball_clear(x); adf_lball_clear(y); adf_rat_clear(q);
    flint_cleanup();
}

/* Runs the child; returns the signal (0 if it exited) and copies stderr to err. */
static int
ic_run(int which, int forged, int * exit_code, char * err, size_t cap)
{
    int fd[2], st = 0;
    size_t n = 0;
    pid_t pid;
    struct rlimit nocore = {0, 0};
    fflush(stdout);
    fflush(stderr);
    if (pipe(fd) != 0)
        abort();
    pid = fork();
    if (pid < 0)
        abort();
    if (pid == 0)
    {
        setrlimit(RLIMIT_CORE, &nocore);
#ifdef __linux__
        prctl(PR_SET_DUMPABLE, 0);
#endif
        close(fd[0]);
        dup2(fd[1], 2);
        close(fd[1]);
        ic_child(which, forged);
        _exit(0);
    }
    close(fd[1]);
    while (n < cap - 1)
    {
        ssize_t r = read(fd[0], err + n, cap - 1 - n);
        if (r <= 0)
            break;
        n += (size_t) r;
    }
    err[n] = 0;
    {
        char junk[256];
        while (read(fd[0], junk, sizeof junk) > 0)
            ;
    }
    close(fd[0]);
    if (waitpid(pid, &st, 0) != pid)
        abort();
    *exit_code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
    return WIFSIGNALED(st) ? WTERMSIG(st) : 0;
}

ADF_TEST(entry_check_of_every_public_function)
{
    int i, code;
    char err[512];
    for (i = 0; i < IC_COUNT; i++)
    {
        int sig;
        /* the canonical control (and, for IC_SET_OUT, the forged output) returns normally and silently */
        sig = ic_run(i, 0, &code, err, sizeof err);
        ADF_CHECK_MSG(sig == 0 && code == 0 && err[0] == 0, "%s (case %d): control: signal %d, exit %d, stderr '%s'",
                      ic_name[i], i, sig, code, err);
        if (i == IC_SET_OUT)
            continue;
        sig = ic_run(i, 1, &code, err, sizeof err);
        ADF_CHECK_MSG(sig == SIGABRT, "%s (case %d): a forged argument did not abort (signal %d, exit %d)", ic_name[i],
                      i, sig, code);
        ADF_CHECK_MSG(strstr(err, "ADF_CHECK_INVARIANTS") != NULL && strstr(err, ic_name[i]) != NULL &&
                      strstr(err, i >= IC_SET_RAT && i <= IC_SET_RAT_BALL ? "adf_rat" : "adf_lball") != NULL,
                      "%s (case %d): stderr '%s'", ic_name[i], i, err);
    }
}
#endif /* ADF_CHECK_INVARIANTS */

/* ---------------------------------------------------------------------- set_fball, LIMIT (lane f-slice3) */

/* Finding 1 of lanes/f-slice2/result.md: set_fball at p = 3 of the adele A = H - 1, H = 6^(2^25 + 1) (87 million bits)
   returned ADF_LIMIT only after fmpz_remove had formed v_3(H) (5 to 6 s here, 13 to 16 s in the report). The answer is
   LIMIT: k = v_3(H) - v_3(A) = 2^25 + 1, 3^k has 2^26 + 2 bits, more than ADF_LBALL_BITS_MAX, and the centre A is a
   unit of 87 million bits, far above 3^k: no small-centre shortcut. The test fails on the old code by time. The
   bound is 3.5 s (the new code needs 1 to 2 s: a divisibility test of H by a power of 3 of 2^26 bits). */
#include <time.h>

static double
now_s(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double) t.tv_sec + 1e-9 * (double) t.tv_nsec;
}

ADF_TEST(set_fball_limit_is_decided_without_the_full_valuation)
{
    adf_fball_t f;
    adf_lball_t x, s;
    fmpz_t A, H, d;
    double t0;
    int st;
    adf_fball_init(f);
    adf_lball_init(x);
    adf_lball_init(s);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_set_ui(H, 6);
    fmpz_pow_ui(H, H, (1ul << 25) + 1);
    fmpz_sub_ui(A, H, 1);
    fmpz_one(d);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    sentinel(x);
    sentinel(s);
    t0 = now_s();
    st = adf_lball_set_fball(x, place_of(3), f);
    t0 = now_s() - t0;
    ADF_CHECK_MSG(st == ADF_LIMIT && adf_lball_identical(x, s), "p = 3: status %s", adf_status_str(st));
    ADF_CHECK_MSG(t0 < 3.5, "p = 3: LIMIT took %.1f s (bound 3.5 s)", t0);
    /* p = 2 and p = 5 are as before: LIMIT at 2 (2^k, k = 2^25 + 1, has 2^26 + 2 bits), OK at 5 (the ball around 0) */
    t0 = now_s();
    ADF_CHECK(adf_lball_set_fball(x, place_of(2), f) == ADF_LIMIT && adf_lball_identical(x, s));
    ADF_CHECK(adf_lball_set_fball(x, place_of(5), f) == ADF_OK && !x->exact && x->N == 0 && fmpq_is_zero(x->u));
    t0 = now_s() - t0;
    ADF_CHECK_MSG(t0 < 3.5, "p = 2 and 5 took %.1f s (bound 3.5 s)", t0);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(f);
    adf_lball_clear(x);
    adf_lball_clear(s);
}

/* The two cases of the early decision that must NOT be LIMIT or must be: H = 2^(2^25 + 5) has v_2(H) above the bound
   kmax = 2^25 of p = 2. With A = 1, d = 1 the centre 1 is a small integer: the result 1 + O(2^(2^25 + 5)) is fine
   (the shortcut of L0; no power of 2 is formed). With d = 3 the centre is 1/3 modulo 2^k, k = 2^25 + 5: LIMIT. */
ADF_TEST(set_fball_small_centre_above_the_bound_is_ok)
{
    adf_fball_t f;
    adf_lball_t x, s;
    fmpz_t A, H, d;
    slong e = (1L << 25) + 5;
    adf_fball_init(f);
    adf_lball_init(x);
    adf_lball_init(s);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_one(H);
    fmpz_mul_2exp(H, H, (ulong) e);
    fmpz_set_ui(A, 3);
    fmpz_one(d);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    ADF_CHECK(adf_lball_set_fball(x, place_of(2), f) == ADF_OK && !x->exact && x->N == e && x->v == 0 &&
              fmpz_equal_si(fmpq_numref(x->u), 3));
    fmpz_set_ui(A, 1);
    fmpz_set_ui(d, 3);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    sentinel(x);
    sentinel(s);
    ADF_CHECK(adf_lball_set_fball(x, place_of(2), f) == ADF_LIMIT && adf_lball_identical(x, s));
    /* at p = 3 the adele (1 + H Zhat)/3 has v_3(H) = 0, v_3(d) = 1, so N = -1, and the centre 1/3 has valuation -1 >= N:
       the ball around 0 of exponent -1 */
    ADF_CHECK(adf_lball_set_fball(x, place_of(3), f) == ADF_OK && !x->exact && x->N == -1 && fmpq_is_zero(x->u));
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(f);
    adf_lball_clear(x);
    adf_lball_clear(s);
}
