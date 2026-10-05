/* Slice 3.1-a. Set meaning and ownership: docs/api-3.md 1; conventions 5.10, 4.4.
   Quotient proofs: docs/proofs/quotient.md:232-250 (P10).
   FLINT operations used for exact ordering: refs/src/flint-3.0.1/arf.rst:18-38,
   :65-68 (round toward zero), :411-413 (exact radius conversion), :638-645 (sum).
   Two-bit rounding of a nonzero finite sum retains its sign: arf has unbounded exponents.
   Thus the sign of (m1 +/- r1) - (m2 +/- r2) is exact without forming huge endpoints. */
#include <adelefeld.h>
#include "invariants.h"
#include <stdint.h>
#include <stdlib.h>

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

/* R exact-work bounds. refs/src/flint-3.0.1/arf.rst:310-315 warns that conversion
   can allocate according to the exponent. Test both exponent and projected bit size first. */
static int q_arf_bound(arf_srcptr a)
{
    slong e, b;
    if (arf_is_zero(a)) return 1;
    if (fmpz_cmp_si(ARF_EXPREF(a), -ADF_QCLASS_EXP_MAX) < 0 ||
        fmpz_cmp_si(ARF_EXPREF(a), ADF_QCLASS_EXP_MAX) > 0) return 0;
    e = fmpz_get_si(ARF_EXPREF(a)); b = arf_bits(a);
    return b <= ADF_QCLASS_BITS_MAX && FLINT_MAX(e, b) <= ADF_QCLASS_BITS_MAX &&
           (e >= b ? 1 : b-e+0) <= ADF_QCLASS_BITS_MAX;
}
static int q_size(const fmpq_t a)
{
    return fmpz_bits(fmpq_numref(a)) <= ADF_QCLASS_BITS_MAX &&
           fmpz_bits(fmpq_denref(a)) <= ADF_QCLASS_BITS_MAX;
}
/* Conservative projected cross-products, including the carry bit, BEFORE arithmetic. */
static int q_add(fmpq_t z, const fmpq_t x, const fmpq_t y, int subtract)
{
    flint_bitcnt_t xn = fmpz_bits(fmpq_numref(x)), xd = fmpz_bits(fmpq_denref(x));
    flint_bitcnt_t yn = fmpz_bits(fmpq_numref(y)), yd = fmpz_bits(fmpq_denref(y));
    if (fmpq_is_zero(y)) { if (!q_size(x)) return 0; fmpq_set(z, x); return 1; }
    if (fmpq_is_zero(x)) {
        if (!q_size(y)) return 0;
        if (subtract) fmpq_neg(z, y); else fmpq_set(z, y);
        return 1;
    }
    /* Equal denominators need no cross-product. In particular the exponent boundary
       must not be refused solely for an unnecessary squared dyadic denominator. */
    if (fmpz_equal(fmpq_denref(x), fmpq_denref(y))) {
        if (FLINT_MAX(xn, yn)+1 > ADF_QCLASS_BITS_MAX || xd > ADF_QCLASS_BITS_MAX) return 0;
        if (subtract) fmpq_sub(z, x, y); else fmpq_add(z, x, y);
        return 1;
    }
    if (FLINT_MAX(xn+yd, yn+xd)+1 > ADF_QCLASS_BITS_MAX || xd+yd > ADF_QCLASS_BITS_MAX)
        return 0;
    if (subtract) fmpq_sub(z, x, y); else fmpq_add(z, x, y);
    return 1;
}
/* Read endpoints without clipping. Local raw H bounds the CRT numerator before reconstruction. */
static int q_read(fmpq_t lo, fmpq_t hi, adf_rat_t a, adf_rat_t N,
                  arb_srcptr inf, adf_fball_srcptr fin)
{
    arf_t r; fmpq_t m, rad; int ok = 0;
    arf_init(r); fmpq_init(m); fmpq_init(rad);
    arf_set_mag(r, arb_radref(inf));
    if (!q_arf_bound(arb_midref(inf)) || !q_arf_bound(r) ||
        fmpz_bits(fin->A) > ADF_QCLASS_BITS_MAX ||
        fmpz_bits(fin->H) > ADF_QCLASS_BITS_MAX ||
        fmpz_bits(fin->d) > ADF_QCLASS_BITS_MAX) goto done;
    arf_get_fmpq(m, arb_midref(inf)); arf_get_fmpq(rad, r);
    if (!q_add(lo, m, rad, 1) || !q_add(hi, m, rad, 0)) goto done;
    adf_fball_get_center(a, fin); adf_fball_get_radius(N, fin);
    ok = q_size(a->q) && q_size(N->q);
done:
    arf_clear(r); fmpq_clear(m); fmpq_clear(rad); return ok;
}
/* Q1, with review R1: divide exact rationals using correctly rounded arf (arf.rst:24-35,
   :662-681). Build the least RU30 and its successor directly in mag; mag.rst:6-15
   allows extra ulps in general conversions, so no general mag rounding call is used. */
