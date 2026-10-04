"""Audit retained evidence, remove owned scratch products, then write report.md once."""
from pathlib import Path
from fractions import Fraction
import hashlib
import json
import shutil
import textwrap
from oracle import random_cases

lane = Path(__file__).resolve().parent
root = lane.parent.parent
assert not (lane / 'report.md').exists(), 'report must be written once'
digest = hashlib.sha256((root / 'src/localfactor.c').read_bytes()).hexdigest()
assert (lane / 'source.sha256').read_text().split()[0] == digest, 'source changed during review'
results = []
for path in sorted((lane / 'runs').glob('*.json')):
    if '.partial.' in path.name:
        continue
    r = json.loads(path.read_text())
    assert r['failures'] == 0 and not r['findings'], path
    results.append((path, r))
assert len(results) == 36, len(results)


def totals(kind, p=None, exclude_san=True):
    chosen = [r for path, r in results if r['kind'] == kind and (p is None or r['p'] == p)
              and not (exclude_san and path.name.startswith('san-'))]
    return {k: sum(r[k] for r in chosen) for k in
            ['cases', 'ok', 'nd', 'domain', 'limit', 'samples', 'derivative_samples', 'failures']}


random = {p: totals('random', p) for p in [2, 3, 5, 7, 65537, 18446744073709551557, 0]}
distinct = {}
for p in random:
    entries = [r for path, r in results if r['kind'] == 'random' and r['p'] == p]
    boxes = [tuple(tuple(x) for x in c['s']) for r in entries
             for c in random_cases(p, r['start'], r['count_requested'])]
    distinct[p] = len(set(boxes))
    assert distinct[p] == 5000, (p, distinct[p])

driver = json.loads((lane / 'driver.json').read_text())
dense = json.loads((lane / 'dense-summary.json').read_text())
recurrence = json.loads((lane / 'recurrence-summary.json').read_text())
assert not driver['failures'] and dense['failures'] == recurrence['failures'] == 0
faults = [json.loads(p.read_text()) for p in sorted((lane / 'mutants').glob('*.json'))
          if '-witness' not in p.name]
assert len(faults) == 13
assert sum(f.get('test_exit') == 0 for f in faults) == 5
cost = [r for p in sorted(lane.glob('cost-*.json')) for r in json.loads(p.read_text())]
assert len(cost) == 36 and all(r['exit'] == 0 for r in cost)
assert json.loads((lane / 'san-statuses-off.log').read_text())['failures'] == 0
assert 'Sanitizer' not in (lane / 'san-cost-off.log').read_text()
assert 'failures=0 bits=4000' in (lane / 'pole-geometry.log').read_text()
assert '13 failures=0' in (lane / 'handles.log').read_text()
assert '36725 checks, 0 failed checks' in (lane / 'baseline.log').read_text()

lines = []


def para(s):
    lines.extend(textwrap.wrap(s, 112, break_long_words=False, break_on_hyphens=False))
    lines.append('')


def section(s):
    lines.extend(['## ' + s, ''])


def code(s):
    lines.extend(['```text', *s.splitlines(), '```', ''])


lines.extend(['# Lane f-review15: adversarial local zeta review', ''])
para('A dyadic pair (man, exp) means man*2^exp. Inputs below give real midpoint, imaginary midpoint, '
     'real radius, and imaginary radius separately. All rectangles are closed. The tested source SHA256 is '
     + digest + '. The installed C FLINT headers identify version 3.0.1.')

section('Findings')
lines.extend(['### F1. MAJOR: half of B passes the test but cuts off a true value', ''])
para('Fault: append mag_mul_2exp_si(B, B, -1) to the last multiplication in real_derivative_bound. '
     'test_localfactor returns exit 0: 10 tests, 36725 checks, 0 failed. This is a coverage defect. '
     'The injected implementation would violate the BLOCKER enclosure rule.')
