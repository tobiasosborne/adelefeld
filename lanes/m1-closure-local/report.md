# Lane m1-closure-local report

Verdicts: CLOSED 2; CLOSED WITH EDIT 1; OPEN 0; SETTLED BY DECISION 1. NO BLOCKER OPEN.
The review has no new finding. R4 needs the exact test edit written in `closure.md`.

## Work done and files written

I reread the four findings, the cap repair report, the invariant report, the relevant contracts, and the code.
I reran all four unchanged C reproducers. I built release, invariant, and ASan/UBSan libraries in `build-closure`.
I tested admitted pointers after M1-D2, local caps against a rational oracle, and the R4 fixture under the
debug borrow count. I reversed three test or code changes in scratch copies to check the new tests.

- `docs/reviews/m1/local/closure.md`
- `docs/reviews/m1/local/closure-checks/cap_oracle.py`
- `docs/reviews/m1/local/closure-checks/probe_closure.c`
- `docs/reviews/m1/local/closure-checks/invalid_fixture_closure.c`
- `docs/reviews/m1/local/closure-checks/revert_guard.py`
- `docs/reviews/m1/local/closure-checks/drop_neg_check.py`
- `docs/reviews/m1/local/closure-checks/revert_fixture.py`
- `lanes/m1-closure-local/report.md`

No production file or old reproducer was changed. Scratch binaries and logs were made only in
`build-closure`, which was removed after the checks.

## Commands and results

All commands ran from the worktree root. `cc` commands used `-Iinclude`, `-lflint -lgmp -lm`, and the
matching build's static library. Invariant commands added `-DADF_CHECK_INVARIANTS -pthread`.
Sanitizer commands added `-fsanitize=address,undefined -fno-omit-frame-pointer` and ran with
`ASAN_OPTIONS=detect_leaks=0`.

| Command | Result |
|---|---|
| `free -g` | 23 GB available before the builds. |
| `make clean BUILD=build-closure && make -j2 BUILD=build-closure` | exit 0; 14 library objects. |
| `make -j2 BUILD=build-closure/inv INV=1 all` | exit 0; 14 library objects. |
| `make -j2 BUILD=build-closure/san SAN=1 all` | exit 0; 14 library objects. |
| Build and run unchanged `checks/cap_local.c` | exit 0; local and global statuses each `0,0,0,0,0`. |
| Build and run unchanged `checks/invalid_fixture.c` | exit 0; old fixture `L=0`, assertions pass=1. |
| Run unchanged `checks/invariant_check.c` with invariant library | exit 134; named `adf_fball_neg`, `x`. |
| Build and run unchanged `checks/probe.c bad-context` against sanitizer library | exit 1; ASan overflow. |
| `python3 checks/oracle.py --probe build-closure/probe-inv --mode small` | 900 requests, 0 mismatches. |
| `python3 checks/oracle.py --probe build-closure/probe-inv` | 40,208 requests, 0 mismatches. |
| `python3 checks/oracle.py --probe build-closure/probe` | 40,208 requests, 0 mismatches. |
| `python3 closure-checks/cap_oracle.py build-closure/probe` | 1,440 cases, 0 mismatches. |
| `python3 closure-checks/cap_oracle.py build-closure/probe-inv` | 1,440 cases, 0 mismatches. |
| Run `closure-checks/probe_closure.c` in 3 builds | 7 checks each, 0 failures. |
| Build and run `closure-checks/invalid_fixture_closure.c` in 3 modes | 16 checks each, 0 failures. |
| `make -j2 BUILD=build-closure check` | exit 0; 42 programs passed. |
| `make -j2 BUILD=build-closure/inv INV=1 check` | exit 2; 37 passed, 5 known programs failed. |
| `make clean BUILD=build-closure` after the checks | exit 0; `build-closure` absent. |
| Run `build-closure/inv/test_invariants` | 9 tests, 5,376 checks, 0 failures; 1,432 aborts. |
| Run `build-closure/inv/test_invariants_lifetime` | 13 tests, 116 checks, 0 failures. |
| Run `build-closure/inv/test_cap_local` | 6 tests, 253 checks, 0 failures. |
| Run `build-closure/test_cap_local` | 6 tests, 253 checks, 0 failures. |
| Run `build-closure/inv/test_fball` | exit 134 after R4 test passed; known `identical_local_guard` abort. |
| Valgrind with `--error-exitcode=17 --leak-check=full` on fixture | 0 errors; 12 allocs, 12 frees. |

The unchanged reproducer source paths in this table are under `docs/reviews/m1/local/checks/`.
The new source paths are under `docs/reviews/m1/local/`. Every listed C source was compiled into
`build-closure` with `cc -std=c11 -O1` or `-O2 -g -Wall -Wextra -Werror` and the flags above.

Scratch reversal commands and results:

- `python3 closure-checks/revert_guard.py build-closure/old_cap.c`, then compile that object, archive it with
  the other release objects, build `tests/test_cap_local.c`, and run: exit 1; 89 failed checks in 3 tests.
- `python3 closure-checks/drop_neg_check.py build-closure/old_fball.c`, then compile that invariant object,
  archive it with the other invariant objects, build `tests/test_invariants.c`, and run: exit 1;
  43 failed checks in 2 tests, including both R3 assertions.
- `python3 closure-checks/revert_fixture.py build-closure/old_test_fball.c`, then build and run it against
  the release library: exit 1; 2 failed checks in 1 test, both in `canonicalise_raw_and_domains`.

Read-only inspection commands were `cat` on `CLAUDE.md`, the three lane rule files, the local review, and
the two repair reports; `rg -n` on `docs/SPEC.md`, `docs/PLAN.md`, headers, conventions, tests, and source;
`sed -n` and `nl -ba` on those files and the reproducers; `rg --files` on the review checks; `free -g`;
and `pgrep -af` for builds. Each completed with exit 0. `awk 'length($0)>116'` found 1 long line in the
first closure draft; after correction it found 0. `ast.parse` accepted 4 of 4 new Python files.
The first debug-log parser falsely attributed the final `check FAILED` line to the last test program.
A corrected parser counted 42 programs and exactly the five known failures.

## Not done

I did not repair the five older tests that step outside the debug contract. I did not review performance,
allocation failure, all exported functions independently, or all thread interleavings. I did not review
the other milestone 1 closure lanes. M1-D10 and M1-D11 remain proposed decisions.

## Sources pending

No new source is pending for this closure. The inherited Haar-measure source pending in
`docs/reviews/m1/local/review.md` was outside this check and remains pending.

## Findings against the specification

None found in this scope.
