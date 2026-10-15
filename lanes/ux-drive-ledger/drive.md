# Ledger test drive

Started: 2026-10-16T00:20:45.329354+11:00

## Rules and preparation

Read CLAUDE.md, lanes/COMMON.md, lanes/UX-COMMON.md, the ledger brief, and only ux/ledger/README.md.
Read SPEC 9.1 to 9.3.3 and PLAN 1F, as named by the brief.
No applicable AGENTS.md was found in the ancestor paths checked.
The build lane report has not been opened.
Only this lane is writable, apart from build outputs explicitly required by step 1.
All generated ledger artifacts will be directed to a lane-local copy.
No git state command, bd, installation, or subagent is used.
Initial read-only discovery commands were not wrapped in timeout; subsequent commands are bounded.
The README has descriptive text after its first shell command, without a comment marker.
I interpret that trailing text as prose; it would be passed to make if copied literally.

## Step 1: setup

At preparation time, build/adf did not exist. Expected: the documented build makes it.

### setup-copy at 2026-10-16T00:21:23+11:00

Expected: A lane-local copy preserves all original claim and seal files.

Command:

```sh
timeout 175 cp -r ux/ledger lanes/ux-drive-ledger/ledger
```

Exit: 0. Elapsed: 0.023 s. Full output: `setup-copy.log`.

```text

```

### setup-import at 2026-10-16T00:21:38+11:00

Expected: The README advertises 86 imported fixture claims.

Command:

```sh
timeout 175 python3 -B ux/ledger/import_fixtures.py --out lanes/ux-drive-ledger/ledger/claims
```

Exit: 0. Elapsed: 0.053 s. Full output: `setup-import.log`.

```text
import: 86 claims written to lanes/ux-drive-ledger/ledger/claims

```

### setup-dry-run at 2026-10-16T00:21:38+11:00

Expected: a1 exposes the script and expected line before execution.

Command:

```sh
timeout 175 python3 -B ux/ledger/seal.py --ledger lanes/ux-drive-ledger/ledger --only a1-cos-4-at-2 --dry-run
```

Exit: 0. Elapsed: 0.037 s. Full output: `setup-dry-run.log`.

```text
== a1-cos-4-at-2: cos(4) = 1 mod 16 in Z_2 (line 1 of the manuscript's table).
   driver lines:
     prec 4
     cos_at 4 with 2
   expected lines:
     2: 1 + O(2^4)
   expected exit status: 0

```

### setup-seal at 2026-10-16T00:21:38+11:00

Expected: Four refuted, one open, one proposed; all other claims sealed; exit 1.

Command:

```sh
timeout 175 python3 -B ux/ledger/seal.py --ledger lanes/ux-drive-ledger/ledger
```

Exit: 1. Elapsed: 0.670 s. Full output: `setup-seal.log`.

```text
a1-cos-4-at-2                refuted   unchanged; confirmed by build 22ccde6c6306
a2-v2-log-3                  sealed    unchanged; confirmed by build 22ccde6c6306
a3-exp-3-exp-12-mod-3-8      refuted   unchanged; confirmed by build 22ccde6c6306
a4-sin-3-at-3                sealed    unchanged; confirmed by build 22ccde6c6306
a5-exp-3-exp-12-distance     sealed    unchanged; confirmed by build 22ccde6c6306
a6-flint-equal-ignores-precision proposed  no script; left proposed
b1-exp-log-5-is-1            sealed    unchanged; confirmed by build 22ccde6c6306
b2-product-formula-6-35      sealed    unchanged; confirmed by build 22ccde6c6306
b3-exp-log-5-is-5            refuted   unchanged; confirmed by build 22ccde6c6306
b4-x2-plus-1-root-in-z2      refuted   unchanged; confirmed by build 22ccde6c6306
b5-1-sqrt-1-plus-8z2         sealed    unchanged; confirmed by build 22ccde6c6306
b5-2-sqrt-5-plus-8z2         sealed    unchanged; confirmed by build 22ccde6c6306
b5-sqrt-1-plus-4z2           open      unchanged; confirmed by build 22ccde6c6306
b6-two-square-family         sealed    unchanged; confirmed by build 22ccde6c6306
fx-01_spec_4_1               sealed    unchanged; confirmed by build 22ccde6c6306
fx-02_spec_4_2               sealed    unchanged; confirmed by build 22ccde6c6306
fx-03_spec_4_3               sealed    unchanged; confirmed by build 22ccde6c6306
fx-04_spec_4_4_cap           sealed    unchanged; confirmed by build 22ccde6c6306
fx-05_spec_4_5               sealed    unchanged; confirmed by build 22ccde6c6306
fx-06_pairs                  sealed    unchanged; confirmed by build 22ccde6c6306
fx-07_status                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-08_show_type              sealed    unchanged; confirmed by build 22ccde6c6306
fx-09_settings               sealed    unchanged; confirmed by build 22ccde6c6306
fx-10_line_language          sealed    unchanged; confirmed by build 22ccde6c6306
fx-11_guard                  sealed    unchanged; confirmed by build 22ccde6c6306
fx-12_status_order           sealed    unchanged; confirmed by build 22ccde6c6306
fx-13_dump                   sealed    unchanged; confirmed by build 22ccde6c6306
fx-catalogue-binomial        sealed    unchanged; confirmed by build 22ccde6c6306
fx-catalogue-cyclotomic      sealed    unchanged; confirmed by build 22ccde6c6306
fx-catalogue-power           sealed    unchanged; confirmed by build 22ccde6c6306
fx-catalogue-volume          sealed    unchanged; confirmed by build 22ccde6c6306
fx-char-class                sealed    unchanged; confirmed by build 22ccde6c6306
fx-char-dump                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-char-eval                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-char-values               sealed    unchanged; confirmed by build 22ccde6c6306
fx-drv-ball-ops              sealed    unchanged; confirmed by build 22ccde6c6306
fx-drv-ball-pairs            sealed    unchanged; confirmed by build 22ccde6c6306
fx-drv-ball-values           sealed    unchanged; confirmed by build 22ccde6c6306
fx-f-at-prime                sealed    unchanged; confirmed by build 22ccde6c6306
fx-f-cross                   sealed    unchanged; confirmed by build 22ccde6c6306
fx-f-places-1000             sealed    unchanged; confirmed by build 22ccde6c6306
fx-f-places-hostile          sealed    unchanged; confirmed by build 22ccde6c6306
fx-f-project                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-ffun-add                  sealed    unchanged; confirmed by build 22ccde6c6306
fx-ffun-algebra              sealed    unchanged; confirmed by build 22ccde6c6306
fx-ffun-dump                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-ffun-fourier              sealed    unchanged; confirmed by build 22ccde6c6306
fx-ffun-values               sealed    unchanged; confirmed by build 22ccde6c6306
fx-gfunc-log-refine-status   sealed    unchanged; confirmed by build 22ccde6c6306
fx-gfunc-log-refine-values   sealed    unchanged; confirmed by build 22ccde6c6306
fx-gfunc-log-status          sealed    unchanged; confirmed by build 22ccde6c6306
fx-gfunc-log-values          sealed    unchanged; confirmed by build 22ccde6c6306
fx-gfunc-root                sealed    unchanged; confirmed by build 22ccde6c6306
fx-gfunc-series              sealed    unchanged; confirmed by build 22ccde6c6306
fx-i-01-show-type            sealed    unchanged; confirmed by build 22ccde6c6306
fx-i-02-ucoset               sealed    unchanged; confirmed by build 22ccde6c6306
fx-i-03-idele                sealed    unchanged; confirmed by build 22ccde6c6306
fx-i-04-idclass              sealed    unchanged; confirmed by build 22ccde6c6306
fx-i-05-maps                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-i-06-status               sealed    unchanged; confirmed by build 22ccde6c6306
fx-localfactor-status        sealed    unchanged; confirmed by build 22ccde6c6306
fx-localfactor-values        sealed    unchanged; confirmed by build 22ccde6c6306
fx-poisson                   sealed    unchanged; confirmed by build 22ccde6c6306
fx-pow-status                sealed    unchanged; confirmed by build 22ccde6c6306
fx-pow-values                sealed    unchanged; confirmed by build 22ccde6c6306
fx-psi-class                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-psi-class-union           sealed    unchanged; confirmed by build 22ccde6c6306
fx-psi-local                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-psi-status                sealed    unchanged; confirmed by build 22ccde6c6306
fx-psi-values                sealed    unchanged; confirmed by build 22ccde6c6306
fx-qclass-arith              sealed    unchanged; confirmed by build 22ccde6c6306
fx-qclass-dump               sealed    unchanged; confirmed by build 22ccde6c6306
fx-qclass-lift               sealed    unchanged; confirmed by build 22ccde6c6306
fx-qclass-reduce             sealed    unchanged; confirmed by build 22ccde6c6306
fx-qclass-sets               sealed    unchanged; confirmed by build 22ccde6c6306
fx-qclass-text               sealed    unchanged; confirmed by build 22ccde6c6306
fx-rfun-algebra              sealed    unchanged; confirmed by build 22ccde6c6306
fx-rfun-dump                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-rfun-eval                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-rfun-fourier              sealed    unchanged; confirmed by build 22ccde6c6306
fx-rfun-text                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-root-status               sealed    unchanged; confirmed by build 22ccde6c6306
fx-root-values               sealed    unchanged; confirmed by build 22ccde6c6306
fx-s-hostile                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-s-realroots               sealed    unchanged; confirmed by build 22ccde6c6306
fx-s-recover                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-s-roots                   sealed    unchanged; confirmed by build 22ccde6c6306
fx-symbol-hilbert            sealed    unchanged; confirmed by build 22ccde6c6306
fx-symbol-residue            sealed    unchanged; confirmed by build 22ccde6c6306
fx-tate                      sealed    unchanged; confirmed by build 22ccde6c6306
fx-tate-continue             sealed    unchanged; confirmed by build 22ccde6c6306
fx-tate-fe                   sealed    unchanged; confirmed by build 22ccde6c6306
fx-tate-gamma                sealed    unchanged; confirmed by build 22ccde6c6306
fx-tate-local                sealed    unchanged; confirmed by build 22ccde6c6306
fx-tensor-eval               sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-hostile              sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-prime                sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-real                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-u-dump-local              sealed    unchanged; confirmed by build 22ccde6c6306
fx-u-dump-units              sealed    unchanged; confirmed by build 22ccde6c6306
seal: 100 claims; proposed 1, sealed 94, refuted 4, open 1, ratified 0; 0.5 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac

```