para('Smallest input found in the stated search family: place real; prec=16; midpoint '
     '(-1,-4) + i(0,0); radii (1,-8), (0,0). The interval is [-17/256,-15/256]. '
     'The mutant returns OK with real midpoint (-34607,-10), radius (68570759,-25), '
     'and imaginary midpoint and radius zero. The unchanged function returns OK with real midpoint '
     '(-17373,-9), radius (301185087,-27), and contains the endpoint samples.')
para('At the right endpoint s=-15/256, the independent 600-digit value is '
     '-35.9251733059044577950542572008038016459547224312512608049836776537. '
     'It lies below the mutant lower endpoint by 0.0857070023174979561870697008038. '
     'The numerical margin is less than 10^-520 times the component scale. The search tried 364 candidates '
     'in order: midpoint -2^-e for e=1..5; radius 2^-r for r=e+1..e+14; precision '
     '2,4,8,16,32,64,128,256. This is a minimum within that ordering, not a global minimum.')
para('The separate prec=256 witness, s=-2^-4 +/- 2^-14, misses 9 of 20 queried samples. '
     'Its mutated bound is (120928953,-18); a sampled |L_inf\'| exceeds it by a factor '
     '1.1093715842849024110299305002. The unchanged bound is (120928953,-17). '
     'The source of the value formula is refs/src/tate-poonen/notes.txt:1014-1016. '
     'The derivative computation uses psi=Gamma\'/Gamma from refs/src/flint-3.0.1/acb.rst:916-918.')
para('Reason, step by step: the right endpoint belongs to the input interval; the returned dyadic '
     'interval has a computable exact lower endpoint; the independent Gamma and power evaluation is '
     'smaller than that lower endpoint by the displayed margin; therefore the returned rectangle misses '
     'an input image value. Details: minimized.json and mutants/half-B-witness.json.')
code("printf '0 16 -1 -4 0 0 1 -8 0 0\\n' | timeout 5 lanes/f-review15/build/bridge-half-B")

lines.extend(['### F2. MAJOR: lower Eplus passes the test but accepts a pole ball', ''])
para('Fault: replace the final E computation with scalar arb evaluation of the intended expression, '
     'then use arb_get_mag_lower instead of an upper bound. test_localfactor returns exit 0: '
     '10 tests, 36725 checks, 0 failed. The injected implementation would violate the BLOCKER pole rule.')
para('Input: p=2; prec=2; midpoint (1,0) + i(0,0); radii (1,0), (0,0). '
     'The mutant returns OK with real midpoint (1,1), radius (916259693,33); '
     'imaginary midpoint (0,0), radius (610839795,32). The unchanged function returns NOT_DETERMINED, '
     'leaves y untouched, and writes where=2. The same distinguishing input also works at prec=128.')
para('Reason, step by step: the input is [0,2] on the real axis; zero is an endpoint; '
     'at zero the finite factor denominator is 1-exp(0)=0; this is the simple pole of '
     'refs/src/tate-poonen/notes.txt:1733 continued by the factor formula. '
     'Here the intended E is exp(-log 2)(exp(log 2)-1)=1/2. A lower rounded E need not reach '
     'the denominator zero. OK is forbidden independently of the size of the finite output. '
     'Details: minimized.json and mutants/lower-E-witness.json.')
code("printf '2 2 1 0 0 0 1 0 0 0\\n' | timeout 5 lanes/f-review15/build/bridge-lower-E")

lines.extend(['### F3. MAJOR: an open integer test passes but changes the pole status', ''])
para('Fault: replace arf_cmp(h, lo) >= 0 by > 0 in the real pole test. '
     'test_localfactor returns exit 0: 10 tests, 36725 checks, 0 failed.')
para('Input: place real; prec=2; midpoint (-255,-1) + i(0,0); radii (1,-1), (0,0). '
     'The input is [-128,-127]. The unchanged function returns NOT_DETERMINED; the mutant returns LIMIT. '
     'Both leave the exact y representation untouched and write where=real. '
     'The original larger witness [-200,-199] at prec=64 has the same difference.')
