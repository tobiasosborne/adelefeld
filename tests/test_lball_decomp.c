/* tests/test_lball_decomp.c: the rest of work package 1F.3 (lane f-slice3; include/adelefeld/lball.h, section
   "slice 1F.3-b"; docs/api-1f.md L9 to L13): adf_lball_teichmuller, adf_lball_decompose_teich, adf_lball_frac,
   adf_lball_unit_mod, adf_lball_pow_si.

   Oracles, all written in this file and independent of the library code under test.
   1. Teichmueller: the limit of a^(p^n), computed modulo p^N by fmpz_powm with the exponent p^N (the library uses
      Newton's method on T^(p-1) - 1); the residue, w^(p-1) = 1 and the residue of w are checked as well.
   2. Decomposition: for every ball p^m (u + p^k Z_p) of a universe (p = 2, 3, 5, 7 exhaustive at small k, p = 11 and
      2^64 - 59 sampled), for a sample of the points a = u + p^k t of the unit ball: the brute-force split
      a = w_bf u_bf (w_bf as in 1., u_bf = a / w_bf modulo p^L) lies in the returned factors; the p classes of the
      u_bf modulo p^(k + 1) are all met (the returned ball is not too large); the product ball w * u (library
      multiplication) contains the unit ball and, for a precision of w at least k, equals it.
   3. Fractional part: the definition of Proposition 19 (0 <= r < 1, denominator a power of p, x - r in Z_p) on
      exact values and on every point of a sample of every ball; constancy on the ball for N >= 0 and, for N < 0, two
      points of the ball with different fractional parts.
   4. Unit modulo p^k: the congruences b out = a (exact), out = u (ball), 0 <= out < p^k.
   5. Powers: for every ball and every k of a list, the exact rationals s^k of a sample of points s lie in the
      result, and the exponent of the result equals the smallest valuation of a difference of two sample values
      (tightness; the sample contains the points that attain it); exact values against fmpq_pow_si; k = 0;
      statuses; limits; aliasing.
   What would make a case fail is stated at each test. */

#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/fmpq.h>
#include <flint/ulong_extras.h>
#include <flint/fmpz.h>
#include "support/jsonl.h"
#include "test_runner.h"

/* ---------------------------------------------------------------------------------------------------- helpers */

static adf_place_t
place_of(ulong p)
{
    adf_place_t v;
    int st = adf_place_prime(&v, p);
    ADF_CHECK(st == ADF_OK);
    return v;
}

static void
sentinel(adf_lball_t x)
{
    x->p = 1000003;
    fmpz_set_si(fmpq_numref(x->u), 7);
    fmpz_one(fmpq_denref(x->u));
    x->v = 2;
    x->N = 5;
    x->exact = 0;
}

static int
is_sentinel(const adf_lball_t x)
{
    return x->p == 1000003 && fmpz_equal_si(fmpq_numref(x->u), 7) && fmpz_is_one(fmpq_denref(x->u)) && x->v == 2 &&
           x->N == 5 && x->exact == 0;
}

/* v_p of a nonzero rational. */
static slong
val_q(const fmpq_t q, ulong p)
{
    fmpz_t P, r;
    slong a, b;
    fmpz_init_set_ui(P, p);
    fmpz_init(r);
    a = fmpz_remove(r, fmpq_numref(q), P);
    b = fmpz_remove(r, fmpq_denref(q), P);
    fmpz_clear(P);
    fmpz_clear(r);
    return a - b;
}

static void
ppow_q(fmpq_t out, ulong p, slong e)
{
    fmpz_t pp;
    fmpz_init_set_ui(pp, p);
    if (e >= 0)
    {
        fmpz_pow_ui(fmpq_numref(out), pp, (ulong) e);
        fmpz_one(fmpq_denref(out));
    }
    else
    {
        fmpz_one(fmpq_numref(out));
        fmpz_pow_ui(fmpq_denref(out), pp, (ulong) -e);
    }
    fmpz_clear(pp);
}

/* the value p^v u of the fields, an fmpq */
static void
value_of(fmpq_t c, const adf_lball_t x)
{
    fmpq_t t;
    fmpq_init(t);
    ppow_q(t, x->p, x->v);
    fmpq_mul(c, x->u, t);
    fmpq_clear(t);
}

/* s in the set x, decided by the definition: exact: equal; ball: v_p(s - centre) >= N (or s = centre). */
static int
in_set(const fmpq_t s, const adf_lball_t x)
{
    fmpq_t c, d;
    int r;
    fmpq_init(c);
    fmpq_init(d);
    value_of(c, x);
    fmpq_sub(d, s, c);
    r = fmpq_is_zero(d) || (!x->exact && val_q(d, x->p) >= x->N);
    fmpq_clear(c);
    fmpq_clear(d);
    return r;
}

static void
ball_of(adf_lball_t z, ulong p, const fmpq_t r, slong N)
{
    adf_rat_t q;
    int st;
    adf_rat_init(q);
    fmpq_set(q->q, r);
    st = adf_lball_set_rat_ball(z, place_of(p), q, N);
    ADF_CHECK(st == ADF_OK);
    adf_rat_clear(q);
}

static void
exact_of(adf_lball_t z, ulong p, const fmpq_t r)
{
    adf_rat_t q;
    int st;
    adf_rat_init(q);
    fmpq_set(q->q, r);
    st = adf_lball_set_rat(z, place_of(p), q);
    ADF_CHECK(st == ADF_OK);
    adf_rat_clear(q);
}

/* x = p^v0 (u + p^k Z_p), u an integer prime to p, as a library ball */
static void
unit_ball(adf_lball_t x, ulong p, slong v0, ulong u, slong k)
{
    fmpq_t c, t;
    fmpq_init(c);
    fmpq_init(t);
    ppow_q(t, p, v0);
    fmpq_set_ui(c, u, 1);
    fmpq_mul(c, c, t);
    ball_of(x, p, c, v0 + k);
    fmpq_clear(c);
    fmpq_clear(t);
}

/* w = the brute-force Teichmueller representative of the residue r at the odd prime p, modulo p^L:
   r^(p^L) modulo p^L (the limit of r^(p^n)). */
static void
bf_teich(fmpz_t w, ulong p, ulong r, ulong L)
{
    fmpz_t P, e, R;
    fmpz_init_set_ui(P, p);
    fmpz_pow_ui(P, P, L);
    fmpz_init_set(e, P);
    fmpz_init_set_ui(R, r);
    fmpz_powm(w, R, e, P);
    fmpz_clear(P);
    fmpz_clear(e);
    fmpz_clear(R);
}

/* the sign factor at p = 2 modulo 2^L: 1 if the unit is 1 mod 4, else -1 */
static void
bf_sign(fmpz_t w, ulong a4, ulong L)
{
    if (a4 == 1)
        fmpz_one(w);
    else
    {
        fmpz_one(w);
        fmpz_mul_2exp(w, w, L);
        fmpz_sub_ui(w, w, 1);
    }
}

static void
pk_of(fmpz_t P, ulong p, ulong k)
{
    fmpz_set_ui(P, p);
    fmpz_pow_ui(P, P, k);
}

/* 1 if the exact or ball value w (unit, v = 0) is congruent to the integer z modulo p^n, n = its precision, or
   for an exact w if its value (a rational with denominator prime to p) equals z modulo p^L. */
static int
factor_matches(const adf_lball_t w, const fmpz_t z, ulong L)
{
    fmpz_t P, a, b;
    int r;
    fmpz_init(P);
    fmpz_init(a);
    fmpz_init(b);
    if (w->exact)
    {
        pk_of(P, w->p, L);
        if (!fmpz_invmod(b, fmpq_denref(w->u), P))
            r = 0;
        else
        {
            fmpz_mul(a, fmpq_numref(w->u), b);
            fmpz_sub(a, a, z);
            r = fmpz_divisible(a, P);
        }
    }
    else
    {
        pk_of(P, w->p, (ulong) w->N);
        fmpz_mod(a, z, P);
        r = fmpz_equal(a, fmpq_numref(w->u)) && fmpz_is_one(fmpq_denref(w->u));
    }
    fmpz_clear(P);
    fmpz_clear(a);
    fmpz_clear(b);
    return r;
}

/* ---------------------------------------------------------------------------------- Teichmueller representative */

