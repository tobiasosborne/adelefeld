/* Exact oracle: proto/lroot_checks.py; comparison precision H=r+1 in normalized units.
   Any different status, count, seed, exponent, centre, alias or failed transaction fails a case. */
#include <limits.h>
#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include "support/jsonl.h"
#include "test_runner.h"

static const jsonl_value *field(const jsonl_value *r, const char *key)
{
    const jsonl_value *v = NULL;
    jsonl_error_t e;
    ADF_CHECK_MSG(jsonl_field(r, key, &v, &e), "%s", jsonl_error_message(&e));
    return v;
}
static slong num(const jsonl_value *r, const char *key)
{
    const char *s = jsonl_int_text(field(r, key), NULL);
    ADF_CHECK(s != NULL);
    return s ? strtol(s, NULL, 10) : 0;
}
static void raw(adf_lball_t x, ulong p, slong a, slong m, slong M, int exact)
{
    x->p=p; x->v=a ? m : 0; x->N=exact ? 0 : M; x->exact=exact; fmpq_set_si(x->u,a,1);
    ADF_CHECK(adf_lball_is_canonical(x));
}
static ulong arr(const jsonl_value *r, const char *key, size_t j)
{
    const char *s = jsonl_int_text(jsonl_at(field(r,key), j, NULL), NULL);
    ADF_CHECK(s != NULL);
    return s ? strtoul(s,NULL,10) : 0;
}

ADF_TEST(enumeration_all_rows)
{
    jsonl_file *f=NULL;
    jsonl_error_t err;
    adf_lball_t x,y,z,saved,want;
    adf_lball_struct roots[12];
    ulong ids[12];
    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/f-slice8/balls.jsonl",&f,&err),
                  "%s",jsonl_error_message(&err));
    if (!f) return;
    ADF_CHECK(jsonl_count(f)==8110);
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z);
    adf_lball_init(saved); adf_lball_init(want);
    raw(saved,11,17,0,0,1);
    for (int i=0;i<12;i++) adf_lball_init(roots+i);
    for (size_t k=0;k<jsonl_count(f);k++)
    {
        const jsonl_value *r=jsonl_record(f,k);
        ulong p=(ulong)num(r,"p"), n=(ulong)num(r,"n"), count=99;
        slong len=-1;
        int st=(int)num(r,"s");
        size_t expected=jsonl_size(field(r,"ids"));
        raw(x,p,num(r,"a"),num(r,"m"),num(r,"M"),0);
        for (int i=0;i<12;i++) { adf_lball_set(roots+i,saved); ids[i]=99; }
        ADF_CHECK(adf_lball_root_count(&count,x,n)==st);
        ADF_CHECK(adf_lball_roots(roots,ids,&len,12,x,n,30)==st);
        if (st)
        {
            ADF_CHECK(count==99 && len==-1);
            for (int i=0;i<12;i++) ADF_CHECK(ids[i]==99 && adf_lball_identical(roots+i,saved));
            adf_lball_set(y,saved); adf_lball_set(z,x);
            ADF_CHECK(adf_lball_root_seed(y,x,n,1,30)==st && adf_lball_identical(y,saved));
            ADF_CHECK(adf_lball_root_seed(z,z,n,1,30)==st && adf_lball_identical(z,x));
            continue;
        }
        ADF_CHECK(count==expected && len==(slong)expected);
        for (slong i=0;i<len;i++)
        {
            ulong seed=arr(r,"ids",(size_t)i);
            if (n==1) adf_lball_set(want,x);
            else raw(want,p,(slong)arr(r,"bs",(size_t)i),num(r,"m")/(slong)n,num(r,"E"),0);
            ADF_CHECK(ids[i]==seed && adf_lball_equal_set(roots+i,want));
            ADF_CHECK(adf_lball_is_canonical(roots+i));
            adf_lball_set(z,x);
            ADF_CHECK(adf_lball_root_seed(y,x,n,seed,30)==ADF_OK && adf_lball_equal_set(y,want));
            ADF_CHECK(adf_lball_root_seed(z,z,n,seed,30)==ADF_OK && adf_lball_equal_set(z,want));
            if (n==2)
                ADF_CHECK(adf_lball_sqrt_seed(z,x,seed,30)==ADF_OK && adf_lball_equal_set(z,want));
            ADF_CHECK(adf_lball_pow_si(z,roots+i,(slong)n)==ADF_OK && adf_lball_equal_set(z,x));
        }
        for (slong i=len;i<12;i++) ADF_CHECK(ids[i]==99 && adf_lball_identical(roots+i,saved));
    }
    printf("  oracle rows: %zu\n",jsonl_count(f));
    for (int i=0;i<12;i++) adf_lball_clear(roots+i);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z);
    adf_lball_clear(saved); adf_lball_clear(want); jsonl_close(f);
}

