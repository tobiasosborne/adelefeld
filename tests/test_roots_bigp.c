/* tests/test_roots_bigp.c: the roots modulo p for every prime of a place (milestone S, S.2, slice 4, lane
   s2-slice4; decision S-D10, docs/SPEC.md 15.3): adf_roots_padic and adf_roots_padic_partial above
   ADF_ROOTS_P_EVAL_MAX, and the two routes that find the roots modulo p.

   The statements tested are docs/proofs/solvers.md Proposition 3.7 (line 1284): (1) the roots by
   evaluation at every residue, (2) a list of distinct roots found by any method is complete exactly when
   its length is deg gcd(g, X^p - X); Algorithm P and Proposition 3.5 (line 1160) above the bound; the
   contract is include/adelefeld/roots.h.

   Two functions of src/roots.c with hidden visibility are declared below, as tests/test_roots_padic.c
   declares the others: the search of the roots modulo p with the route forced, and the check that a
   candidate list is the complete list of distinct roots. They are not part of the interface.

   The oracles are written here and do not call the library:
     - below the bound, the evaluation route (P3.7(1)) is the oracle of the gcd route; both are run and
       their lists compared; polynomials with every residue a root, no root, planted roots;
     - above the bound, products of linear factors X - r with integer roots r of distinct residues modulo p
       (and one case with two roots congruent modulo p), times a constant and now and then a quadratic
       X^2 - c with c a non-residue modulo p (Euler's criterion: c^((p-1)/2) = -1). The roots of such a
       product in Z_p are exactly the planted r: Z_p is an integral domain, so a root of the product is a
       root of one factor, and X^2 - c has no root modulo p, so none in Z_p. With the normalised
       polynomial g of the oracle (fmpq_poly), s = v_p(g'(r)) (not infinite: r is a simple root of g,
       solvers L3.1(2)) and K = max(prec, s + 1), the complete list is exactly the certificates
       (r mod p^K, K, s) in increasing order of the centre (roots.h, adf_roots_padic_partial: K and a[i]). */

#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <flint/fmpq_poly.h>
#include <flint/fmpz_vec.h>
#include <flint/nmod_poly.h>
#include <flint/ulong_extras.h>

#include <adelefeld.h>

#include "test_runner.h"

/* hidden in src/roots.c (not declared by a public header, not exported by the shared object) */
slong adf_roots_modp(ulong * roots, const nmod_poly_t h, int route);
int adf_roots_modp_check(const nmod_poly_t h, const ulong * cand, slong m);

#define ROUTE_EVAL 1
#define ROUTE_GCD 2

/* ---- helpers ---- */

static adf_place_t
place_of(ulong p)
{
    adf_place_t v = adf_place_inf();

    if (adf_place_prime(&v, p) != ADF_OK)
        abort();
    return v;
}

/* the largest prime <= n (n >= 2), by n_is_prime (proved for every word: src/place.c) */
static ulong
prime_at_most(ulong n)
{
    while (!n_is_prime(n))
        n--;
    return n;
}

/* the least prime >= n */
static ulong
prime_at_least(ulong n)
{
    while (!n_is_prime(n))
        n++;
    return n;
}

/* a non-residue modulo the odd prime p, by Euler's criterion */
static ulong
non_residue(ulong p, flint_rand_t st)
{
    ulong c, ninv = n_preinvert_limb(p);

    for (;;)
    {
        c = 1 + n_randint(st, p - 1);
        if (n_powmod2_ui_preinv(c, (p - 1) / 2, p, ninv) == p - 1)
            return c;
    }
}

static int
cmp_ulong(const void * x, const void * y)
{
    ulong a = *(const ulong *) x, b = *(const ulong *) y;

    return a < b ? -1 : a > b;
}

