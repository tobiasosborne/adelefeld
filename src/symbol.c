/* Quadratic symbols. Definitions: docs/proofs/catalogue.md:14-27, precision:31-57.
   FLINT: refs/src/flint-3.0.1/fmpz.rst:1166-1172 (Jacobi: positive odd lower entry;
   Kronecker: every integer lower entry). Conventions 3.1-3.3 and 4.3-4.4. */
#include <adelefeld.h>
#include "invariants.h"
#ifdef ADF_CHECK_INVARIANTS
#define ADF_INV_UCOSET(x) do { if (!adf_ucoset_is_canonical(x)) \
    adf_inv_fail(__func__, #x, "adf_ucoset"); } while (0)
#else
#define ADF_INV_UCOSET(x) ((void)0)
#endif

static int report(int st, adf_place_t *where, adf_place_t p)
{
    if (st != ADF_OK && where != NULL) *where = p;
    return st;
}

/* Definition 1:17-20. Positive lower entry: explicit supplement, including zero for even a.
   Nonpositive entries delegate to FLINT, with independent definition fixtures at both signs and 0. */
static int symbol(int kind, const fmpz_t a, const fmpz_t b)
{
    fmpz_t m;
    ulong t, r;
    int s;
    if (kind != 2) return fmpz_jacobi(a,b);
    if (fmpz_sgn(b) <= 0) return fmpz_kronecker(a,b);
    t = fmpz_val2(b);
    if (t == 0) return fmpz_jacobi(a,b);
    if (fmpz_is_even(a)) return 0;
    r = fmpz_fdiv_ui(a,8);
    s = (t % 2 == 0 || r == 1 || r == 7) ? 1 : -1;
    fmpz_init(m);
    fmpz_fdiv_q_2exp(m,b,t);
    s *= fmpz_jacobi(a,m);
    fmpz_clear(m);
    return s;
}

/* Sufficient K of Proposition 2:31-39, without factorisation. b is positive. */
static void required_modulus(fmpz_t K, int kind, const fmpz_t b)
{
    fmpz_set(K,b);
    if (kind == 2 && fmpz_is_even(b))
    {
        fmpz_fdiv_q_2exp(K,b,fmpz_val2(b));
        fmpz_mul_ui(K,K,8);
    }
}
static int lower_status(int kind, const fmpz_t b)
{
    return kind == 1 && (fmpz_sgn(b) <= 0 || fmpz_is_even(b)) ? ADF_DOMAIN : ADF_OK;
}

/* Y2: exact or certified K|N. No output is changed until the certificate is known. */
static int finite_symbol(int *z, int kind, const fmpz_t a, const fmpz_t N, const fmpz_t b)
{
    fmpz_t K;
    int st = lower_status(kind,b);
    if (st != ADF_OK) return st;
    if (fmpz_is_zero(N)) { *z = symbol(kind,a,b); return ADF_OK; }
    if (fmpz_sgn(b) <= 0) return ADF_NOT_DETERMINED;
    fmpz_init(K);
    required_modulus(K,kind,b);
    st = fmpz_divisible(N,K) ? ADF_OK : ADF_NOT_DETERMINED;
    if (st == ADF_OK) *z = symbol(kind,a,b);
    fmpz_clear(K);
    return st;
}

/* Y3: d divides A+Ht solvable iff gcd(H,d)|A. This distinguishes mixed from empty domain.
   get_fmpz3 reads either backend and yields the canonical global triple. */
static int ball_symbol(int *z, int kind, const adf_fball_t x, const fmpz_t b)
{
    fmpz_t A,H,d,g;
    int st = lower_status(kind,b);
    if (st != ADF_OK) return st;
    fmpz_init(A); fmpz_init(H); fmpz_init(d); fmpz_init(g);
    adf_fball_get_fmpz3(A,H,d,x);
    if (!fmpz_is_one(d))
    {
        fmpz_gcd(g,H,d);
        st = fmpz_divisible(A,g) ? ADF_NOT_DETERMINED : ADF_DOMAIN;
    }
    else st = finite_symbol(z,kind,A,H,b);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); fmpz_clear(g);
    return st;
}
static int legendre_lower(adf_place_t p)
{
    return adf_place_is_archimedean(p) || adf_place_prime_get(p)==2 ? ADF_DOMAIN : ADF_OK;
}

