/* Independent neighbouring inputs for M1-D3. The expected statuses follow from the
   stored exponent rule and from an exact finite point, not from the function under test. */
#include <stdio.h>
#include <adelefeld.h>
#include <flint/arb.h>

static int
one_case(int radius, slong exponent, int negative)
{
    adf_adele_t x;
    adf_rat_t out, expected;
    fmpz_t shift;
    int st, want, good;

    adf_adele_init(x);
    adf_rat_init(out);
    adf_rat_init(expected);
    fmpz_init(shift);
    adf_fball_set_si(&x->fin, negative ? -1 : 1);
    adf_rat_set_si(out, -99);
    fmpz_set_si(shift, exponent - 1);
    if (radius)
    {
        arb_set_si(x->inf, negative ? -1 : 1);
        mag_one(arb_radref(x->inf));
        mag_mul_2exp_fmpz(arb_radref(x->inf), arb_radref(x->inf), shift);
    }
    else
    {
        arb_set_si(x->inf, negative ? -1 : 1);
        arb_mul_2exp_fmpz(x->inf, x->inf, shift);
    }
    good = adf_adele_is_canonical(x);
    good &= radius ? fmpz_cmp_si(MAG_EXPREF(arb_radref(x->inf)), exponent) == 0
                   : fmpz_cmp_si(ARF_EXPREF(arb_midref(x->inf)), exponent) == 0;
    st = adf_adele_reconstruct(out, x);
    want = (exponent > ADF_RECON_EXP_MAX || exponent < -ADF_RECON_EXP_MAX)
           ? ADF_LIMIT : (radius ? ADF_OK : ADF_NO_SOLUTION);
    good &= st == want;
    adf_rat_set_si(expected, st == ADF_OK ? (negative ? -1 : 1) : -99);
    good &= adf_rat_equal(out, expected);
    printf("%s e=%ld sign=%d status=%d expected=%d pass=%d\n",
           radius ? "radius" : "midpoint", (long) exponent, negative ? -1 : 1,
           st, want, good);
    fmpz_clear(shift);
    adf_rat_clear(expected);
    adf_rat_clear(out);
    adf_adele_clear(x);
    return good;
}

int
main(void)
{
    const slong exps[] = {
        -ADF_RECON_EXP_MAX - 1, -ADF_RECON_EXP_MAX,
        -ADF_RECON_EXP_MAX + 1, ADF_RECON_EXP_MAX - 1,
        ADF_RECON_EXP_MAX, ADF_RECON_EXP_MAX + 1
    };
    int count = 0, fail = 0;
    size_t i;
    for (i = 0; i < sizeof(exps) / sizeof(exps[0]); i++)
    {
        int radius, negative;
        for (radius = 0; radius <= 1; radius++)
            for (negative = 0; negative <= 1; negative++)
            {
                fail += !one_case(radius, exps[i], negative);
                count++;
            }
    }
    printf("cases=%d failures=%d\n", count, fail);
    return fail != 0;
}
