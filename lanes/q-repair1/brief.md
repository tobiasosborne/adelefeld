# Lane q-repair1: close the two test gaps of review q-review3 in `tests/test_qclass_reduce.c`

Review q-review3 (Claude Opus, 2026-10-07; `docs/reviews/m3/review-qclass-reduce.md`) found no defect in
`adf_qclass_reduce` (`src/qclass.c`, lane q-slice2) but two faults that `tests/test_qclass_reduce.c` does not
detect. You add the tests that detect them. You do not change `src/qclass.c`, the headers or the design.

**You own:** `tests/test_qclass_reduce.c`, `tests/ref/vectors/q-repair1/` (new vectors, with their generator in
your lane directory), `lanes/q-repair1/`. Everything else is read-only. Read `lanes/COMMON.md` and
`lanes/COMMON-C.md` first; then the two findings R1 and R2 of the review, the comment block of
`adf_qclass_reduce` in `include/adelefeld/qclass.h`, algorithm R in `docs/api-3.md` section 2.2 (step 3:
"enumerate floor(l_j) <= n < ceil(h_j)"), and how `tests/test_qclass_reduce.c` builds lifts and checks a result
today (reuse its helpers; keep its style).

**R1 (MAJOR).** Fault F2: when `0 < frac(h) < 2^-100` and `floor(h) > floor(l)`, the loop bound `stop` is set
to `floor(h)` instead of `ceil(h)`; the piece for `n = floor(h)` is neither built nor counted. The review's
smallest input: the lift `[-2^-212, 2^-210] x {0}` (real ball with midpoint `3 * 2^-213`, radius `5 * 2^-213`;
finite part the exact point 0), `prec` 53. Correct: count 2, limit 2 gives `OK` with the two pieces
`[1 - 2^-212, 1] x {1}` and `[0, 2^-210] x {0}`; limit 1 gives `LIMIT`. Under F2: count 1, limit 1 gives `OK`
with one piece, and the input point `(2^-210 ; 0)` is not in it.
Tests to add: (a) this input exactly, both limits, the two pieces checked exactly (dyadic end points after Q1,
finite triples) and the status; (b) a family of lifts whose upper end is an integer plus `2^-200` (and plus
`2^-101`, `2^-99` as controls), for a short interval (width below 1) and a long one (width 5 to 40), with
several finite radii including a fractional one: the stored count must equal the count of your own algorithm
R in exact arithmetic (in the test, with `fmpq`: `ceil(h) - floor(l)` per fibre, before deduplication), and
every end point of the input must lie in some stored piece as a point of `A/Q` (the test already has an exact
membership check or the review's `lanes/q-review3/model.py` shows how; in C, use the exact lift
`adf_qclass_lift`-style access the existing test uses).

**R2 (MINOR).** Fault F8: `y->form` is written at the start of the second pass, after the array is allocated
and before the last `LIMIT` exit; the header promises "On LIMIT y is untouched". No current check reaches a
`LIMIT` in the second pass. The review's input: the lift `[5/8, 7/8] x (1/3 + 2 Zhat)` at
`prec = ADF_REAL_PREC_MAX` (2097152), limit 10: correct `LIMIT` with `y` untouched (the rounded midpoint
exceeds the exact-work bit bound; at `prec` 2097088 the same input is `OK`, 0.31 s). Tests to add: this input
with `y` initialised to a known non-trivial class before the call, `y` compared exactly after; the aliased
call `y = x`; and the `OK` at 2097088 with the pieces checked.

**Red before green (COMMON-C rule 1), by the review's faults.** `lanes/q-review3/faults.py` plants faults in a
scratch copy of `src/qclass.c` and builds the test against it; read it and reuse its F2 and F8 patches (copy
what you need into your lane directory; do not write under `lanes/q-review3/`). Record: the new test FAILS
against F2 and against F8 and PASSES against the unmodified source. Then the whole file under
`SAN=1 INV=1` (build only the archive and this test: `timeout 600 make -j2 BUILD=lanes/q-repair1/build-san
SAN=1 INV=1 lanes/q-repair1/build-san/test_qclass_reduce`, or the make target the Makefile offers for one
test; do not run the repository's suites as a whole). Every program under `timeout`, none over 170 s; the
`ADF_REAL_PREC_MAX` case must stay under 5 s, else reduce it to the smallest precision that still gives
`LIMIT` in the second pass and say so.

Report: `lanes/q-repair1/result.md` (a Claude lane cannot write `report.md`), written once at the end: the
tests added with their check counts; the red and green runs with commands and results; anything found against
the code (none is expected; if the code is wrong, say so with the input, do not change it). Delete your build
trees at the end. Keep the generator, not large generated files.
