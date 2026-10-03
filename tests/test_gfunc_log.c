/* Idele Log wrappers: IL1-IL8 of docs/design/idele-log.md. Exact integer oracle at H=5;
   real endpoint oracle mpmath at 800 bits, enclosing dyadics at 500 fractional bits.
   A wrong field, residue set, status, place, changed failed output, or missed endpoint fails. */
#include <limits.h>
#include <string.h>
#include <adelefeld.h>
#include "support/jsonl.h"
#define main gfunc_log_test_main
#include "test_runner.h"
#undef main

static adf_place_t prime(ulong p)
{
    adf_place_t v;
    ADF_CHECK(adf_place_prime(&v, p) == ADF_OK);
    return v;
}
static void mk(adf_idele_t x, slong a, ulong b, slong c, ulong M)
{
    arb_set_si(x->inf, 2);
    fmpq_set_si(x->r, a, b);
    fmpz_set_si(x->u.c, c); fmpz_set_ui(x->u.N, M);
    ADF_CHECK(adf_idele_is_canonical(x));
}
ADF_TEST(conservative_Log)
{
    adf_idele_t x;
    adf_adele_t y;
    adf_place_t w = prime(97);
    adf_idele_init(x); adf_adele_init(y);
    mk(x, 4, 1, 1, 9); arb_one(x->inf);
    ADF_CHECK(adf_idele_Log(y, &w, x, 64) == ADF_OK);
    ADF_CHECK(arb_is_zero(y->inf));
    ADF_CHECK(fmpz_is_zero(y->fin.A) && fmpz_equal_ui(y->fin.H, 4) && fmpz_is_one(y->fin.d));
    ADF_CHECK(adf_place_equal(w, prime(97)));
    adf_idele_clear(x); adf_adele_clear(y);
}
ADF_TEST(other_slice_A_calls)
{
    adf_idele_t x;
    adf_adele_t y;
    adf_sball_t s;
    adf_idele_init(x); adf_adele_init(y); adf_sball_init(s);
    mk(x, 4, 1, 1, 9); arb_set_si(x->inf, -1);
    ADF_CHECK(adf_idele_log_abs(y, NULL, x, 64) == ADF_OK);
    ADF_CHECK(adf_idele_Log_at(s, NULL, x, prime(3), 5) == ADF_OK);
    ADF_CHECK(adf_idele_log_abs_at(s, NULL, x, adf_place_inf(), 64) == ADF_OK);
    adf_idele_clear(x); adf_adele_clear(y); adf_sball_clear(s);
}

