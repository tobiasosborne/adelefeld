/* tests/test_roots_seed.c: the type adf_rootlist, the seed function adf_root_padic_from_seed and the
   entries verifier at a prime (milestone S, S.2, slice 1, lane s2-slice1).

   The statements tested are docs/proofs/solvers.md Lemma 3.1 (line 1013), Definition and Proposition
   3.2 (line 1044), Proposition 3.3 (line 1090), 3.11 (line 1449), Proposition 3.12 (line 1487) and
   Proposition 3.13(1) (line 1524); the contract is include/adelefeld/roots.h. The oracles are written
   here and do not call the library: the normalised polynomial is computed over Q with fmpq_poly (the
   library uses fmpz_poly_gcd), the strong form by exact evaluation, and the roots in a ball by an
   enumeration modulo p^M level by level (the oracle approx_roots of proto/solvers_checks.py:1791).
   The vectors tests/ref/vectors/s2-slice1/seed.jsonl come from seed_root of the reference
   (lanes/s2-slice1/gen_vectors.py).

   Two functions of src/roots.c with hidden visibility are declared below: the seed computation and
   the certificate check on a prime given as an fmpz. They are not part of the interface; they are
   called here for primes of more than one word, which an adf_place_t cannot hold (roots.h, "The
   prime"). */

#include <stddef.h>
#include <string.h>

#include <flint/fmpq_poly.h>
#include <flint/fmpz_vec.h>

#include <adelefeld.h>

#include "support/jsonl.h"

#include "test_runner.h"

/* hidden in src/roots.c (not declared by a public header, not exported by the shared object) */
int adf_roots_seed_core(fmpz_t a_out, slong * K, slong * s, const fmpz_poly_t g, const fmpz_t p,
                        const fmpz_t a, slong prec_p);
int adf_roots_cert_check(const fmpz_poly_t g, const fmpz_t p, const fmpz_t a, slong K, slong s);

/* ---- helpers ---- */

static void
poly_set_si(fmpz_poly_t f, const slong * c, slong len)
{
    slong i;

    fmpz_poly_zero(f);
    for (i = 0; i < len; i++)
        fmpz_poly_set_coeff_si(f, i, c[i]);
}

/* f = lead * prod (X - r_i) */
static void
poly_from_roots(fmpz_poly_t f, slong lead, const slong * r, slong n)
{
    fmpz_poly_t t;
    slong i;

    fmpz_poly_init(t);
    fmpz_poly_set_si(f, lead);
    for (i = 0; i < n; i++)
    {
        fmpz_poly_zero(t);
        fmpz_poly_set_coeff_si(t, 1, 1);
        fmpz_poly_set_coeff_si(t, 0, -r[i]);
        fmpz_poly_mul(f, f, t);
    }
    fmpz_poly_clear(t);
}

/* The oracle of the normalised polynomial (solvers L3.1(3)): f / gcd(f, f') over Q (fmpq_poly_gcd is
   monic), scaled to a primitive integer polynomial with positive leading coefficient; 1 for f
   constant. Independent of the library, which works with fmpz_poly_gcd. */
static void
oracle_normalise(fmpz_poly_t g, int * reduced, const fmpz_poly_t f)
{
    fmpq_poly_t F, D, G, Q;
    fmpz_t den;

    fmpq_poly_init(F);
    fmpq_poly_init(D);
    fmpq_poly_init(G);
    fmpq_poly_init(Q);
    fmpz_init(den);
    fmpq_poly_set_fmpz_poly(F, f);
    fmpq_poly_derivative(D, F);
    fmpq_poly_gcd(G, F, D);
    fmpq_poly_div(Q, F, G);
    fmpq_poly_get_numerator(g, Q);
    fmpz_poly_primitive_part(g, g);
    *reduced = fmpz_poly_degree(f) > 0 && fmpq_poly_degree(G) > 0;
    fmpq_poly_clear(F);
    fmpq_poly_clear(D);
    fmpq_poly_clear(G);
    fmpq_poly_clear(Q);
    fmpz_clear(den);
}

/* v_p(x) of x != 0, by repeated division */
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

/* the strong form for g at a (solvers P3.12): g'(a) != 0 and (g(a) = 0 or v(g(a)) > 2 v(g'(a)));
   *s = v(g'(a)) when g'(a) != 0 */
static int
oracle_strong(slong * s, const fmpz_poly_t g, const fmpz_t p, const fmpz_t a)
{
    fmpz_poly_t d;
    fmpz_t ga, da;
    int ok;

    fmpz_poly_init(d);
    fmpz_init(ga);
    fmpz_init(da);
    fmpz_poly_derivative(d, g);
    fmpz_poly_evaluate_fmpz(ga, g, a);
    fmpz_poly_evaluate_fmpz(da, d, a);
    *s = -1;
    if (fmpz_is_zero(da))
        ok = 0;
    else
    {
        *s = vp(da, p);
        ok = fmpz_is_zero(ga) || vp(ga, p) > 2 * *s;
    }
    fmpz_poly_clear(d);
    fmpz_clear(ga);
    fmpz_clear(da);
    return ok;
}

/* r = g(x) mod M, by Horner */
static void
eval_mod(fmpz_t r, const fmpz_poly_t g, const fmpz_t x, const fmpz_t M)
{
    slong i;

    fmpz_zero(r);
    for (i = fmpz_poly_length(g) - 1; i >= 0; i--)
    {
        fmpz_mul(r, r, x);
        fmpz_add(r, r, g->coeffs + i);
        fmpz_mod(r, r, M);
    }
}

/* The enumeration of Proposition 3.2 (check_s2_certificate, proto/solvers_checks.py:1847): the number
   of x modulo p^M with x = c modulo p^e and g(x) = 0 modulo p^M, level by level (a root modulo
   p^(i+1) reduces to a root modulo p^i). *same is 1 if every one of them is = ap modulo p^K. Returns
   -1 if a level has more than cap elements. */
static slong
oracle_enum(int * same, const fmpz_poly_t g, ulong p, const fmpz_t c, slong e, slong M, const fmpz_t ap,
            slong K, slong cap)
{
    fmpz * cur, * nxt, * tmp;
    fmpz_t q, pi, v, y, pK, fp;
    slong ncur, nnxt, i, j;
    ulong t;

    fmpz_init(q);
    fmpz_init(pi);
    fmpz_init(v);
    fmpz_init(y);
    fmpz_init(pK);
    fmpz_init_set_ui(fp, p);
    cur = _fmpz_vec_init(cap);
    nxt = _fmpz_vec_init(cap);
    fmpz_pow_ui(q, fp, e);
    fmpz_mod(cur + 0, c, q);
    eval_mod(v, g, cur + 0, q);
    ncur = fmpz_is_zero(v) ? 1 : 0;
    for (i = e; i < M && ncur > 0; i++)
    {
        fmpz_pow_ui(pi, fp, i);
        fmpz_mul_ui(q, pi, p);
        nnxt = 0;
        for (j = 0; j < ncur; j++)
            for (t = 0; t < p; t++)
            {
                fmpz_set(y, cur + j);
                fmpz_addmul_ui(y, pi, t);
                eval_mod(v, g, y, q);
                if (fmpz_is_zero(v))
                {
                    if (nnxt == cap)
                    {
                        ncur = -1;
                        goto done;
                    }
                    fmpz_set(nxt + nnxt, y);
                    nnxt++;
                }
            }
        tmp = cur;
        cur = nxt;
        nxt = tmp;
        ncur = nnxt;
    }
    *same = 1;
    fmpz_pow_ui(pK, fp, K);
    for (j = 0; j < ncur; j++)
    {
        fmpz_sub(y, cur + j, ap);
        if (!fmpz_divisible(y, pK))
            *same = 0;
    }
done:
    _fmpz_vec_clear(cur, cap);
    _fmpz_vec_clear(nxt, cap);
    fmpz_clear(q);
    fmpz_clear(pi);
    fmpz_clear(v);
    fmpz_clear(y);
    fmpz_clear(pK);
    fmpz_clear(fp);
    return ncur;
}

/* 1 if x = y modulo p^k */
static int
cong(const fmpz_t x, const fmpz_t y, const fmpz_t p, slong k)
{
    fmpz_t d, q;
    int r;

    fmpz_init(d);
    fmpz_init(q);
    fmpz_sub(d, x, y);
    fmpz_pow_ui(q, p, k);
    r = fmpz_divisible(d, q);
    fmpz_clear(d);
    fmpz_clear(q);
    return r;
}

/* A deep snapshot of a list, to test "L untouched": the raw bytes of the struct (the pointers and the
   integers) and copies of what the pointers point to. */
typedef struct
{
    adf_rootlist_struct raw;
    fmpz_poly_t g;
    slong n;
    fmpz * a;
    slong K[4], s[4];
} snap_t;

static void
snap_take(snap_t * S, const adf_rootlist_t L)
{
    slong i;

    memcpy(&S->raw, L, sizeof(adf_rootlist_struct));
    fmpz_poly_init(S->g);
    fmpz_poly_set(S->g, L->g);
    S->n = (L->a != NULL && L->n > 0 && L->n <= 4) ? L->n : 0;
    S->a = _fmpz_vec_init(4);
    for (i = 0; i < S->n; i++)
    {
        fmpz_set(S->a + i, L->a + i);
        S->K[i] = L->K[i];
        S->s[i] = L->s[i];
    }
}

static int
snap_same(const snap_t * S, const adf_rootlist_t L)
{
    slong i;

    if (memcmp(&S->raw, L, sizeof(adf_rootlist_struct)) != 0 || !fmpz_poly_equal(S->g, L->g))
        return 0;
    for (i = 0; i < S->n; i++)
        if (!fmpz_equal(S->a + i, L->a + i) || S->K[i] != L->K[i] || S->s[i] != L->s[i])
            return 0;
    return 1;
}

static void
snap_clear(snap_t * S)
{
    fmpz_poly_clear(S->g);
    _fmpz_vec_clear(S->a, 4);
}

static adf_place_t
place_of(ulong p)
{
    adf_place_t v = adf_place_inf();
    int st = adf_place_prime(&v, p);

    if (st != ADF_OK)
        abort();
    return v;
}

/* Runs the seed function and checks what every OK result must satisfy against the oracles: status
   OK exactly when the strong form holds for the normalised g (oracle); then scope SEED, n = 1,
   nu = 0, complete = 0, g and reduced as the oracle, s = v(g'(a)), K = max(prec, s + 1),
   0 <= a' < p^K, a' = a modulo p^(s+1), is_canonical, verify_entries. Returns the status; writes
   the certificate. The count of failures goes to the runner. */
