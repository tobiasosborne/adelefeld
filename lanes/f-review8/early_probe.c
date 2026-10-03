#include <adelefeld.h>
#include <stdio.h>
extern unsigned long review_enum_calls;
int main(void)
{
    adf_lball_t x;
    adf_lball_struct out[2];
    ulong ids[2]={99,99}; slong len=-1;
    adf_lball_init(x); for(int i=0;i<2;i++) adf_lball_init(out+i);
    x->p=3; fmpq_set_si(x->u,-2,1); x->v=0; x->N=0; x->exact=1;
    int st=adf_lball_roots(out,ids,&len,2,x,2,ADF_LBALL_EXP_MAX+1);
    printf("p=3 x=-2 exact n=2 N=2^60+1 status=%s len=%ld identifiers_calls=%lu\n",
           adf_status_str(st),len,review_enum_calls);
    for(int i=0;i<2;i++) adf_lball_clear(out+i); adf_lball_clear(x);
    return st!=ADF_LIMIT || len!=-1;
}
