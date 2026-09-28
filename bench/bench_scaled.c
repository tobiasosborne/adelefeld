/* bench/bench_scaled.c: benchmark rows of the scaled-residue policy of work package 1.7
   (docs/PLAN.md section 6, row 1.7: "add, mul"), provisional.  Contract: docs/PERF.md section 7.

   Rows: adf_scaled_add and adf_scaled_mul, at a modulus of one word (K = 2^61 + 3) and at a
   modulus of 4096 bits (K = 2^4095 + 19), both contexts without blocks (conventions 5.14).
   Kind: call rate (one operation per call over fixed operands); warm; every call allocates its
   FLINT temporaries; results are consumed after the timed region.  docs/PERF.md section 4 has
   no floor for these operations ("Floors for the operations still to be written"), so no ratio
   is reported and the rows are provisional in the sense of PERF section 7.

   The operands are scaled values s (u + K Zhat) with s = 5/2 and a residue u of the full size
   of the modulus; both come from adf_scaled_set_fball outside the timed region.  The context is
   built outside the timed region as well (PERF section 4: set-up of contexts is a separate row).

   Build and run: `make -C bench run` (the rows appear in bench/results/<utc>_scaled.txt). */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>

#include <adelefeld/scaled.h>

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

#define SCALED_CALLS 20000ULL

typedef struct
{
    adf_scaled_t x;
    adf_scaled_t y;
    adf_scaled_t z;
    ulong acc; /* read only after the timed region */
} scaled_ctx;

