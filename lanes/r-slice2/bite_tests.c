#define ADF_TEST_NO_MAIN
/* tests/test_roots_real_isolate.c: the real roots by exact isolation (lane r-slice1, issue adf-8di):
   adf_roots_real of include/adelefeld/roots.h with the candidates of src/roots_real.c, and that hidden
   function itself.

   The statements tested are those of docs/design/real-roots.md: the isolation by Descartes' rule of signs
   (Algorithm D, Propositions R2 and R3), the refinement (Algorithm F, Proposition R4: one root kept, the
   accuracy of decision S-D19, balls nested when prec grows, a dyadic root with a short odd part returned as
   an exact ball) and the whole (Proposition R5: the list passes the test of solvers P3.8, both verifiers and
   the count). The cost: the family of docs/reviews/s2/review-real.md finding 1 (the roots 2^e and 2^e + 1),
   which took 97 s at e = 1500 with the old candidates, has a bound on the time here.

   The oracle is written here and calls neither the library nor FLINT's count of real roots: the normalised
   polynomial over Q (fmpq_poly), a Sturm chain over Q with its sign variations at rational points and at the
   two infinities (the same oracle as tests/test_roots_real.c; Sturm's theorem is source pending there too),
   and exact signs at rational points by fmpz_poly_evaluate_fmpq. For planted roots the oracle is also the
   list of the planted rationals. The exact end points of a ball are read with arb_get_interval_fmpz_2exp
   (flint-3.0.1 arb.rst:461 to 466).

   Times are wall times (CLOCK_MONOTONIC). The bounds are 10 to 100 times the measured times of the new
   code (lanes/r-slice1/result.md), so that a build with the sanitizers on a shared machine passes, and far
   below the times of the old candidates (lanes/r-slice1/runs/bench_before.txt). */

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <string.h>
#include <time.h>

#include <flint/fmpq_poly.h>
#include <flint/fmpz_vec.h>

#include <adelefeld.h>

#include "test_runner.h"

/* hidden in src/roots_real.c (not declared by a public header, not exported by the shared object) */
int adf_roots_real_isolate(arb_ptr cand, slong * m, const fmpz_poly_t g, slong prec);

static double
now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double) t.tv_sec + 1e-9 * (double) t.tv_nsec;
}

/* ---- polynomials ---- */

static void
poly_set_si(fmpz_poly_t f, const slong * c, slong len)
{
    slong i;

    fmpz_poly_zero(f);
    for (i = 0; i < len; i++)
        fmpz_poly_set_coeff_si(f, i, c[i]);
}

/* f = f * (den X - num)^mult */
static void
poly_mul_root(fmpz_poly_t f, const fmpq_t r, slong mult)
{
    fmpz_poly_t t;
    fmpz_t c;
    slong i;

    fmpz_poly_init(t);
    fmpz_init(c);
    fmpz_neg(c, fmpq_numref(r));
    fmpz_poly_set_coeff_fmpz(t, 1, fmpq_denref(r));
    fmpz_poly_set_coeff_fmpz(t, 0, c);
    for (i = 0; i < mult; i++)
        fmpz_poly_mul(f, f, t);
    fmpz_clear(c);
    fmpz_poly_clear(t);
}

/* f = f * (a X - b) */
static void
poly_mul_lin(fmpz_poly_t f, const fmpz_t a, const fmpz_t b)
{
    fmpz_poly_t t;

    fmpz_poly_init(t);
    fmpz_poly_set_coeff_fmpz(t, 1, a);
    fmpz_poly_set_coeff_fmpz(t, 0, b);
    fmpz_neg(t->coeffs, t->coeffs);
    fmpz_poly_mul(f, f, t);
    fmpz_poly_clear(t);
}

/* r = m 2^e as a rational (e of any sign, |e| small enough for a word) */
static void
q_set_2exp_si(fmpq_t r, slong m, slong e)
{
    fmpz_set_si(fmpq_numref(r), m);
    fmpz_one(fmpq_denref(r));
    if (e >= 0)
        fmpz_mul_2exp(fmpq_numref(r), fmpq_numref(r), (ulong) e);
    else
        fmpz_mul_2exp(fmpq_denref(r), fmpq_denref(r), (ulong) -e);
    fmpq_canonicalise(r);
}

/* the normalised polynomial by the oracle: f / gcd(f, f') over Q, primitive, positive leading coefficient;
   1 for f constant (solvers L3.1(3)) */
static void
oracle_normalise(fmpz_poly_t g, const fmpz_poly_t f)
{
    fmpq_poly_t F, D, H, Q;

    if (fmpz_poly_degree(f) <= 0)
    {
        fmpz_poly_one(g);
        return;
    }
    fmpq_poly_init(F);
    fmpq_poly_init(D);
    fmpq_poly_init(H);
    fmpq_poly_init(Q);
    fmpq_poly_set_fmpz_poly(F, f);
    fmpq_poly_derivative(D, F);
    fmpq_poly_gcd(H, F, D);
    fmpq_poly_div(Q, F, H);
    fmpq_poly_get_numerator(g, Q);
    fmpz_poly_primitive_part(g, g);
    if (fmpz_sgn(g->coeffs + fmpz_poly_degree(g)) < 0)
        fmpz_poly_neg(g, g);
    fmpq_poly_clear(F);
    fmpq_poly_clear(D);
    fmpq_poly_clear(H);
    fmpq_poly_clear(Q);
}

/* ---- the oracle: a Sturm chain over Q (as tests/test_roots_real.c) ---- */

typedef struct
{
    fmpq_poly_struct * p;
    slong len, alloc;
    int is_const;
} chain_t;

static void
chain_init(chain_t * c, const fmpz_poly_t g)
{
    slong d = fmpz_poly_degree(g), i;
    fmpq_poly_t r;

    c->is_const = d <= 0;
    c->len = 0;
    c->alloc = 0;
    c->p = NULL;
    if (c->is_const)
        return;
    c->alloc = d + 1;
    c->p = flint_malloc(c->alloc * sizeof(fmpq_poly_struct));
    for (i = 0; i < c->alloc; i++)
        fmpq_poly_init(c->p + i);
    fmpq_poly_set_fmpz_poly(c->p + 0, g);
    fmpq_poly_derivative(c->p + 1, c->p + 0);
    c->len = 2;
    fmpq_poly_init(r);
    while (c->len <= d)
    {
        fmpq_poly_rem(r, c->p + c->len - 2, c->p + c->len - 1);
        if (fmpq_poly_is_zero(r))
            break;
        fmpq_poly_neg(c->p + c->len, r);
        c->len++;
    }
    fmpq_poly_clear(r);
}

static void
chain_clear(chain_t * c)
{
    slong i;

    for (i = 0; i < c->alloc; i++)
        fmpq_poly_clear(c->p + i);
    if (c->p != NULL)
        flint_free(c->p);
}

