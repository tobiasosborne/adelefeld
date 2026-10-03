#include <adelefeld.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    adf_idele_t x,y; adf_idele_init(x);adf_idele_init(y);
    const char *text="(1 ; 1 * [1 mod 8])";
    int st=adf_idele_set_str(x,text,strlen(text),64,NULL);
    if(st!=ADF_OK)return 2;
    st=adf_idele_root(y,NULL,x,4,1,64);
    printf("degree=4 input_unit_exact=%d returned=%s output_unit_exact=%d canonical_output=%d\n",
           adf_ucoset_is_exact(&x->u),adf_status_str(st),adf_ucoset_is_exact(&y->u),adf_idele_is_canonical(y));
    adf_idele_clear(x);adf_idele_clear(y);flint_cleanup();return st==ADF_NOT_DETERMINED?0:1;
}
