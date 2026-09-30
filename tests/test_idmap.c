/* tests/test_idmap.c: ideles and adeles (lane i-slice3; include/adelefeld/idmap.h: adf_adele_set_idele,
   adf_adele_set_idele_simple, adf_idele_set_adele, adf_adele_div_idele; and adf_adele_div_rat of adele.h against
   docs/proofs/ideles.md P18; docs/api-2.md 3.3, Statements M, N, O; ideles.md P16 to P19).

   The oracles:
   1. Enumeration in Z/M. The finite part of an idele (X, r, c U(N)) is {r u : u in c U(N)}; at a level M (a
      multiple of lcm(N, 2) with the primes 2, 3, 5, 7 once more) the units u are the residues w in (Z/M)^x with
      w = c mod N, and r u lies in a ball r c' + r L Zhat (L | M) exactly when w = c' mod L. Every r w must lie in
      both hulls; the radius of the smallest hull must be r times the gcd of M and the differences of the w
      (P16.3, tightness).
   2. The quotients of the division: for x = I x (a + M Zhat), a = A/D, M = B/D, and y = (Y, r, c U(N)), the finite
      parts of x/y are ((A + B z) w)/(D r) with z an integer and w the inverse of a unit of c U(N); modulo
      Mod = lcm(6, B, |A| L, L) they are enumerated, and the smallest ball has the radius (gcd of Mod and the
      differences)/(D r). Every quotient must lie in the result. The real part must contain the four exact
      quotients of the end points (the quotient set of two intervals, Y excluding 0, is the interval between
      them).
   3. The library itself, another way: the division equals the product rule (adf_fball_mul) with the smallest
      hull of the inverse idele (adf_idele_inv, adf_adele_set_idele), and for an exact unit the division by the
      exact rational e r (adf_adele_div_rat).
   4. tests/ref/vectors/i-slice3/idmap.jsonl, written by lanes/i-slice3/gen_vectors.py from part 4 of
      proto/ideles_checks.py: the canonical triples of both hulls, the statuses of adele to idele, and the finite
      part of the division computed by the formula of P19 (not by the product rule). Every line is run. */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <flint/arb.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>

#include <adelefeld.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* ---- helpers ---- */

static void
arb_set_exact(arb_t x, const fmpz_t m1, const fmpz_t e1, ulong m2, slong e2)
{
    arf_t t, u;
    arf_init(t);
    arf_init(u);
    arf_set_fmpz_2exp(arb_midref(x), m1, e1);
    mag_set_ui_2exp_si(arb_radref(x), m2, e2);
    arf_set_mag(t, arb_radref(x));
    arf_set_ui(u, m2);
    arf_mul_2exp_si(u, u, e2);
    if (!arf_equal(t, u))
    {
        printf("arb_set_exact: the radius is not held exactly by a mag\n");
        abort();
    }
    arf_clear(t);
    arf_clear(u);
}

static void
arb_set_si_si(arb_t x, slong m, slong em, ulong r, slong er)
{
    fmpz_t a, b;
    fmpz_init_set_si(a, m);
    fmpz_init_set_si(b, em);
    arb_set_exact(x, a, b, r, er);
    fmpz_clear(a);
    fmpz_clear(b);
}

static void
uc_set_si(adf_ucoset_t u, slong c, slong N)
{
    fmpz_t fc, fN;
    fmpz_init_set_si(fc, c);
    fmpz_init_set_si(fN, N);
    if (adf_ucoset_set_fmpz2(u, fc, fN) != ADF_OK)
    {
        printf("uc_set_si: refused (%ld, %ld)\n", (long) c, (long) N);
        abort();
    }
    fmpz_clear(fc);
    fmpz_clear(fN);
}

static void
idele_set_parts_q(adf_idele_t x, const arb_t inf, const fmpq_t r, slong c, slong N)
{
    adf_ucoset_t u;
    adf_ucoset_init(u);
    uc_set_si(u, c, N);
    if (adf_idele_set_parts(x, inf, r, u) != ADF_OK)
    {
        printf("idele_set_parts_q: refused\n");
        abort();
    }
    adf_ucoset_clear(u);
}

/* fin = (A + H Zhat)/d, global canonical */
static void
fball_set_si3(adf_fball_t f, slong A, slong H, slong d)
{
    fmpz_t a, h, e;
    fmpz_init_set_si(a, A);
    fmpz_init_set_si(h, H);
    fmpz_init_set_si(e, d);
    if (adf_fball_set_fmpz3(f, a, h, e) != ADF_OK)
        abort();
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(e);
}

/* 1 if the canonical triple of f is (A, H, d) */
static int
fball_is(const adf_fball_t f, const fmpz_t A, const fmpz_t H, const fmpz_t d)
{
    fmpz_t a, h, e;
    int ok;
    fmpz_init(a);
    fmpz_init(h);
    fmpz_init(e);
    adf_fball_get_fmpz3(a, h, e, f);
    ok = fmpz_equal(a, A) && fmpz_equal(h, H) && fmpz_equal(e, d);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(e);
    return ok;
}

static int
fball_is_si(const adf_fball_t f, slong A, slong H, slong d)
{
    fmpz_t a, h, e;
    int ok;
    fmpz_init_set_si(a, A);
    fmpz_init_set_si(h, H);
    fmpz_init_set_si(e, d);
    ok = fball_is(f, a, h, e);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(e);
    return ok;
}

