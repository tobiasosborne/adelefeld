# Lane f4-slice7 report

Implemented all eight dump/load/inspect functions, their declarations, C tests, driver calls, Julia calls,
and appended statements. The shared validator and existing loaders were not changed. SPEC and goldens were
not edited. Build trees, binaries and mutation scratch were removed. Five explained mutation survivors remain.
LeakSanitizer cannot run under this harness; ASan/UBSan pass with leak detection disabled.

## Work done

A. `gen_vectors.py` uses the exact parser/printer in proto/text_grammar.py, without importing C.
There are 40 generated ffun values and 40 generated rfun values, plus 13 status/binding records per type.
The six finite shapes are (1,1), (2,3), (3,2), (6,6), (12,5), (1,1024). Real functions have 0 to 4 terms,
degrees 0 to 6, retained P=[], complex parameters and parameter radii. Inputs include exact values,
2000-bit mantissas, zero radii, small/large radii, and radius mantissas at the 30-bit boundary.
Two real-function vectors now distinguish pure imaginary last coefficients, exact and inexact.
Final sizes: ffun.jsonl 302431 bytes; rfun.jsonl 34534 bytes; total 336965 bytes, below 400000.
Statuses: ffun 40 OK, 8 PARSE, 3 DOMAIN, 2 UNSUPPORTED; rfun 40 OK, 7 PARSE, 4 DOMAIN, 2 UNSUPPORTED.
Incomplete above-cap counts give PARSE under the fixed stage order. Complete oversized bodies are generated
in C at runtime, rather than stored in the bounded vector corpus.

B. Both C programs consume all 53 records and all relevant goldens: four ffun rows, six rfun rows.
Each constructs 2000 random C values and checks representation identity and exact dump bytes after loading.
Tests cover bounded non-NUL spans, every truncation of a representative valid dump, byte/grammar/limit/domain
precedence, failure sentinels, untouched descriptors, nctx=0, ignored convenience ctx, bindings 0/1/SIZE_MAX,
noncanonical balls in every ball position, Re(A)<=0, retained zero terms and arbitrary binary exponents.
They interpose arb_load_str and count FLINT allocations. INV tests require three SIGABRT children per type.
The finite count boundaries are 1048575, 1048576, 1048577. Real term/one-polynomial boundaries are
65535, 65536, 65537. Total coefficients also have those boundaries split across two terms.
Rejected oversized bodies trap value allocations at or above 1048576 bytes before their count preflight.

C. Eight public functions and three private helpers were appended to src/dump.c.
They reuse dp_validate unchanged, finish its DP_NOSEM result with D1 caps and type predicates, then build
exact dyadics in temporary owned storage. Polynomial lengths are assigned directly. No normalisation,
sorting or FLINT string load occurs. Binding counts are checked after full validation. Commit is by swap.
The previously existing dump.c prefix was compared with the scratch snapshot: identical, result 1.
M1-D9 applies only to qclass pieces. It does not bound these bodies' binary exponents.

D. Driver dump/load/inspect dispatch and the explicit body/type table cover both types.
The rfun value slot is ADF_DRV_RFUN_VALUE, since ADF_DRV_RFUN already names an operation enum.
Its storage, initialization, clearing and printing branches were added for dump/load.
The fixtures' expected lines were written by hand before running: six ffun lines and seven rfun lines.
Julia uses layout queries, exercises both types, and frees every dump string. Its file is registered in
tests/test_julia.sh. The full driver script ran in a lane-local source copy to keep its build files owned.

E. Both API documents have an appended "Slice 4c, the dump forms" section: grammar, stages, statuses,
caps, exact representation argument, cost, checks and the NULL-nctx decision. Two HEADER-FINDINGs are below.

F. All nine named scratch faults compiled and were caught by test assertions. The tool sweep selected
60 distinct mutants from 198 candidates, restricted to the new functions/helpers and these two tests.
Three surviving test gaps were closed and those exact mutants retested. Final accounting is 46 killed,
five explained survivors, nine not compiled, zero timed out, zero excused. No extra candidate was selected.

## Files written

