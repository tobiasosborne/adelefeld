/* tests/test_gfunc.c: functions at all places (lane f-slice10, WP 1F.8; include/adelefeld/gfunc.h; docs/api-1f8.md
   G1 to G4).

   Oracles, each with its output precision.
   1. tests/ref/vectors/f-slice10/rat_root.jsonl, made by proto/gfunc_checks.py with exact integers only (two
      independent integer roots, Newton and the bisection of proto/lpow_checks.py:70, and for small values the
      criterion of the valuations, Proposition 16 step 3): the status, the place, and the exact root on the branch
      +1. OUTPUT PRECISION: exact. The branch -1 is derived here from the row (even n: the negative root and the
      same status; odd n >= 3: DOMAIN except for 0; n = 1: the identity).
   2. tests/ref/vectors/f-slice10/real_root.jsonl: rows of rf_vector of proto/functions_checks.py:1622 (mpmath
      intervals at 800 bits, exact integer roots; no arb): the status of the real root of a real ball and [lo, hi]
      that contains its image. OUTPUT PRECISION: the real coordinate of the result must contain [lo, hi] (widened by
      2^-350 of its size) and, on a "tight" row, have a radius at most 4 (hi - lo) + 2^(5 - prec) max|.|; it must
      also be identical (arb_equal) to adf_real_root on the same ball, negated for the branch -1.
   3. Hand values: the named cases of PLAN row 1F.8 (1 has the rational square roots 1 and -1; 8 has the cube
      root 2; 2 has no square root), -4 with n = 2, -8 with n = 3, 0, n = 0, n = 1, degrees above the bit length
      and above WORD_MAX, invalid signs; every status of the three functions with the state of the outputs
      (untouched, `where`
      untouched or the real place); every aliasing combination (y = x).
   What would make a case fail is stated at each test. */

#define _POSIX_C_SOURCE 200809L   /* fork, pipe: the entry check of the debug build at the end of this file */

#include <limits.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/arb.h>
#include <flint/arf.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include <flint/mag.h>
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

#define WMAX ((ulong) WORD_MAX)

static unsigned long cases;

/* ------------------------------------------------------------------------------------------------------ helpers */

static adf_place_t place_of(ulong p)
{
    adf_place_t v;
    ADF_CHECK(adf_place_prime(&v, p) == ADF_OK);
    return v;
}

/* The sentinel of `where`: the prime 97, a place no call below reports. */
static adf_place_t sentinel_place(void) { return place_of(97); }
static int is_sentinel_place(adf_place_t w) { return adf_place_equal(w, sentinel_place()); }
static int is_real_place(adf_place_t w) { return adf_place_is_archimedean(w); }

static int status_from_name(const char *s)
{
    for (int i = 0; i < ADF_STATUS_COUNT; i++)
        if (strcmp(adf_status_str(i), s) == 0)
            return i;
    return -1;
}

static const jsonl_value *member(const jsonl_value *rec, const char *key)
{
    jsonl_error_t err;
    const jsonl_value *v = NULL;
    int ok = jsonl_field(rec, key, &v, &err);
    ADF_CHECK_MSG(ok == 1, "%s", jsonl_error_message(&err));
    return ok ? v : NULL;
}
static int has(const jsonl_value *rec, const char *key)
{
    jsonl_error_t err;
    const jsonl_value *v = NULL;
    return jsonl_field(rec, key, &v, &err);
}
static const char *member_int(const jsonl_value *rec, const char *key)
{
    jsonl_error_t err;
    const jsonl_value *v = member(rec, key);
    const char *t = NULL;
    int ok = v ? jsonl_int_text_or_string(v, &t, &err) : 0;   /* the big integers are JSON strings */
    ADF_CHECK_MSG(ok, "%s: %s", key, ok ? "" : jsonl_error_message(&err));
    return ok ? t : "0";
}
static const char *member_str(const jsonl_value *rec, const char *key)
{
    jsonl_error_t err;
    size_t len;
    const jsonl_value *v = member(rec, key);
    const char *s = v ? jsonl_string(v, &len, &err) : NULL;
    ADF_CHECK(s != NULL);
    return s ? s : "";
}
static jsonl_file *open_vectors(const char *path)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    ADF_CHECK_MSG(jsonl_open(path, &f, &err), "%s", jsonl_error_message(&err));
    return f;
}
/* x = {"m", "e", "rm", "re"}: the ball m 2^e +- rm 2^re (as tests/test_rfunc.c). */
static void rb_from_json(arb_t x, const jsonl_value *o)
{
    fmpz_t m, e;
    fmpz_init(m); fmpz_init(e);
    ADF_CHECK(fmpz_set_str(m, member_int(o, "m"), 10) == 0);
    ADF_CHECK(fmpz_set_str(e, member_int(o, "e"), 10) == 0);
    arb_set_fmpz_2exp(x, m, e);
    mag_set_ui_2exp_si(arb_radref(x), strtoul(member_int(o, "rm"), NULL, 10),
                       strtol(member_int(o, "re"), NULL, 10));
    fmpz_clear(m); fmpz_clear(e);
}
static void dy_from_json(arf_t x, const jsonl_value *o)
{
    fmpz_t m, e;
    fmpz_init(m); fmpz_init(e);
    ADF_CHECK(fmpz_set_str(m, member_int(o, "m"), 10) == 0);
    ADF_CHECK(fmpz_set_str(e, member_int(o, "e"), 10) == 0);
    arf_set_fmpz_2exp(x, m, e);
    fmpz_clear(m); fmpz_clear(e);
}

static void rat_si(adf_rat_t x, slong a, slong b)
{
    fmpq_t q;
    fmpq_init(q);
    fmpq_set_si(q, a, (ulong) b);
    ADF_CHECK(adf_rat_set_fmpq(x, q) == ADF_OK);
    fmpq_clear(q);
}
static void rat_str(adf_rat_t x, const char *num, const char *den)
{
    fmpz_t a, b;
    fmpz_init(a); fmpz_init(b);
    ADF_CHECK(fmpz_set_str(a, num, 10) == 0);
    ADF_CHECK(fmpz_set_str(b, den, 10) == 0);
    ADF_CHECK(adf_rat_set_fmpz2(x, a, b) == ADF_OK);
    fmpz_clear(a); fmpz_clear(b);
}
static ulong ulong_of(const char *s) { return strtoul(s, NULL, 10); }

/* The sentinels of the outputs: the rational 12345/7, the adele (12345 ; 12345/7), the idele of 12345/7 (no call
   below returns any of them). */
static void sent_rat(adf_rat_t y) { rat_si(y, 12345, 7); }
static int is_sent_rat(const adf_rat_t y)
{
    adf_rat_t t;
    int r;
    adf_rat_init(t); sent_rat(t);
    r = adf_rat_identical(y, t);
    adf_rat_clear(t);
    return r;
}
static void sent_adele(adf_adele_t y)
{
    adf_rat_t t;
    adf_rat_init(t); sent_rat(t);
    adf_adele_set_rat(y, t, 64);
    arb_set_si(y->inf, 12345);
    adf_rat_clear(t);
}
static int is_sent_adele(const adf_adele_t y)
{
    adf_adele_t t;
    int r;
    adf_adele_init(t); sent_adele(t);
    r = adf_adele_identical(y, t);
    adf_adele_clear(t);
    return r;
}
static void sent_idele(adf_idele_t y)
{
    adf_rat_t t;
    adf_rat_init(t); sent_rat(t);
    ADF_CHECK(adf_idele_set_rat(y, t, 64) == ADF_OK);
    adf_rat_clear(t);
}
static int is_sent_idele(const adf_idele_t y)
{
    adf_idele_t t;
    int r;
    adf_idele_init(t); sent_idele(t);
    r = adf_idele_identical(y, t);
    adf_idele_clear(t);
    return r;
}

/* The combined status of conventions 3.3 over the real coordinate (first) and the finite part, and the place:
   the real place if the real status is the maximum, else untouched (the finite part names no prime). */
static int combine(int st_real, int st_fin, int *real_named)
{
    int st = st_real > st_fin ? st_real : st_fin;
    *real_named = st != ADF_OK && st_real == st;
    return st;
}

/* The real coordinate of a result against a row of real_root.jsonl (or of the five series, slice B): contains
   [lo, hi] widened by 2^-350 max(|lo|, |hi|, 2^-50); on a tight row 2 rad <= 4 (hi - lo) + 2^(5 - p) max + 8 slack
   (the bound of tests/test_rfunc.c, check_enclosure). neg: the image is -[lo, hi]. */
static void check_real_image(const arb_t R, const jsonl_value *rec, slong prec, int neg, const char *what)
{
    arb_t Ri;
    arf_t lo, hi, wid, rad, lhs, rhs, t, mx, slack;
    slong p = prec < 2 ? 2 : prec;
    const slong W = 3000;
    unsigned long line = jsonl_line_of(rec);
    arb_init(Ri);
    arf_init(lo); arf_init(hi); arf_init(wid); arf_init(rad); arf_init(lhs); arf_init(rhs); arf_init(t);
    arf_init(mx); arf_init(slack);
    dy_from_json(lo, member(rec, "lo"));
    dy_from_json(hi, member(rec, "hi"));
    if (neg) { arf_neg(lo, lo); arf_neg(hi, hi); arf_swap(lo, hi); }
    arf_sub(wid, hi, lo, W, ARF_RND_UP);
    arf_abs(t, lo); arf_abs(mx, hi); arf_max(mx, mx, t);
    arf_set_si_2exp_si(t, 1, -50);
    arf_max(slack, mx, t);
    arf_mul_2exp_si(slack, slack, -350);
    ADF_CHECK_MSG(arb_is_finite(R), "%s line %lu: not finite", what, line);
    arb_set(Ri, R);
    arb_add_error_arf(Ri, slack);
    ADF_CHECK_MSG(arb_contains_arf(Ri, lo) && arb_contains_arf(Ri, hi), "%s line %lu: the image is not enclosed",
                  what, line);
    /* The bound of tests/test_rfunc.c is asserted from 24 bits on. Below, arb's root of 2^1000 +- 2^960 at prec 2
       has a relative radius near 2 (lanes/f-slice10/result.md, finding 3): rfunc.h promises no radius, and the
       identity with adf_real_root's ball is checked separately (check_same_real_root). */
    if (p >= 24 && has(rec, "tight") && strtol(member_int(rec, "tight"), NULL, 10) == 1)
    {
        arf_set_mag(rad, arb_radref(R));
        arf_mul_2exp_si(lhs, rad, 1);
        arf_mul_2exp_si(t, mx, 5 - p);
        arf_mul_2exp_si(rhs, wid, 2);
        arf_add(rhs, rhs, t, W, ARF_RND_UP);
        arf_mul_2exp_si(t, slack, 3);
        arf_add(rhs, rhs, t, W, ARF_RND_UP);
        ADF_CHECK_MSG(arf_cmp(lhs, rhs) <= 0, "%s line %lu: radius too large", what, line);
    }
    arb_clear(Ri);
    arf_clear(lo); arf_clear(hi); arf_clear(wid); arf_clear(rad); arf_clear(lhs); arf_clear(rhs); arf_clear(t);
    arf_clear(mx); arf_clear(slack);
}

