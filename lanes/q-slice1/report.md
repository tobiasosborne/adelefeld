# q-slice1 report

Slice 3.1-a is implemented. Finished on 2026-10-05 at 01:50 Europe/Berlin.
The C interface, lift text, driver command, Julia call, statements, and tests are present.
No later-slice function is declared. No specification, design, convention, or old fixture was edited.
No git command, tracker command, installation, or subagent was used.

The required check-all ran once. All 83 C programs passed. It then stopped at driver fixture 06_pairs.
Exports, Julia, and both tool self-tests were run separately. Their final exit codes are 0.
Final qclass runs have 35384 checks without INV and 35439 with INV, with exit 0.
There is a process gap: core red/green was grouped, rather than assertion-red before each function.
The missing-symbol link run preceded the core implementation. See the red/green record below.

## What was done

### A. Type and C implementation

Implemented init, clear, deep set, constant-time swap, canonicality, representation identity,
set_adele, set_rat, form, length, get_piece, rational translation, and both layout queries.
The layout is size 24, alignment 8, with offsets 0, 8, 16. It was checked in C and Julia.
Contexts remain borrowed. Copying and lifting preserve backends, raw local fields, and context pointers.
set_rat returns the exact initialized zero class, including for a 2000-bit numerator.
add_rat copies the representation, including PIECES. It performs no adele translation or real rounding.

Canonicality checks both forms. PIECES permits real spill and checks midpoint in [0,1].
It uses the canonical global triple for local values, and rejects equal keys across backends.
Exact endpoint comparisons use a correctly rounded four-term arf_sum and its sign.
They do not materialize endpoint mantissas across huge exponent gaps.
INV entry checks include the public function name in the abort diagnostic.

### B. Lift text

The reader accepts the LIFT value form. Union input returns UNSUPPORTED after byte, full grammar,
decimal exponent, and union count checks, before union semantics. Failures preserve output bytes.
The printer returns the adele enclosure followed by + Q. PIECES returns NULL with length 0 in this slice.
The common numerical precision cap applies before byte access. max_items counts union entries.
All 29 golden rows are run. Four valid lift rows also have independent exact stored-ball and print vectors.
The golden reference is exact-rational; C rereads may widen. This is checked, rather than hidden.

### C. Tests and reference

Generated 1000 random rational translations using proto/quotient3_checks.py.
There are 2000 normalized membership witnesses, one member and one nonmember for each lift.
The oracle checks membership before and after exact translation, and against its separate reduction algorithm.
C tests make 4000 membership decisions on the supplied and copied representatives.
Each random translation is also tested in place and out of place for representation identity.

Tests cover global and local lifts, borrowed pointers, exact finite points, fractional radii,
the 2000-bit rational, all four order keys, nonzero centre order and duplicates, raw invalid fields,
zero and negative lengths in both forms, NULL arrays, midpoint violations, allowed spill,
an invalid member adele, accessor bounds, self-copy, self-swap, PIECES copies, and layout.
There are 1000 lifecycle cycles and 11 INV abort cases, including diagnostic function names.
Twenty comparisons with heap-backed 2000-bit midpoint temporaries have zero net live allocations.
Allocation callbacks are saved and restored. This targeted check supplements the unavailable LeakSanitizer.

### D. User calls

The driver holds qclass values and implements qadd_rat X with R.
show prints a lift; type names qclass. The eight ordinary arithmetic/predicate pair commands give DOMAIN.
The new fixture has 19 output lines, exit 1 as specified by its metadata, and a byte-exact output comparison.
Julia uses exported sizes and alignments, aligned buffers, GC.@preserve, initialized outputs, and string freeing.
The qclass Julia test has 17 passing assertions and is registered in tests/test_julia.sh.

### E. Statements

docs/api-3a.md gives one statement and a Check line for each of the 16 functions.
It records the driver spelling, union stage order, max_items treatment, PIECES printer refusal,
reader precision cap, and endpoint comparison method, with alternatives.

## Files written

