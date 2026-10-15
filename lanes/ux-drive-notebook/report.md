# Notebook test drive report

The supplied 13 scenario records reproduce. The slice has 4 defects: incomplete replay validation,
lossy input transport for computed adeles, an invented DOMAIN explanation, and a missing refinement control.
The numerical examples make the place display useful. The complete agent scenarios cannot yet be performed.

## Scope and execution

Read CLAUDE.md, COMMON.md, UX-COMMON.md, the vision, then only the notebook README before the drive.
Read SPEC 4.1-4.3, 9.2-9.3.3 and 10, and the relevant PLAN principles, milestones and acceptance rules.
The builder report was first opened at step 5, at 2026-10-16T00:55:34+11:00.

No shared file was changed. No git command or bd command was run. No packages were installed.
The README writes ux/notebook/out, outside this lane's ownership. Build/adf was also absent.
I therefore copied unchanged sources into lanes/ux-drive-notebook/work and ran from that copy's root.
This is an isolation deviation from the requested shared-root setup, not a repair to the slice.
The initial discovery and rule reads lacked timeout; every test and later shell command had timeout.
Tests used at most 2 CPUs, with an outer 180-second bound. The longest logged check took 20.055 seconds.

Firefox failed with signal 11 on both pages and on about:blank. No screenshot was produced.
DOM inspection used Node, HTML text and a small mock DOM; it does not establish visual layout.

## DEFECTS

### D1. Replay calls inconsistent and unchecked records the same

Commands: `timeout 120 python3 ../replay_probe.py`, then
`timeout 30 sh ux/notebook/replay.sh ../a_flow.records.jsonl`, from the isolated root.
Exact variants and full output are in replay_probe.txt and replay_skipped.txt.

Starting with B cell 2, change each field separately while retaining the other fields:

- status: OK to NO_SOLUTION;
- dump: adf1 Q rat 16 7 to adf1 Q rat 17 7, which denotes 23/7;
- the base-2 digits: (011)1010 to (0)0;
- input: recover(c; num = 30, den = 15) to recover(c; num = 0, den = 1).

All 4 variants exit 0 and end with `replay: every record is the same`.
The unchanged control exits 0. Changing value to 23/7 exits 1 and reports driver value 22/7.
The 4-cell a_flow notebook has 2 Julia errors without driver lines. Replay skips both and still exits 0,
ending with the same success sentence. Only 2 of its 4 records were checked.

Expected: report these fields as different or explicitly unchecked. The brief says records are compared
byte for byte. NO_SOLUTION contradicts 22/7; the altered dump and digits denote different numbers.
The changed bounds allow only n=0,d=1, which fails 0=146 mod 1000. This is a different question.
The implementation documents value-text comparison internally, but the user-facing success claim is broader.

### D2. A computed adele is widened when sent back to the driver

Reproducer: builder_followups.jl cells 7-10; run it with the README's run_file command below, then:

```sh
timeout 30 sh ux/notebook/replay.sh ../builder_followups.records.jsonl
```

The inputs are:

```julia
x = adf"(0.99 +/- 0.0099 ; 0 mod 1)"
y = x + Rat(0)
project(y, :real)
reconstruct(y)
```

x and y have identical dumps. The live reconstruct(y) returns `error: NO_SOLUTION`.
Its saved driver command is `reconstruct (0.99 +/- 0.01 ; 0 mod 1)`, which returns `1`.
Replay exits 1: `cell 10: differs: record "error: NO_SOLUTION", driver "1"`.
No record was edited. The builder disclosed the transport limitation, but did not test this consequence.

Expected: a fresh record replays the same question and status. The original stated real interval is
[0.9801,0.9999], which contains no integer; the finite progression is Z.
Direct driver reconstruction of that original literal also gives NO_SOLUTION.
The outward printed interval [0.98,1] admits 1. Reading the display as input changes the question.

Projection is affected too. Direct projection of the original prints `real: 0.99 +/- 0.01`.
project(y,:real) prints `real: 0.99 +/- 0.011`, after parsing the already widened text.
The latter is an enclosure but is not the requested stored component. No warning appears on the page.
Evidence: builder_followups.records.jsonl, replay_computed.txt, followups_values.txt.

