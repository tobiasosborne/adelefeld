# f-review5 report

## Findings

### R1. MINOR: F13 gives an unqualified, incorrect status for LONG_MIN

Location: docs/api-1f4.md:422-423. The sentence says LONG_MIN "returns LIMIT unless an exact-zero
result ignores it." This omits the earlier domain check.

Input: each of sin, cos, sinh, cosh at p=2, N=-9223372036854775808, on either exact 2 or 2 Z_2.

The code returns DOMAIN for exact 2 and NOT_DETERMINED for 2 Z_2. It returns LIMIT in none of these
eight calls. Each distinct output remains the exact sentinel -23/29 at p=19. Aliased inputs also
remain unchanged. The named-place calls give the same status and set where to 2.

Why: exact 2 lies outside 4 Z_2. The ball 2 Z_2 contains 0, which is in 4 Z_2, and 2, which is not.
The domain check at src/lfunc.c:759 precedes the requested-precision check. These statuses implement
the specified check order. The sentence in F13 should be qualified by successful input and domain checks.
This is a documentation finding, not an implementation status failure.

Reproduce from the repository root after the probe compilation listed below:

    ASAN_OPTIONS=detect_leaks=0 timeout 10 lanes/f-review5/build/probe \
      < lanes/f-review5/f13.in

Exit 0. The first four output lines start with status 7, the next four with status 1.
The diagnostic line is rows=8 checks=138 failures=0. Evidence: f13.in, f13.out, f13.stderr.

### R2. MINOR: the absent parity doubles most of the Horner work

Location: src/lfunc.c:727-735. This cost was acknowledged in the f-slice7 report but not measured there.

Input: exact x=3/2 at p=3, requested N=2000, all four functions.
The current code returns OK and a ball of exponent 2000 in every case.

A reviewer implementation pairs two consecutive Horner steps. It uses the same modulus, same finite
sum, same denominator division, and returns the same residue in all four cases.

| Function | Current steps | Paired steps | Current CPU ms | Paired CPU ms | Ratio |
|---|---:|---:|---:|---:|---:|
| sin | 3997 | 1999 | 76.225436 | 36.009152 | 2.117 |
| cos | 3998 | 1999 | 71.580113 | 34.905123 | 2.051 |
| sinh | 3997 | 1999 | 68.015647 | 33.988612 | 2.001 |
| cosh | 3998 | 1999 | 86.436150 | 36.421342 | 2.373 |

There was one timing run, with one call per function and implementation. These are individual measurements,
not a distribution or a general performance bound. The residue comparison has four equalities and zero failures.

Own proof that the work is avoidable:

1. For a step from a retained degree k, the coefficient at k-1 is zero.
2. The first step gives F'=kF and A'=xA.
3. The second gives F''=(k-1)F' and A''=xA'+epsilon_(k-2)F''.
4. Substitute: F''=k(k-1)F and A''=x^2 A+epsilon_(k-2)F''.
5. Reduction modulo p^W commutes with each integer operation. The paired state is identical.
6. For an odd top degree, retain the final multiplication by x. The final division is unchanged.

The omitted work is a modular multiplication and associated reductions for each absent coefficient.
No precision or domain policy needs to change.

Reproduce:

    timeout 45 lanes/f-review5/build/cost_probe

Exit 0, four equal=1 lines. Program and single-run evidence: cost_probe.c and cost.log.
The same x=3/2, p=3, N=2000 also occurs in the independent large-precision oracle attack.

## Attacks without another finding

The independent oracle is attack.py. It imports no lane oracle, fixtures, or evaluator.
Its tail proof and rational evaluation proof are in its opening comment:

1. Counting multiples of p^j in k! gives v_p(k!)=sum floor(k/p^j)<=k/(p-1).
2. On the admitted domain, v_p(x^k/k!)>=k/2.
3. At absolute precision H, retain every degree k<2H. All omitted terms lie in p^H Z_p.
4. Their valuations tend to infinity. Closedness gives the same bound for the infinite tail.
5. For modest operands, Fraction sums the rational terms exactly before reduction.
6. Larger operands and H=2000 use exact residues of those rational terms. Strip p factors from
   each denominator, invert its unit part, and sum modulo p^H. This changes no finite-sum residue.

Twenty-one cases at H=27 compare the Fraction and modular paths. All twenty-one agree.
The first local run timed out during large Fraction arithmetic. The completed rerun uses modular
rational residues for operands above 256 bits, retaining the same degrees and requested precision.

| Attack | Cases | C assertions | Further checks | Failures |
|---|---:|---:|---|---:|
| Local exact points and balls | 2016 | 34274 | 21 oracle path comparisons | 0 |
| Small-ball enumeration | 252 | 4286 | 2088 image points; 116 hull witnesses | 0 |
| Status and limit boundaries | 2548 | 43318 | Domain, result and working-power limits | 0 |
| Absolute precision 2000 | 32 | 546 | Independent rational residues | 0 |
| Old exp/log/Log comparison | 1008 | 21170 | Distinct and aliased old results | 0 |
| Real-place wrappers | 1264 | 5387 | Complete arb outputs and status order | 0 |
| Driver | 340 | 340 line comparisons | 224 library results; 116 hostile commands | 0 |

