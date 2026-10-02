/* f-review6: N-D13 cost: a ball with large relative precision; the requested N cannot reduce the work. */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <adelefeld.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
int main(int argc, char **argv)
{
    slong R=atol(argv[1]); adf_lball_t x,y,l; flint_rand_t st; fmpz_t P; double t0,t1,t2,t3; int s1,s2,s3;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(l); flint_randinit(st); fmpz_init(P);
    /* x = w^2 mod 2^R with w a random odd R-bit number: a square ball 1 mod 8 of relative precision R */
    fmpz_one(P); fmpz_mul_2exp(P,P,R);
    fmpz_randbits(fmpq_numref(x->u),st,R); fmpz_abs(fmpq_numref(x->u),fmpq_numref(x->u)); fmpz_setbit(fmpq_numref(x->u),0);
    fmpz_powm_ui(fmpq_numref(x->u),fmpq_numref(x->u),2,P);
    x->p=2; x->v=0; x->N=R; x->exact=0;
    printf("canonical %d, R=%ld\n",adf_lball_is_canonical(x),(long)R);
    t0=now(); s1=adf_lball_root_seed(y,x,2,1,10); t1=now();
    s2=adf_lball_Log(l,x,10); t2=now();
    s3=adf_lball_Log(l,x,R); t3=now();
    printf("root_seed N=10: %s, N_out=%ld, %.3f s | Log N=10: %s %.4f s | Log N=R: %s %.3f s\n",
           adf_status_str(s1),(long)y->N,t1-t0,adf_status_str(s2),t2-t1,adf_status_str(s3),t3-t2);
    return 0;
}