### D3. The DOMAIN card invents the cause for an invalid place

Reproducer: extras.jl cell 8, `root(9, 2; at = 4, seed = 3)`.
Driver command: `root_at 9 with 4 with 2 with 3`. Output: `error: DOMAIN`, exit 1.
The page says the existence criterion fails or the seed/degree is invalid, and suggests another seed.

Expected: identify an invalid place, or say the reason was not given. 4=2*2 is not prime.
The degree 2 and seed 3 are valid in the example at 5, where 3^2=9.
The driver supplied no reason sub-code. Trying another seed cannot make 4 a prime.
The status itself is correct; the explanation and action are misleading.
Evidence: extras_dom.txt, extras.records.jsonl and builder_demands.txt.

### D4. Above p=97, the page asks for a click on a control it did not render

Reproducer: builder_followups.jl cell 4:
`root(-1, 2; at = 101, seed = 10, prec = 8)`.
The page correctly gives `3156215819652023 + O(101^8)` without digits.
It also says `click the fog for 12 more digits (prec = 20)`.
That cell has 0 data-rewrite controls and no clickable fog. The instruction cannot be followed.

Expected: retain a separate refinement control with the precision in its suggestion, or remove the instruction.
The displayed root is correct: its square plus 1 is divisible by 101^8, and its residue is 10 mod 101.
Evidence: followups_dom.txt and followups_values.txt.

## USABILITY

These 6 findings are ordered by cost to the brief's user. Static operation is allowed by D-UX1;
the missing interactions below are gaps against the vision, not demands to add Pluto or a server.

1. A5 cannot be completed. The brief promises equal(teichmuller(...),root(...)) and compare(...).
   The supplied fifth cell instead tests roots(2). Trying the promised expressions gives UndefVarError
   for equal and compare; teichmuller alone is also undefined. No argument note or acceptance state exists.
2. B3-B4 cannot be completed as a certificate workflow. The brief says the notebook keeps two certificates
   and shows what changed when bounds change. All 8 B records lack certificate fields and acceptance state.
   The 22/7 answers are correct, but no certificate or Euclidean-row comparison is available.
3. Picking and refining require several manual edits. The brief promises a click that pins a branch and a
   moving fog edge. The click changes display text only; the fan remains until the suggestion is copied into
   the file and rerun. Fan fog is not a control, so an extra run is needed before refining the chosen root.
   Alt-Up does nothing. The README and stale marker explain this honestly; no hidden computation is claimed.
4. The agent window cannot yet support the promised provenance or complete record comparison.
   The brief requires kind, value, dump, status, failing place, precision per place and library version.
   All 13 scenario records lack a version stamp. Four successful root records and one projection have null
   dumps, although local and partial dumps are supported by the driver. D1-D2 show the replay consequences.
5. The README's FBall(c,N) leaves argument types unspecified. FBall(0,25) gives a MethodError.
   The documented two-argument package constructor needs Rat arguments; only the builder's later example
   supplies that missing detail. That failing constructor path was stopped without changing the slice.
6. The brief promises that hovering any output shows its set, with a dump one click away.
   B has 5 strip tooltips and 5 dump controls. A has 0 of either, including all 4 successful local outputs.
   The local values can be checked from their text and sidecar, but the promised inspection path is absent.

## HONESTY

There are 4 negative findings, overlapping the 4 defects rather than adding 4 more defects:
D1 overstates verification; D2 silently changes the input enclosure; D3 states a cause it did not compute;
D4 advertises an action that has no control.

The 3 supplied failure cards correctly say `place: not given`. Knowing the requested place does not supply
an independently reported failing place. The 5 successful local/projection null dumps are omissions, not
fabricated data; they should be identified as unavailable to the user. Failure outputs appropriately lack dumps.

