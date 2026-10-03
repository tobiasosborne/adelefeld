#!/usr/bin/env python3
from pathlib import Path
import json
import textwrap
import hashlib
import sys
P=Path('lanes/f-review8')
body=(P/'report_body.txt').read_text()
body=body.replace('lpow.h:55-56','lpow.h:57-58').replace('lpow.h:23-24','lpow.h:25-26')
body=body.replace('api-1f6.md:307 says','api-1f6.md:306 says')
checks=(P/'report_checks.txt').read_text()
fixtures=(P/'report_fixtures.txt').read_text()
cost=(P/'report_cost.txt').read_text().replace('Five final one-trial rows','Six final one-trial rows')
data=json.loads((P/'cost-data.json').read_text())
assert len(data)==36
assert all(d['total'][1]/d['components'][1]<=1.5 for d in data)
assert all(d['trials'] in (1,3) for d in data)
assert sum(d['trials'] for d in data)==94

commands='''
## Commands and results

All paths below are relative to the repository root. The archive was built once:
`timeout 600 make -j2 BUILD=lanes/f-review8/build lanes/f-review8/build/libadelefeld.a`.
Exit 0; build-archive.log has the complete compiler/archive output.

Mutation preparation: `timeout 180 python3 -B lanes/f-review8/mutations.py prepare`, exit 0,
3 test-object and 2 support-object compilations. Every nested compiler and linker is
under timeout 150. `timeout 180 python3 -B lanes/f-review8/mutations.py baseline`, exit 0:
9 tests/2760383 checks for lpow, 10/1308401 for lroot, 15/522217 for rfunc_prime;
all three have 0 failed checks. The archive is linked unchanged in every case.

Fault batch commands, each under timeout 180 with the same script:
`mutations.py 0 4`, `4 8`, `8 12`, `12 16`, `14 16`, and `7 8` (the final sign fault).
Batch exits are 0,0,0,1,0,0. The exit-1 batch ran the early-status and seed faults,
then stopped on a replacement-count assertion before compiling the centre fault.
fix_mutation_scope.py corrected that harness needle, after which 14..16 ran.
refine_sign_fault.py corrected the sign fault to the exact odd-parity image; its 7..8
rerun supplies the final table. All final faults compile/link. The table gives every
individual runner result; 54 actual executions include baseline and superseded runs.

`timeout 60 cc -std=gnu11 -O2 -g -Iinclude lanes/f-review8/h.c
lanes/f-review8/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review8/build/h`:
exit 0. h.c is copied from f-review7; no earlier attack script was rerun.
`timeout 180 python3 -B lanes/f-review8/proof_checks.py`: exit 0; 5760 group cases,
43200 point comparisons, 8640 tightness pairs, 89950 precision inequalities,
2112 unions, 9636 exponent-period comparisons, and 3 endpoint calls; 0 failures.
`timeout 30 python3 -B lanes/f-review8/proof_boundaries.py`: exit 0; 11296 inequalities, 0 failures.
`timeout 30 lanes/f-review8/build/h < lanes/f-review8/working_precision.in`: exit 0, 2 OK calls.
`timeout 30 lanes/f-review8/build/h < lanes/f-review8/spec_precision.in`: exit 0, 1 OK call.
`timeout 30 python3 -B lanes/f-review8/audit_fixtures.py`: exit 0; 6930/3024/4352 fixture
rows counted, 1260 degree-one rows, 4788 root-power images, 250 scaled root rows,
3 displayed integer certificates, and the 9-versus-27-residue SPEC example.

`timeout 60 cc -std=gnu11 -O2 -g -Iinclude lanes/f-review8/alpha.c
lanes/f-review8/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review8/build/alpha`:
exit 0. `timeout 30 lanes/f-review8/build/alpha`: exit 0, 11 checks, 0 failures.
`timeout 60 cc -std=gnu11 -O2 -g -Iinclude -Isrc lanes/f-review8/repro_r8.c
lanes/f-review8/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review8/build/repro_r8`:
exit 0. `timeout 60 lanes/f-review8/build/repro_r8`: exit 0, 15972 cases, 13485
nonsolvable, 0 failures. `timeout 180 python3 -B lanes/f-review8/r8_count_details.py`:
exit 0, same 15972 cases/0 failures, 5 digit products/6 powering calls/11 reference products.
Its nested compile and test each have timeout 60.

`timeout 180 python3 -B lanes/f-review8/reproduce_survivors.py`: exit 0, 3 original/mutant
pairs distinguished at p^2; nested compiles use timeout 60, executions timeout 30.
`timeout 30 python3 -B lanes/f-review8/survivor_minimal.py`: exit 0, 1 pair distinguished,
exact root witness 42. `timeout 180 python3 -B lanes/f-review8/early_probe_build.py`:
exit 0, 2 instrumented builds and 2 calls; identifier counts 0 and 1, both LIMIT.
Nested compiles use timeout 60, executions timeout 30.

Cost compilation: `timeout 60 cc -std=gnu11 -O2 -g -Iinclude lanes/f-review8/cost.c
lanes/f-review8/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review8/build/cost`, exit 0.
The command was repeated after adding bounded-run arguments and flushing completed phases;
no library code changed. cost_phase.c is compiled with the same flags and timeout 60.
`timeout 90 python3 -B lanes/f-review8/instrument_cost.py`: exit 0, one scratch timing-only
lpow source and one benchmark executable. Its nested compile has timeout 60.

Cost supervisors: `timeout 180 python3 -B lanes/f-review8/cost_run.py` (exit 124),
then ranges `12 16` (0), `16 17` (124), `18 24` (0), `24 28` (0), and `30 34` (0).
The last range is pinned with `taskset -c 1`. Their case executables all use timeout 150.
The first range's 65537 ball row retains only its completed first trial.
The P64 exact total's partial phase is recorded but excluded from the final table.

Each of these phase calls uses `timeout 150 lanes/f-review8/build/cost_phase`:
`powrat P64 10000 0 components 0` and `powrat P64 10000 1 total 1` (both exit 124);
an isolated `powrat P64 10000 0 components 0` (exit 0, 96.725110058 s, valid=1).
P64 is expanded to its decimal value in the actual commands and log names.

Each final instrumented call uses timeout 150, executable build/cost_instrument, arguments
`FUNCTION PRIME 10000 BALLFLAG total CPU`. Six calls all exit 0, valid=1:
powrat P64 exact and ball; powunit 65537 exact and ball; powunit P64 exact and ball.
The table contains their total/component timings; each has one complete trial.
No timed executable exceeds its 150-second cap. Output comparisons are outside timing.

`timeout 30 python3 -B lanes/f-review8/tables.py`: exit 0; 16 final faults, 54 actual test runs,
0 test timeouts; 36 complete cost rows, 94 final timing trials, 0 median ratios over 1.5.
`timeout 30 python3 -B lanes/f-review8/cold_ratios.py`: exit 0; 8 initial-state ratios over 1.5.
`timeout 10 lscpu` and `timeout 10 uname -a`: exit 0; hardware/version metadata quoted above.
The source hashes in source-sha256.txt match the files used by the scratch copies.
`timeout 30 python3 -B lanes/f-review8/final_checks.py preflight` and the same command
without preflight validate 36 cost rows, 16 fault rows, 94 final trials, 4 source hashes,
report structure/line lengths and exact survivor/precision residue certificates.
The preflight result is 0 failures. The final check also validates the file manifest.

Setup failures retained in progress.md: the apply_patch API returned empty results without
creating two files; the first prepare attempt therefore exited 2, running no tests.
The files were then written and checked on disk. The mutation replacement assertion above
was also a harness failure, not a survivor. No state-changing git, bd, installation or whole
repository suite command was run. Core dumps were disabled (ulimit -c=0).

## Files written

All outputs are under lanes/f-review8/. files-manifest.txt lists their exact paths.
The main text artifacts are progress.md, report.md, the staging prose, fault-table.md,
cost-table.md, cost-data.json, source-sha256.txt and the per-run logs.
Programs: h.c; proof_checks.py; proof_boundaries.py; alpha.c; repro_r8.c;
repro_r8_counted.c; r8_count_details.py; audit_fixtures.py; mutations.py;
reproduce_survivors.py; survivor_minimal.py; early_probe.c; early_probe_build.py;
cost.c; cost_phase.c; cost_instrument.c; instrument_cost.py; cost_run.py;
tables.py; cold_ratios.py; assemble_report.py; final_checks.py.
Small setup scripts are also listed in the manifest. Their commands were
`timeout 30 python3 -B lanes/f-review8/NAME.py`, with NAME=add_root_fault,
fix_mutation_scope, finish_cost_setup, cost_flush, refine_sign_fault, prose_corrections,
final_setup; every command exited 0. They alter only this lane's scratch harness or prose. build/ holds the one archive,
objects, executables, timing-only source copies, all 16 fault sources and their logs.
The lane's pre-existing brief.md and automatic lane/stdout logs were not edited by this work.

## Not done

No product source, public header, specification, proof file, assertion or fixture was changed.
No repair is proposed as an unreviewed patch. No new broad value/status/branch-list campaign,
full repository suite, driver campaign, Julia execution, long fuzz run or sanitizer archive
was run. Numerical proof grids are finite; the written derivations cover the general claims.
The full branch-count maximum was not measured. Single-trial rows have no timing-spread estimate.
The eight raw cold ratios have not been assigned to a reviewed-code line because initial
states differ; no repeat cold-start campaign was performed to diagnose them.

## Sources pending

[source pending: FLINT 3.0.1 n_powmod2_ui_preinv implementation for exact internal product counts]
Only its documented operation is used: refs/src/flint-3.0.1/ulong_extras.rst:537-541.
The primitive-root guarantee is read at :1410-1413, unrestricted modular products at :368-373.
No undisclosed FLINT algorithm or timing floor is quoted from memory. Existing naming sources
for Teichmueller representatives and Iwasawa Log remain pending in functions.md:101 and :343.
The proof review uses their definitions and written algebra, not an external naming convention.
'''

