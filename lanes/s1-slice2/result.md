# Result of lane s1-slice2: the rest of linsolve.h (S.1 complete)

## What was done

Nine functions added to `include/adelefeld/linsolve.h` and `src/linsolve.c`, each with its test first.
`tests/test_linsolve.c` is unchanged and passes.

- `adf_linsol_set`, `adf_linsol_swap`, `adf_linsol_identical` (kind, N, r, c and every shape and entry of
  G, E, V, x0, y).
- `adf_linsol_kernel_order` (product of N/g_i, g_i the pivot of row i of G; solvers Lemma 2.3(3), line 551),
  `adf_linsol_get_image_cert`, `adf_linsol_contains` (x - x0 reduced by G with the greedy reduction, Lemma
  2.3(2); 0 for kind EMPTY or a shape of x other than c by 1).
- `adf_linsolve_fball`, `adf_linsol_verify_fball` (solvers P2.9, line 841): the system (A', b', N) is built
  from the canonical triples of the balls (`adf_fball_get_fmpz3`, so local and global balls are treated
  alike), N = lcm(H_i), then `adf_linsolve_mod`. Order of statuses: DOMAIN (r not the number of rows of A),
  UNSUPPORTED (a ball with H = 0), then those of `adf_linsolve_mod` (OK, NO_SOLUTION, LIMIT). `sol` is
  untouched on DOMAIN, UNSUPPORTED and LIMIT. `verify_fball` returns 0 for wrong r or an exact ball.
- `adf_linsol_get_fball` (P2.10, line 883): the ball x0[j] + rho_j Zhat, rho_j = gcd(N, G[.][j]), stored
  canonical global. DOMAIN for j outside [0, c) is decided before NO_SOLUTION for kind EMPTY (x untouched
  on both).
- `linsolve.h` now includes `adelefeld/fball.h`.

## Files written

