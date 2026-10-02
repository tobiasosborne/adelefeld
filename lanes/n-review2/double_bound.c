/* Numerical attack only. arb's outward ball arithmetic: refs/src/flint-3.0.1/arb.rst:3-13.
   This does not prove an error bound for the system libm over every 64-bit integer. */
#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

static ulong state=3791027;
static ulong next(void) {state^=state<<13; state^=state>>7; state^=state<<17; return state;}

int main(void)
{
    const ulong sizes[]={1,64,67108864,268435456};
    ulong ps[1500]; int np=0, checks=0,fail=0;
    arb_t p,l,l2,q,b,err;
    mag_t upper;
    double worst=0;
    arb_init(p); arb_init(l); arb_init(l2); arb_init(q); arb_init(b); arb_init(err); mag_init(upper);
    arb_set_ui(l2,2); arb_log(l2,l2,256);
    ps[np++]=2; ps[np++]=3; ps[np++]=5; ps[np++]=7; ps[np++]=~(ulong)0;
    for(int i=2;i<64;i++) {
        ps[np++]=((ulong)1<<i)-1; ps[np++]=(ulong)1<<i; ps[np++]=((ulong)1<<i)+1;
    }
    for(int i=0;i<1000;i++) {ulong v=next()>>1; ps[np++]=n_nextprime(v<2 ? 2:v,1);}
    for(int i=0;i<np;i++) {
        ulong prime=ps[i];
        double ld=log2((double)prime);
        arb_set_ui(p,prime); arb_log(l,p,256); arb_div(l,l,l2,256);
        arb_set_d(err,ld); arb_sub(err,err,l,256); arb_abs(err,err);
        arb_get_mag(upper,err); double e=mag_get_d(upper); if(e>worst) worst=e;
        for(int j=0;j<4;j++) {
            double t=(double)sizes[j]/ld; ulong bound=(ulong)t+1;
            arb_set_ui(q,sizes[j]); arb_div(q,q,l,256); arb_set_ui(b,bound);
            checks++;
            if(!arb_le(q,b)) {
                fail++; printf("FAIL p=%lu bits=%lu bound=%lu\n",prime,sizes[j],bound);
            }
        }
    }
    printf("DBL_MANT_DIG=%d radix=%d inputs=%d bound_checks=%d failures=%d max_log2_abs_error_upper=%.17g\n",
           DBL_MANT_DIG,FLT_RADIX,np,checks,fail,worst);
    arb_clear(p); arb_clear(l); arb_clear(l2); arb_clear(q); arb_clear(b); arb_clear(err); mag_clear(upper);
    flint_cleanup(); return fail!=0;
}
