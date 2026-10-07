/* tests/test_rfun.c: slice 4d of docs/api-4.md, the real test functions adf_rfun (include/adelefeld/rfun.h).
   Contract: docs/api-4.md sections 1, 2, 5, 6; docs/api-4b.md; conventions 5.12, 9.2-9.5, 11.3; analysis
   Proposition 5 (docs/proofs/analysis.md:174-212).
   Oracles: tests/golden/rfun.tsv (every row); tests/ref/vectors/f4-slice2/ written by
   lanes/f4-slice2/gen_vectors.py from proto/functions4_checks.py (exact closure results of rterm_translate,
   rterm_dilate, rterm_product; values of rvalue_ball at 400 bits) and proto/text_grammar.py (texts).
   A closure result is compared parameter by parameter and coefficient by coefficient with the exact
   result of every listed member of the input balls; numbers with a pi part are formed at 1024 bits.
   Strings are freed with adf_str_free. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/golden.h"
#include "support/jsonl.h"
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define PREC 128
#define EXP 1024

static unsigned long checks;
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
static int boolean(const jsonl_value *v)
{
    jsonl_error_t e; int b = 0; CHECK(jsonl_bool(v, &b, &e)); return b;
}
static void q_of(fmpq_t q, const char *s)
{
    CHECK(fmpq_set_str(q, s, 10) == 0); fmpq_canonicalise(q);
}

/* ------------------------------------------------------------------ building values */
/* An input number [re, im] or [re, im, e]: the rationals at PREC, radius 2^e added to both parts. */
static void in_num(acb_t z, const jsonl_value *v, slong prec)
{
    fmpq_t q; fmpq_init(q);
    q_of(q, str(at(v, 0))); arb_set_fmpq(acb_realref(z), q, prec);
    q_of(q, str(at(v, 1))); arb_set_fmpq(acb_imagref(z), q, prec);
    if (jsonl_size(v) == 3) {
        long e = number(at(v, 2));
        arb_add_error_2exp_si(acb_realref(z), e); arb_add_error_2exp_si(acb_imagref(z), e);
    }
    fmpq_clear(q);
}
/* An exact number [re0, re1, im0, im1] = (re0 + re1 pi) + i (im0 + im1 pi), at EXP bits. */
static void ex_num(acb_t z, const jsonl_value *v)
{
    arb_t pi, t; fmpq_t q; int k;
    arb_init(pi); arb_init(t); fmpq_init(q);
    arb_const_pi(pi, EXP);
    for (k = 0; k < 2; k++) {
        arb_ptr part = k ? acb_imagref(z) : acb_realref(z);
        q_of(q, str(at(v, 2 * k))); arb_set_fmpq(part, q, EXP);
        q_of(q, str(at(v, 2 * k + 1))); arb_set_fmpq(t, q, EXP); arb_mul(t, t, pi, EXP);
        arb_add(part, part, t, EXP);
    }
    arb_clear(pi); arb_clear(t); fmpq_clear(q);
}
/* 1 if the exact number has no pi part and both parts are dyadic of fewer than 100 bits in all. */
static int ex_small_dyadic(const jsonl_value *v)
{
    fmpq_t q; int ok = 1, k; fmpq_init(q);
    for (k = 0; k < 4; k++) {
        q_of(q, str(at(v, k)));
        if (k % 2) ok &= fmpq_is_zero(q);
        else ok &= (slong) fmpz_val2(fmpq_denref(q)) + 1 == (slong) fmpz_bits(fmpq_denref(q)),
             ok &= fmpz_bits(fmpq_numref(q)) + fmpz_bits(fmpq_denref(q)) < 100;
    }
    fmpq_clear(q); return ok;
}
static void term_from_json(adf_rterm_struct *t, const jsonl_value *v, slong prec)
{
    const jsonl_value *P = field(v, "P"); size_t j; acb_t c; acb_init(c);
    acb_poly_zero(t->P);
    for (j = 0; j < jsonl_size(P); j++) { in_num(c, at(P, j), prec); acb_poly_set_coeff_acb(t->P, (slong) j, c); }
    in_num(t->A, field(v, "A"), prec); in_num(t->B, field(v, "B"), prec); in_num(t->C, field(v, "C"), prec);
    acb_clear(c);
}
__attribute__((noinline)) static adf_rterm_struct *terms_new(slong n)
{
    adf_rterm_struct *t = (adf_rterm_struct *) flint_malloc((size_t) (n > 0 ? n : 1) * sizeof(*t)); slong i;
    for (i = 0; i < n; i++) { acb_poly_init(t[i].P); acb_init(t[i].A); acb_init(t[i].B); acb_init(t[i].C); }
    return t;
}
/* not inlined: gcc 13 reports a false -Wstringop-overflow on acb_clear of a flint_malloc array */
__attribute__((noinline)) static void terms_free(adf_rterm_struct *t, slong n)
{
    slong i;
    for (i = 0; i < n; i++) { acb_poly_clear(t[i].P); acb_clear(t[i].A); acb_clear(t[i].B); acb_clear(t[i].C); }
    flint_free(t);
}
static void fun_from_json(adf_rfun_t f, const jsonl_value *v, slong prec)
{
    slong n = (slong) jsonl_size(v), i; adf_rterm_struct *t = terms_new(n);
    for (i = 0; i < n; i++) term_from_json(t + i, at(v, (size_t) i), prec);
    CHECK(adf_rfun_set_terms(f, t, n) == ADF_OK);
    CHECK(adf_rfun_is_canonical(f));
    terms_free(t, n);
}
/* A one-term function P exp(-pi A x^2 + B x + C) from small numbers: P given by n real coefficients. */
static void fun_simple(adf_rfun_t f, const double *P, slong n, double Are, double Aim, double B, double C)
{
    adf_rterm_struct *t = terms_new(1); slong j;
    for (j = 0; j < n; j++) {
        acb_t c; acb_init(c); acb_set_d(c, P[j]); acb_poly_set_coeff_acb(t->P, j, c); acb_clear(c);
    }
    acb_set_d_d(t->A, Are, Aim); acb_set_d(t->B, B); acb_set_d(t->C, C);
    CHECK(adf_rfun_set_terms(f, t, 1) == ADF_OK);
    terms_free(t, 1);
}
static void rat_of(adf_rat_t r, const char *s) { q_of(r->q, s); }

/* ------------------------------------------------------------------ checking closure results */
/* Every coefficient and parameter of the result term R contains the exact member term E; when exact
   is set, the inputs were exact dyadic and nothing in the formula rounds (no pi part, a small dyadic):
   then R must be exact. When tight is set, every radius is below 2^-80 (1 + |value|). */
