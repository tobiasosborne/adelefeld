/* Slice 3.1-f: adf_qclass_neg and adf_qclass_add (docs/api-3.md 2.4, Q1, Q3; section 5 row "add, neg").
   Vectors: tests/ref/vectors/q-slice7/arith.jsonl, from lanes/q-slice7/gen_vectors.py and the oracle
   proto/quotient3_checks.py (add, neg, reduce, round_piece). The finite sum is (a+b) + gcd(N,M) Zhat by
   docs/proofs/precision.md:25-30. Membership below solves the rational translation condition of
   quotient.md P7:107-110 exactly, independently of algorithm R; the containment checks of
   tests/test_qclass_reduce.c are reused. Exact set relations use adf_qclass_contains (Q2). */
#include <adelefeld.h>
#include "support/jsonl.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
/* Compile the same source here (as tests/test_qclass_reduce.c does with src/qclass.c) to check the
   bounded helpers at their boundaries, which the public calls cannot reach separately: an input at
   such a boundary is refused later in R either way. All public calls below use this same source. */
#include "../src/qclass_arith.c"

static ulong checks, contains_ok, contains_limit;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "arith line %d: %s\n", __LINE__, #c); abort(); } } while (0)

static const jsonl_value *field(const jsonl_value *v, const char *k)
{
    const jsonl_value *r; jsonl_error_t e;
    CHECK(jsonl_field(v, k, &r, &e)); return r;
}
static const jsonl_value *at(const jsonl_value *v, size_t i)
{
    jsonl_error_t e; const jsonl_value *r = jsonl_at(v, i, &e); CHECK(r); return r;
}
static const char *str(const jsonl_value *v)
{
    size_t n; jsonl_error_t e; const char *s = jsonl_string(v, &n, &e);
    CHECK(s && strlen(s) == n); return s;
}
static slong integer(const jsonl_value *v)
{
    jsonl_error_t e; const char *s = jsonl_int_text(v, &e); CHECK(s); return atol(s);
}
static int boolean(const jsonl_value *v)
{
    jsonl_error_t e; int b; CHECK(jsonl_bool(v, &b, &e)); return b;
}
static void rat(fmpq_t q, const jsonl_value *v) { CHECK(!fmpq_set_str(q, str(v), 10)); }
static void ends(fmpq_t lo, fmpq_t hi, arb_srcptr x)
{
    arf_t r; fmpq_t q;
    arf_init(r); fmpq_init(q); arf_get_fmpq(q, arb_midref(x));
    arf_set_mag(r, arb_radref(x)); arf_get_fmpq(hi, r);
    fmpq_sub(lo, q, hi); fmpq_add(hi, q, hi); fmpq_clear(q); arf_clear(r);
}
/* An exact dyadic radius of at most 30 bits, stored as a mag without rounding. */
static void set_mag_exact(mag_t m, const fmpq_t q)
{
    arf_t r; fmpq_t t;
    arf_init(r); fmpq_init(t); arf_set_fmpq(r, q, ARF_PREC_EXACT, ARF_RND_NEAR);
    if (arf_is_zero(r)) mag_zero(m);
    else {
        fmpz_t mant, exp; fmpz_init(mant); fmpz_init(exp);
        arf_get_fmpz_2exp(mant, exp, r);
        fmpz_mul_2exp(mant, mant, 30-fmpz_bits(mant));
        MAG_MAN(m) = fmpz_get_ui(mant); fmpz_set(MAG_EXPREF(m), ARF_EXPREF(r));
        fmpz_clear(mant); fmpz_clear(exp);
    }
    arf_set_mag(r, m); arf_get_fmpq(t, r); CHECK(fmpq_equal(t, q));
    arf_clear(r); fmpq_clear(t);
}
static void input_piece(adf_adele_struct *x, const jsonl_value *v)
{
    fmpq_t q; adf_rat_t a, N;
    fmpq_init(q); adf_rat_init(a); adf_rat_init(N);
    rat(q, field(v, "mid")); arf_set_fmpq(arb_midref(x->inf), q, ARF_PREC_EXACT, ARF_RND_NEAR);
    { fmpq_t t; fmpq_init(t); arf_get_fmpq(t, arb_midref(x->inf)); CHECK(fmpq_equal(t, q)); fmpq_clear(t); }
    rat(q, field(v, "rad")); set_mag_exact(arb_radref(x->inf), q);
    rat(a->q, field(v, "a")); rat(N->q, field(v, "N"));
    CHECK(adf_fball_set_center_radius(&x->fin, a, N) == ADF_OK);
    fmpq_clear(q); adf_rat_clear(a); adf_rat_clear(N);
}
static void input(adf_qclass_t x, const jsonl_value *v)
{
    const jsonl_value *ps = field(v, "pieces"); slong i;
    adf_qclass_clear(x);
    x->form = strcmp(str(field(v, "form")), "lift") ? ADF_QCLASS_PIECES : ADF_QCLASS_LIFT;
    x->len = (slong) jsonl_size(ps);
    x->piece = flint_malloc((size_t) x->len*sizeof(*x->piece));
    for (i = 0; i < x->len; i++) { adf_adele_init(x->piece+i); input_piece(x->piece+i, at(ps, (size_t) i)); }
    CHECK(adf_qclass_is_canonical(x));
}
/* (s,w) in pi(x) iff some rational q has s+q in [lo,hi] and w+q in a+N Zhat; N Zhat cap Q = N Z
   (precision.md Lemma 1), so q = a-w+kN with k an integer, or q = a-w for N = 0. */
