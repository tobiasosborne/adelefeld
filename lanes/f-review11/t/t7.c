#include "orc.h"
static ll fails=0,cnt=0;
#define F(...) do{ if(fails++<25){ printf("FAIL " __VA_ARGS__); printf("\n"); } }while(0)
int main(int argc,char**argv){
  int pert=argc>1; setvbuf(stdout,NULL,_IONBF,0);
  flint_rand_t rs; flint_randinit(rs);
  ull ps[]={2305843009213693951ULL,18446744073709551557ULL,9223372036854775783ULL,9223372036854775837ULL,18446744073709551533ULL,4294967291ULL,4294967311ULL};
  for(int pi=0;pi<7;pi++){ ull p=ps[pi]; adf_place_t pl; if(adf_place_prime(&pl,p)) {printf("not prime %llu\n",p); continue;}
    for(int it=0;it<3000;it++){
      fmpz_t n[2],d[2],un[2],ud[2]; adf_rat_t r[2]; adf_lball_t L[2]; adf_idele_t I[2]; int al[2];
      for(int j=0;j<2;j++){ fmpz_init(n[j]);fmpz_init(d[j]);fmpz_init(un[j]);fmpz_init(ud[j]);adf_rat_init(r[j]);adf_lball_init(L[j]);adf_idele_init(I[j]);
        fmpz_randtest_not_zero(n[j],rs,it%2?20:200); fmpz_randtest_not_zero(d[j],rs,it%3?6:100); if(fmpz_sgn(d[j])<0)fmpz_neg(d[j],d[j]);
        if(fmpz_fdiv_ui(n[j],p)==0) fmpz_add_ui(n[j],n[j],1); if(fmpz_fdiv_ui(n[j],p)==0) fmpz_add_ui(n[j],n[j],1); if(fmpz_fdiv_ui(d[j],p)==0) fmpz_add_ui(d[j],d[j],1); if(fmpz_fdiv_ui(n[j],p)==0||fmpz_fdiv_ui(d[j],p)==0||fmpz_is_zero(n[j])) fmpz_set_ui(n[j],5); fmpz_set(un[j],n[j]); fmpz_set(ud[j],d[j]);
        al[j]=n_randint(rs,3); /* 0,1,2 : power of p in numerator; 3 -> denominator */
        int e=al[j]; if(e==2){ fmpz_mul_ui(d[j],d[j],p);} else if(e==1) fmpz_mul_ui(n[j],n[j],p);
        adf_rat_set_fmpz2(r[j],n[j],d[j]); adf_lball_set_rat(L[j],pl,r[j]); adf_idele_set_rat(I[j],r[j],64); }
      int a=al[0]!=0, b=al[1]!=0; /* parity of valuation; e==2 -> denominator -> odd */
      /* units u,w : n d (the part prime to p) */
      fmpz_t u,w,pr; fmpz_init(u);fmpz_init(w);fmpz_init(pr); fmpz_mul(u,un[0],ud[0]); fmpz_mul(w,un[1],ud[1]);
      int ls_u=o_leg(u,p), ls_w=o_leg(w,p);
      int want=((a&&b&&(p%4==3))?-1:1)*(b?ls_u:1)*(a?ls_w:1);
      if(pert) want=-want;
      int v1=99,v2=99,v3=99,w_; adf_place_t wh;
      int s1=adf_rat_hilbert_at(&v1,&wh,r[0],r[1],pl), s2=adf_lball_hilbert(&v2,&wh,L[0],L[1]), s3=adf_idele_hilbert_at(&v3,&wh,I[0],I[1],pl); (void)w_;
      cnt++; if(s1||s2||s3||v1!=want||v2!=want||v3!=want) F("p=%llu it=%d a=%d b=%d rat=%d lball=%d idele=%d want=%d st=%d%d%d",p,it,a,b,v1,v2,v3,want,s1,s2,s3);
      for(int j=0;j<2;j++){ fmpz_clear(n[j]);fmpz_clear(d[j]);fmpz_clear(un[j]);fmpz_clear(ud[j]);adf_rat_clear(r[j]);adf_lball_clear(L[j]);adf_idele_clear(I[j]);}
      fmpz_clear(u);fmpz_clear(w);fmpz_clear(pr);
    } }
  /* Legendre / Jacobi / Kronecker large prime lower entries vs Euler */
  { fmpz_t A,B; fmpz_init(A);fmpz_init(B); ll f0=fails,c0=cnt; for(int pi=0;pi<7;pi++)for(int it=0;it<2000;it++){ adf_place_t pl; adf_place_prime(&pl,ps[pi]); fmpz_randtest(A,rs,it%2?100:3000);
      int v; adf_place_t wh; int st=adf_fmpz_legendre(&v,&wh,A,pl); int want=o_leg(A,ps[pi]); cnt++; if(pert)want=-want; if(st||v!=want) F("leg large p=%llu got %d want %d",ps[pi],v,want);
      fmpz_set_ui(B,ps[pi]); int k; st=adf_fmpz_jacobi(&k,&wh,A,B); cnt++; if(st||k!=want) F("jac large");
      st=adf_fmpz_kronecker(&k,NULL,A,B); cnt++; if(st||k!=want) F("kron large");
      adf_ucoset_t U; adf_ucoset_init(U); fmpz_t c,N; fmpz_init(c);fmpz_init(N); fmpz_set_ui(N,ps[pi]); fmpz_mul_ui(N,N,3); fmpz_set_ui(c,1); fmpz_add(c,c,N); fmpz_add_ui(c,c,ps[pi]*0+0);
      /* unit coset c mod 3p: c = 1 + 3p.. gcd 1 ; symbol is that of 1 -> 1 at p */
      fmpz_set_ui(c,1); if(adf_ucoset_set_fmpz2(U,c,N)==0){ st=adf_ucoset_legendre(&k,&wh,U,pl); cnt++; if(st||k!=1) F("uc leg large st=%d k=%d",st,k);}
      adf_ucoset_clear(U); fmpz_clear(c);fmpz_clear(N); }
    printf("large-p legendre/jacobi/kronecker/ucoset cases=%lld\n",cnt-c0); fmpz_clear(A);fmpz_clear(B); }
  printf("t7 large primes (2^61-1, 2^64-59, 2^63-25, ...): cases=%lld fails=%lld\n",cnt,fails); flint_randclear(rs); flint_cleanup(); return 0; }
