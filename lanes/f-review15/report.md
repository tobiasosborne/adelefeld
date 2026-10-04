# Lane f-review15: adversarial local zeta review

A dyadic pair (man, exp) means man*2^exp. Inputs below give real midpoint, imaginary midpoint, real radius, and
imaginary radius separately. All rectangles are closed. The tested source SHA256 is
e5efca95e7cb11c9c90139af41d62f79debb63bcb85286cad82ccb9f08f01dba. The installed C FLINT headers identify version
3.0.1.

## Findings

### F1. MAJOR: half of B passes the test but cuts off a true value

Fault: append mag_mul_2exp_si(B, B, -1) to the last multiplication in real_derivative_bound. test_localfactor
returns exit 0: 10 tests, 36725 checks, 0 failed. This is a coverage defect. The injected implementation would
violate the BLOCKER enclosure rule.

Smallest input found in the stated search family: place real; prec=16; midpoint (-1,-4) + i(0,0); radii (1,-8),
(0,0). The interval is [-17/256,-15/256]. The mutant returns OK with real midpoint (-34607,-10), radius
(68570759,-25), and imaginary midpoint and radius zero. The unchanged function returns OK with real midpoint
(-17373,-9), radius (301185087,-27), and contains the endpoint samples.

At the right endpoint s=-15/256, the independent 600-digit value is
-35.9251733059044577950542572008038016459547224312512608049836776537. It lies below the mutant lower endpoint by
0.0857070023174979561870697008038. The numerical margin is less than 10^-520 times the component scale. The
search tried 364 candidates in order: midpoint -2^-e for e=1..5; radius 2^-r for r=e+1..e+14; precision
2,4,8,16,32,64,128,256. This is a minimum within that ordering, not a global minimum.

The separate prec=256 witness, s=-2^-4 +/- 2^-14, misses 9 of 20 queried samples. Its mutated bound is
(120928953,-18); a sampled |L_inf'| exceeds it by a factor 1.1093715842849024110299305002. The unchanged bound
is (120928953,-17). The source of the value formula is refs/src/tate-poonen/notes.txt:1014-1016. The derivative
computation uses psi=Gamma'/Gamma from refs/src/flint-3.0.1/acb.rst:916-918.

Reason, step by step: the right endpoint belongs to the input interval; the returned dyadic interval has a
computable exact lower endpoint; the independent Gamma and power evaluation is smaller than that lower endpoint
by the displayed margin; therefore the returned rectangle misses an input image value. Details: minimized.json
and mutants/half-B-witness.json.

```text
printf '0 16 -1 -4 0 0 1 -8 0 0\n' | timeout 5 lanes/f-review15/build/bridge-half-B
```

### F2. MAJOR: lower Eplus passes the test but accepts a pole ball

Fault: replace the final E computation with scalar arb evaluation of the intended expression, then use
arb_get_mag_lower instead of an upper bound. test_localfactor returns exit 0: 10 tests, 36725 checks, 0 failed.
The injected implementation would violate the BLOCKER pole rule.

Input: p=2; prec=2; midpoint (1,0) + i(0,0); radii (1,0), (0,0). The mutant returns OK with real midpoint (1,1),
radius (916259693,33); imaginary midpoint (0,0), radius (610839795,32). The unchanged function returns
NOT_DETERMINED, leaves y untouched, and writes where=2. The same distinguishing input also works at prec=128.

Reason, step by step: the input is [0,2] on the real axis; zero is an endpoint; at zero the finite factor
denominator is 1-exp(0)=0; this is the simple pole of refs/src/tate-poonen/notes.txt:1733 continued by the
factor formula. Here the intended E is exp(-log 2)(exp(log 2)-1)=1/2. A lower rounded E need not reach the
denominator zero. OK is forbidden independently of the size of the finite output. Details: minimized.json and
mutants/lower-E-witness.json.

```text
printf '2 2 1 0 0 0 1 0 0 0\n' | timeout 5 lanes/f-review15/build/bridge-lower-E
```

### F3. MAJOR: an open integer test passes but changes the pole status

Fault: replace arf_cmp(h, lo) >= 0 by > 0 in the real pole test. test_localfactor returns exit 0: 10 tests,
36725 checks, 0 failed.