static const jsonl_value *field(const jsonl_value *r, const char *key)
{
    const jsonl_value *v = NULL;
    jsonl_error_t e;
    ADF_CHECK_MSG(jsonl_field(r, key, &v, &e), "%s", jsonl_error_message(&e));
    return v;
}
static const char *integer(const jsonl_value *v)
{
    const char *s = "0";
    jsonl_error_t e;
    ADF_CHECK_MSG(jsonl_int_text_or_string(v, &s, &e), "%s", jsonl_error_message(&e));
    return s;
}
static slong number(const jsonl_value *r, const char *k) { return strtol(integer(field(r,k)), NULL, 10); }
static jsonl_file *vectors(const char *path)
{
    jsonl_file *f = NULL;
    jsonl_error_t e;
    ADF_CHECK_MSG(jsonl_open(path, &f, &e), "%s", jsonl_error_message(&e));
    return f;
}
static void from_row(adf_idele_t x, const jsonl_value *row)
{
    jsonl_error_t e;
    const jsonl_value *r = field(row,"r");
    arb_set_si(x->inf, 2);
    fmpz_set_str(fmpq_numref(x->r), integer(jsonl_at(r,0,&e)), 10);
    fmpz_set_str(fmpq_denref(x->r), integer(jsonl_at(r,1,&e)), 10);
    fmpz_set_str(x->u.c, integer(field(row,"c")), 10);
    fmpz_set_str(x->u.N, integer(field(row,"M")), 10);
    ADF_CHECK(adf_idele_is_canonical(x));
}
static void expected(adf_lball_t l, const jsonl_value *row, adf_place_t p)
{
    adf_rat_t q;
    jsonl_error_t e;
    int exact = 0;
    const jsonl_value *r = field(row,"expected");
    adf_rat_init(q);
    fmpz_set_str(fmpq_numref(q->q), integer(field(r,"center")), 10);
    ADF_CHECK(jsonl_bool(field(r,"exact"), &exact, &e));
    if (exact) ADF_CHECK(adf_lball_set_rat(l,p,q) == ADF_OK);
    else ADF_CHECK(adf_lball_set_rat_ball(l,p,q,number(r,"exponent")) == ADF_OK);
    adf_rat_clear(q);
}
ADF_TEST(local_selection)
{
    jsonl_file *f = vectors("tests/ref/vectors/f-slice11/local.jsonl");
    adf_idele_t x, original;
    adf_sball_t s;
    adf_lball_t e, l;
    unsigned long image_rows=0, membership=0;
    if (!f) return;
    adf_idele_init(x); adf_idele_init(original); adf_sball_init(s);
    adf_lball_init(e); adf_lball_init(l);
    for (size_t i=0; i<jsonl_count(f); i++)
    {
        const jsonl_value *row = jsonl_record(f,i), *im = NULL;
        jsonl_error_t err;
        adf_place_t p = prime((ulong)number(row,"p")), w = prime(97);
        from_row(x,row); adf_idele_set(original,x); expected(e,row,p);
        int st = adf_idele_Log_at(s,&w,x,p,number(row,"requested"));
        ADF_CHECK_MSG(st==ADF_OK,"local row %zu status %d",i+1,st);
        if (st!=ADF_OK) continue;
        ADF_CHECK(adf_sball_num_places(s)==1 && adf_sball_arch(s)==ADF_ARCH_NONE);
        ADF_CHECK(adf_sball_get_lball(l,s,p)==ADF_OK);
        ADF_CHECK_MSG(adf_lball_identical(l,e),"local row %zu fields",i+1);
        ADF_CHECK(adf_place_equal(w,prime(97)) && adf_idele_identical(x,original));
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,p,number(row,"requested"))==ADF_OK);
        if (jsonl_field(row,"image",&im,&err))
        {
            ulong pp = adf_place_prime_get(p), mod=1, step=1, centre;
            adf_rat_t q;
            adf_rat_init(q);
            for (slong j=0;j<number(row,"H");j++) mod*=pp;
            for (slong j=0;j<l->N;j++) step*=pp;
            ADF_CHECK(adf_lball_get_center(q,l)==ADF_OK);
            centre=fmpz_get_ui(fmpq_numref(q->q));
            unsigned char *set = calloc(mod,1);
            ADF_CHECK(set!=NULL);
            if (set)
            {
                for (size_t j=0;j<jsonl_size(im);j++)
                    set[strtoul(integer(jsonl_at(im,j,&err)),NULL,10)]=1;
                for (ulong z=0;z<mod;z++)
                {
                    ADF_CHECK_MSG(set[z]==(z%step==centre),"image row %zu residue %lu",i+1,z);
                    membership++;
                }
                free(set);
            }
            adf_rat_clear(q); image_rows++;
        }
    }
    printf("local_selection: %zu rows, %lu complete H=5 images, %lu memberships\n",
           jsonl_count(f),image_rows,membership);
    adf_idele_clear(x); adf_idele_clear(original); adf_sball_clear(s);
    adf_lball_clear(e); adf_lball_clear(l); jsonl_close(f);
}

