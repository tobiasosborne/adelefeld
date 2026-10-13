# Lane t5-slice4: report (slice 5d, the continuation and the completed function)

Status: steps A to F are done, end to end, for C = 1 and for the 17 golden characters, in both user calls.
Worktree `/home/user/adelefeld-wt/t5-slice4`, branch `lane/t5-slice4`, nothing committed. No HEADER-FINDING: both
declarations are implemented as the design writes them.

## Files

New: `tests/test_tate_continue.c`, `tests/ref/vectors/t5-slice4/continue.jsonl` (161 KB),
`tests/driver/tate-continue.cmd`, `tests/driver/tate-continue.out`, `tests/julia/tate_continue.jl`. Lane directory:
`gen_vectors.py`, `derive_fixture.py`, `faults.py`, `faults.out`, `resurvivors.py`, `resurvivors.out`, `mut.sh`,
`mutate.log` (8 KB), `redgreen.md`, this file.
Changed: `include/adelefeld/tate.h` (two declarations appended), `src/tate.c`, `tools/adf/adf.c` (two commands),
`tools/adf/README.md` (one section at the end), `tests/test_julia.sh` (one block after the `tate.jl` block),
`docs/api-5b.md` ("Slice 5d" appended). Build trees, scratch fault copies and the root `build/` are removed.

## What each step did

**A. Vectors.** `timeout 300 python3 -B lanes/t5-slice4/gen_vectors.py` (7 s) writes 300 records from
`proto/tate_checks.py`:
- 208 `value` records: C = 1 and the 17 golden characters at `1/2`, `1/2 + 14i`, `3/8 + 3i/4`, `-1`, `-2`, `-3`,
  `-7/2`, `2 + 3i`, `1 - 2^-40`, `2^-40`, `1 + 2^-10 i`, and the trivial zeros (even: 0, -2, -4; odd: -1, -3;
  C = 1: -2, -4). Each has I and Lambda at 60 digits (mpmath) and python-flint's value completed by hand. The
  generator asserts that the two sources agree to 1e-45 relative. At the 40 trivial zeros the mpmath value is the
  limit `2 (-1)^k L'(s0)/k!`, and the FLINT value is `W Lambda(1 - s0, conj chi)` with FLINT's L. Both are asserted
  finite and nonzero.
- 8 `ball` records: corners at radius `2^-60`, the zeta witness on `[-1/4, -1/8]`, and the pole-free rectangle
  `1 + 2^-40 +/- 2^-200` with mpmath and FLINT end values.
- 45 `pole` records: `pole_status` for exact dyadic balls, for C = 1, 5, 4.
- 6 `residue` records: `(s - 1) Lambda` at `1 + 2^-k` and `s Lambda` at `2^-k`, for k = 10, 20, 40.
- 27 `cutoff` records: N, R, the degrees and the bound `E_n + E_t + remainders`, at `1/2`, `3/8 + 3i/4`, `-3/2`
  for C = 1, 4, 5 and bits 20, 53, 80.
- 6 `side` records: each Mellin side separately (`numeric_piece`, R = infinity).

python-flint is 0.9.0 on FLINT 3.6.0; it is a reference only.

**B. Tests.** They are in a new binary, `tests/test_tate_continue.c`, so that the link-wrap stays out of
`test_tate`. They were written in full before the code; the red and green runs are in `redgreen.md`.
- The forbidden path. The test defines 23 FLINT symbols itself, each aborting: `acb_dirichlet` zeta/zeta_rs/jets,
  hurwitz, hurwitz_precomp_eval, eta, xi, the theta functions, root_number_theta, every `l_*`, and `acb_zeta`,
  `acb_hurwitz_zeta`, `arb_zeta`, `arb_zeta_ui`, `arb_hurwitz_zeta`. The library is linked statically, so these
  definitions interpose. With `-DADF_TATE_LINK_WRAP` and `-Wl,--wrap=...` the same bodies are the `__wrap_`
  definitions; that build was run as well (24261 checks). The section `forbidden` checks in forked children that
  four of the wrapped calls abort.
- Every value record, for both calls: OK, narrow, containing both sources. Bits are 53, plus 20 and 80 for
  C = 1, 4, 5, 16, plus 4 for C = 1. That is 788 OK calls and 1576 points.
