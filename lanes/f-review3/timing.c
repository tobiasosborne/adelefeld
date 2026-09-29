#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* One call, one thread, compact output. Run under timeout.
   Threads API: refs/src/flint-3.0.1/flint.rst:172-178.
   Integer strings: refs/src/flint-3.0.1/fmpz.rst:427-431. */
int main(int argc, char **argv)
{
    adf_lball_t x, y;
    struct timespec before, after;
    ulong p;
    slong N;
    int st;
    double elapsed;
    if (argc != 5)
        return 2;
    flint_set_num_threads(1);
    p = strtoul(argv[2], NULL, 10);
    N = strtol(argv[3], NULL, 10);
    adf_lball_init(x); adf_lball_init(y);
    x->p = p;
    if (fmpz_set_str(fmpq_numref(x->u), argv[4], 10))
        return 2;
    if (!adf_lball_is_canonical(x))
        return 3;
    clock_gettime(CLOCK_MONOTONIC, &before);
    st = strcmp(argv[1], "log") == 0 ? adf_lball_log(y, x, N) : adf_lball_Log(y, x, N);
    clock_gettime(CLOCK_MONOTONIC, &after);
    elapsed = after.tv_sec-before.tv_sec + (after.tv_nsec-before.tv_nsec)*1e-9;
    printf("function=%s p=%lu unit=%s N=%ld status=%d exact=%d v=%ld K=%ld "
           "canonical=%d elapsed_seconds=%.9f threads=%d\n", argv[1], p, argv[4], N, st,
           y->exact, y->v, y->N, adf_lball_is_canonical(y), elapsed, flint_get_num_threads());
    adf_lball_clear(x); adf_lball_clear(y);
    flint_cleanup();
    return 0;
}
