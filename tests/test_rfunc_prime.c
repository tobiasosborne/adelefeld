/* tests/test_rfunc_prime.c: exp_at, log_at, Log_at at a PRIME of a partial ball (lane f-slice6;
   include/adelefeld/rfunc.h, "A PRIME"; include/adelefeld/lfunc.h; docs/api-1f.md, section "Slice 1F.4-b").

   Oracles.
   1. tests/ref/vectors/f-slice6/at_prime.jsonl, made by lanes/f-slice6/gen_vectors.py with exact Fractions only (no
      line of src/lfunc.c, no FLINT): exp, log, Log of an exact rational at p = 2, 3, 5, 7, N = 1, 4, 9, 16. Every
      line is run through the _at function on a partial ball {real, p, q}: the status, the reported place, and on OK
      the exactness, the exponent K and the centre; the state of the output on a status; the same aliased.
   2. tests/ref/vectors/f-slice4/lfunc_cases.jsonl (the vectors of the lfunc.h functions: exact inputs and balls,
      statuses): the result of _at must have the FIELDS of the result of the lfunc.h function on the component.
   3. Hand tests: every status of lfunc.h arrives with where = the prime; the functions not yet at a prime are
      UNSUPPORTED with where = the prime; prec is the absolute precision at a prime (no ADF_REAL_PREC_MAX); Log_at at
      the real place; an adele projected to {2, 3, 5, real}; NULL where; aliasing.

   What would make a case fail is stated at each test. */

#include <limits.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/arb.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include "support/jsonl.h"
#include "test_runner.h"

#define EMAX ADF_LBALL_EXP_MAX

typedef int (*at_fn)(adf_sball_t, adf_place_t *, const adf_sball_t, adf_place_t, slong);
typedef int (*lf_fn)(adf_lball_t, const adf_lball_t, slong);

/* ---------------------------------------------------------------------------------------------------- helpers */

static adf_place_t
place_of(ulong p)
{
    adf_place_t v;
    int st = adf_place_prime(&v, p);
    ADF_CHECK(st == ADF_OK);
    return v;
}

static at_fn
at_of(const char * name)
{
    if (strcmp(name, "exp") == 0)
        return adf_sball_exp_at;
    if (strcmp(name, "log") == 0)
        return adf_sball_log_at;
    ADF_CHECK_MSG(strcmp(name, "Log") == 0, "unknown function %s", name);
    return adf_sball_Log_at;
}

static lf_fn
lf_of(const char * name)
{
    if (strcmp(name, "exp") == 0)
        return adf_lball_exp;
    if (strcmp(name, "log") == 0)
        return adf_lball_log;
    ADF_CHECK_MSG(strcmp(name, "Log") == 0, "unknown function %s", name);
    return adf_lball_Log;
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

static ulong
member_ulong(const jsonl_value * rec, const char * key)
{
    return strtoul(member_int(rec, key), NULL, 10);
}

static jsonl_file *
open_vectors(const char * path)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    ADF_CHECK_MSG(jsonl_open(path, &f, &err), "%s", jsonl_error_message(&err));
    return f;
}

static void
rat_of(adf_rat_t r, slong num, slong den)
{
    fmpz_t n, d;
    fmpz_init_set_si(n, num);
    fmpz_init_set_si(d, den);
    ADF_CHECK(adf_rat_set_fmpz2(r, n, d) == ADF_OK);
    fmpz_clear(n);
    fmpz_clear(d);
}

/* x = the exact rational num/den at p. */
static void
lb_exact(adf_lball_t x, ulong p, slong num, slong den)
{
    adf_rat_t r;
    adf_rat_init(r);
    rat_of(r, num, den);
    ADF_CHECK(adf_lball_set_rat(x, place_of(p), r) == ADF_OK);
    adf_rat_clear(r);
}

/* x = the ball num/den + p^N Z_p. */
static void
lb_ball(adf_lball_t x, ulong p, slong num, slong den, slong N)
{
    adf_rat_t r;
    adf_rat_init(r);
    rat_of(r, num, den);
    ADF_CHECK(adf_lball_set_rat_ball(x, place_of(p), r, N) == ADF_OK);
    adf_rat_clear(r);
}

/* x = the raw fields (must be canonical). */
static void
lb_raw(adf_lball_t x, ulong p, int exact, slong un, slong v, slong N)
{
    x->p = p;
    fmpz_set_si(fmpq_numref(x->u), un);
    fmpz_one(fmpq_denref(x->u));
    x->v = v;
    x->N = N;
    x->exact = exact;
    ADF_CHECK(adf_lball_is_canonical(x));
}

static int
fields_equal(const adf_lball_t x, const adf_lball_t y)
{
    return x->p == y->p && x->exact == y->exact && x->v == y->v && x->N == y->N && fmpq_equal(x->u, y->u);
}

/* The partial ball with the real component r (r > 0 exact, or none if r_si == 0) and the components loc[0..n-1]. */
static void
make_sball(adf_sball_t s, slong r_si, const adf_lball_struct * loc, slong n)
{
    arb_t r;
    arb_init(r);
    arb_set_si(r, r_si);
    ADF_CHECK(adf_sball_set_arb_lballs(s, NULL, r_si != 0 ? r : NULL, loc, n) == ADF_OK);
    arb_clear(r);
}

/* A sentinel output: the real component 12345 alone. */
static void
sentinel(adf_sball_t y)
{
    adf_sball_t t;
    adf_sball_init(t);
    make_sball(t, 12345, NULL, 0);
    adf_sball_swap(y, t);
    adf_sball_clear(t);
}

static int
is_sentinel(const adf_sball_t y)
{
    adf_sball_t t;
    int r;
    adf_sball_init(t);
    sentinel(t);
    r = adf_sball_identical(y, t);
    adf_sball_clear(t);
    return r;
}

/* y is a partial ball over the ONE prime p of the shape the header promises, with the component c. */
static int
is_one_place(const adf_sball_t y, ulong p, const adf_lball_t c)
{
    return adf_sball_is_canonical(y) && y->arch == ADF_ARCH_NONE && y->len == 1 && adf_sball_num_places(y) == 1 &&
           arb_is_zero(acb_realref(y->inf)) && arb_is_zero(acb_imagref(y->inf)) && y->loc[0].p == p &&
           fields_equal(&y->loc[0], c);
}