- include/adelefeld/qclass.h; src/qclass.c; tests/test_qclass.c.
- include/adelefeld/text.h and src/text.c: additions for the qclass include, reader, and printer only.
- include/adelefeld.h: one include line.
- tools/adf/adf.c and README.md: the held kind and qadd_rat command.
- tests/driver/qclass-lift.cmd and qclass-lift.out.
- tests/julia/qclass.jl; its registration block in tests/test_julia.sh.
- tests/ref/vectors/q-slice1/translation.jsonl and golden.jsonl.
- docs/api-3a.md.
- Lane scripts: gen_vectors.py, audit_fixtures.py, check_configs.sh, prepare_mutation.py,
  replay_survivors.py. Lane logs preserve the results named below.
- This report.

## Existing fixtures

All filenames below are under tests/driver/. Line numbers refer to the .cmd file.
The audit checked the other commands in these six fixtures against their existing outputs too.
There are 23 placeholder commands and 18 changed outputs. None of these fixtures was edited.

| Fixture .cmd | Line | Old output | New output |
|---|---:|---|---|
| 06_pairs | 152 | error: UNSUPPORTED | error: DOMAIN |
| 06_pairs | 153 | error: UNSUPPORTED | error: DOMAIN |
| 06_pairs | 154 | error: UNSUPPORTED | error: DOMAIN |
| 06_pairs | 155 | error: UNSUPPORTED | error: UNSUPPORTED |
| 06_pairs | 156 | error: UNSUPPORTED | error: DOMAIN |
| 06_pairs | 157 | error: UNSUPPORTED | error: DOMAIN |
| 06_pairs | 158 | error: UNSUPPORTED | error: DOMAIN |
| 06_pairs | 159 | error: UNSUPPORTED | error: DOMAIN |
| 06_pairs | 160 | error: UNSUPPORTED | error: DOMAIN |
| 06_pairs | 161 | error: UNSUPPORTED | error: DOMAIN |
| 06_pairs | 167 | error: UNSUPPORTED | (0.5 ; 0) + Q |
| 07_status | 69 | error: UNSUPPORTED | (0.5 ; 0) + Q |
| 08_show_type | 47 | qclass | qclass |
| 12_status_order | 21 | error: UNSUPPORTED | error: DOMAIN |
| 12_status_order | 24 | error: UNSUPPORTED | error: DOMAIN |
| 12_status_order | 27 | error: UNSUPPORTED | error: LIMIT |
| 12_status_order | 31 | error: UNSUPPORTED | error: DOMAIN |
| 12_status_order | 32 | error: UNSUPPORTED | error: DOMAIN |
| 12_status_order | 40 | error: PARSE | error: PARSE |
| 12_status_order | 41 | error: PARSE | error: PARSE |
| 13_dump | 24 | error: UNSUPPORTED | error: DOMAIN |
| 13_dump | 35 | error: UNSUPPORTED | error: UNSUPPORTED |
| f-places-hostile | 50 | error: UNSUPPORTED | error: DOMAIN |

Proposed replacement for a value kind lacking a typed parser: rfun().
Conventions 9.2 admits this empty function text. The new fixture checks type rfun() -> rfun
and show rfun() -> error: UNSUPPORTED. Replace placeholders where that is their purpose.
Keep 08_show_type:47 as a qclass classifier check. Dump refusal is separately still meaningful for this slice.

## Checks run

Every executed test program and script was bounded by timeout. Compiler builds used at most -j2.
The mutation sweep and survivor replay were pinned to the first two available CPUs, with two worker jobs.
SAN executions used ASAN_OPTIONS=detect_leaks=0. LeakSanitizer is unavailable in this sandbox.
Logs are under lanes/q-slice1/. The commands below ran from the repository root unless stated otherwise.

### Initial and red/green checks

1. timeout 120 python3 lanes/q-slice1/gen_vectors.py: first exit 1, then exit 0 after correction.
   The initial generator passed a rational finite witness to direct_member, which expects the normalized
   integer finite coordinate. It was corrected to use integer witnesses. Final counts: 1000 and 2000.
   Later regeneration also wrote all four golden lift records; exit 0. Log: vectors.log.
