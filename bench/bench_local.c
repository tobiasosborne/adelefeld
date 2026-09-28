/* bench/bench_local.c: benchmark rows of the local backend of adf_fball, work package 1.8
   (docs/PLAN.md section 6, row 1.8: "conversions; batch add and mul"), next to the same
   operations on the global forms. Contract: docs/PERF.md section 7. Build and run:
   `make -C bench run` (the rows appear in bench/results/<utc>_local.txt), or
   `./bench/bench_local --run` from the repository root after `make -C bench bench_local`.

   Operands: one context of 128 distinct primes just above 2^32 (K of about 4096 bits, as in
   bench/bench_modctx.c), and one of a single block 2^64 - 59. Local values with d = 1 and random
   residues, which are units modulo every block with overwhelming probability, so the product
   takes the blockwise path of policies Proposition 22 (h = 1); the global rows use the
   canonical global forms of the same values. The rows are provisional (PERF section 7): the
   machine is shared with other lanes. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/ulong_extras.h>

#include <adelefeld.h>

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

#define LOCAL_BLOCKS 128
#define LOCAL_BATCH 256

typedef struct
{
    adf_fball_struct * x;       /* LOCAL_BATCH inputs */
    adf_fball_struct * y;
    adf_fball_struct * z;       /* outputs */
    const adf_modctx_struct * ctx;
    ulong acc;                  /* read only after the timed region */
} batch_ctx;

static __attribute__((noinline)) void
run_add(void * v, unsigned long long calls)
{
    batch_ctx * c = (batch_ctx *) v;
    unsigned long long i;
    slong j;
    for (i = 0; i < calls; i++)
        for (j = 0; j < LOCAL_BATCH; j++)
            adf_fball_add(c->z + j, c->x + j, c->y + j);
    c->acc = (ulong) fmpz_bits(c->z[0].d) + (c->z[0].res != NULL ? c->z[0].res[0] : 0);
}

static __attribute__((noinline)) void
run_mul(void * v, unsigned long long calls)
{
    batch_ctx * c = (batch_ctx *) v;
    unsigned long long i;
    slong j;
    for (i = 0; i < calls; i++)
        for (j = 0; j < LOCAL_BATCH; j++)
            adf_fball_mul(c->z + j, c->x + j, c->y + j);
    c->acc = (ulong) fmpz_bits(c->z[0].d) + (c->z[0].res != NULL ? c->z[0].res[0] : 0);
}

static __attribute__((noinline)) void
run_set_local(void * v, unsigned long long calls)
{
    batch_ctx * c = (batch_ctx *) v;
    unsigned long long i;
    slong j;
    for (i = 0; i < calls; i++)
        for (j = 0; j < LOCAL_BATCH; j++)
            (void) adf_fball_set_local(c->z + j, c->y + j, c->ctx);
    c->acc = c->z[0].res[0];
}

static __attribute__((noinline)) void
run_set_global(void * v, unsigned long long calls)
{
    batch_ctx * c = (batch_ctx *) v;
    unsigned long long i;
    slong j;
    for (i = 0; i < calls; i++)
        for (j = 0; j < LOCAL_BATCH; j++)
            adf_fball_set_global(c->z + j, c->x + j);
    c->acc = (ulong) fmpz_bits(c->z[0].A);
}

typedef struct
{
    const char * name;
    bench_kind kind;
    const char * family;
    bench_fn fn;
    unsigned long long calls;
    batch_ctx * ctx;
} local_case;

static unsigned long long g_checksum = 0;
static unsigned long long g_seed = 0;

static void
report_case(FILE * f, local_case * c, int trials, const char * bits)
{
    bench_stats st = bench_measure(c->fn, c->ctx, c->calls, LOCAL_BATCH, trials, 1);
    g_checksum ^= (unsigned long long) c->ctx->acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n", c->name,
            bench_kind_name(c->kind), c->family, bits, g_seed, "yes", "yes", c->calls,
            c->calls * LOCAL_BATCH, (unsigned long long) c->ctx->acc, st.min, st.median, st.max);
    fflush(f);
    printf("%-30s %-17s min %10.4f  median %10.4f  max %10.4f ns/op\n", c->name,
           bench_kind_name(c->kind), st.min, st.median, st.max);
}

