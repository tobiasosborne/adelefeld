#include "orc.h"
#include <limits.h>
static ll fails=0,cnt=0;
#define F(...) do{ if(fails++<40){ printf("FAIL " __VA_ARGS__); printf("\n"); } }while(0)
#define CHECK(c,...) do{ cnt++; if(!(c)) F(__VA_ARGS__); }while(0)
static adf_place_t P(ull p){ adf_place_t x; if(p) adf_place_prime(&x,p); else x=adf_place_inf(); return x; }
static void mk(adf_lball_t x,ull p,ll n,ll d,ll N,int exact){ adf_rat_t c; adf_rat_init(c); fmpz_t a,b; fmpz_init_set_si(a,n);fmpz_init_set_si(b,d); adf_rat_set_fmpz2(c,a,b);
  if(exact) adf_lball_set_rat(x,P(p),c); else adf_lball_set_rat_ball(x,P(p),c,N); adf_rat_clear(c);fmpz_clear(a);fmpz_clear(b);}
int main(void){
  setvbuf(stdout,NULL,_IONBF,0);
  int v; adf_place_t w; fmpz_t A,B; fmpz_init_set_si(A,5);fmpz_init_set_si(B,3);
  /* legendre domains */
  v=99; w=P(7); int st=adf_fmpz_legendre(&v,&w,A,P(0)); CHECK(st==ADF_DOMAIN&&v==99&&adf_place_is_archimedean(w),"leg inf st=%d",st);
  v=99; w=P(7); st=adf_fmpz_legendre(&v,&w,A,P(2)); CHECK(st==ADF_DOMAIN&&v==99&&adf_place_prime_get(w)==2,"leg 2");
  v=99; st=adf_fmpz_legendre(&v,NULL,A,P(2)); CHECK(st==ADF_DOMAIN&&v==99,"leg 2 NULL");
  v=99; w=P(7); st=adf_fmpz_legendre(&v,&w,A,P(3)); CHECK(st==ADF_OK&&adf_place_prime_get(w)==7,"where touched on OK");
  { adf_fball_t X; adf_fball_init(X); adf_fball_set_si(X,5); v=99;w=P(7); st=adf_fball_legendre(&v,&w,X,P(0)); CHECK(st==ADF_DOMAIN&&v==99&&adf_place_is_archimedean(w),"fb leg inf");
    fmpz_t a,h,d; fmpz_init_set_si(a,1);fmpz_init_set_si(h,2);fmpz_init_set_si(d,1); adf_fball_set_fmpz3(X,a,h,d); v=99; w=P(7); st=adf_fball_legendre(&v,&w,X,P(5)); CHECK(st==ADF_NOT_DETERMINED&&v==99&&adf_place_prime_get(w)==5,"fb leg ND where=p st=%d",st);
    adf_fball_clear(X);fmpz_clear(a);fmpz_clear(h);fmpz_clear(d);}
  { adf_ucoset_t U; adf_ucoset_init(U); fmpz_t c,n; fmpz_init_set_si(c,1);fmpz_init_set_si(n,4); adf_ucoset_set_fmpz2(U,c,n); v=99;w=P(7); st=adf_ucoset_legendre(&v,&w,U,P(3)); CHECK(st==ADF_NOT_DETERMINED&&v==99&&adf_place_prime_get(w)==3,"uc leg ND");
    st=adf_ucoset_legendre(&v,&w,U,P(2)); CHECK(st==ADF_DOMAIN,"uc leg 2"); fmpz_set_si(n,0); fmpz_set_si(c,-1); adf_ucoset_set_fmpz2(U,c,n); st=adf_ucoset_legendre(&v,&w,U,P(7)); CHECK(st==ADF_OK&&v==-1,"uc exact -1 mod 7 -> %d",v);
    st=adf_ucoset_kronecker(&v,NULL,U,B); CHECK(st==ADF_OK,"kr"); adf_ucoset_clear(U);fmpz_clear(c);fmpz_clear(n);}
  /* lball statuses */
  { adf_lball_t a,b,z,zb; adf_lball_init(a);adf_lball_init(b);adf_lball_init(z);adf_lball_init(zb);
    mk(a,3,1,1,0,1); mk(b,5,1,1,0,1); v=99; w=P(0); st=adf_lball_hilbert(&v,&w,a,b); CHECK(st==ADF_DOMAIN&&v==99&&adf_place_prime_get(w)==3,"lball diff primes where=smaller (3) st=%d",st);
    v=99; w=P(0); st=adf_lball_hilbert(&v,&w,b,a); CHECK(st==ADF_DOMAIN&&adf_place_prime_get(w)==3,"lball diff primes swapped");
    mk(z,3,0,1,0,1); mk(zb,3,0,1,4,0); mk(a,3,1,1,5,0); mk(b,3,1,1,0,1);
    v=99; w=P(0); st=adf_lball_hilbert(&v,&w,z,b); CHECK(st==ADF_DOMAIN&&v==99&&adf_place_prime_get(w)==3,"exact zero");
    v=99; w=P(0); st=adf_lball_hilbert(&v,&w,b,z); CHECK(st==ADF_DOMAIN,"exact zero 2nd");
    v=99; w=P(0); st=adf_lball_hilbert(&v,&w,zb,b); CHECK(st==ADF_NOT_DETERMINED&&v==99&&adf_place_prime_get(w)==3,"zero-containing ball st=%d",st);
    v=99; w=P(0); st=adf_lball_hilbert(&v,&w,zb,z); CHECK(st==ADF_DOMAIN,"zero ball + exact zero -> DOMAIN st=%d",st);
    v=99; w=P(0); st=adf_lball_hilbert(&v,&w,z,zb); CHECK(st==ADF_DOMAIN,"exact zero + zero ball");
    v=99; w=P(7); st=adf_lball_hilbert(&v,&w,a,b); CHECK(st==ADF_OK&&adf_place_prime_get(w)==7,"where untouched on OK (lball)");
    v=99; st=adf_lball_hilbert(&v,NULL,zb,b); CHECK(st==ADF_NOT_DETERMINED,"where NULL");
    /* different primes and zero: zero at 3 vs prime 5 exact ->? */
    mk(b,5,1,1,0,1); v=99; w=P(0); st=adf_lball_hilbert(&v,&w,z,b); CHECK(st==ADF_DOMAIN&&adf_place_prime_get(w)==3,"zero + other prime");
    /* aliasing: both inputs same object, output value pointer */
    mk(b,3,1,1,0,1); v=99; st=adf_lball_hilbert(&v,&w,b,b); CHECK(st==ADF_OK&&v==1,"(1,1)=1");
    /* extreme exponents by forging fields; N-v = 1 or 3, p=2 and 3 */
    adf_lball_t e; adf_lball_init(e);
    ll vs[]={LONG_MAX-3,LONG_MAX-2,LONG_MAX-1,LONG_MIN,LONG_MIN+1,LONG_MIN+2,(1LL<<62)+1,-(1LL<<62)-1};
    for(int i=0;i<8;i++)for(int dN=1;dN<=4;dN++){ ll vv=vs[i]; if(vv>LONG_MAX-dN) continue;
      for(int u=1;u<(1<<dN)&&u<16;u+=2){
        e->p=2; fmpq_set_si(e->u,u,1); e->v=vv; e->N=vv+dN; e->exact=0;
        if(!adf_lball_is_canonical(e)) { continue; }
        mk(b,2,3,1,0,1); /* b = exact 3 */
        v=99; st=adf_lball_hilbert(&v,&w,e,b);
        /* truth: a = 2^vv * u', u' = u mod 2^dN; b=3: (a,3)_2 = (-1)^{eps(u)eps(3)+ vv*om(3)... }: use oracle on all lifts mod 8 */
        int seen[2]={0,0}; for(ull uu=u;uu<(1ULL<<(dN+3));uu+=(1ULL<<dN)){ seen[o_hil_parts(2,(int)(vv&1),uu%32,0,3)>0]=1; }
        int want_nd=seen[0]&&seen[1]; CHECK((st==ADF_NOT_DETERMINED)==want_nd && (st==ADF_OK? v==(seen[1]?1:-1):1),"extreme v=%lld dN=%d u=%d st=%d v=%d",vv,dN,u,st,v);
      } }
    adf_lball_clear(e);
    adf_lball_clear(a);adf_lball_clear(b);adf_lball_clear(z);adf_lball_clear(zb); }
  /* real */
  { arb_t x,y; arb_init(x);arb_init(y); arb_set_si(x,-3); arb_set_si(y,-2); v=99; w=P(7); st=adf_real_hilbert(&v,&w,x,y); CHECK(st==ADF_OK&&v==-1&&adf_place_prime_get(w)==7,"real -,-");
    arb_set_si(y,2); st=adf_real_hilbert(&v,&w,x,y); CHECK(st==ADF_OK&&v==1,"real -,+");
    arb_zero(y); v=99; w=P(7); st=adf_real_hilbert(&v,&w,x,y); CHECK(st==ADF_DOMAIN&&v==99&&adf_place_is_archimedean(w),"real zero");
    arb_set_si(y,1); mag_set_ui(arb_radref(y),2); v=99; st=adf_real_hilbert(&v,&w,x,y); CHECK(st==ADF_NOT_DETERMINED&&v==99,"real contains zero");
    arb_zero(x); v=99; st=adf_real_hilbert(&v,&w,x,y); CHECK(st==ADF_DOMAIN,"exact zero beats ball containing zero");
    arb_set_si(x,-3); arb_pos_inf(y); v=99; st=adf_real_hilbert(&v,&w,x,y); CHECK(st==ADF_DOMAIN&&v==99,"inf");
    arb_indeterminate(y); st=adf_real_hilbert(&v,&w,x,y); CHECK(st==ADF_DOMAIN,"nan");
    arb_set_si(y,-1); mag_inf(arb_radref(y)); st=adf_real_hilbert(&v,&w,x,y); CHECK(st==ADF_DOMAIN,"rad inf");
    arb_set_si(y,-1); arb_set_si(x,-1); mag_set_ui(arb_radref(x),1); /* [-2,0]: contains 0 at the end */ st=adf_real_hilbert(&v,&w,y,x); CHECK(st==ADF_NOT_DETERMINED,"[-2,0] closed end contains zero st=%d",st);
    arb_set_si(x,-1); mag_set_ui_2exp_si(arb_radref(x),1,-3); st=adf_real_hilbert(&v,&w,x,x); CHECK(st==ADF_OK&&v==-1,"tight negative");
    arb_clear(x);arb_clear(y); }
  /* rat at places */
  { adf_rat_t a,b; adf_rat_init(a);adf_rat_init(b); adf_rat_set_si(a,3);adf_rat_zero(b); v=99; w=P(7); st=adf_rat_hilbert_at(&v,&w,a,b,P(5)); CHECK(st==ADF_DOMAIN&&v==99&&adf_place_prime_get(w)==5,"rat zero where=v");
    st=adf_rat_hilbert_at(&v,&w,a,b,P(0)); CHECK(st==ADF_DOMAIN&&adf_place_is_archimedean(w),"rat zero inf");
    st=adf_rat_hilbert_at(&v,NULL,a,b,P(0)); CHECK(st==ADF_DOMAIN,"rat NULL");
    adf_rat_set_si(b,5); w=P(7); st=adf_rat_hilbert_at(&v,&w,a,b,P(2)); CHECK(st==ADF_OK&&adf_place_prime_get(w)==7,"rat OK keeps where");
    adf_rat_clear(a);adf_rat_clear(b);}
  /* idele */
  { adf_idele_t a,b; adf_idele_init(a);adf_idele_init(b); adf_rat_t q; adf_rat_init(q); adf_rat_set_si(q,-6); adf_idele_set_rat(a,q,64); adf_rat_set_si(q,-1); adf_idele_set_rat(b,q,64);
    v=99; w=P(7); st=adf_idele_hilbert_at(&v,&w,a,b,P(0)); CHECK(st==ADF_OK&&v==-1,"idele inf (-6,-1)=-1");
    st=adf_idele_hilbert_at(&v,&w,a,b,P(3)); CHECK(st==ADF_OK&&v==o_hil_si(-6,-1,3),"idele 3 got %d want %d",v,o_hil_si(-6,-1,3));
    st=adf_idele_hilbert_at(&v,&w,a,b,P(2)); CHECK(st==ADF_OK&&v==o_hil_si(-6,-1,2),"idele 2");
    adf_rat_clear(q);adf_idele_clear(a);adf_idele_clear(b);}
  printf("t5: checks=%lld fails=%lld\n",cnt,fails); return fails!=0; }
