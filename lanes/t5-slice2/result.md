# Lane t5-slice2: report (slice 5c, the Tate vector and the global value for Re(s) > 1)

Status: zeta and the primitive characters are both done, end to end, in the one user call. Steps A to F are done.
Worktree `/home/tobias/Projects/adelefeld-wt/t5-slice2`, branch `lane/t5-slice2`, nothing committed.

## Files

New: `include/adelefeld/tate.h`, `src/tate.c`, `tests/test_tate.c`, `tests/ref/vectors/t-slice2/tate.jsonl`
(115 KB), `tests/driver/tate.cmd`, `tests/driver/tate.out`, `tests/julia/tate.jl`, `docs/api-5b.md`.
The lane directory holds `gen_vectors.py`, `derive_fixture.py`, `faults.py`, `faults.out`, `resurvivors.py`,
`resurvivors.out`, `mutate.log` (10 KB) and `redgreen.md`.
Changed: `include/adelefeld.h` (one line, `#include "adelefeld/tate.h"`), `tools/adf/adf.c` (two commands),
`tools/adf/README.md` (one section), `tests/test_julia.sh` (the `tate.jl` block).

## What each step did

**A. Vectors.** `timeout 300 python3 -B lanes/t5-slice2/gen_vectors.py` (4.7 s) writes, from
`proto/tate_checks.py`:
- 18 `vector` records: C = 1 and the 17 golden characters, with f[j] as exact phases or null.
- 144 `value` records: I_chi(s) at 60 digits (mpmath Hurwitz) for s = 2, 3, 9/8, 3/2 + 14i, 2 + 3i, 10001/10000
  and two balls (radius 2^-70 on 2 + 3i, 2^-60 on 3/2; midpoint and four corners). Each value has python-flint's
  `dirichlet_char(C, label).l_function` completed by hand with pi^-z Gamma(z) as a second source. The generator
  asserts that the two sources agree to 1e-45 relative.
- 57 `cutoff` records: the oracle's N, R and panel degrees.
- 18 `piece` records: T3 references from the oracle's `numeric_piece`.
- 1 `witness` record: zeta on [9/8, 5/4]; its two end values differ by 3.91.

The installed python-flint is 0.8.0 on FLINT 3.3.1. It is a reference only.

**B. Tests** (`tests/test_tate.c`, written before the code). The red and green runs are in `redgreen.md`. The tests
cover:
- Vectors exactly: cardinal phases exact, others overlapping the 512-bit phase with radius <= 2^-118; `chi->s`
  ignored.
- Statuses with sentinel bytes for both calls.
- Every value record: containment of both sources and diameter <= 2^-bits (bits 53, and 20 and 80 for
  C = 1, 4, 5, 16).
- `I(2) = pi/6`; chi_4 at s = 2 against Catalan/(2 pi) and against FLINT's `acb_dirichlet_l` completed by hand;
  `L(1, chi_4)`: DOMAIN.
- The domain gate: exact 1, 1/2 + 10i, 0, the closed-endpoint ball [1 - 2^-9, 1] give DOMAIN; a ball straddling 1
  gives NOT_DETERMINED; a nonfinite s gives DOMAIN.
- bits -1, 2^21 + 1, WORD_MIN give DOMAIN, with the precision cap first and the size cap first.
- The work cap (D2):
  - C = 1031 and C = 65536 are LIMIT before any setup;
  - C = 1024 is refused by the charge before the transform;
  - C = 1021 is LIMIT in 1.14 s;
  - zeta at bits 2^21 is LIMIT in under 0.01 s;
  - zeta at bits 1400 is refused by the sum of the panels while each panel fits.
- The width witness: NOT_DETERMINED. The input balls at bits 80: NOT_DETERMINED, with corner references that prove
  no OK is possible.
- The width shrinking at bits 20, 53, 80 for C = 1, 5, 16 (the SPEC 8 acceptance), and the same targets met from
  prec 2 by the retries.
- `z = s` aliasing, also on failure.
- 1 + 2^-20 near the pole.
- T5: both `adf_tensor_poisson` enclosures against the theta expansion with L6 tails, at u = 1, 3/2, 2/3, for
  C = 1, 4, 5, 16. The dual coefficients `g[n mod C] Q(n/C)` from the two transforms against
  `W conj(chi(n)) n^e` with `adf_char_root_number`.
- T3 pieces against the incomplete-Gamma integral at targets 2^-60 and 2^-6. At 2^-6 all 12 records with R > 1
  need the remainder.
