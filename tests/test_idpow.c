/* tests/test_idpow.c: integer powers of unit cosets, ideles and idele classes (lane i-slice3;
   include/adelefeld/idpow.h; docs/api-2.md 3.3, Statements J, K, L; docs/proofs/ideles.md P13).

   The oracles:
   1. Enumeration in Z/M. For a coset x = c U(N) and a level M (a multiple of N and of the modulus of the result,
      with one more power of every prime p <= max(|k| + 2, 7) and of every prime of N), the image of P_k in
      (Z/M)^x is {w^k mod M : w in the image of x}. Its hull modulus D (the gcd of M and the differences) has the
      canonical form M_k of P13 exactly when the result of pow_tight is the smallest coset (P13.3); every w^k must
      lie in the default coset of pow and in the tight coset. The default coset must equal the product of |k|
      copies of x by adf_ucoset_mul (inverted for k < 0), the rule of slice 1.
   2. Exact rational arithmetic (fmpq) for the real part: the set of |xi|^k over the interval
      [|m| - rho, |m| + rho] is [L, H] with L, H exact powers of the ends; a result must contain sign * L and
      sign * H, exclude 0, and have the sign of X^k.
   3. tests/ref/vectors/i-slice3/idpow.jsonl, written by lanes/i-slice3/gen_vectors.py from part 4 of
      proto/ideles_checks.py (checked there against the table M_k of part 1, enumeration and exact points). It
      gives the status (a function of the correctly rounded ends, Statement K.3), the rounded ends lo, hi, the
      exact ends L, H, the content and both units. Every line is run. */

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

/* The ball [m1 2^e1 +- m2 2^e2], m2 < 2^30, exactly (checked). */
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

static int
uc_is(const adf_ucoset_t u, slong c, slong N)
{
    return fmpz_equal_si(u->c, c) && fmpz_equal_si(u->N, N);
}

static void
idele_set_parts_si(adf_idele_t x, const arb_t inf, slong rn, slong rd, slong c, slong N)
{
    fmpq_t r;
    adf_ucoset_t u;
    fmpq_init(r);
    adf_ucoset_init(u);
    fmpq_set_si(r, rn, (ulong) rd);
    uc_set_si(u, c, N);
    if (adf_idele_set_parts(x, inf, r, u) != ADF_OK)
    {
        printf("idele_set_parts_si: refused\n");
        abort();
    }
    fmpq_clear(r);
    adf_ucoset_clear(u);
}

static void
idclass_set_parts_si(adf_idclass_t x, const arb_t t, slong c, slong N)
{
    adf_ucoset_t u;
    adf_ucoset_init(u);
    uc_set_si(u, c, N);
    if (adf_idclass_set_parts(x, t, u) != ADF_OK)
    {
        printf("idclass_set_parts_si: refused\n");
        abort();
    }
    adf_ucoset_clear(u);
}

/* The exact end points of abs(X). */
static void
abs_ends(fmpq_t lo, fmpq_t hi, const arb_t x)
{
    fmpq_t m, r;
    arf_t t;
    fmpq_init(m);
    fmpq_init(r);
    arf_init(t);
    arf_get_fmpq(m, arb_midref(x));
    fmpq_abs(m, m);
    arf_set_mag(t, arb_radref(x));
    arf_get_fmpq(r, t);
    fmpq_sub(lo, m, r);
    fmpq_add(hi, m, r);
    fmpq_clear(m);
    fmpq_clear(r);
    arf_clear(t);
}

/* 1 if z contains s * [L, H] and not 0, is finite, and has the sign s. */
static int
encloses(const arb_t z, const fmpq_t L, const fmpq_t H, int s)
{
    fmpq_t a, b;
    int ok;
    fmpq_init(a);
    fmpq_init(b);
    fmpq_set(a, L);
    fmpq_set(b, H);
    if (s < 0)
    {
        fmpq_neg(a, a);
        fmpq_neg(b, b);
    }
    ok = arb_contains_fmpq(z, a) && arb_contains_fmpq(z, b) && arb_is_nonzero(z) && arb_is_finite(z)
         && arf_sgn(arb_midref(z)) == s;
    fmpq_clear(a);
    fmpq_clear(b);
    return ok;
}

/* Oracle 2: z encloses {xi^k : xi in X} (k != 0, small) with the sign sign(X)^k. */
static int
encloses_power(const arb_t z, const arb_t x, slong k)
{
    fmpq_t L, H;
    int ok, s;
    fmpq_init(L);
    fmpq_init(H);
    abs_ends(L, H, x);
    fmpq_pow_si(L, L, k);
    fmpq_pow_si(H, H, k);
    s = (k % 2 != 0 && arf_sgn(arb_midref(x)) < 0) ? -1 : 1;
    ok = k > 0 ? encloses(z, L, H, s) : encloses(z, H, L, s);
    fmpq_clear(L);
    fmpq_clear(H);
    return ok;
}

/* ---- oracle 1: enumeration in Z/M, M < 2^19, so a product of two residues fits in a word ---- */

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

static ulong
pow_mod(ulong w, slong k, ulong m)
{
    ulong r = 1 % m, n;
    if (k < 0)
        w = inv_mod(w, m);
    n = k < 0 ? (ulong) (-k) : (ulong) k;
    w %= m;
    while (n)
    {
        if (n & 1)
            r = (r * w) % m;
        w = (w * w) % m;
        n >>= 1;
    }
    return r;
}

