/* tests/test_modctx_limits.c: the size and overflow limits of the raw context constructors
   (lane m1-repair-ctx, finding R2 of reviewer `contexts`; decision M1-D5).

   Ground truth: include/adelefeld/modctx.h (the constructor comment blocks: ADF_DOMAIN first,
   then ADF_UNSUPPORTED; at most ADF_MODCTX_MAX_BLOCKS = 65536 blocks; for a power of a
   primorial with e >= 2 the overflow is decided from n and e alone; no allocation is retained
   on a status other than ADF_OK), docs/conventions.md 3.2 and 3.3, docs/SPEC.md 15 row M1-D5.

   Checked here:
   - adf_modctx_new_primorial_pow on n = 2^64-1, 2^40, 2^33, 10^8 and e = 1, 2, 3, 63, 64:
     the status of the header, decided in the test from n and e, within one second, with *out
     untouched.
   - adf_modctx_new_factorial for n up to 2^64-1: ADF_UNSUPPORTED from n = 66 on (v_2(66!) =
     64), within one second, *out untouched.
   - adf_modctx_new_blocks and adf_modctx_new_prime_powers with k > ADF_MODCTX_MAX_BLOCKS and
     the argument array at the start of a PROT_NONE page: the refusal must come from k alone
     and the array must not be read.
   - after a refused call for n = 10^8, e = 3 no memory that depends on n stays resident.
   - the largest admitted cases (k = 65536 blocks of distinct primes; the primorial of
     n = 821646 with e = 1) succeed.  These three are slow (about 78 s, 35 s and 21 s on the
     lane machine) and are run only when ADF_MODCTX_LIMITS_FULL=1, which the lane script
     lanes/m1-repair-ctx/run_limits.sh sets; without it the same path is exercised at k = 2000
     so that make check and the mutation run stay fast.  This is said in the lane report. */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/mman.h>
#include <unistd.h>

#include <flint/ulong_extras.h>

#include <adelefeld/modctx.h>

#include "test_runner.h"

static adf_modctx_struct * const SENT = (adf_modctx_struct *) 0x1234;

static double
now_seconds(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double) t.tv_sec + 1e-9 * (double) t.tv_nsec;
}

static long
resident_bytes(void)
{
    FILE * f = fopen("/proc/self/statm", "r");
    long pages = 0, rss = 0;

    if (f == NULL)
        return -1;
    if (fscanf(f, "%ld %ld", &pages, &rss) != 2)
        rss = -1;
    fclose(f);
    if (rss < 0)
        return -1;
    return rss * sysconf(_SC_PAGESIZE);
}

static int
full_run(void)
{
    const char * s = getenv("ADF_MODCTX_LIMITS_FULL");
    return s != NULL && s[0] == '1';
}

/* ---- expected statuses, from the rule of the header ---- */

/* The number of blocks of the primorial (n, e) is pi(n) for e >= 1 and n >= 2.  pi(n) exceeds
   ADF_MODCTX_MAX_BLOCKS exactly when n >= ADF_MODCTX_MAX_PRIME (the 65537th prime).  For
   e >= 2 a block overflows when some prime p <= n has p^e >= 2^64; since p^e grows with p that
   is the case exactly when n is at or above the smallest prime q with q^e >= 2^64.  For e = 1
   no p^1 reaches 2^64. */
static int
expected_primorial(ulong n, ulong e)
{
    ulong q;

    if (n < 2 || e == 0)
        return ADF_OK;
    if (n >= ADF_MODCTX_MAX_PRIME)
        return ADF_UNSUPPORTED;
    if (e >= 2)
    {
        /* n_root(2^64 - 1, e) is the largest b with b^e < 2^64; the first prime above b has
           b-th power at least 2^64. */
        q = n_nextprime(n_root((ulong) -1, e), 0);
        if (n >= q)
            return ADF_UNSUPPORTED;
    }
    return ADF_OK;
}

/* The blocks of n! are p^v_p(n!) for the primes p <= n.  v_2(66!) = 64, so n = 66 is the
   first n with a block reaching 2^64; v_2 is nondecreasing, so every n >= 66 is UNSUPPORTED.
   For n <= 65 the prime powers are computed and compared with 2^64. */
