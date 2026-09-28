/* tests/test_recon_vectors.c: adf_fball_reconstruct and adf_adele_reconstruct against the
   JSON-lines vector oracle.

   Every line of tests/ref/vectors/recon.jsonl is run (tests/ref/README.md, the table of the
   vector format: op reconstruct, the fields ball, lo, hi, the status none, one or several, and
   the list of solutions). The C code does not read the Python reference; the vectors are the
   fixture.

   For a line with status one the candidate of the function must be the recorded solution. For a
   line with status several the recorded list pins the whole candidate set without repeating the
   formula of Proposition 11: every recorded solution must lie in the closed interval and in the
   ball, consecutive ones must differ by the radius, the neighbours just outside the list must be
   outside the interval, and the length of the list must be the number of candidates an
   enumeration of a + N k finds. For a line with status none the enumeration must find no
   candidate at all. On both failure statuses the output of the function must be untouched
   (conventions 4.3).

   The extra vectors of the lane, tests/ref/vectors/m1-recon/, are read by the same code and were
   written with the Python reference adfref/recon.py by lanes/m1-recon/gen_vectors.py:
   recon_edges.jsonl has the format above and covers the end points of the closed interval, the
   width of the radius, radii with a denominator and operands of 4096 bits; recon_adele.jsonl
   carries a real ball as the exact numbers mid = mid_num * 2^mid_exp and rad = 2^rad_exp, plus
   the end points lo and hi of the closed interval, and drives adf_adele_reconstruct. */

#include <stdio.h>
#include <string.h>

#include <flint/arb.h>
#include <flint/arf.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include <flint/mag.h>

#include <adelefeld/adele.h>
#include <adelefeld/recon.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* The status of a vector line, "none", "one" or "several", as ADF_NO_SOLUTION, ADF_OK and
   ADF_NOT_UNIQUE (conventions 6.8); -1 for anything else. */
static int
want_status(const char * s, size_t len)
{
    if (len == 4 && memcmp(s, "none", 4) == 0)
        return ADF_NO_SOLUTION;
    if (len == 3 && memcmp(s, "one", 3) == 0)
        return ADF_OK;
    if (len == 7 && memcmp(s, "several", 7) == 0)
        return ADF_NOT_UNIQUE;
    return -1;
}

/* Read the integer of the JSON value into v, which is already initialised. */
static int
json_fmpz(fmpz_t v, const jsonl_value * j, jsonl_error_t * err)
{
    const char * text = jsonl_int_text(j, err);

    if (text == NULL)
        return 0;
    return fmpz_set_str(v, text, 10) == 0;
}

/* The rational {num, den} of the vector format, as a canonical fmpq. */
static int
json_fmpq(fmpq_t q, const jsonl_value * o, jsonl_error_t * err)
{
    const jsonl_value * jn, * jd;
    fmpz_t n, d;
    int ok;

    if (!jsonl_field(o, "num", &jn, err) || !jsonl_field(o, "den", &jd, err))
        return 0;
    fmpz_init(n);
    fmpz_init(d);
    ok = json_fmpz(n, jn, err) && json_fmpz(d, jd, err) && fmpz_cmp_ui(d, 0) != 0;
    if (ok)
        fmpq_set_fmpz_frac(q, n, d);
    fmpz_clear(n);
    fmpz_clear(d);
    return ok;
}

