/* own oracle: from definitions only (no FLINT symbol functions, no repository checks) */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef long long ll; typedef unsigned long long ull;
static int PERTURB=0; /* oracle fault injection */
static ull mulm(ull a,ull b,ull m){return (ull)((unsigned __int128)a*b%m);}
static ull powm(ull a,ull e,ull m){ull r=1%m;a%=m;while(e){if(e&1)r=mulm(r,a,m);a=mulm(a,a,m);e>>=1;}return r;}
/* Legendre, p odd prime: count squares mod p for small p, Euler's criterion otherwise */
static int o_leg(const fmpz_t a,ull p){
  ull r=fmpz_fdiv_ui(a,p); if(r==0) return 0;
  if(p<=3000){ for(ull x=1;x<p;x++) if(x*x%p==r) return PERTURB==1?-1:1; return PERTURB==1?1:-1;}
  return powm(r,(p-1)/2,p)==1?1:-1;
}
/* Kronecker for b fitting ll, by Definition 1 of catalogue.md (trial division) */
static int o_kron(const fmpz_t a,ll b){
  int s=1; if(b==0){ return (fmpz_cmp_si(a,1)==0||fmpz_cmp_si(a,-1)==0)?1:0; }
  if(b<0){ if(fmpz_sgn(a)<0) s=-s; b=-b; }
  ull B=b; ull q=2;
  while(B>1){
    if(q*q>B) q=B;
    int h=0; while(B%q==0){B/=q;h++;}
    if(h){
      int f;
      if(q==2){ if(fmpz_is_even(a)) f=0; else { ull a8=fmpz_fdiv_ui(a,8); f=(((a8*a8-1)/8)%2)?-1:1; } }
      else f=o_leg(a,q);
      if(f==0) return 0; if(f<0&&(h&1)) s=-s;
    }
    if(B==1)break;
    q=(q==2)?3:q+2;
  }
  return s;
}
/* Hilbert: primitive solutions of a x^2+b y^2=z^2 mod p^k; square classes reduced by brute-force squares */
static ull ipow(ull p,int k){ull r=1;while(k--)r*=p;return r;}
static int o_hil_cls(ull p,int k,int al,ull u,int be,ull w){
  ull M=ipow(p,k); ull a=u*(al?p:1)%M, b=w*(be?p:1)%M;
  char *sq=calloc(M,1),*usq=calloc(M,1);
  for(ull z=0;z<M;z++){ sq[z*z%M]=1; if(z%p) usq[z*z%M]=1; }
  int found=0;
  for(ull x=0;x<M&&!found;x++)for(ull y=0;y<M;y++){
    ull v=(mulm(a,x*x%M,M)+mulm(b,y*y%M,M))%M;
    if((x%p||y%p)? sq[v] : usq[v]){found=1;break;}
  }
  free(sq);free(usq); if(PERTURB==2) found=!found; return found?1:-1;
}
static int KOVR=0; /* override of k for the cross-check */
static ull sqrep(ull p,int k,ull u){ /* least r in 1..p^k with r = u s^2 mod p^k, s a unit */
  ull M=ipow(p,k); ull best=M;
  for(ull s=1;s<M;s++) if(s%p){ ull r=mulm(u%M,s*s%M,M); if(r==0)r=M; if(r<best)best=r; }
  return best;
}
typedef struct{ull p;int al,be;ull u,w;int val;}ent; static ent *memo; static int nmemo=0,capm=0;
static int o_k(ull p){ return KOVR?KOVR:((p==2)?5:3); }
static ull *reptab[32]; static int repk[32]; static ull repp[32]; static int nrep=0;
static ull sqrep_c(ull p,int k,ull u){
  int i; for(i=0;i<nrep;i++) if(repp[i]==p&&repk[i]==k) break;
  ull M=ipow(p,k);
  if(i==nrep){ reptab[i]=malloc(M*sizeof(ull)); repp[i]=p;repk[i]=k; nrep++; for(ull x=0;x<M;x++) reptab[i][x]=0; }
  u%=M; if(!reptab[i][u]) reptab[i][u]=sqrep(p,k,u);
  return reptab[i][u];
}
static int o_hil_parts(ull p,int al,ull u,int be,ull w){
  int k=o_k(p); ull ru=sqrep_c(p,k,u),rw=sqrep_c(p,k,w);
  for(int i=0;i<nmemo;i++) if(memo[i].p==p&&memo[i].al==al&&memo[i].be==be&&memo[i].u==ru&&memo[i].w==rw) return memo[i].val;
  int v=o_hil_cls(p,k,al,ru,be,rw);
  if(nmemo==capm){capm=capm?2*capm:64;memo=realloc(memo,capm*sizeof(ent));}
  memo[nmemo++]=(ent){p,al,be,ru,rw,v}; return v;
}
/* nonzero n/d: parity of v_p, and unit of n d / p^(v(n)+v(d)) mod p^k (n/d = n d / d^2 same class) */
static void o_split(ull p,const fmpz_t n,const fmpz_t d,int*par,ull*unit){
  ull M=ipow(p,o_k(p));
  fmpz_t x,y; fmpz_init(x);fmpz_init(y); fmpz_set(x,n);fmpz_set(y,d);
  int e=0; while(fmpz_fdiv_ui(x,p)==0){fmpz_divexact_ui(x,x,p);e++;} while(fmpz_fdiv_ui(y,p)==0){fmpz_divexact_ui(y,y,p);e++;}
  *par=e&1; *unit=mulm(fmpz_fdiv_ui(x,M),fmpz_fdiv_ui(y,M),M);
  fmpz_clear(x);fmpz_clear(y);
}
static int o_hil(const fmpz_t an,const fmpz_t ad,const fmpz_t bn,const fmpz_t bd,ull p){
  if(p==0){ int sa=fmpz_sgn(an)*fmpz_sgn(ad), sb=fmpz_sgn(bn)*fmpz_sgn(bd); int r=(sa<0&&sb<0)?-1:1; return PERTURB==3?-r:r; }
  int al,be; ull u,w; o_split(p,an,ad,&al,&u); o_split(p,bn,bd,&be,&w);
  return o_hil_parts(p,al,u,be,w);
}
static int o_hil_si(ll a,ll b,ull p){ fmpz_t A,B,o; fmpz_init_set_si(A,a);fmpz_init_set_si(B,b);fmpz_init_set_ui(o,1); int r=o_hil(A,o,B,o,p); fmpz_clear(A);fmpz_clear(B);fmpz_clear(o); return r; }