### setup-render at 2026-10-16T00:21:57+11:00

Expected: The static ledger renders all 100 claims.

Command:

```sh
timeout 175 python3 -B ux/ledger/render.py --ledger lanes/ux-drive-ledger/ledger
```

Exit: 0. Elapsed: 0.053 s. Full output: `setup-render.log`.

```text
render: lanes/ux-drive-ledger/ledger/ledger.html, 873531 bytes

```

### setup-firefox at 2026-10-16T00:21:57+11:00

Expected: Headless Firefox writes an image of the original ledger page.

Command:

```sh
timeout 175 bash -c 'timeout 45 firefox --headless --profile "$(mktemp -d lanes/ux-drive-ledger/firefox.XXXXXX)"
--screenshot "$PWD/lanes/ux-drive-ledger/ledger.png" "file://$PWD/ux/ledger/ledger.html"'
```

Exit: -11. Elapsed: 0.191 s. Full output: `setup-firefox.log`.

```text
*** You are running in headless mode.
timeout: the monitored command dumped core

```

### scenario-a at 2026-10-16T00:23:41+11:00

Expected: All five computed A claims reproduce; digits and distance agree; a6 remains uncomputed.

Command:

```sh
timeout 175 python3 -B lanes/ux-drive-ledger/scenarios.py a
```

Exit: 0. Elapsed: 0.068 s. Full output: `scenario-a.log`.

```text

a1-cos-4-at-2: cos(4) = 1 mod 16 in Z_2 (line 1 of the manuscript's table).
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/a1-cos-4-at-2.cmd
Input:
prec 4
cos_at 4 with 2
Output:
2: 9 + O(2^4)
exit=0

a2-v2-log-3: v_2(log 3) = 2 in Q_2.
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/a2-v2-log-3.cmd
Input:
prec 3
log_at 3 with 2
Output:
2: 4 + O(2^3)
exit=0

a3-exp-3-exp-12-mod-3-8: exp(3) = exp(12) mod 3^8 in Z_3 (line 3 of the manuscript's table).
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/a3-exp-3-exp-12-mod-3-8.cmd
Input:
prec 8
exp_at 3 with 3
exp_at 12 with 3
Output:
3: 958 + O(3^8)
3: 5125 + O(3^8)
exit=0

a4-sin-3-at-3: sin(3) = 3 mod 9 in Z_3.
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/a4-sin-3-at-3.cmd
Input:
prec 2
sin_at 3 with 3
Output:
3: 3 + O(3^2)
exit=0

a5-exp-3-exp-12-distance: exp(3) and exp(12) agree modulo 9 and not modulo 27 in Z_3, because exp preserves
distances on 3 Z_3 and v_3(12 - 3) = 2.
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/a5-exp-3-exp-12-distance.cmd
Input:
prec 2
exp_at 3 with 3
exp_at 12 with 3
prec 3
exp_at 3 with 3
exp_at 12 with 3
Output:
3: 4 + O(3^2)
3: 4 + O(3^2)
3: 13 + O(3^3)
3: 22 + O(3^3)
exit=0

a6-flint-equal-ignores-precision: FLINT's padic equality test says that exp(3) and exp(12) are equal in Q_3 at
precision 8, because it ignores precision.
No script: cannot reproduce FLINT equality through this driver. Proposed, not evidence.
Every x in 4 + 8 Z_2 is 4(1 + 2k); its unit factor is odd, hence v_2(x) = 2.
Exponential centres: [958, 5125] difference: 4167
Base-3 digits: 958 01022111
Base-3 digits: 5125 21000211
13 % 9 = 22 % 9 = 4; 22 - 13 = 9. The 3^2 digit is the first difference.

Independent assertions: 21

```

