/* tests/test_ucoset.c: adf_ucoset (milestone 2, slice 1, lane i-slice1; include/adelefeld/ucoset.h;
   docs/api-2.md 1.3 Statements A, B, C; docs/proofs/ideles.md P9, P10, P11).

   The oracles, in order of independence:
   1. Enumeration in (Z/M)^x, written in this file with machine integers: a coset c U(N) with N | M is
      the full preimage of its image {u in [0, M) : gcd(u, M) = 1, u = c mod N} (proto/ideles_checks.py,
      header of the file), and the exact unit e has the image {e mod M}. For every pair of the 66
      values with N <= 14 (two exact units among them), at the levels M = 5 L, 12 L, 28 L (L the lcm of
      the moduli; each level has an odd prime that gives every coset at least two elements, so an exact
      unit and a coset are told apart): the product set against the image of adf_ucoset_mul, the
      inverse set against the image of adf_ucoset_inv, the point 1 in x * x^-1, and the three
      predicates against inclusion, intersection and equality of the images.
   2. tests/ref/vectors/i-slice1/ucoset.jsonl, written by lanes/i-slice1/gen_vectors.py from the
      ref_uc_* functions of proto/ideles_checks.py (checked there by enumeration); it holds moduli of
      1000 to 3000 bits. Every line is run.
   3. Hand-computed examples of SPEC 5, conventions 5.6 and tests/golden/ucoset.tsv. */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <flint/fmpz.h>

#include <adelefeld/ucoset.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* ---- helpers ---- */

static void
uc_set_si2(adf_ucoset_t x, slong c, slong N)
{
    fmpz_t fc, fN;
    fmpz_init_set_si(fc, c);
    fmpz_init_set_si(fN, N);
    if (adf_ucoset_set_fmpz2(x, fc, fN) != ADF_OK)
    {
        printf("uc_set_si2: (%ld, %ld) refused\n", (long) c, (long) N);
        abort();
    }
    fmpz_clear(fc);
    fmpz_clear(fN);
}

/* 1 if the stored pair of x is (c, N). */
static int
uc_is(const adf_ucoset_t x, slong c, slong N)
{
    return fmpz_equal_si(x->c, c) && fmpz_equal_si(x->N, N);
}

/* Writes the raw fields without any check (for the predicates). */
static void
uc_raw(adf_ucoset_t x, slong c, slong N)
{
    fmpz_set_si(x->c, c);
    fmpz_set_si(x->N, N);
}

static ulong
gcd_ul(ulong a, ulong b)
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
lcm_ul(ulong a, ulong b)
{
    return a / gcd_ul(a, b) * b;
}

/* The image of the set of x in (Z/M)^x as a 0/1 array of length M. N | M is required. */
static void
level_set(unsigned char * s, const adf_ucoset_t x, ulong M)
{
    ulong u;
    slong c = fmpz_get_si(x->c);
    ulong N = fmpz_get_ui(x->N);

    memset(s, 0, M);
    if (N == 0)
    {
        s[(ulong) ((c % (slong) M + (slong) M) % (slong) M)] = 1;
        return;
    }
    if (M % N != 0)
        abort();
    for (u = 0; u < M; u++)
        if (gcd_ul(u, M) == 1 && (ulong) (((slong) u - c) % (slong) N + (slong) N) % N == 0)
            s[u] = 1;
}

/* All canonical cosets with N <= nmax, and the two exact units. */
static slong
all_cosets(slong * cs, slong * Ns, slong nmax)
{
    slong n = 0, N, c;

    cs[n] = 1;
    Ns[n++] = 0;
    cs[n] = -1;
    Ns[n++] = 0;
    for (N = 1; N <= nmax; N++)
        for (c = 1; c <= N; c++)
            if (gcd_ul((ulong) c, (ulong) N) == 1)
            {
                cs[n] = c;
                Ns[n++] = N;
            }
    return n;
}

