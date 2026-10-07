/* Algorithm R oracle and Q1 contract: docs/api-3.md 2.2, 4; quotient.md P3, P6, P8, P10.
   Vectors document exact stored dyadic intervals. Expected storage deduplicates after Q1.
   Membership below solves the rational translation condition independently of R. */
#include <adelefeld.h>
#include "support/jsonl.h"
#include "support/golden.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

static int watch_exact_set;
static ulong premature_fibre_copies;
static void observed_fmpq_set(fmpq_t dst, const fmpq_t src)
{
    if (watch_exact_set && fmpz_is_one(fmpq_denref(src)) && fmpz_bits(fmpq_numref(src)) > 1000)
        premature_fibre_copies++;
    fmpq_set(dst, src);
}
/* Compile the same source here to check its exact-work preflight BEFORE conversion.
   A public LIMIT alone cannot distinguish an early refusal from a later q_size refusal.
   The other qclass test links the archive normally; all vector calls below use this same source. */
#define fmpq_set observed_fmpq_set
#include "../src/qclass.c"
#undef fmpq_set

static ulong checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "reduce line %d: %s\n", __LINE__, #c); abort(); } } while (0)

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
static void rat(fmpq_t q, const jsonl_value *v) { CHECK(!fmpq_set_str(q, str(v), 10)); }
static void ends(fmpq_t lo, fmpq_t hi, arb_srcptr x)
{
    arf_t r; fmpq_t q;
    arf_init(r); fmpq_init(q); arf_get_fmpq(q, arb_midref(x));
    arf_set_mag(r, arb_radref(x)); arf_get_fmpq(hi, r);
    fmpq_sub(lo, q, hi); fmpq_add(hi, q, hi); fmpq_clear(q); arf_clear(r);
}
static void input_piece(adf_adele_struct *x, const jsonl_value *v)
{
    fmpq_t q; arf_t r; adf_rat_t a, N;
    fmpq_init(q); arf_init(r); adf_rat_init(a); adf_rat_init(N);
    rat(q, field(v, "mid"));
    arf_set_fmpq(arb_midref(x->inf), q, ARF_PREC_EXACT, ARF_RND_NEAR);
    rat(q, field(v, "rad")); arf_set_fmpq(r, q, ARF_PREC_EXACT, ARF_RND_NEAR);
    if (arf_is_zero(r)) mag_zero(arb_radref(x->inf));
    else {
        fmpz_t mant, exp; fmpz_init(mant); fmpz_init(exp);
        arf_get_fmpz_2exp(mant, exp, r);
        fmpz_mul_2exp(mant, mant, 30-fmpz_bits(mant));
        MAG_MAN(arb_radref(x->inf)) = fmpz_get_ui(mant);
        fmpz_set(MAG_EXPREF(arb_radref(x->inf)), ARF_EXPREF(r));
        fmpz_clear(mant); fmpz_clear(exp);
    }
    /* The dyadic vector radius already has <=30 bits: this conversion must be exact. */
    { fmpq_t t; fmpq_init(t); arf_set_mag(r, arb_radref(x->inf)); arf_get_fmpq(t, r);
      CHECK(fmpq_equal(t, q)); fmpq_clear(t); }
    rat(a->q, field(v, "a")); rat(N->q, field(v, "N"));
    CHECK(adf_fball_set_center_radius(&x->fin, a, N) == ADF_OK);
    ends(a->q, N->q, x->inf);
    rat(q, field(v, "lo")); CHECK(fmpq_equal(q, a->q));
    rat(q, field(v, "hi")); CHECK(fmpq_equal(q, N->q));
    fmpq_clear(q); arf_clear(r); adf_rat_clear(a); adf_rat_clear(N);
}
static void input(adf_qclass_t x, const jsonl_value *v)
{
    const jsonl_value *ps; jsonl_error_t e; slong i;
    adf_qclass_clear(x);
    if (jsonl_field(v, "input_pieces", &ps, &e)) {
        x->form = ADF_QCLASS_PIECES; x->len = (slong) jsonl_size(ps);
        x->piece = flint_malloc((size_t) x->len*sizeof(*x->piece));
        for (i = 0; i < x->len; i++) {
            adf_adele_init(x->piece+i); input_piece(x->piece+i, at(ps, (size_t) i));
        }
    } else { adf_qclass_init(x); input_piece(x->piece, v); }
    CHECK(adf_qclass_is_canonical(x));
}
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
static void validate(const adf_qclass_t y, const jsonl_value *v)
{
    const jsonl_value *ps = field(v, "rounded"), *ex = field(v, "exact"), *points = field(v, "points");
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
        CHECK(fmpq_equal(lo, l) && fmpq_equal(hi, h)); /* Pins RU30 then successor exactly. */
        adf_fball_get_fmpz3(A, H, D, &y->piece[i].fin);
        rat(l, at(p, 2)); CHECK(fmpz_equal(A, fmpq_numref(l)));
        rat(l, at(p, 3)); CHECK(fmpz_equal(H, fmpq_numref(l)) && fmpz_is_one(D));
        CHECK(arf_sgn(arb_midref(y->piece[i].inf)) >= 0);
        CHECK(arf_cmp_ui(arb_midref(y->piece[i].inf), 1) <= 0);
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
        CHECK(found); /* Superset AND rho-d <= 2^-28 d. */
    }
    CHECK(jsonl_size(points) == 40);
    for (i = 0; i < 40; i++) {
        int yes; jsonl_error_t e; const jsonl_value *p = at(points, i);
        rat(l, at(p, 0)); rat(h, at(p, 1)); CHECK(jsonl_bool(at(p, 2), &yes, &e));
        if (yes) CHECK(union_member(y, l, h));
    }
    fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(l); fmpq_clear(h); fmpq_clear(m);
    fmpq_clear(d); fmpq_clear(t); fmpq_clear(rho); fmpq_clear(bound);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(D); arf_clear(r);
}
static void vectors(const char *path)
{
    jsonl_file *f; jsonl_error_t e; size_t i; adf_qclass_t x, y, saved, alias;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(saved); adf_qclass_init(alias);
    CHECK(jsonl_open(path, &f, &e));
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i); slong K = integer(field(v, "raw"));
        slong prec = integer(field(v, "prec"));
        input(x, v); adf_qclass_set(saved, y);
        { size_t j; fmpq_t s, w; const jsonl_value *points = field(v, "points");
          fmpq_init(s); fmpq_init(w);
          for (j = 0; j < jsonl_size(points); j++) {
              int yes; const jsonl_value *p = at(points, j);
              rat(s, at(p, 0)); rat(w, at(p, 1)); CHECK(jsonl_bool(at(p, 2), &yes, &e));
              CHECK(union_member(x, s, w) == yes);
          }
          fmpq_clear(s); fmpq_clear(w); }
        CHECK(adf_qclass_reduce(y, x, K-1, prec) == ADF_LIMIT);
        CHECK(adf_qclass_identical(y, saved));
        CHECK(adf_qclass_reduce(y, x, 0, prec) == ADF_LIMIT && adf_qclass_identical(y, saved));
        CHECK(adf_qclass_reduce(y, x, K, prec) == ADF_OK); validate(y, v);
        adf_qclass_set(alias, x); CHECK(adf_qclass_reduce(alias, alias, K, prec) == ADF_OK);
        CHECK(adf_qclass_identical(alias, y));
    }
    printf("%s: %zu vectors, %zu point labels\n", path, jsonl_count(f), 40*jsonl_count(f));
    jsonl_close(f); adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(saved);
    adf_qclass_clear(alias);
}
static void printer(void)
{
    adf_qclass_t x, y; size_t n; char *s;
    adf_qclass_init(x); adf_qclass_init(y);
    arb_set_d(x->piece->inf, 0.5); adf_fball_set_si(&x->piece->fin, 7);
    CHECK(adf_qclass_reduce(y, x, 1, 53) == ADF_OK);
    s = adf_qclass_get_str(&n, y, 2);
    CHECK(s && n == strlen("union((0.5 ; 7)) + Q") && !strcmp(s, "union((0.5 ; 7)) + Q"));
    adf_str_free(s); adf_qclass_clear(x); adf_qclass_clear(y);
}
/* refs/src/flint-3.0.1/memory.rst:16-22 permits allocator callbacks. On the simple
   small-count refusal below no rational temporary needs heap storage; NO allocation is allowed. */
