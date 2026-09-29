/* roots.c: the type adf_rootlist, the seed function and the entries verifier at a prime (milestone S,
   S.2, slice 1).

   The contract is the comment block of include/adelefeld/roots.h: docs/api-s.md sections 1 and 4
   (decisions S-D10, S-D13, S-D14, S-D16, S-D17, S-D18 of docs/SPEC.md 15.3). The statements are those
   of docs/proofs/solvers.md section 3 (cited below as solvers.md:<line>); the reference is
   proto/solvers_checks.py (squarefree_part at line 2377, normalise_g 1701, root_cert_ok 1631,
   newton_step 1639, seed_root 1721, balls_meet 1749, padic_verify_entries 1754); the tests are
   tests/test_roots_seed.c.

   Everything is exact integer arithmetic with fmpz: the valuations by fmpz_remove, the residues by
   fmpz_mod, the evaluations modulo p^m by Horner's rule. p is a prime of a place (roots.h, "The
   prime"); no step needs the roots modulo p, and every step holds for every prime, 2 included
   (solvers.md:1003). FLINT routines used and what they promise (refs/src/flint-3.0.1):
     fmpz_poly_gcd, fmpz_poly.rst:1271: the gcd of two polynomials of Z[X], "normalised to have
       non-negative leading coefficient";
     fmpz_poly_divides, fmpz_poly.rst:1923: 1 and the quotient if B divides A exactly;
     fmpz_poly_primitive_part, fmpz_poly.rst:1476: divided by the content, "non-negative leading
       coefficient"; fmpz_poly_content, fmpz_poly.rst:1461: the non-negative content;
     fmpz_poly_derivative, fmpz_poly.rst:2176; fmpz_poly_evaluate_fmpz, fmpz_poly.rst:2230;
     fmpz_remove, fmpz.rst:1142: removes the factor f > 1 and returns the valuation;
     fmpz_mod, fmpz.rst:880: the remainder is non-negative for a positive modulus;
     fmpz_invmod, fmpz.rst:1154: the inverse modulo h, return value 0 if there is none;
     fmpz_pow_ui, fmpz.rst:913; fmpz_bits, fmpz.rst:605: the number of bits of |f|;
     arb_lt and arb_is_finite, arb.rst:705 to 716 and 606.

   Exponents are slong and are computed with the checked operations below (S-D18). A power p^m is
   formed only after 2 K bits(p) <= ADF_ROOTS_BITS_MAX has been tested for the K of the call, and every
   m used is below 2 K. */

#include <flint/fmpz_vec.h>
#include <flint/nmod_poly.h>

#include <flint/ulong_extras.h>   /* n_is_prime */
#include <adelefeld.h>

#if defined(__GNUC__) || defined(__clang__)
#define ADF_ROOTS_HIDDEN __attribute__((visibility("hidden")))
#else
#define ADF_ROOTS_HIDDEN
#endif

/* Not part of the interface (hidden in a shared object; tests/test_exports.sh): the seed computation
   and the certificate check for a prime given as an fmpz. The public functions call them with the
   prime of a place; tests/test_roots_seed.c calls them with primes of more than one word. */
ADF_ROOTS_HIDDEN int adf_roots_seed_core(fmpz_t a_out, slong * K, slong * s, const fmpz_poly_t g,
                                         const fmpz_t p, const fmpz_t a, slong prec_p);
ADF_ROOTS_HIDDEN int adf_roots_cert_check(const fmpz_poly_t g, const fmpz_t p, const fmpz_t a, slong K, slong s);

/* ---- checked slong arithmetic (S-D18) ---- */

/* *r = x + y; 0 if the sum is outside [WORD_MIN, WORD_MAX] (then *r is untouched) */
static int
add_checked(slong * r, slong x, slong y)
{
    if ((y > 0 && x > WORD_MAX - y) || (y < 0 && x < WORD_MIN - y))
        return 0;
    *r = x + y;
    return 1;
}

/* *r = x - y; 0 on overflow */
static int
sub_checked(slong * r, slong x, slong y)
{
    if ((y < 0 && x > WORD_MAX + y) || (y > 0 && x < WORD_MIN + y))
        return 0;
    *r = x - y;
    return 1;
}

/* 1 if 0 <= K and 2 K bits <= ADF_ROOTS_BITS_MAX (bits >= 1): the limit of S-D18, decided by a
   division, so that nothing overflows (2 K b <= M exactly when K <= floor(M / (2 b))) */
static int
prec_within_limit(slong K, ulong bits)
{
    return K >= 0 && (ulong) K <= (ulong) ADF_ROOTS_BITS_MAX / (2 * bits);
}

/* ---- arithmetic modulo p^m ---- */

/* r = g(x) mod M by Horner's rule, M > 0; r must not alias x */
static void
eval_mod(fmpz_t r, const fmpz_poly_t g, const fmpz_t x, const fmpz_t M)
{
    fmpz_t y;
    slong i;

    fmpz_init(y);
    fmpz_mod(y, x, M);
    fmpz_zero(r);
    for (i = fmpz_poly_length(g) - 1; i >= 0; i--)
    {
        fmpz_mul(r, r, y);
        fmpz_add(r, r, g->coeffs + i);
        fmpz_mod(r, r, M);
    }
    fmpz_clear(y);
}

/* v_p(x) for x != 0 and p >= 2 */
static slong
val_p(const fmpz_t x, const fmpz_t p)
{
    fmpz_t t;
    slong v;

    fmpz_init(t);
    v = fmpz_remove(t, x, p);
    fmpz_clear(t);
    return v;
}

/* 1 if 0 <= a < p^K, for K >= 0 and p >= 2. p^K is formed only when K < bits(a): otherwise
   a < 2^bits(a) <= 2^K <= p^K. */
static int
below_power(const fmpz_t a, const fmpz_t p, slong K)
{
    fmpz_t q;
    int r;

    if (fmpz_sgn(a) < 0)
        return 0;
    if (fmpz_is_zero(a) || (ulong) K >= fmpz_bits(a))
        return 1;
    fmpz_init(q);
    fmpz_pow_ui(q, p, (ulong) K);
    r = fmpz_cmp(a, q) < 0;
    fmpz_clear(q);
    return r;
}

