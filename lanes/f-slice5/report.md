# f-slice5: cost of the series at a prime

Date: 2026-09-30.

## Result

The measured word-prime families meet the two-second target. At p=2^64-59 and N=10000:

| Input and function | Minimum s | Median s | Maximum s |
|---|---:|---:|---:|
| exp(p) | 0.078779516 | 0.079072621 | 0.081512526 |
| log(1+p) | 0.042181003 | 0.043326163 | 0.046383188 |
| Log(2) | 0.755331627 | 0.770731474 | 0.848686014 |

All 2000 stored cases agree in status, centre, valuation, exponent, prime and exact flag.
Each case also agrees when the output aliases the input. The final stored test has 80002 checks and 0 failures.
The final reviewer suites have 0 differences. The benchmark matrix has 96 full-residue FLINT comparisons
and 0 differences. Its --run entry point has another 3 comparisons and 0 differences.

The default LeakSanitizer run could not pass under ptrace. The clean SAN rerun used
ASAN_OPTIONS=detect_leaks=0; address and undefined-behaviour checks passed. Valgrind checked the stored
comparison separately: 0 errors, 0 bytes and 0 blocks in use at exit. This limitation is not hidden as a pass
of the default LeakSanitizer mode.

## Changes and proofs

1. F8 proves that degree k needs H_k=K-k v+v_p(k) digits of the unit power of z/p^v.
   The recurrence uses the nonincreasing upper bound K-k v+floor(log_p k). The sum retains K digits.
2. F8 also proves unit division by a word denominator. Choose j so r+j Q is divisible by the denominator,
   then divide exactly. There is no full-power integer inverse per term in this route.
3. The existing odd-prime powered route is retained. One measured power took 0.657397564 s; the
   Teichmueller lift took 3.077587530 s. The power is formed once at the original working precision.
4. Methods 1 and 2 missed the target. F9 then proves a factorisation into principal units of increasing
   valuation and an exact binary-splitting tree for their shorter log sums. Residual inverses need K-m
   digits after removing m digits. Exact tree arithmetic loses no working digits through denominators.
5. The original loop is retained when the formed working modulus fits the tagged-word range.
   The test uses fmpz_bits(P), not W bits(p), which can overestimate its size.
   The larger evaluators are kept out of line with FLINT_STATIC_NOINLINE.

All domain checks, exact shortcuts, output forms, image exponents and F7 status limits remain the same.
The original global W guard remains checked even where F8/F9 need less precision. Exp retains its original
Horner evaluator. No FLINT padic call was added to the library. No declaration or representation was changed.

The new proofs are at the end of docs/api-1f4.md. F8 applies to v>=1, including the raw log series at 2.
F9 uses the principal disc v>=c after the existing torsion reduction. The tree and factor proofs are our own.
FLINT documents rectangular log splitting in refs/src/flint-3.0.1/padic.rst:509-516 and balancing chunks
against valuation for exp in :440-447. No claim about the undocumented internals of padic_log_balanced is made.

## Baseline preservation and red run

Saved src/lfunc.c before implementation changes as baseline.c. Its SHA256 is

e39a78444a4123b3875a16f6b9c3aa06a8edc61dfcf5647691e3e4eb763dcfdd

The generator called the unmodified probe-before executable. It stored 2000 rows, 379622 bytes.
Seed: 930505. Functions: 652 exp, 697 log, 651 Log. Prime counts: 334 at 2 and 3, and 333 at each of
5,7,11,2^64-59. There are 970 exact and 1030 ball inputs. Log includes negative valuations.
Requested N is drawn from a distribution bounded by 3000; the observed maximum is 2850.
Most requests use N=-2..36; the first 50 include larger requests. Only the first two candidate requests
at the word prime use the larger range. This distribution keeps complete stored centres below 1 MB.
The seed and distribution are in gen_stored.py; stored.in records all actual inputs.

The added R1 test forks one child per reviewer input and uses alarm(5). The constant and its purpose
are stated in the test. Before changes: 12 tests, 454429 checks, 2 failed checks, 1 failed test, exit 1.
Both children stopped with signal 14. The stored comparison itself passed before any implementation change.

## Measurements, one method at a time

| Stage | log(1+p), N=10000, s | Log(2), N=10000, s | Stored differences |
|---|---:|---:|---:|
| Entry implementation | 74.823288153 | >120 | 0 |
| Decreasing term modulus | 21.501793087 | five-second test failed | 0 |
| Word unit division | 22.529920997 | >120 | 0 |
| Initial balanced factors/tree | 0.082623155 | 1.777919119 | 0 |
| Final source | 0.043326163 | 0.770731474 | 0 |

