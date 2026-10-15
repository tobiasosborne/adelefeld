# Atlas test drive

Start: 2026-10-16T00:11:41+11:00, from `date -Iseconds`.
Working directory: repository root. Driver and generated files stay in `lanes/ux-drive-atlas/`.
The builder's report is unread until step 5.

## Preliminary reading

Read `CLAUDE.md`, `lanes/UX-DRIVE.md`, `lanes/COMMON.md`, `lanes/UX-COMMON.md`, the atlas brief,
then only `ux/atlas/README.md` within the slice. Read SPEC 2 and 9.3.1 to 9.3.3, PLAN 1F,
and the driver's README. No AGENTS.md was found at the ancestor paths or under lanes, ux or docs.
The first three read-only shell calls lacked timeout; subsequent shell commands use timeout.
No git command, tracker command, package installation, or builder report has been used.

## 1. Setup

Expected: the README's isolated build makes a driver; scenes reproduce without editing the slice.
`build/adf` does not exist. Python, Node, Firefox and cc are installed; FLINT headers exist.
The README's default build and scene regeneration write outside this lane. Its isolated-build recipe
and fresh output directory are used, substituting this lane for `lanes/ux-atlas`.
This substitution is required by file ownership, not a fix to the implementation.

Commands planned:

```sh
timeout 170 make -j2 BUILD=lanes/ux-drive-atlas/build all
timeout 120 cc -Iinclude tools/adf/adf.c lanes/ux-drive-atlas/build/libadelefeld.a \
  -lflint -lgmp -lm -o lanes/ux-drive-atlas/build/adf
```

The README commands' exact unmodified forms are not run when they write outside this lane.
The literal `make -C tools/adf`, default scene build, and `lanes/ux-atlas` output paths remain untested.

2026-10-16T00:13:33+11:00: setup complete.
Both build commands returned 0. Logs: `build.log`, `driver-build.log`.

```sh
timeout 170 env PYTHONDONTWRITEBYTECODE=1 ADF=lanes/ux-drive-atlas/build/adf \
  sh ux/atlas/build_scenes.sh lanes/ux-drive-atlas/scenes
timeout 120 env PYTHONDONTWRITEBYTECODE=1 ADF=lanes/ux-drive-atlas/build/adf \
  python3 -m unittest discover -s ux/atlas
timeout 120 node --test --test-isolation=none ux/atlas/test_atlas.mjs
timeout 10 diff -ru ux/atlas/scenes lanes/ux-drive-atlas/scenes
```

Results: 2 scenes regenerated, diff 0 lines; 22 Python tests, 0 failures; 9 Node tests, 0 failures.
The diff ran inside a timeout 10 shell in the original invocation. Logs are retained next to this log.

```sh
timeout 30 taskset -c 0,1 firefox --headless --profile lanes/ux-drive-atlas/firefox-profile \
  --screenshot lanes/ux-drive-atlas/initial.png file://$PWD/ux/atlas/atlas.html
```

Expected: a screenshot of the empty page. Actual: exit 139, `timeout: the monitored command dumped core`.
No PNG exists. This is an environment/browser failure before scene loading, not an established page defect.
No screenshot or real browser interaction is claimed. The Node stand-in below checks emitted DOM, not pixels.

## 2. Scenario A

2026-10-16T00:16:03+11:00: A1 to A4.
Read the HTML the README says to open, its loaded `atlas.js`, and the selected `sqrt9.json`.
A1: the address and lens are labels, not inputs. The picker loads the saved scene at real, 2, 3, 5 and 7.
The brief's default had only 2, 3 and 5. The README correctly names the additional 7.
A2: real points -3 and 3; two branches at each prime. At depth 1 the 2 and 3 paths coincide.
A3: the page emits 8 thread curves, connecting each of two rational roots to four tree roots.
Its thread data names 3 and -3 and links to exact driver root calls. The fog explicitly says the lens
was not computed there. Hovering the fog does not compute roots at 7, 11 or 13.
A4: path nodes have hover and focus handlers, but no click/branch selection handler. There is no braid.
The README and page state that this is outside the slice. No false partial-product claim is shown.

```sh
timeout 30 python3 lanes/ux-drive-atlas/check_scene.py ux/atlas/scenes/sqrt9.json
timeout 30 node lanes/ux-drive-atlas/check_dom.cjs ux/atlas/scenes/sqrt9.json \
  lanes/ux-drive-atlas/sqrt9-dom.json
```

