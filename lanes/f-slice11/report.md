# f-slice11 report

Both slices are implemented. Slice A was recorded finished before slice B started.
Six functions, the driver commands, the Julia example, selected fixtures and G7-G12 are written.
No git command, bd, package installation, subagent or edit of docs/SPEC.md was used.
No source helper was copied from src/gfunc.c. That source is unchanged by this lane.

## Functions and decisions

- adf_idele_Log and adf_idele_log_abs return an adele with finite triple (0,4,1), even for exact input.
- adf_idele_Log_at returns one exact local image enclosed at min(N,E), or at N for exact input.
  Only the proved local zero retains exactness. Stored (c,M)=(1,2) gives E=2 at 2.
- adf_idele_log_abs_at is real-only; a prime gives UNSUPPORTED with that prime.
- adf_idele_Log_refine and adf_idele_log_abs_refine retain the all-places baseline and use integer CRT.
  Exact local zero is rounded to N. Empty lists give the conservative result.

The five choices in the brief are followed, to be recorded by the orchestrator as N-D17.
No new public projection function is added. The descriptor is private and uses named-prime removals.
The compact centre uses p^K, without forming an input ball at m+k or a power p^k.
The local evaluator checks its actual working precision W. G8-G9 justify the arbitrary higher lift.

Decisions where the design is silent:

1. Known local exponent/compact-power refusals precede aggregate CRT refusal. Earlier primes are
   evaluated before reporting a later known LIMIT, to preserve canonical ties with working-power LIMIT.
   Aggregate refusal otherwise precedes actual local sums, leaving where untouched. The alternative
   evaluates all centres before refusing their combined modulus. G11-G12 and the header state the choice.
2. The driver uses space-separated primes, following project, and none for an empty list.
   Comma-separated primes would add another grammar. prec supplies N and real bits separately in C.
3. logabs and logabs_refine are aliases for log_abs and log_abs_refine. New commands accept idele operands.
   Existing lower-case log_at keeps its series meaning. Log_at on these commands uses the idele API.
4. Snapshot tests define padding and inactive small-arf slots before copying struct bytes.
   The alternative of dropping byte comparisons would lose part of the requested preservation check.
   Every original byte comparison and value/status assertion remains. FLINT cleanup runs once at exit.

## Proof and test scope

G7-G12 in docs/api-1f8.md prove the wrapper, descriptor, compact-centre congruence, limits,
rounded intersections, CRT and failure precedence. They refer to IL1-IL8 and the earlier local proofs.
No counterexample to the mathematical design or SPEC was found.

Final fixtures: 6784 local rows / 874168 bytes, 6 real rows / 1543 bytes,
280 CRT rows / 37391 bytes. Total 913102 bytes, below 1000000.
proto/idlog_checks.py is unchanged. Lane writers import its integer oracle.
The full 21880-row fixture is neither written nor added by this lane.

The local selection covers restricted odd primes, 2 with k>=2, k=0 and stored k=1,
unrestricted primes with c possibly divisible by p, both exact signs, positive/negative content
valuations, N<E, N=E and N>E, and all eight verbatim design fault witnesses.
All selected rows are read. Any wrong p, exact flag, u, v, N, status or where makes a case fail.

Twenty whole image comparisons use H=H'=5 and enumerate 20814 admissible input units.
The C result is compared in both membership directions at 101035 residues.
A missing or extra residue, or an incorrect exponent, fails. These are finite quotient checks;
IL1-IL7 prove the infinite set statements.

Real tests use six intervals and five precisions: 120 wrapper calls. Their balls must be identical
to rfunc.h's results and contain endpoint values from mpmath at 800 bits, rounded to 500-bit dyadics.
This is an independent numerical endpoint check; it is not a new certified real analytic proof.
Real coordinate and content differ in the tests. Both signs and real zero results are included.

