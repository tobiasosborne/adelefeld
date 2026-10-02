# f-slice7 report

Implementation is complete. Full acceptance is not complete. Both requested full runs hit the
180-second bound during test_resid_rest. Four read-only legacy test files need the prepared patch
legacy-tests.patch. The ownership question received no answer, so that patch is not applied.
The new local and partial-ball tests pass, including ASan/UBSan. No finding against SPEC was found.

## Functions and implementation

Built adf_lball_sin, adf_lball_cos, adf_lball_sinh, adf_lball_cosh, and their four adf_sball _at forms.
The prime implementations use their own factorial series on fmpz. No FLINT padic function is called.
The real sinh_at and cosh_at use arb_sinh and arb_cosh, including exp_at's precision and finiteness rules.
Added the four driver commands and a Julia example computing sin(5), cos(5), sinh(5) at 5 to N=20,
cosh(4) at 2 to N=20, and DOMAIN for sin(1) at 5.

The added evaluator uses a common denominator L!, signed parity coefficients and modular Horner evaluation.
The old exp, log and Log evaluators and their helpers were not changed or factored out.
All failures preserve the output, including y=x. Exact zero alone gives an exact constant value.
Input bounds precede the whole-ball domain test. The requested precision is absolute.

## Decisions and alternatives

1. Cos/cosh use E=2M-v_p(2) on a centred ball p^M Z_p and E=M on every noncentred ball.
   Alternative: retain safe E=M everywhere. The chosen centred rule uses Proposition 10's proved
   smallest hull and is tested separately from the safe noncentred rule. Both use K=min(N,E).
2. The new evaluator traverses every degree, including zero coefficients of the other parity.
   Alternative: Horner in x^2, or refactor exp into a general evaluator. The chosen form has the
   direct numerator-polynomial proof F11 and leaves the reviewed old functions unchanged.
   It has an avoidable cost: visiting the absent parity doubles some modular Horner work.
   There was no benchmark and no optimization claim.

For the orchestrator's SPEC 15.4 record, decision 1 is the new numerical policy. The existing absolute-N,
exact-zero, domain-status, and limit policies are continued, not replaced. No SPEC file was edited.

## What is proved

New statements F10-F14 at the end of docs/api-1f4.md:

- F10: the odd/even term counts, already present in Proposition 7, and the infinite-tail bound from Lemma 5.
- F11: the signed common-denominator Horner invariant; divisibility by p^D; W=K+D working precision;
  correct reduction after division by the p part and inversion of the unit part of L!.
- F12: enclosure of every point of every admitted ball; K=min(N,E); the centred cosine hull;
  exact-zero and constant-centre shortcuts. It also proves that sin/sinh map each domain ball onto
  their stated image ball, by a contraction iteration for f(t)=t+h(t).
- F13: exponent arithmetic, intermediate-power limits, full-word prime counts, output transactions and aliasing.
- F14: transfer to one component of a partial ball, real arb enclosures and the inherited loss/status rule.

No optimal noncentred cosine hull is claimed or proved. No irrationality assertion for other exact inputs
is used. Finite enumeration and identity checks are evidence for the implementation, not substitutes for proofs.

## Numerical checks and sensitivity

The oracle is proto/lfunc_trig_checks.py. It uses Python int and Fraction, no FLINT and no C evaluator.
It sums exact rational series with a tail bound three digits beyond requested N, then reduces the final sum.
Fixtures: 4064 cases, 503 point rows, 8 Hensel roots; total 592240 bytes, below 1 MB.
The cases include 1200 seeded random inputs, rational denominators, negative centres, 1000-bit operands,
and p=2^64-59 at N=1,2,8,20,80,200. Every case is run with distinct and aliased outputs, locally and through _at.

Enumeration covers all domain balls c<=M<=c+2 at p=2,3,5,7, with every integer representative modulo
p^(c+3). Point values have explicit output precision H=2(c+2)+1-v_p(2), at least E+1.
There are 1296 ball/request calls, 18108 point-containment checks, and 480 smallest-hull witnesses.
A wrong residue, exact flag, or promised exponent fails a case. A claimed smallest hull also fails
unless two oracle image values differ modulo p^(E+1).

