/* tests/test_tate.c: slice 5c of docs/api-5.md, the Tate test vector and the global Tate integral in Re(s) > 1
   (include/adelefeld/tate.h: adf_tate_vector, adf_tate_integral; statements in docs/api-5b.md "Slice 5c").
   Contract: docs/api-5.md section 1 (statuses, order of checks, D2), section 3 (:121-165, the two comment blocks),
   section 4 (P15 cutoffs, T3), section 7 (acceptance); docs/SPEC.md 8 (:470-502: FLINT's values completed by
   hand; the width shrinks with precision); analysis P11, P13 (docs/proofs/analysis.md:452-626).
   Oracle: tests/ref/vectors/t5-slice2/tate.jsonl, written by lanes/t5-slice2/gen_vectors.py from
   proto/tate_checks.py: the vectors (f[j] = chi(j) as exact phases, conventions 6.5) for C = 1 and the 17 golden
   characters of tests/golden/gauss.tsv; I_chi(s) at 60 digits (mpmath Hurwitz) and python-flint's L value
   completed by hand with pi^-z Gamma(z) (a second source) for s in 2, 3, 9/8, 3/2 + 14i, 2 + 3i, 10001/10000 and
   two balls (the four corners and the midpoint); the width witness of N-D23 (zeta on [9/8, 5/4]).
   What OK must give: every reference value inside the result, and each coordinate diameter <= 2^-bits
   (containment alone passes a function returning huge balls). Failure: z untouched (bytes and value).
   FLINT's acb_dirichlet_l and the Catalan constant are used here as references only (api-5.md:146-149). */
#include <adelefeld.h>
#include <flint/acb_dirichlet.h>
#include <flint/dirichlet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "support/jsonl.h"
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define VEC "tests/ref/vectors/t5-slice2/tate.jsonl"
#define RP 512                                            /* precision of the reference balls */

static unsigned long checks, values_in, points_in;

/* Not part of the interface (hidden in a shared object, as src/roots.c:49-53): T3 for one Mellin integral,
   val + qerr encloses sum_(n<=N) coef[n] integral_1^R exp(-pi n^2 t/C) t^(z-1) dt; qerr <= target. */
int adf_tate_taylor_piece(acb_t val, arb_t qerr, const acb_t z, acb_srcptr coef, ulong N, int e, ulong C, ulong R,
                          const arf_t target, slong prec);
/* Not part of the interface (hidden): one attempt, with its cutoffs N, R, panel count K, the panel degrees of the
   two sides (deg[0..K), deg[K..2K)) and the work charged. */
int adf_tate_cutoffs(ulong *N, ulong *R, ulong *K, ulong *deg, ulong *work, const adf_char_t chi, const acb_t s,
                     slong bits, slong prec);
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

static double seconds(void) { return (double) clock() / (double) CLOCKS_PER_SEC; }

/* ------------------------------------------------------------------ JSON helpers */
static const jsonl_value *field(const jsonl_value *v, const char *name)
{
    const jsonl_value *out; jsonl_error_t e;
    CHECK(jsonl_field(v, name, &out, &e)); return out;
}
static const jsonl_value *at(const jsonl_value *v, size_t i)
{
    jsonl_error_t e; const jsonl_value *out = jsonl_at(v, i, &e);
    CHECK(out != NULL); return out;
}
static const char *str(const jsonl_value *v)
{
    jsonl_error_t e; size_t n; const char *s = jsonl_string(v, &n, &e);
    CHECK(s != NULL); return s;
}
static long number(const jsonl_value *v)
{
    jsonl_error_t e; const char *s = jsonl_int_text(v, &e);
    CHECK(s != NULL); return strtol(s, NULL, 10);
}
static int is_null(const jsonl_value *v) { jsonl_error_t e; return jsonl_is_null(v, &e); }
static void q_of(fmpq_t q, const char *s) { CHECK(fmpq_set_str(q, s, 10) == 0); fmpq_canonicalise(q); }
static void ball_of(acb_t z, const jsonl_value *v)
{
    CHECK(arb_set_str(acb_realref(z), str(at(v, 0)), RP) == 0);
    CHECK(arb_set_str(acb_imagref(z), str(at(v, 1)), RP) == 0);
}
/* The spectral input: midpoint [re, im] exact rationals (rounded at 2 RP bits), radii exact dyadics. */
static void s_of(acb_t s, const jsonl_value *rec)
{
    fmpq_t q; arb_t r; int k;
    fmpq_init(q); arb_init(r);
    for (k = 0; k < 2; k++) {
        arb_ptr c = k ? acb_imagref(s) : acb_realref(s);
        q_of(q, str(at(field(rec, "s"), (size_t) k))); arb_set_fmpq(c, q, 2 * RP);
        q_of(q, str(at(field(rec, "rad"), (size_t) k))); arb_set_fmpq(r, q, 2 * RP);
        CHECK(arb_is_exact(r));
        arb_add_error(c, r);
    }
    fmpq_clear(q); arb_clear(r);
}
static void char_of(adf_char_t chi, const jsonl_value *rec)
{
    CHECK(adf_char_set_conrey(chi, (ulong) number(field(rec, "q")), (ulong) number(field(rec, "n"))) == ADF_OK);
}

/* ------------------------------------------------------------------ predicates and sentinels */
/* 1 iff z is finite and every coordinate diameter is <= 2^-bits. */
static int narrow(const acb_t z, slong bits)
{
    return acb_is_finite(z) && mag_cmp_2exp_si(arb_radref(acb_realref(z)), -bits - 1) <= 0
        && mag_cmp_2exp_si(arb_radref(acb_imagref(z)), -bits - 1) <= 0;
}
/* The largest coordinate radius. */
static void maxrad(mag_t m, const acb_t z) { mag_max(m, arb_radref(acb_realref(z)), arb_radref(acb_imagref(z))); }

