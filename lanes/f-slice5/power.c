#define _GNU_SOURCE
#include <adelefeld.h>
#include <sched.h>
#include <stdio.h>
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW,&t);
    return t.tv_sec+1e-9*t.tv_nsec; }
int main(void)
{
    ulong p = UWORD(18446744073709551557);
    fmpz_t P,a,z;
    adf_lball_t w;
    adf_place_t place;
    double start;
    cpu_set_t cpus;
    CPU_ZERO(&cpus); CPU_SET(2,&cpus); sched_setaffinity(0,sizeof cpus,&cpus);
    flint_set_num_threads(1);
    fmpz_init(P); fmpz_init_set_ui(a,2); fmpz_init(z); adf_lball_init(w);
    adf_place_prime(&place,p); fmpz_ui_pow_ui(P,p,10000);
    start=now(); fmpz_powm_ui(z,a,p-1,P);
    printf("power_seconds=%.9f checksum=%lu\n",now()-start,fmpz_fdiv_ui(z,65521)); fflush(stdout);
    start=now(); int st=adf_lball_teichmuller(w,place,2,10000);
    printf("teich_seconds=%.9f status=%d checksum=%lu\n",now()-start,st,
           fmpz_fdiv_ui(fmpq_numref(w->u),65521));
    fmpz_clear(P); fmpz_clear(a); fmpz_clear(z); adf_lball_clear(w); flint_cleanup();
    return 0;
}