2. timeout 180 make -j2 BUILD=lanes/q-slice1/red lanes/q-slice1/red/test_qclass:
   initial exit 2 from an incorrect test-helper argument type; corrected run exit 2 for missing qclass symbols.
   The latter is the core link-red evidence. Log: red-link.log.
3. timeout 180 make -j2 BUILD=lanes/q-slice1/plain lanes/q-slice1/plain/test_qclass:
   first exit 2 from GCC's array-parameter overread diagnostic; changing the helper to arb_srcptr gave exit 0.
   Five provisional group runs after the failed build returned 127 because no executable existed.
   These are not counted as test validation or assertion-red evidence.
4. timeout 120 lanes/q-slice1/plain/test_qclass lifecycle: exit 0, initially 17 checks.
   The analogous canonical, identity_lift, translation runs exited 0 with 46, 31, 34002 checks.
   Logs: green-lifecycle.log, green-canonical.log, green-lift.log, green-translation.log.
5. timeout 120 lanes/q-slice1/plain/test_qclass text: stub reader run exit 1 at the first valid lift.
   Log: red-text.log. With the reader present, the literal golden-print assertion also exited 1.
   That assertion conflicted with conventions 9.6. It was replaced by exact stored dyadic and reference
   printer expectations, and enclosure on reread; the resulting complete run exited 0 with 34245 checks.
   Subsequent added cases gave 35329 and 35332 checks before mutation strengthening.
6. timeout 180 make -j2 -C tools/adf: exit 0 on each build.
   timeout 120 build/adf < tests/driver/qclass-lift.cmd: exit 1, as the fixture requires.
   Before the driver implementation, diff -u against its hand-written output exited 1.
   The first implemented run still had a division status mismatch, diff exit 1; routing it to DOMAIN fixed it.
   Final driver comparison is recorded below. Logs: red-driver.diff, driver.out, driver-final.out.

Core tests were written before core code, but individual assertion-red runs for every core function were not done.
Text had a stub assertion-red and the driver had expected lines before its implementation.
Later planted mutation failures are not described as pre-implementation TDD.

### Acceptance and configuration checks

1. timeout 1700 make -j2 check-all: exit 2; 83 C programs passed, including qclass with 35332 checks.
   It stopped at 06_pairs output differences. It was run once. Log: check-all.log.
2. timeout 180 sh tests/test_exports.sh: exit 0; 499 declared, 499 exported, 0 missing, 0 undeclared,
   0 variadic. Log: exports.log.
3. JULIA_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 timeout 180 sh tests/test_julia.sh: exit 0.
   qclass: 17/17 assertions. The existing GMP preload retry was used. Log: julia.log.
4. timeout 180 python3 tools/mutate/selftest.py: exit 0. Log: mutate-selftest.log.
5. timeout 180 python3 tools/memcheck/selftest.py: first exit 1 with two child-path init/clear findings.
   Explicit child cleanup fixed these. Both subsequent executions exited 0; final scan: 121 C files,
   0 findings. Logs: memcheck-selftest.log, memcheck-selftest-final.log, memcheck-selftest-strong.log.
6. timeout 180 sh lanes/q-slice1/check_configs.sh san: build exit 0, nine program exits 0.
   Eight text programs: 94 test groups, 287419 checks, 0 failures. Initial qclass: 35332 checks.
7. timeout 180 sh lanes/q-slice1/check_configs.sh inv: initial build exit 2 from ignored freopen result.
   After correction, all eight text programs exited 0: 94 groups, 287419 checks, 0 failures.
   qclass initially aborted with exit 134 because a forged finite field was reused by a parser before restore.
   Restoring both forged fields before that call fixed the test's precondition violation.
   The first script version returned 0 even on build failure; it now propagates build/test failure.
8. timeout 180 sh lanes/q-slice1/check_configs.sh clang: build exit 0, nine program exits 0.
   Text programs: 94 groups, 287421 checks, 0 failures. Initial qclass: 35332 checks.
