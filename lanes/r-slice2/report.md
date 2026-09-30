# Lane r-slice2 report

Date: 2026-09-30. No specification file was changed. No git state command or bd command was run.

## What was built

The public real-root finder uses its trusted count to stop the descent. Count zero builds no isolation tree.
The descent stops when its points and one-root cells account for the whole count. The old hidden candidate
entry remains for existing tests and review probes; the public call supplies its already computed count.

Endpoint contraction tries a geometrically smaller dyadic cell at either edge. It requires a clean new
endpoint, at least two variations in the retained cell, and zero variations in the discarded interval.
Failure leaves the node unchanged and falls back to bisection. Success skips the same bisection chain,
including only empty siblings. It does not decide a status or skip an earlier one-root cell.

Two further costs were necessary to fix the required inputs:

- The complex product spent about 3.9 s in FLINT's count alone. real_count now optionally counts an exact
  rational translate and dyadic scale. This preserves the number of roots and squarefreeness. The output
  polynomial g is unchanged. FLINT's count remains trusted; this is not an independent count certificate.
- Mignotte spent about 22 s refining the right member of its tiny pair after isolation had finished.
  Refinement now evaluates a fixed local polynomial. Certified Arb filters return only the same exact sign
  or rounded secant index; uncertainty repeats the integer calculation. Filters apply to degree at least 8.
  The final certificate uses integer evaluations, with its own exact dyadic translate when that shortens
  both endpoint mantissas. It uses no candidate polynomial or isolation cache.

The prototype follows the count preprocessing, count stopping, contraction, local coordinates and scale
limits. It evaluates exact integers instead of using the optional Arb filter. The differential script
compares endpoint balls exactly and now checks the real place, including constant inputs.

The design contains R6 through R9 and their proofs. R4(3) now assumes refinement returns without LIMIT.
R4(4) fixes the same floor for both precisions and assumes both calls return without LIMIT.
The header describes the new cost and temporary polynomials. PERF's real-root row links the floors and
measurements in the design. eval2 now uses the same exact local identity as the certificate.

## Files written

Repository files:

- src/roots_real.c
- src/roots.c: only the real-place declarations, helpers and adf_roots_real
- include/adelefeld/roots.h: real-root prose only
- docs/design/real-roots.md
- docs/PERF.md: the real-root row only
- tests/test_roots_real_isolate.c
- tests/fuzz/diff_roots_real.py
- proto/real_isolation.py
- proto/test_real_isolation.py
- bench/bench_roots_real.c