/* 1 if the balls x + p^k Z_p and y + p^l Z_p meet, that is x = y modulo p^min(k, l) (solvers.md:1007
   to 1008; balls_meet, proto/solvers_checks.py:1749), for k, l >= 0; by the valuation of x - y, so
   that no power of p is formed */
static int
balls_meet(const fmpz_t x, slong k, const fmpz_t y, slong l, const fmpz_t p)
{
    fmpz_t d;
    int r;

    if (fmpz_equal(x, y))
        return 1;
    fmpz_init(d);
    fmpz_sub(d, x, y);
    r = val_p(d, p) >= FLINT_MIN(k, l);
    fmpz_clear(d);
    return r;
}

/* ---- the normalised polynomial (solvers L3.1(3), solvers.md:1020 to 1023) ---- */

/* g = f / gcd(f, f') made primitive with positive leading coefficient, for f != 0; g = 1 for f
   constant. *reduced = 1 iff deg gcd(f, f') > 0 for nonconstant f (roots.h). gcd(f, f') of
   fmpz_poly_gcd is a gcd in Z[X], so it divides f in Z[X]; the division is exact, and a failure of
   fmpz_poly_divides would be a defect of FLINT (abort). g may alias f. */
static void
normalise(fmpz_poly_t g, int * reduced, const fmpz_poly_t f)
{
    fmpz_poly_t d, h, q;

    if (fmpz_poly_degree(f) <= 0)
    {
        *reduced = 0;
        fmpz_poly_set_ui(g, 1);
        return;
    }
    fmpz_poly_init(d);
    fmpz_poly_init(h);
    fmpz_poly_init(q);
    fmpz_poly_derivative(d, f);
    fmpz_poly_gcd(h, f, d);
    *reduced = fmpz_poly_degree(h) > 0;
    if (!fmpz_poly_divides(q, f, h))
        flint_abort();
    fmpz_poly_primitive_part(g, q);
    fmpz_poly_clear(d);
    fmpz_poly_clear(h);
    fmpz_poly_clear(q);
}

/* 1 if g is its own normalised polynomial: not 0, content 1, positive leading coefficient, and
   gcd(g, g') constant (g = 1 when g is constant) */
static int
is_normalised(const fmpz_poly_t g)
{
    fmpz_poly_t d, h;
    fmpz_t c;
    int r;

    if (fmpz_poly_is_zero(g))
        return 0;
    if (fmpz_poly_degree(g) == 0)
        return fmpz_poly_is_one(g);
    if (fmpz_sgn(g->coeffs + fmpz_poly_degree(g)) <= 0)
        return 0;
    fmpz_init(c);
    fmpz_poly_content(c, g);
    r = fmpz_is_one(c);
    fmpz_clear(c);
    if (!r)
        return 0;
    fmpz_poly_init(d);
    fmpz_poly_init(h);
    fmpz_poly_derivative(d, g);
    fmpz_poly_gcd(h, g, d);
    r = fmpz_poly_degree(h) == 0;
    fmpz_poly_clear(d);
    fmpz_poly_clear(h);
    return r;
}

/* ---- life cycle ---- */

void
adf_rootlist_init(adf_rootlist_t L)
{
    L->place = adf_place_inf();
    L->scope = ADF_ROOTLIST_PARTITION;
    L->reduced = 0;
    L->complete = 1;
    fmpz_poly_init(L->g);
    fmpz_poly_set_ui(L->g, 1);
    L->n = 0;
    L->a = NULL;
    L->K = NULL;
    L->s = NULL;
    L->nu = 0;
    L->ua = NULL;
    L->ue = NULL;
    L->ball = NULL;
    L->count = 0;
}

/* releases the arrays and sets the pointers to NULL; g and the integer fields stay */
static void
free_arrays(adf_rootlist_t L)
{
    if (L->a != NULL)
        _fmpz_vec_clear(L->a, L->n);
    flint_free(L->K);
    flint_free(L->s);
    if (L->ua != NULL)
        _fmpz_vec_clear(L->ua, L->nu);
    flint_free(L->ue);
    if (L->ball != NULL)
        _arb_vec_clear(L->ball, L->n);
    L->a = NULL;
    L->K = NULL;
    L->s = NULL;
    L->ua = NULL;
    L->ue = NULL;
    L->ball = NULL;
}

void
adf_rootlist_clear(adf_rootlist_t L)
{
    free_arrays(L);
    fmpz_poly_clear(L->g);
}

/* ---- the predicate ---- */

/* the pointer rule of roots.h: a pointer is NULL exactly when its length is 0 */
static int
ptr_ok(const void * ptr, slong len)
{
    return (ptr == NULL) == (len == 0);
}

/* the shape of a list at a prime: lengths, pointers, and the rules of the scope (roots.h, the
   predicate; solvers P3.12(5), P3.13(3)); the place is a prime. A place made by adf_place_prime is a
   proved prime (src/place.c); a list of unknown origin may carry a word written by hand, so the test
   of adf_place_prime (n_is_prime, certified for every word: refs/src/flint-3.0.1/ulong_extras.rst:833
   to 836) is made again here (docs/reviews/s2/review.md, finding 1). It also refuses p < 2. */
static int
shape_ok_prime(const adf_rootlist_t L)
{
    if (!n_is_prime(adf_place_prime_get(L->place)))
        return 0;
    if (L->n < 0 || L->nu < 0 || L->count != 0 || L->ball != NULL)
        return 0;
    if (!ptr_ok(L->a, L->n) || !ptr_ok(L->K, L->n) || !ptr_ok(L->s, L->n))
        return 0;
    if (!ptr_ok(L->ua, L->nu) || !ptr_ok(L->ue, L->nu))
        return 0;
    if (L->reduced != 0 && L->reduced != 1)
        return 0;
    if (L->scope == ADF_ROOTLIST_SEED)
        return L->n == 1 && L->nu == 0 && L->complete == 0;
    if (L->scope == ADF_ROOTLIST_PARTITION)
        return L->complete == (L->nu == 0 ? 1 : 0);
    return 0;
}

