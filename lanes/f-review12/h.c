#include <stdlib.h>
#include <adelefeld.h>
#include <stdio.h>
#include <string.h>
static void rd(fmpz_t z){ char buf[20000]; if(scanf("%19999s",buf)!=1) exit(0); fmpz_set_str(z,buf,10);}
int main(void){
  char cmd[64];
  fmpz_t a,b,c,d,e; fmpz_init(a);fmpz_init(b);fmpz_init(c);fmpz_init(d);fmpz_init(e);
  while(scanf("%63s",cmd)==1){
    if(!strcmp(cmd,"binom")||!strcmp(cmd,"tight")){
      ulong k; int st; adf_fball_t x,y; adf_fball_init(x);adf_fball_init(y);
      rd(a);rd(b);rd(d); { char buf[64]; if(scanf("%63s",buf)!=1) return 1; k=strtoul(buf,0,10);}
      if(adf_fball_set_fmpz3(x,a,b,d)!=ADF_OK){printf("BADIN\n");continue;}
      st = cmd[0]=='b' ? adf_fball_binom(y,NULL,x,k):adf_fball_binom_tight(y,NULL,x,k);
      printf("%d",st);
      if(st==ADF_OK){ adf_fball_get_fmpz3(a,b,d,y); printf(" "); fmpz_print(a);printf(" ");fmpz_print(b);printf(" ");fmpz_print(d);
        printf(" canon=%d",adf_fball_is_canonical(y)); }
      printf("\n"); adf_fball_clear(x);adf_fball_clear(y);
    } else if(!strncmp(cmd,"pp",2)){
      int mode=cmd[2]-'0'; adf_ucoset_t u,r; adf_fball_t x; int st;
      adf_ucoset_init(u);adf_ucoset_init(r);adf_fball_init(x);
      rd(a);rd(b); rd(c);rd(d); /* c N e M */
      if(adf_ucoset_set_fmpz2(u,a,b)!=ADF_OK){printf("BADIN\n");continue;}
      fmpz_one(e);
      if(adf_fball_set_fmpz3(x,c,d,e)!=ADF_OK){printf("BADIN\n");continue;}
      st= mode==0?adf_ucoset_profpow(r,NULL,u,x): mode==1?adf_ucoset_profpow_coarse(r,NULL,u,x):adf_ucoset_profpow_fine(r,NULL,u,x);
      printf("%d",st);
      if(st==ADF_OK){ printf(" "); fmpz_print(r->c);printf(" ");fmpz_print(r->N);printf(" canon=%d norm=%d",adf_ucoset_is_canonical(r),adf_ucoset_is_normal(r));}
      printf("\n"); adf_ucoset_clear(u);adf_ucoset_clear(r);adf_fball_clear(x);
    } else if(!strcmp(cmd,"cy")||!strcmp(cmd,"cyi")){
      adf_ucoset_t u; adf_idclass_t k; arb_t t; fmpz_t j,n; int st;
      adf_ucoset_init(u); adf_idclass_init(k); arb_init(t); arb_one(t); fmpz_init(j);fmpz_init(n);
      rd(a);rd(b);rd(n);
      if(adf_ucoset_set_fmpz2(u,a,b)!=ADF_OK){printf("BADIN\n");continue;}
      adf_idclass_set_parts(k,t,u); fmpz_set_si(j,-777);
      st= cmd[2]=='i' ? adf_idclass_cyclo_exp_uinv(j,NULL,k,n):adf_idclass_cyclo_exp_u(j,NULL,k,n);
      printf("%d ",st); fmpz_print(j); printf("\n");
      adf_ucoset_clear(u);adf_idclass_clear(k);arb_clear(t);fmpz_clear(j);fmpz_clear(n);
    }
  }
  return 0;}
