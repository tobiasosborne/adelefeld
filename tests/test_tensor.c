/* tests/test_tensor.c: slice 4f of docs/api-4.md, evaluation and additive integrals of test functions
   (include/adelefeld/tensor.h; statements in docs/api-4c.md "Slice 4f").
   Contract: docs/api-4.md section 6 (E1 steps 1-5, the integrals and norms), section 1 (statuses, caps D1),
   decision D3; analysis Proposition 4 (docs/proofs/analysis.md:150-173).
   Oracle: tests/ref/vectors/f4-slice5/ written by lanes/f4-slice5/gen_vectors.py from proto/functions4_checks.py
   (evaluate, evaluate_partial, rvalue_ball, rterm_transform, rterm_product); every record is run.
   Tightness, stated against the exact hull (a test of containment alone passes a function that returns the hull
   of ALL values): the hull h of the selected values and of 0 (when the ball leaves the support) is formed here by
   endpoints at 1024 bits; z must lie in h widened by 2^-120 (1 + |h|) + 2^-26 rad(h) on every side (exact and
   ball values at PREC = 128; the second term is the 30-bit radius of arb, rounded up by each operation).
   A value f[j] that is not selected and lies outside h must lie outside z (exclusion; counted).
   Products with a real value: z lies in the 1024-bit product of the oracle ball and the hull widened by
   2^-96 (1 + |v|). Integrals and norms: they contain the exact rational of the oracle, radius at most
   2^-120 (1 + |v|). */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/jsonl.h"
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define PREC 128
#define EXP 1024

static unsigned long checks, exclusions;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

/* ------------------------------------------------------------------ JSON helpers */
static const jsonl_value *field(const jsonl_value *v, const char *name)
{
    const jsonl_value *out; jsonl_error_t e;
    CHECK(jsonl_field(v, name, &out, &e)); return out;
}
static int has_field(const jsonl_value *v, const char *name)
{
    const jsonl_value *out; jsonl_error_t e;
    return jsonl_field(v, name, &out, &e);
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
static int is_null(const jsonl_value *v) { return jsonl_is(v, JSONL_NULL); }
static int boolean(const jsonl_value *v)
{
    jsonl_error_t e; int b = 0; CHECK(jsonl_bool(v, &b, &e)); return b;
}
static void q_of(fmpq_t q, const char *s)
{
    CHECK(fmpq_set_str(q, s, 10) == 0); fmpq_canonicalise(q);
}
static void z_of(fmpz_t z, const char *s) { CHECK(fmpz_set_str(z, s, 10) == 0); }

/* ------------------------------------------------------------------ building values */
/* f[j] = j + 1 (the oracle's family), or (j^2 mod 7 - 3) + i (j mod 3), or (j + 1)/3 +/- 2^-20 (ball). */
enum { FAM_INT, FAM_CPLX, FAM_BALL };
static void fun_family(adf_ffun_t f, ulong D, ulong M, int fam)
{
    slong L = (slong) (D * M), j; acb_ptr v = _acb_vec_init(L);
    for (j = 0; j < L; j++) {
        if (fam == FAM_INT) acb_set_si(v + j, j + 1);
        else if (fam == FAM_CPLX) {
            arb_set_si(acb_realref(v + j), (j * j) % 7 - 3); arb_set_si(acb_imagref(v + j), j % 3);
        }
        else {
            acb_set_si(v + j, j + 1); acb_div_ui(v + j, v + j, 3, PREC);
            arb_add_error_2exp_si(acb_realref(v + j), -20);
        }
    }
    CHECK(adf_ffun_set_acb_vec(f, D, M, v, L) == ADF_OK);
    _acb_vec_clear(v, L);
}
static void fun_vals(adf_ffun_t f, ulong D, ulong M, const double *re)
{
    slong L = (slong) (D * M), j; acb_ptr v = _acb_vec_init(L);
    for (j = 0; j < L; j++) acb_set_d(v + j, re[j]);
    CHECK(adf_ffun_set_acb_vec(f, D, M, v, L) == ADF_OK);
    _acb_vec_clear(v, L);
}
static void ball3(adf_fball_t x, const char *A, const char *H, const char *d)
{
    fmpz_t a, h, e; fmpz_init(a); fmpz_init(h); fmpz_init(e);
    z_of(a, A); z_of(h, H); z_of(e, d);
    CHECK(adf_fball_set_fmpz3(x, a, h, e) == ADF_OK);
    fmpz_clear(a); fmpz_clear(h); fmpz_clear(e);
}
static void ball3si(adf_fball_t x, slong A, slong H, slong d)
{
    fmpz_t a, h, e; fmpz_init_set_si(a, A); fmpz_init_set_si(h, H); fmpz_init_set_si(e, d);
    CHECK(adf_fball_set_fmpz3(x, a, h, e) == ADF_OK);
    fmpz_clear(a); fmpz_clear(h); fmpz_clear(e);
}
/* A one-term function P exp(-pi A x^2 + B x + C) from small numbers: P given by n real coefficients. */
static void fun_simple(adf_rfun_t f, const double *P, slong n, double Are, double Aim, double B, double C)
{
    adf_rterm_struct t; slong j;
    acb_poly_init(t.P); acb_init(t.A); acb_init(t.B); acb_init(t.C);
    for (j = 0; j < n; j++) {
        acb_t c; acb_init(c); acb_set_d(c, P[j]); acb_poly_set_coeff_acb(t.P, j, c); acb_clear(c);
    }
    acb_set_d_d(t.A, Are, Aim); acb_set_d(t.B, B); acb_set_d(t.C, C);
    CHECK(adf_rfun_set_terms(f, &t, 1) == ADF_OK);
    acb_poly_clear(t.P); acb_clear(t.A); acb_clear(t.B); acb_clear(t.C);
}
static void gauss(adf_rfun_t f) { static const double P[1] = { 1 }; fun_simple(f, P, 1, 1, 0, 0, 0); }
static void in_num(acb_t z, const jsonl_value *v)
{
    fmpq_t q; fmpq_init(q);
    q_of(q, str(at(v, 0))); arb_set_fmpq(acb_realref(z), q, EXP);
    q_of(q, str(at(v, 1))); arb_set_fmpq(acb_imagref(z), q, EXP);
    fmpq_clear(q);
}
static void rfun_json(adf_rfun_t f, const jsonl_value *terms)
{
    slong n = (slong) jsonl_size(terms), i; size_t j;
    adf_rterm_struct *t = (adf_rterm_struct *) flint_malloc((size_t) n * sizeof(*t)); acb_t c;
    acb_init(c);
    for (i = 0; i < n; i++) {
        const jsonl_value *v = at(terms, (size_t) i), *P = field(v, "P");
        acb_poly_init(t[i].P); acb_init(t[i].A); acb_init(t[i].B); acb_init(t[i].C);
        for (j = 0; j < jsonl_size(P); j++) { in_num(c, at(P, j)); acb_poly_set_coeff_acb(t[i].P, (slong) j, c); }
        in_num(t[i].A, field(v, "A")); in_num(t[i].B, field(v, "B")); in_num(t[i].C, field(v, "C"));
    }
    CHECK(adf_rfun_set_terms(f, t, n) == ADF_OK);
    for (i = 0; i < n; i++) { acb_poly_clear(t[i].P); acb_clear(t[i].A); acb_clear(t[i].B); acb_clear(t[i].C); }
    flint_free(t); acb_clear(c);
}
/* An oracle ball [re, im], two arb texts of 60 digits. */
static void ref_ball(acb_t z, const jsonl_value *v)
{
    CHECK(arb_set_str(acb_realref(z), str(at(v, 0)), EXP) == 0);
    CHECK(arb_set_str(acb_imagref(z), str(at(v, 1)), EXP) == 0);
}

/* ------------------------------------------------------------------ the hull checks */
/* h = the exact hull (1024 bits) of the f[j] with m[j] = '1' and of 0 when outside; 0 for nothing. Formed from
   the endpoints (arb_get_lbound_arf, arb_get_ubound_arf at 1024 bits), their minimum and maximum, and one
   arb_set_interval_arf: chained acb_union widens by about 2^-27 relative per step in FLINT 3.0.1. */
static void hull_ref(acb_t h, const adf_ffun_t f, const char *m, int outside)
{
    ulong j, L = f->D * f->M; int k, n = 0; arf_t lo[2], hi[2], t;
    for (k = 0; k < 2; k++) { arf_init(lo[k]); arf_init(hi[k]); }
    arf_init(t);
    for (j = 0; j <= L; j++) {
        const acb_struct *v = j < L ? f->f + j : NULL;
        if (j < L && m[j] != '1') continue;
        if (j == L && !outside) continue;
        for (k = 0; k < 2; k++) {
            const arb_struct *x = v == NULL ? NULL : (k ? acb_imagref(v) : acb_realref(v));
            if (x == NULL) arf_zero(t); else arb_get_lbound_arf(t, x, EXP);
            if (!n || arf_cmp(t, lo[k]) < 0) arf_set(lo[k], t);
            if (x != NULL) arb_get_ubound_arf(t, x, EXP);
            if (!n || arf_cmp(t, hi[k]) > 0) arf_set(hi[k], t);
        }
        n++;
    }
    acb_zero(h);
    if (n) {
        arb_set_interval_arf(acb_realref(h), lo[0], hi[0], EXP);
        arb_set_interval_arf(acb_imagref(h), lo[1], hi[1], EXP);
    }
    for (k = 0; k < 2; k++) { arf_clear(lo[k]); arf_clear(hi[k]); }
    arf_clear(t);
}
/* w = h widened by 2^k (1 + |h|) + 2^-26 rad(h) in both parts: the radius of a ball is a 30-bit mag, rounded
   up by every operation (a product with the exact 1 included), so no bound below about 2^-28 rad is possible. */
static void widen(acb_t w, const acb_t h, slong k)
{
    mag_t t, u; mag_init(t); mag_init(u);
    acb_get_mag(t, h); mag_one(u); mag_add(t, t, u); mag_mul_2exp_si(t, t, k);
    mag_max(u, arb_radref(acb_realref(h)), arb_radref(acb_imagref(h)));
    mag_mul_2exp_si(u, u, -26); mag_add(t, t, u);
    acb_set(w, h); arb_add_error_mag(acb_realref(w), t); arb_add_error_mag(acb_imagref(w), t);
    mag_clear(t); mag_clear(u);
}
/* z is not wider than the hull h: z inside h widened as widen states (that z contains the values is checked
   value by value: a rectangle that contains the values contains their hull). */
static int same_hull(const acb_t z, const acb_t h)
{
    acb_t w; int ok; acb_init(w); widen(w, h, -120); ok = acb_contains(w, z); acb_clear(w);
    return ok;
}
/* z == mid +/- rad exactly, real, both small integers. */
static int exact_ball(const acb_t z, slong mid, ulong rad)
{
    acb_t e; int ok; acb_init(e); acb_set_si(e, mid); mag_set_ui(arb_radref(acb_realref(e)), rad);
    ok = acb_equal(z, e); acb_clear(e); return ok;
}
/* z against the mask m and the outside flag: containment, tightness, exclusion, the exact 0 of the empty set. */
static void check_hull(const acb_t z, const adf_ffun_t f, const char *m, int outside)
{
    ulong j, L = f->D * f->M; int any = 0; acb_t h, w;
    acb_init(h); acb_init(w);
    CHECK(strlen(m) == L);
    hull_ref(h, f, m, outside);
    for (j = 0; j < L; j++) if (m[j] == '1') { CHECK(acb_contains(z, f->f + j)); any = 1; }
    if (outside) CHECK(acb_contains_zero(z));
    widen(w, h, -120); CHECK(acb_contains(w, z));
    for (j = 0; j < L; j++)
        if (m[j] == '0' && !acb_overlaps(h, f->f + j)) { CHECK(!acb_contains(z, f->f + j)); exclusions++; }
    if (!outside && !acb_contains_zero(h)) { CHECK(!acb_contains_zero(z)); exclusions++; }
    if (!any && !outside) CHECK(0);           /* the cosets cover (1/D) Zhat: never both empty and inside */
    if (!any) CHECK(acb_is_zero(z));          /* E1 step 2: the empty index set gives exactly zero */
    acb_clear(h); acb_clear(w);
}

/* ------------------------------------------------------------------ E1 steps 1-3: vectors */
static void eval_vectors(void)
{
    jsonl_file *fl; jsonl_error_t e; size_t i; int fam;
    adf_ffun_t f; adf_fball_t x; acb_t z;
    adf_ffun_init(f); adf_fball_init(x); acb_init(z);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice5/eval.jsonl", &fl, &e));
    CHECK(jsonl_count(fl) == 1603);
    for (i = 0; i < jsonl_count(fl); i++) {
        const jsonl_value *v = jsonl_record(fl, i);
        ulong D = (ulong) number(field(v, "D")), M = (ulong) number(field(v, "M"));
        const char *m = str(field(v, "m")); int o = boolean(field(v, "o"));
        ball3(x, str(field(v, "A")), str(field(v, "H")), str(field(v, "d")));
        for (fam = FAM_INT; fam <= FAM_BALL; fam++) {
            fun_family(f, D, M, fam);
            acb_set_d(z, 1e300);
            CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK);
            check_hull(z, f, m, o);
        }
    }
    jsonl_close(fl);
    adf_ffun_clear(f); adf_fball_clear(x); acb_clear(z);
}

