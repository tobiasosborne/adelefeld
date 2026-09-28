/* Enumeration oracle uses the input integers and rational normalization, not fball getters. */
#include <stdio.h>
#include <string.h>
#include "adelefeld.h"

int main(void)
{
    adf_modctx_struct *ctx = NULL;
    adf_fball_t g, l, y;
    fmpz_t A, H, d;
    fmpq_t a, n;
    ulong q = 12;
    int count = 0;
    adf_modctx_new_blocks(&ctx, &q, 1);
    adf_fball_init(g); adf_fball_init(l); adf_fball_init(y);
    fmpz_init(A); fmpz_init_set_ui(H, 12); fmpz_init(d); fmpq_init(a); fmpq_init(n);
    for (long i = -25; i <= 25; i++) for (long den = 1; den <= 12; den++) {
        char expected[120], *as, *ns, *got;
        size_t len;
        fmpz_set_si(A, i); fmpz_set_si(d, den);
        if (adf_fball_set_fmpz3(g, A, H, d) || adf_fball_set_local(l, g, ctx)) return 1;
        fmpq_set_si(a, (i % 12 + 12) % 12, den); fmpq_set_si(n, 12, den);
        as = fmpq_get_str(NULL, 10, a); ns = fmpq_get_str(NULL, 10, n);
        snprintf(expected, sizeof(expected), "(* ; %s mod %s)", as, ns);
        got = adf_fball_get_str(&len, l);
        if (strcmp(got, expected) || len != strlen(expected)) return 2;
        if (adf_fball_set_str(y, got, len, NULL) || !adf_fball_equal_set(y, g)) return 3;
        adf_str_free(got); flint_free(as); flint_free(ns); count++;
    }
    printf("local_print_cases=%d failures=0\n", count);
    adf_fball_clear(g); adf_fball_clear(l); adf_fball_clear(y); adf_modctx_free(ctx);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); fmpq_clear(a); fmpq_clear(n); flint_cleanup();
    return 0;
}
