/* tests/test_roots_real.c: the real roots (milestone S, S.2, slice 3, lane s2-slice3): adf_roots_real,
   adf_rootlist_get_arb, and the real place of adf_rootlist_is_canonical, adf_rootlist_verify_entries and
   adf_rootlist_verify_complete.

   The statements tested are docs/proofs/solvers.md Proposition 3.8 (line 1313), Proposition 3.9 (line 1346),
   Algorithm RR and Proposition 3.10 (line 1399) and Proposition 3.13(4), (5) (line 1542); decisions S-D11,
   S-D13, S-D19 of docs/SPEC.md 15.3; the contract is include/adelefeld/roots.h. The oracles are written here
   and do not call the library, nor FLINT's count of real roots: the normalised polynomial over Q with
   fmpq_poly, a Sturm chain over Q (g, g', then minus the remainders) and its sign variations at rational
   points and at the two infinities, and the sign of g at a rational point by fmpz_poly_evaluate_fmpq. The
   exact end points of a ball are read with arb_get_interval_fmpz_2exp (arb.rst:461) and turned into
   rationals here. The vectors tests/ref/vectors/s2-slice3/real.jsonl come from real_roots_ref of the
   reference (lanes/s2-slice3/gen_vectors.py).

   One function of src/roots.c with hidden visibility is declared below: steps 5 to 7 of Algorithm RR for
   candidate balls given by the caller (the reference rr_finish, proto/solvers_checks.py:2594). It is not
   part of the interface; it is called here to reach the widening of step 5 and the tests of step 6 with
   balls that FLINT does not produce (solvers P3.10(4): on 60 calls of FLINT no ball had to be widened). */

#include <stddef.h>
#include <string.h>

#include <flint/fmpq_poly.h>
#include <flint/fmpz_vec.h>

#include <adelefeld.h>

#include "support/jsonl.h"

#include "test_runner.h"

/* hidden in src/roots.c (not declared by a public header, not exported by the shared object) */
int adf_roots_real_finish(adf_rootlist_t L, const fmpz_poly_t f, slong count, arb_srcptr in, slong m, slong prec);

/* ---- helpers: polynomials ---- */

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
    fmpz_clear(c);
    for (i = 0; i < mult; i++)
        fmpz_poly_mul(f, f, t);
    fmpz_poly_clear(t);
}

/* the oracle of the normalised polynomial: f / gcd(f, f') over Q, scaled to a primitive integer polynomial
   with positive leading coefficient; 1 for f constant (solvers L3.1(3)) */
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

/* ---- the oracle: a Sturm chain over Q ---- */

typedef struct
{
    fmpq_poly_struct * p;
    slong len;
    int is_const;                               /* g constant: no chain, no root */
} chain_t;

/* S_0 = g, S_1 = g', S_(k+1) = -(S_(k-1) mod S_k) until the remainder is 0; g squarefree (then the last
   element is a nonzero constant) */
static void
chain_init(chain_t * c, const fmpz_poly_t g)
{
    slong d = fmpz_poly_degree(g), i;
    fmpq_poly_t r;

    c->is_const = d <= 0;
    c->len = 0;
    c->p = NULL;
    if (c->is_const)
        return;
    c->p = flint_malloc((d + 1) * sizeof(fmpq_poly_struct));
    for (i = 0; i <= d; i++)
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
    /* the entries c->len to d stay initialised and zero; chain_clear clears all d + 1 */
}

static void
chain_clear(chain_t * c, const fmpz_poly_t g)
{
    slong d = fmpz_poly_degree(g), i;

    if (c->p == NULL)
        return;
    for (i = 0; i <= d; i++)
        fmpq_poly_clear(c->p + i);
    flint_free(c->p);
}

/* the number of sign changes of the chain at x (inf = 0), at -infinity (inf = -1) or +infinity (inf = 1) */
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

/* the sign of g at the rational x, exact */
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

/* the number of distinct roots of the squarefree g in (lo, hi] (Sturm), lo_inf = -1 for -infinity,
   hi_inf = 1 for +infinity */
static slong
roots_half_open(const chain_t * c, const fmpq_t lo, int lo_inf, const fmpq_t hi, int hi_inf)
{
    if (c->is_const)
        return 0;
    return variations(c, lo, lo_inf) - variations(c, hi, hi_inf);
}

/* the number of roots of g in the open interval (lo, hi) */
static slong
roots_open(const chain_t * c, const fmpz_poly_t g, const fmpq_t lo, int lo_inf, const fmpq_t hi, int hi_inf)
{
    slong k = roots_half_open(c, lo, lo_inf, hi, hi_inf);

    if (!c->is_const && hi_inf == 0 && sign_at(g, hi) == 0)
        k--;
    return k;
}

/* the number of roots of g in the closed interval [lo, hi] */
static slong
roots_closed(const chain_t * c, const fmpz_poly_t g, const fmpq_t lo, const fmpq_t hi)
{
    slong k = roots_half_open(c, lo, 0, hi, 0);

    if (!c->is_const && sign_at(g, lo) == 0)
        k++;
    return k;
}

/* ---- helpers: balls ---- */

/* r = m 2^e */
static void
q_set_2exp(fmpq_t r, const fmpz_t m, const fmpz_t e)
{
    if (fmpz_sgn(e) >= 0)
    {
        fmpz_mul_2exp(fmpq_numref(r), m, fmpz_get_ui(e));
        fmpz_one(fmpq_denref(r));
    }
    else
    {
        fmpz_t k;
        fmpz_init(k);
        fmpz_neg(k, e);
        fmpz_set(fmpq_numref(r), m);
        fmpz_one(fmpq_denref(r));
        fmpz_mul_2exp(fmpq_denref(r), fmpq_denref(r), fmpz_get_ui(k));
        fmpz_clear(k);
    }
    fmpq_canonicalise(r);
}

/* the exact end points of the finite ball x as rationals */
static void
ends_q(fmpq_t lo, fmpq_t hi, const arb_t x)
{
    fmpz_t a, b, e;

    fmpz_init(a);
    fmpz_init(b);
    fmpz_init(e);
    arb_get_interval_fmpz_2exp(a, b, e, x);
    q_set_2exp(lo, a, e);
    q_set_2exp(hi, b, e);
    fmpz_clear(a);
    fmpz_clear(b);
    fmpz_clear(e);
}

/* a ball that contains [lo, hi] (the union of two rounded balls at prec bits; its exact end points are read
   back by the test wherever they matter) */
static void
ball_from_q(arb_t x, const fmpq_t lo, const fmpq_t hi, slong prec)
{
    arb_t t;

    arb_init(t);
    arb_set_fmpq(x, lo, prec);
    arb_set_fmpq(t, hi, prec);
    arb_union(x, x, t, prec);
    arb_clear(t);
}

/* ball = mid + [-rad, rad] with mid = m1 2^e1, rad = m2 2^e2 (exact when m2 fits the mantissa of a mag) */
static void
ball_set_2exp(arb_t x, slong m1, slong e1, ulong m2, slong e2)
{
    arf_set_si_2exp_si(arb_midref(x), m1, e1);
    mag_set_ui_2exp_si(arb_radref(x), m2, e2);
}

/* a list at the real place written by hand: g, reduced, n balls copied from b, count. Not inlined: gcc 13 at -O2
   reports a false stringop-overread for the copy when it is inlined into a caller with a vector of two balls. */
static void __attribute__((noinline))
make_list(adf_rootlist_t M, const fmpz_poly_t g, int reduced, arb_srcptr b, slong n, slong count)
{
    adf_rootlist_init(M);
    fmpz_poly_set(M->g, g);
    M->reduced = reduced;
    M->n = n;
    M->count = count;
    M->ball = n > 0 ? _arb_vec_init(n) : NULL;
    if (n > 0)
        _arb_vec_set(M->ball, b, n);
}

/* ---- a snapshot, for "L untouched" ---- */

typedef struct
{
    adf_rootlist_struct raw;
    fmpz_poly_t g;
    arb_ptr ball;
    fmpz * a;
    slong n;
} snap_t;

