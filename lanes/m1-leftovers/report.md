# Lane m1-leftovers: report

Three tasks: the target `check-all` (adf-4lj), tests for the known gaps (adf-whv), and the six
statements that mutation testing showed to be without effect (adf-xrt). All three are finished.
`lanes/m1-leftovers/progress.md` has the detail of each step, `lanes/m1-leftovers/redgreen.log`
the red runs.

## What was done

### Task 1: the target `check-all`

`Makefile`, four places and nothing else: a line in the header comment, `.PHONY`, the target
between `check` and `clean`, and two lines in `help`. The target runs the six steps in the order of
the brief with `set -e` in the one shell of the recipe, so it stops at the first failure, and it
passes `CC`, `SAN` and `INV` to `make check` on the command line of the sub-make. `SAN` also goes
to `tests/test_driver.sh` and `CC` to `tests/test_exports.sh`, because those two scripts read them
(tests/test_driver.sh:25 and tests/test_exports.sh:42). `make check` itself is unchanged.

### Task 2: four tests for the known gaps

| File | What it reaches | Red run |
|---|---|---|
| `tests/test_common_unsupported.c` | the status `ADF_UNSUPPORTED`, src/common.c:161 | line 161 returns `ADF_OK` |
| `tests/test_modctx_pin.c` | the bound `[0, K)`, src/modctx.c:603 | the bound moved by one |
| `tests/test_text_r3r9.c` | R3 (printer, reader) and R9 (prec, digits) | old printer; old target |
| item 4 of the brief | nothing to write: the test is there | not applicable |

`tests/test_recon_canon.c` belongs to task 3 and is described there.

The new files are picked up by the Makefile without an edit: `make check` runs 46 programs where it
ran 42.

### Task 3: the six statements

Three of the six were deleted, one was already deleted by another lane, one is kept and pinned by a
new test, and the reasons are in `progress.md` before each deletion. `git diff --stat src/` says
`2 files changed, 3 deletions(-)`: the `fmpq_sub(diff, a, b);` at src/fball.c:1057, the
`fmpz_one(fmpq_denref(q));` at src/recon.c:135 and the `fmpz_one(fmpq_denref(c->q));` at
src/recon.c:234.

## The files written

* `Makefile`: the target `check-all`, `.PHONY`, the header comment, the help text (35 lines changed
  in all, 33 added and 2 removed, none of them in `check`).
* `tests/test_common_unsupported.c` (new, 6 tests, 48 checks).
* `tests/test_modctx_pin.c` (new, 7 tests, 5716 checks, 8 ms).
* `tests/test_text_r3r9.c` (new, 8 tests, 144 checks).
* `tests/test_recon_canon.c` (new, 5 tests, 139 checks).
* `src/fball.c`: one line deleted, nothing else.
* `src/recon.c`: two lines deleted, nothing else.
* `lanes/m1-leftovers/progress.md`, `lanes/m1-leftovers/redgreen.log`, this file.

No other file of the tree was written. No mutation run was made in this lane. No git command that
changes state was run; `git status`, `git diff`, `git log` and `git show` were read only.

## The checks, with the commands and the results

### Task 1

```
$ make -j2 check                          (before the change, the tree was clean)
check passed: all 42 test programs
real 0m22,037s

$ make check-all
check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest
exit 0
```

The six steps in the log of that run, in order: `== make check CC=cc SAN=0 INV=0` (line 1),
`== building the shared object` (line 533, `tests/test_exports.sh`),
`== build/libadelefeld.so is up to date ...` (line 738, `tests/test_julia.sh`),
`== python3 tools/mutate/selftest.py` (line 744), `== python3 tools/memcheck/selftest.py`.
Julia 1.12.5 is installed, so its step ran and was not skipped. The two tool self-tests both
printed `selftest: passed`.

The failing run, without touching the real script or the real Makefile: a copy of
`tests/test_exports.sh` under `build/scratchcheck/` with `echo "scratch copy: forced failure"; exit 1`
added after `set -u`, and a copy of the Makefile in which that one path is replaced.

