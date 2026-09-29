/* tests/test_sball.c: adf_sball, partial balls over a finite set of places (lane f-slice2; include/adelefeld/sball.h;
   docs/api-1f.md statements S1 to S4).

   Oracles.
   1. The vectors tests/ref/vectors/f-slice2/sball_*.jsonl, made by lanes/f-slice2/gen_vectors.py from the sections
      f-slice1 and f-slice2 of proto/functions_checks.py: the projection of an adele to a place list (any order,
      repetition), the componentwise operations (the local components by the reference of L2 to L5, the real component
      by the exact set of results of interval arithmetic on rational end points), the set predicates. Every line of
      every file is run. The expected partial balls are built by writing the fields of the struct, not by the
      constructor under test.
   2. Enumeration written in this file, independent of the reference: the projection of (A + H Zhat)/d to a prime is a
      ball that contains every point A/d + H z/d (z = 0 .. p^2 - 1) and is the smallest such (the p classes modulo the
      next digit are all met); and, for the operations, random TUPLES of points, one in each operand at each place
      (real end points, local points c + p^N a): the tuple of results lies in the result component at every place.
      That is the test that pairs the right components of the two operands.
   3. Forged values for is_canonical, statuses with the state of the outputs and the reported place, limits (the
      LIMIT of a component at the first failing prime), aliasing, the example of the brief.

   What would make a case fail is stated at each test. */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include <flint/mag.h>
#include "support/jsonl.h"
#include "test_runner.h"

/* ------------------------------------------------------------------------------------------------- helpers */