para('Reason, step by step: S/2=[-64,-63.5]; floor(hi)=-64 equals lo; the closed test must accept '
     'that integer. Gamma has its pole at -64, hence L_inf has a pole at -128 '
     '(refs/src/tate-poonen/notes.txt:62-64,1014-1016). The open test misses the endpoint; '
     'direct Gamma is not finite; shift 65 exceeds the fallback cap and gives LIMIT. '
     'The contract requires the pole test to give NOT_DETERMINED first. Details: minimized.json.')
code("printf '0 2 -255 -1 0 0 1 -1 0 0\\n' | timeout 5 lanes/f-review15/build/bridge-open-integer")

lines.extend(['### F4 and F5. MINOR: two further bound mutations remain unclassified', ''])
para('Omitting +log(pi) in B and omitting the multiplication by M0 both pass all 36725 checks. '
     'Their bounds differ from the declared Z6 computation. Eighteen additional thin boxes per fault '
     'were checked at 600 digits, with 20 samples and derivative comparisons per box. '
     'No wrong status, image enclosure, or sampled derivative bound was found for these two faults. '
     'Their semantic equivalence or global soundness is unresolved; they are not claimed as MAJOR '
     'enclosure defects. This is the same unchecked quantity identified by the previous lane report.')
para('Exact distinguishing trace inputs and bound values are in witnesses.json. '
     'The first input in that family is real s=-2^-2 +/- 2^-12, imaginary part exactly zero, prec=256. '
     'Both faults and the unchanged function return OK there. The direct trace distinguishes the '
     'computed B; it does not prove a violated upper bound. Reproducer: witnesses.py after mutations.py.')
para('On that input the unchanged B is (400306253,-22); the bound without log(pi) is '
     '(45178009,-19); the bound without M0 is (557331365,-23). Both faulty versions return the '
     'same finite y as the original: real midpoint (-22649151191588155,-51), radius '
     '(839531535,-36), imaginary part exactly zero. Their sampled derivative ratios are '
     '0.3538989555 and 0.4590001913 respectively. No sample distinguishes a wrong value here.')

section('Fault table')
lines.extend(['| Fault | Test exit | Failed checks | Detection |',
              '|---|---:|---:|---|'])
order = ['half-B', 'no-enlargement', 'lower-E', 'open-integer', 'fallback-65', 'inward-round',
         'wrong-sign', 'lowprec-log-no-radius', 'max-radius', 'no-logpi-B', 'no-M0-B', 'drop-pi',
         'wrong-Gamma-scale']
counts = {'half-B':'0','no-enlargement':'252','lower-E':'0','open-integer':'0','fallback-65':'3',
          'inward-round':'82','wrong-sign':'not completed','lowprec-log-no-radius':'not completed',
          'max-radius':'94','no-logpi-B':'0','no-M0-B':'0','drop-pi':'518','wrong-Gamma-scale':'617'}
for name in order:
    f = next(f for f in faults if f['fault'] == name)
    desc = 'survives' if f['test_exit'] == 0 else 'assertions' if f['test_exit'] == 1 else 'assertions, SIGSEGV'
    lines.append(f"| {name} | {f['test_exit']} | {counts[name]} | {desc} |")
lines.append('')
para('All 13 faults compiled. Eight were detected and five survived. The two crashing runs emitted '
     'failed assertions before SIGSEGV; their complete failed-check totals are unavailable. '
     'The inward rounding fault is killed by p=2, s=2, prec=2. The n=65 fault is killed by the '
     'existing recurrence LIMIT check. The wrong-sign and low-precision-log faults are killed by '
     'the prime pole lattice. Full first assertions and commands are retained in mutations.log '
     'and mutants/*.log. No test was weakened or edited.')

section('Independent checks and their failure conditions')
para('The primary value oracle is mpmath 1.3.0, at 480 or more decimal digits; the dense special '
     'check uses 420 digits and the fault witnesses use 600. It uses mpmath.gamma and mpmath.power, '
     'not proto/zeta_checks.py, acb_gamma, or acb_pow as a reference. The finite integer checks '
     'use exact Fraction arithmetic. The repository test itself retains its own existing FLINT references.')