/* h = c prod (X - r_i) over F_p */
static void
nmod_from_roots(nmod_poly_t h, ulong c, const ulong * r, slong n)
{
    nmod_poly_t t;
    slong i;

    nmod_poly_init_mod(t, h->mod);
    nmod_poly_zero(h);
    nmod_poly_set_coeff_ui(h, 0, c);
    for (i = 0; i < n; i++)
    {
        nmod_poly_zero(t);
        nmod_poly_set_coeff_ui(t, 1, 1);
        nmod_poly_set_coeff_ui(t, 0, nmod_neg(r[i], h->mod));
        nmod_poly_mul(h, h, t);
    }
    nmod_poly_clear(t);
}

/* the distinct values of r[0..n), sorted, into out; returns their number */
static slong
distinct_sorted(ulong * out, const ulong * r, slong n)
{
    slong i, m = 0;

    memcpy(out, r, n * sizeof(ulong));
    qsort(out, n, sizeof(ulong), cmp_ulong);
    for (i = 0; i < n; i++)
        if (m == 0 || out[m - 1] != out[i])
            out[m++] = out[i];
    return m;
}

/* Both routes on h; 1 if they return the same list and the check accepts it. want (if not NULL) is the
   expected sorted list of nwant roots. */
static int
routes_agree(const nmod_poly_t h, const ulong * want, slong nwant, const char * what)
{
    slong cap = FLINT_MAX(nmod_poly_degree(h), 0) + 1, n1, n2, i;
    ulong * r1 = flint_malloc(cap * sizeof(ulong));
    ulong * r2 = flint_malloc(cap * sizeof(ulong));
    int ok;

    n1 = adf_roots_modp(r1, h, ROUTE_EVAL);
    n2 = adf_roots_modp(r2, h, ROUTE_GCD);
    ok = n1 == n2 && n1 >= 0 && n1 < cap;
    for (i = 0; ok && i < n1; i++)
        ok = r1[i] == r2[i] && (i == 0 || r1[i - 1] < r1[i]);
    if (ok && want != NULL)
    {
        ok = n1 == nwant;
        for (i = 0; ok && i < n1; i++)
            ok = r1[i] == want[i];
    }
    ok = ok && adf_roots_modp_check(h, r1, n1) == 1;
    if (!ok)
        ADF_CHECK_MSG(0, "%s: p = %lu, deg %ld: %ld roots by evaluation, %ld by the gcd route, %ld wanted", what,
                      (unsigned long) h->mod.n, (long) nmod_poly_degree(h), (long) n1, (long) n2, (long) nwant);
    flint_free(r1);
    flint_free(r2);
    return ok;
}

/* ---- 1. below the bound: the two routes give identical lists ---- */

