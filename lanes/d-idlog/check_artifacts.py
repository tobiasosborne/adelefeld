"""Bounded artifact checks for the d-idlog report; standard library only."""
import json
from collections import Counter
from pathlib import Path


def main():
    files = ('docs/design/idele-log.md', 'proto/idlog_checks.py', 'lanes/d-idlog/progress.md',
             'lanes/d-idlog/check_artifacts.py')
    for name in files:
        lines = Path(name).read_text().splitlines()
        bad = [(i, len(t)) for i, t in enumerate(lines, 1) if len(t) > 116]
        assert not bad, (name, bad)
        print(f'lines path={name} count={len(lines)} over_116={len(bad)}')
    for name in files:
        if name.endswith('.py'):
            compile(Path(name).read_text(), name, 'exec')
    counts = Counter()
    primes = set()
    valuations = set()
    for line in Path('lanes/d-idlog/fixtures.jsonl').read_text().splitlines():
        row = json.loads(line)
        p = row['p']
        primes.add(p)
        a, b = row['r']
        m = 0
        while a % p == 0:
            a //= p
            m += 1
        while b % p == 0:
            b //= p
            m -= 1
        valuations.add(m)
        ex = row['expected']
        assert ex['p'] == p
        if ex['exact']:
            assert row['M'] == 0 and a == b
            assert ex['center'] == 0 and ex['exponent'] is None
            counts['exact_zero'] += 1
        else:
            E = ex['exponent']
            assert ex['center'] == 0 if E <= 0 else 0 <= ex['center'] < p**E
        counts['rows'] += 1
        counts['exact_input' if row['M'] == 0 else 'inexact_input'] += 1
    assert counts == Counter(rows=21880, exact_zero=120, exact_input=480, inexact_input=21400)
    assert primes == {2, 3, 5, 7} and valuations == {-2, -1, 0, 1, 2}
    print('syntax_checks=2 errors=0')
    print('fixtures ' + json.dumps(dict(counts), sort_keys=True))
    print('fixture_primes=4 content_valuations=5 canonical_field_errors=0')


if __name__ == '__main__':
    main()
