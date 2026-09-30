/* lball.c: adf_lball, a ball in Q_p at one prime (milestone 1F.3, first slice; include/adelefeld/lball.h).

   Contract: docs/conventions.md 5.8; docs/proofs/functions.md Definition 1 (line 11), Proposition 4 (line 92); the
   statements L0 to L8 of docs/api-1f.md, "Statements to add to functions.md". FLINT functions used, from
   refs/src/flint-3.0.1/fmpz.rst: fmpz_remove (lines 1142-1150: removes all factors f > 1 from op, returns their
   number, 0 for op = 0), fmpz_invmod (lines 1154-1160: inverse modulo h, h != 0), and from
   /usr/include/flint/ulong_extras.h:335 n_is_prime (certified, as place.c).

   Every value is stored as p^v u with u a rational unit (exact) or an integer prime to p (ball) and 0 < u < p^(N - v)
   (docs/conventions.md 5.8). The whole file works on (u, v) and forms a power of p only where the result needs one:
     - lb_make reduces a rational q, times p^v0, to the canonical form (statement L0): the only place where p^k, with
       k = N - v the relative precision of a ball, is formed, and not even there when q is an integer >= 0 below
       p^k (compared by bit lengths);
     - a sum forms p^(v - m) for the operands of larger valuation (m the smallest valuation), never p^v itself;
     - a product forms none: it multiplies the unit parts and adds the valuations;
     - the predicates form none: they compare valuations of differences (statement L8).
   Sizes are bounded before anything grows: ADF_LBALL_BITS_MAX for a power of p (ADF_LIMIT), ADF_LBALL_EXP_MAX for the
   exponents of inputs and results (ADF_LIMIT), so no exponent arithmetic here overflows slong: the inputs are within
   2^60, and the sums and differences formed below (v + v', N + N', N - 2 v, N - v) are within 3 * 2^60 < 2^63.
   A function computes into a temporary and swaps it into the output only on ADF_OK (conventions 4.3): aliasing of the
   output with the inputs is then free, and a status leaves the output untouched. */

#include <limits.h>
#include <math.h>

#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include "invariants.h"

