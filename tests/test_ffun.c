/* Slice 4a: independent exact vectors, F1 refinement and F4 quantitative error checks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <adelefeld.h>
#include "support/jsonl.h"
#include "support/golden.h"

static unsigned long checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); abort(); } } while (0)
static jsonl_error_t je;
static const jsonl_value *field(const jsonl_value *v, const char *s)
{
    const jsonl_value *r = NULL;
    CHECK(jsonl_field(v, s, &r, &je));
    return r;
}
static const jsonl_value *at(const jsonl_value *v, size_t i)
{
    const jsonl_value *r = jsonl_at(v, i, &je);
    CHECK(r != NULL); return r;
}
static long integer(const jsonl_value *v)
{
    const char *s = jsonl_int_text(v, &je);
    CHECK(s != NULL); return strtol(s, NULL, 10);
}
static void rational(fmpq_t q, const jsonl_value *v)
{
    size_t n;
    const char *s = jsonl_string(v, &n, &je);
    CHECK(s != NULL); CHECK(fmpq_set_str(q, s, 10) == 0); fmpq_canonicalise(q);
}
static void ball_part(arb_t a, const jsonl_value *m, const jsonl_value *r)
{
    fmpq_t q;
    arb_t t;
    fmpq_init(q); arb_init(t);
    rational(q, m); arb_set_fmpq(a, q, 512);
    rational(q, r); arb_set_fmpq(t, q, 512);
    mag_t error; mag_init(error); arb_get_mag(error,t); arb_add_error_mag(a,error); mag_clear(error);
    rational(q,m); CHECK(arb_contains_fmpq(a,q));
    arb_clear(t); fmpq_clear(q);
}
static void vector_ball(acb_t z, const jsonl_value *v)
{
    ball_part(acb_realref(z), at(v,0), at(v,1));
    ball_part(acb_imagref(z), at(v,2), at(v,3));
}
static void load(adf_ffun_t x, ulong D, ulong M, const jsonl_value *a, int complex)
{
    slong n = (slong) jsonl_size(a), j;
    acb_ptr f = _acb_vec_init(n);
    fmpq_t q;
    fmpq_init(q);
    for (j=0; j<n; j++) {
        if (complex) vector_ball(f+j, at(a,(size_t)j));
        else { rational(q, at(a,(size_t)j)); acb_set_fmpq(f+j,q,512); }
    }
    CHECK(adf_ffun_set_acb_vec(x,D,M,f,n) == ADF_OK);
    _acb_vec_clear(f,n); fmpq_clear(q);
}
static void preserved(const adf_ffun_t x, const adf_ffun_t old, const unsigned char *bytes)
{
    CHECK(memcmp(x,bytes,sizeof(*x)) == 0); CHECK(adf_ffun_identical(x,old));
}
static void lifecycle(void)
{
    adf_ffun_t x,y,z;
    acb_ptr a = _acb_vec_init(6);
    unsigned char bytes[sizeof(*x)];
    adf_ffun_init(x); adf_ffun_init(y); adf_ffun_init(z);
    CHECK(x->D==1 && x->M==1 && x->f!=NULL && acb_is_zero(x->f));
    CHECK(adf_ffun_is_canonical(x));
    CHECK(adf_sizeof_ffun()==sizeof(*x)); CHECK(adf_alignof_ffun()==_Alignof(adf_ffun_struct));
    for (int i=0;i<6;i++) acb_set_si(a+i,i-3);
    CHECK(adf_ffun_set_acb_vec(x,2,3,a,6)==ADF_OK);
    adf_ffun_set(y,x); CHECK(adf_ffun_identical(x,y)); CHECK(x->f!=y->f);
    acb_one(a); CHECK(acb_equal(x->f,y->f));
    adf_ffun_set(x,x); CHECK(adf_ffun_identical(x,y));
    adf_ffun_swap(x,z); CHECK(z->D==2 && z->M==3 && x->D==1);
    adf_ffun_swap(x,z); adf_ffun_swap(x,x); CHECK(adf_ffun_identical(x,y));
    memcpy(bytes,x,sizeof(*x));
    CHECK(adf_ffun_set_acb_vec(x,0,1,a,6)==ADF_DOMAIN); preserved(x,y,bytes);
    CHECK(adf_ffun_set_acb_vec(x,1,0,a,6)==ADF_DOMAIN); preserved(x,y,bytes);
    CHECK(adf_ffun_set_acb_vec(x,2,3,a,5)==ADF_DOMAIN); preserved(x,y,bytes);
    CHECK(adf_ffun_set_acb_vec(x,1,1,NULL,1)==ADF_DOMAIN); preserved(x,y,bytes);
    CHECK(adf_ffun_set_acb_vec(x,1,1,a,-1)==ADF_DOMAIN); preserved(x,y,bytes);
    acb_indeterminate(a+5);
    CHECK(adf_ffun_set_acb_vec(x,2,3,a,6)==ADF_DOMAIN); preserved(x,y,bytes);
    x->D=0; CHECK(!adf_ffun_is_canonical(x)); x->D=2;
    x->M=0; CHECK(!adf_ffun_is_canonical(x)); x->M=3;
    acb_ptr saved=x->f; x->f=NULL; CHECK(!adf_ffun_is_canonical(x)); x->f=saved;
    ulong savedD=x->D,savedM=x->M;
    x->D=UWORD(1)<<61; x->M=2; CHECK(!adf_ffun_is_canonical(x));
    x->D=savedD; x->M=savedM;
    x->M=UWORD_MAX; CHECK(!adf_ffun_is_canonical(x)); x->M=3;
    acb_indeterminate(x->f); CHECK(!adf_ffun_is_canonical(x)); acb_set(x->f,y->f);
    acb_ptr zeros=_acb_vec_init(2);
    adf_ffun_clear(z); adf_ffun_init(z);
    CHECK(adf_ffun_set_acb_vec(y,1,2,zeros,2)==ADF_OK);
    CHECK(!adf_ffun_identical(z,y)); CHECK(!adf_ffun_identical(y,z));
    CHECK(adf_ffun_set_acb_vec(y,2,1,zeros,2)==ADF_OK);
    CHECK(!adf_ffun_identical(z,y)); CHECK(!adf_ffun_identical(y,z));
    _acb_vec_clear(zeros,2);
    _acb_vec_clear(a,6); adf_ffun_clear(x); adf_ffun_clear(y); adf_ffun_clear(z);
}
static void text(void)
{
    golden_file *file;
    golden_error_t ge;
    adf_ffun_t x,old;
    unsigned char bytes[sizeof(*x)];
    size_t n;
    adf_ffun_init(x); adf_ffun_init(old); acb_set_si(x->f,17);
    CHECK(golden_open("tests/golden/ffun.tsv",GOLDEN_DEFAULT_MAX_INPUT,&file,&ge));
    CHECK(golden_count(file)==18);
    for (size_t i=0;i<golden_count(file);i++) {
        const golden_record *r=golden_record_at(file,i);
        adf_ffun_set(old,x); memcpy(bytes,x,sizeof(*x));
        int st=adf_ffun_set_str(x,r->input,r->input_len,128,NULL);
        if (r->is_status) { CHECK(strcmp(adf_status_str(st),r->status)==0); preserved(x,old,bytes); }
        else {
            CHECK(st==ADF_OK); char *s=adf_ffun_get_str(&n,x,ADF_DIGITS_DEFAULT);
            CHECK(s!=NULL);
            if (strcmp(s,r->expected)) fprintf(stderr,"golden %zu got %s expected %s\n",i,s,r->expected);
            CHECK(n==r->expected_len); CHECK(strcmp(s,r->expected)==0);
            adf_str_free(s);
        }
    }
    const char *s="ffun(D=0, M=1; (0e9999999) + (0)*i)";
    adf_ffun_set(old,x); memcpy(bytes,x,sizeof(*x));
    CHECK(adf_ffun_set_str(x,s,strlen(s),53,NULL)==ADF_LIMIT); preserved(x,old,bytes);
    CHECK(adf_ffun_set_str(x,s,strlen(s)-1,53,NULL)==ADF_PARSE); preserved(x,old,bytes);
    adf_text_limits_t lim; adf_text_limits_default(&lim); lim.max_items=0;
    s="ffun(D=1, M=1; (0) + (0)*i)";
    CHECK(adf_ffun_set_str(x,s,strlen(s),53,&lim)==ADF_LIMIT); preserved(x,old,bytes);
    lim.max_items=10; lim.max_len=2;
    CHECK(adf_ffun_set_str(x,s,strlen(s),53,&lim)==ADF_LIMIT); preserved(x,old,bytes);
    s="ffun(D=1, M=1; (0) + (0)*i)\0junk";
    CHECK(adf_ffun_set_str(x,s,31,53,NULL)==ADF_PARSE); preserved(x,old,bytes);
    CHECK(adf_ffun_set_str(x,"bad",3,ADF_REAL_PREC_MAX+1,NULL)==ADF_LIMIT);
    s="ffun(D=0, M=99999999999999999999999999999999999999999999999999; (0) + (0)*i)";
    CHECK(adf_ffun_set_str(x,s,strlen(s),53,NULL)==ADF_DOMAIN); preserved(x,old,bytes);
    acb_zero(x->f); arf_one(arb_midref(acb_realref(x->f)));
    fmpz_set_si(ARF_EXPREF(arb_midref(acb_realref(x->f))),10000000);
    char *out=adf_ffun_get_str(&n,x,ADF_DIGITS_DEFAULT); CHECK(out==NULL && n==0);
    adf_ffun_clear(x); adf_ffun_init(x);
    jsonl_file *jf;
    CHECK(jsonl_open("tests/ref/vectors/f4-slice1/texts.jsonl",&jf,&je));
    for (size_t i=0;i<jsonl_count(jf);i++) {
        const jsonl_value *r=jsonl_record(jf,i);
        size_t rawlen,elen;
        const char *raw=jsonl_string(field(r,"raw"),&rawlen,&je);
        const char *expected=jsonl_string(field(r,"expected"),&elen,&je);
        CHECK(raw!=NULL && expected!=NULL);
        adf_ffun_set(old,x); memcpy(bytes,x,sizeof(*x));
        int status=adf_ffun_set_str(x,raw,rawlen,128,NULL);
        if (expected[0]=='!') {
            CHECK(strcmp(adf_status_str(status),expected+1)==0); preserved(x,old,bytes);
        } else {
            CHECK(status==ADF_OK); char *printed=adf_ffun_get_str(&n,x,20);
            CHECK(printed!=NULL && n==elen && strcmp(printed,expected)==0); adf_str_free(printed);
        }
    }
    jsonl_close(jf);
    adf_ffun_clear(x); adf_ffun_clear(old); golden_close(file);
}
static void sum(void)
{
    jsonl_file *file;
    adf_ffun_t a,b,u,v,s,t,old;
    fmpq_t q;
    unsigned char bytes[sizeof(*s)];
    adf_ffun_init(a); adf_ffun_init(b); adf_ffun_init(u); adf_ffun_init(v);
    adf_ffun_init(s); adf_ffun_init(t); adf_ffun_init(old); fmpq_init(q);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice1/sums.jsonl",&file,&je));
    for (size_t i=0;i<jsonl_count(file);i++) {
        const jsonl_value *r=jsonl_record(file,i), *x=field(r,"a"), *y=field(r,"b");
        ulong D=(ulong)integer(field(r,"D")), M=(ulong)integer(field(r,"M"));
        load(a,(ulong)integer(at(x,0)),(ulong)integer(at(x,1)),at(x,2),0);
        load(b,(ulong)integer(at(y,0)),(ulong)integer(at(y,1)),at(y,2),0);
        CHECK(adf_ffun_refine(u,a,D,M)==ADF_OK); CHECK(adf_ffun_refine(v,b,D,M)==ADF_OK);
        CHECK(adf_ffun_add(s,a,b,53)==ADF_OK); CHECK(s->D==D && s->M==M);
        for (ulong j=0;j<D*M;j++) {
            rational(q,at(field(r,"ra"),j)); CHECK(acb_contains_fmpq(u->f+j,q));
            rational(q,at(field(r,"rb"),j)); CHECK(acb_contains_fmpq(v->f+j,q));
            rational(q,at(field(r,"sum"),j)); CHECK(acb_contains_fmpq(s->f+j,q));
            CHECK(arb_is_exact(acb_realref(s->f+j))); CHECK(arb_is_zero(acb_imagref(s->f+j)));
        }
        CHECK(adf_ffun_add(t,b,a,53)==ADF_OK); CHECK(adf_ffun_identical(t,s));
        adf_ffun_set(t,a); CHECK(adf_ffun_refine(t,t,D,M)==ADF_OK); CHECK(adf_ffun_identical(t,u));
        adf_ffun_set(t,a); CHECK(adf_ffun_add(t,t,b,53)==ADF_OK); CHECK(adf_ffun_identical(t,s));
        adf_ffun_set(t,b); CHECK(adf_ffun_add(t,a,t,53)==ADF_OK); CHECK(adf_ffun_identical(t,s));
        adf_ffun_set(old,s); memcpy(bytes,s,sizeof(*s));
        CHECK(adf_ffun_refine(s,a,5,6)==ADF_DOMAIN); preserved(s,old,bytes);
        CHECK(adf_ffun_refine(s,a,0,6)==ADF_DOMAIN); preserved(s,old,bytes);
        CHECK(adf_ffun_refine(s,a,6,0)==ADF_DOMAIN); preserved(s,old,bytes);
        CHECK(adf_ffun_refine(s,a,6,5)==ADF_DOMAIN); preserved(s,old,bytes);
        CHECK(adf_ffun_refine(s,a,1048576,6)==ADF_LIMIT); preserved(s,old,bytes);
        CHECK(adf_ffun_add(s,a,b,ADF_REAL_PREC_MAX+1)==ADF_LIMIT); preserved(s,old,bytes);
        acb_zero(t->f); /* t is not zero as a whole; use initialized singleton below. */
        adf_ffun_clear(t); adf_ffun_init(t);
        CHECK(adf_ffun_add(s,a,t,53)==ADF_OK); CHECK(adf_ffun_identical(s,a));
    }
    fmpq_clear(q); jsonl_close(file);
    adf_ffun_clear(a); adf_ffun_clear(b); adf_ffun_clear(u); adf_ffun_clear(v);
    adf_ffun_clear(s); adf_ffun_clear(t); adf_ffun_clear(old);
}
/* F4 conservative coordinate allowance used here:
   B = 4 sum_j(rad_re+rad_im)/M + 128 L^2 max_j(1, |mid_re|+|mid_im|+r_j) 2^-p/M.
   Rectangle propagation costs at most twice each disk input allowance. The factor 128 bounds
   phase eta <= sqrt(2)(4+2^-27)2^-p, four scalar products, additions, division and mag rounding.
   It is deliberately above those directed errors, but shrinks with precision on exact inputs. */
