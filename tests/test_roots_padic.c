/* tests/test_roots_padic.c: all roots at a prime (milestone S, S.2, slice 2, lane s2-slice2):
   adf_roots_padic, adf_roots_padic_partial, adf_rootlist_get_unresolved, adf_rootlist_verify_complete.

   The statements tested are docs/proofs/solvers.md Proposition 3.4 (line 1121), Algorithm P and
   Proposition 3.5 (line 1160), Proposition 3.6 (line 1246), Proposition 3.7(1) (line 1284), 3.11
   (line 1449) and Proposition 3.13(2), (3) (line 1524); the contract is include/adelefeld/roots.h.
   The oracles are written here and do not call the library: the normalised polynomial over Q with
   fmpq_poly (as tests/test_roots_seed.c), the approximate roots modulo p^M by an enumeration level by
   level (approx_roots, proto/solvers_checks.py:1791), and an integer Bezout identity u g + v g' = R
   (solvers P3.6(3)) whose value R bounds the precision M that the enumeration needs (see
   "The precision of the enumeration" below). The vectors tests/ref/vectors/s2-slice2/padic.jsonl come
   from rootlist_padic of the reference (lanes/s2-slice2/gen_vectors.py).

   Two functions of src/roots.c with hidden visibility are declared below: the whole search with the
   limit of S-D18 as a parameter, and the certification of one simple digit (solvers P3.4(3), Algorithm
   P step 3). They are not part of the interface; they are called here to reach the limits of S-D18
   that no polynomial of a size a test can handle reaches. */

#include <stddef.h>
#include <string.h>

#include <flint/fmpq_poly.h>
#include <flint/fmpz_vec.h>

#include <adelefeld.h>

#include "support/jsonl.h"

#include "test_runner.h"

/* hidden in src/roots.c (not declared by a public header, not exported by the shared object) */
int adf_roots_padic_core(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec_p, slong depth,
                         int strict, slong bits_max);
int adf_roots_certify(fmpz_t a_out, slong * K, slong * s, const fmpz_poly_t g, const fmpz_poly_t h,
                      const fmpz_t p, const fmpz_t a, const fmpz_t pe, slong e, slong w, ulong b, slong prec_p,
                      slong bits_max);

/* ---- helpers ---- */

static void
poly_set_si(fmpz_poly_t f, const slong * c, slong len)
{
    slong i;

    fmpz_poly_zero(f);
    for (i = 0; i < len; i++)
        fmpz_poly_set_coeff_si(f, i, c[i]);
}

/* f = lead * prod (X - r_i) */
static void
poly_from_roots(fmpz_poly_t f, slong lead, const slong * r, slong n)
{
    fmpz_poly_t t;
    slong i;

    fmpz_poly_init(t);
    fmpz_poly_set_si(f, lead);
    for (i = 0; i < n; i++)
    {
        fmpz_poly_zero(t);
        fmpz_poly_set_coeff_si(t, 1, 1);
        fmpz_poly_set_coeff_si(t, 0, -r[i]);
        fmpz_poly_mul(f, f, t);
    }
    fmpz_poly_clear(t);
}

static adf_place_t
place_of(ulong p)
{
    adf_place_t v = adf_place_inf();
    int st = adf_place_prime(&v, p);

    if (st != ADF_OK)
        abort();
    return v;
}

/* The oracle of the normalised polynomial (solvers L3.1(3)): f / gcd(f, f') over Q, scaled to a
   primitive integer polynomial with positive leading coefficient; 1 for f constant. */
static void
oracle_normalise(fmpz_poly_t g, int * reduced, const fmpz_poly_t f)
{
    fmpq_poly_t F, D, G, Q;

    fmpq_poly_init(F);
    fmpq_poly_init(D);
    fmpq_poly_init(G);
    fmpq_poly_init(Q);
    fmpq_poly_set_fmpz_poly(F, f);
    fmpq_poly_derivative(D, F);
    fmpq_poly_gcd(G, F, D);
    fmpq_poly_div(Q, F, G);
    fmpq_poly_get_numerator(g, Q);
    fmpz_poly_primitive_part(g, g);
    *reduced = fmpz_poly_degree(f) > 0 && fmpq_poly_degree(G) > 0;
    if (fmpz_poly_degree(f) <= 0)
        fmpz_poly_set_ui(g, 1);
    fmpq_poly_clear(F);
    fmpq_poly_clear(D);
    fmpq_poly_clear(G);
    fmpq_poly_clear(Q);
}

/* v_p(x) of x != 0, by repeated division */
static slong
vp(const fmpz_t x, const fmpz_t p)
{
    fmpz_t t;
    slong v = 0;

    fmpz_init_set(t, x);
    while (fmpz_divisible(t, p))
    {
        fmpz_divexact(t, t, p);
        v++;
    }
    fmpz_clear(t);
    return v;
}

/* The oracle of solvers P3.6(3): for g nonconstant with gcd(g, g') = 1 in Q[X], polynomials S, T of
   Q[X] with S g + T g' = 1 (from fmpq_poly_xgcd; the identity is checked here, so nothing of FLINT's
   description of the function is relied on [source pending: flint-3.0.1 fmpq_poly.rst is not under
   refs/]); R = the least common multiple of the denominators of S and T, so that u = R S and v = R T
   are in Z[X] and u g + v g' = R. Returns v_p(R); 0 for g constant; -1 if the identity fails. */
static slong
oracle_bezout_val(const fmpz_poly_t g, const fmpz_t p)
{
    fmpq_poly_t G, D, S, T, A, B;
    fmpz_t R;
    slong v;

    if (fmpz_poly_degree(g) <= 0)
        return 0;
    fmpq_poly_init(G);
    fmpq_poly_init(D);
    fmpq_poly_init(S);
    fmpq_poly_init(T);
    fmpq_poly_init(A);
    fmpq_poly_init(B);
    fmpz_init(R);
    fmpq_poly_set_fmpz_poly(A, g);
    fmpq_poly_derivative(D, A);
    fmpq_poly_xgcd(G, S, T, A, D);
    fmpq_poly_mul(A, A, S);
    fmpq_poly_mul(B, D, T);
    fmpq_poly_add(A, A, B);
    if (!fmpq_poly_is_one(A))
        v = -1;
    else
    {
        fmpz_lcm(R, fmpq_poly_denref(S), fmpq_poly_denref(T));
        v = vp(R, p);
    }
    fmpq_poly_clear(G);
    fmpq_poly_clear(D);
    fmpq_poly_clear(S);
    fmpq_poly_clear(T);
    fmpq_poly_clear(A);
    fmpq_poly_clear(B);
    fmpz_clear(R);
    return v;
}

/* r = g(x) mod M, by Horner, 0 <= r < M */
static void
eval_mod(fmpz_t r, const fmpz_poly_t g, const fmpz_t x, const fmpz_t M)
{
    slong i;

    fmpz_zero(r);
    for (i = fmpz_poly_length(g) - 1; i >= 0; i--)
    {
        fmpz_mul(r, r, x);
        fmpz_add(r, r, g->coeffs + i);
        fmpz_mod(r, r, M);
    }
}

/* The approximate roots (approx_roots, proto/solvers_checks.py:1791): the x in [0, p^M) with x = c
   modulo p^e and g(x) = 0 modulo p^M, level by level (a root modulo p^(i+1) reduces to a root modulo
   p^i). They are written to Z (an array of cap entries) and counted; -1 if a level has more than cap
   elements. */
static slong
oracle_roots_mod(fmpz * Z, slong cap, const fmpz_poly_t g, ulong p, const fmpz_t c, slong e, slong M)
{
    fmpz * nxt;
    fmpz_t q, pi, v, y, fp;
    slong ncur, nnxt, i, j;
    ulong t;

    fmpz_init(q);
    fmpz_init(pi);
    fmpz_init(v);
    fmpz_init(y);
    fmpz_init_set_ui(fp, p);
    nxt = _fmpz_vec_init(cap);
    fmpz_pow_ui(q, fp, (ulong) e);
    fmpz_mod(Z + 0, c, q);
    eval_mod(v, g, Z + 0, q);
    ncur = fmpz_is_zero(v) ? 1 : 0;
    for (i = e; i < M && ncur > 0; i++)
    {
        fmpz_pow_ui(pi, fp, (ulong) i);
        fmpz_mul_ui(q, pi, p);
        nnxt = 0;
        for (j = 0; j < ncur; j++)
            for (t = 0; t < p; t++)
            {
                fmpz_set(y, Z + j);
                fmpz_addmul_ui(y, pi, t);
                eval_mod(v, g, y, q);
                if (fmpz_is_zero(v))
                {
                    if (nnxt == cap)
                    {
                        ncur = -1;
                        goto done;
                    }
                    fmpz_set(nxt + nnxt, y);
                    nnxt++;
                }
            }
        for (j = 0; j < nnxt; j++)
            fmpz_swap(Z + j, nxt + j);
        ncur = nnxt;
    }
done:
    _fmpz_vec_clear(nxt, cap);
    fmpz_clear(q);
    fmpz_clear(pi);
    fmpz_clear(v);
    fmpz_clear(y);
    fmpz_clear(fp);
    return ncur;
}

