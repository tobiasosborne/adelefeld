/* Arithmetic catalogue. The formulas are proved in docs/proofs/catalogue.md:291-312.
   FLINT domains: refs/src/flint-3.0.1/fmpz.rst:997-1008, 1040-1048. */
#include <adelefeld.h>
#include "invariants.h"
#ifdef ADF_CHECK_INVARIANTS
#define CAT_INV_UC(x) do { if (!adf_ucoset_is_canonical(x)) \
    adf_inv_fail(__func__, #x, "adf_ucoset"); } while (0)
#define CAT_INV_CLASS(x) do { if (!adf_idclass_is_canonical(x)) \
    adf_inv_fail(__func__, #x, "adf_idclass"); } while (0)
#else
#define CAT_INV_UC(x) ((void)0)
#define CAT_INV_CLASS(x) ((void)0)
#endif

/* Y10, also the domain argument of docs/api-1f9.md Y3. */
static int integral_status(const fmpz_t A, const fmpz_t H, const fmpz_t d)
{
    fmpz_t g;
    int st;
    if (fmpz_is_one(d)) return ADF_OK;
    fmpz_init(g);
    fmpz_gcd(g,H,d);
    st=fmpz_divisible(A,g) ? ADF_NOT_DETERMINED : ADF_DOMAIN;
    fmpz_clear(g);
    return st;
}

/* Falling polynomial, valid for arbitrary signed a. The quotient at each step is integral:
   B_i=B_(i-1)*(a-i+1)/i. fmpz_bin_uiui only accepts word-sized nonnegative a. */
static void binomial(fmpz_t z, const fmpz_t a, ulong k)
{
    fmpz_t t;
    fmpz_init(t);
    fmpz_one(z);
    for (ulong i=1;i<=k;i++)
    {
        fmpz_sub_ui(t,a,i-1);
        fmpz_mul(z,z,t);
        fmpz_divexact_ui(z,z,i);
    }
    fmpz_clear(t);
}

/* Proposition 14:291-312. Compute entirely before touching the output, including aliases. */
static int ball_binomial(adf_fball_t y, const adf_fball_t x, ulong k, int tight)
{
    fmpz_t A,H,d,C,R,t,b;
    int st;
    if (k>(tight ? ADF_BINOM_TIGHT_K_MAX : ADF_BINOM_K_MAX)) return ADF_LIMIT;
    fmpz_init(A); fmpz_init(H); fmpz_init(d); fmpz_init(C);
    fmpz_init(R); fmpz_init(t); fmpz_init(b);
    adf_fball_get_fmpz3(A,H,d,x);
    st=integral_status(A,H,d);
    if (st==ADF_OK)
    {
        binomial(C,A,k);
        if (k==0 || fmpz_is_zero(H)) fmpz_zero(R);
        else if (tight)
        {
            fmpz_zero(R);
            for (ulong j=1;j<=k;j++)
            {
                fmpz_mul_ui(t,H,j); fmpz_add(t,t,A);
                binomial(b,t,k); fmpz_sub(b,b,C);
                fmpz_gcd(R,R,b);
            }
        }
        else
        {
            fmpz_fac_ui(t,k); fmpz_gcd(R,H,t); fmpz_divexact(R,H,R);
        }
        st=adf_fball_set_fmpz3(y,C,R,d);
    }
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); fmpz_clear(C);
    fmpz_clear(R); fmpz_clear(t); fmpz_clear(b);
    return st;
}
int adf_fball_binom(adf_fball_t y, adf_place_t *where, const adf_fball_t x, ulong k)
{
    ADF_INV_FBALL(x);
    (void)where;
    return ball_binomial(y,x,k,0);
}
int adf_fball_binom_tight(adf_fball_t y, adf_place_t *where, const adf_fball_t x, ulong k)
{
    ADF_INV_FBALL(x);
    (void)where;
    return ball_binomial(y,x,k,1);
}

/* Definition 11:218-224; fmpz_powm domain refs/src/flint-3.0.1/fmpz.rst:923-929.
   Explicit inverse for signed e: invmod domain :1154-1160. m>=1, gcd(c,m)=1. */
static void signed_powm(fmpz_t r, const fmpz_t c, const fmpz_t e, const fmpz_t m)
{
    fmpz_t b,k;
    if (fmpz_is_one(m)) { fmpz_zero(r); return; }
    fmpz_init(b); fmpz_init(k);
    fmpz_abs(k,e);
    if (fmpz_sgn(e)<0)
    {
        if (!fmpz_invmod(b,c,m)) flint_abort(); /* A unit at this modulus, by the caller's proof. */
    }
    else fmpz_mod(b,c,m);
    fmpz_powm(r,b,k,m);
    fmpz_mod(r,r,m); /* The documented e=0 result is 1, including modulus 1 handled above. */
    fmpz_clear(b); fmpz_clear(k);
}

/* Y12: A=gcd(N*g_N,c^M-1) and B the maximal outside block. No N factorisation.
   Proposition 13:245-287. tight(U(1),g) reuses ideles.md:267-289.
   CRT domains refs/src/flint-3.0.1/fmpz.rst:1292-1303; branches exclude modulus 1. */
static void finest(adf_ucoset_t y, const adf_ucoset_t a, const fmpz_t e,
                   const fmpz_t M, const fmpz_t g)
{
    adf_ucoset_t all,block;
    fmpz_t A,B,rest,h,t,r,one;
    adf_ucoset_init(all); adf_ucoset_init(block);
    fmpz_init(A); fmpz_init(B); fmpz_init(rest); fmpz_init(h);
    fmpz_init(t); fmpz_init(r); fmpz_init(one); fmpz_one(one);
    fmpz_set(A,a->N); fmpz_set(rest,g);
    for (;;)
    {
        fmpz_gcd(h,rest,a->N);
        if (fmpz_is_one(h)) break;
        fmpz_mul(A,A,h); fmpz_divexact(rest,rest,h);
    }
    if (!fmpz_is_zero(M))
    {
        signed_powm(t,a->c,M,A); fmpz_sub_ui(t,t,1); fmpz_gcd(A,A,t);
    }
    /* U(1)^g supplies all outside primes, with canon already applied to a lone factor 2. */
    adf_ucoset_set_fmpz2(all,one,one);
    adf_ucoset_pow_tight(block,all,(slong)fmpz_get_ui(g));
    fmpz_set(B,block->N);
    for (;;)
    {
        fmpz_gcd(h,B,a->N);
        if (fmpz_is_one(h)) break;
        fmpz_divexact(B,B,h);
    }
    signed_powm(r,a->c,e,A);
    if (fmpz_is_one(A)) fmpz_one(r);
    else if (!fmpz_is_one(B)) fmpz_CRT(r,r,A,one,B,0);
    fmpz_mul(t,A,B);
    adf_ucoset_set_fmpz2(y,r,t);
    adf_ucoset_normalise(y,y);
    fmpz_clear(A); fmpz_clear(B); fmpz_clear(rest); fmpz_clear(h);
    fmpz_clear(t); fmpz_clear(r); fmpz_clear(one);
    adf_ucoset_clear(all); adf_ucoset_clear(block);
}

/* Proposition 12:228-241 and Proposition 13:245-287. Domain first, then exact cases,
   then the finest g limit. Input and output may coincide: result is held in u until success. */
static int profpow(adf_ucoset_t y, const adf_ucoset_t a, const adf_fball_t x, int policy)
{
    fmpz_t e,M,d,D,r,g;
    adf_ucoset_t u;
    int st;
    fmpz_init(e); fmpz_init(M); fmpz_init(d); fmpz_init(D); fmpz_init(r); fmpz_init(g);
    adf_ucoset_init(u);
    adf_fball_get_fmpz3(e,M,d,x);
    st=integral_status(e,M,d);
    if (st!=ADF_OK) goto done;
    adf_ucoset_normalise(u,a);
    if (fmpz_is_zero(M) && fmpz_is_zero(e)) adf_ucoset_one(u);
    else if (fmpz_is_zero(u->N))
    {
        if (fmpz_is_one(u->c) || fmpz_is_even(M))
        {
            if (fmpz_is_one(u->c) || fmpz_is_even(e)) adf_ucoset_one(u);
            else adf_ucoset_minus_one(u);
        }
        else if (policy==0) st=ADF_NOT_DETERMINED;
        else { fmpz_one(r); adf_ucoset_set_fmpz2(u,r,r); }
    }
    else if (policy==2)
    {
        fmpz_gcd(g,e,M);
        if (fmpz_cmp_ui(g,ADF_PROFPOW_FINE_G_MAX)>0) st=ADF_LIMIT;
        else if (fmpz_is_zero(M)) adf_ucoset_pow_tight(u,u,fmpz_get_si(e));
        else finest(u,u,e,M,g);
    }
    else if (fmpz_is_zero(M) && fmpz_fits_si(e)) adf_ucoset_pow(u,u,fmpz_get_si(e));
    else
    {
        fmpz_set(D,u->N);
        if (!fmpz_is_zero(M))
        {
            signed_powm(r,u->c,M,u->N); fmpz_sub_ui(r,r,1); fmpz_gcd(D,D,r);
        }
        if (policy==0 && !fmpz_equal(D,u->N)) st=ADF_NOT_DETERMINED;
        else
        {
            signed_powm(r,u->c,e,D);
            adf_ucoset_set_fmpz2(u,r,D); adf_ucoset_normalise(u,u);
        }
    }
    if (st==ADF_OK) adf_ucoset_swap(y,u);
done:
    fmpz_clear(e); fmpz_clear(M); fmpz_clear(d); fmpz_clear(D); fmpz_clear(r); fmpz_clear(g);
    adf_ucoset_clear(u);
    return st;
}
int adf_ucoset_profpow(adf_ucoset_t y, adf_place_t *where,
                       const adf_ucoset_t a, const adf_fball_t x)
{
    CAT_INV_UC(a); ADF_INV_FBALL(x);
    (void)where;
    return profpow(y,a,x,0);
}
int adf_ucoset_profpow_coarse(adf_ucoset_t y, adf_place_t *where,
                              const adf_ucoset_t a, const adf_fball_t x)
{
    CAT_INV_UC(a); ADF_INV_FBALL(x);
    (void)where;
    return profpow(y,a,x,1);
}
int adf_ucoset_profpow_fine(adf_ucoset_t y, adf_place_t *where,
                            const adf_ucoset_t a, const adf_fball_t x)
{
    CAT_INV_UC(a); ADF_INV_FBALL(x);
    (void)where;
    return profpow(y,a,x,2);
}

/* Proposition 15:318-343, canonical precision. The original order is kept for the output.
   Reciprocity names: conventions 6.6; refs/src/milne-cft/CFT.txt:9883-9886. */
static void canon_modulus(fmpz_t m, const fmpz_t n)
{
    fmpz_set(m,n);
    if (fmpz_fdiv_ui(m,4)==2) fmpz_divexact_ui(m,m,2);
}
static int cyclo(fmpz_t j, const adf_idclass_t x, const fmpz_t n, int inverse)
{
    fmpz_t target,N,r;
    int st=ADF_OK;
    if (fmpz_sgn(n)<=0) return ADF_DOMAIN;
    fmpz_init(target); fmpz_init(N); fmpz_init(r);
    canon_modulus(target,n);
    if (fmpz_is_zero(x->u.N)) fmpz_mod(r,x->u.c,n);
    else
    {
        canon_modulus(N,x->u.N);
        if (!fmpz_divisible(N,target)) st=ADF_NOT_DETERMINED;
        else
        {
            fmpz_mod(r,x->u.c,target);
            if (!fmpz_equal(target,n) && fmpz_is_even(r)) fmpz_add(r,r,target);
        }
    }
    if (st==ADF_OK)
    {
        if (inverse && !fmpz_invmod(r,r,n)) flint_abort(); /* Certified unit, also valid at n=1. */
        fmpz_set(j,r);
    }
    fmpz_clear(target); fmpz_clear(N); fmpz_clear(r);
    return st;
}
int adf_idclass_cyclo_exp_u(fmpz_t j, adf_place_t *where, const adf_idclass_t x, const fmpz_t n)
{
    CAT_INV_CLASS(x);
    (void)where;
    return cyclo(j,x,n,0);
}
int adf_idclass_cyclo_exp_uinv(fmpz_t j, adf_place_t *where, const adf_idclass_t x, const fmpz_t n)
{
    CAT_INV_CLASS(x);
    (void)where;
    return cyclo(j,x,n,1);
}
