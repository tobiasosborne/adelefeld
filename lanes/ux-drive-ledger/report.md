# Ledger test-drive report

Verdict: the computed scenario values reproduce, but the slice needs repairs before its states are trusted.
There are 4 defects, 6 usability findings and 4 honesty findings. Findings overlap where stated.
No file under ux/ was changed. No specification was changed. No git state command or bd was run.
The builder's report was first read after drive steps 1 to 4.

## What was done

Completed the 6 stages of lanes/UX-DRIVE.md, including all 10 scenario steps and 3 extra promises.
Built the missing driver, then used a lane-local copy for importing, sealing and rendering.
Independently reran 99 scripts: 1,720 output lines and all 99 exit codes match the stored seals.
The 13 computed scenario scripts produce 48 lines; all match. The remaining scenario claim has no script.
Checked both scenarios' HTML content, links, digit differences, counts and filter logic with Node.
Enumerated square residues for all 24 odd primes from 3 to 97. All 24 signs agree with the driver.
Read both cited sources of the family claim under refs/. Checked the FLINT note with FLINT itself.
Repeated the builder's test kinds, including one bounded mutation and the reopening recipe.

The 100 original claims remain: 94 sealed, 4 refuted, 1 open, 1 proposed, 0 ratified.
Import, seal and render reproduce all 200 claim/seal/page files byte for byte.
The 16 builder unit tests pass. One LIMIT-to-proved-no mutant is killed by one assertion failure.

## DEFECTS

### D1. The first README command fails when copied literally

Exact command, in a fresh noninteractive shell:

```sh
timeout 600 make -j2 && timeout 600 make -C tools/adf     the library and the driver build/adf
```

Exit 2. Output: `make: *** No rule to make target 'the'. Stop.`
Expected: exit 0 and a built driver. The trailing explanation needs a comment marker or a separate line.
The intended build, without that explanation, exits 0. No README fix was applied.
Evidence: `readme-literal.log`, `build.log`. Other README command lines also contain unmarked explanations.
Only the literal first line was tested as a failing copy-and-paste command.

### D2. A claimed place becomes a computed place without a driver call

Command:

```sh
timeout 180 python3 -B lanes/ux-drive-ledger/extras.py
```

The `drive-footprint` claim copies b6's 24 driver calls and adds 101 only to `places`.
The sealer exits 0 and reports `drive-footprint sealed first seal`.
The page has a filled `class="c wide"` span labelled `11 ... 101`.
Its title lists 22 primes, including 101. The script has no command at 101.
Expected: 101 remains named but uncomputed, or the inconsistent claim is rejected.
This follows directly from the brief: a filled box denotes a place where the claim was computed.
The sealer has only 24 output lines, for the 24 original primes. There is no 25th computation to justify 101.
Evidence: `extra-ledger/claims/drive-footprint.json`, `extra-ledger/ledger.html`, `status-checks.log`.
The first extra probe missed the compressed span; the follow-up checks its class and title explicitly.

### D3. A prefixed NOT_DETERMINED result is marked REFUTED

Command:

```sh
timeout 180 python3 -B lanes/ux-drive-ledger/status-checks.py
```

The reproduction is `status-ledger/claims/drive-prefixed-open.json`, with an expected numeric line `0`.
Its script is:

```text
prec 128
tate_gamma ((1 +/- 0.001) + (0)*i ; 0) with 3 with ((1) + (0)*i ; 0) with char(q=1, n=1, s=(0) + (0)*i)
```

Actual output:

```text
continuation S=((1 +/- 0.001) + (0)*i ; 0): error: NOT_DETERMINED
```

Observed: sealer exit 1, verdict `refuted`, page `REFUTED`, no dashed status badge.
Expected: verdict `open` and the D-UX4 open-class badge. No line proves a contradiction.
The raw status text remains visible, but its state and shape contradict it.
Raw `error: LIMIT` is handled correctly as open; raw `error: PARSE` is handled correctly as refuted.
This is also relevant to imported fixtures: tate-gamma and tate-fe already contain prefixed error lines.
Evidence: `status-checks.log`, `status-ledger/ledger.html`.