### scenario-a-dom at 2026-10-16T00:24:40+11:00

Expected: A cards show the exact driver lines, the differing digits and child links; filters select 6 A cards.

Command:

```sh
timeout 175 node lanes/ux-drive-ledger/dom-check.js ux/ledger/ledger.html a
```

Exit: 1. Elapsed: 0.024 s. Full output: `scenario-a-dom.log`.

```text
DOM state counts: {"refuted":4,"sealed":94,"proposed":1,"open":1}
node:internal/assert/utils:77
    throw err;
    ^

AssertionError [ERR_ASSERTION]: a1-cos-4-at-2 statement
    at check (/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/dom-check.js:11:45)
    at Object.<anonymous>
(/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/dom-check.js:33:3)
    at Module._compile (node:internal/modules/cjs/loader:1761:14)
    at Object..js (node:internal/modules/cjs/loader:1893:10)
    at Module.load (node:internal/modules/cjs/loader:1481:32)
    at Module._load (node:internal/modules/cjs/loader:1300:12)
    at TracingChannel.traceSync (node:diagnostics_channel:328:14)
    at wrapModuleLoad (node:internal/modules/cjs/loader:245:24)
    at Module.executeUserEntryPoint [as runMain] (node:internal/modules/run_main:154:5)
    at node:internal/main/run_main_module:33:47 {
  generatedMessage: false,
  code: 'ERR_ASSERTION',
  actual: false,
  expected: true,
  operator: '==',
  diff: 'simple'
}

Node.js v24.11.1

```

### readme-literal at 2026-10-16T00:24:52+11:00

Expected: The README build command should work when copied literally.

Command:

```sh
timeout 175 bash --noprofile --norc -c 'timeout 600 make -j2 && timeout 600 make -C tools/adf     the library and
the driver build/adf'
```

Exit: 2. Elapsed: 0.027 s. Full output: `readme-literal.log`.

```text
make: Nothing to be done for 'all'.
make: Entering directory '/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/tools/adf'
make: *** No rule to make target 'the'.  Stop.
make: Leaving directory '/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/tools/adf'

```

The first DOM check failed because my regex match destructuring read attributes as the card body.
I corrected the lane harness. The page writes digit exponents as plain text, so the check uses that form.
This was a harness error, not a slice defect. The original failing log remains recorded above.

Scenario A step 1: there is no claim bar. The README directs the user to JSON files and dry-run scripts.
Steps 2 to 4: all ten computed output lines reproduce; the four primary outcomes match the brief.
Step 5: there are no ratify, comment or report export controls. README explicitly defers ratification.
No screenshot was created: Firefox exited by signal 11. No visual layout conclusion is drawn.

### scenario-a-dom-fixed at 2026-10-16T00:25:17+11:00

Expected: A card content, digits, links and filters match the recorded data.

Command:

```sh
timeout 175 node lanes/ux-drive-ledger/dom-check.js ux/ledger/ledger.html a
```

Exit: 1. Elapsed: 0.037 s. Full output: `scenario-a-dom-fixed.log`.

```text
DOM state counts: {"refuted":4,"sealed":94,"proposed":1,"open":1}
a1-cos-4-at-2: card text and all script/expect/actual lines checked.
a2-v2-log-3: card text and all script/expect/actual lines checked.
a3-exp-3-exp-12-mod-3-8: card text and all script/expect/actual lines checked.
a4-sin-3-at-3: card text and all script/expect/actual lines checked.
a5-exp-3-exp-12-distance: card text and all script/expect/actual lines checked.
a6-flint-equal-ignores-precision: card text and all script/expect/actual lines checked.
Controls: <button type="button" data-filter="all" aria-pressed="true"> <button type="button" data-filter="proposed"
aria-pressed="false"> <button type="button" data-filter="sealed" aria-pressed="false"> <button type="button"
data-filter="refuted" aria-pressed="false"> <button type="button" data-filter="open" aria-pressed="false"> <button
type="button" data-filter="ratified" aria-pressed="false"> <select id="qsel">
No claim entry, comment, ratify, retract, cite, split or report export control found.
node:internal/assert/utils:77
    throw err;
    ^

AssertionError [ERR_ASSERTION]: status legend STATUS_COUNT
    at check (/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/dom-check.js:11:45)
    at Object.<anonymous>
(/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/dom-check.js:78:3)
    at Module._compile (node:internal/modules/cjs/loader:1761:14)
    at Object..js (node:internal/modules/cjs/loader:1893:10)
    at Module.load (node:internal/modules/cjs/loader:1481:32)
    at Module._load (node:internal/modules/cjs/loader:1300:12)
    at TracingChannel.traceSync (node:diagnostics_channel:328:14)
    at wrapModuleLoad (node:internal/modules/cjs/loader:245:24)
    at Module.executeUserEntryPoint [as runMain] (node:internal/modules/run_main:154:5)
    at node:internal/main/run_main_module:33:47 {
  generatedMessage: false,
  code: 'ERR_ASSERTION',
  actual: false,
  expected: true,
  operator: '==',
  diff: 'simple'
}

Node.js v24.11.1

```

### scenario-b at 2026-10-16T00:25:17+11:00

Expected: Eight B scripts reproduce; all 24 Legendre signs agree with exhaustive residues.

Command:

```sh
timeout 175 python3 -B lanes/ux-drive-ledger/scenarios.py b
```

Exit: 0. Elapsed: 0.060 s. Full output: `scenario-b.log`.

