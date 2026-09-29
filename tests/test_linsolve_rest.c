/* tests/test_linsolve_rest.c: the rest of linsolve.h (slice 2 of S.1, lane s1-slice2): adf_linsol_set,
   _swap, _identical, _kernel_order, _get_image_cert, _contains, adf_linsolve_fball,
   adf_linsol_verify_fball, adf_linsol_get_fball. docs/api-s.md section 3; docs/proofs/solvers.md
   Lemma 2.3 (line 551), Propositions 2.6, 2.9 (line 841), 2.10 (line 883).

   The oracles, in order of independence:
   1. The enumeration of (Z/N)^c with machine integers, written in this file (as in
      tests/test_linsolve.c): the kernel, the set of solutions, the span of the returned G.
   2. For the balls: membership of A x in the ball (a + H Zhat)/d of the RAW triple that this file chose,
      decided on the integer point x by exact integer arithmetic, d (A x) - a divisible by H (the
      criterion of precision.md Lemma 1, as check_s1_adelic of proto/solvers_checks.py:1426 does), on
      x in [0, N)^c and on x + N z. The library never sees this criterion.
   3. tests/ref/vectors/s1-slice2/fball.jsonl, written by lanes/s1-slice2/gen_vectors.py from
      adelic_system and linsolve_mod of proto/solvers_checks.py: every line is run. */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <flint/fmpz.h>
#include <flint/fmpz_mat.h>
#include <flint/ulong_extras.h>

#include <adelefeld.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* ---- small helpers (the same as in tests/test_linsolve.c, written again: that file owns its statics) ---- */

static ulong rng_state = 0xD1B54A32D192ED03UL;

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

#define MAXD 6
#define MAXSET 6000

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

/* the set {x : A x = b} (zero_rhs: b = 0) as a bitmap of N^c entries; returns its size */
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

/* the span of the rows of M over Z/N (entries read as machine integers); returns its size */
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
    for (i = 0; i < r; i++)
    {
        s->b[i] += N * (rnd_below(5) - 2);
        for (j = 0; j < c; j++)
            s->A[i][j] += N * (rnd_below(5) - 2);
    }
}

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

/* a deep copy written here by hand, NOT by adf_linsol_set, so that set can be tested against it */
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

/* field by field, written here, NOT by adf_linsol_identical */
static int
linsol_same(const adf_linsol_t a, const adf_linsol_t b)
{
    return a->kind == b->kind && fmpz_equal(a->N, b->N) && a->r == b->r && a->c == b->c && mat_same(a->G, b->G)
           && mat_same(a->E, b->E) && mat_same(a->V, b->V) && mat_same(a->x0, b->x0) && mat_same(a->y, b->y);
}

/* ------------------------------------------------------------------------------------------------
   1. set, swap, identical (the grid of slice 1: all systems modulo 4 with r, c <= 2)
   ------------------------------------------------------------------------------------------------ */

/* the system number code of the grid: r, c and the r c + r digits of A and b in base 4 */
static void
grid_sys(sys_t * s, slong r, slong c, slong code)
{
    slong i, j;

    s->N = 4;
    s->r = r;
    s->c = c;
    for (i = 0; i < r; i++)
        for (j = 0; j < c; j++)
        {
            s->A[i][j] = code % 4;
            code /= 4;
        }
    for (i = 0; i < r; i++)
    {
        s->b[i] = code % 4;
        code /= 4;
    }
}

static int
solve_sys(adf_linsol_t sol, const sys_t * s)
{
    fmpz_mat_t A, b;
    fmpz_t N;
    int st;

    sys_to_fmpz(A, b, N, s);
    st = adf_linsolve_mod(sol, A, b, N);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    return st;
}

ADF_TEST(set_swap_identical_on_the_grid)
{
    slong r, c, code, total = 0, nsame = 0, ndiff = 0, npermuted_same = 0, npermuted_diff = 0;
    slong nfail = 0;

    for (r = 0; r <= 2; r++)
        for (c = 0; c <= 2; c++)
            for (code = 0; code < ipow(4, r * c + r); code++)
            {
                sys_t s, p;
                adf_linsol_t a, b, cpy, d;
                int ok = 1;

                grid_sys(&s, r, c, code);
                adf_linsol_init(a);
                adf_linsol_init(b);
                adf_linsol_init(cpy);
                adf_linsol_init(d);
                solve_sys(a, &s);
                solve_sys(b, &s);
                /* two runs on the same system are identical */
                ok = ok && adf_linsol_identical(a, b) == 1 && linsol_same(a, b);
                /* set: a deep copy, equal field by field, independent memory */
                adf_linsol_set(cpy, a);
                ok = ok && linsol_same(cpy, a) && adf_linsol_identical(cpy, a) == 1;
                ok = ok && (a->G->r == 0 || cpy->G->rows != a->G->rows);
                /* set onto a value that holds something else releases it: d holds a permuted system */
                p = s;
                if (r >= 2)
                {
                    slong j;
                    slong t;

                    for (j = 0; j < c; j++)
                    {
                        t = p.A[0][j];
                        p.A[0][j] = p.A[r - 1][j];
                        p.A[r - 1][j] = t;
                    }
                    t = p.b[0];
                    p.b[0] = p.b[r - 1];
                    p.b[r - 1] = t;
                }
                solve_sys(d, &p);
                {
                    /* identical exactly when kind, G, E, V, x0 and y agree (N, r, c agree by construction) */
                    int agree = d->kind == a->kind && mat_same(d->G, a->G) && mat_same(d->E, a->E)
                                && mat_same(d->V, a->V) && mat_same(d->x0, a->x0) && mat_same(d->y, a->y);

                    ok = ok && adf_linsol_identical(a, d) == agree && adf_linsol_identical(d, a) == agree;
                    ok = ok && mat_same(d->G, a->G);       /* the kernel never depends on the order */
                    if (agree)
                        npermuted_same++;
                    else
                        npermuted_diff++;
                }
                adf_linsol_set(d, a);
                ok = ok && linsol_same(d, a);
                /* aliasing: set(x, x) leaves x unchanged and valid */
                adf_linsol_set(cpy, cpy);
                ok = ok && linsol_same(cpy, a) && adf_linsol_identical(cpy, cpy) == 1;
                /* swap: exchanges; swap(x, x) does nothing */
                {
                    adf_linsol_t e;

                    adf_linsol_init(e);
                    adf_linsol_swap(cpy, e);
                    ok = ok && linsol_same(e, a) && cpy->kind == ADF_LINSOL_COSET && fmpz_is_one(cpy->N)
                         && cpy->r == 0 && cpy->c == 0 && cpy->G->r == 0 && cpy->x0->r == 0;
                    adf_linsol_swap(e, e);
                    ok = ok && linsol_same(e, a);
                    adf_linsol_swap(e, cpy);
                    ok = ok && linsol_same(cpy, a);
                    adf_linsol_clear(e);
                }
                /* a system with another right-hand side: identical exactly when the fields agree */
                if (r >= 1)
                {
                    sys_t q = s;
                    adf_linsol_t f;
                    int agree;

                    q.b[0] = (q.b[0] + 1) % 4;
                    adf_linsol_init(f);
                    solve_sys(f, &q);
                    agree = f->kind == a->kind && mat_same(f->G, a->G) && mat_same(f->E, a->E)
                            && mat_same(f->V, a->V) && mat_same(f->x0, a->x0) && mat_same(f->y, a->y);
                    ok = ok && adf_linsol_identical(a, f) == agree;
                    if (agree)
                        nsame++;
                    else
                        ndiff++;
                    adf_linsol_clear(f);
                }
                total++;
                if (!ok)
                    nfail++;
                ADF_CHECK_MSG(ok, "r %ld c %ld code %ld", r, c, code);
                adf_linsol_clear(a);
                adf_linsol_clear(b);
                adf_linsol_clear(cpy);
                adf_linsol_clear(d);
            }
    printf("set/swap/identical: %ld systems, %ld failed; other rhs identical %ld, differing %ld; "
           "permuted equations identical %ld, differing %ld\n",
           total, nfail, nsame, ndiff, npermuted_same, npermuted_diff);
    ADF_CHECK(total == 4455 && nsame > 0 && ndiff > 0 && npermuted_same > 0 && npermuted_diff > 0);
}