static void
check_teich_one(ulong p, ulong r, slong prec)
{
    adf_lball_t w;
    fmpz_t bf, P, t;
    slong n = prec < 1 ? 1 : prec;
    int st, rational = (p == 2 || r % p == 1 || r % p == p - 1);
    adf_lball_init(w);
    fmpz_init(bf);
    fmpz_init(P);
    fmpz_init(t);
    sentinel(w);
    st = adf_lball_teichmuller(w, place_of(p), r, prec);
    ADF_CHECK_MSG(st == ADF_OK, "p=%lu r=%lu prec=%ld: status %s", p, r, (long) prec, adf_status_str(st));
    if (st == ADF_OK)
    {
        ADF_CHECK(adf_lball_is_canonical(w));
        ADF_CHECK_MSG((w->exact != 0) == rational, "p=%lu r=%lu: exact = %d, rational = %d", p, r, w->exact, rational);
        if (p == 2)
        {
            ADF_CHECK(w->exact && fmpq_is_one(w->u) && w->v == 0);
        }
        else
        {
            bf_teich(bf, p, r % p, (ulong) n + 2);
            ADF_CHECK_MSG(factor_matches(w, bf, (ulong) n + 2), "p=%lu r=%lu prec=%ld: not the limit of r^(p^n)", p, r,
                          (long) n);
            if (!w->exact)
            {
                /* the ball: precision n, unit, w^(p-1) = 1 modulo p^n, w = r modulo p */
                ADF_CHECK(w->N == n && w->v == 0);
                pk_of(P, p, (ulong) n);
                fmpz_powm_ui(t, fmpq_numref(w->u), p - 1, P);
                ADF_CHECK(fmpz_is_one(t) || n == 0);
                ADF_CHECK(fmpz_fdiv_ui(fmpq_numref(w->u), p) == r % p);
            }
            else
            {
                /* exact: +1 or -1 */
                ADF_CHECK((fmpq_is_one(w->u) && r % p == 1) || (fmpz_equal_si(fmpq_numref(w->u), -1) && r % p == p - 1));
            }
        }
    }
    fmpz_clear(bf);
    fmpz_clear(P);
    fmpz_clear(t);
    adf_lball_clear(w);
}

ADF_TEST(teichmuller_against_the_limit_of_powers)
{
    ulong ps[5] = {2, 3, 5, 7, 11};
    slong precs[8] = {-3, 0, 1, 2, 3, 5, 9, 40};
    int i, j;
    ulong r;
    for (i = 0; i < 5; i++)
        for (r = 1; r < ps[i] + 3 * ps[i]; r++)
        {
            if (r % ps[i] == 0)
                continue;
            for (j = 0; j < 8; j++)
                check_teich_one(ps[i], r, precs[j]);
        }
    /* the big prime and a long precision */
    check_teich_one(18446744073709551557ul, 2, 3);
    check_teich_one(18446744073709551557ul, 18446744073709551556ul, 3);
    check_teich_one(18446744073709551557ul, 123456789, 5);
    check_teich_one(5, 2, 300);
    check_teich_one(5, 3, 1000);
}

ADF_TEST(teichmuller_statuses)
{
    adf_lball_t w;
    adf_lball_init(w);
    sentinel(w);
    ADF_CHECK(adf_lball_teichmuller(w, adf_place_inf(), 2, 5) == ADF_DOMAIN && is_sentinel(w));
    ADF_CHECK(adf_lball_teichmuller(w, place_of(5), 0, 5) == ADF_DOMAIN && is_sentinel(w));
    ADF_CHECK(adf_lball_teichmuller(w, place_of(5), 10, 5) == ADF_DOMAIN && is_sentinel(w));
    ADF_CHECK(adf_lball_teichmuller(w, place_of(2), 4, 5) == ADF_DOMAIN && is_sentinel(w));
    /* the ball needs p^n: n bits(p) above the bit bound is LIMIT, before any allocation that grows */
    ADF_CHECK(adf_lball_teichmuller(w, place_of(5), 2, ADF_LBALL_BITS_MAX / 3 + 1) == ADF_LIMIT && is_sentinel(w));
    ADF_CHECK(adf_lball_teichmuller(w, place_of(5), 2, LONG_MAX) == ADF_LIMIT && is_sentinel(w));
    /* an exact factor has no precision: no limit */
    ADF_CHECK(adf_lball_teichmuller(w, place_of(5), 4, LONG_MAX) == ADF_OK && w->exact && fmpz_equal_si(fmpq_numref(w->u), -1));
    ADF_CHECK(adf_lball_teichmuller(w, place_of(2), 3, LONG_MAX) == ADF_OK && w->exact && fmpq_is_one(w->u));
    adf_lball_clear(w);
}

/* ---------------------------------------------------------------------------------------------- decomposition */

/* One ball x = p^m (u + p^k Z_p) (a unit ball U = x / p^m of relative precision k) and one precision. */
static void
check_split_ball(ulong p, slong m, ulong u, slong k, slong prec)
{
    adf_lball_t x, w, uu, U, prod;
    slong mm = 12345;
    ulong index = 999;
    fmpz_t bfw, bfu, a, P, Pk1, inv, L1;
    fmpz_t seen[16];
    int st, t, nseen = 0, i;
    ulong L;
    slong n = prec < 1 ? 1 : prec;

    adf_lball_init(x);
    adf_lball_init(w);
    adf_lball_init(uu);
    adf_lball_init(U);
    adf_lball_init(prod);
    fmpz_init(bfw);
    fmpz_init(bfu);
    fmpz_init(a);
    fmpz_init(P);
    fmpz_init(Pk1);
    fmpz_init(inv);
    fmpz_init(L1);
    for (i = 0; i < 16; i++)
        fmpz_init(seen[i]);
    unit_ball(x, p, m, u, k);
    sentinel(w);
    sentinel(uu);
    st = adf_lball_decompose_teich(&mm, w, &index, uu, x, prec);
    if (p == 2 && k == 1)
    {
        ADF_CHECK_MSG(st == ADF_NOT_DETERMINED && is_sentinel(w) && is_sentinel(uu) && mm == 12345 && index == 999,
                      "p=2 k=1 m=%ld u=%lu: status %s", (long) m, u, adf_status_str(st));
        goto done;
    }
    ADF_CHECK_MSG(st == ADF_OK, "p=%lu m=%ld u=%lu k=%ld prec=%ld: status %s", p, (long) m, u, (long) k, (long) prec,
                  adf_status_str(st));
    if (st != ADF_OK)
        goto done;
    ADF_CHECK(mm == m);
    ADF_CHECK(adf_lball_is_canonical(w) && adf_lball_is_canonical(uu));
    ADF_CHECK(uu->v == 0 && !uu->exact && uu->N == k);
    L = (ulong) (k > n ? k : n) + 2;
    pk_of(P, p, L);
    for (t = 0; t < (int) p + 2; t++)
    {
        /* the point a = u + p^k t of the unit ball */
        ulong ra;
        fmpz_set_ui(a, p);
        fmpz_pow_ui(a, a, (ulong) k);
        fmpz_mul_ui(a, a, (ulong) t);
        fmpz_add_ui(a, a, u);
        if (p == 2)
        {
            ra = fmpz_fdiv_ui(a, 4);
            bf_sign(bfw, ra, L);
        }
        else
        {
            ra = fmpz_fdiv_ui(a, p);
            bf_teich(bfw, p, ra, L);
        }
        ADF_CHECK_MSG(index == ra, "p=%lu u=%lu k=%ld t=%d: index %lu, brute force %lu", p, u, (long) k, t, index, ra);
        ADF_CHECK_MSG(factor_matches(w, bfw, L), "p=%lu u=%lu k=%ld prec=%ld t=%d: w is not the brute-force factor", p, u,
                      (long) k, (long) prec, t);
        ADF_CHECK(fmpz_invmod(inv, bfw, P));
        fmpz_mul(bfu, a, inv);
        fmpz_mod(bfu, bfu, P);
        /* the principal unit lies in the ball uu: same residue modulo p^k */
        {
            fmpz_t Pk, d;
            fmpz_init(Pk);
            fmpz_init(d);
            pk_of(Pk, p, (ulong) k);
            fmpz_sub(d, bfu, fmpq_numref(uu->u));
            ADF_CHECK_MSG(fmpz_divisible(d, Pk), "p=%lu u=%lu k=%ld prec=%ld t=%d: principal unit outside the ball", p, u,
                          (long) k, (long) prec, t);
            fmpz_clear(Pk);
            fmpz_clear(d);
        }
        /* it is a principal unit: 1 modulo p (modulo 4 at p = 2) */
        ADF_CHECK(fmpz_fdiv_ui(bfu, p == 2 ? 4 : p) == 1);
        /* tightness: the classes modulo p^(k + 1) of the t = 0 .. p - 1 */
        if (t < (int) p)
        {
            int dup = 0;
            pk_of(Pk1, p, (ulong) k + 1);
            fmpz_mod(L1, bfu, Pk1);
            for (i = 0; i < nseen; i++)
                if (fmpz_equal(seen[i], L1))
                    dup = 1;
            ADF_CHECK(!dup);
            if (nseen < 16)
                fmpz_set(seen[nseen++], L1);
        }
    }
    ADF_CHECK(nseen == (int) p || nseen == 16);
    /* the product ball contains the unit ball; with a precision of w at least k it equals it */
    {
        int s2 = adf_lball_decompose(&mm, U, x);
        ADF_CHECK(s2 == ADF_OK);
        s2 = adf_lball_mul(prod, w, uu);
        ADF_CHECK(s2 == ADF_OK);
        ADF_CHECK(adf_lball_contains(U, prod));
        if (n >= k || w->exact)
            ADF_CHECK_MSG(adf_lball_equal_set(U, prod), "p=%lu u=%lu k=%ld prec=%ld: w u is not the unit ball", p, u,
                          (long) k, (long) prec);
    }
    /* aliasing: w = x, u = x give the results of the separate call */
    {
        adf_lball_t xa;
        slong m2;
        ulong i2;
        adf_lball_init(xa);
        adf_lball_set(xa, x);
        {
            adf_lball_t w2, u2;
            adf_lball_init(w2);
            adf_lball_init(u2);
            ADF_CHECK(adf_lball_decompose_teich(&m2, w2, &i2, u2, x, prec) == ADF_OK);
            ADF_CHECK(adf_lball_decompose_teich(&m2, xa, &i2, u2, xa, prec) == ADF_OK && adf_lball_identical(xa, w2));
            adf_lball_set(xa, x);
            ADF_CHECK(adf_lball_decompose_teich(&m2, w2, &i2, xa, xa, prec) == ADF_OK && adf_lball_identical(xa, u2));
            adf_lball_clear(w2);
            adf_lball_clear(u2);
        }
        adf_lball_clear(xa);
    }
done:
    for (i = 0; i < 16; i++)
        fmpz_clear(seen[i]);
    adf_lball_clear(x);
    adf_lball_clear(w);
    adf_lball_clear(uu);
    adf_lball_clear(U);
    adf_lball_clear(prod);
    fmpz_clear(bfw);
    fmpz_clear(bfu);
    fmpz_clear(a);
    fmpz_clear(P);
    fmpz_clear(Pk1);
    fmpz_clear(inv);
    fmpz_clear(L1);
}

