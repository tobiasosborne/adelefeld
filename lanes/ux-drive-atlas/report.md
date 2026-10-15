# Atlas test-drive report

The two supplied scenes reproduce and their numbers check. One rendering defect remains for an accepted
large prime. There are four usability findings and one disclosed departure from D-UX5.
No wrong arithmetic, dropped driver status, or counterexample to SPEC was found.

Only `lanes/ux-drive-atlas/` was written. No slice file was changed. No git command, bd command, package
installation or subagent was used. The builder report was first read at step 5, after both scenarios and
the three extra promises had been tried. This is a test drive, not a code review or a repair.

## DEFECTS

### D1. Accepted prime 101 produces overlapping nodes

The README allows primes below 1000. This command succeeds, exit 0:

```sh
timeout 130 env PYTHONDONTWRITEBYTECODE=1 python3 ux/atlas/scene.py 9 --lens 'roots 2' \
  --primes 101 --driver lanes/ux-drive-atlas/build/adf --output lanes/ux-drive-atlas/prime101.json
timeout 30 node lanes/ux-drive-atlas/check_dom.cjs lanes/ux-drive-atlas/prime101.json \
  lanes/ux-drive-atlas/prime101-dom.json
```

Actual output: 5479 SVG elements, including 101 circles on the first digit row. The first two centres
are 2.7450980392156907 units apart; their radii sum to 4.4. Thus the circles overlap. The next digit rows
have 202 nodes each and less separation. All 2754 arithmetic/evidence checks on this scene pass.

Expected: readable residue wheels, as promised by The surface: "Above p = 13 a tree is drawn as a
residue wheel"; alternatively, an explicit unsupported-range refusal until such a view exists.
The geometric contradiction is direct: 280/102 < 2.2+2.2. It does not depend on a screenshot.
Evidence: `prime101-dom.json`, `prime101-check.log`, and the D1 line in `finish-checks.log`.

No other defect was established. In particular, Firefox's exit 139 is not attributed to the page.
The README's ordinary build commands were not executed literally because they write outside this lane.
Its documented isolated-build route and alternate scene-output directory both worked.

## USABILITY

Listed in order of cost to a user following the brief. Explicit first-slice exclusions are identified.

1. U1. Both scenarios become a file-generation workflow when the user wants to change the mathematics.
   Promises: "The human types 9 into the address bar"; "pick different branches at different places";
   "The agent resumes from the human's camera". The address and lens are labels. Branch clicks, braids,
   tours, folding and handover are absent; depth 12 is refused with a precise five-level limit.
   These exclusions are honest and mostly deliberate first-slice limits. The modulo-36 follow-up is
   possible through a new CLI scene: its four checked residues are 0,1,9,28. It cannot resume a page camera.
2. U2. Negative valuation is correct only after inspecting hover text. Promise: "the tree grows upward
   above the root by -v levels". For 1/5 at 5, the top node is 5^-1 Z_5 and the first digit belongs to
   exponent -1, but the graph starts at the same height as an ordinary integral tree and grows downward.
   Neither an exponent scale nor the Z_5 baseline is visible. All 366 evidence checks for 1/5 pass.
3. U3. Missing input and a failed function domain look alike. Promises: "Every dark place has a one-line
   reason under it" and "a status belongs to a place". The README's finite-only exp example puts DOMAIN
   on the real ruler because no real coordinate was supplied. The gate only adds "Reported failing
   place: not given". It does not explain this cause. The raw status is correct and is retained.
4. U4. The idempotent lens title is ambiguous. Promise: Scene 1 lights the solutions of x^2 = x.
   The top label says `roots 2`; the map's accessible label describes roots of `(* ; 0 mod 30)`.
   The actual equation X^2-X appears only below the map as coefficients `0 -1 1`.
   The eight table rows are correct, but the equation should be visible at the point of interpretation.

## HONESTY

H1. The literal D-UX5 rule is not satisfied by digits, sibling hover centres and fog counts computed in
the Python builder. D-UX5 requires every displayed number to come from the driver or Julia.
The builder, README and footer disclose this derivation; it is not concealed fabrication.
The digit values and hover balls passed independent modular checks. The fixture transcript contains their
source centres but does not itself print the digit arrays. The next slice needs either driver digit output
or an explicit owner-approved change to the rule; this lane changed neither the rule nor the implementation.

The other tested honesty boundaries hold:

- Empty polynomial root lists remain OK/empty. They are not relabelled DOMAIN. Separate local-root calls
  on -1 do return DOMAIN at 2,3,7, and both outputs are preserved in the scene's transcript.
- The fog says input footprint only and lens not computed there. It does not claim roots at undrawn primes.
- A modular solution such as 6 is a table residue, not a rational root thread.
- `failing_place: not given` is accurate: `exp_at 1 with 2` prints only `error: DOMAIN`.
- Polynomial branch seeds marked `not given` are absent from the polynomial solver's output.
  The sqrt9 seeds are obtained by extra roots_at calls and all 40 stored labels check.