static int
agree_at(ulong p, flint_rand_t st, int with_all_residues)
{
    nmod_poly_t h, q;
    fmpz_poly_t F, H;
    ulong r[160], want[160], c;
    slong i, j, n, deg, bad = 0, cases = 0;

    nmod_poly_init(h, p);
    nmod_poly_init(q, p);
    fmpz_poly_init(F);
    fmpz_poly_init(H);
    /* random polynomials of degree 0 to 12 (the leading coefficient not 0 modulo p) */
    for (i = 0; i < 40; i++)
    {
        deg = n_randint(st, 13);
        nmod_poly_zero(h);
        for (j = 0; j < deg; j++)
            nmod_poly_set_coeff_ui(h, j, n_randint(st, p));
        nmod_poly_set_coeff_ui(h, deg, 1 + n_randint(st, p - 1));
        bad += !routes_agree(h, NULL, 0, "random");
        cases++;
    }
    /* planted roots (with repetitions), times a constant: the roots are exactly the planted residues */
    for (i = 0; i < 40; i++)
    {
        n = 1 + n_randint(st, 9);
        for (j = 0; j < n; j++)
            r[j] = n_randint(st, p);
        c = 1 + n_randint(st, p - 1);
        nmod_from_roots(h, c, r, n);
        bad += !routes_agree(h, want, distinct_sorted(want, r, n), "planted");
        cases++;
    }
    /* no root: products of quadratics X^2 - c with c a non-residue (p odd), X^2 + X + 1 at p = 2 */
    for (i = 0; i < 10; i++)
    {
        nmod_poly_one(h);
        for (j = 0; j <= (slong) n_randint(st, 3); j++)
        {
            nmod_poly_zero(q);
            nmod_poly_set_coeff_ui(q, 2, 1);
            if (p == 2)
            {
                nmod_poly_set_coeff_ui(q, 1, 1);
                nmod_poly_set_coeff_ui(q, 0, 1);
            }
            else
                nmod_poly_set_coeff_ui(q, 0, p - non_residue(p, st));
            nmod_poly_mul(h, h, q);
        }
        bad += !routes_agree(h, want, 0, "no root");
        cases++;
    }
    /* degree 0 (a nonzero constant: no root) and degree 1 (a X + b: the root -b/a) */
    for (i = 0; i < 10; i++)
    {
        nmod_poly_zero(h);
        nmod_poly_set_coeff_ui(h, 0, 1 + n_randint(st, p - 1));
        bad += !routes_agree(h, want, 0, "degree 0");
        c = n_randint(st, p);                   /* h = u (X + c), u a unit: the root -c */
        nmod_poly_set_coeff_ui(h, 1, 1 + n_randint(st, p - 1));
        nmod_poly_set_coeff_ui(h, 0, nmod_mul(c, h->coeffs[1], h->mod));
        want[0] = nmod_neg(c, h->mod);
        bad += !routes_agree(h, want, 1, "degree 1");
        cases += 2;
    }
    /* degree above 100: random, and 120 planted roots (distinct when p > 120) times a random cubic */
    for (i = 0; i < 3; i++)
    {
        deg = 101 + n_randint(st, 50);
        nmod_poly_zero(h);
        for (j = 0; j < deg; j++)
            nmod_poly_set_coeff_ui(h, j, n_randint(st, p));
        nmod_poly_set_coeff_ui(h, deg, 1 + n_randint(st, p - 1));
        bad += !routes_agree(h, NULL, 0, "random, degree > 100");
        for (j = 0; j < 120; j++)
            r[j] = n_randint(st, p);
        nmod_from_roots(h, 1 + n_randint(st, p - 1), r, 120);
        n = distinct_sorted(want, r, 120);
        bad += !routes_agree(h, want, n, "planted, degree 120");
        nmod_poly_zero(q);
        for (j = 0; j < 3; j++)
            nmod_poly_set_coeff_ui(q, j, n_randint(st, p));
        nmod_poly_set_coeff_ui(q, 3, 1);
        nmod_poly_mul(h, h, q);
        bad += !routes_agree(h, NULL, 0, "planted times a cubic, degree 123");
        cases += 3;
    }
    /* every residue a root: c (X^p - X) + p H for an integer polynomial H, reduced modulo p */
    if (with_all_residues)
        for (i = 0; i < 2; i++)
        {
            c = 1 + n_randint(st, p - 1);
            fmpz_poly_zero(F);
            fmpz_poly_set_coeff_ui(F, p, c);
            fmpz_poly_set_coeff_si(F, 1, -(slong) c);
            fmpz_poly_zero(H);
            for (j = 0; j < 5; j++)
                fmpz_poly_set_coeff_si(H, n_randint(st, p + 3), (slong) n_randint(st, 1000) - 500);
            fmpz_poly_scalar_mul_ui(H, H, p);
            fmpz_poly_add(F, F, H);
            fmpz_poly_get_nmod_poly(h, F);
            ADF_CHECK(nmod_poly_degree(h) == (slong) p);
            {
                ulong * all = flint_malloc(p * sizeof(ulong));
                for (j = 0; j < (slong) p; j++)
                    all[j] = j;
                bad += !routes_agree(h, all, p, "every residue a root");
                flint_free(all);
            }
            cases++;
        }
    nmod_poly_clear(h);
    nmod_poly_clear(q);
    fmpz_poly_clear(F);
    fmpz_poly_clear(H);
    ADF_CHECK_MSG(bad == 0, "p = %lu: %ld of %ld cases disagree", (unsigned long) p, (long) bad, (long) cases);
    return bad == 0;
}