static adf_place_t
place_of(ulong p)
{
    adf_place_t v;
    int st = adf_place_prime(&v, p);
    ADF_CHECK(st == ADF_OK);
    return v;
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
member_int(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    const jsonl_value * v = member(rec, key);
    const char * t = v ? jsonl_int_text(v, &err) : NULL;
    ADF_CHECK(t != NULL);
    return t ? t : "0";
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

static slong
member_slong(const jsonl_value * rec, const char * key)
{
    return strtol(member_int(rec, key), NULL, 10);
}

static void
fmpz_from_text(fmpz_t z, const char * t)
{
    ADF_CHECK(fmpz_set_str(z, t, 10) == 0);
}

/* A place named in a vector: the string "inf" or a prime. */
static adf_place_t
place_from_json(const jsonl_value * v)
{
    jsonl_error_t err;
    if (jsonl_is(v, JSONL_STR))
    {
        size_t len;
        const char * s = jsonl_string(v, &len, &err);
        ADF_CHECK(s != NULL && strcmp(s, "inf") == 0);
        return adf_place_inf();
    }
    return place_of(strtoul(jsonl_int_text(v, &err), NULL, 10));
}

/* x = {"m", "e", "rm", "re"}: the ball m 2^e +- rm 2^re (rm < 2^30, so the radius is exact). */
static void
rb_from_json(arb_t x, const jsonl_value * o)
{
    fmpz_t m, e;
    fmpz_init(m);
    fmpz_init(e);
    fmpz_from_text(m, member_int(o, "m"));
    fmpz_from_text(e, member_int(o, "e"));
    arb_set_fmpz_2exp(x, m, e);
    mag_set_ui_2exp_si(arb_radref(x), strtoul(member_int(o, "rm"), NULL, 10), member_slong(o, "re"));
    fmpz_clear(m);
    fmpz_clear(e);
}

static void
dy_from_json(arf_t x, const jsonl_value * o)
{
    fmpz_t m, e;
    fmpz_init(m);
    fmpz_init(e);
    fmpz_from_text(m, member_int(o, "m"));
    fmpz_from_text(e, member_int(o, "e"));
    arf_set_fmpz_2exp(x, m, e);
    fmpz_clear(m);
    fmpz_clear(e);
}

/* q = m 2^e as an fmpq. */
static void
dy_to_fmpq(fmpq_t q, const fmpz_t m, slong e)
{
    fmpz_set(fmpq_numref(q), m);
    fmpz_one(fmpq_denref(q));
    if (e >= 0)
        fmpz_mul_2exp(fmpq_numref(q), fmpq_numref(q), (ulong) e);
    else
        fmpz_mul_2exp(fmpq_denref(q), fmpq_denref(q), (ulong) -e);
    fmpq_canonicalise(q);
}

/* The lball fields written directly. */
static void
lb_fields(adf_lball_struct * x, ulong p, int exact, const fmpz_t un, const fmpz_t ud, slong v, slong N)
{
    x->p = p;
    fmpz_set(fmpq_numref(x->u), un);
    fmpz_set(fmpq_denref(x->u), ud);
    x->v = v;
    x->N = N;
    x->exact = exact;
}

static void
lb_fields_si(adf_lball_struct * x, ulong p, int exact, slong un, ulong ud, slong v, slong N)
{
    x->p = p;
    fmpz_set_si(fmpq_numref(x->u), un);
    fmpz_set_ui(fmpq_denref(x->u), ud);
    x->v = v;
    x->N = N;
    x->exact = exact;
}

static void
lb_from_json(adf_lball_struct * x, const jsonl_value * o)
{
    fmpz_t un, ud;
    fmpz_init(un);
    fmpz_init(ud);
    fmpz_from_text(un, member_int(o, "un"));
    fmpz_from_text(ud, member_int(o, "ud"));
    lb_fields(x, strtoul(member_int(o, "p"), NULL, 10), (int) member_slong(o, "exact"), un, ud,
              member_slong(o, "v"), member_slong(o, "N"));
    fmpz_clear(un);
    fmpz_clear(ud);
}

/* s = the value with the given fields, written directly into the struct (the loc array is allocated with
   flint_malloc, as adf_sball_clear expects). r may be NULL (then the imaginary part is 0 and so is the real one). */
static void
sb_poke(adf_sball_t s, int arch, const arb_t r, const adf_lball_struct * loc, slong n)
{
    slong i;
    adf_sball_clear(s);
    adf_sball_init(s);
    s->arch = arch;
    acb_zero(s->inf);
    if (r != NULL)
        arb_set(acb_realref(s->inf), r);
    s->len = n;
    s->loc = n > 0 ? (adf_lball_struct *) flint_malloc((size_t) n * sizeof(adf_lball_struct)) : NULL;
    for (i = 0; i < n; i++)
    {
        adf_lball_init(&s->loc[i]);
        adf_lball_set(&s->loc[i], &loc[i]);
    }
}

/* s = a partial ball from a record {"arch", "inf", "loc"}. */
static void
sb_from_json(adf_sball_t s, const jsonl_value * o)
{
    jsonl_error_t err;
    const jsonl_value * inf = member(o, "inf");
    const jsonl_value * loc = member(o, "loc");
    size_t i, n = loc ? jsonl_size(loc) : 0;
    adf_lball_struct * arr = (adf_lball_struct *) flint_malloc((n ? n : 1) * sizeof(adf_lball_struct));
    arb_t r;
    arb_init(r);
    for (i = 0; i < n; i++)
    {
        adf_lball_init(&arr[i]);
        lb_from_json(&arr[i], jsonl_at(loc, i, &err));
    }
    if (inf != NULL && !jsonl_is_null(inf, NULL))
        rb_from_json(r, inf);
    sb_poke(s, (int) member_slong(o, "arch"), r, arr, (slong) n);
    for (i = 0; i < n; i++)
        adf_lball_clear(&arr[i]);
    flint_free(arr);
    arb_clear(r);
}

static void
lb_exact_of(adf_lball_struct * x, ulong p, slong num, slong den)
{
    adf_rat_t q;
    fmpz_t n, d;
    int st;
    adf_rat_init(q);
    fmpz_init_set_si(n, num);
    fmpz_init_set_si(d, den);
    ADF_CHECK(adf_rat_set_fmpz2(q, n, d) == ADF_OK);
    st = adf_lball_set_rat(x, place_of(p), q);
    ADF_CHECK(st == ADF_OK);
    fmpz_clear(n);
    fmpz_clear(d);
    adf_rat_clear(q);
}

static void
lb_ball_of(adf_lball_struct * x, ulong p, slong num, slong den, slong N)
{
    adf_rat_t q;
    fmpz_t n, d;
    int st;
    adf_rat_init(q);
    fmpz_init_set_si(n, num);
    fmpz_init_set_si(d, den);
    ADF_CHECK(adf_rat_set_fmpz2(q, n, d) == ADF_OK);
    st = adf_lball_set_rat_ball(x, place_of(p), q, N);
    ADF_CHECK(st == ADF_OK);
    fmpz_clear(n);
    fmpz_clear(d);
    adf_rat_clear(q);
}

/* A recognisable canonical value that a function must leave alone on a status: real 12345, places {3, 7}. */
static void
sb_sentinel(adf_sball_t s)
{
    adf_lball_struct loc[2];
    arb_t r;
    adf_lball_init(&loc[0]);
    adf_lball_init(&loc[1]);
    arb_init(r);
    lb_exact_of(&loc[0], 3, 1, 2);
    lb_ball_of(&loc[1], 7, 5, 1, 4);
    arb_set_si(r, 12345);
    sb_poke(s, ADF_ARCH_REAL, r, loc, 2);
    adf_lball_clear(&loc[0]);
    adf_lball_clear(&loc[1]);
    arb_clear(r);
}

typedef void (*rec_fn)(const jsonl_value *);

static void
run_vectors(const char * name, rec_fn body, size_t min_lines)
{
    char path[256];
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;
    snprintf(path, sizeof path, "tests/ref/vectors/f-slice2/%s", name);
    ADF_CHECK_MSG(jsonl_open(path, &f, &err) == 1, "%s", jsonl_error_message(&err));
    if (f == NULL)
        return;
    ADF_CHECK_MSG(jsonl_count(f) >= min_lines, "%s has %lu lines", name, (unsigned long) jsonl_count(f));
    for (i = 0; i < jsonl_count(f); i++)
        body(jsonl_record(f, i));
    jsonl_close(f);
}

/* ---------------------------------------------------------------------------------------- life cycle, layout */

/* The layout of conventions 5.9, 12.4 (offsets are the contract of a binding). A moved field fails here. */
ADF_TEST(layout_and_init)
{
    adf_sball_t x;
    ADF_CHECK(adf_sizeof_sball() == 120 && adf_alignof_sball() == 8);
    ADF_CHECK(sizeof(adf_sball_struct) == 120 && offsetof(adf_sball_struct, arch) == 0 &&
              offsetof(adf_sball_struct, inf) == 8 && offsetof(adf_sball_struct, len) == 104 &&
              offsetof(adf_sball_struct, loc) == 112);
    ADF_CHECK(sizeof(acb_struct) == 96 && adf_sizeof_lball() == 48);
    ADF_CHECK(ADF_ARCH_NONE == 0 && ADF_ARCH_REAL == 1 && ADF_ARCH_COMPLEX == 2);
    adf_sball_init(x);
    ADF_CHECK(x->arch == ADF_ARCH_NONE && x->len == 0 && x->loc == NULL && acb_is_zero(x->inf));
    ADF_CHECK(adf_sball_is_canonical(x) && adf_sball_num_places(x) == 0 && adf_sball_arch(x) == ADF_ARCH_NONE);
    adf_sball_clear(x);
}

ADF_TEST(set_swap_identical)
{
    adf_sball_t x, y, z, s;
    adf_lball_struct loc[3];
    arb_t r;
    int i;
    for (i = 0; i < 3; i++)
        adf_lball_init(&loc[i]);
    arb_init(r);
    lb_exact_of(&loc[0], 2, 3, 5);
    lb_ball_of(&loc[1], 5, 7, 3, 6);
    lb_ball_of(&loc[2], 13, 0, 1, -2);
    arb_set_si(r, -7);
    mag_set_ui(arb_radref(r), 3);
    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(z);
    adf_sball_init(s);
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);
    ADF_CHECK(adf_sball_is_canonical(x));
    adf_sball_set(y, x);
    ADF_CHECK(adf_sball_identical(x, y) && adf_sball_is_canonical(y) && y->loc != x->loc);
    adf_sball_set(y, y);
    ADF_CHECK(adf_sball_identical(x, y));
    /* set over a value with other places releases the old array */
    sb_sentinel(z);
    adf_sball_set(z, x);
    ADF_CHECK(adf_sball_identical(z, x));
    adf_sball_set(z, s);   /* the empty value */
    ADF_CHECK(adf_sball_identical(z, s) && z->len == 0 && z->arch == ADF_ARCH_NONE);
    /* swap */
    sb_sentinel(z);
    adf_sball_set(y, x);
    adf_sball_swap(y, z);
    ADF_CHECK(adf_sball_identical(z, x) && !adf_sball_identical(y, x) && y->len == 2);
    adf_sball_swap(y, y);
    ADF_CHECK(y->len == 2);
    /* identical: each field */
    sb_poke(y, ADF_ARCH_REAL, r, loc, 3);
    ADF_CHECK(adf_sball_identical(x, y));
    y->arch = ADF_ARCH_COMPLEX;
    ADF_CHECK(!adf_sball_identical(x, y));
    y->arch = ADF_ARCH_REAL;
    arb_add_ui(acb_realref(y->inf), acb_realref(y->inf), 1, 100);
    ADF_CHECK(!adf_sball_identical(x, y));
    sb_poke(y, ADF_ARCH_REAL, r, loc, 3);
    mag_set_ui(arb_radref(acb_realref(y->inf)), 4);
    ADF_CHECK(!adf_sball_identical(x, y));
    sb_poke(y, ADF_ARCH_REAL, r, loc, 3);
    y->len = 2;
    ADF_CHECK(!adf_sball_identical(x, y));
    y->len = 3;
    y->loc[1].N = 7;
    ADF_CHECK(!adf_sball_identical(x, y));
    sb_poke(y, ADF_ARCH_REAL, r, loc, 3);
    y->loc[2].p = 17;
    ADF_CHECK(!adf_sball_identical(x, y));
    sb_poke(y, ADF_ARCH_REAL, r, loc, 3);
    fmpz_set_si(fmpq_numref(y->loc[0].u), 1);
    ADF_CHECK(!adf_sball_identical(x, y));
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(z);
    adf_sball_clear(s);
    arb_clear(r);
    for (i = 0; i < 3; i++)
        adf_lball_clear(&loc[i]);
}

/* ------------------------------------------------------------------------------------------- is_canonical */

/* One clause of the predicate at a time is forged; each must give 0, and the good values 1. A predicate that misses
   a clause fails on its forged value. The array of a forged value is made by hand and restored before the clear. */
ADF_TEST(is_canonical_rejects_each_clause)
{
    adf_sball_t x;
    adf_lball_struct loc[3];
    arb_t r;
    adf_lball_struct * saved;
    int i;
    for (i = 0; i < 3; i++)
        adf_lball_init(&loc[i]);
    arb_init(r);
    lb_exact_of(&loc[0], 2, 1, 3);
    lb_ball_of(&loc[1], 5, 7, 1, 4);
    lb_exact_of(&loc[2], 11, 2, 7);
    arb_set_si(r, 5);
    adf_sball_init(x);

    /* good values */
    ADF_CHECK(adf_sball_is_canonical(x));
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);
    ADF_CHECK(adf_sball_is_canonical(x));
    sb_poke(x, ADF_ARCH_NONE, NULL, loc, 3);
    ADF_CHECK(adf_sball_is_canonical(x));
    sb_poke(x, ADF_ARCH_REAL, r, NULL, 0);
    ADF_CHECK(adf_sball_is_canonical(x));
    sb_poke(x, ADF_ARCH_COMPLEX, r, loc, 1);
    arb_set_si(acb_imagref(x->inf), -4);
    mag_set_ui(arb_radref(acb_imagref(x->inf)), 1);
    ADF_CHECK(adf_sball_is_canonical(x));   /* a complex ball may have any imaginary part */
    mag_set_ui(arb_radref(acb_realref(x->inf)), 2);
    ADF_CHECK(adf_sball_is_canonical(x));

    /* arch out of range */
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);
    x->arch = 3;
    ADF_CHECK(!adf_sball_is_canonical(x));
    x->arch = -1;
    ADF_CHECK(!adf_sball_is_canonical(x));
    x->arch = INT_MAX;
    ADF_CHECK(!adf_sball_is_canonical(x));
    /* arch = REAL with a non-zero imaginary part (midpoint, or radius: not the exact 0) */
    x->arch = ADF_ARCH_REAL;
    arb_set_si(acb_imagref(x->inf), 1);
    ADF_CHECK(!adf_sball_is_canonical(x));
    arb_zero(acb_imagref(x->inf));
    mag_set_ui_2exp_si(arb_radref(acb_imagref(x->inf)), 1, -1000);
    ADF_CHECK(!adf_sball_is_canonical(x));
    mag_zero(arb_radref(acb_imagref(x->inf)));
    ADF_CHECK(adf_sball_is_canonical(x));
    /* arch = NONE with inf not the exact 0 (real part, real radius, imaginary part) */
    x->arch = ADF_ARCH_NONE;
    ADF_CHECK(!adf_sball_is_canonical(x));   /* real part is 5 */
    arb_zero(acb_realref(x->inf));
    ADF_CHECK(adf_sball_is_canonical(x));
    mag_set_ui(arb_radref(acb_realref(x->inf)), 1);
    ADF_CHECK(!adf_sball_is_canonical(x));
    mag_zero(arb_radref(acb_realref(x->inf)));
    arb_set_si(acb_imagref(x->inf), 1);
    ADF_CHECK(!adf_sball_is_canonical(x));
    arb_zero(acb_imagref(x->inf));
    ADF_CHECK(adf_sball_is_canonical(x));
    /* non-finite inf: each part, each kind */
    x->arch = ADF_ARCH_COMPLEX;
    arb_indeterminate(acb_realref(x->inf));
    ADF_CHECK(!adf_sball_is_canonical(x));
    arb_pos_inf(acb_realref(x->inf));
    ADF_CHECK(!adf_sball_is_canonical(x));
    arb_zero_pm_inf(acb_realref(x->inf));
    ADF_CHECK(!adf_sball_is_canonical(x));
    arb_set_si(acb_realref(x->inf), 1);
    arb_neg_inf(acb_imagref(x->inf));
    ADF_CHECK(!adf_sball_is_canonical(x));
    arb_zero(acb_imagref(x->inf));
    x->arch = ADF_ARCH_REAL;
    arb_indeterminate(acb_realref(x->inf));
    ADF_CHECK(!adf_sball_is_canonical(x));
    arb_set_si(acb_realref(x->inf), 5);
    ADF_CHECK(adf_sball_is_canonical(x));
    /* len negative; len > 0 with a NULL array */
    x->len = -1;
    ADF_CHECK(!adf_sball_is_canonical(x));
    x->len = LONG_MIN;
    ADF_CHECK(!adf_sball_is_canonical(x));
    x->len = 3;
    saved = x->loc;
    x->loc = NULL;
    ADF_CHECK(!adf_sball_is_canonical(x));
    x->loc = saved;
    ADF_CHECK(adf_sball_is_canonical(x));
    /* a component that is not canonical: a composite prime, a ball with u not an integer, u >= p^(N - v) */
    x->loc[1].p = 25;
    ADF_CHECK(!adf_sball_is_canonical(x));
    x->loc[1].p = 5;
    fmpz_set_si(fmpq_denref(x->loc[1].u), 2);
    ADF_CHECK(!adf_sball_is_canonical(x));
    fmpz_set_si(fmpq_denref(x->loc[1].u), 1);
    fmpz_set_si(fmpq_numref(x->loc[1].u), 2 * 625);
    ADF_CHECK(!adf_sball_is_canonical(x));
    fmpz_set_si(fmpq_numref(x->loc[1].u), 7);
    ADF_CHECK(adf_sball_is_canonical(x));
    x->loc[0].exact = 2;
    ADF_CHECK(!adf_sball_is_canonical(x));
    x->loc[0].exact = 1;
    /* primes not strictly increasing: equal, decreasing (the components themselves are canonical) */
    x->loc[1].p = 2;   /* 7 < 2^(4 - 0)?  u = 7, N = 4, v = 0: 7 < 16 and 2 does not divide 7 */
    ADF_CHECK(adf_lball_is_canonical(&x->loc[1]));
    ADF_CHECK(!adf_sball_is_canonical(x));   /* 2, 2, 11 */
    x->loc[0].p = 3;
    fmpz_set_si(fmpq_numref(x->loc[0].u), 1);
    fmpz_set_si(fmpq_denref(x->loc[0].u), 2);
    ADF_CHECK(adf_lball_is_canonical(&x->loc[0]));
    ADF_CHECK(!adf_sball_is_canonical(x));   /* 3, 2, 11 */
    x->loc[1].p = 3;
    ADF_CHECK(!adf_sball_is_canonical(x));   /* 3, 3, 11 */
    x->loc[1].p = 5;
    ADF_CHECK(adf_sball_is_canonical(x));    /* 3, 5, 11 */
    x->loc[2].p = 5;
    ADF_CHECK(!adf_sball_is_canonical(x));   /* 3, 5, 5 */
    x->loc[2].p = 13;
    ADF_CHECK(adf_sball_is_canonical(x));   /* 3, 5, 13 */
    ADF_CHECK(x->len == 3);

    adf_sball_clear(x);
    arb_clear(r);
    for (i = 0; i < 3; i++)
        adf_lball_clear(&loc[i]);
}