typedef struct { acb_t z, copy; unsigned char bytes[sizeof(acb_struct)]; } sentinel;
static void sent_init(sentinel *s)
{
    acb_init(s->z); acb_init(s->copy);
    acb_set_si_si(s->z, 12345, -678); mag_set_ui_2exp_si(arb_radref(acb_realref(s->z)), 3, -7);
    acb_set(s->copy, s->z); memcpy(s->bytes, s->z, sizeof(acb_struct));
}
static void sent_check(const sentinel *s)
{
    CHECK(memcmp(s->bytes, s->z, sizeof(acb_struct)) == 0); CHECK(acb_equal(s->z, s->copy));
}
static void sent_clear(sentinel *s) { acb_clear(s->z); acb_clear(s->copy); }
static int tate_sent(const adf_char_t chi, const acb_t s, slong bits, slong prec)
{
    sentinel t; int st;
    sent_init(&t); st = adf_tate_integral(t.z, chi, s, bits, prec);
    if (st != ADF_OK) sent_check(&t);
    sent_clear(&t); return st;
}

/* E(theta) = exp(2 pi i theta) at RP bits. */
static void phase_ref(acb_t z, const fmpq_t th)
{
    arb_t t; arb_init(t);
    arb_set_fmpq(t, th, RP); arb_mul_2exp_si(t, t, 1);
    arb_sin_cos_pi(acb_imagref(z), acb_realref(z), t, RP);
    arb_clear(t);
}

/* ------------------------------------------------------------------ adf_tate_vector */
static void vector_record(const jsonl_value *rec)
{
    adf_char_t chi, chi2; adf_rfun_t phi, phi2; adf_ffun_t f, f2; acb_t ref, s; fmpq_t th;
    const jsonl_value *ph = field(rec, "phases"); ulong C = (ulong) number(field(rec, "C")), j;
    int e = (int) number(field(rec, "e"));
    adf_char_init(chi); adf_char_init(chi2); adf_rfun_init(phi); adf_rfun_init(phi2); adf_ffun_init(f);
    adf_ffun_init(f2); acb_init(ref); acb_init(s); fmpq_init(th);
    char_of(chi, rec);
    CHECK(adf_char_get_conductor(chi) == C && adf_char_get_label(chi) == (ulong) number(field(rec, "label")));
    CHECK(adf_char_get_parity(chi) == e);
    CHECK(adf_tate_vector(phi, f, chi, 128) == ADF_OK);
    CHECK(adf_ffun_is_canonical(f) && f->D == 1 && f->M == C && jsonl_size(ph) == C);
    for (j = 0; j < C; j++) {
        acb_srcptr v = f->f + j;
        if (is_null(at(ph, j))) { CHECK(acb_is_zero(v) && acb_is_exact(v)); continue; }
        q_of(th, str(at(ph, j))); phase_ref(ref, th);
        /* overlap with the 512-bit reference (an exact coordinate such as cos(2 pi/3) = -1/2 is stored exactly
           by adf_phase_get_acb and cannot contain the reference's own ball) */
        CHECK(acb_overlaps(v, ref));
        if (fmpz_cmp_ui(fmpq_denref(th), 4) <= 0 && fmpz_cmp_ui(fmpq_denref(th), 3) != 0)
            CHECK(acb_is_exact(v) && acb_equal(v, ref));          /* cardinal phases 0, 1/4, 1/2, 3/4: exact */
        else
            CHECK(!acb_is_exact(v) && mag_cmp_2exp_si(arb_radref(acb_realref(v)), -118) <= 0
                  && mag_cmp_2exp_si(arb_radref(acb_imagref(v)), -118) <= 0);
    }
    if (C == 1) CHECK(acb_is_one(f->f));
    /* the term x^e exp(-pi x^2), exact */
    CHECK(adf_rfun_is_canonical(phi) && phi->len == 1);
    CHECK(acb_poly_length(phi->term[0].P) == e + 1 && acb_is_one(phi->term[0].P->coeffs + e));
    if (e) CHECK(acb_is_zero(phi->term[0].P->coeffs) && acb_is_exact(phi->term[0].P->coeffs));
    CHECK(acb_is_one(phi->term[0].A) && acb_is_exact(phi->term[0].A));
    CHECK(acb_is_zero(phi->term[0].B) && acb_is_exact(phi->term[0].B) && acb_is_zero(phi->term[0].C)
          && acb_is_exact(phi->term[0].C));
    /* chi->s is ignored */
    acb_set_si_si(s, 5, 7); mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -3);
    adf_char_set(chi2, chi); CHECK(adf_char_set_s(chi2, s) == ADF_OK);
    CHECK(adf_tate_vector(phi2, f2, chi2, 128) == ADF_OK);
    CHECK(adf_rfun_identical(phi, phi2) && adf_ffun_identical(f, f2));
    adf_char_clear(chi); adf_char_clear(chi2); adf_rfun_clear(phi); adf_rfun_clear(phi2); adf_ffun_clear(f);
    adf_ffun_clear(f2); acb_clear(ref); acb_clear(s); fmpq_clear(th);
}