static void *(*old_malloc)(size_t);
static void *(*old_calloc)(size_t, size_t);
static void *(*old_realloc)(void *, size_t);
static void (*old_free)(void *);
static ulong allocations;
static void *observe_malloc(size_t n) { allocations++; return old_malloc(n); }
static void *observe_calloc(size_t n, size_t s) { allocations++; return old_calloc(n, s); }
static void *observe_realloc(void *p, size_t n) { allocations++; return old_realloc(p, n); }

static void refused(adf_qclass_t y, const adf_qclass_t x, slong limit, slong prec)
{
    adf_qclass_t saved; adf_qclass_struct bytes; adf_adele_struct *members;
    adf_qclass_init(saved); adf_qclass_set(saved, y); memcpy(&bytes, y, sizeof(bytes));
    members = malloc((size_t) y->len*sizeof(*members)); CHECK(members);
    memcpy(members, y->piece, (size_t) y->len*sizeof(*members));
    CHECK(adf_qclass_reduce(y, x, limit, prec) == ADF_LIMIT);
    CHECK(!memcmp(y, &bytes, sizeof(bytes)));
    CHECK(!memcmp(y->piece, members, (size_t) y->len*sizeof(*members)));
    CHECK(adf_qclass_identical(y, saved)); free(members); adf_qclass_clear(saved);
}
static void bounds(void)
{
    adf_qclass_t x, y, saved; fmpz_t z; adf_rat_t a, N; clock_t start; double sec;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(saved);
    fmpz_init(z); adf_rat_init(a); adf_rat_init(N);
    arb_set_si(y->piece->inf, 7); adf_fball_set_si(&y->piece->fin, -19);
    adf_qclass_set(saved, y);
    refused(y, x, 1, ADF_REAL_PREC_MAX+1); refused(y, x, -1, 53);
    CHECK(adf_qclass_reduce(y, x, 1, ADF_REAL_PREC_MAX) == ADF_OK);
    CHECK(y->form == ADF_QCLASS_PIECES && arb_is_zero(y->piece->inf));
    CHECK(adf_qclass_reduce(y, x, 1, -100) == ADF_OK); /* max(prec,2) */
    arb_one(x->piece->inf); mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, -3);
    fmpq_set_si(N->q, 2, 1); CHECK(adf_fball_set_center_radius(&x->piece->fin, a, N) == ADF_OK);
    /* Warm caches before observing the preflight. */
    refused(y, x, 1, 53);
    __flint_get_memory_functions(&old_malloc, &old_calloc, &old_realloc, &old_free);
    allocations = 0;
    __flint_set_memory_functions(observe_malloc, observe_calloc, observe_realloc, old_free);
    CHECK(adf_qclass_reduce(y, x, 1, 53) == ADF_LIMIT);
    __flint_set_memory_functions(old_malloc, old_calloc, old_realloc, old_free);
    CHECK(allocations == 0); printf("count preflight: %lu allocation calls\n", allocations);
    adf_qclass_set(y, saved);
    /* Exactly 10^30 fractional fibres; compare B before any fibre loop or array allocation. */
    arb_zero(x->piece->inf); fmpz_set_str(fmpq_denref(N->q), "1000000000000000000000000000000", 10);
    fmpz_one(fmpq_numref(N->q));
    CHECK(adf_fball_set_center_radius(&x->piece->fin, a, N) == ADF_OK);
    start = clock(); refused(y, x, 10, 53); sec = (double) (clock()-start)/CLOCKS_PER_SEC;
    CHECK(sec < 1.0); printf("10^30 count LIMIT: CPU %.6f s, guard 1.0 s\n", sec);
    /* Width count is also rejected before the integer loop. */
    adf_fball_zero(&x->piece->fin); arb_zero(x->piece->inf);
    mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, 100);
    start = clock(); refused(y, x, 10, 53); sec = (double) (clock()-start)/CLOCKS_PER_SEC;
    CHECK(sec < 1.0); printf("2^101 width LIMIT: CPU %.6f s, guard 1.0 s\n", sec);
