#!/usr/bin/env python3
"""f-review14 part 2: plant one own fault at a time in a scratch copy of src/symbol.c or
src/catalogue.c and run the matching test program, the eight driver cases and the probe.

Usage (repository root):
    python3 -B lanes/f-review14/mutate.py prep
    python3 -B lanes/f-review14/mutate.py list
    python3 -B lanes/f-review14/mutate.py S1 S2 ...

Every program runs under timeout. Nothing outside lanes/f-review14/build is written.
"""
import json, os, shutil, subprocess, sys

B = 'lanes/f-review14/build'
MUT = B + '/mut'
SRC = {'S': 'src/symbol.c', 'C': 'src/catalogue.c'}
TEST = {'S': 'tests/test_symbol.c', 'C': 'tests/test_catalogue.c'}
CC = ['cc', '-Iinclude', '-Isrc', '-Itests', '-std=c11', '-O1', '-g', '-w']
LIBS = ['-lflint', '-lgmp', '-lm']
DRV = {'S': ['symbol-residue', 'symbol-hilbert'],
       'C': ['catalogue-binomial', 'catalogue-power', 'catalogue-cyclotomic', 'catalogue-volume']}

F = {}


def fault(i, kind, what, old, new, count=1):
    assert old in open(SRC[kind]).read()
    F[i] = (kind, what, old, new, count)


# ---------------------------------------------------------------- symbols
fault('S1', 'S', '(a/2): the supplement accepts 3 mod 8, so 3 and 5 are not separated',
      's = (t % 2 == 0 || r == 1 || r == 7) ? 1 : -1;',
      's = (t % 2 == 0 || r == 1 || r == 3) ? 1 : -1;')
fault('S2', 'S', 'sign for a negative lower entry dropped',
      '    if (fmpz_sgn(b) <= 0) return fmpz_kronecker(a,b);',
      '    if (fmpz_sgn(b) < 0) { fmpz_t bb; int z; fmpz_init(bb); fmpz_neg(bb,b);'
      ' z = fmpz_kronecker(a,bb); fmpz_clear(bb); return z; }')
fault('S3', 'S', 'precision rule at 2: 2 known digits read as 3',
      'if(a->v <= WORD_MAX-3 && a->N >= a->v+3) k=3;',
      'if(a->v <= WORD_MAX-2 && a->N >= a->v+2) k=3;')
fault('S4', 'S', "scale cofactor r' dropped at an odd prime only",
      'c->u[0]=scale.u[0]*unit_legendre(fmpz_fdiv_ui(a->u.c,p),p);',
      'c->u[0]=unit_legendre(fmpz_fdiv_ui(a->u.c,p),p);')
fault('S5', 'S', 'real place at a rational pair: +1 always',
      '*z=adf_rat_sgn(a)<0 && adf_rat_sgn(b)<0 ? -1 : 1;',
      '*z=1;')
fault('S6', 'S', 'where written on success as well as on failure',
      'if (st != ADF_OK && where != NULL) *where = p;',
      'if (where != NULL) *where = p;')
fault('S7', 'S', 'different local primes: the larger prime is reported',
      'return report(ADF_DOMAIN,where,adf_place_cmp(v,w)<0 ? v : w);',
      'return report(ADF_DOMAIN,where,v);')
fault('S8', 'S', 'Kronecker certificate modulus: the factor 8 is dropped',
      '        fmpz_mul_ui(K,K,8);',
      '        fmpz_set(K,K);')
fault('S9', 'S', 'Jacobi accepts an even lower entry',
      'return kind == 1 && (fmpz_sgn(b) <= 0 || fmpz_is_even(b)) ? ADF_DOMAIN : ADF_OK;',
      'return kind == 1 && fmpz_sgn(b) <= 0 ? ADF_DOMAIN : ADF_OK;')
fault('S10', 'S', 'non-integral ball: DOMAIN and NOT_DETERMINED exchanged',
      'st = fmpz_divisible(A,g) ? ADF_NOT_DETERMINED : ADF_DOMAIN;',
      'st = fmpz_divisible(A,g) ? ADF_DOMAIN : ADF_NOT_DETERMINED;')
fault('S11', 'S', 'odd formula: the u and w factors interchanged',
      'return (alpha && beta && p%4==3 ? -1 : 1) * (beta ? u : 1) * (alpha ? w : 1);',
      'return (alpha && beta && p%4==3 ? -1 : 1) * (beta ? w : 1) * (alpha ? u : 1);')
fault('S12', 'S', 'class_result: the second operand is not varied',
      'for(int i=0;i<a->n;i++) for(int j=0;j<b->n;j++)',
      'for(int i=0;i<a->n;i++) for(int j=0;j<1;j++)')
