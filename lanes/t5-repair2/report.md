# Lane t5-repair2

F1, F2 and F3 are closed. Five owned repository files changed. No header, vector, production driver,
or driver fixture changed. The final 24 test runs exit 0, with 1792968 checks in total.

## Files written

- src/localfactor.c: bounded Gamma recurrence refinement and a sharper real derivative certificate.
- tests/test_tate_local.c: append-only review_real_width, 60 new checks.
- tests/test_tate_gamma.c: append-only inclusive_character_cap, 43 new checks.
- docs/api-5a.md: appended Closure of m5-review1, including the certificate proof and fixture width.
- docs/api-5b.md: S2's conductor-1 qualification, S12's justification and limit, and S20's Check line.

Lane sources: audit.py, check_driver.py, check_f2.py, measure_f1.py, prepare_mutations.py,
run_checks.py, probe.c, probe_lgamma.c and probe_lgamma_rising.c.
Lane records: checks.jsonl, final-counts.json, f3-counts.json, this report, and the logs in artifacts.txt.
artifacts.txt lists the retained files produced by this lane. Harness-owned brief.md, lane.log and
stdout.log are excluded. All eight lane build directories were deleted; zero remain (cleanup.log).

## F1: real integral width

The two requested characters, (1,1) and (5,4), are even. Their input has z = s/2 centred at -3/8.
The brief's description as an odd integral at z = -1/8 does not describe those two inputs.
The regression uses the requested inputs without substitution: alpha = 1, prec = 160,
S = -3/4 +/- 2^-12 + i (0 +/- 2^-12).

The fixed sampled lower width is 0.00010116010017604849. The independently reviewed sampled width
is 0.000101160100176048496970148371598. The test also certifies the lower separation using
512-bit point balls at the centre and four corners. It requires containment of all five point balls.
The coordinate target remains 64 times that lower width plus 20 times 2^-152.
For Re(z+2) between 1 and 2, the defining Gamma integral bounds |Gamma(z+2)| by
integral_0^1 1 dt + integral_1^inf t exp(-t) dt < 2. Also |z| > 0.37, |z+1| > 0.62 and
|pi^-z| < 2. The recurrence therefore gives image_bound = 20.

The unchanged implementation failed four new coordinate-width assertions, over two characters.
It still passed all existing vectors. A recurrence-only intermediate change also failed those four
assertions and the review's two width checks. The diagnostic probes show why: with a product loop,
shifting to z+2 gives radius about 0.00620, versus about 0.00390 directly. Log-Gamma and rising-product
probes did not close the target. No test or width statement was weakened.

The final repair retains a finite direct candidate and intersects it with the shifted candidate for
0 < n <= 64, with n chosen so Re(z+n) >= 1. Division requires a product excluding zero.
The exact/mixed pole checks run first. A zero-containing computed product cannot imply DOMAIN.
A finite direct candidate remains available when the recurrence cannot certify division, or n > 64.
When the direct candidate fails and n > 64, the existing LIMIT rule remains.

The additional derivative certificate is needed for this fixture. Its proof is:

1. Iterating Gamma(z+1) = z Gamma(z) gives Gamma(z+n)/product_(j=0..n-1)(z+j) = Gamma(z).
   Each ball operation encloses the pointwise operation. Intersecting two candidates keeps the image.
2. Differentiating L(s) = pi^(-s/2) Gamma(s/2) gives
   L'(s) = L(s) (digamma(s/2) - log(pi))/2 on the pole-free rectangle.
3. The candidate bounds |L| on that rectangle. The digamma ball bounds the other factor. Their outward
   magnitude product, divided by 2, bounds |L'|. Its minimum with the existing Z6 bound is still a bound.
4. The segment from the midpoint to any input point stays in the rectangle and has length at most R.
   Integrating L' on that segment gives variation at most R B. The enlarged midpoint ball, its
   intersection with the candidate, and the final outward rounding all preserve enclosure.

Sources: refs/src/tate-poonen/notes.txt:52-61 (Gamma integral and recurrence), :62-64 (poles/no zeros);
refs/src/flint-3.0.1/acb.rst:893-896 (acb_gamma), :916-918 (digamma).
The stepwise certificate is also appended to api-5a.md.

