/* Local roots by exp(Log(unit)/n), with explicit torsion branches.
   docs/proofs/functions.md:410,428-438 (criterion and construction), :463-489 (exact image),
   :538 (rational roots). docs/api-1f5.md R1-R9 prove the finite tests and precision bookkeeping.
   The exponent of a ball result is K=min(N,E) (decision N-D14, docs/SPEC.md 15.4; R2, R4).
   FLINT APIs: refs/src/flint-3.0.1/fmpz.rst:983-988 (exact integer root), :923-930 (powm),
   :1154-1160 (invmod); ulong_extras.rst:1410 (n_primitive_root_prime), :1203 (n_factor),
   :1078 (n_remove), :140 (n_pow), :216 (n_preinvert_limb), :368 (n_mulmod2_preinv),
   :537 (n_powmod2_ui_preinv), :477 (n_invmod); fmpq.rst:140 (fmpq_is_pm1); fmpz_mod.rst:20
   (fmpz_mod_ctx_init), :85 (fmpz_mod_mul).
   No p-adic root iteration is used, including when p divides n. */
#include <stdlib.h>
#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include <flint/fmpz_mod.h>
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

/* What every branch of one input shares (R2 step 1, R4; F2 of review f-review6): j=m/n; the
   exponent K (N for an exact input, min(N,E) for a ball, N-D14); the working precision L; the
   principal-unit root z0=exp(Log(a/p^m)/n) at L, computed at most once (st0=-1 before); and
   the rational roots of an exact input with their identifiers (R3), at most two. */
typedef struct
{
    slong j, K, L, s;
    int st0, nrat;
    adf_lball_t z0;
    ulong rid[2];
    adf_lball_t rat[2];
} shared_t;

/* Find the rational branches, if any, by numerator/denominator roots of the UNIT, never by
   constructing p^m. R3, functions.md:538; fmpz.rst:983-988. The rational roots are +/-q
   for even n and the signed q for odd n. Other torsion branches can still be irrational. */
static void rational_branches(shared_t *sh, const adf_lball_t x, ulong n)
{
    adf_lball_t y;
    fmpz_t a,r;
    int negative=fmpq_sgn(x->u)<0;
    if (negative && n%2==0) return;
    adf_lball_init(y); fmpz_init(a); fmpz_init(r);
    fmpz_abs(a,fmpq_numref(x->u));
    if (!integer_root(fmpq_numref(y->u),a,n) ||
        !integer_root(fmpq_denref(y->u),fmpq_denref(x->u),n)) goto done;
    y->p=x->p; y->v=0; y->N=0; y->exact=1;
    if (negative) fmpq_neg(y->u,y->u);
    for (int k=0;k<(n%2 ? 1 : 2);k++)
    {
        if (k) fmpq_neg(y->u,y->u);
        if (adf_lball_unit_mod(r,y,x->p==2 ? 2 : 1)!=ADF_OK) break;
        sh->rid[sh->nrat]=fmpz_get_ui(r);
        adf_lball_set(sh->rat[sh->nrat],y);
        sh->rat[sh->nrat]->v=sh->j;
        sh->nrat++;
    }
done:
    adf_lball_clear(y); fmpz_clear(a); fmpz_clear(r);
}

/* After a successful criterion. E=j+(M-m)-s avoids (n-1)*j overflow (R6 step 1). */
static void shared_init(shared_t *sh, const adf_lball_t x, ulong n, slong s, slong N)
{
    slong E;
    sh->s=s; sh->st0=-1; sh->nrat=0; sh->L=0;
    adf_lball_init(sh->z0); adf_lball_init(sh->rat[0]); adf_lball_init(sh->rat[1]);
    /* n<=|m|<=2^60 here when m!=0 (R6 step 1); zero and degree 1 do not use j */
    sh->j=n<2 || fmpq_is_zero(x->u) || x->v==0 ? 0 : x->v/(slong)n;
    if (x->exact) sh->K=N;
    else { E=sh->j+(x->N-x->v)-s; sh->K=N<E ? N : E; }
    if (x->exact && n>=2 && !fmpq_is_zero(x->u)) rational_branches(sh,x,n);
}

static void shared_clear(shared_t *sh)
{ adf_lball_clear(sh->z0); adf_lball_clear(sh->rat[0]); adf_lball_clear(sh->rat[1]); }

/* R4 steps 1-3, for K>j: L=max(K-j,c); Log at L+s, exact division by n, exp at L. No power is
   tested here: Log, div and exp make their own checks (lfunc.h, lball.h), and the exact unit
   +-1 needs none (Log=0 exactly, F2 of api-1f4.md; exp(0)=1). F1 of review f-review6. */
