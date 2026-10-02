/* Local roots by exp(Log(unit)/n), with explicit torsion branches.
   docs/proofs/functions.md:410,428-438 (criterion and construction), :463-489 (exact image),
   :538 (rational roots). docs/api-1f5.md R1-R6 prove the finite tests and precision bookkeeping.
   FLINT APIs: refs/src/flint-3.0.1/fmpz.rst:983-988 (exact integer root), :923-930 (powm),
   :1154-1160 (invmod); nmod_poly.rst:2392-2398 (all distinct nonzero finite-field roots).
   No p-adic root iteration is used, including when p divides n. */
#include <stdlib.h>
#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include <flint/nmod_poly.h>
#include "invariants.h"

static void check_input(const adf_lball_t x, const char *fn)
{
#ifdef ADF_CHECK_INVARIANTS
    if (!adf_lball_is_canonical(x)) adf_inv_fail(fn, "x", "adf_lball");
#else
    (void)x; (void)fn;
#endif
}
static int exponent_ok(slong a)
{ return a>=-ADF_LBALL_EXP_MAX && a<=ADF_LBALL_EXP_MAX; }
static int power_ok(ulong p, slong k)
{ return k>=0 && k<=ADF_LBALL_BITS_MAX/(slong)FLINT_BIT_COUNT(p); }
static slong valuation(ulong n, ulong p)
{
    slong s=0;
    while (n%p==0) { n/=p; s++; }
    return s;
}
/* Word modular powering with no signed conversion or overflowing word product. */
static ulong power_mod(ulong a, ulong n, ulong p)
{
    fmpz_t A,P;
    ulong r;
    fmpz_init_set_ui(A,a); fmpz_init_set_ui(P,p);
    fmpz_powm_ui(A,A,n,P); r=fmpz_get_ui(A);
    fmpz_clear(A); fmpz_clear(P);
    return r;
}
static void unit_centre(adf_lball_t u, const adf_lball_t x)
{
    u->p=x->p; u->v=0; u->N=0; u->exact=1; fmpq_set(u->u,x->u);
}

/* R1. With the guard, valuation and torsion are constant. Log of the unit centre modulo
   p^(c+s) is zero iff the whole ball meets the logarithmic criterion (isometry, :336).
   Outside the guard return NOT_DETERMINED even if an earlier obstruction could decide no. */
static int criterion(ulong *count, ulong *index, slong *s, const adf_lball_t x, ulong n)
{
    adf_lball_t u,l;
    fmpz_t r;
    ulong d, abs_m;
    slong c=x->p==2 ? 2 : 1;
    int st;
    if (!exponent_ok(x->v) || !exponent_ok(x->N)) return ADF_LIMIT;
    if (n==0) return ADF_DOMAIN;
    if (n==1) { *count=1; *index=0; *s=0; return ADF_OK; }
    if (fmpq_is_zero(x->u))
    {
        if (!x->exact) return ADF_NOT_DETERMINED;
        *count=1; *index=0; *s=0; return ADF_OK;
    }
    *s=valuation(n,x->p);
    if (!x->exact && x->N-x->v<c+*s) return ADF_NOT_DETERMINED;
    abs_m=(ulong)(x->v<0 ? -x->v : x->v);
    if (abs_m%n) return ADF_DOMAIN;
    adf_lball_init(u); adf_lball_init(l); fmpz_init(r);
    unit_centre(u,x);
    st=adf_lball_unit_mod(r,u,c);
    if (st!=ADF_OK) goto done;
    *index=fmpz_fdiv_ui(r,x->p==2 ? 4 : x->p);
    d=n_gcd(n,x->p==2 ? 2 : x->p-1);
    if ((x->p==2 && d==2 && *index!=1) ||
        (x->p!=2 && power_mod(*index,(x->p-1)/d,x->p)!=1))
    { st=ADF_DOMAIN; goto done; }
    st=adf_lball_Log(l,u,c+*s);
    if (st==ADF_OK && !adf_lball_contains_zero(l)) st=ADF_DOMAIN;
    if (st==ADF_OK) *count=d;
done:
    adf_lball_clear(u); adf_lball_clear(l); fmpz_clear(r);
    return st;
}

/* Proposition 13, functions.md:410; R1. Locals keep scalar outputs transactional. */
int adf_lball_root_count(ulong *count, const adf_lball_t x, ulong n)
{
    ulong d,index;
    slong s;
    int st;
    check_input(x,__func__);
    st=criterion(&d,&index,&s,x,n);
    if (st==ADF_OK) *count=d;
    return st;
}

