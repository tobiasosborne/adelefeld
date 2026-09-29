# Result of lane s1-slice1: linear systems modulo N, slice 1

## What was done

The first slice of S.1 runs end to end: a header, the library code, tests against an enumeration and against
the reference, a Julia call, and a differential script.

- `adf_linsol`, with `init`, `clear` and `is_canonical`. Its layout is fixed here and pinned in
  `tests/test_linsolve.c`: 192 bytes, alignment 8. The fields are `kind` at 0, `N` at 8, `r` at 16, `c` at 24,
  and `G`, `E`, `V`, `x0`, `y` at 32, 64, 96, 128, 160. `adf_sizeof_linsol` and `adf_alignof_linsol` are
  header-inline and exported.
- `adf_linsolve_mod`: Algorithm L (solvers 2.8) on Algorithm H (2.5), in the library's own code, using fmpz
  arithmetic modulo N for every N >= 1. It does not factor N, test primality, have a word-size path or call a
  FLINT Howell routine.
  - Statuses are decided in this order: `DOMAIN` (N < 1, or b not r by 1), then `LIMIT` (r + c > 4096), both
    before any allocation and with `sol` untouched; then `OK` or `NO_SOLUTION`, with `sol` written (S-D6).
  - Every result goes through `adf_linsol_verify` before it is returned. If the check refuses the result, the
    function calls `flint_abort`, because that would be a defect of the library.
- `adf_linsol_verify` checks (K1) to (K5), then (K6) or (K7), by matrix products and greedy reductions (no
  elimination). It refuses a wrong modulus, wrong sizes or shapes, and entries outside [0, N).
- Accessors: `adf_linsol_kind`, `adf_linsol_kernel_rows`, `adf_linsol_get_kernel`, `adf_linsol_get_particular`,
  `adf_linsol_get_dual`. Nothing else of api-s section 3 is declared.

## Files written

- `include/adelefeld/linsolve.h` (new).
- `include/adelefeld.h`: one include line.
- `src/linsolve.c` (new).
- `tests/test_linsolve.c` (new, 12 tests, with the layout pins).
- `tests/ref/vectors/s1-slice1/linsolve.jsonl`: 483 lines, written by `lanes/s1-slice1/gen_vectors.py`.
- `tests/julia/linsolve.jl` (new).
- `tests/fuzz/diff_linsolve.py` (new).
- `lanes/s1-slice1/`:
  - `redgreen.log`, `mutants.py`, `probe_third_case.py`, `gen_vectors.py`;
  - logs: `check-plain.log`, `check-san.log`, `check-clang.log`, `exports.log`, `julia.log`, `fuzz180.out`;
  - `result.md` (this file).

## Checks

**Red, then green** (`redgreen.log`):
- Red 1: link error.
- Red 2 (stubs): 12 of 12 tests failed by assertions, 13890 failed checks.
- Green: 12 tests, 14415 checks, 0 failed.

On the way, one test was wrong: its planted x depended on the row index. The library correctly returned
`NO_SOLUTION`, with a y that (K7) accepts. The test was repaired.

**`./build/test_linsolve`.** A case fails when any of these is violated: the status; x0 + S(G) equal, as a set,
to the enumerated solutions; S(G) equal to the enumerated kernel; G equal entry by entry to the Howell form found
by search in the kernel (`howell_brute`, written again in the test); `adf_linsol_verify` = 1; `is_canonical`.
- All systems modulo 4 with r, c <= 2, the sizes 0 included: 4455 systems. OK 2569, NO_SOLUTION 1886. Kernel of
  one element 1757, not free 1456. 0 failures.
- Random systems, N in {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18}, r, c <= 3: 7088 systems. OK 4749,
  NO_SOLUTION 2339. Kernel of one element 3006, not free 1159. 0 failures.
- The test fails if any of these four counts is 0. It prints a histogram of kernel sizes.
- One equation a z = b, every N < 40 and every a, b: 20540 cases. The status must agree with gcd(a, N) | b and
  G with (N/gcd); 0 failures.
- Canonical form: 440 systems. The equations were permuted, multiplied by units, and given a redundant
  combination and a zero equation, with N up to 10^30. 111 have no solution. G or the status differed in 0.
- Vectors: all 483 lines. Status, G, E, V, x0 and y are compared entry by entry with `linsolve_mod`; 0 differ.
  - The lines include the 11 fixed cases of `check_s1_edge`.
  - 123 lines have N above 64 bits: 64 to 122 bits and 2000 bits, with planted solutions.
  - Entries go up to thousands of bits, of both signs.
- Changed certificates, 13 kinds of change, 37 080 tries. None was accepted while its claim is false by
  enumeration. Every kind had some refused. The kinds are:
  - a generator removed, changed, or multiplied by a unit;
  - a row of E, or of V, removed or changed;
  - x0 or y changed;
  - the kind flipped;
  - the certificate of another matrix, and of A with one row replaced;
  - a row of E repeated with a generator removed;
  - a generator plus a multiple of a later one.
