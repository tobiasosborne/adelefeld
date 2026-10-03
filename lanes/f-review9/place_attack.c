#include <adelefeld.h>
#include <stdio.h>
typedef int (*fn)(adf_adele_t, adf_place_t *, const adf_adele_t, slong);
static fn fs[] = {adf_adele_exp,adf_adele_sin,adf_adele_sinh,adf_adele_cos,adf_adele_cosh};
int main(void) {
    unsigned char composite[10001]={0}; ulong primes[1300],len=0;
    for(ulong d=2;d*d<=10000;d++) if(!composite[d])
        for(ulong k=d*d;k<=10000;k+=d) composite[k]=1;
    for(ulong p=2;p<=10000;p++) if(!composite[p]) primes[len++]=p;
    adf_adele_t x,y,before; adf_rat_t q; adf_place_t sentinel;
    adf_adele_init(x); adf_adele_init(y); adf_adele_init(before); adf_rat_init(q);
    if(adf_place_prime(&sentinel,97)!=ADF_OK) return 2;
    unsigned long cases=0,calls=0,bad=0,max_den_bits=0;
    for(ulong j=0;j<500;j++) {
        fmpq_set_ui(q->q,4,1);
        for(ulong k=1;k<=j%200;k++) fmpz_mul_ui(fmpq_numref(q->q),fmpq_numref(q->q),primes[k]);
        if(j%3==0) fmpz_mul_2exp(fmpq_numref(q->q),fmpq_numref(q->q),j*20);
        if(j%3==1) fmpz_add_ui(fmpq_numref(q->q),fmpq_numref(q->q),1);
        fmpz_one(fmpq_denref(q->q));
        fmpz_mul_2exp(fmpq_denref(q->q),fmpq_denref(q->q),j*30);
        fmpz_add_ui(fmpq_denref(q->q),fmpq_denref(q->q),1);
        if(j%2) fmpz_neg(fmpq_numref(q->q),fmpq_numref(q->q));
        fmpq_canonicalise(q->q);
        if(fmpz_bits(fmpq_denref(q->q))>max_den_bits) max_den_bits=fmpz_bits(fmpq_denref(q->q));
        ulong want=0;
        for(ulong k=0;k<len;k++) {
            ulong p=primes[k];
            if(fmpz_fdiv_ui(fmpq_numref(q->q),p==2?4:p)!=0) { want=p;break; }
        }
        if(!want) return 3;
        arb_zero(x->inf); adf_fball_set_rat(&x->fin,q); cases++;
        for(int fun=0;fun<5;fun++) for(int mode=0;mode<3;mode++) {
            adf_place_t where=sentinel;
            if(mode==1) adf_adele_set(y,x); else { arb_set_si(y->inf,123);adf_fball_set_si(&y->fin,123); }
            adf_adele_set(before,y);
            int st=fs[fun](y,mode==2?NULL:&where,mode==1?y:x,64);calls++;
            if(st!=ADF_DOMAIN || (mode!=2 && adf_place_prime_get(where)!=want) ||
                !adf_adele_identical(y,before)) bad++;
        }
    }
    printf("rational_cases=%lu calls=%lu failures=%lu sieve_primes=%lu max_denominator_bits=%lu\n",
           cases,calls,bad,len,max_den_bits);
    adf_adele_clear(x);adf_adele_clear(y);adf_adele_clear(before);adf_rat_clear(q);flint_cleanup();
    return bad?1:0;
}
