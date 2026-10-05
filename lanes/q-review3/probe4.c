#include <adelefeld.h>
#include <stdio.h>
int main(void){
    mag_t m; fmpq_t q; mag_init(m); fmpq_init(q);
    mag_zero(m);
    MAG_MAN(m) = 536870913UL; fmpz_set_si(MAG_EXPREF(m), -31);
    printf("direct man=536870913 exp=-31: get_d=%.17g ", mag_get_d(m));
    mag_get_fmpq(q, m); printf("fmpq="); fmpq_print(q); printf("\n");
    mag_zero(m); MAG_MAN(m) = 1UL; fmpz_set_si(MAG_EXPREF(m), 0);
    printf("direct man=1 exp=0: get_d=%.17g ", mag_get_d(m));
    mag_get_fmpq(q, m); printf("fmpq="); fmpq_print(q); printf("\n");
    mag_zero(m); MAG_MAN(m) = 536870912UL; fmpz_set_si(MAG_EXPREF(m), 0);
    printf("direct man=2^29 exp=0: get_d=%.17g ", mag_get_d(m));
    mag_get_fmpq(q, m); printf("fmpq="); fmpq_print(q); printf("\n");
    printf("sizeof mag=%d sizeof fmpz=%d sizeof arf=%d\n",(int)sizeof(mag_struct),(int)sizeof(fmpzi_struct),(int)sizeof(arf_struct));
    { fmpz_t z; fmpz_init(z); fmpz_set_si(z, -31);
      printf("fmpz -31 prints: "); fmpz_print(z); printf("  raw=%ld %ld\n", (long)((long*)z)[0], (long)((long*)z)[1]); }
    return 0;
}
