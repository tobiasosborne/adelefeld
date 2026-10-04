/* Probe of FLINT 3.0.1 acb_gamma on the boxes the design names (local-zeta.md:594-597, the recurrence limit
   fixture), and on huge arguments: is the direct value finite? */
#include <flint/acb.h>
#include <stdio.h>
#include <time.h>

static void box(acb_t z, double x, double y, double rx, double ry)
{
    arb_set_d(acb_realref(z), x);
    arb_set_d(acb_imagref(z), y);
    mag_set_d(arb_radref(acb_realref(z)), rx);
    mag_set_d(arb_radref(acb_imagref(z)), ry);
}

static void try(const char *name, const acb_t s, slong prec)
{
    acb_t z, g;
    clock_t t0 = clock();
    acb_init(z);
    acb_init(g);
    acb_mul_2exp_si(z, s, -1);
    acb_gamma(g, z, prec);
    printf("%-40s prec %5ld finite %d  %.3f s  ", name, (long) prec, acb_is_finite(g),
           (double) (clock() - t0) / CLOCKS_PER_SEC);
    acb_printn(g, 10, 0);
    printf("\n");
    acb_clear(z);
    acb_clear(g);
}

int main(void)
{
    acb_t s;
    acb_init(s);
    box(s, -2, 1.6, 0.1, 0.1);
    try("[-2.1,-1.9]+i[1.5,1.7]", s, 288);
    try("[-2.1,-1.9]+i[1.5,1.7]", s, 160);
    try("[-2.1,-1.9]+i[1.5,1.7]", s, 48);
    box(s, -130, 0.25, 1, 0.1);
    try("[-131,-129]+i[0.15,0.35]", s, 288);
    for (int k = 120; k <= 132; k += 2)
    {
        char name[64];
        box(s, -k, 0.5, 1, 0.4);
        snprintf(name, sizeof name, "[%d +/- 1] + i[0.5 +/- 0.4]", -k);
        try(name, s, 160);
    }
    acb_zero(s);
    arf_set_si_2exp_si(arb_midref(acb_realref(s)), 1, 1000);
    arb_one(acb_imagref(s));
    try("2^1000 + i", s, 160);
    arf_neg(arb_midref(acb_realref(s)), arb_midref(acb_realref(s)));
    try("-2^1000 + i", s, 160);
    acb_zero(s);
    arf_set_si_2exp_si(arb_midref(acb_imagref(s)), 1, 1000);
    arb_one(acb_realref(s));
    try("1 + i 2^1000", s, 160);
    acb_clear(s);
    flint_cleanup();
    return 0;
}