/* ============================================================================================ slice A: the root */

/* One call of adf_rat_root against the expectation (st, real_named, root); y and where start as sentinels; then the
   same call with y = a (aliasing). Fails on: another status; `where` written when it must stay, or not the real
   place when it must be; y written on a status; another root; an aliased call that differs. */
static void expect_rat(const adf_rat_t a, ulong n, int sign, int st, int real_named, const adf_rat_t root,
                       const char *what, unsigned long line)
{
    adf_rat_t y, z;
    adf_place_t w = sentinel_place(), w2 = sentinel_place();
    int got, got2;
    adf_rat_init(y); adf_rat_init(z);
    sent_rat(y);
    got = adf_rat_root(y, &w, a, n, sign);
    ADF_CHECK_MSG(got == st, "%s line %lu n=%lu sign=%d: status %d, want %d", what, line, n, sign, got, st);
    if (st == ADF_OK)
    {
        ADF_CHECK_MSG(got != ADF_OK || adf_rat_equal(y, root), "%s line %lu n=%lu sign=%d: wrong root", what, line,
                      n, sign);
        ADF_CHECK_MSG(is_sentinel_place(w), "%s line %lu: where written on OK", what, line);
    }
    else
    {
        ADF_CHECK_MSG(is_sent_rat(y), "%s line %lu n=%lu sign=%d: y written on a status", what, line, n, sign);
        ADF_CHECK_MSG(real_named ? is_real_place(w) : is_sentinel_place(w), "%s line %lu n=%lu sign=%d: where",
                      what, line, n, sign);
    }
    adf_rat_set(z, a);
    got2 = adf_rat_root(z, &w2, z, n, sign);
    ADF_CHECK_MSG(got2 == got && (got == ADF_OK ? adf_rat_equal(z, y) : adf_rat_equal(z, a)),
                  "%s line %lu: aliasing y = a differs", what, line);
    ADF_CHECK(adf_rat_root(y, NULL, a, n, sign) == st);     /* where may be NULL */
    adf_rat_clear(y); adf_rat_clear(z);
    cases += 3;
}

/* The expectation of the branch sign from the row of the branch +1 (gfunc.h, "THE BRANCH"). */
static void derive_sign(int *st, int *real_named, adf_rat_t root, const adf_rat_t a, ulong n, int sign)
{
    if (sign == 1 || n == 1)
        return;
    if (adf_rat_is_zero(a)) { *st = ADF_OK; *real_named = 0; adf_rat_zero(root); return; }
    if (n % 2 == 1) { *st = ADF_DOMAIN; *real_named = 0; return; }
    if (*st == ADF_OK)
        adf_rat_neg(root, root);
}

/* Every row of rat_root.jsonl with both signs. A case fails on any difference from the row (exact root, status,
   place), on y written on a status, on aliasing; and, on OK with n <= 64, on root^n != a (FLINT's fmpq_pow_si). */
ADF_TEST(rat_root_vectors)
{
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice10/rat_root.jsonl");
    unsigned long before = cases, ok_rows = 0;
    if (!f) return;
    for (size_t i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        adf_rat_t a, root;
        ulong n = ulong_of(member_int(rec, "n"));
        adf_rat_init(a); adf_rat_init(root);
        rat_str(a, member_int(rec, "num"), member_int(rec, "den"));
        for (int sign = 1; sign >= -1; sign -= 2)
        {
            int st = status_from_name(member_str(rec, "status"));
            int real_named = strcmp(member_str(rec, "where"), "real") == 0;
            if (st == ADF_OK) rat_str(root, member_int(rec, "rnum"), member_int(rec, "rden"));
            derive_sign(&st, &real_named, root, a, n, sign);
            expect_rat(a, n, sign, st, real_named, root, "rat_root.jsonl", jsonl_line_of(rec));
            if (st == ADF_OK && n <= 64 && sign == 1)
            {
                fmpq_t t;
                fmpq_init(t);
                fmpq_pow_si(t, root->q, (slong) n);
                ADF_CHECK(fmpq_equal(t, a->q));
                fmpq_clear(t);
                ok_rows++;
            }
        }
        adf_rat_clear(a); adf_rat_clear(root);
    }
    printf("rat_root_vectors: %zu rows, %lu calls, %lu checked root^n = a\n", jsonl_count(f), cases - before,
           ok_rows);
    jsonl_close(f);
}

/* The named cases of PLAN row 1F.8 and the edges of the degree and the selector. A case fails on the value or the
   status in the comment. */
ADF_TEST(rat_root_named_cases)
{
    adf_rat_t a, r;
    adf_rat_init(a); adf_rat_init(r);
    /* 1 has the rational square roots 1 and -1 */
    rat_si(a, 1, 1); rat_si(r, 1, 1); expect_rat(a, 2, 1, ADF_OK, 0, r, "1^(1/2)", 0);
    rat_si(r, -1, 1); expect_rat(a, 2, -1, ADF_OK, 0, r, "1^(1/2) branch -1", 0);
    /* 8 has the cube root 2; -8 the cube root -2; the branch -1 names nothing for n = 3 */
    rat_si(a, 8, 1); rat_si(r, 2, 1); expect_rat(a, 3, 1, ADF_OK, 0, r, "8^(1/3)", 0);
    expect_rat(a, 3, -1, ADF_DOMAIN, 0, r, "8^(1/3) branch -1", 0);
    rat_si(a, -8, 1); rat_si(r, -2, 1); expect_rat(a, 3, 1, ADF_OK, 0, r, "-8^(1/3)", 0);
    expect_rat(a, 3, -1, ADF_DOMAIN, 0, r, "-8^(1/3) branch -1", 0);
    /* 2 has no square root: DOMAIN, where untouched (the first failing prime, 2, is not named) */
    rat_si(a, 2, 1); expect_rat(a, 2, 1, ADF_DOMAIN, 0, r, "2^(1/2)", 0);
    expect_rat(a, 2, -1, ADF_DOMAIN, 0, r, "2^(1/2) branch -1", 0);
    /* -4: no real square root, the real place is named */
    rat_si(a, -4, 1); expect_rat(a, 2, 1, ADF_DOMAIN, 1, r, "-4^(1/2)", 0);
    expect_rat(a, 2, -1, ADF_DOMAIN, 1, r, "-4^(1/2) branch -1", 0);
    /* 0: the root 0 for every n >= 1 and either sign */
    rat_si(a, 0, 1); rat_si(r, 0, 1);
    for (ulong n = 1; n <= 7; n++)
        for (int s = 1; s >= -1; s -= 2)
            expect_rat(a, n, s, ADF_OK, 0, r, "0", 0);
    expect_rat(a, UWORD_MAX, -1, ADF_OK, 0, r, "0, n = UWORD_MAX", 0);
    /* n = 0: DOMAIN, where untouched, for every a and sign (also 0 and an invalid sign) */
    expect_rat(a, 0, 1, ADF_DOMAIN, 0, r, "0^(1/0)", 0);
    rat_si(a, -4, 1); expect_rat(a, 0, 5, ADF_DOMAIN, 0, r, "-4^(1/0)", 0);
    rat_si(a, 4, 1); expect_rat(a, 0, 1, ADF_DOMAIN, 0, r, "4^(1/0)", 0);
    /* n = 1: the identity, the sign ignored */
    rat_si(a, -5, 3);
    expect_rat(a, 1, 1, ADF_OK, 0, a, "n = 1", 0);
    expect_rat(a, 1, -1, ADF_OK, 0, a, "n = 1, sign -1", 0);
    expect_rat(a, 1, 0, ADF_OK, 0, a, "n = 1, sign 0", 0);
    expect_rat(a, 1, INT_MIN, ADF_OK, 0, a, "n = 1, sign INT_MIN", 0);
    /* invalid selectors for n >= 2: DOMAIN, where untouched, before the real place */
    rat_si(a, 4, 1);
    expect_rat(a, 2, 0, ADF_DOMAIN, 0, r, "sign 0", 0);
    expect_rat(a, 2, 2, ADF_DOMAIN, 0, r, "sign 2", 0);
    expect_rat(a, 3, -2, ADF_DOMAIN, 0, r, "sign -2", 0);
    expect_rat(a, 2, INT_MAX, ADF_DOMAIN, 0, r, "sign INT_MAX", 0);
    rat_si(a, -4, 1); expect_rat(a, 2, 0, ADF_DOMAIN, 0, r, "-4, sign 0", 0);
    rat_si(a, 0, 1); expect_rat(a, 2, 3, ADF_DOMAIN, 0, r, "0, sign 3", 0);
    /* degrees above the bit length and above WORD_MAX: 1 and -1 have roots, others none */
    rat_si(a, 1, 1); rat_si(r, 1, 1);
    expect_rat(a, WMAX, 1, ADF_OK, 0, r, "1, n = WORD_MAX", 0);
    expect_rat(a, WMAX + 1, 1, ADF_OK, 0, r, "1, n = 2^63", 0);
    rat_si(r, -1, 1);
    expect_rat(a, WMAX + 1, -1, ADF_OK, 0, r, "1, n = 2^63, sign -1", 0);
    rat_si(a, -1, 1);
    expect_rat(a, UWORD_MAX, 1, ADF_OK, 0, r, "-1, n = UWORD_MAX", 0);
    expect_rat(a, WMAX + 1, 1, ADF_DOMAIN, 1, r, "-1, n = 2^63 (even)", 0);
    rat_si(a, 1, 2); expect_rat(a, WMAX, 1, ADF_DOMAIN, 0, r, "1/2, n = WORD_MAX", 0);
    rat_si(a, 3, 1); expect_rat(a, UWORD_MAX, 1, ADF_DOMAIN, 0, r, "3, n = UWORD_MAX", 0);
    {
        /* 2^63 has 64 bits: n = 63 gives 2, n = 64 = bits gives DOMAIN; 2^64 (65 bits) with n = 64 gives 2;
           1/2^63 with n = 63 gives 1/2 (the denominator tested) */
        fmpz_t t;
        fmpz_init(t);
        fmpz_one(t); fmpz_mul_2exp(t, t, 63); fmpq_set_fmpz_frac(a->q, t, fmpq_denref(r->q));
        fmpz_one(fmpq_denref(a->q));
        rat_si(r, 2, 1); expect_rat(a, 63, 1, ADF_OK, 0, r, "2^63, n = 63", 0);
        expect_rat(a, 64, 1, ADF_DOMAIN, 0, r, "2^63, n = 64", 0);
        fmpz_mul_2exp(fmpq_numref(a->q), fmpq_numref(a->q), 1);
        expect_rat(a, 64, 1, ADF_OK, 0, r, "2^64, n = 64", 0);
        fmpz_one(fmpq_numref(a->q)); fmpz_one(t); fmpz_mul_2exp(fmpq_denref(a->q), t, 63);
        rat_si(r, 1, 2); expect_rat(a, 63, 1, ADF_OK, 0, r, "1/2^63, n = 63", 0);
        rat_si(r, -1, 2); expect_rat(a, 62, -1, ADF_DOMAIN, 0, r, "1/2^63, n = 62", 0);
        fmpz_clear(t);
    }
    adf_rat_clear(a); adf_rat_clear(r);
}