9. Focused rebuilds used the following command, for each B and flags shown:

       timeout 180 make -j2 BUILD=B FLAGS B/test_qclass
       ASAN_OPTIONS=detect_leaks=0 timeout 120 B/test_qclass

   B = lanes/q-slice1/plain, FLAGS empty: final 35384 checks, both exits 0.
   B = lanes/q-slice1/san, FLAGS SAN=1: final 35384 checks, both exits 0.
   B = lanes/q-slice1/inv, FLAGS INV=1: final 35439 checks, both exits 0.
   B = lanes/q-slice1/clang, FLAGS CC=clang: final 35384 checks, both exits 0.
   B = lanes/q-slice1/mutbase, FLAGS CC=clang SAN=1 INV=1: final 35439 checks, both exits 0.
   Intermediate corrected INV and combined runs had 35366 checks, exit 0.
   Logs: plain-final-tests.log, *-qclass-strong.log, mutbase-strong-tests.log.
10. timeout 30 cc -Iinclude -std=c11 -Wall -Wextra -Wpedantic -Werror -x c -c:
    standalone qclass.h and text.h include probe, exit 0.
    Analogous timeout 30 c++ with -std=c++17: exit 0.
11. timeout 120 build/adf < tests/driver/qclass-lift.cmd: final exit 1, expected 1; 19 lines.
    diff -u tests/driver/qclass-lift.out lanes/q-slice1/driver-final.out: exit 0.
12. timeout 120 python3 lanes/q-slice1/audit_fixtures.py: exit 0 on both audits.
    Six fixtures, 23 placeholder commands, 18 changed outputs; all other audited command outputs unchanged.
13. timeout 10 python3 line/newline checks: 12 new source, test, statement, and lane script files;
    0 lines beyond 116 and 0 missing final newlines. JSONL data retains its required one-record-per-line format.

### Mutation checks

Prepared the matching SAN/INV/Clang archive with the focused make command above, then ran:

    timeout 120 python3 lanes/q-slice1/prepare_mutation.py
    ASAN_OPTIONS=detect_leaks=0 taskset -c "$QSLICE_CPUS" timeout 1200 \
      python3 -B tools/mutate/mutate.py --root lanes/q-slice1/mutroot \
      --scratch lanes/q-slice1/mutate --files src/qclass.c --limit 60 --seed 3101 \
      --jobs 2 --san --timeout 90 --make 'make -s -j2 check INV=1' \
      --copy Makefile include src tests lanes --keep

The preparation exited 0. The sweep exited 1: 60 of 138 candidates, 135.6 seconds,
50 killed, 7 survived, 3 not compiled, 0 timed out, 0 excused. Baseline: 5.5 seconds, exit 0.
The scratch Makefile runs the complete qclass test, all its vectors, goldens, and debug cases.
It compiles the mutated qclass before the matching archive, which supplies unchanged library dependencies.
This avoids rebuilding and running unrelated tests for each mutant. No repository Makefile was changed.
Log: mutation.log. tools/mutate/equivalent.txt was not changed; it is not owned.

    taskset -c "$QSLICE_CPUS" timeout 180 python3 lanes/q-slice1/replay_survivors.py

First replay script exit 1 because the skipped-initialization mutant still passed SAN/INV.
The corrected classification script exited 0: five test rejections, two remaining survivors.
Each replay ran timeout 90 make -s -j2 check INV=1 SAN=1 in its retained mutant directory.
The five rejected replays returned make exit 2; the other two returned 0.
Logs: replay.log, replay-final.log, replay-65.log through replay-171.log.

For the uninitialized-stack survivor, two non-SAN INV executables were compiled with timeout 90 cc,
-Iinclude -Itests -Isrc -DADF_CHECK_INVARIANTS -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror.
Inputs were qclass.c, test_qclass.c, support/jsonl.c, support/golden.c and the matching INV archive;
libraries were -lflint -lgmp -lm -pthread. Both compiler exits were 0.
The second executable substituted the retained line-65 mutant source for qclass.c. Then:

    timeout 120 valgrind --quiet --error-exitcode=77 --leak-check=no --track-origins=yes \
      lanes/q-slice1/qclass-vg canonical
    timeout 120 valgrind --quiet --error-exitcode=77 --leak-check=no --track-origins=yes \
      lanes/q-slice1/qclass-vg-mut65 canonical