/* the sign changes of the chain at x (inf = 0), at -infinity (inf = -1) or +infinity (inf = 1) */
static slong
variations(const chain_t * c, const fmpq_t x, int inf)
{
    fmpq_t v;
    slong i, n = 0;
    int s, last = 0;

    fmpq_init(v);
    for (i = 0; i < c->len; i++)
    {
        const fmpq_poly_struct * q = c->p + i;
        slong d = fmpq_poly_degree(q);
        if (inf == 0)
        {
            fmpq_poly_evaluate_fmpq(v, q, x);
            s = fmpq_sgn(v);
        }
        else
        {
            s = fmpz_sgn(q->coeffs + d);
            if (inf < 0 && d % 2 == 1)
                s = -s;
        }
        if (s == 0)
            continue;
        if (last != 0 && s != last)
            n++;
        last = s;
    }
    fmpq_clear(v);
    return n;
}

static int
sign_at(const fmpz_poly_t g, const fmpq_t x)
{
    fmpq_t v;
    int s;

    fmpq_init(v);
    fmpz_poly_evaluate_fmpq(v, g, x);
    s = fmpq_sgn(v);
    fmpq_clear(v);
    return s;
}

/* the number of distinct roots of the squarefree g in (lo, hi]; lo_inf = -1 for -infinity, hi_inf = 1 for
   +infinity */
static slong
roots_half_open(const chain_t * c, const fmpq_t lo, int lo_inf, const fmpq_t hi, int hi_inf)
{
    if (c->is_const)
        return 0;
    return variations(c, lo, lo_inf) - variations(c, hi, hi_inf);
}

/* the number of roots in the open interval (lo, hi) */
static slong
roots_open(const chain_t * c, const fmpz_poly_t g, const fmpq_t lo, int lo_inf, const fmpq_t hi, int hi_inf)
{
    slong k = roots_half_open(c, lo, lo_inf, hi, hi_inf);

    if (!c->is_const && hi_inf == 0 && sign_at(g, hi) == 0)
        k--;
    return k;
}

/* the number of roots in the closed interval [lo, hi] */
static slong
roots_closed(const chain_t * c, const fmpz_poly_t g, const fmpq_t lo, const fmpq_t hi)
{
    slong k = roots_half_open(c, lo, 0, hi, 0);

    if (!c->is_const && sign_at(g, lo) == 0)
        k++;
    return k;
}

/* ---- balls ---- */

static void
q_set_fmpz_2exp(fmpq_t r, const fmpz_t m, const fmpz_t e)
{
    fmpz_set(fmpq_numref(r), m);
    fmpz_one(fmpq_denref(r));
    if (fmpz_sgn(e) >= 0)
        fmpz_mul_2exp(fmpq_numref(r), fmpq_numref(r), fmpz_get_ui(e));
    else
    {
        fmpz_t k;
        fmpz_init(k);
        fmpz_neg(k, e);
        fmpz_mul_2exp(fmpq_denref(r), fmpq_denref(r), fmpz_get_ui(k));
        fmpz_clear(k);
    }
    fmpq_canonicalise(r);
}

/* the exact end points of the finite ball x */
static void
ends_q(fmpq_t lo, fmpq_t hi, const arb_t x)
{
    fmpz_t a, b, e;

    fmpz_init(a);
    fmpz_init(b);
    fmpz_init(e);
    arb_get_interval_fmpz_2exp(a, b, e, x);
    q_set_fmpz_2exp(lo, a, e);
    q_set_fmpz_2exp(hi, b, e);
    fmpz_clear(a);
    fmpz_clear(b);
    fmpz_clear(e);
}

/* 1 if the n balls b are the true complete list of the real roots of the squarefree g at the precision prec:
   n = the Sturm count; each ball finite, an exact root or lo < hi with g(lo) g(hi) < 0; exactly one root in
   each closed ball; hi_i < lo_(i+1); no root below the first, in a gap or above the last; accuracy at least
   max(prec, 2) or exact. Failures are reported with ADF_CHECK_MSG and the label what. */
static int
balls_true(const fmpz_poly_t g, arb_srcptr b, slong n, slong prec, const char * what)
{
    chain_t c;
    fmpq_t lo, hi, phi, z;
    slong i, total, need = prec < 2 ? 2 : prec;
    int ok = 1, sl, sh;

#define BCHECK(cond, ...)                                                                    \
    do                                                                                       \
    {                                                                                        \
        int ok_ = (cond);                                                                    \
        ADF_CHECK_MSG(ok_, __VA_ARGS__);                                                     \
        ok = ok && ok_;                                                                      \
    } while (0)

    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_init(phi);
    fmpq_init(z);
    chain_init(&c, g);
    total = c.is_const ? 0 : roots_half_open(&c, z, -1, z, 1);
    BCHECK(n == total, "%s: %ld balls, Sturm count %ld", what, (long) n, (long) total);
    for (i = 0; i < n && ok; i++)
    {
        BCHECK(arb_is_finite(b + i), "%s: ball %ld not finite", what, (long) i);
        if (!ok)
            break;
        ends_q(lo, hi, b + i);
        if (fmpq_equal(lo, hi))
            BCHECK(sign_at(g, lo) == 0, "%s: exact ball %ld is not a root", what, (long) i);
        else
        {
            sl = sign_at(g, lo);
            sh = sign_at(g, hi);
            BCHECK(fmpq_cmp(lo, hi) < 0 && sl * sh < 0, "%s: ball %ld: signs %d, %d", what, (long) i, sl, sh);
        }
        BCHECK(roots_closed(&c, g, lo, hi) == 1, "%s: ball %ld holds %ld roots", what, (long) i,
               (long) roots_closed(&c, g, lo, hi));
        BCHECK(arb_is_exact(b + i) || arb_rel_accuracy_bits(b + i) >= need, "%s: ball %ld: accuracy %ld < %ld",
               what, (long) i, (long) arb_rel_accuracy_bits(b + i), (long) need);
        if (i == 0)
            BCHECK(roots_open(&c, g, z, -1, lo, 0) == 0, "%s: a root below the first ball", what);
        else
        {
            BCHECK(fmpq_cmp(phi, lo) < 0, "%s: balls %ld and %ld not disjoint and increasing", what,
                   (long) i - 1, (long) i);
            BCHECK(roots_open(&c, g, phi, 0, lo, 0) == 0, "%s: a root in the gap before ball %ld", what,
                   (long) i);
        }
        if (i == n - 1)
            BCHECK(roots_open(&c, g, hi, 0, z, 1) == 0, "%s: a root above the last ball", what);
        fmpq_swap(phi, hi);
    }
    chain_clear(&c);
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpq_clear(phi);
    fmpq_clear(z);
    return ok;
}

/* 1 if L is the true list of adf_roots_real(L, f, prec): g and reduced as the oracle says, the shape of the
   real place, n = count, the balls true (balls_true), and the predicate and both verifiers accept */
static int
list_true(const adf_rootlist_t L, const fmpz_poly_t f, slong prec, const char * what)
{
    fmpz_poly_t g;
    int ok = 1;

    fmpz_poly_init(g);
    oracle_normalise(g, f);
    BCHECK(fmpz_poly_equal(L->g, g), "%s: g is not the normalised polynomial", what);
    BCHECK(L->reduced == (fmpz_poly_degree(f) > 0 && fmpz_poly_degree(g) < fmpz_poly_degree(f)),
           "%s: reduced %d", what, L->reduced);
    BCHECK(adf_place_is_archimedean(L->place) && L->scope == ADF_ROOTLIST_PARTITION && L->complete == 1 &&
           L->nu == 0 && L->count == L->n, "%s: shape", what);
    BCHECK(L->a == NULL && L->K == NULL && L->s == NULL && L->ua == NULL && L->ue == NULL &&
           (L->ball == NULL) == (L->n == 0), "%s: pointers", what);
    ok = balls_true(g, L->ball, L->n, prec, what) && ok;
    BCHECK(adf_rootlist_is_canonical(L) == 1, "%s: not canonical", what);
    BCHECK(adf_rootlist_verify_entries(L, f) == 1, "%s: refused by verify_entries", what);
    BCHECK(adf_rootlist_verify_complete(L, f, 0) == 1, "%s: refused by verify_complete", what);
    fmpz_poly_clear(g);
    return ok;
}

