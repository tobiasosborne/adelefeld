# r-cost2 report

Applied the r-cost1 count proposal in real_count only. Cluster's five-run public-call median fell from
10.420865 s to 0.151585 s. Scaled fell from 2.271892 s to 0.045519 s. Dense random did not improve.
All four required final configurations and both driver fixtures passed. No header, specification,
certificate, complete verifier, or independent verifier recount was changed. No git or bd command ran.

## Implementation and proof

The reduced mean numerator-plus-denominator cutoff is 512 bits. It admits the measured 385-bit cluster
mean; its degree-50 Taylor shift took median 0.001754 s. The plain-count window without translation is
-32 <= scale <= 32. Either scaling direction outside that window can now be used. FLINT counts the
transformed polynomial only if its height is strictly smaller; otherwise it counts the original g.
There is one trusted count in either branch. A rejected transform adds construction and height checking.
An accepted lower-height transform is not guaranteed to have a smaller wall-clock count cost.

R9, docs/design/real-roots.md:366-384, proves the affine bijections preserve the distinct-root count and
squarefreeness. The original normalized polynomial remains the polynomial of each output certificate.
The scaling call cites both directions and the local FLINT source. For positive k, coefficient i is
multiplied by 2^(k i); for negative k, by 2^((-k)(d-i)), before positive 2-content removal.
Sources: refs/src/flint-src-3.0.1/fmpz_poly/scale_2exp.c:23-61 and
refs/src/flint-3.0.1/fmpz_poly.rst:578-581. Taylor shift composes with X+a in place at :2492-2494.
The trusted count's squarefree precondition is at :3265-3269.

## Red and green

Tests were appended before changing real_count. Cluster and scaled match the benchmark's constructions
exactly, with degree 50 and e = 190 and 150. Both must return OK, length 50, stored count 50 and
complete = 1 at prec 2. Cluster has the requested unchanged 2 s bound, using CLOCK_MONOTONIC as in
 test_roots_real_isolate.c:39-45. The append-only addition supplies the Linux clock declaration because
strict-C11 feature selection has already occurred before the appended time.h include.

The random test uses the permitted smaller degree 20. It otherwise uses the exact benchmark construction:
fmpz_poly_randtest, seed (1,2), 10000-bit bound, leading coefficient 3. Its plain FLINT oracle is called
once in the test. This is a wrapper equivalence check; the planted product counts are known independently.
The degree-50 random input remains in the benchmark.

Red command, exit 1:

    timeout 170 taskset -c 2 lanes/r-cost2/build-before/test_roots_real

11 tests, 10880 checks, 1 failed check. Cluster took 11.291522 s, status 0, 50 roots, complete 1;
only the timing assertion failed. Scaled took 2.018488 s, status 0, 50 roots, complete 1.
The degree-20 plain oracle took 0.797953 s and counted 6 roots.

Initial green commands, each exit 0:

    timeout 170 taskset -c 2 lanes/r-cost2/build-after/test_roots_real
    timeout 170 taskset -c 2 lanes/r-cost2/build-after/test_roots_real_isolate

Real: 11 tests, 10880 checks, 0 failures; cluster 0.125029 s, scaled 0.034757 s, both OK/50/complete 1.
The random oracle took 0.745020 s, count 6. Isolate: 15 tests, 19320 checks, 0 failures.

The mutation survivor later led to one additional test for the existing test-only finish helper:
zero polynomial and negative candidate length must return DOMAIN and preserve the list.
Its focused mutant replay failed the zero-polynomial DOMAIN check. Original code passes the final test.

## Before and after measurements

FLINT headers report 3.0.1. GCC is 13.3.0; clang is 18.1.3. Every row is the median of five runs,
CPU 2, CLOCK_MONOTONIC_RAW, prec 2. Products have i=1..50. Individual commands, outputs and exits are
in before-*.json and after-*.json; table.py checked all 120 calls, with 0 failed statuses or counts.

