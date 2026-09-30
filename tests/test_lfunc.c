/* tests/test_lfunc.c: exp, log and the Iwasawa Log on local balls (lane f-slice4; include/adelefeld/lfunc.h;
   docs/api-1f4.md F1 to F7; docs/proofs/functions.md Propositions 6, 7, 7b, 8, 10, 11, Lemma 9).

   Oracles.
   1. The reference proto/lfunc_checks.py, through the vector files of tests/ref/vectors/f-slice4 made by
      lanes/f-slice4/gen_vectors.py. lfunc_cases.jsonl: input, requested N, status and result, compared field by
      field (every line, also with the output aliased to the input). lfunc_points.jsonl: the value of f at every
      point of a grid (p = 2: [0, 2^8), 3: [0, 3^5), 5: [0, 5^4), 7: [0, 7^3)) modulo p^(M + 2), computed by exact
      rational sums with a tail bound of three digits more than asked (so a count of terms that is too short in C
      shows as a different residue).
   2. Enumeration over that grid: for every ball of the grid and several requested N, the status agrees with the
      membership of the grid points in the domain (all in: OK, none: DOMAIN, both: NOT_DETERMINED), the exponent is
      min(N, E), the value of f at EVERY grid point of the ball lies in the result, and when the exponent is E the
      values modulo p^(E + 1) are not all equal: no smaller ball contains them.
   3. FLINT's padic_exp, padic_log and padic_teichmuller at the centre (refs/src/flint-3.0.1/padic.rst:410-425,
      494-507, 551-559) as a second opinion: 1000 random inputs, precision up to 200, and precision 2000.
   4. Identities as containment (Lemma 9, Proposition 11): exp(log x) and log(exp x) against x, Log(x y) against
      Log x + Log y, Log(p) = 0.
   5. Statuses with the state of the output, limits, aliasing, and (with -DADF_CHECK_INVARIANTS) the entry check.

   What would make a case fail is stated at each test. */

#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include <flint/fmpz_vec.h>
#include <flint/padic.h>
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

#define BIGP UWORD(18446744073709551557)          /* 2^64 - 59 */
#define EMAX ADF_LBALL_EXP_MAX

typedef int (*lfn_t)(adf_lball_t, const adf_lball_t, slong);

/* ---------------------------------------------------------------------------------------------------- helpers */

static adf_place_t
place_of(ulong p)
{
    adf_place_t v;
    int st = adf_place_prime(&v, p);
    ADF_CHECK(st == ADF_OK);
    return v;
}

static lfn_t
fn_of(const char * name)
{
    if (strcmp(name, "exp") == 0)
        return adf_lball_exp;
    if (strcmp(name, "log") == 0)
        return adf_lball_log;
    if (strcmp(name, "Log") == 0)
        return adf_lball_Log;
    ADF_CHECK_MSG(0, "unknown function %s", name);
    return adf_lball_exp;
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

/* x = the exact rational q at p. */
static void
lb_exact_q(adf_lball_t x, ulong p, const fmpq_t q)
{
    adf_rat_t r;
    int st;
    adf_rat_init(r);
    fmpq_set(r->q, q);
    st = adf_lball_set_rat(x, place_of(p), r);
    ADF_CHECK(st == ADF_OK);
    adf_rat_clear(r);
}

/* x = the ball q + p^N Z_p. */
static void
lb_ball_q(adf_lball_t x, ulong p, const fmpq_t q, slong N)
{
    adf_rat_t r;
    int st;
    adf_rat_init(r);
    fmpq_set(r->q, q);
    st = adf_lball_set_rat_ball(x, place_of(p), r, N);
    ADF_CHECK_MSG(st == ADF_OK, "set_rat_ball status %d", st);
    adf_rat_clear(r);
}

static void
lb_exact_si(adf_lball_t x, ulong p, slong a, slong b)
{
    fmpq_t q;
    fmpq_init(q);
    fmpq_set_si(q, a, (ulong) b);
    lb_exact_q(x, p, q);
    fmpq_clear(q);
}

static void
lb_ball_si(adf_lball_t x, ulong p, slong a, slong b, slong N)
{
    fmpq_t q;
    fmpq_init(q);
    fmpq_set_si(q, a, (ulong) b);
    lb_ball_q(x, p, q, N);
    fmpq_clear(q);
}

/* Raw fields (the value must be canonical; checked). */
static void
lb_raw(adf_lball_t x, ulong p, int exact, const fmpz_t un, const fmpz_t ud, slong v, slong N)
{
    x->p = p;
    fmpz_set(fmpq_numref(x->u), un);
    fmpz_set(fmpq_denref(x->u), ud);
    x->v = v;
    x->N = N;
    x->exact = exact;
    ADF_CHECK(adf_lball_is_canonical(x));
}

static void
lb_raw_si(adf_lball_t x, ulong p, int exact, slong un, slong v, slong N)
{
    fmpz_t a, b;
    fmpz_init_set_si(a, un);
    fmpz_init_set_ui(b, 1);
    lb_raw(x, p, exact, a, b, v, N);
    fmpz_clear(a);
    fmpz_clear(b);
}

/* The sentinel written into an output before a call that must leave it untouched. */
static void
sentinel(adf_lball_t y)
{
    lb_raw_si(y, 11, 1, 13, 3, 0);
}

static int
is_sentinel(const adf_lball_t y)
{
    return y->p == 11 && y->exact == 1 && y->v == 3 && y->N == 0 && fmpz_equal_si(fmpq_numref(y->u), 13)
           && fmpz_is_one(fmpq_denref(y->u));
}

static int
fields_equal(const adf_lball_t x, const adf_lball_t y)
{
    return x->p == y->p && x->exact == y->exact && x->v == y->v && x->N == y->N && fmpq_equal(x->u, y->u);
}

/* JSON accessors */
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

static void
member_fmpz(fmpz_t z, const jsonl_value * rec, const char * key)
{
    int r = fmpz_set_str(z, member_int(rec, key), 10);
    ADF_CHECK(r == 0);
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

/* x = the lball of a JSON object {p, exact, un, ud, v, N}. */
static void
lb_from_json(adf_lball_t x, const jsonl_value * o)
{
    fmpz_t un, ud;
    fmpz_init(un);
    fmpz_init(ud);
    member_fmpz(un, o, "un");
    member_fmpz(ud, o, "ud");
    lb_raw(x, member_ulong(o, "p"), (int) member_slong(o, "exact"), un, ud, member_slong(o, "v"), member_slong(o, "N"));
    fmpz_clear(un);
    fmpz_clear(ud);
}

static jsonl_file *
open_vectors(const char * name)
{
    char path[256];
    jsonl_error_t err;
    jsonl_file * f = NULL;
    snprintf(path, sizeof path, "tests/ref/vectors/f-slice4/%s", name);
    ADF_CHECK_MSG(jsonl_open(path, &f, &err), "%s", jsonl_error_message(&err));
    return f;
}

/* Old-code fixtures, seed 930505. Compare every stored field and status, including aliasing.
   2000 inputs at six primes, balls and exact rationals, N through 3000 and negative Log valuations. */
ADF_TEST(stored_before_optimisation)
{
    jsonl_file *f = NULL;
    jsonl_error_t err;
    size_t i;
    adf_lball_t x, y, z, want;
    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/f-slice5/stored.jsonl", &f, &err),
                  "%s", jsonl_error_message(&err));
    if (!f) return;
    ADF_CHECK(jsonl_count(f) == 2000);
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(want);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        lfn_t fn = fn_of(member_str(rec, "f"));
        int st, ast, expected = (int) member_slong(rec, "st");
        slong N = member_slong(rec, "N");
        lb_from_json(x, member(rec, "x")); lb_from_json(want, member(rec, "y"));
        /* The saved probe's unchanged-output sentinel. */
        y->p = 7; y->v = -3; y->N = 0; y->exact = 1; fmpq_set_si(y->u, 17, 19);
        adf_lball_set(z, x);
        st = fn(y, x, N); ast = fn(z, z, N);
        ADF_CHECK_MSG(st == expected && fields_equal(y, want), "stored row %lu", (ulong) i+1);
        ADF_CHECK_MSG(ast == expected && fields_equal(z, st == ADF_OK ? want : x),
                      "stored alias row %lu", (ulong) i+1);
    }
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(want);
    jsonl_close(f);
}

