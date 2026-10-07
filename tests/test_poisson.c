/* tests/test_poisson.c: slice 4g of docs/api-4.md, Poisson summation with certified tails
   (include/adelefeld/tensor.h, adf_tensor_poisson; statements in docs/api-4c.md "Slice 4g").
   Contract: docs/api-4.md section 7 (statement P1 steps 1-5), section 1 (statuses, caps D1), section 8 (faults
   of 4.5), section 10 (the theta witness); analysis Proposition 7 and Lemma 6 (docs/proofs/analysis.md:213-294).
   Oracle: tests/ref/vectors/f4-slice6/poisson.jsonl written by lanes/f4-slice6/gen_vectors.py from
   proto/functions4_checks.py (poisson_sides_ball: both sums certified at tails below 2^-230, 60 digits;
   choose_cutoffs: the cutoffs for bits 20, 53, 80, 128; lattice_tail: the Lemma 6 bound per lattice and term at
   N = 0, 1, 2, 4, 8, 16). Every record is run at every bits.
   What OK must give, per record and bits (prec = bits + 64): left and right each contain the oracle's ball; every
   coordinate diameter <= 2^-bits (containment alone passes a function returning huge balls); left overlaps
   right; NL and NR equal the oracle's cutoffs (the promise of the code: the same search, with a certified upper
   bound of the same Lemma 6 value); the first N of 0, 1, 2, 4, 8, 16 whose planted tail E(N) <= 2^-bits/8 is the
   returned cutoff (the refusal of a tail above epsilon/8); each coordinate radius lies in [E, E (1 + 2^-40) +
   2^-(bits+20)] for the planted E of the returned cutoff (the tail is added to both coordinates, once, with the
   factor 2 of Lemma 6). */
#include <adelefeld.h>
#include <math.h>
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

#define EXP 1024
#define VEC "tests/ref/vectors/f4-slice6/poisson.jsonl"

static unsigned long checks, planted, radius_checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

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
static void q_of(fmpq_t q, const char *s)
{
    CHECK(fmpq_set_str(q, s, 10) == 0); fmpq_canonicalise(q);
}
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
static void ffun_json(adf_ffun_t f, const jsonl_value *v)
{
    ulong D = (ulong) number(field(v, "D")), M = (ulong) number(field(v, "M"));
    const jsonl_value *a = field(v, "f"); slong L = (slong) (D * M), j; acb_ptr w = _acb_vec_init(L);
    CHECK((slong) jsonl_size(a) == L);
    for (j = 0; j < L; j++) in_num(w + j, at(a, (size_t) j));
    CHECK(adf_ffun_set_acb_vec(f, D, M, w, L) == ADF_OK);
    _acb_vec_clear(w, L);
}
static void ref_ball(acb_t z, const jsonl_value *v)
{
    CHECK(arb_set_str(acb_realref(z), str(at(v, 0)), EXP) == 0);
    CHECK(arb_set_str(acb_imagref(z), str(at(v, 1)), EXP) == 0);
}

/* ------------------------------------------------------------------ building values */
/* One term P(x) exp(-pi A x^2 + B x + C), P with n real coefficients. */
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
/* The Gaussian c exp(-pi x^2) with the ball c as its constant polynomial. */
static void gauss_c(adf_rfun_t f, const arb_t c)
{
    adf_rterm_struct t;
    acb_poly_init(t.P); acb_init(t.A); acb_init(t.B); acb_init(t.C);
    { acb_t z; acb_init(z); acb_set_arb(z, c); acb_poly_set_coeff_acb(t.P, 0, z); acb_clear(z); }
    acb_one(t.A);
    CHECK(adf_rfun_set_terms(f, &t, 1) == ADF_OK);
    acb_poly_clear(t.P); acb_clear(t.A); acb_clear(t.B); acb_clear(t.C);
}
static void one11(adf_ffun_t f)
{
    acb_t v; acb_init(v); acb_one(v); CHECK(adf_ffun_set_acb_vec(f, 1, 1, v, 1) == ADF_OK); acb_clear(v);
}
static void fun_vals(adf_ffun_t f, ulong D, ulong M, const double *re)
{
    slong L = (slong) (D * M), j; acb_ptr v = _acb_vec_init(L);
    for (j = 0; j < L; j++) acb_set_d(v + j, re[j]);
    CHECK(adf_ffun_set_acb_vec(f, D, M, v, L) == ADF_OK);
    _acb_vec_clear(v, L);
}

