# f-fixture1 report

Date: 2026-10-02. Base commit: 995960d6c82c97d83eed43fd8c5383d04c284668.

## Result

Items 1 to 3 are complete. The separate old-code fixture has 465 rows and 534848 bytes.
The new stored comparison detects all six actual faults from the referee's table.
The unchanged scratch control has 0 failed checks.

The isolated new comparison takes 0.95 s: 18602 checks, 0 failures.
The final normal test_lfunc takes 3.82 s: 13 tests, 473031 checks, 0 failures.
The final sanitized test_lfunc takes 7.08 s: the same 473031 checks, 0 failures,
and 0 ASan/UBSan diagnostics with ASAN_OPTIONS=detect_leaks=0.

Neither required full acceptance suite completed within the three-minute cap.
check-all stopped at test_resid_rest; SAN=1 check stopped at test_dump_limits.
Both exited 124 after 180.00 s. Their completed programs had 0 failed checks.
These are incomplete runs, not passes. Each full acceptance command was invoked once.

src/lfunc.c, docs/SPEC.md, and the original stored.jsonl match their starting SHA256 hashes.
The only tracked source edit is 33 added lines in tests/test_lfunc.c; existing tests stay unchanged.
No library optimization, benchmark, git state change, bd command, or package installation was done.

## Fixture and provenance

The generator was written first: lanes/f-fixture1/gen_stored_large.py, seed 20261002.
It writes tests/ref/vectors/f-slice5/stored_large.jsonl in the existing f, N, st, x, y row format.
All expected values come from src/lfunc.c at commit 1cf0e42, obtained with git show.
build.sh compiles that file with the three public symbols renamed to old_lball_exp,
old_lball_log, and old_lball_Log, as in the referee's build.sh.

The old source SHA256 is:

    e39a78444a4123b3875a16f6b9c3aa06a8edc61dfcf5647691e3e4eb763dcfdd

stored_probe.c calls only those old symbols for expected outputs and alias checks.
All 930 calls completed. Each of 465 rows passed the probe's alias, unchanged-output,
and canonical-result checks. Every new row has status OK and a ball result.
The status assertion is still present in the C test; the old fixture retains its other statuses.

Generation took 30.58 s. All 138 calls at the word prime together took 18.384 s.
The seven prime batches each run under timeout 60, inside the generator's timeout 180.

The inputs include 220 log, 217 Log, and 28 exp rows; 241 exact inputs and 224 balls.
There are 168 Log inputs with negative valuation, 24 at each prime.
Families include exact rational units, dense balls, the powered Log path, sign reduction at 2,
and requests both above and below the precision available from a ball.
All 465 saved inputs agree with a fresh deterministic construction from the seed.

## Distribution and actual routes

The route rule is read from src/lfunc.c:535-604:

1. Compute the effective output precision K, including the ball's exponent and valuation.
2. For the direct principal-unit path, T is count_log(p, K, vz).
   For the powered path, the working precision uses count_log(p, K, 1).
3. W = K + floor_log(max(1, T), p), using that count for the working precision.
4. After the preceding shortcuts, bits(p^W) <= FLINT_BITS - 2 selects log_sum_word.
   Otherwise K <= 64 selects log_sum (F8); K > 64 selects log_balanced (F9).

No estimate based on e(2K) is used. FLINT_BITS is read from the compiled probe and checked to be 64.

| Prime | Rows | Word | F8 | F9 | Shortcut | Exp | K=39..64 | K=65..3000 | Deep F9 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 2 | 66 | 8 | 30 | 24 | 0 | 4 | 38 | 24 | 17 |
| 3 | 66 | 3 | 35 | 24 | 0 | 4 | 32 | 24 | 17 |
| 5 | 66 | 3 | 35 | 24 | 0 | 4 | 32 | 24 | 17 |
| 7 | 66 | 3 | 35 | 24 | 0 | 4 | 32 | 24 | 16 |
| 11 | 66 | 3 | 35 | 24 | 0 | 4 | 32 | 24 | 17 |
| 65537 | 66 | 3 | 35 | 24 | 0 | 4 | 32 | 24 | 17 |
| 18446744073709551557 | 69 | 0 | 35 | 24 | 6 | 4 | 32 | 24 | 17 |
| Total | 465 | 23 | 240 | 168 | 6 | 28 | 230 | 168 | 118 |

