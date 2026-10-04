#include "orc.h"
static ll fails=0;
#define F(...) do{ if(fails++<25){ printf("FAIL " __VA_ARGS__); printf("\n"); } }while(0)
static ll gcdl(ll a,ll b){ if(a<0)a=-a; if(b<0)b=-b; while(b){ll t=a%b;a=b;b=t;} return a; }
int main(int argc,char**argv){
  PERTURB=argc>1?atoi(argv[1]):0; setvbuf(stdout,NULL,_IONBF,0);
  ll bs[]={1,3,5,7,9,15,21,25,45,2,4,6,8,10,12,16,18,24,30,32,48,0,-1,-3,-2,-8,-15};
  int nb=sizeof(bs)/sizeof(*bs);
  ll uc_cases=0,uc_ok=0,uc_nd=0,uc_nd_single=0,fb_cases=0,fb_ok=0,fb_nd=0,fb_nd_single=0,dom=0;
  fmpz_t A,H,d,B,C,Nn,r; fmpz_init(A);fmpz_init(H);fmpz_init(d);fmpz_init(B);fmpz_init(C);fmpz_init(Nn);fmpz_init(r);
  for(int bi=0;bi<nb;bi++){ ll b=bs[bi]; fmpz_set_si(B,b);
    for(int kind=0;kind<3;kind++){ /* 0 legendre (b odd prime only), 1 jacobi, 2 kronecker */
      if(kind==0 && !(b==3||b==5||b==7)) continue;
      for(ll N=0;N<=96;N++){
        for(ll c=(N==0?-1:1);c<=(N==0?1:N);c+=(N==0?2:1)){
          if(N>0 && gcdl(c,N)!=1) continue;
          if(N>0 && (c%7==3) && N>40) continue; /* thin */
          adf_ucoset_t U; adf_ucoset_init(U); fmpz_set_si(C,c);fmpz_set_si(Nn,N);
          if(adf_ucoset_set_fmpz2(U,C,Nn)){ adf_ucoset_clear(U); continue; }
          int v=99,st; adf_place_t w=adf_place_inf();
          if(kind==0){ adf_place_t pl; adf_place_prime(&pl,b); st=adf_ucoset_legendre(&v,&w,U,pl);}
          else if(kind==1) st=adf_ucoset_jacobi(&v,&w,U,B); else st=adf_ucoset_kronecker(&v,&w,U,B);
          uc_cases++;
          int domain_expected=(kind==1&&(b<=0||b%2==0));
          if(domain_expected){ if(st!=ADF_DOMAIN||v!=99) F("uc dom kind%d b=%lld st=%d",kind,b,st); adf_ucoset_clear(U); continue; }
          if(st==ADF_OK){ uc_ok++;
            if(N==0){ int want=o_kron(C,b); if(v!=want) F("uc exact c=%lld b=%lld got %d want %d",c,b,v,want); }
            else { ll bb=b<0?-b:b; if(b<=0) { F("uc OK with N>0,b<=0"); } else { ll K=bb; if(kind==2&&bb%2==0){ while(K%2==0)K/=2; K*=8; }
              ll L=N/gcdl(N,K)*K; int seen[3]={0,0,0}; /* every unit residue mod L congruent c mod N */
              for(ll rr=0;rr<L;rr++){ if(gcdl(rr,L)!=1&&!(L==1)) continue; if((rr-c)%N) continue; fmpz_set_si(r,rr); seen[o_kron(r,b)+1]=1; }
              if(seen[0]+seen[1]+seen[2]>1) F("BLOCKER uc OK but points differ kind%d c=%lld N=%lld b=%lld",kind,c,N,b);
              else if(v!=(seen[2]?1:seen[1]?0:-1)) F("uc wrong sign kind%d c=%lld N=%lld b=%lld got %d",kind,c,N,b,v); } }
          } else if(st==ADF_NOT_DETERMINED){ uc_nd++; if(v!=99) F("uc value touched");
            if(N>0 && b>0){ ll K=b; if(kind==2&&b%2==0){ while(K%2==0)K/=2; K*=8; } ll L=N/gcdl(N,K)*K; int seen[3]={0,0,0};
              for(ll rr=0;rr<L;rr++){ if(gcdl(rr,L)!=1&&L!=1) continue; if((rr-c)%N) continue; fmpz_set_si(r,rr); seen[o_kron(r,b)+1]=1; }
              if(seen[0]+seen[1]+seen[2]<=1) uc_nd_single++; } }
          else F("uc status %d",st);
          adf_ucoset_clear(U);
        }
      }
      /* finite balls A + H Zhat over d */
      for(ll Hh=0;Hh<=50;Hh++)for(ll dd=1;dd<=4;dd++)for(ll Aa=-12;Aa<=12;Aa+=1){
        if((Aa*3+Hh)%5==0 && Hh>20) continue;
        fmpz_set_si(A,Aa);fmpz_set_si(H,Hh);fmpz_set_si(d,dd);
        adf_fball_t X; adf_fball_init(X); if(adf_fball_set_fmpz3(X,A,H,d)){adf_fball_clear(X);continue;}
        int v=99,st; adf_place_t w=adf_place_inf();
        if(kind==0){ adf_place_t pl; adf_place_prime(&pl,b); st=adf_fball_legendre(&v,&w,X,pl);}
        else if(kind==1) st=adf_fball_jacobi(&v,&w,X,B); else st=adf_fball_kronecker(&v,&w,X,B);
        fb_cases++;
        fmpz_t cA,cH,cd; fmpz_init(cA);fmpz_init(cH);fmpz_init(cd); adf_fball_get_fmpz3(cA,cH,cd,X);
        ll a_=fmpz_get_si(cA),h_=fmpz_get_si(cH),d_=fmpz_get_si(cd);
        fmpz_clear(cA);fmpz_clear(cH);fmpz_clear(cd);
        int domain_expected=(kind==1&&(b<=0||b%2==0));
        if(domain_expected){ if(st!=ADF_DOMAIN||v!=99) F("fb dom kind%d b=%lld st=%d",kind,b,st); adf_fball_clear(X); continue; }
        if(st==ADF_DOMAIN){ dom++; /* no integer point t with a+ht = 0 mod d  ->  empty in Zhat */
          if(d_>1){ ll g=gcdl(h_,d_); if(a_%g==0 && g>0 && h_>0) F("fb DOMAIN though points exist (%lld,%lld,%lld)",a_,h_,d_); if(h_==0 && a_%d_==0) F("fb DOMAIN exact integer"); } else F("fb DOMAIN for d=1");
        } else if(st==ADF_OK){ fb_ok++;
          if(d_!=1) F("fb OK with d>1 (%lld,%lld,%lld) b=%lld",a_,h_,d_,b);
          else if(h_==0){ fmpz_set_si(A,a_); int want=o_kron(A,b); if(v!=want) F("fb exact a=%lld b=%lld got %d want %d",a_,b,v,want); }
          else { ll bb=b<0?-b:b; if(b<=0) F("fb OK b<=0 with H>0"); else { ll K=bb; if(kind==2&&bb%2==0){ while(K%2==0)K/=2; K*=8; } ll L=h_/gcdl(h_,K)*K; int seen[3]={0,0,0};
              for(ll rr=0;rr<L;rr++){ if((rr-a_)%h_) continue; fmpz_set_si(r,rr); seen[o_kron(r,b)+1]=1; }
              if(seen[0]+seen[1]+seen[2]>1) F("BLOCKER fb OK but points differ kind%d A=%lld H=%lld b=%lld",kind,a_,h_,b);
              else if(v!=(seen[2]?1:seen[1]?0:-1)) F("fb wrong sign kind%d A=%lld H=%lld b=%lld got %d",kind,a_,h_,b,v); } }
        } else if(st==ADF_NOT_DETERMINED){ fb_nd++; if(v!=99) F("fb value touched");
          if(d_==1&&h_>0&&b>0){ ll K=b; if(kind==2&&b%2==0){ while(K%2==0)K/=2; K*=8; } ll L=h_/gcdl(h_,K)*K; int seen[3]={0,0,0};
              for(ll rr=0;rr<L;rr++){ if((rr-a_)%h_) continue; fmpz_set_si(r,rr); seen[o_kron(r,b)+1]=1; }
              if(seen[0]+seen[1]+seen[2]<=1) fb_nd_single++; }
          if(d_>1){ ll g=gcdl(h_,d_); if(!(h_>0&&a_%g==0)) F("fb ND but should be DOMAIN (%lld,%lld,%lld)",a_,h_,d_); } }
        else F("fb status %d",st);
        adf_fball_clear(X);
      }
    }
  }
  printf("ucoset: cases=%lld OK=%lld ND=%lld ND-but-singleton(refusals of the K|N rule)=%lld\n",uc_cases,uc_ok,uc_nd,uc_nd_single);
  printf("fball: cases=%lld OK=%lld ND=%lld DOMAIN=%lld ND-but-singleton=%lld\n",fb_cases,fb_ok,fb_nd,dom,fb_nd_single);
  printf("fails=%lld\n",fails); return 0; }
