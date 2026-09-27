/* bench/bench_word.c: matched word-kernel rows for the adelefeld benchmark.
   Contract: docs/PERF.md section 7.  Build and run: make -C bench run.
   The compiled hot loops of the chain kernels are saved under bench/asm. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "harness.h"

#include <flint/flint.h>
#include <flint/nmod.h>
#include <flint/ulong_extras.h>

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "unknown"
#endif

/* The modulus of the baseline: p = 2^62 - 57, 62 bits. */
#define BENCH_MODULUS ((UWORD(1) << 62) - 57)

/* Operations per run.  Kept so that each benchmark stays well under 5 s on a
   quiet machine and does not run long on a busy one. */
#define CHAIN_ADD_REPS  20000000ULL
#define CHAIN_MUL_REPS  10000000ULL
#define CHAIN_MUL2_REPS  5000000ULL
#define BATCH_N             1024
#define BATCH_ADD_PASSES  4096ULL
#define BATCH_MUL_PASSES  2048ULL
#define BATCH_MUL2_PASSES 2048ULL

typedef struct {
    nmod_t mod;
    ulong mod_n;
    ulong ninv;
    ulong b;        /* fixed second operand of a chain */
    ulong x0;       /* start value of a chain */
    ulong acc;      /* result, read only after the timed region */
    ulong *a;       /* batch left operands */
    ulong *v;       /* batch right operands */
    ulong *out;     /* batch results */
    ulong n;        /* batch length */
} word_ctx;

/* ---------------------------------------------------------------- kernels */
/* These four functions are the timed hot loops.  Each chain loop feeds its
   own result back as the next left operand; the dependency is visible in
   bench/asm (see bench/README.md). */

static __attribute__((noinline))
void chain_add_run(void *v, unsigned long long calls)
{
    word_ctx *c = (word_ctx *) v;
    ulong x = c->x0;
    const ulong b = c->b;
    const nmod_t mod = c->mod;
    unsigned long long i;
    for (i = 0; i < calls; i++)
        x = nmod_add(x, b, mod);
    c->acc = x;
}

static __attribute__((noinline))
void chain_mul_run(void *v, unsigned long long calls)
{
    word_ctx *c = (word_ctx *) v;
    ulong x = c->x0;
    const ulong b = c->b;
    const nmod_t mod = c->mod;
    unsigned long long i;
    for (i = 0; i < calls; i++)
        x = nmod_mul(x, b, mod);
    c->acc = x;
}

static __attribute__((noinline))
void chain_mul2_run(void *v, unsigned long long calls)
{
    word_ctx *c = (word_ctx *) v;
    ulong x = c->x0;
    const ulong b = c->b;
    const ulong mod_n = c->mod_n;
    const ulong ninv = c->ninv;
    unsigned long long i;
    for (i = 0; i < calls; i++)
        x = n_mulmod2_preinv(x, b, mod_n, ninv);
    c->acc = x;
}

static __attribute__((noinline))
void batch_add_run(void *v, unsigned long long passes)
{
    word_ctx *c = (word_ctx *) v;
    const ulong n = c->n;
    const ulong b = c->b;
    const nmod_t mod = c->mod;
    ulong *a = c->a, *out = c->out;
    unsigned long long p;
    ulong i;
    (void) b;
    for (p = 0; p < passes; p++)
        for (i = 0; i < n; i++)
            out[i] = nmod_add(a[i], c->v[i], mod);
    c->acc = out[n - 1];
}

static __attribute__((noinline))
void batch_mul_run(void *v, unsigned long long passes)
{
    word_ctx *c = (word_ctx *) v;
    const ulong n = c->n;
    const nmod_t mod = c->mod;
    ulong *a = c->a, *out = c->out;
    unsigned long long p;
    ulong i;
    for (p = 0; p < passes; p++)
        for (i = 0; i < n; i++)
            out[i] = nmod_mul(a[i], c->v[i], mod);
    c->acc = out[n - 1];
}

static __attribute__((noinline))
void batch_mul2_run(void *v, unsigned long long passes)
{
    word_ctx *c = (word_ctx *) v;
    const ulong n = c->n;
    const ulong mod_n = c->mod_n;
    const ulong ninv = c->ninv;
    ulong *a = c->a, *out = c->out;
    unsigned long long p;
    ulong i;
    for (p = 0; p < passes; p++)
        for (i = 0; i < n; i++)
            out[i] = n_mulmod2_preinv(a[i], c->v[i], mod_n, ninv);
    c->acc = out[n - 1];
}