ADF_TEST(both_routes_give_the_same_roots_below_the_bound)
{
    static const ulong small[] = { 2, 3, 5, 7, 11, 13, 101, 257 };
    flint_rand_t st;
    ulong p;
    size_t i;
    int k;

    flint_randinit(st);
    for (i = 0; i < sizeof(small) / sizeof(small[0]); i++)
        agree_at(small[i], st, 1);
    /* the three largest primes <= ADF_ROOTS_P_EVAL_MAX; every residue a root at the largest one only (the
       evaluation of a polynomial of degree p at p residues costs p^2) */
    p = ADF_ROOTS_P_EVAL_MAX;
    for (k = 0; k < 3; k++)
    {
        p = prime_at_most(p);
        agree_at(p, st, k == 0);
        p--;
    }
    flint_randclear(st);
}

/* ---- 2. above the bound: planted roots, the complete list, both verifiers ---- */

/* the normalised polynomial by fmpq_poly (as tests/test_roots_padic.c, oracle_normalise) */
static void
oracle_normalise(fmpz_poly_t g, const fmpz_poly_t f)
{
    fmpq_poly_t F, D, G, Q;

    fmpq_poly_init(F);
    fmpq_poly_init(D);
    fmpq_poly_init(G);
    fmpq_poly_init(Q);
    fmpq_poly_set_fmpz_poly(F, f);
    fmpq_poly_derivative(D, F);
    fmpq_poly_gcd(G, F, D);
    fmpq_poly_div(Q, F, G);
    fmpq_poly_get_numerator(g, Q);
    fmpz_poly_primitive_part(g, g);
    if (fmpz_poly_degree(f) <= 0)
        fmpz_poly_set_ui(g, 1);
    fmpq_poly_clear(F);
    fmpq_poly_clear(D);
    fmpq_poly_clear(G);
    fmpq_poly_clear(Q);
}

static slong
vp(const fmpz_t x, const fmpz_t p)
{
    fmpz_t t;
    slong v = 0;

    fmpz_init_set(t, x);
    while (fmpz_divisible(t, p))
    {
        fmpz_divexact(t, t, p);
        v++;
    }
    fmpz_clear(t);
    return v;
}

/* the planted case: f = lead prod (X - r_i) (times X^2 - c if c != 0), the distinct roots r_0..r_(n-1).
   Runs both functions at prec and depth and compares with the oracle list. Returns 1 if all agree. */
