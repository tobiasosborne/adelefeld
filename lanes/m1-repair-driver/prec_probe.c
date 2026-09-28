/* lanes/m1-repair-driver/prec_probe.c: the binary exponent of the midpoint (ARF_EXP) and of
   the radius (MAG_EXP) of a real result rounded at prec p.  Ground truth for R8.
   Build: cc -Iinclude -std=c11 -O1 lanes/m1-repair-driver/prec_probe.c build/libadelefeld.a
          -lflint -lgmp -lm -o build/prec_probe */
#include <stdio.h>
#include <flint/arf.h>
#include <flint/mag.h>
#include <adelefeld.h>

static void
show_exponents(const char * what, slong p, adf_adele_t y)
{
    arb_t z;

    char * sm, * sr, * v;
    slong em, er;

    arb_init(z);
    arb_set(z, y->inf);
    em = fmpz_get_si(ARF_EXPREF(arb_midref(z)));
    er = fmpz_get_si(MAG_EXPREF(arb_radref(z)));
    v = arf_get_str(arb_midref(z), 30);
    sm = fmpz_get_str(NULL, 10, ARF_EXPREF(arb_midref(z)));
    sr = fmpz_get_str(NULL, 10, MAG_EXPREF(arb_radref(z)));
    printf("%s prec %6ld mid_exp %12s rad_exp %12s rad_zero %d mid %s\n", what, (long) p, sm, sr,
           (int) mag_is_zero(arb_radref(z)), v);
    flint_free(v); flint_free(sm); flint_free(sr);
    arb_clear(z);
    (void) em; (void) er;
}

int main(void)
{
    slong p;
    fmpq_t t;
    adf_rat_t q;
    adf_adele_t x, y;

    fmpq_init(t);
    adf_rat_init(q);
    adf_adele_init(x);
    adf_adele_init(y);
    adf_rat_set_si(q, 1);
    adf_adele_set_rat(x, q, 64);
    fmpq_set_si(t, 3, 1);
    if (adf_rat_set_fmpq(q, t) != ADF_OK) { printf("set_fmpq failed\n"); return 1; }
    for (p = 1; p <= 6; p++)
    {
        int st = adf_adele_div_rat(y, x, q, p);
        if (st != ADF_OK) { printf("div_rat prec %ld: status %d\n", (long) p, st); break; }
        show_exponents("div(1;0)/3  ", p, y);
    }
    for (p = 99998; p <= 100003; p++)
    {
        int st = adf_adele_div_rat(y, x, q, p);
        if (st != ADF_OK) { printf("div_rat prec %ld: status %d\n", (long) p, st); break; }
        show_exponents("div(1;0)/3  ", p, y);
    }
    fmpq_clear(t);
    adf_adele_clear(x); adf_adele_clear(y); adf_rat_clear(q);
    return 0;
}