An intermediate contended run had a Log median of 2.093228368 s. Its clock probes were not usable for
cycle conversion. It is retained in the intermediate logs. The residual-inverse precision was then reduced.
The later paired microbenchmark found a 20-28% regression at p=2,N=32 because the initial word guard used
an upper bound for modulus size. Using the actual size removed that regression.

The final paired microbenchmark uses one process, alternating old/new order, five trials, reused outputs,
p=2,3,5,2^64-59 and N=8,16,32. It has 36 comparisons and median new/old ratios 0.385 to 1.032.
The largest increase is Log at 5,N=8: 729.445 to 752.496 ns. Old min/max: 713.635/750.618 ns;
new min/max: 745.122/766.710 ns. The trial ranges overlap. A small regression cannot be ruled out.
Full values are in small-rate-final.log. The earlier single-call and paired runs are also retained.

### Where the entry implementation spends time

Private copies instrument the saved source, not the library. These are partial prefixes on cpu 4 with
shared work, not the speed-table baseline. Both profile programs timed out, with exit 124.

| Input | Terms | Power/reduce s | p division s | Unit inverse s | Term product s | Sum/reduce s |
|---|---:|---:|---:|---:|---:|---:|
| log(1+p) | 5000 | 0.068221 | 0.450437 | 4.209618 | 58.602675 | 86.309951 |
| Log(2) | 2000 | 58.400745 | 0.238840 | 0.613780 | 19.448258 | 39.102594 |

The first ran under timeout 180, the second under timeout 120. The timed operation totals include shared-core
waiting. The term products and second sum reduction dominate the first prefix; power/reduction dominates
the second. My initial progress claim that inverses dominated was wrong for these measured prefixes.

### Full before/after table

Times are seconds. After values are medians of 3 trials, or 5 at the word prime, labelled big below.
Before values have 3 trials where completed; the remaining baseline at 5 and the word prime has 1 trial.
The two partial rows at 3 contain completed first calls, not invented medians. >180 and >120 mean no
completed call was observed within those deadlines. Exp's evaluator is unchanged; clock/load differences
can change its measured time. The before timings are provisional because some runs shared cpu 2.

| p | N | f | before s | after s | FLINT s | mul mod s | after / FLINT | after / mul |
|---|---:|---|---:|---:|---:|---:|---:|---:|
| 2 | 2000 | exp | 0.000611 | 0.000419 | 0.000106 | 0.000001 | 3.971 | 489.6 |
| 2 | 2000 | log | 0.001194 | 0.000096 | 0.000201 | 0.000001 | 0.478 | 126.9 |
| 2 | 2000 | Log | 0.001199 | 0.000110 | 0.000199 | 0.000001 | 0.555 | 145.6 |
| 2 | 10000 | exp | 0.012255 | 0.007785 | 0.001024 | 0.000008 | 7.600 | 1002.3 |
| 2 | 10000 | log | 0.041789 | 0.000808 | 0.002078 | 0.000008 | 0.389 | 96.9 |
| 2 | 10000 | Log | 0.041874 | 0.000807 | 0.002056 | 0.000008 | 0.393 | 103.9 |
| 2 | 100000 | exp | 1.464158 | 0.879675 | 0.038472 | 0.000220 | 22.865 | 4007.5 |
| 2 | 100000 | log | 10.099534 | 0.030524 | 0.089805 | 0.000230 | 0.340 | 132.7 |
| 2 | 100000 | Log | 7.613716 | 0.031088 | 0.086852 | 0.000233 | 0.358 | 133.6 |
| 3 | 2000 | exp | 0.001854 | 0.002069 | 0.000641 | 0.000005 | 3.225 | 407.1 |
| 3 | 2000 | log | 0.005491 | 0.000367 | 0.000837 | 0.000005 | 0.438 | 75.2 |
| 3 | 2000 | Log | 0.005502 | 0.000368 | 0.000849 | 0.000005 | 0.434 | 75.7 |
| 3 | 10000 | exp | 0.041531 | 0.046585 | 0.008563 | 0.000051 | 5.440 | 912.1 |
| 3 | 10000 | log | 0.415525 | 0.003837 | 0.009743 | 0.000051 | 0.394 | 75.0 |
| 3 | 10000 | Log | 0.369513 | 0.003865 | 0.009670 | 0.000050 | 0.400 | 77.1 |
| 3 | 100000 | exp | 8.718384 | 4.800843 | 0.288307 | 0.001365 | 16.652 | 3517.5 |
| 3 | 100000 | log | 88.666025 (partial run) | 0.115025 | 0.329452 | 0.001427 | 0.349 | 80.6 |
| 3 | 100000 | Log | 110.166717 (partial run) | 0.118666 | 0.346969 | 0.001425 | 0.342 | 83.3 |
| 5 | 2000 | exp | 0.001477 | 0.001350 | 0.000396 | 0.000008 | 3.406 | 161.8 |
| 5 | 2000 | log | 0.023369 | 0.000435 | 0.001207 | 0.000009 | 0.361 | 50.7 |
| 5 | 2000 | Log | 0.033095 | 0.000478 | 0.001208 | 0.000009 | 0.396 | 54.1 |
| 5 | 10000 | exp | 0.066307 | 0.030489 | 0.004647 | 0.000095 | 6.561 | 321.6 |
| 5 | 10000 | log | 1.175641 | 0.004338 | 0.013404 | 0.000091 | 0.324 | 47.7 |
| 5 | 10000 | Log | 1.623544 | 0.004882 | 0.013665 | 0.000089 | 0.357 | 54.6 |
| 5 | 100000 | exp | 6.717748 | 3.008924 | 0.169355 | 0.002142 | 17.767 | 1405.0 |
| 5 | 100000 | log | 153.062729 | 0.119474 | 0.428286 | 0.002242 | 0.279 | 53.3 |
| 5 | 100000 | Log | >180 | 0.130957 | 0.424249 | 0.002144 | 0.309 | 61.1 |
| big | 10000 | exp | 0.113944 | 0.079073 | 0.032849 | 0.008283 | 2.407 | 9.5 |
| big | 10000 | log | 74.823288 | 0.043326 | 0.712751 | 0.008188 | 0.061 | 5.3 |
| big | 10000 | Log | >120 | 0.770731 | 0.803203 | 0.006713 | 0.960 | 114.8 |

