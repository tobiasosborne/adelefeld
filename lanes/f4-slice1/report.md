# Lane f4-slice1 report

Slice 4a is implemented end to end. No git command or bd command was run. No specification file was changed.
All changes stay in the owned paths. The six lane build/scratch directories and mutation scratch were removed.
The requested driver/export checks leave their normal root build artifacts for the orchestrator.

## Work per step

A. gen_vectors.py uses proto/functions4_checks.py and its exact text oracle. The final four JSONL files contain
22 function records, 18 text records, 1 sum record and 4 cap records, totaling 359328 bytes.
The function records have 1274 cells: 14 exact inputs and 8 inputs with radii. All eight requested layouts occur.
The six valid golden inputs also have transform records. Small transforms use 320-bit certified reference balls.
Exact rational coordinates are isolated by cyclotomic polynomial reduction. The L=1024 delta uses 96-bit
certificates and the sparse exact phase formula. Full uncertain transforms have 96-bit certificates.
The sum record writes all 36 cells of each input refinement and all 36 sums at (D,M)=(6,6).
The text records contain every fixed canonical/rejected row. The caps include setter length 2^20 and 2^20+1,
and direct-transform length 1024 and 1025. Every record is read by test_ffun.
Python-flint reports version 0.8.0 with FLINT 3.3.1; the C implementation uses the installed FLINT 3.0.1.
The reference uses ball certificates, not mpmath's numerical comparison margin as an error certificate.

B. test_ffun checks lifecycle, deep copies, swap/self-swap, full predicates, layout queries, raw setters,
all 18 fixed goldens, every generated vector, and byte/representation preservation on failures.
It checks each common-refinement cell, zero identity, commutativity, exact sum rounding, and all permitted
whole-object aliases with radii. Repeated balls lose correlations as F1 permits; enclosure is asserted there.
It checks non-symmetric complex data, delta_1 at (2,3), exact cardinal reflection, weighted Parseval,
precision 2/53/cap/cap+1, negative precision, and shrinking radii at 64/128/212 bits.
The F4 radius allowance is stated in test_ffun.c and docs/api-4a.md. Exact certificates and four joint input
corners are contained; uncertain reference rectangles must overlap. Radius is checked independently of containment.
The tests also use a 2002-bit value. A noncardinal cap call returns NOT_DETERMINED and preserves both distinct
and aliased outputs. Seventeen INV child cases cover every input/output argument position.
Memory hooks observe zero allocations on two early cap refusals and 1000 init allocations with 1000 frees.
LeakSanitizer itself cannot run under this sandbox's ptrace setup; the attempted run is recorded below.

C. The new header and C implementation expose the exact declared layout and fourteen public operations,
including the two exported/header-inline layout queries. adf_ffun_refine is implemented completely and declared.
Raw setters, refinement, addition and Fourier preflight the D1 sizes before output allocation.
Precision failure comes first. Every status call builds a temporary and commits once.
Fourier uses the reduced negative angle at adf_phase_get_acb, the 1/M weight, and exchanged dimensions.
Every multiplication, addition and division is an outward acb operation. Private cleanup can release nonfinite
intermediates under INV without invoking public clear's canonical precondition. No global library state is added.
The ffun reader/printer is appended to src/text.c and reuses its lexical, decimal and printing machinery.
Exactly one umbrella include and one clause in the conventions status-table row were added.

D. The three driver commands and direct shell forms are wired: ffun, ffun_add, ffun_fourier.
Three fixtures have 15 hand-derived lines, including delta_1 at (4,1) and delta_0 at (2,3) with coefficient 3.
The sum fixture separately distinguishes zero extension and repetition.
Julia allocates aligned byte storage using both layout queries, uses GC.@preserve, frees every returned string,
and checks Fourier, reflection, aliasing, addition and preserved LIMIT. Its test is registered in test_julia.sh.
The bare-scalar command in the brief conflicts with the fixed grammar; the equivalent complex-entry form works.

