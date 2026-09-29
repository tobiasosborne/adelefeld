/* Independent enumeration of Definition 1.1 and the limit rule of Proposition 1.7.
   Build: cc -std=c11 -O2 -Iinclude lanes/s3-review/probe.c build/libadelefeld.a
          -lflint -lgmp -lm -o lanes/s3-review/probe */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <flint/fmpz.h>
#include <adelefeld/resid.h>

static long gcd(long a, long b)
{
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { long t = a % b; a = b; b = t; }
    return a;
}

static int certificate_ok(const adf_recon_cert_t k, long c, long m, long A)
{
    long rp = fmpz_get_si(k->Rp), tp = fmpz_get_si(k->Tp);
    long r = fmpz_get_si(k->R), t = fmpz_get_si(k->T);
    return k->kind == 1 && (r-c*t)%m == 0 && (rp-c*tp)%m == 0
        && labs(t*rp-tp*r) == m && 0 <= r && r <= A && A < rp
        && t != 0 && t*tp <= 0;
}

static int oracle(long c, long m, long A, long B, long limit,
                  const adf_recon_cert_t k, long *sn, long *sd, long *solcount)
{
    long n, d, found = 0, low = 0;
    long rp = fmpz_get_si(k->Rp), tp = fmpz_get_si(k->Tp);
    long r = fmpz_get_si(k->R), t = fmpz_get_si(k->T);
    long ell = limit > 0 ? limit : 0;
    if (A < 0 || B < 1) { *solcount = 0; return ADF_NO_SOLUTION; }
    for (d = 1; d <= B; ++d)
        for (n = -A; n <= A; ++n)
            if (gcd(n,d)==1 && gcd(d,m)==1 && (n-c*d)%m==0)
            {
                if (!found) { *sn = n; *sd = d; }
                ++found;
                if (A < m && 2*A*B >= m && labs(t) <= B)
                {
                    long num = n*tp-d*rp, den = r*tp-rp*t;
                    if (den == 0 || num%den != 0 || num/den == 0)
                    { fprintf(stderr,"bad coordinates\n"); exit(3); }
                    if (labs(num/den) <= ell) ++low;
                }
            }
    *solcount = found;
    if (A < m && 2*A*B >= m && labs(t) <= B && B/labs(t) > ell && low < 2)
        return ADF_NOT_DETERMINED;
    return found == 0 ? ADF_NO_SOLUTION : found == 1 ? ADF_OK : ADF_NOT_UNIQUE;
}

static void fail(long c, long m, long A, long B, long limit, const char *why, int got, int want)
{
    fprintf(stderr,"FAIL c=%ld m=%ld A=%ld B=%ld limit=%ld %s got=%d want=%d\n",
            c,m,A,B,limit,why,got,want);
    exit(1);
}

int main(void)
{
    adf_resid_t x;
    adf_recon_cert_t k;
    adf_rat_t q;
    fmpz_t cf,mf,af,bf,oc,om;
    long m,c,A,B,li, cases=0, statuses[11]={0}, same_x_two_y=0;
    const long limits[]={-7,0,1,2,5,17};
    adf_resid_init(x); adf_recon_cert_init(k); adf_rat_init(q);
    fmpz_init(cf); fmpz_init(mf); fmpz_init(af); fmpz_init(bf);
    fmpz_init(oc); fmpz_init(om);
    for (m=1;m<=18;m++) for(c=-m;c<=2*m;c++)
    for(A=-1;A<=m+2;A++) for(B=-1;B<=m+2;B++)
    {
        fmpz_set_si(cf,c); fmpz_set_si(mf,m);
        fmpz_set_si(af,A); fmpz_set_si(bf,B);
        if (adf_resid_set_fmpz2(x,cf,mf)!=ADF_OK || !adf_resid_is_canonical(x))
            fail(c,m,A,B,0,"setter",0,1);
        adf_resid_get_fmpz2(oc,om,x);
        if (fmpz_get_si(oc)!=((c%m)+m)%m || fmpz_get_si(om)!=m)
            fail(c,m,A,B,0,"getter",0,1);
        for(li=0;li<6;li++)
        {
            long lim=limits[li], sn=0,sd=0,count=0;
            int got,want, cgot;
            fmpz_set_si(fmpq_numref(q->q),-777);
            fmpz_set_si(fmpq_denref(q->q),13);
            fmpz_set_si(k->Rp,999); k->kind=1;
            got=adf_resid_reconstruct(q,k,x,af,bf,lim);
            if (A>=0 && A<m && B>=1)
            {
                if (!certificate_ok(k,c,m,A))
                    fail(c,m,A,B,lim,"certificate",0,1);
                cgot=adf_recon_cert_check(k,x,af);
                if (cgot!=1) fail(c,m,A,B,lim,"checker rejects true",cgot,1);
            }
            else if (k->kind!=0 || !fmpz_is_zero(k->Rp) || !fmpz_is_zero(k->Tp)
                     || !fmpz_is_zero(k->R) || !fmpz_is_zero(k->T))
                fail(c,m,A,B,lim,"kind zero",k->kind,0);
            want=oracle(c,m,A,B,lim,k,&sn,&sd,&count);
            if (got!=want) fail(c,m,A,B,lim,"status",got,want);
            if (got==ADF_OK)
            {
                if (count!=1 || !fmpz_equal_si(fmpq_numref(q->q),sn)
                    || !fmpz_equal_si(fmpq_denref(q->q),sd) || !adf_rat_is_canonical(q))
                    fail(c,m,A,B,lim,"fraction",0,1);
            }
            else if (!fmpz_equal_si(fmpq_numref(q->q),-777)
                     || !fmpz_equal_si(fmpq_denref(q->q),13))
                fail(c,m,A,B,lim,"q touched",0,1);
            statuses[got]++; cases++;
            if (got==ADF_NOT_UNIQUE && lim==1 && k->kind==1) same_x_two_y++;
        }
    }
    printf("cases=%ld OK=%ld NO_SOLUTION=%ld NOT_UNIQUE=%ld NOT_DETERMINED=%ld limit1_NOT_UNIQUE=%ld\n",
           cases,statuses[ADF_OK],statuses[ADF_NO_SOLUTION],
           statuses[ADF_NOT_UNIQUE],statuses[ADF_NOT_DETERMINED],same_x_two_y);
    fmpz_clear(cf); fmpz_clear(mf); fmpz_clear(af); fmpz_clear(bf);
    fmpz_clear(oc); fmpz_clear(om);
    adf_rat_clear(q); adf_recon_cert_clear(k); adf_resid_clear(x);
    return 0;
}
