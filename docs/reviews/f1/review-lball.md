# f-review1: adversarial review

Date: 2026-09-29. No repository implementation, header, specification or author test was changed.
No git command or bd command was run. No subagents were used.

The report has 4 MAJOR findings and 2 MINOR findings. No BLOCKER was established on canonical inputs.
The seven arithmetic counterexamples below return LIMIT, rather than an incorrect enclosure on OK.

All commands below run from the repository root. Build output was redirected into this lane with BUILD,
because the ordinary build directory is outside this lane's ownership. The review programs link the
resulting build/libadelefeld.a inside this lane; invariant and sanitizer archives use separate directories.

## Findings

### F1. MAJOR: exact inverse applies the ball precision limit

Location: src/lball.c:656. Input: p = 5, exact = 1, u = 1, v = E or -E, N = 0, where E = 2^60.
Both inputs satisfy is_canonical and the documented exponent limits. inv returns LIMIT in both cases,
with the sentinel output untouched. The result is the exact value with u = 1, v = -E or E, N = 0.

Proof: the value is 5^v, so its reciprocal is 5^(-v). The output uses only u = 1 and one admitted exponent.
No power needs to be allocated. The guard tests N - 2v, which is -2E or 2E, although an exact value has
no finite precision and the later call to lb_make passes N = 0 for that case.

Reproduce: `timeout 20 lanes/f-review1/boundaries`. The first two lines report LIMIT instead of OK.
The program exits 1 and reports 7 failed cases in total. boundaries-inv and boundaries-san give the same
two failures; see the commands under Checks.

### F2. MAJOR: subtraction refuses small results because it first canonicalises the negated operand

Location: src/lball.c:585. Inputs at p = 5:

- x = y = 1 + 5^E Z_5. sub(x, y) returns LIMIT; the smallest result is 5^E Z_5.
- x = Z_5, y = 1 + 5^E Z_5. sub(x, y) returns LIMIT; the smallest result is Z_5.

All input fields are admitted and canonical. In both calls the output remains the sentinel.
Both true results have u = 0, v = 0, and an admitted N, so their representation has constant size.

Proof for the first case: (1 + 5^E a) - (1 + 5^E b) = 5^E(a - b).
As a and b range independently over Z_5, a - b ranges over Z_5, by taking b = 0.
Proof for the second case: a - 1 - 5^E b lies in Z_5, and every t in Z_5 is reached by b = 0, a = t + 1.
The temporary negation has centre 5^E - 1 and needs a forbidden power; this temporary is unnecessary
for either final result. This is a valid small result refused, not a claim that neg itself must succeed.

Reproduce: `timeout 20 lanes/f-review1/boundaries`. Lines 3 and 4 report LIMIT instead of OK.

### F3. MAJOR: division refuses small results because it materialises the full inverse first

Location: src/lball.c:676. Inputs at p = 5:

- x = exact 0, y = 3 + 5^E Z_5. div returns LIMIT; the result is exact 0.
- x = Z_5, y = 3 + 5^E Z_5. div returns LIMIT; the result is Z_5.
- x = y = 5^(-E) + Z_5. div returns LIMIT; the result is 1 + 5^E Z_5.

All inputs are canonical and within the exponent limits. All three result representations have u = 0
or 1 and admitted exponents. The output remains untouched in all three failures.

Proof for the first two cases: every point of y is a unit, because it is 3 modulo 5.
Its reciprocal is therefore defined, and multiplying exact 0 by it gives exact 0.
For any fixed point t of y, division by t maps Z_5 bijectively onto Z_5, so the second quotient set is Z_5.
The code instead tries to construct the full inverse centre of 3 modulo 5^E and hits the bit bound.

For the third case, write each point as 5^(-E)(1 + 5^E a).
A quotient is (1 + 5^E a)/(1 + 5^E b); subtracting 1 gives 5^E(a - b)/(1 + 5^E b).
The denominator is a unit, so every quotient lies in 1 + 5^E Z_5.
Every point of that ball is reached by b = 0 and the corresponding a.
The inverse alone would have N = 2E, outside the limit, but the quotient has N = E and v = 0.

