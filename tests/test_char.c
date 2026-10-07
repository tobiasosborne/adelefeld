/* Slice a: independent oracle vectors and all 17 CV-60 golden rows.
   docs/api-3c.md 1-6, P3/P4; refs/src/flint-3.0.1/acb_dirichlet.rst:358-378. */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <flint/acb_dirichlet.h>
#include <flint/ulong_extras.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "support/jsonl.h"
#include "support/golden.h"

static long checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "test_char:%d: %s\n", __LINE__, #c); abort(); } } while (0)
#ifdef ADF_CHAR_WRAP_SETUP
static long setups, setup_fail_at, phase_calls, phase_fail_at;
static int phase_status, phase_widen_coordinate, sqrt_widen;
int __real_dirichlet_group_init(dirichlet_group_t G, ulong q);
int __wrap_dirichlet_group_init(dirichlet_group_t G, ulong q)
{
    setups++;
    if (setups == setup_fail_at) return 0;
    return __real_dirichlet_group_init(G, q);
}
int __real_adf_phase_get_acb(acb_t z, const fmpq_t theta, slong prec);
int __wrap_adf_phase_get_acb(acb_t z, const fmpq_t theta, slong prec)
{
    phase_calls++;
    if (phase_calls == phase_fail_at && phase_status != ADF_OK) return phase_status;
    int st = __real_adf_phase_get_acb(z, theta, prec);
    if (st == ADF_OK && phase_calls == phase_fail_at && phase_widen_coordinate)
        arb_add_error_2exp_si(phase_widen_coordinate == 1 ? acb_realref(z) : acb_imagref(z), -10);
    return st;
}
void __real_arb_sqrt_ui(arb_t z, ulong x, slong prec);
void __wrap_arb_sqrt_ui(arb_t z, ulong x, slong prec)
{
    __real_arb_sqrt_ui(z, x, prec);
    if (sqrt_widen) arb_add_error_2exp_si(z, -10);
}
#endif
static jsonl_error_t err;
static const jsonl_value *field(const jsonl_value *r, const char *key)
{
    const jsonl_value *v;
    CHECK(jsonl_field(r, key, &v, &err)); return v;
}
static const char *str(const jsonl_value *v)
{
    size_t len; const char *s = jsonl_string(v, &len, &err);
    CHECK(s != NULL); return s;
}
static ulong ui(const jsonl_value *v)
{
    const char *s = jsonl_int_text(v, &err); CHECK(s != NULL); return strtoul(s, NULL, 10);
}
static jsonl_file *vectors(const char *name, size_t count)
{
    char path[120]; jsonl_file *f;
    snprintf(path, sizeof(path), "tests/ref/vectors/c-slice1/%s.jsonl", name);
    CHECK(jsonl_open(path, &f, &err)); CHECK(jsonl_count(f) == count); return f;
}
static void snapshot(unsigned char *bytes, const adf_char_t x)
{
    memcpy(bytes, x, sizeof(adf_char_struct));
}
static void untouched(const unsigned char *bytes, const adf_char_t x)
{
    CHECK(memcmp(bytes, x, sizeof(adf_char_struct)) == 0);
}
/* FLINT allocation callbacks: refs/src/flint-3.0.1/memory.rst:16-24.
   Warm caches before observing owned lifecycle and call-local temporaries. */
