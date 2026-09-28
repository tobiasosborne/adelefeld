/* place_check.c: reads unsigned 64-bit numbers from stdin, one per line, and writes for each
   "p status is_arch prime_get" from adf_place_prime; then checks the order and equality of the
   places made against the numeric order (conventions 7). Built and driven by place_check.py. */
#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>

int main(void)
{
    static adf_place_t pl[400000];
    static ulong pv[400000];
    size_t n = 0, i, bad = 0;
    unsigned long long p;

    while (scanf("%llu", &p) == 1)
    {
        adf_place_t v;
        int s;
        v.opaque = 0xDEADBEEFUL;    /* sentinel: must stay on DOMAIN (conventions 4.3) */
        s = adf_place_prime(&v, (ulong) p);
        if (s != ADF_OK && v.opaque != 0xDEADBEEFUL)
            printf("TOUCHED %llu\n", p);
        printf("%llu %d %d %lu\n", p, s, s == ADF_OK ? adf_place_is_archimedean(v) : -1,
               s == ADF_OK ? adf_place_prime_get(v) : 0UL);
        if (s == ADF_OK && n < 400000) { pl[n] = v; pv[n] = (ulong) p; n++; }
    }
    /* order: inf first, then numeric; 2000 x 2000 pairs */
    for (i = 0; i < n && i < 2000; i++)
    {
        size_t j;
        if (adf_place_cmp(adf_place_inf(), pl[i]) != -1 || adf_place_cmp(pl[i], adf_place_inf()) != 1)
            bad++;
        for (j = 0; j < n && j < 2000; j++)
        {
            int want = (pv[i] > pv[j]) - (pv[i] < pv[j]);
            if (adf_place_cmp(pl[i], pl[j]) != want || adf_place_equal(pl[i], pl[j]) != (want == 0))
                bad++;
        }
    }
    fprintf(stderr, "order checks: %zu places, %zu bad\n", n, bad);
    return bad != 0;
}
