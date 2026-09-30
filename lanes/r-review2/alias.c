#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
int main(void)
{
    adf_rootlist_t L; fmpz_poly_t f, z; int st; slong n0, i; arb_t a, b;
    fmpz_poly_init(f); fmpz_poly_init(z); adf_rootlist_init(L); arb_init(a); arb_init(b);
    /* (X^2-2)(X+3)^2 * 6 */
    fmpz_poly_set_str(f, "5  -108 -72 42 36 6");
    st = adf_roots_real(L, f, 10); CHECK(st == ADF_OK); CHECK(L->n == 3);
    n0 = L->n; fmpz_poly_set(z, L->g); fmpz_poly_zero(f); fmpz_poly_set(f, z); fmpz_poly_zero(z);
    /* alias: f = L->g */
    st = adf_roots_real(L, L->g, 40); CHECK(st == ADF_OK); CHECK(L->n == n0);
    CHECK(adf_rootlist_verify_complete(L, f, 0));
    for (i = 0; i < L->n; i++) { adf_rootlist_get_arb(a, L, i); CHECK(arb_rel_accuracy_bits(a) >= 40 || arb_is_exact(a)); }
    /* error statuses leave L untouched */
    st = adf_roots_real(L, z, 10); CHECK(st == ADF_DOMAIN); CHECK(L->n == n0);
    st = adf_roots_real(L, f, 3000000); CHECK(st == ADF_LIMIT); CHECK(L->n == n0);
    CHECK(adf_rootlist_verify_complete(L, f, 0));
    /* constant */
    fmpz_poly_set_si(f, -7); st = adf_roots_real(L, f, 10); CHECK(st == ADF_OK); CHECK(L->n == 0 && L->count == 0);
    CHECK(adf_rootlist_verify_complete(L, f, 0));
    /* alias with the constant g */
    st = adf_roots_real(L, L->g, 10); CHECK(st == ADF_OK && L->n == 0);
    /* prec 0, -5, 1 */
    fmpz_poly_set_str(f, "3  -2 0 1");
    st = adf_roots_real(L, f, 0); CHECK(st == ADF_OK && L->n == 2); CHECK(adf_rootlist_verify_complete(L, f, 0));
    st = adf_roots_real(L, f, -5); CHECK(st == ADF_OK && L->n == 2);
    st = adf_roots_real(L, f, 1); CHECK(st == ADF_OK && L->n == 2);
    /* LIMIT on a fresh list: L stays the init value */
    { adf_rootlist_t M2; fmpz_poly_t h; fmpz_t p;
      adf_rootlist_init(M2); fmpz_poly_init(h); fmpz_init(p);
      fmpz_one(p); fmpz_mul_2exp(p, p, 16777216 - 1);      /* 2^(M-1) X - 1 : root 2^(1-M) < 2^(2-M) */
      fmpz_poly_set_coeff_si(h, 0, -1); fmpz_poly_set_coeff_fmpz(h, 1, p);
      st = adf_roots_real(M2, h, 2); CHECK(st == ADF_LIMIT); CHECK(M2->n == 0 && M2->count == 0 && fmpz_poly_is_one(M2->g));
      adf_rootlist_clear(M2); fmpz_poly_clear(h); fmpz_clear(p); }
    printf("alias tests done, fails %d\n", fails);
    adf_rootlist_clear(L); fmpz_poly_clear(f); fmpz_poly_clear(z); arb_clear(a); arb_clear(b);
    flint_cleanup_master();
    return fails != 0;
}
