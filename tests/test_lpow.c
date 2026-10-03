/* tests/test_lpow.c: rational powers and principal-unit powers at a prime (lane f-slice9; include/adelefeld/lpow.h;
   docs/api-1f6.md P1 to P8).

   Oracles.
   1. tests/ref/vectors/f-slice9/powrat_balls.jsonl, made by proto/lpow_checks.py with exact integers only: for every
      unit ball U + p^r Z_p (p = 2, 3, 5, 7; r up to 6, 4, 3, 2) and 22 fractions e/n (reduced and not), the seeds
      that are branches and the unit centre gamma of the image of the branch, checked there as a SET modulo
      p^(Erel+1) against gamma + p^Erel Z_p (output precision: the centre modulo p^Erel, the exponent Erel). Here
      the rows are run unscaled and scaled by p^(n' j), j = -1, 1, 2 (P7(c): the image moves to p^(e' j) times
      the image), at the requested N = BIG (the image itself), N = E', E' - 1, E' - 3 (the ball at N that contains
      it, N-D14), N = e' j (the ball around 0); every other seed must be DOMAIN, unguarded rows NOT_DETERMINED.
   2. powrat_exact.jsonl: exact units p^m U, every seed; the exact rational value or the ball at the stated N
      (5 at p = 2, 3; 4 at p = 5, 7) with the centre found by exhaustive roots modulo p^h.
   3. powunit.jsonl: Proposition 18 and P5: the set {u^s mod p^(R+1)} enumerated over the base ball and over the
      integer exponents s0 + p^B t; the C result must be the ball (centre mod p^R, exponent R) at N = BIG and the
      ball at min(N, R) that contains it at N = R, R - 1, R - 2, 1, 0, -2; exact inputs the value modulo p^N.
   4. adf_lball_pow_si (L12) for e/n with n' = 1 (identical fields and status) and adf_lball_root_seed for e = 1
      (identical fields): the brief demands that n = 1 agrees with pow_si exactly.
   5. Hand values and identities with a stated output precision (u^(s+t) = u^s u^t, (uv)^s = u^s v^s, integer
      powers, principal roots), p = 2^64 - 59 at N = 200, inputs of thousands of bits, every status, the limits,
      every aliasing combination, untouched outputs on a status.
   What would make a case fail is stated at each test. */

#include <limits.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include <flint/ulong_extras.h>
#include "support/jsonl.h"
#include "test_runner.h"

#define BIG ADF_LBALL_EXP_MAX
#define EMAX ADF_LBALL_EXP_MAX

static unsigned long cases;   /* calls compared, printed per test */

/* ------------------------------------------------------------------------------------------------------ helpers */

static adf_place_t place_of(ulong p)
{
    adf_place_t v;
    ADF_CHECK(adf_place_prime(&v, p) == ADF_OK);
    return v;
}

static const jsonl_value *member(const jsonl_value *r, const char *key)
{
    const jsonl_value *v = NULL;
    jsonl_error_t e;
    ADF_CHECK_MSG(jsonl_field(r, key, &v, &e), "%s", jsonl_error_message(&e));
    return v;
}
static int has(const jsonl_value *r, const char *key)
{
    const jsonl_value *v = NULL;
    jsonl_error_t e;
    return jsonl_field(r, key, &v, &e);
}
static const char *itext(const jsonl_value *v)
{
    const char *s = jsonl_int_text(v, NULL);
    ADF_CHECK(s != NULL);
    return s ? s : "0";
}
static slong num(const jsonl_value *r, const char *key) { return strtol(itext(member(r, key)), NULL, 10); }
static slong at(const jsonl_value *a, size_t i, size_t k)
{
    return strtol(itext(jsonl_at(jsonl_at(a, i, NULL), k, NULL)), NULL, 10);
}
static void fz(fmpz_t z, const jsonl_value *v) { ADF_CHECK(fmpz_set_str(z, itext(v), 10) == 0); }

static jsonl_file *open_vectors(const char *path)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    ADF_CHECK_MSG(jsonl_open(path, &f, &err), "%s", jsonl_error_message(&err));
    return f;
}

/* x = the exact p^v (a/b), a/b a unit at p (or 0 with v = 0); raw fields, checked canonical. */
static void exact_fz(adf_lball_t x, ulong p, const fmpz_t a, const fmpz_t b, slong v)
{
    x->p = p; x->v = fmpz_is_zero(a) ? 0 : v; x->N = 0; x->exact = 1;
    fmpq_set_fmpz_frac(x->u, a, b);
    ADF_CHECK(adf_lball_is_canonical(x));
}
static void exact_si(adf_lball_t x, ulong p, slong a, slong b, slong v)
{
    fmpz_t A, B;
    fmpz_init_set_si(A, a); fmpz_init_set_si(B, b);
    exact_fz(x, p, A, B, v);
    fmpz_clear(A); fmpz_clear(B);
}
/* x = the ball p^v (u + p^(N - v) Z_p), u a unit integer below p^(N - v) (or 0 with v = 0): raw fields. */
static void ball_si(adf_lball_t x, ulong p, slong u, slong v, slong N)
{
    x->p = p; x->v = u ? v : 0; x->N = N; x->exact = 0;
    fmpq_set_si(x->u, u, 1);
    ADF_CHECK(adf_lball_is_canonical(x));
}
static void ball_fz(adf_lball_t x, ulong p, const fmpz_t u, slong v, slong N)
{
    x->p = p; x->v = fmpz_is_zero(u) ? 0 : v; x->N = N; x->exact = 0;
    fmpz_set(fmpq_numref(x->u), u); fmpz_one(fmpq_denref(x->u));
    ADF_CHECK(adf_lball_is_canonical(x));
}
/* x = the rational a/b of Q (any valuation) as an exact value at p, through the constructor of lball.h. */
static void rat_lb(adf_lball_t x, ulong p, const fmpz_t a, const fmpz_t b)
{
    adf_rat_t r;
    adf_rat_init(r);
    ADF_CHECK(adf_rat_set_fmpz2(r, a, b) == ADF_OK);
    ADF_CHECK(adf_lball_set_rat(x, place_of(p), r) == ADF_OK);
    adf_rat_clear(r);
}
static void rat_lb_si(adf_lball_t x, ulong p, slong a, slong b)
{
    fmpz_t A, B;
    fmpz_init_set_si(A, a); fmpz_init_set_si(B, b);
    rat_lb(x, p, A, B);
    fmpz_clear(A); fmpz_clear(B);
}
/* x = the ball c + p^N Z_p for the integer c >= 0 (canonical reduction by set_rat_ball). */
static void cball_si(adf_lball_t x, ulong p, slong c, slong N)
{
    adf_rat_t r;
    fmpz_t a, b;
    fmpz_init_set_si(a, c); fmpz_init_set_si(b, 1);
    adf_rat_init(r);
    ADF_CHECK(adf_rat_set_fmpz2(r, a, b) == ADF_OK);
    ADF_CHECK(adf_lball_set_rat_ball(x, place_of(p), r, N) == ADF_OK);
    adf_rat_clear(r); fmpz_clear(a); fmpz_clear(b);
}

/* The sentinel output: the exact 17 at 11 (a value no call below returns). */
static void sentinel(adf_lball_t y) { exact_si(y, 11, 17, 1, 0); }
static int is_sentinel(const adf_lball_t y)
{
    adf_lball_t t;
    int r;
    adf_lball_init(t);
    sentinel(t);
    r = adf_lball_identical(y, t);
    adf_lball_clear(t);
    return r;
}