/* ------------------------------------------------------------------ predicates of the result */
/* 1 iff every coordinate diameter is <= 2^-bits (and finite). */
static int narrow(const acb_t z, slong bits)
{
    return acb_is_finite(z) && mag_cmp_2exp_si(arb_radref(acb_realref(z)), -bits - 1) <= 0
        && mag_cmp_2exp_si(arb_radref(acb_imagref(z)), -bits - 1) <= 0;
}
/* The sentinel of the untouched checks: 12345/8 + (-7/16) i, cutoffs 777 and 778; byte copies kept. */
typedef struct { acb_t l, r; ulong nl, nr; unsigned char bl[sizeof(acb_struct)], br[sizeof(acb_struct)]; } sentinel;
static void sent_init(sentinel *s)
{
    acb_init(s->l); acb_init(s->r);
    arb_set_si(acb_realref(s->l), 12345); arb_mul_2exp_si(acb_realref(s->l), acb_realref(s->l), -3);
    arb_set_si(acb_imagref(s->l), -7); arb_mul_2exp_si(acb_imagref(s->l), acb_imagref(s->l), -4);
    acb_set(s->r, s->l); acb_neg(s->r, s->r);
    s->nl = 777; s->nr = 778;
    memcpy(s->bl, s->l, sizeof(acb_struct)); memcpy(s->br, s->r, sizeof(acb_struct));
}
static void sent_check(const sentinel *s)
{
    acb_t a; acb_init(a);
    CHECK(memcmp(s->bl, s->l, sizeof(acb_struct)) == 0 && memcmp(s->br, s->r, sizeof(acb_struct)) == 0);
    arb_set_si(acb_realref(a), 12345); arb_mul_2exp_si(acb_realref(a), acb_realref(a), -3);
    arb_set_si(acb_imagref(a), -7); arb_mul_2exp_si(acb_imagref(a), acb_imagref(a), -4);
    CHECK(acb_equal(s->l, a)); acb_neg(a, a); CHECK(acb_equal(s->r, a));
    CHECK(s->nl == 777 && s->nr == 778);
    acb_clear(a);
}
static int pois_sent(sentinel *s, const adf_rfun_t phi, const adf_ffun_t f, slong bits, slong prec)
{
    return adf_tensor_poisson(s->l, s->r, &s->nl, &s->nr, phi, f, bits, prec);
}
static void sent_clear(sentinel *s) { acb_clear(s->l); acb_clear(s->r); }

/* ------------------------------------------------------------------ the planted tails */
static const slong NS[6] = { 0, 1, 2, 4, 8, 16 };
/* E_L(N_i) = sum_j |f[j]| sum_terms B(j/D, M, N_i), E_R(N_i) = fmax sum_terms B(0, 1/M, N_i) from the record
   (oracle values at 20 digits; |f[j]| at EXP bits). */
static void planted_tails(arb_ptr EL, arb_ptr ER, const jsonl_value *v, const adf_ffun_t f)
{
    const jsonl_value *tl = field(v, "tl"), *tr = field(v, "tr"); size_t i, k; arb_t t, a, fm;
    arb_init(t); arb_init(a); arb_init(fm);
    for (k = 0; k < 6; k++) { arb_zero(EL + k); arb_zero(ER + k); }
    for (i = 0; i < jsonl_size(tl); i++) {
        const jsonl_value *e = at(tl, i); slong j = number(at(e, 0));
        acb_abs(a, f->f + j, EXP);
        for (k = 0; k < 6; k++) {
            CHECK(arb_set_str(t, str(at(at(e, 2), k)), EXP) == 0);
            arb_addmul(EL + k, a, t, EXP);
        }
    }
    CHECK(arb_set_str(fm, str(field(v, "fmax")), EXP) == 0);
    for (i = 0; i < jsonl_size(tr); i++)
        for (k = 0; k < 6; k++) {
            CHECK(arb_set_str(t, str(at(at(tr, i), k)), EXP) == 0);
            arb_addmul(ER + k, fm, t, EXP);
        }
    arb_clear(t); arb_clear(a); arb_clear(fm);
}
/* The index i of N_i among NS, or -1. */
static int ns_index(ulong n)
{
    int k; for (k = 0; k < 6; k++) if ((ulong) NS[k] == n) return k;
    return -1;
}
/* The cutoff checks against the planted tails E (six values) for the returned N: every N_i < N has E(N_i) not
   certainly below 2^-bits/8 (1 - 10^-15) (else the search should have stopped there), and E(N), if N is among
   NS, is not certainly above 2^-bits/8 (1 + 10^-15) (the refusal of a tail above epsilon/8; the oracle values
   have 20 digits). Then the radius window of the header comment on both coordinates of z. */