static int member(const adf_adele_struct *x, const fmpq_t s, const fmpq_t w)
{
    fmpq_t lo, hi, t; adf_rat_t a, N; fmpz_t l, h; int yes;
    fmpq_init(lo); fmpq_init(hi); fmpq_init(t); adf_rat_init(a); adf_rat_init(N);
    fmpz_init(l); fmpz_init(h); ends(lo, hi, x->inf);
    adf_fball_get_center(a, &x->fin); adf_fball_get_radius(N, &x->fin);
    fmpq_sub(t, w, s); fmpq_sub(t, t, a->q); fmpq_add(lo, lo, t); fmpq_add(hi, hi, t);
    if (fmpq_is_zero(N->q)) yes = fmpq_sgn(lo) <= 0 && fmpq_sgn(hi) >= 0;
    else {
        fmpq_div(lo, lo, N->q); fmpq_div(hi, hi, N->q);
        fmpz_cdiv_q(l, fmpq_numref(lo), fmpq_denref(lo));
        fmpz_fdiv_q(h, fmpq_numref(hi), fmpq_denref(hi)); yes = fmpz_cmp(l, h) <= 0;
    }
    fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(t); adf_rat_clear(a); adf_rat_clear(N);
    fmpz_clear(l); fmpz_clear(h); return yes;
}
static int union_member(const adf_qclass_t x, const fmpq_t s, const fmpq_t w)
{
    slong i; for (i = 0; i < x->len; i++) if (member(x->piece+i, s, w)) return 1;
    return 0;
}
/* Stored rounded list equal to the oracle's (pins Q1's kernel and the count after deduplication);
   every exact piece enclosed by a stored piece of the same finite part with rho-d <= 2^-28 d. */