ADF_TEST(exact_branches_limits_and_aliasing)
{
    adf_lball_t x,y,z,save;
    adf_lball_struct rs[6];
    ulong ids[6], count=99;
    slong len=-1;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(save);
    for (int i=0;i<6;i++) adf_lball_init(rs+i);
    for (ulong p=2;p<=7;p=n_nextprime(p,1))
    {
        raw(x,p,p==3 ? 1 : 9,p==3 ? 2 : 0,0,1);
        ADF_CHECK(adf_lball_roots(rs,ids,&len,6,x,2,20)==ADF_OK && len==2);
        for (int i=0;i<2;i++)
        {
            ADF_CHECK(rs[i].exact && adf_lball_pow_si(z,rs+i,2)==ADF_OK && adf_lball_equal_set(z,x));
            adf_lball_set(y,x);
            ADF_CHECK(adf_lball_sqrt_seed(y,y,ids[i],LONG_MIN)==ADF_OK && adf_lball_equal_set(y,rs+i));
        }
        raw(x,p,0,0,0,1);
        ADF_CHECK(adf_lball_roots(rs,ids,&len,6,x,12,LONG_MIN)==ADF_OK && len==1 && ids[0]==0);
        ADF_CHECK(rs[0].exact && adf_lball_equal_set(rs,x));
    }
    raw(x,2,3,0,0,1);
    ADF_CHECK(adf_lball_root_count(&count,x,2)==ADF_DOMAIN);
    raw(x,7,2,0,0,1);
    ADF_CHECK(adf_lball_root_count(&count,x,3)==ADF_DOMAIN);
    /* Irrational roots of 2 at 7, and rational / irrational branches of x^3=8 at 7. */
    for (int cube=0;cube<2;cube++)
    {
        raw(x,7,cube ? 8 : 2,0,0,1);
        ADF_CHECK(adf_lball_roots(rs,ids,&len,6,x,cube ? 3 : 2,8)==ADF_OK);
        ADF_CHECK(len==(cube ? 3 : 2));
        for (slong i=0;i<len;i++)
        {
            int rational=cube && ids[i]==2;
            ADF_CHECK(rs[i].exact==rational);
            if (!rational) ADF_CHECK(rs[i].N==8);
            ADF_CHECK(adf_lball_pow_si(z,rs+i,cube ? 3 : 2)==ADF_OK && adf_lball_contains(x,z));
        }
    }
    raw(x,5,9,-4,0,1); fmpz_set_ui(fmpq_denref(x->u),4);
    ADF_CHECK(adf_lball_roots(rs,ids,&len,6,x,2,20)==ADF_OK && len==2);
    for (int i=0;i<2;i++)
        ADF_CHECK(rs[i].exact && rs[i].v==-2 && fmpz_equal_ui(fmpq_denref(rs[i].u),2));
    /* All array/input alias slots, including unused slots. */
    for (int slot=0;slot<6;slot++)
    {
        raw(rs+slot,7,1,0,3,0);
        ADF_CHECK(adf_lball_roots(rs,ids,&len,6,rs+slot,3,20)==ADF_OK && len==3);
        ADF_CHECK(ids[0]==1 && ids[1]==2 && ids[2]==4);
    }
    raw(save,11,17,0,0,1);
    for (int i=0;i<6;i++) { adf_lball_set(rs+i,save); ids[i]=99; }
    raw(x,7,1,0,3,0); len=-1;
    ADF_CHECK(adf_lball_roots(rs,ids,&len,2,x,3,20)==ADF_LIMIT && len==-1);
    ADF_CHECK(adf_lball_roots(rs,ids,&len,-1,x,3,20)==ADF_DOMAIN && len==-1);
    for (int i=0;i<6;i++) ADF_CHECK(ids[i]==99 && adf_lball_identical(rs+i,save));
    for (int mode=0;mode<6;mode++)
    {
        slong N=20;
        int st=ADF_LIMIT;
        raw(x,7,2,0,0,1);
        if (mode==0) N=LONG_MIN;
        if (mode==1) N=LONG_MAX;
        if (mode==2) N=ADF_LBALL_BITS_MAX;
        if (mode==3) x->v=ADF_LBALL_EXP_MAX+1;
        if (mode==4) { st=ADF_DOMAIN; }
        if (mode==5) { raw(x,2,1,0,2,0); st=ADF_NOT_DETERMINED; }
        adf_lball_set(y,save); adf_lball_set(z,x);
        ADF_CHECK(adf_lball_root_seed(y,x,2,mode==4 ? 0 : (mode==5 ? 1 : 3),N)==st);
        ADF_CHECK(adf_lball_identical(y,save));
        ADF_CHECK(adf_lball_root_seed(z,z,2,mode==4 ? 0 : (mode==5 ? 1 : 3),N)==st);
        ADF_CHECK(adf_lball_identical(z,x));
    }
    raw(x,5,1,0,0,1);
    ADF_CHECK(adf_lball_root_seed(y,x,0,1,20)==ADF_DOMAIN);
    for (int i=0;i<6;i++) adf_lball_clear(rs+i);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(save);
}