- `include/adelefeld/linsolve.h`, `src/linsolve.c` (additions only).
- `tests/test_linsolve_rest.c` (new, 13 tests).
- `tests/ref/vectors/s1-slice2/fball.jsonl` (780 lines), from `lanes/s1-slice2/gen_vectors.py`.
- `tests/julia/linsolve.jl`: a second testset appended (slice 1's testset is unchanged).
- `lanes/s1-slice2/`: `gen_vectors.py`, `mutate.py`, `mutants.out`, `redgreen.log`, `red2.out`, `green1.out`,
  `green2.out`, `check-plain.log`, `check-san.log`, `check-clang.log`, `exports.log`, `julia.log`, `result.md`.

## Checks

**Red, then green** (`redgreen.log`).
- Red 1: link error, nine undefined references.
- Red 2 (stubs returning 0, UNSUPPORTED, untouched): 13 tests, 30444 checks, 20327 failed checks, 13 failed
  tests, all assertions.
- Green 1 (real code): 39 failed checks in 2 tests. Both were defects of my tests, not of the library: a
  `char coord[64]` buffer for N up to 360, and `fmpz_randbits` giving negative H and d (refused by
  `set_fmpz3`, DOMAIN). Repaired in the test.
- Green 2: 13 tests, 30328 checks, 0 failed checks. The count differs from Red 2 because the test of the
  local/global backends skips systems by a random draw that changed with the added mixed arrays.

**`./build/test_linsolve_rest`.** A check fails when a status, a matrix or a ball differs from the oracle.
- Grid modulo 4, r, c <= 2, sizes 0 included: 4455 systems.
  - set/swap/identical: 0 failures. Permuted equations: identical in some cases and not in others (both
    counts > 0, asserted), and `identical` equals the field-by-field comparison in every case.
  - `kernel_order` equals the enumerated size of the kernel (also for kind EMPTY) and, for COSET, the
    number of solutions. `contains` agrees with the enumeration on all 4^c points and on x + N z (25533
    shifted points; yes 4455, no 63234). `get_image_cert` copies, including the alias of the destination
    to `sol->E`, `sol->V`. 0 bad.
- Random systems, the twelve small N, r, c <= 3: 1800 systems, `contains` yes 171290, no 405431, kernel order
  > 1 in 1007. 0 bad.
- 28 single changes of a value (each field, shapes): `identical` sees all 28.
- Balls, membership of A x by exact integer arithmetic on the raw triple (d val - a divisible by H) on x in
  [0, N)^c and on x + N z: 2815 systems (185 skipped for N^c > 6000), OK 1591, NO_SOLUTION 1224.
  N below the product of the H_i in 807; 2651 raw triples not canonical; d > 1 and different H_i occur. x0 + S(G)
  equals the enumerated set; `sol->N` equals the lcm of the canonical radii; `verify_fball` accepts the result
  and, for every changed x0, agrees with `contains`; 0 bad.
- Coordinate balls: 2774 on the fball systems, and 6604 on 5655 small systems (the grid and the random
  moduli); the residues of the ball equal the residues of the j-th coordinates of all solutions. 0 bad.
- Local and global balls (context blocks 16, 27, 25, 7): 544 systems, 358 with a local ball, 52 with mixed
  arrays; `identical` in all, `verify_fball` cross-checked. 0 bad.
- Statuses with `sol` compared to a prior value: DOMAIN for r = 3, 1, -1, 0 against 2 rows, UNSUPPORTED for
  an exact ball in either position, DOMAIN before UNSUPPORTED, LIMIT for 4097 rows (c = 0), DOMAIN before
  LIMIT; `get_fball`: DOMAIN for j = -1, 1, 1000, NO_SOLUTION on EMPTY, x untouched (`identical` to the
  marker), local x becomes global.
- Large operands: 3 systems with entries of 3000 bits, radii of 1700 to 2000+ bits, denominators of 40 bits,
  a planted x of 200 bits; x and x + N z (z of 3000 bits) are in the coset and in every ball (exact
  criterion), and in every coordinate ball. 0 failures.
- Vectors: all 780 lines (OK 391, NO_SOLUTION 389, N above 64 bits 104, 759 coordinate balls). Status, N, G,
  E, V, x0, y and every coordinate ball are compared entry by entry with the reference; 0 differ.

**The tests bite** (`lanes/s1-slice2/mutate.py`, scratch copy `build/scratch`, one change at a time; failed
checks, failed tests of 13):

| change | failed checks | failed tests |
|---|---|---|
| `lcm` replaced by the product of the H_i | 1159 | 3 (membership, statuses, vectors) |
| denominator d of a ball ignored | 771 | 3 (membership, large operands, vectors) |
| rho_j from the G column only (initial gcd value 0, N only if the result is 0) | 232 | 3 (membership, get_fball grid, vectors) |
| `contains` without the greedy reduction | 2094 | 4 |
| `kernel_order` with g_i for N/g_i | 2137 | 3 |

The rho mutant is my reading of "without the gcd with N": the gcd runs over the column of G only.
The unmutated scratch copy passes: 30328 checks, 0 failed.

**Full runs**, from `make clean` each time (the last line):
- `make -j2 check`: `check passed: all 50 test programs`.
- `make -j2 check SAN=1`: `check passed: all 50 test programs`. `test_linsolve_rest`: 13 tests, 30328 checks, 0 failed.
- `make -j2 check CC=clang`: `check passed: all 50 test programs`.
- `sh tests/test_exports.sh`: `passed: 227 of 227 declared functions are exported, 0 are not implemented yet,
  no exported name is undeclared, no variadic function`.
- `sh tests/test_julia.sh`: `test_julia: passed (with LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10)`;
  slice 1's testset 15 of 15, the new testset 14 of 14.

## What is not done

- No `make mutate` (replaced by item 4).
- No long fuzz run; nothing here is a fuzz result.
- The Julia call uses small balls only: the ball array holds the structs by value, copied byte by byte; a ball
  with a big fmpz (a pointer) would need its owner alive, and none is used.
- No benchmark, driver, text or dump forms (not in the lane).
- Local balls are tested against one context (K = 75600); a ball whose canonical triple has H not dividing K
  stays global in the mixed arrays.

## Header findings

- `docs/api-s.md` does not say the order of DOMAIN and NO_SOLUTION in `adf_linsol_get_fball`. Implemented and
  documented: DOMAIN first, as `adf_linsolve_mod` decides argument errors first.
- `docs/api-s.md` does not list LIMIT for `adf_linsolve_fball`. It comes from `adf_linsolve_mod` (r + c >
  4096) and leaves `sol` untouched; the header says so.
- `adf_linsol_get_image_cert(E, V, sol)`: E and V must be different objects (stated in the header).
- Avoidable cost: `adf_linsolve_fball` builds A' and b' in full before the LIMIT decision of
  `adf_linsolve_mod`; harmless, the sizes are those of A.

## Findings against the specification or `docs/proofs/solvers.md`

None. P2.9 and P2.10 hold on every system tested against the exact enumeration.
