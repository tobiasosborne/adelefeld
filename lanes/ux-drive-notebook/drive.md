# Notebook test drive

Start: 2026-10-16T00:47:10+11:00 (first recorded `date -Iseconds`).

Read CLAUDE.md, COMMON.md, UX-COMMON.md, UX-DRIVE.md, the notebook brief, then its README.
No builder report has been opened. Read SPEC 4.1-4.3, 9.2-9.3.3 and 10; PLAN principles, 1, 1F, S, acceptance.
Initial file discovery, CLAUDE and UX-DRIVE reads lacked timeout before that instruction was read.
All later shell commands use timeout. No test ran without timeout.

## 1. Setup

2026-10-16T00:47:35+11:00: build/adf and build/libadelefeld.so are missing.
README run.sh writes ux/notebook/out, which this lane may not change.
Use a byte-for-byte source copy under this lane, without git, refs or other lanes.
Run the README commands unchanged from that copy's root in a fresh non-login shell.
This is a drive isolation choice, not a repair to the slice. It differs from running in the shared root.
Generated builds, Julia depot and pages stay in the lane. Limit each process to 180 seconds and 2 CPU cores.

### setup_make: 2026-10-16T00:48:44+11:00

Expected: Static library builds; exit 0.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 600 make -j2
```

Result: exit 0; 11.144 seconds; output `setup_make.txt`.

### setup_driver: 2026-10-16T00:49:00+11:00

Expected: Driver builds; exit 0.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 600 make -C tools/adf
```

Result: exit 0; 11.656 seconds; output `setup_driver.txt`.

### setup_readme: 2026-10-16T00:49:12+11:00

Expected: Both scenario pages and records generated; exit 0.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 180 sh ux/notebook/run.sh
```

Result: exit 0; 15.291 seconds; output `setup_readme.txt`.

### a_firefox: 2026-10-16T00:49:44+11:00

Expected: Firefox creates a screenshot of scenario A.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 30 firefox --headless \
    --profile "$(timeout 5 mktemp -d ../firefox-a.XXXXXX)" \
    --window-size 1400,2400 \
    --screenshot "$PWD/../a_roots.png" \
    "file://$PWD/ux/notebook/out/a_roots.html"
```

Result: exit -11; 0.199 seconds; output `a_firefox.txt`.

### a_values: 2026-10-16T00:51:03+11:00

Expected: All 5 driver results and exact root digits agree.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 20 python3 ../a_check.py
```

Result: exit 0; 0.038 seconds; output `a_values.txt`.

### a_dom: 2026-10-16T00:51:03+11:00

Expected: Scenario A DOM, seed rewrite and fog suggestion work.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 20 node ../dom_check.js
```

Result: exit 0; 0.024 seconds; output `a_dom.txt`.

### firefox_control: 2026-10-16T00:51:14+11:00

Expected: Blank-page control distinguishes browser failure from page failure.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 15 firefox --headless \
    --profile "$(timeout 5 mktemp -d ../firefox-control.XXXXXX)" \
    --screenshot "$PWD/../blank.png" \
    about:blank
```

Result: exit -11; 0.204 seconds; output `firefox_control.txt`.

### a_flow: 2026-10-16T00:51:13+11:00

Expected: Copied seed and precision calls evaluate; comparison capability is exposed or refuses honestly.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 60 sh ux/notebook/julia.sh \
    -e 'push!(LOAD_PATH, "ux/notebook/src");
    using Adelica;
    Adelica.run_file("../a_flow.jl", "../a_flow")'
```

Result: exit 0; 3.041 seconds; output `a_flow.txt`.

## 2. Scenario A observations

All five stored outputs match a fresh driver. The three inexact roots pass exact modular checks.
The digits use sum(d_i * 5^i); 280182 has digits 32431212, 110443 has digits 12013233.
79753541295807 has digits 40423140223032431212 and reduces to 280182 modulo 390625.
For -3, 2 + sum(4 * 5^i, i=1..k-1) = 5^k - 3. For 3, higher digits are zero.
The square residues mod 5 are 0, 1 and 4, so the added roots(2) cell correctly gives DOMAIN.

The branch click edits text and marks it stale; restoring works in a mock DOM.
The old fan remains displayed. Copying the suggestion to a notebook and rerunning is required.
The fan fog is not a control: selection must be rerun before an 8-to-20 fog suggestion exists.
Alt-Up does nothing. This is consistent with the README's static page, but costs extra file edits.
The scenario replaces the brief's fifth step (Teichmueller equality and point comparison) with roots(2).
No ghost, acceptance action, or argument note is supplied.

