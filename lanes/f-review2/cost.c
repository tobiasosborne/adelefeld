#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double seconds(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + 1e-9*t.tv_nsec;
}

int main(int argc, char **argv)
{
    ulong exponent = argc > 1 ? strtoul(argv[1],NULL,10) : 33554433;
    adf_adele_t x;
    adf_sball_t y;
    fmpz_t a,h,d;
    adf_place_t places[4], where;
    ulong primes[4] = {3,2,5,UWORD(18446744073709551557)};
    adf_adele_init(x); adf_sball_init(y); fmpz_init_set_ui(a,1); fmpz_init_set_ui(h,6);
    fmpz_init_set_ui(d,1);
    double start=seconds(); fmpz_pow_ui(h,h,exponent);
    printf("H_bits=%lu construction_seconds=%.6f\n",fmpz_bits(h),seconds()-start); fflush(stdout);
    if (adf_fball_set_fmpz3(&x->fin,a,h,d) != ADF_OK) return 2;
    for (int i=0;i<4;i++)
    {
        adf_place_prime(places+i,primes[i]); where=adf_place_inf(); start=seconds();
        int st=adf_sball_project(y,&where,x,places+i,1);
        printf("p=%lu status=%s seconds=%.6f canonical=%d len=%ld",primes[i],adf_status_str(st),
               seconds()-start,adf_sball_is_canonical(y),y->len);
        if (st==ADF_OK) { printf(" u=");fmpq_print(y->loc[0].u);
            printf(" v=%ld N=%ld",y->loc[0].v,y->loc[0].N); }
        putchar('\n'); fflush(stdout);
    }
    adf_adele_clear(x); adf_sball_clear(y); fmpz_clear(a); fmpz_clear(h); fmpz_clear(d);
    flint_cleanup(); return 0;
}
