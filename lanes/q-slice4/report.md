# q-slice4 report

Slice 3.1-d is implemented in the owned paths. The new C programs pass in the five configurations below.
The requested direct driver call works. The new Julia call passes nine tests.
There are two retained contract discrepancies and stale read-only integration expectations.
The complete integration checks therefore do not pass. No specification, golden, or oracle was changed.

## Work by step

A. `gen_vectors.py` generates 100 array/union records, 29 text golden records, and 18 dump records.
Their sizes are 63398, 4562, and 3252 bytes: 71212 bytes total, below 400 KB.
Array keys use exact Fraction endpoints and `quotient3_checks.Piece/piece_key`.
Text and dump expectations use `proto/text_grammar.py`.
The text records also contain exact input endpoints and independent C rounding/printing expectations.
Dump records cover lifts and 1, 2, 7 pieces; global, repeated-context, and multiple-context finite parts.

B. `test_qclass_text` covers all 29 data rows of qclass.tsv, exact keys/order/count for all 100 array vectors,
raw counts 1, 2, 1000, pre-deduplication limits, wrong order, first-backend retention, and raw local cancellation.
It checks DOMAIN/LIMIT byte preservation, non-finite entries, two contexts, resource edges, and text stage order.
It checks nearest rounding at two bits and the numerical precision cap before input access.
`test_qclass_dump` covers all 13 qclass dump golden rows and all 18 dump vector records.
It checks strict order/duplicates, raw fields, descriptors/capacities, binding counts/order, and self reload.
It checks truncated buffers without a terminating NUL, malformed counts/bytes, zero counts, and exponent edges.
It interposes arb_load_str so an unsafe call is an assertion failure.
Each run performs 2000 dump/load identity round trips and 2000 printed-text enclosure rereads.
The random classes alternate constructor and reduction results, with local inputs.
Both programs check their new INV preconditions by expected SIGABRT in child processes.

C. The constructor validates raw entries, uses bounded canonical global keys, sorts stably, and retains raw storage.
The union reader reuses the existing lexer. It checks exact decimal midpoints, rounds to nearest, and encloses
the exact input intervals. A conservative decimal preflight prevents huge caller limits from reaching powers.
The four typed dump functions reuse the shared validator, bind contexts in traversal order, and preserve raw data.
They build a temporary and exchange it into the output only after validation and binding checks.
The shared validator and other typed loaders were not changed.
The two headers contain the constructor and four dump declarations with status, alias, cost, and source comments.

D. The driver dispatches qclass dump/load through the typed functions.
The brief's print spelling was absent: the driver had show. I added print as its alias and the direct argv call.
The two new fixtures were written before their runs; their expected 7 and 8 lines match byte for byte.
`tests/julia/qclass_text.jl` uses exported layout queries, GC.@preserve, and finally blocks for every owned string
and class. Its registration is in tests/test_julia.sh.

E. The appended Slice 3.1-d section of docs/api-3a.md states contracts, stages, limits, costs, decisions,
and checks.
It includes the integer proof of the conservative decimal preflight and the two retained discrepancies.

F. All eight required scratch faults compiled and failed their tests, including the final repetition.
The scoped mutation tool ran 60 mutants. Five original survivors were subsequently killed by added tests/probes.
Seven survivors remain listed below. Scratch sources and build trees were removed.

## Files written

- src/qclass.c; include/adelefeld/qclass.h, constructor declaration only.
- src/text.c, adf_qclass_set_str and its attached comment only.
- src/dump.c, added qclass functions only; include/adelefeld/dump.h, qclass declarations only.
- tests/test_qclass_text.c; tests/test_qclass_dump.c.
- tests/ref/vectors/q-slice4/{pieces,text,dump}.jsonl.
- tools/adf/adf.c, qclass dispatch and the required print spelling/direct-call branch; its README lines.
- tests/driver/qclass-text.{cmd,out}; tests/driver/qclass-dump.{cmd,out}.
- tests/julia/qclass_text.jl; its registration in tests/test_julia.sh.
- docs/api-3a.md, append only.
- Lane scripts, probes, redgreen.md, check logs, fault/survivor summaries, and this report.

## Commands and results

Every executed test program/script had a timeout. Builds used make -j2 or a single compiler.
Development runs and their intermediate counts are recorded in redgreen.md.
The exact expanded final build/test commands and outputs are in own-final-checks.log and final-checks.log.

1. `timeout 30 python3 lanes/q-slice4/gen_vectors.py`: exit 0, 100/29/18 records, 71212 bytes.
   It was run initially and regenerated after adding C rounding expectations; final regeneration has these sizes.

