/* Shared checks for this lane's two independently linked dump tests. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "jsonl.h"
#include "golden.h"
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

static unsigned long checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)
/* Rejecting an oversized complete body must not allocate the value before its count preflight.
   Allocator hooks are documented in refs/src/flint-3.0.1/memory.rst:9-24. */
static int reject_preflight;
static void *(*old_alloc)(size_t);
static void *(*old_calloc)(size_t, size_t);
static void *(*old_realloc)(void *, size_t);
static void (*old_free)(void *);
static void *(*base_alloc)(size_t);
static void *(*base_calloc)(size_t, size_t);
static void *(*base_realloc)(void *, size_t);
static void (*base_free)(void *);
static size_t live_blocks, allocated_blocks;
static void *count_alloc(size_t n)
{ void *p = base_alloc(n); if (p) { live_blocks++; allocated_blocks++; } return p; }
static void *count_calloc(size_t n, size_t size)
{ void *p = base_calloc(n, size); if (p) { live_blocks++; allocated_blocks++; } return p; }
static void *count_realloc(void *p, size_t n)
{
    void *q = base_realloc(p, n);
    if (!p && q) { live_blocks++; allocated_blocks++; }
    return q;
}
static void count_free(void *p) { if (p) { CHECK(live_blocks > 0); live_blocks--; } base_free(p); }
static void *guard_alloc(size_t n) { CHECK(n < 1048576); return old_alloc(n); }
static void *guard_calloc(size_t n, size_t size)
{ CHECK(!size || n < 1048576 / size); return old_calloc(n, size); }
static void *guard_realloc(void *p, size_t n) { CHECK(n < 1048576); return old_realloc(p, n); }
#if FINITE
#define TYPE adf_ffun_t
#define STRUCT adf_ffun_struct
#define INIT adf_ffun_init
#define CLEAR adf_ffun_clear
#define SET adf_ffun_set
#define SAME adf_ffun_identical
#define LOAD adf_ffun_load_str
#define BINDS adf_ffun_load_str_binds
#define DUMP adf_ffun_dump_str
#define INSPECT adf_ffun_dump_inspect
#define KIND "ffun"
#define NGOLD 4
#else
#define TYPE adf_rfun_t
#define STRUCT adf_rfun_struct
#define INIT adf_rfun_init
#define CLEAR adf_rfun_clear
#define SET adf_rfun_set
#define SAME adf_rfun_identical
#define LOAD adf_rfun_load_str
#define BINDS adf_rfun_load_str_binds
#define DUMP adf_rfun_dump_str
#define INSPECT adf_rfun_dump_inspect
#define KIND "rfun"
#define NGOLD 6
#endif

int arb_load_str(arb_t x, const char *s)
{ (void) x; (void) s; CHECK(0 && "unexpected FLINT string loader"); return 1; }

/* Counts the calls of fmpz_set_str; the loaders validate every token before any FLINT call
   (docs/conventions.md 10.2, include/adelefeld/dump.h:292-293), so a PARSE or LIMIT text makes
   none (review m4, finding 2). The conversion is the documented one (fmpz.rst:427-431) done with
   GMP's mpz_set_str, so that legitimate calls on valid texts still work. */
static unsigned long set_str_calls;
int fmpz_set_str(fmpz_t f, const char *str, int b)
{
    mpz_t m; int r;
    set_str_calls++; mpz_init(m); r = mpz_set_str(m, str, b);
    if (r == 0) fmpz_set_mpz(f, m);
    mpz_clear(m); return r == 0 ? 0 : -1;
}

static int status(const char *s)
{
    int i;
    for (i = 0; i < ADF_STATUS_COUNT; i++) if (!strcmp(s, adf_status_str(i))) return i;
    CHECK(0 && "unknown status"); return -1;
}

static const jsonl_value *field(const jsonl_value *v, const char *name)
{ const jsonl_value *out; jsonl_error_t e; CHECK(jsonl_field(v, name, &out, &e)); return out; }

