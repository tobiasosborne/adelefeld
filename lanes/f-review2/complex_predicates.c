#include <adelefeld.h>
#include <stdio.h>

int main(void)
{
    adf_sball_t x,y;
    adf_sball_init(x);adf_sball_init(y);x->arch=y->arch=ADF_ARCH_COMPLEX;
    unsigned long cases=0,mismatches=0;
    for(int a=-2;a<=2;a+=2)for(int ar=0;ar<=2;ar++)
    for(int b=-2;b<=2;b+=2)for(int br=0;br<=2;br++)
    for(int c=-2;c<=2;c+=2)for(int cr=0;cr<=2;cr++)
    for(int d=-2;d<=2;d+=2)for(int dr=0;dr<=2;dr++)
    {
        arb_set_si(acb_realref(x->inf),a);mag_set_ui(arb_radref(acb_realref(x->inf)),ar);
        arb_set_si(acb_imagref(x->inf),b);mag_set_ui(arb_radref(acb_imagref(x->inf)),br);
        arb_set_si(acb_realref(y->inf),c);mag_set_ui(arb_radref(acb_realref(y->inf)),cr);
        arb_set_si(acb_imagref(y->inf),d);mag_set_ui(arb_radref(acb_imagref(y->inf)),dr);
        int eq=a==c&&ar==cr&&b==d&&br==dr;
        int over=a-ar<=c+cr&&c-cr<=a+ar&&b-br<=d+dr&&d-dr<=b+br;
        int inside=c-cr<=a-ar&&a+ar<=c+cr&&d-dr<=b-br&&b+br<=d+dr;
        mismatches+=adf_sball_equal_set(x,y)!=eq;
        mismatches+=adf_sball_overlaps(x,y)!=over;
        mismatches+=adf_sball_contains(x,y)!=inside;
        cases+=3;
    }
    printf("predicate_calls=%lu mismatches=%lu\n",cases,mismatches);
    adf_sball_clear(x);adf_sball_clear(y);flint_cleanup();return mismatches?1:0;
}