#if FLINT_BITS == 64
    /* The count fits slong and the requested limit, but K*sizeof(adele) overflows size_t. */
    mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, 57); fmpz_one(x->piece->fin.H);
    start = clock(); refused(y, x, WORD_MAX, 53); sec = (double) (clock()-start)/CLOCKS_PER_SEC;
    CHECK(sec < 1.0); printf("2^58 byte-product LIMIT: CPU %.6f s, guard 1.0 s\n", sec);
    adf_fball_zero(&x->piece->fin);
#endif
    arb_zero(x->piece->inf); fmpz_one(z); fmpz_mul_2exp(z, z, 2000); fmpz_neg(z, z);
    arb_set_fmpz(x->piece->inf, z); CHECK(adf_qclass_reduce(y, x, 1, 53) == ADF_OK);
    fmpz_neg(z, z); CHECK(arb_is_zero(y->piece->inf) && fmpz_equal(y->piece->fin.A, z));
    /* Midpoint exponent boundary, both signs. */
    arb_one(x->piece->inf);
    fmpz_set_si(ARF_EXPREF(arb_midref(x->piece->inf)), ADF_QCLASS_EXP_MAX);
    CHECK(adf_qclass_reduce(y, x, 1, 53) == ADF_OK);
    fmpz_set_si(ARF_EXPREF(arb_midref(x->piece->inf)), ADF_QCLASS_EXP_MAX+1);
    refused(y, x, 1, 53);
    fmpz_set_si(ARF_EXPREF(arb_midref(x->piece->inf)), -ADF_QCLASS_EXP_MAX);
    CHECK(adf_qclass_reduce(y, x, 1, 53) == ADF_OK);
    fmpz_set_si(ARF_EXPREF(arb_midref(x->piece->inf)), -ADF_QCLASS_EXP_MAX-1);
    refused(y, x, 1, 53);
    /* b-E+1 is the denominator bit count, including the leading 1 of 2^(b-E). */
    { fmpz_t exp; fmpz_init_set_si(exp, -ADF_QCLASS_BITS_MAX);
      fmpz_one(z); fmpz_mul_2exp(z, z, ADF_QCLASS_BITS_MAX-1); fmpz_add_ui(z, z, 1);
      arf_set_fmpz_2exp(arb_midref(x->piece->inf), z, exp);
      CHECK(!q_arf_bound(arb_midref(x->piece->inf)));
      refused(y, x, 1, 53);
      /* Exactly the cap: 1/2 + 2^(-(BITS_MAX-1)), denominator bit count BITS_MAX.
         At full precision it remains an exact point and must succeed. */
      fmpz_one(z); fmpz_mul_2exp(z, z, ADF_QCLASS_BITS_MAX-2); fmpz_add_ui(z, z, 1);
      fmpz_add_ui(exp, exp, 1); arf_set_fmpz_2exp(arb_midref(x->piece->inf), z, exp);
      CHECK(q_arf_bound(arb_midref(x->piece->inf)));
      CHECK(adf_qclass_reduce(y, x, 1, ADF_REAL_PREC_MAX) == ADF_OK);
      CHECK(arb_equal(y->piece->inf, x->piece->inf)); fmpz_clear(exp); }
    /* Radius exponent guard; a positive radius at the allowed upper edge fails the count instead. */
    arb_zero(x->piece->inf); mag_one(arb_radref(x->piece->inf));
    fmpz_set_si(MAG_EXPREF(arb_radref(x->piece->inf)), ADF_QCLASS_EXP_MAX+1);
    refused(y, x, 1, 53);
    fmpz_set_si(MAG_EXPREF(arb_radref(x->piece->inf)), -ADF_QCLASS_EXP_MAX-1);
    refused(y, x, 2, 53);
    arb_zero(x->piece->inf);
    fmpz_one(z); fmpz_mul_2exp(z, z, ADF_QCLASS_BITS_MAX-1);
    fmpz_set(fmpq_numref(N->q), z); fmpz_one(fmpq_denref(N->q));
    CHECK(adf_fball_set_center_radius(&x->piece->fin, a, N) == ADF_OK);
    CHECK(adf_qclass_reduce(y, x, 1, 53) == ADF_OK); /* exactly the finite bit cap */
    fmpz_mul_2exp(fmpq_numref(N->q), fmpq_numref(N->q), 1);
    CHECK(adf_fball_set_center_radius(&x->piece->fin, a, N) == ADF_OK); refused(y, x, 1, 53);
    /* Same-object LIMIT preserves the exact original representation. */
    refused(x, x, 1, 53);
    adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(saved);
    fmpz_clear(z); adf_rat_clear(a); adf_rat_clear(N);
}
static void local_and_glue(void)
{
    adf_qclass_t x, y, global; adf_modctx_struct *ctx = NULL;
    ulong blocks[] = {4, 3}; fmpq_t s, w; adf_rat_t a, N;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(global);
    fmpq_init(s); fmpq_init(w); adf_rat_init(a); adf_rat_init(N);
    CHECK(adf_modctx_new_blocks(&ctx, blocks, 2) == ADF_OK);
    arb_one(x->piece->inf); fmpq_set_si(N->q, 3, 1);
    CHECK(adf_fball_set_center_radius(&x->piece->fin, a, N) == ADF_OK);
    CHECK(adf_qclass_reduce(global, x, 1, 53) == ADF_OK);
    CHECK(adf_fball_set_local(&x->piece->fin, &x->piece->fin, ctx) == ADF_OK);
    CHECK(!fmpz_is_one(x->piece->fin.d)); /* raw d cancels to 1 */
    CHECK(adf_qclass_reduce(y, x, 1, 53) == ADF_OK && adf_qclass_identical(y, global));
    fmpq_zero(s); fmpq_set_si(w, 2, 1); CHECK(union_member(y, s, w));
    fmpq_set_si(w, 1, 1); CHECK(!union_member(y, s, w));
    /* Review witness: [1/2,1] x (0 mod 3) contains (0,2) through the upper boundary.
       Its reduced value must still contain that point; (0,1) is absent. */
    arb_set_d(x->piece->inf, 0.75); mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, -2);
    adf_fball_set_global(&x->piece->fin, &x->piece->fin);
    CHECK(adf_qclass_reduce(y, x, 1, 53) == ADF_OK);
    fmpq_set_si(w, 2, 1); CHECK(union_member(x, s, w) && union_member(y, s, w));
    fmpq_set_si(w, 1, 1); CHECK(!union_member(x, s, w) && !union_member(y, s, w));
    /* The other half of the named witness meets it ONLY at the glued point. */
    arb_set_d(global->piece->inf, 0.25); mag_set_ui_2exp_si(arb_radref(global->piece->inf), 1, -2);
    fmpq_set_si(a->q, 2, 1);
    CHECK(adf_fball_set_center_radius(&global->piece->fin, a, N) == ADF_OK);
    fmpq_set_si(w, 2, 1); CHECK(union_member(global, s, w) && union_member(y, s, w));
    adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(global); adf_modctx_free(ctx);
    fmpq_clear(s); fmpq_clear(w); adf_rat_clear(a); adf_rat_clear(N);
}
static void remaining_preflight(void)
{
    adf_qclass_t x, y; fmpz_t A, H, d;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_clear(x);
    x->form = ADF_QCLASS_PIECES; x->len = 2; x->piece = flint_malloc(2*sizeof(*x->piece));
    adf_adele_init(x->piece); adf_adele_init(x->piece+1);
    arb_set_d(x->piece[0].inf, 0.5); mag_set_ui_2exp_si(arb_radref(x->piece[0].inf), 3, -1);
    fmpz_one(x->piece[0].fin.H);
    fmpz_init(A); fmpz_init(H); fmpz_init_set_ui(d, 1);
    fmpz_one(A); fmpz_mul_2exp(A, A, 2000); fmpz_sub_ui(A, A, 1);
    fmpz_one(H); fmpz_mul_2exp(H, H, 2001);
    arb_set_d(x->piece[1].inf, 0.75);
    CHECK(adf_fball_set_fmpz3(&x->piece[1].fin, A, H, d) == ADF_OK);
    CHECK(adf_qclass_is_canonical(x));
    /* First interval [-1,2] consumes all three construction slots. The second B=1
       exceeds remaining=0; its centre must not be copied into a fibre before LIMIT. */
    premature_fibre_copies = 0; watch_exact_set = 1; refused(y, x, 3, 53); watch_exact_set = 0;
    CHECK(premature_fibre_copies == 0);
    printf("remaining-count preflight: %lu premature fibre copies\n", premature_fibre_copies);
    adf_qclass_clear(x); adf_qclass_clear(y); fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}