static void validate(const adf_qclass_t y, const jsonl_value *v)
{
    const jsonl_value *ps = field(v, "rounded"), *ex = field(v, "exact");
    fmpq_t lo, hi, l, h, m, d, t, rho, bound;
    fmpz_t A, H, D; arf_t r; size_t i, j;
    fmpq_init(lo); fmpq_init(hi); fmpq_init(l); fmpq_init(h); fmpq_init(m);
    fmpq_init(d); fmpq_init(t); fmpq_init(rho); fmpq_init(bound);
    fmpz_init(A); fmpz_init(H); fmpz_init(D); arf_init(r);
    CHECK(y->form == ADF_QCLASS_PIECES && adf_qclass_is_canonical(y));
    CHECK((size_t) y->len == jsonl_size(ps));
    for (i = 0; i < jsonl_size(ps); i++) {
        const jsonl_value *p = at(ps, i);
        ends(lo, hi, y->piece[i].inf); rat(l, at(p, 0)); rat(h, at(p, 1));
        CHECK(fmpq_equal(lo, l) && fmpq_equal(hi, h));
        CHECK(mag_is_zero(arb_radref(y->piece[i].inf)) || (MAG_MAN(arb_radref(y->piece[i].inf)) >= (UWORD(1) << 29)
              && MAG_MAN(arb_radref(y->piece[i].inf)) < (UWORD(1) << 30)));
        adf_fball_get_fmpz3(A, H, D, &y->piece[i].fin);
        rat(l, at(p, 2)); CHECK(fmpz_equal(A, fmpq_numref(l)));
        rat(l, at(p, 3)); CHECK(fmpz_equal(H, fmpq_numref(l)) && fmpz_is_one(D));
    }
    for (i = 0; i < jsonl_size(ex); i++) {
        const jsonl_value *p = at(ex, i); int found = 0;
        rat(l, at(p, 0)); rat(h, at(p, 1));
        for (j = 0; j < (size_t) y->len; j++) {
            adf_fball_get_fmpz3(A, H, D, &y->piece[j].fin);
            rat(t, at(p, 2)); if (!fmpz_equal(A, fmpq_numref(t))) continue;
            rat(t, at(p, 3)); if (!fmpz_equal(H, fmpq_numref(t))) continue;
            ends(lo, hi, y->piece[j].inf);
            if (fmpq_cmp(lo, l) > 0 || fmpq_cmp(hi, h) < 0) continue;
            arf_get_fmpq(m, arb_midref(y->piece[j].inf));
            fmpq_sub(d, m, l); fmpq_sub(t, h, m); if (fmpq_cmp(t, d) > 0) fmpq_set(d, t);
            arf_set_mag(r, arb_radref(y->piece[j].inf)); arf_get_fmpq(rho, r);
            fmpq_sub(t, rho, d); fmpq_div_2exp(bound, d, 28);
            if (fmpq_sgn(t) < 0 || fmpq_cmp(t, bound) > 0) continue;
            found = 1; break;
        }
        CHECK(found);
    }
    fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(l); fmpq_clear(h); fmpq_clear(m);
    fmpq_clear(d); fmpq_clear(t); fmpq_clear(rho); fmpq_clear(bound);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(D); arf_clear(r);
}
/* The sampled input points lie in the inputs (checked here, in C); their sum or negation in z. */
static void points(const adf_qclass_t z, const adf_qclass_t x, const adf_qclass_t y, const jsonl_value *v)
{
    const jsonl_value *ps = field(v, "points"); size_t i; int add = y != NULL;
    fmpq_t s1, w1, s2, w2;
    fmpq_init(s1); fmpq_init(w1); fmpq_init(s2); fmpq_init(w2);
    CHECK(jsonl_size(ps) == 40);
    for (i = 0; i < 40; i++) {
        const jsonl_value *p = at(ps, i);
        rat(s1, at(p, 0)); rat(w1, at(p, 1)); CHECK(union_member(x, s1, w1));
        if (add) {
            rat(s2, at(p, 2)); rat(w2, at(p, 3)); CHECK(union_member(y, s2, w2));
            fmpq_add(s1, s1, s2); fmpq_add(w1, w1, w2);
        } else { fmpq_neg(s1, s1); fmpq_neg(w1, w1); }
        CHECK(union_member(z, s1, w1));
    }
    fmpq_clear(s1); fmpq_clear(w1); fmpq_clear(s2); fmpq_clear(w2);
}
static int call(adf_qclass_t z, const adf_qclass_t x, const adf_qclass_t y, slong limit, slong prec)
{
    return y ? adf_qclass_add(z, x, y, limit, prec) : adf_qclass_neg(z, x, limit, prec);
}
/* A status failure leaves the struct bytes, the member bytes and the represented storage unchanged. */
static void refused(adf_qclass_t z, const adf_qclass_t x, const adf_qclass_t y, slong limit, slong prec)
{
    adf_qclass_t saved; adf_qclass_struct bytes; adf_adele_struct *members;
    adf_qclass_init(saved); adf_qclass_set(saved, z); memcpy(&bytes, z, sizeof(bytes));
    members = malloc((size_t) z->len*sizeof(*members)); CHECK(members);
    memcpy(members, z->piece, (size_t) z->len*sizeof(*members));
    CHECK(call(z, x, y, limit, prec) == ADF_LIMIT);
    CHECK(!memcmp(z, &bytes, sizeof(bytes)));
    CHECK(!memcmp(z->piece, members, (size_t) z->len*sizeof(*members)));
    CHECK(adf_qclass_identical(z, saved)); free(members); adf_qclass_clear(saved);
}
/* Exact Q2 containment when its budget allows; LIMIT is counted, never taken as a pass. */
static void inside(const adf_qclass_t a, const adf_qclass_t b)
{
    int truth = -1, s = adf_qclass_contains(&truth, a, b, WORD(4000000));
    if (s == ADF_OK) { CHECK(truth == 1); contains_ok++; }
    else { CHECK(s == ADF_LIMIT); contains_limit++; }
}
static void zero_class(adf_qclass_t z) { adf_qclass_clear(z); adf_qclass_init(z); }
/* A canonical LIFT with nonzero real and fractional finite radius (legal only as a LIFT). */
static void sentinel(adf_qclass_t z)
{
    adf_rat_t a, N; adf_rat_init(a); adf_rat_init(N); zero_class(z);
    arb_set_d(z->piece->inf, 0.375); mag_set_ui_2exp_si(arb_radref(z->piece->inf), 3, -5);
    fmpq_set_si(a->q, -5, 7); fmpq_set_si(N->q, 2, 3);
    CHECK(adf_fball_set_center_radius(&z->piece->fin, a, N) == ADF_OK);
    adf_rat_clear(a); adf_rat_clear(N); CHECK(adf_qclass_is_canonical(z));
}