/* Five seconds per call is the regression constant requested for review R1.
   A child alarm bounds even the old code, so a stalled call fails instead of blocking the suite. */
ADF_TEST(review_R1_five_seconds)
{
    int i;
    for (i = 0; i < 2; i++)
    {
        pid_t child = fork();
        int status = 0;
        ADF_CHECK(child >= 0);
        if (child == 0)
        {
            adf_lball_t x, y;
            int st;
            alarm(5);
            adf_lball_init(x); adf_lball_init(y); x->p = BIGP;
            fmpz_set_ui(fmpq_numref(x->u), i == 0 ? BIGP : 2);
            if (i == 0) fmpz_add_ui(fmpq_numref(x->u), fmpq_numref(x->u), 1);
            st = i == 0 ? adf_lball_log(y, x, 10000) : adf_lball_Log(y, x, 10000);
            alarm(0);
            adf_lball_clear(x); adf_lball_clear(y);
            _exit(st == ADF_OK ? 0 : 1);
        }
        if (child > 0)
        {
            ADF_CHECK(waitpid(child, &status, 0) == child);
            ADF_CHECK_MSG(WIFEXITED(status) && WEXITSTATUS(status) == 0,
                          "R1 input %d exceeded 5 seconds or failed: wait status %d", i+1, status);
        }
    }
}

/* v_p of a nonzero integer */
static slong
vp_fmpz(const fmpz_t a, ulong p)
{
    fmpz_t P, r;
    slong e;
    fmpz_init_set_ui(P, p);
    fmpz_init(r);
    e = fmpz_remove(r, a, P);
    fmpz_clear(P);
    fmpz_clear(r);
    return e;
}

/* ------------------------------------------------------------------------------------------ 1. the case vectors */

/* Every line of lfunc_cases.jsonl: the status, and on OK every field of the result, must be those of the
   reference; on another status the output must be the sentinel. The same call with the output aliased to the input
   must give the same status and result, and leave the input as it was on a status other than OK. Fails on any
   difference of status or of one field (the precision K, the centre, the valuation, the exact flag). */
ADF_TEST(cases_from_the_reference)
{
    jsonl_file * f = open_vectors("lfunc_cases.jsonl");
    size_t i, n = f ? jsonl_count(f) : 0;
    adf_lball_t x, y, z, want;
    ulong counts[ADF_STATUS_COUNT] = {0};
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(want);
    for (i = 0; i < n; i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * fname = member_str(rec, "f");
        lfn_t fn = fn_of(fname);
        slong N = member_slong(rec, "N");
        int want_st = status_from_name(member_str(rec, "status")), st, st2;
        lb_from_json(x, member(rec, "x"));
        sentinel(y);
        st = fn(y, x, N);
        ADF_CHECK_MSG(st == want_st, "line %lu (%s, %s): status %s, want %s", (unsigned long) i + 1, fname,
                      member_str(rec, "note"), adf_status_str(st), adf_status_str(want_st));
        if (want_st >= 0 && want_st < ADF_STATUS_COUNT)
            counts[want_st]++;
        if (st == ADF_OK && want_st == ADF_OK)
        {
            lb_from_json(want, member(rec, "result"));
            ADF_CHECK_MSG(fields_equal(y, want) && adf_lball_is_canonical(y),
                          "line %lu (%s, %s): result p^%ld u + p^%ld Z_p exact %d, want v %ld N %ld exact %d",
                          (unsigned long) i + 1, fname, member_str(rec, "note"), (long) y->v, (long) y->N, y->exact,
                          (long) want->v, (long) want->N, want->exact);
        }
        else
            ADF_CHECK_MSG(is_sentinel(y), "line %lu: output written on status %s", (unsigned long) i + 1,
                          adf_status_str(st));
        /* aliased: y = x */
        adf_lball_set(z, x);
        st2 = fn(z, z, N);
        ADF_CHECK(st2 == st);
        if (st == ADF_OK)
            ADF_CHECK_MSG(fields_equal(z, y), "line %lu: aliased result differs", (unsigned long) i + 1);
        else
            ADF_CHECK_MSG(fields_equal(z, x), "line %lu: aliased input changed on a status", (unsigned long) i + 1);
    }
    ADF_CHECK(n >= 700);
    ADF_CHECK(counts[ADF_OK] > 0 && counts[ADF_DOMAIN] > 0 && counts[ADF_NOT_DETERMINED] > 0);
    printf("  cases_from_the_reference: %lu lines, OK %lu, DOMAIN %lu, NOT_DETERMINED %lu\n", (unsigned long) n,
           counts[ADF_OK], counts[ADF_DOMAIN], counts[ADF_NOT_DETERMINED]);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(want);
    jsonl_close(f);
}

/* ------------------------------------------------------------------------------------------ 2. the grid points */

#define NGRID 4
static const ulong grid_p[NGRID] = {2, 3, 5, 7};
static const slong grid_M[NGRID] = {8, 5, 4, 3};

/* value[g][fi][t]: f(t) modulo p^(M+2) for the grid point t, fi = 0 exp, 1 log, 2 Log; have[..] = 1 if listed. */
typedef struct
{
    fmpz * value[3];
    char * have[3];
    slong size;
} grid_t;

static grid_t grids[NGRID];
static int grids_loaded = 0;

static int
fi_of(const char * name)
{
    return strcmp(name, "exp") == 0 ? 0 : strcmp(name, "log") == 0 ? 1 : 2;
}

static const char * const fname_of[3] = {"exp", "log", "Log"};

/* Every line of lfunc_points.jsonl: f of the exact point t at precision n = M + 2 must be the ball value + p^n Z_p,
   or an exact rational congruent to the value (exp 0 = 1, log 1 = 0, log(-1) = 0 at 2, Log(+-p^m) = 0). Fails on a
   wrong residue, a wrong exponent, a status other than OK. Loads the table for the enumeration. */
