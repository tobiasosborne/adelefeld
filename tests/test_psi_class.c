/* Slice 3.2-b (lane q-slice5): the character on classes, docs/api-3.md 3.2:370-389, D3-3:809-817, Q4.
   Oracle: tests/ref/vectors/q-slice5/class.jsonl from lanes/q-slice5/gen_vectors.py, which uses
   proto/quotient3_checks.py psi, phase_arcs, nearest_distance, reduce and round_piece.
   Sign (conventions 6.1:844; refs/src/tate-poonen/notes.txt:693-700): psi(m ; a) = E(a - m); the class
   character is constant on classes (analysis Lemma 2 step 4:92-94), so a stored entry contributes the
   image of its adele, and the class image is the union over the stored entries.
   Hull bound asserted (api-3.md 3.3): every stored endpoint lies outside the exact hull of the UNION of arcs
   and exceeds it by at most 4*2^-p + 2^-28*(W/2 + 2*2^-p), W the true coordinate width of that hull.
   No same-type aliasing exists for these signatures (output acb or fmpq, input qclass). */
#include <adelefeld.h>
#include "support/jsonl.h"
#include "support/golden.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

static unsigned long checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

static void qread(fmpq_t q, const char *s)
{ CHECK(fmpq_set_str(q, s, 10) == 0); fmpq_canonicalise(q); }
static void mod1(fmpq_t q)
{ fmpz_mod(fmpq_numref(q), fmpq_numref(q), fmpq_denref(q)); fmpq_canonicalise(q); }
static const jsonl_value *field(const jsonl_value *v, const char *key)
{
    const jsonl_value *out; jsonl_error_t e;
    CHECK(jsonl_field(v, key, &out, &e)); return out;
}
static const jsonl_value *at(const jsonl_value *v, size_t i)
{ jsonl_error_t e; const jsonl_value *w = jsonl_at(v, i, &e); CHECK(w != NULL); return w; }
static void jq(fmpq_t q, const jsonl_value *v)
{
    jsonl_error_t e; const char *s = jsonl_string(v, NULL, &e);
    CHECK(s != NULL); qread(q, s);
}
static slong jint(const jsonl_value *v)
{ jsonl_error_t e; const char *s = jsonl_int_text(v, &e); CHECK(s != NULL); return atol(s); }
/* (m +/- r ; a + N Zhat) with exact dyadic m and r; r = k/2^j, k < 2^30. */
static void setball(adf_adele_t x, const fmpq_t m, const fmpq_t r, const fmpq_t a, const fmpq_t N)
{
    adf_rat_t c, n;
    adf_rat_init(c); adf_rat_init(n);
    fmpq_set(c->q, a); fmpq_set(n->q, N);
    CHECK(adf_fball_set_center_radius(&x->fin, c, n) == ADF_OK);
    arb_set_fmpq(x->inf, m, FLINT_MAX(4096, fmpz_bits(fmpq_numref(m)) + 64));
    CHECK(arb_is_exact(x->inf));
    CHECK(fmpz_bits(fmpq_numref(r)) <= 30);
    mag_set_ui_2exp_si(arb_radref(x->inf), fmpz_get_ui(fmpq_numref(r)), 1-fmpz_bits(fmpq_denref(r)));
    CHECK(adf_adele_is_canonical(x));
    adf_rat_clear(c); adf_rat_clear(n);
}
static void entry_read(adf_adele_t x, const jsonl_value *e)
{
    fmpq_t m, r, a, N; arf_t t; fmpq_t back;
    fmpq_init(m); fmpq_init(r); fmpq_init(a); fmpq_init(N); arf_init(t); fmpq_init(back);
    jq(m, at(e, 0)); jq(r, at(e, 1)); jq(a, at(e, 2)); jq(N, at(e, 3));
    setball(x, m, r, a, N);
    arf_set_mag(t, arb_radref(x->inf)); arf_get_fmpq(back, t); CHECK(fmpq_equal(back, r));
    fmpq_clear(m); fmpq_clear(r); fmpq_clear(a); fmpq_clear(N); arf_clear(t); fmpq_clear(back);
}
/* A PIECES class built in place: the struct layout is public (qclass.h, conventions 5.10); the class
   owns a flint_malloc array of initialized adeles, which adf_qclass_clear releases. */
