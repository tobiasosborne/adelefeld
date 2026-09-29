/* bench/bench_modctx.c: benchmark rows of work package 1.8 (docs/PLAN.md section 6) for the two
   conversions of docs/PERF.md section 4: "Global integer to k residues" (adf_modctx_reduce) and
   "k residues to the global integer" (adf_modctx_combine), at n = 4096 bits and k = 128 blocks.

   Contract: docs/PERF.md section 7. The rows are independent batches (the conversions have no
   chain: each call reads one integer or one residue vector and writes its conversion). The
   operand family is the one of docs/PERF.md section 4: k blocks below 2^32 (here 128 distinct
   primes just below 2^32, pairwise coprime, of product K of about 4096 bits) and integers
   A of about n = 4096 bits. Each call allocates its scratch inside (conventions 4.2), so the
   rows are warm and allocating.

   Floor of docs/PERF.md section 4 (MODEL, vector I/O, worst case over dense inputs):
   max(b/64, 4k/32) cycles to residues and max(4k/64, b/32) cycles back, b = ceil(n/8) = 512
   bytes, k = 128: max(8, 16) = 16 and max(8, 16) = 16 cycles, the example of that section.
   The numbers of this file are provisional (quiet_machine: no).

   Build and run:
   make -C bench run (the rows appear in bench/results/<utc>_modctx.txt). */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_vec.h>
#include <flint/ulong_extras.h>

#include <adelefeld/modctx.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

/* The conversions are exported by src/modctx.c for the local backend (work package 1.8); no
   public header declares them (HEADER-FINDING 2 of lane m1-modctx). */
void adf_modctx_reduce(ulong * res, const fmpz_t A, const adf_modctx_struct * ctx);
void adf_modctx_combine(fmpz_t A, const ulong * res, const adf_modctx_struct * ctx);

#define MC_BLOCKS   128
#define MC_BATCH_N   64
#define MC_CALLS    400        /* passes over the batch per timed sample */

typedef struct
{
    const adf_modctx_struct * ctx;
    fmpz * a;                  /* MC_BATCH_N integers of about 4096 bits */
    fmpz * out;                /* MC_BATCH_N outputs of the recombination */
    ulong * res;               /* MC_BATCH_N * MC_BLOCKS residues */
    ulong * scratch;           /* MC_BLOCKS words of one reduction */
    ulong acc;                 /* read only after the timed region */
} modctx_ctx;
/* --------------------------------------------------------------- kernels */

static __attribute__((noinline)) void
modctx_reduce_batch_run(void * v, unsigned long long passes)
{
    modctx_ctx * c = (modctx_ctx *) v;
    unsigned long long p;
    ulong i;

    for (p = 0; p < passes; p++)
        for (i = 0; i < MC_BATCH_N; i++)
            adf_modctx_reduce(c->scratch, c->a + i, c->ctx);
    c->acc = c->scratch[0] + c->scratch[MC_BLOCKS - 1];
}

static __attribute__((noinline)) void
modctx_combine_batch_run(void * v, unsigned long long passes)
{
    modctx_ctx * c = (modctx_ctx *) v;
    unsigned long long p;
    ulong i;

    for (p = 0; p < passes; p++)
        for (i = 0; i < MC_BATCH_N; i++)
            adf_modctx_combine(c->out + i, c->res + i * MC_BLOCKS, c->ctx);
    c->acc = (ulong) fmpz_bits(c->out + (MC_BATCH_N - 1));
}

/* -------------------------------------------------------------- the cases */

typedef struct
{
    const char * name;
    bench_kind kind;
    const char * operand_family;
    char bit_lengths[160];
    bench_fn fn;
    unsigned long long calls;
    unsigned long long ops_per_call;
    modctx_ctx ctx;
} modctx_case;

static unsigned long long g_checksum = 0;
static unsigned long long g_seed = 0;
static ulong g_kbits = 0;

/* The context: MC_BLOCKS pairwise coprime word blocks below 2^32 (distinct primes just below
   2^32), of product K of about 4096 bits; then the batch. Returns 0 on failure. */
static int
fill_ctx(modctx_ctx * c, flint_rand_t state)
{
    ulong q[MC_BLOCKS];
    ulong p = (UWORD(1) << 32) - 5;
    ulong i, j;
    adf_modctx_struct * ctx = NULL;
    fmpz_t K;

    memset(c, 0, sizeof(*c));
    for (i = 0; i < MC_BLOCKS; i++)
    {
        while (!n_is_prime(p))
            p -= 2;
        q[i] = p;
        p -= 2;
    }
    for (i = 0; i < MC_BLOCKS; i++)
        for (j = 0; j < i; j++)
            if (n_gcd(q[i], q[j]) != 1)
                return 0;
    if (adf_modctx_new_blocks(&ctx, q, MC_BLOCKS) != ADF_OK)
        return 0;
    c->ctx = ctx;

    fmpz_init(K);
    adf_modctx_get_modulus(K, ctx);
    g_kbits = (ulong) fmpz_bits(K);

    c->a = _fmpz_vec_init(MC_BATCH_N);
    c->out = _fmpz_vec_init(MC_BATCH_N);
    c->res = malloc(MC_BATCH_N * MC_BLOCKS * sizeof(ulong));
    c->scratch = malloc(MC_BLOCKS * sizeof(ulong));
    for (i = 0; i < MC_BATCH_N; i++)
    {
        fmpz_randm(c->a + i, state, K);
        adf_modctx_reduce(c->res + i * MC_BLOCKS, c->a + i, ctx);
    }
    fmpz_clear(K);
    return 1;
}