Headless Firefox exits by signal 11 both on A and on about:blank; no screenshot exists.
This is an environment limitation, not evidence that the notebook page itself fails to render.
HTML and inline handlers were inspected with Node; no rendered appearance is claimed.

## 3. Scenario B

### b_dom: 2026-10-16T00:51:48+11:00

Expected: All eight scenario B cells and their controls can be inspected.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 20 node ../dom_check.js work/ux/notebook/out/b_which_rational.html
```

Result: exit 0; 0.025 seconds; output `b_dom.txt`.

### b_firefox: 2026-10-16T00:51:48+11:00

Expected: Attempt the required scenario B screenshot.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 15 firefox --headless \
    --profile "$(timeout 5 mktemp -d ../firefox-b.XXXXXX)" \
    --window-size 1400,3000 \
    --screenshot "$PWD/../b_which_rational.png" \
    "file://$PWD/ux/notebook/out/b_which_rational.html"
```

Result: exit -11; 0.182 seconds; output `b_firefox.txt`.

### b_values: 2026-10-16T00:52:48+11:00

Expected: All B outputs, valuations, digits and reconstruction candidates agree independently.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 30 python3 ../b_check.py
```

Result: exit 0; 0.077 seconds; output `b_values.txt`.

## Scenario B observations

All 8 driver results match. Five dumps load to the same printed values.
At 2, 146 = 2 mod 8, digits 010. At 5, 146 = 21 mod 125, digits 041.
For a periodic block B of length t above r low digits, its value is B*p^r/(1-p^t).
Thus the binary expansion (011)1010 is 10 + 3*16/(1-8) = 22/7.
The same exact fraction check passed the base-7 and base-11 expansions.
The 11-adic unit residue is 5 after removing 11; all three printed valuations match direct division.
The printed real intervals contain 22/7. 7*146=1022=22 mod 1000.
Enumeration of reduced n/d gives counts 1, 1 and 82 for boxes (30,15), (40,30), (400,300).
For c=1,m=2003,A=3,B=1001, exhaustive enumeration gives only 1.
The driver still returns NOT_DETERMINED because its search limit is 1000; the page says this honestly.
The progression 1/7+k has only k=3 in [3.13,3.15]; its neighbors are outside.
The displayed outward-rounded radius 0.011 also admits just that point.
No certificate, author or acceptance state exists in the 8 records.
The page labels a dump as a dump, not as a certificate. The certificate comparison of brief B3-B4 is absent.

## 4. Three promises beyond the scenarios

1. The surface, place strip: exact rationals, uncertain zero, and an unfactored cofactor.
2. The surface, branches: clicking a seed keeps explicit precision and a named input.
3. Interaction model, agent window and replay: a saved result can be checked after its fields change.

### extras_run: 2026-10-16T00:53:42+11:00

Expected: Three promised interactions remain mathematically honest on additional inputs.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 60 sh ux/notebook/julia.sh \
    -e 'push!(LOAD_PATH, "ux/notebook/src");
    using Adelica;
    Adelica.run_file("../extras.jl", "../extras")'
```

Result: exit 0; 4.319 seconds; output `extras_run.txt`.

### extras_dom: 2026-10-16T00:54:00+11:00

Expected: Inspect additional strip, cofactor, branch and status HTML.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 20 node ../dom_check.js extras.html
```

Result: exit 0; 0.026 seconds; output `extras_dom.txt`.

### replay_probe: 2026-10-16T00:54:18+11:00

Expected: Unchanged records pass; changed result data is reported, never silently called the same.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 120 python3 ../replay_probe.py
```

Result: exit 0; 6.026 seconds; output `replay_probe.txt`.

### extras_values: 2026-10-16T00:55:34+11:00