/* 1 if x = y modulo p^k */
static int
cong(const fmpz_t x, const fmpz_t y, const fmpz_t p, slong k)
{
    fmpz_t d, q;
    int r;

    fmpz_init(d);
    fmpz_init(q);
    fmpz_sub(d, x, y);
    fmpz_pow_ui(q, p, (ulong) k);
    r = fmpz_divisible(d, q);
    fmpz_clear(d);
    fmpz_clear(q);
    return r;
}

/* The balls of Proposition 3.2: the number of x modulo p^(K+s+3) with x = a modulo p^(s+1) and
   g(x) = 0 modulo p^(K+s+3) (check_s2_certificate, proto/solvers_checks.py:1847); *same = 1 if all of
   them are = a modulo p^K. -1 if too many. */
static slong
oracle_ball_count(int * same, const fmpz_poly_t g, ulong p, const fmpz_t a, slong K, slong s)
{
    fmpz * Z;
    fmpz_t fp;
    slong n, i, cap = 1 << 16;

    Z = _fmpz_vec_init(cap);
    fmpz_init_set_ui(fp, p);
    n = oracle_roots_mod(Z, cap, g, p, a, s + 1, K + s + 3);
    *same = 1;
    for (i = 0; i < n; i++)
        if (!cong(Z + i, a, fp, K))
            *same = 0;
    _fmpz_vec_clear(Z, cap);
    fmpz_clear(fp);
    return n;
}

static slong
ipow(ulong p, slong s)
{
    slong r = 1, i;

    for (i = 0; i < s; i++)
        r *= (slong) p;
    return r;
}

/* A deep snapshot of a list, to test "L untouched": the raw bytes of the struct (the pointers and the
   integers) and copies of what the pointers point to. */
typedef struct
{
    adf_rootlist_struct raw;
    fmpz_poly_t g;
    slong n, nu;
    fmpz * a;
    fmpz * ua;
    slong * K;
    slong * s;
    slong * ue;
} snap_t;

static void
snap_take(snap_t * S, const adf_rootlist_t L)
{
    slong i;

    memcpy(&S->raw, L, sizeof(adf_rootlist_struct));
    fmpz_poly_init(S->g);
    fmpz_poly_set(S->g, L->g);
    S->n = L->a != NULL ? L->n : 0;
    S->nu = L->ua != NULL ? L->nu : 0;
    S->a = _fmpz_vec_init(S->n + 1);
    S->ua = _fmpz_vec_init(S->nu + 1);
    S->K = flint_malloc((S->n + 1) * sizeof(slong));
    S->s = flint_malloc((S->n + 1) * sizeof(slong));
    S->ue = flint_malloc((S->nu + 1) * sizeof(slong));
    for (i = 0; i < S->n; i++)
    {
        fmpz_set(S->a + i, L->a + i);
        S->K[i] = L->K[i];
        S->s[i] = L->s[i];
    }
    for (i = 0; i < S->nu; i++)
    {
        fmpz_set(S->ua + i, L->ua + i);
        S->ue[i] = L->ue[i];
    }
}

static int
snap_same(const snap_t * S, const adf_rootlist_t L)
{
    slong i;

    if (memcmp(&S->raw, L, sizeof(adf_rootlist_struct)) != 0 || !fmpz_poly_equal(S->g, L->g))
        return 0;
    for (i = 0; i < S->n; i++)
        if (!fmpz_equal(S->a + i, L->a + i) || S->K[i] != L->K[i] || S->s[i] != L->s[i])
            return 0;
    for (i = 0; i < S->nu; i++)
        if (!fmpz_equal(S->ua + i, L->ua + i) || S->ue[i] != L->ue[i])
            return 0;
    return 1;
}

static void
snap_clear(snap_t * S)
{
    fmpz_poly_clear(S->g);
    _fmpz_vec_clear(S->a, S->n + 1);
    _fmpz_vec_clear(S->ua, S->nu + 1);
    flint_free(S->K);
    flint_free(S->s);
    flint_free(S->ue);
}

/* 1 if the two lists are equal field by field (place, scope, reduced, complete, g, the certificates,
   the classes, count) */
static int
lists_equal(const adf_rootlist_t A, const adf_rootlist_t B)
{
    slong i;

    if (!adf_place_equal(A->place, B->place) || A->scope != B->scope || A->reduced != B->reduced ||
        A->complete != B->complete || !fmpz_poly_equal(A->g, B->g) || A->n != B->n || A->nu != B->nu ||
        A->count != B->count)
        return 0;
    for (i = 0; i < A->n; i++)
        if (!fmpz_equal(A->a + i, B->a + i) || A->K[i] != B->K[i] || A->s[i] != B->s[i])
            return 0;
    for (i = 0; i < A->nu; i++)
        if (!fmpz_equal(A->ua + i, B->ua + i) || A->ue[i] != B->ue[i])
            return 0;
    return 1;
}

/* dst = a deep copy of src at a prime, with the allocators of roots.h (a, ua: _fmpz_vec_init; K, s,
   ue: flint_malloc), leaving out the certificate of index drop (-1: none) */
static void
list_copy_drop(adf_rootlist_t dst, const adf_rootlist_t src, slong drop)
{
    slong i, j;

    adf_rootlist_clear(dst);
    adf_rootlist_init(dst);
    dst->place = src->place;
    dst->scope = src->scope;
    dst->reduced = src->reduced;
    dst->complete = src->complete;
    fmpz_poly_set(dst->g, src->g);
    dst->n = src->n - (drop >= 0 && drop < src->n ? 1 : 0);
    if (dst->n > 0)
    {
        dst->a = _fmpz_vec_init(dst->n);
        dst->K = flint_malloc(dst->n * sizeof(slong));
        dst->s = flint_malloc(dst->n * sizeof(slong));
    }
    for (i = 0, j = 0; i < src->n; i++)
        if (i != drop)
        {
            fmpz_set(dst->a + j, src->a + i);
            dst->K[j] = src->K[i];
            dst->s[j] = src->s[i];
            j++;
        }
    dst->nu = src->nu;
    if (dst->nu > 0)
    {
        dst->ua = _fmpz_vec_init(dst->nu);
        dst->ue = flint_malloc(dst->nu * sizeof(slong));
    }
    for (i = 0; i < src->nu; i++)
    {
        fmpz_set(dst->ua + i, src->ua + i);
        dst->ue[i] = src->ue[i];
    }
    dst->count = 0;
}

/* counting FLINT's allocator (flint.h: __flint_set_memory_functions); GMP's own allocations of mpz
   limbs and libc are not counted */
static unsigned long flint_allocs = 0;
static void * (*orig_alloc)(size_t);
static void * (*orig_calloc)(size_t, size_t);
static void * (*orig_realloc)(void *, size_t);
static void (*orig_free)(void *);

static void *
count_alloc(size_t n)
{
    flint_allocs++;
    return orig_alloc(n);
}

static void *
count_calloc(size_t n, size_t m)
{
    flint_allocs++;
    return orig_calloc(n, m);
}

static void *
count_realloc(void * p, size_t n)
{
    flint_allocs++;
    return orig_realloc(p, n);
}

static void
count_start(void)
{
    __flint_get_memory_functions(&orig_alloc, &orig_calloc, &orig_realloc, &orig_free);
    flint_allocs = 0;
    __flint_set_memory_functions(count_alloc, count_calloc, count_realloc, orig_free);
}

static unsigned long
count_stop(void)
{
    __flint_set_memory_functions(orig_alloc, orig_calloc, orig_realloc, orig_free);
    return flint_allocs;
}

static ulong lcg_state = 20260929;

static slong
lcg(slong lo, slong hi)
{
    lcg_state = lcg_state * 6364136223846793005UL + 1442695040888963407UL;
    return lo + (slong) ((lcg_state >> 33) % (ulong) (hi - lo + 1));
}

/* ---- 1. against the enumeration ---- */

