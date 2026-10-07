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

/* Slice 4b. Own finite-index proofs: docs/api-4.md:119-146 (F1-F3).
   Ball enclosure: refs/src/flint-3.0.1/arb.rst:6-12, acb.rst:6-12. */
/* Fix operand order by stored balls, so commutativity is representation identity even when
   directed radius rounding in acb_mul differs by operand order. No numerical data are rounded. */
static int ffun_ball_cmp(const acb_t a, const acb_t b)
{
    int c=arf_cmp(arb_midref(acb_realref(a)),arb_midref(acb_realref(b)));
    if (!c) c=mag_cmp(arb_radref(acb_realref(a)),arb_radref(acb_realref(b)));
    if (!c) c=arf_cmp(arb_midref(acb_imagref(a)),arb_midref(acb_imagref(b)));
    if (!c) c=mag_cmp(arb_radref(acb_imagref(a)),arb_radref(acb_imagref(b)));
    return c;
}
int adf_ffun_mul(adf_ffun_t z, const adf_ffun_t x, const adf_ffun_t y, slong prec)
{
    adf_ffun_t t;
    acb_t copy;
    ulong D,M;
    int st;
    if (prec>ADF_REAL_PREC_MAX) return ADF_LIMIT;
    if (ffun_lcm(&D,x->D,y->D)!=ADF_OK || ffun_lcm(&M,x->M,y->M)!=ADF_OK) return ADF_LIMIT;
    st=ffun_shape(D,M); if (st==ADF_LIMIT) return st;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y); ADF_INV_FFUN(z);
    if (st!=ADF_OK) return st;
    ffun_allocate(t,D,M); acb_init(copy);
    ulong rx=D/x->D, ry=D/y->D, lx=x->D*x->M, ly=y->D*y->M;
    slong p=FLINT_MAX(prec,2);
    for (ulong k=0;k<D*M;k++) {
        if (k%rx==0 && k%ry==0) {
            acb_srcptr a=x->f+(k/rx)%lx, b=y->f+(k/ry)%ly;
            if (ffun_ball_cmp(a,b)>0) { acb_srcptr c=a; a=b; b=c; }
            /* acb.rst:463-468 permits a same-pointer squaring shortcut. Use independent balls
               here, so the product family and its representation do not depend on input aliasing. */
            if (a==b) { acb_set(copy,b); b=copy; }
            acb_mul(t->f+k,a,b,p);
        }
        if (!acb_is_finite(t->f+k)) { acb_clear(copy); ffun_dispose(t); return ADF_NOT_DETERMINED; }
    }
    acb_clear(copy); adf_ffun_swap(z,t); adf_ffun_clear(t); return ADF_OK;
}
/* D1 integer input preflight uses bit counts without forming a product. */
static int ffun_rat_bits(const fmpq_t q)
{
    return fmpz_bits(fmpq_numref(q))<=ADF_FFUN_BITS_MAX &&
           fmpz_bits(fmpq_denref(q))<=ADF_FFUN_BITS_MAX;
}
/* F2, docs/api-4.md:130-135. Reduce the numerator before its bounded word product. */
int adf_ffun_translate_rat(adf_ffun_t y, const adf_ffun_t x, const adf_rat_t q)
{
    adf_ffun_t t;
    ulong D,M=x->M,den;
    int st;
    if (!ffun_rat_bits(q->q) || fmpz_cmp_ui(fmpq_denref(q->q),ADF_FFUN_ITEMS_MAX)>0)
        return ADF_LIMIT;
    den=fmpz_sgn(fmpq_denref(q->q))>0 ? fmpz_get_ui(fmpq_denref(q->q)) : 0;
    if (ffun_lcm(&D,x->D,den)!=ADF_OK) return ADF_LIMIT;
    st=ffun_shape(D,M); if (st==ADF_LIMIT) return st;
    if (st==ADF_OK && D!=x->D && D*M>ADF_FFUN_ITEMS_MAX/2) return ADF_LIMIT;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y); ADF_INV_RAT(q);
    if (st!=ADF_OK) return st;
    /* Reuse F1 only when the denominator requires refinement. The work cap charges both passes. */
    adf_ffun_srcptr source=x;
    if (D!=x->D) {
        adf_ffun_init(t);
        st=adf_ffun_refine(t,x,D,M);
        if (st!=ADF_OK) { adf_ffun_clear(t); return st; }
        source=t;
    }
    adf_ffun_t out;
    ffun_allocate(out,D,M);
    ulong L=D*M, shift=(fmpz_fdiv_ui(fmpq_numref(q->q),L)*(D/den))%L;
    for (ulong k=0;k<L;k++) acb_set(out->f+k,source->f+(k+L-shift)%L);
    if (D!=x->D) adf_ffun_clear(t);
    adf_ffun_swap(y,out); adf_ffun_clear(out); return ADF_OK;
}
/* F3 size projection: no large numerator*D or denominator*M is ever formed. */
static int ffun_dilate_shape(ulong *D, ulong *M, const adf_ffun_t x, const fmpq_t q)
{
    if (!ffun_rat_bits(q)) return ADF_LIMIT;
    if (!x->D || !x->M) return ADF_DOMAIN;
    ulong bound=ADF_FFUN_ITEMS_MAX/x->D;
    if (fmpz_cmp_si(fmpq_numref(q),-(slong)bound)<0 || fmpz_cmp_ui(fmpq_numref(q),bound)>0 ||
        fmpz_cmp_ui(fmpq_denref(q),ADF_FFUN_ITEMS_MAX/x->M)>0) return ADF_LIMIT;
    if (fmpz_sgn(fmpq_denref(q))<=0) return ADF_DOMAIN;
    slong s=fmpz_get_si(fmpq_numref(q));
    *D=(ulong)(s<0 ? -s : s)*x->D;
    *M=fmpz_get_ui(fmpq_denref(q))*x->M;
    return ffun_shape(*D,*M);
}
/* F3, docs/api-4.md:137-142. Holes remain initialized zero. Unit is a residue modulo old DM. */
static void ffun_dilate_cells(adf_ffun_t out, const adf_ffun_t x, const fmpq_t q, ulong unit)
{
    ulong t=fmpz_get_ui(fmpq_denref(q)), L=x->D*x->M;
    int negative=fmpq_sgn(q)<0;
    for (ulong k=0;k<out->D*out->M;k++) if (k%t==0) {
        ulong j=(unit*((k/t)%L))%L;
        if (negative && j) j=L-j;
        acb_set(out->f+k,x->f+j);
    }
}
int adf_ffun_dilate_rat(adf_ffun_t y, const adf_ffun_t x, const adf_rat_t q)
{
    adf_ffun_t t;
    ulong D=0,M=0;
    int st=ffun_dilate_shape(&D,&M,x,q->q);
    if (st==ADF_LIMIT) return st;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y); ADF_INV_RAT(q);
    if (fmpq_is_zero(q->q)) return ADF_DOMAIN;
    if (st!=ADF_OK) return st;
    ffun_allocate(t,D,M); ffun_dilate_cells(t,x,q->q,1);
    adf_ffun_swap(y,t); adf_ffun_clear(t); return ADF_OK;
}
#include <flint/ulong_extras.h>
#ifdef ADF_CHECK_INVARIANTS
static ADF_INV_NOINLINE void ffun_idele_entry(const adf_idele_t a, const char *fn)
{
    if (!adf_idele_is_canonical(a)) adf_inv_fail(fn,"a","adf_idele");
}
#define ADF_FFUN_INV_IDELE(a) ffun_idele_entry(a,__func__)
#else
#define ADF_FFUN_INV_IDELE(a) ((void)0)
#endif
/* F3.3-4, docs/api-4.md:143-146: enumerate the full compatible unit image, including
   singleton images with L not dividing N. The real component is ignored by the array operation. */
