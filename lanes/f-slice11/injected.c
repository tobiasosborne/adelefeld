#include <adelefeld.h>
#include <stdio.h>
int main(void)
{
    adf_idele_t x;
    adf_adele_t y,b;
    adf_sball_t s,t;
    adf_place_t w=adf_place_inf();
    int bad=0;
    adf_idele_init(x); adf_adele_init(y); adf_adele_init(b);
    adf_sball_init(s); adf_sball_init(t);
    arb_set_si(x->inf,2); arb_set_si(y->inf,123); adf_adele_set(b,y);
    for(int a=0;a<2;a++)
    {
        int st=a?adf_idele_log_abs(y,&w,x,64):adf_idele_Log(y,&w,x,64);
        bad+=st!=ADF_NOT_DETERMINED || !adf_place_is_archimedean(w) || !adf_adele_identical(y,b);
        st=a?adf_idele_log_abs_at(s,&w,x,adf_place_inf(),64):adf_idele_Log_at(s,&w,x,adf_place_inf(),64);
        bad+=st!=ADF_NOT_DETERMINED || !adf_place_is_archimedean(w) || !adf_sball_identical(s,t);
        adf_place_t p;
        if (adf_place_prime(&p,3)!=ADF_OK) return 2;
        st=a?adf_idele_log_abs_refine(y,&w,x,&p,1,5,64):adf_idele_Log_refine(y,&w,x,&p,1,5,64);
        bad+=st!=ADF_NOT_DETERMINED || !adf_place_is_archimedean(w) || !adf_adele_identical(y,b);
    }
    adf_idele_clear(x); adf_adele_clear(y); adf_adele_clear(b); adf_sball_clear(s); adf_sball_clear(t);
    printf("injected real evaluator: 6 calls, %d failures\n",bad);
    return bad!=0;
}