The finite strips say Z_p for the other primes; exact zero correctly says 0 at every prime.
The cofactor probes show 1009 and 1022117 as unfactored, without claiming their primes were discovered.
The digits are finite computations from value text, as permitted by the amended D-UX5.
The NOT_DETERMINED card states that the driver's search limit 1000 cannot be raised from this notebook.
The stale marker clearly says the page cannot compute. No dump is presented as a reconstruction certificate.

## Independent numerical checks

All 5 A and all 8 B records match direct driver output, including the 3 status outputs.
The roots of -1 at 5 have centres 280182 and 110443 modulo 390625, and
79753541295807 modulo 95367431640625 at precision 20. Each squares to -1 at the stated modulus.
Their digits are 32431212, 12013233 and 40423140223032431212, respectively.
For -3, 2+sum(4*5^i,i=1..k-1)=5^k-3. For 3, every higher digit is zero.
The square residues modulo 5 are 0,1,4; this independently proves the added roots(2) DOMAIN result.

For rational periodic digits, multiply the finite repeated-block sum by 1-p^t.
All inner terms cancel. Modulo any fixed p^k, the final shifted block becomes zero after enough repeats.
Therefore a block B above r low digits contributes B*p^r/(1-p^t).
The exact fraction checks recover 22/7 at 2,7,11 and 6/35 at 2,3,5,7.
Valuations and absolute values agree with division of the numerators and denominators.
The 6/35 absolute-value product is (6/35)*(1/2)*(1/3)*5*7=1.

Exhaustive reduced-fraction enumeration gives 1 candidate for bounds (30,15), 1 for (40,30),
and 82 for (400,300), all with residue 146 modulo 1000. The first two candidates are 22/7.
7*146=1022=22 mod 1000; 2*30*15=900 and 2*40*30=2400.
The progression 1/7+k meets [3.13,3.15] only at k=3; its two neighbors lie outside.
For residue 1 modulo 2003 with bounds (3,1001), exhaustive enumeration gives only 1.
The driver returns NOT_DETERMINED because its bounded procedure did not finish; the page does not claim otherwise.

## Comparison with the builder report

- The 129 Julia tests reproduce, with testset counts 45,11,6,20,13,12,22.
- The README Node command exits 0 but reports 1 file-level test here, including under env -i.
  Direct execution reports all 3 named tests, 3 passed. The drive also independently checks the inline handlers.
- Regeneration under env -i HOME PATH reproduces all 4 output files byte for byte.
  All 24 slice files match the isolated copy after the mutation is restored.
- Replays of A and B give 5/5 and 8/8 same. The builder's changed-value test also detects the change.
  It does not cover the 4 unchecked fields or skipped records of D1.
- One claimed mutation kind was rerun: digit-recurrence mutant 1 was killed, 1 of 1 attempted.
  The mutation script prints a fixed denominator of 10; its output is not evidence that 10 were rerun.
- A clean run took 7.208 seconds, with page times 2.3 and 2.5 seconds, within the reported ranges.
- README length is 66 lines. Source line widths meet 116, but 57 lines in the 4 generated files exceed it.
  The longest is 1429 characters. The builder's all-files line-width claim is too broad.
- The builder's screenshot result could not be reproduced. The blank-page control also crashes.
- The displayed page values, seed preservation, explicit precision, named assignments, status classes,
  zero handling, cofactor handling and dumps agree with the inspected HTML and numerical probes.
  The computed-adele limitation and prime-above-97 instruction have the defects described above.

## Library demands checked against the brief's 8 items

1. Structured result: unmet at the driver surface. The calls print values or bare errors, with no failing-place
   field, reason code or certificate. Successful metadata is assembled by the slice.
2. Local/partial value forms: show accepts both forms now. project still prints the legacy labelled text.
   The builder's narrower statement about commands at places is correct.
3. Unit, idele and class dumps: all 3 succeed. Local and partial dumps also succeed, 5 kinds tested in total.
   LBall and Idele remain undefined in the notebook environment.
4. Digits: no documented driver accessor. The slice derives them in Julia. An exploratory padic_digits
   command is PARSE; this is evidence about driver exposure, not a proof about every C API.