static void vectors(const char *path)
{
    jsonl_file *f; jsonl_error_t e; size_t i, nadd = 0, nneg = 0;
    adf_qclass_t x, y, z, t, u, w, zero; fmpq_t q; adf_rat_t r;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(z); adf_qclass_init(t);
    adf_qclass_init(u); adf_qclass_init(w); adf_qclass_init(zero); fmpq_init(q); adf_rat_init(r);
    CHECK(jsonl_open(path, &f, &e));
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i);
        slong K = integer(field(v, "raw")), prec = integer(field(v, "prec"));
        int add = !strcmp(str(field(v, "op")), "add"), self = add && boolean(field(v, "self"));
        const adf_qclass_struct *yy;
        input(x, field(v, "x"));
        if (add && !self) input(y, field(v, "y"));
        yy = !add ? NULL : (self ? x : y);
        sentinel(z);
        refused(z, x, yy, K-1, prec); refused(z, x, yy, 0, prec); refused(z, x, yy, -1, prec);
        refused(z, x, yy, WORD_MIN, prec); refused(z, x, yy, K, ADF_REAL_PREC_MAX+1);
        CHECK(call(z, x, yy, K, prec) == ADF_OK);
        validate(z, v); points(z, x, yy, v);
        /* Every aliasing combination gives the same storage. */
        adf_qclass_set(t, x); CHECK(call(t, t, yy == x ? t : yy, K, prec) == ADF_OK);
        CHECK(adf_qclass_identical(t, z));
        if (add) {
            /* y + x: identical storage (the sort key does not see the pair order). */
            CHECK(adf_qclass_add(t, yy, x, K, prec) == ADF_OK && adf_qclass_identical(t, z));
            adf_qclass_set(t, yy); CHECK(adf_qclass_add(t, x, t, K, prec) == ADF_OK);
            CHECK(adf_qclass_identical(t, z));
            /* z = x = y: the sum of the class with itself, independent variation. */
            CHECK(adf_qclass_add(u, x, x, WORD_MAX, prec) == ADF_OK);
            adf_qclass_set(t, x); CHECK(adf_qclass_add(t, t, t, WORD_MAX, prec) == ADF_OK);
            CHECK(adf_qclass_identical(t, u));
            /* Exact translation commutes: add_rat then add, and add then add_rat. */
            fmpq_set_si(r->q, -7, 3); adf_qclass_add_rat(t, x, r);
            CHECK(adf_qclass_add(t, t, yy, K, prec) == ADF_OK && adf_qclass_identical(t, z));
            adf_qclass_add_rat(t, z, r); CHECK(adf_qclass_identical(t, z));
            nadd++;
        } else {
            /* neg(neg(x)) encloses x; x + neg(x) encloses the zero class. */
            CHECK(adf_qclass_neg(t, z, WORD_MAX, prec) == ADF_OK); inside(x, t);
            CHECK(adf_qclass_add(t, x, z, WORD_MAX, prec) == ADF_OK); inside(zero, t);
            nneg++;
        }
    }
    printf("%s: %zu add, %zu neg vectors, %zu sample points\n", path, nadd, nneg, 40*jsonl_count(f));
    jsonl_close(f); adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(z); adf_qclass_clear(t);
    adf_qclass_clear(u); adf_qclass_clear(w); adf_qclass_clear(zero); fmpq_clear(q); adf_rat_clear(r);
}
/* With the zero class the exact sum is x itself: the result is reduce(x), identical, so it is not
   wider than one Q1 margin. Negation of a lift is reduce of the exactly negated adele. */
