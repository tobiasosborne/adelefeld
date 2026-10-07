#!/usr/bin/env python3
"""Slice 4a fixtures from the design oracle; exact Fractions throughout."""
import json
import sys
from fractions import Fraction as Q
from pathlib import Path
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'proto'))
import functions4_checks as oracle
from flint import ctx, arb, acb
import sympy as sp
ctx.prec = 320
OUT = Path('tests/ref/vectors/f4-slice1')
OUT.mkdir(parents=True, exist_ok=True)

def z(v):
    return [str(v.real.mid().fmpq()), str(v.real.rad().fmpq()),
            str(v.imag.mid().fmpq()), str(v.imag.rad().fmpq())]

def exact_coordinates(D, M, f, g):
    L = D*M
    data = [z(v) for v in g]
    if L > 60:
        return data
    X = sp.Symbol('X')
    phi = sp.Poly(sp.cyclotomic_poly(L, X), X, domain=sp.QQ)
    for k in range(L):
        P = sp.Poly(sum(sp.Rational(v.numerator,v.denominator)*X**((-j*k)%L)
                        for j,v in enumerate(f)), X, domain=sp.QQ).rem(phi)
        C = sp.Poly(sum(sp.Rational(v.numerator,v.denominator)*X**((j*k)%L)
                        for j,v in enumerate(f)), X, domain=sp.QQ).rem(phi)
        re = (P+C).mul_ground(sp.Rational(1,2)).rem(phi)
        if re.degree() <= 0:
            data[k][0:2] = [str(re.nth(0)/M), '0']
        diff = P-C
        if diff.is_zero:
            data[k][2:4] = ['0','0']
        elif L%4 == 0:
            im = (diff*sp.Poly(X**(3*L//4)/2,X,domain=sp.QQ)).rem(phi)
            if im.degree() <= 0:
                data[k][2:4] = [str(im.nth(0)/M),'0']
    return data

def emit(name, records):
    p = OUT / name
    p.write_text(''.join(json.dumps(r, separators=(',', ':')) + '\n' for r in records))
    print(name, len(records), p.stat().st_size)

records = []
for D, M in [(1,1),(2,3),(3,2),(4,1),(1,4),(6,6),(12,5),(1,1024)]:
    L = D*M
    f = [Q((j*7)%11-5, 8) for j in range(L)]
    if L == 1024:
        f = [Q(0)]*L
        f[1] = Q(1)
        ctx.prec = 96
        # The sparse exact sum is the oracle's phase formula, avoiding a million Python products.
        g = [oracle.phase_ball(Q(-k,L))/M for k in range(L)]
    else:
        g = oracle.ffun_transform(D,M,f)[2]
    ctx.prec = 320
    records.append(dict(D=D,M=M,f=[[str(v),'0','0','0'] for v in f],g=exact_coordinates(D,M,f,g),
                        cyclotomic=[[str(v), str(Q(-j,L)%1)] for j,v in enumerate(f)]))
    if L <= 60:
        records.append(dict(D=D,M=M,f=[[str(v),'1/64',str(Q(j%3-1,4)),'1/128']
                                      for j,v in enumerate(f)],g=[],cyclotomic=[]))
# The six valid golden inputs also get transformed, without changing the fixed golden file.
texts = []
for line in Path('tests/golden/ffun.tsv').read_text().splitlines():
    if not line or line.startswith('#'):
        continue
    raw, expected = line.split('\t')
    try:
        got = oracle.grammar.canonical('ffun',raw.encode())
    except oracle.grammar.TextError as error:
        got = '!'+error.status
    assert got == expected
    texts.append(dict(raw=raw,expected=got))
    if got.startswith('!'):
        continue
    tree = oracle.grammar._syntax('ffun',got)
    D,M = int(tree[1]),int(tree[2])
    vals = [oracle.grammar._complex(v) for v in tree[3][1]]
    f = [[str(a),str(r),str(b),str(t)] for (a,r),(b,t) in vals]
    if all(not r and not b and not t for (a,r),(b,t) in vals):
        values = [a for (a,r),(b,t) in vals]
        g = exact_coordinates(D,M,values,oracle.ffun_transform(D,M,values)[2])
    else:
        g = []
    records.append(dict(D=D,M=M,f=f,g=g,cyclotomic=[]))
# Exact nonsymmetric complex array with cardinal phases.
f = [sp.Rational(j-2,8)+sp.I*sp.Rational(j*j-3,4) for j in range(4)]
g = oracle.ffun_transform(4,1,f)[2]
records.append(dict(D=4,M=1,f=[[str(sp.re(v)),'0',str(sp.im(v)),'0'] for v in f],
                    g=[z(v) for v in g],cyclotomic=[]))
# Full uncertain transforms are independent rectangular certificates. Their rectangles may be wider
# than C's, so tests check overlap as well as exact corner containment and the separate F4 width bound.
ctx.prec = 96
for record in records:
    record['uncertain_g'] = []
    if any(Q(v[1]) or Q(v[3]) for v in record['f']):
        vals = [acb(arb(v[0],v[1]),arb(v[2],v[3])) for v in record['f']]
        record['uncertain_g'] = [z(v) for v in oracle.ffun_transform(record['D'],record['M'],vals)[2]]
ctx.prec = 320
emit('functions.jsonl',records)
emit('texts.jsonl',texts)
a = (2,3,[Q(j-2,4) for j in range(6)])
b = (3,2,[Q(3-j,8) for j in range(6)])
D,M,f = oracle.combine(a,b)
emit('sums.jsonl',[dict(a=[a[0],a[1],[str(v) for v in a[2]]],
                       b=[b[0],b[1],[str(v) for v in b[2]]],D=D,M=M,
                       ra=[str(v) for v in oracle.refine(*a,D,M)],
                       rb=[str(v) for v in oracle.refine(*b,D,M)],sum=[str(v) for v in f])])
emit('caps.jsonl',[dict(D=1,M=1024,set=0,fourier=0),dict(D=1,M=1025,set=0,fourier=10),
                   dict(D=1,M=1048576,set=0,fourier=10),dict(D=1,M=1048577,set=10,fourier=10)])
assert sum(p.stat().st_size for p in OUT.iterdir()) <= 400*1024