- src/dump.c: only the new dump implementation, starting at line 2786.
- include/adelefeld/dump.h: new type includes, comment block and eight declarations.
- tests/test_ffun_dump.c, tests/test_rfun_dump.c; shared test code in lanes/f4-slice7/dump_test.h.
- tests/ref/vectors/f4-slice7/ffun.jsonl and rfun.jsonl.
- tools/adf/adf.c: ffun/rfun dispatch and rfun dump/load storage; tools/adf/README.md: their dump/load lines.
- tests/driver/ffun-dump.cmd, ffun-dump.out, rfun-dump.cmd, rfun-dump.out.
- tests/julia/ffun_dump.jl and its registration in tests/test_julia.sh.
- Appended sections in docs/api-4a.md and docs/api-4b.md.
- Lane generators, test helpers, check/fault/mutation/differential/cleanup/style scripts, bounded logs,
  redgreen.md, JSON fault/retest results, mutation-map.txt, layout_probe.c and this report.

## Commands and results

All test/script commands used timeout. Builds used at most two jobs per make invocation.
No state-changing git command, bd, installation or check-all command was run.

1. `timeout 180 python3 -B proto/test_text_grammar.py`: run twice, 35 tests each, zero failures, exits 0.
   The final run took 16.518 seconds. `timeout 60 python3 -B lanes/f4-slice7/gen_vectors.py` ran three times:
   106 rows each; totals 336991, 336976, 336965 bytes after the distinguishing-input additions; exits 0.

2. Red build, then green build:

       timeout 180 make -s -j2 BUILD=lanes/f4-slice7/plain \
         lanes/f4-slice7/plain/test_ffun_dump lanes/f4-slice7/plain/test_rfun_dump

   Initially both translation units failed without declarations. With declarations, both failed to link
   against the absent symbols. After implementation, build exit 0. The first successful direct runs were:
   `timeout 180 lanes/f4-slice7/plain/test_ffun_dump`: 17062 checks, zero failures, exit 0;
   `timeout 180 lanes/f4-slice7/plain/test_rfun_dump`: 17222 checks, zero failures, exit 0.
   One earlier rfun run failed on a handwritten exponent fixture missing a token; the fixture was corrected.

3. Final requested matrix commands:

       timeout 180 python3 -B lanes/f4-slice7/checks.py plain
       timeout 180 env ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 \
         python3 -B lanes/f4-slice7/checks.py san
       timeout 180 python3 -B lanes/f4-slice7/checks.py inv
       timeout 180 python3 -B lanes/f4-slice7/checks.py clang

   Each builds the ten explicit targets with timeout 180 make -s -j2, then runs each executable under
   timeout 180. Flags are none, SAN=1 CC=clang, INV=1, and CC=clang respectively.
   All four final builds exited 0. All 40 final program runs exited 0; zero failed checks/tests.
   The per-program check counts are:

   | Program | plain | SAN=1, clang | INV=1 | clang |
   |---|---:|---:|---:|---:|
   | test_ffun_dump | 50316 | 49116 | 50322 | 49116 |
   | test_rfun_dump | 59370 | 59476 | 59376 | 59476 |
   | test_dump | 12259 | 12259 | 12259 | 12259 |
   | test_dump_ctx | 28587 | 28587 | 28587 | 28587 |
   | test_dump_golden | 763 | 763 | 763 | 763 |
   | test_dump_limits | 297 | 297 | 297 | 297 |
   | test_dump_local | 36205 | 36205 | 36205 | 36205 |
   | test_dump_units | 32505051 | 32505051 | 32505051 | 32505051 |
   | test_qclass_dump | 33913 | 33913 | 33915 | 33913 |
   | test_char_eval | 1738298 | 1738298 | 1738325 | 1738298 |

   New-test FLINT blocks allocated/live: plain and INV, ffun 33092/0, rfun 41680/0;
   SAN and clang, ffun 31892/0, rfun 41786/0. INV confirms six expected new-test aborts in total.
   Intermediate whole-suite runs also used these commands: plain once, INV twice, SAN twice, clang once.
   Initial INV: one failed program from a 12-byte fixture for a 13-byte dump; corrected, then zero failures.
   Initial SAN with detect_leaks=1: ten failed programs, nine ptrace LeakSanitizer failures and one test UB
   from zero-length memcpy with NULL. The test now skips that copy. Subsequent SAN with detect_leaks=0:
   ten programs, zero failures. The other intermediate whole-suite runs had zero failed programs.
   LeakSanitizer's message says it does not work under ptrace. Its diagnostics are in lsan-attempt.log.

   Additional `checks.py MODE --own` runs used timeout 180, TASK_JOBS=1 and detect_leaks=0:
   plain twice, INV twice, SAN once, clang once; two programs per run, all exits 0.
   Their superseding counts before the final split-coefficient cases were plain 50316/59312,
   INV 50322/59318, SAN and clang 49116/59418. Logs and summaries are retained.

