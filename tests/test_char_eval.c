/* Slice b: api-3c P1/P2/P3, Q4 and conventions 10.2. Independent oracle vectors. */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <flint/dirichlet.h>
#include <flint/ulong_extras.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "support/jsonl.h"
#include "support/golden.h"

static long checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "test_char_eval:%d: %s\n", __LINE__, #c); abort(); } } while (0)
/* No unsafe loader is allowed, including for texts failing at their last byte. */
int arb_load_str(arb_t x, const char *s)
{ (void) x; (void) s; CHECK(0 && "unsafe FLINT loader called"); return 1; }
#ifdef ADF_CHAR_EVAL_WRAP
static long setups, fail_setup, phase_calls;
static int fail_phase, wide_phase, sqrt_nonfinite;
int __real_dirichlet_group_init(dirichlet_group_t G, ulong q);
int __wrap_dirichlet_group_init(dirichlet_group_t G, ulong q)
{
    setups++;
    if (setups == fail_setup) return 0;
    return __real_dirichlet_group_init(G, q);
}
int __real_adf_phase_get_acb(acb_t z, const fmpq_t t, slong prec);
int __wrap_adf_phase_get_acb(acb_t z, const fmpq_t t, slong prec)
{
    phase_calls++;
    if (fail_phase) return fail_phase;
    if (wide_phase) {
        acb_one(z);
        if (wide_phase == 2) mag_set_ui_2exp_si(arb_radref(acb_realref(z)), 3, -55);
        else arb_add_error_2exp_si(acb_realref(z), 1);
        return ADF_OK;
    }
    return __real_adf_phase_get_acb(z, t, prec);
}
void __real_arb_sqrt_ui(arb_t z, ulong x, slong prec);
void __wrap_arb_sqrt_ui(arb_t z, ulong x, slong prec)
{
    if (sqrt_nonfinite) arb_indeterminate(z);
    else __real_arb_sqrt_ui(z, x, prec);
}
#endif
static jsonl_error_t err;
static const jsonl_value *field(const jsonl_value *v, const char *key)
{ const jsonl_value *p; CHECK(jsonl_field(v, key, &p, &err)); return p; }
static const jsonl_value *at(const jsonl_value *v, size_t i)
{ const jsonl_value *p = jsonl_at(v, i, &err); CHECK(p != NULL); return p; }
static const char *str(const jsonl_value *v)
{ size_t n; const char *s = jsonl_string(v, &n, &err); CHECK(s != NULL); return s; }
static ulong ui(const jsonl_value *v)
{ const char *s = jsonl_int_text(v, &err); CHECK(s != NULL); return strtoul(s, NULL, 10); }
static jsonl_file *vectors(const char *name, size_t n)
{
    char p[120]; jsonl_file *f;
    snprintf(p, sizeof(p), "tests/ref/vectors/c-slice2/%s.jsonl", name);
    CHECK(jsonl_open(p, &f, &err)); CHECK(jsonl_count(f) == n); return f;
}
static void rational(fmpq_t q, const jsonl_value *v)
{ CHECK(fmpq_set_str(q, str(v), 10) == 0); fmpq_canonicalise(q); }
static void sentinel(acb_t z)
{ acb_set_si(z, 17); arb_set_si(acb_imagref(z), -19); arb_add_error_2exp_si(acb_realref(z), -9); }
static void unchanged(const acb_struct *bytes, const acb_t z)
{ CHECK(memcmp(bytes, z, sizeof(*bytes)) == 0); }

/* Each endpoint excess <= 4*2^-p + 2^-28*(W/2 + 2*2^-p), api-3c 3 and psi.h.
   The oracle intervals have outward rounding to 60 decimal places. Use their inner endpoints
   for excess, and their outer endpoints for non-disjointness. At p<=128 their width is negligible. */
