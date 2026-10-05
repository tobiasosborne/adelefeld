#include <adelefeld.h>
#include <stdio.h>
int main(void){
    mag_t m; mag_init(m);
    mag_zero(m); MAG_MAN(m) = 536870913UL; fmpz_set_si(MAG_EXPREF(m), -31);
    printf("raw slong at EXPREF = %ld, fmpz_print says ", (long)(*(long*) MAG_EXPREF(m)));
    fmpz_print(MAG_EXPREF(m)); printf("\n");
    printf("MAG_EXP(si) = %ld\n", (long)(*(long*) MAG_EXPREF(m)));
    { arf_t a; arf_init(a); arf_set_si(a, -31);
      printf("arf raw exp = %ld\n", (long)(*(long*) ARF_EXPREF(a))); }
    return 0;
}