```text

b1-exp-log-5-is-1: exp(Log 5) = 1 at 5, written as two lines: Log 5 = 0 at 5, then exp(0) = 1 at 5.
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/b1-exp-log-5-is-1.cmd
Input:
prec 8
Log_at (5 ; 5 * [1]) with 5
exp_at 0 with 5
Output:
5: 0
5: 1
exit=0

b2-product-formula-6-35: The absolute values of 6/35 multiply to 1 over all places: |6/35|_inf |6/35|_2 |6/35|_3
|6/35|_5 |6/35|_7 = 1, and |6/35|_p = 1 at every other prime.
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/b2-product-formula-6-35.cmd
Input:
abs (1 ; 6/35 * [1]) with 2
abs (1 ; 6/35 * [1]) with 3
abs (1 ; 6/35 * [1]) with 5
abs (1 ; 6/35 * [1]) with 7
abs (1 ; 6/35 * [1]) with 11
prec 128
digits 10
norm (0.171428571428571428571428571 +/- 1e-27 ; 6/35 * [1])
Output:
1/2
1/3
5
7
1
1 +/- 8.4e-27
exit=0

b3-exp-log-5-is-5: exp(Log 5) = 5 at 5. Deliberately wrong expectation: the agent's first guess that exp undoes Log
at 5, kept as a refuted claim.
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/b3-exp-log-5-is-5.cmd
Input:
prec 8
Log_at (5 ; 5 * [1]) with 5
exp_at 0 with 5
Output:
5: 0
5: 1
exit=0

b4-x2-plus-1-root-in-z2: x^2 + 1 has a root in Z_2 (the agent's guess: the root 1 mod 2 lifts).
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/b4-x2-plus-1-root-in-z2.cmd
Input:
roots 1 0 1 with 2 with 8
Output:
none
exit=0

b5-1-sqrt-1-plus-8z2: On 1 + 8 Z_2 the square roots are 1 + O(2^2) and 3 + O(2^2) (all branches).
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/b5-1-sqrt-1-plus-8z2.cmd
Input:
prec 8
roots_at (* ; 1 mod 8) with 2 with 2
Output:
2 [+1]: 1 + O(2^2); 2 [-1]: 3 + O(2^2)
exit=0

b5-2-sqrt-5-plus-8z2: On 5 + 8 Z_2 there is no square root (all branches).
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/b5-2-sqrt-5-plus-8z2.cmd
Input:
prec 8
roots_at (* ; 5 mod 8) with 2 with 2
Output:
error: DOMAIN
exit=1

b5-sqrt-1-plus-4z2: The square roots of 1 + 4 Z_2 are determined to 2 digits (all branches).
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/b5-sqrt-1-plus-4z2.cmd
Input:
prec 8
roots_at (* ; 1 mod 4) with 2 with 2
Output:
error: NOT_DETERMINED
exit=1

b6-two-square-family: 2 is a square in Q_p exactly when p = 1 or 7 mod 8 (p odd); computed for the odd primes 3 to
97, argued for every larger p.
Command: timeout 20 /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/build/adf
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/b6-two-square-family.cmd
Input:
legendre 2 with 3
legendre 2 with 5
legendre 2 with 7
legendre 2 with 11
legendre 2 with 13
legendre 2 with 17
legendre 2 with 19
legendre 2 with 23
legendre 2 with 29
legendre 2 with 31
legendre 2 with 37
legendre 2 with 41
legendre 2 with 43
legendre 2 with 47
legendre 2 with 53
legendre 2 with 59
legendre 2 with 61
legendre 2 with 67
legendre 2 with 71
legendre 2 with 73
legendre 2 with 79
legendre 2 with 83
legendre 2 with 89
legendre 2 with 97
Output:
-1
-1
1
-1
-1
1
-1
1
-1
1
-1
1
-1
1
-1
-1
-1
-1
1
1
1
-1
1
1
exit=0
p=3, roots modulo p=[], sign=-1
p=5, roots modulo p=[], sign=-1
p=7, roots modulo p=[3, 4], sign=1
p=11, roots modulo p=[], sign=-1
p=13, roots modulo p=[], sign=-1
p=17, roots modulo p=[6, 11], sign=1
p=19, roots modulo p=[], sign=-1
p=23, roots modulo p=[5, 18], sign=1
p=29, roots modulo p=[], sign=-1
p=31, roots modulo p=[8, 23], sign=1
p=37, roots modulo p=[], sign=-1
p=41, roots modulo p=[17, 24], sign=1
p=43, roots modulo p=[], sign=-1
p=47, roots modulo p=[7, 40], sign=1
p=53, roots modulo p=[], sign=-1
p=59, roots modulo p=[], sign=-1
p=61, roots modulo p=[], sign=-1
p=67, roots modulo p=[], sign=-1
p=71, roots modulo p=[12, 59], sign=1
p=73, roots modulo p=[32, 41], sign=1
p=79, roots modulo p=[9, 70], sign=1
p=83, roots modulo p=[], sign=-1
p=89, roots modulo p=[25, 64], sign=1
p=97, roots modulo p=[14, 83], sign=1
Squares modulo 8: [0, 1, 4] ; 5 is absent. Odd squares are 1 mod 8.
6/35 = 2*3/(5*7); the only prime factors are 2, 3, 5, 7.
Exact product: 1 ; input-centre error: -3/7000000000000000000000000000
Norm interval fits in 1 +/- 8.4e-27. This is an enclosure, not a computed exact equality.

Independent assertions: 59

```

### scenario-b-dom at 2026-10-16T00:25:17+11:00

Expected: B cards and filters show 5 sealed, 2 refuted, 1 open and the split children.

Command:

```sh
timeout 175 node lanes/ux-drive-ledger/dom-check.js ux/ledger/ledger.html b
```

Exit: 1. Elapsed: 0.046 s. Full output: `scenario-b-dom.log`.

```text
DOM state counts: {"refuted":4,"sealed":94,"proposed":1,"open":1}
b1-exp-log-5-is-1: card text and all script/expect/actual lines checked.
b2-product-formula-6-35: card text and all script/expect/actual lines checked.
b3-exp-log-5-is-5: card text and all script/expect/actual lines checked.
b4-x2-plus-1-root-in-z2: card text and all script/expect/actual lines checked.
b5-1-sqrt-1-plus-8z2: card text and all script/expect/actual lines checked.
b5-2-sqrt-5-plus-8z2: card text and all script/expect/actual lines checked.
b5-sqrt-1-plus-4z2: card text and all script/expect/actual lines checked.
b6-two-square-family: card text and all script/expect/actual lines checked.
Controls: <button type="button" data-filter="all" aria-pressed="true"> <button type="button" data-filter="proposed"
aria-pressed="false"> <button type="button" data-filter="sealed" aria-pressed="false"> <button type="button"
data-filter="refuted" aria-pressed="false"> <button type="button" data-filter="open" aria-pressed="false"> <button
type="button" data-filter="ratified" aria-pressed="false"> <select id="qsel">
No claim entry, comment, ratify, retract, cite, split or report export control found.
node:internal/assert/utils:77
    throw err;
    ^

AssertionError [ERR_ASSERTION]: status legend STATUS_COUNT
    at check (/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/dom-check.js:11:45)
    at Object.<anonymous>
(/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/dom-check.js:78:3)
    at Module._compile (node:internal/modules/cjs/loader:1761:14)
    at Object..js (node:internal/modules/cjs/loader:1893:10)
    at Module.load (node:internal/modules/cjs/loader:1481:32)
    at Module._load (node:internal/modules/cjs/loader:1300:12)
    at TracingChannel.traceSync (node:diagnostics_channel:328:14)
    at wrapModuleLoad (node:internal/modules/cjs/loader:245:24)
    at Module.executeUserEntryPoint [as runMain] (node:internal/modules/run_main:154:5)
    at node:internal/main/run_main_module:33:47 {
  generatedMessage: false,
  code: 'ERR_ASSERTION',
  actual: false,
  expected: true,
  operator: '==',
  diff: 'simple'
}

Node.js v24.11.1

```

The next DOM checks reached the legend, then my header regex treated STATUS_COUNT as a status.
STATUS_COUNT is the number of statuses, not a status. The harness now checks the 11 actual names only.
All preceding card-content and filter checks passed in those runs.

## Step 3: scenario B

All eight scripts and 38 output lines reproduce. The 24 symbol values also match residue enumeration.
The queue has five sealed B claims, not the nine of the full vision. Two are refuted and one is open.
The two root children link to the open parent. The family is sealed, with an argued tail, not an ARGUED state.
The family already has citations; the citation-entry and ratification workflow is absent from the controls.
The product formula card explains its real ball and promoted expected line. Its exact equality is argued.

