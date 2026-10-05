/* The precision cap named by qclass.h must be available from that header alone. */
#include <adelefeld/qclass.h>
#include <stdio.h>
_Static_assert(ADF_REAL_PREC_MAX == 2097152, "qclass precision cap");
int main(void)
{
    adf_qclass_t x, y; int s;
    adf_qclass_init(x); adf_qclass_init(y);
    s = adf_qclass_reduce(y, x, 1, ADF_REAL_PREC_MAX);
    printf("header-only include: status=%d, precision cap=%d\n", s, ADF_REAL_PREC_MAX);
    adf_qclass_clear(x); adf_qclass_clear(y); flint_cleanup(); return s;
}