/* Local backend (blocks 8, 9, 5; as tests/test_qclass_arith.c:360-366): the same result as the global ball. */
static void local_vectors(void)
{
    jsonl_file *fl; jsonl_error_t e; size_t i; ulong blocks[] = { 8, 9, 5 };
    adf_modctx_struct *ctx = NULL; adf_ffun_t f; adf_fball_t x, y; acb_t z, zg; adf_rat_t c, R; fmpz_t A, H, d;
    adf_adele_t a; adf_rfun_t phi;
    adf_ffun_init(f); adf_fball_init(x); adf_fball_init(y); acb_init(z); acb_init(zg); adf_rat_init(c);
    adf_rat_init(R); fmpz_init(A); fmpz_init(H); fmpz_init(d); adf_adele_init(a); adf_rfun_init(phi);
    gauss(phi);
    CHECK(adf_modctx_new_blocks(&ctx, blocks, 3) == ADF_OK);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice5/local.jsonl", &fl, &e));
    CHECK(jsonl_count(fl) == 56);
    for (i = 0; i < jsonl_count(fl); i++) {
        const jsonl_value *v = jsonl_record(fl, i), *cn = field(v, "canon");
        ulong D = (ulong) number(field(v, "D")), M = (ulong) number(field(v, "M"));
        const char *m = str(field(v, "m")); int o = boolean(field(v, "o")), fam;
        q_of(c->q, str(field(v, "c"))); q_of(R->q, str(field(v, "R")));
        CHECK(adf_fball_set_center_radius(x, c, R) == ADF_OK);
        CHECK(adf_fball_set_local(y, x, ctx) == ADF_OK && adf_fball_is_local(y));
        adf_fball_get_fmpz3(A, H, d, y);
        CHECK(fmpz_cmp_si(A, strtol(str(at(cn, 0)), NULL, 10)) == 0);
        CHECK(fmpz_cmp_si(H, strtol(str(at(cn, 1)), NULL, 10)) == 0);
        CHECK(fmpz_cmp_si(d, strtol(str(at(cn, 2)), NULL, 10)) == 0);
        for (fam = FAM_INT; fam <= FAM_BALL; fam++) {
            fun_family(f, D, M, fam);
            CHECK(adf_ffun_eval(z, f, y, PREC) == ADF_OK);
            CHECK(adf_ffun_eval(zg, f, x, PREC) == ADF_OK);
            CHECK(acb_equal(z, zg));
            check_hull(z, f, m, o);
            /* the adele with a local finite part and the exact real 0 (phi(0) = 1 exactly) */
            arb_zero(a->inf); adf_fball_set(&a->fin, y);
            CHECK(adf_tensor_eval(z, phi, f, a, PREC) == ADF_OK);
            check_hull(z, f, m, o);
        }
    }
    jsonl_close(fl);
    adf_adele_clear(a); adf_fball_clear(x); adf_fball_clear(y); adf_modctx_free(ctx);
    adf_ffun_clear(f); acb_clear(z); acb_clear(zg); adf_rat_clear(c); adf_rat_clear(R);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); adf_rfun_clear(phi);
}

