/* Edge and checker probes, separate from the author's test suite. */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <flint/fmpz.h>
#include <adelefeld/resid.h>

static unsigned long state=0x674b355dUL;
static long pick(long n) { state=state*1664525UL+1013904223UL; return (long)(state%n); }
static void need(int b, const char *s) { if (!b) { fprintf(stderr,"FAIL %s\n",s); exit(1); } }
static int own_check(const adf_recon_cert_t k,long c,long m,long a)
{
    long rp=fmpz_get_si(k->Rp),tp=fmpz_get_si(k->Tp);
    long r=fmpz_get_si(k->R),t=fmpz_get_si(k->T);
    return k->kind==1 && m>=1 && (r-c*t)%m==0 && (rp-c*tp)%m==0
        && labs(t*rp-tp*r)==m && r>=0 && r<=a && a<rp && t!=0 && t*tp<=0;
}
int main(void)
{
    adf_resid_t x; adf_recon_cert_t k; adf_rat_t q;
    fmpz_t c,m,a,b,zero,gotc,gotm;
    long i, cert_checks=0, huge_calls=0, aliases=0, noncanonical=0;
    adf_resid_init(x); adf_recon_cert_init(k); adf_rat_init(q);
    fmpz_init(c); fmpz_init(m); fmpz_init(a); fmpz_init(b);
    fmpz_init(zero); fmpz_init(gotc); fmpz_init(gotm);
    for(i=0;i<100000;i++)
    {
        long cv=pick(101)-50, mv=pick(31), av=pick(31)-1;
        fmpz_set_si(x->c,cv); fmpz_set_si(x->m,mv);
        fmpz_set_si(a,av);
        fmpz_set_si(k->Rp,pick(61)-30); fmpz_set_si(k->Tp,pick(61)-30);
        fmpz_set_si(k->R,pick(61)-30); fmpz_set_si(k->T,pick(61)-30);
        k->kind=(int)pick(4)-1;
        need(adf_recon_cert_check(k,x,a)==own_check(k,cv,mv,av),"checker random");
        cert_checks++;
    }
    fmpz_set_si(x->c,1); fmpz_set_si(x->m,7);
    fmpz_set_si(k->Rp,7); fmpz_set_si(k->Tp,0);
    fmpz_set_si(k->R,1); fmpz_set_si(k->T,1);
    k->kind=1; fmpz_set_si(a,1);
    need(adf_recon_cert_check(k,x,a)==1,"true certificate"); cert_checks++;
    for(i=0;i<4;i++)
    {
        fmpz_t *field = i==0 ? &k->Rp : i==1 ? &k->Tp : i==2 ? &k->R : &k->T;
        fmpz_add_ui(*field,*field,1);
        need(adf_recon_cert_check(k,x,a)==0,"mutated certificate"); cert_checks++;
        fmpz_sub_ui(*field,*field,1);
    }
    fmpz_set_si(c,-7); fmpz_set_si(m,6);
    need(adf_resid_set_fmpz2(x,c,m)==ADF_OK,"set negative c");
    need(fmpz_equal_si(x->c,5) && fmpz_equal_si(x->m,6),"reduce negative c");
    need(adf_resid_set_fmpz2(x,c,zero)==ADF_DOMAIN,"reject m zero");
    need(fmpz_equal_si(x->c,5) && fmpz_equal_si(x->m,6),"untouched m zero");
    need(adf_resid_set_fmpz2(x,x->m,x->c)==ADF_OK,"setter swapped members");
    need(fmpz_equal_si(x->c,1) && fmpz_equal_si(x->m,5),"setter alias values"); aliases++;
    adf_resid_get_fmpz2(x->m,x->c,x);
    need(fmpz_equal_si(x->m,1) && fmpz_equal_si(x->c,5),"getter swapped members"); aliases++;
    need(!adf_resid_is_canonical(x),"getter changed member");
    adf_resid_get_fmpz2(gotc,gotm,x);
    need(fmpz_equal_si(gotc,5) && fmpz_equal_si(gotm,1),"getter independent"); aliases++;
    fmpz_set_si(x->c,-1); fmpz_set_si(x->m,7);
    need(!adf_resid_is_canonical(x),"negative c canonical"); noncanonical++;
    fmpz_set_si(x->c,7); need(!adf_resid_is_canonical(x),"c=m canonical"); noncanonical++;
    fmpz_set_si(x->c,0); fmpz_zero(x->m);
    need(!adf_resid_is_canonical(x),"m=0 canonical"); noncanonical++;
    /* A manual unreduced c still denotes the same congruence when m is positive. */
    fmpz_set_si(x->c,-4); fmpz_set_si(x->m,7);
    fmpz_set_si(a,1); fmpz_set_si(b,2);
    need(adf_resid_reconstruct(q,k,x,a,b,0)==ADF_OK,"unreduced c result");
    need(fmpz_equal_si(fmpq_numref(q->q),-1)
        && fmpz_equal_si(fmpq_denref(q->q),2),"unreduced c fraction");
    /* B / |T| exceeds the word range. Only the admitted number of rounds may run. */
    fmpz_set_si(x->c,1); fmpz_set_si(x->m,7);
    fmpz_set_si(a,3); fmpz_one(b); fmpz_mul_2exp(b,b,200);
    need(adf_resid_reconstruct(q,k,x,a,b,LONG_MAX)==ADF_NOT_UNIQUE,"huge X two solutions"); huge_calls++;
    fmpz_one(x->m); fmpz_mul_2exp(x->m,x->m,130); fmpz_add_ui(x->m,x->m,13);
    fmpz_set_si(a,1);
    need(adf_resid_reconstruct(q,k,x,a,b,-7)==ADF_NOT_DETERMINED,"huge X negative limit"); huge_calls++;
    need(adf_resid_reconstruct(q,k,x,a,b,3)==ADF_NOT_DETERMINED,"huge X short limit"); huge_calls++;
    fmpz_set_si(x->c,3); fmpz_set_si(x->m,7);
    need(adf_resid_reconstruct(q,k,x,a,b,0)==ADF_NOT_DETERMINED,"cut sign of T"); huge_calls++;
    fmpz_set_si(b,2);
    need(adf_resid_reconstruct(q,k,x,a,b,0)==ADF_OK,"negative T fast path"); huge_calls++;
    need(fmpz_equal_si(fmpq_numref(q->q),-1)
        && fmpz_equal_si(fmpq_denref(q->q),2),"negative T fraction");
    printf("cert_checks=%ld huge_calls=%ld aliases=%ld noncanonical=%ld\n",
           cert_checks,huge_calls,aliases,noncanonical);
    /* The aliasing test's assertions accept a false NO_SOLUTION for this concrete case. */
    fmpz_set_si(x->c,3); fmpz_set_si(x->m,100);
    fmpz_set_si(a,3); fmpz_set_si(b,3);
    need(adf_resid_reconstruct(q,k,x,a,b,0)==ADF_OK,"test gap real status");
    need(fmpz_equal_si(fmpq_numref(q->q),3)
        && fmpz_equal_si(fmpq_denref(q->q),1),"test gap real fraction");
    need((ADF_NO_SOLUTION==ADF_OK || ADF_NO_SOLUTION==ADF_NO_SOLUTION)
        && ADF_NO_SOLUTION==ADF_NO_SOLUTION,"test gap expression");
    printf("test_gap: c=3 m=100 A=3 B=3, actual OK 3/1; false NO_SOLUTION passes both assertions\n");
    fmpz_clear(c); fmpz_clear(m); fmpz_clear(a); fmpz_clear(b);
    fmpz_clear(zero); fmpz_clear(gotc); fmpz_clear(gotm);
    adf_rat_clear(q); adf_recon_cert_clear(k); adf_resid_clear(x);
    return 0;
}