Expected: Additional numbers, digits, cofactors and branch suggestions are reproducible.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 20 python3 ../extras_check.py
```

Result: exit 0; 0.042 seconds; output `extras_values.txt`.

## Extra-promise observations

6/35 has valuations 1,1,-1,-1 at 2,3,5,7; its displayed periods give exactly 6/35.
The displayed absolute values multiply with 6/35 to 1. No product-formula output is supplied.
Zero correctly says 0 at every prime. The two unfactored cofactors are 1009 and 1022117=1009*1013.
Neither has a divisor from 2 through 999. The page does not pretend their primes are known.
FBall(0,25) gives MethodError. The README names FBall(c,N) without giving required argument types.
That constructor path was stopped without a fix.
The 20-digit fan keeps prec=20 in both suggestions; the named fan keeps r= in both suggestions.

For root(9,2;at=4,seed=3), DOMAIN is correct: 4 is not prime.
The status card says the existence criterion fails or the seed/degree is invalid, and suggests another seed.
The seed 3 and degree 2 are valid; the invalid place is the actual cause. No reason was supplied by the driver.
This is a misleading reason/action in the presentation, not a wrong library status.

Replay rejects a changed value (23/7) with exit 1, but accepts four inconsistent records with exit 0:
status NO_SOLUTION, dump of 23/7, digits (0)0, and input with bounds num=0,den=1.
Each says `replay: every record is the same`. The value alone was checked; the record was not.
This is a defect against the brief's comparison of records and a misleading success claim.

## 5. Builder comparison

2026-10-16T00:55:35+11:00: opening the builder report for the first time, after steps 1-4.

### builder_node: 2026-10-16T00:55:56+11:00

Expected: Builder claims 3 passing click-script tests.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 30 node --test ux/notebook/test/page.test.js
```

Result: exit 0; 0.154 seconds; output `builder_node.txt`.

### builder_julia: 2026-10-16T00:55:56+11:00

Expected: Builder claims 129 passing Julia checks.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 120 sh ux/notebook/test.sh
```

Result: exit 0; 10.961 seconds; output `builder_julia.txt`.

### builder_node_clean: 2026-10-16T00:56:47+11:00

Expected: Clean environment reports the three named Node tests.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 30 env -i HOME="$HOME" PATH="$PATH" node --test ux/notebook/test/page.test.js
```

Result: exit 0; 0.154 seconds; output `builder_node_clean.txt`.

### builder_regenerate: 2026-10-16T00:56:47+11:00

Expected: Clean-shell generation reproduces all four committed output files byte for byte.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 60 env -i HOME="$HOME" PATH="$PATH" sh ux/notebook/run.sh
 timeout 10 diff -r ../../../ux/notebook/out ux/notebook/out
```

Result: exit 0; 7.208 seconds; output `builder_regenerate.txt`.

### builder_demands: 2026-10-16T00:57:13+11:00

Expected: Probe all eight library-demand claims through available driver commands.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 20 build/adf -v ../demands.cmd
```

Result: exit 1; 0.009 seconds; output `builder_demands.txt`.

### builder_mutation: 2026-10-16T00:57:13+11:00

Expected: Rerun one of the claimed killed mutants in the isolated copy.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 120 python3 ../../../lanes/ux-notebook/mutate.py 1
```

Result: exit 0; 20.055 seconds; output `builder_mutation.txt`.

### builder_node_direct: 2026-10-16T00:57:34+11:00

Expected: Direct execution resolves whether the three named tests run in this Node environment.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 30 node ux/notebook/test/page.test.js
```

Result: exit 0; 0.058 seconds; output `builder_node_direct.txt`.

### builder_replay_a: 2026-10-16T00:57:35+11:00

Expected: Builder claims all five A records replay unchanged.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 30 sh ux/notebook/replay.sh ux/notebook/out/a_roots.records.jsonl
```

Result: exit 0; 3.567 seconds; output `builder_replay_a.txt`.

### builder_followups: 2026-10-16T00:58:34+11:00

Expected: Check builder suggestions, missing wrappers, and its disclosed computed-adele widening.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 60 sh ux/notebook/julia.sh \
    -e 'push!(LOAD_PATH, "ux/notebook/src");
    using Adelica;
    Adelica.run_file("../builder_followups.jl", "../builder_followups")'
```

Result: exit 0; 5.608 seconds; output `builder_followups.txt`.

### replay_skipped: 2026-10-16T00:59:10+11:00

Expected: Replay must distinguish unchecked Julia errors from matching records.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 30 sh ux/notebook/replay.sh ../a_flow.records.jsonl
```

Result: exit 0; 1.214 seconds; output `replay_skipped.txt`.

### replay_computed: 2026-10-16T00:59:44+11:00

Expected: A fresh computed-adele notebook should replay without differences.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 30 sh ux/notebook/replay.sh ../builder_followups.records.jsonl
```

Result: exit 1; 0.947 seconds; output `replay_computed.txt`.

### followups_dom: 2026-10-16T00:59:45+11:00