/* ---- the adele ---- */

/* One call of adf_adele_root(y, where, x, n, sign, prec) and the aliased call; returns the status and leaves the
   result in y. Fails on y written on a status (y starts as the sentinel), on `where` against `real_named`, on
   where written on OK, on an aliased call that differs. */
static int run_adele(adf_adele_t y, const adf_adele_t x, ulong n, int sign, slong prec, int st, int real_named,
                     const char *what, unsigned long line)
{
    adf_adele_t z;
    adf_place_t w = sentinel_place(), w2 = sentinel_place();
    int got, got2;
    adf_adele_init(z);
    sent_adele(y);
    got = adf_adele_root(y, &w, x, n, sign, prec);
    ADF_CHECK_MSG(got == st, "%s line %lu n=%lu sign=%d prec=%ld: status %d, want %d", what, line, n, sign,
                  (long) prec, got, st);
    if (got != ADF_OK)
        ADF_CHECK_MSG(is_sent_adele(y), "%s line %lu: y written on a status", what, line);
    if (st != ADF_OK)
        ADF_CHECK_MSG(real_named ? is_real_place(w) : is_sentinel_place(w), "%s line %lu n=%lu sign=%d: where",
                      what, line, n, sign);
    else
        ADF_CHECK_MSG(is_sentinel_place(w), "%s line %lu: where written on OK", what, line);
    adf_adele_set(z, x);
    got2 = adf_adele_root(z, &w2, z, n, sign, prec);
    ADF_CHECK_MSG(got2 == got && (got == ADF_OK ? adf_adele_identical(z, y) : adf_adele_identical(z, x)),
                  "%s line %lu: aliasing y = x differs", what, line);
    adf_adele_clear(z);
    cases += 2;
    return got;
}

/* The real coordinate must be adf_real_root's ball (negated for even n and the branch -1). */
static void check_same_real_root(const arb_t J, const arb_t I, ulong n, int sign, slong prec, const char *what)
{
    arb_t t;
    arb_init(t);
    ADF_CHECK(adf_real_root(t, I, n, prec) == ADF_OK);
    if (n % 2 == 0 && sign == -1) arb_neg(t, t);
    ADF_CHECK_MSG(arb_equal(J, t), "%s: the real coordinate is not adf_real_root's ball", what);
    arb_clear(t);
}

/* Every row of real_root.jsonl as the real coordinate I of an adele (I ; q), q in {1, 0, -1, 2, 32}, both signs.
   The expectation: the real status of the row (n = 1: the identity, OK); the finite status of q by hand (1 and 0:
   roots for all n; -1: odd n only; 2: n = 1 only; 32: n = 1 and 5); for n >= 3 odd and sign -1: DOMAIN untouched
   (the exact zero adele does not occur: no row is the exact 0 with q = 0 except the ball 0, see below); the
   combination of conventions 3.3. A case fails on: the status or the place; y written on a status; on OK, the real
   coordinate not containing the image [lo, hi] (negated for the branch -1) or not tight on a tight row, or not
   identical to adf_real_root's ball; the finite coordinate not the exact rational root; for n = 1, y not an
   identical copy of x; aliasing. */
ADF_TEST(adele_root_real_vectors)
{
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice10/real_root.jsonl");
    const slong qs[5] = {1, 0, -1, 2, 32};
    unsigned long before = cases, oks = 0;
    if (!f) return;
    for (size_t i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        ulong n = ulong_of(member_int(rec, "n"));
        slong prec = strtol(member_int(rec, "prec"), NULL, 10);
        int st_row = status_from_name(member_str(rec, "status"));
        adf_adele_t x, y;
        adf_rat_t q, root;
        adf_adele_init(x); adf_adele_init(y); adf_rat_init(q); adf_rat_init(root);
        for (int k = 0; k < 5; k++)
            for (int sign = 1; sign >= -1; sign -= 2)
            {
                int st_fin, st_real = st_row, st, named, zero_adele;
                rat_si(q, qs[k], 1);
                adf_adele_set_rat(x, q, 64);
                rb_from_json(x->inf, member(rec, "x"));
                zero_adele = arb_is_zero(x->inf) && qs[k] == 0;
                /* the finite status and root on the branch +1 */
                if (n == 1 || qs[k] == 0 || qs[k] == 1 || (qs[k] == -1 && n % 2 == 1) || (qs[k] == 32 && n == 5))
                    st_fin = ADF_OK;
                else
                    st_fin = ADF_DOMAIN;
                if (qs[k] == 32 && n == 5) rat_si(root, 2, 1);
                else if (n == 1) adf_rat_set(root, q);
                else adf_rat_set(root, q);   /* 1, 0, -1 are their own roots */
                if (n % 2 == 0 && sign == -1) adf_rat_neg(root, root);
                if (n == 1)
                    st = ADF_OK, named = 0;
                else if (n % 2 == 1 && sign == -1 && !zero_adele)
                    st = ADF_DOMAIN, named = 0;
                else
                    st = combine(st_real, st_fin, &named);
                if (run_adele(y, x, n, sign, prec, st, named, "real_root.jsonl", jsonl_line_of(rec)) != ADF_OK)
                    continue;
                if (n == 1)
                {
                    ADF_CHECK_MSG(adf_adele_identical(y, x), "line %lu: n = 1 is not the identity",
                                  jsonl_line_of(rec));
                    continue;
                }
                if (zero_adele && n % 2 == 1 && sign == -1)
                {
                    ADF_CHECK(arb_is_zero(y->inf) && adf_fball_is_exact(&y->fin));
                    continue;
                }
                check_real_image(y->inf, rec, prec, n % 2 == 0 && sign == -1, "real_root.jsonl");
                check_same_real_root(y->inf, x->inf, n, sign, prec, "real_root.jsonl");
                ADF_CHECK(adf_fball_is_exact(&y->fin) && adf_fball_contains_rat(&y->fin, root));
                oks++;
            }
        adf_adele_clear(x); adf_adele_clear(y); adf_rat_clear(q); adf_rat_clear(root);
    }
    printf("adele_root_real_vectors: %zu rows, %lu calls, %lu OK results checked\n", jsonl_count(f), cases - before,
           oks);
    jsonl_close(f);
}

/* Every row of rat_root.jsonl as the adele of the rational, (q ; q) at prec 64, and as (|q| + 1 ; q) (a positive
   real ball), both signs. The adele of a rational must agree with adf_rat_root: the same status and place (the
   real ball of q != 0 at 64 bits has the sign of q); on OK the finite part is the exact root and the real ball
   contains it. With the real ball |q| + 1 > 0 only the finite part can fail, so the place is never named. A case
   fails on any of these. */
ADF_TEST(adele_root_rat_vectors)
{
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice10/rat_root.jsonl");
    unsigned long before = cases;
    if (!f) return;
    for (size_t i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        ulong n = ulong_of(member_int(rec, "n"));
        adf_adele_t x, y;
        adf_rat_t a, root;
        arb_t c;
        adf_adele_init(x); adf_adele_init(y); adf_rat_init(a); adf_rat_init(root); arb_init(c);
        rat_str(a, member_int(rec, "num"), member_int(rec, "den"));
        for (int form = 0; form < 2; form++)
            for (int sign = 1; sign >= -1; sign -= 2)
            {
                int st = status_from_name(member_str(rec, "status"));
                int named = strcmp(member_str(rec, "where"), "real") == 0;
                if (st == ADF_OK) rat_str(root, member_int(rec, "rnum"), member_int(rec, "rden"));
                derive_sign(&st, &named, root, a, n, sign);
                adf_adele_set_rat(x, a, 64);
                if (form == 1)
                {
                    arb_abs(x->inf, x->inf);
                    arb_add_si(x->inf, x->inf, 1, 64);
                    if (named) { named = 0; st = (n % 2 == 0 && fmpq_sgn(a->q) < 0) ? ADF_DOMAIN : st; }
                }
                if (adf_rat_is_zero(a) && n % 2 == 1 && sign == -1 && n != 1 && form == 1)
                    st = ADF_DOMAIN, named = 0;   /* (1 ; 0) is not the exact zero adele */
                if (run_adele(y, x, n, sign, 64, st, named, "rat_root.jsonl as adele",
                              jsonl_line_of(rec)) != ADF_OK)
                    continue;
                ADF_CHECK(adf_fball_is_exact(&y->fin) && adf_fball_contains_rat(&y->fin, n == 1 ? a : root));
                if (form == 0)
                {
                    arb_set_fmpq(c, n == 1 ? a->q : root->q, 256);
                    ADF_CHECK_MSG(arb_overlaps(y->inf, c), "line %lu: the real ball misses the root",
                                  jsonl_line_of(rec));
                    if (fmpz_is_one(fmpq_denref(root->q)) && fmpz_bits(fmpq_numref(root->q)) < 60 && n > 1)
                        ADF_CHECK(arb_contains_fmpz(y->inf, fmpq_numref(root->q)));
                }
            }
        adf_adele_clear(x); adf_adele_clear(y); adf_rat_clear(a); adf_rat_clear(root); arb_clear(c);
    }
    printf("adele_root_rat_vectors: %zu rows, %lu calls\n", jsonl_count(f), cases - before);
    jsonl_close(f);
}

