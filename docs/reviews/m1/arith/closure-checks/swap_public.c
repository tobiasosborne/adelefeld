/* Public-level witness for R4. Both operand orders enclose the same product,
   but identical compares the stored midpoint and radius. */
#include <stdio.h>
#include <adelefeld.h>
#include <flint/acb.h>
#include <flint/arb.h>

int
main(void)
{
    flint_rand_t state;
    adf_adele_t x, y, z1, z2;
    adf_cadele_t cx, cy, cz1, cz2;
    int i, real_diff = 0, complex_diff = 0, cases = 20000;

    flint_randinit(state);
    flint_randseed(state, 1, 2);
    adf_adele_init(x); adf_adele_init(y);
    adf_adele_init(z1); adf_adele_init(z2);
    adf_cadele_init(cx); adf_cadele_init(cy);
    adf_cadele_init(cz1); adf_cadele_init(cz2);
    adf_fball_one(&x->fin);
    adf_fball_one(&y->fin);
    adf_fball_one(&cx->fin);
    adf_fball_one(&cy->fin);
    for (i = 0; i < cases; i++)
    {
        slong prec = 2 + n_randint(state, 200);
        arb_randtest(x->inf, state, 1 + n_randint(state, 300),
                     1 + n_randint(state, 8));
        arb_randtest(y->inf, state, 1 + n_randint(state, 300),
                     1 + n_randint(state, 8));
        if (arb_is_finite(x->inf) && arb_is_finite(y->inf))
        {
            adf_adele_mul(z1, x, y, prec);
            adf_adele_mul(z2, y, x, prec);
            real_diff += !adf_adele_identical(z1, z2);
        }
        acb_randtest(cx->inf, state, 1 + n_randint(state, 300),
                     1 + n_randint(state, 8));
        acb_randtest(cy->inf, state, 1 + n_randint(state, 300),
                     1 + n_randint(state, 8));
        if (acb_is_finite(cx->inf) && acb_is_finite(cy->inf))
        {
            adf_cadele_mul(cz1, cx, cy, prec);
            adf_cadele_mul(cz2, cy, cx, prec);
            complex_diff += !adf_cadele_identical(cz1, cz2);
        }
    }
    printf("cases=%d real_order_diff=%d complex_order_diff=%d\n",
           cases, real_diff, complex_diff);
    adf_cadele_clear(cz2); adf_cadele_clear(cz1);
    adf_cadele_clear(cy); adf_cadele_clear(cx);
    adf_adele_clear(z2); adf_adele_clear(z1);
    adf_adele_clear(y); adf_adele_clear(x);
    flint_randclear(state);
    return real_diff == 0 || complex_diff == 0;
}
