"""Bounded hand mutants of the repaired lines of ux/bench (lanes/ux-bench-r1). Each mutant is applied in place,
the named test classes are run, and the file is restored; the script checks the hashes at the end.

    timeout 300 python3 lanes/ux-bench-r1/mutants.py
"""
import hashlib
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / 'ux' / 'bench'
MUTANTS = [
    ('sweep.py', "        return '%s %s with %d' % (op, x, place), None",
     "        return '%s %s with %d' % (op, place, x), None", 'SymbolTest'),
    ('sweep.py', "        if place == 'R':\n            return None, ('%s X with P",
     "        if place == 'Q':\n            return None, ('%s X with P", 'SymbolTest'),
    ('sweep.py', "SYMBOL_RE = re.compile(r'^(?:-1|0|1)$')", "SYMBOL_RE = re.compile(r'^-?\\d+$')", 'SymbolTest'),
    ('sweep.py', "           'process_time_s': round(lifetime, 6),",
     "           'process_time_s': round(times[-1], 6) if times else 0.0,", 'LifetimeTest'),
    ('sweep.py', "    lifetime = time.perf_counter() - t0\n    err",
     "    lifetime = (times[-1] if times else 0.0)\n    err", 'LifetimeTest'),
    ('render.py', "    if not terms or min(terms)[0] >= r['prec']:",
     "    if not terms or min(terms)[0] > r['prec']:", 'PrecisionTest'),
    ('render.py', "    if not terms or min(terms)[0] >= r['prec']:",
     "    if not terms or max(terms)[0] >= r['prec']:", 'PrecisionTest'),
    ('render.py', "    k, p = min(terms)", "    k, p = max(terms)", 'PrecisionTest'),
    ('render.py', "'' if place == 'R' else 'requested prec %d'", "'' if place == 'R' else 'prec %d'",
     'PrecisionTest'),
    ('render.py', "    state = 'refuted' if exc else f['state']", "    state = f['state']", 'RefutedTest'),
    ('render.py', "        lit = [r for r in covered if r['row'] in exc]", "        lit = covered", 'RefutedTest'),
    ('render.py', "            continue                                 # a refuted finding",
     "            pass                                     # a refuted finding", 'RefutedTest'),
    ('render.py', "    if state != f['state']:", "    if state == 'never':", 'RefutedTest'),
    ('render.py', "        return str(full.relative_to(ROOT))", "        return str(full.relative_to(ROOT.parent))",
     'ProvenanceTest'),
    ('render.py', "    page = render(e, records, findings, records_path=shown_path(rpath),",
     "    page = render(e, records, findings, records_path='ux/bench/out/%s.records.jsonl' % e['name'],",
     'ProvenanceTest'),
    ('render.py', "                  findings_path=shown_path(fpath) if fpath.exists() else None)",
     "                  findings_path='ux/bench/experiments/%s.findings.json' % e['name'])", 'ProvenanceTest'),
]


def digest():
    return {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(BENCH.glob('*.py'))}


def main():
    before = digest()
    killed = 0
    for i, (name, old, new, test) in enumerate(MUTANTS, 1):
        path = BENCH / name
        text = path.read_text()
        if text.count(old) != 1:
            sys.exit('mutant %d: the original text is not found once in %s' % (i, name))
        try:
            path.write_text(text.replace(old, new))
            r = subprocess.run(['timeout', '120', sys.executable, '-m', 'unittest', 'test_bench.' + test],
                               cwd=BENCH, capture_output=True, text=True, env={'PYTHONDONTWRITEBYTECODE': '1',
                                                                                'PATH': '/usr/bin:/bin'})
        finally:
            path.write_text(text)
        dead = r.returncode != 0
        killed += dead
        print('mutant %2d %-9s %-15s %s' % (i, name, test, 'killed' if dead else 'SURVIVED'))
    assert digest() == before, 'a file was not restored'
    print('%d of %d mutants killed; files restored' % (killed, len(MUTANTS)))


if __name__ == '__main__':
    main()