/* Failure leaves both outputs untouched (bytes of the structs and the values). */
static void vector_statuses(void)
{
    adf_char_t chi, big; adf_rfun_t phi, pc; adf_ffun_t f, fc; unsigned char pb[sizeof(adf_rfun_struct)],
    fb[sizeof(adf_ffun_struct)];
    adf_char_init(chi); adf_char_init(big); adf_rfun_init(phi); adf_rfun_init(pc); adf_ffun_init(f);
    adf_ffun_init(fc);
    CHECK(adf_char_set_conrey(chi, 5, 2) == ADF_OK);
    CHECK(adf_tate_vector(phi, f, chi, 64) == ADF_OK);
    adf_rfun_set(pc, phi); adf_ffun_set(fc, f); memcpy(pb, phi, sizeof pb); memcpy(fb, f, sizeof fb);
    CHECK(adf_char_set_conrey(chi, 4, 3) == ADF_OK);
    CHECK(adf_tate_vector(phi, f, chi, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    CHECK(memcmp(pb, phi, sizeof pb) == 0 && memcmp(fb, f, sizeof fb) == 0);
    CHECK(adf_rfun_identical(phi, pc) && adf_ffun_identical(f, fc));
    /* C above the character cap (a forged struct: no canonical char has it): LIMIT before setup and before INV */
    big->q = ADF_CHAR_MOD_MAX + 1; big->n = 2; big->parity = 0;
    CHECK(adf_tate_vector(phi, f, big, 64) == ADF_LIMIT);
    CHECK(memcmp(pb, phi, sizeof pb) == 0 && memcmp(fb, f, sizeof fb) == 0);
    big->q = 1; big->n = 1;
    /* prec = ADF_REAL_PREC_MAX is admitted; prec below 2 works at 2 */
    CHECK(adf_tate_vector(phi, f, chi, -5) == ADF_OK && f->M == 4);
    CHECK(adf_tate_vector(phi, f, chi, ADF_REAL_PREC_MAX) == ADF_OK && f->M == 4);
    adf_char_clear(chi); adf_char_clear(big); adf_rfun_clear(phi); adf_rfun_clear(pc); adf_ffun_clear(f);
    adf_ffun_clear(fc);
}

/* ------------------------------------------------------------------ adf_tate_integral against the vectors */
static void value_record(const jsonl_value *rec, slong bits)
{
    adf_char_t chi; acb_t s, z, ref; size_t k;
    const jsonl_value *pts = field(rec, "points"), *fl = field(rec, "flint");
    int st;
    adf_char_init(chi); acb_init(s); acb_init(z); acb_init(ref);
    char_of(chi, rec); s_of(s, rec);
    if (jsonl_size(pts) == 5 && bits > 64) {
        /* N-D23: the input radius (2^-60 or 2^-70) holds the width: two corner references differ by more than
           2^-bits in a coordinate, so no OK is possible; NOT_DETERMINED with z untouched, then OK at bits 48 */
        acb_t d; acb_init(d);
        ball_of(ref, at(pts, 1)); ball_of(d, at(pts, 4)); acb_sub(d, d, ref, RP);
        CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(d)), -bits - 8) < 0);
        CHECK(arf_cmpabs_2exp_si(arb_midref(acb_realref(d)), -bits + 1) > 0
              || arf_cmpabs_2exp_si(arb_midref(acb_imagref(d)), -bits + 1) > 0);
        CHECK(tate_sent(chi, s, bits, bits + 32) == ADF_NOT_DETERMINED);
        acb_clear(d);
        bits = 48;
    }
    st = adf_tate_integral(z, chi, s, bits, bits + 32);
    if (st != ADF_OK)
        fprintf(stderr, "value: q=%ld n=%ld s=%s+%si bits=%ld: status %d\n", number(field(rec, "q")),
                number(field(rec, "n")), str(at(field(rec, "s"), 0)), str(at(field(rec, "s"), 1)), (long) bits, st);
    CHECK(st == ADF_OK);
    CHECK(narrow(z, bits));
    for (k = 0; k < jsonl_size(pts); k++) {
        ball_of(ref, at(pts, k)); CHECK(acb_contains(z, ref)); points_in++;
        ball_of(ref, at(fl, k)); CHECK(acb_contains(z, ref)); points_in++;
    }
    values_in++;
    adf_char_clear(chi); acb_clear(s); acb_clear(z); acb_clear(ref);
}

/* T3 against an independent integral (mpmath's incomplete Gamma, the oracle's numeric_piece): at the target 2^-60
   and at the coarse target 2^-6, where the remainder is needed (counted: the polynomial part alone misses). */
static unsigned long pieces_needing_remainder;
static void piece_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t z, val, ref, w; acb_ptr coef; arb_t qerr; arf_t tg; fmpq_t q; fmpz_t a;
    ulong N = (ulong) number(field(rec, "N")), R = (ulong) number(field(rec, "R")), n, C; int e, k;
    adf_char_init(chi); acb_init(z); acb_init(val); acb_init(ref); acb_init(w); arb_init(qerr); arf_init(tg);
    fmpq_init(q); fmpz_init(a); coef = _acb_vec_init((slong) N + 1);
    char_of(chi, rec); C = adf_char_get_conductor(chi); e = adf_char_get_parity(chi);
    for (k = 0; k < 2; k++) {
        q_of(q, str(at(field(rec, "z"), (size_t) k))); arb_set_fmpq(k ? acb_imagref(z) : acb_realref(z), q, RP);
    }
    for (n = 1; n <= N; n++) {
        fmpz_set_ui(a, n); CHECK(adf_char_chi(coef + n, chi, a, 256) == ADF_OK);
        if (e) acb_mul_ui(coef + n, coef + n, n, 256);
    }
    ball_of(ref, field(rec, "value"));
    for (k = 0; k < 2; k++) {
        arf_one(tg); arf_mul_2exp_si(tg, tg, k ? -6 : -60);
        CHECK(adf_tate_taylor_piece(val, qerr, z, coef, N, e, C, R, tg, 256) == ADF_OK);
        CHECK(arb_is_nonnegative(qerr) && arb_is_finite(qerr));
        { arf_t u; arf_init(u); arb_get_ubound_arf(u, qerr, 64); CHECK(arf_cmp(u, tg) <= 0); arf_clear(u); }
        acb_set(w, val); arb_add_error(acb_realref(w), qerr); arb_add_error(acb_imagref(w), qerr);
        if (R == 1) CHECK(acb_is_zero(val) && arb_is_zero(qerr) && acb_contains(ref, val));   /* the exact 0 */
        else CHECK(acb_contains(w, ref));
        if (k && R > 1 && !acb_contains(val, ref)) pieces_needing_remainder++;
    }
    adf_char_clear(chi); acb_clear(z); acb_clear(val); acb_clear(ref); acb_clear(w); arb_clear(qerr); arf_clear(tg);
    fmpq_clear(q); fmpz_clear(a); _acb_vec_clear(coef, (slong) N + 1);
}

