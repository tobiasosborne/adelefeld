/* bench/bench_modctx.c: benchmark rows of the two local-backend conversions of work package
   1.8 (docs/PLAN.md section 6): the global integer to k residues and k residues to the global
   integer, for a 4096-bit modulus with k = 128 blocks.  Contract: docs/PERF.md section 7 and
   the rows of section 4.  Build and run: `make -C bench run` (the row appears in
   bench/results/<utc>_modctx.txt).

   The conversions are the internal kernels of src/modctx.c (docs/conventions.md 5.14;
   docs/proofs/policies.md Lemma 17).  They are linked into this binary from ../src/modctx.c,
   as bench_fball does with ../src/fball.c; the context is built outside the timed region, as
   the set-up of a context is a separate row amortised over the uses (PERF section 4). */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/ulong_extras.h>

#include <adelefeld/modctx.h>

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

/* The two internal kernels of src/modctx.c (hidden symbols, not public). */
void adf_modctx_reduce(const adf_modctx_struct * ctx, const fmpz_t a, ulong * res);
void adf_modctx_recombine(fmpz_t out, const adf_modctx_struct * ctx, const ulong * res);

#define MODCTX_BLOCKS   128
#define MODCTX_TARGET_BITS 4096
#define MODCTX_CALLS    20000ULL
#define MODCTX_CRT_CALLS 20000ULL

typedef struct
{
    const adf_modctx_struct * ctx;
    fmpz_t a;                 /* the integer reduced, < K */
    ulong res[MODCTX_BLOCKS]; /* its residues, fixed for the recombination row */
    ulong acc;                /* read only after the timed region */
} modctx_ctx;

