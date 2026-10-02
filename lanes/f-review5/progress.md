# f-review5 running notes

Only lanes/f-review5 is writable for this review. No repository suites will be run.
The archive will be built once with two jobs. Each program is bounded by timeout, at most 60 seconds.

Read CLAUDE.md, the f-slice7 report, SPEC 9.3.1/9.3.2 and N-D12, API F10-F14, and header contracts.
The review will use an independent termwise rational oracle, status and alias probes, named-place probes,
driver type checks, and an old-source differential check. Proof review is in progress.

Initial read-only inventory: no AGENTS.md matched rg --files (exit 1, zero matches).

Archive build: timeout 60 make -j2 BUILD=lanes/f-review5/build, exit 0. build.log records it.
Old source extraction: timeout 10 git show 27f7e5f:src/lfunc.c, exit 0; stored as old_lfunc.c.
The renamed old object and ASan/UBSan probe compiled with timeout 30 cc, both exit 0.
Only the probe translation unit is instrumented; the archive is the one ordinary build.

First local attack: timeout 60 python3 -B lanes/f-review5/attack.py local, exit 124.
Its C probe completed 2016 rows / 34274 checks / 0 failures. The Python Fraction oracle then timed out.
The oracle now uses its proved exact rational residue recurrence for operands above 256 bits.
This changes representation of intermediate terms, not output precision or the cutoff.
The direct Fraction path and modular path agreed on 21 separate 27-digit cases before the timeout.

Enumeration attack: timeout 60 python3 -B lanes/f-review5/attack.py enumerate, exit 0.
252 ball/request cases, 4286 C checks, 2088 point enclosures, 116 smallest-hull witnesses, 0 failures.
The grid has all admitted integer representatives modulo p^(c+2), p=2,3,5,7, M=c,c+1.

Rerun local: exit 0, 2016 cases, 34274 C checks, 21 oracle cross-checks, 0 failures.
Limits: exit 0, 2548 cases, 43318 C checks, 0 failures.
Large: exit 0, 32 cases at N=2000, 546 C checks, 0 failures.
Regression: exit 0, 1008 cases, 21170 C checks, 0 differences from 27f7e5f.
Each command was timeout 60 python3 -B lanes/f-review5/attack.py MODE; logs are MODE.log.

Real probe: 1264 rows, 5387 checks, 0 failures; 336 finite, 24 lost-finiteness, 180 precision-limit,
360 unsupported-complex and 360 absent-place cases, plus 4 exact-zero calls at the maximum precision.
Driver: 224 lines compared to local library output and 116 hostile commands, 0 mismatches.

Findings being documented:
MINOR: F13 step 1 says LONG_MIN returns LIMIT unless exact zero ignores it. Exact 2 at p=2 returns
DOMAIN and 2 Z_2 returns NOT_DETERMINED. The check order elsewhere is correct; the sentence lacks a qualifier.
MINOR: the acknowledged absent parity costs 3997/3998 steps instead of 1999 on x=3/2 at p=3, N=2000.
One cost_probe run gives original/paired milliseconds 76.225436/36.009152 (sin),
71.580113/34.905123 (cos), 68.015647/33.988612 (sinh), 86.436150/36.421342 (cosh).
All 4 residues agree. No repeated timing or general performance claim is made.
