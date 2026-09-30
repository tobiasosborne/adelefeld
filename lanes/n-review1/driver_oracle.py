from fractions import Fraction as Q
from math import gcd
from collections import Counter
from pathlib import Path
import subprocess

counts = Counter()
failures = []
commands, tests = [], []

def add(command, test):
    commands.append(command)
    tests.append(test)

def chk(ok, detail):
    counts['checks'] += 1
    if not ok:
        failures.append(detail)
        if len(failures) <= 15:
            print('FAIL', detail)

def unit(text):
    t = text.strip()[1:-1].split(' mod ')
    return int(t[0]), int(t[1]) if len(t) == 2 else 0

def vp(a, p):
    v = 0
    while a and a % p == 0:
        a //= p
        v += 1
    return v

def unit_power(text, c, N, k, tight):
    if text.startswith('error:'):
        chk(False, ('unitpower_status', c, N, k, tight, text))
        return
    a, M = unit(text)
    if k == 0:
        chk((a, M) == (1, 0), ('unitpow_zero', text))
        return
    chk(M > 0 and gcd(a, M) == 1, ('unitpow_canonical', text))
    for p in (2, 3, 5, 7, 11, 13):
        en, em = vp(N, p), vp(M, p)
        depth = max(en, em)+1
        modulus = p**depth
        images = {pow(t, k, modulus) for t in range(1, modulus)
                  if t % p and (t-c) % p**en == 0}
        returned = {t for t in range(1, modulus) if t % p and (t-a) % p**em == 0}
        first = min(images)
        differences = [(y-first) % modulus for y in images if y != first]
        common = min((vp(d, p) for d in differences), default=depth)
        hull = {t for t in range(1, modulus) if t % p and (t-first) % p**common == 0}
        chk(hull == returned if tight else images <= returned,
            ('unitpow_local', c, N, k, tight, p, a, M, sorted(images), sorted(returned)))
        counts['power_image_residues'] += len(images)

def real(text):
    parts = text.split('+/-')
    m = Q(parts[0].strip())
    r = Q(parts[1].strip()) if len(parts) == 2 else Q(0)
    return m-r, m+r

def encloses(text, a, b, label):
    if text.startswith('error:'):
        chk(False, (label, text, a, b))
        return
    lo, hi = real(text)
    chk(lo <= a <= b <= hi, (label, text, a, b))

def fmt(q):
    return str(q.numerator) if q.denominator == 1 else str(q)

def fmtdec(q):
    # Inputs below are multiples of 1/8.
    return str(q.numerator/q.denominator).rstrip('0').rstrip('.') if q.denominator != 1 else str(q.numerator)

for N in range(1, 13):
    for c in range(1, N+1):
        if gcd(c, N) != 1:
            continue
        text = f'[{c} mod {N}]'
        for k in (-6, -4, -3, -2, -1, 0, 1, 2, 3, 4, 6):
            for tight in (False, True):
                add(f'{"powtight" if tight else "pow"} {text} with {k}',
                    lambda out, c=c, N=N, k=k, tight=tight: unit_power(out, c, N, k, tight))

for m in (Q(-4), Q(-2), Q(-1,2), Q(1,2), Q(2), Q(4)):
    for rad in (Q(0), abs(m)/4):
        for scale in (Q(1,2), Q(1), Q(2)):
            x = f'({fmtdec(m)} +/- {fmtdec(rad)} ; {fmt(scale)} * [5 mod 6])'
            lo, hi = abs(m)-rad, abs(m)+rad
            add('norm '+x, lambda out, lo=lo/scale, hi=hi/scale: encloses(out, lo, hi, 'norm'))
            add('abs '+x+' with real', lambda out, lo=lo, hi=hi: encloses(out, lo, hi, 'abs'))
            for p in (2, 3, 5, 2**64-59):
                v = vp(scale.numerator, p)-vp(scale.denominator, p)
                add(f'valuation {x} with {p}', lambda out, v=v: chk(out == str(v), ('valuation', out, v)))
                want = Q(p)**(-v)
                add(f'abs {x} with {p}', lambda out, want=want: chk(Q(out) == want, ('absprime', out, want)))
            def class_test(out, lo=lo/scale, hi=hi/scale, m=m):
                chk(out.startswith('<'), ('class_status', out))
                if not out.startswith('<'):
                    return
                head, u = out[1:-1].split(' ; ')
                encloses(head, lo, hi, 'class')
                chk(unit(u) == (2 if m > 0 else 1, 3), ('class_sign', out, m))
            add('class '+x, class_test)
            for simple in (False, True):
                def hull_test(out, simple=simple, scale=scale, m=m, rad=rad):
                    chk(out.startswith('('), ('hull_status', out))
                    if not out.startswith('('):
                        return
                    head, fin = out[1:-1].split(' ; ')
                    encloses(head, m-rad, m+rad, 'hull_real')
                    c, N = map(Q, fin.split(' mod '))
                    wantN = scale*(3 if simple else 6)
                    wantc = scale*(2 if simple else 5)
                    chk(N == wantN and (c-wantc)/N == int((c-wantc)/N), ('hull_finite', out, wantc, wantN))
                add(('hullsimple ' if simple else 'hull ')+x, hull_test)
            add('neg '+x, lambda out, m=m, rad=rad: encloses(out[1:].split(' ; ')[0], -m-rad, -m+rad, 'neg'))
            for k in (-3, -1, 0, 1, 2, 3):
                images = ((m-rad)**k, (m+rad)**k)
                a, b = min(images), max(images)
                for op in ('pow', 'powtight'):
                    def power_test(out, a=a, b=b, k=k, scale=scale):
                        chk(out.startswith('('), ('idele_powstatus', out))
                        if not out.startswith('('):
                            return
                        head, fin = out[1:-1].split(' ; ')
                        encloses(head, a, b, 'idele_pow')
                        r = Q(fin.split(' * ')[0])
                        chk(r == scale**k, ('idele_power_content', out, scale, k))
                    add(f'{op} {x} with {k}', power_test)
            add('inv '+x, lambda out, a=min(1/(m-rad),1/(m+rad)), b=max(1/(m-rad),1/(m+rad)):
                encloses(out[1:].split(' ; ')[0], a, b, 'inv'))

for q in (Q(-8), Q(-1,2), Q(1,4), Q(1), Q(8)):
    add('idele '+fmt(q), lambda out, q=q: chk(out == f'({fmtdec(q)} ; {fmt(abs(q))} * [{-1 if q < 0 else 1}])',
                                            ('idele_q', out, q)))
    text = f'({fmtdec(q)} ; {fmt(q)})'
    add('unitof '+text, lambda out, q=q: chk(out == f'({fmtdec(q)} ; {fmt(abs(q))} * [{-1 if q < 0 else 1}])',
                                            ('unitof_q', out, q)))

Path('lanes/n-review1/driver-oracle.cmd').write_text('\n'.join(commands)+'\n')
run = subprocess.run(['timeout', '90', 'lanes/n-review1/adf', 'lanes/n-review1/driver-oracle.cmd'],
                     capture_output=True, text=True, timeout=92)
outs = run.stdout.splitlines()
chk(run.returncode == 0 and len(outs) == len(tests), ('driver_exit_lines', run.returncode, len(outs), len(tests)))
for test, out in zip(tests, outs):
    test(out)
counts['commands'] = len(commands)
print('counts', dict(counts))
print('failures', len(failures))
raise SystemExit(bool(failures))