E. docs/api-4a.md states each operation's behavior by reference to design sections 1-4 and F1-F4.
It gives the enclosure steps, statuses, caps, costs, Check lines, and choices where the design is silent.
The simple direct implementation recomputes phases. A cached phase table would avoid repeated trig work.
Addition fetches both refined values directly, avoiding two temporary refined arrays.

F. All twelve original named faults were caught. Three test-gap survivors were caught after adding assertions.
A final Fourier early-commit fault was caught by the valid NOT_DETERMINED failure snapshot.
The final SAN+INV scratch run has 16 compiled faults and 16 SIGABRT results.
Two mutation sweeps ran 44 attempts, 32 distinct mutations, in 1016.1 seconds. The remaining branch survivor
is listed below. No mutation-tool code or equivalent.txt entry was changed.

## Files written

- include/adelefeld/ffun.h and src/ffun.c.
- The appended ffun block in src/text.c; one include line in include/adelefeld.h.
- One authorized clause in docs/conventions.md, row "Integrals, Poisson summation".
- tests/test_ffun.c and tests/ref/vectors/f4-slice1/{functions,texts,sums,caps}.jsonl.
- The ffun command wiring in tools/adf/adf.c and its README section.
- tests/driver/ffun-{values,add,fourier}.cmd and their .out files.
- tests/julia/ffun.jl and its registration block in tests/test_julia.sh.
- docs/api-4a.md.
- Lane gen_vectors.py, plant_faults.py, cap_phase_probe.c, oracle-zero.cmd, redgreen.md, this report,
  fault-results.json, and bounded build/check/fault/mutation logs and driver output captures.

## Commands and results

Every test/script used timeout. Builds used make -j2. Sweep workers were limited to one, with two make jobs.
The initial build/red/green corrections and intermediate counts are recorded in redgreen.md.

1. `timeout 120 python3 -B proto/functions4_checks.py`: exit 0, 2670 checks.
   `timeout 60 python3 -B lanes/f4-slice1/gen_vectors.py`: final exit 0, 22+18+1+4 records, 359328 bytes.
   Initial generator refusal: exit 1 at 767453 bytes; no oversized fixture remains.
   One edit had an indentation error, and one cyclotomic scalar operation used the wrong SymPy API; both exit 1.
   Cap fixture LIMIT was corrected from 9 to the declared status 10 before its green run.

2. Final ordinary build and test:

       timeout 180 make -j2 BUILD=lanes/f4-slice1/build lanes/f4-slice1/build/test_ffun
       timeout 120 lanes/f4-slice1/build/test_ffun

   Both exit 0; 57465 checks. The final build followed the noncardinal cap failure and low-precision containment
   additions. Earlier full greens were 38643, 39567, 39583 and 57439 checks.
   Initial first-file compile red: exit 2, absent declarations. Staged compile errors included unavailable
   _acb_vec_equal and a missing SIZE_MAX include, each exit 2; corrected without weakening tests.
   Function-group reds: text, refinement/add and Fourier each exited 134 at the expected assertion.
   Group greens: lifecycle 32, text 79, sum 627, Fourier 37742, caps 65 checks.
   A test initially passed digits=0, which violates text.h; it was corrected to ADF_DIGITS_DEFAULT.

3. Final SAN build and test:

       timeout 180 make -j2 BUILD=lanes/f4-slice1/san SAN=1 lanes/f4-slice1/san/test_ffun
       ASAN_OPTIONS=detect_leaks=0 timeout 120 lanes/f4-slice1/san/test_ffun

   Both exit 0; 57465 checks, 0 sanitizer diagnostics.
   The initial `ASAN_OPTIONS=detect_leaks=1 timeout 120 lanes/f4-slice1/san/test_ffun` exited 1:
   LeakSanitizer reported its fatal ptrace incompatibility. No LeakSanitizer leak count is claimed.
   The alternative init/clear hook check observes 1000 allocations and 1000 frees.

