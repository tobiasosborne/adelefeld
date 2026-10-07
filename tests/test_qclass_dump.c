/* Slice 3.1-d. Strict bytes, CV-52 validation, raw local data and traversal bindings (10.2). */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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

/* This loader reconstructs validated fields and never needs arb_load_str.
   Interpose the unsafe loader so an early call is an assertion failure even when FLINT would return. */
int arb_load_str(arb_t x, const char *s)
{
    (void) x; (void) s; CHECK(0 && "unexpected FLINT arb string load"); return 1;
}

static const jsonl_value *field(const jsonl_value *v, const char *name)
{ const jsonl_value *out; jsonl_error_t e; CHECK(jsonl_field(v, name, &out, &e)); return out; }
static const char *str(const jsonl_value *v)
{
    jsonl_error_t e; size_t n; const char *s = jsonl_string(v, &n, &e); CHECK(s != NULL); return s;
}
static int read_q(adf_qclass_t x, const char *s, size_t n,
                  const adf_modctx_struct *const *binds, size_t nb, const adf_text_limits_t *lim)
{
    adf_qclass_struct before = *x; adf_adele_struct *members;
    int st;
    members = malloc((size_t) x->len * sizeof(*members)); CHECK(members != NULL);
    memcpy(members, x->piece, (size_t) x->len * sizeof(*members));
    st = adf_qclass_load_str_binds(x, s, n, binds, nb, lim);
    if (st != ADF_OK) {
        CHECK(!memcmp(&before, x, sizeof(before)));
        CHECK(!memcmp(members, x->piece, (size_t) x->len * sizeof(*members)));
    }
    free(members); return st;
}
static void check_dump(adf_qclass_t x, const char *s, size_t n, size_t want_ctx)
{
    adf_modctx_struct *owned[8]; const adf_modctx_struct *binds[8];
    adf_ctx_desc_t desc[9]; size_t i, cap, count = 999, len; char *out;
    CHECK(adf_qclass_dump_inspect(&count, NULL, s, n, NULL) == ADF_OK && count == want_ctx);
    for (i = 0; i < 9; i++) adf_ctx_desc_init(desc+i);
    cap = 9; CHECK(adf_qclass_dump_inspect(&cap, desc, s, n, NULL) == ADF_OK && cap == count);
    CHECK(fmpz_is_one(desc[8].K) && desc[8].k == 0 && desc[8].q == NULL);
    CHECK(count <= 8);
    for (i = 0; i < count; i++) {
        fmpz_t k; fmpz_init(k);
        CHECK(adf_modctx_new_from_dump(owned+i, s, n, i, NULL) == ADF_OK);
        binds[i] = owned[i]; adf_modctx_get_modulus(k, owned[i]);
        CHECK(fmpz_equal(k, desc[i].K) && adf_modctx_nblocks(owned[i]) == desc[i].k);
        fmpz_clear(k);
    }
    CHECK(read_q(x, s, n, binds, count, NULL) == ADF_OK);
    CHECK(adf_qclass_is_canonical(x));
    out = adf_qclass_dump_str(&len, x);
    CHECK(out != NULL && len == n && !memcmp(out, s, n) && out[n] == 0); adf_str_free(out);
    if (count) CHECK(read_q(x, s, n, binds, count-1, NULL) == ADF_DOMAIN);
    CHECK(read_q(x, s, n, binds, count+1, NULL) == ADF_DOMAIN);
    CHECK(read_q(x, s, n, binds, SIZE_MAX, NULL) == ADF_DOMAIN);
    if (count) {
        const adf_modctx_struct *save = binds[count-1]; binds[count-1] = NULL;
        CHECK(read_q(x, s, n, binds, count, NULL) == ADF_DOMAIN); binds[count-1] = save;
        cap = count-1;
        { adf_ctx_desc_t bytes = desc[0];
          CHECK(adf_qclass_dump_inspect(&cap, desc, s, n, NULL) == ADF_LIMIT && cap == count-1);
          CHECK(!memcmp(&bytes, desc, sizeof(bytes))); }
    }
    if (count == 0) CHECK(adf_qclass_load_str(x, s, n, NULL, NULL) == ADF_OK);
    else {
        int same = 1;
        for (i = 1; i < count; i++)
            same &= fmpz_equal(desc[0].K, desc[i].K) && desc[0].k == desc[i].k &&
                    !memcmp(desc[0].q, desc[i].q, (size_t) desc[0].k * sizeof(ulong));
        CHECK(adf_qclass_load_str(x, s, n, owned[0], NULL) == (same ? ADF_OK : ADF_DOMAIN));
        CHECK(read_q(x, s, n, binds, count, NULL) == ADF_OK);
    }
    /* Release all borrowers before their contexts, including after a one-context load. */
    adf_qclass_clear(x); adf_qclass_init(x);
    for (i = 0; i < count; i++) adf_modctx_free(owned[i]);
    for (i = 0; i < 9; i++) adf_ctx_desc_clear(desc+i);
}