/* Build LOCAL_BATCH local pairs in ctx (d = 1, random residues) and their global forms. */
static void
make_operands(batch_ctx * loc, batch_ctx * glo, batch_ctx * conv, const adf_modctx_struct * ctx,
              flint_rand_t st)
{
    slong j, i, k = adf_modctx_nblocks(ctx);
    fmpz_t A, K, one;

    fmpz_init(A);
    fmpz_init(K);
    fmpz_init_set_ui(one, 1);
    adf_modctx_get_modulus(K, ctx);
    loc->x = flint_malloc(LOCAL_BATCH * sizeof(adf_fball_struct));
    loc->y = flint_malloc(LOCAL_BATCH * sizeof(adf_fball_struct));
    loc->z = flint_malloc(LOCAL_BATCH * sizeof(adf_fball_struct));
    glo->x = flint_malloc(LOCAL_BATCH * sizeof(adf_fball_struct));
    glo->y = flint_malloc(LOCAL_BATCH * sizeof(adf_fball_struct));
    glo->z = flint_malloc(LOCAL_BATCH * sizeof(adf_fball_struct));
    for (j = 0; j < LOCAL_BATCH; j++)
    {
        adf_fball_init(loc->x + j);
        adf_fball_init(loc->y + j);
        adf_fball_init(loc->z + j);
        adf_fball_init(glo->x + j);
        adf_fball_init(glo->y + j);
        adf_fball_init(glo->z + j);
        for (i = 0; i < 2; i++)
        {
            adf_fball_struct * g = i == 0 ? glo->x + j : glo->y + j;
            adf_fball_struct * l = i == 0 ? loc->x + j : loc->y + j;
            fmpz_randm(A, st, K);
            (void) adf_fball_set_fmpz3(g, A, K, one);
            if (adf_fball_set_local(l, g, ctx) != ADF_OK)
                abort();
        }
    }
    loc->ctx = ctx;
    glo->ctx = ctx;
    /* conversions: x local (to global), y global (to local) */
    conv->x = loc->x;
    conv->y = glo->x;
    conv->z = loc->z;
    conv->ctx = ctx;
    (void) k;
    fmpz_clear(A);
    fmpz_clear(K);
    fmpz_clear(one);
}

static void
free_operands(batch_ctx * loc, batch_ctx * glo)
{
    slong j;
    for (j = 0; j < LOCAL_BATCH; j++)
    {
        adf_fball_clear(loc->x + j);
        adf_fball_clear(loc->y + j);
        adf_fball_clear(loc->z + j);
        adf_fball_clear(glo->x + j);
        adf_fball_clear(glo->y + j);
        adf_fball_clear(glo->z + j);
    }
    flint_free(loc->x);
    flint_free(loc->y);
    flint_free(loc->z);
    flint_free(glo->x);
    flint_free(glo->y);
    flint_free(glo->z);
}

