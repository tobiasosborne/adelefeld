/* Exact oracle: proto/lroot_checks.py; comparison precision H=r+1 in normalized units.
   Any different status, count, seed, exponent, centre, alias or failed transaction fails a case. */
#include <limits.h>
#include <time.h>
#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include "support/jsonl.h"
#include "test_runner.h"

/* N-D14 (docs/SPEC.md 15.4): a ball result has exponent K=min(N,E); BIG>=E gives the exact image. */
#define BIG ADF_LBALL_EXP_MAX

/* The ball of exponent K<E that contains the exact image img=p^j(b+p^(E-j)Z_p) (N-D14):
   the zero ball at K for K<=j (R4 step 5), else centre b mod p^(K-j), valuation j. */
static void coarse(adf_lball_t want, const adf_lball_t img, slong K)
{
    fmpz_t r;
    fmpz_init(r);
    if (K<=img->v) { want->p=img->p; want->v=0; want->N=K; want->exact=0; fmpq_zero(want->u); }
    else
    {
        ADF_CHECK(adf_lball_unit_mod(r,img,K-img->v)==ADF_OK);
        want->p=img->p; want->v=img->v; want->N=K; want->exact=0; fmpq_set_fmpz(want->u,r);
    }
    ADF_CHECK(adf_lball_is_canonical(want));
    fmpz_clear(r);
}

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
        ADF_CHECK(adf_lball_roots(roots,ids,&len,12,x,n,BIG)==st);
        if (st)
        {
            ADF_CHECK(count==99 && len==-1);
            for (int i=0;i<12;i++) ADF_CHECK(ids[i]==99 && adf_lball_identical(roots+i,saved));
            adf_lball_set(y,saved); adf_lball_set(z,x);
            ADF_CHECK(adf_lball_root_seed(y,x,n,1,BIG)==st && adf_lball_identical(y,saved));
            ADF_CHECK(adf_lball_root_seed(z,z,n,1,BIG)==st && adf_lball_identical(z,x));
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
            ADF_CHECK(adf_lball_root_seed(y,x,n,seed,BIG)==ADF_OK && adf_lball_equal_set(y,want));
            ADF_CHECK(adf_lball_root_seed(z,z,n,seed,BIG)==ADF_OK && adf_lball_equal_set(z,want));
            if (n==2)
                ADF_CHECK(adf_lball_sqrt_seed(z,x,seed,BIG)==ADF_OK && adf_lball_equal_set(z,want));
            ADF_CHECK(adf_lball_pow_si(z,roots+i,(slong)n)==ADF_OK && adf_lball_equal_set(z,x));
            if (n==1) continue;
            /* N-D14: N=E is the image itself; N=E-1, E-3 give the enclosing ball at N. */
            ADF_CHECK(adf_lball_root_seed(y,x,n,seed,num(r,"E"))==ADF_OK && adf_lball_identical(y,want));
            for (slong low=1;low<=3;low+=2)
            {
                adf_lball_t c;
                adf_lball_init(c);
                coarse(c,want,num(r,"E")-low);
                ADF_CHECK(adf_lball_root_seed(y,x,n,seed,num(r,"E")-low)==ADF_OK);
                ADF_CHECK(adf_lball_identical(y,c) && adf_lball_contains(want,y));
                adf_lball_clear(c);
            }
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
        ADF_CHECK(adf_lball_root_seed(y,x,n,id,BIG)==ADF_OK && y->N==E && y->v==j);
        /* N-D14: every N from j-2 to E+1; the centre is u mod p^(N-j), u being in the image. */
        for (slong N=j-2;N<=E+1;N++)
        {
            slong K=N<E ? N : E;
            if (K<=j) raw(point,p,0,0,K,0);
            else
            {
                fmpz_ui_pow_ui(P,p,(ulong)(K-j)); fmpz_set_ui(tmp,u); fmpz_mod(tmp,tmp,P);
                raw(point,p,1,j,K,0); fmpq_set_fmpz(point->u,tmp);
                ADF_CHECK(adf_lball_is_canonical(point));
            }
            ADF_CHECK(adf_lball_root_seed(z,x,n,id,N)==ADF_OK && adf_lball_identical(z,point));
            ADF_CHECK(adf_lball_contains(y,z));
        }
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
    /* N-D14: E=j+r-s=3*2^59 is beyond the bound, so N>=E is LIMIT; N=20 gives the ball at 20. */
    ADF_CHECK(adf_lball_root_seed(y,x,2,1,LONG_MAX)==ADF_LIMIT && adf_lball_identical(y,saved));
    ADF_CHECK(adf_lball_root_seed(y,x,2,1,ADF_LBALL_EXP_MAX)==ADF_OK);
    ADF_CHECK(y->v==-ADF_LBALL_EXP_MAX/2 && y->N==ADF_LBALL_EXP_MAX && fmpq_is_one(y->u) && !y->exact);
    ADF_CHECK(adf_lball_root_seed(y,x,2,1,20)==ADF_OK);
    ADF_CHECK(y->v==-ADF_LBALL_EXP_MAX/2 && y->N==20 && fmpq_is_one(y->u) && !y->exact);
    ADF_CHECK(adf_lball_is_canonical(y));
    /* The branch -1 needs the centre 3^(20+2^59)-1 (lball.h: a stored centre beyond the bit bound). */
    adf_lball_set(y,saved);
    ADF_CHECK(adf_lball_root_seed(y,x,2,2,20)==ADF_LIMIT && adf_lball_identical(y,saved));
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

/* N-D14 (docs/SPEC.md 15.4): for a ball, K=min(N,E). x=9+7^10 Z_7, n=2: E=10, branches 3, 4.
   Expected centres by hand: 3 and 7^4-3=2398 at N=4. Negative j: 7^-2(9+7^10 Z_7), j=-1, E=9. */
ADF_TEST(nd14_ball_result_exponent_is_min_of_N_and_E)
{
    adf_lball_t x,y,z,img,want,saved;
    adf_lball_struct rs[4];
    ulong ids[4];
    slong len=-1;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(img);
    adf_lball_init(want); adf_lball_init(saved);
    for (int i=0;i<4;i++) adf_lball_init(rs+i);
    raw(saved,11,17,0,0,1);
    raw(x,7,9,0,10,0);
    ADF_CHECK(adf_lball_root_seed(y,x,2,3,4)==ADF_OK);
    raw(want,7,3,0,4,0);
    ADF_CHECK(adf_lball_identical(y,want));
    ADF_CHECK(adf_lball_root_seed(y,x,2,4,4)==ADF_OK);
    raw(want,7,2398,0,4,0);
    ADF_CHECK(adf_lball_identical(y,want));
    /* N=E and N>E: the exact image, centre 3 modulo 7^10 */
    for (slong N=10;N<=12;N++)
    {
        ADF_CHECK(adf_lball_root_seed(y,x,2,3,N)==ADF_OK);
        raw(want,7,3,0,10,0);
        ADF_CHECK(adf_lball_identical(y,want));
    }
    /* N=9: E-1; the centre 3 stays, exponent 9 */
    ADF_CHECK(adf_lball_root_seed(y,x,2,3,9)==ADF_OK && y->N==9 && fmpz_equal_ui(fmpq_numref(y->u),3));
    /* N<=j=0: the zero ball at N (R4 step 5) */
    for (slong N=-3;N<=0;N++)
    {
        raw(want,7,0,0,N,0);
        ADF_CHECK(adf_lball_root_seed(y,x,2,4,N)==ADF_OK && adf_lball_identical(y,want));
    }
    /* y=x aliasing at N<E */
    adf_lball_set(z,x);
    ADF_CHECK(adf_lball_root_seed(z,z,2,4,4)==ADF_OK);
    raw(want,7,2398,0,4,0);
    ADF_CHECK(adf_lball_identical(z,want));
    ADF_CHECK(adf_lball_sqrt_seed(z,x,4,4)==ADF_OK && adf_lball_identical(z,want));
    /* all branches at N=4, with x aliasing a slot */
    adf_lball_set(rs+1,x);
    ADF_CHECK(adf_lball_roots(rs,ids,&len,4,rs+1,2,4)==ADF_OK && len==2);
    ADF_CHECK(ids[0]==3 && ids[1]==4);
    raw(want,7,3,0,4,0); ADF_CHECK(adf_lball_identical(rs,want));
    raw(want,7,2398,0,4,0); ADF_CHECK(adf_lball_identical(rs+1,want));
    /* negative valuation: x=7^-2(9+7^10 Z_7), v=-2, M=8: j=-1, E=-1+10-0=9 */
    raw(x,7,9,-2,8,0);
    ADF_CHECK(adf_lball_root_seed(img,x,2,3,BIG)==ADF_OK && img->v==-1 && img->N==9);
    for (slong N=-4;N<=11;N++)
    {
        slong K=N<9 ? N : 9;
        ADF_CHECK(adf_lball_root_seed(y,x,2,3,N)==ADF_OK);
        if (K<=-1) raw(want,7,0,0,K,0);
        else { raw(want,7,1,-1,K,0); fmpq_set_si(want->u,3,1); }
        ADF_CHECK(adf_lball_identical(y,want) && adf_lball_contains(img,y));
    }
    /* K is checked against the exponent bound: N=LONG_MIN is LIMIT for a ball, outputs untouched */
    adf_lball_set(y,saved); len=-1;
    ADF_CHECK(adf_lball_root_seed(y,x,2,3,LONG_MIN)==ADF_LIMIT && adf_lball_identical(y,saved));
    ADF_CHECK(adf_lball_roots(rs,ids,&len,4,x,2,LONG_MIN)==ADF_LIMIT && len==-1);
    /* degree 1 stays the identity for every N, also in the list (mutant of early_status) */
    ADF_CHECK(adf_lball_root_seed(y,x,1,0,LONG_MIN)==ADF_OK && adf_lball_identical(y,x));
    ADF_CHECK(adf_lball_roots(rs,ids,&len,4,x,1,LONG_MIN)==ADF_OK && len==1 && ids[0]==0);
    ADF_CHECK(adf_lball_identical(rs,x));
    raw(z,7,2,0,0,1); len=-1;
    ADF_CHECK(adf_lball_roots(rs,ids,&len,4,z,1,LONG_MIN)==ADF_OK && len==1 && ids[0]==0);
    ADF_CHECK(adf_lball_identical(rs,z));
    for (int i=0;i<4;i++) adf_lball_clear(rs+i);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(img);
    adf_lball_clear(want); adf_lball_clear(saved);
}

/* F1 of review f-review6: the exact unit 1 needs no power. 1+2^R Z_2, n=2, seed 1 has the image
   1+2^(R-1) Z_2 (R2: s=1, j=0); no power of 2 is formed. The branch -1 needs the centre
   2^K-1 and is LIMIT for K beyond the bit bound (lball.h), OK at N=100. */
ADF_TEST(f1_unit_one_ball_needs_no_power)
{
    adf_lball_t x,y,want,saved;
    adf_lball_struct rs[2];
    ulong ids[2]={99,99};
    slong len=-1;
    const slong Rs[2]={WORD(1)<<27,ADF_LBALL_EXP_MAX};
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(want); adf_lball_init(saved);
    for (int i=0;i<2;i++) adf_lball_init(rs+i);
    raw(saved,11,17,0,0,1);
    for (int k=0;k<2;k++)
    {
        slong R=Rs[k];
        raw(x,2,1,0,R,0);
        raw(want,2,1,0,R-1,0);
        ADF_CHECK(adf_lball_root_seed(y,x,2,1,LONG_MAX)==ADF_OK && adf_lball_identical(y,want));
        ADF_CHECK(adf_lball_root_seed(y,x,2,1,BIG)==ADF_OK && adf_lball_identical(y,want));
        ADF_CHECK(adf_lball_sqrt_seed(y,x,1,LONG_MAX)==ADF_OK && adf_lball_identical(y,want));
        raw(want,2,1,0,100,0);
        ADF_CHECK(adf_lball_root_seed(y,x,2,1,100)==ADF_OK && adf_lball_identical(y,want));
        adf_lball_set(y,saved);
        ADF_CHECK(adf_lball_root_seed(y,x,2,3,LONG_MAX)==ADF_LIMIT && adf_lball_identical(y,saved));
        ADF_CHECK(adf_lball_roots(rs,ids,&len,2,x,2,LONG_MAX)==ADF_LIMIT && len==-1 && ids[0]==99);
        ADF_CHECK(adf_lball_root_seed(y,x,2,3,100)==ADF_OK && y->N==100 && y->v==0);
        fmpz_one(fmpq_numref(want->u)); fmpz_mul_2exp(fmpq_numref(want->u),fmpq_numref(want->u),100);
        fmpz_sub_ui(fmpq_numref(want->u),fmpq_numref(want->u),1);
        ADF_CHECK(adf_lball_identical(y,want));
        ADF_CHECK(adf_lball_roots(rs,ids,&len,2,x,2,100)==ADF_OK && len==2 && ids[0]==1 && ids[1]==3);
        ADF_CHECK(adf_lball_identical(rs+1,want) && fmpq_is_one(rs[0].u) && rs[0].N==100);
        len=-1; ids[0]=99;
    }
    /* 1+5^(2^60) Z_5, n=3: one branch, image 1+5^(2^60) Z_5 (s=0) */
    raw(x,5,1,0,ADF_LBALL_EXP_MAX,0);
    ADF_CHECK(adf_lball_root_seed(y,x,3,1,LONG_MAX)==ADF_OK && adf_lball_identical(y,x));
    ADF_CHECK(adf_lball_roots(rs,ids,&len,2,x,3,LONG_MAX)==ADF_OK && len==1 && ids[0]==1);
    ADF_CHECK(adf_lball_identical(rs,x));
    /* where the power is formed, the limit stays: centre 17, Log needs 2^(2^27) */
    raw(x,2,17,0,WORD(1)<<27,0);
    adf_lball_set(y,saved);
    ADF_CHECK(adf_lball_root_seed(y,x,2,1,LONG_MAX)==ADF_LIMIT && adf_lball_identical(y,saved));
    ADF_CHECK(adf_lball_root_seed(y,x,2,1,100)==ADF_OK && y->N==100);
    ADF_CHECK(adf_lball_pow_si(want,y,2)==ADF_OK);
    raw(saved,2,17,0,101,0);
    ADF_CHECK(adf_lball_equal_set(want,saved));
    for (int i=0;i<2;i++) adf_lball_clear(rs+i);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(want); adf_lball_clear(saved);
}

/* F2 of review f-review6, R8: the identifiers of every branch, against a search over all residues.
   Every odd prime p<110, every degree n in 2..p-1 and 2(p-1), every unit w<p (DOMAIN when w is not
   an n-th power residue): the list is {t : t^n=w mod p} in increasing order. N=0 gives zero balls (no lift). */
ADF_TEST(f2_identifiers_against_residue_search)
{
    adf_lball_t x;
    adf_lball_struct *rs=flint_malloc(200*sizeof(adf_lball_struct));
    ulong ids[200], want[200], lists=0;
    adf_lball_init(x);
    for (int i=0;i<200;i++) adf_lball_init(rs+i);
    for (ulong p=3;p<110;p=n_nextprime(p,1))
        for (ulong n=2;n<=p;n++)
        {
            ulong deg=n==p ? 2*(p-1) : n, pw[200];
            for (ulong t=1;t<p;t++) pw[t]=n_powmod2(t,deg,p);
            for (ulong w=1;w<p;w++)
            {
                slong len=-1, k=0;
                for (ulong t=1;t<p;t++) if (pw[t]==w) want[k++]=t;
                raw(x,p,(slong)w,0,0,1);
                int st=adf_lball_roots(rs,ids,&len,200,x,deg,0);
                if (k==0) { ADF_CHECK(st==ADF_DOMAIN && len==-1); continue; }
                ADF_CHECK(st==ADF_OK && len==k && (ulong)k==n_gcd(deg,p-1));
                for (slong i=0;i<len && i<k;i++) ADF_CHECK(ids[i]==want[i]);
                lists++;
            }
        }
    printf("  identifier lists compared: %lu\n",lists);
    for (int i=0;i<200;i++) adf_lball_clear(rs+i);
    flint_free(rs); adf_lball_clear(x);
}

/* the CPU seconds of one roots call */
static double timed_roots(int *st, adf_lball_ptr y, ulong *ids, slong *len, slong cap,
                          const adf_lball_t x, ulong n, slong N)
{
    clock_t t0=clock();
    *st=adf_lball_roots(y,ids,len,cap,x,n,N);
    return (double)(clock()-t0)/CLOCKS_PER_SEC;
}

/* F2 of review f-review6: LIMIT is decided before the enumeration, and the d branches are listed
   as t0 zeta^i (R8). Guards in CPU seconds: 2 s where the old code took 5.7 to 6 s (Rabin's
   method on T^d-w^e), 10 s for the evaluation of all branches. Identifiers: d distinct, increasing,
   t^n=w modulo p. */
ADF_TEST(f2_all_branches_cost_and_early_limit)
{
    const ulong P64=UWORD(18446744073709551557);
    const struct { ulong p, n, w; } cs[3]={{65537,65536,1},{P64,6028,1},{65537,12288,0}};
    adf_lball_t x,saved;
    adf_lball_ptr rs=flint_malloc(65536*sizeof(adf_lball_struct));
    ulong *ids=flint_malloc(65536*sizeof(ulong));
    slong len=-1;
    int st;
    double sec;
    adf_lball_init(x); adf_lball_init(saved);
    raw(saved,11,17,0,0,1);
    for (slong i=0;i<65536;i++) { adf_lball_init(rs+i); adf_lball_set(rs+i,saved); ids[i]=99; }
    /* capacity 1 < d=65536: LIMIT at once (old code: already before the enumeration) */
    raw(x,65537,1,0,0,1);
    sec=timed_roots(&st,rs,ids,&len,1,x,65536,20);
    ADF_CHECK(st==ADF_LIMIT && len==-1 && sec<2.0);
    /* the Teichmueller factor needs 65537^(2^40): LIMIT before the enumeration (old: 6.0 s) */
    sec=timed_roots(&st,rs,ids,&len,65536,x,65536,WORD(1)<<40);
    ADF_CHECK_MSG(st==ADF_LIMIT && len==-1 && sec<2.0,"LIMIT after %.2f s",sec);
    ADF_CHECK(ids[0]==99 && ids[65535]==99 && adf_lball_identical(rs,saved) &&
              adf_lball_identical(rs+65535,saved));
    for (int c=0;c<3;c++)
    {
        ulong p=cs[c].p, n=cs[c].n, w=cs[c].w, d=n_gcd(n,p-1);
        if (w==0) w=n_powmod2(3,n,p); /* 3^n: a non-trivial n-th power residue at 65537 */
        raw(x,p,1,0,0,1); fmpz_set_ui(fmpq_numref(x->u),w);
        for (int pass=0;pass<2;pass++)
        {
            slong N=pass ? 20 : 0;
            sec=timed_roots(&st,rs,ids,&len,65536,x,n,N);
            ADF_CHECK_MSG(st==ADF_OK && len==(slong)d && sec<(pass ? 10.0 : 2.0),
                          "p=%lu n=%lu N=%ld: %.2f s",p,n,(long)N,sec);
            if (st!=ADF_OK) continue;
            for (slong i=0;i<len;i++)
            {
                ADF_CHECK(n_powmod2(ids[i],n,p)==w && (i==0 || ids[i-1]<ids[i]));
                ADF_CHECK(adf_lball_is_canonical(rs+i));
                if (!pass) ADF_CHECK(rs[i].exact || (fmpq_is_zero(rs[i].u) && rs[i].N==0));
                else ADF_CHECK(rs[i].exact || (rs[i].v==0 && rs[i].N==20));
            }
            /* a sample of 16 branches: x lies inside y^n */
            for (slong i=0;pass && i<len;i+=len/16 ? len/16 : 1)
            {
                adf_lball_t z;
                adf_lball_init(z);
                ADF_CHECK(adf_lball_pow_si(z,rs+i,(slong)n)==ADF_OK && adf_lball_contains(x,z));
                adf_lball_clear(z);
            }
        }
    }
    for (slong i=0;i<65536;i++) adf_lball_clear(rs+i);
    flint_free(rs); flint_free(ids);
    adf_lball_clear(x); adf_lball_clear(saved);
}

/* Finding 3 of review f-review7 (lanes/f-review7/early.in lines 8 to 13): the branch p-1 (or 3 at 2)
   of a unit ball 1+p^R Z_p with the exact z0=1 needs the centre p^(K-j)-1 (R4 step 4), LIMIT for
   K-j beyond the bit bound; the branch 1 needs no power. 1+7^(2^40) Z_7 and 1+2^(2^27) Z_2, n=2,
   N=LONG_MAX: the list is LIMIT (decided before the listing since lane f-repair5, R6 step 6) and
   every output is untouched; seed 1 is OK with the image, seed p-1 (3 at 2) is LIMIT. Fails on
   another status, a written slot, id or len, or another value of seed 1. */
ADF_TEST(review7_f3_branch_limit_after_a_good_branch)
{
    const struct { ulong p; slong R; ulong neg; } cs[2]={{7,WORD(1)<<40,6},{2,WORD(1)<<27,3}};
    adf_lball_t x,y,want,saved;
    adf_lball_struct rs[3];
    ulong ids[3];
    slong len;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(want); adf_lball_init(saved);
    raw(saved,11,17,0,0,1);
    for (int i=0;i<3;i++) adf_lball_init(rs+i);
    for (int c=0;c<2;c++)
    {
        ulong p=cs[c].p;
        slong R=cs[c].R;
        for (int i=0;i<3;i++) { adf_lball_set(rs+i,saved); ids[i]=99; }
        len=-7;
        raw(x,p,1,0,R,0);
        ADF_CHECK_MSG(adf_lball_roots(rs,ids,&len,3,x,2,LONG_MAX)==ADF_LIMIT,"p=%lu",p);
        ADF_CHECK(len==-7);
        for (int i=0;i<3;i++) ADF_CHECK(ids[i]==99 && adf_lball_identical(rs+i,saved));
        /* the same with x aliasing a slot */
        adf_lball_set(rs+1,x);
        ADF_CHECK(adf_lball_roots(rs,ids,&len,3,rs+1,2,LONG_MAX)==ADF_LIMIT && len==-7);
        ADF_CHECK(adf_lball_identical(rs+1,x) && adf_lball_identical(rs,saved) && ids[0]==99);
        /* seed 1: the image 1+p^(R-s) Z_p, s=v_p(2); seed p-1: LIMIT */
        raw(want,p,1,0,p==2 ? R-1 : R,0);
        ADF_CHECK(adf_lball_root_seed(y,x,2,1,LONG_MAX)==ADF_OK && adf_lball_identical(y,want));
        adf_lball_set(y,saved);
        ADF_CHECK(adf_lball_root_seed(y,x,2,cs[c].neg,LONG_MAX)==ADF_LIMIT && adf_lball_identical(y,saved));
    }
    for (int i=0;i<3;i++) adf_lball_clear(rs+i);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(want); adf_lball_clear(saved);
}

/* R9 (lane f-repair5): adf_lball_roots lifts two Teichmueller representatives, not one per branch.
   p=2^64-59, x=3^n+p^30 Z_p (z0 a ball), n=d=24068, N=20: all branches. Guard in CPU seconds, as
   above: GUARD_R9 s; the timings of both codes are in lanes/f-repair5/result.md. Every listed branch
   is identical (all fields) to adf_lball_root_seed at its seed, which lifts its own representative:
   all of them at d=1094 and d=6028 (exact x=1, the branches 1 and p-1 rational), every 97th at
   d=24068. Fails on the time, a status, a count, or a branch that differs in any field. */
#define GUARD_R9 0.45
ADF_TEST(r9_one_teichmuller_lift_per_list)
{
    const ulong P64=UWORD(18446744073709551557);
    const struct { ulong n; int ball; slong step; } cs[3]={{24068,1,97},{1094,0,1},{6028,0,1}};
    adf_lball_t x,y;
    adf_lball_ptr rs=flint_malloc(24068*sizeof(adf_lball_struct));
    ulong *ids=flint_malloc(24068*sizeof(ulong)), compared=0;
    slong len=-1;
    int st;
    double sec;
    adf_lball_init(x); adf_lball_init(y);
    for (slong i=0;i<24068;i++) adf_lball_init(rs+i);
    for (int c=0;c<3;c++)
    {
        ulong n=cs[c].n, d=n_gcd(n,P64-1);
        if (cs[c].ball) { raw(x,P64,1,0,30,0); fmpz_set_ui(fmpq_numref(x->u),n_powmod2(3,n,P64)); }
        else raw(x,P64,1,0,0,1);
        sec=timed_roots(&st,rs,ids,&len,24068,x,n,20);
        ADF_CHECK_MSG(st==ADF_OK && len==(slong)d,"n=%lu: status %d len %ld",n,st,(long)len);
        if (cs[c].ball) ADF_CHECK_MSG(sec<GUARD_R9,"n=%lu: all branches in %.3f s",n,sec);
        printf("  n=%lu d=%lu N=20: all branches in %.3f s CPU\n",n,d,sec);
        for (slong i=0;st==ADF_OK && i<len;i+=cs[c].step)
        {
            ADF_CHECK(adf_lball_root_seed(y,x,n,ids[i],20)==ADF_OK);
            ADF_CHECK_MSG(adf_lball_identical(y,rs+i),"n=%lu seed %lu differs",n,ids[i]);
            compared++;
        }
    }
    printf("  branches compared with root_seed: %lu\n",compared);
    for (slong i=0;i<24068;i++) adf_lball_clear(rs+i);
    flint_free(rs); flint_free(ids); adf_lball_clear(x); adf_lball_clear(y);
}