static const char *string(const jsonl_value *v, size_t *n)
{ jsonl_error_t e; const char *s = jsonl_string(v, n, &e); CHECK(s != NULL); return s; }

static void one(TYPE x, const char *s, size_t n, int want, size_t nb,
                const adf_text_limits_t *lim)
{
    STRUCT before = *x;
    TYPE saved;
    void *members;
    size_t bytes, count = 773, len;
    int st, textwant = nb && want == ADF_DOMAIN ? ADF_OK : want;
    adf_ctx_desc_t d;
    adf_ctx_desc_t oldd;
    char *dump;
    unsigned long calls0 = set_str_calls;
    INIT(saved); SET(saved, x);
#if FINITE
    bytes = (size_t) (x->D * x->M) * sizeof(*x->f);
    members = malloc(bytes); CHECK(members != NULL); memcpy(members, x->f, bytes);
#else
    bytes = (size_t) x->len * sizeof(*x->term);
    members = malloc(bytes ? bytes : 1); CHECK(members != NULL);
    if (bytes) memcpy(members, x->term, bytes);
#endif
    /* Exactly n allocated bytes: no NUL is available to the loader. */
    { char *raw = malloc(n ? n : 1); CHECK(raw != NULL); memcpy(raw, s, n);
      if (reject_preflight) {
          __flint_get_memory_functions(&old_alloc, &old_calloc, &old_realloc, &old_free);
          __flint_set_memory_functions(guard_alloc, guard_calloc, guard_realloc, old_free);
      }
      st = BINDS(x, raw, n, NULL, nb, lim);
      if (reject_preflight) __flint_set_memory_functions(old_alloc, old_calloc, old_realloc, old_free);
      free(raw); }
    if (st != want) fprintf(stderr, KIND " length %zu: want %s, got %s\n",
                            n, adf_status_str(want), adf_status_str(st));
    CHECK(st == want);
    if (st != ADF_OK) {
        CHECK(!memcmp(&before, x, sizeof(before)) && SAME(x, saved));
#if FINITE
        CHECK(!memcmp(members, x->f, bytes));
#else
        CHECK(!bytes || !memcmp(members, x->term, bytes));
#endif
    }
    free(members); CLEAR(saved);
    adf_ctx_desc_init(&d); fmpz_set_ui(d.K, 37); oldd = d;
    CHECK(INSPECT(&count, &d, s, n, lim) == textwant);
    CHECK(count == (textwant == ADF_OK ? 0 : 773));
    CHECK(!memcmp(&oldd, &d, sizeof(d)) && fmpz_equal_ui(d.K, 37));
    adf_ctx_desc_clear(&d);
    count = 884;
    CHECK(INSPECT(&count, NULL, s, n, lim) == textwant);
    CHECK(count == (textwant == ADF_OK ? 0 : 884));
    if (want == ADF_PARSE || want == ADF_LIMIT) {
        if (set_str_calls != calls0) fprintf(stderr, KIND " length %zu: %s after %lu fmpz_set_str calls\n",
                                             n, adf_status_str(want), set_str_calls - calls0);
        CHECK(set_str_calls == calls0);
    }
    if (st == ADF_OK) {
        dump = DUMP(&len, x);
        CHECK(dump != NULL && len == n && !memcmp(s, dump, n) && dump[n] == 0);
        CHECK(LOAD(x, dump, len, (const adf_modctx_struct *) (uintptr_t) 1, lim) == ADF_OK);
        adf_str_free(dump);
    }
}

static void vectors(void)
{
    jsonl_file *f; jsonl_error_t e; size_t i, n, ignored;
    TYPE x; INIT(x);
    CHECK(jsonl_open("tests/ref/vectors/f4-slice7/" KIND ".jsonl", &f, &e));
    CHECK(jsonl_count(f) == 53);
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i);
        const char *s = string(field(v, "text"), &n);
        const char *want = string(field(v, "status"), &ignored);
        const char *nb = jsonl_int_text(field(v, "binds"), &e); CHECK(nb != NULL);
        one(x, s, n, status(want), (size_t) strtoul(nb, NULL, 10), NULL);
    }
    jsonl_close(f); CLEAR(x);
    printf(KIND " vectors: 53 (40 generated values)\n");
}

