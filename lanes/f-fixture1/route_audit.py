#!/usr/bin/env python3
"""Check the generator's route counts against an instrumented scratch copy of the actual C source."""
from collections import Counter
from pathlib import Path
import subprocess

from gen_stored_large import ROOT, LANE, audit, cases, line, route


def run(args, **kw):
    return subprocess.run(['timeout', '60', *map(str, args)], check=True, text=True, **kw)


def main():
    build = LANE / 'build'
    src = (ROOT / 'src/lfunc.c').read_text()
    for tag, call in [('word', 'log_sum_word(S, p, zr, T, P, PK);'),
                      ('F8', 'log_sum(S, p, zr, vz, K, T, PK);'),
                      ('F9', 'log_balanced(S, p, zr, vz, K, PK);')]:
        assert src.count('        ' + call) == 1
        traced = ('{ fprintf(stderr, "' + tag + ' %ld %ld %ld\\n", '
                  '(long) K, (long) W, (long) vz); ' + call + ' }')
        src = src.replace('        ' + call, '        ' + traced)
    path = build / 'lfunc_trace.c'
    path.write_text('#include <stdio.h>\n' + src)
    flags = '-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude -Isrc'.split()
    aliases = ['-Dadf_lball_' + f + '=old_lball_' + f for f in ('exp', 'log', 'Log')]
    run(['cc', *flags, *aliases, '-c', path, '-o', build / 'lfunc_trace.o'])
    run(['cc', *flags, LANE / 'stored_probe.c', build / 'lfunc_trace.o', build / 'libadelefeld.a',
         '-lflint', '-lgmp', '-lm', '-o', build / 'route_probe'])
    rows = cases(64)
    result = run([build / 'route_probe'], input='\n'.join(map(line, rows)) + '\n', capture_output=True)
    (LANE / 'route-trace.log').write_text(result.stderr)
    expected = []
    for row in rows:
        r, K, W, v = route(row, 64)
        if r in ('word', 'F8', 'F9'):
            expected += [f'{r} {K} {W} {v}'] * 2
    assert result.stderr.splitlines() == expected
    audit(rows, 64)
    print(f'C route entries={len(expected)} mismatches=0 '
          f'counts={dict(Counter(s.split()[0] for s in expected))}')


if __name__ == '__main__':
    main()