/* the balls and classes of a list at p pairwise disjoint (solvers P3.2(4)); exponents >= 0 */
static int
disjoint(const adf_rootlist_t L, const fmpz_t p)
{
    slong i, j, m = L->n + L->nu;

    for (i = 0; i < m; i++)
        for (j = 0; j < i; j++)
        {
            const fmpz * x = i < L->n ? L->a + i : L->ua + (i - L->n);
            const fmpz * y = j < L->n ? L->a + j : L->ua + (j - L->n);
            slong k = i < L->n ? L->K[i] : L->ue[i - L->n];
            slong l = j < L->n ? L->K[j] : L->ue[j - L->n];
            if (balls_meet(x, k, y, l, p))
                return 0;
        }
    return 1;
}

static int
is_canonical_prime(const adf_rootlist_t L)
{
    fmpz_t p;
    slong i;
    int r = 1;

    if (!shape_ok_prime(L))
        return 0;
    fmpz_init_set_ui(p, adf_place_prime_get(L->place));
    for (i = 0; i < L->n && r; i++)
    {
        /* (R1), solvers.md:1048 */
        r = L->s[i] >= 0 && L->K[i] > L->s[i] && below_power(L->a + i, p, L->K[i]);
        if (r && i > 0)
            r = fmpz_cmp(L->a + i - 1, L->a + i) < 0;
    }
    for (i = 0; i < L->nu && r; i++)
    {
        r = L->ue[i] >= 0 && below_power(L->ua + i, p, L->ue[i]);
        if (r && i > 0)
            r = fmpz_cmp(L->ua + i - 1, L->ua + i) < 0;
    }
    r = r && disjoint(L, p);
    fmpz_clear(p);
    return r;
}

/* at the real place (temporary, roots.h): PARTITION, complete, n = count, finite balls with
   hi_i < lo_(i+1) */
static int
is_canonical_real(const adf_rootlist_t L)
{
    slong i;

    if (L->scope != ADF_ROOTLIST_PARTITION || L->complete != 1 || L->nu != 0 || L->n < 0 || L->count != L->n)
        return 0;
    if (L->reduced != 0 && L->reduced != 1)
        return 0;
    if (L->a != NULL || L->K != NULL || L->s != NULL || L->ua != NULL || L->ue != NULL)
        return 0;
    if (!ptr_ok(L->ball, L->n))
        return 0;
    for (i = 0; i < L->n; i++)
    {
        if (!arb_is_finite(L->ball + i))
            return 0;
        if (i > 0 && !arb_lt(L->ball + i - 1, L->ball + i))
            return 0;
    }
    return 1;
}

int
adf_rootlist_is_canonical(const adf_rootlist_t L)
{
    if (!is_normalised(L->g))
        return 0;
    if (adf_place_is_archimedean(L->place))
        return is_canonical_real(L);
    return is_canonical_prime(L);
}

/* ---- the certificate check (solvers D3.2, solvers.md:1046 to 1050; root_cert_ok,
   proto/solvers_checks.py:1631) ---- */

int
adf_roots_cert_check(const fmpz_poly_t g, const fmpz_t p, const fmpz_t a, slong K, slong s)
{
    fmpz_poly_t d;
    fmpz_t q, v;
    slong Ks;
    int r;

    /* (R1): K > s >= 0 and 0 <= a < p^K; a K beyond the limit is not tested (roots.h) */
    if (!(s >= 0 && K > s) || !prec_within_limit(K, fmpz_bits(p)) || !add_checked(&Ks, K, s))
        return 0;
    if (!below_power(a, p, K))
        return 0;
    fmpz_poly_init(d);
    fmpz_init(q);
    fmpz_init(v);
    /* (R2): v(g'(a)) = s: g'(a) = 0 modulo p^s and not modulo p^(s+1) */
    fmpz_poly_derivative(d, g);
    fmpz_pow_ui(q, p, (ulong) s + 1);
    eval_mod(v, d, a, q);
    r = !fmpz_is_zero(v);
    if (r)
    {
        fmpz_pow_ui(q, p, (ulong) s);
        r = fmpz_divisible(v, q);
    }
    /* (R3): v(g(a)) >= K + s */
    if (r)
    {
        fmpz_pow_ui(q, p, (ulong) Ks);
        eval_mod(v, g, a, q);
        r = fmpz_is_zero(v);
    }
    fmpz_poly_clear(d);
    fmpz_clear(q);
    fmpz_clear(v);
    return r;
}

/* ---- the seed computation (solvers P3.12, P3.3, P3.2(3)) ---- */

/* One Newton step of solvers P3.3 (solvers.md:1092 to 1094; newton_step, proto/solvers_checks.py:1639):
   from the certificate (a, k, s) for (g, p), F = g(a)/p^(k+s) and D = g'(a)/p^s, u = D^(-1) modulo
   p^(k-s), a = (a - p^k F u) mod p^(2k - s), k = 2k - s. a is in [0, p^k) on entry. d is g'. Returns
   0 on an overflow of an exponent (then a and k are untouched). 2k < 2K, so the powers are within
   the limit tested by the caller. The inverse exists because D is prime to p (R2); if FLINT finds
   none, or if the precision does not grow, the certificate was not one: a defect, and the function
   aborts instead of returning a ball that is not proved. */
