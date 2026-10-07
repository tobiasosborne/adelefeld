/* Slice 4b: oracle arrays, exact permutations, independent rectangle corners and transactions.
   F1 permits loss of correlations between duplicated balls, but no loss of enclosure. */
#define _DEFAULT_SOURCE
#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include "support/jsonl.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/mman.h>

static unsigned long checks, records, cells;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c); abort(); } } while (0)
static jsonl_error_t je;
static jsonl_file *inputs;
static const jsonl_value *field(const jsonl_value *v, const char *s)
{
    const jsonl_value *r=NULL; CHECK(jsonl_field(v,s,&r,&je)); return r;
}
static const jsonl_value *at(const jsonl_value *v, size_t i)
{
    const jsonl_value *r=jsonl_at(v,i,&je); CHECK(r!=NULL); return r;
}
static long integer(const jsonl_value *v)
{
    const char *s=jsonl_int_text(v,&je); CHECK(s!=NULL); return strtol(s,NULL,10);
}
static const char *string(const jsonl_value *v)
{
    size_t n; const char *s=jsonl_string(v,&n,&je); CHECK(s!=NULL); return s;
}
static void rational(fmpq_t q, const jsonl_value *v)
{
    CHECK(fmpq_set_str(q,string(v),10)==0); fmpq_canonicalise(q);
}
static void part(arb_t a, const jsonl_value *v, size_t i)
{
    fmpq_t q; arb_t t; mag_t m;
    fmpq_init(q); arb_init(t); mag_init(m);
    rational(q,at(v,i)); arb_set_fmpq(a,q,512);
    rational(q,at(v,i+1)); arb_set_fmpq(t,q,512); arb_get_mag(m,t); arb_add_error_mag(a,m);
    rational(q,at(v,i)); CHECK(arb_contains_fmpq(a,q));
    mag_clear(m); arb_clear(t); fmpq_clear(q);
}
static void load(adf_ffun_t x, const jsonl_value *v)
{
    ulong D=(ulong)integer(field(v,"D")), M=(ulong)integer(field(v,"M"));
    const jsonl_value *a=field(v,"f"); slong n=(slong)jsonl_size(a);
    acb_ptr f=_acb_vec_init(n);
    for (slong j=0;j<n;j++) { part(acb_realref(f+j),at(a,j),0); part(acb_imagref(f+j),at(a,j),2); }
    CHECK(adf_ffun_set_acb_vec(x,D,M,f,n)==ADF_OK); _acb_vec_clear(f,n);
}
static void input(adf_ffun_t x, const jsonl_value *v)
{
    load(x,jsonl_record(inputs,(size_t)integer(v)));
}
static jsonl_file *open_vectors(const char *name)
{
    char path[128]; jsonl_file *f;
    snprintf(path,sizeof(path),"tests/ref/vectors/f4-slice4/%s.jsonl",name);
    if (!jsonl_open(path,&f,&je)) fprintf(stderr,"%s\n",jsonl_error_message(&je));
    CHECK(f!=NULL); return f;
}
static void equal(const adf_ffun_t x, const adf_ffun_t y)
{
    CHECK(x->D==y->D); CHECK(x->M==y->M);
    for (ulong j=0;j<x->D*x->M;j++) {
        cells++;
        if (!acb_equal(x->f+j,y->f+j)) {
            fprintf(stderr,"layout (%lu,%lu), cell %lu: ",x->D,x->M,j);
            acb_printd(x->f+j,20); printf(" != "); acb_printd(y->f+j,20); printf("\n"); fflush(stdout);
        }
        CHECK(acb_equal(x->f+j,y->f+j));
    }
}
static void preserved(const adf_ffun_t x, const adf_ffun_t old, const unsigned char *bytes)
{
    CHECK(memcmp(x,bytes,sizeof(*x))==0); equal(x,old);
}
/* Take actual input rectangle endpoints, then multiply exact rational corners independently.
   The assertion is for each refined cell; independently chosen repeated balls are allowed by F1. */
