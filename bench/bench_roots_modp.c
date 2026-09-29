/* bench/bench_roots_modp.c: the two routes of the roots modulo p (milestone S, S.2, slice 4, lane s2-slice4;
   decision S-D10, docs/proofs/solvers.md Proposition 3.7): evaluation at every residue (route 1) and the
   degree route (route 2: d = gcd(h, X^p - X), the candidates of nmod_poly_roots, each tested). The
   crossover sets ADF_ROOTS_P_EVAL_MAX (include/adelefeld/roots.h).

   For each prime p (the largest prime <= 2^b, b = 4, 6 to 10, 12 to 20 in steps of 2), each degree 2, 8, 32 and
   two families of polynomials over F_p (random: a monic polynomial with uniform coefficients, about one root
   on average; split: deg distinct roots, the most work for the splitting of route 2, when p >= deg), the
   time of one call of the hidden function adf_roots_modp (src/roots.c) with the route forced, averaged
   over a batch of 16 polynomials (kind: independent batch). Every call of a batch returns a number of roots
   that is summed into a checksum outside the timed region; the two routes must give the same sum (else the
   run stops). min, median, max over the trials, and for each p the geometric mean over the cases of the
   ratio of the medians (above 1: route 2 is faster). The machine is shared with other lanes: see the header
   of the result file.

   Build and run: make -C bench, then ./bench/bench_roots_modp --run [--trials N] [--cpu N] from the
   repository root; the table appears on stdout and in bench/results/<utc>_roots_modp.txt. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/nmod_poly.h>
#include <flint/ulong_extras.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

/* hidden in src/roots.c */
slong adf_roots_modp(ulong * roots, const nmod_poly_t h, int route);

#define NPOLY 16
#define MAXDEG 32

typedef struct
{
    nmod_poly_struct h[NPOLY];
    ulong roots[MAXDEG + 1];
    int route;
    unsigned long long acc;
} modp_ctx;

static void
modp_run(void * vctx, unsigned long long calls)
{
    modp_ctx * c = (modp_ctx *) vctx;
    unsigned long long i;
    int j;

    for (i = 0; i < calls; i++)
        for (j = 0; j < NPOLY; j++)
            c->acc += (unsigned long long) adf_roots_modp(c->roots, c->h + j, c->route);
}

static ulong
prime_at_most(ulong n)
{
    while (!n_is_prime(n))
        n--;
    return n;
}

static void
fill(modp_ctx * c, ulong p, slong deg, int split, flint_rand_t st)
{
    nmod_poly_t t;
    ulong r[MAXDEG];
    slong i, j, k;
    int fresh;

    nmod_poly_init(t, p);
    for (j = 0; j < NPOLY; j++)
    {
        nmod_poly_init(c->h + j, p);
        if (!split)
        {
            for (i = 0; i < deg; i++)
                nmod_poly_set_coeff_ui(c->h + j, i, n_randint(st, p));
            nmod_poly_set_coeff_ui(c->h + j, deg, 1);
            continue;
        }
        for (i = 0; i < deg; i++)
            do
            {
                r[i] = n_randint(st, p);
                for (k = 0, fresh = 1; k < i; k++)
                    fresh = fresh && r[k] != r[i];
            }
            while (!fresh);
        nmod_poly_set_coeff_ui(c->h + j, 0, 1);
        for (i = 0; i < deg; i++)
        {
            nmod_poly_zero(t);
            nmod_poly_set_coeff_ui(t, 1, 1);
            nmod_poly_set_coeff_ui(t, 0, nmod_neg(r[i], t->mod));
            nmod_poly_mul(c->h + j, c->h + j, t);
        }
    }
    nmod_poly_clear(t);
}

static void
unfill(modp_ctx * c)
{
    int j;

    for (j = 0; j < NPOLY; j++)
        nmod_poly_clear(c->h + j);
}

/* calls per sample so that one sample takes about 0.03 s */
static unsigned long long
calibrate(modp_ctx * c)
{
    unsigned long long calls = 1;
    double t0, t;

    for (;;)
    {
        t0 = bench_now_raw();
        modp_run(c, calls);
        t = bench_now_raw() - t0;
        if (t > 0.03 || calls >= (1ULL << 30))
            return calls;
        calls *= t < 0.003 ? 10 : 2;
    }
}