Reproduce: `timeout 20 lanes/f-review1/boundaries`. Lines 5 to 7 report LIMIT instead of OK.

### F4. MAJOR: invariant build omits required checks in nine public entry paths

Locations: src/lball.c:245, 255, 308, 316, 328, 372, 382, 388, 394.
conventions.md 4.4 requires a canonical-input check and an abort in the invariant build.
This finding concerns that debug contract; it does not claim release-build guarantees on invalid inputs.

The probe supplies an initialised lball with p = 4 to place, is_exact, contains_zero, get_prec, set, swap,
and identical. All seven return normally. place returns the archimedean place, get_prec returns OK with
N = 0, and set and swap transfer the non-canonical value to the output.

The other two probes supply an initialised adf_rat whose unreduced fields are numerator 2, denominator 4
to set_rat and set_rat_ball at p = 5. Both return OK without aborting.
set_rat stores u = 2/4 and produces an output for which is_canonical returns 0.
set_rat_ball happens to produce a canonical output, but still misses the required input check.
The add control with p = 4 aborts with the expected invariant message, exit -6.

Reproduce: `timeout 65 python3 -B lanes/f-review1/check_invariants.py`.
Result: 9 invalid entry calls, 9 missed checks, 1 aborting control, script exit 1.
To isolate the non-canonical constructor output: `timeout 5 lanes/f-review1/invariant_probe rat`.
It prints returned=0 and output_canonical=0, then exits 0.

### F5. MINOR: L0 contains an equality with the wrong sign

Location: docs/api-1f.md:71. The proof writes t - u = (b u - a)/b when t = a/b.
Take p = 2, N = 1, q = t = 1/3, a = 1, b = 3, u = 1.
The left side is -2/3; the written right side is 2/3.
The correct equality is t - u = (a - b u)/b.
Both signs give the same valuation, so this does not refute L0's reduction statement.

Reproduce: `timeout 10 python3 -B lanes/f-review1/proof_probe.py`.
Result: equality=False and an assertion failure, exit 1.

### F6. MINOR: the unit-valuation assertion cannot reject any canonical exact output

Location: tests/test_lball.c:976. The assertion is
unit->v == 0 || (unit->exact && !fmpq_is_zero(unit->u)).
For a canonical exact nonzero output its right disjunct is true regardless of valuation.
For a canonical exact zero output its left disjunct is true because its valuation field must be 0.
Thus every canonical exact output passes this assertion, including the wrong unit 5 with valuation 1.
Other assertions in the round-trip test may still detect a faulty decomposition; this finding is about
this assertion, not the claim that the entire test can never fail.

Reproduce: `timeout 10 lanes/f-review1/test_assertion`.
Result: wrong_unit=5, canonical=1, valuation=1, assertion_at_test_lball_976=1; exit 1.

## Attacks without a counterexample

The independent oracle is oracle.py. It uses Python Fraction arithmetic, does not import the author's
reference, and never uses library contains or equal_set to decide enclosure or tightness.
bridge.c only passes raw fields and calls the functions under review.

- 4,740 arithmetic cases across add, sub, mul, div, neg and inv, including 500 expected failure statuses.
  The 4,240 successful cases enumerate 1,010,640 exact point results.
  A case fails if a point result differs from an exact output or has v_p(result - centre) < output N.
  Tightness fails if the minimum valuation of differences from the first sampled result differs from N,
  or if a constant sampled image is returned as an inexact ball.
  This establishes smallest-ball witnesses for those cases, rather than testing only round-trip identities.
- Pair counts: 300 at 2, 300 at 3, 150 at 5, 100 at 7, and 200 at 2^64 - 59.
  At 2 and 3, a ranges over three full digits; at 5 and 7, over two full digits.
  At the large prime, a is sampled from 0, 1, 2, p-1, p, p+1, p^2-1.
  Two results whose difference has valuation N already exclude every smaller ball, even at the large prime.
  Negative centres, rational units, mixed exact/ball inputs, different and negative valuations, and balls
  containing 0 are included. Centre valuations arise from random rational centres scaled by p^-4 to p^4.