static void tail_checks(arb_srcptr E, ulong N, slong bits, const acb_t z)
{
    int k, idx = ns_index(N); arb_t g, lo, hi; mag_t up, m, w;
    arb_init(g); arb_init(lo); arb_init(hi); mag_init(up); mag_init(m); mag_init(w);
    arb_one(g); arb_mul_2exp_si(g, g, -bits - 3);
    arb_mul_ui(lo, g, 999999999999999ul, EXP); arb_div_ui(lo, lo, 1000000000000000ul, EXP);
    arb_mul_ui(hi, g, 1000000000000001ul, EXP); arb_div_ui(hi, hi, 1000000000000000ul, EXP);
    for (k = 0; k < 6 && NS[k] < (slong) N; k++) {
        CHECK(!arb_lt(E + k, lo));
        planted++;
    }
    if (idx >= 0) {
        CHECK(!arb_gt(E + idx, hi));
        planted++;
        arb_get_mag(up, E + idx);
        mag_mul_2exp_si(w, up, -40); mag_add(up, up, w);
        mag_one(w); mag_mul_2exp_si(w, w, -bits - 20); mag_add(up, up, w);
        arb_get_mag_lower(m, E + idx);
        mag_mul_2exp_si(w, m, -50); mag_sub_lower(m, m, w);
        CHECK(mag_cmp(arb_radref(acb_realref(z)), m) >= 0 && mag_cmp(arb_radref(acb_imagref(z)), m) >= 0);
        CHECK(mag_cmp(arb_radref(acb_realref(z)), up) <= 0 && mag_cmp(arb_radref(acb_imagref(z)), up) <= 0);
        radius_checks++;
    }
    arb_clear(g); arb_clear(lo); arb_clear(hi); mag_clear(up); mag_clear(m); mag_clear(w);
}

/* ------------------------------------------------------------------ the vectors */
static const slong BITS[4] = { 20, 53, 80, 128 };

static void vectors(void)
{
    jsonl_file *fl; jsonl_error_t e; size_t i; int b; unsigned long n = 0;
    CHECK(jsonl_open(VEC, &fl, &e));
    for (i = 0; i < jsonl_count(fl); i++) {
        const jsonl_value *v = jsonl_record(fl, i);
        adf_rfun_t phi; adf_ffun_t f; acb_t l, r, rl, rr; ulong NL, NR; arb_ptr EL, ER;
        if (strcmp(str(field(v, "k")), "pois") != 0) continue;
        adf_rfun_init(phi); adf_ffun_init(f); acb_init(l); acb_init(r); acb_init(rl); acb_init(rr);
        EL = _arb_vec_init(6); ER = _arb_vec_init(6);
        rfun_json(phi, field(v, "terms")); ffun_json(f, v);
        ref_ball(rl, field(v, "left")); ref_ball(rr, field(v, "right"));
        planted_tails(EL, ER, v, f);
        for (b = 0; b < 4; b++) {
            char key[8]; const jsonl_value *cut;
            snprintf(key, sizeof key, "%ld", (long) BITS[b]);
            cut = field(field(v, "cuts"), key);
            CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, BITS[b], BITS[b] + 64) == ADF_OK);
            if (!acb_contains(l, rl) || !acb_contains(r, rr))
                fprintf(stderr, "record %zu (%s), bits %ld\n", i, str(field(v, "phi")), (long) BITS[b]);
            CHECK(acb_contains(l, rl));
            CHECK(acb_contains(r, rr));
            CHECK(narrow(l, BITS[b]) && narrow(r, BITS[b]));
            CHECK(acb_overlaps(l, r));
            CHECK(NL == (ulong) number(at(cut, 0)));
            CHECK(NR == (ulong) number(at(cut, 1)));
            tail_checks(EL, NL, BITS[b], l);
            tail_checks(ER, NR, BITS[b], r);
            n++;
        }
        _arb_vec_clear(EL, 6); _arb_vec_clear(ER, 6);
        adf_rfun_clear(phi); adf_ffun_clear(f); acb_clear(l); acb_clear(r); acb_clear(rl); acb_clear(rr);
    }
    jsonl_close(fl);
    printf("vectors: %lu calls checked, %lu planted tail comparisons, %lu radius windows\n", n, planted,
           radius_checks);
}

/* ------------------------------------------------------------------ the right and left sums directly */
/* right against sum_(|n| <= NR) g[n mod L] phihat(n/M) with g = adf_ffun_fourier(f), phihat = adf_rfun_fourier
   (slices 4a, 4e), and left against sum_j f[j] sum_(|n| <= NL) phi(j/D + M n) by adf_rfun_eval: each direct
   partial sum lies in the returned ball (the returned ball is the partial sum plus the tail, at a precision at
   least prec), and the two midpoints differ by at most 2^-bits. */
