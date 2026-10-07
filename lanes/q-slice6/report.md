# Lane q-slice6 report

Completed slice 3.1-e. Date: 2026-10-08, Asia/Singapore.
The three queries, C tests, driver commands, Julia call test and API statements are implemented.
No specification file was changed. No git command or bd command was run.

## Work by step

A. Generated 279 exact vector pairs and one huge-denominator refusal row.
The two vector files total 152142 bytes, below 400000 bytes.
Families include 68 glued and wrong-sign pairs, refinements, unequal containment, boundary intersections,
spill, finite points, real points, negative centres, 2000-bit centres, mixed moduli and fractional radii.
The generator checks 414620 direct membership pairs and 558 independent containment decisions.
It imports compare/brute_sets from proto/quotient3_checks.py and interval covering from q-review3/model.py.
The C suite also checks 1305 stored rational membership labels through direct diagonal translation.

B. Tests cover every vector, all queries, both orders, budget and budget-1, zero and negative limits,
identity aliasing, relation implications, input preservation, untouched truth, local canonical triples,
exactness without prec, and six invalid-input aborts in INV builds.
They check zero bulk allocations on budget refusal, 2000 pieces with L=LONG_MAX-1, saturation at LONG_MAX,
an impossible 2^59 construction, and numerator/denominator/exponent bounds.
Each query call checks balanced bulk allocations and initialized fmpz objects.
New bulk memory is poisoned in the test harness, so incomplete initialization cannot rely on zero-filled memory.

C. Implemented src/qclass_sets.c, with the declarations appended at the end of qclass.h's declarations.
The private exact reader/add kernel and R steps are copied minimally from src/qclass.c; that file is unchanged.
There is no Q1 call or real rounding in the query implementation.
B is checked before the fibre loop. Raw counts saturate in fmpz at work_limit+1.
K counts both inputs before deduplication. E counts actual exact piece endpoints, not artificial cell boundaries.
The full (2E+1) K L budget is checked before bulk allocation, including on identical inputs.
Impossible byte products are refused before streaming endpoints.
Endpoint discovery uses O(K E) streaming work before allocation. Point candidates are sorted once globally.
Empty exterior cells are skipped; glued real zero is retained. The fibre count is at most 2E+1.
Queries decide residue unions and exact integer point fibres by Q2, with the glue m-1 at real zero.

D. Added qequal, qcontains and qoverlaps with two class operands and an integer work limit.
The driver fixture has 22 hand-derived lines, including the modulus-3 glued pair and its budget 42/41 boundary.
Added and registered tests/julia/qclass_sets.jl. Its call signatures use exported storage size/alignment.

E. Appended Slice 3.1-e to docs/api-3a.md: represented-set meanings, preservation by exact R, budget,
preflight, statuses, cost, checks and decisions with alternatives.

F. Eleven planted faults compile and fail. The two tool sweeps judge 40 and 12 selections.
Five original mutation survivors and the one final-sweep survivor were killed by added component checks.
Three original survivors remain equivalent; each is explained below.

## Files written

- src/qclass_sets.c
- include/adelefeld/qclass.h: appended declarations and contract only
- tests/test_qclass_sets.c
- tests/ref/vectors/q-slice6/sets.jsonl and limits.jsonl
- tools/adf/adf.c: three command entries and their dispatch only
- tools/adf/README.md: appended query documentation only
- tests/driver/qclass-sets.cmd and qclass-sets.out
- tests/julia/qclass_sets.jl
- tests/test_julia.sh: the new test block only
- docs/api-3a.md: append only
- lanes/q-slice6/gen_vectors.py, differential.py, plant_faults.py, check_survivors.py,
  check_size_survivor.py, mutation_check.sh, redgreen.md, this report, and the named check logs/results

The Makefile discovers the new source and test by wildcard. It was not changed.

## Checks and commands

All test programs and scripts were bounded by timeout. Builds used at most two jobs.
The final C test matrix used the following commands for each row, with its listed flags:

```sh
timeout 180 make -s -j2 BUILD=lanes/q-slice6/CONFIG FLAGS \
  lanes/q-slice6/CONFIG/test_qclass_sets lanes/q-slice6/CONFIG/test_qclass_reduce
timeout 180 lanes/q-slice6/CONFIG/test_qclass_sets
timeout 180 lanes/q-slice6/CONFIG/test_qclass_reduce
```