/* want = the ball of exponent K <= E that contains the image img = p^v (b + p^(E - v) Z_p) (N-D14): the ball p^K Z_p
   around 0 for K <= v, else centre b mod p^(K - v), valuation v (as tests/test_lroot.c). For K >= E the image. */
static void coarse(adf_lball_t want, const adf_lball_t img, slong K)
{
    fmpz_t r;
    if (K >= img->N) { adf_lball_set(want, img); return; }
    fmpz_init(r);
    if (fmpq_is_zero(img->u) || K <= img->v) { want->p = img->p; want->v = 0; want->N = K; want->exact = 0;
                                               fmpq_zero(want->u); }
    else
    {
        ADF_CHECK(adf_lball_unit_mod(r, img, K - img->v) == ADF_OK);
        want->p = img->p; want->v = img->v; want->N = K; want->exact = 0; fmpq_set_fmpz(want->u, r);
    }
    ADF_CHECK(adf_lball_is_canonical(want));
    fmpz_clear(r);
}

static void reduce(slong *e1, ulong *n1, slong e, ulong n)
{
    ulong ae = e < 0 ? -(ulong) e : (ulong) e, g = n_gcd(ae, n);
    *n1 = n / g;
    *e1 = g == 1 ? e : (e < 0 ? -(slong) (ae / g) : (slong) (ae / g));
}

/* One call of powrat compared with the expected status and value; y = x aliased; outputs untouched on a status.
   Fails on: another status, other fields, a written output on a status, an aliased call that differs. */
static void check_powrat(const adf_lball_t x, slong e, ulong n, ulong seed, slong N, int st, const adf_lball_t want)
{
    adf_lball_t y, z;
    adf_lball_init(y); adf_lball_init(z);
    sentinel(y);
    adf_lball_set(z, x);
    int got = adf_lball_powrat(y, x, e, n, seed, N);
    ADF_CHECK_MSG(got == st, "powrat p=%lu e=%ld n=%lu seed=%lu N=%ld: status %d, expected %d",
                  x->p, e, n, seed, N, got, st);
    if (st == ADF_OK && got == ADF_OK)
    {
        ADF_CHECK_MSG(adf_lball_identical(y, want), "powrat p=%lu e=%ld n=%lu seed=%lu N=%ld: wrong value",
                      x->p, e, n, seed, N);
        ADF_CHECK(adf_lball_is_canonical(y));
    }
    else if (st != ADF_OK)
        ADF_CHECK(is_sentinel(y));
    ADF_CHECK(adf_lball_powrat(z, z, e, n, seed, N) == st);
    ADF_CHECK(adf_lball_identical(z, st == ADF_OK ? want : x));
    cases++;
    adf_lball_clear(y); adf_lball_clear(z);
}

/* ----------------------------------------------------------------------- 1. rational powers of the grid balls */

/* Every row of powrat_balls.jsonl, unscaled and scaled by p^(n' j), j = -1, 1, 2; for n' = 1 the comparison with
   pow_si; plus the rows with a valuation not divisible by n' (DOMAIN when guarded, else NOT_DETERMINED).
   Fails on: a wrong status for any seed, an exponent other than min(N, E'), a centre other than the oracle's
   gamma modulo p^min(N,E') - e' j, a result for an invalid seed, a written output on a status. */
ADF_TEST(powrat_grid_balls)
{
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice9/powrat_balls.jsonl");
    adf_lball_t x, img, want, ps;
    size_t ok_rows = 0;
    if (!f) return;
    ADF_CHECK(jsonl_count(f) == 6930);
    cases = 0;
    adf_lball_init(x); adf_lball_init(img); adf_lball_init(want); adf_lball_init(ps);
    for (size_t k = 0; k < jsonl_count(f); k++)
    {
        const jsonl_value *r = jsonl_record(f, k), *ok = member(r, "ok");
        ulong p = (ulong) num(r, "p"), n = (ulong) num(r, "n"), n1;
        slong e = num(r, "e"), U = num(r, "U"), rr = num(r, "r"), E = num(r, "E"), e1;
        size_t nok = jsonl_size(ok);
        reduce(&e1, &n1, e, n);
        for (slong j = -1; j <= 2; j++)
        {
            slong m = (slong) n1 * j;
            ball_si(x, p, U, m, m + rr);
            if (n1 == 1)
            {
                int st = adf_lball_pow_si(ps, x, e1);
                for (int t = 0; t < 3; t++)
                    check_powrat(x, e, n, t == 0 ? 0 : t == 1 ? 1 : 7, t == 0 ? BIG : t == 1 ? 3 : -2, st, ps);
                continue;
            }
            for (ulong seed = 0; seed <= (p == 2 ? 4 : p); seed++)
            {
                int st = E < 0 ? ADF_NOT_DETERMINED : ADF_DOMAIN;
                slong gamma = 0;
                for (size_t i = 0; i < nok; i++)
                    if ((ulong) at(ok, i, 0) == seed) { st = ADF_OK; gamma = at(ok, i, 1); }
                if (st != ADF_OK) { check_powrat(x, e, n, seed, BIG, st, NULL); continue; }
                ok_rows++;
                slong ej = e1 * j, Ep = ej + E;
                ball_si(img, p, gamma, ej, Ep);
                slong Ns[6] = { BIG, Ep, Ep - 1, Ep - 3, ej, ej - 2 };
                for (int t = 0; t < 6; t++)
                {
                    coarse(want, img, Ns[t] < Ep ? Ns[t] : Ep);
                    check_powrat(x, e, n, seed, Ns[t], ADF_OK, want);
                }
            }
            /* valuation m + 1, not divisible by n' >= 2 */
            ball_si(x, p, U, m + 1, m + 1 + rr);
            check_powrat(x, e, n, 1, BIG, E < 0 ? ADF_NOT_DETERMINED : ADF_DOMAIN, NULL);
        }
    }
    printf("  powrat grid rows %zu, OK branch rows %zu, calls %lu\n", jsonl_count(f), ok_rows, cases);
    adf_lball_clear(x); adf_lball_clear(img); adf_lball_clear(want); adf_lball_clear(ps);
    jsonl_close(f);
}

/* ------------------------------------------------------------------------------- 2. rational powers, exact */

/* Every row of powrat_exact.jsonl, every seed 0..p (0..4 at 2). Fails on: a status other than the oracle's, a ball
   for a rational branch or an exact value for an irrational one, a centre other than root^e' mod p^(N - e' j),
   an exponent other than N. Also: an exact result r must satisfy r^n' = x^e' exactly (pow_si). */
