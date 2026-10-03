# Lane f-repair7: the four findings of review f-review9 on the first all-places forms (`gfunc.h`)

Review f-review9 (`docs/reviews/f1/review-gfunc.md`; its programs are in `lanes/f-review9/`) found no wrong
result in `include/adelefeld/gfunc.h`, `src/gfunc.c` (lane f-slice10). It found one MAJOR test gap and three
MINOR findings. You close the four. The values the library returns on valid inputs do not change.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 5, mutation testing, is replaced by item 3
below), the review (F1 to F4, "Planted faults", "Sources pending"), `lanes/f-review9/faults.py` (how the
reviewer builds a faulty `src/gfunc.c` and runs `test_gfunc` against it), `include/adelefeld/gfunc.h`,
`src/gfunc.c`, `tests/test_gfunc.c`, `docs/api-1f8.md` (G1, G4, G6), `docs/conventions.md` 4.4 (the debug
invariant checks: line 320 and around), `src/invariants.h`, and ONE existing file that has entry checks, as the
pattern: `src/adele.c` (look for `ADF_INV_`).

**You own:** `src/gfunc.c`, `tests/test_gfunc.c`, `docs/api-1f8.md`, `lanes/f-repair7/`. Everything else is
read-only. IMPORTANT: another lane (f-slice11) is appending a new block at the END of `gfunc.h`, a new section
at the END of `docs/api-1f8.md` and a new file `src/gfunc_log.c` at this moment: do not touch `gfunc.h`, do not
add anything at the end of `docs/api-1f8.md` (edit only inside G1, G4, G6), and keep your changes to
`src/gfunc.c` local to the functions named below. No git command that changes state, no `bd`. At most 2 cores;
every command under `timeout`; build into `BUILD=lanes/f-repair7/build` (for example
`timeout 600 make -j2 BUILD=lanes/f-repair7/build lanes/f-repair7/build/test_gfunc`). Write nothing into `/tmp`
except under a directory you remove at the end.

## The four repairs

1. **F1 (MAJOR test gap).** A mutant that returns `OK` for an idele with an INEXACT unit when `n >= 4` passes
   `test_gfunc`: `tests/test_gfunc.c` lines 748-765 try inexact units at degrees 2 and 3 only. Add to the idele
   status test: inexact units (`[1 mod 8]`, `[5 mod 6]`, `[2 mod 3]`, `[1 mod 5]`, a modulus of 200 bits) with
   `n` in 2, 3, 4, 5, 6, 7, 8, 12, 64, `2^32`, `WORD_MAX`, `UWORD_MAX`, both signs, positive real ball:
   `NOT_DETERMINED`, the output untouched, `where` untouched; and with a negative real ball and even `n`:
   `DOMAIN` at the real place (N-D16). The reviewer's input: `x = (1 ; 1 * [1 mod 8])`, `n = 4`.
2. **F2 (MINOR test gap).** Degree 1 is promised to be an EXACT copy (`gfunc.h` lines 129-130). A mutant that
   normalises the unit after the copy passes. Add: `x = (1 ; 1 * [5 mod 6])` (canonical, not normal), `n = 1`,
   any sign, `prec = 2`: the output is IDENTICAL field by field (the unit stays `[5 mod 6]`); the same for an
   adele with an inexact finite part and a real ball of 300 bits at `prec = 2` (no rounding).
3. **F3 (MINOR).** `docs/conventions.md` 4.4 requires that under `-DADF_CHECK_INVARIANTS` a non-canonical input
   makes a public function abort with the invariant message at its entry. `src/gfunc.c` has no entry checks.
   Add them to the eight public functions in the pattern of `src/adele.c` (the macros of `src/invariants.h`;
   use the ones that exist for `adf_rat`, `adf_adele`, `adf_idele`; if one is missing, say so and use the
   public canonical predicate with `flint_abort` as `src/lball.c` does). The release build must be
   byte-identical in behaviour: the macros expand to nothing there. Test: see how the existing tests check an
   entry abort under `INV=1` (grep `tests/` for `ADF_CHECK_INVARIANTS`) and add the same kind of check for one
   root function and one series function; if no test in the tree does this, state it and show the abort once by
   hand with the reviewer's `lanes/f-review9/repro_invariants.c` (command and output in the report).
4. **F4 (MINOR).** `docs/api-1f8.md` line 71 says that `q = r^n` with integer `r` and `q >= 2` implies
   `r >= 2`; false for `q = 4`, `n = 2`, `r = -2`. The step needs `|r| >= 2`, which gives `q >= 2^n` and
   `bits(q) >= n + 1` as before. Correct the sentence. Then the two bounds the review lists as "sources
   pending": (a) G1(c) casts `fmpz_bits` to a `slong`: either cite the FLINT text that bounds the bit count
   (search `refs/src/flint-3.0.1/fmpz.rst` and the GMP documentation under `refs/` if present; cite file and
   line) or mark the sentence `[source pending: ...]` exactly as the review words it; (b) G6: prove that the
   candidate prime of `first_failing_prime` fits a `ulong` for every numerator the library can hold (the bound
   of G6 gives at most `floor(log_3 |A|) + 1` odd primes tried; a numerator of `B` bits allows about `B / 1.58`
   candidates, and the k-th prime is below `2 k ln k` for `k >= 3`: give the inequality that keeps it below
   `2^64` for every `B` that fits in memory, or state the limit and what the code does beyond it, reading the
   code). Do not cite from memory: an inequality on primes that you use is either proved in the text or marked
   `[source pending]`.

## Order of work and checks

1. Tests first, and show that they bite: build the reviewer's faults F13 (`inexact idele high degree`) and F11
   (`degree one normalised`) with `lanes/f-review9/faults.py` (copy what you need into your lane directory; the
   reviewer's build directories are not in the tree), run the NEW `test_gfunc` against each (it must fail) and
   against the unmodified library (it must pass). Record the three runs with their failed-check counts in
   `lanes/f-repair7/redgreen.log`.
2. The entry checks (item 3); the sentences (item 4).
3. Run the reviewer's other 12 faults (`faults.py` lists them) against the new `test_gfunc` and give the table:
   which fail, which pass, and for one that passes why.
4. Final checks: `test_gfunc` in your build directory; the same under `SAN=1`
   (`BUILD=lanes/f-repair7/build-san`, `ASAN_OPTIONS=detect_leaks=1`); `timeout 600 make -j2
   BUILD=lanes/f-repair7/build-inv INV=1 lanes/f-repair7/build-inv/test_gfunc` and its run (the debug build:
   every test must still pass, so no valid input trips an entry check); `timeout 1200 make -j2 check-all` ONCE
   at the end in `build/` (give its last line).

Report: `lanes/f-repair7/report.md`, written once, at the end; running notes in `lanes/f-repair7/progress.md`.
In it: each of F1 to F4 with what was done (file and line); the red and green runs; the fault table; the checks
with commands and last lines; what is not done; findings (a sentence of the review that is itself wrong, with
the counterexample). Your work counts as finished only when `report.md` exists.