#ifdef ADF_CHECK_INVARIANTS
#include <stdio.h>
#include <flint/flint.h>
#define ADF_INV_LBALL(x)                                                                                    \
    do                                                                                                      \
    {                                                                                                       \
        if (!adf_lball_is_canonical(x))                                                                     \
        {                                                                                                   \
            fprintf(stderr, "adelefeld: ADF_CHECK_INVARIANTS: %s: argument %s is not a canonical adf_lball\n", \
                    __func__, #x);                                                                          \
            fflush(stderr);                                                                                 \
            flint_abort();                                                                                  \
        }                                                                                                   \
    }                                                                                                       \
    while (0)
#else
#define ADF_INV_LBALL(x) ((void) 0)
#endif

#define EXP_MAX  ADF_LBALL_EXP_MAX
#define BITS_MAX ADF_LBALL_BITS_MAX
#define NO_BOUND LONG_MAX   /* "N = infinity" in the exponent of a sum of two exact values */

static int
exp_ok(slong a)
{
    return a >= -EXP_MAX && a <= EXP_MAX;
}

/* The exponents of an input are within the bound (the exact value has N = 0, the ball around 0 has v = 0). */
static int
in_bounds(const adf_lball_struct * x)
{
    return exp_ok(x->v) && exp_ok(x->N);
}

/* 1 if p^k, k >= 0, has at most BITS_MAX bits (p^k has at most k bits(p) bits). */
static int
pow_ok(ulong p, slong k)
{
    return k >= 0 && k <= BITS_MAX / (slong) FLINT_BIT_COUNT(p);
}

static void
fmpz_pow_p(fmpz_t out, ulong p, ulong k)
{
    fmpz_set_ui(out, p);
    fmpz_pow_ui(out, out, k);
}

static slong
sat_add(slong a, slong b)
{
    if (b > 0 && a > LONG_MAX - b)
        return LONG_MAX;
    if (b < 0 && a < LONG_MIN - b)
        return LONG_MIN;
    return a + b;
}

/* v_p of a nonzero integer, and the cofactor written to rest (fmpz.rst:1142-1150). rest must not alias a. */
static slong
val_fmpz(fmpz_t rest, const fmpz_t a, const fmpz_t P)
{
    return fmpz_remove(rest, a, P);
}

/* v_p of a nonzero rational. */
static slong
val_fmpq(const fmpq_t q, ulong p)
{
    fmpz_t P, r;
    slong a, b;
    fmpz_init_set_ui(P, p);
    fmpz_init(r);
    a = val_fmpz(r, fmpq_numref(q), P);
    b = val_fmpz(r, fmpq_denref(q), P);
    fmpz_clear(P);
    fmpz_clear(r);
    return a - b;
}

static void
set_exact_zero(adf_lball_struct * x)
{
    fmpq_zero(x->u);
    x->v = 0;
    x->N = 0;
    x->exact = 1;
}

static void
set_ball_zero(adf_lball_struct * x, slong N)
{
    fmpq_zero(x->u);
    x->v = 0;
    x->N = N;
    x->exact = 0;
}

/* Statement L0. out (initialised, a temporary of the caller) = the canonical form of p^v0 q, exact if `exact`,
   else of p^v0 q + p^N Z_p. q is any rational. Status ADF_OK, or ADF_LIMIT: the exponent of the exact result or of
   a ball of nonzero centre is beyond EXP_MAX, N is, or p^k with k = N - v(centre) exceeds the bit bound. */
static int
lb_make(adf_lball_struct * out, ulong p, const fmpq_t q, slong v0, int exact, slong N)
{
    fmpz_t P, nr, dr, pk, inv;
    slong w, k, a, b;
    int status = ADF_OK;

    out->p = p;
    if (!exact && !exp_ok(N))
        return ADF_LIMIT;
    if (fmpq_is_zero(q))
    {
        if (exact)
            set_exact_zero(out);
        else
            set_ball_zero(out, N);
        return ADF_OK;
    }
    fmpz_init_set_ui(P, p);
    fmpz_init(nr);
    fmpz_init(dr);
    a = val_fmpz(nr, fmpq_numref(q), P);
    b = val_fmpz(dr, fmpq_denref(q), P);
    w = v0 + (a - b);                   /* |v0| <= 2^61 and |a - b| is below the bit size of q: no overflow */
    if (exact)
    {
        if (!exp_ok(w))
            status = ADF_LIMIT;
        else
        {
            fmpz_swap(fmpq_numref(out->u), nr);
            fmpz_swap(fmpq_denref(out->u), dr);
            out->v = w;
            out->N = 0;
            out->exact = 1;
        }
    }
    else if (w >= N)
        set_ball_zero(out, N);
    else if (!exp_ok(w))
        status = ADF_LIMIT;
    else
    {
        ulong bp1 = FLINT_BIT_COUNT(p) - 1;
        ulong need;
        k = N - w;                          /* >= 1, at most 2^61 */
        need = (fmpz_bits(nr) + bp1 - 1) / bp1;
        if (fmpz_is_one(dr) && fmpz_sgn(nr) > 0 && (ulong) k >= need)
        {
            /* 0 < nr < 2^bits(nr) <= 2^(k bp1) <= p^k: nr is its own residue (L0), p^k is not formed */
            fmpz_swap(fmpq_numref(out->u), nr);
        }
        else if (!pow_ok(p, k))
            status = ADF_LIMIT;
        else
        {
            fmpz_init(pk);
            fmpz_init(inv);
            fmpz_pow_p(pk, p, (ulong) k);
            fmpz_mod(nr, nr, pk);
            if (!fmpz_is_one(dr))
            {
                int ok = fmpz_invmod(inv, dr, pk);
                (void) ok;                                  /* dr is prime to p, so prime to p^k: ok = 1 */
                fmpz_mul(nr, nr, inv);
                fmpz_mod(nr, nr, pk);
            }
            fmpz_swap(fmpq_numref(out->u), nr);
            fmpz_clear(pk);
            fmpz_clear(inv);
        }
        if (status == ADF_OK)
        {
            fmpz_one(fmpq_denref(out->u));
            out->v = w;
            out->N = N;
            out->exact = 0;
        }
    }
    fmpz_clear(P);
    fmpz_clear(nr);
    fmpz_clear(dr);
    return status;
}

/* The unchecked copy, swap and identity that the functions below use on values they made or already checked: the public
   functions of the same name check their arguments under ADF_CHECK_INVARIANTS (conventions 4.4), and an OUTPUT of an
   arithmetic function is not an input, so it is not checked. */
static void
lb_copy(adf_lball_struct * y, const adf_lball_struct * x)
{
    y->p = x->p;
    fmpq_set(y->u, x->u);
    y->v = x->v;
    y->N = x->N;
    y->exact = x->exact;
}

static void
lb_swap(adf_lball_struct * x, adf_lball_struct * y)
{
    ulong p = x->p;
    slong v = x->v, N = x->N;
    int e = x->exact;
    x->p = y->p; x->v = y->v; x->N = y->N; x->exact = y->exact;
    y->p = p; y->v = v; y->N = N; y->exact = e;
    fmpq_swap(x->u, y->u);
}

static int
lb_identical(const adf_lball_struct * x, const adf_lball_struct * y)
{
    return x->p == y->p && x->exact == y->exact && x->v == y->v && x->N == y->N && fmpq_equal(x->u, y->u);
}

/* Puts the temporary into the output on OK, and clears the temporary. */
static int
finish(adf_lball_struct * z, adf_lball_struct * res, int status)
{
    if (status == ADF_OK)
        lb_swap(z, res);
    adf_lball_clear(res);
    return status;
}

/* ---------------------------------------------------------------------------------------------- life cycle */

void
adf_lball_init(adf_lball_t x)
{
    x->p = 2;
    fmpq_init(x->u);
    x->v = 0;
    x->N = 0;
    x->exact = 1;
}

void
adf_lball_clear(adf_lball_t x)
{
    fmpq_clear(x->u);
}

void
adf_lball_set(adf_lball_t y, const adf_lball_t x)
{
    ADF_INV_LBALL(x);
    lb_copy(y, x);
}

void
adf_lball_swap(adf_lball_t x, adf_lball_t y)
{
    ADF_INV_LBALL(x);
    ADF_INV_LBALL(y);
    lb_swap(x, y);
}

/* conventions 5.8, the predicate. Never aborts: u < p^(N - v) is decided by bit lengths unless p^(N - v) is smaller
   than 2^(2 bits(u)), when it is formed. */
int
adf_lball_is_canonical(const adf_lball_t x)
{
    ulong p = x->p;
    if (p < 2 || !n_is_prime(p))
        return 0;
    if (x->exact != 0 && x->exact != 1)
        return 0;
    if (!fmpq_is_canonical(x->u))
        return 0;
    if (x->exact)
    {
        if (x->N != 0)
            return 0;
        if (fmpq_is_zero(x->u))
            return x->v == 0;
        return fmpz_fdiv_ui(fmpq_numref(x->u), p) != 0 && fmpz_fdiv_ui(fmpq_denref(x->u), p) != 0;
    }
    if (!fmpz_is_one(fmpq_denref(x->u)))
        return 0;
    if (fmpq_is_zero(x->u))
        return x->v == 0;
    if (!(x->v < x->N) || fmpz_sgn(fmpq_numref(x->u)) <= 0 || fmpz_fdiv_ui(fmpq_numref(x->u), p) == 0)
        return 0;
    {
        ulong k = (ulong) x->N - (ulong) x->v;          /* > 0, and exact: the difference is below 2^64 */
        ulong bp1 = FLINT_BIT_COUNT(p) - 1;
        ulong bu = fmpz_bits(fmpq_numref(x->u));
        fmpz_t pk;
        int less;
        if (k >= (bu + bp1 - 1) / bp1)
            return 1;                                   /* p^k >= 2^(k bp1) >= 2^bu > u */
        fmpz_init(pk);
        fmpz_pow_p(pk, p, k);
        less = fmpz_cmp(fmpq_numref(x->u), pk) < 0;
        fmpz_clear(pk);
        return less;
    }
}

int
adf_lball_identical(const adf_lball_t x, const adf_lball_t y)
{
    ADF_INV_LBALL(x);
    ADF_INV_LBALL(y);
    return lb_identical(x, y);
}

/* ---------------------------------------------------------------------------------------------- constructors */

int
adf_lball_set_rat(adf_lball_t x, adf_place_t v, const adf_rat_t q)
{
    adf_lball_t res;
    int st;
    ADF_INV_RAT(q);
    if (adf_place_is_archimedean(v))
        return ADF_DOMAIN;
    adf_lball_init(res);
    st = lb_make(res, adf_place_prime_get(v), q->q, 0, 1, 0);
    return finish(x, res, st);
}

int
adf_lball_set_rat_ball(adf_lball_t x, adf_place_t v, const adf_rat_t c, slong N)
{
    adf_lball_t res;
    int st;
    ADF_INV_RAT(c);
    if (adf_place_is_archimedean(v))
        return ADF_DOMAIN;
    adf_lball_init(res);
    st = lb_make(res, adf_place_prime_get(v), c->q, 0, 0, N);
    return finish(x, res, st);
}

/* api-1f.md L13 (lane f-slice3, finding 1 of lanes/f-slice2/result.md). H > 0, q = A/d canonical, A > 0.
   Returns 1 if adf_lball_set_fball must return ADF_LIMIT, decided without computing v_p(H) to the end, else 0 (then
   the caller computes it; the answer of the caller is the same as before this function existed).
   The only LIMIT of lb_make that the projection can meet besides the bounds of the exponents is the reduction of
   the centre modulo p^k, k = v_p(H) - v_p(A) (L13, step 1), with k bits(p) > BITS_MAX, i.e. k > kmax = BITS_MAX /
   bits(p), and the centre not the small integer that lb_make keeps unreduced (nr > 0, dr = 1, k >=
   ceil(bits(nr)/(bits(p) - 1))). Bit lengths: p^v <= H and p >= 2^(bits(p) - 1) give v_p(H) <= vHmax = (bits(H) -
   1)/(bits(p) - 1) (L13, step 2).
     (i)  vA + kmax >= vHmax: k <= vHmax - vA <= kmax, never LIMIT: return 0.
     (ii) dr = 1 and vA + need <= vHmax: the small-integer shortcut may hold (it needs k >= need): return 0.
     (iii) else k >= need is impossible or dr != 1, so k > kmax is the only question: k > kmax iff p^(vA + kmax + 1)
          divides H (L13, step 3), one divisibility test. */
static int
fball_limit_certain(ulong p, const fmpz_t H, const fmpz_t d, const fmpq_t q)
{
    fmpz_t P, nr, dr, pt;
    slong a, b, vd, vA, kmax = BITS_MAX / (slong) FLINT_BIT_COUNT(p);
    ulong bp1 = FLINT_BIT_COUNT(p) - 1, vHmax, need;
    int r = 0;
    if (fmpq_is_zero(q))
        return 0;
    fmpz_init_set_ui(P, p);
    fmpz_init(nr);
    fmpz_init(dr);
    fmpz_init(pt);
    a = val_fmpz(nr, fmpq_numref(q), P);
    b = val_fmpz(dr, fmpq_denref(q), P);
    vd = val_fmpz(pt, d, P);
    vA = vd + a - b;                                    /* v_p(A) >= 0, A the numerator of the triple */
    vHmax = (fmpz_bits(H) - 1) / bp1;
    {
        /* a tighter upper bound of v_p(H) (review n-review1, C2): p^t <= H < 2^bits(H) gives t < bits(H) / log2(p);
           the quotient is taken in double and 1 is added, far above its rounding error (below 2^-20 for 2^28 bits).
           An upper bound only: it decides "not certain" faster, and every use below needs an upper bound. */
        double t = (double) fmpz_bits(H) / log2((double) p);
        ulong b2 = (ulong) t + 1;
        if (b2 < vHmax)
            vHmax = b2;
    }
    if ((ulong) vA + (ulong) kmax >= vHmax)
        goto out;
    if (fmpz_is_one(dr) && fmpz_sgn(nr) > 0)
    {
        need = (fmpz_bits(nr) + bp1 - 1) / bp1;
        if ((ulong) vA + need <= vHmax)
            goto out;
    }
    {
        /* p^T divides H, T = vA + kmax + 1 <= vHmax (so p^T is about the size of H at most). The test is preceded
        by the tests of p^1 and p^64: each is a necessary condition, and where v_p(H) is small (the usual case) the
           first fails at the cost of one pass over H, not of a power of 2^26 bits. */
        ulong T = (ulong) vA + (ulong) kmax + 1, t = 1;
        r = 1;
        while (r)
        {
            if (t > T)
                t = T;
            fmpz_pow_ui(pt, P, t);
            r = fmpz_divisible(H, pt);
            if (t == T)
                break;
            t = t == 1 ? 64 : T;
        }
    }
out:
    fmpz_clear(P);
    fmpz_clear(nr);
    fmpz_clear(dr);
    fmpz_clear(pt);
    return r;
}

/* api-1f.md L1 (the projection) and L13 (the early decision of LIMIT). */
int
adf_lball_set_fball(adf_lball_t x, adf_place_t v, const adf_fball_t f)
{
    adf_lball_t res;
    fmpz_t A, H, d, P, r;
    fmpq_t q;
    ulong p;
    int st;
    if (adf_place_is_archimedean(v))
        return ADF_DOMAIN;
    p = adf_place_prime_get(v);
    fmpz_init(A); fmpz_init(H); fmpz_init(d); fmpz_init(r);
    fmpz_init_set_ui(P, p);
    fmpq_init(q);
    adf_lball_init(res);
    adf_fball_get_fmpz3(A, H, d, f);
    fmpq_set_fmpz_frac(q, A, d);
    if (fmpz_is_zero(H))
        st = lb_make(res, p, q, 0, 1, 0);
    else if (fball_limit_certain(p, H, d, q))
        st = ADF_LIMIT;
    else
    {
        slong e = val_fmpz(r, H, P) - val_fmpz(r, d, P);
        st = lb_make(res, p, q, 0, 0, e);
    }
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); fmpz_clear(r); fmpz_clear(P);
    fmpq_clear(q);
    return finish(x, res, st);
}

