# r-cost1 report

Implemented the refinement-filter change. Exact refinement evaluations for X^50 - 2 at 16384 fell from
98 to 0. Maximum precision now completes: median 91.769871 s, versus five 170 s timeouts before.
The requested 10 s and 5 s targets were not reached. The unchanged endpoint certificate dominates.

B is not applied. real_count is in src/roots.c, outside the owned paths. An ownership question received
no answer. count-proposal.patch is an unapplied, unlinked proposal confined to that function.
The complete verifier still performs its independent plain recount. No header or specification changed.

## Before and after

FLINT 3.0.1, CPU 2, CLOCK_MONOTONIC_RAW. Each result uses five runs. The main comparison interleaves
old and new executables. Maximum-precision before values are censored, not assigned a finite median.
Raw commands, exit codes and outputs are in before.jsonl, before-serial.jsonl, paired.jsonl, after.jsonl,
counts.jsonl and references.jsonl. measurements.md was written before the filter change.
Early overlapping runs were excluded. The first small/20000 reference batch overlapped profiling and
was repeated; the table uses its final five runs. Timings varied substantially on the shared machine.

| Input (family, e, d) | prec | Before s | After s | eval2 s |
| --- | ---: | ---: | ---: | ---: |
| deg, 190, 50 | 2 | 0.092760 | 0.047105 | 0.001389324 |
| small, 0, 50 | 2 | 0.014002 | 0.017048 | 0.000081663 |
| small, 0, 50 | 20000 | 0.014223 | 0.014871 | 0.000137792 |
| scaled, 150, 50 | 2 | 2.175044 | 2.103012 | 0.000969679 |
| tiny, 150, 50 | 2 | 0.075460 | 0.076687 | 0.000873327 |
| cluster, 190, 50 | 2 | 10.802954 | 10.694166 | 0.004278207 |
| random, 10000, 50 | 2 | 14.127581 | 12.252622 | 0.005342488 |
| sqrt2, 0, 2 | 2097152 | 0.940909 | 0.921131 | 0.088264061 |
| xn2, 0, 50 | 16384 | 1.174218 | 0.391825 | 0.078687633 |
| xn2, 0, 50 | 65536 | 4.976292 | 2.005763 | 0.620194271 |
| xn2, 0, 50 | 262144 | 20.433207 | 7.151269 | 4.546090339 |
| xn2, 0, 50 | 2097152 | >170 (0/5) | 91.769871 | 35.982333616 |
| wilkinson, 0, 50 | 20000 | 37.977476 | 16.611681 | 7.107090958 |

Families: deg = prod (X - 2^e - i); small = prod (X - i); scaled = prod (X - i 2^e);
tiny = prod (2^e X - i); cluster = prod (3 2^e X - (2^e + i)); all products have i=1..d.
random uses fmpz_poly_randtest, seed (1,2), leading coefficient 3, degree 50, largest coefficient 9965 bits.
xn2 = X^d - 2. wilkinson = prod (X - i) + 1, matching the review's perturbed input.
Pure Wilkinson is the small/20000 control; its dyadic roots are exact and it is much cheaper.

The reference is the existing eval2 mode on returned balls. For xn2 and wilkinson, cand_eval2 skips
certificate setup and evaluates the same exact candidate endpoints. R5 and the unchanged certificate
justify this reference. It does not supply a cheaper production certificate. Each reference time is
itself an average over at least 0.2 s; the table takes the median of five such measurements.

## Count experiments and profile

Five-run count medians, including preprocessing:

| Input | Plain count s | Equivalent count s | Result |
| --- | ---: | ---: | ---: |
| scaled/150/50 | adf dominated by about 2 s | 0.068015 | 50 |
| cluster/190/50 | single profile 7.766153 | 0.043665 | 50 |
| random/10000/50 | 22.171760 | 19.371258 | 12 |

The random scale was 48. Coefficient height grew from 9965 to 11471 bits. Ranges overlap; no robust
large improvement was established for this dense input. Its Sturm profile was 13.048243 s: remainders
9.587881 s, exact divisions 3.356982 s, 49 remainder steps. The cluster profile was 7.766114 s:
remainders 7.286650 s, divisions 0.413298 s, 49 steps. These are single profiles, not medians.
Mean translation reduces the cluster to 241-bit coefficients. Its mean has numerator-plus-denominator
size 385 bits, beyond the production cutoff of 16. The proposal permits 512 bits, positive scaling,
and rejects a transformed polynomial whose height fails to shrink. It has not been integrated or tested
as production code. R9, docs/design/real-roots.md:366-384, proves all count equivalences used here.

