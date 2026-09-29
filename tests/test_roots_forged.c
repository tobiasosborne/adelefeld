/* test_roots_forged.c: a list whose place was written by hand and is not a prime is refused by the predicate
   and by the entries verifier (docs/reviews/s2/review.md, finding 1; the input is the one of
   lanes/s2-review/forged_place.c). A place made by adf_place_prime is a proved prime (src/place.c,
   adf_place_prime); a verifier is called on values of unknown origin and tests it again. */

#include <adelefeld.h>
#include <flint/fmpz_vec.h>
#include "test_runner.h"

/* The list of the review: g = f = X^2 + 3, scope SEED, one certificate (1, 1, 0), at the place word pw. */
static void
forged(adf_rootlist_t L, fmpz_poly_t f, ulong pw)
{
    fmpz_poly_init(f);
    fmpz_poly_set_coeff_si(f, 0, 3);
    fmpz_poly_set_coeff_si(f, 2, 1);
    adf_rootlist_init(L);
    L->place.opaque = pw;
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
}

ADF_TEST(a_composite_place_is_refused)
{
    static const ulong composite[] = { 4, 6, 9, 15, 1048576, UWORD(18446744073709551615) };
    adf_rootlist_t L;
    fmpz_poly_t f;
    adf_place_t v;
    size_t i;

    for (i = 0; i < sizeof(composite) / sizeof(composite[0]); i++)
    {
        ADF_CHECK(adf_place_prime(&v, composite[i]) == ADF_DOMAIN);
        forged(L, f, composite[i]);
        ADF_CHECK_MSG(adf_rootlist_is_canonical(L) == 0, "canonical at %lu", composite[i]);
        ADF_CHECK_MSG(adf_rootlist_verify_entries(L, f) == 0, "verified at %lu", composite[i]);
        ADF_CHECK_MSG(adf_rootlist_verify_complete(L, f, 3) == 0, "complete at %lu", composite[i]);
        adf_rootlist_clear(L);
        fmpz_poly_clear(f);
    }
}

/* The control: the same shape at the prime 2 with a true certificate is accepted. X^2 + 3 has no root in
   Z_2, so the control uses g = X^2 - 1 and the ball 1 + 8 Z_2 with s = 1 (g(1) = 0, g'(1) = 2). */
ADF_TEST(the_same_shape_at_a_prime_is_accepted)
{
    adf_rootlist_t L;
    fmpz_poly_t f;

    forged(L, f, 2);
    fmpz_poly_set_coeff_si(f, 0, -1);
    fmpz_poly_set(L->g, f);
    L->K[0] = 3;
    L->s[0] = 1;
    ADF_CHECK(adf_rootlist_is_canonical(L) == 1);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
}

