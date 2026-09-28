/* bench/bench_adele.c: benchmark rows of work package 1.3 (docs/PLAN.md section 6):
   adf_adele add and mul, as latency chains, at the two real precisions 53 and 4096 bits and
   with finite parts of word size and of 4096 bits. Contract: docs/PERF.md section 7. Build and
   run: make -C bench bench_adele && ./bench/bench_adele --run (the row appears in
   bench/results/<utc>_adele.txt). Each row's second operand is the exact (3 ; 3), so a chain
   keeps the operand sizes of the finite part linear while it exercises both coordinates. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/arb.h>

#include <adelefeld/adele.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

#define ADELE_CHAIN_53_WORD      20000ULL
#define ADELE_CHAIN_MUL_53_WORD   3000ULL
#define ADELE_CHAIN_53_4096       2000ULL
#define ADELE_CHAIN_MUL_53_4096    300ULL
#define ADELE_CHAIN_4096_WORD     4000ULL
#define ADELE_CHAIN_MUL_4096_WORD  500ULL
#define ADELE_CHAIN_4096_4096      800ULL
#define ADELE_CHAIN_MUL_4096_4096  100ULL

typedef struct
{
    adf_adele_t x;   /* loop-carried value of a chain */
    adf_adele_t y;   /* fixed second operand, the exact (3 ; 3) */
    ulong acc;       /* read only after the timed region */
} adele_ctx;

