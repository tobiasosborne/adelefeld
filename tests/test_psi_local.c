/* Slice 3.2-c (lane q-slice5): the local character and the adele place variants, docs/api-3.md 3.2:391-414,
   Q4 step 8:625-628. Oracle: tests/ref/vectors/q-slice5/local.jsonl and place.jsonl from
   lanes/q-slice5/gen_vectors.py (proto/quotient3_checks.py local_phase, local_image, psi, phase_arcs).
   Signs (conventions 6.1:844; refs/src/tate-poonen/notes.txt:693-700): psi_p(x) = E(fp_p(x)), psi_inf(x) =
   E(-x). Hand check before the code: fp_2(1/6) = 1/2, since 1/6 - 1/2 = -1/3 lies in Z_2; the ordinary
   fractional part 1/6 is wrong at 2 (fault 3 of api-3.md 5). So psi_2(1/6) = E(1/2) = -1 exactly, and at
   p = 3, fp_3(1/6) = 2/3 (1/6 - 2/3 = -1/2 lies in Z_3), psi_3(1/6) = E(2/3). With psi_inf(1/6)... the
   diagonal 1/6 has the product E(1/2) E(2/3) E(-1/6) = E(1) = 1 (analysis Lemma 2:87-88).
   Hull bound asserted: the one of api-3.md 3.3, as in tests/test_psi_class.c. */
