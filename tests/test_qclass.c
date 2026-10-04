/* Slice 3.1-a. Exact membership vectors use proto/quotient3_checks.py, P10.
   Every golden lift is checked; union syntax is checked for this slice's status.
   Hand-built PIECES exercise the storage contract without declaring a constructor. */
#include <adelefeld.h>
#include <stddef.h>
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

/* refs/src/flint-3.0.1/memory.rst:16-22: save and restore the allocation callbacks.
   Count live blocks only during a read-only canonicality call after warming the caches.
   This checks temporary cleanup even when LeakSanitizer is unavailable. */
static void *(*saved_alloc)(size_t);
static void *(*saved_calloc)(size_t, size_t);
static void *(*saved_realloc)(void *, size_t);
static void (*saved_free)(void *);
static long live_blocks;
static void *count_alloc(size_t n) { void *p = saved_alloc(n); if (p) live_blocks++; return p; }
static void *count_calloc(size_t n, size_t s)
{ void *p = saved_calloc(n, s); if (p) live_blocks++; return p; }
static void *count_realloc(void *p, size_t n)
{
    void *q = saved_realloc(p, n);
    if (!p && q) live_blocks++;
    if (p && !n && !q) live_blocks--;
    return q;
}
static void count_free(void *p) { if (p) live_blocks--; saved_free(p); }

static void read_adele(adf_adele_t a, const char *s)
{
    CHECK(adf_adele_set_str(a, s, strlen(s), 128, NULL) == ADF_OK);
}

static int qread(adf_qclass_t x, const char *s, size_t n, slong prec, const adf_text_limits_t *lim)
{
    adf_qclass_struct bytes;
    adf_adele_struct piece;
    memcpy(&bytes, x, sizeof(bytes)); memcpy(&piece, x->piece, sizeof(piece));
    int st = adf_qclass_set_str(x, s, n, prec, lim);
    if (st != ADF_OK) {
        CHECK(!memcmp(x, &bytes, sizeof(bytes)));
        CHECK(!memcmp(x->piece, &piece, sizeof(piece)));
    }
    return st;
}

static void pieces(adf_qclass_t x, slong n)
{
    slong i;
    adf_qclass_clear(x);
    x->form = ADF_QCLASS_PIECES;
    x->len = n;
    x->piece = flint_malloc((size_t) n * sizeof(*x->piece));
    for (i = 0; i < n; i++) adf_adele_init(x->piece + i);
}

static void lifecycle(void)
{
    adf_qclass_t x, y;
    adf_adele_t zero;
    adf_qclass_init(x); adf_qclass_init(y); adf_adele_init(zero);
    CHECK(x->form == ADF_QCLASS_LIFT && x->len == 1 && x->piece != NULL);
    CHECK(adf_qclass_form(x) == ADF_QCLASS_LIFT && adf_qclass_length(x) == 1);
    CHECK(adf_adele_identical(x->piece, zero));
    read_adele(x->piece, "(-2.5 +/- 0.125 ; 7/3 mod 4/5)");
    adf_qclass_set(y, x);
    CHECK(y->piece != x->piece && adf_adele_identical(y->piece, x->piece));
    adf_qclass_set(x, x);
    CHECK(adf_adele_identical(y->piece, x->piece));
    adf_qclass_swap(x, x);
    CHECK(adf_adele_identical(y->piece, x->piece));
    pieces(y, 2);
    arb_one(y->piece[1].inf);
    { adf_adele_struct *p = x->piece, *q = y->piece;
      adf_qclass_swap(x, y);
      CHECK(x->piece == q && y->piece == p && x->len == 2 && y->len == 1);
      CHECK(x->form == ADF_QCLASS_PIECES && y->form == ADF_QCLASS_LIFT); }
    adf_qclass_set(y, x);
    CHECK(y->len == 2 && y->form == ADF_QCLASS_PIECES && y->piece != x->piece);
    CHECK(adf_adele_identical(y->piece + 1, x->piece + 1));
    CHECK(adf_qclass_form(x) == ADF_QCLASS_PIECES && adf_qclass_length(x) == 2);
    CHECK(adf_qclass_form(y) == ADF_QCLASS_PIECES && adf_qclass_length(y) == 2);
    CHECK(adf_sizeof_qclass() == sizeof(adf_qclass_struct));
    CHECK(adf_alignof_qclass() == _Alignof(adf_qclass_struct));
    CHECK(sizeof(adf_qclass_struct) == 24 && _Alignof(adf_qclass_struct) == 8);
    CHECK(offsetof(adf_qclass_struct, form) == 0 && offsetof(adf_qclass_struct, len) == 8);
    CHECK(offsetof(adf_qclass_struct, piece) == 16);
    adf_qclass_clear(x); adf_qclass_clear(y); adf_adele_clear(zero);
    { int i; for (i = 0; i < 1000; i++) {
        adf_qclass_init(x); adf_qclass_init(y); adf_qclass_set(y, x);
        CHECK(x->piece != y->piece); adf_qclass_clear(x); adf_qclass_clear(y);
    } }
}