int adf_fmpz_legendre(int *z, adf_place_t *where, const fmpz_t a, adf_place_t p)
{
    fmpz_t b;
    int st = legendre_lower(p);
    if (st != ADF_OK) return report(st,where,p);
    fmpz_init(b); fmpz_set_ui(b,adf_place_prime_get(p));
    *z = symbol(0,a,b);
    fmpz_clear(b);
    return ADF_OK;
}
int adf_fmpz_jacobi(int *z, adf_place_t *where, const fmpz_t a, const fmpz_t b)
{
    int st = lower_status(1,b);
    (void)where;
    if (st == ADF_OK) *z = symbol(1,a,b);
    return st;
}
int adf_fmpz_kronecker(int *z, adf_place_t *where, const fmpz_t a, const fmpz_t b)
{
    (void)where;
    *z = symbol(2,a,b);
    return ADF_OK;
}
int adf_fball_legendre(int *z, adf_place_t *where, const adf_fball_t a, adf_place_t p)
{
    fmpz_t b;
    int st;
    ADF_INV_FBALL(a);
    st = legendre_lower(p);
    if (st != ADF_OK) return report(st,where,p);
    fmpz_init(b); fmpz_set_ui(b,adf_place_prime_get(p));
    st = ball_symbol(z,0,a,b);
    fmpz_clear(b);
    return report(st,where,p);
}
int adf_fball_jacobi(int *z, adf_place_t *where, const adf_fball_t a, const fmpz_t b)
{
    ADF_INV_FBALL(a);
    (void)where;
    return ball_symbol(z,1,a,b);
}
int adf_fball_kronecker(int *z, adf_place_t *where, const adf_fball_t a, const fmpz_t b)
{
    ADF_INV_FBALL(a);
    (void)where;
    return ball_symbol(z,2,a,b);
}
int adf_ucoset_legendre(int *z, adf_place_t *where, const adf_ucoset_t a, adf_place_t p)
{
    fmpz_t b;
    int st;
    ADF_INV_UCOSET(a);
    st = legendre_lower(p);
    if (st != ADF_OK) return report(st,where,p);
    fmpz_init(b); fmpz_set_ui(b,adf_place_prime_get(p));
    st = finite_symbol(z,0,a->c,a->N,b);
    fmpz_clear(b);
    return report(st,where,p);
}
int adf_ucoset_jacobi(int *z, adf_place_t *where, const adf_ucoset_t a, const fmpz_t b)
{
    ADF_INV_UCOSET(a);
    (void)where;
    return finite_symbol(z,1,a->c,a->N,b);
}
int adf_ucoset_kronecker(int *z, adf_place_t *where, const adf_ucoset_t a, const fmpz_t b)
{
    ADF_INV_UCOSET(a);
    (void)where;
    return finite_symbol(z,2,a->c,a->N,b);
}


/* Hilbert symbols: catalogue.md:63-165. The odd formula is :68, the 2 formula :73,
   and the real formula :75. Class precision and scale cofactor:135-163.
   n_mulmod2 and n_invmod: refs/src/flint-3.0.1/ulong_extras.rst:375-379,477-483.
   They accept full-word residues; the inverse exists because the denominator is a local unit. */
