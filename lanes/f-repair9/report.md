# Lane f-repair9: local-factor test repair

Part 1 is done. F4 has verdict (b): log(pi) is not needed in the derivative certificate.
The proof and its production consequence are in Y16 and src/localfactor.c.
F5 is NOT DONE: neither a certified missed-value witness nor a global proof was obtained.
M0 remains in production. The report does not classify F5 as equivalent.

## Files written

- tests/test_localfactor.c: three regression families and guards after failed assertions.
- tests/ref/vectors/f-repair9/near-poles.jsonl: 275467 bytes, 6728 records, 1346 boxes, 5382 references.
- src/localfactor.c: the proved Bsmall certificate and the representation containment guard described below.
- docs/api-1f9.md, Y16/Y17 only: the numbered proof, checks, guard and corrected LIMIT documentation.
- Lane sources: fixtures.py, crosscheck.py, active_check.py, mutations.py, bridge.c, constants.c,
  bound_check.c, compare.c, search.c, search_setup.py, run_test.py, finalize.py and f5-analysis.md.
- Lane evidence: the saved original old-localfactor.c, bsmall-before-rounding-guard.c, logs,
  mutants/*.json and *.log, check-*.json, crosscheck.json, audit.json and this report.

mutations.py and bridge.c were copied from f-review15. The mutation generator now reads old-localfactor.c,
so the thirteen faults remain exactly the review's faults after the production F4 change.
The saved original SHA256 is e5efca95e7cb11c9c90139af41d62f79debb63bcb85286cad82ccb9f08f01dba.
No specification, header, driver, q-slice1 file, git state or tracker was changed.

## F1-F3: red, then green

F1 box 0 is [-17/256,-15/256]. Its right endpoint is -15/256.
The certified cell for its value contains -35.92517330590445779505... .
The half-B mutant fails `acb_contains(y, ref)` for box 0, sample 2, prec=16.
Box 1 is -2^-4 +/- 2^-14. The same assertion fails for sample 2 at prec=256.
Both assertions are independently checked in finalize.py against the retained mutant log.

The additional family is the full product of poles 0,-2,-4,-16, d=2^-e for e=1..12,
rx=d/2^k for k=1..14, and two shapes. Real intervals have midpoint pole-d and imaginary radius zero.
Complex boxes have midpoint pole+d, imaginary midpoint zero and ry=rx/16.
Each midpoint has distance exactly d from its pole. Every box runs at prec 16,32,64,128,256.
There are 1344 family boxes plus the two review witnesses: 6730 primary calls and 26910 sample checks.
Each primary call also checks separate output, aliasing and where=NULL through call().

The exporter asserts n<=64 and max Re(Z+n)<=64. The actual C trace confirms 6730 OK calls and
6730 finite derivative-bound traces, with 0 failures: the refinement is active on every family call.
Real samples are both endpoints and the midpoint. Complex samples are four corners and the midpoint.

References come from proto/zeta_checks.py point_enclosure, the Z8 integrated Taylor polynomial,
at 2048 bits, with T=512, degree 2200, proved polynomial and infinite tails, recurrence and scalar arithmetic.
There is no acb_gamma, acb_pow or call to the library under test in this exporter.
Each scalar enclosure is wholly contained in a dyadic cell [k*2^e,(k+1)*2^e] with 44 significant bits.
The exporter asserts that containment; the C test checks the whole cell without a fallback Gamma reference.
null is the certified exact zero imaginary component at a real reference point.
Each input is followed by its three or five reference records. The longest line is 51 characters.

The twenty 400-digit mpmath cross-checks are (box,sample):
(0,2), (1,2), (2,0), (3,4), (28,2), (29,0), (114,0), (115,3), (338,2), (339,0),
(450,0), (451,4), (674,2), (675,0), (898,0), (899,4), (1010,0), (1011,4), (1344,2), (1345,0).
All twenty values lie strictly inside their nonzero component cells; real imaginary parts are exactly zero.
crosscheck.json retains the exact inputs and 80-digit values. This is a numerical consistency check;
the enclosure certificate comes from Z8, without a claimed mpmath rounding guarantee.

F2 tests every prime 2,3,5,7,65537,2^64-59, radius 2^e for e=-10..3 and prec 2,16,64,128,256.
There are two real segments and four complex corner contacts with zero at each parameter pair.
All 2520 calls must be NOT_DETERMINED. In particular [0,2] at p=2, prec=2 and 128 fails the mutant.
Its first failed assertion is `st == ADF_NOT_DETERMINED`, p=2, radius=2^-10, left segment, prec=2.
The green helper checks untouched output and where=p in all failure modes.

F3 tests n=1,2,33,64,65,100,1000, widths 2^-20,1/2,1, both closed endpoint orientations,
and prec 2,16,64,128,256. All 210 calls must be NOT_DETERMINED, before any recurrence limit.
The mutant first fails at n=64, width=2^-20, right side, prec=2, returning status 10 (LIMIT).
The retained log also contains [-128,-127] at prec=2 and [-200,-199] at prec=64 returning LIMIT.
The library returns NOT_DETERMINED with untouched y and where=real through all three helper modes.

The original test crashes were reproduced: wrong-sign and lowprec-log-no-radius both exited -11.
run_row recorded the wrong OK on a non-OK fixture, then read its absent value, point_error and samples.
It now reads references only when both actual and expected statuses are OK and y is finite.
The failed status assertion remains. call_mode also guards midpoint-bit reads after its finiteness assertion.
Both faults now finish with complete counts and exit 1. No assertion or permitted status was weakened.

## F4: verdict (b)

Y16 proves |Gamma'(w)-log(pi) Gamma(w)|<=M1 for the whole admitted positive strip.
The proof bounds J(a)=integral t^(a-1) exp(-t) |log(t/pi)| dt.
It proves J(1)<179/90<2, J(2)<2, J(c)<=c! for integers c>=2, then uses convexity between them.
Differentiating the recurrence gives
Bsmall = pi^(-min Re(S)/2) [M1+M0 sum 1/delta_j]/(2 P).
All steps, including three certified constant inequalities, are written in Y16.

The bound computation changed from (400306253,-22) to (45178009,-19) on the review's thin -1/4 box.
bound_check.c first returned exit 1 with 1 failed check against the old computation, then exit 0 with
1 check and 0 failures against Bsmall. constants.c has 3 strict scalar comparisons and 0 failures.

The first old/new comparison had equal statuses on 20000 seeded boxes, but 59 stored new balls were not
contained in the stored old balls. Nested mathematical intersections can lose nesting when converted to
arb's midpoint and 30-bit radius representation and rounded outward. This was not hidden or waived.
compare-rounding-failures.log retains all 59 cases; bsmall-before-rounding-guard.c retains that implementation.

Production now also forms the previous rounded certificate. If the new rounded ball is not contained in it,
production keeps the previous ball. Both certificates are sound. There is no additional Gamma evaluation.
Thus log(pi) remains computed in this representation guard, although Bsmall does not need it for soundness.
The repeated comparison returns exit 0: 20000 boxes, 17571 OK, 2294 NOT_DETERMINED, 135 LIMIT,
15229 equal OK balls, 2342 strictly contained OK balls, 0 status or containment failures.
All Part 1 references remain contained. No M0 simplification was made.

## F5: search and what is not done

f5-analysis.md proves soundness of the ORIGINAL no-M0 mutant when n=0 and when Re(Z+n) stays in [1,2].
The suggested small positive boxes near s=2.9232 and s=40 have n=0, so they cannot give the requested witness.
Thin near-pole boxes often satisfy the second proved case. This does not settle wider shifted boxes.

search.c searches near 63 poles within the shift cap, positive boxes and two wide complex-box families.
It uses dyadic inputs and 384-bit Gamma/digamma point evaluations as candidate finders only.
For each OK it checks the midpoint and four corners for a disjoint value or a derivative lower bound above B.
It records whether the midpoint enlargement has a smaller component radius than the first candidate.
This prevents reporting a changed representation as evidence that the smaller enclosure was active.

| Seed | Boxes | OK | ND | Active B | Tighter | Samples | Value misses | Derivative exceedances |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 9041 | 20000 | 18916 | 1084 | 18916 | 3994 | 94580 | 0 | 0 |
| 9042 | 20000 | 18925 | 1075 | 18925 | 3897 | 94625 | 0 | 0 |

Both have 0 LIMIT statuses and exit 0. In the unproved region n>0, ceil(B0)>2, the first seed has
1540 active boxes and 555 tighter midpoint enclosures; the second has 1519 and 535 respectively.
Largest recorded derivative/B ratios are (1073741800,-30) and (1073741805,-30), dyadic pairs as in the review.
The earlier 9041 search used the same boxes and samples; its initial changed counter counted representation
changes, not radius improvements. search-tighter-9041.log corrects that count to 3994.
Sampling is not a global proof. No F5 witness was certified or added. The general n>0, ceil(B0)>2 case
remains NOT DONE. Production retains M0. No global equivalence or soundness claim is made for this fault.

## Fault table before and after

Before counts are those of f-review15, whose source hash matches the saved original.
After counts are from fault-table.log against the final test and final reference file.
All thirteen compile with exit 0. Eleven finish with exit 1; two finish with exit 0; none crashes.

| Fault | Before exit | Before failed | After exit | After failed | Result |
|---|---:|---:|---:|---:|---|
| half-B | 0 | 0 | 1 | 4036 | F1 detected |
| no-enlargement | 1 | 252 | 1 | 24470 | remains detected |
| lower-E | 0 | 0 | 1 | 840 | F2 detected |
| open-integer | 0 | 0 | 1 | 60 | F3 detected |
| fallback-65 | 1 | 3 | 1 | 3 | remains detected |
| inward-round | 1 | 82 | 1 | 82 | remains detected |
| wrong-sign | -11 | unknown | 1 | 1816 | count completes |
| lowprec-log-no-radius | -11 | unknown | 1 | 235 | count completes |
| max-radius | 1 | 94 | 1 | 1614 | remains detected |
| no-logpi-B | 0 | 0 | 0 | 0 | F4 proved sound, (b) |
| no-M0-B | 0 | 0 | 0 | 0 | F5 NOT DONE |
| drop-pi | 1 | 518 | 1 | 24356 | remains detected |
| wrong-Gamma-scale | 1 | 617 | 1 | 26403 | remains detected |

## Commands and results

Commands ran from the repository root. Every test or Python script had a timeout at most 170 seconds.
Manual compilation used timeout 30. Child tests of mutations.py use timeout 20.
After the early overlap noted below, commands were pinned with taskset -c 0,1; make used at most -j2.
The following commands give the final evidence; build targets are under this lane.

```text
timeout 170 make -j2 BUILD=lanes/f-repair9/build lanes/f-repair9/build/test_localfactor
taskset -c 0,1 timeout 40 python3 lanes/f-repair9/run_test.py plain
taskset -c 0,1 timeout 170 make -j2 BUILD=lanes/f-repair9/build-san SAN=1 \
  lanes/f-repair9/build-san/test_localfactor
taskset -c 0,1 timeout 40 python3 lanes/f-repair9/run_test.py san
taskset -c 0,1 timeout 170 make -j2 BUILD=lanes/f-repair9/build-inv INV=1 \
  lanes/f-repair9/build-inv/test_localfactor
taskset -c 0,1 timeout 40 python3 lanes/f-repair9/run_test.py inv
taskset -c 0,1 timeout 170 make -j2 BUILD=lanes/f-repair9/build-clang CC=clang \
  lanes/f-repair9/build-clang/test_localfactor
taskset -c 0,1 timeout 40 python3 lanes/f-repair9/run_test.py clang
```

run_test.py executes each binary under taskset -c 0,1 timeout 30 and records its exact exit and elapsed time.
All four builds exit 0 with 0 compiler warnings. The final results are:

| Configuration | Tests | Checks | Failed checks | Exit | Seconds |
|---|---:|---:|---:|---:|---:|
| Plain | 13 | 306730 | 0 | 0 | 2.399763 |
| SAN=1 | 13 | 306730 | 0 | 0 | 3.665638 |
| INV=1 | 14 | 306749 | 0 | 0 | 3.674996 |
| CC=clang | 13 | 306730 | 0 | 0 | 2.667685 |

SAN uses ASAN_OPTIONS=detect_leaks=0 and produces 0 ASan/UBSan reports.
LeakSanitizer cannot run in this sandbox, as stated in the brief; leaks were not checked.
The system FLINT shared library was not rebuilt with sanitizers.

```text
timeout 170 python3 lanes/f-repair9/fixtures.py --pole 0
timeout 170 python3 lanes/f-repair9/fixtures.py --pole -2
timeout 170 python3 lanes/f-repair9/fixtures.py --pole -4
timeout 170 python3 lanes/f-repair9/fixtures.py --pole -16
timeout 170 python3 lanes/f-repair9/fixtures.py --assemble
timeout 170 python3 lanes/f-repair9/crosscheck.py
timeout 60 python3 lanes/f-repair9/active_check.py
timeout 170 python3 lanes/f-repair9/mutations.py --fault half-B --fault lower-E --fault open-integer
timeout 170 python3 lanes/f-repair9/mutations.py
```

Generation: four batches of 336 boxes and 1344 references each, exit 0; assembly 1346 boxes,
5382 references, 6728 records, 275467 bytes, exit 0. Cross-check: 20 samples, 0 failures, exit 0.
Active check: 6730 traces, 6730 OK, 0 failures, exit 0; it compiles its bridge under timeout 30.
Red command: F1/F2/F3 each compile exit 0 and test exit 1, failed counts 4036/840/60.
Full fault command: 13 compiled, 11 detected, 2 survivors, as in the table, script exit 0.

Manual C commands can be reproduced with the following prefix and link suffix. Each compilation exits 0.
The old-object compilation instead uses -c and the export-renaming flag below.

```text
taskset -c 0,1 timeout 30 cc -std=c11 -O2 -g -Iinclude -Isrc SOURCE \
  lanes/f-repair9/build/libadelefeld.a -lflint -lgmp -lm -o OUTPUT
```

SOURCE/OUTPUT pairs were lanes/f-repair9/bridge.c -> build/bridge-base,
constants.c -> build/constants, bound_check.c -> build/bound-check,
compare.c -> build/compare (also links build/old-localfactor.o), and search.c -> build/search.
Paths without a prefix in this paragraph are relative to lanes/f-repair9.
The constants compile was timeout 30 cc -std=c11 -O2 -Wall -Wextra -Werror lanes/f-repair9/constants.c
with -lflint -lgmp -lm -o lanes/f-repair9/build/constants. The trace bridge adds -DREVIEW_TRACE.

```text
taskset -c 0,1 timeout 30 cc -std=c11 -O2 -g -Iinclude -Isrc \
  -Dadf_local_zeta_factor_at=old_local_zeta_factor_at \
  -c lanes/f-repair9/old-localfactor.c -o lanes/f-repair9/build/old-localfactor.o
timeout 5 lanes/f-repair9/build/constants
timeout 5 lanes/f-repair9/build/bound-check
timeout 170 lanes/f-repair9/build/compare
timeout 10 python3 lanes/f-repair9/search_setup.py
timeout 170 lanes/f-repair9/build/search 20000 9041
timeout 170 lanes/f-repair9/build/search 20000 9042
timeout 10 python3 lanes/f-repair9/finalize.py
```

Results: constants 3/0, exit 0; bound check red 1/1, exit 1, then green 1/0, exit 0;
comparison first 20000/59, exit 1, then 20000/0, exit 0; search setup 3 trace sites, exit 0;
search results are tabulated above, both exit 0. Final audit: 4 configurations, 13 faults, 11 detected,
2 survivors, 0 audit assertion failures, exit 0; 9 owned scratch paths removed.
The four build trees, lane bytecode cache and four intermediate pole JSON files have been removed.

Earlier checks and corrections, retained in their logs:

- Initial library baseline: timeout 30 lanes/f-repair9/build/test_localfactor, 10 tests, 36725 checks,
  0 failures, exit 0. The initial timeout 170 make -j2 lane test build exits 0.
- timeout 170 python3 lanes/f-repair9/mutations.py --fault wrong-sign --fault lowprec-log-no-radius:
  before guards, exits -11/-11; after guards, 10 tests, 36637/37485 checks, 136/235 failures, exits 1/1.
- The first two fixture-script launches occurred before its file was created and exited 2/2.
  After creating it, the four pole batches were run for each of three geometries, each batch exit 0.
- First diagonal complex geometry: green test 286104 checks, 16 failures, exit 1, because three boxes
  gave NOT_DETERMINED. Red F1/F2/F3 counts were 4052/856/76.
- Second, vertically thin left geometry: green test 279684 checks, 6 failures, exit 1, because one box
  gave NOT_DETERMINED. Red counts were 12084/846/66. Assertions were kept; the complex centres moved right.
- Final right geometry before record splitting: green 279819 checks, 0 failures, exit 0;
  red counts 4036/840/60. Subsequent record splitting added parser checks, with the final counts above.
- Each geometry's timeout 170 fixtures.py --assemble and crosscheck.py exited 0;
  fixture sizes were 258525,245023,243175 bytes; each cross-check was 20/0 at 400 digits.
- timeout 170 mutations.py --fault lower-E --fault open-integer before fixture creation had
  69906/66546 checks and 841/61 failures, including the missing-file assertion; both exited 1.
- The full fault table also ran before record splitting: 13 compiled, 11 detected, 2 survivors,
  0 SIGSEGV exits. fault-table-before-format.log retains those counts.
- An earlier plain run before record splitting had 279819 checks, 0 failures and 4.752304 seconds.
- Three search runs on seed 9041 used the same 20000 boxes and 94580 samples, with 0 misses and
  0 derivative exceedances. The corrected tighter count is 3994; the final run adds the unproved-region count.
- timeout 5 bridge-base probes: one thin-left box returns ND; four corresponding right-side boxes return OK,
  with 0 output errors. A timeout 10 scalar prototype probe also found a non-finite direct Gamma and quotient.
- A scratch no-logpi trace bridge, compiled with -DREVIEW_TRACE, ran under timeout 5 and returned
  OK with B=(45178009,-19), 1 call and 0 output errors.
- Import probes under timeout 10 reported python-flint 0.8.0 and mpmath 1.3.0. JSON reads confirmed
  3 minimized and 42 witness records. Static line/newline, source-hash and evidence checks are in finalize.py.

## Workflow exceptions and remaining work

F5 remains not done. No full repository, driver or Julia suite was run; only the requested localfactor builds.
No long differential fuzz run or global derivative supremum computation was claimed.
The initial fixture generation briefly overlapped two Python workers and a bridge compilation before CPU
affinity was set. Three single-worker processes overlapped; the two-core cap was not enforced at that point.
Later work used taskset -c 0,1 and no more than two workers.

The initial prototype import also generated proto/__pycache__/zeta_checks.cpython-312.pyc outside lane ownership.
Later imports disable bytecode writes. That generated file was left for the orchestrator: this lane cannot
delete files outside its owned paths. No other out-of-scope write was intended.

## Sources pending

- [source pending: FLINT 3.3.1 scalar arb enclosure contracts for the python-flint oracle runtime.]
  This is Z8's existing source obligation; the on-disk scalar contract is FLINT 3.0.1 arb.rst:6-12.
- [source pending: FLINT 3.0.1 acb/arb exponential and Gamma evaluator source and range guards.]
  Evaluator internals were not proved here. Installed behaviour is tested.
- [source pending: Gelfond-Schneider in refs/ for nonzero exact dyadic finite poles.]
  Production does not use that classification. No mpmath rigorous error guarantee is claimed or needed
  for the Z8 certificates; its twenty-point cross-check is numerical only.

Ground truth read: refs/src/tate-poonen/notes.txt:53-65,1014-1016,1733;
refs/src/flint-3.0.1/arb.rst:6-17, acb.rst:6-17,916-918 and mag.rst:6-9.
The new weighted-integral and partial F5 proofs are our own numbered arguments.
Final report check: timeout 10 python3 with a line/newline audit, 295 lines, 0 overlong lines, final newline.

## Findings against the specification

None established. F1-F3 are injected violations now rejected by the tests.
F4 is a proved smaller certificate. F5 remains unclassified.