5. Interesting places: no documented driver query. An exploratory places command is PARSE.
   The slice's bounded trial division and explicit remaining cofactors behave as advertised.
6. Reason codes: the same bare DOMAIN appears for nonsquareness and a composite place. The distinction is lost.
7. Per-place result/status map: exp_at with real and 5 is PARSE. The one-prime call gives DOMAIN with no map.
8. Determinism: observed for the 4 regenerated fixtures and 13 replays. A provenance stamp is still absent.
   An exploratory version command is PARSE; the C version functions do exist, so this is not a C absence claim.

The driver gives true for equal on the same finite ball and undecided for compare on it.
The notebook's missing Teichmueller/equality workflow is therefore distinct from the mathematical distinction.
The local Teichmueller C entry point is declared in include/adelefeld/lball.h; no driver command is documented.

## VISION

The slice makes the case for place strips through correct digits, explicit branches and readable status data.
It does not yet make the case for an agent notebook with inspectable certificates and trustworthy acceptance.
The next slice should preserve exact inputs through evaluation and compare complete output records on replay.

## FOR THE REPAIR

Smallest changes first; each item has a testable outcome.

1. Document FBall(Rat(c),Rat(N)) with a runnable example and name the missing scenario features in the README.
   Keep the README below 80 lines; a user following its constructor example must get a strip.
2. Qualify the DOMAIN reason when no sub-code exists, including invalid places among possible causes.
   The composite-place probe must not recommend fixing only the seed or claim a failed root criterion.
3. Render a refinement action for p>97, or remove the unreachable click instruction.
   The 101-adic example must have a working explicit prec=20 suggestion if refinement is advertised.
4. Make replay distinguish verified, different and unchecked records and check the claimed result fields.
   All 4 metadata variants must be rejected or marked unchecked; missing driver lines must not imply equality.
5. Preserve computed adele inputs losslessly across driver calls. A fresh builder_followups notebook must
   retain the stored real component and replay reconstruct(y) as NO_SOLUTION, with 0 differences.
6. Add visible provenance and local-value inspection: dump or explicit unavailability, versions, and set text.
   Every successful A output and B projection must have a truthful inspection route.
7. In a later slice, implement or explicitly refuse the A5 comparison and B certificate/acceptance workflow.
   The two promised scenarios must not silently substitute other tasks for these steps.

## What is not done

No slice code was repaired. No actual rendered appearance, pointer hit area, responsive layout or browser
keyboard behavior was verified. The pure handlers were tested through Node only.
No live agent, Tab acceptance, certificate retention, or full-record recomputation exists in this slice.
Only 1 mutation was rerun; no fuzzing or full library test sweep was performed.
Driver probes do not establish absence of every possible library function.
No full external-source audit was needed for the finite arithmetic checks.

## Sources pending

None for the claims made here. Arithmetic checks are written out above and in the scripts.
The brief, specification, headers and driver README are cited as project contracts, not as external authorities.
No outside mathematical formula was attributed from memory.

## Findings against the specification

None. The failures concern the notebook's implementation and its reading of the brief.
The computed-adele example illustrates why SPEC 10 distinguishes a printed enclosure from a lossless dump;
it is not a counterexample to the specification.

## Counts and time

Start: 2026-10-16T00:47:10+11:00. End of work: 2026-10-16T01:09:17+11:00. Elapsed: 1327 seconds.

6 prescribed drive stages completed. All 5 A steps, all 4 B steps and 3 extra promises were attempted.
36 notebook cells were evaluated: 5 A, 8 B, 4 A-flow, 9 extra and 10 builder follow-up cells.
29 logged check invocations; 317 counted numerical/DOM assertions and builder test cases.
The 317 include assertions that reproduce defects, not 317 claims that the slice is correct.
There were also 6 replay variants, 20 demand-probe driver commands and 1 mutation attempt.
4 defects, 6 usability findings, 4 overlapping negative honesty findings, 0 findings against SPEC.
3 Firefox attempts failed by signal 11; 0 screenshots. All supplied 13 record values/statuses reproduced.

## Files written and retained