static void endpoint(fmpq_t q, const arb_t x, int sign)
{
    fmpq_t r; fmpq_init(r);
    arf_get_fmpq(q,arb_midref(x)); mag_get_fmpq(r,arb_radref(x));
    if (sign) fmpq_add(q,q,r); else fmpq_sub(q,q,r);
    fmpq_clear(r);
}
static void corners(const acb_t z, const acb_t a, const acb_t b)
{
    fmpq_t ar,ai,br,bi,re,im,t;
    fmpq_init(ar); fmpq_init(ai); fmpq_init(br); fmpq_init(bi);
    fmpq_init(re); fmpq_init(im); fmpq_init(t);
    for (int c=0;c<16;c++) {
        endpoint(ar,acb_realref(a),c&1); endpoint(ai,acb_imagref(a),c&2);
        endpoint(br,acb_realref(b),c&4); endpoint(bi,acb_imagref(b),c&8);
        fmpq_mul(re,ar,br); fmpq_mul(t,ai,bi); fmpq_sub(re,re,t);
        fmpq_mul(im,ar,bi); fmpq_mul(t,ai,br); fmpq_add(im,im,t);
        CHECK(arb_contains_fmpq(acb_realref(z),re)); CHECK(arb_contains_fmpq(acb_imagref(z),im));
    }
    fmpq_clear(ar); fmpq_clear(ai); fmpq_clear(br); fmpq_clear(bi);
    fmpq_clear(re); fmpq_clear(im); fmpq_clear(t);
}
static void product(void)
{
    jsonl_file *file=open_vectors("products");
    adf_ffun_t x,w,y,a,b,z,h;
    adf_ffun_init(x); adf_ffun_init(w); adf_ffun_init(y); adf_ffun_init(a);
    adf_ffun_init(b); adf_ffun_init(z); adf_ffun_init(h);
    for (size_t i=0;i<jsonl_count(file);i++) {
        const jsonl_value *v=jsonl_record(file,i); records++;
        input(x,field(v,"x")); input(w,field(v,"w")); load(y,field(v,"y"));
        load(a,field(v,"a")); load(b,field(v,"b"));
        CHECK(adf_ffun_refine(z,x,a->D,a->M)==ADF_OK); equal(z,a);
        CHECK(adf_ffun_refine(z,w,b->D,b->M)==ADF_OK); equal(z,b);
        CHECK(adf_ffun_mul(z,x,w,128)==ADF_OK);
        CHECK(z->D==y->D && z->M==y->M);
        for (ulong j=0;j<y->D*y->M;j++) {
            cells++; CHECK(acb_overlaps(z->f+j,y->f+j)); corners(z->f+j,a->f+j,b->f+j);
            if (!integer(field(v,"uncertain"))) CHECK(acb_equal(z->f+j,y->f+j));
            else {
                mag_t bound,tiny; mag_init(bound); mag_init(tiny); mag_set_ui_2exp_si(tiny,1,-100);
                for (int t=0;t<2;t++) {
                    arb_srcptr ref=t ? acb_imagref(y->f+j) : acb_realref(y->f+j);
                    arb_srcptr got=t ? acb_imagref(z->f+j) : acb_realref(z->f+j);
                    mag_mul_ui(bound,arb_radref(ref),8); mag_add(bound,bound,tiny);
                    CHECK(mag_cmp(arb_radref(got),bound)<=0);
                }
                mag_clear(bound); mag_clear(tiny);
            }
        }
        CHECK(adf_ffun_mul(h,w,x,128)==ADF_OK); equal(h,z);
        adf_ffun_set(h,x); CHECK(adf_ffun_mul(h,h,w,128)==ADF_OK); equal(h,z);
        adf_ffun_set(h,w); CHECK(adf_ffun_mul(h,x,h,128)==ADF_OK); equal(h,z);
        CHECK(adf_ffun_mul(z,x,x,128)==ADF_OK);
        adf_ffun_set(h,x); CHECK(adf_ffun_mul(h,h,h,128)==ADF_OK); equal(h,z);
    }
    /* Multiplication by 1_Zhat is identity only on functions supported in Zhat. */
    acb_one(w->f); adf_ffun_clear(w); adf_ffun_init(w); acb_one(w->f);
    acb_ptr f=_acb_vec_init(3); for (int j=0;j<3;j++) acb_set_si(f+j,j+1);
    CHECK(adf_ffun_set_acb_vec(x,1,3,f,3)==ADF_OK);
    CHECK(adf_ffun_mul(z,x,w,128)==ADF_OK); equal(z,x); _acb_vec_clear(f,3);
    f=_acb_vec_init(6); acb_one(f+1);
    CHECK(adf_ffun_set_acb_vec(x,2,3,f,6)==ADF_OK);
    CHECK(adf_ffun_mul(z,x,w,128)==ADF_OK); CHECK(!adf_ffun_identical(z,x));
    for (int j=0;j<6;j++) CHECK(acb_is_zero(z->f+j));
    _acb_vec_clear(f,6);
    adf_ffun_clear(x); adf_ffun_clear(w); adf_ffun_clear(y); adf_ffun_clear(a);
    adf_ffun_clear(b); adf_ffun_clear(z); adf_ffun_clear(h); jsonl_close(file);
}
static void numeric_product(void)
{
    adf_ffun_t x,w,z,h; fmpz_t n; fmpq_t expected;
    adf_ffun_init(x); adf_ffun_init(w); adf_ffun_init(z); adf_ffun_init(h);
    fmpz_init(n); fmpq_init(expected);
    fmpz_one(n); fmpz_mul_2exp(n,n,40); fmpz_add_ui(n,n,3);
    acb_set_fmpz(x->f,n); acb_one(w->f);
    CHECK(!adf_ffun_identical(x,w)); CHECK(!adf_ffun_identical(w,x));
    CHECK(adf_ffun_mul(z,x,w,53)==ADF_OK); equal(z,x);
    CHECK(adf_ffun_mul(z,x,w,2)==ADF_OK); CHECK(!acb_equal(z->f,x->f));
    CHECK(adf_ffun_mul(h,x,w,-10)==ADF_OK); equal(h,z);
    CHECK(arb_contains_fmpz(acb_realref(z->f),n));
    fmpz_mul_2exp(n,n,1960); fmpz_add_ui(n,n,7);
    acb_set_fmpz(x->f,n); arb_set_si(acb_imagref(x->f),-3);
    arb_add_error_2exp_si(acb_realref(x->f),-4); arb_add_error_2exp_si(acb_imagref(x->f),-5);
    acb_set_si(w->f,-2); arb_set_si(acb_imagref(w->f),5);
    arb_add_error_2exp_si(acb_realref(w->f),-3); arb_add_error_2exp_si(acb_imagref(w->f),-4);
    CHECK(adf_ffun_mul(z,x,w,4096)==ADF_OK); corners(z->f,x->f,w->f);
    CHECK(adf_ffun_mul(h,w,x,4096)==ADF_OK); equal(z,h);
    adf_ffun_set(h,x); CHECK(adf_ffun_mul(h,h,w,4096)==ADF_OK); equal(z,h);
    adf_ffun_set(h,w); CHECK(adf_ffun_mul(h,x,h,4096)==ADF_OK); equal(z,h);
    CHECK(adf_ffun_mul(z,x,x,4096)==ADF_OK);
    adf_ffun_set(h,x); CHECK(adf_ffun_mul(h,h,h,4096)==ADF_OK); equal(z,h);
    /* Same-pointer acb multiplication must still enclose independently selected input members. */
    acb_zero(x->f); arb_add_error_2exp_si(acb_realref(x->f),0);
    arb_add_error_2exp_si(acb_imagref(x->f),0);
    CHECK(adf_ffun_mul(z,x,x,128)==ADF_OK); CHECK(arb_contains_si(acb_realref(z->f),2));
    corners(z->f,x->f,x->f);
    adf_ffun_set(w,x); CHECK(adf_ffun_mul(h,x,w,128)==ADF_OK); equal(z,h);
    acb_set_si(x->f,3); acb_set_si(w->f,7);
    CHECK(adf_ffun_mul(z,x,w,ADF_REAL_PREC_MAX)==ADF_OK); CHECK(acb_equal_si(z->f,21));
    fmpq_set_si(expected,21,1); CHECK(acb_contains_fmpq(z->f,expected));
    fmpq_clear(expected); fmpz_clear(n);
    adf_ffun_clear(x); adf_ffun_clear(w); adf_ffun_clear(z); adf_ffun_clear(h);
}
static int unary_call(const char *op, adf_ffun_t y, const adf_ffun_t x, const adf_rat_t q)
{
    if (!strcmp(op,"translate")) return adf_ffun_translate_rat(y,x,q);
    if (!strcmp(op,"dilate")) return adf_ffun_dilate_rat(y,x,q);
    if (!strcmp(op,"reflect")) return adf_ffun_reflect(y,x);
    CHECK(!strcmp(op,"conj")); return adf_ffun_conj(y,x);
}
static void unary(const char *group)
{
    jsonl_file *file=open_vectors("unary");
    adf_ffun_t x,y,z,h; adf_rat_t q,inv;
    adf_ffun_init(x); adf_ffun_init(y); adf_ffun_init(z); adf_ffun_init(h);
    adf_rat_init(q); adf_rat_init(inv);
    for (size_t i=0;i<jsonl_count(file);i++) {
        const jsonl_value *v=jsonl_record(file,i); const char *op=string(field(v,"op"));
        if (strcmp(group,"all") && strcmp(group,op)) continue;
        records++; input(x,field(v,"x")); load(y,field(v,"y"));
        if (!strcmp(op,"translate") || !strcmp(op,"dilate")) rational(q->q,field(v,"q"));
        CHECK(unary_call(op,z,x,q)==ADF_OK); equal(z,y);
        adf_ffun_set(h,x); CHECK(unary_call(op,h,h,q)==ADF_OK); equal(h,y);
        if (!strcmp(op,"translate")) fmpq_neg(inv->q,q->q);
        else if (!strcmp(op,"dilate")) fmpq_inv(inv->q,q->q);
        CHECK(unary_call(op,h,z,inv)==ADF_OK);
        CHECK(adf_ffun_refine(y,x,h->D,h->M)==ADF_OK); equal(h,y);
        if (!strcmp(op,"conj")) {
            for (ulong j=0;j<x->D*x->M;j++) arb_zero(acb_imagref(x->f+j));
            CHECK(adf_ffun_conj(z,x)==ADF_OK); equal(z,x);
        }
        if (!strcmp(op,"dilate") && fmpq_is_one(q->q)) equal(z,x);
    }
    adf_rat_clear(q); adf_rat_clear(inv); adf_ffun_clear(x); adf_ffun_clear(y);
    adf_ffun_clear(z); adf_ffun_clear(h); jsonl_close(file);
}
static void idele(void)
{
    jsonl_file *file=open_vectors("ideles");
    adf_ffun_t x,y,z,h; adf_idele_t a;
    adf_ffun_init(x); adf_ffun_init(y); adf_ffun_init(z); adf_ffun_init(h); adf_idele_init(a);
    for (size_t i=0;i<jsonl_count(file);i++) {
        const jsonl_value *v=jsonl_record(file,i); records++;
        input(x,field(v,"x")); rational(a->r,field(v,"r"));
        CHECK(fmpz_set_str(a->u.c,string(field(v,"c")),10)==0);
        CHECK(fmpz_set_str(a->u.N,string(field(v,"N")),10)==0);
        CHECK(adf_idele_is_canonical(a));
        for (int alias=0;alias<2;alias++) {
            unsigned char bytes[sizeof(*z)];
            adf_ffun_set(z,x); adf_ffun_set(h,z); memcpy(bytes,z,sizeof(*z));
            int st=adf_ffun_dilate_idele(z,alias ? z : x,a);
            CHECK(!strcmp(adf_status_str(st),string(field(v,"status"))));
            if (st==ADF_OK) { load(y,field(v,"y")); equal(z,y); }
            else preserved(z,h,bytes);
        }
        /* Finite functions ignore the real component, including its sign and radius. */
        arb_set_si(a->inf,-7); arb_add_error_2exp_si(a->inf,-3);
        int st=adf_ffun_dilate_idele(z,x,a);
        CHECK(!strcmp(adf_status_str(st),string(field(v,"status"))));
        if (st==ADF_OK) equal(z,y);
        arb_one(a->inf);
    }
    /* Exhaust the unit-image criterion for small moduli, including L=1 and L=2. */
    for (ulong L=1;L<=16;L++) {
        acb_ptr f=_acb_vec_init((slong)L);
        for (ulong j=0;j<L;j++) acb_set_ui(f+j,j+1);
        CHECK(adf_ffun_set_acb_vec(x,1,L,f,(slong)L)==ADF_OK); _acb_vec_clear(f,(slong)L);
        fmpq_one(a->r);
        for (ulong N=1;N<=16;N++) for (ulong c=1;c<=N;c++) if (n_gcd(c,N)==1) {
            ulong count=0,residue=0,g=n_gcd(L,N);
            /* Independent CRT lifts modulo lcm(N,L), not the implementation's residue filter. */
            ulong mod=L*(N/g);
            for (ulong j=0;j<L;j++) {
                int seen=0;
                for (ulong k=j;k<mod;k+=L) if (n_gcd(k,mod)==1 && k%N==c%N) seen=1;
                if (seen) { count++; residue=j; }
            }
            fmpz_set_ui(a->u.c,c); fmpz_set_ui(a->u.N,N);
            adf_ffun_set(z,x); adf_ffun_set(h,z); unsigned char bytes[sizeof(*z)];
            memcpy(bytes,z,sizeof(*z));
            int st=adf_ffun_dilate_idele(z,x,a);
            CHECK(st==(count==1 ? ADF_OK : ADF_NOT_DETERMINED));
            if (st==ADF_OK) for (ulong j=0;j<L;j++) CHECK(acb_equal(z->f+j,x->f+(residue*j)%L));
            else preserved(z,h,bytes);
        }
    }
    /* Ambiguity is refused even for zero arrays. */
    for (ulong j=0;j<x->D*x->M;j++) acb_zero(x->f+j);
    fmpz_one(a->u.c); fmpz_one(a->u.N);
    CHECK(adf_ffun_dilate_idele(z,x,a)==ADF_NOT_DETERMINED);
    adf_idele_clear(a); adf_ffun_clear(x); adf_ffun_clear(y);
    adf_ffun_clear(z); adf_ffun_clear(h); jsonl_close(file);
}
static void covariance(void)
{
    jsonl_file *file=open_vectors("covariance");
    adf_ffun_t x,left,right,g,t,ref; adf_rat_t q,inv; acb_t factor,diff;
    adf_ffun_init(x); adf_ffun_init(left); adf_ffun_init(right); adf_ffun_init(g);
    adf_ffun_init(t); adf_ffun_init(ref); adf_rat_init(q); adf_rat_init(inv);
    acb_init(factor); acb_init(diff);
    for (size_t i=0;i<jsonl_count(file);i++) {
        const jsonl_value *v=jsonl_record(file,i); records++; input(x,field(v,"x"));
        rational(q->q,field(v,"q")); fmpq_inv(inv->q,q->q);
        CHECK(adf_ffun_dilate_rat(t,x,q)==ADF_OK); CHECK(adf_ffun_fourier(left,t,128)==ADF_OK);
        CHECK(adf_ffun_fourier(g,x,128)==ADF_OK); CHECK(adf_ffun_dilate_rat(right,g,inv)==ADF_OK);
        fmpq_abs(inv->q,q->q); acb_set_fmpq(factor,inv->q,256);
        /* |q|_f = 1/|q|_real, so inverse finite modulus is the positive rational |q|. */
        for (ulong j=0;j<right->D*right->M;j++) acb_mul(right->f+j,right->f+j,factor,128);
        CHECK(left->D==right->D && left->M==right->M);
        for (int side=0;side<2;side++) {
            adf_ffun_srcptr actual=side ? right : left; load(ref,field(v,side ? "rhs" : "lhs"));
            CHECK(actual->D==ref->D && actual->M==ref->M);
            for (ulong j=0;j<actual->D*actual->M;j++) {
                cells++; CHECK(acb_overlaps(actual->f+j,ref->f+j));
                CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(actual->f+j)),-90)<0);
                CHECK(mag_cmp_2exp_si(arb_radref(acb_imagref(actual->f+j)),-90)<0);
            }
        }
        for (ulong j=0;j<left->D*left->M;j++) {
            acb_sub(diff,left->f+j,right->f+j,128); CHECK(acb_contains_zero(diff));
        }
    }
    acb_clear(factor); acb_clear(diff); adf_rat_clear(q); adf_rat_clear(inv);
    adf_ffun_clear(x); adf_ffun_clear(left); adf_ffun_clear(right);
    adf_ffun_clear(g); adf_ffun_clear(t); adf_ffun_clear(ref); jsonl_close(file);
}
/* Guard-page arrays expose invalid reads inside the unsanitized FLINT shared library.
   Allocator hooks: refs/src/flint-3.0.1/memory.rst:9-24. Every hook is restored after cleanup. */
