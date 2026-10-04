/* Probe: the recurrence fallback Gamma(z+n)/rising(z,n) on FLINT 3.0.1, z = (-2k +/- 0.1)/2 + i(0.8 +/- 0.05). */
#include <flint/acb.h>
#include <stdio.h>

int main(void)
{
    acb_t s, z, g, q, t;
    acb_init(s); acb_init(z); acb_init(g); acb_init(q); acb_init(t);
    for (int k = 2; k <= 6; k++)
    {
        slong n = 2 * k / 2 + 2;  /* 1 - floor(-k - 0.05) */
        acb_set_si(s, -2 * k);
        mag_set_d(arb_radref(acb_realref(s)), 0.1);
        arb_set_d(acb_imagref(s), 1.6);
        mag_set_d(arb_radref(acb_imagref(s)), 0.1);
        acb_mul_2exp_si(z, s, -1);
        acb_add_ui(t, z, (ulong) n, 288);
        acb_gamma(g, t, 288);
        acb_rising_ui(q, z, (ulong) n, 288);
        printf("k=%d n=%ld gamma(z+n): ", k, (long) n);
        acb_printn(g, 8, 0);
        printf("  rising: ");
        acb_printn(q, 8, 0);
        acb_div(g, g, q, 288);
        printf("  ratio: ");
        acb_printn(g, 8, 0);
        acb_one(q);
        for (slong j = 0; j < n; j++)
        {
            acb_add_ui(t, z, (ulong) j, 288);
            acb_mul(q, q, t, 288);
        }
        printf("  loop product: ");
        acb_printn(q, 8, 0);
        printf(" contains 0: %d\n", acb_contains_zero(q));
    }
    acb_clear(s); acb_clear(z); acb_clear(g); acb_clear(q); acb_clear(t);
    flint_cleanup();
    return 0;
}
