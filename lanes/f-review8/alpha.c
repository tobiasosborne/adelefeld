#include <adelefeld.h>
#include <stdio.h>
static void exact(adf_lball_t x,ulong p,slong a,ulong b)
{ x->p=p; fmpq_set_si(x->u,a,b); x->v=0; x->N=0; x->exact=1; }
int main(void)
{
    const struct { ulong p; slong a; ulong b; slong w,alpha; } rows[]={
        {2,-1,1,-1,-1},{2,1,1,1,-1},{2,3,1,-1,2},{2,5,1,1,2},{2,7,1,-1,3},
        {2,1,3,-1,2},{3,4,1,1,1},{3,1,4,1,1},{5,26,1,1,2},{7,50,1,1,2}};
    adf_lball_t x,w,d,l; int failures=0;
    adf_lball_init(x); adf_lball_init(w); adf_lball_init(d); adf_lball_init(l);
    for(unsigned i=0;i<sizeof(rows)/sizeof(rows[0]);i++)
    {
        exact(x,rows[i].p,rows[i].a,rows[i].b); exact(w,rows[i].p,rows[i].w,1);
        int st=adf_lball_sub(d,x,w),ls=adf_lball_Log(l,x,12);
        int dz=fmpq_is_zero(d->u),lz=fmpq_is_zero(l->u);
        int pass=st==ADF_OK && ls==ADF_OK && (rows[i].alpha<0 ? dz && lz && l->exact :
                  !dz && !lz && d->v==rows[i].alpha && l->v==rows[i].alpha && l->N==12);
        failures+=!pass;
        printf("p=%lu u=%ld/%lu w=%ld expected_alpha=%ld sub_v=%ld log_v=%ld exact_log=%d pass=%d\n",
               rows[i].p,rows[i].a,rows[i].b,rows[i].w,rows[i].alpha,d->v,l->v,l->exact,pass);
    }
    exact(x,2,1,1); x->exact=0; x->N=1;
    int st=adf_lball_Log(l,x,12);
    int pass=st==ADF_OK && fmpq_is_zero(l->u) && !l->exact && l->N==2;
    failures+=!pass;
    printf("A=1 path: Log(1+2 Z_2)=4 Z_2 pass=%d; comparison_precision=2^2\n",pass);
    printf("P6 checks=11 failures=%d nonzero_Log_precision=p^12 exact_subtractions=10\n",failures);
    adf_lball_clear(x); adf_lball_clear(w); adf_lball_clear(d); adf_lball_clear(l);
    return failures ? 1 : 0;
}
