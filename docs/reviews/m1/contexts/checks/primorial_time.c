/* primorial_time.c: adf_modctx_new_primorial_pow(&c, n, e) on one hostile argument; prints the
   status, the wall time and the peak resident set. For e >= 2 and n above 2^(64/e) the answer
   is ADF_UNSUPPORTED (the block p^e of the largest prime p <= n does not fit a word), which
   an O(1) test on n and e could decide. Run under `timeout`.
   usage: primorial_time N E
   Build: cc -std=c11 -O2 -Iinclude primorial_time.c build/libadelefeld.a -lflint -lgmp -lm */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <adelefeld.h>

int main(int argc, char ** argv)
{
    ulong n = strtoul(argv[1], NULL, 0), e = strtoul(argv[2], NULL, 0);
    adf_modctx_struct * c = NULL;
    struct timespec t0, t1;
    struct rusage ru;
    int st;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    st = adf_modctx_new_primorial_pow(&c, n, e);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    getrusage(RUSAGE_SELF, &ru);
    printf("n=%lu e=%lu status=%s nblocks=%ld time=%.2fs maxrss=%ldMB\n", n, e, adf_status_str(st),
           st == ADF_OK ? (long) adf_modctx_nblocks(c) : -1L,
           (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec), ru.ru_maxrss / 1024);
    adf_modctx_free(c);
    {   /* resident set after the call returned and the context (if any) was freed */
        long pages = 0, rss = 0;
        FILE * f = fopen("/proc/self/statm", "r");
        if (f && fscanf(f, "%ld %ld", &pages, &rss) == 2)
            printf("  resident after return: %ldMB\n", rss * sysconf(_SC_PAGESIZE) / (1024 * 1024));
        if (f) fclose(f);
    }
    return 0;
}
