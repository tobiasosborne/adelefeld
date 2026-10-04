/* WP 1F.9 second group. Exact integer oracle rows from proto/catalogue2_checks.py.
   Every fixture row is used, including output aliasing and unchanged values on failures. */
#include <adelefeld.h>
#include <stdlib.h>
#include <string.h>
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "support/jsonl.h"
#include "test_runner.h"

static const char *field(const jsonl_value *r, const char *key)
{
    jsonl_error_t e;
    const jsonl_value *v;
    const char *s = NULL;
    ADF_CHECK(jsonl_field(r,key,&v,&e));
    ADF_CHECK(jsonl_int_text_or_string(v,&s,&e));
    return s;
}
static adf_place_t sentinel(void)
{
    adf_place_t p;
    ADF_CHECK(adf_place_prime(&p,97)==ADF_OK);
    return p;
}
static int binom_call(int tight, adf_fball_t y, adf_place_t *w, const adf_fball_t x, ulong k)
{
    return tight ? adf_fball_binom_tight(y,w,x,k) : adf_fball_binom(y,w,x,k);
}
ADF_TEST(binomial_vectors)
{
    jsonl_file *f = NULL;
    jsonl_error_t err;
    fmpz_t a,N,d,C,R;
    adf_fball_t x,y,want,old;
    adf_place_t w,mark=sentinel();
    ADF_CHECK(jsonl_open("tests/ref/vectors/f-slice13/binomial.jsonl",&f,&err));
    if (!f) return;
    fmpz_init(a); fmpz_init(N); fmpz_init(d); fmpz_init(C); fmpz_init(R);
    adf_fball_init(x); adf_fball_init(y); adf_fball_init(want); adf_fball_init(old);
    for (size_t i=0;i<jsonl_count(f);i++)
    {
        const jsonl_value *r=jsonl_record(f,i);
        ulong k=strtoul(field(r,"k"),NULL,10);
        int tight=atoi(field(r,"tight")), st=atoi(field(r,"status"));
        ADF_CHECK(fmpz_set_str(a,field(r,"a"),10)==0);
        ADF_CHECK(fmpz_set_str(N,field(r,"N"),10)==0);
        ADF_CHECK(fmpz_set_str(d,field(r,"d"),10)==0);
        ADF_CHECK(fmpz_set_str(C,field(r,"C"),10)==0);
        ADF_CHECK(fmpz_set_str(R,field(r,"R"),10)==0);
        ADF_CHECK(adf_fball_set_fmpz3(x,a,N,d)==0);
        fmpz_one(d);
        ADF_CHECK(adf_fball_set_fmpz3(want,C,R,d)==0);
        for (int alias=0;alias<2;alias++)
        {
            adf_fball_set(old,x);
            adf_fball_set_si(y,99);
            adf_fball_ptr out=alias ? x : y;
            w=mark;
            ADF_CHECK_MSG(binom_call(tight,out,&w,x,k)==st,"binomial row %zu",i);
            ADF_CHECK(adf_place_equal(w,mark));
            ADF_CHECK(adf_fball_identical(out,st==0 ? want : (alias ? old : y)));
            if (!alias && st!=0) ADF_CHECK(adf_fball_is_exact(y) && fmpz_equal_si(y->A,99));
            ADF_CHECK(adf_fball_is_canonical(out));
        }
    }
    printf("binomial fixture rows: %zu\n",jsonl_count(f));
    jsonl_close(f);
    fmpz_clear(a); fmpz_clear(N); fmpz_clear(d); fmpz_clear(C); fmpz_clear(R);
    adf_fball_clear(x); adf_fball_clear(y); adf_fball_clear(want); adf_fball_clear(old);
}
ADF_TEST(binomial_plan_limits_huge)
{
    adf_fball_t x,y,old;
    fmpz_t a,N,d,C;
    adf_place_t w=sentinel(), mark=w;
    adf_fball_init(x); adf_fball_init(y); adf_fball_init(old);
    fmpz_init(a); fmpz_init(N); fmpz_init(d); fmpz_init(C);
    fmpz_zero(a); fmpz_set_ui(N,8); fmpz_one(d);
    ADF_CHECK(adf_fball_set_fmpz3(x,a,N,d)==0);
    ADF_CHECK(adf_fball_binom_tight(y,&w,x,4)==0 && fmpz_equal_ui(y->H,2));
    ADF_CHECK(adf_fball_binom(y,NULL,x,4)==0 && fmpz_is_one(y->H));
    ADF_CHECK(adf_fball_binom_tight(y,NULL,x,0)==0 && adf_fball_is_exact(y));
    ADF_CHECK(fmpz_is_one(y->A));
    /* Thousands of bits in both centre and radius; binom(a,2)=a*(a-1)/2. */
    fmpz_one(a); fmpz_mul_2exp(a,a,4096); fmpz_add_ui(a,a,3);
    fmpz_mul_2exp(N,N,10000);
    ADF_CHECK(adf_fball_set_fmpz3(x,a,N,d)==0);
    fmpz_sub_ui(C,a,1); fmpz_mul(C,C,a); fmpz_divexact_ui(C,C,2);
    for (int tight=0;tight<2;tight++)
    {
        ADF_CHECK(binom_call(tight,y,NULL,x,2)==0);
        ADF_CHECK(fmpz_equal(y->A,C));
        fmpz_mul_2exp(C,y->H,1); ADF_CHECK(fmpz_equal(C,N));
        fmpz_sub_ui(C,a,1); fmpz_mul(C,C,a); fmpz_divexact_ui(C,C,2);
        adf_fball_set(old,x);
        ulong cap=tight ? ADF_BINOM_TIGHT_K_MAX : ADF_BINOM_K_MAX;
        ADF_CHECK(binom_call(tight,x,&w,x,cap+1)==ADF_LIMIT);
        ADF_CHECK(adf_fball_identical(x,old) && adf_place_equal(w,mark));
        adf_fball_set_si(y,-1);
        ADF_CHECK(binom_call(tight,x,NULL,y,cap)==0);
        ADF_CHECK(adf_fball_is_exact(x) && fmpz_equal_si(x->A,cap%2 ? -1 : 1));
        adf_fball_set(x,old);
    }
    fmpz_clear(a); fmpz_clear(N); fmpz_clear(d); fmpz_clear(C);
    adf_fball_clear(x); adf_fball_clear(y); adf_fball_clear(old);
}
ADF_TEST(binomial_local_and_mixed)
{
    adf_modctx_struct *ctx=NULL;
    const ulong blocks[]={8};
    adf_fball_t x,local,y,old;
    fmpz_t a,N,d;
    adf_fball_init(x); adf_fball_init(local); adf_fball_init(y); adf_fball_init(old);
    fmpz_init(a); fmpz_init(N); fmpz_init(d);
    ADF_CHECK(adf_modctx_new_blocks(&ctx,blocks,1)==0);
    fmpz_zero(a); fmpz_set_ui(N,8); fmpz_one(d);
    ADF_CHECK(adf_fball_set_fmpz3(x,a,N,d)==0);
    ADF_CHECK(adf_fball_set_local(local,x,ctx)==0);
    ADF_CHECK(adf_fball_binom_tight(local,NULL,local,4)==0);
    ADF_CHECK(local->backend==ADF_GLOBAL && fmpz_equal_ui(local->H,2));
    /* The mixed input (1/2) Zhat contains the domain point 0 and the nonintegral point 1/2.
       More integral precision does not repair a missing-domain certificate. */
    fmpz_one(N); fmpz_set_ui(d,2);
    ADF_CHECK(adf_fball_set_fmpz3(x,a,N,d)==0);
    adf_fball_set(old,x);
    ADF_CHECK(adf_fball_binom(x,NULL,x,0)==ADF_NOT_DETERMINED);
    ADF_CHECK(adf_fball_identical(x,old));
    ADF_CHECK(adf_fball_binom_tight(x,NULL,x,ADF_BINOM_TIGHT_K_MAX+1)==ADF_LIMIT);
    ADF_CHECK(adf_fball_identical(x,old));
    fmpz_clear(a); fmpz_clear(N); fmpz_clear(d);
    adf_fball_clear(x); adf_fball_clear(local); adf_fball_clear(y); adf_fball_clear(old);
    adf_modctx_free(ctx);
}