/* ------------------------------------------------------------------ E1 step 3 and the hand cases */
static void exact_cases(void)
{
    adf_ffun_t f; adf_fball_t x; acb_t z, h, t; adf_rfun_t phi; adf_adele_t a;
    static const double six[6] = { 1, 2, 3, 4, 5, 6 }, jump[2] = { 10, 20 };
    adf_ffun_init(f); adf_fball_init(x); acb_init(z); acb_init(h); acb_init(t); adf_rfun_init(phi);
    adf_adele_init(a);
    fun_vals(f, 2, 3, six);
    /* (D, M) = (2, 3), f = [1..6]: Zhat meets {0, 2, 4}, values {1, 3, 5}: exactly the hull [1, 5] */
    ball3si(x, 0, 1, 1);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK);
    CHECK(exact_ball(z, 3, 2));                                     /* [1, 5] exactly */
    CHECK(!acb_contains_zero(z)); acb_set_si(t, 6); CHECK(!acb_contains(z, t));
    /* (1/4) Zhat meets all six indices and points outside support: contains 0 and 6, exactly [0, 6] */
    ball3si(x, 0, 1, 4);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK);
    CHECK(acb_contains_zero(z)); acb_set_si(t, 6); CHECK(acb_contains(z, t));
    acb_zero(h); acb_union(h, h, t, PREC); CHECK(same_hull(z, h)); CHECK(exact_ball(z, 3, 3));
    /* the exact point 1/5 is outside support: exactly 0 */
    ball3si(x, 1, 0, 5);
    acb_set_si(z, 99);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK && acb_is_zero(z));
    /* a point ball gives one value exactly: 7 = 14/2, index 14 mod 6 = 2, value 3; -1/2 = -1/D: index 5 */
    ball3si(x, 7, 0, 1);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK && acb_equal(z, f->f + 2));
    ball3si(x, -1, 0, 2);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK && acb_equal(z, f->f + 5));
    /* a ball value at a point: the value itself, radius included */
    fun_family(f, 2, 3, FAM_BALL);
    ball3si(x, 7, 0, 1);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK && acb_equal(z, f->f + 2));
    /* the jump of PLAN 4.4: 1 + 2 Zhat ... f = 10 on 2 Zhat, 20 on 1 + 2 Zhat; Zhat crosses the jump: both
       values; its centre 0 alone would give 10 only. The coset 1 + 2 Zhat alone gives 20 exactly. */
    fun_vals(f, 1, 2, jump);
    ball3si(x, 0, 1, 1);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK);
    acb_set_si(t, 10); CHECK(acb_contains(z, t)); acb_set_si(t, 20); CHECK(acb_contains(z, t));
    acb_set_si(t, 9); CHECK(!acb_contains(z, t)); acb_set_si(t, 21); CHECK(!acb_contains(z, t));
    ball3si(x, 1, 2, 1);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK); acb_set_si(t, 20); CHECK(acb_equal(z, t));
    /* (1/2) + Zhat lies outside (1/1) Zhat entirely: no index, exactly 0 */
    ball3si(x, 1, 2, 2);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK && acb_is_zero(z));
    /* the same jump through tensor_eval at the real 0: phi(0) = 1, z = [10, 20] */
    gauss(phi); arb_zero(a->inf); ball3si(&a->fin, 0, 1, 1);
    CHECK(adf_tensor_eval(z, phi, f, a, PREC) == ADF_OK);
    acb_set_si(t, 10); CHECK(acb_contains(z, t)); acb_set_si(t, 20); CHECK(acb_contains(z, t));
    acb_set_si(h, 15); mag_set_ui(arb_radref(acb_realref(h)), 5); CHECK(same_hull(z, h));   /* 1 (15 +/- 5) */
    /* prec 2, 53, the cap, 0, -5 (p = max(prec, 2)) */
    fun_vals(f, 2, 3, six); ball3si(x, 0, 1, 1);
    CHECK(adf_ffun_eval(z, f, x, 2) == ADF_OK);              /* endpoints rounded outward at 2 bits: 5 -> 6 */
    acb_set_si(t, 1); CHECK(acb_contains(z, t)); acb_set_si(t, 5); CHECK(acb_contains(z, t));
    CHECK(adf_ffun_eval(z, f, x, 53) == ADF_OK);
    acb_set_si(h, 1); acb_set_si(t, 5); acb_union(h, h, t, PREC); CHECK(same_hull(z, h));
    CHECK(adf_ffun_eval(z, f, x, ADF_REAL_PREC_MAX) == ADF_OK && same_hull(z, h));
    CHECK(adf_ffun_eval(z, f, x, 0) == ADF_OK && acb_contains(z, h));
    CHECK(adf_ffun_eval(z, f, x, -5) == ADF_OK && acb_contains(z, h));
    adf_ffun_clear(f); adf_fball_clear(x); acb_clear(z); acb_clear(h); acb_clear(t); adf_rfun_clear(phi);
    adf_adele_clear(a);
}

/* ------------------------------------------------------------------ E1 step 4 (D3): vectors */
/* sball of the record: places (p, centre, e or null); real component r (NULL: arch NONE). */
static void sball_json(adf_sball_t s, const jsonl_value *loc, const arb_t r)
{
    slong n = (slong) jsonl_size(loc), i; adf_lball_struct *l = flint_malloc((size_t) (n ? n : 1) * sizeof(*l));
    adf_rat_t c; adf_place_t v, where;
    adf_rat_init(c);
    for (i = 0; i < n; i++) {
        const jsonl_value *t = at(loc, (size_t) i);
        adf_lball_init(l + i);
        CHECK(adf_place_prime(&v, (ulong) number(at(t, 0))) == ADF_OK);
        q_of(c->q, str(at(t, 1)));
        if (is_null(at(t, 2))) CHECK(adf_lball_set_rat(l + i, v, c) == ADF_OK);
        else CHECK(adf_lball_set_rat_ball(l + i, v, c, number(at(t, 2))) == ADF_OK);
    }
    CHECK(adf_sball_set_arb_lballs(s, &where, r, l, n) == ADF_OK);
    CHECK(adf_sball_is_canonical(s));
    for (i = 0; i < n; i++) adf_lball_clear(l + i);
    flint_free(l); adf_rat_clear(c);
}
static void sball_vectors(void)
{
    jsonl_file *fl; jsonl_error_t e; size_t i; int fam; unsigned long nproj = 0, nloc = 0, nagree = 0;
    adf_ffun_t f; adf_sball_t s; adf_rfun_t phi; acb_t z, za, w; arb_t r; adf_adele_t a; adf_place_t pl[3], where;
    adf_ffun_init(f); adf_sball_init(s); adf_rfun_init(phi); acb_init(z); acb_init(za); acb_init(w); arb_init(r);
    adf_adele_init(a);
    gauss(phi);
    pl[0] = adf_place_inf();
    CHECK(adf_place_prime(pl + 1, 2) == ADF_OK); CHECK(adf_place_prime(pl + 2, 3) == ADF_OK);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice5/sball.jsonl", &fl, &e));
    CHECK(jsonl_count(fl) == 140);
    for (i = 0; i < jsonl_count(fl); i++) {
        const jsonl_value *v = jsonl_record(fl, i);
        ulong D = (ulong) number(field(v, "D")), M = (ulong) number(field(v, "M"));
        const char *m = str(field(v, "m"));
        if (has_field(v, "proj")) {
            const jsonl_value *t = field(v, "proj");
            arb_zero(a->inf); ball3(&a->fin, str(at(t, 0)), str(at(t, 1)), str(at(t, 2)));
            CHECK(adf_sball_project(s, &where, a, pl, 3) == ADF_OK);
            nproj++;
        } else {
            arb_zero(r); sball_json(s, field(v, "loc"), r);
            nloc++;
        }
        CHECK(adf_sball_arch(s) == ADF_ARCH_REAL);
        for (fam = FAM_INT; fam <= FAM_BALL; fam++) {
            fun_family(f, D, M, fam);
            CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_OK);     /* phi(0) = 1 exactly */
            check_hull(z, f, m, 1);                                         /* zero always included */
            if (has_field(v, "proj")) {
                CHECK(adf_tensor_eval(za, phi, f, a, PREC) == ADF_OK);
                check_hull(za, f, str(field(v, "am")), boolean(field(v, "ao")));
                widen(w, z, -120); CHECK(acb_contains(w, za));    /* the same sets up to rounding */
                if (!strcmp(m, str(field(v, "am")))) nagree++;   /* then z is the hull of za and 0 (above) */
            }
        }
    }
    CHECK(nproj == 49 && nloc == 91 && nagree == 3 * 46);
    jsonl_close(fl);
    adf_ffun_clear(f); adf_sball_clear(s); adf_rfun_clear(phi); acb_clear(z); acb_clear(za); acb_clear(w);
    arb_clear(r); adf_adele_clear(a);
}