static void debug_entry(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int st; pid_t child = fork(); CHECK(child >= 0);
    if (!child) {
        adf_qclass_t x, y; alarm(5); adf_qclass_init(x); adf_qclass_init(y);
        x->form = 99; (void) adf_qclass_reduce(y, x, 0, 53); _exit(0);
    }
    CHECK(waitpid(child, &st, 0) == child); CHECK(WIFSIGNALED(st) && WTERMSIG(st) == SIGABRT);
#endif
}

static int test_piece_cmp(const void *vp, const void *vq)
{
    const adf_adele_struct *p = vp, *q = vq; fmpq_t l, h, a, b; int c;
    fmpq_init(l); fmpq_init(h); fmpq_init(a); fmpq_init(b);
    ends(l, h, p->inf); ends(a, b, q->inf); c = fmpq_cmp(l, a);
    if (!c) c = fmpq_cmp(h, b);
    if (!c) c = fmpz_cmp(p->fin.H, q->fin.H);
    if (!c) c = fmpz_cmp(p->fin.A, q->fin.A);
    fmpq_clear(l); fmpq_clear(h); fmpq_clear(a); fmpq_clear(b); return c;
}
/* For additional printer fixtures, choose a tighter dyadic lift radius so Q1's successor
   is RD30 of the golden decimal radius. The printed decimal then matches the golden row.
   The seven original fixtures below continue to use their original reader-built lifts. */