/* -------------------------------------------------------------- the cases */

typedef struct {
    const char *name;
    bench_kind kind;
    const char *operand_family;
    char bit_lengths[96];
    int warm;
    int allocating;
    bench_fn fn;
    unsigned long long calls;
    unsigned long long ops_per_call;
    word_ctx ctx;
} word_case;

static void ctx_init(word_ctx *c, ulong p)
{
    memset(c, 0, sizeof(*c));
    c->mod_n = p;
    nmod_init(&c->mod, p);
    c->ninv = c->mod.ninv;
}

static ulong bits_of(ulong x)
{
    return x == 0 ? 0 : (ulong) n_sizeinbase(x, 2);
}

/* A residue below p with its 61st bit set, so that the chain operands are
   full width for p = 2^62 - 57. */
static ulong rand_full(flint_rand_t state, ulong p)
{
    ulong lo = UWORD(1) << 61;
    return n_randint(state, p - lo - 1) + lo;
}

static ulong max_bits(const ulong *p, ulong n)
{
    ulong i, m = 0;
    for (i = 0; i < n; i++) {
        ulong b = bits_of(p[i]);
        if (b > m)
            m = b;
    }
    return m;
}

static unsigned long long g_checksum = 0;
static unsigned long long g_seed = 0;

static void report_case(FILE *f, word_case *wc, int trials)
{
    bench_stats st = bench_measure(wc->fn, &wc->ctx, wc->calls,
                                   wc->ops_per_call, trials, 1);
    unsigned long long ops = wc->calls * wc->ops_per_call;
    g_checksum ^= (unsigned long long) wc->ctx.acc;
    fprintf(f, "%s\t%s\t%s\t%s\t%llu\t%s\t%s\t%llu\t%llu\t%llu\t%.4f\t%.4f\t%.4f\n",
            wc->name, bench_kind_name(wc->kind), wc->operand_family,
            wc->bit_lengths, g_seed,
            wc->warm ? "yes" : "no", wc->allocating ? "yes" : "no",
            wc->calls, ops, (unsigned long long) wc->ctx.acc,
            st.min, st.median, st.max);
    fflush(f);
    printf("%-18s %-17s min %8.3f  median %8.3f  max %8.3f ns/op\n",
           wc->name, bench_kind_name(wc->kind), st.min, st.median, st.max);
}

static void write_header(FILE *f, const char *run_id, const char *model,
                         int requested_cpu, int actual_cpu,
                         const bench_tsc_calib *cal, int trials, int cores)
{
    char gmp[64];
    snprintf(gmp, sizeof(gmp), "%d.%d.%d", __GNU_MP_VERSION,
             __GNU_MP_VERSION_MINOR, __GNU_MP_VERSION_PATCHLEVEL);
    fprintf(f, "run_id: %s\n", run_id);
    fprintf(f, "tag: word\n");
    fprintf(f, "quiet_machine: no\n");
    fprintf(f, "machine: %s\n", model);
    fprintf(f, "cores_online: %d\n", cores);
    fprintf(f, "affinity_requested_cpu: %d\n", requested_cpu);
    fprintf(f, "affinity_actual_cpu: %d\n", actual_cpu);
    fprintf(f, "affinity_method: sched_setaffinity\n");
    fprintf(f, "compiler: gcc %s\n", __VERSION__);
    fprintf(f, "flags: %s\n", BENCH_CFLAGS);
    fprintf(f, "flint_version: %s\n", FLINT_VERSION);
    fprintf(f, "gmp_version: %s\n", gmp);
    fprintf(f, "clock_source: clock_gettime(CLOCK_MONOTONIC_RAW)\n");
    fprintf(f, "tsc_present: %s\n", bench_tsc_present() ? "yes" : "no");
    fprintf(f, "tsc_invariant: %s\n", bench_tsc_invariant() ? "yes" : "no");
    fprintf(f, "cycles_per_ns_trials: %d\n", cal->trials);
    fprintf(f, "cycles_per_ns_min: %.5f\n", cal->cycles_per_ns_min);
    fprintf(f, "cycles_per_ns_median: %.5f\n", cal->cycles_per_ns_median);
    fprintf(f, "cycles_per_ns_max: %.5f\n", cal->cycles_per_ns_max);
    fprintf(f, "cycles_per_ns_spread: %.5f\n",
            cal->cycles_per_ns_max - cal->cycles_per_ns_min);
    fprintf(f, "seed: %llu\n", g_seed);
    fprintf(f, "modulus_bits: 62\n");
    fprintf(f, "modulus: %lu\n", (unsigned long) BENCH_MODULUS);
    fprintf(f, "trials_per_case: %d\n", trials);
    fprintf(f, "batch_length: %d\n", BATCH_N);
    fprintf(f, "asm_dir: bench/asm\n");
    fprintf(f, "asm_files: chain_add_run.s,chain_mul_run.s,chain_mul2_run.s\n");
    fprintf(f, "warm: yes\n");
    fprintf(f, "allocating: no\n");
    fprintf(f, "\n");
    fprintf(f, "# one table follows; columns are tab separated\n");
    fprintf(f, "# name\tkind\toperand_family\tbit_lengths\tseed\twarm\t"
               "allocating\tcalls\tops\tchecksum\tns_per_op_min\t"
               "ns_per_op_median\tns_per_op_max\n");
}

