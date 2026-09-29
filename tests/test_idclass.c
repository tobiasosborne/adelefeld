/* tests/test_idclass.c: adf_idclass and the class map (lane i-slice2; include/adelefeld/idclass.h;
   docs/api-2.md 2.3, Statements F and G; docs/proofs/ideles.md P15; SPEC 5 "Sign preservation").

   The oracles:
   1. Exact rational end points (fmpq), as tests/test_idele.c: a real result must contain the exact end
      points of the set it encloses (abs(X)/r for the class map, T T' and 1/T for product and inverse),
      exclude 0 and be positive.
   2. Enumeration in Z/M: the set of a unit coset c U(N) at a level M (a multiple of N) is the set of the
      residues u in [0, M) with gcd(u, M) = 1 and u = c mod N (ideles.md, lines 4-8 of proto/ideles_checks.py;
      for an exact unit e, the one residue e mod M). The unit of a class must have exactly the image
      sign(X) * (image of u) at every level; products and inverses of class units the product and inverse
      images. The levels used (60 L and 84 L) have an odd prime that splits every coset with N >= 1 into at
      least two residues, so an exact unit and a coset are told apart.
   3. The group law and the kernel (P15.1): the class of q x has the same unit set as the class of x, and
      their real balls meet; the class of x y and the product of the classes likewise.
   4. tests/ref/vectors/i-slice2/idclass.jsonl, written by lanes/i-slice2/gen_vectors.py from part 3 of
      proto/ideles_checks.py. Every line is run. */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <flint/arb.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include <flint/ulong_extras.h>

#include <adelefeld/idclass.h>

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

/* 1 if z is finite, positive, and contains [L, H]. */
static int
encloses_pos(const arb_t z, const fmpq_t L, const fmpq_t H)
{
    return arb_is_finite(z) && arb_is_positive(z) && arb_contains_fmpq(z, L) && arb_contains_fmpq(z, H);
}

/* Oracle 1 for the class map: z contains abs(X)/r. */
static int
encloses_class_t(const arb_t z, const arb_t X, const fmpq_t r)
{
    fmpq_t L, H;
    int ok;
    fmpq_init(L);
    fmpq_init(H);
    abs_ends(L, H, X);
    fmpq_div(L, L, r);
    fmpq_div(H, H, r);
    ok = encloses_pos(z, L, H);
    fmpq_clear(L);
    fmpq_clear(H);
    return ok;
}

static int
encloses_mul_t(const arb_t z, const arb_t a, const arb_t b)
{
    fmpq_t La, Ha, Lb, Hb;
    int ok;
    fmpq_init(La);
    fmpq_init(Ha);
    fmpq_init(Lb);
    fmpq_init(Hb);
    abs_ends(La, Ha, a);
    abs_ends(Lb, Hb, b);
    fmpq_mul(La, La, Lb);
    fmpq_mul(Ha, Ha, Hb);
    ok = encloses_pos(z, La, Ha);
    fmpq_clear(La);
    fmpq_clear(Ha);
    fmpq_clear(Lb);
    fmpq_clear(Hb);
    return ok;
}

static int
encloses_inv_t(const arb_t z, const arb_t a)
{
    fmpq_t L, H;
    int ok;
    fmpq_init(L);
    fmpq_init(H);
    abs_ends(L, H, a);
    fmpq_inv(L, L);
    fmpq_inv(H, H);
    ok = encloses_pos(z, H, L);
    fmpq_clear(L);
    fmpq_clear(H);
    return ok;
}

static void
ucoset_si(adf_ucoset_t u, slong c, slong N)
{
    fmpz_t fc, fN;
    fmpz_init_set_si(fc, c);
    fmpz_init_set_si(fN, N);
    if (adf_ucoset_set_fmpz2(u, fc, fN) != ADF_OK)
        abort();
    fmpz_clear(fc);
    fmpz_clear(fN);
}

static void
idele_si(adf_idele_t x, const arb_t inf, slong rn, slong rd, slong c, slong N)
{
    fmpq_t r;
    adf_ucoset_t u;
    fmpq_init(r);
    adf_ucoset_init(u);
    fmpq_set_si(r, rn, (ulong) rd);
    ucoset_si(u, c, N);
    if (adf_idele_set_parts(x, inf, r, u) != ADF_OK)
        abort();
    fmpq_clear(r);
    adf_ucoset_clear(u);
}

static void
class_si(adf_idclass_t x, const arb_t t, slong c, slong N)
{
    adf_ucoset_t u;
    adf_ucoset_init(u);
    ucoset_si(u, c, N);
    if (adf_idclass_set_parts(x, t, u) != ADF_OK)
        abort();
    adf_ucoset_clear(u);
}

static int
unit_is(const adf_ucoset_t u, slong c, slong N)
{
    return fmpz_equal_si(u->c, c) && fmpz_equal_si(u->N, N);
}

/* ---- oracle 2: enumeration in Z/M ---- */

#define LEVEL_MAX 4096