| Family / e / d | adf before s | adf after s | count before s | count after s |
| --- | ---: | ---: | ---: | ---: |
| cluster / 190 / 50 | 10.420865 | 0.151585 | 14.385575 | 8.833954 |
| scaled / 150 / 50 | 2.271892 | 0.045519 | 2.263085 | 2.106068 |
| tiny / 150 / 50 | 0.081181 | 0.038682 | 7.282192 | 7.070694 |
| random / 10000 / 50 | 15.303456 | 16.036433 | 7.900291 | 10.241381 |
| deg / 190 / 50 | 0.030322 | 0.059014 | 0.011854 | 0.018379 |
| small / 0 / 50 | 0.009823 | 0.016365 | 0.006767 | 0.011125 |

count is the unchanged benchmark mode on the raw family input. It measures neither production
normalization nor real_count's optional transform. Its variation is a shared-machine timing control.
Timed benchmark programs were serial on CPU 2. Builds and part of the interrupted self-test overlapped
early samples on CPU 3. Dense-random public times ranged over 8.534189-20.606929 s before and
9.143530-17.272934 s after. No random-input speedup is established.

An extra interleaved deg comparison, five runs per executable, gave 0.050580 s before and 0.061648 s after.
This small regression is retained. It does not affect the cluster's timing assertion.

Every individual benchmark command was:

    timeout 170 taskset -c 2 B/bench_roots_real MODE FAMILY e 50 2

B is lanes/r-cost2/build-before or build-after; MODE is adf or count; FAMILY and e are in the table.
Parent commands were timeout 170 python3 lanes/r-cost2/measure.py PHASE FAMILY, split by MODE for random.
The interleaved check used timeout 170 taskset -c 2 python3 lanes/r-cost2/paired_deg.py, exit 0, 10 calls.

Other measurements, all exit 0:

    timeout 170 taskset -c 3 lanes/r-cost2/build-before/bench_roots_real countmean cluster 190 50 2
    timeout 170 taskset -c 2 lanes/r-cost2/build-after/shift_probe
    timeout 170 taskset -c 2 lanes/r-cost2/build-after/bench_roots_real countscale random 10000 50 2

countmean: scale 4, height 241, count 50, 0.034934 s (one exploratory run).
shift_probe: 5 shifts, 385-bit mean, median shift 0.001753936 s, rational transform 0.002491193 s,
height 212 before dyadic scaling. Random countscale: scale 48, height 9965 -> 11471, count 12,
11.756066 s (one run).

Count-call tracing used this command for each input below, exit 0 in all 9 runs:

    timeout 170 taskset -c 2 lanes/r-cost2/build-after/count_trace FAMILY e 50

Cluster/190: normalized height 9524 -> count height 241; scaled/150: 7715 -> 241;
tiny/150: 7454 -> 241; deg/190: 9501 -> 241; small/0: unchanged 218;
random/10000: unchanged 9965, repeated once after strengthening the trace's expected-count check.
Scaled/600: 30215 -> 215; tiny/600: 29954 -> 240. Their means exceed 512 bits, so these last cases
exercise positive and negative scaling without translation. All calls made exactly one trusted count,
returned OK and complete 1, and counted 50 product roots or 12 random roots.

## Builds and complete checks

Every build returned 0. All directories were under this lane and were removed after the final checks.
The baseline build used CPUs 2,3; subsequent initial builds used CPU 3; final rebuilds used CPUs 2,3.
Each make used -j2 and timeout 170. The command pattern was:

    timeout 170 taskset -c CPU make -s -j2 BUILD=B FLAGS B/test_roots_real B/test_roots_real_isolate

B is lanes/r-cost2/build-before, build-after, build-san, build-inv or build-clang.
FLAGS are empty, empty, SAN=1, INV=1 or CC=clang respectively. Final rebuilds covered after/san/inv/clang.
Benchmark links used cc -std=c11 -O2 -g -Wall -Wextra -Iinclude, the corresponding archive,
and -lflint -lgmp -lm. count_trace also used -Wl,--wrap=fmpz_poly_num_real_roots.