ADF_TEST(large_prime_and_word_degrees)
{
    const ulong p=UWORD(18446744073709551557);
    adf_lball_t x,y,z;
    ulong count=99;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z);
    /* a=(1+p)^2+p^3: nonrational square root, seed 1; 200 absolute digits. */
    raw(x,p,1,0,0,1);
    fmpz_t P,t;
    fmpz_init_set_ui(P,p); fmpz_init(t);
    fmpz_add_ui(t,P,1); fmpz_mul(t,t,t);
    fmpz_pow_ui(fmpq_numref(x->u),P,3); fmpz_add(fmpq_numref(x->u),fmpq_numref(x->u),t);
    ADF_CHECK(adf_lball_sqrt_seed(y,x,1,200)==ADF_OK && !y->exact && y->N==200);
    ADF_CHECK(adf_lball_pow_si(z,y,2)==ADF_OK && adf_lball_contains(x,z));
    for (int k=0;k<3;k++)
    {
        ulong n=k==0 ? (ulong)WORD_MAX : (k==1 ? UWORD_MAX : p);
        raw(x,p,1,0,3,0);
        ADF_CHECK(adf_lball_root_count(&count,x,n)==ADF_OK && count==n_gcd(n,p-1));
        ADF_CHECK(adf_lball_root_seed(y,x,n,1,200)==ADF_OK && y->N==(k==2 ? 2 : 3));
    }
    raw(x,2,1,0,64,0);
    ADF_CHECK(adf_lball_root_seed(y,x,UWORD(1)<<62,1,200)==ADF_OK && y->N==2);
    /* Count succeeds without a branch array even when listing is limited. */
    raw(x,p,1,0,0,1);
    ADF_CHECK(adf_lball_root_count(&count,x,p-1)==ADF_OK && count==p-1);
    ADF_CHECK(adf_lball_root_seed(y,x,p-1,1,LONG_MIN)==ADF_OK && y->exact);
    fmpz_clear(P); fmpz_clear(t);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z);
}

/* Integer modular powering is an independent value oracle here. Fixed seed: 2400 calls;
   input unit exponents r<=8, comparison H=r+1. E and a distance-p^E witness are explicit. */
