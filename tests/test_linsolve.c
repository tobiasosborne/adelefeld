/* tests/test_linsolve.c: adf_linsol, adf_linsolve_mod and adf_linsol_verify (slice 1 of S.1, lane
   s1-slice1; docs/api-s.md section 3; docs/proofs/solvers.md Definition 2.1 to Proposition 2.8).

   The oracles, in order of independence:
   1. The enumeration of (Z/N)^c, written in this file with machine integers: the set of solutions,
      the kernel, the span of the returned G, and the Howell form of the kernel found by search in
      the kernel itself from conditions (E1) to (E3) (howell_brute, the method of howell_brute in
      proto/solvers_checks.py:857, written again here). All systems modulo 4 with r, c <= 2 and
      random systems for N in 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18 with r, c <= 3.
   2. The rule for one equation a z = b modulo N (solvers P2.8(4), shoup-ntb:ntb-v2.txt:1221 to
      1225): solvable exactly when gcd(a, N) divides b; G is the row (N / gcd(a, N)), or empty.
   3. tests/ref/vectors/s1-slice1/linsolve.jsonl, written by lanes/s1-slice1/gen_vectors.py from
      linsolve_mod of proto/solvers_checks.py (every line is run): G, E, V, x0, y entry by entry.
   4. For the checker: changed certificates, judged by the enumeration of 1.
   The one call of FLINT's Howell form is the wrapper test at the end, labelled as such. */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <flint/fmpz.h>
#include <flint/fmpz_mat.h>

#include <adelefeld/linsolve.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* ---- layout pins (docs/api-s.md section 1: fixed by this lane) ---- */

_Static_assert(sizeof(fmpz_mat_struct) == 32, "fmpz_mat_struct is not 32 bytes");
_Static_assert(sizeof(adf_linsol_struct) == 192, "sizeof(adf_linsol_struct) != 192");
_Static_assert(_Alignof(adf_linsol_struct) == 8, "_Alignof(adf_linsol_struct) != 8");
_Static_assert(offsetof(adf_linsol_struct, kind) == 0, "offset of kind");
_Static_assert(offsetof(adf_linsol_struct, N) == 8, "offset of N");
_Static_assert(offsetof(adf_linsol_struct, r) == 16, "offset of r");
_Static_assert(offsetof(adf_linsol_struct, c) == 24, "offset of c");
_Static_assert(offsetof(adf_linsol_struct, G) == 32, "offset of G");
_Static_assert(offsetof(adf_linsol_struct, E) == 64, "offset of E");
_Static_assert(offsetof(adf_linsol_struct, V) == 96, "offset of V");
_Static_assert(offsetof(adf_linsol_struct, x0) == 128, "offset of x0");
_Static_assert(offsetof(adf_linsol_struct, y) == 160, "offset of y");
_Static_assert(_Generic(((adf_linsol_struct *) 0)->kind, int: 1, default: 0), "type of kind");
_Static_assert(_Generic(((adf_linsol_struct *) 0)->N, fmpz *: 1, default: 0), "type of N");
_Static_assert(_Generic(((adf_linsol_struct *) 0)->r, slong: 1, default: 0), "type of r");
_Static_assert(_Generic(((adf_linsol_struct *) 0)->c, slong: 1, default: 0), "type of c");
_Static_assert(_Generic(((adf_linsol_struct *) 0)->G, fmpz_mat_struct *: 1, default: 0), "type of G");
_Static_assert(_Generic(((adf_linsol_struct *) 0)->y, fmpz_mat_struct *: 1, default: 0), "type of y");
_Static_assert(sizeof(adf_linsol_t) == 192, "sizeof(adf_linsol_t)");
_Static_assert(ADF_LINSOL_COSET == 0 && ADF_LINSOL_EMPTY == 1, "kind values");
_Static_assert(ADF_LINSOLVE_DIM_MAX == 4096, "S-D8");

/* ---- a small random generator (xorshift64), fixed seed ---- */

static ulong rng_state = 0x9E3779B97F4A7C15UL;