At 65537 and 2^64-59 there is no residue enumeration. Restricted-ball comparisons and additivity
use N=3; Log(1+p)=p mod p^2 is checked at N=2. Tests also cover nonprincipal units 2,3,6,
exact signs, inverses and unrestricted c divisible by p. The local comparison uses adf_lball_Log.
A 4097-bit numerator and 3005-bit denominator test compares the compact lift with the exact centre at N=6.

CRT tests use all 280 rows, 525 named projections, the unlisted factor at 11, shuffled lists and
73056 two-period memberships when R<=4096. Larger moduli have triple/projection checks.
They fail on any different triple, factor, membership or real ball. The named witness is (12,36,1).

Status tests include shape errors, length ceiling, exact-zero rounding, compact/working/aggregate
limits, precision ceiling, canonical repetitions, real/prime precedence and earlier working/later
compact ties. Failure compares fields and struct bytes; NULL where is exercised. Valid ideles are used.
No value alias is allowed between these different types. Repeated calls reuse the same outputs.
NOT_DETERMINED is tested by an injected real evaluator failure: six calls, zero failed checks.

## Files written

- src/gfunc_log.c: new implementation. include/adelefeld/gfunc.h: appended blocks only.
- tests/test_gfunc_log.c and tests/julia/gfunc_log.jl: new tests and callable examples.
- tests/ref/vectors/f-slice11/{local,real,crt}.jsonl: selected oracle data.
- docs/api-1f8.md: appended Log section, G7-G12 and implementation decisions.
- tools/adf/adf.c and tools/adf/README.md: Log, logabs/log_abs, Log_at, log_abs_at,
  Log_refine and logabs_refine/log_abs_refine.
- tests/driver/gfunc-log-values.cmd/.out and gfunc-log-status.cmd/.out.
- tests/driver/gfunc-log-refine-values.cmd/.out and gfunc-log-refine-status.cmd/.out.
  Expected lines were written by hand before the driver runs.
- tests/test_julia.sh: two lines, one for each existing loader path.
- lanes/f-slice11/: progress, writers, fault script, injected probe, artifact check, logs and this report.
  Build trees and scratch implementations are confined to the lane until the requested final checks.

## Commands and results

All test programs and scripts ran under timeout. Builds used at most two jobs.
For the table, L denotes lanes/f-slice11 and B denotes lanes/f-slice11/build.
These abbreviations are paths, not different commands. Stage logs retain their earlier test versions.

| Command | Exit and numerical result |
|---|---|
| timeout 120 python3 -B L/write_selection.py | 0; initially 6776 local rows, finally 6784 |
| timeout 120 python3 -B L/write_crt.py | 0; 280 CRT rows, final total 913102 bytes |
| timeout 10 python3 -B proto/idlog_checks.py --expect 5 4 1 1 0 3 | 0; centre 45, exponent 3 |
| timeout 180 make -j2 BUILD=B B/test_gfunc_log, first red | 2; undefined adf_idele_Log |
| timeout 120 B/test_gfunc_log, first green | 0; 1 test, 7 checks, 0 failures |
| timeout 120 B/test_gfunc_log, A assertion red | 1; 2 tests, 12 checks, 3 failures |
| timeout 120 B/test_gfunc_log, A green | 0; 6 tests, 318552 checks, 0 failures |
| timeout 120 B/test_gfunc_log, expanded A | 0; 8 tests, 318570 checks, 0 failures |
| timeout 120 B/test_gfunc_log, B assertion red | 1; 9 tests, 318577 checks, 4 failures |
| timeout 120 B/test_gfunc_log, first B green | 0; 11 tests, 2193775 checks, 0 failures |
| timeout 120 B/test_gfunc_log, canonical tie green | 0; 11 tests, 2193783 checks, 0 failures |
| timeout 120 B/test_gfunc_log, expanded B | 0; 12 tests, 2193803 checks, 0 failures |
| timeout 120 B/test_gfunc_log, final harness | 0; 12 tests, 2194049 checks, 0 failures |
| timeout 180 python3 -B L/faults.py | 0; 12 compiled, 12 rejected, all exit 1 |
| timeout 180 python3 -B L/faults.py B | 0; 12 compiled, 12 rejected, all exit 1 |
| timeout 120 B/ceiling_mutant | 1; 3 failed checks after the ceiling test was added |
| timeout 120 B/exponent_mutant | 1; 1 failed where check after the B test was added |
| timeout 30 B/injected, A | 0; 4 calls, 0 failed checks |
| timeout 30 B/injected, A+B | 0; 6 calls, 0 failed checks |
| timeout 10 python3 -B L/artifact_check.py | 0; 0 checked lines over 116, 913102 fixture bytes |