```
$ make -f build/scratchcheck/Makefile.red check-all
scratch copy: forced failure
make: *** [build/scratchcheck/Makefile.red:123: check-all] Error 1
exit 2
```

The steps that ran in that run: `== make check CC=cc SAN=0 INV=0` (line 1) and
`== sh build/scratchcheck/fake_exports.sh` (line 532). The banners of steps 4, 5 and 6 and the line
`check-all passed` are nowhere in the log, so the target stopped at the first failure.

### Task 2

```
$ make build/test_common_unsupported && ./build/test_common_unsupported
6 tests, 48 checks, 0 failed checks, 0 failed tests          (green)
red, build/scratch/common, src/common.c:161 = `return ADF_OK;`
6 tests, 48 checks, 16 failed checks, 5 failed tests, exit 1

$ make build/test_modctx_pin && ./build/test_modctx_pin
7 tests, 5716 checks, 0 failed checks, 0 failed tests, real 0m0,008s   (green)
red, build/scratch/pin, `fmpz_add_ui(out, out, 1);` after src/modctx.c:603
7 tests, 5716 checks, 2847 failed checks, 7 failed tests, exit 1
  the old pin of the same bound on the same build: 239 checks, 76 failed, 1 failed test
second case, the line deleted instead of moved: the new pin 0 failed, the old pin 0 failed

$ make build/test_text_r3r9 && ./build/test_text_r3r9
8 tests, 144 checks, 0 failed checks, 0 failed tests          (green)
red, build/scratch/text, the old printer (tx_arb_printable returns 1)
8 tests, 144 checks, 12 failed checks, 3 failed tests, exit 1
  the old printer prints "(1e100001 +/- 1.1e99998 ; 0)" for "(9.99e100000 ; 0)" and the default
  reader gives ADF_LIMIT (10): the text and the status of the review
red, the same scratch, the old target and the old corpus
8 tests, 144 checks, 5 failed checks, 1 failed test, exit 1
```

Item 4 needed no change; the one line is in `progress.md`, section 2.4.

### Task 3

```
after each of the three deletions:  make -j2 check, exit 0, "check passed: all 45 test programs"

$ make build/test_recon_canon && ./build/test_recon_canon
5 tests, 139 checks, 0 failed checks, 0 failed tests          (green)
red, build/scratch/canon, `fmpq_canonicalise(q);` deleted from src/recon.c
5 tests, 139 checks, 25 failed checks, 3 failed tests, exit 1
```

### The four runs at the end

```
$ make clean && make -j2 check              real 0m25,124s
check passed: all 46 test programs
$ make clean && make -j2 check SAN=1        real 0m47,539s
check passed: all 46 test programs
$ make clean && make -j2 check INV=1        real 0m29,477s
check passed: all 46 test programs
$ make clean && make check-all              real 2m0,297s
check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest
```

The first `make check-all` of the four failed with exit 2 and the reason was the memory checker
self-test: it runs `tools/memcheck/check_uninit.py` over `tests/*.c` and requires no finding, and
`tests/test_modctx_pin.c` gave five, because its helper initialised a parameter with `fmpz_one`
rather than with a call whose name contains `_init`. The three tests concerned now call
`fmpz_init(K)` themselves and the helper writes with `fmpz_set_ui(K, 1)`. After that

```
$ python3 tools/memcheck/check_uninit.py tests/test_modctx_pin.c tests/test_recon_canon.c \
      tests/test_common_unsupported.c tests/test_text_r3r9.c
# use-before-init: 0
# clear-before-init: 0
# init-without-clear: 0
exit 0
```

and `make check-all` passes. The table above is the state after that change; the three `make check`
runs were repeated after it as well.

Line length: no line of the four new test files, of the new part of the Makefile, of
`progress.md` or of this file is longer than 116 characters (checked with awk).

## What is not done

* No mutation run (`make mutate` is forbidden in this lane), so the three deleted statements were
  not re-measured with the mutation tool. The reasons are from the code, as the brief asks, and each
  deletion was followed by a full `make -j2 check`.