ADF_TEST(real_selection)
{
    jsonl_file *f = vectors("tests/ref/vectors/f-slice11/real.jsonl");
    adf_idele_t x;
    adf_adele_t y, before;
    adf_sball_t s;
    arb_t r;
    arf_t lo,hi;
    fmpz_t z,exp;
    const slong ps[]={-2,2,5,64,256};
    if (!f) return;
    adf_idele_init(x); adf_adele_init(y); adf_adele_init(before); adf_sball_init(s);
    arb_init(r); arf_init(lo); arf_init(hi); fmpz_init(z); fmpz_init(exp);
    for (size_t i=0;i<jsonl_count(f);i++)
    {
        const jsonl_value *row=jsonl_record(f,i);
        mk(x,4,1,1,9);
        arb_set_si(x->inf,number(row,"m")); arb_mul_2exp_si(x->inf,x->inf,number(row,"e"));
        mag_set_ui_2exp_si(arb_radref(x->inf),(ulong)number(row,"rm"),number(row,"e"));
        fmpz_set_si(exp,number(row,"bound_exp"));
        fmpz_set_str(z,integer(field(row,"lo")),10); arf_set_fmpz_2exp(lo,z,exp);
        fmpz_set_str(z,integer(field(row,"hi")),10); arf_set_fmpz_2exp(hi,z,exp);
        for (size_t j=0;j<sizeof(ps)/sizeof(ps[0]);j++)
        {
            adf_place_t w=prime(97);
            int want=number(row,"m")>0?ADF_OK:ADF_DOMAIN;
            adf_adele_set(before,y);
            ADF_CHECK(adf_idele_Log(y,&w,x,ps[j])==want);
            if (want==ADF_OK)
            {
                ADF_CHECK(adf_real_log(r,x->inf,ps[j])==ADF_OK && arb_equal(r,y->inf));
                ADF_CHECK(arb_contains_arf(y->inf,lo) && arb_contains_arf(y->inf,hi));
                ADF_CHECK(adf_place_equal(w,prime(97)));
            }
            else ADF_CHECK(adf_adele_identical(before,y) && adf_place_is_archimedean(w));
            ADF_CHECK(adf_idele_Log_at(s,NULL,x,adf_place_inf(),ps[j])==want);
            if (want==ADF_OK) ADF_CHECK(arb_equal(acb_realref(s->inf),y->inf) && s->len==0);
            w=prime(97);
            ADF_CHECK(adf_idele_log_abs(y,&w,x,ps[j])==ADF_OK);
            ADF_CHECK(adf_real_log_abs(r,x->inf,ps[j])==ADF_OK && arb_equal(r,y->inf));
            ADF_CHECK(arb_contains_arf(y->inf,lo) && arb_contains_arf(y->inf,hi));
            ADF_CHECK(fmpz_is_zero(y->fin.A) && fmpz_equal_ui(y->fin.H,4));
            ADF_CHECK(adf_place_equal(w,prime(97)));
            ADF_CHECK(adf_idele_log_abs_at(s,&w,x,adf_place_inf(),ps[j])==ADF_OK);
            ADF_CHECK(arb_equal(acb_realref(s->inf),r) && s->len==0 && adf_place_equal(w,prime(97)));
        }
    }
    printf("real_selection: 6 intervals, 5 precisions, 120 wrapper calls, mpmath 800-bit endpoints\n");
    adf_idele_clear(x); adf_adele_clear(y); adf_adele_clear(before); adf_sball_clear(s);
    arb_clear(r); arf_clear(lo); arf_clear(hi); fmpz_clear(z); fmpz_clear(exp); jsonl_close(f);
}

/* Whole-struct byte checks include padding and inactive small arf mantissa slots.
   Define those bytes before the snapshot; active fields and comparisons are unchanged.
   The small-arf layout is refs/src/flint-3.0.1/arf.rst:43-54. */
