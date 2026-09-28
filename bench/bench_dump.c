/* bench/bench_dump.c: benchmark rows of work package 1.4 for the dump form (docs/PLAN.md section 6,
   row 1.4: "print, parse"): dump and load of adf_rat, adf_fball (global and local), adf_adele and
   adf_scaled, for word-sized and 4096-bit values and a context of 128 blocks, each as an
   independent batch. Contract: docs/PERF.md section 7; the floor of docs/PERF.md section 4, row
   "Text dump of B bytes; parse of a valid text of B bytes", is B bytes written or read.
   Provisional (brief of lane m1-dump): the rows are measurements only; the mean text length B is
   written in the column bit_lengths so that ns per byte can be read off.
   Build and run: make -C bench run (the rows appear in bench/results/<utc>_dump.txt), or
   ./bench/bench_dump --run --cpu 2 --trials 9 from the repository root.

   A dump row times adf_x_dump_str and the flint_free of its string; a load row times
   adf_x_load_str_binds on the texts dumped before the timed region, into initialised values.
   Both allocate. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/ulong_extras.h>

#include <adelefeld.h>

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

#define DB_BATCH_N 128

enum { T_RAT = 0, T_FBALL = 1, T_LOCAL = 2, T_ADELE = 3, T_SCALED = 4 };

typedef struct
{
    int type;
    adf_rat_struct * r;
    adf_fball_struct * f;
    adf_adele_struct * a;
    adf_scaled_struct * s;
    const adf_modctx_struct * ctx;
    char ** text;
    size_t * len;
    ulong n;
    ulong acc;           /* read only after the timed region */
} dump_ctx;

/* ---------------------------------------------------------------- kernels */

static __attribute__((noinline)) void
dump_run(void * v, unsigned long long passes)
{
    dump_ctx * c = (dump_ctx *) v;
    unsigned long long p;
    ulong i;
    size_t len, total = 0;
    char * s;

    for (p = 0; p < passes; p++)
        for (i = 0; i < c->n; i++)
        {
            if (c->type == T_RAT)
                s = adf_rat_dump_str(&len, c->r + i);
            else if (c->type == T_FBALL || c->type == T_LOCAL)
                s = adf_fball_dump_str(&len, c->f + i);
            else if (c->type == T_ADELE)
                s = adf_adele_dump_str(&len, c->a + i);
            else
                s = adf_scaled_dump_str(&len, c->s + i);
            total += len;
            flint_free(s);
        }
    c->acc = (ulong) total;
}

static __attribute__((noinline)) void
load_run(void * v, unsigned long long passes)
{
    dump_ctx * c = (dump_ctx *) v;
    unsigned long long p;
    ulong i, bad = 0;
    const adf_modctx_struct * b[1];
    size_t nb = (c->type == T_LOCAL || c->type == T_SCALED) ? 1 : 0;   /* one context occurrence */

    b[0] = c->ctx;
    for (p = 0; p < passes; p++)
        for (i = 0; i < c->n; i++)
        {
            int st;
            if (c->type == T_RAT)
                st = adf_rat_load_str_binds(c->r + i, c->text[i], c->len[i], b, 0, NULL);
            else if (c->type == T_FBALL || c->type == T_LOCAL)
                st = adf_fball_load_str_binds(c->f + i, c->text[i], c->len[i], b, nb, NULL);
            else if (c->type == T_ADELE)
                st = adf_adele_load_str_binds(c->a + i, c->text[i], c->len[i], b, 0, NULL);
            else
                st = adf_scaled_load_str_binds(c->s + i, c->text[i], c->len[i], b, nb, NULL);
            bad += (st != ADF_OK);
        }
    c->acc = bad;
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
    dump_ctx ctx;
} dump_case;

