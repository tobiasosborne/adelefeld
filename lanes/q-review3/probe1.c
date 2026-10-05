#include <adelefeld.h>
#include <stdio.h>
int main(void){
    arb_t a; fmpz_t mm,me; arf_t r; fmpq_t q; arb_init(a); fmpz_init(mm); fmpz_init(me); arf_init(r); fmpq_init(q);
    fmpz_set_si(mm,536870913); fmpz_set_si(me,-31); arf_set_fmpz_2exp(r, mm, me);
    arf_set(arb_midref(a), r);
    arf_get_mag(arb_radref(a), r);
    mag_get_fmpq(q, arb_radref(a));
    printf("mag_get_fmpq: "); fmpq_print(q); printf("\n");
    printf("man=%lu exp=", MAG_MAN(arb_radref(a))); fmpz_print(MAG_EXPREF(arb_radref(a))); printf("\n");
    printf("%s\n", arb_get_str(a, 10, 0));
    return 0;
}
