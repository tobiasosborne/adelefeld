/* lanes/q-review2/probe_endcmp.c: the end_cmp of src/qclass.c on a pair of real balls, against an
   exact endpoint subtraction at high precision. */
#include <adelefeld.h>
#include <stdio.h>

static void set_rad(mag_t r, ulong man, slong e) { mag_set_ui_2exp_si(r, man, e); }

static void set_pow2(arf_t r, slong man, mp_limb_signed_t e)
{
    fmpz_t m, x;
    fmpz_init(m); fmpz_init(x);
    fmpz_set_si(m, man); fmpz_set_si(x, (slong) e);
    arf_set_fmpz_2exp(r, m, x);
    fmpz_clear(m); fmpz_clear(x);
}

/* the code of src/qclass.c end_cmp */
static int end_cmp(arb_srcptr x, arb_srcptr y, int upper)
{
    arf_struct terms[4];
    arf_t sum;
    int i, c;
    for (i = 0; i < 4; i++) arf_init(terms + i);
    arf_init(sum);
    arf_set(terms, arb_midref(x));
    arf_neg(terms + 1, arb_midref(y));
    arf_set_mag(terms + 2, arb_radref(x));
    arf_set_mag(terms + 3, arb_radref(y));
    if (!upper) arf_neg(terms + 2, terms + 2);
    if (upper) arf_neg(terms + 3, terms + 3);
    arf_sum(sum, terms, 4, 2, ARF_RND_DOWN);
    c = arf_sgn(sum);
    arf_clear(sum);
    for (i = 0; i < 4; i++) arf_clear(terms + i);
    return c;
}

int main(void)
{
    arb_t x, y;
    arf_t lo_x, hi_x, lo_y, hi_y, d;
    slong K = 60;
    arb_init(x); arb_init(y);
    arf_t rx, ry;
    arf_init(lo_x); arf_init(hi_x); arf_init(lo_y); arf_init(hi_y); arf_init(d);
    arf_init(rx); arf_init(ry);
    /* x = 1/2 +/- 2^-K ; y = 1/2 + 2^-K +/- 2^(1-K) */
    set_pow2(arb_midref(x), 1, -1); set_rad(arb_radref(x), 1, -K);
    {
        fmpz_t man, ex;
        fmpz_init(man); fmpz_init(ex);
        fmpz_setbit(man, K - 1);
        fmpz_add_ui(man, man, 1);      /* 2^(K-1) + 1 */
        fmpz_set_si(ex, -K);
        arf_set_fmpz_2exp(arb_midref(y), man, ex);
        fmpz_clear(man); fmpz_clear(ex);
    }
    set_rad(arb_radref(y), 2, 1 - K);
    arf_set_mag(rx, arb_radref(x));
    arf_set_mag(ry, arb_radref(y));
    /* exact endpoints: lo = mid - rad, computed exactly */
    arf_sub(lo_x, arb_midref(x), rx, 2048, ARF_RND_DOWN);
    arf_sub(hi_x, arb_midref(x), rx, 2048, ARF_RND_UP);
    arf_sub(lo_y, arb_midref(y), ry, 2048, ARF_RND_DOWN);
    arf_sub(hi_y, arb_midref(y), ry, 2048, ARF_RND_UP);
    arf_sub(d, &lo_x, &lo_y, 2048, ARF_RND_DOWN);
    printf("K = %ld\n", (long) K);
    printf("end_cmp lower = %2d   exact sign of lo_x - lo_y = %2d   equal = %d\n",
           end_cmp(x, y, 0), arf_sgn(d), arf_equal(lo_x, lo_y));
    arf_sub(d, &hi_x, &hi_y, 2048, ARF_RND_DOWN);
    printf("end_cmp upper = %2d   exact sign of hi_x - hi_y = %2d\n",
           end_cmp(x, y, 1), arf_sgn(d));
    arf_sub(d, &lo_y, &lo_x, 2048, ARF_RND_DOWN);
    printf("reversed lower = %2d   exact = %2d\n", end_cmp(y, x, 0), arf_sgn(d));
    /* print the endpoints as decimals */
    {
        fmpz_t mm, ee; int i; arf_struct * v[5];
        fmpz_init(mm); fmpz_init(ee);
        v[0] = lo_x; v[1] = hi_x; v[2] = lo_y; v[3] = hi_y;
        v[4] = arb_midref(x);
        for (i = 0; i < 5; i++) {
            char *sm, *se;
            arf_get_fmpz_2exp(mm, ee, v[i]);
            sm = fmpz_get_str(NULL, 10, mm); se = fmpz_get_str(NULL, 10, ee);
            printf("v[%d] = %s * 2^%s\n", i, sm, se);
            flint_free(sm); flint_free(se);
        }
        printf("rad_x man %lu exp %ld, rad_y man %lu exp %ld\n",
               (ulong) arb_radref(x)->man, arb_radref(x)->exp,
               (ulong) arb_radref(y)->man, arb_radref(y)->exp);
    }
    return 0;
}
