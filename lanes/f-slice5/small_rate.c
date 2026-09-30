#define _GNU_SOURCE
#include <adelefeld.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int baseline_exp(adf_lball_t, const adf_lball_t, slong);
int baseline_log(adf_lball_t, const adf_lball_t, slong);
int baseline_Log(adf_lball_t, const adf_lball_t, slong);
typedef int (*fn_t)(adf_lball_t,const adf_lball_t,slong);
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW,&t);
    return t.tv_sec+1e-9*t.tv_nsec; }
static int cmp(const void *a,const void *b) { double x=*(const double *)a,y=*(const double *)b;
    return (x>y)-(x<y); }
int main(void)
{
    const ulong primes[4]={2,3,5,UWORD(18446744073709551557)};
    const int digits[3]={8,16,32};
    fn_t funcs[2][3]={{baseline_exp,baseline_log,baseline_Log},
                     {adf_lball_exp,adf_lball_log,adf_lball_Log}};
    const char *names[3]={"exp","log","Log"};
    int pi,ni,fi;
    adf_lball_t x,y[2];
    cpu_set_t cpus;
    CPU_ZERO(&cpus); CPU_SET(2,&cpus); if(sched_setaffinity(0,sizeof cpus,&cpus)) return 2;
    flint_set_num_threads(1); adf_lball_init(x); adf_lball_init(y[0]); adf_lball_init(y[1]);
    puts("cpu=2 threads=1 clock=MONOTONIC_RAW seed=0 warm_outputs=1 kind=call_rate trials=5");
    for(pi=0;pi<4;pi++) for(ni=0;ni<3;ni++) for(fi=0;fi<3;fi++)
    {
        int trial,stage,k,reps=pi==3?1000:10000;
        ulong p=primes[pi]; slong N=digits[ni],c=p==2?2:1;
        double t[2][5],start;
        x->p=p; x->v=fi==0?c:0; fmpq_one(x->u);
        if(fi==1) { fmpz_ui_pow_ui(fmpq_numref(x->u),p,(ulong)c);
            fmpz_add_ui(fmpq_numref(x->u),fmpq_numref(x->u),1); }
        if(fi==2) fmpq_set_si(x->u,p==2?5:2,1);
        for(k=0;k<1000;k++) { funcs[0][fi](y[0],x,N); funcs[1][fi](y[1],x,N); }
        for(trial=0;trial<5;trial++) for(stage=0;stage<2;stage++)
        {
            int s=(stage+trial)%2,st=0;
            start=now(); for(k=0;k<reps;k++) st|=funcs[s][fi](y[s],x,N);
            t[s][trial]=(now()-start)/reps; if(st) return 3;
        }
        if(y[0]->p!=y[1]->p || y[0]->v!=y[1]->v || y[0]->N!=y[1]->N
           || y[0]->exact!=y[1]->exact || !fmpq_equal(y[0]->u,y[1]->u)) return 4;
        qsort(t[0],5,sizeof(double),cmp); qsort(t[1],5,sizeof(double),cmp);
        printf("%s %lu %ld old_ns=%.3f,%.3f,%.3f new_ns=%.3f,%.3f,%.3f ratio=%.3f reps=%d\n",
               names[fi],p,N,t[0][0]*1e9,t[0][2]*1e9,t[0][4]*1e9,
               t[1][0]*1e9,t[1][2]*1e9,t[1][4]*1e9,t[1][2]/t[0][2],reps); fflush(stdout);
    }
    adf_lball_clear(x); adf_lball_clear(y[0]); adf_lball_clear(y[1]); flint_cleanup(); return 0;
}