- 250 constructors checked against an independently reduced rational centre; 250 sets of centre, valuation,
  absolute-value, decomposition, exact-tag, zero-membership and canonical-output checks.
  A wrong field, status, scalar output, rational output or aliased decomposition fails the case.
- 4,200 equal_set, identical, overlaps and contains calls checked against exact rational centre differences.
  A mismatch in the boolean fails the case. Four additional different-prime predicate calls also return 0.
- 644 successful arithmetic calls with output aliased to each input, compared with the out-of-place call.
  Decomposition is also checked with its output aliased to its input on every nonzero constructor case.
  Separate status_aliases.py checks 16 non-OK cases in three output positions: 48 calls, 0 changed outputs.
- 30,000 canonical power-threshold cases: k = 1..200 at 2, 3, 5, 7 and 2^64 - 59; values adjacent to p^k,
  adjacent binary thresholds, negative and zero u, and three valuations per candidate.
  Another 90 cases use slong endpoints, including N - v = 2^64 - 1.
  Each result is compared with direct integer inequalities, not a bit-length shortcut.
- 195,840 raw-field canonicality cases, including zero and negative denominators, unreduced rationals,
  invalid exact tags, composite primes, signs, zero centres and v >= N.
  A disagreement with a small-integer gcd and power oracle, or an abort, fails the run.
- 1,500 global projections and 1,000 local projections checked against the raw rational centre and radius.
  Local contexts use blocks (4,9,5), (8,25,7), (16,27,11), and the single block 2^64 - 59.
  A wrong precision, centre, exact tag or status fails the case, including denominators sharing factors
  with blocks and primes absent from a context.
- L3 and L4 were read case by case. No false product or inverse statement was established.
  For L3, fixing the operand whose linear error term attains K gives every result residue; when both
  centres are zero, taking one factor equal to its radius gives every result point.
  For L4, the stated map -z/(1+z) has unit denominator and is its own inverse on p^k Z_p for k >= 1.
  This covers negative valuations and p = 2 without a parity exception.
- The release and invariant oracle runs give identical counts and 0 mismatches.
  With leak detection disabled, sanitizer runs give the same results and no address or undefined-behaviour
  diagnostics. This is bounded deterministic enumeration, not a long fuzz run or a leak certification.

## Judgment of the author's decisions 2 to 5

2. Statuses: the exact-zero and zero-containing-ball distinctions express different mathematical failures.
The conventions 3.2 matrix lacks an lball row covering these statuses together with LIMIT and DOMAIN,
so the orchestrator must reconcile that matrix with the published header.

3. Limits: the exponent bounds keep the admitted sums and differences away from signed overflow.
Their application is defective for exact inverse and for intermediate values of subtraction and division,
as F1 to F3 show.

4. Two primes: DOMAIN for arithmetic and false for set predicates specify the compatibility requirement.
The four arithmetic and four predicate cross-prime checks exercised here match that decision.

5. is_canonical: the lower bound p^k >= 2^(k(bits(p)-1)) justifies the shortcut when it answers true.
The fallback and unsigned exponent difference had 0 mismatches in 225,930 independent cases, but F4 shows
that several debug entry paths do not call this predicate.

## Files written

All paths are inside lanes/f-review1/:

- Sources: bridge.c, oracle.py, extras.py, boundaries.c, invariant_probe.c, check_invariants.py,
  canonical_raw.c, proof_probe.py, test_assertion.c, status_aliases.py, and this report.md.
- Executables: bridge, bridge-inv, bridge-san, boundaries, boundaries-inv, boundaries-san,
  invariant_probe, canonical_raw, canonical_raw-san, test_assertion.
