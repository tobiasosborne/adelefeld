#include <adelefeld.h>
#include <flint/mag.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
static unsigned long checks, bad, calls;
#define C(x) do { checks++; if (!(x)) { bad++; if (bad < 20) printf("line=%d %s\n", __LINE__, #x); } } while (0)
typedef int (*sfn)(adf_adele_t, adf_place_t *, const adf_adele_t, slong);
static sfn fs[] = {adf_adele_exp, adf_adele_sin, adf_adele_sinh, adf_adele_cos, adf_adele_cosh};
static adf_place_t sent(void) { adf_place_t w; C(adf_place_prime(&w, 97) == ADF_OK); return w; }
static int place(adf_place_t w, int want) {
    return want == -1 ? adf_place_equal(w, sent()) :
        want == 0 ? adf_place_is_archimedean(w) : adf_place_prime_get(w) == (ulong)want;
}
int main(void) {
    adf_adele_t x,y,t,a; adf_idele_t i,j,jt,it;
    adf_adele_init(x); adf_adele_init(y); adf_adele_init(t); adf_adele_init(a);
    adf_idele_init(i); adf_idele_init(j); adf_idele_init(jt); adf_idele_init(it);
    adf_modctx_struct *ctx = NULL; ulong block = 8;
    C(adf_modctx_new_blocks(&ctx, &block, 1) == ADF_OK);
    slong precisions[] = {LONG_MIN, -1, 0, 1, 2, 64, ADF_REAL_PREC_MAX, ADF_REAL_PREC_MAX+1};
    ulong degrees[] = {0,1,2,3,WORD_MAX,(ulong)WORD_MAX+1,UWORD_MAX};
    int signs[] = {INT_MIN,-1,0,1,2,INT_MAX};
    unsigned long matrix = 0, skipped = 0;
    for (int real=0; real<5; real++) for (int finite=0; finite<6; finite++) {
        arb_set_si(x->inf, real == 1 ? -1 : real == 2 ? 0 : 1);
        if (real == 3) mag_one(arb_radref(x->inf));
        if (real == 4) { arb_one(x->inf); arb_mul_2exp_si(x->inf,x->inf,1000); }
        if (finite < 3) adf_fball_set_si(&x->fin, finite == 0 ? 0 : finite == 1 ? 1 : -1);
        else {
            fmpz_set_si(x->fin.A, finite == 3 ? 0 : 4); fmpz_set_ui(x->fin.H, finite == 3 ? 4 : 8);
            fmpz_one(x->fin.d); C(adf_fball_canonicalise(&x->fin) == ADF_OK);
            if (finite == 5) C(adf_fball_set_local(&x->fin,&x->fin,ctx) == ADF_OK);
        }
        C(adf_adele_is_canonical(x));
        for (unsigned np=0; np<sizeof(precisions)/sizeof(*precisions); np++) {
            slong p = precisions[np];
            for (unsigned nd=0; nd<sizeof(degrees)/sizeof(*degrees); nd++)
                for (unsigned ns=0; ns<sizeof(signs)/sizeof(*signs); ns++) {
                    ulong n=degrees[nd]; int s=signs[ns], st=ADF_OK, wp=-1;
                    if (p == ADF_REAL_PREC_MAX && real >= 3 && n >= 2 && (s == 1 || s == -1)) {
                        skipped += 3; continue;
                    }
                    if (p>ADF_REAL_PREC_MAX) { st=ADF_LIMIT; wp=0; }
                    else if (!n) st=ADF_DOMAIN;
                    else if (n==1) st=ADF_OK;
                    else if (s!=1 && s!=-1) st=ADF_DOMAIN;
                    else if (s==-1 && n%2 && !(real==2 && finite==0)) st=ADF_DOMAIN;
                    else {
                        int sr = (n%2==0 && real==1) ? ADF_DOMAIN : ADF_OK;
                        /* real=3 is [0,2], wholly inside even-root domain. */
                        int sf = finite>=3 ? ADF_NOT_DETERMINED :
                            finite==2 && n%2==0 ? ADF_DOMAIN : ADF_OK;
                        st=sr>sf?sr:sf; if (st && st==sr) wp=0;
                    }
                    for (int mode=0; mode<3; mode++) {
                        adf_place_t w=sent(); adf_adele_set(a,x);
                        if (mode==1) adf_adele_set(y,x);
                        else { arb_set_si(y->inf,123); adf_fball_set_si(&y->fin,123); }
                        adf_adele_set(t,y);
                        int got=adf_adele_root(y,mode==2?NULL:&w,mode==1?y:x,n,s,p); calls++; matrix++;
                        C(got==st); C(mode==2 || place(w,wp));
                        if (st) C(adf_adele_identical(y,t));
                        else { C(adf_adele_is_canonical(y)); if (n==1) C(adf_adele_identical(y,a)); }
                    }
                }
            for (int fn=0; fn<5; fn++) {
                if (p == ADF_REAL_PREC_MAX && real != 2) { skipped += 3; continue; }
                int st=ADF_OK, wp=-1;
                if (p>ADF_REAL_PREC_MAX) { st=ADF_LIMIT; wp=0; }
                else if (finite==1 || finite==2) { st=ADF_DOMAIN; wp=2; }
                else {
                    if (finite>=3) st=ADF_NOT_DETERMINED;
                    if (real==4 && (fn==0 || fn==2 || fn==4)) { st=ADF_NOT_DETERMINED; wp=0; }
                }
                for (int mode=0; mode<3; mode++) {
                    adf_place_t w=sent(); if(mode==1) adf_adele_set(y,x);
                    else { arb_set_si(y->inf,123); adf_fball_set_si(&y->fin,123); }
                    adf_adele_set(t,y);
                    int got=fs[fn](y,mode==2?NULL:&w,mode==1?y:x,p); calls++;
                    C(got==st); C(mode==2 || place(w,wp));
                    if(st) C(adf_adele_identical(y,t)); else C(adf_adele_is_canonical(y));
                }
            }
        }
    }
    /* Degree 1 preserves nonnormal but canonical units exactly, at every precision and selector. */
    const char *txt="(1 +/- 0.25 ; 1 * [5 mod 6])";
    C(adf_idele_set_str(i,txt,strlen(txt),64,NULL)==ADF_OK);
    C(adf_idele_is_canonical(i)); C(!adf_ucoset_is_normal(&i->u));
    unsigned long identity=0;
    for(unsigned np=0;np<sizeof(precisions)/sizeof(*precisions);np++)
        for(unsigned ns=0;ns<sizeof(signs)/sizeof(*signs);ns++)
            for(int mode=0;mode<3;mode++) {
                adf_place_t w=sent(); adf_idele_set(it,i);
                if(mode==1) adf_idele_set(j,i); else adf_idele_set(j,jt);
                adf_idele_set(jt,j);
                int st=adf_idele_root(j,mode==2?NULL:&w,mode==1?j:i,1,signs[ns],precisions[np]);
                calls++; identity++;
                int limit=precisions[np]>ADF_REAL_PREC_MAX;
                C(st==(limit?ADF_LIMIT:ADF_OK)); C(mode==2 || place(w,limit?0:-1));
                C(adf_idele_identical(j,limit?jt:it));
            }
    printf("root_matrix_calls=%lu idele_identity_calls=%lu calls=%lu checks=%lu failures=%lu skipped_cost=%lu\n",
           matrix,identity,calls,checks,bad,skipped);
    adf_adele_clear(x); adf_adele_clear(y); adf_adele_clear(t); adf_adele_clear(a);
    adf_idele_clear(i); adf_idele_clear(j); adf_idele_clear(jt); adf_idele_clear(it);
    adf_modctx_free(ctx); flint_cleanup(); return bad?1:0;
}
