/* borrow.c: set_adele and set with a local finite part keep the borrowed context pointer, after the
   source adele is cleared. */
#include <adelefeld.h>
#include <stdio.h>
int main(void)
{
    adf_modctx_struct *ctx = NULL;
    ulong q[2] = {2, 3};
    adf_qclass_t x, y;
    adf_adele_struct aa[1];
    adf_adele_struct * a = aa;
    int bad = 0;
    if (adf_modctx_new_blocks(&ctx, q, 2) != ADF_OK) return 2;
    adf_adele_init(a);
    a->fin.backend = ADF_LOCAL;
    a->fin.mctx = ctx;
    a->fin.res = flint_calloc(2, sizeof(ulong));
    a->fin.res[0] = 1; a->fin.res[1] = 2;
    fmpz_set_ui(a->fin.H, 6);
    fmpz_set_ui(a->fin.d, 1);                 /* the canonical triple is (1, 6, 1) */
    arf_set_si(arb_midref(a->inf), 1);
    arf_mul_2exp_si(arb_midref(a->inf), arb_midref(a->inf), -1);
    mag_zero(arb_radref(a->inf));
    adf_qclass_init(x);
    adf_qclass_init(y);
    adf_qclass_set_adele(x, a);
    adf_adele_clear(a);                        /* the source goes away */
    if (x->piece[0].fin.backend != ADF_LOCAL || x->piece[0].fin.mctx != ctx) {
        printf("SET ADELE LOST THE CONTEXT\n"); bad++;
    }
    if (x->piece[0].fin.res[0] != 1 || x->piece[0].fin.res[1] != 2) {
        printf("SET ADELE LOST THE RESIDUES\n"); bad++;
    }
    if (!adf_qclass_is_canonical(x)) { printf("NOT CANONICAL\n"); bad++; }
    adf_qclass_set(y, x);
    if (y->piece[0].fin.mctx != ctx) { printf("SET LOST THE CONTEXT\n"); bad++; }
    if (!adf_qclass_identical(y, x)) { printf("NOT IDENTICAL\n"); bad++; }
    printf("problems %d\n", bad);
    adf_qclass_clear(x);
    adf_qclass_clear(y);
    adf_modctx_free(ctx);
    return bad ? 1 : 0;
}