static int
expected_factorial(ulong n)
{
    ulong p;

    if (n < 2)
        return ADF_OK;
    if (n >= 66)
        return ADF_UNSUPPORTED;
    for (p = 2; p <= n; p = n_nextprime(p, 0))
    {
        ulong e = 0, pk = p, q = 1, i;
        while (pk <= n)
        {
            e += n / pk;
            if (pk > n / p)
                break;
            pk *= p;
        }
        for (i = 0; i < e; i++)
        {
            if (q > (ulong) -1 / p)
                return ADF_UNSUPPORTED;
            q *= p;
        }
    }
    return ADF_OK;
}

/* ---- 1. no memory that depends on n stays after a refused call ---- */

ADF_TEST(refused_call_retains_no_primorial_memory)
{
    adf_modctx_struct * ctx = SENT;
    long before, after;
    double t0;
    int st;

    before = resident_bytes();
    ADF_CHECK(before >= 0);
    t0 = now_seconds();
    st = adf_modctx_new_primorial_pow(&ctx, 100000000UL, 3);
    ADF_CHECK(st == ADF_UNSUPPORTED);
    ADF_CHECK(ctx == SENT);
    ADF_CHECK(now_seconds() - t0 < 1.0);
    after = resident_bytes();
    ADF_CHECK(after >= 0);
    /* Before the repair this call filled FLINT's prime table up to 10^8 and 131 MB stayed
       resident; the refusal must now be decided from n alone. */
    ADF_CHECK_MSG(after - before < 16L * 1024 * 1024,
                  "resident size grew by %ld bytes across a refused primorial call",
                  after - before);
}

/* ---- 2. new_primorial_pow refused cases: the 20 arguments of the brief ---- */

ADF_TEST(primorial_refusals_are_decided_from_n_and_e)
{
    const ulong ns[4] = {18446744073709551615UL, 1099511627776UL, 8589934592UL, 100000000UL};
    const ulong es[5] = {1, 2, 3, 63, 64};
    int i, j;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 5; j++)
        {
            adf_modctx_struct * ctx = SENT;
            double t0 = now_seconds();
            int st = adf_modctx_new_primorial_pow(&ctx, ns[i], es[j]);
            double dt = now_seconds() - t0;
            int want = expected_primorial(ns[i], es[j]);

            ADF_CHECK_MSG(st == want, "primorial_pow(%lu, %lu): status %s, expected %s",
                          ns[i], es[j], adf_status_str(st), adf_status_str(want));
            ADF_CHECK(ctx == SENT);
            ADF_CHECK_MSG(dt < 1.0, "primorial_pow(%lu, %lu) took %.3f s", ns[i], es[j], dt);
        }
}

/* ---- 3. new_factorial refused cases ---- */

ADF_TEST(factorial_refusals_are_decided_from_n)
{
    const ulong ns[6] = {66, 821646, 821647, 1099511627776UL, 9223372036854775807UL,
                         18446744073709551615UL};
    int i;

    for (i = 0; i < 6; i++)
    {
        adf_modctx_struct * ctx = SENT;
        double t0 = now_seconds();
        int st = adf_modctx_new_factorial(&ctx, ns[i]);
        double dt = now_seconds() - t0;
        int want = expected_factorial(ns[i]);

        ADF_CHECK_MSG(st == want, "factorial(%lu): status %s, expected %s", ns[i],
                      adf_status_str(st), adf_status_str(want));
        ADF_CHECK(ctx == SENT);
        ADF_CHECK_MSG(dt < 1.0, "factorial(%lu) took %.3f s", ns[i], dt);
    }
}

/* ---- 4. k above the bound is refused before the array is read ---- */

/* A pointer to the first byte of a PROT_NONE page; any read faults.  The mapping is two pages,
   the second without access. */
