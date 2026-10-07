# c-slice3 report

Slice c is implemented end to end. All four declarations are implemented as designed.
The default evaluates t^s times the slice b unit hull. Strict uses the finite-coset certificate.
Idele conversion supplies |x_inf|/r and sign(x_inf)u exactly once.
The specification, oracle, goldens and existing character tests were not changed.
No git command, tracker command, installation or subagent was used.

## Work per step

A. gen_vectors.py imports proto/char_checks.py. Exact phase sets are independently checked by lifting.
Point values use exp(s log(t)), with exact cardinal phases. Python-flint uses 512-bit certified arithmetic.
Endpoints are rounded outward to integer multiples of 10^-60. No adelefeld evaluator is imported.

| Vector file | Records | Bytes |
|---|---:|---:|
| params.jsonl | 25 | 747 |
| values.jsonl | 640 | 151460 |
| images.jsonl | 143 | 165159 |
| classes.jsonl | 2172 | 39409 |
| ideles.jsonl | 128 | 3978 |
| invalid.jsonl | 5 | 52 |
| Total | 3113 | 360805 |

All 108 primitive characters of conductor <=24 occur. Five exact s choices and radius-bearing s occur.
Exact units +/-1, C|N, ambiguous cosets and a nonunit printed representative are included.
Ideles have both real signs, all four requested contents and real radii.
Non-dyadic rational t is enclosed at 512 bits on input; fitting dyadic t is stored exactly.
The five invalid t rows exercise the constructor's DOMAIN status, not invalid evaluator storage.

family_hull.py proves and computes the continuous-family hull by finite stationary/cardinal candidates.
The stepwise proof is appended to api-3d. There are 40 family images and 160 certified extrema.
Each extremum interval is at most two units wide on the 10^-60 grid. All 640 point values are referenced.
The full interval boxes per phase are retained separately from attained point values.

B. test_char_class reads every vector file and tests every case at precision 2,53,128.
It checks containment of every point interval and certified hull extremum, including continuous families.
The endpoint allowance over the reference rectangle product is
256*M*2^-p + 2^-24*(W+M*2^-p), with M=max(1, endpoint magnitudes), W the coordinate width.
Against the exact value hull, add the explicitly computed interval-product overhang.
Real line cases reject a square. The numerical enclosure of every family is checked by its hull extrema.

Strict ambiguity preserves sentinel bytes. Uncertainty in t and s is accepted when the unit value is fixed.
Constructor DOMAIN, conversion NOT_DETERMINED, precision/conductor LIMIT and numerical failures preserve outputs.
The tests cover all four cap preflights, both sides of each cap, injected phase LIMIT and setup UNSUPPORTED.
Injected nonfinite real power, complex power and product test the final commit guards.
Conversion failure prevents all dependent phase/power work. Thirty-two INV children abort.
Member alias tests include either coordinate of z against either s coordinate and class t/idele inf.

All 128 idele rows agree identically with explicit class conversion followed by class evaluation.
Negating only inf changes the set by chi(-1); even and odd parity are tested.
Conjugation is tested through slice b's conj, including the uncertain s rectangle.
The design example is derived by hand: t=2, u'=-1, chi_3(2)(-1)=-1, hence value -2.
This real integer-power path gives exact -2. The general public contract promises containment.

C. char.c appends shared class/idele helpers and four wrappers. It reuses char_unit.
Real s uses arb_pow with exactly zero imaginary output. Complex s uses acb_pow on positive-real t.
Every temporary is cleared. Bounds precede INV, allocation, ambiguity and conversion.
The output is swapped only after the last numerical check. No new HEADER-FINDING was needed.

D. char_eval and char_eval_strict accept idele or class text, through the existing with grammar.
The fixture contains 21 hand-derived lines. Both direct CLI spellings return exact -2 for the example.
Julia uses exported layout queries, preserved owners and the design's adf_char_eval_idele signature.

E. api-3d has appended Slice c: function meanings, P1/P2, P15, power contract, sign, statuses,
cost, aliasing, the continuous-family extremum proof and Check lines. No new design decision was made.

F. All six required scratch faults compile and are rejected. Mutation testing selects 45 distinct candidates.
The sample, interrupted batch completion and targeted slice c checks are detailed below.

## Files written