static int q_round(arb_t z, const fmpq_t lo, const fmpq_t hi, slong prec)
{
    fmpq_t mid, m, d, t; arf_t u, den; fmpz_t mant, exp; ulong v; int ok = 0;
    fmpq_init(mid); fmpq_init(m); fmpq_init(d); fmpq_init(t);
    arf_init(u); arf_init(den); fmpz_init(mant); fmpz_init(exp);
    if (!q_add(mid, lo, hi, 0)) goto done;
    if (!fmpz_is_even(fmpq_numref(mid)) &&
        fmpz_bits(fmpq_denref(mid)) >= ADF_QCLASS_BITS_MAX) goto done;
    fmpq_div_2exp(mid, mid, 1);
    arf_set_fmpz(u, fmpq_numref(mid)); arf_set_fmpz(den, fmpq_denref(mid));
    arf_div(arb_midref(z), u, den, FLINT_MAX(prec, 2), ARF_RND_NEAR);
    if (!q_arf_bound(arb_midref(z))) goto done;
    arf_get_fmpq(m, arb_midref(z));
    if (!q_add(d, m, lo, 1) || !q_add(t, hi, m, 1)) goto done;
    if (fmpq_cmp(t, d) > 0) fmpq_set(d, t);
    if (fmpq_is_zero(d)) mag_zero(arb_radref(z));
    else {
        arf_set_fmpz(u, fmpq_numref(d)); arf_set_fmpz(den, fmpq_denref(d));
        arf_div(u, u, den, 30, ARF_RND_CEIL);
        /* u = v * 2^(E-30), 2^29 <= v < 2^30. Normalise a shortened mantissa. */
        arf_get_fmpz_2exp(mant, exp, u);
        fmpz_mul_2exp(mant, mant, 30-fmpz_bits(mant));
        v = fmpz_get_ui(mant)+1;
        fmpz_set(MAG_EXPREF(arb_radref(z)), ARF_EXPREF(u));
        if (v == (UWORD(1) << 30)) {
            v >>= 1; fmpz_add_ui(MAG_EXPREF(arb_radref(z)), MAG_EXPREF(arb_radref(z)), 1);
        }
        MAG_MAN(arb_radref(z)) = v;
    }
    ok = 1;
done:
    fmpq_clear(mid); fmpq_clear(m); fmpq_clear(d); fmpq_clear(t);
    arf_clear(u); arf_clear(den); fmpz_clear(mant); fmpz_clear(exp); return ok;
}
/* Reduction outputs are global; equality of these keys is equality of stored set pieces. */
static int q_piece_cmp(const void *vp, const void *vq)
{
    const adf_adele_struct *p = vp, *q = vq; int c;
    c = end_cmp(p->inf, q->inf, 0);
    if (!c) c = end_cmp(p->inf, q->inf, 1);
    if (!c) c = fmpz_cmp(p->fin.H, q->fin.H);
    if (!c) c = fmpz_cmp(p->fin.A, q->fin.A);
    return c;
}
/* Algorithm R: exact translation precedes Q1 (P10:242-250), including finite points.
   P6:129-151 fixes closed endpoint counts; P8:181-198 gives the fractional fibres.
   Two passes keep count and all bounds ahead of piece allocation; y is swapped only on OK. */