#define NRAND 16
#define NPLANT 24
#define NPOLY (NRAND + NPLANT)

/* The polynomials of the enumeration for the prime p: NRAND random ones of degree 1 to 4 with
   coefficients in [-3, 3], and NPLANT products of 2 to 4 linear factors with planted integer roots in
   [-6, 6]: a repeated root (k = 0 modulo 3), two roots congruent modulo p^t with t up to 6, 4, 3, 2 for
   p = 2, 3, 5, 7 (k = 1 modulo 3), a leading coefficient 1, 2, 3 or p, and a factor X^2 + 1
   (k = 1 modulo 4) or X^2 + X + 1 (k = 2 modulo 4). */
static slong
enum_polys(fmpz_poly_t * F, ulong p)
{
    slong k, i, n = 0, deg, nr, tmax;
    slong c[5], r[4];
    fmpz_poly_t t;

    tmax = p == 2 ? 6 : p == 3 ? 4 : p == 5 ? 3 : 2;
    fmpz_poly_init(t);
    for (k = 0; k < NRAND; k++)
    {
        deg = lcg(1, 4);
        for (i = 0; i <= deg; i++)
            c[i] = lcg(-3, 3);
        if (c[deg] == 0)
            c[deg] = lcg(0, 1) ? 1 : -2;
        poly_set_si(F[n++], c, deg + 1);
    }
    for (k = 0; k < NPLANT; k++)
    {
        nr = lcg(2, 4);
        for (i = 0; i < nr; i++)
            r[i] = lcg(-6, 6);
        if (k % 3 == 0)
            r[1] = r[0];                                    /* a repeated root */
        else if (k % 3 == 1)
            r[1] = r[0] + ipow(p, lcg(1, tmax)) * (lcg(0, 1) ? 1 : -1);   /* congruent modulo p^t */
        poly_from_roots(F[n], k % 4 == 3 ? (slong) p : lcg(1, 3), r, nr);
        if (k % 4 == 1)
        {
            slong q[3] = {1, 0, 1};
            poly_set_si(t, q, 3);
            fmpz_poly_mul(F[n], F[n], t);
        }
        else if (k % 4 == 2)
        {
            slong q[3] = {1, 1, 1};
            poly_set_si(t, q, 3);
            fmpz_poly_mul(F[n], F[n], t);
        }
        n++;
    }
    fmpz_poly_clear(t);
    return n;
}

#define NPREC 6
#define NDEPTH 9
#define ZCAP (1 << 20)

/* The precision of the enumeration. Let g be squarefree and nonconstant, u g + v g' = R with u, v in
   Z[X] and R != 0 (oracle_bezout_val), and L a list of adf_roots_padic_partial. Claim: if
   M >= 2 v_p(R) + 2 and M >= K + s for every ball of L, every x in Z_p with v_p(g(x)) >= M lies in a
   ball or a class of L. Proof: follow the classes of Algorithm P that contain x. At an opened class
   (a, e) with w = w(a, e) and h = g_(a,e), x = a + p^e y and g(x) = p^w h(y). (i) If b = y mod p is no
   root of h modulo p, v(g(x)) = w. Then v(g'(x)) >= w - e (from p^e g'(a + p^e Y) = p^w h'(Y),
   solvers.md:1143), so v(R) = v(u(x) g(x) + v(x) g'(x)) >= w - e; and w >= 2 e - 1 (for e = 0 as
   w >= 0; for e >= 1 the parent has a root modulo p, so w(parent) >= 2 (e - 1) by (W), solvers.md:1203,
   and w >= w(parent) + 1 by P3.4(4)); so v(R) >= (w - 1) / 2, w <= 2 v(R) + 1 < M: impossible. (ii) If
   b is a simple root, h(y) = (y - beta) times a unit on b + p Z_p, so v(g(x)) = w + v(x - alpha) - e =
   s + v(x - alpha) >= M >= K + s, and x lies in the ball of alpha. (iii) Otherwise x lies in a child,
   opened or unresolved. The levels end at depth + 1. By disjointness x then lies in exactly one. So the
   enumeration uses M = max(2 v_p(R) + 2, K + s over every ball, ue over every class) for each
   polynomial and every run of it; a larger M only makes the set of approximate roots finer. */