static ulong
rnd(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static slong
rnd_below(slong n)
{
    return (slong) (rnd() % (ulong) n);
}

static slong
gcd_sl(slong a, slong b)
{
    if (a < 0)
        a = -a;
    if (b < 0)
        b = -b;
    while (b != 0)
    {
        slong t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static slong
mod_sl(slong a, slong N)
{
    slong t = a % N;
    return t < 0 ? t + N : t;
}

/* ---- small systems with machine integers, and the enumeration of (Z/N)^c ---- */

#define MAXD 6
#define MAXSET 6000 /* 18^3 = 5832 */

typedef struct
{
    slong N, r, c;
    slong A[MAXD][MAXD];
    slong b[MAXD];
} sys_t;

static slong
ipow(slong N, slong c)
{
    slong p = 1, i;

    for (i = 0; i < c; i++)
        p *= N;
    return p;
}

static void
idx_to_vec(slong * x, slong idx, slong N, slong c)
{
    slong j;

    for (j = c - 1; j >= 0; j--)
    {
        x[j] = idx % N;
        idx /= N;
    }
}

static slong
vec_to_idx(const slong * x, slong N, slong c)
{
    slong j, idx = 0;

    for (j = 0; j < c; j++)
        idx = idx * N + x[j];
    return idx;
}

/* the set {x : A x = b} (zero_rhs: b = 0), as a bitmap of N^c entries; returns its size */
static slong
solutions_brute(char * set, const sys_t * s, int zero_rhs)
{
    slong n = ipow(s->N, s->c), idx, i, j, count = 0;
    slong x[MAXD];

    for (idx = 0; idx < n; idx++)
    {
        int ok = 1;

        idx_to_vec(x, idx, s->N, s->c);
        for (i = 0; i < s->r && ok; i++)
        {
            slong acc = 0;

            for (j = 0; j < s->c; j++)
                acc = mod_sl(acc + s->A[i][j] * x[j], s->N);
            if (acc != (zero_rhs ? 0 : mod_sl(s->b[i], s->N)))
                ok = 0;
        }
        set[idx] = (char) ok;
        count += ok;
    }
    return count;
}

/* the span of the k rows of a matrix with c columns over Z/N (entries read as machine integers);
   returns its size */
static slong
span_brute(char * set, const fmpz_mat_t M, slong N, slong c)
{
    slong n = ipow(N, c), idx, i, t, j, count;
    slong x[MAXD], row[MAXD];
    char * next = flint_malloc((size_t) n);

    memset(set, 0, (size_t) n);
    set[0] = 1;
    for (i = 0; i < M->r; i++)
    {
        for (j = 0; j < c; j++)
            row[j] = mod_sl(fmpz_get_si(fmpz_mat_entry(M, i, j)), N);
        memset(next, 0, (size_t) n);
        for (idx = 0; idx < n; idx++)
        {
            if (!set[idx])
                continue;
            idx_to_vec(x, idx, N, c);
            for (t = 0; t < N; t++)
            {
                slong y[MAXD];

                for (j = 0; j < c; j++)
                    y[j] = mod_sl(x[j] + t * row[j], N);
                next[vec_to_idx(y, N, c)] = 1;
            }
        }
        memcpy(set, next, (size_t) n);
    }
    flint_free(next);
    count = 0;
    for (idx = 0; idx < n; idx++)
        count += set[idx];
    return count;
}

/* The Howell form of the module S (a bitmap), from Definition 2.1 by search in S: the pivot of
   column j is the least non-zero entry v[j] of the vectors of S with v[0..j-1] = 0 (solvers P2.4,
   step 2); the row of that pivot is the one vector v of S with v[0..j-1] = 0, v[j] = pivot and
   v[l] < pivot of l for every later pivot column l (E3). Writes the rows into H (at most c) and
   returns their number, or -1 if a row is not unique (which P2.4 excludes). */
static slong
howell_brute(slong H[MAXD][MAXD], const char * S, slong N, slong c)
{
    slong n = ipow(N, c), idx, j, l, k = 0;
    slong piv[MAXD], x[MAXD];

    for (j = 0; j < c; j++)
    {
        piv[j] = 0;
        for (idx = 0; idx < n; idx++)
        {
            int lead0 = 1;

            if (!S[idx])
                continue;
            idx_to_vec(x, idx, N, c);
            for (l = 0; l < j; l++)
                lead0 = lead0 && x[l] == 0;
            if (lead0 && x[j] != 0 && (piv[j] == 0 || x[j] < piv[j]))
                piv[j] = x[j];
        }
    }
    for (j = 0; j < c; j++)
    {
        slong found = 0;

        if (piv[j] == 0)
            continue;
        for (idx = 0; idx < n; idx++)
        {
            int ok = 1;

            if (!S[idx])
                continue;
            idx_to_vec(x, idx, N, c);
            for (l = 0; l < j; l++)
                ok = ok && x[l] == 0;
            ok = ok && x[j] == piv[j];
            for (l = j + 1; l < c; l++)
                ok = ok && (piv[l] == 0 || x[l] < piv[l]);
            if (ok)
            {
                found++;
                for (l = 0; l < c; l++)
                    H[k][l] = x[l];
            }
        }
        if (found != 1)
            return -1;
        k++;
    }
    return k;
}

/* 1 if the module K (a bitmap of size |K|) is free, K = (Z/N)^t: |K| = N^t and, for every divisor
   d of N, exactly d^t elements x of K have d x = 0. (For K = sum of Z/m_i with m_i | N the count
   at d is prod gcd(d, m_i); at d = p it gives t of the m_i divisible by p, at d = p^a (the exact
   power of p in N) it gives that those t have p-part p^a.) */
static int
module_is_free(const char * K, slong size, slong N, slong c)
{
    slong t = 0, p = 1, d, idx, j, n = ipow(N, c);
    slong x[MAXD];

    while (p < size)
    {
        p *= N;
        t++;
    }
    if (p != size)
        return 0;
    for (d = 1; d <= N; d++)
    {
        slong cnt = 0;

        if (N % d != 0)
            continue;
        for (idx = 0; idx < n; idx++)
        {
            int z = 1;

            if (!K[idx])
                continue;
            idx_to_vec(x, idx, N, c);
            for (j = 0; j < c; j++)
                z = z && (d * x[j]) % N == 0;
            cnt += z;
        }
        if (cnt != ipow(d, t))
            return 0;
    }
    return 1;
}

static void
sys_to_fmpz(fmpz_mat_t A, fmpz_mat_t b, fmpz_t N, const sys_t * s)
{
    slong i, j;

    fmpz_mat_init(A, s->r, s->c);
    fmpz_mat_init(b, s->r, 1);
    fmpz_init_set_si(N, s->N);
    for (i = 0; i < s->r; i++)
    {
        for (j = 0; j < s->c; j++)
            fmpz_set_si(fmpz_mat_entry(A, i, j), s->A[i][j]);
        fmpz_set_si(fmpz_mat_entry(b, i, 0), s->b[i]);
    }
}

static void
rand_sys(sys_t * s, slong N, slong r, slong c)
{
    slong i, j, d = 1;

    s->N = N;
    s->r = r;
    s->c = c;
    if (rnd_below(10) < 4)
    {
        do
            d = 1 + rnd_below(N);
        while (N % d != 0);
    }
    for (i = 0; i < r; i++)
        for (j = 0; j < c; j++)
            s->A[i][j] = mod_sl(d * rnd_below(N), N);
    for (i = 0; i < r; i++)
        s->b[i] = rnd_below(N);
    if (c > 0 && rnd_below(2) == 0)
    {
        slong x[MAXD];

        for (j = 0; j < c; j++)
            x[j] = rnd_below(N);
        for (i = 0; i < r; i++)
        {
            slong acc = 0;

            for (j = 0; j < c; j++)
                acc += s->A[i][j] * x[j];
            s->b[i] = mod_sl(acc, N);
        }
    }
    for (i = 0; i < r; i++)                  /* entries outside [0, N) */
    {
        s->b[i] += N * (rnd_below(5) - 2);
        for (j = 0; j < c; j++)
            s->A[i][j] += N * (rnd_below(5) - 2);
    }
}

/* ---- matrices ---- */

static int
mat_same(const fmpz_mat_t a, const fmpz_mat_t b)
{
    slong i, j;

    if (a->r != b->r || a->c != b->c)
        return 0;
    for (i = 0; i < a->r; i++)
        for (j = 0; j < a->c; j++)
            if (!fmpz_equal(fmpz_mat_entry(a, i, j), fmpz_mat_entry(b, i, j)))
                return 0;
    return 1;
}

static void
mat_copy(fmpz_mat_t d, const fmpz_mat_t s)
{
    slong i, j;

    fmpz_mat_clear(d);
    fmpz_mat_init(d, s->r, s->c);
    for (i = 0; i < s->r; i++)
        for (j = 0; j < s->c; j++)
            fmpz_set(fmpz_mat_entry(d, i, j), fmpz_mat_entry(s, i, j));
}

/* remove row i */
static void
mat_del_row(fmpz_mat_t M, slong i)
{
    fmpz_mat_t T;
    slong a, j, k = 0;

    fmpz_mat_init(T, M->r - 1, M->c);
    for (a = 0; a < M->r; a++)
    {
        if (a == i)
            continue;
        for (j = 0; j < M->c; j++)
            fmpz_set(fmpz_mat_entry(T, k, j), fmpz_mat_entry(M, a, j));
        k++;
    }
    fmpz_mat_swap(M, T);
    fmpz_mat_clear(T);
}

/* insert a copy of row i before row i */
static void
mat_dup_row(fmpz_mat_t M, slong i)
{
    fmpz_mat_t T;
    slong a, j, k = 0;

    fmpz_mat_init(T, M->r + 1, M->c);
    for (a = 0; a < M->r; a++)
    {
        for (j = 0; j < M->c; j++)
            fmpz_set(fmpz_mat_entry(T, k, j), fmpz_mat_entry(M, a, j));
        k++;
        if (a == i)
        {
            for (j = 0; j < M->c; j++)
                fmpz_set(fmpz_mat_entry(T, k, j), fmpz_mat_entry(M, a, j));
            k++;
        }
    }
    fmpz_mat_swap(M, T);
    fmpz_mat_clear(T);
}

static void
linsol_copy(adf_linsol_t d, const adf_linsol_t s)
{
    d->kind = s->kind;
    fmpz_set(d->N, s->N);
    d->r = s->r;
    d->c = s->c;
    mat_copy(d->G, s->G);
    mat_copy(d->E, s->E);
    mat_copy(d->V, s->V);
    mat_copy(d->x0, s->x0);
    mat_copy(d->y, s->y);
}

static int
linsol_same(const adf_linsol_t a, const adf_linsol_t b)
{
    return a->kind == b->kind && fmpz_equal(a->N, b->N) && a->r == b->r && a->c == b->c && mat_same(a->G, b->G)
           && mat_same(a->E, b->E) && mat_same(a->V, b->V) && mat_same(a->x0, b->x0) && mat_same(a->y, b->y);
}

/* ---- the check of one small system against the enumeration ---- */

typedef struct
{
    unsigned long systems, ok, none, ker_one, ker_all, ker_other, not_free, bad;
    unsigned long ker_hist[6000];
} tally_t;

/* The result sol of the system s against the enumeration. Returns 1 if every claim holds. */
static int
judge(const adf_linsol_t sol, int st, const sys_t * s, tally_t * t)
{
    static char sols[MAXSET], ker[MAXSET], span[MAXSET], coset[MAXSET];
    slong H[MAXD][MAXD];
    slong n = ipow(s->N, s->c), nsol, nker, nspan, k, i, j, idx;
    fmpz_mat_t A, b;
    fmpz_t N;
    int ok = 1;

    sys_to_fmpz(A, b, N, s);
    nsol = solutions_brute(sols, s, 0);
    nker = solutions_brute(ker, s, 1);
    ok = ok && adf_linsol_verify(sol, A, b, N) == 1;
    ok = ok && adf_linsol_is_canonical(sol);
    ok = ok && fmpz_equal(sol->N, N) && sol->r == s->r && sol->c == s->c;
    ok = ok && sol->G->c == s->c && sol->G->r <= s->c && adf_linsol_kernel_rows(sol) == sol->G->r;
    if (ok)
    {
        nspan = span_brute(span, sol->G, s->N, s->c);
        ok = ok && nspan == nker && memcmp(span, ker, (size_t) n) == 0;       /* S(G) = K */
        k = howell_brute(H, ker, s->N, s->c);
        ok = ok && k == sol->G->r;                                            /* G = Howell form of K */
        for (i = 0; ok && i < k; i++)
            for (j = 0; j < s->c; j++)
                ok = ok && fmpz_equal_si(fmpz_mat_entry(sol->G, i, j), H[i][j]);
    }
    if (ok && st == ADF_OK)
    {
        slong x[MAXD], y[MAXD];

        ok = ok && adf_linsol_kind(sol) == ADF_LINSOL_COSET && nsol > 0;
        ok = ok && sol->x0->r == s->c && sol->x0->c == 1 && sol->y->r == 0 && sol->y->c == 0;
        memset(coset, 0, (size_t) n);
        for (idx = 0; ok && idx < n; idx++)
        {
            if (!span[idx])
                continue;
            idx_to_vec(x, idx, s->N, s->c);
            for (j = 0; j < s->c; j++)
                y[j] = mod_sl(x[j] + fmpz_get_si(fmpz_mat_entry(sol->x0, j, 0)), s->N);
            coset[vec_to_idx(y, s->N, s->c)] = 1;
        }
        ok = ok && memcmp(coset, sols, (size_t) n) == 0;                      /* x0 + S(G) = solutions */
        t->ok++;
    }
    else if (ok && st == ADF_NO_SOLUTION)
    {
        ok = ok && adf_linsol_kind(sol) == ADF_LINSOL_EMPTY && nsol == 0;
        ok = ok && sol->y->r == s->r && sol->y->c == 1 && sol->x0->r == 0 && sol->x0->c == 0;
        t->none++;
    }
    else
        ok = 0;
    t->systems++;
    if (nker < 6000)
        t->ker_hist[nker]++;
    if (nker == 1)
        t->ker_one++;
    else if (nker == n)
        t->ker_all++;
    else
        t->ker_other++;
    if (!module_is_free(ker, nker, s->N, s->c))
        t->not_free++;
    if (!ok)
        t->bad++;
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    return ok;
}

static int
solve_and_judge(const sys_t * s, tally_t * t)
{
    adf_linsol_t sol;
    fmpz_mat_t A, b;
    fmpz_t N;
    int st, ok;

    sys_to_fmpz(A, b, N, s);
    adf_linsol_init(sol);
    st = adf_linsolve_mod(sol, A, b, N);
    ok = judge(sol, st, s, t);
    adf_linsol_clear(sol);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    return ok;
}

static void
print_tally(const char * what, const tally_t * t)
{
    slong s;

    printf("%s: %lu systems, OK %lu, NO_SOLUTION %lu; kernel of one element %lu, the whole space %lu, other %lu; "
           "not free %lu; failures %lu\n", what, t->systems, t->ok, t->none, t->ker_one, t->ker_all, t->ker_other,
           t->not_free, t->bad);
    printf("   kernel sizes (size:count):");
    for (s = 1; s < 6000; s++)
        if (t->ker_hist[s])
            printf(" %ld:%lu", (long) s, t->ker_hist[s]);
    printf("\n");
}

/* ---- life cycle ---- */

ADF_TEST(init_value_and_accessors)
{
    adf_linsol_t sol;
    fmpz_mat_t A, b, M;
    fmpz_t N;

    adf_linsol_init(sol);
    ADF_CHECK(sol->kind == ADF_LINSOL_COSET && fmpz_is_one(sol->N) && sol->r == 0 && sol->c == 0);
    ADF_CHECK(sol->G->r == 0 && sol->G->c == 0 && sol->E->r == 0 && sol->E->c == 0 && sol->V->r == 0
              && sol->V->c == 0 && sol->x0->r == 0 && sol->x0->c == 1 && sol->y->r == 0 && sol->y->c == 0);
    ADF_CHECK(adf_linsol_is_canonical(sol));
    ADF_CHECK(adf_linsol_kind(sol) == ADF_LINSOL_COSET && adf_linsol_kernel_rows(sol) == 0);
    /* the init value is the verified solution of the empty system modulo 1 */
    fmpz_mat_init(A, 0, 0);
    fmpz_mat_init(b, 0, 1);
    fmpz_init_set_ui(N, 1);
    ADF_CHECK(adf_linsol_verify(sol, A, b, N) == 1);
    fmpz_set_ui(N, 2);
    ADF_CHECK(adf_linsol_verify(sol, A, b, N) == 0);          /* another modulus */
    fmpz_mat_init(M, 3, 3);
    fmpz_one(fmpz_mat_entry(M, 0, 0));
    ADF_CHECK(adf_linsol_get_particular(M, sol) == 1 && M->r == 0 && M->c == 1);
    fmpz_mat_clear(M);
    fmpz_mat_init(M, 2, 2);
    fmpz_one(fmpz_mat_entry(M, 1, 1));
    ADF_CHECK(adf_linsol_get_dual(M, sol) == 0 && M->r == 2 && M->c == 2 && fmpz_is_one(fmpz_mat_entry(M, 1, 1)));
    adf_linsol_get_kernel(M, sol);
    ADF_CHECK(M->r == 0 && M->c == 0);
    ADF_CHECK(adf_sizeof_linsol() == 192 && adf_alignof_linsol() == 8);
    fmpz_mat_clear(M);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    adf_linsol_clear(sol);
}

ADF_TEST(accessors_copy_and_aliasing)
{
    adf_linsol_t sol;
    fmpz_mat_t A, b, M;
    fmpz_t N;
    slong i;

    /* 2 x + 4 y = 6 modulo 8 (A = [2 4], b = [6]); the kernel is x + 2 y = 0 modulo 4, and check_s1_edge
       (proto/solvers_checks.py:1334) gives G = [[2, 1], [0, 2]] */
    fmpz_mat_init(A, 1, 2);
    fmpz_mat_init(b, 1, 1);
    fmpz_init_set_ui(N, 8);
    fmpz_set_ui(fmpz_mat_entry(A, 0, 0), 2);
    fmpz_set_ui(fmpz_mat_entry(A, 0, 1), 4);
    fmpz_set_ui(fmpz_mat_entry(b, 0, 0), 6);
    adf_linsol_init(sol);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_OK);
    ADF_CHECK(adf_linsol_kernel_rows(sol) == 2);
    fmpz_mat_init(M, 5, 1);
    adf_linsol_get_kernel(M, sol);
    ADF_CHECK(M->r == 2 && M->c == 2 && fmpz_equal_si(fmpz_mat_entry(M, 0, 0), 2)
              && fmpz_equal_si(fmpz_mat_entry(M, 0, 1), 1) && fmpz_is_zero(fmpz_mat_entry(M, 1, 0))
              && fmpz_equal_si(fmpz_mat_entry(M, 1, 1), 2));
    ADF_CHECK(adf_linsol_get_particular(M, sol) == 1 && M->r == 2 && M->c == 1);
    ADF_CHECK(adf_linsol_get_dual(M, sol) == 0 && M->r == 2 && M->c == 1);
    /* the output is the member of sol itself */
    adf_linsol_get_kernel(sol->G, sol);
    ADF_CHECK(sol->G->r == 2 && fmpz_equal_si(fmpz_mat_entry(sol->G, 1, 1), 2));
    ADF_CHECK(adf_linsol_get_particular(sol->x0, sol) == 1 && sol->x0->r == 2);
    ADF_CHECK(adf_linsol_verify(sol, A, b, N) == 1);
    /* no solution: 2 x + 4 y = 1 modulo 8 */
    fmpz_set_ui(fmpz_mat_entry(b, 0, 0), 1);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_NO_SOLUTION);
    ADF_CHECK(adf_linsol_kind(sol) == ADF_LINSOL_EMPTY);
    ADF_CHECK(adf_linsol_get_particular(M, sol) == 0 && M->r == 2 && M->c == 1);
    ADF_CHECK(adf_linsol_get_dual(M, sol) == 1 && M->r == 1 && M->c == 1);
    /* y^T A = 0 and y^T b != 0 modulo 8: y is 4 (Howell form of [A | I_1] = [[2, 4, 1], [0, 0, 4]]) */
    ADF_CHECK(M->r == 1 && M->c == 1 && fmpz_equal_si(fmpz_mat_entry(M, 0, 0), 4));
    ADF_CHECK(adf_linsol_get_dual(sol->y, sol) == 1 && sol->y->r == 1);
    ADF_CHECK(adf_linsol_verify(sol, A, b, N) == 1);
    /* A and b the same object (c = 1): 3 x = 3 modulo 9 */
    fmpz_mat_clear(M);
    fmpz_mat_init(M, 1, 1);
    fmpz_set_ui(fmpz_mat_entry(M, 0, 0), 3);
    fmpz_set_ui(N, 9);
    ADF_CHECK(adf_linsolve_mod(sol, M, M, N) == ADF_OK && adf_linsol_verify(sol, M, M, N) == 1);
    ADF_CHECK(sol->G->r == 1 && fmpz_equal_si(fmpz_mat_entry(sol->G, 0, 0), 3));
    ADF_CHECK(sol->x0->r == 1 && fmpz_equal_si(fmpz_mat_entry(sol->x0, 0, 0), 1));
    /* sol written again over a result of another shape */
    for (i = 0; i < 3; i++)
        ADF_CHECK(adf_linsolve_mod(sol, M, M, N) == ADF_OK && adf_linsol_verify(sol, M, M, N) == 1);
    fmpz_mat_clear(M);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    adf_linsol_clear(sol);
}

/* ---- 1. the enumeration ---- */

ADF_TEST(every_system_modulo_4_with_r_c_at_most_2)
{
    static tally_t t;
    sys_t s;
    slong r, c, na, nb, a, bb, i, j;

    memset(&t, 0, sizeof(t));
    for (r = 0; r <= 2; r++)
        for (c = 0; c <= 2; c++)
        {
            na = ipow(4, r * c);
            nb = ipow(4, r);
            for (a = 0; a < na; a++)
                for (bb = 0; bb < nb; bb++)
                {
                    slong u = a, v = bb;

                    s.N = 4;
                    s.r = r;
                    s.c = c;
                    for (i = 0; i < r; i++)
                        for (j = 0; j < c; j++)
                        {
                            s.A[i][j] = u % 4;
                            u /= 4;
                        }
                    for (i = 0; i < r; i++)
                    {
                        s.b[i] = v % 4;
                        v /= 4;
                    }
                    ADF_CHECK_MSG(solve_and_judge(&s, &t), "N = 4, r = %ld, c = %ld, A index %ld, b index %ld",
                                  (long) r, (long) c, (long) a, (long) bb);
                }
        }
    print_tally("all systems modulo 4, r, c <= 2", &t);
    ADF_CHECK(t.bad == 0 && t.ok > 0 && t.none > 0 && t.ker_one > 0 && t.not_free > 0);
    ADF_CHECK(t.systems == 1 + 1 + 1 + 4 + 16 + 64 + 16 + 256 + 4096);
}

ADF_TEST(random_systems_small_moduli_r_c_at_most_3)
{
    static tally_t t;
    static const slong moduli[] = { 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18 };
    sys_t s;
    size_t m;
    slong r, c, k;

    memset(&t, 0, sizeof(t));
    for (m = 0; m < sizeof(moduli) / sizeof(moduli[0]); m++)
        for (r = 0; r <= 3; r++)
            for (c = 0; c <= 3; c++)
                for (k = 0; k < (moduli[m] == 1 ? 3 : 40); k++)
                {
                    rand_sys(&s, moduli[m], r, c);
                    ADF_CHECK_MSG(solve_and_judge(&s, &t), "N = %ld, r = %ld, c = %ld, case %ld", (long) moduli[m],
                                  (long) r, (long) c, (long) k);
                }
    print_tally("random systems, N in {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18}, r, c <= 3", &t);
    ADF_CHECK(t.bad == 0 && t.ok > 0 && t.none > 0 && t.ker_one > 0 && t.not_free > 0);
}

/* ---- 2. one equation (solvers P2.8(4)) ---- */

ADF_TEST(one_equation_every_a_b_for_N_below_40)
{
    adf_linsol_t sol;
    fmpz_mat_t A, b;
    fmpz_t N;
    slong n, a, bb;
    unsigned long cases = 0, bad = 0;

    adf_linsol_init(sol);
    fmpz_mat_init(A, 1, 1);
    fmpz_mat_init(b, 1, 1);
    fmpz_init(N);
    for (n = 1; n < 40; n++)
        for (a = 0; a < n; a++)
            for (bb = 0; bb < n; bb++)
            {
                slong g = gcd_sl(a, n);
                int want = bb % g == 0 ? ADF_OK : ADF_NO_SOLUTION, st, ok;

                fmpz_set_si(N, n);
                fmpz_set_si(fmpz_mat_entry(A, 0, 0), a - 3 * n);
                fmpz_set_si(fmpz_mat_entry(b, 0, 0), bb + 5 * n);
                st = adf_linsolve_mod(sol, A, b, N);
                cases++;
                ok = st == want && adf_linsol_verify(sol, A, b, N) == 1;
                if (n / g < n)
                    ok = ok && sol->G->r == 1 && fmpz_equal_si(fmpz_mat_entry(sol->G, 0, 0), n / g);
                else
                    ok = ok && sol->G->r == 0;
                if (!ok)
                    bad++;
            }
    printf("one equation a z = b modulo N, N < 40: %lu cases, %lu failures\n", cases, bad);
    ADF_CHECK(bad == 0 && cases == 20540);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    adf_linsol_clear(sol);
}

/* ---- canonical form ---- */

/* the system (A, b) changed: equations permuted, multiplied by units, two redundant equations added
   (a random combination of the equations, and a zero equation) */
static void
changed_system(fmpz_mat_t A2, fmpz_mat_t b2, const fmpz_mat_t A, const fmpz_mat_t b, const fmpz_t N)
{
    slong r = A->r, c = A->c, i, j, perm[MAXD + 2];
    fmpz_t u, g, q;

    fmpz_init(u);
    fmpz_init(g);
    fmpz_init(q);
    fmpz_mat_init(A2, r + 2, c);
    fmpz_mat_init(b2, r + 2, 1);
    for (i = 0; i < r + 2; i++)
        perm[i] = i;
    for (i = r + 1; i > 0; i--)
    {
        slong k = rnd_below(i + 1), t = perm[i];

        perm[i] = perm[k];
        perm[k] = t;
    }
    for (i = 0; i < r; i++)
    {
        do
        {
            fmpz_set_ui(u, rnd());
            fmpz_mod(u, u, N);
            fmpz_gcd(g, u, N);
        }
        while (!fmpz_is_one(g));
        for (j = 0; j < c; j++)
            fmpz_mul(fmpz_mat_entry(A2, perm[i], j), fmpz_mat_entry(A, i, j), u);
        fmpz_mul(fmpz_mat_entry(b2, perm[i], 0), fmpz_mat_entry(b, i, 0), u);
    }
    for (i = 0; i < r; i++)                         /* row perm[r] = sum q_i (A_i | b_i); row perm[r + 1] = 0 */
    {
        fmpz_set_ui(q, rnd() % 1000);
        for (j = 0; j < c; j++)
            fmpz_addmul(fmpz_mat_entry(A2, perm[r], j), fmpz_mat_entry(A, i, j), q);
        fmpz_addmul(fmpz_mat_entry(b2, perm[r], 0), fmpz_mat_entry(b, i, 0), q);
    }
    fmpz_clear(u);
    fmpz_clear(g);
    fmpz_clear(q);
}

ADF_TEST(canonical_under_permutation_units_and_redundant_equations)
{
    static const char * moduli[] = { "4", "6", "8", "9", "12", "36", "360", "1048576", "18446744073709551616",
                                     "1000000000000000000000000000000", "12157665459056928801" };
    adf_linsol_t s1, s2;
    fmpz_mat_t A, b, A2, b2;
    fmpz_t N;
    size_t m;
    slong k, i, j, r, c;
    unsigned long n = 0, bad = 0, none = 0;

    adf_linsol_init(s1);
    adf_linsol_init(s2);
    fmpz_init(N);
    for (m = 0; m < sizeof(moduli) / sizeof(moduli[0]); m++)
    {
        fmpz_set_str(N, moduli[m], 10);
        for (k = 0; k < 40; k++)
        {
            int st1, st2;
            fmpz_t d;

            r = 1 + rnd_below(3);
            c = 1 + rnd_below(4);
            fmpz_init(d);
            fmpz_one(d);
            if (rnd_below(2) == 0)                          /* zero divisors */
                fmpz_set_ui(d, (ulong) (2 + rnd_below(5)));
            fmpz_mat_init(A, r, c);
            fmpz_mat_init(b, r, 1);
            for (i = 0; i < r; i++)
            {
                for (j = 0; j < c; j++)
                {
                    fmpz_set_ui(fmpz_mat_entry(A, i, j), rnd());
                    fmpz_mul(fmpz_mat_entry(A, i, j), fmpz_mat_entry(A, i, j), d);
                    fmpz_mod(fmpz_mat_entry(A, i, j), fmpz_mat_entry(A, i, j), N);
                }
                fmpz_set_ui(fmpz_mat_entry(b, i, 0), rnd_below(3) == 0 ? rnd() : 0);
            }
            st1 = adf_linsolve_mod(s1, A, b, N);
            changed_system(A2, b2, A, b, N);
            st2 = adf_linsolve_mod(s2, A2, b2, N);
            n++;
            none += st1 == ADF_NO_SOLUTION;
            if (st1 != st2 || !mat_same(s1->G, s2->G) || s1->kind != s2->kind
                || adf_linsol_verify(s2, A2, b2, N) != 1)
                bad++;
            fmpz_mat_clear(A);
            fmpz_mat_clear(b);
            fmpz_mat_clear(A2);
            fmpz_mat_clear(b2);
            fmpz_clear(d);
        }
    }
    printf("canonical form: %lu systems changed by permutation, units and redundant equations, %lu without "
           "solution; %lu with a different G or status\n", n, none, bad);
    ADF_CHECK(bad == 0 && n > 0 && none > 0);
    fmpz_clear(N);
    adf_linsol_clear(s1);
    adf_linsol_clear(s2);
}

/* ---- 3. the vectors ---- */

static int
read_fmpz(fmpz_t v, const jsonl_value * j)
{
    jsonl_error_t err;
    const char * text = jsonl_int_text(j, &err);

    if (text == NULL)
    {
        printf("%s\n", jsonl_error_message(&err));
        return 0;
    }
    return fmpz_set_str(v, text, 10) == 0;
}

/* M = the array of rows arr, with cols columns (arr may have 0 rows) */
static int
read_rows(fmpz_mat_t M, const jsonl_value * arr, slong cols)
{
    jsonl_error_t err;
    slong rows = (slong) jsonl_size(arr), i, j;
    int ok = 1;

    fmpz_mat_clear(M);
    fmpz_mat_init(M, rows, cols);
    for (i = 0; i < rows; i++)
    {
        const jsonl_value * row = jsonl_at(arr, (size_t) i, &err);

        ok = ok && row != NULL && (slong) jsonl_size(row) == cols;
        for (j = 0; ok && j < cols; j++)
            ok = ok && read_fmpz(fmpz_mat_entry(M, i, j), jsonl_at(row, (size_t) j, &err));
    }
    return ok;
}

/* M = the array arr as a column of n entries */
static int
read_col(fmpz_mat_t M, const jsonl_value * arr, slong n)
{
    jsonl_error_t err;
    slong i;
    int ok = (slong) jsonl_size(arr) == n;

    fmpz_mat_clear(M);
    fmpz_mat_init(M, n, 1);
    for (i = 0; ok && i < n; i++)
        ok = ok && read_fmpz(fmpz_mat_entry(M, i, 0), jsonl_at(arr, (size_t) i, &err));
    return ok;
}

static const jsonl_value *
field(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    const jsonl_value * v = NULL;

    if (!jsonl_field(rec, key, &v, &err))
        printf("%s\n", jsonl_error_message(&err));
    return v;
}

ADF_TEST(vectors_s1_slice1_every_line)
{
    jsonl_error_t err;
    jsonl_file * file;
    size_t i;
    adf_linsol_t sol;
    fmpz_mat_t A, b, G, E, V, x0, y;
    fmpz_t N, t;
    unsigned long n_ok = 0, n_none = 0, bad = 0, n_big = 0;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/s1-slice1/linsolve.jsonl", &file, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (file == NULL)
        return;
    adf_linsol_init(sol);
    fmpz_mat_init(A, 0, 0);
    fmpz_mat_init(b, 0, 0);
    fmpz_mat_init(G, 0, 0);
    fmpz_mat_init(E, 0, 0);
    fmpz_mat_init(V, 0, 0);
    fmpz_mat_init(x0, 0, 0);
    fmpz_mat_init(y, 0, 0);
    fmpz_init(N);
    fmpz_init(t);
    for (i = 0; i < jsonl_count(file); i++)
    {
        const jsonl_value * rec = jsonl_record(file, i);
        const char * st_name;
        size_t len;
        slong r, c;
        int st, want, ok = 1;

        ok = ok && read_fmpz(N, field(rec, "N"));
        ok = ok && read_fmpz(t, field(rec, "r"));
        r = fmpz_get_si(t);
        ok = ok && read_fmpz(t, field(rec, "c"));
        c = fmpz_get_si(t);
        ok = ok && read_rows(A, field(rec, "A"), c) && A->r == r;
        ok = ok && read_col(b, field(rec, "b"), r);
        st_name = jsonl_string(field(rec, "status"), &len, &err);
        want = strcmp(st_name, "OK") == 0 ? ADF_OK : ADF_NO_SOLUTION;
        ok = ok && read_rows(G, field(rec, "G"), c) && read_rows(E, field(rec, "E"), r)
             && read_rows(V, field(rec, "V"), c);
        if (want == ADF_OK)
        {
            ok = ok && read_col(x0, field(rec, "x0"), c);
            fmpz_mat_clear(y);
            fmpz_mat_init(y, 0, 0);
        }
        else
        {
            ok = ok && read_col(y, field(rec, "y"), r);
            fmpz_mat_clear(x0);
            fmpz_mat_init(x0, 0, 0);
        }
        ADF_CHECK_MSG(ok, "line %lu: could not be read", (unsigned long) (i + 1));
        if (!ok)
            continue;
        st = adf_linsolve_mod(sol, A, b, N);
        ok = st == want && fmpz_equal(sol->N, N) && sol->r == r && sol->c == c;
        ok = ok && mat_same(sol->G, G) && mat_same(sol->E, E) && mat_same(sol->V, V) && mat_same(sol->x0, x0)
             && mat_same(sol->y, y);
        ok = ok && adf_linsol_verify(sol, A, b, N) == 1 && adf_linsol_is_canonical(sol);
        ADF_CHECK_MSG(ok, "line %lu: status %d, want %d, or a matrix differs", (unsigned long) (i + 1), st, want);
        if (!ok)
            bad++;
        if (want == ADF_OK)
            n_ok++;
        else
            n_none++;
        if (fmpz_bits(N) > 64)
            n_big++;
    }
    printf("vectors: %lu lines, OK %lu, NO_SOLUTION %lu, N above 64 bits %lu; %lu differ\n",
           (unsigned long) jsonl_count(file), n_ok, n_none, n_big, bad);
    ADF_CHECK(jsonl_count(file) > 400 && n_ok > 0 && n_none > 0 && n_big > 0);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_mat_clear(G);
    fmpz_mat_clear(E);
    fmpz_mat_clear(V);
    fmpz_mat_clear(x0);
    fmpz_mat_clear(y);
    fmpz_clear(N);
    fmpz_clear(t);
    adf_linsol_clear(sol);
    jsonl_close(file);
}

/* ---- 4. the checker refuses false certificates ---- */

#define NKINDS 13
static const char * kind_names[NKINDS] = {
    "generator removed", "generator changed", "row of E removed (with V)", "row of E changed",
    "row of V removed (E kept)", "row of V changed", "x0 or y changed", "kind flipped",
    "generator multiplied by a unit", "certificate of another matrix", "certificate of A with one row replaced",
    "row of E repeated, generator removed", "generator plus a multiple of a later one"
};

/* Is the claim of the (changed) result m true for s, by enumeration: S(G) is the kernel, G is its
   Howell form, and x0 is a solution (COSET) or there is none (EMPTY)? */
static int
claim_true(const adf_linsol_t m, const sys_t * s)
{
    static char sols[MAXSET], ker[MAXSET], span[MAXSET];
    slong H[MAXD][MAXD], x[MAXD];
    slong n = ipow(s->N, s->c), nsol, k, i, j;

    if (m->G->c != s->c || m->G->r > s->c)
        return 0;
    for (i = 0; i < m->G->r; i++)
        for (j = 0; j < s->c; j++)
            if (fmpz_sgn(fmpz_mat_entry(m->G, i, j)) < 0 || fmpz_cmp_si(fmpz_mat_entry(m->G, i, j), s->N) >= 0)
                return 0;
    nsol = solutions_brute(sols, s, 0);
    solutions_brute(ker, s, 1);
    span_brute(span, m->G, s->N, s->c);
    if (memcmp(span, ker, (size_t) n) != 0)
        return 0;
    k = howell_brute(H, ker, s->N, s->c);
    if (k != m->G->r)
        return 0;
    for (i = 0; i < k; i++)
        for (j = 0; j < s->c; j++)
            if (!fmpz_equal_si(fmpz_mat_entry(m->G, i, j), H[i][j]))
                return 0;
    if (m->kind == ADF_LINSOL_COSET)
    {
        if (m->x0->r != s->c || m->x0->c != 1)
            return 0;
        for (j = 0; j < s->c; j++)
        {
            if (fmpz_sgn(fmpz_mat_entry(m->x0, j, 0)) < 0 || fmpz_cmp_si(fmpz_mat_entry(m->x0, j, 0), s->N) >= 0)
                return 0;
            x[j] = fmpz_get_si(fmpz_mat_entry(m->x0, j, 0));
        }
        return sols[vec_to_idx(x, s->N, s->c)];
    }
    if (m->kind == ADF_LINSOL_EMPTY)
        return nsol == 0;
    return 0;
}

static void
rand_entry(fmpz_t e, slong N)
{
    fmpz_set_si(e, rnd_below(N));
}

/* Apply the change `kind` to m (a copy of the result for s). Returns 0 if it does not apply. */
static int
change(adf_linsol_t m, int kind, const sys_t * s)
{
    slong N = s->N, i, j;

    switch (kind)
    {
    case 0:
        if (m->G->r == 0)
            return 0;
        mat_del_row(m->G, rnd_below(m->G->r));
        return 1;
    case 1:
        if (m->G->r == 0)
            return 0;
        rand_entry(fmpz_mat_entry(m->G, rnd_below(m->G->r), rnd_below(s->c)), N);
        return 1;
    case 2:
        if (m->E->r == 0)
            return 0;
        i = rnd_below(m->E->r);
        mat_del_row(m->E, i);
        mat_del_row(m->V, i);
        return 1;
    case 4:
        if (m->V->r == 0)
            return 0;
        mat_del_row(m->V, rnd_below(m->V->r));
        return 1;
    case 3:
        if (m->E->r == 0)
            return 0;
        rand_entry(fmpz_mat_entry(m->E, rnd_below(m->E->r), rnd_below(s->r)), N);
        return 1;
    case 5:
        if (m->V->r == 0)
            return 0;
        rand_entry(fmpz_mat_entry(m->V, rnd_below(m->V->r), rnd_below(s->c)), N);
        return 1;
    case 6:
        if (m->kind == ADF_LINSOL_COSET)
        {
            if (s->c == 0)
                return 0;
            rand_entry(fmpz_mat_entry(m->x0, rnd_below(s->c), 0), N);
        }
        else
            rand_entry(fmpz_mat_entry(m->y, rnd_below(s->r), 0), N);
        return 1;
    case 7:
        fmpz_mat_clear(m->x0);
        fmpz_mat_clear(m->y);
        if (m->kind == ADF_LINSOL_COSET)
        {
            m->kind = ADF_LINSOL_EMPTY;
            fmpz_mat_init(m->x0, 0, 0);
            fmpz_mat_init(m->y, s->r, 1);
            for (i = 0; i < s->r; i++)
                rand_entry(fmpz_mat_entry(m->y, i, 0), N);
        }
        else
        {
            m->kind = ADF_LINSOL_COSET;
            fmpz_mat_init(m->x0, s->c, 1);
            fmpz_mat_init(m->y, 0, 0);
            for (i = 0; i < s->c; i++)
                rand_entry(fmpz_mat_entry(m->x0, i, 0), N);
        }
        return 1;
    case 8:
    {
        slong u;

        if (m->G->r == 0 || N <= 2)
            return 0;
        do
            u = 2 + rnd_below(N - 2);
        while (gcd_sl(u, N) != 1);
        i = rnd_below(m->G->r);
        for (j = 0; j < s->c; j++)
        {
            fmpz_mul_si(fmpz_mat_entry(m->G, i, j), fmpz_mat_entry(m->G, i, j), u);
            fmpz_mod_ui(fmpz_mat_entry(m->G, i, j), fmpz_mat_entry(m->G, i, j), (ulong) N);
        }
        return 1;
    }
    case 9:
    case 10:
    {
        sys_t s2 = *s;
        adf_linsol_t o;
        fmpz_mat_t A2, b2;
        fmpz_t N2;

        if (kind == 9)
            rand_sys(&s2, N, s->r, s->c);
        else
        {
            i = rnd_below(s->r);
            for (j = 0; j < s->c; j++)
                s2.A[i][j] = rnd_below(N);
        }
        sys_to_fmpz(A2, b2, N2, &s2);
        adf_linsol_init(o);
        adf_linsolve_mod(o, A2, b2, N2);
        mat_copy(m->E, o->E);
        mat_copy(m->V, o->V);
        mat_copy(m->G, o->G);
        adf_linsol_clear(o);
        fmpz_mat_clear(A2);
        fmpz_mat_clear(b2);
        fmpz_clear(N2);
        return 1;
    }
    case 11:
        if (m->E->r == 0 || m->G->r == 0)
            return 0;
        i = rnd_below(m->E->r);
        mat_dup_row(m->E, i);
        mat_dup_row(m->V, i);
        mat_del_row(m->G, rnd_below(m->G->r));
        return 1;
    case 12:
    {
        slong l, q = 1 + rnd_below(N - 1 > 0 ? N - 1 : 1);

        if (m->G->r < 2)
            return 0;
        i = rnd_below(m->G->r - 1);
        l = i + 1 + rnd_below(m->G->r - 1 - i);
        for (j = 0; j < s->c; j++)
        {
            fmpz_addmul_ui(fmpz_mat_entry(m->G, i, j), fmpz_mat_entry(m->G, l, j), (ulong) q);
            fmpz_mod_ui(fmpz_mat_entry(m->G, i, j), fmpz_mat_entry(m->G, i, j), (ulong) N);
        }
        return 1;
    }
    default:
        return 0;
    }
}

ADF_TEST(checker_refuses_false_certificates)
{
    static const slong moduli[] = { 2, 4, 6, 8, 9, 12 };
    unsigned long tried[NKINDS] = { 0 }, refused[NKINDS] = { 0 };
    unsigned long true_acc[NKINDS] = { 0 }, false_acc[NKINDS] = { 0 };
    adf_linsol_t sol, m;
    sys_t s;
    size_t mi;
    slong k, rep;
    int kind, st;

    adf_linsol_init(sol);
    adf_linsol_init(m);
    for (mi = 0; mi < sizeof(moduli) / sizeof(moduli[0]); mi++)
        for (k = 0; k < 300; k++)
        {
            fmpz_mat_t A, b;
            fmpz_t N;
            slong r = 1 + rnd_below(3), c = 1 + rnd_below(3);

            rand_sys(&s, moduli[mi], r, c);
            if (rnd_below(3) == 0)                     /* systems without solution, often */
                s.b[0] += 1;
            sys_to_fmpz(A, b, N, &s);
            st = adf_linsolve_mod(sol, A, b, N);
            ADF_CHECK(st == ADF_OK || st == ADF_NO_SOLUTION);
            for (rep = 0; rep < 2 && (st == ADF_OK || st == ADF_NO_SOLUTION); rep++)
                for (kind = 0; kind < NKINDS; kind++)
                {
                    int acc;

                    linsol_copy(m, sol);
                    if (!change(m, kind, &s))
                        continue;
                    tried[kind]++;
                    acc = adf_linsol_verify(m, A, b, N);
                    if (!acc)
                        refused[kind]++;
                    else if (claim_true(m, &s))
                        true_acc[kind]++;
                    else
                    {
                        false_acc[kind]++;
                        ADF_CHECK_MSG(0, "change \"%s\" accepted and false: N = %ld, r = %ld, c = %ld",
                                      kind_names[kind], (long) s.N, (long) r, (long) c);
                    }
                }
            fmpz_mat_clear(A);
            fmpz_mat_clear(b);
            fmpz_clear(N);
        }
    for (kind = 0; kind < NKINDS; kind++)
    {
        printf("changed certificates, %-42s: %5lu tried, %5lu refused, %5lu accepted and true, %lu accepted and "
               "false\n", kind_names[kind], tried[kind], refused[kind], true_acc[kind], false_acc[kind]);
        ADF_CHECK_MSG(refused[kind] > 0, "no certificate refused for the change \"%s\"", kind_names[kind]);
        ADF_CHECK(false_acc[kind] == 0);
    }
    adf_linsol_clear(sol);
    adf_linsol_clear(m);
}

/* The checker never aborts and refuses results of the wrong shape (a result of another system) */
/* the changes of the test below, on a result sol of the system (A, b, N) with x0 2 by 1 and V e by 2 */
static void
wrong_shapes(const adf_linsol_t sol, const fmpz_mat_t A, const fmpz_mat_t b, const fmpz_t N)
{
    adf_linsol_t m;
    fmpz_mat_t b2;
    fmpz_t N2;

    adf_linsol_init(m);
    fmpz_init_set_ui(N2, 24);
    ADF_CHECK(adf_linsol_verify(sol, A, b, N2) == 0);
    fmpz_set_si(N2, 0);
    ADF_CHECK(adf_linsol_verify(sol, A, b, N2) == 0);
    fmpz_set_si(N2, -12);
    ADF_CHECK(adf_linsol_verify(sol, A, b, N2) == 0);
    fmpz_mat_init(b2, 2, 2);
    ADF_CHECK(adf_linsol_verify(sol, A, b2, N) == 0);
    fmpz_mat_clear(b2);
    /* r and c of the value changed, a kind outside {0, 1} */
    linsol_copy(m, sol);
    m->r = 3;
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    linsol_copy(m, sol);
    m->c = -1;
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    linsol_copy(m, sol);
    m->kind = 7;
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    linsol_copy(m, sol);
    fmpz_zero(m->N);
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    /* an entry outside [0, N) */
    linsol_copy(m, sol);
    fmpz_add_ui(fmpz_mat_entry(m->x0, 0, 0), fmpz_mat_entry(m->x0, 0, 0), 12);
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    linsol_copy(m, sol);
    fmpz_sub_ui(fmpz_mat_entry(m->V, 0, 0), fmpz_mat_entry(m->V, 0, 0), 12);
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    /* a matrix of the wrong width, x0 with a second column, a y present for kind COSET */
    linsol_copy(m, sol);
    fmpz_mat_clear(m->x0);
    fmpz_mat_init(m->x0, 2, 2);
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    linsol_copy(m, sol);
    fmpz_mat_clear(m->V);
    fmpz_mat_init(m->V, sol->V->r, 3);
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    linsol_copy(m, sol);
    fmpz_mat_clear(m->E);
    fmpz_mat_init(m->E, sol->E->r, 1);
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    linsol_copy(m, sol);
    fmpz_mat_clear(m->y);
    fmpz_mat_init(m->y, 2, 1);
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    /* a zero row in G (not echelon), and a G with too many columns */
    linsol_copy(m, sol);
    fmpz_mat_clear(m->G);
    fmpz_mat_init(m->G, 1, 2);
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    linsol_copy(m, sol);
    fmpz_mat_clear(m->G);
    fmpz_mat_init(m->G, 0, 3);
    ADF_CHECK(adf_linsol_verify(m, A, b, N) == 0 && !adf_linsol_is_canonical(m));
    fmpz_clear(N2);
    adf_linsol_clear(m);
}

/* The checker never aborts and refuses results of the wrong shape or modulus */
ADF_TEST(checker_refuses_wrong_shapes_and_moduli)
{
    adf_linsol_t sol;
    fmpz_mat_t A, b;
    fmpz_t N;
    sys_t s;

    adf_linsol_init(sol);
    s.N = 12;
    s.r = 2;
    s.c = 2;
    s.A[0][0] = 2;
    s.A[0][1] = 4;
    s.A[1][0] = 6;
    s.A[1][1] = 3;
    s.b[0] = 2;
    s.b[1] = 3;
    sys_to_fmpz(A, b, N, &s);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_OK && adf_linsol_verify(sol, A, b, N) == 1);
    ADF_CHECK(sol->x0->r == 2 && sol->x0->c == 1 && sol->V->r > 0 && sol->V->c == 2);
    if (sol->x0->r == 2 && sol->x0->c == 1 && sol->V->r > 0 && sol->V->c == 2)
        wrong_shapes(sol, A, b, N);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    adf_linsol_clear(sol);
}