The first compile used undefined symbols. Subsequent function reds used temporary UNSUPPORTED stubs.
redgreen.log records every function red and the green runs. Later builds of B/test_gfunc_log used the
same make command above, some with INV=1; every such build succeeded. No full INV build is claimed:
the incremental lane builds reused objects first compiled with INV=0. The dedicated mutation judge
compiled the new source and test with ADF_CHECK_INVARIANTS against a sanitized INV=0 library.

Driver build command, exit 0 after each slice:

```text
timeout 120 cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
  tools/adf/adf.c lanes/f-slice11/build/libadelefeld.a -lflint -lgmp -lm \
  -o lanes/f-slice11/build/adf
```

Each command below used timeout 60, then diff -u against its hand-written .out (all four diffs exit 0):

| Command after timeout 60 | Exit; expected output lines |
|---|---|
| B/adf tests/driver/gfunc-log-values.cmd | 0; 12 |
| B/adf tests/driver/gfunc-log-status.cmd | 1; 7 |
| B/adf tests/driver/gfunc-log-refine-values.cmd | 0; 11 |
| B/adf tests/driver/gfunc-log-refine-status.cmd | 1; 7 |

Shared builds after A and B, both exit 0:

```text
timeout 180 cc -shared -fPIC -std=c11 -O2 -Iinclude src/*.c \
  -lflint -lgmp -lm -o lanes/f-slice11/build/libadelefeld.so
```

`timeout 60 julia --startup-file=no tests/julia/gfunc_log.jl B/libadelefeld.so` first exited 1:
Julia's bundled GMP lacks __gmpn_modexact_1_odd. The existing repository workaround was then used:

```text
LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 timeout 60 julia --startup-file=no \
  tests/julia/gfunc_log.jl lanes/f-slice11/build/libadelefeld.so
```

A: exit 0, 59/59 checks. A+B: exit 0, 59/59 and 22/22 checks. The final acceptance run used
the same existing loader workaround automatically. No Julia or GMP installation was changed.

Fault, injected and survivor programs were compiled under timeout 120 with cc -Iinclude -Isrc,
the lane archive and -lflint -lgmp -lm. Fault/survivor programs also link the two test support
objects and compile tests/test_gfunc_log.c; injected programs compile L/injected.c instead.
All those compiles exited 0. faults.py uses -DADF_CHECK_INVARIANTS, -O1 -g and -pthread,
and runs each scratch binary under timeout 90 with RLIMIT_AS=4 GiB.

## Planted fault table

All entries compiled. Every planted program exited 1 with failed assertions. A and B each plant
the eight finite faults; A adds O1-O4 and B adds B1-B4. Logs give the exact failing lines.