/* ---------------------------------------------------------------------------------------------- constructor */

/* The constructor sorts by prime, copies the real ball and the components, and reports the first offending place.
   A constructor that does not sort fails on the order; one that writes on DOMAIN fails on the sentinel. */
ADF_TEST(set_arb_lballs)
{
    adf_sball_t y, s, want;
    adf_lball_struct loc[4], sorted[4];
    arb_t r;
    adf_place_t where, mark = place_of(1000003);
    adf_place_t v;
    int i, st;

    for (i = 0; i < 4; i++)
    {
        adf_lball_init(&loc[i]);
        adf_lball_init(&sorted[i]);
    }
    adf_sball_init(y);
    adf_sball_init(s);
    adf_sball_init(want);
    arb_init(r);

    lb_exact_of(&loc[0], 7, 1, 7);
    lb_ball_of(&loc[1], 2, 5, 3, 6);
    lb_ball_of(&loc[2], 13, 4, 1, 3);
    lb_exact_of(&loc[3], 3, 0, 1);
    for (i = 0; i < 4; i++)
        ADF_CHECK(adf_lball_is_canonical(&loc[i]));
    /* sorted: 2, 3, 7, 13 */
    adf_lball_set(&sorted[0], &loc[1]);
    adf_lball_set(&sorted[1], &loc[3]);
    adf_lball_set(&sorted[2], &loc[0]);
    adf_lball_set(&sorted[3], &loc[2]);
    arb_set_si(r, -3);
    mag_set_ui_2exp_si(arb_radref(r), 5, -4);

    where = mark;
    st = adf_sball_set_arb_lballs(y, &where, r, loc, 4);
    ADF_CHECK(st == ADF_OK && adf_place_equal(where, mark));
    sb_poke(want, ADF_ARCH_REAL, r, sorted, 4);
    ADF_CHECK(adf_sball_is_canonical(y) && adf_sball_identical(y, want));
    ADF_CHECK(arb_equal(acb_realref(y->inf), r) && arb_is_zero(acb_imagref(y->inf)));
    /* the components are copies: changing the input array does not change y */
    lb_exact_of(&loc[0], 7, 2, 1);
    ADF_CHECK(adf_sball_identical(y, want));

    /* no real part: the tag NONE; no components: the empty value; NULL where */
    st = adf_sball_set_arb_lballs(y, NULL, NULL, loc, 4);
    ADF_CHECK(st == ADF_OK && y->arch == ADF_ARCH_NONE && y->len == 4 && acb_is_zero(y->inf) && adf_sball_is_canonical(y));
    st = adf_sball_set_arb_lballs(y, NULL, NULL, NULL, 0);
    ADF_CHECK(st == ADF_OK && y->arch == ADF_ARCH_NONE && y->len == 0 && adf_sball_is_canonical(y));
    st = adf_sball_set_arb_lballs(y, NULL, r, NULL, 0);
    ADF_CHECK(st == ADF_OK && y->arch == ADF_ARCH_REAL && y->len == 0 && adf_sball_is_canonical(y));

    /* statuses: y untouched, where written */
    sb_sentinel(s);
    /* a repeated prime */
    adf_lball_set(&loc[2], &loc[1]);   /* two components at 2 */
    adf_sball_set(y, s);
    where = mark;
    st = adf_sball_set_arb_lballs(y, &where, r, loc, 4);
    ADF_CHECK(st == ADF_DOMAIN && adf_sball_identical(y, s) && adf_place_equal(where, place_of(2)));
    /* a non-finite real ball: each kind */
    lb_ball_of(&loc[2], 13, 4, 1, 3);
    {
        arb_t bad[3];
        for (i = 0; i < 3; i++)
            arb_init(bad[i]);
        arb_indeterminate(bad[0]);
        arb_pos_inf(bad[1]);
        arb_zero_pm_inf(bad[2]);
        for (i = 0; i < 3; i++)
        {
            where = mark;
            st = adf_sball_set_arb_lballs(y, &where, bad[i], loc, 4);
            ADF_CHECK(st == ADF_DOMAIN && adf_sball_identical(y, s) && adf_place_is_archimedean(where));
            arb_clear(bad[i]);
        }
    }
    /* a component that is not canonical, at a prime: the place of that prime; at a composite: no place */
    adf_lball_set(&loc[2], &sorted[3]);
    loc[2].p = 13;
    fmpz_set_si(fmpq_denref(loc[2].u), 2);
    where = mark;
    st = adf_sball_set_arb_lballs(y, &where, r, loc, 4);
    ADF_CHECK(st == ADF_DOMAIN && adf_sball_identical(y, s) && adf_place_equal(where, place_of(13)));
    fmpz_set_si(fmpq_denref(loc[2].u), 1);
    loc[2].p = 15;
    where = mark;
    st = adf_sball_set_arb_lballs(y, &where, r, loc, 4);
    ADF_CHECK(st == ADF_DOMAIN && adf_sball_identical(y, s) && adf_place_equal(where, mark));
    /* n < 0: DOMAIN, where untouched */
    where = mark;
    st = adf_sball_set_arb_lballs(y, &where, r, loc, -1);
    ADF_CHECK(st == ADF_DOMAIN && adf_sball_identical(y, s) && adf_place_equal(where, mark));
    ADF_CHECK(adf_sball_get_place(&v, s, 0) == ADF_OK);   /* the sentinel is intact and has the place inf first */

    adf_sball_clear(y);
    adf_sball_clear(s);
    adf_sball_clear(want);
    arb_clear(r);
    for (i = 0; i < 4; i++)
    {
        adf_lball_clear(&loc[i]);
        adf_lball_clear(&sorted[i]);
    }
}

/* ------------------------------------------------------------------------------------------------ accessors */

ADF_TEST(accessors)
{
    adf_sball_t x, e;
    adf_lball_struct loc[3];
    adf_lball_t c;
    arb_t r, out;
    adf_place_t v, mark = place_of(1000003);
    int i;
    for (i = 0; i < 3; i++)
        adf_lball_init(&loc[i]);
    adf_lball_init(c);
    arb_init(r);
    arb_init(out);
    adf_sball_init(x);
    adf_sball_init(e);
    lb_ball_of(&loc[0], 2, 5, 3, 6);
    lb_exact_of(&loc[1], 5, 7, 3);
    lb_ball_of(&loc[2], 7, 4, 1, 3);
    arb_set_si(r, 9);

    /* the empty value */
    ADF_CHECK(adf_sball_num_places(e) == 0 && adf_sball_arch(e) == ADF_ARCH_NONE);
    v = mark;
    ADF_CHECK(adf_sball_get_place(&v, e, 0) == ADF_DOMAIN && adf_place_equal(v, mark));
    ADF_CHECK(!adf_sball_has_place(e, adf_place_inf()) && !adf_sball_has_place(e, place_of(2)));

    /* {inf, 2, 5, 7} */
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);
    ADF_CHECK(adf_sball_num_places(x) == 4 && adf_sball_arch(x) == ADF_ARCH_REAL);
    ADF_CHECK(adf_sball_get_place(&v, x, 0) == ADF_OK && adf_place_is_archimedean(v));
    ADF_CHECK(adf_sball_get_place(&v, x, 1) == ADF_OK && adf_place_prime_get(v) == 2);
    ADF_CHECK(adf_sball_get_place(&v, x, 2) == ADF_OK && adf_place_prime_get(v) == 5);
    ADF_CHECK(adf_sball_get_place(&v, x, 3) == ADF_OK && adf_place_prime_get(v) == 7);
    v = mark;
    ADF_CHECK(adf_sball_get_place(&v, x, 4) == ADF_DOMAIN && adf_place_equal(v, mark));
    ADF_CHECK(adf_sball_get_place(&v, x, -1) == ADF_DOMAIN && adf_place_equal(v, mark));
    ADF_CHECK(adf_sball_get_place(&v, x, LONG_MAX) == ADF_DOMAIN && adf_sball_get_place(&v, x, LONG_MIN) == ADF_DOMAIN);
    ADF_CHECK(adf_sball_has_place(x, adf_place_inf()) && adf_sball_has_place(x, place_of(2)) &&
              adf_sball_has_place(x, place_of(5)) && adf_sball_has_place(x, place_of(7)));
    ADF_CHECK(!adf_sball_has_place(x, place_of(3)) && !adf_sball_has_place(x, place_of(11)) &&
              !adf_sball_has_place(x, place_of(1000003)));
    /* components */
    for (i = 0; i < 3; i++)
    {
        adf_lball_set(c, &loc[(i + 1) % 3]);   /* a different value in c first */
        ADF_CHECK(adf_sball_get_lball(c, x, adf_lball_place(&loc[i])) == ADF_OK && adf_lball_identical(c, &loc[i]));
    }
    lb_exact_of(c, 3, 1, 1);
    {
        adf_lball_t keep;
        adf_lball_init(keep);
        adf_lball_set(keep, c);
        ADF_CHECK(adf_sball_get_lball(c, x, place_of(3)) == ADF_DOMAIN && adf_lball_identical(c, keep));
        ADF_CHECK(adf_sball_get_lball(c, x, adf_place_inf()) == ADF_DOMAIN && adf_lball_identical(c, keep));
        ADF_CHECK(adf_sball_get_lball(c, e, place_of(2)) == ADF_DOMAIN && adf_lball_identical(c, keep));
        adf_lball_clear(keep);
    }
    arb_set_si(out, 77);
    ADF_CHECK(adf_sball_get_arb(out, x, adf_place_inf()) == ADF_OK && arb_equal(out, r));
    arb_set_si(out, 77);
    ADF_CHECK(adf_sball_get_arb(out, x, place_of(2)) == ADF_DOMAIN && arb_equal_si(out, 77));
    ADF_CHECK(adf_sball_get_arb(out, e, adf_place_inf()) == ADF_DOMAIN && arb_equal_si(out, 77));
    x->arch = ADF_ARCH_COMPLEX;
    ADF_CHECK(adf_sball_get_arb(out, x, adf_place_inf()) == ADF_DOMAIN && arb_equal_si(out, 77));
    ADF_CHECK(adf_sball_num_places(x) == 4 && adf_sball_arch(x) == ADF_ARCH_COMPLEX);
    /* no archimedean place: 3 places, the first is a prime */
    sb_poke(x, ADF_ARCH_NONE, NULL, loc, 3);
    ADF_CHECK(adf_sball_num_places(x) == 3 && adf_sball_get_place(&v, x, 0) == ADF_OK && adf_place_prime_get(v) == 2);
    ADF_CHECK(!adf_sball_has_place(x, adf_place_inf()));

    adf_sball_clear(x);
    adf_sball_clear(e);
    adf_lball_clear(c);
    arb_clear(r);
    arb_clear(out);
    for (i = 0; i < 3; i++)
        adf_lball_clear(&loc[i]);
}