/* The ball {A, H, d} of the vector format, canonicalised. */
static int
json_ball(adf_fball_t x, const jsonl_value * o, jsonl_error_t * err)
{
    const jsonl_value * jA, * jH, * jd;
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

/* Is q the rational of the JSON object? */
static int
json_rat_equals(const fmpq_t q, const jsonl_value * o, jsonl_error_t * err)
{
    fmpq_t want;
    int equal;

    fmpq_init(want);
    if (!json_fmpq(want, o, err))
    {
        fmpq_clear(want);
        return 0;
    }
    equal = fmpq_equal(q, want);
    fmpq_clear(want);
    return equal;
}
/* The marker left in the output before every call; no recorded candidate is -99/7. */
static void
set_marker(adf_rat_t q)
{
    fmpq_set_si(q->q, -99, 7);
}

/* a + N k as a canonical rational, for an integer k. */
static void
candidate(fmpq_t c, const fmpq_t a, const fmpq_t N, slong k)
{
    fmpq_t t;

    fmpq_init(t);
    fmpq_set_si(t, k, 1);
    fmpq_mul(t, t, N);
    fmpq_add(c, t, a);
    fmpq_canonicalise(c);
    fmpq_clear(t);
}

/* count_candidates(a, N, lo, hi, first): the number of candidates of a + N Zhat in the closed
   interval, found by enumerating k upwards from 0 and downwards from -1 and comparing rationals:
   no floor and no ceiling. The candidates are contiguous in k (they are an arithmetic
   progression), so the two sweeps count them all; each sweep stops at the first k whose candidate
   is outside the interval on that side, and the guard stops a pathological input. The smallest
   candidate is written to first when there is one. Returns the count. */
static int
count_candidates(const fmpq_t a, const fmpq_t N, const fmpq_t lo, const fmpq_t hi, fmpq_t first)
{
    fmpq_t c;
    slong k;
    int n = 0, have = 0;

    if (fmpq_cmp(lo, hi) > 0)
        return 0;
    /* A ball of radius 0 is the point a. */
    if (fmpq_is_zero(N))
    {
        if (fmpq_cmp(lo, a) <= 0 && fmpq_cmp(a, hi) <= 0)
        {
            fmpq_set(first, a);
            return 1;
        }
        return 0;
    }
    fmpq_init(c);
    for (k = 0; k <= 1000000; k++)
    {
        candidate(c, a, N, k);
        if (fmpq_cmp(c, hi) > 0)
            break;
        if (fmpq_cmp(lo, c) <= 0)
        {
            if (!have)
            {
                fmpq_set(first, c);
                have = 1;
            }
            n++;
        }
    }
    for (k = -1; k >= -1000000; k--)
    {
        candidate(c, a, N, k);
        if (fmpq_cmp(c, lo) < 0)
            break;
        if (fmpq_cmp(c, hi) <= 0)
        {
            if (!have)
            {
                fmpq_set(first, c);
                have = 1;
            }
            n++;
        }
    }
    fmpq_clear(c);
    return n;
}

/* The exact interval of a real ball, read through arf: arb_get_interval_arf at `prec` bits and
   arf_get_fmpq on the two end points. Independent of the arb_get_interval_fmpz_2exp route of
   src/recon.c. [source pending: the entries of arf.rst and of arb.rst for these two functions;
   only arb.rst is on this machine. The declarations are /usr/include/flint/arb.h:355 and
   /usr/include/flint/arf.h:1125; 5000 round trips of arf_get_fmpq on dyadic arf values of up to
   100 bits returned the exact rational every time (lanes/m1-recon/report.md).] */
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

/* The centre and the radius of a ball, as fmpq. */
static void
ball_center_radius(fmpq_t a, fmpq_t N, const adf_fball_t x)
{
    adf_rat_t ra, rN;

    adf_rat_init(ra);
    adf_rat_init(rN);
    adf_fball_get_center(ra, x);
    adf_fball_get_radius(rN, x);
    fmpq_set(a, ra->q);
    fmpq_set(N, rN->q);
    adf_rat_clear(ra);
    adf_rat_clear(rN);
}

/* Check the recorded list of solutions against the ball (a, N) and the closed interval
   [lo, hi], and against the enumeration, which found `n` candidates. */
static void
check_solution_list(const jsonl_value * jsol, const char * path, size_t line,
                    jsonl_error_t * err, const fmpq_t a, const fmpq_t N, const fmpq_t lo,
                    const fmpq_t hi, int n)
{
    size_t nsol = jsonl_size(jsol);
    size_t j;
    fmpq_t prev, sol, diff, quot, first, last;

    ADF_CHECK_MSG((int) nsol == n, "%s line %lu: the enumeration found %d candidates, the file "
                                  "records %lu",
                  path, line, n, nsol);
    if (nsol == 0)
        return;
    fmpq_init(prev);
    fmpq_init(sol);
    fmpq_init(diff);
    fmpq_init(quot);
    fmpq_init(first);
    fmpq_init(last);
    for (j = 0; j < nsol; j++)
    {
        ADF_CHECK(json_fmpq(sol, jsonl_at(jsol, j, err), err));
        /* The recorded solution is in the closed interval. */
        ADF_CHECK_MSG(fmpq_cmp(lo, sol) <= 0 && fmpq_cmp(sol, hi) <= 0,
                      "%s line %lu: a recorded solution is outside the interval", path, line);
        /* The recorded solution is in a + N Zhat. */
        fmpq_sub(quot, sol, a);
        if (!fmpq_is_zero(N))
        {
            fmpq_div(quot, quot, N);
            ADF_CHECK_MSG(fmpz_is_one(fmpq_denref(quot)), "%s line %lu: a recorded solution is not in the "
                                                  "ball",
                          path, line);
        }
        if (j > 0)
        {
            fmpq_sub(diff, sol, prev);
            ADF_CHECK_MSG(fmpq_equal(diff, N), "%s line %lu: two consecutive recorded solutions do "
                                               "not differ by the radius",
                          path, line);
        }
        fmpq_set(prev, sol);
    }
    /* The candidates next to the list are outside the interval. */
    ADF_CHECK(json_fmpq(first, jsonl_at(jsol, 0, err), err));
    ADF_CHECK(json_fmpq(last, jsonl_at(jsol, nsol - 1, err), err));
    fmpq_sub(first, first, N);
    ADF_CHECK_MSG(fmpq_cmp(first, lo) < 0, "%s line %lu: the candidate before the first recorded "
                                           "one is inside the interval",
                  path, line);
    fmpq_add(last, last, N);
    ADF_CHECK_MSG(fmpq_cmp(last, hi) > 0, "%s line %lu: the candidate after the last recorded one "
                                          "is inside the interval",
                  path, line);
    fmpq_clear(prev);
    fmpq_clear(sol);
    fmpq_clear(diff);
    fmpq_clear(quot);
    fmpq_clear(first);
    fmpq_clear(last);
}

/* Check the status, the value and the state of the output of one call against a vector line. */
static void
check_result(const char * path, size_t line, jsonl_error_t * err, int got, int want,
             adf_rat_srcptr q, adf_rat_srcptr marker, const jsonl_value * jsol,
             const fmpq_t a, const fmpq_t N, const fmpq_t lo, const fmpq_t hi)
{
    fmpq_t first;
    int n;

    ADF_CHECK_MSG(got == want, "%s line %lu: the status is %s, expected %s", path, line,
                  adf_status_str(got), adf_status_str(want));
    fmpq_init(first);
    n = count_candidates(a, N, lo, hi, first);
    if (want == ADF_OK)
    {
        ADF_CHECK_MSG(n == 1, "%s line %lu: the enumeration found %d candidates, status one", path,
                      line, n);
        ADF_CHECK_MSG(jsonl_size(jsol) == 1, "%s line %lu: status one has one solution", path,
                      line);
        if (jsonl_size(jsol) == 1)
        {
            ADF_CHECK_MSG(json_rat_equals(q->q, jsonl_at(jsol, 0, err), err),
                          "%s line %lu: the candidate is not the recorded one", path, line);
            ADF_CHECK_MSG(fmpq_equal(first, q->q), "%s line %lu: the candidate is not the one the "
                                                   "enumeration found",
                          path, line);
        }
        ADF_CHECK(adf_rat_is_canonical(q));
    }
    else
    {
        if (want == ADF_NOT_UNIQUE)
        {
            ADF_CHECK_MSG(n >= 2, "%s line %lu: the enumeration found %d candidates, status "
                                  "several",
                          path, line, n);
            check_solution_list(jsol, path, line, err, a, N, lo, hi, n);
        }
        else
        {
            ADF_CHECK_MSG(n == 0, "%s line %lu: the enumeration found %d candidates, status none",
                          path, line, n);
            ADF_CHECK_MSG(jsonl_size(jsol) == 0, "%s line %lu: status none has no solution",
                          path, line);
        }
        ADF_CHECK_MSG(adf_rat_identical(q, marker) == 1,
                      "%s line %lu: the output was written on %s", path, line,
                      adf_status_str(got));
    }
    fmpq_clear(first);
}

/* The files of the vector format of adfref/recon.py: the ball, the two end points, the status and
   the list of solutions. Every line is run. */
static void
run_reconstruct_vectors(const char * path)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i, nlines;
    fmpq_t fa, fN, flo, fhi;

    ADF_CHECK_MSG(jsonl_open(path, &f, &err) == 1, "%s: %s", path, jsonl_error_message(&err));
    if (f == NULL)
        return;
    nlines = jsonl_count(f);
    ADF_CHECK_MSG(nlines > 0, "%s: the file has no line", path);
    fmpq_init(fa);
    fmpq_init(fN);
    fmpq_init(flo);
    fmpq_init(fhi);

    for (i = 0; i < nlines; i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * jball, * jlo, * jhi, * jstatus, * jsol;
        const char * status;
        size_t status_len;
        adf_fball_t x;
        adf_rat_t lo, hi, q, marker;
        int want, got;

        ADF_CHECK(jsonl_field(rec, "ball", &jball, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "lo", &jlo, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "hi", &jhi, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "status", &jstatus, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "solutions", &jsol, &err) == 1);
        status = jsonl_string(jstatus, &status_len, &err);
        want = want_status(status == NULL ? "" : status, status_len);
        ADF_CHECK_MSG(want >= 0, "%s line %lu: the status is not one of none, one, several", path,
                      jsonl_line_of(rec));

        adf_fball_init(x);
        adf_rat_init(lo);
        adf_rat_init(hi);
        adf_rat_init(q);
        adf_rat_init(marker);
        ADF_CHECK(json_ball(x, jball, &err));
        ADF_CHECK(json_fmpq(flo, jlo, &err));
        ADF_CHECK(json_fmpq(fhi, jhi, &err));
        ADF_CHECK(adf_rat_set_fmpq(lo, flo) == ADF_OK);
        ADF_CHECK(adf_rat_set_fmpq(hi, fhi) == ADF_OK);
        ball_center_radius(fa, fN, x);
        set_marker(q);
        set_marker(marker);
        got = adf_fball_reconstruct(q, x, lo, hi);
        check_result(path, jsonl_line_of(rec), &err, got, want, q, marker, jsol, fa, fN, flo,
                     fhi);

        adf_fball_clear(x);
        adf_rat_clear(lo);
        adf_rat_clear(hi);
        adf_rat_clear(q);
        adf_rat_clear(marker);
    }

    fmpq_clear(fa);
    fmpq_clear(fN);
    fmpq_clear(flo);
    fmpq_clear(fhi);
    jsonl_close(f);
}

