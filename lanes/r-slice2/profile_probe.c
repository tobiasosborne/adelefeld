#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <flint/fmpz_vec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int adf_roots_real_isolate(arb_ptr cand, slong *m, const fmpz_poly_t g, slong prec);
int adf_roots_real_finish(adf_rootlist_t L, const fmpz_poly_t f, slong count,
                          arb_srcptr in, slong m, slong prec);

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static void factor(fmpz_poly_t f, const fmpz_t a, const fmpz_t b)
{
    fmpz_poly_t h;
    fmpz_poly_init(h);
    fmpz_poly_set_coeff_fmpz(h, 1, a);
    fmpz_poly_set_coeff_fmpz(h, 0, b);
    fmpz_poly_mul(f, f, h);
    fmpz_poly_clear(h);
}

static void family(fmpz_poly_t f, const char *name, slong d, slong e)
{
    fmpz_t a, b;
    fmpz_poly_t h, u, v;
    slong i;
    fmpz_init(a); fmpz_init(b);
    fmpz_poly_init(h); fmpz_poly_init(u); fmpz_poly_init(v);
    fmpz_poly_one(f);
    if (!strcmp(name, "sqrt2"))
    {
        fmpz_poly_zero(f); fmpz_poly_set_coeff_si(f, 2, 1); fmpz_poly_set_coeff_si(f, 0, -2);
    }
    else if (!strcmp(name, "cluster"))
    {
        fmpz_set_ui(a, 3);
        fmpz_one(b); fmpz_mul_2exp(b, b, e); fmpz_mul_ui(b, b, 3);
        fmpz_neg(b, b);
        for (i = 1; i <= d; i++) { fmpz_sub_ui(b, b, 1); factor(f, a, b); }
    }
    else if (!strcmp(name, "mignotte") || !strcmp(name, "positive"))
    {
        fmpz_one(a); fmpz_mul_2exp(a, a, e); fmpz_set_si(b, -1);
        fmpz_poly_one(h); factor(h, a, b);
        fmpz_poly_mul(f, h, h);
        fmpz_poly_scalar_mul_si(f, f, !strcmp(name, "positive") ? 2 : -2);
        fmpz_poly_set_coeff_si(f, d, 1);
    }
    else if (!strcmp(name, "complex"))
    {
        /* Product of distinct positive quadratics 2^(2e) (3X-1)^2 + i. */
        fmpz_one(a); fmpz_mul_2exp(a, a, 2 * e);
        for (i = 1; i <= d / 2; i++)
        {
            fmpz_add_ui(b, a, i); fmpz_poly_set_coeff_fmpz(h, 0, b);
            fmpz_mul_si(b, a, -6); fmpz_poly_set_coeff_fmpz(h, 1, b);
            fmpz_mul_ui(b, a, 9); fmpz_poly_set_coeff_fmpz(h, 2, b);
            fmpz_poly_mul(f, f, h);
        }
    }
    else if (!strcmp(name, "wilkinson"))
    {
        fmpz_one(a);
        for (i = 1; i <= d; i++) { fmpz_set_si(b, -i); factor(f, a, b); }
    }
    else if (!strcmp(name, "chebyshev"))
    {
        /* refs/src/flint-3.0.1/fmpz_poly.rst:3373-3375 defines T_n(cos t) = cos(n t).
           Adding u^(n+1) + u^(-n-1) and u^(n-1) + u^(-n+1), with u + u^-1 = 2x,
           gives the recurrence below and T_0 = 1, T_1 = x. */
        fmpz_poly_one(u); fmpz_poly_set_coeff_si(v, 1, 1);
        for (i = 2; i <= d; i++)
        {
            fmpz_poly_shift_left(h, v, 1); fmpz_poly_scalar_mul_si(h, h, 2);
            fmpz_poly_sub(h, h, u); fmpz_poly_swap(u, v); fmpz_poly_swap(v, h);
        }
        fmpz_poly_set(f, d == 0 ? u : v);
    }
    else if (!strcmp(name, "tiny"))
    {
        fmpz_one(a); fmpz_mul_2exp(a, a, e);
        for (i = 1; i <= d; i++) { fmpz_set_si(b, -i); factor(f, a, b); }
    }
    else if (!strcmp(name, "linear"))
    {
        fmpz_one(a); fmpz_mul_2exp(a, a, e); fmpz_set_si(b, -d);
        factor(f, a, b);
    }
    else { fprintf(stderr, "unknown family\n"); exit(2); }
    fmpz_clear(a); fmpz_clear(b);
    fmpz_poly_clear(h); fmpz_poly_clear(u); fmpz_poly_clear(v);
}