para('The stated numerical margin is 10^(-dps+80) times a component scale: the maximum of the '
     'component magnitude, |L|*10^(-dps+80), and 10^-dps. A failed containment check is evaluated '
     'again with 100 extra digits. This is a numerical margin, not an independently certified '
     'mpmath error guarantee. The 4000-bit scalar pole certificates and exact rational checks '
     'are separate from these approximate value comparisons. The scalar enclosure contract is '
     'refs/src/flint-3.0.1/arb.rst:6-12.')
para('Cases are bridge records. Exact-value, pole, status, and recurrence records make three '
     'public calls: separate y, y=s, and where=NULL. Non-OK representation checks compare '
     'arb_dump_str for each component. Every OK must be finite; where must stay unchanged. '
     'Every failure must preserve y and set where=v.')
lines.extend(['| Place | Random records | Distinct boxes | OK | ND | LIMIT | Value samples |',
              '|---|---:|---:|---:|---:|---:|---:|'])
for p, r in random.items():
    label = 'real' if p == 0 else str(p)
    lines.append(f"| {label} | {r['cases']} | {distinct[p]} | {r['ok']} | {r['nd']} | {r['limit']} | {r['samples']} |")
lines.append('')
para('Every random record checks 20 queried points on OK: corners, edge midpoints, the centre, '
     'and dyadic interior points. Degenerate rectangles necessarily repeat some points. '
     'Midpoint exponents range from -200 to 200; both signs occur; effective radii range from '
     'zero to less than one; requested precisions include 2 and 4096. A case fails for an '
     'unexpected status where one is prescribed, an OK pole ball, a non-finite output, a '
     'changed failure representation, an incorrect where, or a sampled value outside the stated margin.')
for kind, title in [('exact','Finite integer values'), ('finite-poles','Prime boundary boxes'),
                    ('real-poles','Real pole boxes'), ('near','Near-pole boxes')]:
    r = totals(kind)
    para(f"{title}: {r['cases']} records; {r['ok']} OK, {r['nd']} NOT_DETERMINED, "
         f"{r['domain']} DOMAIN, {r['limit']} LIMIT; {r['samples']} value samples; 0 failures.")
para('The 288 finite integer cases check p^s/(p^s-1) exactly for s in '
     '-257,-65,-16,-3,-2,-1,1,2,3,16,65,257 at all six primes and precisions 2,64,512,4096.')
para('Prime boundary cases use k=0,+/-1,2,999999,+/-1000000,2^100,2^1000 and 23 seeded indices '
     'per prime. Midpoints have 2200 bits and are offset by five ulps. Radii are two 30-bit '
     'radius units above or below the distance. Thin segments, real intervals reaching zero, '
     'and enormous-radius boxes are included. Separate 4000-bit scalar arithmetic certifies '
     '594 memberships and 384 exclusions, with 0 failures.')
para('Real poles use 170 indices, including 0,1,2,63,64,65,100,999999,1000000,2^200. '
     'Cases include exact poles, both thin directions, closed endpoint contacts, intervals '
     'between poles with width almost two, imaginary intervals one radius ulp away from zero, '
     'and -2n+2^-5000. Membership is checked with exact Fraction arithmetic. '
     'The near-pole sweep uses radii d/2,d/16,d/1000 and 200 real plus 300 records per prime.')
para(f"Recurrence: {recurrence['cases']} records, {recurrence['fallback_calls']} records with a fallback, "
     f"{recurrence['ok']} OK, {recurrence['samples']} samples, 0 failures. The shift ranges from 1 to 65; "
     'precisions are 16,64,256, with selected 4096-bit cases. n<=64 must not produce LIMIT; '
     'the n=65 boxes must produce LIMIT after pole exclusion.')
