"""Assemble the completed lane report from retained check records."""
import json
import re
from pathlib import Path

lane = Path('lanes/f-repair8')
review = {
    'B1': '4684 / 5', 'B2': '4841 / 7', 'B4': '614 / 1', 'B4b': 'SIGABRT(6)',
    'B5': 'SIGFPE(8)', 'B6': '3100 / 4', 'B7': '30 / 1', 'B8': '9036 / 6',
    'B9': '0 / 0', 'B9c': '0 / 0', 'B9d': '0 / 0', 'B10': '0 / 0',
    'B10b': '2 / 1', 'B11': '0 / 0', 'B12': '78 / 1', 'B13': '190 / 1',
    'O1': '0 / 0', 'O2': '72 / 1', 'O3': '0 / 0', 'O4': '1 / 1',
    'O5': '1 / 1', 'O6': '169 / 1', 'O8': '67 / 4', 'O9': '24 checks + SIGABRT(6)',
    'O10': '1 / 1', 'O11': '2 / 1', 'O14': '8 / 2', 'O16': '94996 / 2',
    'O17': '1 / 1', 'O18': '4 / 1', 'O19': '1 / 1', 'O22': '2 / 1',
    'O23': '0 / 0', 'O24': '3 / 1',
}


def records(phase):
    rows = [json.loads(s) for s in (lane / ('mutations-' + phase + '.jsonl')).read_text().splitlines()]
    assert len(rows) == 34
    assert {r['id'] for r in rows} == set(review)
    assert all(r['test_exit'] in (1, -6, -8) for r in rows)
    return {r['id']: r for r in rows}


def failures(record):
    if record['test_exit'] < 0:
        return 'SIGABRT(6)' if record['test_exit'] == -6 else 'SIGFPE(8)'
    match = re.search(r'(\d+) failed checks, (\d+) failed tests', record['test_summary'])
    assert match
    return ' / '.join(match.groups())


part1, final = records('part1'), records('final')
fault_table = '\n'.join('| ' + i + ' | ' + review[i] + ' | ' + failures(part1[i]) +
                        ' | ' + failures(final[i]) + ' |' for i in review)
end_rows = []
for name in ('plain', 'san', 'inv', 'clang'):
    log = (lane / ('end-' + name + '.log')).read_text()
    assert log.splitlines()[-1] == '16 tests, 2652899 checks, 0 failed checks, 0 failed tests'
    timing = (lane / ('end-' + name + '-time.log')).read_text().strip()
    wall = re.search(r'wall=(\S+)', timing).group(1)
    user = float(re.search(r'CPU_user=(\S+)', timing).group(1))
    system = float(re.search(r'CPU_system=(\S+)', timing).group(1))
    cost = re.search(r'CPU=(\S+) guard', log).group(1)
    end_rows.append(f'| {name} | 0 | 16 | 2652899 | 0 | {wall} | {user + system:.2f} | {cost} |')
end_table = '\n'.join(end_rows)
suite = json.loads((lane / 'check-all.json').read_text())
assert suite['exit'] == 0 and not (lane / 'build').exists()
drivers = json.loads((lane / 'drivers.json').read_text())
assert len(drivers) == 5 and all(r.get('differences', 0) == 0 for r in drivers)
driver_table = '\n'.join('| ' + Path(r['command'].split()[-1]).stem + ' | ' +
                         str(r['exit']) + ' | ' + str(r['expected_exit']) + ' | ' +
                         str(r['lines']) + ' | 0 |' for r in drivers[1:])
native = json.loads((lane / 'comparison-native.json').read_text())
injected = json.loads((lane / 'comparison-injected.json').read_text())
assert native['differences'] == injected['differences'] == 0
vectors = json.loads((lane / 'vectors-final.log').read_text())
assert vectors['oracle_disagreements'] == 0

