#define _GNU_SOURCE
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <string.h>
static double now(void)
{ struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW,&t); return t.tv_sec+1e-9*t.tv_nsec; }
int main(int argc,char **argv)
{
    if(argc!=7) return 2;
    const char *fn=argv[1], *phase=argv[5];
    ulong p=strtoul(argv[2],NULL,10); slong N=strtol(argv[3],NULL,10);
    int ball=atoi(argv[4]),unit=!strcmp(fn,"powunit"),slot=atoi(argv[6]);
    cpu_set_t allowed,pin; CPU_ZERO(&allowed); CPU_ZERO(&pin); sched_getaffinity(0,sizeof(allowed),&allowed);
    int cpu=-1;
    for(int k=0;k<CPU_SETSIZE;k++) if(CPU_ISSET(k,&allowed)) { if(slot--==0) { cpu=k; break; } }
    if(cpu>=0) { CPU_SET(cpu,&pin); sched_setaffinity(0,sizeof(pin),&pin); }
    flint_set_num_threads(1);
    adf_lball_t x,s,xc,sc,y,a,b;
    adf_lball_init(x); adf_lball_init(s); adf_lball_init(xc); adf_lball_init(sc);
    adf_lball_init(y); adf_lball_init(a); adf_lball_init(b);
    x->p=p; x->v=0; x->N=ball ? N+3 : 0; x->exact=!ball;
    fmpz_set_ui(fmpq_numref(x->u),p); if(!unit) fmpz_mul_ui(fmpq_numref(x->u),fmpq_numref(x->u),2);
    fmpz_add_ui(fmpq_numref(x->u),fmpq_numref(x->u),1); fmpz_one(fmpq_denref(x->u));
    s->p=p; s->v=0; s->N=ball ? N+2 : 0; s->exact=!ball; fmpq_set_ui(s->u,2,1);
    adf_lball_set(xc,x); xc->exact=1; xc->N=0;
    adf_lball_set(sc,s); sc->exact=1; sc->N=0;
    printf("phase=%s fn=%s p=%lu N=%ld ball=%d cpu=%d threads=1 trials=1 compiler=%s "
           "flags=-std=gnu11,-O2,-g FLINT=%s GMP=%s kind=call_rate output_reused=1 "
           "internal_allocating=1 seed=0 clock=CLOCK_MONOTONIC_RAW input_bits=%ld,%ld\n",
           phase,fn,p,N,ball,cpu,__VERSION__,FLINT_VERSION,gmp_version,
           fmpz_bits(fmpq_numref(x->u)),fmpz_bits(fmpq_numref(s->u)));
    fflush(stdout);
    int st; double start=now(),t1=start,t2=start,t3=start;
    if(!strcmp(phase,"total"))
        st=unit ? adf_lball_powunit(y,x,s,N) : adf_lball_powrat(y,x,-5,2,1,N);
    else if(unit)
    {
        st=adf_lball_Log(a,xc,N); t1=now();
        if(st==ADF_OK) st=adf_lball_mul(b,a,sc); t2=now();
        if(st==ADF_OK) st=adf_lball_exp(y,b,N); t3=now();
    }
    else
    {
        st=adf_lball_root_seed(a,x,2,1,N); t1=now();
        if(st==ADF_OK) st=adf_lball_pow_si(y,a,-5); t2=now(); t3=t2;
    }
    double elapsed=now()-start;
    printf("completed phase=%s seconds=%.9f status=%d first=%.9f second=%.9f third=%.9f\n",
           phase,elapsed,st,t1-start,t2-t1,t3-t2); fflush(stdout);
    // Independent full-residue certificate after timing. n=2 is a unit at every measured prime.
    fmpz_t mod,check,tmp; fmpz_init(mod); fmpz_init(check); fmpz_init(tmp);
    fmpz_ui_pow_ui(mod,p,N);
    int valid=st==ADF_OK && !y->exact && y->v==0 && y->N==N && adf_lball_is_canonical(y);
    if(st==ADF_OK)
    {
        if(unit)
        {
            fmpz_powm_ui(check,fmpq_numref(x->u),2,mod);
            valid &= fmpz_equal(check,fmpq_numref(y->u));
        }
        else
        {
            fmpz_powm_ui(check,fmpq_numref(y->u),2,mod);
            fmpz_powm_ui(tmp,fmpq_numref(x->u),5,mod);
            fmpz_mul(check,check,tmp); fmpz_mod(check,check,mod);
            valid &= fmpz_is_one(check) && fmpz_fdiv_ui(fmpq_numref(y->u),p)==1;
        }
    }
    printf("PHASE %s %lu %ld %s %s %.9f valid=%d output_bits=%ld checksum=%lu\n",
           fn,p,N,ball ? "ball" : "exact",phase,elapsed,valid,
           fmpz_bits(fmpq_numref(y->u)),fmpz_fdiv_ui(fmpq_numref(y->u),1000003));
    fmpz_clear(mod); fmpz_clear(check); fmpz_clear(tmp);
    adf_lball_clear(x); adf_lball_clear(s); adf_lball_clear(xc); adf_lball_clear(sc);
    adf_lball_clear(y); adf_lball_clear(a); adf_lball_clear(b);
    return !valid;
}