static void tighter_golden_radius(arb_t x, const char *entry, const char *end)
{
    const char *r = strstr(entry, "+/-"); int decimal = 0;
    fmpq_t q; arf_t u, den; fmpz_t mant, exp; ulong v;
    if (!r || r >= end) return;
    fmpq_init(q); fmpz_one(fmpq_denref(q)); arf_init(u); arf_init(den);
    fmpz_init(mant); fmpz_init(exp); r += 3;
    while (*r == ' ') r++;
    for (; r < end && *r != ' ' && *r != ';'; r++) {
        if (*r == '.') { decimal = 1; continue; }
        CHECK(*r >= '0' && *r <= '9');
        fmpz_mul_ui(fmpq_numref(q), fmpq_numref(q), 10);
        fmpz_add_ui(fmpq_numref(q), fmpq_numref(q), (ulong) (*r-'0'));
        if (decimal) fmpz_mul_ui(fmpq_denref(q), fmpq_denref(q), 10);
    }
    fmpq_canonicalise(q); CHECK(fmpq_sgn(q) > 0);
    arf_set_fmpz(u, fmpq_numref(q)); arf_set_fmpz(den, fmpq_denref(q));
    arf_div(u, u, den, 30, ARF_RND_FLOOR); arf_get_fmpz_2exp(mant, exp, u);
    fmpz_mul_2exp(mant, mant, 30-fmpz_bits(mant)); v = fmpz_get_ui(mant);
    fmpz_set(MAG_EXPREF(arb_radref(x)), ARF_EXPREF(u));
    if (v == (UWORD(1) << 29)) {
        MAG_MAN(arb_radref(x)) = (UWORD(1) << 30)-1;
        fmpz_sub_ui(MAG_EXPREF(arb_radref(x)), MAG_EXPREF(arb_radref(x)), 1);
    } else MAG_MAN(arb_radref(x)) = v-1;
    fmpq_clear(q); arf_clear(u); arf_clear(den); fmpz_clear(mant); fmpz_clear(exp);
}
static void golden_unions(void)
{
    golden_file *f; golden_error_t e; size_t i, n; ulong rows = 0;
    adf_qclass_t lift, reduced, family;
    adf_qclass_init(lift); adf_qclass_init(reduced); adf_qclass_init(family);
    CHECK(golden_open("tests/golden/qclass.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &e));
    for (i = 0; i < golden_count(f); i++) {
        const golden_record *r = golden_record_at(f, i); const char *p; char *printed;
        slong j, used = 0, keep; int tighter;
        if (r->is_status || strncmp(r->input, "union", 5)) continue;
        /* Midpoint 1 with radius 1/2 is legal stored spill, but cannot be a Q1 output:
           RN_p of an interval midpoint in [0,1] can round to 1 only from >=7/8 (p>=2),
           whence d<=1/4. Even Q1's margin cannot reach radius 1/2. */
        if (strstr(r->input, "(1 +/- 0.5")) continue;
        tighter = strstr(r->input, "+/-") && !strstr(r->input, "0.125");
        adf_qclass_clear(family); family->form = ADF_QCLASS_PIECES; family->len = 0;
        family->piece = flint_malloc(16*sizeof(*family->piece));
        p = r->input+6;
        while ((p = strchr(p, '(')) != NULL) {
            const char *end = strchr(p, ')'); CHECK(end);
            CHECK(adf_adele_set_str(lift->piece, p, (size_t) (end-p+1), 128, NULL) == ADF_OK);
            if (tighter) tighter_golden_radius(lift->piece->inf, p, end);
            CHECK(adf_qclass_reduce(reduced, lift, 8, 128) == ADF_OK);
            for (j = 0; j < reduced->len; j++) {
                CHECK(used < 16); adf_adele_init(family->piece+used);
                adf_adele_set(family->piece+used, reduced->piece+j); used++;
            }
            p = end+1;
        }
        family->len = used; CHECK(used > 0);
        qsort(family->piece, (size_t) used, sizeof(*family->piece), test_piece_cmp);
        keep = 1;
        for (j = 1; j < used; j++) if (test_piece_cmp(family->piece+keep-1, family->piece+j)) {
            if (keep != j) adf_adele_swap(family->piece+keep, family->piece+j);
            keep++;
        }
        for (j = keep; j < used; j++) adf_adele_clear(family->piece+j);
        family->len = keep; CHECK(adf_qclass_is_canonical(family));
        printed = adf_qclass_get_str(&n, family, 2);
        CHECK(printed && n == r->expected_len && !memcmp(printed, r->expected, n));
        CHECK(adf_qclass_set_str(lift, printed, n, 128, NULL) == ADF_UNSUPPORTED);
        adf_str_free(printed); rows++;
    }
    CHECK(rows == 12); printf("union golden rows built by reduce: %lu\n", rows);
    golden_close(f); adf_qclass_clear(lift); adf_qclass_clear(reduced); adf_qclass_clear(family);
}
static void printer_order(void)
{
    adf_qclass_t x, y; slong i; size_t n; char *s;
    const char *want = "union((0.62 +/- 0.26 ; 0 mod 1), (0.5 +/- 0.13 ; 0 mod 1)) + Q";
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_clear(x);
    x->form = ADF_QCLASS_PIECES; x->len = 2; x->piece = flint_malloc(2*sizeof(*x->piece));
    for (i = 0; i < 2; i++) {
        adf_adele_init(x->piece+i); fmpz_one(x->piece[i].fin.H);
        MAG_MAN(arb_radref(x->piece[i].inf)) = (UWORD(1) << 30)-1;
        fmpz_set_si(MAG_EXPREF(arb_radref(x->piece[i].inf)), i-3);
    }
    arf_set_d(arb_midref(x->piece[0].inf), 0.5); arf_set_d(arb_midref(x->piece[1].inf), 0.625);
    CHECK(adf_qclass_is_canonical(x)); CHECK(adf_qclass_reduce(y, x, 2, 128) == ADF_OK);
    /* Q1 yields exact radii 1/8 and 1/4. Stored lower ends tie at 3/8;
       upper ends put 0.5 first. PRINTED lower ends are 0.37 and 0.36, reversing that order. */
    CHECK(y->len == 2 && arf_get_d(arb_midref(y->piece[0].inf), ARF_RND_NEAR) == 0.5);
    s = adf_qclass_get_str(&n, y, 2); CHECK(s && n == strlen(want) && !strcmp(s, want));
    adf_str_free(s); adf_qclass_clear(x); adf_qclass_clear(y);
}

/* Review q-review3 (docs/reviews/m3/review-qclass-reduce.md, R1 and R2), lane q-repair1.
   Own algorithm R count, docs/api-3.md:164-165 (step 3): for l_j < h_j the integers
   floor(l_j) <= n < ceil(h_j); for l_j = h_j the one n = floor(l_j). Fibres a_j = a + j A/B,
   0 <= j < B, docs/api-3.md:161-163. Exact fmpq; the count is before deduplication (:172-173). */
static slong own_count(const adf_adele_struct *x)
{
    fmpq_t lo, hi, t, l, h; adf_rat_t a, N; fmpz_t f, c, total; slong j, B, K;
    fmpq_init(lo); fmpq_init(hi); fmpq_init(t); fmpq_init(l); fmpq_init(h);
    adf_rat_init(a); adf_rat_init(N); fmpz_init(f); fmpz_init(c); fmpz_init(total);
    ends(lo, hi, x->inf); adf_fball_get_center(a, &x->fin); adf_fball_get_radius(N, &x->fin);
    CHECK(fmpz_fits_si(fmpq_denref(N->q))); B = fmpz_get_si(fmpq_denref(N->q));
    for (j = 0; j < B; j++) {
        fmpq_mul_si(t, N->q, j); fmpq_add(t, t, a->q); fmpq_sub(l, lo, t); fmpq_sub(h, hi, t);
        if (fmpq_equal(l, h)) { fmpz_add_ui(total, total, 1); continue; }
        fmpz_fdiv_q(f, fmpq_numref(l), fmpq_denref(l)); fmpz_cdiv_q(c, fmpq_numref(h), fmpq_denref(h));
        fmpz_add(total, total, c); fmpz_sub(total, total, f);
    }
    CHECK(fmpz_fits_si(total)); K = fmpz_get_si(total);
    fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(t); fmpq_clear(l); fmpq_clear(h);
    adf_rat_clear(a); adf_rat_clear(N); fmpz_clear(f); fmpz_clear(c); fmpz_clear(total);
    return K;
}
/* Every end point (lo ; a_j), (hi ; a_j) of every fibre of the lift x lies in x and in y. */
static void input_ends_in(const adf_qclass_t y, const adf_qclass_t x)
{
    fmpq_t lo, hi, w; adf_rat_t a, N; slong j, B;
    fmpq_init(lo); fmpq_init(hi); fmpq_init(w); adf_rat_init(a); adf_rat_init(N);
    ends(lo, hi, x->piece->inf);
    adf_fball_get_center(a, &x->piece->fin); adf_fball_get_radius(N, &x->piece->fin);
    B = fmpz_get_si(fmpq_denref(N->q));
    for (j = 0; j < B; j++) {
        fmpq_mul_si(w, N->q, j); fmpq_add(w, w, a->q);
        CHECK(union_member(x, lo, w) && union_member(x, hi, w));
        CHECK(union_member(y, lo, w)); CHECK(union_member(y, hi, w));
    }
    fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(w); adf_rat_clear(a); adf_rat_clear(N);
}
/* R1 family (tests/ref/vectors/q-repair1/tiny_frac.jsonl): upper end h_j = integer + 2^-200,
   + 2^-101, + 2^-99 at a dyadic fibre centre. vectors() above checks the oracle's stored lists;
   here the count is our own: OK at the own count K, LIMIT (y untouched) at K-1, ends enclosed. */
static void tiny_frac(const char *path)
{
    jsonl_file *f; jsonl_error_t e; size_t i; adf_qclass_t x, y; slong total = 0;
    adf_qclass_init(x); adf_qclass_init(y);
    arb_set_si(y->piece->inf, 7); adf_fball_set_si(&y->piece->fin, -19);
    CHECK(jsonl_open(path, &f, &e));
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i); slong K, prec = integer(field(v, "prec"));
        input(x, v); CHECK(x->form == ADF_QCLASS_LIFT);
        K = own_count(x->piece); CHECK(K == integer(field(v, "raw"))); CHECK(K >= 2);
        refused(y, x, K-1, prec);
        CHECK(adf_qclass_reduce(y, x, K, prec) == ADF_OK); CHECK(y->len <= K);
        input_ends_in(y, x); total += K;
        adf_qclass_clear(y); adf_qclass_init(y); /* back to a LIFT for the next refusal */
        arb_set_si(y->piece->inf, 7); adf_fball_set_si(&y->piece->fin, -19);
    }
    printf("%s: %zu lifts, own count %ld pieces\n", path, jsonl_count(f), total);
    jsonl_close(f); adf_qclass_clear(x); adf_qclass_clear(y);
}
static void pow2(fmpq_t q, slong e) { fmpq_one(q); if (e >= 0) fmpq_mul_2exp(q, q, (ulong) e);
                                      else fmpq_div_2exp(q, q, (ulong) -e); }