static void
fill(dump_case * wc, const char * name, const char * family, int type, int load, unsigned long long passes,
     flint_rand_t state, ulong bits, const adf_modctx_struct * ctx)
{
    ulong i;
    size_t total = 0;

    memset(wc, 0, sizeof(*wc));
    wc->name = name;
    wc->kind = BENCH_INDEPENDENT_BATCH;
    wc->operand_family = family;
    wc->fn = load ? load_run : dump_run;
    wc->calls = passes;
    wc->ctx.type = type;
    wc->ctx.ctx = ctx;
    wc->ctx.n = DB_BATCH_N;
    wc->ctx.text = (char **) calloc(DB_BATCH_N, sizeof(char *));
    wc->ctx.len = (size_t *) calloc(DB_BATCH_N, sizeof(size_t));
    wc->ctx.r = (adf_rat_struct *) malloc(DB_BATCH_N * sizeof(adf_rat_struct));
    wc->ctx.f = (adf_fball_struct *) malloc(DB_BATCH_N * sizeof(adf_fball_struct));
    wc->ctx.a = (adf_adele_struct *) malloc(DB_BATCH_N * sizeof(adf_adele_struct));
    wc->ctx.s = (adf_scaled_struct *) malloc(DB_BATCH_N * sizeof(adf_scaled_struct));
    for (i = 0; i < DB_BATCH_N; i++)
    {
        fmpz_t A, H, d, K;

        fmpz_init(A);
        fmpz_init(H);
        fmpz_init(d);
        fmpz_init(K);
        do
            fmpz_randbits(H, state, bits);
        while (fmpz_sgn(H) <= 0);
        fmpz_randm(A, state, H);
        do
            fmpz_randbits(d, state, bits);
        while (fmpz_sgn(d) <= 0);
        adf_rat_init(wc->ctx.r + i);
        adf_fball_init(wc->ctx.f + i);
        adf_adele_init(wc->ctx.a + i);
        fmpq_init(wc->ctx.s[i].s);
        fmpz_init(wc->ctx.s[i].u);
        wc->ctx.s[i].mctx = ctx;
        wc->ctx.s[i].exact = 1;
        if (type == T_RAT)
        {
            fmpq_set_fmpz_frac(wc->ctx.r[i].q, A, d);
            wc->ctx.text[i] = adf_rat_dump_str(wc->ctx.len + i, wc->ctx.r + i);
        }
        else if (type == T_FBALL)
        {
            (void) adf_fball_set_fmpz3(wc->ctx.f + i, A, H, d);
            wc->ctx.text[i] = adf_fball_dump_str(wc->ctx.len + i, wc->ctx.f + i);
        }
        else if (type == T_LOCAL)
        {
            /* a raw local value (conventions 5.3), fields written directly */
            slong j, k = adf_modctx_nblocks(ctx);
            adf_fball_struct * x = wc->ctx.f + i;
            fmpz_zero(x->A);
            adf_modctx_get_modulus(x->H, ctx);
            fmpz_set(x->d, d);
            x->res = flint_malloc((size_t) k * sizeof(ulong));
            for (j = 0; j < k; j++)
                x->res[j] = n_randlimb(state) % adf_modctx_block(ctx, j);
            x->mctx = ctx;
            x->backend = ADF_LOCAL;
            wc->ctx.text[i] = adf_fball_dump_str(wc->ctx.len + i, x);
        }
        else if (type == T_ADELE)
        {
            arb_randtest(wc->ctx.a[i].inf, state, 53, 8);
            (void) adf_fball_set_fmpz3(&wc->ctx.a[i].fin, A, H, d);
            wc->ctx.text[i] = adf_adele_dump_str(wc->ctx.len + i, wc->ctx.a + i);
        }
        else
        {
            adf_scaled_struct * x = wc->ctx.s + i;
            adf_modctx_get_modulus(K, ctx);
            fmpq_set_fmpz_frac(x->s, H, d);
            fmpq_abs(x->s, x->s);
            fmpz_randm(x->u, state, K);
            x->exact = 0;
            wc->ctx.text[i] = adf_scaled_dump_str(wc->ctx.len + i, x);
        }
        total += wc->ctx.len[i];
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
        fmpz_clear(K);
    }
    snprintf(wc->bit_lengths, sizeof(wc->bit_lengths), "operands up to %lu bit; mean text B = %lu bytes; n = %d",
             (unsigned long) bits, (unsigned long) (total / DB_BATCH_N), (int) DB_BATCH_N);
}

