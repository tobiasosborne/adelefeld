/* lanes/m1-repair-dump/checks/bigctx.c: a dump with k blocks of the first k primes, built in C;
   the time of the inspector and of adf_modctx_new_from_dump on it, and their statuses.
   Usage: bigctx <k> [k...]  (at most 2 in one run, so that one run stays under three minutes). */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/ulong_extras.h>

#include <adelefeld.h>

/* "adf1 Q modctx <K hex> <k hex> <q_1 hex> ... <q_k hex>" with q_i the i-th prime. */
static char *
modctx_text(slong k)
{
    slong i;
    ulong p = 1;
    fmpz_t K;
    char * h, * t;
    size_t cap, n = 0;

    fmpz_init_set_ui(K, 1);
    for (i = 0; i < k; i++)
    {
        p = n_nextprime(p, 1);
        fmpz_mul_ui(K, K, p);
    }
    cap = 64 + 8 * ((size_t) k + 4) + (size_t) fmpz_sizeinbase(K, 16) + 2;
    h = fmpz_get_str(NULL, 16, K);
    t = (char *) flint_malloc(cap);
    n += (size_t) sprintf(t, "adf1 Q modctx %s %lx", h, (unsigned long) k);
    p = 1;
    for (i = 0; i < k; i++)
    {
        p = n_nextprime(p, 1);
        n += (size_t) sprintf(t + n, " %lx", (unsigned long) p);
    }
    flint_free(h);
    fmpz_clear(K);
    t[n] = '\0';
    return t;
}

int
main(int argc, char ** argv)
{
    int a;

    for (a = 1; a < argc; a++)
    {
        slong i;
        slong k = atol(argv[a]);
        char * t = modctx_text(k);
        size_t n = strlen(t);
        char * fb = (char *) flint_malloc(n + 32 + 2 * (size_t) k);
        size_t nfb;
        double t0, t1;
        adf_modctx_struct * c = NULL;
        size_t nctx = 0;
        int st;

        /* the same context inside a local fball: a body every typed reader accepts */
        nfb = (size_t) sprintf(fb, "adf1 Q fball l 1 %s", t + 14);
        for (i = 0; i < k; i++)
            nfb += (size_t) sprintf(fb + nfb, " 0");
        printf("k=%ld bytes=%zu\n", (long) k, n);
        fflush(stdout);
        t0 = (double) clock() / CLOCKS_PER_SEC;
        st = adf_fball_dump_inspect(&nctx, NULL, fb, nfb, NULL);
        t1 = (double) clock() / CLOCKS_PER_SEC;
        printf("  fball_inspect status=%s seconds=%.6f\n", adf_status_str(st), t1 - t0);
        fflush(stdout);
        t0 = (double) clock() / CLOCKS_PER_SEC;
        st = adf_modctx_new_from_dump(&c, t, n, 0, NULL);
        t1 = (double) clock() / CLOCKS_PER_SEC;
        printf("  new_from_dump status=%s blocks=%ld seconds=%.6f\n", adf_status_str(st),
               c ? (long) adf_modctx_nblocks(c) : -1L, t1 - t0);
        adf_modctx_free(c);
        flint_free(fb);
        flint_free(t);
        fflush(stdout);
    }
    flint_cleanup_master();
    return 0;
}