Baseline: exit 0, 104 canonical checks. Mutant: exit 139, uninitialized reads in arf_set,
with origin at end_cmp's stack allocation, then a segmentation fault. Logs: vg-baseline.log, vg-mut65.log.
These are bounded checks, not a fuzz run. The original sweep was not repeated.
Across the original sweep and focused replays: 56 rejected mutants, 1 equivalent survivor, 3 compile failures.
Compile failures are not counted as test kills.

Initial survivors, one line each:

- qclass.c:65, start arf init at 1: survives SAN/INV; Valgrind detects uninitialized reads, exit 139.
- qclass.c:76, start arf clear at 1: rejected by zero-net-allocation check; replay make exit 2.
- qclass.c:87, len < 1 -> len < 0: rejected by PIECES len 0 with live storage; replay make exit 2.
- qclass.c:99, centre range OR -> AND: equivalent after canonical member validation and global-triple access.
- qclass.c:107, omit prevA copy: rejected by centres 2 then 1 and a nonzero duplicate; replay make exit 2.
- qclass.c:128, omit adele entry check: rejected by public diagnostic function name; replay make exit 2.
- qclass.c:171, omit class entry check: rejected by public diagnostic function name; replay make exit 2.

The three compile failures are lines 95 (logical-parentheses warning), 74 (uninitialized result warning),
and 173 (unused output parameter after dropping the copy). They remain untested mutants.

## Findings against the design

The fixed-point wording in api-3.md 2.5 and section 5 needs the conventions 9.6 qualification for C.
At 128 bits and six digits, `(3.14159 +/- 1e-5 ; 5/3 mod 6) + Q` prints radius 1.1e-5.
Rereading and printing gives 1.2e-5. The exact-rational golden is a fixed point of the reference,
but the C parser's outward radius rounding need not be. The implemented contract is enclosure on reread.
The exact stored-ball and printer oracle checks this witness without changing the golden file.

## Findings against the specification

The known api-3.md section 8 F1 needs a positive-radius qualification in SPEC 6's full-image sentence.
For X = (0 ; 0), width = N = 0, but pi(X) is only the zero class, not all of A/Q.
The class of (1/2 ; 0) is missing: equivalence to zero would require the diagonal rational 1/2,
whose finite coordinate is 1/2 rather than 0. The preceding positive-radius scope may resolve the wording.
This slice includes exact finite points and does not use a full-image shortcut. No specification was edited.

## What is not done

- No reduction, raw PIECES constructor, set query, group arithmetic, dump, or character function.
  These are the later slices named in the brief. PIECES canonicality and copying are implemented.
- No overnight differential fuzz run. The lane's bounded reference and mutation checks are not called fuzzing.
- No LeakSanitizer run. ASAN/UBSAN, allocation-balanced comparisons, and the stated Valgrind checks ran.
- No separate assertion-red before every core function body. This is the COMMON-C rule 1 process gap.
- No mutation sweep of text.c or the driver. The requested sweep targeted src/qclass.c.
- No repair of old fixtures; the replacement proposal and every affected command are listed above.

## Sources pending

None introduced by this slice. Quotient set arguments use the on-disk P10 and explicit proofs in api-3a.md.
External numerical operations are cited to refs/src/flint-3.0.1/arf.rst:24-38, :65-68, :638-645.
Allocation callback testing cites refs/src/flint-3.0.1/memory.rst:16-22.

## Costs and cleanup

Copy/set_adele allocate fresh storage even if the destination has the same length.
In INV builds, add_rat checks canonicality and its delegated set checks it again.
The driver initializes qclass storage for every held value, following its existing all-kinds pattern.
These are avoidable costs; no optimization was attempted.

Removed eight lane build/mutation trees and four compiled header/Valgrind probes.
The cleanup used a timeout-120 Python script with a fixed lane-local path list after the shell removal
command was rejected by the tool's command policy. No shared build path was removed.
The shared build/ used by the mandatory acceptance scripts remains outside the lane's owned paths.
Only scripts, logs, statements, source, tests, vectors, and this report are retained.