4. Driver red/green executable builds used timeout 60 cc, -std=c11 -O2, -Iinclude, the lane's plain archive,
   tools/adf/adf.c, -lflint -lgmp -lm, and outputs adf-red/adf in the lane. Both builds exited 0.
   Each fixture ran under timeout 20. Red: 13 differing lines, all UNSUPPORTED. Green: 13 equal lines;
   both diff -u commands exited 0. Executables exited 1 as the error-containing fixtures require.
   `timeout 10 python3 -B lanes/f4-slice7/prepare_driver.py`: exit 0.
   In lanes/f4-slice7/driverroot:

       timeout 180 env MAKEFLAGS=-j1 sh tests/test_driver.sh

   Exit 0: 93 cases, 101594 expected lines, zero differences. The 100000-line case took 0.067 seconds.

5. Shared library and Julia:

       timeout 180 cc -std=c11 -O1 -g -fPIC -shared -Iinclude src/*.c \
         -lflint -lgmp -lm -o lanes/f4-slice7/libadelefeld.so
       timeout 60 env JULIA_NUM_THREADS=1 JULIA_NUM_GC_THREADS=1 julia --startup-file=no \
         tests/julia/ffun_dump.jl lanes/f4-slice7/libadelefeld.so
       timeout 60 env LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
         JULIA_NUM_THREADS=1 JULIA_NUM_GC_THREADS=1 julia --startup-file=no \
         tests/julia/ffun_dump.jl lanes/f4-slice7/libadelefeld.so

   Shared build exit 0, two existing GCC rf_rat_ok stringop-overread warnings in src/rfun.c:339,424.
   First Julia exit 1 on __gmpn_modexact_1_odd, matching the documented bundled-GMP issue.
   The system path was checked with ldconfig. Preload retry exit 0, 18/18 checks passed.
   `timeout 10 nm -D lanes/f4-slice7/libadelefeld.so` filtered for the eight names: eight exported T symbols.

6. `timeout 180 python3 -B lanes/f4-slice7/differential.py`: exit 0, 15000 texts, 11250 edited,
   4556 accepted, zero status/count/preservation/dump-byte mismatches. This is a differential smoke run.

7. Fault and mutation commands/results are in the next two sections.
   `timeout 30 cc -std=c11 -Iinclude lanes/f4-slice7/layout_probe.c -o lanes/f4-slice7/layout_probe`:
   exit 0. `timeout 10 lanes/f4-slice7/layout_probe`: exit 0; acb 96 bytes, rterm 312, ulong 8, slong 8.
   A bounded Python comparison of the existing dump.c prefix with its snapshot returned identical=1.

8. `timeout 30 python3 -B lanes/f4-slice7/cleanup.py`: exit 0, 13 paths removed; zero remaining lane build
   directories and zero mutation scratch directories. It also removed the oversized harness stdout.log.
   `timeout 10 python3 -B lanes/f4-slice7/style_check.py`: exit 0; 21 prose/code files, zero line/newline
   failures, 76 long machine JSONL records, vector bytes 336965, zero retained logs above 100000 bytes.

## Named fault table

`timeout 180 python3 -B lanes/f4-slice7/faults.py`: final exit 0, nine builds succeeded, nine tests exited 1.
Each scratch build uses timeout 90 make -s -j2 SAN=1 INV=1 test_KIND_dump; each executable uses timeout 90.
Seven faults use --quick, retaining vectors, goldens, random values, malformed texts and INV checks;
the late-count and cap-boundary faults use the complete tests with caps.

| Fault | Distinguishing failure | Test exit |
|---|---|---:|
| Reorder terms | Re-dumped bytes differ | 1 |
| Trim exact trailing zero | Expected DOMAIN, got OK | 1 |
| FLINT load before byte validation | Interposed arb_load_str assertion | 1 |
| Count cap after values | Allocation >=1048576 before rejection | 1 |
| D M count mismatch ignored | Expected PARSE, got OK | 1 |
| Re(A) predicate omitted | Expected DOMAIN, got OK | 1 |
| Cap reduced by one | Exact-cap dump expected OK, got LIMIT | 1 |
| x written before validation finishes | Output bytes/identity sentinel changed | 1 |
| nctx write omitted | Expected zero count, old count 773 remains | 1 |

An initial invocation had nine link failures and zero executed tests because its base archive lacked INV
symbols. After rebuilding the base, a preliminary successful fault run caught all nine: eight exits 1,
one SIGABRT from changing x's dimension. The final early-write fault changes a coefficient instead,
so the sentinel assertion supplies the witness. The final cap fault uses cap-1, rather than a quotient tie.
These preliminary failures are not counted as executed final kills.

## Scoped mutation sweep

`timeout 10 python3 -B lanes/f4-slice7/prepare_mutation.py`: exit 0, run twice to correct the base archive.
The existing source prefix is included verbatim in dump_unchanged.h. Only new implementation lines are
candidates. The unchanged objects come from the full SAN+INV clang archive. The only tests are the two
complete new dump programs. The mutation tool and equivalent.txt were not modified.

The archive was built by timeout 180 make -s -j2 BUILD=lanes/f4-slice7/combined SAN=1 INV=1 CC=clang all:
exit 0. `timeout 180 env ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 make -s -j2 \
-C lanes/f4-slice7/mutroot check SAN=1 INV=1`: initial link exit 2; corrected baseline exit 0, two tests passed.

    timeout 900 env ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
      python3 -B -u tools/mutate/mutate.py --root lanes/f4-slice7/mutroot \
      --scratch /tmp/adf-f4-slice7-mutate --files src/dump.c --limit 60 --seed 410807 \
      --jobs 1 --timeout 60 --san --make 'make -s -j1 check INV=1' \
      --copy Makefile include src tests lanes --keep

Tool exit 1. Baseline 17.6 seconds. Of 198 candidates, 60 ran in 853.1 seconds:
43 killed, eight survived, nine not compiled, zero timed out, zero excused.
The nine compile failures were six pointer-star syntax/type mutations, two mixed-logic -Werror diagnostics,
and one removed assignment leaving count uninitialized. They were not executed or counted as kills.

`timeout 180 python3 -B lanes/f4-slice7/retest_survivors.py`: exit 0. It retested three existing selected
mutants using timeout 120 make -s -j2 check SAN=1 INV=1 after copying expanded inputs/tests:

- Scratch 70:31, imaginary midpoint test omitted: test exit 1, make exit 2; valid pure imaginary P rejected.
- Scratch 50:27, skip comparison < to <=: test exit 1, make exit 2; oversized total accepted as OK.
- Scratch 50:31, skip product * to /: test exit 1, make exit 2; preflight allocation guard failed.

Final distinct-mutant counts: 46 killed, five surviving, nine not compiled. Three repeats, no new candidate.
Retained survivors, one line each (scratch line 2 maps to repository line 2786):

- 28:56, M initializer 0 to 1: positive dimensions overwrite M; zero-factor cases still have D=0 or fail domain.
- 28:49, D initializer 0 to 1: positive dimensions overwrite D; zero-factor cases still have M=0 or fail domain.
- 57:91, capacity-guard LIMIT to OK: count<=1048576 from D1, so the 64-bit word/byte guard cannot be reached.
- 13:17, memset byte 0 to 1: st.mode is overwritten; other fields are unused on already validated syntax.
- 13:5, memset removed: st.mode is assigned; the remaining fields are unused on already validated syntax.

The initializer reasoning is: (1) both positive factors enter dp_word, overwriting both words; (2) with a
zero factor, the other zero initializer still makes the product zero; (3) the positivity check then rejects.
The capacity reasoning is: (1) D1 gives count<=2^20; (2) measured sizeof(acb_struct)=96, hence at most
100663296 bytes; (3) both this and the count fit the declared 64-bit size_t/slong platform.
The state reasoning is: (1) st.mode=DP_SYNTAX is assigned; (2) dp_w_arb uses other state only on a failed
syntax read or a different mode; (3) these spans already passed complete grammar validation.
These are explanations for retained survivors, not equivalent.txt excuses or additional killed mutants.

## Findings against the specification

No counterexample to SPEC 7 or N-D23 was found. No specification or fixed grammar was changed.

## Findings against the design, reference and goldens

1. HEADER-FINDING: api-4.md section 2's unconditional dump/load identity promise needs limits and D1.
   Step 1: a canonical ffun can have D=1, M=1048577 and that many live zero entries; its predicate has no D1 cap.
   Step 2: its canonical dump has M token 100001 and exactly 1048577 acb groups.
   Step 3: N-D23 requires LIMIT on loading that complete dump even with expanded caller limits.
   Thus unconditional identity cannot hold. The same example uses 65537 retained zero-P rfun terms with A=1.
   Identity is implemented when caller limits and D1 admit the dumped representation.

2. HEADER-FINDING: load's cost cannot depend only on input text size when replacing owned old storage.
   Step 1: choose an old canonical array with L live entries, or an old real function with L owned fields.
   Step 2: load a fixed one-entry/zero-term dump. Step 3: successful replacement must clear the L old entries.
   Hence old representation size contributes to cost, including under INV. The new comments state this.

3. Reference gap: proto/text_grammar.py:1303-1314 checks caller max_items for these bodies, without N-D23's
   fixed count caps or its global coefficient sum. Corpus values stay below those caps. C boundary tests
   derive the fixed LIMIT cases directly from N-D23. The reference was not edited.

4. Goldens: four ffun and six rfun rows exist; all are consumed and pass. No missing-row gap was found.
   The two pure imaginary polynomial witnesses were added only to this lane's reference-generated corpus.

5. JSONL records necessarily exceed 116 columns for these dense dumps; splitting a record would violate
   the strict one-object-per-line vector reader. Prose/code and driver/Julia fixtures respect 116 columns.
   Machine check logs retain exact commands and can also have longer lines.

Decision where the design is silent: NULL nctx gives DOMAIN, matching the existing dp_inspect courtesy.
Avoidable cost: repeated full traversals for grammar stages, caps, predicates and construction; driver load
also inspects before loading. No optimization or change to the shared validator was attempted.

## Sources and sources pending

Read local ground truth: conventions 10.1/10.2 and 5.11/5.12; api-4.md sections 1,2,8,9,10; SPEC 7/15.4;
PLAN milestone 4; dump.h/dump.c and the repaired dump lane reports/review; existing qclass/character dump tests.
External library facts used: refs/src/flint-3.0.1/arf.rst:227-242 (exact dyadics), arb.rst:286-298
(serialization), acb_poly.rst:47-54 (direct length and normalization), memory.rst:9-24 (allocator hooks).
Sources pending: none for this slice. No analytic theorem or external Fourier convention is used here.

## What is not done and process limits

LeakSanitizer validation is unavailable under ptrace. FLINT counters check retained FLINT blocks and do not
replace a general leak detector. The 15000-input differential is not a long fuzz campaign.
No full check-all, full Julia suite, whole-source mutation sweep or benchmark was run.
Separate per-symbol assertion-red cycles were not staged after the two initial file-level link failures;
the tests for all eight absent functions were written before their implementation.
I started the combined -j2 build before collecting the failed fault-run completion. The aggregate two-core
bound was not established for that short interval. Later fault runs were serialized; mutation/driver
concurrency used one compiler job each. No background agent was spawned.