static int
run(int cpu, int trials)
{
    static const int bexp[11] = { 4, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20 };
    static const slong degs[3] = { 2, 8, 32 };
    char run_id[64], model[BENCH_STR];
    flint_rand_t st;
    modp_ctx c;
    bench_stats s[2];
    unsigned long long acc[2], sum = 0;
    double lsum;
    int actual_cpu, ib, id, split, route, ncase;
    ulong p;
    FILE * f;

    actual_cpu = bench_pin_cpu(cpu);
    if (actual_cpu < 0)
    {
        fprintf(stderr, "warning: sched_setaffinity(%d) failed; running unpinned\n", cpu);
        actual_cpu = bench_get_cpu();
    }
    f = bench_results_open("roots_modp", run_id, sizeof(run_id));
    if (f == NULL)
    {
        fprintf(stderr, "cannot open result file\n");
        return 1;
    }
    bench_cpu_model(model, sizeof(model));
    fprintf(f, "# bench_roots_modp, run %s\n# cpu model: %s; pinned to cpu %d (requested %d)\n", run_id, model,
            actual_cpu, cpu);
    fprintf(f, "# compiler flags: %s; FLINT %s; kind: %s; trials %d; batch of %d polynomials\n", BENCH_CFLAGS,
            FLINT_VERSION, bench_kind_name(BENCH_INDEPENDENT_BATCH), trials, NPOLY);
    fprintf(f, "# clock: CLOCK_MONOTONIC_RAW; the machine is shared with other lanes (not idle)\n");
    fprintf(f, "# time of one call in microseconds: min / median / max over the trials\n");
    fprintf(f, "%-9s %-4s %-6s %-30s %-30s %s\n", "p", "deg", "family", "route 1 (evaluation)", "route 2 (gcd)",
            "median 1 / median 2");
    printf("%-9s %-4s %-6s %-30s %-30s %s\n", "p", "deg", "family", "route 1 (evaluation)", "route 2 (gcd)",
           "median 1 / median 2");
    flint_randinit(st);
    for (ib = 0; ib < 11; ib++)
    {
        p = prime_at_most(UWORD(1) << bexp[ib]);
        lsum = 0;
        ncase = 0;
        for (id = 0; id < 3; id++)
            for (split = 0; split <= 1; split++)
            {
                if (split && (ulong) degs[id] > p)
                    continue;
                fill(&c, p, degs[id], split, st);
                for (route = 1; route <= 2; route++)
                {
                    c.route = route;
                    c.acc = 0;
                    s[route - 1] = bench_measure(modp_run, &c, calibrate(&c), NPOLY, trials, 1);
                    acc[route - 1] = c.acc;
                }
                /* the same inputs give the same numbers of roots per call for both routes */
                c.acc = 0;
                c.route = 1;
                modp_run(&c, 1);
                acc[0] = c.acc;
                c.acc = 0;
                c.route = 2;
                modp_run(&c, 1);
                acc[1] = c.acc;
                if (acc[0] != acc[1])
                {
                    fprintf(stderr, "p = %lu, deg %ld: the routes disagree (%llu, %llu roots)\n",
                            (unsigned long) p, (long) degs[id], acc[0], acc[1]);
                    return 1;
                }
                sum += acc[0];
                fprintf(f, "%-9lu %-4ld %-6s %9.2f / %9.2f / %9.2f %9.2f / %9.2f / %9.2f %8.3f\n",
                        (unsigned long) p, (long) degs[id], split ? "split" : "random", s[0].min / 1e3,
                        s[0].median / 1e3, s[0].max / 1e3, s[1].min / 1e3, s[1].median / 1e3, s[1].max / 1e3,
                        s[0].median / s[1].median);
                printf("%-9lu %-4ld %-6s %9.2f / %9.2f / %9.2f %9.2f / %9.2f / %9.2f %8.3f\n",
                       (unsigned long) p, (long) degs[id], split ? "split" : "random", s[0].min / 1e3,
                       s[0].median / 1e3, s[0].max / 1e3, s[1].min / 1e3, s[1].median / 1e3, s[1].max / 1e3,
                       s[0].median / s[1].median);
                fflush(stdout);
                lsum += log(s[0].median / s[1].median);
                ncase++;
                unfill(&c);
            }
        fprintf(f, "%-9lu geometric mean of median 1 / median 2 over %d cases: %.3f\n", (unsigned long) p, ncase,
                exp(lsum / ncase));
        printf("%-9lu geometric mean of median 1 / median 2 over %d cases: %.3f\n", (unsigned long) p, ncase,
               exp(lsum / ncase));
    }
    fprintf(f, "# checksum (roots of one batch, summed): %llu\n", sum);
    fclose(f);
    flint_randclear(st);
    printf("checksum %llu\n", sum);
    return 0;
}

int
main(int argc, char ** argv)
{
    int i, cpu = 2, trials = 7;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--run") == 0)
            ;
        else if (strcmp(argv[i], "--trials") == 0 && i + 1 < argc)
            trials = atoi(argv[++i]);
        else if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc)
            cpu = atoi(argv[++i]);
        else
        {
            fprintf(stderr, "usage: %s [--run] [--trials N] [--cpu N]\n", argv[0]);
            return 2;
        }
    }
    if (trials < 1 || trials > BENCH_MAX_TRIALS)
        trials = 7;
    return run(cpu, trials);
}
