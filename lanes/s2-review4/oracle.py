#!/usr/bin/env python3
"""Independent integer arithmetic oracles. No author reference or FLINT arithmetic in the oracle."""
import collections
import math
import os
import random
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
RNG = random.Random(294)
COUNTS = collections.Counter()


def probable(n):
    # A candidate filter only. probe.c proves each selected prime with adf_place_prime.
    if n < 2:
        return False
    d, s = n - 1, 0
    while d % 2 == 0:
        d //= 2
        s += 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n == a:
            return True
        if n % a == 0:
            return False
        x = pow(a, d, n)
        for _ in range(s):
            if x == n - 1 or x == 1 and _ == 0:
                break
            x = x * x % n
        else:
            return False
    return True


def below(n):
    n = (n - 1) | 1
    while not probable(n):
        n -= 2
    return n


def mul(a, b):
    c = [0] * (len(a) + len(b) - 1)
    for i, x in enumerate(a):
        for j, y in enumerate(b):
            c[i + j] += x * y
    return c


def prod(roots):
    f = [1]
    for r in roots:
        f = mul(f, [-r, 1])
    return f


def evaluate(f, a, modulus=None):
    v = 0
    for c in reversed(f):
        v = v * a + c
        if modulus:
            v %= modulus
    return v


def derivative(f):
    return [i * c for i, c in enumerate(f)][1:]