static void
load_grids(int check)
{
    jsonl_file * f;
    size_t i, n;
    int g, k;
    adf_lball_t x, y, V;
    fmpz_t val, num, den, Pn;
    fmpq_t q;
    ulong exact_results = 0;
    if (grids_loaded && !check)
        return;
    f = open_vectors("lfunc_points.jsonl");
    n = f ? jsonl_count(f) : 0;
    if (!grids_loaded)
        for (g = 0; g < NGRID; g++)
        {
            fmpz_t t;
            fmpz_init(t);
            fmpz_ui_pow_ui(t, grid_p[g], (ulong) grid_M[g]);
            grids[g].size = fmpz_get_si(t);
            for (k = 0; k < 3; k++)
            {
                grids[g].value[k] = _fmpz_vec_init(grids[g].size);
                grids[g].have[k] = calloc((size_t) grids[g].size, 1);
            }
            fmpz_clear(t);
        }
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(V);
    fmpz_init(val); fmpz_init(num); fmpz_init(den); fmpz_init(Pn);
    fmpq_init(q);
    for (i = 0; i < n; i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * xo = member(rec, "x");
        ulong p = member_ulong(rec, "p");
        slong nn = member_slong(rec, "n"), t;
        int fi = fi_of(member_str(rec, "f"));
        for (g = 0; g < NGRID && grid_p[g] != p; g++)
            ;
        ADF_CHECK(g < NGRID && nn == grid_M[g] + 2);
        if (g == NGRID)
            continue;
        member_fmpz(num, xo, "num");
        member_fmpz(den, xo, "den");
        member_fmpz(val, rec, "value");
        ADF_CHECK(fmpz_is_one(den));
        t = fmpz_get_si(num);
        fmpz_set(grids[g].value[fi] + t, val);
        grids[g].have[fi][t] = 1;
        if (!check)
            continue;
        fmpq_set_fmpz_frac(q, num, den);
        lb_exact_q(x, p, q);
        {
            int st = fn_of(fname_of[fi])(y, x, nn);
            ADF_CHECK_MSG(st == ADF_OK, "%s(%ld) at %lu: status %s", fname_of[fi], (long) t, p, adf_status_str(st));
            if (st != ADF_OK)
                continue;
        }
        if (y->exact)
        {
            /* the exact value must be 0 or 1 and agree with the reference modulo p^n */
            fmpz_ui_pow_ui(Pn, p, (ulong) nn);
            ADF_CHECK(y->v == 0 && fmpz_is_one(fmpq_denref(y->u)) && fmpz_cmp_ui(fmpq_numref(y->u), 1) <= 0
                      && fmpz_sgn(fmpq_numref(y->u)) >= 0);
            fmpz_mod(num, fmpq_numref(y->u), Pn);
            ADF_CHECK_MSG(fmpz_equal(num, val), "%s(%ld) at %lu: exact result differs from the reference",
                          fname_of[fi], (long) t, p);
            exact_results++;
        }
        else
        {
            fmpq_set_fmpz_frac(q, val, den);
            lb_ball_q(V, p, q, nn);
            ADF_CHECK_MSG(adf_lball_equal_set(V, y), "%s(%ld) at %lu mod p^%ld: C differs from the reference",
                          fname_of[fi], (long) t, p, (long) nn);
        }
    }
    if (check)
    {
        ADF_CHECK(n >= 2000);
        /* exact results among the grid points: exp(0) (4 primes), log(1) (4), log(-1) is not a grid point,
           Log(p^m) for every power of p in the grid */
        ADF_CHECK(exact_results >= 8);
        printf("  points_from_the_reference: %lu points, %lu exact results\n", (unsigned long) n, exact_results);
    }
    grids_loaded = 1;
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(V);
    fmpz_clear(val); fmpz_clear(num); fmpz_clear(den); fmpz_clear(Pn);
    fmpq_clear(q);
    jsonl_close(f);
}

ADF_TEST(points_from_the_reference)
{
    load_grids(1);
}

/* 1 if the rational t (= y / p^j) lies in the domain of f. */
static int
in_domain(int fi, ulong p, const fmpz_t y, slong j)
{
    slong c = p == 2 ? 2 : 1;
    if (fi == 2)
        return !fmpz_is_zero(y);
    if (fi == 0)
        return fmpz_is_zero(y) || vp_fmpz(y, p) - j >= c;
    /* log: t != 0 and v(t - 1) >= 1; here j = 0 */
    {
        fmpz_t d;
        int r;
        if (fmpz_is_zero(y))
            return 0;
        fmpz_init(d);
        fmpz_sub_ui(d, y, 1);
        r = fmpz_is_zero(d) || fmpz_fdiv_ui(d, p) == 0;
        fmpz_clear(d);
        return r;
    }
}

/* The exponent E of lfunc.h for a ball of centre valuation m (ignored for a zero centre) and exponent N. */
static slong
image_E(int fi, ulong p, int zero_centre, slong m, slong N)
{
    slong c = p == 2 ? 2 : 1, r;
    if (fi == 0)
        return N;
    if (fi == 1)
        return N >= c ? N : 2;
    r = N - (zero_centre ? 0 : m);
    return r >= c ? r : 2;
}

/* 2. For p = 2, 3, 5, 7, every f, every ball B = y/p^j + p^N Z_p of the grid (j = 0, and j = 1, 2 for Log;
   0 <= y < p^(N + j); N + j at most M minus one or two spare digits), the points of B modulo p^M are
   (y + p^(N + j) s)/p^j, 0 <= s < p^(M - N - j), and the value of f at each is in the table (Log(y'/p^j) = Log(y')
   by the definition of Log, Proposition 11). Checks, for N_req = E, E - 1, E + 3 and LONG_MAX:
     status = OK if every point is in the domain, DOMAIN if none, NOT_DETERMINED otherwise (conventions 3.1);
     on OK: the result is a ball of exponent min(N_req, E); the value of f at EVERY point, a ball of exponent M + 2,
     lies inside the result; and if the exponent is E, two of the values differ modulo p^(E + 1) (so no ball of
     exponent E + 1 contains the image: the result is the smallest ball);
     on another status the output is the sentinel.
   Fails if a point value lies outside the result (a wrong centre, or a radius taken from the input without the
   loss of Log), if the exponent is larger or smaller than min(N_req, E), or if a status is wrong. */