* Task 2.2 does not pin the bound against the *deletion* of src/modctx.c:603, only against the
  bound being moved. On FLINT 3.0.1 `fmpz_multi_CRT_precomp(out, ..., 0)` returns a value of
  `[0, K)` by itself, so no test of the value can see the difference; this is what the closure judge
  of the contexts review found (lanes/m1-closure-contexts/report.md:133-134) and it is confirmed
  here by deleting the line in the scratch copy: the old pin and the new pin are both green.
* Task 2.3, R9: the prec = 2 half of the finding is not closed in the tree, and this lane cannot
  close it (see the findings below). The test measures it and says so; it does not assert it.
* The scratch trees under `build/scratch/` were removed at the end; their commands and outputs are
  in `redgreen.log`.
* No new vector file was generated (`tests/ref/vectors/` untouched): the four new test files need
  none, they compute their own expected values.

## Sources pending

* `[source pending: FLINT 3.0.1 documentation of fmpz_fdiv_r under refs/]` (the range of the
  function that carries the bound of task 2.2). The repair lane cites the same gap.
* `[source pending: the FLINT 3.0.1 source of arb_get_interval_fmpz_2exp]`, already marked in
  src/recon.c; the exact form of the bounds is what task 3 pins, and the test of it needs no
  source beyond the comment of the function.
* Everything else the new tests use is on disk: refs/src/flint-3.0.1/fmpq.rst:21-26 (canonical
  form), /usr/include/flint/fmpq.h:28-32 (`fmpq_init` sets 0/1),
  refs/src/flint-3.0.1/fmpq.rst:1362-1365 and arb.rst:468-470 as cited by the sources themselves.

## Findings against the specification

1. **R9 of the text review is only half closed, and the closure judgement overstates it.**
   `docs/reviews/m1/text/closure.md:15` says of R9: "Four accepted texts with control-byte pairs
   cover prec 2/191 and digits 1/30." Measured on the committed corpus of
   `tests/fuzz/corpus/text/` (283 files), read as the target reads it, 90 of the files are
   accepted adele or cadele texts; over those the reached prec values are 65 of the 190 of the
   range 2..191 and **the value 2 is not among them**, while all 30 digits values, including 1,
   are reached. The value prec = 2 is reached only by inputs that the rat and fball paths of the
   target accept, and those two paths do not take a prec (text.h:132-147), so for the adele and
   cadele round trips of the target the prec half of R9 is still open. The new test prints the
   measurement on every run. The fix is one control byte in front of one accepted adele text in
   the corpus, a file this lane does not own.
2. **Two of the six statements of task 3 were already gone.** The two dead guards of
   `adf_fball_prec_at` (lines 420 and 422 of `lanes/m1-testgaps/report.md`) were removed by commit
   2b804c5 (lane m1-repair-adele) together with the two `#pragma weak` declarations. The report of
   the test-gap lane is out of date on those two lines.
3. **The line numbers of the brief are out of date.** `tests/test_recon.c:1313` is
   `an_adele_with_an_infinite_real_ball_is_rejected` at line 1362 today, and the report numbers of
   `lanes/m1-testgaps/report.md` section 8 do not match `src/recon.c` and `src/fball.c` any more;
   the statements were found by their text and the present lines are in `progress.md`.
4. **A test of the public result cannot pin the canonicalise of the bounds.** The public output of
   `adf_adele_reconstruct` has the same value with and without `fmpq_canonicalise(q);`, and the
   reason is in the code (the bounds are read by `fmpq_cmp`, `adf_rat_sub`, `adf_rat_div`,
   `fmpz_cdiv_q` and `fmpz_fdiv_q`, all of which are homogeneous, and the value written out is
   built from `a`, `N` and `kmin`). A test of the canonical form of that line therefore has to
   reach the static helper, which `tests/test_recon_canon.c` does by including `src/recon.c`, the
   pattern of `tests/test_common.c:22`. This is worth a line in `docs/workflow.md`: a "dead" line
   of that kind can only be pinned from inside the file.
5. Nothing in `docs/SPEC.md` was found to be wrong. No mathematical claim was refuted; the four
   items above are about coverage, about line numbers and about a limit of what a test can see.