ADF_TEST(every_run_against_the_enumeration)
{
    static const ulong primes[4] = {2, 3, 5, 7};
    fmpz_poly_t F[NPOLY], g;
    adf_rootlist_t R[NPREC][NDEPTH], LS, LC;
    fmpz * Z;
    fmpz_t fp, zero, q;
    slong i, j, k, np, prec, depth, M, vR, nz, nb;
    ulong p;
    int st, reduced, same;
    slong n_runs = 0, n_complete = 0, n_incomplete = 0, n_empty = 0, n_spos = 0, n_skip = 0, n_bad = 0;
    slong n_balls = 0, n_nested = 0, n_x = 0, n_d0 = 0, n_below = 0;
    snap_t S;

    for (prec = 0; prec < NPREC; prec++)
        for (depth = 0; depth < NDEPTH; depth++)
            adf_rootlist_init(R[prec][depth]);
    adf_rootlist_init(LS);
    adf_rootlist_init(LC);
    fmpz_init(fp);
    fmpz_init(zero);
    fmpz_init(q);
    fmpz_poly_init(g);
    for (i = 0; i < NPOLY; i++)
        fmpz_poly_init(F[i]);
    Z = _fmpz_vec_init(ZCAP);
    for (int ip = 0; ip < 4; ip++)
    {
        p = primes[ip];
        fmpz_set_ui(fp, p);
        np = enum_polys(F, p);
        for (i = 0; i < np; i++)
        {
            oracle_normalise(g, &reduced, F[i]);
            vR = oracle_bezout_val(g, fp);
            ADF_CHECK_MSG(vR >= 0, "p = %lu, polynomial %ld: the Bezout identity of the oracle fails", p, (long) i);
            M = FLINT_MAX(2 * vR + 2, 1);
            for (prec = 1; prec <= NPREC; prec++)
                for (depth = 0; depth < NDEPTH; depth++)
                {
                    adf_rootlist_struct * L = R[prec - 1][depth];
                    st = adf_roots_padic_partial(L, F[i], place_of(p), prec, depth);
                    n_runs++;
                    ADF_CHECK_MSG(st == ADF_OK, "p = %lu, polynomial %ld, prec %ld, depth %ld: status %d", p,
                                  (long) i, (long) prec, (long) depth, st);
                    if (st != ADF_OK)
                    {
                        n_bad++;
                        adf_rootlist_clear(L);
                        adf_rootlist_init(L);
                        continue;
                    }
                    for (k = 0; k < L->n; k++)
                        M = FLINT_MAX(M, L->K[k] + L->s[k]);
                    for (k = 0; k < L->nu; k++)
                        M = FLINT_MAX(M, L->ue[k]);
                }
            nz = oracle_roots_mod(Z, ZCAP, g, p, zero, 0, M);
            if (nz < 0)
                n_skip++;
            else
                n_x += nz;
            for (prec = 1; prec <= NPREC; prec++)
                for (depth = 0; depth < NDEPTH; depth++)
                {
                    adf_rootlist_struct * L = R[prec - 1][depth];
                    int ok = 1;

                    /* the shape: place, scope, g, reduced, complete = 1 exactly when nu = 0 */
                    ok = ok && adf_place_equal(L->place, place_of(p)) && L->scope == ADF_ROOTLIST_PARTITION;
                    ok = ok && fmpz_poly_equal(L->g, g) && L->reduced == reduced;
                    ok = ok && L->complete == (L->nu == 0) && adf_rootlist_is_canonical(L);
                    ok = ok && adf_rootlist_verify_entries(L, F[i]) == 1;
                    ADF_CHECK_MSG(ok, "p = %lu, polynomial %ld, prec %ld, depth %ld: shape", p, (long) i,
                                  (long) prec, (long) depth);
                    /* K = max(prec_p, s + 1), ue = depth + 1 */
                    for (k = 0; k < L->n; k++)
                        ok = ok && L->K[k] == FLINT_MAX(prec, L->s[k] + 1);
                    for (k = 0; k < L->nu; k++)
                        ok = ok && L->ue[k] == depth + 1;
                    ADF_CHECK_MSG(ok, "p = %lu, polynomial %ld, prec %ld, depth %ld: K or ue", p, (long) i,
                                  (long) prec, (long) depth);
                    /* every approximate root modulo p^M in exactly one ball or class */
                    for (j = 0; j < nz && ok; j++)
                    {
                        nb = 0;
                        for (k = 0; k < L->n; k++)
                            nb += cong(Z + j, L->a + k, fp, L->K[k]);
                        for (k = 0; k < L->nu; k++)
                            nb += cong(Z + j, L->ua + k, fp, L->ue[k]);
                        if (nb != 1)
                        {
                            ok = 0;
                            ADF_CHECK_MSG(0, "p = %lu, polynomial %ld, prec %ld, depth %ld, M %ld: an approximate "
                                          "root lies in %ld balls or classes", p, (long) i, (long) prec,
                                          (long) depth, (long) M, (long) nb);
                        }
                    }
                    /* every ball holds p^s approximate roots modulo p^(K+s+3) in its class modulo p^(s+1) */
                    for (k = 0; k < L->n && ok; k++)
                    {
                        slong cnt = oracle_ball_count(&same, g, p, L->a + k, L->K[k], L->s[k]);
                        n_balls++;
                        if (cnt != ipow(p, L->s[k]) || !same)
                        {
                            ok = 0;
                            ADF_CHECK_MSG(0, "p = %lu, polynomial %ld, prec %ld, depth %ld: ball %ld holds %ld "
                                          "approximate roots, expected %ld", p, (long) i, (long) prec,
                                          (long) depth, (long) k, (long) cnt, (long) ipow(p, L->s[k]));
                        }
                    }
                    /* nested balls for growing precision (P3.3(1)): the same roots, every ball of
                       precision prec - 1 holds exactly one ball of precision prec, with the same s (the
                       order of the centres may change with the precision) */
                    if (prec >= 2 && ok)
                    {
                        adf_rootlist_struct * L0 = R[prec - 2][depth];
                        int nest = L0->n == L->n && L0->nu == L->nu;
                        for (k = 0; k < L0->n && nest; k++)
                        {
                            slong m = 0, t;
                            for (t = 0; t < L->n; t++)
                                m += L0->s[k] == L->s[t] && cong(L0->a + k, L->a + t, fp, L0->K[k]);
                            nest = m == 1;
                        }
                        for (k = 0; k < L->nu && nest; k++)
                            nest = fmpz_equal(L0->ua + k, L->ua + k) && L0->ue[k] == L->ue[k];
                        ADF_CHECK_MSG(nest, "p = %lu, polynomial %ld, prec %ld, depth %ld: not nested", p,
                                      (long) i, (long) prec, (long) depth);
                        n_nested++;
                        ok = ok && nest;
                    }
                    /* the strict function: NOT_DETERMINED with L untouched exactly when complete = 0 */
                    snap_take(&S, LS);
                    st = adf_roots_padic(LS, F[i], place_of(p), prec, depth);
                    if (L->complete)
                        ok = ok && st == ADF_OK && lists_equal(LS, L);
                    else
                        ok = ok && st == ADF_NOT_DETERMINED && snap_same(&S, LS);
                    snap_clear(&S);
                    ADF_CHECK_MSG(ok, "p = %lu, polynomial %ld, prec %ld, depth %ld: the strict function, "
                                  "status %d", p, (long) i, (long) prec, (long) depth, st);
                    /* the complete verifier accepts a complete list at the depth that produced it, and
                       refuses every incomplete one */
                    ADF_CHECK(adf_rootlist_verify_complete(L, F[i], depth) == L->complete);
                    if (!ok)
                        n_bad++;
                    if (L->complete)
                        n_complete++;
                    else
                        n_incomplete++;
                    if (L->n == 0 && L->nu == 0)
                        n_empty++;
                    for (k = 0; k < L->n; k++)
                        if (L->s[k] > 0)
                        {
                            n_spos++;
                            break;
                        }
                }
            /* the least depth D0 that completes the run at prec 1: the list of depth 8 is accepted at
               D0 and refused at D0 - 1 (P3.13(2): the rerun leaves a class unresolved) */
            for (depth = 0; depth < NDEPTH && !R[0][depth]->complete; depth++)
                ;
            if (depth < NDEPTH)
            {
                n_d0++;
                ADF_CHECK(adf_rootlist_verify_complete(R[0][NDEPTH - 1], F[i], depth) == 1);
                if (depth >= 1)
                {
                    n_below++;
                    ADF_CHECK(adf_rootlist_verify_complete(R[0][NDEPTH - 1], F[i], depth - 1) == 0);
                }
            }
        }
    }
    printf("enumeration: %ld runs (p = 2, 3, 5, 7; %d polynomials each; prec 1 to %d, depth 0 to %d): "
           "complete %ld, incomplete %ld, empty %ld, with s > 0 %ld; %ld approximate roots, %ld balls counted, "
           "%ld nested pairs; %ld polynomials skipped (enumeration above %d); least depth found for %ld, "
           "of them >= 1 for %ld; %ld failures\n", (long) n_runs, NPOLY, NPREC, NDEPTH - 1, (long) n_complete,
           (long) n_incomplete, (long) n_empty, (long) n_spos, (long) n_x, (long) n_balls, (long) n_nested,
           (long) n_skip, ZCAP, (long) n_d0, (long) n_below, (long) n_bad);
    ADF_CHECK(n_complete > 0 && n_incomplete > 0 && n_empty > 0 && n_spos > 0 && n_below > 0);
    ADF_CHECK(n_skip == 0);
    _fmpz_vec_clear(Z, ZCAP);
    for (i = 0; i < NPOLY; i++)
        fmpz_poly_clear(F[i]);
    fmpz_poly_clear(g);
    fmpz_clear(fp);
    fmpz_clear(zero);
    fmpz_clear(q);
    adf_rootlist_clear(LS);
    adf_rootlist_clear(LC);
    for (prec = 0; prec < NPREC; prec++)
        for (depth = 0; depth < NDEPTH; depth++)
            adf_rootlist_clear(R[prec][depth]);
}

/* ---- 2. the vectors of the reference ---- */

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

/* 1 if the certificates and classes of L are those of the vector line (arrays [a, K, s] and [a, e]) */
static int
list_matches(const adf_rootlist_t L, const jsonl_value * certs, const jsonl_value * unres)
{
    fmpz_t x;
    slong i, K = 0, s = 0;
    int ok;

    if (L->n != (slong) jsonl_size(certs) || L->nu != (slong) jsonl_size(unres))
        return 0;
    fmpz_init(x);
    ok = 1;
    for (i = 0; i < L->n && ok; i++)
    {
        const jsonl_value * c = jsonl_at(certs, (size_t) i, NULL);
        ok = jsonl_size(c) == 3 && read_fmpz(x, jsonl_at(c, 0, NULL)) && read_si(&K, jsonl_at(c, 1, NULL)) &&
             read_si(&s, jsonl_at(c, 2, NULL));
        ok = ok && fmpz_equal(x, L->a + i) && K == L->K[i] && s == L->s[i];
    }
    for (i = 0; i < L->nu && ok; i++)
    {
        const jsonl_value * c = jsonl_at(unres, (size_t) i, NULL);
        ok = jsonl_size(c) == 2 && read_fmpz(x, jsonl_at(c, 0, NULL)) && read_si(&K, jsonl_at(c, 1, NULL));
        ok = ok && fmpz_equal(x, L->ua + i) && K == L->ue[i];
    }
    fmpz_clear(x);
    return ok;
}

