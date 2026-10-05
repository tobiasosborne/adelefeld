#include <stdio.h>
#include <string.h>
#include "adelefeld/dump.h"
int main(void){
  adf_idele_t x,y; arb_t a; fmpq_t r; adf_ucoset_t u; size_t l; char*s; int st;
  adf_idele_init(x); adf_idele_init(y); arb_init(a); fmpq_init(r); adf_ucoset_init(u);
  arb_set_si(a,3); mag_set_ui(arb_radref(a),2); fmpq_one(r);
  st=adf_idele_set_parts(x,a,r,u); printf("set_parts(3 +/- 2): %d\n",st);
  s=adf_idele_dump_str(&l,x); printf("dump: %s\n",s);
  st=adf_idele_load_str(y,s,l,NULL,NULL); printf("load back: %d (0 expected)\n",st);
  return 0;}