fault('S13', 'S', 'even numerator at a positive even lower entry: 0 dropped',
      'if (fmpz_is_even(a)) return 0;',
      'if (0) return 0;')
fault('S14', 'S', 'positive radius with a nonpositive lower entry: value returned',
      '    if (fmpz_sgn(b) <= 0) return ADF_NOT_DETERMINED;',
      '    if (fmpz_sgn(b) <= 0) { *z = symbol(kind,a,b); return ADF_OK; }')
fault('S15', 'S', 'odd local ball: the unit residue is taken to be a square',
      'c->n=1; c->u[0]=unit_legendre(unit_residue(a->u,p),p);',
      'c->n=1; c->u[0]=1;')
fault('S16', 'S', '2-adic unit classes: the known-digit mask is one digit short',
      'int mask = (1<<k)-1;',
      'int mask = k<=1 ? 0 : (1<<(k-1))-1;')
fault('S17', 'S', 'idele at an odd prime outside N: the unknown unit is a square',
      'else { c->n=2; c->u[0]=1; c->u[1]=-1; }',
      'else { c->n=1; c->u[0]=1; }')
fault('S18', 'S', 'where never written on failure',
      'if (st != ADF_OK && where != NULL) *where = p;',
      'if (0 && where != NULL) *where = p;')

# ---------------------------------------------------------------- catalogue
fault('C1', 'C', 'conservative radius N/gcd(N,k!) replaced by N/gcd(N,(k-1)!)',
      'fmpz_fac_ui(t,k); fmpz_gcd(R,H,t); fmpz_divexact(R,H,R);',
      'fmpz_fac_ui(t,k-1); fmpz_gcd(R,H,t); fmpz_divexact(R,H,R);')
fault('C2', 'C', 'smallest radius: binom(a) subtracted instead of the centre binom(a,k)',
      'binomial(b,t,k); fmpz_sub(b,b,C);',
      'binomial(b,t,k); fmpz_sub(b,b,A);')
fault('C3', 'C', 'the k limit is decided after the domain check',
      '    if (k>(tight ? ADF_BINOM_TIGHT_K_MAX : ADF_BINOM_K_MAX)) return ADF_LIMIT;\n',
      '    int limit_hit = k>(tight ? ADF_BINOM_TIGHT_K_MAX : ADF_BINOM_K_MAX);\n')
fault('C4', 'C', 'coarse modulus D = gcd(N, c^M + 1)',
      'signed_powm(r,u->c,M,u->N); fmpz_sub_ui(r,r,1); fmpz_gcd(D,D,r);',
      'signed_powm(r,u->c,M,u->N); fmpz_add_ui(r,r,1); fmpz_gcd(D,D,r);')
fault('C5', 'C', 'finest: the outside-prime block is dropped',
      '    adf_ucoset_set_fmpz2(all,one,one);\n'
      '    adf_ucoset_pow_tight(block,all,(slong)fmpz_get_ui(g));\n'
      '    fmpz_set(B,block->N);',
      '    fmpz_one(B); (void)all; (void)block;')
fault('C6', 'C', 'finest: the CRT centre replaced by c^e modulo the product',
      '    if (fmpz_is_one(A)) fmpz_one(r);\n'
      '    else if (!fmpz_is_one(B)) fmpz_CRT(r,r,A,one,B,0);',
      '    if (fmpz_is_one(A)) fmpz_one(r);\n'
      '    else if (!fmpz_is_one(B)) { fmpz_t T; fmpz_init(T); fmpz_mul(T,A,B); fmpz_mod(r,r,T); fmpz_clear(T); }')
fault('C7', 'C', 'finest: canon not applied to the returned modulus',
      '    adf_ucoset_set_fmpz2(y,r,t);\n    adf_ucoset_normalise(y,y);',
      '    adf_ucoset_set_fmpz2(y,r,t);')
fault('C8', 'C', 'cyclo: the inverse flag is used the other way round',
      'if (inverse && !fmpz_invmod(r,r,n)) flint_abort();',
      'if (!inverse && !fmpz_invmod(r,r,n)) flint_abort();')
fault('C9', 'C', 'cyclo: the exponent is returned modulo canon(n), not n',
      'if (!fmpz_equal(target,n) && fmpz_is_even(r)) fmpz_add(r,r,target);',
      '')
fault('C10', 'C', 'canon: any even modulus is halved',
      'if (fmpz_fdiv_ui(m,4)==2) fmpz_divexact_ui(m,m,2);',
      'if (fmpz_is_even(m)) fmpz_divexact_ui(m,m,2);')
