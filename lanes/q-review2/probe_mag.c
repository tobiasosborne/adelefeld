#include <adelefeld.h>
#include <stdio.h>
int main(void)
{
    mag_t a;
    double d;
    mag_init(a);
    mag_set_ui(a, 3);
    printf("set_ui(3): man %lu exp %ld  d %.17g\n", (ulong) MAG_MAN(a), (long) MAG_EXP(a), mag_get_d(a));
    mag_set_ui(a, 1);
    printf("set_ui(1): man %lu exp %ld  d %.17g\n", (ulong) MAG_MAN(a), (long) MAG_EXP(a), mag_get_d(a));
    mag_one(a);
    printf("one      : man %lu exp %ld  d %.17g\n", (ulong) MAG_MAN(a), (long) MAG_EXP(a), mag_get_d(a));
    mag_set_ui_2exp_si(a, 1, -60);
    printf("ui2exp(1,-60): man %lu exp %ld  d %.17g\n", (ulong) MAG_MAN(a), (long) MAG_EXP(a), mag_get_d(a));
    mag_one(a); mag_mul_2exp_si(a, a, -60);
    printf("one*2^-60   : man %lu exp %ld  d %.17g\n", (ulong) MAG_MAN(a), (long) MAG_EXP(a), mag_get_d(a));
    d = mag_get_d(a); (void) d;
    return 0;
}