Results: 1279 checks, 0 failures, 46 driver records replayed byte for byte. DOM: 4 sliders, 20 changes,
918 SVG elements, 152 focus checks, finite coordinates. Logs: `sqrt9-check.log`, `sqrt9-dom.log`.
The independent checker imports no slice code. It checks every stored digit and hover-ball centre.

Digit proof used by the checker:
1. For rational q and start exponent s, q / p^s has denominator coprime to p.
2. Modulo p^(k-s), invert that denominator by the integer extended inverse and reduce the numerator.
3. Repeated division by p gives digits d_i with q congruent to sum d_i p^(s+i) modulo p^k.
4. Every prefix therefore names exactly the printed ball. The same calculation checks unlit siblings.
For q = 3 at depth 5, digits are 11000 at 2, 01000 at 3, 30000 at 5, all low first.
For q = -3 they are 10111 at 2, 02222 at 3, 24444 at 5.
5. (x-3)(x+3) = x^2-9, so these two distinct rational roots exhaust the roots in each field.
6. At 2, -3 is 1 modulo 4 and 3 is 3 modulo 4: seeds +1 and -1 respectively. At 3 the unit
parts of 3 and -3 are 1 and 2 modulo 3. At 5 the seeds are 3 and 2. All stored seeds agree.
These are direct arithmetic proofs. No outside formula is being cited from memory.

2026-10-16T00:14:00+11:00: generated A5 input before the A1-A4 checks finished.

```sh
timeout 130 env PYTHONDONTWRITEBYTECODE=1 python3 ux/atlas/scene.py -1 --lens 'roots 2' \
  --primes 2 3 5 7 --driver lanes/ux-drive-atlas/build/adf \
  --output lanes/ux-drive-atlas/minus1.json
```

Result: exit 0, no output. The README's output path was changed only to respect lane ownership.

2026-10-16T00:17:31+11:00: A5 and A6 complete.

```sh
timeout 30 python3 lanes/ux-drive-atlas/check_scene.py lanes/ux-drive-atlas/minus1.json
timeout 30 node lanes/ux-drive-atlas/check_dom.cjs lanes/ux-drive-atlas/minus1.json \
  lanes/ux-drive-atlas/minus1-dom.json
timeout 30 lanes/ux-drive-atlas/build/adf lanes/ux-drive-atlas/scenario-a.cmd
timeout 30 env PYTHONDONTWRITEBYTECODE=1 python3 ux/atlas/scene.py -1 --lens 'roots 2' \
  --primes 5 --depth-max 12 --driver lanes/ux-drive-atlas/build/adf \
  --output lanes/ux-drive-atlas/minus1-depth12.json
```

minus1: 319 checks, 0 failures, 46 records replayed. DOM: 4 sliders, 20 changes, 299 SVG elements,
46 focus checks. Real, 2, 3, 7 are complete empty lists; 5 has two roots at all five depths.
The page reports OK/empty, as the polynomial solver does. The separate local-root calls return DOMAIN.
No DOMAIN status is hidden: the two APIs have different contracts, and the transcript preserves both.
The deepest handle is 5. The depth-12 builder attempt returns 1 with
`scene: depth range must satisfy 1 <= min <= max <= 5`. This is an honest slice limit.
The explicit driver probe returns 1 because four lines are DOMAIN; stderr is empty. Exact output is
`scenario-a.out`. At 5 it gives centres 280182 and 110443 modulo 5^8, then 123327057 and 120813568
modulo 5^12. The roots of 9 at 7, 11 and 13 are exactly 3 and -3. These are direct probes, not page values.

Own existence checks: a real square is nonnegative; the squares modulo 4 are 0,1, modulo 3 are 0,1,
and modulo 7 are 0,1,2,4. Thus -1 cannot be a square at those four places. Modulo 5 the roots are 2,3.
If a^2+1 is 0 modulo 5^k, replacing a by a+t*5^k gives
a^2+1+2*a*t*5^k modulo 5^(k+1). Since 2*a is invertible modulo 5, exactly one t in 0..4 extends
each root. This proves the two paths are complete at every tested depth. Numeric checks follow below.

## 3. Scenario B

2026-10-16T00:17:48+11:00: B1 to B5 complete.
B1: load idempotents30. At depth 1 all three trees show 0 and 1. The polynomial is printed below the map;
the top lens label remains `roots 2`, which is ambiguous without reading those coefficients.
B2: 8 table rows, no eight braids. The page correctly calls them residues, not rational roots.
B3: changing the 2 handle to 4 gives 0 and 1 modulo 16. All table contains results are true.
B4: fog says input footprint only, lens not computed here, zero undrawn modulus factors below 1000.
30 = 2*3*5, all drawn, so the count 0 is correct. There is no narration about infinitely many solutions.
B5: no tour, folding, handover or command line on the page. The CLI can produce the requested new scene.

