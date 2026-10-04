#include "orc.h"
static ll fails=0,cnt=0;
#define F(...) do{ if(fails++<25){ printf("FAIL " __VA_ARGS__); printf("\n"); } }while(0)
static int H(const fmpq_t a,const fmpq_t b,ull p,int*st){
  adf_rat_t x,y; adf_rat_init(x);adf_rat_init(y); adf_rat_set_fmpq(x,a);adf_rat_set_fmpq(y,b);
  adf_place_t pl; if(p) adf_place_prime(&pl,p); else pl=adf_place_inf();
  int v=99; adf_place_t w; *st=adf_rat_hilbert_at(&v,&w,x,y,pl); adf_rat_clear(x);adf_rat_clear(y); cnt++; return v; }
static ull SP[]={2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53};
static void randq(fmpq_t q,flint_rand_t rs,int big){ /* +- prod p^e, e in -k..k, plus a random cofactor of primes list */
  fmpq_one(q); for(int i=0;i<16;i++){ int e=(int)n_randint(rs,big?60:3)-(big?30:1); fmpq_t t; fmpq_init(t); fmpq_set_ui(t,SP[i],1); fmpq_pow_si(t,t,e); fmpq_mul(q,q,t); fmpq_clear(t);} if(n_randint(rs,2)) fmpq_neg(q,q); }
