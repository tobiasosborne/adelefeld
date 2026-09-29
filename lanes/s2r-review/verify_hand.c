#include <stdio.h>
#include <flint/fmpz_poly.h>
#include <flint/arb.h>
#include <adelefeld.h>

static int check(const char *name, adf_rootlist_t L, const fmpz_poly_t f,
                 int canonical, int entries, int complete)
{
    int a = adf_rootlist_is_canonical(L);
    int b = adf_rootlist_verify_entries(L, f);
    int c = adf_rootlist_verify_complete(L, f, 0);
    int error = a != canonical || b != entries || c != complete;
    printf("%s: canonical=%d entries=%d complete=%d error=%d\n", name, a, b, c, error);
    return error;
}

int main(void)
{
    fmpz_poly_t f, t;
    adf_rootlist_t L;
    int errors = 0;
    slong i;
    fmpz_poly_init(f); fmpz_poly_init(t); adf_rootlist_init(L);
    fmpz_poly_one(f);
    for (i = 1; i <= 3; i++) {
        fmpz_poly_zero(t);
        fmpz_poly_set_coeff_si(t, 0, -i);
        fmpz_poly_set_coeff_si(t, 1, 1);
        fmpz_poly_mul(f, f, t);
    }
    fmpz_poly_set(L->g, f);
    L->n = L->count = 1;
    L->ball = _arb_vec_init(1);
    arb_set_si(L->ball + 0, 2);
    mag_set_ui_2exp_si(arb_radref(L->ball + 0), 1, 1); /* [0,4], three roots */
    errors += check("three_in_one", L, f, 1, 1, 0);
    arb_set_si(L->ball + 0, 1);
    errors += check("missing_two", L, f, 1, 1, 0);
    arf_set_si_2exp_si(arb_midref(L->ball + 0), 3, -1);
    mag_set_ui_2exp_si(arb_radref(L->ball + 0), 1, -1); /* [1,2], endpoints roots */
    errors += check("root_endpoint", L, f, 1, 0, 0);
    arb_pos_inf(L->ball + 0);
    errors += check("infinite", L, f, 0, 0, 0);
    arb_indeterminate(L->ball + 0);
    errors += check("nan", L, f, 0, 0, 0);
    arb_set_si(L->ball + 0, 2);
    mag_set_ui_2exp_si(arb_radref(L->ball + 0), 1, 1000000000);
    errors += check("huge_radius", L, f, 0, 0, 0);
    arb_set_si(L->ball + 0, 1);
    L->count = 2;
    errors += check("false_count", L, f, 0, 0, 0);
    L->count = 1;
    fmpz_poly_one(L->g);
    errors += check("wrong_g", L, f, 1, 0, 0);
    fmpz_poly_set(L->g, f);
    _arb_vec_clear(L->ball, 1);
    L->n = L->count = 3;
    L->ball = _arb_vec_init(3);
    for (i = 0; i < 3; i++) arb_set_si(L->ball + i, i + 1);
    errors += check("three_exact", L, f, 1, 1, 1);
    arb_swap(L->ball + 0, L->ball + 1);
    errors += check("out_of_order", L, f, 0, 0, 0);
    arb_swap(L->ball + 0, L->ball + 1);
    arf_set_si_2exp_si(arb_midref(L->ball + 0), 3, -1);
    mag_set_ui_2exp_si(arb_radref(L->ball + 0), 1, -1); /* [1,2], touches second */
    errors += check("touching", L, f, 0, 0, 0);
    printf("cases=11 errors=%d\n", errors);
    adf_rootlist_clear(L); fmpz_poly_clear(f); fmpz_poly_clear(t);
    return errors != 0;
}
