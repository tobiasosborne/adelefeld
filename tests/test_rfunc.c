/* tests/test_rfunc.c: the real functions at the archimedean place (lane f-slice2; include/adelefeld/rfunc.h;
   docs/api-1f.md statements S5 to S7).

   Oracles.
   1. The vectors tests/ref/vectors/f-slice2/rfunc_real.jsonl, made by lanes/f-slice2/gen_vectors.py from the section
      f-slice2 of proto/functions_checks.py: the image of the input ball is enclosed by mpmath intervals at 800 bits
      (exp, log, sin, cos) or by exact integer roots (sqrt, root); the status is decided on the exact end points.
      Every line is run at the arb level and at the level of the partial ball. A line asserts: the status; the
      output untouched on a status; on OK, the result contains [lo, hi] (the bounds of the image, widened by 2^-600),
      and, when the line says "tight", the radius is at most 4 times the true width plus the rounding of prec bits;
      for sin and cos without "tight": the radius is at most the input radius plus the rounding.
   2. Hand tests of what arb does badly and what the wrapper promises: the odd root of a negative ball and of 0 (arb
      returns NaN), the even root of 0, every non-finite input, huge arguments, prec below 2, aliasing.
   3. The statuses and the reported place of the functions on a partial ball.

   What would make a case fail is stated at each test. */

#include <limits.h>
#include <math.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/arb.h>
#include <flint/arf.h>
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

/* x = {"m", "e", "rm", "re"}: the ball m 2^e +- rm 2^re (rm < 2^30, so the radius is exact). */
static void
rb_from_json(arb_t x, const jsonl_value * o)
{
    fmpz_t m, e;
    fmpz_init(m);
    fmpz_init(e);
    ADF_CHECK(fmpz_set_str(m, member_int(o, "m"), 10) == 0);
    ADF_CHECK(fmpz_set_str(e, member_int(o, "e"), 10) == 0);
    arb_set_fmpz_2exp(x, m, e);
    mag_set_ui_2exp_si(arb_radref(x), strtoul(member_int(o, "rm"), NULL, 10), strtol(member_int(o, "re"), NULL, 10));
    fmpz_clear(m);
    fmpz_clear(e);
}

static void
dy_from_json(arf_t x, const jsonl_value * o)
{
    fmpz_t m, e;
    fmpz_init(m);
    fmpz_init(e);
    ADF_CHECK(fmpz_set_str(m, member_int(o, "m"), 10) == 0);
    ADF_CHECK(fmpz_set_str(e, member_int(o, "e"), 10) == 0);
    arf_set_fmpz_2exp(x, m, e);
    fmpz_clear(m);
    fmpz_clear(e);
}

typedef int (*real_fn)(arb_t, const arb_t, slong);
typedef int (*sball_fn)(adf_sball_t, adf_place_t *, const adf_sball_t, adf_place_t, slong);

typedef struct
{
    const char * name;
    real_fn f;
    sball_fn g;
} fn_entry;

static const fn_entry FNS[] = {{"exp", adf_real_exp, adf_sball_exp_at},
                               {"log", adf_real_log, adf_sball_log_at},
                               {"log_abs", adf_real_log_abs, adf_sball_log_abs_at},
                               {"sin", adf_real_sin, adf_sball_sin_at},
                               {"cos", adf_real_cos, adf_sball_cos_at},
                               {"sqrt", adf_real_sqrt, adf_sball_sqrt_at}};
#define NFNS (sizeof FNS / sizeof FNS[0])

static const fn_entry *
find_fn(const char * name)
{
    size_t i;
    for (i = 0; i < NFNS; i++)
        if (strcmp(FNS[i].name, name) == 0)
            return &FNS[i];
    return NULL;
}

/* One call of the function `f` (root: with the degree n) at the arb level. */
static int
call_real(const char * f, ulong n, arb_t y, const arb_t x, slong prec)
{
    if (strcmp(f, "root") == 0)
        return adf_real_root(y, x, n, prec);
    return find_fn(f)->f(y, x, prec);
}

