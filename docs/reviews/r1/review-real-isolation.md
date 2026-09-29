# r-review1: adversarial review of real-root isolation, adf-8di

## Findings

### 1. MAJOR: modest inputs still exceed ten seconds, including an input with no real roots

Three inputs exceed the brief's limit: degree <= 50, coefficient bits <= 10000, and prec <= 4096.
Times are measurements on the shared laptop. They are not portable lower bounds.

| Input | Degree | Max coefficient bits | prec | Call seconds | Status | Roots |
|---|---:|---:|---:|---:|---|---:|
| M = X^50 - 2(2^4000 X - 1)^2 | 50 | 8002 | 2 | 45.110611 | OK | 4 |
| P = X^50 + 2(2^4999 X - 1)^2 | 50 | 10000 | 2 | 11.096533 | OK | 0 |
| C = product_(i=1..25) (2^380 (3X - 1)^2 + i) | 50 | 9598 | 2 | 12.571040 | OK | 0 |

For M, peak memory was 17020 KiB. A separate run measured normalisation at 0.000133 s, FLINT's count at
0.054479 s, isolation plus refinement at 45.015855 s, and finish including another normalisation at
1.063366 s. The slow part is the new candidate route, not the trusted count or the final certificate.

For P, the full call used 8320 KiB peak memory. A separate stage run measured the trusted count at
0.053950 s with count 0, followed by 18.005171 s in isolation plus refinement, also returning zero roots.
The stage run shared the two allowed CPUs with another check; the full call above was a later run.
The input has only four nonzero coefficients. The output is empty. The descent after a trusted zero
count is unnecessary for the specified certificate. C used 17184 KiB peak memory for an empty output.

The author names descent for a cluster far from zero. M also makes a cluster near zero expensive.
P and C show a different wasted descent: complex roots close to the real axis after the real count is zero.

Why the answers above are true:

1. Write A = 2^4000. M = q_minus q_plus, where
   q_minus = X^25 - sqrt(2) A X + sqrt(2) and
   q_plus = X^25 + sqrt(2) A X - sqrt(2). This follows by multiplying the two factors.
2. q_plus' = 25 X^24 + sqrt(2) A > 0. Its limits have opposite signs, so it has exactly one real root.
   Its values at 0 and 1/A have opposite signs. This root is between those points.
3. q_minus is negative at minus infinity and positive at 0. It is positive at 1/A, negative at 2/A,
   and positive at plus infinity. At 2/A its value is 2^25/A^25 - sqrt(2), which is negative.
   Thus it has at least three distinct real roots in these disjoint intervals.
4. q_minus' = 25 X^24 - sqrt(2) A has exactly two zeros. Its signs give three monotone intervals,
   so q_minus has at most three real roots. The factors cannot share a root: that would require
   A X - 1 = 0 and X^25 = 0 at once. Therefore M has exactly four distinct real roots.
5. For every real x, both summands of P are nonnegative. If x^50 = 0, x = 0 and the other summand is 2.
   They cannot vanish together. Thus P is strictly positive and has no real root.
6. Every factor of C is at least i > 0 on the real line. Thus C has no real root.
   Two different factors differ by a nonzero constant, so cannot share a root. Each factor's
   derivative vanishes only at 1/3, where the factor equals i, so each is squarefree too.

The stage runs found gcd degree 0 for M and P. The displayed inputs are squarefree.

Reproduce, after the build and compile commands under Checks:

```sh
timeout 60 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/r-review1/probe mignotte 50 4000 2 skip
timeout 60 taskset -c 0,1 lanes/r-review1/probe mignotte 50 4000 2 stages
timeout 60 taskset -c 0,1 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/r-review1/probe positive 50 4999 2
timeout 60 taskset -c 0,1 lanes/r-review1/probe positive 50 4999 2 stages
timeout 30 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/r-review1/probe complex 50 190 2 skip
```

The `skip` argument omits the public verifiers after the timed call. It does not change the call or its
certificate. The stage command calls the same hidden candidate function, then the same finish function.

### 2. MINOR: R4(3) omits the scale-limit qualification

`docs/design/real-roots.md:157` says that a dyadic root with odd part of at most need + 1 bits is returned
as an exact point. There is no qualification about LIMIT in this assertion.

Input: f = 2^(M - 1) X - 1, M = 16777216, prec = 2. Its single root is r = 2^(1 - M).
The odd part is 1, with one bit. The root itself is of admissible size: r > 2^-M, with one midpoint bit
and zero radius. The public function returns ADF_LIMIT in 0.235533 s, not an exact point.