/* -------------------------------------------------------------------------------- 1. the vectors, exact inputs */

/* Every line of at_prime.jsonl on the partial ball {real 7, p, q} (q another prime, its component 3): status; where =
   the place p on a status other than OK and untouched on OK; the output untouched on a status; on OK one place, the
   exactness, the exponent and the centre of the reference; the result is the one of adf_lball_* (identical fields);
   the same aliased. Fails on: a wrong function, a where that is not the prime, a written output, a result that has
   the other components, an exponent other than N (or 0 for an exact result), a wrong centre. */
ADF_TEST(vectors_exact_at_prime)
{
    jsonl_file * f = open_vectors("tests/ref/vectors/f-slice6/at_prime.jsonl");
    size_t i, n = f ? jsonl_count(f) : 0;
    ulong counts[ADF_STATUS_COUNT] = {0}, exacts = 0;
    adf_lball_t c[2], want;
    adf_sball_t x, y, z;
    adf_place_t where, mark = place_of(1000003);
    fmpq_t centre;

    adf_lball_init(c[0]);
    adf_lball_init(c[1]);
    adf_lball_init(want);
    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(z);
    fmpq_init(centre);
    for (i = 0; i < n; i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * fname = member_str(rec, "f");
        ulong p = member_ulong(rec, "p"), q = (p == 3) ? 5 : 3;
        slong N = member_slong(rec, "N");
        int want_st = status_from_name(member_str(rec, "status")), st, st2;
        fmpq_t xq;

        fmpq_init(xq);
        ADF_CHECK(fmpz_set_str(fmpq_numref(xq), member_int(rec, "xn"), 10) == 0);
        ADF_CHECK(fmpz_set_str(fmpq_denref(xq), member_int(rec, "xd"), 10) == 0);
        fmpq_canonicalise(xq);
        {
            adf_rat_t r;
            adf_rat_init(r);
            fmpq_set(r->q, xq);
            ADF_CHECK(adf_lball_set_rat(&c[0][0], place_of(p), r) == ADF_OK);
            adf_rat_clear(r);
        }
        lb_exact(c[1], q, 3, 1);
        make_sball(x, 7, (adf_lball_struct[]){c[0][0], c[1][0]}, 2);
        sentinel(y);
        where = mark;
        st = at_of(fname)(y, &where, x, place_of(p), N);
        ADF_CHECK_MSG(st == want_st, "line %lu (%s at %lu): status %s, want %s", (unsigned long) i + 1, fname,
                      (unsigned long) p, adf_status_str(st), adf_status_str(want_st));
        counts[want_st >= 0 && want_st < ADF_STATUS_COUNT ? want_st : 0]++;
        if (st == ADF_OK && want_st == ADF_OK)
        {
            int ex = (int) member_slong(rec, "exact");
            slong K = member_slong(rec, "K");
            const char * cs = member_str(rec, "c");
            ADF_CHECK_MSG(adf_place_equal(where, mark), "line %lu: where written on OK", (unsigned long) i + 1);
            ADF_CHECK_MSG(adf_sball_is_canonical(y) && y->arch == ADF_ARCH_NONE && y->len == 1 &&
                              y->loc[0].p == p && y->loc[0].exact == ex,
                          "line %lu (%s at %lu): the shape of the result", (unsigned long) i + 1, fname,
                          (unsigned long) p);
            if (ex)
                exacts++;
            else
                ADF_CHECK_MSG(y->loc[0].N == K, "line %lu: exponent %ld, want %ld", (unsigned long) i + 1,
                              (long) y->loc[0].N, (long) K);
            {
                adf_rat_t cr;
                adf_rat_init(cr);
                ADF_CHECK(adf_lball_get_center(cr, &y->loc[0]) == ADF_OK);
                ADF_CHECK(fmpz_set_str(fmpq_numref(centre), cs, 10) == 0);
                fmpz_one(fmpq_denref(centre));
                ADF_CHECK_MSG(fmpq_equal(cr->q, centre), "line %lu (%s(%ld/%s) at %lu, N %ld): centre differs",
                              (unsigned long) i + 1, fname, (long) strtol(member_int(rec, "xn"), NULL, 10),
                              member_int(rec, "xd"), (unsigned long) p, (long) N);
                adf_rat_clear(cr);
            }
            /* the function of lfunc.h on the component gives the same fields */
            ADF_CHECK(lf_of(fname)(want, &c[0][0], N) == ADF_OK);
            ADF_CHECK_MSG(is_one_place(y, p, want), "line %lu: not the result of lfunc.h", (unsigned long) i + 1);
        }
        else
        {
            ADF_CHECK_MSG(adf_place_equal(where, place_of(p)), "line %lu: where is not the prime", (unsigned long) i + 1);
            ADF_CHECK_MSG(is_sentinel(y), "line %lu: output written on status %s", (unsigned long) i + 1,
                          adf_status_str(st));
        }
        /* aliased: z = x */
        adf_sball_set(z, x);
        where = mark;
        st2 = at_of(fname)(z, &where, z, place_of(p), N);
        ADF_CHECK(st2 == st);
        if (st == ADF_OK)
            ADF_CHECK_MSG(adf_sball_identical(z, y), "line %lu: aliased result differs", (unsigned long) i + 1);
        else
            ADF_CHECK_MSG(adf_sball_identical(z, x), "line %lu: aliased input changed on a status",
                          (unsigned long) i + 1);
        fmpq_clear(xq);
    }
    ADF_CHECK(n >= 1800);
    ADF_CHECK(counts[ADF_OK] > 500 && counts[ADF_DOMAIN] > 500 && exacts > 50);
    printf("  vectors_exact_at_prime: %lu lines, OK %lu (exact %lu), DOMAIN %lu\n", (unsigned long) n, counts[ADF_OK],
           exacts, counts[ADF_DOMAIN]);
    fmpq_clear(centre);
    adf_lball_clear(c[0]);
    adf_lball_clear(c[1]);
    adf_lball_clear(want);
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(z);
    jsonl_close(f);
}

