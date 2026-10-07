/* Slice 3.1-f: negation and addition of quotient classes, docs/api-3.md 2.4 (lines 224-260).
   Sets: Q3 (docs/api-3.md:575-589): pi(X)+pi(Y) = pi(X+Y), -pi(X) = pi(-X); both distribute over
   the stored entries (Q3 step 2). The finite sum is (a+b) + gcd(N,M) Zhat by
   docs/proofs/precision.md:25-30 (Proposition 1), gcd of rationals as defined at :9-11; this
   includes zero and fractional radii. Negation reverses the real end points (Q3 step 3).
   Each exact sum or negation is then reduced by algorithm R (docs/api-3.md 2.2; quotient.md
   P6:125-151, P8:177-198, P10:232-250) and rounded by Q1 (docs/api-3.md:516-544).
   The real end points are added EXACTLY before R: an early arb addition can change the raw
   number of crossings (docs/api-3.md:262-264). The count is R's construction count before
   rounding and deduplication (SPEC 15.4 N-D21, D3-2); LIMIT, never NEEDS_SPLIT.
   Copied, not shared: the bounded exact read/add helpers (src/qclass.c:180-235), Q1
   (src/qclass.c:239-272), the storage keys (src/qclass.c:62-80, 274-282) and the R loop
   (src/qclass.c:286-353), so that src/qclass.c stays untouched while lane q-slice4 edits it.
   The test checks add(x, 0) and neg(x) for a lift against adf_qclass_reduce bit for bit,
   so a drift of this copy from the original kernel is detected.
   FLINT: fmpq_gcd refs/src/flint-3.0.1/fmpq.rst:510-522; exact conversions arf.rst:310-315,
   :411-413; correctly rounded division arf.rst:24-35, :662-681; mag layout mag.rst:6-15. */
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

/* Exact order of stored end points; copy of src/qclass.c:59-80 (proof in its file header). */
static int qa_end_cmp(arb_srcptr x, arb_srcptr y, int upper)
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
/* Copy of src/qclass.c:178-235. R exact-work bounds. refs/src/flint-3.0.1/arf.rst:310-315 warns that conversion
   can allocate according to the exponent. Test both exponent and projected bit size first. */
