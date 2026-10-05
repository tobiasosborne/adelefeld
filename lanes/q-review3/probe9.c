#include <adelefeld.h>
#include <stdio.h>
int main(void){
    fmpq_t d, num, den; arf_t u, A, B; fmpz_t mant, exp; ulong v; mag_t z; slong prec = 30;
    fmpq_init(d); fmpq_init(num); fmpq_init(den); arf_init(u); arf_init(A); arf_init(B);
    fmpz_init(mant); fmpz_init(exp); mag_init(z);
    fmpq_set_si(d, 1, 4);                      /* d = 1/4 */
    arf_set_fmpq(A, d, 2000000, ARF_RND_NEAR);
    arf_set_ui(B, 1);
    arf_div(u, A, B, prec, ARF_RND_CEIL);
    arf_get_fmpq(num, u);
    printf("u = "); fmpq_print(num);
    printf("  ARF_EXPREF(u)="); fmpz_print(ARF_EXPREF(u)); printf("\n");
    arf_get_fmpz_2exp(mant, exp, u);
    printf("mant="); fmpz_print(mant); printf(" exp="); fmpz_print(exp);
    printf(" bits=%d\n", (int)fmpz_bits(mant));
    fmpz_mul_2exp(mant, mant, prec - (int)fmpz_bits(mant));
    v = fmpz_get_ui(mant) + 1;
    printf("v=%lu\n", v);
    mag_zero(z);
    fmpz_set(MAG_EXPREF(z), ARF_EXPREF(u));
    if (v == (UWORD(1) << 30)) { v >>= 1; fmpz_add_ui(MAG_EXPREF(z), MAG_EXPREF(z), 1); }
    MAG_MAN(z) = v;
    { fmpq_t r; fmpq_init(r); mag_get_fmpq(r, z); printf("stored rho = "); fmpq_print(r);
      printf("   get_d=%.17g\n", mag_get_d(z)); fmpq_clear(r); }
    printf("expect rho = (2^29+1)*2^-31 = 268435457/1073741824? no: 0.25+2^-31\n");
    printf("field convention check: man*2^(exp-30) = %.17g\n",
           ldexp((double)v, (long)MAG_EXP(z) - 30));
    return 0;
}