static int power_call(int policy, adf_ucoset_t y, adf_place_t *w,
                      const adf_ucoset_t a, const adf_fball_t x)
{
    if (policy==0) return adf_ucoset_profpow(y,w,a,x);
    if (policy==1) return adf_ucoset_profpow_coarse(y,w,a,x);
    return adf_ucoset_profpow_fine(y,w,a,x);
}
ADF_TEST(power_vectors)
{
    jsonl_file *f=NULL;
    jsonl_error_t err;
    fmpz_t c,N,e,M,d,C,R;
    adf_ucoset_t a,y,old,want;
    adf_fball_t x;
    adf_place_t w,mark=sentinel();
    ADF_CHECK(jsonl_open("tests/ref/vectors/f-slice13/power.jsonl",&f,&err));
    if (!f) return;
    fmpz_init(c); fmpz_init(N); fmpz_init(e); fmpz_init(M);
    fmpz_init(d); fmpz_init(C); fmpz_init(R);
    adf_ucoset_init(a); adf_ucoset_init(y); adf_ucoset_init(old); adf_ucoset_init(want);
    adf_fball_init(x);
    for (size_t i=0;i<jsonl_count(f);i++)
    {
        const jsonl_value *r=jsonl_record(f,i);
        int policy=atoi(field(r,"policy")), st=atoi(field(r,"status"));
        ADF_CHECK(fmpz_set_str(c,field(r,"c"),10)==0);
        ADF_CHECK(fmpz_set_str(N,field(r,"N"),10)==0);
        ADF_CHECK(fmpz_set_str(e,field(r,"e"),10)==0);
        ADF_CHECK(fmpz_set_str(M,field(r,"M"),10)==0);
        ADF_CHECK(fmpz_set_str(d,field(r,"d"),10)==0);
        ADF_CHECK(fmpz_set_str(C,field(r,"C"),10)==0);
        ADF_CHECK(fmpz_set_str(R,field(r,"R"),10)==0);
        ADF_CHECK(adf_ucoset_set_fmpz2(a,c,N)==0);
        ADF_CHECK(adf_fball_set_fmpz3(x,e,M,d)==0);
        ADF_CHECK(adf_ucoset_set_fmpz2(want,C,R)==0);
        for (int alias=0;alias<2;alias++)
        {
            adf_ucoset_set(old,a); adf_ucoset_minus_one(y);
            adf_ucoset_ptr out=alias ? a : y;
            w=mark;
            ADF_CHECK_MSG(power_call(policy,out,&w,a,x)==st,"power row %zu",i);
            ADF_CHECK(adf_place_equal(w,mark));
            ADF_CHECK(adf_ucoset_identical(out,st==0 ? want : (alias ? old : y)));
            if (!alias && st!=0) ADF_CHECK(fmpz_is_zero(y->N) && fmpz_equal_si(y->c,-1));
            if (st==0) ADF_CHECK(adf_ucoset_is_normal(out));
        }
    }
    printf("power fixture rows: %zu\n",jsonl_count(f));
    jsonl_close(f);
    fmpz_clear(c); fmpz_clear(N); fmpz_clear(e); fmpz_clear(M);
    fmpz_clear(d); fmpz_clear(C); fmpz_clear(R);
    adf_ucoset_clear(a); adf_ucoset_clear(y); adf_ucoset_clear(old); adf_ucoset_clear(want);
    adf_fball_clear(x);
}
ADF_TEST(power_plan_boundary_huge_statuses)
{
    fmpz_t c,N,e,M,d;
    adf_ucoset_t a,y,old,integer;
    adf_fball_t x;
    adf_place_t w=sentinel(),mark=w;
    fmpz_init(c); fmpz_init(N); fmpz_init(e); fmpz_init(M); fmpz_init(d);
    adf_ucoset_init(a); adf_ucoset_init(y); adf_ucoset_init(old); adf_ucoset_init(integer);
    adf_fball_init(x);
    fmpz_set_ui(c,2); fmpz_set_ui(N,5); fmpz_set_ui(e,2); fmpz_set_ui(M,4); fmpz_one(d);
    ADF_CHECK(adf_ucoset_set_fmpz2(a,c,N)==0);
    ADF_CHECK(adf_fball_set_fmpz3(x,e,M,d)==0);
    ADF_CHECK(adf_ucoset_profpow_fine(y,&w,a,x)==0);
    ADF_CHECK(fmpz_equal_ui(y->c,49) && fmpz_equal_ui(y->N,120));
    ADF_CHECK(adf_ucoset_profpow(y,NULL,a,x)==0 && fmpz_equal_ui(y->N,5));
    fmpz_set_ui(M,2); fmpz_zero(e);
    ADF_CHECK(adf_fball_set_fmpz3(x,e,M,d)==0);
    adf_ucoset_set(old,y);
    ADF_CHECK(adf_ucoset_profpow(y,&w,a,x)==ADF_NOT_DETERMINED);
    ADF_CHECK(adf_ucoset_identical(y,old) && adf_place_equal(w,mark));
    ADF_CHECK(adf_ucoset_profpow_coarse(y,NULL,a,x)==0 && fmpz_is_one(y->N));
    ADF_CHECK(adf_ucoset_profpow_fine(y,NULL,a,x)==0 && fmpz_equal_ui(y->N,24));
    /* Same profinite base residue, exponent points 0 and 2 give residues 1 and 4 mod 5. */
    adf_ucoset_pow(y,a,0); ADF_CHECK(fmpz_is_one(y->c) && fmpz_is_zero(y->N));
    adf_ucoset_pow(y,a,2); ADF_CHECK(fmpz_equal_ui(y->c,4));
    for (slong k=-6;k<=6;k++)
    {
        adf_fball_set_si(x,k);
        adf_ucoset_pow(integer,a,k);
        ADF_CHECK(adf_ucoset_profpow(y,NULL,a,x)==0 && adf_ucoset_identical(y,integer));
        adf_ucoset_pow_tight(integer,a,k);
        ADF_CHECK(adf_ucoset_profpow_fine(y,NULL,a,x)==0 && adf_ucoset_identical(y,integer));
    }
    fmpz_one(e); fmpz_mul_2exp(e,e,4096); fmpz_add_ui(e,e,1);
    adf_fball_set_fmpz(x,e);
    ADF_CHECK(adf_ucoset_profpow(y,NULL,a,x)==0 && fmpz_equal_ui(y->c,2));
    fmpz_neg(e,e); adf_fball_set_fmpz(x,e);
    ADF_CHECK(adf_ucoset_profpow(y,NULL,a,x)==0 && fmpz_equal_ui(y->c,3));
    /* A thousands-bit N is admitted by all policies: a=1, e=1, M a multiple of N.
       U(N)^M is trivial mod N; g=1 makes the finest modulus N. */
    fmpz_one(c); fmpz_one(N); fmpz_mul_2exp(N,N,4096);
    fmpz_one(e); fmpz_set(M,N);
    ADF_CHECK(adf_ucoset_set_fmpz2(a,c,N)==0);
    ADF_CHECK(adf_fball_set_fmpz3(x,e,M,d)==0);
    for (int policy=0;policy<3;policy++)
        ADF_CHECK(power_call(policy,y,NULL,a,x)==0 && fmpz_equal(y->N,N) && fmpz_is_one(y->c));
    /* g boundary: U(1)^256 has modulus 2^10*3*5*17*257=67107840. */
    fmpz_one(N); ADF_CHECK(adf_ucoset_set_fmpz2(a,c,N)==0);
    adf_fball_set_si(x,256);
    ADF_CHECK(adf_ucoset_profpow_fine(y,NULL,a,x)==0 && fmpz_equal_ui(y->N,67107840));
    adf_fball_set_si(x,257); adf_ucoset_set(old,y);
    ADF_CHECK(adf_ucoset_profpow_fine(y,&w,a,x)==ADF_LIMIT);
    ADF_CHECK(adf_ucoset_identical(y,old) && adf_place_equal(w,mark));
    for (int policy=0;policy<3;policy++) for (int mixed=0;mixed<2;mixed++)
    {
        fmpz_set_ui(e,mixed ? 0 : 1); fmpz_set_ui(M,mixed ? 1 : 2); fmpz_set_ui(d,2);
        ADF_CHECK(adf_fball_set_fmpz3(x,e,M,d)==0);
        adf_ucoset_set(old,a);
        ADF_CHECK(power_call(policy,a,&w,a,x)==(mixed ? ADF_NOT_DETERMINED : ADF_DOMAIN));
        ADF_CHECK(adf_ucoset_identical(a,old) && adf_place_equal(w,mark));
    }
    fmpz_clear(c); fmpz_clear(N); fmpz_clear(e); fmpz_clear(M); fmpz_clear(d);
    adf_ucoset_clear(a); adf_ucoset_clear(y); adf_ucoset_clear(old); adf_ucoset_clear(integer);
    adf_fball_clear(x);
}

