# Place Atlas slice report

The builder, viewer, two reproducible scenes, README and tests are finished.
Only ux/atlas/ and lanes/ux-atlas/ were written. No git command or bd command was run.
No C library file, specification, plan, shared document or dependency was changed.

## What was built

scene.py accepts a rational or finite ball, identity / roots n / exp / log, one to four primes,
and depths 1..5. It runs one driver process per scene under timeout 120, with many command lines.
It preserves the complete input and output, settings, command/output pairs, exit code and driver-source hash.
It computes digits, prefix balls, sibling hover text and fog counts from parsed values.
It preserves branch count separately from distinct drawn-path count.
Polynomial certificates can have greater precision than the requested drawing.

Rational root inputs use integer-coefficient polynomials obtained by clearing the denominator.
Additional roots_at calls supply branch identifiers. Matching uses the full certificate precision.
At denominator primes roots_at also supplies the non-integral roots that the polynomial solver omits.
Ball-valued root inputs use roots_at, since an uncertain coefficient is not an integer polynomial.
The idempotent scene explicitly overrides the polynomial to X^2-X; its input footprint is 0 mod 30.
Eight modular residues are enumerated, then checked with driver show, mul, contains and project calls.

atlas.html has inline CSS and interaction code, plus the shared pure geometry in atlas.js.
Its palette, SVG classes and striped fog pattern reuse docs/ux/mock-2-place-atlas.html.
Fonts are local fallbacks. No external library, font, package or service is used.
The file picker works through File.text(); served query loading uses ?scene=scenes/NAME.json.
Depth handles select stored results. The page does not compute p-adic values.
It draws the real ruler, up to four trees, rational threads, status shapes and fog.
Only lit parents grow children; other continuations are cones. Geometry omits siblings above depth 6.
All node hover balls, including unlit siblings, are supplied by the builder.

## Files written

- ux/atlas/scene.py
- ux/atlas/atlas.html
- ux/atlas/atlas.js
- ux/atlas/build_scenes.sh
- ux/atlas/scenes/sqrt9.json
- ux/atlas/scenes/idempotents30.json
- ux/atlas/README.md
- ux/atlas/test_scene.py
- ux/atlas/test_atlas.mjs
- ux/atlas/.gitignore
- lanes/ux-atlas/check_scenes.py
- lanes/ux-atlas/check_viewer.mjs
- lanes/ux-atlas/check_style.py
- lanes/ux-atlas/.gitignore
- lanes/ux-atlas/report.md

Generated lane artifacts: build/ contains the isolated C objects, archive and driver; regen/ contains two
reproduced scenes. Ten additional JSON scenes are minus1, exp5, logminus1, fraction, fractionroot, ballroot,
coarse, sqrt2, exp0 and log1. Two screenshot harnesses are sqrt9-screenshot.html and
idempotents30-screenshot.html. The harnesses inject fixture data into a copy of the page for headless capture.
They are not screenshots. Build products, regeneration copies and harnesses are ignored locally.
Python also created ignored __pycache__ files. No PNG was produced.

## Checks run

The repository had no build/adf. The driver was compiled from the current sources within this lane:

```sh
timeout 170 make -j2 BUILD=lanes/ux-atlas/build all
timeout 120 cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
  tools/adf/adf.c lanes/ux-atlas/build/libadelefeld.a -lflint -lgmp -lm \
  -o lanes/ux-atlas/build/adf
```

Both commands exited 0. The make used at most two jobs. The normal tools/adf Makefile was read but not run,
because it writes outside the owned paths. No C tests were run; this lane changes no C code.

An initial format probe used `timeout 120 /home/tobias/Projects/adelefeld/build/adf`, exit 0, nine output lines.
The probe commands, after prec 64 and digits 20, were project 9 at real/2/3/5/7; realroots -9 0 1;
roots -9 0 1 at 2, 3 and 5 at precision 1; realroots 0 -1 1; roots 0 -1 1 at 2 at precision 1;
and root 9 with degree 2 with each sign. It established the exact points -3 and 3 and the deeper certificates
at 2 and 3. All saved scenes were then generated and replayed with the isolated current-source driver.

