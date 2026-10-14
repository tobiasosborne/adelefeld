/* lanes/t5-repair1/probe_lf.c: a search probe (not a test) for the inherited Gamma certificate of
   src/localfactor.c (real_derivative_bound, real_factor). For a list of exact dyadic boxes s at the real place it
   prints one line per box and precision: index, status, the number of sample points of a 5 x 5 grid (corners and
   edges included) whose value is not overlapped by the result, the two radii, the midpoint bits. The sample values
   are FLINT's pi^(-t/2) Gamma(t/2) at 320 bits: a candidate finder only; a witness goes into the tests with an
   mpmath reference (lanes/t5-repair1/gen_lf_vectors.py). Linked once against the original archive and once per
   mutant object (lanes/t5-repair1/probe.py). */
#include <adelefeld.h>
#include <adelefeld/localfactor.h>
#include <stdio.h>
#include <math.h>

static long idx;

static void value(acb_t r, const acb_t t)
{
    acb_t z, pi;
    acb_init(z); acb_init(pi);
    acb_mul_2exp_si(z, t, -1);
    acb_gamma(r, z, 320);
    acb_const_pi(pi, 320);
    acb_neg(z, z);
    acb_pow(z, pi, z, 320);
    acb_mul(r, r, z, 320);
    acb_clear(z); acb_clear(pi);
}

/* s = (x + i y) +/- (rx, ry), each given as m 2^e */
static void box(double x, double y, double rx, double ry)
{
    static const slong precs[3] = { 24, 53, 120 };
    acb_t s, res, t, v;
    adf_place_t inf = adf_place_inf();
    acb_init(s); acb_init(res); acb_init(t); acb_init(v);
    arb_set_d(acb_realref(s), x); arb_set_d(acb_imagref(s), y);
    mag_set_d(arb_radref(acb_realref(s)), rx); mag_set_d(arb_radref(acb_imagref(s)), ry);
    for (int k = 0; k < 3; k++)
    {
        int st = adf_local_zeta_factor_at(res, NULL, s, inf, precs[k]);
        int miss = 0;
        if (st == ADF_OK)
        {
            for (int i = 0; i <= 4; i++)
                for (int j = 0; j <= 4; j++)
                {
                    arb_set_d(acb_realref(t), x + rx * (i - 2) / 2.0);
                    arb_set_d(acb_imagref(t), y + ry * (j - 2) / 2.0);
                    value(v, t);
                    if (!acb_overlaps(res, v)) miss++;
                }
            printf("%ld %d %d %.6e %.6e %ld %ld | %g %g %g %g p%ld\n", idx, st, miss,
                   mag_get_d(arb_radref(acb_realref(res))), mag_get_d(arb_radref(acb_imagref(res))),
                   (long) arf_bits(arb_midref(acb_realref(res))), (long) arf_bits(arb_midref(acb_imagref(res))),
                   x, y, rx, ry, (long) precs[k]);
        }
        else
            printf("%ld %d | %g %g %g %g p%ld\n", idx, st, x, y, rx, ry, (long) precs[k]);
        idx++;
    }
    acb_clear(s); acb_clear(res); acb_clear(t); acb_clear(v);
}

int main(void)
{
    static const double xs[] = { -11.5, -9, -7.5, -5, -3.25, -2, -1, -0.5, 0.5, 1, 2, 3, 4.5, 6, 9, 17, 30 };
    static const double ys[] = { 0, 0.125, 0.5, 1, 3, 6, 10, 20 };
    static const double rs[] = { 0.5, 0.125, 1.0 / 64, 1.0 / 1024, 1.0 / 1048576 };
    static const double big[] = { 1, 2, 4 };
    /* exact real part (zero real radius): A = lo + n = 1 = hi + n when x = 2 - 2m */
    for (int m = 0; m < 6; m++)
        for (unsigned a = 0; a < sizeof ys / sizeof ys[0]; a++)
            for (unsigned b = 0; b < sizeof rs / sizeof rs[0]; b++)
                box(2.0 - 2 * m, ys[a], 0, rs[b]);
    /* square complex boxes */
    for (unsigned c = 0; c < sizeof xs / sizeof xs[0]; c++)
        for (unsigned a = 0; a < sizeof ys / sizeof ys[0]; a++)
            for (unsigned b = 0; b < sizeof rs / sizeof rs[0]; b++)
                box(xs[c], ys[a], rs[b], rs[b]);
    /* real segments */
    for (unsigned c = 0; c < sizeof xs / sizeof xs[0]; c++)
        for (unsigned b = 0; b < sizeof rs / sizeof rs[0]; b++)
            box(xs[c], 0, rs[b], 0);
    /* large boxes (R > 1) high above the real axis */
    for (unsigned c = 0; c < sizeof big / sizeof big[0]; c++)
        for (int xx = 2; xx <= 40; xx += 6)
            for (int yy = 6; yy <= 30; yy += 8)
                box(xx, yy, big[c], big[c]);
    /* random boxes (a fixed seed): x on a 1/16 grid in [-12, 30], y on a 1/16 grid in [0, 12], radii 2^-k or 0 */
    {
        flint_rand_t st;
        flint_randinit(st);
        for (int i = 0; i < 2500; i++)
        {
            double x = (double) ((slong) n_randint(st, 42 * 16) - 12 * 16) / 16;
            double y = (double) n_randint(st, 12 * 16) / 16;
            double rx = n_randint(st, 4) == 0 ? 0 : ldexp(1, -(int) n_randint(st, 13));
            double ry = ldexp(1, -(int) n_randint(st, 13));
            box(x, y, rx, ry);
        }
        flint_randclear(st);
    }
    flint_cleanup();
    return 0;
}