static void arf_inactive(arf_t x)
{
    if (ARF_SIZE(x)<=ARF_NOPTR_LIMBS)
        for (slong i=ARF_SIZE(x);i<ARF_NOPTR_LIMBS;i++) ARF_NOPTR_D(x)[i]=0;
}
static void sball_padding(adf_sball_t s)
{
    size_t a=offsetof(adf_sball_struct,arch)+sizeof(s->arch);
    size_t b=offsetof(adf_sball_struct,inf);
    memset((unsigned char *)s+a,0,b-a);
    arf_inactive(arb_midref(acb_realref(s->inf)));
    arf_inactive(arb_midref(acb_imagref(s->inf)));
}
static void adele_padding(adf_adele_t y)
{
    size_t a=offsetof(adf_fball_struct,backend)+sizeof(y->fin.backend);
    size_t b=offsetof(adf_fball_struct,mctx);
    memset((unsigned char *)&y->fin+a,0,b-a);
    arf_inactive(arb_midref(y->inf));
}
static void fail_at(adf_sball_t s, const adf_idele_t x, adf_place_t v, slong N, int want, int absval)
{
    adf_sball_t before;
    unsigned char raw[sizeof(adf_sball_struct)];
    adf_place_t w=prime(97);
    adf_sball_init(before); sball_padding(s); adf_sball_set(before,s); memcpy(raw,s,sizeof(raw));
    int st=absval?adf_idele_log_abs_at(s,&w,x,v,N):adf_idele_Log_at(s,&w,x,v,N);
    ADF_CHECK(st==want && adf_place_equal(w,v));
    ADF_CHECK(adf_sball_identical(s,before) && memcmp(raw,s,sizeof(raw))==0);
    ADF_CHECK((absval?adf_idele_log_abs_at(s,NULL,x,v,N):adf_idele_Log_at(s,NULL,x,v,N))==want);
    adf_sball_clear(before);
}
ADF_TEST(slice_A_status_and_limits)
{
    adf_idele_t x;
    adf_adele_t y,before;
    adf_sball_t s;
    adf_idele_init(x); adf_adele_init(y); adf_adele_init(before); adf_sball_init(s);
    mk(x,4,1,1,9);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(3),5)==ADF_OK);
    fail_at(s,x,prime(5),WORD_MIN,ADF_LIMIT,0);
    fail_at(s,x,prime(3),-ADF_LBALL_EXP_MAX-1,ADF_LIMIT,0);
    fail_at(s,x,prime(3),WORD_MAX,ADF_UNSUPPORTED,1);
    fail_at(s,x,adf_place_inf(),ADF_REAL_PREC_MAX+1,ADF_LIMIT,0);
    fail_at(s,x,adf_place_inf(),ADF_REAL_PREC_MAX+1,ADF_LIMIT,1);
    arb_set_si(x->inf,-2);
    fail_at(s,x,adf_place_inf(),64,ADF_DOMAIN,0);
    for (int a=0;a<2;a++)
    {
        adf_place_t w=prime(97);
        unsigned char raw[sizeof(adf_adele_struct)];
        adele_padding(y); adf_adele_set(before,y); memcpy(raw,y,sizeof(raw));
        ADF_CHECK((a?adf_idele_log_abs(y,&w,x,WORD_MAX):adf_idele_Log(y,&w,x,WORD_MAX))==ADF_LIMIT);
        ADF_CHECK(adf_place_is_archimedean(w) && adf_adele_identical(y,before));
        ADF_CHECK(memcmp(raw,y,sizeof(raw))==0);
    }
    /* Exact powers ignore extreme N in _at; finite inputs retain finite exponents. */
    mk(x,3,1,-1,0);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(3),WORD_MAX)==ADF_OK && s->loc[0].exact);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(3),WORD_MIN)==ADF_OK && s->loc[0].exact);
    mk(x,1,3,1,0);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(3),WORD_MAX)==ADF_OK && s->loc[0].exact);
    mk(x,4,1,1,9);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(5),ADF_REAL_PREC_MAX+1)==ADF_OK && s->loc[0].N==1);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(3),WORD_MAX)==ADF_OK && s->loc[0].N==2);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(3),-ADF_LBALL_EXP_MAX)==ADF_OK);
    /* A huge restricted modulus is already in the input; N=4 never builds its local ball. */
    mk(x,3,1,1,1); fmpz_one(x->u.N); fmpz_mul_2exp(x->u.N,x->u.N,100000);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(2),4)==ADF_OK && s->loc[0].N==4);
    mk(x,2,1,1,0);
    fail_at(s,x,prime(UWORD_MAX-58),ADF_LBALL_BITS_MAX/64+1,ADF_LIMIT,0);
    mk(x,3,1,1,0);
    fail_at(s,x,prime(2),ADF_LBALL_BITS_MAX/2,ADF_LIMIT,0);
    /* Repeated outputs are supported; even exact global input remains conservative. */
    mk(x,1,1,1,0); arb_one(x->inf);
    ADF_CHECK(adf_idele_Log(y,NULL,x,64)==ADF_OK && fmpz_equal_ui(y->fin.H,4));
    mk(x,1,1,-1,0); arb_set_si(x->inf,-1);
    ADF_CHECK(adf_idele_log_abs(y,NULL,x,64)==ADF_OK && arb_is_zero(y->inf));
    adf_idele_clear(x); adf_adele_clear(y); adf_adele_clear(before); adf_sball_clear(s);
}