The private refinement reproducer starts at (c, k) = (0, KMIN), with KMIN = 2 - M.
It contains exactly this root, with clean ends. Galloping cannot form a smaller scale and returns LIMIT.
This contradicts the unqualified R4(3). The public header explicitly permits this scale LIMIT, so this
is a defect in the proposition's scope, not a false public status under that header.
R4(3) needs a condition that refinement returns without LIMIT.

```sh
timeout 30 lanes/r-review1/probe linear 1 16777215 2
timeout 20 lanes/r-review1/design_probe
```

The second command prints `dyadic root=2^(1-M) need=2 status=10`.

### 3. MINOR: R4(4)'s literal assertion for different floors is false

`docs/design/real-roots.md:158-160` says nesting holds for larger need "for any floors".
Its proof at lines 187-188 instead requires f_1 = f_2.

Input to the private refinement: g = 3X - 100, start cell (0, 6), r = 100/3.
Both of the following satisfy the stated hypothesis f < r when a floor is supplied:

- need = 2, floor = 33: status OK, cell [8533/256, 17067/512].
- need = 3, no floor: status OK, cell [33, 67/2].

The second cell is not inside the first; its left end 33 is below 8533/256.
The C reproducer prints c = 17066, k = -9 for the first and c = 66, k = -1 for the second.

Why: after galloping the sequence reaches [32, 40], then [33, 67/2]. The first cell has accuracy 2.
The second has accuracy 6. With floor 33 the second cell cannot stop, because its left end equals the
floor. It takes another quadratic step. Without that floor, need 3 stops at the second cell.
The visited cells are nested; the different stop constraints reverse which result is smaller.

```sh
timeout 20 lanes/r-review1/design_probe
```

This refutes the wording that permits different floors. The public algorithm uses the same static floor
for both precisions. No public nesting counterexample was found. The claim should explicitly fix the
same floor; its proof already does so.

### 4. MINOR: the differential script accepts a wrong place for constant inputs

The scratch mutation changes only `real_write` in a copy of `src/roots.c`: when g is constant,
set the list's place to prime 2. The required result is at the real place (`roots.h:345-348`).
No root, count, accuracy, or verifier logic is weakened by the mutation.

The unchanged differential script returned exit 0 after 241 calls in 30 s:
235 OK, 6 DOMAIN, 608 roots, 255 exact balls, 8 empty lists, 113 reduced inputs, and 0 disagreements.
It also reported exact agreement with the Python isolation port for all 235 OK lists.

For the planted constant input f = 7, prec = 2, a direct bridge call confirmed:
status = 0, canonical = 1, entries = 1, complete verifier = 1, place_is_real = 0.
The empty list for a constant is also valid at prime 2, so the generic verifiers do not catch the wrong
place. `diff_roots_real.py:287-293` checks accuracy, scope, completeness and verifiers, but not the place.

```sh
timeout 85 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/r-review1/mutate_diff.py
```

This command rebuilds the scratch shared library and runs the unchanged script. All scratch files stay
under this lane. The native review probes link against the lane archive, not this mutant library.

## Attacks without another finding

- The independent driver completed 1397 calls on 516 constructed inputs, with 4199 returned roots,
  1311 exact balls, 881 nesting pairs, and 20063 oracle checks. It found 0 errors.
  It tests exact endpoint signs, an independent rational chain's count in each ball and on the whole
  real line, strict ordering, planted-root coverage, actual arb accuracy, place, scope, completeness,
  the public predicates, and the two verifiers. Each mismatch or any unexpected status fails the run.
- The inputs include 400 seeded random draws; 60 products of quadratic factors; repeated factors;
  negative content and leading coefficients; roots at 0; both signs with equal absolute value;
  dyadic levels through 1000; touching cells; clusters 2^e + 1/3 and 2^e + 2/3 through e = 4000;
  and roots 2^-100000. Precisions range from WORD_MIN through 256.
- Ten extra calls tested the degree-50 Chebyshev polynomial, Wilkinson 20 and 40, and the planted pair
  2^e, 2^e + 1 at e = 10000 and 100000. They returned 228 roots, passed 944 oracle checks and
  five nesting comparisons, and found 0 errors. Chebyshev had 50 roots at each of two precisions.
- The final sanitizer driver repeated all 1397 calls: 4199 roots, 881 nesting pairs, 20063 oracle
  checks, 0 errors. AddressSanitizer and UndefinedBehaviorSanitizer emitted 0 diagnostics.