The two K-range columns count log and Log rows only. Deep F9 means K >= 200 and
entry vz = c, with c = 1 at odd primes and c = 2 at 2. These entries take several doubling steps.
At every prime, both log and Log occur on F8 and F9.
At the word prime all 24 F9 rows have K <= 1499; at the other primes the maximum is 3000.

Each prime has at least three log/Log rows at EACH of K=64 and K=65.
There are also three rows on EACH side of the tagged-word boundary for the family vz=c:

| Prime | Largest K fitting a tagged word | Next K |
|---|---:|---:|
| 2 | 57 | 58 |
| 3 | 36 | 37 |
| 5 | 24 | 25 |
| 7 | 21 | 22 |
| 11 | 16 | 17 |
| 65537 | 3 | 4 |
| 18446744073709551557 | 0 | 1 |

At the word prime, K=0 and K=1 return through shortcuts, before any modulus is formed.
Three additional K=2 rows exercise the first actual F8 entry there.

route_audit.py instruments a SCRATCH copy of the C route selection and records route, K, W, and vz.
All 862 actual entries match the Python predictions, including the alias calls:
46 word, 480 F8, 336 F9, 0 mismatches.
This audit counts the source's actual branch choices; it does not infer a branch merely from requested N.

## Red, green, and planted faults

The new comparison precedes the existing stored test, so the crashing valuation fault reaches it first.
The first red run was f8_digit_K40: 346 new-fixture failures, 0 old-fixture failures.
The following green run used the unchanged library: 13 tests, 473031 checks, 0 failures, 3.99 s.

The original referee tool was run from the lane-owned build/harness directory.
Its hardcoded lanes/f-review4/build path is a symlink to this lane's build directory.
src, include, tests, and the referee scripts are read-only symlinks to their real locations.
No real f-review4 file was created or changed.

Each actual fault used this command, with NAME replaced by the table entry:

    timeout 180 python3 -B lanes/f-review4/faults.py ../faults NAME

The shell also set ulimit -c 0 and ulimit -v 2097152.
Each script exited 0 after reporting the expected faulty-test result.
The tool unconditionally builds its probe and runs oracle.py in small mode.
No exhaustive, random, large, or largeball oracle mode was run.

| Fault/control | New stored failures | Old stored failures | Whole-test failures | Test result | Elapsed s |
|---|---:|---:|---:|---|---:|
| f8_digit_K40 | 346 | 0 | 387 | exit 1 | 45.93 |
| f9_inverse | 242 | 10 | 520 | exit 1 | 46.54 |
| f9_inverse_bigp | 42 | 0 | 44 | exit 1 | 65.58 |
| f9_valuation | 4 before crash | not reached | incomplete | signal 11 | 5.84 |
| f9_tree | 336 | 18 | 700 | exit 1 | 64.32 |
| f8_digit | 390 | 650 | 1708 | exit 1 | 65.44 |
| control_unmodified | 0 | 0 | 0 | exit 0 | 64.71 |

The valuation fault fails the value and alias assertions at rows 33 and 34, then crashes at row 35.
Its count is a prefix of the new comparison, not a completed count over 465 rows.
The referee tool loses that count because no end-of-test summary is printed after a crash.
A direct rerun of its already-built test captured all four assertion lines:

    timeout 60 lanes/f-fixture1/build/faults/f9_valuation/test_lfunc

That rerun exited 139 after 0.29 s; core dumps and the 2 GiB memory bound remained in force.
Evidence: fault-f9_valuation-assertions.log. No assertion or fixture row was removed to avoid the crash.

control.py loads the unchanged referee harness and adds an identity text replacement.
It therefore builds a byte-identical source copy through the same archive and test path.
From build/harness its command was timeout 180 python3 -B ../../control.py.
cmp of src/lfunc.c and build/faults/control_unmodified/lfunc.c exited 0.
The control has 13 tests, 473031 checks, 0 failures.
The non-fault control_lower_bound variant was not rerun; the required unmodified control was used instead.

The referee's small oracle checks its own inputs, not all rows of this new fixture:

| Fault/control | Small inputs completed | Differences from old | Oracle failures | Point failures |
|---|---:|---:|---:|---:|
| f8_digit_K40 | 7440 | 1621 | 1621 | 1004 |
| f9_inverse | 7440 | 824 | 824 | 758 |
| f9_inverse_bigp | 7440 | 160 | 160 | 128 |
| f9_valuation | incomplete | not counted | not counted | not counted |
| f9_tree | 7440 | 1408 | 1408 | 896 |
| f8_digit | 7440 | 3272 | 3272 | 2012 |
| control_unmodified | 7440 | 0 | 0 | 0 |