static void canonical(void)
{
    adf_qclass_t x;
    adf_adele_struct *saved;
    adf_modctx_struct *ctx = NULL;
    ulong blocks[] = {4, 3};
    adf_qclass_init(x);
    CHECK(adf_qclass_is_canonical(x));
    x->form = -1; CHECK(!adf_qclass_is_canonical(x)); x->form = 0;
    x->form = 2; CHECK(!adf_qclass_is_canonical(x)); x->form = 0;
    x->len = 0; CHECK(!adf_qclass_is_canonical(x));
    x->len = -1; CHECK(!adf_qclass_is_canonical(x)); x->len = 1;
    saved = x->piece; x->piece = NULL; CHECK(!adf_qclass_is_canonical(x)); x->piece = saved;
    arb_set_si(x->piece->inf, -9); CHECK(adf_qclass_is_canonical(x));
    arb_indeterminate(x->piece->inf); CHECK(!adf_qclass_is_canonical(x)); arb_zero(x->piece->inf);
    fmpz_zero(x->piece->fin.d); CHECK(!adf_qclass_is_canonical(x)); fmpz_one(x->piece->fin.d);
    pieces(x, 2);
    x->len = 0; CHECK(!adf_qclass_is_canonical(x));
    x->len = -1; CHECK(!adf_qclass_is_canonical(x)); x->len = 2;
    saved = x->piece; x->piece = NULL; CHECK(!adf_qclass_is_canonical(x)); x->piece = saved;
    x->form = ADF_QCLASS_LIFT;
    CHECK(!adf_qclass_is_canonical(x)); x->form = ADF_QCLASS_PIECES;
    arb_one(x->piece[1].inf); CHECK(adf_qclass_is_canonical(x));
    arb_set_si(x->piece[0].inf, -1); CHECK(!adf_qclass_is_canonical(x));
    arb_set_si(x->piece[0].inf, 2); CHECK(!adf_qclass_is_canonical(x));
    arb_zero(x->piece[0].inf); arb_add_error_2exp_si(x->piece[0].inf, 1);
    CHECK(adf_qclass_is_canonical(x)); /* spill is legal; midpoint 0, endpoint -2 */
    arb_zero(x->piece[0].inf);
    read_adele(x->piece, "(0 ; 1/2)"); CHECK(!adf_qclass_is_canonical(x));
    read_adele(x->piece, "(0 ; 0 mod 1/2)"); CHECK(!adf_qclass_is_canonical(x));
    read_adele(x->piece, "(0 ; -7)"); CHECK(adf_qclass_is_canonical(x));
    fmpz_set_si(x->piece->fin.H, -1); CHECK(!adf_qclass_is_canonical(x));
    fmpz_zero(x->piece->fin.H);
    read_adele(x->piece, "(0 ; 0 mod 3)");
    fmpz_set_si(x->piece->fin.A, 3); CHECK(!adf_qclass_is_canonical(x));
    fmpz_set_si(x->piece->fin.A, -1); CHECK(!adf_qclass_is_canonical(x));
    fmpz_zero(x->piece->fin.A);
    read_adele(x->piece, "(0 ; 0 mod 3)");
    CHECK(adf_qclass_is_canonical(x));
    adf_adele_swap(x->piece, x->piece + 1); CHECK(!adf_qclass_is_canonical(x));
    adf_adele_swap(x->piece, x->piece + 1);
    adf_adele_set(x->piece + 1, x->piece); CHECK(!adf_qclass_is_canonical(x));
    read_adele(x->piece + 1, "(0 ; 1 mod 3)"); CHECK(adf_qclass_is_canonical(x));
    read_adele(x->piece, "(0 ; 2 mod 3)"); CHECK(!adf_qclass_is_canonical(x));
    read_adele(x->piece, "(0 ; 1 mod 3)"); CHECK(!adf_qclass_is_canonical(x));
    read_adele(x->piece, "(0 ; 0 mod 3)");
    adf_adele_swap(x->piece, x->piece + 1); CHECK(!adf_qclass_is_canonical(x));
    /* Force heap-backed arf temporaries, rather than only inline mantissas. */
    read_adele(x->piece, "(0 ; 0)"); read_adele(x->piece + 1, "(1 ; 0)");
    { fmpz_t mant, exp; int j;
      fmpz_init(mant); fmpz_init_set_si(exp, -2000); fmpz_one(mant);
      fmpz_mul_2exp(mant, mant, 1999); fmpz_add_ui(mant, mant, 1);
      arf_set_fmpz_2exp(arb_midref(x->piece->inf), mant, exp);
      CHECK(adf_qclass_is_canonical(x));
      __flint_get_memory_functions(&saved_alloc, &saved_calloc, &saved_realloc, &saved_free);
      live_blocks = 0;
      __flint_set_memory_functions(count_alloc, count_calloc, count_realloc, count_free);
      for (j = 0; j < 20; j++) {
          CHECK(adf_qclass_is_canonical(x)); CHECK(live_blocks == 0);
      }
      __flint_set_memory_functions(saved_alloc, saved_calloc, saved_realloc, saved_free);
      fmpz_clear(mant); fmpz_clear(exp); }
    read_adele(x->piece, "(0 ; 0 mod 3)");
    read_adele(x->piece + 1, "(0 ; 0 mod 6)"); CHECK(adf_qclass_is_canonical(x));
    adf_adele_swap(x->piece, x->piece + 1); CHECK(!adf_qclass_is_canonical(x));
    read_adele(x->piece, "(0.5 +/- 0.25 ; 0 mod 3)");
    read_adele(x->piece + 1, "(0.625 +/- 0.375 ; 0 mod 3)");
    CHECK(adf_qclass_is_canonical(x)); /* equal lower ends; upper ends decide */
    adf_adele_swap(x->piece, x->piece + 1); CHECK(!adf_qclass_is_canonical(x));
    CHECK(adf_modctx_new_blocks(&ctx, blocks, 2) == ADF_OK);
    read_adele(x->piece, "(0 ; 0 mod 3)");
    read_adele(x->piece + 1, "(0 ; 1 mod 3)");
    CHECK(adf_fball_set_local(&x->piece[0].fin, &x->piece[0].fin, ctx) == ADF_OK);
    CHECK(!fmpz_is_one(x->piece[0].fin.d)); /* raw d=4 cancels to global d=1 */
    CHECK(adf_qclass_is_canonical(x));
    adf_adele_set(x->piece + 1, x->piece);
    adf_fball_set_global(&x->piece[1].fin, &x->piece[1].fin);
    CHECK(!adf_qclass_is_canonical(x)); /* equal keys across backends */
    read_adele(x->piece + 1, "(1 ; 1 mod 3)");
    CHECK(adf_qclass_is_canonical(x));
    { ulong *res = x->piece->fin.res; x->piece->fin.res = NULL;
      CHECK(!adf_qclass_is_canonical(x)); x->piece->fin.res = res; }
    /* Huge exponent gaps must not require materializing exact endpoint mantissas. */
    read_adele(x->piece, "(0 ; 0)"); read_adele(x->piece + 1, "(1 ; 0)");
    mag_one(arb_radref(x->piece->inf));
    fmpz_one(MAG_EXPREF(arb_radref(x->piece->inf)));
    fmpz_mul_2exp(MAG_EXPREF(arb_radref(x->piece->inf)),
                 MAG_EXPREF(arb_radref(x->piece->inf)), 100);
    mag_set(arb_radref(x->piece[1].inf), arb_radref(x->piece[0].inf));
    CHECK(adf_qclass_is_canonical(x)); /* cancellation of enormous equal radii */
    adf_adele_swap(x->piece, x->piece + 1); CHECK(!adf_qclass_is_canonical(x));
    adf_qclass_clear(x); adf_modctx_free(ctx);
}