- Build artifacts: build/, invbuild/, sanbuild/, containing the three archives, objects and dependencies;
  build/ also contains the author's test_lball executable and its support objects.
- Logs: build.log, invbuild.log, sanbuild.log, author-build.log, author-test.log, boundaries.log,
  boundaries-inv.log, boundaries-san.log, boundaries-san-noleak.log, invariants.log, oracle.log,
  oracle-inv.log, oracle-san.log, oracle-san-noleak.log, extras.log, extras-san.log,
  extras-san-noleak.log, canonical-raw.log, canonical-raw-san.log, canonical-raw-san-noleak.log,
  proof.log, assertion.log, status-aliases.log, status-aliases-san.log.
The pre-existing brief.md and lane.log were not edited.

## Checks and commands

Commands shown without redirection had stdout and stderr saved in the corresponding log listed above.
All test programs and Python scripts ran under timeout; no command reached its timeout.
Builds used -j2. The invariant and sanitizer builds overlapped one oracle process, so the aggregate
two-core cap was not enforced during those overlaps; no build used more than two compiler jobs.

Library builds, each exit 0 and one archive produced:

    timeout 180 make -j2 BUILD=lanes/f-review1/build all
    timeout 180 make -j2 BUILD=lanes/f-review1/invbuild INV=1 all
    timeout 180 make -j2 BUILD=lanes/f-review1/sanbuild SAN=1 INV=1 all

Each release review executable was compiled with the following command, substituting T with bridge,
boundaries, canonical_raw and test_assertion; all final compiles exited 0:

    timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude lanes/f-review1/T.c \
      lanes/f-review1/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review1/T

bridge was recompiled after adding projection commands. The first boundaries compile failed with one
const-qualifier error in the review program's use of fmpq_equal_si, exit 1; the attempted timeout run of
the absent binary exited 127. The review program was corrected to compare numerator and denominator.

Invariant compiles used the following form for invariant_probe, bridge and boundaries, all exit 0:
The bridge and boundaries output names were bridge-inv and boundaries-inv; invariant_probe kept its name.

    timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Werror -DADF_CHECK_INVARIANTS -Iinclude \
      lanes/f-review1/T.c lanes/f-review1/invbuild/libadelefeld.a -lflint -lgmp -lm -pthread \
      -o lanes/f-review1/T-inv

bridge-inv was compiled twice, before and after the adapter's projection commands were added.
Sanitizer compiles used the following form for bridge, canonical_raw and boundaries, all exit 0:
bridge additionally used -DADF_CHECK_INVARIANTS.

    timeout 60 cc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
      -fno-omit-frame-pointer -Iinclude lanes/f-review1/T.c lanes/f-review1/sanbuild/libadelefeld.a \
      -lflint -lgmp -lm -pthread -o lanes/f-review1/T-san

The canonical_raw sanitizer output name used an underscore: canonical_raw-san.

Test executions:

    timeout 160 python3 -B lanes/f-review1/oracle.py

Exit 0: 4,740 arithmetic cases, 1,010,640 point results, 4,240 tight hulls, 4,200 predicates,
250 constructors, 250 accessor sets, 644 alias calls, 500 untouched status outputs.

    timeout 160 python3 -B lanes/f-review1/oracle.py lanes/f-review1/bridge-inv

Exit 0, the same counts and 0 mismatches.

    timeout 160 python3 -B lanes/f-review1/extras.py

Exit 0: 30,000 thresholds, 90 extremes, 1,500 global and 1,000 local projections, 0 mismatches.

    timeout 30 lanes/f-review1/canonical_raw

Exit 0: 195,840 cases, 0 failures.

    timeout 20 lanes/f-review1/boundaries
    timeout 20 lanes/f-review1/boundaries-inv

Each exits 1: 7 cases, 9 canonical input checks, 7 false LIMIT results, 7 untouched outputs.

    timeout 65 python3 -B lanes/f-review1/check_invariants.py