All files are under lanes/ux-drive-notebook. Existing brief.md and harness logs were not changed by this work.

- report.md and drive.md: final findings and incremental timestamped log.
- checks.json: exact commands, working directory, timestamps, exit codes and elapsed time for every logged check.
- 29 named .txt outputs, one per check in the list below.
- run_check.py and prepare.py: bounded execution logger and isolated-copy preparation helper.
- a_check.py, b_check.py, extras_check.py, followups_check.py, dom_check.js, replay_probe.py and audit.py.
- a_flow.jl, extras.jl, builder_followups.jl, plus their 3 HTML pages and 3 records files.
- demands.cmd and 6 replay_*.jsonl variants.
- observed/: the 4 generated scenario HTML/record files, retained byte for byte.
- slice-sha256.json: hashes of the 24 tested slice files.

The temporary work/ tree held 2517 source, build, test-output and cache files and was removed after the checks.
All 3 temporary Firefox profiles were removed. No generated build or copied source tree needs to be committed.
prepare.py captures the initial copy operation; run it, then the setup commands below, to recreate work/.

## Commands and results

Commands below ran from lanes/ux-drive-notebook/work in a fresh non-login shell, through run_check.py.
The logger imposed timeout 180 and affinity to at most 2 CPUs; the caller imposed timeout 185.
The two make commands retain the prescribed inner timeout 600, bounded by the outer 180.
Exact command strings and output filenames are in checks.json. Long commands below are only reflowed.
Each .txt file preserves the full output, including compiler warnings and expected failures.

1. setup_make: exit 0; 11.144 s; static library built.

```sh
timeout 600 make -j2
```

2. setup_driver: exit 0; 11.656 s; driver built.

```sh
timeout 600 make -C tools/adf
```

3. setup_readme: exit 0; 15.291 s; 625/625 exports; 5 A cells and 8 B cells generated.

```sh
timeout 180 sh ux/notebook/run.sh
```

4. a_firefox: exit -11; 0.199 s; signal 11; 0 screenshots.

```sh
timeout 30 firefox --headless \
    --profile "$(timeout 5 mktemp -d ../firefox-a.XXXXXX)" \
    --window-size 1400,2400 \
    --screenshot "$PWD/../a_roots.png" \
    "file://$PWD/ux/notebook/out/a_roots.html"
```

5. a_values: exit 0; 0.038 s; 26 checks.

```sh
timeout 20 python3 ../a_check.py
```

6. a_dom: exit 0; 0.024 s; 20 checks.

```sh
timeout 20 node ../dom_check.js
```

7. firefox_control: exit -11; 0.204 s; signal 11 on about:blank; 0 screenshots.

```sh
timeout 15 firefox --headless \
    --profile "$(timeout 5 mktemp -d ../firefox-control.XXXXXX)" \
    --screenshot "$PWD/../blank.png" \
    about:blank
```

8. a_flow: exit 0; 3.041 s; 4 cells; 2 roots and 2 Julia errors.

```sh
timeout 60 sh ux/notebook/julia.sh \
    -e 'push!(LOAD_PATH, "ux/notebook/src");
    using Adelica;
    Adelica.run_file("../a_flow.jl", "../a_flow")'
```

9. b_dom: exit 0; 0.025 s; 13 checks.

```sh
timeout 20 node ../dom_check.js work/ux/notebook/out/b_which_rational.html
```

10. b_firefox: exit -11; 0.182 s; signal 11; 0 screenshots.

```sh
timeout 15 firefox --headless \
    --profile "$(timeout 5 mktemp -d ../firefox-b.XXXXXX)" \
    --window-size 1400,3000 \
    --screenshot "$PWD/../b_which_rational.png" \
    "file://$PWD/ux/notebook/out/b_which_rational.html"
```

11. b_values: exit 0; 0.077 s; 58 checks; counts 1,1,82,1.

```sh
timeout 30 python3 ../b_check.py
```

12. extras_run: exit 0; 4.319 s; 9 cells; 7 driver outputs and 2 Julia errors.