ADF_TEST(vectors_s2_slice2_every_line)
{
    const char * path = "tests/ref/vectors/s2-slice2/padic.jsonl";
    jsonl_file * file;
    jsonl_error_t err;
    const jsonl_value * rec, * v, * vc, * vu;
    fmpz_poly_t f, g;
    fmpz_t pz;
    adf_rootlist_t LS, LP;
    size_t i, len;
    slong prec = 0, depth = 0, red = 0, cmp = 0, expect = 0, d0 = 0;
    const char * st_text = NULL, * what;
    int st, stp, want;
    slong n_ok = 0, n_nd = 0, n_dom = 0, n_bad = 0, n_named = 0, n_expect = 0, n_d0 = 0, n_below = 0;
    snap_t S;

    if (!jsonl_open(path, &file, &err))
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    fmpz_poly_init(f);
    fmpz_poly_init(g);
    fmpz_init(pz);
    adf_rootlist_init(LS);
    adf_rootlist_init(LP);
    for (i = 0; i < jsonl_count(file); i++)
    {
        int ok = 1;
        rec = jsonl_record(file, i);
        ok = ok && jsonl_field(rec, "f", &v, &err) && read_poly(f, v);
        ok = ok && jsonl_field(rec, "p", &v, &err) && read_fmpz(pz, v) && fmpz_abs_fits_ui(pz);
        ok = ok && jsonl_field(rec, "prec", &v, &err) && read_si(&prec, v);
        ok = ok && jsonl_field(rec, "depth", &v, &err) && read_si(&depth, v);
        ok = ok && jsonl_field(rec, "status", &v, &err) && (st_text = jsonl_string(v, &len, &err)) != NULL;
        ADF_CHECK_MSG(ok, "line %lu: unreadable", (unsigned long) i + 1);
        if (!ok)
            break;
        want = strcmp(st_text, "OK") == 0 ? ADF_OK
             : strcmp(st_text, "NOT_DETERMINED") == 0 ? ADF_NOT_DETERMINED
             : strcmp(st_text, "DOMAIN") == 0 ? ADF_DOMAIN : -1;
        snap_take(&S, LS);
        st = adf_roots_padic(LS, f, place_of(fmpz_get_ui(pz)), prec, depth);
        stp = adf_roots_padic_partial(LP, f, place_of(fmpz_get_ui(pz)), prec, depth);
        if (st != want || stp != (want == ADF_DOMAIN ? ADF_DOMAIN : ADF_OK))
        {
            n_bad++;
            ADF_CHECK_MSG(0, "line %lu: statuses %d, %d, reference %s", (unsigned long) i + 1, st, stp, st_text);
            snap_clear(&S);
            continue;
        }
        if (st != ADF_OK)
            ADF_CHECK_MSG(snap_same(&S, LS), "line %lu: L touched on status %d", (unsigned long) i + 1, st);
        snap_clear(&S);
        if (st == ADF_DOMAIN)
        {
            n_dom++;
            continue;
        }
        if (st == ADF_OK)
            n_ok++;
        else
            n_nd++;
        ok = jsonl_field(rec, "g", &v, &err) && read_poly(g, v);
        ok = ok && jsonl_field(rec, "reduced", &v, &err) && read_si(&red, v);
        ok = ok && jsonl_field(rec, "complete", &v, &err) && read_si(&cmp, v);
        ok = ok && jsonl_field(rec, "certs", &vc, &err) && jsonl_field(rec, "unres", &vu, &err);
        ok = ok && fmpz_poly_equal(g, LP->g) && LP->reduced == red && LP->complete == cmp;
        ok = ok && list_matches(LP, vc, vu) && LP->scope == ADF_ROOTLIST_PARTITION;
        ok = ok && adf_place_equal(LP->place, place_of(fmpz_get_ui(pz)));
        ok = ok && adf_rootlist_is_canonical(LP) && adf_rootlist_verify_entries(LP, f) == 1;
        if (st == ADF_OK)
            ok = ok && lists_equal(LS, LP);
        if (!ok)
        {
            n_bad++;
            ADF_CHECK_MSG(0, "line %lu: the list differs from the reference", (unsigned long) i + 1);
            continue;
        }
        if (jsonl_field(rec, "what", &v, &err) && (what = jsonl_string(v, &len, &err)) != NULL &&
            strncmp(what, "PADIC_CASES", 11) == 0)
            n_named++;
        /* the number of roots in Z_p of PADIC_CASES, at the depth 8 and above */
        if (jsonl_field(rec, "expect", &v, &err) && !jsonl_is_null(v, NULL) && depth >= 8)
        {
            ADF_CHECK_MSG(read_si(&expect, v) && LP->complete == 1 && LP->n == expect, "line %lu",
                          (unsigned long) i + 1);
            n_expect++;
        }
        /* the complete verifier: a complete list is accepted at its depth and at the least depth d0 that
           completes the run, and refused at d0 - 1; an incomplete list is refused */
        if (LP->complete)
            ADF_CHECK_MSG(adf_rootlist_verify_complete(LP, f, depth) == 1, "line %lu", (unsigned long) i + 1);
        else
            ADF_CHECK(adf_rootlist_verify_complete(LP, f, 12) == 0);
        if (LP->complete && jsonl_field(rec, "d0", &v, &err) && !jsonl_is_null(v, NULL) && read_si(&d0, v))
        {
            n_d0++;
            ADF_CHECK_MSG(adf_rootlist_verify_complete(LP, f, d0) == 1, "line %lu", (unsigned long) i + 1);
            if (d0 >= 1)
            {
                n_below++;
                ADF_CHECK_MSG(adf_rootlist_verify_complete(LP, f, d0 - 1) == 0, "line %lu", (unsigned long) i + 1);
            }
        }
    }
    printf("vectors: %lu lines (%ld of PADIC_CASES), OK %ld, NOT_DETERMINED %ld, DOMAIN %ld; numbers of roots "
           "compared %ld; least depths %ld (%ld refused one below); %ld differ\n",
           (unsigned long) jsonl_count(file), (long) n_named, (long) n_ok, (long) n_nd, (long) n_dom,
           (long) n_expect, (long) n_d0, (long) n_below, (long) n_bad);
    ADF_CHECK(jsonl_count(file) > 600 && n_named == 31 * 8 && n_ok > 0 && n_nd > 0 && n_dom > 0);
    ADF_CHECK(n_expect > 0 && n_below > 0 && n_bad == 0);
    jsonl_close(file);
    adf_rootlist_clear(LS);
    adf_rootlist_clear(LP);
    fmpz_poly_clear(f);
    fmpz_poly_clear(g);
    fmpz_clear(pz);
}

/* ---- 3. fixed cases ---- */

ADF_TEST(x2_plus_1_at_2_and_the_examples_of_3_11)
{
    static const slong c1[3] = {1, 0, 1}, c9[3] = {-9, 0, 1}, c3[3] = {-3, 0, 1}, cx[4] = {0, 2, 0, 1};
    fmpz_poly_t f;
    fmpz_t a;
    adf_rootlist_t L;
    slong e = 0, K = 0, s = 0, d;
    snap_t S;

    fmpz_poly_init(f);
    fmpz_init(a);
    adf_rootlist_init(L);
    /* api-s.md 4, note 1; solvers 3.11(2): x^2 + 1 at 2 */
    poly_set_si(f, c1, 3);
    snap_take(&S, L);
    ADF_CHECK(adf_roots_padic(L, f, place_of(2), 3, 0) == ADF_NOT_DETERMINED);
    ADF_CHECK(snap_same(&S, L));
    snap_clear(&S);
    ADF_CHECK(adf_roots_padic_partial(L, f, place_of(2), 3, 0) == ADF_OK);
    ADF_CHECK(L->complete == 0 && L->n == 0 && L->nu == 1 && adf_rootlist_unresolved_length(L) == 1);
    ADF_CHECK(adf_rootlist_get_unresolved(a, &e, L, 0) == 1 && fmpz_equal_si(a, 1) && e == 1);
    for (d = 1; d <= 4; d++)
    {
        ADF_CHECK(adf_roots_padic(L, f, place_of(2), 3, d) == ADF_OK);
        ADF_CHECK(L->complete == 1 && L->n == 0 && L->nu == 0 && L->a == NULL && L->ua == NULL);
    }
    /* 3.11(3): x^2 - 9 at 2: the roots 3 and -3 in 3 + 4 Z_2 and 1 + 4 Z_2, s = 1 */
    poly_set_si(f, c9, 3);
    ADF_CHECK(adf_roots_padic(L, f, place_of(2), 4, 8) == ADF_OK && L->n == 2);
    ADF_CHECK(adf_rootlist_get_cert(a, &K, &s, L, 0) && fmpz_equal_si(a, 3) && K == 4 && s == 1);
    ADF_CHECK(adf_rootlist_get_cert(a, &K, &s, L, 1) && fmpz_equal_si(a, 13) && K == 4 && s == 1);
    ADF_CHECK(adf_roots_padic(L, f, place_of(2), 1, 8) == ADF_OK && L->n == 2 && L->K[0] == 2 && L->K[1] == 2);
    /* 3.11(4): x^2 - 3 at 2: no root, complete */
    poly_set_si(f, c3, 3);
    ADF_CHECK(adf_roots_padic(L, f, place_of(2), 4, 8) == ADF_OK && L->n == 0 && L->complete == 1);
    /* 3.11(5): X^3 + 2 X at 2: the only root 0 with s = 1, K = s + 1 = 2 for prec 1 (not the least
       isolating precision, which is 0) */
    poly_set_si(f, cx, 4);
    ADF_CHECK(adf_roots_padic(L, f, place_of(2), 1, 8) == ADF_OK && L->n == 1);
    ADF_CHECK(adf_rootlist_get_cert(a, &K, &s, L, 0) && fmpz_is_zero(a) && K == 2 && s == 1);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_clear(a);
}

