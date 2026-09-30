# Lane i-review2: adversarial review of milestone 2, slices 2 and 3

Only lanes/i-review2/ was written. No git or bd command was run. No package was installed.
The default archive is lanes/i-review2/build/libadelefeld.a. The second archive has SAN=1 and INV=1.
source.sha256 records the reviewed headers, sources and contract documents.
The default archive was still current at the end: make -q returned 0.

## Findings

### F1. MAJOR: INV=1 allocates before the precision limit is checked

Contract: docs/SPEC.md:923, N-D8. Above 2097152 bits, LIMIT must be decided from prec alone,
before any allocation. The public headers repeat this rule without an exception for INV=1.

Input: a canonical idele with real part exactly 1, content
(2^4096 + 3)/(2^2048 + 7), reduced, and unit [1 mod (2^2048 + 1)].
prec = 2097153. The power exponent is 0.

Code returns LIMIT from all four calls in limit_alloc.c:
adf_idele_norm, adf_idclass_set_idele, adf_idele_pow, adf_idele_pow_tight.
With SAN=1 INV=1, each call makes 14 FLINT allocator calls requesting 135160 bytes before returning.
With the default archive, each makes 0 allocator calls and requests 0 bytes.

Why this violates the contract:
1. limit_alloc.c validates the input before measuring. It clears unused FLINT caches before each call.
2. Its allocator hooks count only allocations during that call.
3. src/idele.c:521 checks the invariant before checking prec at line 522.
   src/idclass.c:102 does the same before line 103.
   src/idpow.c:323 and :330 check the invariant before entering the helper that checks prec.
4. The invariant predicates compute integer gcds. Those checks allocate on this valid large input.
5. Thus this is an allocation-order violation on valid input. It does not rely on an invalid-input abort.

The hook API is documented in refs/src/flint-3.0.1/memory.rst:16-21.
The measurement counts FLINT allocator requests, not every system or GMP allocation.

Reproduce from the repository root:

    timeout 180 make -j2 BUILD=lanes/i-review2/build-san SAN=1 INV=1
    timeout 60 cc -std=c11 -O1 -g -Wall -Wextra -Werror -DADF_CHECK_INVARIANTS \
      -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude \
      lanes/i-review2/limit_alloc.c lanes/i-review2/build-san/libadelefeld.a \
      -lflint -lgmp -lm -o lanes/i-review2/limit-alloc-san
    ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/i-review2/limit-alloc-san

Evidence: limit-alloc-san.log and limit-alloc.log.
An early public prec check would avoid reading these inputs before returning LIMIT.

### F2. MINOR: the class header promises exactness across different precisions

Contract sentence: include/adelefeld/idclass.h:107-109 says the class of a rational is the exact
<1 ; [1]> when the real ball of the rational is exact. There is no condition on the conversion precision.

Input: adf_idele_set_rat(x, 5, 64), followed by adf_idclass_set_idele(c, x, 2).
The constructor returns OK and its real part is exactly 5.
The class conversion returns OK, t = 1 +/- 1/2, unit [1]. t is not exact.
adf_idele_norm(t, x, 2) returns the same inexact ball.

What is true, step by step:
1. Every point of x is the diagonal rational 5, so its norm is 5/5 = 1.
2. The code first rounds the exact input endpoint 5 down to 4 and up to 6 at two bits.
3. Scaling these endpoints by the exact 1/5 and rounding again gives 3/4 and 3/2.
4. Kernel B returns 1 +/- 1/2. This encloses 1, but is not the exact class promised by the header.
5. Statement G.5 in docs/api-2.md:305 correctly includes the missing condition that q fits the
   precision of the class call. The public class header omits it.

This input also exposes endpoint pre-rounding in the norm. A single final rounding of the true
endpoint 5/5 gives 1 in both directions; the implementation rounds before the rational scale and again
after it. include/adelefeld/idele.h:195-198 says each endpoint is rounded once.
Statement F explicitly defines its inputs as already rounded endpoints, so the implementation follows F.
The finite rational factor is exact. No norm point lies outside this result.
I do not claim that SPEC 5 requires the smallest real ball.