static ulong
gcd_ui(ulong a, ulong b)
{
    while (b != 0)
    {
        ulong t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static ulong
lcm_ui(ulong a, ulong b)
{
    return a / gcd_ui(a, b) * b;
}

static ulong
inv_mod(ulong a, ulong m)
{
    slong r0 = (slong) m, r1 = (slong) (a % m), s0 = 0, s1 = 1;
    while (r1 != 0)
    {
        slong q = r0 / r1, t;
        t = r0 - q * r1;
        r0 = r1;
        r1 = t;
        t = s0 - q * s1;
        s0 = s1;
        s1 = t;
    }
    if (r0 != 1)
        abort();
    return (ulong) (((s0 % (slong) m) + (slong) m) % (slong) m);
}

/* 1 if the rational a/b lies in f */
static int
fball_has(const adf_fball_t f, const fmpz_t a, const fmpz_t b)
{
    adf_rat_t q;
    int in;
    adf_rat_init(q);
    fmpq_set_fmpz_frac(q->q, a, b);
    in = adf_fball_contains_rat(f, q);
    adf_rat_clear(q);
    return in;
}

/* the radius of f as an fmpq */
static void
radius_of(fmpq_t R, const adf_fball_t f)
{
    adf_rat_t q;
    adf_rat_init(q);
    adf_fball_get_radius(q, f);
    fmpq_set(R, q->q);
    adf_rat_clear(q);
}

/* ---- JSON helpers ---- */

static const jsonl_value *
field(const jsonl_value * rec, const char * key)
{
    const jsonl_value * v;
    jsonl_error_t err;
    if (!jsonl_field(rec, key, &v, &err))
    {
        printf("field %s: %s\n", key, jsonl_error_message(&err));
        abort();
    }
    return v;
}

static void
read_fmpz(fmpz_t z, const jsonl_value * v)
{
    jsonl_error_t err;
    const char * t;
    if (!jsonl_int_text_or_string(v, &t, &err) || fmpz_set_str(z, t, 10) != 0)
        abort();
}

static void
read_fmpz_at(fmpz_t z, const jsonl_value * arr, size_t i)
{
    jsonl_error_t err;
    read_fmpz(z, jsonl_at(arr, i, &err));
}

static slong
read_si(const jsonl_value * v)
{
    fmpz_t z;
    slong s;
    fmpz_init(z);
    read_fmpz(z, v);
    if (!fmpz_fits_si(z))
        abort();
    s = fmpz_get_si(z);
    fmpz_clear(z);
    return s;
}

static void
read_fmpq(fmpq_t q, const jsonl_value * v)
{
    read_fmpz_at(fmpq_numref(q), v, 0);
    read_fmpz_at(fmpq_denref(q), v, 1);
}

static void
read_ball(arb_t b, const jsonl_value * rec)
{
    fmpz_t m1, e1, m2, e2;
    fmpz_init(m1);
    fmpz_init(e1);
    fmpz_init(m2);
    fmpz_init(e2);
    read_fmpz_at(m1, field(rec, "mid"), 0);
    read_fmpz_at(e1, field(rec, "mid"), 1);
    read_fmpz_at(m2, field(rec, "rad"), 0);
    read_fmpz_at(e2, field(rec, "rad"), 1);
    arb_set_exact(b, m1, e1, fmpz_get_ui(m2), fmpz_get_si(e2));
    fmpz_clear(m1);
    fmpz_clear(e1);
    fmpz_clear(m2);
    fmpz_clear(e2);
}

static void
read_uc(adf_ucoset_t u, const jsonl_value * v)
{
    read_fmpz_at(u->c, v, 0);
    read_fmpz_at(u->N, v, 1);
    if (!adf_ucoset_is_canonical(u))
        abort();
}

static void
read_idele(adf_idele_t x, const jsonl_value * v)
{
    adf_ucoset_t u;
    fmpq_t r;
    arb_t inf;
    adf_ucoset_init(u);
    fmpq_init(r);
    arb_init(inf);
    read_ball(inf, v);
    read_fmpq(r, field(v, "r"));
    read_uc(u, field(v, "u"));
    if (adf_idele_set_parts(x, inf, r, u) != ADF_OK || !adf_idele_is_canonical(x))
    {
        printf("read_idele: the input is not an idele\n");
        abort();
    }
    adf_ucoset_clear(u);
    fmpq_clear(r);
    arb_clear(inf);
}

static void
read_triple(fmpz_t A, fmpz_t H, fmpz_t d, const jsonl_value * v)
{
    read_fmpz_at(A, v, 0);
    read_fmpz_at(H, v, 1);
    read_fmpz_at(d, v, 2);
}

static void
read_adele(adf_adele_t x, const jsonl_value * v)
{
    arb_t inf;
    adf_fball_t f;
    fmpz_t A, H, d;
    arb_init(inf);
    adf_fball_init(f);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    read_ball(inf, v);
    read_triple(A, H, d, field(v, "fin"));
    if (adf_fball_set_fmpz3(f, A, H, d) != ADF_OK || !fball_is(f, A, H, d)
        || adf_adele_set_arb_fball(x, inf, f) != ADF_OK)
    {
        printf("read_adele: the input is not a canonical adele\n");
        abort();
    }
    arb_clear(inf);
    adf_fball_clear(f);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

static int
status_of(const char * s)
{
    if (strcmp(s, "OK") == 0)
        return ADF_OK;
    if (strcmp(s, "UNIT_NOT_CERTIFIED") == 0)
        return ADF_UNIT_NOT_CERTIFIED;
    if (strcmp(s, "NOT_UNIT") == 0)
        return ADF_NOT_UNIT;
    if (strcmp(s, "LIMIT") == 0)
        return ADF_LIMIT;
    abort();
}

static const char *
str_of(const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    size_t len;
    const char * s = jsonl_string(field(rec, key), &len, &err);
    if (s == NULL)
        abort();
    return s;
}

/* The real part of x / y contains the four quotients of the exact end points (oracle 2). */
static int
real_quotient_ok(const arb_t z, const arb_t I, const arb_t Y)
{
    fmpq_t il, ih, yl, yh, q, m, r;
    arf_t t;
    int ok = arb_is_finite(z), i, j;
    fmpq_init(il);
    fmpq_init(ih);
    fmpq_init(yl);
    fmpq_init(yh);
    fmpq_init(q);
    fmpq_init(m);
    fmpq_init(r);
    arf_init(t);
    arf_get_fmpq(m, arb_midref(I));
    arf_set_mag(t, arb_radref(I));
    arf_get_fmpq(r, t);
    fmpq_sub(il, m, r);
    fmpq_add(ih, m, r);
    arf_get_fmpq(m, arb_midref(Y));
    arf_set_mag(t, arb_radref(Y));
    arf_get_fmpq(r, t);
    fmpq_sub(yl, m, r);
    fmpq_add(yh, m, r);
    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++)
        {
            fmpq_div(q, i ? ih : il, j ? yh : yl);
            ok &= arb_contains_fmpq(z, q);
        }
    fmpq_clear(il);
    fmpq_clear(ih);
    fmpq_clear(yl);
    fmpq_clear(yh);
    fmpq_clear(q);
    fmpq_clear(m);
    fmpq_clear(r);
    arf_clear(t);
    return ok;
}

/* ---- the hulls ---- */

ADF_TEST(hulls_contain_every_point_and_the_small_one_is_tight)
{
    static const slong rs[][2] = { { 1, 1 }, { 3, 2 }, { 2, 9 }, { 10, 7 }, { 1, 4 }, { 5, 1 } };
    adf_idele_t x, xn;
    adf_adele_t small, simple, a;
    adf_ucoset_t u;
    arb_t inf;
    fmpq_t r, R, want;
    fmpz_t num, den;
    slong N, c;
    size_t i;
    int n_cases = 0, n_coarser = 0;

    adf_idele_init(x);
    adf_idele_init(xn);
    adf_adele_init(small);
    adf_adele_init(simple);
    adf_adele_init(a);
    adf_ucoset_init(u);
    arb_init(inf);
    fmpq_init(r);
    fmpq_init(R);
    fmpq_init(want);
    fmpz_init(num);
    fmpz_init(den);
    arb_set_si_si(inf, -7, -1, 3, -3);                   /* -3.5 +- 3/8 */
    for (N = 0; N <= 30; N++)
        for (c = (N == 0 ? -1 : 1); c <= (N == 0 ? 1 : N); c += (N == 0 ? 2 : 1))
        {
            ulong L, M, Nn, w, D = 0, w0 = 0;
            int first = 1, all_in = 1;
            if (N >= 1 && gcd_ui((ulong) c, (ulong) N) != 1)
                continue;
            for (i = 0; i < sizeof(rs) / sizeof(rs[0]); i++)
            {
                fmpq_set_si(r, rs[i][0], (ulong) rs[i][1]);
                idele_set_parts_q(x, inf, r, c, N);
                adf_adele_set_idele(small, x);
                adf_adele_set_idele_simple(simple, x);
                ADF_CHECK(arb_equal(small->inf, inf) && arb_equal(simple->inf, inf));
                ADF_CHECK(adf_adele_is_canonical(small) && adf_adele_is_canonical(simple)
                          && !adf_fball_is_local(&small->fin));
                if (N == 0)
                {
                    /* an exact unit: the exact rational r e (SPEC 5) */
                    fmpq_mul_si(want, r, c);
                    ADF_CHECK(adf_fball_is_exact(&small->fin) && adf_fball_is_exact(&simple->fin)
                              && fball_is_si(&small->fin, fmpz_get_si(fmpq_numref(want)), 0,
                                             fmpz_get_si(fmpq_denref(want)))
                              && adf_fball_equal_set(&small->fin, &simple->fin));
                    n_cases++;
                    continue;
                }
                /* oracle 1 at the level M = 210 lcm(N, 2) */
                L = lcm_ui((ulong) N, 2);
                M = L * 210;
                first = 1;
                all_in = 1;
                for (w = (ulong) c % (ulong) N; w < M; w += (ulong) N)
                {
                    if (gcd_ui(w, M) != 1)
                        continue;
                    fmpz_set_ui(num, w);
                    fmpz_mul(num, num, fmpq_numref(r));
                    all_in &= fball_has(&small->fin, num, fmpq_denref(r))
                              && fball_has(&simple->fin, num, fmpq_denref(r));
                    if (first)
                    {
                        w0 = w;
                        D = M;
                        first = 0;
                    }
                    else
                        D = gcd_ui(D, w - w0);
                }
                ADF_CHECK_MSG(all_in, "(%ld, %ld), r = %ld/%ld: a point outside a hull", (long) c, (long) N,
                              (long) rs[i][0], (long) rs[i][1]);
                /* tightness (P16.3): the radius of the small hull is r D, and D = lcm(N, 2) */
                radius_of(R, &small->fin);
                fmpq_mul_ui(want, r, D);
                ADF_CHECK_MSG(fmpq_equal(R, want) && D == L, "(%ld, %ld): radius", (long) c, (long) N);
                /* the simple hull: radius r N' (N' the normal modulus), contains the small one */
                Nn = (N % 4 == 2) ? (ulong) N / 2 : (ulong) N;
                radius_of(R, &simple->fin);
                fmpq_mul_ui(want, r, Nn);
                ADF_CHECK(fmpq_equal(R, want) && adf_fball_contains(&small->fin, &simple->fin));
                n_coarser += !adf_fball_equal_set(&small->fin, &simple->fin);
                ADF_CHECK((Nn % 2 == 0) == adf_fball_equal_set(&small->fin, &simple->fin));   /* P16.4 */
                /* both depend on the set only: the normal form of the unit gives the same balls */
                adf_ucoset_normalise(u, &x->u);
                ADF_CHECK(adf_idele_set_parts(xn, inf, r, u) == ADF_OK);
                adf_adele_set_idele(a, xn);
                ADF_CHECK(adf_adele_identical(a, small));
                adf_adele_set_idele_simple(a, xn);
                ADF_CHECK(adf_adele_identical(a, simple));
                n_cases++;
            }
        }
    printf("hulls: %d (coset, content) cases, simple hull strictly coarser in %d\n", n_cases, n_coarser);
    ADF_CHECK(n_cases > 1000 && n_coarser > 500);
    /* the examples: [5 mod 6] and [2 mod 3] have the smallest hull 5 + 6 Zhat; the simple hulls 2 + 3 Zhat */
    fmpq_one(r);
    idele_set_parts_q(x, inf, r, 5, 6);
    adf_adele_set_idele(small, x);
    adf_adele_set_idele_simple(simple, x);
    ADF_CHECK(fball_is_si(&small->fin, 5, 6, 1) && fball_is_si(&simple->fin, 2, 3, 1));
    idele_set_parts_q(x, inf, r, 2, 3);
    adf_adele_set_idele(small, x);
    ADF_CHECK(fball_is_si(&small->fin, 5, 6, 1));
    /* (x ; 3/2 [5 mod 36]): 15/2 + 54 Zhat = (15 + 108 Zhat)/2; (x ; 3/2 [-1]): the exact -3/2 */
    fmpq_set_si(r, 3, 2);
    idele_set_parts_q(x, inf, r, 5, 36);
    adf_adele_set_idele(small, x);
    ADF_CHECK(fball_is_si(&small->fin, 15, 108, 2));
    idele_set_parts_q(x, inf, r, -1, 0);
    adf_adele_set_idele(small, x);
    ADF_CHECK(fball_is_si(&small->fin, -3, 0, 2));
    /* the output had a local finite part: it becomes global (no context kept) */
    {
        adf_modctx_struct * ctx;
        static const ulong q[] = { 8, 9, 5 };
        adf_fball_t f;
        adf_fball_init(f);
        ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 3) == ADF_OK);
        fball_set_si3(f, 7, 12, 1);
        ADF_CHECK(adf_fball_set_local(f, f, ctx) == ADF_OK && adf_fball_is_local(f));
        ADF_CHECK(adf_adele_set_arb_fball(a, inf, f) == ADF_OK && adf_fball_is_local(&a->fin));
        adf_adele_set_idele(a, x);
        ADF_CHECK(!adf_fball_is_local(&a->fin) && fball_is_si(&a->fin, -3, 0, 2));
        ADF_CHECK(adf_adele_set_arb_fball(a, inf, f) == ADF_OK);
        adf_adele_set_idele_simple(a, x);
        ADF_CHECK(!adf_fball_is_local(&a->fin) && fball_is_si(&a->fin, -3, 0, 2));
        adf_fball_clear(f);
        adf_adele_clear(a);
        adf_adele_init(a);
        adf_modctx_free(ctx);
    }
    fmpz_clear(num);
    fmpz_clear(den);
    fmpq_clear(r);
    fmpq_clear(R);
    fmpq_clear(want);
    arb_clear(inf);
    adf_ucoset_clear(u);
    adf_idele_clear(x);
    adf_idele_clear(xn);
    adf_adele_clear(small);
    adf_adele_clear(simple);
    adf_adele_clear(a);
}

