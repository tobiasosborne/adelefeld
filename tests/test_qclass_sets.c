/* Q2 exact set queries. Oracle: proto/quotient3_checks.py; independent cover: q-review3/model.py.
   Sources: docs/proofs/quotient.md:71,129,181,212; docs/api-3.md:546-574; SPEC N-D21. */
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

static int watch_bulk;
static ulong bulk_allocations, live_bulk, live_integers;
static void *observed_bulk(size_t n)
{
    void *p;
    if (watch_bulk) bulk_allocations++;
    live_bulk++; p = flint_malloc(n); memset(p, 0xa5, n); return p;
}
static void observed_free(void *p) { if (p) live_bulk--; flint_free(p); }
static void observed_zinit(fmpz_t z) { live_integers++; fmpz_init(z); }
static void observed_zinit_si(fmpz_t z, slong n) { live_integers++; fmpz_init_set_si(z, n); }
static void observed_zclear(fmpz_t z) { live_integers--; fmpz_clear(z); }
/* Compile the same query source to check preflight placement and exact saturation directly.
   Driver/Julia calls link the archive normally. This mirrors test_qclass_reduce's exact-kernel checks. */
#define flint_malloc observed_bulk
#define flint_free observed_free
#define fmpz_init observed_zinit
#define fmpz_init_set_si observed_zinit_si
#define fmpz_clear observed_zclear
#include "../src/qclass_sets.c"
#undef flint_malloc
#undef flint_free
#undef fmpz_init
#undef fmpz_init_set_si
#undef fmpz_clear

static ulong checks, calls;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "sets line %d: %s\n", __LINE__, #c); abort(); } } while (0)
typedef int (*query)(int *, const adf_qclass_t, const adf_qclass_t, slong);
static query queries[] = {adf_qclass_equal_set, adf_qclass_contains, adf_qclass_overlaps};
static int stages = 1;
static const char *case_name = "bounds";

