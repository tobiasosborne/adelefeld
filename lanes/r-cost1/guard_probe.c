/* Focused red checks for the sampled overflow and minimum-bound survivors. */
#include "roots_guard_probe.c"
#include <stdio.h>

int main(void)
{
    fmpz_poly_t h;
    slong K, product = 123;
    int failures = 0, ok;
    fmpz_poly_init(h);
    fmpz_poly_set_coeff_si(h, 1, 1); fmpz_poly_set_coeff_si(h, 0, -1);
    ok = root_bound_exp(&K, h);
    failures += !ok || K != 1;
    printf("X - 1 bound: returned %d, K %ld, expected 1\n", ok, (long) K);
    ok = mul_checked_nonneg(&product, WORD_MAX, 2);
    failures += ok != 0;
    printf("WORD_MAX times 2: returned %d, expected refusal\n", ok);
    if (ok == 0)
    {
        fmpz_poly_set_coeff_si(h, 1, 0); fmpz_poly_set_coeff_si(h, 2, 1);
        ok = bound_holds(h, WORD_MAX);
        failures += ok != -1;
        printf("degree-two shift overflow: returned %d, expected -1\n", ok);
    }
    fmpz_poly_clear(h); flint_cleanup();
    printf("%d failed checks\n", failures);
    return failures != 0;
}