int main(int argc,char**argv){
  setvbuf(stdout,NULL,_IONBF,0); int pert=argc>1?atoi(argv[1]):0;
  flint_rand_t rs; flint_randinit(rs);
  fmpq_t a,b,c,ab,t; fmpq_init(a);fmpq_init(b);fmpq_init(c);fmpq_init(ab);fmpq_init(t);
  ull ps[]={2,3,5,7,13,31,2305843009213693951ULL /*2^61-1*/,18446744073709551557ULL /*largest prime<2^64*/};
  ll bitsmax=0;
  for(int it=0;it<2000;it++){
    int big=it%2; randq(a,rs,big);randq(b,rs,big);randq(c,rs,big);
    if(big){ fmpq_t m; fmpq_init(m); fmpz_t z; fmpz_init(z); fmpz_randbits(z,rs,1000+n_randint(rs,1000)); if(fmpz_is_zero(z))fmpz_one(z); fmpz_t one_; fmpz_init_set_ui(one_,1); fmpq_set_fmpz_frac(m,z,one_); fmpq_canonicalise(m); fmpq_mul(a,a,m); fmpq_mul(a,a,m); fmpq_clear(m);fmpz_clear(z);}
    { slong bb=fmpz_bits(fmpq_numref(a)); if(bb>bitsmax)bitsmax=bb; }
    for(int pi=-1;pi<8;pi++){ ull p=pi<0?0:ps[pi]; int s1,s2,s3,s4,s5;
      int hab=H(a,b,p,&s1), hba=H(b,a,p,&s2);
      fmpq_mul(ab,a,c); int hac=H(ab,b,p,&s3); int hc=H(c,b,p,&s4), hab2=hab;
      if(s1||s2||s3||s4) F("status");
      if(hab!=hba) F("symmetry p=%llu it=%d",p,it);
      if(hac!=hab2*hc) F("bilinearity p=%llu it=%d",p,it);
      fmpq_neg(t,a); int h1=H(a,t,p,&s5); if(s5||h1!=1) F("(a,-a) p=%llu it=%d got %d",p,it,h1);
      fmpq_one(t); fmpq_sub(t,t,a); if(!fmpq_is_zero(t)){ int h2=H(a,t,p,&s5); if(s5||h2!=1) F("(a,1-a) p=%llu it=%d got %d",p,it,h2);}
      int hsq=H(a,a,p,&s5); int hm=H(a,(fmpq_set_si(t,-1,1),t),p,&s5); if(hsq!=hm) F("(a,a)!=(a,-1) p=%llu it=%d",p,it);
    }
    /* product formula over inf, and every p dividing 2ab: all primes are in SP */
    int prod=1; int s; for(int pi=-1;pi<16;pi++){ ull p=pi<0?0:SP[pi]; prod*=H(a,b,p,&s); }
    if(pert) prod=-prod;
    if(prod!=1) F("product formula it=%d got %d",it,prod);
  }
  printf("hilbert identities: queries=%lld max numerator bits=%lld fails=%lld\n",cnt,bitsmax,fails);
  /* Jacobi reciprocity and supplements, big */
  ll f0=fails,c0=cnt; fmpz_t X,Y,m1,s2; fmpz_init(X);fmpz_init(Y);fmpz_init(m1);fmpz_init(s2);
  for(int it=0;it<2000;it++){
    int bits=it%2?2000:60; fmpz_randbits(X,rs,bits); fmpz_randbits(Y,rs,bits); fmpz_abs(X,X);fmpz_abs(Y,Y);fmpz_setbit(X,0);fmpz_setbit(Y,0);
    if(fmpz_sgn(X)==0)fmpz_one(X); if(fmpz_sgn(Y)==0)fmpz_one(Y);
    int v1,v2,v3; adf_place_t w;
    cnt++; if(adf_fmpz_jacobi(&v1,&w,X,Y)||adf_fmpz_jacobi(&v2,&w,Y,X)) { F("recip status"); continue; }
    int e=(fmpz_fdiv_ui(X,4)==3&&fmpz_fdiv_ui(Y,4)==3)?-1:1; if(pert) e=-e;
    if(v1*v2!=e && v1!=0) F("reciprocity it=%d v1=%d v2=%d",it,v1,v2);
    if((v1==0)!=(v2==0)) F("zero asymmetry it=%d",it);
    fmpz_set_si(m1,-1); fmpz_set_ui(s2,2);
    adf_fmpz_jacobi(&v1,&w,m1,Y); int want=(fmpz_fdiv_ui(Y,4)==3)?-1:1; if(v1!=want) F("(-1/n) it=%d",it);
    adf_fmpz_jacobi(&v1,&w,s2,Y); want=(fmpz_fdiv_ui(Y,8)==1||fmpz_fdiv_ui(Y,8)==7)?1:-1; if(v1!=want) F("(2/n) it=%d",it);
    /* multiplicativity in the upper entry: (X Z / Y) = (X/Y)(Z/Y) */
    fmpz_mul(s2,X,X); adf_fmpz_jacobi(&v1,&w,s2,Y); int g=fmpz_is_one(Y)?1:0; (void)g; fmpz_t gg; fmpz_init(gg); fmpz_gcd(gg,X,Y); if(fmpz_is_one(gg)&&v1!=1) F("(X^2/Y) != 1 it=%d",it); fmpz_clear(gg);
    /* Kronecker bilinear in lower entry incl. negative/even/zero lower: (a/(bc)) = (a/b)(a/c) */
    fmpz_t Bn,Cn,BC; fmpz_init(Bn);fmpz_init(Cn);fmpz_init(BC);
    fmpz_randtest(Bn,rs,40);fmpz_randtest(Cn,rs,40); fmpz_mul(BC,Bn,Cn);
    int k1,k2,k3; cnt++; adf_fmpz_kronecker(&k1,NULL,X,Bn);adf_fmpz_kronecker(&k2,NULL,X,Cn);adf_fmpz_kronecker(&k3,NULL,X,BC);
    if(k3!=k1*k2) F("kronecker multiplicativity in lower entry: a=%s b=%s c=%s: %d %d %d",fmpz_sgn(X)>0?"+big":"-big",fmpz_get_str(NULL,10,Bn),fmpz_get_str(NULL,10,Cn),k1,k2,k3);
    fmpz_clear(Bn);fmpz_clear(Cn);fmpz_clear(BC);
  }
  printf("jacobi/kronecker identities: queries=%lld fails=%lld\n",cnt-c0,fails-f0);
  return 0;}