2. The final configuration command was:

       timeout 180 python3 lanes/q-slice4/checks.py --own-only

   Script exit 0. For each row it ran the following commands sequentially, with the row's make flags:

       timeout 180 make -s -j2 BUILD=lanes/q-slice4/CONFIG FLAGS \
         lanes/q-slice4/CONFIG/test_qclass_text lanes/q-slice4/CONFIG/test_qclass_dump
       ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/q-slice4/CONFIG/test_qclass_text
       ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/q-slice4/CONFIG/test_qclass_dump

   | CONFIG | FLAGS | Build/text/dump exits | Text checks | Dump checks |
   |---|---|---|---|---|
   | build | none | 0/0/0 | 8309 | 33854 |
   | san | SAN=1 | 0/0/0 | 8309 | 33854 |
   | inv | INV=1 | 0/0/0 | 8311 | 33856 |
   | clang | CC=clang | 0/0/0 | 8309 | 33854 |
   | san-inv | CC=clang SAN=1 INV=1 | 0/0/0 | 8311 | 33856 |

   All five dump runs have 2000 identity and 2000 enclosure round trips.
   Earlier detect_leaks=1 SAN runs of both programs exited 1: LeakSanitizer cannot run under ptrace here.
   ASan/UBSan runs then used detect_leaks=0. A counted complete FLINT allocation lifetime ends at zero live blocks.

3. `timeout 180 python3 lanes/q-slice4/checks.py`: exit 0 for the harness.
   It records expected integration failures instead of stopping at them. Its first five configurations passed,
   before the final low-precision/byte-preservation test additions; their text count was 8301/8303.
   It built and ran every tests/test_dump*.c, test_text*.c, test_qclass*.c program from the lane build.
   Build exit 0; each program ran under timeout 60. Eighteen programs: 16 exit 0, two nonzero.

   | Program suffix | Exit | Completed checks or failure |
   |---|---|---|
   | dump | 0 | 12259 |
   | dump_ctx | 0 | 28587 |
   | dump_golden | 0 | 761 |
   | dump_limits | 0 | 297 |
   | dump_local | 0 | 36204 |
   | dump_units | 0 | 32505050 |
   | qclass | 1 | test_qclass.c:401 expects union UNSUPPORTED |
   | qclass_dump | 0 | 33854 |
   | qclass_reduce | SIGABRT (-6) | line 456 expects printed union reread UNSUPPORTED |
   | qclass_text | 0 | 8301 at this run; final count 8309 above |
   | text_adele | 0 | 84390 |
   | text_classify | 0 | 20776 |
   | text_fball | 0 | 21545 |
   | text_idele | 0 | 73465 |
   | text_limits | 0 | 105 |
   | text_local | 0 | 68868 |
   | text_r3r9 | 0 | 148 |
   | text_rat | 0 | 18122 |

4. `timeout 180 sh tests/test_driver.sh`: exit 1 at read-only fixture 13_dump.
   Exactly one shown line differs: qclass lift dump bytes replace error: UNSUPPORTED.
   The script stops there; no claim is made for its remaining fixtures.
   New fixtures were also run directly:

       timeout 30 build/adf < tests/driver/qclass-text.cmd
       diff -u tests/driver/qclass-text.out lanes/q-slice4/driver-text-final.out
       timeout 30 build/adf < tests/driver/qclass-dump.cmd
       diff -u tests/driver/qclass-dump.out lanes/q-slice4/driver-dump-final.out
       timeout 30 build/adf print 'union((0.5 ; 7)) + Q'

   Fixture processes exit 1 as their error rows require; 7/8 expected lines; both diffs exit 0.
   The direct call exits 0 and writes union((0.5 ; 7)) + Q.
   Driver build command: `timeout 180 make -s -j2 -C tools/adf`, exit 0.
   Initial dump/print fixture reds and later greens are recorded in redgreen.md.