static void direct(void)
{
    jsonl_file *fl; jsonl_error_t e; size_t i; unsigned long n = 0; slong bits = 53, prec = 200;
    CHECK(jsonl_open(VEC, &fl, &e));
    for (i = 0; i < jsonl_count(fl); i++) {
        const jsonl_value *v = jsonl_record(fl, i);
        adf_rfun_t phi, hat; adf_ffun_t f, g; acb_t l, r, s, t, w; arb_t x; fmpq_t q; ulong NL, NR, L; slong k, j;
        if (strcmp(str(field(v, "k")), "pois") != 0) continue;
        adf_rfun_init(phi); adf_rfun_init(hat); adf_ffun_init(f); adf_ffun_init(g);
        acb_init(l); acb_init(r); acb_init(s); acb_init(t); acb_init(w); arb_init(x); fmpq_init(q);
        rfun_json(phi, field(v, "terms")); ffun_json(f, v);
        CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, bits, prec) == ADF_OK);
        CHECK(adf_ffun_fourier(g, f, prec) == ADF_OK);
        CHECK(adf_rfun_fourier(hat, phi, prec) == ADF_OK);
        L = f->D * f->M;
        CHECK(g->D == f->M && g->M == f->D);
        for (k = -(slong) NR; k <= (slong) NR; k++) {
            fmpq_set_si(q, k, f->M); arb_set_fmpq(x, q, prec);
            CHECK(adf_rfun_eval(t, hat, x, prec) == ADF_OK);
            acb_addmul(s, g->f + (((k % (slong) L) + (slong) L) % (slong) L), t, prec);
        }
        CHECK(acb_contains(r, s) || acb_overlaps(r, s));
        acb_sub(w, r, s, prec);
        CHECK(arf_cmpabs_2exp_si(arb_midref(acb_realref(w)), -bits) <= 0);
        CHECK(arf_cmpabs_2exp_si(arb_midref(acb_imagref(w)), -bits) <= 0);
        acb_zero(s);
        for (j = 0; j < (slong) L; j++)
            for (k = -(slong) NL; k <= (slong) NL; k++) {
                fmpq_set_si(q, j + (slong) L * k, f->D); arb_set_fmpq(x, q, prec);
                CHECK(adf_rfun_eval(t, phi, x, prec) == ADF_OK);
                acb_addmul(s, f->f + j, t, prec);
            }
        CHECK(acb_overlaps(l, s));
        acb_sub(w, l, s, prec);
        CHECK(arf_cmpabs_2exp_si(arb_midref(acb_realref(w)), -bits) <= 0);
        CHECK(arf_cmpabs_2exp_si(arb_midref(acb_imagref(w)), -bits) <= 0);
        n++;
        adf_rfun_clear(phi); adf_rfun_clear(hat); adf_ffun_clear(f); adf_ffun_clear(g);
        acb_clear(l); acb_clear(r); acb_clear(s); acb_clear(t); acb_clear(w); arb_clear(x); fmpq_clear(q);
    }
    jsonl_close(fl);
    printf("direct: %lu records, right and left against direct partial sums\n", n);
}

/* ------------------------------------------------------------------ theta, the witness, retries */
static void theta_ref(arb_t th)
{
    jsonl_file *fl; jsonl_error_t e; size_t i;
    CHECK(jsonl_open(VEC, &fl, &e));
    for (i = 0; i < jsonl_count(fl); i++) {
        const jsonl_value *v = jsonl_record(fl, i);
        if (strcmp(str(field(v, "k")), "theta") == 0) CHECK(arb_set_str(th, str(field(v, "v")), EXP) == 0);
    }
    jsonl_close(fl);
    arb_add_error_2exp_si(th, -195);                     /* 60 digits */
}

/* E(k) = B_phi(0, 1, NS[k]) of the Gaussian at (D, M) = (1, 1), f = [1] (the first record of the vectors). */
static void gauss_tail(arb_t E, int k)
{
    jsonl_file *fl; jsonl_error_t e; const jsonl_value *v;
    CHECK(jsonl_open(VEC, &fl, &e));
    v = jsonl_record(fl, 0);
    CHECK(strcmp(str(field(v, "phi")), "gauss") == 0 && number(field(v, "D")) == 1 && number(field(v, "M")) == 1);
    CHECK(arb_set_str(E, str(at(at(at(field(v, "tl"), 0), 2), (size_t) k)), 300) == 0);
    jsonl_close(fl);
}

