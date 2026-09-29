#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <flint/fmpz.h>
#include <flint/fmpz_poly.h>
#include <adelefeld.h>

static uint64_t state = UINT64_C(0x91a73e40c2d8b65f);
static uint32_t rnd(void)
{
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return (uint32_t) state;
}

static long vp(long x, long p)
{
    long s = 0;
    if (x == 0) return 1000000;
    while (x % p == 0) { x /= p; s++; }
    return s;
}

static int in_class(long root, const fmpz_t a, long e, long p)
{
    fmpz_t d, q, P;
    int ans;
    fmpz_init_set_si(d, root);
    fmpz_sub(d, d, a);
    fmpz_init(q);
    fmpz_init_set_ui(P, (ulong) p);
    fmpz_pow_ui(q, P, (ulong) e);
    ans = fmpz_divisible(d, q);
    fmpz_clear(d); fmpz_clear(q); fmpz_clear(P);
    return ans;
}

int main(void)
{
    const long ps[] = {2, 3, 5};
    long cases = 0, calls = 0, incomplete = 0, certs = 0, classes = 0, errors = 0;
    long ip, it;
    for (ip = 0; ip < 3; ip++) for (it = 0; it < 600; it++)
    {
        long p = ps[ip], roots[4], s[4], n = 2 + (rnd() % 3);
        long t = rnd() % 8, delta = 1, i, j, maxs = 0, depth;
        long scale = 1 + (rnd() % 3), base = (long) (rnd() % 31) - 15;
        adf_place_t place;
        fmpz_poly_t f, linear;
        adf_rootlist_t L, S;
        for (i = 0; i < t; i++) delta *= p;
        roots[0] = base;
        for (i = 1; i < n; i++) roots[i] = base + (long) i * delta;
        for (i = 0; i < n; i++)
        {
            s[i] = 0;
            for (j = 0; j < n; j++) if (i != j) s[i] += vp(roots[i] - roots[j], p);
            if (s[i] > maxs) maxs = s[i];
        }
        if (adf_place_prime(&place, (ulong) p) != ADF_OK) return 2;
        fmpz_poly_init(f); fmpz_poly_init(linear);
        fmpz_poly_set_si(f, (rnd() & 1) ? scale * p : -scale);
        for (i = 0; i < n; i++)
        {
            long multiplicity = 1 + (rnd() % 3);
            fmpz_poly_zero(linear);
            fmpz_poly_set_coeff_si(linear, 1, 1);
            fmpz_poly_set_coeff_si(linear, 0, -roots[i]);
            for (j = 0; j < multiplicity; j++) fmpz_poly_mul(f, f, linear);
        }
        adf_rootlist_init(L); adf_rootlist_init(S);
        for (depth = 0; depth <= maxs + 1; depth++)
        {
            int st = adf_roots_padic_partial(L, f, place, 1 + (rnd() % 4), depth);
            int strict;
            calls++;
            if (st != ADF_OK) { errors++; break; }
            if (L->n + L->nu == 0 || L->complete != (L->nu == 0) || L->scope != ADF_ROOTLIST_PARTITION)
                errors++;
            if (!adf_rootlist_verify_entries(L, f)) errors++;
            if (adf_rootlist_verify_complete(L, f, depth) != L->complete) errors++;
            for (i = 0; i < n; i++)
            {
                long hits = 0;
                for (j = 0; j < L->n; j++) hits += in_class(roots[i], L->a + j, L->K[j], p);
                for (j = 0; j < L->nu; j++) hits += in_class(roots[i], L->ua + j, L->ue[j], p);
                if (hits != 1) errors++;
            }
            for (i = 0; i < L->n; i++)
            {
                long hits = 0, which = -1;
                for (j = 0; j < n; j++) if (in_class(roots[j], L->a + i, L->K[i], p))
                {
                    hits++;
                    which = j;
                }
                if (hits != 1 || (which >= 0 && L->s[i] != s[which])) errors++;
                certs++;
            }
            classes += L->nu;
            if (!L->complete) incomplete++;
            strict = adf_roots_padic(S, f, place, 2, depth);
            if (strict != (L->complete ? ADF_OK : ADF_NOT_DETERMINED)) errors++;
            if (errors) {
                printf("failure p=%ld case=%ld depth=%ld n=%ld smax=%ld errors=%ld\n",
                       p, it, depth, n, maxs, errors);
                break;
            }
        }
        if (depth != maxs + 2 || L->n != n || !L->complete) errors++;
        cases++;
        adf_rootlist_clear(L); adf_rootlist_clear(S);
        fmpz_poly_clear(f); fmpz_poly_clear(linear);
        if (errors) break;
    }
    printf("cases=%ld partial_calls=%ld incomplete=%ld certs=%ld classes=%ld errors=%ld\n",
           cases, calls, incomplete, certs, classes, errors);
    return errors ? 1 : 0;
}