ADF_TEST(haar_and_existing_content)
{
    fmpz_t A,H,d;
    fmpq_t want,content;
    adf_rat_t volume;
    adf_fball_t x,local;
    adf_idele_t idele;
    adf_modctx_struct *ctx=NULL;
    const ulong blocks[]={8};
    fmpz_init(A); fmpz_init(H); fmpz_init(d);
    adf_rat_init(volume); fmpq_init(want); fmpq_init(content);
    adf_fball_init(x); adf_fball_init(local); adf_idele_init(idele);
    for (slong a=-5;a<=5;a++) for (ulong h=0;h<=12;h++) for (ulong den=1;den<=7;den++)
    {
        fmpz_set_si(A,a); fmpz_set_ui(H,h); fmpz_set_ui(d,den);
        ADF_CHECK(adf_fball_set_fmpz3(x,A,H,d)==0);
        if (h) fmpq_set_ui(want,den,h);
        else fmpq_zero(want);
        adf_rat_set_si(volume,99);
        adf_fball_haar_volume(volume,x);
        ADF_CHECK(fmpq_equal(volume->q,want) && fmpq_is_canonical(volume->q));
    }
    /* Nonintegral radius and thousands-bit radius/denominator. */
    fmpz_one(H); fmpz_mul_2exp(H,H,4096); fmpz_add_ui(H,H,1);
    fmpz_one(d); fmpz_mul_2exp(d,d,5000); fmpz_zero(A);
    ADF_CHECK(adf_fball_set_fmpz3(x,A,H,d)==0);
    fmpq_set_fmpz_frac(want,d,H);
    adf_fball_haar_volume(volume,x); ADF_CHECK(fmpq_equal(volume->q,want));
    fmpz_zero(A); fmpz_set_ui(H,8); fmpz_set_ui(d,3);
    ADF_CHECK(adf_fball_set_fmpz3(x,A,H,d)==0);
    ADF_CHECK(adf_modctx_new_blocks(&ctx,blocks,1)==0);
    ADF_CHECK(adf_fball_set_local(local,x,ctx)==0);
    fmpq_set_si(want,3,8);
    adf_fball_haar_volume(volume,local); ADF_CHECK(fmpq_equal(volume->q,want));
    /* Existing accessor: content r, independently of real coordinate and unit. P10:202-208. */
    fmpq_set_si(idele->r,15,14); arb_set_si(idele->inf,-7); adf_ucoset_minus_one(&idele->u);
    adf_idele_content(content,idele);
    fmpq_set_si(want,15,14); ADF_CHECK(fmpq_equal(content,want));
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
    adf_rat_clear(volume); fmpq_clear(want); fmpq_clear(content);
    adf_fball_clear(x); adf_fball_clear(local); adf_idele_clear(idele);
    adf_modctx_free(ctx);
}