static int
run_seed(adf_rootlist_t L, fmpz_t ap, slong * K, slong * s, const fmpz_poly_t f, ulong p, const fmpz_t a,
         slong prec)
{
    fmpz_poly_t g, gl;
    fmpz_t fp, pK;
    slong so;
    int reduced, strong, st;

    fmpz_poly_init(g);
    fmpz_poly_init(gl);
    fmpz_init_set_ui(fp, p);
    fmpz_init(pK);
    oracle_normalise(g, &reduced, f);
    strong = oracle_strong(&so, g, fp, a);
    st = adf_root_padic_from_seed(L, f, place_of(p), a, prec);
    ADF_CHECK_MSG(st == (strong ? ADF_OK : ADF_NOT_DETERMINED), "p = %lu, prec %ld: status %d, strong %d", p,
                  (long) prec, st, strong);
    if (st == ADF_OK)
    {
        adf_rootlist_get_poly(gl, L);
        ADF_CHECK(adf_rootlist_get_cert(ap, K, s, L, 0) == 1);
        fmpz_pow_ui(pK, fp, *K);
        ADF_CHECK(fmpz_poly_equal(gl, g));
        ADF_CHECK(L->reduced == reduced);
        ADF_CHECK(adf_rootlist_scope(L) == ADF_ROOTLIST_SEED && adf_rootlist_length(L) == 1 &&
                  adf_rootlist_unresolved_length(L) == 0 && adf_rootlist_is_complete(L) == 0);
        ADF_CHECK(adf_place_equal(adf_rootlist_place(L), place_of(p)));
        ADF_CHECK_MSG(*s == so && *K == FLINT_MAX(prec, so + 1), "s %ld (oracle %ld), K %ld, prec %ld",
                      (long) *s, (long) so, (long) *K, (long) prec);
        ADF_CHECK(fmpz_sgn(ap) >= 0 && fmpz_cmp(ap, pK) < 0);
        ADF_CHECK(cong(ap, a, fp, so + 1));
        ADF_CHECK(adf_rootlist_is_canonical(L));
        ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1);
    }
    fmpz_poly_clear(g);
    fmpz_poly_clear(gl);
    fmpz_clear(fp);
    fmpz_clear(pK);
    return st;
}

/* ---- 1. layout and init ---- */

ADF_TEST(layout_pins)
{
    ADF_CHECK(sizeof(adf_rootlist_struct) == 120);
    ADF_CHECK(ADF_ALIGNOF(adf_rootlist_struct) == 8);
    ADF_CHECK(adf_sizeof_rootlist() == 120 && adf_alignof_rootlist() == 8);
    ADF_CHECK(offsetof(adf_rootlist_struct, place) == 0);
    ADF_CHECK(offsetof(adf_rootlist_struct, scope) == 8);
    ADF_CHECK(offsetof(adf_rootlist_struct, reduced) == 12);
    ADF_CHECK(offsetof(adf_rootlist_struct, complete) == 16);
    ADF_CHECK(offsetof(adf_rootlist_struct, g) == 24);
    ADF_CHECK(offsetof(adf_rootlist_struct, n) == 48);
    ADF_CHECK(offsetof(adf_rootlist_struct, a) == 56);
    ADF_CHECK(offsetof(adf_rootlist_struct, K) == 64);
    ADF_CHECK(offsetof(adf_rootlist_struct, s) == 72);
    ADF_CHECK(offsetof(adf_rootlist_struct, nu) == 80);
    ADF_CHECK(offsetof(adf_rootlist_struct, ua) == 88);
    ADF_CHECK(offsetof(adf_rootlist_struct, ue) == 96);
    ADF_CHECK(offsetof(adf_rootlist_struct, ball) == 104);
    ADF_CHECK(offsetof(adf_rootlist_struct, count) == 112);
    ADF_CHECK(ADF_ROOTLIST_PARTITION == 0 && ADF_ROOTLIST_SEED == 1);
}

ADF_TEST(init_value_and_accessors)
{
    adf_rootlist_t L;
    fmpz_poly_t g, f;
    fmpz_t a;
    adf_fball_t x;
    slong K = -7, s = -7;

    adf_rootlist_init(L);
    fmpz_poly_init(g);
    fmpz_poly_init(f);
    fmpz_init_set_si(a, 99);
    adf_fball_init(x);
    ADF_CHECK(adf_place_is_archimedean(adf_rootlist_place(L)));
    ADF_CHECK(adf_rootlist_scope(L) == ADF_ROOTLIST_PARTITION);
    ADF_CHECK(adf_rootlist_length(L) == 0 && adf_rootlist_unresolved_length(L) == 0);
    ADF_CHECK(adf_rootlist_is_complete(L) == 1);
    ADF_CHECK(L->reduced == 0 && L->count == 0);
    ADF_CHECK(L->a == NULL && L->K == NULL && L->s == NULL && L->ua == NULL && L->ue == NULL && L->ball == NULL);
    adf_rootlist_get_poly(g, L);
    ADF_CHECK(fmpz_poly_is_one(g));
    ADF_CHECK(adf_rootlist_is_canonical(L));
    ADF_CHECK(adf_rootlist_get_cert(a, &K, &s, L, 0) == 0 && fmpz_equal_si(a, 99) && K == -7 && s == -7);
    ADF_CHECK(adf_rootlist_get_fball(x, L, 0) == 0 && adf_fball_is_exact(x));
    fmpz_poly_set_ui(f, 1);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0);      /* the real place: 0 in this slice */
    adf_rootlist_get_poly(L->g, L);                         /* g may be L->g */
    ADF_CHECK(fmpz_poly_is_one(L->g));
    adf_rootlist_clear(L);
    fmpz_poly_clear(g);
    fmpz_poly_clear(f);
    fmpz_clear(a);
    adf_fball_clear(x);
}

/* ---- 2. against the enumeration ---- */

/* the cache of the enumeration for one polynomial and one prime: the key (a mod p^(s+1), s, K) */
typedef struct
{
    fmpz_t c;
    slong s, K;
    slong count;
    int same;
    fmpz_t ap;
} enum_entry;

#define ENUM_CACHE 4096

static ulong lcg_state = 20260929;

static slong
lcg(slong lo, slong hi)
{
    lcg_state = lcg_state * 6364136223846793005UL + 1442695040888963407UL;
    return lo + (slong) ((lcg_state >> 33) % (ulong) (hi - lo + 1));
}

/* The polynomials of the enumeration for the prime p: ten random ones of degree 1 to 4 with
   coefficients in [-3, 3], and eight products with planted integer roots in [-6, 6] (repeated roots
   included), a leading coefficient 1, 2, 3 or p, and now and then a factor X^2 + 1 or X^2 + X + 1. */
static slong
enum_polys(fmpz_poly_t * F, ulong p)
{
    slong k, i, n = 0, deg, nr;
    slong c[5], r[4];
    fmpz_poly_t t;

    fmpz_poly_init(t);
    for (k = 0; k < 10; k++)
    {
        deg = lcg(1, 4);
        for (i = 0; i <= deg; i++)
            c[i] = lcg(-3, 3);
        if (c[deg] == 0)
            c[deg] = lcg(0, 1) ? 1 : -2;
        poly_set_si(F[n++], c, deg + 1);
    }
    for (k = 0; k < 8; k++)
    {
        nr = lcg(1, 4);
        for (i = 0; i < nr; i++)
            r[i] = lcg(-6, 6);
        if (k % 3 == 0 && nr >= 2)
            r[1] = r[0];                                    /* a repeated root */
        poly_from_roots(F[n], k % 4 == 3 ? (slong) p : lcg(1, 3), r, nr);
        if (k % 4 == 1)
        {
            slong q[3] = {1, 0, 1};
            poly_set_si(t, q, 3);
            fmpz_poly_mul(F[n], F[n], t);
        }
        else if (k % 4 == 2)
        {
            slong q[3] = {1, 1, 1};
            poly_set_si(t, q, 3);
            fmpz_poly_mul(F[n], F[n], t);
        }
        n++;
    }
    fmpz_poly_clear(t);
    return n;
}

ADF_TEST(every_seed_below_p4_against_the_enumeration)
{
    static const ulong primes[5] = {2, 3, 5, 7, 11};
    fmpz_poly_t F[18], g;
    adf_rootlist_t L;
    fmpz_t a, ap, fp, c, q;
    enum_entry * cache;
    slong K, s, i, j, np, prec, ncache, e, count;
    ulong p, p4, ai;
    int st, reduced, same;
    slong n_ok = 0, n_nd = 0, n_enum = 0, n_bad = 0, n_s_pos = 0, n_differ_f = 0, n_calls = 0;

    adf_rootlist_init(L);
    fmpz_init(a);
    fmpz_init(ap);
    fmpz_init(fp);
    fmpz_init(c);
    fmpz_init(q);
    fmpz_poly_init(g);
    for (i = 0; i < 18; i++)
        fmpz_poly_init(F[i]);
    cache = flint_malloc(ENUM_CACHE * sizeof(enum_entry));
    for (i = 0; i < ENUM_CACHE; i++)
    {
        fmpz_init(cache[i].c);
        fmpz_init(cache[i].ap);
    }
    for (int ip = 0; ip < 5; ip++)
    {
        p = primes[ip];
        fmpz_set_ui(fp, p);
        p4 = p * p * p * p;
        np = enum_polys(F, p);
        for (i = 0; i < np; i++)
        {
            oracle_normalise(g, &reduced, F[i]);
            ncache = 0;
            for (ai = 0; ai < p4; ai++)
            {
                fmpz_set_ui(a, ai);
                {
                    /* the strong form for f and for g differ (information: check_s2_seed) */
                    slong sf, sg;
                    if (oracle_strong(&sf, F[i], fp, a) != oracle_strong(&sg, g, fp, a))
                        n_differ_f++;
                }
                for (prec = 1; prec <= 6; prec++)
                {
                    n_calls++;
                    st = run_seed(L, ap, &K, &s, F[i], p, a, prec);
                    if (st != ADF_OK)
                    {
                        n_nd++;
                        continue;
                    }
                    n_ok++;
                    if (s > 0)
                        n_s_pos++;
                    /* the enumeration modulo p^(K+s+3) in a + p^(s+1) Z_p, cached by class */
                    fmpz_pow_ui(q, fp, s + 1);
                    fmpz_mod(c, a, q);
                    for (j = 0; j < ncache; j++)
                        if (cache[j].s == s && cache[j].K == K && fmpz_equal(cache[j].c, c))
                            break;
                    if (j == ncache)
                    {
                        if (ncache == ENUM_CACHE)
                            abort();
                        e = s + 1;
                        count = oracle_enum(&same, g, p, c, e, K + s + 3, ap, K, 200000);
                        fmpz_set(cache[j].c, c);
                        cache[j].s = s;
                        cache[j].K = K;
                        cache[j].count = count;
                        cache[j].same = same;
                        fmpz_set(cache[j].ap, ap);
                        ncache++;
                        n_enum++;
                    }
                    {
                        slong ps = 1, t;
                        for (t = 0; t < s; t++)
                            ps *= (slong) p;
                        if (cache[j].count != ps || !cache[j].same || !fmpz_equal(cache[j].ap, ap))
                        {
                            n_bad++;
                            ADF_CHECK_MSG(0, "p = %lu, poly %ld, seed %lu, prec %ld: %ld roots modulo p^(K+s+3) "
                                          "in the ball, expected %ld; same %d", p, (long) i, ai, (long) prec,
                                          (long) cache[j].count, (long) ps, cache[j].same);
                        }
                    }
                }
            }
        }
    }
    printf("enumeration: %ld calls (p = 2, 3, 5, 7, 11; 18 polynomials each; every seed in [0, p^4); prec 1 "
           "to 6): OK %ld (s > 0 in %ld), NOT_DETERMINED %ld; %ld enumerations of balls; strong form of f and "
           "of g differ at %ld seeds; %ld failures\n", (long) n_calls, (long) n_ok, (long) n_s_pos, (long) n_nd,
           (long) n_enum, (long) n_differ_f, (long) n_bad);
    ADF_CHECK(n_ok > 0 && n_nd > 0 && n_s_pos > 0 && n_differ_f > 0);
    for (i = 0; i < ENUM_CACHE; i++)
    {
        fmpz_clear(cache[i].c);
        fmpz_clear(cache[i].ap);
    }
    flint_free(cache);
    for (i = 0; i < 18; i++)
        fmpz_poly_clear(F[i]);
    fmpz_poly_clear(g);
    fmpz_clear(a);
    fmpz_clear(ap);
    fmpz_clear(fp);
    fmpz_clear(c);
    fmpz_clear(q);
    adf_rootlist_clear(L);
}