Single section-clock profiles:

| Input | Count s | Isolate/refine s | Certificate s | Complete verifier s |
| --- | ---: | ---: | ---: | ---: |
| cluster/190/50, before | 11.885964 | 0.151242 | 0.014082 | 13.916625 |
| random/10000/50, before | 20.135522 | 1.619963 | 0.019296 | 23.194535 |
| xn2/262144, before | 0.000007 | 25.729195 | 13.582454 | 6.749715 |
| xn2/262144, after | 0.000003 | 0.813819 | 7.772373 | 5.498909 |
| wilkinson/20000, before | 0.013655 | 36.908765 | 22.281253 | 10.971973 |
| wilkinson/20000, after | 0.012176 | 1.266163 | 16.149420 | 6.610679 |
| xn2/2097152, after | 0.000004 | 7.858118 | 55.883947 | 29.535387 |

The final corrected profiler counts both sign-filter attempts. At xn2/262144: 42 QIR attempts,
92 signs, 0 exact values; secant filters 0.321312 s and sign filters 0.491906 s.
At wilkinson/20000: 738 QIR attempts, 1576 signs, 0 exact values; secants 0.506867 s, signs 0.743991 s.
The earlier maximum-precision profile omitted successful high-precision retries from its sign timer;
its outer section clocks, 48 QIR attempts and 0 exact values remain valid. No sign-time claim uses it.

## Changes and proof

QIR already doubled j on success. The local shift was already performed once per initial cell.
The old j+64 secant filter and fixed 64-bit sign filter became uncertain near roots, causing exact
homogeneous Horner on degree times the endpoint precision bits. The new filters include the accumulated
coordinate depth and the local coefficient height. A failed filter still uses the same exact arithmetic.
Coordinates, grid integers, stopping rules and certificate are unchanged. Section 10 of the design gives
six steps proving equivalence from ball enclosure and unique-integer semantics. No new worst-case bound
on exact fallbacks is claimed. store_bound extracts the existing final overflow check for direct tests.
All measurement counters are per-call; the hidden test hooks exist only with ADF_CHECK_INVARIANTS.

## Checks, commands and results

All programs and scripts used timeout. Benchmark commands were:

    timeout 170 taskset -c 2 B/bench_roots_real MODE FAMILY e d prec

B is build-before or build-after below this lane. The JSONL files give every concrete command and result.
measure.py used five repeats, split into bounded batches for slower cases. paired.py interleaved old/new.
The five slow baseline calls and five new calls used an outer timeout 180 around measure.py and an inner
benchmark timeout 170. count, countscale, countmean, eval2 and cand_eval2 were measured as listed above.
The instrumented benchmark used the same arguments, with scratch roots.c and roots_real.c objects.
profile.py generated these copies. sturm_profile.c and a compiled local num_real_roots.c supplied the
internal Sturm section clocks; no installed FLINT source or binary was modified.

Build command for every configuration:

    timeout 170 taskset -c 3 make -s -j2 BUILD=lanes/r-cost1/build-CONFIG FLAGS TARGETS

TARGETS were the two complete test executables in that build directory. FLAGS: none for after;
SAN=1 for san; INV=1 for red; CC=clang for clang; SAN=1 INV=1 for mutcache. Builds returned 0.
Benchmarks were linked with the header's cc -std=c11 -O2 -g -Wall -Wextra command and the lane archive.
Scratch profiling, comparison, guard and oracle sources used the same compiler/link libraries.

Every full test command was timeout 170 taskset -c 3 B/test_roots_real or B/test_roots_real_isolate.
Sanitized reruns additionally used env ASAN_OPTIONS=detect_leaks=0.

| Build | real tests/checks/failures | isolate tests/checks/failures | Exit |
| --- | --- | --- | ---: |
| plain | 9 / 10867 / 0 | 15 / 19320 / 0 | 0 |
| SAN=1, leak detection disabled | 9 / 10867 / 0 | 15 / 19320 / 0 | 0 |
| INV=1 | 9 / 10867 / 0 | 17 / 19363 / 0 | 0 |
| CC=clang | 9 / 10867 / 0 | 15 / 19320 / 0 | 0 |
| SAN=1 INV=1, leak detection disabled | built | 17 / 19363 / 0 | 0 |