```sh
timeout 30 python3 lanes/ux-drive-atlas/check_scene.py ux/atlas/scenes/idempotents30.json
timeout 30 node lanes/ux-drive-atlas/check_dom.cjs ux/atlas/scenes/idempotents30.json \
  lanes/ux-drive-atlas/idempotents-dom.json
timeout 130 env PYTHONDONTWRITEBYTECODE=1 python3 ux/atlas/scene.py '(* ; 0 mod 36)' \
  --lens 'roots 2' --polynomial '0 -1 1' --modular 36 --primes 2 3 --depth-max 2 \
  --driver lanes/ux-drive-atlas/build/adf --output lanes/ux-drive-atlas/idempotents36.json
timeout 30 python3 lanes/ux-drive-atlas/check_scene.py lanes/ux-drive-atlas/idempotents36.json
timeout 30 node lanes/ux-drive-atlas/check_dom.cjs lanes/ux-drive-atlas/idempotents36.json \
  lanes/ux-drive-atlas/idempotents36-dom.json
```

30: 622 checks, 0 failures, 51 records replayed. DOM: 3 sliders, 15 changes, 561 SVG elements,
93 focus checks. 36: builder exit 0; 107 checks, 0 failures, 24 records replayed. DOM: 2 sliders,
4 changes, 124 SVG elements, 17 focus checks. The four residues are 0,1,9,28.
The follow-up's first-digit table shows residues modulo 2 and 3, not the complete depth-2 residues.
This is labelled as first digits, so it is not a false number.

Own proof: consecutive integers e and e-1 have gcd 1, since a common divisor divides their difference 1.
Thus p^k divides e(e-1) only when it divides one entire factor. Each prime power has exactly 0 and 1.
Enumeration of all 30 representatives gives 0,1,6,10,15,16,21,25; enumeration of 36 gives 0,1,9,28.
Two numbers with the same residues at 2,3,5 differ by a multiple of all three, hence of 30; the eight
listed choices are distinct and exhaust the eight 0/1 triples. The analogous argument uses 4 and 9 for 36.
The driver squares and contains records are replayed; integer verification of every stored square follows.

## 4. Three additional promises

2026-10-16T00:19:20+11:00: all three generated and loaded.

1. The trees: "Negative valuation: the tree grows upward above the root by -v levels"; test 1/5.
2. The trees: "Above p = 13 a tree is drawn as a residue wheel"; test p = 101.
3. Interaction model: "lens (which function: identity, square roots, exp, Log, a map)"; test exp.

```sh
timeout 130 env PYTHONDONTWRITEBYTECODE=1 python3 ux/atlas/scene.py 1/5 --lens identity \
  --primes 2 5 --driver lanes/ux-drive-atlas/build/adf --output lanes/ux-drive-atlas/onefifth.json
timeout 130 env PYTHONDONTWRITEBYTECODE=1 python3 ux/atlas/scene.py 9 --lens 'roots 2' \
  --primes 101 --driver lanes/ux-drive-atlas/build/adf --output lanes/ux-drive-atlas/prime101.json
timeout 130 env PYTHONDONTWRITEBYTECODE=1 python3 ux/atlas/scene.py '(* ; 5 mod 25)' --lens exp \
  --primes 2 3 5 --driver lanes/ux-drive-atlas/build/adf --output lanes/ux-drive-atlas/exp5.json
timeout 60 bash -c 'for scene in onefifth prime101 exp5; do
  timeout 30 python3 lanes/ux-drive-atlas/check_scene.py lanes/ux-drive-atlas/$scene.json \
    > lanes/ux-drive-atlas/$scene-check.log 2>&1
  timeout 30 node lanes/ux-drive-atlas/check_dom.cjs lanes/ux-drive-atlas/$scene.json \
    lanes/ux-drive-atlas/$scene-dom.json > lanes/ux-drive-atlas/$scene-dom.log 2>&1
done'
```

All three generation commands exit 0. Check results, each with 0 failures:

