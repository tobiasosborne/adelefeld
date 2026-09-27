#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include <malloc.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/acb_dirichlet.h>
#include <flint/nmod.h>
#include <flint/padic.h>
#include <flint/ulong_extras.h>
typedef struct { fmpq_t centre, radius; } ball;
typedef struct { arb_t inf; ball fin; } adele;
int main(void) {
    flint_printf("FLINT %s sizes fmpz=%wu fmpq=%wu arb=%wu acb=%wu nmod=%wu padic=%wu tight_ball=%wu adele=%wu\n",FLINT_VERSION,sizeof(fmpz),sizeof(fmpq),sizeof(arb_struct),sizeof(acb_struct),sizeof(nmod_t),sizeof(padic_struct),sizeof(ball),sizeof(adele));
    fmpq_t q; fmpq_init(q); fmpq_set_si(q,1,3);
    arb_t x; arb_init(x); arb_set_fmpq(x,q,128);
    printf("arb(1/3) exact=%d contains_q=%d finite=%d\n",arb_is_exact(x),arb_contains_fmpq(x,q),arb_is_finite(x));
    char *dump=arb_dump_str(x); printf("arb(1/3) dump=%s\n",dump); flint_free(dump);
    acb_t s,zeta,xi,completed,pi; acb_init(s); acb_init(zeta); acb_init(xi); acb_init(completed); acb_init(pi);
    acb_set_ui(s,3); acb_const_pi(pi,128); acb_dirichlet_zeta(zeta,s,128); acb_dirichlet_xi(xi,s,128);
    acb_div(completed,zeta,pi,128); acb_mul_2exp_si(completed,completed,-1);
    printf("s=3 xi overlaps completed_zeta=%d ",acb_overlaps(xi,completed));
    acb_mul_ui(completed,completed,3,128);
    printf("xi overlaps 3*completed_zeta=%d\n",acb_overlaps(xi,completed));
    acb_clear(s); acb_clear(zeta); acb_clear(xi); acb_clear(completed); acb_clear(pi);
    flint_rand_t st; flint_randinit(st);
    ulong p=(UWORD(1)<<62)-57;
    ulong u=n_randint(st,p), b=n_randint(st,p)|1;
    ulong g=n_randbits(st,62)|1, h=n_randbits(st,62);
    flint_printf("baseline initial x=%wu b=%wu p=%wu b<p=%d gcd_inputs=%wu,%wu\n",u,b,p,b<p,g,h);
    fmpz_t A,B,M,D; fmpz_init(A); fmpz_init(B); fmpz_init(M); fmpz_init(D);
    fmpz_randbits(A,st,4096); fmpz_abs(A,A); fmpz_randbits(B,st,4096); fmpz_abs(B,B); fmpz_randbits(M,st,4096); fmpz_abs(M,M); fmpz_mul(D,A,B);
    flint_printf("baseline actual bit lengths A=%wu B=%wu M=%wu product=%wu\n",fmpz_bits(A),fmpz_bits(B),fmpz_bits(M),fmpz_bits(D));
    for(size_t n=1;n<=512;n=(n==1?24:n==24?25:n==25?48:512)) {
        void *p=malloc(n); printf("malloc request=%zu usable=%zu\n",n,malloc_usable_size(p)); free(p); if(n==512)break;
    }
    unsigned int aux; unsigned long long t0=__rdtscp(&aux), t1=__rdtscp(&aux);
    printf("RDTSCP executable=%d ticks_monotone=%d (not core-cycle calibration)\n",1,t1>=t0);
    fmpz_clear(A); fmpz_clear(B); fmpz_clear(M); fmpz_clear(D); flint_randclear(st); arb_clear(x); fmpq_clear(q); flint_cleanup();
    return 0;
}