static void fill_chain(word_case *wc, const char *name, const char *family,
                       bench_fn fn, unsigned long long calls, ulong p,
                       ulong b, ulong x0)
{
    memset(wc, 0, sizeof(*wc));
    ctx_init(&wc->ctx, p);
    wc->name = name;
    wc->kind = BENCH_LATENCY_CHAIN;
    wc->operand_family = family;
    wc->warm = 1;
    wc->allocating = 0;
    wc->fn = fn;
    wc->calls = calls;
    wc->ops_per_call = 1;
    wc->ctx.b = b;
    wc->ctx.x0 = x0;
    snprintf(wc->bit_lengths, sizeof(wc->bit_lengths),
             "modulus 62 bit; b %lu bit; x0 %lu bit", bits_of(b), bits_of(x0));
}

static void fill_batch(word_case *wc, const char *name, const char *family,
                       bench_fn fn, unsigned long long passes, ulong p,
                       flint_rand_t state)
{
    ulong i;
    memset(wc, 0, sizeof(*wc));
    ctx_init(&wc->ctx, p);
    wc->name = name;
    wc->kind = BENCH_INDEPENDENT_BATCH;
    wc->operand_family = family;
    wc->warm = 1;
    wc->allocating = 0;
    wc->fn = fn;
    wc->calls = passes;
    wc->ops_per_call = BATCH_N;
    wc->ctx.n = BATCH_N;
    wc->ctx.a = (ulong *) malloc(BATCH_N * sizeof(ulong));
    wc->ctx.v = (ulong *) malloc(BATCH_N * sizeof(ulong));
    wc->ctx.out = (ulong *) malloc(BATCH_N * sizeof(ulong));
    for (i = 0; i < BATCH_N; i++) {
        wc->ctx.a[i] = n_randint(state, p);
        wc->ctx.v[i] = n_randint(state, p);
    }
    snprintf(wc->bit_lengths, sizeof(wc->bit_lengths),
             "modulus 62 bit; a %lu bit; b %lu bit (max over arrays)",
             max_bits(wc->ctx.a, BATCH_N), max_bits(wc->ctx.v, BATCH_N));
}

static void free_case(word_case *wc)
{
    free(wc->ctx.a);
    free(wc->ctx.v);
    free(wc->ctx.out);
}

/* ------------------------------------------------------------- self-tests */

static int failures = 0;

static void expect(int ok, const char *what)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

static int stats_equal(bench_stats s, double mn, double md, double mx)
{
    return s.min == mn && s.median == md && s.max == mx;
}

