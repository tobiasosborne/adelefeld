/* Valid noncardinal input at the numeric cap; probe the documented phase certificate failure. */
#include <adelefeld.h>
#include <stdio.h>
#include <time.h>
int main(void)
{
    adf_ffun_t x,y,old;
    acb_ptr f=_acb_vec_init(3);
    adf_ffun_init(x); adf_ffun_init(y); adf_ffun_init(old);
    acb_one(f+1); acb_set_si(y->f,77); adf_ffun_set(old,y);
    int set=adf_ffun_set_acb_vec(x,1,3,f,3);
    clock_t begin=clock();
    int st=adf_ffun_fourier(y,x,ADF_REAL_PREC_MAX);
    printf("set=%d status=%d unchanged=%d seconds=%.3f\n",set,st,
           adf_ffun_identical(y,old),(double)(clock()-begin)/CLOCKS_PER_SEC);
    adf_ffun_clear(x); adf_ffun_clear(y); adf_ffun_clear(old); _acb_vec_clear(f,3);
    flint_cleanup(); return 0;
}