static int
newton_step(fmpz_t a, slong * k, const fmpz_poly_t g, const fmpz_poly_t d, const fmpz_t p, slong s)
{
    fmpz_t pk, ps, q, F, D, u;
    slong k2k, k2, kps, kms;

    if (!add_checked(&k2k, *k, *k) || !sub_checked(&k2, k2k, s) || !add_checked(&kps, *k, s) ||
        !sub_checked(&kms, *k, s))
        return 0;
    if (k2 <= *k || kms < 1)
    {
        flint_printf("adelefeld: adf_root_padic_from_seed: internal error: Newton step from k = %wd, s = %wd\n",
                     *k, s);
        flint_abort();
    }
    fmpz_init(pk);
    fmpz_init(ps);
    fmpz_init(q);
    fmpz_init(F);
    fmpz_init(D);
    fmpz_init(u);
    fmpz_pow_ui(pk, p, (ulong) *k);
    fmpz_pow_ui(ps, p, (ulong) s);
    /* F = (g(a) mod p^(2k)) / p^(k+s), in [0, p^(k-s)): P3.3(2), g(a) is needed modulo p^(2k) */
    fmpz_mul(q, pk, pk);
    eval_mod(F, g, a, q);
    fmpz_mul(q, pk, ps);
    fmpz_divexact(F, F, q);
    /* D = (g'(a) mod p^k) / p^s, in [0, p^(k-s)), prime to p */
    eval_mod(D, d, a, pk);
    fmpz_divexact(D, D, ps);
    /* u = D^(-1) modulo q = p^(k-s) */
    fmpz_divexact(q, pk, ps);
    if (!fmpz_invmod(u, D, q))
    {
        flint_printf("adelefeld: adf_root_padic_from_seed: internal error: no inverse in a Newton step\n");
        flint_abort();
    }
    /* a = (a - p^k (F u mod q)) mod p^(2k - s) */
    fmpz_mul(F, F, u);
    fmpz_mod(F, F, q);
    fmpz_submul(a, pk, F);
    fmpz_pow_ui(q, p, (ulong) k2);
    fmpz_mod(a, a, q);
    *k = k2;
    fmpz_clear(pk);
    fmpz_clear(ps);
    fmpz_clear(q);
    fmpz_clear(F);
    fmpz_clear(D);
    fmpz_clear(u);
    return 1;
}

/* The seed computation on g (normalised, not checked here) at the prime p (>= 2, not checked):
   statuses and outputs as adf_root_padic_from_seed (roots.h), with (a_out, *K, *s) written on ADF_OK
   only. a_out may alias a. solvers P3.12 (solvers.md:1487 to 1517) and seed_root,
   proto/solvers_checks.py:1721. */
int
adf_roots_seed_core(fmpz_t a_out, slong * K, slong * s, const fmpz_poly_t g, const fmpz_t p, const fmpz_t a,
                    slong prec_p)
{
    fmpz_poly_t d;
    fmpz_t ga, da, a1, q;
    slong sv, vg, k0, k, Kv, s1, s2;
    ulong bits = fmpz_bits(p);
    int st = ADF_OK;

    if (prec_p < 1)
        return ADF_DOMAIN;
    if (!prec_within_limit(prec_p, bits))       /* before any allocation (S-D18) */
        return ADF_LIMIT;
    fmpz_poly_init(d);
    fmpz_init(ga);
    fmpz_init(da);
    fmpz_init(a1);
    fmpz_init(q);
    fmpz_poly_derivative(d, g);
    fmpz_poly_evaluate_fmpz(ga, g, a);
    fmpz_poly_evaluate_fmpz(da, d, a);
    /* the strong form for g (P3.12, hypotheses; P3.12(4)): g'(a) != 0 and v(g(a)) > 2 s */
    if (fmpz_is_zero(da))
    {
        st = ADF_NOT_DETERMINED;
        goto done;
    }
    sv = val_p(da, p);
    vg = WORD_MAX;                              /* v(0) infinite */
    if (!fmpz_is_zero(ga))
    {
        vg = val_p(ga, p);
        if (!add_checked(&s2, sv, sv))
        {
            st = ADF_LIMIT;
            goto done;
        }
        if (vg <= s2)
        {
            st = ADF_NOT_DETERMINED;
            goto done;
        }
    }
    /* K = max(prec_p, s + 1) (P3.12(2)); the limit before any power of p */
    if (!add_checked(&s1, sv, 1))
    {
        st = ADF_LIMIT;
        goto done;
    }
    Kv = FLINT_MAX(prec_p, s1);
    if (!prec_within_limit(Kv, bits))
    {
        st = ADF_LIMIT;
        goto done;
    }
    /* k0 = v(g(a)) - s >= s + 1 (P3.12(2)); infinite when g(a) = 0 */
    k0 = fmpz_is_zero(ga) ? WORD_MAX : vg - sv;
    if (k0 >= Kv)
    {
        /* (a mod p^K, K, s) directly, p^k0 never formed (S-D18; Taylor argument of P3.12(2)) */
        fmpz_pow_ui(q, p, (ulong) Kv);
        fmpz_mod(a1, a, q);
    }
    else
    {
        /* (a mod p^k0, k0, s) is a certificate; lift by Newton steps until k >= K (P3.3(1)), k0 < K */
        k = k0;
        fmpz_pow_ui(q, p, (ulong) k);
        fmpz_mod(a1, a, q);
        while (k < Kv)
            if (!newton_step(a1, &k, g, d, p, sv))
            {
                st = ADF_LIMIT;
                goto done;
            }
        /* reduced modulo p^K (P3.2(3)) */
        fmpz_pow_ui(q, p, (ulong) Kv);
        fmpz_mod(a1, a1, q);
    }
    fmpz_swap(a_out, a1);
    *K = Kv;
    *s = sv;
done:
    fmpz_poly_clear(d);
    fmpz_clear(ga);
    fmpz_clear(da);
    fmpz_clear(a1);
    fmpz_clear(q);
    return st;
}

int
adf_root_padic_from_seed(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, const fmpz_t a, slong prec_p)
{
    fmpz_poly_t g;
    fmpz_t fp, ap;
    slong K = 0, s = 0;
    ulong pw;
    int reduced, st;

    /* DOMAIN first (S-D16: f = 0 is refused before g is formed), L untouched */
    if (fmpz_poly_is_zero(f) || prec_p < 1 || adf_place_is_archimedean(p))
        return ADF_DOMAIN;
    /* the limit of the precision before any allocation: an fmpz of a word may allocate */
    pw = adf_place_prime_get(p);
    if (!prec_within_limit(prec_p, FLINT_BIT_COUNT(pw)))
        return ADF_LIMIT;
    fmpz_poly_init(g);
    fmpz_init_set_ui(fp, pw);
    fmpz_init(ap);
    normalise(g, &reduced, f);
    st = adf_roots_seed_core(ap, &K, &s, g, fp, a, prec_p);
    if (st == ADF_OK)
    {
        /* L written only now: f and a may be fields of L */
        free_arrays(L);
        L->place = p;
        L->scope = ADF_ROOTLIST_SEED;
        L->reduced = reduced;
        L->complete = 0;
        fmpz_poly_swap(L->g, g);
        L->n = 1;
        L->a = _fmpz_vec_init(1);
        L->K = flint_malloc(sizeof(slong));
        L->s = flint_malloc(sizeof(slong));
        fmpz_swap(L->a + 0, ap);
        L->K[0] = K;
        L->s[0] = s;
        L->nu = 0;
        L->count = 0;
    }
    fmpz_poly_clear(g);
    fmpz_clear(fp);
    fmpz_clear(ap);
    return st;
}

