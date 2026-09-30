/* certificate attack driver. stdin per line: prec count m  (a b e)*m  ncoef c0..cd
   candidate ball i = [a,b] 2^e (as midpoint/radius, exact). Calls adf_roots_real_finish; prints status and, on OK,
   the stored balls' endpoints. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int adf_roots_real_finish(adf_rootlist_t L, const fmpz_poly_t f, slong count, arb_srcptr in, slong m, slong prec);
static char buf[1 << 22];
int main(void)
{
    while (fgets(buf, sizeof buf, stdin))
    {
        char *tok, *sp = NULL; slong prec, count, m, i, nc; arb_ptr in; fmpz_poly_t f; adf_rootlist_t L; int st;
        fmpz_t a, b, e, mid, rad;
        tok = strtok_r(buf, " \n", &sp); prec = atol(tok);
        count = atol(strtok_r(NULL, " \n", &sp)); m = atol(strtok_r(NULL, " \n", &sp));
        in = _arb_vec_init(m > 0 ? m : 1);
        fmpz_init(a); fmpz_init(b); fmpz_init(e); fmpz_init(mid); fmpz_init(rad);
        for (i = 0; i < m; i++)
        {
            arf_t x;
            fmpz_set_str(a, strtok_r(NULL, " \n", &sp), 10); fmpz_set_str(b, strtok_r(NULL, " \n", &sp), 10);
            fmpz_set_str(e, strtok_r(NULL, " \n", &sp), 10);
            /* [a,b] 2^e : mid = (a+b)/2, rad = (b-a)/2 ; use exponent e-1 : mid = (a+b) 2^(e-1) */
            fmpz_add(mid, a, b); fmpz_sub(rad, b, a);
            arf_init(x);
            fmpz_sub_ui(e, e, 1);
            arf_set_fmpz_2exp(arb_midref(in + i), mid, e);
            arf_set_fmpz_2exp(x, rad, e);
            arf_get_mag(arb_radref(in + i), x);       /* rad is a power of two or small integer times: exact if <= 30 bits */
            arf_clear(x);
        }
        nc = atol(strtok_r(NULL, " \n", &sp));
        fmpz_poly_init(f); adf_rootlist_init(L);
        for (i = 0; i < nc; i++)
        { fmpz_set_str(a, strtok_r(NULL, " \n", &sp), 10); fmpz_poly_set_coeff_fmpz(f, i, a); }
        st = adf_roots_real_finish(L, f, count, in, m, prec);
        printf("%d", st);
        if (st == 0)
            for (i = 0; i < L->n; i++)
            {
                arb_get_interval_fmpz_2exp(a, b, e, L->ball + i);
                printf(" "); fmpz_print(a); printf(" "); fmpz_print(b); printf(" "); fmpz_print(e);
            }
        printf("\n"); fflush(stdout);
        adf_rootlist_clear(L); fmpz_poly_clear(f); _arb_vec_clear(in, m > 0 ? m : 1);
        fmpz_clear(a); fmpz_clear(b); fmpz_clear(e); fmpz_clear(mid); fmpz_clear(rad);
    }
    flint_cleanup_master();
    return 0;
}