para('Derivative checks: 1200 samples on 60 random real boxes; 2430 samples on 30 dense random '
     'boxes; 1782 samples on 22 dense special OK boxes. Total 5412 completed comparisons, '
     '0 instances of |L_inf\'|>B. The special family includes poles 0,-2,-16,-62,-124, '
     'distances 2^-4,2^-20,2^-100, thin boxes, and wide left and positive boxes. '
     'The grid is 9 by 9. The largest completed sampled ratio is '
     '0.6666666505237424308893861841. A sampled derivative above B would be a finding even '
     'if the final output contained all value samples. Sampling gives a lower bound on the '
     'supremum, not its exact value.')
para('Derivative calculation, step by step: differentiate exp(-s log(pi)/2) to obtain '
     '-log(pi)/2 times that prefactor; differentiate Gamma(s/2) to obtain Gamma\'(s/2)/2; '
     'apply the product rule; substitute psi=Gamma\'/Gamma. This gives '
     'L_inf\'=L_inf*(psi(s/2)-log(pi))/2. The bound trace stores B before multiplication by R.')
para('Statuses: 48 records, 144 public calls, 0 failures. Non-finite midpoint parts cover NaN, '
     '+infinity and -infinity; either radius can be infinite. mag radii have no NaN state. '
     'The precision cap wins first, including LONG_MAX and non-finite inputs; LONG_MIN and '
     'precisions below two are clamped; exact -200 is DOMAIN; a ball around -200 is ND. '
     'All four documented statuses occurred. The handle program adds 13 checks, 0 failures. '
     'Composites are refused by the constructor without modifying it; a forged handle '
     'violates the precondition and aborts under ADF_CHECK_INVARIANTS; cap+1 still wins first.')
para('Cost: 36 matrix calls and one initial enormous-exponent pilot, 0 timeouts. At real s=1, '
     'times at 64,4096,65536,1048576 bits are 0.000141,0.000776,0.028712,1.294944 s. '
     'At p=2 they are 0.000090,0.000472,0.021146,0.876199 s. The matrix includes '
     'Re(s)=+/-2^1000 with Im(s)=1, Im(s)=+/-2^1000 with Re(s)=1, and exponents '
     '+/-2^60 in both components. All return within the 60 s threshold and preserve '
     'failure outputs. Full inputs, statuses and timings are in cost-*.json.')
para('Memory: bridge.c and cost.c compiled with -fsanitize=address,undefined and '
     '-fno-sanitize-recover=undefined. localfactor.c and place.c are instrumented; the system '
     'FLINT library and unrelated archive objects are not. With leak detection off, the '
     'bridge completes 48 status, 978 prime-pole and 1530 real-pole records, with 1300 value '
     'samples and 0 failures; cost completes three calls. No address or undefined-behaviour '
     'report is emitted. Leak-enabled exits fail because LeakSanitizer cannot operate under '
     'the sandbox ptrace environment. Leaks were not tested.')
para('Driver: 30 hostile command lines, 30 expected lines, 0 differences; driver exit 1 '
     'for the file containing errors. Ten printed lines are compared with a separate direct '
     'library caller. Other kinds of operand, real adeles, invalid places, 4, 2^64, missing '
     'operands, syntax precedence, and radii in only one component are included.')

section('Commands and results')
para('All commands ran from the repository root. The only archive build used the command '
     'below; test target construction did not rebuild the archive. No whole repository '
     'suite, git command, or bd command was run.')
code('timeout 600 make -j2 BUILD=lanes/f-review15/build lanes/f-review15/build/libadelefeld.a\n'
     'timeout 10 python3 lanes/f-review15/setup.py\n'
     'timeout 60 make -j2 BUILD=lanes/f-review15/build lanes/f-review15/build/test_localfactor\n'
     'timeout 30 lanes/f-review15/build/test_localfactor')
para('Results: archive exit 0; setup exit 0, two trace insertion sites; test build exit 0; '
     'baseline exit 0, 10 tests, 36725 checks, 0 failed. Logs: build.log, test-build.log, baseline.log.')
