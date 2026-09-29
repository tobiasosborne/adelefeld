/* i-review1: constructors on raw data, predicates on garbage, states on non-OK status, aliasing. */
#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); fails++; } } while (0)

int main(void)
{
    adf_idele_t x, y, z; arb_t b; fmpq_t q; adf_ucoset_t u; int st;
    adf_idele_init(x); adf_idele_init(y); adf_idele_init(z); arb_init(b); fmpq_init(q); adf_ucoset_init(u);

    /* set_parts: DOMAIN cases leave x untouched */
    arb_set_si(b, 5); fmpq_set_si(q, 3, 2);
    CHECK(adf_idele_set_parts(x, b, q, u) == ADF_OK);
    CHECK(fmpq_cmp_si(x->r, 3) < 0 && adf_idele_is_canonical(x));
    adf_idele_set(y, x);
    arb_zero(b); CHECK(adf_idele_set_parts(x, b, q, u) == ADF_DOMAIN); CHECK(adf_idele_identical(x, y));
    arb_set_si(b, 1); mag_set_ui(arb_radref(b), 1); CHECK(adf_idele_set_parts(x, b, q, u) == ADF_DOMAIN);
    mag_set_ui(arb_radref(b), 2); CHECK(adf_idele_set_parts(x, b, q, u) == ADF_DOMAIN);
    arb_set_si(b, 1); mag_inf(arb_radref(b)); CHECK(adf_idele_set_parts(x, b, q, u) == ADF_DOMAIN);
    arb_indeterminate(b); CHECK(adf_idele_set_parts(x, b, q, u) == ADF_DOMAIN);
    arb_set_si(b, 1);
    fmpq_zero(q); CHECK(adf_idele_set_parts(x, b, q, u) == ADF_DOMAIN);
    fmpq_set_si(q, -3, 2); CHECK(adf_idele_set_parts(x, b, q, u) == ADF_DOMAIN);
    fmpz_zero(fmpq_denref(q)); fmpz_one(fmpq_numref(q)); CHECK(adf_idele_set_parts(x, b, q, u) == ADF_DOMAIN);
    /* non-canonical fmpq: 6/4, and -6/-4 (both negative) */
    fmpz_set_si(fmpq_numref(q), 6); fmpz_set_si(fmpq_denref(q), 4);
    CHECK(adf_idele_set_parts(x, b, q, u) == ADF_OK);
    CHECK(fmpz_cmp_si(fmpq_numref(x->r), 3) == 0 && fmpz_cmp_si(fmpq_denref(x->r), 2) == 0);
    fmpz_set_si(fmpq_numref(q), -6); fmpz_set_si(fmpq_denref(q), -4);
    st = adf_idele_set_parts(x, b, q, u);
    printf("set_parts r=-6/-4 (noncanonical, = 3/2): status %d, is_canonical %d\n", st, adf_idele_is_canonical(x));
    if (st == ADF_OK) CHECK(adf_idele_is_canonical(x));
    else printf("  (DOMAIN for a negative denominator: the header says r may be any fmpq with a nonzero denominator)\n");

    /* is_canonical / identical on garbage: no abort */
    adf_idele_init(z);
    fmpz_zero(fmpq_denref(z->r)); CHECK(!adf_idele_is_canonical(z));
    fmpz_set_si(fmpq_denref(z->r), -2); fmpz_set_si(fmpq_numref(z->r), -1); CHECK(!adf_idele_is_canonical(z));
    fmpq_one(z->r); arb_zero(z->inf); CHECK(!adf_idele_is_canonical(z)); arb_one(z->inf);
    fmpz_set_si(z->u.N, -3); CHECK(!adf_ucoset_is_canonical(&z->u)); CHECK(!adf_idele_is_canonical(z));
    fmpz_set_si(z->u.N, 0); fmpz_set_si(z->u.c, 7); CHECK(!adf_ucoset_is_canonical(&z->u));
    fmpz_set_si(z->u.N, 4); fmpz_set_si(z->u.c, 2); CHECK(!adf_ucoset_is_canonical(&z->u));
    CHECK(!adf_ucoset_is_normal(&z->u));
    fmpz_set_si(z->u.N, 6); fmpz_set_si(z->u.c, 5); CHECK(adf_ucoset_is_canonical(&z->u) && !adf_ucoset_is_normal(&z->u));
    fmpz_set_si(z->u.N, 2); fmpz_set_si(z->u.c, 1);
    fmpz_set_si(z->u.N, 1); fmpz_set_si(z->u.c, 1); CHECK(adf_ucoset_is_canonical(&z->u));
    fmpz_set_si(z->u.N, 1); fmpz_set_si(z->u.c, 0); CHECK(!adf_ucoset_is_canonical(&z->u));
    fmpz_set_si(z->u.N, 0); fmpz_set_si(z->u.c, 0); CHECK(!adf_ucoset_is_canonical(&z->u));
    fmpz_set_si(z->u.N, 0); fmpz_set_si(z->u.c, 1);

    /* set_rat: q > 0, q < 0, exact, aliasing of nothing; output on NOT_UNIT untouched */
    {
        adf_rat_t r; adf_rat_init(r);
        adf_idele_set(x, y);
        CHECK(adf_idele_set_rat(x, r, 64) == ADF_NOT_UNIT); CHECK(adf_idele_identical(x, y));
        adf_rat_set_si(r, -7);
        CHECK(adf_idele_set_rat(x, r, 2) == ADF_OK);
        CHECK(arb_contains_si(x->inf, -7) && arb_is_nonzero(x->inf)); CHECK(fmpz_equal_si(x->u.c, -1) && fmpz_is_zero(x->u.N));
        CHECK(fmpq_cmp_si(x->r, 7) == 0);
        adf_rat_clear(r);
    }

    /* mul / inv of exact unit -1 with -1: unit exact [1] */
    adf_ucoset_minus_one(&z->u); arb_set_si(z->inf, -1);
    CHECK(adf_idele_is_canonical(z));
    CHECK(adf_idele_mul(x, z, z, 2) == ADF_OK);
    CHECK(fmpz_is_one(x->u.c) && fmpz_is_zero(x->u.N) && arb_is_one(x->inf));
    CHECK(adf_idele_inv(x, z, 2) == ADF_OK);
    CHECK(fmpz_equal_si(x->u.c, -1) && fmpz_is_zero(x->u.N) && arb_equal(x->inf, z->inf));

    /* get_unit with u the destination; get_fmpz2 aliasing */
    {
        adf_ucoset_t w; fmpz_t c, N; adf_ucoset_init(w); fmpz_init(c); fmpz_init(N);
        fmpz_set_si(w->c, 5); fmpz_set_si(w->N, 6);
        adf_ucoset_get_fmpz2(w->N, w->c, w);   /* swapped members: allowed by header */
        printf("get_fmpz2 into (w->N, w->c): c'=%ld N'=%ld (from (5,6): c = N-slot gets c=5, N-slot gets N=6)\n",
               (long) fmpz_get_si(w->N), (long) fmpz_get_si(w->c));
        adf_ucoset_clear(w); fmpz_clear(c); fmpz_clear(N);
    }

    adf_idele_clear(x); adf_idele_clear(y); adf_idele_clear(z); arb_clear(b); fmpq_clear(q); adf_ucoset_clear(u);
    printf("misc: %d failures\n", fails);
    return fails != 0;
}
