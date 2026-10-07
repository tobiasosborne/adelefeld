/* tests/test_rfun_fourier.c: slice 4e of docs/api-4.md, the transform, derivative, integral and norm of adf_rfun
   (include/adelefeld/rfun.h; statements in docs/api-4b.md "Slice 4e").
   Contract: docs/api-4.md sections 1, 5 (R1, R2), 6, 8; analysis Proposition 5 (docs/proofs/analysis.md:174-212);
   conventions 6.1 (the transform F f(y) = integral f(x) conj(psi_inf(x y)) dx with psi_inf(x) = E(-x), so the
   real kernel is E(+x y)).
   Oracle: tests/ref/vectors/f4-slice3/ written by lanes/f4-slice3/gen_vectors.py from proto/functions4_checks.py
   (rterm_transform, rterm_derivative, rterm_product, rvalue_ball; balls at 400 bits printed with 60 digits; a
   numerical quadrature as the second source). Every record is run.
   Tightness, stated against the radius (a test of containment alone passes a function that returns huge
   balls): for exact inputs at PREC = 128 bits every parameter, coefficient, value, integral and norm has
   radius at most 2^-96 (1 + |v|), v the oracle value; for inputs with ball parameters (radii up to 2^-12)
   at most 2^-2 (1 + |v|). Exceptions, each stated where it is made: F(F(x)) at Re(A) = 10^-30 (containment
   only), and comparisons of two enclosures (overlap with both radii at most 2^-80 (1 + |v|)). */
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
#define TIGHT_EXACT (-96)
#define TIGHT_BALL (-2)
#define LOOSE 100000

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
/* An oracle ball [re, im], two arb texts of 60 digits. */
static void ref_ball(acb_t z, const jsonl_value *v)
{
    CHECK(arb_set_str(acb_realref(z), str(at(v, 0)), 400) == 0);
    CHECK(arb_set_str(acb_imagref(z), str(at(v, 1)), 400) == 0);
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
/* a sentinel value whose bytes must survive a failing call */
static void sentinel(adf_rfun_t x)
{
    static const double P[2] = { 3, -1 };
    fun_simple(x, P, 2, 2, 0.5, 0.25, -1);
}
static void rat_of(adf_rat_t r, const char *s) { q_of(r->q, s); }

/* ------------------------------------------------------------------ checks */
/* a part of a result contains the part of the oracle ball; where the result part is exact (radius 0) and the
   oracle's is not (a ball from 400-bit arithmetic around an exact value), the exact point must lie in it */
static int part_in(const arb_t r, const arb_t z)
{
    return arb_contains(r, z) || (arb_is_exact(r) && arb_contains(z, r));
}
static int ball_in(const acb_t r, const acb_t z)
{
    return part_in(acb_realref(r), acb_realref(z)) && part_in(acb_imagref(r), acb_imagref(z));
}
/* r contains the oracle ball z (as ball_in), and its radius is at most 2^e (1 + |z|). */
static void check_in(const acb_t r, const acb_t z, slong e)
{
    mag_t m, b; mag_init(m); mag_init(b);
    if (!ball_in(r, z)) {
        fprintf(stderr, "result: "); acb_printd(r, 30); fprintf(stderr, "\noracle: "); acb_printd(z, 30);
        fprintf(stderr, "\n");
    }
    CHECK(ball_in(r, z));
    acb_get_mag(b, z); mag_add_ui(b, b, 1); mag_mul_2exp_si(b, b, e);
    mag_hypot(m, arb_radref(acb_realref(r)), arb_radref(acb_imagref(r)));
    if (mag_cmp(m, b) > 0) {
        fprintf(stderr, "radius: "); mag_printd(m, 5); fprintf(stderr, " bound: "); mag_printd(b, 5);
        fprintf(stderr, "\n");
    }
    CHECK(mag_cmp(m, b) <= 0);
    mag_clear(m); mag_clear(b);
}
/* the radius of r is at most 2^e (1 + |r|) */
static void check_tight(const acb_t r, slong e)
{
    mag_t m, b; mag_init(m); mag_init(b);
    acb_get_mag(b, r); mag_add_ui(b, b, 1); mag_mul_2exp_si(b, b, e);
    mag_hypot(m, arb_radref(acb_realref(r)), arb_radref(acb_imagref(r)));
    CHECK(mag_cmp(m, b) <= 0);
    mag_clear(m); mag_clear(b);
}
/* a result term against an oracle term of balls ({"P": [[re, im]...], "A": .., "B": .., "C": ..}) */
static void check_ball_term(const adf_rterm_struct *R, const jsonl_value *E, slong e)
{
    const jsonl_value *P = field(E, "P"); slong j, n = (slong) jsonl_size(P); acb_t z; acb_init(z);
    CHECK(acb_poly_length(R->P) == n);
    for (j = 0; j < n; j++) { ref_ball(z, at(P, (size_t) j)); check_in(R->P->coeffs + j, z, e); }
    ref_ball(z, field(E, "A")); check_in(R->A, z, e);
    ref_ball(z, field(E, "B")); check_in(R->B, z, e);
    ref_ball(z, field(E, "C")); check_in(R->C, z, e);
    CHECK(arb_is_positive(acb_realref(R->A)));
    acb_clear(z);
}
/* a result term against an exact oracle term in pi form; coefficients beyond the oracle's length contain 0 */
static void check_exact_term(const adf_rterm_struct *R, const jsonl_value *E, slong e)
{
    const jsonl_value *P = field(E, "P"); slong j, n = (slong) jsonl_size(P); acb_t z; acb_init(z);
    CHECK(acb_poly_length(R->P) >= n);
    for (j = 0; j < acb_poly_length(R->P); j++) {
        if (j < n) ex_num(z, at(P, (size_t) j)); else acb_zero(z);
        check_in(R->P->coeffs + j, z, e);
    }
    ex_num(z, field(E, "A")); check_in(R->A, z, e);
    ex_num(z, field(E, "B")); check_in(R->B, z, e);
    ex_num(z, field(E, "C")); check_in(R->C, z, e);
    acb_clear(z);
}
/* two enclosures of one value overlap; with tight radii (checked) this is a real comparison */
static void check_same(const acb_t a, const acb_t b, slong e)
{
    if (!acb_overlaps(a, b)) {
        fprintf(stderr, "a: "); acb_printd(a, 30); fprintf(stderr, "\nb: "); acb_printd(b, 30);
        fprintf(stderr, "\n");
    }
    CHECK(acb_overlaps(a, b));
    check_tight(a, e); check_tight(b, e);
}
static int all_A_at_least(const adf_rfun_t f, double lo)
{
    slong i; arb_t t; int ok = 1; arb_init(t); arb_set_d(t, lo);
    for (i = 0; i < f->len; i++) ok &= arb_ge(acb_realref(f->term[i].A), t);
    arb_clear(t); return ok;
}

/* ------------------------------------------------------------------ the oracle vectors */
static void vectors(void)
{
    jsonl_file *f; jsonl_error_t e; size_t i, k, m; slong t;
    adf_rfun_t x, y, yy, d, xc, pr; acb_t z, w, r; arb_t n2, ya; fmpq_t q;
    unsigned long nhat = 0, nquad = 0;
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(yy); adf_rfun_init(d); adf_rfun_init(xc); adf_rfun_init(pr);
    acb_init(z); acb_init(w); acb_init(r); arb_init(n2); arb_init(ya); fmpq_init(q);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice3/transform.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 60);
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i), *hat = field(v, "hat");
        int exact = boolean(field(v, "exact_in")); slong te = exact ? TIGHT_EXACT : TIGHT_BALL, tb;
        fun_from_json(x, field(v, "x"), PREC);
        /* tb: the bound for integrals, norms and values. At Re(A) = 10^-30 the exponents (C + B^2/(4 pi A),
           and A' y^2, B' y of the transform) have size up to 2^100, so at 128 bits each has an absolute error
           near 2^-28 and exp of their sum a relative error of a few times that, whatever the method: there
           the bound is 2^-16 (1 + |v|) */
        tb = all_A_at_least(x, 0.01) ? te : FLINT_MAX(te, -16);
        /* the transform: every member's exact transform (R1) in the result, term by term */
        CHECK(adf_rfun_fourier(y, x, PREC) == ADF_OK && adf_rfun_is_canonical(y) && y->len == x->len);
        CHECK((size_t) x->len == jsonl_size(hat));
        for (t = 0; t < x->len; t++)
            for (m = 0; m < jsonl_size(at(hat, (size_t) t)); m++, nhat++)
                check_ball_term(y->term + t, at(at(hat, (size_t) t), m), te);
        /* F(F(x)) encloses the reflection of every member: amplitude 1 (R2 step 3). Tightness only where
           every Re(A) >= 1/100: at Re(A) = 10^-30 the coefficients of F(x) have size (2 pi A)^-j and the
           second transform cancels them at 128 bits (a property of the arithmetic, not of the formula) */
        CHECK(adf_rfun_fourier(yy, y, PREC) == ADF_OK && yy->len == x->len);
        for (t = 0; t < x->len; t++)
            for (m = 0; m < jsonl_size(at(field(v, "refl"), (size_t) t)); m++)
                check_exact_term(yy->term + t, at(at(field(v, "refl"), (size_t) t), m),
                                 all_A_at_least(x, 0.01) ? te + 6 : LOOSE);
        /* the derivative P' + (B - 2 pi A x) P */
        CHECK(adf_rfun_derivative(d, x, PREC) == ADF_OK && adf_rfun_is_canonical(d) && d->len == x->len);
        for (t = 0; t < x->len; t++)
            for (m = 0; m < jsonl_size(at(field(v, "deriv"), (size_t) t)); m++)
                check_exact_term(d->term + t, at(at(field(v, "deriv"), (size_t) t), m), te);
        /* the integral: the oracle's F(phi)(0), and our own transform at 0 (an independent path: the value
           recurrence h_j against the polynomial recurrence of R1) */
        CHECK(adf_rfun_integral(z, x, PREC) == ADF_OK);
        for (m = 0; m < jsonl_size(field(v, "integral")); m++) {
            ref_ball(w, at(field(v, "integral"), m)); check_in(z, w, tb);
        }
        arb_zero(ya);
        CHECK(adf_rfun_eval(w, y, ya, PREC) == ADF_OK);
        if (exact && tb == te) check_same(z, w, -80); else CHECK(acb_overlaps(z, w));
        /* the norm: the oracle's sum over every ordered pair; the integral of phi conj(phi) built by mul and
           conj of slice 4d */
        CHECK(adf_rfun_norm2(n2, x, PREC) == ADF_OK);
        CHECK(arb_is_nonnegative(n2) || arb_is_zero(n2));
        acb_set_arb(r, n2);
        for (m = 0; m < jsonl_size(field(v, "norm2")); m++) {
            ref_ball(w, at(field(v, "norm2"), m));
            arb_zero(acb_imagref(w));                   /* the oracle's imaginary part is a ball around 0 */
            check_in(r, w, tb);
        }
        CHECK(adf_rfun_conj(xc, x) == ADF_OK && adf_rfun_mul(pr, x, xc, PREC) == ADF_OK);
        CHECK(adf_rfun_integral(w, pr, PREC) == ADF_OK);
        CHECK(arb_contains_zero(acb_imagref(w)) && arb_overlaps(acb_realref(w), n2));
        /* values of the transform at y: the oracle's members contained in adf_rfun_eval of the result */
        for (k = 0; k < jsonl_size(field(v, "values")); k++) {
            const jsonl_value *pt = at(field(v, "values"), k);
            q_of(q, str(field(pt, "y"))); arb_set_fmpq(ya, q, PREC);
            CHECK(adf_rfun_eval(r, y, ya, PREC) == ADF_OK);
            for (m = 0; m < jsonl_size(field(pt, "members")); m++) {
                ref_ball(w, at(field(pt, "members"), m)); check_in(r, w, exact && tb == te ? te + 8 : tb);
            }
        }
        /* the second source: mp.quad at 60 digits (a numerical check, margin 1e-40 relative) */
        if (has_field(v, "quad_norm2")) {
            arb_t qv, mar; arb_init(qv); arb_init(mar); nquad++;
            CHECK(arb_set_str(qv, str(at(field(v, "quad_integral"), 0)), 400) == 0);
            arb_set_str(mar, "1e-40", 400); arb_mul(mar, mar, qv, 400); arb_abs(mar, mar);
            arb_add_ui(mar, mar, 0, 400); arb_add_error(qv, mar); arb_add_error_2exp_si(qv, -133);
            CHECK(arb_overlaps(acb_realref(z), qv));
            CHECK(arb_set_str(qv, str(at(field(v, "quad_integral"), 1)), 400) == 0);
            arb_add_error(qv, mar); arb_add_error_2exp_si(qv, -133);
            CHECK(arb_overlaps(acb_imagref(z), qv));
            CHECK(arb_set_str(qv, str(field(v, "quad_norm2")), 400) == 0);
            arb_set_str(mar, "1e-40", 400); arb_mul(mar, mar, qv, 400); arb_abs(mar, mar);
            arb_add_error(qv, mar); arb_add_error_2exp_si(qv, -133);
            CHECK(arb_overlaps(n2, qv));
            for (k = 0; k < jsonl_size(field(v, "quad_hat")); k++) {
                const jsonl_value *pt = at(field(v, "quad_hat"), k);
                q_of(q, str(field(pt, "y"))); arb_set_fmpq(ya, q, PREC);
                CHECK(adf_rfun_eval(r, y, ya, PREC) == ADF_OK);
                CHECK(arb_set_str(acb_realref(w), str(at(field(pt, "v"), 0)), 400) == 0);
                CHECK(arb_set_str(acb_imagref(w), str(at(field(pt, "v"), 1)), 400) == 0);
                arb_set_str(mar, "1e-40", 400);
                arb_add_error(acb_realref(w), mar); arb_add_error(acb_imagref(w), mar);
                CHECK(acb_overlaps(r, w));
            }
            arb_clear(qv); arb_clear(mar);
        }
    }
    CHECK(nhat >= 100 && nquad >= 10);
    jsonl_close(f);
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(yy); adf_rfun_clear(d); adf_rfun_clear(xc);
    adf_rfun_clear(pr); acb_clear(z); acb_clear(w); acb_clear(r); arb_clear(n2); arb_clear(ya); fmpq_clear(q);
}