para('Manual C compilation used timeout 30 cc -std=c11 -O2 -g -Iinclude. All nine executables '
     'compiled with exit 0: bridge, bridge-base, cost, adf, driver-library, handles, bridge-san, '
     'cost-san, pole-geometry. bridge adds -Isrc -DREVIEW_TRACE; handles adds -Isrc '
     '-DADF_CHECK_INVARIANTS and src/localfactor.c src/place.c. Both sanitizer builds use '
     '-O1 -Isrc -fsanitize=address,undefined -fno-omit-frame-pointer '
     '-fno-sanitize-recover=undefined; bridge-san adds src/place.c, cost-san adds '
     'src/localfactor.c src/place.c. Each links the lane archive and -lflint -lgmp -lm. '
     'adf uses tools/adf/adf.c; bridge-base and bridge-san use bridge.c; cost-san uses cost.c; '
     'driver-library uses driver_library.c; pole-geometry uses pole_geometry.c. Other names '
     'match their lane C files. The plain '
     'bridge-base links the archive without the trace include; it was compiled but not run.')
para('Completed oracle commands follow. Each listed JSON retains status counts, samples, '
     'seconds, and the bridge exit result. All listed commands exit 0. Near and random '
     'records test the failure conditions above; their counts are not added to the baseline '
     'repository check count.')
for path, r in results:
    cmd = 'timeout 170 python3 lanes/f-review15/oracle.py ' + r['kind']
    if r['kind'] in ['random','near']:
        cmd += f" --p {r['p']} --start {r['start']} --count {r['count_requested']}"
    elif r['kind'] == 'bounds':
        cmd += f" --start {r['start']} --count {r['count_requested']} --tag {path.stem}"
    if path.name.startswith('san-'):
        cmd = 'ASAN_OPTIONS=detect_leaks=0 ' + cmd + ' --exe bridge-san --tag ' + path.stem
    if r['kind'] == 'random' and r['p'] == 0 and r['start'] == 0:
        cmd += ' --derivatives'
    # Wrap long commands at argument boundaries without changing their shell meaning.
    if len(cmd) > 112:
        split = cmd.rfind(' ', 0, 103)
        code(cmd[:split] + ' \\\n  ' + cmd[split+1:])
    else:
        code(cmd)
    para(f"{path.name}: {r['cases']} records, {r['samples']} value samples, "
         f"{r['derivative_samples']} derivative samples, {r['seconds']} s, exit 0.")
code('timeout 30 python3 lanes/f-review15/statuses.py\n'
     'timeout 170 python3 lanes/f-review15/dense.py\n'
     'timeout 60 python3 lanes/f-review15/pole_geometry.py\n'
     'timeout 170 python3 lanes/f-review15/recurrence.py\n'
     'timeout 60 python3 lanes/f-review15/driver_checks.py\n'
     'ulimit -c 0\n'
     'timeout 30 lanes/f-review15/build/handles\n'
     'timeout 170 python3 lanes/f-review15/mutations.py\n'
     'timeout 170 python3 lanes/f-review15/witnesses.py\n'
     'timeout 170 python3 lanes/f-review15/minimize.py')
para('Results, in order: 48/0 failures; 24 cases and 1782/0 sample failures; '
     '978/0 membership failures; 201 cases and 140/0 sample failures; 30/0 differences; '
     '13/0 handle failures; 13 compiled faults, eight detected and five surviving; '
     '42 targeted witness records with three distinguished faults; 366 minimisation '
     'candidate inputs with three distinguished faults. The witness command ran twice: the second '
     'run fixes the placement of the half-B trace and reproduces the same enclosure failure.')
code('timeout 170 python3 lanes/f-review15/cost_probes.py precision --place 0\n'
     'timeout 170 python3 lanes/f-review15/cost_probes.py precision --place 2\n'
     'timeout 170 python3 lanes/f-review15/cost_probes.py huge\n'
     'timeout 170 python3 lanes/f-review15/cost_probes.py tiny\n'
     'timeout 170 python3 lanes/f-review15/cost_probes.py huge-exponent\n'
     'timeout 60 lanes/f-review15/build/cost 0 64 real-imag 1152921504606846976 1')
