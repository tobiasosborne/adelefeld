/* Certify membership independently with scalar arb endpoints, not factor evaluation. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static char tok[8][16384], kk[4096];
    ulong p;
    int expected;
    acb_t s;
    arb_t pole, a;
    fmpz_t man, ex, k;
    acb_init(s); arb_init(pole); arb_init(a);
    fmpz_init(man); fmpz_init(ex); fmpz_init(k);
    unsigned cases = 0, members = 0, disjoint = 0, failures = 0;
    while (scanf("%lu %4095s %d", &p, kk, &expected) == 3) {
        for (int j = 0; j < 8; j++) if (scanf("%16383s", tok[j]) != 1) abort();
        acb_zero(s);
        for (int j = 0; j < 4; j++) {
            if (fmpz_set_str(man, tok[2*j], 10) || fmpz_set_str(ex, tok[2*j+1], 10)) abort();
            if (j < 2) arf_set_fmpz_2exp(arb_midref(j ? acb_imagref(s) : acb_realref(s)), man, ex);
            else {
                mag_ptr r = arb_radref(j == 3 ? acb_imagref(s) : acb_realref(s));
                if (fmpz_bits(man) > 30) mag_set_fmpz_2exp_fmpz(r, man, ex);
                else mag_set_ui_2exp_si(r, fmpz_get_ui(man), fmpz_get_si(ex));
            }
        }
        if (fmpz_set_str(k, kk, 10)) abort();
        arb_const_pi(pole, 4000);
        arb_mul_fmpz(pole, pole, k, 4000); arb_mul_2exp_si(pole, pole, 1);
        arb_log_ui(a, p, 4000); arb_div(pole, pole, a, 4000);
        int member = arb_contains_zero(acb_realref(s)) && arb_contains(acb_imagref(s), pole);
        int outside = !arb_contains_zero(acb_realref(s)) || !arb_overlaps(acb_imagref(s), pole);
        cases++; members += member; disjoint += outside;
        if (expected ? !member : !outside) failures++;
    }
    printf("pole geometry cases=%u certified_member=%u certified_disjoint=%u failures=%u bits=4000\n",
           cases, members, disjoint, failures);
    acb_clear(s); arb_clear(pole); arb_clear(a);
    fmpz_clear(man); fmpz_clear(ex); fmpz_clear(k); flint_cleanup(); return failures != 0;
}