/* ---- 3. the boundary of the strong form ---- */

ADF_TEST(seeds_with_equality_are_refused_and_L_untouched)
{
    /* v(g(a)) = 2 v(g'(a)) exactly: X^2 + 3 at 2, seed 1 (v(4) = 2, v(2) = 1; solvers P3.2(6));
       X^3 - 10 at 3, seed 1 (Conrad 4.3: |f(1)| = 1/9 = |f'(1)|^2); more are found by the search
       below. Also g'(a) = 0: X^2 - 1 at 3, seed 0. */
    static const slong f1[3] = {3, 0, 1}, f2[4] = {-10, 0, 0, 1}, f3[3] = {-1, 0, 1};
    fmpz_poly_t f, g;
    adf_rootlist_t L;
    fmpz_t a, fp, ga, da;
    snap_t S;
    slong s, n_eq = 0, n_ref = 0;
    ulong p, ai;
    int reduced, st;

    fmpz_poly_init(f);
    fmpz_poly_init(g);
    fmpz_init(a);
    fmpz_init(fp);
    fmpz_init(ga);
    fmpz_init(da);
    adf_rootlist_init(L);
    /* a list with content, so that "untouched" means something */
    poly_set_si(f, f3, 3);
    fmpz_set_ui(a, 1);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(5), a, 3) == ADF_OK);
    snap_take(&S, L);

    poly_set_si(f, f1, 3);
    fmpz_set_ui(a, 1);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(2), a, 4) == ADF_NOT_DETERMINED);
    ADF_CHECK(snap_same(&S, L));
    poly_set_si(f, f2, 4);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(3), a, 4) == ADF_NOT_DETERMINED);
    ADF_CHECK(snap_same(&S, L));
    poly_set_si(f, f3, 3);
    fmpz_set_ui(a, 0);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(3), a, 4) == ADF_NOT_DETERMINED);   /* g'(0) = 0 */
    ADF_CHECK(snap_same(&S, L));

    /* every seed with equality among the polynomials X^2 - d, X^3 - d, d in [-30, 30], p = 2, 3, 5, seeds
       in [0, 200): all refused, L untouched */
    for (slong deg = 2; deg <= 3; deg++)
        for (slong d = -30; d <= 30; d++)
            for (int ip = 0; ip < 3; ip++)
            {
                p = (ulong[]){2, 3, 5}[ip];
                fmpz_set_ui(fp, p);
                fmpz_poly_zero(f);
                fmpz_poly_set_coeff_si(f, deg, 1);
                fmpz_poly_set_coeff_si(f, 0, -d);
                oracle_normalise(g, &reduced, f);
                {
                    fmpz_poly_t dg;
                    fmpz_poly_init(dg);
                    fmpz_poly_derivative(dg, g);
                    for (ai = 0; ai < 200; ai++)
                    {
                        fmpz_set_ui(a, ai);
                        fmpz_poly_evaluate_fmpz(ga, g, a);
                        fmpz_poly_evaluate_fmpz(da, dg, a);
                        if (fmpz_is_zero(ga) || fmpz_is_zero(da))
                            continue;
                        s = vp(da, fp);
                        if (vp(ga, fp) != 2 * s)
                            continue;
                        n_eq++;
                        st = adf_root_padic_from_seed(L, f, place_of(p), a, 3);
                        if (st == ADF_NOT_DETERMINED && snap_same(&S, L))
                            n_ref++;
                    }
                    fmpz_poly_clear(dg);
                }
            }
    printf("seeds with v(g(a)) = 2 v(g'(a)): %ld, refused with L untouched: %ld\n", (long) n_eq, (long) n_ref);
    ADF_CHECK(n_eq > 100 && n_ref == n_eq);
    snap_clear(&S);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_poly_clear(g);
    fmpz_clear(a);
    fmpz_clear(fp);
    fmpz_clear(ga);
    fmpz_clear(da);
}

/* ---- 4. the examples ---- */

/* the certificate of the seed a for f at p with prec, into (ap, K, s); status returned */
static int
seed_cert(fmpz_t ap, slong * K, slong * s, const slong * c, slong len, ulong p, slong a, slong prec)
{
    fmpz_poly_t f;
    adf_rootlist_t L;
    fmpz_t fa;
    int st;

    fmpz_poly_init(f);
    fmpz_init_set_si(fa, a);
    adf_rootlist_init(L);
    poly_set_si(f, c, len);
    st = run_seed(L, ap, K, s, f, p, fa, prec);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_clear(fa);
    return st;
}

ADF_TEST(conrad_examples_4_2_4_3_4_4)
{
    /* conrad-hensel:hensel.txt:331 to 361 */
    static const slong c42[5] = {1, 2, 2, -7, 1};         /* X^4 - 7X^3 + 2X^2 + 2X + 1 at 3 */
    static const slong c43f[4] = {-10, 0, 0, 1};          /* X^3 - 10 at 3 */
    static const slong c43g[4] = {-5, 0, 0, 1};           /* X^3 - 5 at 3 */
    static const slong c44[4] = {-8, -2, -1, 1};          /* X^3 - X^2 - 2X - 8 at 2 */
    fmpz_t ap;
    slong K, s;

    fmpz_init(ap);
    /* 4.2: seed 2: f(2) = -27, f'(2) = -42: the root = 2 mod 9, digits 2 + 3^2 + 3^4 + 2 3^5 + 2 3^6
       (hensel.txt:338): 2036 modulo 3^7 */
    ADF_CHECK(seed_cert(ap, &K, &s, c42, 5, 3, 2, 7) == ADF_OK);
    ADF_CHECK(s == 1 && K == 7 && fmpz_equal_si(ap, 2036));
    /* seed 5: |f(5)| = 1/27 < |f'(5)|^2 = 1/9; the root = 5 mod 9, digits 2 + 3 + 2 3^2 + 2 3^3 + 2 3^4
       + 2 3^6: 1697 modulo 3^7 */
    ADF_CHECK(seed_cert(ap, &K, &s, c42, 5, 3, 5, 7) == ADF_OK);
    ADF_CHECK(s == 1 && K == 7 && fmpz_equal_si(ap, 1697));
    /* 4.3: X^3 - 10, seed 1 refused (|f(1)| = 1/9 = |f'(1)|^2), seed 4 accepted: the root = 4 mod 9,
       digits 1 + 3 + 3^2 + 2 3^6 + 3^7: 3658 modulo 3^8 */
    ADF_CHECK(seed_cert(ap, &K, &s, c43f, 4, 3, 1, 8) == ADF_NOT_DETERMINED);
    ADF_CHECK(seed_cert(ap, &K, &s, c43f, 4, 3, 4, 8) == ADF_OK);
    ADF_CHECK(s == 1 && K == 8 && fmpz_equal_si(ap, 3658));
    /* X^3 - 5, seed 2: |g(2)| = 1/3, |g'(2)| = 1/9: refused */
    ADF_CHECK(seed_cert(ap, &K, &s, c43g, 4, 3, 2, 8) == ADF_NOT_DETERMINED);
    /* 4.4: seed 0: beta = 2^2 + 2^4 + 2^6 + 2^7 + 2^8 + 2^10 + ...: 1492 modulo 2^11, s = 1 */
    ADF_CHECK(seed_cert(ap, &K, &s, c44, 4, 2, 0, 11) == ADF_OK);
    ADF_CHECK(s == 1 && K == 11 && fmpz_equal_si(ap, 1492));
    /* seed 2: gamma = 2 + 2^2 + 2^4 + 2^6 + 2^7 + 2^8 + 2^9 + ...: 982 modulo 2^10 */
    ADF_CHECK(seed_cert(ap, &K, &s, c44, 4, 2, 2, 10) == ADF_OK);
    ADF_CHECK(s == 1 && K == 10 && fmpz_equal_si(ap, 982));
    /* seed 1: alpha = 1 + 2 + 2^2 + 2^4 + 2^6 + 2^9 + ... (Example 2.7): 599 modulo 2^10, s = 0 */
    ADF_CHECK(seed_cert(ap, &K, &s, c44, 4, 2, 1, 10) == ADF_OK);
    ADF_CHECK(s == 0 && K == 10 && fmpz_equal_si(ap, 599));
    fmpz_clear(ap);
}