4. Final INV build and test:

       timeout 180 make -j2 BUILD=lanes/f4-slice1/inv INV=1 lanes/f4-slice1/inv/test_ffun
       timeout 120 lanes/f4-slice1/inv/test_ffun

   Both exit 0; 57516 checks. Seventeen child processes produce the expected SIGABRT.
   Cap+1 still returns LIMIT before an invalid canonical-input check.

5. Final Clang build and test:

       timeout 180 make -j2 BUILD=lanes/f4-slice1/clang CC=clang lanes/f4-slice1/clang/test_ffun
       timeout 120 lanes/f4-slice1/clang/test_ffun

   Both exit 0; 57465 checks.

6. Final combined build and test:

       timeout 180 make -j2 BUILD=lanes/f4-slice1/combined CC=clang SAN=1 INV=1 \
         lanes/f4-slice1/combined/test_ffun
       ASAN_OPTIONS=detect_leaks=0 timeout 120 lanes/f4-slice1/combined/test_ffun

   Both exit 0; 57516 checks. The fault binaries use this configuration and its library/support objects.

7. Final text regressions were built in the plain lane build with make -j2 and timeout 180.
   Each command below exited 0 with 0 failed checks and 0 failed tests:

| Command | Tests | Checks |
|---|---:|---:|
| timeout 120 lanes/f4-slice1/build/test_text_adele | 20 | 84390 |
| timeout 120 lanes/f4-slice1/build/test_text_classify | 8 | 20776 |
| timeout 120 lanes/f4-slice1/build/test_text_fball | 10 | 21545 |
| timeout 120 lanes/f4-slice1/build/test_text_idele | 14 | 73465 |
| timeout 120 lanes/f4-slice1/build/test_text_limits | 11 | 105 |
| timeout 120 lanes/f4-slice1/build/test_text_local | 10 | 68868 |
| timeout 120 lanes/f4-slice1/build/test_text_r3r9 | 8 | 148 |
| timeout 120 lanes/f4-slice1/build/test_text_rat | 13 | 18122 |

   The same eight programs had passed once before the final reference-loader additions.
   Their final build command named all eight binaries under BUILD=lanes/f4-slice1/build.

8. Driver checks:

       timeout 180 make -j2 -C tools/adf
       timeout 30 build/adf < tests/driver/ffun-fourier.cmd
       timeout 30 build/adf < tests/driver/ffun-add.cmd
       timeout 30 build/adf < tests/driver/ffun-values.cmd
       timeout 180 sh tests/test_driver.sh

   Build exit 0. Each new fixture exits 1 as its error rows require, with 4,4,7 lines respectively;
   diffs against the expected files exit 0. The pre-implementation Fourier fixture gave 4 PARSE lines, exit 1.
   The full driver script passed twice: final exit 0, 82 cases, 101468 expected lines.
   Its 100000-line case took 0.079 s in the final run.
   Direct shell calls for ffun, ffun_add with two quoted operands, and ffun_fourier each exited 0.
   The final Fourier shell call returned the hand-derived (1,4) array [1,-i,-1,i].

9. Export and Julia checks:

       timeout 180 sh tests/test_exports.sh
       JULIA_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 \
         LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
         timeout 60 julia --startup-file=no tests/julia/ffun.jl build/libadelefeld.so
       timeout 10 sh -n tests/test_julia.sh

   All exit 0. Export check: 537/537 functions exported, 0 missing, 0 undeclared, 0 variadic.
   Julia ran twice; final 20/20 tests. The system-GMP preload is the repository's existing Julia workaround.
   The registered full Julia harness was syntax checked; only this slice's Julia program was run here.