static int
call_sball(const char * f, ulong n, adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v,
           slong prec)
{
    if (strcmp(f, "root") == 0)
        return adf_sball_root_at(y, where, x, v, n, prec);
    return find_fn(f)->g(y, where, x, v, prec);
}

/* A partial ball with only the real component r. */
static void
sball_real(adf_sball_t s, const arb_t r)
{
    int st = adf_sball_set_arb_lballs(s, NULL, r, NULL, 0);
    ADF_CHECK(st == ADF_OK);
}

/* --------------------------------------------------------------------------------------------- the vectors */

/* The result R (a ball) against the row: contains [lo, hi] widened by 2^-350 max(|lo|, |hi|, 2^-50) (the bounds
   are written outward-rounded at 400 bits); the radius bound. */
static void
check_enclosure(const char * f, const arb_t R, const arb_t x, slong prec, const jsonl_value * rec)
{
    arb_t Ri;
    arf_t lo, hi, wid, rad, in_rad, lhs, rhs, t, mx, slack;
    slong p = prec < 2 ? 2 : prec;
    int tight = (int) strtol(member_int(rec, "tight"), NULL, 10);
    unsigned long line = jsonl_line_of(rec);
    const slong W = 3000;

    arb_init(Ri);
    arf_init(lo);
    arf_init(hi);
    arf_init(wid);
    arf_init(rad);
    arf_init(in_rad);
    arf_init(lhs);
    arf_init(rhs);
    arf_init(t);
    arf_init(mx);
    arf_init(slack);
    dy_from_json(lo, member(rec, "lo"));
    dy_from_json(hi, member(rec, "hi"));
    arf_sub(wid, hi, lo, W, ARF_RND_UP);
    arf_abs(t, lo);
    arf_abs(mx, hi);
    arf_max(mx, mx, t);
    arf_set_si_2exp_si(t, 1, -50);
    arf_max(slack, mx, t);
    arf_mul_2exp_si(slack, slack, -350);

    ADF_CHECK_MSG(arb_is_finite(R), "%s line %lu: result not finite", f, line);
    arb_set(Ri, R);
    arb_add_error_arf(Ri, slack);
    ADF_CHECK_MSG(arb_contains_arf(Ri, lo) && arb_contains_arf(Ri, hi), "%s line %lu: result does not enclose the image",
                  f, line);

    arf_set_mag(rad, arb_radref(R));
    if (tight)
    {
        /* 2 rad <= 4 (hi - lo) + 2^(5 - p) max(|lo|, |hi|) + 8 slack */
        arf_mul_2exp_si(lhs, rad, 1);
        arf_mul_2exp_si(t, mx, 5 - p);
        arf_mul_2exp_si(rhs, wid, 2);
        arf_add(rhs, rhs, t, W, ARF_RND_UP);
        arf_mul_2exp_si(t, slack, 3);
        arf_add(rhs, rhs, t, W, ARF_RND_UP);
        ADF_CHECK_MSG(arf_cmp(lhs, rhs) <= 0, "%s line %lu: radius %g too large for width %g at prec %ld", f, line,
                      arf_get_d(rad, ARF_RND_NEAR), arf_get_d(wid, ARF_RND_NEAR), (long) prec);
    }
    else if (strcmp(f, "sin") == 0 || strcmp(f, "cos") == 0)
    {
        /* Lipschitz 1: rad <= input radius + 2^(4 - p) */
        arf_set_mag(in_rad, arb_radref(x));
        arf_set_si_2exp_si(t, 1, 4 - p);
        arf_add(rhs, in_rad, t, W, ARF_RND_UP);
        ADF_CHECK_MSG(arf_cmp(rad, rhs) <= 0, "%s line %lu: radius %g above the Lipschitz bound", f, line,
                      arf_get_d(rad, ARF_RND_NEAR));
    }

    arb_clear(Ri);
    arf_clear(lo);
    arf_clear(hi);
    arf_clear(wid);
    arf_clear(rad);
    arf_clear(in_rad);
    arf_clear(lhs);
    arf_clear(rhs);
    arf_clear(t);
    arf_clear(mx);
    arf_clear(slack);
}

