/* bench/bench_recon.c: the benchmark row of work package 1.6 (docs/PLAN.md section 6, row 1.6,
   "Rational reconstruction from a full ball"), provisional.

   The two calls of the package, adf_fball_reconstruct and adf_adele_reconstruct, as a latency
   chain and as an independent batch, for word-sized and for 4096-bit operands. Contract:
   docs/PERF.md section 7 (kind, operand family and bit lengths, seed, warm, affinity, compiler,
   flags, library versions, clock source; min, median, max; the result is consumed outside the
   timed region). Build and run: make -C bench run, the row appears in
   bench/results/<utc>_recon.txt.

   The chain of the finite-ball call uses a fixed second and third operand and writes the same
   candidate every time (the interval [a, a], so exactly one candidate), which keeps the sizes of
   the operands constant from call to call: a chain that changes the ball would measure the
   growth of the operands instead of the cost of one call. The chain of the adele call is the
   same, with the real ball fixed as well. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/arb.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/mag.h>

#include <adelefeld/adele.h>
#include <adelefeld/recon.h>

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

/* One timed sample stays near a second, so the eight rows cost a few seconds together. */
#define RECON_BATCH_N            256
#define RECON_CHAIN_FBALL_WORD   20000ULL
#define RECON_CHAIN_ADELE_WORD   10000ULL
#define RECON_CHAIN_FBALL_4096    2000ULL
#define RECON_CHAIN_ADELE_4096    1000ULL
#define RECON_BATCH_FBALL_WORD     100ULL
#define RECON_BATCH_ADELE_WORD      50ULL
#define RECON_BATCH_FBALL_4096      10ULL
#define RECON_BATCH_ADELE_4096       5ULL

typedef struct
{
    adf_fball_t x;        /* the finite ball of the chain, or of the batch */
    adf_rat_t lo, hi;     /* the closed interval of the chain, or of the batch */
    adf_adele_t a;        /* the adele of the chain, or of the batch */
    adf_rat_struct * out;  /* one output per element of the batch */
    adf_rat_t q;          /* the output of the chain */
    ulong n;              /* length of a batch */
    ulong acc;            /* read only after the timed region */
} recon_ctx;

/* ---------------------------------------------------------------- kernels */