/* ------------------------------------------------------------- 2. the vectors of lfunc.h: balls and every status */

static void
lb_from_json(adf_lball_t x, const jsonl_value * o)
{
    fmpz_t un, ud;
    fmpz_init(un);
    fmpz_init(ud);
    ADF_CHECK(fmpz_set_str(un, member_int(o, "un"), 10) == 0);
    ADF_CHECK(fmpz_set_str(ud, member_int(o, "ud"), 10) == 0);
    x->p = member_ulong(o, "p");
    fmpz_set(fmpq_numref(x->u), un);
    fmpz_set(fmpq_denref(x->u), ud);
    x->v = member_slong(o, "v");
    x->N = member_slong(o, "N");
    x->exact = (int) member_slong(o, "exact");
    ADF_CHECK(adf_lball_is_canonical(x));
    fmpz_clear(un);
    fmpz_clear(ud);
}

/* Every line of lfunc_cases.jsonl (balls, exact inputs, every status of lfunc.h, limits) through _at, on the partial
   ball {p} alone: the status equals that of the lfunc.h function; on OK the component of the result has the fields of
   the reference result; on another status where = the prime and the output is untouched. Fails on any difference. */
ADF_TEST(vectors_of_lfunc_through_at)
{
    jsonl_file * f = open_vectors("tests/ref/vectors/f-slice4/lfunc_cases.jsonl");
    size_t i, n = f ? jsonl_count(f) : 0;
    ulong counts[ADF_STATUS_COUNT] = {0};
    adf_lball_t c, want;
    adf_sball_t x, y;
    adf_place_t where, mark = place_of(1000003);

    adf_lball_init(c);
    adf_lball_init(want);
    adf_sball_init(x);
    adf_sball_init(y);
    for (i = 0; i < n; i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * fname = member_str(rec, "f");
        slong N = member_slong(rec, "N");
        int want_st = status_from_name(member_str(rec, "status")), st;

        lb_from_json(c, member(rec, "x"));
        make_sball(x, 0, c, 1);
        sentinel(y);
        where = mark;
        st = at_of(fname)(y, &where, x, place_of(c->p), N);
        ADF_CHECK_MSG(st == want_st, "line %lu (%s, %s): status %s, want %s", (unsigned long) i + 1, fname,
                      member_str(rec, "note"), adf_status_str(st), adf_status_str(want_st));
        if (want_st >= 0 && want_st < ADF_STATUS_COUNT)
            counts[want_st]++;
        if (st == ADF_OK && want_st == ADF_OK)
        {
            lb_from_json(want, member(rec, "result"));
            ADF_CHECK_MSG(is_one_place(y, c->p, want) && adf_place_equal(where, mark), "line %lu (%s, %s): result",
                          (unsigned long) i + 1, fname, member_str(rec, "note"));
        }
        else
            ADF_CHECK_MSG(adf_place_equal(where, place_of(c->p)) && is_sentinel(y), "line %lu (%s): where or output",
                          (unsigned long) i + 1, member_str(rec, "note"));
    }
    ADF_CHECK(n >= 700);
    ADF_CHECK(counts[ADF_OK] > 0 && counts[ADF_DOMAIN] > 0 && counts[ADF_NOT_DETERMINED] > 0);
    printf("  vectors_of_lfunc_through_at: %lu lines, OK %lu, DOMAIN %lu, NOT_DETERMINED %lu, LIMIT %lu\n",
           (unsigned long) n, counts[ADF_OK], counts[ADF_DOMAIN], counts[ADF_NOT_DETERMINED], counts[ADF_LIMIT]);
    adf_lball_clear(c);
    adf_lball_clear(want);
    adf_sball_clear(x);
    adf_sball_clear(y);
    jsonl_close(f);
}

/* ------------------------------------------------------------------------------------------------ 3. hand tests */

/* One status per line, built by hand: DOMAIN, NOT_DETERMINED and LIMIT of lfunc.h arrive with where = the prime and
   leave y untouched; NULL where is allowed. A wrapper that reports the archimedean place, or writes y, fails. */
ADF_TEST(statuses_arrive_with_the_prime)
{
    adf_lball_t c;
    adf_sball_t x, y;
    adf_place_t where, mark = place_of(1000003), five = place_of(5), two = place_of(2);
    struct
    {
        const char * name;
        ulong p;
        int kind;      /* 0 exact num/den, 1 ball num/den + p^N, 2 raw ball of exponent N above the bound */
        slong num, den, N, req;
        int want;
    } rows[] = {
        {"exp", 5, 0, 1, 1, 0, 8, ADF_DOMAIN},                /* v(1) = 0 < 1 */
        {"exp", 5, 0, 1, 5, 0, 8, ADF_DOMAIN},                /* v(1/5) = -1 */
        {"exp", 2, 0, 2, 1, 0, 8, ADF_DOMAIN},                /* v_2(2) = 1 < 2: exp needs 4 Z_2 */
        {"exp", 5, 1, 0, 1, 0, 8, ADF_NOT_DETERMINED},        /* the ball O(5^0) meets 5 Z_5 and its complement */
        {"exp", 5, 1, 3, 1, 1, 8, ADF_DOMAIN},                /* 3 + 5 Z_5: a unit, misses 5 Z_5 */
        {"exp", 5, 0, 5, 1, 0, LONG_MAX, ADF_LIMIT},          /* K = N above the bound */
        {"exp", 5, 2, 0, 1, EMAX + 1, 8, ADF_LIMIT},          /* an input exponent above the bound */
        {"log", 5, 0, 5, 1, 0, 8, ADF_DOMAIN},               /* v(5 - 1) = 0 */
        {"log", 5, 0, 0, 1, 0, 8, ADF_DOMAIN},               /* the exact 0 */
        {"log", 5, 1, 1, 1, 0, 8, ADF_NOT_DETERMINED},       /* 1 + O(5^0) contains 1 and points outside 1 + 5 Z_5 */
        {"log", 5, 1, 2, 1, 1, 8, ADF_DOMAIN},               /* 2 + 5 Z_5 misses 1 + 5 Z_5 */
        {"log", 5, 0, 6, 1, 0, LONG_MAX, ADF_LIMIT},
        {"Log", 5, 0, 0, 1, 0, 8, ADF_DOMAIN},               /* the exact 0 */
        {"Log", 5, 1, 0, 1, 3, 8, ADF_NOT_DETERMINED},       /* O(5^3) contains 0 */
        {"Log", 2, 1, 0, 1, 1, 8, ADF_NOT_DETERMINED},
        {"Log", 5, 2, 0, 1, EMAX + 1, 8, ADF_LIMIT},
    };
    size_t k;

    adf_lball_init(c);
    adf_sball_init(x);
    adf_sball_init(y);
    for (k = 0; k < sizeof rows / sizeof rows[0]; k++)
    {
        int st;
        if (rows[k].kind == 0)
            lb_exact(c, rows[k].p, rows[k].num, rows[k].den);
        else if (rows[k].kind == 1)
            lb_ball(c, rows[k].p, rows[k].num, rows[k].den, rows[k].N);
        else
            lb_raw(c, rows[k].p, 0, 0, 0, rows[k].N);
        make_sball(x, 3, c, 1);
        sentinel(y);
        where = mark;
        st = at_of(rows[k].name)(y, &where, x, place_of(rows[k].p), rows[k].req);
        ADF_CHECK_MSG(st == rows[k].want && adf_place_equal(where, place_of(rows[k].p)) && is_sentinel(y),
                      "row %lu (%s at %lu): status %s, want %s", (unsigned long) k, rows[k].name,
                      (unsigned long) rows[k].p, adf_status_str(st), adf_status_str(rows[k].want));
        ADF_CHECK(at_of(rows[k].name)(y, NULL, x, place_of(rows[k].p), rows[k].req) == rows[k].want);
    }
    (void) five;
    (void) two;
    adf_lball_clear(c);
    adf_sball_clear(x);
    adf_sball_clear(y);
}

