/* Local-backend conversion and output replacement, including context borrow lifetime. */
#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); abort(); } } while (0)
int main(void)
{
    const ulong blocks[] = {8, 9, 5};
    const slong hs[] = {1, 2, 3, 6, 12, 72, 360};
    adf_modctx_struct *ctx = NULL;
    adf_adele_t g, l, zg, zl, ali;
    adf_idele_t x, old;
    fmpz_t a, h, d, c, n;
    unsigned long checks = 0, cases = 0;
    adf_adele_init(g); adf_adele_init(l); adf_adele_init(zg); adf_adele_init(zl); adf_adele_init(ali);
    adf_idele_init(x); adf_idele_init(old);
    fmpz_init(a); fmpz_init(h); fmpz_init(d); fmpz_init(c); fmpz_init(n);
    CHECK(adf_modctx_new_blocks(&ctx, blocks, 3) == ADF_OK);
    for (int ai = -17; ai <= 17; ai++) for (int hi = 0; hi < 7; hi++) for (int di = 1; di <= 7; di++)
    {
        fmpz_set_si(a, ai); fmpz_set_si(h, hs[hi]); fmpz_set_si(d, di);
        CHECK(adf_fball_set_fmpz3(&g->fin, a, h, d) == ADF_OK);
        CHECK(adf_fball_set_local(&l->fin, &g->fin, ctx) == ADF_OK);
        arb_set_si(g->inf, ai); arb_set(l->inf, g->inf);
        CHECK(adf_idele_set_adele(old, l) == (ai == 0 ? ADF_NOT_UNIT : ADF_UNIT_NOT_CERTIFIED));
        arb_set_si(x->inf, -3); fmpq_set_si(x->r, hi % 2 ? 6 : 35, hi % 2 ? 35 : 6);
        fmpz_set_si(c, hi % 2 ? 5 : 3); fmpz_set_si(n, hi % 2 ? 6 : 8);
        CHECK(adf_ucoset_set_fmpz2(&x->u, c, n) == ADF_OK);
        CHECK(adf_adele_div_idele(zg, g, x, 32) == ADF_OK);
        CHECK(adf_adele_div_idele(zl, l, x, 32) == ADF_OK);
        CHECK(adf_adele_identical(zg, zl)); CHECK(zl->fin.backend == ADF_GLOBAL);
        adf_adele_set(ali, l); CHECK(adf_adele_div_idele(ali, ali, x, 32) == ADF_OK);
        CHECK(adf_adele_identical(ali, zg));
        adf_adele_set(zl, l); adf_adele_set_idele(zl, x); CHECK(zl->fin.backend == ADF_GLOBAL);
        adf_adele_set(zl, l); adf_adele_set_idele_simple(zl, x); CHECK(zl->fin.backend == ADF_GLOBAL);
        CHECK(adf_adele_is_canonical(zl)); cases++;
    }
    adf_adele_clear(g); adf_adele_clear(l); adf_adele_clear(zg); adf_adele_clear(zl); adf_adele_clear(ali);
    adf_idele_clear(x); adf_idele_clear(old); adf_modctx_free(ctx);
    fmpz_clear(a); fmpz_clear(h); fmpz_clear(d); fmpz_clear(c); fmpz_clear(n); flint_cleanup();
    printf("local: cases=%lu assertions=%lu failures=0\n", cases, checks); return 0;
}