- Also passed:
  - wrong shapes, moduli, kinds and entries are refused;
  - `DOMAIN` for N = 0, N = -12 and four wrong shapes of b; `LIMIT` for 4097 by 0 and 0 by 4097, with `sol`
    untouched on both (deep comparison); r + c = 4096 is accepted;
  - N = 1, r = 0, c = 0, the zero matrix, and a planted system with entries of about 3000 bits.
- Wrapper test, labelled as such: `fmpz_mat_howell_form_mod` on [A^T | I_c] padded with r zero rows. 270 of 270
  outputs are identical to E, V, G.

**Do the tests bite (item 5)?** `python3 lanes/s1-slice1/mutants.py`, one change at a time in a scratch copy
under `build/mutants/`.
- Each of (K1) to (K7) disabled in `adf_linsol_verify`: `checker_refuses_false_certificates` fails every time.
  Changed certificates accepted and false, per condition: K1 947, K2 1907, K3 1216, K4 2603, K5 1314, K6 2738,
  K7 2212.
- Algorithm H without the pending pair of the first case: the internal check aborts. With that check disabled,
  10 of 12 tests fail (3724 checks).
- The reduction of x0 modulo N removed: the internal check aborts. With the check disabled, 7 tests fail (2791
  checks).
- Algorithm H without the pending pair of the third case: this change survives. It is equivalent; see the
  findings.

**Build and run checks:**
- `make clean && make -j2 check`: `check passed: all 44 test programs`.
- The same with `SAN=1`: `check passed: all 44 test programs`, with no sanitizer report.
- The same with `CC=clang`: `check passed: all 44 test programs`.
- `sh tests/test_exports.sh`: `test_exports: passed: 217 of 217 declared functions are exported, 0 are not
  implemented yet, no exported name is undeclared, no variadic function`.
- `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 julia --startup-file=no tests/julia/linsolve.jl
  build/libadelefeld.so`: `adf_linsolve_mod through ccall | 15 15`.
  - Without the LD_PRELOAD it fails with the known quirk: `undefined symbol: __gmpn_modexact_1_odd`
    (`tests/test_julia.sh`).
  - It covers one system with solutions (the solution set is enumerated in Julia), one without, a 200-bit
    modulus, and `DOMAIN`.

**Fuzzing:** `python3 tests/fuzz/diff_linsolve.py --seconds 180 --seed 1`.
- Result: 413 816 systems, 0 disagreements. There are about 83 000 of each kind of modulus: small,
  composite, prime, 64-bit, 300-bit. OK 267 926, NO_SOLUTION 145 890.
- It asserts that the status, G, E, V, x0 and y are equal to the reference, that the accessors agree, and that
  both checkers accept.
- **This run was a smoke test, not a long fuzzing run.** A 5 s run against a library built with the x0-mod
  change (check disabled) failed at case 2, so the script does detect a wrong result.

## Not done

- Not in this slice: `adf_linsolve_fball`, `_verify_fball`, `_get_fball`, `kernel_order`, `get_image_cert`,
  `contains`, `set`, `swap`, `identical`, FLINT as an engine, benchmarks, the driver.
- No long fuzzing run.
- `INV=1` was not run.
- The case with r + c at the limit and both r and c large (about 400 MB) is not tested.

## Header findings (`linsolve.h` is owned by this lane)

- The init value has x0 of shape 0 by 1, read as "c by 1 when present, with c = 0". "No rows anywhere" in
  api-s holds.
- Behaviour when the internal check refuses a result is not in api-s. The function calls `flint_abort` with a
  message; the header states this.

## Findings against the specification and `docs/proofs/solvers.md`

- **Algorithm H has an unneeded step (not an error).** In the third case of Algorithm H
  (solvers.md:635 to 637), adding the pair ((N/g) w', j + 1) is implied by what is already there.
  - Proof: (N/h) w and v' lie in the span U of the later rows and pending vectors. Then (N/g) v = (N/h) v' +
    (a/g)(N/h) w and (N/g) w = (h/g)(N/h) w, so (N/g) w' = s (N/g) v + t (N/g) w is in U.
  - Probe (`probe_third_case.py`): 34 000 row sets, with the third case taken 60 577 times; 0 outputs
    differ.
  - The proof of 2.5 stays valid. The cost bound 2.5(4) counts these pairs.
- **The fixed cases of `check_s1_edge` are copied.** They are local to that function, so `gen_vectors.py`
  copies them, with lines cited; the functions are imported.

## Avoidable costs (not optimised)

- The internal check reduces A and b modulo N again.
- (K4) forms N^c.
- The `NO_SOLUTION` path runs Algorithm H in full on [A | I_r], and discards E2 and V2, when only one y is
  needed.
