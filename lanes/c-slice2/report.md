# c-slice2 report

Slice b is implemented: seven public functions, driver calls, strict character dumps and Julia.
All 36 required final test executions pass. P3's universal FLINT identification remains source pending.
No SPEC, golden, oracle, test_char, src/text.c, git state or tracker was changed. No subagent was used.

## Work per step

A. gen_vectors.py imports proto/char_checks.py and generates five JSONL files, 144868 bytes in total.

| File | Records | Bytes |
|---|---:|---:|
| conj.jsonl | 1206 primitive characters through C=80 | 34453 |
| operands.jsonl | 1 shared operand array | 2484 |
| images.jsonl | 23 shared phase sets and certified hulls | 14515 |
| cosets.jsonl | 4973 cases | 91013 |
| dump.jsonl | 39 texts | 2403 |

There are 4972 cosets for all 108 primitive pairs C<=24, with every requested N.
Operands include negative and 2001-bit integers. Nonunits modulo C occur when gcd(c,N)=1 permits them.
The additional (27,2,2,9) case has phases {1/18,7/18,13/18}; it closes a mutation coverage gap.
Hull coordinates are exact rational-angle cosines/sines, hence algebraic root-of-unity coordinates.
Their intervals are certified by python-flint at 512 bits and rounded outward onto the 60-decimal grid.
Thirty dump inputs have exact or radius-bearing s. The remaining nine exercise malformed inputs and bounds.
There are eight char rows in tests/golden/dump.tsv. All eight are tested; no golden gap was found.

B. test_char_eval reads every vector. It tests every phase and all four hull extrema at p=2,53,128.
The endpoint allowance is 4*2^-p + 2^-28*(W/2+2*2^-p), with W the true coordinate width.
Line hulls reject a universal square. Strict snapshots preserve z byte-for-byte on ambiguity and errors.
Exact [1]/[-1], C|N, huge singleton/ambiguous N, precision/modulus caps, aliasing and INV are covered.
Unit evaluation ignores s because its input has no t coordinate.

All 1206 primitive conjugate labels match the oracle. Involution, self-aliasing and conjugated s are tested.
All 1966 characters through modulus 80 give 74434 exact unit phase products equal to 1 after conjugation.
A manually verified primitive (65537,3) tests uncapped conjugation and inverse label 21846.
Dump round trips are byte-identical. Inspect writes count 0 and preserves descriptors.
Failure tests preserve both object bytes and an independent copy of s. arb_load_str is interposed and never called.
Wrapped tests inject setup failure, phase failure, borderline/large cosine width and a nonfinite root denominator.
Nine INV children abort as required, including both conjugation arguments and the numerical member aliases.

C. Unit evaluation reuses char_phase and char_round. It scans units a mod C agreeing with c mod gcd(C,N).
It minimizes the four exact circle distances and certifies the cosine bounds before one output swap.
Strict ambiguity precedes setup and trig in a normal build; INV may set up the character predicate.
Conjugation negates component exponents and rebuilds the label with _dirichlet_char_exp.
Dump functions share the existing validator and exact dyadic builder.
They reject imprimitive inputs and never lower.
Label 1 at q>1 is cheaply imprimitive, so DOMAIN precedes D1 without group setup.
The loader cap test uses verified primitive (65537,3), which still gives LIMIT before setup.

D. char_unit, char_unit_strict and char_conj work as scripts and direct CLI calls.
Generic dump/load uses the typed functions. Generic character arithmetic remains UNSUPPORTED.
The two fixtures contain 24 hand-derived output lines. Julia uses layout queries and preserved storage owners.

E. docs/api-3d.md has an appended Slice b: P1/P2 proofs, the hull certificate, statuses, costs,
conditional P3 argument, sources, decisions and Check lines. The specification is unchanged.

F. Eleven named faults compile and are rejected by the two-test suite. Four mutation gaps were closed.
The idele-sign fault belongs to slice c and is not claimed as a C kill here.

## Files written