## Step 4: three extra promises

Try branch naming (Surface: Branches), source-gated ratification (Interaction: agents cannot ratify),
and the computed footprint (Surface: a filled box is a place where the claim was computed).
Use isolated copies. These are user-file edits supported by FORMAT.md, not changes to the slice.

### scenario-a-dom-final at 2026-10-16T00:26:12+11:00

Expected: All A DOM checks including the 11 status names pass.

Command:

```sh
timeout 175 node lanes/ux-drive-ledger/dom-check.js ux/ledger/ledger.html a
```

Exit: 0. Elapsed: 0.074 s. Full output: `scenario-a-dom-final.log`.

```text
DOM state counts: {"refuted":4,"sealed":94,"proposed":1,"open":1}
a1-cos-4-at-2: card text and all script/expect/actual lines checked.
a2-v2-log-3: card text and all script/expect/actual lines checked.
a3-exp-3-exp-12-mod-3-8: card text and all script/expect/actual lines checked.
a4-sin-3-at-3: card text and all script/expect/actual lines checked.
a5-exp-3-exp-12-distance: card text and all script/expect/actual lines checked.
a6-flint-equal-ignores-precision: card text and all script/expect/actual lines checked.
Controls: <button type="button" data-filter="all" aria-pressed="true"> <button type="button" data-filter="proposed"
aria-pressed="false"> <button type="button" data-filter="sealed" aria-pressed="false"> <button type="button"
data-filter="refuted" aria-pressed="false"> <button type="button" data-filter="open" aria-pressed="false"> <button
type="button" data-filter="ratified" aria-pressed="false"> <select id="qsel">
No claim entry, comment, ratify, retract, cite, split or report export control found.
DOM assertions: 63
The JS run used lightweight element stubs; no pixel or layout verification is claimed.

```

### scenario-b-dom-final at 2026-10-16T00:26:12+11:00

Expected: All B DOM checks including the 11 status names pass.

Command:

```sh
timeout 175 node lanes/ux-drive-ledger/dom-check.js ux/ledger/ledger.html b
```

Exit: 0. Elapsed: 0.075 s. Full output: `scenario-b-dom-final.log`.

```text
DOM state counts: {"refuted":4,"sealed":94,"proposed":1,"open":1}
b1-exp-log-5-is-1: card text and all script/expect/actual lines checked.
b2-product-formula-6-35: card text and all script/expect/actual lines checked.
b3-exp-log-5-is-5: card text and all script/expect/actual lines checked.
b4-x2-plus-1-root-in-z2: card text and all script/expect/actual lines checked.
b5-1-sqrt-1-plus-8z2: card text and all script/expect/actual lines checked.
b5-2-sqrt-5-plus-8z2: card text and all script/expect/actual lines checked.
b5-sqrt-1-plus-4z2: card text and all script/expect/actual lines checked.
b6-two-square-family: card text and all script/expect/actual lines checked.
Controls: <button type="button" data-filter="all" aria-pressed="true"> <button type="button" data-filter="proposed"
aria-pressed="false"> <button type="button" data-filter="sealed" aria-pressed="false"> <button type="button"
data-filter="refuted" aria-pressed="false"> <button type="button" data-filter="open" aria-pressed="false"> <button
type="button" data-filter="ratified" aria-pressed="false"> <select id="qsel">
No claim entry, comment, ratify, retract, cite, split or report export control found.
DOM assertions: 149
The JS run used lightweight element stubs; no pixel or layout verification is claimed.

```

### extra-promises at 2026-10-16T00:26:53+11:00

Expected: Branch ambiguity is refused; missing sources block ratification; uncomputed places stay unfilled.

Command:

```sh
timeout 175 python3 -B lanes/ux-drive-ledger/extras.py
```

Exit: 0. Elapsed: 0.479 s. Full output: `extra-promises.log`.

```text
CLAIM drive-branch places= [5] state= proposed script lines= 2
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/extra-ledger --only drive-branch
drive-branch                 sealed    first seal
seal: 1 claims; proposed 0, sealed 1, refuted 0, open 0, ratified 0; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac
  exit= 0
CLAIM drive-citation places= [3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83,
89, 97] state= ratified script lines= 24
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/extra-ledger --only drive-citation
drive-citation               ratified  first seal
seal: 1 claims; proposed 0, sealed 0, refuted 0, open 0, ratified 1; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac
  exit= 0
CLAIM drive-footprint places= [3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83,
89, 97, 101] state= proposed script lines= 24
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/extra-ledger --only drive-footprint
drive-footprint              sealed    first seal
seal: 1 claims; proposed 0, sealed 1, refuted 0, open 0, ratified 0; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac
  exit= 0
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/render.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/extra-ledger
render: lanes/ux-drive-ledger/extra-ledger/ledger.html, 32076 bytes
  exit= 0
drive-branch verdict= sealed actual= ['5 [2]: 2 + O(5^1); 5 [3]: 3 + O(5^1)']
PROMISE RESULT: FAILED: ambiguous singular branch claim accepted
drive-citation verdict= sealed actual= ['-1', '-1', '1', '-1', '-1', '1', '-1', '1', '-1', '1', '-1', '1', '-1',
'1', '-1', '-1', '-1', '-1', '1', '1', '1', '-1', '1', '1']
PROMISE RESULT: FAILED: missing-source claim displayed RATIFIED
drive-footprint verdict= sealed actual= ['-1', '-1', '1', '-1', '-1', '1', '-1', '1', '-1', '1', '-1', '1', '-1',
'1', '-1', '-1', '-1', '-1', '1', '1', '1', '-1', '1', '1']
PROMISE RESULT: 101 not displayed computed
Three extra promises tried. Each result is printed above; no slice files changed.

```

Step 4 outcomes: branch ambiguity seals; a source-free all-primes argument can display RATIFIED.
The added 101 is omitted from the visible strip because the prime family is compressed into a range.
That compressed strip requires a follow-up to distinguish hidden metadata from a computed-place assertion.

## Step 5: builder comparison

Steps 1 to 4 are now complete. The build lane report is opened for the first time below.

### builder-unit-tests at 2026-10-16T00:27:52+11:00

Expected: The builder reports 16 passing unit tests.

Command:

```sh
timeout 175 env TMPDIR=lanes/ux-drive-ledger/tmp PYTHONDONTWRITEBYTECODE=1 python3 -B -m unittest discover -s
ux/ledger -v
```

Exit: 0. Elapsed: 1.766 s. Full output: `builder-unit-tests.log`.