static __attribute__((noinline)) void
recon_fball_chain_run(void * v, unsigned long long calls)
{
    recon_ctx * c = (recon_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_fball_reconstruct(c->q, c->x, c->lo, c->hi);
    c->acc = (ulong) fmpq_numref(c->q->q)[0];
}

static __attribute__((noinline)) void
recon_adele_chain_run(void * v, unsigned long long calls)
{
    recon_ctx * c = (recon_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_adele_reconstruct(c->q, c->a);
    c->acc = (ulong) fmpq_numref(c->q->q)[0];
}

static __attribute__((noinline)) void
recon_fball_batch_run(void * v, unsigned long long passes)
{
    recon_ctx * c = (recon_ctx *) v;
    unsigned long long p;
    ulong i;

    for (p = 0; p < passes; p++)
        for (i = 0; i < c->n; i++)
            adf_fball_reconstruct(c->out + i, c->x, c->lo, c->hi);
    c->acc = (ulong) fmpq_numref(c->out[c->n - 1].q)[0];
}

static __attribute__((noinline)) void
recon_adele_batch_run(void * v, unsigned long long passes)
{
    recon_ctx * c = (recon_ctx *) v;
    unsigned long long p;
    ulong i;

    for (p = 0; p < passes; p++)
        for (i = 0; i < c->n; i++)
            adf_adele_reconstruct(c->out + i, c->a);
    c->acc = (ulong) fmpq_numref(c->out[c->n - 1].q)[0];
}

/* -------------------------------------------------------------- the cases */

typedef enum { RECON_CALL_FBALL = 0, RECON_CALL_ADELE = 1 } recon_call;

typedef struct
{
    const char * name;
    bench_kind kind;
    const char * operand_family;
    char bit_lengths[192];
    bench_fn fn;
    unsigned long long calls;
    unsigned long long ops_per_call;
    recon_call which;
    recon_ctx ctx;
} recon_case;

static void
big_fmpz(fmpz_t z, slong bits, ulong seed)
{
    slong i;

    fmpz_zero(z);
    for (i = 0; i < bits; i++)
        if (((ulong) i + seed) % 3 == 0)
            fmpz_setbit(z, (ulong) i);
}

/* A finite ball of `bits` bits: A and H of that many bits, d a power of two of 4000 bits, so that
   the centre A/d is a dyadic number, as the real ball of the adele needs. */
static void
rand_ball(adf_fball_t x, flint_rand_t state, ulong bits)
{
    fmpz_t A, H, d;
    ulong bl = bits < 8 ? 8 : bits;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    do
        fmpz_randbits(H, state, bl);
    while (fmpz_sgn(H) <= 0);
    fmpz_randm(A, state, H);
    fmpz_one(d);
    fmpz_mul_2exp(d, d, 4000);
    (void) adf_fball_set_fmpz3(x, A, H, d);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

/* The real ball of the adele: the exact ball of the centre a of x with the radius 2^-899, which
   is a ball of positive radius for a centre of 4096 bits and is a point for a word-sized centre
   only up to the resolution of arb. Written through arb_midref and arb_radref
   (/usr/include/flint/arb.h:38-39), the macros of FLINT. */
static void
set_real_about(arb_t r, const adf_fball_t x, slong re)
{
    adf_rat_t a;
    fmpz_t num, den, ex, one;

    adf_rat_init(a);
    fmpz_init(num);
    fmpz_init(den);
    fmpz_init(ex);
    fmpz_init_set_ui(one, 1);
    adf_fball_get_center(a, x);
    fmpz_fdiv_q(num, fmpq_numref(a->q), fmpq_denref(a->q));
    fmpz_fdiv_r(den, fmpq_numref(a->q), fmpq_denref(a->q));
    if (fmpz_is_zero(den))
    {
        arb_set_fmpz(r, num);
    }
    else
    {
        /* A centre that is not dyadic is rounded to 64 bits; the row says so. */
        arb_set_fmpq(r, a->q, 64);
    }
    fmpz_set_si(ex, re);
    mag_set_fmpz_2exp_fmpz(arb_radref(r), one, ex);
    fmpz_clear(num);
    fmpz_clear(den);
    fmpz_clear(ex);
    fmpz_clear(one);
    adf_rat_clear(a);
}

static void
ctx_init(recon_ctx * c, slong n)
{
    adf_rat_init(c->lo);
    adf_rat_init(c->hi);
    adf_rat_init(c->q);
    adf_fball_init(c->x);
    arb_init(c->a->inf);
    adf_fball_init(&c->a->fin);
    c->n = (ulong) n;
}

static void
ctx_clear(recon_ctx * c)
{
    adf_rat_clear(c->lo);
    adf_rat_clear(c->hi);
    adf_rat_clear(c->q);
    adf_fball_clear(c->x);
    arb_clear(c->a->inf);
    adf_fball_clear(&c->a->fin);
}

static void
fill_chain(recon_case * wc, const char * name, const char * family, bench_fn fn,
           unsigned long long calls, recon_call which, flint_rand_t state, ulong bits)
{
    memset(wc, 0, sizeof(*wc));
    ctx_init(&wc->ctx, 1);
    rand_ball(wc->ctx.x, state, bits);
    /* The interval [a, a]: exactly one candidate, the centre, whatever the size of the ball. */
    adf_fball_get_center(wc->ctx.lo, wc->ctx.x);
    adf_fball_get_center(wc->ctx.hi, wc->ctx.x);
    adf_fball_set(&wc->ctx.a->fin, wc->ctx.x);
    set_real_about(wc->ctx.a->inf, wc->ctx.x, -899);
    wc->name = name;
    wc->kind = BENCH_LATENCY_CHAIN;
    wc->operand_family = family;
    wc->fn = fn;
    wc->calls = calls;
    wc->ops_per_call = 1;
    wc->which = which;
    snprintf(wc->bit_lengths, sizeof(wc->bit_lengths),
             "start A %lu bit, H %lu bit, d 4001 bit; the real ball of radius 2^-899; "
             "[a, a] as the interval; %llu dependent calls",
             (unsigned long) fmpz_bits(wc->ctx.x->A), (unsigned long) fmpz_bits(wc->ctx.x->H),
             calls);
}

static void
fill_batch(recon_case * wc, const char * name, const char * family, bench_fn fn,
           unsigned long long passes, recon_call which, flint_rand_t state, ulong bits)
{
    ulong i;

    memset(wc, 0, sizeof(*wc));
    ctx_init(&wc->ctx, RECON_BATCH_N);
    rand_ball(wc->ctx.x, state, bits);
    adf_fball_get_center(wc->ctx.lo, wc->ctx.x);
    adf_fball_get_center(wc->ctx.hi, wc->ctx.x);
    adf_fball_set(&wc->ctx.a->fin, wc->ctx.x);
    set_real_about(wc->ctx.a->inf, wc->ctx.x, -899);
    wc->ctx.out = (adf_rat_struct *) malloc(RECON_BATCH_N * sizeof(adf_rat_struct));
    for (i = 0; i < RECON_BATCH_N; i++)
        adf_rat_init(wc->ctx.out + i);
    wc->name = name;
    wc->kind = BENCH_INDEPENDENT_BATCH;
    wc->operand_family = family;
    wc->fn = fn;
    wc->calls = passes;
    wc->ops_per_call = RECON_BATCH_N;
    wc->which = which;
    snprintf(wc->bit_lengths, sizeof(wc->bit_lengths),
             "one ball, A and H up to %lu bit, d 4001 bit; one real ball of radius 2^-899; "
             "[a, a] as the interval; n = %d outputs",
             (unsigned long) fmpz_bits(wc->ctx.x->H), (int) RECON_BATCH_N);
}

static void
free_case(recon_case * wc)
{
    ulong i;

    ctx_clear(&wc->ctx);
    if (wc->ctx.out != NULL)
    {
        for (i = 0; i < RECON_BATCH_N; i++)
            adf_rat_clear(wc->ctx.out + i);
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
report_case(FILE * f, recon_case * wc, int trials)
{
    bench_stats st =
        bench_measure(wc->fn, &wc->ctx, wc->calls, wc->ops_per_call, trials, 1);
    unsigned long long ops = wc->calls * wc->ops_per_call;

    g_checksum ^= (unsigned long long) wc->ctx.acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n", wc->name,
            bench_kind_name(wc->kind), wc->operand_family, wc->bit_lengths, g_seed, "yes", "yes",
            wc->calls, ops, (unsigned long long) wc->ctx.acc, st.min, st.median, st.max);
    fflush(f);
    printf("%-24s %-17s min %10.4f  median %10.4f  max %10.4f ns/op\n", wc->name,
           bench_kind_name(wc->kind), st.min, st.median, st.max);
}

static void
write_header(FILE * f, const char * run_id, const char * model, int requested_cpu, int actual_cpu,
             const bench_tsc_calib * cal, int trials, int cores)
{
    char gmp[64];

    snprintf(gmp, sizeof(gmp), "%d.%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
             __GNU_MP_VERSION_PATCHLEVEL);
    fprintf(f, "run_id: %s\n", run_id);
    fprintf(f, "tag: recon\n");
    fprintf(f, "provisional: yes (PLAN row 1.6, one row; the other lanes of the milestone may "
               "change the cost of the shared helpers)\n");
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
    fprintf(f, "cycles_per_ns_spread: %.5f\n", cal->cycles_per_ns_max - cal->cycles_per_ns_min);
    fprintf(f, "seed: %llu\n", g_seed);
    fprintf(f, "operand_family: adf_fball canonical global triples A, H, d as stated per row; "
               "the adele rows add the arb of the same centre with the radius 2^-899\n");
    fprintf(f, "trials_per_case: %d\n", trials);
    fprintf(f, "batch_length: %d\n", RECON_BATCH_N);
    fprintf(f, "warm: yes\n");
    fprintf(f, "allocating: yes (fmpq and fmpz temporaries inside each call, and the output is "
               "rewritten in a batch)\n");
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
    recon_case cases[8];
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

    fill_chain(&cases[nc++], "recon_fball_chain_word",
               "adf_fball_reconstruct; [a, a] against a ball with A,H word sized",
               recon_fball_chain_run, RECON_CHAIN_FBALL_WORD, RECON_CALL_FBALL, state, 60);
    fill_chain(&cases[nc++], "recon_adele_chain_word",
               "adf_adele_reconstruct; the same ball and the real ball of radius 2^-899",
               recon_adele_chain_run, RECON_CHAIN_ADELE_WORD, RECON_CALL_ADELE, state, 60);
    fill_chain(&cases[nc++], "recon_fball_chain_4096",
               "adf_fball_reconstruct; [a, a] against a ball with A,H of 4096 bit",
               recon_fball_chain_run, RECON_CHAIN_FBALL_4096, RECON_CALL_FBALL, state, 4096);
    fill_chain(&cases[nc++], "recon_adele_chain_4096",
               "adf_adele_reconstruct; the same ball and the real ball of radius 2^-899",
               recon_adele_chain_run, RECON_CHAIN_ADELE_4096, RECON_CALL_ADELE, state, 4096);
    fill_batch(&cases[nc++], "recon_fball_batch_word",
               "adf_fball_reconstruct; one ball, independent outputs",
               recon_fball_batch_run, RECON_BATCH_FBALL_WORD, RECON_CALL_FBALL, state, 60);
    fill_batch(&cases[nc++], "recon_adele_batch_word",
               "adf_adele_reconstruct; one adele, independent outputs",
               recon_adele_batch_run, RECON_BATCH_ADELE_WORD, RECON_CALL_ADELE, state, 60);
    fill_batch(&cases[nc++], "recon_fball_batch_4096",
               "adf_fball_reconstruct; one ball with A,H of 4096 bit, independent outputs",
               recon_fball_batch_run, RECON_BATCH_FBALL_4096, RECON_CALL_FBALL, state, 4096);
    fill_batch(&cases[nc++], "recon_adele_batch_4096",
               "adf_adele_reconstruct; one adele with A,H of 4096 bit, independent outputs",
               recon_adele_batch_run, RECON_BATCH_ADELE_4096, RECON_CALL_ADELE, state, 4096);

    f = bench_results_open("recon", run_id, sizeof(run_id));
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
