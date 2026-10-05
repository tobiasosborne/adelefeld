/* Search only. FLINT Gamma samples here locate candidates; only Z8 can certify a final witness. */
#include <adelefeld.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static mag_t search_B;
static acb_t search_candidate;
static int active, changed;
static slong search_shift, search_cb;
#include "build/search-source.c"

static uint64_t rng = 9041;
static ulong next(ulong n)
{
    rng = rng*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
    return (ulong) ((rng >> 24) % n);
}

static void sample(acb_t value, acb_t deriv, const acb_t pt)
{
    acb_t z, psi;
    arb_t lp;
    acb_init(z); acb_init(psi); arb_init(lp);
    arb_const_pi(lp, 384); arb_log(lp, lp, 384);
    acb_mul_2exp_si(z, pt, -1);
    acb_gamma(value, z, 384);
    acb_mul_arb(psi, z, lp, 384); acb_neg(psi, psi); acb_exp(psi, psi, 384);
    acb_mul(value, value, psi, 384);
    acb_digamma(psi, z, 384); acb_sub_arb(psi, psi, lp, 384);
    acb_mul(deriv, value, psi, 384); acb_mul_2exp_si(deriv, deriv, -1);
    acb_clear(z); acb_clear(psi); arb_clear(lp);
}

int main(int argc, char **argv)
{
    ulong count = argc > 1 ? strtoul(argv[1], NULL, 10) : 20000;
    if (argc > 2) rng = strtoull(argv[2], NULL, 10);
    acb_t s, y, pt, value, deriv;
    arf_t r;
    mag_t d, largest;
    ulong ok = 0, nd = 0, limit = 0, refined = 0, tighter = 0, values = 0, misses = 0, bad_B = 0;
    ulong unresolved = 0, unresolved_tighter = 0;
    acb_init(s); acb_init(y); acb_init(pt); acb_init(value); acb_init(deriv);
    acb_init(search_candidate); mag_init(search_B); arf_init(r); mag_init(d); mag_init(largest);
    for (ulong i = 0; i < count; i++)
    {
        slong x, xe, iy, ie, re, im;
        int mode = (int) (i % 4);
        if (mode == 0)
        {
            /* Near all poles inside the refinement cap, on both real sides. */
            xe = -(slong) (1+next(30));
            x = -2*(slong) next(63)*(WORD(1) << -xe)+(next(2) ? 1 : -1);
            re = xe-1-(slong) next(18);
            iy = next(3) ? (slong) (1+next(8)) : 0; ie = xe;
            im = re-(slong) next(8);
        }
        else if (mode == 1)
        {
            /* Positive, including the Gamma/digamma zero vicinity and s=40. */
            x = i % 16 == 1 ? 383156 : i % 16 == 5 ? 40*131072 : (slong) (1+next(62*131072));
            xe = -17; iy = (slong) next(17)-8; ie = -(slong) next(12);
            re = -(slong) (2+next(34)); im = re-(slong) next(8);
        }
        else
        {
            /* Wide rectangles, usually above the real pole line. */
            x = (slong) next(150*1024)-90*1024; xe = -10;
            re = (slong) next(18)-12; im = (slong) next(18)-14;
            iy = (slong) (1+next(64)); ie = im+(slong) next(4);
        }
        acb_zero(s);
        arf_set_si_2exp_si(arb_midref(acb_realref(s)), x, xe);
        arf_set_si_2exp_si(arb_midref(acb_imagref(s)), iy, ie);
        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, re);
        mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 1, im);
        if (mode == 0 && iy == 0) mag_zero(arb_radref(acb_imagref(s)));
        active = changed = 0;
        int st = adf_local_zeta_factor_at(y, NULL, s, adf_place_inf(), 256);
        if (st != ADF_OK)
        {
            nd += st == ADF_NOT_DETERMINED; limit += st == ADF_LIMIT;
            continue;
        }
        ok++; refined += active;
        tighter += changed;
        if (active && search_shift > 0 && search_cb > 2)
        {
            unresolved++;
            unresolved_tighter += changed;
        }
        for (int a = -1; a <= 1; a++)
            for (int b = -1; b <= 1; b++)
            {
                if ((a == 0) != (b == 0)) continue;
                acb_get_mid(pt, s);
                arf_set_mag(r, arb_radref(acb_realref(s)));
                arf_mul_si(r, r, a, ARF_PREC_EXACT, ARF_RND_DOWN);
                arf_add(arb_midref(acb_realref(pt)), arb_midref(acb_realref(pt)), r,
                        ARF_PREC_EXACT, ARF_RND_DOWN);
                arf_set_mag(r, arb_radref(acb_imagref(s)));
                arf_mul_si(r, r, b, ARF_PREC_EXACT, ARF_RND_DOWN);
                arf_add(arb_midref(acb_imagref(pt)), arb_midref(acb_imagref(pt)), r,
                        ARF_PREC_EXACT, ARF_RND_DOWN);
                sample(value, deriv, pt);
                values++;
                if (active)
                {
                    acb_get_mag_lower(d, deriv);
                    if (mag_cmp(d, search_B) > 0) bad_B++;
                    mag_div(d, d, search_B);
                    mag_max(largest, largest, d);
                }
                if (!acb_overlaps(y, value))
                {
                    misses++;
                    printf("MISS %lu mode %d: 0 256 %ld %ld %ld %ld 1 %ld %s %ld corner %d %d\n",
                           i, mode, x, xe, iy, ie, re, mode == 0 && iy == 0 ? "0" : "1", im, a, b);
                }
            }
    }
    printf("boxes=%lu OK=%lu ND=%lu LIMIT=%lu active=%lu changed=%lu samples=%lu misses=%lu bad_B=%lu ratio=",
           count, ok, nd, limit, refined, tighter, values, misses, bad_B);
    mag_print(largest); printf("\n");
    printf("unproved region n>0 and cb>2: active=%lu tighter=%lu\n", unresolved, unresolved_tighter);
    acb_clear(s); acb_clear(y); acb_clear(pt); acb_clear(value); acb_clear(deriv);
    acb_clear(search_candidate); mag_clear(search_B); arf_clear(r); mag_clear(d); mag_clear(largest);
    flint_cleanup();
    return misses || bad_B ? 1 : 0;
}