static void
split_universe(ulong p, slong kmax, ulong ustep)
{
    slong m, k;
    ulong u, pk;
    slong precs[3] = {1, 3, 7};
    int j;
    for (m = -2; m <= 2; m += (p > 5 ? 2 : 1))
        for (k = 1; k <= kmax; k++)
        {
            pk = 1;
            for (u = 0; u < (ulong) k; u++)
                pk *= p;
            for (u = 1; u < pk; u += ustep)
            {
                if (u % p == 0)
                    continue;
                for (j = 0; j < 3; j++)
                    check_split_ball(p, m, u, k, precs[j]);
            }
        }
}

ADF_TEST(decompose_teich_p2)
{
    split_universe(2, 5, 1);
}
ADF_TEST(decompose_teich_p3)
{
    split_universe(3, 3, 1);
}
ADF_TEST(decompose_teich_p5)
{
    split_universe(5, 2, 1);
    split_universe(5, 3, 7);
}
ADF_TEST(decompose_teich_p7)
{
    split_universe(7, 2, 1);
}
ADF_TEST(decompose_teich_p11)
{
    split_universe(11, 2, 5);
}

ADF_TEST(decompose_teich_the_big_prime)
{
    ulong p = 18446744073709551557ul;
    adf_lball_t x, w, u, prod;
    fmpz_t bf, P, inv, a;
    slong m;
    ulong index;
    fmpq_t c;
    adf_lball_init(x);
    adf_lball_init(w);
    adf_lball_init(u);
    adf_lball_init(prod);
    fmpz_init(bf);
    fmpz_init(P);
    fmpz_init(inv);
    fmpz_init(a);
    fmpq_init(c);
    /* x = p^-1 (12345678901234567 + p^2 Z_p) */
    unit_ball(x, p, -1, 12345678901234567ul, 2);
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, 4) == ADF_OK);
    ADF_CHECK(m == -1 && index == 12345678901234567ul);
    bf_teich(bf, p, index, 6);
    ADF_CHECK(factor_matches(w, bf, 6) && w->N == 4);
    pk_of(P, p, 2);
    ADF_CHECK(fmpz_invmod(inv, bf, P));
    fmpz_set_ui(a, 12345678901234567ul);
    fmpz_mul(a, a, inv);
    fmpz_mod(a, a, P);
    ADF_CHECK(fmpz_equal(a, fmpq_numref(u->u)) && u->N == 2 && u->v == 0);
    ADF_CHECK(adf_lball_mul(prod, w, u) == ADF_OK);
    ADF_CHECK(adf_lball_decompose(&m, x, x) == ADF_OK && adf_lball_contains(x, prod));
    adf_lball_clear(x);
    adf_lball_clear(w);
    adf_lball_clear(u);
    adf_lball_clear(prod);
    fmpz_clear(bf);
    fmpz_clear(P);
    fmpz_clear(inv);
    fmpz_clear(a);
    fmpq_clear(c);
}

/* exact values: x = p^m a/b */
static void
check_split_exact(ulong p, slong m, slong a, slong b, slong prec)
{
    adf_lball_t x, w, u, prod;
    fmpq_t q, t, pm;
    fmpz_t bfw, P, ib, r, A;
    slong mm = 777;
    ulong index = 555, ra;
    slong n = prec < 1 ? 1 : prec;
    ulong L = (ulong) n + 3;
    int st;
    adf_lball_init(x);
    adf_lball_init(w);
    adf_lball_init(u);
    adf_lball_init(prod);
    fmpq_init(q);
    fmpq_init(t);
    fmpq_init(pm);
    fmpz_init(bfw);
    fmpz_init(P);
    fmpz_init(ib);
    fmpz_init(r);
    fmpz_init(A);
    fmpq_set_si(t, a, (ulong) b);
    ppow_q(pm, p, m);
    fmpq_mul(q, t, pm);
    exact_of(x, p, q);
    sentinel(w);
    sentinel(u);
    st = adf_lball_decompose_teich(&mm, w, &index, u, x, prec);
    ADF_CHECK_MSG(st == ADF_OK, "exact p=%lu m=%ld %ld/%ld: %s", p, (long) m, (long) a, (long) b, adf_status_str(st));
    if (st == ADF_OK)
    {
        pk_of(P, p, L);
        ADF_CHECK(mm == m);
        fmpz_set_si(r, b);
        ADF_CHECK(fmpz_invmod(ib, r, P));
        fmpz_set_si(A, a);
        fmpz_mul(A, A, ib);
        fmpz_mod(A, A, P);                                  /* the unit t modulo p^L */
        ra = fmpz_fdiv_ui(A, p == 2 ? 4 : p);
        ADF_CHECK(index == ra);
        if (p == 2)
            bf_sign(bfw, ra, L);
        else
            bf_teich(bfw, p, ra, L);
        ADF_CHECK(factor_matches(w, bfw, L));
        ADF_CHECK(adf_lball_is_canonical(w) && adf_lball_is_canonical(u));
        if (w->exact)
        {
            /* u is the exact rational t / w */
            fmpq_t e;
            fmpq_init(e);
            fmpq_div(e, t, w->u);
            ADF_CHECK(u->exact && fmpq_equal(u->u, e) && u->v == 0);
            fmpq_clear(e);
        }
        else
        {
            ADF_CHECK(!u->exact && u->N == n && u->v == 0);
            ADF_CHECK(adf_lball_mul(prod, w, u) == ADF_OK && in_set(t, prod));
            ADF_CHECK(prod->N == n && prod->v == 0);
        }
        /* principal unit */
        {
            fmpz_t d, uc, ub;
            fmpz_init(d);
            fmpz_init(uc);
            fmpz_init(ub);
            fmpz_invmod(uc, bfw, P);
            fmpz_mul(uc, uc, A);
            fmpz_mod(uc, uc, P);
            /* u modulo p^n equals A / w */
            if (u->exact)
            {
                fmpz_invmod(ub, fmpq_denref(u->u), P);
                fmpz_mul(ub, ub, fmpq_numref(u->u));
                fmpz_mod(ub, ub, P);
                fmpz_sub(d, ub, uc);
                ADF_CHECK(fmpz_divisible(d, P));
            }
            else
            {
                fmpz_t Pn;
                fmpz_init(Pn);
                pk_of(Pn, p, (ulong) n);
                fmpz_sub(d, fmpq_numref(u->u), uc);
                ADF_CHECK(fmpz_divisible(d, Pn));
                fmpz_clear(Pn);
            }
            ADF_CHECK(fmpz_fdiv_ui(uc, p == 2 ? 4 : p) == 1);
            fmpz_clear(d);
            fmpz_clear(uc);
            fmpz_clear(ub);
        }
    }
    adf_lball_clear(x);
    adf_lball_clear(w);
    adf_lball_clear(u);
    adf_lball_clear(prod);
    fmpq_clear(q);
    fmpq_clear(t);
    fmpq_clear(pm);
    fmpz_clear(bfw);
    fmpz_clear(P);
    fmpz_clear(ib);
    fmpz_clear(r);
    fmpz_clear(A);
}

ADF_TEST(decompose_teich_exact_values)
{
    ulong ps[5] = {2, 3, 5, 7, 11};
    slong ms[3] = {-2, 0, 3};
    slong precs[3] = {1, 4, 9};
    int i, j, l;
    slong a, b;
    for (i = 0; i < 5; i++)
        for (a = -13; a <= 13; a++)
            for (b = 1; b <= 9; b++)
            {
                if (a % (slong) ps[i] == 0 || b % (slong) ps[i] == 0 || n_gcd((ulong) (a < 0 ? -a : a), (ulong) b) != 1)
                    continue;
                for (j = 0; j < 3; j++)
                    for (l = 0; l < 3; l++)
                        check_split_exact(ps[i], ms[j], a, b, precs[l]);
            }
}

