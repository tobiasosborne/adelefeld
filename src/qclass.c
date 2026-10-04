/* Slice 3.1-a. Set meaning and ownership: docs/api-3.md 1; conventions 5.10, 4.4.
   Quotient proofs: docs/proofs/quotient.md:232-250 (P10).
   FLINT operations used for exact ordering: refs/src/flint-3.0.1/arf.rst:18-38,
   :65-68 (round toward zero), :411-413 (exact radius conversion), :638-645 (sum).
   Two-bit rounding of a nonzero finite sum retains its sign: arf has unbounded exponents.
   Thus the sign of (m1 +/- r1) - (m2 +/- r2) is exact without forming huge endpoints. */
#include <adelefeld.h>
#include "invariants.h"

#ifdef ADF_CHECK_INVARIANTS
#define ADF_INV_QCLASS(x) \
    do { if (!adf_qclass_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_qclass"); } while (0)
#else
#define ADF_INV_QCLASS(x) ((void) 0)
#endif

/* conventions 5.10: init is the LIFT zero, with owned initialized storage. */
void adf_qclass_init(adf_qclass_t x)
{
    x->form = ADF_QCLASS_LIFT;
    x->len = 1;
    x->piece = flint_malloc(sizeof(*x->piece));
    adf_adele_init(x->piece);
}

void adf_qclass_clear(adf_qclass_t x)
{
    slong i;
    for (i = 0; i < x->len; i++) adf_adele_clear(x->piece + i);
    flint_free(x->piece);
}

/* conventions 4.1: deep ownership; the adele copy retains its borrowed context. */
void adf_qclass_set(adf_qclass_t y, const adf_qclass_t x)
{
    adf_qclass_struct t;
    slong i;
    ADF_INV_QCLASS(x);
    if (x == y) return;
    t.form = x->form;
    t.len = x->len;
    t.piece = flint_malloc((size_t) t.len * sizeof(*t.piece));
    for (i = 0; i < t.len; i++) {
        adf_adele_init(t.piece + i);
        adf_adele_set(t.piece + i, x->piece + i);
    }
    adf_qclass_swap(y, &t);
    adf_qclass_clear(&t);
}

void adf_qclass_swap(adf_qclass_t x, adf_qclass_t y)
{
    adf_qclass_struct t = *x;
    *x = *y;
    *y = t;
}

/* Compare exact lower (upper=0) or upper (upper=1) ends; proof in the file header.
   refs/src/flint-3.0.1/arf.rst:638: arf_sum rounds the exact sum once. */
static int end_cmp(arb_srcptr x, arb_srcptr y, int upper)
{
    arf_struct terms[4];
    arf_t sum;
    int i, c;
    for (i = 0; i < 4; i++) arf_init(terms + i);
    arf_init(sum);
    arf_set(terms, arb_midref(x));
    arf_neg(terms + 1, arb_midref(y));
    arf_set_mag(terms + 2, arb_radref(x));
    arf_set_mag(terms + 3, arb_radref(y));
    if (!upper) arf_neg(terms + 2, terms + 2);
    if (upper) arf_neg(terms + 3, terms + 3);
    arf_sum(sum, terms, 4, 2, ARF_RND_DOWN);
    c = arf_sgn(sum);
    arf_clear(sum);
    for (i = 0; i < 4; i++) arf_clear(terms + i);
    return c;
}

/* Predicate exception to CV-09: validate each member BEFORE using its checked accessors.
   docs/api-3.md 1 and conventions 5.10: keys use the canonical global triple, even for local storage. */
int adf_qclass_is_canonical(const adf_qclass_t x)
{
    fmpz_t A, H, d, prevA, prevH;
    slong i;
    int ok = 1;
    if ((x->form != ADF_QCLASS_LIFT && x->form != ADF_QCLASS_PIECES) || x->len < 1 || !x->piece)
        return 0;
    if (x->form == ADF_QCLASS_LIFT)
        return x->len == 1 && adf_adele_is_canonical(x->piece);
    fmpz_init(A); fmpz_init(H); fmpz_init(d); fmpz_init(prevA); fmpz_init(prevH);
    for (i = 0; i < x->len; i++) {
        const adf_adele_struct *a = x->piece + i;
        int c;
        if (!adf_adele_is_canonical(a) || arf_sgn(arb_midref(a->inf)) < 0 ||
            arf_cmp_ui(arb_midref(a->inf), 1) > 0) { ok = 0; break; }
        adf_fball_get_fmpz3(A, H, d, &a->fin);
        if (!fmpz_is_one(d) || fmpz_sgn(H) < 0 ||
            (fmpz_sgn(H) > 0 && (fmpz_sgn(A) < 0 || fmpz_cmp(A, H) >= 0))) { ok = 0; break; }
        if (i > 0) {
            c = end_cmp(x->piece[i-1].inf, a->inf, 0);
            if (!c) c = end_cmp(x->piece[i-1].inf, a->inf, 1);
            if (!c) c = fmpz_cmp(prevH, H);
            if (!c) c = fmpz_cmp(prevA, A);
            if (c >= 0) { ok = 0; break; }
        }
        fmpz_set(prevA, A); fmpz_set(prevH, H);
    }
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); fmpz_clear(prevA); fmpz_clear(prevH);
    return ok;
}

/* CV-02: identity of storage, rather than equality of image sets. */
int adf_qclass_identical(const adf_qclass_t x, const adf_qclass_t y)
{
    slong i;
    ADF_INV_QCLASS(x); ADF_INV_QCLASS(y);
    if (x->form != y->form || x->len != y->len) return 0;
    for (i = 0; i < x->len; i++)
        if (!adf_adele_identical(x->piece + i, y->piece + i)) return 0;
    return 1;
}

/* quotient.md P10.1:236: the lift copies the exact stored set before projection. */
void adf_qclass_set_adele(adf_qclass_t y, const adf_adele_t x)
{
    adf_qclass_t t;
    ADF_INV_ADELE(x);
    adf_qclass_init(t);
    adf_adele_set(t->piece, x);
    adf_qclass_swap(y, t);
    adf_qclass_clear(t);
}

/* quotient.md P10.1:236: pi(q) = pi(0); no conversion to arb. */
void adf_qclass_set_rat(adf_qclass_t y, const adf_rat_t q)
{
    adf_qclass_t t;
    ADF_INV_RAT(q);
    (void) q;
    adf_qclass_init(t);
    adf_qclass_swap(y, t);
    adf_qclass_clear(t);
}

/* docs/api-3.md 2.1: these queries count stored representatives, not quotient fibers. */
int adf_qclass_form(const adf_qclass_t x)
{
    ADF_INV_QCLASS(x);
    return x->form;
}

slong adf_qclass_length(const adf_qclass_t x)
{
    ADF_INV_QCLASS(x);
    return x->len;
}

/* CV-06: check index before writing the initialized destination. */
int adf_qclass_get_piece(adf_adele_t a, const adf_qclass_t x, slong i)
{
    ADF_INV_QCLASS(x);
    if (i < 0 || i >= x->len) return ADF_DOMAIN;
    adf_adele_set(a, x->piece + i);
    return ADF_OK;
}

/* P10.1, docs/api-3.md 2.4: identity on stored representations avoids P10.3 widening. */
void adf_qclass_add_rat(adf_qclass_t y, const adf_qclass_t x, const adf_rat_t q)
{
    ADF_INV_QCLASS(x); ADF_INV_RAT(q);
    (void) q;
    adf_qclass_set(y, x);
}
