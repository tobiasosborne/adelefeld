# Lane t5-repair1: the test gaps of slices 5a to 5d (result)

All five items are done. The two L14 survivors and the work-count survivor of `src/tate.c` now fail tests. In
`src/localfactor.c` the new test kills 7 of the 11 mutants that survived `test_localfactor`. Of the 4 left, 250 is
sound (it only loosens a guard); 247, 252 (drop) and 344 have neither a proof nor a witness of a wrong result. There
is a MAJOR finding against the tests: mutant 217 gives a wrong enclosure at `s = 2 + i(1/2 +/- 1/8)`, and before
this lane it passed all three test programs. No defect of `src/` or `include/` was found, and neither was changed.
One existing test function (`test_tate_gamma.c`) got a one-line repair of its own byte comparison, which valgrind
had flagged.

Files written. Tests: `tests/test_tate_continue.c` (section `jbound`), `tests/test_tate.c` (`work_count`),
`tests/test_localfactor.c` (`real_derivative_bound_boxes`), `tests/test_tate_gamma.c` (one line and a comment).
Vectors: `tests/ref/vectors/t5-repair1/jbound.jsonl` (20 records, 6610 bytes), `gamma_bound.jsonl` (8 records, 33787
bytes). Lane: `gen_vectors.py`, `gen_lf_vectors.py`, `replant.py`, `probe.py`, `probe_lf.c`, `vg.sh`,
`redgreen.md`, logs.

## Item 1: L14's second branch (`tt_jbound`)

Added: section `jbound` of `test_tate_continue` and 20 records written by `lanes/t5-repair1/gen_vectors.py`. It is
my own mpmath evaluation of one attempt, 60 digits, built from analysis.md L6 :215-221, L14 :630-639, P15 :652-661,
api-5.md :196-220 and api-5b.md decision 5. Neither the C code nor the proto oracle is imported. It covers zeta
(C = 1, mode Lambda) at s = 41/2, 9, -39/2, -8, 2 and bits 4, 20, 53, 80. For each record the test calls
`adf_tate_trace` with sentinel bit 1, which makes the forward side exactly 10^6 before the error is added. So the
radii of that side are exactly the bound E_n + E_t + both quadrature remainders. The test requires:

- N, R, K, every panel degree and the work to equal the generator's;
- both radii to lie within 10^-6 relative of the generator's bound.

The generator asserts that each comparison it decides has a relative margin of at least 10^-9.

Why the second branch is taken (the proof is in the comment of the test): b = lower(pi) < 3.15. At s = 41/2,
r1 = 37/4, so 2 rplus = 37/2 > b R for R = 1 (every N step) and for R = 2, 4. At s = -39/2 the same holds for r2,
and at s = 9 and -8 (r = 7/2) for R = 1, 2. The generator records the branch of every step: 16 records end the
N search in the second branch.

- Generator: `timeout 300 python3 -B lanes/t5-repair1/gen_vectors.py`. 20 records. 10 of them tell m247 apart, 13
  tell m551 apart (by N, R, a degree, the work, or the bound).