static void *(*saved_alloc)(size_t);
static void *(*saved_calloc)(size_t, size_t);
static void *(*saved_realloc)(void *, size_t);
static void (*saved_free)(void *);
static long live_blocks, allocated_blocks;
static void *count_alloc(size_t n)
{
    void *p = saved_alloc(n); if (p) { live_blocks++; allocated_blocks++; } return p;
}
static void *count_calloc(size_t n, size_t size)
{
    void *p = saved_calloc(n, size); if (p) { live_blocks++; allocated_blocks++; } return p;
}
static void *count_realloc(void *p, size_t n)
{
    void *q = saved_realloc(p, n);
    if (!p && q) { live_blocks++; allocated_blocks++; }
    if (p && !n && !q) live_blocks--;
    return q;
}
static void count_free(void *p) { if (p) live_blocks--; saved_free(p); }
static void memory_cycle(const acb_t s)
{
    adf_char_t x, y; acb_t tau;
    adf_char_init(x); adf_char_init(y); acb_init(tau);
    CHECK(adf_char_set_conrey_acb(x, 5, 2, s) == ADF_OK);
    adf_char_set(y, x); adf_char_swap(x, y); adf_char_set(x, x); adf_char_swap(x, x);
    CHECK(adf_char_gauss_sum(tau, x, 53) == ADF_OK);
    adf_char_clear(x); adf_char_clear(y); acb_clear(tau);
}
static void memory(void)
{
    acb_t s; fmpz_t huge;
    acb_init(s); fmpz_init(huge); fmpz_one(huge);
    fmpz_mul_2exp(huge, huge, 2000); fmpz_add_ui(huge, huge, 1); arb_set_fmpz(acb_realref(s), huge);
    for (int i = 0; i < 10; i++) memory_cycle(s);
    __flint_get_memory_functions(&saved_alloc, &saved_calloc, &saved_realloc, &saved_free);
    live_blocks = 0; allocated_blocks = 0;
    __flint_set_memory_functions(count_alloc, count_calloc, count_realloc, count_free);
    for (int i = 0; i < 100; i++) { memory_cycle(s); CHECK(live_blocks == 0); }
    __flint_set_memory_functions(saved_alloc, saved_calloc, saved_realloc, saved_free);
    CHECK(allocated_blocks > 0); printf("memory: %ld FLINT blocks, %ld live after 100 cycles\n",
                                       allocated_blocks, live_blocks);
    acb_clear(s); fmpz_clear(huge);
}
static void life(void)
{
    adf_char_t x, y; acb_t s, t; fmpz_t huge;
    adf_char_init(x); adf_char_init(y); acb_init(s); acb_init(t); fmpz_init(huge);
    CHECK(adf_sizeof_char() == sizeof(adf_char_struct));
    CHECK(adf_alignof_char() == _Alignof(adf_char_struct));
    CHECK(sizeof(adf_char_struct) == 120 && _Alignof(adf_char_struct) == 8);
    CHECK(offsetof(adf_char_struct, s) == 24);
    CHECK(offsetof(adf_char_struct, q) == 0 && offsetof(adf_char_struct, n) == sizeof(ulong));
    CHECK(offsetof(adf_char_struct, parity) == 2*sizeof(ulong));
    CHECK(adf_char_is_canonical(x));
    CHECK(x->q == 1 && x->n == 1 && x->parity == 0 && acb_is_zero(x->s));
    acb_set_si(s, 3); arb_add_error_2exp_si(acb_realref(s), -2000);
    /* Exercise an owned heap midpoint as well as a radius. */
    fmpz_one(huge); fmpz_mul_2exp(huge, huge, 2000); fmpz_add_ui(huge, huge, 1);
    arb_set_fmpz(acb_imagref(s), huge);
    acb_set(x->s, s); adf_char_set(y, x); CHECK(adf_char_identical(x, y));
    acb_zero(x->s); CHECK(!adf_char_identical(x, y)); CHECK(acb_equal(y->s, s));
    adf_char_swap(x, y); CHECK(acb_equal(x->s, s) && acb_is_zero(y->s));
    adf_char_set(x, x); CHECK(acb_equal(x->s, s));
    adf_char_swap(x, x); CHECK(acb_equal(x->s, s));
    adf_char_get_s(t, x); CHECK(acb_equal(s, t));
    CHECK(adf_char_get_conductor(x) == 1 && adf_char_get_label(x) == 1);
    CHECK(adf_char_get_parity(x) == 0);
    x->q = 0; CHECK(!adf_char_is_canonical(x)); x->q = 1;
    x->n = 0; CHECK(!adf_char_is_canonical(x)); x->n = 1;
    x->parity = 1; CHECK(!adf_char_is_canonical(x)); x->parity = 0;
    acb_indeterminate(x->s); CHECK(!adf_char_is_canonical(x)); acb_zero(x->s);
    x->q = 16; x->n = 9; CHECK(!adf_char_is_canonical(x)); x->q = 1; x->n = 1;
    /* 65539 is prime; a nonprincipal character is primitive above the algorithm cap. */
    x->q = 65539; x->n = 2;
    dirichlet_group_t G; dirichlet_char_t c;
    CHECK(dirichlet_group_init(G, x->q)); dirichlet_char_init(c, G);
    dirichlet_char_log(c, G, x->n); x->parity = dirichlet_parity_char(G, c);
    CHECK(adf_char_is_canonical(x));
    dirichlet_char_clear(c); dirichlet_group_clear(G); x->q = 1; x->n = 1; x->parity = 0;
    adf_char_clear(x); adf_char_clear(y); acb_clear(s); acb_clear(t); fmpz_clear(huge);
}
static void constructors(void)
{
    jsonl_file *f = vectors("lower", 1966); adf_char_t x; acb_t s, t;
    unsigned char bytes[sizeof(adf_char_struct)]; ulong order;
    adf_char_init(x); acb_init(s); acb_init(t);
    acb_set_si(s, -3); arb_set_si(acb_imagref(s), 2);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *r = jsonl_record(f, i), *v = field(r, "lower");
        ulong q = ui(field(r, "q")), n = ui(field(r, "n"));
        CHECK(adf_char_set_conrey_acb(x, q, n, s) == ADF_OK);
        CHECK(x->q == ui(jsonl_at(v, 0, &err)) && x->n == ui(jsonl_at(v, 1, &err)));
        CHECK(x->parity == (int) ui(jsonl_at(v, 2, &err)));
        CHECK(adf_char_get_order(&order, x) == ADF_OK && order == ui(jsonl_at(v, 3, &err)));
        CHECK(acb_equal(x->s, s)); CHECK(adf_char_is_canonical(x));
        CHECK(adf_char_set_conrey(x, q, n+2*q) == ADF_OK && acb_is_zero(x->s));
    }
    CHECK(adf_char_set_conrey(x, 1, 0) == ADF_OK);
    CHECK(adf_char_set_conrey(x, 1, UWORD_MAX) == ADF_OK);
    CHECK(adf_char_set_conrey(x, 65536, 1) == ADF_OK && x->q == 1);
    CHECK(adf_char_set_conrey(x, 5, 2) == ADF_OK); snapshot(bytes, x);
    CHECK(adf_char_set_conrey(x, 0, 1) == ADF_DOMAIN); untouched(bytes, x);
    CHECK(adf_char_set_conrey(x, 6, 0) == ADF_DOMAIN); untouched(bytes, x);
    CHECK(adf_char_set_conrey(x, 6, 2) == ADF_DOMAIN); untouched(bytes, x);
    CHECK(adf_char_set_conrey(x, 65538, 2) == ADF_DOMAIN); untouched(bytes, x);
    struct timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a);
