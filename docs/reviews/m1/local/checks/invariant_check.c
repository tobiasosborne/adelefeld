/* R3: build both this file and the library with -DADF_CHECK_INVARIANTS. */
#include <adelefeld.h>
#include <assert.h>
#include <stdio.h>
#ifndef ADF_CHECK_INVARIANTS
#error This reproducer requires ADF_CHECK_INVARIANTS
#endif
int main(void)
{
    adf_modctx_struct *ctx = NULL;
    ulong block = 4;
    adf_fball_t x, y;
    fmpz_t A, H, d;
    adf_fball_init(x); adf_fball_init(y);
    fmpz_init_set_ui(A, 1); fmpz_init_set_ui(H, 4); fmpz_init_set_ui(d, 1);
    assert(adf_modctx_new_blocks(&ctx, &block, 1) == 0);
    assert(adf_fball_set_fmpz3(x, A, H, d) == 0);
    assert(adf_fball_set_local(x, x, ctx) == 0);
    assert(adf_fball_is_canonical(x));
    fmpz_zero(x->d); /* A readable object that violates predicate L. */
    assert(!adf_fball_is_canonical(x));
    adf_fball_neg(y, x); /* conventions.md:288 requires flint_abort with a message. */
    printf("ADF_CHECK_INVARIANTS=1; neg returned; output canonical=%d\n", adf_fball_is_canonical(y));
    adf_fball_clear(x); adf_fball_clear(y); adf_modctx_free(ctx);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
    flint_cleanup();
    return 0;
}
