#!/usr/bin/env python3
"""Replay f-review13's unchanged fault patches in lane scratch storage.
Usage (repository root): timeout 170 python3 -B lanes/f-repair8/mutate.py prep | ID...
The default source is the saved part-1 implementation. Set IDLOG_MUTATION_SOURCE=src/gfunc_log.c
and IDLOG_PHASE=final for the repaired source. Each ID builds build/mut/<ID>/ in this lane.
prep compiles the test, driver and review probe. Only the C test is run for each mutant.
Every program is under timeout; JSONL records retain counts and the first failed assertions.
"""
import os, sys, subprocess, shutil, json

B = 'lanes/f-repair8/build/plain'
MUT = 'lanes/f-repair8/build/mut'
SRC = open(os.environ.get('IDLOG_MUTATION_SOURCE', 'lanes/f-repair8/part1_gfunc_log.c')).read()
CC = ['cc', '-Iinclude', '-Isrc', '-Itests', '-std=c11', '-O2', '-g', '-w']
LIBS = ['-lflint', '-lgmp', '-lm']

F = {}
def fault(i, what, old, new, count=1):
    F[i] = (what, old, new, count)

fault('B1', 'k = v_p(M) + 1 (off by one up)',
      'fmpz_remove(stripped, x->u.N, prime);', 'fmpz_remove(stripped, x->u.N, prime) + 1;')
fault('B2', 'k = v_p(M) - 1 (off by one down)',
      'fmpz_remove(stripped, x->u.N, prime);', 'fmpz_remove(stripped, x->u.N, prime) - 1;')
fault('B3', 'm = v_p(r) with the wrong sign', 'd->m = a - b;', 'd->m = b - a;')
fault('B4', "cofactor r' replaced by r in the compact residue",
      'fmpz_mod(residue, fmpq_numref(d.unit), mod);', 'fmpz_mod(residue, fmpq_numref(x->r), mod);')
fault('B4b', "cofactor r' replaced by r (numerator and denominator)",
      'fmpz_mod(residue, fmpq_numref(d.unit), mod);', 'fmpz_mod(residue, fmpq_numref(x->r), mod);')
fault('B5', 'p=2, k=2 evaluated by the k>=3 route (no K<=2 shortcut)',
      'if (d.K <= c ||', 'if ((d.K <= c && !(p == 2 && d.k == 2)) ||')
fault('B5p', 'p=2, k=2, 0<K<=2 evaluated by the k>=3 route (shortcut kept only for K<=0)',
      'if (d.K <= c ||', 'if ((d.K <= c && !(p == 2 && d.k == 2 && d.K > 0)) ||')
fault('B6', 'unrestricted odd prime image p^2 Z_p',
      'E = p == 2 ? 2 : 1;', 'E = p == 2 ? 2 : (!d->exact && d->k == 0 ? 2 : 1);')
fault('B7', 'Log uses log|t| on the real coordinate',
      'return conservative(y, where, x, prec, adf_sball_Log_at);',
      'return conservative(y, where, x, prec, adf_sball_log_abs_at);')
fault('B8', 'K = N instead of min(N, E)', 'd->K = d->exact || N < E ? N : E;', 'd->K = N;')
fault('B9', 'preflight: n<0 DOMAIN decided before real prec LIMIT',
      '    if (prec > ADF_REAL_PREC_MAX)\n    {\n        if (where != NULL) *where = adf_place_inf();\n'
      '        return ADF_LIMIT;\n    }\n    if (n < 0) return ADF_DOMAIN;',
      '    if (n < 0) return ADF_DOMAIN;\n    if (prec > ADF_REAL_PREC_MAX)\n    {\n'
      '        if (where != NULL) *where = adf_place_inf();\n        return ADF_LIMIT;\n    }')
fault('B9c', 'preflight: aggregate refusal decided before a later known local refusal',
      '        if (st != ADF_OK)\n        {\n            if (where != NULL) *where = ps[i];',
      '        if (st != ADF_OK && aggregate)\n        { fmpq_clear(d.unit); fmpz_clear(pz); return ADF_LIMIT; }\n'
      '        if (st != ADF_OK)\n        {\n            if (where != NULL) *where = ps[i];')
fault('B10', 'refine writes y when the real part is OK, before the finite failure check',
      '    if (st == ADF_OK) adf_adele_swap(y, t);\n    else if (where != NULL) *where = st_real == st',
      '    if (st_real == ADF_OK) adf_adele_swap(y, t);\n'
      '    if (st != ADF_OK && where != NULL) *where = st_real == st')
fault('B10b', 'one_place writes y before the failure check',
      '    if (st == ADF_OK) adf_sball_swap(y, t);\n    else if (where != NULL) *where = v;',
      '    adf_sball_swap(y, t);\n    if (st != ADF_OK && where != NULL) *where = v;')
fault('B11', 'where set on OK by Log_at at infinity',
      '    if (st == ADF_OK) adf_sball_swap(y, t);\n    else if (where != NULL) *where = v;',
      '    if (st == ADF_OK) { adf_sball_swap(y, t); if (where != NULL && !absval && '
      'adf_place_is_archimedean(v)) *where = v; }\n    else if (where != NULL) *where = v;')