/* ------------------------------------------------------------------------------------------------ accessors */

adf_place_t
adf_lball_place(const adf_lball_t x)
{
    adf_place_t v;
    int st;
    ADF_INV_LBALL(x);
    st = adf_place_prime(&v, x->p);
    if (st != ADF_OK)
        v = adf_place_inf();            /* not reached for a canonical value */
    return v;
}

int
adf_lball_is_exact(const adf_lball_t x)
{
    ADF_INV_LBALL(x);
    return x->exact;
}

int
adf_lball_contains_zero(const adf_lball_t x)
{
    ADF_INV_LBALL(x);
    return fmpq_is_zero(x->u);
}

int
adf_lball_get_prec(slong * N, const adf_lball_t x)
{
    ADF_INV_LBALL(x);
    if (x->exact)
        return ADF_DOMAIN;
    *N = x->N;
    return ADF_OK;
}

int
adf_lball_get_center(adf_rat_t c, const adf_lball_t x)
{
    fmpq_t t;
    ADF_INV_LBALL(x);
    if (!exp_ok(x->v))
        return ADF_LIMIT;
    if (fmpq_is_zero(x->u))
    {
        fmpq_zero(c->q);
        return ADF_OK;
    }
    if (!pow_ok(x->p, x->v < 0 ? -x->v : x->v))
        return ADF_LIMIT;
    fmpq_init(t);
    fmpz_pow_p(x->v >= 0 ? fmpq_numref(t) : fmpq_denref(t), x->p, (ulong) (x->v < 0 ? -x->v : x->v));
    if (x->v >= 0)
        fmpz_one(fmpq_denref(t));
    else
        fmpz_one(fmpq_numref(t));
    fmpq_mul(c->q, x->u, t);
    fmpq_clear(t);
    return ADF_OK;
}

