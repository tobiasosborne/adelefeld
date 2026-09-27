/* bench/harness.h: shared measurement harness for the adelefeld benchmarks.
   Contract: docs/PERF.md section 7.  The word benchmarks live in bench_word.c. */
#ifndef ADELEFELD_BENCH_HARNESS_H
#define ADELEFELD_BENCH_HARNESS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define BENCH_MAX_TRIALS 64
#define BENCH_STR 192

/* The kind of measurement.  A value is compared with a floor only when the
   kinds match (docs/PERF.md, "The metric"). */
typedef enum {
    BENCH_LATENCY_CHAIN = 0,
    BENCH_INDEPENDENT_BATCH = 1,
    BENCH_CALL_RATE = 2
} bench_kind;

const char *bench_kind_name(bench_kind kind);

/* Order statistics over n values.  The input array is not modified.
   median is the sorted value at index n/2 (upper median for even n). */
typedef struct {
    double min;
    double median;
    double max;
} bench_stats;

void bench_stats_compute(const double *values, size_t n, bench_stats *out);

/* Wall clock.  bench_now_raw is clock_gettime(CLOCK_MONOTONIC_RAW) in seconds. */
double bench_now_raw(void);

/* Time-stamp counter.  Recorded only as a calibration; the timed region uses
   bench_now_raw. */
int bench_tsc_present(void);
int bench_tsc_invariant(void);
uint64_t bench_rdtsc(void);

typedef struct {
    int trials;
    double cycles_per_ns_min;
    double cycles_per_ns_median;
    double cycles_per_ns_max;
} bench_tsc_calib;

/* Measures TSC ticks per nanosecond over trials windows of about
   seconds_per_trial each.  Costs trials * seconds_per_trial seconds. */
void bench_tsc_calibrate(bench_tsc_calib *out, int trials,
                         double seconds_per_trial);

/* CPU affinity through sched_setaffinity(2).  bench_pin_cpu returns the cpu
   that is actually in use after the call, or -1. */
int bench_pin_cpu(int cpu);
int bench_get_cpu(void);

/* A timed kernel: fn(ctx, calls) performs calls * ops_per_call operations and
   leaves its result in *ctx, which the caller consumes after the timed region. */
typedef void (*bench_fn)(void *ctx, unsigned long long calls);

/* Runs warmup calls, then trials timed calls, and returns the per-operation
   time in nanoseconds.  The result is not used inside the timed region. */
bench_stats bench_measure(bench_fn fn, void *ctx,
                          unsigned long long calls,
                          unsigned long long ops_per_call,
                          int trials, int warmup);

/* Opens bench/results/<run_id>_<tag>.txt, creating bench/results if needed.
   run_id_out receives the run id (UTC, second resolution).  Returns NULL on
   failure.  The working directory is the repository root. */
FILE *bench_results_open(const char *tag, char *run_id_out,
                         size_t run_id_size);

/* Reads the CPU model name from /proc/cpuinfo.  Returns buf, or "unknown". */
const char *bench_cpu_model(char *buf, size_t size);

#endif