#include <adelefeld.h>
#include "support/jsonl.h"
#include "support/golden.h"
#include <flint/ulong_extras.h>
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
static const char *str(const jsonl_value *v)
{ jsonl_error_t e; const char *s = jsonl_string(v, NULL, &e); CHECK(s != NULL); return s; }
static void jq(fmpq_t q, const jsonl_value *v) { qread(q, str(v)); }
static void setball(adf_adele_t x, const fmpq_t m, const fmpq_t r, const fmpq_t a, const fmpq_t N)
{
    adf_rat_t c, n;
    adf_rat_init(c); adf_rat_init(n);
    fmpq_set(c->q, a); fmpq_set(n->q, N);
    CHECK(adf_fball_set_center_radius(&x->fin, c, n) == ADF_OK);
    arb_set_fmpq(x->inf, m, FLINT_MAX(4096, fmpz_bits(fmpq_numref(m)) + 64));
    CHECK(arb_is_exact(x->inf)); CHECK(fmpz_bits(fmpq_numref(r)) <= 30);
    mag_set_ui_2exp_si(arb_radref(x->inf), fmpz_get_ui(fmpq_numref(r)), 1-fmpz_bits(fmpq_denref(r)));
    CHECK(adf_adele_is_canonical(x));
    adf_rat_clear(c); adf_rat_clear(n);
}
static void seed_phase(fmpq_t q) { fmpq_set_si(q, 19, 7); fmpq_mul_2exp(q, q, 130); }
static void seed_complex(acb_t z)
{
    acb_set_si(z, 17);
    arf_mul_2exp_si(arb_midref(acb_realref(z)), arb_midref(acb_realref(z)), 130);
    arf_add_ui(arb_midref(acb_realref(z)), arb_midref(acb_realref(z)), 1, ARF_PREC_EXACT, ARF_RND_NEAR);
    arb_set_si(acb_imagref(z), -23); mag_set_ui_2exp_si(arb_radref(acb_realref(z)), 1, -4);
}
static int leval(acb_t z, const adf_lball_t x, slong p, int strict)
{
    adf_lball_t copy; acb_t saved; acb_struct bytes; int st;
    adf_lball_init(copy); adf_lball_set(copy, x);
    seed_complex(z); acb_init(saved); acb_set(saved, z); memcpy(&bytes, z, sizeof(bytes));
    st = strict ? adf_lball_psi_tate_strict(z, x, p) : adf_lball_psi_tate(z, x, p);
    if (st) { CHECK(!memcmp(&bytes, z, sizeof(bytes))); CHECK(acb_equal(z, saved)); }
    else CHECK(acb_is_finite(z));
    CHECK(adf_lball_identical(copy, x));
    acb_clear(saved); adf_lball_clear(copy);
    return st;
}
static int lphase(fmpq_t q, const adf_lball_t x)
{
    fmpq_t saved; fmpq qbytes; int st;
    seed_phase(q); fmpq_init(saved); fmpq_set(saved, q); memcpy(&qbytes, q, sizeof(qbytes));
    st = adf_lball_psi_tate_phase(q, x);
    if (st) { CHECK(!memcmp(&qbytes, q, sizeof(qbytes))); CHECK(fmpq_equal(q, saved)); }
    else CHECK(fmpq_is_canonical(q) && fmpq_sgn(q) >= 0 && fmpq_cmp_si(q, 1) < 0);
    fmpq_clear(saved);
    return st;
}
/* psi_at through both where forms: NULL, and a sentinel that must be untouched on OK and v otherwise. */
static int aeval(acb_t z, const adf_adele_t x, adf_place_t v, slong p, int strict)
{
    acb_t saved, z2; acb_struct bytes; adf_place_t where, sentinel; int st, st2;
    adf_adele_t copy; adf_adele_init(copy); adf_adele_set(copy, x);
    acb_init(saved); acb_init(z2);
    CHECK(adf_place_prime(&sentinel, 1000003) == ADF_OK); where = sentinel;
    seed_complex(z); acb_set(saved, z); memcpy(&bytes, z, sizeof(bytes));
    st = strict ? adf_adele_psi_tate_strict_at(z, &where, x, v, p) : adf_adele_psi_tate_at(z, &where, x, v, p);
    if (st) {
        CHECK(!memcmp(&bytes, z, sizeof(bytes))); CHECK(acb_equal(z, saved)); CHECK(adf_place_equal(where, v));
    } else { CHECK(acb_is_finite(z)); CHECK(!memcmp(&where, &sentinel, sizeof(where))); }
    seed_complex(z2);
    st2 = strict ? adf_adele_psi_tate_strict_at(z2, NULL, x, v, p) : adf_adele_psi_tate_at(z2, NULL, x, v, p);
    CHECK(st2 == st); if (!st) CHECK(acb_equal(z, z2)); else CHECK(acb_equal(z2, saved));
    CHECK(adf_adele_identical(copy, x));
    acb_clear(saved); acb_clear(z2); adf_adele_clear(copy);
    return st;
}
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
static void arcs_inside(const acb_t z, const jsonl_value *arcs)
{
    if (jsonl_is(arcs, JSONL_NULL)) return;
    fmpq_t lo, hi; fmpq_init(lo); fmpq_init(hi);
    for (size_t k = 0; k < jsonl_size(arcs); k++) {
        jq(lo, at(at(arcs, k), 0)); jq(hi, at(at(arcs, k), 1));
        CHECK(contains_phase(z, lo)); CHECK(contains_phase(z, hi));
        fmpq_add(lo, lo, hi); fmpq_div_2exp(lo, lo, 1); CHECK(contains_phase(z, lo));
    }
    fmpq_clear(lo); fmpq_clear(hi);
}
static ulong jprime(const jsonl_value *v) { return strtoul(str(v), NULL, 10); }
static void place_of(adf_place_t *v, ulong p) { CHECK(adf_place_prime(v, p) == ADF_OK); }
static const slong precs[] = {2, 20, 53, 128};
static unsigned long nlocal, nplaces, nhulls;
static void local_vectors(void)
{
    jsonl_file *f; jsonl_error_t e; adf_lball_t x; adf_place_t v; adf_rat_t c; fmpq_t th, want;
    acb_t z, s; fmpz_t B;
    adf_lball_init(x); adf_rat_init(c); fmpq_init(th); fmpq_init(want); acb_init(z); acb_init(s); fmpz_init(B);
    CHECK(jsonl_open("tests/ref/vectors/q-slice5/local.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 432);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *r = jsonl_record(f, i), *ev = field(r, "e"), *phase = field(r, "phase");
        place_of(&v, jprime(field(r, "p"))); jq(c->q, field(r, "a"));
        if (jsonl_is(ev, JSONL_NULL)) CHECK(adf_lball_set_rat(x, v, c) == ADF_OK);
        else CHECK(adf_lball_set_rat_ball(x, v, c, atol(jsonl_int_text(ev, &e))) == ADF_OK);
        int single = !jsonl_is(phase, JSONL_NULL);
        CHECK(lphase(th, x) == (single ? ADF_OK : ADF_NOT_DETERMINED));
        if (single) { jq(want, phase); CHECK(fmpq_equal(th, want)); }
        CHECK(fmpz_set_str(B, str(field(r, "order")), 10) == 0);
        CHECK(single == fmpz_is_one(B));
        for (int j = 0; j < 4; j++) {
            CHECK(leval(z, x, precs[j], 0) == ADF_OK);
            CHECK(leval(s, x, precs[j], 1) == (single ? ADF_OK : ADF_NOT_DETERMINED));
            if (single) { CHECK(acb_equal(z, s)); CHECK(contains_phase(z, want)); }
            CHECK(rect_ok(z, field(r, "dist"), precs[j])); nhulls += 2;
            arcs_inside(z, field(r, "arcs"));
            /* The base of the root family is a member at every order. */
            jq(want, field(r, "base")); CHECK(contains_phase(z, want));
        }
        nlocal++;
    }
    jsonl_close(f);
    adf_lball_clear(x); adf_rat_clear(c); fmpq_clear(th); fmpq_clear(want);
    acb_clear(z); acb_clear(s); fmpz_clear(B);
}
static void place_vectors(void)
{
    jsonl_file *f; jsonl_error_t e; adf_adele_t x; adf_lball_t lb; adf_place_t v; fmpq_t m, r, a, N, th, sum, want;
    acb_t z, s, prod, g; fmpz_t B;
    adf_adele_init(x); adf_lball_init(lb); fmpq_init(m); fmpq_init(r); fmpq_init(a); fmpq_init(N);
    fmpq_init(th); fmpq_init(sum); fmpq_init(want); acb_init(z); acb_init(s); acb_init(prod); acb_init(g);
    fmpz_init(B);
    CHECK(jsonl_open("tests/ref/vectors/q-slice5/place.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 30);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *rec = jsonl_record(f, i), *xv = field(rec, "x"), *pl = field(rec, "places");
        jq(m, at(xv, 0)); jq(r, at(xv, 1)); jq(a, at(xv, 2)); jq(N, at(xv, 3));
        setball(x, m, r, a, N);
        for (int j = 0; j < 4; j++) {
            slong p = precs[j];
            fmpq_zero(sum); acb_one(prod);
            int all_single = 1;
            for (size_t k = 0; k < jsonl_size(pl); k++) {
                const jsonl_value *P = at(pl, k), *phase = field(P, "phase");
                int inf = !strcmp(str(field(P, "v")), "inf");
                if (inf) v = adf_place_inf(); else place_of(&v, jprime(field(P, "v")));
                CHECK(fmpz_set_str(B, str(field(P, "order")), 10) == 0);
                CHECK(aeval(z, x, v, p, 0) == ADF_OK);
                /* strict: real uncertainty allowed at infinity; NOT_DETERMINED iff v_p(N) < 0 at a prime */
                CHECK(aeval(s, x, v, p, 1) == (fmpz_is_one(B) ? ADF_OK : ADF_NOT_DETERMINED));
                if (fmpz_is_one(B)) CHECK(acb_equal(z, s));
                CHECK(rect_ok(z, field(P, "dist"), p)); nhulls += 2;
                arcs_inside(z, field(P, "arcs"));
                jq(want, field(P, "base")); CHECK(contains_phase(z, want));
                if (!jsonl_is(phase, JSONL_NULL)) {
                    jq(want, phase); fmpq_add(sum, sum, want); CHECK(contains_phase(z, want));
                    if (fmpq_is_zero(want) && !inf) CHECK(acb_is_one(z));     /* E(0) = 1 exactly */
                    if (!inf) {               /* the exact local phase through the projection to Q_p */
                        CHECK(adf_lball_set_fball(lb, v, &x->fin) == ADF_OK);
                        CHECK(lphase(th, lb) == ADF_OK && fmpq_equal(th, want));
                    }
                } else all_single = 0;
                acb_mul(prod, prod, z, 512);
                nplaces++;
            }
            const jsonl_value *glob = field(rec, "global");
            CHECK(adf_adele_psi_tate(g, x, p) == ADF_OK);
            if (!jsonl_is(glob, JSONL_NULL)) {
                CHECK(all_single); jq(want, glob); mod1(sum); CHECK(fmpq_equal(sum, want));
                CHECK(adf_adele_psi_tate_phase(th, x) == ADF_OK && fmpq_equal(th, want));
                CHECK(contains_phase(prod, want)); CHECK(acb_overlaps(prod, g));
            }
            /* Sampled points of x: E(a + k N - t) lies in the product of the local enclosures. */
            for (int k = 0; k < 20; k++) {
                fmpq_t t, w; fmpq_init(t); fmpq_init(w);
                fmpq_set_si(t, 2*k-19, 19); fmpq_mul(t, t, r); fmpq_add(t, t, m);
                fmpq_set_si(w, k, 1); fmpq_mul(w, w, N); fmpq_add(w, w, a); fmpq_sub(w, w, t); mod1(w);
                CHECK(contains_phase(prod, w)); CHECK(contains_phase(g, w));
                fmpq_clear(t); fmpq_clear(w);
            }
        }
    }
    jsonl_close(f);
    adf_adele_clear(x); adf_lball_clear(lb); fmpq_clear(m); fmpq_clear(r); fmpq_clear(a); fmpq_clear(N);
    fmpq_clear(th); fmpq_clear(sum); fmpq_clear(want); acb_clear(z); acb_clear(s); acb_clear(prod); acb_clear(g);
    fmpz_clear(B);
}
/* Golden rows of psi_phases.tsv at places: x = (0 ; row). The real factor is exactly 1; the product of the
   local enclosures at the primes dividing the denominators of a and N contains every listed angle; for a
   one-angle row the exact local phases sum to it (Lemma 2 step 2 and the CRT of step 3). */
static void golden(void)
{
    golden_file *f; golden_error_t e; adf_adele_t x; adf_lball_t lb; adf_place_t v; fmpq_t th, sum, want;
    acb_t z, prod; fmpz_t dd;
    adf_adele_init(x); adf_lball_init(lb); fmpq_init(th); fmpq_init(sum); fmpq_init(want);
    acb_init(z); acb_init(prod); fmpz_init(dd);
    CHECK(golden_open("tests/golden/psi_phases.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &e));
    CHECK(golden_count(f) == 17);
    for (size_t i = 0; i < golden_count(f); i++) {
        const golden_record *g = golden_record_at(f, i);
        if (g->is_status) continue;
        CHECK(adf_fball_set_str(&x->fin, g->input, g->input_len, NULL) == ADF_OK);
        arb_zero(x->inf);
        CHECK(aeval(z, x, adf_place_inf(), 128, 0) == ADF_OK && acb_is_one(z));
        adf_rat_t c, N; adf_rat_init(c); adf_rat_init(N);
        adf_fball_get_center(c, &x->fin); adf_fball_get_radius(N, &x->fin);
        fmpz_mul(dd, fmpq_denref(c->q), fmpq_denref(N->q));
        acb_one(prod); fmpq_zero(sum);
        int single = 1;
        for (ulong p = 2; p <= 61; p = n_nextprime(p, 1)) {
            if (!fmpz_divisible_si(dd, (slong) p)) continue;
            place_of(&v, p);
            CHECK(aeval(z, x, v, 128, 0) == ADF_OK); acb_mul(prod, prod, z, 512);
            CHECK(adf_lball_set_fball(lb, v, &x->fin) == ADF_OK);
            if (lphase(th, lb) == ADF_OK) fmpq_add(sum, sum, th); else single = 0;
        }
        char *text = malloc(g->expected_len+1); CHECK(text != NULL);
        memcpy(text, g->expected, g->expected_len); text[g->expected_len] = 0;
        int n = 0;
        for (char *tok = strtok(text, " "); tok; tok = strtok(NULL, " ")) {
            qread(want, tok); CHECK(contains_phase(prod, want)); n++;
        }
        CHECK(single == (n == 1));
        if (n == 1) { mod1(sum); CHECK(fmpq_equal(sum, want)); }
        free(text); adf_rat_clear(c); adf_rat_clear(N);
    }
    golden_close(f);
    adf_adele_clear(x); adf_lball_clear(lb); fmpq_clear(th); fmpq_clear(sum); fmpq_clear(want);
    acb_clear(z); acb_clear(prod); fmpz_clear(dd);
}
/* Fault 3 (api-3.md 5): the exact local 1/6 at 2; the branch at e = 0 at p = 2; e above and below 0. */
static void witnesses(void)
{
    adf_lball_t x; adf_place_t two, three; adf_rat_t c; fmpq_t th; acb_t z, w;
    adf_lball_init(x); adf_rat_init(c); fmpq_init(th); acb_init(z); acb_init(w);
    place_of(&two, 2); place_of(&three, 3);
    fmpq_set_si(c->q, 1, 6);
    CHECK(adf_lball_set_rat(x, two, c) == ADF_OK);
    CHECK(lphase(th, x) == ADF_OK && fmpq_equal_si(th, 0) == 0);
    { fmpq_t h; fmpq_init(h); fmpq_set_si(h, 1, 2); CHECK(fmpq_equal(th, h)); fmpq_clear(h); }
    acb_set_si(w, -1);
    CHECK(leval(z, x, 53, 0) == ADF_OK && acb_equal(z, w));          /* -1 exactly, not E(1/6) */
    CHECK(adf_lball_set_rat(x, three, c) == ADF_OK);
    CHECK(lphase(th, x) == ADF_OK);
    { fmpq_t h; fmpq_init(h); fmpq_set_si(h, 2, 3); CHECK(fmpq_equal(th, h)); fmpq_clear(h); }
    /* e = 0 at p = 2: 1/6 + Z_2 has the single phase 1/2: strict and the getter succeed. */
    CHECK(adf_lball_set_rat_ball(x, two, c, 0) == ADF_OK);
    CHECK(lphase(th, x) == ADF_OK);
    CHECK(leval(z, x, 53, 1) == ADF_OK && acb_equal(z, w));
    /* e = -1 at p = 2: 1/6 + 2^-1 Z_2 has phases 0 and 1/2: the getter and strict fail, default {+1,-1}. */
    CHECK(adf_lball_set_rat_ball(x, two, c, -1) == ADF_OK);
    CHECK(lphase(th, x) == ADF_NOT_DETERMINED);
    CHECK(leval(z, x, 53, 1) == ADF_NOT_DETERMINED);
    CHECK(leval(z, x, 53, 0) == ADF_OK && arb_is_zero(acb_imagref(z)));
    CHECK(arb_contains_si(acb_realref(z), 1) && arb_contains_si(acb_realref(z), -1));
    /* e = 1: 1/6 + 2 Z_2 still one phase. Exact 0 and the ball O(2^-3) around 0. */
    CHECK(adf_lball_set_rat_ball(x, two, c, 1) == ADF_OK);
    CHECK(lphase(th, x) == ADF_OK && leval(z, x, 53, 1) == ADF_OK && acb_equal(z, w));
    fmpq_zero(c->q); CHECK(adf_lball_set_rat(x, two, c) == ADF_OK);
    CHECK(lphase(th, x) == ADF_OK && fmpq_is_zero(th)); CHECK(leval(z, x, 2, 0) == ADF_OK && acb_is_one(z));
    CHECK(adf_lball_set_rat_ball(x, two, c, -3) == ADF_OK);
    CHECK(lphase(th, x) == ADF_NOT_DETERMINED && leval(z, x, 53, 1) == ADF_NOT_DETERMINED);
    CHECK(leval(z, x, 53, 0) == ADF_OK);
    /* the 8th roots: the hull is the full square [-1,1]^2 */
    CHECK(arb_contains_si(acb_realref(z), 1) && arb_contains_si(acb_imagref(z), -1));
    /* Local additivity (analysis L2:85): exact phases add modulo 1, also at 2. */
    flint_rand_t st; flint_randinit(st);
    adf_lball_t y, sxy; adf_lball_init(y); adf_lball_init(sxy); fmpq_t u, t; fmpq_init(u); fmpq_init(t);
    for (int i = 0; i < 300; i++) {
        adf_place_t v; place_of(&v, n_nth_prime(1 + n_randint(st, 5)));
        fmpq_set_si(c->q, (slong) n_randint(st, 201) - 100, 1 + n_randint(st, 360));
        CHECK(adf_lball_set_rat(x, v, c) == ADF_OK);
        fmpq_set_si(c->q, (slong) n_randint(st, 201) - 100, 1 + n_randint(st, 360));
        CHECK(adf_lball_set_rat(y, v, c) == ADF_OK);
        CHECK(adf_lball_add(sxy, x, y) == ADF_OK);
        CHECK(lphase(th, x) == ADF_OK && lphase(u, y) == ADF_OK && lphase(t, sxy) == ADF_OK);
        fmpq_add(th, th, u); mod1(th); CHECK(fmpq_equal(th, t));
    }
    flint_randclear(st); adf_lball_clear(y); adf_lball_clear(sxy); fmpq_clear(u); fmpq_clear(t);
    adf_lball_clear(x); adf_rat_clear(c); fmpq_clear(th); acb_clear(z); acb_clear(w);
}
/* Limits and statuses of the local and place calls. */
static void limits(void)
{
    adf_lball_t x; adf_place_t v, inf = adf_place_inf(); adf_adele_t a; fmpq_t th; acb_t z; adf_rat_t c, N;
    adf_lball_init(x); adf_adele_init(a); fmpq_init(th); acb_init(z); adf_rat_init(c); adf_rat_init(N);
    /* prec above the cap: LIMIT first; also with where. */
    CHECK(leval(z, x, ADF_REAL_PREC_MAX+1, 0) == ADF_LIMIT && leval(z, x, ADF_REAL_PREC_MAX+1, 1) == ADF_LIMIT);
    CHECK(leval(z, x, ADF_REAL_PREC_MAX, 0) == ADF_OK && acb_is_one(z));
    CHECK(aeval(z, a, inf, ADF_REAL_PREC_MAX+1, 0) == ADF_LIMIT);
    place_of(&v, 7); CHECK(aeval(z, a, v, ADF_REAL_PREC_MAX+1, 1) == ADF_LIMIT);
    /* An exact phase with a denominator of 2^64 bits at the largest 64-bit prime: (1/p^(10^6)) and the ball
       O(p^-(2^40)) around 0. The powers are NOT formed (a formed power would be 2^46 bits: no return). */
    place_of(&v, UWORD(18446744073709551557));
    adf_lball_clear(x); adf_lball_init(x);
    x->p = UWORD(18446744073709551557); x->exact = 1; x->v = -1000000; x->N = 0; fmpq_one(x->u);
    CHECK(adf_lball_is_canonical(x));
    CHECK(lphase(th, x) == ADF_LIMIT); CHECK(leval(z, x, 53, 0) == ADF_LIMIT);
    CHECK(leval(z, x, 53, 1) == ADF_LIMIT);
    x->exact = 0; x->v = 0; x->N = -(WORD(1) << 40); fmpq_zero(x->u);
    CHECK(adf_lball_is_canonical(x));
    CHECK(lphase(th, x) == ADF_LIMIT); CHECK(leval(z, x, 53, 0) == ADF_LIMIT);
    CHECK(leval(z, x, 53, 1) == ADF_LIMIT);                 /* LIMIT before strict's NOT_DETERMINED */
    x->v = -(WORD(1) << 40) - 1; x->N = -(WORD(1) << 40); fmpq_one(x->u);
    CHECK(lphase(th, x) == ADF_LIMIT);
    /* lball's exponent bound (|v| above ADF_LBALL_EXP_MAX): LIMIT. */
    x->exact = 1; x->v = -ADF_LBALL_EXP_MAX-1; x->N = 0;
    CHECK(lphase(th, x) == ADF_LIMIT && leval(z, x, 53, 0) == ADF_LIMIT);
    x->v = ADF_LBALL_EXP_MAX+1; CHECK(lphase(th, x) == ADF_LIMIT);
    x->v = ADF_LBALL_EXP_MAX; CHECK(lphase(th, x) == ADF_OK && fmpq_is_zero(th));    /* v >= 0: no power */
    CHECK(leval(z, x, 53, 0) == ADF_OK && acb_is_one(z));
    /* The bit boundary at p = 2: 2^(cap-1) has cap bits (admitted by the getter), 2^cap is LIMIT. At p = 3
       3^(3cap/4) has more than cap bits although k (bits(p)-1)+1 <= cap: formed and then rejected. */
    adf_lball_clear(x); adf_lball_init(x); x->p = 2; x->exact = 1; x->N = 0; fmpq_one(x->u);
    x->v = -(ADF_QCLASS_BITS_MAX-1); CHECK(adf_lball_is_canonical(x));
    CHECK(lphase(th, x) == ADF_OK && fmpz_bits(fmpq_denref(th)) == ADF_QCLASS_BITS_MAX);
    x->v = -ADF_QCLASS_BITS_MAX; CHECK(lphase(th, x) == ADF_LIMIT);
    x->p = 3; x->v = -(3*ADF_QCLASS_BITS_MAX/4); CHECK(lphase(th, x) == ADF_LIMIT);  /* 1.19 cap bits */
    x->v = -(ADF_QCLASS_BITS_MAX/2);                                      /* 0.79 cap bits */
    CHECK(lphase(th, x) == ADF_OK && fmpz_bits(fmpq_denref(th)) < ADF_QCLASS_BITS_MAX);
    /* Place calls: exact bounds at infinity and at a prime, with where = v; strict ND with where = v. */
    arf_one(arb_midref(a->inf)); arf_mul_2exp_si(arb_midref(a->inf), arb_midref(a->inf), ADF_QCLASS_EXP_MAX);
    CHECK(aeval(z, a, inf, 53, 0) == ADF_LIMIT);
    place_of(&v, 5); CHECK(aeval(z, a, v, 53, 0) == ADF_OK && acb_is_one(z));      /* real part not read */
    arb_zero(a->inf);
    fmpq_one(c->q); fmpz_mul_2exp(fmpq_denref(c->q), fmpq_denref(c->q), ADF_QCLASS_BITS_MAX);
    adf_fball_set_rat(&a->fin, c);
    place_of(&v, 2); CHECK(aeval(z, a, v, 53, 0) == ADF_LIMIT);
    CHECK(aeval(z, a, inf, 53, 0) == ADF_OK && acb_is_one(z));                    /* finite part not read */
    fmpq_set_si(c->q, 1, 3); fmpq_set_si(N->q, 1, 2); CHECK(adf_fball_set_center_radius(&a->fin, c, N) == ADF_OK);
    CHECK(aeval(z, a, v, 53, 1) == ADF_NOT_DETERMINED);
    CHECK(aeval(z, a, v, 53, 0) == ADF_OK && arb_is_zero(acb_imagref(z)));
    place_of(&v, 3);
    CHECK(aeval(z, a, v, 53, 1) == ADF_OK && !acb_is_exact(z));   /* E(1/3): v_3(1/2) = 0 */
    /* At infinity strict permits real uncertainty: E(-[0.25 - 1/8, 0.25 + 1/8]). */
    arb_set_d(a->inf, 0.25); mag_set_ui_2exp_si(arb_radref(a->inf), 1, -3);
    CHECK(aeval(z, a, inf, 53, 1) == ADF_OK && arb_is_negative(acb_imagref(z)));     /* sign of E(-1/4) = -i */
    /* The certificate at the cap: a noncardinal local phase fails numerically, cardinal ones are exact. */
    CHECK(aeval(z, a, v, ADF_REAL_PREC_MAX, 0) == ADF_NOT_DETERMINED);
#ifndef ADF_CHECK_INVARIANTS
    arb_indeterminate(a->inf);
    CHECK(aeval(z, a, v, 53, 0) == ADF_DOMAIN); CHECK(aeval(z, a, inf, 53, 1) == ADF_DOMAIN);
    CHECK(aeval(z, a, v, ADF_REAL_PREC_MAX+1, 0) == ADF_LIMIT);
#endif
    adf_lball_clear(x); adf_adele_clear(a); fmpq_clear(th); acb_clear(z); adf_rat_clear(c); adf_rat_clear(N);
}
#ifdef ADF_CHECK_INVARIANTS
/* A forged lball (p = 4), a non-finite adele and a forged place handle abort under INV. */
static void inv(void)
{
    for (int k = 0; k < 6; k++) {
        pid_t pid = fork(); CHECK(pid >= 0);
        if (!pid) {
            adf_lball_t x; adf_adele_t a; acb_t z; fmpq_t th; adf_place_t v = adf_place_inf();
            if (!freopen("/dev/null", "w", stderr)) _exit(2);
            adf_lball_init(x); adf_adele_init(a); acb_init(z); fmpq_init(th); x->p = 4;
            if (k == 0) adf_lball_psi_tate(z, x, 64);
            if (k == 1) adf_lball_psi_tate_strict(z, x, 64);
            if (k == 2) adf_lball_psi_tate_phase(th, x);
            if (k >= 3) { memset(&v, 0, sizeof(v)); v.opaque = 4; }
            if (k == 3) adf_adele_psi_tate_at(z, NULL, a, v, 64);
            if (k == 4) adf_adele_psi_tate_strict_at(z, NULL, a, v, 64);
            if (k == 5) { arb_indeterminate(a->inf); adf_adele_psi_tate_at(z, NULL, a, adf_place_inf(), 64); }
            /* Reached only if the entry check is missing. The clears are for tools/memcheck. */
            x->p = 2; adf_lball_clear(x); adf_adele_clear(a); acb_clear(z); fmpq_clear(th);
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
    if (!strcmp(s, "all") || !strcmp(s, "witnesses")) witnesses();
    if (!strcmp(s, "all") || !strcmp(s, "limits")) limits();
    if (!strcmp(s, "all") || !strcmp(s, "golden")) golden();
    if (!strcmp(s, "all") || !strcmp(s, "local")) local_vectors();
    if (!strcmp(s, "all") || !strcmp(s, "place")) place_vectors();
#ifdef ADF_CHECK_INVARIANTS
    if (!strcmp(s, "all")) inv();
#endif
    printf("psi_local: %lu checks, %lu local records, %lu place evaluations, %lu hull coordinates (%s)\n",
           checks, nlocal, nplaces, nhulls, s);
    flint_cleanup(); return 0;
}