static __attribute__((noinline)) void
adele_add_chain_run(void * v, unsigned long long calls)
{
    adele_ctx * c = (adele_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_adele_add(c->x, c->x, c->y, 53);
    c->acc = (ulong) fmpz_bits(c->x->fin.A) + (ulong) fmpz_bits(c->x->fin.H);
}

static __attribute__((noinline)) void
adele_mul_chain_run(void * v, unsigned long long calls)
{
    adele_ctx * c = (adele_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_adele_mul(c->x, c->x, c->y, 53);
    c->acc = (ulong) fmpz_bits(c->x->fin.A) + (ulong) fmpz_bits(c->x->fin.H);
}

static __attribute__((noinline)) void
adele_add_chain_run_4096(void * v, unsigned long long calls)
{
    adele_ctx * c = (adele_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_adele_add(c->x, c->x, c->y, 4096);
    c->acc = (ulong) fmpz_bits(c->x->fin.A) + (ulong) fmpz_bits(c->x->fin.H);
}

static __attribute__((noinline)) void
adele_mul_chain_run_4096(void * v, unsigned long long calls)
{
    adele_ctx * c = (adele_ctx *) v;
    unsigned long long i;

    for (i = 0; i < calls; i++)
        adf_adele_mul(c->x, c->x, c->y, 4096);
    c->acc = (ulong) fmpz_bits(c->x->fin.A) + (ulong) fmpz_bits(c->x->fin.H);
}

typedef struct
{
    const char * name;
    bench_kind kind;
    const char * operand_family;
    char bit_lengths[128];
    bench_fn fn;
    unsigned long long calls;
    unsigned long long ops_per_call;
    adele_ctx ctx;
} adele_case;

static ulong
bits_of(const fmpz_t x)
{
    return (ulong) fmpz_bits(x);
}

/* A canonical finite ball with H a random value of `bits` bits and d = 1. */
static void
rand_fball(adf_fball_t x, flint_rand_t state, ulong bits)
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

/* x = (a random integer of `prec` bits ; a random ball of `fin_bits` bits); y = (3 ; 3). */
static void
fill_chain(adele_case * wc, const char * name, const char * family, bench_fn fn,
           unsigned long long calls, flint_rand_t state, slong prec, ulong fin_bits,
           const char * what)
{
    fmpz_t m;
    adf_fball_t f;
    arb_t r;

    memset(wc, 0, sizeof(*wc));
    adf_adele_init(wc->ctx.x);
    adf_adele_init(wc->ctx.y);
    fmpz_init(m);
    adf_fball_init(f);
    arb_init(r);

    fmpz_randbits(m, state, (ulong) prec < 2 ? 2 : (ulong) prec);
    arb_set_fmpz(r, m);
    rand_fball(f, state, fin_bits);
    (void) adf_adele_set_arb_fball(wc->ctx.x, r, f);
    adf_adele_set_si(wc->ctx.y, 3);

    wc->name = name;
    wc->kind = BENCH_LATENCY_CHAIN;
    wc->operand_family = family;
    wc->fn = fn;
    wc->calls = calls;
    wc->ops_per_call = 1;
    snprintf(wc->bit_lengths, sizeof(wc->bit_lengths),
             "%s; start real %ld bit, fin H %lu bit, d %lu bit; y = (3 ; 3); %llu dependent "
             "calls", what, (long) prec, (unsigned long) bits_of(wc->ctx.x->fin.H),
             (unsigned long) bits_of(wc->ctx.x->fin.d), calls);

    fmpz_clear(m);
    adf_fball_clear(f);
    arb_clear(r);
}

static void
free_case(adele_case * wc)
{
    adf_adele_clear(wc->ctx.x);
    adf_adele_clear(wc->ctx.y);
}

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
report_case(FILE * f, adele_case * wc, int trials)
{
    bench_stats st = bench_measure(wc->fn, &wc->ctx, wc->calls, wc->ops_per_call, trials, 1);
    unsigned long long ops = wc->calls * wc->ops_per_call;

    g_checksum ^= (unsigned long long) wc->ctx.acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n",
            wc->name, bench_kind_name(wc->kind), wc->operand_family, wc->bit_lengths, g_seed,
            "yes", "yes", wc->calls, ops, (unsigned long long) wc->ctx.acc, st.min,
            st.median, st.max);
    fflush(f);
    printf("%-26s %-17s min %10.4f  median %10.4f  max %10.4f ns/op\n", wc->name,
           bench_kind_name(wc->kind), st.min, st.median, st.max);
}

static void
write_header(FILE * f, const char * run_id, const char * model, int requested_cpu,
             int actual_cpu, const bench_tsc_calib * cal, int trials, int cores)
{
    char gmp[64];

    snprintf(gmp, sizeof(gmp), "%d.%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
             __GNU_MP_VERSION_PATCHLEVEL);
    fprintf(f, "run_id: %s\n", run_id);
    fprintf(f, "tag: adele\n");
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
    fprintf(f, "operand_family: adf_adele (arb real ball ; canonical global finite ball)\n");
    fprintf(f, "trials_per_case: %d\n", trials);
    fprintf(f, "batch_length: 1\n");
    fprintf(f, "warm: yes\n");
    fprintf(f, "allocating: yes (arb and fmpq temporaries inside each call)\n");
    fprintf(f, "\n");
    fprintf(f, "# one table follows; columns are tab separated\n");
    fprintf(f, "# name\tkind\toperand_family\tbit_lengths\tseed\twarm\t"
               "allocating\tcalls\tops\tchecksum\tns_per_op_min\tns_per_op_median\t"
               "ns_per_op_max\n");
}

static int
run(int cpu, unsigned long long seed, int trials)
{
    char run_id[64];
    char model[BENCH_STR];
    bench_tsc_calib cal;
    adele_case cases[8];
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

    fill_chain(&cases[nc++], "adele_add_chain_53_word", "adf_adele_add; x = x + (3 ; 3); "
               "real 53 bit, finite word", adele_add_chain_run, ADELE_CHAIN_53_WORD, state,
               53, 60, "word");
    fill_chain(&cases[nc++], "adele_mul_chain_53_word", "adf_adele_mul; x = x * (3 ; 3); "
               "real 53 bit, finite word", adele_mul_chain_run, ADELE_CHAIN_MUL_53_WORD,
               state, 53, 60, "word");
    fill_chain(&cases[nc++], "adele_add_chain_53_4096", "adf_adele_add; x = x + (3 ; 3); "
               "real 53 bit, finite 4096 bit", adele_add_chain_run, ADELE_CHAIN_53_4096,
               state, 53, 4096, "4096 bit");
    fill_chain(&cases[nc++], "adele_mul_chain_53_4096", "adf_adele_mul; x = x * (3 ; 3); "
               "real 53 bit, finite 4096 bit", adele_mul_chain_run, ADELE_CHAIN_MUL_53_4096,
               state, 53, 4096, "4096 bit");
    fill_chain(&cases[nc++], "adele_add_chain_4096_word", "adf_adele_add; x = x + (3 ; 3); "
               "real 4096 bit, finite word", adele_add_chain_run_4096,
               ADELE_CHAIN_4096_WORD, state, 4096, 60, "word");
    fill_chain(&cases[nc++], "adele_mul_chain_4096_word", "adf_adele_mul; x = x * (3 ; 3); "
               "real 4096 bit, finite word", adele_mul_chain_run_4096,
               ADELE_CHAIN_MUL_4096_WORD, state, 4096, 60, "word");
    fill_chain(&cases[nc++], "adele_add_chain_4096_4096", "adf_adele_add; x = x + (3 ; 3); "
               "real 4096 bit, finite 4096 bit", adele_add_chain_run_4096,
               ADELE_CHAIN_4096_4096, state, 4096, 4096, "4096 bit");
    fill_chain(&cases[nc++], "adele_mul_chain_4096_4096", "adf_adele_mul; x = x * (3 ; 3); "
               "real 4096 bit, finite 4096 bit", adele_mul_chain_run_4096,
               ADELE_CHAIN_MUL_4096_4096, state, 4096, 4096, "4096 bit");

    f = bench_results_open("adele", run_id, sizeof(run_id));
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
    int i, cpu = 2, trials = 3;
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
