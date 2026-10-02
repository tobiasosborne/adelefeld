/* Focused INV reproducer. Only these two read-only implementation files are included;
   the rest is linked from the one normal archive. Allocator API: refs/src/flint-3.0.1/memory.rst:16-21. */
#define ADF_CHECK_INVARIANTS
#include "../../src/sball.c"
#include "../../src/rfunc.c"
#include <limits.h>

static void *(*saved_malloc)(size_t), *(*saved_calloc)(size_t,size_t), *(*saved_realloc)(void *,size_t);
static void (*saved_free)(void *);
static size_t alloc_calls, alloc_bytes;
static void *hook_malloc(size_t n) { alloc_calls++; alloc_bytes+=n; return saved_malloc(n); }
static void *hook_calloc(size_t n,size_t s) { alloc_calls++; alloc_bytes+=n*s; return saved_calloc(n,s); }
static void *hook_realloc(void *p,size_t n) { alloc_calls++; alloc_bytes+=n; return saved_realloc(p,n); }
static void hook_free(void *p) { saved_free(p); }

int main(void)
{
    adf_lball_t l;
    adf_sball_t x,y;
    arb_t a,b;
    adf_place_t where, inf=adf_place_inf();
    const slong precs[]={ADF_REAL_PREC_MAX+1,LONG_MAX};
    int fail=0, total=0;
    adf_lball_init(l); adf_sball_init(x); adf_sball_init(y); arb_init(a); arb_init(b); arb_one(a);
    l->p=5; l->exact=1;
    fmpz_one(fmpq_numref(l->u)); fmpz_mul_2exp(fmpq_numref(l->u),fmpq_numref(l->u),4096);
    fmpz_add_ui(fmpq_numref(l->u),fmpq_numref(l->u),3);
    fmpz_one(fmpq_denref(l->u)); fmpz_mul_2exp(fmpq_denref(l->u),fmpq_denref(l->u),2048);
    fmpz_add_ui(fmpq_denref(l->u),fmpq_denref(l->u),7); fmpq_canonicalise(l->u);
    if(adf_sball_set_arb_lballs(x,NULL,a,l,1)!=ADF_OK) return 2;
    arb_set_si(b,17);
    __flint_get_memory_functions(&saved_malloc,&saved_calloc,&saved_realloc,&saved_free);
    __flint_set_memory_functions(hook_malloc,hook_calloc,hook_realloc,hook_free);
    /* Positive control: the invariant check must exercise the hooks. */
    flint_cleanup(); alloc_calls=alloc_bytes=0;
    int canonical=adf_sball_is_canonical(x);
    printf("control canonical=%d calls=%zu bytes=%zu\n",canonical,alloc_calls,alloc_bytes);
    if(!canonical || alloc_calls==0) fail++;
    for(int j=0;j<2;j++) for(int i=0;i<18;i++) {
        slong p=precs[j]; int st;
        flint_cleanup(); alloc_calls=alloc_bytes=0;
        (void)adf_place_prime(&where,3);
        switch(i) {
        case 0: st=adf_sball_add(y,&where,x,x,p); break;
        case 1: st=adf_sball_sub(y,&where,x,x,p); break;
        case 2: st=adf_sball_mul(y,&where,x,x,p); break;
        case 3: st=adf_sball_exp_at(y,&where,x,inf,p); break;
        case 4: st=adf_sball_log_at(y,&where,x,inf,p); break;
        case 5: st=adf_sball_Log_at(y,&where,x,inf,p); break;
        case 6: st=adf_sball_log_abs_at(y,&where,x,inf,p); break;
        case 7: st=adf_sball_sin_at(y,&where,x,inf,p); break;
        case 8: st=adf_sball_cos_at(y,&where,x,inf,p); break;
        case 9: st=adf_sball_sqrt_at(y,&where,x,inf,p); break;
        case 10: st=adf_sball_root_at(y,&where,x,inf,0,p); break;
        case 11: st=adf_real_exp(b,a,p); break;
        case 12: st=adf_real_log(b,a,p); break;
        case 13: st=adf_real_log_abs(b,a,p); break;
        case 14: st=adf_real_sin(b,a,p); break;
        case 15: st=adf_real_cos(b,a,p); break;
        case 16: st=adf_real_sqrt(b,a,p); break;
        default: st=adf_real_root(b,a,0,p); break;
        }
        int ok=st==ADF_LIMIT && alloc_calls==0 && y->arch==0 && y->len==0 && arb_equal_si(b,17);
        if(i<11) ok=ok && adf_place_is_archimedean(where);
        printf("function=%d prec=%ld status=%d calls=%zu bytes=%zu untouched=%d\n",
               i,p,st,alloc_calls,alloc_bytes,ok);
        total++; if(!ok) fail++;
    }
    __flint_set_memory_functions(saved_malloc,saved_calloc,saved_realloc,saved_free);
    adf_lball_clear(l); adf_sball_clear(x); adf_sball_clear(y); arb_clear(a); arb_clear(b); flint_cleanup();
    printf("precision_calls=%d failures=%d\n",total,fail);
    return fail!=0;
}