static void goldens(void)
{
    golden_file *g; golden_error_t e; size_t i, rows = 0;
    TYPE x; INIT(x);
    CHECK(golden_open("tests/golden/dump.tsv", GOLDEN_DEFAULT_MAX_INPUT, &g, &e));
    for (i = 0; i < golden_count(g); i++) {
        const golden_record *r = golden_record_at(g, i);
        if (r->input_len >= 12 && !memcmp(r->input, "adf1 Q " KIND " ", 12)) {
            one(x, r->input, r->input_len, r->is_status ? status(r->status) : ADF_OK, 0, NULL);
            rows++;
        }
    }
    CHECK(rows == NGOLD); golden_close(g); CLEAR(x);
    printf(KIND " goldens: %zu\n", rows);
}

static uint32_t rng = 410807;
static unsigned rnd(unsigned n) { rng = rng * 1664525U + 1013904223U; return rng % n; }

static void random_acb(acb_t z, unsigned iteration)
{
    unsigned k; fmpz_t m; fmpz_init(m);
    for (k = 0; k < 2; k++) {
        arb_ptr a = k ? acb_imagref(z) : acb_realref(z);
        fmpz_set_ui(m, 1 + 2 * rnd(999));
        if (iteration % 17 == 0) { fmpz_mul_2exp(m, m, 2000); fmpz_add_ui(m, m, 1); }
        if (rnd(2)) fmpz_neg(m, m);
        arf_set_fmpz(arb_midref(a), m);
        arb_mul_2exp_si(a, a, (slong) rnd(4001) - 2000);
        mag_set_ui_2exp_si(arb_radref(a), rnd(2) ? 0 : 1 + rnd(999), (slong) rnd(4001) - 2000);
    }
    fmpz_clear(m);
}

static void randoms(void)
{
    TYPE x, y; unsigned i, j; size_t n, count; char *s, *again;
    INIT(x); INIT(y);
    for (i = 0; i < 2000; i++) {
#if FINITE
        ulong D = 1 + rnd(6), M = 1 + rnd(6); acb_ptr f = _acb_vec_init((slong) (D * M));
        for (j = 0; j < D * M; j++) random_acb(f + j, i);
        CHECK(adf_ffun_set_acb_vec(x, D, M, f, (slong) (D * M)) == ADF_OK);
        _acb_vec_clear(f, (slong) (D * M));
#else
        adf_rterm_struct terms[4]; unsigned nt = rnd(5), k;
        for (j = 0; j < nt; j++) {
            acb_t z; acb_init(z); acb_poly_init(terms[j].P);
            acb_init(terms[j].A); acb_init(terms[j].B); acb_init(terms[j].C);
            for (k = 0; k < i % 8; k++) {
                random_acb(z, i); acb_poly_set_coeff_acb(terms[j].P, k, z);
            }
            random_acb(terms[j].A, i); arb_set_ui(acb_realref(terms[j].A), 3);
            mag_set_ui_2exp_si(arb_radref(acb_realref(terms[j].A)), rnd(2), -2);
            random_acb(terms[j].B, i); random_acb(terms[j].C, i); acb_clear(z);
        }
        CHECK(adf_rfun_set_terms(x, terms, nt) == ADF_OK);
        for (j = 0; j < nt; j++) {
            acb_poly_clear(terms[j].P); acb_clear(terms[j].A);
            acb_clear(terms[j].B); acb_clear(terms[j].C);
        }
#endif
        s = DUMP(&n, x); CHECK(s != NULL);
        CHECK(LOAD(y, s, n, NULL, NULL) == ADF_OK && SAME(x, y));
        again = DUMP(&count, y); CHECK(count == n && !memcmp(s, again, n));
        adf_str_free(again);
        count = 777; CHECK(INSPECT(&count, NULL, s, n, NULL) == ADF_OK && count == 0);
        CHECK(BINDS(y, s, n, NULL, 0, NULL) == ADF_OK && SAME(x, y));
        CHECK(BINDS(y, s, n, NULL, 1, NULL) == ADF_DOMAIN && SAME(x, y));
        CHECK(BINDS(y, s, n, NULL, (size_t) -1, NULL) == ADF_DOMAIN && SAME(x, y));
        adf_str_free(s);
    }
    CLEAR(x); CLEAR(y); printf(KIND " random identical round trips: 2000\n");
}