static void
snap_take(snap_t * S, const adf_rootlist_t L)
{
    slong i;

    memcpy(&S->raw, L, sizeof(adf_rootlist_struct));
    fmpz_poly_init(S->g);
    fmpz_poly_set(S->g, L->g);
    S->n = L->n;
    S->ball = L->ball != NULL ? _arb_vec_init(L->n) : NULL;
    S->a = L->a != NULL ? _fmpz_vec_init(L->n) : NULL;
    for (i = 0; i < L->n; i++)
    {
        if (S->ball != NULL)
            arb_set(S->ball + i, L->ball + i);
        if (S->a != NULL)
            fmpz_set(S->a + i, L->a + i);
    }
}

static int
snap_same(const snap_t * S, const adf_rootlist_t L)
{
    slong i;
    int r = memcmp(&S->raw, L, sizeof(adf_rootlist_struct)) == 0 && fmpz_poly_equal(S->g, L->g);

    for (i = 0; i < S->n && r; i++)
    {
        if (S->ball != NULL)
            r = arb_equal(S->ball + i, L->ball + i);
        if (r && S->a != NULL)
            r = fmpz_equal(S->a + i, L->a + i);
    }
    return r;
}

static void
snap_clear(snap_t * S)
{
    fmpz_poly_clear(S->g);
    if (S->ball != NULL)
        _arb_vec_clear(S->ball, S->n);
    if (S->a != NULL)
        _fmpz_vec_clear(S->a, S->n);
}

/* ---- the check of a list against the oracle ---- */

/* 1 if L is the true complete list of the real roots of f at the precision prec (every property below holds,
   checked with ADF_CHECK_MSG and the label what): g the normalised polynomial of the oracle, reduced, the shape,
   n = count = the Sturm count of the oracle, each ball finite with exact end points that pass the test of P3.8
   computed here, exactly one root in each closed ball, hi_i < lo_(i+1), no root in a gap nor on the two outer
   rays, the accuracy of every ball at least max(prec, 2) or the ball exact; the predicate and both verifiers
   accept */
static int
list_is_true(const adf_rootlist_t L, const fmpz_poly_t f, slong prec, const char * what)
{
    fmpz_poly_t g;
    chain_t c;
    fmpq_t lo, hi, plo, phi, z;
    slong i, total, need = prec < 2 ? 2 : prec;
    int ok = 1, sl, sh;

    fmpz_poly_init(g);
    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_init(plo);
    fmpq_init(phi);
    fmpq_init(z);
    oracle_normalise(g, f);
    chain_init(&c, g);
    total = c.is_const ? 0 : roots_half_open(&c, z, -1, z, 1);

#define LCHECK(cond, ...)                                                                    \
    do                                                                                       \
    {                                                                                        \
        int ok_ = (cond);                                                                    \
        ADF_CHECK_MSG(ok_, __VA_ARGS__);                                                     \
        ok = ok && ok_;                                                                      \
    } while (0)

    LCHECK(fmpz_poly_equal(L->g, g), "%s: g is not the normalised polynomial", what);
    LCHECK(L->reduced == (fmpz_poly_degree(f) > 0 && fmpz_poly_degree(g) < fmpz_poly_degree(f)),
           "%s: reduced %d", what, L->reduced);
    LCHECK(adf_place_is_archimedean(L->place) && L->scope == ADF_ROOTLIST_PARTITION && L->complete == 1 &&
           L->nu == 0, "%s: shape", what);
    LCHECK(L->a == NULL && L->K == NULL && L->s == NULL && L->ua == NULL && L->ue == NULL &&
           (L->ball == NULL) == (L->n == 0), "%s: pointers", what);
    LCHECK(L->n == total && L->count == total, "%s: n = %ld, count = %ld, Sturm %ld", what, (long) L->n,
           (long) L->count, (long) total);
    for (i = 0; i < L->n && ok; i++)
    {
        const arb_struct * b = L->ball + i;
        LCHECK(arb_is_finite(b), "%s: ball %ld not finite", what, (long) i);
        if (!ok)
            break;
        ends_q(lo, hi, b);
        if (fmpq_equal(lo, hi))
            LCHECK(sign_at(g, lo) == 0, "%s: exact ball %ld is not a root", what, (long) i);
        else
        {
            sl = sign_at(g, lo);
            sh = sign_at(g, hi);
            LCHECK(fmpq_cmp(lo, hi) < 0 && sl * sh < 0, "%s: ball %ld: signs %d, %d", what, (long) i, sl, sh);
        }
        LCHECK(roots_closed(&c, g, lo, hi) == 1, "%s: ball %ld holds %ld roots", what, (long) i,
               (long) roots_closed(&c, g, lo, hi));
        LCHECK(arb_is_exact(b) || arb_rel_accuracy_bits(b) >= need, "%s: ball %ld: accuracy %ld < %ld", what,
               (long) i, (long) arb_rel_accuracy_bits(b), (long) need);
        if (i == 0)
            LCHECK(roots_open(&c, g, z, -1, lo, 0) == 0, "%s: a root below the first ball", what);
        else
        {
            LCHECK(fmpq_cmp(phi, lo) < 0, "%s: balls %ld and %ld not disjoint and increasing", what, (long) i - 1,
                   (long) i);
            LCHECK(roots_open(&c, g, phi, 0, lo, 0) == 0, "%s: a root in the gap before ball %ld", what, (long) i);
        }
        if (i == L->n - 1)
            LCHECK(roots_open(&c, g, hi, 0, z, 1) == 0, "%s: a root above the last ball", what);
        fmpq_swap(plo, lo);
        fmpq_swap(phi, hi);
    }
    LCHECK(adf_rootlist_is_canonical(L) == 1, "%s: not canonical", what);
    LCHECK(adf_rootlist_verify_entries(L, f) == 1, "%s: refused by verify_entries", what);
    LCHECK(adf_rootlist_verify_complete(L, f, 0) == 1, "%s: refused by verify_complete", what);
#undef LCHECK
    chain_clear(&c, g);
    fmpz_poly_clear(g);
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpq_clear(plo);
    fmpq_clear(phi);
    fmpq_clear(z);
    return ok;
}

/* 1 if M is a true complete list of the real roots of f by the oracle (no ADF_CHECK): g and reduced right, the
   shape of the predicate, n = count = the Sturm count, every ball of exactly one root, disjoint and increasing */
static int
oracle_complete(const adf_rootlist_t M, const fmpz_poly_t f)
{
    fmpz_poly_t g;
    chain_t c;
    fmpq_t lo, hi, phi, z;
    slong i, total;
    int ok;

    fmpz_poly_init(g);
    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_init(phi);
    fmpq_init(z);
    oracle_normalise(g, f);
    chain_init(&c, g);
    total = c.is_const ? 0 : roots_half_open(&c, z, -1, z, 1);
    ok = fmpz_poly_equal(M->g, g) &&
         M->reduced == (fmpz_poly_degree(f) > 0 && fmpz_poly_degree(g) < fmpz_poly_degree(f)) &&
         M->n == total && M->count == total && M->complete == 1 && M->scope == ADF_ROOTLIST_PARTITION;
    for (i = 0; i < M->n && ok; i++)
    {
        ok = arb_is_finite(M->ball + i);
        if (!ok)
            break;
        ends_q(lo, hi, M->ball + i);
        ok = roots_closed(&c, g, lo, hi) == 1 && (i == 0 || fmpq_cmp(phi, lo) < 0);
        fmpq_swap(phi, hi);
    }
    chain_clear(&c, g);
    fmpz_poly_clear(g);
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpq_clear(phi);
    fmpq_clear(z);
    return ok;
}

/* 1 if every ball of M holds at least one root of the normalised polynomial of f, the balls are disjoint and
   increasing, and g is right (what the entries verifier proves when it accepts: solvers P3.13(4)) */
