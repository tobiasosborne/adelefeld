#include <adelefeld.h>
#include <stdio.h>
int main(void){
    mag_t lo, hi; arb_t a; char *s;
    mag_init(lo); mag_init(hi); arb_init(a);
    mag_set_ui_2exp_si(lo, 1, 0); mag_set_ui_2exp_si(hi, 3, 0);
    arb_set_interval_mag(a, lo, hi, 200);
    s = arb_get_str(a, 20, 0); printf("interval [1,3] printed: %s\n", s); flint_free(s);
    /* now a direct field write meaning "value = man * 2^exp" per the library's intent */
    mag_zero(lo); MAG_MAN(lo) = 536870913UL; fmpz_set_si(MAG_EXPREF(lo), -31);
    mag_zero(hi); MAG_MAN(hi) = 536870914UL; fmpz_set_si(MAG_EXPREF(hi), -31);
    arb_set_interval_mag(a, lo, hi, 200);
    s = arb_get_str(a, 20, 0); printf("direct man=2^29+1 exp=-31: %s\n", s); flint_free(s);
    return 0;
}
