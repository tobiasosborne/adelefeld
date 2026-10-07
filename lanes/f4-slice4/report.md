# Lane f4-slice4 report

Slice 4b is implemented end to end. The six operations, driver calls, Julia calls and statements are complete.
All final C configurations, driver acceptance and Julia checks pass. LeakSanitizer cannot run under ptrace.
Mutation testing found four test gaps; all four now have failing scratch regressions. Three survivors remain
as redundant size checks or an equal-ball ordering change. One bounded mutation batch has no final counts.

No git command or bd command was run. SPEC, the design oracle, existing goldens and tests/test_ffun.c are unchanged.
Source edits stay in the owned paths. Eight lane build/scratch directories were removed before this report.
The requested driver/export checks leave their normal root build artifacts for the orchestrator.

## Work per step

A. gen_vectors.py imports the unchanged proto/functions4_checks.py indexing and certified transform oracle.
The final six JSONL files contain 310 records and 394497 bytes, below the 400000-byte bound:

| File | Records | Bytes |
|---|---:|---:|
| inputs.jsonl | 16 | 6477 |
| unary.jsonl | 218 | 223282 |
| products.jsonl | 14 | 85844 |
| ideles.jsonl | 53 | 48684 |
| covariance.jsonl | 4 | 28783 |
| caps.jsonl | 5 | 1427 |

All seven requested layouts occur with exact values and radii. Input arrays are shared through record indices.
All 16 input records are referenced; there are zero unused inputs. Products include both refined arrays.
Unary records include the requested rational shifts and dilations, negative signs, and delta_1 at (2,3).
A 2000-bit translation is computed; 2000-bit numerator/denominator dilations are refused by the array cap.
Idele records cover contents 1, 2, 1/3, 6/5, exact units, precise cosets, ambiguity and five explicit delta cases.
Covariance stores both oracle-transform arrays for exact deltas at (2,3) and (4,1).
Their 96-bit certificates are compared by overlap with the tighter C results; C radii must also be below 2^-90.

B. tests/test_ffun_algebra.c reads every operation record, compares every copied cell exactly and checks products
against exact rational corner products of the actual input rectangles. It checks all whole-object aliases,
including nonzero radii, commutativity, round trips up to refinement, zero-dilation failure snapshots,
reflections, conjugates, covariance, precise and ambiguous units, and the ignored real idele component.
Independent CRT lifts check every unit residue for N=1..16 and old length L=1..16: 1280 coset cases.
Caps include the integer-bit boundary, the separate and combined work caps, and lcm overflow projection.
The projection probe uses initialized objects with NULL storage and an exact product 2^64+1: LIMIT must precede INV.
Nineteen INV child cases cover every new input/output position and malformed rational/idele components.
Their diagnostics must name the public operation; a later swap abort is not accepted as an entry check.
Guard-page allocations detect before-array reads inside the uninstrumented FLINT shared library.

C. All six functions are appended to src/ffun.c; declarations/comments are appended to ffun.h.
The existing shape/lcm helpers and commit pattern are reused. Translation calls the existing F1 refinement
operation when its denominator requires it. Every result is private until its last check and one final swap.
Product orders complete balls deterministically, so commutativity is representation identity despite directed
radius rounding. It copies one coincident source pointer to avoid FLINT's alias-dependent squaring shortcut.
Rational dilation uses (|num(q)|D,den(q)M), exact holes and exact sign handling.
Idele dilation enumerates the full singleton criterion, including L=6,N=3. It uses content r and ignores inf.

D. The six driver commands follow the existing with grammar and also support direct quoted shell operands.
The 14 expected fixture lines were derived before the handlers were added, including a D!=M delta dilated by 2/3
and an ambiguous idele refusal. Julia calls all six symbols, including the design's rational-dilation ccall,
using exported size/alignment queries. The new test is registered in tests/test_julia.sh.

E. The appended Slice 4b section of docs/api-4a.md gives steps, preservation/enclosure arguments, statuses,
caps, costs and Check lines. Choices where the design is silent are recorded there.
A refined translation uses two charged passes, so it refuses refined lengths above 2^19.
A pure permutation uses one pass. Non-exact idele units charge old length plus new length.
Exact units skip enumeration.