static void identity_lift(void)
{
    adf_qclass_t x, y, zero;
    adf_adele_t a, b, sentinel;
    adf_rat_t q;
    adf_modctx_struct *ctx = NULL;
    ulong block = 12;
    int i;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(zero);
    adf_adele_init(a); adf_adele_init(b); adf_adele_init(sentinel); adf_rat_init(q);
    CHECK(adf_qclass_identical(x, y));
    read_adele(a, "(-7.5 +/- 0.125 ; 7/3 mod 2)");
    adf_qclass_set_adele(x, a); CHECK(!adf_qclass_identical(x, y));
    CHECK(adf_qclass_get_piece(b, x, 0) == ADF_OK && adf_adele_identical(a, b));
    CHECK(x->form == ADF_QCLASS_LIFT && x->len == 1 && adf_qclass_is_canonical(x));
    adf_qclass_set(y, x); CHECK(adf_qclass_identical(x, y));
    read_adele(sentinel, "(8.5 ; -5/7)"); adf_adele_set(b, sentinel);
    CHECK(adf_qclass_get_piece(b, x, -1) == ADF_DOMAIN && adf_adele_identical(b, sentinel));
    CHECK(adf_qclass_get_piece(b, x, 1) == ADF_DOMAIN && adf_adele_identical(b, sentinel));
    CHECK(adf_qclass_get_piece(b, x, WORD_MAX) == ADF_DOMAIN && adf_adele_identical(b, sentinel));
    for (i = 0; i < 4; i++) {
        if (i == 0) fmpq_zero(q->q);
        if (i == 1) fmpq_set_si(q->q, 1, 2);
        if (i == 2) fmpq_set_si(q->q, -7, 3);
        if (i == 3) {
            fmpz_one(fmpq_numref(q->q));
            fmpz_mul_2exp(fmpq_numref(q->q), fmpq_numref(q->q), 1999);
            fmpz_add_ui(fmpq_numref(q->q), fmpq_numref(q->q), 3);
            fmpz_set_ui(fmpq_denref(q->q), 3); fmpq_canonicalise(q->q);
            CHECK(fmpz_bits(fmpq_numref(q->q)) == 2000);
        }
        adf_qclass_set_rat(x, q); CHECK(adf_qclass_identical(x, zero));
        CHECK(arb_is_zero(x->piece->inf) && adf_fball_is_exact(&x->piece->fin));
    }
    CHECK(adf_modctx_new_blocks(&ctx, &block, 1) == ADF_OK);
    read_adele(a, "(0.5 +/- 0.125 ; 1 mod 3)");
    CHECK(adf_fball_set_local(&a->fin, &a->fin, ctx) == ADF_OK);
    adf_qclass_set_adele(x, a);
    CHECK(adf_qclass_get_piece(b, x, 0) == ADF_OK && adf_adele_identical(a, b));
    CHECK(adf_fball_context(&b->fin) == ctx);
    adf_qclass_set(y, x); CHECK(adf_qclass_identical(x, y));
    adf_fball_set_global(&y->piece->fin, &y->piece->fin);
    CHECK(!adf_qclass_identical(x, y));
    pieces(y, 1); adf_adele_set(y->piece, x->piece); CHECK(!adf_qclass_identical(x, y));
    pieces(y, 2); arb_one(y->piece[1].inf); CHECK(!adf_qclass_identical(y, zero));
    pieces(zero, 1); CHECK(!adf_qclass_identical(y, zero)); /* same form, different length */
    adf_qclass_add_rat(zero, y, q); CHECK(adf_qclass_identical(y, zero));
    adf_fball_set_si(&zero->piece[1].fin, 9); CHECK(!adf_qclass_identical(y, zero));
    adf_qclass_set(zero, y);
    adf_qclass_add_rat(y, y, q); CHECK(adf_qclass_identical(y, zero));
    CHECK(adf_qclass_get_piece(b, y, 1) == ADF_OK && arb_is_one(b->inf));
    adf_qclass_set_adele(y, a); CHECK(adf_qclass_identical(x, y)); /* discard owned PIECES */
    adf_qclass_add_rat(y, x, q); CHECK(adf_qclass_identical(x, y));
    adf_qclass_add_rat(x, x, q); CHECK(adf_qclass_identical(x, y));
    arb_set_fmpz(a->inf, fmpq_numref(q->q));
    adf_fball_set_rat(&a->fin, q);
    adf_qclass_set_adele(x, a);
    CHECK(adf_qclass_get_piece(b, x, 0) == ADF_OK && adf_adele_identical(a, b));
    adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(zero);
    adf_adele_clear(a); adf_adele_clear(b); adf_adele_clear(sentinel); adf_rat_clear(q);
    adf_modctx_free(ctx);
}

