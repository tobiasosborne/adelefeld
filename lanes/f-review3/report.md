# f-review3: adversarial review of local exp, log and Log

## Findings

### R1. MAJOR: runtime exceeds 10 seconds at a one-word prime

Prime p = 18446744073709551557 = 2^64 - 59.

Input A: exact x = 18446744073709551558 = 1+p; function log; requested N = 10000.

The code returns OK, exact=0, v=1, K=10000, with a canonical output.
The complete unit is in bench.out, row 7. Its residue agrees with my independent exact rational oracle.
The first measured call took 59.218463789 seconds. A separate one-thread call took 65.234967101 seconds.
Both exceed the brief's 10-second criterion for N <= 10000.

Input B: exact x = 2; function Log; the same p and N.

The single-call program did not finish within 120 seconds. timeout returned 124.
No function status or output was observed for that call. This exceeds the same criterion.

Reproduce after the compilation commands below:

```sh
timeout 120 lanes/f-review3/build/timing log 18446744073709551557 10000 18446744073709551558
timeout 120 lanes/f-review3/build/timing Log 18446744073709551557 10000 2
```

The input sizes and requested exponents are allowed. For input A, v(x-1)=1.
Since all retained degrees are below p, T=9999 and W=10000. W*bits(p)=640000 is below 67108864.
The loop at src/lfunc.c:339-359 performs a unit inverse, multiplication and reduction for each degree.
The result for input A is numerically correct. This finding uses the runtime criterion of this review.
The header at include/adelefeld/lfunc.h:59-60 states that cost is not bounded by the limits.

Evidence: timing-log.log, timing-Log.log, bench.out, bench-validate.log.

### R2. MINOR: F2's prose claims more than its proof establishes

Location: docs/api-1f4.md:97-98, after the proof of F2.
It says that the exact results are all the rational values of log and Log at rational arguments.

The preceding proof assumes Log(x)=0 and classifies that zero fibre.
For Log(x)=q with q a nonzero rational, injectivity gives u=exp(q), not u=1.
None of that proof's steps excludes a rational x with such a q.
The statement needs a further argument. The bold F2 statement only classifies zero values.

Concrete uncovered input: exact x=4 at p=3, N=12, for both log and Log.
The code returns OK and the ball 303798 + 3^12 Z_3:
exact=0, v=1, unit=101266, K=12. This residue agrees with the independent oracle.
A finite residue and the zero-fibre proof do not establish that the exact value is not a nonzero rational.
This finding is a missing implication; no rational-valued counterexample is asserted.

```sh
timeout 30 lanes/f-review3/build/probe < lanes/f-review3/proof-gap.in
```

Read the failed implication alongside oracle-proof.md, final paragraph.
Evidence: proof-gap.in, proof-gap.out, precision.log.

[source pending: a proof excluding nonzero rational log and Log values at rational arguments]

## Attacks without an enclosure or status finding

Every normal probe request is repeated with y=x.
Every non-OK result is checked against an unchanged sentinel and an unchanged aliased input.
Every OK result is checked for canonical form.
A value check fails on any different residue. A radius check fails on any different K or exact flag.
For ball samples, a check fails when any sampled point's value lies outside the returned ball.
A tightness check fails unless two image residues differ modulo p^(E+1), while agreeing modulo p^E.

The independent oracle imports no author reference and no padic implementation.
It evaluates exact rational partial sums, using my proved infinite-tail bounds.
At odd primes its general Log route lifts the torsion root by Newton steps.
At 2 it uses the raw log series of the odd unit, including units congruent to 3 modulo 4.
Balanced evaluation at large N changes only the order of exact rational arithmetic.
The tail bounds and the finite root replacement are proved stepwise in oracle-proof.md.

