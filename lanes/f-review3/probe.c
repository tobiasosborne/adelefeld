#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Stream: function p unit_n unit_d v M exact requested_N.
   Raw fields permit canonical inputs at the exponent limits without constructing p^v.
   FLINT integer strings: refs/src/flint-3.0.1/fmpz.rst:427-431.
   All calls link against this lane's archive. No padic API is used. */
static double seconds(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static int same(const adf_lball_t a, const adf_lball_t b)
{
    return a->p == b->p && a->v == b->v && a->N == b->N && a->exact == b->exact
        && fmpq_equal(a->u, b->u);
}

int main(void)
{
    char name[8], un[200000], ud[200000];
    ulong p;
    slong v, M, N;
    int exact, count = 0;
    adf_lball_t x, y, z, sentinel;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(sentinel);
    sentinel->p = 7; sentinel->v = -3;
    fmpq_set_si(sentinel->u, 17, 19);
    while (scanf("%7s %lu %199999s %199999s %ld %ld %d %ld", name, &p, un, ud, &v, &M, &exact, &N) == 8)
    {
        int (*fn)(adf_lball_t, const adf_lball_t, slong);
        int st, ast, canonical, unchanged, alias;
        double elapsed;
        fn = strcmp(name, "exp") == 0 ? adf_lball_exp
            : strcmp(name, "log") == 0 ? adf_lball_log : adf_lball_Log;
        x->p = p; x->v = v; x->N = M; x->exact = exact;
        if (fmpz_set_str(fmpq_numref(x->u), un, 10) || fmpz_set_str(fmpq_denref(x->u), ud, 10))
            return 2;
        if (!adf_lball_is_canonical(x))
        {
            fprintf(stderr, "noncanonical input %d\n", count);
            return 3;
        }
        adf_lball_set(y, sentinel); adf_lball_set(z, x);
        elapsed = seconds();
        st = fn(y, x, N);
        elapsed = seconds() - elapsed;
        ast = fn(z, z, N);
        alias = ast == st && same(z, st == ADF_OK ? y : x);
        unchanged = st == ADF_OK || same(y, sentinel);
        canonical = st != ADF_OK || adf_lball_is_canonical(y);
        printf("%d %d %ld %ld ", st, y->exact, y->v, y->N);
        fmpz_print(fmpq_numref(y->u)); putchar(' '); fmpz_print(fmpq_denref(y->u));
        printf(" %d %d %d %.9f\n", alias, unchanged, canonical, elapsed);
        fflush(stdout);
        count++;
    }
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(sentinel);
    flint_cleanup();
    return 0;
}