The final largest coordinate diameter is 0.000185743942665794747881591320038 for both characters.
The diameter/sample ratio is 1.8361383820552307233, below 64. Both calls return OK.

## F2: completed limit and conductor 1

S2 now qualifies entireness by C > 1 and names the poles at 0 and 1 for C = 1.
S12 now uses S9's local regularity at its permitted s0. It does not call Lambda entire for C = 1.

The normalization is api-5.md:124-126: Lambda = (pi/C)^(-z) Gamma(z) L(s), z = (s+e)/2.
At s0 = -e-2k, z0 = -k. P10's own residue proof (analysis.md:444-446) gives
Gamma(z(s)) ~ 2 (-1)^k/(k! (s-s0)). The regular split forces L(s0) = 0.
Multiplication by the prefactor at z0 gives the completed limit
2 (-1)^k (pi/C)^k L'(s0)/k!, which is now stated in S12 and its reference description.

For conductor 1 at s0 = -2, the naked number is
0.060896914116786541560503060942309553294000967089948.
The completed value is
0.19131329801558517112507657012043306851299186427523.
The five-check script checks three textual corrections and two numerical distinctions.
Its red run has three failures; its green runs have zero. No runtime code or vectors changed for F2.

## F3: production driver independence

The existing fixture already distinguishes A and R. At zeta 1/2 its imaginary radii print as
2.1e-26 and 1.5e-27. The derivation was read in lanes/t5-slice5/derive_fixture.py before running it.
No fixture line was appended. S20 now names the driver fixture as the production-handler check and
identifies independence/hand as checks of the test's separate fe_sides.

The production copy fault was planted only in a lane scratch copy of tools/adf/adf.c:
acb_mul(R, W, B, wp) became acb_set(R, A).
The unchanged full driver test rejected it at tate-fe, with two different lines among 15: lines 1 and 7.
The baseline full driver test passed 100 cases and 101691 expected lines.
At the end, the baseline was rerun and the fault replanted. The same two lines differ and the driver
test exits 1. The helper exits 0 only when that specific rejection occurs. See f3-counts.json.
The production handler and both fixture files remain untouched.

## Commands and results

All commands ran from the repository root unless stated otherwise. The shell abbreviations below
shorten the command recipes. Replace MODE and NAME with the stated choices. Builds can be recreated.

```sh
repair_lane=lanes/t5-repair2
repair_plain=$repair_lane/build-plain
P() { taskset -c 0,1 timeout "$@"; }
```

All six binaries are named in the final table. run_checks.py wraps each make and each binary in
its own timeout 170. Exact commands, exits, elapsed times and log names are recorded in checks.jsonl.
The outer script timeout is 175. Builds use -j2. Affinity is CPUs 0 and 1 throughout numerical work.
The C headers report FLINT 3.0.1. No installation, git state change, bd, or check-all was run.

### Red and green runs

- `P 170 make -j2 BUILD=$repair_plain $repair_plain/test_tate_local`: first build exit 2 at a missing
  main caused by temporary test placement; after restoring the original include block, exit 0.
  Logs: f1-red-build.log and f1-red-build2.log. That link error is not counted as the F1 red result.
- `P 170 $repair_plain/test_tate_local`: F1 red exit 1, 22807 checks, four failed checks;
  recurrence-only intermediate exit 1 with the same counts; final green exit 0, 22807 checks, zero failures.
  Logs: f1-red.log, f1-green-local.log, f1-green-local2.log.
- `P 170 make -j2 BUILD=$repair_plain $repair_plain/test_tate_local $repair_plain/test_tate_gamma`:
  recurrence build exit 0; the first derivative implementation build exit 2 with GCC object-size warnings
  at the endpoint calls; moving the certificate into the existing derivative-bound helper gave exit 0.
  Logs: f1-green-build.log, f1-green-build2.log, f1-green-build3.log.
- `P 170 $repair_plain/test_tate_gamma`: F1 green exit 0, 47955 checks, zero failures before the cap test.
  Log: f1-green-gamma.log.
- `P 20 python3 -B $repair_lane/check_f2.py`: red exit 1, five checks, three failures; green exit 0,
  five checks, zero failures; final repeat exit 0 with the same counts. Logs: f2-red.log, f2-green.log,
  f2-green-final.log.
