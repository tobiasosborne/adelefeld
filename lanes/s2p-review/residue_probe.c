#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <flint/fmpz_poly.h>
#include <adelefeld.h>

static uint64_t state = UINT64_C(0x75dc8e12a369b04f);
static uint32_t rnd(void)
{
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return (uint32_t) state;
}

static long evaluate(const long * c, long d, long x, long mod, int derivative)
{
    long i, v = 0;
    if (derivative)
        for (i = d; i >= 1; i--) v = (v * x + i * c[i]) % mod;
    else
        for (i = d; i >= 0; i--) v = (v * x + c[i]) % mod;
    return (v + mod) % mod;
}

static long pv(long x, long p)
{
    long s = 0;
    if (x == 0) return 1000000;
    while (x % p == 0) { x /= p; s++; }
    return s;
}

static int congruent(long x, const fmpz_t a, long p, long e)
{
    fmpz_t d, pp;
    int yes;
    fmpz_init_set_si(d, x);
    fmpz_sub(d, d, a);
    fmpz_init_set_ui(pp, (ulong) p);
    fmpz_pow_ui(pp, pp, (ulong) e);
    yes = fmpz_divisible(d, pp);
    fmpz_clear(d); fmpz_clear(pp);
    return yes;
}

int main(void)
{
    const long pp[] = {2, 3, 5};
    const long mm[] = {16384, 19683, 15625};
    long ip, it, attempted = 0, usable = 0, nonintegral = 0, roots_total = 0;
    long partial = 0, failures = 0;
    for (ip = 0; ip < 3; ip++) for (it = 0; it < 2000; it++)
    {
        long p = pp[ip], mod = mm[ip], c[7], d = 2 + (rnd() % 5), x, i, j, nroot = 0;
        long reps[7], moduli[7], apos = 0, st;
        int usable_case = 1;
        fmpz_poly_t f;
        adf_rootlist_t L;
        adf_place_t place;
        c[d] = 1 + (rnd() % 5);
        for (i = 0; i < d; i++) c[i] = (long) (rnd() % 61) - 30;
        fmpz_poly_init(f);
        for (i = 0; i <= d; i++) fmpz_poly_set_coeff_si(f, i, c[i]);
        attempted++;
        for (x = 0; x < mod; x++) if (evaluate(c, d, x, mod, 0) == 0)
        {
            long deriv = evaluate(c, d, x, mod, 1), s = pv(deriv, p), q = 1;
            if (s >= 1000000 || 2 * s >= pv(mod, p)) { usable_case = 0; break; }
            for (i = 0; i <= s; i++) q *= p;
            for (i = 0; i < nroot; i++) if (reps[i] % q == x % q && moduli[i] == q) break;
            if (i == nroot)
            {
                if (nroot >= 7) { usable_case = 0; break; }
                reps[nroot] = x;
                moduli[nroot] = q;
                nroot++;
            }
            apos++;
        }
        if (!usable_case) { fmpz_poly_clear(f); continue; }
        usable++;
        roots_total += nroot;
        if (nroot && c[0] != 0) nonintegral++;
        if (adf_place_prime(&place, (ulong) p) != ADF_OK) return 2;
        adf_rootlist_init(L);
        st = adf_roots_padic(L, f, place, 1, 12);
        if (st != ADF_OK || L->n != nroot || L->nu || !L->complete) failures++;
        else
        {
            for (i = 0; i < nroot; i++)
            {
                long hits = 0;
                for (j = 0; j < L->n; j++)
                {
                    if (L->K[j] > pv(mod, p) - pv(evaluate(c, d, reps[i], mod, 1), p))
                    { failures++; continue; }
                    hits += congruent(reps[i], L->a + j, p, L->K[j]);
                }
                if (hits != 1) failures++;
            }
            for (i = 0; i < L->n; i++)
            {
                long hits = 0;
                for (j = 0; j < nroot; j++) hits += congruent(reps[j], L->a + i, p, L->K[i]);
                if (hits != 1) failures++;
            }
            if (!adf_rootlist_verify_complete(L, f, 12)) failures++;
        }
        if (nroot)
        {
            st = adf_roots_padic_partial(L, f, place, 2, 0);
            if (st != ADF_OK) failures++;
            else
            {
                for (i = 0; i < nroot; i++)
                {
                    long hits = 0;
                    for (j = 0; j < L->n; j++) hits += congruent(reps[i], L->a + j, p, L->K[j]);
                    for (j = 0; j < L->nu; j++) hits += congruent(reps[i], L->ua + j, p, L->ue[j]);
                    if (hits != 1) failures++;
                }
                partial++;
            }
        }
        if (failures)
        {
            printf("failure p=%ld case=%ld deg=%ld approx=%ld oracle=%ld status=%ld listed=%ld ",
                   p, it, d, apos, nroot, st, L->n);
            for (i = 0; i <= d; i++) printf("%ld%s", c[i], i == d ? "\n" : ",");
            return 1;
        }
        adf_rootlist_clear(L);
        fmpz_poly_clear(f);
    }
    printf("attempted=%ld usable=%ld with_nonzero_constant_and_roots=%ld roots=%ld partial=%ld failures=%ld\n",
           attempted, usable, nonintegral, roots_total, partial, failures);
    return 0;
}