The stream probe runs each case with distinct and aliased local outputs, then with distinct and
aliased named-place outputs. It also requests an absent prime. On success it checks the exact fields,
the single surviving component, canonical form, and unchanged where. On failure it checks unchanged
values, local object bytes, and the reported prime. Partial inputs include a second prime and alternate
between real and complex archimedean components. The extra components cannot influence the chosen prime.

The local attack uses p=2,3,5,7,13,65537,18446744073709551557. It includes signed rational centres,
centred and noncentred balls, negative requested precision, and large numerators and denominators.
Random unit operands are drawn with up to 4096 numerator bits and 3072 denominator bits, then scaled by p^c.
A wrong residue, prime, exponent, exact flag, status, or alias result would fail.

Enumeration uses p=2,3,5,7, M=c,c+1 and every admitted integer representative modulo p^(c+2).
Each ball is requested below, at, and above E. For sin/sinh and centred cos/cosh it requires two
oracle images whose difference has valuation exactly E. The centred cases include 4 Z_2 and 8 Z_2.
Both even functions return exponents 3 and 5 there. A smaller promised hull losing a point, a larger
centred hull, an exact result for an uncertain input, or a missing witness would fail.

The limit attack uses LONG_MIN, LONG_MAX, -2^60-1, 2^60, 2^60+1, input valuations near both
exponent bounds, centred exponents near 2^59 and 2^60, exact zero, disjoint inputs, overlapping
inputs, and working-power limits. It includes uncapped E>2^60 with valid capped K.
No huge power is formed in these boundary cases. Returning a status against the header order,
refusing a specified no-power shortcut, changing output on failure, or using E instead of K would fail.

The large attack has exact rational points and noncentred balls at p=2,3,5,7, N=2000.
Its ball centres also have thousands of bits. It compares all four residues at absolute precision 2000.
These are centre checks; whole-ball enclosure is separately attacked by the small-ball enumeration.

The regression attack has 336 independently chosen inputs times three functions: exp, log, Log.
It covers the seven primes, signed rational inputs, negative valuations, exact inputs and balls,
and N from -5 through 27. It compares statuses and every output field, with both alias modes.
Separately, the 24519 bytes and 686 lines extracted from 27f7e5f equal the current lfunc.c prefix exactly.

The real attack has 336 finite results, 24 lost-finiteness statuses, 180 precision-limit statuses,
360 unsupported complex-place calls, 360 absent-place calls, and four exact-zero calls at
ADF_REAL_PREC_MAX. It includes LONG_MIN clamping, precisions 0,2,17,80,257, MAX+1, signed large
arguments, and intervals crossing zero. It demands arb's complete output at the same working precision.
Any different radius, changed failure output, wrong where, retained extra component, or wrong priority fails.

Driver inputs include unit cosets, ideles, classes, complex adeles, and local text lacking a typed parser.
They also include malformed or composite places, extra places, a finite ball requested at real, exact 2
at 2, and 2 Z_2. All 116 hostile commands produce their expected error. The other 224 output lines
equal the corresponding local-library values, including canonical centres and exponents.

referee.md records the F10-F14 review, including the count, divisibility, contraction, hull,
exponent-arithmetic and arb-transfer arguments. The F13 wording is the only proof-text finding.
Read the new C tests, Julia example, lane oracle, and fault-insertion script. No identically true
assertion was identified. The hyperbolic identity on uncertain balls only checks compatibility with
a wider enclosure; the separate point-oracle checks carry its enclosure evidence.

## Commands and results

All commands ran from the repository root. B means lanes/f-review5/build below.
Every test program and script had a timeout of at most 60 seconds. At most two processes performed
substantial computation concurrently. No repository test suite was run.

The archive was built once:

    timeout 60 make -j2 BUILD=lanes/f-review5/build > lanes/f-review5/build.log 2>&1

Exit 0; one archive. No subsequent make invocation.

Old-source extraction and compilation:

    timeout 10 git show 27f7e5f:src/lfunc.c > lanes/f-review5/old_lfunc.c
    timeout 30 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude -Isrc \
      -Dadf_lball_exp=old_exp -Dadf_lball_log=old_log -Dadf_lball_Log=old_Log \
      -c lanes/f-review5/old_lfunc.c -o lanes/f-review5/build/old_lfunc.o

Both exit 0. The only other git use was the read-only comparison:

    timeout 10 git diff 27f7e5f -- src/lfunc.c src/rfunc.c \
      include/adelefeld/lfunc.h include/adelefeld/rfunc.h

Exit 0. No git command changed state. No bd command was run.