static void
read_coset(adf_ucoset_t x, const jsonl_value * v)
{
    const char * t;
    jsonl_error_t err;

    if (!jsonl_int_text_or_string(jsonl_at(v, 0, &err), &t, &err) || fmpz_set_str(x->c, t, 10) != 0)
        abort();
    if (!jsonl_int_text_or_string(jsonl_at(v, 1, &err), &t, &err) || fmpz_set_str(x->N, t, 10) != 0)
        abort();
}

static void
read_int(fmpz_t z, const jsonl_value * rec, const char * key)
{
    const jsonl_value * v;
    const char * t;
    jsonl_error_t err;

    if (!jsonl_field(rec, key, &v, &err) || !jsonl_int_text_or_string(v, &t, &err)
        || fmpz_set_str(z, t, 10) != 0)
    {
        printf("read_int: %s: %s\n", key, jsonl_error_message(&err));
        abort();
    }
}

static int
read_bool(const jsonl_value * rec, const char * key)
{
    const jsonl_value * v;
    jsonl_error_t err;
    int b;

    if (!jsonl_field(rec, key, &v, &err) || !jsonl_bool(v, &b, &err))
        abort();
    return b;
}

static const char *
read_str(const jsonl_value * rec, const char * key)
{
    const jsonl_value * v;
    jsonl_error_t err;
    size_t len;
    const char * s;

    if (!jsonl_field(rec, key, &v, &err) || (s = jsonl_string(v, &len, &err)) == NULL)
        abort();
    return s;
}

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

/* ---- layout, life cycle, predicates of the storage ---- */

ADF_TEST(layout_is_16_bytes_c_then_N)
{
    ADF_CHECK(adf_sizeof_ucoset() == 16);
    ADF_CHECK(adf_alignof_ucoset() == 8);
    ADF_CHECK(offsetof(adf_ucoset_struct, c) == 0);
    ADF_CHECK(offsetof(adf_ucoset_struct, N) == 8);
}

ADF_TEST(init_is_the_exact_unit_one)
{
    adf_ucoset_t x;
    adf_ucoset_init(x);
    ADF_CHECK(uc_is(x, 1, 0));
    ADF_CHECK(adf_ucoset_is_canonical(x));
    ADF_CHECK(adf_ucoset_is_normal(x));
    ADF_CHECK(adf_ucoset_is_exact(x));
    adf_ucoset_minus_one(x);
    ADF_CHECK(uc_is(x, -1, 0) && adf_ucoset_is_exact(x) && adf_ucoset_is_canonical(x));
    adf_ucoset_one(x);
    ADF_CHECK(uc_is(x, 1, 0));
    adf_ucoset_clear(x);
}

ADF_TEST(is_canonical_is_the_predicate_of_conventions_5_6)
{
    static const slong yes[][2] = { { 1, 1 }, { 1, 2 }, { 5, 6 }, { 1, 0 }, { -1, 0 }, { 7, 12 }, { 3, 4 } };
    static const slong no[][2] = { { 0, 1 }, { 2, 1 }, { 0, 0 }, { 2, 0 }, { -2, 0 }, { 0, 6 }, { 6, 6 },
                                   { 7, 6 }, { -1, 6 }, { 4, 6 }, { 1, -1 }, { -1, -1 }, { 3, 2 } };
    adf_ucoset_t x;
    size_t i;

    adf_ucoset_init(x);
    for (i = 0; i < sizeof(yes) / sizeof(yes[0]); i++)
    {
        uc_raw(x, yes[i][0], yes[i][1]);
        ADF_CHECK_MSG(adf_ucoset_is_canonical(x), "(%ld, %ld)", (long) yes[i][0], (long) yes[i][1]);
    }
    for (i = 0; i < sizeof(no) / sizeof(no[0]); i++)
    {
        uc_raw(x, no[i][0], no[i][1]);
        ADF_CHECK_MSG(!adf_ucoset_is_canonical(x), "(%ld, %ld)", (long) no[i][0], (long) no[i][1]);
        ADF_CHECK(!adf_ucoset_is_normal(x));
    }
    /* normal: canonical and N != 2 mod 4 */
    uc_raw(x, 5, 6);
    ADF_CHECK(!adf_ucoset_is_normal(x));
    uc_raw(x, 1, 2);
    ADF_CHECK(!adf_ucoset_is_normal(x));
    uc_raw(x, 2, 3);
    ADF_CHECK(adf_ucoset_is_normal(x));
    uc_raw(x, 3, 4);
    ADF_CHECK(adf_ucoset_is_normal(x));
    uc_raw(x, 1, 1);
    ADF_CHECK(adf_ucoset_is_normal(x));
    uc_raw(x, -1, 0);
    ADF_CHECK(adf_ucoset_is_normal(x));
    /* huge fields do not disturb the predicate */
    fmpz_set_ui(x->N, 1);
    fmpz_mul_2exp(x->N, x->N, 5000);
    fmpz_add_ui(x->N, x->N, 1);
    fmpz_set_si(x->c, -3);
    ADF_CHECK(!adf_ucoset_is_canonical(x));
    fmpz_add(x->c, x->c, x->N);
    ADF_CHECK(adf_ucoset_is_canonical(x));
    adf_ucoset_clear(x);
}