/* each planted root in exactly one ball of L, each ball of L holds exactly one planted root; returns the number
   of balls that are exact */
static slong
planted_match(const adf_rootlist_t L, const fmpq * r, slong k, const char * what)
{
    fmpq_t lo, hi;
    slong i, j, m, exact = 0;

    fmpq_init(lo);
    fmpq_init(hi);
    ADF_CHECK_MSG(L->n == k, "%s: %ld balls for %ld planted roots", what, (long) L->n, (long) k);
    for (i = 0; i < k; i++)
    {
        for (j = 0, m = 0; j < L->n; j++)
        {
            ends_q(lo, hi, L->ball + j);
            m += fmpq_cmp(lo, r + i) <= 0 && fmpq_cmp(r + i, hi) <= 0;
        }
        ADF_CHECK_MSG(m == 1, "%s: planted root %ld in %ld balls", what, (long) i, (long) m);
    }
    for (j = 0; j < L->n; j++)
    {
        ends_q(lo, hi, L->ball + j);
        for (i = 0, m = 0; i < k; i++)
            m += fmpq_cmp(lo, r + i) <= 0 && fmpq_cmp(r + i, hi) <= 0;
        ADF_CHECK_MSG(m == 1, "%s: ball %ld holds %ld planted roots", what, (long) j, (long) m);
        exact += arb_is_exact(L->ball + j);
    }
    fmpq_clear(lo);
    fmpq_clear(hi);
    return exact;
}

/* 1 if r is one of the exact balls of L */
static int
exact_ball_at(const adf_rootlist_t L, const fmpq_t r)
{
    fmpq_t lo, hi;
    slong j;
    int found = 0;

    fmpq_init(lo);
    fmpq_init(hi);
    for (j = 0; j < L->n && !found; j++)
    {
        ends_q(lo, hi, L->ball + j);
        found = fmpq_equal(lo, hi) && fmpq_equal(lo, r);
    }
    fmpq_clear(lo);
    fmpq_clear(hi);
    return found;
}

/* ---- 1. the hidden function on the 18 REAL_CASES (proto/solvers_checks.py:2613) ---- */

typedef struct
{
    const char * name;
    slong nf;
    slong flen[10];
    slong fac[10][7];
    slong mult[10];
    slong expect;
} real_case;