/* E1 step 4 by hand: (2, 3), [1..6], p = 3 with 1 + 3 Z_3 keeps {2, 5} (the oracle's check, functions4_checks.py
   :440) and zero; no places keeps everything and zero. An exact point at 2 that is 1/4 keeps nothing (v_2(j/2 -
   1/4) = -2 < 0 = v_2(3)): exactly 0. Arch NONE with the Gaussian: R = 1, the box [-1, 1] + i [-1, 1]. */
static void sball_cases(void)
{
    adf_ffun_t f; adf_sball_t s; adf_rfun_t phi; acb_t z, h, t; arb_t r; adf_lball_t l; adf_rat_t c;
    adf_place_t v, where;
    static const double six[6] = { 1, 2, 3, 4, 5, 6 };
    adf_ffun_init(f); adf_sball_init(s); adf_rfun_init(phi); acb_init(z); acb_init(h); acb_init(t); arb_init(r);
    adf_lball_init(l); adf_rat_init(c);
    fun_vals(f, 2, 3, six); gauss(phi);
    CHECK(adf_place_prime(&v, 3) == ADF_OK); fmpq_set_si(c->q, 1, 1);
    CHECK(adf_lball_set_rat_ball(l, v, c, 1) == ADF_OK);
    arb_zero(r); CHECK(adf_sball_set_arb_lballs(s, &where, r, l, 1) == ADF_OK);
    CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_OK);
    acb_zero(h); acb_set_si(t, 3); acb_union(h, h, t, PREC); acb_set_si(t, 6); acb_union(h, h, t, PREC);
    CHECK(same_hull(z, h));
    CHECK(adf_sball_set_arb_lballs(s, &where, r, l, 0) == ADF_OK);
    CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_OK);
    acb_zero(h); acb_set_si(t, 6); acb_union(h, h, t, PREC); CHECK(same_hull(z, h));
    CHECK(adf_place_prime(&v, 2) == ADF_OK); fmpq_set_si(c->q, 1, 4);
    CHECK(adf_lball_set_rat(l, v, c) == ADF_OK);
    CHECK(adf_sball_set_arb_lballs(s, &where, r, l, 1) == ADF_OK);
    acb_set_si(z, 7);
    CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_OK && acb_is_zero(z));
    /* arch NONE, Gaussian, no places: [-1, 1] (1 + i) times [0, 6] */
    CHECK(adf_sball_set_arb_lballs(s, &where, NULL, l, 0) == ADF_OK && adf_sball_arch(s) == ADF_ARCH_NONE);
    fun_vals(f, 1, 1, six);
    CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_OK);
    acb_zero(h); arb_add_error_2exp_si(acb_realref(h), 0); arb_add_error_2exp_si(acb_imagref(h), 0);
    CHECK(acb_contains(z, h));
    widen(h, h, -100); acb_zero(t); acb_union(t, t, f->f, PREC); acb_mul(h, h, t, EXP); widen(h, h, -100);
    CHECK(acb_contains(h, z));
    adf_ffun_clear(f); adf_sball_clear(s); adf_rfun_clear(phi); acb_clear(z); acb_clear(h); acb_clear(t);
    arb_clear(r); adf_lball_clear(l); adf_rat_clear(c);
}

/* Arch NONE: the bound of E1 step 5 against 1000 sampled real points of each phi of the vectors, and of a drift
   exp(-pi x^2 + 4 x) whose maximum exp(4/pi) = 3.57 exceeds the bound without the factor exp(beta^2/(2 alpha)). */
static void none_bound_one(const adf_rfun_t phi, double lo, double hi)
{
    adf_ffun_t f; adf_sball_t s; acb_t z, y; arb_t t; adf_place_t where; slong k;
    static const double one[1] = { 1 };
    adf_ffun_init(f); adf_sball_init(s); acb_init(z); acb_init(y); arb_init(t);
    fun_vals(f, 1, 1, one);
    CHECK(adf_sball_set_arb_lballs(s, &where, NULL, NULL, 0) == ADF_OK);
    CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_OK);
    for (k = 0; k < 1000; k++) {
        arb_set_d(t, lo + (hi - lo) * (double) k / 999.0);
        CHECK(adf_rfun_eval(y, phi, t, PREC) == ADF_OK);
        CHECK(acb_contains(z, y));
    }
    adf_ffun_clear(f); adf_sball_clear(s); acb_clear(z); acb_clear(y); arb_clear(t);
}
static void none_bounds(void)
{
    jsonl_file *fl; jsonl_error_t e; size_t i; adf_rfun_t phi; unsigned long n = 0;
    static const double one[1] = { 1 }, cube[4] = { 0.5, -1, 0, 2 };
    adf_rfun_init(phi);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice5/tensor.jsonl", &fl, &e));
    for (i = 0; i < jsonl_count(fl); i++) {
        const jsonl_value *v = jsonl_record(fl, i);
        if (strcmp(str(field(v, "k")), "int")) continue;
        rfun_json(phi, field(v, "terms"));
        none_bound_one(phi, -6, 6); n++;
    }
    CHECK(n == 5);
    jsonl_close(fl);
    fun_simple(phi, one, 1, 1, 0, 4, 0); none_bound_one(phi, -1, 3);
    fun_simple(phi, one, 1, 1, 0, -4, 0); none_bound_one(phi, -3, 1);
    fun_simple(phi, cube, 4, 0.25, 0.5, 1, 0.5); none_bound_one(phi, -10, 10);
    adf_rfun_clear(phi);
}

/* ------------------------------------------------------------------ tensor_eval: vectors */
/* (2, 3), [1..6] at Zhat, (1/4) Zhat, 1/5, 7 (E1 step 3 and a point): masks by hand from step 1. */
static const char *const TB[4][4] = { { "0", "1", "1", "101010" }, { "0", "1", "4", "111111" },
                                      { "1", "0", "5", "000000" }, { "7", "0", "1", "001000" } };
