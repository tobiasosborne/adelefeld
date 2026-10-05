/* Compare the proved Bsmall implementation with the saved review baseline on 20000 seeded boxes. */
#include <adelefeld.h>
#include <stdint.h>
#include <stdio.h>
int old_local_zeta_factor_at(acb_t, adf_place_t *, const acb_t, adf_place_t, slong);
static uint64_t state = 94026;
static ulong next(ulong n)
{
    state = state*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
    return (ulong) ((state >> 24) % n);
}
int main(void)
{
    static const slong precs[] = {16, 32, 64, 128, 256};
    acb_t s, old, new;
    ulong equal = 0, strict = 0, failures = 0, ok = 0, nd = 0, limit = 0;
    acb_init(s); acb_init(old); acb_init(new);
    for (ulong i = 0; i < 20000; i++)
    {
        slong x, xe, y, ye, rx, ry;
        if (i % 3 == 0)
        {
            xe = -(slong) (1+next(24));
            x = -2*(slong) next(63)*(WORD(1) << -xe)+(next(2) ? 1 : -1);
            rx = xe-1-(slong) next(14);
            y = (slong) next(3)-1; ye = xe;
            ry = rx-4;
        }
        else
        {
            x = (slong) next(192*1024)-128*1024; xe = -10;
            rx = (slong) next(18)-12;
            y = (slong) next(257)-128; ye = -(slong) next(12);
            ry = (slong) next(18)-14;
        }
        acb_zero(s);
        arf_set_si_2exp_si(arb_midref(acb_realref(s)), x, xe);
        arf_set_si_2exp_si(arb_midref(acb_imagref(s)), y, ye);
        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, rx);
        mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 1, ry);
        if (i % 6 == 0 && y == 0) mag_zero(arb_radref(acb_imagref(s)));
        slong prec = precs[next(5)];
        int a = old_local_zeta_factor_at(old, NULL, s, adf_place_inf(), prec);
        int b = adf_local_zeta_factor_at(new, NULL, s, adf_place_inf(), prec);
        int fail = a != b;
        ok += b == ADF_OK; nd += b == ADF_NOT_DETERMINED; limit += b == ADF_LIMIT;
        if (a == ADF_OK && b == ADF_OK)
        {
            int same = acb_equal(old, new);
            equal += same;
            strict += !same && acb_contains(old, new);
            fail |= !acb_contains(old, new);
        }
        if (fail)
        {
            failures++;
            printf("FAIL row %lu prec %ld statuses %d %d input %ld %ld %ld %ld 1 %ld 1 %ld\n",
                   i, prec, a, b, x, xe, y, ye, rx, ry);
            acb_printd(old, 20); printf(" old\n"); acb_printd(new, 20); printf(" new\n");
        }
    }
    printf("boxes=20000 OK=%lu ND=%lu LIMIT=%lu equal=%lu strict_containment=%lu failures=%lu\n",
           ok, nd, limit, equal, strict, failures);
    acb_clear(s); acb_clear(old); acb_clear(new); flint_cleanup();
    return failures ? 1 : 0;
}