/* identical is false after each single change of a value, in each of its fields */
ADF_TEST(identical_sees_every_field)
{
    sys_t s;
    adf_linsol_t a, b;
    slong which, i, ncase = 0;
    int kinds;

    for (kinds = 0; kinds < 2; kinds++)
    {
        s.N = 8;
        s.r = 1;
        s.c = 2;
        s.A[0][0] = 2;
        s.A[0][1] = 4;
        s.b[0] = kinds == 0 ? 6 : 1;
        adf_linsol_init(a);
        adf_linsol_init(b);
        ADF_CHECK(solve_sys(a, &s) == (kinds == 0 ? ADF_OK : ADF_NO_SOLUTION));
        for (which = 0; which < 14; which++)
        {
            fmpz_mat_struct * M = NULL;

            linsol_copy(b, a);
            ADF_CHECK(adf_linsol_identical(a, b) == 1);
            switch (which)
            {
                case 0: b->kind = 1 - b->kind; break;
                case 1: fmpz_add_ui(b->N, b->N, 1); break;
                case 2: b->r++; break;
                case 3: b->c++; break;
                case 4: M = b->G; break;
                case 5: M = b->E; break;
                case 6: M = b->V; break;
                case 7: M = b->x0; break;
                case 8: M = b->y; break;
                default: break;
            }
            if (which >= 4 && which <= 8)
            {
                if (M->r == 0 || M->c == 0)
                {
                    /* no entry to change: give the matrix another shape instead */
                    fmpz_mat_clear(M);
                    fmpz_mat_init(M, 1, 1);
                }
                else
                    fmpz_add_ui(fmpz_mat_entry(M, M->r - 1, M->c - 1), fmpz_mat_entry(M, M->r - 1, M->c - 1), 1);
            }
            else if (which == 9)
                mat_del_row(b->G, 0);
            else if (which == 10)
            {
                fmpz_mat_clear(b->E);
                fmpz_mat_init(b->E, a->E->r + 1, a->E->c);
                for (i = 0; i < a->E->r; i++)
                    fmpz_set(fmpz_mat_entry(b->E, i, 0), fmpz_mat_entry(a->E, i, 0));
            }
            else if (which == 11)
            {
                fmpz_mat_clear(b->V);
                fmpz_mat_init(b->V, a->V->r, a->V->c + 1);
            }
            else if (which == 12)
            {
                /* x0 and y: the same entries, the other shape (transposed 1 by n) */
                fmpz_mat_t t;

                fmpz_mat_init(t, a->x0->c, a->x0->r);
                fmpz_mat_swap(b->x0, t);
                fmpz_mat_clear(t);
                if (b->x0->r == a->x0->r && b->x0->c == a->x0->c)
                    b->kind = 1 - b->kind;   /* the shape 0 by 0 is its own transpose: change something */
            }
            else if (which == 13)
            {
                fmpz_mat_t t;

                fmpz_mat_init(t, a->y->c, a->y->r);
                fmpz_mat_swap(b->y, t);
                fmpz_mat_clear(t);
                if (b->y->r == a->y->r && b->y->c == a->y->c)
                    b->kind = 1 - b->kind;
            }
            ADF_CHECK_MSG(adf_linsol_identical(a, b) == 0 && adf_linsol_identical(b, a) == 0,
                          "kinds %d change %ld not seen", kinds, which);
            ncase++;
        }
        adf_linsol_clear(a);
        adf_linsol_clear(b);
    }
    printf("identical: %ld single changes, all seen\n", ncase);
}

/* ------------------------------------------------------------------------------------------------
   2. kernel_order, contains, get_image_cert (the grid, then random systems for the twelve small N)
   ------------------------------------------------------------------------------------------------ */

typedef struct
{
    unsigned long systems, contains_yes, contains_no, shifted, ord_gt1, notfree, bad;
} tally_t;

/* one solved small system against the enumeration */
static int
judge_rest(const sys_t * s, tally_t * t)
{
    static char sols[MAXSET], ker[MAXSET];
    adf_linsol_t sol;
    fmpz_mat_t A, b, x, E, V;
    fmpz_t N, ord;
    slong n = ipow(s->N, s->c), nsol, nker, idx, j;
    int st, ok = 1;

    sys_to_fmpz(A, b, N, s);
    adf_linsol_init(sol);
    st = adf_linsolve_mod(sol, A, b, N);
    nsol = solutions_brute(sols, s, 0);
    nker = solutions_brute(ker, s, 1);
    fmpz_init(ord);
    /* kernel_order = number of kernel elements, also for kind EMPTY */
    adf_linsol_kernel_order(ord, sol);
    ok = ok && fmpz_equal_si(ord, nker);
    if (nker > 1)
        t->ord_gt1++;
    /* the order is the product of N/g_i over the rows; the enumeration of the span of G agrees */
    {
        static char span[MAXSET];

        ok = ok && span_brute(span, sol->G, s->N, s->c) == nker;
    }
    /* for kind COSET also the number of solutions modulo N */
    if (st == ADF_OK)
        ok = ok && fmpz_equal_si(ord, nsol);
    else
        ok = ok && nsol == 0;
    /* contains agrees with the enumeration on every x in (Z/N)^c and on x + N z */
    fmpz_mat_init(x, s->c, 1);
    for (idx = 0; idx < n; idx++)
    {
        slong xv[MAXD];
        int want = sols[idx], got;

        idx_to_vec(xv, idx, s->N, s->c);
        for (j = 0; j < s->c; j++)
            fmpz_set_si(fmpz_mat_entry(x, j, 0), xv[j]);
        got = adf_linsol_contains(sol, x);
        ok = ok && got == want;
        if (want)
            t->contains_yes++;
        else
            t->contains_no++;
        if (idx % 3 == 0)
        {
            for (j = 0; j < s->c; j++)
                fmpz_set_si(fmpz_mat_entry(x, j, 0), xv[j] + s->N * (rnd_below(9) - 4));
            ok = ok && adf_linsol_contains(sol, x) == want;
            t->shifted++;
        }
    }
    fmpz_mat_clear(x);
    /* a wrong shape is refused, whatever the kind */
    {
        fmpz_mat_t w;

        fmpz_mat_init(w, s->c + 1, 1);
        ok = ok && adf_linsol_contains(sol, w) == 0;
        fmpz_mat_clear(w);
        fmpz_mat_init(w, s->c, 2);
        ok = ok && adf_linsol_contains(sol, w) == 0;
        fmpz_mat_clear(w);
    }
    /* get_image_cert: copies, shapes set by the function, alias of the destination allowed */
    fmpz_mat_init(E, 3, 5);
    fmpz_mat_init(V, 0, 2);
    adf_linsol_get_image_cert(E, V, sol);
    ok = ok && mat_same(E, sol->E) && mat_same(V, sol->V);
    if (E->r > 0)
        ok = ok && E->rows[0] != sol->E->rows[0];
    fmpz_mat_clear(E);
    fmpz_mat_clear(V);
    {
        adf_linsol_t keep;

        adf_linsol_init(keep);
        adf_linsol_set(keep, sol);
        adf_linsol_get_image_cert(sol->E, sol->V, sol);
        ok = ok && linsol_same(sol, keep);
        adf_linsol_clear(keep);
    }
    t->systems++;
    if (!ok)
        t->bad++;
    fmpz_clear(ord);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    adf_linsol_clear(sol);
    return ok;
}