The invariant suite was repeated after initializing test sentinels: 17 tests, 19363 checks, 0 failures.
Every original vector ran: 163 lines, 161 OK cases with 385 roots, 2 DOMAIN cases.
Default SAN runs reached 0 assertion failures but both exited 1 because LeakSanitizer cannot run under
ptrace here. The first no-LSan isolate rerun failed the existing 2 s timing check at 2.641962 s.
An unchanged retry passed at 0.896904 s. No threshold or assertion was weakened.

New red-green checks:

- Exact-evaluation counter: before 98, bound 16, 1 failed check; depth-only attempt 64, still 1 failure;
  coefficient-height guard 0, 0 failures. The same run checked the exact balls with the rational oracle.
- Degree-fifty public test: five precisions, four nested pairs, including 8192/8193, exact Sturm checks.
- Guard probes: minimum-bound mutant returned K=2 instead of 1; product overflow mutant returned 1
  instead of refusal; bound-overflow-status mutant returned 0 instead of -1. Each probe exited 1.
  The final invariant test checks eight minimum bounds and eight arithmetic/storage conditions.
- Stack fixture: twelve planted dyadic roots force more than four pending nodes. Removing reallocation
  caused an ASan heap-buffer-overflow, exit 1. The final full suites pass its independent exact oracle.

Additional exact checks:

    timeout 170 taskset -c 3 build-after/compare_candidates
    timeout 170 taskset -c 3 build-after/oracle_cases random 10000 50 2
    timeout 170 taskset -c 3 build-after/oracle_cases cluster 190 50 2
    timeout 170 taskset -c 3 build-after/exact_high xn2 2097152
    timeout 170 taskset -c 3 build-after/exact_high wilkinson 20000

Comparison: 36 runs, 1232 balls, 0 differences against saved pre-change code, exit 0.
Cluster rational Sturm oracle: 50 roots, 301 checks, 0 failures, exit 0.
Dense random rational Sturm oracle: timeout 170, exit 124; not independently completed.
Independent power/product checks: 2 and 50 roots respectively, 0 sign failures, exit 0. These use no
FLINT root count. They check exact endpoint signs, exact relative-radius inequalities, and disjoint
known root regions. The Wilkinson count comes from fifty disjoint quarter-width sign-change intervals.
An initial scratch oracle build had a family-name conflict; it was fixed before these runs.

Driver command: timeout 170 taskset -c 3 sh tests/test_driver.sh from build-driver-root.
Plain driver: 100 cases, 101691 expected lines, 0 differences, exit 0. realroots fixtures are s-realroots
and s-hostile. An optional SAN run also passed 100 cases and 101691 lines with leak detection disabled.
That optional run was accidentally launched from the repository root. It generated build/driver,
build/drv-san and build/adf-san outside the owned paths. These were left for the orchestrator; no source
outside ownership was changed. The lane's own build mirrors and build trees were removed.

NO_ADF check: compiled with -DNO_ADF -Wall -Wextra -Werror; countmean cluster returned 50, exit 0;
countmean small/degree-zero returned 0, exit 0. An initial build warned about missing fmpq declarations;
the explicit header include corrected it. git diff --check returned 0.

## Mutation result

Primary command (1200 s outer cap, two jobs, sanitizer and invariant builds):

    timeout 1200 taskset -c 2,3 env ASAN_OPTIONS=detect_leaks=0 python3 tools/mutate/mutate.py
      --root lanes/r-cost1/build-mutation-root --scratch lanes/r-cost1/build-mutants
      --files src/roots_real.c --limit 60 --seed 20261015 --jobs 2 --timeout 170 --san
      --make "timeout 170 make -s -j2 check INV=1 SAN=1 BUILD=lanes/r-cost1/build-mutcache
              TEST_SRC='tests/test_roots_real.c tests/test_roots_real_isolate.c'"
      --copy Makefile include src tests lanes

