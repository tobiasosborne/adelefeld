/* sball.c: adf_sball, a partial ball over a finite set of places (milestone 1F.1, second slice of 1F;
   include/adelefeld/sball.h).

   Contract: docs/conventions.md 5.9 (struct and predicate), 7 (places; canonical order: the archimedean place first,
   then the primes increasing), 3.3 (the reported place), 4.1, 4.3; docs/proofs/functions.md Proposition 22 (line 725:
   applying f after projection to a finite place set S is forming (f_v(x_v)) for v in S, the result typed over S);
   the statements S1 to S4 of docs/api-1f.md. The components at the primes are adf_lball (src/lball.c, lane f-slice1);
   the real component is an arb at the working precision (refs/src/flint-3.0.1/arb.rst: arb_add line 767, arb_sub 785,
   arb_mul 798, arb_neg 729; acb.rst: acb_contains 302, acb_overlaps 267, acb_is_finite 224, acb_equal 240).

   Every function that builds a value builds it in a temporary and swaps it into the output only on ADF_OK
   (conventions 4.3), so aliasing of the output with an input is free and a status leaves the output untouched.
   The array loc is allocated with flint_malloc (sball.h, "Ownership"). */

#include <stdlib.h>

#include <adelefeld.h>
#include <flint/flint.h>

