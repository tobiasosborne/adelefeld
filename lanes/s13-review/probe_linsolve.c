#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>

static long gcdl(long a, long b)
{
    long t;
    if (a < 0) a = -a;
    while (b) { t = a % b; a = b; b = t; }
    return a;
}

static long lcm(long a, long b) { return a / gcdl(a, b) * b; }
static unsigned long rng = 712367;
static unsigned long next(void) { rng = 1664525UL * rng + 1013904223UL; return rng; }
static long draw(long n) { return (long)(next() % (unsigned long)n); }

static long allocs, callocs, reallocs;
static void *count_malloc(size_t n) { allocs++; return malloc(n); }
static void *count_calloc(size_t n, size_t k) { callocs++; return calloc(n, k); }
static void *count_realloc(void *p, size_t n) { reallocs++; return realloc(p, n); }
static void count_free(void *p) { free(p); }

int main(void)
{
    fmpz_mat_t A, xv, E, V;
    adf_fball_t balls[2], coord;
    adf_linsol_t sol, copy;
    adf_rat_t q;
    fmpz_t a, h, d, ord;
    long cases = 0, points = 0, coords = 0, oks = 0, empties = 0, mutated = 0;
    fmpz_mat_init(A, 2, 2); fmpz_mat_init(xv, 2, 1);
    fmpz_mat_init(E, 0, 0); fmpz_mat_init(V, 0, 0);
    adf_fball_init(balls[0]); adf_fball_init(balls[1]); adf_fball_init(coord);
    adf_linsol_init(sol); adf_linsol_init(copy); adf_rat_init(q);
    fmpz_init(a); fmpz_init(h); fmpz_init(d); fmpz_init(ord);
    for (long iter = 0; iter < 3000; iter++)
    {
        long mat[2][2], aa[2], hh[2], dd[2], N = 1, count = 0, kcount = 0;
        int marks[2][13] = {{0}};
        for (long i = 0; i < 2; i++)
        {
            for (long j = 0; j < 2; j++)
            {
                mat[i][j] = draw(9) - 4;
                fmpz_set_si(fmpz_mat_entry(A, i, j), mat[i][j]);
            }
            fmpz_set_si(a, draw(15) - 7);
            fmpz_set_si(h, 1 + draw(12));
            fmpz_set_si(d, (draw(2) ? 1 : -1) * (1 + draw(6)));
            if (adf_fball_set_fmpz3(balls[i], a, h, d) != ADF_OK) abort();
            adf_fball_get_fmpz3(a, h, d, balls[i]);
            aa[i] = fmpz_get_si(a); hh[i] = fmpz_get_si(h); dd[i] = fmpz_get_si(d);
            N = lcm(N, hh[i]);
        }
        if (N > 12) continue;
        int st = adf_linsolve_fball(sol, A, balls[0], 2);
        if (st != ADF_OK && st != ADF_NO_SOLUTION) abort();
        if (!adf_linsol_verify_fball(sol, A, balls[0], 2)) abort();
        if (fmpz_get_si(sol->N) != N) abort();
        adf_linsol_set(copy, sol);
        if (!adf_linsol_identical(copy, sol)) abort();
        adf_linsol_swap(copy, sol); adf_linsol_swap(copy, sol);
        if (!adf_linsol_identical(copy, sol)) abort();
        adf_linsol_get_image_cert(E, V, sol);
        if (!fmpz_mat_equal(E, sol->E) || !fmpz_mat_equal(V, sol->V)) abort();
        fmpz_add_ui(copy->N, copy->N, 1);
        if (adf_linsol_verify_fball(copy, A, balls[0], 2)) abort();
        mutated++;
        for (long x = 0; x < N; x++)
        for (long y = 0; y < N; y++)
        {
            long z[2] = {x, y};
            int want = 1, kern = 1;
            for (long i = 0; i < 2; i++)
            {
                long ax = mat[i][0]*x + mat[i][1]*y;
                if ((dd[i]*ax - aa[i]) % hh[i]) want = 0;
                if ((dd[i]*ax) % hh[i]) kern = 0;
            }
            if (want) { count++; marks[0][x] = marks[1][y] = 1; }
            if (kern) kcount++;
            fmpz_set_si(fmpz_mat_entry(xv, 0, 0), z[0]);
            fmpz_set_si(fmpz_mat_entry(xv, 1, 0), z[1]);
            if (adf_linsol_contains(sol, xv) != want) abort();
            points++;
        }
        adf_linsol_kernel_order(ord, sol);
        if (fmpz_get_si(ord) != kcount) abort();
        if ((st == ADF_OK) != (count > 0)) abort();
        if (st == ADF_OK) oks++; else empties++;
        for (long j = 0; j < 2; j++)
        {
            int cst = adf_linsol_get_fball(coord, sol, j);
            if (st == ADF_NO_SOLUTION)
            {
                if (cst != ADF_NO_SOLUTION) abort();
                continue;
            }
            if (cst != ADF_OK) abort();
            for (long v = 0; v < N; v++)
            {
                adf_rat_set_si(q, v);
                if (adf_fball_contains_rat(coord, q) != marks[j][v]) abort();
                coords++;
            }
        }
        cases++;
    }
    printf("oracle cases=%ld points=%ld coords=%ld OK=%ld NO_SOLUTION=%ld false-cert=%ld\n",
           cases, points, coords, oks, empties, mutated);
    fmpz_mat_clear(A); fmpz_mat_clear(xv); fmpz_mat_clear(E); fmpz_mat_clear(V);
    adf_fball_clear(balls[0]); adf_fball_clear(balls[1]); adf_fball_clear(coord);
    adf_linsol_clear(sol); adf_linsol_clear(copy); adf_rat_clear(q);
    fmpz_clear(a); fmpz_clear(h); fmpz_clear(d); fmpz_clear(ord);

    /* Exercise the size guard with an otherwise valid array of finite balls. */
    long rows = ADF_LINSOLVE_DIM_MAX + 1;
    adf_fball_struct *many = malloc((size_t)rows * sizeof(*many));
    if (!many) abort();
    fmpz_mat_init(A, rows, 0); adf_linsol_init(sol); adf_linsol_init(copy);
    fmpz_init(a); fmpz_init(h); fmpz_init(d);
    for (long i = 0; i < rows; i++) adf_fball_init(many + i);
    fmpz_set_si(a, 0); fmpz_set_si(h, 1); fmpz_set_si(d, 1);
    for (long i = 0; i < rows; i++)
        adf_fball_set_fmpz3(many + i, a, h, d);
    __flint_set_memory_functions(count_malloc, count_calloc, count_realloc, count_free);
    int st = adf_linsolve_fball(sol, A, many, rows);
    printf("limit status=%s allocations=%ld calloc=%ld realloc=%ld\n",
           adf_status_str(st), allocs, callocs, reallocs);
    if (st != ADF_LIMIT || !adf_linsol_identical(sol, copy)) abort();
    fmpz_zero(h); adf_fball_set_fmpz3(many, a, h, d);
    allocs = callocs = reallocs = 0;
    int ust = adf_linsolve_fball(sol, A, many, rows);
    printf("unsupported status=%s allocations=%ld calloc=%ld realloc=%ld\n",
           adf_status_str(ust), allocs, callocs, reallocs);
    if (ust != ADF_UNSUPPORTED || !adf_linsol_identical(sol, copy)) abort();
    fmpz_mat_t rhs;
    fmpz_mat_init(rhs, rows, 1); fmpz_one(h);
    allocs = callocs = reallocs = 0;
    int mst = adf_linsolve_mod(sol, A, rhs, h);
    printf("mod limit status=%s allocations=%ld calloc=%ld realloc=%ld\n",
           adf_status_str(mst), allocs, callocs, reallocs);
    if (mst != ADF_LIMIT || !adf_linsol_identical(sol, copy)) abort();
    fmpz_mat_clear(rhs);
    for (long i = 0; i < rows; i++) adf_fball_clear(many + i);
    free(many); fmpz_mat_clear(A); adf_linsol_clear(sol); adf_linsol_clear(copy);
    fmpz_clear(a); fmpz_clear(h); fmpz_clear(d);
    return st == ADF_LIMIT ? 0 : 1;
}