static void hull_check(const acb_t z, const jsonl_value *image, slong p)
{
    arf_t lo, hi; arb_t bound, excess, width, epsilon; fmpq_t q[4];
    arf_init(lo); arf_init(hi); arb_init(bound); arb_init(excess); arb_init(width); arb_init(epsilon);
    for (int j = 0; j < 4; j++) fmpq_init(q[j]);
    arb_one(epsilon); arb_mul_2exp_si(epsilon, epsilon, -p);
    for (int part = 0; part < 2; part++) {
        const jsonl_value *ext = field(image, "extrema");
        for (int j = 0; j < 4; j++) rational(q[j], at(at(ext, 2*part+j/2), j%2));
        arb_get_interval_arf(lo, hi, part ? acb_imagref(z) : acb_realref(z), ARF_PREC_EXACT);
        arb_set_fmpq(excess, q[1], 512); CHECK(!arb_lt(excess, (arb_set_arf(width, lo), width)));
        arb_set_fmpq(excess, q[2], 512); CHECK(!arb_gt(excess, (arb_set_arf(width, hi), width)));
        fmpq_sub(q[3], q[3], q[0]); arb_set_fmpq(width, q[3], 512);
        arb_mul_2exp_si(width, width, -1); arb_mul_2exp_si(bound, epsilon, 1);
        arb_add(bound, bound, width, 512); arb_mul_2exp_si(bound, bound, -28);
        arb_mul_2exp_si(width, epsilon, 2); arb_add(bound, bound, width, 512);
        arb_set_fmpq(excess, q[1], 512); arb_sub_arf(excess, excess, lo, 512);
        CHECK(arb_le(excess, bound));
        arb_set_arf(excess, hi); arb_set_fmpq(width, q[2], 512); arb_sub(excess, excess, width, 512);
        CHECK(arb_le(excess, bound));
    }
    for (int j = 0; j < 4; j++) fmpq_clear(q[j]);
    arf_clear(lo); arf_clear(hi); arb_clear(bound); arb_clear(excess); arb_clear(width); arb_clear(epsilon);
}
static void unit_vectors(void)
{
    jsonl_file *cases = vectors("cosets", 4973), *images = vectors("images", 23);
    jsonl_file *operands = vectors("operands", 1);
    const jsonl_value *cs = field(jsonl_record(operands, 0), "c");
    adf_char_t x; adf_ucoset_t u; acb_t z, point; fmpz_t c, N; fmpq_t phase; int zero;
    adf_char_init(x); adf_ucoset_init(u); acb_init(z); acb_init(point);
    fmpz_init(c); fmpz_init(N); fmpq_init(phase);
    for (size_t i = 0; i < jsonl_count(cases); i++) {
        const jsonl_value *v = jsonl_record(cases, i), *im;
        CHECK(adf_char_set_conrey(x, ui(at(v, 0)), ui(at(v, 1))) == ADF_OK);
        CHECK(x->q == ui(at(v, 0)) && x->n == ui(at(v, 1)));
        CHECK(fmpz_set_str(c, str(at(cs, ui(at(v, 2)))), 10) == 0); fmpz_set_ui(N, ui(at(v, 3)));
        CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
        CHECK(adf_char_chi_phase(&zero, phase, x, c) == ADF_OK && zero == (int) ui(at(v, 6)));
        im = jsonl_record(images, ui(at(v, 4)));
        for (int k = 0; k < 3; k++) {
            slong p = k == 2 ? 128 : k ? 53 : 2; acb_struct bytes;
            sentinel(z); bytes = *z;
            CHECK(adf_char_eval_ucoset_strict(z, x, u, p) ==
                  (ui(at(v, 5)) ? ADF_OK : ADF_NOT_DETERMINED));
            if (!ui(at(v, 5))) unchanged(&bytes, z);
            CHECK(adf_char_eval_ucoset(z, x, u, p) == ADF_OK); hull_check(z, im, p);
            const jsonl_value *ps = field(im, "phases");
            for (size_t j = 0; j < jsonl_size(ps); j++) {
                rational(phase, at(ps, j)); CHECK(adf_phase_get_acb(point, phase, 256) == ADF_OK);
                CHECK(acb_contains(z, point) || acb_equal(z, point));
            }
        }
        /* Unit cosets have no t; s is deliberately not applied by these functions. */
        acb_set_si(point, 7); arb_set_si(acb_imagref(point), 3);
        CHECK(adf_char_set_s(x, point) == ADF_OK);
        CHECK(adf_char_eval_ucoset(point, x, u, 128) == ADF_OK && acb_equal(z, point));
    }
    fmpq_clear(phase); fmpz_clear(c); fmpz_clear(N); acb_clear(z); acb_clear(point);
    adf_ucoset_clear(u); adf_char_clear(x); jsonl_close(cases); jsonl_close(images); jsonl_close(operands);
}
static void unit_edges(void)
{
    adf_char_t x; adf_ucoset_t u; acb_t z, point; fmpz_t c, N;
    adf_char_init(x); adf_ucoset_init(u); acb_init(z); acb_init(point); fmpz_init(c); fmpz_init(N);
    CHECK(adf_char_set_conrey(x, 3, 2) == ADF_OK);
    for (int sign = -1; sign <= 1; sign += 2) {
        fmpz_set_si(c, sign); fmpz_zero(N); CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
        acb_set_si(point, sign);
        CHECK(adf_char_eval_ucoset(z, x, u, ADF_REAL_PREC_MAX) == ADF_OK && acb_equal(z, point));
        CHECK(adf_char_eval_ucoset_strict(z, x, u, 0) == ADF_OK && acb_equal(z, point));
    }
    fmpz_set_ui(c, 3); fmpz_set_ui(N, 4); CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
    CHECK(adf_char_eval_ucoset(z, x, u, 128) == ADF_OK && arb_is_zero(acb_imagref(z)));
    CHECK(arb_contains_si(acb_realref(z), -1) && arb_contains_si(acb_realref(z), 1));
    fmpz_one(c); fmpz_mul_2exp(N, c, 2000); fmpz_mul_ui(N, N, 3);
    CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
    CHECK(adf_char_eval_ucoset_strict(z, x, u, 53) == ADF_OK && acb_is_one(z));
    fmpz_one(c); fmpz_mul_2exp(N, c, 2000); fmpz_set_ui(c, 3);
    CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
    CHECK(adf_char_eval_ucoset(z, x, u, 128) == ADF_OK && arb_is_zero(acb_imagref(z)));
    CHECK(arb_contains_si(acb_realref(z), -1) && arb_contains_si(acb_realref(z), 1));
    /* Inclusive precision cap on a general root: certification may fail, but never commits then. */
    CHECK(adf_char_set_conrey(x, 7, 3) == ADF_OK); fmpz_set_ui(c, 3); fmpz_set_ui(N, 7);
    CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
    sentinel(z); acb_struct cap_bytes = *z;
    int cap_st = adf_char_eval_ucoset(z, x, u, ADF_REAL_PREC_MAX);
    CHECK(cap_st == ADF_OK || cap_st == ADF_NOT_DETERMINED);
    if (cap_st != ADF_OK) unchanged(&cap_bytes, z);
    else CHECK(acb_is_finite(z));
    fmpz_one(c);
    CHECK(adf_char_set_conrey(x, 65536, 5) == ADF_OK);
    fmpz_set_ui(N, 65536); CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
    CHECK(adf_char_eval_ucoset(z, x, u, 2) == ADF_OK && acb_is_one(z));
    CHECK(adf_char_eval_ucoset_strict(z, x, u, 53) == ADF_OK && acb_is_one(z));
    for (int strict = 0; strict < 2; strict++) {
        acb_struct bytes; ulong q = x->q; sentinel(z); bytes = *z;
        x->q = 65537;
        CHECK((strict ? adf_char_eval_ucoset_strict(z, x, u, 53) :
                        adf_char_eval_ucoset(z, x, u, 53)) == ADF_LIMIT); unchanged(&bytes, z);
        x->q = 0;
        CHECK((strict ? adf_char_eval_ucoset_strict(z, x, u, ADF_REAL_PREC_MAX+1) :
                        adf_char_eval_ucoset(z, x, u, ADF_REAL_PREC_MAX+1)) == ADF_LIMIT);
        unchanged(&bytes, z); x->q = q;
    }
    fmpz_clear(c); fmpz_clear(N); acb_clear(z); acb_clear(point); adf_ucoset_clear(u); adf_char_clear(x);
}
static void conjugation(void)
{
    jsonl_file *f = vectors("conj", 1206); adf_char_t x, y, yy; acb_t s, t; fmpz_t a; fmpq_t p, q;
    adf_char_init(x); adf_char_init(y); adf_char_init(yy); acb_init(s); acb_init(t);
    fmpz_init(a); fmpq_init(p); fmpq_init(q);
    acb_set_si(s, 3); arb_set_si(acb_imagref(s), -7); arb_add_error_2exp_si(acb_imagref(s), -8);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i);
        CHECK(adf_char_set_conrey_acb(x, ui(field(v, "C")), ui(field(v, "n")), s) == ADF_OK);
        adf_char_conj(y, x);
        CHECK(y->q == x->q && y->n == ui(field(v, "inverse")) && y->parity == x->parity);
        acb_conj(t, s); CHECK(acb_equal(y->s, t)); adf_char_conj(yy, y); CHECK(adf_char_identical(yy, x));
        adf_char_conj(x, x); CHECK(adf_char_identical(x, y));
    }
    /* All 1966 characters of moduli <=80, including imprimitive inputs after lowering. */
    size_t chars = 0, units = 0;
    for (ulong C = 1; C <= 80; C++) for (ulong n = 1; n <= C; n++) if (n_gcd(C, n) == 1) {
        CHECK(adf_char_set_conrey(x, C, n) == ADF_OK); adf_char_conj(y, x); chars++;
        for (ulong j = 0; j < C; j++) if (n_gcd(j, C) == 1) {
            int zero; fmpz_set_ui(a, j);
            CHECK(adf_char_chi_phase(&zero, p, x, a) == ADF_OK && !zero);
            CHECK(adf_char_chi_phase(&zero, q, y, a) == ADF_OK && !zero);
            fmpq_add(p, p, q); CHECK(fmpz_divisible(fmpq_numref(p), fmpq_denref(p))); units++;
        }
    }
    /* Conjugation has no D1 cap; build a canonical above-cap input without the bounded constructor. */
    dirichlet_group_t G; dirichlet_char_t ch;
    CHECK(dirichlet_group_init(G, 65537)); dirichlet_char_init(ch, G); dirichlet_char_log(ch, G, 3);
    CHECK(dirichlet_conductor_char(G, ch) == 65537);
    x->q = 65537; x->n = 3; x->parity = dirichlet_parity_char(G, ch);
    dirichlet_char_clear(ch); dirichlet_group_clear(G); CHECK(adf_char_is_canonical(x));
    adf_char_conj(y, x); CHECK(y->q == 65537 && y->n == 21846 && y->parity == x->parity);
    adf_char_conj(y, y); CHECK(adf_char_identical(x, y));
    CHECK(chars == 1966); printf("conjugation: %zu characters, %zu unit products\n", chars, units);
    fmpq_clear(p); fmpq_clear(q); fmpz_clear(a); acb_clear(s); acb_clear(t);
    adf_char_clear(x); adf_char_clear(y); adf_char_clear(yy); jsonl_close(f);
}
static int load(adf_char_t x, const char *s, size_t n, const adf_text_limits_t *lim)
{
    adf_char_struct bytes = *x; acb_t old; int st; acb_init(old); acb_set(old, x->s);
    st = adf_char_load_str_binds(x, s, n, NULL, 0, lim);
    if (st != ADF_OK) { CHECK(memcmp(&bytes, x, sizeof(bytes)) == 0); CHECK(acb_equal(old, x->s)); }
    acb_clear(old); return st;
}
static void dump_one(adf_char_t x, const char *s, size_t n, const char *status)
{
    size_t count = 77, len; adf_ctx_desc_t d, bytes; char *out;
    int st = load(x, s, n, NULL); CHECK(!strcmp(adf_status_str(st), status));
    adf_ctx_desc_init(&d); bytes = d;
    CHECK(adf_char_dump_inspect(&count, &d, s, n, NULL) == st);
    CHECK(!memcmp(&bytes, &d, sizeof(d))); CHECK(count == (st == ADF_OK ? 0 : 77));
    if (st == ADF_OK) {
        adf_char_t y; adf_char_init(y); count = 99;
        CHECK(adf_char_dump_inspect(&count, NULL, s, n, NULL) == ADF_OK && count == 0);
        out = adf_char_dump_str(&len, x); CHECK(len == n && !memcmp(out, s, n) && out[n] == 0);
        CHECK(adf_char_load_str(y, out, len, (const adf_modctx_struct *) x, NULL) == ADF_OK);
        CHECK(adf_char_identical(x, y)); adf_str_free(out); adf_char_clear(y);
        adf_char_struct before = *x;
        CHECK(adf_char_load_str_binds(x, s, n, NULL, 1, NULL) == ADF_DOMAIN);
        CHECK(!memcmp(&before, x, sizeof(before)));
        CHECK(adf_char_load_str_binds(x, s, n, NULL, SIZE_MAX, NULL) == ADF_DOMAIN);
    }
    adf_ctx_desc_clear(&d);
}
static void dumps(void)
{
    jsonl_file *f = vectors("dump", 39); adf_char_t x; adf_char_init(x);
    for (size_t i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i); const char *s = str(field(v, "input"));
        dump_one(x, s, strlen(s), str(field(v, "status")));
    }
    jsonl_close(f);
    golden_file *g; golden_error_t ge; size_t rows = 0;
    CHECK(golden_open("tests/golden/dump.tsv", GOLDEN_DEFAULT_MAX_INPUT, &g, &ge));
    for (size_t i = 0; i < golden_count(g); i++) {
        const golden_record *r = golden_record_at(g, i);
        if (r->input_len >= 12 && !memcmp(r->input, "adf1 Q char ", 12)) {
            dump_one(x, r->input, r->input_len, r->is_status ? r->status : "OK"); rows++;
        }
    }
    CHECK(rows == 8); golden_close(g);
    const char *s = "adf1 Q char 5 2 0 0 0 0 0 0 0 0"; size_t n = strlen(s);
    for (size_t i = 0; i < n; i++) CHECK(load(x, s, i, NULL) != ADF_OK);
    char raw[80]; memcpy(raw, s, n); raw[n] = '\0';
    CHECK(load(x, raw, n+1, NULL) == ADF_PARSE); raw[n] = (char) 0x80;
    CHECK(load(x, raw, n+1, NULL) == ADF_PARSE);
    adf_text_limits_t lim; adf_text_limits_default(&lim); lim.max_len = 0;
    CHECK(load(x, s, n, &lim) == ADF_LIMIT);
    CHECK(load(x, "adf1 Q char 5 2 0 0 0 0 0 0 0 0!", n+1, &lim) == ADF_LIMIT);
    s = "adf1 Q char 10001 0 0 0 0 0 0 0 0 0";
    CHECK(load(x, s, strlen(s), NULL) == ADF_DOMAIN);
    s = "adf1 Q char 10001 3 0 1 0 0 0 0 0 0";
    CHECK(load(x, s, strlen(s), NULL) == ADF_DOMAIN);
    s = "adf1 Q char 10001 1 0 0 0 0 0 0 0 0!";
    CHECK(load(x, s, strlen(s), NULL) == ADF_PARSE);
    adf_char_clear(x);
}
/* Observe FLINT-owned allocations when LeakSanitizer is unavailable under ptrace.
   This is not a count of all GMP/runtime allocations (memory.rst:16-24). */
