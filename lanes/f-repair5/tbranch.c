/* f-repair5: wall time of adf_lball_roots for all branches at p = 2^64 - 59, and the identity of every
   listed branch with adf_lball_root_seed at its seed (every branch when CHECK=all, else a sample of 64).
   Usage: tbranch n w N [ballN]   (x = w exact, or the ball w + p^ballN Z_p when ballN > 0). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <adelefeld.h>
#include <flint/ulong_extras.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
int main(int argc, char **argv)
{
    const ulong p = UWORD(18446744073709551557);
    ulong n = strtoul(argv[1], 0, 10), w = strtoul(argv[2], 0, 10);
    slong N = strtol(argv[3], 0, 10), bN = argc > 4 ? strtol(argv[4], 0, 10) : 0;
    ulong d = n_gcd(n, p - 1);
    int all = getenv("CHECK") && !strcmp(getenv("CHECK"), "all");
    adf_lball_t x, y;
    adf_lball_ptr rs = flint_malloc(d * sizeof(adf_lball_struct));
    ulong *ids = flint_malloc(d * sizeof(ulong));
    slong len = -1, bad = 0, checked = 0;
    adf_lball_init(x); adf_lball_init(y);
    for (ulong i = 0; i < d; i++) adf_lball_init(rs + i);
    x->p = p; fmpq_set_ui(x->u, w, 1); x->v = 0; x->N = bN > 0 ? bN : 0; x->exact = bN > 0 ? 0 : 1;
    double t0 = now();
    int st = adf_lball_roots(rs, ids, &len, (slong) d, x, n, N);
    double t1 = now();
    for (slong i = 0; st == ADF_OK && i < len; i += (all || len < 64) ? 1 : len / 64)
    {
        checked++;
        if (adf_lball_root_seed(y, x, n, ids[i], N) != ADF_OK || !adf_lball_identical(y, rs + i)) bad++;
    }
    printf("n=%lu d=%lu N=%ld st=%s len=%ld roots %.4f s (%.2f us/branch); seeded identity %ld checked, %ld bad\n",
           n, d, (long) N, adf_status_str(st), (long) len, t1 - t0, 1e6 * (t1 - t0) / (double) d, checked, bad);
    for (ulong i = 0; i < d; i++) adf_lball_clear(rs + i);
    flint_free(rs); flint_free(ids); adf_lball_clear(x); adf_lball_clear(y);
    return bad != 0;
}