/* x = (I ; F) with I = [m +- r] (m, r integers) and F = (A + H Zhat)/d. */
static void adele_mk(adf_adele_t x, slong m, slong r, slong A, slong H, slong d)
{
    fmpz_t a, h, dd;
    fmpz_init_set_si(a, A); fmpz_init_set_si(h, H); fmpz_init_set_si(dd, d);
    ADF_CHECK(adf_fball_set_fmpz3(&x->fin, a, h, dd) == ADF_OK);
    arb_set_si(x->inf, m);
    mag_set_ui(arb_radref(x->inf), (ulong) r);
    fmpz_clear(a); fmpz_clear(h); fmpz_clear(dd);
}

/* Every status of adf_adele_root with the state of the outputs (gfunc.h). A case fails on the status, the place or
   the value in its comment. */
ADF_TEST(adele_root_statuses)
{
    adf_adele_t x, y;
    adf_modctx_struct *ctx = NULL;
    ulong q0 = 12;
    adf_adele_init(x); adf_adele_init(y);
    /* LIMIT first, where = real, before n = 0 and an invalid sign */
    adele_mk(x, 4, 0, 4, 0, 1);
    run_adele(y, x, 2, 1, ADF_REAL_PREC_MAX + 1, ADF_LIMIT, 1, "LIMIT", 0);
    run_adele(y, x, 0, 7, WORD_MAX, ADF_LIMIT, 1, "LIMIT before n = 0", 0);
    run_adele(y, x, 2, 1, ADF_REAL_PREC_MAX, ADF_OK, 0, "prec at the limit", 0);
    ADF_CHECK(arb_is_exact(y->inf) && arf_equal_si(arb_midref(y->inf), 2));
    /* a prec below 2 is 2 */
    run_adele(y, x, 2, 1, -5, ADF_OK, 0, "prec -5", 0);
    run_adele(y, x, 2, 1, 0, ADF_OK, 0, "prec 0", 0);
    /* n = 0: DOMAIN untouched; an invalid sign: DOMAIN untouched */
    run_adele(y, x, 0, 1, 64, ADF_DOMAIN, 0, "n = 0", 0);
    run_adele(y, x, 2, 0, 64, ADF_DOMAIN, 0, "sign 0", 0);
    run_adele(y, x, 2, -3, 64, ADF_DOMAIN, 0, "sign -3", 0);
    run_adele(y, x, 3, -1, 64, ADF_DOMAIN, 0, "odd n, sign -1", 0);
    /* the exact zero adele: the root (0 ; 0) for either sign, also odd n */
    adele_mk(x, 0, 0, 0, 0, 1);
    run_adele(y, x, 3, -1, 64, ADF_OK, 0, "(0 ; 0), n = 3, sign -1", 0);
    ADF_CHECK(arb_is_zero(y->inf) && adf_fball_is_exact(&y->fin) && fmpz_is_zero(y->fin.A));
    run_adele(y, x, 2, -1, 64, ADF_OK, 0, "(0 ; 0), n = 2, sign -1", 0);
    ADF_CHECK(arb_is_zero(y->inf) && fmpz_is_zero(y->fin.A));
    /* a real ball around 0 is not the exact zero: odd n with sign -1 is DOMAIN */
    adele_mk(x, 0, 1, 0, 0, 1);
    run_adele(y, x, 3, -1, 64, ADF_DOMAIN, 0, "([0 +- 1] ; 0), n = 3, sign -1", 0);
    adele_mk(x, 0, 0, 0, 4, 1);
    run_adele(y, x, 3, -1, 64, ADF_DOMAIN, 0, "(0 ; 0 + 4 Zhat), n = 3, sign -1", 0);
    /* n = 1: the identity for every input, an inexact finite part included */
    adele_mk(x, -3, 1, 2, 4, 1);
    run_adele(y, x, 1, 1, 64, ADF_OK, 0, "n = 1", 0);
    ADF_CHECK(adf_adele_identical(y, x));
    run_adele(y, x, 1, 0, 2, ADF_OK, 0, "n = 1, sign 0", 0);
    ADF_CHECK(adf_adele_identical(y, x));
    /* the examples of the header */
    adele_mk(x, -4, 0, 4, 0, 1); run_adele(y, x, 2, 1, 64, ADF_DOMAIN, 1, "(-4 ; 4)", 0);
    adele_mk(x, 4, 0, 2, 0, 1); run_adele(y, x, 2, 1, 64, ADF_DOMAIN, 0, "(4 ; 2)", 0);
    adele_mk(x, -4, 0, 2, 0, 1); run_adele(y, x, 2, 1, 64, ADF_DOMAIN, 1, "(-4 ; 2)", 0);
    adele_mk(x, 0, 1, 2, 0, 1); run_adele(y, x, 2, 1, 64, ADF_DOMAIN, 0, "([-1, 1] ; 2)", 0);
    adele_mk(x, 0, 1, 4, 0, 1); run_adele(y, x, 2, 1, 64, ADF_NOT_DETERMINED, 1, "([-1, 1] ; 4)", 0);
    /* a finite part of positive radius: NOT_DETERMINED, where untouched (2 + 4 Zhat: no prime examined) */
    adele_mk(x, 4, 0, 2, 4, 1); run_adele(y, x, 2, 1, 64, ADF_NOT_DETERMINED, 0, "(4 ; 2 + 4 Zhat)", 0);
    adele_mk(x, 8, 0, 0, 4, 1); run_adele(y, x, 3, 1, 64, ADF_NOT_DETERMINED, 0, "(8 ; 0 + 4 Zhat)", 0);
    adele_mk(x, -4, 0, 2, 4, 1); run_adele(y, x, 2, 1, 64, ADF_DOMAIN, 1, "(-4 ; 2 + 4 Zhat)", 0);
    adele_mk(x, 0, 1, 2, 4, 1); run_adele(y, x, 2, 1, 64, ADF_NOT_DETERMINED, 1, "([-1, 1] ; 2 + 4 Zhat)", 0);
    /* a local-backend finite part (never exact, fball.h:154): NOT_DETERMINED, where untouched */
    ADF_CHECK(adf_modctx_new_blocks(&ctx, &q0, 1) == ADF_OK);
    adele_mk(x, 4, 0, 4, 12, 1);
    ADF_CHECK(adf_fball_set_local(&x->fin, &x->fin, ctx) == ADF_OK);
    ADF_CHECK(x->fin.backend == ADF_LOCAL);
    run_adele(y, x, 2, 1, 64, ADF_NOT_DETERMINED, 0, "(4 ; local 4 + 12 Zhat)", 0);
    run_adele(y, x, 1, 1, 64, ADF_OK, 0, "(4 ; local), n = 1", 0);
    ADF_CHECK(adf_adele_identical(y, x));
    adf_adele_clear(y);
    adf_adele_init(y);
    adf_adele_clear(x);
    adf_adele_init(x);
    adf_modctx_free(ctx);
    /* (-8 ; 8), n = 3: the real root -2 and the finite root 2: each coordinate its own real root */
    adele_mk(x, -8, 0, 8, 0, 1); run_adele(y, x, 3, 1, 64, ADF_OK, 0, "(-8 ; 8)", 0);
    ADF_CHECK(arb_contains_si(y->inf, -2) && arb_is_negative(y->inf) && fmpz_equal_si(y->fin.A, 2));
    /* (9 ; 9/4), n = 2, branch -1: (-3 ; -3/2) */
    adele_mk(x, 9, 0, 9, 0, 4); run_adele(y, x, 2, -1, 64, ADF_OK, 0, "(9 ; 9/4) branch -1", 0);
    ADF_CHECK(arf_equal_si(arb_midref(y->inf), -3) && fmpz_equal_si(y->fin.A, -3) && fmpz_equal_si(y->fin.d, 2));
    /* where = NULL is accepted */
    adele_mk(x, -4, 0, 4, 0, 1);
    ADF_CHECK(adf_adele_root(y, NULL, x, 2, 1, 64) == ADF_DOMAIN);
    adf_adele_clear(x); adf_adele_clear(y);
}

/* ---- the idele ---- */

static int run_idele(adf_idele_t y, const adf_idele_t x, ulong n, int sign, slong prec, int st, int real_named,
                     const char *what, unsigned long line)
{
    adf_idele_t z;
    adf_place_t w = sentinel_place(), w2 = sentinel_place();
    int got, got2;
    adf_idele_init(z);
    sent_idele(y);
    got = adf_idele_root(y, &w, x, n, sign, prec);
    ADF_CHECK_MSG(got == st, "%s line %lu n=%lu sign=%d: status %d, want %d", what, line, n, sign, got, st);
    if (got != ADF_OK)
        ADF_CHECK_MSG(is_sent_idele(y), "%s line %lu: y written on a status", what, line);
    else
        ADF_CHECK_MSG(adf_idele_is_canonical(y), "%s line %lu: the result is not a canonical idele", what, line);
    if (st != ADF_OK)
        ADF_CHECK_MSG(real_named ? is_real_place(w) : is_sentinel_place(w), "%s line %lu n=%lu: where", what, line,
                      n);
    else
        ADF_CHECK_MSG(is_sentinel_place(w), "%s line %lu: where written on OK", what, line);
    adf_idele_set(z, x);
    got2 = adf_idele_root(z, &w2, z, n, sign, prec);
    ADF_CHECK_MSG(got2 == got && (got == ADF_OK ? adf_idele_identical(z, y) : adf_idele_identical(z, x)),
                  "%s line %lu: aliasing y = x differs", what, line);
    adf_idele_clear(z);
    cases += 2;
    return got;
}