ADF_TEST(ball_enumeration_over_the_grid)
{
    int g, fi;
    ulong balls = 0, points = 0, tight = 0;
    adf_lball_t B, y, V;
    fmpz_t yy, pt, Pj, step, r0, r1, PK, one;
    fmpq_t q;
    load_grids(0);
    fmpz_init_set_ui(one, 1);
    adf_lball_init(B); adf_lball_init(y); adf_lball_init(V);
    fmpz_init(yy); fmpz_init(pt); fmpz_init(Pj); fmpz_init(step); fmpz_init(r0); fmpz_init(r1); fmpz_init(PK);
    fmpq_init(q);
    for (g = 0; g < NGRID; g++)
    {
        ulong p = grid_p[g];
        slong M = grid_M[g], spare = p <= 3 ? 2 : 1;
        for (fi = 0; fi < 3; fi++)
        {
            slong j, jmax = fi == 2 ? 2 : 0, N;
            for (j = 0; j <= jmax; j++)
                for (N = -j; N + j <= M - spare; N++)
                {
                    slong y0, ymax, s, smax, t;
                    fmpz_ui_pow_ui(step, p, (ulong) (N + j));
                    ymax = fmpz_get_si(step);
                    fmpz_ui_pow_ui(Pj, p, (ulong) j);
                    smax = grids[g].size / ymax;
                    for (y0 = 0; y0 < ymax; y0++)
                    {
                        int n_in = 0, n_out = 0, want, k;
                        slong reqs[4];
                        fmpz_set_si(yy, y0);
                        fmpq_set_fmpz_frac(q, yy, Pj);
                        lb_ball_q(B, p, q, N);
                        for (s = 0; s < smax; s++)
                        {
                            fmpz_set_si(pt, y0 + ymax * s);
                            if (in_domain(fi, p, pt, j))
                                n_in++;
                            else
                                n_out++;
                        }
                        want = n_out == 0 ? ADF_OK : n_in == 0 ? ADF_DOMAIN : ADF_NOT_DETERMINED;
                        {
                            slong E = image_E(fi, p, y0 == 0, y0 == 0 ? 0 : vp_fmpz(yy, p) - j, N);
                            reqs[0] = E; reqs[1] = E - 1; reqs[2] = E + 3; reqs[3] = LONG_MAX;
                            for (k = 0; k < 4; k++)
                            {
                                slong K;
                                int st, distinct = 0;
                                sentinel(y);
                                st = fn_of(fname_of[fi])(y, B, reqs[k]);
                                balls++;
                                ADF_CHECK_MSG(st == want, "%s at %lu of y=%ld/p^%ld + p^%ld: status %s, want %s",
                                              fname_of[fi], p, (long) y0, (long) j, (long) N, adf_status_str(st),
                                              adf_status_str(want));
                                if (st != ADF_OK)
                                {
                                    ADF_CHECK(is_sentinel(y));
                                    continue;
                                }
                                K = y->N;
                                ADF_CHECK_MSG(!y->exact && K == (reqs[k] < E ? reqs[k] : E),
                                              "%s at %lu of y=%ld/p^%ld + p^%ld, N_req %ld: exponent %ld, E %ld",
                                              fname_of[fi], p, (long) y0, (long) j, (long) N, (long) reqs[k],
                                              (long) K, (long) E);
                                if (K + 1 > M + 2 || K < 0)
                                    continue;
                                fmpz_ui_pow_ui(PK, p, (ulong) (K + 1));
                                for (s = 0; s < smax; s++)
                                {
                                    t = y0 + ymax * s;
                                    ADF_CHECK(grids[g].have[fi][t]);
                                    fmpz_set(pt, grids[g].value[fi] + t);
                                    fmpq_set_fmpz_frac(q, pt, one);
                                    lb_ball_q(V, p, q, M + 2);
                                    ADF_CHECK_MSG(adf_lball_contains(V, y),
                                                  "%s at %lu: f(%ld/p^%ld) outside the result of the ball %ld/p^%ld "
                                                  "+ p^%ld, N_req %ld", fname_of[fi], p, (long) t, (long) j, (long) y0,
                                                  (long) j, (long) N, (long) reqs[k]);
                                    points++;
                                    fmpz_mod(r1, pt, PK);
                                    if (s == 0)
                                        fmpz_set(r0, r1);
                                    else if (!fmpz_equal(r0, r1))
                                        distinct = 1;
                                }
                                if (K == E)
                                {
                                    ADF_CHECK_MSG(distinct, "%s at %lu of %ld/p^%ld + p^%ld: not the smallest ball",
                                                  fname_of[fi], p, (long) y0, (long) j, (long) N);
                                    tight++;
                                }
                            }
                        }
                    }
                }
        }
    }
    printf("  ball_enumeration_over_the_grid: %lu calls, %lu point values enclosed, %lu smallest-ball witnesses\n",
           balls, points, tight);
    ADF_CHECK(balls >= 7600 && tight >= 3800);           /* the size of the universe above */
    adf_lball_clear(B); adf_lball_clear(y); adf_lball_clear(V);
    fmpz_clear(yy); fmpz_clear(pt); fmpz_clear(Pj); fmpz_clear(step); fmpz_clear(r0); fmpz_clear(r1); fmpz_clear(PK);
    fmpz_clear(one);
    fmpq_clear(q);
}

/* ---------------------------------------------------------------------------------- 3. the cases of SPEC 9.3.2 */

/* By hand, with their meaning. Fails if a precision case of SPEC 9.3.2 is not as the table says. */
ADF_TEST(spec_precision_cases)
{
    adf_lball_t x, a, b, c, y;
    slong v;
    int inf, st;
    adf_lball_init(x); adf_lball_init(a); adf_lball_init(b); adf_lball_init(c); adf_lball_init(y);

    /* review N4: exp 3 and exp 12 at precision 8 differ at digit 2; both lie in exp(3 + 9 Z_3), exponent 2 */
    lb_exact_si(x, 3, 3, 1);
    ADF_CHECK(adf_lball_exp(a, x, 8) == ADF_OK && a->N == 8 && fmpz_equal_si(fmpq_numref(a->u), 958));
    lb_exact_si(x, 3, 12, 1);
    ADF_CHECK(adf_lball_exp(b, x, 8) == ADF_OK && b->N == 8 && fmpz_equal_si(fmpq_numref(b->u), 5125));
    ADF_CHECK(!adf_lball_overlaps(a, b));
    lb_ball_si(x, 3, 3, 1, 2);
    ADF_CHECK(adf_lball_exp(c, x, 8) == ADF_OK && c->N == 2);
    ADF_CHECK(adf_lball_contains(a, c) && adf_lball_contains(b, c));

    /* exp keeps the radius p^N (an isometry): 4 + 2^5 Z_2 -> exponent 5 */
    lb_ball_si(x, 2, 4, 1, 5);
    ADF_CHECK(adf_lball_exp(y, x, 100) == ADF_OK && y->N == 5 && y->v == 0);

    /* log(-1) = 0 at 2, exactly; log(3) has valuation 2 at 2; log(1 + 2 Z_2) = 4 Z_2 */
    lb_exact_si(x, 2, -1, 1);
    ADF_CHECK(adf_lball_log(y, x, 50) == ADF_OK && y->exact && fmpq_is_zero(y->u));
    lb_exact_si(x, 2, 3, 1);
    ADF_CHECK(adf_lball_log(y, x, 50) == ADF_OK && !y->exact && y->N == 50);
    st = adf_lball_valuation(&v, &inf, y);
    ADF_CHECK(st == ADF_OK && v == 2 && !inf);
    lb_ball_si(x, 2, 1, 1, 1);
    ADF_CHECK(adf_lball_log(y, x, 50) == ADF_OK && !y->exact && y->N == 2 && fmpq_is_zero(y->u));

    /* the losses of Log: v_3(12 - 3) = 2 but Log(3 + 9 Z_3) = 3 Z_3; v_2(10 - 2) = 3 but Log(2 + 8 Z_2) = 4 Z_2 */
    lb_ball_si(x, 3, 3, 1, 2);
    ADF_CHECK(adf_lball_Log(y, x, 50) == ADF_OK && y->N == 1 && fmpq_is_zero(y->u));
    lb_ball_si(x, 2, 2, 1, 3);
    ADF_CHECK(adf_lball_Log(y, x, 50) == ADF_OK && y->N == 2 && fmpq_is_zero(y->u));
    /* m digits lost: 3^4 * 2 + 3^9 Z_3 has m = 4, r = 5 */
    lb_ball_si(x, 3, 162, 1, 9);
    ADF_CHECK(adf_lball_Log(y, x, 50) == ADF_OK && y->N == 5);
    /* -m digits gained: 2/27 + 3^2 Z_3 has m = -3, r = 5 */
    lb_ball_si(x, 3, 2, 27, 2);
    ADF_CHECK(adf_lball_Log(y, x, 50) == ADF_OK && y->N == 5);
    /* Log(p) = 0 exactly at several primes, the large one included */
    {
        const ulong ps[5] = {2, 3, 5, 7, BIGP};
        int i;
        for (i = 0; i < 5; i++)
        {
            fmpq_t q;
            fmpq_init(q);
            fmpz_set_ui(fmpq_numref(q), ps[i]);
            lb_exact_q(x, ps[i], q);
            ADF_CHECK(adf_lball_Log(y, x, 20) == ADF_OK && y->exact && fmpq_is_zero(y->u));
            fmpq_clear(q);
        }
    }
    adf_lball_clear(x); adf_lball_clear(a); adf_lball_clear(b); adf_lball_clear(c); adf_lball_clear(y);
}