ADF_TEST(kernel_order_contains_image_cert_on_the_grid)
{
    slong r, c, code;
    tally_t t;

    memset(&t, 0, sizeof t);
    for (r = 0; r <= 2; r++)
        for (c = 0; c <= 2; c++)
            for (code = 0; code < ipow(4, r * c + r); code++)
            {
                sys_t s;
                int ok;

                grid_sys(&s, r, c, code);
                ok = judge_rest(&s, &t);
                ADF_CHECK_MSG(ok, "r %ld c %ld code %ld", r, c, code);
            }
    printf("grid mod 4: %lu systems, contains yes %lu no %lu, shifted %lu, kernel order > 1 in %lu; %lu bad\n",
           t.systems, t.contains_yes, t.contains_no, t.shifted, t.ord_gt1, t.bad);
    ADF_CHECK(t.systems == 4455 && t.contains_yes > 0 && t.contains_no > 0 && t.ord_gt1 > 0 && t.bad == 0);
}

ADF_TEST(kernel_order_contains_image_cert_random_small_moduli)
{
    static const slong Ns[] = {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18};
    slong k, rep;
    tally_t t;

    memset(&t, 0, sizeof t);
    for (k = 0; k < 12; k++)
        for (rep = 0; rep < 150; rep++)
        {
            sys_t s;
            slong r = rnd_below(4), c = rnd_below(4);
            int ok;

            if (ipow(Ns[k], c) > MAXSET)
                continue;
            rand_sys(&s, Ns[k], r, c);
            ok = judge_rest(&s, &t);
            ADF_CHECK_MSG(ok, "N %ld r %ld c %ld", Ns[k], r, c);
        }
    printf("random small N: %lu systems, contains yes %lu no %lu, kernel order > 1 in %lu; %lu bad\n", t.systems,
           t.contains_yes, t.contains_no, t.ord_gt1, t.bad);
    ADF_CHECK(t.systems > 1500 && t.contains_yes > 0 && t.contains_no > 0 && t.ord_gt1 > 100 && t.bad == 0);
}

/* the order of a kernel with pivots that are not N (the product of N/g_i, not of g_i): x = 2 y mod 8 style */
ADF_TEST(kernel_order_named_values)
{
    adf_linsol_t sol;
    fmpz_mat_t A, b;
    fmpz_t N, n;

    fmpz_init(n);
    fmpz_init_set_ui(N, 8);
    /* 2 x + 4 y = 6 modulo 8: G = [[2,1],[0,2]], order (8/2)(8/2) = 16 */
    fmpz_mat_init(A, 1, 2);
    fmpz_mat_init(b, 1, 1);
    fmpz_set_ui(fmpz_mat_entry(A, 0, 0), 2);
    fmpz_set_ui(fmpz_mat_entry(A, 0, 1), 4);
    fmpz_set_ui(fmpz_mat_entry(b, 0, 0), 6);
    adf_linsol_init(sol);
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_OK);
    adf_linsol_kernel_order(n, sol);
    ADF_CHECK(fmpz_equal_ui(n, 16));
    /* the init value: N = 1, k = 0: order 1; the kernel of the empty system modulo 1 */
    adf_linsol_clear(sol);
    adf_linsol_init(sol);
    fmpz_set_ui(n, 77);
    adf_linsol_kernel_order(n, sol);
    ADF_CHECK(fmpz_is_one(n));
    /* N = 2^100, a x = 0 with a = 2^40: G = [2^60], order 2^40 (N / g, with g = 2^60) */
    fmpz_set_ui(N, 1);
    fmpz_mul_2exp(N, N, 100);
    fmpz_set_ui(fmpz_mat_entry(A, 0, 1), 0);
    fmpz_set_ui(fmpz_mat_entry(A, 0, 0), 1);
    fmpz_mul_2exp(fmpz_mat_entry(A, 0, 0), fmpz_mat_entry(A, 0, 0), 40);
    fmpz_zero(fmpz_mat_entry(b, 0, 0));
    ADF_CHECK(adf_linsolve_mod(sol, A, b, N) == ADF_OK);
    adf_linsol_kernel_order(n, sol);
    /* A = [2^40, 0]: G = [[2^60, 0], [0, 1]], the kernel is {(x, y) : 2^40 x = 0}: 2^40 * 2^100 elements */
    {
        fmpz_t want;

        fmpz_init_set_ui(want, 1);
        fmpz_mul_2exp(want, want, 140);
        ADF_CHECK(fmpz_equal(n, want));
        fmpz_clear(want);
    }
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    fmpz_clear(n);
    adf_linsol_clear(sol);
}

/* ------------------------------------------------------------------------------------------------
   3. adf_linsolve_fball, adf_linsol_verify_fball, adf_linsol_get_fball
   ------------------------------------------------------------------------------------------------ */

#define MAXR 4

typedef struct
{
    slong r, c;
    slong A[MAXR][MAXD];
    slong a[MAXR], H[MAXR], d[MAXR];      /* raw triples: the ball (a + H Zhat)/d, H >= 1, d >= 1 */
} fsys_t;

/* membership of A x in every ball of the raw triples, on the integer point x (exact integers) */
static int
fmember(const fsys_t * f, const slong * x)
{
    slong i, j;

    for (i = 0; i < f->r; i++)
    {
        slong val = 0;

        for (j = 0; j < f->c; j++)
            val += f->A[i][j] * x[j];
        if (mod_sl(f->d[i] * val - f->a[i], f->H[i]) != 0)
            return 0;
    }
    return 1;
}

/* the canonical radius of the raw triple: H / gcd(a, H, d) (conventions 5.2), for the expected N */
static slong
canon_H(slong a, slong H, slong d)
{
    return H / gcd_sl(gcd_sl(a, H), d);
}

static slong
lcm_sl(slong a, slong b)
{
    return a / gcd_sl(a, b) * b;
}

static void
fsys_build(const fsys_t * f, fmpz_mat_t A, adf_fball_struct * balls)
{
    slong i, j;

    fmpz_mat_init(A, f->r, f->c);
    for (i = 0; i < f->r; i++)
    {
        fmpz_t a, H, d;

        for (j = 0; j < f->c; j++)
            fmpz_set_si(fmpz_mat_entry(A, i, j), f->A[i][j]);
        fmpz_init_set_si(a, f->a[i]);
        fmpz_init_set_si(H, f->H[i]);
        fmpz_init_set_si(d, f->d[i]);
        adf_fball_init(balls + i);
        ADF_CHECK(adf_fball_set_fmpz3(balls + i, a, H, d) == ADF_OK);
        fmpz_clear(a);
        fmpz_clear(H);
        fmpz_clear(d);
    }
}

static void
fsys_random(fsys_t * f)
{
    static const slong Hs[] = {1, 2, 3, 4, 6, 8, 9, 12, 5, 10, 15, 18, 2, 3, 4};
    static const slong ds[] = {1, 1, 2, 3, 4, 6, 5, 8, 1};
    slong i, j;

    f->r = rnd_below(4);
    f->c = rnd_below(4);
    for (i = 0; i < f->r; i++)
    {
        f->H[i] = Hs[rnd_below(15)];
        f->d[i] = ds[rnd_below(9)];
        f->a[i] = rnd_below(41) - 20;
        if (rnd_below(4) == 0)
            f->a[i] *= f->d[i];            /* often not canonical: a, H, d with a common factor */
        for (j = 0; j < f->c; j++)
            f->A[i][j] = rnd_below(13) - 6;
    }
}