- Inexact real output endpoints are marked `not given`; exact dyadic points have equal endpoints.
  Driver dumps already expose dyadic storage for supplied inputs. The unmet demand is exporting the
  actual computed function/project output in that form, not all support for dyadic dumps.
- Periods, build stamps and cost estimates are not claimed. Stored depths are disclosed as stored data.
- No discarded status or non-reproducing transcript was found in 360 replayed records.

## VISION

The slice makes the case for comparing local branches and digits on one map.
It does not yet demonstrate braids or a human resuming an agent's tour.
The next slice would benefit most from branch selection with a saved partial-product scene.

## Checks and results

Read `CLAUDE.md`, the lane rules, UX decisions, the atlas brief, SPEC 2 and 9.3.1-9.3.3, PLAN 1F,
then the slice README. The first three read-only shell calls lacked timeout; later shell commands and
all builds, tests and scripts used timeout. No run used more than two worker cores or exceeded 170 seconds.

Setup commands, from the repository root:

```sh
timeout 170 make -j2 BUILD=lanes/ux-drive-atlas/build all
timeout 120 cc -Iinclude tools/adf/adf.c lanes/ux-drive-atlas/build/libadelefeld.a \
  -lflint -lgmp -lm -o lanes/ux-drive-atlas/build/adf
timeout 170 env PYTHONDONTWRITEBYTECODE=1 ADF=lanes/ux-drive-atlas/build/adf \
  sh ux/atlas/build_scenes.sh lanes/ux-drive-atlas/scenes
timeout 10 diff -ru ux/atlas/scenes lanes/ux-drive-atlas/scenes
timeout 120 env PYTHONDONTWRITEBYTECODE=1 ADF=lanes/ux-drive-atlas/build/adf \
  python3 -m unittest discover -s ux/atlas
timeout 120 node --test --test-isolation=none ux/atlas/test_atlas.mjs
```

All exit 0. Two scenes reproduce with a zero-line diff. Python: 22 tests, 0 failures. Node: 9 tests,
0 failures. The diff was originally run inside a timeout 10 shell; the line above is the equivalent command.

Every scene below was checked with these commands, substituting its input and output paths:

```sh
timeout 30 python3 lanes/ux-drive-atlas/check_scene.py SCENE.json
timeout 30 node lanes/ux-drive-atlas/check_dom.cjs SCENE.json lanes/ux-drive-atlas/NAME-dom.json
```

The two supplied inputs are `ux/atlas/scenes/sqrt9.json` and `ux/atlas/scenes/idempotents30.json`.
The other inputs are in this lane. Each invocation's output is retained in `NAME-check.log` and
`NAME-dom.log`; the idempotents30 log prefix is `idempotents`. Exact generation commands are in `drive.md`
and `finish_checks.py`. The checker uses integer/Fraction arithmetic and imports no slice implementation.

| Scene | Evidence assertions | Replayed records | Final failures |
|---|---:|---:|---:|
| sqrt9 | 1279 | 46 | 0 |
| minus1 | 319 | 46 | 0 |
| idempotents30 | 622 | 51 | 0 |
| idempotents36 | 107 | 24 | 0 |
| onefifth | 366 | 13 | 0 |
| prime101 | 2754 | 16 | 0 |
| exp5 | 152 | 19 | 0 |
| logminus1 | 149 | 19 | 0 |
| fractionroot | 408 | 16 | 0 |
| ballroot | 127 | 8 | 0 |
| coarse | 109 | 18 | 0 |
| sqrt2 | 369 | 46 | 0 |
| exp0 | 473 | 19 | 0 |
| log1 | 473 | 19 | 0 |

The checks include complete transcript stdout/stderr/exit-code equality, result/status fields, digits,
prefix and sibling hover balls, counts, rational threads, exact real roots, and modular solution lists.
The page code is executed in a small Node DOM stand-in: 14 scene loads, 179 depth changes and 1520 focus
checks. This validates emitted elements and handlers, not browser layout or real pointer interaction.

Additional scenario probes:

```sh
timeout 30 lanes/ux-drive-atlas/build/adf lanes/ux-drive-atlas/scenario-a.cmd
timeout 30 env PYTHONDONTWRITEBYTECODE=1 python3 ux/atlas/scene.py -1 --lens 'roots 2' \
  --primes 5 --depth-max 12 --driver lanes/ux-drive-atlas/build/adf \
  --output lanes/ux-drive-atlas/minus1-depth12.json
```

Both exit 1 as expected. The driver has four DOMAIN lines; its successful lines certify roots of 9 at
7,11,13 and roots of -1 at 5 to depths 8 and 12. The builder says
`scene: depth range must satisfy 1 <= min <= max <= 5` and creates no depth-12 scene.
At depth 8 the two 5-adic centres are 280182 and 110443; at depth 12 they are 123327057 and 120813568.
Their squares are -1 modulo the corresponding powers, and the deeper centres reduce to the earlier ones.

Step 5 reruns and probes:

