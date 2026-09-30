/* bench.c (lane f-review4): the cost of the old route (commit 1cf0e42, symbols old_lball_*) against the new
   one on the same input. Input line: name p unit_num unit_den v M exact N reps mask
   (mask bit 0: time old; bit 1: time new). Calls alternate old/new; the minimum and the median of `reps`
   calls are printed in seconds, then the ratio of medians new/old, then the maximum resident set size (KiB)
   after the calls (getrusage). Build: bench.sh. */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

int old_lball_log(adf_lball_t, const adf_lball_t, slong);
int old_lball_Log(adf_lball_t, const adf_lball_t, slong);
typedef int (*fn_t)(adf_lball_t, const adf_lball_t, slong);

static double seconds(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static int cmp(const void * a, const void * b)
{
    double x = *(const double *) a, y = *(const double *) b;
    return x < y ? -1 : x > y;
}

int main(void)
{
    static char name[8], un[4000000], ud[4000000];
    ulong p;
    slong v, M, N, reps;
    int exact, mask;
    adf_lball_t x, y;
    adf_lball_init(x); adf_lball_init(y);
    while (scanf("%7s %lu %3999999s %3999999s %ld %ld %d %ld %ld %d", name, &p, un, ud, &v, &M, &exact, &N, &reps, &mask) == 10)
    {
        fn_t fnew = strcmp(name, "log") == 0 ? adf_lball_log : adf_lball_Log;
        fn_t fold = strcmp(name, "log") == 0 ? old_lball_log : old_lball_Log;
        double * to = malloc(sizeof(double) * (size_t) reps), * tn = malloc(sizeof(double) * (size_t) reps);
        slong i;
        struct rusage ru;
        int st = 0;
        x->p = p; x->v = v; x->N = M; x->exact = exact;
        if (fmpz_set_str(fmpq_numref(x->u), un, 10) || fmpz_set_str(fmpq_denref(x->u), ud, 10) || !adf_lball_is_canonical(x))
            return 2;
        for (i = 0; i < reps; i++)
        {
            double t0;
            if (mask & 1) { t0 = seconds(); st = fold(y, x, N); to[i] = seconds() - t0; } else to[i] = -1;
            if (mask & 2) { t0 = seconds(); st = fnew(y, x, N); tn[i] = seconds() - t0; } else tn[i] = -1;
        }
        qsort(to, (size_t) reps, sizeof(double), cmp);
        qsort(tn, (size_t) reps, sizeof(double), cmp);
        getrusage(RUSAGE_SELF, &ru);
        printf("%s p=%lu N=%ld K=%ld st=%d old_min=%.9f old_med=%.9f new_min=%.9f new_med=%.9f ratio=%.3f maxrss_kib=%ld\n",
               name, p, (long) N, (long) (st == ADF_OK ? y->N : -1), st, to[0], to[reps / 2], tn[0], tn[reps / 2],
               (mask & 1) && (mask & 2) ? tn[reps / 2] / to[reps / 2] : -1.0, ru.ru_maxrss);
        fflush(stdout);
        free(to); free(tn);
    }
    adf_lball_clear(x); adf_lball_clear(y);
    flint_cleanup();
    return 0;
}