F. All 14 named algebra faults compile and fail in plain builds. The final SAN+INV run adds four regressions
for mutation-discovered test gaps: 18 compiled, 18 failed. Every required named fault is caught.
Bounded random mutation runs and remaining survivors are recorded below. Scratch source copies are removed.

## Files written

- src/ffun.c: appended implementation and private helpers only.
- include/adelefeld/ffun.h: appended declarations, comments and idele include.
- tests/test_ffun_algebra.c.
- tests/ref/vectors/f4-slice4/: inputs, unary, products, ideles, covariance and caps JSONL files.
- tools/adf/adf.c: the six commands and their shared parsing/dispatch paths.
- tools/adf/README.md: the six command descriptions.
- tests/driver/ffun-algebra.cmd and tests/driver/ffun-algebra.out.
- tests/julia/ffun_algebra.jl and its registration lines in tests/test_julia.sh.
- docs/api-4a.md: appended Slice 4b.
- lanes/f4-slice4/: generator, fault harness, direct-call harness, mutation wrapper, redgreen.md,
  fault-results*.json, mutation-summary.json, selected-mutant lists, bounded check logs and this report.

## Checks and commands

Every test program and test script was bounded by timeout. Builds used at most two make jobs.
Mutation workers were limited to one, each with two make jobs. No check-all or package installation was run.
Intermediate red/green commands, assertion failures and counts are in redgreen.md.

Oracle and vectors:

    timeout 120 python3 -B proto/functions4_checks.py
    timeout 120 python3 -B lanes/f4-slice4/gen_vectors.py

Oracle exit 0: 2670 checks; finite_algebra 1044, dilation_covariance 440, idele_indices 325, faults_41 6.
Final generator exit 0: 310 records, 394497 bytes. Earlier generator attempts had two Python type errors and
size refusals at 2465618 and 510000 bytes. No oracle or golden was changed to resolve them.

For each row below, the build command was:

    timeout 180 make -s -j2 BUILD=lanes/f4-slice4/MODE FLAGS \
      lanes/f4-slice4/MODE/test_ffun lanes/f4-slice4/MODE/test_ffun_algebra

The test commands were:

    timeout 120 lanes/f4-slice4/MODE/test_ffun
    timeout 120 lanes/f4-slice4/MODE/test_ffun_algebra

SAN rows prepend ASAN_OPTIONS=detect_leaks=0 to both test commands. Every build and test exits 0.
The algebra records/cell counts are 294 and 95393 in every row.

| MODE | FLAGS | test_ffun checks | algebra checks |
|---|---|---:|---:|
| build | default compiler | 57465 | 626743 |
| san | SAN=1 | 57465 | 626743 |
| inv | INV=1 | 57516 | 626857 |
| clang | CC=clang | 57465 | 626743 |
| combined | CC=clang SAN=1 INV=1 | 57516 | 626857 |

INV test_ffun has 17 expected SIGABRT children. INV algebra has 19, with entry-diagnostic checks.
Address/undefined sanitizers report zero errors in the passing configurations.

    ASAN_OPTIONS=detect_leaks=1 timeout 120 lanes/f4-slice4/san/test_ffun
    ASAN_OPTIONS=detect_leaks=1 timeout 120 lanes/f4-slice4/san/test_ffun_algebra

Both final attempts exit 1 with LeakSanitizer's fatal ptrace incompatibility. No leak count is claimed.
An earlier algebra leak-detection attempt had the same exit and diagnostic.

    timeout 180 make -s -j2 -C tools/adf
    timeout 30 build/adf < tests/driver/ffun-algebra.cmd
    timeout 180 sh tests/test_driver.sh
    timeout 30 python3 -B lanes/f4-slice4/direct_calls.py
    timeout 180 sh tests/test_exports.sh

Before handlers, the fixture run exited 1 with 14 PARSE lines. After handlers it exits 1 as prescribed,
with all 14 hand-derived lines equal. Final driver acceptance exits 0: 87 cases, 101519 expected lines.
Direct-call harness exits 0: 12/12 comparisons, covering all six commands, optional with and ambiguity.
Exports exits 0: 586/586 declared functions, zero missing, zero undeclared, zero variadic functions.

    OPENBLAS_NUM_THREADS=1 JULIA_NUM_THREADS=1 LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
      timeout 60 julia --startup-file=no tests/julia/ffun_algebra.jl build/libadelefeld.so
    timeout 10 sh -n tests/test_julia.sh