```sh
timeout 60 sh ux/notebook/julia.sh \
    -e 'push!(LOAD_PATH, "ux/notebook/src");
    using Adelica;
    Adelica.run_file("../extras.jl", "../extras")'
```

13. extras_dom: exit 0; 0.026 s; 13 checks.

```sh
timeout 20 node ../dom_check.js extras.html
```

14. replay_probe: exit 0; 6.026 s; 6 variants; 1 value mismatch caught; 4 metadata mismatches accepted.

```sh
timeout 120 python3 ../replay_probe.py
```

15. extras_values: exit 0; 0.042 s; 28 checks.

```sh
timeout 20 python3 ../extras_check.py
```

16. builder_node: exit 0; 0.154 s; 1 file-level test passed.

```sh
timeout 30 node --test ux/notebook/test/page.test.js
```

17. builder_julia: exit 0; 10.961 s; 129/129 checks passed.

```sh
timeout 120 sh ux/notebook/test.sh
```

18. builder_node_clean: exit 0; 0.154 s; 1 file-level test passed.

```sh
timeout 30 env -i HOME="$HOME" PATH="$PATH" node --test ux/notebook/test/page.test.js
```

19. builder_regenerate: exit 0; 7.208 s; 4 files identical; page times 2.3 and 2.5 seconds.

```sh
timeout 60 env -i HOME="$HOME" PATH="$PATH" sh ux/notebook/run.sh
 timeout 10 diff -r ../../../ux/notebook/out ux/notebook/out
```

20. builder_demands: exit 1; 0.009 s; 20 commands: 12 successes, 3 DOMAIN, 5 PARSE.

```sh
timeout 20 build/adf -v ../demands.cmd
```

21. builder_mutation: exit 0; 20.055 s; 1 mutant attempted, 1 killed; fixed printed denominator 10.

```sh
timeout 120 python3 ../../../lanes/ux-notebook/mutate.py 1
```

22. builder_node_direct: exit 0; 0.058 s; 3 named tests passed.

```sh
timeout 30 node ux/notebook/test/page.test.js
```

23. builder_replay_a: exit 0; 3.567 s; 5/5 same.

```sh
timeout 30 sh ux/notebook/replay.sh ux/notebook/out/a_roots.records.jsonl
```

24. builder_followups: exit 0; 5.608 s; 10 cells, including 2 undefined wrappers.

```sh
timeout 60 sh ux/notebook/julia.sh \
    -e 'push!(LOAD_PATH, "ux/notebook/src");
    using Adelica;
    Adelica.run_file("../builder_followups.jl", "../builder_followups")'
```

25. replay_skipped: exit 0; 1.214 s; 2 same, 2 unchecked; success exit despite unchecked records.

```sh
timeout 30 sh ux/notebook/replay.sh ../a_flow.records.jsonl
```

26. replay_computed: exit 1; 0.947 s; 7 same, 2 unchecked, 1 status difference.

```sh
timeout 30 sh ux/notebook/replay.sh ../builder_followups.records.jsonl
```

27. followups_dom: exit 0; 0.030 s; 13 checks.

```sh
timeout 20 node ../dom_check.js builder_followups.html
```

28. builder_audit: exit 0; 0.038 s; 24 files identical; 57 long generated lines; 0 version fields.

```sh
timeout 20 python3 ../audit.py
```

29. followups_values: exit 0; 0.106 s; 14 checks, including 2 reproduced defects.

```sh
timeout 20 python3 ../followups_check.py
```

Final artifact QA, from the shared repository root:

```sh
timeout 10 python3 lanes/ux-drive-notebook/validate_outputs.py
```

Exit 0. Both Markdown files have 0 lines above 116 characters; 4 retained outputs match their hashes.
All 29 check outputs exist; 0 temporary work trees and 0 Firefox profiles remain.
The first formatting inspection found 6 long command lines in drive.md and 0 in report.md.
Those log commands were reflowed without changing their meaning; checks.json retains the exact strings.
The final helper validate_outputs.py and its output artifact_qa.txt are also retained.
This formatting/inventory check is separate from the 29 task-check invocations counted above.