report = f"""# f-repair8 report

Completed on 2026-10-05. The eight gaps F1-F8 are covered. The cost defect F9 is repaired.
Status, where and value have 0 differences in 26000 old/new requests. No specification was changed.
The final four test builds have 16 tests, 2652899 checks and 0 failures each.
Both mutation phases reject all 34 requested faults. The full suite exits 0. Build trees were removed.

## Files written

- tests/test_gfunc_log.c: oracle rows, three literal centres, status/state checks and the CPU guard.
- tests/ref/vectors/f-repair8/local.jsonl and README.md: new vectors and their provenance.
- src/gfunc_log.c: a private check of W on the known-refusal path.
- include/adelefeld/gfunc.h: only the comment block of Log_refine; no signature changed.
- docs/api-1f8.md: only G7-G12, from line 274 onward; the new proof is G11 steps 10-15.
- lanes/f-repair8/: mutate.py, write_vectors.py, differential.c, compare.py, drivers.py, suite.py,
  write_report.py, old_gfunc_log.c and part1_gfunc_log.c; check logs, JSON records and this report.

The two saved C sources are identical. old_gfunc_log.c was obtained with the read-only command
git show HEAD:src/gfunc_log.c. HEAD was 2ea60b1abe2657c3b1d4efaf28199fe244e2845d.
The patch definitions in the copied mutate.py retain the review's faults unchanged.
No state-changing git command, bd command, system installation or edit of an unowned file remains.
The original Makefile was read only. It already registers test_gfunc_log; vectors need no registration.

## F1-F8: rows and red assertions

write_vectors.py calls proto/idlog_checks.py for every expected ball and every image.
It independently compares the centre, exponent and exact tag with lanes/f-review13/il_oracle.py.
The latter uses inversion of an exponential table, with no log series.
All 369 distinct input image cases agree as sets between the two routes, and with predicted().
All 2358 rows agree. The file stores 63 complete H=5 images, enumerating 21963 units.
The C test checks 362805 memberships in those new complete images, in both directions.

The grid has p=2,3,5,7; k=1,2,3; M=p^k or 11*p^k; v_p(r)=-2,0,2; and
r'=1, p+1, 1/(p+1). Requested N are the distinct members of -2,0,1,2,3,E,E+1,6.
The first two units other than +-1 modulo p^k are selected when they exist.
There are 2088 non-+-1 rows. There is no such unit modulo 2, 3 or 4.
The remaining 270 rows keep those boundaries. At stored k=1 at 2, M=22 permits c=3.
Complete images use v_p(r)=0 and N=6; M=p^k, except M=22 at that boundary.

The initial vector run had 60 stored complete images. Three images at the stored k=1 boundary
were added before the final checks. Row count and expected ball values stayed the same.
The final generator also checks every one of the 369 distinct image cases before writing atomically.

Literal checks require 4 + O(2^3) for (1 ; 1 * [3 mod 8]), 6 + O(3^2) for
(1 ; 1 * [2 mod 9]), and the refined triple (24,36,1) at {{3}} with N=2.
The main-phase check keeps the review's input: (2 ; 1 * [2 mod 5^22369620]),
N=22369620, S={{2,5}}. It requires where=5. No faster replacement was used.

These are the first failed assertions recorded by the part-1 red runs:

| Fault / finding | Failed assertion | Required outcome |
|---|---|---|
| O1 / F1 | adf_lball_identical(l,e) | Oracle ball, including its nonzero centre |
| B9 / F2 | st==want | LIMIT (10), not DOMAIN (7) |
| O3 / F3 | st==want | LIMIT on both baseline-charge inputs |
| B10 / F4 | adf_adele_identical(y,before) && memcmp(raw,y,sizeof(raw))==0 | y retained |
| B11 / F5 | adf_place_equal(w,prime(97)) | Sentinel retained on OK at infinity |
| B9c / F6 | adf_place_equal(w,named?place:prime(97)) | where=2 |
| B9d / F7 | adf_place_equal(w,named?place:prime(97)) | where=infinity |
| O23 / F8 | adf_place_equal(w,named?place:prime(97)) | where=5 |

Both F3 inputs are literal resource requests: exact content 1, S={{3}}, N=33554431;
and S={{2}}, N=33554433. Their aggregate charge is 67108866, exceeding 67108864 by 2.
F4 uses a positive real part and content 3 at 2 with N=33554432.
F6 uses content 3 and S={{2,3,5}} at that N, so an aggregate excess already exists at 3.
F2 and F7 use excessive real precision together with n=-1 and n=65537.
All finite failure checks compare fields and struct bytes; NULL where is also exercised.

Exact assertion text, source line at the red run and counts are in mutations-part1.jsonl.
The unmutated part-1 run has 15 tests, 2652718 checks and 0 failures.
Its command is timeout 170 lanes/f-repair8/build/plain/test_gfunc_log.
The repaired code passes the final program with the additional cost guard and all 63 images.

## Fault table before and after

Entries are failed checks / failed tests, or the signal. Review numbers come from
docs/reviews/f1/review-gfunc-log-referee.md, the same record as lanes/f-review13/result.md.
The second column after the review uses the new part-1 tests and the saved unmutated source.
The last uses the repaired source and the final test program.

| ID | Review | New tests / old code | Repaired code |
|---|---|---|---|
{fault_table}

In each new phase: 34 mutants, 31 assertion failures, 2 SIGABRT and 1 SIGFPE.
There are 0 survivors, 0 mutant test timeouts and 0 compile failures.
All eight former survivors fail. All 26 previously detected faults remain detected.
B3, B5p and B12b are the review's three equivalent faults; they were not requested or rerun.
O9 prints 24 failed checks before its abort in both new phases.

## F9: status, proof and red-green cost

For x=(2 ; 2 * [1]), N=1048577 and real prec=64, the fifteen-prime list is
3,5,7,11,13,17,19,23,29,31,37,41,43,47,18446744073709551557.
Its status is LIMIT and where is the last prime, 2^64-59.
The same status and place apply to {{3,2^64-59}} and {{2^64-59}}.
Without that last prime, the fourteen-prime list exceeds the aggregate and leaves where untouched.
These outcomes are stated in G11 and remain exactly the same.

G11 steps 10-15 prove the replacement. An earlier prime that passed preflight has only
its working-power LIMIT left to decide. The descriptor settles the zero/no-power branches.
Otherwise W comes from J=min{{j>=1: j h-floor(log_p(j))>=K}} and T=J-1.
The maximum W for h=d usually proves success immediately. Near the bound, the direct route's
valuation below K equals v_p(a-b), or v_2(sa-b), for t=r'c=a/b.
An odd nonprincipal residue uses W(K,1) before its torsion power. The compact lift is unnecessary.
Thus the first earlier working LIMIT is found with comparisons and factor removals only.
No earlier series, compact power, torsion power or CRT is computed on this refusal path.

RED command:

~~~sh
timeout 5 lanes/f-repair8/build/plain/test_gfunc_log
~~~

The test first prints that the fifteen-prime call is starting. Old code does not return:
exit 124 after 5.00 s wall, 4.96 s user CPU and 0.04 s system CPU.
The assertion st==ADF_LIMIT && sec<1.0 is not reached. This is the required timeout red run,
not a claimed wrong status or value. cost-red.log and cost-red-time.log retain it.

GREEN: the repaired call returns LIMIT at 18446744073709551557 in 0.000370 s CPU.
The whole initial green run exits 0: 16 tests, 2652737 checks, 0 failures, 16.28 s wall.
The final four call timings are below; all satisfy the 1.0 s CPU guard and preserve y.
Admitted-request evaluation, aggregate-only refusal and the canonical tie rule are unchanged.

## Old/new comparison

compare.py builds old and new archives with the same other objects. It replaces gfunc_log.o
with the saved HEAD source or the repaired source. differential.c uses seed 0x8f1300261004.
It compares every status and where, every successful real-ball dump and canonical finite triple,
and preservation of a sentinel value on failures. Both wrappers are sampled.
There are small and large N, empty and shuffled lists, repeats, infinity, invalid lengths,
negative real inputs, aggregate refusals and earlier-working/later-known refusals.

| Route | Requests | Large N | OK | NOT_DETERMINED | DOMAIN | LIMIT | Differences |
|---|---|---|---|---|---|---|---|
| Native | 20000 | 6698 | 4951 | 0 | 7992 | 7057 | 0 |
| Injected real refusal | 6000 | 2023 | 1469 | 10 | 2380 | 2141 | 0 |

The native run has 2502 empty lists and 1672 negative lengths; the injected run has 751 and 460.
The injected scratch evaluator returns NOT_DETERMINED at prec=97 in both sources.
This is explicitly injected: no valid natural input for that real-evaluator failure is claimed.
These four statuses are every status allowed for the refinement on valid idele values.
Old/new wall times are 5.263865/0.104620 s for the native run and 6.278307/0.106621 s injected.
Every compiler, archive and executable command, exit code and time is in comparison-native.json
and comparison-injected.json. All 16 subprocess commands exit 0.

## Commands and checks

Commands below ran from the repository root unless a different cwd is stated.
The aliases only shorten the recorded command spellings:

~~~sh
lane=lanes/f-repair8
bld=$lane/build
timeout 170 python3 -B $lane/write_vectors.py
timeout 170 make -j2 BUILD=$bld/plain $bld/plain/test_gfunc_log
timeout 170 make -j1 BUILD=$bld/plain $bld/plain/test_gfunc_log
timeout 170 make -j1 BUILD=$bld/san SAN=1 $bld/san/test_gfunc_log
timeout 170 make -j1 BUILD=$bld/inv INV=1 $bld/inv/test_gfunc_log
timeout 170 make -j1 BUILD=$bld/clang CC=clang $bld/clang/test_gfunc_log
~~~

The generator ran three times, each exit 0: 2358 rows and 0 oracle disagreements.
Stored image counts were 60, 63, 63. The last also reports 369 independent image comparisons.
The -j2 plain build ran twice, exits 0, for part 1 and the old-code cost red.
Each -j1 build ran once and exited 0. Independent build/test workers never exceeded 2.

Mutation prep and batches, all with exit 0:

~~~sh
timeout 170 python3 -B $lane/mutate.py prep
timeout 170 python3 -B $lane/mutate.py B9 B9c B9d B10
timeout 170 python3 -B $lane/mutate.py B11 O1 O3 O23
timeout 170 python3 -B $lane/mutate.py B1 B2 B4 B4b
timeout 170 python3 -B $lane/mutate.py B5 B6 B7 B8
timeout 170 python3 -B $lane/mutate.py B10b B12 B13 O2
timeout 170 python3 -B $lane/mutate.py O4 O5 O6 O8
timeout 170 python3 -B $lane/mutate.py O9 O10 O11 O14
timeout 170 python3 -B $lane/mutate.py O16 O17 O18 O19
timeout 170 python3 -B $lane/mutate.py O22 O24
~~~

The same prep and nine batches ran again with IDLOG_PHASE=final and
IDLOG_MUTATION_SOURCE=src/gfunc_log.c before each command. Each mutant program is under timeout 60.
The harness also bounds its compile/link commands by timeout 170. Its actual test exits and counts
are the table above and the two JSONL records. The final sweep recompiles the final test program.

For each build, the final command is timeout 170 $bld/NAME/test_gfunc_log.
SAN uses ASAN_OPTIONS=detect_leaks=0. /usr/bin/time records wall, user and system CPU.

| Build | Exit | Tests | Checks | Failures | Wall s | CPU s | Guarded call CPU s |
|---|---|---|---|---|---|---|---|
{end_table}

Before the three added boundary images, the first SAN, INV and clang runs also exited 0:
16 tests, 2652737 checks and 0 failures each. Their wall times were 21.18, 29.84 and 36.37 s.
No AddressSanitizer error or UBSan runtime error occurs in the final SAN log.
All build, test and time logs are retained outside the removed build trees.

~~~sh
timeout 170 python3 -B $lane/compare.py
timeout 170 python3 -B $lane/compare.py injected
timeout 170 python3 -B $lane/drivers.py
~~~

Each script exits 0. The comparison counts are above. The driver script compiles adf.c against
the repaired plain archive under timeout 170, then runs each case under timeout 60:

| Case | Exit | Expected exit | Lines | Output differences |
|---|---|---|---|---|
{driver_table}

Whole suite, once:

~~~sh
ASAN_OPTIONS=detect_leaks=0 timeout 1700 python3 -B $lane/suite.py
~~~

The wrapper runs timeout 1700 make -j2 check-all in lanes/f-repair8/build/suite.
It copies the Makefiles and tools into owned lane storage and links read-only input directories.
Only the lane-local Makefile adds timeout 170 around each C program and each subsequent script.
This avoids writes outside the lane from the driver's fixed build paths.
The original Makefile and tool sources are unchanged.

Result: exit 0 in {suite['wall']:.6f} s wall; 79 C programs, 865 tests, 66888560 checks,
0 failed checks and 0 failed tests. Driver: 64 cases, 101179 lines, 0 differences.
Exports: 462 declared, 462 exported, 0 missing, 0 extra, 0 variadic.
Julia exits 0 with the suite's existing system-GMP LD_PRELOAD workaround.
Mutation-tool self-test: 84 OK checks, 0 FAIL checks, exit 0.
Memory-check self-test: 16 OK checks, 0 FAIL checks, exit 0; static scan of 115 files, 0 findings.
check-all.log, check-all.json and suite-counts.json retain the full record.

Final audits: git diff --check exits 0. The inline audit command is timeout 170 python3 -B -.
It checks 15 owned source/prose files: 0 lines over 116 characters and 0 missing final newlines.
JSONL data and command-output logs keep their record layout. The same inline Python command form
audits the mutation records: 34 distinct requested IDs in each phase and 0 survivors.
cmp lanes/f-repair8/old_gfunc_log.c lanes/f-repair8/part1_gfunc_log.c exits 0.
Cleanup also uses timeout 170 python3 -B - and leaves 0 build trees. final-audit.json retains the counts.

## Not done

LeakSanitizer cannot run in this sandbox. ASAN_OPTIONS=detect_leaks=0 was used for SAN and the
whole suite. This is not a library leak check; the orchestrator's suite supplies that check.
The review's three equivalent faults were not rerun. Driver and Julia were not run per mutant;
the brief requires the C mutation table and four driver cases against the repaired archive.
The NOT_DETERMINED comparison uses a labelled scratch injection, not a natural failing real input.
No other requested work remains.

## Sources pending

None new. Inherited from the design and functions.md: [source pending: conventional name and
normalisation of Iwasawa Log]; [source pending: name Teichmueller]; [source pending: real analytic
facts of functions.md Lemma 2]. The finite algorithm uses the defined series and repository proofs.
The new proof is stepwise in G11. Its FLINT factor-removal source was read on disk:
refs/src/flint-3.0.1/fmpz.rst:1142-1150. Powers and inverses were also read at :913-917 and :1154-1161.

## Findings against the specification

None. F9 changes cost only. The status, canonical place and successful enclosures are preserved.
"""
for line in report.splitlines():
    assert len(line) <= 116, (len(line), line)
assert report.endswith('\n')
(lane / 'report.md').write_text(report)
print('report.md written:', len(report.splitlines()), 'lines; build_trees_remaining=0')