static slong
fsys_N(const fsys_t * f)
{
    slong i, N = 1;

    for (i = 0; i < f->r; i++)
        N = lcm_sl(N, canon_H(f->a[i], f->H[i], f->d[i]));
    return N;
}

/* the solutions of the fball system against the enumeration of x in [0, N)^c and x + N z; returns 1 if
   every claim holds; *nsol is the number of solutions modulo N */
static int
judge_fball(const fsys_t * f, const adf_linsol_t sol, int st, slong * nsol_out)
{
    static char ref[MAXSET], coset[MAXSET], span[MAXSET];
    slong N = fsys_N(f), n, idx, j, nsol = 0;
    slong x[MAXD], y[MAXD];
    int ok = 1;

    ok = ok && fmpz_equal_si(sol->N, N) && sol->r == f->r && sol->c == f->c;
    n = ipow(N, f->c);
    for (idx = 0; idx < n; idx++)
    {
        idx_to_vec(x, idx, N, f->c);
        ref[idx] = (char) fmember(f, x);
        nsol += ref[idx];
        if (rnd_below(3) == 0)              /* the class modulo N decides membership: x + N z, any z */
        {
            for (j = 0; j < f->c; j++)
                y[j] = x[j] + N * (rnd_below(11) - 5);
            ok = ok && fmember(f, y) == ref[idx];
        }
    }
    *nsol_out = nsol;
    if (nsol == 0)
        return ok && st == ADF_NO_SOLUTION && sol->kind == ADF_LINSOL_EMPTY;
    ok = ok && st == ADF_OK && sol->kind == ADF_LINSOL_COSET;
    if (!ok)
        return 0;
    span_brute(span, sol->G, N, f->c);
    memset(coset, 0, (size_t) n);
    for (idx = 0; idx < n; idx++)
    {
        if (!span[idx])
            continue;
        idx_to_vec(x, idx, N, f->c);
        for (j = 0; j < f->c; j++)
            y[j] = mod_sl(x[j] + fmpz_get_si(fmpz_mat_entry(sol->x0, j, 0)), N);
        coset[vec_to_idx(y, N, f->c)] = 1;
    }
    return memcmp(coset, ref, (size_t) n) == 0;                 /* x0 + S(G) = the solutions */
}

/* the ball of coordinate j against the residues of the j-th coordinates of all solutions */
static int
judge_get_fball(const fsys_t * f, const adf_linsol_t sol)
{
    static char ref[MAXSET];
    slong N = fsys_N(f), n = ipow(N, f->c), idx, j, t;
    slong x[MAXD];
    int ok = 1;

    for (j = 0; j < f->c; j++)
    {
        char coord[2048];
        adf_fball_t ball;
        fmpz_t A, H, d;

        memset(coord, 0, sizeof coord);
        for (idx = 0; idx < n; idx++)
        {
            idx_to_vec(x, idx, N, f->c);
            if (fmember(f, x))
                coord[x[j]] = 1;
        }
        (void) ref;
        adf_fball_init(ball);
        fmpz_init(A);
        fmpz_init(H);
        fmpz_init(d);
        ok = ok && adf_linsol_get_fball(ball, sol, j) == ADF_OK;
        ok = ok && adf_fball_is_canonical(ball) && ball->backend == ADF_GLOBAL;
        adf_fball_get_fmpz3(A, H, d, ball);
        ok = ok && fmpz_is_one(d) && fmpz_sgn(H) > 0 && fmpz_cmp_si(H, N) <= 0 && N % fmpz_get_si(H) == 0;
        if (ok)
        {
            slong h = fmpz_get_si(H), a = fmpz_get_si(A);

            /* the residues of the ball in [0, N) are exactly the coordinates of the solutions */
            for (t = 0; t < N; t++)
                ok = ok && (mod_sl(t - a, h) == 0) == (coord[t] != 0);
        }
        adf_fball_clear(ball);
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
    }
    return ok;
}

ADF_TEST(fball_against_exact_membership)
{
    slong rep, nsys = 0, nok = 0, nnone = 0, nskip = 0, nbad = 0, nnotcanon = 0, ncoord = 0, nlcm_lt_prod = 0;

    for (rep = 0; rep < 3000; rep++)
    {
        fsys_t f;
        fmpz_mat_t A;
        adf_fball_struct balls[MAXR];
        adf_linsol_t sol;
        slong i, nsol, N;
        int st, ok = 1;

        fsys_random(&f);
        N = fsys_N(&f);
        if (ipow(N, f.c) > MAXSET)
        {
            nskip++;
            continue;
        }
        {
            slong prod = 1;

            for (i = 0; i < f.r; i++)
                prod *= canon_H(f.a[i], f.H[i], f.d[i]);
            if (N < prod)
                nlcm_lt_prod++;
            for (i = 0; i < f.r; i++)
                if (canon_H(f.a[i], f.H[i], f.d[i]) != f.H[i] || f.d[i] != 1)
                    nnotcanon++;
        }
        fsys_build(&f, A, balls);
        adf_linsol_init(sol);
        st = adf_linsolve_fball(sol, A, balls, f.r);
        ok = judge_fball(&f, sol, st, &nsol);
        /* verify_fball accepts the result */
        ok = ok && adf_linsol_verify_fball(sol, A, balls, f.r) == 1 && adf_linsol_is_canonical(sol);
        if (ok && st == ADF_OK)
        {
            ok = judge_get_fball(&f, sol);
            ncoord += f.c;
        }
        /* verify_fball refuses a changed x0 (kind COSET) that is not a solution */
        if (ok && st == ADF_OK && f.c > 0)
        {
            adf_linsol_t w;
            slong t;

            adf_linsol_init(w);
            adf_linsol_set(w, sol);
            for (t = 1; t < N; t++)
            {
                fmpz_add_ui(fmpz_mat_entry(w->x0, 0, 0), fmpz_mat_entry(w->x0, 0, 0), 1);
                fmpz_mod(fmpz_mat_entry(w->x0, 0, 0), fmpz_mat_entry(w->x0, 0, 0), w->N);
                {
                    fmpz_mat_t xx;
                    int inside;

                    fmpz_mat_init(xx, f.c, 1);
                    mat_copy(xx, w->x0);
                    inside = adf_linsol_contains(sol, xx);
                    fmpz_mat_clear(xx);
                    ok = ok && adf_linsol_verify_fball(w, A, balls, f.r) == inside;
                }
            }
            adf_linsol_clear(w);
        }
        /* the balls are unchanged */
        for (i = 0; i < f.r; i++)
        {
            fmpz_t a, H, d;

            fmpz_init(a);
            fmpz_init(H);
            fmpz_init(d);
            adf_fball_get_fmpz3(a, H, d, balls + i);
            ok = ok && fmpz_get_si(H) == canon_H(f.a[i], f.H[i], f.d[i]);
            fmpz_clear(a);
            fmpz_clear(H);
            fmpz_clear(d);
        }
        nsys++;
        if (st == ADF_OK)
            nok++;
        else
            nnone++;
        if (!ok)
            nbad++;
        ADF_CHECK_MSG(ok, "system %ld (r %ld c %ld N %ld)", rep, f.r, f.c, N);
        for (i = 0; i < f.r; i++)
            adf_fball_clear(balls + i);
        fmpz_mat_clear(A);
        adf_linsol_clear(sol);
    }
    printf("fball: %ld systems (%ld skipped for N^c > %d); OK %ld, NO_SOLUTION %ld; lcm below the product in %ld; "
           "non-canonical raw triples %ld; coordinate balls checked %ld; %ld bad\n",
           nsys, nskip, MAXSET, nok, nnone, nlcm_lt_prod, nnotcanon, ncoord, nbad);
    ADF_CHECK(nsys > 2000 && nok > 200 && nnone > 200 && nlcm_lt_prod > 100 && nnotcanon > 500 && nbad == 0);
}