/* ---- statuses ---- */

ADF_TEST(statuses_domain_and_limit_leave_sol_untouched)
{
    adf_linsol_t sol, keep;
    fmpz_mat_t A, b, bad;
    fmpz_t N;
    sys_t s;

    adf_linsol_init(sol);
    adf_linsol_init(keep);
    s.N = 12;
    s.r = 2;
    s.c = 2;
    s.A[0][0] = 2;
    s.A[0][1] = 4;
    s.A[1][0] = 6;
    s.A[1][1] = 3;
    s.b[0] = 2;
    s.b[1] = 1;
    sys_to_fmpz(A, b, N, &s);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_NO_SOLUTION && adf_linsol_verify(sol, A, b, N) == 1);
    linsol_copy(keep, sol);
    /* N < 1 */
    fmpz_zero(N);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_DOMAIN && linsol_same(sol, keep));
    fmpz_set_si(N, -12);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_DOMAIN && linsol_same(sol, keep));
    fmpz_set_si(N, 12);
    /* b of a wrong shape */
    fmpz_mat_init(bad, 2, 2);
    ADF_CHECK(adf_linsolve_mod(sol, A, bad, N) == ADF_DOMAIN && linsol_same(sol, keep));
    fmpz_mat_clear(bad);
    fmpz_mat_init(bad, 3, 1);
    ADF_CHECK(adf_linsolve_mod(sol, A, bad, N) == ADF_DOMAIN && linsol_same(sol, keep));
    fmpz_mat_clear(bad);
    fmpz_mat_init(bad, 0, 1);
    ADF_CHECK(adf_linsolve_mod(sol, A, bad, N) == ADF_DOMAIN && linsol_same(sol, keep));
    fmpz_mat_clear(bad);
    fmpz_mat_init(bad, 2, 0);
    ADF_CHECK(adf_linsolve_mod(sol, A, bad, N) == ADF_DOMAIN && linsol_same(sol, keep));
    fmpz_mat_clear(bad);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    /* r + c above the limit: 4097 by 0 and 0 by 4097 (nothing is allocated for the entries) */
    fmpz_mat_init(A, 4097, 0);
    fmpz_mat_init(b, 4097, 1);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_LIMIT && linsol_same(sol, keep));
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_mat_init(A, 0, 4097);
    fmpz_mat_init(b, 0, 1);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_LIMIT && linsol_same(sol, keep));
    /* DOMAIN is decided before LIMIT */
    fmpz_zero(N);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_DOMAIN && linsol_same(sol, keep));
    fmpz_set_si(N, 12);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    /* r + c = 4096 is inside: 4096 equations in no unknown with b = 0 modulo N */
    fmpz_mat_init(A, 4096, 0);
    fmpz_mat_init(b, 4096, 1);
    fmpz_set_si(fmpz_mat_entry(b, 4095, 0), 24);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_OK && adf_linsol_verify(sol, A, b, N) == 1);
    ADF_CHECK(sol->r == 4096 && sol->c == 0 && sol->x0->r == 0 && sol->x0->c == 1 && sol->E->r == 0);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    adf_linsol_clear(sol);
    adf_linsol_clear(keep);
}