static int qa_arf_bound(arf_srcptr a)
{
    slong e, b;
    if (arf_is_zero(a)) return 1;
    if (fmpz_cmp_si(ARF_EXPREF(a), -ADF_QCLASS_EXP_MAX) < 0 ||
        fmpz_cmp_si(ARF_EXPREF(a), ADF_QCLASS_EXP_MAX) > 0) return 0;
    e = fmpz_get_si(ARF_EXPREF(a)); b = arf_bits(a);
    return b <= ADF_QCLASS_BITS_MAX && FLINT_MAX(e, b) <= ADF_QCLASS_BITS_MAX &&
           (e >= b ? 1 : b-e+1) <= ADF_QCLASS_BITS_MAX;
}
static int qa_size(const fmpq_t a)
{
    return fmpz_bits(fmpq_numref(a)) <= ADF_QCLASS_BITS_MAX &&
           fmpz_bits(fmpq_denref(a)) <= ADF_QCLASS_BITS_MAX;
}
/* Conservative projected cross-products, including the carry bit, BEFORE arithmetic. */
static int qa_add(fmpq_t z, const fmpq_t x, const fmpq_t y, int subtract)
{
    flint_bitcnt_t xn = fmpz_bits(fmpq_numref(x)), xd = fmpz_bits(fmpq_denref(x));
    flint_bitcnt_t yn = fmpz_bits(fmpq_numref(y)), yd = fmpz_bits(fmpq_denref(y));
    if (fmpq_is_zero(y)) { if (!qa_size(x)) return 0; fmpq_set(z, x); return 1; }
    if (fmpq_is_zero(x)) {
        if (!qa_size(y)) return 0;
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
static int qa_read(fmpq_t lo, fmpq_t hi, adf_rat_t a, adf_rat_t N,
                  arb_srcptr inf, adf_fball_srcptr fin)
{
    arf_t r; fmpq_t m, rad; int ok = 0;
    arf_init(r); fmpq_init(m); fmpq_init(rad);
    arf_set_mag(r, arb_radref(inf));
    if (!qa_arf_bound(arb_midref(inf)) || !qa_arf_bound(r) ||
        fmpz_bits(fin->A) > ADF_QCLASS_BITS_MAX ||
        fmpz_bits(fin->H) > ADF_QCLASS_BITS_MAX ||
        fmpz_bits(fin->d) > ADF_QCLASS_BITS_MAX) goto done;
    arf_get_fmpq(m, arb_midref(inf)); arf_get_fmpq(rad, r);
    if (!qa_add(lo, m, rad, 1) || !qa_add(hi, m, rad, 0)) goto done;
    adf_fball_get_center(a, fin); adf_fball_get_radius(N, fin);
    ok = qa_size(a->q) && qa_size(N->q);
done:
    arf_clear(r); fmpq_clear(m); fmpq_clear(rad); return ok;
}
/* Copy of src/qclass.c:236-272.
   Q1, with review R1: divide exact rationals using correctly rounded arf (arf.rst:24-35,
   :662-681). Build the least RU30 and its successor directly in mag; mag.rst:6-15
   allows extra ulps in general conversions, so no general mag rounding call is used. */
static int qa_round(arb_t z, const fmpq_t lo, const fmpq_t hi, slong prec)
{
    fmpq_t mid, m, d, t; arf_t u, den; fmpz_t mant, exp; ulong v; int ok = 0;
    fmpq_init(mid); fmpq_init(m); fmpq_init(d); fmpq_init(t);
    arf_init(u); arf_init(den); fmpz_init(mant); fmpz_init(exp);
    if (!qa_add(mid, lo, hi, 0)) goto done;
    if (!fmpz_is_even(fmpq_numref(mid)) &&
        fmpz_bits(fmpq_denref(mid)) >= ADF_QCLASS_BITS_MAX) goto done;
    fmpq_div_2exp(mid, mid, 1);
    arf_set_fmpz(u, fmpq_numref(mid)); arf_set_fmpz(den, fmpq_denref(mid));
    arf_div(arb_midref(z), u, den, FLINT_MAX(prec, 2), ARF_RND_NEAR);
    if (!qa_arf_bound(arb_midref(z))) goto done;
    arf_get_fmpq(m, arb_midref(z));
    if (!qa_add(d, m, lo, 1) || !qa_add(t, hi, m, 1)) goto done;
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
/* Copy of src/qclass.c:273-282.
   Reduction outputs are global; equality of these keys is equality of stored set pieces. */
static int qa_piece_cmp(const void *vp, const void *vq)
{
    const adf_adele_struct *p = vp, *q = vq; int c;
    c = qa_end_cmp(p->inf, q->inf, 0);
    if (!c) c = qa_end_cmp(p->inf, q->inf, 1);
    if (!c) c = fmpz_cmp(p->fin.H, q->fin.H);
    if (!c) c = fmpz_cmp(p->fin.A, q->fin.A);
    return c;
}

/* Projected bound before the rational gcd: its numerator divides a numerator, its denominator
   divides the product of the denominators (fmpq.rst:510-517: gcd(ps,qr)/(qs), canonicalised). */
static int qa_gcd(fmpq_t g, const fmpq_t x, const fmpq_t y)
{
    if (fmpz_bits(fmpq_denref(x))+fmpz_bits(fmpq_denref(y)) > ADF_QCLASS_BITS_MAX) return 0;
    fmpq_gcd(g, x, y);
    return qa_size(g);
}

/* Exact source k of the construction, before any rounding. Negation (y == NULL):
   [lo,hi] x (a + N Zhat) -> [-hi,-lo] x (-a + N Zhat), Q3 step 3. Addition: pair k = i*len(y)+j,
   [lo1+lo2, hi1+hi2] x ((a1+a2) + gcd(N1,N2) Zhat), Q3 step 3 and precision.md:25-30. */
typedef struct { fmpq_t lo, hi, lo2, hi2; adf_rat_t a, N, a2, N2; } qa_source_t;
static int qa_source(qa_source_t *s, const adf_qclass_t x, const adf_qclass_t y, slong k)
{
    const adf_adele_struct *u;
    if (!y) {
        u = x->piece+k;
        if (!qa_read(s->lo2, s->hi2, s->a, s->N, u->inf, &u->fin)) return 0;
        fmpq_neg(s->lo, s->hi2); fmpq_neg(s->hi, s->lo2); fmpq_neg(s->a->q, s->a->q);
        return 1;
    }
    u = x->piece+k/y->len;
    if (!qa_read(s->lo, s->hi, s->a, s->N, u->inf, &u->fin)) return 0;
    u = y->piece+k%y->len;
    if (!qa_read(s->lo2, s->hi2, s->a2, s->N2, u->inf, &u->fin)) return 0;
    return qa_add(s->lo, s->lo, s->lo2, 0) && qa_add(s->hi, s->hi, s->hi2, 0) &&
           qa_add(s->a->q, s->a->q, s->a2->q, 0) && qa_gcd(s->N->q, s->N->q, s->N2->q);
}

/* Algorithm R on the exact sources (steps 2-5 as src/qclass.c:286-353), then Q1, sort, dedup.
   Two passes keep the count and every bound ahead of piece allocation; z is swapped only on OK.
   Each source yields at least one piece (B >= 1 fibres, each with ceil(h) > floor(l) or a point),
   so nsrc <= piece_limit is necessary and is checked by the callers without forming a product. */
static int qa_construct(adf_qclass_t z, const adf_qclass_t x, const adf_qclass_t y,
                        slong nsrc, slong piece_limit, slong prec)
{
    adf_qclass_struct out = {ADF_QCLASS_PIECES, 0, NULL};
    qa_source_t s; fmpq_t l, h, centre, one, nrat, left, right;
    fmpz_t first, stop, n, count, total, remaining, A, B, fin, unity;
    slong pass, i, j, fibres, pos = 0, keep; int status = ADF_LIMIT;
    fmpq_init(s.lo); fmpq_init(s.hi); fmpq_init(s.lo2); fmpq_init(s.hi2);
    adf_rat_init(s.a); adf_rat_init(s.N); adf_rat_init(s.a2); adf_rat_init(s.N2);
    fmpq_init(l); fmpq_init(h); fmpq_init(centre);
    fmpq_init(one); fmpq_one(one); fmpq_init(nrat); fmpq_init(left); fmpq_init(right);
    fmpz_init(first); fmpz_init(stop); fmpz_init(n); fmpz_init(count); fmpz_init(total);
    fmpz_init(remaining); fmpz_init(A); fmpz_init(B); fmpz_init(fin); fmpz_init_set_ui(unity, 1);
    for (pass = 0; pass < 2; pass++) {
        fmpz_zero(total);
        for (i = 0; i < nsrc; i++) {
            if (!qa_source(&s, x, y, i)) goto done;
            fmpz_set(A, fmpq_numref(s.N->q)); fmpz_set(B, fmpq_denref(s.N->q));
            fmpz_set_si(remaining, piece_limit); fmpz_sub(remaining, remaining, total);
            if (fmpz_cmp(B, remaining) > 0) goto done;
            fibres = fmpz_get_si(B); fmpq_set(centre, s.a->q);
            for (j = 0; j < fibres; j++) {
                if (!qa_add(l, s.lo, centre, 1) || !qa_add(h, s.hi, centre, 1)) goto done;
                fmpz_fdiv_q(first, fmpq_numref(l), fmpq_denref(l));
                if (fmpq_equal(l, h)) fmpz_add_ui(stop, first, 1);
                else fmpz_cdiv_q(stop, fmpq_numref(h), fmpq_denref(h));
                fmpz_sub(count, stop, first); fmpz_add(total, total, count);
                if (fmpz_cmp_si(total, piece_limit) > 0) goto done;
                if (pass) {
                    for (fmpz_set(n, first); fmpz_cmp(n, stop) < 0; fmpz_add_ui(n, n, 1)) {
                        fmpq_set_fmpz(nrat, n);
                        if (!qa_add(left, l, nrat, 1) || !qa_add(right, h, nrat, 1)) goto done;
                        if (fmpq_sgn(left) < 0) fmpq_zero(left);
                        if (fmpq_cmp(right, one) > 0) fmpq_one(right);
                        if (!qa_round(out.piece[pos].inf, left, right, prec)) goto done;
                        fmpz_neg(fin, n); if (!fmpz_is_zero(A)) fmpz_mod(fin, fin, A);
                        adf_fball_set_fmpz3(&out.piece[pos].fin, fin, A, unity); pos++;
                    }
                }
                if (j+1 < fibres && !qa_add(centre, centre, s.N->q, 0)) goto done;
            }
        }
        if (!pass) {
            out.len = fmpz_get_si(total);
            if ((size_t) out.len > SIZE_MAX/sizeof(*out.piece)) { out.len = 0; goto done; }
            out.piece = flint_malloc((size_t) out.len*sizeof(*out.piece));
            for (i = 0; i < out.len; i++) adf_adele_init(out.piece+i);
        }
    }
    qsort(out.piece, (size_t) out.len, sizeof(*out.piece), qa_piece_cmp);
    keep = 1;
    for (i = 1; i < out.len; i++) {
        if (qa_piece_cmp(out.piece+keep-1, out.piece+i)) {
            if (keep != i) adf_adele_swap(out.piece+keep, out.piece+i);
            keep++;
        }
    }
    for (i = keep; i < out.len; i++) adf_adele_clear(out.piece+i);
    out.len = keep; adf_qclass_swap(z, &out); status = ADF_OK;
done:
    if (out.piece) adf_qclass_clear(&out);
    fmpq_clear(s.lo); fmpq_clear(s.hi); fmpq_clear(s.lo2); fmpq_clear(s.hi2);
    adf_rat_clear(s.a); adf_rat_clear(s.N); adf_rat_clear(s.a2); adf_rat_clear(s.N2);
    fmpq_clear(l); fmpq_clear(h); fmpq_clear(centre);
    fmpq_clear(one); fmpq_clear(nrat); fmpq_clear(left); fmpq_clear(right);
    fmpz_clear(first); fmpz_clear(stop); fmpz_clear(n); fmpz_clear(count); fmpz_clear(total);
    fmpz_clear(remaining); fmpz_clear(A); fmpz_clear(B); fmpz_clear(fin); fmpz_clear(unity);
    return status;
}

/* -pi(X) = pi(-X) (Q3); R and Q1 on the exactly negated entries. Checks in reduce's order. */
int adf_qclass_neg(adf_qclass_t y, const adf_qclass_t x, slong piece_limit, slong prec)
{
    ADF_INV_QCLASS(x);
    if (prec > ADF_REAL_PREC_MAX || piece_limit < 1 || x->len > piece_limit) return ADF_LIMIT;
    return qa_construct(y, x, NULL, x->len, piece_limit, prec);
}

/* pi(X)+pi(Y) = pi(X+Y) (Q3), pairs of stored entries; the pair count len(x) len(y) is compared
   with piece_limit by division, before any product is formed. */
int adf_qclass_add(adf_qclass_t z, const adf_qclass_t x, const adf_qclass_t y,
                   slong piece_limit, slong prec)
{
    ADF_INV_QCLASS(x); ADF_INV_QCLASS(y);
    if (prec > ADF_REAL_PREC_MAX || piece_limit < 1 || x->len > piece_limit ||
        y->len > piece_limit/x->len) return ADF_LIMIT;
    return qa_construct(z, x, y, x->len*y->len, piece_limit, prec);
}
