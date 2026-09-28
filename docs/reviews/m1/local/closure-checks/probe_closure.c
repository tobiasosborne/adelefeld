/* R2 after M1-D2: malformed fields with a live context and a full residue allocation. */
#include <adelefeld.h>
#include <assert.h>
#include <stdio.h>

int main(void)
{
    adf_modctx_struct *ctx = NULL;
    adf_fball_t x;
    fmpz_t A, H, d;
    ulong block = 4;
    const adf_modctx_struct *saved;
    int checks = 0;

    assert(adf_modctx_new_blocks(&ctx, &block, 1) == ADF_OK);
    adf_fball_init(x);
    fmpz_init_set_ui(A, 2);
    fmpz_init_set_ui(H, 4);
    fmpz_init_set_ui(d, 2);
    assert(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    assert(adf_fball_set_local(x, x, ctx) == ADF_OK);
    assert(adf_fball_is_canonical(x)); checks++;
    fmpz_zero(x->d);
    assert(!adf_fball_is_canonical(x)); checks++;
    fmpz_set_ui(x->d, 2);
    fmpz_set_ui(x->H, 5);
    assert(!adf_fball_is_canonical(x)); checks++;
    fmpz_set_ui(x->H, 4);
    x->res[0] = 4;
    assert(!adf_fball_is_canonical(x)); checks++;
    x->res[0] = 2;
    saved = x->mctx;
    x->mctx = NULL;
    assert(!adf_fball_is_canonical(x)); checks++;
    x->mctx = saved;
    x->backend = 77;
    assert(!adf_fball_is_canonical(x)); checks++;
    x->backend = ADF_LOCAL;
    assert(adf_fball_is_canonical(x)); checks++;

    adf_fball_clear(x);
    adf_modctx_free(ctx);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
    flint_cleanup();
    printf("live-pointer predicate checks=%d failures=0\n", checks);
    return 0;
}
