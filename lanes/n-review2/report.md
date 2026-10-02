# Lane n-review2

Read CLAUDE.md, the named contracts and PLAN milestones, the repair report, and the original finding list.
All writes are in lanes/n-review2. The normal archive was built once with make -j2.
No state-changing git command, tracker command, package installation, or repository test suite was run.
The only git command used was read-only git show to obtain the two old printer source files at 674c9db.
There are two MINOR documentation findings below. The probes found no BLOCKER or MAJOR code defect.

## Monotonicity: exact counterexample

Input: mid = 793/512, rad = 741/512, digits = 1. Its exact lower endpoint is 13/128 > 0.
Both exponents are 1. The radius is exactly representable with the 30-bit mag mantissa
(refs/src/flint-3.0.1/mag.rst:6-8). The stored idele and class pass their canonical predicates.

1. At k = 2, nk = 1 and q = 0. Rounding mid gives M = 2.
2. E = rad + abs(M-mid) = 243/128 = 1.8984375. Upward rounding to two digits gives R = 1.9.
3. q' = 0 and M passes the multiple test. The output is 2 +/- 1.9, with lower endpoint 1/10 > 0.
4. At k = 3, nk = 2 and q = -1. Rounding mid gives M = 1.5.
5. E = rad + abs(M-mid) = 383/256 = 1.49609375. Upward rounding to three digits gives R = 1.5.
6. q' = -1 and M passes the multiple test. The output is 1.5 +/- 1.5, with lower endpoint 0.

Thus k1 = 2 satisfies both sign conditions and k2 = 3 satisfies neither. This disproves monotonicity from 2.
It does not disprove eventual monotonicity after an input-dependent level. This is an own proof.

The Python reference and the current C level function both give these exact texts. The public current and old
printers return `(2 +/- 1.9 ; 1 * [1])`, length 21. Current idele and class refusal decisions agree.

Commands, from the repository root:

```sh
timeout 60 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/n-review2/monotonicity.py
timeout 60 lanes/n-review2/printer_probe level
timeout 60 lanes/n-review2/printer_probe counterexample
```

Results: 4 Python assertions, 0 failures; C levels 2..5 have predicates 1, 0, 1, 1; public probe 0 failures.
Logs: monotonicity.log, level.log, counterexample.log. The expanded proof is in proofs.md A.

## Findings against the specification

### F1. MINOR: the reported refusal threshold is inaccurate

Locations: docs/SPEC.md:929, row N-D11; docs/api-2.md:671; lanes/n-repair1/result.md:59.
The texts say that refusal starts at about 5500 bits in the stated family.

Input family: mid = 2^(b-1) + 1/2, rad = 2^(b-1), digits = 1, with content 1 and unit [1].
The same real part is used in a class. Both stored exponents are b.

| b | Current result | Formed levels | Work charged to formed levels | Seconds, idele call |
|---|---|---|---|---|
| 4000 | 2438 bytes | 2410 | 9642410 | 0.301724 |
| 5500 | 3342 bytes | 3314 | 18230314 | 0.744224 |
| 7462 | 4522 bytes | 4494 | 33538722 | 1.378537 |
| 7463 | NULL, len 0 | 4495 | 33550680 | 1.147236 |
| 8000 | NULL, len 0 | 4193 | 33548193 | 0.952693 |
| 100000 | NULL, len 0 | 335 | 33500335 | 3.072608 |

The true first refusal is b = 7463. Here is an exact proof, independent of timing.

1. For b >= 2, let r = 2^(b-1) and D = X(r). r is an even integer and X(r+1/2) = D.
2. The necessary lower bound proved in F2 gives k >= D+2.
3. At k = D+2, q = 0. Ties to even rounds r+1/2 to r. R = r+1/2 exactly.
   The output lower endpoint is -1/2, so this level fails.
4. At k = D+3, q = -1, and both M = r+1/2 and R = r are exact. The level succeeds.
5. The exact same rational pair starts the second pass. The same text appears and the call ends.
   Therefore exactly 2(D+2) levels are required, all with S = max(64,b+1).