/* The width is checked after the remainders and tails are added: zeta at s = 2 +/- r, bits 20, radii r across the
   boundary where the image width reaches 2^-20 (|I'(2)| = 0.749; the enclosure radius is about 1.26 r plus the
   added error, so the boundary is near r = 1.5 2^-22); every OK result is narrow and contains both end values;
   both OK and NOT_DETERMINED occur. A code that checked the width before adding the error radius returns OK with a
   wider ball near the boundary. */
static void boundary(void)
{
    adf_char_t chi; acb_t s, z, ref; arb_t t; int k, ok = 0, nd = 0, st, j;
    adf_char_init(chi); acb_init(s); acb_init(z); acb_init(ref); arb_init(t);
    for (k = 0; k < 192; k++) {
        acb_set_ui(s, 2);
        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), (ulong) (128 + k), -29);       /* 2^-22 (1 + k/128) */
        st = adf_tate_integral(z, chi, s, 20, 64);
        CHECK(st == ADF_OK || st == ADF_NOT_DETERMINED);
        if (st == ADF_NOT_DETERMINED) { nd++; continue; }
        ok++;
        CHECK(narrow(z, 20));
        for (j = -1; j <= 1; j += 2) {        /* I(2 -/+ r) = pi^(-1 -/+ r/2) Gamma(1 -/+ r/2) zeta(2 -/+ r) */
            acb_t p, g; acb_init(p); acb_init(g);
            arb_set_arf(acb_realref(p), arb_midref(acb_realref(s)));
            arb_set_ui(t, 128 + (ulong) k); arb_mul_2exp_si(t, t, -29); if (j < 0) arb_neg(t, t);
            arb_add(acb_realref(p), acb_realref(p), t, RP); arb_zero(acb_imagref(p));
            acb_dirichlet_zeta(ref, p, RP);
            acb_mul_2exp_si(g, p, -1); acb_gamma(g, g, RP); acb_mul(ref, ref, g, RP);
            acb_const_pi(g, RP); acb_mul_2exp_si(p, p, -1); acb_neg(p, p); acb_pow(g, g, p, RP);
            acb_mul(ref, ref, g, RP);
            CHECK(acb_contains(z, ref));
            acb_clear(p); acb_clear(g);
        }
    }
    CHECK(ok > 0 && nd > 0);
    printf("boundary: %d OK, %d NOT_DETERMINED across the width 2^-20\n", ok, nd);
    adf_char_clear(chi); acb_clear(s); acb_clear(z); acb_clear(ref); arb_clear(t);
}

/* The certificate against the oracle's: N, R and every panel degree J of both sides equal those of
   continuation(chi, s, bits) (P15 searches with L6 and L14, T3's degree search; api-5.md:194-217). */
static void cutoff_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t s; ulong N, R, K, deg[128], work; size_t k, j;
    const jsonl_value *dg = field(rec, "degrees");
    slong bits = number(field(rec, "bits"));
    fmpq_t q; fmpq_init(q);
    adf_char_init(chi); acb_init(s); q_of(q, str(field(rec, "s"))); arb_set_fmpq(acb_realref(s), q, 2 * RP);
    fmpq_clear(q);
    char_of(chi, rec);
    CHECK(adf_tate_cutoffs(&N, &R, &K, deg, &work, chi, s, bits, bits + 64) == ADF_OK);
    CHECK(N == (ulong) number(field(rec, "N")) && R == (ulong) number(field(rec, "R")));
    CHECK(jsonl_size(at(dg, 0)) == K && jsonl_size(at(dg, 1)) == K);
    for (k = 0; k < 2; k++)
        for (j = 0; j < K; j++)
            CHECK(deg[k * K + j] == (ulong) number(at(at(dg, k), j)));
    CHECK(work > 0 && work <= (ulong) ADF_TATE_WORK_MAX);
    adf_char_clear(chi); acb_clear(s);
}

static void vectors(void)
{
    jsonl_file *file; jsonl_error_t err; size_t i, n, nv = 0, nval = 0, nw = 0, np = 0, nc = 0;
    double t0 = seconds();
    CHECK(jsonl_open(VEC, &file, &err));
    n = jsonl_count(file);
    for (i = 0; i < n; i++) {
        const jsonl_value *rec = jsonl_record(file, i); const char *kind = str(field(rec, "kind"));
        if (!strcmp(kind, "vector")) { vector_record(rec); nv++; }
        else if (!strcmp(kind, "value")) {
            long q = number(field(rec, "q"));
            value_record(rec, 53);
            if (q == 1 || q == 4 || q == 5 || q == 16) { value_record(rec, 20); value_record(rec, 80); }
            nval++;
        } else if (!strcmp(kind, "cutoff")) {
            cutoff_record(rec); nc++;
        } else if (!strcmp(kind, "piece")) {
            piece_record(rec); np++;
        } else if (!strcmp(kind, "witness")) {
            /* N-D23: zeta on the hull [9/8, 5/4]; the end values differ by more than 1, so no width <= 1 */
            adf_char_t chi; acb_t s; adf_char_init(chi); acb_init(s);
            char_of(chi, rec); s_of(s, rec);
            CHECK(tate_sent(chi, s, 0, 64) == ADF_NOT_DETERMINED);
            CHECK(tate_sent(chi, s, 20, 64) == ADF_NOT_DETERMINED);
            adf_char_clear(chi); acb_clear(s); nw++;
        }
    }
    jsonl_close(file);
    CHECK(nv == 18 && nval == 144 && nw == 1 && np == 18 && nc == 57);
    /* the coarse target is not vacuous: in some records the polynomial part alone misses the integral */
    CHECK(pieces_needing_remainder > 0);
    printf("vectors: %zu vectors, %zu value records (%lu calls OK, %lu reference points inside), %zu witness,"
           " %zu T3 pieces (%lu need the remainder), %zu cutoff records in %.2f s\n",
           nv, nval, values_in, points_in, nw, np, pieces_needing_remainder, nc, seconds() - t0);
}

