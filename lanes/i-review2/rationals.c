/* Diagonal rational norms with independent constructor and norm precisions; large finite places. */
#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); abort(); } } while (0)
int main(void)
{
    slong ps[] = {2, 3, 4, 8, 30, 64, 256};
    adf_idele_t x;
    adf_idclass_t c;
    adf_rat_t q, av;
    arb_t t;
    adf_place_t place;
    fmpz_t prime;
    unsigned long checks = 0, okay = 0, nd = 0, cases = 0;
    adf_idele_init(x); adf_idclass_init(c); adf_rat_init(q); adf_rat_init(av); arb_init(t); fmpz_init(prime);
    for (int i = -80; i <= 80; i++) if (i) for (int d = 1; d <= 31; d++)
        for (int pc = 0; pc < 7; pc++) for (int pn = 0; pn < 7; pn++)
    {
        fmpq_set_si(q->q, i, d); CHECK(adf_idele_set_rat(x, q, ps[pc]) == ADF_OK);
        arb_set_si(t, 97); int st = adf_idele_norm(t, x, ps[pn]);
        int sc = adf_idclass_set_idele(c, x, ps[pn]); CHECK(st == sc);
        if (st == ADF_OK)
        {
            CHECK(arb_contains_si(t, 1)); CHECK(arb_equal(t, c->t));
            CHECK(fmpz_is_one(c->u.c) && fmpz_is_zero(c->u.N)); okay++;
        }
        else { CHECK(st == ADF_NOT_DETERMINED && arb_equal_si(t, 97)); nd++; }
        cases++;
    }
    const ulong p = UWORD(18446744073709551557);
    CHECK(adf_place_prime(&place, p) == ADF_OK); fmpz_set_ui(prime, p);
    for (int sign = -1; sign <= 1; sign += 2) for (int e = 0; e < 5; e++)
    {
        fmpz_pow_ui(fmpq_numref(q->q), prime, e); fmpz_one(fmpq_denref(q->q));
        if (sign < 0) fmpq_inv(q->q, q->q);
        CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
        slong v = -99; CHECK(adf_idele_valuation_at(&v, x, place) == ADF_OK && v == sign * e);
        CHECK(adf_idele_abs_at(av, x, place) == ADF_OK);
        fmpq_inv(q->q, q->q); CHECK(fmpq_equal(q->q, av->q));
    }
    printf("rationals: cases=%lu OK=%lu NOT_DETERMINED=%lu big_prime_cases=10 assertions=%lu failures=0\n",
           cases, okay, nd, checks);
    adf_idele_clear(x); adf_idclass_clear(c); adf_rat_clear(q); adf_rat_clear(av);
    arb_clear(t); fmpz_clear(prime); flint_cleanup(); return 0;
}