ADF_TEST(decompose_teich_statuses_and_limits)
{
    adf_lball_t x, w, u;
    fmpq_t q;
    slong m = 41;
    ulong index = 43;
    slong E = ADF_LBALL_EXP_MAX;
    int st;
    adf_lball_init(x);
    adf_lball_init(w);
    adf_lball_init(u);
    fmpq_init(q);
    /* exact 0: DOMAIN */
    fmpq_zero(q);
    exact_of(x, 5, q);
    sentinel(w);
    sentinel(u);
    st = adf_lball_decompose_teich(&m, w, &index, u, x, 5);
    ADF_CHECK(st == ADF_DOMAIN && is_sentinel(w) && is_sentinel(u) && m == 41 && index == 43);
    st = adf_lball_decompose_teich(&m, w, &index, u, x, -5);
    ADF_CHECK(st == ADF_DOMAIN);
    /* the ball around 0: NOT_DETERMINED, at p = 2 as well */
    ball_of(x, 5, q, 4);
    st = adf_lball_decompose_teich(&m, w, &index, u, x, 5);
    ADF_CHECK(st == ADF_NOT_DETERMINED && is_sentinel(w) && is_sentinel(u) && m == 41 && index == 43);
    ball_of(x, 2, q, -3);
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, 5) == ADF_NOT_DETERMINED && is_sentinel(w));
    /* p = 2: the sign is not determined at k = 1 (the points 1 and 3 modulo 4), determined at k = 2; an exact
       value is always determined */
    fmpq_set_si(q, 3, 1);
    ball_of(x, 2, q, 1);
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, 5) == ADF_NOT_DETERMINED && is_sentinel(w));
    fmpq_set_si(q, 3, 8);
    ball_of(x, 2, q, -2);       /* 2^-3 (3 + 2 Z_2) */
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, 5) == ADF_NOT_DETERMINED && is_sentinel(w));
    ball_of(x, 2, q, -1);       /* 2^-3 (3 + 4 Z_2) */
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, 5) == ADF_OK && m == -3 && index == 3 &&
              w->exact && fmpz_equal_si(fmpq_numref(w->u), -1) && u->N == 2 && fmpz_equal_si(fmpq_numref(u->u), 1));
    fmpq_set_si(q, 3, 1);
    exact_of(x, 2, q);
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, 5) == ADF_OK && m == 0 && index == 3 && u->exact &&
              fmpz_equal_si(fmpq_numref(u->u), -3));
    /* limits: an input exponent, N - m, the size of the result. x = 1 + O(5^(2^40)): w = omega(1) = 1 is exact and
       the unit centre 1 is small, so the split is fine; r = 2 needs 5^(2^40): LIMIT, outputs untouched. */
    fmpq_set_si(q, 1, 1);
    ball_of(x, 5, q, 1L << 40);
    sentinel(w);
    sentinel(u);
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, 5) == ADF_OK && w->exact && fmpq_is_one(w->u) &&
              u->N == (1L << 40) && fmpq_is_one(u->u) && index == 1);
    fmpq_set_si(q, 2, 1);
    ball_of(x, 5, q, 1L << 40);
    sentinel(w);
    sentinel(u);
    m = 41;
    index = 43;
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, 5) == ADF_LIMIT && is_sentinel(w) && is_sentinel(u) &&
              m == 41 && index == 43);
    /* precision too large for the Teichmueller ball, small unit ball */
    fmpq_set_si(q, 2, 1);
    ball_of(x, 5, q, 3);
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, ADF_LBALL_BITS_MAX) == ADF_LIMIT && is_sentinel(w));
    /* N - m above the exponent bound: x = 1 + O(5^E) at valuation -E has N - v = 2 E */
    fmpq_set_si(q, 1, 1);
    ball_of(x, 5, q, 1);
    x->v = -E;
    x->N = E;
    ADF_CHECK(adf_lball_is_canonical(x));
    ADF_CHECK(adf_lball_decompose_teich(&m, w, &index, u, x, 5) == ADF_LIMIT && is_sentinel(w));
    fmpq_clear(q);
    adf_lball_clear(x);
    adf_lball_clear(w);
    adf_lball_clear(u);
}

/* ------------------------------------------------------------------------------------------ fractional part */

/* the definition of Proposition 19 for r and the exact value s. */
static int
frac_def_ok(const fmpq_t r, const fmpq_t s, ulong p)
{
    fmpq_t d;
    fmpz_t P, rest;
    int ok;
    fmpq_init(d);
    fmpz_init_set_ui(P, p);
    fmpz_init(rest);
    ok = fmpq_sgn(r) >= 0 && fmpz_cmp(fmpq_numref(r), fmpq_denref(r)) < 0;
    fmpz_remove(rest, fmpq_denref(r), P);
    ok = ok && fmpz_is_one(rest);
    fmpq_sub(d, s, r);
    ok = ok && (fmpq_is_zero(d) || val_q(d, p) >= 0);
    fmpq_clear(d);
    fmpz_clear(P);
    fmpz_clear(rest);
    return ok;
}

ADF_TEST(frac_of_exact_values)
{
    ulong ps[4] = {2, 3, 5, 18446744073709551557ul};
    int i, e;
    slong a, b;
    adf_lball_t x;
    adf_rat_t r;
    fmpq_t s, pe;
    adf_lball_init(x);
    adf_rat_init(r);
    fmpq_init(s);
    fmpq_init(pe);
    for (i = 0; i < 4; i++)
        for (e = -2; e <= 4; e++)
            for (a = -40; a <= 40; a += (i == 3 ? 13 : 1))
                for (b = 1; b <= 7; b += 2)
                {
                    int st;
                    fmpq_set_si(s, a, (ulong) b);
                    ppow_q(pe, ps[i], -e);              /* denominator p^e for e >= 0, numerator p^-e otherwise */
                    fmpq_mul(s, s, pe);
                    exact_of(x, ps[i], s);
                    fmpq_set_si(r->q, 1234567, 1);
                    st = adf_lball_frac(r, x);
                    ADF_CHECK_MSG(st == ADF_OK, "frac status %s", adf_status_str(st));
                    ADF_CHECK_MSG(frac_def_ok(r->q, s, ps[i]), "p=%lu s=%ld/%ld p^-%d: frac wrong", ps[i], (long) a,
                                  (long) b, e);
                }
    /* the examples of Proposition 19: 1/3 at 2 is integral (fraction 0), 7/25 at 5 is 7/25, -1/4 at 2 is 3/4 */
    fmpq_set_si(s, 1, 3);
    exact_of(x, 2, s);
    ADF_CHECK(adf_lball_frac(r, x) == ADF_OK && fmpq_is_zero(r->q));
    fmpq_set_si(s, 7, 25);
    exact_of(x, 5, s);
    ADF_CHECK(adf_lball_frac(r, x) == ADF_OK && fmpz_equal_si(fmpq_numref(r->q), 7) && fmpz_equal_si(fmpq_denref(r->q), 25));
    fmpq_set_si(s, -1, 4);
    exact_of(x, 2, s);
    ADF_CHECK(adf_lball_frac(r, x) == ADF_OK && fmpz_equal_si(fmpq_numref(r->q), 3) && fmpz_equal_si(fmpq_denref(r->q), 4));
    fmpq_zero(s);
    exact_of(x, 2, s);
    fmpq_set_si(r->q, 5, 3);
    ADF_CHECK(adf_lball_frac(r, x) == ADF_OK && fmpq_is_zero(r->q));
    fmpq_clear(s);
    fmpq_clear(pe);
    adf_lball_clear(x);
    adf_rat_clear(r);
}

static void
check_frac_ball(ulong p, slong v, ulong u, slong N)
{
    adf_lball_t x, y;
    adf_rat_t r, r0;
    fmpq_t c, pn, s;
    int st, t;
    adf_lball_init(x);
    adf_lball_init(y);
    adf_rat_init(r);
    adf_rat_init(r0);
    fmpq_init(c);
    fmpq_init(pn);
    fmpq_init(s);
    ppow_q(c, p, v);
    fmpq_mul_ui(c, c, u);
    ball_of(x, p, c, N);
    fmpq_set_si(r->q, 999, 1);
    st = adf_lball_frac(r, x);
    if (N >= 0)
    {
        ADF_CHECK_MSG(st == ADF_OK, "p=%lu v=%ld u=%lu N=%ld: %s", p, (long) v, u, (long) N, adf_status_str(st));
        for (t = 0; t < (int) p + 2; t++)
        {
            ppow_q(pn, p, N);
            fmpq_mul_ui(pn, pn, (ulong) t);
            fmpq_add(s, c, pn);
            ADF_CHECK_MSG(frac_def_ok(r->q, s, p), "p=%lu v=%ld u=%lu N=%ld t=%d: not the fractional part of a point", p,
                          (long) v, u, (long) N, t);
        }
    }
    else
    {
        ADF_CHECK_MSG(st == ADF_NOT_DETERMINED && fmpz_equal_si(fmpq_numref(r->q), 999), "p=%lu v=%ld u=%lu N=%ld: %s", p,
                      (long) v, u, (long) N, adf_status_str(st));
        /* the ball really has two fractional parts: the points c and c + p^N differ by a non-integer */
        exact_of(y, p, c);
        ADF_CHECK(adf_lball_frac(r, y) == ADF_OK);
        ppow_q(pn, p, N);
        fmpq_add(s, c, pn);
        exact_of(y, p, s);
        ADF_CHECK(adf_lball_frac(r0, y) == ADF_OK);
        ADF_CHECK(!fmpq_equal(r->q, r0->q));
    }
    fmpq_clear(c);
    fmpq_clear(pn);
    fmpq_clear(s);
    adf_lball_clear(x);
    adf_lball_clear(y);
    adf_rat_clear(r);
    adf_rat_clear(r0);
}