Input: place real; prec=2; midpoint (-255,-1) + i(0,0); radii (1,-1), (0,0). The input is [-128,-127]. The
unchanged function returns NOT_DETERMINED; the mutant returns LIMIT. Both leave the exact y representation
untouched and write where=real. The original larger witness [-200,-199] at prec=64 has the same difference.

Reason, step by step: S/2=[-64,-63.5]; floor(hi)=-64 equals lo; the closed test must accept that integer. Gamma
has its pole at -64, hence L_inf has a pole at -128 (refs/src/tate-poonen/notes.txt:62-64,1014-1016). The open
test misses the endpoint; direct Gamma is not finite; shift 65 exceeds the fallback cap and gives LIMIT. The
contract requires the pole test to give NOT_DETERMINED first. Details: minimized.json.

```text
printf '0 2 -255 -1 0 0 1 -1 0 0\n' | timeout 5 lanes/f-review15/build/bridge-open-integer
```

### F4 and F5. MINOR: two further bound mutations remain unclassified

Omitting +log(pi) in B and omitting the multiplication by M0 both pass all 36725 checks. Their bounds differ
from the declared Z6 computation. Eighteen additional thin boxes per fault were checked at 600 digits, with 20
samples and derivative comparisons per box. No wrong status, image enclosure, or sampled derivative bound was
found for these two faults. Their semantic equivalence or global soundness is unresolved; they are not claimed
as MAJOR enclosure defects. This is the same unchecked quantity identified by the previous lane report.

Exact distinguishing trace inputs and bound values are in witnesses.json. The first input in that family is real
s=-2^-2 +/- 2^-12, imaginary part exactly zero, prec=256. Both faults and the unchanged function return OK
there. The direct trace distinguishes the computed B; it does not prove a violated upper bound. Reproducer:
witnesses.py after mutations.py.

On that input the unchanged B is (400306253,-22); the bound without log(pi) is (45178009,-19); the bound without
M0 is (557331365,-23). Both faulty versions return the same finite y as the original: real midpoint
(-22649151191588155,-51), radius (839531535,-36), imaginary part exactly zero. Their sampled derivative ratios
are 0.3538989555 and 0.4590001913 respectively. No sample distinguishes a wrong value here.

## Fault table

| Fault | Test exit | Failed checks | Detection |
|---|---:|---:|---|
| half-B | 0 | 0 | survives |
| no-enlargement | 1 | 252 | assertions |
| lower-E | 0 | 0 | survives |
| open-integer | 0 | 0 | survives |
| fallback-65 | 1 | 3 | assertions |
| inward-round | 1 | 82 | assertions |
| wrong-sign | -11 | not completed | assertions, SIGSEGV |
| lowprec-log-no-radius | -11 | not completed | assertions, SIGSEGV |
| max-radius | 1 | 94 | assertions |
| no-logpi-B | 0 | 0 | survives |
| no-M0-B | 0 | 0 | survives |
| drop-pi | 1 | 518 | assertions |
| wrong-Gamma-scale | 1 | 617 | assertions |

