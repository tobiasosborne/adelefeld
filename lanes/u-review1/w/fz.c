/* differential harness: lines "typ mode l0 l1 l2 l3 hex" -> "status canon dumpeq insp nctx unchg" */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "adelefeld/dump.h"

static unsigned char *unhex(const char *h, size_t *n)
{
    size_t L = strlen(h), i;
    unsigned char *b = malloc(L / 2 + 1);
    if (h[0] == '-' && h[1] == 0) { *n = 0; return b; }
    for (i = 0; i < L / 2; i++) { unsigned v; sscanf(h + 2 * i, "%2x", &v); b[i] = (unsigned char) v; }
    *n = L / 2;
    return b;
}

#define GEN(T, SENT)                                                                                   \
static void run_##T(char mode, const adf_text_limits_t *lim, const char *s, size_t len)               \
{                                                                                                      \
    adf_##T##_t x, cp; size_t l0, l1, nctx = 77; char *d0, *d1; int st, ist, canon = -1, deq = -1, un; \
    const adf_modctx_struct *dummy[2] = {NULL, NULL};                                                  \
    adf_##T##_init(x); adf_##T##_init(cp);                                                             \
    st = adf_##T##_load_str(x, SENT, strlen(SENT), NULL, NULL);                                        \
    if (st) { printf("SENTINEL FAIL %d\n", st); exit(2); }                                             \
    adf_##T##_set(cp, x);                                                                              \
    d0 = adf_##T##_dump_str(&l0, x);                                                                   \
    if (mode == 'l') st = adf_##T##_load_str(x, s, len, NULL, lim);                                    \
    else if (mode == 'a') st = adf_##T##_load_str_binds(x, s, len, NULL, 0, lim);                      \
    else if (mode == 'b') st = adf_##T##_load_str_binds(x, s, len, dummy, 1, lim);                     \
    else st = adf_##T##_load_str_binds(x, s, len, NULL, 1, lim);                                       \
    if (st == 0) {                                                                                     \
        canon = adf_##T##_is_canonical(x);                                                             \
        d1 = adf_##T##_dump_str(&l1, x);                                                               \
        deq = (l1 == len && s != NULL && memcmp(d1, s, len) == 0);                                     \
        adf_str_free(d1);                                                                              \
        un = -1;                                                                                       \
    } else {                                                                                           \
        d1 = adf_##T##_dump_str(&l1, x);                                                               \
        un = (l1 == l0 && memcmp(d0, d1, l0) == 0 && adf_##T##_identical(x, cp));                      \
        adf_str_free(d1);                                                                              \
    }                                                                                                  \
    ist = adf_##T##_dump_inspect(&nctx, NULL, s, len, lim);                                            \
    printf("%d %d %d %d %zu %d\n", st, canon, deq, ist, nctx, un);                                     \
    adf_str_free(d0); adf_##T##_clear(x); adf_##T##_clear(cp);                                         \
}

GEN(ucoset, "adf1 Q ucoset 5 6")
GEN(idele, "adf1 Q idele 1 3 0 1 0 5 7 5 c")
GEN(idclass, "adf1 Q idclass 5 0 3 0 5 c")
GEN(lball, "adf1 Q lball 5 b 3 0 4")
GEN(sball, "adf1 Q sball r 3 0 1 0 2 3 b 1 0 2 5 x 1 3 1")

int main(void)
{
    char *line = NULL; size_t cap = 0;
    while (getline(&line, &cap, stdin) > 0) {
        char typ[16], mode, a[32], b[32], c[32], d[32]; char *hex = malloc(strlen(line) + 2);
        adf_text_limits_t lim; size_t n; unsigned char *buf; int uselim;
        if (sscanf(line, "%15s %c %31s %31s %31s %31s %s", typ, &mode, a, b, c, d, hex) != 7) { puts("BADLINE"); continue; }
        uselim = strcmp(a, "-") != 0;
        if (uselim) { lim.max_len = strtoull(a, 0, 10); lim.max_exp10 = strtoll(b, 0, 10); lim.max_prec = strtoll(c, 0, 10); lim.max_items = strtoll(d, 0, 10); }
        if (hex[0] == 'N' && hex[1] == 0) { buf = NULL; n = 0; } else buf = unhex(hex, &n);
        /* trailing guard: copy to an exact-size heap block so ASan sees overreads */
        if (buf) { unsigned char *e = malloc(n ? n : 1); memcpy(e, buf, n); free(buf); buf = e; }
        const char *s = (const char *) buf;
        fflush(stdout);
        pid_t pid = getenv("NOFORK") ? 0 : fork();
        if (pid > 0) { int ws; waitpid(pid, &ws, 0); if (WIFSIGNALED(ws)) printf("CRASH %d\n", WTERMSIG(ws)); else if (WEXITSTATUS(ws)) printf("CRASH exit %d\n", WEXITSTATUS(ws)); free(buf); free(hex); continue; }
        if (!strcmp(typ, "ucoset")) run_ucoset(mode, uselim ? &lim : NULL, s, n);
        else if (!strcmp(typ, "idele")) run_idele(mode, uselim ? &lim : NULL, s, n);
        else if (!strcmp(typ, "idclass")) run_idclass(mode, uselim ? &lim : NULL, s, n);
        else if (!strcmp(typ, "lball")) run_lball(mode, uselim ? &lim : NULL, s, n);
        else if (!strcmp(typ, "sball")) run_sball(mode, uselim ? &lim : NULL, s, n);
        else puts("BADTYPE");
        fflush(stdout); if (!getenv("NOFORK")) _exit(0); free(buf); free(hex);
    }
    free(line); return 0;
}