#ifdef ADF_CHAR_WRAP_SETUP
    long before = setups;
#endif
    CHECK(adf_char_set_conrey(x, 65537, 1) == ADF_LIMIT);
    clock_gettime(CLOCK_MONOTONIC, &b); untouched(bytes, x);
    double elapsed = b.tv_sec-a.tv_sec + 1e-9*(b.tv_nsec-a.tv_nsec);
    CHECK(elapsed < 0.010); printf("cap refusal %.9f seconds\n", elapsed);
#ifdef ADF_CHAR_WRAP_SETUP
    CHECK(setups == before);
#endif
    acb_indeterminate(t);
    CHECK(adf_char_set_conrey_acb(x, 5, 2, t) == ADF_DOMAIN); untouched(bytes, x);
    CHECK(adf_char_set_conrey_acb(x, 65537, 1, t) == ADF_DOMAIN); untouched(bytes, x);
    CHECK(adf_char_set_s(x, t) == ADF_DOMAIN); untouched(bytes, x);
    CHECK(adf_char_set_s(x, s) == ADF_OK && acb_equal(x->s, s));
    adf_char_get_s(t, x); CHECK(acb_equal(s, t));
    x->q = 65539; order = 123;
    CHECK(adf_char_get_order(&order, x) == ADF_LIMIT && order == 123);
    x->q = 5;
    acb_clear(s); acb_clear(t); adf_char_clear(x); jsonl_close(f);
}
static void operand(fmpz_t a, const jsonl_value *v, ulong C)
{
    const char *s;
    if (jsonl_is(v, JSONL_INT)) s = jsonl_int_text(v, &err); else s = str(v);
    if (!strcmp(s, "C")) fmpz_set_ui(a, C);
    else if (!strcmp(s, "-C")) { fmpz_set_ui(a, C); fmpz_neg(a, a); }
    else if (!strcmp(s, "2C")) fmpz_set_ui(a, 2*C);
    else if (!strcmp(s, "-2C")) { fmpz_set_ui(a, 2*C); fmpz_neg(a, a); }
    else CHECK(fmpz_set_str(a, s, 10) == 0);
}
static void phases(void)
{
    jsonl_file *f = vectors("phases", 285), *inputs = vectors("inputs", 1); adf_char_t x;
    const jsonl_value *operands = field(jsonl_record(inputs, 0), "a");
    CHECK(jsonl_size(operands) == 40);
    fmpz_t a; fmpq_t t, expected; acb_t z, want; int zero;
    adf_char_init(x); fmpz_init(a); fmpq_init(t); fmpq_init(expected); acb_init(z); acb_init(want);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *r = jsonl_record(f, i), *v = field(r, "phases");
        ulong C = ui(field(r, "C")), n = ui(field(r, "n"));
        CHECK(adf_char_set_conrey(x, C, n) == ADF_OK); CHECK(jsonl_size(v) == 40);
        for (size_t j = 0; j < 40; j++) {
            const char *s = str(jsonl_at(v, j, &err)); operand(a, jsonl_at(operands, j, &err), C);
            zero = -1; fmpq_set_si(t, 7, 8);
            CHECK(adf_char_chi_phase(&zero, t, x, a) == ADF_OK);
            if (!strcmp(s, "zero")) { CHECK(zero == 1 && fmpq_is_zero(t)); acb_zero(want); }
            else {
                CHECK(fmpq_set_str(expected, s, 10) == 0); fmpq_canonicalise(expected);
                CHECK(zero == 0 && fmpq_equal(t, expected));
                CHECK(adf_phase_get_acb(want, expected, 256) == ADF_OK);
            }
            CHECK(adf_char_chi(z, x, a, 128) == ADF_OK); CHECK(acb_overlaps(z, want));
            if (zero) CHECK(acb_is_zero(z));
        }
    }
    CHECK(adf_char_set_conrey(x, 5, 4) == ADF_OK); fmpz_set_si(a, 2);
    CHECK(adf_char_chi_phase(&zero, t, x, a) == ADF_OK && zero == 0 && fmpq_equal_si(t, 0) == 0);
    fmpq_set_si(expected, 1, 2); CHECK(fmpq_equal(t, expected));
    CHECK(adf_char_chi(z, x, a, 2) == ADF_OK && acb_equal_si(z, -1));
    CHECK(adf_char_chi(z, x, a, 53) == ADF_OK && acb_equal_si(z, -1));
    CHECK(adf_char_chi(z, x, a, ADF_REAL_PREC_MAX) == ADF_OK && acb_equal_si(z, -1));
    acb_set_si(z, 17); x->q = 0;
    CHECK(adf_char_chi(z, x, a, ADF_REAL_PREC_MAX+1) == ADF_LIMIT && acb_equal_si(z, 17));
    x->q = 65539; zero = 17; fmpq_set_si(t, 7, 8);
    CHECK(adf_char_chi_phase(&zero, t, x, a) == ADF_LIMIT && zero == 17);
    fmpq_set_si(expected, 7, 8); CHECK(fmpq_equal(t, expected));
    CHECK(adf_char_chi(z, x, a, 53) == ADF_LIMIT && acb_equal_si(z, 17));
    x->q = 5;
    CHECK(adf_char_set_conrey(x, ADF_CHAR_MOD_MAX, 5) == ADF_OK && x->q == ADF_CHAR_MOD_MAX);
    fmpz_one(a); zero = -1;
    CHECK(adf_char_chi_phase(&zero, t, x, a) == ADF_OK && zero == 0 && fmpq_is_zero(t));
    CHECK(adf_char_chi(z, x, a, 53) == ADF_OK && acb_is_one(z));
    ulong order; CHECK(adf_char_get_order(&order, x) == ADF_OK && order == 16384);
    fmpz_zero(a);
    CHECK(adf_char_chi_phase(&zero, t, x, a) == ADF_OK && zero == 1 && fmpq_is_zero(t));
    adf_char_clear(x); fmpz_clear(a); fmpq_clear(t); fmpq_clear(expected);
    acb_clear(z); acb_clear(want); jsonl_close(f); jsonl_close(inputs);
}
static void radii(const acb_t z, ulong C, slong p, ulong factor)
{
    slong w = FLINT_MIN(ADF_REAL_PREC_MAX, FLINT_MAX(p, 2)+FLINT_CLOG2(C)+8);
    arf_t r, bound; arf_init(r); arf_init(bound);
    arf_set_ui(bound, factor*C); arf_mul_2exp_si(bound, bound, -w);
    for (int j = 0; j < 2; j++) {
        const arb_struct *v = j ? acb_imagref(z) : acb_realref(z);
        arf_set_mag(r, arb_radref(v)); CHECK(arf_cmp(r, bound) <= 0);
        if (p == 128) CHECK(mag_cmp_2exp_si(arb_radref(v), -60) < 0);
    }
    arf_clear(r); arf_clear(bound);
}
static void envelope(const acb_t z, const jsonl_value *v)
{
    fmpq_t mid, error, lo, hi; arf_t zl, zh;
    fmpq_init(mid); fmpq_init(error); fmpq_init(lo); fmpq_init(hi); arf_init(zl); arf_init(zh);
    CHECK(fmpq_set_str(error, str(jsonl_at(v, 2, &err)), 10) == 0); fmpq_canonicalise(error);
    for (int j = 0; j < 2; j++) {
        CHECK(fmpq_set_str(mid, str(jsonl_at(v, j, &err)), 10) == 0); fmpq_canonicalise(mid);
        fmpq_sub(lo, mid, error); fmpq_add(hi, mid, error);
        const arb_struct *part = j ? acb_imagref(z) : acb_realref(z);
        arb_get_interval_arf(zl, zh, part, ARF_PREC_EXACT);
        arf_get_fmpq(mid, zl); CHECK(fmpq_cmp(mid, hi) <= 0);
        arf_get_fmpq(mid, zh); CHECK(fmpq_cmp(mid, lo) >= 0);
    }
    fmpq_clear(mid); fmpq_clear(error); fmpq_clear(lo); fmpq_clear(hi); arf_clear(zl); arf_clear(zh);
}
static void reference(acb_t z, const adf_char_t x)
{
    dirichlet_group_t G; dirichlet_char_t c;
    CHECK(dirichlet_group_init(G, x->q)); dirichlet_char_init(c, G);
    dirichlet_char_log(c, G, x->n); acb_dirichlet_gauss_sum(z, G, c, 256);
    dirichlet_char_clear(c); dirichlet_group_clear(G);
}
static void gauss(void)
{
    jsonl_file *f = vectors("gauss", 45); adf_char_t x, conjugate;
    acb_t tau, W, ref, tc, wc, product; arb_t norm; slong precisions[] = {2, 53, 128};
    adf_char_init(x); adf_char_init(conjugate); acb_init(tau); acb_init(W); acb_init(ref);
    acb_init(tc); acb_init(wc); acb_init(product); arb_init(norm);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *r = jsonl_record(f, i);
        ulong C = ui(field(r, "C")), n = ui(field(r, "n"));
        CHECK(adf_char_set_conrey(x, C, n) == ADF_OK);
        CHECK(adf_char_set_conrey(conjugate, C, C == 1 ? 1 : n_invmod(n, C)) == ADF_OK);
        reference(ref, x);
        for (size_t j = 0; j < 3; j++) {
            slong p = precisions[j];
            CHECK(adf_char_gauss_sum(tau, x, p) == ADF_OK);
            CHECK(adf_char_root_number(W, x, p) == ADF_OK);
            CHECK(acb_overlaps(tau, ref)); envelope(tau, field(r, "tau")); envelope(W, field(r, "W"));
            radii(tau, C, p, 8); radii(W, C, p, 32);
            CHECK(adf_char_gauss_sum(tc, conjugate, p) == ADF_OK);
            CHECK(adf_char_root_number(wc, conjugate, p) == ADF_OK);
            acb_abs(norm, tau, 256); arb_sqr(norm, norm, 256); CHECK(arb_contains_si(norm, C));
            acb_mul(product, tau, tc, 256); CHECK(acb_contains_int(product));
            acb_set_si(ref, x->parity ? -(slong) C : (slong) C); CHECK(acb_contains(product, ref));
            acb_abs(norm, W, 256); CHECK(arb_contains_si(norm, 1));
            acb_mul(product, W, wc, 256);
            CHECK(arb_contains_si(acb_realref(product), 1) && arb_contains_zero(acb_imagref(product)));
            reference(ref, x);
        }
        acb_set_si(x->s, 9); arb_set_si(acb_imagref(x->s), -4);
        CHECK(adf_char_gauss_sum(tc, x, 128) == ADF_OK && acb_equal(tc, tau));
        CHECK(adf_char_root_number(wc, x, 128) == ADF_OK && acb_equal(wc, W));
    }
    CHECK(adf_char_set_conrey(x, 8, 7) == ADF_OK && x->q == 4 && x->n == 3);
    CHECK(adf_char_gauss_sum(tau, x, 128) == ADF_OK);
    acb_zero(ref); arb_set_si(acb_imagref(ref), 2); CHECK(acb_equal(tau, ref));
    CHECK(adf_char_set_conrey(x, 1, 1) == ADF_OK);
    CHECK(adf_char_gauss_sum(tau, x, ADF_REAL_PREC_MAX) == ADF_OK && acb_is_one(tau));
    CHECK(adf_char_root_number(W, x, ADF_REAL_PREC_MAX) == ADF_OK && acb_is_one(W));
    x->q = 0; acb_set_si(tau, 19); acb_set_si(W, 23);
    CHECK(adf_char_gauss_sum(tau, x, ADF_REAL_PREC_MAX+1) == ADF_LIMIT && acb_equal_si(tau, 19));
    CHECK(adf_char_root_number(W, x, ADF_REAL_PREC_MAX+1) == ADF_LIMIT && acb_equal_si(W, 23));
    x->q = 65537;
    CHECK(adf_char_gauss_sum(tau, x, 128) == ADF_LIMIT && acb_equal_si(tau, 19));
    CHECK(adf_char_root_number(W, x, 128) == ADF_LIMIT && acb_equal_si(W, 23));
    x->q = 1; adf_char_clear(x); adf_char_clear(conjugate);
    acb_clear(tau); acb_clear(W); acb_clear(ref); acb_clear(tc); acb_clear(wc); acb_clear(product);
    arb_clear(norm); jsonl_close(f);
}
/* The numerical conductor cap is inclusive, including a full direct sum at the boundary. */
static void boundary(void)
{
    adf_char_t x; acb_t tau, W; arb_t norm;
    adf_char_init(x); acb_init(tau); acb_init(W); arb_init(norm);
    CHECK(adf_char_set_conrey(x, ADF_CHAR_MOD_MAX, 5) == ADF_OK && x->q == ADF_CHAR_MOD_MAX);
    CHECK(adf_char_gauss_sum(tau, x, 2) == ADF_OK); radii(tau, x->q, 2, 8);
    CHECK(adf_char_root_number(W, x, 2) == ADF_OK); radii(W, x->q, 2, 32);
    acb_abs(norm, tau, 256); arb_sqr(norm, norm, 256); CHECK(arb_contains_si(norm, ADF_CHAR_MOD_MAX));
    acb_abs(norm, W, 256); CHECK(arb_contains_si(norm, 1));
    adf_char_clear(x); acb_clear(tau); acb_clear(W); arb_clear(norm);
}
/* Certification at the full precision cap may fail, but never changes the output. */
static void failures(void)
{
    adf_char_t x; acb_t z; fmpz_t a; fmpq_t theta; int zero; ulong order;
    unsigned char bytes[sizeof(adf_char_struct)];
    adf_char_init(x); acb_init(z); fmpz_init(a); fmpq_init(theta);
    (void)zero; (void)order; (void)bytes;
    CHECK(adf_char_set_conrey(x, 4, 3) == ADF_OK);
    CHECK(adf_char_gauss_sum(z, x, ADF_REAL_PREC_MAX) == ADF_OK);
    CHECK(arb_is_zero(acb_realref(z)) && arb_equal_si(acb_imagref(z), 2));
    CHECK(adf_char_root_number(z, x, ADF_REAL_PREC_MAX) == ADF_OK && acb_is_one(z));
    CHECK(adf_char_set_conrey(x, 3, 2) == ADF_OK); acb_set_si(z, 37);
    /* First noncardinal term is E(1/3); the phase API cannot certify its full-cap width. */
    CHECK(adf_char_gauss_sum(z, x, ADF_REAL_PREC_MAX) == ADF_NOT_DETERMINED);
    CHECK(acb_equal_si(z, 37));
    CHECK(adf_char_root_number(z, x, ADF_REAL_PREC_MAX) == ADF_NOT_DETERMINED);
    CHECK(acb_equal_si(z, 37));
#ifdef ADF_CHAR_WRAP_SETUP
    CHECK(adf_char_set_conrey(x, 1, 1) == ADF_OK); snapshot(bytes, x);
    setup_fail_at = setups+1;
    CHECK(adf_char_set_conrey(x, 16, 9) == ADF_UNSUPPORTED); untouched(bytes, x);
    setup_fail_at = setups+2;
    CHECK(adf_char_set_conrey(x, 16, 9) == ADF_UNSUPPORTED); untouched(bytes, x);
    setup_fail_at = 0; CHECK(adf_char_set_conrey(x, 5, 2) == ADF_OK);
#ifdef ADF_CHECK_INVARIANTS
    const long inv_setups = 1;
#else
    const long inv_setups = 0;
#endif
    setup_fail_at = setups+1+inv_setups; order = 99;
    CHECK(adf_char_get_order(&order, x) == ADF_UNSUPPORTED && order == 99);
    setup_fail_at = setups+1+inv_setups; zero = 99; fmpq_set_si(theta, 3, 4);
    CHECK(adf_char_chi_phase(&zero, theta, x, a) == ADF_UNSUPPORTED && zero == 99);
    CHECK(fmpq_cmp_si(theta, 0) > 0 && fmpz_equal_ui(fmpq_denref(theta), 4));
    setup_fail_at = setups+1+inv_setups;
    CHECK(adf_char_gauss_sum(z, x, 128) == ADF_UNSUPPORTED && acb_equal_si(z, 37));
    setup_fail_at = setups+1+2*inv_setups;
    CHECK(adf_char_chi(z, x, a, 128) == ADF_UNSUPPORTED && acb_equal_si(z, 37));
    setup_fail_at = setups+1+2*inv_setups;
    CHECK(adf_char_root_number(z, x, 128) == ADF_UNSUPPORTED && acb_equal_si(z, 37));
    setup_fail_at = 0;
    long cap_setups = setups; snapshot(bytes, x);
    CHECK(adf_char_set_conrey_acb(x, 65537, 1, z) == ADF_LIMIT); untouched(bytes, x);
    const char *cap_text = "char(q=65537, n=1, s=(0) + (0)*i)";
    CHECK(adf_char_set_str(x, cap_text, strlen(cap_text), 128, NULL) == ADF_LIMIT); untouched(bytes, x);
    CHECK(setups == cap_setups);
    for (int st = ADF_NOT_DETERMINED; st <= ADF_LIMIT; st += ADF_LIMIT-ADF_NOT_DETERMINED) {
        phase_status = st; phase_fail_at = phase_calls+1; fmpz_set_ui(a, 2);
        CHECK(adf_char_chi(z, x, a, 128) == st && acb_equal_si(z, 37));
        phase_fail_at = phase_calls+2;
        CHECK(adf_char_gauss_sum(z, x, 128) == st && acb_equal_si(z, 37));
        phase_fail_at = phase_calls+2;
        CHECK(adf_char_root_number(z, x, 128) == st && acb_equal_si(z, 37));
    }
    phase_fail_at = 0;
    /* Inject enclosing but overwide numerical intermediates to test BOTH radius predicates.
       This tests the final certificate, not a claim that the phase API produces such balls. */
    CHECK(adf_char_set_conrey(x, 4, 3) == ADF_OK); phase_status = ADF_OK;
    for (int coordinate = 1; coordinate <= 2; coordinate++) {
        phase_widen_coordinate = coordinate; phase_fail_at = phase_calls+1;
        CHECK(adf_char_gauss_sum(z, x, 128) == ADF_NOT_DETERMINED && acb_equal_si(z, 37));
    }
    phase_widen_coordinate = 0; phase_fail_at = 0; sqrt_widen = 1;
    CHECK(adf_char_root_number(z, x, 128) == ADF_NOT_DETERMINED && acb_equal_si(z, 37));
    sqrt_widen = 0;
#endif
    adf_char_clear(x); acb_clear(z); fmpz_clear(a); fmpq_clear(theta);
}
static int status(const char *s)
{
    for (int i = 0; i < ADF_STATUS_COUNT; i++) if (!strcmp(s, adf_status_str(i))) return i;
    CHECK(0); return -1;
}
static void texts(void)
{
    jsonl_file *f = vectors("texts", 32); adf_char_t x, y; unsigned char bytes[sizeof(adf_char_struct)];
    adf_char_init(x); adf_char_init(y);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *r = jsonl_record(f, i);
        size_t n; const char *s = jsonl_string(field(r, "input"), &n, &err);
        const char *expected = str(field(r, "expected")); snapshot(bytes, x);
        int st = adf_char_set_str(x, s, n, 128, NULL);
        if (*expected == '!') { CHECK(st == status(expected+1)); untouched(bytes, x); }
        else {
            CHECK(st == ADF_OK); CHECK(adf_char_set_str(y, expected, strlen(expected), 256, NULL) == ADF_OK);
            CHECK(x->q == y->q && x->n == y->n && x->parity == y->parity);
            const jsonl_value *bounds = field(r, "s_bounds"); fmpq_t endpoint; fmpq_init(endpoint);
            for (size_t k = 0; k < 4; k++) {
                CHECK(fmpq_set_str(endpoint, str(jsonl_at(bounds, k, &err)), 10) == 0);
                fmpq_canonicalise(endpoint);
                CHECK(arb_contains_fmpq(k < 2 ? acb_realref(x->s) : acb_imagref(x->s), endpoint));
            }
            fmpq_clear(endpoint);
            size_t len; char *printed = adf_char_get_str(&len, x, 20);
            CHECK(printed != NULL && len == strlen(printed));
            CHECK(adf_char_set_str(y, printed, len, 256, NULL) == ADF_OK);
            CHECK(acb_contains(y->s, x->s));
            if (acb_is_exact(x->s)) CHECK(adf_char_identical(x, y));
            adf_str_free(printed);
        }
    }
    const char *valid = "char(q=5, n=2, s=(0) + (0)*i)";
    CHECK(adf_char_set_str(x, valid, strlen(valid), 128, NULL) == ADF_OK); snapshot(bytes, x);
    adf_text_limits_t lim; adf_text_limits_default(&lim); lim.max_len = 1;
    CHECK(adf_char_set_str(x, NULL, 2, 128, &lim) == ADF_LIMIT); untouched(bytes, x);
    CHECK(adf_char_set_str(x, NULL, 1, ADF_REAL_PREC_MAX+1, NULL) == ADF_LIMIT); untouched(bytes, x);
    lim.max_len = 1000; lim.max_exp10 = 3;
    const char *bad[] = {"char(q=18446744073709551616, n=0, s=(1e4) + (0)*i)",
                        "char(q=0, n=0, s=(1e4) + (0)*i)",
                        "char(q=0, n=0, s=(1e4) + (0)*j)",
                        "char(q=18446744073709551616, n=0, s=(0) + (0)*i)",
                        "char(q=65538, n=2, s=(0) + (0)*i)"};
    int want[] = {ADF_LIMIT, ADF_LIMIT, ADF_PARSE, ADF_UNSUPPORTED, ADF_DOMAIN};
    for (size_t i = 0; i < 5; i++) {
        CHECK(adf_char_set_str(x, bad[i], strlen(bad[i]), 128, &lim) == want[i]); untouched(bytes, x);
    }
    CHECK(adf_char_set_str(x, valid, strlen(valid), 2, NULL) == ADF_OK);
    CHECK(adf_char_set_str(x, valid, strlen(valid), 53, NULL) == ADF_OK);
    CHECK(adf_char_set_str(x, valid, strlen(valid), ADF_REAL_PREC_MAX, NULL) == ADF_OK);
    arb_one(acb_realref(x->s)); arb_mul_2exp_si(acb_realref(x->s), acb_realref(x->s), 100002);
    size_t len = 9; CHECK(adf_char_get_str(&len, x, 20) == NULL && len == 0);
    adf_char_clear(x); adf_char_clear(y); jsonl_close(f);
}
static void goldens(void)
{
    golden_file *f; golden_error_t e; adf_char_t x; acb_t tau, W, reference_tau;
    adf_cadele_t parsed; arb_t denominator; char text[600]; size_t real_rows = 0;
    adf_char_init(x); acb_init(tau); acb_init(W); acb_init(reference_tau); adf_cadele_init(parsed);
    arb_init(denominator);
    CHECK(golden_open("tests/golden/gauss.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &e));
    CHECK(golden_count(f) == 17);
    for (size_t i = 0; i < golden_count(f); i++) {
        const golden_record *r = golden_record_at(f, i);
        const char *p = strstr(r->expected, " tau="), *end = strstr(r->expected, " W=");
        CHECK(p != NULL && end != NULL);
        CHECK(adf_char_set_str(x, r->input, r->input_len, 128, NULL) == ADF_OK);
        CHECK(x->parity == r->expected[2]-'0');
        CHECK(adf_char_gauss_sum(tau, x, 128) == ADF_OK);
        CHECK(adf_char_root_number(W, x, 128) == ADF_OK); reference(reference_tau, x);
        CHECK(acb_overlaps(tau, reference_tau));
        snprintf(text, sizeof(text), "(%.*s ; 0)", (int) (end-p-5), p+5);
        CHECK(adf_cadele_set_str(parsed, text, strlen(text), 256, NULL) == ADF_OK);
        CHECK(acb_overlaps(tau, parsed->inf));
        snprintf(text, sizeof(text), "(%s ; 0)", end+3);
        CHECK(adf_cadele_set_str(parsed, text, strlen(text), 256, NULL) == ADF_OK);
        CHECK(acb_overlaps(W, parsed->inf)); radii(tau, x->q, 128, 8); radii(W, x->q, 128, 32);
        arb_sqrt_ui(denominator, x->q, 256); acb_div_arb(reference_tau, reference_tau, denominator, 256);
        if (x->parity) acb_div_onei(reference_tau, reference_tau);
        CHECK(acb_overlaps(W, reference_tau));
        ulong order; CHECK(adf_char_get_order(&order, x) == ADF_OK);
        if (order == 2) {
            CHECK(arb_contains_si(acb_realref(W), 1) && arb_contains_zero(acb_imagref(W))); real_rows++;
        }
    }
    CHECK(real_rows == 8);
    golden_close(f); adf_char_clear(x); acb_clear(tau); acb_clear(W); acb_clear(reference_tau);
    adf_cadele_clear(parsed); arb_clear(denominator);
}
#ifdef ADF_CHECK_INVARIANTS
static void invariant_child(int which)
{
    adf_char_t x, y; fmpz_t a; fmpq_t phase; acb_t z; ulong order; int zero;
    adf_char_init(x); adf_char_init(y); fmpz_init(a); fmpq_init(phase); acb_init(z);
    x->parity = 1;
    switch (which) {
        case 0: adf_char_set(y, x); break;
        case 1: adf_char_swap(y, x); break;
        case 2: (void) adf_char_identical(x, y); break;
        case 3: (void) adf_char_get_conductor(x); break;
        case 4: (void) adf_char_get_label(x); break;
        case 5: (void) adf_char_get_parity(x); break;
        case 6: adf_char_get_s(z, x); break;
        case 7: (void) adf_char_get_order(&order, x); break;
        case 8: (void) adf_char_chi_phase(&zero, phase, x, a); break;
        case 9: (void) adf_char_chi(z, x, a, 53); break;
        case 10: (void) adf_char_gauss_sum(z, x, 53); break;
        case 11: (void) adf_char_root_number(z, x, 53); break;
        case 12: (void) adf_char_get_str((size_t *) &order, x, 20); break;
        case 13: (void) adf_char_set_s(x, z); break;
        case 14: adf_char_clear(x); break;
        case 15: x->parity = 0; (void) adf_char_chi(x->s, x, a, 53); break;
        case 16: x->parity = 0; (void) adf_char_gauss_sum(x->s, x, 53); break;
        case 17: x->parity = 0; (void) adf_char_root_number(x->s, x, 53); break;
        case 18: x->parity = 0; (void) adf_char_set_s(x, x->s); break;
        case 19: x->parity = 0; adf_char_get_s(x->s, x); break;
        case 20: x->parity = 0; (void) adf_char_set_conrey_acb(x, 1, 1, x->s); break;
        case 21: adf_char_swap(x, y); break;
        case 22: (void) adf_char_identical(y, x); break;
        case 23: adf_char_set(x, y); break;
    }
    /* Reached only if the entry check is missing. The clears are for tools/memcheck. */
    if (which != 14) adf_char_clear(x);
    adf_char_clear(y); fmpz_clear(a); fmpq_clear(phase); acb_clear(z);
    _exit(0);
}
static void invariants(void)
{
    fflush(NULL);
    for (int i = 0; i < 24; i++) {
        pid_t pid = fork(); CHECK(pid >= 0);
        if (!pid) { if (freopen("/dev/null", "w", stderr) == NULL) _exit(2); invariant_child(i); }
        int status_; CHECK(waitpid(pid, &status_, 0) == pid);
        CHECK(WIFSIGNALED(status_) && WTERMSIG(status_) == SIGABRT);
    }
}
#endif
int main(int argc, char **argv)
{
    flint_set_num_threads(1);
    if (argc == 1 || !strcmp(argv[1], "life")) life();
    if (argc == 1 || !strcmp(argv[1], "memory")) memory();
    if (argc == 1 || !strcmp(argv[1], "constructors")) constructors();
    if (argc == 1 || !strcmp(argv[1], "phases")) phases();
    if (argc == 1 || !strcmp(argv[1], "gauss")) gauss();
    if (argc == 1 || !strcmp(argv[1], "boundary")) boundary();
    if (argc == 1 || !strcmp(argv[1], "failures")) failures();
    if (argc == 1 || !strcmp(argv[1], "texts")) texts();
    if (argc == 1 || !strcmp(argv[1], "goldens")) goldens();
#ifdef ADF_CHECK_INVARIANTS
    if (argc == 1) invariants();
#endif
    flint_cleanup(); printf("test_char: %ld checks\n", checks); return 0;
}
