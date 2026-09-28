/* R4: reproduce the untouched fixture at tests/test_fball.c:1012-1014. */
#include <adelefeld.h>
#include <stdio.h>
int main(void)
{
    adf_fball_t x;
    char dummy = 0;
    adf_fball_init(x);
    fmpz_set_ui(x->A, 2);
    fmpz_set_ui(x->H, 4);
    fmpz_set_ui(x->d, 2);
    x->backend = ADF_LOCAL;
    x->mctx = (const adf_modctx_struct *) (const void *) &dummy;
    x->res = flint_malloc(2 * sizeof(ulong));
    x->res[0] = 0;
    x->res[1] = 1;
    int before = adf_fball_is_canonical(x); /* Rejects A != 0 before reading mctx. */
    int status = adf_fball_canonicalise(x);
    int pinned = status == 0 && fmpz_equal_ui(x->A, 2) &&
                 fmpz_equal_ui(x->H, 4) && fmpz_equal_ui(x->d, 2);
    printf("fixture satisfies L=%d; canonicalise status=%d; original assertions pass=%d\n",
           before, status, pinned);
    adf_fball_clear(x);
    flint_cleanup();
    return 0;
}