static void theta(void)
{
    adf_rfun_t phi; adf_ffun_t f; acb_t l, r, th; arb_t t; ulong NL, NR; int b;
    static const ulong cut[4] = { 2, 4, 4, 8 };          /* choose_cutoffs of the oracle, gauss at (1, 1) */
    adf_rfun_init(phi); adf_ffun_init(f); acb_init(l); acb_init(r); acb_init(th); arb_init(t);
    gauss(phi); one11(f); theta_ref(acb_realref(th));
    CHECK(arf_cmp_si(arb_midref(acb_realref(th)), 1) > 0);
    for (b = 0; b < 4; b++) {
        CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, BITS[b], 144) == ADF_OK);
        CHECK(acb_contains(l, th) && acb_contains(r, th));
        CHECK(narrow(l, BITS[b]) && narrow(r, BITS[b]));
        CHECK(NL == cut[b] && NR == cut[b]);
    }
    /* n = 0 included: the theta value is above 1 + 2 exp(-pi) - 10^-3 */
    arb_set_d(t, 1.0855);
    CHECK(arb_gt(acb_realref(l), t));
    /* bits = 0: epsilon = 1, goal 1/8; E(0) = B(0, 1, 0) = 0.0864 of the oracle passes: N = 0, the term n = 0
       alone, 1 +/- 0.0864 (both coordinates), which contains theta */
    CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, 0, 144) == ADF_OK);
    CHECK(acb_contains(l, th) && acb_contains(r, th) && narrow(l, 0) && NL == 0 && NR == 0);
    arb_set_d(t, 0.0864);
    CHECK(mag_cmp_2exp_si(arb_radref(acb_imagref(l)), -3) <= 0 && arb_contains_arf(acb_imagref(l), arb_midref(t)));
    /* precision retry: prec 20 cannot give 2^-53 (one rounding of 1.08 at 20 bits is 2^-20), the retries can */
    CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, 53, 20) == ADF_OK);
    CHECK(acb_contains(l, th) && acb_contains(r, th) && narrow(l, 53) && narrow(r, 53));
    CHECK(NL == 4 && NR == 4);
    /* bits 100 from prec 20: the retries must pass 64 and 128 bits (20, 40, 80, 160) */
    CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, 100, 20) == ADF_OK);
    CHECK(acb_contains(l, th) && acb_contains(r, th) && narrow(l, 100) && narrow(r, 100));
    CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, 30, 2) == ADF_OK);
    CHECK(acb_contains(l, th) && narrow(l, 30) && narrow(r, 30));
    /* the design's example: --bits 80 --prec 144 */
    CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, 80, 144) == ADF_OK);
    CHECK(narrow(l, 80) && NL == 4 && NR == 4);
    adf_rfun_clear(phi); adf_ffun_clear(f); acb_clear(l); acb_clear(r); acb_clear(th); arb_clear(t);
    printf("theta: contains sum exp(-pi n^2) at bits 0, 20, 30, 53, 80, 128; retries from prec 2 and 20\n");
}

static double seconds(void)
{
    return (double) clock() / (double) CLOCKS_PER_SEC;                    /* processor time */
}

/* The witness of api-4.md section 10: c in [1, 2]; then c in [1, 1 + 2^-40] (OK: the input radius is carried);
   c in [1, 1 + 2^-10] at bits 20 (NOT_DETERMINED); and the width rule after the tail (below). */
static void witness(void)
{
    adf_rfun_t phi, g; adf_ffun_t f; sentinel s; arb_t c, th; acb_t l, r, lo; ulong NL, NR;
    slong bits[3] = { 0, 10, 53 }; int k;
    adf_rfun_init(phi); adf_rfun_init(g); adf_ffun_init(f); arb_init(c); arb_init(th); acb_init(l); acb_init(r);
    acb_init(lo); one11(f); theta_ref(th);
    arb_set_d(c, 1.5); mag_set_d(arb_radref(c), 0.5);
    gauss_c(phi, c);
    for (k = 0; k < 3; k++) {
        double t0 = seconds();
        sent_init(&s);
        CHECK(pois_sent(&s, phi, f, bits[k], 144) == ADF_NOT_DETERMINED);
        sent_check(&s); sent_clear(&s);
        /* the stall rule: one doubling 144 -> 288 that does not halve the width stops the retries (the doublings
           to 2^21 bits would take seconds) */
        CHECK(seconds() - t0 < 1.0);
    }
    sent_init(&s);
    CHECK(pois_sent(&s, phi, f, 20, 20) == ADF_NOT_DETERMINED);
    sent_check(&s); sent_clear(&s);
    /* c in [1, 1 + 2^-40]: OK at bits 20, both theta and (1 + 2^-40) theta inside */
    arb_one(c); arb_mul_2exp_si(c, c, -41); arb_add_ui(c, c, 1, 200); mag_set_ui_2exp_si(arb_radref(c), 1, -41);
    gauss_c(g, c);
    CHECK(adf_tensor_poisson(l, r, &NL, &NR, g, f, 20, 144) == ADF_OK);
    acb_set_arb(lo, th); CHECK(acb_contains(l, lo) && acb_contains(r, lo));
    arb_mul_2exp_si(acb_realref(lo), th, -40); arb_add(acb_realref(lo), acb_realref(lo), th, 300);
    CHECK(acb_contains(l, lo) && acb_contains(r, lo));
    CHECK(narrow(l, 20) && narrow(r, 20));
    /* c in [1, 1 + 2^-10] at bits 20: NOT_DETERMINED */
    arb_one(c); arb_mul_2exp_si(c, c, -11); arb_add_ui(c, c, 1, 200); mag_set_ui_2exp_si(arb_radref(c), 1, -11);
    gauss_c(g, c);
    sent_init(&s);
    CHECK(pois_sent(&s, g, f, 20, 144) == ADF_NOT_DETERMINED);
    sent_check(&s); sent_clear(&s);
    {   /* the width rule after the tail (bits 13: N = 1 on both sides, E = E_L(1) = E_R(1) = 6.97e-6 of the oracle,
           lattice_tail([1], 1, 0, 0; 0, 1, 1), the first record of the vectors); c = 1 + d/2 +/- d/2 with
           d S_1 = 2^-13 - E/2, S_1 = sum_(|n| <= 1) exp(-pi n^2): the diameter before the tail is 2^-13 - E/2,
           after it 2^-13 + 3E/2; with d/2 the diameter after the tail is below 2^-13 */
        arb_t S2, E, d, t; slong n;
        arb_init(S2); arb_init(E); arb_init(d); arb_init(t);
        for (n = -1; n <= 1; n++) {
            arb_const_pi(t, 300); arb_mul_si(t, t, -n * n, 300); arb_exp(t, t, 300); arb_add(S2, S2, t, 300);
        }
        gauss_tail(E, 1);
        arb_mul_2exp_si(t, E, -1); arb_one(d); arb_mul_2exp_si(d, d, -13); arb_sub(d, d, t, 300);
        arb_div(d, d, S2, 300);
        for (n = 0; n < 2; n++) {
            arb_set_arf(t, arb_midref(d)); arb_mul_2exp_si(t, t, -1 - n);           /* rad = d/2, then d/4 */
            arf_add_ui(arb_midref(c), arb_midref(t), 1, ARF_PREC_EXACT, ARF_RND_DOWN);
            arf_get_mag(arb_radref(c), arb_midref(t));
            gauss_c(g, c);
            if (n == 0) {
                sent_init(&s);
                CHECK(pois_sent(&s, g, f, 13, 144) == ADF_NOT_DETERMINED);
                sent_check(&s); sent_clear(&s);
            } else {
                CHECK(adf_tensor_poisson(l, r, &NL, &NR, g, f, 13, 144) == ADF_OK);
                CHECK(NL == 1 && NR == 1 && narrow(l, 13) && narrow(r, 13));
            }
        }
        arb_clear(S2); arb_clear(E); arb_clear(d); arb_clear(t);
    }
    adf_rfun_clear(phi); adf_rfun_clear(g); adf_ffun_clear(f); arb_clear(c); arb_clear(th); acb_clear(l);
    acb_clear(r); acb_clear(lo);
    printf("witness: NOT_DETERMINED with sentinels untouched; input radii carried; width after the tail\n");
}