static const real_case REAL_CASES[18] = {
    { "(x-1)(x-2)(x-3)", 3, { 2, 2, 2 }, { { -1, 1 }, { -2, 1 }, { -3, 1 } }, { 1, 1, 1 }, 3 },
    { "x^2-2", 1, { 3 }, { { -2, 0, 1 } }, { 1 }, 2 },
    { "x^2+1", 1, { 3 }, { { 1, 0, 1 } }, { 1 }, 0 },
    { "x", 1, { 2 }, { { 0, 1 } }, { 1 }, 1 },
    { "7", 1, { 1 }, { { 7 } }, { 1 }, 0 },
    { "2x-1", 1, { 2 }, { { -1, 2 } }, { 1 }, 1 },
    { "(2x-1)(x^2-2)", 2, { 2, 3 }, { { -1, 2 }, { -2, 0, 1 } }, { 1, 1 }, 3 },
    { "(x-1)^2(x+3)", 2, { 2, 2 }, { { -1, 1 }, { 3, 1 } }, { 2, 1 }, 2 },
    { "x^3(x-1)^2(x^2+1)", 3, { 2, 2, 3 }, { { 0, 1 }, { -1, 1 }, { 1, 0, 1 } }, { 3, 2, 1 }, 2 },
    { "x^5-x-1", 1, { 6 }, { { -1, -1, 0, 0, 0, 1 } }, { 1 }, 1 },
    { "(1000x^2-2000)(1000x^2-2001)", 2, { 3, 3 }, { { -2000, 0, 1000 }, { -2001, 0, 1000 } }, { 1, 1 }, 4 },
    { "(x^2-2)^2-10^-12 scaled", 1, { 5 }, { { WORD(3999999999999), 0, WORD(-4000000000000), 0,
                                                WORD(1000000000000) } }, { 1 }, 4 },
    { "Wilkinson 10", 10, { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
      { { -1, 1 }, { -2, 1 }, { -3, 1 }, { -4, 1 }, { -5, 1 }, { -6, 1 }, { -7, 1 }, { -8, 1 }, { -9, 1 },
        { -10, 1 } }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 10 },
    { "(x^2-13)(x^2-17)(x^2-221)", 3, { 3, 3, 3 }, { { -13, 0, 1 }, { -17, 0, 1 }, { -221, 0, 1 } }, { 1, 1, 1 },
      6 },
    { "Chebyshev T_6", 1, { 7 }, { { -1, 0, 18, 0, -48, 0, 32 } }, { 1 }, 6 },
    { "x^4+x^3+x^2+x+1", 1, { 5 }, { { 1, 1, 1, 1, 1 } }, { 1 }, 0 },
    { "x^4-10x^2+1", 1, { 5 }, { { 1, 0, -10, 0, 1 } }, { 1 }, 4 },
    { "(3x+7)(5x-11)(x^2+x+1)", 3, { 2, 2, 3 }, { { 7, 3 }, { -11, 5 }, { 1, 1, 1 } }, { 1, 1, 1 }, 2 },
};

static void
case_poly(fmpz_poly_t f, const real_case * rc)
{
    fmpz_poly_t t;
    slong i, j;

    fmpz_poly_init(t);
    fmpz_poly_one(f);
    for (i = 0; i < rc->nf; i++)
    {
        poly_set_si(t, rc->fac[i], rc->flen[i]);
        for (j = 0; j < rc->mult[i]; j++)
            fmpz_poly_mul(f, f, t);
    }
    fmpz_poly_clear(t);
}

ADF_TEST(isolate_on_real_cases)
{
    static const slong precs[] = { 2, 20, 53, 200 };
    fmpz_poly_t f, g;
    arb_ptr b;
    slong i, k, m, d, good = 0, runs = 0;
    char what[160];
    int st;

    fmpz_poly_init(f);
    fmpz_poly_init(g);
    for (i = 0; i < 18; i++)
    {
        case_poly(f, REAL_CASES + i);
        oracle_normalise(g, f);
        d = fmpz_poly_degree(g);
        if (d < 1)
            continue;                           /* the constant 7: the caller handles it (count 0) */
        b = _arb_vec_init(d);
        for (k = 0; k < 4; k++)
        {
            flint_sprintf(what, "isolate %s at prec %wd", REAL_CASES[i].name, precs[k]);
            m = -1;
            st = adf_roots_real_isolate(b, &m, g, precs[k]);
            runs++;
            ADF_CHECK_MSG(st == ADF_OK && m == REAL_CASES[i].expect, "%s: status %d, %ld balls", what, st,
                          (long) m);
            if (st == ADF_OK && m >= 0 && m <= d)
                good += balls_true(g, b, m, precs[k], what);
        }
        _arb_vec_clear(b, d);
    }
    printf("   isolate: %ld runs on the 17 nonconstant REAL_CASES, %ld true by the oracle\n", (long) runs,
           (long) good);
    ADF_CHECK(runs == 68 && good == runs);
    fmpz_poly_clear(f);
    fmpz_poly_clear(g);
}

/* ---- 2. adf_roots_real on the 18 REAL_CASES ---- */

ADF_TEST(real_cases_end_to_end)
{
    static const slong precs[] = { 2, 20, 53, 200, 2000 };
    adf_rootlist_t L;
    fmpz_poly_t f;
    slong i, k, good = 0, runs = 0;
    char what[160];

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    for (i = 0; i < 18; i++)
        for (k = 0; k < 5; k++)
        {
            case_poly(f, REAL_CASES + i);
            flint_sprintf(what, "%s at prec %wd", REAL_CASES[i].name, precs[k]);
            runs++;
            ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s: status", what);
            ADF_CHECK_MSG(L->n == REAL_CASES[i].expect, "%s: %ld roots", what, (long) L->n);
            good += list_true(L, f, precs[k], what);
        }
    printf("   REAL_CASES: %ld runs (prec 2, 20, 53, 200, 2000), %ld lists true by the oracle\n", (long) runs,
           (long) good);
    ADF_CHECK(good == runs);
    fmpz_poly_clear(f);
    adf_rootlist_clear(L);
}

/* ---- 3. the slow family: 1234567 (X - 2^e)(X - 2^e - 1), with a bound on the time ---- */

/* the families of bench/bench_roots_real.c and proto/test_real_isolation.py; r gets the two planted roots */
static void
family(fmpz_poly_t f, fmpq * r, const char * name, ulong e)
{
    fmpz_t a, b, t;

    fmpz_init(a);
    fmpz_init(b);
    fmpz_init(t);
    fmpz_one(t);
    fmpz_mul_2exp(t, t, e);                     /* t = 2^e */
    if (strcmp(name, "pair") == 0)              /* 1234567 (X - 2^e)(X - 2^e - 1) */
    {
        fmpz_poly_set_ui(f, 1234567);
        fmpz_one(a);
        poly_mul_lin(f, a, t);
        fmpz_add_ui(b, t, 1);
        poly_mul_lin(f, a, b);
        fmpq_set_fmpz_frac(r + 0, t, a);
        fmpq_set_fmpz_frac(r + 1, b, a);
    }
    else if (strcmp(name, "thirds") == 0)       /* (3X - 3 2^e - 1)(3X - 3 2^e - 2): 2^e + 1/3, 2^e + 2/3 */
    {
        fmpz_poly_one(f);
        fmpz_set_ui(a, 3);
        fmpz_mul_ui(b, t, 3);
        fmpz_add_ui(b, b, 1);
        poly_mul_lin(f, a, b);
        fmpq_set_fmpz_frac(r + 0, b, a);
        fmpz_add_ui(b, b, 1);
        poly_mul_lin(f, a, b);
        fmpq_set_fmpz_frac(r + 1, b, a);
    }
    else if (strcmp(name, "close") == 0)        /* (X - 1)(2^e X - 2^e - 1): 1 and 1 + 2^-e */
    {
        fmpz_poly_one(f);
        fmpz_one(a);
        fmpz_one(b);
        poly_mul_lin(f, a, b);
        fmpq_set_fmpz_frac(r + 0, b, a);
        fmpz_add_ui(b, t, 1);
        poly_mul_lin(f, t, b);
        fmpq_set_fmpz_frac(r + 1, b, t);
    }
    else                                        /* "cthirds": (3X - 1)(3 2^e X - 2^e - 1): 1/3, 1/3 + 2^-e/3 */
    {
        fmpz_poly_one(f);
        fmpz_set_ui(a, 3);
        fmpz_one(b);
        poly_mul_lin(f, a, b);
        fmpq_set_fmpz_frac(r + 0, b, a);
        fmpz_mul_ui(a, t, 3);
        fmpz_add_ui(b, t, 1);
        poly_mul_lin(f, a, b);
        fmpq_set_fmpz_frac(r + 1, b, a);
    }
    fmpz_clear(a);
    fmpz_clear(b);
    fmpz_clear(t);
}

static void
run_family(const char * name, const ulong * es, slong ne, const slong * precs, slong np, double bound)
{
    adf_rootlist_t L;
    fmpz_poly_t f;
    fmpq r[2];
    slong i, k;
    double t, worst = 0;
    char what[160];

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    fmpq_init(r + 0);
    fmpq_init(r + 1);
    for (i = 0; i < ne; i++)
        for (k = 0; k < np; k++)
        {
            family(f, r, name, es[i]);
            flint_sprintf(what, "%s e = %wu at prec %wd", name, es[i], precs[k]);
            t = now();
            ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s: status", what);
            t = now() - t;
            worst = t > worst ? t : worst;
            ADF_CHECK_MSG(t < bound, "%s: %.3f s, the bound is %.1f s", what, t, bound);
            list_true(L, f, precs[k], what);
            planted_match(L, r, 2, what);
        }
    printf("   %s: e up to %lu, the slowest call %.4f s (bound %.1f s)\n", name, (unsigned long) es[ne - 1], worst,
           bound);
    fmpq_clear(r + 0);
    fmpq_clear(r + 1);
    fmpz_poly_clear(f);
    adf_rootlist_clear(L);
}

ADF_TEST(slow_family_pair_with_a_bound_on_the_time)
{
    static const ulong es[] = { 600, 1500, 3000, 10000 };
    static const slong precs[] = { 2, 53 };

    run_family("pair", es, 4, precs, 2, 10.0);
    run_family("thirds", es, 4, precs, 2, 10.0);
}

/* ---- 4. close small roots: 1 and 1 + 2^-e; 1/3 and 1/3 + 2^-e / 3 ---- */

ADF_TEST(close_small_roots)
{
    static const ulong es[] = { 40, 600, 3000, 10000 };
    static const slong precs[] = { 2, 53, 300 };

    run_family("close", es, 4, precs, 3, 10.0);
    run_family("cthirds", es, 4, precs, 3, 10.0);
}

/* ---- 5. Wilkinson 20 and Mignotte X^n - 2 (a X - 1)^2 ---- */

ADF_TEST(wilkinson_20_and_mignotte)
{
    static const slong precs[] = { 2, 53, 300 };
    static const slong mig[][2] = { { 5, 3 }, { 10, 10 }, { 20, 100 }, { 17, 1000 }, { 30, 7 } };
    adf_rootlist_t L;
    fmpz_poly_t f, t;
    fmpq r[20];
    fmpz_t a;
    slong i, k, n;
    char what[160];

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    fmpz_poly_init(t);
    fmpz_init(a);
    for (i = 0; i < 20; i++)
    {
        fmpq_init(r + i);
        fmpq_set_si(r + i, i + 1, 1);
    }
    fmpz_poly_one(f);
    for (i = 0; i < 20; i++)
        poly_mul_root(f, r + i, 1);
    for (k = 0; k < 3; k++)
    {
        flint_sprintf(what, "Wilkinson 20 at prec %wd", precs[k]);
        ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s: status", what);
        list_true(L, f, precs[k], what);
        /* design R4(3): the odd parts of 1..20 have at most 5 bits, so exact from need = 4 on */
        ADF_CHECK_MSG(planted_match(L, r, 20, what) == 20 || precs[k] < 4, "%s: the integer roots are not all "
                      "exact", what);
    }
    /* Mignotte: X^n - 2 (a X - 1)^2, two real roots very close to 1/a (and n - 2 others, counted by the
       oracle) */
    for (i = 0; i < 5; i++)
    {
        n = mig[i][0];
        fmpz_poly_zero(f);
        fmpz_poly_set_coeff_si(f, n, 1);
        fmpz_poly_zero(t);
        fmpz_poly_set_coeff_si(t, 1, mig[i][1]);
        fmpz_poly_set_coeff_si(t, 0, -1);
        fmpz_poly_mul(t, t, t);
        fmpz_poly_scalar_mul_si(t, t, 2);
        fmpz_poly_sub(f, f, t);
        for (k = 0; k < 3; k++)
        {
            flint_sprintf(what, "Mignotte n = %wd, a = %wd at prec %wd", n, mig[i][1], precs[k]);
            ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s: status", what);
            list_true(L, f, precs[k], what);
            ADF_CHECK_MSG(L->n >= 2, "%s: %ld roots", what, (long) L->n);
        }
    }
    for (i = 0; i < 20; i++)
        fmpq_clear(r + i);
    fmpz_clear(a);
    fmpz_poly_clear(f);
    fmpz_poly_clear(t);
    adf_rootlist_clear(L);
}

/* ---- 6. roots at dyadic points and at 0 (design Proposition R4(3), decision 4 of the brief) ---- */

ADF_TEST(dyadic_roots_and_zero_are_exact_balls)
{
    /* 0, 1/2, -3/4, 5/8, 2^-30, 12345, -7 2^40, and 1/3 (not dyadic); every root double, times X^2 + 1 */
    static const slong num[] = { 0, 1, -3, 5, 1, 12345, -7, 1 };
    static const slong ex[] = { 0, -1, -2, -3, -30, 0, 40, 0 };
    static const slong precs[] = { 2, 10, 53 };
    static const slong x2p1[] = { 1, 0, 1 };
    adf_rootlist_t L;
    fmpz_poly_t f, t;
    fmpq r[8];
    slong i, k, need, odd;
    char what[160];

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    fmpz_poly_init(t);
    for (i = 0; i < 8; i++)
    {
        fmpq_init(r + i);
        if (i < 7)
            q_set_2exp_si(r + i, num[i], ex[i]);
        else
            fmpq_set_si(r + i, 1, 3);
    }
    fmpz_poly_set_si(f, -5);
    for (i = 0; i < 8; i++)
        poly_mul_root(f, r + i, 2);
    poly_set_si(t, x2p1, 3);
    fmpz_poly_mul(f, f, t);
    for (k = 0; k < 3; k++)
    {
        flint_sprintf(what, "dyadic roots at prec %wd", precs[k]);
        ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s: status", what);
        list_true(L, f, precs[k], what);
        planted_match(L, r, 8, what);
        need = precs[k] < 2 ? 2 : precs[k];
        for (i = 0; i < 7; i++)
        {
            /* the odd part of the numerator has odd bits; exact when odd <= need + 1 (design R4(3)); 0 always */
            odd = num[i] == 0 ? 0 : (slong) FLINT_BIT_COUNT(FLINT_ABS(num[i]));
            if (odd <= need + 1)
                ADF_CHECK_MSG(exact_ball_at(L, r + i), "%s: the dyadic root %ld 2^%ld is not an exact ball", what,
                              (long) num[i], (long) ex[i]);
        }
        ADF_CHECK_MSG(!exact_ball_at(L, r + 7), "%s: 1/3 as an exact ball", what);
    }
    for (i = 0; i < 8; i++)
        fmpq_clear(r + i);
    fmpz_poly_clear(f);
    fmpz_poly_clear(t);
    adf_rootlist_clear(L);
}

/* ---- 7. no real root, degree 1, multiple roots of f, negative leading coefficient ---- */

ADF_TEST(no_root_degree_one_multiple_and_negative_lead)
{
    static const slong none1[] = { 1, 0, 1 }, none2[] = { 1, 1, 1, 1, 1 }, none3[] = { 2, 0, 1 };
    static const slong precs[] = { 2, 53, 1000 };
    adf_rootlist_t L;
    fmpz_poly_t f, t;
    fmpq r[4];
    fmpz_t a, b;
    slong i, k, j;
    char what[160];

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    fmpz_poly_init(t);
    fmpz_init(a);
    fmpz_init(b);
    for (i = 0; i < 4; i++)
        fmpq_init(r + i);
    for (k = 0; k < 3; k++)
    {
        /* no real root: X^2 + 1, X^4 + X^3 + X^2 + X + 1, (X^2 + 1)(X^2 + 2)^3 */
        poly_set_si(f, none1, 3);
        ADF_CHECK(adf_roots_real(L, f, precs[k]) == ADF_OK && L->n == 0 && L->ball == NULL);
        list_true(L, f, precs[k], "X^2 + 1");
        poly_set_si(f, none2, 5);
        ADF_CHECK(adf_roots_real(L, f, precs[k]) == ADF_OK && L->n == 0);
        list_true(L, f, precs[k], "X^4 + X^3 + X^2 + X + 1");
        poly_set_si(t, none3, 3);
        fmpz_poly_pow(t, t, 3);
        poly_set_si(f, none1, 3);
        fmpz_poly_mul(f, f, t);
        ADF_CHECK(adf_roots_real(L, f, precs[k]) == ADF_OK && L->n == 0 && L->reduced == 1);
        list_true(L, f, precs[k], "(X^2 + 1)(X^2 + 2)^3");
        /* degree 1: 3 X - 7; X - 10^400 (10^400 = 5^400 2^400, odd part of 929 bits); 2^100 X - 1 */
        fmpz_set_ui(a, 3);
        fmpz_set_ui(b, 7);
        fmpz_poly_one(f);
        poly_mul_lin(f, a, b);
        fmpq_set_si(r + 0, 7, 3);
        flint_sprintf(what, "3X - 7 at prec %wd", precs[k]);
        ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s", what);
        list_true(L, f, precs[k], what);
        planted_match(L, r, 1, what);
        fmpz_one(a);
        fmpz_set_ui(b, 10);
        fmpz_pow_ui(b, b, 400);
        fmpz_poly_one(f);
        poly_mul_lin(f, a, b);
        fmpq_set_fmpz_frac(r + 0, b, a);
        flint_sprintf(what, "X - 10^400 at prec %wd", precs[k]);
        ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s", what);
        list_true(L, f, precs[k], what);
        planted_match(L, r, 1, what);
        ADF_CHECK_MSG(exact_ball_at(L, r + 0) == (precs[k] >= 928), "%s: exact %d", what, exact_ball_at(L, r + 0));
        fmpz_one(a);
        fmpz_mul_2exp(a, a, 100);
        fmpz_one(b);
        fmpz_poly_one(f);
        poly_mul_lin(f, a, b);
        fmpq_set_fmpz_frac(r + 0, b, a);
        flint_sprintf(what, "2^100 X - 1 at prec %wd", precs[k]);
        ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s", what);
        list_true(L, f, precs[k], what);
        ADF_CHECK_MSG(exact_ball_at(L, r + 0), "%s: 2^-100 not exact", what);
        /* multiple roots of f: (X - 1)^5 (X + 2)^3 (2 X - 1)^2 (3 X + 1), and each with a negative leading
           coefficient: the same list */
        for (j = 0; j < 2; j++)
        {
            fmpq_set_si(r + 0, 1, 1);
            fmpq_set_si(r + 1, -2, 1);
            fmpq_set_si(r + 2, 1, 2);
            fmpq_set_si(r + 3, -1, 3);
            fmpz_poly_set_si(f, j == 0 ? 1 : -6);
            poly_mul_root(f, r + 0, 5);
            poly_mul_root(f, r + 1, 3);
            poly_mul_root(f, r + 2, 2);
            poly_mul_root(f, r + 3, 1);
            flint_sprintf(what, "multiple roots, lead %s, at prec %wd", j == 0 ? "+" : "-", precs[k]);
            ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK && L->reduced == 1, "%s", what);
            list_true(L, f, precs[k], what);
            planted_match(L, r, 4, what);
        }
        /* negative leading coefficient, squarefree: -(X - 1)(X - 2)(X - 3) and -X^5 + X + 1 */
        case_poly(f, REAL_CASES + 0);
        fmpz_poly_neg(f, f);
        ADF_CHECK(adf_roots_real(L, f, precs[k]) == ADF_OK && L->n == 3 && L->reduced == 0);
        list_true(L, f, precs[k], "-(X - 1)(X - 2)(X - 3)");
        case_poly(f, REAL_CASES + 9);
        fmpz_poly_neg(f, f);
        ADF_CHECK(adf_roots_real(L, f, precs[k]) == ADF_OK && L->n == 1);
        list_true(L, f, precs[k], "-X^5 + X + 1");
    }
    for (i = 0; i < 4; i++)
        fmpq_clear(r + i);
    fmpz_clear(a);
    fmpz_clear(b);
    fmpz_poly_clear(f);
    fmpz_poly_clear(t);
    adf_rootlist_clear(L);
}