static struct { void *value, *mapping; } guarded[16];
static size_t guard_page;
static void *guard_alloc(size_t n)
{
    if (n!=4*sizeof(acb_struct)) return malloc(n);
    for (int i=0;i<16;i++) if (!guarded[i].value) {
        void *p=mmap(NULL,2*guard_page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        if (p==MAP_FAILED || mprotect(p,guard_page,PROT_NONE)!=0) abort();
        guarded[i].mapping=p; guarded[i].value=(char *)p+guard_page; return guarded[i].value;
    }
    abort();
}
static void guard_free(void *p)
{
    for (int i=0;i<16;i++) if (p && p==guarded[i].value) {
        if (munmap(guarded[i].mapping,2*guard_page)!=0) abort();
        guarded[i].value=NULL; guarded[i].mapping=NULL; return;
    }
    free(p);
}
static void *guard_calloc(size_t n, size_t m)
{
    if (m && n>SIZE_MAX/m) return NULL;
    void *p=guard_alloc(n*m); if (p) memset(p,0,n*m); return p;
}
static void *guard_realloc(void *p, size_t n)
{
    for (int i=0;i<16;i++) if (p && p==guarded[i].value) {
        void *q=guard_alloc(n);
        if (q) memcpy(q,p,FLINT_MIN(n,4*sizeof(acb_struct)));
        guard_free(p); return q;
    }
    return realloc(p,n);
}
static void guarded_fourier(void)
{
    void *(*old_alloc)(size_t); void *(*old_calloc)(size_t,size_t);
    void *(*old_realloc)(void *,size_t); void (*old_free)(void *);
    guard_page=(size_t)sysconf(_SC_PAGESIZE); CHECK(guard_page>=4*sizeof(acb_struct));
    __flint_get_memory_functions(&old_alloc,&old_calloc,&old_realloc,&old_free);
    flint_cleanup(); __flint_set_memory_functions(guard_alloc,guard_calloc,guard_realloc,guard_free);
    adf_ffun_t x,y; adf_ffun_init(x); adf_ffun_init(y);
    acb_ptr f=_acb_vec_init(4); acb_one(f+1);
    CHECK(adf_ffun_set_acb_vec(x,4,1,f,4)==ADF_OK); _acb_vec_clear(f,4);
    CHECK(adf_ffun_fourier(y,x,128)==ADF_OK);
    CHECK(acb_equal_si(y->f,1)); CHECK(acb_equal_si(y->f+2,-1));
    CHECK(arb_equal_si(acb_imagref(y->f+1),-1)); CHECK(arb_equal_si(acb_imagref(y->f+3),1));
    adf_ffun_clear(x); adf_ffun_clear(y); flint_cleanup();
    for (int i=0;i<16;i++) CHECK(guarded[i].value==NULL);
    __flint_set_memory_functions(old_alloc,old_calloc,old_realloc,old_free);
}
static void caps(void)
{
    jsonl_file *file=open_vectors("caps");
    adf_ffun_t x,y,old; adf_rat_t q; adf_idele_t a;
    adf_ffun_init(x); adf_ffun_init(y); adf_ffun_init(old); adf_rat_init(q); adf_idele_init(a);
    acb_set_si(y->f,17); adf_ffun_set(old,y); unsigned char bytes[sizeof(*y)]; memcpy(bytes,y,sizeof(*y));
    for (size_t i=0;i<jsonl_count(file);i++) {
        const jsonl_value *v=jsonl_record(file,i); records++; rational(q->q,field(v,"q"));
        int st=unary_call(string(field(v,"op")),y,x,q);
        CHECK(!strcmp(adf_status_str(st),string(field(v,"status")))); preserved(y,old,bytes);
        adf_ffun_set(x,y); unsigned char xb[sizeof(*x)]; memcpy(xb,x,sizeof(*x));
        st=unary_call(string(field(v,"op")),x,x,q);
        CHECK(!strcmp(adf_status_str(st),string(field(v,"status")))); preserved(x,old,xb);
    }
    fmpz_one(fmpq_numref(q->q)); fmpz_mul_2exp(fmpq_numref(q->q),fmpq_numref(q->q),ADF_FFUN_BITS_MAX);
    fmpz_one(fmpq_denref(q->q));
    CHECK(adf_ffun_translate_rat(y,x,q)==ADF_LIMIT); preserved(y,old,bytes);
    adf_ffun_t probe; adf_ffun_init(probe);
    fmpz_t boundary; fmpz_init(boundary);
    fmpz_fdiv_q_2exp(boundary,fmpq_numref(q->q),1);
    fmpz_set(fmpq_numref(q->q),boundary);
    CHECK(adf_ffun_translate_rat(probe,x,q)==ADF_OK); equal(probe,x);
    fmpq_one(a->r); fmpz_set(a->u.N,boundary); fmpz_sub_ui(a->u.c,boundary,1);
    CHECK(adf_ffun_dilate_idele(probe,x,a)==ADF_OK); equal(probe,x);
    adf_ffun_clear(probe); fmpz_clear(boundary); fmpz_one(a->u.c); fmpz_zero(a->u.N);
    fmpz_mul_2exp(fmpq_numref(q->q),fmpq_numref(q->q),1);
    fmpq_set(a->r,q->q); CHECK(adf_ffun_dilate_idele(y,x,a)==ADF_LIMIT); preserved(y,old,bytes);
    fmpq_one(a->r); fmpz_set(a->u.N,fmpq_numref(q->q)); fmpz_one(a->u.c);
    CHECK(adf_ffun_dilate_idele(y,x,a)==ADF_LIMIT); preserved(y,old,bytes);
    acb_ptr f=_acb_vec_init(1025);
    CHECK(adf_ffun_set_acb_vec(x,1025,1,f,1025)==ADF_OK);
    CHECK(adf_ffun_set_acb_vec(old,1,1025,f,1025)==ADF_OK);
    CHECK(adf_ffun_mul(y,x,old,128)==ADF_LIMIT); /* 1025^2 > 2^20 */
    CHECK(memcmp(y,bytes,sizeof(*y))==0); CHECK(acb_equal_si(y->f,17));
    CHECK(adf_ffun_mul(y,x,x,ADF_REAL_PREC_MAX+1)==ADF_LIMIT);
    /* Refinement and permutation are two charged passes; each alone would fit. */
    fmpq_set_si(q->q,1,513);
    CHECK(adf_ffun_translate_rat(y,x,q)==ADF_LIMIT); CHECK(memcmp(y,bytes,sizeof(*y))==0);
    /* The combined enumeration+output charge exceeds D1 although each array fits. */
    _acb_vec_clear(f,1025); f=_acb_vec_init(1024);
    CHECK(adf_ffun_set_acb_vec(x,1,1024,f,1024)==ADF_OK); _acb_vec_clear(f,1024);
    fmpz_set_ui(a->u.N,1024); fmpq_set_si(a->r,1024,1);
    CHECK(adf_ffun_dilate_idele(y,x,a)==ADF_LIMIT); CHECK(memcmp(y,bytes,sizeof(*y))==0);
    /* Valid allocated inputs may exceed the arithmetic cap; predicates still accept them. */
    adf_ffun_clear(x); x->D=1; x->M=ADF_FFUN_ITEMS_MAX+1; x->f=_acb_vec_init((slong)x->M);
    CHECK(adf_ffun_is_canonical(x));
    CHECK(adf_ffun_reflect(y,x)==ADF_LIMIT); CHECK(memcmp(y,bytes,sizeof(*y))==0);
    /* Size preflight takes precedence over invalid numerical input in an INV build. */
    acb_indeterminate(x->f);
    CHECK(adf_ffun_mul(y,x,x,ADF_REAL_PREC_MAX+1)==ADF_LIMIT);
    CHECK(adf_ffun_reflect(y,x)==ADF_LIMIT); CHECK(memcmp(y,bytes,sizeof(*y))==0);
    acb_zero(x->f);
    CHECK(adf_ffun_conj(y,x)==ADF_LIMIT); CHECK(memcmp(y,bytes,sizeof(*y))==0);
    /* Exact lcm is 2^64+1. Size refusal must precede INV, without forming the overflowing product. */
    adf_ffun_t large_a,large_b; adf_ffun_init(large_a); adf_ffun_init(large_b);
    acb_ptr saved_a=large_a->f, saved_b=large_b->f;
    large_a->D=274177; large_b->D=UWORD(67280421310721);
    large_a->f=NULL; large_b->f=NULL;
    fmpz_t product,power; fmpz_init(product); fmpz_init(power);
    fmpz_set_ui(product,large_a->D); fmpz_mul_ui(product,product,large_b->D);
    fmpz_one(power); fmpz_mul_2exp(power,power,64); fmpz_add_ui(power,power,1);
    CHECK(fmpz_equal(product,power));
    CHECK(adf_ffun_mul(y,large_a,large_b,128)==ADF_LIMIT); CHECK(memcmp(y,bytes,sizeof(*y))==0);
    large_a->D=large_b->D=1; large_a->f=saved_a; large_b->f=saved_b;
    adf_ffun_clear(large_a); adf_ffun_clear(large_b); fmpz_clear(product); fmpz_clear(power);
    adf_rat_clear(q); adf_idele_clear(a); adf_ffun_clear(x); adf_ffun_clear(y); adf_ffun_clear(old);
    jsonl_close(file);
}
#ifdef ADF_CHECK_INVARIANTS
static void invariants(void)
{
    for (int mode=0;mode<19;mode++) {
        int diagnostic_pipe[2]; CHECK(pipe(diagnostic_pipe)==0);
        pid_t pid=fork(); CHECK(pid>=0);
        if (!pid) {
            close(diagnostic_pipe[0]);
            CHECK(dup2(diagnostic_pipe[1],STDERR_FILENO)>=0); close(diagnostic_pipe[1]);
            adf_ffun_t x,y,z; adf_rat_t q; adf_idele_t a;
            adf_ffun_init(x); adf_ffun_init(y); adf_ffun_init(z);
            adf_rat_init(q); adf_rat_one(q); adf_idele_init(a);
            if (mode<3) {
                acb_indeterminate((mode==0 ? x : mode==1 ? y : z)->f);
                (void)adf_ffun_mul(z,x,y,128);
            } else if (mode<11) {
                acb_indeterminate((mode%2 ? x : z)->f);
                const char *op=mode<5 ? "translate" : mode<7 ? "dilate" : mode<9 ? "reflect" : "conj";
                (void)unary_call(op,z,x,q);
            } else if (mode<14) {
                fmpz_zero(fmpq_denref(q->q));
                if (mode==11) (void)adf_ffun_translate_rat(z,x,q);
                else if (mode==12) (void)adf_ffun_dilate_rat(z,x,q);
                else { fmpq_set(a->r,q->q); (void)adf_ffun_dilate_idele(z,x,a); }
            } else {
                if (mode==14) acb_indeterminate(x->f);
                if (mode==15) acb_indeterminate(z->f);
                if (mode==16) arb_zero(a->inf);
                if (mode==17) fmpz_zero(a->u.c);
                if (mode==18) fmpq_zero(a->r);
                (void)adf_ffun_dilate_idele(z,x,a);
            }
            /* Reached only if the entry check is missing. The clears are for tools/memcheck. */
            adf_ffun_clear(x); adf_ffun_clear(y); adf_ffun_clear(z); adf_rat_clear(q); adf_idele_clear(a);
            _exit(0);
        }
        close(diagnostic_pipe[1]);
        int st; CHECK(waitpid(pid,&st,0)==pid); CHECK(WIFSIGNALED(st) && WTERMSIG(st)==SIGABRT);
        char diagnostic[512]; ssize_t n=read(diagnostic_pipe[0],diagnostic,sizeof(diagnostic)-1);
        CHECK(n>0); close(diagnostic_pipe[0]); diagnostic[n]='\0';
        const char *fn=mode<3 ? "adf_ffun_mul:" : mode<5 || mode==11 ? "adf_ffun_translate_rat:" :
                       mode<7 || mode==12 ? "adf_ffun_dilate_rat:" : mode<9 ? "adf_ffun_reflect:" :
                       mode<11 ? "adf_ffun_conj:" : "adf_ffun_dilate_idele:";
        CHECK(strstr(diagnostic,fn)!=NULL); /* Entry failure, before a later swap checks the output. */
    }
}
#endif
int main(int argc, char **argv)
{
    struct rlimit r={0,0}; (void)setrlimit(RLIMIT_CORE,&r);
    const char *group=argc>1 ? argv[1] : "all"; inputs=open_vectors("inputs");
    if (!strcmp(group,"all") || !strcmp(group,"mul")) { product(); numeric_product(); }
    unary(group);
    if (!strcmp(group,"all") || !strcmp(group,"idele")) idele();
    if (!strcmp(group,"all") || !strcmp(group,"covariance")) covariance();
    if (!strcmp(group,"all") || !strcmp(group,"caps")) caps();
    if (!strcmp(group,"all") || !strcmp(group,"guard")) guarded_fourier();
#ifdef ADF_CHECK_INVARIANTS
    if (!strcmp(group,"all") || !strcmp(group,"inv")) invariants();
#endif
    jsonl_close(inputs); flint_cleanup();
    printf("test_ffun_algebra: %lu checks, %lu records, %lu cells\n",checks,records,cells);
    return 0;
}