```sh
timeout 170 env ADF=lanes/ux-atlas/build/adf sh ux/atlas/build_scenes.sh
timeout 170 env ADF=lanes/ux-atlas/build/adf ux/atlas/build_scenes.sh lanes/ux-atlas/regen
timeout 120 diff -ru ux/atlas/scenes lanes/ux-atlas/regen
```

Each command exited 0. Two scenes were regenerated; diff wrote zero lines.
Regeneration was also run using sh for the alternate output directory, with the same zero-line diff.
sqrt9 has 46 command/output records and driver exit 0. idempotents30 has 51 records and driver exit 1:
its finite input has no real coordinate, so the input projection at real is error: DOMAIN.
The polynomial realroots call is independent and returns 0; 1. The builder preserves both results.

```sh
timeout 120 python3 -m unittest discover -s ux/atlas -p 'test_scene.py'
timeout 120 python3 -m unittest discover -s ux/atlas -v
timeout 120 python3 -m unittest discover -s ux/atlas
```

Initial red run: exit 1, one import error before scene.py existed. First green run: 14 tests, zero failures.
The expanded suite passed 21 tests. A regression test then failed one of 22 tests: coarse drawing precision
had assigned the same seed to two distinct certificates. Matching at certificate precision repaired it.
Final runs: exit 0, 22 tests, zero failures and zero errors.
Also run from ux/atlas/: `timeout 120 python3 -m unittest`; 22 tests, zero failures, exit 0.

```sh
timeout 120 node --test ux/atlas/test_atlas.mjs
timeout 120 node ux/atlas/test_atlas.mjs
timeout 120 node --test --test-reporter=spec ux/atlas/test_atlas.mjs
timeout 120 node --test --test-isolation=none ux/atlas/test_atlas.mjs
timeout 120 node --test --test-isolation=none lanes/ux-atlas/check_viewer.mjs
timeout 120 node --test --test-isolation=none ux/atlas/test_atlas.mjs lanes/ux-atlas/check_viewer.mjs
```

Initial red Node run: exit 1, one failed test file before atlas.js existed.
Initial pure-function suite: seven tests, zero failures. With fixture checks: nine tests, zero failures.
The standard isolated --test reporter on this machine reports one passed file, exit 0.
Direct execution reported seven tests at that stage. The explicit isolation=none run reports all subtests.
Final combined run: exit 0, 14 tests, 14 passed, zero failed, zero skipped.
Nine test geometry, siblings, statuses, stored depths and fixture hover-ball coverage.
Five exercise the inline page code with a minimal DOM: Scenario A, Scenario B, picker, served query and gate.
These five are structural smoke checks, not a browser-engine or visual test.

```sh
timeout 170 python3 lanes/ux-atlas/check_scenes.py --driver lanes/ux-atlas/build/adf
timeout 120 python3 lanes/ux-atlas/check_style.py
```

Scene check: exit 0, two transcript replays matched stdout/stderr/exit code exactly, 46 and 51 output lines.
Ten additional scenes were built and their digit congruences checked. Their maximum-depth results are:

| Scene | Records | Real status | Prime: status, distinct paths |
|---|---:|---|---|
| minus1 | 46 | OK, empty | 2: OK,0; 3: OK,0; 5: OK,2; 7: OK,0 |
| exp5 | 19 | DOMAIN | 2: NOT_DETERMINED,0; 3: NOT_DETERMINED,0; 5: OK,1 |
| logminus1 | 19 | DOMAIN | 2: OK,1; 3: DOMAIN,0; 5: DOMAIN,0 |
| fraction | 13 | OK | 2: OK,1; 5: OK,1 |
| fractionroot | 16 | OK | 5: OK,2 |
| ballroot | 8 | DOMAIN | 2: OK,2 |
| coarse | 18 | DOMAIN | 2: OK,1; 3: OK,1; 5: OK,1 |
| sqrt2 | 46 | OK | 2: OK,0; 3: OK,0; 5: OK,0; 7: OK,2 |
| exp0 | 19 | OK | 2: OK,1; 3: OK,1; 5: OK,1 |
| log1 | 19 | OK | 2: OK,1; 3: OK,1; 5: OK,1 |