/* Stored piece i of y has exactly the dyadic ends [L, U] and the finite triple (A, H, 1). */
static void piece_is(const adf_qclass_t y, slong i, const fmpq_t L, const fmpq_t U, slong A, slong H)
{
    fmpq_t lo, hi; fmpz_t a, h, d;
    fmpq_init(lo); fmpq_init(hi); fmpz_init(a); fmpz_init(h); fmpz_init(d);
    ends(lo, hi, y->piece[i].inf); CHECK(fmpq_equal(lo, L)); CHECK(fmpq_equal(hi, U));
    adf_fball_get_fmpz3(a, h, d, &y->piece[i].fin);
    CHECK(fmpz_equal_si(a, A) && fmpz_equal_si(h, H) && fmpz_is_one(d));
    fmpq_clear(lo); fmpq_clear(hi); fmpz_clear(a); fmpz_clear(h); fmpz_clear(d);
}
/* R1, the review's smallest input: [-2^-212, 2^-210] x {0} at prec 53. Algorithm R: l = -2^-212,
   h = 2^-210, n = -1, 0: [1 - 2^-212, 1] x {1} and [0, 2^-210] x {0}; count 2.
   Q1 (docs/api-3.md section 4): midpoints RN_53 are 2^-211 and 1; d = 2^-211 and 2^-212 are powers of two,
   so RU30(d) = d and the successor adds 2^-29 d: radii 2^-211 + 2^-240 and 2^-212 + 2^-241. */
