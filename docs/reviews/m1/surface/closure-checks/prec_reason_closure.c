/* Counterexample to the parenthetical reason for the prec cap in SPEC M1-D1.
   Compute 2^50000 / 3 at prec 100001 directly through the public library interface. */
#include <adelefeld.h>
#include <flint/mag.h>
#include <stdio.h>

int
main(void)
{
    adf_adele_t x, z;
    adf_rat_t three;
    char * s;
    size_t len = 0;
    int status, printable;

    adf_adele_init(x);
    adf_adele_init(z);
    adf_rat_init(three);
    adf_adele_set_si(x, 1);
    arb_mul_2exp_si(x->inf, x->inf, 50000);
    adf_rat_set_si(three, 3);
    status = adf_adele_div_rat(z, x, three, 100001);
    s = status == ADF_OK ? adf_adele_get_str(&len, z, 20) : NULL;
    printable = s != NULL;
    printf("status=%d radius_exp=%ld printable=%d length=%zu\n", status,
           (long) fmpz_get_si(MAG_EXPREF(arb_radref(z->inf))), printable, len);
    adf_str_free(s);
    adf_rat_clear(three);
    adf_adele_clear(x);
    adf_adele_clear(z);
    return status == ADF_OK && printable ? 0 : 1;
}