### D4. The a6 FLINT equality note is false as written

Commands:

```sh
timeout 175 cc -std=c11 -Wall -Wextra -Werror lanes/ux-drive-ledger/flint-equality.c \
  -lflint -lgmp -o lanes/ux-drive-ledger/flint-equality
timeout 175 lanes/ux-drive-ledger/flint-equality
```

Observed, exit 0:

```text
exp success: 1 1
exp(3): 958
exp(12): 5125
precision: 8 8
padic_equal: 0
```

The displayed a6 statement says the equality test returns true for these values at precision 8.
It returns false. The note is proposed, so this is not a false seal, but it is a false factual explanation.
The driver independently prints the same two centres. Their difference is 4167, divisible by 9, not by 27.
FLINT's documented comparison tests unit and valuation: refs/src/flint-3.0.1/padic.rst:263-266.
Ignoring precision metadata does not make two different stored centres equal.
Expected: remove or correct the stronger equality claim, with a reproducible probe attached.
Evidence: `flint-equality.c`, `flint-equality.log`.

## USABILITY

Ordered by cost to a user of the brief.

1. U1. The referee cannot finish the review. Scenario A promises: "ratifies the two sealed claims" and
   "exports a referee report". There are no ratify, comment or export controls. The README discloses the
   lack of a ratification command; the builder report discloses all three omissions. JSON editing can set
   ratified, but provides no review thread or exported report. This is outside the implemented first slice.
2. U2. Source-gated ratification does not exist. Scenario B promises refusal "until the argument cites a
   local source by file and line under refs/". `drive-citation` removes every citation and sets the documented
   `state` field to `ratified`; it still displays RATIFIED. No authenticated actor is recorded. This is a
   missing workflow, not a demand for authentication machinery in this static slice.
3. U3. Branch ambiguity is accepted. The surface promises rejection of "the square root of -1 at 5 is 2 mod 5"
   without a seed. That exact statement, with an all-roots script returning both branches, seals successfully.
   Expected: require a branch or an explicit all-branches statement. No branch-choice aid appears.
4. U4. The claim bar and scratchpad are absent. Scenario A promises typing four lines "in the claim bar".
   The actual entry method is copying and editing JSON, including exact expected driver text and exit status.
   Dry-run does expose the calls before execution. The product formula also needs a manually copied result.
5. U5. The morning queue cannot prioritize review. The interaction model promises ordering by "what a person
   must decide". Available controls are six state buttons and one question select. Scenario B has 5 sealed
   claims, not 9, and no ARGUED state or scope-gap filter. Its 8 rows include the 2 children of the open claim.
6. U6. Imported footprints omit places. The surface promises "places where that assertion has actually been
   computed". There are 29 empty imported footprints; 20 imported scripts contain `with real`, and none of
   those footprints names R. This under-reports evidence, unlike D2's over-reporting. The builder acknowledges
   the extraction limitation, but reports 19 real-bearing claims; my literal scan finds 20.

The three extra promises were U3, U2 and D2. All three accept a state that the vision says should be refused
or displayed differently. The branch and citation cases are recorded as scope limitations, not extra defects
in the exact-output comparison algorithm. The documented slice deliberately has no semantic claim checker.

## HONESTY

1. H1. D2 is uncomputed evidence shown as computed. Claim metadata supplies the filled places without evidence
   for each place. U6 is the reverse provenance gap: real and prime computations can be omitted from the strip.
2. H2. U2 lets an unsupported all-primes argument display RATIFIED. U3 lets an ambiguous branch statement seal.
   The footer correctly says a seal records what a build computed, not mathematical truth. That helps, but
   does not enforce the promised branch and citation rules. The supplied b6 citations themselves are valid.