static int principal(shared_t *sh, const adf_lball_t x, ulong n)
{
    adf_lball_t u,l,d;
    slong c=x->p==2 ? 2 : 1;
    if (sh->st0>=0) return sh->st0;
    sh->L=sh->K-sh->j < c ? c : sh->K-sh->j;
    adf_lball_init(u); adf_lball_init(l); adf_lball_init(d);
    unit_centre(u,x);
    sh->st0=adf_lball_Log(l,u,sh->L+sh->s);
    if (sh->st0==ADF_OK)
    {
        /* n/p^s is a unit, obtained by word division without forming p^s. */
        ulong nu=n;
        for (slong k=0;k<sh->s;k++) nu/=x->p;
        d->p=x->p; d->v=sh->s; d->N=0; d->exact=1;
        fmpq_set_ui(d->u,nu,1);
        sh->st0=adf_lball_div(l,l,d);
    }
    if (sh->st0==ADF_OK) sh->st0=adf_lball_exp(sh->z0,l,sh->L);
    adf_lball_clear(u); adf_lball_clear(l); adf_lball_clear(d);
    return sh->st0;
}

/* R2-R4. The checked branch is p^j omega(seed) exp(Log(unit)/n), exponent K.
   Exact division by n after Log at L+s gives L digits; exp and torsion multiplication retain L.
   The product with each torsion factor is R2 step 1; z0 is shared by all branches. */
static int branch(adf_lball_t y, shared_t *sh, const adf_lball_t x, ulong n, ulong seed)
{
    adf_lball_t z,t,res;
    fmpz_t r;
    int st=ADF_OK;
    if (n==1 || fmpq_is_zero(x->u)) { adf_lball_set(y,x); return ADF_OK; }
    for (int i=0;i<sh->nrat;i++)
        if (sh->rid[i]==seed) { adf_lball_set(y,sh->rat[i]); return ADF_OK; }
    if (!exponent_ok(sh->K)) return ADF_LIMIT;
    adf_lball_init(z); adf_lball_init(t); adf_lball_init(res); fmpz_init(r);
    if (sh->K<=sh->j)
    {
        /* R4 step 5: the root is in p^K Z_p; also a ball input with N<=j (N-D14). */
        res->p=x->p; res->exact=0; res->N=sh->K; res->v=0; fmpq_zero(res->u);
        goto commit;
    }
    st=principal(sh,x,n);
    if (st!=ADF_OK) goto done;
    if (x->p==2)
    {
        t->p=2; fmpq_set_si(t->u,seed==1 ? 1 : -1,1);
    }
    else st=adf_lball_teichmuller(t,adf_lball_place(x),seed,sh->L);
    if (st!=ADF_OK) goto done;
    st=adf_lball_mul(z,sh->z0,t);
    if (st!=ADF_OK) goto done;
    /* The exact 1 has the unit 1 modulo every p^(K-j): no power is formed (F1). */
    if (z->exact && fmpq_is_one(z->u)) fmpz_one(r);
    else st=adf_lball_unit_mod(r,z,sh->K-sh->j);
    if (st!=ADF_OK) goto done;
    res->p=x->p; res->v=sh->j; res->N=sh->K; res->exact=0;
    fmpq_set_fmpz(res->u,r);
commit:
    adf_lball_set(y,res);
done:
    adf_lball_clear(z); adf_lball_clear(t); adf_lball_clear(res); fmpz_clear(r);
    return st;
}

/* R1, R4; functions.md:410,463. Invalid selectors are constructor-like DOMAIN errors. */
int adf_lball_root_seed(adf_lball_t y, const adf_lball_t x, ulong n, ulong seed, slong N)
{
    ulong d,index;
    slong s;
    shared_t sh;
    int st;
    check_input(x,__func__);
    st=criterion(&d,&index,&s,x,n);
    if (st!=ADF_OK) return st;
    if (n==1) { adf_lball_set(y,x); return ADF_OK; }
    if (fmpq_is_zero(x->u))
    {
        if (seed!=0) return ADF_DOMAIN;
    }
    else if (x->p==2)
    {
        if ((seed!=1 && seed!=3) || (n%2 ? seed : 1)!=index) return ADF_DOMAIN;
    }
    else if (seed==0 || seed>=x->p || power_mod(seed,n,x->p)!=index) return ADF_DOMAIN;
    shared_init(&sh,x,n,s,N);
    st=branch(y,&sh,x,n,seed);
    shared_clear(&sh);
    return st;
}

