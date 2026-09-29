# Lane s1-slice1: linear systems modulo `N`, the first slice, end to end

`docs/workflow.md` rules 1 and 2: the smallest piece that a user can call and that computes something
true. The decisions S-D6 (`sol` is a report, written on `OK` and on `NO_SOLUTION`), S-D7 (Algorithm H in
the library's own code, the checker on every result), S-D8 (`ADF_LINSOLVE_DIM_MAX = 4096`), S-D9 (one
computation modulo `N`) are taken (`docs/SPEC.md` 15.3).

Read first: `lanes/COMMON-C.md` (rule 6 does not hold for `linsolve.h`, which is yours; rule 5 is replaced
by item 5 below); `docs/proofs/solvers.md` section 2, lines 495 to 840 (Definition 2.1 to Proposition 2.8)
and Proposition 2.11; `docs/api-s.md` sections 1 and 3; `proto/solvers_checks.py` (the reference of
Algorithm H, Algorithm L and the checker, and the functions `check_s1_*`); `include/adelefeld/resid.h` and
`src/resid.c` for the style of a slice.

**You own:** `include/adelefeld/linsolve.h` (new), the one include line in `include/adelefeld.h`,
`src/linsolve.c` (new), `tests/test_linsolve.c` (new; the pins of the layout go here, NOT into
`tests/test_abi.c`, which another lane is changing), `tests/ref/vectors/s1-slice1/` (new),
`tests/julia/linsolve.jl` (new), `tests/fuzz/diff_linsolve.py` (new), `lanes/s1-slice1/`. Everything else
is read-only.

## What is built

1. `include/adelefeld/linsolve.h`: the type `adf_linsol` of `docs/api-s.md` section 3 with `init`, `clear`,
   `is_canonical`, and
   - `adf_linsolve_mod(sol, A, b, N)`: Algorithm L of `solvers` 2.8 on Algorithm H of 2.5; statuses and
     outputs as the table of section 3 says, `LIMIT` decided before any allocation; every result passes
     the conditions of the checker before it is returned (note 3);
   - `adf_linsol_verify(sol, A, b, N)`: (K1) to (K5) and (K6) or (K7), by matrix products, no elimination;
   - `adf_linsol_kind`, `adf_linsol_kernel_rows`, `adf_linsol_get_kernel`, `adf_linsol_get_particular`,
     `adf_linsol_get_dual`.
   Nothing else of section 3 is declared in this slice.
2. `N` is any integer `>= 1`, of any size; no factorisation, no test of primality, no word-size path. Entries
   of `A` and `b` are any integers and are reduced. The sizes 0 (`r = 0`, `c = 0`), `N = 1` and the zero
   matrix are ordinary inputs (`solvers` P2.8(4), note 1 of section 3). No routine of FLINT for the Howell
   form is called in the library; `fmpz_mat_howell_form_mod` is used in one wrapper test only, labelled
   as such, with the padding that `solvers` P2.11(2) asks for.
3. Tests first, red then green (`lanes/s1-slice1/redgreen.log`), in `tests/test_linsolve.c`:
   - Against the enumeration of `(Z/N)^c`, written in the test itself: all systems modulo 4 with
     `r, c <= 2` (every `A`, every `b`); random systems for `N` in 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18
     with `r, c <= 3`. For each: the status; `x0 + S(G)` is exactly the set of solutions; `S(G)` is exactly
     the kernel; `adf_linsol_verify` accepts the result. Print the counts of `OK` and `NO_SOLUTION` and of
     the kernel sizes; fail if `OK`, `NO_SOLUTION`, a kernel of one element, or a kernel that is not a free
     module (for example `N = 4`, generator `(2)`) has count 0.
   - `G` is canonical: the same system with permuted equations, with equations multiplied by units, and
     with redundant equations added gives an identical `G`.
   - Vectors written by `lanes/s1-slice1/gen_vectors.py` from the reference of `proto/solvers_checks.py`
     (import, do not copy): the fixed cases of `check_s1_edge`; `N` of 64 to 122 bits and of 2000 bits with
     a planted solution; `G`, `x0`, `y` compared entry by entry with the reference (Algorithm L determines
     them).
   - The checker refuses false certificates: for results of the small grid, change one thing (a generator
     removed or changed, a row of `E` or `V` removed or changed, `x0` or `y` changed, the kind flipped, a
     generator multiplied by a unit, the certificate of another matrix) and require that
     `adf_linsol_verify` returns 0 or that the claim is still true by enumeration. Print how many changed
     certificates were refused and how many were true; fail if no certificate was refused for any one kind
     of change.
   - Statuses: `N < 1`, `b` of a wrong shape (`DOMAIN`), `r + c` above the limit (`LIMIT`, tested with a
     matrix of 4097 by 0 or 0 by 4097, which allocates nothing); `sol` untouched on both.
4. `tests/julia/linsolve.jl` in the style of `tests/julia/resid.jl`: one system with solutions, one without,
   read through the accessors. `tests/fuzz/diff_linsolve.py --seconds N --seed S`: random systems
   (`r, c <= 6`; `N` small, highly composite, prime, 64 bits, 300 bits; entries any size) to the C function
   through `ctypes` and to the reference: equal status, equal `G`, `x0`, `y`, and the C checker accepts.
   Run it for 180 seconds; that run is a smoke test.
5. Show that the tests bite, in a scratch copy under `build/`, one change at a time, with the test that
   fails recorded in `redgreen.log`: each of (K1) to (K7) removed from `adf_linsol_verify` (a checker
   without it must accept a false certificate of the test set); in Algorithm H, the step that adds the
   multiple `N / gcd` of a row (the Howell property) removed; the reduction of `x0` removed. No
   `make mutate`.
6. `make clean && make -j2 check`, the same with `SAN=1` and with `CC=clang`, `sh tests/test_exports.sh`
   pass; `julia --startup-file=no tests/julia/linsolve.jl build/libadelefeld.so` passes (see
   `tests/test_julia.sh` for the `LD_PRELOAD` of libgmp). Give the last line of each.

Not in this slice: `adf_linsolve_fball`, `adf_linsol_verify_fball`, `adf_linsol_get_fball`,
`adf_linsol_kernel_order`, `adf_linsol_get_image_cert`, `adf_linsol_contains`, `set`, `swap`, `identical`,
FLINT as an engine, benchmarks, the driver.

If a statement of `docs/proofs/solvers.md` section 2 is wrong or an algorithm of it cannot be implemented as
written, do not work around it silently: give the counterexample in the report.