| Fault | Test that rejects it |
|---|---|
| F1 use m+k as image exponent | local_selection: canonical exponent/fields |
| F2 omit input m and then subtract it | local_selection: canonical exponent/fields |
| F3 drop Log of the content | local_selection: translated centre |
| F4 unrestricted 2 gives 2 Z_2 | local_selection: exponent and complete image |
| F5 unrestricted odd gives Z_p | local_selection: exponent and complete image |
| F6 ignore requested cap | local_selection: requested exponent |
| F7 stored k=1 uses ordinary radius | local_selection: stored (1,2) image |
| F8 exact unit becomes unrestricted | local_selection: exact-zero tag |
| O1 real Log silently uses absolute value | real_selection: negative real DOMAIN |
| O2 real coordinate comes from content | conservative_Log: real log(1)=0 |
| O3 conservative factor 4 becomes 2 | conservative_Log: finite triple |
| O4 success writes where | local_selection: success sentinel |
| B1 CRT baseline factor 4 becomes 2 | refinement_witness: (12,36,1) |
| B2 exact zero is not rounded | refinement_selection: exact-input CRT triple |
| B3 lower-priority real failure wins | refinement_status_and_limits: 2-adic working LIMIT |
| B4 CRT centre always zero | refinement_witness: centre 12 |

## Automatic mutation testing

The tool's last report paragraph was read: --timeout bounds the baseline as well.
Only this lane's test is the judge. No tool or equivalent.txt repair was made.
Scratch roots are snapshots under B, with mutation scratch directories outside those snapshots.
Copying the live lanes into scratch inside live lanes would recurse: copy_tree ignores only pycache.