- Small cases: 16443 requests, 32886 function calls, 31143 value/enclosure checks,
  1945 smallest-ball witnesses, 273 exact results and 11151 non-OK statuses.
  Primes: 2, 3, 5, 7, 11 and 2^64-59. Requested N: -3, 0, 1, 2, 4, 9, 13.
  Ball exponents: -4, -1, 0, 1, 2, 3, 5, 8; exact inputs also tested.
  Includes exp's boundary and one below it; log of 3 at 2; log(-1);
  zero-containing balls; denominators; Log at negative and positive centre valuation;
  and the precision-loss centres 3, 12 at 3 and 2, 10 at 2.
- Perturbations include p^M, (p-1)*p^M, -p^M, p^M/(p+1), p^(M+1) and (p+1)*p^M.
  Thus the final admitted digit and fractional perturbations are tested.
- Exhaustive requested precision 1 through 300 at 2 and 3: 12 centres, 3600 requests,
  7200 calls and 3600 comparisons with a separately computed residue at precision 304.
  The centres include rational denominators, negative signs, raw log of 3 at 2,
  and Log with centre valuation -4. Powers of p and the next requested precision are included.
- Seeded denominator cases: 3000 requests, 6000 calls, 16499 value/enclosure checks,
  1477 smallest-ball witnesses and 1 exact result. Denominators range from 1 to 31, prime to p.
  Centre valuations range from -6 to 5 before the domain adjustment.
- General large-prime units: 80 requests and 160 calls, precision 1 through 20.
  Includes Log(2/p^3) and Log(3*p^4/(p+1)), using the independent root lift.
- Large precision: 30 requests and 60 calls at N=2000 and N=10000, for p=2,3,5,7,11.
  All 30 residues agree with the exact rational oracle. Maximum measured call: 2.608263077 seconds.
- The 7 completed benchmark rows, including exp(p) and log(1+p) at the large prime and N=10000,
  have 7 independent high-precision residue comparisons, with 0 differences.
- Large-prime powered route: Log(-(p+1)/p^4) at N=2000 and its ball of exponent 16.
  Two requests, 4 calls, 8 value/enclosure checks and 1 smallest-ball witness.
  Its residue p-1 forces the library's a^(p-1) route; the oracle uses the exact torsion factor -1.
  The exact input has image precision 2000. The ball's centre valuation is -4, so it gains 4 digits:
  its returned exponent is 20. Maximum measured call: 3.408537819 seconds.
- Limits and check order: 161 requests, 322 calls and 161 status/alias comparisons.
  Includes LONG_MIN, LONG_MAX, +-2^60, input bounds before domain checks,
  working-power limits, relative exponent 2^61, exact results and no-power shortcuts.
- Memory run: 600 requests, 1200 calls, 385 independent residue comparisons and 215 non-OK statuses.
  Valgrind counted 5486 allocations, 5486 frees, 279080 allocated bytes,
  0 bytes at exit and 0 errors from 0 contexts.
- Sanitized target translation unit plus invariant checks: the small and limit suites above,
  16604 requests and 33208 calls. Address/undefined-behaviour diagnostics: 0.
  LeakSanitizer was disabled for these completed runs; Valgrind checked leaks separately.
- The author's C test program: 10 tests, 374421 checks, 0 failed checks and 0 failed tests.

## Proof review

Proposition 7b: the lower bound k*v-floor(log_p k) is nondecreasing, including at powers of p.
Its first qualifying degree bounds every later omitted term. The comparison with the safe count follows.
No failure was found in the tail argument.

Proposition 8: both the numerator residue and the modulus are divisible by the needed p^e.
The exact integer division leaves W-e >= n digits. Unit inversion and addition lose no digits.
The proof handles constant and empty sums separately. No failure was found.

F4: the integral numerator polynomial has every summand divisible by p^D.
Reduction modulo p^(K+D) followed by division retains K digits.
This remains valid for rational centres with unit denominators. No failure was found.

F5: the valuation of a nonzero residue below W is the true valuation; a zero residue supplies lower bound W.
Increasing the bound cannot increase the needed count. The code's zero shortcut uses valuation at least c.
No failure was found.