static void
free_case(dump_case * wc)
{
    ulong i;

    for (i = 0; i < wc->ctx.n; i++)
    {
        flint_free(wc->ctx.text[i]);
        adf_rat_clear(wc->ctx.r + i);
        adf_fball_clear(wc->ctx.f + i);
        adf_adele_clear(wc->ctx.a + i);
        fmpq_clear(wc->ctx.s[i].s);
        fmpz_clear(wc->ctx.s[i].u);
    }
    free(wc->ctx.text);
    free(wc->ctx.len);
    free(wc->ctx.r);
    free(wc->ctx.f);
    free(wc->ctx.a);
    free(wc->ctx.s);
}

/* ----------------------------------------------------------------- driver */

static unsigned long long g_checksum = 0;
static unsigned long long g_seed = 0;

static void
report_case(FILE * f, dump_case * wc, int trials)
{
    bench_stats st = bench_measure(wc->fn, &wc->ctx, wc->calls, DB_BATCH_N, trials, 1);
    unsigned long long ops = wc->calls * DB_BATCH_N;

    g_checksum ^= (unsigned long long) wc->ctx.acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n",
            wc->name, bench_kind_name(wc->kind), wc->operand_family, wc->bit_lengths,
            g_seed, "yes", "yes", wc->calls, ops, (unsigned long long) wc->ctx.acc,
            st.min, st.median, st.max);
    fflush(f);
    printf("%-26s %-17s min %10.1f  median %10.1f  max %10.1f ns/op  (%s)\n",
           wc->name, bench_kind_name(wc->kind), st.min, st.median, st.max, wc->bit_lengths);
}

static void
write_header(FILE * f, const char * run_id, const char * model, int requested_cpu,
             int actual_cpu, const bench_tsc_calib * cal, int trials)
{
    char gmp[64];
    snprintf(gmp, sizeof(gmp), "%d.%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
             __GNU_MP_VERSION_PATCHLEVEL);
    fprintf(f, "run_id: %s\n", run_id);
    fprintf(f, "tag: dump\n");
    fprintf(f, "provisional: yes (lane m1-dump; no floor derived beyond PERF 4: B bytes read or written)\n");
    fprintf(f, "quiet_machine: no\n");
    fprintf(f, "machine: %s\n", model);
    fprintf(f, "affinity_requested_cpu: %d\n", requested_cpu);
    fprintf(f, "affinity_actual_cpu: %d\n", actual_cpu);
    fprintf(f, "affinity_method: sched_setaffinity\n");
    fprintf(f, "compiler: gcc %s\n", __VERSION__);
    fprintf(f, "flags: %s\n", BENCH_CFLAGS);
    fprintf(f, "flint_version: %s\n", FLINT_VERSION);
    fprintf(f, "gmp_version: %s\n", gmp);
    fprintf(f, "clock_source: clock_gettime(CLOCK_MONOTONIC_RAW)\n");
    fprintf(f, "cycles_per_ns_median: %.5f\n", cal->cycles_per_ns_median);
    fprintf(f, "seed: %llu\n", g_seed);
    fprintf(f, "operand_family: dump texts of adf_rat, adf_fball (global, local 128 blocks), adf_adele, "
               "adf_scaled (128 blocks)\n");
    fprintf(f, "trials_per_case: %d\n", trials);
    fprintf(f, "batch_length: %d\n", DB_BATCH_N);
    fprintf(f, "warm: yes\n");
    fprintf(f, "allocating: yes (the dumped string, fmpz temporaries, the residue array)\n");
    fprintf(f, "\n");
    fprintf(f, "# one table follows; columns are tab separated\n");
    fprintf(f, "# name\tkind\toperand_family\tbit_lengths\tseed\twarm\t"
               "allocating\tcalls\tops\tchecksum\tns_per_op_min\t"
               "ns_per_op_median\tns_per_op_max\n");
}