The valuation-fault probe exits 1. Other small-mode processes exit 0 even when their reported
mismatch counters are nonzero; the numbers above, not the process exit alone, describe those checks.

## Commands and checks

All test programs and scripts ran under timeout, including the enclosing timeout of the referee tool.
Builds used at most two jobs. At most two single-threaded lane processes overlapped.
Final suites and direct final tests were restricted to CPUs 0 and 1.
There was one initial lane build, incremental test builds, and one clean for the final normal suite.
The sanitized build uses build/san to prevent reuse of normal objects without another clean.

For compact command notation below, B means lanes/f-fixture1/build.

- timeout 180 make -j2 BUILD=B B/libadelefeld.a B/support/jsonl.o B/support/golden.o:
  exit 0; build.log.
- timeout 120 sh lanes/f-fixture1/build.sh:
  exit 0; renamed old source and probe compiled; probe-build.log includes the old-source SHA256.
- timeout 30 python3 -B lanes/f-fixture1/gen_stored_large.py --plan:
  exit 0; 465 inputs and the route counts above; plan.log.
- timeout 180 python3 -B lanes/f-fixture1/gen_stored_large.py:
  exit 0; 465 rows, 534848 bytes, 930 old-code calls; generate.log.
- timeout 10 sh lanes/f-fixture1/prepare_faults.sh:
  exit 0; the lane-owned scratch root was created.
- timeout 60 make -j2 BUILD=B B/test_lfunc:
  exit 0; test-build.log.
- timeout 60 B/test_lfunc:
  exit 0; 13 tests, 473031 checks, 0 failures; 3.99 s; green.log.
- timeout 90 python3 -B lanes/f-fixture1/route_audit.py:
  exit 0; 862 C route entries, 0 mismatches; route-audit.log and route-trace.log.
- The isolated-test compilation below exited 0; stored-only-build.log.

    timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude -Itests \
      lanes/f-fixture1/stored_large_only.c B/support/golden.o B/support/jsonl.o \
      B/libadelefeld.a -lflint -lgmp -lm -o B/stored_large_only

- timeout 60 B/stored_large_only:
  exit 0; 18602 checks, 0 failures, 0.95 s; stored-only-green.log.
- The six referee fault commands, the control command, and the crash-prefix rerun:
  results and elapsed times are in the tables above; fault-*.log.
- timeout 10 python3 -B lanes/f-fixture1/check_artifacts.py:
  exit 0; 465 inputs matched, 534848 bytes, 0 source/prose lines over 116; artifacts.log.
  This was repeated after progress-note edits to check their line lengths.
- An initial inline JSON/statistics audit exited 1 at Python's 4300-digit conversion limit.
  check_artifacts.py sets sys.set_int_max_str_digits(0), as the generator already did; the rerun passed.
- timeout 10 sha256sum -c lanes/f-fixture1/unchanged.sha256:
  3 matches, 0 mismatches, both before and after acceptance runs; scope-final.log.
- git diff --check:
  exit 0; git diff --numstat gives 33 additions and 0 deletions in tests/test_lfunc.c.
  git status shows no changed tracked path outside that file.
- The timeout 10 inline Python log-summary extractions exited 0.
  They count completed per-program summaries only; final-summary.log.

The final normal acceptance command, invoked once:

    timeout 180 taskset -c 0,1 sh -c 'make clean && make -j2 check-all'

Exit 124, 180.00 s. Before interruption, 46 programs completed 38613598 checks with 0 failures.
The interrupted program was build/test_resid_rest. The driver, exports, Julia, and tool self-tests
were not reached by check-all. Last line of check-all.log:

    make: *** [Makefile:123: check-all] Terminated

The specifically requested final lfunc run:

    timeout 60 taskset -c 0,1 build/test_lfunc

Exit 0, 3.82 s. Last line of final-lfunc.log:

    13 tests, 473031 checks, 0 failed checks, 0 failed tests

The final sanitizer acceptance command, invoked once:

    ASAN_OPTIONS=detect_leaks=0 timeout 180 taskset -c 0,1 \
      make -j2 check SAN=1 BUILD=build/san