ADF_TEST(large_prime_routes)
{
    const ulong ps[]={65537,UWORD_MAX-58};
    adf_idele_t x;
    adf_sball_t s;
    adf_lball_t a,b,c,sum,input;
    adf_rat_t q;
    fmpz_t p,M,z;
    adf_idele_init(x); adf_sball_init(s); adf_rat_init(q);
    adf_lball_init(a); adf_lball_init(b); adf_lball_init(c); adf_lball_init(sum); adf_lball_init(input);
    fmpz_init(p); fmpz_init(M); fmpz_init(z);
    for (size_t i=0;i<2;i++)
    {
        adf_place_t v=prime(ps[i]);
        fmpz_set_ui(p,ps[i]); fmpz_pow_ui(M,p,3);
        mk(x,1,1,1,1); fmpz_set(x->u.N,M);
        fmpz_add_ui(fmpq_numref(x->r),p,1);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,3)==ADF_OK);
        ADF_CHECK(adf_sball_get_lball(a,s,v)==ADF_OK && a->N==3 && a->v==1);
        fmpq_set(q->q,x->r);
        ADF_CHECK(adf_lball_set_rat_ball(input,v,q,3)==ADF_OK);
        ADF_CHECK(adf_lball_Log(b,input,3)==ADF_OK && adf_lball_identical(a,b));
        /* Exact witness log(1+p) mod p^2 = p, both primes odd and >3. */
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,2)==ADF_OK);
        ADF_CHECK(adf_lball_get_center(q,&s->loc[0])==ADF_OK && fmpz_equal(fmpq_numref(q->q),p));
        fmpz_mul_ui(fmpq_numref(x->r),p,2); fmpz_add_ui(fmpq_numref(x->r),fmpq_numref(x->r),1);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,3)==ADF_OK); adf_lball_set(b,&s->loc[0]);
        fmpz_add_ui(z,p,1); fmpz_mul(fmpq_numref(x->r),fmpq_numref(x->r),z);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,3)==ADF_OK); adf_lball_set(c,&s->loc[0]);
        ADF_CHECK(adf_lball_add(sum,a,b)==ADF_OK && adf_lball_identical(sum,c));
        fmpq_inv(x->r,x->r);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,3)==ADF_OK);
        ADF_CHECK(adf_lball_add(sum,c,&s->loc[0])==ADF_OK && fmpq_is_zero(sum->u) && sum->N==3);
        /* Unrestricted c may be divisible by p: it must never be evaluated. */
        fmpz_add_ui(x->u.N,p,1); fmpz_set(x->u.c,p);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,WORD_MAX)==ADF_OK && s->loc[0].N==1);
        printf("large_prime_routes: p=%lu, restricted comparison/additivity at N=3, witness at N=2\n",ps[i]);
    }
    adf_idele_clear(x); adf_sball_clear(s); adf_rat_clear(q);
    adf_lball_clear(a); adf_lball_clear(b); adf_lball_clear(c); adf_lball_clear(sum); adf_lball_clear(input);
    fmpz_clear(p); fmpz_clear(M); fmpz_clear(z);
}
ADF_TEST(compact_lifts_and_large_content)
{
    adf_idele_t x;
    adf_sball_t s;
    adf_lball_t direct,input;
    adf_rat_t q;
    adf_idele_init(x); adf_sball_init(s); adf_lball_init(direct); adf_lball_init(input); adf_rat_init(q);
    /* A compact lift equal to 1 cannot make a nonzero exact centre's logarithm exact. */
    mk(x,126,1,1,0);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(5),3)==ADF_OK);
    ADF_CHECK(!s->loc[0].exact && s->loc[0].N==3 && fmpq_is_zero(s->loc[0].u));
    /* Huge numerator and denominator with a negative content valuation; compare exact centre.
       Working comparison at N=8 at 3. No enumeration or nonzero-rational-value assertion. */
    mk(x,1,1,1,729);
    fmpz_one(fmpq_numref(x->r)); fmpz_mul_2exp(fmpq_numref(x->r),fmpq_numref(x->r),4096);
    fmpz_add_ui(fmpq_numref(x->r),fmpq_numref(x->r),1);
    fmpz_one(fmpq_denref(x->r)); fmpz_mul_2exp(fmpq_denref(x->r),fmpq_denref(x->r),3000);
    fmpz_mul_ui(fmpq_denref(x->r),fmpq_denref(x->r),27);
    fmpq_canonicalise(x->r);
    fmpq_set(q->q,x->r);
    ADF_CHECK(adf_lball_set_rat(input,prime(3),q)==ADF_OK);
    ADF_CHECK(adf_lball_Log(direct,input,6)==ADF_OK);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,prime(3),8)==ADF_OK);
    ADF_CHECK(adf_lball_identical(direct,&s->loc[0]));
    adf_idele_clear(x); adf_sball_clear(s); adf_lball_clear(direct); adf_lball_clear(input); adf_rat_clear(q);
}
ADF_TEST(real_precision_ceiling)
{
    adf_idele_t x;
    adf_adele_t y;
    adf_sball_t s;
    adf_place_t w=prime(97);
    adf_idele_init(x); adf_adele_init(y); adf_sball_init(s);
    /* Log(1) needs no transcendental sum even at the admitted 2^21-bit ceiling. */
    ADF_CHECK(adf_idele_Log(y,&w,x,ADF_REAL_PREC_MAX)==ADF_OK);
    ADF_CHECK(arb_is_zero(y->inf) && adf_place_equal(w,prime(97)));
    ADF_CHECK(adf_idele_log_abs(y,NULL,x,ADF_REAL_PREC_MAX)==ADF_OK);
    ADF_CHECK(adf_idele_Log_at(s,NULL,x,adf_place_inf(),ADF_REAL_PREC_MAX)==ADF_OK);
    ADF_CHECK(adf_idele_log_abs_at(s,NULL,x,adf_place_inf(),ADF_REAL_PREC_MAX)==ADF_OK);
    adf_idele_clear(x); adf_adele_clear(y); adf_sball_clear(s);
}
ADF_TEST(refinement_witness)
{
    adf_idele_t x;
    adf_adele_t y;
    adf_place_t ps[]={prime(3)},w=prime(97);
    adf_idele_init(x); adf_adele_init(y); mk(x,4,1,1,9);
    ADF_CHECK(adf_idele_Log_refine(y,&w,x,ps,1,5,64)==ADF_OK);
    ADF_CHECK(fmpz_equal_ui(y->fin.A,12) && fmpz_equal_ui(y->fin.H,36) && fmpz_is_one(y->fin.d));
    arb_set_si(x->inf,-2);
    ADF_CHECK(adf_idele_log_abs_refine(y,&w,x,ps,1,5,64)==ADF_OK);
    ADF_CHECK(fmpz_equal_ui(y->fin.A,12) && fmpz_equal_ui(y->fin.H,36));
    adf_idele_clear(x); adf_adele_clear(y);
}