static void pieces_build(adf_qclass_t q, const jsonl_value *entries)
{
    slong n = (slong) jsonl_size(entries);
    adf_qclass_clear(q);
    q->form = ADF_QCLASS_PIECES; q->len = n;
    q->piece = flint_malloc((size_t) n * sizeof(adf_adele_struct));
    for (slong i = 0; i < n; i++) { adf_adele_init(q->piece + i); entry_read(q->piece + i, at(entries, i)); }
    CHECK(adf_qclass_is_canonical(q));
}
static void pieces_from(adf_qclass_t q, adf_adele_struct *xs, slong n)
{
    adf_qclass_clear(q);
    q->form = ADF_QCLASS_PIECES; q->len = n;
    q->piece = flint_malloc((size_t) n * sizeof(adf_adele_struct));
    for (slong i = 0; i < n; i++) { adf_adele_init(q->piece + i); adf_adele_set(q->piece + i, xs + i); }
}
static void seed_phase(fmpq_t q)
{ fmpq_set_si(q, 19, 7); fmpq_mul_2exp(q, q, 130); }
static void seed_complex(acb_t z)
{
    acb_set_si(z, 17);
    arf_mul_2exp_si(arb_midref(acb_realref(z)), arb_midref(acb_realref(z)), 130);
    arf_add_ui(arb_midref(acb_realref(z)), arb_midref(acb_realref(z)), 1, ARF_PREC_EXACT, ARF_RND_NEAR);
    arb_set_si(acb_imagref(z), -23); mag_set_ui_2exp_si(arb_radref(acb_realref(z)), 1, -4);
}
/* Every call through these wrappers: seeded sentinel, untouched bytes and value on failure, the input
   unchanged (representation identity) on every status. */
static int qeval(acb_t z, const adf_qclass_t x, slong p, int strict)
{
    adf_qclass_t copy; acb_t saved; acb_struct bytes; int st;
    adf_qclass_init(copy); adf_qclass_set(copy, x);
    seed_complex(z); acb_init(saved); acb_set(saved, z); memcpy(&bytes, z, sizeof(bytes));
    st = strict ? adf_qclass_psi_tate_strict(z, x, p) : adf_qclass_psi_tate(z, x, p);
    if (st) { CHECK(!memcmp(&bytes, z, sizeof(bytes))); CHECK(acb_equal(z, saved)); }
    else CHECK(acb_is_finite(z));
    CHECK(adf_qclass_identical(copy, x));
    acb_clear(saved); adf_qclass_clear(copy);
    return st;
}
static int qphase(fmpq_t q, const adf_qclass_t x)
{
    fmpq_t saved; fmpq qbytes; int st;
    seed_phase(q); fmpq_init(saved); fmpq_set(saved, q); memcpy(&qbytes, q, sizeof(qbytes));
    st = adf_qclass_psi_tate_phase(q, x);
    if (st) { CHECK(!memcmp(&qbytes, q, sizeof(qbytes))); CHECK(fmpq_equal(q, saved)); }
    else CHECK(fmpq_is_canonical(q) && fmpq_sgn(q) >= 0 && fmpq_cmp_si(q, 1) < 0);
    fmpq_clear(saved);
    return st;
}
/* Independent evaluation of E(t) by FLINT at 512 bits; cardinal values are exact. */
static void reference(acb_t z, const fmpq_t t)
{
    fmpq_t q; fmpq_init(q); fmpq_mul_2exp(q, t, 1);
    arb_sin_cos_pi_fmpq(acb_imagref(z), acb_realref(z), q, 512);
    fmpq_clear(q);
}
static void coordinate(arb_t z, const fmpq_t d, int negative)
{
    fmpq_t q; fmpq_init(q); fmpq_mul_2exp(q, d, 1);
    arb_cos_pi_fmpq(z, q, 512); if (negative) arb_neg(z, z);
    fmpq_clear(q);
}
/* 1 iff the stored coordinate contains [low, high] and each endpoint exceeds it by at most
   4 eps + 2^-28 (W/2 + 2 eps), eps = 2^-p, W = high - low. Reference uncertainty < 2^-500. */
