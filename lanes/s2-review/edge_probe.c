#include <adelefeld.h>
#include <stdio.h>

static int failed;
#define CHECK(x) do { if (!(x)) { printf("failure at line %d\n", __LINE__); failed++; } } while (0)

int main(void)
{
    fmpz_poly_t f;
    fmpz_t a, t;
    adf_rootlist_t L;
    adf_place_t p2, p3, p64;
    int checks = 0;

    fmpz_poly_init(f); fmpz_init(a); fmpz_init(t); adf_rootlist_init(L);
    CHECK(adf_place_prime(&p2, 2) == ADF_OK); checks++;
    CHECK(adf_place_prime(&p3, 3) == ADF_OK); checks++;
    CHECK(adf_place_prime(&p64, UWORD(18446744073709551557)) == ADF_OK); checks++;

    /* X^2 - 9 at 2: two nearby roots, derivative valuation one. */
    fmpz_poly_set_coeff_si(f, 0, -9);
    fmpz_poly_set_coeff_si(f, 2, 1);
    fmpz_set_si(a, 3);
    CHECK(adf_root_padic_from_seed(L, f, p2, a, 6) == ADF_OK); checks++;
    CHECK(L->s[0] == 1 && L->K[0] == 6 && fmpz_equal_si(L->a, 3)); checks++;
    fmpz_one(t); fmpz_mul_2exp(t, t, 4096); fmpz_add_ui(a, t, 3);
    CHECK(adf_root_padic_from_seed(L, f, p2, a, 6) == ADF_OK); checks++;
    CHECK(fmpz_equal_si(L->a, 3)); checks++;
    fmpz_set_si(a, -3);
    CHECK(adf_root_padic_from_seed(L, f, p2, a, 6) == ADF_OK); checks++;
    CHECK(fmpz_equal_si(L->a, 61)); checks++;

    /* Both inputs alias fields of L. Then reuse L at another prime. */
    CHECK(adf_root_padic_from_seed(L, L->g, p2, L->a, 7) == ADF_OK); checks++;
    CHECK(L->K[0] == 7 && L->s[0] == 1 && fmpz_equal_si(L->a, 125)); checks++;
    fmpz_set_si(a, 0);
    CHECK(adf_root_padic_from_seed(L, f, p3, a, 5) == ADF_NOT_DETERMINED); checks++;
    CHECK(adf_place_equal(L->place, p2) && L->K[0] == 7); checks++;
    fmpz_poly_zero(f);
    fmpz_poly_set_coeff_si(f, 0, -1);
    fmpz_poly_set_coeff_si(f, 1, 1);
    fmpz_set_si(a, 1);
    CHECK(adf_root_padic_from_seed(L, f, p3, a, 5) == ADF_OK); checks++;
    CHECK(adf_place_equal(L->place, p3) && L->s[0] == 0 && L->K[0] == 5); checks++;
    CHECK(adf_rootlist_verify_entries(L, f) == 1); checks++;

    CHECK(adf_root_padic_from_seed(L, f, p64, a, 131073) == ADF_LIMIT); checks++;
    CHECK(adf_place_equal(L->place, p3) && L->K[0] == 5); checks++;
    CHECK(adf_root_padic_from_seed(L, f, p64, a, 20) == ADF_OK); checks++;
    CHECK(adf_place_equal(L->place, p64) && L->K[0] == 20); checks++;

    fmpz_poly_zero(f);
    fmpz_poly_set_coeff_si(f, 0, 18);
    CHECK(adf_root_padic_from_seed(L, f, p2, a, 2) == ADF_NOT_DETERMINED); checks++;
    CHECK(adf_place_equal(L->place, p64) && L->K[0] == 20); checks++;
    fmpz_poly_zero(f);
    CHECK(adf_root_padic_from_seed(L, f, p2, a, 2) == ADF_DOMAIN); checks++;
    CHECK(adf_place_equal(L->place, p64) && L->K[0] == 20); checks++;

    printf("checks=%d failures=%d\n", checks, failed);
    adf_rootlist_clear(L); fmpz_poly_clear(f); fmpz_clear(a); fmpz_clear(t);
    return failed ? 1 : 0;
}
