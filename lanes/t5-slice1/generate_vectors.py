#!/usr/bin/env python3
"""Slice 5a fixtures; independent point enclosures and oracle closed forms.

Run: timeout 120 python3 -B lanes/t5-slice1/generate_vectors.py
P9/P10 and api-5 section 2 define the values. zeta_checks.point_enclosure
integrates Gamma by a Taylor polynomial with tails, not FLINT Gamma.
Finite references use its scalar real interval assembly. Numerical strings
have 60 digits and an explicit decimal conversion margin. Input rationals
are dyadic; radii fit mag's 30-bit significand exactly. Width allowance is
64 times a certified sampled image diameter plus 2^(-prec+8)*image_bound.
Conductor 65537 and its square are descriptor-only LIMIT tests: no group
setup or large phase array is requested. They still have mathematical value 1.
"""
import sys
sys.dont_write_bytecode = True
from pathlib import Path
from fractions import Fraction as F
from itertools import product
import json
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'proto'))
import tate_checks as t
import zeta_checks as z
from flint import arb, ctx
mp = t.mp
mp.mp.dps = 90
ctx.prec = 512
ROOT = Path(__file__).resolve().parents[2]
rows = []
cache = {}


def m(q):
    return mp.mpf(q.numerator) / q.denominator


def dec(q):
    return mp.nstr(q, 60)


def up(q):
    return str(t.endpoints(q)[1])


def point(p, q, alpha, s, e):
    key = (p, q, alpha, s, e)
    if key in cache:
        return cache[key]
    if p and q > 1:
        value = t.local_integral(p, mp.mpc(m(alpha[0]),m(alpha[1])),
                                 t.Character(q,()), mp.mpc(m(s[0]),m(s[1])))
        assert value == 1
        result = ('1', '0', (arb(1), arb(0)))
    else:
        if not p:
            box = z.point_enclosure('real', s[0] + e, s[1])
            value = t.real_integral(e, mp.mpc(m(s[0]), m(s[1])))
        else:
            a = z.aa(p).log()
            T = z.cexp((-a*z.aa(s[0]), -a*z.aa(s[1])))
            box = z.cdiv((arb(1), arb(0)), z.cadd((arb(1), arb(0)),
                z.cneg(z.cmul((z.aa(alpha[0]), z.aa(alpha[1])), T))))
            value = t.local_integral(p, mp.mpc(m(alpha[0]), m(alpha[1])), t.character(1),
                                     mp.mpc(m(s[0]), m(s[1])))
        texts = []
        for j, v in enumerate((value.real, value.imag)):
            if v == 0 and box[j].is_zero():
                texts.append('0')
            else:
                mid = dec(v)
                err = (abs(box[j]-arb(mid))+arb(10)**-59*max(arb(1), abs(box[j]))).upper()
                text = mid + ' +/- ' + dec(mp.mpf(str(t.endpoints(err)[1].numerator)) /
                                          t.endpoints(err)[1].denominator)
                assert arb(text).contains(box[j]), ('decimal enclosure', key, j)
                texts.append(text)
        result = (*texts, box)
    cache[key] = result
    return result