/* The idele of every non-zero rational of rat_root.jsonl (exact unit [sign q], content |q|), n from the row and 0,
   both signs. The expectation is the row (the idele of q follows the rational contract, G4): the same status and
   place; on OK the content is |root|, the unit the exact [sign(root)], the real ball identical to adf_real_root's
   (negated for the branch -1) and containing the root, excluding 0. A case fails
   on any of these or on aliasing. */
ADF_TEST(idele_root_rat_vectors)
{
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice10/rat_root.jsonl");
    unsigned long before = cases;
    if (!f) return;
    for (size_t i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        ulong n = ulong_of(member_int(rec, "n"));
        adf_idele_t x, y;
        adf_rat_t a, root;
        fmpq_t c;
        arb_t t;
        adf_rat_init(a);
        rat_str(a, member_int(rec, "num"), member_int(rec, "den"));
        if (adf_rat_is_zero(a)) { adf_rat_clear(a); continue; }
        adf_idele_init(x); adf_idele_init(y); adf_rat_init(root); fmpq_init(c); arb_init(t);
        ADF_CHECK(adf_idele_set_rat(x, a, 64) == ADF_OK);
        for (int sign = 1; sign >= -1; sign -= 2)
        {
            int st = status_from_name(member_str(rec, "status"));
            int named = strcmp(member_str(rec, "where"), "real") == 0;
            if (st == ADF_OK) rat_str(root, member_int(rec, "rnum"), member_int(rec, "rden"));
            derive_sign(&st, &named, root, a, n, sign);
            if (run_idele(y, x, n, sign, 64, st, named, "rat_root.jsonl as idele", jsonl_line_of(rec)) != ADF_OK)
                continue;
            if (n == 1) { ADF_CHECK(adf_idele_identical(y, x)); continue; }
            fmpq_abs(c, root->q);
            ADF_CHECK(fmpq_equal(c, y->r));
            ADF_CHECK(adf_ucoset_is_exact(&y->u) && fmpz_equal_si(y->u.c, fmpq_sgn(root->q)));
            check_same_real_root(y->inf, x->inf, n, sign, 64, "idele");
            arb_set_fmpq(t, root->q, 256);
            ADF_CHECK(arb_overlaps(y->inf, t) && arb_is_nonzero(y->inf));
        }
        adf_idele_clear(x); adf_idele_clear(y); adf_rat_clear(a); adf_rat_clear(root); fmpq_clear(c); arb_clear(t);
    }
    printf("idele_root_rat_vectors: %zu rows, %lu calls\n", jsonl_count(f), cases - before);
    jsonl_close(f);
}

/* x = (I, r, (c, N)) with I = [m +- rad] (integers), r = rn/rd, the unit coset c U(N) (N = 0: the exact [c]). */
static void idele_mk(adf_idele_t x, slong m, slong rad, slong rn, slong rd, slong c, slong N)
{
    arb_t I;
    fmpq_t r;
    adf_ucoset_t u;
    fmpz_t cc, NN;
    arb_init(I); fmpq_init(r); adf_ucoset_init(u); fmpz_init_set_si(cc, c); fmpz_init_set_si(NN, N);
    arb_set_si(I, m); mag_set_ui(arb_radref(I), (ulong) rad);
    fmpq_set_si(r, rn, (ulong) rd);
    ADF_CHECK(adf_ucoset_set_fmpz2(u, cc, NN) == ADF_OK);
    ADF_CHECK(adf_idele_set_parts(x, I, r, u) == ADF_OK);
    arb_clear(I); fmpq_clear(r); adf_ucoset_clear(u); fmpz_clear(cc); fmpz_clear(NN);
}

/* x = ([m] ; 1 ; [1 mod (2^bits + 1)]): a canonical unit coset of a modulus of `bits` bits, inexact. */
static void
idele_mk_mod_bits(adf_idele_t x, slong m, ulong bits)
{
    arb_t I;
    fmpq_t r;
    adf_ucoset_t u;
    fmpz_t c, N;
    arb_init(I); fmpq_init(r); adf_ucoset_init(u); fmpz_init(c); fmpz_init(N);
    arb_set_si(I, m); fmpq_one(r); fmpz_one(c);
    fmpz_one(N); fmpz_mul_2exp(N, N, bits); fmpz_add_ui(N, N, 1);
    ADF_CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
    ADF_CHECK(adf_idele_set_parts(x, I, r, u) == ADF_OK);
    arb_clear(I); fmpq_clear(r); adf_ucoset_clear(u); fmpz_clear(c); fmpz_clear(N);
}

/* Every status of adf_idele_root (gfunc.h): n = 0, 1, 2, 3; exact and inexact units; real balls of both signs; the
   combination; LIMIT; aliasing. A case fails on the status, the place or the value in its comment. */
ADF_TEST(idele_root_statuses)
{
    adf_idele_t x, y;
    adf_idele_init(x); adf_idele_init(y);
    /* an inexact unit (1 mod 8), a positive real ball */
    idele_mk(x, 4, 0, 4, 1, 1, 8);
    run_idele(y, x, ADF_IDELE_PREC_MAX + 1, 0, 64, ADF_DOMAIN, 0, "n huge even, sign 0", 0);
    run_idele(y, x, 2, 1, ADF_IDELE_PREC_MAX + 1, ADF_LIMIT, 1, "LIMIT", 0);
    run_idele(y, x, 0, 1, ADF_IDELE_PREC_MAX + 1, ADF_LIMIT, 1, "LIMIT before n = 0", 0);
    run_idele(y, x, 0, 1, 64, ADF_DOMAIN, 0, "n = 0", 0);
    run_idele(y, x, 1, 1, 64, ADF_OK, 0, "n = 1", 0);
    ADF_CHECK(adf_idele_identical(y, x));
    run_idele(y, x, 1, 9, 64, ADF_OK, 0, "n = 1, sign 9", 0);
    run_idele(y, x, 2, 1, 64, ADF_NOT_DETERMINED, 0, "inexact unit, n = 2", 0);
    run_idele(y, x, 2, -1, 64, ADF_NOT_DETERMINED, 0, "inexact unit, n = 2, sign -1", 0);
    run_idele(y, x, 3, 1, 64, ADF_NOT_DETERMINED, 0, "inexact unit, n = 3", 0);
    run_idele(y, x, 3, -1, 64, ADF_DOMAIN, 0, "n = 3, sign -1 (an idele is never 0)", 0);
    run_idele(y, x, 2, 2, 64, ADF_DOMAIN, 0, "sign 2", 0);
    /* an inexact unit with a negative real ball: DOMAIN at the real place for even n, NOT_DETERMINED for odd n */
    idele_mk(x, -4, 1, 4, 1, 1, 8);
    run_idele(y, x, 2, 1, 64, ADF_DOMAIN, 1, "negative real ball, inexact unit, n = 2", 0);
    run_idele(y, x, 3, 1, 64, ADF_NOT_DETERMINED, 0, "negative real ball, inexact unit, n = 3", 0);
    /* exact units: the rational contract */
    idele_mk(x, 4, 0, 4, 1, 1, 0);
    run_idele(y, x, 2, 1, 64, ADF_OK, 0, "(4, 4, [1]), n = 2", 0);
    ADF_CHECK(arf_equal_si(arb_midref(y->inf), 2) && arb_is_exact(y->inf) && fmpz_equal_si(fmpq_numref(y->r), 2)
              && fmpz_equal_si(y->u.c, 1) && fmpz_is_zero(y->u.N));
    run_idele(y, x, 2, -1, 64, ADF_OK, 0, "(4, 4, [1]), n = 2, sign -1", 0);
    ADF_CHECK(arf_equal_si(arb_midref(y->inf), -2) && fmpz_equal_si(fmpq_numref(y->r), 2)
              && fmpz_equal_si(y->u.c, -1) && fmpz_is_zero(y->u.N));
    run_idele(y, x, 3, 1, 64, ADF_DOMAIN, 0, "(4, 4, [1]), n = 3: 4 is no cube", 0);
    idele_mk(x, 4, 0, 4, 1, -1, 0);
    run_idele(y, x, 2, 1, 64, ADF_DOMAIN, 0, "(4, 4, [-1]), n = 2: the finite part -4", 0);
    idele_mk(x, -4, 0, 4, 1, 1, 0);
    run_idele(y, x, 2, 1, 64, ADF_DOMAIN, 1, "(-4, 4, [1]), n = 2: the real part", 0);
    idele_mk(x, -8, 0, 8, 27, 1, 0);
    run_idele(y, x, 3, 1, 64, ADF_OK, 0, "(-8, 8/27, [1]), n = 3", 0);
    ADF_CHECK(arb_contains_si(y->inf, -2) && fmpz_equal_si(fmpq_numref(y->r), 2)
              && fmpz_equal_si(fmpq_denref(y->r), 3) && fmpz_equal_si(y->u.c, 1));
    idele_mk(x, 8, 0, 8, 1, -1, 0);
    run_idele(y, x, 3, 1, 64, ADF_OK, 0, "(8, 8, [-1]), n = 3", 0);
    ADF_CHECK(fmpz_equal_si(y->u.c, -1) && arb_contains_si(y->inf, 2) && fmpz_is_zero(y->u.N));
    idele_mk(x, 2, 0, 2, 1, 1, 0);
    run_idele(y, x, 2, 1, 64, ADF_DOMAIN, 0, "(2, 2, [1]), n = 2", 0);
    run_idele(y, x, WMAX + 1, 1, 64, ADF_DOMAIN, 0, "(2, 2, [1]), n = 2^63", 0);
    idele_mk(x, 1, 0, 1, 1, 1, 0);
    run_idele(y, x, UWORD_MAX, 1, 64, ADF_OK, 0, "the idele 1, n = UWORD_MAX", 0);
    ADF_CHECK(adf_idele_identical(y, x));
    /* a real ball close to 0 at 2 bits: 1 +- (1 - 2^-20) excludes 0, and so must the root (or NOT_DETERMINED at the
       real place, never a ball that contains 0); the status is printed, the invariant checked by run_idele */
    {
        int st;
        idele_mk(x, 1, 0, 1, 1, 1, 0);
        mag_set_ui_2exp_si(arb_radref(x->inf), (1 << 20) - 1, -20);
        ADF_CHECK(arb_is_nonzero(x->inf));
        for (slong p = 2; p <= 8; p += 6)
            for (ulong n = 2; n <= 3; n++)
            {
                adf_place_t w = sentinel_place();
                sent_idele(y);
                st = adf_idele_root(y, &w, x, n, 1, p);
                ADF_CHECK(st == ADF_OK || (st == ADF_NOT_DETERMINED && is_real_place(w) && is_sent_idele(y)));
                ADF_CHECK(st != ADF_OK || (adf_idele_is_canonical(y) && arb_is_nonzero(y->inf)));
                printf("idele_root_statuses: (1 +- (1 - 2^-20), 1, [1]), n = %lu, prec %ld: %s\n", n, (long) p,
                       adf_status_str(st));
            }
    }
    /* the real root that arb cannot certify free of 0: 2^999 +- 2^960 is positive, its cube root at prec 2 is
       [4.48e102 +- 8.15e102] in FLINT 3.0.1 (measured, lanes/f-slice10/progress.md): NOT_DETERMINED at the real
       place, y untouched; at prec 64 the same idele has the root (2^333 +- ..., 2^333, [1]) */
    idele_mk(x, 1, 0, 1, 1, 1, 0);
    arb_mul_2exp_si(x->inf, x->inf, 999);
    mag_set_ui_2exp_si(arb_radref(x->inf), 1, 960);
    fmpz_one(fmpq_numref(x->r)); fmpz_mul_2exp(fmpq_numref(x->r), fmpq_numref(x->r), 999);
    ADF_CHECK(adf_idele_is_canonical(x));
    run_idele(y, x, 3, 1, 2, ADF_NOT_DETERMINED, 1, "(2^999 +- 2^960, 2^999, [1]), n = 3, prec 2", 0);
    run_idele(y, x, 3, 1, 64, ADF_OK, 0, "(2^999 +- 2^960, 2^999, [1]), n = 3, prec 64", 0);
    ADF_CHECK(fmpz_bits(fmpq_numref(y->r)) == 334 && arb_is_positive(y->inf));
    /* where = NULL accepted */
    idele_mk(x, -4, 0, 4, 1, 1, 0);
    ADF_CHECK(adf_idele_root(y, NULL, x, 2, 1, 64) == ADF_DOMAIN);
    adf_idele_clear(x); adf_idele_clear(y);
}