| Scene | Arithmetic/evidence checks | Replayed records | Depth changes | Focus checks | SVG elements |
|---|---:|---:|---:|---:|---:|
| onefifth | 366 | 13 | 10 | 42 | 264 |
| prime101 | 2754 | 16 | 5 | 910 | 5479 |
| exp5 | 152 | 19 | 15 | 11 | 101 |

1/5: data correctly starts at exponent -1 and has digits 100000 at 5 through absolute depth 5.
The root hover is `[p=5: 0 + O(5^-1)]`; the first lit child is `1/5 + O(5^0)`.
The graph still starts at the same top y-coordinate as the 2-tree and grows downward. There is no visible
exponent or Z_p baseline, so the reader must hover to discover that the top is 5^-1 Z_5. This is a usability
finding against the promised upward extension, not a wrong numeric hover. Periods are explicitly not supplied.

101: allowed by the README, and all numbers check. The page uses the same fan of siblings as at 2.
At row 1 the 101 nodes occupy width 280, hence centres are 280/102 = 2.745... SVG units apart.
Unlit node radius is 2.2, diameter 4.4: adjacent circles overlap. Rows below have 202 siblings and still
less separation. No wheel is drawn and no refusal or warning is given. This is a geometry defect for
accepted input. It is established from emitted coordinates, not from a screenshot.

exp: at 5, depth 1 gives 1 + O(5), depths 2..5 give 6 + O(25). At 2 and 3 the balls are Z_p, so
they contain both a convergent input 0 and an input 1 outside exp's local domain: NOT_DETERMINED.
The finite-only input has no real coordinate; the driver's real call returns DOMAIN and the ruler draws
a solid gate. It lacks a specific explanation that the real input is missing rather than negative.
At 5, for x = 5+25t, x^n/n! is divisible by 25 for every n >= 2: v_5(n!) <= n/4, so
n-v_5(n!) >= 2. Thus exp(x) is 1+x = 6 modulo 25. The first-depth reduction is 1 modulo 5.

The full scene transcript reproduces all statuses. The renderer exposes `failing_place: not given`.
The displayed digit strings and sibling hover centres are computed by the Python builder, as the README
and footer disclose. The literal D-UX5 requirement that every displayed number come from the driver is
not met by these derived digits. The independent arithmetic checks found no wrong values. This is an
honesty/provenance finding, not a claim of concealed fabrication.

Step 4 is complete. The builder report is still unread. Step 5 starts next.

## 5. Builder comparison

2026-10-16T00:21:03+11:00: first read of `lanes/ux-atlas/report.md`.
The report discloses derived digits, missing wheels and periods, no tours or editing, and failed screenshots.
Its test counts match setup. Its paths table matches all 35 prime/depth combinations in the two fixtures.
All described page claims match emitted DOM. Actual browser rendering remains unverified in both lanes.

```sh
timeout 120 node --test --test-isolation=none lanes/ux-atlas/check_viewer.mjs
timeout 120 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/ux-atlas/check_style.py
timeout 30 lanes/ux-drive-atlas/build/adf lanes/ux-drive-atlas/library.cmd
timeout 170 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/ux-drive-atlas/finish_checks.py
```

Read the builder's checks before running them. Its `check_scenes.py` writes to lanes/ux-atlas, so it was
not run. `finish_checks.py` reproduces its seven remaining examples in this lane and uses my independent
checker. The three other example types were already checked as minus1, exp5 and onefifth.
All child commands include timeout. The source contains the exact scene CLI arguments and check commands.

Builder DOM suite: 5 tests, 0 failures, exit 0. This includes picker and served-query handlers with a stub.
Builder style suite: 10 files, 0 long lines, README 79 lines, HTML 401 lines, exit 0.
The new example records and final independent-check results are:

| Scene | Records | Checks | Failures | Real status | Prime results at depth 5 |
|---|---:|---:|---:|---|---|
| logminus1 | 19 | 149 | 0 | DOMAIN | 2 OK/1 path; 3,5 DOMAIN |
| fractionroot | 16 | 408 | 0 | OK | 5 OK/2 paths |
| ballroot | 8 | 127 | 0 | DOMAIN | 2 OK/2 paths |
| coarse | 18 | 109 | 0 | DOMAIN | 2,3,5 OK/1 path each |
| sqrt2 | 46 | 369 | 0 | OK | 2,3,5 OK/empty; 7 OK/2 paths |
| exp0 | 19 | 473 | 0 | OK | 2,3,5 OK/1 path each |
| log1 | 19 | 473 | 0 | OK | 2,3,5 OK/1 path each |