6. Work is 2(D+2)max(64,b+1), which is nondecreasing in b.
7. At b = 7462, D = 2245: work = 33538722 <= 33554432.
   At b = 7463, D = 2246: work = 33558144 > 33554432.
   The b = 1 ball also prints.

The direct calls at both boundary inputs agree with this proof. The replay scanned 2000 inputs,
b = 6001..8000: first refusal 7463, 538 refused, 0 reversals. It skips only proved-failing levels but
charges their work, so it predicts the existing linear search rather than changing its budget.

Reproduce with:

```sh
timeout 60 lanes/n-review2/printer_probe family 5500
timeout 60 lanes/n-review2/printer_probe family 7462
timeout 60 lanes/n-review2/printer_probe family 7463
timeout 60 lanes/n-review2/family_scan
timeout 60 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/n-review2/arithmetic_checks.py
```

The last command has 5 exact arithmetic checks and 0 failures. Proofs.md C gives the details.
This finding concerns the description of the accepted work bound; it is not a wrong enclosure or refusal.

### F2. MINOR: a necessary lower bound does not require monotonicity

Locations: docs/SPEC.md:929 says both alternatives need monotonicity. The alternatives cell of
docs/api-2.md:701 says a proved lower bound does not exist. The broader statements are false.
Statement Q's narrower claim that it has not proved a bound from the two separate stored exponents is
not refuted here. The bound below also uses the exact margin.

Input: any positive ball mid > rad > 0. Put delta = mid-rad. At a successful level k, set
U = 10^(X(rad)-k+1).

1. E = rad + abs(M-mid) >= rad, so the rounding unit of R is an integer multiple of U.
   Therefore R is a multiple of U.
2. The final q' is at least X(R)-k+1, which is at least X(rad)-k+1 because R >= rad.
   The multiple test implies that M is a multiple of U too.
3. Success means M-R > 0. Hence M-R >= U. Enclosure gives M-R <= delta.
4. Thus U <= delta, and k >= X(rad)-X(delta)+1.

All levels below max(2, X(rad)-X(delta)+1) can be skipped without any monotonicity assumption.
For the F1 family this gives k >= D+2; the next level D+3 succeeds exactly.
The code still searches from k = 2 and refuses b = 7463 as reported above. No production change was made.

The source-including family_scan.c implements this proved skip for the replay. Reproduce with
`timeout 60 lanes/n-review2/family_scan`: 2000 inputs, 0 disagreement with the exact family formula.
Proofs.md B gives the general proof. This finding does not justify binary search for the least level.

## Attacks without a code finding

### D2: counter, refusal paths, exact points, and historical text

The counter is tested before tx_real_level, at src/text.c:2023-2030. The brief's suggestion that one extra
level might be formed is not what this implementation does. Completed levels never exceed 2^25 work.
The attempted charge can exceed the bound by at most S of the refused level; that level is not formed.
At b = 7463 the attempted total is 33558144, an accounting excess of 3712; completed work is 33550680.
At b = 100000 the attempted total is 33600336, an accounting excess of 45904; 335 levels were formed.
The counter persists into the second pass, as the b = 7463 and 8000 probes demonstrate.
No overflow or wrong comparison was found in these cases or in the source inspection.

The historical comparison used git show at 674c9db and 17 renamed exports. The current copies add counters.
The first run compared 1280 calls on 320 generated balls. The expanded run compared 1600 calls on the same
320 balls, now including both old public printers. There were 0 mismatches in either run.
The balls include positive and negative idele parts, dyadic scaling from -1000 through 1000, near-zero
endpoints, and digits 1..35. A changed byte, changed refusal, nonzero len on NULL, or current/instrumented
disagreement would fail the probe. This is a bounded differential probe, not a long fuzz run.

