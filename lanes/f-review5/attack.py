"""Independent review oracle and bounded attacks. Run one mode per timeout 60 invocation.

The series definitions are the task's definitions (docs/proofs/functions.md:22-26).
The cutoff and evaluation below do not import the implementation or the lane's oracle.

Tail proof:
1. In k!, count multiples of p, p^2, ... . Thus v_p(k!) = sum floor(k/p^j).
2. This is at most sum k/p^j = k/(p-1).
3. For an admitted argument, w >= 2 at 2, w >= 1 otherwise. Therefore
   v_p(x^k/k!) >= k*(w-1/(p-1)) >= k/2.
4. Keep all degrees k < 2H. Every omitted term has valuation >= H, tending to infinity.
   Finite tails lie in p^H Z_p by the valuation inequality. Their limit lies there by closedness.
5. For H <= 90 this script sums actual Fraction terms. For larger H it reduces each exact
   rational term modulo p^H: write k! = p^d U, x = p^w a/b. Its residue is
   p^(kw-d) a^k/(b^k U). The denominator is a unit. The recurrence for the unit factor uses
   only invertible denominators. Reducing each term commutes with the finite sum.
   Both paths consequently return the infinite-series residue to absolute precision H.
6. Signs do not change the tail proof. All four sums are formed separately from the same terms.

Fraction and modular paths are cross-checked on the ordinary local attack.
"""
from fractions import Fraction
from functools import lru_cache
from pathlib import Path
import json
import os
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
PRIMES = [2, 3, 5, 7, 13, 65537, 2**64 - 59]
B = 2**60
BITS = 2**26
MODE = sys.argv[1]
RNG = random.Random(309517)


def valuation(a, p):
    if not a:
        return 10**30
    v = 0
    while a % p == 0:
        a //= p
        v += 1
    return v