static void
real_row(const jsonl_value * rec)
{
    const char * f = member_str(rec, "f");
    ulong n = strtoul(member_int(rec, "n"), NULL, 10);
    slong prec = strtol(member_int(rec, "prec"), NULL, 10);
    int want = status_from_name(member_str(rec, "status"));
    unsigned long line = jsonl_line_of(rec);
    arb_t x, y, z, sentinel;
    adf_sball_t sx, sy;
    adf_place_t where, inf = adf_place_inf(), mark = place_of(1000003);
    int st, st2;

    arb_init(x);
    arb_init(y);
    arb_init(z);
    arb_init(sentinel);
    rb_from_json(x, member(rec, "x"));
    arb_set_si(sentinel, 12345);
    arb_set(y, sentinel);

    st = call_real(f, n, y, x, prec);
    ADF_CHECK_MSG(st == want, "%s n=%lu line %lu: status %s, want %s", f, n, line, adf_status_str(st),
                  adf_status_str(want));
    if (st == ADF_OK && want == ADF_OK)
        check_enclosure(f, y, x, prec, rec);
    else
        ADF_CHECK_MSG(arb_equal(y, sentinel), "%s line %lu: output written on a status", f, line);

    /* aliasing: y = x gives the same ball on OK and leaves x on a status */
    arb_set(z, x);
    st2 = call_real(f, n, z, z, prec);
    ADF_CHECK_MSG(st2 == st, "%s line %lu: aliased status", f, line);
    ADF_CHECK_MSG(arb_equal(z, st == ADF_OK ? y : x), "%s line %lu: aliased result", f, line);

    /* the partial ball: the same status, the result at the one place, the place reported on failure */
    adf_sball_init(sx);
    adf_sball_init(sy);
    sball_real(sx, x);
    sball_real(sy, sentinel);
    where = mark;
    st2 = call_sball(f, n, sy, &where, sx, inf, prec);
    ADF_CHECK_MSG(st2 == st, "%s line %lu: sball status %s, arb status %s", f, line, adf_status_str(st2),
                  adf_status_str(st));
    if (st == ADF_OK)
    {
        ADF_CHECK_MSG(adf_place_equal(where, mark), "%s line %lu: where written on OK", f, line);
        ADF_CHECK_MSG(adf_sball_is_canonical(sy) && adf_sball_arch(sy) == ADF_ARCH_REAL && sy->len == 0 &&
                          adf_sball_num_places(sy) == 1,
                      "%s line %lu: result is not a partial ball over the one place", f, line);
        ADF_CHECK_MSG(arb_equal(acb_realref(sy->inf), y) && arb_is_zero(acb_imagref(sy->inf)),
                      "%s line %lu: sball result differs from the arb result", f, line);
    }
    else
    {
        int degree_zero = strcmp(f, "root") == 0 && n == 0;
        ADF_CHECK_MSG(adf_place_equal(where, degree_zero ? mark : inf), "%s line %lu: where is wrong", f, line);
        ADF_CHECK_MSG(arb_equal(acb_realref(sy->inf), sentinel), "%s line %lu: sball output written", f, line);
    }
    adf_sball_clear(sx);
    adf_sball_clear(sy);
    arb_clear(x);
    arb_clear(y);
    arb_clear(z);
    arb_clear(sentinel);
}

ADF_TEST(vectors_real)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;
    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/f-slice2/rfunc_real.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    ADF_CHECK_MSG(jsonl_count(f) >= 1000, "only %lu lines", (unsigned long) jsonl_count(f));
    for (i = 0; i < jsonl_count(f); i++)
        real_row(jsonl_record(f, i));
    jsonl_close(f);
}

/* ----------------------------------------------------------------------------------- what arb does badly */

/* arb_root_ui returns NaN for the odd root of a negative ball and of 0 (lane d-functions, probe). The wrapper
   returns a finite ball that contains the root. A wrapper that called arb_root_ui directly fails here. */