ADF_TEST(volume_vectors)
{
    jsonl_file *f=NULL;
    jsonl_error_t err;
    fmpz_t a,H,d,num,den;
    adf_fball_t x;
    adf_rat_t volume;
    ADF_CHECK(jsonl_open("tests/ref/vectors/f-slice13/volume.jsonl",&f,&err));
    if (!f) return;
    fmpz_init(a); fmpz_init(H); fmpz_init(d); fmpz_init(num); fmpz_init(den);
    adf_fball_init(x); adf_rat_init(volume);
    for (size_t i=0;i<jsonl_count(f);i++)
    {
        const jsonl_value *r=jsonl_record(f,i);
        ADF_CHECK(fmpz_set_str(a,field(r,"a"),10)==0);
        ADF_CHECK(fmpz_set_str(H,field(r,"H"),10)==0);
        ADF_CHECK(fmpz_set_str(d,field(r,"d"),10)==0);
        ADF_CHECK(fmpz_set_str(num,field(r,"num"),10)==0);
        ADF_CHECK(fmpz_set_str(den,field(r,"den"),10)==0);
        ADF_CHECK(adf_fball_set_fmpz3(x,a,H,d)==0);
        adf_fball_haar_volume(volume,x);
        ADF_CHECK(fmpz_equal(fmpq_numref(volume->q),num) && fmpz_equal(fmpq_denref(volume->q),den));
    }
    printf("volume fixture rows: %zu\n",jsonl_count(f));
    jsonl_close(f);
    fmpz_clear(a); fmpz_clear(H); fmpz_clear(d); fmpz_clear(num); fmpz_clear(den);
    adf_fball_clear(x); adf_rat_clear(volume);
}
static int cyclo_call(int inverse, fmpz_t j, adf_place_t *w,
                      const adf_idclass_t x, const fmpz_t n)
{
    return inverse ? adf_idclass_cyclo_exp_uinv(j,w,x,n) : adf_idclass_cyclo_exp_u(j,w,x,n);
}
ADF_TEST(cyclotomic_vectors)
{
    jsonl_file *f=NULL;
    jsonl_error_t err;
    fmpz_t c,N,n,j,want,old;
    adf_idclass_t x;
    adf_place_t w,mark=sentinel();
    ADF_CHECK(jsonl_open("tests/ref/vectors/f-slice13/cyclotomic.jsonl",&f,&err));
    if (!f) return;
    fmpz_init(c); fmpz_init(N); fmpz_init(n); fmpz_init(j); fmpz_init(want); fmpz_init(old);
    adf_idclass_init(x);
    arb_set_si(x->t,17); /* Real coordinate acts trivially. */
    for (size_t i=0;i<jsonl_count(f);i++)
    {
        const jsonl_value *r=jsonl_record(f,i);
        int inverse=atoi(field(r,"inverse")), st=atoi(field(r,"status"));
        ADF_CHECK(fmpz_set_str(c,field(r,"c"),10)==0);
        ADF_CHECK(fmpz_set_str(N,field(r,"N"),10)==0);
        ADF_CHECK(fmpz_set_str(n,field(r,"n"),10)==0);
        ADF_CHECK(fmpz_set_str(want,field(r,"j"),10)==0);
        ADF_CHECK(adf_ucoset_set_fmpz2(&x->u,c,N)==0);
        fmpz_set(old,n);
        for (int alias=0;alias<2;alias++)
        {
            fmpz_set_ui(j,99); w=mark;
            fmpz *out=alias ? n : j;
            ADF_CHECK_MSG(cyclo_call(inverse,out,&w,x,n)==st,"cyclotomic row %zu",i);
            ADF_CHECK(adf_place_equal(w,mark));
            ADF_CHECK(fmpz_equal(out,st==0 ? want : (alias ? old : want)));
        }
    }
    printf("cyclotomic fixture rows: %zu\n",jsonl_count(f));
    jsonl_close(f);
    fmpz_clear(c); fmpz_clear(N); fmpz_clear(n); fmpz_clear(j); fmpz_clear(want); fmpz_clear(old);
    adf_idclass_clear(x);
}
ADF_TEST(cyclotomic_spec_boundary_huge)
{
    adf_idclass_t x;
    adf_idele_t idele;
    fmpz_t c,N,n,j;
    adf_place_t w=sentinel(),mark=w;
    adf_idclass_init(x); adf_idele_init(idele);
    fmpz_init(c); fmpz_init(N); fmpz_init(n); fmpz_init(j);
    /* Specification vector: p=3, u'=1 at 3 and 3^-1 elsewhere.
       c=19 mod 63 is 1 mod 9 and 5 mod 7. Catalogue.md:324-326 and :339-341. */
    fmpz_set_ui(c,19); fmpz_set_ui(N,63);
    ADF_CHECK(adf_ucoset_set_fmpz2(&idele->u,c,N)==0);
    fmpq_set_si(idele->r,3,1);
    ADF_CHECK(adf_idclass_set_idele(x,idele,64)==0);
    fmpz_set_ui(n,7);
    ADF_CHECK(adf_idclass_cyclo_exp_uinv(j,&w,x,n)==0 && fmpz_equal_ui(j,3));
    ADF_CHECK(adf_idclass_cyclo_exp_u(j,NULL,x,n)==0 && fmpz_equal_ui(j,5));
    fmpz_set_ui(n,9);
    ADF_CHECK(adf_idclass_cyclo_exp_uinv(j,NULL,x,n)==0 && fmpz_is_one(j));
    ADF_CHECK(adf_idclass_cyclo_exp_u(j,NULL,x,n)==0 && fmpz_is_one(j));
    /* Removing one 3-adic digit admits 1 and 4 mod 9 (inverse exponents 1 and 7). */
    fmpz_one(c); fmpz_set_ui(N,3);
    ADF_CHECK(adf_ucoset_set_fmpz2(&x->u,c,N)==0);
    for (int inverse=0;inverse<2;inverse++)
    {
        fmpz_set_ui(j,99);
        ADF_CHECK(cyclo_call(inverse,j,&w,x,n)==ADF_NOT_DETERMINED && fmpz_equal_ui(j,99));
        ADF_CHECK(adf_place_equal(w,mark));
    }
    /* The canonical n=6 exception needs an odd lift: 2 mod 3 acts by exponent 5 mod 6. */
    fmpz_set_ui(c,2);
    ADF_CHECK(adf_ucoset_set_fmpz2(&x->u,c,N)==0);
    fmpz_set_ui(n,6);
    ADF_CHECK(adf_idclass_cyclo_exp_u(j,NULL,x,n)==0 && fmpz_equal_ui(j,5));
    ADF_CHECK(adf_idclass_cyclo_exp_uinv(j,NULL,x,n)==0 && fmpz_equal_ui(j,5));
    fmpz_one(N); fmpz_mul_2exp(N,N,4096); fmpz_add_ui(N,N,1);
    fmpz_sub_ui(c,N,1); fmpz_set(n,N);
    ADF_CHECK(adf_ucoset_set_fmpz2(&x->u,c,N)==0);
    for (int inverse=0;inverse<2;inverse++)
    {
        ADF_CHECK(cyclo_call(inverse,j,NULL,x,n)==0 && fmpz_equal(j,c));
        fmpz_mul_2exp(n,N,1); /* Original 2N, canonical target N. c=N-1 is even. */
        ADF_CHECK(cyclo_call(inverse,n,NULL,x,n)==0);
        fmpz_mul_2exp(j,N,1); fmpz_sub_ui(j,j,1);
        ADF_CHECK(fmpz_equal(n,j));
        fmpz_set(n,N);
    }
    fmpz_clear(c); fmpz_clear(N); fmpz_clear(n); fmpz_clear(j);
    adf_idclass_clear(x); adf_idele_clear(idele);
}

