/* stdin: prec c0..cd. Times only the isolation+refinement (count passed in as argv[1]) and real_finish via adf_roots_real_finish */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int adf_roots_real_isolate_counted(arb_ptr cand, slong * m, const fmpz_poly_t g, slong prec, slong count);
int adf_roots_real_finish(adf_rootlist_t L, const fmpz_poly_t f, slong count, arb_srcptr in, slong m, slong prec);
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
int main(int argc, char **argv)
{
    static char buf[1 << 24];
    char *tok, *sp; slong prec, i = 0, m, count = atol(argv[1]); fmpz_poly_t f; adf_rootlist_t L; double t; int s;
    arb_ptr cand;
    (void) argc;
    if (!fgets(buf, sizeof buf, stdin)) return 1;
    fmpz_poly_init(f); adf_rootlist_init(L);
    tok = strtok_r(buf, " \n", &sp); prec = atol(tok);
    if (argc > 2) prec = atol(argv[2]);
    while ((tok = strtok_r(NULL, " \n", &sp)))
    { fmpz_t x; fmpz_init(x); fmpz_set_str(x, tok, 10); fmpz_poly_set_coeff_fmpz(f, i++, x); fmpz_clear(x); }
    cand = _arb_vec_init(i);
    t = now(); s = adf_roots_real_isolate_counted(cand, &m, f, prec, count);
    printf("isolate+refine %.2f status %d m %ld\n", now() - t, s, (long) m);
    if (s == 0)
    { t = now(); s = adf_roots_real_finish(L, f, count, cand, m, prec); printf("finish %.2f status %d\n", now() - t, s); }
    return 0;
}