ADF_TEST(powrat_exact_rows)
{
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice9/powrat_exact.jsonl");
    adf_lball_t x, want, y, a, b;
    fmpz_t A, B;
    size_t exact = 0, balls = 0;
    if (!f) return;
    ADF_CHECK(jsonl_count(f) == 3024);
    cases = 0;
    adf_lball_init(x); adf_lball_init(want); adf_lball_init(y); adf_lball_init(a); adf_lball_init(b);
    fmpz_init(A); fmpz_init(B);
    for (size_t k = 0; k < jsonl_count(f); k++)
    {
        const jsonl_value *r = jsonl_record(f, k), *ok = member(r, "ok");
        ulong p = (ulong) num(r, "p"), n = (ulong) num(r, "n"), n1;
        slong e = num(r, "e"), N = num(r, "N"), e1;
        reduce(&e1, &n1, e, n);
        fz(A, member(r, "num")); fz(B, member(r, "den"));
        exact_fz(x, p, A, B, num(r, "m"));
        for (ulong seed = 0; seed <= (p == 2 ? 4 : p); seed++)
        {
            int st = ADF_DOMAIN;
            for (size_t i = 0; i < jsonl_size(ok); i++)
            {
                const jsonl_value *o = jsonl_at(ok, i, NULL);
                if ((ulong) at(ok, i, 0) != seed) continue;
                st = ADF_OK;
                fz(A, jsonl_at(o, 3, NULL)); fz(B, jsonl_at(o, 4, NULL));
                if (at(ok, i, 1)) { exact_fz(want, p, A, B, at(ok, i, 2)); exact++; }
                else { ball_fz(want, p, A, at(ok, i, 2), N); balls++; }
            }
            check_powrat(x, e, n, seed, N, st, want);
            if (st == ADF_OK && want->exact)
            {
                ADF_CHECK(adf_lball_pow_si(a, want, (slong) n1) == ADF_OK);
                ADF_CHECK(adf_lball_pow_si(b, x, e1) == ADF_OK);
                ADF_CHECK(adf_lball_equal_set(a, b));
            }
        }
    }
    printf("  powrat exact rows %zu: exact results %zu, balls %zu, calls %lu\n", jsonl_count(f), exact, balls,
           cases);
    fmpz_clear(A); fmpz_clear(B);
    adf_lball_clear(x); adf_lball_clear(want); adf_lball_clear(y); adf_lball_clear(a); adf_lball_clear(b);
    jsonl_close(f);
}

/* ------------------------------------------------------------- 3. the degree 1: pow_si; e = 1: root_seed */

/* n' = 1 is adf_lball_pow_si EXACTLY (lpow.h, P1): same status, identical fields, for zero, zero balls, balls of
   negative valuation, exact values, e = 0, LONG_MIN, LONG_MAX, non-reduced fractions, any seed and N. Fails on any
   difference. And 1/n (also 2/(2n)) is adf_lball_root_seed exactly. */
ADF_TEST(degree_one_is_pow_si_and_numerator_one_is_root)
{
    adf_lball_struct xs[10];
    adf_lball_t want, y;
    const slong es[7] = { 0, 1, -1, 3, -4, LONG_MIN, LONG_MAX };
    cases = 0;
    for (int i = 0; i < 10; i++) adf_lball_init(xs + i);
    adf_lball_init(want); adf_lball_init(y);
    exact_si(xs + 0, 5, 0, 1, 0);           /* the exact 0 */
    ball_si(xs + 1, 5, 0, 0, 3);            /* O(5^3) */
    ball_si(xs + 2, 5, 0, 0, -2);           /* O(5^-2) */
    exact_si(xs + 3, 3, 5, 7, -2);          /* 5/63 at 3 */
    ball_si(xs + 4, 2, 3, -3, 1);           /* 3/8 + 2 Z_2 */
    ball_si(xs + 5, 2, 1, 0, 1);            /* 1 + 2 Z_2: pow_si's case e = 1 of L12 */
    ball_si(xs + 6, 7, 3, 2, 6);
    exact_si(xs + 7, 2, -1, 1, 0);
    ball_si(xs + 8, 3, 2, 60, EMAX);        /* exponent at the bound */
    exact_si(xs + 9, 13, 1, 1, EMAX);       /* the exact 13^(2^60) */
    for (int i = 0; i < 10; i++)
        for (int k = 0; k < 7; k++)
            for (ulong mult = 1; mult <= 3; mult++)
            {
                slong e = es[k];
                ulong n = 1;
                int st;
                if (mult > 1)
                {
                    if (e == LONG_MIN || e == LONG_MAX || (e != 0 && (e > 1000 || e < -1000))) continue;
                    e *= (slong) mult; n = mult;
                }
                st = adf_lball_pow_si(want, xs + i, es[k]);
                for (ulong seed = 0; seed < 3; seed++)
                    check_powrat(xs + i, e, n, seed, seed == 0 ? BIG : seed == 1 ? 2 : LONG_MIN, st, want);
            }
    /* e/n = 0/0 is DOMAIN: the denominator 0 is invalid before the fraction is reduced */
    for (int i = 0; i < 10; i++) check_powrat(xs + i, 0, 0, 0, 5, ADF_DOMAIN, NULL);
    for (int i = 0; i < 10; i++) check_powrat(xs + i, 3, 0, 1, 5, ADF_DOMAIN, NULL);
    /* 1/n and 2/(2n) against root_seed: degrees 2, 3, 4, 6 at 2, 3, 5, 7, 13; exact and balls */
    for (ulong p = 2; p <= 13; p = n_nextprime(p, 1))
        for (ulong n = 2; n <= 6; n++)
            for (slong a = 1; a < 60; a += 7)
                for (int kind = 0; kind < 3; kind++)
                {
                    adf_lball_t x;
                    adf_lball_init(x);
                    if (kind == 0) rat_lb_si(x, p, a * a * a * a, 1);
                    else if (kind == 1) cball_si(x, p, (slong) n_pow(p, n) * a * a, (slong) n + 6);
                    else cball_si(x, p, 1 + (slong) p * a, 8);
                    for (ulong seed = 0; seed <= (p == 2 ? 3 : p - 1); seed++)
                        for (slong N = -1; N <= 9; N += 5)
                        {
                            int st = adf_lball_root_seed(want, x, n, seed, N);
                            check_powrat(x, 1, n, seed, N, st, want);
                            check_powrat(x, 2, 2 * n, seed, N, st, want);
                        }
                    adf_lball_clear(x);
                }
    printf("  pow_si and root_seed agreement: calls %lu\n", cases);
    for (int i = 0; i < 10; i++) adf_lball_clear(xs + i);
    adf_lball_clear(want); adf_lball_clear(y);
}

/* -------------------------------------------------------------------------------- 4. zero, hand values */

/* The exact 0 and balls around 0 at n' >= 2; 9^(3/2) at 5 and at 2; 8^(2/3) at 5; 2^(1/2) at 2 and at 7;
   (1 + 16 Z_2)^(3/2). Each expected value is computed in the comment. Fails on another status or value. */