### Lower bound and reference distance

The output floor was stated before measuring in progress.md. For a compulsory fresh dense centre in the
existing fmpz layout, B bytes must be written. The Intel vector-store model gives B/64 cycles.
This is a weak MODEL bound, conditional on the store model and clock. It excludes cached, symbolic,
compressed and exact-shortcut outputs. It is not an arithmetic lower bound for every possible algorithm.

One multiplication followed by reduction modulo p^N is measured separately as a REFERENCE, not a proved
floor. The input pair is p^N-17,p^N-33. FLINT is another REFERENCE. Context setup is outside the comparison
regions; the library has no reusable context. All results are consumed outside the timed regions.

Final benchmark: cpu 2, one FLINT thread, CLOCK_MONOTONIC_RAW, fixed inputs, seed 0, reused outputs.
GCC 13.3.0, -O2 -g; full warning/standard flags are in the compilation commands. FLINT 3.0.1, GMP 6.3.0.
Clock probes next to the final run: 3.105 to 3.783 adds/ns under the one-cycle-add assumption cited in
clockprobe.c. A later laptop load read 3.24. No hardware cycle counter was used.

| f | centre bytes | store cycles | MODEL store floor us | after / store floor |
|---|---:|---:|---:|---:|
| exp | 80000 | 1250.000 | 0.330 to 0.403 | 196416 to 239305 |
| log | 79992 | 1249.875 | 0.330 to 0.403 | 107633 to 131135 |
| Log | 79992 | 1249.875 | 0.330 to 0.403 | 1914688 to 2332775 |

At the word prime, after/multiplication-reference ratios are 9.5,5.3,114.8 for exp,log,Log.
After/FLINT ratios are 2.407,0.061,0.960. These are call-rate reference ratios, not optimality claims.
The three floor ratios above apply only to the specified dense-output model and measured after calls.

## Planted faults

Three private source copies were compiled and run against only the stored comparison.
Each compiled with exit 0 and then exited 1 on value assertions; each ran 80002 checks.

| Fault | Failed checks |
|---|---:|
| Term working precision one digit too small | 672 |
| Inverse/division of the wrong unit integer | 84 |
| Decreasing modulus applied to the sum | 730 |

faults.py retains the patches and executables. The first attempt at fault 2 stopped on a compiler
indentation warning; it was fixed in the scratch patch before any sensitivity credit was taken.
No mutation sweep on src/lfunc.c was run. The mandatory check-all tool self-tests did run.

## Commands and results

Every test/script had a timeout. Builds used at most -j2. No package installation, git command or bd command
was run. One early overlap of scratch compilation, lane rebuild and a pinned baseline process did not have
a combined affinity bound. Its core usage was not measured. Final worker runs were restricted to cpus 2 and 4;
benchmarks used cpu 2. This scheduling limitation is not represented as an isolated baseline measurement.