Exit 124, 180.00 s. Before interruption, 15 programs completed 428223 checks with 0 failures.
The interrupted program was build/san/test_dump_limits. The suite had not reached test_lfunc.
There were 0 ASan/UBSan diagnostics in its log. Last line of check-san.log:

    make: *** [Makefile:102: check] Terminated

The already-built sanitized lfunc test was then checked directly, without rebuilding or rerunning the suite:

    ASAN_OPTIONS=detect_leaks=0 timeout 60 taskset -c 0,1 build/san/test_lfunc

Exit 0, 7.08 s, 0 ASan/UBSan diagnostics. Last line of san-lfunc.log:

    13 tests, 473031 checks, 0 failed checks, 0 failed tests

Elapsed times come from /usr/bin/time. Final command times and exit codes are retained in the matching
.time files. Normal and sanitized test_lfunc both read EVERY row of both stored fixtures.

## Files written

Owned repository files:

- tests/test_lfunc.c: the added stored_large_before_optimisation test.
- tests/ref/vectors/f-slice5/stored_large.jsonl: the new 465-row fixture.

Lane sources and inputs:

- gen_stored_large.py, stored_probe.c, build.sh, stored_large.in.
- route_audit.py, stored_large_only.c, check_artifacts.py.
- prepare_faults.sh, control.py.
- progress.md, unchanged.sha256, this report.md written once after the work.

Lane evidence:

- build.log, probe-build.log, plan.log, generate.log, test-build.log, green.log.
- route-audit.log, route-trace.log, stored-only-build.log, stored-only-green.log.
- fault-f8_digit_K40.log, fault-f8_digit.log, fault-f9_inverse.log, fault-f9_inverse_bigp.log.
- fault-f9_tree.log, fault-f9_valuation.log, fault-f9_valuation-assertions.log.
- fault-control_unmodified.log, artifacts.log, scope-final.log, final-summary.log.
- check-all.log/.time, check-san.log/.time, final-lfunc.log/.time, san-lfunc.log/.time.
- build/: private objects, archives, binaries, traced source, old source, and fault copies.

Only the final acceptance work used repository-root build/ and build/san/.
The harness-maintained lane.log was not edited by this lane.

## What is not done

- Completion of check-all and SAN=1 check: both single permitted invocations reached timeout 180.
  The orchestrator must complete the full acceptance suites separately.
- Optional item 4, MINOR 2: no library edit and no benchmark. The reported small-N regression remains open.
- clang, INV, default LeakSanitizer, a mutation sweep, or a long fuzz run.
- An independent exact oracle calculation for every newly stored large row.
  The mandated expected answers come from the old code; the referee tool used only its small mode.
- A second full old-code regeneration: seed/input reconstruction and the C route audit were checked,
  but the generator's optional --check mode was not run.

## Sources pending

None introduced by this test-only change.
The old source commit, actual current source, and the on-disk reviewer scripts are the ground truth here.
No new mathematical statement is attributed to an external source from memory.
The pre-existing F2 source-pending issue mentioned in f-slice5/report.md is outside this lane.

## Findings against the specification

None. No counterexample to docs/SPEC.md or the reviewed numerical implementation was found.

## Clarifications against the brief and review wording

1. At 2, the requested F9 entry v(z)=1 is impossible.
   src/lfunc.c:535-547 chooses the sign of a so that s*a=1 modulo 4.
   Consequently z=s*a-1 has v_2(z)>=2 before route selection.
   F9 itself assumes v>=c, with c=2 at 2. The fixture uses v(z)=2 there.
2. The actual working precision is K+e(T), not K+e(2K).
   src/lfunc.c:555-572 selects T from count_log, including the powered-path lower bound.
   The latter expression in the brief is an upper-bound estimate, not the branch rule.
3. At p=2^64-59 no positive power fits the tagged-word range.
   Already p has 64 bits, above FLINT_BITS-2=62; every p^W with W>=1 is at least p.
   Thus the formal boundary requests K=0,1 take shortcuts; K=2 is the first actual F8 route.
   They are reported as six shortcuts and three F8 rows, not as a fabricated word-route crossing.
4. The brief calls f9_inverse an old-fixture survivor.
   Review section 5 actually gives it 10 failed checks, reproduced here.
   The survivor is f9_inverse_bigp: 0 old-fixture failures, now 42 new-fixture failures.