static void r1_smallest(void)
{
    adf_qclass_t x, y, alias; fmpq_t L, U, t, s, w;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(alias);
    fmpq_init(L); fmpq_init(U); fmpq_init(t); fmpq_init(s); fmpq_init(w);
    arf_set_si_2exp_si(arb_midref(x->piece->inf), 3, -213);
    mag_set_ui_2exp_si(arb_radref(x->piece->inf), 5, -213);
    ends(L, U, x->piece->inf); pow2(t, -212); fmpq_neg(t, t); CHECK(fmpq_equal(L, t));
    pow2(t, -210); CHECK(fmpq_equal(U, t)); CHECK(adf_fball_is_exact(&x->piece->fin));
    CHECK(own_count(x->piece) == 2);
    arb_set_si(y->piece->inf, 7); adf_fball_set_si(&y->piece->fin, -19);
    refused(y, x, 1, 53);
    CHECK(adf_qclass_reduce(y, x, 2, 53) == ADF_OK); CHECK(y->form == ADF_QCLASS_PIECES);
    CHECK(y->len == 2 && adf_qclass_is_canonical(y));
    pow2(L, -240); fmpq_neg(L, L); pow2(U, -210); pow2(t, -240); fmpq_add(U, U, t);
    piece_is(y, 0, L, U, 0, 0);
    pow2(t, -212); pow2(s, -241); fmpq_add(t, t, s);
    fmpq_one(L); fmpq_sub(L, L, t); fmpq_one(U); fmpq_add(U, U, t);
    piece_is(y, 1, L, U, 1, 0);
    /* The input point (2^-210 ; 0) that F2 loses, and the other end (-2^-212 ; 0). */
    fmpq_zero(w); pow2(s, -210); CHECK(union_member(x, s, w) && union_member(y, s, w));
    pow2(s, -212); fmpq_neg(s, s); CHECK(union_member(x, s, w) && union_member(y, s, w));
    input_ends_in(y, x);
    adf_qclass_set(alias, x); refused(alias, alias, 1, 53);
    CHECK(adf_qclass_reduce(alias, alias, 2, 53) == ADF_OK && adf_qclass_identical(alias, y));
    adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(alias);
    fmpq_clear(L); fmpq_clear(U); fmpq_clear(t); fmpq_clear(s); fmpq_clear(w);
}
/* R2: a LIMIT of the second pass. [5/8, 7/8] x (1/3 + 2 Zhat) has count 1 (l = 7/24, h = 13/24),
   so the first pass passes any limit >= 1. At prec ADF_REAL_PREC_MAX the Q1 midpoint of 5/12 has a
   denominator above ADF_QCLASS_BITS_MAX and q_round refuses: LIMIT, and "On LIMIT y is untouched"
   (header of adf_qclass_reduce). y is a LIFT, so a premature y->form = PIECES is visible. */