static const int TBO[4] = { 0, 1, 1, 0 };
static void tensor_vectors(void)
{
    jsonl_file *fl; jsonl_error_t e; size_t i; int b; unsigned long nv = 0, ni = 0;
    adf_ffun_t f; adf_rfun_t phi; adf_adele_t a; acb_t z, ref, h, w, u, fi; arb_t n, fn, rn; fmpq_t q;
    adf_ffun_init(f); adf_rfun_init(phi); adf_adele_init(a); acb_init(z); acb_init(ref); acb_init(h); acb_init(w);
    acb_init(u); acb_init(fi); arb_init(n); arb_init(fn); arb_init(rn); fmpq_init(q);
    fun_family(f, 2, 3, FAM_INT);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice5/tensor.jsonl", &fl, &e));
    CHECK(jsonl_count(fl) == 25);
    for (i = 0; i < jsonl_count(fl); i++) {
        const jsonl_value *v = jsonl_record(fl, i);
        rfun_json(phi, field(v, "terms"));
        if (!strcmp(str(field(v, "k")), "val")) {
            q_of(q, str(field(v, "x"))); ref_ball(ref, field(v, "v"));
            for (b = 0; b < 4; b++) {
                ulong j;
                arb_set_fmpq(a->inf, q, PREC); ball3(&a->fin, TB[b][0], TB[b][1], TB[b][2]);
                CHECK(adf_tensor_eval(z, phi, f, a, PREC) == ADF_OK);
                for (j = 0; j < 6; j++)
                    if (TB[b][3][j] == '1') { acb_mul(u, ref, f->f + j, EXP); CHECK(acb_overlaps(z, u)); }
                if (TBO[b]) CHECK(acb_contains_zero(z));
                hull_ref(h, f, TB[b][3], TBO[b]); acb_mul(w, ref, h, EXP); widen(w, w, -96);
                CHECK(acb_contains(w, z));
                /* the product of the components, operation by operation */
                CHECK(adf_rfun_eval(u, phi, a->inf, PREC) == ADF_OK);
                CHECK(adf_ffun_eval(fi, f, &a->fin, PREC) == ADF_OK);
                acb_mul(u, u, fi, PREC); CHECK(acb_equal(z, u));
                /* a real ball of radius 2^-10 around x: the values at both ends, times each selected f[j] */
                arb_add_error_2exp_si(a->inf, -10);
                CHECK(adf_tensor_eval(z, phi, f, a, PREC) == ADF_OK);
                {   arb_t t; arb_init(t); int s;
                    for (s = -1; s <= 1; s += 2) {
                        arb_set_fmpq(t, q, EXP); arb_set_si(acb_realref(u), s);
                        arb_mul_2exp_si(acb_realref(u), acb_realref(u), -10);
                        arb_add(t, t, acb_realref(u), EXP);
                        CHECK(adf_rfun_eval(u, phi, t, EXP) == ADF_OK);
                        for (j = 0; j < 6; j++)
                            if (TB[b][3][j] == '1') { acb_mul(w, u, f->f + j, EXP); CHECK(acb_contains(z, w)); }
                    }
                    arb_clear(t); }
            }
            nv++;
        } else {
            /* tensor integral = rfun integral times (1/M) sum f[j] = 21/3 = 7; tensor norm2 times 91/3 */
            ref_ball(ref, field(v, "i"));
            CHECK(adf_tensor_integral(z, phi, f, PREC) == ADF_OK);
            acb_mul_si(u, ref, 7, EXP); CHECK(acb_overlaps(z, u)); widen(w, u, -96); CHECK(acb_contains(w, z));
            CHECK(adf_rfun_integral(u, phi, PREC) == ADF_OK); CHECK(adf_ffun_integral(fi, f, PREC) == ADF_OK);
            CHECK(acb_is_exact(fi) && acb_equal_si(fi, 7));
            ref_ball(ref, field(v, "n"));
            CHECK(adf_tensor_norm2(n, phi, f, PREC) == ADF_OK);
            fmpq_set_si(q, 91, 3); arb_set_fmpq(rn, q, EXP); arb_mul(rn, rn, acb_realref(ref), EXP);
            CHECK(arb_overlaps(n, rn)); CHECK(arb_is_nonnegative(n));
            acb_set_arb(u, rn); widen(w, u, -96); CHECK(arb_contains(acb_realref(w), n));
            CHECK(adf_ffun_norm2(fn, f, PREC) == ADF_OK && arb_contains_fmpq(fn, q));
            ni++;
        }
    }
    CHECK(nv == 20 && ni == 5);
    jsonl_close(fl);
    adf_ffun_clear(f); adf_rfun_clear(phi); adf_adele_clear(a); acb_clear(z); acb_clear(ref); acb_clear(h);
    acb_clear(w); acb_clear(u); acb_clear(fi); arb_clear(n); arb_clear(fn); arb_clear(rn); fmpq_clear(q);
}

/* ------------------------------------------------------------------ integrals and norms */
static void integral_vectors(void)
{
    jsonl_file *fl; jsonl_error_t e; size_t i, j;
    adf_ffun_t f, g; acb_ptr vals; acb_t z; arb_t n, ng; fmpq_t q; mag_t r;
    adf_ffun_init(f); adf_ffun_init(g); acb_init(z); arb_init(n); arb_init(ng); fmpq_init(q); mag_init(r);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice5/integral.jsonl", &fl, &e));
    CHECK(jsonl_count(fl) == 14);
    for (i = 0; i < jsonl_count(fl); i++) {
        const jsonl_value *v = jsonl_record(fl, i), *fv = field(v, "f");
        ulong D = (ulong) number(field(v, "D")), M = (ulong) number(field(v, "M"));
        vals = _acb_vec_init((slong) jsonl_size(fv));
        for (j = 0; j < jsonl_size(fv); j++) in_num(vals + j, at(fv, j));
        CHECK(adf_ffun_set_acb_vec(f, D, M, vals, (slong) jsonl_size(fv)) == ADF_OK);
        CHECK(adf_ffun_integral(z, f, PREC) == ADF_OK);
        q_of(q, str(at(field(v, "i"), 0))); CHECK(arb_contains_fmpq(acb_realref(z), q));
        if (fmpz_is_one(fmpq_denref(q)) || fmpz_is_pm1(fmpq_denref(q))) CHECK(arb_is_exact(acb_realref(z)));
        arb_get_mag(r, acb_realref(z)); CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(z)), -110) <= 0);
        q_of(q, str(at(field(v, "i"), 1))); CHECK(arb_contains_fmpq(acb_imagref(z), q));
        CHECK(mag_cmp_2exp_si(arb_radref(acb_imagref(z)), -110) <= 0);
        CHECK(adf_ffun_norm2(n, f, PREC) == ADF_OK);
        q_of(q, str(field(v, "n"))); CHECK(arb_contains_fmpq(n, q)); CHECK(arb_is_nonnegative(n));
        CHECK(mag_cmp_2exp_si(arb_radref(n), -100) <= 0);
        if (M == 1 || M == 4) CHECK(arb_is_exact(n) && acb_is_exact(z));        /* dyadic: exact */
        /* Parseval (analysis P4:158): the norm of the direct transform of slice 4a, weight 1/D */
        CHECK(adf_ffun_fourier(g, f, PREC) == ADF_OK);
        CHECK(adf_ffun_norm2(ng, g, PREC) == ADF_OK);
        CHECK(arb_overlaps(n, ng)); CHECK(arb_contains_fmpq(ng, q));
        _acb_vec_clear(vals, (slong) jsonl_size(fv));
    }
    jsonl_close(fl);
    /* the norm is clipped: a value 0 +/- 1 has the true norm in [0, 1]; the enclosure starts at 0, not below */
    {   acb_t b; acb_init(b); arb_add_error_2exp_si(acb_realref(b), 0);
        CHECK(adf_ffun_set_acb_vec(f, 1, 1, b, 1) == ADF_OK);
        CHECK(adf_ffun_norm2(n, f, PREC) == ADF_OK && arb_is_nonnegative(n));
        arb_one(ng); CHECK(arb_contains(n, ng));
        {   adf_rfun_t phi; adf_rfun_init(phi); gauss(phi);
            CHECK(adf_tensor_norm2(n, phi, f, PREC) == ADF_OK && arb_is_nonnegative(n));
            adf_rfun_clear(phi); }
        acb_clear(b); }
    /* the faults_44 example exactly: (2, 3), [1..6]: integral 7, norm 91/3 (functions4_checks.py:604-605) */
    fun_family(f, 2, 3, FAM_INT);
    CHECK(adf_ffun_integral(z, f, PREC) == ADF_OK && acb_is_exact(z) && acb_equal_si(z, 7));
    CHECK(adf_ffun_norm2(n, f, PREC) == ADF_OK); fmpq_set_si(q, 91, 3); CHECK(arb_contains_fmpq(n, q));
    /* the zero function: exactly zero */
    adf_ffun_clear(f); adf_ffun_init(f);
    CHECK(adf_ffun_integral(z, f, PREC) == ADF_OK && acb_is_zero(z));
    CHECK(adf_ffun_norm2(n, f, PREC) == ADF_OK && arb_is_zero(n));
    adf_ffun_clear(f); adf_ffun_clear(g); acb_clear(z); arb_clear(n); arb_clear(ng); fmpq_clear(q); mag_clear(r);
}