int
main(int argc, char ** argv)
{
    char run_id[64];
    char model[BENCH_STR];
    bench_tsc_calib cal;
    dump_case cases[16];
    flint_rand_t state;
    adf_modctx_struct * c128 = NULL;
    ulong q[128], p = UWORD(1) << 40;
    int actual_cpu, nc = 0, i, cpu = 2, trials = 9;
    FILE * f;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--run") == 0)
            ;
        else if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc)
            cpu = atoi(argv[++i]);
        else if (strcmp(argv[i], "--trials") == 0 && i + 1 < argc)
            trials = atoi(argv[++i]);
        else
        {
            fprintf(stderr, "usage: %s [--run] [--cpu N] [--trials N]\n", argv[0]);
            return 2;
        }
    }
    g_seed = 20260928ULL;
    flint_randinit(state);
    flint_randseed(state, (ulong) g_seed, 0);
    for (i = 0; i < 128; i++)
        q[i] = p = n_nextprime(p, 1);                 /* 128 primes of 41 bits: K about 5250 bits */
    if (adf_modctx_new_blocks(&c128, q, 128) != ADF_OK)
        return 1;
    actual_cpu = bench_pin_cpu(cpu);
    if (actual_cpu < 0)
        actual_cpu = bench_get_cpu();
    bench_tsc_calibrate(&cal, 5, 0.02);

    fill(&cases[nc++], "rat_dump_word", "adf_rat_dump_str; A/d word sized", T_RAT, 0, 40, state, 60, NULL);
    fill(&cases[nc++], "rat_load_word", "adf_rat_load_str_binds; A/d word sized", T_RAT, 1, 40, state, 60, NULL);
    fill(&cases[nc++], "rat_dump_4096", "adf_rat_dump_str; A/d 4096 bit", T_RAT, 0, 4, state, 4096, NULL);
    fill(&cases[nc++], "rat_load_4096", "adf_rat_load_str_binds; A/d 4096 bit", T_RAT, 1, 4, state, 4096, NULL);
    fill(&cases[nc++], "fball_dump_4096", "adf_fball_dump_str; global (A,H,d) 4096 bit", T_FBALL, 0, 2, state,
         4096, NULL);
    fill(&cases[nc++], "fball_load_4096", "adf_fball_load_str_binds; global (A,H,d) 4096 bit", T_FBALL, 1, 2,
         state, 4096, NULL);
    fill(&cases[nc++], "fball_local_dump_128", "adf_fball_dump_str; local, 128 blocks, d 60 bit", T_LOCAL, 0, 2,
         state, 60, c128);
    fill(&cases[nc++], "fball_local_load_128", "adf_fball_load_str_binds; local, 128 blocks, d 60 bit", T_LOCAL,
         1, 2, state, 60, c128);
    fill(&cases[nc++], "adele_dump_word", "adf_adele_dump_str; arb 53 bit, fin word sized", T_ADELE, 0, 20,
         state, 60, NULL);
    fill(&cases[nc++], "adele_load_word", "adf_adele_load_str_binds; arb 53 bit, fin word sized", T_ADELE, 1, 20,
         state, 60, NULL);
    fill(&cases[nc++], "scaled_dump_128", "adf_scaled_dump_str; s 60 bit, K of 128 blocks", T_SCALED, 0, 2, state,
         60, c128);
    fill(&cases[nc++], "scaled_load_128", "adf_scaled_load_str_binds; s 60 bit, K of 128 blocks", T_SCALED, 1, 2,
         state, 60, c128);

    f = bench_results_open("dump", run_id, sizeof(run_id));
    if (f == NULL)
    {
        fprintf(stderr, "cannot open result file\n");
        return 1;
    }
    bench_cpu_model(model, sizeof(model));
    write_header(f, run_id, model, cpu, actual_cpu, &cal, trials);
    for (i = 0; i < nc; i++)
        report_case(f, &cases[i], trials);
    fprintf(f, "\n# checksum of all case results, computed outside the timed region\n");
    fprintf(f, "checksum: %llu\n", g_checksum);
    fclose(f);
    for (i = 0; i < nc; i++)
        free_case(&cases[i]);
    adf_modctx_free(c128);
    flint_randclear(state);
    printf("checksum %llu\n", g_checksum);
    return 0;
}