- `test_tate_continue jbound`: 2141 checks. The whole plain program gives 25800 checks (24261 before).
- Replants (`timeout 900 python3 -B lanes/t5-repair1/replant.py ...`, log `replant-final.log`):
  - m247 (src/tate.c:247 `arb_mul(u, bb, R0, TT_TP);` removed, so exp(-(R0 - R))): exit 1 at
    test_tate_continue.c:693 (N, R). Before this lane it passed the sections `cutoffs` and `values` (exit 0).
  - m551 (src/tate.c:623, the upper bound of Re(z') dropped, so r2 = -1): exit 1 at :707 (the radius bound). This
    mutant was already dead in the merged tree: slice 5d's `cutoffs` fail at :407 and `values` at :248 (a value
    outside its enclosure). It had survived only the 5c program `test_tate`.

## Item 2: the work count

Added: `work_count` in `test_tate`. It checks zeta at s = 2, bits 4 through `adf_tate_cutoffs`: work = 43, with
N = 1, R = 2, K = 1 and J = 13, 15. The comment works the count out by decision 5:

    1 + (1 + 2 - 1 + 1) + 4 + 1 + 2 + 2 + 30 = 43

It also checks that the same identity, work = 15 + J_fwd + J_dual, holds at s = 2 + 2^-10 i. The generator of item
1 gives the same 43, and every jbound record also pins the work.

- `test_tate`: 22909 checks (22904 before).
- m161 (src/tate.c:161 `tt_charge(w, 1)` -> `tt_charge(w, 0)`): exit 1 at test_tate.c:675 (`work == 43`). In
  `test_tate_continue jbound` it fails at :699.

## Item 3: the inherited Gamma certificate (`real_derivative_bound`, `real_factor`)

I regenerated the mutants with `tools/mutate/mutate.py --files src/localfactor.c --list`, using seed 53001 (limit
60) and seeds 51001 and 51002 (limit 20). I replanted each one against `test_localfactor` before and after the new
test (`replant-lf.log`, `replant-final.log`). Search: `lanes/t5-repair1/probe.py` with `probe_lf.c`. It ran 3589
boxes at precisions 24, 53 and 120, which is 10767 calls, 10491 of them OK. Each OK result was compared with 25
grid values at 320 bits. These values come from FLINT and are used only to find candidates; the boxes that went
into the test use mpmath references. Logs: `probe2.log`, `probe3.log`.

Added: `real_derivative_bound_boxes` in `test_localfactor`. It reads 8 boxes from `gamma_bound.jsonl`
(`gen_lf_vectors.py`, mpmath, no FLINT Gamma), each at precisions 24, 53 and 120.

- Enclosure: all 25 grid samples must lie in the result, which gives 600 checks.
- Width, on 4 of the boxes: in each component the result must lie in ym +/- R Bsmall, within 10^-5 relative and the
  final rounding (12 checks). Here ym is the mpmath value at the midpoint, and Bsmall comes from the Y16 statement
  (api-1f9.md:476-483). This is Y16 step 7, the midpoint value enlarged by R B. On these boxes the candidate is
  wider than that ball, so the refinement decides the result.

`test_localfactor`: 14 tests and 309581 checks (13 tests and 306730 before).

| Mutant (line, change) | Kind | Before (test_localfactor) | After |
|---|---|---|---|
| 217 drop `mag_inv(inva, f)` | weakens: c = 1, M1 = 1 < J(1) | survived (all 3 programs) | killed :1254, box 0 |
| 221 `(cb - 1)!` -> `cb!` | looser (sound) | survived | killed :1267, 4.33 > R Bsmall 1.80 |
| 222 swap | equivalent | - | - |
| 223 `mag_mul(m1, inva, inva)` -> `mag_add` | looser: 2/A >= 1/A^2 | survived | killed :1267, 0.0597 > 0.0398 |
| 237 drop `mag_sqrt_lower` | weakens if delta_j > 1 | killed :878 (real_poles) | killed |
| 240 swap | equivalent | - | - |
| 244 drop `mag_set(oldsum, sum)` | weakens the guard | killed :870 | killed |
| 247 `mag_mul(oldsum, oldsum, m0)` -> `mag_add` | weakens the guard | survived | survived: open |
| 248 `mag_add(oldsum, .., m1)` -> `mag_sub` | weakens the guard | survived (test_tate_local kills) | killed :1254 |
| 250 guard not halved | looser guard, sound | survived | survived: sound |
| 252 swap | equivalent | - | - |
| 252 drop `mag_mul(sum, sum, m0)` | weakens Bsmall (no M0) | survived | survived: open |
| 253 swap | equivalent | - | - |
| 255 Bsmall not halved | looser | survived | killed :1267, 0.0796 > 0.0398 |
| 262 swap | equivalent | - | - |
| 344 `mag_mul(oldB, oldB, R)` -> `mag_add` | guard, weaker only if R > 1 | survived | survived: open |
| 344 swap | equivalent | - | - |
| 367 drop `acb_set_round(previous, ...)` | midpoint above prec | killed :94 (midpoint bits) | killed |

Three controls that are not on the brief's list behave the same way. 225 (drop `mag_add(m1, m1, f)`) and 253 (drop
`mag_add(sum, sum, m1)`) both survived `test_localfactor` before; `test_tate_local` killed them, and now
`test_localfactor` does too (:1254). 238 (drop `mag_mul_lower(P, P, d)`) was already killed by `test_localfactor`.
Several mutants on the brief's list (237, 244, 367 and the 222 drop) survived only because lane t5-slice3's run was
limited to `test_tate_gamma`; `test_localfactor` kills them.

