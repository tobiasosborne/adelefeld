/* Exact Q2 set queries: docs/api-3.md:185-223,546-574; SPEC N-D21 (D3-1).
   Closed splitting and glue: docs/proofs/quotient.md:71-73,129-151,181-198,212-226.
   This file copies only the bounded exact-read/add kernel and R steps from src/qclass.c:184-234,286-348.
   It omits Q1 entirely. No stored real ball is rounded or clipped before diagonal translation.
   FLINT exact conversions/comparisons: refs/src/flint-3.0.1/arf.rst:310-315,411-413;
   fmpq.rst:153-161,400-410; fmpz.rst:838-851,1051-1055; memory.rst:9-25. */
#include <adelefeld.h>
#include "invariants.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef ADF_CHECK_INVARIANTS
#define ADF_INV_QCLASS(x) \
    do { if (!adf_qclass_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_qclass"); } while (0)
#else
#define ADF_INV_QCLASS(x) ((void) 0)
#endif

static int qs_arf_bound(arf_srcptr a)
{
    slong e, b;
    if (arf_is_zero(a)) return 1;
    if (fmpz_cmp_si(ARF_EXPREF(a), -ADF_QCLASS_EXP_MAX) < 0 ||
        fmpz_cmp_si(ARF_EXPREF(a), ADF_QCLASS_EXP_MAX) > 0) return 0;
    e = fmpz_get_si(ARF_EXPREF(a)); b = arf_bits(a);
    return b <= ADF_QCLASS_BITS_MAX && FLINT_MAX(e, b) <= ADF_QCLASS_BITS_MAX &&
           (e >= b ? 1 : b-e+1) <= ADF_QCLASS_BITS_MAX;
}
static int qs_size(const fmpq_t a)
{
    return fmpz_bits(fmpq_numref(a)) <= ADF_QCLASS_BITS_MAX &&
           fmpz_bits(fmpq_denref(a)) <= ADF_QCLASS_BITS_MAX;
}
static int qs_add(fmpq_t z, const fmpq_t x, const fmpq_t y, int subtract)
{
    flint_bitcnt_t xn = fmpz_bits(fmpq_numref(x)), xd = fmpz_bits(fmpq_denref(x));
    flint_bitcnt_t yn = fmpz_bits(fmpq_numref(y)), yd = fmpz_bits(fmpq_denref(y));
    if (fmpq_is_zero(y)) { if (!qs_size(x)) return 0; fmpq_set(z, x); return 1; }
    if (fmpq_is_zero(x)) {
        if (!qs_size(y)) return 0;
        if (subtract) fmpq_neg(z, y); else fmpq_set(z, y);
        return 1;
    }
    if (fmpz_equal(fmpq_denref(x), fmpq_denref(y))) {
        if (FLINT_MAX(xn, yn)+1 > ADF_QCLASS_BITS_MAX || xd > ADF_QCLASS_BITS_MAX) return 0;
    } else if (FLINT_MAX(xn+yd, yn+xd)+1 > ADF_QCLASS_BITS_MAX || xd+yd > ADF_QCLASS_BITS_MAX)
        return 0;
    if (subtract) fmpq_sub(z, x, y); else fmpq_add(z, x, y);
    return 1;
}
static int qs_read(fmpq_t lo, fmpq_t hi, adf_rat_t a, adf_rat_t N,
                   arb_srcptr inf, adf_fball_srcptr fin)
{
    arf_t r; fmpq_t m, rad; int ok = 0;
    arf_init(r); fmpq_init(m); fmpq_init(rad); arf_set_mag(r, arb_radref(inf));
    if (!qs_arf_bound(arb_midref(inf)) || !qs_arf_bound(r) ||
        fmpz_bits(fin->A) > ADF_QCLASS_BITS_MAX ||
        fmpz_bits(fin->H) > ADF_QCLASS_BITS_MAX || fmpz_bits(fin->d) > ADF_QCLASS_BITS_MAX) goto done;
    arf_get_fmpq(m, arb_midref(inf)); arf_get_fmpq(rad, r);
    if (!qs_add(lo, m, rad, 1) || !qs_add(hi, m, rad, 0)) goto done;
    adf_fball_get_center(a, fin); adf_fball_get_radius(N, fin);
    ok = qs_size(a->q) && qs_size(N->q);
done:
    arf_clear(r); fmpq_clear(m); fmpq_clear(rad); return ok;
}

/* Saturate in fmpz at limit+1, including LONG_MAX. No machine-word sum or product. */
static void qs_accumulate(fmpz_t total, const fmpz_t count, slong limit)
{
    fmpz_t cap; fmpz_init_set_si(cap, limit); fmpz_add_ui(cap, cap, 1);
    fmpz_add(total, total, count); if (fmpz_cmp(total, cap) > 0) fmpz_set(total, cap);
    fmpz_clear(cap);
}
typedef void (*qs_visit)(const fmpq_t, const fmpq_t, const fmpz_t, const fmpz_t, int, void *);

/* Exact R, in count-only or streaming construction mode. Both input families share the raw limit.
   B <= remaining is tested BEFORE copying a fibre or entering its loop (R5).
   A visitor uses only initialized rational temporaries; no piece array is needed by the preflight. */
static int qs_scan(const adf_qclass_t x, const adf_qclass_t y, slong limit,
                   fmpz_t total, fmpz_t L, qs_visit visit, void *ctx)
{
    adf_rat_t a, N; fmpq_t lo, hi, l, h, centre, nr, left, right;
    fmpz_t first, stop, n, count, remaining, A, B, m, g, t;
    int side, ok = 0; slong i, j, fibres;
    adf_rat_init(a); adf_rat_init(N); fmpq_init(lo); fmpq_init(hi); fmpq_init(l); fmpq_init(h);
    fmpq_init(centre); fmpq_init(nr); fmpq_init(left); fmpq_init(right);
    fmpz_init(first); fmpz_init(stop); fmpz_init(n); fmpz_init(count); fmpz_init(remaining);
    fmpz_init(A); fmpz_init(B); fmpz_init(m); fmpz_init(g); fmpz_init(t);
    fmpz_zero(total); fmpz_one(L);
    for (side = 0; side < 2; side++) {
        const adf_qclass_struct *q = side ? y : x;
        for (i = 0; i < q->len; i++) {
            if (!qs_read(lo, hi, a, N, q->piece[i].inf, &q->piece[i].fin)) goto done;
            fmpz_set(A, fmpq_numref(N->q)); fmpz_set(B, fmpq_denref(N->q));
            fmpz_set_si(remaining, limit); fmpz_sub(remaining, remaining, total);
            if (fmpz_cmp(B, remaining) > 0) goto done;
            if (!fmpz_is_zero(A)) {
                fmpz_gcd(g, L, A); fmpz_divexact(t, L, g);
                if (fmpz_bits(t)+fmpz_bits(A) > ADF_QCLASS_BITS_MAX) goto done;
                fmpz_mul(L, t, A); if (fmpz_cmp_si(L, limit) > 0) goto done;
            }
            fibres = fmpz_get_si(B); fmpq_set(centre, a->q);
            for (j = 0; j < fibres; j++) {
                if (!qs_add(l, lo, centre, 1) || !qs_add(h, hi, centre, 1)) goto done;
                fmpz_fdiv_q(first, fmpq_numref(l), fmpq_denref(l));
                if (fmpq_equal(l, h)) fmpz_add_ui(stop, first, 1);
                else fmpz_cdiv_q(stop, fmpq_numref(h), fmpq_denref(h));
                fmpz_sub(count, stop, first); qs_accumulate(total, count, limit);
                if (fmpz_cmp_si(total, limit) > 0) goto done;
                if (visit) for (fmpz_set(n, first); fmpz_cmp(n, stop) < 0; fmpz_add_ui(n, n, 1)) {
                    fmpq_set_fmpz(nr, n);
                    if (!qs_add(left, l, nr, 1) || !qs_add(right, h, nr, 1)) goto done;
                    if (fmpq_sgn(left) < 0) fmpq_zero(left);
                    if (fmpq_cmp_ui(right, 1) > 0) fmpq_one(right);
                    fmpz_neg(m, n); if (!fmpz_is_zero(A)) fmpz_mod(m, m, A);
                    if (fmpz_is_zero(A) && fmpq_is_one(right) && fmpz_sgn(m) < 0 &&
                        fmpz_bits(m)+1 > ADF_QCLASS_BITS_MAX) goto done;
                    visit(left, right, m, A, side, ctx);
                }
                if (j+1 < fibres && !qs_add(centre, centre, N->q, 0)) goto done;
            }
        }
    }
    ok = 1;
done:
    adf_rat_clear(a); adf_rat_clear(N); fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(l); fmpq_clear(h);
    fmpq_clear(centre); fmpq_clear(nr); fmpq_clear(left); fmpq_clear(right);
    fmpz_clear(first); fmpz_clear(stop); fmpz_clear(n); fmpz_clear(count); fmpz_clear(remaining);
    fmpz_clear(A); fmpz_clear(B); fmpz_clear(m); fmpz_clear(g); fmpz_clear(t); return ok;
}
typedef struct { fmpq_t previous, next; int have_previous, have_next; } qs_endpoint;
static void qs_next(const fmpq_t lo, const fmpq_t hi, const fmpz_t m,
                    const fmpz_t A, int side, void *ctx)
{
    qs_endpoint *e = ctx; int i; (void) m; (void) A; (void) side;
    for (i = 0; i < 2; i++) {
        const fmpq *t = i ? hi : lo;
        if ((!e->have_previous || fmpq_cmp(t, e->previous) > 0) &&
            (!e->have_next || fmpq_cmp(t, e->next) < 0)) {
            fmpq_set(e->next, t); e->have_next = 1;
        }
    }
}
typedef struct { fmpq_t lo, hi; fmpz_t m, A; int side; } qs_piece;
typedef struct { fmpz_t centre; slong piece; int glue; } qs_point;
static int qs_storage_status(slong K)
{
    if (K > (LONG_MAX-2)/2 || (size_t) K > SIZE_MAX/sizeof(qs_piece) ||
        (size_t) (2*K+2) > SIZE_MAX/sizeof(fmpq) || (size_t) (2*K) > SIZE_MAX/sizeof(qs_point))
        return ADF_LIMIT;
    return ADF_OK;
}
/* Discover E before ANY bulk allocation. Streaming minimum selection costs O(K E),
   within the O(K^2 L) total bound, and avoids an endpoint array before its budget is known. */
static int qs_preflight(slong *K, slong *modulus, const adf_qclass_t x,
                        const adf_qclass_t y, slong limit)
{
    fmpz_t raw, L, E, budget; qs_endpoint ep; int ok = 0;
    if (limit < 1 || x->len > limit || y->len > limit-x->len) return 0;
    fmpz_init(raw); fmpz_init(L); fmpz_init(E); fmpz_init(budget);
    fmpq_init(ep.previous); fmpq_init(ep.next); ep.have_previous = 0;
    if (!qs_scan(x, y, limit, raw, L, NULL, NULL)) goto done;
    /* An impossible byte product is refused before streaming through its potentially huge K. */
    if (qs_storage_status(fmpz_get_si(raw)) != ADF_OK) goto done;
    /* Every nonempty construction has E >= 1. This lower bound can refuse without discovering E. */
    fmpz_mul(budget, raw, L); fmpz_mul_ui(budget, budget, 3);
    if (fmpz_cmp_si(budget, limit) > 0) goto done;
    do {
        ep.have_next = 0;
        if (!qs_scan(x, y, limit, raw, L, qs_next, &ep)) goto done;
        if (ep.have_next) {
            fmpz_add_ui(E, E, 1); fmpq_set(ep.previous, ep.next); ep.have_previous = 1;
            fmpz_mul_ui(budget, E, 2); fmpz_add_ui(budget, budget, 1);
            fmpz_mul(budget, budget, raw); fmpz_mul(budget, budget, L);
            if (fmpz_cmp_si(budget, limit) > 0) goto done;
        }
    } while (ep.have_next);
    *K = fmpz_get_si(raw); *modulus = fmpz_get_si(L); ok = 1;
done:
    fmpz_clear(raw); fmpz_clear(L); fmpz_clear(E); fmpz_clear(budget);
    fmpq_clear(ep.previous); fmpq_clear(ep.next); return ok;
}
typedef struct { qs_piece *pieces; slong pos; } qs_output;
static void qs_store(const fmpq_t lo, const fmpq_t hi, const fmpz_t m,
                     const fmpz_t A, int side, void *ctx)
{
    qs_output *out = ctx; qs_piece *p = out->pieces+out->pos++;
    fmpq_set(p->lo, lo); fmpq_set(p->hi, hi); fmpz_set(p->m, m); fmpz_set(p->A, A); p->side = side;
}
static int qs_qcmp(const void *a, const void *b) { return fmpq_cmp(a, b); }
static int qs_point_cmp(const void *a, const void *b)
{
    const qs_point *p = a, *q = b; return fmpz_cmp(p->centre, q->centre);
}
typedef struct { unsigned char *residues; fmpz *points; slong len; } qs_fiber;
static void qs_center(qs_fiber *f, const fmpz_t m, const fmpz_t A, slong L)
{
    slong r = (slong) fmpz_fdiv_ui(m, (ulong) L), step = fmpz_get_si(A);
    r %= step;
    while (r < L) { f->residues[r] = 1; if (step >= L-r) break; r += step; }
}
/* Q2 steps 2 and 4: (1,m) contributes (0,m-1), including exact finite points. */
static void qs_fibers(qs_fiber f[2], const qs_piece *p, slong K, slong L, const fmpq_t s,
                      const qs_point *points, slong npoints)
{
    slong i, side; fmpz_t shifted; fmpz_init(shifted);
    for (side = 0; side < 2; side++) { memset(f[side].residues, 0, (size_t) L); f[side].len = 0; }
    for (i = 0; i < K; i++) {
        qs_fiber *g = f+p[i].side;
        if (fmpz_is_zero(p[i].A)) continue;
        if (fmpq_cmp(p[i].lo, s) <= 0 && fmpq_cmp(s, p[i].hi) <= 0)
            qs_center(g, p[i].m, p[i].A, L);
        if (fmpq_is_zero(s) && fmpq_is_one(p[i].hi)) {
            fmpz_sub_ui(shifted, p[i].m, 1); qs_center(g, shifted, p[i].A, L);
        }
    }
    /* Candidates are sorted once globally, so every cell needs only a linear point scan. */
    for (i = 0; i < npoints; i++) {
        const qs_piece *piece = p+points[i].piece; qs_fiber *g = f+piece->side;
        int active = points[i].glue ? fmpq_is_zero(s) :
            fmpq_cmp(piece->lo, s) <= 0 && fmpq_cmp(s, piece->hi) <= 0;
        if (!active || g->residues[fmpz_fdiv_ui(points[i].centre, (ulong) L)]) continue;
        if (g->len && fmpz_equal(g->points+g->len-1, points[i].centre)) continue;
        fmpz_set(g->points+g->len++, points[i].centre);
    }
    fmpz_clear(shifted);
}
/* Q2 step 5: positive cosets require residue inclusion; points cannot cover a missing coset. */
static int qs_inside(const qs_fiber *a, const qs_fiber *b, slong L)
{
    slong i, j = 0;
    for (i = 0; i < L; i++) if (a->residues[i] && !b->residues[i]) return 0;
    for (i = 0; i < a->len; i++) {
        if (b->residues[fmpz_fdiv_ui(a->points+i, (ulong) L)]) continue;
        while (j < b->len && fmpz_cmp(b->points+j, a->points+i) < 0) j++;
        if (j == b->len || !fmpz_equal(a->points+i, b->points+j)) return 0;
    }
    return 1;
}
static int qs_intersects(const qs_fiber *a, const qs_fiber *b, slong L)
{
    slong i, j;
    for (i = 0; i < L; i++) if (a->residues[i] && b->residues[i]) return 1;
    for (i = 0; i < a->len; i++) if (b->residues[fmpz_fdiv_ui(a->points+i, (ulong) L)]) return 1;
    for (i = 0; i < b->len; i++) if (a->residues[fmpz_fdiv_ui(b->points+i, (ulong) L)]) return 1;
    i = j = 0;
    while (i < a->len && j < b->len) {
        int c = fmpz_cmp(a->points+i, b->points+j);
        if (!c) return 1;
        if (c < 0) i++; else j++;
    }
    return 0;
}
static int qs_query(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong limit, int kind)
{
    slong K = 0, L = 0, i, side, nends, unique, npoints = 0;
    qs_output out; qs_fiber f[2]; fmpq *ends; qs_point *points;
    const fmpq *minimum, *maximum; int have_glue = 0;
    fmpz_t raw, modulus; fmpq_t s; int status = ADF_LIMIT, xy = 1, yx = 1, overlap = 0;
    ADF_INV_QCLASS(x); ADF_INV_QCLASS(y);
    if (!qs_preflight(&K, &L, x, y, limit)) return ADF_LIMIT;
    out.pieces = flint_malloc((size_t) K*sizeof(*out.pieces)); out.pos = 0;
    ends = flint_malloc((size_t) (2*K+2)*sizeof(*ends));
    points = flint_malloc((size_t) (2*K)*sizeof(*points));
    for (i = 0; i < 2*K; i++) fmpz_init(points[i].centre);
    for (i = 0; i < K; i++) {
        fmpq_init(out.pieces[i].lo); fmpq_init(out.pieces[i].hi);
        fmpz_init(out.pieces[i].m); fmpz_init(out.pieces[i].A);
    }
    for (i = 0; i < 2*K+2; i++) fmpq_init(ends+i);
    for (side = 0; side < 2; side++) {
        f[side].residues = flint_malloc((size_t) L);
        f[side].points = flint_malloc((size_t) (2*K)*sizeof(*f[side].points));
        for (i = 0; i < 2*K; i++) fmpz_init(f[side].points+i);
    }
    fmpz_init(raw); fmpz_init(modulus); fmpq_init(s);
    if (!qs_scan(x, y, limit, raw, modulus, qs_store, &out)) goto done;
    minimum = out.pieces[0].lo; maximum = out.pieces[0].hi;
    fmpq_zero(ends); fmpq_one(ends+1); nends = 2;
    for (i = 0; i < K; i++) {
        if (fmpq_cmp(out.pieces[i].lo, minimum) < 0) minimum = out.pieces[i].lo;
        if (fmpq_cmp(out.pieces[i].hi, maximum) > 0) maximum = out.pieces[i].hi;
        have_glue |= fmpq_is_one(out.pieces[i].hi);
        fmpq_set(ends+nends++, out.pieces[i].lo); fmpq_set(ends+nends++, out.pieces[i].hi);
        if (fmpz_is_zero(out.pieces[i].A)) {
            fmpz_set(points[npoints].centre, out.pieces[i].m);
            points[npoints].piece = i; points[npoints++].glue = 0;
            if (fmpq_is_one(out.pieces[i].hi)) {
                fmpz_sub_ui(points[npoints].centre, out.pieces[i].m, 1);
                points[npoints].piece = i; points[npoints++].glue = 1;
            }
        }
    }
    qsort(points, (size_t) npoints, sizeof(*points), qs_point_cmp);
    qsort(ends, (size_t) nends, sizeof(*ends), qs_qcmp); unique = 1;
    for (i = 1; i < nends; i++) if (!fmpq_equal(ends+unique-1, ends+i)) {
        if (unique != i) fmpq_swap(ends+unique, ends+i);
        unique++;
    }
    for (i = 0; i+1 < unique; i++) {
        int gap;
        for (gap = 0; gap < 2; gap++) {
            if (!gap) fmpq_set(s, ends+i);
            else {
                if (!qs_add(s, ends+i, ends+i+1, 0)) goto done;
                if (!fmpz_is_even(fmpq_numref(s)) &&
                    fmpz_bits(fmpq_denref(s)) >= ADF_QCLASS_BITS_MAX) goto done;
                fmpq_div_2exp(s, s, 1);
            }
            /* Exterior artificial cells are empty on both sides, except a glued real zero.
               Skipping them keeps the number of fibre tests within 2E+1 actual piece endpoints. */
            if ((fmpq_cmp(s, minimum) < 0 || fmpq_cmp(s, maximum) > 0) &&
                !(fmpq_is_zero(s) && have_glue)) continue;
            qs_fibers(f, out.pieces, K, L, s, points, npoints);
            xy &= qs_inside(f, f+1, L); yx &= qs_inside(f+1, f, L);
            overlap |= qs_intersects(f, f+1, L);
        }
    }
    *truth = kind == 0 ? xy && yx : (kind == 1 ? xy : overlap); status = ADF_OK;
done:
    fmpz_clear(raw); fmpz_clear(modulus); fmpq_clear(s);
    for (i = 0; i < K; i++) {
        fmpq_clear(out.pieces[i].lo); fmpq_clear(out.pieces[i].hi);
        fmpz_clear(out.pieces[i].m); fmpz_clear(out.pieces[i].A);
    }
    for (i = 0; i < 2*K+2; i++) fmpq_clear(ends+i);
    for (i = 0; i < 2*K; i++) fmpz_clear(points[i].centre);
    for (side = 0; side < 2; side++) {
        for (i = 0; i < 2*K; i++) fmpz_clear(f[side].points+i);
        flint_free(f[side].residues); flint_free(f[side].points);
    }
    flint_free(ends); flint_free(points); flint_free(out.pieces); return status;
}
int adf_qclass_equal_set(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong work_limit)
{
    return qs_query(truth, x, y, work_limit, 0);
}
int adf_qclass_contains(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong work_limit)
{
    return qs_query(truth, x, y, work_limit, 1);
}
int adf_qclass_overlaps(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong work_limit)
{
    return qs_query(truth, x, y, work_limit, 2);
}