/* ------------------------------------------------------------------ statuses: outputs untouched */
static acb_struct snap_z; static arb_struct snap_n;
static void sentinel(acb_t z, arb_t n)
{
    acb_set_d_d(z, 0.125, -3.5); arb_set_d(n, 17.25); snap_z = *z; snap_n = *n;
}
static int untouched(const acb_t z, const arb_t n)
{
    acb_t c; arb_t d; int ok;
    acb_init(c); arb_init(d); acb_set_d_d(c, 0.125, -3.5); arb_set_d(d, 17.25);
    ok = acb_equal(z, c) && arb_equal(n, d) && !memcmp(z, &snap_z, sizeof(snap_z))
         && !memcmp(n, &snap_n, sizeof(snap_n));
    acb_clear(c); arb_clear(d); return ok;
}
/* All seven calls with the same inputs; each must return st and leave its output untouched. */
static void all_seven(int st, const adf_rfun_t phi, const adf_ffun_t f, const adf_fball_t x, const adf_adele_t a,
                      const adf_sball_t s, slong prec, int skip)
{
    acb_t z; arb_t n; acb_init(z); arb_init(n);
#define SEVEN(call) CHECK((call) == st && (st == ADF_OK || untouched(z, n)))
    if (!(skip & 1)) { sentinel(z, n); SEVEN(adf_ffun_eval(z, f, x, prec)); }
    if (!(skip & 2)) { sentinel(z, n); SEVEN(adf_tensor_eval(z, phi, f, a, prec)); }
    if (!(skip & 4)) { sentinel(z, n); SEVEN(adf_tensor_eval_sball(z, phi, f, s, prec)); }
    if (!(skip & 8)) { sentinel(z, n); SEVEN(adf_ffun_integral(z, f, prec)); }
    if (!(skip & 16)) { sentinel(z, n); SEVEN(adf_ffun_norm2(n, f, prec)); }
    if (!(skip & 32)) { sentinel(z, n); SEVEN(adf_tensor_integral(z, phi, f, prec)); }
    if (!(skip & 64)) { sentinel(z, n); SEVEN(adf_tensor_norm2(n, phi, f, prec)); }
#undef SEVEN
    acb_clear(z); arb_clear(n);
}
static void statuses(void)
{
    adf_rfun_t phi, big; adf_ffun_t f; adf_fball_t x; adf_adele_t a; adf_sball_t s, sc; acb_t z; arb_t n, r;
    adf_place_t where; static const double one[1] = { 1 };
    adf_rfun_init(phi); adf_rfun_init(big); adf_ffun_init(f); adf_fball_init(x); adf_adele_init(a);
    adf_sball_init(s); adf_sball_init(sc); acb_init(z); arb_init(n); arb_init(r);
    gauss(phi); fun_family(f, 2, 3, FAM_CPLX); ball3si(x, 1, 3, 2); arb_set_d(a->inf, 0.25);
    ball3si(&a->fin, 1, 3, 2); arb_set_d(r, 0.25);
    CHECK(adf_sball_set_arb_lballs(s, &where, r, NULL, 0) == ADF_OK);
    /* prec: 2, 53, 0, -5 and the cap run; one above the cap is LIMIT for all seven */
    all_seven(ADF_OK, phi, f, x, a, s, 2, 0) ; all_seven(ADF_OK, phi, f, x, a, s, 53, 0);
    all_seven(ADF_LIMIT, phi, f, x, a, s, ADF_REAL_PREC_MAX + 1, 0);
    all_seven(ADF_LIMIT, phi, f, x, a, s, WORD_MAX, 0);
    {   acb_t z2; arb_t n2; acb_init(z2); arb_init(n2);
        CHECK(adf_tensor_eval(z, phi, f, a, ADF_REAL_PREC_MAX) == ADF_OK);
        CHECK(adf_tensor_eval(z2, phi, f, a, 0) == ADF_OK && acb_overlaps(z, z2));
        CHECK(adf_tensor_eval(z2, phi, f, a, -5) == ADF_OK && acb_overlaps(z, z2));
        CHECK(adf_tensor_eval_sball(z, phi, f, s, ADF_REAL_PREC_MAX) == ADF_OK);
        CHECK(adf_tensor_norm2(n, phi, f, ADF_REAL_PREC_MAX) == ADF_OK);
        CHECK(adf_tensor_norm2(n2, phi, f, 2) == ADF_OK && arb_overlaps(n, n2));
        acb_clear(z2); arb_clear(n2); }
    /* COMPLEX arch: DOMAIN, z untouched; LIMIT first */
    sc->arch = ADF_ARCH_COMPLEX; acb_set_d_d(sc->inf, 0.25, 1);
    CHECK(adf_sball_is_canonical(sc));
    sentinel(z, n); CHECK(adf_tensor_eval_sball(z, phi, f, sc, PREC) == ADF_DOMAIN && untouched(z, n));
    sentinel(z, n);
    CHECK(adf_tensor_eval_sball(z, phi, f, sc, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT && untouched(z, n));
    /* NOT_DETERMINED: C = 10^300 overflows exp; z untouched (the finite part is computed first) */
    fun_simple(big, one, 1, 1, 0, 0, 1e300);
    all_seven(ADF_NOT_DETERMINED, big, f, x, a, s, PREC, 1 | 8 | 16);
    CHECK(adf_sball_set_arb_lballs(sc, &where, NULL, NULL, 0) == ADF_OK);
    sentinel(z, n); CHECK(adf_tensor_eval_sball(z, big, f, sc, PREC) == ADF_NOT_DETERMINED && untouched(z, n));
#ifndef ADF_CHECK_INVARIANTS
    /* a nonfinite raw real input: DOMAIN (a precondition that INV would reject first) */
    arb_indeterminate(a->inf);
    sentinel(z, n); CHECK(adf_tensor_eval(z, phi, f, a, PREC) == ADF_DOMAIN && untouched(z, n));
    sentinel(z, n); CHECK(adf_tensor_eval(z, phi, f, a, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT && untouched(z, n));
#endif
    /* the zero rfun: every tensor result is exactly 0 */
    adf_rfun_clear(big); adf_rfun_init(big); arb_set_d(a->inf, 0.25);
    CHECK(adf_tensor_eval(z, big, f, a, PREC) == ADF_OK && acb_is_zero(z));
    CHECK(adf_tensor_eval_sball(z, big, f, s, PREC) == ADF_OK && acb_is_zero(z));
    CHECK(adf_tensor_eval_sball(z, big, f, sc, PREC) == ADF_OK && acb_is_zero(z));
    CHECK(adf_tensor_integral(z, big, f, PREC) == ADF_OK && acb_is_zero(z));
    CHECK(adf_tensor_norm2(n, big, f, PREC) == ADF_OK && arb_is_zero(n));
    adf_rfun_clear(phi); adf_rfun_clear(big); adf_ffun_clear(f); adf_fball_clear(x); adf_adele_clear(a);
    adf_sball_clear(s); adf_sball_clear(sc); acb_clear(z); arb_clear(n); arb_clear(r);
}

/* ------------------------------------------------------------------ caps of D1 */
static void caps(void)
{
    adf_rfun_t phi, many; adf_ffun_t f, forged; adf_fball_t x; adf_adele_t a; adf_sball_t s; acb_t z; arb_t n;
    adf_lball_struct l[3]; adf_rat_t c; adf_place_t v, where; fmpz_t A, H, d; slong i; acb_ptr small;
    adf_rfun_init(phi); adf_rfun_init(many); adf_ffun_init(f); adf_fball_init(x); adf_adele_init(a);
    adf_sball_init(s); acb_init(z); arb_init(n); adf_rat_init(c); fmpz_init(A); fmpz_init(H); fmpz_init(d);
    gauss(phi); ball3si(x, 0, 1, 1); arb_zero(a->inf); ball3si(&a->fin, 0, 1, 1);
    CHECK(adf_sball_set_arb_lballs(s, &where, acb_realref(z), NULL, 0) == ADF_OK);
    /* an ffun of 2^20 + 1 entries (forged: the cap is decided before any entry is read) is LIMIT for all seven */
    small = _acb_vec_init(2);
    forged->D = 1; forged->M = ADF_FFUN_ITEMS_MAX + 1; forged->f = small;
    all_seven(ADF_LIMIT, phi, forged, x, a, s, PREC, 0);
    forged->D = ADF_FFUN_ITEMS_MAX + 1; forged->M = 1;
    all_seven(ADF_LIMIT, phi, forged, x, a, s, PREC, 0);
    _acb_vec_clear(small, 2);
    /* exactly 2^20 entries pass */
    small = _acb_vec_init((slong) ADF_FFUN_ITEMS_MAX);
    CHECK(adf_ffun_set_acb_vec(f, 1, ADF_FFUN_ITEMS_MAX, small, (slong) ADF_FFUN_ITEMS_MAX) == ADF_OK);
    _acb_vec_clear(small, (slong) ADF_FFUN_ITEMS_MAX);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK && acb_is_zero(z));
    CHECK(adf_ffun_integral(z, f, PREC) == ADF_OK && acb_is_zero(z));
    /* work units: one per index and supplied prime; 2^20 entries with one prime pass, with two do not */
    for (i = 0; i < 3; i++) adf_lball_init(l + i);
    fmpq_set_si(c->q, 1, 3);
    CHECK(adf_place_prime(&v, 2) == ADF_OK); CHECK(adf_lball_set_rat_ball(l, v, c, 3) == ADF_OK);
    CHECK(adf_place_prime(&v, 5) == ADF_OK); CHECK(adf_lball_set_rat_ball(l + 1, v, c, 1) == ADF_OK);
    CHECK(adf_place_prime(&v, 7) == ADF_OK); CHECK(adf_lball_set_rat_ball(l + 2, v, c, 1) == ADF_OK);
    arb_zero(n);
    CHECK(adf_sball_set_arb_lballs(s, &where, n, l, 1) == ADF_OK);
    CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_OK && acb_is_zero(z));
    CHECK(adf_sball_set_arb_lballs(s, &where, n, l, 2) == ADF_OK);
    acb_one(z); CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_LIMIT && acb_is_one(z));
    small = _acb_vec_init((slong) ADF_FFUN_ITEMS_MAX / 2);
    CHECK(adf_ffun_set_acb_vec(f, 1, ADF_FFUN_ITEMS_MAX / 2, small, (slong) ADF_FFUN_ITEMS_MAX / 2) == ADF_OK);
    _acb_vec_clear(small, (slong) ADF_FFUN_ITEMS_MAX / 2);
    CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_OK && acb_is_zero(z));
    CHECK(adf_sball_set_arb_lballs(s, &where, n, l, 3) == ADF_OK);
    acb_one(z); CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_LIMIT && acb_is_one(z));
    /* bits: a raw field of ADF_FFUN_BITS_MAX - 128 bits passes, one more bit is LIMIT (also in the adele) */
    fun_family(f, 2, 3, FAM_INT);
    fmpz_one(A); fmpz_mul_2exp(A, A, (ulong) ADF_FFUN_BITS_MAX - 129); fmpz_add_ui(A, A, 1); fmpz_one(d);
    CHECK(fmpz_bits(A) == (ulong) ADF_FFUN_BITS_MAX - 128);
    CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK && acb_equal(z, f->f + 2 * ((fmpz_fdiv_ui(A, 3)))));
    adf_fball_set(&a->fin, x); CHECK(adf_tensor_eval(z, phi, f, a, PREC) == ADF_OK);
    fmpz_mul_2exp(A, A, 1);
    CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    acb_one(z); CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_LIMIT && acb_is_one(z));
    adf_fball_set(&a->fin, x); CHECK(adf_tensor_eval(z, phi, f, a, PREC) == ADF_LIMIT && acb_is_one(z));
    fmpz_set_ui(H, 7); fmpz_swap(A, d);                    /* a huge denominator d: also LIMIT */
    CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_LIMIT && acb_is_one(z));
    /* an lball centre whose numerator has more than ADF_FFUN_BITS_MAX - 128 bits: LIMIT */
    fmpz_one(fmpq_numref(c->q)); fmpz_mul_2exp(fmpq_numref(c->q), fmpq_numref(c->q), (ulong) ADF_FFUN_BITS_MAX);
    fmpz_add_ui(fmpq_numref(c->q), fmpq_numref(c->q), 1); fmpz_one(fmpq_denref(c->q));
    CHECK(adf_place_prime(&v, 3) == ADF_OK); CHECK(adf_lball_set_rat(l, v, c) == ADF_OK);
    CHECK(adf_sball_set_arb_lballs(s, &where, n, l, 1) == ADF_OK);
    CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_LIMIT && acb_is_one(z));
    /* rfun caps of D1: 2^16 + 1 terms (zero polynomials, forged length) are LIMIT for the four tensor calls */
    many->len = ADF_RFUN_TERMS_MAX + 1;
    many->term = (adf_rterm_struct *) flint_malloc((size_t) many->len * sizeof(adf_rterm_struct));
    for (i = 0; i < many->len; i++) {
        acb_poly_init(many->term[i].P); acb_init(many->term[i].A); acb_one(many->term[i].A);
        acb_init(many->term[i].B); acb_init(many->term[i].C);
    }
    ball3si(x, 0, 1, 1); ball3si(&a->fin, 0, 1, 1);
    CHECK(adf_sball_set_arb_lballs(s, &where, n, NULL, 0) == ADF_OK);
    all_seven(ADF_LIMIT, many, f, x, a, s, PREC, 1 | 8 | 16);
    CHECK(adf_sball_set_arb_lballs(s, &where, NULL, NULL, 0) == ADF_OK);
    all_seven(ADF_LIMIT, many, f, x, a, s, PREC, 1 | 2 | 8 | 16 | 32 | 64);
    many->len = ADF_RFUN_TERMS_MAX;                       /* exactly 2^16 terms pass (the NONE bound: 0) */
    CHECK(adf_tensor_eval_sball(z, many, f, s, PREC) == ADF_OK && acb_is_zero(z));
    many->len = ADF_RFUN_TERMS_MAX + 1;
    for (i = 0; i < 3; i++) adf_lball_clear(l + i);
    adf_rfun_clear(phi); adf_rfun_clear(many); adf_ffun_clear(f); adf_fball_clear(x); adf_adele_clear(a);
    adf_sball_clear(s); acb_clear(z); arb_clear(n); adf_rat_clear(c); fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}