ADF_TEST(zero_polynomial_constants_degree_one_and_repeated_factors)
{
    static const slong r[3] = {1, 1, -2};
    fmpz_poly_t f;
    fmpz_t a, t;
    adf_rootlist_t L;
    slong K = 0, s = 0, c;
    snap_t S;

    fmpz_poly_init(f);
    fmpz_init(a);
    fmpz_init(t);
    adf_rootlist_init(L);
    /* the zero polynomial: DOMAIN, L untouched */
    snap_take(&S, L);
    ADF_CHECK(adf_roots_padic(L, f, place_of(3), 2, 3) == ADF_DOMAIN);
    ADF_CHECK(adf_roots_padic_partial(L, f, place_of(3), 2, 3) == ADF_DOMAIN);
    ADF_CHECK(snap_same(&S, L));
    snap_clear(&S);
    /* constants: the empty complete list, g = 1, reduced 0 */
    for (c = -4; c <= 12; c += 4)
    {
        fmpz_poly_set_si(f, c == 0 ? 9 : c);
        ADF_CHECK(adf_roots_padic(L, f, place_of(3), 2, 0) == ADF_OK);
        ADF_CHECK(L->n == 0 && L->nu == 0 && L->complete == 1 && fmpz_poly_is_one(L->g) && L->reduced == 0);
        ADF_CHECK(adf_place_equal(L->place, place_of(3)) && adf_rootlist_is_canonical(L));
        ADF_CHECK(adf_rootlist_verify_complete(L, f, 0) == 1);
    }
    /* degree 1: 27 X at 3 gives g = X and (0, K, 0) (solvers P3.12(3), the same for the search) */
    fmpz_poly_zero(f);
    fmpz_poly_set_coeff_si(f, 1, 27);
    ADF_CHECK(adf_roots_padic(L, f, place_of(3), 5, 0) == ADF_OK && L->n == 1);
    ADF_CHECK(adf_rootlist_get_cert(a, &K, &s, L, 0) && fmpz_is_zero(a) && K == 5 && s == 0);
    /* 4 X - 6 at 2: g = 2 X - 3, the root 3/2 is not in Z_2 */
    fmpz_poly_set_coeff_si(f, 1, 4);
    fmpz_poly_set_coeff_si(f, 0, -6);
    ADF_CHECK(adf_roots_padic(L, f, place_of(2), 5, 0) == ADF_OK && L->n == 0 && L->complete == 1);
    /* at 3 the root 3/2 is in Z_3: 2 a = 3 modulo 3^5 */
    ADF_CHECK(adf_roots_padic(L, f, place_of(3), 5, 0) == ADF_OK && L->n == 1);
    ADF_CHECK(adf_rootlist_get_cert(a, &K, &s, L, 0) && K == 5 && s == 0);
    fmpz_mul_ui(t, a, 2);
    fmpz_sub_ui(t, t, 3);
    ADF_CHECK(fmpz_divisible_si(t, 243));
    /* (X - 1)^2 (X + 2) at 3 (S-D13): two distinct roots, reduced = 1, complete */
    poly_from_roots(f, 1, r, 3);
    ADF_CHECK(adf_roots_padic(L, f, place_of(3), 2, 12) == ADF_OK && L->n == 2 && L->reduced == 1);
    ADF_CHECK(adf_rootlist_verify_complete(L, f, 12) == 1);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_clear(a);
    fmpz_clear(t);
}

/* ---- 4. the complete verifier ---- */

enum { CH_DROP, CH_MERGE, CH_MOVE, CH_DOUBLE, CH_FALSE_N, CH_CLASS, CH_BELOW, CH_KINDS };

static const char * ch_name[CH_KINDS] = {"a certificate dropped", "two balls merged", "a ball moved off its root",
                                          "a ball doubled", "a false n", "a class kept with complete = 1",
                                          "a depth one below the least"};

/* The changed lists of one polynomial (solvers P3.13(2); check_s2_lists, proto/solvers_checks.py:2076):
   each must be refused by adf_rootlist_verify_complete. tried[k] and refused[k] count them by kind. */
static void
changed_lists(slong * tried, slong * refused, const fmpz_poly_t f, ulong p)
{
    adf_rootlist_t L, C;
    fmpz_t fp, d, q;
    slong i, m, n, D0, depth;
    int r;

    adf_rootlist_init(L);
    adf_rootlist_init(C);
    fmpz_init_set_ui(fp, p);
    fmpz_init(d);
    fmpz_init(q);
    /* an incomplete list with complete = 1 */
    for (depth = 0; depth <= 3; depth++)
        if (adf_roots_padic_partial(L, f, place_of(p), 2, depth) == ADF_OK && L->nu > 0)
        {
            list_copy_drop(C, L, -1);
            C->complete = 1;
            tried[CH_CLASS]++;
            r = adf_rootlist_verify_complete(C, f, 12);
            ADF_CHECK_MSG(r == 0, "p = %lu, depth %ld: %s accepted", p, (long) depth, ch_name[CH_CLASS]);
            refused[CH_CLASS] += r == 0;
        }
    if (adf_roots_padic(L, f, place_of(p), 2, 12) != ADF_OK)
        goto done;
    ADF_CHECK(adf_rootlist_verify_complete(L, f, 12) == 1);
    /* the least depth D0 that completes the run; one below is refused */
    for (D0 = 0; D0 <= 12; D0++)
    {
        adf_rootlist_t T;
        int st, cmp;

        adf_rootlist_init(T);
        st = adf_roots_padic_partial(T, f, place_of(p), 2, D0);
        cmp = st == ADF_OK && T->complete;
        adf_rootlist_clear(T);
        if (cmp)
            break;
    }
    if (D0 >= 1 && D0 <= 12)
    {
        tried[CH_BELOW]++;
        r = adf_rootlist_verify_complete(L, f, D0 - 1);
        ADF_CHECK_MSG(r == 0, "p = %lu: the depth %ld accepted", p, (long) D0 - 1);
        refused[CH_BELOW] += r == 0;
        ADF_CHECK(adf_rootlist_verify_complete(L, f, D0) == 1);
    }
    n = L->n;
    for (i = 0; i < n; i++)
    {
        /* dropped: the entries still hold, the list is short */
        list_copy_drop(C, L, i);
        tried[CH_DROP]++;
        ADF_CHECK(adf_rootlist_verify_entries(C, f) == 1);
        r = adf_rootlist_verify_complete(C, f, 12);
        ADF_CHECK_MSG(r == 0, "p = %lu: %s accepted", p, ch_name[CH_DROP]);
        refused[CH_DROP] += r == 0;
        /* moved: the centre + p^(K-1) modulo p^K */
        list_copy_drop(C, L, -1);
        fmpz_pow_ui(q, fp, (ulong) C->K[i] - 1);
        fmpz_add(C->a + i, C->a + i, q);
        fmpz_mul_ui(q, q, p);
        fmpz_mod(C->a + i, C->a + i, q);
        tried[CH_MOVE]++;
        r = adf_rootlist_verify_complete(C, f, 12);
        ADF_CHECK_MSG(r == 0, "p = %lu: %s accepted", p, ch_name[CH_MOVE]);
        refused[CH_MOVE] += r == 0;
        /* doubled: ball i twice (the arrays of n + 1 entries) */
        {
            adf_rootlist_t E;
            slong j;

            adf_rootlist_init(E);
            list_copy_drop(E, L, -1);
            _fmpz_vec_clear(E->a, E->n);
            flint_free(E->K);
            flint_free(E->s);
            E->n = n + 1;
            E->a = _fmpz_vec_init(n + 1);
            E->K = flint_malloc((n + 1) * sizeof(slong));
            E->s = flint_malloc((n + 1) * sizeof(slong));
            for (j = 0; j <= n; j++)
            {
                slong k = j <= i ? j : j - 1;
                fmpz_set(E->a + j, L->a + k);
                E->K[j] = L->K[k];
                E->s[j] = L->s[k];
            }
            tried[CH_DOUBLE]++;
            r = adf_rootlist_verify_complete(E, f, 12);
            ADF_CHECK_MSG(r == 0, "p = %lu: %s accepted", p, ch_name[CH_DOUBLE]);
            refused[CH_DOUBLE] += r == 0;
            adf_rootlist_clear(E);
        }
        /* merged: balls i and i + 1 replaced by the least ball that holds both */
        if (i + 1 < n)
        {
            list_copy_drop(C, L, i + 1);
            fmpz_sub(d, L->a + i, L->a + i + 1);
            /* equal centres cannot come from a correct list (the balls are disjoint); the guard keeps the
               test finite on a defective one */
            m = fmpz_is_zero(d) ? FLINT_MIN(L->K[i], L->K[i + 1]) : vp(d, fp);
            fmpz_pow_ui(q, fp, (ulong) m);
            fmpz_mod(C->a + i, L->a + i, q);
            C->K[i] = m;
            tried[CH_MERGE]++;
            r = adf_rootlist_verify_complete(C, f, 12);
            ADF_CHECK_MSG(r == 0, "p = %lu: %s accepted", p, ch_name[CH_MERGE]);
            refused[CH_MERGE] += r == 0;
        }
    }
    if (n >= 1)
    {
        /* a false n: n - 1 with the arrays of n entries (restored before the clear) */
        list_copy_drop(C, L, -1);
        C->n = n - 1;
        tried[CH_FALSE_N]++;
        r = adf_rootlist_verify_complete(C, f, 12);
        ADF_CHECK_MSG(r == 0, "p = %lu: %s accepted", p, ch_name[CH_FALSE_N]);
        refused[CH_FALSE_N] += r == 0;
        C->n = n;
    }
done:
    adf_rootlist_clear(L);
    adf_rootlist_clear(C);
    fmpz_clear(fp);
    fmpz_clear(d);
    fmpz_clear(q);
}