/* -------------------------------------------------------------------------------------------- the projection */

static void
adele_from_json(adf_adele_t a, const jsonl_value * o)
{
    arb_t r;
    fmpz_t A, H, d;
    adf_fball_t f;
    arb_init(r);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    adf_fball_init(f);
    rb_from_json(r, member(o, "inf"));
    fmpz_from_text(A, member_int(o, "A"));
    fmpz_from_text(H, member_int(o, "H"));
    fmpz_from_text(d, member_int(o, "d"));
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    ADF_CHECK(adf_adele_set_arb_fball(a, r, f) == ADF_OK);
    adf_fball_clear(f);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    arb_clear(r);
}

static void
project_row(const jsonl_value * rec)
{
    jsonl_error_t err;
    const jsonl_value * pl = member(rec, "places");
    size_t i, n = jsonl_size(pl);
    adf_place_t * ps = (adf_place_t *) flint_malloc((n ? n : 1) * sizeof(adf_place_t));
    adf_adele_t a;
    adf_sball_t y, want, s;
    adf_place_t where, mark = place_of(1000003);
    int st, want_st = status_from_name(member_str(rec, "status"));
    unsigned long line = jsonl_line_of(rec);

    for (i = 0; i < n; i++)
        ps[i] = place_from_json(jsonl_at(pl, i, &err));
    adf_adele_init(a);
    adf_sball_init(y);
    adf_sball_init(want);
    adf_sball_init(s);
    adele_from_json(a, member(rec, "x"));
    sb_sentinel(y);
    sb_sentinel(s);
    where = mark;
    st = adf_sball_project(y, &where, a, ps, (slong) n);
    ADF_CHECK_MSG(st == want_st, "project line %lu: status %s, want %s", line, adf_status_str(st),
                  adf_status_str(want_st));
    if (want_st == ADF_OK && st == ADF_OK)
    {
        sb_from_json(want, member(rec, "result"));
        ADF_CHECK_MSG(adf_sball_is_canonical(y), "project line %lu: not canonical", line);
        ADF_CHECK_MSG(adf_sball_identical(y, want), "project line %lu: result differs", line);
        ADF_CHECK_MSG(adf_place_equal(where, mark), "project line %lu: where written on OK", line);
        ADF_CHECK_MSG(adf_sball_num_places(y) == (slong) n, "project line %lu: number of places", line);
        /* every requested place is a place of y, and the components equal the single projections */
        for (i = 0; i < n; i++)
        {
            ADF_CHECK_MSG(adf_sball_has_place(y, ps[i]), "project line %lu: place %lu missing", line, (unsigned long) i);
            if (!adf_place_is_archimedean(ps[i]))
            {
                adf_lball_t one, got;
                adf_lball_init(one);
                adf_lball_init(got);
                ADF_CHECK(adf_lball_set_fball(one, ps[i], &a->fin) == ADF_OK);
                ADF_CHECK(adf_sball_get_lball(got, y, ps[i]) == ADF_OK && adf_lball_identical(one, got));
                adf_lball_clear(one);
                adf_lball_clear(got);
            }
        }
    }
    else
    {
        ADF_CHECK_MSG(adf_sball_identical(y, s), "project line %lu: output written on a status", line);
        if (want_st != ADF_OK)
            ADF_CHECK_MSG(adf_place_equal(where, place_from_json(member(rec, "where"))), "project line %lu: where", line);
    }
    flint_free(ps);
    adf_adele_clear(a);
    adf_sball_clear(y);
    adf_sball_clear(want);
    adf_sball_clear(s);
}

ADF_TEST(vectors_project)
{
    run_vectors("sball_project.jsonl", project_row, 200);
}

/* Enumeration, independent of the reference: for p = 2, 3, 5, 7 and the adele (r ; (A + H Zhat)/d), every point
   A/d + H z/d (z = 0 .. p^2 - 1) is inside the component at p, and the p classes of these points modulo the next
   digit (balls of exponent N + 1 around them) are pairwise different: no smaller ball contains them all. A projection
   that returns a larger ball fails the second, one that returns a smaller fails the first. */
ADF_TEST(projection_enumeration)
{
    const ulong primes[4] = {2, 3, 5, 7};
    slong dd, HH, AA;
    unsigned long cases = 0;
    adf_adele_t a;
    adf_sball_t y;
    arb_t r;
    adf_place_t ps[5];
    int k;
    adf_adele_init(a);
    adf_sball_init(y);
    arb_init(r);
    arb_set_si(r, 1);
    for (k = 0; k < 4; k++)
        ps[k] = place_of(primes[k]);
    ps[4] = adf_place_inf();
    for (dd = 1; dd <= 30; dd += (dd < 8 ? 1 : 7))
        for (HH = 0; HH <= 300; HH += (HH < 12 ? 1 : (HH < 60 ? 5 : 47)))
            for (AA = -40; AA <= 40; AA += 13)
            {
                fmpz_t A, H, d;
                adf_fball_t f;
                fmpz_init_set_si(A, AA);
                fmpz_init_set_si(H, HH);
                fmpz_init_set_si(d, dd);
                adf_fball_init(f);
                ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
                ADF_CHECK(adf_adele_set_arb_fball(a, r, f) == ADF_OK);
                ADF_CHECK(adf_sball_project(y, NULL, a, ps, 5) == ADF_OK);
                for (k = 0; k < 4; k++)
                {
                    ulong p = primes[k], z;
                    adf_lball_t comp, pt, cls[7];
                    unsigned long distinct = 0;
                    ulong j;
                    adf_lball_init(comp);
                    adf_lball_init(pt);
                    ADF_CHECK(adf_sball_get_lball(comp, y, ps[k]) == ADF_OK);
                    for (j = 0; j < 7; j++)
                        adf_lball_init(cls[j]);
                    if (HH == 0)
                        ADF_CHECK(comp->exact);
                    else
                        ADF_CHECK(!comp->exact);
                    for (z = 0; z < p * p; z++)
                    {
                        fmpq_t point;
                        adf_rat_t q;
                        fmpq_init(point);
                        adf_rat_init(q);
                        /* point = A/d + H z / d */
                        fmpq_set_si(point, AA + HH * (slong) z, dd);
                        fmpq_set(q->q, point);
                        ADF_CHECK(adf_lball_set_rat(pt, ps[k], q) == ADF_OK);
                        ADF_CHECK_MSG(adf_lball_contains(pt, comp), "p=%lu A=%ld H=%ld d=%ld z=%lu", p, (long) AA,
                                      (long) HH, (long) dd, z);
                        if (HH > 0)
                        {
                            adf_lball_t c2;
                            unsigned long m;
                            int seen = 0;
                            adf_lball_init(c2);
                            ADF_CHECK(adf_lball_set_rat_ball(c2, ps[k], q, comp->N + 1) == ADF_OK);
                            for (m = 0; m < distinct; m++)
                                if (adf_lball_equal_set(cls[m], c2))
                                    seen = 1;
                            if (!seen && distinct < 7)
                                adf_lball_set(cls[distinct++], c2);
                            adf_lball_clear(c2);
                        }
                        adf_rat_clear(q);
                        fmpq_clear(point);
                    }
                    if (HH > 0)
                        ADF_CHECK_MSG(distinct == p, "p=%lu A=%ld H=%ld d=%ld: %lu classes", p, (long) AA, (long) HH,
                                      (long) dd, distinct);
                    for (j = 0; j < 7; j++)
                        adf_lball_clear(cls[j]);
                    adf_lball_clear(comp);
                    adf_lball_clear(pt);
                    cases++;
                }
                adf_fball_clear(f);
                fmpz_clear(A);
                fmpz_clear(H);
                fmpz_clear(d);
            }
    ADF_CHECK(cases > 1000);
    arb_clear(r);
    adf_sball_clear(y);
    adf_adele_clear(a);
}

/* Places in every order give the same partial ball (a projection that keeps the order given fails); n = 0 is the
   empty set of places with places = NULL; the real place alone; a repetition is DOMAIN with the place; x with a local
   backend as well. */
ADF_TEST(project_orders_and_statuses)
{
    adf_adele_t a;
    adf_sball_t y, y2, s;
    adf_place_t ps[4], where, mark = place_of(1000003), inf = adf_place_inf();
    int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    adf_place_t base[3];
    int i, st;
    fmpz_t A, H, d;
    adf_fball_t f;
    arb_t r;

    adf_adele_init(a);
    adf_sball_init(y);
    adf_sball_init(y2);
    adf_sball_init(s);
    fmpz_init_set_si(A, 7);
    fmpz_init_set_si(H, 360);
    fmpz_init_set_si(d, 5);
    adf_fball_init(f);
    arb_init(r);
    arb_set_si(r, 3);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    ADF_CHECK(adf_adele_set_arb_fball(a, r, f) == ADF_OK);
    base[0] = place_of(2);
    base[1] = place_of(5);
    base[2] = inf;
    ADF_CHECK(adf_sball_project(y, NULL, a, base, 3) == ADF_OK && adf_sball_is_canonical(y));
    ADF_CHECK(y->arch == ADF_ARCH_REAL && y->len == 2 && y->loc != NULL && y->loc[0].p == 2 && y->loc[1].p == 5);
    /* 7/5 + 360/5 Zhat = 7/5 + 72 Zhat: at 2 the ball 7/5 + 2^3 Z_2; at 5: v_5(72) = 0, the ball 5^0 Z_5 */
    ADF_CHECK(y->len == 2 && y->loc != NULL && y->loc[0].N == 3 && !y->loc[0].exact && y->loc[1].N == 0 &&
              !y->loc[1].exact);
    for (i = 0; i < 6; i++)
    {
        ps[0] = base[perm[i][0]];
        ps[1] = base[perm[i][1]];
        ps[2] = base[perm[i][2]];
        ADF_CHECK(adf_sball_project(y2, NULL, a, ps, 3) == ADF_OK && adf_sball_identical(y, y2));
    }
    /* the real place alone and the primes alone */
    ps[0] = inf;
    ADF_CHECK(adf_sball_project(y2, NULL, a, ps, 1) == ADF_OK && y2->arch == ADF_ARCH_REAL && y2->len == 0 &&
              arb_equal(acb_realref(y2->inf), r));
    ps[0] = place_of(5);
    ps[1] = place_of(2);
    ADF_CHECK(adf_sball_project(y2, NULL, a, ps, 2) == ADF_OK && y2->arch == ADF_ARCH_NONE && y2->len == 2 &&
              y2->loc[0].p == 2 && acb_is_zero(y2->inf));
    ADF_CHECK(adf_sball_project(y2, NULL, a, NULL, 0) == ADF_OK && y2->len == 0 && y2->arch == ADF_ARCH_NONE &&
              adf_sball_is_canonical(y2));
    ADF_CHECK(adf_sball_project(y2, NULL, a, ps, 0) == ADF_OK && y2->len == 0);
    /* repetitions */
    sb_sentinel(s);
    sb_sentinel(y2);
    ps[0] = place_of(5);
    ps[1] = place_of(2);
    ps[2] = place_of(5);
    where = mark;
    st = adf_sball_project(y2, &where, a, ps, 3);
    ADF_CHECK(st == ADF_DOMAIN && adf_sball_identical(y2, s) && adf_place_equal(where, place_of(5)));
    ps[0] = inf;
    ps[1] = inf;
    where = mark;
    st = adf_sball_project(y2, &where, a, ps, 2);
    ADF_CHECK(st == ADF_DOMAIN && adf_sball_identical(y2, s) && adf_place_equal(where, inf));
    where = mark;
    st = adf_sball_project(y2, &where, a, ps, -1);
    ADF_CHECK(st == ADF_DOMAIN && adf_sball_identical(y2, s) && adf_place_equal(where, mark));
    /* the archimedean part is copied as it is (radius included) */
    mag_set_ui_2exp_si(arb_radref(a->inf), 3, -20);
    ps[0] = inf;
    ADF_CHECK(adf_sball_project(y2, NULL, a, ps, 1) == ADF_OK && arb_equal(acb_realref(y2->inf), a->inf));

    arb_clear(r);
    adf_fball_clear(f);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_sball_clear(y);
    adf_sball_clear(y2);
    adf_sball_clear(s);
    adf_adele_clear(a);
}

