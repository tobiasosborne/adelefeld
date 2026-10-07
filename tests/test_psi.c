/* Slice 3.2-a; exact oracle vectors use proto/quotient3_checks.py psi and Q4 distances.
   Hand sign derivation, BEFORE implementation: conventions 6.1:844 has psi_inf(t)=E(-t),
   psi_p(t)=E(fp_p(t)); refs/src/tate-poonen/notes.txt:693-700 fixes these opposite signs.
   At (0 ; 1/3), fp_3(1/3)=1/3; all other fp_p are 0 and E(-0)=1.
   Thus psi=E(1/3)=-1/2+i sqrt(3)/2, with POSITIVE imaginary part.
   Lines 733-740 discuss the Fourier transform's conjugate, not a reversal of psi.
   No same-type output/input alias is permitted by any of the five signatures. */
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
static void jq(fmpq_t q, const jsonl_value *v)
{
    jsonl_error_t e; const char *s = jsonl_string(v, NULL, &e);
    CHECK(s != NULL); qread(q, s);
}
static void setball(adf_adele_t x, const fmpq_t m, const fmpq_t r,
                    const fmpq_t a, const fmpq_t N)
{
    adf_rat_t c, n;
    adf_rat_init(c); adf_rat_init(n);
    fmpq_set(c->q, a); fmpq_set(n->q, N);
    CHECK(adf_fball_set_center_radius(&x->fin, c, n) == ADF_OK);
    arb_set_fmpq(x->inf, m, FLINT_MAX(4096, fmpz_bits(fmpq_numref(m)) + 64));
    CHECK(arb_is_exact(x->inf));
    mag_set_ui_2exp_si(arb_radref(x->inf), fmpz_get_ui(fmpq_numref(r)),
                       1-fmpz_bits(fmpq_denref(r)));
    CHECK(adf_adele_is_canonical(x));
    adf_rat_clear(c); adf_rat_clear(n);
}
static void seed_phase(fmpq_t q)
{ fmpq_set_si(q, 19, 7); fmpq_mul_2exp(q, q, 130); }
static void seed_complex(acb_t z)
{
    acb_set_si(z, 17);
    arf_mul_2exp_si(arb_midref(acb_realref(z)), arb_midref(acb_realref(z)), 130);
    arf_add_ui(arb_midref(acb_realref(z)), arb_midref(acb_realref(z)), 1,
               ARF_PREC_EXACT, ARF_RND_NEAR);
    arb_set_si(acb_imagref(z), -23); mag_set_ui_2exp_si(arb_radref(acb_realref(z)), 1, -4);
}
static int fphase(fmpq_t q, const adf_fball_t x)
{
    seed_phase(q);
    fmpq_t saved; fmpq_init(saved); fmpq_set(saved, q);
    fmpq qbytes; memcpy(&qbytes, q, sizeof(qbytes));
    int st = adf_fball_psi_tate_phase(q, x);
    if (st) { CHECK(!memcmp(&qbytes, q, sizeof(qbytes))); CHECK(fmpq_equal(q, saved)); }
    else CHECK(fmpq_is_canonical(q) && fmpq_sgn(q) >= 0 && fmpq_cmp_si(q, 1) < 0);
    fmpq_clear(saved);
    return st;
}
static int aphase(fmpq_t q, const adf_adele_t x)
{
    seed_phase(q);
    fmpq_t saved; fmpq_init(saved); fmpq_set(saved, q);
    fmpq qbytes; memcpy(&qbytes, q, sizeof(qbytes));
    int st = adf_adele_psi_tate_phase(q, x);
    if (st) { CHECK(!memcmp(&qbytes, q, sizeof(qbytes))); CHECK(fmpq_equal(q, saved)); }
    else CHECK(fmpq_is_canonical(q) && fmpq_sgn(q) >= 0 && fmpq_cmp_si(q, 1) < 0);
    fmpq_clear(saved);
    return st;
}
static int eval(acb_t z, const adf_adele_t x, slong p, int strict)
{
    seed_complex(z);
    acb_t saved; acb_init(saved); acb_set(saved, z);
    acb_struct bytes; memcpy(&bytes, z, sizeof(bytes));
    int st = strict ? adf_adele_psi_tate_strict(z, x, p) : adf_adele_psi_tate(z, x, p);
    if (st) { CHECK(!memcmp(&bytes, z, sizeof(bytes))); CHECK(acb_equal(z, saved)); }
    acb_clear(saved);
    return st;
}
static int peval(acb_t z, const fmpq_t q, slong p)
{
    seed_complex(z);
    acb_t saved; acb_init(saved); acb_set(saved, z);
    acb_struct bytes; memcpy(&bytes, z, sizeof(bytes));
    int st = adf_phase_get_acb(z, q, p);
    if (st) { CHECK(!memcmp(&bytes, z, sizeof(bytes))); CHECK(acb_equal(z, saved)); }
    acb_clear(saved); return st;
}
/* Independently evaluate exact rational phases with FLINT at 512 bits. Cardinal values
   are exact. All ordinary containment tests use much lower output precision. */
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
/* Assert BOTH containment and the section 3.3 bound for each endpoint:
   4 epsilon + 2^-28 (W/2 + 2 epsilon). Reference uncertainty is <2^-500.
   A unit square for E3 fails the imaginary width assertion at p=20,53,128. */