ADF_TEST(normalised_polynomial_27X_repeated_factor_and_the_prime_2)
{
    static const slong c27[2] = {0, 27};
    static const slong crep[4] = {2, -3, 0, 1};           /* (X - 1)^2 (X + 2) = X^3 - 3X + 2 */
    static const slong c9[3] = {-9, 0, 1}, c1[3] = {1, 0, 1}, c17[3] = {-17, 0, 1}, c3[3] = {-3, 0, 1};
    static const slong c32[4] = {0, 2, 0, 1};             /* X^3 + 2X at 2 (solvers 3.11(5)) */
    static const slong cneg[3] = {18, 0, -6};             /* -6 X^2 + 18 = -6 (X^2 - 3): g = X^2 - 3 */
    fmpz_poly_t f, g;
    adf_rootlist_t L;
    fmpz_t a, ap;
    slong K, s;

    fmpz_poly_init(f);
    fmpz_poly_init(g);
    fmpz_init(a);
    fmpz_init(ap);
    adf_rootlist_init(L);

    /* f = 27 X at 3, seed 0: g = X, certificate (0, K, 0), not (0, K, 3) (solvers P3.12(3)) */
    poly_set_si(f, c27, 2);
    fmpz_set_ui(a, 0);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(3), a, 5) == ADF_OK);
    adf_rootlist_get_poly(g, L);
    ADF_CHECK(fmpz_poly_degree(g) == 1 && fmpz_is_one(g->coeffs + 1) && fmpz_is_zero(g->coeffs + 0));
    ADF_CHECK(adf_rootlist_get_cert(ap, &K, &s, L, 0) && fmpz_is_zero(ap) && K == 5 && s == 0);
    ADF_CHECK(L->reduced == 0);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1);

    /* a repeated factor: (X - 1)^2 (X + 2); g = X^2 + X - 2, reduced = 1, and 1 is a simple root of g:
       at 3, g'(1) = 3, s = 1; at 5, s = 0 */
    poly_set_si(f, crep, 4);
    fmpz_set_ui(a, 1);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(3), a, 1) == ADF_OK);
    adf_rootlist_get_poly(g, L);
    {
        static const slong cg[3] = {-2, 1, 1};
        fmpz_poly_t h;
        fmpz_poly_init(h);
        poly_set_si(h, cg, 3);
        ADF_CHECK(fmpz_poly_equal(g, h));
        fmpz_poly_clear(h);
    }
    ADF_CHECK(L->reduced == 1);
    ADF_CHECK(adf_rootlist_get_cert(ap, &K, &s, L, 0) && fmpz_equal_si(ap, 1) && K == 2 && s == 1);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(5), a, 3) == ADF_OK);
    ADF_CHECK(adf_rootlist_get_cert(ap, &K, &s, L, 0) && fmpz_equal_si(ap, 1) && K == 3 && s == 0);
    ADF_CHECK(L->reduced == 1 && adf_rootlist_verify_entries(L, f) == 1);
    L->reduced = 0;                                       /* a false flag is refused */
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0);
    L->reduced = 1;
    /* the root -2 of multiplicity one of f */
    fmpz_set_si(a, -2);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(5), a, 3) == ADF_OK);
    ADF_CHECK(adf_rootlist_get_cert(ap, &K, &s, L, 0) && fmpz_equal_si(ap, 123) && K == 3 && s == 0);

    /* content and sign: -6 X^2 + 18 has g = X^2 - 3, reduced = 0 */
    poly_set_si(f, cneg, 3);
    fmpz_set_ui(a, 4);                                    /* 4^2 = 16 = 3 mod 13 */
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(13), a, 2) == ADF_OK);
    adf_rootlist_get_poly(g, L);
    ADF_CHECK(fmpz_poly_degree(g) == 2 && fmpz_equal_si(g->coeffs + 0, -3) && fmpz_is_one(g->coeffs + 2));
    ADF_CHECK(L->reduced == 0);

    /* the prime 2 (solvers 3.11): X^2 - 9, seeds 1 and 3: s = 1, the roots -3 (13 mod 16) and 3
       (check_s2_examples, proto/solvers_checks.py:2000) */
    ADF_CHECK(seed_cert(ap, &K, &s, c9, 3, 2, 1, 4) == ADF_OK && fmpz_equal_si(ap, 13) && K == 4 && s == 1);
    ADF_CHECK(seed_cert(ap, &K, &s, c9, 3, 2, 3, 4) == ADF_OK && fmpz_equal_si(ap, 3) && K == 4 && s == 1);
    ADF_CHECK(seed_cert(ap, &K, &s, c9, 3, 2, 3, 1) == ADF_OK && fmpz_equal_si(ap, 3) && K == 2 && s == 1);
    /* X^2 + 1 at 2, seed 1: v(2) = 1 = ... 1 <= 2: refused (the root 1 modulo 2 does not lift) */
    ADF_CHECK(seed_cert(ap, &K, &s, c1, 3, 2, 1, 4) == ADF_NOT_DETERMINED);
    /* X^2 - 3 at 2: no seed works (3 is no square in Z_2) */
    for (slong t = -8; t < 64; t++)
        ADF_CHECK(seed_cert(ap, &K, &s, c3, 3, 2, t, 4) == ADF_NOT_DETERMINED);
    /* X^2 - 17 at 2, seed 1: a square root of 17 in Z_2 to 40 digits */
    ADF_CHECK(seed_cert(ap, &K, &s, c17, 3, 2, 1, 40) == ADF_OK && K == 40 && s == 1);
    {
        fmpz_t t, q;
        fmpz_init(t);
        fmpz_init(q);
        fmpz_mul(t, ap, ap);
        fmpz_sub_ui(t, t, 17);
        fmpz_ui_pow_ui(q, 2, 41);                         /* a'^2 = 17 mod 2^(K+s) */
        ADF_CHECK(fmpz_divisible(t, q));
        fmpz_clear(t);
        fmpz_clear(q);
    }
    /* X^3 + 2X at 2, seed 0: g(0) = 0, s = 1, K = max(prec, 2) (solvers 3.11(5)) */
    ADF_CHECK(seed_cert(ap, &K, &s, c32, 4, 2, 0, 1) == ADF_OK && fmpz_is_zero(ap) && K == 2 && s == 1);

    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_poly_clear(g);
    fmpz_clear(a);
    fmpz_clear(ap);
}

/* ---- 5. K = max(prec_p, s + 1), nested balls; negative and large seeds ---- */

ADF_TEST(precision_and_nested_balls)
{
    static const slong cases[][6] = {
        /* len, coefficients..., p is in the table below */
        {3, -2, 0, 1, 0, 0},                              /* X^2 - 2 at 7, seed 3 */
        {3, -9, 0, 1, 0, 0},                              /* X^2 - 9 at 2, seed 1 */
        {5, 1, 2, 2, -7, 1},                              /* Conrad 4.2 at 3, seed 2 */
        {4, -8, -2, -1, 1, 0},                            /* Conrad 4.4 at 2, seed 0 */
        {3, -17, 0, 1, 0, 0},                             /* X^2 - 17 at 2, seed 1 */
        {3, 0, -16, 1, 0, 0},                             /* X (X - 16) at 2, seed 0: s = 4 */
    };
    static const ulong ps[6] = {7, 2, 3, 2, 2, 2};
    static const slong seeds[6] = {3, 1, 2, 0, 1, 0};
    fmpz_t ap, prev, fp, a;
    slong K, s, Kprev, prec, k, n_nested = 0;

    fmpz_init(ap);
    fmpz_init(prev);
    fmpz_init(fp);
    fmpz_init(a);
    for (k = 0; k < 6; k++)
    {
        fmpz_set_ui(fp, ps[k]);
        Kprev = 0;
        for (prec = 1; prec <= 60; prec++)
        {
            ADF_CHECK(seed_cert(ap, &K, &s, cases[k] + 1, cases[k][0], ps[k], seeds[k], prec) == ADF_OK);
            ADF_CHECK(K == FLINT_MAX(prec, s + 1));
            if (Kprev > 0)
            {
                ADF_CHECK(cong(ap, prev, fp, Kprev));
                n_nested++;
            }
            fmpz_set(prev, ap);
            Kprev = K;
        }
        /* the same root from other representatives of the seed class: negative and huge seeds */
        for (int t = 0; t < 4; t++)
        {
            fmpz_t big;
            adf_rootlist_t L;
            fmpz_poly_t f;
            slong K2, s2;
            fmpz_t ap2;

            fmpz_init(big);
            fmpz_init(ap2);
            fmpz_poly_init(f);
            adf_rootlist_init(L);
            poly_set_si(f, cases[k] + 1, cases[k][0]);
            fmpz_pow_ui(big, fp, 64 + 7 * t);             /* seed + p^(64 + 7t) * (+-(3^200 + t)) */
            fmpz_ui_pow_ui(a, 3, 200);
            fmpz_add_ui(a, a, t);
            if (t % 2)
                fmpz_neg(a, a);
            fmpz_mul(big, big, a);
            fmpz_add_si(big, big, seeds[k]);
            ADF_CHECK(run_seed(L, ap2, &K2, &s2, f, ps[k], big, 60) == ADF_OK);
            ADF_CHECK(fmpz_equal(ap2, prev) && K2 == Kprev);
            adf_rootlist_clear(L);
            fmpz_poly_clear(f);
            fmpz_clear(big);
            fmpz_clear(ap2);
        }
    }
    ADF_CHECK(n_nested == 6 * 59);
    fmpz_clear(ap);
    fmpz_clear(prev);
    fmpz_clear(fp);
    fmpz_clear(a);
}

/* ---- 6. large primes, large precision, the limits ---- */

/* checks a certificate (a', K, s) for g at an fmpz prime independently: 0 <= a' < p^K, K > s,
   v(g'(a')) = s, g(a') = 0 modulo p^(K+s) */
static int
cert_true(const fmpz_poly_t g, const fmpz_t p, const fmpz_t ap, slong K, slong s)
{
    fmpz_poly_t d;
    fmpz_t q, v;
    int ok;

    fmpz_poly_init(d);
    fmpz_init(q);
    fmpz_init(v);
    fmpz_pow_ui(q, p, K);
    ok = K > s && s >= 0 && fmpz_sgn(ap) >= 0 && fmpz_cmp(ap, q) < 0;
    if (ok)
    {
        fmpz_poly_derivative(d, g);
        fmpz_poly_evaluate_fmpz(v, d, ap);
        ok = !fmpz_is_zero(v) && vp(v, p) == s;
        fmpz_pow_ui(q, p, K + s);
        fmpz_poly_evaluate_fmpz(v, g, ap);
        ok = ok && fmpz_divisible(v, q);
    }
    fmpz_poly_clear(d);
    fmpz_clear(q);
    fmpz_clear(v);
    return ok;
}

