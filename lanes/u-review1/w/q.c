#include <stdio.h>
#include <string.h>
#include "adelefeld/dump.h"
int main(int argc,char**argv){
  adf_idele_t x; adf_idele_init(x);
  const char*s=argv[1];
  int st=adf_idele_load_str(x,s,strlen(s),NULL,NULL);
  printf("%d\n",st);
  if(!st){size_t l;char*d=adf_idele_dump_str(&l,x);puts(d);}
  return 0;}