static void check_ex(const acb_t r, const jsonl_value *v, int exact, int tight)
{
    acb_t z; mag_t m, b; acb_init(z); mag_init(m); mag_init(b);
    ex_num(z, v);
    if (!acb_contains(r, z)) {
        fprintf(stderr, "result: "); acb_printd(r, 30); fprintf(stderr, "\nexact:  "); acb_printd(z, 30);
        fprintf(stderr, "\n");
    }
    CHECK(acb_contains(r, z));
    if (exact && ex_small_dyadic(v) && !(acb_is_exact(r) && acb_equal(r, z))) {
        fprintf(stderr, "not exact: "); acb_printd(r, 30); fprintf(stderr, "\n");
    }
    if (exact && ex_small_dyadic(v)) CHECK(acb_is_exact(r) && acb_equal(r, z));
    if (tight) {
        acb_get_mag(b, z); mag_add_ui(b, b, 1); mag_mul_2exp_si(b, b, -80);
        mag_hypot(m, arb_radref(acb_realref(r)), arb_radref(acb_imagref(r)));
        CHECK(mag_cmp(m, b) <= 0);
    }
    acb_clear(z); mag_clear(m); mag_clear(b);
}
static void check_term(const adf_rterm_struct *R, const jsonl_value *E, int exact, int tight)
{
    const jsonl_value *P = field(E, "P"); slong j, n = (slong) jsonl_size(P);
    static const char zero_json[] = "{\"z\":[\"0\",\"0\",\"0\",\"0\"]}";
    jsonl_file *zf; jsonl_error_t e;
    CHECK(jsonl_parse(zero_json, sizeof(zero_json) - 1, "zero", &zf, &e));
    CHECK(acb_poly_length(R->P) >= n);
    for (j = 0; j < acb_poly_length(R->P); j++)
        check_ex(R->P->coeffs + j, j < n ? at(P, (size_t) j) : field(jsonl_record(zf, 0), "z"), exact, tight);
    check_ex(R->A, field(E, "A"), exact, tight);
    check_ex(R->B, field(E, "B"), exact, tight);
    check_ex(R->C, field(E, "C"), exact, tight);
    CHECK(arb_is_positive(acb_realref(R->A)));
    jsonl_close(zf);
}

/* ------------------------------------------------------------------ the golden file (11.3) */
static void expect_print(const adf_rfun_t x, const char *want, slong digits)
{
    size_t n; char *s = adf_rfun_get_str(&n, x, digits);
    CHECK(s != NULL);
    if (strcmp(s, want)) fprintf(stderr, "want: %s\ngot:  %s\n", want, s);
    CHECK(n == strlen(want) && !memcmp(s, want, n)); adf_str_free(s);
}
/* a sentinel value whose bytes must survive a failing call */
static void sentinel(adf_rfun_t x)
{
    static const double P[2] = { 3, -1 };
    fun_simple(x, P, 2, 2, 0.5, 0.25, -1);
}
static int read_r(adf_rfun_t x, const char *s, size_t len, slong prec, const adf_text_limits_t *lim)
{
    adf_rfun_t keep; adf_rfun_struct before = *x; int st;
    adf_rfun_init(keep); adf_rfun_set(keep, x);
    st = adf_rfun_set_str(x, s, len, prec, lim);
    if (st != ADF_OK) { CHECK(!memcmp(&before, x, sizeof(before))); CHECK(adf_rfun_identical(x, keep)); }
    else CHECK(adf_rfun_is_canonical(x));
    adf_rfun_clear(keep); return st;
}
/* conventions 11.3 item 3: a value row prints equal where every real ball of the input is exact at prec
   128 (all goldens but the radius 0.999999); otherwise the text, read back, must contain the value read. */
static void golden(void)
{
    golden_file *g; golden_error_t ge; size_t i; int rows = 0;
    adf_rfun_t x, y; adf_rfun_init(x); adf_rfun_init(y);
    CHECK(golden_open("tests/golden/rfun.tsv", GOLDEN_DEFAULT_MAX_INPUT, &g, &ge));
    CHECK(golden_count(g) == 19);
    for (i = 0; i < golden_count(g); i++) {
        const golden_record *row = golden_record_at(g, i); int st;
        sentinel(x);
        st = read_r(x, row->input, row->input_len, PREC, NULL);
        rows++;
        if (row->is_status) { CHECK(!strcmp(adf_status_str(st), row->status)); continue; }
        CHECK(st == ADF_OK);
        if (strstr(row->input, "0.999999") == NULL) { expect_print(x, row->expected, 20); continue; }
        {   size_t n; char *s = adf_rfun_get_str(&n, x, 20); slong k;
            CHECK(s != NULL && read_r(y, s, n, PREC, NULL) == ADF_OK && y->len == x->len);
            for (k = 0; k < x->len; k++) {
                CHECK(acb_contains(y->term[k].A, x->term[k].A));
                CHECK(acb_poly_length(y->term[k].P) == acb_poly_length(x->term[k].P));
            }
            /* the exact interval [0.000001, 1.999999] of the input lies in the ball read */
            { arb_t lo; arb_init(lo); arb_set_str(lo, "0.000001", 200);
              CHECK(arb_contains(acb_realref(x->term[0].A), lo)); arb_set_str(lo, "1.999999", 200);
              CHECK(arb_contains(acb_realref(x->term[0].A), lo)); arb_clear(lo); }
            adf_str_free(s); }
    }
    CHECK(rows == 19);
    golden_close(g); adf_rfun_clear(x); adf_rfun_clear(y);
}

/* the texts of the reference parser: random terms over dyadic decimals (exact at prec 128) */
static void texts(void)
{
    jsonl_file *f; jsonl_error_t e; size_t i; adf_rfun_t x, y; adf_text_limits_t lim;
    adf_rfun_init(x); adf_rfun_init(y);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice2/texts.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 164);
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i); const char *in = str(field(v, "input"));
        const char *want = str(field(v, "expected")); int st;
        adf_text_limits_default(&lim);
        if (has_field(v, "max_items")) lim.max_items = number(field(v, "max_items"));
        sentinel(x);
        st = read_r(x, in, strlen(in), PREC, &lim);
        if (want[0] == '!') { CHECK(!strcmp(adf_status_str(st), want + 1)); continue; }
        CHECK(st == ADF_OK);
        expect_print(x, want, 20);
        /* round trip (conventions 9.6, 11.3 item 3): the printed text reads back to a value containing x
           (a printed radius such as 0.063 is not dyadic, so the reread ball prints with 0.064) */
        CHECK(read_r(y, want, strlen(want), PREC, &lim) == ADF_OK && y->len == x->len);
        { slong k, j;
          for (k = 0; k < x->len; k++) {
              CHECK(acb_contains(y->term[k].A, x->term[k].A) && acb_contains(y->term[k].C, x->term[k].C));
              CHECK(acb_poly_length(y->term[k].P) == acb_poly_length(x->term[k].P));
              for (j = 0; j < acb_poly_length(x->term[k].P); j++)
                  CHECK(acb_contains(y->term[k].P->coeffs + j, x->term[k].P->coeffs + j));
          } }
    }
    jsonl_close(f); adf_rfun_clear(x); adf_rfun_clear(y);
}