static void zero_and_neg_identities(void)
{
    adf_qclass_t x, z, a, b; adf_rat_t c, N; slong i, j;
    static const double mids[] = {0.0, 0.375, -1.25, 2.5, 0.96875};
    static const slong rad_exp[] = {-100, -3, -1, 0, 1};
    static const slong nums[] = {0, 1, 2, 3, 7, 360}, dens[] = {1, 1, 3, 2, 360, 1};
    adf_qclass_init(x); adf_qclass_init(z); adf_qclass_init(a); adf_qclass_init(b);
    adf_rat_init(c); adf_rat_init(N);
    for (i = 0; i < 5; i++) for (j = 0; j < 6; j++) {
        arb_set_d(x->piece->inf, mids[i]);
        if (i) mag_set_ui_2exp_si(arb_radref(x->piece->inf), 3, rad_exp[i]);
        fmpq_set_si(c->q, -3*(slong) i+1, 5); fmpq_set_si(N->q, nums[j], (ulong) dens[j]);
        CHECK(adf_fball_set_center_radius(&x->piece->fin, c, N) == ADF_OK);
        CHECK(adf_qclass_reduce(a, x, WORD_MAX, 53) == ADF_OK);
        CHECK(adf_qclass_add(b, x, z, WORD_MAX, 53) == ADF_OK && adf_qclass_identical(a, b));
        CHECK(adf_qclass_add(b, z, x, WORD_MAX, 53) == ADF_OK && adf_qclass_identical(a, b));
        CHECK(adf_qclass_add(b, a, z, WORD_MAX, 53) == ADF_OK); /* PIECES + 0 = re-reduction */
        { adf_qclass_t c2; adf_qclass_init(c2);
          CHECK(adf_qclass_reduce(c2, a, WORD_MAX, 53) == ADF_OK && adf_qclass_identical(c2, b));
          adf_qclass_clear(c2); }
        adf_qclass_set(a, x); adf_adele_neg(a->piece, a->piece);
        CHECK(adf_qclass_reduce(a, a, WORD_MAX, 20) == ADF_OK);
        CHECK(adf_qclass_neg(b, x, WORD_MAX, 20) == ADF_OK && adf_qclass_identical(a, b));
    }
    printf("zero class and negated lift identities: 30 lifts\n");
    adf_qclass_clear(x); adf_qclass_clear(z); adf_qclass_clear(a); adf_qclass_clear(b);
    adf_rat_clear(c); adf_rat_clear(N);
}
/* Precision, piece limits, the pair count, R5 preflight and exact-work bounds; all LIMIT untouched. */
static void limits(void)
{
    adf_qclass_t x, y, z; adf_rat_t c, N; clock_t start; double sec; slong i;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(z); adf_rat_init(c); adf_rat_init(N);
    sentinel(z);
    CHECK(adf_qclass_add(z, x, y, 1, 2) == ADF_OK && z->len == 1 && arb_is_zero(z->piece->inf));
    CHECK(adf_qclass_add(z, x, y, 1, ADF_REAL_PREC_MAX) == ADF_OK);
    CHECK(adf_qclass_add(z, x, y, 1, -5) == ADF_OK);
    CHECK(adf_qclass_neg(z, x, 1, ADF_REAL_PREC_MAX) == ADF_OK);
    sentinel(z);
    refused(z, x, y, 1, ADF_REAL_PREC_MAX+1); refused(z, x, NULL, 1, ADF_REAL_PREC_MAX+1);
    refused(z, x, y, 1, WORD_MAX); refused(z, x, NULL, 0, 53); refused(z, x, y, 0, 53);
    refused(z, x, NULL, WORD_MIN, 53);
    /* Pair count: a 3-entry PIECES plus a 2-entry PIECES of real points has 6 pairs and K = 6. */
    { adf_qclass_t p, q; adf_qclass_init(p); adf_qclass_init(q);
      arb_set_d(x->piece->inf, 0.5); fmpq_set_si(N->q, 1, 3); fmpq_zero(c->q);
      CHECK(adf_fball_set_center_radius(&x->piece->fin, c, N) == ADF_OK);
      CHECK(adf_qclass_reduce(p, x, 3, 53) == ADF_OK && p->len == 3);
      fmpq_set_si(N->q, 1, 2);
      CHECK(adf_fball_set_center_radius(&x->piece->fin, c, N) == ADF_OK);
      CHECK(adf_qclass_reduce(q, x, 2, 53) == ADF_OK && q->len == 2);
      CHECK(adf_qclass_add(z, p, q, 6, 53) == ADF_OK);
      sentinel(z); refused(z, p, q, 5, 53); refused(z, q, p, 5, 53); refused(z, p, q, 2, 53);
      CHECK(adf_qclass_neg(z, p, 3, 53) == ADF_OK); refused(z, p, NULL, 2, 53);
      adf_qclass_clear(p); adf_qclass_clear(q); }
    /* R5: exactly 10^30 fibres, and a width of 2^100: LIMIT before any loop. */
    fmpz_set_str(fmpq_denref(N->q), "1000000000000000000000000000000", 10); fmpz_one(fmpq_numref(N->q));
    CHECK(adf_fball_set_center_radius(&x->piece->fin, c, N) == ADF_OK);
    start = clock(); refused(z, x, y, 10, 53); refused(z, y, x, WORD_MAX, 53); refused(z, x, NULL, 10, 53);
    sec = (double) (clock()-start)/CLOCKS_PER_SEC; CHECK(sec < 1.0);
    printf("10^30 fibres LIMIT: CPU %.6f s, guard 1.0 s\n", sec);
    adf_fball_zero(&x->piece->fin); arb_zero(x->piece->inf);
    mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, 100);
    start = clock(); refused(z, x, y, WORD_MAX, 53); refused(z, x, NULL, WORD_MAX, 53);
    sec = (double) (clock()-start)/CLOCKS_PER_SEC; CHECK(sec < 1.0);
    printf("2^101 width LIMIT: CPU %.6f s, guard 1.0 s\n", sec);
    /* 2^62 pieces pass a WORD_MAX limit but not the byte product: LIMIT before allocation. */
    mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, 61);
    start = clock(); refused(z, x, y, WORD_MAX, 53); refused(z, x, NULL, WORD_MAX, 53);
    sec = (double) (clock()-start)/CLOCKS_PER_SEC; CHECK(sec < 1.0);
    printf("2^62 pieces, byte product LIMIT: CPU %.6f s, guard 1.0 s\n", sec);
    /* Exact-work bound: 2^(2^20) + 2^(-2^20) needs more than 2^21 bits (D3-2). Each alone is fine. */
    arb_one(x->piece->inf); arb_mul_2exp_si(x->piece->inf, x->piece->inf, ADF_QCLASS_EXP_MAX-1);
    arb_one(y->piece->inf); arb_mul_2exp_si(y->piece->inf, y->piece->inf, -ADF_QCLASS_EXP_MAX+1);
    CHECK(adf_qclass_neg(z, x, 1, 53) == ADF_OK); CHECK(adf_qclass_neg(z, y, 1, 53) == ADF_OK);
    sentinel(z); refused(z, x, y, WORD_MAX, 53); refused(z, y, x, WORD_MAX, 53);
    /* The same exponent on both sides stays within the bound: sum 2^(2^20), one piece. */
    CHECK(adf_qclass_add(z, x, x, 1, 53) == ADF_OK && z->len == 1);
    /* Finite sum: centres near 1/2 (so that Q1's midpoint exponent is small) with denominators
       of 2^19+1 bits each pass; with 2^20+5 bits each the projected lcm of the centre sum
       exceeds ADF_QCLASS_BITS_MAX although either class alone is admitted. */
    arb_zero(x->piece->inf); arb_zero(y->piece->inf); fmpq_zero(N->q);
    for (i = 0; i < 2; i++) {
        slong bits = i ? 1048580 : 524288;
        fmpz_one(fmpq_numref(c->q)); fmpz_mul_2exp(fmpq_numref(c->q), fmpq_numref(c->q), (ulong) bits-1);
        fmpz_mul_2exp(fmpq_denref(c->q), fmpq_numref(c->q), 1);
        fmpz_add_ui(fmpq_denref(c->q), fmpq_denref(c->q), 1);
        CHECK(adf_fball_set_center_radius(&x->piece->fin, c, N) == ADF_OK);
        fmpz_add_ui(fmpq_denref(c->q), fmpq_denref(c->q), 2);
        CHECK(adf_fball_set_center_radius(&y->piece->fin, c, N) == ADF_OK);
        CHECK(adf_qclass_neg(z, x, 1, 53) == ADF_OK && adf_qclass_neg(z, y, 1, 53) == ADF_OK);
        if (!i) CHECK(adf_qclass_add(z, x, y, 1, 53) == ADF_OK);
        else { sentinel(z); refused(z, x, y, WORD_MAX, 53); refused(z, y, x, WORD_MAX, 53); }
    }
    printf("limits: precision, piece limits, pair count, R5, exact-work bounds\n");
    adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(z); adf_rat_clear(c); adf_rat_clear(N);
}
/* Local backend inputs (blocks 8, 9, 5): the same storage as their global equivalents. */
static void local_inputs(void)
{
    adf_qclass_t x, xl, y, yl, a, b; adf_modctx_struct *ctx = NULL; adf_rat_t c, N;
    ulong blocks[] = {8, 9, 5};
    adf_qclass_init(x); adf_qclass_init(xl); adf_qclass_init(y); adf_qclass_init(yl);
    adf_qclass_init(a); adf_qclass_init(b); adf_rat_init(c); adf_rat_init(N);
    CHECK(adf_modctx_new_blocks(&ctx, blocks, 3) == ADF_OK);
    arb_set_d(x->piece->inf, 0.75); mag_set_ui_2exp_si(arb_radref(x->piece->inf), 5, -3);
    fmpq_set_si(c->q, 1, 4); fmpq_set_si(N->q, 1, 2);
    CHECK(adf_fball_set_center_radius(&x->piece->fin, c, N) == ADF_OK);
    adf_qclass_set(xl, x); CHECK(adf_fball_set_local(&xl->piece->fin, &xl->piece->fin, ctx) == ADF_OK);
    CHECK(xl->piece->fin.mctx != NULL);
    arb_set_d(y->piece->inf, -0.5); mag_set_ui_2exp_si(arb_radref(y->piece->inf), 1, -2);
    fmpq_set_si(c->q, 7, 1); fmpq_set_si(N->q, 12, 1);
    CHECK(adf_fball_set_center_radius(&y->piece->fin, c, N) == ADF_OK);
    adf_qclass_set(yl, y); CHECK(adf_fball_set_local(&yl->piece->fin, &yl->piece->fin, ctx) == ADF_OK);
    CHECK(adf_qclass_add(a, x, y, WORD_MAX, 53) == ADF_OK);
    CHECK(adf_qclass_add(b, xl, yl, WORD_MAX, 53) == ADF_OK && adf_qclass_identical(a, b));
    CHECK(adf_qclass_add(b, xl, y, WORD_MAX, 53) == ADF_OK && adf_qclass_identical(a, b));
    CHECK(adf_qclass_add(b, yl, xl, WORD_MAX, 53) == ADF_OK && adf_qclass_identical(a, b));
    { slong i; for (i = 0; i < b->len; i++) CHECK(b->piece[i].fin.mctx == NULL); }
    CHECK(adf_qclass_neg(a, x, WORD_MAX, 53) == ADF_OK);
    CHECK(adf_qclass_neg(b, xl, WORD_MAX, 53) == ADF_OK && adf_qclass_identical(a, b));
    adf_qclass_set(b, xl); CHECK(adf_qclass_neg(b, b, WORD_MAX, 53) == ADF_OK && adf_qclass_identical(a, b));
    CHECK(xl->piece->fin.mctx == ctx && yl->piece->fin.mctx == ctx);
    printf("local inputs: blocks 8, 9, 5\n");
    adf_qclass_clear(x); adf_qclass_clear(xl); adf_qclass_clear(y); adf_qclass_clear(yl);
    adf_qclass_clear(a); adf_qclass_clear(b); adf_rat_clear(c); adf_rat_clear(N); adf_modctx_free(ctx);
}
/* The hand example of the driver fixture: (0.75 +/- 0.25 ; 0 mod 3) + (0.5 +/- 0.5 ; 1 mod 3).
   Exact sum [1/2,2] x (1 + 3 Zhat); l = -1/2, h = 1; n = -1, 0 give [1/2,1] x (1 mod 3) and
   [0,1] x (0 mod 3). Q1 at 53 bits: midpoints 3/4 and 1/2, radii 1/4+2^-31 and 1/2+2^-30. */
