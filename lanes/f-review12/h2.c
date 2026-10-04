#include <stdlib.h>
#include <stdio.h>
#include <adelefeld.h>
/* idele (sign, r=p, u = 1 at p, p^-1 elsewhere, to precision N = p^a * M) */
static long fails = 0, checks = 0;
int main(void)
{
    ulong ps[] = {2, 3, 5, 7, 11, 13};
    for (int ip = 0; ip < 6; ip++)
    for (int a = 1; a <= 3; a++)
    for (int sg = 0; sg < 2; sg++)
    for (int mm = 0; mm < 5; mm++)
    {
        ulong p = ps[ip];
        ulong Ms[] = {1, 7*11, 9, 5*7, 16*3};
        ulong M = Ms[mm];
        if (M % p == 0) continue;
        fmpz_t pa, MM, N, c, tmp, nn, j;
        fmpz_init(pa); fmpz_init(MM); fmpz_init(N); fmpz_init(c); fmpz_init(tmp); fmpz_init(nn); fmpz_init(j);
        fmpz_set_ui(pa, p); fmpz_pow_ui(pa, pa, a);
        fmpz_set_ui(MM, M);
        fmpz_mul(N, pa, MM);
        /* c = 1 mod p^a, c = p^-1 mod M */
        fmpz_set_ui(tmp, p);
        if (M == 1) fmpz_zero(tmp); else fmpz_invmod(tmp, tmp, MM);
        fmpz_CRT(c, tmp, MM, (fmpz *) (fmpz[]){1}, pa, 0);
        /* when M = 1, CRT with modulus 1 fine */
        adf_ucoset_t u; adf_ucoset_init(u);
        if (adf_ucoset_set_fmpz2(u, c, N) != ADF_OK) { printf("bad ucoset p=%lu\n", p); return 1; }
        arb_t inf; arb_init(inf); arb_set_si(inf, sg ? -3 : 3);
        fmpq_t r; fmpq_init(r); fmpq_set_ui(r, p, 1);
        adf_idele_t x; adf_idele_init(x);
        if (adf_idele_set_parts(x, inf, r, u) != ADF_OK) { printf("bad idele\n"); return 1; }
        adf_idclass_t k; adf_idclass_init(k);
        int st = adf_idclass_set_idele(k, x, 64);
        if (st) { printf("set_idele st %d\n", st); return 1; }
        /* test all n in 1..200 */
        for (ulong n = 1; n <= 200; n++)
        {
            fmpz_set_ui(nn, n);
            int s1 = adf_idclass_cyclo_exp_uinv(j, NULL, k, nn);
            /* oracle: unit of the class = sign * c mod N; reduction mod n determined iff all units agree */
            /* true: u' = sg? -c : c, units in coset; reduce mod n over lcm */
            ulong Nn = fmpz_get_ui(N);
            long cc = (long) fmpz_get_ui(c); if (sg) cc = -cc;
            ulong L = Nn / (ulong) n_gcd(Nn, n) * n; /* lcm */
            long val = -1; int one = 1;
            for (ulong b = 1; b <= L; b++)
            {
                if (n_gcd(b, L) != 1) continue;
                long diff = (long) b - cc; if (diff % (long) Nn) continue;
                long v = (long) (b % n);
                if (val < 0) val = v; else if (v != val) { one = 0; break; }
            }
            checks++;
            if (one)
            {
                ulong inv = n == 1 ? 0 : n_invmod((ulong) val, n);
                if (s1 != ADF_OK || fmpz_cmp_ui(j, inv)) { fails++; printf("FAIL p=%lu a=%d sg=%d M=%lu n=%lu st=%d j=", p, a, sg, M, n, s1); fmpz_print(j); printf(" want %lu\n", inv); }
                /* the SPEC vector: p at p; n prime to p dividing N: exponent p (positive sign) */
                if (!sg && n_gcd(n, p) == 1 && Nn % n == 0 && n > 2 && fmpz_cmp_ui(j, p % n)) { fails++; printf("VECTOR FAIL p=%lu n=%lu\n", p, n); }
            }
            else if (s1 != ADF_NOT_DETERMINED) { fails++; printf("FAIL nd p=%lu a=%d n=%lu st=%d\n", p, a, n, s1); }
        }
        adf_idclass_clear(k); adf_idele_clear(x); fmpq_clear(r); arb_clear(inf); adf_ucoset_clear(u);
        fmpz_clear(pa); fmpz_clear(MM); fmpz_clear(N); fmpz_clear(c); fmpz_clear(tmp); fmpz_clear(nn); fmpz_clear(j);
    }
    printf("idele vector checks %ld fails %ld\n", checks, fails);
    return fails != 0;
}