/* hand cases of the reader: whitespace, exact trailing zeros only, order of stages, NOT_DETERMINED */
static void reader_cases(void)
{
    adf_rfun_t x; adf_text_limits_t lim; const char *s;
    adf_rfun_init(x);
    s = "rfun()";
    sentinel(x); CHECK(read_r(x, s, strlen(s), PREC, NULL) == ADF_OK && x->len == 0 && x->term == NULL);
    /* stage 7: Re(A) = 10^-1000 is positive as a decimal, and its enclosure at prec 2 is (0 +/- ...)?
       arb keeps the exponent, so the enclosure stays positive: OK at every prec */
    s = "rfun(term(P=[], A=(1e-1000) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))";
    CHECK(read_r(x, s, strlen(s), 2, NULL) == ADF_OK);
    /* a positive interval [1 - (1 - 2^-200), ...] = [2^-200, ...] whose enclosure at prec 2 contains 0 */
    s = "rfun(term(P=[], A=(1 +/- 0.99999999999999999999999999999999999999999999999999999999999) + (0)*i, "
        "B=(0) + (0)*i, C=(0) + (0)*i))";
    sentinel(x); CHECK(read_r(x, s, strlen(s), 2, NULL) == ADF_NOT_DETERMINED);
    CHECK(read_r(x, s, strlen(s), 256, NULL) == ADF_OK);
    /* stage 4 before stage 6: max_items exceeded and an invalid A */
    adf_text_limits_default(&lim); lim.max_items = 1;
    s = "rfun(term(P=[(1) + (0)*i, (2) + (0)*i], A=(-1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))";
    sentinel(x); CHECK(read_r(x, s, strlen(s), PREC, &lim) == ADF_LIMIT);
    CHECK(read_r(x, s, strlen(s), PREC, NULL) == ADF_DOMAIN);
    s = "rfun(term(P=[(1e100001) + (0)*i], A=(-1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))";
    CHECK(read_r(x, s, strlen(s), PREC, NULL) == ADF_LIMIT);
    /* syntax before everything */
    s = "rfun(term(P=[(1e100001) + (0)*i], A=(-1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)";
    CHECK(read_r(x, s, strlen(s), PREC, NULL) == ADF_PARSE);
    s = "rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))\x01";
    CHECK(read_r(x, s, strlen(s), PREC, NULL) == ADF_PARSE);
    /* an inexact zero last coefficient is kept, an exact one removed, inner exact zeros kept */
    s = "rfun(term(P=[(0) + (0)*i, (0 +/- 1e-3) + (0)*i, (0) + (0 +/- 0.5)*i, (0) + (0)*i], A=(1) + (0)*i, "
        "B=(0) + (0)*i, C=(0) + (0)*i))";
    CHECK(read_r(x, s, strlen(s), PREC, NULL) == ADF_OK && acb_poly_length(x->term[0].P) == 3);
    CHECK(acb_is_zero(x->term[0].P->coeffs));
    /* two terms are kept in order, not combined */
    s = "rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i), "
        "term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))";
    CHECK(read_r(x, s, strlen(s), PREC, NULL) == ADF_OK && x->len == 2);
    /* the zero function prints rfun(), zero terms are printed */
    expect_print(x, "rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i), "
                    "term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))", 20);
    adf_rfun_clear(x);
}

/* ------------------------------------------------------------------ lifecycle, raw setter, caps */
static void lifecycle(void)
{
    adf_rfun_t x, y; adf_rterm_struct *t; adf_rfun_struct before;
    static const double P[3] = { 1, 0, -2 };
    CHECK(adf_sizeof_rfun() == sizeof(adf_rfun_struct) && adf_alignof_rfun() == _Alignof(adf_rfun_struct));
    CHECK(adf_sizeof_rterm() == sizeof(adf_rterm_struct) && adf_alignof_rterm() == _Alignof(adf_rterm_struct));
    adf_rfun_init(x); adf_rfun_init(y);
    CHECK(x->len == 0 && x->term == NULL && adf_rfun_is_canonical(x) && adf_rfun_identical(x, y));
    fun_simple(x, P, 3, 1, 0.5, 2, -1);
    CHECK(x->len == 1 && adf_rfun_is_canonical(x) && !adf_rfun_identical(x, y));
    adf_rfun_set(y, x); CHECK(adf_rfun_identical(x, y) && y->term != x->term);
    CHECK(y->term[0].P->coeffs != x->term[0].P->coeffs);
    adf_rfun_set(y, y); CHECK(adf_rfun_identical(x, y));
    adf_rfun_clear(y); adf_rfun_init(y);
    adf_rfun_swap(x, y); CHECK(x->len == 0 && y->len == 1);
    adf_rfun_swap(y, y); CHECK(y->len == 1);
    adf_rfun_set(x, y); adf_rfun_set(y, x); CHECK(adf_rfun_identical(x, y));
    { adf_rfun_t z; adf_rfun_init(z); adf_rfun_set(x, z); CHECK(x->len == 0 && x->term == NULL);
      adf_rfun_clear(z); }

    /* raw setter: DOMAIN cases leave y untouched */
    t = terms_new(2);
    acb_poly_set_coeff_si(t[0].P, 0, 1); acb_one(t[0].A);
    acb_poly_set_coeff_si(t[1].P, 1, 1); acb_one(t[1].A);
    sentinel(y); before = *y;
    CHECK(adf_rfun_set_terms(y, t, -1) == ADF_DOMAIN && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_set_terms(y, NULL, 1) == ADF_DOMAIN && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_set_terms(y, NULL, 0) == ADF_OK && y->len == 0);
    sentinel(y); before = *y;
    acb_zero(t[1].A);                                         /* Re(A) = 0 */
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_DOMAIN && !memcmp(&before, y, sizeof(before)));
    acb_set_si(t[1].A, -1);
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_DOMAIN);
    acb_one(t[1].A); mag_one(arb_radref(acb_realref(t[1].A)));  /* 1 +/- 1 contains 0 */
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_DOMAIN);
    acb_one(t[1].A); acb_set_si(t[1].A, 1); arb_set_si(acb_imagref(t[1].A), -7);   /* complex, Re > 0 */
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_OK && y->len == 2 && adf_rfun_is_canonical(y));
    sentinel(y); before = *y;
    arb_indeterminate(acb_realref(t[1].B));
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_DOMAIN && !memcmp(&before, y, sizeof(before)));
    acb_zero(t[1].B); arb_pos_inf(acb_imagref(t[1].C));
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_DOMAIN);
    acb_zero(t[1].C); arb_indeterminate(acb_imagref(t[1].P->coeffs));
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_DOMAIN);
    acb_zero(t[1].P->coeffs);
    /* an exact zero last coefficient: not normalized */
    acb_poly_fit_length(t[1].P, 3); acb_zero(t[1].P->coeffs + 2); _acb_poly_set_length(t[1].P, 3);
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_DOMAIN && !memcmp(&before, y, sizeof(before)));
    /* an inexact zero last coefficient is normalized */
    mag_one(arb_radref(acb_realref(t[1].P->coeffs + 2)));
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_OK && acb_poly_length(y->term[1].P) == 3);
    CHECK(adf_rfun_is_canonical(y));
    /* the first term invalid (Re(A) < 0); a length-1 polynomial whose only coefficient is the exact 0 */
    sentinel(y); before = *y;
    acb_set_si(t[0].A, -1);
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_DOMAIN && !memcmp(&before, y, sizeof(before)));
    acb_one(t[0].A); acb_poly_fit_length(t[0].P, 1); acb_zero(t[0].P->coeffs); _acb_poly_set_length(t[0].P, 1);
    CHECK(adf_rfun_set_terms(y, t, 1) == ADF_DOMAIN && !memcmp(&before, y, sizeof(before)));
    _acb_poly_set_length(t[0].P, 0);
    CHECK(adf_rfun_set_terms(y, t, 1) == ADF_OK && acb_poly_length(y->term[0].P) == 0);
    /* the predicate on raw fields */
    { adf_rfun_struct raw = *y; raw.len = -1; CHECK(!adf_rfun_is_canonical(&raw));
      raw.len = 1; raw.term = NULL; CHECK(!adf_rfun_is_canonical(&raw)); }
    arb_neg(acb_realref(y->term[0].A), acb_realref(y->term[0].A)); CHECK(!adf_rfun_is_canonical(y));
    arb_neg(acb_realref(y->term[0].A), acb_realref(y->term[0].A)); CHECK(adf_rfun_is_canonical(y));
    terms_free(t, 2);
    adf_rfun_clear(x); adf_rfun_clear(y);
}