Both exit 0. Julia passes 38/38 tests. The preload uses the repository's existing Julia/GMP workaround.
The complete test_julia.sh suite was not run; the registered new file and shell syntax were checked directly.

    timeout 180 python3 -B lanes/f4-slice4/plant_faults.py
    ASAN_OPTIONS=detect_leaks=0 timeout 180 python3 -B lanes/f4-slice4/plant_faults.py --san

Every scratch compile and test process is separately bounded by timeout 30. Plain final recorded run:
14 compiled, 14 SIGABRT failures. Final SAN+INV run: 18 compiled, 17 SIGABRT failures and one sanitizer exit 1.
Both harness runs exit 0. Fault diagnostics are retained in fault-results.json and fault-results-san.json.
An intermediate guard-test edit omitted stdint.h: 16 scratch compile failures were not counted as killed.
The old binary's unknown guard-group run printed 438 checks; it was not a valid guard run.
After correction the actual guard group passed 461 checks, and the before-array scratch read failed.

## Fault table

| Fault | Distinguishing group | Plain | SAN+INV |
|---|---|---|---|
| max for common D | mul | SIGABRT | SIGABRT |
| max for common M | mul | SIGABRT | SIGABRT |
| translation k+b | translate | SIGABRT | SIGABRT |
| fill F1 refinement holes | all | SIGABRT | SIGABRT |
| reflect wrong index | reflect | SIGABRT | SIGABRT |
| omit negative dilation reflection | dilate | SIGABRT | SIGABRT |
| fill dilation holes with neighbour | dilate | SIGABRT | SIGABRT |
| accept ambiguous unit image | idele | SIGABRT | SIGABRT |
| ignore unit residue | idele | SIGABRT | SIGABRT |
| omit conjugation | conj | SIGABRT | SIGABRT |
| write output before certificate | idele | SIGABRT | SIGABRT |
| drop product radii | mul | SIGABRT | SIGABRT |
| invert rational dilation | dilate | SIGABRT | SIGABRT |
| invert content | idele | SIGABRT | SIGABRT |
| remove idele output entry check | inv | not run | SIGABRT |
| Fourier read before array | guard | not run | sanitizer exit 1 |
| identity skips cell zero | mul | not run | SIGABRT |
| overflowing lcm projection accepted | caps | not run | SIGABRT |

## Mutation testing

The tool operated on src/ffun.c only. The wrapper overrides TEST_BIN to test_ffun and test_ffun_algebra:

    timeout 120 make -s -j2 check INV=1 CC=clang \
      TEST_BIN='build/test_ffun build/test_ffun_algebra'

SAN=1 is supplied by --san. The wrapper sets detect_leaks=0 and UBSAN_OPTIONS=halt_on_error=1.
A staged root avoids recursively copying the lane's scratch/build directories. Later runs also copy a matching
clang SAN+INV build cache; rewritten dependency targets name build/. Every baseline still executes both tests.
The source and tests are ordinary copies, and every mutant rewrites src/ffun.c before incremental compilation.
The mutation tool and its equivalent file were not changed.

    ASAN_OPTIONS=detect_leaks=0 timeout 180 python3 -u -B tools/mutate/mutate.py \
      --root lanes/f4-slice4/mutation-root --scratch lanes/f4-slice4/mutation-scratch \
      --files src/ffun.c --limit COUNT --seed SEED --jobs 1 --timeout 150 --san \
      --make 'timeout 150 sh lanes/f4-slice4/mutation_check.sh' \
      --copy Makefile include src tests lanes

Cached runs add build to --copy. Selected lists used the same root/file/limit/seed with --list under timeout 10.

| Seed/run | COUNT | Seconds | Killed | Survived | Not compiled | Timed out |
|---|---:|---:|---:|---:|---:|---:|
| 20261008 initial | 6 | 155.6 | 4 | 2 | 0 | 0 |
| 20261008 cached | 24 | 180 bound | no final count | 2 observed | no final count | outer timeout |
| 20261009 | 4 | 33.1 | 3 | 0 | 1 | 0 |
| 20261010 | 4 | 67.1 | 2 | 1 | 1 | 0 |
| 20261011 | 4 | 75.5 | 3 | 1 | 0 | 0 |
| 20261012 | 4 | 83.8 | 3 | 1 | 0 | 0 |
| 20261013 | 4 | 52.3 | 3 | 0 | 1 | 0 |
| 20261014 | 4 | 71.4 | 3 | 1 | 0 | 0 |

