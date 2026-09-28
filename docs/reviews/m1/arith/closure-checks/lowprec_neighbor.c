/* Neighbours of R3: both signs, prec 1, 0 and -5, aliased and separate output.
   The exact input is (1+i ; 1) for the complex call and (1 ; 1) for the real call. */
#include <stdio.h>
#include <adelefeld.h>
#include <flint/acb.h>
#include <flint/fmpq.h>

int
main(void)
{
    const slong precs[] = {1, 0, -5};
    adf_adele_t x, real, real_at_two, real_alias;
    adf_cadele_t cx, complex, complex_at_two, complex_alias;
    adf_rat_t q;
    fmpq_t fq, expected;
    int sign, alias, failures = 0, cases = 0;
    size_t pi;

    adf_adele_init(x);
    adf_adele_init(real);
    adf_adele_init(real_at_two);
    adf_adele_init(real_alias);
    adf_cadele_init(cx);
    adf_cadele_init(complex);
    adf_cadele_init(complex_at_two);
    adf_cadele_init(complex_alias);
    adf_rat_init(q);
    fmpq_init(fq);
    fmpq_init(expected);
    adf_adele_set_si(x, 1);
    adf_cadele_set_adele(cx, x);
    arb_one(acb_imagref(cx->inf));

    for (sign = -1; sign <= 1; sign += 2)
    {
        fmpq_set_si(fq, sign, 3);
        adf_rat_set_fmpq(q, fq);
        fmpq_set_si(expected, sign * 3, 1);
        adf_adele_div_rat(real_at_two, x, q, 2);
        adf_cadele_div_rat(complex_at_two, cx, q, 2);
        for (pi = 0; pi < sizeof(precs) / sizeof(precs[0]); pi++)
            for (alias = 0; alias <= 1; alias++)
            {
                adf_adele_t * rz = alias ? &real_alias : &real;
                adf_cadele_t * cz = alias ? &complex_alias : &complex;
                int sr, sc, okr, okc;
                if (alias)
                {
                    adf_adele_set(*rz, x);
                    adf_cadele_set(*cz, cx);
                }
                sr = adf_adele_div_rat(*rz, alias ? *rz : x, q, precs[pi]);
                sc = adf_cadele_div_rat(*cz, alias ? *cz : cx, q, precs[pi]);
                okr = sr == ADF_OK && adf_adele_is_canonical(*rz)
                      && adf_adele_identical(*rz, real_at_two)
                      && arb_contains_fmpq((*rz)->inf, expected);
                okc = sc == ADF_OK && adf_cadele_is_canonical(*cz)
                      && adf_cadele_identical(*cz, complex_at_two)
                      && arb_contains_fmpq(acb_realref((*cz)->inf), expected)
                      && arb_contains_fmpq(acb_imagref((*cz)->inf), expected);
                cases += 2;
                failures += !okr + !okc;
                printf("sign=%d prec=%ld alias=%d real=%d complex=%d\n",
                       sign, (long) precs[pi], alias, okr, okc);
            }
    }
    printf("cases=%d failures=%d\n", cases, failures);
    fmpq_clear(expected);
    fmpq_clear(fq);
    adf_rat_clear(q);
    adf_cadele_clear(complex_alias);
    adf_cadele_clear(complex_at_two);
    adf_cadele_clear(complex);
    adf_cadele_clear(cx);
    adf_adele_clear(real_alias);
    adf_adele_clear(real_at_two);
    adf_adele_clear(real);
    adf_adele_clear(x);
    return failures != 0;
}