ADF_TEST(seeded_small_ball_comparisons)
{
    adf_lball_t x,y,z,point;
    fmpz_t P,a,b,tmp;
    const ulong ps[]={2,3,5,7};
    ulong state=UWORD(981273), count;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(point);
    fmpz_init(P); fmpz_init(a); fmpz_init(b); fmpz_init(tmp);
    for (int i=0;i<2400;i++)
    {
        state=state*UWORD(6364136223846793005)+1;
        ulong p=ps[(state>>12)%4], n=2+(state>>16)%11, u=1+(state>>24)%100;
        slong s=0,j=(slong)((state>>32)%5)-2,r;
        for (ulong nn=n;nn%p==0;nn/=p) s++;
        r=(p==2 ? 2 : 1)+s+(slong)((state>>40)%3);
        while (u%p==0) u++;
        fmpz_ui_pow_ui(P,p,(ulong)r); fmpz_set_ui(b,u); fmpz_powm_ui(a,b,n,P);
        raw(x,p,(slong)fmpz_get_ui(a),(slong)n*j,(slong)n*j+r,0);
        ulong id=u%(p==2 ? 4 : p);
        slong E=j+r-s;
        ADF_CHECK(adf_lball_root_count(&count,x,n)==ADF_OK && count==n_gcd(n,p==2 ? 2 : p-1));
        ADF_CHECK(adf_lball_root_seed(y,x,n,id,-100)==ADF_OK && y->N==E && y->v==j);
        raw(point,p,(slong)u,j,0,1);
        ADF_CHECK(adf_lball_contains(point,y));
        fmpz_ui_pow_ui(tmp,p,(ulong)(r-s)); fmpz_add_ui(tmp,tmp,u);
        fmpq_set_fmpz(point->u,tmp);
        ADF_CHECK(adf_lball_contains(point,y)); /* difference exactly p^E */
        fmpz_ui_pow_ui(P,p,(ulong)r); fmpz_powm_ui(tmp,tmp,n,P);
        ADF_CHECK(fmpz_equal(tmp,a)); /* its n-th power lies in x */
        ADF_CHECK(adf_lball_pow_si(z,y,(slong)n)==ADF_OK && adf_lball_equal_set(z,x));
    }
    fmpz_clear(P); fmpz_clear(a); fmpz_clear(b); fmpz_clear(tmp);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(point);
}

ADF_TEST(boundaries_and_transaction_after_one_branch)
{
    adf_lball_t x,y,z,saved;
    adf_lball_struct roots[3];
    ulong ids[3]={99,99,99}, count=99;
    slong len=-1;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(saved);
    raw(saved,11,17,0,0,1);
    for (int i=0;i<3;i++) { adf_lball_init(roots+i); adf_lball_set(roots+i,saved); }
    /* First branch of cube root of 1 is exact; the next needs N and fails. */
    raw(x,7,1,0,0,1);
    ADF_CHECK(adf_lball_roots(roots,ids,&len,3,x,3,LONG_MAX)==ADF_LIMIT);
    ADF_CHECK(len==-1);
    for (int i=0;i<3;i++) ADF_CHECK(ids[i]==99 && adf_lball_identical(roots+i,saved));
    raw(x,3,1,-ADF_LBALL_EXP_MAX,ADF_LBALL_EXP_MAX,0);
    adf_lball_set(y,saved);
    ADF_CHECK(adf_lball_root_count(&count,x,2)==ADF_OK && count==2);
    ADF_CHECK(adf_lball_root_seed(y,x,2,1,20)==ADF_LIMIT && adf_lball_identical(y,saved));
    raw(x,3,1,-ADF_LBALL_EXP_MAX,0,1);
    ADF_CHECK(adf_lball_root_seed(y,x,2,1,LONG_MIN)==ADF_OK && y->exact);
    ADF_CHECK(y->v==-ADF_LBALL_EXP_MAX/2);
    /* Whole-ball valuation obstruction, including negative m. */
    for (int j=-1;j<=1;j+=2)
    {
        raw(x,5,1,j,j+4,0); count=99;
        ADF_CHECK(adf_lball_root_count(&count,x,2)==ADF_DOMAIN && count==99);
    }
    raw(x,2,-1,0,0,1);
    ADF_CHECK(adf_lball_root_seed(y,x,UWORD_MAX,3,20)==ADF_OK && adf_lball_equal_set(y,x));
    raw(x,2,0,0,0,1);
    ADF_CHECK(adf_lball_root_seed(y,x,2,1,20)==ADF_DOMAIN);
    raw(x,2,0,0,-4,0);
    ADF_CHECK(adf_lball_root_seed(y,x,1,999,LONG_MIN)==ADF_OK && adf_lball_equal_set(x,y));
    raw(x,7,2,0,0,1);
    for (slong N=-2;N<=8;N++)
    {
        ADF_CHECK(adf_lball_root_seed(y,x,2,3,N)==ADF_OK && !y->exact && y->N==N);
        ADF_CHECK(adf_lball_root_seed(z,x,2,3,10)==ADF_OK && adf_lball_contains(z,y));
    }
    /* Thousands of bits; rational branch detection is exact without factoring. */
    raw(x,5,1,0,0,1);
    fmpz_mul_2exp(fmpq_numref(x->u),fmpq_numref(x->u),4096);
    ADF_CHECK(adf_lball_root_seed(y,x,2,1,LONG_MAX)==ADF_OK && y->exact);
    ADF_CHECK(fmpz_bits(fmpq_numref(y->u))==2049);
    ADF_CHECK(adf_lball_pow_si(z,y,2)==ADF_OK && adf_lball_equal_set(z,x));
    {
        ulong p=UWORD(18446744073709551557);
        raw(x,p,1,0,0,1);
        ADF_CHECK(adf_lball_roots(NULL,NULL,&len,0,x,p-1,20)==ADF_LIMIT && len==-1);
        fmpz_set_ui(fmpq_numref(x->u),p); fmpz_add_ui(fmpq_numref(x->u),fmpq_numref(x->u),9);
        ADF_CHECK(adf_lball_roots(roots,ids,&len,3,x,2,200)==ADF_OK && len==2);
        ADF_CHECK(ids[0]==3 && ids[1]==p-3);
        for (int i=0;i<2;i++)
        {
            ADF_CHECK(!roots[i].exact && roots[i].N==200);
            ADF_CHECK(adf_lball_pow_si(z,roots+i,2)==ADF_OK && adf_lball_contains(x,z));
        }
    }
    for (int i=0;i<3;i++) adf_lball_clear(roots+i);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(saved);
}

