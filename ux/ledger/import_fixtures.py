#!/usr/bin/env python3
"""Import the driver's acceptance fixtures tests/driver/NAME.cmd with NAME.out as claims.

    python3 ux/ledger/import_fixtures.py               every tests/driver/*.cmd into ux/ledger/claims/
    python3 ux/ledger/import_fixtures.py NAME ...      only these fixtures
    options: --fixtures DIR (default tests/driver), --out DIR (default ux/ledger/claims)

One claim per fixture: id `fx-NAME`; statement the first comment line (not the `#!exit` line);
script the lines of the .cmd as they are; expect the lines of the .out; expect_exit from the
`#!exit N` line of the .cmd (0 without one, as tests/test_driver.sh reads it); places by the rule
of `ledger.line_places` (lanes/ux-ledger-r1/brief.md item 4), see `footprint`. Standard library only.
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

QUESTION = 'The driver fixtures: tests/driver/*.cmd with their expected lines, as claims'


def footprint(text):
    """The places of a script: ledger.script_places of its lines (the place operand of a command
    that takes one; the moduli of `p=<n>` and `mod <n>`; R for `real` or a real coordinate)."""
    return ledger.script_places(text.split('\n'))


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
    c = {
        'id': 'fx-' + name.lower(),
        'statement': statement,
        'script': script,
        'expect': expect,
        'expect_exit': int(m.group(1)) if m else 0,
        'places': footprint(text),
        'branch': None,
        'tail': 'nothing said',
        'argument': ('Imported from %s. The expected lines were written by hand from docs/SPEC.md and '
                     'docs/conventions.md before the driver existed (tests/driver/README.md).' % rel),
        'proposer': 'tests/driver (imported by import_fixtures.py)',
        'state': 'proposed',
        'question': QUESTION,
        'source': rel,
    }
    # a fixture with root commands: its one seed, or `all` (each line prints its branch's identifier)
    roots = ledger.root_lines(script)
    seeds = set(seed for _, _, seed in roots)
    if roots:
        c['branch'] = seeds.pop() if len(seeds) == 1 and None not in seeds else 'all'
    else:
        del c['branch']
    return c


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