/* ------------------------------------------------------------------ hand-derived values (SPEC 8) */
static void hand(void)
{
    adf_char_t chi; acb_t s, z, ref, L; arb_t t; dirichlet_group_t G; dirichlet_char_t c;
    adf_char_init(chi); acb_init(s); acb_init(z); acb_init(ref); acb_init(L); arb_init(t);
    /* zeta(2) = pi^2/6 completed: I(2) = pi^-1 Gamma(1) zeta(2) = pi/6 */
    acb_set_ui(s, 2);
    CHECK(adf_tate_integral(z, chi, s, 80, 128) == ADF_OK && narrow(z, 80));
    arb_const_pi(t, RP); arb_div_ui(t, t, 6, RP); acb_set_arb(ref, t);
    CHECK(acb_contains(z, ref));
    /* L(2, chi_4) = Catalan's G; I = pi^(-3/2) Gamma(3/2) G = G/(2 pi) (Gamma(3/2) = sqrt(pi)/2) */
    CHECK(adf_char_set_conrey(chi, 4, 3) == ADF_OK);
    CHECK(adf_tate_integral(z, chi, s, 80, 128) == ADF_OK && narrow(z, 80));
    arb_const_catalan(acb_realref(ref), RP); arb_const_pi(t, RP); arb_mul_2exp_si(t, t, 1);
    arb_div(acb_realref(ref), acb_realref(ref), t, RP); arb_zero(acb_imagref(ref));
    CHECK(acb_contains(z, ref));
    /* the same against FLINT's acb_dirichlet_l (acb_dirichlet.rst:567-569), completed by hand */
    dirichlet_group_init(G, 4); dirichlet_char_init(c, G); dirichlet_char_log(c, G, 3);
    acb_dirichlet_l(L, s, G, c, RP);
    arb_const_pi(t, RP); arb_mul_2exp_si(t, t, 1); acb_div_arb(L, L, t, RP);
    CHECK(acb_contains(z, L));
    dirichlet_char_clear(c); dirichlet_group_clear(G);
    /* L(1, chi_4) = pi/4 is not in the domain: Re(s) = 1 is DOMAIN */
    acb_one(s);
    CHECK(tate_sent(chi, s, 20, 64) == ADF_DOMAIN);
    adf_char_clear(chi); acb_clear(s); acb_clear(z); acb_clear(ref); acb_clear(L); arb_clear(t);
}