ADF_TEST(powrat_zero_and_hand_values)
{
    adf_lball_t x, want, y, sq;
    cases = 0;
    adf_lball_init(x); adf_lball_init(want); adf_lball_init(y); adf_lball_init(sq);
    exact_si(x, 5, 0, 1, 0);
    exact_si(want, 5, 0, 1, 0);
    check_powrat(x, 3, 2, 0, 9, ADF_OK, want);              /* 0^(3/2) = 0, the seed of the exact 0 is 0 */
    check_powrat(x, 6, 4, 0, 9, ADF_OK, want);              /* 6/4 = 3/2 */
    check_powrat(x, 3, 2, 1, 9, ADF_DOMAIN, NULL);          /* seed 1 names no branch of 0 */
    check_powrat(x, -3, 2, 0, 9, ADF_NOT_UNIT, NULL);       /* 0^(-3/2): a negative exponent excludes 0 */
    check_powrat(x, -3, 2, 1, 9, ADF_DOMAIN, NULL);         /* the seed is checked before the sign of e' */
    ball_si(x, 5, 0, 0, 4);                                 /* O(5^4): meets the domain (0) and its complement */
    check_powrat(x, 1, 2, 0, 9, ADF_NOT_DETERMINED, NULL);
    check_powrat(x, -1, 2, 0, 9, ADF_NOT_DETERMINED, NULL);
    check_powrat(x, 4, 3, 1, 9, ADF_NOT_DETERMINED, NULL);
    exact_si(want, 5, 1, 1, 0);
    check_powrat(x, 0, 2, 0, 9, ADF_OK, want);              /* e = 0: the exact 1 (L12) */
    /* 9^(3/2) at 5: the roots of 9 are 3 (unit residue 3) and -3 (residue 2); 3^3 = 27, (-3)^3 = -27, exact */
    exact_si(x, 5, 9, 1, 0);
    exact_si(want, 5, 27, 1, 0);
    check_powrat(x, 3, 2, 3, 9, ADF_OK, want);
    check_powrat(x, 9, 6, 3, 9, ADF_OK, want);
    exact_si(want, 5, -27, 1, 0);
    check_powrat(x, 3, 2, 2, 9, ADF_OK, want);
    check_powrat(x, 3, 2, 1, 9, ADF_DOMAIN, NULL);          /* 1 is not a square root of 9 modulo 5 */
    exact_si(want, 5, -1, 27, 0);
    check_powrat(x, -3, 2, 2, 9, ADF_OK, want);             /* 9^(-3/2) on the branch -3: -1/27 */
    /* at 2: the identifier of the root 3 is 3 (3 = 3 mod 4), of -3 is 1 */
    exact_si(x, 2, 9, 1, 0);
    exact_si(want, 2, 27, 1, 0);
    check_powrat(x, 3, 2, 3, 9, ADF_OK, want);
    exact_si(want, 2, -27, 1, 0);
    check_powrat(x, 3, 2, 1, 9, ADF_OK, want);
    /* 8^(2/3) at 5: gcd(3, 4) = 1, one branch, the root 2 (seed 2): 4 */
    exact_si(x, 5, 8, 1, 0);
    exact_si(want, 5, 4, 1, 0);
    check_powrat(x, 2, 3, 2, 9, ADF_OK, want);
    check_powrat(x, 2, 3, 3, 9, ADF_DOMAIN, NULL);
    /* 2^(1/2) at 2: m = 1 is odd: DOMAIN for both seeds */
    exact_si(x, 2, 1, 1, 1);
    check_powrat(x, 1, 2, 1, 9, ADF_DOMAIN, NULL);
    check_powrat(x, 1, 2, 3, 9, ADF_DOMAIN, NULL);
    /* 2^(1/2) at 7: 2 = 3^2 = 4^2 mod 7: seeds 3 and 4, irrational: balls at N = 9 whose squares contain 2 */
    exact_si(x, 7, 2, 1, 0);
    for (ulong seed = 3; seed <= 4; seed++)
    {
        ADF_CHECK(adf_lball_powrat(y, x, 1, 2, seed, 9) == ADF_OK);
        ADF_CHECK(!y->exact && y->N == 9 && y->v == 0 && fmpz_fdiv_ui(fmpq_numref(y->u), 7) == seed);
        ADF_CHECK(adf_lball_pow_si(sq, y, 2) == ADF_OK && adf_lball_contains(x, sq));
        /* 2^(3/2) = 2 * 2^(1/2) on the same branch: the ball of 2 y at N = 9 */
        ADF_CHECK(adf_lball_powrat(want, x, 3, 2, seed, 9) == ADF_OK);
        ADF_CHECK(adf_lball_mul(sq, y, x) == ADF_OK && adf_lball_equal_set(sq, want));
        cases += 2;
    }
    /* (1 + 16 Z_2)^(3/2), seed 3: the root ball 7 + O(2^3) (tests/julia/lroot.jl); 7^3 = 343 = 7 mod 8 and E' =
       0 + 4 - 1 + 0 = 3 (P2): 7 + O(2^3); at N = 2: 3 + O(2^2) */
    ball_si(x, 2, 1, 0, 4);
    ball_si(want, 2, 7, 0, 3);
    check_powrat(x, 3, 2, 3, 20, ADF_OK, want);
    ball_si(want, 2, 3, 0, 2);
    check_powrat(x, 3, 2, 3, 2, ADF_OK, want);
    /* (1 + 4 Z_2)^(1/2): outside the guard (5 is not a square): NOT_DETERMINED */
    ball_si(x, 2, 1, 0, 2);
    check_powrat(x, 1, 2, 1, 20, ADF_NOT_DETERMINED, NULL);
    printf("  zero and hand values: calls %lu\n", cases);
    adf_lball_clear(x); adf_lball_clear(want); adf_lball_clear(y); adf_lball_clear(sq);
}

/* ------------------------------------------------------------- 5. large prime, thousands of bits, limits */

/* p = 2^64 - 59, N up to 200: x = b^n (b random, 1 to 3 words) exact and as a ball of relative precision 200; the
   result y of x^(e/n) on the branch of b: exact b^e when b is a rational branch (the branch of b itself);
   otherwise y^n' must contain x^e' (pow_si) and the unit residue of y must be seed^e' mod p.
   Thousands of bits: b of 3000 bits at p = 5 and 2. Fails on a wrong status, a lost enclosure or a wrong branch. */
ADF_TEST(powrat_large_prime_and_big_inputs)
{
    const ulong P = UWORD_MAX - 58;   /* 2^64 - 59, a prime */
    flint_rand_t st;
    adf_lball_t x, y, a, b, want;
    fmpz_t B, one, r;
    cases = 0;
    flint_randinit(st);
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(a); adf_lball_init(b); adf_lball_init(want);
    fmpz_init(B); fmpz_init_set_ui(one, 1); fmpz_init(r);
    ADF_CHECK(n_is_prime(P));
    for (int it = 0; it < 60; it++)
    {
        const ulong ns[4] = { 2, 3, 4, 6 };
        const slong es[5] = { 1, -1, 5, -7, 11 };
        ulong n = ns[it % 4];
        slong e = es[it % 5], e1;
        ulong n1;
        reduce(&e1, &n1, e, n);
        do fmpz_randbits(B, st, 64 * (1 + it % 3)); while ((fmpz_fdiv_ui(B, P) == 0) || fmpz_is_zero(B));
        fmpz_abs(B, B);
        ulong seed = fmpz_fdiv_ui(B, P);
        rat_lb(b, P, B, one);                  /* the root itself, exact */
        ADF_CHECK(adf_lball_pow_si(x, b, (slong) n1) == ADF_OK);   /* x = b^n', exact */
        /* exact x: the branch of b is rational, so the result is the exact b^e' */
        ADF_CHECK(adf_lball_pow_si(want, b, e1) == ADF_OK);
        check_powrat(x, e, n, seed, 200, ADF_OK, want);
        /* the ball x + p^(200) Z_p: E' = e' * 0 + 200 - 0 + 0 = 200; y^n' contains x^e' */
        ADF_CHECK(adf_lball_unit_mod(r, x, 200) == ADF_OK);
        ball_fz(a, P, r, 0, 200);
        for (slong N = 50; N <= 200; N += 150)
        {
            ulong res = n_powmod2(seed, e1 < 0 ? (ulong) -e1 : (ulong) e1, P);
            if (e1 < 0) res = n_invmod(res, P);
            rat_lb(b, P, B, one);
            ADF_CHECK(adf_lball_pow_si(want, b, e1) == ADF_OK);        /* b^e', exact, in the image */
            ADF_CHECK(adf_lball_powrat(y, a, e, n, seed, N) == ADF_OK);
            ADF_CHECK(!y->exact && y->N == N && y->v == 0);
            ADF_CHECK(adf_lball_contains(want, y));
            ADF_CHECK(adf_lball_pow_si(b, y, (slong) n1) == ADF_OK);    /* y^n' contains a^e' (exponent 200) */
            ADF_CHECK(adf_lball_pow_si(want, a, e1) == ADF_OK);
            ADF_CHECK(adf_lball_contains(want, b));
            ADF_CHECK(fmpz_fdiv_ui(fmpq_numref(y->u), P) == res);          /* the branch: seed^e' mod p */
            cases++;
        }
    }
    /* thousands of bits: b of 3000 bits, x = b^2 at 5 and at 2 (odd b, so the branch at 2 is b mod 4) */
    for (ulong p = 2; p <= 5; p += 3)
        for (int it = 0; it < 4; it++)
        {
            do fmpz_randbits(B, st, 3000); while ((fmpz_fdiv_ui(B, p) == 0));
            fmpz_abs(B, B);
            ulong seed = fmpz_fdiv_ui(B, p == 2 ? 4 : p);
            rat_lb(b, p, B, one);
            ADF_CHECK(adf_lball_pow_si(x, b, 2) == ADF_OK);
            ADF_CHECK(adf_lball_pow_si(want, b, 3) == ADF_OK);
            check_powrat(x, 3, 2, seed, 40, ADF_OK, want);
            ADF_CHECK(adf_lball_pow_si(want, b, -1) == ADF_OK);
            check_powrat(x, -2, 4, seed, 40, ADF_OK, want);
            /* x + 1: not a square of a rational (b^2 < b^2 + 1 < (b+1)^2); a ball at N = 40, y^2 contains x + 1 */
            fmpz_mul(r, B, B); fmpz_add_ui(r, r, p == 2 ? 8 : 5);
            rat_lb(a, p, r, one);
            if (adf_lball_powrat(y, a, 1, 2, seed, 40) == ADF_OK)
            {
                ADF_CHECK(!y->exact && y->N == 40);
                ADF_CHECK(adf_lball_pow_si(want, y, 2) == ADF_OK && adf_lball_contains(a, want));
                cases++;
            }
            else ADF_CHECK(0);
        }
    printf("  2^64-59 and 3000-bit inputs: calls %lu\n", cases);
    fmpz_clear(B); fmpz_clear(one); fmpz_clear(r);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(a); adf_lball_clear(b); adf_lball_clear(want);
    flint_randclear(st);
}

