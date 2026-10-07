/* Scratch differential scan: a mutated positive-radius certificate must
   contain the unmutated factor's certified exact-point enclosure. */
#include <adelefeld.h>
#include <stdio.h>
int mut_tate(acb_t, adf_place_t *, const acb_t, adf_place_t, const acb_t,
             const adf_char_t, slong);
int main(void)
{
    acb_t s, a, y, point, ref;
    adf_char_t chi;
    long boxes = 0, samples = 0, bad = 0;
    const double radius[] = {0.000244140625,0.125,0.5,1,2,4,8};
    const double imaginary[] = {0.015625,0.25,1,4};
    acb_init(s); acb_init(a); acb_init(y); acb_init(point); acb_init(ref);
    adf_char_init(chi); acb_one(a);
    for (int ix = -120; ix <= 12; ix += 3)
        for (int ir = 0; ir < 7; ir++)
            for (int iy = 0; iy < 4; iy++)
            {
                double x = ix/2.0+0.25, rx = radius[ir], im = imaginary[iy], ry = im/4;
                acb_set_d_d(s, x, im);
                mag_set_d(arb_radref(acb_realref(s)), rx);
                mag_set_d(arb_radref(acb_imagref(s)), ry);
                if (mut_tate(y, NULL, s, adf_place_inf(), a, chi, 53) != ADF_OK) continue;
                boxes++;
                for (int j = -1; j <= 1; j++) for (int k = -1; k <= 1; k++)
                {
                    acb_set_d_d(point, x+j*rx, im+k*ry);
                    if (adf_local_zeta_factor_at(ref, NULL, point, adf_place_inf(), 512) != ADF_OK)
                        continue;
                    samples++;
                    if (!acb_contains(y, ref))
                    {
                        bad++;
                        if (bad <= 8) printf("miss x=%g im=%g rx=%g ry=%g j=%d k=%d\n",
                                             x,im,rx,ry,j,k);
                    }
                }
            }
    printf("boxes=%ld samples=%ld missed=%ld\n",boxes,samples,bad);
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(point); acb_clear(ref); adf_char_clear(chi);
    flint_cleanup();
    return bad ? 1 : 0;
}
