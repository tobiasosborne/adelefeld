#include "orc.h"
static ll fails=0,cnt=0;
static void fail(const char*s,ll a,ll b,ll c,int got,int want){ if(fails++<15) printf("FAIL %s a=%lld b=%lld p=%lld got=%d want=%d\n",s,a,b,c,got,want);}
int main(int argc,char**argv){
  PERTURB=argc>1?atoi(argv[1]):0;
  fmpz_t A,B; fmpz_init(A);fmpz_init(B); int v; adf_place_t w;
  ull primes[]={3,5,7,11,13,101,3001,10007};
  ll f0,c0;
  for(ll a=-150;a<=150;a++){ fmpz_set_si(A,a);
    for(int i=0;i<8;i++){ adf_place_t pl; adf_place_prime(&pl,primes[i]);
      int st=adf_fmpz_legendre(&v,&w,A,pl); int want=o_leg(A,primes[i]); cnt++; if(st||v!=want) fail("leg",a,0,primes[i],v,want);}
    for(ll b=-60;b<=60;b++){ fmpz_set_si(B,b); int want=o_kron(A,b); v=7;
      int st=adf_fmpz_kronecker(&v,NULL,A,B); cnt++; if(st||v!=want) fail("kron",a,b,0,v,want);
      v=7; st=adf_fmpz_jacobi(&v,&w,A,B);
      if(b>0&&b%2){ cnt++; if(st||v!=want) fail("jac",a,b,0,v,want);} else { cnt++; if(st!=ADF_DOMAIN||v!=7) fail("jacdom",a,b,0,st,ADF_DOMAIN);} }
  }
  printf("sym box: cnt=%lld fails=%lld\n",cnt,fails);
  flint_rand_t rs; flint_randinit(rs); f0=fails; c0=cnt;
  for(int t=0;t<400;t++){ fmpz_randbits(A,rs,2000); if(t&1) fmpz_neg(A,A);
    ll bs[]={0,1,-1,2,-2,4,6,8,-8,12,15,-15,45,1000,-999,1<<20,(1LL<<30)+5,3LL*5*7*11*13*17*19*23,-(1LL<<40)};
    for(int i=0;i<19;i++){ fmpz_set_si(B,bs[i]); int want=o_kron(A,bs[i]); v=7; int st=adf_fmpz_kronecker(&v,NULL,A,B); cnt++; if(st||v!=want) fail("bigkron",t,bs[i],0,v,want);} }
  printf("big kron cnt=%lld fails=%lld\n",cnt-c0,fails-f0);
  ull hp[]={0,2,3,5,7,13}; f0=fails; c0=cnt;
  for(int pi=0;pi<6;pi++)for(ll a=-130;a<=130;a++)for(ll b=-130;b<=130;b++){ if(!a||!b)continue;
    adf_rat_t ra,rb; adf_rat_init(ra);adf_rat_init(rb);adf_rat_set_si(ra,a);adf_rat_set_si(rb,b);
    adf_place_t pl; if(hp[pi]) adf_place_prime(&pl,hp[pi]); else pl=adf_place_inf();
    v=7; int st=adf_rat_hilbert_at(&v,&w,ra,rb,pl); int want=o_hil_si(a,b,hp[pi]); cnt++;
    if(st||v!=want) fail("hil",a,b,hp[pi],v,want);
    adf_rat_clear(ra);adf_rat_clear(rb);}
  printf("hilbert int box cnt=%lld fails=%lld\n",cnt-c0,fails-f0);
  f0=fails;c0=cnt;
  for(int pi=1;pi<6;pi++){ ull p=hp[pi]; adf_place_t pl; adf_place_prime(&pl,p);
    for(int t=0;t<6000;t++){ fmpz_t n[2],d[2]; adf_rat_t r[2];
      for(int j=0;j<2;j++){ fmpz_init(n[j]);fmpz_init(d[j]); adf_rat_init(r[j]);
        fmpz_randtest_not_zero(n[j],rs,t%2?40:2000); fmpz_randtest_not_zero(d[j],rs,t%3?10:600); if(fmpz_sgn(d[j])<0)fmpz_neg(d[j],d[j]);
        int e=n_randint(rs,5); for(int i=0;i<e;i++){ if(n_randint(rs,2)) fmpz_mul_ui(n[j],n[j],p); else fmpz_mul_ui(d[j],d[j],p);}
        adf_rat_set_fmpz2(r[j],n[j],d[j]); }
      v=7; int st=adf_rat_hilbert_at(&v,&w,r[0],r[1],pl);
      int want=o_hil(n[0],d[0],n[1],d[1],p); cnt++; if(st||v!=want) fail("rathil",t,0,p,v,want);
      for(int j=0;j<2;j++){fmpz_clear(n[j]);fmpz_clear(d[j]);adf_rat_clear(r[j]);} } }
  printf("hilbert rat cnt=%lld fails=%lld\n",cnt-c0,fails-f0);
  f0=fails;c0=cnt; adf_place_t two; adf_place_prime(&two,2);
  for(int al=0;al<2;al++)for(int u=1;u<8;u+=2)for(int be=0;be<2;be++)for(int ww=1;ww<8;ww+=2){
    ll a=u*(al?2:1), b=ww*(be?2:1); adf_rat_t ra,rb; adf_rat_init(ra);adf_rat_init(rb);adf_rat_set_si(ra,a);adf_rat_set_si(rb,b);
    v=7; int st=adf_rat_hilbert_at(&v,&w,ra,rb,two); int want=o_hil_si(a,b,2);cnt++; if(st||v!=want)fail("cls2",a,b,2,v,want);
    ll a2=-(a+8*5), b2=b+8*3; fmpz_t X,Y,o; fmpz_init_set_si(X,a2);fmpz_init_set_si(Y,b2);fmpz_init_set_ui(o,1);
    adf_rat_set_si(ra,a2);adf_rat_set_si(rb,b2); st=adf_rat_hilbert_at(&v,&w,ra,rb,two); want=o_hil(X,o,Y,o,2); cnt++; if(st||v!=want)fail("cls2b",a2,b2,2,v,want);
    fmpz_clear(X);fmpz_clear(Y);fmpz_clear(o);adf_rat_clear(ra);adf_rat_clear(rb);}
  printf("2-classes cnt=%lld fails=%lld\n",cnt-c0,fails-f0);
  /* oracle cross-check: k+1 */
  if(argc>2){ KOVR=0; ll d=0,n=0; for(int pi=1;pi<6;pi++)for(ll a=-20;a<=20;a++)for(ll b=-20;b<=20;b++){if(!a||!b)continue; int x=o_hil_si(a,b,hp[pi]); nmemo=0; KOVR=(hp[pi]==2)?6:(hp[pi]<=5?4:3); int y=o_hil_si(a,b,hp[pi]); KOVR=0; nmemo=0; n++; if(x!=y)d++;} printf("k vs k+1 (p<=5; 7,13 k=3 both): %lld cases, %lld differ\n",n,d);}
  printf("TOTAL cnt=%lld fails=%lld\n",cnt,fails);
  return 0;}
