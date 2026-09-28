/* set_context_alias.c: adf_scaled_set_context(x, &lost, x, ctx2) with y aliasing x.
   scaled.h (adf_scaled_set_context) and conventions 5.4: "y may alias x" and "write *lost=1
   exactly when the set changes". Oracle: the set before and after, compared exactly with
   adf_fball_equal_set of adf_scaled_get_fball, and the non-aliased call on a copy.
   Exhaustive over K, K' in 1..24, u in 0..K-1 (s = 1).
   Build: cc -std=c11 -O0 -g -Iinclude set_context_alias.c build/libadelefeld.a -lflint -lgmp -lm */
#include <stdio.h>
#include <adelefeld.h>

static void mkscaled(adf_scaled_t x, ulong u, const adf_modctx_struct * ctx)
{
    /* x = 1 * (u + K Zhat): from the tight ball u + K Zhat, lossless in ctx of modulus K */
    adf_fball_t b;
    fmpz_t A, H, d, K;
    int lost = -1;
    fmpz_init(A); fmpz_init(H); fmpz_init(d); fmpz_init(K);
    adf_modctx_get_modulus(K, ctx);
    fmpz_set_ui(A, u); fmpz_set(H, K); fmpz_one(d);
    adf_fball_init(b);
    if (adf_fball_set_fmpz3(b, A, H, d) != ADF_OK) { printf("setup failed\n"); }
    adf_scaled_set_fball(x, &lost, b, ctx);
    adf_fball_clear(b);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); fmpz_clear(K);
}

int main(void)
{
    ulong K1, K2, u;
    long cases = 0, bad_alias = 0, bad_plain = 0, printed = 0, false0 = 0, false1 = 0;
    for (K1 = 1; K1 <= 24; K1++)
        for (K2 = 1; K2 <= 24; K2++)
        {
            adf_modctx_struct *c1 = NULL, *c2 = NULL;
            fmpz_t KK;
            fmpz_init_set_ui(KK, K1);
            adf_modctx_new_fmpz(&c1, KK);
            fmpz_set_ui(KK, K2);
            adf_modctx_new_fmpz(&c2, KK);
            fmpz_clear(KK);
            for (u = 0; u < K1; u++)
            {
                adf_scaled_t x, y;
                adf_fball_t before, after;
                int lost_alias = -1, lost_plain = -1, changed;
                adf_scaled_init(x, c1);
                adf_scaled_init(y, c1);
                mkscaled(x, u, c1);
                adf_fball_init(before); adf_fball_init(after);
                adf_scaled_get_fball(before, x);
                /* plain call */
                adf_scaled_set_context(y, &lost_plain, x, c2);
                /* aliased call */
                adf_scaled_set_context(x, &lost_alias, x, c2);
                adf_scaled_get_fball(after, x);
                changed = !adf_fball_equal_set(before, after);
                cases++;
                if (lost_plain != changed) bad_plain++;
                if (lost_alias != changed)
                {
                    bad_alias++;
                    if (changed) false0++; else false1++;
                    if (printed++ < 5)
                        printf("K=%lu u=%lu K'=%lu: set changed=%d, lost(plain)=%d, lost(y=x)=%d\n",
                               K1, u, K2, changed, lost_plain, lost_alias);
                }
                adf_fball_clear(before); adf_fball_clear(after);
                adf_scaled_clear(x); adf_scaled_clear(y);
            }
            adf_modctx_free(c1); adf_modctx_free(c2);
        }
    printf("cases %ld, wrong lost (plain) %ld, wrong lost (y = x) %ld\n", cases, bad_plain, bad_alias);
    printf("  of these: set changed but lost = 0: %ld; set unchanged but lost = 1: %ld\n", false0, false1);
    return bad_alias || bad_plain;
}
