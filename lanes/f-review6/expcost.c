/* f-review6: where the time of a large-relative-precision root goes: Log, division, exp at p=2 and p=3. */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <adelefeld.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
int main(int argc, char **argv)
{
    ulong p=strtoul(argv[1],0,10); slong R=atol(argv[2]);
    adf_lball_t u,l,d,z; flint_rand_t st; fmpz_t P; double t0,t1,t2,t3; int s;
    adf_lball_init(u); adf_lball_init(l); adf_lball_init(d); adf_lball_init(z); flint_randinit(st); fmpz_init(P);
    fmpz_set_ui(P,p); fmpz_pow_ui(P,P,(ulong)R);
    fmpz_randm(fmpq_numref(u->u),st,P); fmpz_mul_ui(fmpq_numref(u->u),fmpq_numref(u->u),p*p);
    fmpz_add_ui(fmpq_numref(u->u),fmpq_numref(u->u),1);   /* exact unit 1 mod p^2 */
    u->p=p; u->exact=1;
    d->p=p; fmpq_set_si(d->u,1,1); d->v=1; d->exact=1;   /* divide by p (n = p) */
    t0=now(); s=adf_lball_Log(l,u,R+1); t1=now();
    s|=adf_lball_div(l,l,d); t2=now();
    s|=adf_lball_exp(z,l,R); t3=now();
    printf("p=%lu R=%ld status %d: Log %.3f s, div %.3f s, exp %.3f s\n",p,(long)R,s,t1-t0,t2-t1,t3-t2);
    return 0;
}