/* ----------------------------------------------------------------------------- 4. identities as containment */

/* Lemma 9: exp and log are inverse bijections between p^r Z_p and 1 + p^r Z_p, r >= c, and isometries there
   (Propositions 10, 11). So for a ball B in p^c Z_p, log(exp(B)) = B, and for B in 1 + p^c Z_p, exp(log(B)) = B, as
   sets, when N_req is at least the exponent. For exact x the round trip at precision n contains x.
   Proposition 11: Log(x y) = Log x + Log y, so the two balls computed at precision n meet (and are equal when
   both are balls of exponent n). Fails if a round trip does not return the input set, or the two sides of the
   homomorphism are disjoint. */
ADF_TEST(identities_as_containment)
{
    const ulong ps[6] = {2, 3, 5, 7, 13, BIGP};
    flint_rand_t st;
    int i, k;
    ulong trips = 0, homs = 0;
    adf_lball_t x, y, e, l, A, Bq, S, T;
    fmpq_t q, r;
    flint_randinit(st);
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(e); adf_lball_init(l);
    adf_lball_init(A); adf_lball_init(Bq); adf_lball_init(S); adf_lball_init(T);
    fmpq_init(q); fmpq_init(r);
    for (i = 0; i < 6; i++)
    {
        ulong p = ps[i];
        slong c = p == 2 ? 2 : 1;
        for (k = 0; k < 60; k++)
        {
            slong N = c + 1 + (slong) n_randint(st, 20), n = 1 + (slong) n_randint(st, 30);
            slong vv = c + (slong) n_randint(st, 3);
            fmpz_t pv;
            fmpz_init(pv);
            fmpz_set_ui(pv, p);
            fmpz_pow_ui(pv, pv, (ulong) vv);
            /* q = p^vv * a/b */
            fmpz_randtest_not_zero(fmpq_numref(q), st, 40);
            fmpz_randtest_unsigned(fmpq_denref(q), st, 20);
            fmpz_add_ui(fmpq_denref(q), fmpq_denref(q), 1);
            while (fmpz_fdiv_ui(fmpq_denref(q), p) == 0)
                fmpz_add_ui(fmpq_denref(q), fmpq_denref(q), 1);
            fmpq_canonicalise(q);
            fmpz_mul(fmpq_numref(q), fmpq_numref(q), pv);
            fmpq_canonicalise(q);
            /* ball in p^c Z_p: log(exp(B)) = B */
            if (N > vv)
            {
                lb_ball_q(x, p, q, N);
                ADF_CHECK(adf_lball_exp(e, x, N + 5) == ADF_OK);
                ADF_CHECK(adf_lball_log(l, e, N + 5) == ADF_OK);
                ADF_CHECK_MSG(adf_lball_equal_set(l, x), "log(exp(B)) != B at %lu", p);
                /* ball in 1 + p^c Z_p: exp(log(B)) = B */
                fmpq_add_si(r, q, 1);
                lb_ball_q(x, p, r, N);
                ADF_CHECK(adf_lball_log(l, x, N + 5) == ADF_OK);
                ADF_CHECK(adf_lball_exp(e, l, N + 5) == ADF_OK);
                ADF_CHECK_MSG(adf_lball_equal_set(e, x), "exp(log(B)) != B at %lu", p);
                trips += 2;
            }
            /* exact x: log(exp(x)) contains x */
            lb_exact_q(x, p, q);
            ADF_CHECK(adf_lball_exp(e, x, n) == ADF_OK && adf_lball_log(l, e, n) == ADF_OK);
            ADF_CHECK(adf_lball_contains(x, l));
            trips++;
            /* Log(x y) and Log x + Log y, x, y random nonzero rationals with any valuation */
            fmpz_randtest_not_zero(fmpq_numref(q), st, 30);
            fmpz_randtest_not_zero(fmpq_denref(q), st, 20);
            fmpz_abs(fmpq_denref(q), fmpq_denref(q));
            fmpq_canonicalise(q);
            fmpz_randtest_not_zero(fmpq_numref(r), st, 30);
            fmpz_randtest_not_zero(fmpq_denref(r), st, 20);
            fmpz_abs(fmpq_denref(r), fmpq_denref(r));
            fmpq_canonicalise(r);
            lb_exact_q(x, p, q);
            lb_exact_q(y, p, r);
            ADF_CHECK(adf_lball_Log(A, x, n) == ADF_OK && adf_lball_Log(Bq, y, n) == ADF_OK);
            ADF_CHECK(adf_lball_add(S, A, Bq) == ADF_OK);
            fmpq_mul(r, q, r);
            lb_exact_q(y, p, r);
            ADF_CHECK(adf_lball_Log(T, y, n) == ADF_OK);
            ADF_CHECK_MSG(adf_lball_overlaps(S, T), "Log(xy) and Log x + Log y are disjoint at %lu", p);
            if (!A->exact && !Bq->exact && !T->exact)
                ADF_CHECK(adf_lball_equal_set(S, T));
            homs++;
            /* Log(p x) = Log(x) */
            fmpz_mul_ui(fmpq_numref(q), fmpq_numref(q), p);
            fmpq_canonicalise(q);
            lb_exact_q(y, p, q);
            ADF_CHECK(adf_lball_Log(T, y, n) == ADF_OK && adf_lball_equal_set(T, A));
            fmpz_clear(pv);
        }
    }
    printf("  identities_as_containment: %lu round trips, %lu homomorphism checks\n", trips, homs);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(e); adf_lball_clear(l);
    adf_lball_clear(A); adf_lball_clear(Bq); adf_lball_clear(S); adf_lball_clear(T);
    fmpq_clear(q); fmpq_clear(r);
    flint_randclear(st);
}

/* ------------------------------------------------------------------------------ 5. FLINT as a second opinion */