/* LIMIT: input exponents beyond 2^60 (first, before n = 0); a result valuation beyond the bound; the result p^K Z_p
   around 0 needs no power (an exact 7^(2^59) 2 to the 3/2 at N = 5); a result whose relative precision needs a
   power beyond the bit bound; K = N beyond the bound for an exact irrational branch. Outputs untouched.
   Fails on another status, a written output, or a LIMIT where the result is known without a power. */
ADF_TEST(powrat_limits)
{
    adf_lball_t x, want;
    cases = 0;
    adf_lball_init(x); adf_lball_init(want);
    x->p = 5; fmpq_set_si(x->u, 1, 1); x->v = EMAX + 1; x->N = 0; x->exact = 1;
    check_powrat(x, 1, 2, 1, 5, ADF_LIMIT, NULL);
    check_powrat(x, 1, 0, 1, 5, ADF_LIMIT, NULL);           /* the input bound is tested first */
    ball_si(x, 5, 1, 0, EMAX);
    x->N = EMAX + 1;
    check_powrat(x, 1, 2, 1, 5, ADF_LIMIT, NULL);
    /* exact 2^(2^60) at 2, 4/2 = 2/1: pow_si gives v = 2^61: LIMIT (as pow_si) */
    exact_si(x, 2, 1, 1, EMAX);
    check_powrat(x, 4, 2, 1, 5, ADF_LIMIT, NULL);
    /* 3/2: the root 2^(2^59) is exact, the power has v = 3 * 2^59 > 2^60: LIMIT */
    check_powrat(x, 3, 2, 1, 5, ADF_LIMIT, NULL);
    /* exact 7^(2^59) 2 at 7, 3/2: branch seed 3 irrational, v(result) = 3 * 2^58 <= 2^60; N = 5 <= e' j: p^5 Z_7 */
    exact_si(x, 7, 2, 1, EMAX / 2);
    ball_si(want, 7, 0, 0, 5);
    check_powrat(x, 3, 2, 3, 5, ADF_OK, want);
    ball_si(want, 7, 0, 0, -EMAX);
    check_powrat(x, 3, 2, 3, -EMAX, ADF_OK, want);
    check_powrat(x, 3, 2, 3, -EMAX - 1, ADF_LIMIT, NULL);  /* the result exponent beyond the bound */
    /* N = 2^60: K > e' j, relative precision 2^58 digits: a power beyond the bit bound */
    check_powrat(x, 3, 2, 3, EMAX, ADF_LIMIT, NULL);
    /* an exact irrational branch with N = LONG_MAX: K = N beyond the bound */
    exact_si(x, 7, 2, 1, 0);
    check_powrat(x, 1, 2, 3, LONG_MAX, ADF_LIMIT, NULL);
    /* e' j below -2^60: the exact 7^(2^60 - 4) 4 with -2/1... with -3/2: v = -3 (2^59 - 2) */
    exact_si(x, 7, 4, 1, EMAX - 4);
    check_powrat(x, -3, 2, 2, 5, ADF_LIMIT, NULL);         /* the exact rational root 2 7^(2^59-2): v of the cube */
    /* a ball of exponent 2^60 around 1 at 5: 1 + 5^(2^60) Z_5 to the 1/2: E' = 2^60, the centre is 1 (no power) */
    ball_si(x, 5, 1, 0, EMAX);
    ball_si(want, 5, 1, 0, EMAX);
    check_powrat(x, 1, 2, 1, BIG, ADF_OK, want);
    ball_si(want, 5, 1, 0, 30);
    check_powrat(x, 3, 2, 1, 30, ADF_OK, want);
    printf("  limits: calls %lu\n", cases);
    adf_lball_clear(x); adf_lball_clear(want);
}

/* ------------------------------------------------------------------------- 6. principal-unit powers: rows */

static void check_powunit(const adf_lball_t u, const adf_lball_t s, slong N, int st, const adf_lball_t want)
{
    adf_lball_t y, z;
    adf_lball_init(y); adf_lball_init(z);
    sentinel(y);
    int got = adf_lball_powunit(y, u, s, N);
    ADF_CHECK_MSG(got == st, "powunit p=%lu N=%ld: status %d, expected %d", u->p, N, got, st);
    if (st == ADF_OK && got == ADF_OK)
    {
        ADF_CHECK_MSG(adf_lball_identical(y, want), "powunit p=%lu u.N=%ld s.N=%ld N=%ld: wrong value (exact %d, "
                      "v %ld, N %ld)", u->p, u->N, s->N, N, y->exact, y->v, y->N);
        ADF_CHECK(adf_lball_is_canonical(y));
    }
    else if (st != ADF_OK)
        ADF_CHECK(is_sentinel(y));
    adf_lball_set(z, u);
    ADF_CHECK(adf_lball_powunit(z, z, s, N) == st && adf_lball_identical(z, st == ADF_OK ? want : u));
    adf_lball_set(z, s);
    ADF_CHECK(adf_lball_powunit(z, u, z, N) == st && adf_lball_identical(z, st == ADF_OK ? want : s));
    cases++;
    adf_lball_clear(y); adf_lball_clear(z);
}

/* Every row of powunit.jsonl. Fails on: a status, an exponent other than min(N, R), a centre other than the
   enumerated set's centre modulo p^min(N, R), an exact value where a ball is due or the converse, the 2-adic hull
   other than 1 + 2 Z_2, any difference under aliasing y = u or y = s. */