/* An idele whose unit coset is not exact cannot be certified to have an n-th root at all places for n >= 2
   (gfunc.h, adf_idele_root; Proposition 16 steps 1-2, functions.md:549-557): NOT_DETERMINED, `where` untouched,
   at every degree and both signs, unless a larger status comes first: an idele is never 0, so odd n with sign = -1
   is DOMAIN (an invalid selector, `where` untouched), and a negative real ball with even n is DOMAIN at the real
   place (N-D16, the maximum of DOMAIN and NOT_DETERMINED).
   Finding F1 of docs/reviews/f1/review-gfunc.md: the tests of the slice used an inexact unit at degrees 2 and 3
   only, so a mutant that treats an inexact unit as exact from n = 4 on (the reviewer's fault F13, x = (1 ; 1 *
   [1 mod 8]), n = 4) passed them. The degrees here include 4, 5, 6, 7, 8, 12 and above. A case fails on the status,
   on `where` where it must be untouched or named, or on an output written on a status. */
ADF_TEST(idele_root_inexact_units)
{
    static const ulong degrees[] = {2, 3, 4, 5, 6, 7, 8, 12, 64, (ulong) 1 << 32, WMAX, UWORD_MAX};
    static const slong uc[4] = {1, 5, 2, 1}, uN[4] = {8, 6, 3, 5};
    /* which of the five cosets is in normal form (N = 0 or N != 2 mod 4, ucoset.h:71-73): only [5 mod 6] is not */
    static const int unormal[5] = {1, 0, 1, 1, 1};
    adf_idele_t x, y;
    unsigned long before = cases;
    adf_idele_init(x); adf_idele_init(y);
    for (int unit = 0; unit < 5; unit++)
        for (int ball = 0; ball < 2; ball++)
        {
            char what[64];
            if (unit < 4)
                idele_mk(x, ball ? -1 : 1, 0, 1, 1, uc[unit], uN[unit]);
            else
                idele_mk_mod_bits(x, ball ? -1 : 1, 200);
            ADF_CHECK(adf_idele_is_canonical(x) && !adf_ucoset_is_exact(&x->u));
            ADF_CHECK_MSG(adf_ucoset_is_normal(&x->u) == unormal[unit], "unit %d: normal form", unit);
            snprintf(what, sizeof what, "unit %d, real ball %d", unit, ball);
            for (size_t i = 0; i < sizeof degrees / sizeof *degrees; i++)
                for (int sign = 1; sign >= -1; sign -= 2)
                {
                    ulong n = degrees[i];
                    int st, named;
                    if (sign == -1 && n % 2 == 1)
                    {
                        st = ADF_DOMAIN;         /* an idele is never 0: sign = -1 names no branch */
                        named = 0;
                    }
                    else if (ball && n % 2 == 0)
                    {
                        st = ADF_DOMAIN;         /* every point of a negative ball has no real root */
                        named = 1;
                    }
                    else
                    {
                        st = ADF_NOT_DETERMINED;
                        named = 0;
                    }
                    run_idele(y, x, n, sign, 64, st, named, what, 0);
                }
        }
    printf("idele_root_inexact_units: %lu calls\n", cases - before);
    adf_idele_clear(x); adf_idele_clear(y);
}

/* Degree 1 is the identity and an EXACT copy (gfunc.h:129-130; docs/api-1f8.md decision 7: "n = 1: the identity,
   an exact copy (the real ball is not rounded to prec)"). The unit of the idele below is canonical and NOT normal
   ([5 mod 6], whose normal form is [2 mod 3], ucoset.h:75-78): a copy must keep the stored pair, and the real ball
   of the adele has an inexact finite part and a midpoint of 300 bits with a radius of 2^-600, which a rounding to
   prec = 2 would change. Finding F2 of docs/reviews/f1/review-gfunc.md: a mutant that normalises the unit after
   the degree-1 copy passed the tests of the slice, whose only identity input used [1 mod 8], already normal.
   A case fails if any field differs. */
ADF_TEST(degree_one_is_an_exact_copy)
{
    adf_idele_t i, j;
    adf_adele_t x, y;
    adf_idele_init(i); adf_idele_init(j);
    adf_adele_init(x); adf_adele_init(y);
    idele_mk(i, 1, 0, 1, 1, 5, 6);
    ADF_CHECK(adf_idele_is_canonical(i) && adf_ucoset_is_canonical(&i->u) && !adf_ucoset_is_normal(&i->u));
    ADF_CHECK(fmpz_equal_si(i->u.c, 5) && fmpz_equal_si(i->u.N, 6));
    for (int sign = 1; sign >= -1; sign -= 2)
        for (slong prec = 2; prec <= 64; prec += 62)
        {
            run_idele(j, i, 1, sign, prec, ADF_OK, 0, "(1, 1, [5 mod 6]), n = 1", 0);
            ADF_CHECK_MSG(adf_idele_identical(j, i), "sign %d, prec %ld: the copy differs", sign, (long) prec);
            ADF_CHECK_MSG(arb_equal(j->inf, i->inf) && fmpq_equal(j->r, i->r)
                              && fmpz_equal(j->u.c, i->u.c) && fmpz_equal(j->u.N, i->u.N),
                          "sign %d, prec %ld: a field of the copy differs", sign, (long) prec);
            ADF_CHECK_MSG(fmpz_equal_si(j->u.c, 5) && fmpz_equal_si(j->u.N, 6),
                          "sign %d, prec %ld: the unit was normalised", sign, (long) prec);
        }
    /* an adele with an inexact finite part and a real ball of 300 bits: an exact copy at prec = 2 */
    adele_mk(x, 0, 0, 2, 4, 1);
    {
        fmpz_t v, e;
        fmpz_init(v); fmpz_init_set_si(e, -300);
        fmpz_one(v); fmpz_mul_2exp(v, v, 300); fmpz_add_ui(v, v, 12345);   /* 301 bits */
        arb_set_fmpz_2exp(x->inf, v, e);
        mag_set_ui_2exp_si(arb_radref(x->inf), 1, -600);
        fmpz_clear(v); fmpz_clear(e);
    }
    {
        arb_t r;
        arb_init(r);
        arb_set_round(r, x->inf, 2);      /* a rounding to prec 2 changes this ball */
        ADF_CHECK(adf_adele_is_canonical(x) && !adf_fball_is_exact(&x->fin) && !arb_equal(r, x->inf));
        arb_clear(r);
    }
    for (int sign = 1; sign >= -1; sign -= 2)
    {
        run_adele(y, x, 1, sign, 2, ADF_OK, 0, "((2^300 + 12345) 2^-300 +- 2^-600 ; 2 + 4 Zhat), n = 1", 0);
        ADF_CHECK_MSG(adf_adele_identical(y, x), "sign %d: the copy of the real ball was rounded", sign);
    }
    adf_idele_clear(i); adf_idele_clear(j);
    adf_adele_clear(x); adf_adele_clear(y);
}

/* ============================================================= slice B: exp, sin, sinh, cos, cosh at all places */

typedef int (*adele_fn)(adf_adele_t, adf_place_t *, const adf_adele_t, slong);
typedef int (*sball_fn)(adf_sball_t, adf_place_t *, const adf_sball_t, adf_place_t, slong);