Equivalence sentences (one each; `tools/mutate/equivalent.txt` is not mine and is unchanged):

- 222, 240, 252, 253, 262, 344 (argument swaps): `mag_add` and `mag_mul` are commutative and FLINT lets their output
  alias an input, so exchanging the two inputs gives the same mag.
- 250 (not an equivalence, but sound for the contract): doubling `oldsum` only enlarges the previous ball. That ball
  is Z6's enclosure with a bound twice as large, so it still encloses. The returned ball is either the new ball
  (sound by Y16) or the previous one. It changed one result in 10491 OK calls, by a radius ratio of 1.000 at
  `11.4375 + 1.1875i +/- (1/2, 1)`, prec 24.

Open, with partial proofs:

- 252 drop (Bsmall without M0): for n = 0 the sum is 0, so the mutant is identical to the original. When
  1 <= Re(z+n) <= 2 on the whole box it is sound, because |Gamma(z+n)| <= 1 (the argument of
  `lanes/f-repair9/f5-analysis.md` case 2). For n > 0 with max Re(z+n) > 2 it is unresolved: 1561 narrower results
  and 0 missed samples. This is the F5 question of f-repair9 for Bsmall.
- 247: the guard's bound becomes pi^(-lo) [Sum + log pi + M0 + M1]/(2P), which is at least f-repair9's B5. So the
  mutant can be wrong only where B5 is, and F5 is open. 459 narrower results and 0 missed samples.
- 344: oldB + R is at least oldB R when R <= 1 or oldB <= R/(R - 1), so it can only be weaker on boxes with R > 1.
  48 narrower results and 0 missed samples, including 84 boxes of radius 1, 2 or 4 (252 calls).

## Item 4: valgrind

Built with `make -j2 BUILD=lanes/t5-repair1/build-vg CFLAGS='-std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror
-Wno-error=maybe-uninitialized -Wno-error=stringop-overread'`. Each run used `timeout 900 valgrind
--error-exitcode=9 --track-origins=yes --leak-check=full --errors-for-leak-kinds=definite,indirect`
(valgrind 3.22.0; `lanes/t5-repair1/vg.sh`, `vg-summary.txt`, `vg-*.log`).

| Run | Exit | Time | Errors | Lost (definite/indirect) |
|---|---|---|---|---|
| test_tate_continue values | 0 | 868 s | 0 from 0 contexts | 0 / 0, 0 bytes in use |
| test_tate_continue widths | 0 | 346 s | 0 | 0 / 0, 0 bytes in use |
| test_tate_continue statuses | 1 | 298 s | 0 | 0 / 0, 65544 bytes still reachable (exit from CHECK) |
| statuses, scratch copy without the timing checks :611, :617 | 0 | 758 s | 0 | 0 / 0, 0 bytes in use |
| test_tate_continue jbound (new) | 0 | 5 s | 0 | 0 / 0 |
| test_tate_gamma, as merged | 9 | 99 s | 18 from 6 contexts | 0 / 0, all heap freed |
| test_tate_gamma, after the test repair | 0 | 149 s | 0 | 0 / 0 |

- statuses: exit 1 comes from the test's own time limit, `seconds() - t0 < 10` at test_tate_continue.c:611 (the
  work cap took 284.67 s under valgrind). It is not a memory error. The run stopped before the completed-function
  half, so I ran a scratch copy with only the two timing CHECKs replaced. That copy completed with 0 errors.
- test_tate_gamma: all 18 errors were "conditional jump depends on uninitialised value" in `memcmp` at
  test_tate_gamma.c:126, the byte comparison of `call()`. They came from the six failing calls at :543-553 of
  `statuses_precision_and_aliases` (f = epsilon, ram = 1). The origin is the stack temporary of
  `adf_local_epsilon_at` (localfactor.c:809).
  - Cause: :567 calls `fns[f](s, NULL, s, ...)`, which swaps the function's temporary into `s`, together with its
    unwritten inactive mantissa limbs. In the next iteration `arf_set` copies the whole limb union into the test's
    copy, and `memcmp` then reads those limbs.
  - This is a defect of the test, not of the library: the library writes z only on OK.
  - Repair: `acb_clear(s); initialized_acb(s);` at the end of that loop body. This adds a line and removes no
    check. After it: 0 errors; plain, SAN, INV and clang all green.

