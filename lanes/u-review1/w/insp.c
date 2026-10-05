#include <stdio.h>
#include <string.h>
#include "adelefeld/dump.h"
typedef int (*F)(size_t *, adf_ctx_desc_t *, const char *, size_t, const adf_text_limits_t *);
int main(void)
{
    struct { const char *s; F f; } t[] = {
        {"adf1 Q ucoset 5 6", adf_ucoset_dump_inspect}, {"adf1 Q ucoset 5 7", adf_ucoset_dump_inspect},
        {"adf1 Q idele 1 3 0 1 0 5 7 5 c", adf_idele_dump_inspect}, {"adf1 Q idele 1 3 0 1 0 5 7 5", adf_idele_dump_inspect},
        {"adf1 Q idclass 5 0 3 0 5 c", adf_idclass_dump_inspect}, {"adf1 Q idclass 5 0 3 0 5 c x", adf_idclass_dump_inspect},
        {"adf1 Q lball 5 b 3 0 4", adf_lball_dump_inspect}, {"adf1 Q lball 6 b 3 0 4", adf_lball_dump_inspect},
        {"adf1 Q sball n 0", adf_sball_dump_inspect}, {"adf1 Q sball n 1 2 b 1 0 2 3 b 1 0 2", adf_sball_dump_inspect},
    };
    int i, bad = 0;
    for (i = 0; i < 10; i++) {
        adf_ctx_desc_t d[3]; size_t n; int st, k, cap;
        for (cap = 0; cap <= 3; cap += 3) {
            for (k = 0; k < 3; k++) adf_ctx_desc_init(&d[k]);
            fmpz_set_ui(d[1].K, 77); d[1].k = 5;
            n = (size_t) cap;
            st = t[i].f(&n, cap ? d : d, t[i].s, strlen(t[i].s), NULL);
            printf("%-45s cap %d -> st %d nctx %zu d1.K %ld d1.k %ld\n", t[i].s, cap, st, n, fmpz_get_si(d[1].K), (long) d[1].k);
            if ((st == 0 && n != 0) || (st != 0 && n != (size_t) cap) || fmpz_get_si(d[1].K) != 77 || d[1].k != 5) bad++;
            for (k = 0; k < 3; k++) adf_ctx_desc_clear(&d[k]);
        }
    }
    printf("bad %d\n", bad);
    return 0;
}