/* ---- special cases of solvers P2.8(4), and huge entries ---- */

ADF_TEST(special_cases_and_huge_entries)
{
    adf_linsol_t sol;
    fmpz_mat_t A, b;
    fmpz_t N;
    slong i, j;

    adf_linsol_init(sol);
    fmpz_init(N);
    /* N = 1: everything is zero, G, E, V without rows, x0 = 0 */
    fmpz_one(N);
    fmpz_mat_init(A, 2, 3);
    fmpz_mat_init(b, 2, 1);
    fmpz_set_si(fmpz_mat_entry(A, 0, 1), -7);
    fmpz_set_si(fmpz_mat_entry(b, 1, 0), 5);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_OK);
    ADF_CHECK(sol->G->r == 0 && sol->G->c == 3 && sol->E->r == 0 && sol->V->r == 0 && sol->x0->r == 3
              && fmpz_is_zero(fmpz_mat_entry(sol->x0, 2, 0)) && adf_linsol_verify(sol, A, b, N));
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    /* r = 0, N = 10: G = I_c, x0 = 0 */
    fmpz_set_si(N, 10);
    fmpz_mat_init(A, 0, 3);
    fmpz_mat_init(b, 0, 1);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_OK && sol->G->r == 3 && fmpz_mat_is_one(sol->G));
    ADF_CHECK(sol->E->r == 0 && sol->E->c == 0 && sol->V->r == 0 && sol->V->c == 3
              && adf_linsol_verify(sol, A, b, N));
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    /* c = 0: y is the first unit vector e_i with b_i != 0 */
    fmpz_mat_init(A, 3, 0);
    fmpz_mat_init(b, 3, 1);
    fmpz_set_si(fmpz_mat_entry(b, 0, 0), 20);
    fmpz_set_si(fmpz_mat_entry(b, 1, 0), -3);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_NO_SOLUTION && sol->y->r == 3);
    ADF_CHECK(sol->y->r == 3 && sol->y->c == 1 && fmpz_is_zero(fmpz_mat_entry(sol->y, 0, 0))
              && fmpz_is_one(fmpz_mat_entry(sol->y, 1, 0))
              && fmpz_is_zero(fmpz_mat_entry(sol->y, 2, 0)) && adf_linsol_verify(sol, A, b, N));
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    /* the zero matrix, N = 12: G = I_c; solvable exactly when b = 0 */
    fmpz_set_si(N, 12);
    fmpz_mat_init(A, 3, 2);
    fmpz_mat_init(b, 3, 1);
    fmpz_set_si(fmpz_mat_entry(A, 2, 1), 36);
    fmpz_set_si(fmpz_mat_entry(b, 2, 0), -24);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_OK && fmpz_mat_is_one(sol->G) && sol->G->r == 2);
    fmpz_set_si(fmpz_mat_entry(b, 2, 0), -25);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_NO_SOLUTION && adf_linsol_verify(sol, A, b, N));
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    /* entries of thousands of bits and negative, N = 2^200 * 3^50 with a planted solution */
    fmpz_ui_pow_ui(N, 3, 50);
    fmpz_mul_2exp(N, N, 200);
    fmpz_mat_init(A, 3, 4);
    fmpz_mat_init(b, 3, 1);
    for (i = 0; i < 3; i++)
    {
        fmpz_t xj;

        fmpz_init(xj);
        for (j = 0; j < 4; j++)
        {
            fmpz_ui_pow_ui(fmpz_mat_entry(A, i, j), 7 + (ulong) (i + j), 900 + (ulong) (50 * j));
            if ((i + j) % 2)
                fmpz_neg(fmpz_mat_entry(A, i, j), fmpz_mat_entry(A, i, j));
            if (j == 1)
                fmpz_mul_2exp(fmpz_mat_entry(A, i, j), fmpz_mat_entry(A, i, j), 190);
            fmpz_set_si(xj, 1000003 * (j + 1) - 17 * j * j);      /* x_j, the same for every row */
            fmpz_addmul(fmpz_mat_entry(b, i, 0), fmpz_mat_entry(A, i, j), xj);
        }
        fmpz_clear(xj);
    }
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_OK && adf_linsol_verify(sol, A, b, N) == 1);
    ADF_CHECK(adf_linsol_is_canonical(sol));
    fmpz_add_ui(fmpz_mat_entry(b, 0, 0), fmpz_mat_entry(b, 0, 0), 1);
    {
        int st = adf_linsolve_mod(sol, A, b, N);

        ADF_CHECK((st == ADF_OK || st == ADF_NO_SOLUTION) && adf_linsol_verify(sol, A, b, N) == 1);
    }
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    adf_linsol_clear(sol);
}