/* ---- adele -> idele ---- */

ADF_TEST(set_adele_statuses_and_round_trip)
{
    adf_adele_t x, h;
    adf_idele_t y, keep;
    adf_fball_t f;
    arb_t inf;
    fmpq_t want;

    adf_adele_init(x);
    adf_adele_init(h);
    adf_idele_init(y);
    adf_idele_init(keep);
    adf_fball_init(f);
    arb_init(inf);
    fmpq_init(want);
    arb_set_si_si(keep->inf, 5, 0, 1, -1);
    adf_idele_set(y, keep);
    /* (2 +- 1 ; -3/4): OK, (2 +- 1, 3/4, [-1]) */
    arb_set_si_si(inf, 2, 0, 1, 0);
    fball_set_si3(f, -3, 0, 4);
    ADF_CHECK(adf_adele_set_arb_fball(x, inf, f) == ADF_OK);
    ADF_CHECK(adf_idele_set_adele(y, x) == ADF_OK && adf_idele_is_canonical(y) && arb_equal(y->inf, inf));
    fmpq_set_si(want, 3, 4);
    ADF_CHECK(fmpq_equal(y->r, want) && fmpz_equal_si(y->u.c, -1) && fmpz_is_zero(y->u.N));
    /* the round trip: the smallest hull of y is x again */
    adf_adele_set_idele(h, y);
    ADF_CHECK(adf_adele_identical(h, x));
    /* the statuses; y untouched */
    adf_idele_set(y, keep);
    fball_set_si3(f, 1, 6, 1);                                    /* radius > 0 */
    adf_adele_set_arb_fball(x, inf, f);
    ADF_CHECK(adf_idele_set_adele(y, x) == ADF_UNIT_NOT_CERTIFIED && adf_idele_identical(y, keep));
    fball_set_si3(f, 0, 0, 1);                                    /* the exact 0 */
    adf_adele_set_arb_fball(x, inf, f);
    ADF_CHECK(adf_idele_set_adele(y, x) == ADF_NOT_UNIT && adf_idele_identical(y, keep));
    arb_zero(inf);                                                /* real part exact 0 */
    fball_set_si3(f, 1, 0, 1);
    adf_adele_set_arb_fball(x, inf, f);
    ADF_CHECK(adf_idele_set_adele(y, x) == ADF_NOT_UNIT && adf_idele_identical(y, keep));
    fball_set_si3(f, 1, 6, 1);                                    /* NOT_UNIT and UNIT_NOT_CERTIFIED: max */
    adf_adele_set_arb_fball(x, inf, f);
    ADF_CHECK(adf_idele_set_adele(y, x) == ADF_NOT_UNIT && adf_idele_identical(y, keep));
    arb_set_si_si(inf, 1, 0, 1, 0);                               /* [0, 2]: contains 0, not the exact 0 */
    fball_set_si3(f, 1, 0, 1);
    adf_adele_set_arb_fball(x, inf, f);
    ADF_CHECK(adf_idele_set_adele(y, x) == ADF_UNIT_NOT_CERTIFIED && adf_idele_identical(y, keep));
    fball_set_si3(f, 0, 0, 1);                                    /* the exact 0 and a real ball with 0 */
    adf_adele_set_arb_fball(x, inf, f);
    ADF_CHECK(adf_idele_set_adele(y, x) == ADF_NOT_UNIT && adf_idele_identical(y, keep));
    /* a huge exact finite part: 2^300 + 1 and -1/3^50 */
    arb_set_si_si(inf, -1, 0, 1, -1);
    {
        fmpz_t A, H, d;
        fmpz_init(A);
        fmpz_init(H);
        fmpz_init_set_ui(d, 1);
        fmpz_one(A);
        fmpz_mul_2exp(A, A, 300);
        fmpz_add_ui(A, A, 1);
        ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
        adf_adele_set_arb_fball(x, inf, f);
        ADF_CHECK(adf_idele_set_adele(y, x) == ADF_OK && fmpz_equal(fmpq_numref(y->r), A) && fmpz_is_one(y->u.c));
        fmpz_set_si(A, -1);
        fmpz_set_ui(d, 3);
        fmpz_pow_ui(d, d, 50);
        ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
        adf_adele_set_arb_fball(x, inf, f);
        ADF_CHECK(adf_idele_set_adele(y, x) == ADF_OK && fmpz_equal(fmpq_denref(y->r), d)
                  && fmpz_equal_si(y->u.c, -1) && arb_is_negative(y->inf));
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
    }
    /* a local finite part is never exact: UNIT_NOT_CERTIFIED */
    {
        adf_modctx_struct * ctx;
        static const ulong q[] = { 8, 9, 5 };
        ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 3) == ADF_OK);
        fball_set_si3(f, 7, 12, 1);
        ADF_CHECK(adf_fball_set_local(f, f, ctx) == ADF_OK);
        ADF_CHECK(adf_adele_set_arb_fball(x, inf, f) == ADF_OK);
        adf_idele_set(y, keep);
        ADF_CHECK(adf_idele_set_adele(y, x) == ADF_UNIT_NOT_CERTIFIED && adf_idele_identical(y, keep));
        adf_fball_clear(f);
        adf_fball_init(f);
        adf_adele_clear(x);
        adf_adele_init(x);
        adf_modctx_free(ctx);
    }
    fmpq_clear(want);
    arb_clear(inf);
    adf_fball_clear(f);
    adf_adele_clear(x);
    adf_adele_clear(h);
    adf_idele_clear(y);
    adf_idele_clear(keep);
}