static void hand_example(void)
{
    adf_qclass_t x, y, z; adf_rat_t c, N; fmpq_t lo, hi, t;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(z); adf_rat_init(c); adf_rat_init(N);
    fmpq_init(lo); fmpq_init(hi); fmpq_init(t);
    arb_set_d(x->piece->inf, 0.75); mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, -2);
    fmpq_set_si(N->q, 3, 1); CHECK(adf_fball_set_center_radius(&x->piece->fin, c, N) == ADF_OK);
    arb_set_d(y->piece->inf, 0.5); mag_set_ui_2exp_si(arb_radref(y->piece->inf), 1, -1);
    fmpq_set_si(c->q, 1, 1); CHECK(adf_fball_set_center_radius(&y->piece->fin, c, N) == ADF_OK);
    CHECK(adf_qclass_add(z, x, y, 1, 53) == ADF_LIMIT);
    CHECK(adf_qclass_add(z, x, y, 2, 53) == ADF_OK && z->len == 2);
    ends(lo, hi, z->piece[0].inf);
    fmpq_set_si(t, -1, 1); fmpq_div_2exp(t, t, 30); CHECK(fmpq_equal(lo, t));
    CHECK(fmpz_equal_si(z->piece[0].fin.A, 0) && fmpz_equal_si(z->piece[0].fin.H, 3));
    ends(lo, hi, z->piece[1].inf);
    fmpq_set_si(t, 1, 1); fmpq_div_2exp(t, t, 31); fmpq_sub(t, hi, t); CHECK(fmpz_equal_si(fmpq_numref(t), 1));
    CHECK(fmpz_equal_si(fmpq_denref(t), 1));
    CHECK(fmpz_equal_si(z->piece[1].fin.A, 1) && fmpz_equal_si(z->piece[1].fin.H, 3));
    printf("hand example: two pieces, crossing at 1\n");
    adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(z); adf_rat_clear(c); adf_rat_clear(N);
    fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(t);
}
static void debug_entry(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int k;
    fflush(NULL);
    for (k = 0; k < 3; k++) {
        pid_t pid = fork(); int status; CHECK(pid >= 0);
        if (!pid) {
            adf_qclass_t x, y, z; alarm(5);
            if (!freopen("/dev/null", "w", stderr)) _exit(2);
            adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(z);
            (k == 1 ? y : x)->form = 8;
            if (k == 2) adf_qclass_neg(z, x, 0, 53); else adf_qclass_add(z, x, y, 0, 53);
            _exit(0);
        }
        CHECK(waitpid(pid, &status, 0) == pid);
        CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
    printf("INV: 3 invalid input aborts\n");
#endif
}
static void arf_pow(arf_t a, slong mant_bits, slong exp)
{
    fmpz_t m, e; fmpz_init(m); fmpz_init(e);
    fmpz_one(m); fmpz_mul_2exp(m, m, (ulong) mant_bits-1); if (mant_bits > 1) fmpz_add_ui(m, m, 1);
    fmpz_set_si(e, exp); arf_set_fmpz_2exp(a, m, e); fmpz_clear(m); fmpz_clear(e);
}
/* Boundaries of the copied helpers (D3-2: exponent 2^20, 2^21 bits) and Q1's binade carry. */
static void components(void)
{
    arf_t a; fmpq_t x, y, z, lo, hi; arb_t r; fmpz_t D; arf_t t; fmpq_t q;
    const slong BM = ADF_QCLASS_BITS_MAX, EM = ADF_QCLASS_EXP_MAX;
    arf_init(a); fmpq_init(x); fmpq_init(y); fmpq_init(z); fmpq_init(lo); fmpq_init(hi);
    arb_init(r); fmpz_init(D); arf_init(t); fmpq_init(q);
    /* Exponent: arf exponent = mantissa bits + exp. */
    arf_pow(a, 1, EM-1); CHECK(qa_arf_bound(a)); arf_pow(a, 1, EM); CHECK(!qa_arf_bound(a));
    arf_pow(a, 1, -EM-1); CHECK(qa_arf_bound(a)); arf_pow(a, 1, -EM-2); CHECK(!qa_arf_bound(a));
    /* b - e + 1 denominator bits: 2^20-bit odd mantissa at e = -2^20 needs 2^21+1 bits. */
    arf_pow(a, EM, -2*EM); CHECK(!qa_arf_bound(a));
    arf_pow(a, EM-1, -2*EM+1); CHECK(qa_arf_bound(a));
    /* max(e,b) = b = 2^21 at e = 1 is admitted; 2^21+1 bits are not. */
    arf_pow(a, BM, 1-BM); CHECK(qa_arf_bound(a)); arf_pow(a, BM+1, -BM); CHECK(!qa_arf_bound(a));
    /* Equal denominators of exactly 2^21 bits are admitted; numerators need a carry bit. */
    fmpz_one(D); fmpz_mul_2exp(D, D, (ulong) BM-1); fmpz_add_ui(D, D, 1);
    fmpz_one(fmpq_numref(x)); fmpz_set(fmpq_denref(x), D); fmpq_set(y, x);
    CHECK(qa_add(z, x, y, 0) && fmpz_equal_si(fmpq_numref(z), 2));
    fmpz_add_ui(fmpq_denref(x), D, 2); fmpz_set(fmpq_denref(y), fmpq_denref(x));
    fmpz_mul_2exp(fmpq_denref(x), fmpq_denref(x), 1); fmpz_mul_2exp(fmpq_denref(y), fmpq_denref(y), 1);
    CHECK(!qa_add(z, x, y, 0)); /* 2^21+1-bit denominators */
    fmpz_one(fmpq_numref(x)); fmpz_mul_2exp(fmpq_numref(x), fmpq_numref(x), (ulong) BM-1);
    fmpz_one(fmpq_denref(x)); fmpq_one(y); fmpz_set_si(fmpq_numref(z), 5);
    CHECK(!qa_add(z, x, y, 0) && fmpz_equal_si(fmpq_numref(z), 5)); /* 2^21-bit numerator plus 1 */
    fmpz_fdiv_q_2exp(fmpq_numref(x), fmpq_numref(x), 1); CHECK(qa_add(z, x, y, 0));
    /* Q1 carry: d = (2^31-3)/2^32 has RU30 = (2^30-1) 2^-31, whose successor is 1/2 = 2^29 2^-30. */
    fmpq_set_si(x, (slong) ((UWORD(1) << 31)-3), 1); fmpq_div_2exp(x, x, 32);
    fmpq_set_si(lo, 1, 2); fmpq_sub(lo, lo, x); fmpq_set_si(hi, 1, 2); fmpq_add(hi, hi, x);
    CHECK(qa_round(r, lo, hi, 53));
    CHECK(MAG_MAN(arb_radref(r)) == (UWORD(1) << 29));
    arf_set_mag(t, arb_radref(r)); arf_get_fmpq(q, t); fmpq_set_si(x, 1, 2); CHECK(fmpq_equal(q, x));
    /* Q1 refuses (returns 0) when its midpoint sum exceeds the bit bound. */
    fmpz_one(D); fmpz_mul_2exp(D, D, (ulong) BM/2+5); fmpz_add_ui(D, D, 1);
    fmpz_one(fmpq_numref(hi)); fmpz_set(fmpq_denref(hi), D);
    fmpz_one(fmpq_numref(lo)); fmpz_add_ui(fmpq_denref(lo), D, 2);
    CHECK(!qa_round(r, lo, hi, 53));
    printf("components: exponent, bit, carry and refusal boundaries of the copied helpers\n");
    arf_clear(a); fmpq_clear(x); fmpq_clear(y); fmpq_clear(z); fmpq_clear(lo); fmpq_clear(hi);
    arb_clear(r); fmpz_clear(D); arf_clear(t); fmpq_clear(q);
}
int main(void)
{
    components();
    vectors("tests/ref/vectors/q-slice7/arith.jsonl");
    zero_and_neg_identities(); hand_example(); limits(); local_inputs(); debug_entry();
    printf("qclass arith: %lu checks; Q2 containment %lu decided, %lu over budget\n",
           checks, contains_ok, contains_limit);
    flint_cleanup(); return 0;
}