static void r2_second_pass(void)
{
    adf_qclass_t x, y, alias; adf_rat_t a, N; fmpq_t lo, hi, m, d, t, rho, q;
    arf_t r; fmpz_t A, H, D; clock_t start; double sec;
    const slong ok_prec = 2097088;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(alias); adf_rat_init(a); adf_rat_init(N);
    fmpq_init(lo); fmpq_init(hi); fmpq_init(m); fmpq_init(d); fmpq_init(t); fmpq_init(rho); fmpq_init(q);
    arf_init(r); fmpz_init(A); fmpz_init(H); fmpz_init(D);
    arf_set_si_2exp_si(arb_midref(x->piece->inf), 3, -2);
    mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, -3);
    fmpq_set_si(a->q, 1, 3); fmpq_set_si(N->q, 2, 1);
    CHECK(adf_fball_set_center_radius(&x->piece->fin, a, N) == ADF_OK);
    ends(lo, hi, x->piece->inf); fmpq_set_si(q, 5, 8); CHECK(fmpq_equal(lo, q));
    fmpq_set_si(q, 7, 8); CHECK(fmpq_equal(hi, q)); CHECK(own_count(x->piece) == 1);
    /* A non-trivial LIFT: (7 +/- 2^-5 ; 1/6 + 5 Zhat). */
    arb_set_si(y->piece->inf, 7); mag_set_ui_2exp_si(arb_radref(y->piece->inf), 1, -5);
    fmpq_set_si(a->q, 1, 6); fmpq_set_si(N->q, 5, 1);
    CHECK(adf_fball_set_center_radius(&y->piece->fin, a, N) == ADF_OK);
    CHECK(y->form == ADF_QCLASS_LIFT && adf_qclass_is_canonical(y));
    start = clock(); refused(y, x, 10, ADF_REAL_PREC_MAX);
    adf_qclass_set(alias, x); refused(alias, alias, 10, ADF_REAL_PREC_MAX);
    CHECK(adf_qclass_identical(alias, x) && alias->form == ADF_QCLASS_LIFT);
    sec = (double) (clock()-start)/CLOCKS_PER_SEC; CHECK(sec < 5.0);
    printf("second-pass LIMIT at prec %ld: CPU %.3f s for two calls, guard 5 s\n",
           (long) ADF_REAL_PREC_MAX, sec);
    /* The same input 64 bits below the cap: OK, one piece [7/24, 13/24] x (0 + 2 Zhat), Q1 checked. */
    start = clock();
    CHECK(adf_qclass_reduce(y, x, 1, ok_prec) == ADF_OK);
    sec = (double) (clock()-start)/CLOCKS_PER_SEC; CHECK(sec < 5.0);
    printf("same input at prec %ld: OK, CPU %.3f s, guard 5 s\n", (long) ok_prec, sec);
    CHECK(y->form == ADF_QCLASS_PIECES && y->len == 1 && adf_qclass_is_canonical(y));
    adf_fball_get_fmpz3(A, H, D, &y->piece->fin);
    CHECK(fmpz_is_zero(A) && fmpz_equal_si(H, 2) && fmpz_is_one(D));
    /* Midpoint RN_p(5/12): at most p bits and |m - 5/12| <= ulp/2 = 2^(-2-p) (5/12 in [1/4,1/2),
       not dyadic, so this characterises the nearest p-bit number). */
    CHECK(arf_bits(arb_midref(y->piece->inf)) <= ok_prec);
    arf_get_fmpq(m, arb_midref(y->piece->inf)); fmpq_set_si(q, 5, 12); fmpq_sub(t, m, q);
    fmpq_abs(t, t); fmpq_mul_2exp(t, t, (ulong) ok_prec+2); CHECK(fmpq_cmp_ui(t, 1) <= 0);
    /* Q1 radius: d = max(m - 7/24, 13/24 - m), 0 <= rho - d <= 2^-28 d. */
    fmpq_set_si(lo, 7, 24); fmpq_set_si(hi, 13, 24);
    fmpq_sub(d, m, lo); fmpq_sub(t, hi, m); if (fmpq_cmp(t, d) > 0) fmpq_set(d, t);
    arf_set_mag(r, arb_radref(y->piece->inf)); arf_get_fmpq(rho, r);
    fmpq_sub(t, rho, d); CHECK(fmpq_sgn(t) >= 0);
    fmpq_mul_2exp(t, t, 28); CHECK(fmpq_cmp(t, d) <= 0);
    ends(m, t, y->piece->inf); CHECK(fmpq_cmp(m, lo) <= 0 && fmpq_cmp(t, hi) >= 0);
    input_ends_in(y, x);
    adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(alias); adf_rat_clear(a); adf_rat_clear(N);
    fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(m); fmpq_clear(d); fmpq_clear(t); fmpq_clear(rho);
    fmpq_clear(q); arf_clear(r); fmpz_clear(A); fmpz_clear(H); fmpz_clear(D);
}

int main(int argc, char **argv)
{
    const char *tiny = "tests/ref/vectors/q-repair1/tiny_frac.jsonl";
    if (argc > 1 && !strncmp(argv[1], "repair-", 7)) { /* one part of lane q-repair1 alone */
        if (!strcmp(argv[1], "repair-vectors")) vectors(tiny);
        else if (!strcmp(argv[1], "repair-count")) tiny_frac(tiny);
        else if (!strcmp(argv[1], "repair-r1")) r1_smallest();
        else if (!strcmp(argv[1], "repair-r2")) r2_second_pass();
        else CHECK(0);
        printf("test_qclass_reduce %s: %lu checks\n", argv[1], checks); flint_cleanup(); return 0;
    }
    vectors("tests/ref/vectors/q-slice2/integer.jsonl");
    if (argc == 1 || !strcmp(argv[1], "fractional"))
        vectors("tests/ref/vectors/q-slice2/fractional.jsonl");
    if (argc == 1 || !strcmp(argv[1], "fractional"))
        vectors("tests/ref/vectors/q-slice2/spill.jsonl");
    vectors(tiny); tiny_frac(tiny); r1_smallest(); r2_second_pass();
    printer(); golden_unions(); printer_order(); bounds(); remaining_preflight();
    local_and_glue(); debug_entry();
    printf("test_qclass_reduce: %lu checks\n", checks); flint_cleanup(); return 0;
}