3. H3. D3 hides the status class, though the original error text survives. The 11 names are present in the
   legend, and raw error lines have the requested class shapes. Thus the defect is prefixed-line handling.
4. H4. D4 is an uncomputed and false note, marked proposed. This is an honest lack of a seal, but a bad claim
   to present as the explanation of a refutation. The b2 product formula is more careful: its argument says
   the norm line was promoted, and that its ball contains 1. It does not compute an exact real equality.

"Not given" audit:

- Failing place: correct for the tested root status. Even `adf -v` prints no failing-place field.
- Dump of a result: correct as a missing automatic record field. Five tested typed dump commands work;
  the driver cannot attach a dump automatically to a preceding computed result without another typed input.
- The a6 empty script is correctly disclosed. No driver command in the slice computes FLINT equality.
- Root certificates and reconstruction certificates are not printed. Values and branches are printed.
- The page has no claim-entry, ratification, comment, split, citation or export controls hidden elsewhere in HTML.

## Builder comparison and library demands

The build, 86 imports, 100-state totals, 16 tests, deterministic files, syntax check, oracle centres, and
reopening all reproduce. Reopening retains a history record and names both builds by 12-character hashes.
A second run keeps the claim open. The current driver hash differs from the stored historical hash, but its
results agree, so the unchanged seals continue to name their original build, as documented.
The builder's claims about card text, relationships, filters and state shapes match the HTML checks.
The build report's literal README claim does not survive D1. Its visual claims remain unverified here.
The mutation check was one new mutant, not a reconstruction of the builder's ten historical mutants.

1. Batch records: partial. Direct runs give values and status text; verbose runs give source-line numbers and
   statuses. Kind, automatic dump and failing place are absent. Local value lines do expose their precision.
2. Unified version stamp: unmet through the driver. `version` and `adf_version_check` both give PARSE.
   The slice records its own commit, executable hash, header version and resolved FLINT path.
3. Certificates: unmet through this interface. `recover` gives `22/7`, and polynomial roots give two balls.
   No certificate is printed; `recon_cert_check ...` gives PARSE. This does not claim the C API lacks checkers.
4. Predicates: valid local, partial, real-adele and idele set-predicate probes all give DOMAIN.
5. Dumps: the five unit/local/partial kinds tested return adf1 dumps. The missing feature is result-record
   capture, not the existence of those typed dumps. The driver also constructs the idele of 6/35 directly;
   typing a rational as the real coordinate of an idele literal still gives PARSE.
6. Stable status language: all 11 names appear, and changed output reopens. D3 prevents full classification.
   Compatibility across arbitrary future builds was not tested.

## Checks, commands and results

All check programs ran under timeout. The build used 2 cores; other checks ran serially.
The check list uses `L=lanes/ux-drive-ledger` and `S=ux/ledger` as path abbreviations only.
Each command below had an outer timeout of 175 seconds through run.py, except the separately bounded build.
The exact invocations, dates, stdout and exit codes are in drive.md and the named logs.

- Build. Command: `bash --noprofile --norc -c 'timeout 600 make -j2 && timeout 600 make -C tools/adf'`. Result: exit
  0.
- Copy. Command: `cp -r ux/ledger lanes/ux-drive-ledger/ledger`. Result: exit 0.
- Import. Command: `python3 -B S/import_fixtures.py --out L/ledger/claims`. Result: 86, exit 0.
- Dry-run. Command: `python3 -B S/seal.py --ledger L/ledger --only a1-cos-4-at-2 --dry-run`. Result: exit 0.
- Seal. Command: `python3 -B S/seal.py --ledger L/ledger`. Result: 100; 94/4/1/1/0 states; exit 1.
- Render. Command: `python3 -B S/render.py --ledger L/ledger`. Result: 873531 bytes; exit 0.
- Literal README. Command: command in D1. Result: exit 2.
- Scenario A. Command: `python3 -B L/scenarios.py a`. Result: 5 scripts, 10 lines, 21 assertions.
- Scenario B. Command: `python3 -B L/scenarios.py b`. Result: 8 scripts, 38 lines, 59 assertions.
- A HTML. Command: `node L/dom-check.js S/ledger.html a`. Result: 63 assertions.
- B HTML. Command: `node L/dom-check.js S/ledger.html b`. Result: 149 assertions.
- Extra promises. Command: `python3 -B L/extras.py`. Result: 3 promises tried; 3 unmet.
- Unit tests. Command: `env TMPDIR=L/tmp PYTHONDONTWRITEBYTECODE=1 python3 -B -m unittest discover -s S -v`. Result:
  16 pass.