static void width(const adf_ffun_t x, const adf_ffun_t g, slong p)
{
    mag_t floor,bound,t,scale,largest;
    mag_init(floor); mag_init(bound); mag_init(t); mag_init(scale); mag_init(largest);
    mag_one(largest);
    ulong L=x->D*x->M;
    for (ulong j=0;j<L;j++) {
        mag_add(t,arb_radref(acb_realref(x->f+j)),arb_radref(acb_imagref(x->f+j)));
        mag_add(floor,floor,t);
        acb_get_mag(t,x->f+j); mag_max(largest,largest,t);
    }
    mag_mul_ui(floor,floor,4); mag_mul_ui(bound,largest,128*L*L);
    mag_mul_2exp_si(bound,bound,-p); mag_add(bound,bound,floor);
    mag_set_ui(scale,x->M); mag_div(bound,bound,scale);
    for (ulong k=0;k<L;k++) {
        CHECK(mag_cmp(arb_radref(acb_realref(g->f+k)),bound)<=0);
        CHECK(mag_cmp(arb_radref(acb_imagref(g->f+k)),bound)<=0);
    }
    mag_clear(floor); mag_clear(bound); mag_clear(t); mag_clear(scale); mag_clear(largest);
}
static void fourier(void)
{
    jsonl_file *file;
    adf_ffun_t x,g,h,copy;
    acb_t ref,phase,term;
    fmpq_t q;
    arb_t lhs,rhs,t;
    adf_ffun_init(x); adf_ffun_init(g); adf_ffun_init(h); adf_ffun_init(copy);
    acb_init(ref); acb_init(phase); acb_init(term); fmpq_init(q);
    arb_init(lhs); arb_init(rhs); arb_init(t);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice1/functions.jsonl",&file,&je));
    for (size_t i=0;i<jsonl_count(file);i++) {
        const jsonl_value *r=jsonl_record(file,i), *refs=field(r,"g");
        ulong D=(ulong)integer(field(r,"D")), M=(ulong)integer(field(r,"M")), L=D*M;
        load(x,D,M,field(r,"f"),1);
        CHECK(adf_ffun_fourier(g,x,53)==ADF_OK); CHECK(g->D==M && g->M==D); width(x,g,53);
        const jsonl_value *uncertain=field(r,"uncertain_g");
        CHECK(jsonl_size(uncertain)==0 || jsonl_size(uncertain)==L);
        for (ulong k=0;k<jsonl_size(uncertain);k++) {
            vector_ball(ref,at(uncertain,k)); CHECK(acb_overlaps(g->f+k,ref));
        }
        for (ulong k=0;k<L;k++) {
            if (jsonl_size(refs)) { vector_ball(ref,at(refs,k));
                if (!acb_contains(g->f+k,ref)) {
                    fprintf(stderr,"reference failure i=%zu k=%lu\n",i,k);
                    acb_printn(g->f+k,25,0); puts(""); acb_printn(ref,25,0); puts("");
                }
                CHECK(acb_contains(g->f+k,ref)); }
            else {
                /* Four corners choose every independent input coordinate at the same signed end.
                   The midpoint Fourier sum has no coefficient error; high precision gives a certificate. */
                for (int corner=0;corner<4;corner++) {
                    acb_zero(ref);
                    for (ulong j=0;j<L;j++) {
                        arb_get_interval_arf(arb_midref(acb_realref(term)),arb_midref(acb_imagref(term)),
                                             acb_realref(x->f+j),512);
                        arf_set(arb_midref(acb_realref(term)),corner&1 ?
                                arb_midref(acb_imagref(term)) : arb_midref(acb_realref(term)));
                        arf_t lo,hi; arf_init(lo); arf_init(hi);
                        arb_get_interval_arf(lo,hi,acb_imagref(x->f+j),512);
                        arf_set(arb_midref(acb_imagref(term)),corner&2 ? hi : lo);
                        mag_zero(arb_radref(acb_realref(term))); mag_zero(arb_radref(acb_imagref(term)));
                        arf_clear(lo); arf_clear(hi);
                        fmpq_set_ui(q,(L-(j*k)%L)%L,L); fmpq_canonicalise(q);
                        /* Independent trig formula, rather than the production phase evaluator. */
                        arb_set_fmpq(t,q,512); arb_mul_2exp_si(t,t,1);
                        arb_sin_cos_pi(acb_imagref(phase),acb_realref(phase),t,512);
                        acb_mul(term,term,phase,512); acb_add(ref,ref,term,512);
                    }
                    acb_div_ui(ref,ref,M,512); CHECK(acb_contains(g->f+k,ref));
                }
            }
        }
        if (L<=60) {
            adf_ffun_set(copy,x); CHECK(adf_ffun_fourier(copy,copy,53)==ADF_OK);
            CHECK(adf_ffun_identical(copy,g));
            CHECK(adf_ffun_fourier(h,g,128)==ADF_OK);
            for (ulong j=0;j<L;j++) CHECK(acb_overlaps(h->f+j,x->f+(L-j)%L));
            if (jsonl_size(refs)) {
                arb_zero(lhs); arb_zero(rhs);
                for (ulong j=0;j<L;j++) {
                    acb_abs(t,x->f+j,128); arb_sqr(t,t,128); arb_add(lhs,lhs,t,128);
                    acb_abs(t,g->f+j,128); arb_sqr(t,t,128); arb_add(rhs,rhs,t,128);
                }
                arb_div_ui(lhs,lhs,M,128); arb_div_ui(rhs,rhs,D,128); CHECK(arb_overlaps(lhs,rhs));
                CHECK(adf_ffun_fourier(h,x,128)==ADF_OK); width(x,h,128);
                for (ulong k=0;k<L;k++) {
                    vector_ball(ref,at(refs,k)); CHECK(acb_contains(h->f+k,ref));
                    CHECK(mag_cmp(arb_radref(acb_realref(h->f+k)),
                                  arb_radref(acb_realref(g->f+k)))<=0);
                }
            }
        }
    }
    /* Exact cardinal phases: F^2 has exactly the reflection, including swapped weights. */
    acb_ptr a=_acb_vec_init(4); for (int j=0;j<4;j++) acb_set_si(a+j,j+1);
    CHECK(adf_ffun_set_acb_vec(x,4,1,a,4)==ADF_OK);
    CHECK(adf_ffun_fourier(g,x,53)==ADF_OK); CHECK(adf_ffun_fourier(h,g,53)==ADF_OK);
    for (int j=0;j<4;j++) CHECK(acb_equal(h->f+j,x->f+(4-j)%4));
    /* D != M delta at index one: each negative phase is weighted by 1/3. */
    _acb_vec_clear(a,4); a=_acb_vec_init(6); acb_one(a+1);
    CHECK(adf_ffun_set_acb_vec(x,2,3,a,6)==ADF_OK); CHECK(adf_ffun_fourier(g,x,53)==ADF_OK);
    for (ulong k=0;k<6;k++) {
        fmpq_set_ui(q,(6-k)%6,6); fmpq_canonicalise(q);
        arb_set_fmpq(t,q,512); arb_mul_2exp_si(t,t,1);
        arb_sin_cos_pi(acb_imagref(ref),acb_realref(ref),t,512); acb_div_ui(ref,ref,3,512);
        CHECK(acb_contains(g->f+k,ref));
    }
    CHECK(adf_ffun_fourier(g,x,2)==ADF_OK); width(x,g,2);
    for (ulong k=0;k<6;k++) {
        fmpq_set_ui(q,(6-k)%6,6); fmpq_canonicalise(q);
        arb_set_fmpq(t,q,512); arb_mul_2exp_si(t,t,1);
        arb_sin_cos_pi(acb_imagref(ref),acb_realref(ref),t,512); acb_div_ui(ref,ref,3,512);
        CHECK(acb_contains(g->f+k,ref));
    }
    _acb_vec_clear(a,6); jsonl_close(file);
    acb_clear(ref); acb_clear(phase); acb_clear(term); fmpq_clear(q);
    arb_clear(lhs); arb_clear(rhs); arb_clear(t);
    adf_ffun_clear(x); adf_ffun_clear(g); adf_ffun_clear(h); adf_ffun_clear(copy);
}
static void caps(void)
{
    jsonl_file *file;
    adf_ffun_t x,y,old;
    unsigned char bytes[sizeof(*y)];
    adf_ffun_init(x); adf_ffun_init(y); adf_ffun_init(old);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice1/caps.jsonl",&file,&je));
    for (size_t i=0;i<jsonl_count(file);i++) {
        const jsonl_value *r=jsonl_record(file,i);
        ulong D=(ulong)integer(field(r,"D")), M=(ulong)integer(field(r,"M"));
        slong L=(slong)(D*M);
        acb_ptr a=_acb_vec_init(L<=1048576 ? L : 1);
        adf_ffun_set(old,y); memcpy(bytes,y,sizeof(*y));
        int st=adf_ffun_set_acb_vec(y,D,M,a,L);
        CHECK(st==integer(field(r,"set")));
        if (st!=ADF_OK) preserved(y,old,bytes);
        else {
            adf_ffun_set(x,y); adf_ffun_set(old,y); memcpy(bytes,y,sizeof(*y));
            st=adf_ffun_fourier(y,x,53); CHECK(st==integer(field(r,"fourier")));
            if (st!=ADF_OK) preserved(y,old,bytes);
        }
        _acb_vec_clear(a,L<=1048576 ? L : 1);
    }
    adf_ffun_clear(x); adf_ffun_init(x); acb_one(x->f);
    for (int i=0;i<3;i++) {
        slong p=i==0 ? 2 : i==1 ? 53 : ADF_REAL_PREC_MAX;
        CHECK(adf_ffun_fourier(y,x,p)==ADF_OK); CHECK(acb_is_one(y->f));
        CHECK(adf_ffun_add(y,x,x,p)==ADF_OK); CHECK(acb_equal_si(y->f,2));
    }
    adf_ffun_set(old,y); memcpy(bytes,y,sizeof(*y));
    x->D=0;
    CHECK(adf_ffun_fourier(y,x,ADF_REAL_PREC_MAX+1)==ADF_LIMIT); preserved(y,old,bytes);
    CHECK(adf_ffun_add(y,x,x,ADF_REAL_PREC_MAX+1)==ADF_LIMIT); preserved(y,old,bytes);
    x->D=1;
    CHECK(adf_ffun_set_acb_vec(y,UWORD_MAX,UWORD_MAX,NULL,WORD_MAX)==ADF_LIMIT);
    preserved(y,old,bytes);
    /* Noncardinal phase certificate failure at the precision cap, after one full output row. */
    acb_ptr delta=_acb_vec_init(3); acb_one(delta+1);
    CHECK(adf_ffun_set_acb_vec(x,1,3,delta,3)==ADF_OK);
    adf_ffun_set(old,y); memcpy(bytes,y,sizeof(*y));
    CHECK(adf_ffun_fourier(y,x,ADF_REAL_PREC_MAX)==ADF_NOT_DETERMINED); preserved(y,old,bytes);
    adf_ffun_set(old,x); memcpy(bytes,x,sizeof(*x));
    CHECK(adf_ffun_fourier(x,x,ADF_REAL_PREC_MAX)==ADF_NOT_DETERMINED); preserved(x,old,bytes);
    _acb_vec_clear(delta,3);
    jsonl_close(file); adf_ffun_clear(x); adf_ffun_clear(y); adf_ffun_clear(old);
}
static void aliases_and_precision(void)
{
    adf_ffun_t a,b,out,t;
    acb_ptr f=_acb_vec_init(6);
    fmpz_t big;
    adf_ffun_init(a); adf_ffun_init(b); adf_ffun_init(out); adf_ffun_init(t);
    fmpz_init(big); fmpz_one(big); fmpz_mul_2exp(big,big,2001); fmpz_add_ui(big,big,7);
    acb_set_fmpz(f,big);
    for (int j=1;j<6;j++) {
        acb_set_si(f+j,j-3); arb_set_si(acb_imagref(f+j),2-j);
        mag_set_ui_2exp_si(arb_radref(acb_realref(f+j)),1,-8);
        mag_set_ui_2exp_si(arb_radref(acb_imagref(f+j)),1,-9);
    }
    CHECK(adf_ffun_set_acb_vec(a,2,3,f,6)==ADF_OK);
    CHECK(adf_ffun_set_acb_vec(b,3,2,f,6)==ADF_OK);
    adf_ffun_set(t,a); CHECK(adf_ffun_identical(t,a)); adf_ffun_set(t,t);
    adf_ffun_swap(t,t); CHECK(adf_ffun_identical(t,a));
    CHECK(adf_ffun_add(out,a,b,53)==ADF_OK);
    adf_ffun_set(t,a); CHECK(adf_ffun_add(t,t,b,53)==ADF_OK); CHECK(adf_ffun_identical(t,out));
    adf_ffun_set(t,b); CHECK(adf_ffun_add(t,a,t,53)==ADF_OK); CHECK(adf_ffun_identical(t,out));
    CHECK(adf_ffun_add(out,a,a,53)==ADF_OK);
    adf_ffun_set(t,a); CHECK(adf_ffun_add(t,t,t,53)==ADF_OK); CHECK(adf_ffun_identical(t,out));
    for (ulong j=0;j<6;j++) {
        acb_mul_2exp_si(f+j,a->f+j,1); CHECK(acb_contains(out->f+j,f+j));
    }
    CHECK(adf_ffun_refine(out,a,6,6)==ADF_OK);
    adf_ffun_set(t,a); CHECK(adf_ffun_refine(t,t,6,6)==ADF_OK); CHECK(adf_ffun_identical(out,t));
    CHECK(adf_ffun_fourier(out,a,53)==ADF_OK);
    adf_ffun_set(t,a); CHECK(adf_ffun_fourier(t,t,53)==ADF_OK); CHECK(adf_ffun_identical(out,t));
    /* Exact nonsymmetric complex cardinal data: all transforms and reflection are exact. */
    for (int j=0;j<4;j++) { acb_set_si(f+j,j-2); arb_set_si(acb_imagref(f+j),j*j-3); }
    CHECK(adf_ffun_set_acb_vec(a,4,1,f,4)==ADF_OK);
    CHECK(adf_ffun_fourier(out,a,53)==ADF_OK); CHECK(adf_ffun_fourier(t,out,53)==ADF_OK);
    for (int j=0;j<4;j++) CHECK(acb_equal(t->f+j,a->f+(4-j)%4));
    /* Three-precision shrinkage on non-cardinal exact data, with the same F4 radius bound. */
    for (int j=0;j<6;j++) acb_zero(f+j);
    acb_one(f+1); CHECK(adf_ffun_set_acb_vec(a,2,3,f,6)==ADF_OK);
    slong ps[3]={64,128,212};
    for (int j=0;j<3;j++) {
        CHECK(adf_ffun_fourier(out,a,ps[j])==ADF_OK); width(a,out,ps[j]);
        if (j) CHECK(mag_cmp(arb_radref(acb_imagref(out->f+1)),
                            arb_radref(acb_imagref(t->f+1)))<0);
        adf_ffun_set(t,out);
    }
    CHECK(adf_ffun_fourier(out,a,2)==ADF_OK); width(a,out,2);
    CHECK(adf_ffun_fourier(t,a,-100)==ADF_OK); CHECK(adf_ffun_identical(out,t));
    /* Addition's projected lcm exceeds the cap even though each operand fits. */
    acb_ptr large=_acb_vec_init(1025);
    CHECK(adf_ffun_set_acb_vec(a,1024,1,large,1024)==ADF_OK);
    CHECK(adf_ffun_set_acb_vec(b,1025,1,large,1025)==ADF_OK);
    adf_ffun_set(t,out); unsigned char bytes[sizeof(*out)]; memcpy(bytes,out,sizeof(*out));
    CHECK(adf_ffun_add(out,a,b,53)==ADF_LIMIT); preserved(out,t,bytes);
    adf_ffun_set(t,a); memcpy(bytes,a,sizeof(*a));
    CHECK(adf_ffun_add(a,a,b,53)==ADF_LIMIT); preserved(a,t,bytes);
    CHECK(adf_ffun_refine(a,a,2048,1)==ADF_OK);
    adf_ffun_set(t,a); memcpy(bytes,a,sizeof(*a));
    CHECK(adf_ffun_fourier(a,a,53)==ADF_LIMIT); preserved(a,t,bytes);
    CHECK(adf_ffun_refine(a,a,2097152,1)==ADF_LIMIT); preserved(a,t,bytes);
    _acb_vec_clear(large,1025); _acb_vec_clear(f,6); fmpz_clear(big);
    adf_ffun_clear(a); adf_ffun_clear(b); adf_ffun_clear(out); adf_ffun_clear(t);
}
static unsigned long allocations, frees;
static void count_free(void *p) { if (p) frees++; free(p); }
static void *count_alloc(size_t n) { allocations++; return malloc(n); }
static void *count_calloc(size_t n,size_t m) { allocations++; return calloc(n,m); }
static void *count_realloc(void *p,size_t n) { allocations++; return realloc(p,n); }
static void allocation_preflight(void)
{
    adf_ffun_t x,y;
    void *(*a)(size_t), *(*c)(size_t,size_t), *(*r)(void *,size_t);
    void (*f)(void *);
    adf_ffun_init(x); adf_ffun_init(y);
    acb_ptr data=_acb_vec_init(1025);
    CHECK(adf_ffun_set_acb_vec(x,1,1025,data,1025)==ADF_OK);
    __flint_get_memory_functions(&a,&c,&r,&f);
    __flint_set_memory_functions(count_alloc,count_calloc,count_realloc,free);
    allocations=0;
    int st=adf_ffun_fourier(y,x,53);
    unsigned long count=allocations;
    __flint_set_memory_functions(a,c,r,f);
    CHECK(st==ADF_LIMIT); CHECK(count==0);
    __flint_set_memory_functions(count_alloc,count_calloc,count_realloc,free);
    allocations=0;
    st=adf_ffun_set_acb_vec(y,1,1048577,NULL,1048577); count=allocations;
    __flint_set_memory_functions(a,c,r,f);
    CHECK(st==ADF_LIMIT); CHECK(count==0);
    _acb_vec_clear(data,1025); adf_ffun_clear(x); adf_ffun_clear(y);
    __flint_set_memory_functions(count_alloc,count_calloc,count_realloc,count_free);
    allocations=0; frees=0;
    for (int i=0;i<1000;i++) { adf_ffun_init(x); adf_ffun_clear(x); }
    count=allocations; unsigned long released=frees;
    __flint_set_memory_functions(a,c,r,f);
    CHECK(count==1000); CHECK(released==1000);
}
#ifdef ADF_CHECK_INVARIANTS
static void invariants(void)
{
    for (int mode=0;mode<17;mode++) {
        pid_t pid=fork(); CHECK(pid>=0);
        if (!pid) {
            adf_ffun_t x,y;
            adf_ffun_init(x); adf_ffun_init(y); acb_indeterminate(x->f);
            if (mode==0) adf_ffun_set(y,x);
            if (mode==1) adf_ffun_swap(y,x);
            if (mode==2) (void)adf_ffun_identical(y,x);
            if (mode==3) (void)adf_ffun_refine(y,x,1,1);
            if (mode==4) (void)adf_ffun_add(y,x,y,53);
            if (mode==5) (void)adf_ffun_fourier(y,x,53);
            if (mode==6) { size_t n; adf_str_free(adf_ffun_get_str(&n,x,20)); }
            if (mode==7) adf_ffun_clear(x);
            if (mode==8) (void)adf_ffun_set_acb_vec(x,1,1,y->f,1);
            if (mode==9) {
                const char *s="ffun(D=1, M=1; (0) + (0)*i)";
                (void)adf_ffun_set_str(x,s,strlen(s),53,NULL);
            }
            if (mode==10) adf_ffun_swap(x,y);
            if (mode==11) adf_ffun_set(x,y);
            if (mode==12) (void)adf_ffun_identical(x,y);
            if (mode==13) (void)adf_ffun_refine(x,y,1,1);
            if (mode==14) (void)adf_ffun_add(y,y,x,53);
            if (mode==15) (void)adf_ffun_add(x,y,y,53);
            if (mode==16) (void)adf_ffun_fourier(x,y,53);
            _exit(0);
        }
        int status; CHECK(waitpid(pid,&status,0)==pid);
        CHECK(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
    }
}
#endif
int main(int argc, char **argv)
{
    const char *group=argc>1 ? argv[1] : "all";
    if (!strcmp(group,"all") || !strcmp(group,"life")) lifecycle();
    if (!strcmp(group,"all") || !strcmp(group,"text")) text();
    if (!strcmp(group,"all") || !strcmp(group,"sum")) sum();
    if (!strcmp(group,"all") || !strcmp(group,"fourier")) fourier();
    if (!strcmp(group,"all") || !strcmp(group,"caps")) caps();
    if (!strcmp(group,"all") || !strcmp(group,"extra")) {
        aliases_and_precision(); allocation_preflight();
#ifdef ADF_CHECK_INVARIANTS
        invariants();
#endif
    }
    printf("test_ffun: %lu checks (%s)\n",checks,group); flint_cleanup(); return 0;
}