/* R3: a positive integer >1 with an n-th root >=2 has at least n+1 bits.
   This handles unsigned degrees beyond WORD_MAX before calling the signed FLINT API. */
static int integer_root(fmpz_t r, const fmpz_t a, ulong n)
{
    if (fmpz_is_one(a)) { fmpz_one(r); return 1; }
    if (n>=(ulong)fmpz_bits(a)) return 0;
    return fmpz_root(r,a,(slong)n);
}

/* Find the rational branch, if any, by numerator/denominator roots of the UNIT, never by
   constructing p^m. R3, functions.md:538; fmpz.rst:983-988. The rational roots are +/-q
   for even n and the signed q for odd n. Other torsion branches can still be irrational. */
static int rational_branch(adf_lball_t y, const adf_lball_t x, ulong n, ulong seed, slong j)
{
    fmpz_t a,r;
    int found=0, negative=fmpq_sgn(x->u)<0;
    if (negative && n%2==0) return 0;
    fmpz_init(a); fmpz_init(r);
    fmpz_abs(a,fmpq_numref(x->u));
    if (!integer_root(fmpq_numref(y->u),a,n) ||
        !integer_root(fmpq_denref(y->u),fmpq_denref(x->u),n)) goto done;
    y->p=x->p; y->v=0; y->N=0; y->exact=1;
    if (negative) fmpq_neg(y->u,y->u);
    for (int k=0;k<(n%2 ? 1 : 2);k++)
    {
        if (k) fmpq_neg(y->u,y->u);
        if (adf_lball_unit_mod(r,y,x->p==2 ? 2 : 1)!=ADF_OK) break;
        if (fmpz_get_ui(r)==seed) { y->v=j; found=1; break; }
    }
done:
    fmpz_clear(a); fmpz_clear(r);
    return found;
}

/* R2-R4. The checked branch is p^j omega(seed) exp(Log(unit)/n).
   Log precision L+s, then exact division by n, gives L digits; exp and torsion multiplication
   retain L. Scaling adds j to the exponent. E=j+(M-m)-s avoids (n-1)*j overflow. */
static int branch(adf_lball_t y, const adf_lball_t x, ulong n, ulong seed, slong s, slong N)
{
    adf_lball_t u,l,d,z,t,res;
    fmpz_t r;
    slong j, K, L;
    int st=ADF_OK;
    if (n==1 || fmpq_is_zero(x->u)) { adf_lball_set(y,x); return ADF_OK; }
    j=x->v==0 ? 0 : x->v/(slong)n; /* n<=|m|<=2^60 here when m!=0 */
    adf_lball_init(u); adf_lball_init(l); adf_lball_init(d);
    adf_lball_init(z); adf_lball_init(t); adf_lball_init(res); fmpz_init(r);
    if (x->exact && rational_branch(res,x,n,seed,j)) goto commit;
    K=x->exact ? N : j+(x->N-x->v)-s;
    if (!exponent_ok(K)) { st=ADF_LIMIT; goto done; }
    if (K<=j)
    {
        res->p=x->p; res->exact=0; res->N=K; res->v=0; fmpq_zero(res->u);
        goto commit;
    }
    L=K-j;
    if (L<(x->p==2 ? 2 : 1)) L=x->p==2 ? 2 : 1;
    if (!power_ok(x->p,L+s)) { st=ADF_LIMIT; goto done; }
    unit_centre(u,x);
    st=adf_lball_Log(l,u,L+s);
    if (st!=ADF_OK) goto done;
    d->p=x->p; d->v=s; d->N=0; d->exact=1;
    /* n/p^s is a unit, obtained by word division without forming p^s. */
    {
        ulong nu=n;
        for (slong k=0;k<s;k++) nu/=x->p;
        fmpq_set_ui(d->u,nu,1);
    }
    st=adf_lball_div(l,l,d);
    if (st!=ADF_OK) goto done;
    st=adf_lball_exp(z,l,L);
    if (st!=ADF_OK) goto done;
    if (x->p==2)
    {
        t->p=2; fmpq_set_si(t->u,seed==1 ? 1 : -1,1);
    }
    else st=adf_lball_teichmuller(t,adf_lball_place(x),seed,L);
    if (st!=ADF_OK) goto done;
    st=adf_lball_mul(z,z,t);
    if (st!=ADF_OK) goto done;
    st=adf_lball_unit_mod(r,z,K-j);
    if (st!=ADF_OK) goto done;
    res->p=x->p; res->v=j; res->N=K; res->exact=0;
    fmpq_set_fmpz(res->u,r);
commit:
    adf_lball_set(y,res);
done:
    adf_lball_clear(u); adf_lball_clear(l); adf_lball_clear(d);
    adf_lball_clear(z); adf_lball_clear(t); adf_lball_clear(res); fmpz_clear(r);
    return st;
}

