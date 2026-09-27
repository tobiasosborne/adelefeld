/* HISTORICAL REFERENCE (2026-09-27). Predates the benchmark contract of docs/PERF.md section 7: the nmod rows are
   dependent chains, the n_gcd row is a chain mixed with input generation, the fmpz and arb rows are call rates on
   fixed operands. Replaced by work package 0.5. Changes after the review: the unused seed argument is removed and
   the constant b is kept below p.
   bench/baseline.c: cost of the FLINT/GMP primitives that adelefeld's milestone-1 types will be built from.
   These are reference measurements of existing code, not of adelefeld. Floors and ratios: docs/PERF.md.
   Build: gcc -O2 bench/baseline.c -o build/baseline -lflint -lmpfr -lgmp ; run: taskset -c 2 build/baseline
   Harness after ~/.claude/skills/perf-bounds/references/measurement.md. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <flint/flint.h>
#include <flint/ulong_extras.h>
#include <flint/nmod.h>
#include <flint/fmpz.h>
#include <flint/arb.h>
#include <flint/acb.h>

static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
#define TIME_KERNEL(name, trials, reps, BODY) do {                   \
    double v[trials];                                                \
    for (int t_ = 0; t_ < (trials); t_++) {                          \
        double t0_ = now();                                          \
        for (long k_ = 0; k_ < (reps); k_++) { BODY;                 \
            __asm__ volatile("" ::: "memory"); }                     \
        v[t_] = (now() - t0_) / (reps) * 1e9; }                      \
    for (int i_ = 1; i_ < (trials); i_++) { double x_ = v[i_]; int j_ = i_; \
        while (j_ > 0 && v[j_-1] > x_) { v[j_] = v[j_-1]; j_--; } v[j_] = x_; } \
    printf("%-34s min %9.2f ns  median %9.2f ns  max %9.2f ns\n", name, v[0], v[(trials)/2], v[(trials)-1]); \
} while (0)

int main(int argc, char **argv)
{
    flint_rand_t st; flint_randinit(st);
    (void) argc; (void) argv;
    ulong p = (UWORD(1) << 62) - 57;
    nmod_t mod; nmod_init(&mod, p);
    ulong x = n_randint(st, p), b = (n_randint(st, p - 2) | 1), acc = 0;

    TIME_KERNEL("nmod_add chain (62-bit)", 15, 20000000, x = nmod_add(x, b, mod));
    acc += x;
    TIME_KERNEL("nmod_mul chain (62-bit)", 15, 20000000, x = nmod_mul(x, b, mod));
    acc += x;
    { ulong u = n_randbits(st, 62) | 1, w = n_randbits(st, 62);
      TIME_KERNEL("n_gcd (62-bit, fresh inputs)", 15, 2000000, acc += n_gcd(u, w); u += 2 * (acc & 3) + 2; w += 3); }

    fmpz_t A, B, C, D, M;
    fmpz_init(A); fmpz_init(B); fmpz_init(C); fmpz_init(D); fmpz_init(M);
    fmpz_randbits(A, st, 4096); fmpz_abs(A, A); fmpz_randbits(B, st, 4096); fmpz_abs(B, B);
    fmpz_randbits(M, st, 4096); fmpz_abs(M, M);
    TIME_KERNEL("fmpz_add 4096 bit", 15, 1000000, fmpz_add(C, A, B));
    TIME_KERNEL("fmpz_mul 4096 x 4096 bit", 15, 100000, fmpz_mul(D, A, B));
    TIME_KERNEL("fmpz_mod 8192 by 4096 bit", 15, 100000, fmpz_mod(C, D, M));
    TIME_KERNEL("fmpz_gcd 4096 bit", 9, 5000, fmpz_gcd(C, A, B));
    acc += fmpz_get_ui(C);

    arb_t X, Y, Z; arb_init(X); arb_init(Y); arb_init(Z);
    arb_set_ui(X, 3); arb_sqrt(X, X, 128); arb_set_ui(Y, 5); arb_sqrt(Y, Y, 128); arb_inv(Y, Y, 128);
    TIME_KERNEL("arb_add 128 bit", 15, 5000000, arb_add(Z, X, Y, 128));
    TIME_KERNEL("arb_mul 128 bit", 15, 5000000, arb_mul(Z, X, Y, 128));
    printf("checksum %lu\n", (unsigned long) acc);
    flint_printf("FLINT %s, sizeof: fmpz %wu, arb %wu, acb %wu, nmod_t %wu\n", FLINT_VERSION,
                 sizeof(fmpz), sizeof(arb_struct), sizeof(acb_struct), sizeof(nmod_t));
    return 0;
}