ADF_TEST(frac_of_balls)
{
    ulong ps[4] = {2, 3, 5, 7};
    int i;
    slong v, N;
    ulong u, pk;
    for (i = 0; i < 4; i++)
    {
        for (v = -3; v <= 2; v++)
            for (N = -3; N <= 3; N++)
            {
                slong k;
                if (N <= v)
                {
                    check_frac_ball(ps[i], 0, 0, N);       /* the ball around 0 */
                    continue;
                }
                k = N - v;
                if (k > 3 && ps[i] > 2)
                    continue;
                pk = 1;
                for (u = 0; u < (ulong) k; u++)
                    pk *= ps[i];
                for (u = 1; u < pk; u += (ps[i] > 3 ? 3 : 1))
                    if (u % ps[i] != 0)
                        check_frac_ball(ps[i], v, u, N);
            }
    }
}

ADF_TEST(frac_statuses_and_limits)
{
    adf_lball_t x;
    adf_rat_t r;
    fmpq_t q;
    slong E = ADF_LBALL_EXP_MAX;
    adf_lball_init(x);
    adf_rat_init(r);
    fmpq_init(q);
    fmpq_set_si(r->q, 999, 1);
    /* ball 3 + O(2^-1) (N < 0) and O(2^-1) around 0 */
    fmpq_set_si(q, 3, 1);
    ball_of(x, 2, q, -1);
    ADF_CHECK(adf_lball_frac(r, x) == ADF_NOT_DETERMINED && fmpz_equal_si(fmpq_numref(r->q), 999));
    /* N = 0 is determined: the integer balls */
    fmpq_set_si(q, 1, 2);
    ball_of(x, 2, q, 0);
    ADF_CHECK(adf_lball_frac(r, x) == ADF_OK && fmpz_equal_si(fmpq_numref(r->q), 1) && fmpz_equal_si(fmpq_denref(r->q), 2));
    /* the denominator of the result is p^|v|: too large */
    fmpq_set_si(q, 1, 1);
    ball_of(x, 5, q, 3);
    x->v = -(ADF_LBALL_BITS_MAX / 3 + 1);
    x->N = 3;
    ADF_CHECK(adf_lball_is_canonical(x));
    fmpq_set_si(r->q, 999, 1);
    ADF_CHECK(adf_lball_frac(r, x) == ADF_LIMIT && fmpz_equal_si(fmpq_numref(r->q), 999));
    /* exact 5^-(2^24) * 1: same */
    fmpq_set_si(q, 1, 1);
    exact_of(x, 5, q);
    x->v = -(ADF_LBALL_BITS_MAX / 3 + 1);
    ADF_CHECK(adf_lball_is_canonical(x));
    ADF_CHECK(adf_lball_frac(r, x) == ADF_LIMIT && fmpz_equal_si(fmpq_numref(r->q), 999));
    /* positive valuation: 0, whatever the size; the exponent bound is checked */
    x->v = E;
    ADF_CHECK(adf_lball_frac(r, x) == ADF_OK && fmpq_is_zero(r->q));
    x->v = -E - 1;
    ADF_CHECK(adf_lball_frac(r, x) == ADF_LIMIT);
    /* a ball with a huge positive valuation is determined (N >= 0): 0 */
    fmpq_set_si(q, 1, 1);
    ball_of(x, 5, q, 1L << 40);
    fmpq_set_si(r->q, 999, 1);
    ADF_CHECK(adf_lball_frac(r, x) == ADF_OK && fmpq_is_zero(r->q));
    fmpq_clear(q);
    adf_lball_clear(x);
    adf_rat_clear(r);
}

/* ------------------------------------------------------------------------------------------------ unit_mod */

ADF_TEST(unit_mod_against_the_congruences)
{
    ulong ps[4] = {2, 3, 5, 7};
    int i;
    slong v, k, kk;
    ulong u, pk;
    fmpz_t out, P, d;
    adf_lball_t x;
    fmpq_t c, s, pv;
    fmpz_init(out);
    fmpz_init(P);
    fmpz_init(d);
    adf_lball_init(x);
    fmpq_init(c);
    fmpq_init(s);
    fmpq_init(pv);
    for (i = 0; i < 4; i++)
        for (v = -2; v <= 2; v++)
            for (k = 1; k <= 3; k++)
            {
                pk = 1;
                for (u = 0; u < (ulong) k; u++)
                    pk *= ps[i];
                for (u = 1; u < pk; u++)
                {
                    if (u % ps[i] == 0)
                        continue;
                    ppow_q(pv, ps[i], v);
                    fmpq_set_ui(c, u, 1);
                    fmpq_mul(c, c, pv);
                    ball_of(x, ps[i], c, v + k);
                    for (kk = 0; kk <= k + 1; kk++)
                    {
                        int st;
                        fmpz_set_si(out, -5);
                        st = adf_lball_unit_mod(out, x, kk);
                        if (kk <= k)
                        {
                            pk_of(P, ps[i], (ulong) kk);
                            ADF_CHECK_MSG(st == ADF_OK, "unit_mod p=%lu k=%ld: %s", ps[i], (long) kk, adf_status_str(st));
                            ADF_CHECK(fmpz_sgn(out) >= 0 && fmpz_cmp(out, P) < 0);
                            fmpz_sub_ui(d, out, u);
                            ADF_CHECK(fmpz_divisible(d, P));
                        }
                        else
                            ADF_CHECK(st == ADF_NOT_DETERMINED && fmpz_equal_si(out, -5));
                    }
                    /* exact value p^v u / 3 (3 prime to p unless p = 3; then /2, or /5 at p = 3 ... use 11) */
                    fmpq_set(s, c);
                    fmpz_mul_ui(fmpq_denref(s), fmpq_denref(s), 11);
                    fmpq_canonicalise(s);
                    exact_of(x, ps[i], s);
                    for (kk = 0; kk <= 4; kk++)
                    {
                        fmpz_set_si(out, -5);
                        ADF_CHECK(adf_lball_unit_mod(out, x, kk) == ADF_OK);
                        pk_of(P, ps[i], (ulong) kk);
                        ADF_CHECK(fmpz_sgn(out) >= 0 && fmpz_cmp(out, P) < 0);
                        fmpz_mul_ui(d, out, 11);
                        fmpz_sub_ui(d, d, u);
                        ADF_CHECK_MSG(fmpz_divisible(d, P), "exact unit_mod p=%lu u=%lu k=%ld", ps[i], u, (long) kk);
                    }
                }
            }
    /* statuses: exact 0, k < 0, ball around 0, the bit bound */
    fmpq_zero(c);
    exact_of(x, 5, c);
    fmpz_set_si(out, -5);
    ADF_CHECK(adf_lball_unit_mod(out, x, 2) == ADF_DOMAIN && fmpz_equal_si(out, -5));
    ball_of(x, 5, c, 4);
    ADF_CHECK(adf_lball_unit_mod(out, x, 2) == ADF_NOT_DETERMINED && fmpz_equal_si(out, -5));
    fmpq_set_si(c, 3, 1);
    exact_of(x, 5, c);
    ADF_CHECK(adf_lball_unit_mod(out, x, -1) == ADF_DOMAIN && fmpz_equal_si(out, -5));
    ADF_CHECK(adf_lball_unit_mod(out, x, ADF_LBALL_BITS_MAX / 3 + 1) == ADF_LIMIT && fmpz_equal_si(out, -5));
    ADF_CHECK(adf_lball_unit_mod(out, x, LONG_MAX) == ADF_LIMIT && fmpz_equal_si(out, -5));
    ADF_CHECK(adf_lball_unit_mod(out, x, 1) == ADF_OK && fmpz_equal_si(out, 3));
    fmpz_clear(out);
    fmpz_clear(P);
    fmpz_clear(d);
    adf_lball_clear(x);
    fmpq_clear(c);
    fmpq_clear(s);
    fmpq_clear(pv);
}

/* ---------------------------------------------------------------------------------------------------- powers */

/* Checks y = x^k against the sample of points: every s^k lies in y, and the exponent of y is the smallest
   valuation of a difference of two sample values (tight). x is a ball. */
