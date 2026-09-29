/* lanes/d-ideles/probe_arb.c: what FLINT 3.0.1 does with two balls of docs/api-2.md.
   Build and run:
     cc -O1 -o /tmp/probe_arb lanes/d-ideles/probe_arb.c -lflint -lm && timeout 20 /tmp/probe_arb
   Probe 1: x = y = 1 +/- (1 - 2^-30) (SPEC 5, sign preservation): arb_mul at several precisions.
   Probe 2: y = (1 + 2^-40) +/- 1, which excludes 0: arb_inv and arb_div at several precisions.
   Probe 3: arb_set_interval_arf on [2^-60, 4]. */
#include <stdio.h>
#include <flint/flint.h>
#include <flint/arb.h>

int main(void)
{
    slong precs[] = { 64, 128, 1024, 100000 };
    slong i;
    arb_t x, y, z, one;
    arf_t lo, hi;

    arb_init(x); arb_init(y); arb_init(z); arb_init(one);
    arf_init(lo); arf_init(hi);
    arb_one(one);

    arb_one(x);
    mag_set_ui_2exp_si(arb_radref(x), (UWORD(1) << 30) - 1, -30);   /* exact: 30 bits */
    printf("probe 1: x = 1 +/- (1 - 2^-30); contains_zero(x) = %d\n", arb_contains_zero(x));
    for (i = 0; i < 4; i++)
    {
        arb_mul(z, x, x, precs[i]);
        printf("  prec %6ld: arb_mul contains_zero = %d, is_positive = %d\n",
               (long) precs[i], arb_contains_zero(z), arb_is_positive(z));
    }

    arb_one(y);
    arb_mul_2exp_si(y, y, -40);
    arb_add_ui(y, y, 1, 200);                                       /* 1 + 2^-40, exact */
    mag_one(arb_radref(y));
    printf("probe 2: y = (1 + 2^-40) +/- 1; contains_zero(y) = %d, is_positive(y) = %d\n",
           arb_contains_zero(y), arb_is_positive(y));
    for (i = 0; i < 4; i++)
    {
        arb_inv(z, y, precs[i]);
        printf("  prec %6ld: arb_inv finite = %d;", (long) precs[i], arb_is_finite(z));
        arb_div(z, one, y, precs[i]);
        printf(" arb_div finite = %d\n", arb_is_finite(z));
    }

    arf_set_ui_2exp_si(lo, 1, -60);
    arf_set_ui(hi, 4);
    for (i = 0; i < 4; i++)
    {
        arb_set_interval_arf(z, lo, hi, precs[i]);
        printf("probe 3: prec %6ld: set_interval [2^-60, 4] is_positive = %d\n",
               (long) precs[i], arb_is_positive(z));
    }

    arb_clear(x); arb_clear(y); arb_clear(z); arb_clear(one);
    arf_clear(lo); arf_clear(hi);
    return 0;
}
