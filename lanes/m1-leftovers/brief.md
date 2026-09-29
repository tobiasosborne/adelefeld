# Lane m1-leftovers: what is left of milestone 1 (issues adf-4lj, adf-whv, adf-xrt)

Read `lanes/COMMON-C.md` first. Three small tasks, in this order. Finish one before the next, and write
`lanes/m1-leftovers/progress.md` after each (what was done, the commands, the results). Do NOT create
`lanes/m1-leftovers/report.md` before all three are finished or given up: the runner takes the existence
of that file as the end of the lane.

**You own:** `Makefile` (one new target, nothing else changed), the new test files named below,
`src/fball.c` and `src/recon.c` (ONLY the lines named in task 3), `lanes/m1-leftovers/`.
Everything else is read-only. No mutation run (`make mutate`) in this lane.

## Task 1 (adf-4lj): a target `check-all`

A target `check-all` of the top-level `Makefile` that runs, one after the other, and stops at the first
failure: `make check`; `sh tests/test_driver.sh`; `sh tests/test_exports.sh`; `sh tests/test_julia.sh`;
`python3 tools/mutate/selftest.py`; `python3 tools/memcheck/selftest.py`. It passes the variables `CC`,
`SAN`, `INV` on to `make check`. A line for it in the help text of the Makefile. Show that it fails when
one script fails (run it with a scratch copy of a script that exits 1, do not change the real script) and
that it passes on the clean tree. `make check` itself is unchanged.

## Task 2 (adf-whv): tests for the known gaps

Each test is red first: apply the change named, in a scratch copy of the tree under `build/` (never in
`src/`), see the test fail there, log it in `lanes/m1-leftovers/redgreen.log`.

1. `src/common.c:161`: no test reaches the status `ADF_UNSUPPORTED` that this line returns. Read the
   function and its header, write `tests/test_common_unsupported.c` that reaches it. Red: the line changed
   to return `ADF_OK`.
2. `tests/test_modctx_limits.c` has a range pin that passes without its line (the closure judge of
   `contexts` named it; `docs/reviews/m1/contexts/` and `lanes/m1-closure-contexts/`). Write
   `tests/test_modctx_pin.c` with a test of the same bound that fails when the bound in `src/modctx.c` is
   moved by one.
3. Text R3 and R9 (`docs/reviews/m1/text/review.md`; the closure judgement in `lanes/m1-closure-text/`): the
   test for R3 passes on the old printer, the test for R9 only covers lines. Write
   `tests/test_text_r3r9.c` with one test for each that fails on the behaviour the review found. If the old
   behaviour cannot be restored in a scratch copy in a few lines, say so and give the input and the two
   outputs.
4. `src/recon.c:272` (`ADF_DOMAIN` for an infinite real ball): by decision M1-D11 (`docs/SPEC.md` section
   15, accepted by TJO on 2026-09-29) this status is a courtesy of the release build. Check that the test
   at `tests/test_recon.c:1313` is compiled only without `ADF_CHECK_INVARIANTS` and says so in a comment.
   If it is, write one line in the progress file and change nothing.

## Task 3 (adf-xrt): six statements that mutation testing showed to be without effect

TJO decided: delete the dead or redundant statements; keep the canonicalise of `src/recon.c:111` and pin
it with a test. The line numbers are those of `lanes/m1-testgaps/report.md` section 8; the source has
moved since, so find each statement by its text and give the present line in the progress file.

- `src/fball.c` 420, 422 (dead guards), 734 (redundant `fmpq_sub`); `src/recon.c` 97, 175 (`fmpz_one` with
  no effect).
- For each: FIRST write down, in `progress.md`, the reason why the statement cannot change any result
  (which earlier statement makes the guard false or the value already set), with the lines. If you cannot
  give that reason from the code, do NOT delete the statement; list it as kept.
- Delete one statement, run `make -j2 check`, then the next.
- `src/recon.c:111`: `tests/test_recon_canon.c`, a test that fails when the canonicalise is removed (the
  bounds of the result in canonical form); red in a scratch copy.

## At the end

`make clean && make -j2 check`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check INV=1`,
`make clean && make check-all` pass; give the last line of each. Then `report.md`.
