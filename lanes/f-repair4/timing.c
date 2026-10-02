/* f-repair4: wall time of adf_lball_roots on the sets of review f-review6, F2 (exact w, all branches). */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <adelefeld.h>
#include <flint/ulong_extras.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
int main(int argc, char **argv)
{
    ulong p=strtoul(argv[1],0,10), n=strtoul(argv[2],0,10), w=argc>4 ? strtoul(argv[4],0,10) : 1;
    slong N=atol(argv[3]), len=-1, cap=argc>5 ? atol(argv[5]) : -2;
    adf_lball_t x; adf_lball_ptr y; ulong *ids, d=n_gcd(n,p-1); double t0,t1; int st;
    if (cap==-2) cap=(slong)d;
    adf_lball_init(x); x->p=p; fmpq_set_ui(x->u,w,1); x->v=0; x->N=0; x->exact=1;
    y=flint_malloc((d+1)*sizeof(adf_lball_struct)); ids=flint_malloc((d+1)*sizeof(ulong));
    for (ulong i=0;i<=d;i++) adf_lball_init(y+i);
    t0=now(); st=adf_lball_roots(y,ids,&len,cap,x,n,N); t1=now();
    printf("p=%lu n=%lu w=%lu N=%ld cap=%ld: %s len=%ld ids[0]=%lu ids[last]=%lu %.4f s\n",p,n,w,(long)N,(long)cap,
           adf_status_str(st),(long)len,st ? 0 : ids[0],st ? 0 : ids[len-1],t1-t0);
    for (ulong i=0;i<=d;i++) adf_lball_clear(y+i);
    flint_free(y); flint_free(ids); adf_lball_clear(x);
    return 0;
}