```text
test_compare_rules (test_ledger.TestCompare.test_compare_rules) ... ok
test_status_classes_cover_status_h (test_ledger.TestCompare.test_status_classes_cover_status_h) ... ok
test_footprint_rule (test_ledger.TestImporter.test_footprint_rule) ... ok
test_two_fixtures (test_ledger.TestImporter.test_two_fixtures) ... ok
test_exp_matches_driver_readme (test_ledger.TestOracle.test_exp_matches_driver_readme) ... ok
test_digit_difference (test_ledger.TestRenderer.test_digit_difference) ... ok
test_page_holds_every_id (test_ledger.TestRenderer.test_page_holds_every_id) ... ok
test_build_identity (test_ledger.TestSealer.test_build_identity) ... ok
test_changed_claim_is_resealed_not_reopened (test_ledger.TestSealer.test_changed_claim_is_resealed_not_reopened) ...
ok
test_dry_run_runs_nothing (test_ledger.TestSealer.test_dry_run_runs_nothing) ... ok
test_failing_setting_prints_a_line (test_ledger.TestSealer.test_failing_setting_prints_a_line) ... ok
test_only_and_exit_zero (test_ledger.TestSealer.test_only_and_exit_zero) ... ok
test_other_build_same_lines_keeps_seal (test_ledger.TestSealer.test_other_build_same_lines_keeps_seal) ... ok
test_reopen_on_other_build (test_ledger.TestSealer.test_reopen_on_other_build) ... ok
test_reseal_same_result_keeps_file (test_ledger.TestSealer.test_reseal_same_result_keeps_file) ... ok
test_verdicts (test_ledger.TestSealer.test_verdicts) ... ok

----------------------------------------------------------------------
Ran 16 tests in 1.642s

OK

```

### driver-demands at 2026-10-16T00:28:23+11:00

Expected: Confirm missing record fields, version command, certificates and predicates; test existing dump forms.

Command:

```sh
timeout 175 build/adf -v lanes/ux-drive-ledger/driver-demands.cmd
```

Exit: 1. Elapsed: 0.004 s. Full output: `driver-demands.log`.

```text
adf: line 1: prec: OK
adf: line 2: roots_at: NOT_DETERMINED
adf: line 3: recover: OK
adf: line 4: roots: OK
adf: line 5: root_at: OK
adf: line 6: equal: PARSE
adf: line 7: contains: PARSE
adf: line 8: contains: DOMAIN
adf: line 9: equal: DOMAIN
adf: line 10: dump: OK
adf: line 11: dump: OK
adf: line 12: dump: OK
adf: line 13: dump: PARSE
adf: line 14: dump: PARSE
adf: line 15: version: PARSE
adf: line 16: adf_version_check: PARSE
adf: line 17: recon_cert_check: PARSE
adf: line 18: prec: OK
adf: line 19: idele: OK
adf: line 20: show: PARSE
error: NOT_DETERMINED
22/7
235 mod 7^4; 2166 mod 7^4
5 [2]: 280182 + O(5^8)
error: PARSE
error: PARSE
error: DOMAIN
error: DOMAIN
adf1 Q ucoset 5 6
adf1 Q idele 1 1 0 0 0 1 1 1 0
adf1 Q idclass 1 0 0 0 1 0
error: PARSE
error: PARSE
error: PARSE
error: PARSE
error: PARSE
(0.17142857142857142857 +/- 1.5e-21 ; 6/35 * [1])
error: PARSE

```

### flint-equality-build at 2026-10-16T00:28:49+11:00

Expected: Compile a direct test of the uncomputed a6 FLINT equality claim.

Command:

```sh
timeout 175 cc -std=c11 -Wall -Wextra -Werror lanes/ux-drive-ledger/flint-equality.c -lflint -lgmp -o
lanes/ux-drive-ledger/flint-equality
```

Exit: 0. Elapsed: 0.087 s. Full output: `flint-equality-build.log`.

```text

```

### flint-equality at 2026-10-16T00:28:49+11:00

Expected: a6 claims padic_equal returns true for exp(3), exp(12), both at precision 8.

Command:

```sh
timeout 175 lanes/ux-drive-ledger/flint-equality
```

Exit: 0. Elapsed: 0.003 s. Full output: `flint-equality.log`.

```text
exp success: 1 1
exp(3): 958
exp(12): 5125
precision: 8 8
padic_equal: 0

```

The first driver-demand probe used an invalid local-ball spelling (`mod` instead of `+ O(...)`).
The corrected probe below uses the form in tests/driver/u-dump-local.cmd. Original errors remain logged.
The 101 footprint follow-up finds `class="c wide"` with title listing 101 and text `11 ... 101`.
Thus the third extra promise fails too: 101 is included in the filled computed group without a driver call.
The exact span is saved in the extra page. The first probe searched only a single-prime span and missed it.

### driver-demands-fixed at 2026-10-16T00:29:33+11:00

Expected: Valid local and partial balls dump successfully but their set predicates are unsupported by the driver.

Command:

```sh
timeout 175 build/adf -v lanes/ux-drive-ledger/driver-demands-fixed.cmd
```

Exit: 1. Elapsed: 0.004 s. Full output: `driver-demands-fixed.log`.

```text
adf: line 1: prec: OK
adf: line 2: roots_at: NOT_DETERMINED
adf: line 3: recover: OK
adf: line 4: roots: OK
adf: line 5: root_at: OK
adf: line 6: equal: DOMAIN
adf: line 7: contains: DOMAIN
adf: line 8: contains: DOMAIN
adf: line 9: equal: DOMAIN
adf: line 10: dump: OK
adf: line 11: dump: OK
adf: line 12: dump: OK
adf: line 13: dump: OK
adf: line 14: dump: OK
adf: line 15: version: PARSE
adf: line 16: adf_version_check: PARSE
adf: line 17: recon_cert_check: PARSE
adf: line 18: prec: OK
adf: line 19: idele: OK
adf: line 20: show: PARSE
error: NOT_DETERMINED
22/7
235 mod 7^4; 2166 mod 7^4
5 [2]: 280182 + O(5^8)
error: DOMAIN
error: DOMAIN
error: DOMAIN
error: DOMAIN
adf1 Q ucoset 5 6
adf1 Q idele 1 1 0 0 0 1 1 1 0
adf1 Q idclass 1 0 0 0 1 0
adf1 Q lball 3 b 1 0 2
adf1 Q sball n 1 3 b 1 0 2
error: PARSE
error: PARSE
error: PARSE
(0.17142857142857142857 +/- 1.5e-21 ; 6/35 * [1])
error: PARSE

```

### builder-checks at 2026-10-16T00:30:43+11:00

Expected: Regeneration is byte-identical, reopening persists, and one classification mutant is killed.

Command:

```sh
timeout 175 python3 -B lanes/ux-drive-ledger/builder-checks.py
```

Exit: 1. Elapsed: 1.102 s. Full output: `builder-checks.log`.

