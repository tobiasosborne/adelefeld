/* Slice 3.1-d. Exact storage keys: quotient3_checks.py piece_key; CV-45, 9.5, 11.3. */
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

static unsigned long checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

static const jsonl_value *field(const jsonl_value *v, const char *name)
{
    const jsonl_value *out; jsonl_error_t e;
    CHECK(jsonl_field(v, name, &out, &e)); return out;
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
static void read_a(adf_adele_struct *a, const char *s)
{ CHECK(adf_adele_set_str(a, s, strlen(s), 128, NULL) == ADF_OK); }
static int read_q(adf_qclass_t x, const char *s, size_t len, const adf_text_limits_t *lim)
{
    adf_qclass_struct before = *x; adf_adele_struct *members;
    int st;
    members = malloc((size_t) x->len * sizeof(*members)); CHECK(members != NULL);
    memcpy(members, x->piece, (size_t) x->len * sizeof(*members));
    st = adf_qclass_set_str(x, s, len, 128, lim);
    if (st != ADF_OK) {
        CHECK(!memcmp(&before, x, sizeof(before)));
        CHECK(!memcmp(members, x->piece, (size_t) x->len * sizeof(*members)));
    }
    free(members); return st;
}
static void expect_print(const adf_qclass_t x, const char *want, slong digits)
{
    size_t n; char *s = adf_qclass_get_str(&n, x, digits);
    CHECK(s != NULL);
    if (strcmp(s, want)) fprintf(stderr, "want: %s\ngot:  %s\n", want, s);
    CHECK(n == strlen(want) && !memcmp(s, want, n)); adf_str_free(s);
}
static int encloses_piece(const adf_qclass_t x, const adf_adele_t a)
{
    slong j;
    for (j = 0; j < x->len; j++)
        if (adf_fball_equal_set(&x->piece[j].fin, &a->fin) && arb_contains(x->piece[j].inf, a->inf))
            return 1;
    return 0;
}

static void vectors(void)
{
    jsonl_file *f; jsonl_error_t e; size_t i, j;
    adf_qclass_t x, y; adf_adele_struct a[8]; fmpq_t q; arf_t r;
    adf_qclass_init(x); adf_qclass_init(y); fmpq_init(q); arf_init(r);
    for (i = 0; i < 8; i++) adf_adele_init(a+i);
    CHECK(jsonl_open("tests/ref/vectors/q-slice4/pieces.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 100);
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i), *entries = field(v, "pieces"), *keys = field(v, "keys");
        long n = number(field(v, "raw"));
        for (j = 0; j < (size_t) n; j++) read_a(a+j, str(jsonl_at(entries, j, &e)));
        CHECK(adf_qclass_set_pieces(x, a, n, n-1) == ADF_LIMIT);
        CHECK(adf_qclass_set_pieces(x, a, n, n) == ADF_OK);
        CHECK(x->form == ADF_QCLASS_PIECES && x->len == number(field(v, "count")));
        CHECK(adf_qclass_is_canonical(x));
        expect_print(x, str(field(v, "printed")), 20);
        for (j = 0; j < (size_t) x->len; j++) {
            const jsonl_value *key = jsonl_at(keys, j, &e); fmpq_t m, rad; adf_rat_t A, H;
            fmpq_init(m); fmpq_init(rad); adf_rat_init(A); adf_rat_init(H);
            arf_get_fmpq(m, arb_midref(x->piece[j].inf));
            arf_set_mag(r, arb_radref(x->piece[j].inf)); arf_get_fmpq(rad, r);
            fmpq_sub(q, m, rad);
            CHECK(fmpq_set_str(m, str(jsonl_at(key, 0, &e)), 10) == 0 && fmpq_equal(m, q));
            fmpq_add(q, q, rad); fmpq_add(q, q, rad);
            CHECK(fmpq_set_str(m, str(jsonl_at(key, 1, &e)), 10) == 0 && fmpq_equal(m, q));
            adf_fball_get_radius(H, &x->piece[j].fin); adf_fball_get_center(A, &x->piece[j].fin);
            CHECK(fmpq_set_str(q, str(jsonl_at(key, 2, &e)), 10) == 0 && fmpq_equal(q, H->q));
            CHECK(fmpq_set_str(q, str(jsonl_at(key, 3, &e)), 10) == 0 && fmpq_equal(q, A->q));
            fmpq_clear(m); fmpq_clear(rad); adf_rat_clear(A); adf_rat_clear(H);
        }
        CHECK(read_q(y, str(field(v, "input")), strlen(str(field(v, "input"))), NULL) == ADF_OK);
        CHECK(adf_qclass_identical(x, y));
        { size_t len; char *s = adf_qclass_get_str(&len, x, 2);
          CHECK(s != NULL && read_q(y, s, len, NULL) == ADF_OK);
          for (j = 0; j < (size_t) x->len; j++) CHECK(encloses_piece(y, x->piece+j));
          adf_str_free(s); }
    }
    jsonl_close(f);
    for (i = 0; i < 8; i++) adf_adele_clear(a+i);
    adf_qclass_clear(x); adf_qclass_clear(y); fmpq_clear(q); arf_clear(r);
}

static void golden(int strict)
{
    golden_file *g; golden_error_t ge; jsonl_file *f; jsonl_error_t e; size_t i, j;
    adf_qclass_t x; fmpq_t lo, hi; adf_rat_t a, h;
    adf_qclass_init(x); fmpq_init(lo); fmpq_init(hi); adf_rat_init(a); adf_rat_init(h);
    CHECK(golden_open("tests/golden/qclass.tsv", GOLDEN_DEFAULT_MAX_INPUT, &g, &ge));
    CHECK(jsonl_open("tests/ref/vectors/q-slice4/text.jsonl", &f, &e));
    CHECK(golden_count(g) == jsonl_count(f));
    for (i = 0; i < golden_count(g); i++) {
        const golden_record *row = golden_record_at(g, i); const jsonl_value *v = jsonl_record(f, i);
        int st = read_q(x, row->input, row->input_len, NULL);
        if (row->is_status) CHECK(!strcmp(adf_status_str(st), row->status));
        else {
            const jsonl_value *members = field(v, "members");
            CHECK(st == ADF_OK && adf_qclass_is_canonical(x));
            expect_print(x, strict ? row->expected : str(field(v, "c_print")),
                         !strncmp(row->input, "union", 5) ? 2 : 6);
            for (j = 0; j < jsonl_size(members); j++) {
                const jsonl_value *m = jsonl_at(members, j, &e); slong k; int found = 0;
                CHECK(fmpq_set_str(lo, str(field(m, "lo")), 10) == 0);
                CHECK(fmpq_set_str(hi, str(field(m, "hi")), 10) == 0);
                CHECK(fmpq_set_str(a->q, str(field(m, "a")), 10) == 0);
                CHECK(fmpq_set_str(h->q, str(field(m, "h")), 10) == 0);
                for (k = 0; k < x->len; k++) {
                    adf_rat_t b, n; adf_rat_init(b); adf_rat_init(n);
                    adf_fball_get_center(b, &x->piece[k].fin); adf_fball_get_radius(n, &x->piece[k].fin);
                    found |= fmpq_equal(a->q, b->q) && fmpq_equal(h->q, n->q) &&
                             arb_contains_fmpq(x->piece[k].inf, lo) && arb_contains_fmpq(x->piece[k].inf, hi);
                    adf_rat_clear(b); adf_rat_clear(n);
                }
                CHECK(found);
            }
        }
    }
    golden_close(g); jsonl_close(f); adf_qclass_clear(x);
    fmpq_clear(lo); fmpq_clear(hi); adf_rat_clear(a); adf_rat_clear(h);
}

static void construction(void)
{
    adf_qclass_t x, saved; adf_adele_struct *a; slong i, n;
    adf_qclass_struct bytes; adf_adele_struct member;
    adf_qclass_init(x); adf_qclass_init(saved); read_a(x->piece, "(-7 ; 1/3)");
    adf_qclass_set(saved, x); bytes = *x; member = *x->piece;
    CHECK(adf_qclass_set_pieces(x, NULL, 1000, 0) == ADF_LIMIT);
    CHECK(adf_qclass_set_pieces(x, NULL, 1, -1) == ADF_LIMIT);
    CHECK(adf_qclass_set_pieces(x, NULL, 0, 1) == ADF_DOMAIN);
    CHECK(adf_qclass_set_pieces(x, NULL, -1, 1) == ADF_DOMAIN);
    CHECK(!memcmp(x, &bytes, sizeof(bytes)) && !memcmp(x->piece, &member, sizeof(member)));
    a = flint_malloc(1000 * sizeof(*a));
    for (i = 0; i < 1000; i++) { adf_adele_init(a+i); read_a(a+i, "(0.5 ; 0 mod 3)"); }
    for (n = 1; n <= 1000; n = n == 1 ? 2 : 1000) {
        CHECK(adf_qclass_set_pieces(x, a, n, n-1) == ADF_LIMIT);
        CHECK(adf_qclass_set_pieces(x, a, n, n) == ADF_OK && x->len == 1);
        if (n == 1000) break;
    }
    adf_qclass_set(x, saved); arb_indeterminate(a[999].inf);
    bytes = *x; member = *x->piece;
    CHECK(adf_qclass_set_pieces(x, a, 1000, 1000) == ADF_DOMAIN);
    CHECK(adf_qclass_identical(x, saved));
    CHECK(!memcmp(x, &bytes, sizeof(bytes)) && !memcmp(x->piece, &member, sizeof(member)));
    read_a(a+999, "(1.5 ; 0 mod 3)");
    CHECK(adf_qclass_set_pieces(x, a, 1000, 1000) == ADF_DOMAIN);
    CHECK(!memcmp(x, &bytes, sizeof(bytes)) && !memcmp(x->piece, &member, sizeof(member)));
    read_a(a+999, "(0.5 ; 1/2 mod 3)");
    CHECK(adf_qclass_set_pieces(x, a, 1000, 1000) == ADF_DOMAIN);
    CHECK(!memcmp(x, &bytes, sizeof(bytes)) && !memcmp(x->piece, &member, sizeof(member)));
    for (i = 0; i < 1000; i++) adf_adele_clear(a+i);
    flint_free(a); adf_qclass_clear(x); adf_qclass_clear(saved);
}

static void limits(void)
{
    const char *s = "union((0.5 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q";
    adf_text_limits_t lim; adf_qclass_t x; adf_text_limits_default(&lim); adf_qclass_init(x);
    lim.max_items = 2; CHECK(read_q(x, s, strlen(s), &lim) == ADF_OK && x->len == 1);
    lim.max_items = 1; CHECK(read_q(x, s, strlen(s), &lim) == ADF_LIMIT);
    s = "union((1.00000000000000000000000000000000000000001 ; 0)) + Q";
    CHECK(read_q(x, s, strlen(s), &lim) == ADF_DOMAIN);
    s = "union((-1e-50 ; 0)) + Q"; CHECK(read_q(x, s, strlen(s), &lim) == ADF_DOMAIN);
    s = "union((1e100001 ; 1/0), (0 ; 0)) + Q";
    CHECK(read_q(x, s, strlen(s), &lim) == ADF_LIMIT);
    s = "union((1e100001 ; 1/0)) + Q";
    CHECK(read_q(x, s, strlen(s), &lim) == ADF_LIMIT);
    s = "union((1e100001 ; 1/0), (0 ; 0)) + Q!";
    CHECK(read_q(x, s, strlen(s), &lim) == ADF_PARSE);
    s = "union((1e100001 ; 1/0)) + Q\x80";
    CHECK(read_q(x, s, strlen(s), &lim) == ADF_PARSE);
    lim.max_len = 0; CHECK(read_q(x, NULL, 1, &lim) == ADF_LIMIT);
    adf_text_limits_default(&lim); lim.max_exp10 = WORD_MAX;
    s = "union((0.5 +/- 1e100000000000 ; 0)) + Q";
    CHECK(read_q(x, s, strlen(s), &lim) == ADF_LIMIT);
    s = "union((0e100000000000 ; 0)) + Q";
    CHECK(read_q(x, s, strlen(s), &lim) == ADF_OK);
    s = "union((0.5 +/- 1e100000000000 ; 0)) + Q!";
    CHECK(read_q(x, s, strlen(s), &lim) == ADF_PARSE);
    s = "union((0.5 +/- 1e100000000000 ; 0)) + Q\x80";
    CHECK(read_q(x, s, strlen(s), &lim) == ADF_PARSE);
    s = "union((0.9 ; 0)) + Q";
    CHECK(adf_qclass_set_str(x, s, strlen(s), 2, NULL) == ADF_OK);
    CHECK(arf_is_one(arb_midref(x->piece->inf)) && fmpz_is_zero(x->piece->fin.H));
    { fmpq_t exact; adf_qclass_struct before = *x; adf_adele_struct member = *x->piece;
      fmpq_init(exact); fmpq_set_si(exact, 9, 10); CHECK(arb_contains_fmpq(x->piece->inf, exact));
      CHECK(adf_qclass_set_str(x, NULL, 1, ADF_REAL_PREC_MAX+1, NULL) == ADF_LIMIT);
      CHECK(!memcmp(&before, x, sizeof(before)) && !memcmp(&member, x->piece, sizeof(member)));
      fmpq_clear(exact); }
    adf_qclass_clear(x);
}

/* refs/src/flint-3.0.1/memory.rst:16-22: save/restore FLINT allocation callbacks.
   Count a complete initialized lifetime with immediate small finite data. No LSan is needed. */
static void *(*saved_alloc)(size_t);
static void *(*saved_calloc)(size_t, size_t);
static void *(*saved_realloc)(void *, size_t);
static void (*saved_free)(void *);
static long live_blocks;
static void *count_alloc(size_t n) { void *p = saved_alloc(n); if (p) live_blocks++; return p; }
static void *count_calloc(size_t n, size_t s)
{ void *p = saved_calloc(n, s); if (p) live_blocks++; return p; }
static void *count_realloc(void *p, size_t n)
{ void *q = saved_realloc(p, n); if (!p && q) live_blocks++; return q; }
static void count_free(void *p) { if (p) live_blocks--; saved_free(p); }
static void allocation_lifetime(void)
{
    adf_qclass_t x; adf_adele_struct a[2];
    adf_adele_init(a); adf_adele_init(a+1);
    read_a(a, "(0.25 ; 0 mod 2)"); read_a(a+1, "(0.75 ; 0 mod 2)");
    __flint_get_memory_functions(&saved_alloc, &saved_calloc, &saved_realloc, &saved_free);
    live_blocks = 0;
    __flint_set_memory_functions(count_alloc, count_calloc, count_realloc, count_free);
    adf_qclass_init(x); CHECK(adf_qclass_set_pieces(x, a, 2, 2) == ADF_OK); adf_qclass_clear(x);
    __flint_set_memory_functions(saved_alloc, saved_calloc, saved_realloc, saved_free);
    CHECK(live_blocks == 0);
    adf_adele_clear(a); adf_adele_clear(a+1);
}

static void local_and_bounds(void)
{
    const char *s = "adf1 Q adele 1 1 -1 0 0 l 2 6 2 2 3 0 0";
    adf_modctx_struct *ctx6, *ctx2; adf_adele_struct a[4]; adf_qclass_t x, saved;
    ulong block = 2; unsigned i; fmpz_t mant; adf_qclass_struct before; adf_adele_struct member;
    CHECK(adf_modctx_new_from_dump(&ctx6, s, strlen(s), 0, NULL) == ADF_OK);
    CHECK(adf_modctx_new_blocks(&ctx2, &block, 1) == ADF_OK);
    adf_qclass_init(x); adf_qclass_init(saved); fmpz_init(mant);
    for (i = 0; i < 4; i++) adf_adele_init(a+i);
    CHECK(adf_adele_load_str(a, s, strlen(s), ctx6, NULL) == ADF_OK);
    read_a(a+1, "(0.5 ; 0 mod 3)"); read_a(a+2, "(0.5 ; 0 mod 4)");
    read_a(a+3, "(0.5 ; 0 mod 2)");
    CHECK(adf_fball_set_local(&a[3].fin, &a[3].fin, ctx2) == ADF_OK);
    CHECK(adf_qclass_set_pieces(x, a, 4, 4) == ADF_OK && x->len == 3);
    CHECK(x->piece[0].fin.mctx == ctx2 && x->piece[1].fin.mctx == ctx6);
    CHECK(fmpz_equal_ui(x->piece[1].fin.H, 6) && fmpz_equal_ui(x->piece[1].fin.d, 2));
    CHECK(adf_adele_identical(x->piece+1, a)); /* stable first raw local duplicate */
    adf_adele_swap(a, a+1);
    CHECK(adf_qclass_set_pieces(x, a, 2, 2) == ADF_OK && x->len == 1);
    CHECK(x->piece[0].fin.backend == ADF_GLOBAL && adf_adele_identical(x->piece, a));
    adf_qclass_set(saved, x); before = *x; member = *x->piece;
    CHECK(adf_qclass_set_pieces(x, a, 2, 1) == ADF_LIMIT);
    CHECK(!memcmp(&before, x, sizeof(before)) && !memcmp(&member, x->piece, sizeof(member)));
    arf_set_ui_2exp_si(arb_midref(a[0].inf), 1, -ADF_QCLASS_EXP_MAX-1);
    CHECK(adf_qclass_set_pieces(x, a, 1, 1) == ADF_OK); /* normalized arf exponent -cap */
    arf_set_ui_2exp_si(arb_midref(a[0].inf), 1, -ADF_QCLASS_EXP_MAX-2);
    adf_qclass_set(x, saved); before = *x; member = *x->piece;
    CHECK(adf_qclass_set_pieces(x, a, 1, 1) == ADF_LIMIT);
    CHECK(!memcmp(&before, x, sizeof(before)) && !memcmp(&member, x->piece, sizeof(member)));
    arb_one(a[0].inf); mag_set_ui_2exp_si(arb_radref(a[0].inf), 1, ADF_QCLASS_EXP_MAX-1);
    CHECK(adf_qclass_set_pieces(x, a, 1, 1) == ADF_OK);
    mag_set_ui_2exp_si(arb_radref(a[0].inf), 1, ADF_QCLASS_EXP_MAX);
    CHECK(adf_qclass_set_pieces(x, a, 1, 1) == ADF_LIMIT);
    arb_zero(a[0].inf); fmpz_one(mant); fmpz_mul_2exp(mant, mant, ADF_QCLASS_BITS_MAX);
    fmpz_add_ui(mant, mant, 1); fmpz_set(a[0].fin.H, mant); fmpz_zero(a[0].fin.A);
    CHECK(adf_qclass_set_pieces(x, a, 1, 1) == ADF_LIMIT);
    for (i = 0; i < 4; i++) adf_adele_clear(a+i);
    adf_qclass_clear(x); adf_qclass_clear(saved); fmpz_clear(mant);
    adf_modctx_free(ctx6); adf_modctx_free(ctx2);
}
static void debug(void)
{
#ifdef ADF_CHECK_INVARIANTS
    pid_t p = fork(); int status; CHECK(p >= 0);
    if (!p) { adf_qclass_t x; adf_qclass_init(x); x->form = 99;
              (void) adf_qclass_set_pieces(x, NULL, 1, 0); _exit(0); }
    CHECK(waitpid(p, &status, 0) == p && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
#endif
}

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "--strict-golden")) golden(1);
    else if (argc > 1 && !strcmp(argv[1], "--construction")) construction();
    else { golden(0); vectors(); construction(); limits(); local_and_bounds(); allocation_lifetime(); debug(); }
    printf("qclass_text: %lu checks\n", checks); flint_cleanup(); return 0;
}
