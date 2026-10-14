/* Only the unchanged certificate, with independently generated root candidates.
   arb_root_ui: refs/src/flint-3.0.1/arb.rst:979-991 (enclosure of the n-th root).
   Acceptance still uses roots.c's exact endpoint tests, solvers P3.8. */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int adf_roots_real_finish(adf_rootlist_t L, const fmpz_poly_t f, slong count,
                          arb_srcptr in, slong m, slong prec);

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

int main(int argc, char ** argv)
{
    slong prec = argc > 1 ? atol(argv[1]) : 2097152;
    fmpz_poly_t f;
    adf_rootlist_t L;
    arb_ptr b = _arb_vec_init(2);
    double t;
    int st;
    fmpz_poly_init(f); adf_rootlist_init(L);
    fmpz_poly_set_coeff_si(f, 50, 1); fmpz_poly_set_coeff_si(f, 0, -2);
    arb_set_ui(b + 1, 2);
    arb_root_ui(b + 1, b + 1, 50, prec + 64);
    arb_neg(b, b + 1);
    printf("candidate bits %ld accuracy %ld\n", (long) arb_bits(b),
           (long) arb_rel_accuracy_bits(b));
    fflush(stdout);
    t = now();
    st = adf_roots_real_finish(L, f, 2, b, 2, prec);
    printf("certificate %.6f s status %d roots %ld\n", now() - t, st, (long) L->n);
    _arb_vec_clear(b, 2); adf_rootlist_clear(L); fmpz_poly_clear(f);
    flint_cleanup();
    return st != ADF_OK;
}