/* ------------------------------------------------------------------ exact cases of R1 and P5 */
static void exact_cases(void)
{
    adf_rfun_t x, y, want; acb_t z; arb_t n;
    static const double G[1] = { 1 }, X1[2] = { 0, 1 };
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(want); acb_init(z); arb_init(n);
    /* F(exp(-pi x^2)) = exp(-pi y^2), exactly (P5) */
    fun_simple(x, G, 1, 1, 0, 0, 0);
    CHECK(adf_rfun_fourier(y, x, PREC) == ADF_OK && adf_rfun_identical(y, x));
    /* F(x exp(-pi x^2)) = i y exp(-pi y^2), exactly (R1 step 2): Q = [0, i], A = 1, B = C = 0 */
    fun_simple(x, X1, 2, 1, 0, 0, 0);
    CHECK(adf_rfun_fourier(y, x, PREC) == ADF_OK);
    adf_rfun_set(want, x); acb_onei(want->term[0].P->coeffs + 1);
    CHECK(adf_rfun_identical(y, want));
    /* and the second transform is the reflection -x exp(-pi x^2), exactly */
    CHECK(adf_rfun_fourier(want, y, PREC) == ADF_OK && adf_rfun_reflect(y, x) == ADF_OK);
    CHECK(adf_rfun_identical(want, y));
    /* A = 4: the amplitude A^(-1/2) = 1/2 (fault of api-4.md 8 for 4.3: omit A^-1/2 at A = 4) */
    fun_simple(x, G, 1, 4, 0, 0, 0);
    CHECK(adf_rfun_fourier(y, x, PREC) == ADF_OK);
    acb_set_d(z, 0.5); CHECK(acb_equal(y->term[0].P->coeffs, z));
    acb_set_d(z, 0.25); CHECK(acb_equal(y->term[0].A, z));
    /* its integral is 1/2 and its norm (8)^(-1/2) */
    CHECK(adf_rfun_integral(z, x, PREC) == ADF_OK);
    {   acb_t h; acb_init(h); acb_set_d(h, 0.5);
        CHECK(acb_contains(z, h)); check_tight(z, TIGHT_EXACT); acb_clear(h); }
    /* both root quadrants: A = 1 + 2i and 1 - 2i; the root has positive real part, the roots of A and 1/A are
       reciprocal, so F^2 has amplitude 1 and not -1 (R2) */
    {   int s; for (s = -1; s <= 1; s += 2) {
        acb_t root, one; acb_init(root); acb_init(one);
        fun_simple(x, G, 1, 1, 2 * s, 0, 0);
        CHECK(adf_rfun_fourier(y, x, PREC) == ADF_OK);
        CHECK(arb_is_positive(acb_realref(y->term[0].P->coeffs)));
        CHECK(s > 0 ? arb_is_negative(acb_imagref(y->term[0].P->coeffs))
                    : arb_is_positive(acb_imagref(y->term[0].P->coeffs)));
        /* (A^(-1/2))^2 A = 1 */
        acb_sqr(root, y->term[0].P->coeffs, PREC); acb_mul(root, root, x->term[0].A, PREC);
        acb_one(one); CHECK(acb_contains(root, one));
        CHECK(adf_rfun_fourier(y, y, PREC) == ADF_OK);
        CHECK(acb_contains(y->term[0].P->coeffs, one));
        acb_neg(one, one); CHECK(!acb_overlaps(y->term[0].P->coeffs, one));
        acb_clear(root); acb_clear(one); } }
    /* norm of a single real Gaussian, closed form (2 a)^(-1/2) exp(2 c + b^2/(2 pi a)) with A = a, B = b, C = c
       real: |phi|^2 = exp(-2 pi a x^2 + 2 b x + 2 c), and P5 step 3 */
    {   arb_t a, b, c, t, u, pi; acb_t hold;
        arb_init(a); arb_init(b); arb_init(c); arb_init(t); arb_init(u); arb_init(pi); acb_init(hold);
        fun_simple(x, G, 1, 0.75, 0, -1.5, 0.25);
        CHECK(adf_rfun_norm2(n, x, PREC) == ADF_OK);
        arb_set_d(a, 0.75); arb_set_d(b, -1.5); arb_set_d(c, 0.25); arb_const_pi(pi, 300);
        arb_mul_2exp_si(t, a, 1); arb_rsqrt(t, t, 300);
        arb_sqr(u, b, 300); arb_div(u, u, pi, 300); arb_div(u, u, a, 300); arb_mul_2exp_si(u, u, -1);
        arb_mul_2exp_si(c, c, 1); arb_add(u, u, c, 300); arb_exp(u, u, 300); arb_mul(t, t, u, 300);
        CHECK(arb_contains(n, t)); acb_set_arb(hold, n); check_tight(hold, TIGHT_EXACT);
        /* a complex Gaussian: |phi|^2 has the real parts, (2 Re A)^(-1/2) exp(2 Re C + (Re B)^2/(2 pi Re A)) */
        fun_simple(x, G, 1, 0.75, 3, -1.5, 0.25);
        /* imaginary parts of B and C do not enter */
        arb_set_d(acb_imagref(x->term[0].B), 2.5); arb_set_d(acb_imagref(x->term[0].C), -7);
        CHECK(adf_rfun_norm2(n, x, PREC) == ADF_OK);
        CHECK(arb_contains(n, t)); acb_set_arb(hold, n); check_tight(hold, TIGHT_EXACT);
        arb_clear(a); arb_clear(b); arb_clear(c); arb_clear(t); arb_clear(u); arb_clear(pi); acb_clear(hold); }
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(want); acb_clear(z); arb_clear(n);
}