ADF_TEST(large_primes_and_large_precision)
{
    /* p of 64 bits through the place: 2^64 - 59, the largest prime below 2^64 */
    fmpz_poly_t f, g;
    fmpz_t p, a, ap, r1, r2, t;
    adf_rootlist_t L;
    slong K, s, prec;
    int st;

    fmpz_poly_init(f);
    fmpz_poly_init(g);
    fmpz_init(p);
    fmpz_init(a);
    fmpz_init(ap);
    fmpz_init(r1);
    fmpz_init(r2);
    fmpz_init(t);
    adf_rootlist_init(L);

    fmpz_set_ui(p, UWORD(18446744073709551557));
    ADF_CHECK(fmpz_is_prime(p) == 1 && fmpz_bits(p) == 64);
    /* g = (X - r1)(X - r2)(X^2 + 1) with r1 = 3^50, r2 = r1 + p^3 (s = 3 at r1), seeds r1 + p^4 t */
    fmpz_ui_pow_ui(r1, 3, 50);
    fmpz_pow_ui(r2, p, 3);
    fmpz_add(r2, r2, r1);
    {
        fmpz_poly_t h;
        fmpz_poly_init(h);
        fmpz_poly_set_coeff_si(f, 1, 1);
        fmpz_neg(t, r1);
        fmpz_poly_set_coeff_fmpz(f, 0, t);
        fmpz_poly_set_coeff_si(h, 1, 1);
        fmpz_neg(t, r2);
        fmpz_poly_set_coeff_fmpz(h, 0, t);
        fmpz_poly_mul(f, f, h);
        fmpz_poly_zero(h);
        fmpz_poly_set_coeff_si(h, 2, 1);
        fmpz_poly_set_coeff_si(h, 0, 1);
        fmpz_poly_mul(f, f, h);
        fmpz_poly_clear(h);
    }
    for (int ip = 0; ip < 8; ip++)
    {
        prec = (slong[]){1, 4, 13, 40, 121, 364, 1093, 2000}[ip];
        fmpz_pow_ui(a, p, 4);
        fmpz_mul_ui(a, a, 12345);
        fmpz_add(a, a, r1);
        st = adf_root_padic_from_seed(L, f, place_of(UWORD(18446744073709551557)), a, prec);
        ADF_CHECK(st == ADF_OK);
        if (st != ADF_OK)
            break;
        adf_rootlist_get_cert(ap, &K, &s, L, 0);
        ADF_CHECK(s == 3 && K == FLINT_MAX(prec, 4));
        ADF_CHECK(cert_true(L->g, p, ap, K, s));
        fmpz_sub(t, ap, r1);                               /* the root is the integer r1 */
        {
            fmpz_t q;
            fmpz_init(q);
            fmpz_pow_ui(q, p, K);
            fmpz_mod(t, t, q);
            ADF_CHECK(fmpz_is_zero(t));
            fmpz_clear(q);
        }
        ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1);
    }
    /* a square root of c = r^2 + 5 p in Z_p, p = 2^64 - 59, from the seed r (g(r) = -5 p, v = 1;
       g'(r) = 2 r, a unit: s = 0), to 2000 digits: a'^2 = c modulo p^2000 */
    {
        fmpz_t c, q;

        fmpz_init(c);
        fmpz_init(q);
        fmpz_set_ui(a, UWORD(12345678901234567));
        fmpz_mul(c, a, a);
        fmpz_addmul_ui(c, p, 5);
        fmpz_poly_zero(f);
        fmpz_poly_set_coeff_si(f, 2, 1);
        fmpz_neg(t, c);
        fmpz_poly_set_coeff_fmpz(f, 0, t);
        ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(UWORD(18446744073709551557)), a, 2000) == ADF_OK);
        adf_rootlist_get_cert(ap, &K, &s, L, 0);
        ADF_CHECK(K == 2000 && s == 0 && cert_true(L->g, p, ap, K, s));
        fmpz_mul(t, ap, ap);
        fmpz_sub(t, t, c);
        fmpz_pow_ui(q, p, 2000);
        ADF_CHECK(fmpz_divisible(t, q));
        ADF_CHECK(cong(ap, a, p, 1));
        fmpz_clear(c);
        fmpz_clear(q);
    }

    /* primes of 65, 89, 300 and 521 bits through the hidden core: 2^64 + 13, 2^89 - 1, 2^299 + 443,
       2^521 - 1 (the Mersenne primes M89 and M521; 2^64 + 13 and 2^299 + 443 are tested here) */
    {
        fmpz_t P[4];
        slong bits[4] = {65, 89, 300, 521};
        int k;

        for (k = 0; k < 4; k++)
            fmpz_init(P[k]);
        fmpz_ui_pow_ui(P[0], 2, 64);
        fmpz_add_ui(P[0], P[0], 13);
        fmpz_ui_pow_ui(P[1], 2, 89);
        fmpz_sub_ui(P[1], P[1], 1);
        fmpz_ui_pow_ui(P[2], 2, 299);
        fmpz_add_ui(P[2], P[2], 443);
        fmpz_ui_pow_ui(P[3], 2, 521);
        fmpz_sub_ui(P[3], P[3], 1);
        for (k = 0; k < 4; k++)
        {
            ADF_CHECK(fmpz_bits(P[k]) == (flint_bitcnt_t) bits[k]);
            if (k < 2)
                ADF_CHECK(fmpz_is_prime(P[k]) == 1);
            else
                ADF_CHECK(fmpz_is_probabprime(P[k]) == 1);
            /* g = (X - r1)(X - r2) with r2 = r1 + p^2 (s = 2), primitive with positive leading coefficient */
            fmpz_ui_pow_ui(r1, 7, 30);
            fmpz_pow_ui(r2, P[k], 2);
            fmpz_add(r2, r2, r1);
            fmpz_poly_zero(g);
            fmpz_poly_set_coeff_si(g, 2, 1);
            fmpz_add(t, r1, r2);
            fmpz_neg(t, t);
            fmpz_poly_set_coeff_fmpz(g, 1, t);
            fmpz_mul(t, r1, r2);
            fmpz_poly_set_coeff_fmpz(g, 0, t);
            for (int ip = 0; ip < 6; ip++)
            {
                prec = (slong[]){1, 7, 31, 127, 511, 2000}[ip];
                fmpz_pow_ui(a, P[k], 3);
                fmpz_mul_si(a, a, -77);
                fmpz_add(a, a, r1);                        /* a seed = r1 modulo p^3 */
                st = adf_roots_seed_core(ap, &K, &s, g, P[k], a, prec);
                ADF_CHECK_MSG(st == ADF_OK, "bits %ld, prec %ld: status %d", (long) bits[k], (long) prec, st);
                if (st != ADF_OK)
                    continue;
                ADF_CHECK(s == 2 && K == FLINT_MAX(prec, 3));
                ADF_CHECK(cert_true(g, P[k], ap, K, s));
                ADF_CHECK(adf_roots_cert_check(g, P[k], ap, K, s) == 1);
                ADF_CHECK(adf_roots_cert_check(g, P[k], ap, K, s + 1) == 0);
                ADF_CHECK(cong(ap, r1, P[k], K));
            }
            /* a seed that is not close enough: r1 + p (v(g(a)) = 1 + 2 ... no: v(g'(a)) = 1, v(g(a)) = 2) */
            fmpz_add(a, r1, P[k]);
            ADF_CHECK(adf_roots_seed_core(ap, &K, &s, g, P[k], a, 5) == ADF_NOT_DETERMINED);
            /* the limit: 2 K bits(p) > ADF_ROOTS_BITS_MAX */
            {
                slong kmax = ADF_ROOTS_BITS_MAX / (2 * bits[k]);
                fmpz_set(a, r1);
                ADF_CHECK(adf_roots_seed_core(ap, &K, &s, g, P[k], a, kmax + 1) == ADF_LIMIT);
                ADF_CHECK(adf_roots_seed_core(ap, &K, &s, g, P[k], a, WORD_MAX) == ADF_LIMIT);
            }
        }
        for (k = 0; k < 4; k++)
            fmpz_clear(P[k]);
    }
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_poly_clear(g);
    fmpz_clear(p);
    fmpz_clear(a);
    fmpz_clear(ap);
    fmpz_clear(r1);
    fmpz_clear(r2);
    fmpz_clear(t);
}

/* counting FLINT's allocator (flint.h: __flint_set_memory_functions); GMP's own allocations of mpz
   limbs and libc are not counted */
static unsigned long flint_allocs = 0;
static void * (*orig_alloc)(size_t);
static void * (*orig_calloc)(size_t, size_t);
static void * (*orig_realloc)(void *, size_t);
static void (*orig_free)(void *);

static void *
count_alloc(size_t n)
{
    flint_allocs++;
    return orig_alloc(n);
}

static void *
count_calloc(size_t n, size_t m)
{
    flint_allocs++;
    return orig_calloc(n, m);
}

static void *
count_realloc(void * p, size_t n)
{
    flint_allocs++;
    return orig_realloc(p, n);
}

/* the seed function with FLINT's allocator counted; the number of allocations in *count */
static int
seed_counted(unsigned long * count, adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, const fmpz_t a,
             slong prec)
{
    int st;

    __flint_get_memory_functions(&orig_alloc, &orig_calloc, &orig_realloc, &orig_free);
    flint_allocs = 0;
    __flint_set_memory_functions(count_alloc, count_calloc, count_realloc, orig_free);
    st = adf_root_padic_from_seed(L, f, p, a, prec);
    __flint_set_memory_functions(orig_alloc, orig_calloc, orig_realloc, orig_free);
    *count = flint_allocs;
    return st;
}

ADF_TEST(limit_of_the_precision_is_decided_before_any_allocation)
{
    static const slong c2[3] = {-2, 0, 1};
    fmpz_poly_t f;
    fmpz_t a;
    adf_rootlist_t L;
    adf_place_t p7 = place_of(7), p64 = place_of(UWORD(18446744073709551557));
    unsigned long n_ok, n_limit7, n_limit64, n_limit64b;

    fmpz_poly_init(f);
    fmpz_init_set_ui(a, 3);
    adf_rootlist_init(L);
    poly_set_si(f, c2, 3);
    ADF_CHECK(seed_counted(&n_ok, L, f, p7, a, 20) == ADF_OK);
    ADF_CHECK(seed_counted(&n_limit7, L, f, p7, a, WORD_MAX) == ADF_LIMIT);
    ADF_CHECK(seed_counted(&n_limit64, L, f, p64, a, WORD_MAX) == ADF_LIMIT);
    ADF_CHECK(seed_counted(&n_limit64b, L, f, p64, a, ADF_ROOTS_BITS_MAX / 128 + 1) == ADF_LIMIT);
    printf("FLINT allocations: an OK call %lu; LIMIT calls %lu, %lu, %lu\n", n_ok, n_limit7, n_limit64, n_limit64b);
    ADF_CHECK(n_ok > 0);                                   /* the counter counts */
    ADF_CHECK(n_limit7 == 0 && n_limit64 == 0 && n_limit64b == 0);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_clear(a);
}