ADF_TEST(odd_roots_of_negative_and_zero)
{
    arb_t x, y, t;
    int st;
    arb_init(x);
    arb_init(y);
    arb_init(t);

    arb_set_si(x, -8);
    st = adf_real_root(y, x, 3, 64);
    ADF_CHECK(st == ADF_OK && arb_is_finite(y) && arb_contains_si(y, -2));
    ADF_CHECK(arb_is_exact(y) || arb_rel_error_bits(y) < -50);

    arb_zero(x);
    arb_set_si(y, 77);
    st = adf_real_root(y, x, 3, 64);
    ADF_CHECK(st == ADF_OK && arb_is_zero(y));

    /* -8 +- 1: contains -9^(1/3) .. -7^(1/3) */
    arb_set_si(x, -8);
    mag_one(arb_radref(x));
    st = adf_real_root(y, x, 3, 100);
    arb_set_si(t, -9);
    arb_root_ui(t, t, 3, 200);   /* NaN: only the negation trick works */
    ADF_CHECK(st == ADF_OK && arb_is_finite(y));
    arb_set_si(t, 9);
    arb_root_ui(t, t, 3, 200);
    arb_neg(t, t);
    ADF_CHECK(arb_contains(y, t));
    arb_set_si(t, 7);
    arb_root_ui(t, t, 3, 200);
    arb_neg(t, t);
    ADF_CHECK(arb_contains(y, t));

    /* a ball across 0: [-1, 1], the root is enclosed by the images of the end points: contains [-1, 1] */
    arb_zero(x);
    mag_one(arb_radref(x));
    st = adf_real_root(y, x, 3, 64);
    ADF_CHECK(st == ADF_OK && arb_is_finite(y) && arb_contains_si(y, -1) && arb_contains_si(y, 1) &&
              arb_contains_zero(y));
    st = adf_real_root(y, x, 5, 64);
    ADF_CHECK(st == ADF_OK && arb_contains_si(y, -1) && arb_contains_si(y, 1));

    /* the even root of exactly 0 is 0, of [0, 2] is a ball that contains [0, 2^(1/n)] */
    arb_zero(x);
    arb_set_si(y, 5);
    st = adf_real_root(y, x, 4, 64);
    ADF_CHECK(st == ADF_OK && arb_is_zero(y));
    st = adf_real_sqrt(y, x, 64);
    ADF_CHECK(st == ADF_OK && arb_is_zero(y));
    arb_set_si(x, 1);
    mag_one(arb_radref(x));
    st = adf_real_sqrt(y, x, 64);
    ADF_CHECK(st == ADF_OK && arb_contains_zero(y) && arb_is_finite(y));
    arb_sqrt_ui(t, 2, 200);
    ADF_CHECK(arb_contains(y, t));

    arb_clear(x);
    arb_clear(y);
    arb_clear(t);
}

/* Every function on every kind of non-finite input: DOMAIN and the output untouched. A wrapper that passed the NaN
   to arb would return OK with a NaN, or NOT_DETERMINED. */
ADF_TEST(non_finite_inputs)
{
    arb_t x[4], y, s;
    size_t i, k;
    arb_init(y);
    arb_init(s);
    for (i = 0; i < 4; i++)
        arb_init(x[i]);
    arb_indeterminate(x[0]);
    arb_pos_inf(x[1]);
    arb_neg_inf(x[2]);
    arb_zero_pm_inf(x[3]);
    arb_set_si(s, 99);
    for (i = 0; i < 4; i++)
    {
        for (k = 0; k < NFNS; k++)
        {
            arb_set(y, s);
            ADF_CHECK_MSG(FNS[k].f(y, x[i], 53) == ADF_DOMAIN && arb_equal(y, s), "%s input %lu", FNS[k].name,
                          (unsigned long) i);
        }
        arb_set(y, s);
        ADF_CHECK(adf_real_root(y, x[i], 3, 53) == ADF_DOMAIN && arb_equal(y, s));
        arb_set(y, s);
        ADF_CHECK(adf_real_root(y, x[i], 2, 53) == ADF_DOMAIN && arb_equal(y, s));
    }
    arb_clear(y);
    arb_clear(s);
    for (i = 0; i < 4; i++)
        arb_clear(x[i]);
}