/* Proposition 13 square specialisation, functions.md:435-442. */
int adf_lball_sqrt_seed(adf_lball_t y, const adf_lball_t x, ulong seed, slong N)
{ check_input(x,__func__); return adf_lball_root_seed(y,x,2,seed,N); }

static int compare_words(const void *a, const void *b)
{
    ulong x=*(const ulong *)a, y=*(const ulong *)b;
    return (x>y)-(x<y);
}

/* R8: one root of T^d=A in F_p, given A^((p-1)/d)=1, d>=2 dividing p-1 and a primitive root g.
   P=p-1=D*R, D the part of P made of the primes of d, gcd(D,R)=1. A=A_R*prod_q A_q, the
   components of the CRT idempotents of P. A_R: d is invertible modulo R. A_q, in the cyclic
   group of order Q=q^a generated by g^(P/Q): its logarithm k is found digit by digit
   (Pohlig-Hellman), the digits below q^b, b=v_q(d), being 0; then (k/q^b)(d/q^b)^(-1) mod Q.
   Returns 0 if the product is not a root (it always is, by R8; the test is a certificate). */
static ulong dth_root(ulong A, ulong d, ulong p, ulong g)
{
    const ulong P=p-1, pinv=n_preinvert_limb(p), Pinv=n_preinvert_limb(P);
    n_factor_t fd;
    ulong D=1, R, t;
    n_factor_init(&fd);
    n_factor(&fd,d,1);
    for (int i=0;i<fd.num;i++)
    {
        ulong rest=P;
        D*=n_pow(fd.p[i],(ulong)n_remove(&rest,fd.p[i]));
    }
    R=P/D;
    t=1;
    if (R>1)
    {
        ulong eR=n_mulmod2_preinv(D,n_invmod(D%R,R),P,Pinv);
        t=n_powmod2_ui_preinv(A,n_mulmod2_preinv(eR,n_invmod(d%R,R),P,Pinv),p,pinv);
    }
    for (int i=0;i<fd.num;i++)
    {
        ulong q=fd.p[i], rest=P, a, b=(ulong)fd.exp[i], Q, Aq, gq, zeta, k=0, qi, dq;
        a=(ulong)n_remove(&rest,q);
        Q=n_pow(q,a);
        /* Aq = A^e, e = (P/Q)((P/Q)^(-1) mod Q) mod P; gq = g^(P/Q) has order Q */
        Aq=n_powmod2_ui_preinv(A,n_mulmod2_preinv(P/Q,n_invmod((P/Q)%Q,Q),P,Pinv),p,pinv);
        gq=n_powmod2_ui_preinv(g,P/Q,p,pinv);
        zeta=n_powmod2_ui_preinv(gq,Q/q,p,pinv);
        qi=n_pow(q,b);
        for (ulong l=b;l<a;l++,qi*=q)
        {
            /* (Aq gq^(-k))^(q^(a-1-l)) = zeta^(digit l) */
            ulong z=n_mulmod2_preinv(Aq,n_powmod2_ui_preinv(gq,(Q-k)%Q,p,pinv),p,pinv), cur=1, c=0;
            z=n_powmod2_ui_preinv(z,n_pow(q,a-1-l),p,pinv);
            while (c<q && cur!=z) { cur=n_mulmod2_preinv(cur,zeta,p,pinv); c++; }
            if (c==q) return 0;
            k+=c*qi;
        }
        dq=d/n_pow(q,b);
        t=n_mulmod2_preinv(t,n_powmod2_ui_preinv(gq,
              n_mulmod2_preinv(k/n_pow(q,b),n_invmod(dq%Q,Q),Q,n_preinvert_limb(Q)),p,pinv),p,pinv);
    }
    return n_powmod2_ui_preinv(t,d,p,pinv)==A ? t : 0;
}

/* R5, R8: d=gcd(n,p-1), h=(p-1)/d, e=(n/d)^(-1) mod h. The torsion test proves index^h=1.
   The roots of T^d-index^e are the n-th roots of index (R5); they are t0 zeta^i, i<d,
   zeta=g^h for a primitive root g (R8), sorted. *t0_out=t0, *zeta_out=zeta for the order of
   evaluation (R9); *zeta_out=0 where there is no such chain (zero, p=2, d=1). No polynomial is
   formed. */