static int
planted_case(ulong pw, const fmpz * r, slong n, slong nrep, slong lead, ulong c, slong prec, slong depth)
{
    fmpz_poly_t f, t, g, dg;
    fmpz_t p, v, q;
    fmpz * a;
    slong * K, * s;
    slong i, j, m;
    adf_rootlist_t L, M;
    int ok = 1, st, stp;

    fmpz_poly_init(f);
    fmpz_poly_init(t);
    fmpz_poly_init(g);
    fmpz_poly_init(dg);
    fmpz_init_set_ui(p, pw);
    fmpz_init(v);
    fmpz_init(q);
    adf_rootlist_init(L);
    adf_rootlist_init(M);
    a = _fmpz_vec_init(n);
    K = flint_malloc(n * sizeof(slong));
    s = flint_malloc(n * sizeof(slong));
    fmpz_poly_set_si(f, lead);
    for (i = 0; i < n + nrep; i++)             /* the first nrep roots twice */
    {
        fmpz_poly_zero(t);
        fmpz_poly_set_coeff_si(t, 1, 1);
        fmpz_neg(v, r + (i < n ? i : i - n));
        fmpz_poly_set_coeff_fmpz(t, 0, v);
        fmpz_poly_mul(f, f, t);
    }
    if (c != 0)
    {
        fmpz_poly_zero(t);
        fmpz_poly_set_coeff_si(t, 2, 1);
        fmpz_set_ui(v, c);
        fmpz_neg(v, v);
        fmpz_poly_set_coeff_fmpz(t, 0, v);
        fmpz_poly_mul(f, f, t);
    }
    /* the oracle list: (r mod p^K, K, s), s = v_p(g'(r)), K = max(prec, s + 1), sorted by the centre */
    oracle_normalise(g, f);
    fmpz_poly_derivative(dg, g);
    for (i = 0; i < n; i++)
    {
        fmpz_poly_evaluate_fmpz(v, dg, r + i);
        if (fmpz_is_zero(v))
            abort();                            /* not a simple root of g: a defect of this test */
        s[i] = vp(v, p);
        K[i] = FLINT_MAX(prec, s[i] + 1);
        fmpz_pow_ui(q, p, (ulong) K[i]);
        fmpz_mod(a + i, r + i, q);
    }
    for (i = 1; i < n; i++)                     /* insertion sort by the centre */
        for (j = i; j > 0 && fmpz_cmp(a + j - 1, a + j) > 0; j--)
        {
            slong tk = K[j - 1], ts = s[j - 1];
            fmpz_swap(a + j - 1, a + j);
            K[j - 1] = K[j];
            K[j] = tk;
            s[j - 1] = s[j];
            s[j] = ts;
        }
    st = adf_roots_padic(L, f, place_of(pw), prec, depth);
    stp = adf_roots_padic_partial(M, f, place_of(pw), prec, depth);
    ok = st == ADF_OK && stp == ADF_OK && L->n == n && L->nu == 0 && L->complete == 1 &&
         L->scope == ADF_ROOTLIST_PARTITION && fmpz_poly_equal(L->g, g) && M->n == n && M->nu == 0;
    for (i = 0; ok && i < n; i++)
        ok = fmpz_equal(L->a + i, a + i) && L->K[i] == K[i] && L->s[i] == s[i] && fmpz_equal(M->a + i, a + i) &&
             M->K[i] == K[i] && M->s[i] == s[i];
    /* every planted root lies in exactly one ball (the oracle argument, stated as a count) */
    for (i = 0; ok && i < n; i++)
    {
        for (j = 0, m = 0; j < L->n; j++)
        {
            fmpz_pow_ui(q, p, (ulong) L->K[j]);
            fmpz_sub(v, r + i, L->a + j);
            m += fmpz_divisible(v, q);
        }
        ok = m == 1;
    }
    ok = ok && adf_rootlist_is_canonical(L) == 1 && adf_rootlist_verify_entries(L, f) == 1 &&
         adf_rootlist_verify_complete(L, f, depth) == 1;
    if (!ok)
        ADF_CHECK_MSG(0, "p = %lu, %ld roots, prec %ld, depth %ld: statuses %d, %d; n = %ld, nu = %ld; "
                      "canonical %d, entries %d, complete %d", (unsigned long) pw, (long) n, (long) prec,
                      (long) depth, st, stp, (long) L->n, (long) L->nu, adf_rootlist_is_canonical(L),
                      adf_rootlist_verify_entries(L, f), adf_rootlist_verify_complete(L, f, depth));
    adf_rootlist_clear(L);
    adf_rootlist_clear(M);
    _fmpz_vec_clear(a, n);
    flint_free(K);
    flint_free(s);
    fmpz_poly_clear(f);
    fmpz_poly_clear(t);
    fmpz_poly_clear(g);
    fmpz_poly_clear(dg);
    fmpz_clear(p);
    fmpz_clear(v);
    fmpz_clear(q);
    return ok;
}