ADF_TEST(powunit_rows)
{
    jsonl_file *f = open_vectors("tests/ref/vectors/f-slice9/powunit.jsonl");
    adf_lball_t u, s, img, want;
    fmpz_t a, b;
    size_t exact = 0, images = 0, hulls = 0, xx = 0;
    if (!f) return;
    ADF_CHECK(jsonl_count(f) == 4352);
    cases = 0;
    adf_lball_init(u); adf_lball_init(s); adf_lball_init(img); adf_lball_init(want);
    fmpz_init(a); fmpz_init(b);
    for (size_t k = 0; k < jsonl_count(f); k++)
    {
        const jsonl_value *r = jsonl_record(f, k);
        ulong p = (ulong) num(r, "p");
        slong A = num(r, "A"), B = num(r, "B");
        fz(a, member(r, "un")); fz(b, member(r, "ud"));
        if (A < 0) rat_lb(u, p, a, b); else cball_si(u, p, fmpz_get_si(a), A);
        fz(a, member(r, "sn")); fz(b, member(r, "sd"));
        if (B < 0) rat_lb(s, p, a, b); else cball_si(s, p, fmpz_get_si(a), B);
        ADF_CHECK(num(r, "st") == 0);
        if (num(r, "exact"))
        {
            fz(a, member(r, "rnum")); fz(b, member(r, "rden"));
            rat_lb(want, p, a, b);
            exact++;
            for (slong N = -3; N <= 30; N += 11) check_powunit(u, s, N, ADF_OK, want);
            continue;
        }
        if (num(r, "R") < 0)
        {
            slong N = num(r, "N");
            cball_si(img, p, num(r, "c"), N);
            xx++;
            for (slong t = 0; t < 4; t++)
            {
                const slong Ns[4] = { N, N - 1, 1, -2 };
                coarse(want, img, Ns[t]);
                check_powunit(u, s, Ns[t], ADF_OK, want);
            }
            continue;
        }
        slong R = num(r, "R");
        cball_si(img, p, num(r, "c"), R);
        if (has(r, "hull")) { hulls++; ADF_CHECK(R == 1 && p == 2); } else images++;
        const slong Ns[7] = { BIG, R, R - 1, R - 2, 1, 0, -2 };
        for (int t = 0; t < 7; t++)
        {
            coarse(want, img, Ns[t] < R ? Ns[t] : R);
            check_powunit(u, s, Ns[t], ADF_OK, want);
        }
    }
    printf("  powunit rows %zu: exact %zu, exact-input balls %zu, images %zu, 2-adic hulls %zu; calls %lu\n",
           jsonl_count(f), exact, xx, images, hulls, cases);
    fmpz_clear(a); fmpz_clear(b);
    adf_lball_clear(u); adf_lball_clear(s); adf_lball_clear(img); adf_lball_clear(want);
    jsonl_close(f);
}

/* ---------------------------------------------------------------- 7. principal-unit powers: statuses, limits */

/* Domains (P4): u outside 1 + p Z_p (DOMAIN), a ball of u that meets it and its complement (NOT_DETERMINED); s
   outside Z_p (DOMAIN), a ball O(p^-1) (NOT_DETERMINED); DOMAIN wins over NOT_DETERMINED; different primes DOMAIN
   first; input LIMIT before the domain; result LIMIT; no power where the centre is 1. Fails on another status or a
   written output. */
ADF_TEST(powunit_statuses_and_limits)
{
    adf_lball_t u, s, want, y;
    cases = 0;
    adf_lball_init(u); adf_lball_init(s); adf_lball_init(want); adf_lball_init(y);
    exact_si(s, 5, 1, 2, 0);                                /* s = 1/2, in Z_5 */
    exact_si(u, 5, 2, 1, 0);  check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* 2 is not 1 mod 5 */
    exact_si(u, 5, 0, 1, 0);  check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* the exact 0 */
    exact_si(u, 5, 1, 1, 1);  check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* 5 */
    exact_si(u, 5, 6, 1, -1); check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* 6/5 */
    ball_si(u, 5, 2, 0, 1);   check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* 2 + 5 Z_5 */
    ball_si(u, 5, 0, 0, 0);   check_powunit(u, s, 9, ADF_NOT_DETERMINED, NULL);  /* Z_5 meets 1 + 5 Z_5 */
    ball_si(u, 5, 0, 0, 1);   check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* 5 Z_5 */
    ball_si(u, 5, 1, -1, 0);  check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* 1/5 + Z_5 */
    ball_si(u, 5, 0, 0, -3);  check_powunit(u, s, 9, ADF_NOT_DETERMINED, NULL);
    exact_si(u, 5, 6, 1, 0);
    exact_si(s, 5, 1, 1, -1); check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* s = 1/5 */
    ball_si(s, 5, 0, 0, -1);  check_powunit(u, s, 9, ADF_NOT_DETERMINED, NULL);  /* O(5^-1) */
    ball_si(s, 5, 2, -1, 0);  check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* 2/5 + Z_5 */
    ball_si(s, 5, 0, 0, 0);
    exact_si(want, 5, 1, 1, 0);
    ADF_CHECK(adf_lball_powunit(y, u, s, 9) == ADF_OK && !y->exact && y->N == 1); /* 6^Z_5 = 1 + 5 Z_5 */
    /* DOMAIN wins: u outside with s undetermined, and u undetermined with s outside */
    ball_si(s, 5, 0, 0, -1); exact_si(u, 5, 2, 1, 0); check_powunit(u, s, 9, ADF_DOMAIN, NULL);
    ball_si(u, 5, 0, 0, 0);  exact_si(s, 5, 1, 1, -1); check_powunit(u, s, 9, ADF_DOMAIN, NULL);
    /* at 2: every odd unit is in the domain (3, -1, 1/3); an even one is not; 1 + Z_2 straddles */
    exact_si(s, 2, 1, 3, 0);
    exact_si(u, 2, 1, 1, 1);  check_powunit(u, s, 9, ADF_DOMAIN, NULL);          /* 2 */
    ball_si(u, 2, 0, 0, 0);   check_powunit(u, s, 9, ADF_NOT_DETERMINED, NULL);  /* Z_2 */
    /* different primes, before the input bound */
    exact_si(u, 3, 4, 1, 0); exact_si(s, 5, 1, 1, 0);
    check_powunit(u, s, 9, ADF_DOMAIN, NULL);
    u->v = EMAX + 1; check_powunit(u, s, 9, ADF_DOMAIN, NULL);
    /* input LIMIT before the domain */
    exact_si(u, 5, 2, 1, 0); u->v = EMAX + 1; exact_si(s, 5, 1, 1, 0);
    check_powunit(u, s, 9, ADF_LIMIT, NULL);
    exact_si(u, 5, 6, 1, 0); ball_si(s, 5, 1, 0, 1); s->N = EMAX + 1;
    check_powunit(u, s, 9, ADF_LIMIT, NULL);
    /* result LIMIT: N beyond the bound with exact inputs; N below -2^60 */
    exact_si(s, 5, 1, 2, 0);
    check_powunit(u, s, EMAX + 1, ADF_LIMIT, NULL);
    check_powunit(u, s, -EMAX - 1, ADF_LIMIT, NULL);
    ball_si(want, 5, 0, 0, -EMAX);
    check_powunit(u, s, -EMAX, ADF_OK, want);
    /* no power for the centre 1: (1 + 5^(2^60) Z_5)^(1/2) = 1 + 5^(2^60) Z_5 (R = A + beta = 2^60) */
    ball_si(u, 5, 1, 0, EMAX);
    ball_si(want, 5, 1, 0, EMAX);
    check_powunit(u, s, LONG_MAX, ADF_OK, want);
    /* a working power beyond the bit bound: (6 + 5^(2^40) Z_5)^(1/2) at N = BIG needs the centre modulo 5^(2^40) */
    ball_si(u, 5, 6, 0, (slong) 1 << 40);
    check_powunit(u, s, BIG, ADF_LIMIT, NULL);
    /* the principal square root of 6 modulo 25: (1 + 5a)^2 = 6 gives 10a = 5, a = 3 mod 5: 16; R = 2^40, K = N = 2 */
    ball_si(want, 5, 16, 0, 2);
    check_powunit(u, s, 2, ADF_OK, want);
    /* the exact 1 needs no domain of the other factor beyond its own: 1^s, u^0 */
    exact_si(u, 5, 1, 1, 0); ball_si(s, 5, 3, 0, 2); exact_si(want, 5, 1, 1, 0);
    check_powunit(u, s, 7, ADF_OK, want);
    ball_si(u, 5, 11, 0, 3); exact_si(s, 5, 0, 1, 0);
    check_powunit(u, s, 7, ADF_OK, want);
    /* u and s the same object (and y = u = s): read as two independent sets. u = s = 1 + 7 Z_7: u0 = 1 (alpha = INF),
       s0 = 1 (beta = 0), A = B = 1: R = min(1 + 0, INF, 2) = 1, the ball 1 + 7 Z_7 (Proposition 18) */
    ball_si(u, 7, 1, 0, 1);
    ball_si(want, 7, 1, 0, 1);
    sentinel(y);
    ADF_CHECK(adf_lball_powunit(y, u, u, 9) == ADF_OK && adf_lball_identical(y, want));
    adf_lball_set(s, u);
    ADF_CHECK(adf_lball_powunit(s, s, s, 9) == ADF_OK && adf_lball_identical(s, want));
    /* u = s = 1 + 3^2 Z_3 ... u0 = 1, s0 = 1, A = B = 2: R = min(2, INF, 4) = 2 */
    ball_si(u, 3, 1, 0, 2); ball_si(want, 3, 1, 0, 2);
    ADF_CHECK(adf_lball_powunit(u, u, u, 9) == ADF_OK && adf_lball_identical(u, want));
    cases += 3;
    printf("  statuses and limits: calls %lu\n", cases);
    adf_lball_clear(u); adf_lball_clear(s); adf_lball_clear(want); adf_lball_clear(y);
}