/* -------------------------------------------------------------------------------------------- set predicates */

/* Statement L8, last paragraph. Returns 1 if the value of x equals the value of y (the valuation of the difference
   is infinity); else 0 and *w = v_p(value(x) - value(y)). The value of a zero centre is 0. No power of p. */
static int
diff_val(slong * w, const adf_lball_struct * x, const adf_lball_struct * y)
{
    int xz = fmpq_is_zero(x->u), yz = fmpq_is_zero(y->u);
    if (xz && yz)
        return 1;
    if (xz)
    {
        *w = y->v;
        return 0;
    }
    if (yz)
    {
        *w = x->v;
        return 0;
    }
    if (x->v != y->v)
    {
        *w = x->v < y->v ? x->v : y->v;
        return 0;
    }
    if (fmpq_equal(x->u, y->u))
        return 1;
    {
        fmpq_t d;
        fmpq_init(d);
        fmpq_sub(d, x->u, y->u);
        *w = sat_add(x->v, val_fmpq(d, x->p));
        fmpq_clear(d);
    }
    return 0;
}

int
adf_lball_equal_set(const adf_lball_t x, const adf_lball_t y)
{
    ADF_INV_LBALL(x);
    ADF_INV_LBALL(y);
    return lb_identical(x, y);
}

int
adf_lball_overlaps(const adf_lball_t x, const adf_lball_t y)
{
    slong w, n;
    ADF_INV_LBALL(x);
    ADF_INV_LBALL(y);
    if (x->p != y->p)
        return 0;
    if (x->exact && y->exact)
        return lb_identical(x, y);
    n = x->exact ? y->N : y->exact ? x->N : (x->N < y->N ? x->N : y->N);
    return diff_val(&w, x, y) || w >= n;
}