/* ---- the division ---- */

/* One case of oracles 2 and 3: x = (inf ; An/Ad + (Bn/Bd) Zhat), y = (yinf, rn/rd, c U(N)). Returns 0 when the
   case is not a canonical input, 1 when it was checked against enumeration, 2 for an exact unit, 3 for a = M = 0,
   4 when the enumeration would be too long (then only oracle 3 and the real part). */
static int
division_case(slong An, slong Ad, slong Bn, slong Bd, slong c, slong N, slong rn, slong rd, const arb_t inf,
              const arb_t yinf)
{
    adf_adele_t x, z, h, w;
    adf_idele_t y, yi;
    adf_fball_t f, t;
    adf_rat_t q;
    fmpq_t r, R, want;
    fmpz_t num, den;
    ulong D, A, B, L, Mod, zz, wv, hull = 0, s0 = 0;
    int first = 1, all_in = 1, kind;
    slong sA;

    if (N >= 1 && gcd_ui((ulong) c, (ulong) N) != 1)
        return 0;
    if (gcd_ui((ulong) (An < 0 ? -An : An), (ulong) Ad) != 1 || gcd_ui((ulong) Bn, (ulong) Bd) != 1)
        return 0;
    adf_adele_init(x);
    adf_adele_init(z);
    adf_adele_init(h);
    adf_adele_init(w);
    adf_idele_init(y);
    adf_idele_init(yi);
    adf_fball_init(f);
    adf_fball_init(t);
    adf_rat_init(q);
    fmpq_init(r);
    fmpq_init(R);
    fmpq_init(want);
    fmpz_init(num);
    fmpz_init(den);
    /* x = (inf ; a + M Zhat), a = An/Ad, M = Bn/Bd, over the common denominator D */
    D = lcm_ui((ulong) Ad, (ulong) Bd);
    sA = An * (slong) (D / (ulong) Ad);
    B = (ulong) Bn * (D / (ulong) Bd);
    fball_set_si3(f, sA, (slong) B, (slong) D);
    ADF_CHECK(adf_adele_set_arb_fball(x, inf, f) == ADF_OK);
    fmpq_set_si(r, rn, (ulong) rd);
    idele_set_parts_q(y, yinf, r, c, N);
    ADF_CHECK(adf_adele_div_idele(z, x, y, 64) == ADF_OK && adf_adele_is_canonical(z));
    ADF_CHECK(real_quotient_ok(z->inf, inf, yinf));
    /* oracle 3: the product rule with the small hull of the inverse */
    ADF_CHECK(adf_idele_inv(yi, y, 64) == ADF_OK);
    adf_adele_set_idele(h, yi);
    adf_fball_mul(t, &x->fin, &h->fin);
    ADF_CHECK_MSG(adf_fball_equal_set(t, &z->fin), "a = %ld/%ld, M = %ld/%ld, (%ld, %ld), r = %ld/%ld", (long) An,
                  (long) Ad, (long) Bn, (long) Bd, (long) c, (long) N, (long) rn, (long) rd);
    if (N == 0)
    {
        /* P18: division by the exact rational c r */
        fmpq_mul_si(q->q, r, c);
        ADF_CHECK(adf_adele_div_rat(w, x, q, 64) == ADF_OK && adf_fball_equal_set(&w->fin, &z->fin));
        kind = 2;
        goto done;
    }
    if (An == 0 && Bn == 0)
    {
        ADF_CHECK(fball_is_si(&z->fin, 0, 0, 1));
        kind = 3;
        goto done;
    }
    /* oracle 2: enumeration modulo Mod */
    A = (ulong) (sA < 0 ? -sA : sA);
    L = lcm_ui((ulong) N, 2);
    Mod = 6;
    if (B)
        Mod = lcm_ui(Mod, B);
    if (A)
        Mod = lcm_ui(Mod, A * L);
    Mod = lcm_ui(Mod, L);
    if ((B ? Mod / B : 1) * (Mod / (ulong) N) > 40000)
    {
        kind = 4;
        goto done;
    }
    for (zz = 0; zz < (B ? Mod / B : 1); zz++)
        for (wv = (ulong) c % (ulong) N; wv < Mod; wv += (ulong) N)
        {
            ulong s, iw;
            if (gcd_ui(wv, Mod) != 1)
                continue;
            iw = inv_mod(wv, Mod);
            /* (sA + B zz) iw mod Mod, sA may be negative */
            s = ((ulong) ((sA % (slong) Mod) + (slong) Mod) + B * zz) % Mod;
            s = (s * iw) % Mod;
            /* the quotient s / (D r) lies in z */
            fmpz_set_ui(num, s);
            fmpz_mul(num, num, fmpq_denref(r));
            fmpz_set_ui(den, D);
            fmpz_mul(den, den, fmpq_numref(r));
            all_in &= fball_has(&z->fin, num, den);
            if (first)
            {
                s0 = s;
                hull = Mod;
                first = 0;
            }
            else
                hull = gcd_ui(hull, s > s0 ? s - s0 : s0 - s);
        }
    ADF_CHECK(all_in);
    /* the radius: hull / (D r) = gcd(|a| L, M) / r (P19) */
    radius_of(R, &z->fin);
    fmpq_set_si(want, (slong) hull, D);
    fmpq_div(want, want, r);
    ADF_CHECK_MSG(fmpq_equal(R, want), "a = %ld/%ld, M = %ld/%ld, (%ld, %ld), r = %ld/%ld", (long) An, (long) Ad,
                  (long) Bn, (long) Bd, (long) c, (long) N, (long) rn, (long) rd);
    kind = 1;
done:
    adf_adele_clear(x);
    adf_adele_clear(z);
    adf_adele_clear(h);
    adf_adele_clear(w);
    adf_idele_clear(y);
    adf_idele_clear(yi);
    adf_fball_clear(f);
    adf_fball_clear(t);
    adf_rat_clear(q);
    fmpq_clear(r);
    fmpq_clear(R);
    fmpq_clear(want);
    fmpz_clear(num);
    fmpz_clear(den);
    return kind;
}

