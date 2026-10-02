#include <adelefeld.h>
#include <limits.h>
#include <stdio.h>

static unsigned checks, failures, decomp_zero;
static void check(int yes, const char *name, int i, int j, int st)
{
    checks++;
    if (!yes) { failures++; printf("FAIL %s i=%d j=%d status=%d\n", name, i, j, st); }
}

int main(void)
{
    const slong exps[] = {LONG_MIN, LONG_MIN+1, LONG_MAX-1, LONG_MAX,
                         -ADF_LBALL_EXP_MAX-1, ADF_LBALL_EXP_MAX+1};
    const slong powers[] = {LONG_MIN, LONG_MIN+1, -2, -1, 0, 1, 2, LONG_MAX};
    adf_lball_t huge[18], small[4], out, before, w;
    adf_rat_t rat;
    fmpz_t z;
    slong m, pre;
    ulong idx;
    int st, inf;
    adf_lball_init(out); adf_lball_init(before); adf_lball_init(w);
    adf_rat_init(rat); fmpz_init(z);
    for (int i=0; i<18; i++) {
        adf_lball_init(huge[i]); huge[i]->p=2;
        if (i<6) { huge[i]->exact=0; huge[i]->N=exps[i]; }
        else if (i<12) { huge[i]->exact=1; huge[i]->v=exps[i-6]; fmpq_one(huge[i]->u); }
        else {
            huge[i]->exact=0; fmpq_one(huge[i]->u);
            huge[i]->v=exps[i-12];
            if (huge[i]->v==LONG_MAX) huge[i]->v--;
            huge[i]->N=huge[i]->v+1;
        }
        check(adf_lball_is_canonical(huge[i]), "canonical", i, 0, 0);
    }
    for (int j=0; j<4; j++) {
        adf_lball_init(small[j]); small[j]->p=2; small[j]->exact=(j<2);
        small[j]->N=(j<2 ? 0 : 4); if (j & 1) fmpq_one(small[j]->u);
    }
    out->p=before->p=3; out->exact=before->exact=1;
    fmpq_set_si(out->u, 2, 1); fmpq_set_si(before->u, 2, 1);
    for (int i=0; i<18; i++) {
        st=adf_lball_neg(out,huge[i]);
        check(st==ADF_LIMIT && adf_lball_identical(out,before),"neg",i,0,st);
        st=adf_lball_inv(out,huge[i]);
        check(st==ADF_LIMIT && adf_lball_identical(out,before),"inv",i,0,st);
        for (int k=0;k<8;k++) {
            st=adf_lball_pow_si(out,huge[i],powers[k]);
            check(st==ADF_LIMIT && adf_lball_identical(out,before),"pow",i,k,st);
        }
        m=71; idx=73;
        st=adf_lball_decompose(&m,out,huge[i]);
        check(st==(i<6 ? ADF_NOT_DETERMINED:ADF_LIMIT) && m==71 &&
              adf_lball_identical(out,before),"decompose",i,0,st);
        if (i<6 && st==ADF_NOT_DETERMINED) decomp_zero++;
        st=adf_lball_decompose_teich(&m,w,&idx,out,huge[i],LONG_MAX);
        check(st==(i<6 ? ADF_NOT_DETERMINED:ADF_LIMIT) && m==71 && idx==73 &&
              adf_lball_identical(out,before),"teich",i,0,st);
        st=adf_lball_frac(rat,huge[i]); check(st==ADF_LIMIT,"frac",i,0,st);
        for (int k=0;k<6;k++) {
            st=adf_lball_exp(out,huge[i],exps[k]);
            check(st==ADF_LIMIT && adf_lball_identical(out,before),"exp",i,k,st);
            st=adf_lball_log(out,huge[i],exps[k]);
            check(st==ADF_LIMIT && adf_lball_identical(out,before),"log",i,k,st);
            st=adf_lball_Log(out,huge[i],exps[k]);
            check(st==ADF_LIMIT && adf_lball_identical(out,before),"Log",i,k,st);
        }
        (void)adf_lball_valuation(&m,&inf,huge[i]);
        (void)adf_lball_abs(rat,huge[i]); (void)adf_lball_get_center(rat,huge[i]);
        (void)adf_lball_get_prec(&pre,huge[i]); (void)adf_lball_unit_mod(z,huge[i],1);
        for(int j=0;j<4;j++) for(int order=0;order<2;order++) {
            adf_lball_srcptr a=order ? small[j]:huge[i], b=order ? huge[i]:small[j];
            st=adf_lball_add(out,a,b);
            check(st==ADF_LIMIT && adf_lball_identical(out,before),"add",i,j,st);
            st=adf_lball_sub(out,a,b);
            check(st==ADF_LIMIT && adf_lball_identical(out,before),"sub",i,j,st);
            st=adf_lball_mul(out,a,b);
            check(st==ADF_LIMIT && adf_lball_identical(out,before),"mul",i,j,st);
            st=adf_lball_div(out,a,b);
            int want=order==0 && j==0 ? ADF_NOT_UNIT : order==0 && j==2 ? ADF_UNIT_NOT_CERTIFIED : ADF_LIMIT;
            check(st==want && adf_lball_identical(out,before),"div",i,j,st);
            (void)adf_lball_equal_set(a,b); (void)adf_lball_contains(a,b); (void)adf_lball_overlaps(a,b);
        }
    }
    for(int i=0;i<18;i++) adf_lball_clear(huge[i]);
    for(int j=0;j<4;j++) adf_lball_clear(small[j]);
    adf_lball_clear(out); adf_lball_clear(before); adf_lball_clear(w);
    adf_rat_clear(rat); fmpz_clear(z); flint_cleanup();
    printf("huge_inputs=18 small_inputs=4 checks=%u failures=%u zero_decompose_not_determined=%u\n",
           checks,failures,decomp_zero);
    return failures!=0;
}