/* R1, R4; functions.md:410,463. Invalid selectors are constructor-like DOMAIN errors. */
int adf_lball_root_seed(adf_lball_t y, const adf_lball_t x, ulong n, ulong seed, slong N)
{
    ulong d,index;
    slong s;
    int st;
    check_input(x,__func__);
    st=criterion(&d,&index,&s,x,n);
    if (st!=ADF_OK) return st;
    if (n==1) return branch(y,x,n,seed,s,N);
    if (fmpq_is_zero(x->u))
    {
        if (seed!=0) return ADF_DOMAIN;
    }
    else if (x->p==2)
    {
        if ((seed!=1 && seed!=3) || (n%2 ? seed : 1)!=index) return ADF_DOMAIN;
    }
    else if (seed==0 || seed>=x->p || power_mod(seed,n,x->p)!=index) return ADF_DOMAIN;
    return branch(y,x,n,seed,s,N);
}

/* Proposition 13 square specialisation, functions.md:435-442. */
int adf_lball_sqrt_seed(adf_lball_t y, const adf_lball_t x, ulong seed, slong N)
{ check_input(x,__func__); return adf_lball_root_seed(y,x,2,seed,N); }

static int compare_words(const void *a, const void *b)
{
    ulong x=*(const ulong *)a, y=*(const ulong *)b;
    return (x>y)-(x<y);
}

/* R5: d=gcd(n,p-1), h=(p-1)/d, e=(n/d)^(-1) mod h. The torsion test proves index^h=1.
   Hence X^d-index^e has precisely the required d nonzero simple roots, by Lemma 3:55.
   Factor a degree d polynomial, not degree n and not a search through all p residues. */
static int identifiers(ulong *ids, ulong p, ulong n, ulong d, ulong index)
{
    fmpz_t a,h;
    nmod_poly_t f;
    ulong A, e;
    int ok;
    if (index==0) { ids[0]=0; return ADF_OK; }
    if (p==2)
    {
        ids[0]=n%2 ? index : 1;
        if (d==2) ids[1]=3;
        return ADF_OK;
    }
    fmpz_init_set_ui(a,n/d); fmpz_init_set_ui(h,(p-1)/d);
    if (fmpz_is_one(h)) e=0;
    else { fmpz_invmod(a,a,h); e=fmpz_get_ui(a); }
    A=power_mod(index,e,p);
    nmod_poly_init(f,p);
    nmod_poly_set_coeff_ui(f,0,p-A); nmod_poly_set_coeff_ui(f,(slong)d,1);
    ok=nmod_poly_find_distinct_nonzero_roots(ids,f);
    nmod_poly_clear(f); fmpz_clear(a); fmpz_clear(h);
    /* A failed certificate must not publish a partial list or claim nonexistence. */
    if (!ok) return ADF_NOT_DETERMINED;
    qsort(ids,(size_t)d,sizeof(ulong),compare_words);
    return ADF_OK;
}

/* R5-R6; functions.md:410,463. Evaluate in separate storage so x may be any output slot. */
int adf_lball_roots(adf_lball_ptr y, ulong *ids, slong *len, slong capacity,
                   const adf_lball_t x, ulong n, slong N)
{
    ulong d,index,*tmpids;
    slong s;
    adf_lball_ptr tmp;
    int st;
    check_input(x,__func__);
    st=criterion(&d,&index,&s,x,n);
    if (st!=ADF_OK) return st;
    if (capacity<0) return ADF_DOMAIN;
    if (d>(ulong)capacity || d>(ulong)ADF_LROOT_BRANCH_MAX) return ADF_LIMIT;
    tmpids=flint_malloc((size_t)d*sizeof(ulong));
    st=identifiers(tmpids,x->p,n,d,index);
    if (st!=ADF_OK) { flint_free(tmpids); return st; }
    tmp=flint_malloc((size_t)d*sizeof(adf_lball_struct));
    for (ulong i=0;i<d;i++) adf_lball_init(tmp+i);
    for (ulong i=0;i<d && st==ADF_OK;i++) st=branch(tmp+i,x,n,tmpids[i],s,N);
    if (st==ADF_OK)
    {
        for (ulong i=0;i<d;i++) { adf_lball_set(y+i,tmp+i); ids[i]=tmpids[i]; }
        *len=(slong)d;
    }
    for (ulong i=0;i<d;i++) adf_lball_clear(tmp+i);
    flint_free(tmp); flint_free(tmpids);
    return st;
}