ADF_TEST(refinement_selection)
{
    jsonl_file *f=vectors("tests/ref/vectors/f-slice11/crt.jsonl");
    adf_idele_t x;
    adf_adele_t y,conservative;
    adf_lball_t e,got;
    adf_rat_t q;
    arb_t real;
    fmpz_t A,R;
    unsigned long memberships=0,projected=0;
    if(!f)return;
    adf_idele_init(x); adf_adele_init(y); adf_adele_init(conservative);
    adf_lball_init(e); adf_lball_init(got); adf_rat_init(q); arb_init(real); fmpz_init(A); fmpz_init(R);
    for(size_t i=0;i<jsonl_count(f);i++)
    {
        const jsonl_value *row=jsonl_record(f,i),*ps=field(row,"primes"),*lc=field(row,"local");
        jsonl_error_t err;
        adf_place_t places[3],w=prime(97);
        slong n=(slong)jsonl_size(ps);
        for(slong j=0;j<n;j++)places[j]=prime(strtoul(integer(jsonl_at(ps,(size_t)j,&err)),NULL,10));
        from_row(x,row);
        fmpz_set_str(A,integer(field(row,"A")),10); fmpz_set_str(R,integer(field(row,"R")),10);
        int st=adf_idele_Log_refine(y,&w,x,n?places:NULL,n,number(row,"N"),64);
        ADF_CHECK_MSG(st==ADF_OK,"CRT row %zu status %d",i+1,st);
        if(st!=ADF_OK)continue;
        ADF_CHECK_MSG(fmpz_equal(y->fin.A,A) && fmpz_equal(y->fin.H,R) && fmpz_is_one(y->fin.d),
                      "CRT row %zu triple",i+1);
        ADF_CHECK(adf_place_equal(w,prime(97)) && adf_adele_is_canonical(y));
        ADF_CHECK(adf_real_log(real,x->inf,64)==ADF_OK && arb_equal(y->inf,real));
        ADF_CHECK(fmpz_fdiv_ui(y->fin.A,4)==0 && fmpz_fdiv_ui(y->fin.H,4)==0);
        for(slong j=0;j<n;j++)
        {
            const jsonl_value *b=jsonl_at(lc,(size_t)j,&err);
            fmpz_set_str(fmpq_numref(q->q),integer(field(b,"center")),10);
            ADF_CHECK(adf_lball_set_rat_ball(e,places[j],q,number(b,"exponent"))==ADF_OK);
            ADF_CHECK(adf_lball_set_fball(got,places[j],&y->fin)==ADF_OK);
            ADF_CHECK_MSG(adf_lball_identical(e,got),"CRT row %zu local %ld",i+1,(long)j);
            projected++;
        }
        ADF_CHECK(adf_lball_set_fball(got,prime(11),&y->fin)==ADF_OK);
        ADF_CHECK(!got->exact && got->N==0 && fmpq_is_zero(got->u));
        if(n==0)
        {
            ADF_CHECK(adf_idele_Log(conservative,NULL,x,64)==ADF_OK);
            ADF_CHECK(adf_adele_identical(y,conservative));
        }
        if(fmpz_cmp_ui(R,4096)<=0)
        {
            ulong rr=fmpz_get_ui(R),aa=fmpz_get_ui(A);
            for(ulong z=0;z<2*rr;z++)
            {
                int direct=z%4==0;
                for(slong j=0;j<n;j++)
                {
                    const jsonl_value *b=jsonl_at(lc,(size_t)j,&err);
                    ulong mod=1,p=(ulong)number(b,"p"),centre=(ulong)number(b,"center");
                    for(slong k=0;k<number(b,"exponent");k++)mod*=p;
                    direct=direct && z%mod==centre;
                }
                ADF_CHECK_MSG(direct==(z%rr==aa),"CRT row %zu membership %lu",i+1,z);
                memberships++;
            }
        }
        arb_neg(x->inf,x->inf); w=prime(97);
        ADF_CHECK(adf_idele_log_abs_refine(y,&w,x,n?places:NULL,n,number(row,"N"),64)==ADF_OK);
        ADF_CHECK(fmpz_equal(y->fin.A,A) && fmpz_equal(y->fin.H,R) && arb_equal(y->inf,real));
        ADF_CHECK(adf_place_equal(w,prime(97)));
    }
    printf("refinement_selection: %zu rows, %lu local projections, %lu two-period memberships\n",
           jsonl_count(f),projected,memberships);
    adf_idele_clear(x); adf_adele_clear(y); adf_adele_clear(conservative);
    adf_lball_clear(e); adf_lball_clear(got); adf_rat_clear(q); arb_clear(real); fmpz_clear(A); fmpz_clear(R);
    jsonl_close(f);
}

