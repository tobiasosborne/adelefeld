# Lane f-repair8: repair of review f-review13 (`Log` on ideles): eight test gaps and one cost defect

Review f-review13 (Claude Opus; `docs/reviews/f1/review-gfunc-log-referee.md`, the same text as
`lanes/f-review13/result.md`) refereed IL1 to IL8 and G7 to G12 and planted 37 faults in `src/gfunc_log.c`.
No wrong value and no false proof step. Eight faults pass every test (findings F1 to F8) and one finding is a
defect of cost (F9). Read the review completely, then `lanes/f-review13/mutate.py` (the faults as patches),
`probe.c` and `distinguish.txt` (the inputs that tell each fault from the library).

## Part 1: tests for F1 to F8 (no change of `src/` in this part)

For each of the eight surviving faults (B9, B9c, B9d, B10, B11, O1, O3, O23) add to `tests/test_gfunc_log.c`
the checks that the unmutated library passes and the mutant fails. Red first: build the test against the
MUTANT and see the new check fail (that is the red run of a test repair; record the failing assertion), then
against the library and see it pass.
- F1 (O1; MAJOR) is a gap of the fixtures, not of one assertion: `lanes/f-slice11/write_selection.py` keeps
  only `c` in `{1, M - 1, p}`, so every restricted row has `Log(c) = 0`. Add rows with `c` not congruent to
  `+-1` modulo `p^k` at 2, 3, 5, 7 (`r' = 1` and `r' != 1`, `k = 1..3`, the complete-image rows too), computed
  by the ORACLE (`proto/idlog_checks.py`, and cross-checked against `lanes/f-review13/il_oracle.py`, which
  does not use the log series: the two must agree on every new row), in a new vector file under
  `tests/ref/vectors/f-repair8/`; the review's three inputs (`(1 ; 1 * [3 mod 8])` at 2 with `N = 3`: `4 +
  O(2^3)`; `(1 ; 1 * [2 mod 9])` at 3 with `N = 2`: `6 + O(3^2)`; `Log_refine` at `{3}`: `24 mod 36`) as
  literal checks.
- F2, F7 (the real-precision `LIMIT` before `n < 0` and before `n > 65536`), F3 (the baseline charge of 4
  bits in the aggregate bound: both inputs of the review), F4 (`y` preserved after a finite `LIMIT` when the
  real part is valid), F5 (`where` untouched on `OK` by `Log_at` at infinity: pass a sentinel), F6 (a known
  local refusal after the aggregate is exceeded: `where = 2`), F8 (the place of a main-phase failure:
  `where = 5`; the review's input takes about 5 s: keep it, or find a faster input that distinguishes the
  fault and say which).
Then the table: each of the eight mutants (use `mutate.py`, copied into your lane directory; build trees
under `lanes/f-repair8/build`) against the new test program: all eight must fail. Also rerun the other 26
detected faults: still detected.

## Part 2: the cost defect F9 (red-green, in `src/gfunc_log.c`)

Review F9: G12 (`docs/api-1f8.md`) says the aggregate refusal comes first "to avoid substantial work for a
request whose combined modulus is refused", but G11 step 4 and `gfunc_log.c:309-317` evaluate every earlier
prime fully once a later prime has a known local refusal. Measured: `x = (2 ; 2 * [1])`, `N = 1048577`, the
primes `{3, 5, ..., 47}` plus `2^64 - 59`: no result within 170 s; `{3, 2^64 - 59}`: `LIMIT` after 8.8 s;
`{2^64 - 59}` alone: 8 ms.
- Decide what the documented status and `where` are for these inputs (G11: read it; the tie rule between
  places) and keep them EXACTLY: the repair changes cost only. The review says the tie rule needs only the
  working exponent `W` from the evaluator's formula, not the series. Prove that in a new paragraph of G11 or
  G12 (steps numbered): which quantity of each earlier prime decides status and `where`, and why it is
  computable without the series.
- RED: a test with a CPU-time guard (the pattern of the guard that lane f-repair6 added for the early `LIMIT`
  in `tests/test_lroot.c` or `test_lpow.c`: find it and follow it): the fifteen-prime input must return its
  documented `LIMIT` and `where` within 1 s of CPU. See it fail (time out) on the present code, under
  `timeout`.
- GREEN: the change. Then the old-against-new comparison: for at least 20000 random refine requests (small
  and large `N`, lists with and without refusals, every status) the status, `where` and value of the new code
  equal those of the old (build the old archive from `git show HEAD:src/gfunc_log.c` into your lane
  directory): 0 differences, or each difference explained and shown to be the documented behaviour.
- Update the sentences of G11, G12 and of the header `gfunc.h` that describe the order and the cost.

**You own:** `tests/test_gfunc_log.c`, `tests/ref/vectors/f-repair8/` (new), `src/gfunc_log.c`, the comment
blocks of the six `Log` declarations in `include/adelefeld/gfunc.h` (no change of a signature), `docs/api-1f8.md`
lines 274 to the end (G7 to G12), `lanes/f-repair8/`. Everything else is read-only. Other lanes are writing
`src/dump.c`, `src/localfactor.c`, `tools/adf/adf.c`, `docs/api-1f9.md`, `Makefile` in other worktrees: not your
files (your test program is registered already; a new vector file needs no Makefile line: check).

**Checks at the end** (commands and numbers in the report; at most 2 jobs; every program under `timeout`):
`test_gfunc_log` in the plain build, under `SAN=1`, `INV=1` and `CC=clang` (separate `BUILD` directories
under your lane directory); the four `tests/driver/gfunc-log-*` cases with the driver linked to the new
archive; `timeout 1700 make -j2 check-all` once (your sandbox cannot run LeakSanitizer: say so; the
orchestrator's suite is the leak check). Remove your build trees at the end. Lines at most 116 characters;
files end with a newline.

Report: `lanes/f-repair8/report.md` (rule 8 of `lanes/COMMON.md`): the fault table before and after; the new
rows and how they were computed; the red and green runs of part 2 with times; the old-against-new comparison
with counts; what is not done.