10. Valid phase-cap failure probe:

        timeout 30 cc -Iinclude -std=c11 -O1 -g -Wall -Wextra -Werror \
          lanes/f4-slice1/cap_phase_probe.c build/drv-plain/libadelefeld.a -lflint -lgmp -lm \
          -o lanes/f4-slice1/build/cap_phase_probe
        timeout 150 lanes/f4-slice1/build/cap_phase_probe

    Both exit 0. Setter status 0, Fourier status 1, unchanged=1, CPU time 0.020 s.
    The case is now in test_ffun, including output equal to input.

11. Scratch faults:

        timeout 180 python3 -B lanes/f4-slice1/plant_faults.py
        ASAN_OPTIONS=detect_leaks=0 timeout 180 python3 -B lanes/f4-slice1/plant_faults.py --san

    Initial plain run: exit 1, 11 SIGABRT and 1 compile failure from unused h in the wrong-denominator fault.
    Corrected plain run: exit 0, 12 compiled and 12 SIGABRT.
    Successive SAN+INV runs: exit 0 with 12,15,16 compiled faults; every run result is SIGABRT.
    The last run's exact compile commands and assertion/INV diagnostics are in fault-results.json.
    Each fault compile and each test process is separately bounded by timeout 30.

12. Mutation commands/results appear in the next section.
    Reference-loader red: `timeout 120 lanes/f4-slice1/build/test_ffun fourier` exited 134 after a new
    assertion exposed lost non-dyadic midpoint rounding error. Adding the supplied radius instead of replacing
    that error gave the final green. No library assertion or mathematical statement was weakened.

13. Oracle discrepancy probes used timeout 10 on the Python oracle and on build/adf.
    Python returned !LIMIT; the C command exited 1 with error: DOMAIN. The C snapshot assertion also passes.
    Style/fixture scans used timeout 10 Python: 16 whole authored files plus the appended text block,
    0 lines over 116, 0 missing final newlines; vectors contain 1274 cells, 14 exact and 8 uncertain records.
    The existing long conventions table row and JSONL machine records retain their required formats.
    Cleanup used timeout 10 Python: removed build, san, inv, clang, combined, faults, and mutation scratch.
    Remaining lane build directories: 0. Removed the oversized 607115-byte harness stdout.log.
    Other retained generated logs are below 100000 bytes. No check-all or long differential fuzz run was made.

## Fault table

All final rows compile successfully under Clang SAN+INV. All test processes return SIGABRT.

| Fault | Distinguishing failure |
|---|---|
| Positive finite sign | Exact-reference containment, (2,3), k=1 |
| Weight 1/D | Exact-reference containment, (2,3), k=0 |
| Retain input layout | Required exchanged dimensions |
| Phase denominator D | Exact-reference containment, (2,3), k=1 |
| max(D) instead of lcm(D) | Required (6,6) common layout |
| max(M) instead of lcm(M) | Required (6,6) common layout |
| Fill zero-extension holes | Refined oracle cell of first input |
| Replace repetition by zero extension | Refined oracle cell of second input |
| Compare direct cap after allocation | Allocation count must be zero |
| Drop input radii | Certified uncertain corner containment |
| Drop output radii | Certified uncertain corner containment |
| Commit raw setter before final validation | INV swap sees the nonfinite tentative object |
| Commit Fourier before phase certificate failure | Destination byte snapshot changes on NOT_DETERMINED |
| Identity dimension OR becomes AND | Zero arrays differing in one dimension must not be identical |
| Addition precision forced to 2 | Exactly representable 53-bit dyadic sum must have zero radius |
| Drop first swap INV check | First-argument invalid child fails the required SIGABRT check |

## Mutation sweep and survivors

Source and tests stayed fixed during each sweep. Both sweeps used one worker, make -j2, SAN and INV,
and only tests/test_ffun.c. Copy entries were Makefile, include, src, tests and lanes.

    ASAN_OPTIONS=detect_leaks=0 timeout 1200 python3 -u tools/mutate/mutate.py --root . \
      --scratch /tmp/adf-f4-slice1-mutate --files src/ffun.c --limit 32 --seed 20261008 \
      --jobs 1 --timeout 90 --san \
      --make "make -s -j2 check INV=1 TEST_SRC='tests/test_ffun.c'" \
      --copy Makefile include src tests lanes

