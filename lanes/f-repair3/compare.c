/* f-repair3: differential comparison of the library's sin, cos, sinh, cosh against ref_lfunc.c, a copy of
   src/lfunc.c taken before the paired Horner loop (sha256 5951d223...6edc0), its seven external names renamed
   adf_lball_* -> ref_lball_*. A case fails on a different status, or on a result (or untouched output) that is
   not adf_lball_identical, for distinct and for aliased output. Usage:
     compare            the differential run; prints counts and exits 1 on any difference
     compare --time     the review's case p = 3, N = 2000, x = 3/2: one call of each implementation, CPU ms */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <adelefeld.h>

int ref_lball_sin(adf_lball_t y, const adf_lball_t x, slong N);
int ref_lball_cos(adf_lball_t y, const adf_lball_t x, slong N);
int ref_lball_sinh(adf_lball_t y, const adf_lball_t x, slong N);
int ref_lball_cosh(adf_lball_t y, const adf_lball_t x, slong N);

typedef int (*fn_t)(adf_lball_t, const adf_lball_t, slong);
static fn_t lib[] = {adf_lball_sin, adf_lball_cos, adf_lball_sinh, adf_lball_cosh};
static fn_t ref[] = {ref_lball_sin, ref_lball_cos, ref_lball_sinh, ref_lball_cosh};
static const char *names[] = {"sin", "cos", "sinh", "cosh"};
static const int odd_of[] = {1, 0, 1, 0};

/* Statistics of the sums actually formed (replicating parity_apply's path and F10's count). */
static ulong summed_at[7], current_prime;
static ulong summed[4], c_even[4], c_odd[4], l_one[4], l_two[4], l_max[4], status_count[4][16];

/* x = q (exact) or q + p^M Z_p (ball), q = s p^v num/den. */
static int set_input(adf_lball_t x, ulong p, const fmpz_t num, const fmpz_t den, slong v, int exact, slong M)
{
    adf_rat_t r;
    adf_place_t place;
    fmpz_t pp;
    int st;
    adf_rat_init(r);
    fmpz_init_set_ui(pp, p);
    fmpq_set_fmpz_frac(r->q, num, den);
    if (v >= 0)
    {
        fmpz_pow_ui(pp, pp, (ulong) v);
        fmpq_mul_fmpz(r->q, r->q, pp);
    }
    else
    {
        fmpz_pow_ui(pp, pp, (ulong) -v);
        fmpq_div_fmpz(r->q, r->q, pp);
    }
    adf_place_prime(&place, p);
    st = exact ? adf_lball_set_rat(x, place, r) : adf_lball_set_rat_ball(x, place, r, M);
    adf_rat_clear(r);
    fmpz_clear(pp);
    return st;
}

static void random_unit(fmpz_t a, flint_rand_t st, ulong p, ulong bits)
{
    fmpz_t pp;
    fmpz_init_set_ui(pp, p);
    do
    {
        fmpz_randbits(a, st, 1 + n_randint(st, bits));
        fmpz_remove(a, a, pp);
    }
    while (fmpz_is_zero(a));
    fmpz_clear(pp);
}

static void tally(int f, const adf_lball_t x, slong N, int status)
{
    ulong p = x->p;
    int odd = odd_of[f];
    slong K, threshold, C, L;
    fmpz_t a, b;
    status_count[f][status & 15]++;
    if (status != ADF_OK || fmpq_is_zero(x->u))
        return;
    K = x->exact || N < x->N ? N : x->N;
    threshold = odd ? x->v : 2 * x->v - (p == 2);
    if (K <= threshold)
        return;
    fmpz_init_set_ui(a, p - 1); fmpz_init_set_ui(b, p - 1);
    fmpz_mul_si(a, a, K); fmpz_sub_ui(a, a, 1);
    fmpz_mul_si(b, b, x->v); fmpz_sub_ui(b, b, 1);
    fmpz_cdiv_q(a, a, b);
    C = fmpz_cmp_si(a, 1) < 0 ? 1 : fmpz_get_si(a);
    fmpz_clear(a); fmpz_clear(b);
    L = C - 1;
    if (L % 2 != odd)
        L--;
    summed[f]++;
    summed_at[current_prime]++;
    if (C % 2) c_odd[f]++; else c_even[f]++;
    if (L == 1) l_one[f]++;
    if (L == 2) l_two[f]++;
    if ((ulong) L > l_max[f]) l_max[f] = (ulong) L;
}

