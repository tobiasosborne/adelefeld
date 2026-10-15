#!/usr/bin/env python3
"""Import the driver's acceptance fixtures tests/driver/NAME.cmd with NAME.out as claims.

    python3 ux/ledger/import_fixtures.py               every tests/driver/*.cmd into ux/ledger/claims/
    python3 ux/ledger/import_fixtures.py NAME ...      only these fixtures
    options: --fixtures DIR (default tests/driver), --out DIR (default ux/ledger/claims)

One claim per fixture: id `fx-NAME`; statement the first comment line (not the `#!exit` line);
script the lines of the .cmd as they are; expect the lines of the .out; expect_exit from the
`#!exit N` line of the .cmd (0 without one, as tests/test_driver.sh reads it); places by the rule
of lanes/ux-ledger/brief.md item 4(a), see `footprint`. Standard library only.
"""
import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import ledger  # noqa: E402

if hasattr(sys, 'set_int_max_str_digits'):
    sys.set_int_max_str_digits(0)  # the hostile fixtures hold integers of 100000 digits

SMALL_PRIMES = [p for p in range(2, 1000) if all(p % q for q in range(2, int(p ** 0.5) + 1))]
TOKEN_RE = re.compile(r'(?:\bp=|\bmod\s+)(\d+)')
QUESTION = 'The driver fixtures: tests/driver/*.cmd with their expected lines, as claims'


def cofactor_text(n):
    s = str(n)
    if len(s) > 40:
        s = '%s...%s (%d digits)' % (s[:12], s[-12:], len(s))
    return 'cofactor ' + s


def footprint(text):
    """The places of a script: the primes below 1000 of every `p=<n>` and `mod <n>` token of the
    lines that are not comments, by trial division; what is left of an n is shown as `cofactor n`."""
    primes, cofactors = set(), set()
    for line in text.split('\n'):
        if line.lstrip(' \t').startswith('#'):
            continue
        for m in TOKEN_RE.finditer(line):
            n = int(m.group(1))
            if n < 2:
                continue
            for p in SMALL_PRIMES:
                if n % p == 0:
                    primes.add(p)
                    while n % p == 0:
                        n //= p
            if n > 1:
                cofactors.add(n)
    return sorted(primes) + [cofactor_text(n) for n in sorted(cofactors)]


def claim_from_fixture(cmd_path):
    name = os.path.basename(cmd_path)[:-4]
    out_path = cmd_path[:-4] + '.out'
    with open(cmd_path, encoding='utf-8') as f:
        text = f.read()
    with open(out_path, encoding='utf-8') as f:
        out = f.read()
    script = text.split('\n')
    if script and script[-1] == '':
        script.pop()
    expect = out.split('\n')
    if expect and expect[-1] == '':
        expect.pop()
    statement = 'Fixture %s.' % name
    for line in script:
        s = line.strip()
        if s.startswith('#') and not s.startswith('#!'):
            statement = s.lstrip('#').strip()
            break
    m = re.search(r'^#!exit ([0-9]+)$', text, re.M)
    rel = os.path.relpath(cmd_path, ROOT)
    return {
        'id': 'fx-' + name.lower(),
        'statement': statement,
        'script': script,
        'expect': expect,
        'expect_exit': int(m.group(1)) if m else 0,
        'places': footprint(text),
        'tail': 'nothing said',
        'argument': ('Imported from %s. The expected lines were written by hand from docs/SPEC.md and '
                     'docs/conventions.md before the driver existed (tests/driver/README.md).' % rel),
        'proposer': 'tests/driver (imported by import_fixtures.py)',
        'state': 'proposed',
        'question': QUESTION,
        'source': rel,
    }


def main(argv=None):
    ap = argparse.ArgumentParser(description='Import driver fixtures as claims.')
    ap.add_argument('names', nargs='*', help='fixture names (default: all)')
    ap.add_argument('--fixtures', default=os.path.join(ROOT, 'tests', 'driver'))
    ap.add_argument('--out', default=os.path.join(HERE, 'claims'))
    args = ap.parse_args(argv)
    names = args.names or sorted(n[:-4] for n in os.listdir(args.fixtures) if n.endswith('.cmd'))
    os.makedirs(args.out, exist_ok=True)
    n = 0
    for name in names:
        cmd = os.path.join(args.fixtures, name + '.cmd')
        if not os.path.exists(cmd[:-4] + '.out'):
            print('import: %s has no .out; skipped' % name, file=sys.stderr)
            continue
        c = claim_from_fixture(cmd)
        faults = ledger.check_claim(c, c['id'])
        if faults:
            print('import: %s: %s' % (name, '; '.join(faults)), file=sys.stderr)
            return 1
        ledger.write_json(os.path.join(args.out, c['id'] + '.json'), c)
        n += 1
    print('import: %d claims written to %s' % (n, os.path.relpath(args.out, ROOT)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