- src/char.c and appended declarations in include/adelefeld/char.h.
- The char validator/typed functions in src/dump.c and declarations in include/adelefeld/dump.h.
- tests/test_char_eval.c and tests/ref/vectors/c-slice2/*.jsonl.
- Character commands/branches in tools/adf/adf.c and their README lines.
- tests/driver/char-eval.cmd/.out and char-dump.cmd/.out.
- tests/julia/char_eval.jl and its block in tests/test_julia.sh.
- The appended Slice b in docs/api-3d.md.
- Lane generator, check/fault/mutation/recheck scripts, red-only stubs, header probe, redgreen.md,
  bounded logs, JSON results, selections and this report.

## Commands and results

Every test program/script was run under timeout. Builds use at most two jobs.
Detailed build/link commands and their exits are in the lane logs and scripts.

1. `timeout 120 python3 -B lanes/c-slice2/gen_vectors.py`: exit 0; 6242 records, 144868 bytes.
   Initial vectors: 6239 records, 143639 bytes. The intermediate principal case gave 143716 bytes.
   `timeout 120 python3 -B proto/char_checks.py`: exit 0; 323350 checks, including faults_33=6.

2. `timeout 180 python3 -B lanes/c-slice2/run_checks.py` builds/runs the four configurations.
   The script's commands are make -s -j2 BUILD=lanes/c-slice2/CONFIG, with SAN=1, INV=1 or CC=clang,
   followed by `timeout 60 lanes/c-slice2/CONFIG/PROGRAM` for each program below.
   The first aggregate final run ended before the last two Clang completions; its final shell tail
   did not retain the underlying script exit. The separate final command
   `timeout 180 python3 -B lanes/c-slice2/run_checks.py clang` exits 0 and completes all nine.
   Final check_results.json records 38 completed executions: 36 required programs, INV wrapper,
   and the unsuccessful LeakSanitizer environment attempt. All 36 required programs exit 0.

| Program | Plain/SAN/Clang checks | INV checks | Final exits |
|---|---:|---:|---|
| test_char | 129725 | 129797 | 0 in all four |
| test_char_eval | 1738298 | 1738325 | 0 in all four |
| test_dump | 12259 | 12259 | 0 in all four |
| test_dump_ctx | 28587 | 28587 | 0 in all four |
| test_dump_golden | 763 | 763 | 0 in all four |
| test_dump_limits | 297 | 297 | 0 in all four |
| test_dump_local | 36205 | 36205 | 0 in all four |
| test_dump_units | 32505051 | 32505051 | 0 in all four |
| test_qclass_dump | 33913 | 33913 | 0 in all four |

   Dump program test counts are 17,14,1,9,10,12 respectively; all have 0 failed checks/tests.
   qclass_dump also completes 2000 random round trips per configuration.
   The final eval-only reruns after using nonprincipal label 3 in the ball-before-D1 assertion
   have the same counts and exit 0 in all four configurations.

3. `ASAN_OPTIONS=detect_leaks=1 timeout 60 lanes/c-slice2/san/test_char_eval`: exit 1.
   LeakSanitizer says it cannot operate under ptrace. detect_leaks=0 passes ASan/UBSan.
   No LeakSanitizer coverage is claimed. FLINT callbacks observe 3300 allocated blocks per 100
   warmed cycles in plain/SAN/Clang and 8100 in INV, with 0 live after every cycle.
   This observes FLINT allocations, not every GMP/runtime allocation.

4. The INV wrapper compile uses cc, ADF_CHECK_INVARIANTS, ADF_CHAR_EVAL_WRAP and linker wraps for
   dirichlet_group_init, adf_phase_get_acb and arb_sqrt_ui. Compile timeout 30: exit 0.
   `timeout 60 lanes/c-slice2/inv/test_char_eval_wrap`: exit 0; 1738351 checks.
   `timeout 150 sh lanes/c-slice2/mutate_check.sh`: final exit 0 under SAN+INV, leak detection disabled;
   wrapped test_char has 129825 checks and wrapped test_char_eval has 1738351.

5. `timeout 180 sh tests/test_driver.sh`: final exit 0; 82 cases, 101494 expected lines, 0 differences.
   Direct calls under timeout 30: default prints the enclosing cardinal square; strict exits 1
   with NOT_DETERMINED; conjugation exits 0 with label 3 and s=(1)+(2)*i.
   `JULIA_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 timeout 180 sh tests/test_julia.sh`: final exit 0;
   slice b 16/16 tests, exports 552/552, 0 missing/undeclared/variadic symbols.
   The suite uses its existing system-GMP preload workaround. Its export build reports an unrelated
   existing lpow.c warning; that read-only file was not changed.

6. Header probe: timeout 30 cc, C11, -Wall -Wextra -Wpedantic -Werror; C++17 compile/link similarly.
   Both timeout 30 programs exit 0; sizeof=120, align=8, general-root precision-cap status=1.
   Cardinal cap tests return OK. The general-root failure preserves z, as the numerical contract allows.

7. `timeout 180 python3 -B lanes/c-slice2/plant_faults.py`: final exit 0.
   Eleven object compiles and 22 links exit 0; 12 test processes abort (-6), 10 exit 0, 0 timeout.
   Each fault is rejected by at least one of the two test programs. Scratch copies are removed.

8. Mutation commands and results are below. Style/cleanup Python scripts run under timeout 20,
   exit 0. Authored C/scripts/prose have 0 lines above 116 and 0 missing final newlines.
   Machine JSONL and command logs retain complete machine records.
   Lane build trees, scratch faults/survivors and six mutation scratch roots are removed.
   The oversized harness transcript copy is removed too; 0 remaining lane logs exceed 100000 bytes.
   Root artifacts from the required driver/exports/Julia scripts are left for the orchestrator.
   check-all was not run.

Development failures are retained in redgreen.md: initial helper compile errors; first link red;
conjugation/dump assertion reds; driver parser reds; a patch rejected before any edit;
Julia relative-path error; the initial cache-observation setup; the intermediate dump_ctx mismatch;
the principal-word-max red; the deferred-product status red; and the aggregate final run interruption.
No failing assertion or statement was weakened to conceal one.

## Named faults

| Fault | test_char exit | test_char_eval exit |
|---|---:|---:|
| n mod C lowering | -6 | 0 |
| parity=n%2 | -6 | 0 |
| zero reported as phase 0/value 1 | -6 | -6 |
| strict writes before failure | 0 | -6 |
| hull from first/last residue only | 0 | -6 |
| use only two coordinate extrema | 0 | -6 |
| conjugate label q-n | 0 | -6 |
| strict accepts only N=0 | 0 | -6 |
| raw chi(c) instead of compatible unit image, F2 | 0 | -6 |
| accept an imprimitive dump | 0 | -6 |
| write x before the last loader check | 0 | -6 |

Five original 3.3 faults and all six requested extra faults are covered.
The sixth original fault drops the idele sign; there is no idele evaluator in this slice.

## Mutation results and survivors

`timeout 1100 python3 -B lanes/c-slice2/mutate_batches.py` exits 0 after six bounded batches.
Each batch runs the following command with J=1..6 and SEED=310309..310314:

    timeout 180 python3 -u tools/mutate/mutate.py --root . \
      --scratch /tmp/adf-c-slice2-mutate-J --files src/char.c --limit 10 --seed SEED \
      --jobs 1 --timeout 45 --san --make 'timeout 150 sh lanes/c-slice2/mutate_check.sh' \
      --copy Makefile include src tests lanes

The check script builds with make -s -j2 SAN=1 INV=1 and runs only wrapped test_char/test_char_eval,
each under timeout 30. Each batch's baseline passes. No source/test was edited during the sweep.

| Batch | Exit | Seconds | Tool killed | Survived | Not compiled | Tool timed out |
|---|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 80.9 | 9 | 0 | 1 | 0 |
| 2 | 1 | 71.5 | 8 | 1 | 1 | 0 |
| 3 | 1 | 140.3 | 6 | 3 | 1 | 0 |
| 4 | 1 | 103.8 | 8 | 1 | 1 | 0 |
| 5 | 0 | 90.3 | 10 | 0 | 0 | 0 |
| 6 | 1 | 147.8 | 9 | 1 | 0 | 0 |

Total: 60 runs, 59 distinct candidates, 634.6 seconds; 50 tool kills, 6 survivors, 4 compile failures.
The duplicate is the infinite CHAR_INDEPENDENT macro. The tool counts its external test timeouts
as killed rather than timed out. These two repeated runs are timeout-based failures, not assertion kills.
The compile failures are :307 and :225 (out-of-bounds arf indexing), :229 (out-of-bounds sums),
and :292 (removed work initialization). They are not test kills.

`timeout 180 python3 -B lanes/c-slice2/recheck_survivors.py`: exit 0; 5 compiles/links exit 0.
Only selected mutations are rerun. Four abort (-6); the infinite macro returns 124 under timeout 2.
The sweep, short rechecks and baselines remain below the 20-minute mutation budget.
Line numbers below refer to the swept char.c; its code was unchanged by the test repairs.

- :190 return true for a nonfinite result: rejected by the added nonfinite sqrt/root injection.
- :330 set up q=1 unnecessarily: rejected by the added principal conjugation setup-count check.
- :296 allow twice the certified cosine width: rejected by the 1.5*2^-53 width injection.
- :280 add rather than subtract the phase distance: rejected by the asymmetric conductor-27 image.

Remaining equivalent survivors, one line each:

- :94 swap n_gcd(n,q) arguments: the set of common divisors and therefore the gcd is unchanged.
- :219 swap the two inputs of exact arf addition: exact addition is commutative and allows output aliasing.

No mutation-tool source or equivalent.txt was changed.

## Findings against the specification

No counterexample was found to the named SPEC statements. SPEC was not edited.
A unit coset contains global units only; its character image never contains zero.
A printed integer c can be a nonunit modulo C. F2 requires a compatible unit lift, not the zero extension chi(c).

## Findings against the design, oracle and goldens

HEADER-FINDING: the design promises extended gcd and exact ball conjugation for the void conjugation call.
The required source-pending fallback needs D(q) group setup/logarithms and has no recoverable setup status.
The implementation has no D1 cap here and fail-stops before a write if setup returns 0.
The universal reachability of such setup failure remains unknown, as in slice a's predicate finding.

HEADER-FINDING: the brief's public dirichlet_char_exp cannot rebuild edited logs.
Header :116-120 only returns cached x->n. The code uses _dirichlet_char_exp, declared at :123.

The initial full-word principal loader mismatch was resolved, not left as a test exception.
Cheap semantics include the known imprimitive principal pair, before D1; this is the design's stage order.
No oracle or golden was changed. The wider asymmetric vector is an additional test, not an oracle repair.
The literal strict singleton criterion concerns ambiguity; the design also permits numerical certificate failure.

## Ground truth, sources pending and not done

The explicit brief requires reading /usr/include/flint/dirichlet.h:
:36 is the component phi field; :84 says the generator powers reconstruct the number;
:114 declares char_log; :116-120 returns cached n; :123 declares _char_exp;
:144-147 identifies n=1 as principal. Exponent negation reconstructs the inverse number.
Its identification with conjugate FLINT characters remains conditional, not universally proved here.
refs/src/flint-3.0.1/acb_dirichlet.rst:337-342 names the pairing without symmetry/formula;
:466 names the Conrey isomorphism without defining its coordinates.
Other read sources: acb.rst:417-419 (conjugation), arb.rst:1125-1138 (rational trig),
arf.rst:24-35 (rounding), mag.rst:6-17 (conversion allowance), memory.rst:9-44 (allocation/caches).
The P1/P2 arguments and conditional P3 steps are written out in docs/api-3d.md.

- [source pending: FLINT 3.0.1 universal Conrey pairing/exponent implementation under refs/].
- [source pending: full-word dirichlet_group_init failure semantics, inherited from slice a].
- [source pending: signed primitive quadratic Gauss evaluation, inherited and unused here].

`timeout 20 curl -LfsS` for the official dirichlet.rst source URL exits 6: DNS resolution fails.
No universal P3 proof or LeakSanitizer run is claimed. No long differential fuzzing was done.
Class/idele evaluations, character products and the idele-sign fault remain outside this slice.
Avoidable costs: four distance comparisons per compatible residue, repeated predicate setup under INV,
and setup for exact +/-1 evaluations. Conjugation's setup cost is the recorded source-dependent discrepancy.