```text
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/import_fixtures.py --out
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/ledger/claims
exit= 0 tail= import: 86 claims written to lanes/ux-drive-ledger/ledger/claims

PASS: repeat-import exit
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/ledger
exit= 1 tail= e6c6306
fx-tate-gamma                sealed    unchanged; confirmed by build 22ccde6c6306
fx-tate-local                sealed    unchanged; confirmed by build 22ccde6c6306
fx-tensor-eval               sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-hostile              sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-prime                sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-real                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-u-dump-local              sealed    unchanged; confirmed by build 22ccde6c6306
fx-u-dump-units              sealed    unchanged; confirmed by build 22ccde6c6306
seal: 100 claims; proposed 1, sealed 94, refuted 4, open 1, ratified 0; 0.4 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac

PASS: repeat-seal exit
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/render.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/ledger
exit= 0 tail= render: lanes/ux-drive-ledger/ledger/ledger.html, 873531 bytes

PASS: repeat-render exit
PASS: 200 claim/seal/page files byte-identical after repeat
PASS: 200 generated files equal committed original
COMMAND: node --check /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/page-script.js
exit= 0 tail= 
PASS: node-syntax exit
PASS: 0 source/README lines over 116
PASS: README has 73 lines
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 3 3 8
exit= 0 tail= 3: 958 + O(3^8)

PASS: oracle-3-8 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 12 3 8
exit= 0 tail= 3: 5125 + O(3^8)

PASS: oracle-12-8 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 3 3 2
exit= 0 tail= 3: 4 + O(3^2)

PASS: oracle-3-2 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 12 3 2
exit= 0 tail= 3: 4 + O(3^2)

PASS: oracle-12-2 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 3 3 3
exit= 0 tail= 3: 13 + O(3^3)

PASS: oracle-3-3 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 12 3 3
exit= 0 tail= 3: 22 + O(3^3)

PASS: oracle-12-3 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/reopened-ledger
exit= 1 tail= a2-v2-log-3                  open      REOPENED (a later build wrote other lines than the earlier
seal)
seal: 1 claims; proposed 0, sealed 0, refuted 0, open 1, ratified 0; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac

PASS: reopen-seal exit
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/render.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/reopened-ledger
exit= 0 tail= render: lanes/ux-drive-ledger/reopened-ledger/ledger.html, 18231 bytes

PASS: reopen-render exit
PASS: reopened verdict open, current run sealed
PASS: one historical seal
Traceback (most recent call last):
  File "/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/builder-checks.py", line 68, in
<module>
    check('REOPENED.' in html and 'f'*64 in html and '2: 0 + O(2^3)' in html,'page names earlier result and build')
  File "/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/builder-checks.py", line 19, in
check
    assert value, label
           ^^^^^
AssertionError: page names earlier result and build

```

The first builder-comparison run passed 24 assertions, then expected the full earlier build hash on the page.
The page names builds by a 12-character prefix in its reopening notice, so that assertion was over-specific.
The corrected check accepts the documented identifying prefix; the full earlier hash remains in the seal file.

### builder-checks-final at 2026-10-16T00:30:57+11:00

Expected: Reopening identifies both builds and persists; one comparison mutant is killed.

Command:

```sh
timeout 175 python3 -B lanes/ux-drive-ledger/builder-checks.py
```

Exit: 1. Elapsed: 1.195 s. Full output: `builder-checks-final.log`.

```text
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/import_fixtures.py --out
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/ledger/claims
exit= 0 tail= import: 86 claims written to lanes/ux-drive-ledger/ledger/claims

PASS: repeat-import exit
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/ledger
exit= 1 tail= e6c6306
fx-tate-gamma                sealed    unchanged; confirmed by build 22ccde6c6306
fx-tate-local                sealed    unchanged; confirmed by build 22ccde6c6306
fx-tensor-eval               sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-hostile              sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-prime                sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-real                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-u-dump-local              sealed    unchanged; confirmed by build 22ccde6c6306
fx-u-dump-units              sealed    unchanged; confirmed by build 22ccde6c6306
seal: 100 claims; proposed 1, sealed 94, refuted 4, open 1, ratified 0; 0.4 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac

PASS: repeat-seal exit
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/render.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/ledger
exit= 0 tail= render: lanes/ux-drive-ledger/ledger/ledger.html, 873531 bytes

PASS: repeat-render exit
PASS: 200 claim/seal/page files byte-identical after repeat
PASS: 200 generated files equal committed original
COMMAND: node --check /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/page-script.js
exit= 0 tail= 
PASS: node-syntax exit
PASS: 0 source/README lines over 116
PASS: README has 73 lines
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 3 3 8
exit= 0 tail= 3: 958 + O(3^8)

PASS: oracle-3-8 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 12 3 8
exit= 0 tail= 3: 5125 + O(3^8)

PASS: oracle-12-8 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 3 3 2
exit= 0 tail= 3: 4 + O(3^2)

PASS: oracle-3-2 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 12 3 2
exit= 0 tail= 3: 4 + O(3^2)

PASS: oracle-12-2 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 3 3 3
exit= 0 tail= 3: 13 + O(3^3)

PASS: oracle-3-3 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 12 3 3
exit= 0 tail= 3: 22 + O(3^3)

PASS: oracle-12-3 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/reopened-ledger
exit= 1 tail= a2-v2-log-3                  open      REOPENED (a later build wrote other lines than the earlier
seal)
seal: 1 claims; proposed 0, sealed 0, refuted 0, open 1, ratified 0; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac

PASS: reopen-seal exit
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/render.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/reopened-ledger
exit= 0 tail= render: lanes/ux-drive-ledger/reopened-ledger/ledger.html, 18328 bytes

PASS: reopen-render exit
PASS: reopened verdict open, current run sealed
Traceback (most recent call last):
  File "/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/builder-checks.py", line 66, in
<module>
    check(len(list((reopened/'seals/history').glob('*.json')))==1,'one historical seal')
  File "/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/builder-checks.py", line 19, in
check
    assert value, label
           ^^^^^
AssertionError: one historical seal

```

The second builder-comparison run reused my reopening directory and found 2 historical seals, not 1.
That was test isolation in my harness. The next run uses a new lane-local directory and retains both old runs.

### builder-checks-complete at 2026-10-16T00:31:21+11:00

Expected: All builder checks complete in a fresh reopening directory.

Command:

```sh
timeout 175 python3 -B lanes/ux-drive-ledger/builder-checks.py
```

Exit: 0. Elapsed: 1.264 s. Full output: `builder-checks-complete.log`.

