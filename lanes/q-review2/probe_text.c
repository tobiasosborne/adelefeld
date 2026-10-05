#include <adelefeld.h>
#include <stdio.h>
static const char * sn(int s){switch(s){case ADF_OK:return "OK";case ADF_PARSE:return "PARSE";
 case ADF_LIMIT:return "LIMIT";case ADF_DOMAIN:return "DOMAIN";case ADF_UNSUPPORTED:return "UNSUP";
 case ADF_NOT_DETERMINED:return "ND";default:return "?";}}
static void t1(const char * s){ adf_adele_t a; adf_qclass_t q; int st;
  adf_adele_init(a); adf_qclass_init(q);
  st = adf_adele_set_str(a, s, strlen(s), 64, NULL);
  printf("adele  %-30s -> %s\n", s, sn(st));
  st = adf_qclass_set_str(q, s, strlen(s), 64, NULL);
  printf("qclass %-30s -> %s\n", s, sn(st));
  adf_adele_clear(a); adf_qclass_clear(q);
}
int main(void){
  t1("(1/0 ; 0)");
  t1("(1/0 ; 0) + Q");
  t1("(0.5 ; 1/0) + Q");
  t1("(0.5 ; 1 mod 0) + Q");
  t1("(0.5 ; 1) + Q");
  t1("(0.5 ; 1 mod 2) + Q");
  t1("(0.5 ; 2 mod 2) + Q");
  return 0;
}