For SAN rows the run commands were prefixed with ASAN_OPTIONS=detect_leaks=0.
Final test-only additions rebuilt and reran test_qclass_sets in all five configurations.
They did not change the implementation or reducer. Every listed final build and run exited 0.

| CONFIG | FLAGS | Query checks | Query calls | Reducer checks | INV aborts |
|---|---|---:|---:|---:|---:|
| plain | none | 86357 | 8394 | 223472 | 0 |
| san | SAN=1 | 86357 | 8394 | 223472 | 0 |
| inv | INV=1 | 86375 | 8394 | 223475 | 6 |
| clang | CC=clang | 86357 | 8394 | 223472 | 0 |
| clang-san-inv | CC=clang SAN=1 INV=1 | 86375 | 8394 | 223475 | 6 |

The reducer reads 213 vectors and 8520 rational point labels. It also checks 61 lifts with 1060 exact pieces.
The query suite reads 279 set vectors and the single huge-B row in every configuration.

| CONFIG | 10^100 B refusal, CPU seconds | 2^59 byte-product refusal, CPU seconds |
|---|---:|---:|
| plain | 0.000003 | 0.000023 |
| san | 0.000006 | 0.000052 |
| inv | 0.000004 | 0.000028 |
| clang | 0.000003 | 0.000027 |
| clang-san-inv | 0.000006 | 0.000042 |

Both time guards are 1 second. The 2000-piece refusal makes zero bulk allocation calls in all five builds.

Additional checks:

1. `timeout 180 python3 lanes/q-slice6/gen_vectors.py`: exit 0; 279 set vectors, 151816 bytes,
   414620 direct membership pairs, 558 independent containment checks, one huge-B refusal.
   Repeated at the end with the same counts; total fixture size 152142 bytes.
2. `timeout 180 sh tests/test_driver.sh`: exit 0; 73 cases, 101370 expected lines, SAN=0.
   Final 100000-line timing: 0.063 seconds. The new fixture contributes 22 lines.
3. `timeout 30 build/adf < tests/driver/qclass-sets.cmd` followed by `diff -u` against its .out:
   the expected driver exit is 1; diff exits 0 with all 22 lines equal.
4. `timeout 180 cc -shared -fPIC -Iinclude -std=c11 -O1 -g -Wall -Wextra src/*.c
   -o lanes/q-slice6/libadelefeld.so -lflint -lgmp -lm`: exit 0.
   It reports one pre-existing maybe-uninitialized warning at src/lpow.c:259; that file is read-only here.
5. `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 timeout 60 julia --startup-file=no
   tests/julia/qclass_sets.jl lanes/q-slice6/libadelefeld.so`: exit 0; 32 tests pass.
6. `timeout 10 sh -n tests/test_julia.sh`: exit 0. The full Julia test script was not run.
7. `timeout 180 python3 lanes/q-slice6/differential.py lanes/q-slice6/libadelefeld.so 120`:
   exit 0; seed 310607, 109214 pairs, 1310568 calls, 1093 brute grids, 343144 membership pairs,
   zero disagreements, 120.000 seconds.
8. After the final implementation changes,
   `timeout 90 python3 lanes/q-slice6/differential.py lanes/q-slice6/libadelefeld.so 30`:
   exit 0; 47917 pairs, 575004 calls, 480 brute grids, 152536 membership pairs,
   zero disagreements, 30.001 seconds.
9. `timeout 180 python3 lanes/q-slice6/plant_faults.py`: final exit 0; eleven compiled,
   eleven killed, zero survivors, zero compile failures. See the fault table.
10. `timeout 180 python3 lanes/q-slice6/check_survivors.py`: exit 0; five killed,
    three expected equivalent survivors, zero compile failures.
11. `timeout 180 python3 lanes/q-slice6/check_size_survivor.py`: exit 0; one killed,
    zero survivors, zero compile failures.
12. Inline Python audits under timeout 10: zero over-116 lines in new code/scripts, zero missing final newlines,
    zero files left in mutation scratch, and no fault-scratch directory.
    JSONL records retain the required one-record-per-line format; diagnostic logs retain tool output.

### Red runs and environment failures

The complete red/green history is in redgreen.md. These failures were not counted as passing checks:

