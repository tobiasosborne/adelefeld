# Lane s1-slice2: the rest of `linsolve.h` (S.1 complete)

Slice 1 is on master (`include/adelefeld/linsolve.h`, `src/linsolve.c`, `tests/test_linsolve.c`,
`lanes/s1-slice1/result.md`): `adf_linsolve_mod`, `adf_linsol_verify`, five accessors. This lane adds the
remaining functions of `docs/api-s.md` section 3. Do not change what the existing functions return;
`tests/test_linsolve.c` must pass unchanged.

Read first: `lanes/COMMON-C.md` (rule 6 does not hold for `linsolve.h`; rule 5 is replaced by item 4
below); `docs/api-s.md` section 3 with all notes; `docs/proofs/solvers.md` Lemma 2.3, Propositions 2.6,
2.9, 2.10; `proto/solvers_checks.py` (`check_s1_adelic` and the reference functions it uses);
`include/adelefeld/fball.h` (the canonical triple, the two backends), `modctx.h`.

**You own:** `include/adelefeld/linsolve.h` and `src/linsolve.c` (additions), `tests/test_linsolve_rest.c`
(new), `tests/ref/vectors/s1-slice2/` (new), `tests/julia/linsolve.jl` (additions),
`lanes/s1-slice2/`. Everything else is read-only.

## What is built, each function with its test first (red, then green, `lanes/s1-slice2/redgreen.log`)

1. `adf_linsol_set`, `adf_linsol_swap`, `adf_linsol_identical`; `adf_linsol_kernel_order`,
   `adf_linsol_get_image_cert`, `adf_linsol_contains`. Tests on the grid of slice 1 (all systems modulo 4
   with `r, c <= 2`; random systems for the twelve small `N`, `r, c <= 3`): `kernel_order` equals the
   number of kernel elements counted by enumeration; `contains` agrees with the enumeration for every `x`
   of `(Z/N)^c`, and for `x + N z`; `identical` holds for two runs on the same system and for permuted
   equations exactly when all of `G`, `E`, `V`, `x0`, `y` agree.
2. `adf_linsolve_fball`, `adf_linsol_verify_fball` (`solvers` P2.9): membership of `A x` in the balls
   tested by exact rational arithmetic on the integer points `x0 + G-combinations` and on `x + N z`; points
   outside the coset are outside at least one ball; an exact ball (`H = 0`) gives `UNSUPPORTED`, `r` not the
   number of rows gives `DOMAIN`, `sol` untouched on both; right-hand sides in the local backend and the
   same balls in the global backend give identical results; balls with denominators `d > 1` and with
   different `H_i` (`N = lcm`).
3. `adf_linsol_get_fball` (`solvers` P2.10): the ball of coordinate `j` is `x0[j] + rho_j Zhat`; on the
   small grid the set of `j`-th coordinates of all solutions modulo `N` equals the residues of the ball;
   `NO_SOLUTION` on kind EMPTY, `DOMAIN` for `j` outside `[0, c)`, `x` untouched.
4. Show that the tests bite, in a scratch copy under `build/`, one change at a time, recorded in
   `redgreen.log`: `lcm` replaced by the product of the `H_i`; the denominator `d` of a ball ignored;
   `rho_j` without the `gcd` with `N`; `contains` without the reduction by `G`; `kernel_order` with
   `g_i` in place of `N/g_i`. No `make mutate`.
5. Vectors from the reference (`lanes/s1-slice2/gen_vectors.py`, import, do not copy); two calls added to
   `tests/julia/linsolve.jl` (`adf_linsolve_fball`, `adf_linsol_get_fball`).
6. `make clean && make -j2 check`, the same with `SAN=1` and with `CC=clang`, `sh tests/test_exports.sh`,
   `sh tests/test_julia.sh` pass. Give the last line of each.

Not in this lane: FLINT as an engine, benchmarks, the driver, text and dump forms.