/* ---- 8. touching cells, and balls nested when prec grows (design Proposition R4(4)) ---- */

/* 1 if every ball of B lies in the ball of the same index of A (same n) */
static int
nested(const adf_rootlist_t A, const adf_rootlist_t B)
{
    fmpq_t lo0, hi0, lo1, hi1;
    slong j;
    int ok = A->n == B->n;

    fmpq_init(lo0);
    fmpq_init(hi0);
    fmpq_init(lo1);
    fmpq_init(hi1);
    for (j = 0; j < A->n && ok; j++)
    {
        ends_q(lo0, hi0, A->ball + j);
        ends_q(lo1, hi1, B->ball + j);
        ok = fmpq_cmp(lo0, lo1) <= 0 && fmpq_cmp(hi1, hi0) <= 0;
    }
    fmpq_clear(lo0);
    fmpq_clear(hi0);
    fmpq_clear(lo1);
    fmpq_clear(hi1);
    return ok;
}

ADF_TEST(touching_cells_and_nesting)
{
    /* roots as num/den: cells that touch at a point that is not a root (1/3, 2/3), cells that end at a root
       (1/2 with 1/4, 3/4), clusters, symmetric roots, and 2^b - 1 next to 2^b + 1 (accuracies that differ by
       one bit across a power of 2) */
    static const slong sets[][5][2] = {
        { { 1, 3 }, { 2, 3 }, { 0, 0 } },
        { { 1, 3 }, { 1, 2 }, { 2, 3 }, { 0, 0 } },
        { { 1, 2 }, { 5, 8 }, { 3, 4 }, { 0, 0 } },
        { { 1, 2 }, { 9, 16 }, { 19, 32 }, { 5, 8 }, { 0, 0 } },
        { { -1, 3 }, { 0, 1 }, { 1, 3 }, { 0, 0 } },
        { { 191, 3 }, { 193, 3 }, { 0, 0 } },
        { { 1021, 3 }, { 1025, 3 }, { 1027, 3 }, { 0, 0 } },
    };
    static const slong precs[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 20, 30, 53, 100 };
    adf_rootlist_t L[15];
    fmpz_poly_t f;
    fmpq r[5];
    slong s, i, k, n, pairs = 0;
    char what[160];

    for (k = 0; k < 15; k++)
        adf_rootlist_init(L[k]);
    fmpz_poly_init(f);
    for (i = 0; i < 5; i++)
        fmpq_init(r + i);
    for (s = 0; s < 7; s++)
    {
        fmpz_poly_one(f);
        for (n = 0; n < 5 && sets[s][n][1] != 0; n++)
        {
            fmpq_set_si(r + n, sets[s][n][0], (ulong) sets[s][n][1]);
            poly_mul_root(f, r + n, 1);
        }
        for (k = 0; k < 15; k++)
        {
            flint_sprintf(what, "touching set %wd at prec %wd", s, precs[k]);
            ADF_CHECK_MSG(adf_roots_real(L[k], f, precs[k]) == ADF_OK, "%s: status", what);
            list_true(L[k], f, precs[k], what);
            planted_match(L[k], r, n, what);
            if (k > 0)
            {
                ADF_CHECK_MSG(nested(L[k - 1], L[k]), "%s: not nested in the list at prec %ld", what,
                              (long) precs[k - 1]);
                pairs++;
            }
        }
    }
    printf("   touching cells: 7 sets at 15 precisions, %ld nested pairs of lists\n", (long) pairs);
    for (k = 0; k < 15; k++)
        adf_rootlist_clear(L[k]);
    for (i = 0; i < 5; i++)
        fmpq_clear(r + i);
    fmpz_poly_clear(f);
}

