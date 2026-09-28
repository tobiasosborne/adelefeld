/* Probe: adf_adele_div_rat and adf_cadele_div_rat at prec = 1. arb_set_fmpq(1/3, prec = 1) may
   give a ball that contains 0; arb_div then returns a non-finite ball, and the output would break
   conventions 5.5 (arb_is_finite(inf)) and 4.4 ("Non-finite ball produced inside a computation from
   finite inputs: never stored; the function returns ADF_NOT_DETERMINED"). */
#include <stdio.h>
#include <adelefeld.h>
#include <flint/arb.h>
#include <flint/acb.h>

int
main(void)
{
    slong prec;
    for (prec = 1; prec <= 3; prec++)
    {
        adf_adele_t x, z;
        adf_cadele_t cx, cz;
        adf_rat_t q;
        fmpz_t n, d;
        arb_t t;
        int st, stc;
        fmpq_t tq;

        adf_adele_init(x); adf_adele_init(z); adf_cadele_init(cx); adf_cadele_init(cz);
        adf_rat_init(q); fmpz_init(n); fmpz_init(d); arb_init(t); fmpq_init(tq);
        fmpz_set_si(n, 1); fmpz_set_si(d, 3);
        adf_rat_set_fmpz2(q, n, d);
        adf_adele_set_si(x, 1);
        adf_cadele_set_adele(cx, x);
        adf_rat_get_fmpq(tq, q);
        arb_set_fmpq(t, tq, prec);
        st = adf_adele_div_rat(z, x, q, prec);
        stc = adf_cadele_div_rat(cz, cx, q, prec);
        printf("prec %ld: arb_set_fmpq(1/3) = ", (long) prec);
        arb_printd(t, 10);
        printf("; adele div_rat status %s, inf = ", adf_status_str(st));
        arb_printd(z->inf, 10);
        printf(", adf_adele_is_canonical = %d; cadele status %s, is_canonical = %d\n",
               adf_adele_is_canonical(z), adf_status_str(stc), adf_cadele_is_canonical(cz));
        adf_adele_clear(x); adf_adele_clear(z); adf_cadele_clear(cx); adf_cadele_clear(cz);
        adf_rat_clear(q); fmpz_clear(n); fmpz_clear(d); arb_clear(t); fmpq_clear(tq);
    }
    return 0;
}