int adf_qclass_reduce(adf_qclass_t y, const adf_qclass_t x, slong piece_limit, slong prec)
{
    adf_qclass_struct out = {ADF_QCLASS_PIECES, 0, NULL};
    adf_rat_t a, N; fmpq_t lo, hi, l, h, centre, one, nrat, left, right;
    fmpz_t first, stop, n, count, total, remaining, A, B, fin, unity;
    slong pass, i, j, fibres, pos = 0, keep; int status = ADF_LIMIT;
    ADF_INV_QCLASS(x);
    if (prec > ADF_REAL_PREC_MAX || piece_limit < 1 || x->len > piece_limit) return ADF_LIMIT;
    adf_rat_init(a); adf_rat_init(N);
    fmpq_init(lo); fmpq_init(hi); fmpq_init(l); fmpq_init(h); fmpq_init(centre);
    fmpq_init(one); fmpq_one(one); fmpq_init(nrat); fmpq_init(left); fmpq_init(right);
    fmpz_init(first); fmpz_init(stop); fmpz_init(n); fmpz_init(count); fmpz_init(total);
    fmpz_init(remaining); fmpz_init(A); fmpz_init(B); fmpz_init(fin); fmpz_init_set_ui(unity, 1);
    for (pass = 0; pass < 2; pass++) {
        fmpz_zero(total);
        for (i = 0; i < x->len; i++) {
            if (!q_read(lo, hi, a, N, x->piece[i].inf, &x->piece[i].fin)) goto done;
            fmpz_set(A, fmpq_numref(N->q)); fmpz_set(B, fmpq_denref(N->q));
            fmpz_set_si(remaining, piece_limit); fmpz_sub(remaining, remaining, total);
            if (fmpz_cmp(B, remaining) > 0) goto done;
            fibres = fmpz_get_si(B); fmpq_set(centre, a->q);
            for (j = 0; j < fibres; j++) {
                if (!q_add(l, lo, centre, 1) || !q_add(h, hi, centre, 1)) goto done;
                fmpz_fdiv_q(first, fmpq_numref(l), fmpq_denref(l));
                if (fmpq_equal(l, h)) fmpz_add_ui(stop, first, 1);
                else fmpz_cdiv_q(stop, fmpq_numref(h), fmpq_denref(h));
                fmpz_sub(count, stop, first); fmpz_add(total, total, count);
                if (fmpz_cmp_si(total, piece_limit) > 0) goto done;
                if (pass) {
                    for (fmpz_set(n, first); fmpz_cmp(n, stop) < 0; fmpz_add_ui(n, n, 1)) {
                        fmpq_set_fmpz(nrat, n);
                        if (!q_add(left, l, nrat, 1) || !q_add(right, h, nrat, 1)) goto done;
                        if (fmpq_sgn(left) < 0) fmpq_zero(left);
                        if (fmpq_cmp(right, one) > 0) fmpq_one(right);
                        if (!q_round(out.piece[pos].inf, left, right, prec)) goto done;
                        fmpz_neg(fin, n); if (!fmpz_is_zero(A)) fmpz_mod(fin, fin, A);
                        adf_fball_set_fmpz3(&out.piece[pos].fin, fin, A, unity); pos++;
                    }
                }
                if (j+1 < fibres && !q_add(centre, centre, N->q, 0)) goto done;
            }
        }
        if (!pass) {
            out.len = fmpz_get_si(total);
            if ((size_t) out.len > SIZE_MAX/sizeof(*out.piece)) { out.len = 0; goto done; }
            out.piece = flint_malloc((size_t) out.len*sizeof(*out.piece));
            for (i = 0; i < out.len; i++) adf_adele_init(out.piece+i);
        }
    }
    qsort(out.piece, (size_t) out.len, sizeof(*out.piece), q_piece_cmp);
    keep = 1;
    for (i = 1; i < out.len; i++) {
        if (q_piece_cmp(out.piece+keep-1, out.piece+i)) {
            if (keep != i) adf_adele_swap(out.piece+keep, out.piece+i);
            keep++;
        }
    }
    for (i = keep; i < out.len; i++) adf_adele_clear(out.piece+i);
    out.len = keep; adf_qclass_swap(y, &out); status = ADF_OK;
done:
    if (out.piece) adf_qclass_clear(&out);
    adf_rat_clear(a); adf_rat_clear(N);
    fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(l); fmpq_clear(h); fmpq_clear(centre);
    fmpq_clear(one); fmpq_clear(nrat); fmpq_clear(left); fmpq_clear(right);
    fmpz_clear(first); fmpz_clear(stop); fmpz_clear(n); fmpz_clear(count); fmpz_clear(total);
    fmpz_clear(remaining); fmpz_clear(A); fmpz_clear(B); fmpz_clear(fin); fmpz_clear(unity);
    return status;
}