```sh
timeout 120 node --test --test-isolation=none lanes/ux-atlas/check_viewer.mjs
timeout 120 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/ux-atlas/check_style.py
timeout 30 lanes/ux-drive-atlas/build/adf lanes/ux-drive-atlas/library.cmd
timeout 170 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/ux-drive-atlas/finish_checks.py
```

DOM suite: 5 tests, 0 failures. Style: 10 files, 0 lines over 116; README 79 lines, HTML 401 lines.
Library probes: exit 1, 15 output lines, stderr empty. All seven numbered demands are discussed in
`drive.md`: partial-ball construction already works; failing-place, digit/period, batch-map, support/cofactor
and cost query outputs are absent from the tested interface. Unknown query spellings return PARSE.
The actual scene output lacks dyadic endpoints even though input dumps contain dyadic storage.

Final finish_checks: exit 0. It reproduces the builder's seven remaining examples, adds 23 arithmetic
assertions, checks 11 status names, confirms four additional thread/endpoint claims, and proves D1.
It was run three times: the first two exits were 1 because my checker mishandled a `real:` prefix and
matched a count macro across a newline. Both mistakes were corrected without changing the slice or
weakening value equality. The two failure logs are retained as `finish-checks-first.log` and
`finish-checks-second.log`. Repeated checks are not added to the final counts.
The builder's check_scenes script was not run directly because it writes into its own lane; our equivalent
commands keep all generated files here. No historical red-test state was recreated.

Screenshot command:

```sh
timeout 30 taskset -c 0,1 firefox --headless --profile lanes/ux-drive-atlas/firefox-profile \
  --screenshot lanes/ux-drive-atlas/initial.png file://$PWD/ux/atlas/atlas.html
```

Exit 139; output: `*** You are running in headless mode.` followed by
`timeout: the monitored command dumped core`. No PNG exists. This agrees with the builder's limitation.
No screenshot content is claimed. Firefox was not repeatedly retried.

## Counts and time

- Six required stages completed; 14 scenario/extra actions: A1-A6, B1-B5, and three additional promises.
- Fourteen scenes; 7730 arithmetic/evidence assertions, including 23 supplementary assertions; 0 final failures.
- 360 replayed driver records; 179 depth changes; 1520 focus checks; 36 passing builder unit/smoke tests.
- One defect; four usability findings; one honesty/provenance finding; zero findings against SPEC.
- Start 2026-10-16T00:11:41+11:00; investigation finished 00:24:15, 12 minutes 34 seconds.
  Report and file checks finished at 00:30:05: total elapsed 18 minutes 24 seconds.

Final validation used `timeout 10 python3 -`: five authored files, zero lines over 116, all required report
sections present, and three vision lines. Result: exit 0, retained in `final-check.log`.

## FOR THE REPAIR

Smallest first. Items after 1 are improvements or declared scope work, not additional arithmetic defects.

1. Make prime 101 readable or refuse it explicitly. Test: generated SVG has no overlapping sibling circles,
   or the builder returns a clear unsupported-range message before writing a scene.
2. Display the effective polynomial equation beside the lens. Test: the idempotent scene title and accessible
   label say X^2-X = 0 rather than implying square roots of its finite input footprint.
3. Explain missing real input at its gate. Test: the README exp example says the finite input has no real
   coordinate, while retaining the raw DOMAIN status and `where: not given` distinction.
4. Mark exponent zero for negative valuations. Test: 1/5 at 5 visibly has a level above Z_5 and labels the
   first digit as exponent -1; hover text must retain the checked balls.
5. Resolve H1 with the owner and driver tier. Test: each displayed digit has actual driver provenance, or
   an approved rule explicitly permits and labels derived finite digit data. Do not silently alter D-UX5.
6. For the next interaction slice, save branch selections as a partial-product scene. Test: +3 at real/2
   and -3 at 3/5 creates that exact partial value, removes the rational-thread claim, and survives replay.

## Files written and work not done

Written here: `drive.md`, this report, `check_scene.py`, `check_dom.cjs`, `finish_checks.py`,
`scenario-a.cmd`, `library.cmd`, their `.out`/`.err` files, all listed scene JSON and DOM JSON files,
per-check logs, build logs, `scenes/` regeneration copies and `build/` objects/archive/driver.
The lane's existing brief and harness logs were not edited by hand. No screenshot was produced.

Not done: repairs, live browser verification, literal commands writing outside ownership, C library tests,
long fuzzing, mutation tests, served browser use, or a live tour. No such work is claimed.

## Sources pending

None. The finite digit, square, idempotent, exp-mod-25 and infinite-prime arguments are written stepwise
in `drive.md`. The builder's cited digit source was verified on disk at
`refs/src/thorne-padic/jackthornenotes.txt:180-214`, including its uniqueness statement at 183-188.
Repository driver grammar and status spellings were read directly from their documented files.

## Findings against the specification

None. No specification was edited or weakened. Scope gaps above are against the vision or UX decisions.