/* LIMIT of the first prime, in canonical order, at which the local projection fails. The adele has A = H - 1 with
   H = 6^(2^25 + 1): at 2 and 3 the reduction of the centre needs 2^(2^25 + 1) resp. 3^(2^25 + 1), more than the bit
   bound (adf_lball_set_fball: LIMIT); at 5 and 7 the projection is fine. Also: only the second failing prime. */
ADF_TEST(project_limit_names_the_first_failing_prime)
{
    adf_adele_t a;
    adf_sball_t y, s;
    fmpz_t A, H, d;
    adf_fball_t f;
    arb_t r;
    adf_place_t ps[5], where, mark = place_of(1000003);
    int st;

    adf_adele_init(a);
    adf_sball_init(y);
    adf_sball_init(s);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    adf_fball_init(f);
    arb_init(r);
    arb_set_si(r, 3);
    fmpz_set_ui(H, 6);
    fmpz_pow_ui(H, H, (1ul << 25) + 1);
    fmpz_sub_ui(A, H, 1);
    fmpz_one(d);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    ADF_CHECK(adf_adele_set_arb_fball(a, r, f) == ADF_OK);
    sb_sentinel(y);
    sb_sentinel(s);
    ps[0] = place_of(7);
    ps[1] = place_of(3);
    ps[2] = adf_place_inf();
    ps[3] = place_of(2);
    ps[4] = place_of(5);
    where = mark;
    st = adf_sball_project(y, &where, a, ps, 5);
    ADF_CHECK(st == ADF_LIMIT && adf_place_equal(where, place_of(2)) && adf_sball_identical(y, s));
    ps[3] = place_of(11);   /* only 3 fails now */
    where = mark;
    st = adf_sball_project(y, &where, a, ps, 5);
    ADF_CHECK(st == ADF_LIMIT && adf_place_equal(where, place_of(3)) && adf_sball_identical(y, s));
    ps[1] = place_of(13);   /* nothing fails */
    where = mark;
    st = adf_sball_project(y, &where, a, ps, 5);
    ADF_CHECK(st == ADF_OK && adf_place_equal(where, mark) && adf_sball_is_canonical(y) && y->len == 4);

    arb_clear(r);
    adf_fball_clear(f);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_sball_clear(y);
    adf_sball_clear(s);
    adf_adele_clear(a);
}

/* --------------------------------------------------------------------------------------- set predicates */

typedef int (*pred_fn)(const adf_sball_t, const adf_sball_t);

static pred_fn
pred_by_name(const char * s)
{
    if (strcmp(s, "equal_set") == 0)
        return adf_sball_equal_set;
    if (strcmp(s, "overlaps") == 0)
        return adf_sball_overlaps;
    if (strcmp(s, "contains") == 0)
        return adf_sball_contains;
    ADF_CHECK_MSG(0, "unknown predicate %s", s);
    return adf_sball_equal_set;
}

static void
pred_row(const jsonl_value * rec)
{
    const char * op = member_str(rec, "op");
    adf_sball_t x, y;
    int want = (int) member_slong(rec, "want"), got;
    adf_sball_init(x);
    adf_sball_init(y);
    sb_from_json(x, member(rec, "x"));
    sb_from_json(y, member(rec, "y"));
    ADF_CHECK_MSG(adf_sball_is_canonical(x) && adf_sball_is_canonical(y), "line %lu: input", jsonl_line_of(rec));
    got = pred_by_name(op)(x, y);
    ADF_CHECK_MSG(got == want, "%s line %lu: got %d, want %d", op, jsonl_line_of(rec), got, want);
    adf_sball_clear(x);
    adf_sball_clear(y);
}

/* Every line: the same places and tags required; components by lball and by intervals (exact end points). A
   predicate that ignores the places, or the tag, or compares the real end points with a tolerance, fails. */
ADF_TEST(vectors_predicates)
{
    run_vectors("sball_pred.jsonl", pred_row, 1000);
}

/* By hand: the reflexive cases, the direction of contains, different places and tags, complex components, and that
   equal sets can have different representations (identical is stricter). */
ADF_TEST(predicates_by_hand)
{
    adf_sball_t x, y, e1, e2;
    adf_lball_struct l2[1], l3[1], l23[2];
    arb_t r, s;
    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(e1);
    adf_sball_init(e2);
    adf_lball_init(&l2[0]);
    adf_lball_init(&l3[0]);
    adf_lball_init(&l23[0]);
    adf_lball_init(&l23[1]);
    arb_init(r);
    arb_init(s);
    lb_ball_of(&l2[0], 2, 1, 1, 5);
    lb_ball_of(&l3[0], 3, 1, 1, 5);
    adf_lball_set(&l23[0], &l2[0]);
    adf_lball_set(&l23[1], &l3[0]);
    arb_set_si(r, 10);
    mag_set_ui(arb_radref(r), 2);        /* [8, 12] */
    arb_set_si(s, 10);
    mag_set_ui(arb_radref(s), 1);        /* [9, 11] */

    /* the empty sets of places: one point, equal to itself */
    ADF_CHECK(adf_sball_equal_set(e1, e2) && adf_sball_overlaps(e1, e2) && adf_sball_contains(e1, e2));
    sb_poke(x, ADF_ARCH_REAL, r, l2, 1);
    sb_poke(y, ADF_ARCH_REAL, s, l2, 1);
    ADF_CHECK(adf_sball_equal_set(x, x) && adf_sball_overlaps(x, x) && adf_sball_contains(x, x));
    ADF_CHECK(!adf_sball_equal_set(x, y) && adf_sball_overlaps(x, y));
    /* "first inside second": y = [9, 11] is inside x = [8, 12] */
    ADF_CHECK(adf_sball_contains(y, x) == 1 && adf_sball_contains(x, y) == 0);
    sb_poke(x, ADF_ARCH_REAL, s, l2, 1);   /* [9, 11] */
    sb_poke(y, ADF_ARCH_REAL, r, l2, 1);   /* [8, 12] */
    ADF_CHECK(adf_sball_contains(x, y) == 1 && adf_sball_contains(y, x) == 0);
    /* disjoint real intervals */
    arb_set_si(s, 100);
    mag_set_ui(arb_radref(s), 1);
    sb_poke(x, ADF_ARCH_REAL, s, l2, 1);
    ADF_CHECK(!adf_sball_overlaps(x, y) && !adf_sball_contains(x, y));
    /* the real intervals touch at an end point: [8, 12] and [12, 14] overlap (closed) */
    arb_set_si(s, 13);
    mag_set_ui(arb_radref(s), 1);
    sb_poke(x, ADF_ARCH_REAL, s, l2, 1);
    ADF_CHECK(adf_sball_overlaps(x, y) && adf_sball_overlaps(y, x));
    /* different places: same primes with and without the real place; other primes; tags */
    sb_poke(x, ADF_ARCH_REAL, r, l2, 1);
    sb_poke(y, ADF_ARCH_NONE, NULL, l2, 1);
    ADF_CHECK(!adf_sball_equal_set(x, y) && !adf_sball_overlaps(x, y) && !adf_sball_contains(x, y) &&
              !adf_sball_contains(y, x));
    sb_poke(y, ADF_ARCH_REAL, r, l3, 1);
    ADF_CHECK(!adf_sball_equal_set(x, y) && !adf_sball_overlaps(x, y) && !adf_sball_contains(x, y));
    sb_poke(y, ADF_ARCH_REAL, r, l23, 2);
    ADF_CHECK(!adf_sball_equal_set(x, y) && !adf_sball_overlaps(x, y) && !adf_sball_contains(x, y) &&
              !adf_sball_contains(y, x));
    sb_poke(y, ADF_ARCH_COMPLEX, r, l2, 1);
    ADF_CHECK(!adf_sball_equal_set(x, y) && !adf_sball_overlaps(x, y) && !adf_sball_contains(x, y));
    /* complex boxes: a box inside a box, and overlapping boxes; the tags agree */
    sb_poke(x, ADF_ARCH_COMPLEX, r, l2, 1);
    arb_set_si(acb_imagref(x->inf), 0);
    mag_set_ui(arb_radref(acb_imagref(x->inf)), 2);
    sb_poke(y, ADF_ARCH_COMPLEX, s, l2, 1);
    arb_set_si(s, 10);
    mag_set_ui(arb_radref(s), 1);
    sb_poke(y, ADF_ARCH_COMPLEX, s, l2, 1);
    mag_set_ui(arb_radref(acb_imagref(y->inf)), 1);   /* y = [9, 11] x [-1, 1] inside x = [8, 12] x [-2, 2] */
    ADF_CHECK(adf_sball_contains(y, x) == 1 && adf_sball_contains(x, y) == 0 && adf_sball_overlaps(x, y) &&
              !adf_sball_equal_set(x, y));
    mag_set_ui(arb_radref(acb_imagref(y->inf)), 3);   /* taller than x: not inside */
    ADF_CHECK(adf_sball_contains(y, x) == 0 && adf_sball_overlaps(x, y));
    arb_set_si(acb_imagref(y->inf), 10);
    ADF_CHECK(!adf_sball_overlaps(x, y));
    /* one interval has one ball (an exact midpoint and a radius), so equal sets are identical here */
    arb_set_si(s, 2);
    mag_set_ui(arb_radref(s), 2);
    arb_set_si(r, 2);
    mag_set_ui(arb_radref(r), 2);
    sb_poke(x, ADF_ARCH_REAL, s, l2, 1);
    sb_poke(y, ADF_ARCH_REAL, r, l2, 1);
    ADF_CHECK(adf_sball_equal_set(x, y) && adf_sball_identical(x, y));
    /* a midpoint that is not exact in the end points' rounding: mid = 1, rad = 2^-70; the end points 1 +- 2^-70 differ
       from 1 +- (2^-70 + 2^-200) although the difference is far below any working precision: not equal */
    arb_one(s);
    mag_set_ui_2exp_si(arb_radref(s), 1, -70);
    arb_one(r);
    mag_set_ui_2exp_si(arb_radref(r), 1, -70);
    arb_add_error_2exp_si(r, -200);
    sb_poke(x, ADF_ARCH_REAL, s, l2, 1);
    sb_poke(y, ADF_ARCH_REAL, r, l2, 1);
    ADF_CHECK(!adf_sball_equal_set(x, y) && adf_sball_contains(x, y) == 1 && adf_sball_contains(y, x) == 0);

    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(e1);
    adf_sball_clear(e2);
    adf_lball_clear(&l2[0]);
    adf_lball_clear(&l3[0]);
    adf_lball_clear(&l23[0]);
    adf_lball_clear(&l23[1]);
    arb_clear(r);
    arb_clear(s);
}

