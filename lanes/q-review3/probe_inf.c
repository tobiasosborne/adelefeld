#include <adelefeld.h>
#include <stdio.h>
int main(void)
{
    adf_qclass_t x, y; int st;
    adf_qclass_init(x); adf_qclass_init(y);
    mag_inf(arb_radref(x->piece->inf));
    printf("inf radius: canonical %d\n", adf_qclass_is_canonical(x));
    if (adf_qclass_is_canonical(x)) { st = adf_qclass_reduce(y, x, 100, 53); printf(" reduce %d\n", st); }
    mag_zero(arb_radref(x->piece->inf)); arf_pos_inf(arb_midref(x->piece->inf));
    printf("inf mid: canonical %d\n", adf_qclass_is_canonical(x));
    if (adf_qclass_is_canonical(x)) { st = adf_qclass_reduce(y, x, 100, 53); printf(" reduce %d\n", st); }
    arf_nan(arb_midref(x->piece->inf));
    printf("nan mid: canonical %d\n", adf_qclass_is_canonical(x));
    if (adf_qclass_is_canonical(x)) { st = adf_qclass_reduce(y, x, 100, 53); printf(" reduce %d\n", st); }
    adf_qclass_clear(x); adf_qclass_clear(y); return 0;
}