ADF_TEST(set_swap_identical)
{
    adf_ucoset_t x, y;
    adf_ucoset_init(x);
    adf_ucoset_init(y);
    uc_set_si2(x, 5, 6);
    uc_set_si2(y, 2, 3);
    ADF_CHECK(!adf_ucoset_identical(x, y));
    ADF_CHECK(adf_ucoset_equal_set(x, y));
    adf_ucoset_swap(x, y);
    ADF_CHECK(uc_is(x, 2, 3) && uc_is(y, 5, 6));
    adf_ucoset_set(x, y);
    ADF_CHECK(uc_is(x, 5, 6) && adf_ucoset_identical(x, y));
    adf_ucoset_set(x, x);
    ADF_CHECK(uc_is(x, 5, 6));
    adf_ucoset_clear(x);
    adf_ucoset_clear(y);
}

/* ---- the constructor ---- */

ADF_TEST(set_fmpz2_reduces_c_and_keeps_N)
{
    /* conventions 5.6 (residue range 1..N, modulus as supplied) and tests/golden/ucoset.tsv */
    static const slong cases[][4] = {
        { 5, 6, 5, 6 }, { -1, 6, 5, 6 }, { 0, 1, 1, 1 }, { 1, 1, 1, 1 }, { 3, 2, 1, 2 }, { 1, 2, 1, 2 },
        { 13, 12, 1, 12 }, { -5, 12, 7, 12 }, { 7, 10, 7, 10 }, { 1, 0, 1, 0 }, { -1, 0, -1, 0 },
        { -7, 1, 1, 1 }, { 41, 36, 5, 36 }, { -35, 36, 1, 36 } };
    adf_ucoset_t x;
    fmpz_t c, N, gc, gN;
    size_t i;

    adf_ucoset_init(x);
    fmpz_init(c);
    fmpz_init(N);
    fmpz_init(gc);
    fmpz_init(gN);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        fmpz_set_si(c, cases[i][0]);
        fmpz_set_si(N, cases[i][1]);
        ADF_CHECK(adf_ucoset_set_fmpz2(x, c, N) == ADF_OK);
        ADF_CHECK_MSG(uc_is(x, cases[i][2], cases[i][3]), "(%ld, %ld)", (long) cases[i][0], (long) cases[i][1]);
        ADF_CHECK(adf_ucoset_is_canonical(x));
        adf_ucoset_get_fmpz2(gc, gN, x);
        ADF_CHECK(fmpz_equal_si(gc, cases[i][2]) && fmpz_equal_si(gN, cases[i][3]));
    }
    /* aliasing: the arguments are the members of x itself */
    uc_raw(x, -1, 6);
    ADF_CHECK(adf_ucoset_set_fmpz2(x, x->c, x->N) == ADF_OK && uc_is(x, 5, 6));
    uc_raw(x, 7, 7);
    ADF_CHECK(adf_ucoset_set_fmpz2(x, x->c, x->c) == ADF_DOMAIN);
    uc_raw(x, 1, 1);
    ADF_CHECK(adf_ucoset_set_fmpz2(x, x->c, x->c) == ADF_OK && uc_is(x, 1, 1));
    /* accessors: members of x as outputs */
    uc_set_si2(x, 5, 6);
    adf_ucoset_get_fmpz2(x->c, x->N, x);
    ADF_CHECK(uc_is(x, 5, 6));
    adf_ucoset_get_fmpz2(x->N, x->c, x);      /* the members crossed: c goes to x->N, N to x->c */
    ADF_CHECK(fmpz_equal_si(x->N, 5) && fmpz_equal_si(x->c, 6));
    uc_set_si2(x, 5, 6);
    adf_ucoset_get_fmpz2(x->N, gN, x);
    ADF_CHECK(fmpz_equal_si(x->N, 5) && fmpz_equal_si(gN, 6));
    fmpz_clear(c);
    fmpz_clear(N);
    fmpz_clear(gc);
    fmpz_clear(gN);
    adf_ucoset_clear(x);
}