static void hullcheck(const arb_t z, const arb_t low, const arb_t high, slong p)
{
    arf_t lo, hi; arb_t l, h, bound, eps, tmp;
    arf_init(lo); arf_init(hi);
    arb_init(l); arb_init(h); arb_init(bound); arb_init(eps); arb_init(tmp);
    arb_get_interval_arf(lo, hi, z, ARF_PREC_EXACT);
    arb_set_arf(l, lo); arb_set_arf(h, hi);
    CHECK(arb_le(l, low)); CHECK(arb_ge(h, high));
    arb_one(eps); arb_mul_2exp_si(eps, eps, -p);
    arb_sub(bound, high, low, 512); arb_mul_2exp_si(bound, bound, -1);
    arb_mul_2exp_si(tmp, eps, 1); arb_add(bound, bound, tmp, 512);
    arb_mul_2exp_si(bound, bound, -28);
    arb_mul_2exp_si(tmp, eps, 2); arb_add(bound, bound, tmp, 512);
    arb_sub(tmp, low, bound, 512); CHECK(arb_ge(l, tmp));
    arb_add(tmp, high, bound, 512); CHECK(arb_le(h, tmp));
    arf_clear(lo); arf_clear(hi);
    arb_clear(l); arb_clear(h); arb_clear(bound); arb_clear(eps); arb_clear(tmp);
}
static void golden(void)
{
    golden_file *f; golden_error_t e; adf_fball_t x; fmpq_t q, expected;
    acb_t z, w; adf_adele_t a;
    adf_fball_init(x); adf_adele_init(a); fmpq_init(q); fmpq_init(expected);
    acb_init(z); acb_init(w);
    CHECK(golden_open("tests/golden/psi_phases.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &e));
    CHECK(golden_count(f) == 17);
    for (size_t i = 0; i < golden_count(f); i++) {
        const golden_record *r = golden_record_at(f, i);
        adf_fball_struct bytes; memcpy(&bytes, x, sizeof(bytes));
        int st = adf_fball_set_str(x, r->input, r->input_len, NULL);
        if (r->is_status) {
            CHECK(!strcmp(adf_status_str(st), r->status));
            CHECK(!memcmp(&bytes, x, sizeof(bytes))); continue;
        }
        CHECK(st == ADF_OK);
        char *text = malloc(r->expected_len+1); CHECK(text != NULL);
        memcpy(text, r->expected, r->expected_len+1);
        char *token = strtok(text, " "); int n = 0;
        while (token) { n++; token = strtok(NULL, " "); }
        memcpy(text, r->expected, r->expected_len+1);
        fmpq_set_si(q, 19, 7);
        CHECK(fphase(q, x) == (n == 1 ? ADF_OK : ADF_NOT_DETERMINED));
        if (n == 1) { qread(expected, text); CHECK(fmpq_equal(q, expected)); }
        adf_fball_set(&a->fin, x); arb_zero(a->inf);
        CHECK(eval(z, a, 128, 0) == ADF_OK);
        CHECK(eval(w, a, 128, 1) == (n == 1 ? ADF_OK : ADF_NOT_DETERMINED));
        token = strtok(text, " ");
        while (token) { qread(expected, token); reference(w, expected); CHECK(acb_contains(z, w));
                       token = strtok(NULL, " "); }
        free(text);
    }
    golden_close(f); adf_fball_clear(x); adf_adele_clear(a);
    fmpq_clear(q); fmpq_clear(expected); acb_clear(z); acb_clear(w);
}
static void vectors(void)
{
    jsonl_file *f; jsonl_error_t e; adf_adele_t x;
    fmpq_t m, r, a, N, q, expected, d; acb_t z, strict, point; arb_t lo, hi;
    adf_adele_init(x); fmpq_init(m); fmpq_init(r); fmpq_init(a); fmpq_init(N);
    fmpq_init(q); fmpq_init(expected); fmpq_init(d);
    acb_init(z); acb_init(strict); acb_init(point); arb_init(lo); arb_init(hi);
    CHECK(jsonl_open("tests/ref/vectors/q-slice3/psi.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 80);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i), *phase = field(v, "phase"), *dist = field(v, "dist");
        jq(m, field(v, "m")); jq(r, field(v, "r")); jq(a, field(v, "a")); jq(N, field(v, "N"));
        setball(x, m, r, a, N);
        /* Check that the fixture describes the STORED radius, not a rounded wish. */
        arf_t rr; arf_init(rr); arf_set_mag(rr, arb_radref(x->inf)); arf_get_fmpq(d, rr);
        CHECK(fmpq_equal(d, r)); arf_clear(rr);
        fmpq_set_si(q, 19, 7);
        int singleton = !jsonl_is(phase, JSONL_NULL);
        CHECK(aphase(q, x) == (singleton ? ADF_OK : ADF_NOT_DETERMINED));
        if (singleton) { jq(expected, phase); CHECK(fmpq_equal(q, expected));
            CHECK(fmpq_is_canonical(q) && fmpq_sgn(q) >= 0 && fmpq_cmp_si(q, 1) < 0); }
        fmpq_set_si(q, 19, 7);
        CHECK(fphase(q, &x->fin) == (fmpz_is_one(fmpq_denref(N)) ? ADF_OK : ADF_NOT_DETERMINED));
        if (fmpz_is_one(fmpq_denref(N))) { fmpq_set(expected, a); mod1(expected);
                                                       CHECK(fmpq_equal(q, expected)); }
        const slong precisions[] = {2, 20, 53, 128};
        for (size_t j = 0; j < 4; j++) {
            slong p = precisions[j];
            CHECK(eval(z, x, p, 0) == ADF_OK && acb_is_finite(z));
            CHECK(eval(strict, x, p, 1) ==
                  (fmpz_is_one(fmpq_denref(N)) ? ADF_OK : ADF_NOT_DETERMINED));
            if (fmpz_is_one(fmpq_denref(N))) CHECK(acb_equal(z, strict));
            jq(d, jsonl_at(dist, 1, &e)); coordinate(lo, d, 1);
            jq(d, jsonl_at(dist, 0, &e)); coordinate(hi, d, 0);
            hullcheck(acb_realref(z), lo, hi, p);
            jq(d, jsonl_at(dist, 3, &e)); coordinate(lo, d, 1);
            jq(d, jsonl_at(dist, 2, &e)); coordinate(hi, d, 0);
            hullcheck(acb_imagref(z), lo, hi, p);
            const jsonl_value *points = field(v, "points"); CHECK(jsonl_size(points) == 40);
            for (size_t k = 0; k < 40; k++) {
                const jsonl_value *pt = jsonl_at(points, k, &e);
                jq(q, field(pt, "phase")); jq(m, field(pt, "t")); jq(a, field(pt, "a"));
                fmpq_sub(expected, a, m); mod1(expected); CHECK(fmpq_equal(q, expected));
                if (j == 0) {
                    arf_t l, h; fmpq_t endpoint;
                    arf_init(l); arf_init(h); fmpq_init(endpoint);
                    arb_get_interval_arf(l, h, x->inf, ARF_PREC_EXACT);
                    arf_get_fmpq(endpoint, l); CHECK(fmpq_cmp(endpoint, m) <= 0);
                    arf_get_fmpq(endpoint, h); CHECK(fmpq_cmp(m, endpoint) <= 0);
                    arf_clear(l); arf_clear(h); fmpq_clear(endpoint);
                    adf_rat_t c; adf_rat_init(c); fmpq_set(c->q, a);
                    CHECK(adf_fball_contains_rat(&x->fin, c)); adf_rat_clear(c);
                }
                reference(point, q); CHECK(acb_contains(z, point));
            }
        }
    }
    jsonl_close(f); adf_adele_clear(x);
    fmpq_clear(m); fmpq_clear(r); fmpq_clear(a); fmpq_clear(N); fmpq_clear(q);
    fmpq_clear(expected); fmpq_clear(d);
    acb_clear(z); acb_clear(strict); acb_clear(point); arb_clear(lo); arb_clear(hi);
}
static void exact_and_additivity(void)
{
    adf_adele_t x, y, sum; fmpq_t m, r, a, N, q, u, v; acb_t z, w, product, ref;
    adf_adele_init(x); adf_adele_init(y); adf_adele_init(sum);
    fmpq_init(m); fmpq_init(r); fmpq_init(a); fmpq_init(N);
    fmpq_init(q); fmpq_init(u); fmpq_init(v);
    acb_init(z); acb_init(w); acb_init(product); acb_init(ref);
    /* 500 exact diagonal rationals: dyadic to permit exact arb storage. */
    for (slong i = 0; i < 500; i++) {
        fmpq_set_si(m, 7*i-1729, UWORD(1) << (i%20)); fmpq_set(a, m);
        setball(x, m, r, a, N); CHECK(aphase(q, x) == ADF_OK && fmpq_is_zero(q));
        CHECK(eval(z, x, 53, 0) == ADF_OK && acb_is_one(z));
    }
    for (slong i = 0; i < 200; i++) {
        fmpq_zero(r); fmpq_zero(N);
        fmpq_set_si(m, i-71, 64); fmpq_set_si(a, 3*i-29, 360); setball(x, m, r, a, N);
        fmpq_set_si(m, 2*i-97, 128); fmpq_set_si(a, 17-i, 12); setball(y, m, r, a, N);
        adf_adele_add(sum, x, y, 256);
        CHECK(aphase(q, x) == ADF_OK); CHECK(aphase(u, y) == ADF_OK);
        CHECK(aphase(v, sum) == ADF_OK); fmpq_add(q, q, u); mod1(q); CHECK(fmpq_equal(q, v));
        CHECK(eval(z, x, 128, 0) == ADF_OK); CHECK(eval(w, y, 128, 0) == ADF_OK);
        acb_mul(product, z, w, 256); reference(ref, v); CHECK(acb_contains(product, ref));
        /* Independent ball sums: product enclosure contains sampled products, and
           the character of the sum contains each product, even for fractional N. */
        mag_set_ui_2exp_si(arb_radref(x->inf), 1, -7);
        mag_set_ui_2exp_si(arb_radref(y->inf), 1, -6);
        fmpq_set_si(N, 2, 3); fmpq_set_si(a, i, 12);
        adf_rat_t c, n; adf_rat_init(c); adf_rat_init(n);
        fmpq_set(c->q, a); fmpq_set(n->q, N);
        CHECK(adf_fball_set_center_radius(&x->fin, c, n) == ADF_OK);
        fmpq_set_si(n->q, 3, 4); CHECK(adf_fball_set_center_radius(&y->fin, c, n) == ADF_OK);
        adf_rat_clear(c); adf_rat_clear(n);
        adf_adele_add(sum, x, y, 256);
        CHECK(eval(z, x, 128, 0) == ADF_OK); CHECK(eval(w, y, 128, 0) == ADF_OK);
        acb_mul(product, z, w, 256); CHECK(eval(z, sum, 128, 0) == ADF_OK);
        for (int k = 0; k < 40; k++) {
            arf_get_fmpq(m, arb_midref(x->inf)); arf_get_fmpq(r, arb_midref(y->inf));
            fmpq_add(m, m, r); fmpq_set_si(r, 3*(2*k-39), 128*39); fmpq_add(m, m, r);
            fmpq_set_si(q, 2*i, 12); fmpq_set_si(u, 17*k, 12); fmpq_add(q, q, u);
            fmpq_sub(q, q, m); mod1(q); reference(ref, q);
            CHECK(acb_contains(z, ref)); CHECK(acb_contains(product, ref));
        }
    }
    /* Non-dyadic diagonal rational: real conversion has radius; no exact phase getter. */
    qread(a, "1/3"); arb_set_fmpq(x->inf, a, 128);
    adf_rat_t c; adf_rat_init(c); fmpq_set(c->q, a); adf_fball_set_rat(&x->fin, c);
    CHECK(!arb_is_exact(x->inf)); CHECK(aphase(q, x) == ADF_NOT_DETERMINED);
    acb_one(w); CHECK(eval(z, x, 128, 0) == ADF_OK && acb_contains(z, w));
    adf_rat_clear(c);
    adf_adele_clear(x); adf_adele_clear(y); adf_adele_clear(sum);
    fmpq_clear(m); fmpq_clear(r); fmpq_clear(a); fmpq_clear(N);
    fmpq_clear(q); fmpq_clear(u); fmpq_clear(v);
    acb_clear(z); acb_clear(w); acb_clear(product); acb_clear(ref);
}
static void phase_eval(void)
{
    fmpq_t q; acb_t z, w; fmpq_init(q); acb_init(z); acb_init(w);
    for (slong i = 0; i < 100; i++) {
        fmpq_set_si(q, i, 100); reference(w, q);
        const slong precisions[] = {2, 20, 53, 128};
        for (size_t j = 0; j < 4; j++) {
            slong p = precisions[j];
            CHECK(peval(z, q, p) == ADF_OK); CHECK(acb_contains(z, w));
            hullcheck(acb_realref(z), acb_realref(w), acb_realref(w), p);
            hullcheck(acb_imagref(z), acb_imagref(w), acb_imagref(w), p);
        }
    }
    for (slong i = 0; i < 4; i++) {
        fmpq_set_si(q, i, 4);
        CHECK(peval(z, q, 2) == ADF_OK && acb_is_exact(z));
        reference(w, q); CHECK(acb_equal(z, w));
        CHECK(peval(z, q, ADF_REAL_PREC_MAX) == ADF_OK && acb_equal(z, w));
    }
    acb_struct bytes; memcpy(&bytes, z, sizeof(bytes));
    CHECK(adf_phase_get_acb(z, q, ADF_REAL_PREC_MAX+1) == ADF_LIMIT);
    CHECK(!memcmp(&bytes, z, sizeof(bytes)));
    fmpq_set_si(q, -1, 4); CHECK(peval(z, q, ADF_REAL_PREC_MAX+1) == ADF_LIMIT);
    fmpz_set_ui(fmpq_numref(q), 2); fmpz_set_ui(fmpq_denref(q), 4);
    CHECK(peval(z, q, ADF_REAL_PREC_MAX+1) == ADF_LIMIT);
    fmpq_zero(q); CHECK(peval(z, q, -100) == ADF_OK && acb_is_one(z));
    /* At the working cap, 1/3's sine enclosure cannot certify width <=2^-p.
       This exercises numerical NOT_DETERMINED, with a nonzero allocated sentinel. */
    qread(q, "1/3"); acb_set_si(z, 17);
    CHECK(peval(z, q, ADF_REAL_PREC_MAX) == ADF_NOT_DETERMINED);
    fmpq_clear(q); acb_clear(z); acb_clear(w);
}
static void edges(void)
{
    adf_adele_t x; fmpq_t q; acb_t z; adf_rat_t c, N;
    adf_adele_init(x); fmpq_init(q); acb_init(z); adf_rat_init(c); adf_rat_init(N);
    fmpq_set_si(q, 19, 7); acb_set_si(z, 17);
    CHECK(eval(z, x, ADF_REAL_PREC_MAX, 0) == ADF_OK && acb_is_one(z));
    CHECK(eval(z, x, ADF_REAL_PREC_MAX, 1) == ADF_OK && acb_is_one(z));
    CHECK(eval(z, x, ADF_REAL_PREC_MAX+1, 0) == ADF_LIMIT);
    CHECK(eval(z, x, ADF_REAL_PREC_MAX+1, 1) == ADF_LIMIT);
    /* The exponent boundary itself is admitted. ARF's exponent of 2^k is k+1. */
    arf_one(arb_midref(x->inf));
    arf_mul_2exp_si(arb_midref(x->inf), arb_midref(x->inf), ADF_QCLASS_EXP_MAX-1);
    CHECK(aphase(q, x) == ADF_OK && fmpq_is_zero(q));
    CHECK(eval(z, x, 53, 0) == ADF_OK && acb_is_one(z));
    arf_one(arb_midref(x->inf));
    arf_mul_2exp_si(arb_midref(x->inf), arb_midref(x->inf), -ADF_QCLASS_EXP_MAX-1);
    CHECK(aphase(q, x) == ADF_OK && !fmpq_is_zero(q));
    CHECK(eval(z, x, 53, 0) == ADF_OK && acb_is_finite(z));
    /* Exact-work limits are checked before ambiguity. */
    arf_one(arb_midref(x->inf));
    arf_mul_2exp_si(arb_midref(x->inf), arb_midref(x->inf), ADF_QCLASS_EXP_MAX);
    CHECK(aphase(q, x) == ADF_LIMIT); CHECK(eval(z, x, 53, 0) == ADF_LIMIT);
    CHECK(eval(z, x, 53, 1) == ADF_LIMIT);
    arf_set_si(arb_midref(x->inf), 0);
    mag_set_ui_2exp_si(arb_radref(x->inf), 1, -ADF_QCLASS_EXP_MAX-2);
    CHECK(aphase(q, x) == ADF_LIMIT); CHECK(eval(z, x, 53, 1) == ADF_LIMIT);
    arb_zero(x->inf);
    fmpz_one(fmpq_denref(c->q)); fmpz_mul_2exp(fmpq_denref(c->q), fmpq_denref(c->q),
                                           ADF_QCLASS_BITS_MAX);
    fmpz_one(fmpq_numref(c->q)); adf_fball_set_rat(&x->fin, c);
    CHECK(fphase(q, &x->fin) == ADF_LIMIT); CHECK(aphase(q, x) == ADF_LIMIT);
    CHECK(eval(z, x, 53, 0) == ADF_LIMIT); CHECK(eval(z, x, 53, 1) == ADF_LIMIT);
    CHECK(peval(z, c->q, 53) == ADF_LIMIT);
    /* An over-limit phase still loses to the precision cap before any evaluation. */
    CHECK(peval(z, c->q, ADF_REAL_PREC_MAX+1) == ADF_LIMIT);
    /* A finite numerator at the bit boundary is admitted and reduced before trig. */
    fmpq_one(c->q);
    fmpz_mul_2exp(fmpq_numref(c->q), fmpq_numref(c->q), ADF_QCLASS_BITS_MAX-1);
    adf_fball_set_rat(&x->fin, c);
    CHECK(fphase(q, &x->fin) == ADF_OK && fmpq_is_zero(q));
    CHECK(aphase(q, x) == ADF_OK && fmpq_is_zero(q));
    CHECK(eval(z, x, 53, 0) == ADF_OK && acb_is_one(z));
    fmpz_mul_2exp(fmpq_numref(c->q), fmpq_numref(c->q), 1);
    adf_fball_set_rat(&x->fin, c);
    CHECK(fphase(q, &x->fin) == ADF_LIMIT); CHECK(eval(z, x, 53, 1) == ADF_LIMIT);
    /* Projected doubling has its own LIMIT even when theta itself fits. */
    fmpz_set(fmpq_denref(c->q), fmpq_numref(c->q));
    fmpz_sub_ui(fmpq_denref(c->q), fmpq_denref(c->q), 1);
    fmpz_sub_ui(fmpq_numref(c->q), fmpq_denref(c->q), 2);
    CHECK(fmpq_is_canonical(c->q)); CHECK(peval(z, c->q, 53) == ADF_LIMIT);
    /* A noncardinal input reaches numerical certificate failure at the cap. */
    fmpq_set_si(c->q, 1, 3); adf_fball_set_rat(&x->fin, c);
    CHECK(eval(z, x, ADF_REAL_PREC_MAX, 0) == ADF_NOT_DETERMINED);
    CHECK(eval(z, x, ADF_REAL_PREC_MAX, 1) == ADF_NOT_DETERMINED);
    /* The largest mantissa allowed by the projected dyadic bit bound is admitted. */
    adf_fball_zero(&x->fin); fmpq_one(c->q);
    fmpz_mul_2exp(fmpq_numref(c->q), fmpq_numref(c->q), ADF_QCLASS_BITS_MAX);
    fmpz_sub_ui(fmpq_numref(c->q), fmpq_numref(c->q), 1);
    fmpz_mul_2exp(fmpq_denref(c->q), fmpq_denref(c->q), ADF_QCLASS_BITS_MAX-1);
    arb_set_fmpq(x->inf, c->q, ADF_QCLASS_BITS_MAX);
    CHECK(arb_is_exact(x->inf)); CHECK(arf_bits(arb_midref(x->inf)) == ADF_QCLASS_BITS_MAX);
    fmpq_one(N->q);
    fmpz_mul_2exp(fmpq_denref(N->q), fmpq_denref(N->q), ADF_QCLASS_BITS_MAX-1);
    CHECK(aphase(q, x) == ADF_OK && fmpq_equal(q, N->q));
    /* Raw H, independently of A and d, has an inclusive bit boundary. */
    arb_zero(x->inf); fmpq_one(c->q); fmpq_one(N->q);
    fmpz_mul_2exp(fmpq_numref(N->q), fmpq_numref(N->q), ADF_QCLASS_BITS_MAX-1);
    CHECK(adf_fball_set_center_radius(&x->fin, c, N) == ADF_OK);
    CHECK(fphase(q, &x->fin) == ADF_OK && fmpq_is_zero(q));
    CHECK(eval(z, x, 53, 1) == ADF_OK && acb_is_one(z));
    fmpz_mul_2exp(fmpq_numref(N->q), fmpq_numref(N->q), 1);
    CHECK(adf_fball_set_center_radius(&x->fin, c, N) == ADF_OK);
    CHECK(fphase(q, &x->fin) == ADF_LIMIT); CHECK(eval(z, x, 53, 1) == ADF_LIMIT);
    /* A projected numerator exactly at the carry bound must be admitted.
       For a=1/3 and m=(2^k-1)/2^k, yn+xd+1=k+2+1; choose k=cap-3. */
    fmpq_set_si(c->q, 1, 3); adf_fball_set_rat(&x->fin, c);
    fmpq_one(N->q);
    fmpz_mul_2exp(fmpq_denref(N->q), fmpq_denref(N->q), ADF_QCLASS_BITS_MAX-3);
    fmpz_sub_ui(fmpq_numref(N->q), fmpq_denref(N->q), 1);
    arb_set_fmpq(x->inf, N->q, ADF_QCLASS_BITS_MAX);
    CHECK(arb_is_exact(x->inf)); fmpq_sub(N->q, c->q, N->q); mod1(N->q);
    CHECK(aphase(q, x) == ADF_OK && fmpq_equal(q, N->q));
    /* RU30's successor crosses a binade here: the exact hull half-width lies
       strictly between 1-2^-29 and 1-2^-30. The mutated mantissa rollover doubles it. */
    adf_fball_zero(&x->fin); arb_zero(x->inf);
    mag_set_ui_2exp_si(arb_radref(x->inf), 131069, -18);
    CHECK(eval(z, x, 128, 0) == ADF_OK);
    arb_t lo, hi; arb_init(lo); arb_init(hi); fmpq_set_si(c->q, 3, 262144);
    coordinate(lo, c->q, 1); arb_one(hi); hullcheck(acb_realref(z), lo, hi, 128);
    arb_clear(lo); arb_clear(hi); arb_zero(x->inf);
    /* Local backend with raw d > 1 and integral canonical radius. */
    ulong blocks[] = {4, 3}; adf_modctx_struct *ctx = NULL;
    CHECK(adf_modctx_new_blocks(&ctx, blocks, 2) == ADF_OK);
    fmpq_set_si(c->q, 1, 1); fmpq_set_si(N->q, 3, 1);
    CHECK(adf_fball_set_center_radius(&x->fin, c, N) == ADF_OK);
    CHECK(adf_fball_set_local(&x->fin, &x->fin, ctx) == ADF_OK);
    CHECK(!fmpz_is_one(x->fin.d)); CHECK(fphase(q, &x->fin) == ADF_OK && fmpq_is_zero(q));
    CHECK(eval(z, x, 128, 1) == ADF_OK && acb_is_one(z));
    adf_adele_clear(x); adf_modctx_free(ctx); adf_adele_init(x);
    arb_indeterminate(x->inf);
    CHECK(eval(z, x, ADF_REAL_PREC_MAX+1, 0) == ADF_LIMIT);
    CHECK(eval(z, x, ADF_REAL_PREC_MAX+1, 1) == ADF_LIMIT);
    arb_zero(x->inf);
#ifndef ADF_CHECK_INVARIANTS
    arb_indeterminate(x->inf);
    CHECK(eval(z, x, ADF_REAL_PREC_MAX+1, 0) == ADF_LIMIT);
    CHECK(eval(z, x, 128, 0) == ADF_DOMAIN); CHECK(eval(z, x, 128, 1) == ADF_DOMAIN);
    CHECK(aphase(q, x) == ADF_DOMAIN);
    arf_pos_inf(arb_midref(x->inf)); mag_zero(arb_radref(x->inf));
    CHECK(eval(z, x, 128, 0) == ADF_DOMAIN); CHECK(aphase(q, x) == ADF_DOMAIN);
    arf_neg_inf(arb_midref(x->inf));
    CHECK(eval(z, x, 128, 1) == ADF_DOMAIN); CHECK(aphase(q, x) == ADF_DOMAIN);
#else
    /* Invalid raw objects violate the canonical precondition; check each debug entry. */
    for (int k = 0; k < 8; k++) {
        pid_t pid = fork(); CHECK(pid >= 0);
        if (!pid) {
            if (!freopen("/dev/null", "w", stderr)) _exit(2);
            if (k < 3) {
                arb_indeterminate(x->inf);
                if (k == 0) adf_adele_psi_tate(z, x, 128);
                if (k == 1) adf_adele_psi_tate_strict(z, x, 128);
                if (k == 2) adf_adele_psi_tate_phase(q, x);
            } else if (k == 3) {
                fmpz_zero(x->fin.d); adf_fball_psi_tate_phase(q, &x->fin);
            } else {
                if (k == 4) fmpq_set_si(q, -1, 4);
                if (k == 5) fmpq_set_si(q, 1, 1);
                if (k == 6) { fmpz_set_ui(fmpq_numref(q), 2); fmpz_set_ui(fmpq_denref(q), 4); }
                if (k == 7) { fmpz_set_ui(fmpq_numref(q), 0); fmpz_set_ui(fmpq_denref(q), 2); }
                adf_phase_get_acb(z, q, 128);
            }
            _exit(0);
        }
        int status; CHECK(waitpid(pid, &status, 0) == pid);
        CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
#endif
    adf_adele_clear(x); fmpq_clear(q); acb_clear(z); adf_rat_clear(c); adf_rat_clear(N);
}
static void stage(const char *s)
{
    adf_adele_t x; fmpq_t q; acb_t z, w;
    adf_adele_init(x); fmpq_init(q); acb_init(z); acb_init(w);
    CHECK(adf_adele_set_str(x, "(0 ; 1/3)", 9, 128, NULL) == ADF_OK);
    if (!strcmp(s, "finite")) {
        CHECK(fphase(q, &x->fin) == ADF_OK); CHECK(fmpq_equal_si(q, 1) == 0);
        CHECK(fmpq_cmp_si(q, 0) > 0 && fmpz_equal_ui(fmpq_denref(q), 3));
    }
    if (!strcmp(s, "adele")) {
        CHECK(aphase(q, x) == ADF_OK); CHECK(fmpz_equal_ui(fmpq_numref(q), 1));
        CHECK(fmpz_equal_ui(fmpq_denref(q), 3));
        mag_set_ui_2exp_si(arb_radref(x->inf), 1, -6);
        CHECK(aphase(q, x) == ADF_NOT_DETERMINED);
    }
    if (!strcmp(s, "default")) {
        qread(q, "1/3"); reference(w, q);
        CHECK(eval(z, x, 128, 0) == ADF_OK && acb_contains(z, w));
        CHECK(arb_is_positive(acb_imagref(z)));
        CHECK(adf_adele_set_str(x, "(0 ; 0 mod 1/2)", 15, 128, NULL) == ADF_OK);
        CHECK(eval(z, x, 128, 0) == ADF_OK && arb_is_zero(acb_imagref(z)));
    }
    if (!strcmp(s, "strict")) {
        CHECK(eval(z, x, 128, 1) == ADF_OK);
        CHECK(adf_adele_set_str(x, "(0 ; 0 mod 1/2)", 15, 128, NULL) == ADF_OK);
        CHECK(eval(z, x, 128, 1) == ADF_NOT_DETERMINED);
    }
    adf_adele_clear(x); fmpq_clear(q); acb_clear(z); acb_clear(w);
}
int main(int argc, char **argv)
{
    const char *s = argc == 2 ? argv[1] : "all";
    if (strcmp(s, "all")) stage(s);
    if (!strcmp(s, "all") || !strcmp(s, "golden")) golden();
    if (!strcmp(s, "all") || !strcmp(s, "vectors")) vectors();
    if (!strcmp(s, "all") || !strcmp(s, "exact")) exact_and_additivity();
    if (!strcmp(s, "all") || !strcmp(s, "phase")) phase_eval();
    if (!strcmp(s, "all") || !strcmp(s, "edges")) edges();
    printf("psi: %lu checks (%s)\n", checks, s); flint_cleanup(); return 0;
}
