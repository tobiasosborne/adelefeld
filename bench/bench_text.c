/* bench/bench_text.c: benchmark rows of work package 1.4 (docs/PLAN.md section 6, row 1.4: "print,
   parse"): the value form of adf_rat, adf_fball and adf_adele, printed and parsed, for word-sized
   and for 4096-bit values, each as an independent batch. Contract: docs/PERF.md section 7.
   Provisional (brief of lane m1-text): no floor is derived here, the rows are measurements only.
   Build and run: make -C bench run (the rows appear in bench/results/<utc>_text.txt).

   A print row times adf_x_get_str and the flint_free of its string; a parse row times
   adf_x_set_str on the texts printed before the timed region. Both allocate. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>

#include <adelefeld.h>

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

#define TX_BATCH_N 256

enum { T_RAT = 0, T_FBALL = 1, T_ADELE = 2 };

typedef struct
{
    int type;
    adf_rat_struct * r;
    adf_fball_struct * f;
    adf_adele_struct * a;
    char ** text;
    size_t * len;
    ulong n;
    ulong acc;           /* read only after the timed region */
} text_ctx;

/* ---------------------------------------------------------------- kernels */

static __attribute__((noinline)) void
print_run(void * v, unsigned long long passes)
{
    text_ctx * c = (text_ctx *) v;
    unsigned long long p;
    ulong i;
    size_t len, total = 0;
    char * s;

    for (p = 0; p < passes; p++)
        for (i = 0; i < c->n; i++)
        {
            if (c->type == T_RAT)
                s = adf_rat_get_str(&len, c->r + i);
            else if (c->type == T_FBALL)
                s = adf_fball_get_str(&len, c->f + i);
            else
                s = adf_adele_get_str(&len, c->a + i, ADF_DIGITS_DEFAULT);
            total += len;
            flint_free(s);
        }
    c->acc = (ulong) total;
}

static __attribute__((noinline)) void
parse_run(void * v, unsigned long long passes)
{
    text_ctx * c = (text_ctx *) v;
    unsigned long long p;
    ulong i, bad = 0;

    for (p = 0; p < passes; p++)
        for (i = 0; i < c->n; i++)
        {
            int st;
            if (c->type == T_RAT)
                st = adf_rat_set_str(c->r + i, c->text[i], c->len[i], NULL);
            else if (c->type == T_FBALL)
                st = adf_fball_set_str(c->f + i, c->text[i], c->len[i], NULL);
            else
                st = adf_adele_set_str(c->a + i, c->text[i], c->len[i], 128, NULL);
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
    text_ctx ctx;
} text_case;

static void
fill(text_case * wc, const char * name, const char * family, int type, int parse, unsigned long long passes,
     flint_rand_t state, ulong bits)
{
    ulong i;
    size_t total = 0;

    memset(wc, 0, sizeof(*wc));
    wc->name = name;
    wc->kind = BENCH_INDEPENDENT_BATCH;
    wc->operand_family = family;
    wc->fn = parse ? parse_run : print_run;
    wc->calls = passes;
    wc->ctx.type = type;
    wc->ctx.n = TX_BATCH_N;
    wc->ctx.text = (char **) calloc(TX_BATCH_N, sizeof(char *));
    wc->ctx.len = (size_t *) calloc(TX_BATCH_N, sizeof(size_t));
    if (type == T_RAT)
        wc->ctx.r = (adf_rat_struct *) malloc(TX_BATCH_N * sizeof(adf_rat_struct));
    else if (type == T_FBALL)
        wc->ctx.f = (adf_fball_struct *) malloc(TX_BATCH_N * sizeof(adf_fball_struct));
    else
        wc->ctx.a = (adf_adele_struct *) malloc(TX_BATCH_N * sizeof(adf_adele_struct));
    for (i = 0; i < TX_BATCH_N; i++)
    {
        fmpz_t A, H, d;

        fmpz_init(A);
        fmpz_init(H);
        fmpz_init(d);
        do
            fmpz_randbits(H, state, bits);
        while (fmpz_sgn(H) <= 0);
        fmpz_randm(A, state, H);
        do
            fmpz_randbits(d, state, bits);
        while (fmpz_sgn(d) <= 0);
        if (type == T_RAT)
        {
            adf_rat_init(wc->ctx.r + i);
            fmpq_set_fmpz_frac(wc->ctx.r[i].q, A, d);
            wc->ctx.text[i] = adf_rat_get_str(wc->ctx.len + i, wc->ctx.r + i);
        }
        else if (type == T_FBALL)
        {
            adf_fball_init(wc->ctx.f + i);
            (void) adf_fball_set_fmpz3(wc->ctx.f + i, A, H, d);
            wc->ctx.text[i] = adf_fball_get_str(wc->ctx.len + i, wc->ctx.f + i);
        }
        else
        {
            arb_init(wc->ctx.a[i].inf);
            adf_fball_init(&wc->ctx.a[i].fin);
            /* a real part of 53-bit midpoint and a 30-bit radius, the finite part as for fball */
            arb_randtest(wc->ctx.a[i].inf, state, 53, 8);
            (void) adf_fball_set_fmpz3(&wc->ctx.a[i].fin, A, H, d);
            wc->ctx.text[i] = adf_adele_get_str(wc->ctx.len + i, wc->ctx.a + i, ADF_DIGITS_DEFAULT);
        }
        total += wc->ctx.len[i];
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
    }
    snprintf(wc->bit_lengths, sizeof(wc->bit_lengths),
             "A,H,d up to %lu bit; mean text %lu bytes; n = %d", (unsigned long) bits,
             (unsigned long) (total / TX_BATCH_N), (int) TX_BATCH_N);
}

static void
free_case(text_case * wc)
{
    ulong i;

    for (i = 0; i < wc->ctx.n; i++)
    {
        flint_free(wc->ctx.text[i]);
        if (wc->ctx.type == T_RAT)
            adf_rat_clear(wc->ctx.r + i);
        else if (wc->ctx.type == T_FBALL)
            adf_fball_clear(wc->ctx.f + i);
        else
        {
            arb_clear(wc->ctx.a[i].inf);
            adf_fball_clear(&wc->ctx.a[i].fin);
        }
    }
    free(wc->ctx.text);
    free(wc->ctx.len);
    free(wc->ctx.r);
    free(wc->ctx.f);
    free(wc->ctx.a);
}

/* ----------------------------------------------------------------- driver */

static unsigned long long g_checksum = 0;
static unsigned long long g_seed = 0;

static void
report_case(FILE * f, text_case * wc, int trials)
{
    bench_stats st = bench_measure(wc->fn, &wc->ctx, wc->calls, TX_BATCH_N, trials, 1);
    unsigned long long ops = wc->calls * TX_BATCH_N;

    g_checksum ^= (unsigned long long) wc->ctx.acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n",
            wc->name, bench_kind_name(wc->kind), wc->operand_family, wc->bit_lengths,
            g_seed, "yes", "yes", wc->calls, ops, (unsigned long long) wc->ctx.acc,
            st.min, st.median, st.max);
    fflush(f);
    printf("%-22s %-17s min %10.1f  median %10.1f  max %10.1f ns/op\n",
           wc->name, bench_kind_name(wc->kind), st.min, st.median, st.max);
}