/* The value of FLINT at x to precision n (padic.rst:410-425 exp, 494-507 log, 551-559 teichmuller): which = 0 exp,
   1 log, 2 Log. Log is log(a / teich(a)) for the unit part a at odd p (functions.md Proposition 4: the root of unity
   w is the Teichmueller representative at odd p) and log(+-a) with +-a = 1 modulo 4 at 2 (Proposition 4, step 2).
   Returns 1 and writes the rational residue to out, or 0 if FLINT refuses. */
static int
flint_value(fmpq_t out, int which, ulong p, const fmpq_t x, slong n)
{
    padic_ctx_t ctx;
    padic_t X, Y, W;
    fmpz_t P;
    int ok = 1;
    fmpz_init_set_ui(P, p);
    padic_ctx_init(ctx, P, 0, 0, PADIC_SERIES);
    padic_init2(X, n + 20);
    padic_init2(W, n + 20);
    padic_init2(Y, n);
    padic_set_fmpq(X, x, ctx);
    if (which == 0)
        ok = padic_exp(Y, X, ctx);
    else if (which == 1)
        ok = padic_log(Y, X, ctx);
    else
    {
        padic_val(X) = 0;                                  /* the unit part a: x / p^m */
        if (p == 2)
        {
            if (fmpz_fdiv_ui(padic_unit(X), 4) == 3)
                padic_neg(X, X, ctx);
        }
        else
        {
            padic_teichmuller(W, X, ctx);
            padic_div(X, X, W, ctx);
        }
        ok = padic_log(Y, X, ctx);
    }
    if (ok)
        padic_get_fmpq(out, Y, ctx);
    padic_clear(X); padic_clear(Y); padic_clear(W);
    padic_ctx_clear(ctx);
    fmpz_clear(P);
    return ok;
}

/* 1000 random exact inputs, p in {2, 3, 5, 7, 11, 13, 101, 2^31 - 1, 2^64 - 59}, precision 1 to 200: the result of
   the library must be the ball (FLINT's value) + p^n Z_p (or an exact value congruent to it). Inputs of log at 2
   are taken in 1 + 4 Z_2 (FLINT refuses 3 and -1: padic.rst:506-507); the library's log of x = 3 mod 4 is compared
   with FLINT's log of -x. Fails on any residue that differs. */
ADF_TEST(flint_second_opinion)
{
    const ulong ps[9] = {2, 3, 5, 7, 11, 13, 101, 2147483647, BIGP};
    flint_rand_t st;
    ulong done = 0, refused = 0, i;
    adf_lball_t x, y, V;
    fmpq_t q, fl, t;
    fmpz_t pv;
    flint_randinit(st);
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(V);
    fmpq_init(q); fmpq_init(fl); fmpq_init(t);
    fmpz_init(pv);
    for (i = 0; i < 1000; i++)
    {
        ulong p = ps[n_randint(st, 9)];
        int which = (int) (i % 3), neg = 0;
        slong c = p == 2 ? 2 : 1, n = 1 + (slong) n_randint(st, p > 1000 ? 40 : 200), m;
        /* q = a/b, a and b prime to p */
        do
            fmpz_randtest_not_zero(fmpq_numref(q), st, 60);
        while (fmpz_fdiv_ui(fmpq_numref(q), p) == 0);
        do
        {
            fmpz_randtest_unsigned(fmpq_denref(q), st, 30);
            fmpz_add_ui(fmpq_denref(q), fmpq_denref(q), 1);
        }
        while (fmpz_fdiv_ui(fmpq_denref(q), p) == 0);
        fmpq_canonicalise(q);
        if (which == 0)
            m = c + (slong) n_randint(st, 4);
        else if (which == 1)
            m = (p == 2 ? 2 : 1) + (slong) n_randint(st, 4);
        else
            m = (slong) n_randint(st, 11) - 5;
        fmpz_set_ui(pv, p);
        fmpz_pow_ui(pv, pv, (ulong) (m < 0 ? -m : m));
        if (m >= 0)
            fmpz_mul(fmpq_numref(q), fmpq_numref(q), pv);
        else
            fmpz_mul(fmpq_denref(q), fmpq_denref(q), pv);
        fmpq_canonicalise(q);
        if (which == 1)
        {
            fmpq_add_si(q, q, 1);                           /* 1 + p^m a/b, in FLINT's domain of log */
            if (p == 2 && n_randint(st, 2))
            {
                neg = 1;                                   /* the library gets -q = 3 mod 4, FLINT gets q */
            }
        }
        if (neg)
        {
            fmpq_neg(t, q);
            lb_exact_q(x, p, t);
        }
        else
            lb_exact_q(x, p, q);
        if (!flint_value(fl, which, p, q, n))
        {
            refused++;
            continue;
        }
        {
            int s = fn_of(fname_of[which])(y, x, n);
            ADF_CHECK_MSG(s == ADF_OK, "%s at %lu: status %s", fname_of[which], p, adf_status_str(s));
            if (s != ADF_OK)
                continue;
        }
        if (y->exact)
        {
            ADF_CHECK(fmpq_is_zero(y->u) ? fmpq_is_zero(fl) : fmpq_is_one(fl));
        }
        else
        {
            lb_ball_q(V, p, fl, n);
            ADF_CHECK_MSG(adf_lball_equal_set(V, y), "%s at p = %lu, n = %ld: differs from FLINT", fname_of[which], p,
                          (long) n);
        }
        done++;
    }
    printf("  flint_second_opinion: %lu compared, %lu refused by FLINT\n", done, refused);
    ADF_CHECK(done >= 990 && refused == 0);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(V);
    fmpq_clear(q); fmpq_clear(fl); fmpq_clear(t);
    fmpz_clear(pv);
    flint_randclear(st);
}

/* Precision 2000: exp(3) at 3, Log(2) at 3 (the power a^(p-1)), log(5 + O(2^2000)) at 2, exp(p) and Log(2) at
   2^64 - 59, against FLINT; the result at 2000 contains the result at 100. Fails on a differing residue. */
ADF_TEST(precision_2000)
{
    struct { int which; ulong p; slong a; slong b; slong n; } cs[] = {
        {0, 3, 3, 1, 2000}, {2, 3, 2, 1, 2000}, {1, 2, 5, 1, 2000}, {2, 2, 3, 1, 2000},
        {0, BIGP, 0, 1, 2000}, {2, BIGP, 2, 1, 2000}, {1, BIGP, 0, 1, 300}};
    int i;
    adf_lball_t x, y, s, V;
    fmpq_t q, fl;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(s); adf_lball_init(V);
    fmpq_init(q); fmpq_init(fl);
    for (i = 0; i < (int) (sizeof cs / sizeof cs[0]); i++)
    {
        ulong p = cs[i].p;
        if (p == BIGP && cs[i].which == 0)
            fmpz_set_ui(fmpq_numref(q), p), fmpz_one(fmpq_denref(q));          /* exp(p) */
        else if (p == BIGP && cs[i].which == 1)
            fmpz_set_ui(fmpq_numref(q), p), fmpz_add_ui(fmpq_numref(q), fmpq_numref(q), 1),
                fmpz_one(fmpq_denref(q));                                      /* log(1 + p) */
        else
            fmpq_set_si(q, cs[i].a, (ulong) cs[i].b);
        lb_exact_q(x, p, q);
        ADF_CHECK(fn_of(fname_of[cs[i].which])(y, x, cs[i].n) == ADF_OK && y->N == cs[i].n);
        ADF_CHECK(flint_value(fl, cs[i].which, p, q, cs[i].n));
        lb_ball_q(V, p, fl, cs[i].n);
        ADF_CHECK_MSG(adf_lball_equal_set(V, y), "case %d: differs from FLINT at precision %ld", i, (long) cs[i].n);
        ADF_CHECK(fn_of(fname_of[cs[i].which])(s, x, 100) == ADF_OK && adf_lball_contains(y, s));
    }
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(s); adf_lball_clear(V);
    fmpq_clear(q); fmpq_clear(fl);
}

