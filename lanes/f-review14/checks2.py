#!/usr/bin/env python3
"""f-review14 referee checks against the library (via lanes/f-review14/probe.c).

Sections
  bino : Y9/Y10, the conservative radius N/gcd(N,k!) and the smallest radius over j=1..k,
         compared with the gcd over j=1..k+5 (Prop 14 step 3), signed tops, d>1 domain.
  pow  : Y11/Y12, strict/coarse criterion and D, and the finest modulus F and its CRT centre
         against a brute-force computation prime by prime (image of (c w)^{e+Mt}).
  cyclo: Y14, determined exactly when canon(n)|canon(N), exponent by direct modular inversion.
  sym  : Y1/Y2/Y3, values from an independent definition and the certificate rule.
Usage: python3 -B lanes/f-review14/checks.py bino|pow|cyclo|sym
"""
import subprocess, sys, math
from fractions import Fraction
from random import Random

PROBE = 'lanes/f-review14/build/probe'


def run(lines):
    r = subprocess.run(['timeout', '170', PROBE], input='\n'.join(lines) + '\n',
                       capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    return r.stdout.splitlines()


def parse(line):
    # "<tag> rest..." -> dict of key=value; the tag is dropped
    out = {'kind': line.split()[1] if len(line.split()) > 1 else ''}
    for t in line.split()[2:]:
        if '=' in t:
            k, v = t.split('=', 1)
            out[k] = v
    return out


# ---------------------------------------------------------------- binomial
def binom(a, k):
    """exact integer binom(a,k) for a in Z, k >= 0"""
    n = 1
    for i in range(1, k + 1):
        n = n * (a - i + 1) // i
    return n


def sec_bino():
    rng = Random(11)
    lines, want = [], []
    cases = []
    for a in list(range(-9, 10)) + [10 ** 6, -(10 ** 6), 2 ** 70, -2 ** 70]:
        for h in (0, 1, 2, 3, 6, 12, 30, 210):
            for k in (0, 1, 2, 3, 5, 8):
                cases.append((a, h, k))
    for _ in range(60):
        cases.append((rng.randrange(-10 ** 6, 10 ** 6), rng.choice([1, 2, 3, 4, 5, 7, 8, 11, 16, 24]),
                      rng.randrange(0, 13)))
    for (a, h, k) in cases:
        lines.append('bino 1 %d %d %d 1' % (k, a, h))
        lines.append('bino 0 %d %d %d 1' % (k, a, h))
        want.append((a, h, k))
    out = run(lines)
    assert len(out) == 2 * len(want), (len(out), len(want))
    bad = 0
    for i, (a, h, k) in enumerate(want):
        tight = parse(out[2 * i])
        cons = parse(out[2 * i + 1])
        C = binom(a, k)
        # conservative radius
        want_r = h // math.gcd(h, math.factorial(k)) if h else 0
        if k == 0:
            want_r = 0
        wr = int(C) % want_r if want_r else C
        if int(cons['H']) != want_r or int(cons['A']) != wr or cons['d'] != '1':
            print('BINO conservative', a, h, k, cons, 'want', want_r, wr); bad += 1
        # tight radius: gcd over j = 1..k, checked against j = 1..k+5
        g = 0
        for j in range(1, k + 1):
            g = math.gcd(g, abs(binom(a + h * j, k) - C))
        g2 = g
        for j in range(k + 1, k + 6):
            g2 = math.gcd(g2, abs(binom(a + h * j, k) - C))
        if g != g2:
            print('BINO k values differ from k+5 values', a, h, k, g, g2); bad += 1
        wr = int(C) % g if g else C
        if int(tight['H']) != g or int(tight['A']) != wr:
            print('BINO tight', a, h, k, tight, 'want', g, wr); bad += 1
    print('bino: %d cases x2, %d mismatches' % (len(want), bad))
    # domain rule (Y10): d>1
    lines, out = [], None
    ds = [(1, 2, 2), (0, 1, 2), (1, 3, 2), (2, 2, 2), (3, 2, 3), (4, 4, 4), (5, 6, 4), (0, 0, 2),
          (1, 1, 2), (7, 14, 7), (6, 4, 8), (1, 1, 1), (5, 3, 9)]
    for (a, h, d) in ds:
        lines.append('bino 0 2 %d %d %d' % (a, h, d))
    out = run(lines)
    bad = 0
    for (a, h, d), l in zip(ds, out):
        r = parse(l)
        # the constructor canonicalises: divide by gcd(A,H,d), then A mod H
        g0 = math.gcd(a, h, d) if h else math.gcd(a, d)
        if g0 > 1:
            a, h, d = a // g0, h // g0, d // g0
        g = math.gcd(h, d)
        want = 'OK' if d == 1 else ('ND' if a % g == 0 else 'DOMAIN')
        if r['kind'] != want:
            print('BINO domain', (a, h, d), r['kind'], 'want', want); bad += 1
    print('bino domain: %d triples, %d mismatches' % (len(ds), bad))


# ---------------------------------------------------------------- profinite power
def canon(n):
    return n // 2 if n % 4 == 2 else n


def order(x, m):
    r, k = x % m, 1
    while r != 1:
        r = r * x % m
        k += 1
        assert k < 10 ** 7
    return k


def finest_prime(c, N, e, M, p, kmax):
    """largest k <= kmax with the image of (c w)^{e+Mt} constant mod p^k; and its value."""
    n = 0 if N % p else 0
    n = 0
    t = N
    while t % p == 0:
        t //= p
        n += 1
    prev = None
    kk = 0
    for k in range(1, kmax + 1):
        m = p ** k
        vals = set()
        if n == 0:
            bases = [x for x in range(m) if x % p]     # p does not divide N: all units
        elif n >= k:
            bases = [c % m]
        else:
            bases = [(c * (1 + p ** n * s)) % m for s in range(m // p ** n)]
        for b in bases:
            if math.gcd(b, m) != 1:
                continue
            base = pow(b, e, m) if e >= 0 else pow(pow(b, -1, m), -e, m)
            step = pow(b, M, m) if M >= 0 else pow(pow(b, -1, m), M, m)
            o = order(step, m)
            v = base
            for _ in range(o):
                vals.add(v)
                v = v * step % m
        if len(vals) != 1:
            break
        kk = k
        prev = next(iter(vals))
    return kk, prev


def crt(pairs):
    r, m = 0, 1
    for (res, mod) in pairs:
        g = math.gcd(m, mod)
        # assume coprime moduli
        assert g == 1, (m, mod)
        t = ((res - r) * pow(m, -1, mod)) % mod
        r += m * t
        m *= mod
    return r % m, m


def sec_pow():
    rng = Random(12)
    cases = []
    for N in (1, 2, 3, 5, 7, 8, 9, 10, 12, 15, 16, 21, 24, 25, 35, 45, 63, 2 * 5, 2 * 7, 4 * 3 * 5):
        for c in (1, 2, 5, 7, 11, 13, 19, 23):
            if math.gcd(c, N) != 1:
                continue
            for (e, M) in ((0, 2), (1, 2), (2, 4), (0, 1), (3, 6), (-2, 4), (4, 0), (-3, 0), (5, 5),
                           (2, 3), (6, 4), (1, 1), (0, 4), (7, 14), (2, 2)):
                cases.append((c, N, e, M))
    for _ in range(120):
        N = rng.choice([1, 2, 3, 5, 7, 9, 11, 13, 15, 21, 25, 30, 35, 45, 63, 2 * 3 * 5])
        c = rng.randrange(1, N) if N > 1 else 1
        while N > 1 and math.gcd(c, N) != 1:
            c = rng.randrange(1, N)
        cases.append((c, N, rng.randrange(-8, 9), rng.randrange(0, 13)))
    lines = []
    for (c, N, e, M) in cases:
        lines.append('pp 0 %d %d %d %d 1' % (c, N, e, M))
        lines.append('pp 1 %d %d %d %d 1' % (c, N, e, M))
        lines.append('pp 2 %d %d %d %d 1' % (c, N, e, M))
    out = run(lines)
    bad = 0
    for i, (c, N, e, M) in enumerate(cases):
        st, co, fi = parse(out[3 * i]), parse(out[3 * i + 1]), parse(out[3 * i + 2])
        Nc = canon(N)
        cc = c % Nc if Nc else c
        # ---- strict and coarse (Y11)
        if M == 0 and e == 0:
            D = Nc
            ce = 1
            want_st = 'OK'
            if st['kind'] != 'OK' or int(st['N']) != 0 or int(st['c']) != 1:
                print('POW exact zero', (c, N, e, M), st); bad += 1
            if co['kind'] != 'OK' or int(co['N']) != 0 or int(co['c']) != 1:
                print('POW exact zero coarse', (c, N, e, M), co); bad += 1
            continue
        if M == 0:
            D = Nc
            ce = pow(c, e, Nc) if Nc > 1 else 1
            want_st = 'OK'
        else:
            D = math.gcd(Nc, pow(c, M, Nc) - 1) if Nc > 1 else 1
            want_st = 'OK' if D == Nc else 'ND'
            ce = pow(c, e, D) if D > 1 else 1
        if st['kind'] != want_st:
            print('POW strict', (c, N, e, M), st['kind'], 'want', want_st); bad += 1
        if co['kind'] != 'OK' or int(co['N']) != D or int(co['c']) != (ce % D if D > 1 else 1):
            if co['kind'] != 'OK' or int(co['N']) != canon(D) or int(co['c']) != (ce % canon(D) if canon(D) > 1 else 1):
                print('POW coarse', (c, N, e, M), co, 'want D', D, 'ce', ce); bad += 1
        # ---- finest (Y12): prime by prime
        g = math.gcd(abs(e), M)
        if M == 0 and e == 0:
            continue
        primes = set()
        t = Nc
        while t > 1:
            p = next(q for q in range(2, t + 1) if t % q == 0)
            while t % p == 0:
                t //= p
            primes.add(p)
        for q in range(2, g + 2):
            if all(q % d for d in range(2, q)):
                primes.add(q)
        pairs, F = [], 1
        for p in sorted(primes):
            kmax = 4 + (2 if Nc % p == 0 else 0)
            if Nc % p == 0 and p == 2:
                kmax += 4
            if Nc % p != 0:
                kmax = 7
            kp, rho = finest_prime(c, Nc, e, M, p, kmax)
            if kp:
                pairs.append((rho % p ** kp, p ** kp))
                F *= p ** kp
        if pairs:
            r, m = crt(pairs)
        else:
            r, m = 1, 1
        F = canon(F)
        if fi['kind'] != 'OK' or int(fi['N']) != F or int(fi['c']) != (r % F if F > 1 else 1):
            print('POW fine', (c, N, e, M), 'lib', fi, 'mine c=%d N=%d (g=%d)' % (r, F, g)); bad += 1
    print('pow: %d cases x3, %d mismatches' % (len(cases), bad))


# ---------------------------------------------------------------- cyclotomic
def sec_cyclo():
    rng = Random(13)
    cases = []
    for n in range(1, 40):
        for N in range(0, 40):
            for c in (1, 2, 3, 5, 7, 11, 19, 25, 29, 41):
                if N == 0 and c not in (1, -1):
                    continue
                if N > 0 and (math.gcd(c, N) != 1 or c > N):
                    continue
                cases.append((c, N, n))
    for _ in range(200):
        N = rng.choice([0, 1, 2, 3, 4, 6, 8, 9, 12, 16, 18, 36, 63, 100])
        c = 1 if N == 0 else rng.randrange(1, N + 1)
        while N > 1 and math.gcd(c, N) != 1:
            c = rng.randrange(1, N + 1)
        cases.append((c, N, rng.randrange(-3, 40)))
    lines = []
    for (c, N, n) in cases:
        lines.append('cy 0 %d %d %d' % (c, N, n))
        lines.append('cy 1 %d %d %d' % (c, N, n))
    out = run(lines)
    bad = 0
    for i, (c, N, n) in enumerate(cases):
        if n < 1:
            continue
        det = (N == 0) or (canon(n) != 0 and canon(N) % canon(n) == 0 and canon(n) != 0)
        det = (N == 0) or (canon(n) == 1) or (canon(N) % canon(n) == 0)
        for inv in (0, 1):
            r = parse(out[2 * i + inv])
            want = 'OK' if det else 'ND'
            if r['kind'] != want:
                print('CYC status', (c, N, n, inv), r['kind'], 'want', want); bad += 1
                continue
            if not det:
                continue
            # exponent: the residue c mod n, odd lift when n = 2m
            j = c % n if n > 1 else 0
            if n % 4 == 2 and j % 2 == 0:
                j += n // 2
            if inv:
                j = pow(j, -1, n) if n > 1 else 0
            if int(r['j']) != j:
                print('CYC value', (c, N, n, inv), r['j'], 'want', j); bad += 1
    print('cyclo: %d cases x2, %d mismatches' % (len(cases), bad))


# ---------------------------------------------------------------- residue symbols
def leg(a, p):
    r = pow(a % p, (p - 1) // 2, p)
    return 0 if a % p == 0 else (1 if r == 1 else -1)


def kronecker(a, b):
    if b == 0:
        return int(abs(a) == 1)
    s = -1 if (b < 0 and a < 0) else 1
    b = abs(b)
    h = 0
    while b % 2 == 0:
        b //= 2
        h += 1
    two = 0 if a % 2 == 0 else 1 - 2 * (((a * a - 1) // 8) % 2)
    j = 1
    for (p, e) in factors(b):
        j *= leg(a, p) ** e
    return s * two ** h * j


def factors(n):
    out, q = [], 2
    while q * q <= n:
        e = 0
        while n % q == 0:
            n //= q
            e += 1
        if e:
            out.append((q, e))
        q += 1
    if n > 1:
        out.append((n, 1))
    return out


def sec_sym():
    rng = Random(14)
    cases = []
    for p in (3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 65537):
        for a in list(range(-2 * p, 2 * p + 1, max(1, p // 7))) + [rng.randrange(-p * p, p * p)]:
            cases.append((0, 0, a, p, 0, 1))
    for _ in range(700):
        cases.append((2, 0, rng.randrange(-10 ** 5, 10 ** 5), rng.randrange(-60, 61), 0, 1))
    for _ in range(400):
        cases.append((1, 0, rng.randrange(-10 ** 5, 10 ** 5), rng.randrange(1, 100, 2), 0, 1))
    for kind in (0, 1, 2):
        for b in ((3, 5, 7, 11, 13, 17, 19, 23, 29, 31) if kind == 0
                  else (3, 9, 2, 4, 8, 24, 12, 5, 25, 6, 10, 14, 16, 20, 40, 50, 100)):
            if kind == 1 and (b % 2 == 0 or b < 0):
                continue
            for N in (1, 2, 3, 4, 8, 9, 12, 16, 24, 27, 48, 5):
                for a in range(0, N):
                    cases.append((kind, 1, a, b, N, 1))
                    if N > 1 and math.gcd(a, N) == 1:
                        cases.append((kind, 2, a, b, N, 1))
    lines = ['sym %d %d %d %d %d %d' % t for t in cases]
    out = run(lines)
    bad = 0
    for i, (kind, inp, a, b, N, d) in enumerate(cases):
        r = parse(out[i])
        if kind == 1 and (b <= 0 or b % 2 == 0):
            want, wv = 'DOMAIN', 99
        elif inp and N and kind == 2 and b <= 0:
            want, wv = 'ND', 99
        elif inp and N and not Kdiv(kind, b, N):
            want, wv = 'ND', 99
        else:
            want, wv = 'OK', kronecker(a, b) if kind != 1 else jacobi(a, b)
        if r['kind'] != want or (want == 'OK' and int(r['z']) != wv):
            print('SYM', (kind, inp, a, b, N, d), r, 'want', want, wv); bad += 1
        if kind == 0 and r['kind'] != 'OK' and r['where'] != '1':
            print('SYM where', (kind, inp, a, b, N, d), r); bad += 1
        if kind != 0 and r['where'] != '0':
            print('SYM where untouched', (kind, inp, a, b, N, d), r); bad += 1
    print('sym: %d cases, %d mismatches' % (len(cases), bad))


def jacobi(a, b):
    j = 1
    for (p, e) in factors(b):
        j *= leg(a, p) ** e
    return j


def Kdiv(kind, b, N):
    if kind == 2 and b % 2 == 0:
        m = b
        while m % 2 == 0:
            m //= 2
        K = 8 * m // math.gcd(m, 1)
        K = 8 * m
        K = math.lcm(m, 8)
    else:
        K = b
    return N % K == 0


if __name__ == '__main__':
    for a in sys.argv[1:] or ['bino', 'pow', 'cyclo', 'sym']:
        {'bino': sec_bino, 'pow': sec_pow, 'cyclo': sec_cyclo, 'sym': sec_sym}[a]()

def sec_prop2():
    """Y2 / Proposition 2 by brute force: if the sufficient modulus K divides N then every
    residue of the ball mod K gives one symbol; and K need not be necessary."""
    from math import gcd, lcm
    import itertools

    def leg(a, p):
        r = pow(a % p, (p - 1) // 2, p)
        return 0 if a % p == 0 else (1 if r == 1 else -1)

    def fac(n):
        out, q = [], 2
        while q * q <= n:
            e = 0
            while n % q == 0:
                n //= q
                e += 1
            if e:
                out.append((q, e))
            q += 1
        if n > 1:
            out.append((n, 1))
        return out

    def kr(a, b):
        if b == 0:
            return int(abs(a) == 1)
        s = -1 if (b < 0 and a < 0) else 1
        b = abs(b)
        h = 0
        while b % 2 == 0:
            b //= 2
            h += 1
        two = 0 if a % 2 == 0 else 1 - 2 * (((a * a - 1) // 8) % 2)
        j = 1
        for (p, e) in fac(b):
            j *= leg(a, p) ** e
        return s * two ** h * j

    cases = determined = 0
    bad = 0
    for b in list(range(-30, 0)) + list(range(0, 61)):
        if b <= 0:
            K = 0
        elif b % 2:
            K = b
        else:
            m = b
            while m % 2 == 0:
                m //= 2
            K = lcm(m, 8)
        for N in ([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 16, 18, 20, 24, 30, 32, 36, 40, 45, 48]
                  + ([K] if K else [])):
            if K and N % K == 0:
                for a in range(0, N):
                    vals = set(kr(a + t * N, b) for t in range(K))
                    cases += 1
                    if len(vals) != 1:
                        print('PROP2 not constant', b, N, a, vals); bad += 1
            elif b > 0:
                # K does not divide N: check whether the ball is nonetheless determined
                seen = set(kr(a + t * N, b) for t in range(lcm(N, 8 * b + 1) if N else 1))
                if len(seen) == 1:
                    determined += 1
    print('prop2: %d certified triples constant mod K, %d mismatches; '
          '%d uncertified balls that are nevertheless determined' % (cases, bad, determined))


def sec_mod8():
    """Y6 step 8: (2,6) has a primitive solution modulo 8 but none modulo 16."""
    for M in (8, 16):
        sols = []
        for x in range(M):
            for y in range(M):
                for z in range(M):
                    if (2 * x * x + 6 * y * y - z * z) % M == 0 and (x % 2 or y % 2 or z % 2):
                        sols.append((x, y, z))
        print('mod', M, 'primitive solutions of 2x^2+6y^2=z^2:', len(sols), sols[:4])