para('Results: 4,4,8,12,8 and 1 calls respectively; all exit 0; 0 timeouts. Each matrix child '
     'uses timeout 60. Detailed commands are in cost-*.json.')
code('ASAN_OPTIONS=detect_leaks=0 REVIEW_BRIDGE=bridge-san \\\n  timeout 30 python3 lanes/f-review15/statuses.py\n'
     'ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/f-review15/build/cost-san 2 4096 real 0 1\n'
     'ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/f-review15/build/cost-san 0 4096 real 0 1\n'
     'ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/f-review15/build/cost-san \\\n  0 64 real-imag 1152921504606846976 -1')
para('Results: 48/0 status failures; three cost exits 0; 0 ASan or UBSan reports. '
     'The same four commands with detect_leaks=1 were attempted first: all exit 1 at '
     'LeakSanitizer shutdown because ptrace is unavailable. The first status run reports '
     '48 records and 0 output errors before the shutdown failure. Logs retain both attempts.')

section('Incomplete or corrected checks')
para('The first finite-poles oracle run exited 1 with six false findings. Its zero-pole '
     'generator left exponent zero after moving the mantissa, so it tested Im(s)=5 and '
     'labelled it as zero. The corrected run has 978 records and 0 findings. '
     'The first scalar membership encoding exited 1 with 943 parsed records and 581 '
     'failures because a negative k was split out of a hyphenated ID incorrectly. '
     'An explicit pole_index field fixes the encoder; the rerun certifies all 978 records.')
para('The initial timeout 170 python3 lanes/f-review15/oracle.py bounds --count 50 run '
     'stopped with BrokenPipeError after the child bridge reached 165 s. Its last checkpoint '
     'records 30 completed boxes, 2430 value samples, 2430 derivative samples and 0 '
     'failures at 159.478 s. It was rerun as three completed ten-box batches, recorded '
     'above. No library call was found to take 60 s.')
para('The three 10 s import/count probes exit 0: mpmath version 1.3.0; the first real random '
     'audit counts 5000 records and 4889 encoded boxes; after 111 new records it counts '
     '5111 records and 5000 encoded boxes. finalize.py independently recomputes 5000 '
     'distinct boxes at every place and checks the unchanged source hash.')
code('timeout 10 python3 -m py_compile lanes/f-review15/finalize.py\n'
     'timeout 10 python3 lanes/f-review15/finalize.py')
para('The syntax check exits 0. A separate 10 s Python evidence audit counts 36 completed '
     'oracle JSON files, 0 completed failures, 0 progress lines over 116 characters, and '
     '0 executable files outside build. finalize.py repeats the evidence checks, validates '
     'the report line lengths, removes only owned scratch products, and writes this report once. '
     'Audit results are retained in audit.json.')
para('For a short interval three oracle subprocesses ran concurrently. That violated the '
     'two-core scheduling instruction. It is recorded in progress.md. Later scheduling '
     'kept at most two workers; oracle.py also confines later direct workers to the same '
     'two allowed CPUs. Every test program or script had a timeout of at most 170 s; '
     'the one archive build used the explicitly prescribed 600 s exception.')

section('Files written and reproduction')
para('Read CLAUDE.md, SPEC 9.3.7, PLAN 1F.9, the header contract, src/localfactor.c, Y16-Y17, '
     'design Z1-Z9, the conventions pole rule and status row, the previous lane report, '
     'the localfactor C and Julia tests, and the driver command and documentation. '
     'Implemented only review programs and fault generators; no file outside this lane was changed.')
para('Only lanes/f-review15 was written. Source and workflow files: bridge.c, cost.c, '
     'cost_probes.py, dense.py, driver_library.c, driver_checks.py, handles.c, minimize.py, '
     'mutations.py, oracle.py, pole_geometry.c, pole_geometry.py, recurrence.py, reproduce.sh, '
     'setup.py, statuses.py, finalize.py, progress.md, report.md. Evidence includes runs/*.json, '
     'mutants/*.json and *.log, cost-*.json, dense*.json, recurrence*.json, witnesses.json, '
     'minimized.json, statuses.json, pole-geometry.inputs, driver.cmd, driver.out, driver.expected, '
     'the build, sanitizer and check logs, source.sha256, distinct.log, audit.json and '
     'written-files.txt. The latter is the exact retained inventory.')