/* ----------------------------------------------------------------------------------- componentwise operations */

static int
call_op(const char * op, adf_sball_t z, adf_place_t * where, const adf_sball_t x, const adf_sball_t y, slong prec)
{
    if (strcmp(op, "add") == 0)
        return adf_sball_add(z, where, x, y, prec);
    if (strcmp(op, "sub") == 0)
        return adf_sball_sub(z, where, x, y, prec);
    if (strcmp(op, "mul") == 0)
        return adf_sball_mul(z, where, x, y, prec);
    return adf_sball_neg(z, where, x);
}

/* The real part of a result against the exact set of results [lo, hi] of the row: contains both, and the radius is
   within the factor F of the half width (1 for add, sub, neg; 2 for mul: arb's product adds |a| s + |b| r + r s) plus
   the rounding of prec bits. */
static void
check_real_result(const char * op, const arb_t R, const jsonl_value * real, slong prec, unsigned long line)
{
    arf_t lo, hi, rad, lhs, rhs, t, mx;
    slong p = prec < 2 ? 2 : prec;
    const slong W = 3000;
    arf_init(lo);
    arf_init(hi);
    arf_init(rad);
    arf_init(lhs);
    arf_init(rhs);
    arf_init(t);
    arf_init(mx);
    dy_from_json(lo, member(real, "lo"));
    dy_from_json(hi, member(real, "hi"));
    ADF_CHECK_MSG(arb_is_finite(R), "%s line %lu: not finite", op, line);
    ADF_CHECK_MSG(arb_contains_arf(R, lo) && arb_contains_arf(R, hi), "%s line %lu: real part does not contain the set of results",
                  op, line);
    arf_set_mag(rad, arb_radref(R));
    arf_sub(rhs, hi, lo, W, ARF_RND_UP);                         /* width = 2 half width */
    arf_mul_2exp_si(rhs, rhs, strcmp(op, "mul") == 0 ? 1 : 0);   /* F times the width */
    arf_mul_2exp_si(t, rhs, -25);                                /* mag rounds a radius up by a relative 2^-29 */
    arf_add(rhs, rhs, t, W, ARF_RND_UP);
    arf_abs(t, lo);
    arf_abs(mx, hi);
    arf_max(mx, mx, t);
    arf_mul_2exp_si(mx, mx, 4 - p);
    arf_add(rhs, rhs, mx, W, ARF_RND_UP);
    arf_mul_2exp_si(lhs, rad, 1);
    ADF_CHECK_MSG(arf_cmp(lhs, rhs) <= 0, "%s line %lu: real radius %g too large (width %g) at prec %ld", op, line,
                  arf_get_d(rad, ARF_RND_NEAR), arf_get_d(rhs, ARF_RND_NEAR), (long) prec);
    arf_clear(lo);
    arf_clear(hi);
    arf_clear(rad);
    arf_clear(lhs);
    arf_clear(rhs);
    arf_clear(t);
    arf_clear(mx);
}

static void
ops_row(const jsonl_value * rec)
{
    const char * op = member_str(rec, "op");
    slong prec = member_slong(rec, "prec");
    int want = status_from_name(member_str(rec, "status")), neg = strcmp(op, "neg") == 0, st, st2;
    unsigned long line = jsonl_line_of(rec);
    adf_sball_t x, y, z, s, xa, ya, res;
    adf_place_t where, mark = place_of(1000003);

    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(z);
    adf_sball_init(s);
    adf_sball_init(xa);
    adf_sball_init(ya);
    adf_sball_init(res);
    sb_from_json(x, member(rec, "x"));
    if (!neg)
        sb_from_json(y, member(rec, "y"));
    ADF_CHECK_MSG(adf_sball_is_canonical(x) && adf_sball_is_canonical(y), "%s line %lu: input", op, line);
    sb_sentinel(z);
    sb_sentinel(s);
    where = mark;
    st = call_op(op, z, &where, x, y, prec);
    ADF_CHECK_MSG(st == want, "%s line %lu: status %s, want %s", op, line, adf_status_str(st), adf_status_str(want));
    if (want == ADF_OK && st == ADF_OK)
    {
        const jsonl_value * result = member(rec, "result");
        const jsonl_value * real = member(result, "real");
        const jsonl_value * locs = member(result, "loc");
        jsonl_error_t err;
        size_t i;
        ADF_CHECK_MSG(adf_sball_is_canonical(z), "%s line %lu: result not canonical", op, line);
        ADF_CHECK_MSG(adf_place_equal(where, mark), "%s line %lu: where written on OK", op, line);
        ADF_CHECK_MSG(z->arch == (int) member_slong(result, "arch") && z->len == (slong) jsonl_size(locs),
                      "%s line %lu: places of the result", op, line);
        for (i = 0; i < jsonl_size(locs) && (slong) i < z->len; i++)
        {
            adf_lball_t w;
            adf_lball_init(w);
            lb_from_json(w, jsonl_at(locs, i, &err));
            ADF_CHECK_MSG(adf_lball_identical(&z->loc[i], w), "%s line %lu: component %lu differs", op, line,
                          (unsigned long) i);
            adf_lball_clear(w);
        }
        if (z->arch == ADF_ARCH_REAL)
        {
            ADF_CHECK(!jsonl_is_null(real, NULL));
            check_real_result(op, acb_realref(z->inf), real, prec, line);
            ADF_CHECK_MSG(arb_is_zero(acb_imagref(z->inf)), "%s line %lu: imaginary part", op, line);
        }
        else
            ADF_CHECK_MSG(acb_is_zero(z->inf) && jsonl_is_null(real, NULL), "%s line %lu: no real place, inf not 0", op, line);
        /* aliasing: (x, x, y), (y, x, y), (x, x, x) */
        adf_sball_set(xa, x);
        st2 = call_op(op, xa, NULL, xa, y, prec);
        ADF_CHECK_MSG(st2 == ADF_OK && adf_sball_identical(xa, z), "%s line %lu: alias (x, x, y)", op, line);
        if (!neg)
        {
            adf_sball_set(ya, y);
            st2 = call_op(op, ya, NULL, x, ya, prec);
            ADF_CHECK_MSG(st2 == ADF_OK && adf_sball_identical(ya, z), "%s line %lu: alias (y, x, y)", op, line);
            adf_sball_set(xa, x);
            st2 = call_op(op, res, NULL, xa, xa, prec);
            ADF_CHECK(st2 == ADF_OK);
            st2 = call_op(op, xa, NULL, xa, xa, prec);
            ADF_CHECK_MSG(st2 == ADF_OK && adf_sball_identical(xa, res), "%s line %lu: alias (x, x, x)", op, line);
        }
    }
    else
    {
        ADF_CHECK_MSG(adf_sball_identical(z, s), "%s line %lu: output written on a status", op, line);
        if (want != ADF_OK)
            ADF_CHECK_MSG(adf_place_equal(where, place_from_json(member(rec, "where"))), "%s line %lu: where", op, line);
        adf_sball_set(xa, x);
        st2 = call_op(op, xa, NULL, xa, y, prec);
        ADF_CHECK_MSG(st2 == st && adf_sball_identical(xa, x), "%s line %lu: alias on a status", op, line);
    }
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(z);
    adf_sball_clear(s);
    adf_sball_clear(xa);
    adf_sball_clear(ya);
    adf_sball_clear(res);
}

ADF_TEST(vectors_ops)
{
    run_vectors("sball_ops.jsonl", ops_row, 300);
}

/* ------------------------------------------------------------------ tuples of points through the operations */

static unsigned long long rng_state = 88172645463325252ULL;

static unsigned long
rnd(unsigned long n)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (unsigned long) (rng_state % n);
}