/* ------------------------------------------------------------------ the sign test of PLAN 4.3 (R1 step 3) */
/* exp(-pi y^2) E(y/3) at y, the transform of exp(-pi (x - 1/3)^2) by R1 step 3, at 300 bits */
static void shifted_exact(acb_t z, const arb_t y)
{
    arb_t t, pi; acb_t e; arb_init(t); arb_init(pi); acb_init(e);
    arb_const_pi(pi, 300); arb_sqr(t, y, 300); arb_mul(t, t, pi, 300); arb_neg(t, t);
    acb_set_arb(z, t); acb_exp(z, z, 300);
    arb_div_ui(t, y, 3, 300); arb_mul_2exp_si(t, t, 1);       /* E(y/3) = exp(pi i 2y/3) */
    acb_set_arb(e, t); acb_exp_pi_i(e, e, 300); acb_mul(z, z, e, 300);
    arb_clear(t); arb_clear(pi); acb_clear(e);
}
static void sign_test(void)
{
    jsonl_file *f; jsonl_error_t e; adf_rfun_t x, y, g; adf_rterm_struct *t; adf_rat_t q; acb_t z, w; arb_t ya;
    static const double G[1] = { 1 };
    int k;
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(g); adf_rat_init(q); acb_init(z); acb_init(w); arb_init(ya);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice3/shifted.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 1);
    {   const jsonl_value *v = jsonl_record(f, 0), *T = field(v, "term"), *P = field(T, "P");
        t = terms_new(1);
        CHECK(jsonl_size(P) == 1);
        ex_num(z, at(P, 0)); acb_set_round(z, z, PREC); acb_poly_set_coeff_acb(t->P, 0, z);
        ex_num(t->A, field(T, "A")); acb_set_round(t->A, t->A, PREC);
        ex_num(t->B, field(T, "B")); acb_set_round(t->B, t->B, PREC);   /* 2 pi/3 */
        ex_num(t->C, field(T, "C")); acb_set_round(t->C, t->C, PREC);   /* -pi/9 */
        CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK); terms_free(t, 1);
        CHECK(adf_rfun_fourier(y, x, PREC) == ADF_OK);
        check_ball_term(y->term, field(v, "hat"), TIGHT_EXACT + 4);
        arb_set_d(ya, 0.25);
        CHECK(adf_rfun_eval(z, y, ya, PREC) == ADF_OK);
        ref_ball(w, field(v, "value")); check_in(z, w, TIGHT_EXACT + 4);
        /* the imaginary part at y = 1/4 is positive; the opposite convention gives the conjugate */
        CHECK(arb_is_positive(acb_imagref(z)));
        shifted_exact(w, ya); CHECK(acb_contains(z, w) || acb_overlaps(z, w));
        acb_conj(w, w); CHECK(!acb_overlaps(z, w)); }
    jsonl_close(f);
    /* end to end with slice 4d: translate exp(-pi x^2) by 1/3, transform, evaluate: exp(-pi y^2) E(y/3) */
    fun_simple(g, G, 1, 1, 0, 0, 0); rat_of(q, "1/3");
    CHECK(adf_rfun_translate_rat(x, g, q, PREC) == ADF_OK && adf_rfun_fourier(y, x, PREC) == ADF_OK);
    for (k = -6; k <= 6; k++) {
        arb_set_si(ya, k); arb_div_ui(ya, ya, 4, PREC);
        CHECK(adf_rfun_eval(z, y, ya, PREC) == ADF_OK);
        shifted_exact(w, ya); check_same(z, w, -90);
        if (k == 1) CHECK(arb_is_positive(acb_imagref(z)));
    }
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(g); adf_rat_clear(q); acb_clear(z); acb_clear(w);
    arb_clear(ya);
}

