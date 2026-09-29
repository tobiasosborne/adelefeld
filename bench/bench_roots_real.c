/* bench_roots_real: where the time of adf_roots_real goes (issue adf-8di, docs/design/real-roots.md).

   A call rate benchmark in the sense of docs/PERF.md section 7: one call on one input, wall time by
   CLOCK_MONOTONIC_RAW, allocating, no affinity set by the program (run it under `taskset -c 2`).
   It measures FLINT 3.0.1 and the library; it changes nothing.

   Build (from the root of the repository, after `make`):
     cc -std=c11 -O2 -g -Wall -Wextra -Iinclude bench/bench_roots_real.c build/libadelefeld.a \
        -lflint -lgmp -lm -o build/bench_roots_real
   Without the library: add -DNO_ADF and leave out build/libadelefeld.a (mode adf is then refused).

   Usage: bench_roots_real MODE FAMILY e d prec
     MODE    flint   arb_fmpz_poly_complex_roots(g, VERBOSE, prec): FLINT prints one line for each working
                     precision (complex_roots.c:96 to 104); then the total time and the sizes of the balls
             trace   the loop of complex_roots.c:91 to 109 and of find_roots.c:112 to 141 done here with the
                     same calls, one line for each working precision: iterations, time, log2 of the largest
                     midpoint, the correction, the number of isolated roots. Stops at the first precision
                     with all roots isolated and accurate to prec, or above 2^22 bits.
             count   fmpz_poly_num_real_roots(g)
             eval    one exact sign of g at the dyadic point m 2^-k nearest to the largest root bound, k = prec
                     (the unit of the lower bound), by the homogeneous Horner rule of src/roots.c:1262 to 1298
             adf     adf_roots_real(L, g, prec)
     FAMILY  pair    1234567 (X - 2^e)(X - 2^e - 1)                       d is ignored
             far     (X - 2^e)(X - 3 2^e)                                  large roots far apart
             close   (X - 1)(2^e X - 2^e - 1): roots 1 and 1 + 2^-e        small roots close together
             deg     prod_(i = 1..d) (X - 2^e - i)                         degree d, roots near 2^e
             small   prod_(i = 1..d) (X - i)                               degree d, small roots (e ignored)
   Every polynomial is squarefree with real roots only, so the count is d (2 for the first three). */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_poly.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/acb_poly.h>
#include <flint/arb_fmpz_poly.h>
#ifndef NO_ADF
#include "adelefeld/roots.h"
#endif

/* defined in acb_poly/find_roots.c:47 to 70, exported by the library, not declared in acb_poly.h */
void _acb_poly_roots_initial_values(acb_ptr roots, slong deg, slong prec);

static double
now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return (double) t.tv_sec + 1e-9 * (double) t.tv_nsec;
}

/* g = g (a X - b) */
static void
mul_linear(fmpz_poly_t g, const fmpz_t a, const fmpz_t b)
{
    fmpz_poly_t l;
    fmpz_poly_init(l);
    fmpz_poly_set_coeff_fmpz(l, 1, a);
    fmpz_poly_set_coeff_fmpz(l, 0, b);
    fmpz_neg(l->coeffs, l->coeffs);
    fmpz_poly_mul(g, g, l);
    fmpz_poly_clear(l);
}

static int
family(fmpz_poly_t g, const char * name, ulong e, slong d)
{
    fmpz_t a, b;
    slong i;
    int ok = 1;

    fmpz_init(a);
    fmpz_init(b);
    fmpz_poly_one(g);
    fmpz_one(a);
    if (!strcmp(name, "pair"))
    {
        fmpz_poly_scalar_mul_ui(g, g, 1234567);
        fmpz_one(b); fmpz_mul_2exp(b, b, e); mul_linear(g, a, b);
        fmpz_add_ui(b, b, 1); mul_linear(g, a, b);
    }
    else if (!strcmp(name, "far"))
    {
        fmpz_one(b); fmpz_mul_2exp(b, b, e); mul_linear(g, a, b);
        fmpz_mul_ui(b, b, 3); mul_linear(g, a, b);
    }
    else if (!strcmp(name, "close"))
    {
        fmpz_one(b); mul_linear(g, a, b);
        fmpz_mul_2exp(a, a, e); fmpz_add_ui(b, a, 1); mul_linear(g, a, b);
    }
    else if (!strcmp(name, "deg") || !strcmp(name, "small"))
    {
        for (i = 1; i <= d; i++)
        {
            fmpz_zero(b);
            if (name[0] == 'd')
                fmpz_setbit(b, e);
            fmpz_add_ui(b, b, (ulong) i);
            mul_linear(g, a, b);
        }
    }
    else
        ok = 0;
    fmpz_clear(a);
    fmpz_clear(b);
    return ok;
}