static void
check_pow_ball(const adf_lball_t x, slong k, int npts)
{
    adf_lball_t y;
    fmpq_t base, s, r, r0, d, pn;
    slong minv = LONG_MAX;
    int t, st, zero_in = fmpq_is_zero(x->u);
    ulong p = x->p;
    adf_lball_init(y);
    fmpq_init(base);
    fmpq_init(s);
    fmpq_init(r);
    fmpq_init(r0);
    fmpq_init(d);
    fmpq_init(pn);
    sentinel(y);
    st = adf_lball_pow_si(y, x, k);
    if (k < 0 && zero_in)
    {
        ADF_CHECK_MSG(st == ADF_UNIT_NOT_CERTIFIED && is_sentinel(y), "pow of a ball with 0 to k = %ld: %s", (long) k,
                      adf_status_str(st));
        goto done;
    }
    ADF_CHECK_MSG(st == ADF_OK, "pow p=%lu v=%ld N=%ld k=%ld: %s", p, (long) x->v, (long) x->N, (long) k, adf_status_str(st));
    if (st != ADF_OK)
        goto done;
    ADF_CHECK(adf_lball_is_canonical(y) && !y->exact);
    value_of(base, x);
    for (t = 0; t < npts; t++)
    {
        ppow_q(pn, p, x->N);
        fmpq_mul_ui(pn, pn, (ulong) t);
        fmpq_add(s, base, pn);
        if (k >= 0)
            fmpq_pow_si(r, s, k);
        else
        {
            fmpq_pow_si(r, s, -k);
            fmpq_inv(r, r);
        }
        ADF_CHECK_MSG(in_set(r, y), "pow p=%lu v=%ld N=%ld u=%lu k=%ld t=%d: s^k not in the result (N'=%ld)", p, (long) x->v,
                      (long) x->N, fmpz_get_ui(fmpq_numref(x->u)), (long) k, t, (long) y->N);
        if (t == 0)
            fmpq_set(r0, r);
        else
        {
            fmpq_sub(d, r, r0);
            if (!fmpq_is_zero(d) && val_q(d, p) < minv)
                minv = val_q(d, p);
        }
    }
    ADF_CHECK_MSG(y->N == minv, "pow p=%lu v=%ld N=%ld u=%lu k=%ld: exponent %ld, smallest difference of samples %ld", p,
                      (long) x->v, (long) x->N, fmpz_get_ui(fmpq_numref(x->u)), (long) k, (long) y->N,
                      (long) minv);
done:
    adf_lball_clear(y);
    fmpq_clear(base);
    fmpq_clear(s);
    fmpq_clear(r);
    fmpq_clear(r0);
    fmpq_clear(d);
    fmpq_clear(pn);
}

static void
pow_universe(ulong p, slong kmax, ulong ustep, int npts, const slong * ks, int nk)
{
    slong v, k, N;
    ulong u, pk;
    int j;
    adf_lball_t x;
    fmpq_t c, pv;
    adf_lball_init(x);
    fmpq_init(c);
    fmpq_init(pv);
    for (v = -2; v <= 2; v++)
        for (k = 1; k <= kmax; k++)
        {
            pk = 1;
            for (u = 0; u < (ulong) k; u++)
                pk *= p;
            for (u = 1; u < pk; u += ustep)
            {
                if (u % p == 0)
                    continue;
                ppow_q(pv, p, v);
                fmpq_set_ui(c, u, 1);
                fmpq_mul(c, c, pv);
                ball_of(x, p, c, v + k);
                for (j = 0; j < nk; j++)
                    if (ks[j] != 0)
                        check_pow_ball(x, ks[j], npts);
            }
        }
    /* balls around 0: exponent k N for k > 0, UNIT_NOT_CERTIFIED for k < 0 */
    fmpq_zero(c);
    for (N = -2; N <= 3; N++)
    {
        ball_of(x, p, c, N);
        for (j = 0; j < nk; j++)
        {
            adf_lball_t y;
            int st;
            adf_lball_init(y);
            sentinel(y);
            st = adf_lball_pow_si(y, x, ks[j]);
            if (ks[j] == 0)
                ADF_CHECK(st == ADF_OK && y->exact && fmpq_is_one(y->u));
            else if (ks[j] < 0)
                ADF_CHECK(st == ADF_UNIT_NOT_CERTIFIED && is_sentinel(y));
            else
                ADF_CHECK_MSG(st == ADF_OK && !y->exact && fmpq_is_zero(y->u) && y->N == ks[j] * N && y->v == 0,
                              "O(p^N)^k: p=%lu N=%ld k=%ld", p, (long) N, (long) ks[j]);
            adf_lball_clear(y);
        }
    }
    adf_lball_clear(x);
    fmpq_clear(c);
    fmpq_clear(pv);
}

static const slong KS[] = {-9, -8, -7, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 16, 25, 27};
#define NKS ((int) (sizeof(KS) / sizeof(KS[0])))

ADF_TEST(pow_balls_p2)
{
    pow_universe(2, 5, 1, 20, KS, NKS);
}
ADF_TEST(pow_balls_p3)
{
    pow_universe(3, 3, 1, 12, KS, NKS);
}
ADF_TEST(pow_balls_p5)
{
    pow_universe(5, 2, 1, 8, KS, NKS);
}
ADF_TEST(pow_balls_p7)
{
    pow_universe(7, 2, 2, 8, KS, NKS);
}

ADF_TEST(pow_the_big_prime_and_large_exponents)
{
    ulong p = 18446744073709551557ul;
    slong ks[6] = {2, 3, -2, 5, 1000, -1001};
    int j;
    adf_lball_t x;
    fmpq_t c;
    adf_lball_init(x);
    fmpq_init(c);
    for (j = 0; j < 6; j++)
    {
        fmpq_set_si(c, 3, 1);
        ball_of(x, p, c, 3);
        check_pow_ball(x, ks[j], 3);
        fmpq_set_si(c, 7, p);
        ball_of(x, p, c, 2);
        check_pow_ball(x, ks[j], 3);
    }
    /* a large exponent that p does not divide keeps the precision and the centre is 3^k modulo p^3 */
    fmpq_set_si(c, 3, 1);
    ball_of(x, p, c, 3);
    {
        adf_lball_t y;
        fmpz_t P3, z, e;
        adf_lball_init(y);
        fmpz_init(P3);
        fmpz_init(z);
        fmpz_init(e);
        ADF_CHECK(adf_lball_pow_si(y, x, 5) == ADF_OK && y->N == 3);       /* p does not divide 5: no gain */
        /* k = 2^40 is not a multiple of p; its power is computed modulo p^3 by repeated squaring */
        ADF_CHECK(adf_lball_pow_si(y, x, 1L << 40) == ADF_OK && y->N == 3 && y->v == 0);
        fmpz_set_ui(z, 3);
        fmpz_set_ui(P3, p);
        fmpz_pow_ui(P3, P3, 3);
        fmpz_one(e);
        fmpz_mul_2exp(e, e, 40);
        fmpz_powm(z, z, e, P3);
        ADF_CHECK(fmpz_equal(z, fmpq_numref(y->u)));
        adf_lball_clear(y);
        fmpz_clear(P3);
        fmpz_clear(z);
        fmpz_clear(e);
    }
    fmpq_clear(c);
    adf_lball_clear(x);
}

ADF_TEST(pow_the_factor_v_p_of_k)
{
    /* p = 5, x = 2 + O(5^2): the k-th power has relative precision 2 + v_5(k). Values by the definition. */
    ulong ps[3] = {3, 5, 7};
    int i;
    slong k;
    for (i = 0; i < 3; i++)
        for (k = 1; k <= 60; k++)
        {
            adf_lball_t x, y;
            fmpq_t c;
            slong e = 0, kk = k;
            adf_lball_init(x);
            adf_lball_init(y);
            fmpq_init(c);
            while (kk % (slong) ps[i] == 0)
            {
                kk /= (slong) ps[i];
                e++;
            }
            fmpq_set_si(c, 2, 1);
            ball_of(x, ps[i], c, 2);
            ADF_CHECK(adf_lball_pow_si(y, x, k) == ADF_OK);
            ADF_CHECK_MSG(y->N == 2 + e && y->v == 0, "p=%lu k=%ld: N' = %ld, want %ld", ps[i], (long) k, (long) y->N,
                          (long) (2 + e));
            ADF_CHECK(adf_lball_pow_si(y, x, -k) == ADF_OK && y->N == 2 + e);
            adf_lball_clear(x);
            adf_lball_clear(y);
            fmpq_clear(c);
        }
}