static const struct
{
    const char *name;
    adele_fn f;
    sball_fn at;
    int constant;      /* f(0) at every prime: Proposition 12 step 1, functions.md:394 */
} SERIES[5] = {
    { "exp", adf_adele_exp, adf_sball_exp_at, 1 },
    { "sin", adf_adele_sin, adf_sball_sin_at, 0 },
    { "sinh", adf_adele_sinh, adf_sball_sinh_at, 0 },
    { "cos", adf_adele_cos, adf_sball_cos_at, 1 },
    { "cosh", adf_adele_cosh, adf_sball_cosh_at, 1 },
};

static int series_index(const char *name)
{
    for (int k = 0; k < 5; k++)
        if (strcmp(SERIES[k].name, name) == 0)
            return k;
    ADF_CHECK_MSG(0, "unknown function %s", name);
    return 0;
}

/* The real coordinate's status and ball by the function at the real place of rfunc.h, independently of gfunc.c. */
static int real_part_by_sball(arb_t J, int k, const arb_t I, slong prec)
{
    adf_sball_t s, t;
    int st;
    adf_sball_init(s); adf_sball_init(t);
    ADF_CHECK(adf_sball_set_arb_lballs(s, NULL, I, NULL, 0) == ADF_OK);
    st = SERIES[k].at(t, NULL, s, adf_place_inf(), prec);
    if (st == ADF_OK)
        ADF_CHECK(adf_sball_get_arb(J, t, adf_place_inf()) == ADF_OK);
    adf_sball_clear(s); adf_sball_clear(t);
    return st;
}

/* One call of the series k and the aliased call; the expectation st and the place: wp = 0 untouched, wp = 1 the
   real place, wp >= 2 that prime. A case fails on the status, y written on a status, the place, where written on
   OK, or an aliased call that differs. On OK it fails unless the finite part is the exact constant and the real
   part is identical to the ball of the function at the real place (rfunc.h). */
static int run_series(adf_adele_t y, int k, const adf_adele_t x, slong prec, int st, ulong wp, const char *what,
                      unsigned long line)
{
    adf_adele_t z;
    adf_place_t w = sentinel_place(), w2 = sentinel_place();
    int got, got2;
    adf_adele_init(z);
    sent_adele(y);
    got = SERIES[k].f(y, &w, x, prec);
    ADF_CHECK_MSG(got == st, "%s %s line %lu prec=%ld: status %d, want %d", SERIES[k].name, what, line, (long) prec,
                  got, st);
    if (got != ADF_OK)
        ADF_CHECK_MSG(is_sent_adele(y), "%s %s line %lu: y written on a status", SERIES[k].name, what, line);
    if (st == ADF_OK || wp == 0)
        ADF_CHECK_MSG(is_sentinel_place(w), "%s %s line %lu: where written", SERIES[k].name, what, line);
    else if (wp == 1)
        ADF_CHECK_MSG(is_real_place(w), "%s %s line %lu: where is not the real place", SERIES[k].name, what, line);
    else
        ADF_CHECK_MSG(!is_real_place(w) && adf_place_prime_get(w) == wp, "%s %s line %lu: where is %lu, want %lu",
                      SERIES[k].name, what, line, adf_place_prime_get(w), wp);
    if (got == ADF_OK)
    {
        arb_t J;
        arb_init(J);
        ADF_CHECK(real_part_by_sball(J, k, x->inf, prec) == ADF_OK);
        ADF_CHECK_MSG(arb_equal(J, y->inf), "%s %s line %lu: the real part is not the real function's ball",
                      SERIES[k].name, what, line);
        ADF_CHECK_MSG(adf_fball_is_exact(&y->fin) && fmpz_equal_si(y->fin.A, SERIES[k].constant)
                      && fmpz_is_one(y->fin.d), "%s %s line %lu: the finite part is not the exact %d",
                      SERIES[k].name,
                      what, line, SERIES[k].constant);
        arb_clear(J);
    }
    adf_adele_set(z, x);
    got2 = SERIES[k].f(z, &w2, z, prec);
    ADF_CHECK_MSG(got2 == got && (got == ADF_OK ? adf_adele_identical(z, y) : adf_adele_identical(z, x)),
                  "%s %s line %lu: aliasing y = x differs", SERIES[k].name, what, line);
    adf_adele_clear(z);
    cases += 2;
    return got;
}

/* Every row of series_real.jsonl (the image [lo, hi] of f over the real ball by mpmath intervals at 800 bits, and
   an upper bound lip of |f'|), as (I ; 0), (I ; 1), (I ; 0 + 4 Zhat). (I ; 0): OK, the real part contains [lo, hi]
   (widened by 2^-350 of its size), is identical to the ball of rfunc.h, and from 24 bits on has a radius at most
   2 lip rad(I) + 2^(6 - prec) max(|lo|, |hi|, 1); the finite part is the exact constant. (I ; 1): DOMAIN at 2, y
   untouched. (I ; 0 + 4 Zhat): NOT_DETERMINED, where untouched. A case fails on any of these or on aliasing. */
ADF_TEST(series_real_vectors)
{
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice10/series_real.jsonl");
    unsigned long before = cases;
    if (!f) return;
    for (size_t i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        int k = series_index(member_str(rec, "f"));
        slong prec = strtol(member_int(rec, "prec"), NULL, 10);
        unsigned long line = jsonl_line_of(rec);
        adf_adele_t x, y;
        adf_adele_init(x); adf_adele_init(y);
        adele_mk(x, 0, 0, 0, 0, 1);
        rb_from_json(x->inf, member(rec, "x"));
        if (run_series(y, k, x, prec, ADF_OK, 0, "series_real.jsonl", line) == ADF_OK)
        {
            arf_t lo, hi, lip, rad, rhs, t;
            slong p = prec < 2 ? 2 : prec;
            check_real_image(y->inf, rec, prec, 0, "series_real.jsonl");
            arf_init(lo); arf_init(hi); arf_init(lip); arf_init(rad); arf_init(rhs); arf_init(t);
            dy_from_json(lo, member(rec, "lo")); dy_from_json(hi, member(rec, "hi"));
            dy_from_json(lip, member(rec, "lip"));
            arf_abs(lo, lo); arf_abs(hi, hi); arf_max(t, lo, hi);
            if (arf_cmp_si(t, 1) < 0) arf_one(t);
            arf_mul_2exp_si(t, t, 6 - p);
            arf_set_mag(rad, arb_radref(x->inf));
            arf_mul(rhs, rad, lip, ARF_PREC_EXACT, ARF_RND_DOWN);
            arf_mul_2exp_si(rhs, rhs, 1);
            arf_add(rhs, rhs, t, 3000, ARF_RND_UP);
            arf_set_mag(rad, arb_radref(y->inf));
            if (p >= 24)
                ADF_CHECK_MSG(arf_cmp(rad, rhs) <= 0, "%s line %lu: radius above the Lipschitz bound",
                              SERIES[k].name, line);
            arf_clear(lo); arf_clear(hi); arf_clear(lip); arf_clear(rad); arf_clear(rhs); arf_clear(t);
        }
        adele_mk(x, 0, 0, 1, 0, 1);
        rb_from_json(x->inf, member(rec, "x"));
        run_series(y, k, x, prec, ADF_DOMAIN, 2, "(I ; 1)", line);
        adele_mk(x, 0, 0, 0, 4, 1);
        rb_from_json(x->inf, member(rec, "x"));
        run_series(y, k, x, prec, ADF_NOT_DETERMINED, 0, "(I ; 0 + 4 Zhat)", line);
        adf_adele_clear(x); adf_adele_clear(y);
    }
    printf("series_real_vectors: %zu rows, %lu calls\n", jsonl_count(f), cases - before);
    jsonl_close(f);
}

/* Every row of series_place.jsonl (q and the first prime p with v_p(q) < c_p, found in exact integers by trying
   2, 3, 5, ...; 0 for q = 0) as (1/2 ; q) for the five functions: q = 0 is OK, every other q DOMAIN with where =
   that prime, y untouched. A case fails on another status or another prime. */
ADF_TEST(series_place_vectors)
{
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice10/series_place.jsonl");
    unsigned long before = cases;
    if (!f) return;
    for (size_t i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        ulong p = ulong_of(member_int(rec, "where"));
        adf_adele_t x, y;
        adf_rat_t q;
        adf_adele_init(x); adf_adele_init(y); adf_rat_init(q);
        rat_str(q, member_int(rec, "num"), member_int(rec, "den"));
        adf_adele_set_rat(x, q, 64);
        arb_set_d(x->inf, 0.5);
        for (int k = 0; k < 5; k++)
            run_series(y, k, x, 64, p == 0 ? ADF_OK : ADF_DOMAIN, p, "series_place.jsonl", jsonl_line_of(rec));
        adf_adele_clear(x); adf_adele_clear(y); adf_rat_clear(q);
    }
    printf("series_place_vectors: %zu rows, %lu calls\n", jsonl_count(f), cases - before);
    jsonl_close(f);
}