ADF_TEST(vectors_recon)
{
    run_reconstruct_vectors("tests/ref/vectors/recon.jsonl");
}

ADF_TEST(vectors_recon_edges_of_the_lane)
{
    run_reconstruct_vectors("tests/ref/vectors/m1-recon/recon_edges.jsonl");
}

ADF_TEST(vectors_recon_adele)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i, nlines;
    fmpq_t fa, fN, flo, fhi, glo, ghi, mid, rad;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-recon/recon_adele.jsonl", &f, &err) == 1,
                  "tests/ref/vectors/m1-recon/recon_adele.jsonl: %s", jsonl_error_message(&err));
    if (f == NULL)
        return;
    nlines = jsonl_count(f);
    ADF_CHECK_MSG(nlines > 0, "the adele vector file has no line");
    fmpq_init(fa);
    fmpq_init(fN);
    fmpq_init(flo);
    fmpq_init(fhi);
    fmpq_init(glo);
    fmpq_init(ghi);
    fmpq_init(mid);
    fmpq_init(rad);

    for (i = 0; i < nlines; i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * jmn, * jme, * jre, * jball, * jlo, * jhi, * jstatus, * jsol;
        const char * status;
        size_t status_len;
        adf_adele_t x;
        adf_rat_t q, marker, lo, hi;
        fmpz_t mn, me, re, one;
        int want, got;

        ADF_CHECK(jsonl_field(rec, "mid_num", &jmn, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "mid_exp", &jme, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "rad_exp", &jre, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "ball", &jball, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "lo", &jlo, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "hi", &jhi, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "status", &jstatus, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "solutions", &jsol, &err) == 1);
        status = jsonl_string(jstatus, &status_len, &err);
        want = want_status(status == NULL ? "" : status, status_len);
        ADF_CHECK_MSG(want >= 0, "adele line %lu: the status is not one of none, one, several",
                      jsonl_line_of(rec));

        fmpz_init(mn);
        fmpz_init(me);
        fmpz_init(re);
        fmpz_init_set_ui(one, 1);
        ADF_CHECK(json_fmpz(mn, jmn, &err));
        ADF_CHECK(json_fmpz(me, jme, &err));
        ADF_CHECK(json_fmpz(re, jre, &err));
        arb_init(x->inf);
        adf_fball_init(&x->fin);
        ADF_CHECK(json_ball(&x->fin, jball, &err));
        /* The real ball [mid - rad, mid + rad] with mid = mid_num * 2^mid_exp and
           rad = 2^rad_exp, exactly: arb_set_fmpz_2exp writes the midpoint without rounding and
           the radius zero (/usr/include/flint/arb.h:187), the radius is then a mag of one limb
           (mag_set_fmpz_2exp_fmpz, /usr/include/flint/mag.h:479), and the accessors are the
           macros of /usr/include/flint/arb.h:38-39. */
        arb_set_fmpz_2exp(x->inf, mn, me);
        mag_set_fmpz_2exp_fmpz(arb_radref(x->inf), one, re);
        ADF_CHECK(arb_is_finite(x->inf));

        /* The end points, from the exact numbers of the vector. */
        fmpz_set(fmpq_numref(mid), mn);
        fmpz_one(fmpq_denref(mid));
        if (fmpz_sgn(me) >= 0)
            fmpz_mul_2exp(fmpq_numref(mid), fmpq_numref(mid), (ulong) fmpz_get_ui(me));
        else
            fmpz_mul_2exp(fmpq_denref(mid), fmpq_denref(mid), (ulong) (-fmpz_get_si(me)));
        fmpq_canonicalise(mid);
        fmpq_set_si(rad, 1, 1);
        if (fmpz_sgn(re) >= 0)
            fmpz_mul_2exp(fmpq_numref(rad), fmpq_numref(rad), (ulong) fmpz_get_ui(re));
        else
            fmpz_mul_2exp(fmpq_denref(rad), fmpq_denref(rad), (ulong) (-fmpz_get_si(re)));
        fmpq_canonicalise(rad);
        fmpq_sub(flo, mid, rad);
        fmpq_add(fhi, mid, rad);
        ADF_CHECK_MSG(json_rat_equals(flo, jlo, &err), "adele line %lu: the vector records an end "
                                                        "point that is not mid - rad",
                      jsonl_line_of(rec));
        ADF_CHECK_MSG(json_rat_equals(fhi, jhi, &err), "adele line %lu: the vector records an end "
                                                        "point that is not mid + rad",
                      jsonl_line_of(rec));
        /* The ball built here is exactly the interval of the vector. */
        real_interval(glo, ghi, x->inf, 8192);
        ADF_CHECK_MSG(fmpq_equal(flo, glo) && fmpq_equal(fhi, ghi),
                      "adele line %lu: the real ball built from the vector is not that interval",
                      jsonl_line_of(rec));

        adf_rat_init(lo);
        adf_rat_init(hi);
        adf_rat_init(q);
        adf_rat_init(marker);
        ADF_CHECK(adf_rat_set_fmpq(lo, flo) == ADF_OK);
        ADF_CHECK(adf_rat_set_fmpq(hi, fhi) == ADF_OK);
        ball_center_radius(fa, fN, &x->fin);
        set_marker(q);
        set_marker(marker);
        got = adf_adele_reconstruct(q, x);
        check_result("tests/ref/vectors/m1-recon/recon_adele.jsonl", jsonl_line_of(rec), &err, got,
                     want, q, marker, jsol, fa, fN, flo, fhi);

        adf_rat_clear(lo);
        adf_rat_clear(hi);
        adf_rat_clear(q);
        adf_rat_clear(marker);
        arb_clear(x->inf);
        adf_fball_clear(&x->fin);
        fmpz_clear(mn);
        fmpz_clear(me);
        fmpz_clear(re);
        fmpz_clear(one);
    }

    fmpq_clear(fa);
    fmpq_clear(fN);
    fmpq_clear(flo);
    fmpq_clear(fhi);
    fmpq_clear(glo);
    fmpq_clear(ghi);
    fmpq_clear(mid);
    fmpq_clear(rad);
    jsonl_close(f);
}