static void
free_ctx(modctx_ctx * c)
{
    _fmpz_vec_clear(c->a, MC_BATCH_N);
    _fmpz_vec_clear(c->out, MC_BATCH_N);
    free(c->res);
    free(c->scratch);
    adf_modctx_free((adf_modctx_struct *) c->ctx);
}

/* ----------------------------------------------------------------- driver */

static int
count_cores(void)
{
    FILE * f = fopen("/sys/devices/system/cpu/online", "r");
    char line[128];
    int cores = 1;
    long lo, hi;

    if (f == NULL)
        return 0;
    if (fgets(line, sizeof(line), f) != NULL)
    {
        char * end = strchr(line, '\n');
        if (end != NULL)
            *end = '\0';
        if (sscanf(line, "%ld-%ld", &lo, &hi) == 2)
            cores = (int) (hi - lo + 1);
    }
    fclose(f);
    return cores;
}

static void
report_case(FILE * f, modctx_case * wc, int trials, const bench_tsc_calib * cal)
{
    bench_stats st = bench_measure(wc->fn, &wc->ctx, wc->calls, wc->ops_per_call,
                                   trials, 1);
    unsigned long long ops = wc->calls * wc->ops_per_call;
    double cyc_min = st.min * cal->cycles_per_ns_median;
    double cyc_med = st.median * cal->cycles_per_ns_median;

    g_checksum ^= (unsigned long long) wc->ctx.acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n",
            wc->name, bench_kind_name(wc->kind), wc->operand_family, wc->bit_lengths,
            g_seed, "yes", "yes", wc->calls, ops, (unsigned long long) wc->ctx.acc,
            st.min, st.median, st.max);
    fprintf(f, "# %s: %.2f cycles/op min, %.2f cycles/op median at %.5f cycles/ns "
               "(tsc median); PERF.md section 4 I/O floor 16 cycles\n",
            wc->name, cyc_min, cyc_med, cal->cycles_per_ns_median);
    fflush(f);
    printf("%-24s %-17s min %10.4f  median %10.4f  max %10.4f ns/op  (%.1f / %.1f cyc)\n",
           wc->name, bench_kind_name(wc->kind), st.min, st.median, st.max, cyc_min, cyc_med);
}

static void
write_header(FILE * f, const char * run_id, const char * model, int requested_cpu,
             int actual_cpu, const bench_tsc_calib * cal, int trials, int cores)
{
    char gmp[64];
    snprintf(gmp, sizeof(gmp), "%d.%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
             __GNU_MP_VERSION_PATCHLEVEL);
    fprintf(f, "run_id: %s\n", run_id);
    fprintf(f, "tag: modctx\n");
    fprintf(f, "quiet_machine: no\n");
    fprintf(f, "provisional: yes (lane m1-modctx; compared with the I/O floor of "
               "docs/PERF.md section 4, n = 4096 bits, k = 128: 16 cycles both ways)\n");
    fprintf(f, "machine: %s\n", model);
    fprintf(f, "cores_online: %d\n", cores);
    fprintf(f, "affinity_requested_cpu: %d\n", requested_cpu);
    fprintf(f, "affinity_actual_cpu: %d\n", actual_cpu);
    fprintf(f, "affinity_method: sched_setaffinity\n");
    fprintf(f, "compiler: gcc %s\n", __VERSION__);
    fprintf(f, "flags: %s\n", BENCH_CFLAGS);
    fprintf(f, "flint_version: %s\n", FLINT_VERSION);
    fprintf(f, "gmp_version: %s\n", gmp);
    fprintf(f, "clock_source: clock_gettime(CLOCK_MONOTONIC_RAW)\n");
    fprintf(f, "tsc_present: %s\n", bench_tsc_present() ? "yes" : "no");
    fprintf(f, "tsc_invariant: %s\n", bench_tsc_invariant() ? "yes" : "no");
    fprintf(f, "cycles_per_ns_trials: %d\n", cal->trials);
    fprintf(f, "cycles_per_ns_min: %.5f\n", cal->cycles_per_ns_min);
    fprintf(f, "cycles_per_ns_median: %.5f\n", cal->cycles_per_ns_median);
    fprintf(f, "cycles_per_ns_max: %.5f\n", cal->cycles_per_ns_max);
    fprintf(f, "cycles_per_ns_spread: %.5f\n",
            cal->cycles_per_ns_max - cal->cycles_per_ns_min);
    fprintf(f, "seed: %llu\n", g_seed);
    fprintf(f, "operand_family: %d pairwise coprime word blocks (distinct primes just below "
               "2^32), product K of %lu bit; integers A uniform in [0, K)\n",
            MC_BLOCKS, (unsigned long) g_kbits);
    fprintf(f, "trials_per_case: %d\n", trials);
    fprintf(f, "batch_length: %d\n", MC_BATCH_N);
    fprintf(f, "warm: yes\n");
    fprintf(f, "allocating: yes (fmpz scratch inside each conversion, conventions 4.2)\n");
    fprintf(f, "\n");
    fprintf(f, "# one table follows; columns are tab separated\n");
    fprintf(f, "# name\tkind\toperand_family\tbit_lengths\tseed\twarm\t"
               "allocating\tcalls\tops\tchecksum\tns_per_op_min\t"
               "ns_per_op_median\tns_per_op_max\n");
}