/* The statuses of the five functions (gfunc.h): the finite parts of the brief, the combination with a real part
   that arb cannot evaluate, LIMIT, prec below 2, the local backend. The real status is taken from the function at
   the real place of rfunc.h (real_part_by_sball), not assumed. A case fails on the status or the place in its
comment. */ ADF_TEST(series_statuses)
{
    adf_adele_t x, y;
    adf_modctx_struct *ctx = NULL;
    ulong q0 = 36;
    arb_t J;
    adf_adele_init(x); adf_adele_init(y); arb_init(J);
    for (int k = 0; k < 5; k++)
    {
        int st_huge;
        /* LIMIT first, where = real, also before a finite part that is DOMAIN */
        adele_mk(x, 0, 0, 1, 0, 1);
        run_series(y, k, x, ADF_REAL_PREC_MAX + 1, ADF_LIMIT, 1, "LIMIT", 0);
        run_series(y, k, x, WORD_MAX, ADF_LIMIT, 1, "LIMIT at WORD_MAX", 0);
        adele_mk(x, 0, 0, 0, 0, 1);
        run_series(y, k, x, ADF_REAL_PREC_MAX, ADF_OK, 0, "prec at the limit", 0);
        run_series(y, k, x, -7, ADF_OK, 0, "prec -7", 0);
        run_series(y, k, x, 0, ADF_OK, 0, "prec 0", 0);
        /* the finite parts of the brief: 0 OK; 1, 4, 12, 1/3, -9/2 DOMAIN at 2, 3, 5, 2, 2; 60 at 7; 4/9 at 3 */
        adele_mk(x, 3, 1, 0, 0, 1); run_series(y, k, x, 64, ADF_OK, 0, "(3 +- 1 ; 0)", 0);
        adele_mk(x, 3, 1, 1, 0, 1); run_series(y, k, x, 64, ADF_DOMAIN, 2, "(3 +- 1 ; 1)", 0);
        adele_mk(x, 3, 1, 4, 0, 1); run_series(y, k, x, 64, ADF_DOMAIN, 3, "(3 +- 1 ; 4)", 0);
        adele_mk(x, 3, 1, 12, 0, 1); run_series(y, k, x, 64, ADF_DOMAIN, 5, "(3 +- 1 ; 12)", 0);
        adele_mk(x, 3, 1, 1, 0, 3); run_series(y, k, x, 64, ADF_DOMAIN, 2, "(3 +- 1 ; 1/3)", 0);
        adele_mk(x, 3, 1, -9, 0, 2); run_series(y, k, x, 64, ADF_DOMAIN, 2, "(3 +- 1 ; -9/2)", 0);
        adele_mk(x, 3, 1, 60, 0, 1); run_series(y, k, x, 64, ADF_DOMAIN, 7, "(3 +- 1 ; 60)", 0);
        adele_mk(x, 3, 1, 4, 0, 9); run_series(y, k, x, 64, ADF_DOMAIN, 3, "(3 +- 1 ; 4/9)", 0);
        /* balls: NOT_DETERMINED, where untouched, no prime examined (1 + 4 Zhat would be DOMAIN at 2 by a look) */
        adele_mk(x, 3, 1, 0, 4, 1); run_series(y, k, x, 64, ADF_NOT_DETERMINED, 0, "(3 +- 1 ; 0 + 4 Zhat)", 0);
        adele_mk(x, 3, 1, 3, 9, 1); run_series(y, k, x, 64, ADF_NOT_DETERMINED, 0, "(3 +- 1 ; 3 + 9 Zhat)", 0);
        adele_mk(x, 3, 1, 1, 4, 1); run_series(y, k, x, 64, ADF_NOT_DETERMINED, 0, "(3 +- 1 ; 1 + 4 Zhat)", 0);
        adele_mk(x, 0, 0, 0, 4, 1); run_series(y, k, x, 64, ADF_NOT_DETERMINED, 0, "(0 ; 0 + 4 Zhat)", 0);
        /* the local backend (never exact): NOT_DETERMINED, where untouched */
        ADF_CHECK(adf_modctx_new_blocks(&ctx, &q0, 1) == ADF_OK);
        adele_mk(x, 3, 1, 0, 36, 1);
        ADF_CHECK(adf_fball_set_local(&x->fin, &x->fin, ctx) == ADF_OK && x->fin.backend == ADF_LOCAL);
        run_series(y, k, x, 64, ADF_NOT_DETERMINED, 0, "(3 +- 1 ; local 0 + 36 Zhat)", 0);
        adf_adele_clear(x); adf_adele_init(x);
        adf_modctx_free(ctx); ctx = NULL;
        /* a real part that arb cannot evaluate (rfunc.h: exp(2^1000) is NOT_DETERMINED): the real status comes from
           rfunc.h; combined with 0, with 1 (DOMAIN at 2 dominates), with 0 + 4 Zhat (both NOT_DETERMINED: the real
           place first) */
        adele_mk(x, 1, 0, 0, 0, 1);
        arb_mul_2exp_si(x->inf, x->inf, 1000);
        st_huge = real_part_by_sball(J, k, x->inf, 64);
        printf("series_statuses: %s(2^1000) at the real place: %s\n", SERIES[k].name, adf_status_str(st_huge));
        ADF_CHECK(k == 1 || k == 3 || st_huge == ADF_NOT_DETERMINED);   /* exp, sinh, cosh overflow */
        run_series(y, k, x, 64, st_huge, st_huge == ADF_OK ? 0 : 1, "(2^1000 ; 0)", 0);
        fmpz_one(x->fin.A);
        run_series(y, k, x, 64, ADF_DOMAIN, 2, "(2^1000 ; 1)", 0);
        fmpz_zero(x->fin.A); fmpz_set_ui(x->fin.H, 4);
        run_series(y, k, x, 64, ADF_NOT_DETERMINED, st_huge == ADF_OK ? 0 : 1, "(2^1000 ; 0 + 4 Zhat)", 0);
        /* where = NULL is accepted */
        adele_mk(x, 0, 0, 12, 0, 1);
        ADF_CHECK(SERIES[k].f(y, NULL, x, 64) == ADF_DOMAIN);
    }
    /* hand values: exp(0) = 1, cos(0) = 1, sin(0) = 0 exactly; exp((1/2 ; 0)) contains 1.6487212707001281468 */
    adele_mk(x, 0, 0, 0, 0, 1);
    run_series(y, 0, x, 64, ADF_OK, 0, "exp (0 ; 0)", 0);
    ADF_CHECK(arb_is_one(y->inf) && fmpz_is_one(y->fin.A));
    run_series(y, 3, x, 64, ADF_OK, 0, "cos (0 ; 0)", 0);
    ADF_CHECK(arb_is_one(y->inf) && fmpz_is_one(y->fin.A));
    run_series(y, 1, x, 64, ADF_OK, 0, "sin (0 ; 0)", 0);
    ADF_CHECK(arb_is_zero(y->inf) && fmpz_is_zero(y->fin.A));
    arb_set_d(x->inf, 0.5);
    run_series(y, 0, x, 64, ADF_OK, 0, "exp (1/2 ; 0)", 0);
    ADF_CHECK(arb_set_str(J, "1.6487212707001281468 +/- 1e-19", 64) == 0);
    ADF_CHECK(arb_overlaps(y->inf, J) && mag_cmp_2exp_si(arb_radref(y->inf), -60) <= 0);
    adf_adele_clear(x); adf_adele_clear(y); arb_clear(J);
}

#ifdef ADF_CHECK_INVARIANTS
/* ------------------------------------------------------------------------------------ the entry check (F3) */

/* conventions 4.4, DECISION CV-09, line 288: with -DADF_CHECK_INVARIANTS every public function checks
   adf_<type>_is_canonical on each argument it reads and calls flint_abort with one line on stderr; the predicates
   never abort themselves (M1-D2, src/invariants.h:53-57). Finding F3 of docs/reviews/f1/review-gfunc.md:
   src/gfunc.c had no entry check. One root function and one series function are checked here, one per argument
   type, each in a child process: the forged argument must end by SIGABRT with a line naming the function and the
   type, and the canonical control must return normally and silently. The output argument is never read. */

enum { IC_RAT, IC_IDELE, IC_SERIES, IC_COUNT };

static const char * const ic_name[IC_COUNT] = {"adf_rat_root", "adf_idele_root", "adf_adele_exp"};
static const char * const ic_type[IC_COUNT] = {"adf_rat", "adf_idele", "adf_adele"};

/* forged = 1: the argument read by the call is not canonical (the rational 2/4, a NaN real ball). */
static void
ic_child(int which, int forged)
{
    adf_rat_t a, b;
    adf_adele_t x, y;
    adf_idele_t i, j;
    adf_rat_init(a); adf_rat_init(b); adf_adele_init(x); adf_adele_init(y); adf_idele_init(i); adf_idele_init(j);
    fmpz_set_si(fmpq_numref(a->q), forged ? 2 : 1);
    fmpz_set_si(fmpq_denref(a->q), 4);                       /* 2/4 is not canonical, 1/4 is */
    fmpq_canonicalise(a->q);                                 /* only the forged case keeps the fraction */
    fmpq_one(i->r);                                         /* the exact idele 1 */
    adf_ucoset_one(&i->u);
    arb_one(i->inf);
    adf_fball_set_si(&x->fin, 1);
    arb_one(x->inf);
    if (forged)
    {
        if (which == IC_RAT)
            fmpz_set_si(fmpq_numref(a->q), 2), fmpz_set_si(fmpq_denref(a->q), 4);
        else if (which == IC_IDELE)
            arb_indeterminate(i->inf);
        else
            arb_indeterminate(x->inf);
    }
    if (which == IC_RAT)
        (void) adf_rat_root(b, NULL, a, 2, 1);
    else if (which == IC_IDELE)
        (void) adf_idele_root(j, NULL, i, 2, 1, 64);
    else
        (void) adf_adele_exp(y, NULL, x, 64);
    /* the control returns here: the values are canonical again, so they can be cleared */
    fmpq_canonicalise(a->q);
    arb_one(i->inf);
    arb_one(x->inf);
    adf_rat_clear(a); adf_rat_clear(b);
    adf_adele_clear(x); adf_adele_clear(y);
    adf_idele_clear(i); adf_idele_clear(j);
    flint_cleanup();
}

/* Runs the child; returns the signal (0 if it exited) and copies stderr into err. */
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

ADF_TEST(entry_check_of_the_public_functions)
{
    int i, code;
    char err[512];
    for (i = 0; i < IC_COUNT; i++)
    {
        int sig = ic_run(i, 0, &code, err, sizeof err);
        ADF_CHECK_MSG(sig == 0 && code == 0 && err[0] == 0,
                      "%s (case %d): control: signal %d, exit %d, stderr '%s'", ic_name[i], i, sig, code, err);
        sig = ic_run(i, 1, &code, err, sizeof err);
        ADF_CHECK_MSG(sig == SIGABRT, "%s (case %d): a forged argument did not abort (signal %d, exit %d)",
                      ic_name[i], i, sig, code);
        ADF_CHECK_MSG(strstr(err, "ADF_CHECK_INVARIANTS") != NULL && strstr(err, ic_name[i]) != NULL
                          && strstr(err, ic_type[i]) != NULL,
                      "%s (case %d): stderr '%s'", ic_name[i], i, err);
    }
}
#endif /* ADF_CHECK_INVARIANTS */