Reproduce F2 through F5:

    timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude \
      lanes/i-review2/findings.c lanes/i-review2/build/libadelefeld.a \
      -lflint -lgmp -lm -o lanes/i-review2/findings
    timeout 30 lanes/i-review2/findings

Evidence: findings.log, lines labelled F1. These labels predate the report's ordering.

### F3. MINOR: the power header's exactness condition includes negative exponents

Contract sentence: include/adelefeld/idpow.h:89 says Z is exact if X is exact and
|m|^|k| has at most p bits. It does not restrict k to positive values.

Input: real part exactly 3, content 1, exact unit [1], k = -1, prec = 2.
adf_idele_pow returns OK with real part 1/4 +/- 1/8. It is not exact.
The advertised condition holds: |m|^|k| = 3 has two bits.

What is true:
1. The real power is 1/3.
2. An exact binary floating-point number cannot equal 1/3.
   If 1/3 = a/2^j for integers a and j >= 0, then 2^j = 3a, which is impossible.
3. The returned ball contains 1/3.
4. Statement K.4, docs/api-2.md:465, includes k > 0. The header drops that hypothesis.
   The tight power inherits the same header promise.

Reproduce: the findings command under F2. Evidence: findings.log, lines labelled F2.

### F4. MINOR: Statement L.5 uses positive repeated multiplication for negative powers

False statement: docs/api-2.md:503-506 says the product of abs(k) independent copies of x has
content r^abs(k), unit c'^abs(k) U(N'), and contains the subset {xi^k}.

Input: the exact rational idele 2, k = -1, prec = 64.
adf_idele_pow returns OK with real part exactly 1/2 and content 1/2.
The product of abs(k) = 1 copies of x is x, with real part exactly 2 and content 2.

What is true:
1. {xi^k} = {1/2}.
2. The real product set of one copy is {2}.
3. {1/2} is not a subset of {2}.
4. For k < 0 the comparison must use copies of the inverse.
   For k = 0, the empty product is exact; the displayed finite-precision coset formula also needs an exception.

The implementation returns the correct inverse. This is a false proof item, not an enclosure defect.
Reproduce: the findings command under F2. Evidence: findings.log, lines labelled F3.

### F5. MINOR: Statement M.2 reverses the numerical radius ratio

False statement: docs/api-2.md:517-519 says the simple hull has twice the smallest hull's radius
when the normal unit modulus is odd.

Input: real part 1, content 1, unit [2 mod 3].
adf_adele_set_idele_simple returns finite part 2 + 3 Zhat, radius 3.
adf_adele_set_idele returns finite part 5 + 6 Zhat, radius 6.

What is true:
1. Units congruent to 2 modulo 3 have odd 2-coordinate.
2. Hence their smallest additive hull is 5 + 6 Zhat.
3. The simple radius is half the smallest radius, not twice it.
4. The simple hull is nevertheless the coarser set, since 6 Zhat is inside 3 Zhat.

The code and the header's phrase "twice as coarse" have the correct direction.
Reproduce: the findings command under F2. Evidence: findings.log, lines labelled F4.

## Statement J was attacked first

I checked the proposed construction against Proposition 13 before running the other sweeps.