ADF_TEST(exact_finite_ring_reference)
{
    jsonl_file *f=NULL;
    jsonl_error_t error;
    adf_lball_t x,y,z,want;
    adf_lball_struct roots[6];
    ulong ids[6];
    slong len;
    ADF_CHECK(jsonl_open("tests/ref/vectors/f-slice8/exact.jsonl",&f,&error));
    if (!f) return;
    ADF_CHECK(jsonl_count(f)==1408);
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(want);
    for (int i=0;i<6;i++) adf_lball_init(roots+i);
    for (size_t k=0;k<jsonl_count(f);k++)
    {
        const jsonl_value *r=jsonl_record(f,k);
        ulong p=(ulong)num(r,"p"),n=(ulong)num(r,"n");
        int status=(int)num(r,"s");
        raw(x,p,1,0,0,1); fmpq_set_si(x->u,num(r,"an"),(ulong)num(r,"ad"));
        ADF_CHECK(adf_lball_roots(roots,ids,&len,6,x,n,num(r,"N"))==status);
        if (status) continue;
        ADF_CHECK(len==(slong)jsonl_size(field(r,"ids")));
        for (slong i=0;i<len;i++)
        {
            ulong seed=arr(r,"ids",(size_t)i),den=arr(r,"qd",(size_t)i);
            raw(want,p,1,0,den ? 0 : num(r,"N"),den!=0);
            if (den)
            {
                slong a=strtol(jsonl_int_text(jsonl_at(field(r,"qn"),(size_t)i,NULL),NULL),NULL,10);
                fmpq_set_si(want->u,a,den);
            }
            else fmpq_set_ui(want->u,arr(r,"bs",(size_t)i),1);
            ADF_CHECK(ids[i]==seed && adf_lball_identical(roots+i,want));
            adf_lball_set(z,x);
            ADF_CHECK(adf_lball_root_seed(z,z,n,seed,num(r,"N"))==ADF_OK && adf_lball_identical(z,want));
            for (slong low=-2;low<=1;low+=3)
            {
                ADF_CHECK(adf_lball_root_seed(y,x,n,seed,low)==ADF_OK);
                if (den) ADF_CHECK(adf_lball_identical(y,want));
                else ADF_CHECK(!y->exact && y->N==low && adf_lball_contains(want,y));
            }
        }
    }
    printf("  exact oracle rows: %zu\n",jsonl_count(f));
    for (int i=0;i<6;i++) adf_lball_clear(roots+i);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(want); jsonl_close(f);
}