F1, F3, F6 and F7: referee notes are in oracle-proof.md.
In particular, p-1 is a unit at odd p, so that division loses 0 absolute digits.
The exponent subtraction is bounded before it occurs. The exp count denominator is positive on its domain.
At 2, the domain requires w >= 2, so w-1 >= 1. The author's division-by-zero domain mutant cannot be
reached by a canonical, accepted input with the unmutated domain check.

## Sensitivity checks

Four changes were made only to copies in this lane. The normal archive was not changed.

1. One fewer exp degree: caught at exp(4), p=2, N=3; mutant residue 1, oracle residue 5.
2. One fewer log degree: caught at log(3), p=2, N=3; mutant residue 0, oracle residue 4.
3. E=r at relative exponent 1 at 2: caught on log(1+2 Z_2), N=2;
   mutant exponent 1, required exponent 2.
4. Omit division by p-1: caught on Log(2/p^3) at p=2^64-59, N=2.

Each test process exited 1 with an assertion failure. The fault harness exited 0.
No test assertion or precision statement was weakened.

## Commands and results

All test programs and Python scripts below ran under timeout. No git command and no bd command was run.
All writable paths used were inside lanes/f-review3.

Build and probe compilation, exits 0:

```sh
timeout 180 make -j2 BUILD=lanes/f-review3/build
timeout 180 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude \
  lanes/f-review3/probe.c lanes/f-review3/build/libadelefeld.a \
  -lflint -lgmp -lm -o lanes/f-review3/build/probe
timeout 180 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude \
  lanes/f-review3/timing.c lanes/f-review3/build/libadelefeld.a \
  -lflint -lgmp -lm -o lanes/f-review3/build/timing
```

Independent checks, all exit 0:

```sh
timeout 180 python3 -B lanes/f-review3/oracle.py precision
timeout 180 python3 -B lanes/f-review3/oracle.py small
timeout 180 python3 -B lanes/f-review3/oracle.py big
timeout 180 python3 -B lanes/f-review3/oracle.py limits
timeout 180 python3 -B lanes/f-review3/oracle.py random
timeout 180 python3 -B lanes/f-review3/oracle.py large
timeout 180 python3 -B lanes/f-review3/oracle.py bench_validate
timeout 180 python3 -B lanes/f-review3/oracle.py powered_large
```

Results, respectively: 3600, 16443, 80, 161, 3000, 30, 7 and 2 requests checked.
The completed large run is in large-balanced.log, after the balanced exact evaluator was added.
The earlier run in large.log exited 124: it produced 30 C rows, then timed out in my slow rational oracle.
Those rows were not counted as checked until the replacement run completed.

Initial benchmark:

```sh
timeout 180 lanes/f-review3/build/probe < lanes/f-review3/bench.in > lanes/f-review3/bench.out
```

It emitted 7 of 8 rows; the final Log case did not complete. The timeout exit was not separately saved,
because the shell then printed the available rows. The separate R1 single-call run saved exit 124.
The completed rows' call times in seconds were 0.017060562, 0.032848061, 0.060726516,
0.222215466, 0.222464831, 0.058790542 and 59.218463789.

Memory commands, all exit 0:

```sh
timeout 180 python3 -B lanes/f-review3/oracle.py memory_inputs
timeout 180 valgrind --leak-check=full --show-leak-kinds=all \
  --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
  --log-file=lanes/f-review3/valgrind.log lanes/f-review3/build/probe \
  < lanes/f-review3/memory.in > lanes/f-review3/memory.out
timeout 180 python3 -B lanes/f-review3/oracle.py memory_validate
```

Results: 600 inputs written; 0 memory errors and 0 leaked bytes; 600 output/status rows checked.

Sanitizer compilation, exit 0:

```sh
timeout 180 cc -std=c11 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -DADF_CHECK_INVARIANTS \
  -Iinclude -Isrc src/lfunc.c lanes/f-review3/probe.c \
  lanes/f-review3/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review3/build/probe-san
```

