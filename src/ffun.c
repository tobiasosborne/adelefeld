/* Slice 4a. Sources: docs/api-4.md F1-F4; docs/proofs/analysis.md P4:150-173;
   refs/src/tate-poonen/notes.txt:693-700,733-740 (character convention);
   refs/src/flint-3.0.1/arb.rst:6-12, acb.rst:6-12 (outward ball arithmetic).
   No global state. Every status call builds privately and commits once. */
#include "adelefeld/ffun.h"
#include "adelefeld/psi.h"
#include "invariants.h"
#include <stdint.h>

#ifdef ADF_CHECK_INVARIANTS
static ADF_INV_NOINLINE void ffun_entry(const adf_ffun_struct *x, const char *fn, const char *arg)
{
    if (!adf_ffun_is_canonical(x)) adf_inv_fail(fn,arg,"adf_ffun");
}
#define ADF_INV_FFUN(x) ffun_entry(x,__func__,#x)
#else
#define ADF_INV_FFUN(x) ((void)0)
#endif

/* D1: divide before multiplying; no unbounded product or allocation is formed. */
static int ffun_shape(ulong D, ulong M)
{
    if (D==0 || M==0) return ADF_DOMAIN;
    if (D>ADF_FFUN_ITEMS_MAX/M) return ADF_LIMIT;
    if (D*M>(ulong)WORD_MAX || D*M>SIZE_MAX/sizeof(acb_struct)) return ADF_LIMIT;
    return ADF_OK;
}
static void ffun_allocate(adf_ffun_t x, ulong D, ulong M)
{
    x->D=D; x->M=M; x->f=_acb_vec_init((slong)(D*M));
}
/* conventions 5.11:760-770: owned zero initialization, no normalization. */
void adf_ffun_init(adf_ffun_t x) { ffun_allocate(x,1,1); }
static void ffun_dispose(adf_ffun_t x)
{
    _acb_vec_clear(x->f,(slong)(x->D*x->M));
}
void adf_ffun_clear(adf_ffun_t x)
{
    ADF_INV_FFUN(x); ffun_dispose(x);
}
void adf_ffun_swap(adf_ffun_t x, adf_ffun_t y)
{
    adf_ffun_struct t;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y); t=*x; *x=*y; *y=t;
}
int adf_ffun_is_canonical(const adf_ffun_t x)
{
    ulong j,L;
    if (x->D==0 || x->M==0 || x->f==NULL || x->D>((UWORD(1)<<62)-1)/x->M) return 0;
    L=x->D*x->M;
    for (j=0;j<L;j++) if (!acb_is_finite(x->f+j)) return 0;
    return 1;
}
void adf_ffun_set(adf_ffun_t y, const adf_ffun_t x)
{
    adf_ffun_t t;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y);
    if (x==y) return;
    ffun_allocate(t,x->D,x->M); _acb_vec_set(t->f,x->f,(slong)(x->D*x->M));
    adf_ffun_swap(y,t); adf_ffun_clear(t);
}
int adf_ffun_identical(const adf_ffun_t x, const adf_ffun_t y)
{
    ADF_INV_FFUN(x); ADF_INV_FFUN(y);
    if (x->D!=y->D || x->M!=y->M) return 0;
    for (ulong j=0;j<x->D*x->M;j++) if (!acb_equal(x->f+j,y->f+j)) return 0;
    return 1;
}
/* docs/api-4.md section 1: raw input validated, no numerical conversion. */
int adf_ffun_set_acb_vec(adf_ffun_t y, ulong D, ulong M, acb_srcptr f, slong n)
{
    adf_ffun_t t;
    int st=ffun_shape(D,M);
    if (st!=ADF_OK) return st;
    ADF_INV_FFUN(y);
    if (n<0 || (ulong)n!=D*M || f==NULL) return ADF_DOMAIN;
    for (slong j=0;j<n;j++) if (!acb_is_finite(f+j)) return ADF_DOMAIN;
    ffun_allocate(t,D,M); _acb_vec_set(t->f,f,n);
    adf_ffun_swap(y,t); adf_ffun_clear(t); return ADF_OK;
}
/* F1, docs/api-4.md:119-129. Holes are zero; old cosets repeat modulo DM. */
int adf_ffun_refine(adf_ffun_t y, const adf_ffun_t x, ulong D2, ulong M2)
{
    adf_ffun_t t;
    int st=ffun_shape(D2,M2);
    if (st==ADF_LIMIT) return st;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y);
    if (st!=ADF_OK || D2%x->D || M2%x->M) return ADF_DOMAIN;
    ffun_allocate(t,D2,M2);
    ulong r=D2/x->D, L=x->D*x->M;
    for (ulong k=0;k<D2*M2;k++)
        if (k%r==0) acb_set(t->f+k,x->f+(k/r)%L);
    adf_ffun_swap(y,t); adf_ffun_clear(t); return ADF_OK;
}
/* F1 common refinement. Project lcm before multiplying; the operands are machine words. */
static int ffun_lcm(ulong *out, ulong a, ulong b)
{
    ulong u=a,v=b;
    if (!a || !b) { *out=0; return ADF_OK; }
    while (v) { ulong r=u%v; u=v; v=r; }
    a/=u;
    if (a>ADF_FFUN_ITEMS_MAX/b) return ADF_LIMIT;
    *out=a*b; return ADF_OK;
}
int adf_ffun_add(adf_ffun_t z, const adf_ffun_t x, const adf_ffun_t y, slong prec)
{
    adf_ffun_t t;
    ulong D,M;
    int st;
    if (prec>ADF_REAL_PREC_MAX) return ADF_LIMIT;
    if (ffun_lcm(&D,x->D,y->D)!=ADF_OK || ffun_lcm(&M,x->M,y->M)!=ADF_OK) return ADF_LIMIT;
    st=ffun_shape(D,M); if (st==ADF_LIMIT) return st;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y); ADF_INV_FFUN(z);
    if (st!=ADF_OK) return st; /* Invalid precondition in a non-INV build. */
    ffun_allocate(t,D,M);
    ulong rx=D/x->D, ry=D/y->D, lx=x->D*x->M, ly=y->D*y->M;
    slong p=FLINT_MAX(prec,2);
    for (ulong k=0;k<D*M;k++) {
        if (k%rx==0 && k%ry==0) acb_add(t->f+k,x->f+(k/rx)%lx,y->f+(k/ry)%ly,p);
        else if (k%rx==0) acb_set(t->f+k,x->f+(k/rx)%lx);
        else if (k%ry==0) acb_set(t->f+k,y->f+(k/ry)%ly);
        if (!acb_is_finite(t->f+k)) { ffun_dispose(t); return ADF_NOT_DETERMINED; }
    }
    adf_ffun_swap(z,t); adf_ffun_clear(t); return ADF_OK;
}
/* F4: the caller supplies the NEGATIVE finite angle, not the phase evaluator.
   Products fit ulong because the checked direct cap gives j,k<1024, hence jk<2^20.
   Each acb operation includes its midpoint error and the propagation of both input radii.
   Division by M is also outward. P4:150-173 fixes the weight and exchanged layout. */
