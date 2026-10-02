/* Old-code output only, with the sentinel and alias contract of the f-slice5 fixture.
   build.sh renames all three public symbols from src/lfunc.c at commit 1cf0e42. */
#include <adelefeld.h>
#include <stdio.h>
#include <string.h>

int old_lball_exp(adf_lball_t, const adf_lball_t, slong);
int old_lball_log(adf_lball_t, const adf_lball_t, slong);
int old_lball_Log(adf_lball_t, const adf_lball_t, slong);

static int same(const adf_lball_t a, const adf_lball_t b)
{
    return a->p == b->p && a->v == b->v && a->N == b->N && a->exact == b->exact && fmpq_equal(a->u, b->u);
}

int main(int argc, char **argv)
{
    static char name[8], un[400000], ud[400000];
    ulong p;
    slong v, M, N;
    int ex, rc;
    adf_lball_t x, y, z, sentinel;
    if (argc == 2 && strcmp(argv[1], "--bits") == 0)
    {
        printf("%d\n", FLINT_BITS);
        return 0;
    }
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(sentinel);
    sentinel->p = 7; sentinel->v = -3; sentinel->N = 0; sentinel->exact = 1;
    fmpq_set_si(sentinel->u, 17, 19);
    while ((rc = scanf("%7s %lu %399999s %399999s %ld %ld %d %ld",
                       name, &p, un, ud, &v, &M, &ex, &N)) == 8)
    {
        int st, ast;
        int (*fn)(adf_lball_t, const adf_lball_t, slong);
        if (strcmp(name, "exp") == 0) fn = old_lball_exp;
        else if (strcmp(name, "log") == 0) fn = old_lball_log;
        else if (strcmp(name, "Log") == 0) fn = old_lball_Log;
        else return 2;
        x->p = p; x->v = v; x->N = M; x->exact = ex;
        if (fmpz_set_str(fmpq_numref(x->u), un, 10) || fmpz_set_str(fmpq_denref(x->u), ud, 10))
            return 3;
        if (!adf_lball_is_canonical(x)) return 4;
        adf_lball_set(y, sentinel); adf_lball_set(z, x);
        st = fn(y, x, N); ast = fn(z, z, N);
        printf("%d %d %ld %ld ", st, y->exact, (long)y->v, (long)y->N);
        fmpz_print(fmpq_numref(y->u)); putchar(' '); fmpz_print(fmpq_denref(y->u));
        printf(" %d %d %d\n", ast == st && same(z, st == ADF_OK ? y : x),
               st == ADF_OK || same(y, sentinel), adf_lball_is_canonical(y));
        fflush(stdout);
    }
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(sentinel);
    flint_cleanup();
    return rc == EOF ? 0 : 5;
}