ADF_TEST(verify_complete_refuses_changed_lists_seed_lists_and_the_example)
{
    static const ulong primes[4] = {2, 3, 5, 7};
    static const slong cxx[3] = {0, -1, 1};
    fmpz_poly_t F[NPOLY], f;
    fmpz_t a;
    adf_rootlist_t L;
    slong tried[CH_KINDS] = {0}, refused[CH_KINDS] = {0}, i, k, prec, n_seed = 0, n_seed_ref = 0;
    ulong p, ai;
    int ok = 1;

    for (i = 0; i < NPOLY; i++)
        fmpz_poly_init(F[i]);
    fmpz_poly_init(f);
    fmpz_init(a);
    adf_rootlist_init(L);
    for (int ip = 0; ip < 4; ip++)
    {
        p = primes[ip];
        k = enum_polys(F, p);
        for (i = 0; i < k; i++)
        {
            changed_lists(tried, refused, F[i], p);
            /* no list of the seed function passes (P3.13(3)) */
            for (ai = 0; ai < p * p; ai++)
                for (prec = 1; prec <= 3; prec += 2)
                {
                    fmpz_set_ui(a, ai);
                    if (adf_root_padic_from_seed(L, F[i], place_of(p), a, prec) != ADF_OK)
                        continue;
                    n_seed++;
                    if (adf_rootlist_verify_complete(L, F[i], 12) == 0)
                        n_seed_ref++;
                }
        }
    }
    /* solvers P3.13, example (a): X (X - 1) at 3, the empty list called complete */
    poly_set_si(f, cxx, 3);
    adf_rootlist_clear(L);
    adf_rootlist_init(L);
    L->place = place_of(3);
    fmpz_poly_set(L->g, f);
    ADF_CHECK(adf_rootlist_is_canonical(L) && L->complete == 1 && L->n == 0);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1);
    ADF_CHECK(adf_rootlist_verify_complete(L, f, 12) == 0);
    /* the true list is accepted */
    ADF_CHECK(adf_roots_padic(L, f, place_of(3), 1, 0) == ADF_OK && L->n == 2);
    ADF_CHECK(adf_rootlist_verify_complete(L, f, 0) == 1);
    /* depth < 0, the real place */
    ADF_CHECK(adf_rootlist_verify_complete(L, f, -1) == 0);
    ADF_CHECK(adf_rootlist_verify_complete(L, f, WORD_MIN) == 0);
    fmpz_poly_zero(f);
    ADF_CHECK(adf_rootlist_verify_complete(L, f, 3) == 0);
    adf_rootlist_clear(L);
    adf_rootlist_init(L);
    fmpz_poly_set_ui(f, 1);
    ADF_CHECK(adf_rootlist_verify_complete(L, f, 3) == 1);        /* the real place: the true list of f = 1 */
    printf("changed lists:");
    for (k = 0; k < CH_KINDS; k++)
    {
        printf(" %s %ld of %ld refused;", ch_name[k], (long) refused[k], (long) tried[k]);
        ok = ok && tried[k] > 0 && refused[k] == tried[k];
    }
    printf(" seed lists %ld of %ld refused\n", (long) n_seed_ref, (long) n_seed);
    ADF_CHECK(ok);
    ADF_CHECK(n_seed > 0 && n_seed_ref == n_seed);
    for (i = 0; i < NPOLY; i++)
        fmpz_poly_clear(F[i]);
    fmpz_poly_clear(f);
    fmpz_clear(a);
    adf_rootlist_clear(L);
}

/* ---- 5. statuses and limits ---- */

/* the partial or the strict function with FLINT's allocator counted */
static int
roots_counted(unsigned long * count, int strict, adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec,
              slong depth)
{
    int st;

    count_start();
    st = strict ? adf_roots_padic(L, f, p, prec, depth) : adf_roots_padic_partial(L, f, p, prec, depth);
    *count = count_stop();
    return st;
}

ADF_TEST(domain_unsupported_and_limit_before_any_allocation_L_untouched)
{
    static const slong c2[3] = {-2, 0, 1}, r2[2] = {5, -11};
    fmpz_poly_t f, z;
    adf_rootlist_t L;
    snap_t S;
    unsigned long cnt;
    ulong pbig = UWORD(1048583), pmax = UWORD(1048573), p64 = UWORD(18446744073709551557);
    int strict, bad = 0;

    fmpz_poly_init(f);
    fmpz_poly_init(z);
    adf_rootlist_init(L);
    poly_set_si(f, c2, 3);
    ADF_CHECK(adf_roots_padic(L, f, place_of(7), 5, 3) == ADF_OK && L->n == 2);
    snap_take(&S, L);
    for (strict = 0; strict <= 1; strict++)
    {
#define EXPECT(st_want, pl, poly, prec, depth)                                                          \
        do                                                                                              \
        {                                                                                               \
            int st_ = roots_counted(&cnt, strict, L, poly, pl, prec, depth);                            \
            if (st_ != (st_want) || cnt != 0 || !snap_same(&S, L))                                      \
            {                                                                                           \
                bad++;                                                                                  \
                ADF_CHECK_MSG(0, "strict %d, prec %ld, depth %ld: status %d (want %d), %lu allocations", \
                              strict, (long) (prec), (long) (depth), st_, st_want, cnt);                \
            }                                                                                           \
        }                                                                                               \
        while (0)
        /* DOMAIN: f = 0, the real place, prec_p < 1, depth < 0 */
        EXPECT(ADF_DOMAIN, place_of(7), z, 5, 3);
        EXPECT(ADF_DOMAIN, adf_place_inf(), f, 5, 3);
        EXPECT(ADF_DOMAIN, place_of(7), f, 0, 3);
        EXPECT(ADF_DOMAIN, place_of(7), f, -1, 3);
        EXPECT(ADF_DOMAIN, place_of(7), f, WORD_MIN, 3);
        EXPECT(ADF_DOMAIN, place_of(7), f, 5, -1);
        EXPECT(ADF_DOMAIN, place_of(7), f, 5, WORD_MIN);
        EXPECT(ADF_DOMAIN, place_of(pbig), f, 5, -1);            /* DOMAIN before UNSUPPORTED */
        /* UNSUPPORTED (TEMPORARY, S-D10): p just above 2^20, and before LIMIT */
        EXPECT(ADF_UNSUPPORTED, place_of(pbig), f, 5, 3);
        EXPECT(ADF_UNSUPPORTED, place_of(pbig), f, WORD_MAX, 3);
        EXPECT(ADF_UNSUPPORTED, place_of(p64), f, 5, 3);
        /* LIMIT of the precision: 2 prec bits(p) > 2^24 */
        EXPECT(ADF_LIMIT, place_of(7), f, ADF_ROOTS_BITS_MAX / 6 + 1, 3);
        EXPECT(ADF_LIMIT, place_of(7), f, WORD_MAX, WORD_MAX);
        EXPECT(ADF_LIMIT, place_of(2), f, ADF_ROOTS_BITS_MAX / 4 + 1, 0);
        EXPECT(ADF_LIMIT, place_of(pmax), f, ADF_ROOTS_BITS_MAX / 40 + 1, 0);
#undef EXPECT
    }
    ADF_CHECK(bad == 0);
    /* the largest prime below the bound works: (X - 5)(X + 11) at 1048573 */
    poly_from_roots(f, 1, r2, 2);
    ADF_CHECK(adf_roots_padic(L, f, place_of(pmax), 3, 2) == ADF_OK && L->n == 2 && L->complete == 1);
    ADF_CHECK(adf_rootlist_verify_complete(L, f, 2) == 1);
    ADF_CHECK(pbig > ADF_ROOTS_P_EVAL_MAX && pmax <= ADF_ROOTS_P_EVAL_MAX && ADF_ROOTS_P_EVAL_MAX == 1048576);
    snap_clear(&S);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_poly_clear(z);
}