ADF_TEST(division_against_enumeration_and_the_inverse)
{
    adf_adele_t x, z;
    adf_idele_t y;
    adf_fball_t f;
    arb_t inf, yinf;
    fmpq_t r;
    slong An, Ad, Bn, Bd, N, c, rn, rd;
    int n_enum = 0, n_exact = 0, n_zero = 0, n_long = 0;

    adf_adele_init(x);
    adf_adele_init(z);
    adf_idele_init(y);
    adf_fball_init(f);
    arb_init(inf);
    arb_init(yinf);
    fmpq_init(r);
    arb_set_si_si(inf, 5, -2, 3, -1);                   /* 1.25 +- 1.5: contains 0, allowed for an adele */
    arb_set_si_si(yinf, -9, -1, 1, -2);                 /* -4.5 +- 0.25 */
    for (An = -4; An <= 4; An++)
        for (Ad = 1; Ad <= 3; Ad += 2)
            for (Bn = 0; Bn <= 6; Bn += (Bn == 0 ? 2 : (Bn == 2 ? 1 : 3)))       /* 0, 2, 3, 6 */
                for (Bd = 1; Bd <= 2; Bd++)
                    for (N = 0; N <= 12; N += (N < 4 ? 1 : 3))
                        for (c = (N == 0 ? -1 : 1); c <= (N == 0 ? 1 : N); c += (N == 0 ? 2 : 1))
                            for (rn = 1; rn <= 5; rn += 4)
                                for (rd = 1; rd <= 3; rd += 2)
                                    switch (division_case(An, Ad, Bn, Bd, c, N, rn, rd, inf, yinf))
                                    {
                                        case 1: n_enum++; break;
                                        case 2: n_exact++; break;
                                        case 3: n_zero++; break;
                                        case 4: n_long++; break;
                                        default: break;
                                    }
    printf("division: %d cases against enumeration, %d by exact units, %d with a = M = 0, %d too long to "
           "enumerate\n", n_enum, n_exact, n_zero, n_long);
    ADF_CHECK(n_enum > 1000 && n_exact > 50 && n_zero > 10);
    /* examples: (1 + 4 Zhat) / [1 mod 1] = 1 + 2 Zhat (ideles.md P19 example); (0 + 6 Zhat) / (3/2 [5 mod 12]) =
       4 Zhat; (3 + 12 Zhat) / [-1] = 9 + 12 Zhat */
    arb_one(inf);
    fball_set_si3(f, 1, 4, 1);
    adf_adele_set_arb_fball(x, inf, f);
    fmpq_one(r);
    arb_set_si(yinf, 3);
    idele_set_parts_q(y, yinf, r, 1, 1);
    ADF_CHECK(adf_adele_div_idele(z, x, y, 64) == ADF_OK && fball_is_si(&z->fin, 1, 2, 1));
    fball_set_si3(f, 0, 6, 1);
    adf_adele_set_arb_fball(x, inf, f);
    fmpq_set_si(r, 3, 2);
    idele_set_parts_q(y, yinf, r, 5, 12);
    ADF_CHECK(adf_adele_div_idele(z, x, y, 64) == ADF_OK && fball_is_si(&z->fin, 0, 4, 1));
    fball_set_si3(f, 3, 12, 1);
    adf_adele_set_arb_fball(x, inf, f);
    fmpq_one(r);
    idele_set_parts_q(y, yinf, r, -1, 0);
    ADF_CHECK(adf_adele_div_idele(z, x, y, 64) == ADF_OK && fball_is_si(&z->fin, 9, 12, 1));
    fmpq_clear(r);
    arb_clear(inf);
    arb_clear(yinf);
    adf_fball_clear(f);
    adf_adele_clear(x);
    adf_adele_clear(z);
    adf_idele_clear(y);
}

