# Lane f-fixture1: the stored fixture of the series at a prime guards F8 and F9 at every prime (adf-6fe)

Review `docs/reviews/f1/review-lfunc-fast.md` (lane f-review4) found no wrong enclosure in the fast
series of `src/lfunc.c` (lane f-slice5; statements F8, F9 of `docs/api-1f4.md`), and one MAJOR finding
about the test that guards them: the stored comparison `stored_before_optimisation` of
`tests/test_lfunc.c` (2000 rows of OLD-code results, `tests/ref/vectors/f-slice5/stored.jsonl`) reaches
the F9 route (`log_balanced`) with 9 of its 2000 rows and the F8 route (`log_sum`) with 384 rows all of
`K <= 38`; it has no row with `K` in `39 .. 64` at any prime and no row with `K > 38` at the word prime
`2^64 - 59`. Planted faults that act only at large `p` or at `K > 40` pass it (review, section 5). The
MINOR 2 of the same review (a regression of 13 percent at `N = 2` at the word prime, the setup of
`log_sum` for a sum of one term) is a second, optional item.

This is test work on reviewed numerical code. The LIBRARY does not change in item 1; the fixture does.
The old code is the ground truth of the fixture: `src/lfunc.c` at commit `1cf0e42` (`git show
1cf0e42:src/lfunc.c` is a read, not a change of state), built under renamed symbols exactly as
`lanes/f-review4/build.sh` does it. Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`,
`lanes/COMMON-C.md` (rule 5, mutation, is replaced by item 3 below), the review above (all of it; its
section 5 names the faults), `lanes/f-slice5/report.md`, `lanes/f-slice5/gen_stored.py` (the generator
of the present fixture; the seed and the distribution are part of the fixture contract, so a NEW file
with its own generator is the clean way, the old file stays), `lanes/f-review4/build.sh`, `probe.c`,
`faults.py`, `oracle.py` (modes `small`, `exhaustive`, `random`: the referee's own oracle, which you may
reuse as a check of the old code's rows), `docs/api-1f4.md` F8 and F9, `src/lfunc.c` (the route
selection: `log_sum_word` for a working modulus within a tagged word, `log_sum` for `K <= 64`,
`log_balanced` above), `tests/test_lfunc.c`.

**You own:** `tests/test_lfunc.c` (additions; the existing tests stay), `tests/ref/vectors/f-slice5/`
(a NEW file `stored_large.jsonl`, below 1 MB; `stored.jsonl` unchanged), `src/lfunc.c` ONLY for item 4
if you do it, `docs/api-1f4.md` ONLY a sentence on the fixture under F8/F9 if one is needed,
`lanes/f-fixture1/` (your generator, your logs, your report). Everything else is read-only. No git
command that changes state, no `bd`. Build into `BUILD=lanes/f-fixture1/build` while you work and into
`build/` only for the final checks.

**Battery.** The machine runs on battery today. Keep the compute of the lane small: at most 2 cores;
no benchmark; no run of more than 3 minutes; every program under `timeout`; no repeated clean
rebuilds (build once, then incrementally); the referee's oracle only in its `small` mode if at all.
At the end ONE `make clean && make -j2 check-all` and ONE `make -j2 check SAN=1` with
`ASAN_OPTIONS=detect_leaks=0` (the codex sandbox cannot run LeakSanitizer; the orchestrator runs the
default sanitizer suite, clang and INV on master). Do not run the clang or INV suites.

1. Generator first: `lanes/f-fixture1/gen_stored_large.py`, deterministic (seed in the file), that
   writes `tests/ref/vectors/f-slice5/stored_large.jsonl` in the SAME row format as `stored.jsonl`
   (so that `lb_from_json` and `fields_equal` of `tests/test_lfunc.c` read it unchanged), from the OLD
   code at `1cf0e42` through a probe you build like `lanes/f-review4/build.sh` (reuse that probe if
   it fits; the probe program of f-slice5, `lanes/f-slice5/stored_only.c` or the one its
   `gen_stored.py` calls, is the pattern for the alias and unchanged-output columns). Rows per prime
   for `p` in 2, 3, 5, 7, 11, 65537, `2^64 - 59`: 20 to 50 with `K` in `39 .. 64` (F8), 20 to 50 with `K`
   in `65 .. 3000` (F9; at the word prime keep most `K` below 1500 so that the old code answers each
   row within seconds: the old code at `N = 10000` at the word prime took 75 s), among them at least 5
   per prime with several doubling steps of F9 (`v(z) = 1`, `K >= 200`), for `log` and `Log` (and a
   few `exp` rows for symmetry), exact inputs and balls, negative valuations for `Log`, and, for each
   prime, at least 3 rows at the route boundaries (`K = 64, 65`; the largest `K` whose working modulus
   `p^W`, `W = K + e(2K)`, fits a tagged word, and the next one; take the rule from `src/lfunc.c`, not
   from the review's estimate). Say in the report how many rows reach each route, counted with the
   route rule read from the code. Write the distribution and the counts into a comment at the head of
   the generator.
2. Test: a new `ADF_TEST(stored_large_before_optimisation)` in `tests/test_lfunc.c`, the twin of
   `stored_before_optimisation` on the new file (value, status and the alias call); red is shown by
   running it on a scratch copy of `src/lfunc.c` with one planted fault (item 3), green on the code of
   master. Keep its run below 10 s on this laptop (`timeout 60 build/test_lfunc`, give the time).
3. Show that it bites: `python3 lanes/f-review4/faults.py <scratch> ...` (read it; it builds a faulty
   archive and runs `test_lfunc`), with its named faults INCLUDING `f9_inverse` (the residual inverse
   one digit short, which passed the old fixture) and the F8 fault that acts only for `K > 40` (the
   review, section 5: find its name in `faults.py`). Each must now fail the NEW stored comparison; give
   the number of failed checks per fault in a table, and the control (an unmodified copy: 0).
4. Optional, only if items 1 to 3 are done with time left: MINOR 2 of the review. If you change
   `src/lfunc.c`, every row of BOTH stored files and every other test must give identical results; show
   the time before and after at the review's input in 3 runs (`lanes/f-review4/bench.sh`, as its
   `rate-recheck.log` says), 20000 alternating calls each, nothing larger. If the gain is under 5
   percent, leave the code and say so.
5. Final checks as under "Battery"; give the last line of each.

Report: `lanes/f-fixture1/report.md`, written ONCE, AT THE END; running notes in
`lanes/f-fixture1/progress.md` (write them as you go: if your session is cut off, they are the report).
In it: the row counts per prime and route; the fault table; the time of the new test; what is not done;
findings against the specification or the review.