static double cpu_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static int timing(void)
{
    adf_lball_t x, y1, y2;
    fmpz_t n, d;
    int bad = 0;
    adf_lball_init(x); adf_lball_init(y1); adf_lball_init(y2);
    fmpz_init_set_ui(n, 3); fmpz_init_set_ui(d, 2);
    set_input(x, 3, n, d, 0, 1, 0);
    for (int f = 0; f < 4; f++)
    {
        double t0, t1, t2;
        int s1, s2;
        t0 = cpu_ms(); s1 = ref[f](y1, x, 2000);
        t1 = cpu_ms(); s2 = lib[f](y2, x, 2000);
        t2 = cpu_ms();
        printf("%-4s p=3 N=2000 x=3/2: reference %.3f ms, library %.3f ms, status %d %d, identical %d, N %ld\n",
               names[f], t1 - t0, t2 - t1, s1, s2, adf_lball_identical(y1, y2), y2->N);
        bad |= s1 != s2 || !adf_lball_identical(y1, y2);
    }
    adf_lball_clear(x); adf_lball_clear(y1); adf_lball_clear(y2);
    fmpz_clear(n); fmpz_clear(d);
    return bad;
}

int main(int argc, char **argv)
{
    const ulong primes[] = {2, 3, 5, 7, 13, 65537, UWORD(18446744073709551557)};
    const int per_prime[] = {450, 900, 450, 450, 450, 300, 300};
    flint_rand_t st;
    adf_lball_t x, y1, y2, z1, z2, sentinel;
    fmpz_t num, den, one;
    ulong inputs = 0, calls = 0, edges = 0, alias_calls = 0, diffs = 0, kinds[5] = {0};
    if (argc > 1 && !strcmp(argv[1], "--time"))
        return timing();
    flint_randinit(st);
    adf_lball_init(x); adf_lball_init(y1); adf_lball_init(y2); adf_lball_init(z1); adf_lball_init(z2);
    adf_lball_init(sentinel);
    fmpz_init(num); fmpz_init(den); fmpz_init_set_ui(one, 1);
    fmpz_set_si(num, -23); fmpz_set_ui(den, 29);
    set_input(sentinel, 19, num, den, 0, 1, 0);
    for (int ip = 0; ip < 7; ip++)
    {
        ulong p = primes[ip];
        current_prime = (ulong) ip;
        slong c = p == 2 ? 2 : 1, nmax = p == 3 ? 2000 : 300;
        for (int i = 0; i < per_prime[ip]; i++)
        {
            int kind = (int) n_randint(st, 10), exact;
            slong v, M = 0, N;
            ulong bits = n_randint(st, 8) == 0 ? 3000 : 64;
            /* N: from the domain edge (N <= threshold, N <= 0) up to nmax; one in three large */
            if (n_randint(st, 3) == 0)
                N = 40 + (slong) n_randint(st, (ulong) (nmax - 39));
            else
                N = -2 + (slong) n_randint(st, 43);
            random_unit(num, st, p, bits);
            random_unit(den, st, p, bits);
            if (n_randint(st, 2)) fmpz_neg(num, num);
            /* v from c - 2 (outside the domain) to c + 5 */
            v = c - 2 + (slong) n_randint(st, 8);
            if (kind <= 3)
            {
                exact = 1; kinds[0]++;                      /* exact nonzero */
            }
            else if (kind <= 6)
            {
                exact = 0; kinds[1]++;                      /* noncentred ball, M > v */
                M = v + 1 + (slong) n_randint(st, n_randint(st, 2) ? 6 : (ulong) nmax);
            }
            else if (kind <= 8)
            {
                exact = 0; kinds[2]++;                      /* ball around 0: M <= v, or a zero centre */
                M = c - 2 + (slong) n_randint(st, n_randint(st, 2) ? 6 : (ulong) nmax);
                if (n_randint(st, 2)) fmpz_zero(num);
            }
            else
            {
                exact = 1; kinds[3]++; fmpz_zero(num);       /* exact zero */
            }
            if (fmpz_is_zero(num)) fmpz_one(den);
            if (set_input(x, p, num, den, v, exact, M) != ADF_OK)
            {
                kinds[4]++;
                continue;
            }
            inputs++;
            for (int f = 0; f < 4; f++)
            {
                int s1, s2;
                adf_lball_set(y1, sentinel); adf_lball_set(y2, sentinel);
                s1 = ref[f](y1, x, N);
                s2 = lib[f](y2, x, N);
                calls++;
                tally(f, x, N, s2);
                if (s1 != s2 || !adf_lball_identical(y1, y2))
                {
                    diffs++;
                    if (diffs <= 10)
                        printf("DIFF %s p=%lu v=%ld M=%ld exact=%d N=%ld status %d %d\n",
                               names[f], p, x->v, x->N, x->exact, N, s1, s2);
                }
                if (N <= 300)
                {
                    adf_lball_set(z1, x); adf_lball_set(z2, x);
                    s1 = ref[f](z1, z1, N);
                    s2 = lib[f](z2, z2, N);
                    alias_calls++;
                    if (s1 != s2 || !adf_lball_identical(z1, z2))
                    {
                        diffs++;
                        if (diffs <= 10)
                            printf("DIFF alias %s p=%lu v=%ld N=%ld\n", names[f], p, x->v, N);
                    }
                }
            }
        }
    }
    /* Fixed edge inputs: requested N at the slong ends, and the W-only LIMIT (K bits(p) <= BITS_MAX < W bits(p)) */
    {
        const ulong ep[] = {3, 2, 3, 2};
        const slong ex[] = {3, 4, 0, 0};
        const slong en[] = {LONG_MIN, LONG_MAX, ADF_LBALL_BITS_MAX / 2, ADF_LBALL_BITS_MAX / 2};
        for (int e = 0; e < 4; e++)
        for (int n = 0; n < 4; n++)
        for (int f = 0; f < 4; f++)
        {
            int s1, s2;
            ulong p = ep[e];
            current_prime = p == 2 ? 0 : 1;
            if (ex[e] == 0)
            {
                if (n == 1) continue;                     /* exact zero: no sum at any N; skip one */
                fmpz_zero(num);
                set_input(x, p, num, one, 0, e == 2, 3);
            }
            else
            {
                fmpz_set_si(num, ex[e]);
                set_input(x, p, num, one, 0, 1, 0);
            }
            adf_lball_set(y1, sentinel); adf_lball_set(y2, sentinel);
            s1 = ref[f](y1, x, en[n]);
            s2 = lib[f](y2, x, en[n]);
            calls++; edges++;
            tally(f, x, en[n], s2);
            if (s1 != s2 || !adf_lball_identical(y1, y2))
            {
                diffs++;
                printf("DIFF edge %s p=%lu x=%ld N=%ld status %d %d\n", names[f], p, ex[e], en[n], s1, s2);
            }
        }
    }
    printf("inputs %lu (exact nonzero %lu, noncentred balls %lu, balls about 0 %lu, exact zero %lu; "
           "%lu draws refused by set_rat_ball)\n", inputs, kinds[0], kinds[1], kinds[2], kinds[3], kinds[4]);
    printf("calls %lu distinct (%lu of them fixed edge calls), %lu aliased\n", calls, edges, alias_calls);
    for (int f = 0; f < 4; f++)
        printf("%-4s sums formed %lu: term count C even %lu, odd %lu; L=1 %lu, L=2 %lu, max L %lu; "
               "status OK %lu DOMAIN %lu NOT_DETERMINED %lu LIMIT %lu\n", names[f], summed[f], c_even[f],
               c_odd[f], l_one[f], l_two[f], l_max[f], status_count[f][ADF_OK], status_count[f][ADF_DOMAIN],
               status_count[f][ADF_NOT_DETERMINED], status_count[f][ADF_LIMIT]);
    printf("sums formed at p = 2, 3, 5, 7, 13, 65537, 2^64-59 (with the edge calls):");
    for (int ip = 0; ip < 7; ip++)
        printf(" %lu", summed_at[ip]);
    printf("\n");
    printf("differences %lu\n", diffs);
    adf_lball_clear(x); adf_lball_clear(y1); adf_lball_clear(y2); adf_lball_clear(z1); adf_lball_clear(z2);
    adf_lball_clear(sentinel);
    fmpz_clear(num); fmpz_clear(den); fmpz_clear(one);
    flint_randclear(st);
    return diffs != 0;
}