The fraction scene starts at exponent -1 at 5; fractionroot has threads -1/5 and 1/5.
The exp5 output stops at exponent 2 at 5 even when depth 5 is requested.
sqrt2 has no rational thread and marks its real dyadic endpoints not given.
exp0 and log1 have the driver-certified rational threads 1 and 0 respectively.

Style check: exit 0, ten source/document files, zero lines over 116 characters, README 79 lines,
HTML 401 lines. Its preceding inline version first failed the strict README bound at 80 lines;
after shortening the README it passed at 79. No bound was relaxed.

The two README example scene commands were also run with --driver lanes/ux-atlas/build/adf:

```sh
timeout 120 python3 ux/atlas/scene.py -1 --lens 'roots 2' --primes 2 3 5 7 \
  --driver lanes/ux-atlas/build/adf --output lanes/ux-atlas/minus1.json
timeout 120 python3 ux/atlas/scene.py '(* ; 5 mod 25)' --lens exp --primes 2 3 5 \
  --driver lanes/ux-atlas/build/adf --output lanes/ux-atlas/exp5.json
```

Both exited 0. Results are in the table above. No system package or Python dependency was installed.

### Screenshot attempts

```sh
timeout 40 firefox --headless --profile lanes/ux-atlas/firefox-profile \
  --window-size 1680,1180 --screenshot lanes/ux-atlas/sqrt9.png \
  file:///home/tobias/Projects/adelefeld-wt/ux-atlas/lanes/ux-atlas/sqrt9-screenshot.html
timeout 40 firefox --headless --profile lanes/ux-atlas/firefox-profile-b \
  --window-size 1680,1180 --screenshot lanes/ux-atlas/idempotents30.png \
  file:///home/tobias/Projects/adelefeld-wt/ux-atlas/lanes/ux-atlas/idempotents30-screenshot.html
```

Both exited 139. Each printed the headless notice and a core-dump notice. Neither produced a PNG.
Firefox was not retried beyond these two scene attempts. No claim of visual verification is made.

## Lit paths in the two fixtures

Each entry below is the count of distinct drawn paths, not the number of certificate records.
Every prime/depth entry in both fixtures has two branch certificates and status OK.

| Scene | Prime | Depth 1 | Depth 2 | Depth 3 | Depth 4 | Depth 5 |
|---|---:|---:|---:|---:|---:|---:|
| sqrt9 | 2 | 1 | 2 | 2 | 2 | 2 |
| sqrt9 | 3 | 1 | 2 | 2 | 2 | 2 |
| sqrt9 | 5 | 2 | 2 | 2 | 2 | 2 |
| sqrt9 | 7 | 2 | 2 | 2 | 2 | 2 |
| idempotents30 | 2 | 2 | 2 | 2 | 2 | 2 |
| idempotents30 | 3 | 2 | 2 | 2 | 2 | 2 |
| idempotents30 | 5 | 2 | 2 | 2 | 2 | 2 |

## What the page shows in Scenario A

1. Load sqrt9.json with the picker. The header shows 9, roots 2, and real/2/3/5/7.
   There is no live value-entry command bar in this static slice.
2. The ruler has the exact points -3 and 3. At default depth 5 every tree has two lit paths.
   The fog bank counts infinitely many undrawn primes, as an input footprint only.
3. Amber and blue rational threads run from their ruler points into every tree; shared prefixes are white.
4. Branch picking and a braid are outside this slice. Clicking cannot create a claimed partial product.
5. Generate and pick minus1.json. The ruler and 2/3/7 trees show complete empty polynomial lists.
   The 5-tree has two paths. These driver calls return none, so the page does not invent DOMAIN gates.
6. Depth handles choose 1..5 independently. At depth 1 the 2/3 trees each have one distinct path but two
   branches; deeper stored results separate them. There is no depth 12 handle or new driver call.

## What the page shows in Scenario B

1. Load idempotents30.json and set each handle to 1. Each tree lights first digits 0 and 1.
2. The table gives the eight choices in prime order 2,3,5:
   0:(0,0,0), 1:(1,1,1), 6:(0,0,1), 10:(0,1,0), 15:(1,0,0), 16:(0,1,1), 21:(1,0,1), 25:(1,1,0).
   They are modular choices; eight rational root threads would be false. The table is not a braid editor.