- At the 40 trivial zeros, Lambda is finite and excludes 0, and I overlaps `C^-z Lambda`.
- continue = integral at `2, 3, 2 + 3i` for C = 1, 5, 16. Completed is narrow and overlaps `C^z I`.
- Hand values: `Lambda(1, chi_4) = 1`, `I(1, chi_4) = 1/4`, `Lambda(0, chi_4) = 1`, `Lambda_zeta(-1) = pi/6`.
- C > 1 at `s = 0, 1, -e, -e - 2` gives OK.
- The pole statuses against the oracle (45). Exact 0 and 1 give DOMAIN. `1 +/- 2^-10`, the contacts
  `[1 - 2^-9, 1]`, `[-2^-9, 0]` and `1 + [0, 2^-9] i` give NOT_DETERMINED, also at bits 2^21, so the geometry is
  decided before the work. `1 + 2^-10 i` gives OK. The T4 rectangle `1 + 2^-30 (1 + i) +/- 2^-110` gives OK. A
  rectangle that excludes 1 while its covering disk does not gives NOT_DETERMINED by the width.
- The residues: `(s - 1) Lambda -> 1` and `s Lambda -> -1`, within `2^(1-k)`.
- `1 + 2^-40 +/- 2^-200`: OK at bits 70, containing both FLINT end values.
- bits -1, 2^21 + 1 and WORD_MIN give DOMAIN after the precision cap and the C cap.
- N-D23: the radius `2^-60` gives NOT_DETERMINED at bits 80, with corner references more than `2^-79` apart. At
  bits 40 the result is OK and contains every corner. The witness gives NOT_DETERMINED at bits 0 and 20.
- The width shrinks at bits 20, 53, 80 for `s = 1/2, -3/2`, C = 1, 5, 16, and the same targets are met from
  prec 2.
- `z = s` aliasing, on success and on failure (DOMAIN, and a width failure).
- Independence. The hidden hook `adf_tate_trace` returns both sides. Each side contains its own reference. A
  sentinel in one side leaves the other side bit for bit equal and moves the sum.
- The oracle's cutoffs (27) and its bound on the radius (18 real points).
- The work cap: bits 2^21 and C = 1021 give LIMIT in 1.7 s. C = 1024 is refused by the charge within 0.5 s.
- INV: 4 aborts. The children clear their objects after the call. The precision cap comes before INV.

**C. Code.**
- `tt_run(z, chi, s, bits, prec, mode)` is the shared body of the three calls: the checks, the retries and the
  width check on the returned quantity. `tt_domain` is the gate of each mode. `tt_attempt` takes a flag for Lambda
  (the budget `2^-bits`, and no factor `C^-z`). `adf_tate_integral` is now `tt_run(..., TT_HALF)`, and its 22904
  checks are unchanged.
- The pole geometry is exact. For C = 1, `acb_is_exact` decides an exact pole; for a pole in a ball,
  `arb_contains_si(Re s, x0) && arb_contains_zero(Im s)` is the test on the closed rectangle. C > 1 has no pole
  test.
- `tt_recip` computes the two reciprocal terms by T4: `1/m` at the exact midpoint, plus `R/(D d)` on both radii.
  When the covering disk meets the pole, it falls back to `acb_inv`.
- The hidden hook `adf_tate_trace` (cutoffs, sides, sentinel) is not exported (`tests/test_exports.sh`: 622 of 622
  declared, 0 undeclared).
- Every source of slice 5d is cited at its use, by file and line.

**D. User calls.**
- `adf tate_continue CHI with S with BITS` prints `VALUE | continuation | s=S`. `adf tate_completed CHI with S with
  BITS` prints `VALUE | completed | s=S`.
- The fixture `tests/driver/tate-continue` has 16 lines, derived before the run by `derive_fixture.py`:
  `Lambda_zeta(1/2) = -3.976966226 +/- 5e-10`, zeta at -2, `chi_4` at -1, (5,4) at 0, C > 1 at s = 1, DOMAIN at 0
  and 1, three NOT_DETERMINED, DOMAIN for bits, LIMIT for C = 1031, and a wrong operand kind. The first run failed
  with PARSE; after the code, all lines were equal on the first run.
- `tests/julia/tate_continue.jl` makes both calls with the five arguments of `tate.jl`, 12 of 12. Against the HEAD
  library it fails at the missing symbol. It is registered in `tests/test_julia.sh`.

**E. Statements.** `docs/api-5b.md`, "Slice 5d":
- the statuses and the order of the checks;
- S9: the split is an identity on the whole plane. P13 step 3 (`analysis.md:602-612`) holds for Re(s) > 1. P15
  step 1 gives a locally uniform majorant, so both integrals are entire. The identity theorem then gives the
  continuation, and every bound of S4 to S6 of 5c holds for every real r;
