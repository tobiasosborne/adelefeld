#!/usr/bin/env python3
"""Independent review checks. Standard library only; no author functions are imported.

Run from the repository root. Exit 1 means a literal reviewed claim was refuted.
An unexpected assertion is a checker failure. Finite computations do not prove topology.
The comparisons below are derived from finite residue sets, exact fractions, and local
valuations. Formula predictions are kept separate from those comparisons.
"""

from collections import Counter
from fractions import Fraction as F
from itertools import product
from math import ceil, floor, gcd, lcm, prod
import random
import sys


COUNTS = Counter()
REFUTATIONS = []
RNG = random.Random(912731)


def test(group, condition):
    COUNTS[group] += 1
    assert condition, (group, COUNTS[group])


def refute(label, literal_claim, witness):
    assert not literal_claim, (label, "counterexample did not refute claim")
    REFUTATIONS.append((label, witness))


def rgcd(*xs):
    den = lcm(*(F(x).denominator for x in xs))
    return F(gcd(*(int(F(x) * den) for x in xs)), den)


def integral(x):
    return F(x).denominator == 1


def factors(n):
    n = abs(n)
    out = []
    p = 2
    while p * p <= n:
        if n % p == 0:
            out.append(p)
            while n % p == 0:
                n //= p
        p += 1
    if n > 1:
        out.append(n)
    return out


def val(x, p):
    x = F(x)
    if not x:
        return float('inf')
    a, b, v = abs(x.numerator), x.denominator, 0
    while a % p == 0:
        a //= p
        v += 1
    while b % p == 0:
        b //= p
        v -= 1
    return v


def inside_point(x, ball):
    a, r = map(F, ball)
    if not r:
        return x == a
    delta = F(x) - a
    if not delta:
        return True
    return (delta/r).denominator == 1


def subset(x, y):
    a, r = map(F, x)
    b, s = map(F, y)
    if not s:
        return r == 0 and a == b
    # Rational witnesses a and a+r suffice: all their Zhat combinations stay in y.
    return inside_point(a, y) and inside_point(a + r, y)


def same(x, y):
    return subset(x, y) and subset(y, x)


def image(ball, den, modulus):
    a, r = ball
    base, step = int(a * den), int(r * den)
    assert integral(a * den) and integral(r * den) and step > 0
    assert modulus % step == 0
    return {i for i in range(modulus) if (i - base) % step == 0}


def point_hull(points):
    anchor = points[0]
    return anchor, rgcd(*(x - anchor for x in points))


def raw_op(x, y, multiply=False):
    a, r = x
    b, s = y
    if multiply:
        return a * b, rgcd(a * s, b * r, r * s)
    return a + b, rgcd(r, s)


def scaled(ball, k):
    a, r = ball
    if not r:
        return ball
    scale = rgcd(a, r / k)
    return scale * (int(a / scale) % k), scale * k


def regressions():
    # P24: the set still has its original local representation (2; [2 mod 4]).
    original = (F(2, 2), F(4, 2))
    cancelled = (F(1), F(2))
    refute('policies P24', not same(original, cancelled),
           '(A,H,d)=(2,4,2), g=2: both old and cancelled data give 1+2 Zhat')
    # P25: reducing the fraction first gives 1 mod 4 despite gcd(2,4)>1.
    solutions = [i for i in range(4) if (2 * i - 2) % 4 == 0]
    refute('policies P25', not solutions and not integral(F(2, 2)),
           'A=2,d=2,q=4: A/d=1 reduces mod 4; 2*x=2 has solutions [1,3]')
    test('proof_repairs', F(4 - 1, 3) == (F(4 * 3, 3) - 1) / 3)
    test('proof_repairs', F(4 - 1, 3) != F(4 * 3, 3) - 1)
    test('proof_repairs', raw_op((F(0), F(2)), (F(0), F(2)), True)[1] == 4)
    # A sum of canonical inputs can change canonical H: (1+2 Zhat)/2 twice.
    test('proof_repairs', raw_op((F(1, 2), F(1)), (F(1, 2), F(1))) == (1, 1))
    test('proof_repairs', raw_op((F(1, 2), F(3)), (F(2, 3), F(2)), True) == (F(1, 3), 1))