static int
run(int cpu, unsigned long long seed, int trials)
{
    char run_id[64], model[BENCH_STR], bits[160];
    bench_tsc_calib cal;
    ulong q[LOCAL_BLOCKS];
    ulong q1 = UWORD(18446744073709551557);        /* 2^64 - 59, prime */
    adf_modctx_struct * c128 = NULL;
    adf_modctx_struct * c1 = NULL;
    batch_ctx loc128, glo128, conv128, loc1, glo1, conv1;
    flint_rand_t st;
    int actual_cpu, i;
    FILE * f;
    fmpz_t K;

    flint_randinit(st);
    flint_randseed(st, (ulong) seed, (ulong) (seed >> 32));
    g_seed = seed;
    {
        ulong p = UWORD(1) << 32;
        for (i = 0; i < LOCAL_BLOCKS; i++)
        {
            p = n_nextprime(p, 0);
            q[i] = p;
        }
    }
    if (adf_modctx_new_blocks(&c128, q, LOCAL_BLOCKS) != ADF_OK || adf_modctx_new_blocks(&c1, &q1, 1) != ADF_OK)
    {
        fprintf(stderr, "cannot build the contexts\n");
        return 1;
    }
    memset(&loc128, 0, sizeof(loc128));
    memset(&glo128, 0, sizeof(glo128));
    memset(&loc1, 0, sizeof(loc1));
    memset(&glo1, 0, sizeof(glo1));
    make_operands(&loc128, &glo128, &conv128, c128, st);
    make_operands(&loc1, &glo1, &conv1, c1, st);

    actual_cpu = bench_pin_cpu(cpu);
    if (actual_cpu < 0)
    {
        fprintf(stderr, "warning: sched_setaffinity(%d) failed; running unpinned\n", cpu);
        actual_cpu = bench_get_cpu();
    }
    printf("pinned to cpu %d (requested %d)\n", actual_cpu, cpu);
    bench_tsc_calibrate(&cal, 9, 0.02);

    f = bench_results_open("local", run_id, sizeof(run_id));
    if (f == NULL)
    {
        fprintf(stderr, "cannot open result file\n");
        return 1;
    }
    bench_cpu_model(model, sizeof(model));
    fprintf(f, "run_id: %s\ntag: local\nquiet_machine: no\nmachine: %s\n", run_id, model);
    fprintf(f, "affinity_requested_cpu: %d\naffinity_actual_cpu: %d\n", cpu, actual_cpu);
    fprintf(f, "compiler: gcc %s\nflags: %s\nflint_version: %s\n", __VERSION__, BENCH_CFLAGS,
            FLINT_VERSION);
    fprintf(f, "gmp_version: %d.%d.%d\n", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
            __GNU_MP_VERSION_PATCHLEVEL);
    fprintf(f, "clock_source: clock_gettime(CLOCK_MONOTONIC_RAW)\n");
    fprintf(f, "cycles_per_ns_min: %.5f\ncycles_per_ns_median: %.5f\ncycles_per_ns_max: %.5f\n",
            cal.cycles_per_ns_min, cal.cycles_per_ns_median, cal.cycles_per_ns_max);
    fprintf(f, "seed: %llu\ntrials_per_case: %d\nwarm: yes\n", g_seed, trials);
    fprintf(f, "allocating: yes (conversions allocate k fmpz in the kernels of src/modctx.c; "
               "global rows allocate fmpq temporaries)\n");
    fprintf(f, "operand_family: %d pairs; context A: 128 primes just above 2^32 (K about 4096 "
               "bits); context B: one block 2^64 - 59; d = 1; random residues (units, h = 1)\n",
            LOCAL_BATCH);
    fprintf(f, "\n# one table follows; columns are tab separated\n");
    fprintf(f, "# name\tkind\toperand_family\tbit_lengths\tseed\twarm\tallocating\tcalls\tops\t"
               "checksum\tns_per_op_min\tns_per_op_median\tns_per_op_max\n");

    fmpz_init(K);
    for (i = 0; i < 2; i++)
    {
        batch_ctx * loc = i == 0 ? &loc128 : &loc1;
        batch_ctx * glo = i == 0 ? &glo128 : &glo1;
        batch_ctx * conv = i == 0 ? &conv128 : &conv1;
        const char * sfx = i == 0 ? "4096_128" : "64_1";
        unsigned long long calls = i == 0 ? 20 : 200;
        char n[8][48];
        local_case cs[6];
        int c;

        adf_modctx_get_modulus(K, loc->ctx);
        snprintf(bits, sizeof(bits), "K %lu bit, k = %ld, d = 1", (unsigned long) fmpz_bits(K),
                 (long) adf_modctx_nblocks(loc->ctx));
        snprintf(n[0], 48, "local_add_batch_%s", sfx);
        snprintf(n[1], 48, "global_add_batch_%s", sfx);
        snprintf(n[2], 48, "local_mul_batch_%s", sfx);
        snprintf(n[3], 48, "global_mul_batch_%s", sfx);
        snprintf(n[4], 48, "set_local_%s", sfx);
        snprintf(n[5], 48, "set_global_%s", sfx);
        cs[0] = (local_case) {n[0], BENCH_INDEPENDENT_BATCH, "local + local, one context", run_add, calls, loc};
        cs[1] = (local_case) {n[1], BENCH_INDEPENDENT_BATCH, "the same sets, global", run_add, calls, glo};
        cs[2] = (local_case) {n[2], BENCH_INDEPENDENT_BATCH, "local * local, h = 1", run_mul, calls, loc};
        cs[3] = (local_case) {n[3], BENCH_INDEPENDENT_BATCH, "the same sets, global", run_mul, calls, glo};
        cs[4] = (local_case) {n[4], BENCH_CALL_RATE, "global ball to local (P19)", run_set_local, calls, conv};
        cs[5] = (local_case) {n[5], BENCH_CALL_RATE, "local to global (L17.3, P24.1)", run_set_global, calls, conv};
        for (c = 0; c < 6; c++)
            report_case(f, &cs[c], trials, bits);
    }
    fmpz_clear(K);
    fprintf(f, "\n# checksum of all case results, computed outside the timed region\n");
    fprintf(f, "checksum: %llu\n", g_checksum);
    fclose(f);
    free_operands(&loc128, &glo128);
    free_operands(&loc1, &glo1);
    adf_modctx_free(c128);
    adf_modctx_free(c1);
    flint_randclear(st);
    printf("checksum %llu\n", g_checksum);
    return 0;
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
        else
        {
            printf("usage: %s [--run] [--seed N] [--trials N] [--cpu N]\n", argv[0]);
            return strcmp(argv[i], "--help") == 0 ? 0 : 2;
        }
    }
    return run(cpu, seed, trials);
}