Exact radius-zero points 2^99999 and 2^-99999 at digits 1000000 each use two counted levels and 200000 work.
They print 30122 and 69917 bytes, in 0.012074 and 0.054311 seconds respectively.
The exact points 1+2^-1000000 and 1+2^-8000000 at digits 1 print 29 and 30 bytes.
They use two levels and 2000005 and 16000005 work, in 0.480888 and 3.544023 seconds.
These expose dependence on mantissa size, which the header explicitly retains. The counter is below its
bound in both cases, so these timings do not demonstrate a failure to stop past the bound.

The positive idele, class, and negative idele refusal paths at b = 100000 all return NULL with len 0.
FLINT and GMP allocation hooks record 7086 allocation events, peak 4071 live blocks, final 0 live blocks
after all objects and caches are cleared. A nonzero final count, non-NULL result, or nonzero len would fail.
The allocation interface and cleanup are documented at refs/src/flint-3.0.1/memory.rst:16-21 and :29-35.
LeakSanitizer was disabled as required; this was an explicit allocator balance check.

Driver checks at prec 20000, digits 1: real text 1 +/- 0.<1000 nines> prints for both types, with lengths
2022 and 2018. With 1500 nines both print error: LIMIT. There are 0 stderr bytes and 0 check failures.

### D1: every documented command's operand positions

driver_matrix.py generated 368 commands using a unit coset, idele, class, and complex adele in the operand
slots, plus all 4 by 4 type pairs for eight binary operations. It includes settings, solver inputs, load,
the one- and three-operand reconstruct forms, and both real and finite places for the repaired commands.
There are 62 cases whose type combinations are documented to admit values. The remaining 306 must return
UNSUPPORTED, DOMAIN, or PARSE. All 368 lines were present; 0 unexpected outputs, 0 stderr bytes.
The driver returned the expected exit 1. Its own code was compiled with ASan and UBSan; the archive was normal.
The allowed-value cases were exercised for dispatch and memory faults, not given a new numerical oracle.

### C1: extreme exponents and omitted zero-centre cases

edge_probe.c constructs 18 canonical huge inputs: zero-centre balls, exact nonzero values, and nonzero
balls near LONG_MIN, LONG_MAX, and both sides of ADF_LBALL_EXP_MAX. It pairs them in both orders with
four small values, including exact zero and a ball around zero. It tests powers at LONG_MIN and LONG_MAX,
decompositions, fractional parts, exp/log/Log, arithmetic, and exercises the accessors and predicates.
There are 1152 asserted checks with 0 failures. Every asserted non-OK result checks the stated status;
the local arithmetic and decomposition checks also check that the relevant outputs stay untouched.

The first UBSan run instrumented only the harness. The second includes the read-only lball.c,
lball_decomp.c, and lfunc.c in focused reproducer objects, linked against the normal archive for the rest.
The second run also has 1152 checks, 0 failures, and 0 UBSan diagnostics.
The six huge zero-centre decompositions give NOT_DETERMINED before the exponent test, as the detailed
decomposition contract specifies. Division by a small zero divisor follows the explicit precedence in
lball.h:229-230. These are not reported as overflow findings.

The source audit found bounds before the exponent differences that remain reachable. For bounded inputs,
the ring-operation sums and differences have at most four terms of size 2^60 and fit in slong.
Integer-power products are preceded by division-based magnitude checks. The series term-count arithmetic
is reached after the working precision bound. A sanitizer diagnostic, wrong status, or changed protected
output would have made these edge cases fail.

### R5: allocation before an excessive real precision is rejected

precision_probe.c includes the two read-only sball.c and rfunc.c implementation files with INV enabled;
the remaining dependencies come from the normal archive. The control invariant check allocates 14 times,
135160 bytes. Thus the allocator hook is exercised by the input used.
It calls all 3 sball ring operations, all 8 functions at the real place including log_abs_at, and all
7 real-ball functions, at MAX+1 and LONG_MAX. Both root wrappers use degree 0 to test LIMIT precedence.
All 36 calls return LIMIT, allocate 0 times and 0 bytes, and leave their value outputs untouched.
The real-place report is checked for the 22 partial-ball calls. There are 0 failures or UBSan diagnostics.
The source guards precede the invariant checks and reads of the value operands.
The lfunc precision is an absolute p-adic precision; the real-precision ceiling does not apply to it.