def directed(q, upward=False):
    if not q:
        return F(0)
    step = F(2)**(q.numerator.bit_length()-q.denominator.bit_length()-72)
    units = q/step
    return (-( (-units.numerator)//units.denominator) if upward else
            units.numerator//units.denominator)*step


def emit(p, q, n, s, a, sr=(F(0), F(0)), ar=(F(0), F(0)), status=0, e=0, exact=''):
    if a == (F(-1,2), ei) and ar == (F(0),F(0)):
        ar = (F(0),F(1,2**240))  # Include E(1/3), which is not an exact dyadic point.
    if status == 0 and p and q == 1 and s[1] == 0 and s[0].denominator == 1:
        power = F(p)**(-int(s[0]))
        if a == (F(-1,2),ei):
            exact = f'ideal E(1/3): 1/(1-({power})*X), X^2+X+1=0'
        else:
            dr, di = 1-a[0]*power, -a[1]*power
            exact = f'{dr/(dr*dr+di*di)} + ({-di/(dr*dr+di*di)})*i'
    row = dict(p=p, q=q, n=n, s=[str(x) for x in s], alpha=[str(x) for x in a],
               sr=[str(x) for x in sr], ar=[str(x) for x in ar], status=status, e=e, exact=exact)
    if status == 0:
        points = list(product(*[(x-r, x+r) if r else (x,) for x, r in zip(s+a, sr+ar)]))
        points.append(s+a)
        samples, boxes = [], []
        for x in dict.fromkeys(points):
            re, im, box = point(p, q, x[2:], x[:2], e)
            samples.append([re, im]); boxes.append(box)
        lower = F(0)
        for box in boxes:
            for j in range(2):
                lo1, hi1 = t.endpoints(box[j]); lo0, hi0 = t.endpoints(boxes[-1][j])
                lower = max(lower, lo1-hi0, lo0-hi1)
        image = max(F(1), *(max(abs(x) for x in t.endpoints(b[j])) for b in boxes for j in range(2)))
        if p and q == 1:
            logp = arb(p).log()
            ss = (arb(z.aa(s[0]),z.aa(sr[0])), arb(z.aa(s[1]),z.aa(sr[1])))
            aa = (arb(z.aa(a[0]),z.aa(ar[0])), arb(z.aa(a[1]),z.aa(ar[1])))
            T = z.cexp((-logp*ss[0],-logp*ss[1]))
            image_box = z.cdiv((arb(1),arb(0)), z.cadd((arb(1),arb(0)),z.cneg(z.cmul(aa,T))))
            image = max(image, *(max(abs(x) for x in t.endpoints(b)) for b in image_box))
        elif not p:
            R = (z.aa(sr[0])**2+z.aa(sr[1])**2).sqrt()
            B = z.derivative_bound('real',(s[0]+e,s[1]),sr)
            image += t.endpoints(R*B)[1]
        row.update(samples=samples, width_lower=str(directed(lower)), image=str(directed(image,True)))
    rows.append(row)

zero = (F(0), F(0))
with mp.workdps(180):
    ei = F(int(mp.nint(mp.sqrt(3)/2 * 2**256)), 2**256)
alphas = [(F(1), F(0)), (F(-1), F(0)), (F(0), F(1)), (F(-1,2), ei),
          (F(2), F(0)), (F(1,2), F(1))]
for p in (2, 3, 5, 7, 13, 65537):
    for a in alphas:
        for s in ((F(2),F(0)), (F(1,2),F(3,4)), (F(-1),F(1,2))):
            emit(p, 1, 1, s, a, exact='1/(1-alpha*p^(-s)); alpha=E(1/3)' if a==alphas[3] else '1/(1-alpha*p^(-s))')
        emit(p, 1, 1, (F(2),F(1,2)), a, (F(1,4096),F(1,8192)),
             (F(1,8192),F(1,16384)))
    for q, n in (((4,3),(8,3)) if p == 2 else ((p, 2 if p != 7 else 3), (p*p, 2 if p != 7 else 3))):
        if q <= 65536:
            chi = t.character(q,n)
            assert chi.C == q, (q,n,chi.C)
        for a in alphas:
            emit(p, q, n, (F(-2),F(3,4)), a, (F(1,4096),F(1,8192)),
                 (F(1,8192),F(1,16384)), status=10 if q>65536 else 0, exact='1')
    for k in (-2,-1,0,1,2):
        aa = (F(p)**k,F(0)); ss = (F(k),F(0))
        if p != 2 and k < 0:
            # p^k is not an exact dyadic raw acb: give a ball around that rational.
            with mp.workdps(180):
                aa = (F(int(mp.nint(m(aa[0])*2**256)),2**256),F(0))
            emit(p,1,1,ss,aa,ar=(F(1,2**240),F(0)),status=1)
        else:
            emit(p,1,1,ss,aa,status=7)
        emit(p,1,1,ss,aa,(F(1,4096),F(1,4096)),status=1)
        emit(p,1,1,(F(k)+F(1,1024),F(0)),aa)
    for a in alphas[:4]:
        with mp.workdps(180):
            pole = mp.log(mp.mpc(m(a[0]),m(a[1])))/mp.log(p)
            for k in (-1,1):
                si = pole.imag+2*mp.pi*k/mp.log(p)
                ss = (F(int(mp.nint(pole.real*2**256)),2**256),F(int(mp.nint(si*2**256)),2**256))
                emit(p,1,1,ss,a,(F(1,2**100),F(1,2**100)),status=1)
for e,q,n in ((0,1,1),(1,4,3)):
    for s in ((F(2),F(0)),(F(1,2),F(3,4)),(F(-3,2),F(1,2)),(F(3),F(0))):
        emit(0,q,n,s,(F(0),F(0)),e=e)
        emit(0,q,n,s,(F(0),F(0)),(F(1,4096),F(1,8192)),e=e)
    for k in range(3):
        pole = -e-2*k
        emit(0,q,n,(F(pole),F(0)),zero,status=7,e=e)
        emit(0,q,n,(F(pole),F(0)),zero,(F(1,4096),F(1,4096)),status=1,e=e)
        emit(0,q,n,(F(pole)+F(1,1024),F(0)),zero,e=e)
# Invalid conductors and alpha geometry; a 3-primary character at p=2.
emit(2,3,2,(F(2),F(0)),(F(1),F(0)),status=7)
for q,n in ((1,1),(4,3)):
    emit(2,q,n,(F(2),F(0)),zero,status=7)
    emit(2,q,n,(F(2),F(0)),zero,ar=(F(1,1024),F(1,1024)),status=1)
emit(2,4,3,(F(2),F(0)),(F(0),F(1)),exact='1')
path = ROOT/'tests/ref/vectors/t5-slice1/local.jsonl'
path.write_text(''.join(json.dumps(row,separators=(',',':'))+'\n' for row in rows))
assert path.stat().st_size <= 400*1024
print(f'{len(rows)} vectors; {sum(len(r.get("samples",[])) for r in rows)} sample enclosures; '
      f'{path.stat().st_size} bytes')