- Appended functions in src/char.c and declarations/include in include/adelefeld/char.h.
- tests/test_char_class.c and tests/ref/vectors/c-slice3/*.jsonl.
- The two commands in tools/adf/adf.c and their tools/adf/README.md lines.
- tests/driver/char-class.cmd/.out, tests/julia/char_class.jl and its tests/test_julia.sh block.
- Appended docs/api-3d.md.
- Lane generator, family_hull.py, check/fault/mutation/replay scripts, redgreen.md,
  bounded logs, selections, JSON results and this report.

## Commands and results

Every test program and script ran under timeout. Builds used -j2 or one compiler invocation.
The exact expanded compile/link/run commands are retained in the lane logs and checks.json.
Build trees and scratch copies are removed. Root artifacts from required driver/Julia scripts remain.
check-all was not run.

1. `timeout 120 python3 -B lanes/c-slice3/gen_vectors.py`: final exit 0; 3113 records, 360805 bytes.
   Initial size-guard exit 1: 422853 bytes. Removing duplicated decimal denominators gave 277546 bytes.
   The continuous-hull extension gave 354354 bytes with reference boxes temporarily in values.jsonl.
   Separating those enclosures from attained values gives the final 360805-byte schema.

2. `timeout 120 python3 -B -` importing char_checks and calling check_cosets/check_evaluation:
   exit 0; cosets=11125, hulls=20736, evaluation=190. Its exact script is in the harness transcript;
   gen_vectors.py also asserts every generated small coset against brute_coset.

3. `timeout 180 python3 -B lanes/c-slice3/run_checks.py CONFIG`, CONFIG=plain,san,inv,clang:
   each final full configuration exits 0. Final class-only reruns use the additional argument class-only.
   They rebuild/run only the changed test and its wrapper, and all four exit 0.
   Each program runs under timeout 60; each wrapper compile under timeout 30.

| Program | Plain | SAN=1 | INV=1 | CC=clang |
|---|---:|---:|---:|---:|
| test_char | 129725 | 129725 | 129797 | 129725 |
| test_char_eval | 1738298 | 1738298 | 1738325 | 1738298 |
| test_char_class | 1208883 | 1208883 | 1208979 | 1208883 |
| test_char_class_wrap | 1208981 | 1208981 | 1209069 | 1208981 |

   Every final exit is 0. The twelve required executions and four new wrapped executions pass.
   The first INV compile exits 2 for a test pointer-type conditional; its corrected run passes.
   Intermediate test compile/assertion failures and initial counts are retained in redgreen.md/checks.json.

4. `ASAN_OPTIONS=detect_leaks=1 timeout 60 lanes/c-slice3/san/test_char_class`:
   final exit 1; LeakSanitizer reports that it cannot operate under ptrace.
   ASan/UBSan checks use detect_leaks=0. No LeakSanitizer pass is claimed.
   FLINT callbacks observe 8600 blocks per 100 warmed cycles in plain/SAN/Clang and 20600 in INV,
   with 0 live after every cycle. The 1024-bit Gauss accumulator exercises heap cleanup.
   This observes FLINT allocations, not every GMP/runtime allocation.

5. `timeout 180 sh tests/test_driver.sh`: final exit 0; 90 cases, 101567 expected lines, 0 differences.
   Both `timeout 30 build/adf char_eval C X` and `timeout 30 build/adf char_eval_strict C with X`
   exit 0 and print (-2)+(0)*i, with C and X the design's exact texts.
   `JULIA_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 timeout 180 sh tests/test_julia.sh`:
   exit 0; new Julia test 17/17, declared exports 595/595, 0 missing/undeclared/variadic symbols.
   The suite uses its existing system-GMP preload retry. Existing lpow.c/rfun.c warnings are unchanged.

6. `timeout 180 python3 -B lanes/c-slice3/plant_faults.py`: final exit 0;
   6 object compiles and 6 links exit 0; 6 assertion aborts (-6), 0 timeout.
   Each compile/link/test is individually bounded by timeout 30. Scratch copies are removed.

7. Mutation commands/results appear below. Final style/cleanup uses `timeout 20 python3 -B -`:
   exit 0; 13 full authored files plus appended char.c checked, 0 lines above 116,
   0 missing final newlines, 0 remaining lane build/fault trees, 0 logs above 100000 bytes.
   Machine JSONL and command-log records keep their required single-line form.

Resource limitation: each build obeyed the two-job cap, but no global CPU affinity cap was set.
A final LSan attempt briefly overlapped two single-job replay processes. Global two-core compliance
for that overlap is not established. No long computation was left unbounded.

## Named faults

| Fault | Compile | Link | Wrapped test exit |
|---|---:|---:|---:|
| Drop real sign after conversion | 0 | 0 | -6 |
| Use t^conj(s) | 0 | 0 | -6 |
| Use abs(inf), omitting division by r | 0 | 0 | -6 |
| Strict ignores the finite-coset certificate | 0 | 0 | -6 |
| Multiply only one corner of the unit hull | 0 | 0 | -6 |
| Write z before the final numerical check | 0 | 0 | -6 |

## Mutation results

`timeout 150 sh lanes/c-slice3/mutate_check.sh` exits 0 under SAN+INV, with leak detection disabled.
It builds/runs only wrapped test_char, test_char_eval and test_char_class, under individual timeouts.
The sampled root is a frozen /tmp copy with only the required source/test tree, lane script and warm archive.
No working source or mutation tool was changed by the sweep.

`timeout 1100 python3 -B lanes/c-slice3/mutate_batches.py` exits 0 after recording the third batch timeout.
Each batch has an independent timeout 180 and runs:

    timeout 180 python3 -u tools/mutate/mutate.py --root /tmp/adf-c-slice3-mutation-root \
      --scratch /tmp/adf-c-slice3-mutate-J --files src/char.c --limit 10 --seed SEED \
      --jobs 1 --timeout 120 --san --make 'timeout 110 sh lanes/c-slice3/mutate_check.sh' \
      --copy Makefile include src tests lanes

| Batch | Seed | Exit | Seconds | Tool killed | Survived | Not compiled |
|---|---:|---:|---:|---:|---:|---:|
| 1 | 310409 | 1 | 137.9 | 8 | 1 | 1 |
| 2 | 310410 | 1 | 137.7 | 6 | 1 | 3 |
| 3 | 310411 | 124 | 180.1 | no final summary | 1 observed | 3 observed |

The first three batches take 455.7 seconds. Every baseline passes. Batch 3 kills its active command on timeout.
`timeout 180 python3 -B lanes/c-slice3/recheck_mutations.py` then judges all ten batch-3 candidates:
6 killed, 3 not compiled, 1 survived. The script itself reaches exit 124 before the two earlier survivors.
`timeout 180 python3 -B lanes/c-slice3/recheck_mutations.py survivors` exits 0:
principal setup is killed; the radius change initially survives class multiplication.
`timeout 30 python3 -B lanes/c-slice3/recheck_mutations.py radius` exits 0 after the direct unit print repair:
the radius candidate compiles/links, then aborts (-6).

`timeout 180 python3 -B lanes/c-slice3/recheck_mutations.py slice-c` exits 0:
15 distinct tool-generated candidates in the appended code compile/link and fail.
They cover both bounds in both helpers, LIMIT status, both strict/default wrappers,
the real/complex branch, both finite guards and both member-alias checks.

Final union: 45 distinct candidates, 37 killed, 7 not compiled, 1 survivor, 0 unjudged candidates.
The three replay-script bounds, radius replay and warm baseline keep the mutation budget below 20 minutes.
The seven compile failures are uninitialized-variable, array-bounds, unused-parameter or parentheses diagnostics.
They are not counted as test kills. No equivalent.txt entry or mutation-tool repair was made.

Remaining survivor, one item:

- char.c:210, a<C changed to a<=C: adds a=C, mathematically a nonunit zero term when C>1;
  C=1 returns before the loop. All three wrapped tests pass. FLINT's unreduced-input contract is source pending,
  so this is conditional equivalence, not a universally sourced implementation proof.

## Findings against the specification

0 counterexamples found to the named SPEC statements. SPEC was not edited.

## Findings against the design, oracle and goldens

0 new declaration contradictions; no new HEADER-FINDING. No oracle or golden file was changed.
The brief's raw positive-ball status belongs to adf_idclass_set_parts: DOMAIN.
Evaluation requires canonical positive class storage; INV aborts on a violation. This follows design section 3.

The prescribed rectangle multiplication generally exceeds the exact rotated finite-value hull.
For fourth roots times R exp(i theta), width excess is 2R min(abs(cos(theta)),abs(sin(theta))).
At t=2,s=2+3i this is nonzero independently of precision. A rounding-only tightness bound would be false.
The tests retain containment and explicitly include that interval-product overhang in their stated bound.

The intermediate whole-reference-box assertion was stronger than enclosure of the true family.
At the singleton-i family with t=2+/-1/4 and s=(1/2+/-1/16)+(14+/-1/32)i,
the certified imaginary minimum is about -1.4627698786; the reference interval box extends to -1.5779842754.
That extra endpoint is not a represented value. Actual witnesses and certified extrema replace that assertion.
The full reference enclosures remain in the vectors. No mathematical acceptance statement was weakened.

The inherited predicate/conjugation source-dependent findings remain as documented in slices a/b.

## Sources pending and not done

- [source pending: FLINT 3.0.1 universal Conrey pairing/exponent implementation under refs/].
- [source pending: full-word dirichlet_group_init failure semantics, inherited from slice a].
- [source pending: signed primitive quadratic Gauss evaluation, inherited and unused].
- [source pending: dirichlet_chi's unreduced a=C input contract, relevant only to the surviving mutant].

Read power/enclosure sources: refs/src/flint-3.0.1/arb.rst:6-12,1034-1040,1187-1192;
acb.rst:6-18,382-389,463-468,637-643,675-679; memory.rst:9-44.
The class/sign argument is ideles P15:368-394 and conventions 5.7/5.13, read before coding.
The continuous-family extremum proof is our own stepwise argument in api-3d.

No long differential fuzzing, LeakSanitizer pass or universal FLINT pairing proof is claimed.
The driver and Julia fixtures were written before their first run, but no separate driver/Julia red is claimed.
The bounded sample uses 45 candidates rather than exhausting the 60-candidate allowance.
Avoidable costs remain: separate base/power/result temporaries, repeated INV setup and conversion allocations.