static void
write_header(FILE * f, const char * run_id, const char * model, int requested_cpu,
             int actual_cpu, const bench_tsc_calib * cal, int trials)
{
    char gmp[64];
    snprintf(gmp, sizeof(gmp), "%d.%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
             __GNU_MP_VERSION_PATCHLEVEL);
    fprintf(f, "run_id: %s\n", run_id);
    fprintf(f, "tag: text\n");
    fprintf(f, "provisional: yes (lane m1-text; no floor derived)\n");
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
    fprintf(f, "operand_family: value-form texts of adf_rat, adf_fball, adf_adele (prec 128, digits 20)\n");
    fprintf(f, "trials_per_case: %d\n", trials);
    fprintf(f, "batch_length: %d\n", TX_BATCH_N);
    fprintf(f, "warm: yes\n");
    fprintf(f, "allocating: yes (the printed string, fmpz and fmpq temporaries)\n");
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
    text_case cases[12];
    flint_rand_t state;
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
    actual_cpu = bench_pin_cpu(cpu);
    if (actual_cpu < 0)
        actual_cpu = bench_get_cpu();
    bench_tsc_calibrate(&cal, 5, 0.02);

    fill(&cases[nc++], "rat_print_word", "adf_rat_get_str; A/d, A,d word sized", T_RAT, 0, 40, state, 60);
    fill(&cases[nc++], "rat_parse_word", "adf_rat_set_str; A/d, A,d word sized", T_RAT, 1, 40, state, 60);
    fill(&cases[nc++], "rat_print_4096", "adf_rat_get_str; A/d, A,d 4096 bit", T_RAT, 0, 2, state, 4096);
    fill(&cases[nc++], "rat_parse_4096", "adf_rat_set_str; A/d, A,d 4096 bit", T_RAT, 1, 2, state, 4096);
    fill(&cases[nc++], "fball_print_word", "adf_fball_get_str; (A,H,d) word sized", T_FBALL, 0, 20, state, 60);
    fill(&cases[nc++], "fball_parse_word", "adf_fball_set_str; (A,H,d) word sized", T_FBALL, 1, 20, state, 60);
    fill(&cases[nc++], "fball_print_4096", "adf_fball_get_str; (A,H,d) 4096 bit", T_FBALL, 0, 1, state, 4096);
    fill(&cases[nc++], "fball_parse_4096", "adf_fball_set_str; (A,H,d) 4096 bit", T_FBALL, 1, 1, state, 4096);
    fill(&cases[nc++], "adele_print_word", "adf_adele_get_str; arb 53 bit, fin word sized", T_ADELE, 0, 4, state,
         60);
    fill(&cases[nc++], "adele_parse_word", "adf_adele_set_str at prec 128; fin word sized", T_ADELE, 1, 4, state,
         60);

    f = bench_results_open("text", run_id, sizeof(run_id));
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
    flint_randclear(state);
    printf("checksum %llu\n", g_checksum);
    return 0;
}