ADF_TEST(division_statuses_aliasing_local_and_prec)
{
    adf_adele_t x, z, keep, a, g;
    adf_idele_t y;
    adf_fball_t f;
    arb_t inf, yinf;
    fmpq_t r;

    adf_adele_init(x);
    adf_adele_init(z);
    adf_adele_init(keep);
    adf_adele_init(a);
    adf_adele_init(g);
    adf_idele_init(y);
    adf_fball_init(f);
    arb_init(inf);
    arb_init(yinf);
    fmpq_init(r);
    arb_set_si_si(inf, 7, -3, 1, -5);
    fball_set_si3(f, 5, 18, 1);
    ADF_CHECK(adf_adele_set_arb_fball(x, inf, f) == ADF_OK);
    arb_set_si_si(yinf, -3, 0, 1, -3);
    fmpq_set_si(r, 10, 3);
    idele_set_parts_q(y, yinf, r, 7, 30);
    arb_set_si(keep->inf, 11);
    adf_adele_set(z, keep);
    /* LIMIT: untouched, also aliased */
    ADF_CHECK(adf_adele_div_idele(z, x, y, ADF_IDELE_PREC_MAX + 1) == ADF_LIMIT && adf_adele_identical(z, keep));
    ADF_CHECK(adf_adele_div_idele(z, x, y, WORD_MAX) == ADF_LIMIT && adf_adele_identical(z, keep));
    adf_adele_set(a, x);
    ADF_CHECK(adf_adele_div_idele(a, a, y, WORD_MAX) == ADF_LIMIT && adf_adele_identical(a, x));
    /* aliasing z = x: the same result */
    ADF_CHECK(adf_adele_div_idele(z, x, y, 64) == ADF_OK);
    adf_adele_set(a, x);
    ADF_CHECK(adf_adele_div_idele(a, a, y, 64) == ADF_OK && adf_adele_identical(a, z));
    ADF_CHECK(real_quotient_ok(z->inf, inf, yinf));
    /* (5 + 18 Zhat) / (10/3 [7 mod 30]): e odd = 7^-1 = 13 mod 30, L = 30: (65 + gcd(150, 18) Zhat) 3/10 =
       39/2 + 9/5 Zhat = (195 + 18 Zhat)/10, canonical (15 + 18 Zhat)/10 */
    ADF_CHECK(fball_is_si(&z->fin, 15, 18, 10));
    /* prec below 2 is 2 */
    ADF_CHECK(adf_adele_div_idele(a, x, y, 2) == ADF_OK);
    ADF_CHECK(adf_adele_div_idele(z, x, y, 0) == ADF_OK && adf_adele_identical(z, a));
    ADF_CHECK(adf_adele_div_idele(z, x, y, WORD_MIN) == ADF_OK && adf_adele_identical(z, a));
    /* at the limit itself: OK */
    ADF_CHECK(adf_adele_div_idele(z, x, y, ADF_IDELE_PREC_MAX) == ADF_OK && real_quotient_ok(z->inf, inf, yinf));
    /* a divisor ball near 0: Y = [1 + 2^-100 +- 1]: arb_div gives a finite (wide) ball, OK */
    {
        arf_t m;
        arf_init(m);
        arf_one(m);
        arf_mul_2exp_si(m, m, -100);
        arf_add_ui(arb_midref(yinf), m, 1, ARF_PREC_EXACT, ARF_RND_DOWN);
        mag_one(arb_radref(yinf));
        ADF_CHECK(arb_is_nonzero(yinf));
        fmpq_one(r);
        idele_set_parts_q(y, yinf, r, 1, 0);
        ADF_CHECK(adf_adele_div_idele(z, x, y, 64) == ADF_OK && real_quotient_ok(z->inf, inf, yinf));
        arf_clear(m);
    }
    /* a local finite part of x: the result is global and the same set as for the global x */
    {
        adf_modctx_struct * ctx;
        static const ulong q[] = { 8, 9, 5 };
        adf_fball_t lf;
        adf_fball_init(lf);
        ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 3) == ADF_OK);
        fball_set_si3(f, 7, 12, 1);
        ADF_CHECK(adf_fball_set_local(lf, f, ctx) == ADF_OK);
        ADF_CHECK(adf_adele_set_arb_fball(a, inf, lf) == ADF_OK && adf_adele_set_arb_fball(g, inf, f) == ADF_OK);
        arb_set_si(yinf, -3);
        fmpq_set_si(r, 5, 4);
        idele_set_parts_q(y, yinf, r, 5, 12);
        ADF_CHECK(adf_adele_div_idele(z, a, y, 64) == ADF_OK && !adf_fball_is_local(&z->fin));
        ADF_CHECK(adf_adele_div_idele(g, g, y, 64) == ADF_OK && adf_fball_equal_set(&z->fin, &g->fin));
        /* the output was local: it is overwritten by a global value */
        ADF_CHECK(adf_adele_div_idele(a, a, y, 64) == ADF_OK && !adf_fball_is_local(&a->fin)
                  && adf_fball_equal_set(&a->fin, &g->fin));
        adf_fball_clear(lf);
        adf_adele_clear(a);
        adf_adele_init(a);
        adf_modctx_free(ctx);
    }
    fmpq_clear(r);
    arb_clear(inf);
    arb_clear(yinf);
    adf_fball_clear(f);
    adf_adele_clear(x);
    adf_adele_clear(z);
    adf_adele_clear(keep);
    adf_adele_clear(a);
    adf_adele_clear(g);
    adf_idele_clear(y);
}