static int hull_ok(const arb_t z, const arb_t low, const arb_t high, slong p)
{
    arf_t lo, hi; arb_t l, h, bound, eps, tmp; int ok;
    arf_init(lo); arf_init(hi); arb_init(l); arb_init(h); arb_init(bound); arb_init(eps); arb_init(tmp);
    arb_get_interval_arf(lo, hi, z, ARF_PREC_EXACT);
    arb_set_arf(l, lo); arb_set_arf(h, hi);
    ok = arb_le(l, low) && arb_ge(h, high);
    arb_one(eps); arb_mul_2exp_si(eps, eps, -p);
    arb_sub(bound, high, low, 512); arb_mul_2exp_si(bound, bound, -1);
    arb_mul_2exp_si(tmp, eps, 1); arb_add(bound, bound, tmp, 512);
    arb_mul_2exp_si(bound, bound, -28);
    arb_mul_2exp_si(tmp, eps, 2); arb_add(bound, bound, tmp, 512);
    arb_sub(tmp, low, bound, 512); ok = ok && arb_ge(l, tmp);
    arb_add(tmp, high, bound, 512); ok = ok && arb_le(h, tmp);
    arf_clear(lo); arf_clear(hi); arb_clear(l); arb_clear(h); arb_clear(bound); arb_clear(eps); arb_clear(tmp);
    return ok;
}
/* The exact union hull from the four oracle distances (Q4), checked on both coordinates. */
static int rect_ok(const acb_t z, const jsonl_value *dist, slong p)
{
    arb_t lo, hi; fmpq_t d; int ok;
    arb_init(lo); arb_init(hi); fmpq_init(d);
    jq(d, at(dist, 1)); coordinate(lo, d, 1); jq(d, at(dist, 0)); coordinate(hi, d, 0);
    ok = hull_ok(acb_realref(z), lo, hi, p);
    jq(d, at(dist, 3)); coordinate(lo, d, 1); jq(d, at(dist, 2)); coordinate(hi, d, 0);
    ok = ok && hull_ok(acb_imagref(z), lo, hi, p);
    arb_clear(lo); arb_clear(hi); fmpq_clear(d);
    return ok;
}
static int contains_phase(const acb_t z, const fmpq_t t)
{
    acb_t w; int ok; acb_init(w); reference(w, t); ok = acb_contains(z, w); acb_clear(w); return ok;
}
/* Exact membership of a sample (t, a) in the stored entry x: rational real endpoints, finite ball. */
static void member(const adf_adele_t x, const fmpq_t t, const fmpq_t a)
{
    arf_t l, h; fmpq_t end; adf_rat_t c;
    arf_init(l); arf_init(h); fmpq_init(end); adf_rat_init(c);
    arb_get_interval_arf(l, h, x->inf, ARF_PREC_EXACT);
    arf_get_fmpq(end, l); CHECK(fmpq_cmp(end, t) <= 0);
    arf_get_fmpq(end, h); CHECK(fmpq_cmp(t, end) <= 0);
    fmpq_set(c->q, a); CHECK(adf_fball_contains_rat(&x->fin, c));
    arf_clear(l); arf_clear(h); fmpq_clear(end); adf_rat_clear(c);
}
static unsigned long npoints, nhulls, nrecords;
static void vectors(void)
{
    jsonl_file *f; jsonl_error_t e; adf_qclass_t q, lift; adf_adele_t x, got;
    fmpq_t t, a, ph, want; acb_t z, s;
    adf_qclass_init(q); adf_qclass_init(lift); adf_adele_init(x); adf_adele_init(got);
    fmpq_init(t); fmpq_init(a); fmpq_init(ph); fmpq_init(want); acb_init(z); acb_init(s);
    CHECK(jsonl_open("tests/ref/vectors/q-slice5/class.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 32);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i), *entries = field(v, "entries");
        const char *kind = jsonl_string(field(v, "kind"), NULL, &e);
        slong n = (slong) jsonl_size(entries);
        nrecords++;
        if (!strcmp(kind, "lift")) {
            CHECK(n == 1); entry_read(x, at(entries, 0)); adf_qclass_set_adele(q, x);
        } else if (!strcmp(kind, "reduce")) {
            entry_read(x, field(v, "lift")); adf_qclass_set_adele(lift, x);
            CHECK(adf_qclass_reduce(q, lift, jint(field(v, "limit")), jint(field(v, "prec"))) == ADF_OK);
            CHECK(adf_qclass_form(q) == ADF_QCLASS_PIECES && adf_qclass_length(q) == n);
            /* The stored C pieces are the oracle's rounded pieces, exactly. */
            for (slong k = 0; k < n; k++) {
                CHECK(adf_qclass_get_piece(got, q, k) == ADF_OK);
                entry_read(x, at(entries, k));
                CHECK(arb_equal(got->inf, x->inf)); CHECK(adf_fball_equal_set(&got->fin, &x->fin));
            }
        } else {
            CHECK(!strcmp(kind, "pieces")); pieces_build(q, entries);
        }
        CHECK(adf_qclass_length(q) == n);
        const char *strict = jsonl_string(field(v, "strict"), NULL, &e);
        int strict_st = !strcmp(strict, "OK") ? ADF_OK : ADF_NOT_DETERMINED;
        const jsonl_value *phase = field(v, "phase"), *arcs = field(v, "arcs");
        int single = !jsonl_is(phase, JSONL_NULL);
        CHECK(qphase(ph, q) == (single ? ADF_OK : ADF_NOT_DETERMINED));
        if (single) { jq(want, phase); CHECK(fmpq_equal(ph, want)); }
        const slong precs[] = {2, 20, 53, 128};
        for (int j = 0; j < 4; j++) {
            slong p = precs[j];
            CHECK(qeval(z, q, p, 0) == ADF_OK);
            CHECK(qeval(s, q, p, 1) == strict_st);
            if (strict_st == ADF_OK) CHECK(acb_equal(z, s));
            CHECK(rect_ok(z, field(v, "dist"), p)); nhulls += 2;
            if (single) CHECK(contains_phase(z, want));
            if (!jsonl_is(arcs, JSONL_NULL))
                for (size_t k = 0; k < jsonl_size(arcs); k++) {
                    const jsonl_value *arc = at(arcs, k);
                    fmpq_t lo, hi; fmpq_init(lo); fmpq_init(hi);
                    jq(lo, at(arc, 0)); jq(hi, at(arc, 1));
                    CHECK(contains_phase(z, lo)); CHECK(contains_phase(z, hi));
                    fmpq_add(lo, lo, hi); fmpq_div_2exp(lo, lo, 1); CHECK(contains_phase(z, lo));
                    fmpq_clear(lo); fmpq_clear(hi);
                }
            /* 40 sampled points of EVERY stored entry, exact phase checked, inside z. */
            const jsonl_value *pts = field(v, "points"); CHECK((slong) jsonl_size(pts) == n);
            for (slong k = 0; k < n; k++) {
                const jsonl_value *pk = at(pts, k);
                CHECK(jsonl_size(pk) == 40);
                if (j == 0) CHECK(adf_qclass_get_piece(got, q, k) == ADF_OK);
                for (size_t r = 0; r < 40; r++) {
                    const jsonl_value *pt = at(pk, r);
                    jq(t, at(pt, 0)); jq(a, at(pt, 1)); jq(ph, at(pt, 2));
                    fmpq_sub(want, a, t); mod1(want); CHECK(fmpq_equal(ph, want));
                    if (j == 0) member(got, t, a);
                    CHECK(contains_phase(z, ph)); npoints++;
                }
            }
        }
    }
    jsonl_close(f);
    adf_qclass_clear(q); adf_qclass_clear(lift); adf_adele_clear(x); adf_adele_clear(got);
    fmpq_clear(t); fmpq_clear(a); fmpq_clear(ph); fmpq_clear(want); acb_clear(z); acb_clear(s);
}
/* The D3-3 sentence: E3's lift is NOT_DETERMINED for strict; its exact two-piece reduction passes;
   both default rectangles enclose +1 and -1 (api-3.md 3.2:379-389, D3-3:809-817). Then the two
   discrimination checks of the brief: a unit square where the hull is a line is rejected by the hull
   test, and the result of evaluating only the first stored piece misses -1. */
static void e3(void)
{
    adf_qclass_t lift, red; adf_adele_t x; acb_t z, w, first; fmpq_t q;
    adf_qclass_init(lift); adf_qclass_init(red); adf_adele_init(x);
    acb_init(z); acb_init(w); acb_init(first); fmpq_init(q);
    CHECK(adf_adele_set_str(x, "(0 ; 0 mod 1/2)", 15, 128, NULL) == ADF_OK);
    adf_qclass_set_adele(lift, x);
    CHECK(adf_qclass_reduce(red, lift, 2, 53) == ADF_OK);
    CHECK(adf_qclass_form(red) == ADF_QCLASS_PIECES && adf_qclass_length(red) == 2);
    CHECK(qeval(z, lift, 128, 1) == ADF_NOT_DETERMINED);
    CHECK(qeval(z, red, 128, 1) == ADF_OK);
    for (int k = 0; k < 2; k++) {
        CHECK(qeval(z, k ? red : lift, 128, 0) == ADF_OK);
        fmpq_zero(q); CHECK(contains_phase(z, q));
        fmpq_set_si(q, 1, 2); CHECK(contains_phase(z, q));
        CHECK(arb_is_zero(acb_imagref(z)));                 /* a line, not a square */
        CHECK(qphase(q, k ? red : lift) == ADF_NOT_DETERMINED);
    }
    /* A unit square fails the imaginary hull test of the line [-1,1] x {0} at p = 128. */
    arb_t lo, hi; arb_init(lo); arb_init(hi);
    acb_zero(w); mag_one(arb_radref(acb_realref(w))); mag_one(arb_radref(acb_imagref(w)));
    arb_set_si(lo, -1); arb_one(hi); CHECK(hull_ok(acb_realref(w), lo, hi, 128));
    arb_zero(lo); arb_zero(hi); CHECK(!hull_ok(acb_imagref(w), lo, hi, 128));
    arb_clear(lo); arb_clear(hi);
    /* Only the first stored piece: E(0) = 1 exactly; the point -1 of the second piece is outside. */
    CHECK(adf_qclass_get_piece(x, red, 0) == ADF_OK);
    CHECK(adf_adele_psi_tate(first, x, 128) == ADF_OK && acb_is_one(first));
    fmpq_set_si(q, 1, 2); CHECK(!contains_phase(first, q));
    /* The driver example of api-3.md 7: union((0 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q, built by hand. */
    adf_adele_struct two[2]; adf_adele_init(two); adf_adele_init(two + 1);
    CHECK(adf_adele_set_str(two, "(0 ; 0 mod 1)", 13, 128, NULL) == ADF_OK);
    CHECK(adf_adele_set_str(two + 1, "(0.5 ; 0 mod 1)", 15, 128, NULL) == ADF_OK);
    pieces_from(red, two, 2); CHECK(adf_qclass_is_canonical(red));
    CHECK(qeval(z, red, 128, 0) == ADF_OK && qeval(w, red, 128, 1) == ADF_OK && acb_equal(z, w));
    CHECK(arb_is_zero(acb_imagref(z)) && arb_contains_si(acb_realref(z), 1) && arb_contains_si(acb_realref(z), -1));
    adf_adele_clear(two); adf_adele_clear(two + 1);
    adf_qclass_clear(lift); adf_qclass_clear(red); adf_adele_clear(x);
    acb_clear(z); acb_clear(w); acb_clear(first); fmpq_clear(q);
}
/* The rows of tests/golden/psi_phases.tsv as classes: the lift of (0 ; row). The class character is the
   adele character on a lift; every listed angle is enclosed; strict and the phase getter decide by the
   number of angles (one angle iff integral finite radius, golden README and conventions 6.1:876-878). */
static void golden(void)
{
    golden_file *f; golden_error_t e; adf_adele_t x; adf_qclass_t q; fmpq_t th, want; acb_t z, w;
    adf_adele_init(x); adf_qclass_init(q); fmpq_init(th); fmpq_init(want); acb_init(z); acb_init(w);
    CHECK(golden_open("tests/golden/psi_phases.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &e));
    CHECK(golden_count(f) == 17);
    for (size_t i = 0; i < golden_count(f); i++) {
        const golden_record *r = golden_record_at(f, i);
        if (r->is_status) continue;           /* parser rows: tests/test_psi.c */
        CHECK(adf_fball_set_str(&x->fin, r->input, r->input_len, NULL) == ADF_OK);
        arb_zero(x->inf); adf_qclass_set_adele(q, x);
        char *text = malloc(r->expected_len+1); CHECK(text != NULL);
        memcpy(text, r->expected, r->expected_len); text[r->expected_len] = 0;
        int n = 0;
        CHECK(qeval(z, q, 128, 0) == ADF_OK);
        CHECK(adf_adele_psi_tate(w, x, 128) == ADF_OK && acb_equal(z, w));
        for (char *tok = strtok(text, " "); tok; tok = strtok(NULL, " ")) {
            qread(want, tok); CHECK(contains_phase(z, want)); n++;
        }
        CHECK(qeval(w, q, 128, 1) == (n == 1 ? ADF_OK : ADF_NOT_DETERMINED));
        /* The lift and its adele agree on strict too (mutation survivor psi.c:290 of the restricted sweep). */
        CHECK(adf_adele_psi_tate_strict(z, x, 128) == (n == 1 ? ADF_OK : ADF_NOT_DETERMINED));
        CHECK(qphase(th, q) == (n == 1 ? ADF_OK : ADF_NOT_DETERMINED));
        if (n == 1) CHECK(fmpq_equal(th, want));
        free(text);
    }
    golden_close(f);
    adf_adele_clear(x); adf_qclass_clear(q); fmpq_clear(th); fmpq_clear(want); acb_clear(z); acb_clear(w);
}
/* Statuses: prec LIMIT first; exact preflight LIMIT of any entry beats fractional radius (strict) and the
   numerical certificate; NOT_DETERMINED from one entry's certificate fails the class; the phase getter's
   LIMIT is first and two different singletons are NOT_DETERMINED. */
static void statuses(void)
{
    adf_adele_struct e[2]; adf_qclass_t q; acb_t z; fmpq_t th; arf_t m;
    adf_adele_init(e); adf_adele_init(e + 1); adf_qclass_init(q); acb_init(z); fmpq_init(th); arf_init(m);
    /* Same singleton phase 3/4 in two entries: OK; z equals adf_adele_psi_tate on either entry. */
    CHECK(adf_adele_set_str(e, "(0.25 ; 0)", 10, 128, NULL) == ADF_OK);
    CHECK(adf_adele_set_str(e + 1, "(0.25 ; 0 mod 3)", 16, 128, NULL) == ADF_OK);
    pieces_from(q, e, 2); CHECK(adf_qclass_is_canonical(q));
    CHECK(qphase(th, q) == ADF_OK);
    { fmpq_t w; fmpq_init(w); fmpq_set_si(w, 3, 4); CHECK(fmpq_equal(th, w)); fmpq_clear(w); }
    acb_t w; acb_init(w);
    CHECK(qeval(z, q, 64, 0) == ADF_OK && adf_adele_psi_tate(w, e, 64) == ADF_OK && acb_equal(z, w));
    CHECK(acb_is_exact(z));                                     /* E(3/4) = -i exactly */
    /* Different singletons: 3/4 and 1/4 (real midpoints 0.25 and 0.75). */
    CHECK(adf_adele_set_str(e + 1, "(0.75 ; 0)", 10, 128, NULL) == ADF_OK);
    pieces_from(q, e, 2); CHECK(adf_qclass_is_canonical(q));
    CHECK(qphase(th, q) == ADF_NOT_DETERMINED);
    CHECK(qeval(z, q, 64, 1) == ADF_OK && arb_is_zero(acb_realref(z)));
    /* One entry with real radius: NOT_DETERMINED although the other entry and all centres agree. */
    adf_adele_set(e + 1, e);
    CHECK(adf_adele_set_str(e, "(0.25 +/- 0.125 ; 0)", 20, 128, NULL) == ADF_OK);
    pieces_from(q, e, 2); CHECK(adf_qclass_is_canonical(q));
    CHECK(qphase(th, q) == ADF_NOT_DETERMINED);
    /* prec above the cap: LIMIT first, on every call. */
    CHECK(qeval(z, q, ADF_REAL_PREC_MAX+1, 0) == ADF_LIMIT);
    CHECK(qeval(z, q, ADF_REAL_PREC_MAX+1, 1) == ADF_LIMIT);
    /* An entry over the exact bound (mantissa of cap bits at exponent 0) after an entry that is fine:
       every call is LIMIT, also the phase getter and strict on a lift-fractional first entry. */
    CHECK(adf_adele_set_str(e, "(0.125 ; 0 mod 1)", 17, 128, NULL) == ADF_OK);
    arf_one(m); arf_mul_2exp_si(m, m, -ADF_QCLASS_BITS_MAX);
    arf_sub_ui(m, m, 1, ARF_PREC_EXACT, ARF_RND_NEAR); arf_neg(m, m);       /* 1 - 2^-cap */
    arb_zero(e[1].inf); adf_fball_zero(&e[1].fin); arf_set(arb_midref(e[1].inf), m);
    CHECK(adf_fball_set_str(&e[1].fin, "(* ; 0 mod 1)", 13, NULL) == ADF_OK);
    pieces_from(q, e, 2); CHECK(adf_qclass_is_canonical(q));
    CHECK(qeval(z, q, 53, 0) == ADF_LIMIT); CHECK(qeval(z, q, 53, 1) == ADF_LIMIT);
    CHECK(qphase(th, q) == ADF_LIMIT);
    CHECK(qeval(z, q, ADF_REAL_PREC_MAX, 0) == ADF_LIMIT);       /* preflight before any certificate */
    /* With cap-8 bits the read and the projected distances are admitted (the adele call agrees). */
    arf_one(m); arf_mul_2exp_si(m, m, -(ADF_QCLASS_BITS_MAX-8));
    arf_sub_ui(m, m, 1, ARF_PREC_EXACT, ARF_RND_NEAR); arf_neg(m, m);
    arf_set(arb_midref(e[1].inf), m); pieces_from(q, e, 2); CHECK(adf_qclass_is_canonical(q));
    CHECK(qeval(z, q, 53, 0) == ADF_OK); CHECK(qphase(th, q) == ADF_NOT_DETERMINED);
    CHECK(adf_adele_psi_tate(w, e + 1, 53) == ADF_OK);
    /* Lift with fractional radius and an over-bound finite centre: strict is LIMIT (preflight first),
       not NOT_DETERMINED; the phase getter is LIMIT. */
    arb_zero(e->inf); adf_fball_zero(&e->fin); adf_rat_t c, N; adf_rat_init(c); adf_rat_init(N);
    fmpq_one(c->q); fmpz_mul_2exp(fmpq_denref(c->q), fmpq_denref(c->q), ADF_QCLASS_BITS_MAX);
    fmpq_set_si(N->q, 1, 2); CHECK(adf_fball_set_center_radius(&e->fin, c, N) == ADF_OK);
    adf_qclass_set_adele(q, e);
    CHECK(qeval(z, q, 53, 1) == ADF_LIMIT); CHECK(qeval(z, q, 53, 0) == ADF_LIMIT);
    CHECK(qphase(th, q) == ADF_LIMIT);
    adf_rat_clear(c); adf_rat_clear(N);
    /* A numerical certificate failure of one entry (a noncardinal phase at the cap) fails the class;
       the cardinal second entry alone succeeds there. */
    CHECK(adf_adele_set_str(e, "(0.125 ; 0 mod 1)", 17, 128, NULL) == ADF_OK);
    CHECK(adf_adele_set_str(e + 1, "(0.5 ; 0 mod 1)", 15, 128, NULL) == ADF_OK);
    pieces_from(q, e, 2); CHECK(adf_qclass_is_canonical(q));
    CHECK(qeval(z, q, ADF_REAL_PREC_MAX, 0) == ADF_NOT_DETERMINED);
    CHECK(qeval(z, q, ADF_REAL_PREC_MAX, 1) == ADF_NOT_DETERMINED);
    adf_qclass_set_adele(q, e + 1);
    CHECK(qeval(z, q, ADF_REAL_PREC_MAX, 0) == ADF_OK && acb_is_exact(z));
    /* Strict on PIECES built from a fractional-radius entry can only come from a lift; a lift whose real
       radius is positive and finite radius integral passes strict (real uncertainty is allowed). */
    CHECK(adf_adele_set_str(e, "(0.5 +/- 0.25 ; 1/3 mod 2)", 26, 128, NULL) == ADF_OK);
    adf_qclass_set_adele(q, e);
    CHECK(qeval(z, q, 53, 1) == ADF_OK && qphase(th, q) == ADF_NOT_DETERMINED);
    acb_clear(w);
    adf_adele_clear(e); adf_adele_clear(e + 1); adf_qclass_clear(q); acb_clear(z); fmpq_clear(th); arf_clear(m);
}
/* Additivity on classes: psi of x + y contains the products of sampled phases of x and y, and the
   product of the two rectangles contains them too (Q4 step 6). Lifts of random small adeles. */
static void additivity(void)
{
    adf_adele_t x, y, s; adf_qclass_t qx, qy, qs; acb_t zx, zy, zs, prod; fmpq_t t; flint_rand_t st;
    adf_adele_init(x); adf_adele_init(y); adf_adele_init(s);
    adf_qclass_init(qx); adf_qclass_init(qy); adf_qclass_init(qs);
    acb_init(zx); acb_init(zy); acb_init(zs); acb_init(prod); fmpq_init(t); flint_randinit(st);
    for (int i = 0; i < 100; i++) {
        fmpq_t m, r, a, N; fmpq_init(m); fmpq_init(r); fmpq_init(a); fmpq_init(N);
        fmpq_set_si(m, (slong) n_randint(st, 129) - 64, 32); fmpq_set_si(r, (slong) n_randint(st, 3), 64);
        fmpq_set_si(a, (slong) n_randint(st, 41) - 20, 1 + n_randint(st, 12));
        fmpq_set_si(N, (slong) n_randint(st, 4), 1 + n_randint(st, 4));
        setball(x, m, r, a, N);
        fmpq_set_si(a, (slong) n_randint(st, 41) - 20, 1 + n_randint(st, 6)); setball(y, m, r, a, N);
        adf_adele_add(s, x, y, 256);
        adf_qclass_set_adele(qx, x); adf_qclass_set_adele(qy, y); adf_qclass_set_adele(qs, s);
        CHECK(qeval(zx, qx, 128, 0) == ADF_OK && qeval(zy, qy, 128, 0) == ADF_OK);
        CHECK(qeval(zs, qs, 128, 0) == ADF_OK); acb_mul(prod, zx, zy, 256);
        for (int k = 0; k < 10; k++) {
            /* points of x and y with the same real and finite step */
            fmpq_t tx, ty, ax, ay; fmpq_init(tx); fmpq_init(ty); fmpq_init(ax); fmpq_init(ay);
            arf_get_fmpq(tx, arb_midref(x->inf)); fmpq_set_si(t, 2*k-9, 9); fmpq_mul(t, t, r);
            fmpq_add(tx, tx, t); fmpq_set(ty, tx);
            adf_rat_t c; adf_rat_init(c); adf_fball_get_center(c, &x->fin); fmpq_set(ax, c->q);
            adf_fball_get_center(c, &y->fin); fmpq_set(ay, c->q); adf_rat_clear(c);
            fmpq_set_si(t, k, 1); fmpq_mul(t, t, N); fmpq_add(ax, ax, t); fmpq_add(ay, ay, t);
            fmpq_add(t, ax, ay); fmpq_sub(t, t, tx); fmpq_sub(t, t, ty); mod1(t);
            CHECK(contains_phase(zs, t)); CHECK(contains_phase(prod, t));
            fmpq_clear(tx); fmpq_clear(ty); fmpq_clear(ax); fmpq_clear(ay);
        }
        fmpq_clear(m); fmpq_clear(r); fmpq_clear(a); fmpq_clear(N);
    }
    adf_adele_clear(x); adf_adele_clear(y); adf_adele_clear(s);
    adf_qclass_clear(qx); adf_qclass_clear(qy); adf_qclass_clear(qs);
    acb_clear(zx); acb_clear(zy); acb_clear(zs); acb_clear(prod); fmpq_clear(t); flint_randclear(st);
}
#ifdef ADF_CHECK_INVARIANTS
/* A forged class (bad tag) aborts each of the three class calls under INV. */
static void inv(void)
{
    for (int k = 0; k < 3; k++) {
        pid_t pid = fork(); CHECK(pid >= 0);
        if (!pid) {
            adf_qclass_t q; acb_t z; fmpq_t th;
            if (!freopen("/dev/null", "w", stderr)) _exit(2);
            adf_qclass_init(q); acb_init(z); fmpq_init(th); q->form = 7;
            if (k == 0) adf_qclass_psi_tate(z, q, 64);
            if (k == 1) adf_qclass_psi_tate_strict(z, q, 64);
            if (k == 2) adf_qclass_psi_tate_phase(th, q);
            _exit(0);
        }
        int status; CHECK(waitpid(pid, &status, 0) == pid);
        CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
}
#endif
int main(int argc, char **argv)
{
    const char *s = argc == 2 ? argv[1] : "all";
    if (!strcmp(s, "all") || !strcmp(s, "e3")) e3();
    if (!strcmp(s, "all") || !strcmp(s, "golden")) golden();
    if (!strcmp(s, "all") || !strcmp(s, "statuses")) statuses();
    if (!strcmp(s, "all") || !strcmp(s, "vectors")) vectors();
    if (!strcmp(s, "all") || !strcmp(s, "additivity")) additivity();
#ifdef ADF_CHECK_INVARIANTS
    if (!strcmp(s, "all")) inv();
#endif
    printf("psi_class: %lu checks, %lu records, %lu points, %lu hull coordinates (%s)\n",
           checks, nrecords, npoints, nhulls, s);
    flint_cleanup(); return 0;
}