/* prec at a prime is the absolute precision N, not a number of bits: (a) no ADF_REAL_PREC_MAX (the exact results
   exp(0) = 1, log(1) = 0, Log(p) = 0 are returned at prec = ADF_REAL_PREC_MAX + 1 and at LONG_MAX, where the real place
   says LIMIT); (b) no rounding up of a small value: N = 0, -3, -1000000 give the same fields as adf_lball_* with that N;
   (c) N = 1: exp(5) = 1 + O(5). A wrapper that clamps prec to [2, REAL_PREC_MAX] fails (a) or (b). */
ADF_TEST(prec_is_the_absolute_precision_at_a_prime)
{
    const slong big[3] = {ADF_REAL_PREC_MAX + 1, ADF_REAL_PREC_MAX + 2, LONG_MAX};
    const slong small[6] = {1, 0, -1, -3, -1000000, LONG_MIN + 1};
    adf_lball_t c, want;
    adf_sball_t x, y;
    adf_place_t where, mark = place_of(1000003), inf = adf_place_inf();
    int i, j, st, ls;

    adf_lball_init(c);
    adf_lball_init(want);
    adf_sball_init(x);
    adf_sball_init(y);
    /* (a) the exact results, and the real place for contrast */
    for (j = 0; j < 3; j++)
    {
        lb_exact(c, 5, 0, 1);
        make_sball(x, 1, c, 1);
        sentinel(y);
        where = mark;
        ADF_CHECK(adf_sball_exp_at(y, &where, x, place_of(5), big[j]) == ADF_OK && adf_place_equal(where, mark));
        ADF_CHECK(y->len == 1 && y->loc[0].exact == 1 && fmpq_is_one(y->loc[0].u) && y->loc[0].v == 0);
        ADF_CHECK(adf_sball_exp_at(y, &where, x, inf, big[j]) == ADF_LIMIT && adf_place_is_archimedean(where));
        lb_exact(c, 5, 1, 1);
        make_sball(x, 1, c, 1);
        ADF_CHECK(adf_sball_log_at(y, &where, x, place_of(5), big[j]) == ADF_OK && y->len == 1 &&
                  y->loc[0].exact == 1 && fmpq_is_zero(y->loc[0].u));
        lb_exact(c, 5, 5, 1);
        make_sball(x, 1, c, 1);
        ADF_CHECK(adf_sball_Log_at(y, &where, x, place_of(5), big[j]) == ADF_OK && y->len == 1 &&
                  y->loc[0].exact == 1 && fmpq_is_zero(y->loc[0].u));
        /* a request that does not exist at a prime: UNSUPPORTED, not LIMIT */
        ADF_CHECK(adf_sball_log_abs_at(y, &where, x, place_of(5), big[j]) == ADF_UNSUPPORTED &&
                  adf_place_equal(where, place_of(5)));
    }
    /* (b) small and negative N */
    lb_exact(c, 5, 5, 1);
    make_sball(x, 0, c, 1);
    for (i = 0; i < 6; i++)
    {
        ls = adf_lball_exp(want, c, small[i]);
        sentinel(y);
        st = adf_sball_exp_at(y, &where, x, place_of(5), small[i]);
        ADF_CHECK_MSG(st == ls, "exp N=%ld: status %s, lfunc %s", (long) small[i], adf_status_str(st),
                      adf_status_str(ls));
        if (st == ADF_OK)
            ADF_CHECK_MSG(is_one_place(y, 5, want), "exp N=%ld: fields", (long) small[i]);
    }
    lb_exact(c, 5, 6, 1);
    make_sball(x, 0, c, 1);
    for (i = 0; i < 6; i++)
    {
        ls = adf_lball_log(want, c, small[i]);
        st = adf_sball_log_at(y, &where, x, place_of(5), small[i]);
        ADF_CHECK_MSG(st == ls && (st != ADF_OK || is_one_place(y, 5, want)), "log N=%ld", (long) small[i]);
        ls = adf_lball_Log(want, c, small[i]);
        st = adf_sball_Log_at(y, &where, x, place_of(5), small[i]);
        ADF_CHECK_MSG(st == ls && (st != ADF_OK || is_one_place(y, 5, want)), "Log N=%ld", (long) small[i]);
    }
    /* (c) N = 1: exp(5) = 1 + O(5) */
    lb_exact(c, 5, 5, 1);
    make_sball(x, 0, c, 1);
    ADF_CHECK(adf_sball_exp_at(y, &where, x, place_of(5), 1) == ADF_OK);
    ADF_CHECK(y->len == 1 && y->loc[0].exact == 0 && y->loc[0].N == 1 && fmpz_is_one(fmpq_numref(y->loc[0].u)) &&
              y->loc[0].v == 0);
    adf_lball_clear(c);
    adf_lball_clear(want);
    adf_sball_clear(x);
    adf_sball_clear(y);
}