ADF_TEST(division_by_an_exact_rational_is_P18)
{
    /* adf_adele_div_rat (adele.h) against ideles.md P18: a/q + (M/|q|) Zhat, negative q included */
    static const slong qs[][2] = { { -3, 2 }, { 5, 7 }, { -1, 1 }, { 12, 1 }, { -2, 9 } };
    adf_adele_t x, z;
    adf_fball_t f, want;
    adf_rat_t q, c, R;
    arb_t inf;
    size_t i;

    adf_adele_init(x);
    adf_adele_init(z);
    adf_fball_init(f);
    adf_fball_init(want);
    adf_rat_init(q);
    adf_rat_init(c);
    adf_rat_init(R);
    arb_init(inf);
    arb_set_si(inf, 3);
    fball_set_si3(f, 5, 18, 4);                          /* 5/4 + 9/2 Zhat */
    ADF_CHECK(adf_adele_set_arb_fball(x, inf, f) == ADF_OK);
    for (i = 0; i < sizeof(qs) / sizeof(qs[0]); i++)
    {
        fmpq_set_si(q->q, qs[i][0], (ulong) qs[i][1]);
        ADF_CHECK(adf_adele_div_rat(z, x, q, 64) == ADF_OK);
        fmpq_set_si(c->q, 5, 4);
        fmpq_div(c->q, c->q, q->q);
        fmpq_set_si(R->q, 9, 2);
        fmpq_div(R->q, R->q, q->q);
        fmpq_abs(R->q, R->q);
        ADF_CHECK(adf_fball_set_center_radius(want, c, R) == ADF_OK && adf_fball_equal_set(&z->fin, want));
    }
    arb_clear(inf);
    adf_rat_clear(q);
    adf_rat_clear(c);
    adf_rat_clear(R);
    adf_fball_clear(f);
    adf_fball_clear(want);
    adf_adele_clear(x);
    adf_adele_clear(z);
}

/* ---- the vectors ---- */