/* ------------------------------------------------------------------ storage bounds (mutation survivors) */
/* The storage of f followed by a nonzero acb that no call may read (entry L): a loop to j <= L would add it.
   And long mantissas (prec 1024, imaginary parts 1/3): the hull's endpoint storage is then allocated, so a wrong
   clear is a leak or a double free under SAN. */
static void guard_cases(void)
{
    adf_ffun_t f; acb_ptr big, keep; adf_fball_t x; adf_sball_t s; adf_rfun_t phi; acb_t z; arb_t n;
    adf_place_t where; slong L = 6, j; fmpq_t q;
    adf_ffun_init(f); adf_fball_init(x); adf_sball_init(s); adf_rfun_init(phi); acb_init(z); arb_init(n);
    fmpq_init(q);
    gauss(phi); fun_family(f, 2, 3, FAM_INT);
    big = _acb_vec_init(L + 1); _acb_vec_set(big, f->f, L); acb_set_d(big + L, 1e10);
    keep = f->f; f->f = big;
    CHECK(adf_ffun_integral(z, f, PREC) == ADF_OK && acb_equal_si(z, 7));
    CHECK(adf_ffun_norm2(n, f, PREC) == ADF_OK); fmpq_set_si(q, 91, 3); CHECK(arb_contains_fmpq(n, q));
    CHECK(mag_cmp_2exp_si(arb_radref(n), -100) <= 0);
    CHECK(adf_tensor_integral(z, phi, f, PREC) == ADF_OK && acb_equal_si(z, 7));
    ball3si(x, 0, 1, 4);                                  /* every index and 0: [0, 6] */
    CHECK(adf_ffun_eval(z, f, x, PREC) == ADF_OK && exact_ball(z, 3, 3));
    CHECK(adf_sball_set_arb_lballs(s, &where, NULL, NULL, 0) == ADF_OK);
    CHECK(adf_tensor_eval_sball(z, phi, f, s, PREC) == ADF_OK);
    {   mag_t m; mag_init(m); acb_get_mag(m, z); CHECK(mag_cmp_2exp_si(m, 4) <= 0); mag_clear(m); }   /* <= 6 R */
    f->f = keep; _acb_vec_clear(big, L + 1);
    /* long mantissas in both parts: (j + 1)/3 + i (j + 2)/3 at 1024 bits, the hull at prec 1024 */
    {   acb_ptr v = _acb_vec_init(L);
        for (j = 0; j < L; j++) {
            fmpq_set_si(q, j + 1, 3); arb_set_fmpq(acb_realref(v + j), q, 1024);
            fmpq_set_si(q, j + 2, 3); arb_set_fmpq(acb_imagref(v + j), q, 1024);
        }
        CHECK(adf_ffun_set_acb_vec(f, 2, 3, v, L) == ADF_OK);
        ball3si(x, 0, 1, 1);
        CHECK(adf_ffun_eval(z, f, x, 1024) == ADF_OK);
        for (j = 0; j < L; j += 2) CHECK(acb_contains(z, v + j));
        CHECK(adf_tensor_eval_sball(z, phi, f, s, 1024) == ADF_OK && acb_contains_zero(z));
        _acb_vec_clear(v, L); }
    adf_ffun_clear(f); adf_fball_clear(x); adf_sball_clear(s); adf_rfun_clear(phi); acb_clear(z); arb_clear(n);
    fmpq_clear(q);
}