Probe compilation:

    timeout 30 cc -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
      -Iinclude lanes/f-review5/probe.c lanes/f-review5/build/old_lfunc.o \
      lanes/f-review5/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review5/build/probe

Exit 0.

Each oracle attack was run as follows, with MODE replaced by the table entry:

    timeout 60 python3 -B lanes/f-review5/attack.py MODE > lanes/f-review5/MODE.log 2>&1

| MODE | Runs | Exits | Result |
|---|---:|---|---|
| local | 2 | 124, 0 | First: C 2016/34274/0, oracle timed out; second: 2016 cases, 0 failures |
| enumerate | 1 | 0 | 252 cases, 2088 point checks, 116 witnesses, 0 failures |
| limits | 1 | 0 | 2548 cases, 0 failures |
| large | 1 | 0 | 32 cases, 0 failures |
| regression | 1 | 0 | 1008 cases, 0 differences |

attack.py runs B/probe under timeout 50 with ASAN_OPTIONS=detect_leaks=0.
It sets OMP_NUM_THREADS=1 and OPENBLAS_NUM_THREADS=1 for that subprocess.
The first local log was overwritten by the completed rerun; its timeout is recorded in progress.md.

Real probe compilation and execution:

    timeout 30 cc -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
      -Iinclude lanes/f-review5/real_probe.c lanes/f-review5/build/libadelefeld.a \
      -lflint -lgmp -lm -o lanes/f-review5/build/real_probe
    ASAN_OPTIONS=detect_leaks=0 timeout 45 lanes/f-review5/build/real_probe \
      > lanes/f-review5/real.log 2>&1

Both exit 0. Result: 1264 rows, 5387 assertions, 0 failures.

Driver compilation and execution:

    timeout 30 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude tools/adf/adf.c \
      lanes/f-review5/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review5/build/adf
    timeout 60 python3 -B lanes/f-review5/driver_attack.py > lanes/f-review5/driver.log 2>&1

Both exit 0. The script runs B/adf under timeout 40. That process exits 1 because error commands
are present. All 340 output lines match; 0 mismatches.

Cost reproducer compilation and its single run:

    timeout 30 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude lanes/f-review5/cost_probe.c \
      lanes/f-review5/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review5/build/cost_probe
    timeout 45 lanes/f-review5/build/cost_probe > lanes/f-review5/cost.log 2>&1

Both exit 0. Four equal residues; times and counts are in R2. No further timing run.

F13 reproducer:

    ASAN_OPTIONS=detect_leaks=0 timeout 10 lanes/f-review5/build/probe \
      < lanes/f-review5/f13.in > lanes/f-review5/f13.out 2> lanes/f-review5/f13.stderr

Exit 0. Eight rows, 138 assertions, 0 harness failures; four DOMAIN and four NOT_DETERMINED.

Final evidence audit:

    timeout 10 python3 -B lanes/f-review5/audit.py > lanes/f-review5/audit.log 2>&1

Exit 0. Old-source prefix equality: 1. Stream input/output rows: 5856/5856.
Seven sanitizer logs: 0 AddressSanitizer or runtime-error diagnostics.
Eight authored source/note files: 0 lines above 116 characters.
The final report text was also checked for the 116-character limit before its single write.

## Files written

All writes are inside lanes/f-review5.

- Sources and notes: attack.py, probe.c, real_probe.c, driver_attack.py, cost_probe.c, audit.py,
  referee.md, progress.md, old_lfunc.c, and this report.md.
- Stream evidence for each MODE in local, enumerate, limits, large, regression:
  MODE.in, MODE.out, MODE.stderr, MODE.log, MODE.failures.json.
- Finding evidence: f13.in, f13.out, f13.stderr, cost.log.
- Other evidence: build.log, real.log, driver.cmd, driver.expected, driver.out, driver.stderr,
  driver.log, audit.log.
- Build artifacts under build/: the archive and its object/dependency files, old_lfunc.o,
  probe, real_probe, adf, cost_probe.

The existing brief.md and lane.log were not edited.

## What is not done

No repository suite, stored-fixture suite, repository oracle, Julia program, or mutation campaign was run.
No Hensel-exp identity oracle was added; the independent rational series are the value oracle.
No exhaustive search over large balls or all input bit patterns was attempted.
The archive and old object are ordinary builds. Only probe.c and real_probe.c were compiled with
ASan/UBSan. Zero diagnostics does not claim that the library was fully instrumented.
LeakSanitizer was disabled. Performance evidence is confined to the single measurement in R2.
No fix was applied, including to the incorrect F13 sentence.

## Sources pending

None for the arguments used here. The series are the task's definitions; the rational tail,
pairing, domain counterexample, and hull arguments are written out.
The external arb enclosure convention and hyperbolic calls were read at
refs/src/flint-3.0.1/arb.rst:6-12 and 1209-1219, respectively.

## Findings against the specification

None. R1 concerns docs/api-1f4.md wording, and R2 concerns avoidable computation.
Neither requires a change to docs/SPEC.md.