/* ----------------------------------------------------------------------- 8. identities, large primes, bits */

/* With exact inputs at N = 30 (output precision 30, absolute): u^(s+t) = u^s u^t and (uv)^s = u^s v^s as equal SETS
   (each side a ball at 30); u^k contains the exact pow_si(u, k) for integers k = -5..5; u^(1/n) equals the principal
   root powrat(u, 1, n, 1) at odd p (seed 1 = the principal branch) and the branch of sign +1 at 2. p = 2, 3, 5, 7,
   13, 2^64 - 59 (N = 200 there); u and s of 3000 bits at 3. Fails on a difference at the stated precision. */
ADF_TEST(powunit_identities)
{
    const ulong primes[6] = { 2, 3, 5, 7, 13, UWORD_MAX - 58 };
    flint_rand_t st;
    adf_lball_t u, v, s, t, st1, a, b, c, d, uv;
    fmpz_t A, B;
    cases = 0;
    flint_randinit(st);
    adf_lball_init(u); adf_lball_init(v); adf_lball_init(s); adf_lball_init(t); adf_lball_init(st1);
    adf_lball_init(a); adf_lball_init(b); adf_lball_init(c); adf_lball_init(d); adf_lball_init(uv);
    fmpz_init(A); fmpz_init(B);
    for (int ip = 0; ip < 6; ip++)
    {
        ulong p = primes[ip];
        slong N = ip == 5 ? 200 : 30, q = p == 2 ? 4 : (slong) (p < 100 ? p : 1);
        for (int it = 0; it < 40; it++)
        {
            /* u = 1 + q k / l, v likewise (l prime to p): principal units (at 2: 1 mod 4) */
            slong k1 = 1 + it, l1 = 2 * it + 1, k2 = 3 + 2 * it, l2 = it + 1;
            while (l1 % (slong) (p < 100 ? p : 1) == 0 && p < 100) l1++;
            while (l2 % (slong) (p < 100 ? p : 1) == 0 && p < 100) l2++;
            if (p == 2) { l1 |= 1; l2 |= 1; }
            if (p > 100)
            {
                fmpz_set_ui(A, p); fmpz_mul_si(A, A, k1); fmpz_add_si(A, A, l1); fmpz_set_si(B, l1);
                rat_lb(u, p, A, B);
                fmpz_set_ui(A, p); fmpz_mul_si(A, A, k2); fmpz_add_si(A, A, l2); fmpz_set_si(B, l2);
                rat_lb(v, p, A, B);
            }
            else { rat_lb_si(u, p, l1 + q * k1, l1); rat_lb_si(v, p, l2 + q * k2, l2); }
            slong sn = 2 * it - 37, sd = p == 3 ? 2 : 3, tn = it + 5, td = p == 5 || p == 7 ? 2 : 1;
            rat_lb_si(s, p, sn, sd);
            rat_lb_si(t, p, tn, td);
            ADF_CHECK(adf_lball_add(st1, s, t) == ADF_OK);
            ADF_CHECK(adf_lball_powunit(a, u, st1, N) == ADF_OK);
            ADF_CHECK(adf_lball_powunit(b, u, s, N) == ADF_OK);
            ADF_CHECK(adf_lball_powunit(c, u, t, N) == ADF_OK);
            ADF_CHECK(adf_lball_mul(d, b, c) == ADF_OK);
            ADF_CHECK_MSG(adf_lball_equal_set(a, d), "u^(s+t) != u^s u^t at p=%lu it=%d", p, it);
            ADF_CHECK(adf_lball_mul(uv, u, v) == ADF_OK);
            ADF_CHECK(adf_lball_powunit(a, uv, s, N) == ADF_OK);
            ADF_CHECK(adf_lball_powunit(c, v, s, N) == ADF_OK);
            ADF_CHECK(adf_lball_mul(d, b, c) == ADF_OK);
            ADF_CHECK_MSG(adf_lball_equal_set(a, d), "(uv)^s != u^s v^s at p=%lu it=%d", p, it);
            for (slong kk = -5; kk <= 5; kk++)
            {
                rat_lb_si(t, p, kk, 1);
                ADF_CHECK(adf_lball_powunit(a, u, t, N) == ADF_OK);
                ADF_CHECK(adf_lball_pow_si(b, u, kk) == ADF_OK);
                ADF_CHECK_MSG(adf_lball_contains(b, a), "u^k at p=%lu k=%ld", p, kk);
            }
            for (ulong n = 2; n <= 4; n++)
            {
                /* u^n has the principal n-th root u (Proposition 13: one principal-unit root); powrat finds it
                   as the rational branch of seed 1 (exact u), powunit as exp(log(u^n)/n), a ball at N */
                if ((p != 2 && n % p == 0) || (p == 2 && n % 2 == 0)) continue;      /* 1/n not in Z_p */
                ADF_CHECK(adf_lball_pow_si(a, u, (slong) n) == ADF_OK);
                rat_lb_si(t, p, 1, (slong) n);
                ADF_CHECK(adf_lball_powunit(b, a, t, N) == ADF_OK);
                ADF_CHECK(adf_lball_powrat(c, a, 1, n, 1, N) == ADF_OK);
                ADF_CHECK(c->exact && adf_lball_equal_set(c, u));
                ADF_CHECK_MSG(adf_lball_contains(u, b) && !b->exact && b->N == N,
                              "u^(1/n) is not the principal root at p=%lu n=%lu", p, n);
            }
            cases += 4;
        }
    }
    /* 3000 bits at 3: u = 1 + 3 X / Y, s = Z / W with X, Y, Z, W of 3000 bits (Y, W prime to 3), N = 40:
       u^s u^(1 - s) contains u (output precision 40) */
    for (int it = 0; it < 4; it++)
    {
        fmpz_t X, Y;
        fmpz_init(X); fmpz_init(Y);
        fmpz_randbits(X, st, 3000);
        do fmpz_randbits(Y, st, 3000); while ((fmpz_fdiv_ui(Y, 3) == 0) || fmpz_is_zero(Y));
        fmpz_mul_ui(A, X, 3); fmpz_add(A, A, Y);
        rat_lb(u, 3, A, Y);
        do fmpz_randbits(Y, st, 3000); while ((fmpz_fdiv_ui(Y, 3) == 0) || fmpz_is_zero(Y));
        fmpz_randbits(X, st, 3000);
        rat_lb(s, 3, X, Y);
        fmpz_sub(X, Y, X);
        rat_lb(t, 3, X, Y);                                   /* 1 - s */
        ADF_CHECK(adf_lball_powunit(a, u, s, 40) == ADF_OK);
        ADF_CHECK(adf_lball_powunit(b, u, t, 40) == ADF_OK);
        ADF_CHECK(adf_lball_mul(c, a, b) == ADF_OK && adf_lball_contains(u, c) && c->N == 40);
        cases++;
        fmpz_clear(X); fmpz_clear(Y);
    }
    printf("  identities: groups %lu\n", cases);
    fmpz_clear(A); fmpz_clear(B);
    adf_lball_clear(u); adf_lball_clear(v); adf_lball_clear(s); adf_lball_clear(t); adf_lball_clear(st1);
    adf_lball_clear(a); adf_lball_clear(b); adf_lball_clear(c); adf_lball_clear(d); adf_lball_clear(uv);
    flint_randclear(st);
}