static void test_stats(void)
{
    bench_stats s;
    double v5[5] = {5.0, 1.0, 3.0, 2.0, 4.0};
    double v4[4] = {10.0, 2.0, 8.0, 4.0};
    double v1[1] = {7.0};
    double v6[6] = {3.0, 3.0, 3.0, 3.0, 3.0, 3.0};

    bench_stats_compute(v5, 5, &s);
    expect(stats_equal(s, 1.0, 3.0, 5.0), "stats n=5 -> min 1 median 3 max 5");
    bench_stats_compute(v4, 4, &s);
    expect(stats_equal(s, 2.0, 8.0, 10.0),
           "stats n=4 -> min 2 median 8 (upper) max 10");
    bench_stats_compute(v1, 1, &s);
    expect(stats_equal(s, 7.0, 7.0, 7.0), "stats n=1 -> 7 7 7");
    bench_stats_compute(v6, 6, &s);
    expect(stats_equal(s, 3.0, 3.0, 3.0), "stats n=6 all equal");
}

/* Cross-checks the chain kernels against a second FLINT function for the
   same operation.  This shows that every step applies the operation. */
static void test_chain_semantics(ulong p)
{
    const unsigned long long k = 1000;
    word_ctx c;
    ulong b, x0, ref, i;

    ctx_init(&c, p);
    b = p - 2;
    x0 = p - 3;
    c.b = b;
    c.x0 = x0;

    chain_add_run(&c, k);
    ref = x0;
    for (i = 0; i < k; i++)
        ref = n_addmod(ref, b, p);
    expect(c.acc == ref, "chain_add 1000 steps matches an n_addmod loop");

    chain_mul_run(&c, k);
    ref = x0;
    for (i = 0; i < k; i++)
        ref = n_mulmod2_preinv(ref, b, p, c.ninv);
    expect(c.acc == ref, "chain_mul 1000 steps matches an n_mulmod2_preinv loop");

    chain_mul2_run(&c, k);
    ref = x0;
    for (i = 0; i < k; i++)
        ref = nmod_mul(ref, b, c.mod);
    expect(c.acc == ref, "chain_mul2 1000 steps matches an nmod_mul loop");
}

/* Reads objdump output for one chain kernel and checks that the innermost
   loop has a back edge and a register that is written by a read-modify-write
   instruction and is not the loop-control register.  That register is a
   loop-carried value, which is the chain dependency.  Prints the found
   instruction so the check can be audited against bench/asm. */
#define MAX_INSNS 600

static void asm_mn_ops(const char *insn, char *mn, char *ops)
{
    const char *p;
    mn[0] = '\0';
    ops[0] = '\0';
    if (sscanf(insn, "%31s", mn) != 1)
        return;
    p = insn + strlen(mn);
    while (*p == ' ' || *p == '\t')
        p++;
    snprintf(ops, BENCH_STR, "%s", p);
}

static void asm_first_op(const char *ops, char *out, size_t outsz)
{
    size_t len;
    (void) outsz;
    out[0] = '\0';
    if (sscanf(ops, "%31[^,]", out) != 1)
        return;
    len = strlen(out);
    while (len > 0 && (out[len - 1] == ' ' || out[len - 1] == '\t'))
        out[--len] = '\0';
}

/* True when reg occurs in s as a whole token, so that rax matches [rax+8]
   and rax, but not raxish. */
static int asm_reg_in(const char *s, const char *reg)
{
    size_t rl = strlen(reg), l = strlen(s), i;
    for (i = 0; i + rl <= l; i++) {
        char before, after;
        int bb, aa;
        if (strncmp(s + i, reg, rl) != 0)
            continue;
        before = i > 0 ? s[i - 1] : ' ';
        after = i + rl < l ? s[i + rl] : '\0';
        bb = !((before >= 'a' && before <= 'z') ||
               (before >= '0' && before <= '9') || before == '_');
        aa = !((after >= 'a' && after <= 'z') ||
               (after >= '0' && after <= '9') || after == '_');
        if (bb && aa)
            return 1;
    }
    return 0;
}

static int asm_is_rmw(const char *mn)
{
    return strcmp(mn, "add") == 0 || strcmp(mn, "adc") == 0 ||
           strcmp(mn, "sub") == 0 || strcmp(mn, "sbb") == 0 ||
           strcmp(mn, "and") == 0 || strcmp(mn, "or") == 0 ||
           strcmp(mn, "xor") == 0 || strcmp(mn, "shl") == 0 ||
           strcmp(mn, "shr") == 0 || strcmp(mn, "sar") == 0 ||
           strcmp(mn, "imul") == 0;
}