fault('C11', 'C', 'cyclo: n = 0 is not refused',
      'if (fmpz_sgn(n)<=0) return ADF_DOMAIN;',
      'if (fmpz_sgn(n)<0) return ADF_DOMAIN;')
fault('C12', 'C', 'cyclo: the unit modulus N is not canonicalised',
      'canon_modulus(N,x->u.N);',
      'fmpz_set(N,x->u.N);')
fault('C13', 'C', 'strict: NOT_DETERMINED only when D = 1',
      'if (policy==0 && !fmpz_equal(D,u->N)) st=ADF_NOT_DETERMINED;',
      'if (policy==0 && fmpz_is_zero(D)) st=ADF_NOT_DETERMINED;')
fault('C14', 'C', 'binomial recurrence off by one',
      'fmpz_sub_ui(t,a,i-1);',
      'fmpz_sub_ui(t,a,i);')
fault('C15', 'C', 'k = 0 with H > 0 keeps the input radius',
      'if (k==0 || fmpz_is_zero(H)) fmpz_zero(R);',
      'if (k==0 && fmpz_is_zero(H)) fmpz_zero(R);')
fault('C16', 'C', 'the finest g limit is one too tight',
      'if (fmpz_cmp_ui(g,ADF_PROFPOW_FINE_G_MAX)>0) st=ADF_LIMIT;',
      'if (fmpz_cmp_ui(g,ADF_PROFPOW_FINE_G_MAX)>=0) st=ADF_LIMIT;')
fault('C17', 'C', 'the integral-domain rule returns the opposite status',
      'st=fmpz_divisible(A,g) ? ADF_NOT_DETERMINED : ADF_DOMAIN;',
      'st=fmpz_divisible(A,g) ? ADF_DOMAIN : ADF_NOT_DETERMINED;')
fault('C18', 'C', 'the unit coset is used without normalising it',
      'adf_ucoset_normalise(u,a);',
      'adf_ucoset_set(u,a);')
fault('C19', 'C', 'the exact base -1: odd and even exponents exchanged',
      'if (fmpz_is_one(u->c) || fmpz_is_even(e)) adf_ucoset_one(u);',
      'if (fmpz_is_one(u->c) || fmpz_is_odd(e)) adf_ucoset_one(u);')

# C3 needs the second half of the patch
fault('C20', 'C', 'binom: the k limit is decided only for an in-domain input',
      '    if (k>(tight ? ADF_BINOM_TIGHT_K_MAX : ADF_BINOM_K_MAX)) return ADF_LIMIT;\n',
      '    int limit_hit = k>(tight ? ADF_BINOM_TIGHT_K_MAX : ADF_BINOM_K_MAX);\n')
fault('C21', 'C', 'profpow: the finest g limit is decided before the domain check',
      '    st=integral_status(e,M,d);\n    if (st!=ADF_OK) goto done;',
      '    st=integral_status(e,M,d);\n'
      '    { fmpz_t g0; fmpz_init(g0); fmpz_gcd(g0,e,M);\n'
      '      if (policy==2 && fmpz_cmp_ui(g0,ADF_PROFPOW_FINE_G_MAX)>0)\n'
      '      { fmpz_clear(g0); st=ADF_LIMIT; goto done; }\n'
      '      fmpz_clear(g0); }\n'
      '    if (st!=ADF_OK) goto done;')

fault('C22', 'C', 'profpow: LIMIT replaces the domain status when g is above the cap',
      '    if (st!=ADF_OK) goto done;',
      '    if (st!=ADF_OK) { fmpz_t g0; fmpz_init(g0); fmpz_gcd(g0,e,M);\n'
      '      if (policy==2 && fmpz_cmp_ui(g0,ADF_PROFPOW_FINE_G_MAX)>0) st=ADF_LIMIT;\n'
      '      fmpz_clear(g0); goto done; }')

EXTRA = {'C3': ('    st=integral_status(A,H,d);\n',
                '    st=integral_status(A,H,d);\n    if (limit_hit) return ADF_LIMIT;\n'),
         'C20': ('    st=integral_status(A,H,d);\n    if (st==ADF_OK)\n    {\n',
                 '    st=integral_status(A,H,d);\n    if (st==ADF_OK) st = limit_hit ? ADF_LIMIT : st;\n'
                 '    if (st==ADF_OK)\n    {\n')}


def apply(i):
    kind, what, old, new, count = F[i]
    s = open(SRC[kind]).read()
    assert s.count(old) == count, (i, s.count(old))
    s = s.replace(old, new)
    if i in EXTRA:
        o2, n2 = EXTRA[i]
        assert s.count(o2) == 1, i
        s = s.replace(o2, n2)
    assert s != open(SRC[kind]).read()
    return s


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def cc(src, obj):
    r = run(['timeout', '170'] + CC + ['-c', src, '-o', obj])
    assert r.returncode == 0, r.stderr[-800:]