def nonresidue(p):
    a = 2
    while pow(a, (p - 1) // 2, p) != p - 1:
        a += 1
    return a


def lift(f, r, p, k):
    # One digit at a time: Taylor's terms of order >= 2 vanish modulo p^(e+1).
    a, pe, df = r, p, derivative(f)
    for _ in range(1, k):
        value = evaluate(f, a)
        assert value % pe == 0
        digit = -(value // pe) * pow(evaluate(df, a, p), -1, p) % p
        a += pe * digit
        pe *= p
        assert evaluate(f, a, pe) == 0
    return a


class Bridge:
    def __init__(self):
        self.p = subprocess.Popen(['timeout', '170', os.path.join(HERE, 'probe')],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)

    def run(self, mode, f, p, prec, depth):
        self.p.stdin.write(f'{mode} {p} {prec} {depth} {len(f)} ' + ' '.join(map(str, f)) + '\n')
        self.p.stdin.flush()
        line = self.p.stdout.readline()
        assert line, ('probe stopped', self.p.poll(), mode, p, len(f), prec, depth)
        return list(map(int, line.split()[1:]))

    def close(self):
        self.p.stdin.close()
        assert self.p.wait(timeout=5) == 0


def read_list(data):
    complete, n, nu, canonical, entries, vc = data[:6]
    reduced = data[6]
    cert = [tuple(data[7 + 3*i:10 + 3*i]) for i in range(n)]
    start = 7 + 3*n
    unres = [tuple(data[start + 2*i:start + 2*i + 2]) for i in range(nu)]
    return (complete, canonical, entries, vc, cert, unres, reduced), data[start + 2*nu:]


def check_padic(br, f, p, prec, depth, roots, t=0, s_values=None, label='hensel', reduced=None):
    data = br.run('P', f, p, prec, depth)
    COUNTS['padic_inputs'] += 1
    st, sp, untouched = data[:3]
    COUNTS[f'status_{st}'] += 1
    if prec < 1 or depth < 0 or not any(f):
        assert (st, sp, untouched) == (7, 7, 1), (label, data)
        return
    if prec > (1 << 24) // (2 * p.bit_length()):
        assert (st, sp, untouched) == (10, 10, 1), (label, data)
        return
    complete = depth >= t
    assert (st, sp, untouched) == (0 if complete else 1, 0, 1), (label, p, data[:3])
    partial, tail = read_list(data[3:])
    assert partial[:4] == (int(complete), 1, 1, int(complete)), (label, p, partial)
    cert, unres = partial[4:6]
    if reduced is not None:
        assert partial[6] == reduced, (label, p, partial[6], reduced)
    assert cert == sorted(cert) and unres == sorted(unres)
    for root in roots:
        coverage = sum((root - a) % p**k == 0 for a, k, s in cert)
        coverage += sum((root - a) % p**e == 0 for a, e in unres)
        assert coverage == 1, (label, p, root, partial)
    if complete:
        sv = s_values or [0] * len(roots)
        want = sorted((a % p**max(prec, s + 1), max(prec, s + 1), s) for a, s in zip(roots, sv))
        assert cert == want and not unres, (label, p, cert, want)
        strict, tail = read_list(tail)
        assert strict == partial
        COUNTS['certificates'] += len(cert)
    else:
        assert all(sum((r - a) % p**k == 0 for r in roots) == 1 for a, k, s in cert)
        assert all(any((r - a) % p**e == 0 for r in roots) for a, e in unres)
        COUNTS['unresolved_classes'] += len(unres)
    assert not tail


def roots_of_unity(p, k):
    d = math.gcd(k, p - 1)
    factors = [q for q in range(2, d + 1) if d % q == 0 and all(q % j for j in range(2, q))]
    for g in range(1, 10000):
        w = pow(g, (p - 1) // d, p)
        if all(pow(w, d // q, p) != 1 for q in factors):
            roots = sorted(pow(w, j, p) for j in range(d))
            assert len(set(roots)) == d and all(pow(x, k, p) == 1 for x in roots)
            return roots
    raise AssertionError('could not construct unity roots')


def main():
    br = Bridge()
    primes = sorted(set([131, 137, 251, 65521, 1048583, 4294967291, 18446744073709551557]
                        + [below(2**b) for b in range(33, 65)] + [below(2**63 + 100)]))
    COUNTS['primes'] = len(primes)
    for p in primes:
        nr = nonresidue(p)
        for n in (0, 1, 2, 8, 17, 32):
            planted = [0, p - 1, (p + 1) // 2][:n]
            while len(planted) < n:
                x = RNG.randrange(p)
                if x not in planted:
                    planted.append(x)
            f = prod(planted)
            # Repetitions and irreducible factors do not add roots.
            if n > 1:
                f = mul(f, prod(planted[:2]))
            f = mul(f, [-nr, 0, 1])
            lead = -1 if n % 2 else p - 1
            f = [lead*c + p*RNG.randrange(-2**4096, 2**4096) for c in f]
            f += [p * 2**4096, -p * (2**4096 + 1)]
            data = br.run('M', f, p, 0, 0)
            assert data == [n] + sorted(planted), ('modp', p, n, data, planted)
            COUNTS['modp_products'] += 1
        # Independent simple Hensel lifts; the integer coefficients are p-perturbed.
        for n in (0, 1, 2, 5):
            residues = list(range(n - 1)) + [p - 1] if n else []
            f = mul(prod(residues), [-nr, 0, 1])
            f = [c + p*RNG.randrange(-2**512, 2**512) for c in f]
            f += [p * (2**1024 + 17)]
            prec = 5
            lifted = [lift(f, r, p, prec) for r in residues]
            check_padic(br, f, p, prec, 0, lifted)
            COUNTS['noninteger_hensel_inputs'] += 1
        # Square residues and nonresidues use Python pow and own digit lifts.
        for square in (False, True):
            a = (p - 3)**2 if square else nr
            f = [-a, 0, 1]
            residues = [3, p - 3] if square else []
            lifted = [lift(f, r, p, 7) for r in residues]
            check_padic(br, f, p, 7, 0, lifted, label='quadratic')
            COUNTS['quadratics'] += 1
        # Reduction of degree 0 and 1, nonmonic, large signed coefficients.
        for h in ([1], [-2, p - 1]):
            f = [c + p*(-2**4096 + i) for i, c in enumerate(h)] + [p, -p, p*2**4096]
            residues = [] if len(h) == 1 else [(-h[0] * pow(h[1], -1, p)) % p]
            data = br.run('M', f, p, 0, 0)
            assert data == [len(residues)] + residues
            COUNTS['degree_drop'] += 1
            lifted = [lift(f, r, p, 3) for r in residues]
            check_padic(br, f, p, 3, 0, lifted, label='degree drop')
        for k in (3, 8, 30):
            f = [-1] + [0] * (k - 1) + [1]
            roots = roots_of_unity(p, k)
            data = br.run('M', f, p, 0, 0)
            assert data == [len(roots)] + roots, ('unity', p, k, data, roots)
            COUNTS['unity_polynomials'] += 1
        # Several close pairs, including residues p-1. More than the author's single pair.
        roots = [-1, -1 + p**3, 2, 2 - p**2, 5]
        f = prod(roots)
        s_values = [3, 3, 2, 2, 0]
        for depth in (0, 1, 2, 3, 4):
            check_padic(br, f, p, 1, depth, roots, 3, s_values, 'close pairs')
        # Repeated factors, large negative content, and a nonintegral root 1/p.
        roots = [-1, 0, p**2 - 1, p - 2]
        f = mul(mul(prod(roots + roots[:1]), [-1, p]), [-nr, 0, 1])
        f = [-p**3 * 2**2048 * c for c in f]
        for depth in (1, 2):
            check_padic(br, f, p, 2, depth, roots, 2, [2, 0, 2, 0], 'content and reduction', 1)
            COUNTS['content_and_repetition'] += 1
        for f, prec, depth in (([0], 3, 0), ([1, 1], 0, 0), ([1, 1], 3, -1),
                               ([1, 1], (1 << 24) // (2*p.bit_length()) + 1, 0)):
            check_padic(br, f, p, prec, depth, [])
    # Degree beyond 1000, both high root count and high irreducible degree.
    for p in (131, 1048583, 18446744073709551557):
        f = [-1] + [0] * 1007 + [1]
        roots = roots_of_unity(p, 1008)
        data = br.run('M', f, p, 0, 0)
        assert data == [len(roots)] + roots
        COUNTS['degree_over_1000'] += 1
    p = 18446744073709551557
    planted = list(range(1001))
    f = prod(planted)
    data = br.run('M', f, p, 0, 0)
    assert data == [1001] + planted
    COUNTS['degree_over_1000'] += 1
    br.close()
    print('oracle:', dict(sorted(COUNTS.items())), '; failures=0')


if __name__ == '__main__':
    if hasattr(sys, 'set_int_max_str_digits'):
        sys.set_int_max_str_digits(0)
    main()