/* The functions that do not exist at a prime yet: UNSUPPORTED with where = the prime, y untouched; a prime that is not
   a place of x is DOMAIN first; the COMPLEX tag concerns the archimedean place only, so exp at a prime of a COMPLEX
   sball is computed. A wrapper that returns OK, LIMIT or the archimedean place fails. */
ADF_TEST(other_functions_and_place_checks)
{
    adf_lball_t c[2], want;
    adf_sball_t x, y, w;
    adf_place_t where, mark = place_of(1000003), inf = adf_place_inf();
    struct
    {
        const char * name;
        int (*g)(adf_sball_t, adf_place_t *, const adf_sball_t, adf_place_t, slong);
    } others[] = {{"log_abs", adf_sball_log_abs_at}, {"sqrt", adf_sball_sqrt_at}};
    at_fn three[3] = {adf_sball_exp_at, adf_sball_log_at, adf_sball_Log_at};
    size_t k;
    int st;

    adf_lball_init(c[0]);
    adf_lball_init(c[1]);
    adf_lball_init(want);
    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(w);
    lb_exact(c[0], 5, 5, 1);
    lb_exact(c[1], 2, 4, 1);
    make_sball(x, 2, (adf_lball_struct[]){c[0][0], c[1][0]}, 2);
    for (k = 0; k < sizeof others / sizeof others[0]; k++)
    {
        sentinel(y);
        where = mark;
        st = others[k].g(y, &where, x, place_of(5), 8);
        ADF_CHECK_MSG(st == ADF_UNSUPPORTED && adf_place_equal(where, place_of(5)) && is_sentinel(y), "%s at 5",
                      others[k].name);
        where = mark;
        st = others[k].g(y, &where, x, place_of(2), 8);
        ADF_CHECK_MSG(st == ADF_UNSUPPORTED && adf_place_equal(where, place_of(2)) && is_sentinel(y), "%s at 2",
                      others[k].name);
        ADF_CHECK(others[k].g(y, NULL, x, place_of(2), 8) == ADF_UNSUPPORTED);
        where = mark;
        st = others[k].g(y, &where, x, place_of(3), 8);   /* 3 is not a place of x */
        ADF_CHECK_MSG(st == ADF_DOMAIN && adf_place_equal(where, place_of(3)) && is_sentinel(y), "%s at 3",
                      others[k].name);
    }
    sentinel(y);
    where = mark;
    ADF_CHECK(adf_sball_root_at(y, &where, x, place_of(5), 3, 8) == ADF_UNSUPPORTED &&
              adf_place_equal(where, place_of(5)) && is_sentinel(y));
    where = mark;
    /* a root of degree 0 at a prime: the prime is unsupported first (the order of the header) */
    ADF_CHECK(adf_sball_root_at(y, &where, x, place_of(5), 0, 8) == ADF_UNSUPPORTED &&
              adf_place_equal(where, place_of(5)));
    for (k = 0; k < 3; k++)
    {
        sentinel(y);
        where = mark;
        st = three[k](y, &where, x, place_of(3), 8);
        ADF_CHECK(st == ADF_DOMAIN && adf_place_equal(where, place_of(3)) && is_sentinel(y));
        /* the prime of x is a place; the archimedean place holds 2 */
    }
    /* an sball without the real place: the archimedean place is not a place of x; a prime is unaffected */
    make_sball(w, 0, c[0], 1);
    sentinel(y);
    where = mark;
    ADF_CHECK(adf_sball_exp_at(y, &where, w, inf, 8) == ADF_DOMAIN && adf_place_is_archimedean(where) &&
              is_sentinel(y));
    ADF_CHECK(adf_sball_exp_at(y, &where, w, place_of(5), 8) == ADF_OK);

    /* COMPLEX tag: a prime is computed, the archimedean place is UNSUPPORTED */
    adf_sball_set(w, x);
    w->arch = ADF_ARCH_COMPLEX;
    arb_set_si(acb_imagref(w->inf), 1);
    ADF_CHECK(adf_sball_is_canonical(w));
    where = mark;
    ADF_CHECK(adf_sball_exp_at(y, &where, w, place_of(5), 8) == ADF_OK && adf_place_equal(where, mark) &&
              y->len == 1 && y->arch == ADF_ARCH_NONE);
    ADF_CHECK(adf_lball_exp(want, &w->loc[1], 8) == ADF_OK);   /* loc[1] = the prime 5 (sorted: 2, 5) */
    ADF_CHECK(is_one_place(y, 5, want));
    sentinel(y);
    for (k = 0; k < 3; k++)
    {
        where = mark;
        ADF_CHECK(three[k](y, &where, w, inf, 8) == ADF_UNSUPPORTED && adf_place_is_archimedean(where) &&
                  is_sentinel(y));
    }
    adf_lball_clear(c[0]);
    adf_lball_clear(c[1]);
    adf_lball_clear(want);
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(w);
}

/* Log_at at the archimedean place is the real logarithm (domain t > 0, SPEC 9.3.2): the same result and statuses as
   log_at; a negative input is DOMAIN with where = inf; the ball across 0 is NOT_DETERMINED; prec above the limit is
   LIMIT with where = inf. A Log_at that took log |t| fails on -2. */