- Driver gaps. Command: `build/adf -v L/driver-demands-fixed.cmd`. Result: 20 input lines; exit 1, detailed above.
- FLINT build. Command: command in D4. Result: exit 0.
- FLINT equality. Command: `L/flint-equality`. Result: unequal; exit 0.
- Builder checks. Command: `python3 -B L/builder-checks.py`. Result: 29 assertions; exit 0.
- Status checks. Command: `python3 -B L/status-checks.py`. Result: D3; 3 assertions pass; exit 0.
- All records. Command: `python3 -B L/all-records.py`. Result: 99 scripts; 1720 lines; 198 comparisons.

The builder-checks command runs import, seal and render again; compares hashes of 200 files against their
previous bytes and the originals; runs `node --check L/page-script.js`; checks 0 source lines over 116 and
73 README lines; runs the oracle for `(x,p,N) = (3,3,8), (12,3,8), (3,3,2), (12,3,2), (3,3,3), (12,3,3)`;
executes seal/render/seal on a fresh reopening copy; and runs the comparison test against one mutant.
The six oracle outputs are 958, 5125, 4, 4, 13 and 22. All agree with separate driver runs.
Full subcommands are printed in `builder-checks-complete.log`; raw outputs also have `.stdout` files.
The mutation's one failing test is the expected successful detection, not a remaining original test failure.

Firefox command, with a lane-local fresh profile:

```sh
timeout 45 firefox --headless --profile "$(mktemp -d lanes/ux-drive-ledger/firefox.XXXXXX)" \
  --screenshot "$PWD/lanes/ux-drive-ledger/ledger.png" "file://$PWD/ux/ledger/ledger.html"
```

It exits by signal 11. Output: `timeout: the monitored command dumped core`. No screenshot exists.
No browser pixels, desktop layout or phone layout were verified. The Node tests use lightweight element stubs
for the page script and regex extraction for the HTML; they are not browser layout tests.

Failed attempts retained, not counted as slice defects:

- First A HTML probe: my regex unpacked the card incorrectly. Exit 1 at the statement check.
- Next A and first B HTML probes: my header scan mistook STATUS_COUNT for a status. Both exit 1.
- First driver-gap probe: I wrote `mod 9` inside a local-ball literal. Those four PARSE lines were my syntax.
  The corrected `+ O(3^2)` probes distinguish working dumps from refused predicates.
- First builder check: it required 64 hash characters in a page that shows a 12-character build prefix.
- Second builder check: it reused a history directory and found 2 records. The final run uses a fresh directory.
- The first footprint probe checked only individual spans. The follow-up proves 101 is in the filled wide span.

There are 26 named command/check logs, including the build and those retries.
Final successful runs contain 638 passing assertions or unit test cases, counted once per final harness run:
21 + 59 + 63 + 149 + 29 + 198 + 3 + 16. Behavioral probes and literal commands are listed separately.

## Counts and time

6 procedure stages; 10 scenario steps attempted; 3 extra promises tried.
26 logged command/check runs; 638 passing assertions or unit test cases in final successful runs.
4 defects; 6 usability findings; 4 honesty findings. Cross-references identify overlap.
Started 2026-10-16T00:20:45+11:00. Finished 2026-10-16T00:39:09+11:00.
Elapsed 1104 seconds (18.4 minutes), including reporting. Preparatory reading began earlier.