The algebraic comparison, written out:
1. For every prime dividing normal N', A has exponent v_p(N') + v_p(k).
   At 2, normality ensures v_2(N') >= 2 when 2 divides N'.
2. The primes of B do not divide N'. Its 2-part, when present, has exponent 2 + v_2(k).
   Every odd prime in B has exponent 1 + v_p(k), with p - 1 dividing k.
3. These are exactly the cases required by Proposition 13. A and B are coprime.
4. An odd prime required in B is d + 1 for an even divisor d of abs(k).
   Enumerating all divisor exponent vectors therefore searches every such prime.
   Even at WORD_MIN, d + 1 <= 2^63 + 1, so the word addition does not overflow.
5. Choose chat = c' modulo A and chat = 1 modulo B.
   A has only primes of N', so chat is coprime to A. It is also coprime to B.
   It is congruent to c' modulo N'. Raising these congruences to k, using inverses for k < 0,
   gives the residue computed by the CRT.
6. For the size bound, the v_p(k) factors used in A and B are disjoint parts of abs(k).
   The remaining factor is at most 4 times one odd prime per divisor d >= 2.
   Multiplication adds bit lengths, and multiplication by 4 adds two bits.
   This gives J.4's displayed upper bound.

The word primality and factorization contracts used here are on disk:
refs/src/flint-3.0.1/ulong_extras.rst:833-840 and :1203-1211.
No claim about Bernoulli denominators was used.

Independent enumeration did not use proto/ideles_checks.py or its power-modulus table.
For a prime power T, the image of the input coset consists of units w modulo T satisfying
w = c modulo gcd(N, T). This is a projection of (Z/lcm(N,T))^*:
compatible congruences lift by choosing the coordinates at primes separately.
Multiplication by a fixed unit lift takes the principal local coset to this coset.
It preserves the gcd of differences of powers. Thus I enumerate the principal coset once,
then check the output residue with a compatible unit lift of c.
The gcd of T and all power differences is the largest congruence modulus at that level.
Removing a solitary factor 2 gives the unit-coset normal modulus.

## Attacks without an additional finding

- Powers: 19384 inputs; every unit centre for every N from 1 through 40.
  Exponents -12 through 12, 30, 60, 64, 210, and primes 13, 17, 19, 23, 29, 31, 37, 41, 43, 47.
  Exact units, WORD_MIN, WORD_MAX and four other large exponents were also included.
  Final sweep: 884736 local coset checks, 4160 distinct enumerations, 44167270 unit residues.
  Every level lcm(N,T) was <= 100000. Primes through 211 were inspected.
  Failure conditions: a wrong default modulus or residue; a tight projected modulus different from
  the normalized gcd of power differences; a missed power residue; non-normal output; alias mismatch.
  No failures. Large-exponent global minimality beyond these finite levels still rests on the proof.

- Real operations: 22000 independent exact-rational cases, with signed dyadic midpoints up to 60 bits,
  radii from zero to wide sign-definite intervals, precisions below 2 through 256.
  Includes positive, zero and negative powers, even and odd exponents, mul_rat, norm and the class map.
  Results: 18982 OK, 3002 NOT_DETERMINED, 16 NOT_UNIT from multiplying by exact zero.
  Failure conditions: a missing true endpoint; wrong sign; zero in an OK idele result;
  a status different from the directed-rounding criterion; changed aliased output on refusal;
  wrong exact content or signed class unit. No failures in default or SAN=1 INV=1 runs.

- Hulls and division: 4028 cases, including 108 exact-unit cases.
  3920 finite enumerations, 183479692 quotient residues, 74240 hull unit residues.
  Inputs include a = 0, M = 0, both zero, all N <= 40, odd N, and scales with factors shared with radii.
  Failure conditions: a quotient or unit outside a result; a smallest radius different from the gcd
  of enumerated differences; a wrong canonical triple; a simple hull missing a unit;
  a real quotient endpoint outside the real ball; alias mismatch. No failures.

- Class invariance: 2940 exactly rational-scaled inputs, q in {-8,-3,-1,1,3,8}.
  95616 enumerated class unit residues. Negative real signs included.
  Failure conditions: different finite class sets or a class ball missing either true norm endpoint.
  No failures. This does not assert equality of the rounded real balls of a computed mul_rat chain.

- Valuations and absolute values: 8744 cases against division counting, including numerator factors,
  denominator factors, neither, and the real-place DOMAIN status with unchanged outputs.
  Another 10 cases used p = 18446744073709551557 and positive or negative valuations 0 through 4.
  Failure conditions: wrong valuation, wrong exact p^(-v), wrong refusal or touched refused output.
  No failures.

- Diagonal rational norms: 243040 calls with independent constructor and norm precisions
  from {2,3,4,8,30,64,256}; 241116 OK, 1924 NOT_DETERMINED.
  Every OK norm contains 1; its class has the exact unit [1].
  Failure conditions: an OK result missing 1, disagreement of norm and class statuses,
  wrong class unit, or changed norm output on refusal. No failures in both builds.

- Class wrappers and precision edges: 37588 assertions, 15000 class operations,
  20 additive-to-idele status combinations, 20 above-cap calls, 10 at-cap calls,
  8 exact powers with arbitrary-size dyadic exponents and word-extreme powers.
  Class multiplication aliases z=x, z=y and z=x=y; inversion and both powers alias the input.
  Failure conditions: missed real endpoint, wrong status, changed refused output, alias mismatch,
  non-canonical result, failure to clamp prec below 2, or a false LIMIT at the inclusive cap.
  No failures in default and SAN=1 INV=1 runs.

- Local backends: 1715 cases, 22296 assertions, context blocks {8,9,5}.
  Compared division of identical global and local balls, aliased local numerators,
  hull replacement of local outputs, conversion statuses and context lifetime.
  Failure conditions: wrong status, different quotient, non-global result, invalid borrow lifetime.
  No failures in default and SAN=1 INV=1 runs.

- Tight-power cost: U(1), WORD_MIN -> 97 modulus bits; WORD_MAX -> 1 bit;
  897612484786617600 -> 556490 bits; 963761198400 -> 31468 bits; 720720 -> 789 bits.
  CPU times were 0.000850, 0.000080, 0.089283, 0.001223, 0.000018 seconds.
  No timeout or abort. These measurements are not a general time bound.

- Conservative content limit: r=2, k=22369622 returns LIMIT before forming the power.
  Its numerator would have 22369623 bits, its denominator 1 bit.
  The documented test instead uses 3k = 67108866 > 67108864.
  This refuses a result below the nominal bit cap, but follows the explicit header formula.
  I did not count it as a wrong status. The supplied test also actually forms (3/2)^(2^24) at the boundary.

- Memory: valgrind on edges and local found 0 errors and 0 bytes in use at exit.
  edges: 24103 allocations, 24103 frees, 2222312 allocated bytes.
  local: 12018 allocations, 12018 frees, 292992 allocated bytes.
  The successful sanitizer runs emitted 0 ASan or UBSan diagnostics.
  LeakSanitizer could not run under the sandbox's tracing setup; leak checks used valgrind.

- Tests that cannot fail: no tautological assertion was identified in the four supplied C tests or
  Python parts 3 and 4. Eleven changed-source mutants compiled and were killed by the supplied tests.
  This is a bounded fault check, not a proof that every assertion can detect every defect.
  F2 through F5 remain uncovered documentation claims. Existing cap tests do not count allocations for F1.

## Commands and results

All paths and commands below are relative to the repository root unless absolute.
No test timeout fired. The time limits were 180 seconds or less.

Builds:

    timeout 180 make -j2 BUILD=lanes/i-review2/build
    timeout 180 make -j2 BUILD=lanes/i-review2/build-san SAN=1 INV=1
    timeout 180 make -j2 BUILD=lanes/i-review2/build \
      lanes/i-review2/build/test_idclass lanes/i-review2/build/test_idele_maps \
      lanes/i-review2/build/test_idpow lanes/i-review2/build/test_idmap
    timeout 60 make -q BUILD=lanes/i-review2/build

Each returned 0. Logs: build.log, build-san.log, build-tests.log.
The final make -q check ran twice and returned 0 both times.

Default harness compilation used this command for each source listed:

    timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude \
      lanes/i-review2/STEM.c lanes/i-review2/build/libadelefeld.a \
      -lflint -lgmp -lm -o lanes/i-review2/EXE

STEM -> EXE:
probe -> probe; findings -> findings; edges -> edges; limit_alloc -> limit-alloc;
local -> local; cost -> cost; rationals -> rationals. Each final compilation returned 0.
probe.c was corrected before its first compilation to use an ordinary reset of its initialized output.

Sanitizer harness compilation used:

    timeout 60 cc -std=c11 -O1 -g -Wall -Wextra -Werror -DADF_CHECK_INVARIANTS \
      -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude \
      lanes/i-review2/STEM.c lanes/i-review2/build-san/libadelefeld.a \
      -lflint -lgmp -lm -o lanes/i-review2/EXE

STEM -> EXE:
edges -> edges-san; limit_alloc -> limit-alloc-san; probe -> probe-san;
local -> local-san; rationals -> rationals-san. Each final compilation returned 0.
The first default and sanitizer compilations of limit_alloc.c each failed with 6 type errors:
I had interchanged the calloc and realloc hook arguments. The following attempted launches each
returned 127 because no executable existed. The hook order was corrected before measurements.

Independent Python checks:

    timeout 180 python3 lanes/i-review2/review.py powers
    timeout 180 python3 lanes/i-review2/review.py real
    timeout 180 python3 lanes/i-review2/review.py maps
    timeout 180 python3 lanes/i-review2/review.py places
    timeout 120 python3 lanes/i-review2/invariance.py

Final results are the counts in the preceding section; each returned 0.
The first power run used only each prime's largest level and made 103288 local checks,
2016 distinct enumerations, 39743016 unit-residue visits, with 0 failures.
I enlarged coverage by lowering the level when necessary to keep lcm(N,T) <= 100000 for every N.
The final power run made the 884736 checks reported above.
The first two real-check runs returned 1 because my oracle used wrong numeric status constants.
It first expected 2 instead of NOT_DETERMINED=1, then expected 3 instead of NOT_UNIT=6.
Only those oracle constants were corrected; no enclosure or status condition was removed.

    ASAN_OPTIONS=detect_leaks=0 ADF_REVIEW_PROBE=lanes/i-review2/probe-san \
      timeout 180 python3 lanes/i-review2/review.py real
    ASAN_OPTIONS=detect_leaks=0 ADF_REVIEW_PROBE=lanes/i-review2/probe-san \
      timeout 180 python3 lanes/i-review2/review.py places

Both returned 0 with the same counts as their default runs.
Logs: powers.log, real.log, maps.log, places.log, invariance.log, real-san.log, places-san.log.

Standalone harness checks:

    timeout 30 lanes/i-review2/findings
    timeout 120 lanes/i-review2/edges
    timeout 60 lanes/i-review2/local
    timeout 120 lanes/i-review2/rationals
    timeout 30 lanes/i-review2/cost
    timeout 30 lanes/i-review2/limit-alloc
    ASAN_OPTIONS=detect_leaks=0 timeout 120 lanes/i-review2/edges-san
    ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/i-review2/local-san
    ASAN_OPTIONS=detect_leaks=0 timeout 120 lanes/i-review2/rationals-san
    ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/i-review2/limit-alloc-san

Each returned 0. Counts and reproducer output are above and in the corresponding EXE.log files.
An earlier "timeout 120 lanes/i-review2/edges-san" without ASAN_OPTIONS failed during LeakSanitizer
shutdown with its ptrace/tracing error. It was rerun with detect_leaks=0.
The initial failed-run text was overwritten by the successful edges-san.log.

    timeout 120 /home/tobias/.local/bin/valgrind --leak-check=full --show-leak-kinds=all \
      --errors-for-leak-kinds=definite,indirect --error-exitcode=99 lanes/i-review2/edges
    timeout 120 /home/tobias/.local/bin/valgrind --leak-check=full --show-leak-kinds=all \
      --errors-for-leak-kinds=definite,indirect --error-exitcode=99 lanes/i-review2/local

Both returned 0. Counts are above. Logs: valgrind-edges.log, valgrind-local.log.

Supplied C tests:

    timeout 120 lanes/i-review2/build/test_idclass
    timeout 120 lanes/i-review2/build/test_idele_maps
    timeout 120 lanes/i-review2/build/test_idpow
    timeout 120 lanes/i-review2/build/test_idmap

Respectively: 14 tests / 9738 checks; 10 / 10466; 6 / 10122; 6 / 54197.
Each returned 0 with 0 failed tests and 0 failed checks.
Logs: test_idclass.log, test_idele_maps.log, test_idpow.log, test_idmap.log.

Supplied Python checks:

    timeout 150 python3 proto/ideles_checks.py part3
    timeout 150 python3 proto/ideles_checks.py part4

part3: 1 check, 0 failed; 1500 ideles, 4600 class points, 350 NOT_DETERMINED,
2037 class products/inverses, 600 rationals, 207 exact norm results.
part4: 3 checks, 0 failed; 8996 coset/exponent table comparisons, 26988 centres,
224 enumerations, 3000 real powers, 1078 NOT_DETERMINED, 11532 idele-power points,
1680 hull cases, 9 additive-to-idele cases, 2499 quotients, 97 exact-unit divisions,
24 cases with a=M=0.
Both returned 0. Logs: part3.log, part4.log.

Mutation check:

    timeout 180 python3 lanes/i-review2/mutations.py

11 mutants, 11 killed, 0 survived, 0 compilation failures, 0 timeouts. Returned 0.
Each mutation's compile and link command ran under timeout 30; its test ran under timeout 35.
Exact commands are constructed in mutations.py. Each modified source copy is in mutants/.

| Mutation | Failed checks | Failed tests |
|---|---:|---:|
| class_norm | 1168 | 7 |
| class_product_unit | 551 | 3 |
| valuation_sum | 737 | 4 |
| norm_reciprocal | 642 | 4 |
| mul_rat_sign | 174 | 3 |
| tight_two_depth | 572 | 3 |
| power_sign | 214 | 3 |
| content_limit_edge | 2 | 1 |
| hull_two | 4235 | 4 |
| div_wrong_unit | 750 | 2 |
| conversion_certification | 37 | 2 |

Every mutated test executable returned 1. Evidence: mutations.log and mutants/*.log.
The changed check totals in two mutants reflect branches taken by the altered implementation.

## Files written

Sources and scripts:
probe.c, review.py, findings.c, edges.c, local.c, rationals.c, limit_alloc.c, cost.c,
invariance.py, mutations.py, report.md. All are under lanes/i-review2/.

Evidence and generated files:
source.sha256; the logs named above; build/ and build-san/ objects, dependencies and archives;
the four default test executables; probe, probe-san, findings, edges, edges-san, local, local-san,
rationals, rationals-san, limit-alloc, limit-alloc-san, cost; mutants/ source copies, objects,
executables and logs; __pycache__/ from importing the independent review transport.
I did not write the pre-existing brief.md, lane.log or stdout.log.

## Not done

- No implementation or contract repair: those paths are not owned by this lane.
- No full all-repository acceptance suite, Julia test or text/dump review.
- No second global oracle for huge M_k. Finite enumeration is capped at 100000.
  For large exponents, uninspected primes beyond 211 and higher local depths rely on the J proof.
- The random real sweep uses bounded dyadic exponents. The separate huge-exponent check covers exact powers,
  not every wide ball at those exponents.
- No known valid input reaches NOT_DETERMINED through a non-finite arb_div in adele/idele division.
  I did not manufacture an invalid divisor containing zero: that violates the idele input precondition.
- No LeakSanitizer leak result is claimed. Valgrind covered edges and local, not every enumeration run.
- The power-limit allocation-order measurement covers four public functions on one large valid idele,
  not every function accepting prec.
- No exhaustive mutation campaign. Eleven faults were selected and tested.

## Sources pending

None for the findings or the independent arithmetic/enumeration arguments.
No external source was cited from memory.

## Findings against the specification

F1 is an implementation violation of the literal allocation-order rule of SPEC 15.4, N-D8, in INV=1.
No counterexample to the mathematics of SPEC 4.5 or 5 was found in the checked cases.
F2 through F5 concern the public header or api-2 statements. They do not refute the enclosure formulas.
The norm endpoint pre-rounding in F2 is recorded explicitly against PLAN 2.2's "exact before rounding"
wording; Statement F documents that algorithm, and the finite rational factor remains exact.
The earlier reported PLAN 2.1 issue about N versus the normal modulus was not counted as a new finding.
