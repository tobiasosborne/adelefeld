#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include "adelefeld/dump.h"

/* Independent checks use public entry points only. Return status; trouble holds contract failures. */
int review_probe(const char *s, size_t len, int kind, size_t occurrence,
                 const adf_text_limits_t *lim, int *trouble)
{
    adf_modctx_struct *ctx = NULL, *base = NULL;
    adf_ctx_desc_t desc;
    size_t n = 1, dl = 0;
    int st, ist = -1;
    char *again = NULL;
    *trouble = 0;
    st = adf_modctx_new_from_dump(&ctx, s, len, occurrence, lim);
    if (st != ADF_OK && ctx != NULL) *trouble |= 1;
    adf_ctx_desc_init(&desc);
    switch (kind) {
    case 0: ist = adf_rat_dump_inspect(&n, &desc, s, len, lim); break;
    case 1: ist = adf_fball_dump_inspect(&n, &desc, s, len, lim); break;
    case 2: ist = adf_scaled_dump_inspect(&n, &desc, s, len, lim); break;
    case 3: ist = adf_adele_dump_inspect(&n, &desc, s, len, lim); break;
    case 4: ist = adf_cadele_dump_inspect(&n, &desc, s, len, lim); break;
    }
    if (kind >= 0 && kind <= 4) {
        if (ist != ADF_OK && (n != 1 || !fmpz_is_one(desc.K) || desc.q || desc.k)) *trouble |= 2;
        if (ist == ADF_OK && n && (st != ADF_OK || !adf_modctx_matches_desc(ctx, &desc)))
            *trouble |= 4;
        if (kind == 2) {
            fmpz_t one;
            fmpz_init_set_ui(one, 1);
            adf_modctx_new_fmpz(&base, one);
            fmpz_clear(one);
        }
#define RUN(TYPE, INIT) do { \
        adf_##TYPE##_t x; unsigned char old[sizeof(x)]; int lst; \
        memset(x, 0, sizeof(x)); INIT; memcpy(old, x, sizeof(x)); \
        lst = adf_##TYPE##_load_str(x, s, len, ctx, lim); \
        if (lst != ist) *trouble |= 8; \
        if (lst != ADF_OK && memcmp(old, x, sizeof(x))) *trouble |= 16; \
        if (lst == ADF_OK) { \
            if (!adf_##TYPE##_is_canonical(x)) *trouble |= 32; \
            again = adf_##TYPE##_dump_str(&dl, x); \
            if (dl != len || memcmp(again, s, len) || again[dl]) *trouble |= 64; \
        } \
        adf_##TYPE##_clear(x); \
    } while (0)
        switch (kind) {
        case 0: RUN(rat, adf_rat_init(x)); break;
        case 1: RUN(fball, adf_fball_init(x)); break;
        case 2: RUN(scaled, adf_scaled_init(x, base)); break;
        case 3: RUN(adele, adf_adele_init(x)); break;
        case 4: RUN(cadele, adf_cadele_init(x)); break;
        }
#undef RUN
    }
    adf_str_free(again);
    adf_ctx_desc_clear(&desc);
    adf_modctx_free(ctx);
    adf_modctx_free(base);
    return st;
}

int review_inspect(const char *s, size_t len, const adf_text_limits_t *lim)
{
    size_t n = 0;
    return adf_scaled_dump_inspect(&n, NULL, s, len, lim);
}

int review_typed_status(const char *s, size_t len, int kind, const adf_text_limits_t *lim)
{
    size_t n = 17;
    switch (kind) {
    case 0: return adf_rat_dump_inspect(&n, NULL, s, len, lim);
    case 1: return adf_fball_dump_inspect(&n, NULL, s, len, lim);
    case 2: return adf_scaled_dump_inspect(&n, NULL, s, len, lim);
    case 3: return adf_adele_dump_inspect(&n, NULL, s, len, lim);
    default: return adf_cadele_dump_inspect(&n, NULL, s, len, lim);
    }
}

static long live = 0, calls = 0;
static void *count_malloc(size_t n) { void *p = malloc(n); calls++; if (p) live++; return p; }
static void *count_calloc(size_t n, size_t m) { void *p = calloc(n, m); calls++; if (p) live++; return p; }
static void *count_realloc(void *p, size_t n)
{
    int was_null = p == NULL;
    void *q = realloc(p, n);
    calls++;
    if (was_null && q) live++;
    if (!was_null && n == 0 && !q) live--;
    return q;
}
static void count_free(void *p) { if (p) live--; free(p); }

int main(int argc, char **argv)
{
    if (argc > 1) {
        FILE *f = fopen(argv[1], "rb");
        char *s; long len; int st; clock_t t; adf_modctx_struct *ctx = NULL;
        if (!f) return 2;
        fseek(f, 0, SEEK_END); len = ftell(f); rewind(f);
        s = malloc((size_t)len);
        if (fread(s, 1, (size_t)len, f) != (size_t)len) return 3;
        fclose(f); t = clock();
        st = adf_modctx_new_from_dump(&ctx, s, (size_t)len, 0, NULL);
        printf("bytes=%ld status=%s blocks=%ld seconds=%.6f\n", len, adf_status_str(st),
               ctx ? adf_modctx_nblocks(ctx) : -1L, (double)(clock()-t)/CLOCKS_PER_SEC);
        adf_modctx_free(ctx); free(s); flint_cleanup_master(); return 0;
    }
    __flint_set_memory_functions(count_malloc, count_calloc, count_realloc, count_free);
    const char *texts[] = {
        "adf1 Q rat 100000000000000000000000000000001 3",
        "adf1 Q rat 100000000000000000000000000000001 0",
        "adf1 Q fball l 2 6 2 2 3 0 0", "adf1 Q fball l 2 6 2 2 3 2 0",
        "adf1 Q scaled x -100000000000000000000000000000003 2 6 2 2 3",
        "adf1 Q scaled s 1 2 6 6 2 2 3", "adf1 Q scaled s 1 2 3 6 2 2 3",
        "adf1 Q adele 1 3 -fffffffffffffffff 3 fffffffffffffffff g 0 0 1",
        "adf1 Q adele 1 2 0 1 0 g 0 0 1",
        "adf1 Q cadele 1 1 0 1 -5 -1 1 3 -3 l 3 6 2 2 3 1 2",
        "adf1 Q cadele 1 1 0 40000001 0 0 0 0 0 g 0 0 1",
        "adf1 Q qclass pieces 1 1 1 -100001 0 0 l 1 2 1 2 0",
        "adf1 Q modctx 6 2 2 3", "adf1 Q modctx 6 2 2 2"
    };
    const int kinds[] = {0,0,1,1,2,2,2,3,3,4,4,-1,-1,-1};
    int errors = 0; size_t cases = 0;
    long page = sysconf(_SC_PAGESIZE);
    char *mem = mmap(NULL, 2*(size_t)page, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED || mprotect(mem+page, (size_t)page, PROT_NONE)) return 4;
    for (size_t i = 0; i < sizeof(texts)/sizeof(texts[0]); i++) {
        size_t len = strlen(texts[i]);
        for (size_t k = 0; k <= len; k++) {
            int trouble;
            char *p = mem+page-k;
            memcpy(p, texts[i], k);
            review_probe(p, k, kinds[i], 0, NULL, &trouble);
            errors += trouble != 0; cases++;
        }
    }
    munmap(mem, 2*(size_t)page);
    flint_cleanup_master();
    printf("guarded_cases=%zu contract_errors=%d allocation_calls=%ld retained_allocations=%ld\n",
           cases, errors, calls, live);
    return errors || live;
}