Exit 1: 9 missed checks in 9 invalid calls, 1 aborting control.
Each child ran as `timeout 5 lanes/f-review1/invariant_probe METHOD`; the nine method names are
place, exact, zero, prec, set, swap, identical, rat, ratball; add_control was the tenth child.
The nine return exit 0, and the control returns -6 through Python's subprocess interface.

    timeout 10 python3 -B lanes/f-review1/proof_probe.py
    timeout 10 lanes/f-review1/test_assertion

Each exits 1, reproducing one MINOR finding.

    timeout 20 python3 -B lanes/f-review1/status_aliases.py

Exit 0: 16 non-OK cases, 48 output-alias calls, 53 checks, 0 failures.

    timeout 180 make -j2 BUILD=lanes/f-review1/build lanes/f-review1/build/test_lball
    timeout 30 lanes/f-review1/build/test_lball

Both exit 0. The author's test gives 20 tests, 3,938,748 checks, 0 failed checks, 0 failed tests.

The first sanitizer executions were these four commands:

    timeout 170 python3 -B lanes/f-review1/oracle.py lanes/f-review1/bridge-san
    timeout 60 lanes/f-review1/canonical_raw-san
    timeout 30 lanes/f-review1/boundaries-san
    timeout 170 python3 -B lanes/f-review1/extras.py lanes/f-review1/bridge-san

All exited 1 because LeakSanitizer reported a fatal inability to run under ptrace.
The two Python runs completed their case counts before their child failed at shutdown.
The two C runs did not preserve buffered stdout in those failed shutdowns.

The follow-up executions disabled only leak detection:

    ASAN_OPTIONS=detect_leaks=0 timeout 170 python3 -B lanes/f-review1/oracle.py lanes/f-review1/bridge-san
    ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/f-review1/canonical_raw-san
    ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/f-review1/boundaries-san
    ASAN_OPTIONS=detect_leaks=0 timeout 170 python3 -B lanes/f-review1/extras.py lanes/f-review1/bridge-san
    ASAN_OPTIONS=detect_leaks=0 timeout 20 python3 -B lanes/f-review1/status_aliases.py lanes/f-review1/bridge-san

Exits respectively 0, 0, 1, 0, 0. Counts match the corresponding release runs.
The boundaries exit 1 is the same seven false LIMIT results, not a sanitizer diagnostic.
No address or undefined-behaviour diagnostic occurred in these five runs.

## Not done

- No fixes, edits outside the lane, mutation run, long fuzz run, benchmark or full repository test suite.
- Julia and the author's Python checks were read but not executed.
- No allocator profiling or certified leak check; LeakSanitizer could not run in this environment.
- No exhaustive enumeration of all large-prime digits or all admitted exponent combinations.
- No invalid-pointer testing; such pointers are outside is_canonical's non-abort promise.
- No claim that a successful bounded enumeration proves enclosure on all Q_p points.
- The review adapter reads rational strings up to 19,999 characters; the generated tests remain below that bound.

## Sources pending

refs/ contains fetch scripts, manifests and README, but none of the source documents cited by the author.
No external formula was quoted from memory; the counterexamples and hull witnesses above are own calculations.

[source pending: refs/src/flint-3.0.1/fmpz.rst:1142-1160, cited for fmpz_remove and fmpz_invmod]
[source pending: FLINT 3.0.1 padic.rst:15-18, cited by conventions.md 5.8 for reduced storage]
[source pending: local source copy for the certified n_is_prime convention cited through ulong_extras.h:335]
[source pending: the Teichmueller naming convention already marked pending in functions.md Proposition 4]

## Findings against the specification

No false mathematical statement in docs/SPEC.md 4 or 9 was established.
F1 to F4 are implementation or contract-enforcement defects; F5 is in api-1f.md, and F6 is in a test.
conventions.md 3.2 still has no explicit lball row: its inversion row lacks LIMIT and DOMAIN, while its
functions-at-places row lacks NOT_UNIT and UNIT_NOT_CERTIFIED. This is an unresolved status-class gap,
not a reason to change the specification or conceal a failure.
