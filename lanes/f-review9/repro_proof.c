#include <adelefeld.h>
#include <stdio.h>
int main(void) {
    adf_rat_t q,r;fmpq_t power,ab;
    adf_rat_init(q);adf_rat_init(r);fmpq_init(power);fmpq_init(ab);adf_rat_set_si(q,4);
    int st=adf_rat_root(r,NULL,q,2,-1);fmpq_pow_si(power,r->q,2);
    printf("q=4 n=2 r=");fmpq_print(r->q);printf(" r^n=");fmpq_print(power);
    fmpq_abs(ab,r->q);
    printf(" r>=2=%d abs(r)>=2=%d status=%s\n",fmpq_cmp_si(r->q,2)>=0,
           fmpq_cmp_si(ab,2)>=0,adf_status_str(st));
    int bad=st!=ADF_OK || !fmpq_equal(power,q->q);
    adf_rat_clear(q);adf_rat_clear(r);fmpq_clear(power);fmpq_clear(ab);flint_cleanup();return bad;
}