## Item 5: the Julia fixture

- Shared library: built `-fPIC` (`make -s -j1 BUILD=lanes/t5-repair1/build-shared CFLAGS='... -fPIC' all`), then
  `cc -shared` with `--whole-archive`.
- Red: in a scratch copy `jl/tate_gamma_wrong.jl` the expected -0.75 becomes -0.7. With
  `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 timeout 300 julia --startup-file=no ...` it exits 1: 35 pass,
  1 fail (`contains(z, ref)` at its line 56; `jl/red.log`).
- Green: the original exits 0, 36 of 36 (`jl/green.log`).
- The scratch copy is deleted, and the fixture was never changed.

## Checks at the end

Every program was run under timeout, with at most 2 jobs. All runs exited 0 with 0 failed checks.

| Build (lanes/t5-repair1/build-*) | test_tate | test_tate_continue | test_localfactor | test_tate_gamma |
|---|---|---|---|---|
| plain | 22909 | 25800 | 14 / 309581 | 8 / 47955 |
| SAN=1, ASAN_OPTIONS=detect_leaks=1 | 22909 | 25800 | 14 / 309581 | 8 / 47955 |
| INV=1 | 22919 | 25810 | 15 / 309600 | 9 / 47985 |
| CC=clang | 22909 | 25800 | 14 / 309581 | 8 / 47955 |

- Replants: items 1 and 2 give 4 of 4 failing (m247, m551, m161 in two programs). Item 3 gives 12 of 16 failing.
  The scratch copies are removed.
- Lines are at most 116 characters and every file ends with a newline (checked in every file I wrote).
- Build trees, executables and the .so are deleted. No log is above 100 KB. `check-all` and the repository suites
  were not run.

## Findings

1. MAJOR, against the tests of `test_localfactor`. The derivative certificate could be weakened unnoticed.
   - 217 (1/A dropped) gives a wrong result at `s = 2 + i(1/2 +/- 1/8)` at every precision tested, and it passed
     `test_localfactor`, `test_tate_local` and `test_tate_gamma`.
   - 225 (c! dropped from M1), 248 (`mag_sub` in the guard) and 253 (M1 dropped from Bsmall) give
     non-enclosures, for example at `s = 9 + 6i +/- (1/8, 1/8)`. They passed `test_localfactor`; only
     `test_tate_local` killed them.
   - 221, 223 and 255 (looser bounds) were not seen by any width check.
   - Probable cause (not checked box by box): the existing families are mostly near the real-axis poles; the
     killing boxes (exact real part at c = 1; n = 0 off the axis where the refinement binds) were not among them.
     The new test closes these gaps.
2. The gcc warnings at -O1 with gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1). The files were not changed.
   - `src/lpow.c:259:10`: 'k' may be used uninitialized [-Wmaybe-uninitialized] (in `via_pow_si` inlined into
     `adf_lball_powunit`; `k` declared at :280).
   - `src/rfun.c:339:29` and `src/rfun.c:424:29`: 'rf_rat_ok' reading 16 bytes from a region of size 8
     [-Wstringop-overread] (argument of type `const fmpq[1]`).
   - With `-Werror` these stop the -O1 build that `tools/memcheck/run.sh` uses.
3. A defect of the test `test_tate_gamma.c` (repaired, item 4), not of the code.
4. The statuses section of `test_tate_continue` cannot finish under valgrind because of its time limits (:611,
   :617). A memcheck run of it needs those checks skipped.
5. No defect of `src/` or `include/` was found. There is no counterexample to L14, P15, decision 5 or Y16.

## Not done

- Mutants 247, 252 (drop) and 344 of `src/localfactor.c` are neither killed nor proved equivalent. The search found
  no wrong enclosure, and F5 of f-repair9 remains open.
- No new run of `tools/mutate/mutate.py`. The red runs are exact replants of the listed mutants.
- No long differential fuzz run. The probe is a candidate search over 10767 calls, not a fuzz run.
- The full Julia suite, `check-all` and the driver were not run.
- Sources pending: none new.