/* ---- 9. random planted roots and random integer polynomials ---- */

ADF_TEST(random_planted_and_random_polynomials)
{
    adf_rootlist_t L;
    fmpz_poly_t f, t;
    fmpq pool[14], r[5];
    flint_rand_t st;
    slong i, j, k, m, n = 0, roots = 0, idx[5];
    static const slong leads[] = { 1, 2, -3, 5 };
    static const slong extra[3][3] = { { 1, 0, 1 }, { 1, 1, 1 }, { 2, -1, 3 } };
    static const slong precs[] = { 2, 20, 64 };
    char what[160];

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    fmpz_poly_init(t);
    flint_randinit(st);
    for (i = 0; i < 14; i++)
        fmpq_init(pool + i);
    for (i = 0; i < 5; i++)
        fmpq_init(r + i);
    /* the pool of proto/test_real_isolation.py, test_planted_random */
    fmpq_set_si(pool + 0, -3, 1);
    fmpq_set_si(pool + 1, -1, 2);
    fmpq_set_si(pool + 2, 0, 1);
    fmpq_set_si(pool + 3, 1, 3);
    fmpq_set_si(pool + 4, 1, 1);
    q_set_2exp_si(pool + 5, (WORD(1) << 40) + 1, -40);                     /* 1 + 2^-40 */
    fmpz_set_ui(fmpq_numref(pool + 6), 10);
    fmpz_pow_ui(fmpq_numref(pool + 6), fmpq_numref(pool + 6), 30);           /* 10^30 */
    fmpz_set_si(fmpq_numref(pool + 7), -1);
    fmpz_set_ui(fmpq_denref(pool + 7), 10);
    fmpz_pow_ui(fmpq_denref(pool + 7), fmpq_denref(pool + 7), 20);           /* -10^-20 */
    fmpq_set_si(pool + 8, 7, 5);
    fmpq_set_si(pool + 9, -7, 5);
    q_set_2exp_si(pool + 10, 1, -30);
    q_set_2exp_si(pool + 11, 1, 200);                                        /* 2^200 */
    fmpq_set_si(pool + 12, 1, 3);
    fmpq_add(pool + 12, pool + 12, pool + 11);                               /* 2^200 + 1/3 */
    q_set_2exp_si(pool + 13, 1, -90);
    fmpq_add(pool + 13, pool + 13, pool + 3);                                /* 1/3 + 2^-90 */
    for (i = 0; i < 60; i++)
    {
        k = 1 + (slong) n_randint(st, 5);
        for (j = 0; j < k; j++)             /* k distinct indices */
        {
            slong q, dup;
            do
            {
                idx[j] = (slong) n_randint(st, 14);
                for (q = 0, dup = 0; q < j; q++)
                    dup |= idx[q] == idx[j];
            } while (dup);
        }
        fmpz_poly_set_si(f, leads[n_randint(st, 4)]);
        for (j = 0; j < k; j++)
        {
            fmpq_set(r + j, pool + idx[j]);
            poly_mul_root(f, r + j, 1 + (slong) n_randint(st, 3));
        }
        if (n_randint(st, 10) < 4)
        {
            poly_set_si(t, extra[n_randint(st, 3)], 3);
            fmpz_poly_mul(f, f, t);
        }
        for (m = 0; m < 3; m++)
        {
            flint_sprintf(what, "planted random %wd at prec %wd", i, precs[m]);
            ADF_CHECK_MSG(adf_roots_real(L, f, precs[m]) == ADF_OK, "%s: status", what);
            list_true(L, f, precs[m], what);
            planted_match(L, r, k, what);
            n++;
        }
    }
    /* random integer polynomials of degree 1 to 9, coefficients in [-20, 20] */
    for (i = 0; i < 150; i++)
    {
        fmpz_poly_zero(f);
        m = 2 + (slong) n_randint(st, 9);
        for (j = 0; j < m; j++)
            fmpz_poly_set_coeff_si(f, j, (slong) n_randint(st, 41) - 20);
        if (fmpz_poly_is_zero(f))
            continue;
        flint_sprintf(what, "random polynomial %wd", i);
        ADF_CHECK_MSG(adf_roots_real(L, f, i % 2 ? 30 : 2) == ADF_OK, "%s: status", what);
        list_true(L, f, i % 2 ? 30 : 2, what);
        roots += L->n;
        n++;
    }
    printf("   random: %ld calls, %ld roots in the random integer polynomials\n", (long) n, (long) roots);
    ADF_CHECK(n >= 320 && roots > 100);
    for (i = 0; i < 14; i++)
        fmpq_clear(pool + i);
    for (i = 0; i < 5; i++)
        fmpq_clear(r + i);
    flint_randclear(st);
    fmpz_poly_clear(f);
    fmpz_poly_clear(t);
    adf_rootlist_clear(L);
}