```text
test_compare_rules (test_ledger.TestCompare.test_compare_rules) ... FAIL

======================================================================
FAIL: test_compare_rules (test_ledger.TestCompare.test_compare_rules)
----------------------------------------------------------------------
Traceback (most recent call last):
  File "/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/test_ledger.py", line 208, in
test_compare_rules
    self.assertEqual(ledger.compare(['1'], ['error: LIMIT'], None, 1)[0], 'open')
AssertionError: 'refuted' != 'open'
- refuted
+ open


----------------------------------------------------------------------
Ran 1 test in 0.000s

FAILED (failures=1)
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/import_fixtures.py --out
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/ledger/claims
exit= 0 tail= import: 86 claims written to lanes/ux-drive-ledger/ledger/claims

PASS: repeat-import exit
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/ledger
exit= 1 tail= e6c6306
fx-tate-gamma                sealed    unchanged; confirmed by build 22ccde6c6306
fx-tate-local                sealed    unchanged; confirmed by build 22ccde6c6306
fx-tensor-eval               sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-hostile              sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-prime                sealed    unchanged; confirmed by build 22ccde6c6306
fx-trig-real                 sealed    unchanged; confirmed by build 22ccde6c6306
fx-u-dump-local              sealed    unchanged; confirmed by build 22ccde6c6306
fx-u-dump-units              sealed    unchanged; confirmed by build 22ccde6c6306
seal: 100 claims; proposed 1, sealed 94, refuted 4, open 1, ratified 0; 0.4 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac

PASS: repeat-seal exit
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/render.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/ledger
exit= 0 tail= render: lanes/ux-drive-ledger/ledger/ledger.html, 873531 bytes

PASS: repeat-render exit
PASS: 200 claim/seal/page files byte-identical after repeat
PASS: 200 generated files equal committed original
COMMAND: node --check /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/page-script.js
exit= 0 tail= 
PASS: node-syntax exit
PASS: 0 source/README lines over 116
PASS: README has 73 lines
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 3 3 8
exit= 0 tail= 3: 958 + O(3^8)

PASS: oracle-3-8 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 12 3 8
exit= 0 tail= 3: 5125 + O(3^8)

PASS: oracle-12-8 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 3 3 2
exit= 0 tail= 3: 4 + O(3^2)

PASS: oracle-3-2 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 12 3 2
exit= 0 tail= 3: 4 + O(3^2)

PASS: oracle-12-2 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 3 3 3
exit= 0 tail= 3: 13 + O(3^3)

PASS: oracle-3-3 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/oracle.py 12 3 3
exit= 0 tail= 3: 22 + O(3^3)

PASS: oracle-12-3 exit
PASS: oracle centre matches independent driver run
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/reopened-47heyj0r
exit= 1 tail= a2-v2-log-3                  open      REOPENED (a later build wrote other lines than the earlier
seal)
seal: 1 claims; proposed 0, sealed 0, refuted 0, open 1, ratified 0; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac

PASS: reopen-seal exit
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/render.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/reopened-47heyj0r
exit= 0 tail= render: lanes/ux-drive-ledger/reopened-47heyj0r/ledger.html, 18231 bytes

PASS: reopen-render exit
PASS: reopened verdict open, current run sealed
PASS: one historical seal
PASS: page names earlier result and build
COMMAND: python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/reopened-47heyj0r
exit= 1 tail= a2-v2-log-3                  open      unchanged
seal: 1 claims; proposed 0, sealed 0, refuted 0, open 1, ratified 0; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac

PASS: reopen-repeat exit
PASS: reopening persists
PASS: one exact mutation site
PASS: LIMIT-to-proved-no mutant killed by 1 assertion failure
Builder comparison assertions: 29

```

### status-checks at 2026-10-16T00:32:32+11:00

Expected: All driver error forms receive D-UX4 classification; missing fields and void ratification are handled.

Command:

```sh
timeout 175 python3 -B lanes/ux-drive-ledger/status-checks.py
```

Exit: 0. Elapsed: 0.729 s. Full output: `status-checks.log`.

```text
render: lanes/ux-drive-ledger/status-ledger/ledger.html, 25490 bytes
COMMAND: timeout 30 python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/status-ledger --only drive-prefixed-open
drive-prefixed-open          refuted   first seal
seal: 1 claims; proposed 0, sealed 0, refuted 1, open 0, ratified 0; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac
 exit= 1
actual= ['continuation S=((1 +/- 0.001) + (0)*i ; 0): error: NOT_DETERMINED'] expected verdict= open observed
verdict= refuted
DEFECT: status is misclassified
COMMAND: timeout 30 python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/status-ledger --only drive-limit
drive-limit                  open      first seal
seal: 1 claims; proposed 0, sealed 0, refuted 0, open 1, ratified 0; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac
 exit= 1
actual= ['error: LIMIT'] expected verdict= open observed verdict= open
COMMAND: timeout 30 python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/status-ledger --only drive-parse
drive-parse                  refuted   first seal
seal: 1 claims; proposed 0, sealed 0, refuted 1, open 0, ratified 0; 0.0 s; build adf 22ccde6c6306, FLINT 3.0.1,
commit 8cb2ec2c71ac
 exit= 1
actual= ['error: PARSE'] expected verdict= refuted observed verdict= refuted
Prefixed NOT_DETERMINED status badge present: False
Prefixed card displays REFUTED: True
COMMAND: timeout 30 python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/seal.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/invalid-ledger
seal: missing-expect.json: no field expect
 exit= 2
COMMAND: timeout 30 python3 -B /home/tobias/Projects/adelefeld-wt/ux-drive-ledger/ux/ledger/render.py --ledger
/home/tobias/Projects/adelefeld-wt/ux-drive-ledger/lanes/ux-drive-ledger/void-ratification
render: lanes/ux-drive-ledger/void-ratification/ledger.html, 18285 bytes
 exit= 0
Ratifying a refuted claim remains REFUTED and says ratification is void.
False computed footprint: <span class="c wide" title="22 primes: 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71 73
79 83 89 97 101">11 ... 101</span>
Imported empty footprints: 29 ['fx-rfun-eval', 'fx-s-roots', 'fx-qclass-dump', 'fx-ffun-fourier', 'fx-tate-local',
'fx-09_settings', 'fx-poisson', 'fx-11_guard', 'fx-ffun-add', 'fx-localfactor-status', 'fx-char-values', 'fx-tate',
'fx-ffun-values', 'fx-tate-continue', 'fx-rfun-dump', 'fx-tate-fe', 'fx-rfun-text', 'fx-rfun-algebra',
'fx-f-places-1000', 'fx-char-dump', 'fx-trig-hostile', 'fx-ffun-dump', 'fx-ffun-algebra', 'fx-rfun-fourier',
'fx-s-realroots', 'fx-tate-gamma', 'fx-psi-status', 'fx-10_line_language', 'fx-s-hostile']
Imported claims with "with real": 20 ; R represented: 0
Status controls verified: raw LIMIT open, raw PARSE refuted, missing field exit 2, void ratification.

```

### all-records at 2026-10-16T00:33:12+11:00

Expected: All 99 stored computed records match separate direct driver runs and exit codes.

Command:

```sh
timeout 175 python3 -B lanes/ux-drive-ledger/all-records.py
```

Exit: 0. Elapsed: 0.575 s. Full output: `all-records.log`.

```text
Independent driver invocations: 99
Output lines reproduced: 1720
Output and exit comparisons: 198
Mismatch count: 0

```

## Step 5 completed and step 6 report

At 2026-10-16T00:39:09+11:00, all work and the report are complete.
All 99 computed seals match 1,720 lines from separate driver runs. There are 198 matching output/exit checks.
The final builder check has 29 assertions, one expected killed mutant, and byte identity for 200 artifacts.
The direct FLINT probe refutes a6. The prefixed-status probe exposes a wrong refuted state.
The source-free ratification and ambiguous branch are recorded as workflow limitations.
The computed-place overstatement and literal README failure complete the four defects.
Final counts: 4 defects, 6 usability findings, 4 honesty findings; 638 final passing assertions/test cases.
The report lists every substantive check and every failed harness attempt.
The builder's report has no finding that was silently adopted as evidence.
The driver-demand experiment confirms missing fields, commands and predicates, with working typed dumps.
Long transcript lines below are visually wrapped to 116 characters. Exact commands and output are retained
in drive.raw.txt and each named log. This wrapping is presentation only; no result is changed.
