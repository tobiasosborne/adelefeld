# Progress

2026-10-05. Read CLAUDE.md, the lane brief, SPEC 9.3.7, PLAN 1F.9, localfactor.h,
src/localfactor.c, design Z1-Z9, conventions 944-956 and the status row, Y16-Y17,
the implementation report, the localfactor test helpers, and the driver handler.

The live conventions status row already includes LIMIT. The previous report's pending
reconciliation is stale in this working tree. No files outside this lane will be written.

Read ground truth: refs/src/tate-poonen/notes.txt:53-65, 1014-1016, 1733;
refs/src/flint-3.0.1/arf.rst:6-39. The factor formulas, Gamma poles, recurrence, and
arbitrary integer exponent contract are present on disk.

Started the single archive build:
`timeout 600 make -j2 BUILD=lanes/f-review15/build lanes/f-review15/build/libadelefeld.a`
with stdout and stderr in build.log. The build limit is the explicit exception in the brief.

Plan: exact dyadic C bridge; independent mpmath oracle with at least 400 decimal digits;
pole geometry; sampled enclosure and derivative checks; statuses; bounded cost probes;
two sanitizer programs; hostile driver lines; at least ten scratch mutations.

Archive build: exit 0. Baseline test_localfactor: 10 tests, 36725 checks, 0 failed.
Exact rational oracle: 288 calls, 288 OK, 288 exact rational containment checks, 0 failures.
Prime boundary oracle (corrected generator): 978 cases, 61 OK, 917 ND, 1220 samples, 0 failures.
Real pole oracle: 1530 cases, 4 OK, 1024 ND, 170 DOMAIN, 332 LIMIT, 80 samples, 0 failures.
Random p=3: 5000 cases, 4344 OK, 656 ND, 86880 samples, 0 failures.
Random p=2 pilot: 250 cases, 219 OK, 31 ND, 4380 samples, 0 failures.
Random real pilot: 150 cases, 126 OK, 24 ND, 2520 samples, 0 failures.
The first 100 real cases also checked 1200 derivative samples; largest |L'|/B was 0.6666020441.

Harness repairs and incomplete runs are not findings against the library:
- The first zero-pole generator changed its mantissa but left exponent 0. It tested Im(s)=5,
  falsely labelled as containing 0. Fixed before the recorded prime boundary run.
- The 50-box dense derivative pilot exceeded its bridge's 165 s lifetime and stopped with
  BrokenPipeError. It produced no completed result. Dense checks will use smaller batches.
- For a short interval, three oracle subprocesses were admitted at once instead of two.
  Subsequent scheduling allows at most two. This violated the lane's scheduling rule.

Thirteen independent faults were planted, each in a scratch copy. Five survived all 36725
checks: half-B, lower-E, open-integer, no-logpi-B, and no-M0-B. The first three have witnesses:

- half-B: real s=-1*2^-4, imaginary midpoint 0, real radius 1*2^-14, imaginary radius 0,
  prec=256. Baseline OK contains all 20 samples. Mutant OK excludes 9 of 20 samples.
  At the left endpoint the excess is 0.00298784171303551805245581012059.
- lower-E: p=2, s=1 +/- 1, imaginary midpoint and radius 0, prec=128. Baseline ND;
  mutant OK. The closed input contains the pole zero.
- open-integer: real s=-399*2^-1 +/- 1*2^-1, imaginary midpoint and radius 0, prec=64.
  Baseline ND; mutant LIMIT. The closed interval contains the pole -200.

These are MAJOR coverage findings, not BLOCKER findings against the unchanged implementation.
The no-logpi-B and no-M0-B witnesses tried 18 thin boxes each without distinguishing a sample.
The derivative trace in the first half-B witness was stored before halving; fixed the trace
to store the final bound. The witness's sampled value failure was unaffected.

Statuses: 48 cases, 0 failures, each with separate y, y=s, and where=NULL modes.
Cost: 36 calls, 0 timeouts, 0 representation failures, 0 non-finite stored balls.
At real s=1, call times at 64,4096,65536,1048576 bits were 0.000141,0.000776,0.028712,1.294944 s.
At p=2 they were 0.000090,0.000472,0.021146,0.876199 s.
The enormous-exponent probes include positive and negative 2^(2^60), 2^(-2^60), and Im(s)=2^1000.

Driver: 30 hostile commands, 30 matching lines, exit 1 as expected for a file with errors.
Ten lines matched a separate direct-library caller byte for byte. Debug handle program:
13 checks, 0 failures; four forged words abort with SIGABRT, and cap+1 returns LIMIT first.

Dense special boxes: 24 cases, 22 OK, 1782 value samples and 1782 derivative samples,
0 failures. The largest sampled |L'|/B was 0.6666666505237424308893861841.
The earlier interrupted dense pilot had a checkpoint after 30 completed cases and 2430
value and derivative samples, with 0 failures. These are partial results, not a completed run.

Read refs/src/flint-3.0.1/acb.rst:916-918: psi = Gamma'/Gamma. The independent oracle uses
mpmath.digamma for psi, not FLINT's evaluator. Product and chain rules give
L_inf'(s) = L_inf(s) (psi(s/2) - log(pi))/2.

Both sanitizer programs were compiled with address and undefined-behaviour instrumentation
for localfactor.c and place.c. The leak-enabled status bridge completed 48 cases with 0
output errors, then LeakSanitizer failed at exit: "does not work under ptrace". All three
cost runs also encountered that LeakSanitizer exit failure. Leak detection will be disabled
for the rerun, as the brief permits; this is not a library memory finding.

Leak-disabled sanitizer rerun: status bridge 48 cases, 0 failures; finite pole bridge
978 cases, 1220 samples, 0 failures; real pole bridge 1530 cases, 80 samples, 0 failures.
Cost sanitizer: 3 calls, 0 errors. No address or undefined-behaviour report was emitted.

The prime membership verifier's first input encoder parsed negative k from a hyphenated ID
incorrectly. It stopped at 943 records and reported 581 failures. This was an encoder error,
not a factor call. An explicit pole_index field fixed it. Corrected scalar arb certificate:
978 records, 594 certified pole memberships, 384 certified disjoint boxes, 0 failures,
4000-bit pi and log computations. No acb_gamma or acb_pow reference is used.

Recurrence sweep: 201 calls, 194 fallback attempts, 7 OK, 140 samples, 0 failures.
Completed dense random runs: 30 cases, 2430 value samples, 2430 derivative samples, 0 failures.
Random boxes: all six primes have 5000 calls each. The real 5000-call run had only 4889
distinct encoded boxes because pilots reused seeds. Adding 111 cases with a new seed.

Witness minimisation searched a stated dyadic family, not all possible inputs. It found:
- half-B: s=-1*2^-4 +/- 1*2^-8, imaginary part exactly 0, prec=16; mutant OK misses
  the right endpoint value by 0.0857070023174979561870697008038. Candidate 364 was first.
- lower-E: s=1 +/- 1 at p=2, imaginary part exactly 0, prec=2; mutant OK contains a pole input.
- open-integer: s=-255*2^-1 +/- 1*2^-1 at real, imaginary part exactly 0, prec=2;
  mutant LIMIT instead of ND. The pole is -128, the first pole at the shift-limit boundary.

The original half-B witness's corrected derivative ratio is 1.1093715842849024110.
The original program's ratio is half that number. Both original and minimised witnesses
have the full exact returned dyadics in witnesses.json and minimized.json.