static void malformed(void)
{
    TYPE x; adf_text_limits_t lim; size_t i, n;
#if FINITE
    const char *s = "adf1 Q ffun 1 1 1 0 0 0 0 0 0 0";
    const char *domain = "adf1 Q ffun 1 1 0 5 0 0 0 0 0 0";
#else
    const char *s = "adf1 Q rfun 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
    const char *domain = "adf1 Q rfun 1 0 0 5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
#endif
    INIT(x); CHECK(LOAD(x, s, strlen(s), NULL, NULL) == ADF_OK);
#if FINITE
    one(x, "adf1 Q ffun 1 0", strlen("adf1 Q ffun 1 0"), ADF_DOMAIN, 0, NULL);
    one(x, "adf1 Q ffun 0 0", strlen("adf1 Q ffun 0 0"), ADF_DOMAIN, 0, NULL);
    one(x, "adf1 Q ffun -1 1", strlen("adf1 Q ffun -1 1"), ADF_PARSE, 0, NULL);
    one(x, "adf1 Q ffun fffffffffffffffff 0",
        strlen("adf1 Q ffun fffffffffffffffff 0"), ADF_DOMAIN, 0, NULL);
    { /* Review m4, finding 2: D or M longer than 15 digits on a text refused at the grammar
         stage (or at a limit) is not converted before the whole body has passed. */
      const char *early[] = {"adf1 Q ffun 1 fffffffffffffffff -43 0 0 0 -1b 0 0 0",
          "adf1 Q ffun fffffffffffffffff -1", "adf1 Q ffun -1 fffffffffffffffff",
          "adf1 Q ffun fffffffffffffffff 1 1 0 0 0 0 0 0 0",
          "adf1 Q ffun 1 1000000000000000 1 0 0 0 0 0 0 0",
          "adf1 Q ffun ffffffffffffffffffffffffffffffff ffffffffffffffffffffffffffffffff 0",
          "adf1 Q ffun fffffffffffffffff 0 z"};
      size_t e;
      for (e = 0; e < sizeof(early) / sizeof(*early); e++)
          one(x, early[e], strlen(early[e]), ADF_PARSE, 0, NULL);
      adf_text_limits_default(&lim); lim.max_items = 0;
      one(x, "adf1 Q ffun 1 1 1 0 0 0 0 0 0 0", 31, ADF_LIMIT, 0, &lim);
      { const char *big = "adf1 Q ffun 1 1 1 -ffffffffffffffffffffffff 0 0 0 0 0 0";
        one(x, big, strlen(big), ADF_LIMIT, 0, &lim); } }
#else
    { const char *negative = "adf1 Q rfun 1 0 -1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
      one(x, negative, strlen(negative), ADF_DOMAIN, 0, NULL);
      one(x, "adf1 Q rfun fffffffffffffffff", 29, ADF_PARSE, 0, NULL);
      one(x, "adf1 Q rfun 1 fffffffffffffffff 0", 33, ADF_PARSE, 0, NULL); }
#endif
    { const char *badballs[] = {"2 0 0 0", "0 5 0 0", "1 0 40000001 0",
                               "1 0 -1 0", "1 0 0 -1"};
      size_t b, slot, j, slots = FINITE ? 2 : 8;
      for (b = 0; b < sizeof(badballs) / sizeof(*badballs); b++)
          for (slot = 0; slot < slots; slot++) {
              char bad[240]; strcpy(bad, FINITE ? "adf1 Q ffun 1 1" : "adf1 Q rfun 1 1");
              for (j = 0; j < slots; j++) {
                  strcat(bad, " ");
                  strcat(bad, slot == j ? badballs[b] : (j % 2 ? "0 0 0 0" : "1 0 0 0"));
              }
              one(x, bad, strlen(bad), ADF_DOMAIN, 0, NULL);
          }
    }
    one(x, domain, strlen(domain), ADF_DOMAIN, 0, NULL);
    adf_text_limits_default(&lim); lim.max_items = 0;
    one(x, domain, strlen(domain), ADF_LIMIT, 0, &lim);
    { char *bad = malloc(strlen(domain) + 2); CHECK(bad != NULL);
      strcpy(bad, domain); strcat(bad, "z"); one(x, bad, strlen(bad), ADF_PARSE, 0, &lim); free(bad); }
    { char *bad = malloc(strlen(domain) + 2); CHECK(bad != NULL);
      strcpy(bad, domain); bad[3] = '2'; strcat(bad, "\x80");
      one(x, bad, strlen(bad), ADF_PARSE, 0, NULL); bad[strlen(bad) - 1] = '\t';
      one(x, bad, strlen(bad), ADF_UNSUPPORTED, 0, NULL); free(bad); }
    for (i = 0; i < strlen(s); i++) {
        int st; size_t count = 19; STRUCT before = *x;
        char *raw = malloc(i ? i : 1); CHECK(raw != NULL); memcpy(raw, s, i);
        unsigned long calls0 = set_str_calls;
        st = LOAD(x, raw, i, NULL, NULL); CHECK(st != ADF_OK && !memcmp(&before, x, sizeof(before)));
        CHECK(INSPECT(&count, NULL, raw, i, NULL) == st && count == 19); free(raw);
        CHECK((st != ADF_PARSE && st != ADF_LIMIT) || set_str_calls == calls0);
    }
    n = 99; CHECK(INSPECT(&n, NULL, NULL, 0, NULL) == ADF_PARSE && n == 99);
    lim.max_len = 0; CHECK(LOAD(x, NULL, 1, NULL, &lim) == ADF_LIMIT);
    /* No M1-D9 exponent cap here, including exponents larger than a word. */
#if FINITE
    s = "adf1 Q ffun 1 1 1 100001 1 -ffffffffffffffffffffffff 0 0 0 0";
#else
    s = "adf1 Q rfun 1 0 1 100001 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
#endif
    one(x, s, strlen(s), ADF_OK, 0, NULL); CLEAR(x);
}