int adf_ffun_fourier(adf_ffun_t y, const adf_ffun_t x, slong prec)
{
    adf_ffun_t t;
    acb_t phase,term;
    fmpq_t theta;
    ulong L;
    int st;
    if (prec>ADF_REAL_PREC_MAX) return ADF_LIMIT;
    st=ffun_shape(x->D,x->M); if (st==ADF_LIMIT) return st;
    L=x->D*x->M;
    if (L && L>ADF_FFUN_ITEMS_MAX/L) return ADF_LIMIT;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y);
    if (st!=ADF_OK) return st;
    ffun_allocate(t,x->M,x->D);
    acb_init(phase); acb_init(term); fmpq_init(theta);
    slong p=FLINT_MAX(prec,2);
    for (ulong k=0;k<L;k++) {
        for (ulong j=0;j<L;j++) {
            ulong h=(j*k)%L;
            fmpq_set_ui(theta,h ? L-h : 0,L); fmpq_canonicalise(theta);
            st=adf_phase_get_acb(phase,theta,p);
            if (st!=ADF_OK) goto done;
            acb_mul(term,x->f+j,phase,p);
            acb_add(t->f+k,t->f+k,term,p);
        }
        acb_div_ui(t->f+k,t->f+k,x->M,p);
        if (!acb_is_finite(t->f+k)) { st=ADF_NOT_DETERMINED; goto done; }
    }
    adf_ffun_swap(y,t);
done:
    fmpq_clear(theta); acb_clear(phase); acb_clear(term); ffun_dispose(t); return st;
}