int
adf_lball_contains(const adf_lball_t x, const adf_lball_t y)
{
    slong w;
    ADF_INV_LBALL(x);
    ADF_INV_LBALL(y);
    if (x->p != y->p)
        return 0;
    if (y->exact)
        return x->exact && lb_identical(x, y);
    if (!x->exact && x->N < y->N)
        return 0;
    return diff_val(&w, x, y) || w >= y->N;
}

/* -------------------------------------------------------------------------------------------------- arithmetic */

int
adf_lball_neg(adf_lball_t y, const adf_lball_t x)
{
    adf_lball_t res;
    fmpq_t m;
    int st;
    ADF_INV_LBALL(x);
    if (!in_bounds(x))
        return ADF_LIMIT;
    adf_lball_init(res);
    if (x->exact)
    {
        lb_copy(res, x);
        fmpq_neg(res->u, x->u);
        return finish(y, res, ADF_OK);
    }
    fmpq_init(m);
    fmpq_neg(m, x->u);                                   /* L5: -c = p^v (-u) */
    st = lb_make(res, x->p, m, x->v, 0, x->N);
    fmpq_clear(m);
    return finish(y, res, st);
}

/* L2 (sum) and L5 with L2 (difference): z = x + sgn y, sgn = +1 or -1. The sign is applied to the unit part of the
   operand y inside the sum, so no canonical value -y is formed: its centre p^k - u needs p^k even where the difference
   is small (finding F2 of docs/reviews/f1/review-lball.md). The operand is left out of the sum when it is 0 or when
   its valuation is at least K (it lies in p^K Z_p). The inputs are checked by the callers. */
static int
lb_addsub(adf_lball_t z, const adf_lball_struct * x, const adf_lball_struct * y, int sgn)
{
    adf_lball_t res;
    const adf_lball_struct * op[2];
    int isy[2];                     /* the sign is tracked by position: x and y may be the same object */
    int n = 0, i, st = ADF_OK, exact;
    slong K, m = LONG_MAX;
    fmpq_t q, t;
    fmpz_t pk;
    if (x->p != y->p)
        return ADF_DOMAIN;
    if (!in_bounds(x) || !in_bounds(y))
        return ADF_LIMIT;
    exact = x->exact && y->exact;
    K = exact ? NO_BOUND : x->exact ? y->N : y->exact ? x->N : (x->N < y->N ? x->N : y->N);
    if (!fmpq_is_zero(x->u) && x->v < K)
    {
        isy[n] = 0;
        op[n++] = x;
    }
    if (!fmpq_is_zero(y->u) && y->v < K)
    {
        isy[n] = 1;
        op[n++] = y;
    }
    adf_lball_init(res);
    fmpq_init(q);
    fmpq_init(t);
    fmpz_init(pk);
    for (i = 0; i < n; i++)
        if (op[i]->v < m)
            m = op[i]->v;
    for (i = 0; i < n && st == ADF_OK; i++)
    {
        /* q += u p^(v - m) */
        if (!pow_ok(x->p, op[i]->v - m))
            st = ADF_LIMIT;
        else
        {
            fmpz_pow_p(pk, x->p, (ulong) (op[i]->v - m));
            fmpq_mul_fmpz(t, op[i]->u, pk);
            if (isy[i] && sgn < 0)
                fmpq_sub(q, q, t);
            else
                fmpq_add(q, q, t);
        }
    }
    if (st == ADF_OK)
        st = lb_make(res, x->p, q, n ? m : 0, exact, exact ? 0 : K);
    fmpq_clear(q);
    fmpq_clear(t);
    fmpz_clear(pk);
    return finish(z, res, st);
}

