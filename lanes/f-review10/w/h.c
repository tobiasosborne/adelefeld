#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <adelefeld.h>
#include <adelefeld/text.h>
/* stdin lines: "AT p N prec idele" / "REF N prec p1,p2,.. idele" */
static adf_place_t ps[70000];
int main(void){
  static char line[1<<20];
  while(fgets(line,sizeof line,stdin)){
    size_t L=strlen(line); while(L&&(line[L-1]=='\n')) line[--L]=0;
    char cmd[32]; int off=0; sscanf(line,"%31s%n",cmd,&off);
    adf_idele_t x; adf_idele_init(x);
    if(!strcmp(cmd,"AT")){
      unsigned long p; long N,prec; int o2;
      sscanf(line+off,"%lu %ld %ld%n",&p,&N,&prec,&o2);
      const char*s=line+off+o2;
      int st=adf_idele_set_str(x,s,strlen(s),200,NULL);
      if(st){printf("PARSE %d\n",st);adf_idele_clear(x);continue;}
      adf_place_t v; adf_place_prime(&v,p);
      adf_sball_t y; adf_sball_init(y); adf_place_t w=adf_place_inf();
      st=adf_idele_Log_at(y,&w,x,v,N);
      printf("st=%d",st); if(st==0) printf(" scanon=%d",adf_sball_is_canonical(y));
      if(st==0){adf_lball_t l; adf_lball_init(l); adf_sball_get_lball(l,y,v);
        adf_rat_t c; adf_rat_init(c); int s2=adf_lball_get_center(c,l); char*cs=fmpq_get_str(NULL,10,c->q);
        long n=-999; if(!l->exact) adf_lball_get_prec(&n,l);
        printf(" c=%s N=%ld exact=%d (%d)",cs,n,l->exact,s2); flint_free(cs); adf_rat_clear(c); adf_lball_clear(l);}
      printf("\n"); adf_sball_clear(y);
    } else if(!strcmp(cmd,"REF")){
      long N,prec; static char pl[1<<19]; int o2;
      sscanf(line+off,"%ld %ld %524287s%n",&N,&prec,pl,&o2);
      const char*s=line+off+o2;
      int st=adf_idele_set_str(x,s,strlen(s),200,NULL);
      if(st){printf("PARSE %d\n",st);adf_idele_clear(x);continue;}
      long n=0; int bad=0;
      if(strcmp(pl,"-")){ char*t=strtok(pl,","); while(t){ unsigned long p=strtoul(t,0,10); if(p==0) ps[n++]=adf_place_inf(); else {int r=adf_place_prime(&ps[n],p); if(r){printf("BADPRIME %d\n",r); bad=1; break;} n++;} t=strtok(NULL,","); } }
      if(bad){adf_idele_clear(x);continue;}
      adf_adele_t y; adf_adele_init(y); adf_place_t w=adf_place_inf();
      st=adf_idele_Log_refine(y,&w,x,ps,n,N,prec);
      printf("st=%d",st);
      if(st==0){char*a=fmpz_get_str(NULL,10,y->fin.A),*h=fmpz_get_str(NULL,10,y->fin.H); printf(" A=%s H=%s canon=%d",a,h,adf_adele_is_canonical(y)); flint_free(a);flint_free(h);}
      printf("\n"); adf_adele_clear(y);
    }
    adf_idele_clear(x);
  }
  return 0;
}
