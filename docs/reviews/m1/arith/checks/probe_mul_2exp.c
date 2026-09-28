/* Probe: what fmpz_mul_2exp(t, 1, s) gives for a shift s near 2^62 and 2^63, the shifts that
   recon.c:95 and :106 pass for an arb exponent in [2^62, 2^64). */
#include <stdio.h>
#include <flint/fmpz.h>

int
main(void)
{
    ulong s[] = { UWORD(1) << 62, (UWORD(1) << 62) + 1, UWORD(1) << 63, (UWORD(1) << 63) + 5 };
    int i;
    for (i = 0; i < 4; i++)
    {
        fmpz_t t, one;
        fmpz_init(t);
        fmpz_init(one);
        fmpz_one(one);
        fmpz_mul_2exp(t, one, s[i]);
        printf("shift %lu: bits of result = %lu, result = ", (unsigned long) s[i],
               (unsigned long) fmpz_bits(t));
        if (fmpz_bits(t) < 200) fmpz_print(t); else printf("(large)");
        printf("\n");
        fmpz_clear(t);
        fmpz_clear(one);
    }
    return 0;
}