static void fail_refine(adf_adele_t y,const adf_idele_t x,const adf_place_t *ps,slong n,
                        slong N,slong prec,int want,adf_place_t place,int named,int absval)
{
    adf_adele_t before;
    adf_place_t w=prime(97);
    unsigned char raw[sizeof(adf_adele_struct)];
    adf_adele_init(before); adele_padding(y); adf_adele_set(before,y); memcpy(raw,y,sizeof(raw));
    int st=absval?adf_idele_log_abs_refine(y,&w,x,ps,n,N,prec):adf_idele_Log_refine(y,&w,x,ps,n,N,prec);
    ADF_CHECK_MSG(st==want,"refine status %d want %d n=%ld N=%ld",st,want,(long)n,(long)N);
    ADF_CHECK(adf_place_equal(w,named?place:prime(97)));
    ADF_CHECK(adf_adele_identical(y,before) && memcmp(raw,y,sizeof(raw))==0);
    ADF_CHECK((absval?adf_idele_log_abs_refine(y,NULL,x,ps,n,N,prec):
                     adf_idele_Log_refine(y,NULL,x,ps,n,N,prec))==want);
    adf_adele_clear(before);
}
ADF_TEST(refinement_status_and_limits)
{
    adf_idele_t x;
    adf_adele_t y;
    adf_place_t ps[]={prime(5),prime(3),prime(2)},repeat[]={prime(5),prime(3),prime(5),prime(3)};
    adf_place_t infs[]={prime(5),adf_place_inf(),prime(5)};
    adf_idele_init(x); adf_adele_init(y); mk(x,4,1,1,9);
    ADF_CHECK(adf_idele_Log_refine(y,NULL,x,ps,3,5,64)==ADF_OK);
    for(int a=0;a<2;a++)
    {
        fail_refine(y,x,NULL,-1,5,64,ADF_DOMAIN,prime(3),0,a);
        fail_refine(y,x,NULL,ADF_IDLOG_PLACES_MAX+1,5,64,ADF_LIMIT,prime(3),0,a);
        fail_refine(y,x,repeat,4,5,64,ADF_DOMAIN,prime(3),1,a);
        fail_refine(y,x,infs,3,5,64,ADF_DOMAIN,adf_place_inf(),1,a);
        fail_refine(y,x,repeat,4,WORD_MIN,WORD_MAX,ADF_LIMIT,adf_place_inf(),1,a);
        fail_refine(y,x,ps,2,-ADF_LBALL_EXP_MAX-1,64,ADF_LIMIT,prime(3),1,a);
    }
    /* The maximum list length is admitted before shape checks. */
    adf_place_t *many=malloc((size_t)ADF_IDLOG_PLACES_MAX*sizeof(*many));
    ADF_CHECK(many!=NULL);
    if(many)
    {
        for(slong i=0;i<ADF_IDLOG_PLACES_MAX;i++)many[i]=prime(3);
        fail_refine(y,x,many,ADF_IDLOG_PLACES_MAX,0,64,ADF_DOMAIN,prime(3),1,0);
        free(many);
    }
    arb_set_si(x->inf,-2);
    fail_refine(y,x,ps,2,WORD_MIN,64,ADF_LIMIT,prime(3),1,0); /* prime LIMIT beats real DOMAIN */
    fail_refine(y,x,ps,3,5,64,ADF_DOMAIN,adf_place_inf(),1,0);
    mk(x,1,1,-1,0);
    /* Exact-zero rounding is local-bound checked before aggregate CRT size. */
    fail_refine(y,x,ps,3,ADF_LBALL_EXP_MAX+1,64,ADF_LIMIT,prime(2),1,0);
    fail_refine(y,x,ps,2,ADF_LBALL_EXP_MAX,64,ADF_LIMIT,prime(3),0,0);
    fail_refine(y,x,ps+1,2,ADF_IDLOG_CRT_BITS_MAX/2,64,ADF_LIMIT,prime(3),0,0);
    ADF_CHECK(adf_idele_Log_refine(y,NULL,x,NULL,0,WORD_MAX,ADF_REAL_PREC_MAX)==ADF_OK);
    ADF_CHECK(fmpz_equal_ui(y->fin.H,4));
    /* A nontrivial centre with fitting compact power but excessive working power. */
    mk(x,3,1,1,0); arb_set_si(x->inf,-2);
    fail_refine(y,x,ps+2,1,ADF_LBALL_BITS_MAX/2,64,ADF_LIMIT,prime(2),1,0);
    /* Earlier actual working LIMIT beats a later preflight compact-power LIMIT. */
    adf_place_t mixed[]={prime(5),prime(2)};
    fail_refine(y,x,mixed,2,ADF_LBALL_BITS_MAX/2,64,ADF_LIMIT,prime(2),1,0);
    /* A compact power over its bound is a local failure, even against negative real input. */
    adf_place_t big[]={prime(UWORD_MAX-58)};
    mk(x,2,1,1,0); arb_set_si(x->inf,-2);
    fail_refine(y,x,big,1,ADF_LBALL_BITS_MAX/64+1,64,ADF_LIMIT,big[0],1,0);
    adf_idele_clear(x); adf_adele_clear(y);
}
ADF_TEST(large_prime_torsion_routes)
{
    const ulong ps[]={65537,UWORD_MAX-58};
    adf_idele_t x;
    adf_sball_t s;
    adf_lball_t a,b,t,input,sum;
    adf_rat_t q;
    fmpz_t p;
    adf_idele_init(x); adf_sball_init(s); adf_rat_init(q); fmpz_init(p);
    adf_lball_init(a); adf_lball_init(b); adf_lball_init(t); adf_lball_init(input); adf_lball_init(sum);
    for(size_t i=0;i<2;i++)
    {
        adf_place_t v=prime(ps[i]);
        mk(x,2,1,1,1); fmpz_set_ui(p,ps[i]); fmpz_pow_ui(x->u.N,p,3);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,3)==ADF_OK); adf_lball_set(a,&s->loc[0]);
        fmpq_set(q->q,x->r);
        ADF_CHECK(adf_lball_set_rat_ball(input,v,q,3)==ADF_OK);
        ADF_CHECK(adf_lball_Log(t,input,3)==ADF_OK && adf_lball_identical(a,t));
        fmpq_set_si(x->r,3,1);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,3)==ADF_OK); adf_lball_set(b,&s->loc[0]);
        fmpq_set_si(x->r,6,1);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,3)==ADF_OK);
        ADF_CHECK(adf_lball_add(sum,a,b)==ADF_OK && adf_lball_identical(sum,&s->loc[0]));
        fmpq_set_si(x->r,2,1); fmpz_zero(x->u.N); fmpz_one(x->u.c);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,3)==ADF_OK); adf_lball_set(a,&s->loc[0]);
        fmpz_set_si(x->u.c,-1);
        ADF_CHECK(adf_idele_Log_at(s,NULL,x,v,3)==ADF_OK && adf_lball_identical(a,&s->loc[0]));
    }
    adf_idele_clear(x); adf_sball_clear(s); adf_rat_clear(q); fmpz_clear(p);
    adf_lball_clear(a); adf_lball_clear(b); adf_lball_clear(t); adf_lball_clear(input); adf_lball_clear(sum);
}

/* FLINT requests one final cleanup after all worker and object lifetimes end:
   refs/src/flint-3.0.1/memory.rst:26-43. This frees its tagged fmpz and real-constant caches. */
int main(void)
{
    int st=gfunc_log_test_main();
    flint_cleanup_master();
    return st;
}