ADF_TEST(set_fmpz2_refuses_and_leaves_x_untouched)
{
    static const slong bad[][2] = { { 2, 4 }, { 0, 6 }, { 3, 6 }, { 2, 6 }, { 5, 0 }, { 0, 0 }, { 2, 0 },
                                    { -2, 0 }, { 1, -1 }, { 5, -6 }, { 0, 2 }, { 0, -1 }, { 6, 6 } };
    adf_ucoset_t x;
    fmpz_t c, N;
    size_t i;

    adf_ucoset_init(x);
    fmpz_init(c);
    fmpz_init(N);
    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
    {
        uc_set_si2(x, 7, 12);
        fmpz_set_si(c, bad[i][0]);
        fmpz_set_si(N, bad[i][1]);
        ADF_CHECK_MSG(adf_ucoset_set_fmpz2(x, c, N) == ADF_DOMAIN, "(%ld, %ld)", (long) bad[i][0],
                      (long) bad[i][1]);
        ADF_CHECK(uc_is(x, 7, 12));
    }
    fmpz_clear(c);
    fmpz_clear(N);
    adf_ucoset_clear(x);
}

/* ---- normal form ---- */

ADF_TEST(normalise_removes_the_factor_2_and_reduces)
{
    static const slong cases[][4] = { { 5, 6, 2, 3 }, { 1, 2, 1, 1 }, { 5, 18, 5, 9 }, { 11, 30, 11, 15 },
                                      { 7, 12, 7, 12 }, { 7, 10, 2, 5 }, { 1, 0, 1, 0 }, { -1, 0, -1, 0 },
                                      { 3, 4, 3, 4 }, { 1, 1, 1, 1 }, { 13, 50, 13, 25 } };
    adf_ucoset_t x, y;
    size_t i;

    adf_ucoset_init(x);
    adf_ucoset_init(y);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        uc_set_si2(x, cases[i][0], cases[i][1]);
        adf_ucoset_normalise(y, x);
        ADF_CHECK_MSG(uc_is(y, cases[i][2], cases[i][3]), "(%ld, %ld)", (long) cases[i][0], (long) cases[i][1]);
        ADF_CHECK(adf_ucoset_is_normal(y) && adf_ucoset_equal_set(x, y));
        adf_ucoset_normalise(x, x);
        ADF_CHECK(adf_ucoset_identical(x, y));
    }
    adf_ucoset_clear(x);
    adf_ucoset_clear(y);
}

/* ---- examples ---- */