static slong
mid_mag(const acb_t z)      /* find_roots.c:17 to 25 */
{
    slong rm = arf_abs_bound_lt_2exp_si(arb_midref(acb_realref(z)));
    slong im = arf_abs_bound_lt_2exp_si(arb_midref(acb_imagref(z)));
    return FLINT_MAX(rm, im);
}

static slong
rad_mag(const acb_t z)      /* find_roots.c:27 to 45 */
{
    arf_t t;
    slong rm, im;
    arf_init(t);
    arf_set_mag(t, arb_radref(acb_realref(z)));
    rm = arf_abs_bound_lt_2exp_si(t);
    arf_set_mag(t, arb_radref(acb_imagref(z)));
    im = arf_abs_bound_lt_2exp_si(t);
    arf_clear(t);
    return FLINT_MAX(rm, im);
}

/* the loops of complex_roots.c:91 to 109 and find_roots.c:104 to 143, with the same calls, for a polynomial
   without deflation and with a nonzero constant term (true for every family here) */
static void
trace(const fmpz_poly_t g, slong target)
{
    slong deg = fmpz_poly_degree(g), prec, iter, maxiter, i, isolated = 0, total_iter = 0;
    slong rootmag, max_rootmag = 0, correction, max_correction = 0, acc;
    acb_poly_t c;
    acb_ptr z = _acb_vec_init(deg);
    double t0, t1, t2, sum = 0;

    acb_poly_init(c);
    printf("# prec iterations total_iterations log2_max_midpoint correction isolated min_accuracy "
           "seconds_iterate seconds_validate\n");
    for (prec = 32; prec <= (WORD(1) << 22); prec *= 2)
    {
        acb_poly_set_fmpz_poly(c, g, prec);
        maxiter = FLINT_MIN(4 * deg + 64, prec);                 /* complex_roots.c:94 */
        t0 = now();
        if (prec == 32)
            _acb_poly_roots_initial_values(z, deg, prec);        /* find_roots.c:104 to 105 */
        for (iter = 0; iter < maxiter; iter++)                   /* find_roots.c:112 to 141 */
        {
            max_rootmag = -ARF_PREC_EXACT;
            for (i = 0; i < deg; i++)
            {
                rootmag = mid_mag(z + i);
                max_rootmag = FLINT_MAX(rootmag, max_rootmag);
            }
            _acb_poly_refine_roots_durand_kerner(z, c->coeffs, deg + 1, prec);
            max_correction = -ARF_PREC_EXACT;
            for (i = 0; i < deg; i++)
            {
                correction = rad_mag(z + i);
                max_correction = FLINT_MAX(correction, max_correction);
            }
            max_correction -= max_rootmag;
            if (max_correction < -prec / 2)
                maxiter = FLINT_MIN(maxiter, iter + 2);
            else if (max_correction < -prec / 3)
                maxiter = FLINT_MIN(maxiter, iter + 3);
            else if (max_correction < -prec / 4)
                maxiter = FLINT_MIN(maxiter, iter + 4);
        }
        total_iter += iter;
        t1 = now();
        isolated = _acb_poly_validate_roots(z, c->coeffs, deg + 1, prec);   /* find_roots.c:143 */
        t2 = now();
        acc = WORD_MAX;
        for (i = 0; i < deg; i++)
            acc = FLINT_MIN(acc, acb_rel_accuracy_bits(z + i));
        sum += t2 - t0;
        printf("%ld %ld %ld %ld %ld %ld %ld %.6f %.6f\n", (long) prec, (long) iter, (long) total_iter,
               (long) max_rootmag, (long) max_correction, (long) isolated, (long) acc, t1 - t0, t2 - t1);
        fflush(stdout);
        if (isolated == deg && acc >= target)                    /* complex_roots.c:111 to 114 */
            break;
    }
    printf("trace: total %.6f s, last prec %ld, iterations %ld\n", sum, (long) prec, (long) total_iter);
    _acb_vec_clear(z, deg);
    acb_poly_clear(c);
}