#ifdef ADF_CHECK_INVARIANTS
/* Every object has its normal init/clear pair, including child paths which should abort.
   The parent checks SIGABRT; exit 42 means a missing entry check. */
ADF_TEST(catalogue_debug_entries)
{
    const char *names[]={"adf_fball_binom", "adf_fball_binom_tight",
        "adf_ucoset_profpow", "adf_ucoset_profpow_coarse", "adf_ucoset_profpow_fine",
        "adf_ucoset_profpow", "adf_ucoset_profpow_coarse", "adf_ucoset_profpow_fine",
        "adf_idclass_cyclo_exp_u", "adf_idclass_cyclo_exp_uinv",
        "adf_idclass_cyclo_exp_u", "adf_idclass_cyclo_exp_uinv", "adf_fball_haar_volume"};
    for (int which=0;which<13;which++)
    {
        int fd[2];
        int opened=pipe(fd);
        ADF_CHECK(opened==0);
        if (opened!=0) return;
        pid_t child=fork();
        ADF_CHECK(child>=0);
        if (child==0)
        {
            adf_fball_t x,y;
            adf_ucoset_t a,b;
            adf_idclass_t cls;
            adf_rat_t vol;
            fmpz_t j,n;
            close(fd[0]);
            (void)dup2(fd[1],STDERR_FILENO); close(fd[1]);
            adf_fball_init(x); adf_fball_init(y);
            adf_ucoset_init(a); adf_ucoset_init(b); adf_idclass_init(cls); adf_rat_init(vol);
            fmpz_init(j); fmpz_init(n); fmpz_set_ui(n,7);
            if (which<2 || (which>=5 && which<8) || which==12) fmpz_zero(x->d);
            if (which>=2 && which<5) fmpz_set_si(a->N,-1);
            if (which==8 || which==9) arb_zero(cls->t);
            if (which==10 || which==11) fmpz_set_si(cls->u.N,-1);
            if (which==0) (void)adf_fball_binom(y,NULL,x,ADF_BINOM_K_MAX+1);
            else if (which==1) (void)adf_fball_binom_tight(y,NULL,x,ADF_BINOM_TIGHT_K_MAX+1);
            else if (which>=2 && which<8) (void)power_call((which-2)%3,b,NULL,a,x);
            else if (which<12) (void)cyclo_call(which%2,j,NULL,cls,n);
            else adf_fball_haar_volume(vol,x);
            fmpz_clear(j); fmpz_clear(n);
            adf_fball_clear(x); adf_fball_clear(y);
            adf_ucoset_clear(a); adf_ucoset_clear(b); adf_idclass_clear(cls); adf_rat_clear(vol);
            _exit(42);
        }
        if (child>0)
        {
            int status=0;
            char message[2048];
            close(fd[1]);
            ssize_t len=read(fd[0],message,sizeof(message)-1);
            close(fd[0]);
            message[len>0 ? (size_t)len : 0]='\0';
            ADF_CHECK(waitpid(child,&status,0)==child);
            ADF_CHECK(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
            ADF_CHECK(strstr(message,names[which])!=NULL);
        }
        else { close(fd[0]); close(fd[1]); }
    }
}
#endif