/* Huge arguments: a status is OK with a finite ball, or NOT_DETERMINED with the output untouched; never a NaN or an
   infinity with OK, and no hang (the test runs under timeout). */
ADF_TEST(huge_arguments_never_give_a_nonfinite_ok)
{
    arb_t x, y, s;
    fmpz_t e;
    size_t k, j;
    slong exps[] = {60, 1000, 100000};
    arb_init(x);
    arb_init(y);
    arb_init(s);
    fmpz_init(e);
    arb_set_si(s, 99);
    for (j = 0; j < 3; j++)
    {
        int neg;
        for (neg = 0; neg < 2; neg++)
        {
            arb_one(x);
            arb_mul_2exp_si(x, x, exps[j]);
            if (neg)
                arb_neg(x, x);
            for (k = 0; k < NFNS; k++)
            {
                int st;
                arb_set(y, s);
                st = FNS[k].f(y, x, 64);
                ADF_CHECK_MSG(st == ADF_OK ? arb_is_finite(y) : arb_equal(y, s), "%s 2^%ld neg %d status %d",
                              FNS[k].name, (long) exps[j], neg, st);
            }
            arb_set(y, s);
            {
                int st = adf_real_root(y, x, 3, 64);
                ADF_CHECK(st == ADF_OK ? arb_is_finite(y) : arb_equal(y, s));
            }
        }
    }
    /* huge exponent 2^(2^62) as an fmpz exponent */
    fmpz_one(e);
    fmpz_mul_2exp(e, e, 62);
    arb_one(x);
    arb_mul_2exp_fmpz(x, x, e);
    for (k = 0; k < NFNS; k++)
    {
        int st;
        arb_set(y, s);
        st = FNS[k].f(y, x, 64);
        ADF_CHECK_MSG(st == ADF_OK ? arb_is_finite(y) : arb_equal(y, s), "%s 2^(2^62) status %d", FNS[k].name, st);
    }
    fmpz_clear(e);
    arb_clear(x);
    arb_clear(y);
    arb_clear(s);
}

/* A prec below 2 is taken as 2 (decision M1-D4): the same ball as at prec 2, exactly. */
ADF_TEST(prec_below_two_is_two)
{
    const slong precs[] = {1, 0, -1, -100, LONG_MIN + 1};
    size_t i, k, j;
    arb_t x, r2, r;
    arb_init(x);
    arb_init(r2);
    arb_init(r);
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 4; j++)
        {
            arb_set_si(x, (slong) j * 3 + 1);
            arb_div_ui(x, x, 7, 200);
            arb_get_mid_arb(x, x);
            mag_set_ui_2exp_si(arb_radref(x), (ulong) j, -6);
            for (k = 0; k < NFNS; k++)
            {
                int s2 = FNS[k].f(r2, x, 2);
                int s1 = FNS[k].f(r, x, precs[i]);
                ADF_CHECK_MSG(s1 == s2 && (s1 != ADF_OK || arb_equal(r, r2)), "%s prec %ld", FNS[k].name,
                              (long) precs[i]);
            }
            ADF_CHECK(adf_real_root(r, x, 3, precs[i]) == ADF_OK && adf_real_root(r2, x, 3, 2) == ADF_OK &&
                      arb_equal(r, r2));
        }
    }
    arb_clear(x);
    arb_clear(r2);
    arb_clear(r);
}

