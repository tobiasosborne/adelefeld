/* dump_run.c: runs adf_modctx_new_from_dump on byte strings read from stdin, one per line,
   as "OCC MAXITEMS HEX" (HEX = the input bytes in hexadecimal, "-" for the empty string).
   Each input is placed twice: in a heap block of exactly len bytes (AddressSanitizer sees a
   read of s[len]) and at the end of a mapped page followed by a PROT_NONE page (a read of
   s[len] faults even without a sanitizer). Both calls must agree. On a failure *out must be
   the sentinel still. Prints "STATUS" or "OK K q1,q2,..." per line, and the dump_str of the
   context, which must equal the canonical body.
   Build: cc -std=c11 -O0 -g [-fsanitize=address,undefined] -Iinclude dump_run.c src/*.c -lflint -lgmp -lm */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <adelefeld.h>

static adf_modctx_struct * const SENT = (adf_modctx_struct *) 0x1234;

static int hexval(int c) { return c <= '9' ? c - '0' : c - 'a' + 10; }

int main(void)
{
    static char line[1 << 22];
    long pg = sysconf(_SC_PAGESIZE);
    size_t maplen = (size_t) pg * 600;   /* up to ~2.4 MB of input */
    char * base = mmap(NULL, maplen + (size_t) pg, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    long bad = 0, n = 0;
    if (base == MAP_FAILED || mprotect(base + maplen, (size_t) pg, PROT_NONE) != 0)
        return 2;
    while (fgets(line, sizeof line, stdin))
    {
        size_t occ, len, i;
        long maxit;
        static char hex[1 << 22];
        char * heap, * pgp;
        adf_text_limits_t lim;
        adf_modctx_struct * c1 = SENT, * c2 = SENT;
        int s1, s2;
        if (sscanf(line, "%zu %ld %s", &occ, &maxit, hex) != 3)
            continue;
        len = strcmp(hex, "-") == 0 ? 0 : strlen(hex) / 2;
        heap = malloc(len ? len : 1);
        for (i = 0; i < len; i++)
            heap[i] = (char) (hexval(hex[2 * i]) * 16 + hexval(hex[2 * i + 1]));
        pgp = base + maplen - len;
        memcpy(pgp, heap, len);
        adf_text_limits_default(&lim);
        if (maxit >= -1)
            lim.max_items = maxit;
        s1 = adf_modctx_new_from_dump(&c1, len ? heap : heap + 0, len, occ, &lim);
        s2 = adf_modctx_new_from_dump(&c2, pgp, len, occ, &lim);
        n++;
        if (s1 != s2) { bad++; printf("DISAGREE %d %d\n", s1, s2); }
        if (s1 != ADF_OK)
        {
            if (c1 != SENT || c2 != SENT) { bad++; printf("OUT-TOUCHED "); }
            printf("%s\n", adf_status_str(s1));
        }
        else
        {
            fmpz_t K;
            slong k, j;
            char * ks, * ds;
            size_t dl;
            fmpz_init(K);
            adf_modctx_get_modulus(K, c1);
            ks = fmpz_get_str(NULL, 10, K);
            k = adf_modctx_nblocks(c1);
            printf("OK %s ", ks);
            for (j = 0; j < k; j++)
                printf("%s%lu", j ? "," : "", adf_modctx_block(c1, j));
            ds = adf_modctx_dump_str(&dl, c1);
            printf(" DUMP=%s\n", ds);
            flint_free(ds);
            flint_free(ks);
            fmpz_clear(K);
            adf_modctx_free(c1);
            adf_modctx_free(c2);
        }
        free(heap);
    }
    fprintf(stderr, "inputs %ld, C-side failures %ld\n", n, bad);
    munmap(base, maplen + (size_t) pg);
    return bad != 0;
}