static int
oracle_entries(const adf_rootlist_t M, const fmpz_poly_t f)
{
    fmpz_poly_t g;
    chain_t c;
    fmpq_t lo, hi, phi;
    slong i;
    int ok;

    fmpz_poly_init(g);
    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_init(phi);
    oracle_normalise(g, f);
    chain_init(&c, g);
    ok = fmpz_poly_equal(M->g, g);
    for (i = 0; i < M->n && ok; i++)
    {
        ok = arb_is_finite(M->ball + i);
        if (!ok)
            break;
        ends_q(lo, hi, M->ball + i);
        ok = roots_closed(&c, g, lo, hi) >= 1 && (i == 0 || fmpq_cmp(phi, lo) < 0);
        fmpq_swap(phi, hi);
    }
    chain_clear(&c, g);
    fmpz_poly_clear(g);
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpq_clear(phi);
    return ok;
}

/* ---- 1. the 18 cases of REAL_CASES (proto/solvers_checks.py:2613) ---- */

typedef struct
{
    const char * name;
    slong nf;                                   /* number of factors */
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

ADF_TEST(real_cases_against_the_sturm_chain_of_the_test)
{
    adf_rootlist_t L;
    fmpz_poly_t f;
    static const slong precs[] = { 2, 20, 53, 200 };
    slong i, k, good = 0, runs = 0;
    char what[160];

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    for (i = 0; i < 18; i++)
        for (k = 0; k < 4; k++)
        {
            case_poly(f, REAL_CASES + i);
            flint_sprintf(what, "%s at prec %wd", REAL_CASES[i].name, precs[k]);
            runs++;
            ADF_CHECK_MSG(adf_roots_real(L, f, precs[k]) == ADF_OK, "%s: status", what);
            ADF_CHECK_MSG(L->n == REAL_CASES[i].expect, "%s: %ld roots, expected %ld", what, (long) L->n,
                          (long) REAL_CASES[i].expect);
            good += list_is_true(L, f, precs[k], what);
        }
    printf("   REAL_CASES: %ld runs (18 polynomials, prec 2, 20, 53, 200), %ld lists true by the oracle\n",
           (long) runs, (long) good);
    ADF_CHECK(good == runs);
    fmpz_poly_clear(f);
    adf_rootlist_clear(L);
}

/* ---- 2. planted roots (check_s2_real_planted, proto/solvers_checks.py:2716) ---- */

/* f = lead * prod (den X - num)^mult * extra; checks that the list is true and that every planted root lies in
   exactly one ball and every ball holds exactly one planted root; returns the number of exact balls */
static slong
planted(const char * name, slong lead, const fmpq * r, const slong * mult, slong k, const slong * extra,
        slong elen, slong prec)
{
    adf_rootlist_t L;
    fmpz_poly_t f, t;
    fmpq_t lo, hi;
    slong i, j, m, exact = 0;
    char what[200];

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    fmpz_poly_init(t);
    fmpq_init(lo);
    fmpq_init(hi);
    fmpz_poly_set_si(f, lead);
    for (i = 0; i < k; i++)
        poly_mul_root(f, r + i, mult[i]);
    if (elen > 0)
    {
        poly_set_si(t, extra, elen);
        fmpz_poly_mul(f, f, t);
    }
    flint_sprintf(what, "planted %s at prec %wd", name, prec);
    ADF_CHECK_MSG(adf_roots_real(L, f, prec) == ADF_OK, "%s: status", what);
    ADF_CHECK_MSG(L->n == k, "%s: %ld balls for %ld roots", what, (long) L->n, (long) k);
    list_is_true(L, f, prec, what);
    for (i = 0; i < k; i++)
    {
        for (j = 0, m = 0; j < L->n; j++)
        {
            ends_q(lo, hi, L->ball + j);
            m += fmpq_cmp(lo, r + i) <= 0 && fmpq_cmp(r + i, hi) <= 0;
        }
        ADF_CHECK_MSG(m == 1, "%s: root %ld in %ld balls", what, (long) i, (long) m);
    }
    for (j = 0; j < L->n; j++)
    {
        ends_q(lo, hi, L->ball + j);
        for (i = 0, m = 0; i < k; i++)
            m += fmpq_cmp(lo, r + i) <= 0 && fmpq_cmp(r + i, hi) <= 0;
        ADF_CHECK_MSG(m == 1, "%s: ball %ld holds %ld planted roots", what, (long) j, (long) m);
        exact += arb_is_exact(L->ball + j);
    }
    fmpz_poly_clear(f);
    fmpz_poly_clear(t);
    fmpq_clear(lo);
    fmpq_clear(hi);
    adf_rootlist_clear(L);
    return exact;
}

ADF_TEST(planted_roots_repeated_large_close_and_dyadic)
{
    fmpq r[20];
    slong mult[20], i, exact, prec;
    static const slong x2p1[] = { 1, 0, 1 }, x2px1[] = { 1, 1, 1 };
    static const slong precs[] = { 2, 20, 53 };
    slong kp;

    for (i = 0; i < 20; i++)
        fmpq_init(r + i);
    for (kp = 0; kp < 3; kp++)
    {
        prec = precs[kp];
        /* repeated roots: (X - 1)^3 (X + 2)^2 (2 X - 1) (X^2 + 1) */
        fmpq_set_si(r + 0, 1, 1);
        fmpq_set_si(r + 1, -2, 1);
        fmpq_set_si(r + 2, 1, 2);
        mult[0] = 3; mult[1] = 2; mult[2] = 1;
        planted("repeated", 3, r, mult, 3, x2p1, 3, prec);
        /* roots of size 10^30: 10^30, -10^30, 1, -1/10^20 */
        fmpz_set_ui(fmpq_numref(r + 0), 10);
        fmpz_pow_ui(fmpq_numref(r + 0), fmpq_numref(r + 0), 30);
        fmpz_one(fmpq_denref(r + 0));
        fmpq_neg(r + 1, r + 0);
        fmpq_set_si(r + 2, 1, 1);
        fmpz_set_si(fmpq_numref(r + 3), -1);
        fmpz_set_ui(fmpq_denref(r + 3), 10);
        fmpz_pow_ui(fmpq_denref(r + 3), fmpq_denref(r + 3), 20);
        mult[0] = mult[1] = mult[2] = mult[3] = 1;
        planted("10^30", -5, r, mult, 4, x2px1, 3, prec);
        /* two roots 2^-40 apart: 1 and 1 + 2^-40, the second double */
        fmpq_set_si(r + 0, 1, 1);
        fmpz_one(fmpq_numref(r + 1));
        fmpz_mul_2exp(fmpq_numref(r + 1), fmpq_numref(r + 1), 40);
        fmpz_add_ui(fmpq_numref(r + 1), fmpq_numref(r + 1), 1);
        fmpz_one(fmpq_denref(r + 1));
        fmpz_mul_2exp(fmpq_denref(r + 1), fmpq_denref(r + 1), 40);
        mult[0] = 1; mult[1] = 2;
        planted("2^-40 apart", 1, r, mult, 2, NULL, 0, prec);
        /* X - 10^400. The pair (X - 10^400)(X - 10^400 - 1) of check_s2_real_planted was a line of the vectors
           (test 8) only: arb_fmpz_poly_complex_roots took about 28 s for it at every prec (its balls carried
           midpoints of 2^21 bits). Lane r-slice1 (the candidates of src/roots_real.c) adds it here, below */
        fmpz_set_ui(fmpq_numref(r + 0), 10);
        fmpz_pow_ui(fmpq_numref(r + 0), fmpq_numref(r + 0), 400);
        fmpz_one(fmpq_denref(r + 0));
        mult[0] = 1;
        planted("X - 10^400", 1, r, mult, 1, NULL, 0, prec);
        /* lane r-slice1: (X - 10^400)(X - 10^400 - 1)^2 */
        fmpz_add_ui(fmpq_numref(r + 1), fmpq_numref(r + 0), 1);
        fmpz_one(fmpq_denref(r + 1));
        mult[1] = 2;
        planted("(X - 10^400)(X - 10^400 - 1)^2", 1, r, mult, 2, NULL, 0, prec);
        /* Wilkinson's polynomial of degree 20 */
        for (i = 0; i < 20; i++)
        {
            fmpq_set_si(r + i, i + 1, 1);
            mult[i] = 1;
        }
        planted("Wilkinson 20", 1, r, mult, 20, NULL, 0, prec);
        /* a root at 0 and at dyadic points: 0 (double), 1/2, -3/4, 5/8, 1/2^30; and 1/3 */
        fmpq_set_si(r + 0, 0, 1);
        fmpq_set_si(r + 1, 1, 2);
        fmpq_set_si(r + 2, -3, 4);
        fmpq_set_si(r + 3, 5, 8);
        fmpz_one(fmpq_numref(r + 4));
        fmpz_one(fmpq_denref(r + 4));
        fmpz_mul_2exp(fmpq_denref(r + 4), fmpq_denref(r + 4), 30);
        fmpq_set_si(r + 5, 1, 3);
        mult[0] = 2; mult[1] = 1; mult[2] = 1; mult[3] = 2; mult[4] = 1; mult[5] = 1;
        exact = planted("0 and dyadic", 7, r, mult, 6, NULL, 0, prec);
        printf("   planted, prec %ld: 0, 1/2, -3/4, 5/8, 2^-30, 1/3: %ld of 6 balls exact\n", (long) prec,
               (long) exact);
        ADF_CHECK(exact >= 1);                  /* at least the root 0 (FLINT removes the factor X exactly) */
    }
    for (i = 0; i < 20; i++)
        fmpq_clear(r + i);
}

/* ---- 3. precision ---- */

ADF_TEST(precision_accuracy_below_2_and_nesting)
{
    static const slong precs[] = { 2, 10, 53, 200, 2000 };
    static const slong low[] = { 1, 0, -5, WORD_MIN };
    adf_rootlist_t L[5], M;
    fmpz_poly_t f;
    fmpq_t lo0, hi0, lo1, hi1;
    slong i, k, j, nest = 0;
    char what[160];
    static const slong c1[] = { -2, 0, 1 };

    for (k = 0; k < 5; k++)
        adf_rootlist_init(L[k]);
    adf_rootlist_init(M);
    fmpz_poly_init(f);
    fmpq_init(lo0);
    fmpq_init(hi0);
    fmpq_init(lo1);
    fmpq_init(hi1);
    for (i = 0; i < 6; i++)
    {
        if (i < 5)
            case_poly(f, REAL_CASES + (i == 0 ? 1 : i == 1 ? 10 : i == 2 ? 11 : i == 3 ? 12 : 14));
        else
        {
            /* X - 10^400 */
            fmpz_poly_zero(f);
            fmpz_poly_set_coeff_si(f, 1, 1);
            fmpz_set_ui(f->coeffs, 10);
            fmpz_pow_ui(f->coeffs, f->coeffs, 400);
            fmpz_neg(f->coeffs, f->coeffs);
        }
        for (k = 0; k < 5; k++)
        {
            flint_sprintf(what, "precision case %wd at prec %wd", i, precs[k]);
            ADF_CHECK_MSG(adf_roots_real(L[k], f, precs[k]) == ADF_OK, "%s: status", what);
            list_is_true(L[k], f, precs[k], what);
            for (j = 0; j < L[k]->n; j++)
                ADF_CHECK_MSG(arb_is_exact(L[k]->ball + j) || arb_rel_accuracy_bits(L[k]->ball + j) >= precs[k],
                              "%s: ball %ld accuracy %ld", what, (long) j,
                              (long) arb_rel_accuracy_bits(L[k]->ball + j));
            /* nested for growing prec: every ball at precs[k] inside the ball of the same root at precs[k-1] */
            if (k > 0 && L[k]->n == L[k - 1]->n)
                for (j = 0; j < L[k]->n; j++)
                {
                    ends_q(lo0, hi0, L[k - 1]->ball + j);
                    ends_q(lo1, hi1, L[k]->ball + j);
                    ADF_CHECK_MSG(fmpq_cmp(lo0, lo1) <= 0 && fmpq_cmp(hi1, hi0) <= 0, "%s: ball %ld not nested",
                                  what, (long) j);
                    nest++;
                }
        }
        /* a prec below 2 is taken as 2: the same list as at prec 2 */
        for (k = 0; k < 4; k++)
        {
            flint_sprintf(what, "precision case %wd at prec %wd", i, low[k]);
            ADF_CHECK_MSG(adf_roots_real(M, f, low[k]) == ADF_OK, "%s: status", what);
            list_is_true(M, f, low[k], what);
            ADF_CHECK_MSG(M->n == L[0]->n, "%s: n", what);
            for (j = 0; j < M->n && j < L[0]->n; j++)
                ADF_CHECK_MSG(arb_equal(M->ball + j, L[0]->ball + j), "%s: ball %ld differs from prec 2", what,
                              (long) j);
        }
    }
    printf("   precision: 6 polynomials at prec 2, 10, 53, 200, 2000 and 1, 0, -5, WORD_MIN; %ld nested pairs\n",
           (long) nest);
    /* the largest precision is accepted: X^2 - 2 at ADF_ROOTS_REAL_PREC_MAX bits */
    poly_set_si(f, c1, 3);
    ADF_CHECK(adf_roots_real(M, f, ADF_ROOTS_REAL_PREC_MAX) == ADF_OK);
    list_is_true(M, f, ADF_ROOTS_REAL_PREC_MAX, "X^2 - 2 at ADF_ROOTS_REAL_PREC_MAX");
    for (k = 0; k < 5; k++)
        adf_rootlist_clear(L[k]);
    adf_rootlist_clear(M);
    fmpz_poly_clear(f);
    fmpq_clear(lo0);
    fmpq_clear(hi0);
    fmpq_clear(lo1);
    fmpq_clear(hi1);
}

/* ---- 4. the verifiers ---- */

enum { K_REMOVED, K_MERGED, K_MOVED, K_WIDENED, K_COUNT, K_N, K_G, K_DOUBLED, K_NUM };
static const char * KIND[K_NUM] = { "ball removed", "two balls merged", "ball moved off its root",
                                    "ball widened over two roots", "false count", "false n",
                                    "g replaced by f", "one root twice" };

/* one changed list M of kind k: the verifiers must refuse it; the entries verifier may accept only what it proves
   (oracle_entries), the complete verifier only a true complete list (oracle_complete: never, for a changed
   list) */
static void
changed(adf_rootlist_t M, const fmpz_poly_t f, int k, slong * built, slong * refused, slong * by_entries,
        const char * name)
{
    int e = adf_rootlist_verify_entries(M, f);
    int c = adf_rootlist_verify_complete(M, f, 0);

    built[k]++;
    ADF_CHECK_MSG(!oracle_complete(M, f), "%s, %s: the changed list is a true list (a defect of the test)", name,
                  KIND[k]);
    ADF_CHECK_MSG(!e || oracle_entries(M, f), "%s, %s: accepted by verify_entries without proof", name, KIND[k]);
    ADF_CHECK_MSG(c == 0, "%s, %s: accepted by verify_complete", name, KIND[k]);
    ADF_CHECK_MSG(!c || e, "%s, %s: complete without entries", name, KIND[k]);
    refused[k] += c == 0;
    by_entries[k] += e == 0;
    adf_rootlist_clear(M);
}

ADF_TEST(verifiers_accept_every_result_and_refuse_changed_lists)
{
    adf_rootlist_t L, M;
    fmpz_poly_t f, g;
    arb_ptr b;
    arb_t t;
    fmpq_t lo, hi, w, x, y;
    slong built[K_NUM] = { 0 }, refused[K_NUM] = { 0 }, by_entries[K_NUM] = { 0 };
    slong i, j, n, accepted = 0;
    chain_t c;

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    fmpz_poly_init(g);
    arb_init(t);
    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_init(w);
    fmpq_init(x);
    fmpq_init(y);
    for (i = 0; i < 18; i++)
    {
        const char * name = REAL_CASES[i].name;
        case_poly(f, REAL_CASES + i);
        ADF_CHECK(adf_roots_real(L, f, 30) == ADF_OK);
        n = L->n;
        accepted += adf_rootlist_verify_entries(L, f) && adf_rootlist_verify_complete(L, f, 0) &&
                    adf_rootlist_verify_complete(L, f, -1) && adf_rootlist_verify_complete(L, f, WORD_MAX);
        b = _arb_vec_init(n + 1);
        /* removed: each ball in turn, count = n - 1 */
        for (j = 0; j < n; j++)
        {
            slong q, r = 0;
            for (q = 0; q < n; q++)
                if (q != j)
                    arb_set(b + r++, L->ball + q);
            make_list(M, L->g, L->reduced, b, n - 1, n - 1);
            changed(M, f, K_REMOVED, built, refused, by_entries, name);
        }
        /* merged: the balls 0 and 1 replaced by one ball over both, count n - 1 and n */
        if (n >= 2)
        {
            arb_union(b + 0, L->ball + 0, L->ball + 1, 200);
            for (j = 2; j < n; j++)
                arb_set(b + j - 1, L->ball + j);
            make_list(M, L->g, L->reduced, b, n - 1, n - 1);
            changed(M, f, K_MERGED, built, refused, by_entries, name);
            make_list(M, L->g, L->reduced, b, n - 1, n);
            changed(M, f, K_MERGED, built, refused, by_entries, name);
        }
        /* moved: ball 0 replaced by [hi + w/8, hi + w/4] (w its width, 2^-20 if exact), when that holds no root
           and lies below ball 1 */
        if (n >= 1)
        {
            ends_q(lo, hi, L->ball + 0);
            fmpq_sub(w, hi, lo);
            if (fmpq_is_zero(w))
                fmpq_set_si(w, 1, 1 << 20);
            fmpq_div_2exp(x, w, 3);
            fmpq_add(x, hi, x);
            fmpq_div_2exp(y, w, 2);
            fmpq_add(y, hi, y);
            ball_from_q(b + 0, x, y, 400);
            for (j = 1; j < n; j++)
                arb_set(b + j, L->ball + j);
            make_list(M, L->g, L->reduced, b, n, n);
            changed(M, f, K_MOVED, built, refused, by_entries, name);
        }
        /* widened: ball 0 widened over the roots of the balls 0 and 1, ball 1 kept */
        if (n >= 2)
        {
            arb_union(b + 0, L->ball + 0, L->ball + 1, 200);
            for (j = 1; j < n; j++)
                arb_set(b + j, L->ball + j);
            make_list(M, L->g, L->reduced, b, n, n);
            changed(M, f, K_WIDENED, built, refused, by_entries, name);
        }
        /* false count: count n + 1, and n - 1 when n >= 1 */
        make_list(M, L->g, L->reduced, L->ball, n, n + 1);
        changed(M, f, K_COUNT, built, refused, by_entries, name);
        if (n >= 1)
        {
            make_list(M, L->g, L->reduced, L->ball, n, n - 1);
            changed(M, f, K_COUNT, built, refused, by_entries, name);
        }
        /* false n: the last ball dropped, count kept */
        if (n >= 1)
        {
            make_list(M, L->g, L->reduced, L->ball, n - 1, n);
            changed(M, f, K_N, built, refused, by_entries, name);
        }
        /* g replaced by f (or by f made primitive) when they differ */
        oracle_normalise(g, f);
        if (!fmpz_poly_equal(f, g))
        {
            make_list(M, f, L->reduced, L->ball, n, n);
            changed(M, f, K_G, built, refused, by_entries, name);
        }
        /* one root twice: ball 0 in the places 0 and 1, ball 1 missing */
        if (n >= 2)
        {
            arb_set(b + 0, L->ball + 0);
            arb_set(b + 1, L->ball + 0);
            for (j = 2; j < n; j++)
                arb_set(b + j, L->ball + j);
            make_list(M, L->g, L->reduced, b, n, n);
            changed(M, f, K_DOUBLED, built, refused, by_entries, name);
        }
        _arb_vec_clear(b, n + 1);
    }
    for (i = 0; i < K_NUM; i++)
    {
        printf("   %-28s %3ld changed lists, %3ld refused (%3ld of them by verify_entries)\n", KIND[i],
               (long) built[i], (long) refused[i], (long) by_entries[i]);
        ADF_CHECK_MSG(built[i] > 0 && refused[i] == built[i], "kind %s", KIND[i]);
    }
    ADF_CHECK(accepted == 18);

    /* [0, 4] for (X - 1)(X - 2)(X - 3): passes the entries verifier, fails the complete one (solvers P3.8(3),
       P3.13, example (b)) */
    case_poly(f, REAL_CASES + 0);
    oracle_normalise(g, f);
    ball_set_2exp(t, 2, 0, 2, 0);
    make_list(M, g, 0, t, 1, 1);
    ends_q(lo, hi, t);
    ADF_CHECK(fmpq_cmp_si(lo, 0) == 0 && fmpq_cmp_si(hi, 4) == 0);
    ADF_CHECK(adf_rootlist_is_canonical(M) == 1);
    ADF_CHECK(adf_rootlist_verify_entries(M, f) == 1);
    ADF_CHECK(adf_rootlist_verify_complete(M, f, 0) == 0);
    M->count = 3;
    ADF_CHECK(adf_rootlist_verify_complete(M, f, 0) == 0);
    adf_rootlist_clear(M);

    /* a root at an end point of a ball that is not a point: [1, 1 + 6/1024] for X - 1 is refused (the test of
       P3.8 asks g(lo) g(hi) < 0); the exact ball [1, 1] is accepted; an exact ball at 1 + 2^-20 is refused */
    fmpz_poly_zero(f);
    fmpz_poly_set_coeff_si(f, 0, -1);
    fmpz_poly_set_coeff_si(f, 1, 1);
    ball_set_2exp(t, 1027, -10, 3, -10);
    ends_q(lo, hi, t);
    ADF_CHECK(fmpq_cmp_si(lo, 1) == 0);
    make_list(M, f, 0, t, 1, 1);
    ADF_CHECK(adf_rootlist_verify_entries(M, f) == 0);
    ADF_CHECK(adf_rootlist_verify_complete(M, f, 0) == 0);
    adf_rootlist_clear(M);
    arb_set_si(t, 1);
    make_list(M, f, 0, t, 1, 1);
    ADF_CHECK(adf_rootlist_verify_entries(M, f) == 1);
    ADF_CHECK(adf_rootlist_verify_complete(M, f, 0) == 1);
    adf_rootlist_clear(M);
    ball_set_2exp(t, (1 << 20) + 1, -20, 0, 0);
    make_list(M, f, 0, t, 1, 1);
    ADF_CHECK(adf_rootlist_verify_entries(M, f) == 0);
    adf_rootlist_clear(M);
    /* the same root at the right end point: [1 - 6/1024, 1] */
    ball_set_2exp(t, 1021, -10, 3, -10);
    make_list(M, f, 0, t, 1, 1);
    ADF_CHECK(adf_rootlist_verify_entries(M, f) == 0);
    adf_rootlist_clear(M);

    /* the verifiers refuse f = 0, and a real list for the wrong f */
    case_poly(f, REAL_CASES + 1);
    ADF_CHECK(adf_roots_real(L, f, 53) == ADF_OK);
    fmpz_poly_zero(g);
    ADF_CHECK(adf_rootlist_verify_entries(L, g) == 0 && adf_rootlist_verify_complete(L, g, 0) == 0);
    case_poly(g, REAL_CASES + 13);
    ADF_CHECK(adf_rootlist_verify_entries(L, g) == 0 && adf_rootlist_verify_complete(L, g, 0) == 0);
    /* a false flag reduced */
    L->reduced = 1;
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0 && adf_rootlist_verify_complete(L, f, 0) == 0);
    L->reduced = 0;
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1);

    /* the chain of the oracle agrees with the expected counts (a check of the test itself) */
    for (i = 0; i < 18; i++)
    {
        case_poly(f, REAL_CASES + i);
        oracle_normalise(g, f);
        chain_init(&c, g);
        ADF_CHECK_MSG((c.is_const ? 0 : roots_half_open(&c, x, -1, x, 1)) == REAL_CASES[i].expect, "%s",
                      REAL_CASES[i].name);
        chain_clear(&c, g);
    }
    arb_clear(t);
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpq_clear(w);
    fmpq_clear(x);
    fmpq_clear(y);
    fmpz_poly_clear(f);
    fmpz_poly_clear(g);
    adf_rootlist_clear(L);
}