/* p^e as an fmpq (any integer e). */
static void
ppow(fmpq_t out, ulong p, slong e)
{
    fmpz_t pp;
    fmpz_init_set_ui(pp, p);
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

/* One point of the lball x: the value for an exact one; p^v u + p^N a for a ball (a a small integer). Built here
   with fmpz_pow_ui, from the fields, not with the library. */
static void
point_of(fmpq_t s, const adf_lball_struct * x, ulong a)
{
    fmpq_t t;
    fmpq_init(t);
    ppow(t, x->p, x->v);
    fmpq_mul(s, x->u, t);
    if (!x->exact)
    {
        ppow(t, x->p, x->N);
        fmpq_mul_ui(t, t, a);
        fmpq_add(s, s, t);
    }
    fmpq_clear(t);
}

/* One point of the real ball x: the end points, the midpoint or an interior point, as an fmpq (the midpoint m 2^e and the
   radius r are exact dyadics). */
static void
real_point_of(fmpq_t q, const arb_t x)
{
    fmpz_t m, e;
    fmpq_t mid, rad;
    unsigned long c = rnd(5);
    fmpz_init(m);
    fmpz_init(e);
    fmpq_init(mid);
    fmpq_init(rad);
    arf_get_fmpz_2exp(m, e, arb_midref(x));
    dy_to_fmpq(mid, m, fmpz_get_si(e));
    if (arf_is_zero(arb_midref(x)))
        fmpq_zero(mid);
    {
        fmpz_t rm, re;
        arf_t ra;
        fmpz_init(rm);
        fmpz_init(re);
        arf_init(ra);
        arf_set_mag(ra, arb_radref(x));
        arf_get_fmpz_2exp(rm, re, ra);
        if (fmpz_is_zero(rm))
            fmpq_zero(rad);
        else
            dy_to_fmpq(rad, rm, fmpz_get_si(re));
        arf_clear(ra);
        fmpz_clear(rm);
        fmpz_clear(re);
    }
    fmpq_set(q, mid);
    if (c == 0)
        fmpq_sub(q, mid, rad);
    else if (c == 1)
        fmpq_add(q, mid, rad);
    else if (c == 3)
    {
        fmpq_mul_ui(rad, rad, rnd(9));
        fmpq_div_2exp(rad, rad, 3);
        fmpq_add(q, mid, rad);
        if (rnd(2))
            fmpq_sub(q, mid, rad);
    }
    fmpz_clear(m);
    fmpz_clear(e);
    fmpq_clear(mid);
    fmpq_clear(rad);
}

static void
random_lball(adf_lball_struct * x, ulong p)
{
    adf_rat_t q;
    slong num = (slong) rnd(19) - 9, den = (slong) rnd(9) + 1;
    fmpz_t n, d;
    int kind = (int) rnd(4);
    adf_rat_init(q);
    fmpz_init_set_si(n, kind == 3 ? num * (slong) p : num);
    fmpz_init_set_si(d, den);
    ADF_CHECK(adf_rat_set_fmpz2(q, n, d) == ADF_OK);
    if (kind == 0)
        ADF_CHECK(adf_lball_set_rat(x, place_of(p), q) == ADF_OK);
    else
        ADF_CHECK(adf_lball_set_rat_ball(x, place_of(p), q, (slong) rnd(6) - 2) == ADF_OK);
    fmpz_clear(n);
    fmpz_clear(d);
    adf_rat_clear(q);
}

static void
random_sball(adf_sball_t s, const ulong * primes, int np)
{
    adf_lball_struct loc[4];
    arb_t r;
    int i;
    fmpz_t m;
    arb_init(r);
    fmpz_init(m);
    for (i = 0; i < np; i++)
    {
        adf_lball_init(&loc[i]);
        random_lball(&loc[i], primes[i]);
    }
    fmpz_set_si(m, (slong) rnd(101) - 50);
    arb_set_fmpz(r, m);
    arb_mul_2exp_si(r, r, (slong) rnd(6) - 3);
    if (rnd(4))
        mag_set_ui_2exp_si(arb_radref(r), rnd(41), (slong) rnd(7) - 6);
    sb_poke(s, ADF_ARCH_REAL, r, loc, np);
    for (i = 0; i < np; i++)
        adf_lball_clear(&loc[i]);
    arb_clear(r);
    fmpz_clear(m);
}

/* A tuple: one point in each component. tuple[0] the real, tuple[1 + i] at the prime i. */
static void
random_tuple(fmpq_t * t, const adf_sball_t s)
{
    slong i;
    real_point_of(t[0], acb_realref(s->inf));
    for (i = 0; i < s->len; i++)
        point_of(t[1 + i], &s->loc[i], rnd(s->loc[i].p * s->loc[i].p));
}

/* For random pairs of partial balls over {inf, 2, 3, 5, 7}, random tuples of points of each, and each operation, the
   tuple of results (the real number, the rational at each prime) lies in the result component at EVERY place. That
   pairs the components of the two operands correctly (a result that added the component of 3 to that of 5 fails on
   the tuple) and gives the real and the local parts the same operation. */
ADF_TEST(tuples_of_points_through_the_operations)
{
    const ulong primes[4] = {2, 3, 5, 7};
    const char * ops[4] = {"add", "sub", "mul", "neg"};
    int iter, o, k, i;
    unsigned long checks = 0;
    adf_sball_t x, y, z;
    fmpq_t tx[5], ty[5], r;
    adf_lball_t ex;
    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(z);
    adf_lball_init(ex);
    fmpq_init(r);
    for (i = 0; i < 5; i++)
    {
        fmpq_init(tx[i]);
        fmpq_init(ty[i]);
    }
    for (iter = 0; iter < 400; iter++)
    {
        int np = 4;
        random_sball(x, primes, np);
        random_sball(y, primes, np);
        for (o = 0; o < 4; o++)
        {
            int st = call_op(ops[o], z, NULL, x, y, (slong) (20 + rnd(60)));
            ADF_CHECK_MSG(st == ADF_OK, "%s: status %s", ops[o], adf_status_str(st));
            if (st != ADF_OK)
                continue;
            for (k = 0; k < 6; k++)
            {
                random_tuple(tx, x);
                random_tuple(ty, y);
                for (i = 0; i < 5; i++)
                {
                    if (o == 0)
                        fmpq_add(r, tx[i], ty[i]);
                    else if (o == 1)
                        fmpq_sub(r, tx[i], ty[i]);
                    else if (o == 2)
                        fmpq_mul(r, tx[i], ty[i]);
                    else
                        fmpq_neg(r, tx[i]);
                    if (i == 0)
                        ADF_CHECK_MSG(arb_contains_fmpq(acb_realref(z->inf), r), "%s: real coordinate outside", ops[o]);
                    else
                    {
                        adf_rat_t q;
                        adf_rat_init(q);
                        fmpq_set(q->q, r);
                        ADF_CHECK(adf_lball_set_rat(ex, place_of(primes[i - 1]), q) == ADF_OK);
                        ADF_CHECK_MSG(adf_lball_contains(ex, &z->loc[i - 1]), "%s: coordinate at %lu outside", ops[o],
                                      primes[i - 1]);
                        adf_rat_clear(q);
                    }
                    checks++;
                }
            }
        }
    }
    ADF_CHECK(checks > 20000);
    for (i = 0; i < 5; i++)
    {
        fmpq_clear(tx[i]);
        fmpq_clear(ty[i]);
    }
    fmpq_clear(r);
    adf_lball_clear(ex);
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(z);
}

/* ----------------------------------------------------------------------- statuses and the reported place */

/* A partial ball with the given primes (any order not needed: increasing), each component the exact 1, and the real
   ball 1 when arch is not NONE. */
static void
sb_ones(adf_sball_t s, int arch, const ulong * primes, int n)
{
    adf_lball_struct loc[4];
    arb_t r;
    int i;
    arb_init(r);
    arb_one(r);
    for (i = 0; i < n; i++)
    {
        adf_lball_init(&loc[i]);
        lb_exact_of(&loc[i], primes[i], 1, 1);
    }
    sb_poke(s, arch, arch != ADF_ARCH_NONE ? r : NULL, loc, n);
    for (i = 0; i < n; i++)
        adf_lball_clear(&loc[i]);
    arb_clear(r);
}

/* DOMAIN and its place: the first place, in the canonical order (inf, then the primes increasing), that belongs to one
   operand only; UNSUPPORTED for the complex tag; the check of the places comes first. */
ADF_TEST(domain_unsupported_and_where)
{
    const ulong p23[2] = {2, 3}, p25[2] = {2, 5}, p2[1] = {2}, p3[1] = {3}, p235[3] = {2, 3, 5};
    struct
    {
        int ax, ay;
        const ulong * px;
        int nx;
        const ulong * py;
        int ny;
        int want_st;
        long want_place;   /* 0 = inf */
    } rows[] = {
        {1, 1, p23, 2, p25, 2, ADF_DOMAIN, 3},      /* 3 in x only; 5 in y only: 3 first */
        {1, 1, p25, 2, p23, 2, ADF_DOMAIN, 3},      /* the same with x and y swapped */
        {0, 1, p23, 2, p23, 2, ADF_DOMAIN, 0},      /* the archimedean place in y only */
        {1, 0, p23, 2, p23, 2, ADF_DOMAIN, 0},
        {1, 1, p2, 1, p23, 2, ADF_DOMAIN, 3},
        {1, 1, p3, 1, p23, 2, ADF_DOMAIN, 2},
        {1, 1, p3, 1, p2, 1, ADF_DOMAIN, 2},
        {0, 0, p235, 3, p23, 2, ADF_DOMAIN, 5},
        {0, 0, p235, 3, p235, 3, ADF_OK, 0},
        {1, 1, NULL, 0, p2, 1, ADF_DOMAIN, 2},
        {0, 1, NULL, 0, NULL, 0, ADF_DOMAIN, 0},
        {0, 0, NULL, 0, NULL, 0, ADF_OK, 0},
        {1, 2, p23, 2, p23, 2, ADF_DOMAIN, 0},      /* real tag against complex tag */
        {0, 2, p23, 2, p23, 2, ADF_DOMAIN, 0},
        {2, 2, p25, 2, p23, 2, ADF_DOMAIN, 3},      /* two complex balls over different primes: the places first */
        {2, 2, p23, 2, p23, 2, ADF_UNSUPPORTED, 0},
        {2, 2, NULL, 0, NULL, 0, ADF_UNSUPPORTED, 0}};
    const char * ops[3] = {"add", "sub", "mul"};
    size_t i;
    int o;
    adf_sball_t x, y, z, s;
    adf_place_t where, mark = place_of(1000003);
    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(z);
    adf_sball_init(s);
    sb_sentinel(s);
    for (i = 0; i < sizeof rows / sizeof rows[0]; i++)
    {
        sb_ones(x, rows[i].ax, rows[i].px, rows[i].nx);
        sb_ones(y, rows[i].ay, rows[i].py, rows[i].ny);
        for (o = 0; o < 4; o++)
        {
            int st;
            sb_sentinel(z);
            where = mark;
            st = call_op(o < 3 ? ops[o] : "neg", z, &where, x, y, 53);
            if (o == 3)
            {
                /* neg has no second operand: OK, or UNSUPPORTED for the complex tag */
                int want = rows[i].ax == 2 ? ADF_UNSUPPORTED : ADF_OK;
                ADF_CHECK_MSG(st == want, "row %lu neg: %s", (unsigned long) i, adf_status_str(st));
                if (want == ADF_OK)
                    ADF_CHECK(adf_place_equal(where, mark));
                else
                    ADF_CHECK(adf_place_is_archimedean(where) && adf_sball_identical(z, s));
                continue;
            }
            ADF_CHECK_MSG(st == rows[i].want_st, "row %lu %s: %s, want %s", (unsigned long) i, ops[o],
                          adf_status_str(st), adf_status_str(rows[i].want_st));
            if (rows[i].want_st == ADF_OK)
            {
                ADF_CHECK(adf_place_equal(where, mark) && adf_sball_is_canonical(z));
                continue;
            }
            ADF_CHECK_MSG(adf_sball_identical(z, s), "row %lu %s: output written on %s", (unsigned long) i, ops[o],
                          adf_status_str(st));
            ADF_CHECK_MSG(rows[i].want_place == 0 ? adf_place_is_archimedean(where)
                                                  : adf_place_prime_get(where) == (ulong) rows[i].want_place,
                          "row %lu %s: where is %lu", (unsigned long) i, ops[o], adf_place_prime_get(where));
        }
    }
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(z);
    adf_sball_clear(s);
}

/* LIMIT of a component: the place of the first failing prime; the output untouched, also aliased. The forged
   components are canonical (u = 1, v = 2^30, exact: an exact power of 5 whose sum with 1 needs 5^(2^30); a ball
   1 + O(3^(2^40)) whose negation needs the residue of -1). */
ADF_TEST(limit_names_the_first_failing_prime)
{
    adf_sball_t x, y, z, s, one;
    adf_lball_struct loc[3], loc1[3];
    arb_t r;
    adf_place_t where, mark = place_of(1000003);
    int st, i, o;
    const char * ops[3] = {"add", "sub", "mul"};

    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(z);
    adf_sball_init(s);
    adf_sball_init(one);
    arb_init(r);
    arb_one(r);
    for (i = 0; i < 3; i++)
    {
        adf_lball_init(&loc[i]);
        adf_lball_init(&loc1[i]);
    }
    sb_sentinel(s);

    /* exact 5^(2^30) and 7^(2^30) against exact 1: the sum is LIMIT at the primes where the power is needed */
    lb_exact_of(&loc[0], 3, 1, 1);
    lb_fields_si(&loc[1], 5, 1, 1, 1, 1L << 30, 0);
    lb_fields_si(&loc[2], 7, 1, 1, 1, 1L << 30, 0);
    ADF_CHECK(adf_lball_is_canonical(&loc[1]) && adf_lball_is_canonical(&loc[2]));
    lb_exact_of(&loc1[0], 3, 1, 1);
    lb_exact_of(&loc1[1], 5, 1, 1);
    lb_exact_of(&loc1[2], 7, 1, 1);
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);
    sb_poke(y, ADF_ARCH_REAL, r, loc1, 3);
    for (o = 0; o < 2; o++)   /* add, sub */
    {
        sb_sentinel(z);
        where = mark;
        st = call_op(ops[o], z, &where, x, y, 53);
        ADF_CHECK_MSG(st == ADF_LIMIT && adf_place_prime_get(where) == 5 && adf_sball_identical(z, s), "%s", ops[o]);
        /* aliased: x is unchanged */
        adf_sball_set(one, x);
        where = mark;
        st = call_op(ops[o], one, &where, one, y, 53);
        ADF_CHECK(st == ADF_LIMIT && adf_place_prime_get(where) == 5 && adf_sball_identical(one, x));
        adf_sball_set(one, y);
        where = mark;
        st = call_op(ops[o], one, &where, x, one, 53);
        ADF_CHECK(st == ADF_LIMIT && adf_place_prime_get(where) == 5 && adf_sball_identical(one, y));
    }
    /* only 7 fails */
    lb_exact_of(&loc[1], 5, 1, 1);
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);
    sb_sentinel(z);
    where = mark;
    st = adf_sball_add(z, &where, x, y, 53);
    ADF_CHECK(st == ADF_LIMIT && adf_place_prime_get(where) == 7 && adf_sball_identical(z, s));
    /* NULL where is fine */
    ADF_CHECK(adf_sball_add(z, NULL, x, y, 53) == ADF_LIMIT && adf_sball_identical(z, s));
    /* nothing fails */
    lb_exact_of(&loc[2], 7, 1, 1);
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);
    where = mark;
    ADF_CHECK(adf_sball_add(z, &where, x, y, 53) == ADF_OK && adf_place_equal(where, mark));

    /* neg and mul: the ball 1 + O(3^(2^40)), its negation needs the residue of -1 modulo 3^(2^40) */
    lb_fields_si(&loc[0], 3, 0, 1, 1, 0, 1L << 40);
    ADF_CHECK(adf_lball_is_canonical(&loc[0]));
    lb_ball_of(&loc[1], 5, 1, 1, 3);
    lb_fields_si(&loc[2], 11, 0, 1, 1, 0, 1L << 40);
    ADF_CHECK(adf_lball_is_canonical(&loc[2]));
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);
    sb_sentinel(z);
    where = mark;
    st = adf_sball_neg(z, &where, x);
    ADF_CHECK(st == ADF_LIMIT && adf_place_prime_get(where) == 3 && adf_sball_identical(z, s));
    adf_sball_set(one, x);
    where = mark;
    ADF_CHECK(adf_sball_neg(one, &where, one) == ADF_LIMIT && adf_sball_identical(one, x));
    lb_ball_of(&loc[0], 3, 1, 1, 3);
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);   /* only 11 fails */
    where = mark;
    ADF_CHECK(adf_sball_neg(z, &where, x) == ADF_LIMIT && adf_place_prime_get(where) == 11 && adf_sball_identical(z, s));
    /* mul: (1 + O(3^(2^40))) times the exact -1 */
    lb_fields_si(&loc[0], 3, 0, 1, 1, 0, 1L << 40);
    lb_exact_of(&loc[1], 5, 1, 1);
    lb_exact_of(&loc[2], 7, 1, 1);
    sb_poke(x, ADF_ARCH_REAL, r, loc, 3);
    lb_exact_of(&loc1[0], 3, -1, 1);
    lb_exact_of(&loc1[1], 5, -1, 1);
    lb_exact_of(&loc1[2], 7, -1, 1);
    sb_poke(y, ADF_ARCH_REAL, r, loc1, 3);
    where = mark;
    st = adf_sball_mul(z, &where, x, y, 53);
    ADF_CHECK(st == ADF_LIMIT && adf_place_prime_get(where) == 3 && adf_sball_identical(z, s));
    where = mark;
    ADF_CHECK(adf_sball_mul(one, &where, x, y, 53) == ADF_LIMIT);

    for (i = 0; i < 3; i++)
    {
        adf_lball_clear(&loc[i]);
        adf_lball_clear(&loc1[i]);
    }
    arb_clear(r);
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(z);
    adf_sball_clear(s);
    adf_sball_clear(one);
}

