/* get_fball_into_local.c: adf_scaled_get_fball(y, x) into an output y that holds a local value
   (scaled.h: "y = the set of x as a global canonical adf_fball"). y must come out global and
   canonical, and the residue array of the old local value must be released (run under
   valgrind or -fsanitize=address). Also adf_scaled_set_fball from a local x whose context
   differs from the target context.
   Build: cc -std=c11 -O0 -g -Iinclude get_fball_into_local.c build/libadelefeld.a -lflint -lgmp -lm */
#include <stdio.h>
#include <adelefeld.h>

int main(void)
{
    ulong q[2] = {4, 9};
    adf_modctx_struct * lc = NULL, * sc = NULL;
    adf_fball_t g, y;
    adf_scaled_t x;
    adf_rat_t c, r;
    fmpz_t K;
    int bad = 0, i;

    adf_modctx_new_blocks(&lc, q, 2);
    fmpz_init_set_ui(K, 10);
    adf_modctx_new_fmpz(&sc, K);
    adf_fball_init(g); adf_fball_init(y);
    adf_rat_init(c); adf_rat_init(r);
    adf_scaled_init(x, sc);
    for (i = 0; i < 2; i++)
    {
        adf_rat_set_si(c, 5); adf_rat_set_si(r, 36);
        adf_fball_set_center_radius(g, c, r);
        if (adf_fball_set_local(y, g, lc) != ADF_OK || !adf_fball_is_local(y)) { printf("setup\n"); return 2; }
        /* x = 3 (7 + 10 Zhat) */
        adf_rat_set_si(c, 21); adf_rat_set_si(r, 30);
        adf_fball_set_center_radius(g, c, r);
        adf_scaled_set_fball(x, NULL, g, sc);
        if (i == 1) adf_scaled_set_rat(x, c, sc);   /* exact case */
        adf_scaled_get_fball(y, x);
        if (adf_fball_is_local(y) || !adf_fball_is_canonical(y)) { bad++; printf("case %d: output not global canonical\n", i); }
        {
            adf_fball_t want;
            adf_fball_init(want);
            if (i == 0) adf_fball_set_center_radius(want, c, r); else adf_fball_set_rat(want, c);
            if (!adf_fball_identical(y, want)) { bad++; printf("case %d: wrong set\n", i); }
            adf_fball_clear(want);
        }
    }
    printf("get_fball into a local output: %s\n", bad ? "FAILED" : "ok");
    adf_scaled_clear(x);
    adf_fball_clear(g); adf_fball_clear(y);
    adf_rat_clear(c); adf_rat_clear(r);
    fmpz_clear(K);
    adf_modctx_free(lc); adf_modctx_free(sc);
    return bad;
}