- The final valgrind driver completed 597 targeted calls: 1523 roots, 582 exact balls, 481 nesting
  pairs, 7549 oracle checks, 0 errors. Memcheck reported 0 errors from 0 contexts, 0 bytes in
  0 blocks at exit, and 143832 allocations matched by 143832 frees.
- The author's two real-root test programs ran 19 tests and 29909 checks in total, with 0 failures.
  No tautological assertion was found in the inspected isolation tests. The differential place gap
  above is an observed test weakness, not an assertion that every other test can fail usefully.
- No wrong list with OK, false NOT_DETERMINED, memory fault, or runnable slong overflow was found.
  Argument-derived precision LIMIT, constant handling, aliasing and untouched outputs were also
  exercised by the author's tests. Huge shifts at the scale boundary were exercised by design_probe.
- The source used for Descartes termination is quoted correctly. The width/separation statement is
  in `refs/src/sagraloff-mehlhorn/tex/arxivfinal.tex:569`; subadditivity is at lines 571-575.
  Endpoint deflation in R3 shifts the coefficient list or negates it, preserving variations.
  No counterexample to R1, R2, R3, or the fixed-floor public part of R4/R5 was found.
- A root at the initial upper bound cannot occur under R1's strict inequality. A power-of-two root
  lies strictly below that bound. The dyadic tests attack the split endpoints that can occur.
- Wilkinson 500 and Chebyshev 500 each hit a 60 s program timeout without a return marker.
  This gives no correctness result and does not prove an infinite loop. Degree 500 is outside
  the brief's degree-at-most-50 cost-finding criterion.

## Independent count argument

