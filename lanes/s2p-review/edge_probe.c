#include <stdio.h>

#include <flint/fmpz_poly.h>
#include <adelefeld.h>

static adf_place_t prime(ulong p)
{
    adf_place_t v;
    if (adf_place_prime(&v, p) != ADF_OK) flint_abort();
    return v;
}

static int check(const char * name, fmpz_poly_t f, ulong p, slong prec, slong low, slong high,
                 slong want_n, slong want_nu)
{
    adf_rootlist_t L;
    int st, bad = 0;
    adf_rootlist_init(L);
    st = adf_roots_padic_partial(L, f, prime(p), prec, low);
    if (st != ADF_OK || L->nu != want_nu) bad++;
    if (want_nu > 0 && adf_roots_padic(L, f, prime(p), prec, low) != ADF_NOT_DETERMINED) bad++;
    st = adf_roots_padic(L, f, prime(p), prec, high);
    if (st != ADF_OK || L->n != want_n || L->nu != 0 ||
        !adf_rootlist_verify_complete(L, f, high)) bad++;
    printf("%s p=%lu low=%ld high=%ld status=%d roots=%ld unresolved=%ld errors=%d\n",
           name, p, low, high, st, L->n, L->nu, bad);
    adf_rootlist_clear(L);
    return bad;
}

static void product(fmpz_poly_t f, const long * roots, long n, long scale)
{
    fmpz_poly_t t;
    long i;
    fmpz_poly_init(t);
    fmpz_poly_set_si(f, scale);
    for (i = 0; i < n; i++)
    {
        fmpz_poly_zero(t);
        fmpz_poly_set_coeff_si(t, 1, 1);
        fmpz_poly_set_coeff_si(t, 0, -roots[i]);
        fmpz_poly_mul(f, f, t);
    }
    fmpz_poly_clear(t);
}

int main(void)
{
    fmpz_poly_t f;
    const long close2[] = {0, 1L << 20};
    const long close3[] = {1, 1 + 2187, 1 - 2187};
    const long repeated[] = {1, 1, 1, -2};
    const long large[] = {1, 2};
    int bad = 0;
    fmpz_poly_init(f);
    product(f, close2, 2, 1);
    bad += check("close2", f, 2, 1, 19, 20, 2, 1);
    product(f, close3, 3, 1);
    bad += check("close3", f, 3, 1, 6, 7, 3, 1);
    product(f, repeated, 4, 27);
    bad += check("repeated", f, 3, 1, 0, 1, 2, 1);
    fmpz_poly_zero(f);
    fmpz_poly_set_coeff_si(f, 2, 1);
    fmpz_poly_set_coeff_si(f, 1, 1);
    fmpz_poly_set_coeff_si(f, 0, 1);
    bad += check("no_root_mod2", f, 2, 3, 0, 0, 0, 0);
    fmpz_poly_zero(f);
    fmpz_poly_set_coeff_si(f, 5, 1);
    fmpz_poly_set_coeff_si(f, 1, -1);
    bad += check("all_residues_mod5", f, 5, 1, 0, 0, 5, 0);
    product(f, large, 2, 1);
    bad += check("near_prime_bound", f, 1048573, 3, 0, 0, 2, 0);
    fmpz_poly_clear(f);
    printf("total_errors=%d\n", bad);
    return bad ? 1 : 0;
}