ADF_TEST(limits_during_the_search_and_exponent_overflow)
{
    fmpz_poly_t f, h;
    fmpz_t a, p, pe, ap;
    adf_rootlist_t L;
    snap_t S;
    slong K = -7, s = -7, i;
    unsigned long cnt;
    int st;
    /* (e, w, prec): the exponents of the certification near WORD_MAX; each is LIMIT with no allocation */
    static const slong cases[][3] = {
        {0, WORD_MAX, 1},                   /* j = w - 2 e + 1 overflows */
        {WORD_MAX / 2 + 1, WORD_MAX, 1},    /* 2 e overflows */
        {WORD_MAX, WORD_MAX, 1},            /* 2 e overflows */
        {1, WORD_MAX, 1},                   /* s + 1 = WORD_MAX, K beyond the bits */
        {0, WORD_MAX - 1, 1},               /* e + j = WORD_MAX, K beyond the bits */
        {3, 7, WORD_MAX},                   /* K = prec_p beyond the bits */
        {0, 0, WORD_MAX},
        {10, 4194304 + 10, 1},              /* s = 2^22, K = s + 1: 2 K bits(2) = 2^24 + 4 */
    };

    fmpz_poly_init(f);
    fmpz_poly_init(h);
    fmpz_init(a);
    fmpz_init_set_ui(p, 2);
    fmpz_init_set_ui(pe, 1);
    fmpz_init(ap);
    adf_rootlist_init(L);
    /* X (X - 2^10) at 2: the roots 0 and 2^10 with s = 10 separate at the level 10; K = 11 for prec 1 */
    fmpz_poly_set_coeff_si(f, 2, 1);
    fmpz_poly_set_coeff_si(f, 1, -1024);
    ADF_CHECK(adf_roots_padic_core(L, f, place_of(2), 1, 20, 1, 44) == ADF_OK && L->n == 2);
    ADF_CHECK(L->n == 2 && L->K[0] == 11 && L->s[0] == 10 && L->K[1] == 11 && L->s[1] == 10);
    snap_take(&S, L);
    /* 2 K bits(p) = 44 > 43: LIMIT during the search, L untouched, for both functions */
    ADF_CHECK(adf_roots_padic_core(L, f, place_of(2), 1, 20, 1, 43) == ADF_LIMIT);
    ADF_CHECK(adf_roots_padic_core(L, f, place_of(2), 1, 20, 0, 43) == ADF_LIMIT);
    ADF_CHECK(snap_same(&S, L));
    /* a class of exponent 6 at depth 5: 2 e bits(p) = 24 */
    ADF_CHECK(adf_roots_padic_core(L, f, place_of(2), 1, 5, 0, 23) == ADF_LIMIT);
    ADF_CHECK(snap_same(&S, L));
    snap_clear(&S);
    ADF_CHECK(adf_roots_padic_core(L, f, place_of(2), 1, 5, 0, 24) == ADF_OK && L->nu == 1 && L->ue[0] == 6);
    ADF_CHECK(adf_roots_padic_core(L, f, place_of(2), 1, 5, 1, 24) == ADF_NOT_DETERMINED);
    /* the certification of one digit: X^2 - 2 at 7, level 0, digit 3, prec 5 */
    fmpz_poly_zero(f);
    fmpz_poly_set_coeff_si(f, 2, 1);
    fmpz_poly_set_coeff_si(f, 0, -2);
    fmpz_set_ui(p, 7);
    ADF_CHECK(adf_roots_certify(ap, &K, &s, f, f, p, a, pe, 0, 0, 3, 5, ADF_ROOTS_BITS_MAX) == ADF_OK);
    ADF_CHECK(K == 5 && s == 0 && fmpz_fdiv_ui(ap, 7) == 3);
    fmpz_mul(a, ap, ap);
    fmpz_sub_ui(a, a, 2);
    ADF_CHECK(fmpz_divisible_si(a, 16807));
    fmpz_zero(a);
    /* the exponents near WORD_MAX */
    fmpz_set_ui(p, 2);
    fmpz_set(ap, pe);
    for (i = 0; i < (slong) (sizeof(cases) / sizeof(cases[0])); i++)
    {
        K = -7;
        s = -7;
        count_start();
        st = adf_roots_certify(ap, &K, &s, f, f, p, a, pe, cases[i][0], cases[i][1], 1, cases[i][2],
                               ADF_ROOTS_BITS_MAX);
        cnt = count_stop();
        ADF_CHECK_MSG(st == ADF_LIMIT && cnt == 0 && K == -7 && s == -7 && fmpz_is_one(ap),
                      "case %ld: status %d, %lu allocations", (long) i, st, cnt);
    }
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_poly_clear(h);
    fmpz_clear(a);
    fmpz_clear(p);
    fmpz_clear(pe);
    fmpz_clear(ap);
}

/* ---- 6. accessors and aliasing ---- */

ADF_TEST(get_unresolved_and_aliasing)
{
    static const slong r[3] = {3, 3 + 2 * 81, -1};
    fmpz_poly_t f, g;
    fmpz_t a;
    adf_rootlist_t L, M;
    slong e = -7, i;

    fmpz_poly_init(f);
    fmpz_poly_init(g);
    fmpz_init_set_si(a, 99);
    adf_rootlist_init(L);
    adf_rootlist_init(M);
    /* the real place and an index outside: 0, untouched */
    ADF_CHECK(adf_rootlist_get_unresolved(a, &e, L, 0) == 0 && fmpz_equal_si(a, 99) && e == -7);
    /* (X - 3)(X - 3 - 2 3^4)(X + 1) at 3, depth 2: the class 3 + 3^3 Z_3 holds two roots */
    poly_from_roots(f, 1, r, 3);
    ADF_CHECK(adf_roots_padic_partial(L, f, place_of(3), 2, 2) == ADF_OK);
    ADF_CHECK(L->complete == 0 && L->nu == 1 && L->n == 1);
    ADF_CHECK(adf_rootlist_get_unresolved(a, &e, L, 0) == 1 && fmpz_equal_si(a, 3) && e == 3);
    e = -7;
    fmpz_set_si(a, 99);
    ADF_CHECK(adf_rootlist_get_unresolved(a, &e, L, 1) == 0 && fmpz_equal_si(a, 99) && e == -7);
    ADF_CHECK(adf_rootlist_get_unresolved(a, &e, L, -1) == 0 && fmpz_equal_si(a, 99) && e == -7);
    ADF_CHECK(L->ua != NULL && adf_rootlist_get_unresolved(L->ua + 0, &e, L, 0) == 1 &&
              fmpz_equal_si(L->ua + 0, 3) && e == 3);
    /* the complete list at depth 4 */
    ADF_CHECK(adf_roots_padic(L, f, place_of(3), 2, 4) == ADF_OK && L->n == 3);
    ADF_CHECK(adf_rootlist_get_unresolved(a, &e, L, 0) == 0);
    /* aliasing: f = L->g, for both functions; the result is that of a copy of L->g */
    fmpz_poly_set(g, L->g);
    for (i = 0; i < 2; i++)
    {
        int st1 = i ? adf_roots_padic(M, g, place_of(3), 5, 4)
                    : adf_roots_padic_partial(M, g, place_of(3), 5, 1);
        int st2 = i ? adf_roots_padic(L, L->g, place_of(3), 5, 4)
                    : adf_roots_padic_partial(L, L->g, place_of(3), 5, 1);
        ADF_CHECK(st1 == ADF_OK && st2 == ADF_OK && lists_equal(L, M));
        fmpz_poly_set(g, L->g);
    }
    adf_rootlist_clear(L);
    adf_rootlist_clear(M);
    fmpz_poly_clear(f);
    fmpz_poly_clear(g);
    fmpz_clear(a);
}