static void debug_entries(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int i;
    for (i = 0; i < 11; i++) {
        int status, fd[2]; char message[512]; size_t used = 0; ssize_t n;
        char chunk[512];
        const char *names[] = {"adf_qclass_set:", "adf_qclass_identical:", "adf_qclass_identical:",
            "adf_qclass_set_adele:", "adf_qclass_set_rat:", "adf_qclass_form:", "adf_qclass_length:",
            "adf_qclass_get_piece:", "adf_qclass_add_rat:", "adf_qclass_add_rat:", "adf_qclass_get_str:"};
        CHECK(pipe(fd) == 0);
        pid_t child = fork();
        CHECK(child >= 0);
        if (child == 0) {
            adf_qclass_t x, y; adf_adele_t a; adf_rat_t q; size_t n;
            close(fd[0]);
            if (dup2(fd[1], STDERR_FILENO) < 0) _exit(2);
            close(fd[1]);
            alarm(5);
            adf_qclass_init(x); adf_qclass_init(y); adf_adele_init(a); adf_rat_init(q);
            x->form = 99;
            switch (i) {
                case 0: adf_qclass_set(y, x); break;
                case 1: (void) adf_qclass_identical(x, y); break;
                case 2: (void) adf_qclass_identical(y, x); break;
                case 3: fmpz_zero(a->fin.d); adf_qclass_set_adele(y, a); break;
                case 4: fmpz_zero(fmpq_denref(q->q)); adf_qclass_set_rat(y, q); break;
                case 5: (void) adf_qclass_form(x); break;
                case 6: (void) adf_qclass_length(x); break;
                case 7: (void) adf_qclass_get_piece(a, x, 0); break;
                case 8: adf_qclass_add_rat(y, x, q); break;
                case 9: fmpz_zero(fmpq_denref(q->q)); adf_qclass_add_rat(y, y, q); break;
                case 10: (void) adf_qclass_get_str(&n, x, 6); break;
            }
            adf_qclass_clear(x); adf_qclass_clear(y); adf_adele_clear(a); adf_rat_clear(q);
            _exit(0);
        }
        close(fd[1]);
        while ((n = read(fd[0], chunk, sizeof(chunk))) > 0) {
            size_t take = (size_t) n < sizeof(message)-1-used ? (size_t) n : sizeof(message)-1-used;
            memcpy(message + used, chunk, take); used += take;
        }
        message[used] = '\0'; close(fd[0]);
        CHECK(waitpid(child, &status, 0) == child);
        CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
        CHECK(strstr(message, names[i]) != NULL);
    }
#endif
}