- Initial generator: exit 1 on samples-not-coset-3. The grid was too small to leave a coset integer
  outside the finite point exceptions. The grid was expanded; neither oracle was changed.
- Initial C make target: exit 2, three undefined symbols. Subsequent containment and overlap reds
  exited 134 on the glue-2-0 vector with temporary LIMIT stubs. Equality/containment/overlap then passed.
- Implementation compilation caught a label typo, uninitialized K/L diagnostics and GCC's member-array warning.
  The reader now takes separate real and finite pointers, matching the reducer's existing pattern.
- Initial INV test build: exit 2 because freopen's result was unchecked. The test now checks it.
- A negative exponent test initially used the shift rather than ARF's stored exponent. It was corrected
  to exceed the actual bound. An initial 2^58 byte-size test fit the smaller query piece struct;
  the impossible construction is 2^59 for this layout.
- Default SAN runs of both programs: exit 1 with LeakSanitizer's fatal ptrace error.
  The reported SAN passes disable leak detection. AddressSanitizer and UBSan remain enabled.
  Explicit bulk and integer-lifecycle balance checks catch the cleanup mutations without LeakSanitizer.
- Julia without preload: exit 1, FLINT cannot resolve __gmpn_modexact_1_odd from Julia's bundled GMP.
  The documented system-GMP preload passed. The registered block has the existing conditional retry.
- Adding the literal point-comparison fault initially produced one compile failure: yx became unused.
  A void read of yx made that wrong implementation compile, after which its truth assertion failed.

## Fault table

Command: `timeout 180 python3 lanes/q-slice6/plant_faults.py`.
Each scratch executable uses clang, SAN=1, INV=1 and its own timeout 15.
Negative return codes below are Python subprocess signal codes, not shell exit codes.

| Fault | Final result | Detecting check |
|---|---:|---|
| Refinement to one residue | -6, SIGABRT | Vector truth |
| One real sample instead of all cells | -6, SIGABRT | Vector truth |
| Unknown-point comparison substituted for set equality | -6, SIGABRT | Alias set truth |
| Glue m+1 instead of m-1 | -6, SIGABRT | Glued-pair truth |
| Missing endpoint glue | -6, SIGABRT | Glued-pair truth |
| Containment reversed | -6, SIGABRT | One-way containment truth |
| Positive coset covered by finite samples | -6, SIGABRT | Coset-versus-points truth |
| Allocation before the budget | -6, SIGABRT | Zero bulk allocations on LIMIT |
| Saturation removed | -6, SIGABRT | Exact cap+1 component check |
| Machine-word count overflow | 1, UBSan | LONG_MAX+1 signed overflow |
| Truth written on LIMIT | -6, SIGABRT | Truth sentinel |

## Mutation sweeps and survivors

The initial full-build wrapper used the following tool command with timeout 900 and seed 310606:

```sh
ASAN_OPTIONS=detect_leaks=0 timeout 900 python3 -u tools/mutate/mutate.py --root . \
  --scratch /tmp/adf-q-slice6-mutate --files src/qclass_sets.c --limit 40 --seed 310606 \
  --jobs 1 --timeout 45 --san --make 'timeout 40 sh lanes/q-slice6/mutation_check.sh' \
  --copy Makefile include src tests lanes
```

Its unmutated baseline passed in 27.4 seconds. It rebuilt unchanged dependencies for every attempt.
It was stopped by SIGTERM: exit 143, one active command killed, scratch removed.
There is no aggregate mutant count for this partial run. Its selections repeat the prefix of the complete run.
A host pkill attempt exited 1 because exec commands have separate PID namespaces.
A temporary lane-wrapper stop guard signalled the mutation process from its own namespace; the guard is removed.
Resource exception: after that failed stop attempt, the dependency make -j2 overlapped with the worker's make -j2.
Up to four compiler processes could run in that interval; peak CPU use was not measured.
This overlap did not follow the intended two-core scheduling. Later builds were serialized or used one compiler.

The revised wrapper compiles the mutated source through the complete test_qclass_sets translation unit,
with SAN and INV, linking the unchanged dependency archive prepared by:

```sh
timeout 180 make -s -j2 BUILD=lanes/q-slice6/mutation-base SAN=1 INV=1 \
  lanes/q-slice6/mutation-base/test_qclass_sets
```