# Reflow prose only. Keep table rows and headings intact, and enforce the lane's line limit.
def reflow(s):
    blocks=s.strip().split('\n\n')
    out=[]
    for b in blocks:
        lines=b.splitlines()
        if all(x.startswith('|') for x in lines) or b.startswith('#'):
            out.append(b)
        elif b.startswith('- '):
            out.append(b)
        else:
            out.append(textwrap.fill(' '.join(x.strip() for x in lines),width=116,
                                     break_long_words=False,break_on_hyphens=False))
    return '\n\n'.join(out)+'\n'
text=reflow(body)+'\n'+reflow(checks)+'\n'+(P/'fault-table.md').read_text()+'\n'
text+=reflow(fixtures)+'\n'+reflow(cost)+'\n'+(P/'cost-table.md').read_text()+'\n'+reflow(commands)
long=[(i,len(s)) for i,s in enumerate(text.splitlines(),1) if len(s)>116]
assert not long,long
assert not (P/'report.md').exists(), 'report must be written once, at completion'
if len(sys.argv)>1:
    (P/'report-preview.md').write_text(text)
    print('preview_lines',len(text.splitlines()),'max_line',max(map(len,text.splitlines())))
    sys.exit(0)
(P/'report.md').write_text(text)
files=[]
for path in sorted(P.rglob('*')):
    if path.is_file() and path.name not in ('brief.md','lane.log','stdout.log'):
        files.append(str(path))
# The final check log is created immediately after assembly.
files.append(str(P/'final-checks.log'))
(P/'files-manifest.txt').write_text('\n'.join(sorted(set(files+[str(P/'files-manifest.txt')])) )+'\n')
print('report_lines',len(text.splitlines()),'max_line',max(map(len,text.splitlines())),
      'cost_rows',len(data),'final_trials',sum(d['trials'] for d in data),'manifest_files',len(files))