/* ---- 5. the predicate at the real place ---- */

ADF_TEST(predicate_at_the_real_place)
{
    adf_rootlist_t L, M;
    fmpz_poly_t f;
    arb_t t;
    arb_ptr b;

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    arb_init(t);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 1);                   /* the init value */
    case_poly(f, REAL_CASES + 0);
    ADF_CHECK(adf_roots_real(L, f, 53) == ADF_OK && L->n == 3);
    if (L->n != 3)
    {
        arb_clear(t);
        fmpz_poly_clear(f);
        adf_rootlist_clear(L);
        return;
    }
    ADF_CHECK(adf_rootlist_is_canonical(L) == 1);
    /* the shape */
    L->count = 2;
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0 && adf_rootlist_verify_entries(L, f) == 0);
    L->count = 3;
    L->complete = 0;
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0 && adf_rootlist_verify_entries(L, f) == 0);
    L->complete = 1;
    L->scope = ADF_ROOTLIST_SEED;
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0 && adf_rootlist_verify_entries(L, f) == 0);
    L->scope = ADF_ROOTLIST_PARTITION;
    L->reduced = 2;
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0 && adf_rootlist_verify_entries(L, f) == 0);
    L->reduced = 0;
    ADF_CHECK(adf_rootlist_is_canonical(L) == 1 && adf_rootlist_verify_entries(L, f) == 1);
    /* the order: two balls exchanged */
    arb_swap(L->ball + 0, L->ball + 1);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0 && adf_rootlist_verify_entries(L, f) == 0);
    arb_swap(L->ball + 0, L->ball + 1);
    /* balls that touch: hi_0 = lo_1 is not hi_0 < lo_1 */
    b = _arb_vec_init(2);
    ball_set_2exp(b + 0, 3, -1, 1, -1);          /* [1, 2] */
    ball_set_2exp(b + 1, 5, -1, 1, -1);          /* [2, 3] */
    make_list(M, L->g, 0, b, 2, 2);
    ADF_CHECK(adf_rootlist_is_canonical(M) == 0);
    adf_rootlist_clear(M);
    /* not finite, and not of admissible size (roots.h, "Real balls"): refused, no abort */
    arb_pos_inf(b + 0);
    make_list(M, L->g, 0, b, 1, 1);
    ADF_CHECK(adf_rootlist_is_canonical(M) == 0 && adf_rootlist_verify_entries(M, f) == 0 &&
              adf_rootlist_verify_complete(M, f, 0) == 0);
    adf_rootlist_clear(M);
    arb_indeterminate(b + 0);
    make_list(M, L->g, 0, b, 1, 1);
    ADF_CHECK(adf_rootlist_is_canonical(M) == 0 && adf_rootlist_verify_entries(M, f) == 0);
    adf_rootlist_clear(M);
    arf_set_si(arb_midref(b + 0), 1);
    mag_set_ui_2exp_si(arb_radref(b + 0), 1, -ADF_ROOTS_BITS_MAX);     /* radius 2^-BITS_MAX: not admissible */
    make_list(M, L->g, 0, b, 1, 1);
    ADF_CHECK(adf_rootlist_is_canonical(M) == 0 && adf_rootlist_verify_entries(M, f) == 0);
    adf_rootlist_clear(M);
    mag_set_ui_2exp_si(arb_radref(b + 0), 1, -ADF_ROOTS_BITS_MAX + 1);  /* 2^(1 - BITS_MAX): admissible */
    make_list(M, L->g, 0, b, 1, 1);
    /* the ball 1 +- 2^(1 - BITS_MAX) isolates the root 1 (end points with 2^24-bit denominators): the entries
       verifier accepts it, the complete one refuses the list of one ball */
    ADF_CHECK(adf_rootlist_is_canonical(M) == 1 && adf_rootlist_verify_entries(M, f) == 1 &&
              adf_rootlist_verify_complete(M, f, 0) == 0);
    adf_rootlist_clear(M);
    arf_set_si_2exp_si(arb_midref(b + 0), 1, ADF_ROOTS_BITS_MAX);       /* midpoint 2^BITS_MAX */
    mag_one(arb_radref(b + 0));
    make_list(M, L->g, 0, b, 1, 1);
    ADF_CHECK(adf_rootlist_is_canonical(M) == 0 && adf_rootlist_verify_entries(M, f) == 0);
    adf_rootlist_clear(M);
    arf_set_si_2exp_si(arb_midref(b + 0), 1, -ADF_ROOTS_BITS_MAX);      /* midpoint 2^-BITS_MAX */
    mag_zero(arb_radref(b + 0));
    make_list(M, L->g, 0, b, 1, 1);
    ADF_CHECK(adf_rootlist_is_canonical(M) == 0 && adf_rootlist_verify_entries(M, f) == 0);
    adf_rootlist_clear(M);
    arf_set_si_2exp_si(arb_midref(b + 0), 1, ADF_ROOTS_BITS_MAX);
    mag_set_ui_2exp_si(arb_radref(b + 0), 1, ADF_ROOTS_BITS_MAX + 5);  /* radius 2^(BITS_MAX + 5) */
    make_list(M, L->g, 0, b, 1, 1);
    ADF_CHECK(adf_rootlist_is_canonical(M) == 0 && adf_rootlist_verify_entries(M, f) == 0);
    adf_rootlist_clear(M);
    _arb_vec_clear(b, 2);
    /* the pointers of a prime at the real place, nu > 0 */
    L->nu = 1;
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0 && adf_rootlist_verify_entries(L, f) == 0);
    L->nu = 0;
    L->K = flint_malloc(3 * sizeof(slong));
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0 && adf_rootlist_verify_entries(L, f) == 0);
    flint_free(L->K);
    L->K = NULL;
    ADF_CHECK(adf_rootlist_is_canonical(L) == 1);
    /* g that is not normalised */
    fmpz_poly_scalar_mul_si(L->g, L->g, 2);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0);
    arb_clear(t);
    fmpz_poly_clear(f);
    adf_rootlist_clear(L);
}

