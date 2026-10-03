# Lane f-slice12: the quadratic symbols and the Hilbert symbol (milestone 1F, WP 1F.9, first slices of the catalogue)

PLAN row 1F.9 (`docs/PLAN.md` line 279) lists the catalogue of functions special to arithmetic, Tier A. SPEC
9.3.7 (`docs/SPEC.md` lines 685-720: the table rows of the Legendre, Jacobi and Kronecker symbols and of the
Hilbert symbol, and the "Notes on the Hilbert symbol") fixes what they are; `docs/proofs/catalogue.md` proves
them (Definition 1 and Proposition 2: the quadratic symbols and the precision they need; the section "Hilbert
symbols": the formula at odd `p`, at 2, at the real place, the product formula, the undetermined cases); the
Python checks are `proto/catalogue_checks.py` (`check_hilbert` and its neighbours). Nothing of the catalogue
exists in C. You build the first two slices, as thin working slices that a user can call (library, driver,
Julia), ONE AFTER THE OTHER, each complete end to end before the next starts:

- **Slice A: Legendre, Jacobi, Kronecker.** On exact integers (`fmpz`), and on the inputs of finite precision
  for which Proposition 2 of `catalogue.md` says the symbol is determined (the symbol depends on `a` modulo a
  stated modulus: a finite ball `a + N Zhat` or a unit coset `c U(N)` determines it exactly when that modulus
  divides `N`; otherwise `NOT_DETERMINED`, never a guess). The test cases of the PLAN row: `(1/2) = +1` against
  `(3/2) = -1` (the Kronecker symbol at 2), and those of the SPEC table.
- **Slice B: the Hilbert symbol `(a, b)_v`** at a place `v` (a prime, including 2, or the real place) for two
  non-zero values at that place: local balls `adf_lball` at a prime (the symbol is determined by the valuations
  modulo 2 and the units modulo `p`, modulo 8 at 2: `NOT_DETERMINED` when a ball does not fix them, a status
  when a ball contains 0), real balls at the real place; then for two exact rationals at a place (and the
  product formula over all places as a TEST, not a function); then for two ideles at a place, with the
  undetermined cases of the SPEC notes. The test cases of the PLAN row: the product formula on rationals;
  solvability of `a x^2 + b y^2 = z^2` modulo 16, 9, 25 on reduced coefficients as an independent oracle; all
  64 pairs of square classes at 2; `(3, 3)_2 = -1` with the cofactor of the scale (`r = s = 3`, unit 1).

Local zeta factors, the profinite power, binomials, content and the cyclotomic action are NOT yours.

Decisions (proposed N-D18; you propose them in the header FIRST, with the alternatives, then implement): the
names and the files (propose `include/adelefeld/symbol.h`, `src/symbol.c`, in the style of `lroot.h` and
`gfunc.h`); how a symbol value is returned (an `int` in `{-1, 0, +1}` through a pointer, with the status as the
return value, is the pattern of the library: check `conventions.md` 2 and 3 and follow it); which input types
each function takes (do not invent a type; `fmpz`, `adf_rat`, `adf_fball`, `adf_ucoset`, `adf_lball`,
`adf_idele`, `arb` exist); the statuses and `where` (conventions 3.1 to 3.3; `gfunc.h` is the newest pattern of
a function with a report `where`); the limits. Where SPEC 9.3.7, `catalogue.md` and the PLAN row disagree or
are silent, that is a finding with both quotations: take the reading a careful number theorist would, and say so.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for the
new header, which is yours; rule 5 as written), `docs/SPEC.md` 9.3.7 and 15.4 (the last five rows, for the
style of a decision), `docs/proofs/catalogue.md` (all of the two sections named above, cite by line),
`proto/catalogue_checks.py`, `docs/conventions.md` 2.1, 2.2, 3.1 to 3.3, 4.4 (debug invariant entry checks:
your public functions need them; `src/gfunc.c` shows the pattern, `src/invariants.h` the macros),
`include/adelefeld/gfunc.h` and `src/gfunc.c`, `lball.h` (valuation, decomposition `p^m w u`, `unit_mod`),
`ucoset.h`, `idele.h`, `fball.h`, `rat.h`, `place.h`, `docs/api-1f8.md` (the style of statements with proofs
and "Check:" lines), `tests/test_gfunc.c`, `tools/adf/adf.c` and `tools/adf/README.md` (the commands `root`,
`Log_at` as the newest patterns), `tests/julia/gfunc.jl`, `tests/test_julia.sh`,
`docs/reviews/f1/review-gfunc.md` and `review-lpow.md` (what reviewers of this code base find: tests that a
planted fault passes; a `DOMAIN` that is not proved; sentences the code does not keep). `refs/src/` is on disk
(FLINT 3.0.1 under `refs/src/flint-3.0.1/`: `fmpz_jacobi`, `fmpz_kronecker`, `n_jacobi` in `fmpz.rst`,
`ulong_extras.rst`: read their domains before you call them, cite file and line).

