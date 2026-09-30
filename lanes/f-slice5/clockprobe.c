/* clockprobe.c: estimate the effective core clock of one pinned cpu from a chain of dependent
 * `add r64, r64` instructions. Assumption: the latency of `add r64, r64` is 1 cycle
 * (uops.info, Alder Lake-P: refs/src/uops-intel/ADD_01_R64_R64.html:224-225, a model input for the
 * Raptor Lake P-core of this laptop). Then adds per ns = cycles per ns. Lane m0-amend, 2026-09-28.
 * Build: gcc -O2 -o clockprobe clockprobe.c ; run: ./clockprobe CPU */
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return 1e9 * (double) t.tv_sec + (double) t.tv_nsec;
}

#define A10 "add %1, %0\n\tadd %1, %0\n\tadd %1, %0\n\tadd %1, %0\n\tadd %1, %0\n\t" \
            "add %1, %0\n\tadd %1, %0\n\tadd %1, %0\n\tadd %1, %0\n\tadd %1, %0\n\t"

static int cmp(const void *a, const void *b)
{
    double x = *(const double *) a, y = *(const double *) b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    int cpu = argc > 1 ? atoi(argv[1]) : 2, t, trials = 9;
    unsigned long long iters = 2000000ULL, i, x = 1, y = 3;  /* 100 adds per iteration */
    double g[9];
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof set, &set) != 0) { perror("sched_setaffinity"); return 1; }
    for (t = -1; t < trials; t++) {           /* t = -1: warm-up, not recorded */
        double t0 = now_ns(), t1;
        for (i = 0; i < iters; i++)
            __asm__ volatile (A10 A10 A10 A10 A10 A10 A10 A10 A10 A10 : "+r"(x) : "r"(y));
        t1 = now_ns();
        if (t >= 0) g[t] = (double) (iters * 100ULL) / (t1 - t0);
    }
    qsort(g, trials, sizeof g[0], cmp);
    printf("cpu %d adds_per_ns (GHz under the 1-cycle assumption): min %.3f median %.3f max %.3f "
           "checksum %llu\n", sched_getcpu(), g[0], g[trials / 2], g[trials - 1], x);
    return 0;
}
