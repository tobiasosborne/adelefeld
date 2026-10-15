"""Independent integer/rational checks of the saved user-facing evidence."""
import collections
import json
from fractions import Fraction
from pathlib import Path
import re
import subprocess
import sys

counts = collections.Counter()
failures = []


def check(kind, condition, detail):
    counts[kind] += 1
    if not condition:
        failures.append(f'{kind}: {detail}')


def digits(q, p, start, end):
    scaled = q / Fraction(p) ** start
    modulus = p ** (end - start)
    n = scaled.numerator * pow(scaled.denominator, -1, modulus) % modulus if modulus > 1 else 0
    result = []
    for _ in range(end - start):
        result.append(n % p)
        n //= p
    return result


def path_checks(path, p, centre):
    start, end = path['start_exponent'], path['end_exponent']
    want = digits(Fraction(centre), p, start, end)
    check('digit_paths', path['digits'] == want, (p, centre, path['digits'], want))
    check('digit_strings', path['digit_string'] == ' '.join(map(str, want)), (p, centre))
    for node in path['nodes']:
        k = node['exponent']
        prefix = digits(Fraction(centre), p, start, k)
        residue = sum((Fraction(d) * Fraction(p) ** (start + i) for i, d in enumerate(prefix)), Fraction())
        check('node_centres', Fraction(node['centre']) == residue, node)
        check('hover_text', node['value_form'] == f'[p={p}: {residue} + O({p}^{k})]', node)


scene_path = Path(sys.argv[1])
s = json.loads(scene_path.read_text())
provenance = s['provenance']
run = subprocess.run(['timeout', '30', 'lanes/ux-drive-atlas/build/adf'],
                     input=provenance['stdin'], text=True, capture_output=True)
check('transcript', run.stdout == provenance['stdout'], 'stdout differs')
check('transcript', run.stderr == provenance['stderr'], 'stderr differs')
check('transcript', run.returncode == provenance['exit_code'], (run.returncode, provenance['exit_code']))
check('record_outputs', run.stdout.splitlines() == [r['output'] for r in provenance['records']],
      'records differ from transcript')
records = provenance['records']
poly = list(map(int, s['polynomial'].split())) if s['polynomial'] else None
for tree in s['trees']:
    p = tree['prime']
    for depth, result in tree['depths'].items():
        check('result_output', result['printed'] == records[result['record']]['output'], (p, depth))
        status = re.match(r'error: (\w+)', result['printed'])
        check('status', result['status'] == (status[1] if status else 'OK'), (p, depth))
        check('branch_count', result['branch_count'] == len(result['paths']), (p, depth))
        distinct = len({tuple(path['digits']) for path in result['paths']})
        check('lit_count', result['lit_path_count'] == distinct, (p, depth))
        for path in result['paths']:
            path_checks(path, p, path['centre'])
            k = path.get('certificate_precision')
            if poly and k is not None:
                a = Fraction(path['centre'])
                value = sum(c * a ** i for i, c in enumerate(poly))
                # A polynomial root certificate need only isolate a root, not make its centre a root mod p^k.
                # In these two scenarios all listed centres do satisfy the stronger elementary congruence.
                if value.denominator == 1:
                    check('polynomial_residue', value.numerator % p ** k == 0, (p, k, a, value))
            if s['value'] == '9' and s['polynomial'] == '-9 0 1':
                k = path['certificate_precision']
                possible = [r for r in [-3, 3] if (Fraction(r) - Fraction(path['centre'])) % p ** k == 0]
                check('rational_branch', len(possible) == 1, (p, depth, path))
                if possible:
                    root = possible[0]
                    unit = root
                    while unit % p == 0:
                        unit //= p
                    seed = ('+1' if unit % 4 == 1 else '-1') if p == 2 else str(unit % p)
                    check('branch_seed', path['seed'] == seed, (p, depth, path['seed'], seed))
        for key, hover in result['node_balls'].items():
            ds = list(map(int, key.split('.'))) if key else []
            start = result['paths'][0]['start_exponent'] if result['paths'] else 0
            residue = sum((Fraction(d) * Fraction(p) ** (start + i) for i, d in enumerate(ds)), Fraction())
            expected = f'[p={p}: {residue} + O({p}^{start + len(ds)})]'
            check('sibling_hover', hover == expected, (key, hover, expected))
for thread in s['threads']:
    q = Fraction(thread['value'])
    record = records[thread['record']]
    output = record['output']
    if record['command'].startswith(('exp_at ', 'log_at ')) and record['command'].endswith(' with real'):
        expected_output = 'real: ' + thread['value']
    else:
        expected_output = thread['value']
    check('thread_source', output == expected_output, (record, expected_output))
    check('thread_real', Fraction(thread['real_point']) == q, thread['real_point'])
    for p, levels in thread['places'].items():
        for path in levels.values():
            path_checks(path, int(p), q)
for ball in s['ruler']['balls']:
    if ball.get('exact_point') is not None:
        q = Fraction(ball['exact_point'])
        check('real_exact', Fraction(ball['midpoint_printed']) == q and Fraction(ball['radius_printed']) == 0, ball)
        if poly:
            check('real_polynomial', sum(c * q ** i for i, c in enumerate(poly)) == 0, ball)
        check('real_endpoints', list(map(Fraction, ball['dyadic_endpoints'])) == [q, q], ball)
if s['modulus']:
    n = int(s['modulus'])
    expected = [e for e in range(n) if (e * e - e) % n == 0]
    check('modular_complete', [int(row['residue']) for row in s['modular_solutions']] == expected, expected)
    for row in s['modular_solutions']:
        e = int(row['residue'])
        check('modular_digits', list(map(int, row['digits'])) == [e % t['prime'] for t in s['trees']], row)
        check('modular_contains', row['contains'] == 'true' and (e * e - e) % n == 0, row)
print(scene_path)
print(json.dumps(dict(counts), sort_keys=True))
print(f'checks={sum(counts.values())} failures={len(failures)} records_replayed={len(records)}')
for failure in failures:
    print(failure)
sys.exit(bool(failures))