static __attribute__((noinline)) void
modctx_reduce_run(void * v, unsigned long long calls)
{
    modctx_ctx * c = (modctx_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_modctx_reduce(c->ctx, c->a, c->res);
    c->acc = (ulong) (c->res[0] + c->res[MODCTX_BLOCKS - 1]);
}

static __attribute__((noinline)) void
modctx_recombine_run(void * v, unsigned long long calls)
{
    modctx_ctx * c = (modctx_ctx *) v;
    fmpz_t out;
    unsigned long long i;

    fmpz_init(out);
    for (i = 0; i < calls; i++)
        adf_modctx_recombine(out, c->ctx, c->res);
    c->acc = (ulong) fmpz_bits(out);
    fmpz_clear(out);
}

typedef struct
{
    const char * name;
    bench_kind kind;
    const char * operand_family;
    char bit_lengths[160];
    bench_fn fn;
    unsigned long long calls;
    modctx_ctx ctx;
} modctx_case;

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

static unsigned long long g_checksum = 0;
static unsigned long long g_seed = 0;

static void
report_case(FILE * f, modctx_case * wc, int trials)
{
    bench_stats st = bench_measure(wc->fn, &wc->ctx, wc->calls, 1, trials, 1);
    unsigned long long ops = wc->calls;

    g_checksum ^= (unsigned long long) wc->ctx.acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n",
            wc->name, bench_kind_name(wc->kind), wc->operand_family, wc->bit_lengths,
            g_seed, "yes", "yes", wc->calls, ops, (unsigned long long) wc->ctx.acc,
            st.min, st.median, st.max);
    fflush(f);
    printf("%-26s %-17s min %10.4f  median %10.4f  max %10.4f ns/op\n",
           wc->name, bench_kind_name(wc->kind), st.min, st.median, st.max);
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
    fprintf(f, "operand_family: one modulus context, 128 pairwise coprime primes near 2^32, "
               "product near 4096 bits; one integer a < K\n");
    fprintf(f, "trials_per_case: %d\n", trials);
    fprintf(f, "warm: yes\n");
    fprintf(f, "allocating: yes (each kernel allocates a temporary fmpz vector)\n");
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
    modctx_ctx shared;
    modctx_case cases[2];
    fmpz_t K;
    ulong q[MODCTX_BLOCKS];
    adf_modctx_struct * ctx = NULL;
    flint_rand_t state;
    unsigned long long checksum;
    int actual_cpu, i;
    ulong Kbits;
    FILE * f;

    flint_randinit(state);
    flint_randseed(state, (ulong) seed, (ulong) (seed >> 32));
    g_seed = seed;

    /* 128 distinct primes just above 2^32; their product is a 4096-bit modulus. */
    {
        ulong p = ((ulong) 1) << 32;
        for (i = 0; i < MODCTX_BLOCKS; i++)
        {
            p = n_nextprime(p, 0);
            q[i] = p;
        }
    }
    memset(&shared, 0, sizeof(shared));
    if (adf_modctx_new_blocks(&ctx, q, MODCTX_BLOCKS) != ADF_OK)
    {
        fprintf(stderr, "cannot build the context\n");
        return 1;
    }
    shared.ctx = ctx;
    fmpz_init(shared.a);
    fmpz_init(K);
    adf_modctx_get_modulus(K, shared.ctx);
    fmpz_randm(shared.a, state, K);
    Kbits = (ulong) fmpz_bits(K);
    adf_modctx_reduce(shared.ctx, shared.a, shared.res);
    fmpz_clear(K);
    shared.acc = 0;

    actual_cpu = bench_pin_cpu(cpu);
    if (actual_cpu < 0)
    {
        fprintf(stderr, "warning: sched_setaffinity(%d) failed; running unpinned\n", cpu);
        actual_cpu = bench_get_cpu();
    }
    printf("pinned to cpu %d (requested %d)\n", actual_cpu, cpu);
    bench_tsc_calibrate(&cal, 9, 0.02);

    memset(cases, 0, sizeof(cases));
    cases[0].name = "modctx_reduce_4096_128";
    cases[0].kind = BENCH_CALL_RATE;
    cases[0].operand_family = "global integer to k residues; a fixed 4096-bit a";
    cases[0].fn = modctx_reduce_run;
    cases[0].calls = MODCTX_CALLS;
    cases[0].ctx.ctx = shared.ctx;
    fmpz_set(cases[0].ctx.a, shared.a);
    memcpy(cases[0].ctx.res, shared.res, sizeof(shared.res));
    cases[1].name = "modctx_recombine_4096_128";
    cases[1].kind = BENCH_CALL_RATE;
    cases[1].operand_family = "k residues to the global integer; fixed residues";
    cases[1].fn = modctx_recombine_run;
    cases[1].calls = MODCTX_CRT_CALLS;
    cases[1].ctx.ctx = shared.ctx;
    fmpz_set(cases[1].ctx.a, shared.a);
    memcpy(cases[1].ctx.res, shared.res, sizeof(shared.res));
    snprintf(cases[0].bit_lengths, sizeof(cases[0].bit_lengths),
             "a %lu bit, K %lu bit, k = %d; one reduce per call",
             (unsigned long) fmpz_bits(shared.a), Kbits, MODCTX_BLOCKS);
    snprintf(cases[1].bit_lengths, sizeof(cases[1].bit_lengths),
             "K %lu bit, k = %d; one recombination per call",
             Kbits, MODCTX_BLOCKS);

    f = bench_results_open("modctx", run_id, sizeof(run_id));
    if (f == NULL)
    {
        fprintf(stderr, "cannot open result file\n");
        return 1;
    }
    bench_cpu_model(model, sizeof(model));
    write_header(f, run_id, model, cpu, actual_cpu, &cal, trials, count_cores());
    for (i = 0; i < 2; i++)
        report_case(f, &cases[i], trials);
    checksum = g_checksum;
    fprintf(f, "\n# checksum of all case results, computed outside the timed region\n");
    fprintf(f, "checksum: %llu\n", checksum);
    fclose(f);
    fmpz_clear(shared.a);
    fmpz_clear(cases[0].ctx.a);
    fmpz_clear(cases[1].ctx.a);
    adf_modctx_free(ctx);
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