static const jsonl_value *field(const jsonl_value *v, const char *key)
{
    jsonl_error_t e; const jsonl_value *out;
    CHECK(jsonl_field(v, key, &out, &e)); return out;
}

static const char *string(const jsonl_value *v)
{
    jsonl_error_t e; size_t n; const char *s = jsonl_string(v, &n, &e);
    CHECK(s != NULL && strlen(s) == n); return s;
}

/* A rational finite witness (s,w) belongs iff a+w-shift meets the real interval.
   Solve w+t in a+N Zhat: t=a-w+N k. Thus ceil((lo-s-a+w)/N) <= floor((hi-s-a+w)/N).
   N=0 fixes t=a-w. This direct proof is independent of reduction. */
static int member(const adf_adele_t x, const fmpq_t s, const fmpq_t w)
{
    fmpq_t lo, hi, t;
    adf_rat_t a, N;
    fmpz_t lower, upper;
    arf_t l, h;
    int yes;
    fmpq_init(lo); fmpq_init(hi); adf_rat_init(a); adf_rat_init(N); fmpq_init(t);
    fmpz_init(lower); fmpz_init(upper); arf_init(l); arf_init(h);
    arb_get_interval_arf(l, h, x->inf, ARF_PREC_EXACT);
    arf_get_fmpq(lo, l); arf_get_fmpq(hi, h);
    adf_fball_get_center(a, &x->fin); adf_fball_get_radius(N, &x->fin);
    fmpq_sub(t, w, s); fmpq_sub(t, t, a->q);
    fmpq_add(lo, lo, t); fmpq_add(hi, hi, t);
    if (fmpq_is_zero(N->q)) yes = fmpq_sgn(lo) <= 0 && fmpq_sgn(hi) >= 0;
    else {
        fmpq_div(lo, lo, N->q); fmpq_div(hi, hi, N->q);
        fmpz_cdiv_q(lower, fmpq_numref(lo), fmpq_denref(lo));
        fmpz_fdiv_q(upper, fmpq_numref(hi), fmpq_denref(hi));
        yes = fmpz_cmp(lower, upper) <= 0;
    }
    fmpq_clear(lo); fmpq_clear(hi); adf_rat_clear(a); adf_rat_clear(N); fmpq_clear(t);
    fmpz_clear(lower); fmpz_clear(upper); arf_clear(l); arf_clear(h);
    return yes;
}