/* The domain table of rfunc.h, one row for each corner: exact 0, exact negative, the end point 0, a ball across 0. */
ADF_TEST(domain_table)
{
    struct
    {
        const char * f;
        long m;      /* midpoint */
        ulong r;     /* radius 2^-e times... radius = r (an integer) */
        int want;
    } rows[] = {
        /* log: DOMAIN if hi <= 0, OK if lo > 0 */
        {"log", 0, 0, ADF_DOMAIN}, {"log", -3, 0, ADF_DOMAIN}, {"log", -1, 1, ADF_DOMAIN}, {"log", -3, 1, ADF_DOMAIN},
        {"log", 1, 1, ADF_NOT_DETERMINED}, {"log", 0, 1, ADF_NOT_DETERMINED}, {"log", 2, 1, ADF_OK},
        {"log", 3, 0, ADF_OK},
        /* log_abs: DOMAIN only for the exact 0 */
        {"log_abs", 0, 0, ADF_DOMAIN}, {"log_abs", 0, 1, ADF_NOT_DETERMINED}, {"log_abs", 1, 1, ADF_NOT_DETERMINED},
        {"log_abs", -1, 1, ADF_NOT_DETERMINED}, {"log_abs", -3, 1, ADF_OK}, {"log_abs", 3, 1, ADF_OK},
        {"log_abs", -5, 0, ADF_OK},
        /* sqrt: DOMAIN if hi < 0, OK if lo >= 0 */
        {"sqrt", 0, 0, ADF_OK}, {"sqrt", -1, 0, ADF_DOMAIN}, {"sqrt", -2, 1, ADF_DOMAIN}, {"sqrt", -1, 1, ADF_NOT_DETERMINED},
        {"sqrt", 0, 1, ADF_NOT_DETERMINED}, {"sqrt", 1, 1, ADF_OK}, {"sqrt", 4, 1, ADF_OK}, {"sqrt", 4, 0, ADF_OK}};
    size_t i;
    arb_t x, y;
    arb_init(x);
    arb_init(y);
    for (i = 0; i < sizeof rows / sizeof rows[0]; i++)
    {
        int st;
        arb_set_si(x, rows[i].m);
        if (rows[i].r)
            mag_set_ui(arb_radref(x), rows[i].r);
        arb_set_si(y, 4242);
        st = call_real(rows[i].f, 0, y, x, 64);
        ADF_CHECK_MSG(st == rows[i].want, "%s(%ld +- %lu): status %s want %s", rows[i].f, rows[i].m, rows[i].r,
                      adf_status_str(st), adf_status_str(rows[i].want));
        ADF_CHECK(st == ADF_OK ? arb_is_finite(y) : arb_equal_si(y, 4242));
    }
    /* even roots follow sqrt; odd roots are OK everywhere; degree 0 is DOMAIN */
    for (i = 0; i < sizeof rows / sizeof rows[0]; i++)
    {
        int s2, s4, s3;
        if (strcmp(rows[i].f, "sqrt") != 0)
            continue;
        arb_set_si(x, rows[i].m);
        if (rows[i].r)
            mag_set_ui(arb_radref(x), rows[i].r);
        s2 = adf_real_root(y, x, 2, 64);
        s4 = adf_real_root(y, x, 4, 64);
        s3 = adf_real_root(y, x, 3, 64);
        ADF_CHECK(s2 == rows[i].want && s4 == rows[i].want && s3 == ADF_OK);
        ADF_CHECK(adf_real_root(y, x, 0, 64) == ADF_DOMAIN);
        ADF_CHECK(adf_real_root(y, x, 1, 64) == ADF_OK && arb_contains(y, x));
        ADF_CHECK(adf_real_root(y, x, 1ul << 63, 64) == (rows[i].want == ADF_DOMAIN || rows[i].want == ADF_NOT_DETERMINED
                                                             ? rows[i].want : ADF_OK));
    }
    arb_clear(x);
    arb_clear(y);
}