/* ---- the one wrapper test of FLINT's Howell form (solvers P2.11(2), (3)) ---- */

/* WRAPPER TEST: fmpz_mat_howell_form_mod of FLINT 3.0.1 on [A^T | I_c], padded with r zero rows so that
   the matrix has at least as many rows as columns (flint-3.0.1:fmpz_mat.rst:1176; solvers P2.11(2)),
   against G, E, V of adf_linsolve_mod. The library never calls this routine; this test only records that
   the two agree on these inputs (solvers P2.11(3), a probe, not a proof). */
ADF_TEST(wrapper_test_fmpz_mat_howell_form_mod)
{
    static const char * moduli[] = { "2", "4", "6", "12", "36", "360", "1000000007", "18446744073709551616",
                                     "1000000000000000000000000000000" };
    adf_linsol_t sol;
    fmpz_t N;
    size_t m;
    slong k, i, j;
    unsigned long n = 0, same = 0;

    adf_linsol_init(sol);
    fmpz_init(N);
    for (m = 0; m < sizeof(moduli) / sizeof(moduli[0]); m++)
    {
        fmpz_set_str(N, moduli[m], 10);
        for (k = 0; k < 30; k++)
        {
            slong r = 1 + rnd_below(4), c = 1 + rnd_below(4), rank, e = 0, g = 0;
            fmpz_mat_t A, b, H;
            int ok = 1;

            fmpz_mat_init(A, r, c);
            fmpz_mat_init(b, r, 1);
            for (i = 0; i < r; i++)
                for (j = 0; j < c; j++)
                {
                    fmpz_set_ui(fmpz_mat_entry(A, i, j), rnd() * (ulong) (1 + rnd_below(3)));
                    fmpz_mod(fmpz_mat_entry(A, i, j), fmpz_mat_entry(A, i, j), N);
                }
            adf_linsolve_mod(sol, A, b, N);
            fmpz_mat_init(H, c + r, r + c);              /* c rows of [A^T | I_c], then r zero rows */
            for (i = 0; i < c; i++)
            {
                for (j = 0; j < r; j++)
                    fmpz_set(fmpz_mat_entry(H, i, j), fmpz_mat_entry(A, j, i));
                fmpz_one(fmpz_mat_entry(H, i, r + i));
            }
            rank = fmpz_mat_howell_form_mod(H, N);
            for (i = 0; i < rank && ok; i++)
            {
                slong p = 0;

                while (p < r + c && fmpz_is_zero(fmpz_mat_entry(H, i, p)))
                    p++;
                if (p < r)
                {
                    ok = e < sol->E->r;
                    for (j = 0; ok && j < r; j++)
                        ok = fmpz_equal(fmpz_mat_entry(H, i, j), fmpz_mat_entry(sol->E, e, j));
                    for (j = 0; ok && j < c; j++)
                        ok = fmpz_equal(fmpz_mat_entry(H, i, r + j), fmpz_mat_entry(sol->V, e, j));
                    e++;
                }
                else
                {
                    ok = g < sol->G->r;
                    for (j = 0; ok && j < c; j++)
                        ok = fmpz_equal(fmpz_mat_entry(H, i, r + j), fmpz_mat_entry(sol->G, g, j));
                    g++;
                }
            }
            ok = ok && e == sol->E->r && g == sol->G->r;
            n++;
            same += ok;
            fmpz_mat_clear(A);
            fmpz_mat_clear(b);
            fmpz_mat_clear(H);
        }
    }
    printf("wrapper test (FLINT fmpz_mat_howell_form_mod, padded): %lu matrices, %lu identical to Algorithm H\n",
           n, same);
    ADF_CHECK(n > 0 && same == n);
    fmpz_clear(N);
    adf_linsol_clear(sol);
}
