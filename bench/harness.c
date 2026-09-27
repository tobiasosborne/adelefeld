/* bench/harness.c: shared measurement harness for the adelefeld benchmarks.
   Contract: docs/PERF.md section 7. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

const char *bench_kind_name(bench_kind kind)
{
    switch (kind) {
    case BENCH_LATENCY_CHAIN:     return "latency_chain";
    case BENCH_INDEPENDENT_BATCH: return "independent_batch";
    case BENCH_CALL_RATE:         return "call_rate";
    }
    return "unknown";
}

void bench_stats_compute(const double *values, size_t n, bench_stats *out)
{
    double v[BENCH_MAX_TRIALS];
    size_t i, j;

    if (n == 0) {
        out->min = out->median = out->max = 0.0;
        return;
    }
    if (n > BENCH_MAX_TRIALS)
        n = BENCH_MAX_TRIALS;
    for (i = 0; i < n; i++)
        v[i] = values[i];
    /* insertion sort: n is at most 64 */
    for (i = 1; i < n; i++) {
        double x = v[i];
        j = i;
        while (j > 0 && v[j - 1] > x) {
            v[j] = v[j - 1];
            j--;
        }
        v[j] = x;
    }
    out->min = v[0];
    out->median = v[n / 2];
    out->max = v[n - 1];
}

double bench_now_raw(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return (double) t.tv_sec + 1e-9 * (double) t.tv_nsec;
}

int bench_tsc_present(void)
{
#if defined(__x86_64__) || defined(__i386__)
    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        return 0;
    return (edx >> 4) & 1;
#else
    return 0;
#endif
}

int bench_tsc_invariant(void)
{
#if defined(__x86_64__) || defined(__i386__)
    unsigned int eax, ebx, ecx, edx;
    unsigned int maxext = __get_cpuid_max(0x80000000, NULL);
    if (maxext < 0x80000007)
        return 0;
    __cpuid(0x80000007, eax, ebx, ecx, edx);
    return (edx >> 8) & 1;
#else
    return 0;
#endif
}

uint64_t bench_rdtsc(void)
{
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi, aux;
    __asm__ volatile("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux) : : "memory");
    return ((uint64_t) hi << 32) | (uint64_t) lo;
#else
    return 0;
#endif
}

void bench_tsc_calibrate(bench_tsc_calib *out, int trials,
                         double seconds_per_trial)
{
    double ratio[BENCH_MAX_TRIALS];
    bench_stats st;
    int t;

    if (trials > BENCH_MAX_TRIALS)
        trials = BENCH_MAX_TRIALS;
    if (trials < 1)
        trials = 1;
    for (t = 0; t < trials; t++) {
        double t0, t1, elapsed;
        uint64_t c0, c1;
        __asm__ volatile("" ::: "memory");
        t0 = bench_now_raw();
        c0 = bench_rdtsc();
        do {
            elapsed = bench_now_raw() - t0;
        } while (elapsed < seconds_per_trial);
        c1 = bench_rdtsc();
        t1 = bench_now_raw();
        __asm__ volatile("" ::: "memory");
        ratio[t] = (double) (c1 - c0) / ((t1 - t0) * 1e9);
    }
    bench_stats_compute(ratio, (size_t) trials, &st);
    out->trials = trials;
    out->cycles_per_ns_min = st.min;
    out->cycles_per_ns_median = st.median;
    out->cycles_per_ns_max = st.max;
}

int bench_pin_cpu(int cpu)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0)
        return -1;
    return sched_getcpu();
}

int bench_get_cpu(void)
{
    return sched_getcpu();
}

bench_stats bench_measure(bench_fn fn, void *ctx,
                          unsigned long long calls,
                          unsigned long long ops_per_call,
                          int trials, int warmup)
{
    double v[BENCH_MAX_TRIALS];
    bench_stats st;
    double ops;
    int t, w;

    if (trials > BENCH_MAX_TRIALS)
        trials = BENCH_MAX_TRIALS;
    if (trials < 1)
        trials = 1;
    ops = (double) calls * (double) ops_per_call;
    for (w = 0; w < warmup; w++)
        fn(ctx, calls);
    for (t = 0; t < trials; t++) {
        double t0, t1;
        __asm__ volatile("" ::: "memory");
        t0 = bench_now_raw();
        fn(ctx, calls);
        t1 = bench_now_raw();
        __asm__ volatile("" ::: "memory");
        v[t] = (t1 - t0) / ops * 1e9;
    }
    bench_stats_compute(v, (size_t) trials, &st);
    return st;
}

static int make_dir(const char *path)
{
    if (mkdir(path, 0777) == 0)
        return 0;
    return errno == EEXIST ? 0 : -1;
}

FILE *bench_results_open(const char *tag, char *run_id_out,
                         size_t run_id_size)
{
    char path[512];
    time_t now;
    struct tm tmv;
    FILE *f;

    if (make_dir("bench") != 0)
        return NULL;
    if (make_dir("bench/results") != 0)
        return NULL;
    now = time(NULL);
    gmtime_r(&now, &tmv);
    snprintf(run_id_out, run_id_size, "%04d-%02d-%02dT%02d%02d%02dZ",
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    snprintf(path, sizeof(path), "bench/results/%s_%s.txt", run_id_out, tag);
    f = fopen(path, "w");
    if (f != NULL)
        printf("result file: %s\n", path);
    return f;
}

const char *bench_cpu_model(char *buf, size_t size)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    char line[BENCH_STR];

    if (size == 0)
        return "unknown";
    snprintf(buf, size, "unknown");
    if (f == NULL)
        return buf;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "model name", 10) == 0) {
            char *colon = strchr(line, ':');
            if (colon != NULL) {
                size_t len;
                colon++;
                while (*colon == ' ' || *colon == '\t')
                    colon++;
                len = strlen(colon);
                while (len > 0 && (colon[len - 1] == '\n' ||
                                   colon[len - 1] == '\r'))
                    colon[--len] = '\0';
                snprintf(buf, size, "%s", colon);
            }
            break;
        }
    }
    fclose(f);
    return buf;
}
