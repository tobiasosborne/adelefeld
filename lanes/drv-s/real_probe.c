/* lanes/drv-s/real_probe.c: the exact balls that adf_roots_real returns, for the check of the
   one line of tests/driver/s-realroots.cmd that depends on which isolating ball FLINT returns.
   The probe is not part of the library and not a test: it writes the exact dyadic midpoint and
   radius of every ball of the list, and the script lanes/drv-s/real_check.py applies the
   algorithm of conventions 9.5 to them and checks the ball against the root.

   Build from the repository root:
     make all && cc -Iinclude -std=c11 -O2 -g lanes/drv-s/real_probe.c build/libadelefeld.a \
         -lflint -lgmp -lm -o build/real_probe */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <flint/fmpz.h>
#include <flint/arb.h>
#include <adelefeld.h>

/* print "a 2^e" of an fmpz a and an exponent e */
static void
put_dyadic(const char * tag, const fmpz_t a, const fmpz_t e)
{
    char * sa = fmpz_get_str(NULL, 10, a);
    char * se = fmpz_get_str(NULL, 10, e);

    printf("%s %s 2^%s\n", tag, sa, se);
    flint_free(sa);
    flint_free(se);
}

int
main(int argc, char ** argv)
{
    fmpz_poly_t f;
    adf_rootlist_t L;
    arb_struct x[1], m[1], r[1];
    fmpz_t c, num, den;
    slong prec = 64, i, n, k;
    int status;

    if (argc < 2)
    {
        fprintf(stderr, "usage: real_probe PREC COEFF ...\n");
        return 2;
    }
    prec = strtol(argv[1], NULL, 10);
    fmpz_poly_init(f);
    fmpz_init(c);
    fmpz_init(num);
    fmpz_init(den);
    for (k = 2; k < argc; k++)
    {
        if (fmpz_set_str(c, argv[k], 10) != 0)
        {
            fprintf(stderr, "real_probe: %s is not an integer\n", argv[k]);
            return 2;
        }
        fmpz_poly_set_coeff_fmpz(f, k - 2, c);
    }
    adf_rootlist_init(L);
    status = adf_roots_real(L, f, prec);
    printf("status %d\n", status);
    if (status == ADF_OK)
    {
        n = adf_rootlist_length(L);
        flint_printf("n %wd\n", n);
        arb_init(x);
        arb_init(m);
        arb_init(r);
        for (i = 0; i < n; i++)
        {
            (void) adf_rootlist_get_arb(x, L, i);
            arb_get_mid_arb(m, x);
            arb_get_rad_arb(r, x);
            flint_printf("ball %wd\n", i);
            arb_get_interval_fmpz_2exp(num, den, c, x);
            put_dyadic("lo", num, c);
            arb_get_interval_fmpz_2exp(num, den, c, m);
            put_dyadic("mid", num, c);
            arb_get_interval_fmpz_2exp(num, den, c, r);
            put_dyadic("rad", num, c);
        }
        arb_clear(x);
        arb_clear(m);
        arb_clear(r);
    }
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_clear(c);
    fmpz_clear(num);
    fmpz_clear(den);
    return status == ADF_OK ? 0 : 1;
}