/* ---- 6. steps 5 to 7 of Algorithm RR on given candidates (the hidden function) ---- */

ADF_TEST(finish_widening_accuracy_after_widening_and_the_count)
{
    adf_rootlist_t L;
    fmpz_poly_t f;
    arb_ptr b;
    arb_t t;
    snap_t S;
    fmpq_t lo, hi;

    adf_rootlist_init(L);
    fmpz_poly_init(f);
    arb_init(t);
    fmpq_init(lo);
    fmpq_init(hi);
    b = _arb_vec_init(3);
    /* X - 1, prec 8, the candidate 1 + 3/1024 +- 3/1024 (the root at its left end point): the test of P3.8 fails,
       the widened ball 1 + 3/1024 +- 3/512 passes it but has accuracy 7 < 8: NOT_DETERMINED (solvers P3.10(4),
       decision S-D19), L untouched */
    fmpz_poly_set_coeff_si(f, 0, -1);
    fmpz_poly_set_coeff_si(f, 1, 1);
    ball_set_2exp(b + 0, 1027, -10, 3, -10);
    ADF_CHECK(arb_rel_accuracy_bits(b + 0) == 8);
    arb_set(t, b + 0);
    mag_mul_2exp_si(arb_radref(t), arb_radref(t), 1);
    ADF_CHECK(arb_rel_accuracy_bits(t) == 7);
    snap_take(&S, L);
    ADF_CHECK(adf_roots_real_finish(L, f, 1, b, 1, 8) == ADF_NOT_DETERMINED);
    ADF_CHECK(snap_same(&S, L));
    snap_clear(&S);
    /* the same at prec 7: OK, the stored ball is the widened one */
    ADF_CHECK(adf_roots_real_finish(L, f, 1, b, 1, 7) == ADF_OK);
    ADF_CHECK(L->n == 1 && arb_equal(L->ball + 0, t));
    list_is_true(L, f, 7, "finish: X - 1, widened at prec 7");
    /* X - 1, prec 8, the candidate 1 + 2^-12 +- 2^-12: widened to 1 + 2^-12 +- 2^-11, accuracy 10: OK */
    ball_set_2exp(b + 0, 4097, -12, 1, -12);
    ADF_CHECK(adf_roots_real_finish(L, f, 1, b, 1, 8) == ADF_OK);
    ball_set_2exp(t, 4097, -12, 1, -11);
    ADF_CHECK(L->n == 1 && arb_equal(L->ball + 0, t));
    list_is_true(L, f, 8, "finish: X - 1, widened at prec 8");
    /* an exact candidate that is not a root: widened by 2^-prec. X - 1 at 1 + 2^-20, prec 8: 1 + 2^-20 +- 2^-8
       has accuracy 7: NOT_DETERMINED. X - 3 at 3 + 2^-20, prec 8: accuracy 8: OK */
    ball_set_2exp(b + 0, (1 << 20) + 1, -20, 0, 0);
    ADF_CHECK(adf_roots_real_finish(L, f, 1, b, 1, 8) == ADF_NOT_DETERMINED);
    fmpz_poly_set_coeff_si(f, 0, -3);
    ball_set_2exp(b + 0, 3 * (1 << 20) + 1, -20, 0, 0);
    ADF_CHECK(adf_roots_real_finish(L, f, 1, b, 1, 8) == ADF_OK);
    ball_set_2exp(t, 3 * (1 << 20) + 1, -20, 1, -8);
    ADF_CHECK(L->n == 1 && arb_equal(L->ball + 0, t));
    list_is_true(L, f, 8, "finish: X - 3, exact candidate widened");
    /* a candidate far from the root: the widening does not help: NOT_DETERMINED */
    ball_set_2exp(b + 0, 5, 0, 1, -3);
    ADF_CHECK(adf_roots_real_finish(L, f, 1, b, 1, 8) == ADF_NOT_DETERMINED);
    /* X^2 - 2 with its true balls: OK with the count 2; the count 3, or 1 with one ball dropped, or the balls
       exchanged: NOT_DETERMINED, L untouched */
    fmpz_poly_zero(f);
    fmpz_poly_set_coeff_si(f, 0, -2);
    fmpz_poly_set_coeff_si(f, 2, 1);
    ADF_CHECK(adf_roots_real(L, f, 53) == ADF_OK && L->n == 2);
    if (L->n == 2)
    {
        arb_set(b + 0, L->ball + 0);
        arb_set(b + 1, L->ball + 1);
    }
    ADF_CHECK(adf_roots_real_finish(L, f, 2, b, 2, 53) == ADF_OK && L->n == 2);
    snap_take(&S, L);
    ADF_CHECK(adf_roots_real_finish(L, f, 3, b, 2, 53) == ADF_NOT_DETERMINED);
    ADF_CHECK(adf_roots_real_finish(L, f, 1, b, 2, 53) == ADF_NOT_DETERMINED);
    ADF_CHECK(adf_roots_real_finish(L, f, 2, b, 1, 53) == ADF_NOT_DETERMINED);
    ADF_CHECK(adf_roots_real_finish(L, f, 2, b + 1, 1, 53) == ADF_NOT_DETERMINED);
    arb_swap(b + 0, b + 1);
    ADF_CHECK(adf_roots_real_finish(L, f, 2, b, 2, 53) == ADF_NOT_DETERMINED);
    /* the same ball twice */
    arb_set(b + 1, b + 0);
    ADF_CHECK(adf_roots_real_finish(L, f, 2, b, 2, 53) == ADF_NOT_DETERMINED);
    /* a candidate not of admissible size: LIMIT */
    arf_set_si(arb_midref(b + 0), 1);
    mag_set_ui_2exp_si(arb_radref(b + 0), 1, ADF_ROOTS_BITS_MAX + 5);
    ADF_CHECK(adf_roots_real_finish(L, f, 1, b, 1, 53) == ADF_LIMIT);
    arb_indeterminate(b + 0);
    ADF_CHECK(adf_roots_real_finish(L, f, 1, b, 1, 53) == ADF_LIMIT);
    ADF_CHECK(snap_same(&S, L));
    snap_clear(&S);
    _arb_vec_clear(b, 3);
    arb_clear(t);
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpz_poly_clear(f);
    adf_rootlist_clear(L);
}

