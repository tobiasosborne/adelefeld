/* stdin lines: prec c0 c1 ... cd (decimal). stdout per line: status count n verE verC time then a b e per ball */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int main(void)
{
    static char *buf; size_t cap = 1 << 26;
    buf = malloc(cap);
    while (fgets(buf, cap, stdin))
    {
        char *tok, *sp; slong prec; fmpz_poly_t f; adf_rootlist_t L; int st, i = 0;
        struct timespec t0, t1; double dt;
        fmpz_poly_init(f); adf_rootlist_init(L);
        tok = strtok_r(buf, " \n", &sp); if (!tok) continue;
        prec = atol(tok);
        while ((tok = strtok_r(NULL, " \n", &sp)))
        { fmpz_t c; fmpz_init(c); fmpz_set_str(c, tok, 10); fmpz_poly_set_coeff_fmpz(f, i++, c); fmpz_clear(c); }
        clock_gettime(CLOCK_MONOTONIC, &t0);
        st = adf_roots_real(L, f, prec);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        dt = (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec);
        if (st == ADF_OK)
        {
            slong j;
            printf("%d %ld %ld %d %d %.3f", st, (long) L->count, (long) L->n,
                   adf_rootlist_verify_entries(L, f), adf_rootlist_verify_complete(L, f, 0), dt);
            for (j = 0; j < L->n; j++)
            {
                fmpz_t a, b, e; fmpz_init(a); fmpz_init(b); fmpz_init(e);
                arb_get_interval_fmpz_2exp(a, b, e, L->ball + j);
                printf(" "); fmpz_print(a); printf(" "); fmpz_print(b); printf(" "); fmpz_print(e);
                fmpz_clear(a); fmpz_clear(b); fmpz_clear(e);
            }
            printf("\n");
        }
        else printf("%d -1 -1 -1 -1 %.3f\n", st, dt);
        fflush(stdout);
        adf_rootlist_clear(L); fmpz_poly_clear(f);
    }
    flint_cleanup_master();
    return 0;
}
