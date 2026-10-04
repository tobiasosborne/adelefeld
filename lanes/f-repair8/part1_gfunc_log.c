/* Log on ideles, WP 1F.8. Mathematical contract: docs/design/idele-log.md IL1-IL8;
   docs/proofs/functions.md:265-294,336-406; docs/api-1f8.md G7-G9.
   FLINT sources read: refs/src/flint-3.0.1/fmpz.rst:913-917 (powers),1132-1149
   (factor removal),1154-1161 (inverse); arb.rst:597-609,1050-1058 (finite Log enclosure).
   All writes are through temporaries. No helper is copied from src/gfunc.c. */
#include <stdlib.h>
#include <adelefeld.h>
#include "invariants.h"

#ifdef ADF_CHECK_INVARIANTS
#define ADF_INV_IDLOG(x) \
    do { if (!adf_idele_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idele"); } while (0)
#else
#define ADF_INV_IDLOG(x) ((void) 0)
#endif

typedef int (*real_at_fn)(adf_sball_t, adf_place_t *, const adf_sball_t, adf_place_t, slong);

/* G7: route through the named real-place API, retaining the identical ball and statuses. */
static int
real_log(arb_t y, const adf_idele_t x, slong prec, real_at_fn at)
{
    adf_sball_t s, t;
    int st;
    adf_sball_init(s); adf_sball_init(t);
    st = adf_sball_set_arb_lballs(s, NULL, x->inf, NULL, 0);
    if (st == ADF_OK)
        st = at(t, NULL, s, adf_place_inf(), prec);
    if (st == ADF_OK)
        st = adf_sball_get_arb(y, t, adf_place_inf());
    adf_sball_clear(s); adf_sball_clear(t);
    return st;
}

static int
conservative(adf_adele_t y, adf_place_t *where, const adf_idele_t x, slong prec, real_at_fn at)
{
    adf_adele_t t;
    int st;
    if (prec > ADF_REAL_PREC_MAX)
    {
        if (where != NULL) *where = adf_place_inf();
        return ADF_LIMIT;
    }
    ADF_INV_IDLOG(x);
    adf_adele_init(t);
    st = real_log(t->inf, x, prec, at);
    if (st == ADF_OK)
    {
        /* IL5: the canonical global triple (0,4,1), even for exact input. */
        fmpz_set_ui(t->fin.H, 4);
        adf_adele_swap(y, t);
    }
    else if (where != NULL) *where = adf_place_inf();
    adf_adele_clear(t);
    return st;
}
int
adf_idele_Log(adf_adele_t y, adf_place_t *where, const adf_idele_t x, slong prec)
{
    return conservative(y, where, x, prec, adf_sball_Log_at);
}
int
adf_idele_log_abs(adf_adele_t y, adf_place_t *where, const adf_idele_t x, slong prec)
{
    return conservative(y, where, x, prec, adf_sball_log_abs_at);
}

/* G8, IL1: a descriptor, not an additive local input ball. The rational unit is r',
   m is v_p(r), k is v_p(M). Exact input and unrestricted input are different tags. */
typedef struct
{
    ulong p;
    slong m, k, K;
    int exact;
    fmpq_t unit;
} local_desc;

static int
exp_ok(slong K)
{
    return K >= -ADF_LBALL_EXP_MAX && K <= ADF_LBALL_EXP_MAX;
}

/* fmpz_remove is read at fmpz.rst:1142-1149. Both valuations are nonnegative slong;
   their difference fits slong. k is never added to m. No factorisation is used. */
static int
describe(local_desc *d, const adf_idele_t x, ulong p, slong N)
{
    fmpz_t prime, stripped;
    slong a, b, E;
    d->p = p;
    d->exact = fmpz_is_zero(x->u.N);
    fmpz_init(prime); fmpz_init(stripped);
    fmpz_set_ui(prime, p);
    a = fmpz_remove(fmpq_numref(d->unit), fmpq_numref(x->r), prime);
    b = fmpz_remove(fmpq_denref(d->unit), fmpq_denref(x->r), prime);
    d->m = a - b;
    d->k = d->exact ? 0 : fmpz_remove(stripped, x->u.N, prime);
    E = p == 2 ? 2 : 1;
    if (d->k > E) E = d->k;
    d->K = d->exact || N < E ? N : E;
    fmpz_clear(prime); fmpz_clear(stripped);
    return exp_ok(d->m) ? ADF_OK : ADF_LIMIT;
}

static void
zero_ball(adf_lball_t y, ulong p, slong K, int exact)
{
    y->p = p; fmpq_zero(y->u); y->v = 0; y->N = exact ? 0 : K; y->exact = exact;
}

/* G8-G9: IL2-IL4, compact centre at K. If a agrees with r'c modulo p^K, K>d,
   b/a is in 1+p^K Z_p. Lemma 9 and Proposition 11:336-364 give Log(b)=Log(a)
   modulo p^K. Thus an arbitrary higher lift is valid for lball_Log's working sum.
   Its finite answer is rounded even when that representative has exact Log zero. */
static int
local_log(adf_lball_t y, const adf_idele_t x, ulong p, slong N)
{
    local_desc d;
    adf_lball_t a, t;
    fmpz_t prime, mod, residue, inv;
    slong bits, c = p == 2 ? 2 : 1;
    int st;
    fmpq_init(d.unit);
    st = describe(&d, x, p, N);
    if (st != ADF_OK) goto done_desc;
    if (d.exact && fmpq_is_one(d.unit))
    {
        zero_ball(y, p, 0, 1); /* IL4: Log(+-p^m)=0, before checking requested N. */
        goto done_desc;
    }
    if (!exp_ok(d.K)) { st = ADF_LIMIT; goto done_desc; }
    if (d.K <= c || (!d.exact && (d.k == 0 || (p == 2 && d.k == 1))) ||
        (fmpq_is_one(d.unit) && fmpz_is_one(x->u.c)))
    {
        zero_ball(y, p, d.K, 0);
        goto done_desc;
    }
    fmpz_init(prime); fmpz_set_ui(prime, p);
    bits = (slong) fmpz_bits(prime);
    if (d.K > ADF_LBALL_BITS_MAX / bits)
    {
        st = ADF_LIMIT; fmpz_clear(prime); goto done_desc;
    }
    fmpz_init(mod); fmpz_init(residue); fmpz_init(inv);
    adf_lball_init(a); adf_lball_init(t);
    fmpz_pow_ui(mod, prime, (ulong) d.K);
    fmpz_mod(residue, fmpq_numref(d.unit), mod);
    fmpz_mod(inv, x->u.c, mod);
    fmpz_mul(residue, residue, inv); fmpz_mod(residue, residue, mod);
    /* Denominator is prime-free, so the inverse exists (IL1). */
    if (!fmpz_invmod(inv, fmpq_denref(d.unit), mod)) flint_abort();
    fmpz_mul(residue, residue, inv); fmpz_mod(residue, residue, mod);
    a->p = p; fmpq_set_fmpz(a->u, residue); /* exact unit, v=0, N=0 */
    st = adf_lball_Log(t, a, d.K); /* F3,F5,F6: computes and checks its actual W. */
    if (st == ADF_OK)
    {
        if (t->exact) zero_ball(t, p, d.K, 0);
        adf_lball_swap(y, t);
    }
    adf_lball_clear(a); adf_lball_clear(t);
    fmpz_clear(prime); fmpz_clear(mod); fmpz_clear(residue); fmpz_clear(inv);
done_desc:
    fmpq_clear(d.unit);
    return st;
}

static int
one_place(adf_sball_t y, adf_place_t *where, const adf_idele_t x,
          adf_place_t v, slong prec, int absval)
{
    adf_sball_t t;
    adf_lball_t l;
    arb_t r;
    int st;
    if (adf_place_is_archimedean(v) && prec > ADF_REAL_PREC_MAX)
    {
        if (where != NULL) *where = v;
        return ADF_LIMIT;
    }
    ADF_INV_IDLOG(x);
    if (absval && !adf_place_is_archimedean(v))
    {
        if (where != NULL) *where = v;
        return ADF_UNSUPPORTED;
    }
    adf_sball_init(t); adf_lball_init(l); arb_init(r);
    if (adf_place_is_archimedean(v))
    {
        st = real_log(r, x, prec, absval ? adf_sball_log_abs_at : adf_sball_Log_at);
        if (st == ADF_OK) st = adf_sball_set_arb_lballs(t, NULL, r, NULL, 0);
    }
    else
    {
        st = local_log(l, x, adf_place_prime_get(v), prec);
        if (st == ADF_OK) st = adf_sball_set_arb_lballs(t, NULL, NULL, l, 1);
    }
    if (st == ADF_OK) adf_sball_swap(y, t);
    else if (where != NULL) *where = v;
    adf_sball_clear(t); adf_lball_clear(l); arb_clear(r);
    return st;
}
int
adf_idele_Log_at(adf_sball_t y, adf_place_t *where, const adf_idele_t x, adf_place_t v, slong prec)
{
    return one_place(y, where, x, v, prec, 0);
}
int
adf_idele_log_abs_at(adf_sball_t y, adf_place_t *where, const adf_idele_t x, adf_place_t v, slong prec)
{
    return one_place(y, where, x, v, prec, 1);
}

/* G10-G12: IL5's baseline intersection and CRT. Ground truth for integer CRT:
   refs/src/baker-padic/padicnotes.txt:374-387; FLINT's canonical nonnegative lift:
   refs/src/flint-3.0.1/fmpz.rst:1292-1304. No full factorisation is required. */
static int
place_order(const void *a, const void *b)
{
    return adf_place_cmp(*(const adf_place_t *)a, *(const adf_place_t *)b);
}
static slong
max_exp(slong a, slong b)
{
    return a > b ? a : b;
}

/* List size and shape have already been checked. Detect local exponent and compact
   power bounds before the aggregate bound. Aggregate work is measured by L bits(p),
   including 2^2. Compare with a remaining budget before multiplying or allocating.
   Exact local zero must be rounded here; it does not bypass the exponent check. */
static int
refine_budget(adf_place_t *where, slong *known, const adf_idele_t x,
              const adf_place_t *ps, slong n, slong N)
{
    local_desc d;
    fmpz_t pz;
    slong total = 4;
    int aggregate = 0, st;
    fmpq_init(d.unit); fmpz_init(pz);
    for (slong i = 0; i < n; i++)
    {
        ulong p = adf_place_prime_get(ps[i]);
        slong K, L, bits, charge;
        st = describe(&d, x, p, N);
        K = d.K;
        fmpz_set_ui(pz, p); bits = (slong) fmpz_bits(pz);
        if (st == ADF_OK && !exp_ok(K)) st = ADF_LIMIT;
        if (st == ADF_OK && K > (p == 2 ? 2 : 1) &&
            !(fmpq_is_one(d.unit) && (d.exact || fmpz_is_one(x->u.c))) &&
            K > ADF_LBALL_BITS_MAX / bits)
            st = ADF_LIMIT;
        if (st != ADF_OK)
        {
            if (where != NULL) *where = ps[i];
            *known = i;
            fmpq_clear(d.unit); fmpz_clear(pz);
            return st; /* sorted: first known local LIMIT, maximal finite status */
        }
        L = max_exp(K, p == 2 ? 2 : 0);
        charge = p == 2 ? L - 2 : L; /* baseline 2^2 was charged once */
        if (charge > (ADF_IDLOG_CRT_BITS_MAX - total) / bits)
            aggregate = 1;
        else total += charge * bits;
    }
    fmpq_clear(d.unit); fmpz_clear(pz);
    return aggregate ? ADF_LIMIT : ADF_OK; /* combined modulus: no particular place */
}

static int
refine(adf_adele_t y, adf_place_t *where, const adf_idele_t x,
       const adf_place_t *primes, slong n, slong N, slong prec, real_at_fn at)
{
    adf_place_t *ps;
    adf_place_t wp = adf_place_inf();
    adf_adele_t t;
    adf_lball_t l;
    adf_rat_t centre;
    fmpz_t pz, q, b, A;
    int st, st_real, st_fin = ADF_OK;
    slong known = -1;
    if (prec > ADF_REAL_PREC_MAX)
    {
        if (where != NULL) *where = adf_place_inf();
        return ADF_LIMIT;
    }
    if (n < 0) return ADF_DOMAIN;
    if (n > ADF_IDLOG_PLACES_MAX) return ADF_LIMIT;
    ps = n == 0 ? NULL : flint_malloc((size_t)n * sizeof(*ps));
    if (n != 0)
    {
        for (slong i = 0; i < n; i++) ps[i] = primes[i];
        qsort(ps, (size_t)n, sizeof(*ps), place_order);
        for (slong i = 0; i < n; i++)
        {
            if (adf_place_is_archimedean(ps[i]) || (i > 0 && adf_place_equal(ps[i-1], ps[i])))
            {
                if (where != NULL) *where = ps[i];
                flint_free(ps);
                return ADF_DOMAIN;
            }
        }
    }
    ADF_INV_IDLOG(x);
    st = refine_budget(&wp, &known, x, ps, n, N);
    if (st != ADF_OK)
    {
        if (known >= 0)
        {
            /* A preceding prime can hit its actual working-power bound, also LIMIT.
               Evaluate those primes to preserve canonical ties with the known LIMIT. */
            adf_lball_init(l);
            for (slong i = 0; i < known; i++)
                if (local_log(l, x, adf_place_prime_get(ps[i]), N) != ADF_OK)
                { wp = ps[i]; break; }
            adf_lball_clear(l);
            if (where != NULL) *where = wp;
        }
        flint_free(ps); return st;
    }
    adf_adele_init(t); adf_lball_init(l); adf_rat_init(centre);
    fmpz_init(pz); fmpz_init(q); fmpz_init(b); fmpz_init(A);
    st_real = real_log(t->inf, x, prec, at);
    fmpz_set_ui(t->fin.H, 4); /* baseline */
    for (slong i = 0; i < n; i++)
    {
        ulong p = adf_place_prime_get(ps[i]);
        slong K, L;
        st = local_log(l, x, p, N);
        if (st != ADF_OK)
        {
            st_fin = st; wp = ps[i];
            break; /* finite failures are LIMIT; remaining primes cannot outrank it */
        }
        K = l->exact ? N : l->N;
        L = max_exp(K, p == 2 ? 2 : 0);
        if (L <= (p == 2 ? 2 : 0)) continue;
        fmpz_set_ui(pz, p); fmpz_pow_ui(q, pz, (ulong)L);
        st = adf_lball_get_center(centre, l); /* integral, canonical centre at K, or exact 0 */
        if (st != ADF_OK) { st_fin = st; wp = ps[i]; break; }
        fmpz_set(b, fmpq_numref(centre->q));
        if (p == 2)
        {
            /* 2 is first when present. Replace its baseline by the stronger condition. */
            fmpz_set(t->fin.A, b); fmpz_set(t->fin.H, q);
        }
        else
        {
            fmpz_CRT(A, t->fin.A, t->fin.H, b, q, 0);
            fmpz_swap(t->fin.A, A); fmpz_mul(t->fin.H, t->fin.H, q);
        }
    }
    st = st_fin;
    if (st_real > st_fin) st = st_real;
    if (st == ADF_OK) adf_adele_swap(y, t);
    else if (where != NULL) *where = st_real == st ? adf_place_inf() : wp;
    adf_adele_clear(t); adf_lball_clear(l); adf_rat_clear(centre);
    fmpz_clear(pz); fmpz_clear(q); fmpz_clear(b); fmpz_clear(A); flint_free(ps);
    return st;
}
int
adf_idele_Log_refine(adf_adele_t y, adf_place_t *where, const adf_idele_t x,
                     const adf_place_t *primes, slong n, slong N, slong prec)
{
    return refine(y, where, x, primes, n, N, prec, adf_sball_Log_at);
}
int
adf_idele_log_abs_refine(adf_adele_t y, adf_place_t *where, const adf_idele_t x,
                         const adf_place_t *primes, slong n, slong N, slong prec)
{
    return refine(y, where, x, primes, n, N, prec, adf_sball_log_abs_at);
}