static int asm_is_movlike(const char *mn)
{
    return strcmp(mn, "mov") == 0 || strcmp(mn, "movzx") == 0 ||
           strcmp(mn, "movsx") == 0 || strcmp(mn, "movsxd") == 0 ||
           strcmp(mn, "lea") == 0 || strcmp(mn, "shlx") == 0 ||
           strcmp(mn, "shrx") == 0 || strcmp(mn, "sarx") == 0 ||
           strncmp(mn, "cmov", 4) == 0;
}

/* Reads the old value of dest, which is what makes a register loop-carried. */
static int asm_reads_old(const char *insn, const char *dest)
{
    char mn[32], ops[BENCH_STR], first[32];
    asm_mn_ops(insn, mn, ops);
    if (strcmp(mn, "mul") == 0 || strcmp(mn, "div") == 0 ||
        strcmp(mn, "idiv") == 0) {
        if (strcmp(dest, "rax") == 0)
            return 1;
        if ((strcmp(mn, "div") == 0 || strcmp(mn, "idiv") == 0) &&
            strcmp(dest, "rdx") == 0)
            return 1;
        return 0;
    }
    if (strcmp(mn, "imul") == 0 && strchr(ops, ',') == NULL)
        return strcmp(dest, "rax") == 0;
    if (asm_is_rmw(mn) && strchr(ops, ',') != NULL) {
        asm_first_op(ops, first, sizeof(first));
        return strcmp(first, dest) == 0 || asm_reg_in(ops, dest);
    }
    if (asm_is_movlike(mn)) {
        char *comma = strchr(ops, ',');
        return comma != NULL && asm_reg_in(comma + 1, dest);
    }
    return 0;
}

static int asm_writes(const char *insn, const char *dest)
{
    char mn[32], ops[BENCH_STR], first[32];
    asm_mn_ops(insn, mn, ops);
    if (strcmp(mn, "mul") == 0 || strcmp(mn, "div") == 0 ||
        strcmp(mn, "idiv") == 0)
        return strcmp(dest, "rax") == 0 || strcmp(dest, "rdx") == 0;
    if (strcmp(mn, "imul") == 0 && strchr(ops, ',') == NULL)
        return strcmp(dest, "rax") == 0;
    asm_first_op(ops, first, sizeof(first));
    if (asm_is_rmw(mn) || asm_is_movlike(mn))
        return strcmp(first, dest) == 0;
    return 0;
}