/* ---------------------------------------------------------------------- 6. statuses, limits, outputs, aliasing */

/* One call with a sentinel output: the status must be want, the output untouched on a status other than OK. */
static void
expect(lfn_t fn, const adf_lball_t x, slong N, int want, const char * what)
{
    adf_lball_t y, z;
    int st;
    adf_lball_init(y);
    adf_lball_init(z);
    sentinel(y);
    st = fn(y, x, N);
    ADF_CHECK_MSG(st == want, "%s: status %s, want %s", what, adf_status_str(st), adf_status_str(want));
    if (st != ADF_OK)
        ADF_CHECK_MSG(is_sentinel(y), "%s: output written on a status", what);
    else
        ADF_CHECK(adf_lball_is_canonical(y));
    adf_lball_set(z, x);
    ADF_CHECK(fn(z, z, N) == st);
    if (st != ADF_OK)
        ADF_CHECK(fields_equal(z, x));
    adf_lball_clear(y);
    adf_lball_clear(z);
}

/* Every status of lfunc.h, with the state of the output. Fails on a wrong status or a written output. */
ADF_TEST(every_status)
{
    adf_lball_t x;
    fmpz_t one;
    adf_lball_init(x);
    fmpz_init_set_ui(one, 1);
    /* DOMAIN and NOT_DETERMINED */
    lb_exact_si(x, 5, 1, 1);        expect(adf_lball_exp, x, 20, ADF_DOMAIN, "exp(1) at 5");
    lb_exact_si(x, 2, 2, 1);        expect(adf_lball_exp, x, 20, ADF_DOMAIN, "exp(2) at 2");
    lb_ball_si(x, 2, 0, 1, 1);      expect(adf_lball_exp, x, 20, ADF_NOT_DETERMINED, "exp(2 Z_2)");
    lb_ball_si(x, 2, 6, 1, 1);      expect(adf_lball_exp, x, 20, ADF_NOT_DETERMINED, "exp(6 + 2 Z_2) = exp(2 Z_2)");
    lb_ball_si(x, 2, 2, 1, 3);      expect(adf_lball_exp, x, 20, ADF_DOMAIN, "exp(2 + 8 Z_2)");
    lb_exact_si(x, 3, 0, 1);        expect(adf_lball_log, x, 20, ADF_DOMAIN, "log(0)");
    lb_exact_si(x, 3, -1, 1);       expect(adf_lball_log, x, 20, ADF_DOMAIN, "log(-1) at 3");
    lb_ball_si(x, 3, 0, 1, 4);      expect(adf_lball_log, x, 20, ADF_DOMAIN, "log(O(3^4))");
    lb_ball_si(x, 3, 0, 1, 0);      expect(adf_lball_log, x, 20, ADF_NOT_DETERMINED, "log(Z_3)");
    lb_ball_si(x, 3, 1, 3, 0);      expect(adf_lball_log, x, 20, ADF_DOMAIN, "log(1/3 + Z_3)");
    lb_ball_si(x, 3, 1, 3, -1);     expect(adf_lball_log, x, 20, ADF_NOT_DETERMINED, "log(1/3 + 3^-1 Z_3)");
    lb_exact_si(x, 7, 0, 1);        expect(adf_lball_Log, x, 20, ADF_DOMAIN, "Log(0)");
    lb_ball_si(x, 7, 0, 1, 3);      expect(adf_lball_Log, x, 20, ADF_NOT_DETERMINED, "Log(O(7^3))");
    lb_ball_si(x, 7, 0, 1, -3);     expect(adf_lball_Log, x, 20, ADF_NOT_DETERMINED, "Log(O(7^-3))");
    /* OK at the edge of each domain */
    lb_ball_si(x, 2, 0, 1, 2);      expect(adf_lball_exp, x, 20, ADF_OK, "exp(4 Z_2)");
    lb_ball_si(x, 3, 0, 1, 1);      expect(adf_lball_exp, x, 20, ADF_OK, "exp(3 Z_3)");
    lb_ball_si(x, 2, 3, 1, 1);      expect(adf_lball_log, x, 20, ADF_OK, "log(1 + 2 Z_2)");
    lb_ball_si(x, 5, 1, 1, 1);      expect(adf_lball_log, x, 20, ADF_OK, "log(1 + 5 Z_5)");
    /* LIMIT: an input beyond the bounds (tested before the domain) */
    lb_raw(x, 3, 1, one, one, EMAX + 1, 0);
    expect(adf_lball_Log, x, 20, ADF_LIMIT, "Log(3^(E+1))");
    expect(adf_lball_exp, x, 20, ADF_LIMIT, "exp(3^(E+1))");
    expect(adf_lball_log, x, 20, ADF_LIMIT, "log(3^(E+1)), outside the domain as well");
    lb_raw(x, 3, 0, one, one, 0, EMAX + 1);
    expect(adf_lball_Log, x, 20, ADF_LIMIT, "Log(1 + 3^(E+1) Z_3)");
    /* LIMIT: the result exponent beyond the bounds */
    lb_exact_si(x, 2, 4, 1);
    expect(adf_lball_exp, x, EMAX + 1, ADF_LIMIT, "exp(4), N = E + 1");
    expect(adf_lball_exp, x, -EMAX - 1, ADF_LIMIT, "exp(4), N = -E - 1");
    expect(adf_lball_exp, x, LONG_MIN, ADF_LIMIT, "exp(4), N = LONG_MIN");
    expect(adf_lball_exp, x, -EMAX, ADF_OK, "exp(4), N = -E: the ball O(2^-E)");
    lb_exact_si(x, 3, 2, 1);
    expect(adf_lball_Log, x, -EMAX - 1, ADF_LIMIT, "Log(2), N = -E - 1");
    expect(adf_lball_Log, x, 1, ADF_OK, "Log(2), N = 1: 3 Z_3, no sum");
    /* LIMIT: the working power p^W beyond ADF_LBALL_BITS_MAX (W >= K) */
    lb_exact_si(x, 2, 4, 1);
    expect(adf_lball_exp, x, ADF_LBALL_BITS_MAX / 2 + 1, ADF_LIMIT, "exp(4) at 2, K bits(2) above the bound");
    expect(adf_lball_exp, x, ADF_LBALL_BITS_MAX / 3, ADF_LIMIT, "exp(4) at 2, W = K + D above the bound");
    lb_exact_si(x, 2, 3, 1);
    expect(adf_lball_Log, x, ADF_LBALL_BITS_MAX / 2, ADF_LIMIT, "Log(3) at 2, W = K + e(T) above the bound");
    lb_exact_si(x, 3, 2, 1);
    expect(adf_lball_Log, x, ADF_LBALL_BITS_MAX / 2, ADF_LIMIT, "Log(2) at 3, W above the bound");
    /* no power, no limit, where the centre is known without a sum */
    lb_exact_si(x, 5, 0, 1);
    expect(adf_lball_exp, x, LONG_MAX, ADF_OK, "exp(0), N = LONG_MAX: exact 1");
    lb_exact_si(x, 5, 1, 1);
    expect(adf_lball_log, x, LONG_MAX, ADF_OK, "log(1), N = LONG_MAX: exact 0");
    lb_raw(x, 5, 1, one, one, EMAX, 0);
    expect(adf_lball_Log, x, LONG_MAX, ADF_OK, "Log(5^E): exact 0");
    lb_raw(x, 5, 0, one, one, 0, EMAX);
    expect(adf_lball_Log, x, EMAX, ADF_OK, "Log(1 + 5^E Z_5) = 5^E Z_5");
    {
        fmpz_t z0;
        fmpz_init(z0);
        lb_raw(x, 5, 0, z0, one, 0, EMAX);
        expect(adf_lball_exp, x, EMAX, ADF_OK, "exp(5^E Z_5) = 1 + 5^E Z_5");
        fmpz_clear(z0);
    }
    lb_raw(x, 3, 1, one, one, EMAX, 0);
    expect(adf_lball_exp, x, EMAX, ADF_OK, "exp(3^E), N = E: 1 + 3^E Z_3 (v(x) >= K)");
    adf_lball_clear(x);
    fmpz_clear(one);
}

