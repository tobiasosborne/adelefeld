#include "orc.h"
static ll fails=0,cnt=0;
#define F(...) do{ if(fails++<25){ printf("FAIL " __VA_ARGS__); printf("\n"); } }while(0)
int main(int argc,char**argv){
  setvbuf(stdout,NULL,_IONBF,0); int pert=argc>1;
  flint_rand_t rs; flint_randinit(rs);
  ull ps[]={0,2,3,5,7,13,1000003,2305843009213693951ULL,18446744073709551557ULL,18446744073709551521ULL};
  for(int it=0;it<3000;it++){
    for(int pi=0;pi<10;pi++){ ull p=ps[pi]; adf_place_t pl; if(p) adf_place_prime(&pl,p); else pl=adf_place_inf();
      fmpz_t n[2],d[2]; adf_rat_t r[2]; adf_idele_t I[2]; adf_lball_t L[2];
      for(int j=0;j<2;j++){ fmpz_init(n[j]);fmpz_init(d[j]);adf_rat_init(r[j]);adf_idele_init(I[j]);adf_lball_init(L[j]);
        fmpz_randtest_not_zero(n[j],rs,it%2?30:1500); fmpz_randtest_not_zero(d[j],rs,it%3?8:500); if(fmpz_sgn(d[j])<0)fmpz_neg(d[j],d[j]);
        if(p){ int e=n_randint(rs,4); for(int i=0;i<e;i++){ if(n_randint(rs,2)) fmpz_mul_ui(n[j],n[j],p); else fmpz_mul_ui(d[j],d[j],p);} }
        adf_rat_set_fmpz2(r[j],n[j],d[j]); adf_idele_set_rat(I[j],r[j],64); if(p) adf_lball_set_rat(L[j],pl,r[j]); }
      int s1,s2,s3=0; int v1=99,v2=99,v3=99; adf_place_t w;
      s1=adf_rat_hilbert_at(&v1,&w,r[0],r[1],pl); s2=adf_idele_hilbert_at(&v2,&w,I[0],I[1],pl); if(p) s3=adf_lball_hilbert(&v3,&w,L[0],L[1]); else v3=v1;
      int want=(p==0||p<=13)?o_hil(n[0],d[0],n[1],d[1],p):v1; cnt++;
      if(pert) want=-want;
      if(s1||s2||s3||v1!=v2||v1!=v3||v1!=want) F("consistency it=%d p=%llu rat=%d idele=%d lball=%d (st %d %d %d) oracle=%d",it,p,v1,v2,v3,s1,s2,s3,want);
      for(int j=0;j<2;j++){ fmpz_clear(n[j]);fmpz_clear(d[j]);adf_rat_clear(r[j]);adf_idele_clear(I[j]);adf_lball_clear(L[j]);}
    } }
  flint_randclear(rs); flint_cleanup(); printf("t6 rat/idele/lball consistency (10 places incl. 2^64-59, 2^61-1): cases=%lld fails=%lld\n",cnt,fails); return 0; }