/* ---- 10. LIMIT: a root below 2^(2 - ADF_ROOTS_BITS_MAX) or at 2^(ADF_ROOTS_BITS_MAX + 5) ---- */

ADF_TEST(limit_for_roots_outside_the_admissible_size)
{
    adf_rootlist_t L;
    fmpz_poly_t f;
    fmpz_t a, b;
    static const slong c3[] = { 6, -2, -3, 1 };
    adf_rootlist_struct raw;
    int st;

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    fmpz_init(a);
    fmpz_init(b);
    poly_set_si(f, c3, 4);
    ADF_CHECK(adf_roots_real(L, f, 64) == ADF_OK && L->n == 3);
    memcpy(&raw, L, sizeof(raw));
    /* 2^(M + 10) X - 1: the root 2^-(M + 10) needs a cell below 2^(2 - M) */
    fmpz_one(a);
    fmpz_mul_2exp(a, a, ADF_ROOTS_BITS_MAX + 10);
    fmpz_one(b);
    fmpz_poly_one(f);
    poly_mul_lin(f, a, b);
    st = adf_roots_real(L, f, 2);
    ADF_CHECK_MSG(st == ADF_LIMIT, "2^(M + 10) X - 1: status %d", st);
    ADF_CHECK(memcmp(&raw, L, sizeof(raw)) == 0);
    /* (3 X - 1)(2^(M + 10) X - 1): the same, next to a root of normal size */
    fmpz_set_ui(a, 3);
    poly_mul_lin(f, a, b);
    st = adf_roots_real(L, f, 2);
    ADF_CHECK_MSG(st == ADF_LIMIT, "(3X - 1)(2^(M + 10) X - 1): status %d", st);
    ADF_CHECK(memcmp(&raw, L, sizeof(raw)) == 0);
    /* X - 2^(M + 5): an exact root whose ball is not of admissible size (midpoint above 2^M) */
    fmpz_one(a);
    fmpz_one(b);
    fmpz_mul_2exp(b, b, ADF_ROOTS_BITS_MAX + 5);
    fmpz_poly_one(f);
    poly_mul_lin(f, a, b);
    st = adf_roots_real(L, f, 2);
    ADF_CHECK_MSG(st == ADF_LIMIT, "X - 2^(M + 5): status %d", st);
    ADF_CHECK(memcmp(&raw, L, sizeof(raw)) == 0);
    /* 2^(2M + 2) X^2 + 1: complex roots of absolute value 2^-(M + 1) and no real one: OK with the empty list,
       not LIMIT (the root bound 2^K is below 2^KMIN; the root cell is taken at KMIN) */
    fmpz_poly_zero(f);
    fmpz_one(a);
    fmpz_mul_2exp(a, a, 2 * ADF_ROOTS_BITS_MAX + 2);
    fmpz_poly_set_coeff_fmpz(f, 2, a);
    fmpz_poly_set_coeff_si(f, 0, 1);
    st = adf_roots_real(L, f, 2);
    ADF_CHECK_MSG(st == ADF_OK && L->n == 0, "2^(2M + 2) X^2 + 1: status %d", st);
    /* 2^(M - 10) X - 1: the root 2^(10 - M) is admissible and exact */
    fmpz_one(a);
    fmpz_mul_2exp(a, a, ADF_ROOTS_BITS_MAX - 10);
    fmpz_one(b);
    fmpz_poly_one(f);
    poly_mul_lin(f, a, b);
    st = adf_roots_real(L, f, 2);
    ADF_CHECK_MSG(st == ADF_OK && L->n == 1 && arb_is_exact(L->ball + 0), "2^(M - 10) X - 1: status %d", st);
    fmpz_clear(a);
    fmpz_clear(b);
    fmpz_poly_clear(f);
    adf_rootlist_clear(L);
}

