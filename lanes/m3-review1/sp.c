/* Lane m3-review1: set_pieces edges (hunt 3). Prints one line per probe. */
#include <adelefeld.h>
#include <stdio.h>
#include <string.h>

static void mk(adf_adele_struct *a, const char *s)
{
    if (adf_adele_set_str(a, s, strlen(s), 128, NULL) != ADF_OK) { printf("bad %s\n", s); }
}
static void sentinel(adf_qclass_t s)
{
    adf_adele_t a; adf_adele_init(a); mk(a, "(-13.75 +/- 0.03125 ; 5/7 mod 9)");
    adf_qclass_set_adele(s, a); adf_adele_clear(a);
}
int main(void)
{
    ulong b1[3] = {8, 9, 5};
    adf_modctx_struct *ctx; adf_adele_struct a[4]; adf_qclass_t y, s; int i, st;
    adf_modctx_new_blocks(&ctx, b1, 3);
    for (i = 0; i < 4; i++) adf_adele_init(a + i);
    adf_qclass_init(y); adf_qclass_init(s); sentinel(s);
    mk(a + 0, "(0.5 ; 7 mod 360)");
    mk(a + 1, "(0.5 ; 7 mod 360)");
    adf_fball_set_local(&a[1].fin, &a[1].fin, ctx);
    /* duplicates: count before dedup */
    adf_qclass_set(y, s); st = adf_qclass_set_pieces(y, a, 2, 1);
    printf("dup n=2 limit=1: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    adf_qclass_set(y, s); st = adf_qclass_set_pieces(y, a, 2, 2);
    printf("dup n=2 limit=2: st %d len %ld backend0 %d canon %d\n", st, (long) y->len, y->piece[0].fin.backend,
           adf_qclass_is_canonical(y));
    adf_qclass_set(y, s); st = adf_qclass_set_pieces(y, a + 0, 1, 1);
    {   /* reversed order keeps the local one */
        adf_adele_struct r[2]; adf_adele_init(r); adf_adele_init(r + 1);
        adf_adele_set(r, a + 1); adf_adele_set(r + 1, a + 0);
        st = adf_qclass_set_pieces(y, r, 2, 2);
        printf("dup reversed: st %d len %ld backend0 %d\n", st, (long) y->len, y->piece[0].fin.backend);
        adf_adele_clear(r); adf_adele_clear(r + 1);
    }
    /* piece_limit 0 and negative with NULL pieces */
    adf_qclass_set(y, s); st = adf_qclass_set_pieces(y, NULL, 5, 0);
    printf("limit 0 NULL: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    st = adf_qclass_set_pieces(y, NULL, 0, 5);
    printf("n 0 NULL: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    st = adf_qclass_set_pieces(y, NULL, -3, 5);
    printf("n -3 NULL: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    /* non-finite entry */
    mk(a + 2, "(0.5 ; 1 mod 2)");
    arf_pos_inf(arb_midref(a[2].inf));
    st = adf_qclass_set_pieces(y, a + 2, 1, 1);
    printf("mid +inf: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    mk(a + 2, "(0.5 ; 1 mod 2)");
    mag_inf(arb_radref(a[2].inf));
    st = adf_qclass_set_pieces(y, a + 2, 1, 1);
    printf("rad inf: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    arf_nan(arb_midref(a[2].inf)); mag_zero(arb_radref(a[2].inf));
    st = adf_qclass_set_pieces(y, a + 2, 1, 1);
    printf("mid nan: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    /* midpoint 1 + 2^-200 (DOMAIN), midpoint 1 with spill (OK) */
    mk(a + 2, "(1 ; 1 mod 2)"); arf_set_ui_2exp_si(arb_midref(a[2].inf), 1, -200);
    arf_add_ui(arb_midref(a[2].inf), arb_midref(a[2].inf), 1, ARF_PREC_EXACT, ARF_RND_NEAR);
    st = adf_qclass_set_pieces(y, a + 2, 1, 1);
    printf("mid 1+2^-200: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    mk(a + 2, "(1 +/- 5 ; 1 mod 2)");
    st = adf_qclass_set_pieces(y, a + 2, 1, 1);
    printf("mid 1 spill 5: st %d canon %d\n", st, adf_qclass_is_canonical(y));
    /* finite part with d = 2 (DOMAIN), raw non-canonical global triple (DOMAIN or OK?) */
    mk(a + 2, "(0.5 ; 1/2 mod 1)");
    adf_qclass_set(y, s); st = adf_qclass_set_pieces(y, a + 2, 1, 1);
    printf("d=2: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    mk(a + 2, "(0.5 ; 1 mod 2)"); fmpz_set_ui(a[2].fin.A, 3);   /* raw A >= H */
    st = adf_qclass_set_pieces(y, a + 2, 1, 1);
    printf("raw A=3 H=2: st %d untouched %d\n", st, adf_qclass_identical(y, s));
    mk(a + 2, "(0.5 ; 1 mod 2)"); fmpz_set_ui(a[2].fin.A, 2); fmpz_set_ui(a[2].fin.H, 4); fmpz_set_ui(a[2].fin.d, 2);
    st = adf_qclass_set_pieces(y, a + 2, 1, 1);
    printf("raw (2,4,2): st %d untouched %d canon %d\n", st, adf_qclass_identical(y, s), adf_qclass_is_canonical(y));
    for (i = 0; i < 4; i++) adf_adele_clear(a + i);
    adf_qclass_clear(y); adf_qclass_clear(s); adf_modctx_free(ctx); flint_cleanup_master();
    return 0;
}