static void *(*old_alloc)(size_t);
static void *(*old_calloc)(size_t, size_t);
static void *(*old_realloc)(void *, size_t);
static void (*old_free)(void *);
static long live_blocks, allocations;
static void *count_alloc(size_t n)
{ void *p = old_alloc(n); if (p) { live_blocks++; allocations++; } return p; }
static void *count_calloc(size_t n, size_t size)
{ void *p = old_calloc(n, size); if (p) { live_blocks++; allocations++; } return p; }
static void *count_realloc(void *p, size_t n)
{
    void *q = old_realloc(p, n);
    if (!p && q) { live_blocks++; allocations++; }
    if (p && !n && !q) live_blocks--;
    return q;
}
static void count_free(void *p) { if (p) live_blocks--; old_free(p); }
static void memory_cycle(void)
{
    adf_char_t x, y; adf_ucoset_t u; acb_t z; fmpz_t c, N; char *s; size_t len, count = 3;
    adf_char_init(x); adf_char_init(y); adf_ucoset_init(u); acb_init(z); fmpz_init(c); fmpz_init(N);
    CHECK(adf_char_set_conrey(x, 5, 2) == ADF_OK); adf_char_conj(y, x); adf_char_conj(x, x);
    fmpz_one(c); fmpz_one(N); CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
    CHECK(adf_char_eval_ucoset(z, x, u, 53) == ADF_OK);
    CHECK(adf_char_eval_ucoset_strict(z, x, u, 53) == ADF_NOT_DETERMINED);
    s = adf_char_dump_str(&len, x);
    CHECK(adf_char_dump_inspect(&count, NULL, s, len, NULL) == ADF_OK && count == 0);
    CHECK(adf_char_load_str(y, s, len, NULL, NULL) == ADF_OK); adf_str_free(s);
    adf_char_clear(x); adf_char_clear(y); adf_ucoset_clear(u); acb_clear(z); fmpz_clear(c); fmpz_clear(N);
}
static void memory(void)
{
    for (int j = 0; j < 10; j++) memory_cycle();
    __flint_get_memory_functions(&old_alloc, &old_calloc, &old_realloc, &old_free);
    __flint_set_memory_functions(count_alloc, count_calloc, count_realloc, count_free);
    for (int j = 0; j < 100; j++) { memory_cycle(); CHECK(live_blocks == 0); }
    __flint_set_memory_functions(old_alloc, old_calloc, old_realloc, old_free);
    printf("memory: %ld FLINT blocks, %ld live after 100 cycles\n", allocations, live_blocks);
}
#ifdef ADF_CHAR_EVAL_WRAP
static void injected_failures(void)
{
    adf_char_t x; adf_ucoset_t u; acb_t z; fmpz_t c, N; acb_struct bytes;
    adf_char_init(x); adf_ucoset_init(u); acb_init(z); fmpz_init(c); fmpz_init(N);
    long before = setups;
    adf_char_conj(x, x); CHECK(setups == before); /* principal shortcut has no group setup */
    CHECK(adf_char_set_conrey(x, 5, 2) == ADF_OK); fmpz_one(c); fmpz_one(N);
    CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK); sentinel(z); bytes = *z;
    before = phase_calls;
    CHECK(adf_char_eval_ucoset_strict(z, x, u, 53) == ADF_NOT_DETERMINED);
    CHECK(phase_calls == before); unchanged(&bytes, z);
    fail_setup = setups + 1;