def link(lib, d, kinds):
    for obj, out, extra in (('test_symbol.o', 'test_symbol', [B + '/support/jsonl.o']),
                            ('test_catalogue.o', 'test_catalogue', [B + '/support/jsonl.o']),
                            ('adf.o', 'adf', []), ('probe.o', 'probe', [])):
        if out.endswith('symbol') and 'S' not in kinds:
            continue
        if out.endswith('catalogue') and 'C' not in kinds:
            continue
        r = run(['timeout', '170', 'cc', MUT + '/' + obj] + extra + [lib] + LIBS + ['-o', d + '/' + out])
        assert r.returncode == 0, r.stderr[-800:]


def driver(d, kinds):
    res = {}
    for name in sum([DRV[k] for k in kinds], []):
        cmd = 'tests/driver/%s.cmd' % name
        want = 1 if open(cmd).readline().startswith('#!exit 1') else 0
        r = run(['timeout', '60', d + '/adf'], stdin=open(cmd))
        ok = r.returncode == want and r.stdout == open('tests/driver/%s.out' % name).read()
        res[name] = 'pass' if ok else 'FAIL'
    return res


def probe(d, kinds):
    ins = []
    for ln in open('lanes/f-review14/distinguish.txt'):
        ins.append(ln)
    r = run(['timeout', '120', d + '/probe'], input=''.join(ins))
    return r.stdout.splitlines() + (['probe exit %d' % r.returncode] if r.returncode else [])


def prep():
    os.makedirs(MUT, exist_ok=True)
    for src, obj in (('tests/test_symbol.c', 'test_symbol.o'), ('tests/test_catalogue.c', 'test_catalogue.o'),
                     ('tools/adf/adf.c', 'adf.o'), ('lanes/f-review14/probe.c', 'probe.o')):
        cc(src, MUT + '/' + obj)
    d = MUT + '/base'
    os.makedirs(d, exist_ok=True)
    shutil.copy(B + '/libadelefeld.a', d + '/lib.a')
    link(d + '/lib.a', d, 'SC')
    base = {'driver': driver(d, 'SC'), 'probe': probe(d, 'SC')}
    json.dump(base, open(MUT + '/baseline.json', 'w'))
    print('baseline driver', base['driver'], 'probe lines', len(base['probe']))


def mutant(i):
    kind = F[i][0]
    d = MUT + '/' + i
    os.makedirs(d, exist_ok=True)
    base = os.path.basename(SRC[kind])
    open(d + '/' + base, 'w').write(apply(i))
    cc(d + '/' + base, d + '/mut.o')
    os.replace(d + '/mut.o', d + '/' + base[:-2] + '.o')
    shutil.copy(B + '/libadelefeld.a', d + '/lib.a')
    assert run(['ar', 'r', d + '/lib.a', d + '/' + base[:-2] + '.o']).returncode == 0
    link(d + '/lib.a', d, kind)
    exe = d + ('/test_symbol' if kind == 'S' else '/test_catalogue')
    with open(d + '/t.log', 'w') as log:
        t = run(['timeout', '170', exe])
        log.write(t.stdout + t.stderr)
    summary, failed = [], []
    for l in (t.stdout + t.stderr).splitlines():
        if 'failed checks' in l:
            summary.append(l.strip())
        if l.startswith('FAIL '):
            failed.append(l.strip()[:120])
    pr = probe(d, kind)
    base = json.load(open(MUT + '/baseline.json'))
    diff = [k + 1 for k, (a, b) in enumerate(zip(pr, base['probe'])) if a != b]
    if len(pr) != len(base['probe']):
        diff.append('len %d vs %d' % (len(pr), len(base['probe'])))
    dr = driver(d, kind)
    rec = dict(id=i, file=SRC[kind], what=F[i][1], exit=t.returncode,
               summary=summary[-1] if summary else '', failed_tests=failed,
               driver=dr, probe_diff=diff,
               probe_lines=[pr[k - 1] if 0 < k <= len(pr) else None for k in diff])
    print(json.dumps(rec))
    with open(MUT + '/results.jsonl', 'a') as f:
        f.write(json.dumps(rec) + '\n')
    os.remove(d + '/t.log')


if __name__ == '__main__':
    if sys.argv[1] == 'prep':
        prep()
    elif sys.argv[1] == 'list':
        for k, v in F.items():
            print(k, v[0], v[1])
    else:
        for i in sys.argv[1:]:
            mutant(i)