#include <adelefeld.h>
#include <flint/fmpz_vec.h>
#include <stdio.h>

/* A hand-built list.  No constructor of place.h is used for the place word. */
int main(void)
{
    fmpz_poly_t f;
    adf_rootlist_t L;
    adf_place_t valid;
    int constructor_status, canonical, verified;

    fmpz_poly_init(f);
    fmpz_poly_set_coeff_si(f, 0, 3);
    fmpz_poly_set_coeff_si(f, 2, 1); /* X^2 + 3 */
    adf_rootlist_init(L);
    constructor_status = adf_place_prime(&valid, 4);
    L->place.opaque = 4;
    L->scope = ADF_ROOTLIST_SEED;
    L->complete = 0;
    fmpz_poly_set(L->g, f);
    L->n = 1;
    L->a = _fmpz_vec_init(1);
    L->K = flint_malloc(sizeof(slong));
    L->s = flint_malloc(sizeof(slong));
    fmpz_set_ui(L->a, 1);
    L->K[0] = 1;
    L->s[0] = 0;

    canonical = adf_rootlist_is_canonical(L);
    verified = adf_rootlist_verify_entries(L, f);
    printf("prime_constructor=%d canonical=%d verified=%d\n", constructor_status, canonical, verified);
    /* A square modulo 8 is 0, 1 or 4, so X^2 + 3 has no root in Z_2. */
    for (int x = 0; x < 8; x++)
        if ((x * x + 3) % 8 == 0)
            printf("unexpected_root_mod_8=%d\n", x);

    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    return canonical == 1 && verified == 1 && constructor_status == ADF_DOMAIN ? 0 : 1;
}