### C2: numerical attack and the remaining proof obligation

double_bound.c compares the computed bound to outward Arb enclosures at 256-bit precision.
The enclosure contract is refs/src/flint-3.0.1/arb.rst:6-12. It uses 1191 integer p values, including
integers adjacent to powers of two, 2^64-1, and 1000 generated primes. For each it uses B = 1, 64,
2^26, and 2^28. There are 4764 bound checks, 0 failures. The maximum observed upper bound on absolute
log2 error is 3.7019345996155642e-15. A computed upper bound below the enclosing exact quotient fails.
No huge H needs to be constructed for this direct attack on the claimed numerical inequality.

Proofs.md D gives a conditional proof with every rounding allowance explicit. It assumes integer conversion
and division relative errors at most 2^-52, and libm log2 relative error at most 2^-51.
The conversion changes the exact logarithm by less than 2^-50. The total logarithm relative error is
less than 2^-49. The quotient absolute error is then less than B*2^-48: below 2^-22 for B <= 2^26,
and below 2^-20 for B <= 2^28. Since v_p(H) < B/log2(p), the computed t is greater than v_p(H)-1.
The code computes floor(t)+1 in integer arithmetic, so this is an upper bound under those hypotheses.
The three rational inequalities in this argument were checked exactly by arithmetic_checks.py.

The universal libm hypothesis has not been sourced or proved. The sample does not prove it for every p.
This review therefore does not claim to have completed the requested unconditional C2 proof.

### Repair tests read without execution

The named tests in test_text_idele.c, test_lball.c, test_sball.c, and test_rfunc.c were read.
Their assertions can fail. No vacuous assertion was identified. The printer timing test uses clock(),
which measures CPU time rather than an independent wall-clock deadline, and checks only after the call
returns. It is not itself a timeout. No repository test program was executed in this lane.

## Commands and checks

checks.md records every build, compilation, and check command, including the repeated differential run,
the two UBSan variants, the source extraction, each individual family/point call, and their numeric results.
The one library build returned exit 0 and produced 29 objects and one archive. All probe compilations
completed. All checking programs returned 0. The two driver executions returned expected exit 1 and their
checking scripts returned 0. No timeout expired and no sanitizer diagnostic appeared.
The first monotonicity log mislabelled four assertions as three; the printed count was corrected to four
and rerun without changing an assertion. The Markdown audit found 0 lines over 116 characters in the
three supporting Markdown files. The report is written once, after the work.

## Files written

Everything below is relative to lanes/n-review2; pre-existing brief and harness logs are not authored here.

- report.md, progress.md, proofs.md, checks.md.
- monotonicity.py, arithmetic_checks.py, prepare_text.py, driver_matrix.py, driver_limit.py.
- printer_probe.c, family_scan.c, edge_probe.c, edge_impl.c, precision_probe.c, leak_probe.c, double_bound.c.
- Extracted or instrumented copies old_text.c, old_text_idele.c, review_text.c, review_text_idele.c.
- Driver command files driver-matrix.cmd and driver-limit.cmd, their .out, .err, and .log files.
- build.log and the normal build/ archive, 29 objects, and dependency files.
- Probe executables and object files in this lane; all numerical logs named in checks.md.

## What is not done

No production or specification edit. No exhaustive search of all admitted balls, all input precisions,
or all type values. No full sanitizer or INV library build. No repository suite or mutation run.
No theorem about eventual monotonicity of the level predicate. No proof from only the two separate
stored exponents. No unconditional C2 error theorem for the installed libm. No claim that a finite
differential probe or allocator-balance run excludes every text or memory defect.

## Sources pending

- [source pending: a documented error bound for the installed C libm log2 on these positive inputs]
- [source pending: a source under refs/ for the conversion and division rounding model in proofs.md D]

The monotonicity counterexample, necessary lower bound, and exact family work formula are own proofs.
