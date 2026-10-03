#define _GNU_SOURCE
#include <adelefeld.h>
#include <flint/flint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <string.h>

static double now(void)
{ struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW,&t); return t.tv_sec+1e-9*t.tv_nsec; }
static int cmp(const void *a,const void *b)
{ double x=*(const double *)a,y=*(const double *)b; return (x>y)-(x<y); }
static void base(adf_lball_t x,ulong p,slong N,int ball,int multiplier)
{
    x->p=p; x->v=0; x->N=ball ? N+3 : 0; x->exact=!ball;
    fmpz_set_ui(fmpq_numref(x->u),p);
    fmpz_mul_ui(fmpq_numref(x->u),fmpq_numref(x->u),multiplier);
    fmpz_add_ui(fmpq_numref(x->u),fmpq_numref(x->u),1);
    fmpz_one(fmpq_denref(x->u));
}
int main(int argc,char **argv)
{
    if (argc!=5 && argc!=6) return 2;
    ulong p=strtoul(argv[2],NULL,10); slong N=strtol(argv[3],NULL,10);
    int trials=argc==6 ? 1 : 3;
    int ball=atoi(argv[4]), unit=!strcmp(argv[1],"powunit");
    adf_lball_t x,s,xc,sc,y,z,a,b;
    adf_lball_init(x); adf_lball_init(s); adf_lball_init(xc); adf_lball_init(sc);
    adf_lball_init(y); adf_lball_init(z); adf_lball_init(a); adf_lball_init(b);
    flint_set_num_threads(1);
    cpu_set_t allowed,pin; CPU_ZERO(&allowed); CPU_ZERO(&pin);
    sched_getaffinity(0,sizeof(allowed),&allowed);
    int cpu=-1;
    for (int k=0;k<CPU_SETSIZE;k++) if (CPU_ISSET(k,&allowed)) { cpu=k; break; }
    if (cpu>=0) { CPU_SET(cpu,&pin); sched_setaffinity(0,sizeof(pin),&pin); }
    base(x,p,N,ball,unit ? 1 : 2);
    s->p=p; s->v=0; s->N=ball ? N+2 : 0; s->exact=!ball; fmpq_set_ui(s->u,2,1);
    adf_lball_set(xc,x); xc->exact=1; xc->N=0;
    adf_lball_set(sc,s); sc->exact=1; sc->N=0;
    printf("fn=%s p=%lu N=%ld ball=%d compiler=%s flags=-std=gnu11,-O2,-g FLINT=%s "
           "GMP=%s cpu=%d threads=1 kind=call_rate warm_output=1 internal_allocating=1 "
           "seed=0 clock=CLOCK_MONOTONIC_RAW input_bits=%ld,%ld trials=%d\n",
           argv[1],p,N,ball,__VERSION__,FLINT_VERSION,gmp_version,cpu,
           fmpz_bits(fmpq_numref(x->u)),fmpz_bits(fmpq_numref(s->u)),trials);
    double total[3],component[3],first[3],second[3],third[3];
    unsigned long checksum=0; int bad=0;
    // Only timing trials, no untimed warm-up calls. Reused outputs; first trial includes cold allocation.
    for (int i=0;i<trials;i++)
    {
        double t0=now();
        int st=unit ? adf_lball_powunit(y,x,s,N) : adf_lball_powrat(y,x,-5,2,1,N);
        total[i]=now()-t0;
        printf("completed_total trial=%d seconds=%.9f status=%d\n",i,total[i],st); fflush(stdout);
        double c0=now(),t1,t2,t3;
        int cs;
        if (!unit)
        {
            cs=adf_lball_root_seed(a,x,2,1,N); t1=now();
            if (cs==ADF_OK) cs=adf_lball_pow_si(z,a,-5);
            t2=now(); t3=t2;
        }
        else
        {
            cs=adf_lball_Log(a,xc,N); t1=now();
            if (cs==ADF_OK) cs=adf_lball_mul(b,a,sc);
            t2=now();
            if (cs==ADF_OK) cs=adf_lball_exp(z,b,N);
            t3=now();
        }
        first[i]=t1-c0; second[i]=t2-t1; third[i]=t3-t2; component[i]=t3-c0;
        // Full comparison and consumption outside both timed regions.
        int same=st==ADF_OK && cs==ADF_OK && adf_lball_equal_set(y,z);
        bad+=!same;
        checksum+=fmpz_fdiv_ui(fmpq_numref(y->u),1000003);
        printf("trial=%d total=%.9f components=%.9f first=%.9f second=%.9f third=%.9f "
               "status=%d component_status=%d equal=%d output_bits=%ld\n",i,total[i],component[i],
               first[i],second[i],third[i],st,cs,same,fmpz_bits(fmpq_numref(y->u)));
        fflush(stdout);
    }
    if (trials==1) { total[1]=total[2]=total[0]; component[1]=component[2]=component[0]; }
    qsort(total,trials,sizeof(double),cmp); qsort(component,trials,sizeof(double),cmp);
    printf("ROW %s %lu %ld %s %.9f %.9f %.9f %.9f %.9f %.9f %.6f %d %lu\n",
           argv[1],p,N,ball ? "ball" : "exact",total[0],total[1],total[2],
           component[0],component[1],component[2],total[1]/component[1],bad,checksum);
    adf_lball_clear(x); adf_lball_clear(s); adf_lball_clear(xc); adf_lball_clear(sc);
    adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(a); adf_lball_clear(b);
    return bad ? 1 : 0;
}
