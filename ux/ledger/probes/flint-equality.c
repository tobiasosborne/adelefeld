/* Copied from lanes/ux-drive-ledger/flint-equality.c (the test drive of the Ledger, D4), unchanged below this
   comment. The evidence of claim a6. Build and run from the repository root:
     cc -std=c11 -Wall -Wextra -Werror ux/ledger/probes/flint-equality.c -lflint -lgmp -o /tmp/flint-equality
   It prints exp(3): 958, exp(12): 5125, precision: 8 8, padic_equal: 0 (FLINT 3.0.1). */
/* APIs: refs/src/flint-3.0.1/padic.rst:79-106, 120-145, 185, 263-266, 397-437.
   This is a direct reproduction of the displayed a6 claim, not a library change. */
#include <stdio.h>
#include <flint/padic.h>
int main(void)
{
    fmpz_t p;
    padic_ctx_t ctx;
    padic_t a, b, x, y;
    fmpz_init_set_ui(p, 3);
    padic_ctx_init(ctx, p, 0, 16, PADIC_TERSE);
    padic_init2(a, 8); padic_init2(b, 8);
    padic_init2(x, 8); padic_init2(y, 8);
    padic_set_si(a, 3, ctx); padic_set_si(b, 12, ctx);
    int okx = padic_exp(x, a, ctx);
    int oky = padic_exp(y, b, ctx);
    printf("exp success: %d %d\nexp(3): ", okx, oky);
    padic_print(x, ctx);
    printf("\nexp(12): "); padic_print(y, ctx);
    printf("\nprecision: %ld %ld\npadic_equal: %d\n",
           (long)padic_prec(x), (long)padic_prec(y), padic_equal(x, y));
    padic_clear(a); padic_clear(b); padic_clear(x); padic_clear(y);
    padic_ctx_clear(ctx); fmpz_clear(p); flint_cleanup();
    return 0;
}