static const jsonl_value *field(const jsonl_value *v, const char *k)
{
    const jsonl_value *r; jsonl_error_t e; CHECK(jsonl_field(v, k, &r, &e)); return r;
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
    int b; jsonl_error_t e; CHECK(jsonl_bool(v, &b, &e)); return b;
}
static void rat(fmpq_t q, const jsonl_value *v) { CHECK(!fmpq_set_str(q, str(v), 10)); }
static void input(adf_qclass_t x, const jsonl_value *v)
{
    const jsonl_value *ps = field(v, "pieces"); slong i; arf_t r; fmpq_t q, t;
    adf_rat_t a, N; fmpz_t mant, exp;
    adf_qclass_clear(x); x->form = (int) integer(field(v, "form")); x->len = (slong) jsonl_size(ps);
    x->piece = flint_malloc((size_t) x->len*sizeof(*x->piece));
    arf_init(r); fmpq_init(q); fmpq_init(t); adf_rat_init(a); adf_rat_init(N);
    fmpz_init(mant); fmpz_init(exp);
    for (i = 0; i < x->len; i++) {
        adf_adele_struct *p = x->piece+i; const jsonl_value *j = at(ps, (size_t) i);
        adf_adele_init(p); rat(q, field(j, "mid"));
        arf_set_fmpq(arb_midref(p->inf), q, ARF_PREC_EXACT, ARF_RND_NEAR);
        rat(q, field(j, "rad")); arf_set_fmpq(r, q, ARF_PREC_EXACT, ARF_RND_NEAR);
        if (arf_is_zero(r)) mag_zero(arb_radref(p->inf));
        else {
            arf_get_fmpz_2exp(mant, exp, r); CHECK(fmpz_bits(mant) <= 30);
            fmpz_mul_2exp(mant, mant, 30-fmpz_bits(mant));
            MAG_MAN(arb_radref(p->inf)) = fmpz_get_ui(mant);
            fmpz_set(MAG_EXPREF(arb_radref(p->inf)), ARF_EXPREF(r));
        }
        arf_set_mag(r, arb_radref(p->inf)); arf_get_fmpq(t, r); CHECK(fmpq_equal(t, q));
        arf_get_fmpq(t, arb_midref(p->inf)); fmpq_sub(t, t, q);
        rat(a->q, field(j, "lo")); CHECK(fmpq_equal(t, a->q));
        fmpq_mul_2exp(q, q, 1); fmpq_add(t, t, q);
        rat(a->q, field(j, "hi")); CHECK(fmpq_equal(t, a->q));
        rat(a->q, field(j, "a")); rat(N->q, field(j, "N"));
        CHECK(adf_fball_set_center_radius(&p->fin, a, N) == ADF_OK);
    }
    CHECK(adf_qclass_is_canonical(x));
    arf_clear(r); fmpq_clear(q); fmpq_clear(t); adf_rat_clear(a); adf_rat_clear(N);
    fmpz_clear(mant); fmpz_clear(exp);
}
/* Direct rational translation membership, without reduction or boundary gluing. */
static int member(const adf_qclass_t x, const fmpq_t s, const fmpq_t w)
{
    slong i; int yes = 0; fmpq_t m, r, lo, hi, t; arf_t ar; fmpz_t l, h; adf_rat_t a, N;
    fmpq_init(m); fmpq_init(r); fmpq_init(lo); fmpq_init(hi); fmpq_init(t); arf_init(ar);
    fmpz_init(l); fmpz_init(h); adf_rat_init(a); adf_rat_init(N);
    for (i = 0; i < x->len && !yes; i++) {
        arf_get_fmpq(m, arb_midref(x->piece[i].inf));
        arf_set_mag(ar, arb_radref(x->piece[i].inf)); arf_get_fmpq(r, ar);
        fmpq_sub(lo, m, r); fmpq_add(hi, m, r);
        adf_fball_get_center(a, &x->piece[i].fin); adf_fball_get_radius(N, &x->piece[i].fin);
        fmpq_sub(t, w, s); fmpq_sub(t, t, a->q); fmpq_add(lo, lo, t); fmpq_add(hi, hi, t);
        if (fmpq_is_zero(N->q)) yes = fmpq_sgn(lo) <= 0 && fmpq_sgn(hi) >= 0;
        else {
            fmpq_div(lo, lo, N->q); fmpq_div(hi, hi, N->q);
            fmpz_cdiv_q(l, fmpq_numref(lo), fmpq_denref(lo));
            fmpz_fdiv_q(h, fmpq_numref(hi), fmpq_denref(hi)); yes = fmpz_cmp(l, h) <= 0;
        }
    }
    fmpq_clear(m); fmpq_clear(r); fmpq_clear(lo); fmpq_clear(hi); fmpq_clear(t); arf_clear(ar);
    fmpz_clear(l); fmpz_clear(h); adf_rat_clear(a); adf_rat_clear(N); return yes;
}
static int call(int which, const adf_qclass_t x, const adf_qclass_t y, slong limit, int status, int expected)
{
    int truth = 719, got; ulong saved_bulk = live_bulk, saved_z = live_integers;
    calls++; got = queries[which](&truth, x, y, limit);
    CHECK(live_bulk == saved_bulk); CHECK(live_integers == saved_z);
    if (got != status || truth != (status == ADF_OK ? expected : 719))
        fprintf(stderr, "%s query %d limit %ld: got (%d,%d), expected (%d,%d)\n",
                case_name, which, limit, got, truth, status, status == ADF_OK ? expected : 719);
    CHECK(got == status);
    CHECK(truth == (status == ADF_OK ? expected : 719)); return truth;
}
static void vectors(void)
{
    jsonl_file *f; jsonl_error_t e; size_t i, j; adf_qclass_t x, y, savedx, savedy; fmpq_t s, w;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(savedx); adf_qclass_init(savedy);
    fmpq_init(s); fmpq_init(w);
    CHECK(jsonl_open("tests/ref/vectors/q-slice6/sets.jsonl", &f, &e));
    for (i = 0; i < jsonl_count(f); i++) {
        const jsonl_value *v = jsonl_record(f, i), *pts = field(v, "points");
        slong K = integer(field(v, "K")), L = integer(field(v, "L")), E = integer(field(v, "E"));
        slong budget = integer(field(v, "budget")); int t[2][3] = {{0}}, d, k;
        case_name = str(field(v, "name"));
        CHECK(budget == (2*E+1)*K*L); input(x, field(v, "x")); input(y, field(v, "y"));
        adf_qclass_set(savedx, x); adf_qclass_set(savedy, y);
        { slong raw = 0, modulus = 0;
          CHECK(qs_preflight(&raw, &modulus, x, y, budget)); CHECK(raw == K && modulus == L); }
        for (j = 0; j < jsonl_size(pts); j++) {
            const jsonl_value *p = at(pts, j); rat(s, at(p, 0)); rat(w, at(p, 1));
            CHECK(member(x, s, w) == boolean(at(p, 2))); CHECK(member(y, s, w) == boolean(at(p, 3)));
        }
        for (d = 0; d < 2; d++) for (k = 0; k < stages; k++) {
            const adf_qclass_struct *a = d ? y : x, *b = d ? x : y;
            int expected = boolean(at(field(v, d ? "yx" : "xy"), (size_t) k));
            t[d][k] = call(k, a, b, budget, ADF_OK, expected);
            bulk_allocations = 0; watch_bulk = 1;
            call(k, a, b, budget-1, ADF_LIMIT, 0);
            watch_bulk = 0; CHECK(bulk_allocations == 0);
            call(k, a, b, 0, ADF_LIMIT, 0); call(k, a, b, -1, ADF_LIMIT, 0);
            call(k, a, a, LONG_MAX, ADF_OK, 1);
        }
        if (stages > 1) CHECK(t[0][0] == (t[0][1] && t[1][1]));
        if (stages > 2) CHECK((!t[0][1] || t[0][2]) && (!t[1][1] || t[1][2]));
        CHECK(adf_qclass_identical(x, savedx) && adf_qclass_identical(y, savedy));
    }
    case_name = "bounds";
    printf("sets: %zu vectors, both orders, %d query types\n", jsonl_count(f), stages);
    jsonl_close(f); adf_qclass_clear(x); adf_qclass_clear(y);
    adf_qclass_clear(savedx); adf_qclass_clear(savedy); fmpq_clear(s); fmpq_clear(w);
}
static void limits(void)
{
    jsonl_file *f; jsonl_error_t e; const jsonl_value *v; adf_qclass_t x, y; int k;
    clock_t start; double sec; fmpz_t z, exp; slong i;
    adf_qclass_init(x); adf_qclass_init(y); fmpz_init(z); fmpz_init(exp);
    CHECK(jsonl_open("tests/ref/vectors/q-slice6/limits.jsonl", &f, &e));
    CHECK(jsonl_count(f) == 1); v = jsonl_record(f, 0); input(x, field(v, "x"));
    start = clock();
    for (k = 0; k < stages; k++) call(k, x, y, integer(field(v, "limit")), ADF_LIMIT, 0);
    sec = (double) (clock()-start)/CLOCKS_PER_SEC; CHECK(sec < 1.0);
    printf("10^100 B preflight: %.6f CPU s (guard 1 s)\n", sec); jsonl_close(f);
    arb_zero(x->piece->inf); adf_fball_zero(&x->piece->fin);
    mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, 100);
    for (k = 0; k < stages; k++) call(k, x, y, LONG_MAX, ADF_LIMIT, 0);
    arb_zero(x->piece->inf); fmpz_set_si(exp, ADF_QCLASS_EXP_MAX+1); fmpz_one(z);
    arf_set_fmpz_2exp(arb_midref(x->piece->inf), z, exp);
    for (k = 0; k < stages; k++) call(k, x, y, LONG_MAX, ADF_LIMIT, 0);
    fmpz_set_si(exp, -ADF_QCLASS_EXP_MAX-2);
    arf_set_fmpz_2exp(arb_midref(x->piece->inf), z, exp);
    for (k = 0; k < stages; k++) call(k, x, y, LONG_MAX, ADF_LIMIT, 0);
    /* 2000 distinct legal pieces with L near LONG_MAX. Products must not wrap. */
    adf_qclass_clear(x); x->form = ADF_QCLASS_PIECES; x->len = 2000;
    x->piece = flint_malloc((size_t) x->len*sizeof(*x->piece));
    for (i = 0; i < x->len; i++) {
        adf_adele_init(x->piece+i); arb_set_si(x->piece[i].inf, i);
        arb_mul_2exp_si(x->piece[i].inf, x->piece[i].inf, -11);
        fmpz_zero(x->piece[i].fin.A); fmpz_set_si(x->piece[i].fin.H, LONG_MAX-1);
    }
    CHECK(adf_qclass_is_canonical(x));
    bulk_allocations = 0; watch_bulk = 1;
    for (k = 0; k < stages; k++) call(k, x, y, LONG_MAX, ADF_LIMIT, 0);
    watch_bulk = 0; CHECK(bulk_allocations == 0);
    printf("2000 pieces, L=LONG_MAX-1: %lu bulk allocations before LIMIT\n", bulk_allocations);
    adf_qclass_clear(x); adf_qclass_clear(y); fmpz_clear(z); fmpz_clear(exp);
}
static void exact_and_local(void)
{
    adf_qclass_t x, y, rx, ry; fmpz_t z, exp, total, count, cap;
    adf_modctx_struct *ctx = NULL; const ulong blocks[] = {4, 3}; int k;
    adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(rx); adf_qclass_init(ry);
    fmpz_init(z); fmpz_init(exp); fmpz_init(total); fmpz_init(count); fmpz_init(cap);
    arb_set_d(x->piece->inf, 0.5); arb_set_d(y->piece->inf, 0.5);
    fmpz_one(z); fmpz_mul_2exp(z, z, 99); fmpz_add_ui(z, z, 1); fmpz_set_si(exp, -100);
    arf_set_fmpz_2exp(arb_midref(y->piece->inf), z, exp);
    fmpz_set_ui(x->piece->fin.H, 3); fmpz_set_ui(y->piece->fin.H, 3);
    for (k = 0; k < stages; k++) call(k, x, y, 100, ADF_OK, 0);
    CHECK(adf_qclass_reduce(rx, x, 1, 2) == ADF_OK);
    CHECK(adf_qclass_reduce(ry, y, 1, 2) == ADF_OK);
    if (stages > 1) call(1, rx, ry, 100, ADF_OK, 1);
    if (stages > 2) call(2, rx, ry, 100, ADF_OK, 1);
    /* Local raw denominator cancels to 1. Canonical finite data must drive the query. */
    adf_qclass_set(y, x); CHECK(adf_modctx_new_blocks(&ctx, blocks, 2) == ADF_OK);
    CHECK(adf_fball_set_local(&y->piece->fin, &y->piece->fin, ctx) == ADF_OK);
    CHECK(!fmpz_is_one(y->piece->fin.d));
    for (k = 0; k < stages; k++) call(k, x, y, 100, ADF_OK, 1);
    /* Exact cap+1 saturation cannot wrap at LONG_MAX, even when one range is 2^101. */
    fmpz_one(count); fmpz_mul_2exp(count, count, 101); fmpz_set_si(total, LONG_MAX);
    qs_accumulate(total, count, LONG_MAX); fmpz_set_si(cap, LONG_MAX); fmpz_add_ui(cap, cap, 1);
    CHECK(fmpz_equal(total, cap));
    fmpz_set_si(total, LONG_MAX-1); fmpz_set_ui(count, 1); qs_accumulate(total, count, LONG_MAX);
    CHECK(fmpz_equal_si(total, LONG_MAX)); qs_accumulate(total, count, LONG_MAX);
    CHECK(fmpz_equal(total, cap));
    fmpz_clear(z); fmpz_clear(exp); fmpz_clear(total); fmpz_clear(count); fmpz_clear(cap);
    adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(rx); adf_qclass_clear(ry);
    adf_modctx_free(ctx);
}
static void preflight_components(void)
{
    fmpz_t total, count, cap, mant, exp; arf_t a; fmpq_t oversized;
    fmpz_init(total); fmpz_init(count); fmpz_init(cap); fmpz_init(mant); fmpz_init(exp); arf_init(a);
    fmpq_init(oversized);
    fmpz_set_si(total, LONG_MAX); fmpz_one(count);
    qs_accumulate(total, count, LONG_MAX); fmpz_set_si(cap, LONG_MAX); fmpz_add_ui(cap, cap, 1);
    CHECK(fmpz_equal(total, cap));
    fmpz_one(count); fmpz_mul_2exp(count, count, 101); qs_accumulate(total, count, LONG_MAX);
    CHECK(fmpz_equal(total, cap));
    fmpz_one(mant); fmpz_mul_2exp(mant, mant, ADF_QCLASS_BITS_MAX-1); fmpz_add_ui(mant, mant, 1);
    fmpz_set_si(exp, -ADF_QCLASS_BITS_MAX); arf_set_fmpz_2exp(a, mant, exp);
    CHECK(!qs_arf_bound(a)); /* Exact denominator has BITS_MAX+1 bits, even though e=0. */
    fmpz_one(mant); fmpz_mul_2exp(mant, mant, ADF_QCLASS_BITS_MAX-2); fmpz_add_ui(mant, mant, 1);
    fmpz_add_ui(exp, exp, 1); arf_set_fmpz_2exp(a, mant, exp); CHECK(qs_arf_bound(a));
    fmpz_one(fmpq_numref(oversized));
    fmpz_mul_2exp(fmpq_numref(oversized), fmpq_numref(oversized), ADF_QCLASS_BITS_MAX);
    CHECK(!qs_size(oversized));
    fmpz_one(fmpq_numref(oversized)); fmpz_one(fmpq_denref(oversized));
    fmpz_mul_2exp(fmpq_denref(oversized), fmpq_denref(oversized), ADF_QCLASS_BITS_MAX);
    CHECK(!qs_size(oversized));
    fmpz_fdiv_q_2exp(fmpq_denref(oversized), fmpq_denref(oversized), 1); CHECK(qs_size(oversized));
    CHECK(qs_storage_status(2) == ADF_OK); CHECK(qs_storage_status(LONG_MAX) == ADF_LIMIT);
    if (sizeof(slong) == 8) {
        adf_qclass_t x, y; clock_t start; double sec;
        CHECK(qs_storage_status((slong) (UWORD(1) << 59)) == ADF_LIMIT);
        adf_qclass_init(x); adf_qclass_init(y); adf_fball_one(&x->piece->fin);
        mag_set_ui_2exp_si(arb_radref(x->piece->inf), 1, 58);
        start = clock(); call(0, x, y, LONG_MAX, ADF_LIMIT, 0);
        sec = (double) (clock()-start)/CLOCKS_PER_SEC; CHECK(sec < 1.0);
        printf("2^59 byte-product preflight: %.6f CPU s (guard 1 s)\n", sec);
        adf_qclass_clear(x); adf_qclass_clear(y);
    }
    fmpz_clear(total); fmpz_clear(count); fmpz_clear(cap);
    fmpz_clear(mant); fmpz_clear(exp); arf_clear(a); fmpq_clear(oversized);
}
static void debug_entry(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int k, side;
    fflush(NULL);
    for (k = 0; k < stages; k++) for (side = 0; side < 2; side++) {
        pid_t pid = fork(); int status; CHECK(pid >= 0);
        if (!pid) {
            adf_qclass_t x, y; int truth; alarm(5);
            if (!freopen("/dev/null", "w", stderr)) _exit(2);
            adf_qclass_init(x); adf_qclass_init(y); (side ? y : x)->form = 8;
            queries[k](&truth, x, y, 0); _exit(0);
        }
        CHECK(waitpid(pid, &status, 0) == pid);
        CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
    printf("INV: %d invalid input aborts\n", 2*stages);
#endif
}
int main(int argc, char **argv)
{
    if (argc > 1) stages = atoi(argv[1]); else stages = 3;
    CHECK(stages >= 1 && stages <= 3); preflight_components(); vectors(); limits();
    exact_and_local(); debug_entry();
    printf("qclass sets: %lu checks, %lu calls\n", checks, calls); flint_cleanup(); return 0;
}
