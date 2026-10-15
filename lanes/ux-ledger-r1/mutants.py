"""Bounded hand mutants of the code this repair changed (CLAUDE.md rule 2; docs/workflow.md rule 4).

    timeout 900 python3 -B lanes/ux-ledger-r1/mutants.py

Each mutant is applied to a copy of ux/ledger in a temporary root (links to tests/, build/, include/,
refs/, docs/) and the unit tests run there under a timeout of 300 s. A mutant is killed when they fail.
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, 'ux', 'ledger')
MUTANTS = [
    ('D3 prefix ignored', 'ledger.py', "r'^(?:.*: )?error: ([A-Z_]+)$'", "r'^error: ([A-Z_]+)$'"),
    ('D3 items not counted', 'ledger.py', "if not m or top_level_items(line) > 1:", "if not m:"),
    ('D3 depth ignored', 'ledger.py', "elif ch == ';' and depth == 0 and", "elif ch == ';' and"),
    ('D2 every named place computed', 'render.py', "computed = set(ledger.computed_places(claim, seal))",
     "computed = set(claim['places']) if seal else set()"),
    ('D2 place operand index 0', 'ledger.py', "    + [('hilbert_at', 2)])", "    + [('hilbert_at', 1)])"),
    ('U6 no R for a real coordinate', 'ledger.py', "or has_real_coordinate(s) or", "or"),
    ('U6 star counts as real', 'ledger.py', "if text[i + 1:j].strip() != '*':", "if True:"),
    ('U3 branch not required', 'ledger.py', "    if b is None:\n        n, word, _ = lines[0]",
     "    if b is None:\n        return ''\n        n, word, _ = lines[0]"),
    ('U3 seed not compared', 'ledger.py', "        if seed != b:", "        if False:"),
    ('U3 sealer ignores fault', 'seal.py', "    if fault:\n        return 'proposed', fault\n",
     "    if False:\n        return 'proposed', fault\n"),
    ('U2 any citation form', 'ledger.py', "or cited_sources(claim['argument']):",
     "or 'refs/' in claim['argument'] or cited_sources(claim['argument']):"),
    ('U2 line range not checked', 'ledger.py', "        if b <= n + 1:", "        if True:"),
]


def main():
    killed = 0
    for name, fname, a, b in MUTANTS:
        d = tempfile.mkdtemp(prefix='ledger-mutant-')
        try:
            shutil.copytree(SRC, os.path.join(d, 'ux', 'ledger'), ignore=shutil.ignore_patterns('__pycache__'))
            for n in ('tests', 'build', 'include', 'refs', 'docs'):
                os.symlink(os.path.join(ROOT, n), os.path.join(d, n))
            p = os.path.join(d, 'ux', 'ledger', fname)
            with open(p) as f:
                s = f.read()
            if s.count(a) != 1:
                print('%-32s NOT APPLIED (%d matches)' % (name, s.count(a)))
                continue
            with open(p, 'w') as f:
                f.write(s.replace(a, b))
            r = subprocess.run([sys.executable, '-B', '-m', 'unittest', 'discover', '-s', 'ux/ledger'], cwd=d,
                               capture_output=True, text=True, timeout=300)
            tail = [ln for ln in r.stderr.split('\n') if ln.startswith(('FAILED', 'OK'))]
            ok = r.returncode != 0
            killed += ok
            print('%-32s %s  %s' % (name, 'killed' if ok else 'SURVIVED', tail[-1] if tail else ''))
        finally:
            shutil.rmtree(d)
    print('mutants: %d, killed %d' % (len(MUTANTS), killed))
    return 0 if killed == len(MUTANTS) else 1


if __name__ == '__main__':
    sys.exit(main())
