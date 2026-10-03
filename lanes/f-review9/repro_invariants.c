#include <adelefeld.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv) {
    adf_adele_t x,y; adf_idele_t i,j;
    adf_adele_init(x);adf_adele_init(y);adf_idele_init(i);adf_idele_init(j);
    arb_indeterminate(x->inf);arb_indeterminate(i->inf);
    int st;
    if(argc>1 && strcmp(argv[1],"control")==0) { adf_adele_set(y,x); st=ADF_OK; }
    else if(argc>1 && strcmp(argv[1],"idele")==0) st=adf_idele_root(j,NULL,i,2,1,64);
    else if(argc>1 && strcmp(argv[1],"series")==0) st=adf_adele_exp(y,NULL,x,64);
    else st=adf_adele_root(y,NULL,x,2,1,64);
    printf("returned=%s canonical_adele_input=%d canonical_idele_input=%d\n",
           adf_status_str(st),adf_adele_is_canonical(x),adf_idele_is_canonical(i));
    adf_adele_clear(x);adf_adele_clear(y);adf_idele_clear(i);adf_idele_clear(j);flint_cleanup();
    return 0;
}