/* ------------------------------------------------------------- 9. an exact integer exponent: pow_si (P9) */

/* P9 of api-1f6.md (lane f-repair5): for a ball u in the domain and the exact integer s = k, the value of powunit
   is the ball of exponent min(N, R) that contains the image pow_si(u, k), R its exponent; k = 0 gives the exact 1.
   1. Review f-review7, finding 2 (lanes/f-review7/limits.in lines 2 to 4): (6 + 5^(2^40) Z_5)^1 and ^2 at N = 2^40
      were LIMIT (a Log modulo 5^(2^40)); the image is 6 + 5^(2^40) Z_5 and 36 + 5^(2^40) Z_5 (L12: rel' = rel +
      v_5(k)), which pow_si returns. Also N = 2^40 - 1 (the ball at N that contains the image, centre 6, no power),
      and at 2 the sign factor: (3 + 2^(2^30) Z_2)^3 = 27 + 2^(2^30) Z_2, ^2 = 9 + 2^(2^30 + 1) Z_2 (v_2(2) = 1).
      k = -1 at 5 stays LIMIT: the centre 1/6 modulo 5^(2^40) needs the power, in pow_si and in the old path.
   2. The grid: p = 2, 3, 5, 7 and 65537, every centre of 1 + p Z_p (every odd centre at 2) modulo p^A, A = 1 to 4
      (fewer at 7, 65537), k = -7..7, N = -2, 0, 1, 2, A - 1, A, A + 1, A + 3, BIG: the result must be identical
      (all fields, with every aliasing of check_powunit) to the coarse ball of pow_si. The old code (one Log, one
      product, one exp, Proposition 18) met these rows before the route of P9 existed (red-green log), so the route
      changes no result that was OK. Fails on a status or a field. */
ADF_TEST(powunit_integer_exponent_is_pow_si)
{
    const ulong ps[5] = { 2, 3, 5, 7, 65537 };
    const slong Amax[5] = { 4, 4, 4, 3, 2 };
    const slong E40 = (slong) 1 << 40, E30 = (slong) 1 << 30;
    adf_lball_t u, s, want, img;
    cases = 0;
    adf_lball_init(u); adf_lball_init(s); adf_lball_init(want); adf_lball_init(img);
    /* 1. the inputs of finding 2 */
    ball_si(u, 5, 6, 0, E40);
    exact_si(s, 5, 1, 1, 0);
    ball_si(want, 5, 6, 0, E40);
    check_powunit(u, s, E40, ADF_OK, want);
    check_powunit(u, s, BIG, ADF_OK, want);
    ball_si(want, 5, 6, 0, E40 - 1);
    check_powunit(u, s, E40 - 1, ADF_OK, want);
    exact_si(s, 5, 2, 1, 0);
    ball_si(want, 5, 36, 0, E40);
    check_powunit(u, s, E40, ADF_OK, want);
    ADF_CHECK(adf_lball_pow_si(img, u, 2) == ADF_OK && adf_lball_identical(img, want));
    exact_si(s, 5, -1, 1, 0);
    check_powunit(u, s, E40, ADF_LIMIT, NULL);
    ball_si(u, 2, 3, 0, E30);
    exact_si(s, 2, 3, 1, 0);
    ball_si(want, 2, 27, 0, E30);
    check_powunit(u, s, BIG, ADF_OK, want);
    exact_si(s, 2, 1, 1, 1);
    ball_si(want, 2, 9, 0, E30 + 1);
    check_powunit(u, s, BIG, ADF_OK, want);
    ball_si(want, 2, 9, 0, E30);
    check_powunit(u, s, E30, ADF_OK, want);
    /* 2. the grid */
    for (int ip = 0; ip < 5; ip++)
    {
        ulong p = ps[ip];
        for (slong A = 1; A <= Amax[ip]; A++)
        {
            fmpz_t PA, c;
            fmpz_init(PA); fmpz_init(c);
            fmpz_set_ui(PA, p); fmpz_pow_ui(PA, PA, (ulong) A);
            /* centres c = 1 mod p (odd p) or odd (p = 2), 0 < c < p^A; at 65537 the first 40 */
            for (fmpz_set_ui(c, p == 2 ? 1 : 1); fmpz_cmp(c, PA) < 0; fmpz_add_ui(c, c, p == 2 ? 2 : p))
            {
                if (p == 65537 && fmpz_cmp_ui(c, 40 * p) > 0) break;
                ball_fz(u, p, c, 0, A);
                for (slong k = -7; k <= 7; k++)
                {
                    const slong Ns[9] = { -2, 0, 1, 2, A - 1, A, A + 1, A + 3, BIG };
                    rat_lb_si(s, p, k, 1);
                    if (k != 0) ADF_CHECK(adf_lball_pow_si(img, u, k) == ADF_OK);
                    for (int t = 0; t < 9; t++)
                    {
                        if (k == 0) exact_si(want, p, 1, 1, 0);
                        else coarse(want, img, Ns[t] < img->N ? Ns[t] : img->N);
                        check_powunit(u, s, Ns[t], ADF_OK, want);
                    }
                }
            }
            fmpz_clear(PA); fmpz_clear(c);
        }
    }
    printf("  integer exponents against pow_si: calls %lu\n", cases);
    adf_lball_clear(u); adf_lball_clear(s); adf_lball_clear(want); adf_lball_clear(img);
}