The driver's oracle does not call either root-count routine, either isolation implementation, or the
author's Python oracle. It uses Fraction arithmetic for division, gcd, and negative remainders.
Normalisation is over Q; a repeated irreducible factor of multiplicity m occurs m - 1 times in gcd(f,f').
Dividing leaves each factor once. This follows by differentiating the factorisation in characteristic 0.

For squarefree g, the chain is q_0 = g, q_1 = g', q_(i+1) = -rem(q_(i-1), q_i).
Each remainder is scaled by a positive rational only. The chain ends in a nonzero constant.

1. Consecutive chain members cannot vanish at the same point. The remainder identity would propagate
   that common zero to the final nonzero constant.
2. At a zero of an interior member q_i, the identity gives q_(i-1) = -q_(i+1).
   Its two neighbours therefore have opposite nonzero signs. Crossing this zero does not change
   the sum of the two adjacent sign variations. Omitting a zero gives the same sum.
3. At a simple root r of g, g' is nonzero. Locally g(x) = (x-r) h(x), with h(r) = g'(r).
   Immediately left of r the first pair has opposite signs; immediately right they have the same
   sign. Its variation drops by one. Interior zeros still contribute no change by step 2.
4. Between these zeros all signs are constant. Thus variation at a minus variation at b equals
   the number of roots in (a,b), for clean endpoints. At infinity the sign of each member is its
   leading sign, with the degree parity applied at minus infinity. Their difference counts all roots.
5. An exact ball is checked by substituting its rational point into g. A nonexact ball must have
   opposite nonzero endpoint signs and chain count 1. Strict order and the independent total count
   then exclude missing roots, duplicated roots, empty balls and balls with multiple roots.

The arb endpoint representation and accuracy convention used by the probe are on disk:
`refs/src/flint-3.0.1/arb.rst:461-466` and `:499-509`. The Chebyshev definition is at
`refs/src/flint-3.0.1/fmpz_poly.rst:3373-3375`. Its recurrence follows by adding u^n and u^-n
with u + u^-1 = 2x; the extra driver checks its root count with the independent rational chain.

## Checks run

Paths below are relative to the repository root. Builds and all test programs/scripts had timeouts.
`taskset -c 0,1` and `PYTHONDONTWRITEBYTECODE=1` were added to the later commands as shown in the logs.

The native compile command was:

```sh
timeout 60 cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
    lanes/r-review1/probe.c lanes/r-review1/build/libadelefeld.a -lflint -lgmp -lm \
    -o lanes/r-review1/probe
```

The design probe substitutes `design_probe.c` and `design_probe` in this command.
The sanitizer probe substitutes `build-san/libadelefeld.a`, output `probe-san`, and adds
`-fsanitize=address,undefined -fno-omit-frame-pointer`.

1. `timeout 170 make -j2 BUILD=lanes/r-review1/build`: exit 0; archive built from 21 source files.
   `timeout 170 make -j1 BUILD=lanes/r-review1/build-san SAN=1`: exit 0; sanitizer archive built.
2. Native probe compilation: four successful builds, exit 0. One intermediate compile exited 1:
   I used the nonexistent FLINT function fmpz_poly_divexact. It was corrected to fmpz_poly_div.
   Design probe: one build, exit 0. Sanitizer probe: two builds, exit 0.
3. `timeout 170 python3 lanes/r-review1/attack.py`: initial attempt exited 1 at Python's 4300-digit
   integer conversion limit. After disabling that limit, exit 0: 1397 calls, 20063 checks, 0 errors.
   The final run, after adding explicit place/scope checks in the probe, also exited 0 with the
   same counts; elapsed 1.770 s, maximum C call 0.003647 s. Logs: attack.log, attack-final.log.
4. Sanitizer command: `timeout 170 env ASAN_OPTIONS=detect_leaks=1:abort_on_error=1
   UBSAN_OPTIONS=halt_on_error=1 python3 lanes/r-review1/attack.py --probe lanes/r-review1/probe-san`.
   The first attempt exited 1 during process shutdown. LeakSanitizer reported that it cannot work
   under ptrace. It supplied no usable leak result. Repeated with detect_leaks=0: exit 0, 1397 calls,
   20063 checks, 0 errors, elapsed 3.427 s. Final probe repeat: exit 0, the same counts, 1.985 s.
   Logs: attack-san.log and attack-san-final.log; the initial failure was printed in the tool output.
5. `timeout 170 python3 lanes/r-review1/attack.py --random 0 --probe lanes/r-review1/probe-valgrind.sh`:
   two runs, both exit 0, each 597 calls and 7549 checks. Elapsed 7.444 s and 4.204 s.
   The wrapper sets full leak checking, all leak kinds as errors, and error exit 99.
   Final heap/error counts are given above. Logs: attack-valgrind-final.log and valgrind.log.
6. `timeout 170 python3 lanes/r-review1/extras.py`: exit 0; 10 calls, 228 roots, 944 checks,
   five nesting pairs, 0 errors. Log: extras.log.
7. `timeout 20 lanes/r-review1/design_probe`: exit 0; two OK cells and one LIMIT, exactly the
   counterexamples in findings 2 and 3. A separate `timeout 20 python3 -c` call of
   `proto.real_isolation.refine` on g = [-100,3], cell (0,6), and the same floors returned exactly
   the same two rational cells. This was the import that wrote the bytecode cache noted below.
8. `timeout 30 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/r-review1/probe linear 1 16777215 2`:
   exit 0; API status 10, 0.235533 s, peak 16468 KiB. Log: linear_boundary.log.
9. `timeout 55 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/r-review1/probe mignotte 50 4000 2 skip`:
   exit 0; API status 0, four roots, 45.110611 s. The 60 s stage command also exited 0;
   its four stage times and 14664 KiB peak are in mignotte_stages.log.
10. `timeout 55 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/r-review1/probe cluster 50 190 2 skip`:
    exit 0; 50 roots, 9580 coefficient bits, 0.149509 s, 16 exact balls, peak 5760 KiB.
11. `timeout 30 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/r-review1/probe complex 50 190 2 skip`:
    exit 0; no roots, 9598 coefficient bits, 12.571040 s, peak 17184 KiB. Log: complex50.log.
12. `timeout 65 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/r-review1/probe positive 50 4000 2 stages`:
    exit 0; count 0 in 0.047075 s, candidate route 9.247703 s, peak 7296 KiB.
    The corresponding 60 s stage command at e = 4999 exited 0; count 0 in 0.053950 s,
    candidate route 18.005171 s, peak 8064 KiB. The later 60 s full-call command at e = 4999
    exited 0; status OK, no roots, 10000 coefficient bits, 11.096533 s, peak 8320 KiB.
13. `timeout 85 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/r-review1/mutate_diff.py`:
    initial attempt exited 1 before compiling: the intended replacement lacked the reduced-field line.
    Corrected run exited 0. It invokes a 60 s shared-library compile and the unchanged differential
    script under a 45 s timeout with `--seconds 30 --seed 1 --lib lanes/r-review1/mutant.so`.
    Result: 241 calls, 0 disagreements, and the wrong-place direct check in finding 4.
14. `timeout 170 make -j1 BUILD=lanes/r-review1/build` with targets
    `lanes/r-review1/build/test_roots_real` and `lanes/r-review1/build/test_roots_real_isolate`:
    exit 0. Each executable then ran under `timeout 170`: 8 tests/10744 checks and 11 tests/19165 checks;
    each reported 0 failed checks and 0 failed tests. Logs: build-tests.log and test_roots_real*.log.
15. A bounded shell loop ran each row below under `timeout 60 /usr/bin/time`, with a 170 s outer
    timeout. Command form: `lanes/r-review1/probe FAMILY DEGREE EXPONENT PREC`. All use the verifiers.

| Arguments | Exit | API status | Roots | C call seconds | Peak KiB |
|---|---:|---:|---:|---:|---:|
| wilkinson 20 0 53 | 0 | 0 | 20, all exact | 0.000443 | 4992 |
| wilkinson 40 0 53 | 0 | 0 | 40, all exact | 0.004209 | 5376 |
| wilkinson 500 0 2 | 124 | no return | unknown | unknown | not recorded |
| chebyshev 50 0 4096 | 0 | 0 | 50 | 1.534379 | 6144 |
| chebyshev 500 0 2 | 124 | no return | unknown | unknown | not recorded |
| tiny 40 200 2 | 0 | 0 | 40, all exact | 2.238927 | 12416 |
| cluster 50 190 4096 | 0 | 0 | 50, 16 exact | 1.222091 | 6144 |

The last two completed programs including verifiers took 4.60 s and 1.66 s. The loop exited 0 after
recording both timeout statuses; neither timed-out case is counted as a passing check. Log: families.log.

16. Two extra `timeout 20 python3 -c` checks were run. A prototype spot check on [9,-72,108]
    gave bound exponent 0 and two roots; [-1,2^100] gave one exact root. Exit 0, proto_spot.log.
    An initial one-line recurrence generator made a different degree-50 polynomial with only two
    real roots; two calls passed 16 rational-chain checks. Exit 0, cheb_oracle.log. This was a harness
    mistake, not a Chebyshev result. It was replaced by extras.py's correct recurrence and 50-root check.
17. `timeout 20 python3 -c` read this report and counted lines longer than 116 characters:
    exit 0; 330 lines at the first check, 0 overlong lines. This was a prose-format check only.

Read-only file and source searches are not test claims. Source hashes are in source.sha256.

## Files written and execution limits

Written in this lane: probe.c, attack.py, extras.py, design_probe.c, mutate_diff.py,
probe-valgrind.sh, mutant_roots.c, mutant.so, probe, probe-san, design_probe, this report,
source.sha256, the named run/build logs, and generated files in build/ and build-san/.
brief.md, lane.log and stdout.log are harness files; I did not edit them.
No repository source, test, specification, or design text was changed. No git or bd command ran.

Two procedural mistakes occurred. An early Python import created
`proto/__pycache__/real_isolation.cpython-312.pyc` outside the owned lane. I did not delete or alter it
after detection; it is left for the orchestrator. Subsequent imports disabled bytecode writes.
One mutation run, one -j1 test build and one oracle run briefly overlapped. That could use three CPUs,
contrary to the lane rule. Later checks shared affinity to CPUs 0 and 1. No make invocation used more than -j2.
Every test program and script had a timeout of at most 170 s; no completed check took three minutes.

## Not done

No long fuzz campaign, no exhaustive enumeration, and no hybrid benchmark. No successful LeakSanitizer
leak run; valgrind supplied the leak check. No correctness conclusion for the two degree-500 timeouts.
No executable exponent-overflow counterexample and no quantified worst-case peak-memory proof.
The false R4 qualifications were not repaired, because the reviewed files are read-only in this lane.

## Sources pending

The review's count argument and planted counts are proved above; they need no external Sturm citation.
Inherited open sources remain:

- [source pending: an on-disk source for Sturm's theorem behind FLINT's trusted real-root count]
- [source pending: Kerber's analysis proving eventual quadratic convergence of Eqir]

The latter is cited indirectly in `refs/src/kerber-sagraloff/tex/arxiv.tex:337-342`.
This review does not assert quadratic convergence from that indirect citation.

## Findings against the specification

None proved. The scale LIMIT in finding 2 is permitted by the header. The nesting statement of N-D2
uses fixed static floors, which the different-floor counterexample does not refute.
S-D11, S-D13 and S-D19 were not refuted by the completed oracle checks.
The time findings meet this review brief's criterion; they are not a stated ten-second promise in SPEC.md.

## Judgment of the reported high-precision regression

A hybrid refinement is worth benchmarking after exact isolation, with the same nesting requirement.
The reported two-times regression does not justify restoring the old complex-root isolation.
First remove the known-empty descent. This review did not measure a hybrid.
