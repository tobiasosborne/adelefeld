/* stdin one line: prec c0 ... cd. Times: num_real_roots(f) (f made squarefree primitive by gcd) and adf_roots_real */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
int main(int argc, char **argv)
{
    static char buf[1 << 24];
    char *tok, *sp; slong prec, i = 0, c; fmpz_poly_t f; adf_rootlist_t L; double t; int s;
    (void) argc; (void) argv;
    if (!fgets(buf, sizeof buf, stdin)) return 1;
    fmpz_poly_init(f); adf_rootlist_init(L);
    tok = strtok_r(buf, " \n", &sp); prec = atol(tok);
    while ((tok = strtok_r(NULL, " \n", &sp)))
    { fmpz_t x; fmpz_init(x); fmpz_set_str(x, tok, 10); fmpz_poly_set_coeff_fmpz(f, i++, x); fmpz_clear(x); }
    { fmpz_poly_t g, df; fmpz_poly_init(g); fmpz_poly_init(df); t = now(); fmpz_poly_derivative(df, f); fmpz_poly_gcd(g, f, df); printf("gcd(f,f') %.2f\n", now() - t); }
    t = now(); c = fmpz_poly_num_real_roots(f); printf("plain num_real_roots %.2f -> %ld\n", now() - t, (long) c);
    t = now(); s = adf_roots_real(L, f, prec); printf("adf_roots_real %.2f status %d n %ld\n", now() - t, s, (long) L->n);
    return 0;
}
