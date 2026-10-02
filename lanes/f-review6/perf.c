#define _POSIX_C_SOURCE 199309L
/* f-review6: cost of all-branch evaluation at p=65537, n=16, N=200 (exact input 3) and a ball input. */
#include <stdio.h>
#include <time.h>
#include <adelefeld.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
int main(int argc, char **argv)
{
    ulong p=65537, n=16; slong N=200; int reps=20;
    adf_lball_t x,y,l,e; adf_lball_struct ys[16]; ulong ids[16]; slong len; double t0,t1,t2,t3,t4;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(l); adf_lball_init(e);
    for (int i=0;i<16;i++) adf_lball_init(ys+i);
    x->p=p; fmpq_set_si(x->u,3,1); x->v=0; x->N=0; x->exact=1;   /* 3 = g is a generator? use 3^16 */
    fmpz_pow_ui(fmpq_numref(x->u), fmpq_numref(x->u), 16);       /* x = 3^16 = 43046721, a 16th power */
    if (argc>1) { fmpz_add_ui(fmpq_numref(x->u),fmpq_numref(x->u),65537*7); } /* irrational branches */
    t0=now(); for (int r=0;r<reps;r++) adf_lball_roots(ys,ids,&len,16,x,n,N); t1=now();
    for (int r=0;r<reps;r++) adf_lball_root_seed(y,x,n,ids[1],N); t2=now();
    for (int r=0;r<reps;r++) { adf_lball_t u; adf_lball_init(u); fmpq_set(u->u,x->u); u->p=p;
        adf_lball_Log(l,u,N); adf_lball_clear(u);} t3=now();
    adf_lball_t d; adf_lball_init(d); d->p=p; fmpq_set_si(d->u,16,1);
    adf_lball_div(e,l,d);
    for (int r=0;r<reps;r++) adf_lball_exp(y,e,N); t4=now();
    printf("len=%ld ids0=%lu exact0=%d\n",(long)len,ids[0],ys[0].exact);
    printf("roots (16 branches): %.3f ms; one seeded branch: %.3f ms; Log at N: %.3f ms; exp at N: %.3f ms\n",
        1e3*(t1-t0)/reps, 1e3*(t2-t1)/reps, 1e3*(t3-t2)/reps, 1e3*(t4-t3)/reps);
    return 0;
}
