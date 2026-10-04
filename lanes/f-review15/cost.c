#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    if (argc != 6) return 2;
    ulong p = strtoul(argv[1], NULL, 10);
    slong prec = strtol(argv[2], NULL, 10), exponent = strtol(argv[4], NULL, 10);
    int sign = atoi(argv[5]);
    adf_place_t v = adf_place_inf(), w = adf_place_inf();
    if (p && adf_place_prime(&v, p)) return 2;
    acb_t s, y, before;
    acb_init(s); acb_init(y); acb_init(before);
    if (!strcmp(argv[3], "imag")) {
        acb_one(s);
        arf_set_si_2exp_si(arb_midref(acb_imagref(s)), sign, exponent);
    } else {
        arf_set_si_2exp_si(arb_midref(acb_realref(s)), sign, exponent);
        if (!strcmp(argv[3], "real-imag")) arb_one(acb_imagref(s));
    }
    acb_set_si_si(y, 7, -5); acb_set(before, y);
    printf("start p=%lu prec=%ld axis=%s mantissa=%d exponent=%ld\n", p, prec, argv[3], sign, exponent);
    fflush(stdout);
    struct timespec a, b;
    clock_gettime(CLOCK_MONOTONIC, &a);
    int st = adf_local_zeta_factor_at(y, &w, s, v, prec);
    clock_gettime(CLOCK_MONOTONIC, &b);
    int preserved = st == ADF_OK || acb_equal(y, before);
    int finite = acb_is_finite(y);
    printf("status=%d seconds=%.6f preserved=%d finite=%d\n", st,
           (b.tv_sec - a.tv_sec) + 1e-9 * (b.tv_nsec - a.tv_nsec), preserved, finite);
    acb_clear(s); acb_clear(y); acb_clear(before); flint_cleanup();
    return !preserved || !finite;
}