/* in[v] = 1 for the residues v in [0, M) of the set of u (M a multiple of the modulus of u, M <= LEVEL_MAX). */
static void
level_set(unsigned char * in, const adf_ucoset_t u, slong M)
{
    slong c = fmpz_get_si(u->c), N = fmpz_get_si(u->N), v;
    memset(in, 0, (size_t) M);
    if (N == 0)
    {
        in[((c % M) + M) % M] = 1;
        return;
    }
    for (v = 0; v < M; v++)
        if (n_gcd((ulong) v, (ulong) M) == 1 && ((v - c) % N + N) % N == 0)
            in[v] = 1;
}

/* 1 if the set of w at the level M is {s a : a in the set of u} (s = +1 or -1). */
static int
level_is_signed(const adf_ucoset_t w, const adf_ucoset_t u, int s, slong M)
{
    unsigned char a[LEVEL_MAX], b[LEVEL_MAX], c[LEVEL_MAX];
    slong v;
    level_set(a, u, M);
    level_set(b, w, M);
    memset(c, 0, (size_t) M);
    for (v = 0; v < M; v++)
        if (a[v])
            c[((s * v) % M + M) % M] = 1;
    return memcmp(b, c, (size_t) M) == 0;
}

/* 1 if the set of w at the level M is {a b : a in u, b in v}. */
static int
level_is_product(const adf_ucoset_t w, const adf_ucoset_t u, const adf_ucoset_t v, slong M)
{
    unsigned char a[LEVEL_MAX], b[LEVEL_MAX], c[LEVEL_MAX], d[LEVEL_MAX];
    slong i, j;
    level_set(a, u, M);
    level_set(b, v, M);
    level_set(d, w, M);
    memset(c, 0, (size_t) M);
    for (i = 0; i < M; i++)
        if (a[i])
            for (j = 0; j < M; j++)
                if (b[j])
                    c[(i * j) % M] = 1;
    return memcmp(c, d, (size_t) M) == 0;
}