/* ------------------------------------------------------------------ covariance and the derivative */
/* the exact functions of the vectors with Re(A) >= 1/4 everywhere */
static int next_fun(jsonl_file *f, size_t *i, adf_rfun_t x)
{
    for (; *i < jsonl_count(f); (*i)++) {
        const jsonl_value *v = jsonl_record(f, *i);
        if (!boolean(field(v, "exact_in")) || jsonl_size(field(v, "x")) == 0) continue;
        fun_from_json(x, field(v, "x"), PREC);
        if (!all_A_at_least(x, 0.25)) continue;
        (*i)++; return 1;
    }
    return 0;
}
static void covariance(void)
{
    jsonl_file *f; jsonl_error_t e; size_t i = 0; int k, nf = 0;
    adf_rfun_t x, y, d, fd, fx; adf_rat_t h; acb_t a, b, c; arb_t ya, yb, hh;
    static const char *hs[4] = { "2/3", "-5", "3", "-1/2" };
    static const char *qs[3] = { "1/3", "-7/2", "5/8" };
    static const double ys[3] = { 0.25, -0.4, 1.125 };
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(d); adf_rfun_init(fd); adf_rfun_init(fx); adf_rat_init(h);
    acb_init(a); acb_init(b); acb_init(c); arb_init(ya); arb_init(yb); arb_init(hh);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice3/transform.jsonl", &f, &e));
    while (next_fun(f, &i, x) && nf < 16) {
        nf++;
        CHECK(adf_rfun_fourier(fx, x, PREC) == ADF_OK);
        /* dilation: F(D_h phi)(y) = |h|^-1 F(phi)(y/h) (SPEC 7, analysis P5 step 6), h negative included */
        for (k = 0; k < 4; k++) {
            int j;
            rat_of(h, hs[k]); arb_set_fmpq(hh, h->q, PREC);
            CHECK(adf_rfun_dilate_rat(d, x, h, PREC) == ADF_OK && adf_rfun_fourier(fd, d, PREC) == ADF_OK);
            for (j = 0; j < 3; j++) {
                arb_set_d(ya, ys[j]);
                CHECK(adf_rfun_eval(a, fd, ya, PREC) == ADF_OK);
                arb_div(yb, ya, hh, PREC);
                CHECK(adf_rfun_eval(b, fx, yb, PREC) == ADF_OK);
                arb_abs(yb, hh); acb_div_arb(b, b, yb, PREC);
                check_same(a, b, -80);
            }
        }
        /* translation: F(T_q phi)(y) = E(q y) F(phi)(y), positive kernel (conventions 6.1) */
        for (k = 0; k < 3; k++) {
            int j;
            rat_of(h, qs[k]);
            CHECK(adf_rfun_translate_rat(d, x, h, PREC) == ADF_OK && adf_rfun_fourier(fd, d, PREC) == ADF_OK);
            for (j = 0; j < 3; j++) {
                arb_set_d(ya, ys[j]);
                CHECK(adf_rfun_eval(a, fd, ya, PREC) == ADF_OK && adf_rfun_eval(b, fx, ya, PREC) == ADF_OK);
                arb_set_fmpq(yb, h->q, PREC); arb_mul(yb, yb, ya, PREC); arb_mul_2exp_si(yb, yb, 1);
                acb_set_arb(c, yb); acb_exp_pi_i(c, c, PREC); acb_mul(b, b, c, PREC);
                check_same(a, b, -80);
            }
        }
        /* the derivative against central differences at 300 bits, h = 2^-50, error O(h^2) */
        for (k = 0; k < 3; k++) {
            arb_t xm, xp, s; acb_t vm, vp; arb_init(xm); arb_init(xp); arb_init(s); acb_init(vm); acb_init(vp);
            CHECK(adf_rfun_derivative(d, x, PREC) == ADF_OK);
            arb_set_d(ya, ys[k] * 1.7 - 0.3);
            CHECK(adf_rfun_eval(a, d, ya, PREC) == ADF_OK);
            arb_one(s); arb_mul_2exp_si(s, s, -50);
            arb_sub(xm, ya, s, 300); arb_add(xp, ya, s, 300);
            CHECK(adf_rfun_eval(vm, x, xm, 300) == ADF_OK && adf_rfun_eval(vp, x, xp, 300) == ADF_OK);
            acb_sub(b, vp, vm, 300); acb_mul_2exp_si(b, b, 49);
            mag_set_ui_2exp_si(arb_radref(s), 1, -70); arb_zero(xm); arb_add_error_2exp_si(xm, -70);
            arb_add_error(acb_realref(b), xm); arb_add_error(acb_imagref(b), xm);
            CHECK(acb_overlaps(a, b)); check_tight(a, -90);
            arb_clear(xm); arb_clear(xp); arb_clear(s); acb_clear(vm); acb_clear(vp);
        }
    }
    CHECK(nf >= 10);
    /* the sign of the translation phase is asserted: E(-q y) is disjoint at q = 1/3, y = 1/4 for exp(-pi x^2) */
    {   static const double G[1] = { 1 };
        fun_simple(x, G, 1, 1, 0, 0, 0); rat_of(h, "1/3");
        CHECK(adf_rfun_translate_rat(d, x, h, PREC) == ADF_OK && adf_rfun_fourier(fd, d, PREC) == ADF_OK);
        arb_set_d(ya, 0.25);
        CHECK(adf_rfun_eval(a, fd, ya, PREC) == ADF_OK);
        CHECK(adf_rfun_fourier(fx, x, PREC) == ADF_OK && adf_rfun_eval(b, fx, ya, PREC) == ADF_OK);
        arb_set_d(yb, -2.0 / 12); acb_set_arb(c, yb); acb_exp_pi_i(c, c, PREC); acb_mul(c, b, c, PREC);
        CHECK(!acb_overlaps(a, c)); }
    /* the derivative as functions: P = 1 + 2x, A = 1 + i, B = 1/2, C = 0 gives
       2 + (B - 2 pi A x)(1 + 2x) = (2 + 1/2) + (1 - 2 pi A) x - 4 pi A x^2 */
    {   static const double P[2] = { 1, 2 }; acb_t want, pa; acb_init(want); acb_init(pa);
        fun_simple(x, P, 2, 1, 1, 0.5, 0);
        CHECK(adf_rfun_derivative(d, x, PREC) == ADF_OK && acb_poly_length(d->term[0].P) == 3);
        acb_const_pi(pa, 300); acb_mul(pa, pa, x->term[0].A, 300);
        acb_set_d(want, 2.5); CHECK(acb_contains(d->term[0].P->coeffs, want));
        acb_mul_2exp_si(want, pa, 1); acb_neg(want, want); acb_add_ui(want, want, 1, 300);
        CHECK(acb_contains(d->term[0].P->coeffs + 1, want)); check_tight(d->term[0].P->coeffs + 1, TIGHT_EXACT);
        acb_mul_2exp_si(want, pa, 2); acb_neg(want, want);
        CHECK(acb_contains(d->term[0].P->coeffs + 2, want)); check_tight(d->term[0].P->coeffs + 2, TIGHT_EXACT);
        CHECK(acb_equal(d->term[0].A, x->term[0].A) && acb_equal(d->term[0].B, x->term[0].B));
        CHECK(acb_equal(d->term[0].C, x->term[0].C));
        acb_clear(want); acb_clear(pa); }
    jsonl_close(f);
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(d); adf_rfun_clear(fd); adf_rfun_clear(fx);
    adf_rat_clear(h);
    acb_clear(a); acb_clear(b); acb_clear(c); arb_clear(ya); arb_clear(yb); arb_clear(hh);
}