/* ---- the entries verifier (solvers P3.13(1), solvers.md:1529 to 1532; padic_verify_entries,
   proto/solvers_checks.py:1754) ---- */

int
adf_rootlist_verify_entries(const adf_rootlist_t L, const fmpz_poly_t f)
{
    fmpz_poly_t h;
    fmpz_t p;
    slong i;
    int red, r;

    if (fmpz_poly_is_zero(f))
        return 0;
    if (adf_place_is_archimedean(L->place))
        return 0;                               /* TEMPORARY (roots.h): the real place in a later slice */
    if (!shape_ok_prime(L))
        return 0;
    fmpz_poly_init(h);
    normalise(h, &red, f);
    r = fmpz_poly_equal(h, L->g) && red == L->reduced;
    fmpz_poly_clear(h);
    if (!r)
        return 0;
    fmpz_init_set_ui(p, adf_place_prime_get(L->place));
    for (i = 0; i < L->n && r; i++)
        r = adf_roots_cert_check(L->g, p, L->a + i, L->K[i], L->s[i]);
    for (i = 0; i < L->nu && r; i++)
        r = L->ue[i] >= 0 && below_power(L->ua + i, p, L->ue[i]);
    r = r && disjoint(L, p);
    fmpz_clear(p);
    return r;
}

/* ---- accessors ---- */

slong
adf_rootlist_length(const adf_rootlist_t L)
{
    return L->n;
}

int
adf_rootlist_is_complete(const adf_rootlist_t L)
{
    return L->complete;
}

slong
adf_rootlist_unresolved_length(const adf_rootlist_t L)
{
    return L->nu;
}

adf_place_t
adf_rootlist_place(const adf_rootlist_t L)
{
    return L->place;
}

int
adf_rootlist_scope(const adf_rootlist_t L)
{
    return L->scope;
}

void
adf_rootlist_get_poly(fmpz_poly_t g, const adf_rootlist_t L)
{
    fmpz_poly_set(g, L->g);
}

int
adf_rootlist_get_cert(fmpz_t a, slong * K, slong * s, const adf_rootlist_t L, slong i)
{
    if (adf_place_is_archimedean(L->place) || i < 0 || i >= L->n || L->a == NULL)
        return 0;
    fmpz_set(a, L->a + i);
    *K = L->K[i];
    *s = L->s[i];
    return 1;
}

/* the finite ball a_i + p^(K_i) Zhat as the canonical global triple (a_i, p^(K_i), 1) (S-D14; SPEC 4.1:
   the precision at p is v_p(H/d) = K_i) */
int
adf_rootlist_get_fball(adf_fball_t x, const adf_rootlist_t L, slong i)
{
    fmpz_t H, one;
    ulong pw;
    int st;

    if (adf_place_is_archimedean(L->place) || i < 0 || i >= L->n || L->a == NULL)
        return 0;
    pw = adf_place_prime_get(L->place);
    if (pw < 2 || L->K[i] < 0 || (ulong) L->K[i] > (ulong) ADF_ROOTS_BITS_MAX / FLINT_BIT_COUNT(pw))
        return 0;
    fmpz_init(H);
    fmpz_init_set_ui(one, 1);
    fmpz_set_ui(H, pw);
    fmpz_pow_ui(H, H, (ulong) L->K[i]);
    st = adf_fball_set_fmpz3(x, L->a + i, H, one);
    fmpz_clear(H);
    fmpz_clear(one);
    return st == ADF_OK;
}

/* ==== slice 2: all roots at a prime (Algorithm P) ====

   Algorithm P and Proposition 3.5 of solvers.md (lines 1160 to 1236), with the level of Proposition 3.4
   (lines 1121 to 1154) and the roots modulo p by evaluation at every residue (Proposition 3.7(1), lines
   1288 to 1289); the reference is padic_roots, proto/solvers_checks.py:1649 to 1698. FLINT routines
   used besides those above (refs/src/flint-3.0.1):
     fmpz_poly_taylor_shift, fmpz_poly.rst:2496: "composing f by x + c";
     fmpz_poly_scalar_divexact_fmpz, fmpz_poly.rst:545: exact division of every coefficient;
     fmpz_poly_get_nmod_poly, fmpz_poly.rst:3142: the coefficients reduced by the modulus;
     nmod_poly_derivative, nmod_poly.rst:1212; nmod_poly_evaluate_nmod, nmod_poly.rst:1243 (Horner, the
       point reduced modulo the modulus).
   No routine of FLINT for roots or factorisation modulo p is called (S-D10, P3.7(1)).

   The classes are not formed as f(a + p^e Y) from f: the polynomial of a child is formed from that of its
   parent, g_(a1, e+1)(Y) = g_(a,e)(b + p Y) / p^v with a1 = a + p^e b and w(a1, e+1) = w(a, e) + v, which is
   the identity f(a1 + p^(e+1) Y) = p^w g_(a,e)(b + p Y) of the proof of P3.4(4) (solvers.md:1149). */

ADF_ROOTS_HIDDEN int adf_roots_padic_core(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec_p,
                                          slong depth, int strict, slong bits_max);
ADF_ROOTS_HIDDEN int adf_roots_certify(fmpz_t a_out, slong * K, slong * s, const fmpz_poly_t g,
                                       const fmpz_poly_t h, const fmpz_t p, const fmpz_t a, const fmpz_t pe,
                                       slong e, slong w, ulong b, slong prec_p, slong bits_max);

/* 1 if 0 <= K and 2 K bits <= max (bits >= 1): the limit of S-D18 with the bound max, by a division */
static int
exp_within(slong K, ulong bits, slong max)
{
    return K >= 0 && max >= 0 && (ulong) K <= (ulong) max / (2 * bits);
}