/* 1 if the set of w at the level M is {a^-1 : a in u}. */
static int
level_is_inverse(const adf_ucoset_t w, const adf_ucoset_t u, slong M)
{
    unsigned char a[LEVEL_MAX], c[LEVEL_MAX], d[LEVEL_MAX];
    slong i;
    level_set(a, u, M);
    level_set(d, w, M);
    memset(c, 0, (size_t) M);
    for (i = 0; i < M; i++)
        if (a[i])
            c[n_invmod((ulong) i, (ulong) M)] = 1;
    return memcmp(c, d, (size_t) M) == 0;
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
read_fmpz_at(fmpz_t z, const jsonl_value * arr, size_t i)
{
    jsonl_error_t err;
    const char * t;
    if (!jsonl_int_text_or_string(jsonl_at(arr, i, &err), &t, &err) || fmpz_set_str(z, t, 10) != 0)
        abort();
}

static slong
read_si(const jsonl_value * v)
{
    fmpz_t z;
    jsonl_error_t err;
    const char * t;
    slong s;
    if (!jsonl_int_text_or_string(v, &t, &err))
        abort();
    fmpz_init(z);
    fmpz_set_str(z, t, 10);
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
read_coset(adf_ucoset_t u, const jsonl_value * v)
{
    read_fmpz_at(u->c, v, 0);
    read_fmpz_at(u->N, v, 1);
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
    read_coset(u, field(v, "u"));
    if (!adf_ucoset_is_canonical(u) || adf_idele_set_parts(x, inf, r, u) != ADF_OK)
        abort();
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
    read_coset(u, field(v, "u"));
    if (!adf_ucoset_is_canonical(u) || adf_idclass_set_parts(x, t, u) != ADF_OK || !adf_idclass_is_canonical(x))
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

/* ---- layout, life cycle, constructors ---- */

ADF_TEST(layout_is_64_bytes)
{
    ADF_CHECK(adf_sizeof_idclass() == 64);
    ADF_CHECK(adf_alignof_idclass() == 8);
    ADF_CHECK(offsetof(adf_idclass_struct, t) == 0);
    ADF_CHECK(offsetof(adf_idclass_struct, u) == 48);
}

ADF_TEST(init_is_the_class_of_one)
{
    adf_idclass_t x;
    adf_idclass_init(x);
    ADF_CHECK(adf_idclass_is_canonical(x));
    ADF_CHECK(arb_is_one(x->t) && unit_is(&x->u, 1, 0));
    adf_idclass_clear(x);
}

ADF_TEST(is_canonical_is_the_class_predicate)
{
    adf_idclass_t x;
    adf_idclass_init(x);
    arb_set_si_si(x->t, 1, 0, 1, -1);                    /* 1 +- 1/2 */
    ADF_CHECK(adf_idclass_is_canonical(x));
    arb_set_si_si(x->t, 1, 0, 1, 0);                     /* 1 +- 1 contains 0 */
    ADF_CHECK(!adf_idclass_is_canonical(x));
    arb_set_si(x->t, -3);                                /* negative */
    ADF_CHECK(!adf_idclass_is_canonical(x));
    arb_zero(x->t);
    ADF_CHECK(!adf_idclass_is_canonical(x));
    arb_pos_inf(x->t);
    ADF_CHECK(!adf_idclass_is_canonical(x));
    arb_indeterminate(x->t);
    ADF_CHECK(!adf_idclass_is_canonical(x));
    arb_set_si(x->t, 3);
    mag_inf(arb_radref(x->t));
    ADF_CHECK(!adf_idclass_is_canonical(x));
    arb_set_si(x->t, 7);
    ADF_CHECK(adf_idclass_is_canonical(x));
    fmpz_set_si(x->u.c, 2);
    fmpz_set_si(x->u.N, 4);                              /* not a unit coset */
    ADF_CHECK(!adf_idclass_is_canonical(x));
    fmpz_set_si(x->u.c, 5);
    fmpz_set_si(x->u.N, 6);                              /* canonical, not normal: allowed */
    ADF_CHECK(adf_idclass_is_canonical(x));
    fmpz_set_si(x->u.c, 2);
    fmpz_set_si(x->u.N, 0);                              /* not an exact unit */
    ADF_CHECK(!adf_idclass_is_canonical(x));
    adf_idclass_clear(x);
}

ADF_TEST(set_parts_accessors_identical_swap)
{
    adf_idclass_t x, y, keep;
    adf_ucoset_t u, g;
    arb_t t, s;

    adf_idclass_init(x);
    adf_idclass_init(y);
    adf_idclass_init(keep);
    adf_ucoset_init(u);
    adf_ucoset_init(g);
    arb_init(t);
    arb_init(s);
    ucoset_si(u, 5, 6);
    arb_set_si_si(t, 5, -1, 1, -30);                     /* 2.5 +- 2^-30 */
    ADF_CHECK(adf_idclass_set_parts(x, t, u) == ADF_OK && adf_idclass_is_canonical(x));
    ADF_CHECK(arb_equal(x->t, t) && unit_is(&x->u, 5, 6));   /* the modulus as supplied (CV-17) */
    adf_idclass_get_t(s, x);
    ADF_CHECK(arb_equal(s, t));
    adf_idclass_norm(s, x);
    ADF_CHECK(arb_equal(s, t));
    adf_idclass_get_unit(g, x);
    ADF_CHECK(adf_ucoset_identical(g, u));
    /* refusals: x untouched */
    adf_idclass_set(keep, x);
    arb_set_si_si(t, 1, 0, 1, 0);
    ADF_CHECK(adf_idclass_set_parts(x, t, u) == ADF_DOMAIN && adf_idclass_identical(x, keep));
    arb_set_si(t, -2);
    ADF_CHECK(adf_idclass_set_parts(x, t, u) == ADF_DOMAIN && adf_idclass_identical(x, keep));
    arb_zero(t);
    ADF_CHECK(adf_idclass_set_parts(x, t, u) == ADF_DOMAIN && adf_idclass_identical(x, keep));
    arb_pos_inf(t);
    ADF_CHECK(adf_idclass_set_parts(x, t, u) == ADF_DOMAIN && adf_idclass_identical(x, keep));
    arb_indeterminate(t);
    ADF_CHECK(adf_idclass_set_parts(x, t, u) == ADF_DOMAIN && adf_idclass_identical(x, keep));
    arb_set_si(t, 2);
    mag_inf(arb_radref(t));
    ADF_CHECK(adf_idclass_set_parts(x, t, u) == ADF_DOMAIN && adf_idclass_identical(x, keep));
    /* identical, swap, set with itself */
    ADF_CHECK(adf_idclass_identical(x, keep) && !adf_idclass_identical(x, y));
    adf_idclass_swap(x, y);
    ADF_CHECK(adf_idclass_identical(y, keep) && arb_is_one(x->t) && unit_is(&x->u, 1, 0));
    adf_idclass_set(y, y);
    ADF_CHECK(adf_idclass_identical(y, keep));
    /* identical: same ball, other stored pair of the same set */
    ucoset_si(u, 2, 3);
    arb_set(t, keep->t);
    ADF_CHECK(adf_idclass_set_parts(x, t, u) == ADF_OK && !adf_idclass_identical(x, keep));
    arb_clear(t);
    arb_clear(s);
    adf_ucoset_clear(u);
    adf_ucoset_clear(g);
    adf_idclass_clear(x);
    adf_idclass_clear(y);
    adf_idclass_clear(keep);
}

/* ---- the class map ---- */

ADF_TEST(class_map_hand_cases_and_the_sign_on_the_unit)
{
    adf_idele_t x;
    adf_idclass_t c;
    adf_rat_t q;
    arb_t inf;
    fmpq_t f;

    adf_idele_init(x);
    adf_idclass_init(c);
    adf_rat_init(q);
    arb_init(inf);
    fmpq_init(f);
    /* (1 ; 1 [-1]) has the class <1 ; [-1]>: not the class of a rational (P15.3) */
    arb_set_si(inf, 1);
    idele_si(x, inf, 1, 1, -1, 0);
    ADF_CHECK(adf_idclass_set_idele(c, x, 64) == ADF_OK && arb_is_one(c->t) && unit_is(&c->u, -1, 0));
    /* (-1 ; 1 [-1]) is the rational -1: the class <1 ; [1]> */
    arb_set_si(inf, -1);
    idele_si(x, inf, 1, 1, -1, 0);
    ADF_CHECK(adf_idclass_set_idele(c, x, 64) == ADF_OK && arb_is_one(c->t) && unit_is(&c->u, 1, 0));
    /* (-2.5 +- 0.25 ; 3/2 [5 mod 36]): unit [31 mod 36], t in [1.5, 11/6] */
    arb_set_si_si(inf, -5, -1, 1, -2);
    idele_si(x, inf, 3, 2, 5, 36);
    fmpq_set_si(f, 3, 2);
    ADF_CHECK(adf_idclass_set_idele(c, x, 64) == ADF_OK && unit_is(&c->u, 31, 36)
              && encloses_class_t(c->t, inf, f) && adf_idclass_is_canonical(c));
    /* a positive ball keeps the unit; the result is in normal form: [5 mod 6] -> [2 mod 3] */
    arb_set_si(inf, 6);
    idele_si(x, inf, 3, 2, 5, 6);
    ADF_CHECK(adf_idclass_set_idele(c, x, 64) == ADF_OK && unit_is(&c->u, 2, 3) && arb_equal_si(c->t, 4));
    /* a negative one: -[5 mod 6] = [1 mod 6] = [1 mod 3] */
    arb_set_si(inf, -6);
    idele_si(x, inf, 3, 2, 5, 6);
    ADF_CHECK(adf_idclass_set_idele(c, x, 64) == ADF_OK && unit_is(&c->u, 1, 3) && arb_equal_si(c->t, 4));
    /* [1 mod 1] (every unit) stays [1 mod 1] under the sign */
    idele_si(x, inf, 3, 2, 1, 1);
    ADF_CHECK(adf_idclass_set_idele(c, x, 64) == ADF_OK && unit_is(&c->u, 1, 1));
    /* the idele of -3/4: the exact class <1 ; [1]>; of -6/35: <t ; [1]> with 1 in t, t not exact */
    fmpq_set_si(q->q, -3, 4);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    ADF_CHECK(adf_idclass_set_idele(c, x, 64) == ADF_OK && arb_is_one(c->t) && unit_is(&c->u, 1, 0));
    fmpq_set_si(q->q, -6, 35);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    ADF_CHECK(adf_idclass_set_idele(c, x, 64) == ADF_OK && arb_contains_si(c->t, 1) && !arb_is_exact(c->t)
              && unit_is(&c->u, 1, 0));
    fmpq_clear(f);
    arb_clear(inf);
    adf_rat_clear(q);
    adf_idele_clear(x);
    adf_idclass_clear(c);
}

ADF_TEST(class_map_units_against_enumeration)
{
    /* every coset (c, N) with N <= 20 and both exact units, both signs of the real ball: the class unit at
       the levels 60 L and 84 L is exactly sign * (the unit of the idele) */
    adf_idele_t x;
    adf_idclass_t c;
    adf_ucoset_t u;
    arb_t inf;
    fmpq_t r;
    slong N, cc, s, k = 0;

    adf_idele_init(x);
    adf_idclass_init(c);
    adf_ucoset_init(u);
    arb_init(inf);
    fmpq_init(r);
    fmpq_set_si(r, 7, 3);
    for (N = 0; N <= 20; N++)
        for (cc = (N == 0 ? -1 : 1); cc <= (N == 0 ? 1 : N); cc += (N == 0 ? 2 : 1))
        {
            if (N >= 1 && n_gcd((ulong) cc, (ulong) N) != 1)
                continue;
            ucoset_si(u, cc, N);
            for (s = -1; s <= 1; s += 2)
            {
                slong L = N == 0 ? 1 : N;
                arb_set_si_si(inf, s * (2 * k + 3), -1, 1, -3);
                ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_OK);
                ADF_CHECK(adf_idclass_set_idele(c, x, 53) == ADF_OK);
                ADF_CHECK(adf_ucoset_is_normal(&c->u) && encloses_class_t(c->t, inf, r));
                ADF_CHECK_MSG(level_is_signed(&c->u, u, (int) s, 60 * L)
                              && level_is_signed(&c->u, u, (int) s, 84 * L),
                              "(%ld, %ld), sign %ld", (long) cc, (long) N, (long) s);
                ADF_CHECK((N == 0) == adf_ucoset_is_exact(&c->u));
                k = (k + 1) % 50;
            }
        }
    fmpq_clear(r);
    arb_clear(inf);
    adf_ucoset_clear(u);
    adf_idele_clear(x);
    adf_idclass_clear(c);
}

ADF_TEST(class_of_q_times_x_is_the_class_of_x)
{
    /* P15.1 (kernel Q^x), Statement G.3; also the homomorphism: class(x y) against class(x) class(y) */
    static const slong qs[][2] = { { -1, 1 }, { 3, 1 }, { -4, 9 }, { 7, 1024 }, { -1000003, 3 }, { 1, 5 } };
    adf_idele_t x, y, z, w;
    adf_idclass_t cx, cz, cy, cw, cp;
    adf_rat_t q;
    arb_t inf;
    slong k;
    size_t i;
    int n_checked = 0;

    adf_idele_init(x);
    adf_idele_init(y);
    adf_idele_init(z);
    adf_idele_init(w);
    adf_idclass_init(cx);
    adf_idclass_init(cz);
    adf_idclass_init(cy);
    adf_idclass_init(cw);
    adf_idclass_init(cp);
    adf_rat_init(q);
    arb_init(inf);
    for (k = 0; k < 24; k++)
    {
        slong N = 1 + (k * 7) % 30, c = 1;
        while (n_gcd((ulong) c, (ulong) N) != 1 || c < (k % 5))
            c++;
        arb_set_si_si(inf, (k & 1 ? -1 : 1) * (9 + 4 * k), -2, (UWORD(1) << (k % 20)) - 1, -(k % 20) - 1);
        idele_si(x, inf, 5 + k, 3 + 2 * k, c, N);
        arb_set_si_si(inf, (k & 2 ? -1 : 1) * (1 + 2 * k), -3, 3, -6);
        idele_si(y, inf, 2, 7, N == 1 ? 1 : N - 1, N);
        ADF_CHECK(adf_idclass_set_idele(cx, x, 64) == ADF_OK);
        ADF_CHECK(adf_idclass_is_canonical(cx) && encloses_class_t(cx->t, x->inf, x->r));
        ADF_CHECK(level_is_signed(&cx->u, &x->u, arf_sgn(arb_midref(x->inf)), 60 * N));
        for (i = 0; i < sizeof(qs) / sizeof(qs[0]); i++)
        {
            fmpq_set_si(q->q, qs[i][0], (ulong) qs[i][1]);
            ADF_CHECK(adf_idele_mul_rat(z, x, q, 64) == ADF_OK);
            ADF_CHECK(adf_idclass_set_idele(cz, z, 64) == ADF_OK);
            ADF_CHECK(adf_idclass_is_canonical(cz) && encloses_class_t(cz->t, z->inf, z->r));
            ADF_CHECK_MSG(adf_ucoset_equal_set(&cz->u, &cx->u) && arb_overlaps(cz->t, cx->t), "k = %ld, i = %zu",
                          (long) k, i);
            n_checked++;
        }
        /* the homomorphism: class(x y) and class(x) class(y) */
        ADF_CHECK(adf_idele_mul(w, x, y, 64) == ADF_OK && adf_idclass_set_idele(cw, w, 64) == ADF_OK);
        ADF_CHECK(adf_idclass_set_idele(cy, y, 64) == ADF_OK && adf_idclass_mul(cp, cx, cy, 64) == ADF_OK);
        ADF_CHECK(adf_ucoset_equal_set(&cw->u, &cp->u) && arb_overlaps(cw->t, cp->t));
        /* the class of x^-1 is the inverse of the class of x */
        ADF_CHECK(adf_idele_inv(w, x, 64) == ADF_OK && adf_idclass_set_idele(cw, w, 64) == ADF_OK);
        ADF_CHECK(adf_idclass_inv(cp, cx, 64) == ADF_OK);
        ADF_CHECK(adf_ucoset_equal_set(&cw->u, &cp->u) && arb_overlaps(cw->t, cp->t));
    }
    ADF_CHECK(n_checked == 24 * 6);
    arb_clear(inf);
    adf_rat_clear(q);
    adf_idele_clear(x);
    adf_idele_clear(y);
    adf_idele_clear(z);
    adf_idele_clear(w);
    adf_idclass_clear(cx);
    adf_idclass_clear(cz);
    adf_idclass_clear(cy);
    adf_idclass_clear(cw);
    adf_idclass_clear(cp);
}

ADF_TEST(class_map_not_determined_and_prec)
{
    adf_idele_t x;
    adf_idclass_t c, keep, w;
    arb_t inf;
    fmpq_t r;

    adf_idele_init(x);
    adf_idclass_init(c);
    adf_idclass_init(keep);
    adf_idclass_init(w);
    arb_init(inf);
    fmpq_init(r);
    /* x = -(1 +- (1 - 2^-30)), content 5/3: the ends of abs(X)/r are about 2^31 apart */
    arb_set_si_si(inf, -1, 0, (UWORD(1) << 30) - 1, -30);
    idele_si(x, inf, 5, 3, 7, 12);
    arb_set_si(inf, 11);
    class_si(c, inf, 1, 5);
    adf_idclass_set(keep, c);
    ADF_CHECK(adf_idclass_set_idele(c, x, 16) == ADF_NOT_DETERMINED && adf_idclass_identical(c, keep));
    ADF_CHECK(adf_idclass_set_idele(c, x, 30) == ADF_NOT_DETERMINED && adf_idclass_identical(c, keep));
    ADF_CHECK(adf_idclass_set_idele(c, x, 64) == ADF_OK && unit_is(&c->u, 5, 12));
    fmpq_set_si(r, 5, 3);
    ADF_CHECK(encloses_class_t(c->t, x->inf, r));
    /* prec below 2 is 2 */
    arb_set_si_si(inf, 7, -3, 1, -5);
    idele_si(x, inf, 1, 3, 1, 1);
    ADF_CHECK(adf_idclass_set_idele(w, x, 2) == ADF_OK);
    ADF_CHECK(adf_idclass_set_idele(c, x, 1) == ADF_OK && adf_idclass_identical(c, w));
    ADF_CHECK(adf_idclass_set_idele(c, x, 0) == ADF_OK && adf_idclass_identical(c, w));
    ADF_CHECK(adf_idclass_set_idele(c, x, WORD_MIN) == ADF_OK && adf_idclass_identical(c, w));
    fmpq_clear(r);
    arb_clear(inf);
    adf_idele_clear(x);
    adf_idclass_clear(c);
    adf_idclass_clear(keep);
    adf_idclass_clear(w);
}

ADF_TEST(class_map_huge_operands)
{
    /* content 3^1000 / 2^3000, ball of exponent 5000: t = abs(X) 2^3000 / 3^1000 */
    adf_idele_t x;
    adf_idclass_t c;
    arb_t inf;
    fmpq_t r;
    fmpz_t m, e;
    adf_ucoset_t u;

    adf_idele_init(x);
    adf_idclass_init(c);
    arb_init(inf);
    fmpq_init(r);
    fmpz_init(m);
    fmpz_init(e);
    adf_ucoset_init(u);
    fmpz_set_ui(fmpq_numref(r), 3);
    fmpz_pow_ui(fmpq_numref(r), fmpq_numref(r), 1000);
    fmpz_one(fmpq_denref(r));
    fmpz_mul_2exp(fmpq_denref(r), fmpq_denref(r), 3000);
    fmpz_set_si(m, -987654321);
    fmpz_set_si(e, 5000);
    arb_set_exact(inf, m, e, 12345, 4990);
    ucoset_si(u, 7, 40);
    ADF_CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_OK);
    ADF_CHECK(adf_idclass_set_idele(c, x, 200) == ADF_OK && encloses_class_t(c->t, inf, r));
    ADF_CHECK(unit_is(&c->u, 33, 40) && arb_rel_accuracy_bits(c->t) >= 5);
    fmpz_clear(m);
    fmpz_clear(e);
    fmpq_clear(r);
    arb_clear(inf);
    adf_ucoset_clear(u);
    adf_idele_clear(x);
    adf_idclass_clear(c);
}

/* ---- class product and inverse ---- */

ADF_TEST(spec5_sign_preservation_for_classes)
{
    adf_idclass_t x, z, keep;
    arb_t t;
    fmpq_t L, H;
    slong p;

    adf_idclass_init(x);
    adf_idclass_init(z);
    adf_idclass_init(keep);
    arb_init(t);
    fmpq_init(L);
    fmpq_init(H);
    arb_set_si_si(t, 1, 0, (UWORD(1) << 30) - 1, -30);  /* 1 +- (1 - 2^-30), positive */
    class_si(x, t, 1, 0);
    arb_mul(t, x->t, x->t, 128);
    ADF_CHECK(arb_contains_zero(t));                    /* arb_mul loses the sign */
    fmpq_set_si(L, 1, 1);
    fmpz_mul_2exp(fmpq_denref(L), fmpq_denref(L), 60);
    fmpq_set_si(H, (WORD(1) << 31) - 1, UWORD(1) << 30);
    fmpq_mul(H, H, H);
    for (p = 2; p <= 140; p++)
    {
        adf_idclass_set(z, keep);
        int st = adf_idclass_mul(z, x, x, p);
        if (p <= 60)
            ADF_CHECK_MSG(st == ADF_NOT_DETERMINED && adf_idclass_identical(z, keep), "prec %ld", (long) p);
        else
            ADF_CHECK_MSG(st == ADF_OK && encloses_pos(z->t, L, H) && unit_is(&z->u, 1, 0), "prec %ld", (long) p);
    }
    fmpq_clear(L);
    fmpq_clear(H);
    arb_clear(t);
    adf_idclass_clear(x);
    adf_idclass_clear(z);
    adf_idclass_clear(keep);
}

ADF_TEST(mul_inv_units_against_enumeration_and_the_class_of_one)
{
    adf_idclass_t x, y, z, a, b;
    arb_t t;
    slong N, M, c, c2, k = 0;

    adf_idclass_init(x);
    adf_idclass_init(y);
    adf_idclass_init(z);
    adf_idclass_init(a);
    adf_idclass_init(b);
    arb_init(t);
    for (N = 0; N <= 12; N++)
        for (M = 0; M <= 12; M += 3)
            for (c = (N == 0 ? -1 : 1); c <= (N == 0 ? 1 : N); c += (N == 0 ? 2 : 1))
            {
                slong L;
                if (N >= 1 && n_gcd((ulong) c, (ulong) N) != 1)
                    continue;
                c2 = M == 0 ? (k & 1 ? 1 : -1) : 1;
                while (M >= 1 && n_gcd((ulong) c2, (ulong) M) != 1)
                    c2++;
                L = (N == 0 ? 1 : N) * (M == 0 ? 1 : M) * 60;
                if (L > LEVEL_MAX)
                    L = (N == 0 ? 1 : N) * (M == 0 ? 1 : M) * 5;
                arb_set_si_si(t, 3 + 2 * k, -2, 3, -5);
                class_si(x, t, c, N);
                arb_set_si_si(t, 5 + 4 * k, -3, (UWORD(1) << (k % 25)) - 1, -(k % 25) - 3);
                class_si(y, t, c2, M);
                ADF_CHECK(adf_idclass_mul(z, x, y, 53) == ADF_OK);
                ADF_CHECK(adf_idclass_is_canonical(z) && adf_ucoset_is_normal(&z->u));
                ADF_CHECK(encloses_mul_t(z->t, x->t, y->t));
                ADF_CHECK_MSG(level_is_product(&z->u, &x->u, &y->u, L), "(%ld, %ld) (%ld, %ld)", (long) c, (long) N,
                              (long) c2, (long) M);
                /* aliasing: z = x, z = y, z = x = y */
                adf_idclass_set(a, x);
                adf_idclass_set(b, y);
                ADF_CHECK(adf_idclass_mul(a, a, b, 53) == ADF_OK && adf_idclass_identical(a, z));
                adf_idclass_set(a, x);
                ADF_CHECK(adf_idclass_mul(b, a, b, 53) == ADF_OK && adf_idclass_identical(b, z));
                ADF_CHECK(adf_idclass_mul(z, x, x, 53) == ADF_OK);
                adf_idclass_set(a, x);
                ADF_CHECK(adf_idclass_mul(a, a, a, 53) == ADF_OK && adf_idclass_identical(a, z));
                /* inverse, aliased, and the class of 1 in x x^-1 */
                ADF_CHECK(adf_idclass_inv(z, x, 53) == ADF_OK && encloses_inv_t(z->t, x->t));
                ADF_CHECK(level_is_inverse(&z->u, &x->u, (N == 0 ? 1 : N) * 60));
                adf_idclass_set(a, x);
                ADF_CHECK(adf_idclass_inv(a, a, 53) == ADF_OK && adf_idclass_identical(a, z));
                ADF_CHECK(adf_idclass_mul(a, x, z, 53) == ADF_OK && arb_contains_si(a->t, 1));
                {
                    adf_ucoset_t one;
                    adf_ucoset_init(one);
                    ADF_CHECK(adf_ucoset_contains(one, &a->u));
                    adf_ucoset_clear(one);
                }
                k++;
            }
    ADF_CHECK(k > 150);
    arb_clear(t);
    adf_idclass_clear(x);
    adf_idclass_clear(y);
    adf_idclass_clear(z);
    adf_idclass_clear(a);
    adf_idclass_clear(b);
}

ADF_TEST(mul_inv_not_determined_leave_the_output_untouched)
{
    adf_idclass_t x, z, keep;
    arb_t t;

    adf_idclass_init(x);
    adf_idclass_init(z);
    adf_idclass_init(keep);
    arb_init(t);
    arb_set_si_si(t, 1, 0, (UWORD(1) << 29) - 1, -29);
    class_si(x, t, 5, 12);
    arb_set_si(t, 3);
    class_si(z, t, 1, 1);
    adf_idclass_set(keep, z);
    ADF_CHECK(adf_idclass_inv(z, x, 16) == ADF_NOT_DETERMINED && adf_idclass_identical(z, keep));
    ADF_CHECK(adf_idclass_mul(z, x, x, 16) == ADF_NOT_DETERMINED && adf_idclass_identical(z, keep));
    adf_idclass_set(keep, x);
    ADF_CHECK(adf_idclass_inv(x, x, 16) == ADF_NOT_DETERMINED && adf_idclass_identical(x, keep));
    ADF_CHECK(adf_idclass_mul(x, x, x, 16) == ADF_NOT_DETERMINED && adf_idclass_identical(x, keep));
    ADF_CHECK(adf_idclass_inv(z, x, 64) == ADF_OK && encloses_inv_t(z->t, x->t) && unit_is(&z->u, 5, 12));
    ADF_CHECK(adf_idclass_inv(z, x, 1) == ADF_NOT_DETERMINED);
    arb_clear(t);
    adf_idclass_clear(x);
    adf_idclass_clear(z);
    adf_idclass_clear(keep);
}

/* ---- the precision limit (orchestrator, 2026-09-30) ---- */

ADF_TEST(prec_above_the_limit_is_LIMIT_and_untouched)
{
    static const slong big[] = { ADF_IDELE_PREC_MAX + 1, WORD(1) << 40, WORD_MAX };
    adf_idele_t xi;
    adf_idclass_t x, z, keep;
    adf_rat_t q;
    size_t i;

    adf_idele_init(xi);
    adf_idclass_init(x);
    adf_idclass_init(z);
    adf_idclass_init(keep);
    adf_rat_init(q);
    fmpq_set_si(q->q, -1, 3);
    ADF_CHECK(adf_idele_set_rat(xi, q, 64) == ADF_OK);
    ADF_CHECK(adf_idclass_set_idele(x, xi, 64) == ADF_OK);
    arb_set_si(z->t, 7);
    adf_idclass_set(keep, z);
    for (i = 0; i < sizeof(big) / sizeof(big[0]); i++)
    {
        slong p = big[i];
        ADF_CHECK_MSG(adf_idclass_set_idele(z, xi, p) == ADF_LIMIT && adf_idclass_identical(z, keep), "prec %ld",
                      (long) p);
        ADF_CHECK_MSG(adf_idclass_mul(z, x, x, p) == ADF_LIMIT && adf_idclass_identical(z, keep), "prec %ld",
                      (long) p);
        ADF_CHECK_MSG(adf_idclass_inv(z, x, p) == ADF_LIMIT && adf_idclass_identical(z, keep), "prec %ld",
                      (long) p);
        adf_idclass_set(z, x);
        ADF_CHECK(adf_idclass_inv(z, z, p) == ADF_LIMIT && adf_idclass_identical(z, x));
        ADF_CHECK(adf_idclass_mul(z, z, z, p) == ADF_LIMIT && adf_idclass_identical(z, x));
        adf_idclass_set(z, keep);
    }
    ADF_CHECK(adf_idclass_set_idele(z, xi, ADF_IDELE_PREC_MAX) == ADF_OK && arb_contains_si(z->t, 1));
    ADF_CHECK(adf_idclass_inv(z, x, ADF_IDELE_PREC_MAX) == ADF_OK && arb_contains_si(z->t, 1));
    ADF_CHECK(adf_idclass_mul(z, x, x, ADF_IDELE_PREC_MAX) == ADF_OK && arb_contains_si(z->t, 1));
    adf_rat_clear(q);
    adf_idele_clear(xi);
    adf_idclass_clear(x);
    adf_idclass_clear(z);
    adf_idclass_clear(keep);
}

/* ---- oracle 4: the vectors ---- */

ADF_TEST(vectors_of_the_reference)
{
    jsonl_file * f;
    jsonl_error_t err;
    size_t i, n_cls = 0, n_mul = 0, n_inv = 0, n_nd = 0, n_exact = 0;
    adf_idele_t xi;
    adf_idclass_t x, y, z, keep;
    adf_ucoset_t u;
    fmpq_t L, H;
    arf_t lo, hi;

    if (!jsonl_open("tests/ref/vectors/i-slice2/idclass.jsonl", &f, &err))
    {
        ADF_CHECK_MSG(0, "%s", jsonl_error_message(&err));
        return;
    }
    adf_idele_init(xi);
    adf_idclass_init(x);
    adf_idclass_init(y);
    adf_idclass_init(z);
    adf_idclass_init(keep);
    adf_ucoset_init(u);
    fmpq_init(L);
    fmpq_init(H);
    arf_init(lo);
    arf_init(hi);
    arb_set_si(keep->t, 3);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const char * op = str_of(rec, "op");
        slong prec = read_si(field(rec, "prec"));
        int want = status_of(str_of(rec, "status")), st;

        adf_idclass_set(z, keep);
        if (strcmp(op, "class") == 0)
        {
            read_idele(xi, field(rec, "x"));
            st = adf_idclass_set_idele(z, xi, prec);
            n_cls++;
        }
        else if (strcmp(op, "mul") == 0)
        {
            read_class(x, field(rec, "x"));
            read_class(y, field(rec, "y"));
            st = adf_idclass_mul(z, x, y, prec);
            n_mul++;
        }
        else if (strcmp(op, "inv") == 0)
        {
            read_class(x, field(rec, "x"));
            st = adf_idclass_inv(z, x, prec);
            n_inv++;
        }
        else
        {
            ADF_CHECK_MSG(0, "unknown op %s on line %zu", op, i + 1);
            continue;
        }
        ADF_CHECK_MSG(st == want, "line %zu: status %d, want %d", i + 1, st, want);
        if (st != ADF_OK || want != ADF_OK)
        {
            ADF_CHECK_MSG(adf_idclass_identical(z, keep), "line %zu: output touched", i + 1);
            n_nd += (want == ADF_NOT_DETERMINED);
            continue;
        }
        read_arf(lo, field(rec, "lo"));
        read_arf(hi, field(rec, "hi"));
        read_fmpq(L, field(rec, "L"));
        read_fmpq(H, field(rec, "H"));
        ADF_CHECK_MSG(adf_idclass_is_canonical(z) && encloses_pos(z->t, L, H), "line %zu", i + 1);
        ADF_CHECK_MSG(read_si(field(rec, "sign")) == 1, "line %zu", i + 1);
        ADF_CHECK_MSG(arb_contains_arf(z->t, lo) && arb_contains_arf(z->t, hi), "line %zu", i + 1);
        if (arf_equal(lo, hi))
        {
            ADF_CHECK_MSG(arb_is_exact(z->t) && arf_equal(arb_midref(z->t), lo), "line %zu", i + 1);
            n_exact++;
        }
        read_coset(u, field(rec, "u"));
        ADF_CHECK_MSG(adf_ucoset_identical(&z->u, u), "line %zu", i + 1);
        /* aliasing: the output is the (first) input */
        if (strcmp(op, "mul") == 0)
        {
            ADF_CHECK(adf_idclass_mul(x, x, y, prec) == ADF_OK && adf_idclass_identical(x, z));
            read_class(x, field(rec, "x"));
            ADF_CHECK(adf_idclass_mul(y, x, y, prec) == ADF_OK && adf_idclass_identical(y, z));
        }
        else if (strcmp(op, "inv") == 0)
            ADF_CHECK(adf_idclass_inv(x, x, prec) == ADF_OK && adf_idclass_identical(x, z));
    }
    printf("vectors: %zu lines: %zu class, %zu mul, %zu inv; %zu NOT_DETERMINED, %zu exact results\n",
           jsonl_count(f), n_cls, n_mul, n_inv, n_nd, n_exact);
    ADF_CHECK(n_cls + n_mul + n_inv == jsonl_count(f) && n_nd > 100);
    jsonl_close(f);
    arf_clear(lo);
    arf_clear(hi);
    fmpq_clear(L);
    fmpq_clear(H);
    adf_ucoset_clear(u);
    adf_idele_clear(xi);
    adf_idclass_clear(x);
    adf_idclass_clear(y);
    adf_idclass_clear(z);
    adf_idclass_clear(keep);
}