/* ------------------------------------------------------------------ statuses, zero, aliasing, prec */
static void statuses(void)
{
    adf_rfun_t x, y, keep, zero; adf_rfun_struct before; acb_t z, zk; arb_t n, nk; const char *s;
    static const double P[2] = { 1, 2 };
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(keep); adf_rfun_init(zero);
    acb_init(z); acb_init(zk); arb_init(n); arb_init(nk);
    fun_simple(x, P, 2, 1, 0.5, 0.5, 0);
    /* prec above the maximum: LIMIT, outputs untouched */
    sentinel(y); before = *y; acb_set_si(z, 7); acb_set(zk, z); arb_set_si(n, 7); arb_set(nk, n);
    CHECK(adf_rfun_fourier(y, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_derivative(y, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_integral(z, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT && acb_equal(z, zk));
    CHECK(adf_rfun_norm2(n, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT && arb_equal(n, nk));
    /* prec 2, 0, -5 (as 2), 53, the maximum */
    CHECK(adf_rfun_fourier(y, x, 2) == ADF_OK && adf_rfun_is_canonical(y));
    CHECK(adf_rfun_fourier(y, x, 0) == ADF_OK && adf_rfun_is_canonical(y));
    CHECK(adf_rfun_fourier(y, x, -5) == ADF_OK && adf_rfun_is_canonical(y));
    CHECK(adf_rfun_derivative(y, x, 2) == ADF_OK && adf_rfun_is_canonical(y));
    CHECK(adf_rfun_integral(z, x, 2) == ADF_OK && acb_is_finite(z));
    CHECK(adf_rfun_norm2(n, x, 2) == ADF_OK && arb_is_finite(n));
    CHECK(adf_rfun_fourier(y, x, 53) == ADF_OK && adf_rfun_is_canonical(y));
    CHECK(adf_rfun_fourier(y, x, ADF_REAL_PREC_MAX) == ADF_OK);
    CHECK(arb_rel_accuracy_bits(acb_realref(y->term[0].C)) > ADF_REAL_PREC_MAX - 64);
    CHECK(arb_rel_accuracy_bits(acb_realref(y->term[0].P->coeffs + 1)) > ADF_REAL_PREC_MAX - 64);
    CHECK(adf_rfun_integral(z, x, ADF_REAL_PREC_MAX) == ADF_OK);
    CHECK(arb_rel_accuracy_bits(acb_realref(z)) > ADF_REAL_PREC_MAX - 64);
    /* Re(1/A) > 0 not certified at prec 2 but at prec 128: NOT_DETERMINED, y untouched (sentinel bytes) */
    s = "rfun(term(P=[(1) + (0)*i], A=(0.0001) + (100)*i, B=(1) + (0)*i, C=(0) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK);
    sentinel(y); before = *y; adf_rfun_set(keep, y);
    CHECK(adf_rfun_fourier(y, x, 2) == ADF_NOT_DETERMINED);
    CHECK(!memcmp(&before, y, sizeof(before)) && adf_rfun_identical(y, keep));
    CHECK(adf_rfun_fourier(y, x, PREC) == ADF_OK && adf_rfun_is_canonical(y));
    /* the same for a ball A whose inverse loses positivity at every prec (dependency in a^2 + b^2) */
    s = "rfun(term(P=[(1) + (0)*i], A=(1.25 +/- 1.2) + (0.3)*i, B=(0) + (0)*i, C=(0) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK);
    sentinel(y); before = *y;
    CHECK(adf_rfun_fourier(y, x, 2) == ADF_NOT_DETERMINED && !memcmp(&before, y, sizeof(before)));
    /* the zero polynomial term too: Re(1/A) is a predicate of every term */
    acb_poly_zero(x->term[0].P);
    CHECK(adf_rfun_fourier(y, x, 2) == ADF_NOT_DETERMINED && !memcmp(&before, y, sizeof(before)));
    /* a nonfinite integral: exp(10^300); z and n untouched */
    s = "rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(1e300) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK);
    acb_set(z, zk); arb_set(n, nk);
    CHECK(adf_rfun_integral(z, x, PREC) == ADF_NOT_DETERMINED && acb_equal(z, zk));
    CHECK(adf_rfun_norm2(n, x, PREC) == ADF_NOT_DETERMINED && arb_equal(n, nk));
    CHECK(adf_rfun_fourier(y, x, PREC) == ADF_OK);           /* parameters only: C' = C finite */
    /* the zero function: transform and derivative of length 0, integral and norm exactly 0 */
    CHECK(adf_rfun_fourier(y, zero, PREC) == ADF_OK && y->len == 0 && y->term == NULL);
    sentinel(y);
    CHECK(adf_rfun_derivative(y, zero, PREC) == ADF_OK && y->len == 0);
    CHECK(adf_rfun_integral(z, zero, PREC) == ADF_OK && acb_is_zero(z) && acb_is_exact(z));
    CHECK(adf_rfun_norm2(n, zero, PREC) == ADF_OK && arb_is_zero(n));
    /* a zero polynomial term is kept with its new parameters; it contributes the exact 0 */
    s = "rfun(term(P=[], A=(2) + (0)*i, B=(1) + (0)*i, C=(0) + (0)*i), term(P=[], A=(1) + (0)*i, "
        "B=(0) + (0)*i, C=(0) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK && x->len == 2);
    CHECK(adf_rfun_fourier(y, x, PREC) == ADF_OK && y->len == 2 && acb_poly_length(y->term[0].P) == 0);
    { acb_t h; acb_init(h); acb_set_d(h, 0.5); CHECK(acb_equal(y->term[0].A, h)); acb_clear(h); }
    CHECK(adf_rfun_derivative(y, x, PREC) == ADF_OK && y->len == 2 && acb_poly_length(y->term[1].P) == 0);
    CHECK(adf_rfun_integral(z, x, PREC) == ADF_OK && acb_is_zero(z));
    CHECK(adf_rfun_norm2(n, x, PREC) == ADF_OK && arb_is_zero(n));
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(keep); adf_rfun_clear(zero);
    acb_clear(z); acb_clear(zk); arb_clear(n); arb_clear(nk);
}

static void aliasing(void)
{
    adf_rfun_t x, y, w; const char *s;
    adf_rfun_init(x); adf_rfun_init(y); adf_rfun_init(w);
    s = "rfun(term(P=[(1 +/- 0.001) + (2)*i, (0) + (-1)*i, (0.5) + (0)*i], A=(1.5 +/- 0.01) + (-0.25)*i, "
        "B=(0.5) + (0.25)*i, C=(0) + (1)*i), term(P=[(3) + (0)*i], A=(0.75) + (2)*i, B=(-1) + (0)*i, "
        "C=(0.25) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK);
    CHECK(adf_rfun_fourier(w, x, PREC) == ADF_OK);
    adf_rfun_set(y, x); CHECK(adf_rfun_fourier(y, y, PREC) == ADF_OK && adf_rfun_identical(y, w));
    CHECK(adf_rfun_derivative(w, x, PREC) == ADF_OK);
    adf_rfun_set(y, x); CHECK(adf_rfun_derivative(y, y, PREC) == ADF_OK && adf_rfun_identical(y, w));
    /* failure with y = x leaves x untouched */
    s = "rfun(term(P=[(1) + (0)*i], A=(0.0001) + (100)*i, B=(1) + (0)*i, C=(0) + (0)*i))";
    CHECK(adf_rfun_set_str(x, s, strlen(s), PREC, NULL) == ADF_OK);
    adf_rfun_set(w, x);
    CHECK(adf_rfun_fourier(x, x, 2) == ADF_NOT_DETERMINED && adf_rfun_identical(x, w));
    adf_rfun_clear(x); adf_rfun_clear(y); adf_rfun_clear(w);
}

/* ------------------------------------------------------------------ the caps of D1 */
/* a raw rfun of n terms with P = [] and A = 1, canonical; built without set_terms */
static void raw_terms(adf_rfun_t x, slong n)
{
    slong i;
    adf_rfun_clear(x);
    x->term = terms_new(n); x->len = n;
    for (i = 0; i < n; i++) acb_one(x->term[i].A);
}
static void caps(void)
{
    adf_rfun_t x, y; adf_rfun_struct before; adf_rterm_struct *t; acb_t z, zk; arb_t n, nk;
    adf_rfun_init(x); adf_rfun_init(y); acb_init(z); acb_init(zk); arb_init(n); arb_init(nk);
    /* transform work: a term of length L charges 2 L^2 - L + 1; L = 724 within 2^20, L = 725 not */
    t = terms_new(1); acb_one(t->A);
    acb_poly_set_coeff_si(t->P, 724, 1);
    CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK);
    sentinel(y); before = *y;
    CHECK(adf_rfun_fourier(y, x, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    acb_poly_zero(t->P); acb_poly_set_coeff_si(t->P, 723, 1);
    CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK);
    CHECK(adf_rfun_fourier(y, x, 2) == ADF_OK && acb_poly_length(y->term[0].P) == 724);
    /* the exact boundary: 724 (1047629 units), one term of length 2 (7) and 470 of length 1 (2 each) charge
       exactly 2^20 and pass; 724 and 474 terms of length 1 charge 2^20 + 1 and do not */
    terms_free(t, 1); t = terms_new(476);
    {   slong i; for (i = 0; i < 476; i++) { acb_one(t[i].A); acb_poly_one(t[i].P); }
        acb_poly_set_coeff_si(t[0].P, 723, 1); acb_poly_set_coeff_si(t[1].P, 1, 1);
        CHECK(adf_rfun_set_terms(x, t, 472) == ADF_OK);
        CHECK(adf_rfun_fourier(y, x, 2) == ADF_OK && y->len == 472);
        acb_poly_one(t[1].P);
        CHECK(adf_rfun_set_terms(x, t, 475) == ADF_OK);
        sentinel(y); before = *y;
        CHECK(adf_rfun_fourier(y, x, 2) == ADF_LIMIT && !memcmp(&before, y, sizeof(before))); }
    terms_free(t, 476); t = terms_new(1);
    /* the work of several terms adds: two terms of length 512 (2 * 523777 <= 2^20), three not */
    terms_free(t, 1); t = terms_new(3);
    acb_one(t[0].A); acb_one(t[1].A); acb_one(t[2].A);
    acb_poly_set_coeff_si(t[0].P, 511, 1); acb_poly_set_coeff_si(t[1].P, 511, 1);
    acb_poly_set_coeff_si(t[2].P, 0, 1);
    CHECK(adf_rfun_set_terms(x, t, 2) == ADF_OK && adf_rfun_fourier(y, x, 2) == ADF_OK);
    acb_poly_set_coeff_si(t[2].P, 30, 1);
    CHECK(adf_rfun_set_terms(x, t, 3) == ADF_OK);
    sentinel(y); before = *y;
    CHECK(adf_rfun_fourier(y, x, 2) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    terms_free(t, 3);
    /* (mutation survivor 262:40 of slice 4d's add) a sum with exactly 2^16 coefficients passes, 2^16 + 1 not */
    {   adf_rfun_t u, s2; adf_rfun_init(u); adf_rfun_init(s2);
        t = terms_new(1); acb_one(t->A);
        acb_poly_set_coeff_si(t->P, ADF_RFUN_COEFFS_MAX / 2 - 1, 1);
        CHECK(adf_rfun_set_terms(u, t, 1) == ADF_OK);
        CHECK(adf_rfun_add(s2, u, u, PREC) == ADF_OK && s2->len == 2);
        acb_poly_set_coeff_si(t->P, ADF_RFUN_COEFFS_MAX / 2, 1);
        CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK);
        sentinel(y); before = *y;
        CHECK(adf_rfun_add(y, u, x, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
        terms_free(t, 1); adf_rfun_clear(u); adf_rfun_clear(s2); }
    /* the derivative of a nonzero term has one more coefficient: 2^16 - 1 passes, 2^16 does not */
    t = terms_new(1); acb_one(t->A);
    acb_poly_set_coeff_si(t->P, ADF_RFUN_COEFFS_MAX - 1, 1);
    CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK);
    sentinel(y); before = *y;
    CHECK(adf_rfun_derivative(y, x, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    acb_poly_zero(t->P); acb_poly_set_coeff_si(t->P, ADF_RFUN_COEFFS_MAX - 2, 1);
    CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK);
    CHECK(adf_rfun_derivative(y, x, 53) == ADF_OK && acb_poly_length(y->term[0].P) == ADF_RFUN_COEFFS_MAX);
    terms_free(t, 1);
    /* the norm is the integral of the product with the conjugate: 256 terms (65536 pairs) pass, 257 not */
    raw_terms(x, 257);
    { slong i; for (i = 0; i < 257; i++) acb_poly_one(x->term[i].P); }
    arb_set_si(n, 7); arb_set(nk, n);
    CHECK(adf_rfun_norm2(n, x, PREC) == ADF_LIMIT && arb_equal(n, nk));
    raw_terms(x, 256);
    { slong i; for (i = 0; i < 256; i++) acb_poly_one(x->term[i].P); }
    CHECK(adf_rfun_norm2(n, x, 53) == ADF_OK);
    {   arb_t w; arb_init(w); arb_set_ui(w, 2); arb_rsqrt(w, w, 53); arb_mul_ui(w, w, 65536, 53);
        CHECK(arb_overlaps(n, w)); arb_clear(w); }        /* 256 exp(-pi x^2): 65536 (2)^(-1/2) */
    /* the product work: one term of length 1024 (1024^2 = 2^20) passes, 1025 not */
    t = terms_new(1); acb_one(t->A);
    acb_poly_set_coeff_si(t->P, 1024, 1);
    CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK);
    arb_set(n, nk);
    CHECK(adf_rfun_norm2(n, x, PREC) == ADF_LIMIT && arb_equal(n, nk));
    acb_poly_zero(t->P); acb_poly_set_coeff_si(t->P, 1023, 1);
    CHECK(adf_rfun_set_terms(x, t, 1) == ADF_OK);
    CHECK(adf_rfun_norm2(n, x, 53) == ADF_OK && arb_is_positive(n));
    terms_free(t, 1);
    /* terms: 2^16 + 1 terms in an input (raw) give LIMIT before any work; 2^16 pass */
    raw_terms(x, ADF_RFUN_TERMS_MAX + 1);
    sentinel(y); before = *y; acb_set_si(z, 7); acb_set(zk, z); arb_set(n, nk);
    CHECK(adf_rfun_fourier(y, x, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_derivative(y, x, PREC) == ADF_LIMIT && !memcmp(&before, y, sizeof(before)));
    CHECK(adf_rfun_integral(z, x, PREC) == ADF_LIMIT && acb_equal(z, zk));
    CHECK(adf_rfun_norm2(n, x, PREC) == ADF_LIMIT && arb_equal(n, nk));
    raw_terms(x, ADF_RFUN_TERMS_MAX);
    CHECK(adf_rfun_fourier(y, x, 2) == ADF_OK && y->len == ADF_RFUN_TERMS_MAX);
    CHECK(adf_rfun_derivative(y, x, 2) == ADF_OK && y->len == ADF_RFUN_TERMS_MAX);
    CHECK(adf_rfun_integral(z, x, 2) == ADF_OK && acb_is_zero(z));
    adf_rfun_clear(x); adf_rfun_clear(y); acb_clear(z); acb_clear(zk); arb_clear(n); arb_clear(nk);
}

/* ------------------------------------------------------------------ INV: entry predicates */
static void debug(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int k;
    for (k = 0; k < 4; k++) {
        pid_t p = fork(); int status; CHECK(p >= 0);
        if (!p) {
            adf_rfun_t x, y; acb_t z; arb_t n; static const double P[1] = { 1 };
            adf_rfun_init(x); adf_rfun_init(y); acb_init(z); arb_init(n);
            fun_simple(x, P, 1, 1, 0, 0, 0);
            arb_neg(acb_realref(x->term[0].A), acb_realref(x->term[0].A));      /* Re(A) < 0 */
            if (k == 0) (void) adf_rfun_fourier(y, x, PREC);
            if (k == 1) (void) adf_rfun_derivative(y, x, PREC);
            if (k == 2) (void) adf_rfun_integral(z, x, PREC);
            if (k == 3) (void) adf_rfun_norm2(n, x, PREC);
            /* Reached only if the entry check is missing. The clears are for tools/memcheck. */
            adf_rfun_clear(x); adf_rfun_clear(y); acb_clear(z); arb_clear(n);
            _exit(0);
        }
        CHECK(waitpid(p, &status, 0) == p && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
    /* the prec cap is decided before the entry predicates */
    {   adf_rfun_t x, y; acb_t z; arb_t n; static const double P[1] = { 1 };
        adf_rfun_init(x); adf_rfun_init(y); acb_init(z); arb_init(n);
        fun_simple(x, P, 1, 1, 0, 0, 0);
        arb_neg(acb_realref(x->term[0].A), acb_realref(x->term[0].A));
        CHECK(adf_rfun_fourier(y, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        CHECK(adf_rfun_derivative(y, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        CHECK(adf_rfun_integral(z, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        CHECK(adf_rfun_norm2(n, x, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        arb_neg(acb_realref(x->term[0].A), acb_realref(x->term[0].A));
        adf_rfun_clear(x); adf_rfun_clear(y); acb_clear(z); arb_clear(n); }
#endif
}

int main(void)
{
    exact_cases();
    sign_test();
    vectors();
    covariance();
    statuses();
    aliasing();
    caps();
    debug();
    printf("rfun_fourier: %lu checks\n", checks);
    flint_cleanup();
    return 0;
}