/* the largest v such that p^v divides every coefficient of h, for h != 0 */
static slong
content_val_p(const fmpz_poly_t h, const fmpz_t p)
{
    slong i, t, v = WORD_MAX;

    for (i = 0; i < fmpz_poly_length(h) && v > 0; i++)
        if (!fmpz_is_zero(h->coeffs + i))
        {
            t = val_p(h->coeffs + i, p);
            if (t < v)
                v = t;
        }
    return v;
}

/* The certification of a simple digit, Algorithm P step 3, first case (solvers.md:1169 to 1172), by
   P3.4(3) (solvers.md:1131 to 1134): at the open class (a, e) with w = w(a, e) and h = g_(a,e), b in
   [0, p) with h(b) = 0 and h'(b) != 0 modulo p. s = w - e; j = max(1, w - 2 e + 1), the least j >= 1
   with j > w - 2 e; beta_j by Newton steps of P3.3 on h from its certificate (b, 1, 0); the certificate
   ((a + p^e beta_j) mod p^(e+j), e + j, s) of g, lifted by Newton steps of P3.3 on g until
   k >= K = max(prec_p, s + 1), then reduced modulo p^K (P3.2(3)). g is the polynomial of Algorithm P
   after step 1, pe = p^e, 0 <= a < p^e. Every exponent is computed with checked arithmetic (S-D18) and
   K and k are tested against bits_max before any allocation and before any power of p is formed:
   ADF_LIMIT with a_out, K, s untouched. Under (W) (solvers.md:1203 to 1207) w >= 2 e and k = s + 1 <= K,
   so every power formed is below p^(2 K). a_out is written on ADF_OK only. */
int
adf_roots_certify(fmpz_t a_out, slong * K, slong * s, const fmpz_poly_t g, const fmpz_poly_t h, const fmpz_t p,
                  const fmpz_t a, const fmpz_t pe, slong e, slong w, ulong b, slong prec_p, slong bits_max)
{
    fmpz_poly_t dh, dg;
    fmpz_t beta, a1, q;
    slong sv, e2, t, j, jj, k, Kv, s1;
    ulong bits = fmpz_bits(p);
    int st = ADF_OK;

    /* s = w - e and j > w - 2 e (P3.4(3): "k > s is j > w - 2 e", solvers.md:1147) */
    if (!sub_checked(&sv, w, e) || !add_checked(&e2, e, e) || !sub_checked(&t, w, e2))
        return ADF_LIMIT;
    j = 1;
    if (!(j > t) && !add_checked(&j, t, 1))
        return ADF_LIMIT;
    if (!add_checked(&k, e, j) || !add_checked(&s1, sv, 1))
        return ADF_LIMIT;
    /* K = max(prec_p, s + 1), the least K >= prec_p with K > s (R1) */
    Kv = prec_p > sv ? prec_p : s1;
    if (!exp_within(Kv, bits, bits_max) || !exp_within(k, bits, bits_max))
        return ADF_LIMIT;
    if (sv < 0)
    {
        /* w >= e by P3.4(3): a defect of the tree, not an input */
        flint_printf("adelefeld: adf_roots_padic: internal error: w = %wd < e = %wd\n", w, e);
        flint_abort();
    }
    fmpz_poly_init(dh);
    fmpz_poly_init(dg);
    fmpz_init_set_ui(beta, b);
    fmpz_init(a1);
    fmpz_init(q);
    fmpz_poly_derivative(dh, h);
    fmpz_poly_derivative(dg, g);
    /* beta_j: (b, 1, 0) is a certificate for h, lifted by P3.3 and reduced modulo p^j (P3.2(3)) */
    jj = 1;
    while (jj < j && st == ADF_OK)
        if (!newton_step(beta, &jj, h, dh, p, 0))
            st = ADF_LIMIT;
    if (st == ADF_OK)
    {
        fmpz_pow_ui(q, p, (ulong) j);
        fmpz_mod(beta, beta, q);
        /* the certificate of P3.4(3) with k = e + j, reduced modulo p^k */
        fmpz_set(a1, a);
        fmpz_addmul(a1, pe, beta);
        fmpz_pow_ui(q, p, (ulong) k);
        fmpz_mod(a1, a1, q);
        /* lifted by P3.3 until k >= K, then reduced modulo p^K */
        while (k < Kv && st == ADF_OK)
            if (!newton_step(a1, &k, g, dg, p, sv))
                st = ADF_LIMIT;
    }
    if (st == ADF_OK)
    {
        fmpz_pow_ui(q, p, (ulong) Kv);
        fmpz_mod(a1, a1, q);
        fmpz_swap(a_out, a1);
        *K = Kv;
        *s = sv;
    }
    fmpz_poly_clear(dh);
    fmpz_poly_clear(dg);
    fmpz_clear(beta);
    fmpz_clear(a1);
    fmpz_clear(q);
    return st;
}

/* a growing list of (centre, exponent, s): the certificates, or the classes (s unused) */
typedef struct
{
    fmpz * a;
    slong * k;
    slong * s;
    slong len;
    slong alloc;
} item_vec;

static void
item_vec_init(item_vec * v)
{
    v->a = NULL;
    v->k = NULL;
    v->s = NULL;
    v->len = 0;
    v->alloc = 0;
}

static void
item_vec_clear(item_vec * v)
{
    if (v->a != NULL)
        _fmpz_vec_clear(v->a, v->alloc);
    flint_free(v->k);
    flint_free(v->s);
}

static void
item_vec_push(item_vec * v, const fmpz_t a, slong k, slong s)
{
    fmpz * b;
    slong i, na;

    if (v->len == v->alloc)
    {
        na = v->alloc == 0 ? 4 : 2 * v->alloc;
        b = _fmpz_vec_init(na);
        for (i = 0; i < v->len; i++)
            fmpz_swap(b + i, v->a + i);
        if (v->a != NULL)
            _fmpz_vec_clear(v->a, v->alloc);
        v->a = b;
        v->k = v->k == NULL ? flint_malloc(na * sizeof(slong)) : flint_realloc(v->k, na * sizeof(slong));
        v->s = v->s == NULL ? flint_malloc(na * sizeof(slong)) : flint_realloc(v->s, na * sizeof(slong));
        v->alloc = na;
    }
    fmpz_set(v->a + v->len, a);
    v->k[v->len] = k;
    v->s[v->len] = s;
    v->len++;
}