ADF_TEST(limits_give_LIMIT_and_leave_L_untouched)
{
    static const slong c2[3] = {-2, 0, 1};
    fmpz_poly_t f;
    fmpz_t a, ap;
    adf_rootlist_t L;
    snap_t S;
    slong K, s, kmax;
    ulong p64 = UWORD(18446744073709551557);

    fmpz_poly_init(f);
    fmpz_init(a);
    fmpz_init(ap);
    adf_rootlist_init(L);
    poly_set_si(f, c2, 3);
    fmpz_set_ui(a, 3);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(7), a, 5) == ADF_OK);
    snap_take(&S, L);

    /* prec_p = WORD_MAX: LIMIT, not an overflow */
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(7), a, WORD_MAX) == ADF_LIMIT);
    ADF_CHECK(snap_same(&S, L));
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(2), a, WORD_MAX) == ADF_LIMIT);
    ADF_CHECK(snap_same(&S, L));
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(p64), a, WORD_MAX / 2 + 1) == ADF_LIMIT);
    ADF_CHECK(snap_same(&S, L));
    /* the limit for p = 7 (3 bits): 2 K 3 <= 2^24 exactly for K <= 2796202 */
    kmax = ADF_ROOTS_BITS_MAX / 6;
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(7), a, kmax + 1) == ADF_LIMIT);
    ADF_CHECK(snap_same(&S, L));
    /* the limit is LIMIT also for a seed that would be NOT_DETERMINED: decided before g is formed */
    fmpz_set_ui(a, 1);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(7), a, kmax + 1) == ADF_LIMIT);
    ADF_CHECK(snap_same(&S, L));
    /* p of 64 bits at the limit: K = 2^24 / 128 = 131072 is computed, one more is LIMIT */
    kmax = ADF_ROOTS_BITS_MAX / 128;
    {
        /* a square root of p - 2 ... simpler: X (X - 1) + p X has the root 0 with s = 0 at every p:
           g = X^2 + (p - 1) X, seed 0, g(0) = 0: a' = 0 without Newton steps */
        fmpz_poly_t h;
        fmpz_t t;

        fmpz_poly_init(h);
        fmpz_init_set_ui(t, p64);
        fmpz_sub_ui(t, t, 1);
        fmpz_poly_set_coeff_si(h, 2, 1);
        fmpz_poly_set_coeff_fmpz(h, 1, t);
        fmpz_set_ui(a, 0);
        ADF_CHECK(adf_root_padic_from_seed(L, h, place_of(p64), a, kmax + 1) == ADF_LIMIT);
        ADF_CHECK(snap_same(&S, L));
        /* a seed with Newton steps at the largest precision: seed 1 of X^2 + (p - 1) X - p = (X - 1)(X + p) */
        fmpz_poly_set_coeff_si(h, 0, 0);
        fmpz_set_ui(t, p64);
        fmpz_neg(t, t);
        fmpz_poly_set_coeff_fmpz(h, 0, t);
        fmpz_set_ui(a, 1);
        fmpz_ui_pow_ui(t, 5, 40);                         /* seed 1 + 5^40 p: Newton steps from k0 = 1 */
        fmpz_mul_ui(t, t, p64);
        fmpz_add(a, a, t);
        ADF_CHECK(adf_root_padic_from_seed(L, h, place_of(p64), a, kmax) == ADF_OK);
        ADF_CHECK(adf_rootlist_get_cert(ap, &K, &s, L, 0) && K == kmax && s == 0 && fmpz_is_one(ap));
        fmpz_poly_clear(h);
        fmpz_clear(t);
        snap_clear(&S);
        snap_take(&S, L);
    }
    /* a LIMIT that comes from s: g = X (X - 2^m) at 2, seed 0: s = m, K = m + 1 for prec 1; 2 K 2 is
       at the limit for m + 1 = 2^22 and above it for m + 1 = 2^22 + 1 */
    {
        fmpz_poly_t h;
        fmpz_t t;
        slong m;

        fmpz_poly_init(h);
        fmpz_init(t);
        for (m = (WORD(1) << 22) - 1; m <= (WORD(1) << 22); m++)
        {
            fmpz_poly_zero(h);
            fmpz_poly_set_coeff_si(h, 2, 1);
            fmpz_ui_pow_ui(t, 2, (ulong) m);
            fmpz_neg(t, t);
            fmpz_poly_set_coeff_fmpz(h, 1, t);
            fmpz_set_ui(a, 0);
            if (m == (WORD(1) << 22) - 1)
            {
                ADF_CHECK(adf_root_padic_from_seed(L, h, place_of(2), a, 1) == ADF_OK);
                ADF_CHECK(adf_rootlist_get_cert(ap, &K, &s, L, 0) && K == m + 1 && s == m && fmpz_is_zero(ap));
                snap_clear(&S);
                snap_take(&S, L);
            }
            else
            {
                ADF_CHECK(adf_root_padic_from_seed(L, h, place_of(2), a, 1) == ADF_LIMIT);
                ADF_CHECK(snap_same(&S, L));
            }
        }
        fmpz_poly_clear(h);
        fmpz_clear(t);
    }
    snap_clear(&S);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_clear(a);
    fmpz_clear(ap);
}

/* ---- 7. statuses DOMAIN, aliasing, get_fball ---- */

ADF_TEST(domain_statuses_leave_L_untouched)
{
    static const slong c2[3] = {-2, 0, 1};
    fmpz_poly_t f, z;
    fmpz_t a;
    adf_rootlist_t L;
    snap_t S;

    fmpz_poly_init(f);
    fmpz_poly_init(z);
    fmpz_init_set_ui(a, 3);
    adf_rootlist_init(L);
    poly_set_si(f, c2, 3);
    /* first on the init value, then on a list with content */
    for (int round = 0; round < 2; round++)
    {
        snap_take(&S, L);
        ADF_CHECK(adf_root_padic_from_seed(L, z, place_of(7), a, 5) == ADF_DOMAIN);
        ADF_CHECK(snap_same(&S, L));
        ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(7), a, 0) == ADF_DOMAIN);
        ADF_CHECK(snap_same(&S, L));
        ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(7), a, -5) == ADF_DOMAIN);
        ADF_CHECK(snap_same(&S, L));
        ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(7), a, WORD_MIN) == ADF_DOMAIN);
        ADF_CHECK(snap_same(&S, L));
        ADF_CHECK(adf_root_padic_from_seed(L, f, adf_place_inf(), a, 5) == ADF_DOMAIN);
        ADF_CHECK(snap_same(&S, L));
        ADF_CHECK(adf_root_padic_from_seed(L, z, adf_place_inf(), a, 0) == ADF_DOMAIN);
        ADF_CHECK(snap_same(&S, L));
        ADF_CHECK(adf_rootlist_verify_entries(L, z) == 0);
        snap_clear(&S);
        ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(7), a, 5) == ADF_OK);
    }
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_poly_clear(z);
    fmpz_clear(a);
}

ADF_TEST(aliasing_f_is_L_g_and_a_is_L_a)
{
    static const slong c[4] = {-10, 0, 0, 1};             /* X^3 - 10 at 3, seed 4 */
    fmpz_poly_t f;
    fmpz_t a, ap1, ap2;
    adf_rootlist_t L;
    slong K, s;

    fmpz_poly_init(f);
    fmpz_init_set_ui(a, 4);
    fmpz_init(ap1);
    fmpz_init(ap2);
    adf_rootlist_init(L);
    poly_set_si(f, c, 4);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(3), a, 12) == ADF_OK);
    adf_rootlist_get_cert(ap1, &K, &s, L, 0);
    /* f = L->g and a = L->a: the same root at a higher precision */
    ADF_CHECK(adf_root_padic_from_seed(L, L->g, place_of(3), L->a, 30) == ADF_OK);
    adf_rootlist_get_cert(ap2, &K, &s, L, 0);
    ADF_CHECK(K == 30 && s == 1 && cong(ap1, ap2, (fmpz[]){3}, 12));
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1);
    /* get_cert into L->a itself */
    ADF_CHECK(adf_rootlist_get_cert(L->a, &K, &s, L, 0) == 1 && fmpz_equal(L->a, ap2));
    /* out of range */
    ADF_CHECK(adf_rootlist_get_cert(ap1, &K, &s, L, 1) == 0 && adf_rootlist_get_cert(ap1, &K, &s, L, -1) == 0);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_clear(a);
    fmpz_clear(ap1);
    fmpz_clear(ap2);
}

ADF_TEST(get_fball_is_the_canonical_cylinder)
{
    static const slong c[3] = {-2, 0, 1};
    fmpz_poly_t f;
    fmpz_t a, ap, A, H, d, q;
    adf_rootlist_t L;
    adf_fball_t x;
    slong K, s;

    fmpz_poly_init(f);
    fmpz_init_set_ui(a, 3);
    fmpz_init(ap);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(q);
    adf_fball_init(x);
    adf_rootlist_init(L);
    poly_set_si(f, c, 3);
    ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(7), a, 20) == ADF_OK);
    adf_rootlist_get_cert(ap, &K, &s, L, 0);
    ADF_CHECK(adf_rootlist_get_fball(x, L, 0) == 1);
    ADF_CHECK(adf_fball_is_canonical(x) && x->backend == ADF_GLOBAL);
    adf_fball_get_fmpz3(A, H, d, x);
    fmpz_ui_pow_ui(q, 7, 20);
    ADF_CHECK(fmpz_equal(A, ap) && fmpz_equal(H, q) && fmpz_is_one(d));
    adf_fball_zero(x);
    ADF_CHECK(adf_rootlist_get_fball(x, L, 1) == 0 && adf_fball_is_exact(x));
    ADF_CHECK(adf_rootlist_get_fball(x, L, -1) == 0 && adf_fball_is_exact(x));
    /* the ball a' = 0 (27 X at 3) is (0, 3^K, 1) */
    {
        static const slong c27[2] = {0, 27};
        poly_set_si(f, c27, 2);
        fmpz_set_ui(a, 1);
        /* g = X, v(g(1)) = 0 */
        ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(3), a, 4) == ADF_NOT_DETERMINED);
        /* seed 9: k0 = v(9) - 0 = 2 < K = 4, Newton from (0, 2, 0); seed 81: k0 = 4 = K, reduced directly */
        for (int t = 0; t < 2; t++)
        {
            fmpz_set_ui(a, t == 0 ? 9 : 81);
            ADF_CHECK(adf_root_padic_from_seed(L, f, place_of(3), a, 4) == ADF_OK);
            ADF_CHECK(adf_rootlist_get_fball(x, L, 0) == 1);
            adf_fball_get_fmpz3(A, H, d, x);
            ADF_CHECK(fmpz_is_zero(A) && fmpz_equal_ui(H, 81) && fmpz_is_one(d));
        }
    }
    adf_rootlist_clear(L);
    adf_fball_clear(x);
    fmpz_poly_clear(f);
    fmpz_clear(a);
    fmpz_clear(ap);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(q);
}