/* Known values: the enclosure contains them and is narrow (a wrong function fails). */
ADF_TEST(known_values)
{
    arb_t x, y, t;
    arb_init(x);
    arb_init(y);
    arb_init(t);
    arb_set_si(x, 1);
    ADF_CHECK(adf_real_exp(y, x, 100) == ADF_OK);
    arb_const_e(t, 200);
    ADF_CHECK(arb_contains(y, t) && arb_rel_error_bits(y) < -95);
    arb_set_si(x, 2);
    ADF_CHECK(adf_real_log(y, x, 100) == ADF_OK);
    arb_const_log2(t, 200);
    ADF_CHECK(arb_contains(y, t) && arb_rel_error_bits(y) < -95);
    arb_set_si(x, -2);
    ADF_CHECK(adf_real_log_abs(y, x, 100) == ADF_OK && arb_contains(y, t));
    arb_set_si(x, 1);
    ADF_CHECK(adf_real_sin(y, x, 100) == ADF_OK);
    arb_sin(t, x, 200);
    ADF_CHECK(arb_contains(y, t) && arb_rel_error_bits(y) < -95);
    ADF_CHECK(adf_real_cos(y, x, 100) == ADF_OK);
    arb_cos(t, x, 200);
    ADF_CHECK(arb_contains(y, t) && arb_rel_error_bits(y) < -95);
    arb_set_si(x, 2);
    ADF_CHECK(adf_real_sqrt(y, x, 100) == ADF_OK);
    arb_sqrt_ui(t, 2, 200);
    ADF_CHECK(arb_contains(y, t) && arb_rel_error_bits(y) < -95);
    arb_set_si(x, 27);
    ADF_CHECK(adf_real_root(y, x, 3, 100) == ADF_OK && arb_contains_si(y, 3));
    arb_set_si(x, -27);
    ADF_CHECK(adf_real_root(y, x, 3, 100) == ADF_OK && arb_contains_si(y, -3));
    arb_set_si(x, 16);
    ADF_CHECK(adf_real_root(y, x, 4, 100) == ADF_OK && arb_contains_si(y, 2) && !arb_contains_si(y, -2));
    arb_clear(x);
    arb_clear(y);
    arb_clear(t);
}

/* ---------------------------------------------------------------------------------- partial balls, statuses */

static void
make_adele(adf_adele_t a, slong num, slong den, slong prec)
{
    adf_rat_t q;
    fmpz_t n, d;
    int st;
    adf_rat_init(q);
    fmpz_init_set_si(n, num);
    fmpz_init_set_si(d, den);
    st = adf_rat_set_fmpz2(q, n, d);
    ADF_CHECK(st == ADF_OK);
    adf_adele_set_rat(a, q, prec);
    fmpz_clear(n);
    fmpz_clear(d);
    adf_rat_clear(q);
}

/* The order of the checks on v: not a place -> DOMAIN (where = v); a prime of x -> UNSUPPORTED (where = v); a COMPLEX
   tag -> UNSUPPORTED (where = inf); then the status of the real function with where = inf. y untouched on each. A
   function that checked the real failure first, or wrote y, fails. */
