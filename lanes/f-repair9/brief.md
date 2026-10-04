# Lane f-repair9: repair of review f-review15 (the local zeta factors): five faults that pass the tests

Review f-review15 (`docs/reviews/f1/review-localfactor.md`, the same text as `lanes/f-review15/report.md`)
found no defect of `src/localfactor.c` and planted 13 faults: five pass `tests/test_localfactor.c` (36725
checks). Three give a wrong enclosure or status (F1, F2, F3: MAJOR test gaps), two change the derivative bound
`B` of design Z6 without an observed consequence (F4, F5: unclassified). Read the review completely, then
`lanes/f-review15/mutations.py` (the faults), `bridge.c` (the input format of the reproducers) and
`minimized.json`, `witnesses.json`; `docs/design/local-zeta.md` Z4 and Z6; `docs/api-1f9.md` Y16, Y17; the
report of the code lane, `lanes/f-slice14/result.md`.

This is a repair of TESTS. `src/localfactor.c` changes only if part 2 proves that a term of `B` is not needed,
and then only by the proof's consequence (see there).

## Part 1: F1, F2, F3

For each fault add to `tests/test_localfactor.c` the checks that the library passes and the mutant fails. Red
first: build the test against the MUTANT (a scratch copy under `lanes/f-repair9/build`; copy the review's
`mutations.py` into your lane directory) and record the failing assertion; then against the library.
- F1 (the bound `B` halved cuts a true value off): the review's inputs: the real place, `prec` 16, the real
  interval `[-17/256, -15/256]`: the right end point value `-35.92517330590445779505...` must be inside the
  result; and `s = -2^-4 +- 2^-14` at `prec` 256. One literal input is not a test of a bound: add a FAMILY:
  thin real boxes and complex boxes near the poles `0, -2, -4, -16` at distances `2^-1 .. 2^-12`, radii
  `d/2 .. d/2^14`, `prec` 16 to 256, where the refinement is active (design Z4 real place step 5: `n <= 64`
  and `max Re(Z + n) <= 64`), each with the values at both end points (or four corners) and the midpoint as
  certified reference enclosures. The references must not come from the code under test: generate them with
  `proto/zeta_checks.py` (its certified Gamma reference of Z8; see how lane f-slice14's
  `lanes/f-slice14/select_fixtures.py` exports samples) into `tests/ref/vectors/f-repair9/`, at most 300 KB,
  and cross-check twenty of them against `mpmath` at 400 digits (say which).
- F2 (the radius `Eplus` of Z4 step 3 rounded down accepts a pole ball): the review's input `p = 2`, `prec` 2,
  the real interval `[0, 2]`: `NOT_DETERMINED`, `y` untouched, `where = 2`; the same at `prec` 128; and a
  family: at `p` in 2, 3, 5, 7, 65537, `2^64 - 59` intervals and boxes whose CLOSED boundary contains the pole
  `0` (end point exactly 0; a corner exactly 0) with radii `2^-10 .. 2^3`, `prec` 2 to 256: never `OK`.
- F3 (the closed integer test made open: `LIMIT` for a pole ball): the review's input: the real place, `prec`
  2, the real interval `[-128, -127]`: `NOT_DETERMINED`, not `LIMIT`; `[-200, -199]` at `prec` 64; and a
  family: intervals `[-2 n, -2 n + w]` and `[-2 n - w, -2 n]` for `n` in 1, 2, 33, 64, 65, 100, 1000 and
  `w` in `2^-20`, 1/2, 1: `NOT_DETERMINED` (the pole at the closed end point is decided before any limit).
Then the table: each of the review's 13 faults against the new test program: the eight detected before stay
detected; F1, F2, F3 are detected now.
- The review also saw the TEST PROGRAM crash (SIGSEGV after failed assertions) under two faults (wrong-sign,
  lowprec-log-no-radius), and lane f-slice14 saw the same under two of its own. A test that crashes loses its
  count. Find the assertion after which the test reads an output it must not read on failure, and guard it;
  the two mutants must end with a count and exit 1.

## Part 2: F4 and F5 (the terms `log(pi)` and `M0` of the bound `B`)

`L_inf'(s) = L_inf(s) (psi(s/2) - log(pi)) / 2` (the review derives it). The code's `B` bounds this on the
box; fault F4 omits `+ log(pi)`, fault F5 omits the multiplication by `M0`, and no test notices. For each of
the two, decide which is true and prove it:
- (a) the term is NEEDED: exhibit an admitted input on which the mutant returns `OK` with a ball that misses a
  true value (certified as in part 1). Where to look, from the formula: F4 where `psi(s/2)` is near 0 while
  `log(pi)` is not: `psi` has its positive zero at `x0 = 1.4616321...`, so boxes around `s = 2 x0 = 2.9232...`
  (there `|L'|` is about `|L| log(pi) / 2` and the mutant's `B` is near 0); F5 where `|L_inf|` is far above 1
  on the box: `s` real and large (`s = 40`: `L` is about `10^7`) with `Z = s/2 <= 64`, or close to a pole.
  Use radii for which the refinement is the tighter of the two enclosures (otherwise the first candidate hides
  the fault): say how you find them. Then add the witness family to the test as in part 1, and the mutant
  must fail.
- (b) the term is NOT needed for soundness on every admitted input: then a proof, steps numbered, in
  `docs/api-1f9.md` Y16, of the smaller bound, and the code is simplified to it (red-green: the old-against-new
  comparison on 20000 random real-place boxes: statuses equal, every new ball contained in the old one or
  equal, and the part 1 references still contained).
One of (a), (b) for each of the two; "not found" after a stated search is reported as not done, with the
search.

**You own:** `tests/test_localfactor.c`, `tests/ref/vectors/f-repair9/` (new), `src/localfactor.c` (only under
part 2 (b)), the Y16/Y17 part of `docs/api-1f9.md` (the last 110 lines), `lanes/f-repair9/`. Everything else
is read-only. No git command that changes state, no `bd`. Another lane (q-slice1) is writing `src/qclass.c`,
`src/text.c`, `tools/adf/adf.c` in another worktree: not your files.

**Checks at the end** (commands and numbers in the report; at most 2 jobs; every program under `timeout`,
none over 170 s): `test_localfactor` in the plain build, under `SAN=1` (your sandbox cannot run LeakSanitizer:
say so), `INV=1`, `CC=clang`, in build directories under your lane directory; the run time of the test
program (it must stay under 30 s in the plain build); the fault table. Remove your build trees at the end.
Lines at most 116 characters; files end with a newline.

Report: `lanes/f-repair9/report.md` (rule 8 of `lanes/COMMON.md`): the fault table before and after; the
families and how the references were certified; the verdict (a) or (b) for F4 and for F5 with the witness or
the proof; what is not done.