Initial plain, invariant and clang suites each passed 11/10880 real checks with 0 failures.
Their isolate totals were 15/19320, 17/19363 and 15/19320 respectively, all 0 failures, exit 0.
Default SAN suites reached 11/10880 and 15/19320 with 0 assertion failures, but both exited 1:
LeakSanitizer cannot run under ptrace here. Reruns disabled leak detection and both exited 0.
Address and undefined-behavior sanitizers remained enabled.

Final suite commands were timeout 170 python3 lanes/r-cost2/checks.py CONFIG, adding nolsan for san.
CONFIG is after, san, inv or clang. The script runs both whole programs with these exact inner commands:

    timeout 170 taskset -c 2 B/test_roots_real
    timeout 170 taskset -c 2 B/test_roots_real_isolate

For san, env ASAN_OPTIONS=detect_leaks=0 precedes each executable.

| Final configuration | Real tests / checks / failures | Isolate tests / checks / failures | Exits |
| --- | --- | --- | --- |
| plain | 12 / 10885 / 0 | 15 / 19320 / 0 | 0, 0 |
| SAN=1, leak detection disabled | 12 / 10885 / 0 | 15 / 19320 / 0 | 0, 0 |
| INV=1 | 12 / 10885 / 0 | 17 / 19363 / 0 | 0, 0 |
| CC=clang | 12 / 10885 / 0 | 15 / 19320 / 0 | 0, 0 |

Final cluster times were 0.242382, 0.223033, 0.102517 and 0.121418 s in the table's order.
The degree-20 oracle times were 1.043912, 0.655827, 0.434593 and 0.514354 s; every count was 6.
Every original real vector ran: 163 lines, 161 OK cases, 385 roots, 2 DOMAIN cases.
Existing no-root and minimum-scale cases remain in the complete isolation suite.

Driver command: timeout 170 sh lanes/r-cost2/driver.sh, exit 0. This is the targeted form of
 tests/test_driver.sh, using the same fixture statuses and byte comparisons. s-realroots: exit 1 as
expected, 10 equal lines; s-hostile: exit 1 as expected, 19 equal lines. Total: 2 cases, 29 lines,
0 differences. The driver was linked in build-after; no repository-root driver build was used.

## Mutation result

The named tools/mutate/README.md is absent. The tool's docstring and --help were read instead.
There is no CLI line selector, so the requested fallback sampled 30 mutants of src/roots.c.
The dry list command returned 0 and found 1208 candidates; its sample contained 0 real_count mutants.

    timeout 20 taskset -c 3 python3 tools/mutate/mutate.py --files src/roots.c
      --limit 30 --seed 20261015 --list --scratch lanes/r-cost2/build-list

Primary command, exit 1:

    timeout 900 taskset -c 2,3 env PYTHONUNBUFFERED=1
      ASAN_OPTIONS=detect_leaks=0:max_allocation_size_mb=512:allocator_may_return_null=1
      python3 tools/mutate/mutate.py --root lanes/r-cost2/build-mutation-root
      --scratch lanes/r-cost2/build-mutants --files src/roots.c --limit 30 --seed 20261015
      --jobs 2 --timeout 170 --san --make 'timeout 170 sh lanes/r-cost2/mutation-check.sh'
      --copy Makefile include src tests lanes

The lane-owned mirror included a SAN=1 INV=1 build cache. mutation-check.sh builds and runs the complete
padic, bigp, seed, forged, real and isolate root suites, with timeout 170 on each program.
Baseline: 6 programs passed in 67.9 s. Sample: 30 completed in 685.5 s, 24 killed, 1 survived,
5 not compiled, 0 timed out, 0 excused. No compile failure is counted as a killed mutant.

Every survivor, with one line:

- 1681, || -> && in test-only finish: missing zero/negative input tests; appended regression; replay killed.