/* ---- 8. the entries verifier: every certificate passes, changed lists are refused ---- */

/* the claim of a list at a small prime, by enumeration: every ball holds exactly one root of g (p^s
   approximate roots modulo p^(K+s+3) in a + p^(s+1), all in a + p^K) and the balls are disjoint */
static int
list_claim_true(const adf_rootlist_t L, ulong p)
{
    fmpz_t c, q, fp, d;
    slong i, j, ps, t, cnt;
    int same, ok = 1;

    fmpz_init(c);
    fmpz_init(q);
    fmpz_init_set_ui(fp, p);
    fmpz_init(d);
    for (i = 0; i < L->n && ok; i++)
    {
        if (L->K[i] <= L->s[i] || L->s[i] < 0)
            ok = 0;
        else
        {
            fmpz_pow_ui(q, fp, L->s[i] + 1);
            fmpz_mod(c, L->a + i, q);
            cnt = oracle_enum(&same, L->g, p, c, L->s[i] + 1, L->K[i] + L->s[i] + 3, L->a + i, L->K[i], 200000);
            for (ps = 1, t = 0; t < L->s[i]; t++)
                ps *= (slong) p;
            ok = cnt == ps && same;
        }
        for (j = 0; j < i && ok; j++)
            if (cong(L->a + i, L->a + j, fp, FLINT_MIN(L->K[i], L->K[j])))
                ok = 0;
    }
    fmpz_clear(c);
    fmpz_clear(q);
    fmpz_clear(fp);
    fmpz_clear(d);
    return ok;
}

enum { CH_MOVED, CH_KUP, CH_SUP, CH_SDOWN, CH_G_IS_F, CH_TWO_BALLS, CH_UNREDUCED, CH_SEED_COMPLETE, CH_KINDS };

static const char * ch_names[CH_KINDS] = {
    "centre moved off the root (a + t p^(K-1))",
    "K raised by one, centre not lifted",
    "s raised by one",
    "s lowered by one",
    "g replaced by f (f != g*)",
    "two balls of one root (PARTITION, n = 2)",
    "centre not reduced (a + p^K)",
    "SEED list with complete = 1",
};

ADF_TEST(verifier_accepts_every_seed_list_and_refuses_changed_lists)
{
    static const ulong primes[4] = {2, 3, 5, 7};
    fmpz_poly_t F[18], g_saved;
    adf_rootlist_t L, L2;
    fmpz_t a, ap, fp, q, a_saved;
    slong tried[CH_KINDS] = {0}, refused[CH_KINDS] = {0}, acc_true[CH_KINDS] = {0}, acc_false[CH_KINDS] = {0};
    slong K, s, i, np, n_lists = 0, n_pass = 0, K_saved, s_saved;
    ulong p, ai, t;
    int kind, v;

    adf_rootlist_init(L);
    adf_rootlist_init(L2);
    fmpz_init(a);
    fmpz_init(ap);
    fmpz_init(fp);
    fmpz_init(q);
    fmpz_init(a_saved);
    fmpz_poly_init(g_saved);
    for (i = 0; i < 18; i++)
        fmpz_poly_init(F[i]);
    lcg_state = 77;
    for (int ip = 0; ip < 4; ip++)
    {
        p = primes[ip];
        fmpz_set_ui(fp, p);
        np = enum_polys(F, p);
        for (i = 0; i < np; i++)
            for (ai = 0; ai < p * p * p; ai++)
            {
                slong prec = 1 + (slong) (ai % 4);
                fmpz_set_ui(a, ai);
                if (adf_root_padic_from_seed(L, F[i], place_of(p), a, prec) != ADF_OK)
                    continue;
                n_lists++;
                n_pass += adf_rootlist_verify_entries(L, F[i]) == 1;
                adf_rootlist_get_cert(ap, &K, &s, L, 0);
                if (K + s + 3 > 12)
                    continue;                              /* the oracle of the truth works modulo p^(K+s+3) */
                fmpz_set(a_saved, L->a);
                K_saved = L->K[0];
                s_saved = L->s[0];
                for (kind = 0; kind < CH_KINDS; kind++)
                {
                    slong nt = kind == CH_MOVED ? (slong) p - 1 : 1;
                    for (t = 1; t <= (ulong) nt; t++)
                    {
                        int is_true = -1;
                        if (kind == CH_MOVED)
                        {
                            fmpz_pow_ui(q, fp, K - 1);
                            fmpz_addmul_ui(L->a, q, t);
                            fmpz_pow_ui(q, fp, K);
                            fmpz_mod(L->a, L->a, q);
                        }
                        else if (kind == CH_KUP)
                            L->K[0] = K + 1;
                        else if (kind == CH_SUP)
                            L->s[0] = s + 1;
                        else if (kind == CH_SDOWN)
                        {
                            if (s == 0)
                                continue;
                            L->s[0] = s - 1;
                        }
                        else if (kind == CH_G_IS_F)
                        {
                            if (fmpz_poly_equal(F[i], L->g))
                                continue;
                            fmpz_poly_set(g_saved, L->g);
                            fmpz_poly_set(L->g, F[i]);
                            is_true = 0;                   /* the list names another polynomial */
                        }
                        else if (kind == CH_TWO_BALLS)
                        {
                            /* L2: PARTITION, n = 2: the ball of L and the ball of the same seed at K + 1 */
                            fmpz_t ap2;
                            slong K2, s2;
                            fmpz_init(ap2);
                            if (adf_root_padic_from_seed(L2, F[i], place_of(p), a, K + 1) != ADF_OK)
                                abort();
                            adf_rootlist_get_cert(ap2, &K2, &s2, L2, 0);
                            fmpz_clear(L2->a);
                            flint_free(L2->a);
                            flint_free(L2->K);
                            flint_free(L2->s);
                            L2->a = _fmpz_vec_init(2);
                            L2->K = flint_malloc(2 * sizeof(slong));
                            L2->s = flint_malloc(2 * sizeof(slong));
                            fmpz_set(L2->a + 0, ap);
                            fmpz_set(L2->a + 1, ap2);
                            L2->K[0] = K;
                            L2->K[1] = K2;
                            L2->s[0] = s;
                            L2->s[1] = s2;
                            L2->n = 2;
                            L2->scope = ADF_ROOTLIST_PARTITION;
                            L2->complete = 1;
                            tried[kind]++;
                            v = adf_rootlist_verify_entries(L2, F[i]);
                            if (!v)
                                refused[kind]++;
                            else if (list_claim_true(L2, p))
                                acc_true[kind]++;
                            else
                                acc_false[kind]++;
                            fmpz_clear(ap2);
                            continue;
                        }
                        else if (kind == CH_UNREDUCED)
                        {
                            fmpz_pow_ui(q, fp, K);
                            fmpz_add(L->a, L->a, q);
                        }
                        else if (kind == CH_SEED_COMPLETE)
                        {
                            L->complete = 1;
                            is_true = 0;                   /* a SEED list claims no completeness */
                        }
                        tried[kind]++;
                        v = adf_rootlist_verify_entries(L, F[i]);
                        if (!v)
                            refused[kind]++;
                        else if (is_true != 0 && list_claim_true(L, p))
                            acc_true[kind]++;
                        else
                            acc_false[kind]++;
                        /* restore */
                        fmpz_set(L->a, a_saved);
                        L->K[0] = K_saved;
                        L->s[0] = s_saved;
                        L->complete = 0;
                        if (kind == CH_G_IS_F)
                            fmpz_poly_set(L->g, g_saved);
                    }
                }
                ADF_CHECK(adf_rootlist_verify_entries(L, F[i]) == 1);     /* restored */
            }
    }
    printf("entries verifier: %ld seed lists, %ld pass\n", (long) n_lists, (long) n_pass);
    ADF_CHECK(n_lists > 0 && n_pass == n_lists);
    for (kind = 0; kind < CH_KINDS; kind++)
    {
        printf("changed lists, %-44s: %5ld tried, %5ld refused, %4ld accepted and true, %ld accepted and false\n",
               ch_names[kind], (long) tried[kind], (long) refused[kind], (long) acc_true[kind],
               (long) acc_false[kind]);
        ADF_CHECK_MSG(refused[kind] > 0, "no list of the kind \"%s\" was refused", ch_names[kind]);
        ADF_CHECK_MSG(acc_false[kind] == 0, "a false list of the kind \"%s\" was accepted", ch_names[kind]);
    }
    for (i = 0; i < 18; i++)
        fmpz_poly_clear(F[i]);
    fmpz_poly_clear(g_saved);
    fmpz_clear(a);
    fmpz_clear(ap);
    fmpz_clear(fp);
    fmpz_clear(q);
    fmpz_clear(a_saved);
    adf_rootlist_clear(L);
    adf_rootlist_clear(L2);
}

/* a list at p with the given certificates, made by hand (arrays with flint_malloc, as roots.h says) */
static void
hand_list(adf_rootlist_t L, const fmpz_poly_t g, ulong p, int scope, int complete, const slong * a,
          const slong * K, const slong * s, slong n)
{
    slong i;

    adf_rootlist_clear(L);
    adf_rootlist_init(L);
    L->place = place_of(p);
    L->scope = scope;
    L->complete = complete;
    fmpz_poly_set(L->g, g);
    L->n = n;
    if (n > 0)
    {
        L->a = _fmpz_vec_init(n);
        L->K = flint_malloc(n * sizeof(slong));
        L->s = flint_malloc(n * sizeof(slong));
        for (i = 0; i < n; i++)
        {
            fmpz_set_si(L->a + i, a[i]);
            L->K[i] = K[i];
            L->s[i] = s[i];
        }
    }
}