/* --------------------------------------------------------------------------- the example of the brief, and use */

/* A user takes the adele of 2/3, projects it to {2, 5, real}, reads the three components, multiplies the projection
   by itself and gets the projection of 4/9; the components are checked against values computed here from the definition
   (2/3 = 2^1 (1/3) at 2; unit at 5). */
ADF_TEST(the_example_of_the_brief)
{
    adf_adele_t a, b;
    adf_sball_t s, t, u;
    adf_place_t ps[3], inf = adf_place_inf();
    adf_lball_t c;
    adf_rat_t q, w;
    arb_t r;
    fmpz_t n, d;
    fmpq_t third;
    adf_place_t where;
    int st;

    adf_adele_init(a);
    adf_adele_init(b);
    adf_sball_init(s);
    adf_sball_init(t);
    adf_sball_init(u);
    adf_lball_init(c);
    adf_rat_init(q);
    adf_rat_init(w);
    arb_init(r);
    fmpz_init_set_si(n, 2);
    fmpz_init_set_si(d, 3);
    fmpq_init(third);
    ADF_CHECK(adf_rat_set_fmpz2(q, n, d) == ADF_OK);
    adf_adele_set_rat(a, q, 200);
    ps[0] = place_of(2);
    ps[1] = place_of(5);
    ps[2] = inf;
    st = adf_sball_project(s, &where, a, ps, 3);
    ADF_CHECK(st == ADF_OK && adf_sball_is_canonical(s) && adf_sball_num_places(s) == 3);
    /* at 2: exact 2/3 = 2^1 (1/3): u = 1/3, v = 1 */
    ADF_CHECK(adf_sball_get_lball(c, s, ps[0]) == ADF_OK && c->exact && c->v == 1 && c->p == 2);
    fmpq_set_si(third, 1, 3);
    ADF_CHECK(fmpq_equal(c->u, third));
    /* at 5: exact 2/3, unit, v = 0 */
    ADF_CHECK(adf_sball_get_lball(c, s, ps[1]) == ADF_OK && c->exact && c->v == 0 && c->p == 5);
    fmpq_set_si(third, 2, 3);
    ADF_CHECK(fmpq_equal(c->u, third));
    /* the real place: a ball around 2/3 at 200 bits */
    ADF_CHECK(adf_sball_get_arb(r, s, inf) == ADF_OK && arb_contains_fmpq(r, third) &&
              mag_cmp_2exp_si(arb_radref(r), -195) < 0);
    /* the product of the projection with itself is the projection of 4/9 (the components are exact rationals) */
    ADF_CHECK(adf_sball_mul(t, &where, s, s, 200) == ADF_OK);
    fmpz_set_si(n, 4);
    fmpz_set_si(d, 9);
    ADF_CHECK(adf_rat_set_fmpz2(w, n, d) == ADF_OK);
    adf_adele_set_rat(b, w, 200);
    ADF_CHECK(adf_sball_project(u, &where, b, ps, 3) == ADF_OK);
    ADF_CHECK(t->len == 2 && u->len == 2 && adf_lball_identical(&t->loc[0], &u->loc[0]) &&
              adf_lball_identical(&t->loc[1], &u->loc[1]));
    fmpq_set_si(third, 4, 9);
    ADF_CHECK(arb_contains_fmpq(acb_realref(t->inf), third));
    ADF_CHECK(arb_overlaps(acb_realref(t->inf), acb_realref(u->inf)));
    /* the sum with the negation is the exact 0 at the primes and a ball around 0 at the real place */
    ADF_CHECK(adf_sball_neg(u, &where, s) == ADF_OK && adf_sball_add(t, &where, s, u, 200) == ADF_OK);
    ADF_CHECK(t->len == 2 && t->loc[0].exact && fmpq_is_zero(t->loc[0].u) && t->loc[1].exact &&
              fmpq_is_zero(t->loc[1].u));
    ADF_CHECK(arb_contains_zero(acb_realref(t->inf)));

    fmpq_clear(third);
    fmpz_clear(n);
    fmpz_clear(d);
    arb_clear(r);
    adf_rat_clear(q);
    adf_rat_clear(w);
    adf_lball_clear(c);
    adf_sball_clear(s);
    adf_sball_clear(t);
    adf_sball_clear(u);
    adf_adele_clear(a);
    adf_adele_clear(b);
}
