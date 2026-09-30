/* usage: prof d bits seed : random dense poly; time gcd(f,f'), num_real_roots, then adf_roots_real */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
int main(int argc, char **argv)
{
    slong d = atol(argv[1]), bits = atol(argv[2]), i;
    flint_rand_t st; fmpz_poly_t f, g, df; adf_rootlist_t L; double t; int s;
    flint_randinit(st); (void) argv[3];
    fmpz_poly_init(f); fmpz_poly_init(g); fmpz_poly_init(df); adf_rootlist_init(L);
    fmpz_poly_randtest(f, st, d + 1, bits);
    fmpz_poly_set_coeff_si(f, d, 3);
    t = now(); fmpz_poly_derivative(df, f); fmpz_poly_gcd(g, f, df); printf("gcd %.2f deg %ld\n", now() - t, (long) fmpz_poly_degree(g));
    t = now(); slong c = fmpz_poly_num_real_roots(f); printf("count %.2f -> %ld\n", now() - t, (long) c);
    t = now(); s = adf_roots_real(L, f, 2); printf("adf_roots_real %.2f status %d n %ld\n", now() - t, s, (long) L->n);
    (void) i;
    return 0;
}
