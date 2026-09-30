# f-slice5 running notes

Read CLAUDE.md, COMMON.md, COMMON-C.md, PERF.md, SPEC 9.3.1/2 and N-D9, PLAN 1F,
api-1f4 F1-F7, functions Propositions 7/7b/8, lfunc.c, lfunc.h and review R1/R2.
Saved the unmodified source as baseline.c before any implementation change.

Before measurement: for padded residues B = ceil(K log2(p)/8), writing B bytes is compulsory.
The Intel vector model gives B/64 cycles, conditional on dense padded output and section 1b.
A modular multiplication is a REFERENCE operation cost, not an arithmetic lower bound.
Both will be measured with the same p, K and warm context. No cycle conversion without a clock probe.

Stored baseline: seed 930505, 2000 calls, 379622 bytes. Generation succeeded before source changes.
Red: 12 tests, 454429 checks, 2 failed checks (the two five-second R1 cases).
Method 1: same stored outputs; R1 still red; log big-prime N10000 = 21.501793087 s.
Method 2: same stored outputs; R1 still red; log = 22.529920997 s; Log timed out at 120 s.
The separate power comparison: 0.657397564 s, Teichmueller lift 3.077587530 s. Kept the existing one power.
Method 4: balanced factors and exact binary splitting. 12 tests, 454429 checks, 0 failures.
Big-prime medians: log 0.082623155 s, Log 1.777919119 s (three trials).
Sensitivity: term precision / wrong integer / sum precision: 1720 / 338 / 2182 failed checks.
The first faults.py run stopped at a compiler indentation warning in fault 2. Fixed the scratch fault syntax;
all three then compiled and exited 1 from the stored comparison. No mutation tool run.

Baseline 100000-digit log/Log at 3 did not finish all three trials within 180 s.
Their completed first calls were 88.666024570 s and 110.166717076 s.
Interrupted the broad baseline shell (exit 130) after those rows; continued p=5 and the big prime with one trial.
Concurrent benchmarks were pinned to cpu 2; this makes timings provisional due to shared-core interference.
From the final runs onward all computations are restricted to cpus 2 and 4, and benchmarks use cpu 2.

Reviewer oracle copy in this lane avoids writing to the reviewer's lane. All eight original modes passed.
Final F8 faults after retaining the tagged-word loop: 808 / 124 / 936 failed checks, each exit 1.
Word-loop regression check: 12 tests, 454429 checks, 0 failures.
An intermediate benchmark with the acceptance suite sharing cpu 2 had Log median 2.093228368 s.
Its clock probes ranged from 0.718 to 2.083 adds/ns, so they are not usable clock calibrations.
These contended logs are retained and will not be used for a cycles conversion.
F9 residual inversion now uses only K-m digits: (u-l)/p^m is integral, and
u/l = 1 + p^m ((u-l)/p^m)/l. Added this precision proof to F9.
Residual check: 12 tests, 454429 checks, 0 failures. Source changes stop here.
The first check-all passed, then check-all was repeated after this last source change.

Timed source copies of the saved baseline (cpu 4, shared; prefix diagnosis only):
log(1+p), first 5000 terms: power/reduction 0.068221 s; p division 0.450437 s;
unit inverse 4.209618 s; term product 58.602675 s; sum reduction 86.309951 s. Timeout 180, exit 124.
Log(2), first 2000 terms: power/reduction 58.400745 s; p division 0.238840 s;
unit inverse 0.613780 s; term product 19.448258 s; sum reduction 39.102594 s. Timeout 120, exit 124.
Thus the initial claim that inverses dominate was wrong for these measured prefixes.
The profile copies instrument individual operations and never replace the library source.

Final benchmark: 96 full residues compared with FLINT, 0 differences.
Big-prime medians: exp 0.138967166 s, log 0.090152120 s, Log 1.721008929 s.
Their five-trial maxima are 0.143551480 s, 0.098825404 s and 1.914626942 s.
Final clock probes: 2.074 to 2.231 adds/ns; kernel read 2.1 GHz afterwards; laptop load 8.11.
The clock probes model a one-cycle add, as documented in the copied clockprobe.c.

Default SAN run: make exit 2. Lfunc itself: 12 tests, 454429 checks, 0 failed checks/tests.
LeakSanitizer could not run under ptrace. A clean SAN rerun with ASAN_OPTIONS=detect_leaks=0 is in progress.
Valgrind on the final stored comparison: exit 0, 80002 checks, 0 failures, 0 memory errors,
79408 allocations and 79408 frees, 0 bytes and 0 blocks in use at exit.
Final reviewer suites and final planted faults passed again after the residual-inverse change.

Paired call-rate microbenchmark exposed a 25% regression at p=2,N=32 despite the initial word guard.
W bits(p) overestimates the actual modulus size (bits(2)=2). The guard now uses fmpz_bits(P).
The larger evaluators are kept out of line with FLINT_STATIC_NOINLINE.
The final paired run (same process, alternating old/new order, five trials): 36 comparisons,
median new/old ratios 0.385 to 1.032. Largest increase: Log at 5,N=8, 729.445 to 752.496 ns;
old min/max 713.635/750.618 ns, new min/max 745.122/766.710 ns (overlapping trial ranges).
For p=2,N=32 the 20-28% regression is gone. Source changes stop after the actual-size word guard.
All acceptance commands and benchmark rows are repeated on this final source.

Final-source check-all: exit 0, all 67 C programs plus driver, exports, Julia and both tool self-tests.
Final source benchmark repeats: big-prime medians exp 0.079072621 s, log 0.043326163 s,
Log 0.770731474 s. Clock probes: 3.105 to 3.783 adds/ns. The later laptop load was 3.24.
The final table/floors and PERF row replace the intermediate measurements above.

All final acceptance runs completed: check-all, SAN with leak detection disabled, clang, INV and headers: exit 0.
C programs: 67 in each run. Check totals: 53930323 normal/SAN, 53930325 clang, 53935856 INV; 0 failures.
Headers: 66 compilations, 0 failures, 38 inline exports, 390 declarations; clang++ skipped by its own probe.
Final fault failures: term precision 672, wrong integer 84, sum precision 730; each executable exited 1.
Final Valgrind stored check: 80002 assertions, 0 failures, 0 errors, 79409 allocations/frees,
0 bytes and 0 blocks in use at exit. Final reviewer suite counts: 3600,16443,80,161,3000,30,2,7; 0 differences.
The benchmark --run entry point also completed, with 3 FLINT residue comparisons and 0 differences.
Scope checksum check: 7 matches, 0 mismatches. New proof/PERF prose and benchmark source: 0 lines over 116.
