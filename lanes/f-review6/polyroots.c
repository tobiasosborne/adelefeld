/* f-review6: cost of nmod_poly_find_distinct_nonzero_roots on T^d - 1 against a generator-power listing. */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <flint/nmod_poly.h>
#include <flint/ulong_extras.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
int main(int argc, char **argv)
{
    ulong p=strtoul(argv[1],0,10), d=strtoul(argv[2],0,10);
    nmod_poly_t f; mp_limb_t *r=flint_malloc(d*sizeof(mp_limb_t)); nmod_t mod; double t0,t1,t2;
    nmod_init(&mod,p);
    nmod_poly_init(f,p); nmod_poly_set_coeff_ui(f,0,p-1); nmod_poly_set_coeff_ui(f,d,1);
    t0=now(); int ok=nmod_poly_find_distinct_nonzero_roots(r,f); t1=now();
    /* listing: zeta = g^((p-1)/d) for a primitive root g (n_primitive_root_prime), then d products */
    ulong g=n_primitive_root_prime(p), z=n_powmod2_ui_preinv(g,(p-1)/d,p,mod.ninv), w=1;
    for (ulong i=0;i<d;i++){ r[i]=w; w=nmod_mul(w,z,mod); }
    t2=now();
    printf("p=%lu d=%lu ok=%d poly roots %.3f s; generator listing %.6f s (w back to 1: %d)\n",p,d,ok,t1-t0,t2-t1,(int)(w==1));
    return 0;
}