static char *repeat(const char *head, const char *unit, size_t count, const char *tail)
{
    size_t a = strlen(head), b = strlen(unit), c = strlen(tail), i;
    char *s = malloc(a + b * count + c + 1); CHECK(s != NULL); memcpy(s, head, a);
    for (i = 0; i < count; i++) memcpy(s + a + b * i, unit, b);
    memcpy(s + a + b * count, tail, c + 1); return s;
}

static void caps(void)
{
    TYPE x; adf_text_limits_t lim; char head[80], *s; size_t k;
    INIT(x); adf_text_limits_default(&lim); lim.max_len = 64U << 20; lim.max_items = 2000000;
#if FINITE
    for (k = 1048575; k <= 1048577; k++) {
        snprintf(head, sizeof(head), "adf1 Q ffun 1 %zx", k);
        s = repeat(head, " 0 0 0 0 0 0 0 0", k, "");
        reject_preflight = k > 1048576;
        one(x, s, strlen(s), k > 1048576 ? ADF_LIMIT : ADF_OK, 0, &lim); free(s);
        reject_preflight = 0;
    }
#else
    for (k = 65535; k <= 65537; k++) {
        snprintf(head, sizeof(head), "adf1 Q rfun %zx", k);
        s = repeat(head, " 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0", k, "");
        reject_preflight = k > 65536;
        one(x, s, strlen(s), k > 65536 ? ADF_LIMIT : ADF_OK, 0, &lim); free(s);
        reject_preflight = 0;
        snprintf(head, sizeof(head), "adf1 Q rfun 1 %zx", k);
        s = repeat(head, " 1 0 0 0 0 0 0 0", k,
                   " 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        reject_preflight = k > 65536;
        one(x, s, strlen(s), k > 65536 ? ADF_LIMIT : ADF_OK, 0, &lim); free(s);
        reject_preflight = 0;
    }
    s = repeat("adf1 Q rfun 2 10000", " 1 0 0 0 0 0 0 0", 65536,
               " 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"
               " 1 0 0 0 0 1 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
    one(x, s, strlen(s), ADF_LIMIT, 0, &lim); free(s);
    for (k = 65535; k <= 65537; k++) {
        const char *params = " 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
        char middle[100], *first;
        snprintf(middle, sizeof(middle), "%s %zx", params, k - 32768);
        first = repeat("adf1 Q rfun 2 8000", " 1 0 0 0 0 0 0 0", 32768, middle);
        s = repeat(first, " 1 0 0 0 0 0 0 0", k - 32768, params); free(first);
        reject_preflight = k > 65536;
        one(x, s, strlen(s), k > 65536 ? ADF_LIMIT : ADF_OK, 0, &lim); free(s);
        reject_preflight = 0;
    }
#endif
    CLEAR(x); printf(KIND " D1 boundaries checked\n");
}

static void invariant_aborts(void)
{
#ifdef ADF_CHECK_INVARIANTS
    unsigned mode;
    for (mode = 0; mode < 3; mode++) {
        pid_t p; int w; fflush(NULL); p = fork(); CHECK(p >= 0);
        if (!p) {
            TYPE x; size_t n; INIT(x);
#if FINITE
            x->D = 0;
            if (mode == 0) (void) DUMP(&n, x);
            else if (mode == 1) (void) LOAD(x, "adf1 Q ffun 1 1 0 0 0 0 0 0 0 0", 31, NULL, NULL);
            else (void) BINDS(x, "adf1 Q ffun 1 1 0 0 0 0 0 0 0 0", 31, NULL, 0, NULL);
            x->D = 1; CLEAR(x);         /* reached only if the call did not abort (memcheck) */
#else
            x->len = -1;
            if (mode == 0) (void) DUMP(&n, x);
            else if (mode == 1) (void) LOAD(x, "adf1 Q rfun 0", 13, NULL, NULL);
            else (void) BINDS(x, "adf1 Q rfun 0", 13, NULL, 0, NULL);
            x->len = 0; CLEAR(x);       /* reached only if the call did not abort (memcheck) */
#endif
            _exit(0);
        }
        CHECK(waitpid(p, &w, 0) == p && WIFSIGNALED(w) && WTERMSIG(w) == SIGABRT);
    }
#endif
}

int main(int argc, char **argv)
{
    __flint_get_memory_functions(&base_alloc, &base_calloc, &base_realloc, &base_free);
    __flint_set_memory_functions(count_alloc, count_calloc, count_realloc, count_free);
    vectors(); goldens(); randoms(); malformed(); invariant_aborts();
    if (argc == 1 || strcmp(argv[1], "--quick")) caps();
    flint_cleanup(); CHECK(live_blocks == 0);
    __flint_set_memory_functions(base_alloc, base_calloc, base_realloc, base_free);
    printf(KIND " memory: %zu FLINT blocks, %zu live\n", allocated_blocks, live_blocks);
    printf(KIND " dump: %lu checks, 0 failures\n", checks); return 0;
}