static int run(const fmpz_poly_t f, slong prec, int verbose, int verify)
{
    adf_rootlist_t L;
    fmpz_t a, b, e;
    slong i, exact = 0, minacc = WORD_MAX, maxbits = 0;
    int st, errors = 0;
    double t;
    adf_rootlist_init(L); fmpz_init(a); fmpz_init(b); fmpz_init(e);
    t = now(); st = adf_roots_real(L, f, prec); t = now() - t;
    if (!verbose)
    {
        printf("status=%d degree=%ld coeffbits=%ld seconds=%.9f n=%ld count=%ld\n", st,
               fmpz_poly_degree(f), FLINT_ABS(_fmpz_vec_max_bits(f->coeffs, f->length)),
               t, L->n, L->count);
        fflush(stdout);
    }
    else printf("%d %ld %.9f\n", st, L->n, t);
    if (st == ADF_OK)
    {
        errors += !adf_place_is_archimedean(L->place);
        errors += L->scope != ADF_ROOTLIST_PARTITION || L->complete != 1 || L->nu != 0;
        errors += L->count != L->n;
    }
    for (i = 0; st == ADF_OK && i < L->n; i++)
    {
        slong acc = arb_rel_accuracy_bits(L->ball + i);
        exact += arb_is_exact(L->ball + i);
        if (acc < minacc) minacc = acc;
        if (!arb_is_exact(L->ball + i) && acc < FLINT_MAX(prec, 2)) errors++;
        arb_get_interval_fmpz_2exp(a, b, e, L->ball + i);
        if ((slong) fmpz_bits(a) > maxbits) maxbits = fmpz_bits(a);
        if ((slong) fmpz_bits(b) > maxbits) maxbits = fmpz_bits(b);
        if (verbose)
        {
            fmpz_print(a); putchar(' '); fmpz_print(b); putchar(' '); fmpz_print(e); putchar('\n');
        }
    }
    if (verify && st == ADF_OK)
    {
        errors += !adf_rootlist_is_canonical(L);
        errors += !adf_rootlist_verify_entries(L, f);
        errors += !adf_rootlist_verify_complete(L, f, 0);
    }
    if (!verbose) printf("exact=%ld minacc=%ld endpointbits=%ld errors=%d\n", exact, minacc, maxbits, errors);
    fflush(stdout);
    adf_rootlist_clear(L); fmpz_clear(a); fmpz_clear(b); fmpz_clear(e);
    return errors ? 1 : 0;
}

static void stages(const fmpz_poly_t f, slong prec)
{
    fmpz_poly_t g, h, df;
    adf_rootlist_t L;
    arb_ptr cand;
    slong d, count, m = 0;
    int st;
    double t;
    fmpz_poly_init(g); fmpz_poly_init(h); fmpz_poly_init(df); adf_rootlist_init(L);
    t = now();
    fmpz_poly_derivative(df, f); fmpz_poly_gcd(h, f, df); fmpz_poly_div(g, f, h);
    fmpz_poly_primitive_part(g, g);
    if (fmpz_sgn(g->coeffs + fmpz_poly_degree(g)) < 0) fmpz_poly_neg(g, g);
    d = fmpz_poly_degree(g);
    printf("normalise_s=%.9f gcd_degree=%ld degree=%ld\n", now() - t, fmpz_poly_degree(h), d);
    fflush(stdout);
    t = now(); count = fmpz_poly_num_real_roots(g);
    printf("count_s=%.9f count=%ld\n", now() - t, count); fflush(stdout);
    cand = _arb_vec_init(d); t = now(); st = adf_roots_real_isolate(cand, &m, g, prec);
    printf("isolate_refine_s=%.9f status=%d m=%ld\n", now() - t, st, m); fflush(stdout);
    if (st == ADF_OK)
    {
        t = now(); st = adf_roots_real_finish(L, f, count, cand, m, prec);
        printf("finish_with_normalise_s=%.9f status=%d\n", now() - t, st); fflush(stdout);
    }
    _arb_vec_clear(cand, d); adf_rootlist_clear(L);
    fmpz_poly_clear(g); fmpz_poly_clear(h); fmpz_poly_clear(df);
}

int main(int argc, char **argv)
{
    fmpz_poly_t f;
    fmpz_t z;
    int result = 0;
    fmpz_poly_init(f); fmpz_init(z);
    if (argc >= 5)
    {
        slong d = atol(argv[2]), e = atol(argv[3]), prec = atol(argv[4]);
        family(f, argv[1], d, e);
        if (argc == 6 && !strcmp(argv[5], "stages")) stages(f, prec);
        else result = run(f, prec, 0, argc == 5);
    }
    else
    {
        char *line = NULL;
        size_t cap = 0;
        while (getline(&line, &cap, stdin) > 0)
        {
            char *p = strtok(line, " \n");
            slong i = 0, prec;
            if (!p) continue;
            prec = atol(p); fmpz_poly_zero(f);
            while ((p = strtok(NULL, " \n")) != NULL)
            {
                if (fmpz_set_str(z, p, 10)) return 2;
                fmpz_poly_set_coeff_fmpz(f, i++, z);
            }
            result |= run(f, prec, 1, 1);
        }
        free(line);
    }
    fmpz_clear(z); fmpz_poly_clear(f); flint_cleanup();
    return result;
}