842 candidates, 60 selected; baseline passed in 24.0 s. Outer exit 124. At least 26 completed before
interruption: at least 10 killed by tests, 9 survived, 6 timed out, 1 did not compile. Two running commands
were terminated at the cap. The tool gives no exact killed total when interrupted. mutation-progress.jsonl
records the active indices and lower bound. No claim is made that all 60 were tested.
Focused survivor probes used timeout 170 and the saved sampled mutations. The three guard probes ran
inside the main mutation window. The stack probe ran after the cap and produced the recorded ASan error.
The final full mutation sweep was not rerun after adding the regressions.

Every survivor, identified by its original run line and token:

- 500, drop right's X division: open-cell roots are unchanged; the retained endpoint factor changes degree.
- 289, bound comparison >0 to >1: gap in minimum-bound tests; focused red and new invariant test added.
- 63, overflow refusal 0 to 1: gap; focused red and product-boundary test added.
- 977, drop fmpz_zero(c): redundant, c is zero from fmpz_init and has not yet been used.
- 208, saturation > to >=: equivalent, equality already computes exactly WORD_MAX.
- 523, drop stack reallocation: gap; heap-overflow red and twelve-root regression added.
- 276, overflow status -1 to 0: gap; focused red and synthetic bound-storage test added.
- 318, root-bound error return 0 to 1: gap; the identical storage check was extracted and tested at WORD_MAX.
- 130, degree guard >0 to >1: equivalent, degree one times positive t cannot exceed WORD_MAX when t fits.

The six timeout sites were lines 603, 880, 338, 136, 666 and 307 in the run snapshot. The compile failure
was dropping fmpz_init_set_si(e,k), leaving k unused. tools/mutate/equivalent.txt was not changed.

## HEADER-FINDINGs and DESIGN-FINDINGs

HEADER-FINDING: precision 2097152 at degree 50 remains a 91.769871 s median workload, versus about
0.921131 s for degree two. Wilkinson plus 1 at precision 20000 remains 16.611681 s. The limit is unchanged.
These are costs, not violations of the header's stated time guarantees; it makes no general time guarantee.

DESIGN-FINDING: both count transformations pay on structured inputs, but real_count is outside ownership.
The 16-bit mean cutoff excludes the measured 385-bit rational mean. The unapplied proposal is supplied.

DESIGN-FINDING: a fresh Descartes upper bound U equal to the number m of certified disjoint balls proves
completeness: P3.8 gives m <= N, Descartes gives N <= U, so m=U implies N=m. Include zero once when g(0)=0.
The local source is refs/src/sagraloff-mehlhorn/tex/arxivfinal.tex:547-553. P3.13(5) currently names a fresh
FLINT/Sturm count, not this criterion. Section 10 records the proof proposal. No verifier code changed.

## Files written

src/roots_real.c; bench/bench_roots_real.c; appended tests in both real-root test files;
appended section 10 in docs/design/real-roots.md. Lane files include measurement scripts, raw JSONL data,
clocked source copies, guard/stack/oracle/comparison probes, logs, count-proposal.patch, tables.md,
measurements.md and this report. Every retained lane artifact is below 100 KB and ends with a newline.
Generated build trees were deleted. The automatic stdout journal was reduced to a retained tail.

## Not done and sources pending

Not done: production count changes; faster verifier; the 10 s and 5 s goals; complete 60-mutant sweep;
full mutation rerun after added tests; LeakSanitizer; a completed independent dense 10000-bit Sturm check;
long fuzzing; endpoint coarsening or changes to the protected certificate implementation.
The unapplied count patch was not linked or validated as a production change.
Inherited pending sources: Sturm's theorem, IVT, eventual quadratic convergence analysis of Eqir,
and the implementation source of arb_rel_accuracy_bits. No new external theorem was cited from memory.
FLINT pow semantics used by the independent oracle are in refs/src/flint-3.0.1/fmpz.rst:913-917.

## Findings against the specification

None. No counterexample or incorrect accepted list was found.

Final artifact audit: 0 files above 100 KB, 0 long artifact lines, 0 missing newlines, 0 lane build trees.
Saved source copies had inherited long lines; these were wrapped without changing code behavior.
The pre-existing 128-character run_table line in bench/bench_roots_real.c remains unchanged.

Final oracle rerun used degree-sized storage and linked src/roots_real.c directly.
Both timeout-170 runs again returned 0: 2 and 50 roots, 0 independent-sign failures.