static void caps(void)
{
    adf_rfun_t x, y, z; adf_rterm_struct *t; slong i, n = ADF_RFUN_TERMS_MAX; adf_rfun_struct before;
    adf_rat_t q;
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(z); adf_rat_init(q);
    /* 2^16 terms accepted; 2^16 + 1 refused before any term is read (a one-element array is passed) */
    t = terms_new(n + 1);
    for (i = 0; i < n + 1; i++) acb_one(t[i].A);
    CHECK(adf_rfun_set_terms(x, t, n) == ADF_OK && x->len == n && adf_rfun_is_canonical(x));
    sentinel(y); before = *y;
    CHECK(adf_rfun_set_terms(y, t, n + 1) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    sentinel(y); before = *y;
    { adf_rterm_struct *one = terms_new(1); acb_one(one->A);
      CHECK(adf_rfun_set_terms(y, one, n + 1) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
      terms_free(one, 1); }
    /* the sum of 2^16 and one term: LIMIT, z untouched */
    sentinel(z); before = *z;
    CHECK(adf_rfun_add(z, x, y, PREC) == ADF_LIMIT && !memcmp(&before, z, sizeof(before)));
    CHECK(adf_rfun_reflect(z, x) == ADF_OK && z->len == n);
    terms_free(t, n + 1);
    /* coefficients: 2^16 in all accepted, 2^16 + 1 refused */
    t = terms_new(2);
    acb_one(t[0].A); acb_one(t[1].A);
    acb_poly_set_coeff_si(t[0].P, ADF_RFUN_COEFFS_MAX / 2 - 1, 1);
    acb_poly_set_coeff_si(t[1].P, ADF_RFUN_COEFFS_MAX / 2 - 1, 1);
    CHECK(adf_rfun_set_terms(x, t, 2) == ADF_OK);
    acb_poly_set_coeff_si(t[1].P, ADF_RFUN_COEFFS_MAX / 2, 1);
    sentinel(y); before = *y;
    CHECK(adf_rfun_set_terms(y, t, 2) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    /* work: the shift of a polynomial of length n costs n (n - 1)/2 multiply-adds, a product of lengths
       n and m costs n m: length 1500 passes 2^20 for both, length 1400 not for the shift */
    acb_poly_zero(t[0].P); acb_poly_set_coeff_si(t[0].P, 1499, 1);
    CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK);
    rat_of(q, "1/3"); sentinel(y); before = *y;
    CHECK(adf_rfun_translate_rat(y, x, q, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_mul(y, x, x, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_dilate_rat(y, x, q, PREC) == ADF_OK);      /* dilation costs total coefficients */
    acb_poly_zero(t[0].P); acb_poly_set_coeff_si(t[0].P, 1399, 1);
    CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK);
    CHECK(adf_rfun_translate_rat(y, x, q, 64) == ADF_OK && acb_poly_length(y->term[0].P) == 1400);
    /* the number of product terms: 300 * 300 > 2^16 */
    terms_free(t, 2); t = terms_new(300);
    for (i = 0; i < 300; i++) acb_one(t[i].A);
    CHECK(adf_rfun_set_terms(x, t, 300) == ADF_OK);
    sentinel(y); before = *y;
    CHECK(adf_rfun_mul(y, x, x, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_set_terms(x, t, 200) == ADF_OK && adf_rfun_mul(y, x, x, PREC) == ADF_OK && y->len == 40000);
    terms_free(t, 300);
    /* boundaries (mutation survivors): product terms 256 * 256 = 2^16 accepted, 256 * 257 refused */
    t = terms_new(257);
    for (i = 0; i < 257; i++) acb_one(t[i].A);
    CHECK(adf_rfun_set_terms(x, t, 256) == ADF_OK && adf_rfun_set_terms(z, t, 257) == ADF_OK);
    CHECK(adf_rfun_mul(y, x, x, PREC) == ADF_OK && y->len == 65536);
    sentinel(y); before = *y;
    CHECK(adf_rfun_mul(y, x, z, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    /* product coefficients: 256 constant terms times one term of length 256 gives 2^16, of length 257 more */
    for (i = 0; i < 256; i++) acb_poly_set_coeff_si(t[i].P, 0, 1);
    CHECK(adf_rfun_set_terms(x, t, 256) == ADF_OK);
    acb_poly_zero(t[256].P); acb_poly_set_coeff_si(t[256].P, 255, 1);
    CHECK(adf_rfun_set_terms(z, t + 256, 1) == ADF_OK);
    CHECK(adf_rfun_mul(y, x, z, PREC) == ADF_OK && y->len == 256);
    acb_poly_set_coeff_si(t[256].P, 256, 1);
    CHECK(adf_rfun_set_terms(z, t + 256, 1) == ADF_OK);
    sentinel(y); before = *y;
    CHECK(adf_rfun_mul(y, x, z, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_mul(y, z, x, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    terms_free(t, 257);
    /* product work: lengths 1024 and 1024 cost exactly 2^20, accepted; 1024 and 1025 refused */
    t = terms_new(2);
    acb_one(t[0].A); acb_one(t[1].A);
    for (i = 0; i < 1024; i++) acb_poly_set_coeff_si(t[0].P, i, 1);
    for (i = 0; i < 1025; i++) acb_poly_set_coeff_si(t[1].P, i, 1);
    CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK && adf_rfun_set_terms(z, t + 1, 1) == ADF_OK);
    CHECK(adf_rfun_mul(y, x, x, 64) == ADF_OK && acb_poly_length(y->term[0].P) == 2047);
    sentinel(y); before = *y;
    CHECK(adf_rfun_mul(y, x, z, 64) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    terms_free(t, 2);
    /* translation work: lengths 1448, 44, 2, 2 and a zero polynomial cost 1047628 + 946 + 1 + 1 + 0 = 2^20 */
    { static const slong lens[6] = { 1448, 44, 2, 2, 0, 2 };
      t = terms_new(6);
      for (i = 0; i < 6; i++) { acb_one(t[i].A); if (lens[i]) acb_poly_set_coeff_si(t[i].P, lens[i] - 1, 1); }
      CHECK(adf_rfun_set_terms(x, t, 5) == ADF_OK);
      rat_of(q, "1/2");
      CHECK(adf_rfun_translate_rat(y, x, q, 64) == ADF_OK && y->len == 5);
      CHECK(adf_rfun_set_terms(x, t, 6) == ADF_OK);
      sentinel(y); before = *y;
      CHECK(adf_rfun_translate_rat(y, x, q, 64) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
      terms_free(t, 6); }
    /* inputs above the caps (read by the text reader, which applies only the caller limits of 8.4):
       every operation refuses them with LIMIT, also against an empty second operand */
    { adf_text_limits_t lim; adf_rfun_t e; adf_idele_t a; arb_t xr; acb_t v; char *s, *w; size_t k, len = 0;
      static const char T[] = "term(P=[], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)";
      static const char Cf[] = "(1) + (0)*i";
      adf_rfun_init(e); adf_idele_init(a); arb_init(xr); acb_init(v);
      adf_text_limits_default(&lim); lim.max_len = (size_t) 1 << 23; lim.max_items = (slong) 1 << 20;
      s = (char *) flint_malloc((size_t) (n + 1) * (sizeof(T) + 2) + 16);
      for (k = 0; k < 2; k++) {
          w = s;
          if (k == 0) {
              memcpy(w, "rfun(", 5); w += 5;
              for (i = 0; i < n + 1; i++) {
                  if (i) { memcpy(w, ", ", 2); w += 2; }
                  memcpy(w, T, sizeof(T) - 1); w += sizeof(T) - 1;
              }
          } else {   /* one term with 2^16 + 1 coefficients */
              memcpy(w, "rfun(term(P=[", 13); w += 13;
              for (i = 0; i < n + 1; i++) {
                  if (i) { memcpy(w, ", ", 2); w += 2; }
                  memcpy(w, Cf, sizeof(Cf) - 1); w += sizeof(Cf) - 1;
              }
              memcpy(w, "], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)", 47); w += 47;
          }
          *w++ = ')'; len = (size_t) (w - s);
          CHECK(adf_rfun_set_str(x, s, len, PREC, &lim) == ADF_OK && adf_rfun_is_canonical(x));
          sentinel(y); before = *y; acb_set_si(v, 7); arb_zero(xr);
          CHECK(adf_rfun_add(y, x, e, PREC) == ADF_LIMIT && adf_rfun_add(y, e, x, PREC) == ADF_LIMIT);
          CHECK(adf_rfun_mul(y, x, e, PREC) == ADF_LIMIT && adf_rfun_mul(y, e, x, PREC) == ADF_LIMIT);
          rat_of(q, "2");
          CHECK(adf_rfun_translate_rat(y, x, q, PREC) == ADF_LIMIT);
          CHECK(adf_rfun_dilate_rat(y, x, q, PREC) == ADF_LIMIT);
          CHECK(adf_rfun_dilate_idele(y, x, a, PREC) == ADF_LIMIT);
          CHECK(adf_rfun_reflect(y, x) == ADF_LIMIT && adf_rfun_conj(y, x) == ADF_LIMIT);
          CHECK(!memcmp(&before, y, sizeof(before)));
          CHECK(adf_rfun_eval(v, x, xr, PREC) == ADF_LIMIT && acb_equal_si(v, 7));
      }
      flint_free(s); adf_rfun_clear(e); adf_idele_clear(a); arb_clear(xr); acb_clear(v); }
    /* bits: a rational of 2^19 + 1 bits (its square passes 2^20 bits) */
    { static const double P[1] = { 1 }; fun_simple(x, P, 1, 1, 0, 0, 0); }
    fmpz_one(fmpq_numref(q->q)); fmpz_mul_2exp(fmpq_numref(q->q), fmpq_numref(q->q), 524288);
    fmpz_one(fmpq_denref(q->q)); fmpz_add_ui(fmpq_denref(q->q), fmpq_denref(q->q), 2);
    sentinel(y); before = *y;
    CHECK(adf_rfun_translate_rat(y, x, q, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_dilate_rat(y, x, q, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    fmpz_one(fmpq_numref(q->q)); fmpz_mul_2exp(fmpq_numref(q->q), fmpq_numref(q->q), 500000);
    CHECK(adf_rfun_translate_rat(y, x, q, PREC) == ADF_OK);
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(z); adf_rat_clear(q);
}

/* ------------------------------------------------------------------ closure vectors */
/* 1 if every stored ball of f is exact (the vector inputs that are dyadic rationals) */
static int fun_exact(const adf_rfun_t f)
{
    slong k, j; int ok = 1;
    for (k = 0; k < f->len; k++) {
        ok &= acb_is_exact(f->term[k].A) && acb_is_exact(f->term[k].B) && acb_is_exact(f->term[k].C);
        for (j = 0; j < acb_poly_length(f->term[k].P); j++) ok &= acb_is_exact(f->term[k].P->coeffs + j);
    }
    return ok;
}
static void closure(void)
{
    jsonl_file *f; jsonl_error_t e; size_t i, k; adf_rfun_t x, y, z, w; adf_rat_t q;
    unsigned long seen[6] = { 0, 0, 0, 0, 0, 0 };
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(z); adf_rfun_init(w); adf_rat_init(q);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice2/closure.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 135);
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i), *mem = field(v, "members");
        const char *op = str(field(v, "op"));
        int exact = boolean(field(v, "exact_in")), tight = 1, st;
        fun_from_json(x, field(v, "x"), PREC);
        if (has_field(v, "y")) fun_from_json(y, field(v, "y"), PREC);
        exact &= fun_exact(x) && (!has_field(v, "y") || fun_exact(y));
        tight &= boolean(field(v, "exact_in"));       /* a ball input gives a ball output */
        if (has_field(v, "q")) {
            rat_of(q, str(field(v, "q"))); tight = strlen(str(field(v, "q"))) < 20;
            /* rounding enters through the division by a denominator that is not a power of 2, and through
               pi; with a dyadic q and exact dyadic inputs every other step is exact at PREC */
            exact &= tight && (slong) fmpz_val2(fmpq_denref(q->q)) + 1 == (slong) fmpz_bits(fmpq_denref(q->q));
            tight &= boolean(field(v, "exact_in"));
        }
        if (!strcmp(op, "translate")) { st = adf_rfun_translate_rat(z, x, q, PREC); seen[0]++; }
        else if (!strcmp(op, "dilate")) { st = adf_rfun_dilate_rat(z, x, q, PREC); seen[1]++; }
        else if (!strcmp(op, "reflect")) { st = adf_rfun_reflect(z, x); seen[2]++; }
        else if (!strcmp(op, "conj")) { st = adf_rfun_conj(z, x); seen[3]++; }
        else if (!strcmp(op, "add")) { st = adf_rfun_add(z, x, y, PREC); seen[4]++; }
        else { CHECK(!strcmp(op, "mul")); st = adf_rfun_mul(z, x, y, PREC); seen[5]++; }
        CHECK(st == ADF_OK && adf_rfun_is_canonical(z));
        CHECK(z->len == (slong) jsonl_size(mem));
        for (k = 0; k < jsonl_size(mem); k++) {
            const jsonl_value *ms = at(mem, k); size_t m;
            for (m = 0; m < jsonl_size(ms); m++)
                check_term(z->term + k, at(ms, m), exact && m == 0 && jsonl_size(ms) == 1, tight && m == 0);
        }
        /* reflection and conjugation are exact and involutions */
        if (!strcmp(op, "reflect")) { CHECK(adf_rfun_reflect(w, z) == ADF_OK && adf_rfun_identical(w, x)); }
        if (!strcmp(op, "conj")) { CHECK(adf_rfun_conj(w, z) == ADF_OK && adf_rfun_identical(w, x)); }
        /* the sum keeps the stored terms exactly, x before y */
        if (!strcmp(op, "add")) {
            slong j;
            for (j = 0; j < x->len; j++) CHECK(acb_poly_equal(z->term[j].P, x->term[j].P)
                                               && acb_equal(z->term[j].A, x->term[j].A));
            for (j = 0; j < y->len; j++) CHECK(acb_poly_equal(z->term[x->len + j].P, y->term[j].P)
                                               && acb_equal(z->term[x->len + j].C, y->term[j].C));
        }
    }
    for (k = 0; k < 6; k++) CHECK(seen[k] > 0);
    jsonl_close(f);
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(z); adf_rfun_clear(w); adf_rat_clear(q);
}

/* ------------------------------------------------------------------ evaluation vectors */
static void ref_ball(acb_t z, const jsonl_value *v)
{
    CHECK(arb_set_str(acb_realref(z), str(at(v, 0)), 200) == 0);
    CHECK(arb_set_str(acb_imagref(z), str(at(v, 1)), 200) == 0);
}
static void values(void)
{
    jsonl_file *f; jsonl_error_t e; size_t i, j, k, m; adf_rfun_t phi; arb_t x; acb_t z, r; fmpq_t q;
    unsigned long npts = 0;
    adf_rfun_init(phi); arb_init(x); acb_init(z); acb_init(r); fmpq_init(q);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice2/values.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 13);
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i), *pts = field(v, "points");
        if (jsonl_is(field(v, "fun"), JSONL_STR)) {
            /* the shifted Gaussian, in exact pi form, set at EXP bits */
            const jsonl_value *T = field(v, "term"); adf_rterm_struct *t = terms_new(1);
            ex_num(r, at(field(T, "P"), 0)); acb_poly_set_coeff_acb(t->P, 0, r);
            ex_num(t->A, field(T, "A")); ex_num(t->B, field(T, "B")); ex_num(t->C, field(T, "C"));
            CHECK(adf_rfun_set_terms(phi, t, 1) == ADF_OK); terms_free(t, 1);
        } else fun_from_json(phi, field(v, "fun"), PREC);
        for (j = 0; j < jsonl_size(pts); j++) {
            const jsonl_value *p = at(pts, j);
            q_of(q, str(field(p, "x"))); arb_set_fmpq(x, q, PREC);
            if (has_field(p, "xrad")) arb_add_error_2exp_si(x, number(field(p, "xrad")));
            CHECK(adf_rfun_eval(z, phi, x, PREC) == ADF_OK);
            npts++;
            if (has_field(p, "samples")) {
                const jsonl_value *S = field(p, "samples");
                for (k = 0; k < jsonl_size(S); k++)
                    for (m = 0; m < jsonl_size(at(S, k)); m++) {
                        ref_ball(r, at(at(S, k), m)); CHECK(acb_contains(z, r));
                    }
            } else {
                const jsonl_value *M = field(p, "members");
                for (m = 0; m < jsonl_size(M); m++) {
                    ref_ball(r, at(M, m));
                    if (!acb_contains(z, r)) { acb_printd(z, 40); printf("\n"); acb_printd(r, 40); printf("\n"); }
                    CHECK(acb_contains(z, r));
                }
                /* at an exact dyadic point of an exact function, more precision gives a smaller ball */
                if (jsonl_size(M) == 1 && arb_is_exact(x)) {
                    acb_t z2; acb_init(z2);
                    CHECK(adf_rfun_eval(z2, phi, x, 4 * PREC) == ADF_OK && acb_contains(z, z2));
                    acb_clear(z2);
                }
            }
        }
    }
    CHECK(npts == 12 * 40 + 5);
    jsonl_close(f);
    adf_rfun_clear(phi); arb_clear(x); acb_clear(z); acb_clear(r); fmpq_clear(q);
}

/* ------------------------------------------------------------------ hand cases of the algebra */
/* PLAN 4.3: exp(-pi (x - 1/3)^2) has B = 2 pi/3, C = -pi/9; a wrong sign of B is caught by evaluation. */
static void shifted_gaussian(void)
{
    adf_rfun_t g, s, wrong; adf_rat_t q; arb_t x, t; acb_t z; static const double one[1] = { 1 };
    adf_rfun_init(g); adf_rfun_init(s); adf_rfun_init(wrong); adf_rat_init(q);
    arb_init(x); arb_init(t); acb_init(z);
    fun_simple(g, one, 1, 1, 0, 0, 0);
    rat_of(q, "1/3");
    CHECK(adf_rfun_translate_rat(s, g, q, PREC) == ADF_OK);
    arb_const_pi(t, EXP); arb_mul_si(t, t, 2, EXP); arb_div_si(t, t, 3, EXP);
    CHECK(arb_contains(acb_realref(s->term[0].B), t) && arb_is_zero(acb_imagref(s->term[0].B)));
    arb_const_pi(t, EXP); arb_div_si(t, t, -9, EXP);
    CHECK(arb_contains(acb_realref(s->term[0].C), t) && arb_is_zero(acb_imagref(s->term[0].C)));
    CHECK(acb_is_one(s->term[0].A) && acb_poly_length(s->term[0].P) == 1 && acb_is_one(s->term[0].P->coeffs));
    /* phi(1/3) = 1, phi(-1/3) = exp(-4 pi/9) */
    arb_set_si(x, 1); arb_div_si(x, x, 3, PREC);
    CHECK(adf_rfun_eval(z, s, x, PREC) == ADF_OK && arb_contains_si(acb_realref(z), 1));
    CHECK(arb_contains_si(acb_imagref(z), 0) && arb_rel_accuracy_bits(acb_realref(z)) > 100);
    adf_rfun_set(wrong, s); acb_neg(wrong->term[0].B, wrong->term[0].B);
    CHECK(adf_rfun_eval(z, wrong, x, PREC) == ADF_OK && !arb_contains_si(acb_realref(z), 1));
    arb_neg(x, x);
    CHECK(adf_rfun_eval(z, wrong, x, PREC) == ADF_OK && arb_contains_si(acb_realref(z), 1));
    CHECK(adf_rfun_eval(z, s, x, PREC) == ADF_OK && !arb_contains_si(acb_realref(z), 1));
    arb_const_pi(t, EXP); arb_mul_si(t, t, -4, EXP); arb_div_si(t, t, 9, EXP); arb_exp(t, t, EXP);
    CHECK(arb_overlaps(acb_realref(z), t));
    /* faults of 4.3: C without -pi A q^2 would give phi(1/3) = exp(pi/9) */
    CHECK(!arb_contains_si(acb_realref(s->term[0].C), 0));
    /* translation and dilation back: translate by -1/3 gives back B = 0 and C = 0 as balls */
    rat_of(q, "-1/3");
    CHECK(adf_rfun_translate_rat(wrong, s, q, PREC) == ADF_OK);
    CHECK(arb_contains_si(acb_realref(wrong->term[0].B), 0) && arb_contains_si(acb_realref(wrong->term[0].C), 0));
    /* dilation: A h^2, B h (faults: h instead of h^2; sign of h lost in B) */
    { adf_rfun_t d; adf_rfun_init(d); rat_of(q, "-2");
      fun_simple(g, one, 1, 1, 0, 2, 0);
      CHECK(adf_rfun_dilate_rat(d, g, q, PREC) == ADF_OK);
      CHECK(acb_equal_si(d->term[0].A, 4) && acb_equal_si(d->term[0].B, -4));
      /* product adds A, B, C (fault: only A) */
      CHECK(adf_rfun_mul(d, g, g, PREC) == ADF_OK && d->len == 1);
      CHECK(acb_equal_si(d->term[0].A, 2) && acb_equal_si(d->term[0].B, 4) && acb_is_zero(d->term[0].C));
      adf_rfun_clear(d); }
    adf_rfun_clear(g); adf_rfun_clear(s); adf_rfun_clear(wrong); adf_rat_clear(q);
    arb_clear(x); arb_clear(t); acb_clear(z);
}

static void statuses(void)
{
    adf_rfun_t x, y, zero; adf_rat_t q; adf_idele_t a; adf_rfun_struct before; arb_t t; acb_t z, keep;
    static const double P[2] = { 1, 2 };
    const char *s;
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(zero); adf_rat_init(q); adf_idele_init(a);
    arb_init(t); acb_init(z); acb_init(keep);
    fun_simple(x, P, 2, 1, 0, 0.5, 0);
    /* dilation by 0: DOMAIN, also for the zero function */
    rat_of(q, "0"); sentinel(y); before = *y;
    CHECK(adf_rfun_dilate_rat(y, x, q, PREC) == ADF_DOMAIN && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_dilate_rat(y, zero, q, PREC) == ADF_DOMAIN && !memcmp(&before, y, sizeof(before)));
    /* prec above the maximum: LIMIT first, before the DOMAIN of h = 0 */
    CHECK(adf_rfun_dilate_rat(y, x, q, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    rat_of(q, "1/3");
    CHECK(adf_rfun_translate_rat(y, x, q, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    CHECK(adf_rfun_add(y, x, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    CHECK(adf_rfun_mul(y, x, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    CHECK(adf_rfun_dilate_idele(y, x, a, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    acb_set_si(z, 7); acb_set(keep, z); arb_one(t);
    CHECK(adf_rfun_eval(z, x, t, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT && acb_equal(z, keep));
    CHECK(!memcmp(&before, y, sizeof(before)));
    /* prec 2, 53, the maximum, and prec <= 0 as 2 */
    CHECK(adf_rfun_translate_rat(y, x, q, 2) == ADF_OK && adf_rfun_is_canonical(y));
    CHECK(adf_rfun_translate_rat(y, x, q, 0) == ADF_OK && adf_rfun_is_canonical(y));
    CHECK(adf_rfun_translate_rat(y, x, q, -5) == ADF_OK && adf_rfun_is_canonical(y));
    CHECK(adf_rfun_translate_rat(y, x, q, 53) == ADF_OK && adf_rfun_is_canonical(y));
    CHECK(adf_rfun_translate_rat(y, x, q, ADF_REAL_PREC_MAX) == ADF_OK);
    CHECK(arb_rel_accuracy_bits(acb_realref(y->term[0].B)) > ADF_REAL_PREC_MAX - 64);
    CHECK(adf_rfun_eval(z, y, t, 2) == ADF_OK && acb_is_finite(z));
    /* Re(A) lost under rounding at prec 2: NOT_DETERMINED, y untouched */
    /* Re(A) = 1.25 +/- 1.2: the sums and products round their midpoint toward 0 at 2 bits (ARF_RND_DOWN) */
    s = "rfun(term(P=[(1) + (0)*i], A=(1.25 +/- 1.2) + (0.3)*i, B=(0) + (0)*i, C=(0) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK);
    rat_of(q, "3"); sentinel(y); before = *y;
    CHECK(adf_rfun_dilate_rat(y, x, q, 2) == ADF_NOT_DETERMINED && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_dilate_rat(y, x, q, PREC) == ADF_OK && arb_is_positive(acb_realref(y->term[0].A)));
    sentinel(y); before = *y;
    CHECK(adf_rfun_mul(y, x, x, 2) == ADF_NOT_DETERMINED && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_mul(y, x, x, PREC) == ADF_OK);
    /* the translation copies A: never NOT_DETERMINED for A */
    CHECK(adf_rfun_translate_rat(y, x, q, 2) == ADF_OK && acb_equal(y->term[0].A, x->term[0].A));
    /* idele dilation: h = a.inf, interval squaring: 1 +/- 0.875 gives h^2 in [1/64, 225/64], positive */
    s = "(1 +/- 0.875 ; 1 * [1])";
    CHECK(adf_idele_set_str(a, s, strlen(s), PREC, NULL) == ADF_OK);
    fun_simple(x, P, 2, 1, 0, 0.5, 0);
    CHECK(adf_rfun_dilate_idele(y, x, a, PREC) == ADF_OK && adf_rfun_is_canonical(y));
    arb_set_d(t, 1.0 / 64); CHECK(arb_contains(acb_realref(y->term[0].A), t));
    arb_set_d(t, 225.0 / 64); CHECK(arb_contains(acb_realref(y->term[0].A), t));
    arb_set_d(t, 0.015); CHECK(!arb_contains(acb_realref(y->term[0].A), t));   /* not two signed copies */
    arb_set_d(t, 0.0625); CHECK(arb_contains(acb_realref(y->term[0].B), t));
    arb_set_d(t, 0.9375); CHECK(arb_contains(acb_realref(y->term[0].B), t));
    arb_set_d(t, 3.75); CHECK(arb_contains(acb_realref(y->term[0].P->coeffs + 1), t));
    /* a negative real component: -2 +/- 0.5, h^2 in [2.25, 6.25], B h in [-1.25, -0.75] */
    s = "(-2 +/- 0.5 ; 3 * [-1])";
    CHECK(adf_idele_set_str(a, s, strlen(s), PREC, NULL) == ADF_OK);
    CHECK(adf_rfun_dilate_idele(y, x, a, PREC) == ADF_OK);
    arb_set_d(t, 2.25); CHECK(arb_contains(acb_realref(y->term[0].A), t));
    arb_set_d(t, 6.25); CHECK(arb_contains(acb_realref(y->term[0].A), t));
    arb_set_d(t, 2.2); CHECK(!arb_contains(acb_realref(y->term[0].A), t));
    arb_set_d(t, -1.25); CHECK(arb_contains(acb_realref(y->term[0].B), t));
    arb_set_d(t, -0.75); CHECK(arb_contains(acb_realref(y->term[0].B), t));
    arb_set_d(t, 0); CHECK(!arb_contains(acb_realref(y->term[0].B), t));
    /* an exact idele -2 equals dilate_rat by -2 */
    s = "(-2 ; 2 * [-1])";
    CHECK(adf_idele_set_str(a, s, strlen(s), PREC, NULL) == ADF_OK);
    { adf_rfun_t d; adf_rfun_init(d); rat_of(q, "-2");
      CHECK(adf_rfun_dilate_idele(y, x, a, PREC) == ADF_OK && adf_rfun_dilate_rat(d, x, q, PREC) == ADF_OK);
      CHECK(adf_rfun_identical(y, d)); adf_rfun_clear(d); }
    /* a ball near 0: 2^-100 +/- 2^-101 */
    arb_one(t); arb_mul_2exp_si(t, t, -100); mag_set_ui_2exp_si(arb_radref(t), 1, -101);
    { fmpq_t r; adf_ucoset_t u; fmpq_init(r); fmpq_one(r); adf_ucoset_init(u);
      CHECK(adf_idele_set_parts(a, t, r, u) == ADF_OK); fmpq_clear(r); adf_ucoset_clear(u); }
    CHECK(adf_rfun_dilate_idele(y, x, a, PREC) == ADF_OK && arb_is_positive(acb_realref(y->term[0].A)));
    /* evaluation: nonfinite x is DOMAIN; a nonfinite result is NOT_DETERMINED; z untouched */
    acb_set_si(z, 7); arb_pos_inf(t);
    CHECK(adf_rfun_eval(z, x, t, PREC) == ADF_DOMAIN && acb_equal(z, keep));
    arb_indeterminate(t);
    CHECK(adf_rfun_eval(z, x, t, PREC) == ADF_DOMAIN && acb_equal(z, keep));
    s = "rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(1e300) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK);
    arb_zero(t);
    CHECK(adf_rfun_eval(z, x, t, PREC) == ADF_NOT_DETERMINED && acb_equal(z, keep));
    /* the zero function and a zero polynomial evaluate to exact 0 */
    CHECK(adf_rfun_eval(z, zero, t, PREC) == ADF_OK && acb_is_zero(z));
    s = "rfun(term(P=[], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK);
    CHECK(adf_rfun_eval(z, x, t, PREC) == ADF_OK && acb_is_zero(z));
    /* a wide x ball: [-1, 1] on exp(-pi x^2): the image [exp(-pi), 1] is enclosed */
    fun_simple(x, P, 1, 1, 0, 0, 0);
    arb_zero(t); mag_one(arb_radref(t));
    CHECK(adf_rfun_eval(z, x, t, PREC) == ADF_OK && arb_contains_si(acb_realref(z), 1));
    { arb_t e; arb_init(e); arb_const_pi(e, PREC); arb_neg(e, e); arb_exp(e, e, PREC);
      CHECK(arb_contains(acb_realref(z), e)); arb_set_d(e, 1.5); CHECK(!arb_contains(acb_realref(z), e));
      arb_clear(e); }
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(zero); adf_rat_clear(q); adf_idele_clear(a);
    arb_clear(t); acb_clear(z); acb_clear(keep);
}

/* order: x + y against y + x, and the product order */
static void order(void)
{
    adf_rfun_t f, g, s, t; arb_t x; acb_t a, b; slong k; static const double P[2] = { 1, -1 }, R[1] = { 2 };
    adf_rfun_init(f); adf_rfun_init(g); adf_rfun_init(s); adf_rfun_init(t); arb_init(x); acb_init(a); acb_init(b);
    fun_simple(f, P, 2, 1, 0, 0, 0); fun_simple(g, R, 1, 2, 1, 1, 0);
    CHECK(adf_rfun_add(s, f, g, PREC) == ADF_OK && adf_rfun_add(t, g, f, PREC) == ADF_OK);
    CHECK(s->len == 2 && !adf_rfun_identical(s, t));
    CHECK(acb_equal(s->term[0].A, f->term[0].A) && acb_equal(t->term[0].A, g->term[0].A));
    for (k = -4; k <= 4; k++) {
        arb_set_si(x, k); arb_mul_2exp_si(x, x, -1);
        CHECK(adf_rfun_eval(a, s, x, PREC) == ADF_OK && adf_rfun_eval(b, t, x, PREC) == ADF_OK);
        CHECK(acb_overlaps(a, b));
    }
    /* the product of (f + g) and (g + f): pairs (0,0),(0,1),(1,0),(1,1) */
    { adf_rfun_t m; adf_rfun_init(m);
      CHECK(adf_rfun_mul(m, s, t, PREC) == ADF_OK && m->len == 4);
      /* f: A = 1; g: A = 2 + i. Pairs (f,g), (f,f), (g,g), (g,f): A = 3 + i, 2, 4 + 2i, 3 + i */
      CHECK(acb_equal(m->term[0].A, m->term[3].A) && acb_equal_si(m->term[1].A, 2));
      CHECK(arb_equal_si(acb_realref(m->term[2].A), 4) && arb_equal_si(acb_imagref(m->term[2].A), 2));
      CHECK(arb_equal_si(acb_realref(m->term[0].A), 3) && arb_equal_si(acb_imagref(m->term[0].A), 1));
      arb_set_si(x, 1); arb_mul_2exp_si(x, x, -2);
      CHECK(adf_rfun_eval(a, m, x, PREC) == ADF_OK && adf_rfun_eval(b, s, x, PREC) == ADF_OK);
      acb_mul(b, b, b, PREC); CHECK(acb_overlaps(a, b));
      adf_rfun_clear(m); }
    adf_rfun_clear(f); adf_rfun_clear(g); adf_rfun_clear(s); adf_rfun_clear(t);
    arb_clear(x); acb_clear(a); acb_clear(b);
}

/* aliasing of every permitted combination, with nonzero radii: the aliased result is identical */
static void aliasing(void)
{
    adf_rfun_t x, y, r, a; adf_rat_t q; adf_idele_t h; const char *s;
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(r); adf_rfun_init(a); adf_rat_init(q); adf_idele_init(h);
    s = "rfun(term(P=[(1 +/- 0.25) + (0.5)*i, (0) + (1 +/- 0.125)*i], A=(1.5 +/- 0.25) + (1)*i, "
        "B=(0.5 +/- 0.125) + (-1)*i, C=(0 +/- 0.5) + (0)*i), term(P=[(2) + (0)*i], A=(1) + (0)*i, "
        "B=(0) + (0)*i, C=(0) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK);
    s = "rfun(term(P=[(3) + (0 +/- 0.5)*i], A=(2 +/- 0.5) + (-1)*i, B=(1) + (0)*i, C=(0) + (0.25 +/- 0.25)*i))";
    CHECK(adf_rfun_set_str(y, s, strlen(s), PREC, NULL) == ADF_OK);
    rat_of(q, "-5/3");
    s = "(-1.5 +/- 0.25 ; 1 * [-1])";
    CHECK(adf_idele_set_str(h, s, strlen(s), PREC, NULL) == ADF_OK);
#define ALIAS2(call_r, call_a) do { CHECK((call_r) == ADF_OK); adf_rfun_set(a, x); CHECK((call_a) == ADF_OK); \
    CHECK(adf_rfun_identical(a, r)); } while (0)
    ALIAS2(adf_rfun_translate_rat(r, x, q, PREC), adf_rfun_translate_rat(a, a, q, PREC));
    ALIAS2(adf_rfun_dilate_rat(r, x, q, PREC), adf_rfun_dilate_rat(a, a, q, PREC));
    ALIAS2(adf_rfun_dilate_idele(r, x, h, PREC), adf_rfun_dilate_idele(a, a, h, PREC));
    ALIAS2(adf_rfun_reflect(r, x), adf_rfun_reflect(a, a));
    ALIAS2(adf_rfun_conj(r, x), adf_rfun_conj(a, a));
    ALIAS2(adf_rfun_add(r, x, y, PREC), adf_rfun_add(a, a, y, PREC));
    ALIAS2(adf_rfun_mul(r, x, y, PREC), adf_rfun_mul(a, a, y, PREC));
    ALIAS2(adf_rfun_add(r, x, x, PREC), adf_rfun_add(a, a, a, PREC));
    ALIAS2(adf_rfun_mul(r, x, x, PREC), adf_rfun_mul(a, a, a, PREC));
    ALIAS2(adf_rfun_add(r, y, x, PREC), adf_rfun_add(a, y, a, PREC));
    ALIAS2(adf_rfun_mul(r, y, x, PREC), adf_rfun_mul(a, y, a, PREC));
    ALIAS2(adf_rfun_add(r, x, x, PREC), adf_rfun_add(a, x, a, PREC));
#undef ALIAS2
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(r); adf_rfun_clear(a);
    adf_rat_clear(q); adf_idele_clear(h);
}

static void debug(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int k;
    for (k = 0; k < 8; k++) {
        pid_t p = fork(); int status; CHECK(p >= 0);
        if (!p) {
            adf_rfun_t x, y; adf_rat_t q; arb_t t; acb_t z; static const double P[1] = { 1 };
            adf_rfun_init(x); adf_rfun_init(y); adf_rat_init(q); arb_init(t); acb_init(z);
            fun_simple(x, P, 1, 1, 0, 0, 0);
            arb_neg(acb_realref(x->term[0].A), acb_realref(x->term[0].A));      /* Re(A) < 0 */
            if (k == 0) (void) adf_rfun_add(y, x, x, PREC);
            if (k == 1) (void) adf_rfun_translate_rat(y, x, q, PREC);
            if (k == 2) (void) adf_rfun_eval(z, x, t, PREC);
            if (k == 3) { arb_neg(acb_realref(x->term[0].A), acb_realref(x->term[0].A));
                          fmpz_zero(fmpq_denref(q->q)); (void) adf_rfun_dilate_rat(y, x, q, PREC); }
            if (k == 4) (void) adf_rfun_add(y, x, y, PREC);      /* y canonical: x alone is checked */
            if (k == 5) (void) adf_rfun_add(y, y, x, PREC);
            if (k == 6) (void) adf_rfun_mul(y, x, y, PREC);
            if (k == 7) (void) adf_rfun_mul(y, y, x, PREC);
            _exit(0);
        }
        CHECK(waitpid(p, &status, 0) == p && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
#endif
}

int main(void)
{
    lifecycle();
    golden();
    texts();
    reader_cases();
    closure();
    values();
    shifted_gaussian();
    statuses();
    order();
    aliasing();
    caps();
    debug();
    printf("rfun: %lu checks\n", checks);
    flint_cleanup();
    return 0;
}