/* same balls in the local backend and in the global backend: identical results */
ADF_TEST(fball_local_and_global_backends_agree)
{
    const ulong q[4] = {16, 27, 25, 7};                 /* K = 75600 */
    adf_modctx_struct * ctx = NULL;
    slong rep, nsys = 0, nlocal = 0, nbad = 0, nmixed = 0;

    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 4) == ADF_OK);
    for (rep = 0; rep < 600; rep++)
    {
        fsys_t f;
        fmpz_mat_t A;
        adf_fball_struct gl[MAXR], lo[MAXR];
        adf_linsol_t s1, s2;
        slong i, N;
        int ok = 1, st1, st2, any_local = 0, all_local = 1;

        fsys_random(&f);
        for (i = 0; i < f.r; i++)             /* radii that divide K: 1, 2, 3, 4, 6, 8, 9, 12, 5, 10, 15, 18 */
            if (75600 % f.H[i] != 0)
                f.H[i] = 1;
        if (f.r > 0 && rnd_below(4) == 0)
            f.H[0] = 11;                      /* 11 does not divide K: this ball stays global (mixed arrays) */
        N = fsys_N(&f);
        if (ipow(N, f.c) > MAXSET)
            continue;
        fsys_build(&f, A, gl);
        for (i = 0; i < f.r; i++)
        {
            adf_fball_init(lo + i);
            if (adf_fball_set_local(lo + i, gl + i, ctx) == ADF_OK)
            {
                any_local = 1;
                ok = ok && lo[i].backend == ADF_LOCAL;
            }
            else
            {
                all_local = 0;
                adf_fball_set(lo + i, gl + i);   /* not a local value of ctx: stays global */
            }
        }
        adf_linsol_init(s1);
        adf_linsol_init(s2);
        st1 = adf_linsolve_fball(s1, A, gl, f.r);
        st2 = adf_linsolve_fball(s2, A, lo, f.r);
        ok = ok && st1 == st2 && adf_linsol_identical(s1, s2) == 1;
        ok = ok && adf_linsol_verify_fball(s1, A, lo, f.r) == 1 && adf_linsol_verify_fball(s2, A, gl, f.r) == 1;
        nsys++;
        nlocal += any_local;
        nmixed += any_local && !all_local;
        if (!ok)
            nbad++;
        ADF_CHECK_MSG(ok, "system %ld", rep);
        for (i = 0; i < f.r; i++)
        {
            adf_fball_clear(gl + i);
            adf_fball_clear(lo + i);
        }
        fmpz_mat_clear(A);
        adf_linsol_clear(s1);
        adf_linsol_clear(s2);
    }
    adf_modctx_free(ctx);
    printf("local/global: %ld systems, %ld with some local ball, %ld mixed; %ld bad\n", nsys, nlocal, nmixed, nbad);
    ADF_CHECK(nsys > 300 && nlocal > 100 && nmixed > 30 && nbad == 0);
}

/* statuses and the state of the outputs */
ADF_TEST(fball_statuses)
{
    fmpz_mat_t A;
    adf_fball_struct b[3];
    adf_linsol_t sol, keep;
    fmpz_t a, H, d;
    slong i;

    fmpz_init(a);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_mat_init(A, 2, 2);
    for (i = 0; i < 3; i++)
        adf_fball_init(b + i);
    fmpz_set_si(fmpz_mat_entry(A, 0, 0), 1);
    fmpz_set_si(fmpz_mat_entry(A, 1, 1), 1);
    fmpz_set_si(a, 3);
    fmpz_set_si(H, 12);
    fmpz_set_si(d, 1);
    ADF_CHECK(adf_fball_set_fmpz3(b + 0, a, H, d) == ADF_OK);
    fmpz_set_si(H, 8);
    ADF_CHECK(adf_fball_set_fmpz3(b + 1, a, H, d) == ADF_OK);
    fmpz_zero(H);
    ADF_CHECK(adf_fball_set_fmpz3(b + 2, a, H, d) == ADF_OK);        /* exact 3 */
    ADF_CHECK(adf_fball_is_exact(b + 2));

    /* sol holds a prior result; every refusal leaves it as it was */
    adf_linsol_init(sol);
    adf_linsol_init(keep);
    ADF_CHECK(adf_linsolve_fball(sol, A, b, 2) == ADF_OK);
    ADF_CHECK(fmpz_equal_si(sol->N, 24));                            /* lcm(12, 8), not 96 */
    adf_linsol_set(keep, sol);

    ADF_CHECK(adf_linsolve_fball(sol, A, b, 3) == ADF_DOMAIN && linsol_same(sol, keep));
    ADF_CHECK(adf_linsolve_fball(sol, A, b, 1) == ADF_DOMAIN && linsol_same(sol, keep));
    ADF_CHECK(adf_linsolve_fball(sol, A, b, -1) == ADF_DOMAIN && linsol_same(sol, keep));
    ADF_CHECK(adf_linsolve_fball(sol, A, b, 0) == ADF_DOMAIN && linsol_same(sol, keep));
    adf_fball_swap(b + 1, b + 2);                                    /* an exact ball among the two */
    ADF_CHECK(adf_linsolve_fball(sol, A, b, 2) == ADF_UNSUPPORTED && linsol_same(sol, keep));
    adf_fball_swap(b + 1, b + 2);
    adf_fball_swap(b + 0, b + 2);
    ADF_CHECK(adf_linsolve_fball(sol, A, b, 2) == ADF_UNSUPPORTED && linsol_same(sol, keep));
    adf_fball_swap(b + 0, b + 2);
    /* DOMAIN is decided before UNSUPPORTED: r wrong and an exact ball present */
    adf_fball_swap(b + 0, b + 2);
    ADF_CHECK(adf_linsolve_fball(sol, A, b, 3) == ADF_DOMAIN && linsol_same(sol, keep));
    adf_fball_swap(b + 0, b + 2);
    /* verify_fball: 0 for wrong r and for an exact ball, 1 for the result */
    ADF_CHECK(adf_linsol_verify_fball(sol, A, b, 2) == 1);
    ADF_CHECK(adf_linsol_verify_fball(sol, A, b, 3) == 0 && adf_linsol_verify_fball(sol, A, b, 1) == 0);
    adf_fball_swap(b + 1, b + 2);
    ADF_CHECK(adf_linsol_verify_fball(sol, A, b, 2) == 0);
    adf_fball_swap(b + 1, b + 2);
    /* r = 0: N = 1, every x is a solution: G = the identity, x0 = 0 */
    {
        fmpz_mat_t A0;

        fmpz_mat_init(A0, 0, 2);
        ADF_CHECK(adf_linsolve_fball(sol, A0, NULL, 0) == ADF_OK);
        ADF_CHECK(fmpz_is_one(sol->N) && sol->r == 0 && sol->c == 2 && adf_linsol_kernel_rows(sol) == 0);
        ADF_CHECK(sol->x0->r == 2 && fmpz_is_zero(fmpz_mat_entry(sol->x0, 0, 0)));
        fmpz_mat_clear(A0);
    }
    for (i = 0; i < 3; i++)
        adf_fball_clear(b + i);
    fmpz_mat_clear(A);
    fmpz_clear(a);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_linsol_clear(sol);
    adf_linsol_clear(keep);
}

