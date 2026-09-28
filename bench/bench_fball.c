/* bench/bench_fball.c: benchmark rows of work package 1.2 (docs/PLAN.md section 6):
   adf_fball tight add and mul, as a latency chain and as an independent batch, for word-sized
   and for 4096-bit operands. Contract: docs/PERF.md section 7. Build and run:
   make -C bench run (the row appears in bench/results/<utc>_fball.txt). */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/fmpz.h>

#include <adelefeld/fball.h>

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

/* One timed sample stays near a second, so the eight rows cost a few seconds together. */
#define FB_BATCH_N           256
#define FB_CHAIN_ADD_WORD  50000ULL
#define FB_CHAIN_MUL_WORD   2048ULL
#define FB_CHAIN_ADD_4096   2000ULL
#define FB_CHAIN_MUL_4096    256ULL
#define FB_BATCH_ADD_WORD    200ULL
#define FB_BATCH_MUL_WORD    100ULL
#define FB_BATCH_ADD_4096     20ULL
#define FB_BATCH_MUL_4096     10ULL

typedef struct
{
    adf_fball_t x;       /* loop-carried value of a chain */
    adf_fball_t y;       /* fixed second operand of a chain */
    adf_fball_struct * a;
    adf_fball_struct * b;
    adf_fball_struct * out;
    ulong n;
    ulong acc;           /* read only after the timed region */
} fball_ctx;

/* ---------------------------------------------------------------- kernels */