/* n random integer roots of up to 100 bits (any sign) with distinct residues modulo p */
static void
draw_roots(fmpz * r, slong n, ulong pw, flint_rand_t st)
{
    fmpz_t p, x, y;
    slong i, j;
    int fresh;

    fmpz_init_set_ui(p, pw);
    fmpz_init(x);
    fmpz_init(y);
    for (i = 0; i < n; i++)
        do
        {
            fmpz_randbits(r + i, st, 1 + n_randint(st, 100));
            fmpz_mod(x, r + i, p);
            fresh = 1;
            for (j = 0; j < i && fresh; j++)
            {
                fmpz_mod(y, r + j, p);
                fresh = !fmpz_equal(x, y);
            }
        }
        while (!fresh);
    fmpz_clear(p);
    fmpz_clear(x);
    fmpz_clear(y);
}

ADF_TEST(planted_roots_above_the_bound_give_exactly_the_planted_list)
{
    ulong primes[5];
    static const slong precs[3] = { 1, 2, 7 };
    flint_rand_t st;
    fmpz r[41];
    fmpz_t pz;
    slong i, j, n, bad = 0, cases = 0;
    int k;

    primes[0] = prime_at_least(UWORD(1) << 20);          /* 21 bits: 1048583 */
    primes[1] = prime_at_most(UWORD(0xFFFFFFFF));         /* 32 bits */
    primes[2] = prime_at_most((UWORD(1) << 48) - 1);      /* 48 bits */
    primes[3] = prime_at_most((UWORD(1) << 63) - 1);      /* 63 bits */
    primes[4] = UWORD(18446744073709551557);               /* 2^64 - 59, 64 bits */
    ADF_CHECK(FLINT_BIT_COUNT(primes[0]) == 21 && FLINT_BIT_COUNT(primes[1]) == 32 &&
              FLINT_BIT_COUNT(primes[2]) == 48 && FLINT_BIT_COUNT(primes[3]) == 63 &&
              FLINT_BIT_COUNT(primes[4]) == 64 && primes[0] > ADF_ROOTS_P_EVAL_MAX);
    flint_randinit(st);
    fmpz_init(pz);
    for (i = 0; i < 41; i++)
        fmpz_init(r + i);
    for (k = 0; k < 5; k++)
    {
        fmpz_set_ui(pz, primes[k]);
        for (i = 0; i < 12; i++)
        {
            n = 1 + n_randint(st, 8);
            draw_roots(r, n, primes[k], st);
            bad += !planted_case(primes[k], r, n, i % 3 == 0 ? 1 : 0, i % 4 == 1 ? 3 : 1,
                                 i % 2 ? non_residue(primes[k], st) : 0, precs[i % 3], 2);
            cases++;
        }
        /* 40 roots, times a quadratic without root */
        draw_roots(r, 40, primes[k], st);
        bad += !planted_case(primes[k], r, 40, 0, 1, non_residue(primes[k], st), 3, 1);
        cases++;
        /* two roots congruent modulo p^t, t = 1, 2 (r1 = r0 + p^t u): a class is opened at the levels 1 to t,
           each with a digit that is a double root modulo p (the gcd route there too); s = t for both roots
           (g'(r0) = (r0 - r1) (r0 - r2)... with the other factor a unit), depth t + 1 suffices (P3.5(4)) */
        for (j = 1; j <= 2; j++)
        {
            draw_roots(r, 3, primes[k], st);
            fmpz_pow_ui(r + 1, pz, (ulong) j);
            fmpz_mul_ui(r + 1, r + 1, 1 + n_randint(st, 1000));
            fmpz_add(r + 1, r + 1, r + 0);
            bad += !planted_case(primes[k], r, 3, 0, 1, 0, 1 + j, j + 2);
            cases++;
        }
        /* the root 0 (twice in f) and the root -1 - p (the residue p - 1): the residues at the two ends */
        fmpz_zero(r + 0);
        fmpz_set_si(r + 1, -1);
        fmpz_sub_ui(r + 1, r + 1, primes[k]);
        bad += !planted_case(primes[k], r, 2, 1, 1, 0, 4, 0);
        cases++;
    }
    ADF_CHECK_MSG(bad == 0, "%ld of %ld planted cases disagree", (long) bad, (long) cases);
    for (i = 0; i < 41; i++)
        fmpz_clear(r + i);
    fmpz_clear(pz);
    flint_randclear(st);
}

