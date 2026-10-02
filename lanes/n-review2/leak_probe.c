/* FLINT hooks: refs/src/flint-3.0.1/memory.rst:16-21; cleanup: same file:29-35.
   GMP hook signatures are read from the installed gmp.h, not cited as a numerical theorem. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>

static void *(*fm)(size_t), *(*fc)(size_t,size_t), *(*fr)(void *,size_t);
static void (*ff)(void *);
static void *(*gm)(size_t), *(*gr)(void *,size_t,size_t);
static void (*gf)(void *,size_t);
static long live;
static unsigned long calls, peak;
static void added(void *p) { if(p) { live++; calls++; if((unsigned long)live>peak) peak=live; } }
static void *hm(size_t n) { void *p=fm(n); added(p); return p; }
static void *hc(size_t n,size_t s) {void *p=fc(n,s); added(p); return p;}
static void *hr(void *p,size_t n) {void *q=fr(p,n); if(!p) added(q); return q;}
static void hf(void *p) {if(p) live--; ff(p);}
static void *hgm(size_t n) {void *p=gm(n); added(p); return p;}
static void *hgr(void *p,size_t o,size_t n) {void *q=gr(p,o,n); if(!p) added(q); return q;}
static void hgf(void *p,size_t n) {if(p) live--; gf(p,n);}

int main(void)
{
    adf_idele_t id;
    adf_idclass_t cl;
    fmpz_t z,e;
    int fail=0;
    flint_cleanup();
    __flint_get_memory_functions(&fm,&fc,&fr,&ff);
    mp_get_memory_functions(&gm,&gr,&gf);
    __flint_set_memory_functions(hm,hc,hr,hf);
    mp_set_memory_functions(hgm,hgr,hgf);
    adf_idele_init(id); adf_idclass_init(cl); fmpz_init(z); fmpz_init_set_si(e,-1);
    fmpz_one(z); fmpz_mul_2exp(z,z,100000); fmpz_add_ui(z,z,1);
    arf_set_fmpz_2exp(arb_midref(id->inf),z,e);
    mag_one(arb_radref(id->inf)); mag_mul_2exp_si(arb_radref(id->inf),arb_radref(id->inf),99999);
    arb_set(cl->t,id->inf);
    for(int i=0;i<3;i++) {
        size_t len=17; char *s;
        if(i==2) arb_neg(id->inf,id->inf);
        if(i==1) s=adf_idclass_get_str(&len,cl,1);
        else s=adf_idele_get_str(&len,id,1);
        printf("case=%d null=%d len=%zu\n",i,s==NULL,len);
        if(s || len) fail++;
        adf_str_free(s);
    }
    adf_idele_clear(id); adf_idclass_clear(cl); fmpz_clear(z); fmpz_clear(e); flint_cleanup();
    __flint_set_memory_functions(fm,fc,fr,ff); mp_set_memory_functions(gm,gr,gf);
    printf("allocation_events=%lu peak_live=%lu final_live=%ld failures=%d\n",calls,peak,live,fail);
    return fail || live;
}