static void translation(void)
{
    jsonl_file *f = NULL; jsonl_error_t e; size_t i, j;
    adf_qclass_t x, y; adf_adele_t a, b; adf_rat_t q; fmpq_t s, w;
    adf_qclass_init(x); adf_qclass_init(y); adf_adele_init(a); adf_adele_init(b);
    adf_rat_init(q); fmpq_init(s); fmpq_init(w);
    CHECK(jsonl_open("tests/ref/vectors/q-slice1/translation.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 1000);
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i), *points = field(v, "points");
        read_adele(a, string(field(v, "lift")));
        CHECK(adf_rat_set_str(q, string(field(v, "q")), strlen(string(field(v, "q"))), NULL) == ADF_OK);
        adf_qclass_set_adele(x, a); adf_qclass_add_rat(y, x, q);
        CHECK(adf_qclass_identical(x, y));
        adf_qclass_add_rat(x, x, q); CHECK(adf_qclass_identical(x, y));
        CHECK(adf_qclass_get_piece(b, y, 0) == ADF_OK);
        for (j = 0; j < jsonl_size(points); j++) {
            const jsonl_value *p = jsonl_at(points, j, &e); int before, after;
            CHECK(fmpq_set_str(s, string(field(p, "s")), 10) == 0);
            CHECK(fmpq_set_str(w, string(field(p, "w")), 10) == 0);
            CHECK(jsonl_bool(field(p, "before"), &before, &e));
            CHECK(jsonl_bool(field(p, "after"), &after, &e));
            CHECK(member(a, s, w) == before && member(b, s, w) == after);
        }
    }
    jsonl_close(f); adf_qclass_clear(x); adf_qclass_clear(y); adf_adele_clear(a); adf_adele_clear(b);
    adf_rat_clear(q); fmpq_clear(s); fmpq_clear(w);
}