/* the sign of g at m 2^-k, as src/roots.c:1262 to 1298 does it for a negative exponent */
static int
sign_at(const fmpz_poly_t g, const fmpz_t m, ulong k)
{
    fmpz_t r, t;
    slong i, d = fmpz_poly_degree(g);
    int s;

    fmpz_init(r);
    fmpz_init(t);
    fmpz_set(r, g->coeffs + d);
    for (i = d - 1; i >= 0; i--)
    {
        fmpz_mul(r, r, m);
        fmpz_mul_2exp(t, g->coeffs + i, k * (ulong) (d - i));
        fmpz_add(r, r, t);
    }
    s = fmpz_sgn(r);
    fmpz_clear(r);
    fmpz_clear(t);
    return s;
}

int
main(int argc, char ** argv)
{
    fmpz_poly_t g;
    ulong e;
    slong d, prec, i, deg;
    double t0, t1;

    if (argc != 6)
    {
        fprintf(stderr, "usage: %s flint|trace|count|eval|adf pair|far|close|deg|small e d prec\n", argv[0]);
        return 2;
    }
    e = strtoul(argv[3], NULL, 10);
    d = strtol(argv[4], NULL, 10);
    prec = strtol(argv[5], NULL, 10);
    fmpz_poly_init(g);
    if (!family(g, argv[2], e, d))
    {
        fprintf(stderr, "unknown family %s\n", argv[2]);
        return 2;
    }
    deg = fmpz_poly_degree(g);
    printf("family %s e %lu degree %ld prec %ld coefficient bits %ld\n", argv[2], e, (long) deg, (long) prec,
           (long) FLINT_ABS(fmpz_poly_max_bits(g)));
    fflush(stdout);
    if (!strcmp(argv[1], "flint"))
    {
        acb_ptr z = _acb_vec_init(deg);
        slong bits = 0, acc = WORD_MAX, real = 0;
        t0 = now();
        arb_fmpz_poly_complex_roots(z, g, ARB_FMPZ_POLY_ROOTS_VERBOSE, prec);
        t1 = now();
        for (i = 0; i < deg; i++)
        {
            bits = FLINT_MAX(bits, acb_bits(z + i));
            acc = FLINT_MIN(acc, acb_rel_accuracy_bits(z + i));
            real += arb_is_zero(acb_imagref(z + i));
        }
        printf("flint: %.6f s, real %ld, largest mantissa %ld bits, least accuracy %ld bits\n", t1 - t0,
               (long) real, (long) bits, (long) acc);
        _acb_vec_clear(z, deg);
    }
    else if (!strcmp(argv[1], "trace"))
        trace(g, prec);
    else if (!strcmp(argv[1], "count"))
    {
        slong n;
        t0 = now();
        n = fmpz_poly_num_real_roots(g);
        t1 = now();
        printf("count: %.6f s, n = %ld\n", t1 - t0, (long) n);
    }
    else if (!strcmp(argv[1], "eval"))
    {
        fmpz_t m;
        int s = 0;
        slong rep = 0;
        fmpz_init(m);
        fmpz_poly_bound_roots(m, g);
        fmpz_mul_2exp(m, m, (ulong) prec);
        fmpz_add_ui(m, m, 1);
        t0 = now();
        do
        {
            s += sign_at(g, m, (ulong) prec);
            rep++;
            t1 = now();
        }
        while (t1 - t0 < 0.2);
        printf("eval: %.9f s for one sign (%ld repetitions), point of %ld bits, sign sum %d\n",
               (t1 - t0) / (double) rep, (long) rep, (long) fmpz_bits(m), s);
        fmpz_clear(m);
    }
    else if (!strcmp(argv[1], "adf"))
    {
#ifndef NO_ADF
        adf_rootlist_t L;
        arb_t x;
        slong bits = 0;
        int st;
        adf_rootlist_init(L);
        arb_init(x);
        t0 = now();
        st = adf_roots_real(L, g, prec);
        t1 = now();
        for (i = 0; adf_rootlist_get_arb(x, L, i); i++)
            bits = FLINT_MAX(bits, arb_bits(x));
        printf("adf: %.6f s, status %d, balls %ld, largest mantissa %ld bits\n", t1 - t0, st, (long) i,
               (long) bits);
        arb_clear(x);
        adf_rootlist_clear(L);
#else
        fprintf(stderr, "built with NO_ADF\n");
        return 2;
#endif
    }
    else
    {
        fprintf(stderr, "unknown mode %s\n", argv[1]);
        return 2;
    }
    fmpz_poly_clear(g);
    flint_cleanup();
    return 0;
}