/* ---- 11. high precision, with a bound on the time ---- */

ADF_TEST(high_precision_with_a_bound_on_the_time)
{
    static const slong c1[] = { -2, 0, 1 }, c2[] = { -1, -1, 0, 0, 0, 1 };
    static const slong precs[] = { 4096, 65536, ADF_ROOTS_REAL_PREC_MAX };
    adf_rootlist_t L;
    fmpz_poly_t f;
    slong i, k;
    double t, worst = 0;
    char what[160];

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    for (i = 0; i < 2; i++)
        for (k = 0; k < 3; k++)
        {
            if (i == 0)
                poly_set_si(f, c1, 3);
            else
                poly_set_si(f, c2, 6);
            flint_sprintf(what, "%s at prec %wd", i == 0 ? "X^2 - 2" : "X^5 - X - 1", precs[k]);
            t = now();
            ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s: status", what);
            t = now() - t;
            worst = t > worst ? t : worst;
            ADF_CHECK_MSG(t < 30.0, "%s: %.3f s, the bound is 30 s", what, t);
            list_true(L, f, precs[k], what);
        }
    printf("   high precision: the slowest call %.4f s (bound 30 s)\n", worst);
    fmpz_poly_clear(f);
    adf_rootlist_clear(L);
}

/* The bound is 2 s on the development laptop, as required by lane r-slice2.
   It covers the public call, including normalisation, count and final certificate. */
#define ISOLATION_SECONDS_MAX 2.0

ADF_TEST(review_cost_inputs)
{
    adf_rootlist_t L;
    fmpz_poly_t f, h;
    fmpz_t a, b;
    slong j, i;
    double t;
    int st;

    adf_rootlist_init(L);
    fmpz_poly_init(f); fmpz_poly_init(h);
    fmpz_init(a); fmpz_init(b);
    for (j = 0; j < 4; j++)
    {
        fmpz_poly_one(f);
        if (j < 2)
        {
            fmpz_one(a); fmpz_mul_2exp(a, a, j == 0 ? 4000 : 4999);
            fmpz_poly_zero(h);
            fmpz_poly_set_coeff_fmpz(h, 1, a);
            fmpz_poly_set_coeff_si(h, 0, -1);
            fmpz_poly_mul(f, h, h);
            fmpz_poly_scalar_mul_si(f, f, j == 0 ? -2 : 2);
            fmpz_poly_set_coeff_si(f, 50, 1);
        }
        else if (j == 2)
        {
            fmpz_one(a); fmpz_mul_2exp(a, a, 380);
            for (i = 1; i <= 25; i++)
            {
                fmpz_add_ui(b, a, i); fmpz_poly_set_coeff_fmpz(h, 0, b);
                fmpz_mul_si(b, a, -6); fmpz_poly_set_coeff_fmpz(h, 1, b);
                fmpz_mul_ui(b, a, 9); fmpz_poly_set_coeff_fmpz(h, 2, b);
                fmpz_poly_mul(f, f, h);
            }
        }
        else
        {
            fmpz_set_ui(a, 3);
            fmpz_one(b); fmpz_mul_2exp(b, b, 30000); fmpz_mul_ui(b, b, 3);
            fmpz_add_ui(b, b, 1); poly_mul_lin(f, a, b);
            fmpz_add_ui(b, b, 1); poly_mul_lin(f, a, b);
        }
        t = now(); st = adf_roots_real(L, f, 2); t = now() - t;
        printf("   review input %ld: %.6f s, status %d, roots %ld\n", j, t, st, L->n);
        ADF_CHECK_MSG(t < ISOLATION_SECONDS_MAX, "review input %ld: %.6f s > %.1f s", j, t,
                      ISOLATION_SECONDS_MAX);
        ADF_CHECK(st == ADF_OK);
        ADF_CHECK(L->n == (j == 0 ? 4 : j == 3 ? 2 : 0));
        ADF_CHECK(L->count == L->n && adf_place_is_archimedean(L->place));
        ADF_CHECK(adf_rootlist_verify_entries(L, f));
        ADF_CHECK(adf_rootlist_verify_complete(L, f, 0));
    }
    fmpz_clear(a); fmpz_clear(b);
    fmpz_poly_clear(f); fmpz_poly_clear(h); adf_rootlist_clear(L);
}

ADF_TEST(planted_roots_near_minimum_scale)
{
    adf_rootlist_t L;
    fmpz_poly_t f;
    fmpz_t a, b;
    fmpq_t r;
    slong i, j;
    int st;
    adf_rootlist_init(L); fmpz_poly_init(f);
    fmpz_init(a); fmpz_init(b); fmpq_init(r);
    fmpz_one(a); fmpz_mul_2exp(a, a, ADF_ROOTS_BITS_MAX - 10);
    for (j = 1; j <= 4; j++)
    {
        fmpz_poly_one(f);
        for (i = (j == 4 ? 1 : j); i <= (j == 4 ? 3 : j + 1); i++)
        {
            fmpz_set_si(b, i); poly_mul_lin(f, a, b);
        }
        st = adf_roots_real(L, f, 2);
        ADF_CHECK(st == ADF_OK);
        ADF_CHECK(L->n == (j == 4 ? 3 : 2) && L->count == L->n);
        for (i = 0; st == ADF_OK && i < (j == 4 ? 3 : 2); i++)
        {
            fmpz_set_si(fmpq_numref(r), (j == 4 ? 1 : j) + i);
            fmpz_set(fmpq_denref(r), a); fmpq_canonicalise(r);
            ADF_CHECK(arb_contains_fmpq(L->ball + i, r));
            ADF_CHECK(arb_is_exact(L->ball + i));
        }
        printf("   minimum scale pair %ld: status %d, roots %ld, exponent %ld\n", j, st, L->n,
               10 - (slong) ADF_ROOTS_BITS_MAX);
        fflush(stdout);
    }
    fmpq_clear(r); fmpz_clear(a); fmpz_clear(b);
    fmpz_poly_clear(f); adf_rootlist_clear(L);
}

ADF_TEST(count_guided_regions)
{
    adf_rootlist_t L;
    fmpz_poly_t f;
    static const slong single[] = { -3, 1 }, three[] = { 0, -1, 0, 1 };
    slong i;
    adf_rootlist_init(L); fmpz_poly_init(f);
    for (i = 0; i < 2; i++)
    {
        poly_set_si(f, i == 0 ? single : three, i == 0 ? 2 : 4);
        ADF_CHECK(adf_roots_real(L, f, 2) == ADF_OK);
        ADF_CHECK(L->n == (i == 0 ? 1 : 3));
        ADF_CHECK(adf_place_is_archimedean(L->place));
        ADF_CHECK(adf_rootlist_verify_entries(L, f));
        ADF_CHECK(adf_rootlist_verify_complete(L, f, 0));
    }
    fmpz_poly_clear(f); adf_rootlist_clear(L);
}

int main(void) {
    for (adf_test *t = adf_test_head; t; t = t->next) {
        if (strcmp(t->name, "review_cost_inputs") && strcmp(t->name, "count_guided_regions")) continue;
        adf_test_current = t->name; t->fn();
    }
    printf("planted checks %lu failures %lu\n", adf_test_checks, adf_test_failures);
    return adf_test_failures ? 1 : 0;
}