@lru_cache(maxsize=None)
def series(q, p, H, modular=False):
    if H <= 0:
        return (0, 0, 0, 0)
    P = p**H
    if not q:
        return (0, 1, 0, 1)
    sums = [0, 1, 0, 1]
    if not modular and H <= 90 and max(q.numerator.bit_length(), q.denominator.bit_length()) <= 256:
        term = Fraction(1)
        for k in range(1, 2*H):
            term = term*q/k
            if k % 2:
                sums[0] += (-1)**(k//2)*term
                sums[2] += term
            else:
                sums[1] += (-1)**(k//2)*term
                sums[3] += term
        return tuple(Fraction(s).numerator * pow(Fraction(s).denominator, -1, P) % P for s in sums)
    w = valuation(q.numerator, p)
    assert q.denominator % p and w >= (2 if p == 2 else 1)
    unit_x = q.numerator//p**w * pow(q.denominator, -1, P) % P
    unit = 1
    d = 0
    for k in range(1, 2*H):
        ku = k
        while ku % p == 0:
            d += 1
            ku //= p
        unit = unit*unit_x*pow(ku, -1, P) % P
        e = k*w-d
        if e >= H:
            continue
        term = unit*pow(p, e, P) % P
        i = 0 if k % 2 else 1
        sums[i] = (sums[i] + (-1)**(k//2)*term) % P
        sums[i+2] = (sums[i+2] + term) % P
    return tuple(sums)


def canon(p, q, M=None):
    q = Fraction(q)
    if M is not None:
        if not q or valuation(q.numerator, p)-valuation(q.denominator, p) >= M:
            return (p, 0, 1, 0, M, 0)
        w = valuation(q.numerator, p)-valuation(q.denominator, p)
        u = q / (Fraction(p)**w)
        r = u.numerator*pow(u.denominator, -1, p**(M-w)) % p**(M-w)
        return (p, r, 1, w, M, 0)
    if not q:
        return (p, 0, 1, 0, 0, 1)
    w = valuation(q.numerator, p)-valuation(q.denominator, p)
    u = q/(Fraction(p)**w)
    return (p, u.numerator, u.denominator, w, 0, 1)


def expected(f, x, N):
    p, un, ud, w, M, exact = x
    c = 2 if p == 2 else 1
    if abs(w) > B or abs(M) > B:
        return 10, None
    if (exact or un) and un and w < c:
        return 7, None
    if not exact and not un and M < c:
        return 1, None
    if exact and not un:
        return 0, (p, Fraction(f % 2), 0, 0, 1)
    E = 2*M-(p == 2) if f % 2 and not un else M
    K = N if exact else min(N, E)
    if abs(K) > B:
        return 10, None
    threshold = 2*w-(p == 2) if f % 2 else w
    if not un or K <= threshold:
        value = f % 2 if K > 0 else 0
    else:
        if K*p.bit_length() > BITS:
            return 10, None
        # Working modulus limit is a specified implementation limit, separate from the oracle.
        C = max(1, ((p-1)*K-1 + ((p-1)*w-2))//((p-1)*w-1))
        L = C-1
        if L % 2 != (1-f % 2):
            L -= 1
        d, j = 0, L
        while j >= p:
            j //= p
            d += j
        if (K+d)*p.bit_length() > BITS:
            return 10, None
        q = Fraction(un, ud)*p**w
        value = series(q, p, K, MODE == 'large')[f]
    v = valuation(value, p) if value else 0
    return 0, (p, Fraction(value//p**v), v, K, 0)


cases = []
def add(x, Ns, fs=range(4)):
    for N in Ns:
        for f in fs:
            cases.append((f, x, N))


if MODE == 'local':
    for p in PRIMES:
        c = 2 if p == 2 else 1
        for j in range(22):
            w = c+j % 4
            a, b = RNG.randrange(-1000, 1000), RNG.randrange(1, 1000)
            while b % p == 0:
                b += 1
            q = Fraction(a*p**w, b)
            x = canon(p, q, None if j % 2 else c+3+j % 5)
            add(x, [-2, c+1, 9+j % 14])
        for j in range(3):
            a, b = RNG.getrandbits(4096), RNG.getrandbits(3072)
            while a % p == 0:
                a += 1
            while b % p == 0:
                b += 1
            q = Fraction((-1)**j*a*p**c, b)
            add(canon(p, q), [12, 31])
        for j in range(3):
            q = Fraction(p**c*(j+1), 1 if p == 2 else 2)
            assert series(q, p, 27) == series(q, p, 27, True)
    print('oracle_path_cross_checks=21', flush=True)
elif MODE == 'enumerate':
    for p in [2, 3, 5, 7]:
        c = 2 if p == 2 else 1
        for M in [c, c+1]:
            for a in range(0, p**M, p**c):
                for f in range(4):
                    E = 2*M-(p == 2) if f % 2 and a == 0 else M
                    for N in [E-1, E, E+1]:
                        cases.append((f, canon(p, a, M), N))
elif MODE == 'limits':
    for p in PRIMES:
        c = 2 if p == 2 else 1
        for M in range(-2, c+3):
            add(canon(p, 0, M), [-B-1, -1, c, 15, B+1])
        for w in [-B-1, -B, -1, 0, c-1, c, B-1, B, B+1]:
            add((p, 1, 1, w, 0, 1), [-2**63, c, B, B+1, 2**63-1])
        for M in [B//2, B//2+1, B, B+1]:
            add((p, 0, 1, 0, M, 0), [B, B+1, 2**63-1])
        add(canon(p, 0), [-2**63, 2**63-1])
        # K itself is within the power limit, while W exceeds it at p=2,3.
        add((p, 1, 1, c, 0, 1), [BITS//p.bit_length()+1])
        if p in [2, 3]:
            add((p, 1, 1, c, 0, 1), [BITS//3])
elif MODE == 'large':
    for p in [2, 3, 5, 7]:
        c = 2 if p == 2 else 1
        add(canon(p, Fraction(p**c, 3 if p == 2 else 2)), [2000])
        add(canon(p, -2*p**c, 2002), [2000])
elif MODE == 'regression':
    for p in PRIMES:
        for j in range(48):
            a, b = RNG.randrange(-5000, 5000), RNG.randrange(1, 2000)
            if j % 3 == 0:
                a *= p**(2 if p == 2 else 1)
            if j % 3 == 1:
                a = b+p*(1+j)
            q = Fraction(a, b)
            if j % 7 == 0:
                q /= p**(1+j % 4)
            x = canon(p, q, None if j % 2 else j % 17-3)
            add(x, [j % 33-5], range(4, 7))
else:
    raise SystemExit('unknown mode')

lines = [' '.join(map(str, (f, *x, N))) for f, x, N in cases]
(ROOT/f'{MODE}.in').write_text('\n'.join(lines)+'\n')
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0', OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1')
run = subprocess.run(['timeout', '50', str(ROOT/'build/probe')], input='\n'.join(lines)+'\n',
                     text=True, capture_output=True, env=env, timeout=55)
(ROOT/f'{MODE}.out').write_text(run.stdout)
(ROOT/f'{MODE}.stderr').write_text(run.stderr)
print(run.stderr.strip(), flush=True)
if run.returncode:
    raise SystemExit(f'probe exit {run.returncode}')
outputs = run.stdout.splitlines()
assert len(outputs) == len(cases)
fails = []
points, hulls = 0, 0
for index, ((f, x, N), out) in enumerate(zip(cases, outputs)):
    s, p, u, v, K, ex = out.split()
    got = (int(p), Fraction(u), int(v), int(K), int(ex))
    st = int(s)
    if MODE != 'regression':
        want_st, want = expected(f, x, N)
        if st != want_st or st == 0 and got != want:
            fails.append(dict(row=index+1, input=lines[index], got=out, want=str((want_st, want))))
    if MODE == 'enumerate' and st == 0:
        p, un, ud, w, M, exact = x
        c = 2 if p == 2 else 1
        a = un*p**w
        E = 2*M-(p == 2) if f % 2 and not a else M
        H = max(E+1, 2*(c+1)+1)
        image = []
        for t in range(a, p**(c+2), p**M):
            val = series(Fraction(t), p, H)[f]
            image.append(val)
            points += 1
            assert (Fraction(val)-got[1]*p**got[2]).numerator % p**got[3] == 0, (index, t)
        if N >= E and (f % 2 == 0 or a == 0):
            assert any(valuation(t-image[0], p) == E for t in image[1:]), (index, E)
            hulls += 1
(ROOT/f'{MODE}.failures.json').write_text(json.dumps(fails, indent=2)+'\n')
print(f'mode={MODE} cases={len(cases)} failures={len(fails)} point_checks={points} hull_witnesses={hulls}')
if fails:
    print(json.dumps(fails[:4], indent=2))
    raise SystemExit(1)