**You own:** `include/adelefeld/symbol.h` (new), `src/symbol.c` (new), `tests/test_symbol.c` (new),
`tests/julia/symbol.jl` (new), `proto/symbol_checks.py` (new: YOUR oracle, exact integers, independent of
FLINT and of `proto/catalogue_checks.py`: the Legendre symbol by Euler's criterion AND by counting squares;
the Hilbert symbol by searching solutions of `a x^2 + b y^2 = z^2` modulo `p^k` with primitive `(x, y, z)`,
with the proof of which `k` suffices), `tests/ref/vectors/f-slice12/` (new, below 1 MB), `docs/api-1f9.md`
(new: the interface and decisions, statements Y1 ... with proofs or with the exact citation of `catalogue.md`
where the proof is there), `tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/symbol-*.cmd` and `.out`
(new; expected lines written BY HAND from mathematics before the run), `lanes/f-slice12/`. In
`include/adelefeld.h` and `tests/test_julia.sh` you may add the lines your files need. Everything else is
read-only. No git command that changes state, no `bd`. At most 2 cores (another session shares this laptop);
every program under `timeout`; build into `BUILD=lanes/f-slice12/build` while you work and into `build/` only
for the final checks. Julia is on the PATH.

Order of work, for slice A and then again for slice B:
1. Header first (comment blocks: the set statement; `catalogue.md` with file and line; every status with the
   input that gives it and `where`; what is untouched on a status other than `OK`; the limits).
2. Tests first, red then green (`lanes/f-slice12/redgreen.log`; a link error counts only for the first test).
   Oracle `proto/symbol_checks.py` with fixture rows; in addition, in C: multiplicativity in each argument,
   quadratic reciprocity with the supplements, periodicity in `a`, and for the Hilbert symbol bilinearity,
   symmetry, `(a, -a) = 1`, `(a, 1 - a) = 1`, the product formula on 2000 pairs of rationals over the places
   dividing `2 a b` and the real place, all 64 pairs of square classes at 2 against the solvability search;
   inputs of thousands of bits; primes 3, 5, 7, 65537, `2^64 - 59`; every status with `where` and untouched
   outputs; the finite-precision inputs on both sides of "determined" (a ball one digit too coarse must be
   `NOT_DETERMINED`, and you prove with two points of that ball that the symbol really takes two values).
3. The code (with the debug invariant entry checks); the driver commands; the Julia example.
4. Six planted faults for each slice in a scratch copy under your build directory, each with the test that
   fails (among them: the supplement for 2 with the wrong residues modulo 8; the Kronecker symbol at 2 for even
   `a` not 0; a ball one digit too coarse accepted; the Hilbert symbol at 2 without the `epsilon(u) epsilon(v)`
   term; the valuation parity of one argument ignored; the real place returning `+1` for two negatives).
   Mutation testing of `src/symbol.c` (at most 60 mutants, `--seed 1`, 2 jobs, `--san`, `timeout 1300`; a
   `--make` that builds and runs only `test_symbol`); read the tool's last line and say how many compiled.
5. Final checks, once, at the end: `timeout 900 make -j2 check-all` in `build/`; then, because this sandbox
   cannot run LeakSanitizer, say so and build `test_symbol` with `SAN=1` (`ASAN_OPTIONS=detect_leaks=0`) and
   with `INV=1` (`BUILD=lanes/f-slice12/build-inv`) and with `CC=clang` (`BUILD=lanes/f-slice12/build-clang`),
   and run each: three more lines in the report. Every source file ends with a newline.

If you run short of time, finish slice A completely rather than leaving both half done, and say what of slice B
is missing.

Report: `lanes/f-slice12/report.md`, written ONCE, AT THE END; running notes in `lanes/f-slice12/progress.md`
as you go (slice A recorded as finished before slice B starts). In it: functions built; the decisions with
their alternatives; what is proved and what is cited; the numbers of the tests and what would have made a case
fail; the fault table; mutation survivors one line each; findings against the specification, the proofs or
this brief; what is not done.