/* ---- 7. statuses, untouched outputs, aliasing, the accessor ---- */

ADF_TEST(statuses_constants_untouched_and_aliasing)
{
    adf_rootlist_t L, P;
    fmpz_poly_t f, h;
    snap_t S;
    arb_t x;
    adf_place_t v;
    slong i;
    static const slong consts[] = { 1, -1, 7, -12 };
    static const slong c3[] = { 6, -2, -3, 1 };      /* (X^2 - 2)(X - 3) */

    adf_rootlist_init(L);
    adf_rootlist_init(P);
    fmpz_poly_init(f);
    fmpz_poly_init(h);
    arb_init(x);
    /* a list to keep untouched */
    poly_set_si(f, c3, 4);
    ADF_CHECK(adf_roots_real(L, f, 64) == ADF_OK && L->n == 3);
    list_is_true(L, f, 64, "(X^2 - 2)(X - 3)");
    /* DOMAIN: f = 0 */
    fmpz_poly_zero(h);
    snap_take(&S, L);
    ADF_CHECK(adf_roots_real(L, h, 64) == ADF_DOMAIN);
    ADF_CHECK(snap_same(&S, L));
    /* LIMIT: prec above ADF_ROOTS_REAL_PREC_MAX, before any allocation */
    ADF_CHECK(adf_roots_real(L, f, ADF_ROOTS_REAL_PREC_MAX + 1) == ADF_LIMIT);
    ADF_CHECK(adf_roots_real(L, f, WORD_MAX) == ADF_LIMIT);
    ADF_CHECK(snap_same(&S, L));
    snap_clear(&S);
    /* constants: OK, the empty list, count 0, g = 1 */
    for (i = 0; i < 4; i++)
    {
        fmpz_poly_set_si(h, consts[i]);
        ADF_CHECK(adf_roots_real(P, h, 53) == ADF_OK);
        ADF_CHECK(P->n == 0 && P->count == 0 && P->ball == NULL && P->complete == 1 && P->reduced == 0 &&
                  fmpz_poly_is_one(P->g) && adf_place_is_archimedean(P->place));
        list_is_true(P, h, 53, "constant");
    }
    fmpz_set_ui(h->coeffs, 10);
    fmpz_pow_ui(h->coeffs, h->coeffs, 30);
    ADF_CHECK(adf_roots_real(P, h, 53) == ADF_OK && P->n == 0);
    /* aliasing: f = L->g */
    fmpz_poly_set(h, L->g);
    ADF_CHECK(adf_roots_real(P, h, 64) == ADF_OK);
    ADF_CHECK(adf_roots_real(L, L->g, 64) == ADF_OK);
    ADF_CHECK(L->n == P->n);
    for (i = 0; i < L->n && i < P->n; i++)
        ADF_CHECK(arb_equal(L->ball + i, P->ball + i));
    list_is_true(L, f, 64, "aliasing f = L->g");
    /* a list at a prime becomes a list at the real place, f = L->g */
    ADF_CHECK(adf_place_prime(&v, 7) == ADF_OK);
    ADF_CHECK(adf_roots_padic(P, f, v, 5, 8) == ADF_OK && P->n == 3 && P->a != NULL);
    ADF_CHECK(adf_roots_real(P, P->g, 64) == ADF_OK);
    ADF_CHECK(P->a == NULL && P->K == NULL && P->s == NULL);
    list_is_true(P, f, 64, "from a list at 7");
    /* the accessor */
    for (i = 0; i < P->n; i++)
    {
        ADF_CHECK(adf_rootlist_get_arb(x, P, i) == 1 && arb_equal(x, P->ball + i));
        ADF_CHECK(adf_rootlist_get_arb(P->ball + i, P, i) == 1 && arb_equal(x, P->ball + i));
    }
    arb_set_si(x, 42);
    ADF_CHECK(adf_rootlist_get_arb(x, P, -1) == 0 && adf_rootlist_get_arb(x, P, P->n) == 0);
    ADF_CHECK(adf_rootlist_get_arb(x, P, WORD_MAX) == 0 && arb_equal_si(x, 42));
    ADF_CHECK(adf_roots_padic(P, f, v, 5, 8) == ADF_OK);
    ADF_CHECK(adf_rootlist_get_arb(x, P, 0) == 0 && arb_equal_si(x, 42));
    /* the other accessors at the real place */
    ADF_CHECK(adf_rootlist_length(L) == 3 && adf_rootlist_is_complete(L) == 1 &&
              adf_rootlist_unresolved_length(L) == 0 && adf_place_is_archimedean(adf_rootlist_place(L)) &&
              adf_rootlist_scope(L) == ADF_ROOTLIST_PARTITION);
    {
        fmpz_t a;
        slong K = -7, s = -7;
        fmpz_init_set_si(a, 5);
        ADF_CHECK(adf_rootlist_get_cert(a, &K, &s, L, 0) == 0 && fmpz_equal_si(a, 5) && K == -7);
        fmpz_clear(a);
    }
    arb_clear(x);
    fmpz_poly_clear(f);
    fmpz_poly_clear(h);
    adf_rootlist_clear(L);
    adf_rootlist_clear(P);
}