#ifdef ADF_CHECK_INVARIANTS
    fail_setup++; /* character entry predicate's group precedes the actual evaluator setup */
#endif
    CHECK(adf_char_eval_ucoset(z, x, u, 53) == ADF_UNSUPPORTED); unchanged(&bytes, z); fail_setup = 0;
    for (int st = ADF_NOT_DETERMINED; st <= ADF_LIMIT; st += ADF_LIMIT-ADF_NOT_DETERMINED) {
        fail_phase = st; CHECK(adf_char_eval_ucoset(z, x, u, 53) == st); unchanged(&bytes, z);
    }
    fail_phase = 0;
    for (wide_phase = 1; wide_phase <= 2; wide_phase++) {
        CHECK(adf_char_eval_ucoset(z, x, u, 53) == ADF_NOT_DETERMINED); unchanged(&bytes, z);
    }
    wide_phase = 0; sqrt_nonfinite = 1;
    CHECK(adf_char_root_number(z, x, 53) == ADF_NOT_DETERMINED); unchanged(&bytes, z); sqrt_nonfinite = 0;
    const char *s = "adf1 Q char 5 2 0 0 0 0 0 0 0 0"; adf_char_struct old = *x; size_t count = 123;
    fail_setup = setups+1; CHECK(load(x, s, strlen(s), NULL) == ADF_UNSUPPORTED); fail_setup = 0;
    CHECK(!memcmp(&old, x, sizeof(old)));
    fail_setup = setups+1;
    CHECK(adf_char_dump_inspect(&count, NULL, s, strlen(s), NULL) == ADF_UNSUPPORTED && count == 123);
    fail_setup = 0;
    old = *x; x->q = 65537; before = setups;
    CHECK(adf_char_eval_ucoset(z, x, u, 53) == ADF_LIMIT);
    CHECK(adf_char_eval_ucoset_strict(z, x, u, 53) == ADF_LIMIT);
    CHECK(setups == before); x->q = old.q;
    adf_char_clear(x); adf_ucoset_clear(u); acb_clear(z); fmpz_clear(c); fmpz_clear(N);
}
#endif
#ifdef ADF_CHECK_INVARIANTS
static void invariants(void)
{
    for (int mode = 0; mode < 9; mode++) {
        fflush(stdout);
        pid_t child = fork(); CHECK(child >= 0);
        if (!child) {
            adf_char_t x, y; adf_ucoset_t u; acb_t z; size_t len;
            adf_char_init(x); adf_char_init(y); adf_ucoset_init(u); acb_init(z);
            if (mode < 2) { x->parity = 1; if (mode) adf_char_conj(y, x);
                           else (void) adf_char_eval_ucoset(z, x, u, 53); }
            else if (mode == 2) { fmpz_zero(u->c); (void) adf_char_eval_ucoset_strict(z, x, u, 53); }
            else if (mode == 3) { x->n = 0; (void) adf_char_dump_str(&len, x); }
            else if (mode == 4) { y->q = 0; adf_char_conj(y, x); }
            else if (mode == 5) (void) adf_char_eval_ucoset(x->s, x, u, 53);
            else if (mode == 6) (void) adf_char_eval_ucoset_strict(x->s, x, u, 53);
            else if (mode == 7) { x->q = 0; (void) adf_char_eval_ucoset_strict(z, x, u, 53); }
            else {
                const char *s = "adf1 Q char 1 1 0 0 0 0 0 0 0 0"; x->q = 0;
                (void) adf_char_load_str(x, s, strlen(s), NULL, NULL);
            }
            _exit(0);
        }
        int status; CHECK(waitpid(child, &status, 0) == child);
        CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
}
#endif
int main(int argc, char **argv)
{
    if (argc == 1 || !strcmp(argv[1], "unit")) { unit_vectors(); unit_edges(); }
    if (argc == 1 || !strcmp(argv[1], "conj")) conjugation();
    if (argc == 1 || !strcmp(argv[1], "dump")) dumps();
    if (argc == 1) memory();
#ifdef ADF_CHAR_EVAL_WRAP
    if (argc == 1) injected_failures();
#endif
#ifdef ADF_CHECK_INVARIANTS
    if (argc == 1) invariants();
#endif
    flint_cleanup(); printf("test_char_eval: %ld checks\n", checks); return 0;
}
