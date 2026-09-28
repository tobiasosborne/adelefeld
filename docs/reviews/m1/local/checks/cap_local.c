/* R1: valid local inputs rejected by all five cap functions. */
#include <adelefeld.h>
#include <assert.h>
#include <stdio.h>

int main(void)
{
    ulong block = 4;
    adf_modctx_struct *ctx = NULL;
    adf_fball_t x, y, out, keep;
    adf_rat_t C, q;
    fmpz_t A, H, d;
    int st[5], unchanged = 0;
    adf_fball_init(x); adf_fball_init(y); adf_fball_init(out); adf_fball_init(keep);
    adf_rat_init(C); adf_rat_init(q);
    fmpz_init_set_ui(A, 2); fmpz_init_set_ui(H, 4); fmpz_init_set_ui(d, 2);
    assert(adf_modctx_new_blocks(&ctx, &block, 1) == ADF_OK);
    assert(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    assert(adf_fball_set_local(x, x, ctx) == ADF_OK);
    assert(adf_fball_is_canonical(x));
    adf_fball_set(y, x);
    adf_fball_set_si(keep, 7);
    fmpq_set_ui(C->q, 1, 1);
    fmpq_set_ui(q->q, 1, 1);
    adf_fball_set(out, keep);
    st[0] = adf_fball_cap(out, x, C);
    unchanged += adf_fball_identical(out, keep);
    st[1] = adf_fball_add_cap(out, x, y, C);
    unchanged += adf_fball_identical(out, keep);
    st[2] = adf_fball_sub_cap(out, x, y, C);
    unchanged += adf_fball_identical(out, keep);
    st[3] = adf_fball_mul_cap(out, x, y, C);
    unchanged += adf_fball_identical(out, keep);
    st[4] = adf_fball_mul_rat_cap(out, x, q, C);
    unchanged += adf_fball_identical(out, keep);
    printf("local canonical=1; expected statuses=0,0,0,0,0; actual=%d,%d,%d,%d,%d; untouched=%d/5\n",
           st[0], st[1], st[2], st[3], st[4], unchanged);
    /* Same sets after explicit global conversion are accepted by every function. */
    adf_fball_set_global(x, x); adf_fball_set_global(y, y);
    st[0] = adf_fball_cap(out, x, C);
    st[1] = adf_fball_add_cap(out, x, y, C);
    st[2] = adf_fball_sub_cap(out, x, y, C);
    st[3] = adf_fball_mul_cap(out, x, y, C);
    st[4] = adf_fball_mul_rat_cap(out, x, q, C);
    printf("same sets global: statuses=%d,%d,%d,%d,%d\n", st[0], st[1], st[2], st[3], st[4]);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
    adf_rat_clear(C); adf_rat_clear(q);
    adf_fball_clear(x); adf_fball_clear(y); adf_fball_clear(out); adf_fball_clear(keep);
    adf_modctx_free(ctx);
    flint_cleanup();
    return 0;
}