static __attribute__((noinline)) void
fball_add_chain_run(void * v, unsigned long long calls)
{
    fball_ctx * c = (fball_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_fball_add(c->x, c->x, c->y);
    c->acc = (ulong) fmpz_bits(c->x->A) + (ulong) fmpz_bits(c->x->d);
}

static __attribute__((noinline)) void
fball_mul_chain_run(void * v, unsigned long long calls)
{
    fball_ctx * c = (fball_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_fball_mul(c->x, c->x, c->y);
    c->acc = (ulong) fmpz_bits(c->x->A) + (ulong) fmpz_bits(c->x->d);
}

static __attribute__((noinline)) void
fball_add_batch_run(void * v, unsigned long long passes)
{
    fball_ctx * c = (fball_ctx *) v;
    unsigned long long p;
    ulong i;

    for (p = 0; p < passes; p++)
        for (i = 0; i < c->n; i++)
            adf_fball_add(c->out + i, c->a + i, c->b + i);
    c->acc = (ulong) fmpz_bits(c->out[c->n - 1].A);
}

static __attribute__((noinline)) void
fball_mul_batch_run(void * v, unsigned long long passes)
{
    fball_ctx * c = (fball_ctx *) v;
    unsigned long long p;
    ulong i;

    for (p = 0; p < passes; p++)
        for (i = 0; i < c->n; i++)
            adf_fball_mul(c->out + i, c->a + i, c->b + i);
    c->acc = (ulong) fmpz_bits(c->out[c->n - 1].A);
}

/* -------------------------------------------------------------- the cases */

typedef struct
{
    const char * name;
    bench_kind kind;
    const char * operand_family;
    char bit_lengths[128];
    bench_fn fn;
    unsigned long long calls;
    unsigned long long ops_per_call;
    fball_ctx ctx;
} fball_case;

static ulong
bits_of(const fmpz_t x)
{
    return (ulong) fmpz_bits(x);
}

/* A canonical ball with A uniform in [0, H), H a random value of `bits` bits and d = 1. */
static void
rand_ball(adf_fball_t x, flint_rand_t state, ulong bits)
{
    fmpz_t A, H, d;
    ulong bl = bits < 2 ? 2 : bits;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    do
        fmpz_randbits(H, state, bl);
    while (fmpz_sgn(H) <= 0);
    fmpz_randm(A, state, H);
    fmpz_one(d);
    (void) adf_fball_set_fmpz3(x, A, H, d);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

static void
fill_chain(fball_case * wc, const char * name, const char * family, bench_fn fn,
           unsigned long long calls, flint_rand_t state, ulong bits, const char * what)
{
    memset(wc, 0, sizeof(*wc));
    adf_fball_init(wc->ctx.x);
    adf_fball_init(wc->ctx.y);
    rand_ball(wc->ctx.x, state, bits);
    /* y = 3 is exact: the tight product with an exact rational keeps the chain sizes linear. */
    {
        fmpz_t three;
        fmpz_init_set_ui(three, 3);
        adf_fball_set_fmpz(wc->ctx.y, three);
        fmpz_clear(three);
    }
    wc->name = name;
    wc->kind = BENCH_LATENCY_CHAIN;
    wc->operand_family = family;
    wc->fn = fn;
    wc->calls = calls;
    wc->ops_per_call = 1;
    snprintf(wc->bit_lengths, sizeof(wc->bit_lengths),
             "%s; start A %lu bit, H %lu bit, d %lu bit; y = 3 exact; %llu dependent calls",
             what, (unsigned long) bits_of(wc->ctx.x->A), (unsigned long) bits_of(wc->ctx.x->H),
             (unsigned long) bits_of(wc->ctx.x->d), calls);
}

static void
fill_batch(fball_case * wc, const char * name, const char * family, bench_fn fn,
           unsigned long long passes, flint_rand_t state, ulong bits, const char * what)
{
    ulong i;

    memset(wc, 0, sizeof(*wc));
    wc->name = name;
    wc->kind = BENCH_INDEPENDENT_BATCH;
    wc->operand_family = family;
    wc->fn = fn;
    wc->calls = passes;
    wc->ops_per_call = FB_BATCH_N;
    wc->ctx.n = FB_BATCH_N;
    wc->ctx.a = (adf_fball_struct *) malloc(FB_BATCH_N * sizeof(adf_fball_struct));
    wc->ctx.b = (adf_fball_struct *) malloc(FB_BATCH_N * sizeof(adf_fball_struct));
    wc->ctx.out = (adf_fball_struct *) malloc(FB_BATCH_N * sizeof(adf_fball_struct));
    for (i = 0; i < FB_BATCH_N; i++)
    {
        adf_fball_init(wc->ctx.a + i);
        adf_fball_init(wc->ctx.b + i);
        adf_fball_init(wc->ctx.out + i);
        rand_ball(wc->ctx.a + i, state, bits);
        rand_ball(wc->ctx.b + i, state, bits);
    }
    snprintf(wc->bit_lengths, sizeof(wc->bit_lengths),
             "%s; a[0] A,H,d up to %lu bit; b[0] up to %lu bit; n = %d",
             what, (unsigned long) bits_of(wc->ctx.a[0].H),
             (unsigned long) bits_of(wc->ctx.b[0].H), (int) FB_BATCH_N);
}

static void
free_case(fball_case * wc)
{
    ulong i;

    adf_fball_clear(wc->ctx.x);
    adf_fball_clear(wc->ctx.y);
    if (wc->ctx.a != NULL)
    {
        for (i = 0; i < wc->ctx.n; i++)
        {
            adf_fball_clear(wc->ctx.a + i);
            adf_fball_clear(wc->ctx.b + i);
            adf_fball_clear(wc->ctx.out + i);
        }
        free(wc->ctx.a);
        free(wc->ctx.b);
        free(wc->ctx.out);
    }
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

static unsigned long long g_checksum = 0;
static unsigned long long g_seed = 0;

static void
report_case(FILE * f, fball_case * wc, int trials)
{
    bench_stats st = bench_measure(wc->fn, &wc->ctx, wc->calls, wc->ops_per_call,
                                   trials, 1);
    unsigned long long ops = wc->calls * wc->ops_per_call;

    g_checksum ^= (unsigned long long) wc->ctx.acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n",
            wc->name, bench_kind_name(wc->kind), wc->operand_family, wc->bit_lengths,
            g_seed, "yes", "no", wc->calls, ops, (unsigned long long) wc->ctx.acc,
            st.min, st.median, st.max);
    fflush(f);
    printf("%-22s %-17s min %10.4f  median %10.4f  max %10.4f ns/op\n",
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
    fprintf(f, "tag: fball\n");
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
    fprintf(f, "operand_family: adf_fball canonical global triples, A,H,d as stated per row\n");
    fprintf(f, "trials_per_case: %d\n", trials);
    fprintf(f, "batch_length: %d\n", FB_BATCH_N);
    fprintf(f, "warm: yes\n");
    fprintf(f, "allocating: yes (fmpq and fmpz temporaries inside each call)\n");
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
    fball_case cases[8];
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

    fill_chain(&cases[nc++], "fball_add_chain_word",
               "adf_fball_add; x = x + 3; A,H,d word sized",
               fball_add_chain_run, FB_CHAIN_ADD_WORD, state, 60, "word");
    fill_chain(&cases[nc++], "fball_mul_chain_word",
               "adf_fball_mul; x = x * 3; A,H,d word sized",
               fball_mul_chain_run, FB_CHAIN_MUL_WORD, state, 60, "word");
    fill_chain(&cases[nc++], "fball_add_chain_4096",
               "adf_fball_add; x = x + 3; A,H,d 4096 bit",
               fball_add_chain_run, FB_CHAIN_ADD_4096, state, 4096, "4096 bit");
    fill_chain(&cases[nc++], "fball_mul_chain_4096",
               "adf_fball_mul; x = x * 3; A,H,d 4096 bit",
               fball_mul_chain_run, FB_CHAIN_MUL_4096, state, 4096, "4096 bit");
    fill_batch(&cases[nc++], "fball_add_batch_word",
               "adf_fball_add; independent pairs; A,H,d word sized",
               fball_add_batch_run, FB_BATCH_ADD_WORD, state, 60, "word");
    fill_batch(&cases[nc++], "fball_mul_batch_word",
               "adf_fball_mul; independent pairs; A,H,d word sized",
               fball_mul_batch_run, FB_BATCH_MUL_WORD, state, 60, "word");
    fill_batch(&cases[nc++], "fball_add_batch_4096",
               "adf_fball_add; independent pairs; A,H,d 4096 bit",
               fball_add_batch_run, FB_BATCH_ADD_4096, state, 4096, "4096 bit");
    fill_batch(&cases[nc++], "fball_mul_batch_4096",
               "adf_fball_mul; independent pairs; A,H,d 4096 bit",
               fball_mul_batch_run, FB_BATCH_MUL_4096, state, 4096, "4096 bit");

    f = bench_results_open("fball", run_id, sizeof(run_id));
    if (f == NULL)
    {
        fprintf(stderr, "cannot open result file\n");
        return 1;
    }
    bench_cpu_model(model, sizeof(model));
    write_header(f, run_id, model, cpu, actual_cpu, &cal, trials, count_cores());
    for (i = 0; i < nc; i++)
        report_case(f, &cases[i], trials);
    checksum = g_checksum;
    fprintf(f, "\n# checksum of all case results, computed outside the timed region\n");
    fprintf(f, "checksum: %llu\n", checksum);
    fclose(f);
    for (i = 0; i < nc; i++)
        free_case(&cases[i]);
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