ADF_TEST(examples_of_spec_5_and_the_exact_factor)
{
    adf_ucoset_t x, y, z;
    adf_ucoset_init(x);
    adf_ucoset_init(y);
    adf_ucoset_init(z);

    /* [5 mod 12] * [7 mod 18] = 35 U(6) = [5 mod 6] = [2 mod 3] in normal form */
    uc_set_si2(x, 5, 12);
    uc_set_si2(y, 7, 18);
    adf_ucoset_mul(z, x, y);
    ADF_CHECK(uc_is(z, 2, 3));
    /* gcd(0, N) = N: [-1] * [5 mod 12] = [7 mod 12] */
    adf_ucoset_minus_one(x);
    uc_set_si2(y, 5, 12);
    adf_ucoset_mul(z, x, y);
    ADF_CHECK(uc_is(z, 7, 12));
    adf_ucoset_mul(z, y, x);
    ADF_CHECK(uc_is(z, 7, 12));
    /* [-1] * [-1] = [1], exact; [1] * [1 mod 1] = [1 mod 1], not exact */
    adf_ucoset_mul(z, x, x);
    ADF_CHECK(uc_is(z, 1, 0));
    adf_ucoset_one(x);
    uc_set_si2(y, 1, 1);
    adf_ucoset_mul(z, x, y);
    ADF_CHECK(uc_is(z, 1, 1));
    /* the product at one modulus keeps it: [5 mod 12] * [5 mod 12] = [1 mod 12] */
    uc_set_si2(x, 5, 12);
    adf_ucoset_mul(z, x, x);
    ADF_CHECK(uc_is(z, 1, 12));
    /* a result in normal form: [5 mod 6] * [1 mod 6] = [2 mod 3], not (5, 6) */
    uc_set_si2(x, 5, 6);
    uc_set_si2(y, 1, 6);
    adf_ucoset_mul(z, x, y);
    ADF_CHECK(uc_is(z, 2, 3));
    /* inverses: [2 mod 3]^-1 = [2 mod 3]; [5 mod 12]^-1 = [5 mod 12]; [3 mod 7]^-1 = [5 mod 7];
       [1 mod 2]^-1 = [1 mod 1]; [-1]^-1 = [-1] */
    uc_set_si2(x, 2, 3);
    adf_ucoset_inv(z, x);
    ADF_CHECK(uc_is(z, 2, 3));
    uc_set_si2(x, 5, 12);
    adf_ucoset_inv(z, x);
    ADF_CHECK(uc_is(z, 5, 12));
    uc_set_si2(x, 3, 7);
    adf_ucoset_inv(z, x);
    ADF_CHECK(uc_is(z, 5, 7));
    uc_set_si2(x, 1, 2);
    adf_ucoset_inv(z, x);
    ADF_CHECK(uc_is(z, 1, 1));
    uc_set_si2(x, 1, 1);
    adf_ucoset_inv(z, x);
    ADF_CHECK(uc_is(z, 1, 1));
    adf_ucoset_minus_one(x);
    adf_ucoset_inv(z, x);
    ADF_CHECK(uc_is(z, -1, 0));
    /* x * x^-1 = U(N'), which is not the exact unit 1 */
    uc_set_si2(x, 5, 12);
    adf_ucoset_inv(y, x);
    adf_ucoset_mul(z, x, y);
    ADF_CHECK(uc_is(z, 1, 12) && !adf_ucoset_is_exact(z));
    /* predicates: [1] is inside [1 mod 1] and not conversely; [1] and [-1] are disjoint; [-1] meets
       [3 mod 4]; [1] does not meet [3 mod 4]; [5 mod 6] = [2 mod 3] */
    adf_ucoset_one(x);
    uc_set_si2(y, 1, 1);
    ADF_CHECK(adf_ucoset_contains(x, y) && !adf_ucoset_contains(y, x) && !adf_ucoset_equal_set(x, y));
    adf_ucoset_minus_one(y);
    ADF_CHECK(!adf_ucoset_overlaps(x, y) && !adf_ucoset_contains(x, y));
    uc_set_si2(z, 3, 4);
    ADF_CHECK(adf_ucoset_overlaps(y, z) && adf_ucoset_contains(y, z) && !adf_ucoset_overlaps(x, z));
    /* [-1] is inside [1 mod 2] = [1 mod 1] (the factor 2) and inside [5 mod 6] = [2 mod 3] */
    uc_set_si2(z, 1, 2);
    ADF_CHECK(adf_ucoset_contains(y, z));
    uc_set_si2(z, 5, 6);
    ADF_CHECK(adf_ucoset_contains(y, z));
    /* [3 mod 4] and [1 mod 4] are different and disjoint; [1 mod 8] is inside [1 mod 4] */
    uc_set_si2(x, 3, 4);
    uc_set_si2(y, 1, 4);
    ADF_CHECK(!adf_ucoset_equal_set(x, y) && !adf_ucoset_overlaps(x, y));
    uc_set_si2(x, 1, 8);
    ADF_CHECK(adf_ucoset_contains(x, y) && !adf_ucoset_contains(y, x) && adf_ucoset_overlaps(x, y));
    /* [2 mod 3] is inside [5 mod 6] and conversely (the same set) */
    uc_set_si2(x, 2, 3);
    uc_set_si2(y, 5, 6);
    ADF_CHECK(adf_ucoset_contains(x, y) && adf_ucoset_contains(y, x));

    adf_ucoset_clear(x);
    adf_ucoset_clear(y);
    adf_ucoset_clear(z);
}