static void text(void)
{
    golden_file *f = NULL; golden_error_t e; size_t i, n, m;
    jsonl_file *gf = NULL; jsonl_error_t je; size_t gi = 0;
    fmpq_t v; arf_t radius;
    adf_qclass_t x, saved, y; adf_text_limits_t lim;
    fmpq_init(v); arf_init(radius);
    adf_qclass_init(x); adf_qclass_init(saved); adf_qclass_init(y);
    read_adele(x->piece, "(-8.5 ; 7/3)"); adf_qclass_set(saved, x);
    CHECK(golden_open("tests/golden/qclass.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &e));
    CHECK(jsonl_open("tests/ref/vectors/q-slice1/golden.jsonl", &gf, &je));
    CHECK(golden_count(f) == 29);
    for (i = 0; i < golden_count(f); i++) {
        const golden_record *r = golden_record_at(f, i);
        int st = qread(x, r->input, r->input_len, 128, NULL);
        if (!strncmp(r->input, "union", 5)) {
            int want = r->is_status && !strcmp(r->status, "PARSE") ? ADF_PARSE : ADF_UNSUPPORTED;
            CHECK(st == want && adf_qclass_identical(x, saved));
        } else if (r->is_status) {
            CHECK(!strcmp(adf_status_str(st), r->status) && adf_qclass_identical(x, saved));
        } else {
            char *s, *t;
            const jsonl_value *rec = jsonl_record(gf, gi++);
            const char *want = string(field(rec, "want")), *second = string(field(rec, "second"));
            CHECK(st == ADF_OK && adf_qclass_is_canonical(x));
            CHECK(strtoul(jsonl_int_text(field(rec, "line"), &je), NULL, 10) == r->line);
            CHECK(fmpq_set_str(v, string(field(rec, "lo")), 10) == 0 && arb_contains_fmpq(x->piece->inf, v));
            CHECK(fmpq_set_str(v, string(field(rec, "hi")), 10) == 0 && arb_contains_fmpq(x->piece->inf, v));
            CHECK(fmpq_set_str(v, string(field(rec, "mid")), 10) == 0);
            { fmpq_t stored; fmpq_init(stored); arf_get_fmpq(stored, arb_midref(x->piece->inf));
              CHECK(fmpq_equal(v, stored));
              CHECK(fmpq_set_str(v, string(field(rec, "rad")), 10) == 0);
              arf_set_mag(radius, arb_radref(x->piece->inf)); arf_get_fmpq(stored, radius);
              CHECK(fmpq_equal(v, stored)); fmpq_clear(stored); }
            s = adf_qclass_get_str(&n, x, 6);
            CHECK(s != NULL && n == strlen(want) && !memcmp(s, want, n));
            CHECK(qread(y, s, n, 128, NULL) == ADF_OK);
            CHECK(arb_contains(y->piece->inf, x->piece->inf));
            CHECK(adf_fball_equal_set(&y->piece->fin, &x->piece->fin));
            t = adf_qclass_get_str(&m, y, 6);
            CHECK(t != NULL && m == strlen(second) && !memcmp(t, second, m));
            adf_str_free(s); adf_str_free(t);
        }
        adf_qclass_set(x, saved);
    }
    CHECK(gi == jsonl_count(gf)); golden_close(f); jsonl_close(gf);
    adf_text_limits_default(&lim); lim.max_len = 0;
    CHECK(qread(x, NULL, 1, ADF_REAL_PREC_MAX + 1, NULL) == ADF_LIMIT);
    CHECK(adf_qclass_identical(x, saved));
    CHECK(qread(x, NULL, 1, 128, &lim) == ADF_LIMIT);
    CHECK(adf_qclass_identical(x, saved));
    CHECK(qread(x, "(0 ; 0) + Q\0", 12, 128, NULL) == ADF_PARSE);
    CHECK(adf_qclass_identical(x, saved));
    adf_text_limits_default(&lim); lim.max_items = 1; lim.max_exp10 = 2;
    { const char *s = "union((0 ; 1/0), (1e3 ; 0)) + Q";
      CHECK(qread(x, s, strlen(s), 128, &lim) == ADF_LIMIT);
      CHECK(adf_qclass_identical(x, saved)); }
    { const char *s = "union((0 ; 1/0), (0 ; 0)) + Q";
      CHECK(qread(x, s, strlen(s), 128, &lim) == ADF_LIMIT);
      CHECK(adf_qclass_identical(x, saved)); }
    lim.max_items = 0;
    CHECK(qread(x, "(0 ; 0) + Q", 11, 128, &lim) == ADF_OK); /* lift != piece count */
    adf_qclass_set(x, saved);
    { const char *s = "(1e3 ; 1/0) + Q";
      CHECK(qread(x, s, strlen(s), 128, &lim) == ADF_LIMIT);
      CHECK(adf_qclass_identical(x, saved)); }
    { const char *s = "union((1e3 ; 1/0)) + q";
      CHECK(qread(x, s, strlen(s), 128, &lim) == ADF_PARSE); }
    { const char *s = "union((1e2 ; 1/0)) + Q";
      lim.max_items = 1;
      CHECK(qread(x, s, strlen(s), 128, &lim) == ADF_UNSUPPORTED);
      lim.max_items = -1; CHECK(qread(x, s, strlen(s), 128, &lim) == ADF_LIMIT); }
    CHECK(qread(x, "(0.125 ; 0) + Q", 15, 0, NULL) == ADF_OK);
    CHECK(arb_contains_si(x->piece->inf, 0) == 0);
    adf_qclass_set(x, saved);
    fmpz_set_si(ARF_EXPREF(arb_midref(x->piece->inf)), ADF_PRINT_EXP_MAX + 1);
    CHECK(adf_qclass_get_str(&n, x, 6) == NULL && n == 0);
    adf_qclass_set(x, saved); mag_one(arb_radref(x->piece->inf));
    fmpz_set_si(MAG_EXPREF(arb_radref(x->piece->inf)), ADF_PRINT_EXP_MAX + 1);
    CHECK(adf_qclass_get_str(&n, x, 6) == NULL && n == 0);
    pieces(x, 1); CHECK(adf_qclass_get_str(&n, x, 6) == NULL && n == 0);
    adf_qclass_clear(x); adf_qclass_clear(saved); adf_qclass_clear(y);
    fmpq_clear(v); arf_clear(radius);
}

int main(int argc, char **argv)
{
    const char *only = argc == 2 ? argv[1] : "all";
#define RUN(name) if (!strcmp(only, "all") || !strcmp(only, #name)) name()
    RUN(lifecycle); RUN(canonical); RUN(identity_lift); RUN(translation); RUN(text); RUN(debug_entries);
    printf("test_qclass: %lu checks; group %s\n", checks, only);
    flint_cleanup(); return 0;
}