Independent truncations check cos(4)=9 mod 16 at 2 at N=4, sin(3)=3 mod 9 at 3 at N=2,
and v_3(cos(3)-1)=2 at N=5. The tests distinguish exact zero from uncertain zero, centred cosine gain
from safe noncentred precision, N below and above E, DOMAIN from NOT_DETERMINED, and every LIMIT route.
They test unchanged outputs on failures, including aliasing, and no-power shortcuts at exponent 2^60.

Hyperbolic identities use exp at N+v_p(2), then divide by 2. Exact-input RHS balls must be contained
in the N-digit result. On ball inputs, interval arithmetic can be wider because it loses dependence;
that containment direction is explicitly reversed. Circular identities use exp(+-i*x) at p=5,13.
The reference lifts i by Hensel to H=N+2; F12's Lipschitz estimate bounds replacement of the true root.
All identity checks also demand the stated exponent or exactness. The independent series fixtures remain primary.

The _at checks demand the correct single component, unchanged where on OK, the prime on failure,
and unchanged output on failure. Real hyperbolic checks compare arb's complete ball at the same precision
and three image points evaluated at 250 bits. Input 2^1000 must produce NOT_DETERMINED without output.

Five faults were inserted only into scratch copies under lanes/f-slice7/build/faults.
The unmodified local test file rejected all five; each test process exited 1.

| Fault | Failed checks | Failed tests |
|---|---:|---:|
| Remove one retained parity term when L>=3 | 857 | 4 |
| Use input M without requested-N cap or centred gain | 2412 | 3 |
| Claim exact constant for non-exact zero | 9186 | 3 |
| Remove alternating signs | 1680 | 3 |
| Lose one working digit when D>0 | 910 | 5 |

No mutation-tool run, fuzz target, benchmark, clang suite or INV suite was run.
LeakSanitizer was disabled as requested; no claim about a LeakSanitizer check is made.

## Commands and results

Commands ran from the repository root. B below denotes lanes/f-slice7/build.
Every test/script invocation was bounded by timeout. Builds used at most two jobs.
Logs named below are in lanes/f-slice7. Red and green chronology is also in redgreen.log.

1. `timeout 180 python3 -B proto/lfunc_trig_checks.py --generate`: exit 0.
   3 independent truncations, 462 reference identity checks, 0 failures; fixture counts above. oracle.log.
2. `timeout 180 make -j2 BUILD=lanes/f-slice7/build lanes/f-slice7/build/test_lfunc_trig`:
   initial exit 2, undefined references to the four new functions. red-link.log.
3. Scratch red executables were linked with timeout 180 cc, C11/O2, -Wall -Wextra -Werror,
   -Iinclude -Itests, the test file, B/red-stubs.c, B/support/jsonl.o, B/libadelefeld.a,
   -lflint -lgmp -lm. Both compilations exited 0.
   `timeout 180 lanes/f-slice7/build/red-trig`: exit 1; 6 tests, 168374 checks,
   11333 failed checks, 6 failed tests. red-assertions.log.
   `timeout 180 lanes/f-slice7/build/red-at`: exit 1; 12 tests, 279045 checks,
   11452 failed checks, 3 failed tests. red-at.log.
4. `timeout 180 python3 -B lanes/f-slice7/driver_vectors.py`: exit 0, 3 golden files, 96 expected lines.
   The old driver was compiled with timeout 180 cc, C11/O2, -Wall -Wextra -Werror, -Iinclude,
   tools/adf/adf.c, B/libadelefeld.a, -lflint -lgmp -lm, -o B/adf-red: exit 0.
   `timeout 180 lanes/f-slice7/build/adf-red < tests/driver/trig-prime.cmd`: exit 1;
   all 56 function commands gave PARSE. Comparison exited 1, 115-line diff. red-driver.diff.
5. Incremental builds with `timeout 180 make -j2 BUILD=lanes/f-slice7/build` and the targets
   B/test_lfunc_trig, then B/test_rfunc_prime and B/test_lfunc: both exited 0.
   `timeout 180 lanes/f-slice7/build/test_lfunc_trig`: exit 0;
   6 tests, 2173970 checks, 0 failures. green-trig.log.
   `timeout 180 lanes/f-slice7/build/test_rfunc_prime`: exit 0;
   12 tests, 279015 checks, 0 failures. green-at.log.
   `timeout 180 lanes/f-slice7/build/test_lfunc`: exit 0;
   12 tests, 454429 checks, 0 failures, including stored old-code fixtures. regression-lfunc.log.