/* The values behind the no-power cases above, and the exact results: fields checked. */
ADF_TEST(exact_results_and_shortcut_values)
{
    adf_lball_t x, y;
    fmpz_t one;
    adf_lball_init(x); adf_lball_init(y);
    fmpz_init_set_ui(one, 1);
    lb_exact_si(x, 5, 0, 1);
    ADF_CHECK(adf_lball_exp(y, x, 7) == ADF_OK && y->exact && fmpq_is_one(y->u) && y->v == 0);
    lb_exact_si(x, 2, -1, 1);
    ADF_CHECK(adf_lball_log(y, x, 7) == ADF_OK && y->exact && fmpq_is_zero(y->u));
    lb_exact_si(x, 3, -1, 9);
    ADF_CHECK(adf_lball_Log(y, x, 7) == ADF_OK && y->exact && fmpq_is_zero(y->u));
    lb_exact_si(x, 3, 1, 9);
    ADF_CHECK(adf_lball_Log(y, x, 7) == ADF_OK && y->exact && fmpq_is_zero(y->u));
    /* 2 * 3^k is not +-3^m: a ball */
    lb_exact_si(x, 3, 18, 1);
    ADF_CHECK(adf_lball_Log(y, x, 7) == ADF_OK && !y->exact && y->N == 7);
    /* a ball of centre 1 is not exact */
    lb_ball_si(x, 3, 1, 1, 4);
    ADF_CHECK(adf_lball_log(y, x, 7) == ADF_OK && !y->exact && y->N == 4 && fmpq_is_zero(y->u));
    lb_raw(x, 5, 0, one, one, 0, EMAX);
    ADF_CHECK(adf_lball_Log(y, x, EMAX) == ADF_OK && !y->exact && y->N == EMAX && fmpq_is_zero(y->u));
    lb_raw(x, 3, 1, one, one, EMAX, 0);
    ADF_CHECK(adf_lball_exp(y, x, EMAX) == ADF_OK && !y->exact && y->N == EMAX && fmpq_is_one(y->u) && y->v == 0);
    lb_exact_si(x, 2, 4, 1);
    ADF_CHECK(adf_lball_exp(y, x, -EMAX) == ADF_OK && !y->exact && y->N == -EMAX && fmpq_is_zero(y->u));
    /* exp 4 = 77 modulo 2^8 (FLINT, probe of lane d-functions): 1 mod 4, 5 mod 8, 13 mod 16 */
    ADF_CHECK(adf_lball_exp(y, x, 2) == ADF_OK && y->N == 2 && fmpq_is_one(y->u));
    ADF_CHECK(adf_lball_exp(y, x, 3) == ADF_OK && y->N == 3 && fmpz_equal_si(fmpq_numref(y->u), 5));
    ADF_CHECK(adf_lball_exp(y, x, 4) == ADF_OK && y->N == 4 && fmpz_equal_si(fmpq_numref(y->u), 13));
    adf_lball_clear(x); adf_lball_clear(y);
    fmpz_clear(one);
}

#ifdef ADF_CHECK_INVARIANTS
/* ------------------------------------------------------------------------------------ the entry check (INV) */

/* conventions 4.4: with -DADF_CHECK_INVARIANTS each function checks x on entry and aborts on a non-canonical x (a
   composite p). The output is not read. Each case in a child process: SIGABRT with a line naming the function; the
   canonical control returns normally and silently. */
static void
ic_child(int which, int forged)
{
    adf_lball_t x, y;
    adf_lball_init(x); adf_lball_init(y);
    lb_raw_si(x, 5, 0, 6, 0, 3);                         /* 6 + 125 Z_5, canonical */
    y->p = 4;                                            /* a forged output is overwritten, not read */
    if (forged)
        x->p = 4;
    if (which == 0)
        (void) adf_lball_exp(y, x, 5);
    else if (which == 1)
        (void) adf_lball_log(y, x, 5);
    else
        (void) adf_lball_Log(y, x, 5);
    x->p = 5;
    y->p = 5;
    adf_lball_clear(x); adf_lball_clear(y);
    flint_cleanup();
}

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
    close(fd[0]);
    if (waitpid(pid, &st, 0) != pid)
        abort();
    *exit_code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
    return WIFSIGNALED(st) ? WTERMSIG(st) : 0;
}

ADF_TEST(entry_check_of_every_function)
{
    static const char * const names[3] = {"adf_lball_exp", "adf_lball_log", "adf_lball_Log"};
    int i, code, sig;
    char err[512];
    for (i = 0; i < 3; i++)
    {
        sig = ic_run(i, 0, &code, err, sizeof err);
        ADF_CHECK_MSG(sig == 0 && code == 0 && err[0] == 0, "%s: control: signal %d, exit %d, stderr '%s'", names[i],
                      sig, code, err);
        sig = ic_run(i, 1, &code, err, sizeof err);
        ADF_CHECK_MSG(sig == SIGABRT, "%s: a forged argument did not abort (signal %d)", names[i], sig);
        ADF_CHECK_MSG(strstr(err, "ADF_CHECK_INVARIANTS") != NULL && strstr(err, names[i]) != NULL
                      && strstr(err, "adf_lball") != NULL, "%s: stderr '%s'", names[i], err);
    }
}
#endif /* ADF_CHECK_INVARIANTS */

/* The tables of the grid are freed last (the runner runs the tests in the order of the file). */
ADF_TEST(zz_free_the_grid)
{
    int g, k;
    if (!grids_loaded)
        return;
    for (g = 0; g < NGRID; g++)
        for (k = 0; k < 3; k++)
        {
            _fmpz_vec_clear(grids[g].value[k], grids[g].size);
            free(grids[g].have[k]);
        }
    grids_loaded = 0;
    flint_cleanup();
}