static int
run(int cpu, unsigned long long seed, int trials)
{
    char run_id[64];
    char model[BENCH_STR];
    bench_tsc_calib cal;
    modctx_case cases[2];
    modctx_ctx shared;
    flint_rand_t state;
    unsigned long long checksum;
    int actual_cpu, nc = 0, i;
    FILE * f;

    flint_randinit(state);
    flint_randseed(state, (ulong) seed, (ulong) (seed >> 32));
    g_seed = seed;

    actual_cpu = bench_pin_cpu(cpu);
    if (actual_cpu < 0)
    {
        fprintf(stderr, "warning: sched_setaffinity(%d) failed; running unpinned\n", cpu);
        actual_cpu = bench_get_cpu();
    }
    printf("pinned to cpu %d (requested %d)\n", actual_cpu, cpu);
    bench_tsc_calibrate(&cal, 9, 0.02);

    if (!fill_ctx(&shared, state))
    {
        fprintf(stderr, "cannot build the context of %d blocks\n", MC_BLOCKS);
        return 1;
    }

    memset(&cases[nc], 0, sizeof(cases[nc]));
    cases[nc].name = "modctx_reduce_4096_k128";
    cases[nc].kind = BENCH_INDEPENDENT_BATCH;
    cases[nc].operand_family = "integer of ~4096 bits to 128 residues (blocks < 2^32)";
    snprintf(cases[nc].bit_lengths, sizeof(cases[nc].bit_lengths),
             "K %lu bit; blocks 32 bit; A uniform in [0, K); %d per batch",
             (unsigned long) g_kbits, MC_BATCH_N);
    cases[nc].fn = modctx_reduce_batch_run;
    cases[nc].calls = MC_CALLS;
    cases[nc].ops_per_call = MC_BATCH_N;
    cases[nc].ctx = shared;
    nc++;

    memset(&cases[nc], 0, sizeof(cases[nc]));
    cases[nc].name = "modctx_combine_4096_k128";
    cases[nc].kind = BENCH_INDEPENDENT_BATCH;
    cases[nc].operand_family = "128 residues (blocks < 2^32) to the integer of [0, K)";
    snprintf(cases[nc].bit_lengths, sizeof(cases[nc].bit_lengths),
             "K %lu bit; residues < 2^32; result in [0, K); %d per batch",
             (unsigned long) g_kbits, MC_BATCH_N);
    cases[nc].fn = modctx_combine_batch_run;
    cases[nc].calls = MC_CALLS;
    cases[nc].ops_per_call = MC_BATCH_N;
    cases[nc].ctx = shared;
    nc++;

    f = bench_results_open("modctx", run_id, sizeof(run_id));
    if (f == NULL)
    {
        fprintf(stderr, "cannot open result file\n");
        return 1;
    }
    bench_cpu_model(model, sizeof(model));
    write_header(f, run_id, model, cpu, actual_cpu, &cal, trials, count_cores());
    for (i = 0; i < nc; i++)
        report_case(f, &cases[i], trials, &cal);
    checksum = g_checksum;
    fprintf(f, "\n# checksum of all case results, computed outside the timed region\n");
    fprintf(f, "checksum: %llu\n", checksum);
    fclose(f);
    free_ctx(&shared);
    flint_randclear(state);
    printf("checksum %llu\n", checksum);
    return 0;
}

static void
usage(const char * argv0)
{
    printf("usage: %s [--run] [--seed N] [--trials N] [--cpu N]\n", argv0);
}

int
main(int argc, char ** argv)
{
    int i, cpu = 2, trials = 15;
    unsigned long long seed = 20260928ULL;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--run") == 0)
            ;
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
            seed = strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--trials") == 0 && i + 1 < argc)
            trials = atoi(argv[++i]);
        else if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc)
            cpu = atoi(argv[++i]);
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else
        {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }
    return run(cpu, seed, trials);
}
