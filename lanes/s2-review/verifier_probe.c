#include <adelefeld.h>
#include <flint/fmpz_vec.h>
#include <stdio.h>

static int failures;
static int checks;

static void expect(const adf_rootlist_t L, const fmpz_poly_t f, int canonical, int verified)
{
    int c = adf_rootlist_is_canonical(L);
    int v = adf_rootlist_verify_entries(L, f);
    checks++;
    if (c != canonical || v != verified) {
        printf("case=%d canonical=%d verified=%d expected=%d,%d\n", checks, c, v,
               canonical, verified);
        failures++;
    }
}

int main(void)
{
    fmpz_poly_t f;
    adf_rootlist_t L;
    adf_place_t p2;
    fmpz_poly_init(f); adf_rootlist_init(L);
    if (adf_place_prime(&p2, 2) != ADF_OK) return 2;
    fmpz_poly_set_coeff_si(f, 0, -9);
    fmpz_poly_set_coeff_si(f, 2, 1);
    fmpz_poly_set(L->g, f);
    L->place = p2;
    L->scope = ADF_ROOTLIST_SEED;
    L->complete = 0;
    L->n = 1;
    L->a = _fmpz_vec_init(1);
    L->K = flint_malloc(sizeof(slong));
    L->s = flint_malloc(sizeof(slong));
    fmpz_set_si(L->a, 3); L->K[0] = 4; L->s[0] = 1;
    expect(L, f, 1, 1); /* valid root at 3 */

    fmpz_set_si(L->a, 1); expect(L, f, 1, 0); fmpz_set_si(L->a, 3);
    L->K[0] = 1; expect(L, f, 0, 0); L->K[0] = 4;
    L->s[0] = 0; expect(L, f, 1, 0); L->s[0] = 1;
    fmpz_set_si(L->a, 19); expect(L, f, 0, 0); fmpz_set_si(L->a, 3);
    fmpz_poly_scalar_mul_si(L->g, L->g, 2); expect(L, f, 0, 0); fmpz_poly_set(L->g, f);
    L->reduced = 1; expect(L, f, 1, 0); L->reduced = 0;
    L->complete = 1; expect(L, f, 0, 0); L->complete = 0;
    L->K[0] = WORD_MAX; expect(L, f, 1, 0); L->K[0] = 4;
    L->scope = ADF_ROOTLIST_PARTITION; L->nu = 1; L->ua = _fmpz_vec_init(1);
    L->ue = flint_malloc(sizeof(slong)); fmpz_set_si(L->ua, 3); L->ue[0] = 2;
    expect(L, f, 0, 0);
    printf("hand_built_cases=%d failures=%d\n", checks, failures);
    adf_rootlist_clear(L); fmpz_poly_clear(f);
    return failures ? 1 : 0;
}