- The width-boundary scan: zeta at 2 +- r over 192 radii gives 65 OK, all narrow and containing both end values,
  and 127 NOT_DETERMINED.
- The 57 cutoff records: N, R and every panel degree equal the oracle's.
- INV: 4 precondition aborts; the children clear their objects after the call.

**C. Code** (`src/tate.c`, 812 lines).
- `adf_tate_vector` uses `adf_char_chi`, `adf_ffun_set_acb_vec` and `adf_rfun_set_terms`.
- `adf_tate_integral` follows the design's D3 and D4: the balanced vector (A = 1/C), `adf_ffun_fourier` and
  `adf_rfun_fourier`, the dual coefficients `g[n mod C] Q(n/C) = W conj(chi(n)) n^e`, the P15 searches with L6
  (beta = 0) and both branches of L14, the T3 panels with the geometric remainder, the pole terms for C = 1, the
  error added to both radii, then times `C^-z`.
- The width check comes after everything. The retry rule is that of `adf_tensor_poisson`. D2 work charging.
- INV entry checks. Two hidden test hooks (`adf_tate_taylor_piece`, `adf_tate_cutoffs`), not exported, following
  the precedent at `src/roots.c:49-53`.

**D. User calls.**
- `adf tate_vector CHI` prints `PHI | F`.
- `adf tate_integral CHI with S with BITS` prints `VALUE | halfplane Re(s) > 1 | s=S`.
- Fixture `tests/driver/tate.cmd` / `tate.out`: 17 lines derived before the run (`derive_fixture.py`:
  conventions 9.5 printing on exact rationals, mpmath midpoints, the oracle's error bounds), among them `I(2)`
  for zeta and chi_4, DOMAIN at s = 1, NOT_DETERMINED for a straddling ball and for the witness. They were equal on
  the first run. The driver code itself was written before the fixture.
- `tests/julia/tate.jl` makes the design's call `ccall((:adf_tate_integral,lib),Cint,(P,P,P,L,L),z,chi,s,64,128)`
  and is registered in `tests/test_julia.sh`.

**E. Statements.** `docs/api-5b.md` "Slice 5c". It is a new file because `docs/api-5a.md` does not exist in this
worktree. It has S1 to S8 with proofs and "Check:" lines, the statuses and order of checks, nine decisions where
the design is silent, and the cost.

**F. Faults and mutation.** Below.

## Checks run (final code)

| Check | Result |
|---|---|
| `timeout 300 lanes/t5-slice2/build/test_tate` (plain) | `tate: 22904 checks`, 3.5 s |
| INV=1 | 22914 checks, 4 aborts |
| SAN=1 with `ASAN_OPTIONS=detect_leaks=1` | 22904 checks, no report, the binary is linked with asan |
| CC=clang | 22904 checks |
| `sh tests/test_driver.sh` | 96 cases, 101643 lines, all equal |
| `sh tests/test_exports.sh` | 619 of 619 exported, the hooks hidden |
| `julia tests/julia/tate.jl build/libadelefeld.so` | 9/9 pass (with the libgmp `LD_PRELOAD` of `test_julia.sh`) |

The builds used `make -j2 BUILD=lanes/t5-slice2/build...`. The INV and SAN runs were made before the last purely
cosmetic line wraps in the test; plain and clang were run after them. `check-all` was not run.

## Fault table

Run with `timeout 900 python3 -B lanes/t5-slice2/faults.py`. It needs `lanes/t5-slice2/build` built first.

| Fault | Result |
|---|---|
| 52.1 C^z omitted | detected `test_tate.c:373` |
| 52.2 wrong real parity | detected `:373` |
| 52.3 chi conjugated twice | detected `:228` |
| 52.4 C^-s omitted from the Hurwitz reference | detected by the generator cross-check and by the C test (exit 1) |
| 52.5 factor 2 | detected `:367` |
| 52.6 xi taken for Lambda | detected `:493` |
| completion with z = s/2 for odd e | detected `:373` |
| completion factor dropped | detected `:367` |
| dual coefficients without conj | detected `:228` |
| W_chi dropped | detected `:228` |
| lower half not replaced through Poisson | detected `:367` |
| quadrature remainder dropped | detected `:367` (zeta(2) at bits 80) |
| width checked before the remainder | detected `:283` (the boundary scan) |
| z written on NOT_DETERMINED | detected `:112` (sentinel bytes) |

## Mutation testing

Command: `tools/mutate/mutate.py --files src/tate.c --limit 60 --seed 5 --jobs 2 --san --make "make -s -j1 INV=1
build/test_tate && timeout 380 build/test_tate" --copy Makefile include src tests`.

Result: 60 mutants in 1427 s (24 min, over the 20-minute bound because each SAN rebuild takes about 30 s). 33
killed, 20 survived, 5 did not compile, 2 timed out.

Afterwards I added the hook `adf_tate_cutoffs`, the 57 cutoff records and two work-cap cases. Re-running the gap
survivors (`resurvivors.py`) kills 8 of them: 122, 387, 231, 232, 218, 374, 356, 336. The 12 remaining, one line
each:
- 358, 370, 235, 174: argument swap in a commutative `arb_max`, `arb_mul` or `arb_add`. Equivalent.
- 209: `> 0` to `>= 0` at r = 0 gives `rp = 0` either way. Equivalent.
- 526: `arb_zero` of a freshly initialized `t`. Equivalent.
- 553: `arf_sgn(lo) <= 0` to `< 0`; `lo` is a lower bound of pi/C and is never 0. Equivalent.
- 533: a defensive exit for a nonfinite `C^-z`, unreachable for finite s. With the new guard (an attempt is OK only
  once its output is written) the mutant now returns NOT_DETERMINED.
- 578: the R search reaching the work cap. It needs fewer than about 40 units left after the N search; I found no
  input that reaches it. The guard turns the mutant into NOT_DETERMINED instead of LIMIT.
- 383: the J-loop work guard `-` to `+`. The charge after the loop still refuses, so the status is the same and
  only the time differs.
- 560: the N search starting at 1. N = 0 was never selected in any of the 57 oracle records or any other input
  tried. Not proved unreachable.
- 551: the upper bound of Re(z') dropped (r2 = -1). This is unsound for e = 1, but no recorded cutoff changes,
  because the forward side dominates the sums. A test gap.

Nothing was added to `tools/mutate/equivalent.txt`.

Tool observation: `--copy ... lanes`, as `lanes/COMMON-C.md` rule 5 suggests, copies the whole lane directory,
build trees included, for every mutant (74 MB here). I dropped `lanes` from the copy.

## Findings against the design, the oracle and the proofs

1. The design (`api-5.md:144`, brief) says to consume the existing tail kernels. The L6 kernel `pn_series` in
   `src/poisson.c` is static, so it cannot be called. `tt_series` is its beta = 0 case written again. Moving it
   into a shared internal header is the orchestrator's call.
2. The design's retry rule (`api-5.md:224-225`, "two successive attempts from precision >= 64 fail to halve") is
   ambiguous. I implemented the rule of `adf_tensor_poisson`: stop when one doubling from a precision >= 64 does not
   halve the largest radius.
3. The oracle agrees with the C code exactly on all 57 cutoff and degree records, and every value contains both
   references. There is no finding against P11 to P15, L6, L14 or T3 to T5. P13 step 5's finite transform
   `(tau/C) conj(chi(-k))` equals the code's derivation `(tau/C)(-1)^e conj(chi(k))` (api-5b.md S3).
4. N-D23 is confirmed in the C code. Input radii set a floor on the width: a radius of 2^-60 reaches bits 56 but
   not 64, a radius of 2^-70 reaches 64 but not 72. The enclosure radius for a real ball is about 1.26 r against
   |I'(2)| r = 0.75 r, a dependency overestimate of about 1.7. That is not a defect, but it bounds what a ball input
   can get.
5. No HEADER-FINDING: `tate.h` is new and owned by this lane. It adds `ADF_TATE_TRANSFORM_C_MAX = 1024` (the D2
   transform cap, made explicit) beside the design's `ADF_TATE_WORK_MAX`.

## Sources pending

- The upper incomplete Gamma documentation (`acb_hypgeom.rst` is not under `refs/`). It is used only through mpmath
  in the generator, as a numerical reference (P15 already marks it pending). Neither the code nor the C test uses
  it.
- Inherited: the universal FLINT Conrey phase and label identification (api-3c/3d). The 18 vectors agree with the
  oracle's phases.

## Not done, and costs noted

- Not done: 5d, 5e, 5a, 5b (not in this slice).
- No fuzzing: none was asked for.
- Costs noted: each retry rebuilds the vector and the transforms and redoes the cutoff searches, although only
  rounding depends on the precision. The C calls to `adf_char_chi` each set up the character group again.