6. The new driver was compiled by the command of item 4 with output B/adf: exit 0.
   A `timeout 180 sh -c` loop ran `timeout 30 B/adf` on every tests/driver/trig-*.cmd,
   required exit 1 from each as specified, and diffed its .out: loop exit 0, all 96 lines equal.
   The exact loop is in the tool history; no expected line came from driver output. green-driver.log.
7. `timeout 180 python3 -B lanes/f-slice7/faults.py`: exit 0, five fault processes exited 1.
   Each scratch compile had timeout 45, each test timeout 30. Counts are in the fault table. faults.log.
8. `timeout 180 cc -std=c11 -O2 -fPIC -shared -Iinclude src/*.c -lflint -lgmp -lm`
   with `-o lanes/f-slice7/build/libadelefeld.so`: exit 0.
   `timeout 60 julia --startup-file=no tests/julia/lfunc_trig.jl B/libadelefeld.so`:
   exit 1 before tests, the documented __gmpn_modexact_1_odd loader error. julia.log.
   First retry with LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10: exit 1, a new Julia syntax error
   (`k div 2`). Corrected it to div(k,2). julia-preload.log.
   Same command with that preload and JULIA_NUM_THREADS=1 after correction: exit 0, 25/25 checks.
   julia-green.log. Printed sin(5)=91977224184255 and cos(5)=15188663525926 modulo 5^20;
   cosh(4)=954473 modulo 2^20; sin(1) at 5 gave DOMAIN.
9. `timeout 30 python3 -B lanes/f-slice7/prepare_legacy_patch.py`: exit 0, four-file patch prepared.
   It writes only the lane patch and scratch copies, never the read-only originals.
   Scratch C compile with timeout 180 cc, the flags/support/archive of item 3 and
   B/legacy/tests/test_rfunc.c instead of red stubs: exit 0.
   `timeout 60 lanes/f-slice7/build/legacy/test_rfunc`: exit 0;
   10 tests, 59700 checks, 0 failures. legacy-c.log.
   Julia scratch copies B/legacy/tests/julia/sball.jl and f_at.jl, each with the preload,
   JULIA_NUM_THREADS=1, timeout 60, --startup-file=no and B/libadelefeld.so:
   both exit 0, respectively 36/36 and 23/23 checks. legacy-julia-sball.log, legacy-julia-f-at.log.
   `timeout 60 B/adf < B/legacy/tests/driver/f-places-hostile.cmd`: expected exit 1;
   diff against the existing .out exited 0. legacy-driver.out.
10. New-file line-length audits used `timeout 30 python3 -` with Path.read_text and len(line)>116.
    Two new overlong lines were split. The last audit found 0 overlong lines in the new oracle,
    local test, Julia test, new lfunc implementation, API statements and declaration blocks.
    Existing long lines in rfunc.c were left alone. Log audits found 0 failure lines and 0 sanitizer
    diagnostic lines in each bounded full-run log. Fixture byte audit returned 592240.
11. The requested clean acceptance command, run ONCE with thread environment caps of 1:

        timeout 180 sh -c 'make clean && make -j2 check-all'

    Exit 124 during test_resid_rest; 47 test-program summaries completed with 0 failures.
    Includes local trig 6/2173970/0 and old local functions 12/454429/0 (tests/checks/failures).
    test_dlopen's export check reported 413/413 declared functions exported, 0 missing.
    Final last line of final-check-all.log:

        make[1]: *** [Makefile:102: check] Terminated

12. `timeout 180 python3 -B lanes/f-slice7/build_san.py`: exit 0, sanitizer binaries built once
    into build/san. Its make uses -j2 and timeout 175. This separates compilation from the test budget.
    The requested sanitizer acceptance command, run ONCE:

        ASAN_OPTIONS=detect_leaks=0 timeout 180 make -j2 check SAN=1 BUILD=build/san

    Exit 124 during test_resid_rest; 47 summaries completed with 0 failures and 0 sanitizer diagnostics.
    Includes local trig 6/2173970/0 and old local functions 12/454429/0.
    Final last line of final-san.log:

        make: *** [Makefile:102: check] Terminated

    Since the new wrapper suite lies after that timeout, ran it separately:

        ASAN_OPTIONS=detect_leaks=0 timeout 60 build/san/test_rfunc_prime

    Exit 0. Last line of final-san-prime.log:

        12 tests, 279015 checks, 0 failed checks, 0 failed tests