The archive's qclass_sets.o is not pulled: the test translation unit defines all three query symbols.
Every mutated query therefore runs; only unrelated dependency recompilation is avoided.
The wrapper compiles under timeout 30 and runs only test_qclass_sets under timeout 15.
One mutation worker uses one compiler process. The earlier make builds use at most two jobs.

The complete first sweep repeated the command above with timeout 300.
Exit 1: 507 candidates, 40 selections, baseline 5.8 seconds; 139.8 seconds for the selections.
Results: 30 killed, eight survived, two not compiled, zero timed out, zero excused.

The final sweep used timeout 180, --limit 12 and --seed 310608, with the same remaining arguments.
Exit 1: 528 candidates, 12 selections, baseline 3.8 seconds; 27.9 seconds for the selections.
Results: ten killed, one survived, one not compiled, zero timed out, zero excused.
Across the completed sweeps there are 52 selections, at most 52 distinct mutations.
The partial run adds no distinct selection. Targeted reruns repeat already-selected semantic changes.
Mutation execution, including the stopped run and follow-ups, stayed below 20 minutes.

First-sweep survivors killed after adding tests:

- Original line 260, point initialization 2*K -> 2+K: initialization/clear balance fails.
- Line 29, projected denominator b-e+1 -> b-e+0: the BITS_MAX+1 denominator check fails.
- Original line 256, allocation refusal LIMIT -> OK: direct storage-bound status check fails.
- Original line 315, skip cleanup of side zero: bulk allocation balance fails.
- Original line 316, clear 2/K rather than 2*K integers: integer lifecycle balance fails.

Final-sweep survivor killed after adding tests:

- Line 33, qs_size && -> ||: oversized numerator/denominator component checks fail.

Remaining equivalent survivors, one line each:

- Line 141, minimum comparison < -> <=: an equal endpoint only copies the same exact value again.
- Line 64, read-size && -> ||: raw A/H/d are already bounded; global cancellation and local CRT cannot enlarge them.
- Current line 151, endpoint byte factor 2*K -> 2/K: the preceding 56*K piece bound implies the 16*(2*K+2) bound.

The last equivalence uses this 64-bit layout and admitted K>=2; no claim is made for an untested ABI.
No equivalent.txt entry was written because that path is not owned by this lane.

Compile failures, not test kills:

- Original line 303, f+1 -> f-1: GCC rejects the negative array index under -Werror.
- Original line 273, remove nends=2: GCC rejects the possibly uninitialized count under -Werror.
- Final line 161, || -> &&: GCC rejects the unparenthesized mixed Boolean operators under -Werror.

## Cleanup and remaining work

Removed six lane build trees: plain, san, inv, clang, clang-san-inv and mutation-base.
Removed the lane shared library and lane bytecode cache. Planted-fault scratch and mutation scratch are absent.
Dedicated logs and results are retained. The automatic harness stdout log is rotated to its last 60 KB.
Rebuilding mutation-base is required before rerunning the fast mutation wrapper.

No requested query work remains. LeakSanitizer could not run under ptrace; this limitation is recorded above.
The full Julia suite and check-all were not run. The new Julia test and script syntax were checked.
set_pieces, union reading, dump forms, class arithmetic and characters remain outside this slice.
Driver union input depends on the separate reader slice; this lane's C tests cover canonical PIECES directly.

Avoidable cost: streaming endpoint discovery rereads canonical finite triples, including local CRT, E times.
It keeps the budget ahead of bulk allocation and stays within O(K^2 L); no optimization was added.

## Sources pending

None for this slice. Quotient P3, P6, P8, P9/P10 and Q2 were read from the repository proof/design files.
FLINT exact conversions, comparisons and memory rules were read under refs/src/flint-3.0.1 and cited in code.

## Findings against the specification

None. SPEC.md was not changed.

## Findings against the design and oracle

No counterexample to Q2 or the implemented design was found.
E's inclusion of artificial boundaries is unstated; the slice counts actual piece endpoints, preserving E<=2K,
and skips exterior empty cells so the stated fibre bound remains valid. This choice and its alternative are
documented.
brute_sets is a finite-grid checker. The generator must include integers outside all point exceptions;
its first insufficient grid was a lane test-generation error, not an oracle correction or a profinite proof.