fault('B12', 'CRT centre not reduced to [0,R) (symmetric representative)',
      'fmpz_CRT(A, t->fin.A, t->fin.H, b, q, 0);', 'fmpz_CRT(A, t->fin.A, t->fin.H, b, q, 1);')
fault('B12b', 'compact residue not reduced after the last product',
      '    fmpz_mul(residue, residue, inv); fmpz_mod(residue, residue, mod);\n    a->p = p;',
      '    fmpz_mul(residue, residue, inv);\n    a->p = p;')
fault('B13', 'named prime dividing neither M nor r skipped in refine',
      '        st = local_log(l, x, p, N);\n        if (st != ADF_OK)\n        {\n            st_fin = st;',
      '        if (!fmpz_is_zero(x->u.N) && fmpz_fdiv_ui(x->u.N, p) != 0 && fmpz_fdiv_ui(fmpq_numref(x->r), p) != 0'
      ' && fmpz_fdiv_ui(fmpq_denref(x->r), p) != 0) continue;\n'
      '        st = local_log(l, x, p, N);\n        if (st != ADF_OK)\n        {\n            st_fin = st;')
fault('O1', "zero centre whenever r'=1 (c ignored)",
      '(fmpq_is_one(d.unit) && fmpz_is_one(x->u.c)))\n    {\n        zero_ball(y, p, d.K, 0);',
      'fmpq_is_one(d.unit))\n    {\n        zero_ball(y, p, d.K, 0);')
fault('O2', 'c dropped from the compact residue',
      '    fmpz_mod(inv, x->u.c, mod);\n    fmpz_mul(residue, residue, inv); fmpz_mod(residue, residue, mod);\n',
      '    fmpz_mod(inv, x->u.c, mod);\n')
fault('O3', 'aggregate: baseline 2^2 not charged (total starts at 0)', 'slong total = 4;', 'slong total = 0;')
fault('O4', 'aggregate: >= instead of >',
      'if (charge > (ADF_IDLOG_CRT_BITS_MAX - total) / bits)',
      'if (charge >= (ADF_IDLOG_CRT_BITS_MAX - total) / bits)')
fault('O5', 'where=infinity whenever the real part failed',
      '*where = st_real == st ? adf_place_inf() : wp;', '*where = st_real != ADF_OK ? adf_place_inf() : wp;')
fault('O6', 'refine: exact local zero not rounded to N', 'K = l->exact ? N : l->N;', 'K = l->N;')
fault('O8', 'conservative ball 0 + 2 Zhat',
      '        /* IL5: the canonical global triple (0,4,1), even for exact input. */\n'
      '        fmpz_set_ui(t->fin.H, 4);',
      '        fmpz_set_ui(t->fin.H, 2);')
fault('O9', 'refine: place list not sorted', '        qsort(ps, (size_t)n, sizeof(*ps), place_order);\n', '')
fault('O10', 'known local refusal: earlier primes not evaluated',
      'for (slong i = 0; i < known; i++)', 'for (slong i = 0; i < 0; i++)')
fault('O11', 'preflight compact-power check dropped',
      '            K > ADF_LBALL_BITS_MAX / bits)\n            st = ADF_LIMIT;',
      '            K > ADF_LBALL_BITS_MAX / bits && 0)\n            st = ADF_LIMIT;')
fault('O14', 'real precision ceiling applied at a prime in Log_at',
      'if (adf_place_is_archimedean(v) && prec > ADF_REAL_PREC_MAX)', 'if (prec > ADF_REAL_PREC_MAX)')
fault('O16', 'exact zero returned for M>0, r\'=1, c=1',
      'if (d.exact && fmpq_is_one(d.unit))', 'if (fmpq_is_one(d.unit) && (d.exact || fmpz_is_one(x->u.c)))')
fault('O17', 'aggregate: named 2 charged L instead of L-2', 'charge = p == 2 ? L - 2 : L;', 'charge = L;')
fault('O18', 'refine: real status ignored when the finite part is OK',
      '    if (st_real > st_fin) st = st_real;\n', '')
fault('O19', 'refine_budget: exp bound not checked (exact zero as in _at)',
      '        if (st == ADF_OK && !exp_ok(K)) st = ADF_LIMIT;\n', '')

fault('B9d', 'preflight: n > 65536 LIMIT decided before real prec LIMIT (where untouched)',
      '    if (prec > ADF_REAL_PREC_MAX)\n    {\n        if (where != NULL) *where = adf_place_inf();\n'
      '        return ADF_LIMIT;\n    }\n    if (n < 0) return ADF_DOMAIN;\n'
      '    if (n > ADF_IDLOG_PLACES_MAX) return ADF_LIMIT;',
      '    if (n > ADF_IDLOG_PLACES_MAX) return ADF_LIMIT;\n    if (prec > ADF_REAL_PREC_MAX)\n    {\n'
      '        if (where != NULL) *where = adf_place_inf();\n        return ADF_LIMIT;\n    }\n'
      '    if (n < 0) return ADF_DOMAIN;')