The first full SAN=1 INV=1 cache build exited 2 at src/text.c:2623 and :2943 (stray #x).
The first mutation command also exited 2: zero judged mutants. The SAN=1 INV=0 cache build exited 0.
The next invariant-source sanitizer baseline passed 318563 assertions, then LSan failed under ptrace;
mutation exited 2 with zero judged mutants. A root-options LSan probe also exited 1 with that error.

Actual judged runs use ASAN_OPTIONS=detect_leaks=0. Address and undefined-behaviour instrumentation
remain enabled. The new source and test have invariant checks. This is not a leak-enabled mutation run.
Two tool jobs each use one make job, keeping at most two build cores. Both runs had timeout 1300.

Common tool options:

```text
--files src/gfunc_log.c --limit 30 --seed 1 --jobs 2 --san --timeout 120
--copy Makefile include src tests lanes
```

A3 root: B/mutation-root-A; scratch: B/mutations-A. B root: B/mutation-root-B; scratch: B/mutations-B.
The --make judge builds with BUILD=lanes/f-slice11/mut-build SAN=1 INV=0, then compiles
src/gfunc_log.c and tests/test_gfunc_log.c separately with -DADF_CHECK_INVARIANTS,
-fsanitize=address,undefined, -fno-omit-frame-pointer, the sanitized archive and support objects,
then runs the debug test under timeout 90. B adds ulimit -v 60000000000 for ASan's large virtual range.
The exact commands and compiler outcomes are in mutate-A*.log, mutate-B.log and the lane tool history.

| Run | Selected | Compiled | Killed | Survived | Not compiled | Timed out | Seconds |
|---|---:|---:|---:|---:|---:|---:|---:|
| A3 | 30 | 23 | 19 | 4 | 7 | 0 | 31.9 |
| B | 30 | 20 | 19 | 1 | 10 | 0 | 266.6 |

60 mutants were judged; 43 compiled. Both tool exits are 1 because survivors remain in the logs.
Noncompiled mutants are not kills. No sweep was repeated. Two gap survivors were rebuilt separately
and rejected after new tests; three survivor occurrences have direct value-preserving reasons below.

Every survivor, with its original stage line:

- A :39, prec > max to >=: a test gap; real_precision_ceiling now rejects it with 3 failed checks.
- A :81, K <= EXP_MAX to <: a test gap; B's aggregate/place boundary now rejects it with 1 failed check.
- A :97, m=a-b to a+b: coprimality makes a or b zero; only |m| is used publicly, so it is unchanged.
- A :133, k==0 to k==1: both branches are already covered by K<=d for these cases; result is unchanged.
- B :226, max_exp uses >= instead of >: when the arguments are equal, either branch returns that value.

## Final acceptance and memory checks

`timeout 900 make -j2 check-all` ran exactly once in build/, exit 0.
77 C programs passed. Driver: 55 cases, 101019 expected lines, all equal.
Exports: 442/442, zero missing, undeclared or variadic. Julia passed, including 81 new checks.
Both tool self-tests passed. Last line:

```text
check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest
```

`timeout 180 make -j2 BUILD=build/san SAN=1 build/san/test_gfunc_log`: exit 0.
Its last line is the cc link command for build/san/test_gfunc_log with address/undefined sanitizers.
`ASAN_OPTIONS=detect_leaks=1 timeout 120 build/san/test_gfunc_log`: exit 1.
It first reported 12 tests, 2194049 checks, 0 failed checks, 0 failed tests. Its last line:

```text
==121==HINT: LeakSanitizer does not work under ptrace (strace, gdb, etc)
```

`timeout 10 valgrind --version`: exit 0, valgrind-3.22.0.
Three supplemental runs used timeout 180 valgrind --leak-check=full --show-leak-kinds=all
--error-exitcode=99, first on build/test_gfunc_log and then twice on B/test_gfunc_log.
No suppression file was used.

| Stage | Exit | Errors / contexts | Heap bytes / blocks at exit |
|---|---:|---:|---:|
| Original snapshots, no cache cleanup | 99 | 100 / 57 | 577640 / 4072 |
| Struct padding defined and FLINT cleanup | 99 | 36 / 9 | 0 / 0 |
| Inactive arf slots defined too | 0 | 0 / 0 | 0 / 0 |

The final run has 613570 allocations and 613570 frees (460557435 bytes allocated over the run).
It reports 12 tests, 2194049 checks, zero failed checks. Last line:

```text
==14== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

The supplemental runs exposed and repaired a test harness defect, not an implementation defect.
Full byte comparisons read unused storage until it was initialized. Cache cleanup is prescribed by
refs/src/flint-3.0.1/memory.rst:26-43; small-arf storage is documented at arf.rst:43-54.
Only the harness changed after check-all and the required sanitizer build. The library source is
unchanged. The final harness was rebuilt and checked under Valgrind. No second full acceptance
or sanitizer build was run, respecting the brief's one-run constraint.

## Findings against the specification, design or brief

No mathematical finding against SPEC or IL1-IL8 was found. The two draft findings of d-idlog are
implemented: the unrestricted 2-adic shell is an additive ball; stored k=1 is valid and needs handling.

The design leaves aggregate-versus-local work-limit ordering unspecified. The concrete order and its
canonical-tie qualification are stated in G11-G12 and the header. This is an implementation decision.

Full INV=1 verification is blocked by read-only src/text.c's two stray #x tokens. Input-independent
reproducer: the SAN=1 INV=1 test build above. That file was not repaired here.
LeakSanitizer is blocked by the execution environment's ptrace restriction. The required run was
attempted and its nonzero exit retained. It is not reported as a passing sanitizer leak scan.
The later harness correction means the one full check-all used the preceding harness version;
all library code and all functional assertions were the same. Targeted final Valgrind checks cover the repair.

## What is not done and sources pending

No part of slice B is omitted. No full INV=1 run or successful LeakSanitizer scan is claimed.
No long differential fuzz run was made. No enumeration at the two large primes was made.
An idele content valuation beyond 2^60 cannot be constructed within the laptop bounds; that
input-limit branch has a proof and source check, but no feasible valid-input empirical witness.
The finite-unit positive exponent boundary likewise needs an infeasible huge modulus in slice A;
the exact-zero refinement boundary is exercised in slice B without forming that modulus.

Pending sources inherited from the design: the conventional name/normalisation of Iwasawa Log,
the name Teichmueller, and the real analytic facts of functions.md Lemma 2.
No new foreign formula was assumed without a source. Finite identities use the defined series
and stepwise repository proofs; no nonzero-rational-value theorem is claimed or needed.

Avoidable costs: descriptor removals repeat in preflight/evaluation; the real wrapper copies balls;
local evaluations and CRT recombinations are independent sequential steps. No optimisation was made.