Exit 1. Baseline 19.4 s. From 263 candidates, 32 attempts ran in 719.9 s:
28 killed, 4 survived, 0 not compiled, 0 timed out, 0 excused.
Three survivors exposed gaps. The added tests and exact scratch mutations kill all three under SAN+INV.

The follow-up used the same command with `timeout 360` and `--limit 12`, after the test/reference additions.
Exit 0. Baseline 37.5 s. Twelve attempts ran in 296.2 s:
12 killed, 0 survived, 0 not compiled, 0 timed out, 0 excused.
It repeats the same selected prefix. Total: 44 attempts, 32 distinct mutations, 1016.1 s of tool sweeps.
No larger or exhaustive sweep is claimed. The final cap-failure test was added after these sweeps and verified
by its targeted early-commit fault and all final build variants.

Initial survivors, one line each:

- ffun.c:66, dimension || changed to &&: killed after testing identical zero arrays with one dimension different.
- ffun.c:118, add precision forced to 2: killed after asserting exact dyadic sum storage at precision 53.
- ffun.c:45, first swap INV check removed: killed after adding the first-argument invalid child case.
- ffun.c:123, omit cleanup on nonfinite addition: still uncovered; no valid finite-input failure witness found.

The last survivor affects a defensive failure cleanup branch. It is retained in the implementation.
It is not excused as equivalent and is not reported as killed. LeakSanitizer cannot supply branch coverage here.

## Findings against the specification

No counterexample to SPEC 7's three finite formulas was found. No specification edit was made.
F4 is tested as an explicit conservative coordinate allowance, including rectangular propagation and rounding;
no claim is made that the stored rectangle's circumscribed disk attains an ideal disk bound.

## Findings against the design, oracle and goldens

No declaration needs a HEADER-FINDING. The bare-scalar driver example in the brief is outside the fixed
complex-entry grammar at conventions 9.2:1192 and the scalar rejected golden row. The equivalent complex form
is implemented and tested. No grammar extension was introduced.

The oracle's proto/text_grammar.py:601-604 adds a significant-digit shortcut that changes one status:

    ffun(D=0, M=99999999999999999999999999999999999999999999999999; (0) + (0)*i)

Full grammar succeeds, item count is 1, decimal limits are satisfied, and DM=0 is below max_items.
The positive-dimension domain rule therefore gives DOMAIN. The oracle returns LIMIT because digit lengths
sum to more than 40. C follows the stated product/domain rules. This is a finding against the oracle,
not against the fixed goldens. All 18 goldens agree with C. No oracle or golden file was changed.

The independent transform oracle can retain a positive reference radius around an exactly rational coordinate.
At (D,M)=(3,2), k=1 in the generated real array, the real coordinate is exactly -5/8.
C can produce that coordinate exactly and cannot contain a strictly wider reference interval.
The generator isolates rational coordinates by exact cyclotomic reduction. Other coordinates remain certified
balls. The C test loader also now retains its own rational-conversion error. Neither issue required a library fix.

## Sources pending and work not done

Sources pending for this slice: none. The character convention and ball enclosure sources were read on disk:
refs/src/tate-poonen/notes.txt:693-700,733-740; refs/src/flint-3.0.1/arb.rst:6-12,
acb.rst:6-12; memory.rst:9-24 for the observed allocator hooks. F1-F4 and analysis P4 supply the own proofs.
The design's broader analytic source obligations remain outside this finite slice.

All requested slice-4a operations, user calls, vectors, statements, regressions and bounded fault work are done.
LeakSanitizer coverage and the nonfinite-addition cleanup branch remain unmeasured. No long differential fuzzing
or exhaustive mutation sweep was performed. Later slices 4b-4g are outside this lane.