/* No UNSUPPORTED: the statuses at primes above the old bound 2^20, with DOMAIN and LIMIT as in roots.h. */
ADF_TEST(statuses_above_the_bound)
{
    static const ulong ps[3] = { UWORD(1048583), UWORD(4294967291), UWORD(18446744073709551557) };
    fmpz_poly_t f, z;
    adf_rootlist_t L;
    int i;

    fmpz_poly_init(f);
    fmpz_poly_init(z);
    adf_rootlist_init(L);
    fmpz_poly_set_coeff_si(f, 0, -2);
    fmpz_poly_set_coeff_si(f, 2, 1);
    for (i = 0; i < 3; i++)
    {
        ADF_CHECK(adf_roots_padic(L, z, place_of(ps[i]), 5, 3) == ADF_DOMAIN);
        ADF_CHECK(adf_roots_padic(L, f, place_of(ps[i]), 5, -1) == ADF_DOMAIN);
        ADF_CHECK(adf_roots_padic(L, f, place_of(ps[i]), WORD_MAX, 3) == ADF_LIMIT);
        ADF_CHECK(adf_roots_padic_partial(L, f, place_of(ps[i]), WORD_MAX, 3) == ADF_LIMIT);
        ADF_CHECK(adf_roots_padic(L, f, place_of(ps[i]), 5, 3) == ADF_OK && L->complete == 1 && L->nu == 0);
        ADF_CHECK(adf_rootlist_verify_complete(L, f, 3) == 1);
        ADF_CHECK(adf_roots_padic_partial(L, f, place_of(ps[i]), 5, 0) == ADF_OK && L->complete == 1);
    }
    /* the limit of the precision at 64 bits: 2 prec 64 > 2^24 exactly when prec > 2^17 */
    ADF_CHECK(adf_roots_padic(L, f, place_of(ps[2]), 131072, 0) == ADF_OK);
    ADF_CHECK(adf_roots_padic(L, f, place_of(ps[2]), 131073, 0) == ADF_LIMIT);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_poly_clear(z);
}

/* ---- 3. a wrong candidate list is refused ---- */

static void
forged_lists(const nmod_poly_t h, const ulong * roots, slong n, ulong nonroot, const char * what)
{
    ulong c[200];
    slong i;

    if (n + 1 > 200)
        abort();
    ADF_CHECK_MSG(adf_roots_modp_check(h, roots, n) == 1, "%s: the true list refused", what);
    /* the true list in reverse order is accepted: the check does not depend on the order */
    for (i = 0; i < n; i++)
        c[i] = roots[n - 1 - i];
    ADF_CHECK_MSG(adf_roots_modp_check(h, c, n) == 1, "%s: the reversed list refused", what);
    /* a non-root added */
    memcpy(c, roots, n * sizeof(ulong));
    c[n] = nonroot;
    ADF_CHECK_MSG(adf_roots_modp_check(h, c, n + 1) == 0, "%s: a non-root added, accepted", what);
    if (n == 0)
        return;
    /* a root removed */
    ADF_CHECK_MSG(adf_roots_modp_check(h, roots + 1, n - 1) == 0, "%s: a root removed, accepted", what);
    /* a root removed and a non-root added: the length is right */
    memcpy(c, roots, n * sizeof(ulong));
    c[0] = nonroot;
    ADF_CHECK_MSG(adf_roots_modp_check(h, c, n) == 0, "%s: a root replaced by a non-root, accepted", what);
    /* a root repeated in place of another: the length is right, every entry is a root */
    if (n >= 2)
    {
        memcpy(c, roots, n * sizeof(ulong));
        c[0] = c[1];
        ADF_CHECK_MSG(adf_roots_modp_check(h, c, n) == 0, "%s: a repeated root, accepted", what);
    }
    /* a root r replaced by r + p (the same residue, but not in [0, p)) */
    if (roots[0] <= UWORD_MAX - h->mod.n)
    {
        memcpy(c, roots, n * sizeof(ulong));
        c[0] += h->mod.n;
        ADF_CHECK_MSG(adf_roots_modp_check(h, c, n) == 0, "%s: a root plus p, accepted", what);
    }
}