int
adf_lball_add(adf_lball_t z, const adf_lball_t x, const adf_lball_t y)
{
    ADF_INV_LBALL(x);
    ADF_INV_LBALL(y);
    return lb_addsub(z, x, y, 1);
}

/* z = x - y: the sum of x and the negation of y, with the sign inside (lb_addsub; L5, L2). */
int
adf_lball_sub(adf_lball_t z, const adf_lball_t x, const adf_lball_t y)
{
    ADF_INV_LBALL(x);
    ADF_INV_LBALL(y);
    return lb_addsub(z, x, y, -1);
}

/* L3. */
int
adf_lball_mul(adf_lball_t z, const adf_lball_t x, const adf_lball_t y)
{
    adf_lball_t res;
    fmpq_t q;
    slong K = NO_BOUND, c;
    int st, exact, xz, yz;
    ADF_INV_LBALL(x);
    ADF_INV_LBALL(y);
    if (x->p != y->p)
        return ADF_DOMAIN;
    if (!in_bounds(x) || !in_bounds(y))
        return ADF_LIMIT;
    adf_lball_init(res);
    xz = fmpq_is_zero(x->u);
    yz = fmpq_is_zero(y->u);
    if ((x->exact && xz) || (y->exact && yz))
    {
        res->p = x->p;
        set_exact_zero(res);
        return finish(z, res, ADF_OK);
    }
    exact = x->exact && y->exact;
    if (!exact)
    {
        if (!y->exact && !xz)
        {
            c = x->v + y->N;
            if (c < K)
                K = c;
        }
        if (!x->exact && !yz)
        {
            c = y->v + x->N;
            if (c < K)
                K = c;
        }
        if (!x->exact && !y->exact)
        {
            c = x->N + y->N;
            if (c < K)
                K = c;
        }
    }
    fmpq_init(q);
    fmpq_mul(q, x->u, y->u);
    st = lb_make(res, x->p, q, (xz || yz) ? 0 : x->v + y->v, exact, exact ? 0 : K);
    fmpq_clear(q);
    return finish(z, res, st);
}

/* L4. */
int
adf_lball_inv(adf_lball_t y, const adf_lball_t x)
{
    adf_lball_t res;
    fmpq_t q;
    int st;
    ADF_INV_LBALL(x);
    if (!in_bounds(x))
        return ADF_LIMIT;
    if (fmpq_is_zero(x->u))
        return x->exact ? ADF_NOT_UNIT : ADF_UNIT_NOT_CERTIFIED;
    /* The exponent of the RESULT: N - 2 v for a ball. An exact value has no precision, and its inverse has the
       valuation -v, which is within the bound because v is (finding F1 of docs/reviews/f1/review-lball.md). */
    if (!x->exact && !exp_ok(x->N - 2 * x->v))
        return ADF_LIMIT;
    adf_lball_init(res);
    fmpq_init(q);
    fmpq_inv(q, x->u);
    st = lb_make(res, x->p, q, -x->v, x->exact, x->exact ? 0 : x->N - 2 * x->v);
    fmpq_clear(q);
    return finish(y, res, st);
}

/* L4a (docs/api-1f.md, the quotient): z = x / y, computed directly and not as x (1/y), so that no intermediate
   value, the inverse of y with its own exponent N - 2 v and its own centre, can be outside the limits when the
   quotient is not (finding F3 of docs/reviews/f1/review-lball.md).
   With c = p^v u the centre of x (u = 0 for a zero centre) and d = p^w t that of y (t != 0), M the precision of y,
   the set of inverses of y is 1/d + p^(M - 2w) Z_p (L4), of valuation -w and unit part 1/t, and by L3 the quotient is
       (u/t) p^(v - w) + p^K Z_p,   K = min( v + M - 2w   [x has centre != 0 and y is a ball],
                                             N - w        [x is a ball],
                                             N + M - 2w   [both are balls] ),
   the terms of an exact operand being absent. Two exact operands give the exact (u/t) p^(v - w); the exact 0 gives
   the exact 0. lb_make then reduces u/t modulo p^(K - (v - w)): the centre is computed only to the relative
   precision of the RESULT (K - (v - w) <= M - w, the relative precision of the inverse, so the residue is the same
   as the one the inverse would give). The bounds: all inputs within EXP_MAX, so every term is within 2^62. */