- S10: the pole geometry;
- S11: T4 near a pole, with the width floor;
- S12: why the trivial zeros are finite;
- S13: the completed function computed directly;
- S14: independence and the link-wrap;
- decisions 10 to 16, the cost, and the "Check:" lines.

**F. Faults and mutation.** Below.

## Checks run (final code)

| Check | Result |
|---|---|
| `timeout 300 lanes/t5-slice4/build/test_tate_continue` (plain) | `tate_continue: 24261 checks`, 9.5 s |
| `test_tate` plain (regression) | `tate: 22904 checks` |
| INV=1 (`build-inv`) | test_tate_continue 24271 checks (4 aborts); test_tate 22914 |
| SAN=1, `ASAN_OPTIONS=detect_leaks=1` (`build-san`) | 24261 and 22904 checks, no sanitizer report |
| LeakSanitizer works here | a 77-byte leak test program is reported |
| CC=clang (`build-clang`) | 24261 and 22904 checks, no warning |
| `-DADF_TATE_LINK_WRAP` with 23 `-Wl,--wrap=` | 24261 checks |
| `sh tests/test_driver.sh` | 98 cases, 101666 lines, all equal |
| `sh tests/test_exports.sh` | passed, 622 of 622 exported, hooks hidden |
| `sh tests/test_julia.sh` | passed (with the libgmp `LD_PRELOAD`), `tate_continue.jl` 12 of 12 |
| `tools/memcheck/check_uninit.py` on the test, `src/tate.c`, `adf.c` | 0, 0, 0 findings |
| valgrind (memcheck flags, -O1), by section | see below |
| `timeout 120 python3 -B proto/tate_checks.py` | TOTAL 466 checks |

All builds used `make -j2 BUILD=lanes/t5-slice4/build...`. `check-all` was not run.

valgrind (`--error-exitcode=9 --track-origins=yes --leak-check=full --errors-for-leak-kinds=definite,indirect`,
each section under `timeout 180`) is clean on 7 of the 11 sections: `forbidden`, `overlap`, `balls`, `poles`,
`residues`, `cutoffs`, `sides`. The first run of `balls` and `poles` found uninitialised bytes in the test's own
sentinel byte comparison. The test was fixed, and both were run again clean. `statuses` and `widths` hit the 180 s
limit. `values` was not run, because it would take longer. `debug` does anything only under INV and was not run.
ASan and LSan cover the four.

## Fault table

Run with `timeout 1500 python3 -B lanes/t5-slice4/faults.py`, which needs `lanes/t5-slice4/build`. The output is
in `faults.out`. 12 of 12 faults are detected.

| Fault | Result |
|---|---|
| 53.1 wrong -a/s sign | detected `test_tate_continue.c:525` (`Lambda_zeta(-1) = pi/6`) |
| 53.2 zero mode retained (b/(s-1) twice) | detected `:576` (`1 + 2^-10 i`) |
| 53.3 dual s instead of 1 - s | detected `:514` (`Lambda(1, chi_4) = 1`) |
| 53.4 L14 factor 2 R^r omitted | detected `:248` (a value record) |
| 53.5 discarded n tail (E_n not added) | detected `:407` (the bound on the radius; added after the first run) |
| 53.6 input radius dropped | detected `:589` |
| `Gamma(z) L(s)` formed (rgamma then gamma) | detected `:534` (C > 1 at the local Gamma poles) |
| pole test on the open rectangle | detected `:565` (contact at bits 2^21; added after the first run) |
| `C^z` times a width-limited I | detected `:643` (the width) |
| dual exponent z instead of z' | detected `:514` |
| z written on NOT_DETERMINED | detected `:191` (sentinel bytes) |
| forbidden `acb_dirichlet_zeta` called | detected: abort in the wrapped symbol |

## Mutation testing

The command was `tools/mutate/mutate.py --files src/tate.c --limit 60 --seed 54 --jobs 2 --timeout 300 --make "sh
lanes/t5-slice4/mut.sh" --copy Makefile include src tests`. `mut.sh` compiles the mutated `tate.c` with
`-fsanitize=address,undefined -DADF_CHECK_INVARIANTS` and links it before a prebuilt SAN+INV archive. It then runs
`test_tate_continue` with `detect_leaks=1`.

Result: 60 mutants in 384 s. 38 were killed, 15 survived, 7 did not compile.

