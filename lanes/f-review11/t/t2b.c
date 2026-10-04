#include "orc.h"
static ll fails=0;
#define F(...) do{ if(fails++<25){ printf("FAIL " __VA_ARGS__); printf("\n"); } }while(0)
static void ratset(adf_rat_t r, ll n, ll d){ fmpz_t N,D; fmpz_init_set_si(N,n);fmpz_init_set_si(D,d); adf_rat_set_fmpz2(r,N,D); fmpz_clear(N);fmpz_clear(D);}
/* lball hilbert */
static ll lb_ok=0,lb_nd=0,lb_nd_single=0,lb_cases=0,lb_pts=0,lb_zero=0;
static void make_lball(adf_lball_t x,ull p,int zero,int exact,ll v,ll N,ll u){ /* ball (p^v u) + p^N Z_p, u unit integer */
  adf_place_t pl; adf_place_prime(&pl,p); adf_rat_t c; adf_rat_init(c);
  fmpz_t n,d; fmpz_init(n);fmpz_init(d); fmpz_set_si(n,u); fmpz_set_ui(d,1);
  if(v>=0){ for(ll i=0;i<v;i++) fmpz_mul_ui(n,n,p);} else { for(ll i=0;i<-v;i++) fmpz_mul_ui(d,d,p);}
  if(zero) fmpz_zero(n);
  adf_rat_set_fmpz2(c,n,d);
  if(exact) { int st=adf_lball_set_rat(x,pl,c); if(st) { printf("setrat st=%d\n",st); exit(1);} }
  else { int st=adf_lball_set_rat_ball(x,pl,c,N); if(st){printf("setball st=%d\n",st);exit(1);} }
  adf_rat_clear(c);fmpz_clear(n);fmpz_clear(d);
}
/* points of ball modulo p^(N+3): returns set bitmask of values for pair */
static void ball_pts(fmpz_t *num,fmpz_t *den,int *np,ull p,int zero,int exact,ll v,ll N,ll u){
  *np=0;
  if(exact){ fmpz_set_si(num[0],u); fmpz_set_ui(den[0],1); if(v>=0) for(ll i=0;i<v;i++) fmpz_mul_ui(num[0],num[0],p); else for(ll i=0;i<-v;i++) fmpz_mul_ui(den[0],den[0],p); *np=1; return; }
  ll e=N-v; /* unit known mod p^e */
  ull pe=ipow(p,e<0?0:e); ull T=ipow(p,3);
  for(ull t=0;t<T;t++){ fmpz_t un; fmpz_init(un); fmpz_set_ui(un,t); fmpz_mul_ui(un,un,pe); fmpz_add_ui(un,un,u);
    fmpz_set(num[*np],un); fmpz_set_ui(den[*np],1);
    if(v>=0) for(ll i=0;i<v;i++) fmpz_mul_ui(num[*np],num[*np],p); else for(ll i=0;i<-v;i++) fmpz_mul_ui(den[*np],den[*np],p);
    (*np)++; fmpz_clear(un);}
}
static void lball_test(int PRIME_LIST_N,ull *pl_){
  fmpz_t na[200],da[200],nb[200],db[200]; for(int i=0;i<200;i++){fmpz_init(na[i]);fmpz_init(da[i]);fmpz_init(nb[i]);fmpz_init(db[i]);}
  for(int pi=0;pi<PRIME_LIST_N;pi++){ ull p=pl_[pi];
    /* descriptor list of balls: v in -2..2, N in v+1..v+4 (u: all units < p^(N-v), up to 6), exact units, */
    typedef struct{int zero,exact;ll v,N,u;}D; D ds[600]; int nd=0;
    for(ll v=-2;v<=2;v++){
      for(ll e=1;e<=4;e++){ ll N=v+e; ull lim=ipow(p,e); int cntu=0; for(ull u=1;u<lim&&cntu<5;u++) if(u%p){ ds[nd++]=(D){0,0,v,N,(ll)u}; cntu++; }
        if(p==2||e<=3){ /* high u too */ for(ull u=lim-1;u>=1&&cntu<8;u--) if(u%p){ ds[nd++]=(D){0,0,v,N,(ll)u}; cntu++; if(u<lim-4)break;} } }
      for(ll u=1;u<=7;u++) if(u%p) {ds[nd++]=(D){0,1,v,0,u}; ds[nd++]=(D){0,1,v,0,-u};}
    }
    for(int i=0;i<nd;i++)for(int j=0;j<nd;j++){
      adf_lball_t A,B; adf_lball_init(A);adf_lball_init(B);
      make_lball(A,p,0,ds[i].exact,ds[i].v,ds[i].N,ds[i].u); make_lball(B,p,0,ds[j].exact,ds[j].v,ds[j].N,ds[j].u);
      int na_,nb_;
      ball_pts(na,da,&na_,p,0,ds[i].exact,ds[i].v,ds[i].N,ds[i].u); ball_pts(nb,db,&nb_,p,0,ds[j].exact,ds[j].v,ds[j].N,ds[j].u);
      int seen[2]={0,0};
      for(int x=0;x<na_;x++)for(int y=0;y<nb_;y++){ int r=o_hil(na[x],da[x],nb[y],db[y],p); seen[r>0]=1; lb_pts++; }
      int val=99; adf_place_t w; w=adf_place_inf(); int st=adf_lball_hilbert(&val,&w,A,B); lb_cases++;
      if(st==ADF_OK){ lb_ok++; if(seen[0]&&seen[1]) F("BLOCKER lball p=%llu OK but both signs: i=%d j=%d",p,i,j); else if(val!=(seen[1]?1:-1)) F("lball p=%llu wrong sign got %d i=%d j=%d (v,N,u)=(%lld,%lld,%lld)|(%lld,%lld,%lld)",p,val,i,j,ds[i].v,ds[i].N,ds[i].u,ds[j].v,ds[j].N,ds[j].u); if(!ds[i].exact||!ds[j].exact){} }
      else if(st==ADF_NOT_DETERMINED){ lb_nd++; if(!(seen[0]&&seen[1])){ lb_nd_single++; if(lb_nd_single<=6) printf("note: NOT_DETERMINED but singleton sign %d p=%llu a=(v%lld,N%lld,u%lld,ex%d) b=(v%lld,N%lld,u%lld,ex%d)\n",seen[1]?1:-1,p,ds[i].v,ds[i].N,ds[i].u,ds[i].exact,ds[j].v,ds[j].N,ds[j].u,ds[j].exact);} if(val!=99) F("value touched on ND"); }
      else F("lball unexpected status %d",st);
      adf_lball_clear(A);adf_lball_clear(B);
    }
  }
}
/* ideles */
static void idele_test(ull p){
  fmpz_t X,Nn,cc; fmpz_init(X);fmpz_init(Nn);fmpz_init(cc);
  ll Ns[]={0,1,2,3,4,5,6,7,8,9,10,12,16,25,27,32,40,45,64,15,18,24,81,125,16*5,8*9};
  int nN=sizeof(Ns)/sizeof(*Ns);
  typedef struct{ll rn,rd;ll c,N;}ID; ID ds[1500]; int nd=0;
  ll rs[][2]={{1,1},{3,1},{2,1},{4,1},{5,7},{1,3},{9,2},{1,8},{1,25},{3,4},{7,1},{27,5}};
  for(unsigned ri=0;ri<sizeof(rs)/sizeof(*rs);ri++)for(int ni=0;ni<nN;ni++){
    ll N=Ns[ni]; if(N==0){ ds[nd++]=(ID){rs[ri][0],rs[ri][1],1,0}; ds[nd++]=(ID){rs[ri][0],rs[ri][1],-1,0}; continue; }
    int k=0; for(ll c=1;c<=N&&k<3;c++){ ll g=c,h=N; while(h){ll t=g%h;g=h;h=t;} if(g==1){ ds[nd++]=(ID){rs[ri][0],rs[ri][1],c,N}; k++; if(k==1&&N>8) c+=N/2; } }
  }
  adf_place_t pl; adf_place_prime(&pl,p);
  ll cases=0,ok=0,nd_=0,ndsing=0;
  for(int i=0;i<nd;i++)for(int j=0;j<nd;j++){
    if((i*7+j)%(p==2?1:(p==3?2:40))) continue; /* thin out */
    adf_idele_t A,B; adf_idele_init(A);adf_idele_init(B);
    ID d[2]={ds[i],ds[j]}; adf_idele_struct *I[2]={A,B};
    fmpz_t num[2][700],den[2][700]; int np[2];
    int skip=0;
    for(int k=0;k<2;k++){
      arb_t one; arb_init(one); arb_one(one); fmpq_t r; fmpq_init(r); fmpq_set_si(r,d[k].rn,d[k].rd);
      adf_ucoset_t u; adf_ucoset_init(u); fmpz_set_si(cc,d[k].c); fmpz_set_si(Nn,d[k].N);
      if(adf_ucoset_set_fmpz2(u,cc,Nn)){ printf("ucoset st\n"); exit(1);}
      if(adf_idele_set_parts(I[k],one,r,u)){printf("set_parts\n");exit(1);}
      /* points: c' = c+N t, t< p^k *? */
      ull Mk=ipow(p,p==2?5:3); np[k]=0;
      if(d[k].N==0){ fmpz_init(num[k][0]);fmpz_init(den[k][0]); fmpz_set_si(num[k][0],d[k].c*d[k].rn); fmpz_set_si(den[k][0],d[k].rd); np[k]=1; }
      else for(ull t=0;t<Mk;t++){ ll c2=d[k].c+d[k].N*(ll)t; if(c2%p==0) continue; fmpz_init(num[k][np[k]]);fmpz_init(den[k][np[k]]); fmpz_set_si(num[k][np[k]],c2); fmpz_mul_si(num[k][np[k]],num[k][np[k]],d[k].rn); fmpz_set_si(den[k][np[k]],d[k].rd); np[k]++; }
      arb_clear(one); fmpq_clear(r); adf_ucoset_clear(u);
    }
    int seen[2]={0,0};
    for(int x=0;x<np[0];x++)for(int y=0;y<np[1];y++){ int r=o_hil(num[0][x],den[0][x],num[1][y],den[1][y],p); seen[r>0]=1; }
    int val=99; adf_place_t w=adf_place_inf(); int st=adf_idele_hilbert_at(&val,&w,A,B,pl); cases++;
    if(st==ADF_OK){ ok++; if(seen[0]&&seen[1]) F("BLOCKER idele p=%llu OK but both signs (r=%lld/%lld c=%lld N=%lld)|(r=%lld/%lld c=%lld N=%lld)",p,d[0].rn,d[0].rd,d[0].c,d[0].N,d[1].rn,d[1].rd,d[1].c,d[1].N); else if(val!=(seen[1]?1:-1)) F("idele wrong sign p=%llu got %d  (r=%lld/%lld c=%lld N=%lld)|(r=%lld/%lld c=%lld N=%lld)",p,val,d[0].rn,d[0].rd,d[0].c,d[0].N,d[1].rn,d[1].rd,d[1].c,d[1].N);}
    else if(st==ADF_NOT_DETERMINED){ nd_++; if(!(seen[0]&&seen[1])){ ndsing++; if(ndsing<=5) printf("note idele ND but singleton %d p=%llu (r=%lld/%lld c=%lld N=%lld)|(r=%lld/%lld c=%lld N=%lld)\n",seen[1]?1:-1,p,d[0].rn,d[0].rd,d[0].c,d[0].N,d[1].rn,d[1].rd,d[1].c,d[1].N);} }
    else F("idele status %d",st);
    for(int k=0;k<2;k++)for(int x=0;x<np[k];x++){fmpz_clear(num[k][x]);fmpz_clear(den[k][x]);}
    adf_idele_clear(A);adf_idele_clear(B);
  }
  printf("idele p=%llu cases=%lld OK=%lld ND=%lld ND-but-singleton=%lld\n",p,cases,ok,nd_,ndsing);
}
int main(int argc,char**argv){
  PERTURB=argc>1?atoi(argv[1]):0; setvbuf(stdout,NULL,_IONBF,0); int only=argc>2?atoi(argv[2]):0;
  ull pl[]={2,3,5};
  if(only!=2){ lball_test(3,pl);
  if(only!=2) printf("lball: cases=%lld pairs-of-points=%lld OK=%lld ND=%lld ND-singleton=%lld fails=%lld\n",lb_cases,lb_pts,lb_ok,lb_nd,lb_nd_single,fails);
  } if(only!=1){ idele_test(5);}
  printf("fails=%lld\n",fails);
  return 0; }
