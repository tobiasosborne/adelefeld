/* Red/green check of the specified Bsmall computation, after its Y16 proof. */
#include <adelefeld.h>
#include "../../src/localfactor.c"
#include <stdio.h>
int main(void)
{
    acb_t s, z;
    arf_t lo, hi;
    arb_t lp;
    mag_t b, previous, expected;
    acb_init(s); acb_init(z); arf_init(lo); arf_init(hi); arb_init(lp);
    mag_init(b); mag_init(previous); mag_init(expected);
    acb_set_si(s, -1); acb_mul_2exp_si(s, s, -2);
    mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -12);
    acb_mul_2exp_si(z, s, -1);
    arb_get_lbound_arf(lo, acb_realref(z), 288);
    arb_get_ubound_arf(hi, acb_realref(z), 288);
    arb_const_pi(lp, 288); arb_log(lp, lp, 288);
    real_derivative_bound(b, previous, z, lo, hi, lp, shift_of(lo), 288);
    mag_set_ui_2exp_si(expected, 45178009, -19);
    int failures = !mag_equal(b, expected);
    printf("Bsmall trace: "); mag_print(b);
    printf("; expected "); mag_print(expected);
    printf("; 1 check, %d failures\n", failures);
    acb_clear(s); acb_clear(z); arf_clear(lo); arf_clear(hi); arb_clear(lp);
    mag_clear(b); mag_clear(previous); mag_clear(expected); flint_cleanup();
    return failures ? 1 : 0;
}