fault('O22', 'aggregate refusal names infinity', '        if (known >= 0)\n        {', '        if (1)\n        {')
fault('O23', 'main-phase local failure names the first listed prime',
      '            st_fin = st; wp = ps[i];\n            break; /* finite failures',
      '            st_fin = st; wp = ps[0];\n            break; /* finite failures')
fault('O24', 'refine: real failure returned before the finite status',
      '    if (st_real > st_fin) st = st_real;', '    if (st_real != ADF_OK) st = st_real;')

# B4b also replaces the denominator
F['B4b'] = (F['B4b'][0], F['B4b'][1], F['B4b'][2], 1)

def apply(i):
    what, old, new, count = F[i]
    s = SRC
    assert s.count(old) == count, (i, s.count(old))
    s = s.replace(old, new)
    if i == 'B4b':
        o2 = 'fmpz_invmod(inv, fmpq_denref(d.unit), mod)'
        assert s.count(o2) == 1
        s = s.replace(o2, 'fmpz_invmod(inv, fmpq_denref(x->r), mod)')
    assert s != SRC
    return s

def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)

def prep():
    os.makedirs(MUT, exist_ok=True)
    for src, obj, extra in (('tests/test_gfunc_log.c', 'test.o', []), ('tools/adf/adf.c', 'adf.o', []),
                            ('lanes/f-review13/probe.c', 'probe.o', [])):
        r = run(['timeout', '170'] + CC + ['-c', src, '-o', MUT + '/' + obj])
        assert r.returncode == 0, r.stderr
    print('prepared test, driver and probe objects')

def link(lib, d):
    os.makedirs(d, exist_ok=True)
    for obj, out, extra in (('test.o', 'test', [B + '/support/golden.o', B + '/support/jsonl.o']),
                            ('adf.o', 'adf', []), ('probe.o', 'probe', [])):
        r = run(['timeout', '170', 'cc', MUT + '/' + obj] + extra + [lib] + LIBS + ['-o', d + '/' + out])
        assert r.returncode == 0, r.stderr

def probe(d):
    r = run(['timeout', '120', d + '/probe'], stdin=open('lanes/f-review13/distinguish.txt'))
    return r.stdout.splitlines() + (['probe exit %d' % r.returncode] if r.returncode else [])

def driver(d):
    res = {}
    for name in ('gfunc-log-values', 'gfunc-log-status', 'gfunc-log-refine-values', 'gfunc-log-refine-status'):
        cmd = 'tests/driver/%s.cmd' % name
        want = 1 if open(cmd).readline().startswith('#!exit 1') else 0
        r = run(['timeout', '60', d + '/adf'], stdin=open(cmd))
        ok = r.returncode == want and r.stdout == open('tests/driver/%s.out' % name).read()
        res[name] = 'pass' if ok else 'FAIL'
    return res

def mutant(i):
    d = MUT + '/' + i
    os.makedirs(d, exist_ok=True)
    open(d + '/gfunc_log.c', 'w').write(apply(i))
    r = run(['timeout', '170'] + CC + ['-c', d + '/gfunc_log.c', '-o', d + '/gfunc_log.o'])
    if r.returncode:
        print(i, 'COMPILE FAIL', r.stderr[:500]); return
    shutil.copy(B + '/libadelefeld.a', d + '/lib.a')
    r = run(['ar', 'r', d + '/lib.a', d + '/gfunc_log.o'])
    assert r.returncode == 0, r.stderr
    link(d + '/lib.a', d)
    with open(d + '/test.log', 'w') as log:
        t = subprocess.run(['timeout', '60', d + '/test'], stdout=log, stderr=subprocess.STDOUT)
    summary, failed, tail, assertions = [], [], [], []
    with open(d + '/test.log', errors='replace') as log:
        for l in log:
            if 'tests,' in l and 'failed tests' in l:
                summary.append(l.strip())
            if l.startswith('FAIL ') and 'failed checks)' in l:
                failed.append(l.split()[1])
            if 'check failed:' in l and len(assertions) < 3:
                assertions.append(l.strip())
            tail = (tail + [l.strip()[:200]])[-3:]
    os.remove(d + '/test.log')
    rec = dict(id=i, what=F[i][0], test_exit=t.returncode,
               test_summary=summary[-1] if summary else tail,
               failed_tests=failed, first_assertions=assertions)
    print(json.dumps(rec), flush=True)
    with open('lanes/f-repair8/mutations-' + os.environ.get('IDLOG_PHASE', 'part1') + '.jsonl', 'a') as f:
        f.write(json.dumps(rec) + '\n')
    for name in ('gfunc_log.o', 'lib.a', 'test', 'adf', 'probe'):
        os.remove(d + '/' + name)

if __name__ == '__main__':
    if sys.argv[1] == 'prep':
        prep()
    elif sys.argv[1] == 'list':
        for k, v in F.items():
            print(k, v[0])
    else:
        for i in sys.argv[1:]:
            mutant(i)