Focused command: timeout 170 taskset -c 2,3 python3 lanes/r-cost2/replay_survivor.py, exit 0.
It uses the existing tool API to select that exact mutant, with SAN=1 INV=1 and the whole real suite.
Result: 1 mutant killed; the diagnostic names the new zero-polynomial DOMAIN assertion.
The original six-suite sample was not repeated after the added test. All required final suites were.

Compile failures: 695, || -> && needed parentheses under -Werror; 454, dropped eval_mod left d unused;
860 and 859, pointer-star -> + produced invalid declarations; 1781, dropped degree assignment left d
possibly uninitialized. The pointer-star cases are mutation-generator defects, not exercised code defects.
No tool source or equivalent.txt was changed.

## Files written and artifact audit

Production changes: src/roots.c, real_count only; appended tests/test_roots_real.c;
one appended r-cost2 subsection in docs/design/real-roots.md section 10. bench/bench_roots_real.c was unchanged.
Lane artifacts contain the scripts, count and shift probes, original count excerpt, raw measurement JSON,
full suite logs, mutation log/list/replay result, setup-errors.md, median table, audit and this report.

Audit command: timeout 170 taskset -c 2 python3 lanes/r-cost2/audit.py, exit 0.
It checked 8 final program exits, all 0; the driver totals; the mutation totals and killed replay;
then removed every lane build tree. Retained artifacts have 0 files above 100 KB, 0 long written lines,
0 missing newlines and 0 lane build trees. Prose checking excludes the pre-existing brief.md and lane.log.
The automatic stdout journal was reduced to a retained tail; complete check/measurement logs remain.

Setup errors and ownership incident are recorded in setup-errors.md. An early benchmark link returned 1
before the archive existed, then returned 0 after the build. A nonexistent test_roots target returned 2;
corrected targets built with exit 0. One mirror-relative cp returned 1; the subsequent copy returned 0.

I launched the mutation self-test before reading its scratch-path convention:

    timeout 170 taskset -c 3 python3 tools/mutate/selftest.py

It generated build/mutate/selftest and tools/mutate/__pycache__/mutate.cpython-312.pyc outside ownership.
I stopped it with Ctrl-C, exit 130, and left those paths for the orchestrator. No completion is claimed.
The later mutation/replay builds stayed inside the lane; the retained replay disables bytecode writes.

## HEADER-FINDINGs and DESIGN-FINDINGs

HEADER-FINDINGs: none new.

DESIGN-FINDING: lower coefficient height does not guarantee cheaper counting. The rejected-transform
branch adds only preprocessing and comparison; an accepted transform can make a small input slower.
The paired deg medians above show this cost. The cutoff is supported at degree 50, not at arbitrary degree.

DESIGN-FINDING: reversing only the sign passed to FLINT's correct scaling routine still preserves the
count by R9. A count assertion cannot diagnose that sign reversal alone. Source direction and transformed
heights were checked directly; the cluster timing regression also detects the old mean cutoff.

SOURCE-FINDING: fmpz_poly.rst:582 writes 2^(k d) for negative k. The source multiplies coefficient i by
2^((-k)(d-i)); the denominator-clearing multiplier is 2^(-k d). The source and production direction agree.
R9 step 3 retains the historical 16-bit/negative-only choices; the appended subsection records their replacement.

## Not done and sources pending

Not done: faster dense-random count; faster independent complete verifier; a full sample rerun after the
new guard test; direct mutation coverage of real_count in the 30-mutant file sample; LeakSanitizer;
a completed tool self-test; long fuzzing; removal of the accidentally generated paths outside ownership.
The degree-50 random polynomial was benchmarked, while the committed random oracle test uses degree 20.

No new source is pending. Inherited pending sources listed by the design remain Sturm's theorem, IVT
and the eventual quadratic convergence analysis of Eqir. This lane adds no uncited external theorem.

## Findings against the specification

None. No counterexample or incorrect accepted list was found. docs/SPEC.md was not changed.