- `P 175 python3 -B $repair_lane/check_driver.py fault`: initial and final helper exits 0;
  the unchanged driver test exits 1, with two changed fixture lines. Logs: f3-red.log, f3-final-fault.log.
- `P 175 python3 -B $repair_lane/check_driver.py baseline`: initial and final exits 0;
  each has 100 cases and 101691 equal expected lines. Logs: f3-green.log, f3-final-baseline.log.

### F1 review harness and probes

```sh
P 60 cc -O2 -g -Iinclude lanes/m5-review1/runner.c "$repair_plain/libadelefeld.a" \
  -lflint -lgmp -lm -o "$repair_plain/runner"
ADF_REVIEW_RUNNER="$PWD/$repair_plain/runner" \
  P 170 python3 -B lanes/m5-review1/extra_checks.py real
ADF_REVIEW_RUNNER="$PWD/$repair_plain/runner" \
  P 20 python3 -B "$repair_lane/measure_f1.py"
```

The runner was compiled twice, both exits 0 (runner-build.log and runner-build2.log).
The recurrence-only harness run exits 1, with 2594 checks, 160 inputs, 480 calls and two failures.
The final harness run exits 0 with the same counts and zero failures.
Logs: f1-harness1.log and f1-harness-green.log. The diameter probe exits 0 and prints the two ratios
above (f1-measure.log). The review runner path override avoids writing in the review lane.

For each of probe, probe_lgamma and probe_lgamma_rising, the following compile/run pair ran twice:

```sh
P 60 cc "$repair_lane/NAME.c" -lflint -lgmp -lm -o "$repair_plain/NAME"
P 10 "$repair_plain/NAME"
```

All six compilations and six probe processes exit 0. Each process prints six shift rows, n = 0..5.
These are enclosure diagnostics with zero assertions. Their final records are f1-probe-final.log,
f1-probe-lgamma-final.log and f1-probe-lgamma-rising-final.log; f1-probe.log retains the first Gamma run.

The two-row F1 Valgrind run uses the same runner and the two requested characters:

```sh
printf '0 0 1 1 0 -0.75 0 -12 1 0 9999 80 160\n1 0 5 4 0 -0.75 0 -12 1 0 9999 80 160\n' |
  P 170 valgrind --error-exitcode=9 --track-origins=yes --leak-check=full \
  --show-leak-kinds=all --errors-for-leak-kinds=definite,indirect \
  --log-file="$repair_lane/f1-valgrind.log" "$repair_plain/runner"
```

Exit 0; two inputs, six local calls; 4465 allocations and 4465 frees; zero bytes/blocks at exit;
zero errors from zero contexts. Output is f1-valgrind.out.

### Final required test matrix

```sh
P 175 python3 -B "$repair_lane/run_checks.py" build MODE
P 175 python3 -B "$repair_lane/run_checks.py" run MODE
```

MODE is plain, san, inv or clang. The build script sets SAN=1, INV=1 or CC=clang for those modes.
It also built san-inv with both flags for the mutation archive. All five initial mode builds exit 0.
All four incremental builds after adding the cap test exit 0.
For san runs, the outer command additionally sets ASAN_OPTIONS=detect_leaks=0.

| Binary | plain | SAN=1 | INV=1 | CC=clang |
|---|---:|---:|---:|---:|
| test_tate_local | 22807 | 22807 | 22822 | 22807 |
| test_tate_gamma | 47998 | 47998 | 48028 | 47998 |
| test_localfactor | 309581 | 309581 | 309600 | 309581 |
| test_tate | 22909 | 22909 | 22919 | 22909 |
| test_tate_continue | 25800 | 25800 | 25810 | 25800 |
| test_tate_fe | 19126 | 19126 | 19126 | 19126 |
| Total | 448221 | 448221 | 448305 | 448221 |

Each of these 24 final binary runs exits 0. There are zero failed assertions and zero ASan/UBSan
error diagnostics in their logs. All 394 local vectors/1861 samples and 721 gamma vectors/1950 samples
run in each corresponding test binary. final-counts.json records the table.

Nonfinal runs are retained, not counted as successful final runs:

- The first SAN=1 six-test batch has six binary exits 1 at LeakSanitizer's fatal ptrace error.
  Repeating with detect_leaks=0 gives six exits 0. The initial diagnostics are in *-lsan.log.
  Full-suite leak detection therefore was not obtained. The separate F1 Valgrind check above passed.
- Before the cap test, all six tests also passed in each of the four modes. Gamma then had 47955 checks,
  or 47985 under INV. Those logs are retained as *-before-cap.log.
- After the cap test, concurrent plain, san and inv batch wrappers exit 124 at their 175-second limits.
  Completed child tests exit 0. Unfinished tests were repeated sequentially by the commands below;
  every replay exits 0. The final clang batch completes with exit 0.

```sh
P 175 python3 -B "$repair_lane/run_checks.py" run plain test_tate_fe
ASAN_OPTIONS=detect_leaks=0 \
  P 175 python3 -B "$repair_lane/run_checks.py" run san test_tate_continue test_tate_fe
P 175 python3 -B "$repair_lane/run_checks.py" run inv test_tate_continue test_tate_fe
```

Logs are plain-run-final.log, san-run-final.log, inv-run-final.log, clang-run-final.log,
and the three *-run-finish.log records. The batch timeouts did not increase the two-core bound.

## Mutation run and closure of its test gap

```sh
P 30 python3 -B "$repair_lane/prepare_mutations.py"
ASAN_OPTIONS=detect_leaks=0 \
  P 1200 python3 -B tools/mutate/mutate.py --root "$repair_lane/build-mutation-root" \
  --scratch "$repair_lane/build-mutation" --files src/localfactor.c --limit 60 \
  --seed 20261015 --jobs 2 --timeout 170 --san --make 'make -s -j2 check INV=1' \
  --copy Makefile include src tests lanes
```

Preparation exits 0. The staged root uses a prebuilt SAN=1/INV=1 archive. Each mutant object precedes
that archive on the link line, so it replaces localfactor.o. Its check target builds and runs the three
direct tests: test_tate_local, test_tate_gamma and test_localfactor. Each test has timeout 60.
The global tests were run in the required matrix above. The staged mutation tests were frozen before
the new inclusive-cap regression was added.

The tool generated 621 choices and selected 60. Its baseline passes in 29.5 seconds.
The bounded run exits 1 because it reports survivors. Duration is 1092.9 seconds, below 20 minutes.
No mutation tool or equivalent.txt was changed. See mutation.log.

| Run | Killed | Survived | Not compiled | Timed out | Excused |
|---|---:|---:|---:|---:|---:|
| Initial 60 selected mutants | 36 | 13 | 11 | 0 | 0 |
| Replay of the cap survivor with the new test | 1 | 0 | 0 | 0 | 0 |

The cap replay repeats an existing selected mutant; it is not a 61st distinct mutant.
One of the 13 survivors is now killed. The 12 other survivors are listed below.
Their explanations concern arithmetic equivalence, redundant checks, or a valid larger enclosure;
they are not untested invalid values. The original tool result remains exit 1.

### Inclusive cap regression: red then green

First, the unchanged runner was given the row below under P 30. It exits 0, retaining conductor 65536,
parity 0, with three OK local calls (cap-boundary-probe.log).

```text
0 0 65536 5 0 0.5 0 9999 1 0 9999 80 160
```

The new test uses that primitive character at infinity, s = 1/2 and alpha = 1.
P10 gives gamma = epsilon = 1. It repeats both calls through the existing alias/atomicity helper.
The scratch source changes only adf_local_gamma_at's conductor test from > to >=.
It is compiled as cap-mutant.o and linked before the unchanged archive into test_cap_mutant.
The source and binaries were kept under the lane build directories and removed at cleanup.

Compile commands use timeout 60 cc with -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror,
-Iinclude -Isrc for the object, and -Iinclude -Itests for the test. The link adds the lane's
support/golden.o, support/jsonl.o and libadelefeld.a, then -lflint -lgmp -lm.
The SAN/INV replay additionally uses -fsanitize=address,undefined -fno-omit-frame-pointer,
-DADF_CHECK_INVARIANTS and -pthread. All four compile/link commands exit 0.