static int identifiers(ulong *ids, ulong *t0_out, ulong *zeta_out, ulong p, ulong n, ulong d,
                       ulong index)
{
    fmpz_t a,h;
    ulong A, e, g, zeta, t0, pinv;
    *t0_out=0; *zeta_out=0;
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
    fmpz_clear(a); fmpz_clear(h);
    A=power_mod(index,e,p);
    if (d==1) { ids[0]=A; return ADF_OK; }
    g=n_primitive_root_prime(p);
    t0=dth_root(A,d,p,g);
    /* A failed certificate must not publish a partial list or claim nonexistence. */
    if (t0==0) return ADF_NOT_DETERMINED;
    pinv=n_preinvert_limb(p);
    zeta=n_powmod2_ui_preinv(g,(p-1)/d,p,pinv);
    ids[0]=t0;
    for (ulong i=1;i<d;i++) ids[i]=n_mulmod2_preinv(ids[i-1],zeta,p,pinv);
    qsort(ids,(size_t)d,sizeof(ulong),compare_words);
    *t0_out=t0; *zeta_out=zeta;
    return ADF_OK;
}

/* The position of seed in the increasing list ids[0..d) (it is there, R8 (b)). */
static ulong position(const ulong *ids, ulong d, ulong seed)
{
    ulong lo=0, hi=d;
    while (hi-lo>1)
    {
        ulong mid=lo+(hi-lo)/2;
        if (ids[mid]<=seed) lo=mid; else hi=mid;
    }
    return lo;
}

static int rational_seed(const shared_t *sh, ulong seed)
{
    for (int i=0;i<sh->nrat;i++) if (sh->rid[i]==seed) return 1;
    return 0;
}

/* R9 (lane f-repair5): every branch of the sorted list ids, evaluated in the order t0 zeta^i of
   R8 (each into its own slot of tmp). At odd p the first general branch whose seed is not 1 or
   p-1 (the two seeds with a rational Teichmueller representative, lball.h) lifts omega(seed) and
   omega(zeta) at L: two lifts for the list, when z0 is exact or has relative precision >= K-j
   (always, by R4; otherwise every branch lifts its own, as root_seed does). With kq=K-j and c0
   the unit of z0 modulo p^kq, y = c0 omega(seed) modulo p^kq, multiplied by omega(zeta) modulo
   p^kq at each step, is the centre that branch() computes for the seed of that step (z=z0*omega,
   then unit_mod at kq), and the branch is p^j y + p^K Z_p (R9). Without a chain (zeta=0: p=2,
   d=1, zero) the branches are taken in the order of ids. */
static int all_branches(adf_lball_ptr tmp, const ulong *ids, ulong t0, ulong zeta, shared_t *sh,
                        const adf_lball_t x, ulong n, ulong d)
{
    adf_lball_t om, w;
    fmpz_t M, y, cw, c0;
    fmpz_mod_ctx_t cM;
    ulong p=x->p, pinv=zeta ? n_preinvert_limb(p) : 0, seed=t0;
    int st=ADF_OK, chain=0;
    adf_lball_init(om); adf_lball_init(w); fmpz_init(M); fmpz_init(y); fmpz_init(cw); fmpz_init(c0);
    for (ulong i=0;i<d && st==ADF_OK;i++)
    {
        ulong k;
        int plain;
        if (zeta==0) { k=i; seed=ids[i]; }
        else
        {
            if (i) seed=n_mulmod2_preinv(seed,zeta,p,pinv);    /* ulong_extras.rst:368 */
            k=position(ids,d,seed);
        }
        plain=seed==1 || seed==p-1;
        if (chain) fmpz_mod_mul(y,y,cw,cM);                    /* fmpz_mod.rst:85 */
        else if (zeta!=0 && !plain && exponent_ok(sh->K) && sh->K>sh->j && !rational_seed(sh,seed) &&
                 (sh->z0->exact || sh->z0->N-sh->z0->v>=sh->K-sh->j))
        {
            /* sh->L is set: early_status ran principal(), as this branch is general and K>j;
               K-j <= L (R4). The lifts need p^L, unit_mod p^(K-j) (lball.h). */
            slong kq=sh->K-sh->j;
            st=adf_lball_teichmuller(om,adf_lball_place(x),seed,sh->L);
            if (st==ADF_OK) st=adf_lball_teichmuller(w,adf_lball_place(x),zeta,sh->L);
            if (st==ADF_OK) st=adf_lball_unit_mod(c0,sh->z0,kq);
            if (st!=ADF_OK) break;
            chain=1;
            fmpz_set_ui(M,p); fmpz_pow_ui(M,M,(ulong)kq);
            fmpz_mod_ctx_init(cM,M);                           /* fmpz_mod.rst:20 */
            if (w->exact) fmpz_sub_ui(cw,M,1);                 /* omega(p-1)=-1 (zeta=p-1, d=2) */
            else fmpz_mod(cw,fmpq_numref(w->u),M);
            fmpz_mod(y,fmpq_numref(om->u),M);                  /* omega(seed) is a ball: seed != +-1 */
            fmpz_mod_mul(y,y,c0,cM);
        }
        if (chain && !plain && !rational_seed(sh,seed))
        {
            tmp[k].p=p; tmp[k].v=sh->j; tmp[k].N=sh->K; tmp[k].exact=0;
            fmpq_set_fmpz(tmp[k].u,y);
        }
        else st=branch(tmp+k,sh,x,n,seed);
    }
    if (chain) fmpz_mod_ctx_clear(cM);
    adf_lball_clear(om); adf_lball_clear(w); fmpz_clear(M); fmpz_clear(y); fmpz_clear(cw); fmpz_clear(c0);
    return st;
}

