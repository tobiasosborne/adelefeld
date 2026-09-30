"""Independent rational series oracle. No import from proto/ or FLINT's padic.

The proof of the tail cutoffs and the finite root lift is in oracle-proof.md.
Every retained summand is formed as an exact Fraction, then reduced; no numerator
is rounded before division. Reduction commutes with finite addition.
"""
from fractions import Fraction as Q
from functools import lru_cache
import json
import random
import subprocess
import sys
import time
from pathlib import Path

LANE = Path(__file__).resolve().parent
BINARY = sys.argv[2] if len(sys.argv)>2 else 'build/probe'
P = (2, 3, 5, 7, 11, 18446744073709551557)
EMAX = 2**60
sys.set_int_max_str_digits(0)


def vi(a, p):
    assert a
    n = 0
    while a % p == 0:
        a //= p
        n += 1
    return n


def vq(a, p):
    return 10**30 if not a else vi(a.numerator, p) - vi(a.denominator, p)


def modq(a, p, n):
    if n <= 0:
        return 0
    assert a.denominator % p
    m = p**n
    return a.numerator * pow(a.denominator, -1, m) % m


def cutoff(p, z, n, exp=False):
    w = vq(z, p)
    if exp:
        # p=2: v(k!) <= k-1; odd p: v(k!) <= (k-1)/2.
        return (n + 4 + w - 2) // (w - 1) if p == 2 else (2*(n+4) + 2*w-2) // (2*w-1)
    # v_p(k) <= k/2, so k*w-v_p(k) >= k*(w-1/2).
    return (2*(n+4) + 2*w-2) // (2*w-1)


@lru_cache(None)
def series(p, z, n, exp=False):
    if n <= 0:
        return 0
    if not z:
        return 1 if exp else 0
    w = vq(z, p)
    assert w >= (2 if p == 2 else 1) if exp else w >= 1
    stop = cutoff(p, z, n, exp)
    modulus = p**n
    if n >= 1000:
        if exp:
            def block(a,b):
                # Product of ratios z/k and sum of their consecutive partial products.
                if b-a == 1:
                    return z.numerator,z.denominator*b,z.numerator
                m = (a+b)//2
                p1,q1,t1 = block(a,m)
                p2,q2,t2 = block(m,b)
                return p1*p2,q1*q2,t1*q2+p1*t2
            _,den,num = block(0,stop-1)
            return modq(Q(den+num,den),p,n)
        def poly(a,b):
            # Exact sum of signed z^(k-a)/k, a <= k < b.
            if b-a == 1:
                return Q(1 if a%2 else -1,a)
            m = (a+b)//2
            return poly(a,m)+z**(m-a)*poly(m,b)
        return modq(z*poly(1,stop),p,n)
    s, term = (1 if exp else 0), Q(1)
    for k in range(1, stop):
        if exp:
            term = term * z / k
            summand = term
        else:
            term *= z
            summand = term / k if k % 2 else -term / k
        assert summand.denominator % p
        s = (s + summand.numerator * pow(summand.denominator, -1, modulus)) % modulus
    return s


def root_lift(a, p, n):
    """Newton lift of w^(p-1)=1; doubles precision, independent of powering by p^n."""
    w = modq(a, p, 1)
    h = 1
    while h < n:
        h = min(2*h, n)
        modulus = p**h
        f = (pow(w, p-1, modulus) - 1) % modulus
        derivative = (p-1)*pow(w, p-2, modulus) % modulus
        w = (w - f*pow(derivative, -1, modulus)) % modulus
        assert pow(w, p-1, modulus) == 1
    return w


@lru_cache(None)
def point(kind, p, a, n):
    if kind == 'exp':
        return series(p, a, n, True)
    if kind == 'log':
        return series(p, a-1, n)
    assert a
    if n <= 0:
        return 0
    unit = a / Q(p)**vq(a, p)
    if p == 2:
        # All odd units are in the raw series domain. Do not use the library's sign route.
        return series(p, unit-1, n)
    if modq(unit, p, 1) == 1:
        principal = unit
    elif modq(unit, p, 1) == p-1:
        principal = -unit
    else:
        principal = unit / root_lift(unit, p, n+3)
    return series(p, principal-1, n)