static int asm_selfdep(const char *binary, const char *sym)
{
    char cmd[700];
    char line[600];
    static unsigned long addr[MAX_INSNS];
    static char txt[MAX_INSNS][BENCH_STR];
    static char ctrl[8][16];
    int nctrl = 0;
    int n = 0, in = 0, i, j, best_i = -1, best_j = -1, pass = 0;
    char want[160];
    FILE *p;

    snprintf(cmd, sizeof(cmd),
             "objdump -d -M intel --no-show-raw-insn --disassemble=%s %s "
             "2>/dev/null", sym, binary);
    p = popen(cmd, "r");
    if (p == NULL) {
        printf("FAIL asm %s: cannot run objdump\n", sym);
        return 1;
    }
    snprintf(want, sizeof(want), "<%s>:", sym);
    while (fgets(line, sizeof(line), p) != NULL) {
        if (strstr(line, want) != NULL) {
            in = 1;
            continue;
        }
        if (!in)
            continue;
        if (line[0] != ' ' && line[0] != '\t' && strstr(line, ">:") != NULL)
            break;
        {
            char a[32], insn[BENCH_STR];
            if (sscanf(line, " %31[0-9a-f]:%*[ \t]%191[^\n]", a, insn) == 2) {
                if (n < MAX_INSNS) {
                    addr[n] = strtoul(a, NULL, 16);
                    snprintf(txt[n], BENCH_STR, "%s", insn);
                    n++;
                }
            }
        }
    }
    pclose(p);
    if (n == 0) {
        printf("FAIL asm %s: no instructions found\n", sym);
        return 1;
    }
    for (j = 1; j < n; j++) {
        char mn[32], t[64];
        unsigned long tgt;
        int k;
        if (sscanf(txt[j], "%31s", mn) != 1 || mn[0] != 'j')
            continue;
        if (sscanf(txt[j], "%*s %63s", t) != 1)
            continue;
        {
            char *lt = strchr(t, '<');
            if (lt != NULL)
                *lt = '\0';
        }
        tgt = strtoul(t, NULL, 16);
        if (tgt > addr[j])
            continue;
        for (k = 0; k < j; k++) {
            if (addr[k] == tgt) {
                best_i = k;
                best_j = j;
                break;
            }
        }
    }
    if (best_i < 0) {
        printf("FAIL asm %s: no loop back edge found\n", sym);
        return 1;
    }
    /* The register(s) tested by the instruction just before the back edge
       are loop control, not the chain value. */
    {
        char mn[32], ops[BENCH_STR];
        if (sscanf(txt[best_j - 1], "%31s %191[^\n]", mn, ops) == 2 &&
            (strcmp(mn, "cmp") == 0 || strcmp(mn, "test") == 0)) {
            char *tok = strtok(ops, ",");
            while (tok != NULL && nctrl < 8) {
                while (*tok == ' ')
                    tok++;
                snprintf(ctrl[nctrl], sizeof(ctrl[nctrl]), "%s", tok);
                nctrl++;
                tok = strtok(NULL, ",");
            }
        }
    }
    for (i = best_i; i < best_j && !pass; i++) {
        char mn[32], ops[BENCH_STR], dest[32];
        int is_implicit = 0, w, carried = 0;
        if (sscanf(txt[i], "%31s %191[^\n]", mn, ops) != 2)
            continue;
        if (strcmp(mn, "mul") == 0 || strcmp(mn, "div") == 0 ||
            strcmp(mn, "idiv") == 0 ||
            (strcmp(mn, "imul") == 0 && strchr(ops, ',') == NULL)) {
            is_implicit = 1;
            snprintf(dest, sizeof(dest), "rax");
        } else if (asm_is_rmw(mn) && strchr(ops, ',') != NULL) {
            asm_first_op(ops, dest, sizeof(dest));
        } else {
            continue;
        }
        if (strcmp(dest, "rsp") == 0 || strcmp(dest, "esp") == 0 ||
            strcmp(dest, "rbp") == 0 || strcmp(dest, "ebp") == 0)
            continue;
        {
            int bad = 0, c;
            for (c = 0; c < nctrl; c++)
                if (strcmp(ctrl[c], dest) == 0)
                    bad = 1;
            if (bad)
                continue;
        }
        /* The register is loop-carried when its old value is read before it
           is first written in the loop.  A temporary that is overwritten at
           the top of the loop fails this test. */
        for (w = best_i; w < best_j; w++)
            if (asm_writes(txt[w], dest))
                break;
        for (j = best_i; j <= w && j < best_j; j++) {
            if (asm_reads_old(txt[j], dest)) {
                carried = 1;
                break;
            }
        }
        if (carried) {
            printf("PASS asm %s: %s (loop-carried %s%s) in loop [%lx,%lx)\n",
                   sym, txt[i], dest,
                   is_implicit ? ", implicit" : "",
                   addr[best_i], addr[best_j]);
            pass = 1;
        }
    }
    if (!pass) {
        printf("FAIL asm %s: no loop-carried read-modify-write found\n", sym);
        return 1;
    }
    return 0;
}

static int count_cores(void)
{
    FILE *f;
    int cores = 0;
    char line[256];
    char *end;
    long lo, hi;

    f = fopen("/sys/devices/system/cpu/online", "r");
    if (f == NULL)
        return 0;
    if (fgets(line, sizeof(line), f) != NULL) {
        end = strchr(line, '\n');
        if (end != NULL)
            *end = '\0';
        if (sscanf(line, "%ld-%ld", &lo, &hi) == 2)
            cores = (int) (hi - lo + 1);
        else
            cores = 1;
    }
    fclose(f);
    return cores;
}