/* ---- 8. the vectors of the reference ---- */

static int
read_fmpz(fmpz_t x, const jsonl_value * v)
{
    const char * t;

    if (!jsonl_int_text_or_string(v, &t, NULL))
        return 0;
    return fmpz_set_str(x, t, 10) == 0;
}

static int
read_si(slong * x, const jsonl_value * v)
{
    fmpz_t t;
    int ok;

    fmpz_init(t);
    ok = read_fmpz(t, v) && fmpz_fits_si(t);
    if (ok)
        *x = fmpz_get_si(t);
    fmpz_clear(t);
    return ok;
}

static int
read_poly(fmpz_poly_t f, const jsonl_value * v)
{
    fmpz_t c;
    size_t i;
    int ok = 1;

    fmpz_init(c);
    fmpz_poly_zero(f);
    for (i = 0; i < jsonl_size(v) && ok; i++)
    {
        ok = read_fmpz(c, jsonl_at(v, i, NULL));
        fmpz_poly_set_coeff_fmpz(f, (slong) i, c);
    }
    fmpz_clear(c);
    return ok;
}

ADF_TEST(vectors_s2_slice3_every_line)
{
    const char * path = "tests/ref/vectors/s2-slice3/real.jsonl";
    jsonl_file * file;
    jsonl_error_t err;
    const jsonl_value * rec, * v, * enc;
    adf_rootlist_t L;
    fmpz_poly_t f, g;
    fmpq_t clo, chi, elo, ehi;
    fmpz_t t;
    slong prec, red, count, n_ok = 0, n_dom = 0, n_roots = 0, i, j;
    size_t k, len;
    const char * st;
    int ok;

    if (!jsonl_open(path, &file, &err))
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    adf_rootlist_init(L);
    fmpz_poly_init(f);
    fmpz_poly_init(g);
    fmpq_init(clo);
    fmpq_init(chi);
    fmpq_init(elo);
    fmpq_init(ehi);
    fmpz_init(t);
    for (k = 0; k < jsonl_count(file); k++)
    {
        rec = jsonl_record(file, k);
        ok = jsonl_field(rec, "f", &v, &err) && read_poly(f, v);
        ok = ok && jsonl_field(rec, "prec", &v, &err) && read_si(&prec, v);
        ok = ok && jsonl_field(rec, "status", &v, &err) && (st = jsonl_string(v, &len, &err)) != NULL;
        ADF_CHECK_MSG(ok, "line %lu: unreadable", (unsigned long) k + 1);
        if (!ok)
            continue;
        if (strcmp(st, "DOMAIN") == 0)
        {
            n_dom++;
            ADF_CHECK_MSG(adf_roots_real(L, f, prec) == ADF_DOMAIN, "line %lu: DOMAIN", (unsigned long) k + 1);
            continue;
        }
        ADF_CHECK_MSG(strcmp(st, "OK") == 0, "line %lu: status %s", (unsigned long) k + 1, st);
        ok = jsonl_field(rec, "g", &v, &err) && read_poly(g, v);
        ok = ok && jsonl_field(rec, "reduced", &v, &err) && read_si(&red, v);
        ok = ok && jsonl_field(rec, "count", &v, &err) && read_si(&count, v);
        ok = ok && jsonl_field(rec, "enc", &enc, &err) && jsonl_size(enc) == (size_t) count;
        ADF_CHECK_MSG(ok, "line %lu: unreadable list", (unsigned long) k + 1);
        if (!ok)
            continue;
        n_ok++;
        ADF_CHECK_MSG(adf_roots_real(L, f, prec) == ADF_OK, "line %lu: status", (unsigned long) k + 1);
        ADF_CHECK_MSG(fmpz_poly_equal(L->g, g) && L->reduced == red, "line %lu: g or reduced",
                      (unsigned long) k + 1);
        ADF_CHECK_MSG(L->n == count && L->count == count, "line %lu: n = %ld, count %ld", (unsigned long) k + 1,
                      (long) L->n, (long) count);
        if (L->n != count)
            continue;
        n_roots += count;
        /* C ball i meets enclosure j exactly when i = j */
        for (i = 0; i < L->n; i++)
        {
            ends_q(clo, chi, L->ball + i);
            for (j = 0; j < count; j++)
            {
                const jsonl_value * e = jsonl_at(enc, (size_t) j, NULL);
                int meet;
                ok = jsonl_size(e) == 4 && read_fmpz(fmpq_numref(elo), jsonl_at(e, 0, NULL)) &&
                     read_fmpz(fmpq_denref(elo), jsonl_at(e, 1, NULL)) &&
                     read_fmpz(fmpq_numref(ehi), jsonl_at(e, 2, NULL)) &&
                     read_fmpz(fmpq_denref(ehi), jsonl_at(e, 3, NULL));
                ADF_CHECK_MSG(ok, "line %lu: enclosure %ld", (unsigned long) k + 1, (long) j);
                if (!ok)
                    continue;
                fmpq_canonicalise(elo);
                fmpq_canonicalise(ehi);
                meet = fmpq_cmp(clo, ehi) <= 0 && fmpq_cmp(elo, chi) <= 0;
                ADF_CHECK_MSG(meet == (i == j), "line %lu: C ball %ld and enclosure %ld: meet %d",
                              (unsigned long) k + 1, (long) i, (long) j, meet);
            }
            ADF_CHECK_MSG(arb_is_exact(L->ball + i) || arb_rel_accuracy_bits(L->ball + i) >= FLINT_MAX(prec, 2),
                          "line %lu: accuracy", (unsigned long) k + 1);
        }
        ADF_CHECK_MSG(adf_rootlist_verify_entries(L, f) == 1 && adf_rootlist_verify_complete(L, f, 0) == 1 &&
                      adf_rootlist_is_canonical(L) == 1, "line %lu: verifiers", (unsigned long) k + 1);
    }
    printf("   %s: %lu lines, %ld OK (%ld roots), %ld DOMAIN\n", path, (unsigned long) jsonl_count(file),
           (long) n_ok, (long) n_roots, (long) n_dom);
    ADF_CHECK(jsonl_count(file) > 150 && n_ok + n_dom == (slong) jsonl_count(file) && n_dom > 0);
    jsonl_close(file);
    fmpz_clear(t);
    fmpq_clear(clo);
    fmpq_clear(chi);
    fmpq_clear(elo);
    fmpq_clear(ehi);
    fmpz_poly_clear(f);
    fmpz_poly_clear(g);
    adf_rootlist_clear(L);
}