int
adf_lball_div(adf_lball_t z, const adf_lball_t x, const adf_lball_t y)
{
    adf_lball_t res;
    fmpq_t q;
    slong K = NO_BOUND, c;
    int st, exact, xz;
    ADF_INV_LBALL(x);
    ADF_INV_LBALL(y);
    if (x->p != y->p)
        return ADF_DOMAIN;
    if (!in_bounds(y))
        return ADF_LIMIT;
    if (fmpq_is_zero(y->u))
        return y->exact ? ADF_NOT_UNIT : ADF_UNIT_NOT_CERTIFIED;
    if (!in_bounds(x))
        return ADF_LIMIT;
    adf_lball_init(res);
    xz = fmpq_is_zero(x->u);
    if (x->exact && xz)
    {
        res->p = x->p;
        set_exact_zero(res);
        return finish(z, res, ADF_OK);
    }
    exact = x->exact && y->exact;
    if (!exact)
    {
        if (!y->exact && !xz)
        {
            c = x->v + y->N - 2 * y->v;
            if (c < K)
                K = c;
        }
        if (!x->exact)
        {
            c = x->N - y->v;
            if (c < K)
                K = c;
        }
        if (!x->exact && !y->exact)
        {
            c = x->N + y->N - 2 * y->v;
            if (c < K)
                K = c;
        }
    }
    fmpq_init(q);
    fmpq_div(q, x->u, y->u);
    st = lb_make(res, x->p, q, xz ? 0 : x->v - y->v, exact, exact ? 0 : K);
    fmpq_clear(q);
    return finish(z, res, st);
}

/* v_p of a nonzero ulong. */
static ulong
vp_ulong(ulong n, ulong p)
{
    ulong e = 0;
    while (n % p == 0)
    {
        n /= p;
        e++;
    }
    return e;
}

/* out = a^n for a nonzero integer a and n >= 1. ADF_LIMIT if the result would have more than BITS_MAX bits, decided
   before the power is formed: bits(a^n) > n (bits(a) - 1) for |a| >= 2, and the formed power has at most n bits(a)
   bits. The powers of +-1 are formed by sign (n may be 2^63). */
static int
lb_int_pow(fmpz_t out, const fmpz_t a, ulong n)
{
    ulong b1;
    if (fmpz_is_pm1(a))
    {
        fmpz_set_si(out, (fmpz_sgn(a) < 0 && (n & 1)) ? -1 : 1);
        return ADF_OK;
    }
    b1 = fmpz_bits(a) - 1;                              /* >= 1 for |a| >= 2 */
    if (n > (ulong) BITS_MAX / b1)
        return ADF_LIMIT;
    fmpz_pow_ui(out, a, n);
    return fmpz_bits(out) > (ulong) BITS_MAX ? ADF_LIMIT : ADF_OK;
}

/* L12 (docs/api-1f.md): y = x^k, the smallest ball containing {s^k : s in x}. With x = p^v (u + p^rel Z_p), u a
   unit integer, rel = N - v >= 1, n = |k| >= 1: the set of n-th powers of the units of the ball is u^n + p^rel' Z_p
   with rel' = rel + v_p(n) + e, e = 1 if p = 2, rel = 1 and n even (Lemma 9, docs/proofs/functions.md line 265, and
   Proposition 4 step 2 for the sign at p = 2); the k-th power for k < 0 is the inverse of that ball (L4: the
   relative precision is kept). The centre u^n mod p^rel' is formed by fmpz_powm, or, where p^rel' is beyond the bit
   bound, as the integer u^n if that is small, and the result is then stored by lb_make (which keeps a small integer
   centre unreduced). */