/* adf_linsol_get_fball: statuses, the state of x, the meaning on named systems */
ADF_TEST(get_fball_statuses_and_named_values)
{
    adf_linsol_t sol, e;
    fmpz_mat_t A, b;
    fmpz_t N, a, H, d;
    adf_fball_t x, mark;
    sys_t s;

    fmpz_init(a);
    fmpz_init(H);
    fmpz_init(d);
    adf_fball_init(x);
    adf_fball_init(mark);
    fmpz_set_si(a, 7);
    fmpz_set_si(H, 5);
    fmpz_set_si(d, 3);
    ADF_CHECK(adf_fball_set_fmpz3(mark, a, H, d) == ADF_OK);
    ADF_CHECK(adf_fball_set_fmpz3(x, a, H, d) == ADF_OK);

    /* x1 + x2 = 0 modulo 2 (solvers P2.10, the "larger" example): both coordinates run through all of Zhat */
    s.N = 2;
    s.r = 1;
    s.c = 2;
    s.A[0][0] = 1;
    s.A[0][1] = 1;
    s.b[0] = 0;
    adf_linsol_init(sol);
    ADF_CHECK(solve_sys(sol, &s) == ADF_OK);
    {
        int j;

        for (j = 0; j < 2; j++)
        {
            ADF_CHECK(adf_linsol_get_fball(x, sol, j) == ADF_OK);
            adf_fball_get_fmpz3(a, H, d, x);
            ADF_CHECK_MSG(fmpz_is_zero(a) && fmpz_is_one(H) && fmpz_is_one(d), "coordinate %d is the ball Zhat", j);
        }
    }
    /* 2 x = 6 modulo 8 (c = 1): x = 3 mod 4: the ball 3 + 4 Zhat, rho = 4 */
    s.N = 8;
    s.r = 1;
    s.c = 1;
    s.A[0][0] = 2;
    s.b[0] = 6;
    ADF_CHECK(solve_sys(sol, &s) == ADF_OK);
    ADF_CHECK(adf_linsol_get_fball(x, sol, 0) == ADF_OK);
    adf_fball_get_fmpz3(a, H, d, x);
    ADF_CHECK(fmpz_equal_si(a, 3) && fmpz_equal_si(H, 4) && fmpz_is_one(d));
    /* 3 x = 4 modulo 8: unit, x = 4 exactly modulo 8: G empty, rho = N = 8, the ball 4 + 8 Zhat
       (with k = 0 the gcd is N itself) */
    s.A[0][0] = 3;
    s.b[0] = 4;
    ADF_CHECK(solve_sys(sol, &s) == ADF_OK && adf_linsol_kernel_rows(sol) == 0);
    ADF_CHECK(adf_linsol_get_fball(x, sol, 0) == ADF_OK);
    adf_fball_get_fmpz3(a, H, d, x);
    ADF_CHECK(fmpz_equal_si(a, 4) && fmpz_equal_si(H, 8) && fmpz_is_one(d));
    /* 6 x = 4 modulo 18, a huge x0 representative: the centre is reduced modulo rho: x0 mod rho */
    s.N = 18;
    s.A[0][0] = 6;
    s.b[0] = 12;
    ADF_CHECK(solve_sys(sol, &s) == ADF_OK);
    ADF_CHECK(adf_linsol_get_fball(x, sol, 0) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(x));
    adf_fball_get_fmpz3(a, H, d, x);
    ADF_CHECK(fmpz_equal_si(H, 3) && fmpz_equal_si(a, 2) && fmpz_is_one(d));         /* 6 x = 12 mod 18: x = 2 mod 3 */

    /* DOMAIN: j outside [0, c); x untouched, also for kind EMPTY (DOMAIN is decided first) */
    ADF_CHECK(adf_fball_set_fmpz3(x, a, H, d) == ADF_OK);
    ADF_CHECK(adf_linsol_get_fball(mark, sol, -1) == ADF_DOMAIN);
    ADF_CHECK(adf_linsol_get_fball(mark, sol, 1) == ADF_DOMAIN);
    ADF_CHECK(adf_linsol_get_fball(mark, sol, 1000) == ADF_DOMAIN);
    fmpz_set_si(a, 7);
    fmpz_set_si(H, 5);
    fmpz_set_si(d, 3);
    ADF_CHECK(adf_fball_set_fmpz3(x, a, H, d) == ADF_OK);
    ADF_CHECK(adf_fball_identical(x, mark) == 1);                                   /* untouched */
    /* NO_SOLUTION on kind EMPTY; x untouched */
    s.N = 8;
    s.A[0][0] = 2;
    s.b[0] = 1;
    adf_linsol_init(e);
    ADF_CHECK(solve_sys(e, &s) == ADF_NO_SOLUTION && adf_linsol_kind(e) == ADF_LINSOL_EMPTY);
    ADF_CHECK(adf_linsol_get_fball(mark, e, 0) == ADF_NO_SOLUTION && adf_fball_identical(x, mark) == 1);
    ADF_CHECK(adf_linsol_get_fball(mark, e, 1) == ADF_DOMAIN && adf_fball_identical(x, mark) == 1);
    /* the init value: c = 0: every j is outside */
    adf_linsol_clear(e);
    adf_linsol_init(e);
    ADF_CHECK(adf_linsol_get_fball(mark, e, 0) == ADF_DOMAIN && adf_fball_identical(x, mark) == 1);

    /* x may hold a local value on entry: the result is global */
    {
        const ulong q[1] = {12};
        adf_modctx_struct * ctx = NULL;
        adf_fball_t loc;

        ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 1) == ADF_OK);
        adf_fball_init(loc);
        fmpz_set_si(a, 5);
        fmpz_set_si(H, 12);
        fmpz_set_si(d, 1);
        ADF_CHECK(adf_fball_set_fmpz3(loc, a, H, d) == ADF_OK);
        ADF_CHECK(adf_fball_set_local(loc, loc, ctx) == ADF_OK && loc->backend == ADF_LOCAL);
        s.N = 8;
        s.A[0][0] = 2;
        s.b[0] = 6;
        ADF_CHECK(solve_sys(sol, &s) == ADF_OK);
        ADF_CHECK(adf_linsol_get_fball(loc, sol, 0) == ADF_OK);
        ADF_CHECK(loc->backend == ADF_GLOBAL && loc->mctx == NULL && loc->res == NULL && adf_fball_is_canonical(loc));
        adf_fball_clear(loc);
        adf_modctx_free(ctx);
    }
    (void) A;
    (void) b;
    (void) N;
    fmpz_clear(a);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
    adf_fball_clear(mark);
    adf_linsol_clear(sol);
    adf_linsol_clear(e);
}

/* the residues of the coordinates of all solutions against the ball, on the whole small grid mod 4 and the
   random small moduli (solvers P2.10; the enumeration is the oracle) */
static int
judge_coords(const sys_t * s, unsigned long * count)
{
    static char sols[MAXSET];
    adf_linsol_t sol;
    slong n = ipow(s->N, s->c), idx, j, t;
    slong x[MAXD];
    int st, ok = 1;

    adf_linsol_init(sol);
    st = solve_sys(sol, s);
    solutions_brute(sols, s, 0);
    for (j = 0; j < s->c; j++)
    {
        adf_fball_t ball;
        fmpz_t A, H, d;
        char coord[2048];
        int stb;

        memset(coord, 0, sizeof coord);
        for (idx = 0; idx < n; idx++)
        {
            idx_to_vec(x, idx, s->N, s->c);
            if (sols[idx])
                coord[x[j]] = 1;
        }
        adf_fball_init(ball);
        fmpz_init(A);
        fmpz_init(H);
        fmpz_init(d);
        stb = adf_linsol_get_fball(ball, sol, j);
        if (st == ADF_NO_SOLUTION)
            ok = ok && stb == ADF_NO_SOLUTION;
        else
        {
            ok = ok && stb == ADF_OK && adf_fball_is_canonical(ball);
            adf_fball_get_fmpz3(A, H, d, ball);
            ok = ok && fmpz_is_one(d) && fmpz_sgn(H) > 0 && s->N % fmpz_get_si(H) == 0;
            if (ok)
                for (t = 0; t < s->N; t++)
                    ok = ok && (mod_sl(t - fmpz_get_si(A), fmpz_get_si(H)) == 0) == (coord[t] != 0);
            (*count)++;
        }
        adf_fball_clear(ball);
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
    }
    adf_linsol_clear(sol);
    return ok;
}

