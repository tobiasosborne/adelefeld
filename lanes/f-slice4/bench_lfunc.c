/* lanes/f-slice4/bench_lfunc.c: a timing row for exp, log and Log at a prime, against FLINT's padic at the same point
   and precision (a lane-local measurement, not part of bench/; the brief asks for no optimisation).

   Build and run from the repository root (the binary goes outside the lane directory):
     cc -std=c11 -O2 -Iinclude lanes/f-slice4/bench_lfunc.c build/libadelefeld.a -lflint -lgmp -lm -o <scratch>/bench
     timeout 300 <scratch>/bench
   Prints one line per case: the function, p, N, the mean time of the library and of FLINT in microseconds. */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>
#include <adelefeld.h>
#include <flint/padic.h>

static double
now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

int
main(void)
{
    struct { const char * f; ulong p; slong a; slong N; int reps; } cs[] = {
        {"exp", 3, 3, 100, 200}, {"exp", 3, 3, 1000, 10}, {"exp", 3, 3, 2000, 3},
        {"log", 3, 4, 100, 200}, {"log", 3, 4, 1000, 10}, {"log", 3, 4, 2000, 3},
        {"Log", 3, 2, 100, 200}, {"Log", 3, 2, 1000, 10},
        {"exp", 2, 4, 1000, 20}, {"log", 2, 5, 1000, 20},
        {"exp", 18446744073709551557UL, 0, 100, 20}, {"Log", 18446744073709551557UL, 2, 100, 20}};
    int i, r;
    for (i = 0; i < (int) (sizeof cs / sizeof cs[0]); i++)
    {
        adf_lball_t x, y;
        adf_place_t v;
        adf_rat_t q;
        padic_ctx_t ctx;
        padic_t X, Y, W;
        fmpz_t P;
        double t0, t1, t2;
        ulong p = cs[i].p;
        adf_place_prime(&v, p);
        adf_lball_init(x); adf_lball_init(y); adf_rat_init(q);
        if (cs[i].a == 0)
            fmpq_set_ui(q->q, p, 1);                      /* exp(p) */
        else
            fmpq_set_si(q->q, cs[i].a, 1);
        adf_lball_set_rat(x, v, q);
        fmpz_init_set_ui(P, p);
        padic_ctx_init(ctx, P, 0, 0, PADIC_SERIES);
        padic_init2(X, cs[i].N + 20); padic_init2(Y, cs[i].N); padic_init2(W, cs[i].N + 20);
        padic_set_fmpq(X, q->q, ctx);
        t0 = now();
        for (r = 0; r < cs[i].reps; r++)
        {
            if (cs[i].f[0] == 'e')
                adf_lball_exp(y, x, cs[i].N);
            else if (cs[i].f[0] == 'l')
                adf_lball_log(y, x, cs[i].N);
            else
                adf_lball_Log(y, x, cs[i].N);
        }
        t1 = now();
        for (r = 0; r < cs[i].reps; r++)
        {
            if (cs[i].f[0] == 'e')
                padic_exp(Y, X, ctx);
            else if (cs[i].f[0] == 'l')
                padic_log(Y, X, ctx);
            else
            {
                padic_teichmuller(W, X, ctx);
                padic_div(W, X, W, ctx);
                padic_log(Y, W, ctx);
            }
        }
        t2 = now();
        printf("%s p=%lu N=%ld: adelefeld %.1f us, FLINT padic %.1f us\n", cs[i].f, p, (long) cs[i].N,
               1e6 * (t1 - t0) / cs[i].reps, 1e6 * (t2 - t1) / cs[i].reps);
        padic_clear(X); padic_clear(Y); padic_clear(W); padic_ctx_clear(ctx); fmpz_clear(P);
        adf_lball_clear(x); adf_lball_clear(y); adf_rat_clear(q);
    }
    flint_cleanup();
    return 0;
}