### Builds, fixtures and numerical checks

- timeout 180 make -j2 BUILD=lanes/f-slice5/build: entry and lane rebuilds, exit 0.
- timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude ... -lflint -lgmp -lm -o ...:
  probe-before, probe, all benchmark variants and power comparison, exit 0.
  Profile copies additionally use -Isrc. The renamed baseline object uses the three public-name defines.
  All source names and full fault compilation arguments are in the retained scripts and build logs.
- timeout 180 python3 -B lanes/f-slice5/gen_stored.py: exit 0; 2000 rows, 379622 bytes.
- timeout 180 make -j2 BUILD=lanes/f-slice5/build lanes/f-slice5/build/test_lfunc: builds exit 0.
- timeout 180 lanes/f-slice5/build/test_lfunc: red, method1, method2 each exit 1 with 2 time failures;
  balanced, word and residual runs each exit 0, 12 tests, 454429 checks, 0 failures.
  The final normal/SAN/clang acceptance runs have that same lfunc count. INV: 13 tests, 454438 checks, 0 failures.
- timeout 900 sh lanes/f-slice5/baseline_bench.sh: interrupted with exit 130 after the partial rows at 3.
  Individual calls used timeout 180, or timeout 120 at the word prime. Completed times are in the table.
- timeout 900 sh lanes/f-slice5/baseline_remaining.sh: exit 0; continued at 5 and the word prime.
  Log at 5,N=100000 had exit 124 at 180 s; word-prime Log had exit 124 at 120 s.
- timeout 120 lanes/f-slice5/build/bench-method1 log 18446744073709551557 10000 1: exit 0, 21.501793087 s.
- timeout 180 sh -c '...bench-method2 log...; timeout 120 ...bench-method2 Log...':
  log exit 0, 22.529920997 s; Log exit 124.
- timeout 180 lanes/f-slice5/build/power: exit 0; power 0.657397564 s, lift 3.077587530 s.
- timeout 60 lanes/f-slice5/build/bench-method4 log/Log 18446744073709551557 10000 3:
  both exit 0; medians 0.082623155/1.777919119 s. Later source versions were measured separately.
- timeout 180 lanes/f-slice5/build/profile-baseline log ... and timeout 120 ... Log ...:
  exits 124; prefix counts and timings are in the profile table above.
- timeout 900 sh lanes/f-slice5/after_bench.sh: final exit 0; 30 families, 96 residue comparisons,
  0 differences. Each row used timeout 180, or 60 at the word prime. Clock probes used timeout 10.
- timeout 180 sh lanes/f-slice5/small_bench.sh: exit 0; 48 before/after pairs, 31 trials per side.
  This single-call run exposed overhead and is superseded by the paired call-rate measurements.
- timeout 180 lanes/f-slice5/build/small_rate: each paired run exit 0, 36 comparisons, 0 value differences.
  Intermediate maximum median ratios: 1.250 and 1.283. Final maximum: 1.032, with the ranges above.
- timeout 60 lanes/f-slice5/build/bench-after --run: exit 0; 3 FLINT comparisons, 0 differences.
- timeout 180 python3 -B lanes/f-slice5/faults.py: final exit 0; 3 executables exit 1;
  failed-check counts 672,84,730. The initial script attempt exited 1 on the scratch compile warning.
- timeout 30 python3 -B lanes/f-slice5/profile_baseline.py and summarize.py: exit 0;
  the first generates private instrumentation; the second writes 30 timing rows and 3 floor rows.

### Required acceptance commands

Each command below ran under timeout 900. Each build began with make clean. check-all was repeated after
the final library changes. The other three runs and the header check are orchestrated by remaining_checks.sh.

| Mode | C programs | Checks | Failed checks |
|---|---:|---:|---:|
| check-all | 67 | 53930323 | 0 |
| SAN=1, detect_leaks=0 | 67 | 53930323 | 0 |
| CC=clang | 67 | 53930325 | 0 |
| INV=1 | 67 | 53935856 | 0 |

Command and last line, each final command exit 0:

    make clean && make -j2 check-all
    check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest

    ASAN_OPTIONS=detect_leaks=0 sh -c 'make clean && make -j2 check SAN=1'
    check passed: all 67 test programs

    make clean && make -j2 check CC=clang
    check passed: all 67 test programs

    make clean && make -j2 check INV=1
    check passed: all 67 test programs

    sh lanes/m1-headers/check_headers.sh
    check_headers: passed

Address/undefined-behaviour diagnostics: 0. The original default SAN command exited 2, with 67
LeakSanitizer fatal errors under ptrace; its last line was make: *** [Makefile:102: check] Error 1.
That log is check-san-default.log. It had 0 failed lfunc assertions. The default mode remains unavailable.