/* Three precisions (design section 8, "three-precision ball tests"): the oracle's check_precision tensor shape,
   (2, 3), f = [1, 2, 0, -1, 3, 1], the shifted Gaussian (1 + x/5) exp(-pi x^2) translated: here B = 7/10.
   The diameters decrease with (prec, bits) = (80, 30), (144, 70), (212, 110). */
static void precisions(void)
{
    static const double fv[6] = { 1, 2, 0, -1, 3, 1 }, P[2] = { 1, 0.2 };
    static const slong pr[3] = { 80, 144, 212 }, bt[3] = { 30, 70, 110 };
    adf_rfun_t phi; adf_ffun_t f; acb_t l, r; ulong NL, NR; int k; mag_t prev, m;
    adf_rfun_init(phi); adf_ffun_init(f); acb_init(l); acb_init(r); mag_init(prev); mag_init(m);
    fun_simple(phi, P, 2, 1, 0, 0.7, 0); fun_vals(f, 2, 3, fv);
    for (k = 0; k < 3; k++) {
        CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, bt[k], pr[k]) == ADF_OK);
        CHECK(narrow(l, bt[k]) && narrow(r, bt[k]) && acb_overlaps(l, r));
        mag_max(m, arb_radref(acb_realref(l)), arb_radref(acb_realref(r)));
        if (k) CHECK(mag_cmp(m, prev) < 0);
        mag_set(prev, m);
    }
    adf_rfun_clear(phi); adf_ffun_clear(f); acb_clear(l); acb_clear(r); mag_clear(prev); mag_clear(m);
}

/* ------------------------------------------------------------------ statuses */