ADF_TEST(get_fball_residues_on_the_grid)
{
    static const slong Ns[] = {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18};
    slong r, c, code, k, rep;
    unsigned long count = 0, nsys = 0, bad = 0;

    for (r = 0; r <= 2; r++)
        for (c = 0; c <= 2; c++)
            for (code = 0; code < ipow(4, r * c + r); code++)
            {
                sys_t s;
                int ok;

                grid_sys(&s, r, c, code);
                ok = judge_coords(&s, &count);
                ADF_CHECK_MSG(ok, "r %ld c %ld code %ld", r, c, code);
                nsys++;
                bad += !ok;
            }
    for (k = 0; k < 12; k++)
        for (rep = 0; rep < 100; rep++)
        {
            sys_t s;
            slong rr = rnd_below(4), cc = rnd_below(4);
            int ok;

            if (ipow(Ns[k], cc) > MAXSET)
                continue;
            rand_sys(&s, Ns[k], rr, cc);
            ok = judge_coords(&s, &count);
            ADF_CHECK_MSG(ok, "N %ld r %ld c %ld", Ns[k], rr, cc);
            nsys++;
            bad += !ok;
        }
    printf("get_fball: %lu systems, %lu coordinate balls with solutions compared with the residues; %lu bad\n",
           nsys, count, bad);
    ADF_CHECK(nsys > 5000 && count > 5000 && bad == 0);
}

/* ------------------------------------------------------------------------------------------------
   4. large operands, LIMIT
   ------------------------------------------------------------------------------------------------ */

ADF_TEST(fball_large_operands_planted_solution)
{
    /* A with entries of about 3000 bits, radii of 1500 to 2000 bits (one a power of 2 times a power of 3),
       denominators of 40 bits; the balls contain A x for a planted x of 200 bits, and the ball centres are
       d (A x), so A x is in the ball exactly. Then x, and x + N z for z of 3000 bits, must be in the coset. */
    slong r = 3, c = 3, i, j, trial;

    for (trial = 0; trial < 3; trial++)
    {
        fmpz_mat_t A, x, xs;
        adf_fball_struct b[3];
        adf_linsol_t sol;
        fmpz_t t, Ai, Hi, di, val, z;
        flint_rand_t st;
        int stat;

        flint_randinit(st);
        for (i = 0; i < 17 * (trial + 1); i++)
            n_randlimb(st);
        fmpz_mat_init(A, r, c);
        fmpz_mat_init(x, c, 1);
        fmpz_mat_init(xs, c, 1);
        fmpz_init(t);
        fmpz_init(Ai);
        fmpz_init(Hi);
        fmpz_init(di);
        fmpz_init(val);
        fmpz_init(z);
        for (i = 0; i < r; i++)
            for (j = 0; j < c; j++)
                fmpz_randtest(fmpz_mat_entry(A, i, j), st, 3000);
        for (j = 0; j < c; j++)
            fmpz_randtest(fmpz_mat_entry(x, j, 0), st, 200);
        for (i = 0; i < r; i++)
        {
            adf_fball_init(b + i);
            fmpz_zero(val);
            for (j = 0; j < c; j++)
                fmpz_addmul(val, fmpz_mat_entry(A, i, j), fmpz_mat_entry(x, j, 0));
            fmpz_randbits(di, st, 40);
            fmpz_abs(di, di);
            fmpz_add_ui(di, di, 1);
            fmpz_mul(Ai, val, di);                              /* centre d (A x) / d = A x */
            if (i == 0)
            {
                fmpz_set_ui(Hi, 1);
                fmpz_mul_2exp(Hi, Hi, 1700);
                fmpz_set_ui(t, 3);
                fmpz_pow_ui(t, t, 300);
                fmpz_mul(Hi, Hi, t);
            }
            else
            {
                fmpz_randbits(Hi, st, 1500 + 200 * (slong) i);
                fmpz_abs(Hi, Hi);
                fmpz_add_ui(Hi, Hi, 1);
            }
            ADF_CHECK(adf_fball_set_fmpz3(b + i, Ai, Hi, di) == ADF_OK);
        }
        adf_linsol_init(sol);
        stat = adf_linsolve_fball(sol, A, b, r);
        ADF_CHECK(stat == ADF_OK);
        ADF_CHECK(adf_linsol_verify_fball(sol, A, b, r) == 1);
        /* N is the lcm of the canonical radii: it is above the largest radius and below their product */
        ADF_CHECK(fmpz_bits(sol->N) >= 1700 + 1 * 0);
        ADF_CHECK(adf_linsol_contains(sol, x) == 1);
        for (j = 0; j < c; j++)
        {
            fmpz_randtest(z, st, 3000);
            fmpz_addmul(fmpz_mat_entry(xs, j, 0), sol->N, z);
            fmpz_add(fmpz_mat_entry(xs, j, 0), fmpz_mat_entry(xs, j, 0), fmpz_mat_entry(x, j, 0));
        }
        ADF_CHECK(adf_linsol_contains(sol, xs) == 1);
        /* exact check of the planted point: (A x)_i is in ball i, by the criterion d (A x) - a divisible by H */
        for (i = 0; i < r; i++)
        {
            fmpz_t a, H, d;

            fmpz_init(a);
            fmpz_init(H);
            fmpz_init(d);
            adf_fball_get_fmpz3(a, H, d, b + i);
            fmpz_zero(val);
            for (j = 0; j < c; j++)
                fmpz_addmul(val, fmpz_mat_entry(A, i, j), fmpz_mat_entry(xs, j, 0));
            fmpz_mul(val, val, d);
            fmpz_sub(val, val, a);
            ADF_CHECK(fmpz_divisible(val, H));
            fmpz_clear(a);
            fmpz_clear(H);
            fmpz_clear(d);
        }
        /* the coordinate balls contain the planted coordinates: x_j - x0[j] is a multiple of rho_j */
        for (j = 0; j < c; j++)
        {
            adf_fball_t ball;
            adf_rat_t q;

            adf_fball_init(ball);
            adf_rat_init(q);
            ADF_CHECK(adf_linsol_get_fball(ball, sol, j) == ADF_OK);
            adf_rat_set_fmpz(q, fmpz_mat_entry(x, j, 0));
            ADF_CHECK(adf_fball_contains_rat(ball, q) == 1);
            adf_rat_clear(q);
            adf_fball_clear(ball);
        }
        for (i = 0; i < r; i++)
            adf_fball_clear(b + i);
        adf_linsol_clear(sol);
        fmpz_mat_clear(A);
        fmpz_mat_clear(x);
        fmpz_mat_clear(xs);
        fmpz_clear(t);
        fmpz_clear(Ai);
        fmpz_clear(Hi);
        fmpz_clear(di);
        fmpz_clear(val);
        fmpz_clear(z);
        flint_randclear(st);
    }
}