ADF_TEST(pow_exact_values_and_k_zero)
{
    ulong ps[3] = {2, 5, 18446744073709551557ul};
    int i;
    slong k, v;
    for (i = 0; i < 3; i++)
        for (v = -2; v <= 2; v++)
            for (k = -6; k <= 6; k++)
            {
                adf_lball_t x, y, z;
                fmpq_t q, pv, e;
                int st;
                adf_lball_init(x);
                adf_lball_init(y);
                adf_lball_init(z);
                fmpq_init(q);
                fmpq_init(pv);
                fmpq_init(e);
                fmpq_set_si(q, -7, 3);
                ppow_q(pv, ps[i], v);
                fmpq_mul(q, q, pv);
                exact_of(x, ps[i], q);
                sentinel(y);
                st = adf_lball_pow_si(y, x, k);
                if (k >= 0)
                    fmpq_pow_si(e, q, k);
                else
                {
                    fmpq_pow_si(e, q, -k);
                    fmpq_inv(e, e);
                }
                exact_of(z, ps[i], e);
                ADF_CHECK_MSG(st == ADF_OK && adf_lball_identical(y, z), "exact pow p=%lu v=%ld k=%ld", ps[i], (long) v, (long) k);
                /* the exact 0 */
                fmpq_zero(q);
                exact_of(x, ps[i], q);
                sentinel(y);
                st = adf_lball_pow_si(y, x, k);
                if (k > 0)
                    ADF_CHECK(st == ADF_OK && y->exact && fmpq_is_zero(y->u));
                else if (k == 0)
                    ADF_CHECK(st == ADF_OK && y->exact && fmpq_is_one(y->u) && y->v == 0);
                else
                    ADF_CHECK(st == ADF_NOT_UNIT && is_sentinel(y));
                adf_lball_clear(x);
                adf_lball_clear(y);
                adf_lball_clear(z);
                fmpq_clear(q);
                fmpq_clear(pv);
                fmpq_clear(e);
            }
    /* k = 0 for a ball: the exact 1 (a convention, stated in the header) */
    {
        adf_lball_t x, y;
        fmpq_t c;
        adf_lball_init(x);
        adf_lball_init(y);
        fmpq_init(c);
        fmpq_set_si(c, 3, 1);
        ball_of(x, 5, c, 2);
        sentinel(y);
        ADF_CHECK(adf_lball_pow_si(y, x, 0) == ADF_OK && y->exact && fmpq_is_one(y->u) && y->p == 5 && y->v == 0);
        fmpq_clear(c);
        adf_lball_clear(x);
        adf_lball_clear(y);
    }
}

ADF_TEST(pow_limits)
{
    slong E = ADF_LBALL_EXP_MAX;
    adf_lball_t x, y;
    fmpq_t c;
    adf_lball_init(x);
    adf_lball_init(y);
    fmpq_init(c);
    /* exact 5^(2^59) has valuation 2^59: the square is exact 5^(2^60) (OK), the cube is beyond the bound */
    fmpq_set_si(c, 1, 1);
    exact_of(x, 5, c);
    x->v = E / 2;
    ADF_CHECK(adf_lball_is_canonical(x));
    sentinel(y);
    ADF_CHECK(adf_lball_pow_si(y, x, 2) == ADF_OK && y->exact && y->v == E && fmpq_is_one(y->u));
    sentinel(y);
    ADF_CHECK(adf_lball_pow_si(y, x, 3) == ADF_LIMIT && is_sentinel(y));
    ADF_CHECK(adf_lball_pow_si(y, x, -2) == ADF_OK && y->exact && y->v == -E);
    ADF_CHECK(adf_lball_pow_si(y, x, LONG_MIN) == ADF_LIMIT);
    ADF_CHECK(adf_lball_pow_si(y, x, LONG_MAX) == ADF_LIMIT);
    ADF_CHECK(adf_lball_pow_si(y, x, 0) == ADF_OK && y->exact && fmpq_is_one(y->u));
    /* the exact 1 and -1 to any power, huge exponents included: never limited */
    fmpq_set_si(c, 1, 1);
    exact_of(x, 5, c);
    ADF_CHECK(adf_lball_pow_si(y, x, LONG_MAX) == ADF_OK && y->exact && fmpq_is_one(y->u));
    ADF_CHECK(adf_lball_pow_si(y, x, LONG_MIN) == ADF_OK && y->exact && fmpq_is_one(y->u));
    fmpq_set_si(c, -1, 1);
    exact_of(x, 5, c);
    ADF_CHECK(adf_lball_pow_si(y, x, LONG_MAX) == ADF_OK && y->exact && fmpz_equal_si(fmpq_numref(y->u), -1));
    ADF_CHECK(adf_lball_pow_si(y, x, LONG_MIN) == ADF_OK && y->exact && fmpq_is_one(y->u));
    /* an exact rational whose power has more than BITS_MAX bits: LIMIT, before the power is formed (fast) */
    fmpq_set_si(c, 3, 1);
    exact_of(x, 5, c);
    sentinel(y);
    ADF_CHECK(adf_lball_pow_si(y, x, 1L << 40) == ADF_LIMIT && is_sentinel(y));
    ADF_CHECK(adf_lball_pow_si(y, x, -(1L << 40)) == ADF_LIMIT && is_sentinel(y));
    ADF_CHECK(adf_lball_pow_si(y, x, 1000) == ADF_OK && y->exact);
    /* a ball with u = 1: the centre stays 1 whatever k: 1 + O(5^(2^40)) to the power 2^50 is 1 + O(5^(2^40 + 0)) (v_5 of
       2^50 is 0): OK, no power of 5 formed. With k = 5^30 the relative precision grows by 30. */
    fmpq_set_si(c, 1, 1);
    ball_of(x, 5, c, 1L << 40);
    sentinel(y);
    ADF_CHECK(adf_lball_pow_si(y, x, 1L << 50) == ADF_OK && y->N == (1L << 40) && fmpq_is_one(y->u));
    ADF_CHECK(adf_lball_pow_si(y, x, -(1L << 50)) == ADF_OK && y->N == (1L << 40) && fmpq_is_one(y->u));
    {
        slong k30 = 1;
        int i;
        for (i = 0; i < 27; i++)
            k30 *= 5;
        ADF_CHECK(adf_lball_pow_si(y, x, k30) == ADF_OK && y->N == (1L << 40) + 27 && fmpq_is_one(y->u));
    }
    /* another centre: 2 + O(5^(2^40)). The cube 8 is a small integer below 5^(2^40): OK, no power of 5 formed. The
       inverse 1/8 modulo 5^(2^40) needs that power: LIMIT. 2^(2^27) is an integer below 5^(2^40) with more than
       BITS_MAX bits: the result is outside the limits, LIMIT (decided before it is formed). */
    fmpq_set_si(c, 2, 1);
    ball_of(x, 5, c, 1L << 40);
    sentinel(y);
    ADF_CHECK(adf_lball_pow_si(y, x, 3) == ADF_OK && y->N == (1L << 40) && fmpz_equal_si(fmpq_numref(y->u), 8));
    sentinel(y);
    ADF_CHECK(adf_lball_pow_si(y, x, -3) == ADF_LIMIT && is_sentinel(y));
    ADF_CHECK(adf_lball_pow_si(y, x, 1L << 27) == ADF_LIMIT && is_sentinel(y));
    /* an input beyond the bound: LIMIT, also for k = 0 */
    fmpq_set_si(c, 1, 1);
    ball_of(x, 5, c, 2);
    x->N = E + 1;
    sentinel(y);
    ADF_CHECK(adf_lball_pow_si(y, x, 0) == ADF_LIMIT && is_sentinel(y));
    /* ball 5^(-2^30) (1 + O(5)) to a power: N = k v + rel' beyond the bound */
    ball_of(x, 5, c, 1);
    x->v = -(E / 4);
    x->N = -(E / 4) + 1;
    ADF_CHECK(adf_lball_is_canonical(x));
    ADF_CHECK(adf_lball_pow_si(y, x, 3) == ADF_OK && y->v == -3 * (E / 4) && y->N == -3 * (E / 4) + 1);
    sentinel(y);
    ADF_CHECK(adf_lball_pow_si(y, x, 5) == ADF_LIMIT && is_sentinel(y));
    /* the ball around 0 with N k beyond the bound */
    fmpq_zero(c);
    ball_of(x, 5, c, E / 2 + 1);
    sentinel(y);
    ADF_CHECK(adf_lball_pow_si(y, x, 2) == ADF_LIMIT && is_sentinel(y));
    fmpq_clear(c);
    adf_lball_clear(x);
    adf_lball_clear(y);
}

ADF_TEST(pow_aliasing_and_composition)
{
    ulong ps[3] = {2, 3, 5};
    slong ks[6] = {-3, -1, 1, 2, 3, 6};
    int i, j;
    slong v;
    for (i = 0; i < 3; i++)
        for (v = -1; v <= 1; v++)
            for (j = 0; j < 6; j++)
            {
                adf_lball_t x, y, z, w;
                fmpq_t c, pv;
                adf_lball_init(x);
                adf_lball_init(y);
                adf_lball_init(z);
                adf_lball_init(w);
                fmpq_init(c);
                fmpq_init(pv);
                ppow_q(pv, ps[i], v);
                fmpq_set_si(c, 5, 1);
                fmpq_mul(c, c, pv);
                ball_of(x, ps[i], c, v + 4);
                if (adf_lball_pow_si(y, x, ks[j]) == ADF_OK)
                {
                    adf_lball_set(z, x);
                    ADF_CHECK(adf_lball_pow_si(z, z, ks[j]) == ADF_OK && adf_lball_identical(z, y));
                    /* the power is the repeated product for k > 0 (a ball times itself as independent factors is
                       larger or equal; the power is contained in the repeated product) */
                    if (ks[j] > 0)
                    {
                        int t;
                        adf_lball_set(w, x);
                        for (t = 1; t < ks[j]; t++)
                            ADF_CHECK(adf_lball_mul(w, w, x) == ADF_OK);
                        ADF_CHECK(adf_lball_contains(y, w));
                    }
                    /* x^(-k) is the inverse of x^k and x^k the inverse of x^(-k) */
                    {
                        adf_lball_t a, b;
                        adf_lball_init(a);
                        adf_lball_init(b);
                        ADF_CHECK(adf_lball_pow_si(a, x, -ks[j]) == ADF_OK);
                        ADF_CHECK(adf_lball_inv(b, a) == ADF_OK && adf_lball_identical(b, y));
                        adf_lball_clear(a);
                        adf_lball_clear(b);
                    }
                }
                else
                    ADF_CHECK(0);
                fmpq_clear(c);
                fmpq_clear(pv);
                adf_lball_clear(x);
                adf_lball_clear(y);
                adf_lball_clear(z);
                adf_lball_clear(w);
            }
}