static void statuses(void)
{
    adf_rfun_t phi, zero, slow; adf_ffun_t f, big, z6; sentinel s; acb_t l, r; ulong NL, NR; double t0, t1;
    static const double P[1] = { 1 }, zv[6] = { 0, 0, 0, 0, 0, 0 };
    adf_rfun_init(phi); adf_rfun_init(zero); adf_rfun_init(slow); adf_ffun_init(f); adf_ffun_init(big);
    adf_ffun_init(z6); acb_init(l); acb_init(r);
    gauss(phi); one11(f);
    /* bits outside [0, 2^21]: DOMAIN, after the prec cap */
    sent_init(&s); CHECK(pois_sent(&s, phi, f, -1, 64) == ADF_DOMAIN); sent_check(&s); sent_clear(&s);
    sent_init(&s); CHECK(pois_sent(&s, phi, f, 2097153, 64) == ADF_DOMAIN); sent_check(&s); sent_clear(&s);
    sent_init(&s); CHECK(pois_sent(&s, phi, f, WORD_MIN, 64) == ADF_DOMAIN); sent_check(&s); sent_clear(&s);
    sent_init(&s); CHECK(pois_sent(&s, phi, f, -1, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT); sent_check(&s);
    sent_clear(&s);
    sent_init(&s); CHECK(pois_sent(&s, phi, f, 20, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT); sent_check(&s);
    sent_clear(&s);
    /* bits = 2^21 is admitted: the zero function (len 0) gives exact zeros and cutoffs 0 at once */
    CHECK(adf_tensor_poisson(l, r, &NL, &NR, zero, f, 2097152, 64) == ADF_OK);
    CHECK(acb_is_zero(l) && acb_is_zero(r) && NL == 0 && NR == 0);
    /* f = 0: left exactly 0 and NL = 0; g = 0: right exactly 0 and NR = 0 */
    fun_vals(z6, 2, 3, zv);
    CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, z6, 128, 64) == ADF_OK);
    CHECK(acb_is_zero(l) && acb_is_zero(r) && NL == 0 && NR == 0);
    /* D1 work cap: Re(A) = 2^-60 and Re(A) = 10^-8 (design section 8: hits the prefix budget): LIMIT, < 10 s */
    t0 = seconds();
    fun_simple(slow, P, 1, ldexp(1.0, -60), 0, 0, 0);
    sent_init(&s); CHECK(pois_sent(&s, slow, f, 20, 64) == ADF_LIMIT); sent_check(&s); sent_clear(&s);
    fun_simple(slow, P, 1, 1e-8, 0, 0, 0);
    sent_init(&s); CHECK(pois_sent(&s, slow, f, 20, 64) == ADF_LIMIT); sent_check(&s); sent_clear(&s);
    t1 = seconds();
    printf("work cap: two LIMIT calls in %.3f s\n", t1 - t0);
    /* the preflight of the Lemma 6 prefix refuses at once; iterating to the cap would take about a second */
    CHECK(t1 - t0 < 0.25);
    /* Re(A) = 1/1000: about 120 prefix iterations, OK */
    fun_simple(slow, P, 1, 1e-3, 0, 0, 0);
    CHECK(adf_tensor_poisson(l, r, &NL, &NR, slow, f, 10, 64) == ADF_OK);
    CHECK(narrow(l, 10) && narrow(r, 10) && acb_overlaps(l, r));
    /* g[0] = 0 and |g[1]| = 1 (f = [1, -1] at (1, 2)): the right tail uses the largest |g[k]|, not |g[0]| */
    {
        static const double pm[2] = { 1, -1 };
        adf_ffun_t h; adf_ffun_init(h); fun_vals(h, 1, 2, pm);
        CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, h, 30, 94) == ADF_OK);
        CHECK(acb_overlaps(l, r) && narrow(l, 30) && narrow(r, 30) && NR >= 1);
        CHECK(arf_cmp_si(arb_midref(acb_realref(r)), 0) > 0 && !arb_contains_zero(acb_realref(r)));
        adf_ffun_clear(h);
    }
    /* a zero polynomial term after a nonzero one changes nothing (its bound is 0, its value is 0) */
    {
        adf_rfun_t two; adf_rterm_struct t[2]; acb_t l2, r2; ulong n1, n2; int k;
        for (k = 0; k < 2; k++) {
            acb_poly_init(t[k].P); acb_init(t[k].A); acb_init(t[k].B); acb_init(t[k].C); acb_one(t[k].A);
        }
        acb_poly_one(t[0].P);
        adf_rfun_init(two); acb_init(l2); acb_init(r2);
        CHECK(adf_rfun_set_terms(two, t, 2) == ADF_OK);
        CHECK(adf_tensor_poisson(l, r, &NL, &NR, phi, f, 53, 117) == ADF_OK);
        CHECK(adf_tensor_poisson(l2, r2, &n1, &n2, two, f, 53, 117) == ADF_OK);
        CHECK(acb_equal(l, l2) && acb_equal(r, r2) && n1 == NL && n2 == NR);
        for (k = 0; k < 2; k++) {
            acb_poly_clear(t[k].P); acb_clear(t[k].A); acb_clear(t[k].B); acb_clear(t[k].C);
        }
        adf_rfun_clear(two); acb_clear(l2); acb_clear(r2);
    }
    /* the rfun caps of D1 at their boundary: 2^16 terms 2^-20 exp(-pi x^2) (2^16 coefficients) pass, with the work
       2 2^16 (transform) + 2 * 3 2^16 (tails at N = 0) + 2 * 2 2^16 (evaluations) + 1 < 2^20, and give theta/16
       with N = 0 at bits 0 (E(0) = 2^16 2^-20 0.0864 = 0.0054 <= 1/8); 2^16 + 1 zero terms are LIMIT */
    {
        slong n = ADF_RFUN_TERMS_MAX, k; adf_rterm_struct *t = flint_malloc((size_t) (n + 1) * sizeof(*t));
        adf_rfun_t many; arb_t th;
        for (k = 0; k <= n; k++) {
            acb_poly_init(t[k].P); acb_init(t[k].A); acb_init(t[k].B); acb_init(t[k].C); acb_one(t[k].A);
            if (k < n) {
                acb_poly_set_coeff_si(t[k].P, 0, 1); acb_mul_2exp_si(t[k].P->coeffs, t[k].P->coeffs, -20);
            }
        }
        adf_rfun_init(many); arb_init(th);
        CHECK(adf_rfun_set_terms(many, t, n) == ADF_OK);
        CHECK(adf_tensor_poisson(l, r, &NL, &NR, many, f, 0, 64) == ADF_OK);
        theta_ref(th); arb_mul_2exp_si(th, th, -4);
        CHECK(NL == 0 && NR == 0 && arb_contains(acb_realref(l), th) && arb_contains(acb_realref(r), th));
        for (k = 0; k <= n; k++) {
            acb_poly_clear(t[k].P); acb_clear(t[k].A); acb_clear(t[k].B); acb_clear(t[k].C);
        }
        flint_free(t); adf_rfun_clear(many); adf_rfun_init(many);
        many->len = n + 1;                                /* zero polynomials, forged length (as test_tensor) */
        many->term = (adf_rterm_struct *) flint_malloc((size_t) many->len * sizeof(adf_rterm_struct));
        for (k = 0; k <= n; k++) {
            acb_poly_init(many->term[k].P); acb_init(many->term[k].A); acb_one(many->term[k].A);
            acb_init(many->term[k].B); acb_init(many->term[k].C);
        }
        sent_init(&s); CHECK(pois_sent(&s, many, f, 0, 64) == ADF_LIMIT); sent_check(&s); sent_clear(&s);
        /* the size caps come before the bits check (adf_rfun_fourier would refuse the terms later as well) */
        sent_init(&s); CHECK(pois_sent(&s, many, f, -1, 64) == ADF_LIMIT); sent_check(&s); sent_clear(&s);
        adf_rfun_clear(many); arb_clear(th);
    }
    /* the size caps: L = 1025, L^2 > 2^20: LIMIT */
    {
        acb_ptr v = _acb_vec_init(1025);
        CHECK(adf_ffun_set_acb_vec(big, 1025, 1, v, 1025) == ADF_OK);
        _acb_vec_clear(v, 1025);
    }
    sent_init(&s); CHECK(pois_sent(&s, phi, big, 20, 64) == ADF_LIMIT); sent_check(&s); sent_clear(&s);
    sent_init(&s); CHECK(pois_sent(&s, phi, big, -1, 64) == ADF_LIMIT); sent_check(&s); sent_clear(&s);
    adf_rfun_clear(phi); adf_rfun_clear(zero); adf_rfun_clear(slow); adf_ffun_clear(f); adf_ffun_clear(big);
    adf_ffun_clear(z6); acb_clear(l); acb_clear(r);
}