static __attribute__((noinline)) void
scaled_add_run(void * v, unsigned long long calls)
{
    scaled_ctx * c = (scaled_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_scaled_add(c->z, c->x, c->y);
    c->acc = (ulong) fmpq_get_d(c->z->s) + (ulong) fmpz_get_si(c->z->u);
}

static __attribute__((noinline)) void
scaled_mul_run(void * v, unsigned long long calls)
{
    scaled_ctx * c = (scaled_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_scaled_mul(c->z, c->x, c->y);
    c->acc = (ulong) fmpq_get_d(c->z->s) + (ulong) fmpz_get_si(c->z->u);
}

typedef struct
{
    const char * name;
    bench_kind kind;
    const char * operand_family;
    char bit_lengths[160];
    bench_fn fn;
    unsigned long long calls;
    scaled_ctx ctx;
} scaled_case;

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
report_case(FILE * f, scaled_case * wc, int trials)
{
    bench_stats st = bench_measure(wc->fn, &wc->ctx, wc->calls, 1, trials, 1);
    unsigned long long ops = wc->calls;

    g_checksum ^= (unsigned long long) wc->ctx.acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n",
            wc->name, bench_kind_name(wc->kind), wc->operand_family, wc->bit_lengths,
            g_seed, "yes", "yes", wc->calls, ops, (unsigned long long) wc->ctx.acc,
            st.min, st.median, st.max);
    fflush(f);
    printf("%-24s %-17s min %10.4f  median %10.4f  max %10.4f ns/op\n",
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
    fprintf(f, "tag: scaled\n");
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
    fprintf(f, "operand_family: scaled values s (u + K Zhat), s = 5/2, u of the full size of "
               "K; K = 2^61 + 3 and K = 2^4095 + 19; contexts without blocks\n");
    fprintf(f, "trials_per_case: %d\n", trials);
    fprintf(f, "warm: yes\n");
    fprintf(f, "allocating: yes (every call allocates its FLINT temporaries)\n");
    fprintf(f, "\n");
    fprintf(f, "# one table follows; columns are tab separated\n");
    fprintf(f, "# name\tkind\toperand_family\tbit_lengths\tseed\twarm\t"
               "allocating\tcalls\tops\tchecksum\tns_per_op_min\t"
               "ns_per_op_median\tns_per_op_max\n");
}

/* x = y = (5/2)(u + K Zhat) in ctx, through adf_scaled_set_fball of the ball (5/2) u + (5/2) K
   Zhat (policies Lemma 5: the stored data are the data of the set). */
static void
scaled_setup(scaled_ctx * c, const adf_modctx_struct * ctx, const fmpz_t u)
{
    adf_rat_t s, centre, radius;
    adf_fball_t b;
    fmpz_t K;

    fmpz_init(K);
    adf_modctx_get_modulus(K, ctx);
    adf_rat_init(s);
    adf_rat_init(centre);
    adf_rat_init(radius);
    adf_fball_init(b);
    fmpq_set_si(s->q, 5, 2);
    fmpq_mul_fmpz(centre->q, s->q, u);
    fmpq_mul_fmpz(radius->q, s->q, K);
    adf_fball_set_center_radius(b, centre, radius);
    adf_scaled_init(c->x, ctx);
    adf_scaled_init(c->y, ctx);
    adf_scaled_init(c->z, ctx);
    adf_scaled_set_fball(c->x, NULL, b, ctx);
    adf_scaled_set_fball(c->y, NULL, b, ctx);
    c->acc = 0;
    adf_fball_clear(b);
    adf_rat_clear(s);
    adf_rat_clear(centre);
    adf_rat_clear(radius);
    fmpz_clear(K);
}

static void
scaled_teardown(scaled_ctx * c)
{
    adf_scaled_clear(c->x);
    adf_scaled_clear(c->y);
    adf_scaled_clear(c->z);
}

static int
run(int cpu, unsigned long long seed, int trials)
{
    char run_id[64];
    char model[BENCH_STR];
    bench_tsc_calib cal;
    scaled_case cases[4];
    adf_modctx_struct * ctxw = NULL;
    adf_modctx_struct * ctxb = NULL;
    fmpz_t Kw, Kb, uw, ub;
    unsigned long long checksum;
    int actual_cpu, i;
    FILE * f;

    g_seed = seed;
    fmpz_init(Kw);
    fmpz_init(Kb);
    fmpz_init(uw);
    fmpz_init(ub);
    /* K = 2^61 + 3 (one word) and K = 2^4095 + 19 (4096 bits), deterministic operands */
    fmpz_one(Kw);
    fmpz_mul_2exp(Kw, Kw, 61);
    fmpz_add_ui(Kw, Kw, 3);
    fmpz_one(Kb);
    fmpz_mul_2exp(Kb, Kb, 4095);
    fmpz_add_ui(Kb, Kb, 19);
    if (adf_modctx_new_fmpz(&ctxw, Kw) != ADF_OK || adf_modctx_new_fmpz(&ctxb, Kb) != ADF_OK)
    {
        fprintf(stderr, "cannot build the contexts\n");
        return 1;
    }
    fmpz_one(uw);
    fmpz_mul_2exp(uw, uw, 60);
    fmpz_add_ui(uw, uw, 12345);
    fmpz_mod(uw, uw, Kw);
    fmpz_one(ub);
    fmpz_mul_2exp(ub, ub, 4094);
    fmpz_add_ui(ub, ub, 54321);
    fmpz_mod(ub, ub, Kb);

    memset(cases, 0, sizeof(cases));
    cases[0].name = "scaled_add_word";
    cases[0].kind = BENCH_CALL_RATE;
    cases[0].operand_family = "scaled add, one-word modulus";
    cases[0].fn = scaled_add_run;
    cases[0].calls = SCALED_CALLS;
    cases[1].name = "scaled_mul_word";
    cases[1].kind = BENCH_CALL_RATE;
    cases[1].operand_family = "scaled mul (default product), one-word modulus";
    cases[1].fn = scaled_mul_run;
    cases[1].calls = SCALED_CALLS;
    cases[2].name = "scaled_add_4096";
    cases[2].kind = BENCH_CALL_RATE;
    cases[2].operand_family = "scaled add, 4096-bit modulus";
    cases[2].fn = scaled_add_run;
    cases[2].calls = SCALED_CALLS;
    cases[3].name = "scaled_mul_4096";
    cases[3].kind = BENCH_CALL_RATE;
    cases[3].operand_family = "scaled mul (default product), 4096-bit modulus";
    cases[3].fn = scaled_mul_run;
    cases[3].calls = SCALED_CALLS;
    scaled_setup(&cases[0].ctx, ctxw, uw);
    scaled_setup(&cases[1].ctx, ctxw, uw);
    scaled_setup(&cases[2].ctx, ctxb, ub);
    scaled_setup(&cases[3].ctx, ctxb, ub);
    snprintf(cases[0].bit_lengths, sizeof(cases[0].bit_lengths),
             "s 5/2, u %lu bit, K %lu bit; one add per call",
             (ulong) fmpz_bits(uw), (ulong) fmpz_bits(Kw));
    snprintf(cases[1].bit_lengths, sizeof(cases[1].bit_lengths),
             "s 5/2, u %lu bit, K %lu bit; one mul per call",
             (ulong) fmpz_bits(uw), (ulong) fmpz_bits(Kw));
    snprintf(cases[2].bit_lengths, sizeof(cases[2].bit_lengths),
             "s 5/2, u %lu bit, K %lu bit; one add per call",
             (ulong) fmpz_bits(ub), (ulong) fmpz_bits(Kb));
    snprintf(cases[3].bit_lengths, sizeof(cases[3].bit_lengths),
             "s 5/2, u %lu bit, K %lu bit; one mul per call",
             (ulong) fmpz_bits(ub), (ulong) fmpz_bits(Kb));

    actual_cpu = bench_pin_cpu(cpu);
    if (actual_cpu < 0)
    {
        fprintf(stderr, "warning: sched_setaffinity(%d) failed; running unpinned\n", cpu);
        actual_cpu = bench_get_cpu();
    }
    printf("pinned to cpu %d (requested %d)\n", actual_cpu, cpu);
    bench_tsc_calibrate(&cal, 9, 0.02);

    f = bench_results_open("scaled", run_id, sizeof(run_id));
    if (f == NULL)
    {
        fprintf(stderr, "cannot open result file\n");
        return 1;
    }
    bench_cpu_model(model, sizeof(model));
    write_header(f, run_id, model, cpu, actual_cpu, &cal, trials, count_cores());
    for (i = 0; i < 4; i++)
        report_case(f, &cases[i], trials);
    checksum = g_checksum;
    fprintf(f, "\n# checksum of all case results, computed outside the timed region\n");
    fprintf(f, "checksum: %llu\n", checksum);
    fclose(f);
    for (i = 0; i < 4; i++)
        scaled_teardown(&cases[i].ctx);
    adf_modctx_free(ctxw);
    adf_modctx_free(ctxb);
    fmpz_clear(Kw);
    fmpz_clear(Kb);
    fmpz_clear(uw);
    fmpz_clear(ub);
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