ADF_TEST(sball_at_statuses_and_places)
{
    adf_adele_t a;
    adf_sball_t x, y, w;
    adf_place_t places3[3], v, where, mark = place_of(1000003), inf = adf_place_inf();
    size_t k;
    arb_t r;

    adf_adele_init(a);
    make_adele(a, 2, 3, 200);
    places3[0] = place_of(5);
    places3[1] = inf;
    places3[2] = place_of(2);
    adf_sball_init(x);
    adf_sball_init(y);
    adf_sball_init(w);
    ADF_CHECK(adf_sball_project(x, NULL, a, places3, 3) == ADF_OK);
    ADF_CHECK(adf_sball_num_places(x) == 3);

    /* a place that is not in x */
    for (k = 0; k < NFNS + 1; k++)
    {
        int st;
        adf_sball_set(y, x);
        where = mark;
        v = place_of(3);
        st = k < NFNS ? FNS[k].g(y, &where, x, v, 53) : adf_sball_root_at(y, &where, x, v, 3, 53);
        ADF_CHECK_MSG(st == ADF_DOMAIN && adf_place_equal(where, v) && adf_sball_identical(y, x), "k=%lu", (unsigned long) k);
        /* a prime of x */
        v = place_of(5);
        where = mark;
        st = k < NFNS ? FNS[k].g(y, &where, x, v, 53) : adf_sball_root_at(y, &where, x, v, 3, 53);
        ADF_CHECK_MSG(st == ADF_UNSUPPORTED && adf_place_equal(where, v) && adf_sball_identical(y, x), "k=%lu",
                      (unsigned long) k);
        v = place_of(2);
        where = mark;
        st = k < NFNS ? FNS[k].g(y, &where, x, v, 53) : adf_sball_root_at(y, &where, x, v, 3, 53);
        ADF_CHECK(st == ADF_UNSUPPORTED && adf_place_equal(where, v) && adf_sball_identical(y, x));
        /* NULL where is allowed */
        st = k < NFNS ? FNS[k].g(y, NULL, x, v, 53) : adf_sball_root_at(y, NULL, x, v, 3, 53);
        ADF_CHECK(st == ADF_UNSUPPORTED);
    }

    /* no archimedean place in x (tag NONE): the archimedean place is not a place of x */
    places3[1] = place_of(7);
    ADF_CHECK(adf_sball_project(w, NULL, a, places3, 3) == ADF_OK && adf_sball_arch(w) == ADF_ARCH_NONE);
    adf_sball_set(y, x);
    where = mark;
    ADF_CHECK(adf_sball_exp_at(y, &where, w, inf, 53) == ADF_DOMAIN && adf_place_equal(where, inf) &&
              adf_sball_identical(y, x));

    /* a COMPLEX tag: UNSUPPORTED at the archimedean place (a value that a binding filled by hand) */
    adf_sball_set(w, x);
    w->arch = ADF_ARCH_COMPLEX;
    arb_set_si(acb_imagref(w->inf), 1);
    ADF_CHECK(adf_sball_is_canonical(w));
    for (k = 0; k < NFNS; k++)
    {
        where = mark;
        adf_sball_set(y, x);
        ADF_CHECK(FNS[k].g(y, &where, w, inf, 53) == ADF_UNSUPPORTED && adf_place_equal(where, inf) &&
                  adf_sball_identical(y, x));
    }

    /* the real failure: where = inf, y untouched (log of a negative real ball) */
    make_adele(a, -2, 3, 200);
    places3[1] = inf;
    ADF_CHECK(adf_sball_project(w, NULL, a, places3, 3) == ADF_OK);
    adf_sball_set(y, x);
    where = mark;
    ADF_CHECK(adf_sball_log_at(y, &where, w, inf, 53) == ADF_DOMAIN && adf_place_equal(where, inf) &&
              adf_sball_identical(y, x));
    where = mark;
    ADF_CHECK(adf_sball_sqrt_at(y, &where, w, inf, 53) == ADF_DOMAIN && adf_place_equal(where, inf));
    /* a root of degree 0: DOMAIN, where untouched (no place is at fault) */
    where = mark;
    ADF_CHECK(adf_sball_root_at(y, &where, w, inf, 0, 53) == ADF_DOMAIN && adf_place_equal(where, mark) &&
              adf_sball_identical(y, x));
    /* NOT_DETERMINED: a ball across 0 under log */
    arb_init(r);
    arb_zero(r);
    mag_one(arb_radref(r));
    adf_sball_clear(w);
    adf_sball_init(w);
    sball_real(w, r);
    where = mark;
    ADF_CHECK(adf_sball_log_at(y, &where, w, inf, 53) == ADF_NOT_DETERMINED && adf_place_equal(where, inf) &&
              adf_sball_identical(y, x));

    /* OK, aliased: y = x with three places becomes the partial ball over the one place */
    make_adele(a, 2, 3, 200);
    places3[1] = inf;
    ADF_CHECK(adf_sball_project(x, NULL, a, places3, 3) == ADF_OK);
    where = mark;
    ADF_CHECK(adf_sball_log_at(x, &where, x, inf, 100) == ADF_OK && adf_place_equal(where, mark));
    ADF_CHECK(adf_sball_is_canonical(x) && adf_sball_num_places(x) == 1 && adf_sball_arch(x) == ADF_ARCH_REAL &&
              x->len == 0);
    /* the brief: log(2/3) = -0.405465108108164381978... */
    {
        arb_t l;
        arb_init(l);
        ADF_CHECK(adf_sball_get_arb(l, x, inf) == ADF_OK);
        ADF_CHECK(fabs(arf_get_d(arb_midref(l), ARF_RND_NEAR) + 0.4054651081081644) < 1e-15);
        ADF_CHECK(mag_cmp_2exp_si(arb_radref(l), -95) < 0);
        arb_clear(l);
    }

    arb_clear(r);
    adf_sball_clear(x);
    adf_sball_clear(y);
    adf_sball_clear(w);
    adf_adele_clear(a);
}