int adf_ffun_dilate_idele(adf_ffun_t y, const adf_ffun_t x, const adf_idele_t a)
{
    adf_ffun_t t;
    ulong D=0,M=0,unit=0;
    int st=ffun_shape(x->D,x->M);
    if (st==ADF_LIMIT || fmpz_bits(a->u.c)>ADF_FFUN_BITS_MAX || fmpz_bits(a->u.N)>ADF_FFUN_BITS_MAX)
        return ADF_LIMIT;
    st=ffun_dilate_shape(&D,&M,x,a->r); if (st==ADF_LIMIT) return st;
    ulong L=x->D*x->M;
    int enumerate=!fmpz_is_zero(a->u.N);
    if (st==ADF_OK && enumerate && L>ADF_FFUN_ITEMS_MAX-D*M) return ADF_LIMIT;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y); ADF_FFUN_INV_IDELE(a);
    if (st!=ADF_OK) return st;
    if (!enumerate) unit=fmpz_fdiv_ui(a->u.c,L);
    else {
        ulong g=n_gcd(L,fmpz_fdiv_ui(a->u.N,L)), c=fmpz_fdiv_ui(a->u.c,g), count=0;
        for (ulong v=0;v<L;v++) if (n_gcd(v,L)==1 && v%g==c) {
            unit=v;
            if (++count>1) return ADF_NOT_DETERMINED;
        }
        /* A canonical unit coset always has a nonempty image (F3.3). */
        if (count!=1) return ADF_NOT_DETERMINED;
    }
    ffun_allocate(t,D,M); ffun_dilate_cells(t,x,a->r,unit);
    adf_ffun_swap(y,t); adf_ffun_clear(t); return ADF_OK;
}
/* F2, docs/api-4.md:134-135. Negate the index, without conjugating the values. */
int adf_ffun_reflect(adf_ffun_t y, const adf_ffun_t x)
{
    adf_ffun_t t;
    int st=ffun_shape(x->D,x->M); if (st==ADF_LIMIT) return st;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y);
    if (st!=ADF_OK) return st;
    ffun_allocate(t,x->D,x->M);
    ulong L=x->D*x->M;
    for (ulong k=0;k<L;k++) acb_set(t->f+k,x->f+(k ? L-k : 0));
    adf_ffun_swap(y,t); adf_ffun_clear(t); return ADF_OK;
}
/* docs/api-4.md:161-162: pointwise conjugation, an exact rectangle bijection. */
int adf_ffun_conj(adf_ffun_t y, const adf_ffun_t x)
{
    adf_ffun_t t;
    int st=ffun_shape(x->D,x->M); if (st==ADF_LIMIT) return st;
    ADF_INV_FFUN(x); ADF_INV_FFUN(y);
    if (st!=ADF_OK) return st;
    ffun_allocate(t,x->D,x->M);
    for (ulong k=0;k<x->D*x->M;k++) acb_conj(t->f+k,x->f+k);
    adf_ffun_swap(y,t); adf_ffun_clear(t); return ADF_OK;
}