/* ------------------------------------------------------------------ INV: preconditions */
static void debug(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int k;
    for (k = 0; k < 6; k++) {
        pid_t p = fork(); int status; CHECK(p >= 0);
        if (!p) {
            adf_rfun_t phi; adf_ffun_t f; acb_t l, r; ulong a = 0, b = 0;
            adf_rfun_init(phi); adf_ffun_init(f); acb_init(l); acb_init(r); gauss(phi); one11(f);
            if (k == 0) (void) adf_tensor_poisson(l, l, &a, &b, phi, f, 20, 64);       /* left == right */
            if (k == 1) (void) adf_tensor_poisson(l, r, &a, &a, phi, f, 20, 64);       /* NL == NR */
            if (k == 2) (void) adf_tensor_poisson(l, r, NULL, &b, phi, f, 20, 64);
            if (k == 3) (void) adf_tensor_poisson(l, r, &a, NULL, phi, f, 20, 64);
            if (k == 4) {
                arb_neg(acb_realref(phi->term[0].A), acb_realref(phi->term[0].A));
                (void) adf_tensor_poisson(l, r, &a, &b, phi, f, 20, 64);
            }
            if (k == 5) { acb_indeterminate(f->f); (void) adf_tensor_poisson(l, r, &a, &b, phi, f, 20, 64); }
            /* Reached only if the entry check is missing. The clears are for tools/memcheck. */
            adf_rfun_clear(phi); adf_ffun_clear(f); acb_clear(l); acb_clear(r);
            _exit(0);
        }
        CHECK(waitpid(p, &status, 0) == p && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
    {   /* the prec cap is decided before the entry predicates */
        adf_rfun_t phi; adf_ffun_t f; acb_t l; ulong a = 0;
        adf_rfun_init(phi); adf_ffun_init(f); acb_init(l); gauss(phi); one11(f);
        acb_indeterminate(f->f);
        CHECK(adf_tensor_poisson(l, l, &a, &a, phi, f, 20, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        acb_zero(f->f);
        adf_rfun_clear(phi); adf_ffun_clear(f); acb_clear(l);
    }
    printf("INV: 6 precondition aborts\n");
#endif
}

int main(void)
{
    vectors();
    direct();
    theta();
    witness();
    precisions();
    statuses();
    debug();
    printf("poisson: %lu checks\n", checks);
    flint_cleanup();
    return 0;
}
