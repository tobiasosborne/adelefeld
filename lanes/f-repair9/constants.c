/* Constants for the weighted Gamma-integral proof; FLINT 3.0.1 scalar enclosures. */
#include <flint/arb.h>
#include <stdio.h>
int main(void)
{
    arb_t pi, lp, t, q;
    int checks = 0, failures = 0;
    arb_init(pi); arb_init(lp); arb_init(t); arb_init(q);
    arb_const_pi(pi, 256);
    arb_log(lp, pi, 256);
    arb_set_ui(q, 23); arb_div_ui(q, q, 20, 256);
    checks++; failures += !arb_lt(lp, q);
    arb_neg(t, pi); arb_exp(t, t, 256);
    arb_div(t, t, pi, 256);
    arb_one(q); arb_div_ui(q, q, 60, 256);
    checks++; failures += !arb_lt(t, q);
    arb_neg(t, pi); arb_exp(t, t, 256);
    arb_one(q); arb_mul_2exp_si(q, q, -1);
    checks++; failures += !arb_lt(t, q);
    printf("constant inequalities: %d checks, %d failures\n", checks, failures);
    arb_clear(pi); arb_clear(lp); arb_clear(t); arb_clear(q);
    flint_cleanup();
    return failures ? 1 : 0;
}