/* ------------------------------------------------------------------ INV: entry predicates */
static void debug(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int k;
    for (k = 0; k < 14; k++) {
        pid_t p = fork(); int status; CHECK(p >= 0);
        if (!p) {
            adf_rfun_t phi; adf_ffun_t f; adf_fball_t x; adf_adele_t a; adf_sball_t s; acb_t z; arb_t n;
            adf_rfun_init(phi); adf_ffun_init(f); adf_fball_init(x); adf_adele_init(a); adf_sball_init(s);
            acb_init(z); arb_init(n); gauss(phi); fun_family(f, 2, 3, FAM_INT);
            if (k == 0) { acb_indeterminate(f->f + 1); (void) adf_ffun_eval(z, f, x, PREC); }
            if (k == 1) { fmpz_zero(x->d); (void) adf_ffun_eval(z, f, x, PREC); }
            if (k == 2) { arb_indeterminate(a->inf); (void) adf_tensor_eval(z, phi, f, a, PREC); }
            if (k == 3) { s->arch = 5; (void) adf_tensor_eval_sball(z, phi, f, s, PREC); }
            if (k == 4) { acb_indeterminate(f->f); (void) adf_ffun_integral(z, f, PREC); }
            if (k == 5) { acb_indeterminate(f->f); (void) adf_ffun_norm2(n, f, PREC); }
            arb_neg(acb_realref(phi->term[0].A), acb_realref(phi->term[0].A));      /* Re(A) < 0 */
            if (k == 6) (void) adf_tensor_integral(z, phi, f, PREC);
            if (k == 7) (void) adf_tensor_norm2(n, phi, f, PREC);
            if (k == 8) (void) adf_tensor_eval_sball(z, phi, f, s, PREC);
            if (k == 9) (void) adf_tensor_eval(z, phi, f, a, PREC);                 /* the rfun of tensor_eval */
            arb_neg(acb_realref(phi->term[0].A), acb_realref(phi->term[0].A));      /* Re(A) > 0 again */
            acb_indeterminate(f->f + 3);                                             /* a nonfinite entry */
            if (k == 10) (void) adf_tensor_eval(z, phi, f, a, PREC);
            if (k == 11) (void) adf_tensor_eval_sball(z, phi, f, s, PREC);
            if (k == 12) (void) adf_tensor_integral(z, phi, f, PREC);
            if (k == 13) (void) adf_tensor_norm2(n, phi, f, PREC);
            /* Reached only if the entry check is missing. The clears are for tools/memcheck. */
            adf_rfun_clear(phi); adf_ffun_clear(f); adf_fball_clear(x); adf_adele_clear(a); adf_sball_clear(s);
            acb_clear(z); arb_clear(n);
            _exit(0);
        }
        CHECK(waitpid(p, &status, 0) == p && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
    /* the prec cap is decided before the entry predicates */
    {   adf_rfun_t phi; adf_ffun_t f; adf_fball_t x; adf_adele_t a; adf_sball_t s; acb_t z; arb_t n;
        adf_rfun_init(phi); adf_ffun_init(f); adf_fball_init(x); adf_adele_init(a); adf_sball_init(s);
        acb_init(z); arb_init(n); gauss(phi); fun_family(f, 2, 3, FAM_INT);
        arb_neg(acb_realref(phi->term[0].A), acb_realref(phi->term[0].A));
        acb_indeterminate(f->f + 1); fmpz_zero(x->d); s->arch = 5;
        all_seven(ADF_LIMIT, phi, f, x, a, s, ADF_REAL_PREC_MAX + 1, 2);
        arb_neg(acb_realref(phi->term[0].A), acb_realref(phi->term[0].A)); acb_zero(f->f + 1); fmpz_one(x->d);
        s->arch = 0;
        adf_rfun_clear(phi); adf_ffun_clear(f); adf_fball_clear(x); adf_adele_clear(a); adf_sball_clear(s);
        acb_clear(z); arb_clear(n); }
#endif
}

int main(void)
{
    exact_cases();
    eval_vectors();
    local_vectors();
    sball_cases();
    sball_vectors();
    none_bounds();
    tensor_vectors();
    integral_vectors();
    statuses();
    caps();
    guard_cases();
    debug();
    CHECK(exclusions > 1000);
    printf("tensor: %lu checks, %lu exclusions\n", checks, exclusions);
    flint_cleanup();
    return 0;
}