Two survivors were gaps, and tests were added for them. Both are now killed (`resurvivors.out`):
- 388 (`nz == 0` to `== 1`): zeta values at bits 4, where N = 1.
- 579 (the transform charge `C*C` to `C+C`): C = 1024 refused within 0.5 s.

The 13 remaining survivors, one line each:
- 375, 380, 386, 245, 250, 705: argument swaps in a commutative `arb_mul` or `arb_add`. Equivalent.
- 803: `&&` to `||` in the copy loop of the 5c hook `adf_tate_cutoffs`. It is not called by this program; the
  writes stay inside the 128-entry array.
- 100: the LIMIT of `adf_tate_vector`. It is not called by this program; `test_tate` checks it.
- 161: `tt_series` charges 0 instead of 1 per Lemma 6 step. The work total is observable only at the cap. This is a
  gap in the accounting tests (no exact work count), not in soundness.
- 247: the second term of L14's second branch uses `exp(-(R0 - R))` instead of `exp(-b R0)`. That bound is
  unsound, but no recorded cutoff changes: I checked with the oracle mutated the same way, for the characters
  (4,3), (5,2), (5,4) at `s = -3/2 ... -11/2` and bits 20, 53, 80. This is a test gap of L14, inherited from 5c
  (compare its survivor 551).
- 493: T4 replaced by direct division (`acb_get_mag_lower` dropped). Both are enclosures and differ only in width,
  in cases the tests do not reach.
- 636: the LIMIT of the R search dropped. It is unreachable in practice (5c's survivor 578); the written-output
  guard returns NOT_DETERMINED.
- 913: the canonical INV check of `tt_run` removed. The INV check of the dependency `adf_char_chi` aborts on the
  same inputs, so the children still abort.

Nothing was added to `tools/mutate/equivalent.txt`.

## Findings against the design, the oracle and the proofs

1. There is no counterexample to P12, P13, L14, P15, T4 or the design's statements. The oracle and the C code agree
   on all 27 continuation cutoff records and on the bound added to the radius. All 300 vector records pass.
2. The design's link-wrap (`api-5.md:353`, `-Wl,--wrap`) cannot be put on the test command line: the Makefile has
   no per-test linker flags, and it is not mine. The test interposes the symbols instead (decision 14). The
   `--wrap` form was run separately.
3. Citation drift in 5c: the comment of `tt_attempt` cited "P13 step 3 (:589-598)", but step 3 is at
   `analysis.md:602-612`. Corrected in the comment this lane rewrote. Other line citations of 5c were not
   rechecked.
4. A contract observation, not a defect (decision 13). A rectangle whose pole geometry is OK but whose image is
   wider than `2^-bits` returns the same NOT_DETERMINED as a rectangle meeting a pole. The test separates the two
   only through the order of the checks: at bits 2^21 the geometry answers before the work cap.
5. Tool finding: the `-O1 -Werror` build of `tools/memcheck/run.sh` fails on files this lane does not own. With
   this gcc it reports `src/lpow.c:259` (`maybe-uninitialized`) and `src/rfun.c:339,424` (`stringop-overread`). I
   built with those two warnings not treated as errors, and ran valgrind by section on `test_tate_continue` only.
6. The oracle's `pole_status` builds balls with python-flint radii (mag, 30 bits), which round up. A ball of
   radius `1/2 - 2^-40` around 1/2 becomes radius 1/2 and touches both poles. The C side rounds the same way, so
   the generator uses only dyadic radii that are exactly representable.

## Sources pending

- The residue `(-1)^k/k!` of Gamma at `-k`. It is used only by the generator for the mpmath reference at the
  trivial zeros, and that reference is cross-checked against the FLINT reference through the functional equation.
- The nonvanishing of `Lambda` at the trivial zeros (equivalently `L(1 - s0, conj chi) != 0`). The code does not
  use it; the tests check the 40 cases numerically.
- Inherited from `analysis.md:30-48`: the identity theorem and holomorphic parameter integration (S9).

## Not done, and costs noted

- valgrind on the sections `values`, `widths`, `statuses` (over 3 minutes under valgrind) and `debug`.
- No exact work-count test (survivor 161). No unit test of `tt_jbound`'s second branch (survivor 247).
- No fuzzing: none was asked for.
- Costs noted, as in 5c: each retry rebuilds the vector and the transforms and redoes the searches. The section
  `statuses` spends about 3 s on the work-cap cases.