ADF_TEST(verifier_and_predicate_on_hand_made_lists)
{
    static const slong c3[3] = {3, 0, 1};                 /* X^2 + 3 at 2 */
    static const slong cx[3] = {0, -1, 1};                /* X (X - 1) at 3 */
    static const slong c9[3] = {-9, 0, 1};                /* X^2 - 9 at 2 */
    fmpz_poly_t f;
    adf_rootlist_t L;

    fmpz_poly_init(f);
    adf_rootlist_init(L);

    /* (1, 1, 1) for X^2 + 3 at 2 satisfies (R2) and (R3) and has k = s; (R1) refuses it
       (solvers P3.2(6)); f has no root in Z_2 */
    poly_set_si(f, c3, 3);
    hand_list(L, f, 2, ADF_ROOTLIST_SEED, 0, (slong[]){1}, (slong[]){1}, (slong[]){1}, 1);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0);
    /* (1, 2, 1): (R3) needs v(f(1)) = v(4) >= 3: refused */
    hand_list(L, f, 2, ADF_ROOTLIST_SEED, 0, (slong[]){1}, (slong[]){2}, (slong[]){1}, 1);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 1);          /* (R1) holds: the predicate does not know f */

    /* solvers P3.13, example (a): X (X - 1) at 3, an empty PARTITION list with complete = 1 passes the
       entries verifier (which certifies no completeness) */
    poly_set_si(f, cx, 3);
    hand_list(L, f, 3, ADF_ROOTLIST_PARTITION, 1, NULL, NULL, NULL, 0);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 1);
    /* PARTITION with complete = 0 and nu = 0 is not consistent */
    L->complete = 0;
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0);
    /* the two roots 0 and 1, both listed: passes; the same ball twice: refused */
    hand_list(L, f, 3, ADF_ROOTLIST_PARTITION, 1, (slong[]){0, 1}, (slong[]){2, 2}, (slong[]){0, 0}, 2);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1 && adf_rootlist_is_canonical(L) == 1);
    hand_list(L, f, 3, ADF_ROOTLIST_PARTITION, 1, (slong[]){1, 1}, (slong[]){2, 3}, (slong[]){0, 0}, 2);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0 && adf_rootlist_is_canonical(L) == 0);
    /* centres not increasing: the predicate refuses, the entries verifier accepts (disjoint, true) */
    hand_list(L, f, 3, ADF_ROOTLIST_PARTITION, 1, (slong[]){1, 0}, (slong[]){2, 2}, (slong[]){0, 0}, 2);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1 && adf_rootlist_is_canonical(L) == 0);
    /* SEED with n = 2 is refused */
    hand_list(L, f, 3, ADF_ROOTLIST_SEED, 0, (slong[]){0, 1}, (slong[]){2, 2}, (slong[]){0, 0}, 2);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0 && adf_rootlist_is_canonical(L) == 0);
    /* g not normalised: 2 X (X - 1) */
    hand_list(L, f, 3, ADF_ROOTLIST_PARTITION, 1, (slong[]){0}, (slong[]){2}, (slong[]){0}, 1);
    fmpz_poly_scalar_mul_si(L->g, L->g, 2);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0);
    fmpz_poly_neg(L->g, f);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 0);
    /* a negative centre, K = s, s < 0: refused by both */
    hand_list(L, f, 3, ADF_ROOTLIST_SEED, 0, (slong[]){-3}, (slong[]){2}, (slong[]){0}, 1);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0 && adf_rootlist_is_canonical(L) == 0);
    hand_list(L, f, 3, ADF_ROOTLIST_SEED, 0, (slong[]){0}, (slong[]){2}, (slong[]){-1}, 1);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0 && adf_rootlist_is_canonical(L) == 0);
    /* a huge K with a small centre: the predicate does not form p^K (it would not return otherwise) */
    hand_list(L, f, 3, ADF_ROOTLIST_SEED, 0, (slong[]){0}, (slong[]){WORD_MAX}, (slong[]){0}, 1);
    ADF_CHECK(adf_rootlist_is_canonical(L) == 1);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0);     /* 2 K bits(p) above the limit: not tested */
    /* X^2 - 9 at 2: the two roots 3 and -3 = 13 mod 16 with s = 1 */
    poly_set_si(f, c9, 3);
    hand_list(L, f, 2, ADF_ROOTLIST_PARTITION, 1, (slong[]){3, 13}, (slong[]){4, 4}, (slong[]){1, 1}, 2);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1 && adf_rootlist_is_canonical(L) == 1);
    /* (3, 2, 1) and (1, 2, 1): disjoint balls modulo 4 */
    hand_list(L, f, 2, ADF_ROOTLIST_PARTITION, 1, (slong[]){1, 3}, (slong[]){2, 2}, (slong[]){1, 1}, 2);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 1);
    /* (1, 1, 1): K = s, refused although R2 and R3 might hold */
    hand_list(L, f, 2, ADF_ROOTLIST_PARTITION, 1, (slong[]){1}, (slong[]){1}, (slong[]){1}, 1);
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0);
    /* the real place: 0 in this slice */
    L->place = adf_place_inf();
    ADF_CHECK(adf_rootlist_verify_entries(L, f) == 0);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
}

/* ---- 9. the vectors of the reference ---- */

static int
read_fmpz(fmpz_t x, const jsonl_value * v)
{
    const char * t;

    if (!jsonl_int_text_or_string(v, &t, NULL))
        return 0;
    return fmpz_set_str(x, t, 10) == 0;
}

static int
read_poly(fmpz_poly_t f, const jsonl_value * v)
{
    fmpz_t c;
    size_t i;
    int ok = 1;

    fmpz_init(c);
    fmpz_poly_zero(f);
    for (i = 0; i < jsonl_size(v) && ok; i++)
    {
        ok = read_fmpz(c, jsonl_at(v, i, NULL));
        fmpz_poly_set_coeff_fmpz(f, (slong) i, c);
    }
    fmpz_clear(c);
    return ok;
}

ADF_TEST(vectors_s2_slice1_every_line)
{
    const char * path = "tests/ref/vectors/s2-slice1/seed.jsonl";
    jsonl_file * file;
    jsonl_error_t err;
    const jsonl_value * rec, * v;
    fmpz_poly_t f, g, gl;
    fmpz_t p, a, ca, ap;
    adf_rootlist_t L;
    size_t i, len;
    slong K, s, prec, cK, cs, n_ok = 0, n_nd = 0, n_dom = 0, n_bad = 0, n_bigp = 0;
    const char * st_text = NULL;
    int st, want, reduced;

    if (!jsonl_open(path, &file, &err))
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    fmpz_poly_init(f);
    fmpz_poly_init(g);
    fmpz_poly_init(gl);
    fmpz_init(p);
    fmpz_init(a);
    fmpz_init(ca);
    fmpz_init(ap);
    adf_rootlist_init(L);
    for (i = 0; i < jsonl_count(file); i++)
    {
        int ok = 1;
        rec = jsonl_record(file, i);
        ok = ok && jsonl_field(rec, "f", &v, &err) && read_poly(f, v);
        ok = ok && jsonl_field(rec, "p", &v, &err) && read_fmpz(p, v);
        ok = ok && jsonl_field(rec, "a", &v, &err) && read_fmpz(a, v);
        ok = ok && jsonl_field(rec, "prec", &v, &err) && read_fmpz(ca, v) && fmpz_fits_si(ca);
        prec = ok ? fmpz_get_si(ca) : 0;
        ok = ok && jsonl_field(rec, "status", &v, &err) && (st_text = jsonl_string(v, &len, &err)) != NULL;
        ADF_CHECK_MSG(ok, "line %lu: unreadable", (unsigned long) i + 1);
        if (!ok)
            break;
        want = strcmp(st_text, "OK") == 0 ? ADF_OK
             : strcmp(st_text, "NOT_DETERMINED") == 0 ? ADF_NOT_DETERMINED
             : strcmp(st_text, "DOMAIN") == 0 ? ADF_DOMAIN : -1;
        if (fmpz_bits(p) > 32)
            n_bigp++;
        st = adf_root_padic_from_seed(L, f, place_of(fmpz_get_ui(p)), a, prec);
        if (st != want)
        {
            n_bad++;
            ADF_CHECK_MSG(0, "line %lu: status %d, reference %s", (unsigned long) i + 1, st, st_text);
            continue;
        }
        if (st == ADF_DOMAIN)
        {
            n_dom++;
            continue;
        }
        if (st == ADF_NOT_DETERMINED)
        {
            n_nd++;
            continue;
        }
        n_ok++;
        ok = jsonl_field(rec, "g", &v, &err) && read_poly(g, v);
        ok = ok && jsonl_field(rec, "reduced", &v, &err) && read_fmpz(ca, v);
        reduced = ok ? (int) fmpz_get_si(ca) : -1;
        ok = ok && jsonl_field(rec, "cert", &v, &err) && jsonl_size(v) == 3 && read_fmpz(ca, jsonl_at(v, 0, NULL));
        ok = ok && read_fmpz(ap, jsonl_at(v, 1, NULL));
        cK = ok ? fmpz_get_si(ap) : -1;
        ok = ok && read_fmpz(ap, jsonl_at(v, 2, NULL));
        cs = ok ? fmpz_get_si(ap) : -1;
        adf_rootlist_get_poly(gl, L);
        adf_rootlist_get_cert(ap, &K, &s, L, 0);
        if (!ok || !fmpz_poly_equal(g, gl) || L->reduced != reduced || !fmpz_equal(ap, ca) || K != cK || s != cs ||
            adf_rootlist_scope(L) != ADF_ROOTLIST_SEED || adf_rootlist_length(L) != 1 ||
            adf_rootlist_is_complete(L) != 0 || adf_rootlist_verify_entries(L, f) != 1 ||
            !adf_rootlist_is_canonical(L))
        {
            n_bad++;
            ADF_CHECK_MSG(0, "line %lu: the list differs from the reference", (unsigned long) i + 1);
        }
    }
    printf("vectors: %lu lines, OK %ld, NOT_DETERMINED %ld, DOMAIN %ld, p above 32 bits %ld; %ld differ\n",
           (unsigned long) jsonl_count(file), (long) n_ok, (long) n_nd, (long) n_dom, (long) n_bigp, (long) n_bad);
    ADF_CHECK(jsonl_count(file) > 1000 && n_ok > 0 && n_nd > 0 && n_dom > 0 && n_bigp > 0);
    jsonl_close(file);
    adf_rootlist_clear(L);
    fmpz_poly_clear(f);
    fmpz_poly_clear(g);
    fmpz_poly_clear(gl);
    fmpz_clear(p);
    fmpz_clear(a);
    fmpz_clear(ca);
    fmpz_clear(ap);
}
