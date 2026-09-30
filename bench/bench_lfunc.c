#define _GNU_SOURCE
#include <adelefeld.h>
#include <flint/padic.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "-O2 -g"
#endif

/* Call rate, reused output, exact centres, absolute N; no context is cached by the library.
   One FLINT thread. CLOCK_MONOTONIC_RAW. Seed 0 (fixed operands).
   padic APIs: refs/src/flint-3.0.1/padic.rst:410-425,494-507; Log uses F3's powered unit.
   Integer modular power: refs/src/flint-3.0.1/fmpz.rst:923-930. */
static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return t.tv_sec + 1e-9*t.tv_nsec;
}
static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
int main(int argc, char **argv)
{
    adf_lball_t x, y;
    fmpz_t pp, P, a, b, r, inv;
    padic_t px, py;
    padic_ctx_t ctx;
    double times[31], mul[31], ref[31], start;
    int cpu = getenv("ADF_BENCH_CPU") ? atoi(getenv("ADF_BENCH_CPU")) : 2;
    ulong checksum = 0;
    int i, st = 0, trials = argc > 4 ? atoi(argv[4]) : 3;
    const char *fn = argc > 1 && strcmp(argv[1], "--run") ? argv[1] : "log";
    ulong p = argc > 2 ? strtoul(argv[2], NULL, 10) : 5;
    slong N = argc > 3 ? atol(argv[3]) : 2000;
    slong c = p == 2 ? 2 : 1, output_bytes;
    cpu_set_t cpus;
    CPU_ZERO(&cpus); CPU_SET(cpu, &cpus);
    i = sched_setaffinity(0, sizeof cpus, &cpus);
    flint_set_num_threads(1);
    if (trials < 1 || trials > 31 || N < 3) return 2;
    if (strcmp(fn,"exp") && strcmp(fn,"log") && strcmp(fn,"Log")) return 2;
    adf_lball_init(x); adf_lball_init(y);
    fmpz_init_set_ui(pp, p); fmpz_init(P); fmpz_init(a); fmpz_init(b); fmpz_init(r); fmpz_init(inv);
    fmpz_ui_pow_ui(P, p, (ulong) N);
    fmpz_sub_ui(r, P, 1); output_bytes = (fmpz_bits(r)+7)/8;
    x->p = p;
    fmpq_one(x->u);
    if (!strcmp(fn, "exp")) x->v = c;
    else if (!strcmp(fn, "log"))
    {
        fmpz_ui_pow_ui(fmpq_numref(x->u), p, (ulong)c);
        fmpz_add_ui(fmpq_numref(x->u), fmpq_numref(x->u), 1);
    }
    else fmpq_set_si(x->u, p == 2 ? 5 : 2, 1);
    padic_ctx_init(ctx, pp, 0, 0, PADIC_TERSE);
    padic_init2(px, N); padic_init2(py, N);
    fmpz_sub_ui(a, P, 17); fmpz_sub_ui(b, P, 33);
    printf("compiler=%s flags=%s flint=%s clock=MONOTONIC_RAW seed=0 cpu=%d pin_status=%d "
           "threads=1 kind=call_rate warm_output=1 p=%lu N=%ld bits=%ld trials=%d fn=%s\n",
           __VERSION__, BENCH_CFLAGS, FLINT_VERSION, sched_getcpu(), i, p, N, fmpz_bits(P), trials, fn);
    fflush(stdout);
    for (i = 0; i < trials; i++)
    {
        start = now();
        st = !strcmp(fn,"exp") ? adf_lball_exp(y,x,N)
             : !strcmp(fn,"log") ? adf_lball_log(y,x,N) : adf_lball_Log(y,x,N);
        times[i] = now()-start;
        if (st != ADF_OK) return 4;
        printf("trial=%d seconds=%.9f status=%d\n", i, times[i], st); fflush(stdout);
        start = now(); fmpz_mul(r,a,b); fmpz_mod(r,r,P); mul[i] = now()-start;
        checksum ^= fmpz_fdiv_ui(r,65521);
        start = now();
        if (!strcmp(fn,"exp"))
        {
            fmpz_ui_pow_ui(r,p,(ulong)c); padic_set_fmpz(px,r,ctx); st = padic_exp(py,px,ctx);
        }
        else
        {
            fmpz_set(r,fmpq_numref(x->u));
            if (!strcmp(fn,"Log") && p != 2) fmpz_powm_ui(r,r,p-1,P);
            padic_set_fmpz(px,r,ctx); st = padic_log(py,px,ctx);
            if (!strcmp(fn,"Log") && p != 2)
            {
                fmpz_set_ui(inv,p-1); fmpz_invmod(inv,inv,P);
                fmpz_mul(padic_unit(py),padic_unit(py),inv);
                padic_reduce(py,ctx);
            }
        }
        ref[i] = now()-start;
        if (!st) return 3;
        /* Consume and compare results outside all timed regions. */
        fmpz_ui_pow_ui(inv,p,(ulong)y->v); fmpz_mul(inv,inv,fmpq_numref(y->u));
        fmpz_mod(inv,inv,P); padic_get_fmpz(r,py,ctx);
        if (!fmpz_equal(inv,r)) return 5;
    }
    qsort(times,trials,sizeof(double),cmp); qsort(mul,trials,sizeof(double),cmp);
    qsort(ref,trials,sizeof(double),cmp);
    printf("seconds_min_med_max=%.9f,%.9f,%.9f mul_mod_min_med_max=%.9f,%.9f,%.9f "
           "flint_min_med_max=%.9f,%.9f,%.9f output_bytes=%ld centre_bits=%ld checksum=%lu mul_checksum=%lu\n",
           times[0],times[trials/2],times[trials-1],mul[0],mul[trials/2],mul[trials-1],
           ref[0],ref[trials/2],ref[trials-1],output_bytes,fmpz_bits(fmpq_numref(y->u)),
           fmpz_fdiv_ui(fmpq_numref(y->u),65521),checksum);
    padic_clear(px); padic_clear(py); padic_ctx_clear(ctx);
    fmpz_clear(pp); fmpz_clear(P); fmpz_clear(a); fmpz_clear(b); fmpz_clear(r); fmpz_clear(inv);
    adf_lball_clear(x); adf_lball_clear(y); flint_cleanup();
    return 0;
}