/* sorts by increasing centre (insertion sort: the lists have at most a few times deg g entries) */
static void
item_vec_sort(item_vec * v)
{
    slong i, j, t;

    for (i = 1; i < v->len; i++)
        for (j = i; j > 0 && fmpz_cmp(v->a + j - 1, v->a + j) > 0; j--)
        {
            fmpz_swap(v->a + j - 1, v->a + j);
            t = v->k[j - 1];
            v->k[j - 1] = v->k[j];
            v->k[j] = t;
            t = v->s[j - 1];
            v->s[j - 1] = v->s[j];
            v->s[j] = t;
        }
}

/* an open class (a, e) of Algorithm P: pe = p^e, w = w(a, e), h = g_(a,e) (P3.4) */
typedef struct
{
    fmpz_t a;
    fmpz_t pe;
    slong e;
    slong w;
    fmpz_poly_t h;
} open_class;

/* pushes the class (a, e) with pe, w and h (h is moved: swapped into the new entry) */
static void
push_class(open_class ** stk, slong * n, slong * alloc, const fmpz_t a, const fmpz_t pe, slong e, slong w,
           fmpz_poly_t h)
{
    open_class * c;

    if (*n == *alloc)
    {
        *alloc = *alloc == 0 ? 8 : 2 * *alloc;
        *stk = *stk == NULL ? flint_malloc(*alloc * sizeof(open_class))
                            : flint_realloc(*stk, *alloc * sizeof(open_class));
    }
    c = *stk + *n;
    fmpz_init_set(c->a, a);
    fmpz_init_set(c->pe, pe);
    c->e = e;
    c->w = w;
    fmpz_poly_init(c->h);
    fmpz_poly_swap(c->h, h);
    (*n)++;
}

static void
open_class_clear(open_class * c)
{
    fmpz_clear(c->a);
    fmpz_clear(c->pe);
    fmpz_poly_clear(c->h);
}

/* Algorithm P (solvers.md:1162 to 1176) on g0 != 0 at the prime pu <= ADF_ROOTS_P_EVAL_MAX with
   k_req = prec_p >= 1 and D = depth >= 0, the limit of S-D18 with the bound bits_max. On ADF_OK the
   arrays and lengths of T (n, a, K, s, nu, ua, ue; T without arrays on entry) hold the certificates and
   the unresolved classes, each sorted by the centre; the other fields of T are not written. On ADF_LIMIT
   T is not written. The certificates refer to g0 with its content at p removed (step 1), which is g0
   for the normalised polynomial (content 1). */
static int
padic_search(adf_rootlist_t T, const fmpz_poly_t g0, ulong pu, slong prec_p, slong depth, slong bits_max)
{
    fmpz_t p, c, q, x, ap, pe1;
    fmpz_poly_t g, hc;
    nmod_poly_t hm, dm;
    item_vec certs, classes;
    open_class * stk = NULL;
    open_class N;
    slong nst = 0, ast = 0, i, v, w0, e1, wc, K = 0, s = 0;
    ulong b, bits = FLINT_BIT_COUNT(pu);
    int st = ADF_OK;

    fmpz_init_set_ui(p, pu);
    fmpz_init(c);
    fmpz_init(q);
    fmpz_init(x);
    fmpz_init(ap);
    fmpz_init(pe1);
    fmpz_poly_init(g);
    fmpz_poly_init(hc);
    nmod_poly_init(hm, pu);
    nmod_poly_init(dm, pu);
    item_vec_init(&certs);
    item_vec_init(&classes);
    /* step 1: g = g0 / p^w0 */
    w0 = content_val_p(g0, p);
    fmpz_pow_ui(q, p, (ulong) w0);
    fmpz_poly_scalar_divexact_fmpz(g, g0, q);
    /* step 2: the class (0, 0) is open, with w(0, 0) = 0 and g_(0,0) = g */
    fmpz_poly_set(hc, g);
    fmpz_one(pe1);
    push_class(&stk, &nst, &ast, c, pe1, 0, 0, hc);
    /* step 3, for every open class until none is open (step 4) */
    while (nst > 0 && st == ADF_OK)
    {
        N = stk[--nst];                         /* N owns the entry now */
        fmpz_poly_get_nmod_poly(hm, N.h);
        nmod_poly_derivative(dm, hm);
        for (b = 0; b < pu && st == ADF_OK; b++)
        {
            if (nmod_poly_evaluate_nmod(hm, b) != 0)
                continue;
            if (nmod_poly_evaluate_nmod(dm, b) != 0)
            {
                /* a simple root of g_(a,e) modulo p: one root of g, certified (P3.4(3)) */
                st = adf_roots_certify(ap, &K, &s, g, N.h, p, N.a, N.pe, N.e, N.w, b, prec_p, bits_max);
                if (st == ADF_OK)
                    item_vec_push(&certs, ap, K, s);
                continue;
            }
            /* a multiple root modulo p: the child (a + p^e b, e + 1) (P3.4(4)) */
            if (!add_checked(&e1, N.e, 1) || !exp_within(e1, bits, bits_max))
            {
                st = ADF_LIMIT;
                break;
            }
            fmpz_set(c, N.a);
            fmpz_addmul_ui(c, N.pe, b);
            if (N.e >= depth)
            {
                /* e + 1 > D: the class is left unresolved (step 3, second case) */
                item_vec_push(&classes, c, e1, 0);
                continue;
            }
            /* g_(c, e+1)(Y) = g_(a,e)(b + p Y) / p^v and w(c, e + 1) = w + v (solvers.md:1149) */
            fmpz_set_ui(x, b);
            fmpz_poly_taylor_shift(hc, N.h, x);
            fmpz_one(q);
            for (i = 0; i < fmpz_poly_length(hc); i++)
            {
                fmpz_mul(hc->coeffs + i, hc->coeffs + i, q);
                fmpz_mul_ui(q, q, pu);
            }
            v = content_val_p(hc, p);
            if (!add_checked(&wc, N.w, v))
            {
                st = ADF_LIMIT;
                break;
            }
            fmpz_pow_ui(q, p, (ulong) v);
            fmpz_poly_scalar_divexact_fmpz(hc, hc, q);
            fmpz_mul_ui(pe1, N.pe, pu);
            push_class(&stk, &nst, &ast, c, pe1, e1, wc, hc);
        }
        open_class_clear(&N);
    }
    if (st == ADF_OK)
    {
        item_vec_sort(&certs);
        item_vec_sort(&classes);
        T->n = certs.len;
        if (T->n > 0)
        {
            T->a = _fmpz_vec_init(T->n);
            T->K = flint_malloc(T->n * sizeof(slong));
            T->s = flint_malloc(T->n * sizeof(slong));
        }
        for (i = 0; i < T->n; i++)
        {
            fmpz_swap(T->a + i, certs.a + i);
            T->K[i] = certs.k[i];
            T->s[i] = certs.s[i];
        }
        T->nu = classes.len;
        if (T->nu > 0)
        {
            T->ua = _fmpz_vec_init(T->nu);
            T->ue = flint_malloc(T->nu * sizeof(slong));
        }
        for (i = 0; i < T->nu; i++)
        {
            fmpz_swap(T->ua + i, classes.a + i);
            T->ue[i] = classes.k[i];
        }
    }
    for (i = 0; i < nst; i++)
        open_class_clear(stk + i);
    flint_free(stk);
    item_vec_clear(&certs);
    item_vec_clear(&classes);
    fmpz_clear(p);
    fmpz_clear(c);
    fmpz_clear(q);
    fmpz_clear(x);
    fmpz_clear(ap);
    fmpz_clear(pe1);
    fmpz_poly_clear(g);
    fmpz_poly_clear(hc);
    nmod_poly_clear(hm);
    nmod_poly_clear(dm);
    return st;
}

