/* probe.c (lane f-review4): the same input through four copies of the code and through FLINT.
   Variants, linked into this one program under renamed public symbols (build.sh):
     new: src/lfunc.c of the worktree (the library archive)             adf_lball_log / adf_lball_Log
     old: src/lfunc.c at commit 1cf0e42 (git show; the single-route code) old_lball_log / old_lball_Log
     sum: src/lfunc.c with the route forced to log_sum (F8)              sum_lball_log / sum_lball_Log
     bal: src/lfunc.c with the route forced to log_balanced (F9)         bal_lball_log / bal_lball_Log
   Input line: name p unit_num unit_den v M exact N flint mask
     (raw fields of adf_lball; flint = 1 asks for FLINT's padic value of f at the centre, at precision K of new;
     mask: bit 0 runs old, bit 1 runs sum, bit 2 runs bal; a variant not run prints "- - - - - -").
   Output line: for each variant "st exact v N num den", then "t_old t_new" (seconds), then
     "alias" (1 if new with y = x gives the same as new), then "fl_ok fl_num fl_den" or "- - -".
   FLINT: refs/src/flint-3.0.1/padic.rst:490-497 (padic_log), :551-559 (padic_teichmuller). */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <flint/padic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int old_lball_log(adf_lball_t, const adf_lball_t, slong);
int old_lball_Log(adf_lball_t, const adf_lball_t, slong);
int sum_lball_log(adf_lball_t, const adf_lball_t, slong);
int sum_lball_Log(adf_lball_t, const adf_lball_t, slong);
int bal_lball_log(adf_lball_t, const adf_lball_t, slong);
int bal_lball_Log(adf_lball_t, const adf_lball_t, slong);

typedef int (*fn_t)(adf_lball_t, const adf_lball_t, slong);

static double seconds(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static int same(const adf_lball_t a, const adf_lball_t b)
{
    return a->p == b->p && a->v == b->v && a->N == b->N && a->exact == b->exact && fmpq_equal(a->u, b->u);
}

static void print_ball(int st, const adf_lball_t y)
{
    printf("%d %d %ld %ld ", st, y->exact, (long) y->v, (long) y->N);
    fmpz_print(fmpq_numref(y->u)); putchar(' '); fmpz_print(fmpq_denref(y->u)); putchar(' ');
}

/* FLINT's value of log (which = 1) or Log (which = 2) at the rational x, modulo p^n; 0 if FLINT refuses. */
static int flint_value(fmpq_t out, int which, ulong p, const fmpq_t x, slong n)
{
    padic_ctx_t ctx;
    padic_t X, Y, W;
    fmpz_t P;
    int ok;
    fmpz_init_set_ui(P, p);
    padic_ctx_init(ctx, P, 0, 0, PADIC_SERIES);
    padic_init2(X, n + 20); padic_init2(W, n + 20); padic_init2(Y, n);
    padic_set_fmpq(X, x, ctx);
    if (which == 2)
    {
        padic_val(X) = 0;
        if (p == 2)
        {
            if (fmpz_fdiv_ui(padic_unit(X), 4) == 3)
                padic_neg(X, X, ctx);
        }
        else
        {
            padic_teichmuller(W, X, ctx);
            padic_div(X, X, W, ctx);
        }
    }
    else if (p == 2 && fmpz_fdiv_ui(padic_unit(X), 4) == 3 && padic_val(X) == 0)
        padic_neg(X, X, ctx);                     /* log(x) = log(-x) at 2 (Proposition 11, step 2) */
    ok = padic_log(Y, X, ctx);
    if (ok)
        padic_get_fmpq(out, Y, ctx);
    padic_clear(X); padic_clear(Y); padic_clear(W);
    padic_ctx_clear(ctx);
    fmpz_clear(P);
    return ok;
}

int main(void)
{
    static char name[8], un[400000], ud[400000];
    ulong p;
    slong v, M, N;
    int exact, want_flint, mask, count = 0, rc;
    adf_lball_t x, y, z, w, t, s;
    fmpq_t centre, fl;
    fmpz_t pv;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(w); adf_lball_init(t); adf_lball_init(s);
    fmpq_init(centre); fmpq_init(fl); fmpz_init(pv);
    while ((rc = scanf("%7s %lu %399999s %399999s %ld %ld %d %ld %d %d", name, &p, un, ud, &v, &M, &exact, &N, &want_flint, &mask)) == 10)
    {
        fn_t fnew, fold, fsum, fbal;
        int which = strcmp(name, "log") == 0 ? 1 : 2;
        int st_new, st_old, st_sum, st_bal, st_al, alias;
        double t0, t_old, t_new;
        if (which == 1) { fnew = adf_lball_log; fold = old_lball_log; fsum = sum_lball_log; fbal = bal_lball_log; }
        else            { fnew = adf_lball_Log; fold = old_lball_Log; fsum = sum_lball_Log; fbal = bal_lball_Log; }
        x->p = p; x->v = v; x->N = M; x->exact = exact;
        if (fmpz_set_str(fmpq_numref(x->u), un, 10) || fmpz_set_str(fmpq_denref(x->u), ud, 10))
            return 2;
        if (!adf_lball_is_canonical(x))
        {
            fprintf(stderr, "noncanonical input %d\n", count);
            return 3;
        }
        t_old = -1; st_old = st_sum = st_bal = -1;
        if (mask & 1) { t0 = seconds(); st_old = fold(y, x, N); t_old = seconds() - t0; }
        t0 = seconds(); st_new = fnew(z, x, N); t_new = seconds() - t0;
        if (mask & 2) st_sum = fsum(w, x, N);
        if (mask & 4) st_bal = fbal(t, x, N);
        adf_lball_set(s, x);
        st_al = fnew(s, s, N);
        alias = st_al == st_new && same(s, st_new == ADF_OK ? z : x);
        print_ball(st_new, z);
        if (mask & 1) print_ball(st_old, y); else printf("- - - - - - ");
        if (mask & 2) print_ball(st_sum, w); else printf("- - - - - - ");
        if (mask & 4) print_ball(st_bal, t); else printf("- - - - - - ");
        printf("%.9f %.9f %d ", t_old, t_new, alias);
        if (want_flint && st_new == ADF_OK && !z->exact)
        {
            /* the centre p^v u as a rational */
            fmpq_set(centre, x->u);
            fmpz_set_ui(pv, p);
            fmpz_pow_ui(pv, pv, (ulong) (v < 0 ? -v : v));
            if (v >= 0) fmpz_mul(fmpq_numref(centre), fmpq_numref(centre), pv);
            else fmpz_mul(fmpq_denref(centre), fmpq_denref(centre), pv);
            fmpq_canonicalise(centre);
            if (flint_value(fl, which, p, centre, z->N))
            {
                printf("1 "); fmpz_print(fmpq_numref(fl)); putchar(' '); fmpz_print(fmpq_denref(fl));
            }
            else
                printf("0 0 1");
        }
        else
            printf("- - -");
        putchar('\n');
        fflush(stdout);
        count++;
    }
    if (rc != EOF) { fprintf(stderr, "probe: input line %d could not be read (scanf %d)\n", count, rc); return 4; }
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(w); adf_lball_clear(t); adf_lball_clear(s);
    fmpq_clear(centre); fmpq_clear(fl); fmpz_clear(pv);
    flint_cleanup();
    return 0;
}