/* 1 if seed is a branch (R5: seed^n=index at odd p; at 2 the identifiers of identifiers()) that
   is not rational (R3). */
static int general_seed(const shared_t *sh, ulong p, ulong n, ulong index, ulong seed)
{
    if (rational_seed(sh,seed)) return 0;
    if (p==2) return n%2 ? seed==index : (seed==1 || seed==3);
    return power_mod(seed,n,p)==index;
}

/* R6 step 6 (F2(b) of review f-review6; finding 3 of review f-review7, lane f-repair5): the
   status of the whole evaluation, decided before the enumeration. general = branches that are
   not rational (R3). If general>0: the exponent K, then z0 (R4). After that a general branch
   with torsion factor t forms z=z0*t and, unless z is the exact 1, the centre modulo p^(K-j);
   every power it forms is at most p^L, and a branch with z not the exact 1 returns LIMIT exactly
   when p^L is beyond the bound. z is the exact 1 for at most one seed: z0 exactly +-1 and t=z0
   (t is exact only for the seeds 1 and p-1, or 3 at 2: lball.h teichmuller). */
static int early_status(shared_t *sh, const adf_lball_t x, ulong n, ulong d, ulong index)
{
    ulong general=d-(ulong)sh->nrat, one=0, p=x->p;
    int st;
    if (n==1 || fmpq_is_zero(x->u) || general==0) return ADF_OK;
    if (!exponent_ok(sh->K)) return ADF_LIMIT;
    if (sh->K<=sh->j) return ADF_OK;
    st=principal(sh,x,n);
    if (st!=ADF_OK) return st;
    if (sh->z0->exact && fmpq_is_pm1(sh->z0->u))   /* refs/src/flint-3.0.1/fmpq.rst:140 */
    {
        ulong seed=fmpq_is_one(sh->z0->u) ? 1 : (p==2 ? 3 : p-1);
        one=(ulong)general_seed(sh,p,n,index,seed);
    }
    if (general>one && !power_ok(p,sh->L)) return ADF_LIMIT;
    return ADF_OK;
}

/* R5-R6, R8; functions.md:410,463. Evaluate in separate storage so x may be any output slot. */
int adf_lball_roots(adf_lball_ptr y, ulong *ids, slong *len, slong capacity,
                   const adf_lball_t x, ulong n, slong N)
{
    ulong d,index,t0,zeta,*tmpids;
    slong s;
    adf_lball_ptr tmp;
    shared_t sh;
    int st;
    check_input(x,__func__);
    st=criterion(&d,&index,&s,x,n);
    if (st!=ADF_OK) return st;
    if (capacity<0) return ADF_DOMAIN;
    if (d>(ulong)capacity || d>(ulong)ADF_LROOT_BRANCH_MAX) return ADF_LIMIT;
    shared_init(&sh,x,n,s,N);
    st=early_status(&sh,x,n,d,index);
    if (st!=ADF_OK) { shared_clear(&sh); return st; }
    tmpids=flint_malloc((size_t)d*sizeof(ulong));
    st=identifiers(tmpids,&t0,&zeta,x->p,n,d,index);
    if (st!=ADF_OK) { flint_free(tmpids); shared_clear(&sh); return st; }
    tmp=flint_malloc((size_t)d*sizeof(adf_lball_struct));
    for (ulong i=0;i<d;i++) adf_lball_init(tmp+i);
    st=all_branches(tmp,tmpids,t0,zeta,&sh,x,n,d);
    if (st==ADF_OK)
    {
        for (ulong i=0;i<d;i++) { adf_lball_set(y+i,tmp+i); ids[i]=tmpids[i]; }
        *len=(slong)d;
    }
    for (ulong i=0;i<d;i++) adf_lball_clear(tmp+i);
    flint_free(tmp); flint_free(tmpids); shared_clear(&sh);
    return st;
}