ADF_TEST(a_wrong_candidate_list_is_refused)
{
    static const ulong ps[6] = { 3, 101, 65521, UWORD(1048583), UWORD(4294967291), UWORD(18446744073709551557) };
    flint_rand_t st;
    nmod_poly_t h, q;
    ulong r[20], want[20], nonroot;
    slong n, j;
    int i, k;

    flint_randinit(st);
    for (k = 0; k < 6; k++)
    {
        ulong p = ps[k];
        nmod_poly_init(h, p);
        nmod_poly_init(q, p);
        for (i = 0; i < 20; i++)
        {
            /* planted roots times X^2 - c without root: the roots are exactly the planted ones */
            n = n_randint(st, 9);
            for (j = 0; j < n; j++)
                r[j] = n_randint(st, p);
            nmod_from_roots(h, 1 + n_randint(st, p - 1), r, n);
            nmod_poly_zero(q);
            nmod_poly_set_coeff_ui(q, 2, 1);
            nmod_poly_set_coeff_ui(q, 0, p - non_residue(p, st));
            nmod_poly_mul(h, h, q);
            n = distinct_sorted(want, r, n);
            /* a non-root: a residue at which h does not vanish (h has at most deg h roots) */
            do
                nonroot = n_randint(st, p);
            while (nmod_poly_evaluate_nmod(h, nonroot) == 0);
            forged_lists(h, want, n, nonroot, "planted");
        }
        /* degree 0 and the empty list; the empty list of a polynomial with a root */
        nmod_poly_zero(h);
        nmod_poly_set_coeff_ui(h, 0, 1);
        ADF_CHECK(adf_roots_modp_check(h, NULL, 0) == 1);
        ADF_CHECK(adf_roots_modp_check(h, r, 1) == 0);
        nmod_poly_set_coeff_ui(h, 1, 1);        /* X + 1: the root p - 1 */
        ADF_CHECK(adf_roots_modp_check(h, NULL, 0) == 0);
        nmod_poly_clear(h);
        nmod_poly_clear(q);
    }
    flint_randclear(st);
}

/* ---- 4. the zero polynomial modulo p cannot occur (solvers L3.1(1)): the search aborts on it ---- */

ADF_TEST(the_zero_polynomial_modulo_p_aborts)
{
    static const ulong ps[2] = { 7, UWORD(4294967291) };
    int k, route, wst;
    pid_t pid;

    for (k = 0; k < 2; k++)
        for (route = 0; route <= 2; route++)
        {
            fflush(NULL);
            pid = fork();
            if (pid == 0)
            {
                nmod_poly_t h;
                ulong r[1];
                FILE * null = freopen("/dev/null", "w", stderr);
                (void) null;
                nmod_poly_init(h, ps[k]);
                adf_roots_modp(r, h, route);
                _exit(0);                        /* not reached if the function aborts */
            }
            ADF_CHECK(pid > 0);
            if (pid <= 0)
                continue;
            ADF_CHECK(waitpid(pid, &wst, 0) == pid);
            ADF_CHECK_MSG(WIFSIGNALED(wst) && WTERMSIG(wst) == SIGABRT, "p = %lu, route %d: no abort",
                          (unsigned long) ps[k], route);
        }
}