3. Set the 2-tree to 4. It still has two paths, 0 and 1 modulo 16. Every table row has a driver true check
   of e^2 in e + 30 Zhat; the transcript also contains the exact driver square.
4. The fog bank says every other p, with zero undrawn modulus-support primes below 1000.
   It does not claim to have computed polynomial roots at undrawn primes. There is no agent narration.
5. Folding 5 and changing to modulus 36 require a different scene; these editing actions are outside the slice.

The walkthrough describes the generated records and inline-code smoke checks. It is not a GUI test claim.

## Library demands met by the slice

1. Digits and paths: finite prefixes and hover balls are derived by the builder from printed centres.
   The page does no expansion. Pre-period and period are not given; no period is claimed.
2. Records with places: commands, outputs and the named call place are retained. The actual failing-place
   output is not given by the driver. That field stays "not given"; the page labels the named call's gate.
3. Partial balls from choices: the driver already accepts their value form, as documented. This slice reads
   projection/function output forms; constructing and editing braids is outside it.
4. Image balls for a map view: not implemented. This viewer lays out one scene, not a batch node-image map.
5. Support without factorisation: primes below 1000 dividing the rational modulus numerator or denominator
   are counted when undrawn. No complete factorisation or unfactored-cofactor claim is made.
6. Exact real endpoints: only exact dyadic point outputs identify both endpoints. Inexact ball endpoints
   are "not given" and use a plain marker. Printed midpoint/radius digits are preserved without alteration.
7. Cost estimate: not given by the driver. Build calls are bounded by timeout 120; handles use stored data.
   This implementation also bounds primes below 1000 and root degree at 64 to keep scene size finite.

## What is not done

No agent, tour, editing, braid, product-formula balance, zoom beyond the handle, or more than four trees.
No rational periods, residue wheels, full factorisation, exact inexact-real endpoints or cost-estimate query.
The prime and degree bounds above are additional builder bounds; larger requests are rejected explicitly.
The two screenshots and actual browser rendering remain unverified because Firefox exited 139.
No parallel model test drive was started by this lane. The orchestrator can run the planned test drive.

## What the test driver should try

Follow ux/atlas/README.md alone. Pick both fixture files from file://, then move every depth handle.
Inspect the coarse path collisions and their distinct seed labels in sqrt9, and all eight table rows in B.
Compare node hover text with the transcript and use keyboard focus as well as pointer hover.
Try lanes/ux-atlas/exp5.json for dashed gates and lanes/ux-atlas/logminus1.json for solid DOMAIN gates.
Try lanes/ux-atlas/sqrt2.json for the plain missing-endpoint markers.
When already served, open each documented ?scene= link; from file:// use the picker.

## Sources and sources pending

The digit convention was read in refs/src/thorne-padic/jackthornenotes.txt:180-214, Proposition 1.10.
The code quotes its uniqueness statement at lines 183-188 and writes out its finite modular computation.
The modulus-support test and bounded modular enumeration are direct finite arguments written in the code.
Driver grammar and status spellings come from the required repository documents and status.h.
Sources pending: none for the implemented arithmetic. Missing driver outputs are listed above as not given.

## Findings against the specification

None. No specification statement was changed or weakened, and no counterexample was found.

## Findings against the brief

- Two root certificates need not give two distinct paths at a shallow drawing depth. At 2 and 3, sqrt9
  has only one distinct depth-1 path. Both branch records and the collision are displayed.
- Polynomial roots/realroots print none for empty complete lists, rather than DOMAIN. The -1 scene keeps
  that distinction. Gates represent actual error statuses, not an inferred replacement status.
- Polynomial certificate lists have no branch identifiers. Rational root scenes obtain them from additional
  roots_at calls; the idempotent polynomial's seed field remains not given.
- The text protocol omits the stored inexact-real dyadic endpoints and the failing-place output.
  The scene and viewer disclose these omissions. A decimal enclosure was not passed off as stored endpoints.