/* ---- aliasing (conventions 4.1: (x, x, y), (y, x, y), (x, x, x)) ---- */

ADF_TEST(aliasing_of_mul_and_inv)
{
    slong cs[80], Ns[80];
    slong n = all_cosets(cs, Ns, 9), i, j;
    adf_ucoset_t x, y, z, a, b;

    adf_ucoset_init(x);
    adf_ucoset_init(y);
    adf_ucoset_init(z);
    adf_ucoset_init(a);
    adf_ucoset_init(b);
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
        {
            uc_raw(x, cs[i], Ns[i]);
            uc_raw(y, cs[j], Ns[j]);
            adf_ucoset_mul(z, x, y);
            adf_ucoset_set(a, x);
            adf_ucoset_set(b, y);
            adf_ucoset_mul(a, a, b);
            ADF_CHECK(adf_ucoset_identical(a, z));
            adf_ucoset_set(a, x);
            adf_ucoset_set(b, y);
            adf_ucoset_mul(b, a, b);
            ADF_CHECK(adf_ucoset_identical(b, z));
            adf_ucoset_set(a, x);
            adf_ucoset_mul(z, x, x);
            adf_ucoset_mul(a, a, a);
            ADF_CHECK(adf_ucoset_identical(a, z));
        }
    for (i = 0; i < n; i++)
    {
        uc_raw(x, cs[i], Ns[i]);
        adf_ucoset_inv(z, x);
        adf_ucoset_inv(x, x);
        ADF_CHECK(adf_ucoset_identical(x, z));
        uc_raw(x, cs[i], Ns[i]);
        adf_ucoset_normalise(z, x);
        adf_ucoset_normalise(x, x);
        ADF_CHECK(adf_ucoset_identical(x, z));
    }
    adf_ucoset_clear(x);
    adf_ucoset_clear(y);
    adf_ucoset_clear(z);
    adf_ucoset_clear(a);
    adf_ucoset_clear(b);
}

/* ---- oracle 1: enumeration in (Z/M)^x ---- */