static int selftest(const char *binary)
{
    ulong p = BENCH_MODULUS;
    printf("# bench_word self-test\n");
    test_stats();
    test_chain_semantics(p);
    failures += asm_selfdep(binary, "chain_add_run");
    failures += asm_selfdep(binary, "chain_mul_run");
    failures += asm_selfdep(binary, "chain_mul2_run");
    printf("# self-test %s (%d failure%s)\n",
           failures == 0 ? "PASS" : "FAIL", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}

/* ----------------------------------------------------------------- driver */

static void usage(const char *argv0)
{
    printf("usage: %s [--run] [--selftest] [--seed N] [--trials N] [--cpu N]\n",
           argv0);
    printf("  --run       measure and write a result file (default if no flag)\n");
    printf("  --selftest  run the harness self-tests\n");
}

static int run(int cpu, unsigned long long seed, int trials)
{
    char run_id[64];
    char model[BENCH_STR];
    char tag[64];
    bench_tsc_calib cal;
    word_case cases[6];
    flint_rand_t state;
    unsigned long long checksum;
    ulong p = BENCH_MODULUS;
    ulong b, x0;
    int actual_cpu, nc = 0, i;
    FILE *f;

    flint_randinit(state);
    flint_randseed(state, (ulong) seed, (ulong) (seed >> 32));
    g_seed = seed;

    actual_cpu = bench_pin_cpu(cpu);
    if (actual_cpu < 0) {
        fprintf(stderr, "warning: sched_setaffinity(%d) failed; running unpinned\n",
                cpu);
        actual_cpu = bench_get_cpu();
    }
    printf("pinned to cpu %d (requested %d)\n", actual_cpu, cpu);
    bench_tsc_calibrate(&cal, 9, 0.02);

    b = rand_full(state, p) | 1;
    x0 = rand_full(state, p);

    fill_chain(&cases[nc++], "nmod_add_chain",
               "nmod_add; second operand fixed, first operand is previous result",
               chain_add_run, CHAIN_ADD_REPS, p, b, x0);
    fill_chain(&cases[nc++], "nmod_mul_chain",
               "nmod_mul; second operand fixed, first operand is previous result",
               chain_mul_run, CHAIN_MUL_REPS, p, b, x0);
    fill_chain(&cases[nc++], "n_mulmod2_preinv_chain",
               "n_mulmod2_preinv; second operand fixed, first operand is previous result",
               chain_mul2_run, CHAIN_MUL2_REPS, p, b, x0);
    fill_batch(&cases[nc++], "nmod_add_batch",
               "nmod_add; independent pairs a[i],b[i]; arrays L1 resident",
               batch_add_run, BATCH_ADD_PASSES, p, state);
    fill_batch(&cases[nc++], "nmod_mul_batch",
               "nmod_mul; independent pairs a[i],b[i]; arrays L1 resident",
               batch_mul_run, BATCH_MUL_PASSES, p, state);
    fill_batch(&cases[nc++], "n_mulmod2_preinv_batch",
               "n_mulmod2_preinv; independent pairs a[i],b[i]; arrays L1 resident",
               batch_mul2_run, BATCH_MUL2_PASSES, p, state);

    snprintf(tag, sizeof(tag), "word");
    f = bench_results_open(tag, run_id, sizeof(run_id));
    if (f == NULL) {
        fprintf(stderr, "cannot open result file\n");
        return 1;
    }
    bench_cpu_model(model, sizeof(model));
    write_header(f, run_id, model, cpu, actual_cpu, &cal, trials,
                 count_cores());
    for (i = 0; i < nc; i++)
        report_case(f, &cases[i], trials);
    checksum = g_checksum;
    fprintf(f, "\n# checksum of all case results, computed outside the timed "
               "region\n");
    fprintf(f, "checksum: %llu\n", checksum);
    fclose(f);
    for (i = 0; i < nc; i++)
        free_case(&cases[i]);
    flint_randclear(state);
    printf("checksum %llu\n", checksum);
    return 0;
}

int main(int argc, char **argv)
{
    int i, do_run = 0, do_selftest = 0, cpu = 2, trials = 15;
    unsigned long long seed = 20260928ULL;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--run") == 0)
            do_run = 1;
        else if (strcmp(argv[i], "--selftest") == 0)
            do_selftest = 1;
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
            seed = strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--trials") == 0 && i + 1 < argc)
            trials = atoi(argv[++i]);
        else if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc)
            cpu = atoi(argv[++i]);
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }
    if (do_selftest)
        return selftest(argv[0]);
    if (!do_run)
        do_run = 1;
    return run(cpu, seed, trials);
}
