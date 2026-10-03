#include <stdio.h>
#include <flint/ulong_extras.h>
static unsigned long mul_calls, pow_calls, digit_mul, digit_pow, binary_digit_products;
static int review_in_digit;
static ulong counted_mul(ulong a, ulong b, ulong n, ulong inv)
{ mul_calls++; digit_mul+=review_in_digit; return n_mulmod2_preinv(a,b,n,inv); }
static ulong counted_pow(ulong a, ulong e, ulong n, ulong inv)
{ pow_calls++; digit_pow+=review_in_digit;
  if(review_in_digit) { ulong t=e; while(t) { if(t&1) binary_digit_products++; t>>=1;
      if(t) binary_digit_products++; } }
  return n_powmod2_ui_preinv(a,e,n,inv); }
#define n_mulmod2_preinv counted_mul
#define n_powmod2_ui_preinv counted_pow
#include "build/r8-counted-source.c"
#undef n_mulmod2_preinv
#undef n_powmod2_ui_preinv
int main(void)
{
    unsigned long cases=0, bad=0, no_solution=0;
    for (ulong p=3;p<150;p=n_nextprime(p,1))
    {
        ulong g=n_primitive_root_prime(p);
        for (ulong d=2;d<p;d++) if ((p-1)%d==0)
            for (ulong A=1;A<p;A++)
            {
                int exists=0;
                for (ulong z=1;z<p;z++) exists|=n_powmod2(z,d,p)==A;
                ulong t=dth_root(A,d,p,g);
                cases++; no_solution+=!exists;
                bad+=(!!t!=exists || (t && n_powmod2(t,d,p)!=A));
            }
    }
    mul_calls=pow_calls=digit_mul=digit_pow=binary_digit_products=0; review_in_digit=0;
    ulong g=n_primitive_root_prime(17), t=dth_root(4,2,17,g);
    printf("R8 cases=%lu nonsolvable=%lu failures=%lu precision=exact_Fp\n",cases,no_solution,bad);
    printf("p=17 d=2 A=4 t=%lu counted_word_products=%lu powering_calls=%lu claimed_bound=6\n",
           t,mul_calls,pow_calls);
    printf("digit_explicit_products=%lu digit_power_calls=%lu "
           "own_binary_power_reference_products=%lu loop_bound=6\n",
           digit_mul,digit_pow,binary_digit_products);
    return bad ? 1 : 0;
}