5. Shared library and Julia:

       timeout 180 cc -shared -fPIC -Iinclude -std=c11 -O1 -g -Wall -Wextra \
         src/*.c -o lanes/q-slice4/build/libadelefeld.so -lflint -lgmp -lm
       JULIA_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
         timeout 60 julia --startup-file=no tests/julia/qclass_text.jl \
         lanes/q-slice4/build/libadelefeld.so
       timeout 10 sh -n tests/test_julia.sh

   All exit 0; Julia 9/9 tests. The shared build emits an existing warning in src/lpow.c:259 about k.
   That source is read-only and was not changed. The complete Julia harness and export script were not run.

6. `timeout 180 python3 lanes/q-slice4/differential.py`: exit 0.
   Seed 310408; 30000 dump texts, 26206 edits, 3761 valid inspections, 1495 successful global loads.
   Zero status, context-count, untouched-output, or dump-byte mismatches against proto/text_grammar.py.
   This was a plain shared-library differential. SAN malformed-buffer checks are in the C program.

7. Retained failing contract checks:

       timeout 60 lanes/q-slice4/build/test_qclass_text --strict-golden
       timeout 60 lanes/q-slice4/build/test_qclass_dump --strict-zero-arch
       timeout 30 python3 lanes/q-slice4/golden_exact.py

   All exit 1. The first two retain their assertions. The last checks all 29 rows, with 12 matching status rows
   and six literal equality differences. Details are in golden-exact.log and the findings below.

8. Probe compile/run commands:

       timeout 30 cc -std=c11 -O2 -Iinclude -Isrc lanes/q-slice4/comparator_probe.c \
         lanes/q-slice4/build/libadelefeld.a -lflint -lgmp -lm -o lanes/q-slice4/build/comparator_probe
       timeout 10 lanes/q-slice4/build/comparator_probe
       timeout 30 cc -std=c11 -O2 -Iinclude lanes/q-slice4/decimal_bound_probe.c \
         lanes/q-slice4/build/libadelefeld.a -lflint -lgmp -lm -o lanes/q-slice4/build/decimal_bound_probe
       (ulimit -v 131072; ulimit -c 0; timeout 5 lanes/q-slice4/build/decimal_bound_probe)

   Comparator compile/run exit 0, self comparison 0.
   Decimal probe before the preflight: compile 0, run 136 (SIGFPE). After it: compile/run 0, status LIMIT.

9. Fault and mutation commands/results are below. `timeout 10 python3 lanes/q-slice4/style_check.py`:
   before this report, exit 0, 26 authored files/blocks, zero lines over 116, all final newlines present.
   JSONL records and verbatim machine output/command logs retain their single-line formats.
   `timeout 20 python3 lanes/q-slice4/cleanup.py`: exit 0, nine trees removed; follow-up exit 0, zero left.
   The oversized harness stdout.log was replaced with its final 80000-byte tail.
   Full check result logs remain separate. Shared root build/ from the required driver check was left alone.

## Fault table

`timeout 180 python3 lanes/q-slice4/plant_faults.py`: exit 0, run initially and again against final implementation.
Each scratch source compiled/linked with clang, SAN=1, INV=1; each test command had timeout 60.
The script contains the exact compiler/link commands. Final outcomes are in fault-results.txt.

| Fault | Test failure | Exit |
|---|---|---|
| F1 limit after deduplication | Raw n-1 limit returns wrong status | 1 |
| F2 sort/dedup raw finite keys | Canonical H ordering/count of local/global entries differs | 1 |
| F3 retain second duplicate | First local backend/context is lost | 1 |
| F4 normalize/sort dump input | Out-of-order golden dump is accepted | 1 |
| F5 FLINT load before byte validation | Interposed arb_load_str assertion | 1 |
| F6 bind wrong occurrence | Multiple-context golden load fails its expected OK | 1 |
| F7 M1-D9 bound plus one | Above-bound golden status differs | 1 |
| F8 write x before last check | Output struct bytes differ on a failing binding | 1 |

## Mutation sweep and survivors

`timeout 20 python3 lanes/q-slice4/prepare_mutation.py`: exit 0.
`timeout 180 make -s -j2 -C lanes/q-slice4/mutroot check SAN=1 INV=1`: exit 0.
Unchanged definitions were copied verbatim into included scratch headers. Candidate .c files contained only
the constructor/new helpers, added qclass dump functions/helpers, and changed qclass reader.
Every mutant still compiled the three full translation units and ran the two complete new programs.
The mutation tool itself and equivalent.txt were not changed.

    ASAN_OPTIONS=detect_leaks=0 timeout 900 python3 -u tools/mutate/mutate.py \
      --root lanes/q-slice4/mutroot --scratch /tmp/adf-q-slice4-mutate \
      --files src/qclass.c src/dump.c src/text.c --limit 60 --seed 310407 --jobs 1 --timeout 45 \
      --san --make 'make -s -j2 check INV=1' --copy Makefile include src tests lanes --keep

Tool exit 1. Baseline 4.0 s. Of 239 candidates, 60 ran in 283.4 s:
44 killed, 12 survived, four not compiled, zero timed out, zero excused.
The candidate snapshot preceded the later conservative decimal-power guard.
That guard has the bounded red/green crash probe and unit checks, but no newly selected tool mutants.

`timeout 120 python3 lanes/q-slice4/retest_survivors.py`: exit 0; five original survivors retested, five exits 1.
Three test gaps were repaired: isolated exponent precedence, freed key storage, and last ordered-block binding.
The two comparator mutations were killed by a component self-comparison probe.
No additional mutation candidate was selected. The retained seven are listed one per line here:

- text scratch:52, i+1 -> i+0: the ignored final comma attempt does not affect the already validated grammar.
- text scratch:42, allocation / -> *: huge-count allocation overflow guard not materialized by lane tests.
- dump scratch:29, allocation > -> >=: exact huge allocation boundary not materialized by lane tests.
- qclass scratch:32, keep=0 -> keep=1: stores the same entries; allocates one spare slot; huge edge untested.
- dump scratch:29, n > WORD_MAX -> >=: that huge validated input count is not materialized by lane tests.
- text scratch:64, swap fmpq_add inputs: exact rational addition is commutative, including the permitted alias.
- text scratch:42, count > WORD_MAX -> >=: that huge union input count is not materialized by lane tests.

The huge-count guards are reported as untested protection, not excused as globally equivalent.
The original scratch-to-repository offsets are in mutation-map.txt; added preflight lines shifted later text lines.
Four compile failures were two mixed-logic -Werror diagnostics, a changed pointer-return token, and an unused ctx
after deleting dp_set_fb. They are not test kills.

## Findings against the specification

No new mathematical counterexample to SPEC was found in this slice.
The storage and enclosure reading follows CV-45 and N-D21. No set-query or arithmetic statement is claimed.

## Findings against the design, goldens, and existing integration checks

1. Literal equality for every C text golden conflicts with conventions 11.3's real enclosure rule.
   The exact Python golden remains correct. At 128 bits, six valid rows differ after reading/printing:
   data rows 2, 3, 8, 9, 14, 16. Row 2 prints 1.1e-5 instead of 1e-5; row 3 prints 0.11 instead of 0.1.
   The wrapping rows print 0.051 instead of 0.05. Row 14 prints radii 0.11 and 0.21.
   Row 16 prints two distinct balls, radii 0.11 and 0.1, instead of the exact oracle's one printed duplicate.
   Dropping the larger C enclosure or printing its smaller original decimal radius would violate enclosure.
   Strict failing equality remains available.
   Ordinary tests check independently predicted C text and exact endpoints.

2. Shared dump validator/oracle stage discrepancy: adf1 Q qclass pieces 1 0 g 0 1 1 returns PARSE.
   Its grammar is a piece with arch count zero. Conventions 10.1 says another arch count gives DOMAIN.
   The shared piece-count preflight assumes eight following tokens per piece and rejects this short body first.
   A longer zero-arch local body reaches DOMAIN. The common validator is read-only for this lane.
   The retained strict-zero-arch assertion exits 1. A repair should use the grammar's minimum token count before
   domain validation; this lane did not alter the shared validator or the oracle.

3. Read-only tests/test_qclass.c and test_qclass_reduce.c still require valid union input to be UNSUPPORTED.
   Both now fail at those assertions. Read-only driver fixture 13_dump expects no qclass dump and stops the full
   driver check. These expectations need the orchestrator's integration update; they were not weakened here.

4. The brief says the driver already has print; the operation table had show. The explicit requested print call
   required its alias and direct argv branch. This choice is documented in api-3a.md and the driver README.

5. A huge caller max_exp10 exposed a SIGFPE before the new decimal preflight.
   The exact reproducer is decimal_bound_probe.c. It now returns LIMIT before powers, without output writes.

## Not done, sources pending, and costs

No set queries, class arithmetic, or class character were implemented.
Exact C golden equality is not claimed for the six enclosure differences.
The shared zero-arch validator discrepancy and read-only stale integration expectations are not repaired.
LeakSanitizer could not run. No allocation-failure injection or exhaustive byte corpus is claimed.
The full Julia harness, export script, and check-all were not run.
The new Julia target and script syntax were checked.
The four dump functions shared a first link red.
No separate initial assertion red was run for each entry point.
No tool mutation candidate was selected in the final decimal preflight; its red/green probe and unit checks ran.

Sources read for new FLINT calls are on disk: refs/src/flint-3.0.1/{arf,arb,fmpz,memory,mag}.rst.
Sources pending: the reused tx_mag_upper comment's exactness claim for mag_set_ui_2exp_si below 2^30 remains
probe-backed. mag.rst:149-151 promises an upper bound. This is sufficient for enclosure; dyadic tightness is tested.
This lane did not claim that the documentation proves that extra exactness property.

Avoidable costs: constructor preflight forms endpoints that sorting then compares again with arf_sum.
Canonical finite centre/radius access can reconstruct a local triple twice. The dump loader repeats canonicality
on the built temporary after shared validation. None of these were optimized.

Run-limit deviation: the mutation command and an initial regression build were briefly started together.
Each used make -j2, so I cannot certify a two-compiler total during that overlap. Subsequent builds were sequential.
No installation, git state change, tracker command, or sub-agent delegation was performed.

Final report style check: 27 authored files/blocks, zero overlong lines, all final newlines present.