All 13 faults compiled. Eight were detected and five survived. The two crashing runs emitted failed assertions
before SIGSEGV; their complete failed-check totals are unavailable. The inward rounding fault is killed by p=2,
s=2, prec=2. The n=65 fault is killed by the existing recurrence LIMIT check. The wrong-sign and
low-precision-log faults are killed by the prime pole lattice. Full first assertions and commands are retained
in mutations.log and mutants/*.log. No test was weakened or edited.

## Independent checks and their failure conditions

The primary value oracle is mpmath 1.3.0, at 480 or more decimal digits; the dense special check uses 420 digits
and the fault witnesses use 600. It uses mpmath.gamma and mpmath.power, not proto/zeta_checks.py, acb_gamma, or
acb_pow as a reference. The finite integer checks use exact Fraction arithmetic. The repository test itself
retains its own existing FLINT references.

The stated numerical margin is 10^(-dps+80) times a component scale: the maximum of the component magnitude,
|L|*10^(-dps+80), and 10^-dps. A failed containment check is evaluated again with 100 extra digits. This is a
numerical margin, not an independently certified mpmath error guarantee. The 4000-bit scalar pole certificates
and exact rational checks are separate from these approximate value comparisons. The scalar enclosure contract
is refs/src/flint-3.0.1/arb.rst:6-12.

Cases are bridge records. Exact-value, pole, status, and recurrence records make three public calls: separate y,
y=s, and where=NULL. Non-OK representation checks compare arb_dump_str for each component. Every OK must be
finite; where must stay unchanged. Every failure must preserve y and set where=v.

| Place | Random records | Distinct boxes | OK | ND | LIMIT | Value samples |
|---|---:|---:|---:|---:|---:|---:|
| 2 | 5000 | 5000 | 4355 | 645 | 0 | 87100 |
| 3 | 5000 | 5000 | 4344 | 656 | 0 | 86880 |
| 5 | 5000 | 5000 | 4314 | 686 | 0 | 86280 |
| 7 | 5000 | 5000 | 4362 | 638 | 0 | 87240 |
| 65537 | 5000 | 5000 | 4266 | 734 | 0 | 85320 |
| 18446744073709551557 | 5000 | 5000 | 4237 | 763 | 0 | 84740 |
| real | 5111 | 5000 | 4551 | 526 | 34 | 91020 |

Every random record checks 20 queried points on OK: corners, edge midpoints, the centre, and dyadic interior
points. Degenerate rectangles necessarily repeat some points. Midpoint exponents range from -200 to 200; both
signs occur; effective radii range from zero to less than one; requested precisions include 2 and 4096. A case
fails for an unexpected status where one is prescribed, an OK pole ball, a non-finite output, a changed failure
representation, an incorrect where, or a sampled value outside the stated margin.

Finite integer values: 288 records; 288 OK, 0 NOT_DETERMINED, 0 DOMAIN, 0 LIMIT; 288 value samples; 0 failures.

Prime boundary boxes: 978 records; 61 OK, 917 NOT_DETERMINED, 0 DOMAIN, 0 LIMIT; 1220 value samples; 0 failures.

Real pole boxes: 1530 records; 4 OK, 1024 NOT_DETERMINED, 170 DOMAIN, 332 LIMIT; 80 value samples; 0 failures.

Near-pole boxes: 2000 records; 1362 OK, 638 NOT_DETERMINED, 0 DOMAIN, 0 LIMIT; 27240 value samples; 0 failures.

The 288 finite integer cases check p^s/(p^s-1) exactly for s in -257,-65,-16,-3,-2,-1,1,2,3,16,65,257 at all six
primes and precisions 2,64,512,4096.

Prime boundary cases use k=0,+/-1,2,999999,+/-1000000,2^100,2^1000 and 23 seeded indices per prime. Midpoints
have 2200 bits and are offset by five ulps. Radii are two 30-bit radius units above or below the distance. Thin
segments, real intervals reaching zero, and enormous-radius boxes are included. Separate 4000-bit scalar
arithmetic certifies 594 memberships and 384 exclusions, with 0 failures.

Real poles use 170 indices, including 0,1,2,63,64,65,100,999999,1000000,2^200. Cases include exact poles, both
thin directions, closed endpoint contacts, intervals between poles with width almost two, imaginary intervals
one radius ulp away from zero, and -2n+2^-5000. Membership is checked with exact Fraction arithmetic. The
near-pole sweep uses radii d/2,d/16,d/1000 and 200 real plus 300 records per prime.

Recurrence: 201 records, 194 records with a fallback, 7 OK, 140 samples, 0 failures. The shift ranges from 1 to
65; precisions are 16,64,256, with selected 4096-bit cases. n<=64 must not produce LIMIT; the n=65 boxes must
produce LIMIT after pole exclusion.

Derivative checks: 1200 samples on 60 random real boxes; 2430 samples on 30 dense random boxes; 1782 samples on
22 dense special OK boxes. Total 5412 completed comparisons, 0 instances of |L_inf'|>B. The special family
includes poles 0,-2,-16,-62,-124, distances 2^-4,2^-20,2^-100, thin boxes, and wide left and positive boxes. The
grid is 9 by 9. The largest completed sampled ratio is 0.6666666505237424308893861841. A sampled derivative
above B would be a finding even if the final output contained all value samples. Sampling gives a lower bound on
the supremum, not its exact value.

Derivative calculation, step by step: differentiate exp(-s log(pi)/2) to obtain -log(pi)/2 times that prefactor;
differentiate Gamma(s/2) to obtain Gamma'(s/2)/2; apply the product rule; substitute psi=Gamma'/Gamma. This
gives L_inf'=L_inf*(psi(s/2)-log(pi))/2. The bound trace stores B before multiplication by R.

Statuses: 48 records, 144 public calls, 0 failures. Non-finite midpoint parts cover NaN, +infinity and
-infinity; either radius can be infinite. mag radii have no NaN state. The precision cap wins first, including
LONG_MAX and non-finite inputs; LONG_MIN and precisions below two are clamped; exact -200 is DOMAIN; a ball
around -200 is ND. All four documented statuses occurred. The handle program adds 13 checks, 0 failures.
Composites are refused by the constructor without modifying it; a forged handle violates the precondition and
aborts under ADF_CHECK_INVARIANTS; cap+1 still wins first.

Cost: 36 matrix calls and one initial enormous-exponent pilot, 0 timeouts. At real s=1, times at
64,4096,65536,1048576 bits are 0.000141,0.000776,0.028712,1.294944 s. At p=2 they are
0.000090,0.000472,0.021146,0.876199 s. The matrix includes Re(s)=+/-2^1000 with Im(s)=1, Im(s)=+/-2^1000 with
Re(s)=1, and exponents +/-2^60 in both components. All return within the 60 s threshold and preserve failure
outputs. Full inputs, statuses and timings are in cost-*.json.

Memory: bridge.c and cost.c compiled with -fsanitize=address,undefined and -fno-sanitize-recover=undefined.
localfactor.c and place.c are instrumented; the system FLINT library and unrelated archive objects are not. With
leak detection off, the bridge completes 48 status, 978 prime-pole and 1530 real-pole records, with 1300 value
samples and 0 failures; cost completes three calls. No address or undefined-behaviour report is emitted.
Leak-enabled exits fail because LeakSanitizer cannot operate under the sandbox ptrace environment. Leaks were
not tested.

Driver: 30 hostile command lines, 30 expected lines, 0 differences; driver exit 1 for the file containing
errors. Ten printed lines are compared with a separate direct library caller. Other kinds of operand, real
adeles, invalid places, 4, 2^64, missing operands, syntax precedence, and radii in only one component are
included.

## Commands and results

All commands ran from the repository root. The only archive build used the command below; test target
construction did not rebuild the archive. No whole repository suite, git command, or bd command was run.

```text
timeout 600 make -j2 BUILD=lanes/f-review15/build lanes/f-review15/build/libadelefeld.a
timeout 10 python3 lanes/f-review15/setup.py
timeout 60 make -j2 BUILD=lanes/f-review15/build lanes/f-review15/build/test_localfactor
timeout 30 lanes/f-review15/build/test_localfactor
```

Results: archive exit 0; setup exit 0, two trace insertion sites; test build exit 0; baseline exit 0, 10 tests,
36725 checks, 0 failed. Logs: build.log, test-build.log, baseline.log.

Manual C compilation used timeout 30 cc -std=c11 -O2 -g -Iinclude. All nine executables compiled with exit 0:
bridge, bridge-base, cost, adf, driver-library, handles, bridge-san, cost-san, pole-geometry. bridge adds -Isrc
-DREVIEW_TRACE; handles adds -Isrc -DADF_CHECK_INVARIANTS and src/localfactor.c src/place.c. Both sanitizer
builds use -O1 -Isrc -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined;
bridge-san adds src/place.c, cost-san adds src/localfactor.c src/place.c. Each links the lane archive and
-lflint -lgmp -lm. adf uses tools/adf/adf.c; bridge-base and bridge-san use bridge.c; cost-san uses cost.c;
driver-library uses driver_library.c; pole-geometry uses pole_geometry.c. Other names match their lane C files.
The plain bridge-base links the archive without the trace include; it was compiled but not run.

Completed oracle commands follow. Each listed JSON retains status counts, samples, seconds, and the bridge exit
result. All listed commands exit 0. Near and random records test the failure conditions above; their counts are
not added to the baseline repository check count.

```text
timeout 170 python3 lanes/f-review15/oracle.py bounds --start 0 --count 10 --tag dense-random-0
```

dense-random-0.json: 10 records, 810 value samples, 810 derivative samples, 31.461 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py bounds --start 10 --count 10 --tag dense-random-10
```

dense-random-10.json: 10 records, 810 value samples, 810 derivative samples, 31.581 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py bounds --start 20 --count 10 --tag dense-random-20
```

dense-random-20.json: 10 records, 810 value samples, 810 derivative samples, 31.647 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py exact
```

exact-0-0.json: 288 records, 288 value samples, 0 derivative samples, 0.37 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py finite-poles
```

finite-poles-0-0.json: 978 records, 1220 value samples, 0 derivative samples, 9.882 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py near --p 0 --start 0 --count 100
```

near-0-0.json: 100 records, 1400 value samples, 0 derivative samples, 141.734 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py near --p 0 --start 100 --count 100
```

near-0-100.json: 100 records, 1380 value samples, 0 derivative samples, 97.226 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py near --p 18446744073709551557 --start 0 --count 300
```

near-18446744073709551557-0.json: 300 records, 4160 value samples, 0 derivative samples, 17.464 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py near --p 2 --start 0 --count 300
```

near-2-0.json: 300 records, 4000 value samples, 0 derivative samples, 8.784 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py near --p 3 --start 0 --count 300
```

near-3-0.json: 300 records, 4120 value samples, 0 derivative samples, 16.588 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py near --p 5 --start 0 --count 300
```

near-5-0.json: 300 records, 4140 value samples, 0 derivative samples, 15.731 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py near --p 65537 --start 0 --count 300
```

near-65537-0.json: 300 records, 4100 value samples, 0 derivative samples, 17.208 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py near --p 7 --start 0 --count 300
```

near-7-0.json: 300 records, 3940 value samples, 0 derivative samples, 16.552 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 0 --count 100 --derivatives
```

random-0-0.json: 100 records, 1680 value samples, 1200 derivative samples, 77.44 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 100 --count 50
```

random-0-100.json: 50 records, 840 value samples, 0 derivative samples, 10.791 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 1150 --count 500
```

random-0-1150.json: 500 records, 8980 value samples, 0 derivative samples, 71.664 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 150 --count 500
```

random-0-150.json: 500 records, 8720 value samples, 0 derivative samples, 87.29 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 1650 --count 500
```

random-0-1650.json: 500 records, 9060 value samples, 0 derivative samples, 67.407 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 2150 --count 500
```

random-0-2150.json: 500 records, 8860 value samples, 0 derivative samples, 49.518 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 2650 --count 500
```

random-0-2650.json: 500 records, 9160 value samples, 0 derivative samples, 53.705 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 3150 --count 500
```

random-0-3150.json: 500 records, 8780 value samples, 0 derivative samples, 57.396 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 3650 --count 500
```

random-0-3650.json: 500 records, 8840 value samples, 0 derivative samples, 116.113 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 4150 --count 500
```

random-0-4150.json: 500 records, 8840 value samples, 0 derivative samples, 74.947 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 4650 --count 350
```

random-0-4650.json: 350 records, 6200 value samples, 0 derivative samples, 45.624 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 5000 --count 111
```

random-0-5000.json: 111 records, 2040 value samples, 0 derivative samples, 15.177 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 0 --start 650 --count 500
```

random-0-650.json: 500 records, 9020 value samples, 0 derivative samples, 60.727 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 18446744073709551557 --start 0 --count 5000
```

random-18446744073709551557-0.json: 5000 records, 84740 value samples, 0 derivative samples, 82.528 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 2 --start 0 --count 250
```

random-2-0.json: 250 records, 4380 value samples, 0 derivative samples, 3.423 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 2 --start 250 --count 4750
```

random-2-250.json: 4750 records, 82720 value samples, 0 derivative samples, 59.17 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 3 --start 0 --count 5000
```

random-3-0.json: 5000 records, 86880 value samples, 0 derivative samples, 81.419 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 5 --start 0 --count 5000
```

random-5-0.json: 5000 records, 86280 value samples, 0 derivative samples, 62.824 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 65537 --start 0 --count 5000
```

random-65537-0.json: 5000 records, 85320 value samples, 0 derivative samples, 68.278 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py random --p 7 --start 0 --count 5000
```

random-7-0.json: 5000 records, 87240 value samples, 0 derivative samples, 58.232 s, exit 0.

```text
timeout 170 python3 lanes/f-review15/oracle.py real-poles
```

real-poles-0-0.json: 1530 records, 80 value samples, 0 derivative samples, 9.655 s, exit 0.

```text
ASAN_OPTIONS=detect_leaks=0 timeout 170 python3 lanes/f-review15/oracle.py finite-poles --exe \
  bridge-san --tag san-finite-poles
```

san-finite-poles.json: 978 records, 1220 value samples, 0 derivative samples, 11.096 s, exit 0.

```text
ASAN_OPTIONS=detect_leaks=0 timeout 170 python3 lanes/f-review15/oracle.py real-poles --exe bridge-san \
  --tag san-real-poles
```

san-real-poles.json: 1530 records, 80 value samples, 0 derivative samples, 12.989 s, exit 0.

```text
timeout 30 python3 lanes/f-review15/statuses.py
timeout 170 python3 lanes/f-review15/dense.py
timeout 60 python3 lanes/f-review15/pole_geometry.py
timeout 170 python3 lanes/f-review15/recurrence.py
timeout 60 python3 lanes/f-review15/driver_checks.py
ulimit -c 0
timeout 30 lanes/f-review15/build/handles
timeout 170 python3 lanes/f-review15/mutations.py
timeout 170 python3 lanes/f-review15/witnesses.py
timeout 170 python3 lanes/f-review15/minimize.py
```

Results, in order: 48/0 failures; 24 cases and 1782/0 sample failures; 978/0 membership failures; 201 cases and
140/0 sample failures; 30/0 differences; 13/0 handle failures; 13 compiled faults, eight detected and five
surviving; 42 targeted witness records with three distinguished faults; 366 minimisation candidate inputs with
three distinguished faults. The witness command ran twice: the second run fixes the placement of the half-B
trace and reproduces the same enclosure failure.

```text
timeout 170 python3 lanes/f-review15/cost_probes.py precision --place 0
timeout 170 python3 lanes/f-review15/cost_probes.py precision --place 2
timeout 170 python3 lanes/f-review15/cost_probes.py huge
timeout 170 python3 lanes/f-review15/cost_probes.py tiny
timeout 170 python3 lanes/f-review15/cost_probes.py huge-exponent
timeout 60 lanes/f-review15/build/cost 0 64 real-imag 1152921504606846976 1
```

Results: 4,4,8,12,8 and 1 calls respectively; all exit 0; 0 timeouts. Each matrix child uses timeout 60.
Detailed commands are in cost-*.json.

```text
ASAN_OPTIONS=detect_leaks=0 REVIEW_BRIDGE=bridge-san \
  timeout 30 python3 lanes/f-review15/statuses.py
ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/f-review15/build/cost-san 2 4096 real 0 1
ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/f-review15/build/cost-san 0 4096 real 0 1
ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/f-review15/build/cost-san \
  0 64 real-imag 1152921504606846976 -1
```

Results: 48/0 status failures; three cost exits 0; 0 ASan or UBSan reports. The same four commands with
detect_leaks=1 were attempted first: all exit 1 at LeakSanitizer shutdown because ptrace is unavailable. The
first status run reports 48 records and 0 output errors before the shutdown failure. Logs retain both attempts.

## Incomplete or corrected checks

The first finite-poles oracle run exited 1 with six false findings. Its zero-pole generator left exponent zero
after moving the mantissa, so it tested Im(s)=5 and labelled it as zero. The corrected run has 978 records and 0
findings. The first scalar membership encoding exited 1 with 943 parsed records and 581 failures because a
negative k was split out of a hyphenated ID incorrectly. An explicit pole_index field fixes the encoder; the
rerun certifies all 978 records.

The initial timeout 170 python3 lanes/f-review15/oracle.py bounds --count 50 run stopped with BrokenPipeError
after the child bridge reached 165 s. Its last checkpoint records 30 completed boxes, 2430 value samples, 2430
derivative samples and 0 failures at 159.478 s. It was rerun as three completed ten-box batches, recorded above.
No library call was found to take 60 s.

The three 10 s import/count probes exit 0: mpmath version 1.3.0; the first real random audit counts 5000 records
and 4889 encoded boxes; after 111 new records it counts 5111 records and 5000 encoded boxes. finalize.py
independently recomputes 5000 distinct boxes at every place and checks the unchanged source hash.

```text
timeout 10 python3 -m py_compile lanes/f-review15/finalize.py
timeout 10 python3 lanes/f-review15/finalize.py
```

The syntax check exits 0. A separate 10 s Python evidence audit counts 36 completed oracle JSON files, 0
completed failures, 0 progress lines over 116 characters, and 0 executable files outside build. finalize.py
repeats the evidence checks, validates the report line lengths, removes only owned scratch products, and writes
this report once. Audit results are retained in audit.json.

For a short interval three oracle subprocesses ran concurrently. That violated the two-core scheduling
instruction. It is recorded in progress.md. Later scheduling kept at most two workers; oracle.py also confines
later direct workers to the same two allowed CPUs. Every test program or script had a timeout of at most 170 s;
the one archive build used the explicitly prescribed 600 s exception.

## Files written and reproduction

Read CLAUDE.md, SPEC 9.3.7, PLAN 1F.9, the header contract, src/localfactor.c, Y16-Y17, design Z1-Z9, the
conventions pole rule and status row, the previous lane report, the localfactor C and Julia tests, and the
driver command and documentation. Implemented only review programs and fault generators; no file outside this
lane was changed.

Only lanes/f-review15 was written. Source and workflow files: bridge.c, cost.c, cost_probes.py, dense.py,
driver_library.c, driver_checks.py, handles.c, minimize.py, mutations.py, oracle.py, pole_geometry.c,
pole_geometry.py, recurrence.py, reproduce.sh, setup.py, statuses.py, finalize.py, progress.md, report.md.
Evidence includes runs/*.json, mutants/*.json and *.log, cost-*.json, dense*.json, recurrence*.json,
witnesses.json, minimized.json, statuses.json, pole-geometry.inputs, driver.cmd, driver.out, driver.expected,
the build, sanitizer and check logs, source.sha256, distinct.log, audit.json and written-files.txt. The latter
is the exact retained inventory.

The scratch build directory, archive, objects, binaries, generated C mutants, and Python bytecode cache were
deleted. All source generators and exact witness outputs remain. No retained file created by this lane has an
executable permission bit.

To recreate the findings after cleanup, run the following from the repository root. The individual commands in
Findings require that reconstruction first. reproduce.sh was written as a command recipe; it was not run as a
whole during this review.

```text
timeout 170 sh lanes/f-review15/reproduce.sh
```

## Not done

No proof of the global image enclosure or exact derivative supremum was attempted. Numerical sampling cannot
exclude an unsampled counterexample. No certified independent Gamma-integral enclosure was built, and no mpmath
correct-rounding guarantee is claimed. The two unclassified B survivors have no demonstrated wrong enclosure. No
leak check completed. The system FLINT shared library was not rebuilt with sanitizers. No Julia program,
all-places operation, adaptive subdivision, full repository suite, or full mutation sweep was run. No production
code, driver code, test or document outside the lane was repaired, since it is read-only.

## Sources pending

[source pending: Gelfond-Schneider theorem in refs/ for the classification of nonzero exact dyadic finite
poles.] The code does not need this classification to reject an undecided denominator.

[source pending: FLINT 3.0.1 acb/arb exponential and Gamma evaluator source and range guards.] The code was
measured as installed; its evaluator internals were not proved.

[source pending: rigorous mpmath 1.3.0 gamma, power and digamma numerical error guarantees.] The point oracle
uses the stated numerical margin; this is not a certificate of all rounding errors.

## Findings against the specification

None established. F1-F3 are tests that admit injected contract violations, while the unchanged implementation
passes their distinguishing inputs. F4-F5 have no demonstrated contract violation. The live conventions status
row already includes LIMIT; the older lane report and Y17 sentence saying that it is pending are stale.