Expected: Inspect builder-suggested pages and the computed projection.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 20 node ../dom_check.js builder_followups.html
```

Result: exit 0; 0.030 seconds; output `followups_dom.txt`.

### builder_audit: 2026-10-16T00:59:45+11:00

Expected: Verify regeneration, restored source, line lengths, tooltips, dump controls and missing fields.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 20 python3 ../audit.py
```

Result: exit 0; 0.038 seconds; output `builder_audit.txt`.

### followups_values: 2026-10-16T01:01:05+11:00

Expected: Verify all new numbers and reproduce the projection/replay and absent-fog defects.

Cwd: `work`.

Command (outer timeout 180; affinity 2 cores):

```sh
timeout 20 python3 ../followups_check.py
```

Result: exit 0; 0.106 seconds; output `followups_values.txt`.

## Builder comparison results

The Julia test counts reproduce exactly: 129/129, with testset counts 45,11,6,20,13,12,22.
Node --test reports one file-level test in this environment, both normally and under env -i.
Direct node execution reports all 3 named tests, 3 pass, 0 fail. No conclusion about Node internals is needed.
Regeneration under env -i HOME PATH gives 0 differences against all 4 original output files.
The 24 slice files also match the isolated copy after restoring mutant 1.
Mutant 1 (digit recurrence sign) was killed. Only this 1 mutant was rerun; the script's denominator 10 is fixed.
The full reported 9/10 result was not rerun.
Source and README line limits hold, but 57 lines in the 4 generated output files exceed 116 characters.
The builder's all-files line claim is too broad. The longest generated line has 1429 characters.
The README has 66 lines, below the D-UX6 bound of 80.

The builder's disclosed decimal-widening limitation has a concrete consequence on a fresh notebook.
x=(0.99 +/- 0.0099 ; 0 mod 1), y=x+Rat(0) have identical dumps.
reconstruct(y) gives NO_SOLUTION, but its saved driver line uses 0.99 +/- 0.01 and returns 1.
The original stated interval is [0.9801,0.9999]; it contains no integer.
The direct driver on the original literal also gives NO_SOLUTION.
The printed interval includes 1, so reading it as input asks a different mathematical question.
project(y,:real) likewise prints 0.99 +/- 0.011; direct projection of the original gives 0.99 +/- 0.01.
The page does not disclose that it projected a widened value. This is not a library or specification defect.

At p=101 the root centre 3156215819652023 squares to -1 modulo 101^8, with seed 10.
The page correctly suppresses digits, but says click the fog for 12 more digits with no data-rewrite control.
The builder's suggested constructors and statuses otherwise match the driver.
LBall and Idele are undefined, as the report says. The integer FBall constructor needs Rat arguments,
as the builder's example reveals only after step 5.

The driver supplies no structured result, failing place, per-call reason, or certificate in these outputs.
show and dump support local/partial value forms; project still prints its legacy form.
All 5 tested dumps (local, partial, unit coset, idele, class) succeed.
exp_at with real and 5 together is PARSE, and the one-prime call gives DOMAIN with no place field.
No documented p-adic digit, place-list or version command exists; exploratory names give PARSE.
This checks driver exposure, not absence of all possible C API calls.
On the same finite ball, driver equal gives true and compare gives undecided.
The notebook's A5 equality comparison therefore cannot be completed through its documented language.

Successful local branches and the projection have null dumps, although the driver can dump local/partial values.
No version field occurs in any of the 13 scenario records. The 3 failure cards say place: not given correctly.
There are 5 dump controls and 5 set tooltips in B, 0 of either in A.
The 5 stored dumps are genuine; no dump is described as a reconstruction certificate.

A periodic digit check used only finite arithmetic: multiply a repeated t-digit block's finite sum by
1-p^t. The terms cancel except the first block and its last shifted copy. For each fixed modulus p^k,
the last copy becomes 0 after enough repeats. Thus the eventual repeated part is B*p^r/(1-p^t).
This is the stepwise justification for the fraction checks; no outside formula is cited from memory.

## 6. Final assessment

2026-10-16T01:04:23+11:00: tests complete; preparing the final report.

2026-10-16T01:05:03+11:00: retained 4 observed scenario outputs and 24 source hashes.
Removed the temporary source/build/depot copy (2517 files) and 3 Firefox profiles.
Recreate the copy with `timeout 30 python3 lanes/ux-drive-notebook/prepare.py` before rerunning checks.
The shared ux/notebook files were read only. No git or bd command was run.