int
adf_lball_pow_si(adf_lball_t y, const adf_lball_t x, slong k)
{
    adf_lball_t res;
    ulong n, p, vpn = 0;
    slong vk, rel, relp, Np;
    fmpz_t c, P;
    fmpq_t q;
    int st = ADF_OK;
    ADF_INV_LBALL(x);
    if (!in_bounds(x))
        return ADF_LIMIT;
    p = x->p;
    adf_lball_init(res);
    res->p = p;
    if (k == 0)
    {
        fmpz_one(fmpq_numref(res->u));
        res->exact = 1;
        return finish(y, res, ADF_OK);
    }
    n = k < 0 ? -(ulong) k : (ulong) k;
    if (fmpq_is_zero(x->u))
    {
        if (k < 0)
        {
            adf_lball_clear(res);
            return x->exact ? ADF_NOT_UNIT : ADF_UNIT_NOT_CERTIFIED;
        }
        if (x->exact)
            set_exact_zero(res);
        else if (x->N != 0 && (ulong) (x->N < 0 ? -x->N : x->N) > (ulong) EXP_MAX / n)
            st = ADF_LIMIT;
        else
            set_ball_zero(res, x->N * (slong) n);           /* |N n| <= EXP_MAX < 2^63 */
        return finish(y, res, st);
    }
    /* the valuation of the result: k v, within the bound */
    if (x->v != 0 && (ulong) (x->v < 0 ? -x->v : x->v) > (ulong) EXP_MAX / n)
    {
        adf_lball_clear(res);
        return ADF_LIMIT;
    }
    vk = 0;
    if (x->v != 0)                                      /* then n <= EXP_MAX, and |v n| <= EXP_MAX */
        vk = k < 0 ? -(x->v * (slong) n) : x->v * (slong) n;
    fmpz_init(c);
    fmpz_init(P);
    fmpq_init(q);
    if (x->exact)
    {
        fmpz_t nu, de;
        fmpz_init(nu);
        fmpz_init(de);
        st = lb_int_pow(nu, fmpq_numref(x->u), n);
        if (st == ADF_OK)
            st = lb_int_pow(de, fmpq_denref(x->u), n);
        if (st == ADF_OK)
        {
            if (k < 0)
                fmpz_swap(nu, de);
            if (fmpz_sgn(de) < 0)
            {
                fmpz_neg(de, de);
                fmpz_neg(nu, nu);
            }
            fmpz_swap(fmpq_numref(res->u), nu);
            fmpz_swap(fmpq_denref(res->u), de);
            res->v = vk;
            res->N = 0;
            res->exact = 1;
        }
        fmpz_clear(nu);
        fmpz_clear(de);
        goto out;
    }
    rel = x->N - x->v;
    vpn = vp_ulong(n, p);
    relp = rel + (slong) vpn + ((p == 2 && rel == 1 && n % 2 == 0) ? 1 : 0);
    Np = vk + relp;                                     /* |vk| <= 2^60, relp <= 2^61 + 64 */
    if (!exp_ok(Np))
    {
        st = ADF_LIMIT;
        goto out;
    }
    if (fmpz_is_one(fmpq_numref(x->u)))
        fmpz_one(c);
    else if (pow_ok(p, relp))
    {
        fmpz_pow_p(P, p, (ulong) relp);
        fmpz_powm_ui(c, fmpq_numref(x->u), n, P);
        if (k < 0)
        {
            int ok = fmpz_invmod(c, c, P);
            (void) ok;                                  /* u is prime to p */
        }
    }
    else if (k > 0)
    {
        /* u^n as an integer: lb_make keeps it unreduced if it is below p^relp (bit lengths), else LIMIT */
        st = lb_int_pow(c, fmpq_numref(x->u), n);
    }
    else
        st = ADF_LIMIT;                                 /* the inverse modulo p^relp needs p^relp */
    if (st == ADF_OK)
    {
        fmpz_swap(fmpq_numref(q), c);
        fmpz_one(fmpq_denref(q));
        st = lb_make(res, p, q, vk, 0, Np);
    }
out:
    fmpz_clear(c);
    fmpz_clear(P);
    fmpq_clear(q);
    return finish(y, res, st);
}

/* ----------------------------------------------------------------------- valuation, absolute value, decomposition */

int
adf_lball_valuation(slong * v, int * is_inf, const adf_lball_t x)
{
    ADF_INV_LBALL(x);
    if (fmpq_is_zero(x->u))
    {
        if (!x->exact)
            return ADF_NOT_DETERMINED;
        *v = 0;
        *is_inf = 1;
        return ADF_OK;
    }
    *v = x->v;
    *is_inf = 0;
    return ADF_OK;
}

int
adf_lball_abs(adf_rat_t a, const adf_lball_t x)
{
    fmpq_t t;
    slong e;
    ADF_INV_LBALL(x);
    if (fmpq_is_zero(x->u))
    {
        if (!x->exact)
            return ADF_NOT_DETERMINED;
        fmpq_zero(a->q);
        return ADF_OK;
    }
    if (!exp_ok(x->v))
        return ADF_LIMIT;
    e = x->v < 0 ? -x->v : x->v;
    if (!pow_ok(x->p, e))
        return ADF_LIMIT;
    fmpq_init(t);
    if (x->v <= 0)                               /* |x| = p^(-v) */
    {
        fmpz_pow_p(fmpq_numref(t), x->p, (ulong) e);
        fmpz_one(fmpq_denref(t));
    }
    else
    {
        fmpz_one(fmpq_numref(t));
        fmpz_pow_p(fmpq_denref(t), x->p, (ulong) e);
    }
    fmpq_swap(a->q, t);
    fmpq_clear(t);
    return ADF_OK;
}

int
adf_lball_decompose(slong * m, adf_lball_t unit, const adf_lball_t x)
{
    adf_lball_t res;
    ADF_INV_LBALL(x);
    if (fmpq_is_zero(x->u))
        return x->exact ? ADF_DOMAIN : ADF_NOT_DETERMINED;
    if (!in_bounds(x) || !exp_ok(x->N - x->v))
        return ADF_LIMIT;
    adf_lball_init(res);
    lb_copy(res, x);
    res->N = x->exact ? 0 : x->N - x->v;
    res->v = 0;
    *m = x->v;
    return finish(unit, res, ADF_OK);
}