Lane files: progress.md, redgreen.log, bite.py, bite_tests.c, capture.c, isolate_existing.c,
roots_before.c, roots_real_before.c, profile_real.c, profile_final.c, profile_probe.c, this report,
the corresponding probe, benchmark, capture, profile and three fault executables and sources,
runs/*.log, runs/*.txt, and build/ compiler artifacts. Harness lane.log was not edited.
No additions were needed in tests/test_roots_real.c; all its existing vectors and assertions ran.

## Lower bound and benchmark

The floor was read and recorded in progress.md before measurement. Under compulsory I/O, read the input
and write the returned balls. Under the retained certificate, determine two exact endpoint signs per
nonexact ball, or exact vanishing for a point. This is an operation-count floor. It does not prescribe a
sign algorithm or prove a lower bound in seconds. eval2 is a reference timing of this required workload.
An empty list has no endpoint signs and no ratio to eval2. The old homogeneous-Horner reference could be
slower than the whole new call; it is not a universal time floor.

Commands, all exit 0:

    timeout 60 taskset -c 2 lanes/r-slice2/bench_before adf FAMILY e d prec
    timeout 170 taskset -c 2 lanes/r-slice2/bench_after --run

Before is one call per row; after is the median of five. This is a shared laptop, not a controlled quiet
run. Both binaries use bench/bench_roots_real.c; the before binary retains the unchanged root code.
Logs: runs/bench_before.txt and runs/bench_after.txt. The full after table has 26 rows, all status 0.

| Input | prec | Before s | After s | eval2 reference s | After / reference |
|---|---:|---:|---:|---:|---:|
| Mignotte, d=50, e=4000 | 2 | 29.704137 | 0.111501 | 0.004330214 | 25.7 |
| Positive, d=50, e=4999 | 2 | 7.787246 | 0.020773 | 0 | undefined |
| Complex, d=50, e=190 | 2 | 4.593804 | 0.002827 | 0 | undefined |
| Thirds, e=30000 | 2 | 0.112229 | 0.001259 | 0.000130900 | 9.6 |
| X^2 - 2 | 2^21 | 0.499636 | 0.371910 | 0.056106208 | 6.6 |

The three added review families are M = X^50 - 2 (2^4000 X - 1)^2,
P = X^50 + 2 (2^4999 X - 1)^2, and C = prod_(i=1..25) (2^380 (3X-1)^2 + i).
The fourth timed input has roots 2^30000 + 1/3 and 2^30000 + 2/3.

The expected counts have direct checks:

1. P is positive: its two summands are nonnegative and cannot both vanish.
2. Every factor of C is at least i > 0. Its product is positive.
3. For M with e=4000, the values at 2^(-e-1), 2^-e, 3*2^(-e-1) have signs -, +, -.
   The outer values at both -2^e and 2^e are positive; M(-1) and M(1) are negative.
   These give three disjoint positive sign-change intervals and one negative interval.
4. The nonzero coefficient signs of M are -, +, -, +. Descartes gives at most three positive roots.
   For M(-X) they are -, -, -, +, giving at most one. Thus the count is four.
   Descartes is quoted from refs/src/sagraloff-mehlhorn/tex/arxivfinal.tex:547-553.

## Red and green checks

All commands in this report had timeout wrappers. Builds used at most -j2.

The lane builds used:

    timeout 170 make -j2 BUILD=lanes/r-slice2/build TARGETS

TARGETS was lanes/r-slice2/build/test_roots_real_isolate, adding
lanes/r-slice2/build/test_roots_real for the combined builds. All ten build phases exited 0:
red, green, local, center, filter, cert, tiny, pairs, final_lane and filter_cutoff.
Their logs are runs/build_*.log.

Runtime command:

    timeout 170 lanes/r-slice2/build/test_roots_real_isolate

| Phase and log | Exit | Tests | Checks | Failed checks |
|---|---:|---:|---:|---:|
| red.log | 1 | 12 | 19189 | 3 |
| green.log, first attempt | 1 | 12 | 19189 | 2 |
| green_filter.log | 0 | 12 | 19189 | 0 |
| green_tiny.log, first cubic attempt | 124 | no completed summary | no completed summary | unknown |
| green_pairs.log | 0 | 13 | 19207 | 0 |
| green_final_lane.log | 0 | 14 | 19225 | 0 |

Red times were 37.025229, 12.499560, 5.726564 and 0.111115 s. The first three failed the required
2.0 s constant. Thirds already met that bound; its improvement is measured and its prototype node bound
is checked separately. No existing C assertion or time bound was weakened.
The final standalone timed cases used 0.139937, 0.025503, 0.003496 and 0.001435 s.

The minimum-scale test contains three pairs and the original cubic, nine planted root occurrences in all.
Their roots are i*2^-16777206, with i in 1..4. Every returned ball is exact and contains its planted root.
The initial cubic timeout was investigated with:

    timeout 170 lanes/r-slice2/probe tiny 3 16777206 2 stages

Exit 124. Normalisation finished in 3.776466 s, then the unscaled FLINT count did not finish within the
limit. Scaling before counting fixed this; the cubic was restored to the test instead of being dropped.

Prototype command:

    timeout 60 env PYTHONDONTWRITEBYTECODE=1 python3 -m unittest proto/test_real_isolation.py

Initial post-change run: 16 tests, 1 failure. The old assertion that the tree had at least e nodes was
obsolete after contraction. It was replaced with a stricter geometric upper bound, while keeping the old
upper bound and all correctness checks. Later runs: 16/16, then 18/18. Latest run: 18 tests in 2.270 s,
0 failures. New cases include count zero, count one and the refinement scale limit.

## Exact output preservation and planted faults

The capture shim wraps the public root finder and the hidden candidate calls. Before executables use the
saved root sources; after/final executables use the lane archive. Existing isolation tests were copied
before the new cases were appended. Capture programs ran as:

    timeout 170 env BALL_CAPTURE=lanes/r-slice2/runs/balls_TAG_KIND.txt lanes/r-slice2/capture_TAG_KIND

TAG was before, after or final; KIND was isolate or real. Every completed capture exited 0.
The isolation program ran 11 tests/19165 checks, and the public program 8 tests/10744 checks.
Final captures contain 1318 calls/3383 balls and 665 calls/2036 balls, respectively.

    cmp lanes/r-slice2/runs/balls_before_isolate.txt lanes/r-slice2/runs/balls_final_isolate.txt
    cmp lanes/r-slice2/runs/balls_before_real.txt lanes/r-slice2/runs/balls_final_real.txt

Both exit 0. The earlier before/after comparisons also exit 0. The 7402 recorded lines are byte-identical.
The new execution preserves the original cells and static floors by R6, and refinement decisions by R8.

    timeout 170 taskset -c 0,1 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/r-slice2/bite.py

Exit 0, 3 of 3 planted faults caught. Each scratch executable exits 1 with 4 failed assertions in 34 checks:
stop one item early; accept contraction without checking the discarded variations; apply the count-zero
shortcut to count one. The final source was used in a repeat with the same results. No production mutation
campaign was run. check-all's required mutation-tool self-test ran only its own fixtures.

## Full checks and differential smoke

| Command under timeout 900 | Exit | Last line |
|---|---:|---|
| make clean && make -j2 check-all | 0 | full last line below |
| make clean && make -j2 check SAN=1, default environment | 2 | make: *** [Makefile:102: check] Error 1 |
| ASAN_OPTIONS=detect_leaks=0; make clean && make -j2 check SAN=1 | 0 | check passed: all 61 test programs |
| make clean && make -j2 check CC=clang | 0 | check passed: all 61 test programs |
| sh lanes/m1-headers/check_headers.sh | 0 | check_headers: passed |

The exact last line of check-all is:

    check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest

The make commands used timeout 900 env PYTHONDONTWRITEBYTECODE=1 bash -c 'COMMAND'.
The supported sanitizer rerun and clang run also used taskset -c 0,1. The sanitizer environment variable
was exported to make, not assigned only to make clean. Logs: runs/check_all.log, check_san_ptrace.log,
check_san.log, check_clang.log, headers.log.
Each complete C suite ran 61 programs, 631 tests and 51950408 checks, with 0 failed assertions/tests.
The default sanitizer run nevertheless failed: all 61 processes hit LeakSanitizer's fatal ptrace error.
ASAN and UBSAN remained enabled in the successful rerun. LeakSanitizer was explicitly disabled.
The header check included clang programs with 8 tests/58 checks and 3 tests/24 checks, both with 0 failures.

Final shared-library build and export check:

    timeout 180 taskset -c 0,1 env CC=clang PYTHONDONTWRITEBYTECODE=1 sh tests/test_exports.sh

Exit 0: 325 of 325 declared functions exported, 0 missing or undeclared.

    timeout 180 taskset -c 0,1 env PYTHONDONTWRITEBYTECODE=1 python3 tests/fuzz/diff_roots_real.py \
        --seconds 120 --seed 20260930

Exit 0: 1075 calls, 1054 OK, 21 DOMAIN, 2848 roots, 1180 exact balls, 35 empty lists, 541 reduced inputs,
1054 exact prototype matches, 0 disagreements. This is a smoke test, not the long fuzz campaign.

## Additional measurements and memory checks

Stage probes were compiled with timeout 60 cc -std=c11 -O2 -Iinclude, the probe source, the lane archive,
and -lflint -lgmp -lm. They ran under timeout 60 with FAMILY degree e prec [stages|skip].
Before local refinement, the Mignotte stage had 22.287287 s in isolation/refinement and 0.713243 s in
finish plus normalisation. Scratch instrumentation put isolation near 0.1 s and the right-pair refinement
near 22 s. Complex's original count stage used 3.925342 s. All completed probes returned status 0.
The later Mignotte public call used 0.406920 s, before the final pinned benchmark's 0.111501 s.

High-precision profile:

    timeout 60 taskset -c 2 lanes/r-slice2/profile_final sqrt2 2 0 2097152 stages

Exit 0: 42 Q steps, 0.496059450 s in Q; 176 exact values, 0.379211108 s in exact values, included in Q.
Candidate/refinement time was 0.496975920 s; finish plus normalisation 0.201822189 s.
The profile is a scratch copy. Production has no profiling state. Exact values and secant work dominate.
No Newton repair of the high-precision route was made. Restricting the filters avoids its measured overhead.

Focused valgrind command form:

    timeout 50 taskset -c 0,1 valgrind --leak-check=full --show-leak-kinds=all \
        --errors-for-leak-kinds=all --error-exitcode=99 lanes/r-slice2/probe_final FAMILY d e 2 skip

| FAMILY d e | Exit | Roots | Allocations / frees | Bytes in use at exit | Errors |
|---|---:|---:|---:|---:|---:|
| mignotte 50 4000 | 0 | 4 | 19507 / 19507 | 0 | 0 |
| complex 50 190 | 0 | 0 | 7932 / 7932 | 0 | 0 |
| tiny 3 1000 | 0 | 3, exact | 4204 / 4204 | 0 | 0 |
| tiny 3 16777206 | 124 | no return | incomplete | 122342416 | 25 at SIGTERM |

The first three cases in the original loop had a timeout 170 outer wrapper; its third, extreme case hit
its 50 s timeout during FLINT normalisation. Its shutdown snapshot had 0 definitely or indirectly lost
bytes, 50466152 possibly lost bytes and 71876264 reachable bytes. This is not a completed leak result.
The e=1000 cubic was a separate completed check of the same count-scaling and point-refinement paths.
Logs: runs/valgrind.log and runs/valgrind_tiny1000.log.

Scratch compiles used timeout 60 cc -std=c11 -O2, -Iinclude, -Itests where needed, and -lflint -lgmp -lm.
The capture compiles used --wrap=adf_roots_real and --wrap=adf_roots_real_isolate; new captures also wrapped
adf_roots_real_isolate_counted. The public capture added the JSONL/golden support objects.
One initial capture compile omitted those support objects: link exit 1, followed by attempted run exit 127.
It was corrected and rerun. One profile attempt used the review probe's unsupported sqrt2 family: exit 2.
The lane copy added that family; the corrected profile above exited 0. All other scratch compiles exited 0.

Final source-comment syntax check, after documentation and proof-comment updates:

    timeout 60 taskset -c 0,1 cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
        -fsyntax-only src/roots.c src/roots_real.c

Exit 0, two translation units. git diff --check returned 0. Prose-width checks found 0 overlong lines in
real-roots.md, the real-root header and progress.md. Only comments/documentation changed after full checks.

## What is proved and what is not

R6 proves that accepted contraction skips the original tree's empty siblings and no output cell or point.
It uses Descartes and variation monotonicity from refs/src/sagraloff-mehlhorn/tex/arxivfinal.tex:547-575.
R7 proves count-guided stopping and repeats the final certificate's soundness argument, conditional on the
true trusted count and IVT. R8 proves the local evaluation identity and transparency of the certified Arb
filters, using refs/src/flint-3.0.1/arb.rst:6-20, 155-165, 531-553 and 639-650.
R9 proves the affine/dyadic count bijection and preservation of squarefreeness.
FLINT arithmetic and FLINT's count are trusted. The count's internal Sturm computation is not certified.

There is no improved worst-case bit-complexity proof, optimal-time claim or quantitative peak-memory proof.
The general eventual quadratic convergence of Q is still not proved here.

## Not done and procedural limits

No long differential campaign or production mutation campaign. No Newton step for an interior cluster.
The high-precision regression against the old complex-root route is not claimed repaired.
The extreme cubic has no completed valgrind result. Default LeakSanitizer cannot run under this harness.
The public completeness verifier still recounts the original g and can remain expensive; it was not changed.

Every make used at most -j2. Some short auxiliary compiles/Python checks overlapped a two-job build or SAN
loop before global affinity was enforced. More than two runnable CPU jobs were briefly possible; strict
whole-lane CPU compliance is not claimed. Later checks shared CPUs 0 and 1. Required full checks used their
900 s wrappers; other completed programs/scripts stayed below 180 s. No system package was installed.

## Sources pending

- [source pending: an on-disk real-analysis statement of IVT; inherited solvers.md:64-67]
- [source pending: an on-disk Sturm theorem behind FLINT's trusted count]
- [source pending: Kerber's full analysis of eventual quadratic convergence of Eqir]

## Findings against the specification

No mathematical counterexample against SPEC.md was found. The two false design qualifications were fixed.
The necessary count preprocessing is an implementation extension proved in R9: it counts an equivalent
polynomial, then uses that same integer as the count of the unchanged normalised g.
The original default SAN command did not pass because of the stated runtime limitation; this is recorded
separately from the successful ASAN/UBSAN run. The incomplete timeout checks are not reported as passes.