static void golden(void)
{
    golden_file *g; golden_error_t e; size_t i, rows = 0; adf_qclass_t x;
    adf_qclass_init(x);
    CHECK(golden_open("tests/golden/dump.tsv", GOLDEN_DEFAULT_MAX_INPUT, &g, &e));
    for (i = 0; i < golden_count(g); i++) {
        const golden_record *r = golden_record_at(g, i); size_t count = 777;
        if (r->input_len < 14 || memcmp(r->input, "adf1 Q qclass ", 14)) continue;
        rows++;
        if (r->is_status) {
            int st = read_q(x, r->input, r->input_len, NULL, 0, NULL);
            CHECK(!strcmp(adf_status_str(st), r->status));
            CHECK(adf_qclass_dump_inspect(&count, NULL, r->input, r->input_len, NULL) == st && count == 777);
        } else {
            CHECK(adf_qclass_dump_inspect(&count, NULL, r->input, r->input_len, NULL) == ADF_OK);
            check_dump(x, r->input, r->input_len, count);
        }
    }
    CHECK(rows == 13); golden_close(g); adf_qclass_clear(x);
}
static void vectors(void)
{
    jsonl_file *f; jsonl_error_t e; size_t i; adf_qclass_t x; adf_qclass_init(x);
    CHECK(jsonl_open("tests/ref/vectors/q-slice4/dump.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 18);
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i); const char *s = str(field(v, "input"));
        const char *count = jsonl_int_text(field(v, "nctx"), &e); CHECK(count != NULL);
        check_dump(x, s, strlen(s), strtoul(count, NULL, 10));
    }
    jsonl_close(f); adf_qclass_clear(x);
}
static void reject(adf_qclass_t x, const char *s, int want, const adf_text_limits_t *lim)
{
    size_t n = strlen(s), count = 555;
    char *bytes = malloc(n); CHECK(bytes != NULL); memcpy(bytes, s, n);
    { int st = read_q(x, bytes, n, NULL, 0, lim);
      if (st != want) fprintf(stderr, "%s: want %s, got %s\n", s, adf_status_str(want), adf_status_str(st));
      CHECK(st == want); }
    CHECK(adf_qclass_dump_inspect(&count, NULL, bytes, n, lim) == want && count == 555);
    free(bytes);
}
static void malformed(void)
{
    const char *s = "adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0";
    adf_qclass_t x; adf_text_limits_t lim; size_t i, count;
    adf_qclass_init(x); adf_text_limits_default(&lim);
    reject(x, "adf1 Q qclass pieces z", ADF_PARSE, NULL);
    reject(x, "adf1 Q qclass pieces -1", ADF_PARSE, NULL);
    reject(x, "adf1 Q qclass pieces ffffffffffffffffff", ADF_PARSE, NULL);
    reject(x, "adf1 Q qclass pieces 7 1 0 0 0 0 g 0 1 1", ADF_PARSE, NULL);
    reject(x, "adf1 Q qclass pieces 0", ADF_DOMAIN, NULL);
    reject(x, "adf1 Q qclass pieces 1 0 l 1 6 2 2 3 0 0", ADF_DOMAIN, NULL);
    reject(x, "adf1 Q qclass lift 0 g 0 1 1", ADF_DOMAIN, NULL);
    reject(x, "adf1 Q qclass lift 1 0 5 0 0 g 0 1 1", ADF_DOMAIN, NULL);
    reject(x, "adf1 Q qclass lift 1 1 0 40000001 0 g 0 1 1", ADF_DOMAIN, NULL);
    reject(x, "adf1 Q qclass pieces 2 1 1 -1 0 0 g 0 1 1 1 1 -1 0 0 g 0 1 1", ADF_DOMAIN, NULL);
    reject(x, "adf1 Q qclass pieces 1 1 1 -100001 0 0 g 0 1 0", ADF_LIMIT, NULL);
    reject(x, "adf1 Q qclass pieces 1 1 0 0 1 100001 g 0 1 1", ADF_LIMIT, NULL);
    reject(x, "adf1 Q qclass pieces 1 1 0 0 1 -100001 g 0 1 1", ADF_LIMIT, NULL);
    reject(x, "adf1 Q qclass pieces 2 1 3 -1 0 0 g 0 1 1 1 1 -100001 0 0 g 0 1 1",
           ADF_LIMIT, NULL);
    reject(x, "adf1 Q qclass pieces 1 1 1 -100001 0 0 g 0 1 0!", ADF_PARSE, NULL);
    reject(x, "adf2 Q qclass pieces 0", ADF_UNSUPPORTED, NULL);
    reject(x, "adf2 Q qclass pieces 0\x80", ADF_PARSE, NULL);
    reject(x, "adf1 Q qclass pieces 1 1 1 0 0 0 g 0 1 1\n", ADF_PARSE, NULL);
    reject(x, "adf1 Q qclass pieces 1 1 1 -1 0 0 g 0 1 1\x80", ADF_PARSE, NULL);
    lim.max_items = 0; reject(x, "adf1 Q qclass pieces 0", ADF_DOMAIN, &lim);
    reject(x, "adf1 Q qclass pieces 1 1 1 -1 0 0 g 0 1 0", ADF_LIMIT, &lim);
    lim.max_items = 1; reject(x, s, ADF_LIMIT, &lim); lim.max_items = 2;
    count = 0; CHECK(adf_qclass_dump_inspect(&count, NULL, s, strlen(s), &lim) == ADF_OK && count == 2);
    for (i = 0; i < strlen(s); i++) {
        char *bytes = malloc(i ? i : 1); int st; CHECK(bytes != NULL); memcpy(bytes, s, i);
        st = read_q(x, bytes, i, NULL, 0, NULL); CHECK(st != ADF_OK);
        count = 444;
        CHECK(adf_qclass_dump_inspect(&count, NULL, bytes, i, NULL) == st && count == 444);
        free(bytes);
    }
    lim.max_len = 0;
    CHECK(read_q(x, NULL, 1, NULL, 0, &lim) == ADF_LIMIT);
    s = "adf1 Q qclass pieces 1 1 0 0 1 100000 g 0 1 1";
    check_dump(x, s, strlen(s), 0);
    s = "adf1 Q qclass pieces 1 1 0 0 1 -100000 g 0 1 1";
    check_dump(x, s, strlen(s), 0);
    s = "adf1 Q qclass lift 1 1 ffffffffffffffffffffffffffffffffffffffff 0 0 g 0 1 1";
    check_dump(x, s, strlen(s), 0);
    adf_qclass_clear(x);
}
static void binding_order(void)
{
    const char *s = "adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0";
    adf_modctx_struct *a, *b; const adf_modctx_struct *binds[2]; adf_qclass_t x, saved;
    size_t n; char *dump;
    CHECK(adf_modctx_new_from_dump(&a, s, strlen(s), 0, NULL) == ADF_OK);
    CHECK(adf_modctx_new_from_dump(&b, s, strlen(s), 1, NULL) == ADF_OK);
    adf_qclass_init(x); adf_qclass_init(saved); binds[0] = a; binds[1] = b;
    CHECK(read_q(x, s, strlen(s), binds, 2, NULL) == ADF_OK);
    CHECK(x->piece[0].fin.mctx == a && x->piece[1].fin.mctx == b);
    adf_qclass_set(saved, x);
    binds[0] = b; binds[1] = a; CHECK(read_q(x, s, strlen(s), binds, 2, NULL) == ADF_DOMAIN);
    CHECK(adf_qclass_identical(x, saved)); binds[0] = a; binds[1] = b;
    dump = adf_qclass_dump_str(&n, x);
    CHECK(read_q(x, dump, n, binds, 2, NULL) == ADF_OK && adf_qclass_identical(x, saved));
    adf_str_free(dump); adf_qclass_clear(x); adf_qclass_clear(saved); adf_modctx_free(a); adf_modctx_free(b);
    s = "adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 0 0";
    { ulong blocks[2] = {3, 2};
      CHECK(adf_modctx_new_blocks(&a, blocks, 2) == ADF_OK);
      adf_qclass_init(x); binds[0] = a;
      CHECK(read_q(x, s, strlen(s), binds, 1, NULL) == ADF_DOMAIN);
      adf_qclass_clear(x); adf_modctx_free(a); }
}
static uint32_t rng = 310407;
static unsigned rnd(unsigned n) { rng = rng * 1664525U + 1013904223U; return rng % n; }
static void random_roundtrips(void)
{
    adf_qclass_t x, y, lift; adf_adele_struct a[7]; adf_modctx_struct *ctx[2];
    const adf_modctx_struct *binds[7]; unsigned i, j; ulong blocks[2] = {2, 3};
    fmpz_t A, H, d; adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(lift);
    fmpz_init(A); fmpz_init(H); fmpz_init(d); fmpz_one(d);
    for (i = 0; i < 2; i++) CHECK(adf_modctx_new_blocks(ctx+i, blocks+i, 1) == ADF_OK);
    for (i = 0; i < 7; i++) adf_adele_init(a+i);
    for (i = 0; i < 2000; i++) {
        unsigned n = 1 + rnd(7); size_t len, nb = 0; char *dump;
        for (j = 0; j < n; j++) {
            unsigned c = rnd(2); fmpz_set_ui(H, blocks[c]); fmpz_set_ui(A, rnd((unsigned) blocks[c]));
            CHECK(adf_fball_set_fmpz3(&a[j].fin, A, H, d) == ADF_OK);
            CHECK(adf_fball_set_local(&a[j].fin, &a[j].fin, ctx[c]) == ADF_OK);
            arf_set_ui_2exp_si(arb_midref(a[j].inf), rnd(17), -4);
            mag_set_ui_2exp_si(arb_radref(a[j].inf), rnd(5), -4);
        }
        CHECK(adf_qclass_set_pieces(x, a, n, n) == ADF_OK);
        if (i % 2) {
            adf_qclass_set_adele(lift, a); CHECK(adf_qclass_reduce(x, lift, 20, 53) == ADF_OK);
        }
        for (j = 0; j < (unsigned) x->len; j++)
            if (x->piece[j].fin.backend == ADF_LOCAL) binds[nb++] = x->piece[j].fin.mctx;
        dump = adf_qclass_dump_str(&len, x);
        CHECK(dump != NULL && read_q(y, dump, len, binds, nb, NULL) == ADF_OK);
        CHECK(adf_qclass_identical(x, y)); adf_str_free(dump);
        dump = adf_qclass_get_str(&len, x, 2);
        CHECK(dump != NULL && adf_qclass_set_str(y, dump, len, 128, NULL) == ADF_OK);
        for (j = 0; j < (unsigned) x->len; j++) {
            slong k; int found = 0;
            for (k = 0; k < y->len; k++)
                found |= adf_fball_equal_set(&x->piece[j].fin, &y->piece[k].fin) &&
                         arb_contains(y->piece[k].inf, x->piece[j].inf);
            CHECK(found);
        }
        adf_str_free(dump);
    }
    for (i = 0; i < 7; i++) adf_adele_clear(a+i);
    adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(lift);
    for (i = 0; i < 2; i++) adf_modctx_free(ctx[i]);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}
static void debug(void)
{
#ifdef ADF_CHECK_INVARIANTS
    pid_t p = fork(); int status; CHECK(p >= 0);
    if (!p) { adf_qclass_t x; size_t n; adf_qclass_init(x); x->form = 99;
              (void) adf_qclass_dump_str(&n, x); _exit(0); }
    CHECK(waitpid(p, &status, 0) == p && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
#endif
}
int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "--strict-zero-arch")) {
        adf_qclass_t x; adf_qclass_init(x);
        /* Keep the failing 10.1 domain-stage assertion visible. Shared validator is read-only here. */
        reject(x, "adf1 Q qclass pieces 1 0 g 0 1 1", ADF_DOMAIN, NULL); adf_qclass_clear(x);
        return 0;
    }
    golden(); vectors(); malformed(); binding_order(); random_roundtrips(); debug();
    printf("qclass_dump: %lu checks, 2000 random round trips\n", checks); flint_cleanup(); return 0;
}
