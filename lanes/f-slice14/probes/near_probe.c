/* Probe: status and value of the local zeta factor at the real place near -2n, and of the steps inside. */
#include <adelefeld.h>
#include <adelefeld/localfactor.h>
#include <stdio.h>

int main(void)
{
    acb_t s, y, z, g;
    adf_place_t inf = adf_place_inf();
    acb_init(s); acb_init(y); acb_init(z); acb_init(g);
    for (int n = 2; n <= 6; n++)
    {
        acb_set_si(s, -2 * n);
        mag_set_d(arb_radref(acb_realref(s)), 0.1);
        mag_set_d(arb_radref(acb_imagref(s)), 0.1);
        arb_set_d(acb_imagref(s), 1.6);
        mag_set_d(arb_radref(acb_imagref(s)), 0.1);
        int st = adf_local_zeta_factor_at(y, NULL, s, inf, 256);
        acb_mul_2exp_si(z, s, -1);
        acb_gamma(g, z, 288);
        printf("s = %d + 1.6 i +/- 0.1: status %d, direct gamma finite %d: ", -2 * n, st, acb_is_finite(g));
        acb_printn(y, 10, 0);
        printf("\n");
    }
    acb_clear(s); acb_clear(y); acb_clear(z); acb_clear(g);
    flint_cleanup();
    return 0;
}