Completed runs: 30 attempts, 29 distinct choices, 21 killed, 6 survivor observations, 3 compile failures,
zero per-mutant timeouts, 538.8 seconds. No mutant was excused by the tool.
The separate cached 24-request run reached its outer timeout, killed its active command and removed scratch.
Its observed survivors are reported, but no unprinted kill/completion count is claimed.
At most 54 mutations were requested across all runs. Total completed-run time plus that outer bound is 718.8 s,
below 20 minutes. No test process was left running. Compile failures are not kills: one removed init body
has an unused parameter; the same logical-operator mutation twice fails clang's parentheses warning under Werror.

Survivors, one line per distinct observed mutation:

- ffun.c:298, remove idele y entry check: test gap closed; operation-name diagnostics kill the scratch mutant.
- ffun.c:25, SIZE_MAX division to multiplication: remains; the prior 2^20 array cap makes it redundant on 64 bits.
- ffun.c:157, Fourier t->f-k read: test gap closed; protected preceding pages make the scratch mutant fail.
- ffun.c:193, comparator >0 to >=0: remains; only equal stored balls swap, with identical arithmetic operands.
- ffun.c:67, identity starts at cell 1: test gap closed; unequal one-cell functions kill the scratch mutant.
- ffun.c:25, WORD_MAX > to >=: remains; the preceding 2^20 array cap excludes both threshold cases on 64 bits.
- ffun.c:103, lcm division to multiplication: test gap closed; the 2^64+1 projection probe kills the scratch mutant.

No test was weakened to remove a survivor. The four new distinguishing tests change no library behavior.

## Findings against the specification

No counterexample to SPEC 7 or F1-F3 was found. No specification text was changed.

## Findings against the brief, design, oracle and goldens

The brief's unrestricted f*1_Zhat=f identity is false. Let f be delta_1 with (D,M)=(2,3).
Then f(1/2)=1, while 1_Zhat(1/2)=0 since v_2(1/2)=-1. The product is zero, and refinement cannot change that.
The tests assert the counterexample and assert the identity for functions supported in Zhat.

No declaration needs a HEADER-FINDING. No new defect of the design, oracle or fixed goldens was found.
Input sharing keeps the vector files below the requested size without omitting output/refined cells.
One-line JSONL records and canonical driver output necessarily exceed the prose line limit; their fixed formats
cannot be wrapped. Authored C, Python, Julia, shell additions and prose use lines at most 116 characters.

## Sources pending and work not done

Sources pending for this finite slice: none. Read local sources:
refs/src/flint-3.0.1/arb.rst:6-12, acb.rst:6-12,463-468, memory.rst:9-24,
fmpz.rst:283-286; the inherited Fourier character convention at
refs/src/tate-poonen/notes.txt:693-700,733-740. F1-F3 and the appended stepwise arguments are the project's own
proofs.
The broader analytic source obligations listed in design section 10 remain outside this finite slice.

LeakSanitizer leak counts are unavailable because it cannot run under ptrace. Defensive nonfinite-product
cleanup
has no constructed valid-input failure witness; that branch is not claimed covered. No long differential fuzzing
or exhaustive mutation sweep was run. The timed-out 24-request mutation batch has no final counts.
The full Julia suite, integral/norm/dump, real functions and tensors are outside the performed slice checks.

Cleanup command: timeout 10 Python shutil.rmtree on build, san, inv, clang, combined, faults, mutation-root,
and mutation-scratch under this lane. Eight directories removed; zero named directories remain.
Final audit under timeout 10: ten test logs have their passing counts; 14 plain and 18 SAN+INV faults compile/fail;
nine source/statement/helper files have zero long lines and zero missing final newlines; all 16 input IDs are used.
Thirty-seven authored check logs are below 100000 bytes; the largest is 12999 bytes.
Runner-owned lane.log and stdout.log were not modified. Root driver/export build artifacts remain
for the orchestrator.
