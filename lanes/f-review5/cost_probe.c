/* A single measurement of visiting the absent parity, p=3, x=3/2, N=2000.
   This is a review reproducer, not a proposed library replacement.

   Own algebra: in two successive Horner steps with epsilon_(k-1)=0,
   F' = k F; A' = x A; F'' = (k-1)F'; A'' = x A' + epsilon_(k-2) F''.
   Hence F'' = k(k-1)F and A'' = x^2 A + epsilon_(k-2) F''.
   Reduction modulo P commutes with these integer operations. For an odd top degree,
   the last degree-1-to-0 step remains A = x A, F = F. Division at the end is unchanged.
   Pairing therefore visits only nonzero coefficients with identical A and F residues.
*/
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <time.h>

typedef int (*fn)(adf_lball_t, const adf_lball_t, slong);
static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return t.tv_sec + t.tv_nsec*1e-9;
}
static int coefficient(slong k, int alternating)
{
    return alternating && (k/2) % 2 ? -1 : 1;
}
static void paired(fmpz_t out, int odd, int alternating, slong *degree, slong *steps)
{
    const slong K = 2000;
    slong L = odd ? 3997 : 3998, D = 0, n = L, W, k;
    fmpz_t P, PD, PK, x, x2, A, F, tmp;
    while (n >= 3) { n /= 3; D += n; }
    W = K+D;
    fmpz_init(P); fmpz_init(PD); fmpz_init(PK); fmpz_init(x);
    fmpz_init(x2); fmpz_init(A); fmpz_init(F); fmpz_init(tmp);
    fmpz_ui_pow_ui(P, 3, W); fmpz_ui_pow_ui(PD, 3, D); fmpz_ui_pow_ui(PK, 3, K);
    fmpz_set_ui(tmp, 2); fmpz_invmod(x, tmp, P); fmpz_mul_ui(x, x, 3); fmpz_mod(x, x, P);
    fmpz_mul(x2, x, x); fmpz_mod(x2, x2, P);
    fmpz_set_si(A, coefficient(L, alternating)); fmpz_one(F);
    *steps = 0; *degree = L;
    for (k = L; k >= 2; k -= 2)
    {
        fmpz_mul_ui(F, F, (ulong) k*(k-1)); fmpz_mod(F, F, P);
        fmpz_mul(A, A, x2);
        if (coefficient(k-2, alternating) > 0) fmpz_add(A, A, F);
        else fmpz_sub(A, A, F);
        fmpz_mod(A, A, P); (*steps)++;
    }
    if (odd) { fmpz_mul(A, A, x); fmpz_mod(A, A, P); (*steps)++; }
    fmpz_divexact(A, A, PD); fmpz_divexact(F, F, PD);
    fmpz_invmod(tmp, F, PK); fmpz_mul(out, A, tmp); fmpz_mod(out, out, PK);
    fmpz_clear(P); fmpz_clear(PD); fmpz_clear(PK); fmpz_clear(x);
    fmpz_clear(x2); fmpz_clear(A); fmpz_clear(F); fmpz_clear(tmp);
}
int main(void)
{
    fn fs[] = {adf_lball_sin, adf_lball_cos, adf_lball_sinh, adf_lball_cosh};
    const char *names[] = {"sin", "cos", "sinh", "cosh"};
    adf_lball_t x, y;
    fmpz_t a, b, pow;
    adf_lball_init(x); adf_lball_init(y); fmpz_init(a); fmpz_init(b); fmpz_init(pow);
    x->p = 3; x->v = 1; fmpq_set_si(x->u, 1, 2);
    for (int f = 0; f < 4; f++)
    {
        slong L, steps;
        double t0 = now();
        int st = fs[f](y, x, 2000);
        double t1 = now();
        paired(a, 1-f % 2, f < 2, &L, &steps);
        double t2 = now();
        fmpz_ui_pow_ui(pow, 3, y->v); fmpz_mul(b, fmpq_numref(y->u), pow);
        int equal = st == ADF_OK && y->N == 2000 && !y->exact && fmpz_equal(a, b);
        printf("f=%s p=3 x=3/2 N=2000 original_steps=%ld paired_steps=%ld "
               "original_ms=%.6f paired_ms=%.6f ratio=%.3f equal=%d\n",
               names[f], L, steps, (t1-t0)*1000, (t2-t1)*1000, (t1-t0)/(t2-t1), equal);
        if (!equal) return 1;
    }
    adf_lball_clear(x); adf_lball_clear(y); fmpz_clear(a); fmpz_clear(b); fmpz_clear(pow);
    flint_cleanup(); return 0;
}
