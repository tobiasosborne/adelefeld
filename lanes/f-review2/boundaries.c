/* Counterexamples to the documented resource-limit contract. */
#include <stdio.h>
#include <adelefeld.h>

static void fields(adf_lball_t x, int exact, slong u, slong v, slong N)
{
    x->p = 5; x->exact = exact; x->v = v; x->N = N;
    fmpq_set_si(x->u, u, 1);
}

static int check(const char *name, int status, const adf_lball_t z,
                 const adf_lball_t want)
{
    int fail = status != ADF_OK || !adf_lball_identical(z, want);
    printf("%s: got=%s expected=OK expected_fields=(5,%d,", name, adf_status_str(status), want->exact);
    fmpq_print(want->u);
    printf(",%ld,%ld) mismatch=%d output_untouched=%d\n", want->v, want->N, fail,
           z->p == 7 && z->exact && z->v == 0 && z->N == 0 &&
           fmpz_is_one(fmpq_denref(z->u)) && fmpz_equal_si(fmpq_numref(z->u), 11));
    return fail;
}

static void sentinel(adf_lball_t z)
{
    fields(z, 1, 11, 0, 0); z->p = 7;
}

int main(void)
{
    adf_lball_t x, y, z, want;
    int failures = 0, admitted = 0;
    slong E = ADF_LBALL_EXP_MAX;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(want);
    for (int sign = -1; sign <= 1; sign += 2)
    {
        fields(x, 1, 1, sign * E, 0); fields(want, 1, 1, -sign * E, 0);
        admitted += adf_lball_is_canonical(x); sentinel(z);
        failures += check(sign < 0 ? "inverse_negative_endpoint" : "inverse_positive_endpoint",
                          adf_lball_inv(z, x), z, want);
    }
    fields(x, 0, 1, 0, E); fields(want, 0, 0, 0, E);
    admitted += adf_lball_is_canonical(x); sentinel(z);
    failures += check("sub_same_fine_ball", adf_lball_sub(z, x, x), z, want);
    fields(x, 0, 0, 0, 0); fields(y, 0, 1, 0, E); fields(want, 0, 0, 0, 0);
    admitted += adf_lball_is_canonical(x) + adf_lball_is_canonical(y); sentinel(z);
    failures += check("sub_coarse_minus_fine", adf_lball_sub(z, x, y), z, want);
    fields(x, 1, 0, 0, 0); fields(y, 0, 3, 0, E); fields(want, 1, 0, 0, 0);
    admitted += adf_lball_is_canonical(x) + adf_lball_is_canonical(y); sentinel(z);
    failures += check("div_zero_by_fine_unit", adf_lball_div(z, x, y), z, want);
    fields(x, 0, 0, 0, 0); fields(want, 0, 0, 0, 0); sentinel(z);
    admitted += adf_lball_is_canonical(x);
    failures += check("div_coarse_zero_by_fine_unit", adf_lball_div(z, x, y), z, want);
    fields(x, 0, 1, -E, 0); fields(want, 0, 1, 0, E); sentinel(z);
    admitted += adf_lball_is_canonical(x);
    failures += check("div_endpoint_ball_by_itself", adf_lball_div(z, x, x), z, want);
    printf("cases=7 canonical_input_checks=%d failures=%d\n", admitted, failures);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(want);
    flint_cleanup();
    return failures ? 1 : 0;
}