ADF_TEST(Log_at_the_real_place)
{
    adf_sball_t x, y, z;
    adf_place_t where, mark = place_of(1000003), inf = adf_place_inf();
    arb_t r, t;
    slong vals[3] = {2, -2, 0};
    int i, s1, s2;

    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(z);
    arb_init(r);
    arb_init(t);
    for (i = 0; i < 3; i++)
    {
        arb_set_si(r, vals[i]);
        ADF_CHECK(adf_sball_set_arb_lballs(x, NULL, r, NULL, 0) == ADF_OK);
        sentinel(y);
        sentinel(z);
        where = mark;
        s1 = adf_sball_Log_at(y, &where, x, inf, 100);
        s2 = adf_sball_log_at(z, NULL, x, inf, 100);
        ADF_CHECK_MSG(s1 == s2, "value %ld: %s against %s", (long) vals[i], adf_status_str(s1), adf_status_str(s2));
        if (vals[i] > 0)
        {
            ADF_CHECK(s1 == ADF_OK && adf_place_equal(where, mark) && adf_sball_identical(y, z));
            arb_const_log2(t, 300);
            ADF_CHECK(arb_contains(acb_realref(y->inf), t) && y->arch == ADF_ARCH_REAL && y->len == 0);
        }
        else
            ADF_CHECK(s1 == ADF_DOMAIN && adf_place_is_archimedean(where) && is_sentinel(y));
    }
    /* the ball 0 +- 1 */
    arb_zero(r);
    mag_one(arb_radref(r));
    ADF_CHECK(adf_sball_set_arb_lballs(x, NULL, r, NULL, 0) == ADF_OK);
    where = mark;
    ADF_CHECK(adf_sball_Log_at(y, &where, x, inf, 100) == ADF_NOT_DETERMINED && adf_place_is_archimedean(where));
    /* the limit: prec above ADF_REAL_PREC_MAX at the real place */
    arb_set_si(r, 2);
    ADF_CHECK(adf_sball_set_arb_lballs(x, NULL, r, NULL, 0) == ADF_OK);
    where = mark;
    ADF_CHECK(adf_sball_Log_at(y, &where, x, inf, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT && adf_place_is_archimedean(where));
    /* a prime that is not a place */
    where = mark;
    ADF_CHECK(adf_sball_Log_at(y, &where, x, place_of(7), 100) == ADF_DOMAIN && adf_place_equal(where, place_of(7)));
    arb_clear(r);
    arb_clear(t);
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(z);
}

/* An adele of a rational, projected to {2, 3, 5, real}, and exp_at, log_at, Log_at at each place: the component of the
   result at a prime has the fields of the lfunc.h function on the component of the projection, at the real place the
   result is that of the arb-level function (identical) and contains the value at 300 bits. Statuses: q = 20/3 has
   v_2 = 2, v_3 = -1, v_5 = 1, so exp is OK at 2, 5 and the real place, and DOMAIN at 3; q = 31 = 1 + 2 3 5 has
   v(q - 1) = 1 at 2, 3, 5, so log is OK at every place; Log(20/3) is OK everywhere. A wrapper that took the component
   of another prime, or the wrong function, fails. */
ADF_TEST(projected_adele_at_every_place)
{
    static const ulong ps[3] = {2, 3, 5};
    adf_adele_t a;
    adf_rat_t q;
    adf_sball_t x, y;
    adf_place_t places[4], where, mark = place_of(1000003), inf = adf_place_inf();
    adf_lball_t c, want;
    arb_t r, w, t;
    int which, i, st, ls;
    const slong nums[3] = {20, 31, 20}, dens[3] = {3, 1, 3};
    at_fn fs[3] = {adf_sball_exp_at, adf_sball_log_at, adf_sball_Log_at};
    lf_fn ls_fn[3] = {adf_lball_exp, adf_lball_log, adf_lball_Log};
    int expect_ok[3][3] = {{1, 0, 1}, {1, 1, 1}, {1, 1, 1}};   /* [function][2, 3, 5] */

    adf_adele_init(a);
    adf_rat_init(q);
    adf_sball_init(x);
    adf_sball_init(y);
    adf_lball_init(c);
    adf_lball_init(want);
    arb_init(r);
    arb_init(w);
    arb_init(t);
    places[0] = place_of(5);
    places[1] = inf;
    places[2] = place_of(2);
    places[3] = place_of(3);
    for (which = 0; which < 3; which++)
    {
        rat_of(q, nums[which], dens[which]);
        adf_adele_set_rat(a, q, 200);
        ADF_CHECK(adf_sball_project(x, NULL, a, places, 4) == ADF_OK && adf_sball_num_places(x) == 4);
        for (i = 0; i < 3; i++)
        {
            ADF_CHECK(adf_sball_get_lball(c, x, place_of(ps[i])) == ADF_OK);
            ls = ls_fn[which](want, c, 20);
            sentinel(y);
            where = mark;
            st = fs[which](y, &where, x, place_of(ps[i]), 20);
            ADF_CHECK_MSG(st == ls && (st == ADF_OK) == (expect_ok[which][i] != 0),
                          "function %d at %lu: status %s, lfunc %s", which, (unsigned long) ps[i], adf_status_str(st),
                          adf_status_str(ls));
            if (st == ADF_OK)
                ADF_CHECK_MSG(is_one_place(y, ps[i], want) && adf_place_equal(where, mark), "function %d at %lu", which,
                              (unsigned long) ps[i]);
            else
                ADF_CHECK(adf_place_equal(where, place_of(ps[i])) && is_sentinel(y));
        }
        /* the real place */
        ADF_CHECK(adf_sball_get_arb(r, x, inf) == ADF_OK);
        st = fs[which](y, &where, x, inf, 200);
        ADF_CHECK(st == ADF_OK && y->arch == ADF_ARCH_REAL && y->len == 0);
        arb_set_fmpq(w, q->q, 300);
        if (which == 0)
        {
            arb_exp(t, w, 300);
            ADF_CHECK(adf_real_exp(t, r, 200) == ADF_OK);
        }
        else if (which == 1)
        {
            arb_log(t, w, 300);
            ADF_CHECK(adf_real_log(t, r, 200) == ADF_OK);
        }
        else
        {
            arb_log(t, w, 300);
            ADF_CHECK(adf_real_log(t, r, 200) == ADF_OK);
        }
        ADF_CHECK(arb_equal(acb_realref(y->inf), t));
        /* the value at 300 bits lies in the ball */
        arb_set_fmpq(w, q->q, 400);
        if (which == 0)
            arb_exp(w, w, 400);
        else
            arb_log(w, w, 400);
        ADF_CHECK(arb_overlaps(acb_realref(y->inf), w));
    }
    adf_adele_clear(a);
    adf_rat_clear(q);
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_lball_clear(c);
    adf_lball_clear(want);
    arb_clear(r);
    arb_clear(w);
    arb_clear(t);
}

/* Aliasing, all permitted combinations: y = x on a partial ball of several places gives the partial ball over the one
   place, equal to the result with a distinct output; on a status x is unchanged. A wrapper that clears y before it has
   read the component fails (a read of freed memory under SAN=1, or a wrong result). */
ADF_TEST(aliasing_at_a_prime)
{
    adf_lball_t c[3];
    adf_sball_t x, y, z;
    adf_place_t where, mark = place_of(1000003), inf = adf_place_inf();
    at_fn fs[3] = {adf_sball_exp_at, adf_sball_log_at, adf_sball_Log_at};
    slong nums[3] = {25, 6, 10}, dens[3] = {1, 1, 1};
    int k, st1, st2;

    adf_lball_init(c[0]);
    adf_lball_init(c[1]);
    adf_lball_init(c[2]);
    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(z);
    for (k = 0; k < 3; k++)
    {
        lb_exact(c[0], 5, nums[k], dens[k]);
        lb_exact(c[1], 2, 4, 1);
        lb_ball(c[2], 7, 0, 1, 0);
        make_sball(x, 9, (adf_lball_struct[]){c[0][0], c[1][0], c[2][0]}, 3);
        ADF_CHECK(adf_sball_num_places(x) == 4);
        adf_sball_set(z, x);
        where = mark;
        st1 = fs[k](y, &where, x, place_of(5), 30);   /* distinct output */
        st2 = fs[k](z, &where, z, place_of(5), 30);   /* aliased */
        ADF_CHECK_MSG(st1 == ADF_OK && st2 == ADF_OK && adf_sball_identical(y, z) && z->len == 1 && z->arch == ADF_ARCH_NONE,
                      "function %d: statuses %s %s", k, adf_status_str(st1), adf_status_str(st2));
        /* on a status: the ball O(7^0) = Z_7 at 7 contains 0 and 1: NOT_DETERMINED for all three */
        adf_sball_set(z, x);
        where = mark;
        st2 = fs[k](z, &where, z, place_of(7), 30);
        st1 = fs[k](y, NULL, x, place_of(7), 30);
        ADF_CHECK_MSG(st1 == st2 && adf_place_equal(where, place_of(7)) && adf_sball_identical(z, x), "function %d at 7", k);
    }
    (void) inf;
    adf_lball_clear(c[0]);
    adf_lball_clear(c[1]);
    adf_lball_clear(c[2]);
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(z);
}

/* A value computed by hand from the definition, not from the reference file: exp(5) at 5 modulo 5^8 = 390625. The
   series 1 + 5 + 25/2 + 125/6 + 625/24 + 3125/120 + 15625/720 + 78125/5040 + ...; the terms of degree k >= 8 have
   valuation k - v_5(k!) >= 8 - 1 - ... : for k = 8, 9 that is 8 - 1 = 7 < 8, so they count; the reference computes
   it exactly. Here only the residue class modulo 5 and modulo 25 is checked by hand: exp(5) = 1 + 5 + 25/2 + ... =
   1 + 5 mod 25, i.e. the centre is 6 modulo 25. Fails if the component is not exp of the input. */
ADF_TEST(exp_of_five_by_hand)
{
    adf_lball_t c;
    adf_sball_t x, y;
    adf_place_t where;
    fmpz_t cen, m;

    adf_lball_init(c);
    adf_sball_init(x);
    adf_sball_init(y);
    fmpz_init(cen);
    fmpz_init(m);
    lb_exact(c, 5, 5, 1);
    make_sball(x, 0, c, 1);
    if (adf_sball_exp_at(y, &where, x, place_of(5), 8) == ADF_OK && y->len == 1)
    {
        ADF_CHECK(y->loc[0].exact == 0 && y->loc[0].N == 8 && y->loc[0].v == 0 &&
                  fmpz_is_one(fmpq_denref(y->loc[0].u)));
        fmpz_set(cen, fmpq_numref(y->loc[0].u));
        fmpz_set_ui(m, 25);
        fmpz_mod(cen, cen, m);
        ADF_CHECK(fmpz_equal_ui(cen, 6));
    }
    else
        ADF_CHECK_MSG(0, "exp_at at a prime did not return a one-place result");
    adf_lball_clear(c);
    adf_sball_clear(x);
    adf_sball_clear(y);
    fmpz_clear(cen);
    fmpz_clear(m);
}

/* 1F.7: all exact oracle rows at a prime of a multi-place input, also aliased. A wrong
   dispatch, lost component precision, wrong where or changed failure output fails a row. */
ADF_TEST(trig_reference_at_prime)
{
    at_fn fs[] = {adf_sball_sin_at, adf_sball_cos_at, adf_sball_sinh_at, adf_sball_cosh_at};
    const char *names[] = {"sin", "cos", "sinh", "cosh"};
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice7/cases.jsonl");
    adf_lball_t c[2], want;
    adf_sball_t x, y, z;
    adf_place_t mark = place_of(1000003), where;
    if (!f) return;
    ADF_CHECK(jsonl_count(f) == 4064);
    adf_lball_init(c[0]); adf_lball_init(c[1]); adf_lball_init(want);
    adf_sball_init(x); adf_sball_init(y); adf_sball_init(z);
    for (size_t row = 0; row < jsonl_count(f); row++)
    {
        const jsonl_value *r = jsonl_record(f, row);
        const char *name = member_str(r, "f");
        int k = 0, st, ast, expected = status_from_name(member_str(r, "status"));
        slong N = member_slong(r, "N");
        while (k < 3 && strcmp(name, names[k])) k++;
        lb_from_json(c[0], member(r, "x")); lb_exact(c[1], 17, 19, 1);
        make_sball(x, 3, (adf_lball_struct[]){c[0][0], c[1][0]}, 2);
        sentinel(y); adf_sball_set(z, x); where = mark;
        st = fs[k](y, &where, x, place_of(c[0]->p), N);
        ADF_CHECK_MSG(st == expected, "trig row %zu status", row+1);
        if (expected == ADF_OK)
        {
            lb_from_json(want, member(r, "y"));
            ADF_CHECK(is_one_place(y, c[0]->p, want) && adf_place_equal(where, mark));
        }
        else
            ADF_CHECK(is_sentinel(y) && adf_place_equal(where, place_of(c[0]->p)));
        ast = fs[k](z, NULL, z, place_of(c[0]->p), N);
        ADF_CHECK(ast == expected && adf_sball_identical(z, expected == ADF_OK ? y : x));
    }
    adf_lball_clear(c[0]); adf_lball_clear(c[1]); adf_lball_clear(want);
    adf_sball_clear(x); adf_sball_clear(y); adf_sball_clear(z); jsonl_close(f);
}

ADF_TEST(trig_at_limits_and_places)
{
    at_fn fs[] = {adf_sball_sin_at, adf_sball_cos_at, adf_sball_sinh_at, adf_sball_cosh_at};
    adf_sball_t x, y, z;
    adf_lball_t c;
    adf_place_t where, p = place_of(2), missing = place_of(7), mark = place_of(11);
    adf_sball_init(x); adf_sball_init(y); adf_sball_init(z); adf_lball_init(c);
    for (int k = 0; k < 4; k++)
    for (int j = 0; j < 7; j++)
    {
        slong N = LONG_MAX;
        int st = ADF_LIMIT;
        lb_exact(c, 2, 4, 1);
        if (j == 1) { lb_exact(c, 2, 2, 1); st = ADF_DOMAIN; }
        if (j == 2) { lb_ball(c, 2, 0, 1, 1); st = ADF_NOT_DETERMINED; }
        if (j == 3) { lb_exact(c, 2, 0, 1); st = ADF_OK; }
        if (j == 4) { N = 8; st = ADF_OK; }
        if (j == 5) { N = -3; st = ADF_OK; }
        if (j == 6) lb_raw(c, 2, 0, 0, 0, EMAX+1);
        make_sball(x, 1, c, 1);
        x->arch = ADF_ARCH_COMPLEX; arb_one(acb_imagref(x->inf));
        sentinel(y); adf_sball_set(z, x); where = mark;
        ADF_CHECK(fs[k](y, &where, x, p, N) == st);
        ADF_CHECK(adf_place_equal(where, st == ADF_OK ? mark : p));
        if (st != ADF_OK) ADF_CHECK(is_sentinel(y));
        ADF_CHECK(fs[k](z, NULL, z, p, N) == st && adf_sball_identical(z, st == ADF_OK ? y : x));
        sentinel(y);
        ADF_CHECK(fs[k](y, &where, x, missing, N) == ADF_DOMAIN && adf_place_equal(where, missing));
        ADF_CHECK(is_sentinel(y));
        ADF_CHECK(fs[k](y, &where, x, adf_place_inf(), 53) == ADF_UNSUPPORTED);
        ADF_CHECK(adf_place_is_archimedean(where) && is_sentinel(y));
    }
    adf_lball_clear(c); adf_sball_clear(x); adf_sball_clear(y); adf_sball_clear(z);
}

/* Real sinh/cosh use exactly arb's enclosure and loss rule. Test a finite width around
   zero and away from zero, endpoints at 250 bits, prec clamping, limits and overflow. */
ADF_TEST(hyperbolic_at_real_arb_and_loss)
{
    at_fn fs[] = {adf_sball_sinh_at, adf_sball_cosh_at};
    adf_sball_t x, y, z;
    adf_place_t where, mark = place_of(11), inf = adf_place_inf();
    arb_t r, expected, point, value;
    adf_sball_init(x); adf_sball_init(y); adf_sball_init(z);
    arb_init(r); arb_init(expected); arb_init(point); arb_init(value);
    for (int f = 0; f < 2; f++)
    {
        for (int j = -2; j <= 2; j++)
        for (int wide = 0; wide < 2; wide++)
        for (int low = 0; low < 2; low++)
        {
            slong prec = low ? -3 : 100, work = low ? 2 : 100;
            arb_set_si(r, j);
            if (wide) arb_add_error_2exp_si(r, -3);
            ADF_CHECK(adf_sball_set_arb_lballs(x, NULL, r, NULL, 0) == ADF_OK);
            if (f) arb_cosh(expected, r, work); else arb_sinh(expected, r, work);
            where = mark; adf_sball_set(z, x);
            ADF_CHECK(fs[f](y, &where, x, inf, prec) == ADF_OK && adf_place_equal(where, mark));
            ADF_CHECK(y->arch == ADF_ARCH_REAL && y->len == 0 && arb_equal(acb_realref(y->inf), expected));
            ADF_CHECK(fs[f](z, NULL, z, inf, prec) == ADF_OK && adf_sball_identical(y, z));
            for (int s = -1; s <= 1; s++)
            {
                arb_set_si(point, j*8 + (wide ? s : 0)); arb_mul_2exp_si(point, point, -3);
                if (f) arb_cosh(value, point, 250); else arb_sinh(value, point, 250);
                ADF_CHECK(arb_contains(acb_realref(y->inf), value));
            }
        }
        sentinel(y); where = mark;
        ADF_CHECK(fs[f](y, &where, x, inf, ADF_REAL_PREC_MAX+1) == ADF_LIMIT);
        ADF_CHECK(adf_place_is_archimedean(where) && is_sentinel(y));
        arb_one(r); arb_mul_2exp_si(r, r, 1000);
        ADF_CHECK(adf_sball_set_arb_lballs(x, NULL, r, NULL, 0) == ADF_OK);
        adf_sball_set(z, x);
        ADF_CHECK(fs[f](y, &where, x, inf, 100) == ADF_NOT_DETERMINED);
        ADF_CHECK(is_sentinel(y) && adf_place_is_archimedean(where));
        ADF_CHECK(fs[f](z, NULL, z, inf, 100) == ADF_NOT_DETERMINED && adf_sball_identical(z, x));
    }
    arb_clear(r); arb_clear(expected); arb_clear(point); arb_clear(value);
    adf_sball_clear(x); adf_sball_clear(y); adf_sball_clear(z);
}