/* 1 if the residue w (a unit modulo M, N | M) lies in the set of the unit value u. */
static int
level_in(ulong w, const adf_ucoset_t u, ulong M)
{
    slong c = fmpz_get_si(u->c);
    ulong N = fmpz_get_ui(u->N);
    if (N == 0)
        return w == (ulong) ((c % (slong) M + (slong) M) % (slong) M);
    return M % N == 0 && w % N == (ulong) c % N;
}

/* the canonical modulus of D (Definition 8): D/2 if D = 2 mod 4 */
static ulong
canon_mod(ulong D)
{
    return D % 4 == 2 ? D / 2 : D;
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

static int
has_field(const jsonl_value * rec, const char * key)
{
    const jsonl_value * v;
    jsonl_error_t err;
    return jsonl_field(rec, key, &v, &err);
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
read_arf(arf_t x, const jsonl_value * v)
{
    fmpz_t m, e;
    fmpz_init(m);
    fmpz_init(e);
    read_fmpz_at(m, v, 0);
    read_fmpz_at(e, v, 1);
    arf_set_fmpz_2exp(x, m, e);
    fmpz_clear(m);
    fmpz_clear(e);
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
    {
        printf("read_uc: not canonical\n");
        abort();
    }
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
read_class(adf_idclass_t x, const jsonl_value * v)
{
    adf_ucoset_t u;
    arb_t t;
    adf_ucoset_init(u);
    arb_init(t);
    read_ball(t, v);
    read_uc(u, field(v, "u"));
    if (adf_idclass_set_parts(x, t, u) != ADF_OK || !adf_idclass_is_canonical(x))
    {
        printf("read_class: the input is not a class\n");
        abort();
    }
    adf_ucoset_clear(u);
    arb_clear(t);
}

static int
status_of(const char * s)
{
    if (strcmp(s, "OK") == 0)
        return ADF_OK;
    if (strcmp(s, "NOT_DETERMINED") == 0)
        return ADF_NOT_DETERMINED;
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

/* ---- unit cosets ---- */

ADF_TEST(ucoset_pow_against_enumeration)
{
    /* Oracle 1 for every coset with N <= 12 and the exponents -4 .. 4 and 6: the tight coset is the smallest
       (its modulus is the canonical hull modulus of the enumerated powers), both contain every power, the
       default equals the product of |k| copies. */
    static const slong ks[] = { -4, -3, -2, -1, 1, 2, 3, 4, 6 };
    static const ulong small_primes[] = { 2, 3, 5, 7, 11 };
    adf_ucoset_t x, y, t, prod;
    slong N, c;
    size_t i, j;
    int n_enum = 0, n_skip = 0;

    adf_ucoset_init(x);
    adf_ucoset_init(y);
    adf_ucoset_init(t);
    adf_ucoset_init(prod);
    for (N = 1; N <= 12; N++)
        for (c = 1; c <= N; c++)
        {
            if (gcd_ui((ulong) c, (ulong) N) != 1)
                continue;
            uc_set_si(x, c, N);
            for (i = 0; i < sizeof(ks) / sizeof(ks[0]); i++)
            {
                slong k = ks[i], e;
                ulong Mt, M, extra = 4, w, D = 0, w0 = 0;
                int first = 1, all_in = 1;
                adf_ucoset_pow(y, x, k);
                adf_ucoset_pow_tight(t, x, k);
                ADF_CHECK_MSG(adf_ucoset_is_normal(y) && adf_ucoset_is_normal(t), "(%ld, %ld)^%ld", (long) c,
                              (long) N, (long) k);
                ADF_CHECK(adf_ucoset_contains(t, y));
                /* the default is the product of |k| copies (slice 1), inverted for k < 0 */
                adf_ucoset_set(prod, x);
                for (e = 1; e < (k < 0 ? -k : k); e++)
                    adf_ucoset_mul(prod, prod, x);
                if (k < 0)
                    adf_ucoset_inv(prod, prod);
                ADF_CHECK_MSG(adf_ucoset_equal_set(prod, y), "(%ld, %ld)^%ld", (long) c, (long) N, (long) k);
                if (k == 1 || k == -1)
                    ADF_CHECK(adf_ucoset_identical(t, y));
                /* the level */
                if (!fmpz_abs_fits_ui(t->N) || fmpz_cmp_ui(t->N, 100000) > 0)
                {
                    n_skip++;
                    continue;
                }
                Mt = fmpz_get_ui(t->N);
                for (j = 0; j < sizeof(small_primes) / sizeof(small_primes[0]); j++)
                    if (small_primes[j] <= (ulong) ((k < 0 ? -k : k) + 2) || small_primes[j] <= 7
                        || N % (slong) small_primes[j] == 0)
                        extra *= small_primes[j];
                M = Mt / gcd_ui(Mt, (ulong) N) * (ulong) N * extra;
                if (M > 400000)
                {
                    n_skip++;
                    continue;
                }
                for (w = (ulong) c % (ulong) N; w < M; w += (ulong) N)       /* the residues c mod N */
                {
                    ulong v;
                    if (gcd_ui(w, M) != 1)
                        continue;
                    v = pow_mod(w, k, M);
                    all_in &= level_in(v, t, M) && level_in(v, y, M);
                    if (first)
                    {
                        w0 = v;
                        D = M;
                        first = 0;
                    }
                    else
                        D = gcd_ui(D, v > w0 ? v - w0 : w0 - v);
                }
                ADF_CHECK_MSG(all_in, "(%ld, %ld)^%ld: a power outside a result", (long) c, (long) N, (long) k);
                ADF_CHECK_MSG(canon_mod(D) == Mt, "(%ld, %ld)^%ld: hull modulus %lu, tight modulus %lu", (long) c,
                              (long) N, (long) k, (unsigned long) canon_mod(D), (unsigned long) Mt);
                n_enum++;
            }
        }
    printf("ucoset powers against enumeration: %d cases, %d levels too large (checked by the vectors)\n", n_enum,
           n_skip);
    ADF_CHECK(n_enum > 150);
    adf_ucoset_clear(x);
    adf_ucoset_clear(y);
    adf_ucoset_clear(t);
    adf_ucoset_clear(prod);
}

ADF_TEST(ucoset_pow_hand_cases_and_aliasing)
{
    adf_ucoset_t x, y, t;
    fmpz_t M;

    adf_ucoset_init(x);
    adf_ucoset_init(y);
    adf_ucoset_init(t);
    fmpz_init(M);
    /* squares of units: M_2 = 24 for N = 1 (P13.5) */
    uc_set_si(x, 1, 1);
    adf_ucoset_pow_tight(t, x, 2);
    ADF_CHECK(uc_is(t, 1, 24));
    adf_ucoset_pow(y, x, 2);
    ADF_CHECK(uc_is(y, 1, 1));
    /* [2 mod 5]^2: default [4 mod 5], tight [49 mod 120] */
    uc_set_si(x, 2, 5);
    adf_ucoset_pow(y, x, 2);
    adf_ucoset_pow_tight(t, x, 2);
    ADF_CHECK(uc_is(y, 4, 5) && uc_is(t, 49, 120));
    /* k = 1, -1: M_k = Nbar, not N (P13.4; finding 1 of i-slice1 against PLAN 2.1) */
    uc_set_si(x, 5, 6);
    adf_ucoset_pow_tight(t, x, 1);
    ADF_CHECK(uc_is(t, 2, 3));
    adf_ucoset_pow(y, x, 1);
    ADF_CHECK(uc_is(y, 2, 3));
    adf_ucoset_pow_tight(t, x, -1);
    ADF_CHECK(uc_is(t, 2, 3));
    uc_set_si(x, 7, 12);
    adf_ucoset_pow_tight(t, x, -1);
    ADF_CHECK(uc_is(t, 7, 12));
    /* k = 0: the exact unit 1, from a coset and from the exact units */
    adf_ucoset_pow(y, x, 0);
    adf_ucoset_pow_tight(t, x, 0);
    ADF_CHECK(uc_is(y, 1, 0) && uc_is(t, 1, 0));
    adf_ucoset_minus_one(x);
    adf_ucoset_pow(y, x, 0);
    ADF_CHECK(uc_is(y, 1, 0));
    /* exact units: [-1]^3 = [-1], [-1]^-2 = [1], [-1]^WORD_MIN = [1], [-1]^WORD_MAX = [-1] */
    adf_ucoset_pow(y, x, 3);
    adf_ucoset_pow_tight(t, x, 3);
    ADF_CHECK(uc_is(y, -1, 0) && uc_is(t, -1, 0));
    adf_ucoset_pow(y, x, -2);
    ADF_CHECK(uc_is(y, 1, 0));
    adf_ucoset_pow_tight(t, x, WORD_MIN);
    ADF_CHECK(uc_is(t, 1, 0));
    adf_ucoset_pow(y, x, WORD_MAX);
    ADF_CHECK(uc_is(y, -1, 0));
    /* WORD_MIN on [3 mod 7]: 3 has order 6 and -2^63 = 4 mod 6, so 3^4 = 4 mod 7 */
    uc_set_si(x, 3, 7);
    adf_ucoset_pow(y, x, WORD_MIN);
    ADF_CHECK(uc_is(y, 4, 7));
    /* the tight power of U(1) for k = WORD_MIN = -2^63: 2^(2 + 63) times the Fermat primes 3, 5, 17, 257, 65537
       (the p with p - 1 | 2^63) */
    uc_set_si(x, 1, 1);
    adf_ucoset_pow_tight(t, x, WORD_MIN);
    fmpz_set_ui(M, UWORD(3) * 5 * 17 * 257 * 65537);
    fmpz_mul_2exp(M, M, 65);
    ADF_CHECK(fmpz_equal(t->N, M) && fmpz_is_one(t->c));
    /* [3 mod 4]^(2^62): A = 4 * 2^62, B = 3 * 5 * 17 * 257 * 65537; the residue is 1 modulo both */
    uc_set_si(x, 3, 4);
    adf_ucoset_pow_tight(t, x, WORD(1) << 62);
    fmpz_set_ui(M, UWORD(3) * 5 * 17 * 257 * 65537);
    fmpz_mul_2exp(M, M, 64);
    ADF_CHECK(fmpz_equal(t->N, M) && fmpz_is_one(t->c));
    adf_ucoset_pow(y, x, WORD(1) << 62);
    ADF_CHECK(uc_is(y, 1, 4));
    /* aliasing: y = x */
    uc_set_si(x, 2, 5);
    adf_ucoset_pow(x, x, 2);
    ADF_CHECK(uc_is(x, 4, 5));
    uc_set_si(x, 2, 5);
    adf_ucoset_pow_tight(x, x, 2);
    ADF_CHECK(uc_is(x, 49, 120));
    uc_set_si(x, 5, 6);
    adf_ucoset_pow_tight(x, x, 1);
    ADF_CHECK(uc_is(x, 2, 3));
    /* a modulus of 300 bits: the power is taken modulo it, not reduced to a word */
    fmpz_one(M);
    fmpz_mul_2exp(M, M, 300);
    fmpz_add_ui(M, M, 1);               /* odd */
    fmpz_set_ui(x->c, 2);
    fmpz_set(x->N, M);
    ADF_CHECK(adf_ucoset_is_canonical(x));
    adf_ucoset_pow(y, x, 301);
    {
        fmpz_t want;
        fmpz_init(want);
        fmpz_set_ui(want, 2);
        fmpz_powm_ui(want, want, 301, M);
        ADF_CHECK(fmpz_equal(y->c, want) && fmpz_equal(y->N, M));
        fmpz_clear(want);
    }
    fmpz_clear(M);
    adf_ucoset_clear(x);
    adf_ucoset_clear(y);
    adf_ucoset_clear(t);
}

/* ---- the vectors ---- */

ADF_TEST(vectors_of_the_reference)
{
    jsonl_file * f;
    jsonl_error_t err;
    size_t i, n_uc = 0, n_idele = 0, n_class = 0, n_nd = 0, n_limit = 0, n_exact = 0, n_ends = 0;
    adf_ucoset_t u, want, want_t, y;
    adf_idele_t x, z, zt, keep;
    adf_idclass_t cx, cz, czt, ckeep;
    fmpq_t r, L, H;
    arf_t lo, hi;

    if (!jsonl_open("tests/ref/vectors/i-slice3/idpow.jsonl", &f, &err))
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    adf_ucoset_init(u);
    adf_ucoset_init(want);
    adf_ucoset_init(want_t);
    adf_ucoset_init(y);
    adf_idele_init(x);
    adf_idele_init(z);
    adf_idele_init(zt);
    adf_idele_init(keep);
    adf_idclass_init(cx);
    adf_idclass_init(cz);
    adf_idclass_init(czt);
    adf_idclass_init(ckeep);
    fmpq_init(r);
    fmpq_init(L);
    fmpq_init(H);
    arf_init(lo);
    arf_init(hi);
    arb_set_si(keep->inf, -5);
    arb_set_si(ckeep->t, 7);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * op = str_of(rec, "op");
        slong k = read_si(field(rec, "k"));

        if (strcmp(op, "uc_pow") == 0)
        {
            read_uc(u, field(rec, "u"));
            read_uc(want, field(rec, "pow"));
            read_uc(want_t, field(rec, "tight"));
            adf_ucoset_pow(y, u, k);
            ADF_CHECK_MSG(adf_ucoset_identical(y, want), "line %zu", i + 1);
            adf_ucoset_pow_tight(y, u, k);
            ADF_CHECK_MSG(adf_ucoset_identical(y, want_t), "line %zu", i + 1);
            adf_ucoset_set(y, u);
            adf_ucoset_pow_tight(y, y, k);
            ADF_CHECK_MSG(adf_ucoset_identical(y, want_t), "line %zu (aliased)", i + 1);
            adf_ucoset_set(y, u);
            adf_ucoset_pow(y, y, k);
            ADF_CHECK_MSG(adf_ucoset_identical(y, want), "line %zu (aliased)", i + 1);
            n_uc++;
            continue;
        }
        {
            slong prec = read_si(field(rec, "prec"));
            int want_st = status_of(str_of(rec, "status")), st, st_t, is_idele = strcmp(op, "idele_pow") == 0;
            arb_srcptr res, res_t;
            if (is_idele)
            {
                read_idele(x, field(rec, "x"));
                adf_idele_set(z, keep);
                adf_idele_set(zt, keep);
                st = adf_idele_pow(z, x, k, prec);
                st_t = adf_idele_pow_tight(zt, x, k, prec);
                res = z->inf;
                res_t = zt->inf;
                n_idele++;
            }
            else if (strcmp(op, "idclass_pow") == 0)
            {
                read_class(cx, field(rec, "x"));
                adf_idclass_set(cz, ckeep);
                adf_idclass_set(czt, ckeep);
                st = adf_idclass_pow(cz, cx, k, prec);
                st_t = adf_idclass_pow_tight(czt, cx, k, prec);
                res = cz->t;
                res_t = czt->t;
                n_class++;
            }
            else
            {
                ADF_CHECK_MSG(0, "unknown op %s on line %zu", op, i + 1);
                continue;
            }
            ADF_CHECK_MSG(st == want_st && st_t == want_st, "line %zu: status %d, %d, want %d", i + 1, st, st_t,
                          want_st);
            if (st != ADF_OK || want_st != ADF_OK)
            {
                if (is_idele)
                    ADF_CHECK_MSG(adf_idele_identical(z, keep) && adf_idele_identical(zt, keep),
                                  "line %zu: output touched", i + 1);
                else
                    ADF_CHECK_MSG(adf_idclass_identical(cz, ckeep) && adf_idclass_identical(czt, ckeep),
                                  "line %zu: output touched", i + 1);
                n_nd += want_st == ADF_NOT_DETERMINED;
                n_limit += want_st == ADF_LIMIT;
                /* aliased: the input stays */
                if (is_idele)
                {
                    adf_idele_set(z, x);
                    ADF_CHECK(adf_idele_pow(z, z, k, prec) == want_st && adf_idele_identical(z, x));
                }
                else
                {
                    adf_idclass_set(cz, cx);
                    ADF_CHECK(adf_idclass_pow_tight(cz, cz, k, prec) == want_st && adf_idclass_identical(cz, cx));
                }
                continue;
            }
            {
                slong s = read_si(field(rec, "sign"));
                read_arf(lo, field(rec, "lo"));
                read_arf(hi, field(rec, "hi"));
                if (has_field(rec, "L"))
                {
                    read_fmpq(L, field(rec, "L"));
                    read_fmpq(H, field(rec, "H"));
                    ADF_CHECK_MSG(encloses(res, L, H, (int) s), "line %zu", i + 1);
                    n_ends++;
                }
                if (s < 0)
                {
                    arf_neg(lo, lo);
                    arf_neg(hi, hi);
                }
                ADF_CHECK_MSG(arb_contains_arf(res, lo) && arb_contains_arf(res, hi) && arb_is_nonzero(res)
                              && arf_sgn(arb_midref(res)) == s, "line %zu", i + 1);
                if (arf_equal(lo, hi))
                {
                    ADF_CHECK_MSG(arb_is_exact(res) && arf_equal(arb_midref(res), lo), "line %zu", i + 1);
                    n_exact++;
                }
                /* the tight power has the same real part (the same kernel) */
                ADF_CHECK_MSG(arb_equal(res, res_t), "line %zu", i + 1);
                read_uc(want, field(rec, "u"));
                read_uc(want_t, field(rec, "ut"));
                if (is_idele)
                {
                    read_fmpq(r, field(rec, "r"));
                    ADF_CHECK_MSG(adf_idele_is_canonical(z) && fmpq_equal(z->r, r)
                                  && adf_ucoset_identical(&z->u, want)
                                  && fmpq_equal(zt->r, r) && adf_ucoset_identical(&zt->u, want_t),
                                  "line %zu", i + 1);
                    /* aliasing: the output is the input */
                    adf_idele_set(z, x);
                    ADF_CHECK(adf_idele_pow(z, z, k, prec) == ADF_OK && arb_equal(z->inf, res_t)
                              && adf_ucoset_identical(&z->u, want));
                    adf_idele_set(z, x);
                    ADF_CHECK(adf_idele_pow_tight(z, z, k, prec) == ADF_OK && adf_idele_identical(z, zt));
                }
                else
                {
                    ADF_CHECK_MSG(adf_idclass_is_canonical(cz) && adf_ucoset_identical(&cz->u, want)
                                  && adf_ucoset_identical(&czt->u, want_t), "line %zu", i + 1);
                    adf_idclass_set(cz, cx);
                    ADF_CHECK(adf_idclass_pow_tight(cz, cz, k, prec) == ADF_OK && adf_idclass_identical(cz, czt));
                    adf_idclass_set(cz, cx);
                    ADF_CHECK(adf_idclass_pow(cz, cz, k, prec) == ADF_OK && arb_equal(cz->t, res_t)
                              && adf_ucoset_identical(&cz->u, want));
                }
            }
        }
    }
    printf("vectors: %zu lines: %zu ucoset, %zu idele, %zu class; %zu NOT_DETERMINED, %zu LIMIT, %zu exact, "
           "%zu with exact ends\n", jsonl_count(f), n_uc, n_idele, n_class, n_nd, n_limit, n_exact, n_ends);
    ADF_CHECK(n_uc + n_idele + n_class == jsonl_count(f) && n_nd > 100 && n_limit >= 10 && n_ends > 400);
    jsonl_close(f);
    arf_clear(lo);
    arf_clear(hi);
    fmpq_clear(r);
    fmpq_clear(L);
    fmpq_clear(H);
    adf_ucoset_clear(u);
    adf_ucoset_clear(want);
    adf_ucoset_clear(want_t);
    adf_ucoset_clear(y);
    adf_idele_clear(x);
    adf_idele_clear(z);
    adf_idele_clear(zt);
    adf_idele_clear(keep);
    adf_idclass_clear(cx);
    adf_idclass_clear(cz);
    adf_idclass_clear(czt);
    adf_idclass_clear(ckeep);
}

/* ---- ideles and classes: hand cases ---- */

ADF_TEST(idele_pow_hand_cases_statuses_aliasing)
{
    adf_idele_t x, z, keep, a;
    arb_t inf;
    fmpq_t want;
    fmpz_t e;

    adf_idele_init(x);
    adf_idele_init(z);
    adf_idele_init(keep);
    adf_idele_init(a);
    arb_init(inf);
    fmpq_init(want);
    fmpz_init(e);
    /* (-3 +- 1 ; 3/2 [5 mod 12])^2 = (positive ball around 9 ; 9/4 [1 mod 12]) */
    arb_set_si_si(inf, -3, 0, 1, 0);
    idele_set_parts_si(x, inf, 3, 2, 5, 12);
    ADF_CHECK(adf_idele_pow(z, x, 2, 64) == ADF_OK && encloses_power(z->inf, x->inf, 2) && arb_is_positive(z->inf));
    fmpq_set_si(want, 9, 4);
    ADF_CHECK(fmpq_equal(z->r, want) && uc_is(&z->u, 1, 12));
    /* odd and negative exponents keep the sign: (-3 +- 1)^-3 is negative */
    ADF_CHECK(adf_idele_pow(z, x, -3, 64) == ADF_OK && encloses_power(z->inf, x->inf, -3)
              && arb_is_negative(z->inf));
    fmpq_set_si(want, 8, 27);
    ADF_CHECK(fmpq_equal(z->r, want) && uc_is(&z->u, 5, 12));    /* 5^-3 = 5 mod 12 */
    ADF_CHECK(adf_idele_pow_tight(z, x, 2, 64) == ADF_OK && uc_is(&z->u, 1, 24));
    /* k = 0: the exact idele 1 */
    adf_idele_init(keep);
    ADF_CHECK(adf_idele_pow(z, x, 0, 64) == ADF_OK && adf_idele_identical(z, keep));
    ADF_CHECK(adf_idele_pow_tight(z, x, 0, 64) == ADF_OK && adf_idele_identical(z, keep));
    /* exact results (K.4): (-3)^3 = -27 exact at prec 5 (27 has 5 bits), a ball at prec 4 */
    arb_set_si(inf, -3);
    idele_set_parts_si(x, inf, 1, 1, 1, 0);
    ADF_CHECK(adf_idele_pow(z, x, 3, 5) == ADF_OK && arb_is_exact(z->inf) && arb_equal_si(z->inf, -27));
    ADF_CHECK(adf_idele_pow(z, x, 3, 4) == ADF_OK && !arb_is_exact(z->inf) && arb_contains_si(z->inf, -27)
              && arb_is_negative(z->inf));
    ADF_CHECK(uc_is(&z->u, 1, 0));
    /* 2^(2^62) and 2^(-2^63): exact, huge exponents (arf exponents are fmpz) */
    arb_set_si(inf, -2);
    idele_set_parts_si(x, inf, 1, 1, -1, 0);
    ADF_CHECK(adf_idele_pow(z, x, WORD(1) << 62, 64) == ADF_OK && arb_is_exact(z->inf) && arb_is_positive(z->inf));
    fmpz_set_si(e, WORD(1) << 62);
    fmpz_add_ui(e, e, 1);
    ADF_CHECK(arf_bits(arb_midref(z->inf)) == 1 && fmpz_equal(ARF_EXPREF(arb_midref(z->inf)), e)
              && uc_is(&z->u, 1, 0));
    ADF_CHECK(adf_idele_pow(z, x, WORD_MIN, 64) == ADF_OK && arb_is_exact(z->inf) && arb_is_positive(z->inf));
    fmpz_set_si(e, WORD_MIN);
    fmpz_add_ui(e, e, 1);
    ADF_CHECK(arf_bits(arb_midref(z->inf)) == 1 && fmpz_equal(ARF_EXPREF(arb_midref(z->inf)), e)
              && uc_is(&z->u, 1, 0));
    ADF_CHECK(adf_idele_pow(z, x, WORD_MAX, 64) == ADF_OK && arb_is_exact(z->inf) && arb_is_negative(z->inf)
              && uc_is(&z->u, -1, 0));
    /* NOT_DETERMINED: x = 1 +- (1 - 2^-30) squared: ends 2^-60 and about 4; at p = 16 z untouched, OK at 128 */
    arb_set_si_si(inf, 1, 0, (UWORD(1) << 30) - 1, -30);
    idele_set_parts_si(x, inf, 1, 1, 1, 1);
    arb_set_si(z->inf, 9);
    adf_idele_set(keep, z);
    ADF_CHECK(adf_idele_pow(z, x, 2, 16) == ADF_NOT_DETERMINED && adf_idele_identical(z, keep));
    ADF_CHECK(adf_idele_pow_tight(z, x, -2, 16) == ADF_NOT_DETERMINED && adf_idele_identical(z, keep));
    adf_idele_set(a, x);
    ADF_CHECK(adf_idele_pow(a, a, 2, 16) == ADF_NOT_DETERMINED && adf_idele_identical(a, x));
    ADF_CHECK(adf_idele_pow(z, x, 2, 128) == ADF_OK && encloses_power(z->inf, x->inf, 2));
    /* LIMIT: prec, and the bits of r^k (Statement L.4), before any other status; z untouched, also aliased */
    arb_set_si_si(inf, -3, 0, 1, 0);
    idele_set_parts_si(x, inf, 3, 2, 5, 12);
    adf_idele_set(z, keep);
    ADF_CHECK(adf_idele_pow(z, x, 2, ADF_IDELE_PREC_MAX + 1) == ADF_LIMIT && adf_idele_identical(z, keep));
    ADF_CHECK(adf_idele_pow(z, x, 0, WORD_MAX) == ADF_LIMIT && adf_idele_identical(z, keep));
    ADF_CHECK(adf_idele_pow_tight(z, x, 1, WORD_MAX) == ADF_LIMIT && adf_idele_identical(z, keep));
    /* |k| (bits(3) + bits(2)) = 4 |k| against 2^26: k = 2^24 + 1 is LIMIT, even where the real part would be
       NOT_DETERMINED */
    ADF_CHECK(adf_idele_pow(z, x, (WORD(1) << 24) + 1, 64) == ADF_LIMIT && adf_idele_identical(z, keep));
    ADF_CHECK(adf_idele_pow(z, x, -(WORD(1) << 24) - 1, 64) == ADF_LIMIT && adf_idele_identical(z, keep));
    ADF_CHECK(adf_idele_pow_tight(z, x, WORD_MIN, 64) == ADF_LIMIT && adf_idele_identical(z, keep));
    adf_idele_set(a, x);
    ADF_CHECK(adf_idele_pow(a, a, WORD_MAX, 64) == ADF_LIMIT && adf_idele_identical(a, x));
    /* at the edge, 4 |k| = 2^26: OK, the content (3/2)^(2^24) is formed */
    arb_set_si(inf, -1);
    idele_set_parts_si(x, inf, 3, 2, 5, 12);
    ADF_CHECK(adf_idele_pow(z, x, WORD(1) << 24, 64) == ADF_OK && arb_is_one(z->inf));
    fmpz_set_ui(e, 3);
    fmpz_pow_ui(e, e, UWORD(1) << 24);
    ADF_CHECK(fmpz_equal(fmpq_numref(z->r), e) && fmpz_bits(fmpq_denref(z->r)) == (UWORD(1) << 24) + 1
              && fmpz_val2(fmpq_denref(z->r)) == WORD(1) << 24 && uc_is(&z->u, 1, 12));
    /* r = 1: no limit on k */
    idele_set_parts_si(x, inf, 1, 1, 5, 12);
    ADF_CHECK(adf_idele_pow(z, x, WORD_MIN, 64) == ADF_OK && arb_is_one(z->inf) && fmpq_is_one(z->r)
              && uc_is(&z->u, 1, 12));
    /* prec below 2 is 2 */
    /* -3 exact at p = 2: ends 3 -> 24, 48 (one binade apart): OK; -3 +- 1/4: ends 8, 64: NOT_DETERMINED */
    arb_set_si(inf, -3);
    idele_set_parts_si(x, inf, 3, 2, 5, 12);
    ADF_CHECK(adf_idele_pow(a, x, 3, 2) == ADF_OK && arb_contains_si(a->inf, -27) && !arb_is_exact(a->inf));
    ADF_CHECK(adf_idele_pow(z, x, 3, 0) == ADF_OK && adf_idele_identical(z, a));
    ADF_CHECK(adf_idele_pow(z, x, 3, WORD_MIN) == ADF_OK && adf_idele_identical(z, a));
    arb_set_si_si(inf, -3, 0, 1, -2);
    idele_set_parts_si(x, inf, 3, 2, 5, 12);
    adf_idele_set(z, keep);
    ADF_CHECK(adf_idele_pow(z, x, 3, 2) == ADF_NOT_DETERMINED && adf_idele_pow(z, x, 3, 1) == ADF_NOT_DETERMINED
              && adf_idele_pow(z, x, 3, -5) == ADF_NOT_DETERMINED && adf_idele_identical(z, keep));
    ADF_CHECK(adf_idele_pow(z, x, 3, 3) == ADF_OK && encloses_power(z->inf, x->inf, 3));
    /* aliasing on OK: the same result */
    adf_idele_set(a, x);
    ADF_CHECK(adf_idele_pow(a, a, -3, 64) == ADF_OK && adf_idele_pow(z, x, -3, 64) == ADF_OK
              && adf_idele_identical(a, z));
    fmpz_clear(e);
    fmpq_clear(want);
    arb_clear(inf);
    adf_idele_clear(x);
    adf_idele_clear(z);
    adf_idele_clear(keep);
    adf_idele_clear(a);
}

ADF_TEST(idele_pow_against_repeated_multiplication)
{
    /* The power set {x^k} lies inside the product set of k independent copies, which adf_idele_mul encloses:
       the default unit equals the unit of the product, the tight unit lies inside it, the contents are equal,
       and the real balls meet at the true power of the midpoint. */
    static const slong ks[] = { -3, -2, 2, 3, 4, 5 };
    static const slong cs[][2] = { { 5, 12 }, { 2, 5 }, { 1, 1 }, { 7, 30 }, { -1, 0 }, { 3, 8 } };
    adf_idele_t x, p, pt, m;
    arb_t inf;
    fmpq_t mid;
    size_t i, j;

    adf_idele_init(x);
    adf_idele_init(p);
    adf_idele_init(pt);
    adf_idele_init(m);
    arb_init(inf);
    fmpq_init(mid);
    for (i = 0; i < sizeof(cs) / sizeof(cs[0]); i++)
        for (j = 0; j < sizeof(ks) / sizeof(ks[0]); j++)
        {
            slong k = ks[j], e;
            arb_set_si_si(inf, -(slong) (7 + 2 * i), -2, 3, -4);
            idele_set_parts_si(x, inf, 5, 3 + (slong) i, cs[i][0], cs[i][1]);
            ADF_CHECK(adf_idele_pow(p, x, k, 64) == ADF_OK && adf_idele_pow_tight(pt, x, k, 64) == ADF_OK);
            adf_idele_set(m, x);
            for (e = 1; e < (k < 0 ? -k : k); e++)
                ADF_CHECK(adf_idele_mul(m, m, x, 64) == ADF_OK);
            if (k < 0)
                ADF_CHECK(adf_idele_inv(m, m, 64) == ADF_OK);
            ADF_CHECK(adf_ucoset_equal_set(&p->u, &m->u) && adf_ucoset_contains(&pt->u, &m->u));
            ADF_CHECK(fmpq_equal(p->r, m->r) && fmpq_equal(pt->r, m->r));
            arf_get_fmpq(mid, arb_midref(x->inf));
            fmpq_pow_si(mid, mid, k);
            ADF_CHECK(arb_contains_fmpq(p->inf, mid) && arb_contains_fmpq(m->inf, mid)
                      && arb_overlaps(p->inf, m->inf));
            ADF_CHECK(encloses_power(p->inf, x->inf, k) && arb_equal(p->inf, pt->inf));
        }
    fmpq_clear(mid);
    arb_clear(inf);
    adf_idele_clear(x);
    adf_idele_clear(p);
    adf_idele_clear(pt);
    adf_idele_clear(m);
}

ADF_TEST(idclass_pow_hand_cases)
{
    adf_idclass_t x, z, keep, one;
    arb_t t;

    adf_idclass_init(x);
    adf_idclass_init(z);
    adf_idclass_init(keep);
    adf_idclass_init(one);
    arb_init(t);
    /* <5/2 +- 1/4 ; [5 mod 36]>^-2 */
    arb_set_si_si(t, 5, -1, 1, -2);
    idclass_set_parts_si(x, t, 5, 36);
    ADF_CHECK(adf_idclass_pow(z, x, -2, 64) == ADF_OK && encloses_power(z->t, x->t, -2) && arb_is_positive(z->t));
    ADF_CHECK(uc_is(&z->u, 13, 36));                    /* 5^-2 = 13 mod 36: 25 * 13 = 325 = 1 mod 36 */
    /* tight: A = 36 * 2 (v_2(k) = 1), B = 1 (3, the only odd p with p - 1 | 2, divides 36); 5^-2 = 49 mod 72 */
    ADF_CHECK(adf_idclass_pow_tight(z, x, -2, 64) == ADF_OK && uc_is(&z->u, 49, 72));
    /* k = 0: <1 ; [1]> exactly */
    ADF_CHECK(adf_idclass_pow(z, x, 0, 64) == ADF_OK && adf_idclass_identical(z, one));
    ADF_CHECK(adf_idclass_pow_tight(z, x, 0, 64) == ADF_OK && adf_idclass_identical(z, one));
    /* exact t = 3: 3^4 = 81 exact at prec 7, not at prec 6 */
    arb_set_si(t, 3);
    idclass_set_parts_si(x, t, 1, 0);
    ADF_CHECK(adf_idclass_pow(z, x, 4, 7) == ADF_OK && arb_is_exact(z->t) && arb_equal_si(z->t, 81));
    ADF_CHECK(adf_idclass_pow(z, x, 4, 6) == ADF_OK && !arb_is_exact(z->t) && arb_contains_si(z->t, 81));
    /* NOT_DETERMINED and LIMIT: untouched, also aliased */
    arb_set_si_si(t, 1, 0, (UWORD(1) << 30) - 1, -30);
    idclass_set_parts_si(x, t, 1, 1);
    arb_set_si(z->t, 5);
    adf_idclass_set(keep, z);
    ADF_CHECK(adf_idclass_pow(z, x, 3, 30) == ADF_NOT_DETERMINED && adf_idclass_identical(z, keep));
    ADF_CHECK(adf_idclass_pow_tight(z, x, 3, 30) == ADF_NOT_DETERMINED && adf_idclass_identical(z, keep));
    ADF_CHECK(adf_idclass_pow(z, x, 3, ADF_IDELE_PREC_MAX + 1) == ADF_LIMIT && adf_idclass_identical(z, keep));
    ADF_CHECK(adf_idclass_pow_tight(z, x, 0, WORD_MAX) == ADF_LIMIT && adf_idclass_identical(z, keep));
    adf_idclass_set(z, x);
    ADF_CHECK(adf_idclass_pow(z, z, 3, 30) == ADF_NOT_DETERMINED && adf_idclass_identical(z, x));
    ADF_CHECK(adf_idclass_pow(z, z, 3, 200) == ADF_OK && encloses_power(z->t, x->t, 3));
    /* the class of the power of an idele is the power of its class (P15.1, a homomorphism) */
    {
        adf_idele_t xi, pi;
        adf_idclass_t c1, c2;
        adf_idele_init(xi);
        adf_idele_init(pi);
        adf_idclass_init(c1);
        adf_idclass_init(c2);
        arb_set_si_si(t, -13, -2, 1, -4);
        idele_set_parts_si(xi, t, 7, 5, 11, 30);
        ADF_CHECK(adf_idele_pow_tight(pi, xi, 3, 64) == ADF_OK && adf_idclass_set_idele(c1, pi, 64) == ADF_OK);
        ADF_CHECK(adf_idclass_set_idele(c2, xi, 64) == ADF_OK && adf_idclass_pow_tight(c2, c2, 3, 64) == ADF_OK);
        ADF_CHECK(adf_ucoset_equal_set(&c1->u, &c2->u) && arb_overlaps(c1->t, c2->t));
        adf_idele_clear(xi);
        adf_idele_clear(pi);
        adf_idclass_clear(c1);
        adf_idclass_clear(c2);
    }
    arb_clear(t);
    adf_idclass_clear(x);
    adf_idclass_clear(z);
    adf_idclass_clear(keep);
    adf_idclass_clear(one);
}