- `P 170 $repair_plain/test_cap_mutant`: first attempt exits 127 because the link was still running;
  zero checks ran. The retry exits 1: 47994 checks, two failures in inclusive_character_cap.
  Logs: cap-mutant-red.log and cap-mutant-red2.log.
- `P 170 $repair_plain/test_tate_gamma`: live green exits 0: 47998 checks, zero failures.
  Log: cap-boundary-green.log. Production code already admitted the cap, so no code change was needed.
- `ASAN_OPTIONS=detect_leaks=0 P 170 $repair_lane/build-san-inv/test_cap_mutant`: exit 1,
  48024 checks, two failures, with the same scratch fault. Log: cap-mutant-san-red.log.
  The final live INV test has 48028 checks, zero failures; live SAN has 47998, zero failures.

The two cap red runs take about 2 and 8 seconds by their log timestamps. With compilation, these
supplementary replays keep total mutation computation below the 20-minute bound.

### Remaining survivors, one reason each

Line numbers refer to src/localfactor.c in this lane's final diff.
For the ceiling explanation, refs/src/flint-3.0.1/arf.rst:80-83 and :284-289 specify upward integer rounding.

- 599, DOMAIN -> OK: q = 0 or p < 2 violates canonical-character/certified-place preconditions; INV stops earlier.
- 267, swapped mag_add inputs: the same two nonnegative bounds are added outward.
- 333, arf_sgn(lo) <= 0 -> <= 1: the extra lo > 0 case sets h = 0, and h >= lo is still false.
- 239, swapped mag_add inputs: the same two bounds are added outward.
- 778, >= 0 -> >= 1: equality is the first exact gamma pole; delegated real_factor still returns DOMAIN.
- 552, q > 1 -> q > 0: the extra q = 1 case fails q % p == 0 for every certified p >= 2.
- 633, swapped acb_mul inputs: alpha times T encloses the same product.
- 291, swapped mag_min inputs: the same smaller derivative bound is selected.
- 233, dropped arf_ceil: the following arf_get_si with ARF_RND_CEIL already computes the same ceiling.
- 779, <= 0 -> < 0: at s = -e, the fallback recurrence has an exact zero factor and still returns exact gamma 0.
- 754, swapped acb_mul inputs: the recurrence product times reciprocal Gamma encloses the same product.
- 173, inverted imaginary-intersection guard: both finite Gamma candidates share the true image, so they
  intersect; the mutant omits imaginary refinement and retains the valid direct enclosure. The fixture still passes.

The 11 noncompiling mutants are not counted as killed: lines 498 and 315 fail the parentheses warning;
669, 448, 549, 551 and 348 can use an uninitialized variable; 232, 698 and 702 leave an unused parameter;
794 leaves rgamma_shifted unused. These diagnostics are retained in mutation.log.

## Final audits, cleanup and limits

`timeout 10 python3 -B lanes/t5-repair2/audit.py` exits 0: five owned edited files, 181 added lines,
nine retained lane source files, zero ownership/line-width/newline/diff-check failures.
Its count audit confirms 24 final exits 0 and 1792968 checks.
The final driver-output comparison exits 0: 15 lines, exactly two changed lines, indices 1 and 7.
The timeout 30 cleanup script removes eight lane build directories and confirms zero remain.
Read-only git diff/check commands changed no git state. Source changes remain within the ownership list.

Not done: check-all, the whole independent milestone-5 review matrix, a fresh F4 precision-cap timing
probe, and full-suite LeakSanitizer checking. F4 remains the recorded adf-m3bw cost item.
F5 is the N-D25 item 6 decision and needs no repair here. No new HEADER-FINDING is needed.
The real refinement adds a bounded shifted Gamma evaluation and one digamma evaluation to the existing
certificate. It introduces no wall-clock bound or adaptive subdivision.

## Sources pending

No new formula in this lane has a source pending. The existing proof imports used by S9 remain pending
in docs/proofs/analysis.md:40-48: [source pending: Fubini, dominated convergence, holomorphic parameter
integration, and the identity theorem]. This lane does not close that separate source obligation.

## Findings against the specification

Zero new findings against docs/SPEC.md. F1 was a fixture-width failure; F2 was a statement error;
F3 was a check-line attribution error. The inclusive-cap mutation gap is now tested.
The width target, enclosure contract, pole rules and precision-cap decision were not weakened.