13. `timeout 60 build/test_rfunc`: exit 1, 10 tests, 59700 checks, 6 failed checks,
    1 failed test. All six failures are the obsolete prime sin/cos UNSUPPORTED expectations.
    `timeout 60 B/adf < tests/driver/f-places-hostile.cmd`: exit 1; diff exited 1 with one changed
    expected line: sin_at is now valid instead of PARSE. legacy-unpatched-c.log and
    legacy-unpatched-driver.diff. The pending patch resolves these without changing the requested behavior.

The last header edit corrected the cost comment to name both unit inverses. It changed no C statement.
No second clean acceptance run or full sanitizer run was made.

## Files written

- include/adelefeld/lfunc.h; src/lfunc.c.
- include/adelefeld/rfunc.h; src/rfunc.c.
- tests/test_lfunc_trig.c; additions and obsolete-expectation migration in tests/test_rfunc_prime.c.
- tests/julia/lfunc_trig.jl; two invocation lines in tests/test_julia.sh, one per existing success branch.
- proto/lfunc_trig_checks.py.
- tests/ref/vectors/f-slice7/cases.jsonl, points.jsonl, roots.jsonl.
- docs/api-1f4.md, new statements at its end.
- tools/adf/adf.c; tools/adf/README.md.
- tests/driver/trig-prime.cmd/.out, trig-real.cmd/.out, trig-hostile.cmd/.out.
- Lane files: progress.md, redgreen.log, driver_vectors.py, faults.py, prepare_legacy_patch.py,
  build_san.py, legacy-tests.patch, this report, and the logs/status files named above.
- Scratch sources, executables and libraries under lanes/f-slice7/build; final build products under build/.

The existing tests/test_lfunc.c and tests/ref/vectors/f-slice5/ were not edited.
No state-changing git command, tracker command, package installation, or delegated agent was used.

## What is not done

Full check-all and full sanitizer acceptance did not finish within the required 180 seconds.
Their later driver, Julia, mutation-tool self-test and memcheck self-test stages were not reached.
The new driver and Julia checks, exports, numerical tests and sanitizer wrapper checks were run as listed.

The explicit ownership rule says every other file is read-only. Permission was requested for the legacy
expectations, but no answer arrived. The tested patch remains unapplied to:

- tests/test_rfunc.c;
- tests/julia/sball.jl;
- tests/julia/f_at.jl;
- tests/driver/f-places-hostile.cmd.

The existing driver .out does not need a change: the unknown-command case becomes tan_at.
The f_at Julia patch replaces the obsolete failure with an exact 12-digit sine-series comparison.
The remaining unsupported-function tests in the owned test_rfunc_prime.c still cover log_abs and roots;
new sin/cos support is checked by the entire new oracle instead of impossible UNSUPPORTED assertions.

The orchestrator must apply or authorize the patch and complete the full suites on master.
The orchestrator's default LeakSanitizer, clang, INV and check_headers.sh checks were not run here.

## Sources pending

None for the new statements or implementation. Existing pending sources elsewhere in the repository
are not resolved by this lane. FLINT citations are on disk: arb.rst:4-12,1209-1219 and
fmpz.rst:852-859,880-883,1154-1160 under refs/src/flint-3.0.1/.
The series, domain, valuation, count and radius arguments cite docs/proofs/functions.md by file and line.
All additional proofs are written out in F10-F14 and in the exact oracle's header.

## Findings against the specification

None. The centred exponent policy is an allowed choice stated in the brief and Proposition 10.
The old unsupported expectations and the two acceptance timeouts are integration issues, not counterexamples.

## Proposed next slice

1F.5 local roots. It addresses the remaining prime root dispatch and requires explicit branch and whole-ball
certification before widening to the power functions of 1F.6.