def fields(p, a, M=None):
    a = Q(a)
    w = vq(a, p)
    if M is None:
        return (0, 1, 0, 0) if not a else (*((a/Q(p)**w).as_integer_ratio()), w, 0)
    if w >= M:
        return (0, 1, 0, M)
    return (modq(a/Q(p)**w, p, M-w), 1, w, M)


def domain(kind, p, a, M):
    if kind == 'Log':
        return 0 if a and (M is None or vq(a, p) < M) else 7 if M is None else 1
    centre = Q(0) if kind == 'exp' else Q(1)
    threshold = (2 if p == 2 else 1) if kind == 'exp' else 1
    d = vq(a-centre, p)
    if M is None:
        return 0 if d >= threshold else 7
    if M >= threshold and d >= threshold:
        return 0
    return 1 if d >= min(M, threshold) else 7


def expected_exact(kind, p, a, M):
    if M is not None:
        return False
    return a == 0 if kind == 'exp' else a == 1 or (p == 2 and a == -1) if kind == 'log' else abs(a/Q(p)**vq(a, p)) == 1


def image(kind, p, a, M):
    r = M if kind != 'Log' else M-vq(a, p)
    return 2 if kind != 'exp' and p == 2 and r == 1 else r


def run(cases, binary=None, seconds=170):
    binary = binary or BINARY
    data = ''.join(f'{f} {p} {un} {ud} {v} {M} {int(ex)} {n}\n'
                   for f, p, un, ud, v, M, ex, n in cases)
    t = time.monotonic()
    proc = subprocess.run(['timeout', str(seconds), str(LANE/binary)], input=data, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if proc.returncode:
        raise AssertionError((proc.returncode, proc.stderr, len(proc.stdout.splitlines())))
    rows = [r.split() for r in proc.stdout.splitlines()]
    assert len(rows) == len(cases), (len(rows), len(cases))
    print(json.dumps({'calls': len(cases)*2, 'wall_seconds': time.monotonic()-t,
                      'max_call_seconds': max(float(r[-1]) for r in rows)}), flush=True)
    return rows


def case(f, p, a, M, n):
    un, ud, v, m = fields(p, a, M)
    return f, p, un, ud, v, m, M is None, n


def check(cases, rows, points=True):
    counts = dict(inputs=0, enclosure_points=0, tight_witnesses=0, exact=0, status=0)
    for c, row in zip(cases, rows):
        f, p, un, ud, v, m, ex, n = c
        a, M = Q(un, ud)*Q(p)**v, None if ex else m
        st, rex, rv, rn, run, rud, alias, untouched, canonical = map(int, row[:9])
        assert alias == untouched == canonical == 1, (c, row)
        want = domain(f, p, a, M)
        assert st == want, (c, row, want)
        counts['inputs'] += 1
        if st:
            counts['status'] += 1
            continue
        exact = expected_exact(f, p, a, M)
        assert rex == exact, (c, row, exact)
        if exact:
            assert Q(run, rud)*Q(p)**rv == (1 if f == 'exp' else 0)
            counts['exact'] += 1
            continue
        E = None if ex else image(f, p, a, M)
        K = n if ex else min(n, E)
        assert rn == K, (c, row, K)
        centre = Q(run, rud)*Q(p)**rv
        assert modq(centre, p, K) == point(f, p, a, K), (c, row, 'centre')
        counts['enclosure_points'] += 1
        if not ex and points:
            # Includes the final admitted digit (p-1)*p^M, fractional digit steps and higher digits.
            deltas = (Q(1), Q(p-1), Q(-1), Q(1, p+1), Q(p), Q(p+1))
            samples = [a + Q(p)**M*d for d in deltas]
            for t in samples:
                assert modq(centre, p, K) == point(f, p, t, K), (c, row, 'outside', t)
                counts['enclosure_points'] += 1
            if n >= E:
                # At r=1 at 2, a and a+2^M can have the same log; a principal perturbation witnesses E=2.
                witness = a*(1+Q(p)**E) if f == 'Log' else a+Q(p)**E
                r0, r1 = point(f, p, a, E+1), point(f, p, witness, E+1)
                assert (r1-r0) % (p**(E+1)) != 0, (c, 'no tight witness')
                assert (r1-r0) % (p**E) == 0
                counts['tight_witnesses'] += 1
    print(json.dumps(counts), flush=True)
    return counts


def suite_small():
    cases = []
    for p in P:
        c = 2 if p == 2 else 1
        for f in ('exp', 'log', 'Log'):
            centres = [Q(0), Q(1), Q(-1), Q(p), Q(p**c), Q(1+p), Q(3), Q(12), Q(2), Q(10),
                       Q(p**c, p+1), Q(2*p**c, p+2 if p != 2 else p+3), Q(1,p), Q(2,p**4),
                       Q(p**3*(p+2), p+1), Q(-p**2, p+1)]
            for a in dict.fromkeys(centres):
                for M in (None, -4, -1, 0, 1, 2, 3, 5, 8):
                    for n in (-3, 0, 1, 2, 4, 9, 13):
                        cases.append(case(f,p,a,M,n))
    rows = run(cases)
    return check(cases, rows)


def suite_precision():
    cases = []
    for p in (2,3):
        c = 2 if p == 2 else 1
        for f, a in [('exp',Q(p**c)), ('exp',Q(-2*p**c,p+1)),
                     ('exp',Q(3*p**(c+1),p+3 if p==2 else p+2)),
                     ('log',Q(1+p)), ('log',Q(1)+Q(p,p+1)), ('Log',Q(2 if p==3 else 3,p**4))]:
            for n in range(1,301):
                cases.append(case(f,p,a,None,n))
    rows = run(cases)
    # Evaluate to 304 once per centre. A higher precision oracle tests every smaller requested N.
    counts = dict(inputs=len(cases), residue_checks=0)
    for c,row in zip(cases,rows):
        f,p,un,ud,v,M,ex,n = c
        st,rex,rv,rn,rnum,rden,alias,untouched,canonical = map(int,row[:9])
        assert st == 0 and rex == 0 and rn == n and alias == untouched == canonical == 1, (c,row)
        a = Q(un,ud)*Q(p)**v
        want = point(f,p,a,304) % p**n
        assert modq(Q(rnum,rden)*Q(p)**rv,p,n) == want, (c,row,want)
        counts['residue_checks'] += 1
    print(json.dumps(counts), flush=True)
    return counts


def suite_large():
    cases = []
    for p in (2,3,5,7,11):
        c = 2 if p == 2 else 1
        for n in (2000,10000):
            for f,a in [('exp',Q(p**c)), ('log',Q(1+p)), ('Log',Q(1+p,p**4))]:
                cases.append(case(f,p,a,None,n))
    rows = run(cases)
    result = check(cases, rows, False)
    return result


def suite_big():
    p = P[-1]
    cases = []
    for f,a in [('exp',Q(p,p+1)), ('log',Q(1)+Q(p,p+1)), ('Log',Q(2,p**3)),
                ('Log',Q(p**4*3,p+1))]:
        for n in range(1,21):
            cases.append(case(f,p,a,None,n))
    rows = run(cases)
    return check(cases,rows,False)


def suite_limits():
    cases, wants = [], []
    for p in P:
        c = 2 if p == 2 else 1
        # Raw canonical values; exponent limits do not belong to the canonical predicate.
        raw = [
            ('exp',1,1,c,0,1,EMAX+1,10),
            ('exp',1,1,c,0,1,-EMAX-1,10),
            ('exp',1,1,c,0,1,-2**63,10),
            ('exp',1,1,c,0,1,-EMAX,0),
            ('exp',1,1,EMAX,0,1,EMAX,0),
            ('exp',0,1,0,0,1,2**63-1,0),
            ('exp',0,1,0,EMAX,0,EMAX,0),
            ('exp',0,1,0,c-1,0,2**63-1,1),
            ('exp',1,1,c-1,c+2,0,EMAX+1,7),
            ('exp',1,1,EMAX+1,0,1,1,10),
            ('log',1,1,EMAX+1,0,1,1,10),
            ('Log',1,1,EMAX+1,0,1,1,10),
            ('log',1,1,0,EMAX+1,0,1,10),
            ('Log',1,1,0,EMAX,0,EMAX,0),
            ('Log',1,1,-EMAX,EMAX,0,2**63-1,10),
            ('Log',1,1,-EMAX,EMAX,0,EMAX,0),
            ('Log',1,1,EMAX-1,EMAX,0,2**63-1,0),
            ('Log',-1,1,-EMAX,0,1,-2**63,0),
            ('Log',0,1,0,-EMAX,0,-2**63,1),
            ('Log',0,1,0,0,1,EMAX+1,7),
            ('log',0,1,0,0,0,EMAX+1,1),
            ('log',0,1,0,1,0,EMAX+1,7),
            ('log',1,1,0,0,1,-2**63,0),
            ('Log',p+1,1,0,0,1,-2**63,10),
            ('log',p+1,1,0,0,1,67108864//p.bit_length()+1,10),
            ('Log',2 if p != 2 else 3,1,0,0,1,
             67108864//p.bit_length()+(1 if p==P[-1] else 0),10),
            ('exp',1,1,c,0,1,67108864//p.bit_length(),10 if p in (2,3,5,7,11) else None),
        ]
        for f,un,ud,v,m,ex,n,want in raw:
            # At the 64-bit prime exp has no factorial p factors for this degree range.
            # Do not perform the allowed million-term boundary call.
            if want is not None:
                cases.append((f,p,un,ud,v,m,ex,n)); wants.append(want)
    rows = run(cases)
    for case_,row,want in zip(cases,rows,wants):
        assert int(row[0]) == want, (case_,row,want)
        assert row[6:9] == ['1','1','1'], (case_,row)
    result = {'inputs':len(cases), 'status_and_alias_checks':len(cases)}
    print(json.dumps(result),flush=True)
    return result


def suite_random():
    rng = random.Random(930114)
    cases=[]
    for p in P:
        for i in range(500):
            denominator = rng.choice([d for d in range(1,32) if d%p])
            m = rng.randint(-6,5)
            unit = rng.randrange(-500,500) or 1
            if unit%p == 0:
                unit += 1
            a = Q(unit,denominator)*Q(p)**m
            f = rng.choice(('exp','log','Log'))
            if f == 'exp':
                a *= Q(p)**(max(2 if p==2 else 1,m)-m)
            elif f == 'log':
                a = 1+Q(unit,denominator)*p**rng.randint(1,5)
            M = None if i%4==0 else vq(a,p)+rng.randint(1,8)
            cases.append(case(f,p,a,M,rng.randint(-2,18)))
    return check(cases,run(cases))


def suite_bench_validate():
    rows = [r.split() for r in (LANE/'bench.out').read_text().splitlines()]
    cases = []
    for line in (LANE/'bench.in').read_text().splitlines()[:len(rows)]:
        f,p,un,ud,v,M,ex,n = line.split()
        cases.append((f,int(p),int(un),int(ud),int(v),int(M),bool(int(ex)),int(n)))
    assert len(cases) == len(rows) == 7
    return check(cases,rows,False)


def suite_memory_inputs():
    cases=[]
    for p in P:
        c=2 if p==2 else 1
        for f,a in [('exp',Q(p**c,p+1)), ('log',Q(1)+Q(p,p+1)),
                    ('Log',Q(2 if p != 2 else 3,p**5)), ('Log',Q(0))]:
            for M in (None,0,1,4,8):
                for n in (-2,1,4,19,60):
                    cases.append(case(f,p,a,M,n))
    (LANE/'memory.in').write_text(''.join(f'{f} {p} {un} {ud} {v} {M} {int(ex)} {n}\n'
                          for f,p,un,ud,v,M,ex,n in cases))
    return {'inputs_written':len(cases)}


def suite_memory_validate():
    cases=[]
    for line in (LANE/'memory.in').read_text().splitlines():
        f,p,un,ud,v,M,ex,n = line.split()
        cases.append((f,int(p),int(un),int(ud),int(v),int(M),bool(int(ex)),int(n)))
    rows=[r.split() for r in (LANE/'memory.out').read_text().splitlines()]
    assert len(rows)==len(cases)==600
    return check(cases,rows,False)


def suite_powered_large():
    p=P[-1]
    # An exact torsion factor -1 gives an independent rational oracle at high precision.
    # The library must use its powered route because the unit residue is p-1, not 1.
    a=-Q(p+1,p**4)
    cases=[case('Log',p,a,None,2000),case('Log',p,a,16,2000)]
    return check(cases,run(cases))


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv)>1 else 'small'
    start = time.monotonic()
    result = globals()['suite_'+mode]()
    print(json.dumps({'suite':mode, 'elapsed_seconds':time.monotonic()-start, **result}), flush=True)