ADF_TEST(fball_limit_and_dimension)
{
    slong r = ADF_LINSOLVE_DIM_MAX + 1, i;
    fmpz_mat_t A;
    adf_fball_struct * b = flint_malloc((size_t) r * sizeof(adf_fball_struct));
    adf_linsol_t sol, keep;

    fmpz_mat_init(A, r, 0);
    for (i = 0; i < r; i++)
        adf_fball_init(b + i);
    {
        fmpz_t z0, H, one;

        fmpz_init(z0);
        fmpz_init_set_ui(H, 2);
        fmpz_init_set_ui(one, 1);
        for (i = 0; i < r; i++)
            ADF_CHECK(adf_fball_set_fmpz3(b + i, z0, H, one) == ADF_OK);      /* the ball 2 Zhat */
        fmpz_clear(z0);
        fmpz_clear(H);
        fmpz_clear(one);
    }
    adf_linsol_init(sol);
    adf_linsol_init(keep);
    /* a prior value to see that sol is untouched */
    {
        fmpz_mat_t A0, b0;
        fmpz_t N;

        fmpz_mat_init(A0, 1, 1);
        fmpz_mat_init(b0, 1, 1);
        fmpz_init_set_ui(N, 6);
        fmpz_set_ui(fmpz_mat_entry(A0, 0, 0), 2);
        ADF_CHECK(adf_linsolve_mod(sol, A0, b0, N) == ADF_OK);
        fmpz_clear(N);
        fmpz_mat_clear(A0);
        fmpz_mat_clear(b0);
    }
    adf_linsol_set(keep, sol);
    ADF_CHECK(adf_linsolve_fball(sol, A, b, r) == ADF_LIMIT && linsol_same(sol, keep));
    /* r wrong by one is DOMAIN, decided before LIMIT */
    ADF_CHECK(adf_linsolve_fball(sol, A, b, r - 1) == ADF_DOMAIN && linsol_same(sol, keep));
    for (i = 0; i < r; i++)
        adf_fball_clear(b + i);
    flint_free(b);
    fmpz_mat_clear(A);
    adf_linsol_clear(sol);
    adf_linsol_clear(keep);
}

/* ------------------------------------------------------------------------------------------------
   5. the vectors: every line of tests/ref/vectors/s1-slice2/fball.jsonl
   ------------------------------------------------------------------------------------------------ */

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

ADF_TEST(vectors_s1_slice2_every_line)
{
    jsonl_error_t err;
    jsonl_file * file;
    size_t i;
    adf_linsol_t sol;
    fmpz_mat_t A, G, E, V, x0, y, Abal;
    fmpz_t N, t, a, H, d, ra, rH, rd;
    unsigned long n_ok = 0, n_none = 0, bad = 0, n_big = 0, n_coord = 0;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/s1-slice2/fball.jsonl", &file, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (file == NULL)
        return;
    adf_linsol_init(sol);
    fmpz_mat_init(A, 0, 0);
    fmpz_mat_init(G, 0, 0);
    fmpz_mat_init(E, 0, 0);
    fmpz_mat_init(V, 0, 0);
    fmpz_mat_init(x0, 0, 0);
    fmpz_mat_init(y, 0, 0);
    fmpz_mat_init(Abal, 0, 0);
    fmpz_init(N);
    fmpz_init(t);
    fmpz_init(a);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(ra);
    fmpz_init(rH);
    fmpz_init(rd);
    for (i = 0; i < jsonl_count(file); i++)
    {
        const jsonl_value * rec = jsonl_record(file, i);
        const jsonl_value * balls_j;
        const char * st_name;
        size_t len;
        slong r, c, k;
        int st, want, ok = 1;
        adf_fball_struct * balls;

        ok = ok && read_fmpz(t, field(rec, "r"));
        r = fmpz_get_si(t);
        ok = ok && read_fmpz(t, field(rec, "c"));
        c = fmpz_get_si(t);
        ok = ok && read_fmpz(N, field(rec, "N"));
        ok = ok && read_rows(A, field(rec, "A"), c) && A->r == r;
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
        balls_j = field(rec, "balls");
        balls = flint_malloc((size_t) (r + 1) * sizeof(adf_fball_struct));
        for (k = 0; k < r; k++)
        {
            const jsonl_value * tr = jsonl_at(balls_j, (size_t) k, &err);

            adf_fball_init(balls + k);
            ok = ok && read_fmpz(a, jsonl_at(tr, 0, &err)) && read_fmpz(H, jsonl_at(tr, 1, &err))
                 && read_fmpz(d, jsonl_at(tr, 2, &err));
            ok = ok && adf_fball_set_fmpz3(balls + k, a, H, d) == ADF_OK;
            /* the canonical triple of the vector is the canonical triple of the ball */
            adf_fball_get_fmpz3(ra, rH, rd, balls + k);
            ok = ok && fmpz_equal(ra, a) && fmpz_equal(rH, H) && fmpz_equal(rd, d);
        }
        st = adf_linsolve_fball(sol, A, balls, r);
        ok = ok && st == want && fmpz_equal(sol->N, N) && sol->r == r && sol->c == c;
        ok = ok && mat_same(sol->G, G) && mat_same(sol->E, E) && mat_same(sol->V, V) && mat_same(sol->x0, x0)
             && mat_same(sol->y, y);
        ok = ok && adf_linsol_verify_fball(sol, A, balls, r) == 1 && adf_linsol_is_canonical(sol);
        if (want == ADF_OK)
        {
            const jsonl_value * xb = field(rec, "xball");
            const jsonl_value * rho = field(rec, "rho");
            adf_fball_t ball;

            adf_fball_init(ball);
            ok = ok && (slong) jsonl_size(xb) == c && (slong) jsonl_size(rho) == c;
            for (k = 0; ok && k < c; k++)
            {
                const jsonl_value * tr = jsonl_at(xb, (size_t) k, &err);

                ok = ok && adf_linsol_get_fball(ball, sol, k) == ADF_OK;
                ok = ok && read_fmpz(a, jsonl_at(tr, 0, &err)) && read_fmpz(H, jsonl_at(tr, 1, &err))
                     && read_fmpz(d, jsonl_at(tr, 2, &err));
                adf_fball_get_fmpz3(ra, rH, rd, ball);
                ok = ok && fmpz_equal(ra, a) && fmpz_equal(rH, H) && fmpz_equal(rd, d);
                ok = ok && read_fmpz(t, jsonl_at(rho, (size_t) k, &err)) && fmpz_equal(t, H);
                n_coord++;
            }
            adf_fball_clear(ball);
        }
        ADF_CHECK_MSG(ok, "line %lu: status %d, want %d, or a matrix or ball differs", (unsigned long) (i + 1), st,
                      want);
        if (!ok)
            bad++;
        if (want == ADF_OK)
            n_ok++;
        else
            n_none++;
        if (fmpz_bits(N) > 64)
            n_big++;
        for (k = 0; k < r; k++)
            adf_fball_clear(balls + k);
        flint_free(balls);
    }
    printf("vectors: %lu lines, OK %lu, NO_SOLUTION %lu, N above 64 bits %lu, coordinate balls %lu; %lu differ\n",
           (unsigned long) jsonl_count(file), n_ok, n_none, n_big, n_coord, bad);
    ADF_CHECK(jsonl_count(file) > 700 && n_ok > 200 && n_none > 200 && n_big > 50 && n_coord > 500);
    fmpz_mat_clear(A);
    fmpz_mat_clear(G);
    fmpz_mat_clear(E);
    fmpz_mat_clear(V);
    fmpz_mat_clear(x0);
    fmpz_mat_clear(y);
    fmpz_mat_clear(Abal);
    fmpz_clear(N);
    fmpz_clear(t);
    fmpz_clear(a);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(ra);
    fmpz_clear(rH);
    fmpz_clear(rd);
    adf_linsol_clear(sol);
    jsonl_close(file);
}