The final finish_checks run exits 0. All seven scenes also load through the actual page code in the DOM
stand-in. It verifies 23 extra arithmetic assertions (12 table squares, 10 lifted-root assertions,
and enclosure of 1/5 by its printed real interval), the overlapping circles, all 11 status names, and
four more builder claims: threads for exp0, log1 and fractionroot; no thread and missing endpoints for sqrt2.

Two earlier finish_checks runs exited 1 due to errors in my checking code, not slice defects:
1. The thread source check assumed a bare output `1`; exp_at correctly prints `real: 1`. I added an exact
   command-dependent prefix expectation, retaining the value equality check. First failure log retained.
2. The header regex used whitespace including newlines and captured ADF_STATUS_COUNT as a status. I restricted
   it to horizontal whitespace before the inline comment. It now finds exactly 11 names. Second log retained.
No slice test or statement was weakened or edited. The final counts below use each final scene once.

### Library demand probes

The 15 commands in `library.cmd` after its precision setting produced 15 lines; exit 1, stderr empty.
`library.out` is retained verbatim. Unknown query names below are explicit exploratory probes, not documented
commands. PARSE alone proves those spellings fail; the driver README also lists no such query interface.

1. Digits/period: `project 1/5 with 5` gives `5: 1/5`. `digits_at ...` and `period ...` give PARSE.
   No digit sequence or period comes from this projection. Builder computation is correctly disclosed.
2. Failing place: `exp_at 1 with 2` gives only `error: DOMAIN`. The actual where output is absent.
3. Partial choices: `show {inf: 3; p=2: 3; p=3: -3; p=5: -3}` round-trips exactly. Thus the parser demand
   is already met, as the builder says. Generic mul on that partial value returns DOMAIN; this does not
   prevent constructing it. The viewer itself has no picker for branch choices.
4. Image batch: a trial `map ... with square` is PARSE. Generic mul on the trial local-ball text is DOMAIN.
   A batch node-image operation is not advertised by the driver. The scene's existing per-place exp is usable.
5. Support/cofactor: `support (* ; 0 mod 3027)` is PARSE. Projection gives constraints only at named places:
   2: O(2^0), 3: O(3^1), 5: O(5^0). It does not print the remaining factor 1009.
6. Real endpoints: `project 1/5 with real` at prec 8 prints `real: 0.19922 +/- 0.00098`, no dyadic storage.
   However, `dump (0.2 ; 1/5)` and `dump {inf: 0.2}` DO expose dyadic storage of supplied inputs.
   The limitation is exporting the actual function/project result through this scene protocol, not an absence
   of all dyadic dump support in the driver. The scene's `not given` on its inexact output is accurate.
7. Cost: trial `cost roots_at ...` and `limits roots_at` give PARSE. No estimate is advertised. The page
   selects stored depths and the builder enforces a five-level cap; neither pretends to give an estimate.

Other `not given` uses: polynomial idempotent seeds are absent from `roots` output; build stamps are absent;
the source hash is present but does not claim to identify a compiled library. Exact dyadic points have
equal endpoints and no `not given`. Periods are not claimed. No raw status or command record was discarded.

The builder's cited digit source exists. Read `refs/src/thorne-padic/jackthornenotes.txt:180-214`, including
the uniqueness statement at 183-188. Our finite arithmetic checks use the stepwise proof above.
Sources pending: none. Findings against SPEC: none. No specification was changed.

2026-10-16T00:24:15+11:00: mathematical checks and builder comparison complete.

Final evidence count: 14 scenes, 7707 scene assertions plus 23 supplementary arithmetic assertions,
0 failed final assertions, 360 replayed driver records, 179 depth changes, 1520 focus checks.
The independent status-name check found 11 names, 0 missing. There are 36 passing builder unit/smoke tests.
The geometry check proves one overlapping-circle defect. Repeated runs during checker debugging are not
added to the counts. No final test failure is hidden; the two checker failures are documented above.

The infinite-prime fog label also has an elementary check: given any finite list of primes, form their
product plus 1. A prime divisor of that integer is on none of the list, since division by each listed
prime leaves remainder 1. Removing four drawn primes therefore still leaves infinitely many.

Final line-length check, run under `timeout 10 python3`, scanned this lane's Markdown, Python and CJS
files: 0 lines over 116. The report is written only after the drive and all comparisons are complete.

## 6. Report complete

Final file check: 5 files, 0 lines over 116, required report sections present.
Finished: 2026-10-16T00:30:05+11:00. Total elapsed from 00:11:41 is recorded by these timestamps.