ADF_TEST(enumeration_mul_inv_predicates)
{
    slong cs[80], Ns[80];
    slong n = all_cosets(cs, Ns, 14), i, j, k;
    ulong lev[3], M, a, b, maxM = 28 * 13 * 14 * 11;
    unsigned char *sx = malloc(maxM), *sy = malloc(maxM), *sz = malloc(maxM), *sp = malloc(maxM);
    adf_ucoset_t x, y, z, w;
    ulong n_pairs = 0, n_prod = 0, n_inv = 0;

    adf_ucoset_init(x);
    adf_ucoset_init(y);
    adf_ucoset_init(z);
    adf_ucoset_init(w);
    ADF_CHECK(n == 66);
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
        {
            ulong L = lcm_ul(Ns[i] ? (ulong) Ns[i] : 1, Ns[j] ? (ulong) Ns[j] : 1);
            int inside = 1, meet = 1, same = 1;

            uc_raw(x, cs[i], Ns[i]);
            uc_raw(y, cs[j], Ns[j]);
            lev[0] = 5 * L;
            lev[1] = 12 * L;
            lev[2] = 28 * L;
            adf_ucoset_mul(z, x, y);
            ADF_CHECK(adf_ucoset_is_canonical(z) && adf_ucoset_is_normal(z));
            ADF_CHECK(adf_ucoset_is_exact(z) == (Ns[i] == 0 && Ns[j] == 0));
            for (k = 0; k < 3; k++)
            {
                int in_k = 1, meet_k = 0, same_k = 1;
                M = lev[k];
                level_set(sx, x, M);
                level_set(sy, y, M);
                for (a = 0; a < M; a++)
                {
                    if (sx[a] && !sy[a])
                        in_k = 0;
                    if (sx[a] && sy[a])
                        meet_k = 1;
                    if (sx[a] != sy[a])
                        same_k = 0;
                }
                inside &= in_k;
                meet &= meet_k;
                same &= same_k;
                if (k < 2)
                {
                    /* the product set against the image of the result */
                    memset(sp, 0, M);
                    for (a = 0; a < M; a++)
                        if (sx[a])
                            for (b = 0; b < M; b++)
                                if (sy[b])
                                    sp[(a * b) % M] = 1;
                    level_set(sz, z, M);
                    ADF_CHECK_MSG(memcmp(sp, sz, M) == 0, "[%ld mod %ld] * [%ld mod %ld] at level %lu",
                                  (long) cs[i], (long) Ns[i], (long) cs[j], (long) Ns[j], (unsigned long) M);
                    n_prod++;
                }
            }
            ADF_CHECK_MSG(adf_ucoset_contains(x, y) == inside, "contains [%ld mod %ld] [%ld mod %ld]",
                          (long) cs[i], (long) Ns[i], (long) cs[j], (long) Ns[j]);
            ADF_CHECK_MSG(adf_ucoset_overlaps(x, y) == meet, "overlaps [%ld mod %ld] [%ld mod %ld]",
                          (long) cs[i], (long) Ns[i], (long) cs[j], (long) Ns[j]);
            ADF_CHECK_MSG(adf_ucoset_equal_set(x, y) == same, "equal_set [%ld mod %ld] [%ld mod %ld]",
                          (long) cs[i], (long) Ns[i], (long) cs[j], (long) Ns[j]);
            n_pairs++;
        }
    for (i = 0; i < n; i++)
    {
        ulong base = Ns[i] ? (ulong) Ns[i] : 1;

        uc_raw(x, cs[i], Ns[i]);
        adf_ucoset_inv(z, x);
        adf_ucoset_mul(w, x, z);
        ADF_CHECK(adf_ucoset_is_canonical(z) && adf_ucoset_is_normal(z));
        ADF_CHECK(adf_ucoset_is_exact(z) == (Ns[i] == 0));
        for (k = 0; k < 2; k++)
        {
            M = (k == 0 ? 5 : 12) * base;
            level_set(sx, x, M);
            memset(sp, 0, M);
            for (a = 0; a < M; a++)
                if (sx[a])
                {
                    for (b = 1; b < M && (a * b) % M != 1 % M; b++)
                        ;
                    sp[b % M] = 1;          /* by search: the oracle does not use FLINT's inverse */
                }
            level_set(sz, z, M);
            ADF_CHECK_MSG(memcmp(sp, sz, M) == 0, "inverse of [%ld mod %ld] at level %lu", (long) cs[i],
                          (long) Ns[i], (unsigned long) M);
            level_set(sz, w, M);
            ADF_CHECK(sz[1 % M] == 1);          /* the point 1 is in x * x^-1 */
            n_inv++;
        }
        adf_ucoset_inv(w, z);
        ADF_CHECK(adf_ucoset_equal_set(w, x));
    }
    printf("enumeration: %lu ordered pairs at 3 levels, %lu product sets, %lu inverse sets\n",
           (unsigned long) n_pairs, (unsigned long) n_prod, (unsigned long) n_inv);
    free(sx);
    free(sy);
    free(sz);
    free(sp);
    adf_ucoset_clear(x);
    adf_ucoset_clear(y);
    adf_ucoset_clear(z);
    adf_ucoset_clear(w);
}