src/lfunc.c and the probe are instrumented. Other archive objects are the normal build.

```sh
timeout 180 python3 -B lanes/f-review3/oracle.py small build/probe-san
ASAN_OPTIONS=detect_leaks=0 timeout 180 python3 -B lanes/f-review3/oracle.py small build/probe-san
ASAN_OPTIONS=detect_leaks=0 timeout 180 python3 -B lanes/f-review3/oracle.py limits build/probe-san
```

The first command exited 1 after 16443 output rows: LeakSanitizer reported that it cannot run under ptrace.
The next two exited 0: 16443 and 161 requests checked, with 0 address/undefined-behaviour diagnostics.

Sensitivity and the author's tests, exits 0:

```sh
timeout 180 python3 -B lanes/f-review3/faults.py
timeout 180 make -j2 BUILD=lanes/f-review3/build lanes/f-review3/build/test_lfunc
timeout 180 lanes/f-review3/build/test_lfunc
```

The fault script runs each compilation under timeout 60 and each oracle under timeout 30.
Results: 4 of 4 faults caught; 10 author tests and 374421 author checks, 0 failures.

The R2 reproducer exited 0 and emitted the 2 rows described in that finding.

Formatting checks used timeout 5 awk to print lines longer than 116 in oracle-proof.md,
probe.c, timing.c and faults.py. Result: 0 printed lines in each.
scope.sha256 records the six reviewed files, generated with timeout 5 sha256sum.
Final checks: timeout 5 test -s lanes/f-review3/report.md exited 0;
timeout 5 awk printed 0 report lines longer than 116;
timeout 5 sha256sum -c lanes/f-review3/scope.sha256 returned 6 matches and 0 mismatches.

One scheduling mistake overlapped the first large oracle, limits and random processes for about 4 seconds.
That interval had three test processes. It did not contribute an R1 timing measurement.
Build parallelism was -j2. The dedicated R1 timing program explicitly used one FLINT thread.

## Files written

- oracle.py, oracle-proof.md, probe.c, timing.c and faults.py.
- fault-1.c, fault-2.c, fault-3.c and fault-4.c: copied target with one intentional fault each.
- bench.in, bench.out, proof-gap.in, proof-gap.out, memory.in and memory.out.
- build.log, author-build.log, author-test.log, precision.log, small.log, big.log, limits.log,
  random.log, large.log, large-balanced.log, bench-validate.log and powered-large.log.
- timing-log.log, timing-Log.log, sanitize-small.log, sanitize-small-nolsan.log and sanitize-limits.log.
- memory-inputs.log, memory-validate.log, valgrind.log, valgrind-exit.log, faults.log and fault-1.log
  through fault-4.log.
- scope.sha256 and report.md.
- build/: the archive, library/support objects and dependency files; test_lfunc; probe, timing,
  probe-san and probe-fault-1 through probe-fault-4.

Existing brief.md, lane.log and stdout.log were not edited by this review.

## Not done

- A full residue oracle for Log(2) at the large prime and N=10000: the C call exceeded 120 seconds.
- Exhaustion of all rational centres or all points of every input ball.
- A long fuzz campaign; the seeded 3000-request run is finite sampling.
- A repeat of the author's Julia suite or proto/lfunc_checks.py. Their source was reviewed.
- LeakSanitizer under ptrace. The separate Valgrind run covered 600 requests.
- A proof or refutation of the stronger rational-value sentence following F2.

## Sources pending

[source pending: a proof excluding nonzero rational log and Log values at rational arguments]

No external formula is supplied from memory. The oracle proofs are written out in oracle-proof.md.
FLINT API conventions used by the harness are cited there or in the C comments to on-disk refs files.

## Findings against the specification

No counterexample to SPEC 9.3.2 was found.
R1 concerns the runtime criterion in this brief. R2 concerns an unsupported sentence in api-1f4.md.