/* ------------------------------------------------------------------------------------------------- vectors */

/* Every line of tests/ref/vectors/f-slice3/lball_slice3.jsonl, made by lanes/f-slice3/gen_vectors.py from the Python
   reference (proto/functions_checks.py, section f-slice3: Teichmueller by the limit of powers, the split, Proposition 19,
   the unit modulo p^k, and L12 checked there by enumeration). A line fails if the status, a field of a result or the
   state of an output after a status differs. The values are written into the struct by hand, not built by the
   constructors under test. */

static int
status_from_name(const char * s)
{
    int i;
    for (i = 0; i < ADF_STATUS_COUNT; i++)
        if (strcmp(adf_status_str(i), s) == 0)
            return i;
    return -1;
}

static const jsonl_value *
member(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    const jsonl_value * v = NULL;
    int ok = jsonl_field(rec, key, &v, &err);
    ADF_CHECK_MSG(ok == 1, "%s", jsonl_error_message(&err));
    return ok ? v : NULL;
}

static const char *
member_int(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    const jsonl_value * v = member(rec, key);
    const char * t = v ? jsonl_int_text(v, &err) : NULL;
    ADF_CHECK(t != NULL);
    return t ? t : "0";
}

static const char *
member_str(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    size_t len;
    const jsonl_value * v = member(rec, key);
    const char * s = v ? jsonl_string(v, &len, &err) : NULL;
    ADF_CHECK(s != NULL);
    return s ? s : "";
}

static void
fmpz_from_text(fmpz_t z, const char * t)
{
    ADF_CHECK(fmpz_set_str(z, t, 10) == 0);
}

static void
lb_from_json(adf_lball_t x, const jsonl_value * o)
{
    fmpz_t un, ud;
    fmpz_init(un);
    fmpz_init(ud);
    fmpz_from_text(un, member_int(o, "un"));
    fmpz_from_text(ud, member_int(o, "ud"));
    x->p = strtoul(member_int(o, "p"), NULL, 10);
    fmpz_set(fmpq_numref(x->u), un);
    fmpz_set(fmpq_denref(x->u), ud);
    x->v = strtol(member_int(o, "v"), NULL, 10);
    x->N = strtol(member_int(o, "N"), NULL, 10);
    x->exact = (int) strtol(member_int(o, "exact"), NULL, 10);
    fmpz_clear(un);
    fmpz_clear(ud);
}

ADF_TEST(vectors_slice3)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i, nop[5] = {0, 0, 0, 0, 0};
    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/f-slice3/lball_slice3.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * op = member_str(rec, "op");
        int want = status_from_name(member_str(rec, "status")), st;
        unsigned long line = jsonl_line_of(rec);
        adf_lball_t x, y, y2, e, e2;
        adf_lball_init(x);
        adf_lball_init(y);
        adf_lball_init(y2);
        adf_lball_init(e);
        adf_lball_init(e2);
        ADF_CHECK(want >= 0);
        if (strcmp(op, "teich") == 0)
        {
            ulong p = strtoul(member_int(rec, "p"), NULL, 10), r = strtoul(member_int(rec, "r"), NULL, 10);
            slong prec = strtol(member_int(rec, "prec"), NULL, 10);
            sentinel(y);
            st = adf_lball_teichmuller(y, place_of(p), r, prec);
            ADF_CHECK_MSG(st == want, "line %lu teich: status %s", line, adf_status_str(st));
            if (want == ADF_OK)
            {
                lb_from_json(e, member(rec, "w"));
                ADF_CHECK_MSG(adf_lball_identical(y, e), "line %lu teich: value", line);
            }
            else
                ADF_CHECK(is_sentinel(y));
            nop[0]++;
        }
        else if (strcmp(op, "split") == 0)
        {
            slong m = 4242, prec = strtol(member_int(rec, "prec"), NULL, 10);
            ulong index = 4343;
            adf_lball_t xa;
            lb_from_json(x, member(rec, "x"));
            sentinel(y);
            sentinel(y2);
            st = adf_lball_decompose_teich(&m, y, &index, y2, x, prec);
            ADF_CHECK_MSG(st == want, "line %lu split: status %s", line, adf_status_str(st));
            if (want == ADF_OK)
            {
                lb_from_json(e, member(rec, "w"));
                lb_from_json(e2, member(rec, "u"));
                ADF_CHECK_MSG(adf_lball_identical(y, e) && adf_lball_identical(y2, e2), "line %lu split: values", line);
                ADF_CHECK_MSG(m == strtol(member_int(rec, "m"), NULL, 10) &&
                              index == strtoul(member_int(rec, "index"), NULL, 10), "line %lu split: m, index", line);
                /* aliased output = input */
                adf_lball_init(xa);
                adf_lball_set(xa, x);
                m = 1;
                ADF_CHECK(adf_lball_decompose_teich(&m, xa, &index, y2, xa, prec) == ADF_OK &&
                          adf_lball_identical(xa, e));
                adf_lball_set(xa, x);
                ADF_CHECK(adf_lball_decompose_teich(&m, y, &index, xa, xa, prec) == ADF_OK &&
                          adf_lball_identical(xa, e2));
                adf_lball_clear(xa);
            }
            else
                ADF_CHECK_MSG(is_sentinel(y) && is_sentinel(y2) && m == 4242 && index == 4343, "line %lu split: outputs",
                              line);
            nop[1]++;
        }
        else if (strcmp(op, "frac") == 0)
        {
            adf_rat_t r;
            adf_rat_init(r);
            fmpq_set_si(r->q, 987, 1);
            lb_from_json(x, member(rec, "x"));
            st = adf_lball_frac(r, x);
            ADF_CHECK_MSG(st == want, "line %lu frac: status %s", line, adf_status_str(st));
            if (want == ADF_OK)
            {
                const jsonl_value * q = member(rec, "r");
                fmpz_t n, d;
                fmpz_init(n);
                fmpz_init(d);
                fmpz_from_text(n, member_int(q, "num"));
                fmpz_from_text(d, member_int(q, "den"));
                ADF_CHECK_MSG(fmpz_equal(n, fmpq_numref(r->q)) && fmpz_equal(d, fmpq_denref(r->q)), "line %lu frac: value",
                              line);
                fmpz_clear(n);
                fmpz_clear(d);
            }
            else
                ADF_CHECK(fmpz_equal_si(fmpq_numref(r->q), 987));
            adf_rat_clear(r);
            nop[2]++;
        }
        else if (strcmp(op, "unit_mod") == 0)
        {
            fmpz_t out, w;
            slong k = strtol(member_int(rec, "k"), NULL, 10);
            fmpz_init_set_si(out, -77);
            fmpz_init(w);
            lb_from_json(x, member(rec, "x"));
            st = adf_lball_unit_mod(out, x, k);
            ADF_CHECK_MSG(st == want, "line %lu unit_mod: status %s", line, adf_status_str(st));
            if (want == ADF_OK)
            {
                fmpz_from_text(w, member_int(rec, "out"));
                ADF_CHECK_MSG(fmpz_equal(out, w), "line %lu unit_mod: value", line);
            }
            else
                ADF_CHECK(fmpz_equal_si(out, -77));
            fmpz_clear(out);
            fmpz_clear(w);
            nop[3]++;
        }
        else if (strcmp(op, "pow") == 0)
        {
            slong k = strtol(member_int(rec, "k"), NULL, 10);
            lb_from_json(x, member(rec, "x"));
            sentinel(y);
            st = adf_lball_pow_si(y, x, k);
            ADF_CHECK_MSG(st == want, "line %lu pow: status %s", line, adf_status_str(st));
            if (want == ADF_OK)
            {
                lb_from_json(e, member(rec, "result"));
                ADF_CHECK_MSG(adf_lball_identical(y, e), "line %lu pow: value", line);
                adf_lball_set(y2, x);
                ADF_CHECK(adf_lball_pow_si(y2, y2, k) == ADF_OK && adf_lball_identical(y2, e));
            }
            else
                ADF_CHECK(is_sentinel(y));
            nop[4]++;
        }
        else
            ADF_CHECK_MSG(0, "line %lu: unknown op %s", line, op);
        adf_lball_clear(x);
        adf_lball_clear(y);
        adf_lball_clear(y2);
        adf_lball_clear(e);
        adf_lball_clear(e2);
    }
    ADF_CHECK(nop[0] == 360 && nop[1] == 720 && nop[2] == 480 && nop[3] == 480 && nop[4] == 960);
    jsonl_close(f);
}