#include <flint/ulong_extras.h>
#ifdef ADF_CHECK_INVARIANTS
#define ADF_INV_LBALL(x) do { if (!adf_lball_is_canonical(x)) \
    adf_inv_fail(__func__, #x, "adf_lball"); } while (0)
#define ADF_INV_IDELE(x) do { if (!adf_idele_is_canonical(x)) \
    adf_inv_fail(__func__, #x, "adf_idele"); } while (0)
#else
#define ADF_INV_LBALL(x) ((void)0)
#define ADF_INV_IDELE(x) ((void)0)
#endif

/* At odd primes u[] holds Legendre signs, at 2 odd residues modulo 8. */
typedef struct { int parity, n, u[4]; } square_classes;

static ulong unit_residue(const fmpq_t u, ulong modulus)
{
    ulong n = fmpz_fdiv_ui(fmpq_numref(u),modulus);
    ulong d = fmpz_fdiv_ui(fmpq_denref(u),modulus);
    return n_mulmod2(n,n_invmod(d,modulus),modulus);
}
static int unit_legendre(ulong u, ulong p)
{
    fmpz_t a,b;
    int z;
    fmpz_init(a); fmpz_init(b);
    fmpz_set_ui(a,u); fmpz_set_ui(b,p);
    z = fmpz_jacobi(a,b);
    fmpz_clear(a); fmpz_clear(b);
    return z;
}

/* Strip p from numerator and denominator separately; parity does not subtract valuations.
   fmpz_remove's integer valuation: refs/src/flint-3.0.1/fmpz.rst:1142-1146.
   Neither nonzero rational input nor stripped denominator is divisible by the modulus. */
static void rational_classes(square_classes *c, const fmpq_t q, ulong p)
{
    fmpq_t u;
    fmpz_t prime;
    slong an,bn;
    ulong residue;
    fmpq_init(u); fmpz_init(prime); fmpz_set_ui(prime,p);
    an = fmpz_remove(fmpq_numref(u),fmpq_numref(q),prime);
    bn = fmpz_remove(fmpq_denref(u),fmpq_denref(q),prime);
    c->parity = an%2 != bn%2;
    c->n = 1;
    residue = unit_residue(u,p==2 ? 8 : p);
    c->u[0] = p==2 ? (int)residue : unit_legendre(residue,p);
    fmpq_clear(u); fmpz_clear(prime);
}

/* Enumerate every 2-adic unit class compatible with k known digits and a residue modulo 8.
   k in 0..3: masks 0,1,3,7. Exact coefficients have k=3. */
static void two_classes(square_classes *c, ulong residue, int k)
{
    int mask = (1<<k)-1;
    c->n=0;
    for(int u=1;u<8;u+=2)
        if ((u & mask) == ((int)residue & mask)) c->u[c->n++]=u;
}
static void local_classes(square_classes *c, const adf_lball_t a)
{
    ulong p=a->p;
    c->parity = a->v%2 != 0;
    if (p!=2)
    {
        c->n=1; c->u[0]=unit_legendre(unit_residue(a->u,p),p);
    }
    else
    {
        int k=3;
        if (!a->exact)
        {
            k=1;
            /* Avoid both N-v overflow and v+3 overflow, even at the slong endpoints. */
            if(a->v <= WORD_MAX-2 && a->N >= a->v+2) k=2;
            if(a->v <= WORD_MAX-3 && a->N >= a->v+3) k=3;
        }
        two_classes(c,unit_residue(a->u,8),k);
    }
}
static void idele_classes(square_classes *c, const adf_idele_t a, ulong p)
{
    square_classes scale;
    rational_classes(&scale,a->r,p);
    c->parity=scale.parity;
    if(p!=2)
    {
        if(fmpz_is_zero(a->u.N) || fmpz_fdiv_ui(a->u.N,p)==0)
        {
            c->n=1;
            c->u[0]=scale.u[0]*unit_legendre(fmpz_fdiv_ui(a->u.c,p),p);
        }
        else { c->n=2; c->u[0]=1; c->u[1]=-1; }
    }
    else
    {
        int k=3;
        if(!fmpz_is_zero(a->u.N))
        {
            ulong n=fmpz_val2(a->u.N);
            k=n>=3 ? 3 : (int)n;
        }
        /* The scale cofactor multiplies the coset unit, including its low known digits. */
        ulong residue=(ulong)scale.u[0]*fmpz_fdiv_ui(a->u.c,8)%8;
        two_classes(c,residue,k); /* k=0 intentionally admits all odd unit classes. */
    }
}
static int class_symbol(ulong p, int alpha, int beta, int u, int w)
{
    if(p==2)
    {
        int eps_u=((u-1)/2)%2, eps_w=((w-1)/2)%2;
        int om_u=((u*u-1)/8)%2, om_w=((w*w-1)/8)%2;
        int exponent=eps_u*eps_w + alpha*om_w + beta*om_u;
        return exponent%2 ? -1 : 1;
    }
    return (alpha && beta && p%4==3 ? -1 : 1) * (beta ? u : 1) * (alpha ? w : 1);
}
static int class_result(int *z, ulong p, const square_classes *a, const square_classes *b)
{
    int sign=class_symbol(p,a->parity,b->parity,a->u[0],b->u[0]);
    for(int i=0;i<a->n;i++) for(int j=0;j<b->n;j++)
        if(class_symbol(p,a->parity,b->parity,a->u[i],b->u[j])!=sign) return ADF_NOT_DETERMINED;
    *z=sign;
    return ADF_OK;
}

/* Y5-Y7. Exact zero is outside; a ball meeting zero meets the domain and its complement. */
int adf_lball_hilbert(int *z, adf_place_t *where, const adf_lball_t a, const adf_lball_t b)
{
    square_classes ac,bc;
    adf_place_t v,w;
    int st;
    ADF_INV_LBALL(a); ADF_INV_LBALL(b);
    v=adf_lball_place(a); w=adf_lball_place(b);
    if(a->p != b->p) return report(ADF_DOMAIN,where,adf_place_cmp(v,w)<0 ? v : w);
    if((a->exact && fmpq_is_zero(a->u)) || (b->exact && fmpq_is_zero(b->u)))
        return report(ADF_DOMAIN,where,v);
    if(fmpq_is_zero(a->u) || fmpq_is_zero(b->u)) return report(ADF_NOT_DETERMINED,where,v);
    local_classes(&ac,a); local_classes(&bc,b);
    st=class_result(z,a->p,&ac,&bc);
    return report(st,where,v);
}
int adf_real_hilbert(int *z, adf_place_t *where, const arb_t a, const arb_t b)
{
    adf_place_t v=adf_place_inf();
    if(!arb_is_finite(a) || !arb_is_finite(b) || arb_is_zero(a) || arb_is_zero(b))
        return report(ADF_DOMAIN,where,v);
    if(arb_contains_zero(a) || arb_contains_zero(b)) return report(ADF_NOT_DETERMINED,where,v);
    *z=arb_is_negative(a) && arb_is_negative(b) ? -1 : 1;
    return ADF_OK;
}
int adf_rat_hilbert_at(int *z, adf_place_t *where, const adf_rat_t a,
                       const adf_rat_t b, adf_place_t v)
{
    square_classes ac,bc;
    int st;
    ADF_INV_RAT(a); ADF_INV_RAT(b);
    if(adf_rat_is_zero(a) || adf_rat_is_zero(b)) return report(ADF_DOMAIN,where,v);
    if(adf_place_is_archimedean(v))
    {
        *z=adf_rat_sgn(a)<0 && adf_rat_sgn(b)<0 ? -1 : 1;
        return ADF_OK;
    }
    rational_classes(&ac,a->q,adf_place_prime_get(v));
    rational_classes(&bc,b->q,adf_place_prime_get(v));
    st=class_result(z,adf_place_prime_get(v),&ac,&bc);
    return report(st,where,v);
}
int adf_idele_hilbert_at(int *z, adf_place_t *where, const adf_idele_t a,
                         const adf_idele_t b, adf_place_t v)
{
    square_classes ac,bc;
    int st;
    ADF_INV_IDELE(a); ADF_INV_IDELE(b);
    if(adf_place_is_archimedean(v)) return adf_real_hilbert(z,where,a->inf,b->inf);
    idele_classes(&ac,a,adf_place_prime_get(v));
    idele_classes(&bc,b,adf_place_prime_get(v));
    st=class_result(z,adf_place_prime_get(v),&ac,&bc);
    return report(st,where,v);
}