/* ---- oracle 2: the vectors ---- */

ADF_TEST(vectors_of_the_reference)
{
    jsonl_file * f;
    jsonl_error_t err;
    size_t i, n_set = 0, n_mul = 0, n_inv = 0, n_dom = 0;
    adf_ucoset_t x, y, z, want, keep;
    fmpz_t c, N;

    if (!jsonl_open("tests/ref/vectors/i-slice1/ucoset.jsonl", &f, &err))
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    adf_ucoset_init(x);
    adf_ucoset_init(y);
    adf_ucoset_init(z);
    adf_ucoset_init(want);
    adf_ucoset_init(keep);
    fmpz_init(c);
    fmpz_init(N);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * op = read_str(rec, "op");

        if (strcmp(op, "set") == 0)
        {
            int st;
            read_int(c, rec, "c");
            read_int(N, rec, "N");
            uc_set_si2(z, 7, 12);
            adf_ucoset_set(keep, z);
            st = adf_ucoset_set_fmpz2(z, c, N);
            if (strcmp(read_str(rec, "status"), "OK") == 0)
            {
                read_coset(want, field(rec, "z"));
                ADF_CHECK_MSG(st == ADF_OK && adf_ucoset_identical(z, want), "line %zu", i + 1);
                read_coset(want, field(rec, "normal"));
                adf_ucoset_normalise(y, z);
                ADF_CHECK_MSG(adf_ucoset_identical(y, want) && adf_ucoset_is_normal(y), "line %zu", i + 1);
                ADF_CHECK(adf_ucoset_is_normal(z) == adf_ucoset_identical(z, y));
            }
            else
            {
                ADF_CHECK_MSG(st == ADF_DOMAIN && adf_ucoset_identical(z, keep), "line %zu", i + 1);
                n_dom++;
            }
            n_set++;
        }
        else if (strcmp(op, "inv") == 0)
        {
            read_coset(x, field(rec, "x"));
            read_coset(want, field(rec, "z"));
            ADF_CHECK(adf_ucoset_is_canonical(x));
            adf_ucoset_inv(z, x);
            ADF_CHECK_MSG(adf_ucoset_identical(z, want), "line %zu", i + 1);
            adf_ucoset_inv(x, x);
            ADF_CHECK(adf_ucoset_identical(x, want));
            n_inv++;
        }
        else if (strcmp(op, "mul") == 0)
        {
            read_coset(x, field(rec, "x"));
            read_coset(y, field(rec, "y"));
            read_coset(want, field(rec, "z"));
            ADF_CHECK(adf_ucoset_is_canonical(x) && adf_ucoset_is_canonical(y));
            adf_ucoset_mul(z, x, y);
            ADF_CHECK_MSG(adf_ucoset_identical(z, want), "line %zu", i + 1);
            ADF_CHECK_MSG(adf_ucoset_equal_set(x, y) == read_bool(rec, "equal_set"), "line %zu", i + 1);
            ADF_CHECK_MSG(adf_ucoset_contains(x, y) == read_bool(rec, "contains"), "line %zu", i + 1);
            ADF_CHECK_MSG(adf_ucoset_overlaps(x, y) == read_bool(rec, "overlaps"), "line %zu", i + 1);
            adf_ucoset_mul(x, x, y);
            ADF_CHECK(adf_ucoset_identical(x, want));
            n_mul++;
        }
        else
            ADF_CHECK_MSG(0, "unknown op %s on line %zu", op, i + 1);
    }
    printf("vectors: %zu lines: %zu set (%zu DOMAIN), %zu mul with 3 predicates, %zu inv\n", jsonl_count(f),
           n_set, n_dom, n_mul, n_inv);
    ADF_CHECK(n_set + n_mul + n_inv == jsonl_count(f) && jsonl_count(f) > 1000);
    jsonl_close(f);
    fmpz_clear(c);
    fmpz_clear(N);
    adf_ucoset_clear(x);
    adf_ucoset_clear(y);
    adf_ucoset_clear(z);
    adf_ucoset_clear(want);
    adf_ucoset_clear(keep);
}