/* ------------------------------------------------------------------ the domain gate and the statuses */
static void gate(void)
{
    adf_char_t chi, chi5, big; acb_t s; double t0;
    adf_char_init(chi); adf_char_init(chi5); adf_char_init(big); acb_init(s);
    CHECK(adf_char_set_conrey(chi5, 5, 2) == ADF_OK);
    /* upper(Re s) <= 1: DOMAIN (exact 1, 1/2 + 10 i, the closed endpoint 0.999 +/- 0.001 reaching 1, s = 0) */
    acb_one(s); CHECK(tate_sent(chi, s, 20, 64) == ADF_DOMAIN); CHECK(tate_sent(chi5, s, 20, 64) == ADF_DOMAIN);
    acb_set_d_d(s, 0.5, 10); CHECK(tate_sent(chi5, s, 20, 64) == ADF_DOMAIN);
    acb_zero(s); CHECK(tate_sent(chi, s, 20, 64) == ADF_DOMAIN);
    /* the exact ball [1 - 2^-9, 1]: midpoint 1 - 2^-10, radius 2^-10, its upper end exactly 1 */
    acb_one(s); arb_set_si(acb_imagref(s), 3);
    arf_set_si_2exp_si(arb_midref(acb_realref(s)), 1023, -10);
    mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -10);
    CHECK(!arb_is_exact(acb_realref(s)));
    CHECK(tate_sent(chi, s, 20, 64) == ADF_DOMAIN); CHECK(tate_sent(chi5, s, 20, 64) == ADF_DOMAIN);
    /* straddling 1: NOT_DETERMINED */
    acb_one(s); mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -10);
    CHECK(tate_sent(chi, s, 20, 64) == ADF_NOT_DETERMINED);
    CHECK(tate_sent(chi5, s, 20, 64) == ADF_NOT_DETERMINED);
    /* the imaginary radius does not matter for the gate: 2 + (0 +/- 100) i passes the gate */
    acb_set_ui(s, 3); mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 1, -40);
    CHECK(tate_sent(chi, s, 10, 64) == ADF_OK);
    /* nonfinite s: DOMAIN */
    acb_indeterminate(s); CHECK(tate_sent(chi, s, 20, 64) == ADF_DOMAIN);
    acb_set_ui(s, 2); arb_pos_inf(acb_imagref(s)); CHECK(tate_sent(chi, s, 20, 64) == ADF_DOMAIN);
    /* bits outside [0, 2^21]: DOMAIN, after the precision cap (LIMIT first) and after the size cap */
    acb_set_ui(s, 2);
    CHECK(tate_sent(chi, s, -1, 64) == ADF_DOMAIN);
    CHECK(tate_sent(chi, s, 2097153, 64) == ADF_DOMAIN);
    CHECK(tate_sent(chi, s, WORD_MIN, 64) == ADF_DOMAIN);
    CHECK(tate_sent(chi, s, -1, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    CHECK(tate_sent(chi, s, 20, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    /* the domain comes after the bits check: s = 1 with bits -1 is DOMAIN either way; prec cap first */
    acb_one(s); CHECK(tate_sent(chi, s, 20, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    acb_set_ui(s, 2);
    /* D2: the transform cap C^2 <= 2^20; C = 1031 (prime, n = 2 primitive) is LIMIT before the bits check */
    CHECK(adf_char_set_conrey(big, 1031, 2) == ADF_OK && adf_char_get_conductor(big) == 1031);
    t0 = seconds();
    CHECK(tate_sent(big, s, 20, 64) == ADF_LIMIT);
    CHECK(tate_sent(big, s, -1, 64) == ADF_LIMIT);
    CHECK(adf_char_set_conrey(big, 65536, 3) == ADF_OK);
    CHECK(tate_sent(big, s, 20, 64) == ADF_LIMIT);
    CHECK(seconds() - t0 < 0.5);
    /* C = 1021 (prime, 1021^2 <= 2^20): the transform fits, the total work of D2 does not: LIMIT within 10 s */
    CHECK(adf_char_set_conrey(big, 1021, 2) == ADF_OK && adf_char_get_conductor(big) == 1021);
    t0 = seconds();
    CHECK(tate_sent(big, s, 20, 64) == ADF_LIMIT);
    printf("work cap: C = 1021 LIMIT in %.2f s\n", seconds() - t0);
    CHECK(seconds() - t0 < 10);
    /* C = 1024 = 2^10 (n = 3, primitive): passes the size check, but C + C^2 > 2^20: the charge before the
       transform
       refuses at once (LIMIT before the O(C^2) work) */
    CHECK(adf_char_set_conrey(big, 1024, 3) == ADF_OK && adf_char_get_conductor(big) == 1024);
    t0 = seconds();
    CHECK(tate_sent(big, s, 20, 64) == ADF_LIMIT);
    CHECK(seconds() - t0 < 0.5);
    {   /* the total charged over the panels: zeta at bits 1400 (one attempt at 64 bits through the hidden hook):
           every panel fits alone (J of the first panel 2405, N = 32), the sum of the panels does not: LIMIT; at
           bits 1200 the same attempt fits (work 501899 of 2^20) */
        ulong N, R, K, deg[128], work; adf_char_t z1; adf_char_init(z1);
        CHECK(adf_tate_cutoffs(&N, &R, &K, deg, &work, z1, s, 1400, 64) == ADF_LIMIT);
        CHECK(work <= (ulong) ADF_TATE_WORK_MAX && (deg[0] + 1) * N < (ulong) ADF_TATE_WORK_MAX / 8);
        CHECK(adf_tate_cutoffs(&N, &R, &K, deg, &work, z1, s, 1200, 64) == ADF_OK);
        CHECK(work > (ulong) ADF_TATE_WORK_MAX / 4 && work <= (ulong) ADF_TATE_WORK_MAX);
        adf_char_clear(z1);
    }
    /* bits = 2^21 is admitted; the work cap gives LIMIT within 10 s */
    t0 = seconds();
    CHECK(tate_sent(chi, s, 2097152, 64) == ADF_LIMIT);
    printf("work cap: bits = 2^21 LIMIT in %.2f s\n", seconds() - t0);
    CHECK(seconds() - t0 < 10);
    /* bits = 0 is admitted */
    CHECK(tate_sent(chi, s, 0, 64) == ADF_OK);
    adf_char_clear(chi); adf_char_clear(chi5); adf_char_clear(big); acb_clear(s);
}

/* ------------------------------------------------------------------ widths, precision, aliasing */
static void widths(void)
{
    static const slong bt[3] = { 20, 53, 80 };
    adf_char_t chi; acb_t s, z, w; mag_t prev, m; int k, c;
    adf_char_init(chi); acb_init(s); acb_init(z); acb_init(w); mag_init(prev); mag_init(m);
    for (c = 0; c < 3; c++) {
        if (c == 1) CHECK(adf_char_set_conrey(chi, 5, 2) == ADF_OK);
        if (c == 2) CHECK(adf_char_set_conrey(chi, 16, 3) == ADF_OK);
        acb_set_si_si(s, 2, 3);
        /* the SPEC acceptance: the width shrinks as the precision (and the target) grows */
        for (k = 0; k < 3; k++) {
            CHECK(adf_tate_integral(z, chi, s, bt[k], bt[k] + 64) == ADF_OK && narrow(z, bt[k]));
            maxrad(m, z);
            if (k) CHECK(mag_cmp(m, prev) < 0);
            mag_set(prev, m);
            /* "prec as needed": a low starting precision is doubled until the width is met */
            CHECK(adf_tate_integral(w, chi, s, bt[k], 2) == ADF_OK && narrow(w, bt[k]) && acb_overlaps(z, w));
        }
        /* z = s aliasing: the same value as into a distinct output */
        CHECK(adf_tate_integral(z, chi, s, 53, 100) == ADF_OK);
        acb_set(w, s);
        CHECK(adf_tate_integral(w, chi, w, 53, 100) == ADF_OK && acb_equal(w, z));
        /* a failing aliased call leaves s */
        acb_one(w); CHECK(adf_tate_integral(w, chi, w, 53, 100) == ADF_DOMAIN && acb_is_one(w));
    }
    /* near the zeta pole at 1: 1 + 2^-20 is in the domain; the value is about 2^20 */
    adf_char_init(chi);
    CHECK(adf_char_set_conrey(chi, 1, 1) == ADF_OK);
    acb_one(s); acb_mul_2exp_si(w, s, -20); acb_add(s, s, w, 64);
    CHECK(adf_tate_integral(z, chi, s, 10, 64) == ADF_OK && narrow(z, 10));
    CHECK(arf_cmp_2exp_si(arb_midref(acb_realref(z)), 19) > 0
          && arf_cmp_2exp_si(arb_midref(acb_realref(z)), 21) < 0);
    adf_char_clear(chi); acb_clear(s); acb_clear(z); acb_clear(w); mag_clear(prev); mag_clear(m);
}

/* ------------------------------------------------------------------ T5: the theta identity at sample t */
/* E = 2 sum_(n > N) n^e exp(-c n^2) bounded by Lemma 6 (beta = 0), c > 0 exact ball, at 64 bits; also adds the
   explicit prefix up to the first K with rho <= 1/2. */
static void series_tail(arb_t E, int e, const arb_t c, slong N)
{
    arb_t rho, term, t; arf_t b; slong K = N + 1;
    arb_init(rho); arb_init(term); arb_init(t); arf_init(b); arb_zero(E);
    for (;;) {
        arb_set_si(t, e); arb_div_si(t, t, K, 64); arb_mul_si(rho, c, 2 * K + 1, 64); arb_sub(t, t, rho, 64);
        arb_exp(rho, t, 64);
        arb_mul_si(t, c, -K * K, 64); arb_exp(term, t, 64); if (e) arb_mul_si(term, term, K, 64);
        arb_get_ubound_arf(b, rho, 64);
        if (arf_cmp_2exp_si(b, -1) <= 0) {
            arb_sub_ui(t, rho, 1, 64); arb_neg(t, t); arb_div(term, term, t, 64); arb_add(E, E, term, 64); break;
        }
        arb_add(E, E, term, 64); K++;
    }
    arb_mul_2exp_si(E, E, 1);
    arb_clear(rho); arb_clear(term); arb_clear(t); arf_clear(b);
}

/* Theta_x(t) = sum_(n in Z) x(n) n^e exp(-pi n^2 t/C) by the expansion |n| <= N plus the Lemma 6 tail, where
   x(n) = chi(n) or conj(chi(n)) (analysis P13). */
static void theta_expansion(acb_t th, const adf_char_t chi, int conj, const arb_t t, slong N, slong pp)
{
    acb_t v, x; arb_t c, E; fmpz_t a; slong n; ulong C = adf_char_get_conductor(chi);
    int e = adf_char_get_parity(chi);
    acb_init(v); acb_init(x); arb_init(c); arb_init(E); fmpz_init(a);
    acb_zero(th);
    if (C == 1) acb_one(th);
    arb_const_pi(c, pp); arb_mul(c, c, t, pp); arb_div_ui(c, c, C, pp);
    for (n = 1; n <= N; n++) {
        fmpz_set_si(a, n); CHECK(adf_char_chi(x, chi, a, pp) == ADF_OK);
        if (conj) acb_conj(x, x);
        arb_mul_si(acb_realref(v), c, -n * n, pp); arb_exp(acb_realref(v), acb_realref(v), pp);
        arb_zero(acb_imagref(v));
        if (e) acb_mul_si(v, v, n, pp);
        acb_mul(v, v, x, pp); acb_mul_2exp_si(v, v, 1);  /* n and -n: chi(-1)(-1)^e = 1 */
        acb_add(th, th, v, pp);
    }
    series_tail(E, e, c, N);
    arb_add_error(acb_realref(th), E); arb_add_error(acb_imagref(th), E);
    acb_clear(v); acb_clear(x); arb_clear(c); arb_clear(E); fmpz_clear(a);
}

static void theta(void)
{
    static const ulong pq[4][2] = { { 1, 1 }, { 4, 3 }, { 5, 2 }, { 16, 3 } };
    static const slong un[3][2] = { { 1, 1 }, { 3, 2 }, { 2, 3 } };
    adf_char_t chi; adf_rfun_t phi, phu, bal, hat; adf_ffun_t f, g; adf_rat_t h; acb_t l, r, ex, W, x, y, d;
    arb_t t, u; fmpz_t a; ulong NL, NR, C, n; int c, k, e; slong pp = 160;
    adf_char_init(chi); adf_rfun_init(phi); adf_rfun_init(phu); adf_rfun_init(bal); adf_rfun_init(hat);
    adf_ffun_init(f); adf_ffun_init(g); adf_rat_init(h); acb_init(l); acb_init(r); acb_init(ex); acb_init(W);
    acb_init(x); acb_init(y); acb_init(d); arb_init(t); arb_init(u); fmpz_init(a);
    for (c = 0; c < 4; c++) {
        CHECK(adf_char_set_conrey(chi, pq[c][0], pq[c][1]) == ADF_OK);
        C = adf_char_get_conductor(chi); e = adf_char_get_parity(chi);
        CHECK(adf_tate_vector(phi, f, chi, pp) == ADF_OK);
        CHECK(adf_char_root_number(W, chi, pp) == ADF_OK);
        /* the dual coefficients from the actual transforms (api-5.md:136-138): the balanced real factor
           x^e exp(-pi x^2/C) is phi dilated by the idele with real part 1/sqrt(C), times C^(e/2) (T5.1);
           g = F f (negative finite kernel), hat = F bal (positive real kernel); g[n mod C] Q(n/C) must contain
           W_chi conj(chi(n)) n^e */
        {
            adf_idele_t id; arb_t q; fmpq_t one; adf_ucoset_t u1;
            adf_idele_init(id); arb_init(q); fmpq_init(one); adf_ucoset_init(u1); fmpq_one(one);
            arb_set_ui(q, C); arb_rsqrt(q, q, pp);
            CHECK(adf_idele_set_parts(id, q, one, u1) == ADF_OK);
            CHECK(adf_rfun_dilate_idele(bal, phi, id, pp) == ADF_OK);
            if (e) {                                      /* C^(e/2): P = C^(-1/2) x becomes x */
                arb_set_ui(q, C); arb_sqrt(q, q, pp); acb_set_arb(x, q);
                acb_poly_scalar_mul(bal->term[0].P, bal->term[0].P, x, pp);
            }
            fmpq_clear(one); adf_ucoset_clear(u1);
            adf_idele_clear(id); arb_clear(q);
        }
        CHECK(adf_ffun_fourier(g, f, pp) == ADF_OK && g->D == C && g->M == 1);
        CHECK(adf_rfun_fourier(hat, bal, pp) == ADF_OK && hat->len == 1);
        for (n = 1; n <= 3 * C; n++) {
            fmpz_set_ui(a, n); CHECK(adf_char_chi(x, chi, a, pp) == ADF_OK); acb_conj(x, x);
            acb_mul(x, x, W, pp); acb_mul_ui(x, x, e ? n : 1, pp);       /* W conj(chi(n)) n^e */
            acb_set_ui(y, n); acb_div_ui(y, y, C, pp);
            acb_poly_evaluate(d, hat->term[0].P, y, pp); acb_mul(d, d, g->f + n % C, pp);
            CHECK(acb_overlaps(d, x));
            CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(d)), -100) <= 0);
        }
        /* T5: both Poisson enclosures at the real dilation u (theta variable t = C u^2: f(u x) of the vector
           phi_e, rational u, the finite factor unchanged) against the expansion plus Lemma 6 tails */
        for (k = 0; k < 3; k++) {
            fmpq_set_si(h->q, un[k][0], (ulong) un[k][1]);
            CHECK(adf_rfun_dilate_rat(phu, phi, h, pp) == ADF_OK);
            CHECK(adf_tensor_poisson(l, r, &NL, &NR, phu, f, 100, pp) == ADF_OK);
            arb_set_fmpq(u, h->q, pp); arb_sqr(t, u, pp); arb_mul_ui(t, t, C, pp);       /* t = C u^2 */
            /* left = u^e Theta_chi(t) */
            theta_expansion(ex, chi, 0, t, 40, pp);
            if (e) acb_mul_arb(ex, ex, u, pp);
            CHECK(acb_overlaps(l, ex) && mag_cmp_2exp_si(arb_radref(acb_realref(ex)), -90) <= 0);
            /* right = u^e W t^(-e-1/2) Theta_conj(chi)(1/t) by P13's identity */
            arb_inv(t, t, pp);
            theta_expansion(ex, chi, 1, t, 40, pp);
            arb_inv(t, t, pp); arb_rsqrt(acb_realref(y), t, pp); arb_zero(acb_imagref(y));
            if (e) { acb_div_arb(y, y, t, pp); acb_mul_arb(y, y, u, pp); }
            acb_mul(ex, ex, y, pp); acb_mul(ex, ex, W, pp);
            CHECK(acb_overlaps(r, ex) && acb_overlaps(l, r));
        }
    }
    adf_char_clear(chi); adf_rfun_clear(phi); adf_rfun_clear(phu); adf_rfun_clear(bal); adf_rfun_clear(hat);
    adf_ffun_clear(f); adf_ffun_clear(g); adf_rat_clear(h); acb_clear(l); acb_clear(r); acb_clear(ex);
    acb_clear(W); acb_clear(x); acb_clear(y); acb_clear(d); arb_clear(t); arb_clear(u); fmpz_clear(a);
    printf("theta: dual coefficients from the transforms and T5 Poisson comparisons for 4 characters\n");
}

/* ------------------------------------------------------------------ INV: preconditions */
static void debug(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int k;
    for (k = 0; k < 4; k++) {
        pid_t p = fork(); int status; CHECK(p >= 0);
        if (!p) {
            adf_char_t chi; adf_rfun_t phi; adf_ffun_t f; acb_t z, s;
            adf_char_init(chi); adf_rfun_init(phi); adf_ffun_init(f); acb_init(z); acb_init(s); acb_set_ui(s, 2);
            if (k == 0) { chi->n = 0; (void) adf_tate_integral(z, chi, s, 20, 64); }   /* gcd(0, 1) != 1 */
            if (k == 1) { chi->n = 0; (void) adf_tate_vector(phi, f, chi, 64); }
            if (k == 2) { CHECK(adf_char_set_conrey(chi, 5, 2) == ADF_OK); chi->parity = 0;
                          (void) adf_tate_integral(z, chi, s, 20, 64); }              /* wrong parity */
            if (k == 3) {                                                                  /* z = chi->s */
                acb_set_ui(chi->s, 2); (void) adf_tate_integral(chi->s, chi, s, 20, 64);
            }
            /* Reached only if the entry check is missing. The clears are for tools/memcheck. */
            chi->n = 1; chi->parity = 0; chi->q = 1;
            adf_char_clear(chi); adf_rfun_clear(phi); adf_ffun_clear(f); acb_clear(z); acb_clear(s);
            _exit(0);
        }
        CHECK(waitpid(p, &status, 0) == p && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
    {   /* the precision cap is decided before the entry predicates */
        adf_char_t chi; acb_t z, s; adf_rfun_t phi; adf_ffun_t f;
        adf_char_init(chi); acb_init(z); acb_init(s); adf_rfun_init(phi); adf_ffun_init(f); acb_set_ui(s, 2);
        chi->n = 0;
        CHECK(adf_tate_integral(z, chi, s, 20, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        CHECK(adf_tate_vector(phi, f, chi, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        chi->n = 1;
        adf_char_clear(chi); acb_clear(z); acb_clear(s); adf_rfun_clear(phi); adf_ffun_clear(f);
    }
    printf("INV: 4 precondition aborts\n");
#endif
}

int main(void)
{
    vector_statuses();
    hand();
    gate();
    widths();
    theta();
    boundary();
    vectors();
    debug();
    printf("tate: %lu checks\n", checks);
    flint_cleanup();
    return 0;
}