Headers: 66 C/C++ compilations, 0 failures, 38 inline exports, 390 declarations.
Clang header/ABI programs: 58 and 24 checks, 0 failures. clang++ was skipped by the script's standard-library
probe; gcc, clang and g++ were exercised.

### Reviewer suite, final source

Commands: timeout 180 python3 -B lanes/f-slice5/oracle.py MODE, for every original mode below.
The oracle is an unmodified copy of lanes/f-review3/oracle.py. Its local path prevents writes outside this lane.
final_oracles.sh rebuilds the probe, runs the planted faults, then oracle_runs.sh runs all modes.
All final modes exited 0; the counts below are requests, normally with ordinary and aliased calls.

| Mode | Requests | Differences |
|---|---:|---:|
| precision | 3600 | 0 |
| small | 16443 | 0 |
| big | 80 | 0 |
| limits | 161 | 0 |
| random | 3000 | 0 |
| large | 30 | 0 |
| powered_large | 2 | 0 |
| bench_validate | 7 | 0 |

The full reviewer bench.in produced 8 output rows with status, alias, untouched-output and canonical checks.
The original bench_validate mode validates only its original 7 rows. The new eighth Log(2) row is checked
at full precision against FLINT in the benchmark; it is not claimed as a full independent rational-oracle row.

### Memory and artifact checks

    timeout 180 valgrind --leak-check=full --show-leak-kinds=all \
      --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
      --log-file=lanes/f-slice5/valgrind.log lanes/f-slice5/build/stored-only

Exit 0. 80002 stored assertions, 0 failures; 0 memory errors; 79409 allocations and 79409 frees;
19196837 allocated bytes; 0 bytes and 0 blocks in use at exit. The executable links the final normal lane archive.

- timeout 5 wc -l/-c tests/ref/vectors/f-slice5/stored.jsonl: 2000 lines, 379622 bytes.
- timeout 5 awk line-length checks on the new proof/PERF blocks and bench source: 0 lines over 116.
  Existing unrelated long lines were not reformatted.
- timeout 5 sha256sum -c lanes/f-slice5/scope.sha256: 7 matches, 0 mismatches.

## Files written

Owned repository paths:

- src/lfunc.c; include/adelefeld/lfunc.h, cost sentences only.
- docs/api-1f4.md, cost sentences and F8/F9; docs/PERF.md, the series row and its measurement contract.
- tests/test_lfunc.c, additions; bench/bench_lfunc.c, new.
- tests/ref/vectors/f-slice5/stored.jsonl, the explicitly requested fixture path.

Lane paths:

- progress.md; baseline.c, method1.c, method2.c, before-noinline.c.
- gen_stored.py, stored.in; oracle.py, oracle_runs.sh, final_oracles.sh, bench.in, bench.out, bench-new.out.
- faults.py, fault-term_precision.c, fault-wrong_integer.c, fault-sum_precision.c, stored_only.c.
- profile_baseline.py, profile-baseline.c, power.c, clockprobe.c, small_rate.c.
- baseline_bench.sh, baseline_remaining.sh, after_bench.sh, small_bench.sh, remaining_checks.sh, summarize.py.
- table.md, floors.md, scope.sha256; the named build, test, timing, profile, oracle and check logs (*.log).
- build/, the private archives, objects, dependency files and executables; report.md, written once at the end.

Only final acceptance builds used build/ at the repository root. No other source, header, test or specification
file was edited. docs/SPEC.md and docs/PLAN.md were read, not changed.

## Not done

- No worst-case two-second bound for every rational centre or ball. Exp still uses the original Horner scheme.
- No strict small-call no-regression proof. One final median is 3.2% higher, with overlapping trial ranges.
- No change to global W/LIMIT policy, because that would change statuses.
- No full independent rational oracle for Log(2) at the word prime, N=10000. FLINT checked its full residue.
- No production mutation sweep, exhaustive oracle over all rational inputs, or long fuzz campaign.
- Default LeakSanitizer under ptrace; clang++ with a working C++ standard-library search path.

## Sources pending

No new source is pending for F8/F9. The pre-existing F2 pending statement remains outside this lane's proof work:

[source pending: a proof that log and Log take no nonzero rational value at a rational argument]

## Findings against the specification

No counterexample to docs/SPEC.md was found. R1 is addressed for the measured families and the two reviewer
inputs. The small-call timing uncertainty and environment limitations above remain visible. The pre-existing
F2 source-pending issue was not reclassified as a proved statement.