#ifdef ADF_CHECK_INVARIANTS
#include <stdio.h>
#define ADF_INV_SBALL(x)                                                                                    \
    do                                                                                                      \
    {                                                                                                       \
        if (!adf_sball_is_canonical(x))                                                                     \
        {                                                                                                   \
            fprintf(stderr, "adelefeld: ADF_CHECK_INVARIANTS: %s: argument %s is not a canonical adf_sball\n", \
                    __func__, #x);                                                                          \
            fflush(stderr);                                                                                 \
            flint_abort();                                                                                  \
        }                                                                                                   \
    }                                                                                                       \
    while (0)
#define ADF_INV_ADELE_ARG(x)                                                                                \
    do                                                                                                      \
    {                                                                                                       \
        if (!adf_adele_is_canonical(x))                                                                     \
        {                                                                                                   \
            fprintf(stderr, "adelefeld: ADF_CHECK_INVARIANTS: %s: argument %s is not a canonical adf_adele\n", \
                    __func__, #x);                                                                          \
            fflush(stderr);                                                                                 \
            flint_abort();                                                                                  \
        }                                                                                                   \
    }                                                                                                       \
    while (0)
#else
#define ADF_INV_SBALL(x) ((void) 0)
#define ADF_INV_ADELE_ARG(x) ((void) 0)
#endif

/* ---- helpers ---- */

static void sb_swap(adf_sball_struct * x, adf_sball_struct * y);

/* t = the empty value with n initialised components (not yet filled) and the tag arch; inf = 0. */
static void
sb_alloc(adf_sball_struct * t, int arch, slong n)
{
    slong i;
    adf_sball_init(t);
    t->arch = arch;
    t->len = n;
    if (n > 0)
    {
        t->loc = (adf_lball_struct *) flint_malloc((size_t) n * sizeof(adf_lball_struct));
        for (i = 0; i < n; i++)
            adf_lball_init(&t->loc[i]);
    }
}

/* The place of the prime p (p is the prime of a canonical component: the call cannot fail). */
static adf_place_t
place_of_prime(ulong p)
{
    adf_place_t v;
    int st = adf_place_prime(&v, p);
    if (st != ADF_OK)
        flint_abort();   /* a canonical component has a prime: this is a defect of the library, not an input */
    return v;
}

static void
report(adf_place_t * where, adf_place_t v)
{
    if (where != NULL)
        *where = v;
}

/* ---- life cycle ---- */

void
adf_sball_init(adf_sball_t x)
{
    x->arch = ADF_ARCH_NONE;
    acb_init(x->inf);
    x->len = 0;
    x->loc = NULL;
}

void
adf_sball_clear(adf_sball_t x)
{
    slong i;
    for (i = 0; i < x->len && x->loc != NULL; i++)
        adf_lball_clear(&x->loc[i]);
    if (x->loc != NULL)
        flint_free(x->loc);
    x->loc = NULL;
    x->len = 0;
    acb_clear(x->inf);
}

void
adf_sball_set(adf_sball_t y, const adf_sball_t x)
{
    adf_sball_struct t;
    slong i;
    ADF_INV_SBALL(x);   /* also when y == x (conventions 4.4; finding R4) */
    if (y == x)
        return;
    sb_alloc(&t, x->arch, x->len);
    acb_set(t.inf, x->inf);
    for (i = 0; i < x->len; i++)
        adf_lball_set(&t.loc[i], &x->loc[i]);
    sb_swap(y, &t);
    adf_sball_clear(&t);
}

void
adf_sball_swap(adf_sball_t x, adf_sball_t y)
{
    ADF_INV_SBALL(x);   /* both arguments, also when x == y (conventions 4.4; finding R4) */
    ADF_INV_SBALL(y);
    sb_swap(x, y);
}

/* The exchange without a check of the predicate: for the temporaries of this file, which are canonical, and so that
   an OUTPUT argument (overwritten, not read) is not checked. */
static void
sb_swap(adf_sball_struct * x, adf_sball_struct * y)
{
    int a;
    slong n;
    adf_lball_struct * l;
    if (x == y)
        return;
    a = x->arch;
    x->arch = y->arch;
    y->arch = a;
    acb_swap(x->inf, y->inf);
    n = x->len;
    x->len = y->len;
    y->len = n;
    l = x->loc;
    x->loc = y->loc;
    y->loc = l;
}

int
adf_sball_is_canonical(const adf_sball_t x)
{
    slong i;
    if (x->arch != ADF_ARCH_NONE && x->arch != ADF_ARCH_REAL && x->arch != ADF_ARCH_COMPLEX)
        return 0;
    if (!acb_is_finite(x->inf))
        return 0;
    if (x->arch == ADF_ARCH_REAL && !arb_is_zero(acb_imagref(x->inf)))
        return 0;
    if (x->arch == ADF_ARCH_NONE && !acb_is_zero(x->inf))
        return 0;
    if (x->len < 0 || (x->len > 0 && x->loc == NULL))
        return 0;
    for (i = 0; i < x->len; i++)
    {
        if (!adf_lball_is_canonical(&x->loc[i]))
            return 0;
        if (i > 0 && !(x->loc[i - 1].p < x->loc[i].p))
            return 0;
    }
    return 1;
}

int
adf_sball_identical(const adf_sball_t x, const adf_sball_t y)
{
    slong i;
    ADF_INV_SBALL(x);
    ADF_INV_SBALL(y);
    if (x->arch != y->arch || x->len != y->len || !acb_equal(x->inf, y->inf))
        return 0;
    for (i = 0; i < x->len; i++)
        if (!adf_lball_identical(&x->loc[i], &y->loc[i]))
            return 0;
    return 1;
}

/* ---- constructors ---- */

typedef struct
{
    ulong p;
    slong i;
} keyed;

static int
keyed_cmp(const void * a, const void * b)
{
    const keyed * x = (const keyed *) a;
    const keyed * y = (const keyed *) b;
    return (x->p > y->p) - (x->p < y->p);
}

/* Statement S1 (sorting the components by prime is a permutation of the factors of a product of sets). */
int
adf_sball_set_arb_lballs(adf_sball_t y, adf_place_t * where, const arb_t r, const adf_lball_struct * loc, slong n)
{
    keyed * k;
    adf_sball_struct t;
    slong i;
    if (n < 0)
        return ADF_DOMAIN;
    if (r != NULL && !arb_is_finite(r))
    {
        report(where, adf_place_inf());
        return ADF_DOMAIN;
    }
    for (i = 0; i < n; i++)
        if (!adf_lball_is_canonical(&loc[i]))
        {
            adf_place_t v;
            if (adf_place_prime(&v, loc[i].p) == ADF_OK)
                report(where, v);
            return ADF_DOMAIN;
        }
    k = (keyed *) flint_malloc((size_t) (n > 0 ? n : 1) * sizeof(keyed));
    for (i = 0; i < n; i++)
    {
        k[i].p = loc[i].p;
        k[i].i = i;
    }
    qsort(k, (size_t) n, sizeof(keyed), keyed_cmp);
    for (i = 1; i < n; i++)
        if (k[i - 1].p == k[i].p)
        {
            report(where, place_of_prime(k[i].p));
            flint_free(k);
            return ADF_DOMAIN;
        }
    sb_alloc(&t, r != NULL ? ADF_ARCH_REAL : ADF_ARCH_NONE, n);
    if (r != NULL)
        arb_set(acb_realref(t.inf), r);
    for (i = 0; i < n; i++)
        adf_lball_set(&t.loc[i], &loc[k[i].i]);
    flint_free(k);
    sb_swap(y, &t);
    adf_sball_clear(&t);
    return ADF_OK;
}

static int
place_cmp_ptr(const void * a, const void * b)
{
    return adf_place_cmp(*(const adf_place_t *) a, *(const adf_place_t *) b);
}

/* Statement S1: the p-th coordinate of an element of x->fin ranges over adf_lball_set_fball (api-1f.md L1); the real
   coordinate over x->inf; the other coordinates are dropped (Proposition 22). */
int
adf_sball_project(adf_sball_t y, adf_place_t * where, const adf_adele_t x, const adf_place_t * places, slong n)
{
    adf_place_t * ps;
    adf_sball_struct t;
    slong i, first_prime, np;
    int has_inf;
    ADF_INV_ADELE_ARG(x);
    if (n < 0)
        return ADF_DOMAIN;
    ps = (adf_place_t *) flint_malloc((size_t) (n > 0 ? n : 1) * sizeof(adf_place_t));
    for (i = 0; i < n; i++)
        ps[i] = places[i];
    qsort(ps, (size_t) n, sizeof(adf_place_t), place_cmp_ptr);
    for (i = 1; i < n; i++)
        if (adf_place_equal(ps[i - 1], ps[i]))
        {
            report(where, ps[i]);
            flint_free(ps);
            return ADF_DOMAIN;
        }
    has_inf = n > 0 && adf_place_is_archimedean(ps[0]);
    first_prime = has_inf ? 1 : 0;
    np = n - first_prime;
    sb_alloc(&t, has_inf ? ADF_ARCH_REAL : ADF_ARCH_NONE, np);
    if (has_inf)
        arb_set(acb_realref(t.inf), x->inf);
    for (i = 0; i < np; i++)
    {
        int st = adf_lball_set_fball(&t.loc[i], ps[first_prime + i], &x->fin);
        if (st != ADF_OK)
        {
            report(where, ps[first_prime + i]);
            adf_sball_clear(&t);
            flint_free(ps);
            return st;
        }
    }
    flint_free(ps);
    sb_swap(y, &t);
    adf_sball_clear(&t);
    return ADF_OK;
}

/* ---- accessors ---- */

int
adf_sball_arch(const adf_sball_t x)
{
    ADF_INV_SBALL(x);
    return x->arch;
}

slong
adf_sball_num_places(const adf_sball_t x)
{
    ADF_INV_SBALL(x);
    return x->len + (x->arch != ADF_ARCH_NONE ? 1 : 0);
}

int
adf_sball_get_place(adf_place_t * v, const adf_sball_t x, slong i)
{
    slong n;
    ADF_INV_SBALL(x);
    n = x->len + (x->arch != ADF_ARCH_NONE ? 1 : 0);
    if (i < 0 || i >= n)
        return ADF_DOMAIN;
    if (x->arch != ADF_ARCH_NONE)
    {
        if (i == 0)
        {
            *v = adf_place_inf();
            return ADF_OK;
        }
        i--;
    }
    *v = place_of_prime(x->loc[i].p);
    return ADF_OK;
}

/* The index of the component at the prime p, or -1 (binary search: the primes are increasing). */
static slong
find_prime(const adf_sball_struct * x, ulong p)
{
    slong lo = 0, hi = x->len;
    while (lo < hi)
    {
        slong mid = lo + (hi - lo) / 2;
        if (x->loc[mid].p == p)
            return mid;
        if (x->loc[mid].p < p)
            lo = mid + 1;
        else
            hi = mid;
    }
    return -1;
}

int
adf_sball_has_place(const adf_sball_t x, adf_place_t v)
{
    ADF_INV_SBALL(x);
    if (adf_place_is_archimedean(v))
        return x->arch != ADF_ARCH_NONE;
    return find_prime(x, adf_place_prime_get(v)) >= 0;
}

int
adf_sball_get_lball(adf_lball_t c, const adf_sball_t x, adf_place_t v)
{
    slong k;
    ADF_INV_SBALL(x);
    if (adf_place_is_archimedean(v))
        return ADF_DOMAIN;
    k = find_prime(x, adf_place_prime_get(v));
    if (k < 0)
        return ADF_DOMAIN;
    adf_lball_set(c, &x->loc[k]);
    return ADF_OK;
}

int
adf_sball_get_arb(arb_t r, const adf_sball_t x, adf_place_t v)
{
    ADF_INV_SBALL(x);
    if (!adf_place_is_archimedean(v) || x->arch != ADF_ARCH_REAL)
        return ADF_DOMAIN;
    arb_set(r, acb_realref(x->inf));
    return ADF_OK;
}

/* ---- set predicates ---- */

/* 1 if the two have the same tag and the same primes. */
static int
same_places(const adf_sball_struct * x, const adf_sball_struct * y)
{
    slong i;
    if (x->arch != y->arch || x->len != y->len)
        return 0;
    for (i = 0; i < x->len; i++)
        if (x->loc[i].p != y->loc[i].p)
            return 0;
    return 1;
}

/* The archimedean components: a product of sets in a product of spaces meets or is inside exactly when the factors do
   (S4). arb_contains(a, b): b inside a (arb.rst:668); acb_contains likewise (acb.rst:302). */
int
adf_sball_equal_set(const adf_sball_t x, const adf_sball_t y)
{
    slong i;
    ADF_INV_SBALL(x);
    ADF_INV_SBALL(y);
    if (!same_places(x, y))
        return 0;
    if (x->arch == ADF_ARCH_REAL && !(arb_contains(acb_realref(x->inf), acb_realref(y->inf)) &&
                                      arb_contains(acb_realref(y->inf), acb_realref(x->inf))))
        return 0;
    if (x->arch == ADF_ARCH_COMPLEX && !(acb_contains(x->inf, y->inf) && acb_contains(y->inf, x->inf)))
        return 0;
    for (i = 0; i < x->len; i++)
        if (!adf_lball_equal_set(&x->loc[i], &y->loc[i]))
            return 0;
    return 1;
}

int
adf_sball_overlaps(const adf_sball_t x, const adf_sball_t y)
{
    slong i;
    ADF_INV_SBALL(x);
    ADF_INV_SBALL(y);
    if (!same_places(x, y))
        return 0;
    if (x->arch == ADF_ARCH_REAL && !arb_overlaps(acb_realref(x->inf), acb_realref(y->inf)))
        return 0;
    if (x->arch == ADF_ARCH_COMPLEX && !acb_overlaps(x->inf, y->inf))
        return 0;
    for (i = 0; i < x->len; i++)
        if (!adf_lball_overlaps(&x->loc[i], &y->loc[i]))
            return 0;
    return 1;
}

int
adf_sball_contains(const adf_sball_t x, const adf_sball_t y)
{
    slong i;
    ADF_INV_SBALL(x);
    ADF_INV_SBALL(y);
    if (!same_places(x, y))
        return 0;
    if (x->arch == ADF_ARCH_REAL && !arb_contains(acb_realref(y->inf), acb_realref(x->inf)))
        return 0;
    if (x->arch == ADF_ARCH_COMPLEX && !acb_contains(y->inf, x->inf))
        return 0;
    for (i = 0; i < x->len; i++)
        if (!adf_lball_contains(&x->loc[i], &y->loc[i]))
            return 0;
    return 1;
}

/* ---- componentwise operations ---- */

/* If the sets of places of x and y differ, writes the first place (canonical order) that is in one only and returns 1.
   A different tag is a different set at the archimedean place. */
static int
places_differ(adf_place_t * where, const adf_sball_struct * x, const adf_sball_struct * y)
{
    slong i = 0, j = 0;
    if (x->arch != y->arch)
    {
        report(where, adf_place_inf());
        return 1;
    }
    while (i < x->len && j < y->len)
    {
        if (x->loc[i].p == y->loc[j].p)
        {
            i++;
            j++;
        }
        else
        {
            report(where, place_of_prime(x->loc[i].p < y->loc[j].p ? x->loc[i].p : y->loc[j].p));
            return 1;
        }
    }
    if (i < x->len)
    {
        report(where, place_of_prime(x->loc[i].p));
        return 1;
    }
    if (j < y->len)
    {
        report(where, place_of_prime(y->loc[j].p));
        return 1;
    }
    return 0;
}

/* The combined status of a call (docs/conventions.md 3.3, from line 211): the maximum of the statuses of the
   places in the numeric order of 3.1; the reported place the first place, in the canonical order, whose status is that
   maximum. The places are visited in the canonical order (the archimedean place, then the primes increasing) and a
   status replaces the current one only if it is larger, so the first place of the maximum is kept. Finding R3: the
   complex tag (UNSUPPORTED at the archimedean place) does not end the inspection; a LIMIT (10) at a prime wins. */
static void
combine(int * st, adf_place_t * wh, int s, adf_place_t v)
{
    if (s > *st)
    {
        *st = s;
        *wh = v;
    }
}

int
adf_sball_neg(adf_sball_t y, adf_place_t * where, const adf_sball_t x)
{
    adf_sball_struct t;
    adf_place_t wh = adf_place_inf();
    slong i;
    int st = ADF_OK;
    ADF_INV_SBALL(x);
    if (x->arch == ADF_ARCH_COMPLEX)
        combine(&st, &wh, ADF_UNSUPPORTED, adf_place_inf());
    sb_alloc(&t, x->arch, x->len);
    if (x->arch == ADF_ARCH_REAL)
        arb_neg(acb_realref(t.inf), acb_realref(x->inf));
    for (i = 0; i < x->len; i++)
    {
        int si = adf_lball_neg(&t.loc[i], &x->loc[i]);
        if (si > st)
            combine(&st, &wh, si, place_of_prime(x->loc[i].p));
    }
    if (st != ADF_OK)
    {
        report(where, wh);
        adf_sball_clear(&t);
        return st;
    }
    sb_swap(y, &t);
    adf_sball_clear(&t);
    return ADF_OK;
}

typedef enum
{
    OP_ADD,
    OP_SUB,
    OP_MUL
} binop;

/* Statement S3: the set of results of a ring operation on products of sets is the product of the sets of results at
   each place; the components are the smallest balls at the primes (L2, L3, L5) and arb's enclosure at the real place.
   Order of the checks: prec above ADF_REAL_PREC_MAX (LIMIT, where = the archimedean place, from prec alone, before
   anything is allocated: finding R5), the places (DOMAIN: a precondition of the operation), then the statuses of the
   places combine by the rule above. */
static int
binary(adf_sball_t z, adf_place_t * where, const adf_sball_t x, const adf_sball_t y, slong prec, binop op)
{
    adf_sball_struct t;
    adf_place_t wh = adf_place_inf();
    slong i;
    int st = ADF_OK;
    if (prec > ADF_REAL_PREC_MAX)
    {
        report(where, adf_place_inf());
        return ADF_LIMIT;
    }
    if (places_differ(where, x, y))
        return ADF_DOMAIN;
    if (x->arch == ADF_ARCH_COMPLEX)
        combine(&st, &wh, ADF_UNSUPPORTED, adf_place_inf());
    if (prec < 2)
        prec = 2;
    sb_alloc(&t, x->arch, x->len);
    if (x->arch == ADF_ARCH_REAL)
    {
        if (op == OP_ADD)
            arb_add(acb_realref(t.inf), acb_realref(x->inf), acb_realref(y->inf), prec);
        else if (op == OP_SUB)
            arb_sub(acb_realref(t.inf), acb_realref(x->inf), acb_realref(y->inf), prec);
        else
            arb_mul(acb_realref(t.inf), acb_realref(x->inf), acb_realref(y->inf), prec);
    }
    for (i = 0; i < x->len; i++)
    {
        int si;
        if (op == OP_ADD)
            si = adf_lball_add(&t.loc[i], &x->loc[i], &y->loc[i]);
        else if (op == OP_SUB)
            si = adf_lball_sub(&t.loc[i], &x->loc[i], &y->loc[i]);
        else
            si = adf_lball_mul(&t.loc[i], &x->loc[i], &y->loc[i]);
        if (si > st)
            combine(&st, &wh, si, place_of_prime(x->loc[i].p));
    }
    if (st != ADF_OK)
    {
        report(where, wh);
        adf_sball_clear(&t);
        return st;
    }
    sb_swap(z, &t);
    adf_sball_clear(&t);
    return ADF_OK;
}

int
adf_sball_add(adf_sball_t z, adf_place_t * where, const adf_sball_t x, const adf_sball_t y, slong prec)
{
    if (prec > ADF_REAL_PREC_MAX)   /* first of all, before the entry check and any read of x or y (finding R5) */
    {
        report(where, adf_place_inf());
        return ADF_LIMIT;
    }
    ADF_INV_SBALL(x);   /* here and not in binary: the message names the public function */
    ADF_INV_SBALL(y);
    return binary(z, where, x, y, prec, OP_ADD);
}

int
adf_sball_sub(adf_sball_t z, adf_place_t * where, const adf_sball_t x, const adf_sball_t y, slong prec)
{
    if (prec > ADF_REAL_PREC_MAX)   /* first of all, before the entry check and any read of x or y (finding R5) */
    {
        report(where, adf_place_inf());
        return ADF_LIMIT;
    }
    ADF_INV_SBALL(x);   /* here and not in binary: the message names the public function */
    ADF_INV_SBALL(y);
    return binary(z, where, x, y, prec, OP_SUB);
}

int
adf_sball_mul(adf_sball_t z, adf_place_t * where, const adf_sball_t x, const adf_sball_t y, slong prec)
{
    if (prec > ADF_REAL_PREC_MAX)   /* first of all, before the entry check and any read of x or y (finding R5) */
    {
        report(where, adf_place_inf());
        return ADF_LIMIT;
    }
    ADF_INV_SBALL(x);   /* here and not in binary: the message names the public function */
    ADF_INV_SBALL(y);
    return binary(z, where, x, y, prec, OP_MUL);
}