## Files written

All report and probe files are under lanes/ux-drive-ledger/:

- `drive.md`, this `report.md`, `drive.raw.txt`, `files.txt`, and 26 named `.log` files.
- `run.py`, `scenarios.py`, `dom-check.js`, `extras.py`, `builder-checks.py`, `status-checks.py`, `all-records.py`.
- `flint-equality.c` and its executable; `ledger-mutant.py`; `page-script.js`.
- 13 scenario `.cmd` files, 14 extracted `.card.txt` files, and 2 driver-demand `.cmd` files.
- `ledger/`: the original copied slice and regenerated data, still identical for the 200 compared artifacts.
- `extra-ledger/`, `status-ledger/`, `invalid-ledger/`, `void-ratification/`: isolated reproductions.
- `reopened-ledger/` and `reopened-47heyj0r/`: failed-harness and final reopening evidence.
- The builder-check subcommand `.stdout` files, a fresh Firefox profile, and temporary-test directory.

The pre-existing brief.md, lane.log and stdout.log were not edited by me.
The explicitly requested build also generated build/ artifacts. No code fix was applied anywhere.
The file inventory excludes those pre-existing lane files and lists the retained lane artifacts.

## Not done and sources pending

No screenshot or browser layout verification succeeded. The Firefox crash was not diagnosed or worked around.
No network, package installation, full mutation sweep, fuzz run, separate build, or future-version test was done.
No missing product workflow was implemented. All repairs are left to the builder's tier.

The b6 sources were read at refs/src/conrad-hensel/hensel.txt:160-169 and
refs/src/shoup-ntb/ntb-v2.txt:15017,15032-15034. They support its argued tail.
FLINT APIs and equality were read in refs/src/flint-3.0.1/padic.rst; the C probe cites its ranges.
The residue enumeration, binary/ternary digits, exact products and interval checks are written in the scripts
and stepwise in drive.md. They require no external source.
[source pending: the original review probe behind a6's stronger claim that the two exponentials compare equal].
That source is needed to explain the incorrect note, not to establish the counterexample measured here.

## Findings against the specification

0 counterexamples to docs/SPEC.md were found. Its text says equality ignores precision and that the two
exponentials differ at digit 2. Both are consistent with the direct result above.
The stronger assertion in brief 3, scenario A step 4, and the a6 card is contradicted by D4.
The specification was left unchanged.

## VISION

This slice makes the case for reproducible claim files and visible refutations.
It does not yet make the case for a human review queue, because ratification and source gates are absent.
The next slice should bind each state and computed-place mark to the actual per-line evidence.

## FOR THE REPAIR

Smallest changes first; each item has a testable outcome.

1. Comment or separate README explanations. Copy every command literally into a fresh shell and check its exit.
2. Correct a6 and the brief's stronger FLINT assertion. Attach a probe showing 958, 5125 and equality result 0.
3. Recognize all driver error wrappers. The D3 reproduction must be OPEN with a dashed NOT_DETERMINED badge;
   retain correct handling of raw LIMIT, raw PARSE, expected DOMAIN and prefixed budget statuses.
4. Separate named places from computed places. Adding 101 only to `places` must not produce a filled 101 group.
   Include R and operand-named primes when importing evidence, or label them explicitly unclassified.
5. Make branch scope an explicit validated field. The singular seedless root example must be refused or
   proposed, while each seeded branch and an explicit all-branches claim can seal.
6. Refuse RATIFIED for an argued tail without a refs/ file-and-line citation. Display the missing-source reason.
   Keep the current void-ratification behavior for refuted claims.
7. Provide the small human workflow needed to finish A: ratify the two primary sealed claims, record the
   manuscript correction, and export a report that retains scripts, states, seals and argued scope.
8. Rerun Firefox at desktop and phone widths in a working environment; retain screenshots before claiming layout.