/* adf_roots_padic and adf_roots_padic_partial (roots.h; api-s.md 4; S-D15) with the limit of S-D18 as the
   parameter bits_max (ADF_ROOTS_BITS_MAX for the public functions). strict = 1: the strict function. */
int
adf_roots_padic_core(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec_p, slong depth, int strict,
                     slong bits_max)
{
    adf_rootlist_t T;
    adf_rootlist_struct tmp;
    ulong pu;
    int reduced, st;

    /* the statuses decided before any allocation, in the order of roots.h */
    if (fmpz_poly_is_zero(f) || adf_place_is_archimedean(p) || prec_p < 1 || depth < 0)
        return ADF_DOMAIN;
    pu = adf_place_prime_get(p);
    if (pu > ADF_ROOTS_P_EVAL_MAX)
        return ADF_UNSUPPORTED;                 /* TEMPORARY (S-D10) */
    if (!exp_within(prec_p, FLINT_BIT_COUNT(pu), bits_max))
        return ADF_LIMIT;
    adf_rootlist_init(T);
    normalise(T->g, &reduced, f);
    st = padic_search(T, T->g, pu, prec_p, depth, bits_max);
    if (st == ADF_OK && strict && T->nu > 0)
        st = ADF_NOT_DETERMINED;                /* CV-06: L untouched */
    if (st == ADF_OK)
    {
        T->place = p;
        T->scope = ADF_ROOTLIST_PARTITION;
        T->reduced = reduced;
        T->complete = T->nu == 0;               /* P3.5(3) */
        T->count = 0;
        /* L written only now (f may be L->g): the contents are exchanged, the old ones cleared */
        tmp = *L;
        *L = *T;
        *T = tmp;
    }
    adf_rootlist_clear(T);
    return st;
}

int
adf_roots_padic_partial(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec_p, slong depth)
{
    return adf_roots_padic_core(L, f, p, prec_p, depth, 0, ADF_ROOTS_BITS_MAX);
}

int
adf_roots_padic(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec_p, slong depth)
{
    return adf_roots_padic_core(L, f, p, prec_p, depth, 1, ADF_ROOTS_BITS_MAX);
}

/* ---- the complete verifier (solvers P3.13(2), (3), solvers.md:1533 to 1538; padic_verify_complete,
   proto/solvers_checks.py:1776) ---- */

int
adf_rootlist_verify_complete(const adf_rootlist_t L, const fmpz_poly_t f, slong depth)
{
    adf_rootlist_t T;
    fmpz_t p;
    slong i, j, m;
    ulong pu;
    int r;

    if (depth < 0 || fmpz_poly_is_zero(f))
        return 0;
    if (adf_place_is_archimedean(L->place))
        return 0;                               /* TEMPORARY (roots.h): the real place in a later slice */
    if (!adf_rootlist_verify_entries(L, f))
        return 0;
    if (L->scope != ADF_ROOTLIST_PARTITION || L->complete != 1 || L->nu != 0)
        return 0;
    pu = adf_place_prime_get(L->place);
    if (pu > ADF_ROOTS_P_EVAL_MAX)
        return 0;                               /* TEMPORARY (S-D10): no rerun above the bound */
    adf_rootlist_init(T);
    fmpz_init_set_ui(p, pu);
    /* the rerun of Algorithm P on g through depth; no class may be left */
    r = padic_search(T, L->g, pu, 1, depth, ADF_ROOTS_BITS_MAX) == ADF_OK && T->nu == 0;
    /* every root ball of the rerun meets exactly one listed ball, and every listed ball exactly one
       root ball of the rerun (P3.2(4)) */
    for (i = 0; i < T->n && r; i++)
    {
        for (j = 0, m = 0; j < L->n; j++)
            m += balls_meet(T->a + i, T->K[i], L->a + j, L->K[j], p);
        r = m == 1;
    }
    for (j = 0; j < L->n && r; j++)
    {
        for (i = 0, m = 0; i < T->n; i++)
            m += balls_meet(T->a + i, T->K[i], L->a + j, L->K[j], p);
        r = m == 1;
    }
    fmpz_clear(p);
    adf_rootlist_clear(T);
    return r;
}

int
adf_rootlist_get_unresolved(fmpz_t a, slong * e, const adf_rootlist_t L, slong i)
{
    if (adf_place_is_archimedean(L->place) || i < 0 || i >= L->nu || L->ua == NULL)
        return 0;
    fmpz_set(a, L->ua + i);
    *e = L->ue[i];
    return 1;
}