def policies():
    centers = list(map(F, (-3, -1, 0, 1, 2))) + [F(-1, 2), F(2, 3)]
    radii = list(map(F, (0, 1, 2, 6))) + [F(1, 2), F(2, 3)]
    balls = list(product(centers, radii))
    for x, y in product(balls, repeat=2):
        for multiply in (False, True):
            pts = [(x[0] + i*x[1]) * (y[0] + j*y[1]) if multiply
                   else x[0] + i*x[1] + y[0] + j*y[1]
                   for i, j in product(range(-1, 3), repeat=2)]
            predicted = raw_op(x, y, multiply)
            test('policies_L1_L2', same(predicted, point_hull(pts)))
            coarse = [(z[0] + F(1, 6), F(1, 6)) for z in (x, y)]
            if subset(x, coarse[0]) and subset(y, coarse[1]):
                test('policies_L1_L2', subset(predicted, raw_op(*coarse, multiply)))
    scales = sorted({F(n, d) for n in range(1, 7) for d in range(1, 7)})
    for k in (1, 2, 3, 4, 6, 10):
        values = [(s*u, s*k, s, u) for s in scales[:6] for u in range(k)]
        signatures = set()
        for a, r, s, u in values:
            test('policies_L5_L6_P7', (r, a % r) not in signatures)
            signatures.add((r, a % r))
        for a, r in balls:
            if not r:
                continue
            best = scaled((a, r), k)
            test('policies_L5_L6_P7', subset((a, r), best))
            test('policies_L5_L6_P7', same((a, r), best) == integral(a*k/r))
            for s in scales:
                if integral(a/s):
                    candidate = (s*(int(a/s) % k), s*k)
                    if subset((a, r), candidate):
                        test('policies_L5_L6_P7', subset(best, candidate))
        for a, r, s, u in values:
            for q in centers:
                g = rgcd(s, q)
                out = (g * (int((a+q)/g) % k), g*k)
                test('policies_P8_P9', same(out, scaled((a+q, r), k)))
                test('policies_P8_P9', same(out, (a+q, r)) == integral(q/s))
                mul = (q*a, abs(q)*r)
                test('policies_P8_P9', same(mul, point_hull([q*(a+i*r) for i in (-1, 0, 1)])))
            for t, v in ((F(1, 2), 0), (F(2), k-1), (F(3, 2), u)):
                h = gcd(u, v, k)
                tight = point_hull([(a+i*r)*(t*v+j*t*k) for i, j in product((0, 1), repeat=2)])
                variant = (s*t*h*((u*v//h) % k), s*t*h*k)
                test('policies_P10', same(tight, variant))
                test('policies_P10', subset(tight, (s*t*((u*v) % k), s*t*k)))
            for k2 in (1, 2, 3, 6, 12):
                s2 = s*rgcd(u, F(k, k2))
                out = (s2*(int(a/s2) % k2), s2*k2)
                test('policies_P11_C12', same(out, scaled((a, r), k2)))
                test('policies_P11_C12', same(out, (a, r)) == integral(F(u*k2, k)))
                common = lcm(k, k2)
                test('policies_P11_C12', same(scaled((a, r), common), (a, r)))
    for ball, cap in product(balls, (F(1, 2), F(1), F(2), F(6))):
        a, r = ball
        out = (a, rgcd(r, cap))
        test('policies_P14_P15', subset(ball, out) and integral(cap/out[1]))
        test('policies_P14_P15', rgcd(out[1], cap) == out[1])
        for j in range(1, 13):
            candidate = (a, cap/j)
            if subset(ball, candidate):
                test('policies_P14_P15', subset(out, candidate))
    for _ in range(100):
        k = RNG.choice((1, 2, 6, 10))
        cap = RNG.choice((F(1, 2), F(2), F(6)))
        x, y = RNG.choices(balls, k=2)
        items = [(a+r, (a, r), scaled((a, r), k), (a, rgcd(r, cap))) for a, r in (x, y)]
        for _ in range(6):
            left, right = RNG.choices(items, k=2)
            multiply = RNG.choice((False, True))
            exact = left[0]*right[0] if multiply else left[0]+right[0]
            tight = raw_op(left[1], right[1], multiply)
            if multiply and left[2][1] and right[2][1]:
                sb = (left[2][0]*right[2][0], left[2][1]*right[2][1]/k)
            else:
                sb = scaled(raw_op(left[2], right[2], multiply), k)
            cb = raw_op(left[3], right[3], multiply)
            cb = cb[0], rgcd(cb[1], cap)
            test('policies_T3', inside_point(exact, tight) and subset(tight, sb) and subset(tight, cb))
            items.append((exact, tight, sb, cb))


def backend():
    contexts = ((1,), (2,), (4,), (6,), (1, 1), (1, 2), (4, 9), (6, 5), (8, 3))
    for _ in range(750):
        blocks = RNG.choice(contexts)
        htot = prod(blocks)
        a, b = RNG.randrange(htot), RNG.randrange(htot)
        d, e = RNG.choice((1, 2, 3, 4, 6, 12)), RNG.choice((1, 2, 3, 4, 6, 12))
        rs, ss = [a % q for q in blocks], [b % q for q in blocks]
        solutions = [i for i in range(htot) if all(i % q == r for q, r in zip(blocks, rs))]
        test('backend_L17_L18', solutions == [a])
        test('backend_L17_L18', gcd(a, htot) == prod(gcd(r, q) for r, q in zip(rs, blocks)))
        x, y = (F(a, d), F(htot, d)), (F(b, e), F(htot, e))
        test('backend_L17_L18', same(x, (F(a+htot, d), F(htot, d))))
        den = lcm(d, e)
        num = a*(den//d)+b*(den//e)
        test('backend_P21', same(raw_op(x, y), (F(num, den), F(htot, den))))
        test('backend_P21', all(num % q == (r*(den//d)+s*(den//e)) % q
                              for r, s, q in zip(rs, ss, blocks)))
        test('backend_P21', same(point_hull([-x[0], -x[0]-x[1]]), (F((-a) % htot, d), x[1])))
        hi = [gcd(r, s, q) for r, s, q in zip(rs, ss, blocks)]
        h = prod(hi)
        tight = point_hull([(x[0]+i*x[1])*(y[0]+j*y[1]) for i, j in product((0, 1), repeat=2)])
        test('backend_P22', same(tight, (F(a*b, d*e), F(htot*h, d*e))))
        test('backend_P22', integral(F(d*e, h)) == integral(htot/tight[1]))
        for r, s, q, hh in zip(rs, ss, blocks, hi):
            test('backend_P22', (a*b-r*s) % (q*hh) == 0)
            if (d*e) % h == 0:
                residue = ((r*s % (q*hh))//hh)*pow(h//hh, -1, q) % q
                test('backend_P22', residue == (a*b//h) % q)
        for scalar in (F(-3, 2), F(0), F(2, 3), F(1, 2), F(6)):
            if scalar:
                m, n = scalar.numerator, scalar.denominator
                radius = abs(scalar)*x[1]
                test('backend_P23', integral(htot/radius) == (d % abs(m) == 0))
                test('backend_P23', same(point_hull([scalar*x[0], scalar*(x[0]+x[1])]),
                                        (scalar*x[0], radius)))
                if d % abs(m) == 0:
                    dd = n*d//abs(m)
                    aa = (1 if m > 0 else -1)*a
                    test('backend_P23', same((F(aa, dd), F(htot, dd)), (scalar*x[0], radius)))
            else:
                test('backend_P23', point_hull([scalar*x[0], scalar*(x[0]+x[1])]) == (0, 0))
        g = gcd(a, htot, d)
        if g > 1:
            for r, q in zip(rs, blocks):
                gi = gcd(g, q)
                residue = (r//gi)*pow(g//gi, -1, q//gi) % (q//gi)
                test('backend_P24_repaired', residue == (a//g) % (q//gi))
            test('backend_P24_repaired', same(x, (F(a//g, d//g), F(htot//g, d//g))))
        c = F(RNG.randrange(-6, 7), RNG.choice((1, 2, 3, 6)))
        radius = F(RNG.randrange(1, 7), RNG.choice((1, 2, 3, 6)))
        d0 = lcm((htot/radius).numerator, c.denominator)
        best = (c, F(htot, d0))
        test('backend_P19_P20', subset((c, radius), best))
        criterion = integral(htot/radius) and integral(c*htot/radius)
        test('backend_P19_P20', same(best, (c, radius)) == criterion)
        for shift in (-2, 0, 3):
            test('backend_P19_P20', lcm((htot/radius).numerator,
                                      (c+shift*radius).denominator) == d0)
        for dd in range(1, 19):
            for aa in range(htot):
                candidate = (F(aa, dd), F(htot, dd))
                if subset((c, radius), candidate):
                    test('backend_P19_P20', subset(best, candidate))
        if gcd(d, htot) > 1:
            test('backend_P25_repaired', any((d*i-a) % htot == 0 for i in range(htot))
                 == (a % gcd(d, htot) == 0))


def canonical_mod(n):
    return n//2 if n % 4 == 2 else n


def unit_image(c, n, modulus):
    assert modulus % n == 0
    return {x for x in range(modulus) if gcd(x, modulus) == 1 and (x-c) % n == 0}


def ideles():
    primes = [p for p in range(2, 44) if factors(p) == [p]]
    for _ in range(200):
        components = {p: F(RNG.choice((-7, -2, 1, 3, 6)), RNG.choice((1, 2, 3, 5)))
                      for p in (2, 3, 5, 7)}
        r = prod(F(p)**val(x, p) for p, x in components.items())
        test('ideles_L2_P3_P14', all(val(x/r, p) == 0 for p, x in components.items()))
        test('ideles_L2_P3_P14', prod(F(p)**(-val(r, p)) for p in primes) == 1/r)
        for p in (2, 3, 5, 7):
            test('ideles_L2_P3_P14', val(components[p]/(r*p), p) != 0)
        q = F(RNG.choice((-7, -2, 1, 3, 6)), RNG.choice((1, 2, 3, 5)))
        test('ideles_L2_P3_P14', abs(q)*prod(F(p)**(-val(q, p)) for p in primes) == 1)
    cosets = [(c, n) for n in range(1, 13) for c in range(n) if gcd(c, n) == 1]
    for c, n in cosets:
        modulus = lcm(n, 2)*30
        vals = unit_image(c, n, modulus)
        test('ideles_L5_P6_L7', bool(vals))
        test('ideles_L5_P6_L7', vals == unit_image(c, canonical_mod(n), modulus))
        test('ideles_L5_P6_L7', vals < image((F(c), F(n)), 1, modulus))
        additive_radius = gcd(modulus, *(v-min(vals) for v in vals))
        test('ideles_P16', additive_radius == lcm(n, 2))
        for scale in (F(1, 2), F(2, 3), F(6)):
            test('ideles_P16', scale*additive_radius == scale*lcm(n, 2))
        for c2, n2 in cosets:
            mod = lcm(n, n2, 2)*2
            left, right = unit_image(c, n, mod), unit_image(c2, n2, mod)
            test('ideles_P9', (left <= right) ==
                 (canonical_mod(n) % canonical_mod(n2) == 0 and (c-c2) % canonical_mod(n2) == 0))
            test('ideles_P9', (left == right) ==
                 ((c % canonical_mod(n), canonical_mod(n)) ==
                  (c2 % canonical_mod(n2), canonical_mod(n2))))
            test('ideles_P9', bool(left & right) == ((c-c2) % gcd(n, n2) == 0))
            actual = {x*y % mod for x in left for y in right}
            test('ideles_P10_P11', actual == unit_image(c*c2, gcd(n, n2), mod))
            inverses = {pow(x, -1, mod) for x in left}
            ci = pow(c, -1, n)
            test('ideles_P10_P11', inverses == unit_image(ci, n, mod))
            test('ideles_P10_P11', 1 in {x*y % mod for x in left for y in inverses})
    exponents = (-30, -12, -6, -4, -3, -2, -1, 1, 2, 3, 4, 6, 12, 16, 30)
    for p in (2, 3, 5, 7):
        for a in range(2 if p == 2 else 1, 4):
            for k in exponents:
                x = F(1+p**a)
                test('ideles_L12', val(x**k-1, p) == a+val(k, p))
    for n in (1, 2, 3, 4, 6, 8, 10, 12, 18):
        for k in exponents:
            nb = canonical_mod(n)
            predicted_mod = 1
            for p in primes:
                a, e = val(nb, p), val(k, p)
                if p == 2:
                    b = a+e if a >= 2 else (2+e if k % 2 == 0 else 0)
                else:
                    b = a+e if a else (1+e if k % (p-1) == 0 else 0)
                predicted_mod *= p**b
                mod = p**max(a+1, b+1, 2)
                inputs = [x for x in range(1, mod) if x % p and (x-1) % (p**a) == 0]
                outputs = {pow(x, k, mod) for x in inputs}
                observed = gcd(mod, *(x-1 for x in outputs))
                observed_b = val(observed, p)
                if p == 2 and observed_b == 1:
                    observed_b = 0
                test('ideles_P13', observed_b == b)
                # Nontrivial residue and a negative representative of that residue.
                c = -1 if a else 1
                coset_outputs = {pow((c*x) % mod, k, mod) for x in inputs}
                test('ideles_P13', all((y-pow(c, k, mod)) % (p**b) == 0 for y in coset_outputs))
            test('ideles_P13', predicted_mod % nb == 0 and predicted_mod % 4 != 2)
    for n in (1, 2, 3, 4, 6, 10):
        mod = 9*n
        test('ideles_P13_zero', len(unit_image(1, n, mod)) > 1)
        test('ideles_P13_zero', {pow(x, 0, mod) for x in unit_image(1, n, mod)} == {1})
    test('ideles_P13', 97 in unit_image(1, 24, 120))
    test('ideles_P13', 97 not in {x*x % 120 for x in unit_image(0, 1, 120)})
    for xi, r, u, q in product((F(-2), F(1, 2)), (F(2, 3), F(6)), (1, 5, 7, 11), (F(-3, 2), F(2))):
        sign = 1 if xi > 0 else -1
        before = abs(xi)/r, (sign*u) % 24
        after_sign = 1 if xi*q > 0 else -1
        after_u = (1 if q > 0 else -1)*u
        after = abs(xi*q)/(abs(q)*r), (after_sign*after_u) % 24
        test('ideles_P15', before == after)
        test('ideles_P15', xi/(sign*r) == before[0])
    for a, radius in product((F(-3), F(0), F(1, 2), F(2, 3)), (F(1, 2), F(1), F(6))):
        for p in primes[:7]:
            if val(a, p) >= 0 and val(radius, p) == 0:
                test('ideles_P17', val(-a/radius, p) >= 0)
    for _ in range(160):
        a = F(RNG.randrange(-4, 5), RNG.choice((1, 2, 3)))
        radius = F(RNG.randrange(0, 5), RNG.choice((1, 2, 3)))
        c, n = RNG.choice(cosets)
        scale = RNG.choice((F(1, 2), F(2, 3), F(6)))
        den = lcm(a.denominator, radius.denominator)
        aa, rr = int(a*den), int(radius*den)
        ll = lcm(n, 2)
        mod = lcm(ll, rr or 1, abs(aa)*ll or 1)*6
        numerators = image((a, radius), den, mod) if radius else {aa % mod}
        inverses = {pow(x, -1, mod) for x in unit_image(c, n, mod)}
        outputs = {x*y % mod for x in numerators for y in inverses}
        if a == radius == 0:
            test('ideles_P18_P19', outputs == {0} and rgcd(abs(a)*ll, radius) == 0)
            continue
        observed = F(gcd(mod, *(x-min(outputs) for x in outputs)), den)/scale
        predicted = rgcd(abs(a)*ll, radius)/scale
        test('ideles_P18_P19', observed == predicted)
        ci = pow(c, -1, n)
        odd = ci if ci % 2 else ci+n
        test('ideles_P18_P19', all((F(x, den)/scale-a*odd/scale)/predicted ==
             floor((F(x, den)/scale-a*odd/scale)/predicted) for x in outputs))
        for q in (F(-3, 2), F(2, 3)):
            test('ideles_P18_P19', same(point_hull([(a+i*radius)/q for i in (-1, 0, 1)]),
                                      (a/q, radius/abs(q))))
    test('ideles_P18_P19', rgcd(0, 0) == 0)  # Author's random division loop skips this input.


def reduced_pieces(lo, hi, a, radius, half=False):
    if radius == 0:
        centers, modulus = [a], 0
    else:
        centers = [a+i*radius for i in range(radius.denominator)]
        modulus = radius.numerator
    out = []
    for center in centers:
        left, right = lo-center, hi-center
        end = floor(right) if half or left == right else ceil(right)-1
        for j in range(floor(left), end+1):
            out.append((max(left, j)-j, min(right, j+1)-j, -j, modulus))
    return out


def in_pieces(s, w, family):
    return any((lo <= s <= hi and inside_point(w, (m, F(n)))) or
               (s == 0 and hi == 1 and inside_point(w+1, (m, F(n))))
               for lo, hi, m, n in family)


def critical(family):
    ends = sorted({F(0), F(1)} | {x for piece in family for x in piece[:2]})
    return sorted({x for x in ends if x < 1} | {(x+y)/2 for x, y in zip(ends, ends[1:])})


def brute_quotient(s, w, lo, hi, a, radius):
    # If w+q-a is in radius*Zhat and is rational, its denominator divides
    # radius.denominator. Searching this grid includes every possible q.
    den = lcm(a.denominator, radius.denominator)
    return any(inside_point(w+F(j, den), (a, radius))
               for j in range(ceil((lo-s)*den), floor((hi-s)*den)+1))


def quotient():
    for _ in range(100):
        comps = {p: F(RNG.randrange(-9, 10), p**RNG.randrange(0, 4)) for p in (2, 3, 5)}
        shift = sum((x % 1 for x in comps.values()), F(0))
        test('quotient_L1_P2_P3_C4', all(val(x-shift, p) >= 0 for p, x in comps.items()))
        t = F(RNG.randrange(-15, 16), 4)
        n = floor(t-shift)
        test('quotient_L1_P2_P3_C4', 0 <= t-shift-n < 1)
    pts = list(product((F(0), F(1, 3), F(1)), range(-2, 3)))
    for (s, z), (ss, zz) in product(pts, repeat=2):
        same_class = integral(ss-s) and zz-z == ss-s
        glued = (s, z) == (ss, zz) or (s == 0 and ss == 1 and zz == z+1) or (
            ss == 0 and s == 1 and z == zz+1)
        test('quotient_L1_P2_P3_C4', same_class == glued)
    for d in range(1, 13):
        for i in range(-d, d+1):
            x = F(i, d)
            if abs(x) < F(1, 2) and inside_point(x, (0, 1)):
                test('quotient_L1_P2_P3_C4', x == 0)
    for _ in range(160):
        a = F(RNG.randrange(-6, 7), RNG.choice((1, 2, 3)))
        radius = RNG.choice((F(0), F(1), F(2), F(6), F(1, 2), F(2, 3), F(3, 2)))
        lo = F(RNG.randrange(-8, 9), 4)
        hi = lo+RNG.choice((F(0), F(1, 4), F(1), F(2), radius))
        family = reduced_pieces(lo, hi, a, radius)
        half = reduced_pieces(lo, hi, a, radius, half=True)
        shifted = reduced_pieces(lo+F(-2, 3), hi+F(-2, 3), a+F(-2, 3), radius)
        test('quotient_P5_P6_P8_P10', family == shifted)
        expanded = reduced_pieces(lo-F(1, 4), hi+F(1, 4), a, radius)
        residues = range(radius.numerator) if radius else range(-12, 13)
        for s, w in product(critical(family+expanded), residues):
            actual = brute_quotient(s, w, lo, hi, a, radius)
            test('quotient_P5_P6_P8_P10', in_pieces(s, w, family) == actual)
            test('quotient_P5_P6_P8_P10', in_pieces(s, w, half) == actual)
            test('quotient_P5_P6_P8_P10', not actual or in_pieces(s, w, expanded))
        if radius > 0 and radius.denominator == 1:
            k = sum(lo-a < j < hi-a for j in range(floor(lo-a), ceil(hi-a)+1))
            test('quotient_P5_P6_P8_P10', len(family) == (k+1 if lo < hi else 1))
            test('quotient_P5_P6_P8_P10', len(half) == floor(hi-a)-floor(lo-a)+1)
    for n, lo, a in product(range(1, 7), (F(-1), F(-1, 4), F(0), F(1, 2)), (F(0), F(-2, 3))):
        for length in (F(0), n-F(1, 4), F(n), n+F(1, 4)):
            fam = reduced_pieces(lo, lo+length, a, F(n))
            full = all(in_pieces(s, w, fam) for s, w in product(critical(fam), range(n)))
            test('quotient_P7', full == (length >= n))
    for numerator, denominator in product(range(1, 6), range(1, 5)):
        if gcd(numerator, denominator) != 1:
            continue
        radius = F(numerator, denominator)
        a = F(-1, 2)
        den = lcm(2, denominator)
        mod = den*numerator*6
        pieces = [image((a+k*radius, F(numerator)), den, mod) for k in range(denominator)]
        test('quotient_P8_minimal', set().union(*pieces) == image((a, radius), den, mod))
        test('quotient_P8_minimal', sum(map(len, pieces)) == len(set().union(*pieces)))
        for c, r in product((a, F(0), radius), (1, 2, 3, 6)):
            ball = image((c, F(r)), den, mod)
            test('quotient_P8_minimal', sum(bool(ball & p) for p in pieces) <= 1)
    for _ in range(140):
        family = []
        for _ in range(RNG.randrange(0, 5)):
            lo, hi = sorted(RNG.choices([F(0), F(1, 4), F(1, 2), F(1)], k=2))
            family.append((lo, hi, RNG.randrange(-3, 4), RNG.choice((1, 2, 3, 4, 6))))
        modulus = lcm(*(piece[3] for piece in family))
        refined = {m: [] for m in range(modulus)}
        for lo, hi, residue, n in family:
            for m in range(modulus):
                if (m-residue) % n == 0:
                    refined[m].append((lo, hi))
                if hi == 1 and (m+1-residue) % n == 0:
                    refined[m].append((F(0), F(0)))
        for s, w in product(critical(family), range(modulus)):
            test('quotient_P9', in_pieces(s, w, family) ==
                 any(lo <= s <= hi for lo, hi in refined[w]))
    for _ in range(200):
        lo = F(RNG.randrange(-6, 7), 3)
        hi = lo+RNG.choice((F(0), F(1, 2), F(1), F(2)))
        a = F(RNG.randrange(-6, 7), RNG.choice((1, 2, 3)))
        radius = RNG.choice((F(0), F(1), F(2), F(1, 2), F(2, 3)))
        predicted = ({a+radius*k for k in range(ceil((lo-a)/radius), floor((hi-a)/radius)+1)}
                     if radius else ({a} if lo <= a <= hi else set()))
        # Valuation oracle, with all relevant primes derived exactly from the fractions.
        for d in range(1, 13):
            for i in range(ceil(lo*d), floor(hi*d)+1):
                q = F(i, d)
                if radius:
                    ps = set(factors(d*a.denominator*radius.numerator*radius.denominator))
                    member = all(val(q-a, p) >= val(radius, p) for p in ps)
                else:
                    member = q == a
                test('quotient_P11', member == (q in predicted))
        if radius and hi-lo < radius:
            test('quotient_P11', len(predicted) <= 1)
        if radius and hi-lo >= radius:
            test('quotient_P11', len(predicted) >= 1)
    for p in range(2, 200):
        if factors(p) != [p]:
            continue
        have_root = False
        for b in (13, 17, 221):
            roots, modulus = {0}, 1
            for _ in range(12 if p == 2 else 4):
                roots = {x+t*modulus for x in roots for t in range(p)
                         if ((x+t*modulus)**2-b) % (modulus*p) == 0}
                modulus *= p
            have_root |= bool(roots)
        test('quotient_P12', have_root)
    constant = 13*17*221
    for i in range(1, constant+1):
        if constant % i == 0:
            test('quotient_P12', all(i*i != b for b in (13, 17, 221)))
    for m, a_bound, b_bound in product(range(1, 25), range(5), range(1, 6)):
        for c in range(m):
            solutions = {F(n, d) for n in range(-a_bound, a_bound+1) for d in range(1, b_bound+1)
                         if gcd(n, d) == gcd(d, m) == 1 and (n-c*d) % m == 0}
            if 2*a_bound*b_bound < m:
                test('quotient_P13', len(solutions) <= 1)
    test('quotient_P13', (-1-1) % 2 == 0 and (1-1) % 2 == 0)
    test('quotient_P13', (1-5*5) % 6 == 0 and not inside_point(F(1, 5), (F(5), F(6))))


def negative_controls():
    # Each assertion supplies a witness rejected by a wrong formula.
    test('negative_controls', not inside_point(F(3, 2), (F(1, 2), F(2))))  # Fixed radius.
    test('negative_controls', not same((F(4), F(4)), (F(4), F(8))))  # Omitted product h.
    test('negative_controls', 3 % 4 != 1)  # U(2) must not become U(4).
    test('negative_controls', pow(3, 2, 16) != 1)  # Unit-square modulus 16 is too fine.
    test('negative_controls', in_pieces(F(0), -1, [(F(1), F(1), 0, 2)]))  # Boundary shift.
    # Common modulus 6: a modulus-2 piece must also populate residues 2 and 4.
    family = [(F(1, 4), F(1, 2), 0, 2), (F(0), F(0), 0, 3)]
    test('negative_controls', in_pieces(F(1, 3), 2, family))


def main():
    regressions()
    policies()
    backend()
    ideles()
    quotient()
    negative_controls()
    for name, count in sorted(COUNTS.items()):
        print(f'CHECK {name}: {count} assertions passed')
    for name, witness in REFUTATIONS:
        print(f'REFUTED {name}: {witness}')
    print(f'TOTAL assertions={sum(COUNTS.values())}; refuted_claims={len(REFUTATIONS)}')
    return int(bool(REFUTATIONS))


if __name__ == '__main__':
    sys.exit(main())