para('The scratch build directory, archive, objects, binaries, generated C mutants, and '
     'Python bytecode cache were deleted. All source generators and exact witness outputs '
     'remain. No retained file created by this lane has an executable permission bit.')
para('To recreate the findings after cleanup, run the following from the repository root. '
     'The individual commands in Findings require that reconstruction first. reproduce.sh '
     'was written as a command recipe; it was not run as a whole during this review.')
code('timeout 170 sh lanes/f-review15/reproduce.sh')

section('Not done')
para('No proof of the global image enclosure or exact derivative supremum was attempted. '
     'Numerical sampling cannot exclude an unsampled counterexample. No certified independent '
     'Gamma-integral enclosure was built, and no mpmath correct-rounding guarantee is claimed. '
     'The two unclassified B survivors have no demonstrated wrong enclosure. No leak check '
     'completed. The system FLINT shared library was not rebuilt with sanitizers. No Julia '
     'program, all-places operation, adaptive subdivision, full repository suite, or full '
     'mutation sweep was run. No production code, driver code, test or document outside '
     'the lane was repaired, since it is read-only.')

section('Sources pending')
para('[source pending: Gelfond-Schneider theorem in refs/ for the classification of nonzero '
     'exact dyadic finite poles.] The code does not need this classification to reject '
     'an undecided denominator.')
para('[source pending: FLINT 3.0.1 acb/arb exponential and Gamma evaluator source and range '
     'guards.] The code was measured as installed; its evaluator internals were not proved.')
para('[source pending: rigorous mpmath 1.3.0 gamma, power and digamma numerical error '
     'guarantees.] The point oracle uses the stated numerical margin; this is not a '
     'certificate of all rounding errors.')

section('Findings against the specification')
para('None established. F1-F3 are tests that admit injected contract violations, while the '
     'unchanged implementation passes their distinguishing inputs. F4-F5 have no '
     'demonstrated contract violation. The live conventions status row already includes '
     'LIMIT; the older lane report and Y17 sentence saying that it is pending are stale.')

report = '\n'.join(lines).rstrip() + '\n'
bad_lines = [(i + 1, len(s)) for i, s in enumerate(report.splitlines()) if len(s) > 116]
assert not bad_lines, bad_lines
assert all(not f.get('findings') for _, f in results)
audit = {'source_sha256': digest, 'completed_oracle_files': len(results),
         'random_records': {str(k): v for k, v in random.items()},
         'distinct_random_boxes': distinct, 'faults': 13, 'survivors': 5,
         'report_lines': len(report.splitlines()), 'report_lines_over_116': 0,
         'cleanup_build_exists_before': (lane / 'build').exists()}
shutil.rmtree(lane / 'build')
shutil.rmtree(lane / '__pycache__', ignore_errors=True)
audit['cleanup_build_exists_after'] = (lane / 'build').exists()
(lane / 'audit.json').write_text(json.dumps(audit, indent=2) + '\n')
inventory = sorted(str(p.relative_to(lane)) for p in lane.rglob('*') if p.is_file()
                   and p.name not in ['brief.md', 'lane.log', 'stdout.log'])
inventory += ['report.md', 'written-files.txt']
(lane / 'written-files.txt').write_text('\n'.join(sorted(set(inventory))) + '\n')
assert not any((lane / p).stat().st_mode & 0o111 for p in inventory
               if p not in ['report.md', 'written-files.txt'])
(lane / 'report.md').write_text(report)
print(json.dumps({'report': str(lane / 'report.md'), 'lines': audit['report_lines'],
                  'oracle_files': len(results), 'random_records': sum(r['cases'] for r in random.values()),
                  'distinct_random_boxes': sum(distinct.values()), 'build_removed': True,
                  'report_lines_over_116': 0}))