ADF_TEST(vectors_of_the_reference)
{
    jsonl_file * f;
    jsonl_error_t err;
    size_t i, n_hull = 0, n_set = 0, n_div = 0, n_limit = 0, n_refused = 0;
    adf_idele_t x, y, ykeep;
    adf_adele_t a, z, zkeep;
    fmpz_t A, H, d;
    fmpq_t r;
    adf_ucoset_t u;

    if (!jsonl_open("tests/ref/vectors/i-slice3/idmap.jsonl", &f, &err))
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    adf_idele_init(x);
    adf_idele_init(y);
    adf_idele_init(ykeep);
    adf_adele_init(a);
    adf_adele_init(z);
    adf_adele_init(zkeep);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpq_init(r);
    adf_ucoset_init(u);
    arb_set_si(ykeep->inf, 13);
    arb_set_si(zkeep->inf, -13);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * op = str_of(rec, "op");
        if (strcmp(op, "hull") == 0)
        {
            read_idele(x, field(rec, "x"));
            adf_adele_set_idele(a, x);
            read_triple(A, H, d, field(rec, "small"));
            ADF_CHECK_MSG(fball_is(&a->fin, A, H, d) && arb_equal(a->inf, x->inf), "line %zu", i + 1);
            adf_adele_set_idele_simple(a, x);
            read_triple(A, H, d, field(rec, "simple"));
            ADF_CHECK_MSG(fball_is(&a->fin, A, H, d) && arb_equal(a->inf, x->inf), "line %zu", i + 1);
            n_hull++;
        }
        else if (strcmp(op, "set_adele") == 0)
        {
            int want = status_of(str_of(rec, "status")), st;
            read_adele(a, field(rec, "x"));
            adf_idele_set(y, ykeep);
            st = adf_idele_set_adele(y, a);
            ADF_CHECK_MSG(st == want, "line %zu: status %d, want %d", i + 1, st, want);
            if (want == ADF_OK && st == ADF_OK)
            {
                read_fmpq(r, field(rec, "r"));
                read_uc(u, field(rec, "u"));
                ADF_CHECK_MSG(adf_idele_is_canonical(y) && fmpq_equal(y->r, r) && adf_ucoset_identical(&y->u, u)
                              && arb_equal(y->inf, a->inf), "line %zu", i + 1);
            }
            else
            {
                ADF_CHECK_MSG(adf_idele_identical(y, ykeep), "line %zu: output touched", i + 1);
                n_refused++;
            }
            n_set++;
        }
        else if (strcmp(op, "div") == 0)
        {
            int want = status_of(str_of(rec, "status")), st;
            slong prec = read_si(field(rec, "prec"));
            read_adele(a, field(rec, "x"));
            read_idele(y, field(rec, "y"));
            adf_adele_set(z, zkeep);
            st = adf_adele_div_idele(z, a, y, prec);
            ADF_CHECK_MSG(st == want, "line %zu: status %d, want %d", i + 1, st, want);
            if (want == ADF_OK && st == ADF_OK)
            {
                read_triple(A, H, d, field(rec, "fin"));
                ADF_CHECK_MSG(fball_is(&z->fin, A, H, d) && real_quotient_ok(z->inf, a->inf, y->inf)
                              && adf_adele_is_canonical(z), "line %zu", i + 1);
                /* aliased */
                ADF_CHECK(adf_adele_div_idele(a, a, y, prec) == ADF_OK && adf_adele_identical(a, z));
            }
            else
            {
                ADF_CHECK_MSG(adf_adele_identical(z, zkeep), "line %zu: output touched", i + 1);
                n_limit += want == ADF_LIMIT;
            }
            n_div++;
        }
        else
            ADF_CHECK_MSG(0, "unknown op %s on line %zu", op, i + 1);
    }
    printf("vectors: %zu lines: %zu hull, %zu set_adele (%zu refused), %zu div (%zu LIMIT)\n", jsonl_count(f),
           n_hull, n_set, n_refused, n_div, n_limit);
    ADF_CHECK(n_hull + n_set + n_div == jsonl_count(f) && n_refused > 40 && n_div > 400);
    jsonl_close(f);
    adf_ucoset_clear(u);
    fmpq_clear(r);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_idele_clear(x);
    adf_idele_clear(y);
    adf_idele_clear(ykeep);
    adf_adele_clear(a);
    adf_adele_clear(z);
    adf_adele_clear(zkeep);
}

/* ---- LIMIT is decided from prec alone, before any allocation, also under ADF_CHECK_INVARIANTS (lane
   i-repair1, finding F1 of docs/reviews/m2/review-slices-2-3.md; SPEC 15.4 N-D8). The oracle is the FLINT
   allocator: refs/src/flint-3.0.1/memory.rst:16-21. The values have a content and a modulus of thousands of
   bits, so that the entry check of the debug build (integer gcds) allocates. ---- */

static void *(*orig_malloc)(size_t);
static void *(*orig_realloc)(void *, size_t);
static void *(*orig_calloc)(size_t, size_t);
static void (*orig_free)(void *);
static size_t alloc_calls;
static int alloc_tracking;

static void *
hook_malloc(size_t n)
{
    alloc_calls += alloc_tracking;
    return orig_malloc(n);
}

static void *
hook_realloc(void * p, size_t n)
{
    alloc_calls += alloc_tracking;
    return orig_realloc(p, n);
}

static void *
hook_calloc(size_t n, size_t s)
{
    alloc_calls += alloc_tracking;
    return orig_calloc(n, s);
}

static void
hook_free(void * p)
{
    orig_free(p);
}

static void
hooks_on(void)
{
    __flint_get_memory_functions(&orig_malloc, &orig_calloc, &orig_realloc, &orig_free);
    __flint_set_memory_functions(hook_malloc, hook_calloc, hook_realloc, hook_free);
}

static void
hooks_off(void)
{
    __flint_set_memory_functions(orig_malloc, orig_calloc, orig_realloc, orig_free);
}

static void
count_begin(void)
{
    flint_cleanup();
    alloc_calls = 0;
    alloc_tracking = 1;
}

static size_t
count_end(void)
{
    alloc_tracking = 0;
    return alloc_calls;
}

static void
set_big_idele(adf_idele_t x)
{
    fmpz_one(fmpq_numref(x->r));
    fmpz_mul_2exp(fmpq_numref(x->r), fmpq_numref(x->r), 4096);
    fmpz_add_ui(fmpq_numref(x->r), fmpq_numref(x->r), 3);
    fmpz_one(fmpq_denref(x->r));
    fmpz_mul_2exp(fmpq_denref(x->r), fmpq_denref(x->r), 2048);
    fmpz_add_ui(fmpq_denref(x->r), fmpq_denref(x->r), 7);
    fmpq_canonicalise(x->r);
    fmpz_one(x->u.c);
    fmpz_one(x->u.N);
    fmpz_mul_2exp(x->u.N, x->u.N, 2048);
    fmpz_add_ui(x->u.N, x->u.N, 1);
}

ADF_TEST(LIMIT_before_any_allocation_of_the_division)
{
    adf_idele_t y;
    adf_adele_t a, z;
    size_t calls[2];
    int st[2], i;

    adf_idele_init(y);
    adf_adele_init(a);
    adf_adele_init(z);
    set_big_idele(y);
    ADF_CHECK(adf_idele_is_canonical(y) && adf_adele_is_canonical(a));
    hooks_on();
    for (i = 0; i < 2; i++)
    {
        count_begin();
        st[i] = adf_adele_div_idele(z, a, y, i == 0 ? ADF_IDELE_PREC_MAX + 1 : WORD_MAX);
        calls[i] = count_end();
    }
    hooks_off();
    for (i = 0; i < 2; i++)
        ADF_CHECK_MSG(st[i] == ADF_LIMIT && calls[i] == 0, "call %d: status %d, %zu allocator calls", i, st[i],
                      calls[i]);
    adf_idele_clear(y);
    adf_adele_clear(a);
    adf_adele_clear(z);
}
