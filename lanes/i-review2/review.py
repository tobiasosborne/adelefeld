"""Independent finite enumeration and exact rational endpoint checks; no proto imports."""
import argparse
import itertools
import math
import os
import random
import subprocess
import sys
from fractions import Fraction as Q

sys.set_int_max_str_digits(0)
ROOT = "lanes/i-review2/"


def run(lines):
    p = subprocess.run(["timeout", "150", os.environ.get("ADF_REVIEW_PROBE", ROOT + "probe")],
                       input="".join(lines), text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode:
        raise RuntimeError((p.returncode, p.stderr[-2000:]))
    out = p.stdout.splitlines()
    assert len(out) == len(lines), (len(out), len(lines))
    return [s.split() for s in out]


def normal(n):
    return n // 2 if n % 4 == 2 else n


def primes(bound):
    return [p for p in range(2, bound + 1) if all(p % d for d in range(2, math.isqrt(p) + 1))]


def powers():
    ks = list(range(-12, 13)) + [30, 60, 64, 210, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47]
    cases = [(c, n, k) for n in range(1, 41) for c in range(1, n + 1) if math.gcd(c, n) == 1 for k in ks]
    cases += [(c, 0, k) for c in (-1, 1) for k in ks + [-(1 << 63), (1 << 63) - 1]]
    cases += [(c, n, k) for n in (1, 2, 4, 8, 15, 40) for c in range(1, n + 1)
              if math.gcd(c, n) == 1 for k in (-(1 << 63), (1 << 63) - 1, 897612484786617600,
                                            963761198400, 720720, (1 << 61) - 1)]
    outputs = run([f"uc {c} {n} {k}\n" for c, n, k in cases])
    ps = primes(211)
    levels = []
    for prime in ps:
        level = prime
        while level * prime <= 100000:
            level *= prime
        levels.append(level)
    # Every local image is the projection of the coset in (Z/lcm(N,T))^*. Enumerate that image.
    # The principal local coset has the same difference gcd as any translate by a unit.
    cache = {}
    checks = 0
    residues = 0
    for (c, n, k), row in zip(cases, outputs):
        dc, dn, tc, tn, alias, norm = map(int, row)
        assert alias == norm == 1
        if k == 0 or n == 0:
            e = 1 if k % 2 == 0 or c == 1 else -1
            assert (dc, dn, tc, tn) == (e, 0, e, 0)
            checks += 1
            continue
        nn = normal(n)
        assert dn == nn and dc % nn == pow(c, k, nn)
        for prime, initial in zip(ps, levels):
            T = initial
            while math.lcm(n, T) > 100000:
                T //= prime
            g = math.gcd(n, T)
            key = (g, T, abs(k))
            if key not in cache:
                D = T
                count = 0
                for w in range(1, T, g):
                    if w % prime:
                        D = math.gcd(D, (pow(w, abs(k), T) - 1) % T)
                        count += 1
                cache[key] = normal(D)
                residues += count
            expected = cache[key]
            actual = normal(math.gcd(tn, T))
            assert actual == expected, (c, n, k, T, actual, expected)
            w = c % g or g
            while w % prime == 0:
                w += g
            assert w < T
            assert (pow(w, k, T) - tc) % actual == 0, (c, n, k, T, tc, actual)
            checks += 1
    print(f"powers: cases={len(cases)} local_coset_checks={checks} "
          f"distinct_enumerations={len(cache)} unit_residues={residues} failures=0")


def canon(a, h, d):
    if h:
        a %= h
    g = math.gcd(math.gcd(a, h), d)
    return a // g, h // g, d // g


def maps():
    rng = random.Random(20260930)
    cases = []
    for n in range(1, 41):
        for c in range(1, n + 1):
            if math.gcd(c, n) == 1:
                for _ in range(8):
                    cases.append((rng.randint(-24, 24), rng.choice([0, 1, 2, 4, 6, 8, 12, 18, 24]),
                                  rng.randint(1, 12), c, n, rng.randint(1, 12), rng.randint(1, 12)))
    cases += [(a, h, d, c, 0, rn, rd) for a in (-5, 0, 3) for h in (0, 2, 12)
              for d in (1, 6) for c in (-1, 1) for rn, rd in ((1, 1), (6, 35), (35, 6))]
    rows = run(["map " + " ".join(map(str, case)) + "\n" for case in cases])
    quotients = units = enumerations = 0
    for case, row in zip(cases, rows):
        ai, hi, di, c, n, rn, rd = case
        small = tuple(map(int, row[:3]))
        simple = tuple(map(int, row[3:6]))
        result = tuple(map(int, row[6:9]))
        st, alias = map(int, row[9:11])
        mid, rad = map(Q, row[11:13])
        assert st == 0 and alias == 1
        # Numerator [-5/2,5/2], negative divisor [-13/4,-11/4].
        assert mid - rad <= Q(-10, 11) and mid + rad >= Q(10, 11)
        rnq = Q(rn, rd)
        if n == 0:
            assert small == simple == canon(rnq.numerator * c, 0, rnq.denominator)
            assert result == canon(ai * c * rd, hi * rd, di * rn)
            continue
        L = math.lcm(n, 2)
        ce = c if c % 2 else c + n
        nn = normal(n)
        cc = c % nn
        assert small == canon(rn * ce, rn * L, rd)
        assert simple == canon(rn * cc, rn * nn, rd)
        # Own enumeration: integer numerators of ((ai+hi*z)/di)/(r*u).
        T = math.lcm(n, hi or 1, abs(ai) * L or 1, 6) * 2
        assert T <= 100000
        ws = [w for w in range(1, T) if math.gcd(w, T) == 1 and (w - c) % n == 0]
        D = T
        first = ai * pow(ws[0], -1, T) % T
        for w in ws:
            wi = pow(w, -1, T)
            for z in range(T // math.gcd(hi, T)):
                t = (ai + hi * z) * wi % T
                D = math.gcd(D, (t - first) % T)
                outa, outh, outd = result
                # Difference is integral after division by the output radius.
                if outh:
                    assert (t * rd * outd - outa * di * rn) % (outh * di * rn) == 0, (case, t, result)
                quotients += 1
        if ai == hi == 0:
            assert result == (0, 0, 1)
        else:
            assert Q(result[1], result[2]) == Q(D * rd, di * rn), (case, D, result)
        # Hull difference gcd from every input unit at a second level.
        U = 30 * L
        us = [w for w in range(1, U) if math.gcd(w, U) == 1 and (w - c) % n == 0]
        G = math.gcd(U, *[w - us[0] for w in us])
        assert Q(small[1], small[2]) == rnq * G
        for w in us:
            for triple in (small, simple):
                a, h, d = triple
                assert ((rnq * w - Q(a, d)) / Q(h, d)).denominator == 1
            units += 1
        enumerations += 1
    print(f"maps: cases={len(cases)} finite_enumerations={enumerations} "
          f"quotient_residues={quotients} hull_units={units} failures=0")


def dyad(m, e):
    return Q(m << e) if e >= 0 else Q(m, 1 << -e)


def floorlog(q):
    e = q.numerator.bit_length() - q.denominator.bit_length()
    if q < dyad(1, e):
        e -= 1
    return e


def roundq(q, p, up=False):
    step = dyad(1, floorlog(q) + 1 - p)
    t = q / step
    n, r = divmod(t.numerator, t.denominator)
    return (n + bool(r and up)) * step


def real():
    rng = random.Random(930)
    cases = []
    for _ in range(22000):
        m = rng.randrange(1, 1 << rng.choice([3, 8, 20, 40, 60]))
        em = rng.randrange(-80, 50)
        width = rng.choice([0, 1, 2, 10, 29, 80, 200])
        rad = rng.randrange(1, 1 << 29) if width else 0
        er = floorlog(dyad(m, em)) - width - 29
        m *= rng.choice([-1, 1])
        p = rng.choice([-5, 0, 1, 2, 3, 4, 8, 20, 64, 100, 256])
        k = rng.choice(list(range(-12, 13)) + [30, 60, 64, 210])
        rn, rd = rng.randint(1, 120), rng.randint(1, 120)
        qn, qd = rng.randint(-120, 120), rng.randint(1, 120)
        fn = rng.choice(["pow", "tight", "mulrat", "norm", "class"])
        c, n = rng.choice([(1, 1), (1, 0), (-1, 0), (5, 6), (3, 8), (2, 5)])
        cases.append((fn, m, em, rad, er, rn, rd, c, n, k, p, qn, qd))
    rows = run(["real " + " ".join(map(str, case)) + "\n" for case in cases])
    okay = nd = notunit = 0
    for case, row in zip(cases, rows):
        fn, m, em, r, er, rn, rd, c, n, k, prec, qn, qd = case
        st = int(row[0]); mid, rad, scale = map(Q, row[1:4]); oc, on, alias = map(int, row[4:7])
        assert alias == 1, (case, row)
        p = max(prec, 2)
        m0, r0 = dyad(abs(m), em), dyad(r, er)
        l, h = m0 - r0, m0 + r0
        ll, hh = roundq(l, p), roundq(h, p, True)
        sign = 1 if m > 0 else -1
        if fn in ("norm", "class"):
            lo, hi = roundq(ll * Q(rd, rn), p), roundq(hh * Q(rd, rn), p, True)
            L, H = l * Q(rd, rn), h * Q(rd, rn)
            sign = 1
        elif fn == "mulrat":
            if qn == 0:
                assert st == 6, (case, st)
                notunit += 1
                continue
            q = abs(Q(qn, qd))
            lo, hi = roundq(ll * q, p), roundq(hh * q, p, True)
            L, H = l * q, h * q
            sign *= 1 if qn > 0 else -1
        else:
            nabs = abs(k)
            lo = hi = Q(1)
            bl, bh = ll, hh
            while nabs:
                if nabs & 1:
                    lo, hi = roundq(lo * bl, p), roundq(hi * bh, p, True)
                nabs >>= 1
                if nabs:
                    bl, bh = roundq(bl * bl, p), roundq(bh * bh, p, True)
            if k < 0:
                lo, hi = roundq(1 / hi, p), roundq(1 / lo, p, True)
            L, H = sorted([l ** k, h ** k])
            if k % 2 == 0:
                sign = 1
        expected = 1 if floorlog(hi) - floorlog(lo) > p else 0
        assert st == expected, (case, row, expected)
        if st:
            nd += 1
            continue
        okay += 1
        L, H = sorted([sign * L, sign * H])
        assert mid - rad <= L <= H <= mid + rad, (case, row, L, H)
        assert abs(mid) > rad and (mid > 0) == (sign > 0)
        if fn in ("pow", "tight"):
            assert scale == Q(rn, rd) ** k
        elif fn == "mulrat":
            assert scale == Q(rn, rd) * abs(Q(qn, qd))
        else:
            if n == 0:
                assert (oc, on) == (c * (1 if m > 0 else -1), 0)
            else:
                assert on == normal(n) and oc % on == (c * (1 if m > 0 else -1)) % on
    print(f"real: cases={len(cases)} OK={okay} NOT_DETERMINED={nd} NOT_UNIT={notunit} failures=0")


def places():
    rng = random.Random(113)
    ps = primes(97) + [18446744073709551557]
    cases = [(rng.randint(1, (1 << 62) - 1), rng.randint(1, (1 << 62) - 1), rng.choice(ps + [0]))
             for _ in range(6000)]
    cases += [(p ** a, q ** b, p) for p in primes(19) for q in primes(19) if p != q
              for a, b in itertools.product(range(1, 8), repeat=2)]
    rows = run([f"place {n} {d} {p}\n" for n, d, p in cases])
    for (n, d, p), row in zip(cases, rows):
        st, sa, val = map(int, row[:3]); absval = Q(row[3])
        if p == 0:
            assert st == sa == 7 and val == 123456 and absval == Q(17, 19)
            continue
        v = 0
        while n % p == 0:
            n //= p; v += 1
        while d % p == 0:
            d //= p; v -= 1
        assert st == sa == 0 and val == v and absval == Q(p) ** (-v)
    print(f"places: cases={len(cases)} failures=0")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("part", choices=["powers", "maps", "real", "places"])
    args = parser.parse_args()
    globals()[args.part]()