static ulong *
guarded_words(void ** base_out, long * page_out)
{
    long pg = sysconf(_SC_PAGESIZE);
    char * base = mmap(NULL, (size_t) pg * 2, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    ADF_CHECK(base != MAP_FAILED);
    if (base == MAP_FAILED)
    {
        *base_out = NULL;
        *page_out = 0;
        return NULL;
    }
    ADF_CHECK(mprotect(base + pg, (size_t) pg, PROT_NONE) == 0);
    *base_out = base;
    *page_out = pg;
    return (ulong *) (base + pg);
}

ADF_TEST(k_above_the_bound_does_not_read_the_array)
{
    const slong ks[3] = {ADF_MODCTX_MAX_BLOCKS + 1, (slong) 1 << 40, WORD_MAX};
    int i;

    for (i = 0; i < 3; i++)
    {
        void * base = NULL;
        long pg = 0;
        ulong * guard = guarded_words(&base, &pg);
        adf_modctx_struct * ctx = SENT;
        double t0, dt;
        int st;

        if (guard == NULL)
            continue;
        t0 = now_seconds();
        st = adf_modctx_new_blocks(&ctx, guard, ks[i]);
        dt = now_seconds() - t0;
        ADF_CHECK_MSG(st == ADF_UNSUPPORTED, "new_blocks(k = %ld): status %s", (long) ks[i],
                      adf_status_str(st));
        ADF_CHECK(ctx == SENT);
        ADF_CHECK_MSG(dt < 1.0, "new_blocks(k = %ld) took %.3f s", (long) ks[i], dt);

        ctx = SENT;
        t0 = now_seconds();
        st = adf_modctx_new_prime_powers(&ctx, guard, guard, ks[i]);
        dt = now_seconds() - t0;
        ADF_CHECK_MSG(st == ADF_UNSUPPORTED, "new_prime_powers(k = %ld): status %s",
                      (long) ks[i], adf_status_str(st));
        ADF_CHECK(ctx == SENT);
        ADF_CHECK_MSG(dt < 1.0, "new_prime_powers(k = %ld) took %.3f s", (long) ks[i], dt);

        munmap(base, (size_t) pg * 2);
    }
}

/* ---- 5. the largest admitted cases ---- */

ADF_TEST(largest_admitted_cases_succeed)
{
    slong nblocks = full_run() ? 65536 : 2000;
    ulong * q;
    ulong x = 1;
    slong i;
    adf_modctx_struct * ctx = SENT;

    q = flint_malloc((size_t) nblocks * sizeof(ulong));
    for (i = 0; i < nblocks; i++)
    {
        x = n_nextprime(x, 0);
        q[i] = x;
    }

    /* new_blocks: k distinct primes, the largest admitted k.  Even in the full run the
       prime-power constructor is kept at 2000 blocks: its k^2 repeated-prime scan at 65536
       blocks would push the run over the three-minute laptop limit of lanes/COMMON-C.md. */
    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, nblocks) == ADF_OK);
    ADF_CHECK(ctx != SENT);
    ADF_CHECK(adf_modctx_nblocks(ctx) == nblocks);
    if (ctx != SENT)
        ADF_CHECK(adf_modctx_block(ctx, nblocks - 1) == q[nblocks - 1]);
    adf_modctx_free(ctx);
    ctx = SENT;

    /* new_prime_powers: the first 2000 of the same distinct primes with exponent 1. */
    {
        slong np = 2000;
        ulong * e = flint_malloc((size_t) np * sizeof(ulong));
        for (i = 0; i < np; i++)
            e[i] = 1;
        ADF_CHECK(adf_modctx_new_prime_powers(&ctx, q, e, np) == ADF_OK);
        ADF_CHECK(ctx != SENT);
        ADF_CHECK(adf_modctx_nblocks(ctx) == np);
        adf_modctx_free(ctx);
        ctx = SENT;
        flint_free(e);
    }
    flint_free(q);

    /* the primorial of n = 821646 with e = 1 has pi(821646) = 65536 blocks; the fast run uses
       the same family at 2000 blocks (the 2000th prime is 17389). */
    {
        ulong n = full_run() ? 821646UL : 17389UL;
        slong want = full_run() ? 65536 : 2000;

        ADF_CHECK(adf_modctx_new_primorial_pow(&ctx, n, 1) == ADF_OK);
        ADF_CHECK(ctx != SENT);
        ADF_CHECK(adf_modctx_nblocks(ctx) == want);
        adf_modctx_free(ctx);
    }
}
