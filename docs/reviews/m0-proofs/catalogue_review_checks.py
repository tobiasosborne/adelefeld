#!/usr/bin/env python3
"""Adversarial checks for docs/proofs/catalogue.md (review, milestone 0).

Written independently of proto/catalogue_checks.py: no function of that file is used to decide a
mathematical question. The author's module is imported only in section M, to run the author's own
checks against deliberately wrong formulas (mutants) and see whether they notice.

Run:  python3 docs/reviews/m0-proofs/catalogue_review_checks.py
Uses: standard library, sympy (kronecker_symbol, factorint), mpmath.
"""

import importlib.util
import io
import random
import re
import sys
from contextlib import redirect_stdout
from fractions import Fraction
from functools import reduce
from math import comb, factorial, gcd, lcm
from pathlib import Path

import mpmath
from sympy import factorint
from sympy.functions.combinatorial.numbers import kronecker_symbol as sym_kron

ROOT = Path(__file__).resolve().parents[3]
RNG = random.Random(20260927)
FAIL = []


def report(name, ok, detail=""):
    print(f"[{'ok' if ok else 'FAIL'}] {name}" + (f": {detail}" if detail else ""))
    if not ok:
        FAIL.append(name)


# ---------------------------------------------------------------- helpers (own code)

def val(n, p):
    """p-adic valuation of a nonzero integer or Fraction."""
    q = Fraction(n)
    assert q != 0
    a, b, v = q.numerator, q.denominator, 0
    while a % p == 0:
        a //= p
        v += 1
    while b % p == 0:
        b //= p
        v -= 1
    return v


def unit_res(q, p, k):
    """Residue modulo p^k of the unit part of the nonzero rational q at p."""
    q = Fraction(q)
    v = val(q, p)
    u = q / Fraction(p) ** v
    m = p ** k
    return u.numerator * pow(u.denominator, -1, m) % m


def canon(n):
    return n // 2 if n % 4 == 2 else n


def primes_upto(n):
    s = [True] * (n + 1)
    out = []
    for i in range(2, n + 1):
        if s[i]:
            out.append(i)
            for j in range(i * i, n + 1, i):
                s[j] = False
    return out


PR = primes_upto(200)


# ---------------------------------------------------------------- 1-3 residue symbols

def leg_sq(a, p):
    """Legendre symbol by the definition: squares of F_p, no Euler criterion."""
    a %= p
    if a == 0:
        return 0
    return 1 if any(x * x % p == a for x in range(1, p)) else -1


def kron_def(a, b):
    """Definition 1 of the proof file, coded from its text, Legendre by squares."""
    if b == 0:
        return 1 if a in (1, -1) else 0
    if b < 0:
        return (-1 if a < 0 else 1) * kron_def(a, -b)
    t = 0
    while b % 2 == 0:
        b //= 2
        t += 1
    out = 1
    for p, h in factorint(b).items():
        out *= leg_sq(a, p) ** h
    two = 0 if a % 2 == 0 else (1 if a % 8 in (1, 7) else -1)
    return out * two ** t


KC = {}


def kr(a, b):
    """Cached kron_def (Definition 1 coded from its text; compared with sympy below)."""
    key = (a, b)
    if key not in KC:
        KC[key] = kron_def(a, b)
    return KC[key]


def sec_symbols():
    n = bad = 0
    for a in range(-60, 61):
        for b in range(-60, 61):
            n += 1
            if kron_def(a, b) != sym_kron(a, b):
                bad += 1
    report("Def 1 vs sympy.kronecker_symbol, a,b in [-60,60]", bad == 0, f"{n} pairs, {bad} differ")

    # Prop 2: the stated modulus is a period; record where it is larger than the least period.
    n = bad = larger = 0
    for b in range(1, 121):
        t = (b & -b).bit_length() - 1
        m = b >> t
        K = m if t == 0 else lcm(m, 8)
        row = [kr(a, b) for a in range(2 * K * 8)]
        if any(row[a] != row[a + K] for a in range(len(row) - K)):
            bad += 1
        least = next(d for d in range(1, K + 1) if K % d == 0 and all(
            row[a] == row[a + d] for a in range(len(row) - d)))
        larger += least < K
        n += 1
    report("Prop 2: K is a period of a -> (a/b), b in 1..120", bad == 0,
           f"{n} values of b, {bad} failures; K larger than least period for {larger} b "
           "(allowed: 'sufficient, not minimal')")
    report("Prop 2 examples (1/2)=1, (3/2)=-1", kr(1, 2) == 1 and kr(3, 2) == -1)

    # Prop 3: set on unit cosets and additive balls. Truth: integers r = c mod N coprime to N*b*2
    # represent profinite units of the coset; integers r = a mod N are points of the ball.
    n = bad = zero_on_unit = det_claims = det_bad = 0
    for N in range(1, 25):
        for b in (1, 2, 3, 4, 5, 6, 8, 9, 12, 15, 16, 21, 24, 40, 45):
            t = (b & -b).bit_length() - 1
            m = b >> t
            K = m if t == 0 else lcm(m, 8)
            L = lcm(N, K)
            for c in range(N):
                if gcd(c, N) != 1:
                    continue
                claim = {kr(r, b) for r in range(L) if gcd(r, L) == 1 and r % N == c % N}
                truth = {kr(r, b) for r in range(c - 6 * L, c + 6 * L, N)
                         if gcd(r, 2 * N * b) == 1}
                n += 1
                bad += claim != truth
                zero_on_unit += 0 in truth
                if N % K == 0:
                    det_claims += 1
                    det_bad += len(truth) != 1
            for a in range(-3, 4):
                claim = {kr(r, b) for r in range(L) if r % N == a % N}
                truth = {kr(a + N * j, b) for j in range(-6 * L, 6 * L)}
                n += 1
                bad += claim != truth
    report("Prop 3: claimed finite sets = sampled truth (cosets and balls)", bad == 0,
           f"{n} inputs, {bad} differ; zero on a unit coset {zero_on_unit} times; "
           f"'K | N => determined' {det_claims} cases, {det_bad} failures")


# ---------------------------------------------------------------- 4-8 Hilbert symbols

def eps(u):
    return ((u - 1) // 2) % 2


def om(u):
    return ((u * u - 1) // 8) % 2


def hil(a, b, p):
    """Definition 4 of the proof file for nonzero rationals; p = 0 means the real place."""
    a, b = Fraction(a), Fraction(b)
    if p == 0:
        return -1 if a < 0 and b < 0 else 1
    al, be = val(a, p), val(b, p)
    if p == 2:
        u, w = unit_res(a, 2, 3), unit_res(b, 2, 3)
        return (-1) ** ((eps(u) * eps(w) + al * om(w) + be * om(u)) % 2)
    u, w = unit_res(a, p, 1), unit_res(b, p, 1)
    sign = (-1) ** ((al * be * (p - 1) // 2) % 2)
    return sign * leg_sq(u, p) ** (be % 2) * leg_sq(w, p) ** (al % 2)


def no_primitive_solution(a, b, p, k):
    """True if a x^2 + b y^2 = z^2 has no solution mod p^k with one coordinate a unit.
    A solution in Q_p scales to a primitive integral one, so True proves the symbol is -1."""
    q = p ** k
    sq = {}
    for z in range(q):
        sq.setdefault(z * z % q, []).append(z)
    for x in range(q):
        for y in range(q):
            r = (a * x * x + b * y * y) % q
            for z in sq.get(r, ()):
                if x % p or y % p or z % p:
                    return False
    return True


def hensel_witness(a, b, p, R):
    """Search integers x,y,z in [0,R) and a coordinate i with v_p(F) > 2 v_p(dF/di), where
    F = a x^2 + b y^2 - z^2 and the coordinate i is a unit. By Hensel's lemma (strong form) F has a
    root in Z_p with the other two coordinates fixed; the root is nonzero since coordinate i is a unit
    and the root agrees with it modulo p. Returns the witness or None."""
    for x in range(R):
        for y in range(R):
            for z in range(R):
                F = a * x * x + b * y * y - z * z
                for co, d in ((x, 2 * a * x), (y, 2 * b * y), (z, -2 * z)):
                    if co % p == 0 or d == 0:
                        continue
                    if F == 0 or val(F, p) > 2 * val(d, p):
                        return (x, y, z)
    return None


def parse_table():
    text = (ROOT / "docs/proofs/catalogue.md").read_text().splitlines()
    rows = {}
    for line in text:
        mm = re.match(r"\|\s*(\d+)\s*\|((\s*[+-]\s*\|){8})\s*$", line)
        if mm:
            rows[int(mm.group(1))] = [1 if s.strip() == "+" else -1
                                      for s in mm.group(2).strip("|").split("|")]
    return rows


def sec_hilbert():
    classes = (1, 3, 5, 7, 2, 6, 10, 14)
    doc = parse_table()
    report("Prop 6: table parsed from catalogue.md", len(doc) == 8, f"{len(doc)} rows")
    mism = 0
    for i, a in enumerate(classes):
        for j, b in enumerate(classes):
            f = hil(a, b, 2)
            if f == -1:
                truth = -1 if no_primitive_solution(a, b, 2, 4) else None
            else:
                truth = 1 if hensel_witness(a, b, 2, 16) else None
            if truth is None or truth != f or doc[a][j] != f:
                mism += 1
                print("   mismatch", a, b, f, truth, doc[a][j])
    report("Prop 6: 64 pairs; -1 by no primitive sol. mod 16, +1 by Hensel witness; = formula = table",
           mism == 0, f"{mism} mismatches")
    # mod 32 must give the same verdict as mod 16 on the -1 entries (no artefact of k=4)
    extra = sum(1 for a in classes for b in classes
                if hil(a, b, 2) == -1 and not no_primitive_solution(a, b, 2, 5))
    report("Prop 6: the 28 minus entries also have no primitive solution mod 32", extra == 0,
           f"{extra} exceptions")
    report("Prop 6 remark: (2,6)_2 = -1 but (1,1,0) solves mod 8",
           hil(2, 6, 2) == -1 and (2 + 6) % 8 == 0 and not no_primitive_solution(2, 6, 2, 3)
           and no_primitive_solution(2, 6, 2, 4))

    # Prop 5 at odd p: every unit residue pair, valuations 0/1.
    tot = mism = 0
    for p in (3, 5, 7, 11, 13):
        for u in range(1, p):
            for w in range(1, p):
                for al in (0, 1):
                    for be in (0, 1):
                        a, b = u * p ** al, w * p ** be
                        f = hil(a, b, p)
                        if f == -1:
                            ok = no_primitive_solution(a, b, p, 2)
                        else:
                            ok = hensel_witness(a, b, p, p * p) is not None
                        tot += 1
                        mism += not ok
    report("Prop 5: odd p in {3,5,7,11,13}, all unit residues, valuations 0/1", mism == 0,
           f"{tot} pairs, {mism} failures (-1 by enumeration mod p^2, +1 by Hensel witness)")

    # Square-class invariance of Definition 4 on arbitrary rationals (proof step 1 of Props 5, 6).
    bad = 0
    squares = [Fraction(s) ** 2 for s in (1, 2, 3, 5, 7, Fraction(1, 2), Fraction(3, 10), 17, Fraction(9, 4))]
    for _ in range(3000):
        a = Fraction(RNG.choice([-1, 1]) * RNG.randint(1, 3000), RNG.randint(1, 300))
        b = Fraction(RNG.choice([-1, 1]) * RNG.randint(1, 3000), RNG.randint(1, 300))
        s, t = RNG.choice(squares), RNG.choice(squares)
        for p in (0, 2, 3, 5, 7, 11):
            bad += hil(a, b, p) != hil(a * s, b * t, p)
            bad += hil(a, b, p) != hil(b, a, p)
    report("Def 4: invariant under squares and symmetric, 3000 random pairs, 6 places", bad == 0,
           f"{bad} failures")
    # Units = 1 mod 8 are squares; 17 in particular, and 1 mod 8 * u gives the same class.
    bad = sum(hil(17 * u, w, 2) != hil(u, w, 2) for u in classes for w in classes)
    report("Def 4 at 2: 17 is a square class 1 (residue 1 mod 8)", bad == 0)

    # Prop 8: product formula on rationals.
    bad = n = 0
    for _ in range(3000):
        a = Fraction(RNG.choice([-1, 1]) * RNG.randint(1, 10 ** 5), RNG.randint(1, 10 ** 4))
        b = Fraction(RNG.choice([-1, 1]) * RNG.randint(1, 10 ** 5), RNG.randint(1, 10 ** 4))
        ps = {2}
        for q in (a, b):
            ps |= set(factorint(abs(q.numerator))) | set(factorint(q.denominator))
        prod = hil(a, b, 0)
        for p in ps:
            prod *= hil(a, b, p)
        # a prime outside the support must give +1
        outside = next(p for p in PR if p not in ps and p > 2)
        bad += prod != 1 or hil(a, b, outside) != 1
        n += 1
    report("Prop 8: product formula, 3000 random rational pairs (num <= 1e5, den <= 1e4)", bad == 0,
           f"{bad} failures")

    # Prop 7: precision of local balls. Bound 3 at p = 2 is needed in general, 2 is not enough.
    counter = None
    for a in (1, 3, 5, 7):
        for b in classes:
            vals = {hil(a + 4 * j, b, 2) for j in range(8)}
            if len(vals) > 1:
                counter = (a, b)
                break
        if counter:
            break
    report("Prop 7: at p=2, relative precision 2 is not enough in general", counter is not None,
           f"ball {counter[0]} + 4 Z_2 against {counter[1]} gives both signs")
    bad = 0
    for _ in range(2000):
        p = RNG.choice((2, 3, 5, 7))
        need = 3 if p == 2 else 1
        cen = [Fraction(RNG.choice([-1, 1]) * RNG.randint(1, 500), RNG.randint(1, 50)) for _ in (0, 1)]
        As = [val(x, p) + need for x in cen]
        ref = hil(cen[0], cen[1], p)
        for _ in range(10):
            x = cen[0] + Fraction(p) ** As[0] * RNG.randint(-10 ** 4, 10 ** 4)
            y = cen[1] + Fraction(p) ** As[1] * RNG.randint(-10 ** 4, 10 ** 4)
            bad += hil(x, y, p) != ref
    report("Prop 7: 2000 random ball pairs at the stated precision bound, 10 points each", bad == 0,
           f"{bad} failures")
    # The author's own precision probe (A=3 at p=2 with b=3) would also pass with A=2:
    weak = {hil(3 + 4 * j, 3, 2) for j in range(-8, 9)}
    report("Prop 7 check weakness: author's probe (3+2^A j, 3) is constant already at A=2",
           len(weak) == 1, "so the probe cannot tell bound 2 from bound 3")

    # Prop 7, ideles: the local unit part at p is (r / p^v_p(r)) * u_p, not u_p.
    # r = s = 3, units exactly 1 (modulus a large power of 2 times 3): the true symbol at 2 is (3,3)_2.
    true2 = hil(3, 3, 2)
    naive2 = (-1) ** ((eps(1) * eps(1) + 0 * om(1) + 0 * om(1)) % 2)
    report("Prop 7 idele pitfall: scales r=s=3, unit 1: true (3,3)_2 vs unit-only evaluation",
           true2 == -1 and naive2 == 1, f"true {true2}, unit part without the cofactor of r gives {naive2}")
    # all-place product for these ideles (all components rational 3) is 1; for the unit-only version it
    # would claim (3,3)_3 = ... and a product -1: show the cofactor matters for the family too.
    fam = [hil(3, 3, v) for v in (0, 2, 3)]
    report("Prop 7: the idele (3,3) is the rational 3, product over places", reduce(lambda x, y: x * y, fam) == 1,
           f"signs at inf,2,3 = {fam}")


# ---------------------------------------------------------------- 9 local zeta factors

def sec_zeta():
    mpmath.mp.dps = 30
    worst = mpmath.mpf(0)
    cuts = [0, mpmath.mpf(10) ** -6, mpmath.mpf(10) ** -3, mpmath.mpf("0.1"), 1, 3, mpmath.inf]
    for s in (mpmath.mpf("0.7"), mpmath.mpf(2), mpmath.mpc("3.5", "2"), mpmath.mpc("1.5", "-3")):
        lhs = 2 * mpmath.quad(lambda x: mpmath.exp(-mpmath.pi * x * x) * x ** (s - 1), cuts, maxdegree=10)
        rhs = mpmath.pi ** (-s / 2) * mpmath.gamma(s / 2)
        worst = max(worst, abs(lhs - rhs) / abs(rhs))
    report("Prop 9: 2 int exp(-pi x^2) x^(s-1) dx = pi^(-s/2) Gamma(s/2), 4 values of s",
           worst < mpmath.mpf(10) ** -20, f"max rel. error {mpmath.nstr(worst, 3)}")
    worst_pole = mpmath.mpf(0)
    for p in (2, 3, 5, 7):
        for k in (-2, -1, 1, 2, 3):
            s = 2j * mpmath.pi * k / mpmath.log(p)
            worst_pole = max(worst_pole, abs(1 - mpmath.power(p, -s)))
            d = mpmath.diff(lambda z: 1 - mpmath.power(p, -z), s)
            assert abs(d - mpmath.log(p)) < mpmath.mpf(10) ** -20
    report("Prop 9: 1 - p^-s vanishes at s = 2 pi i k/log p with derivative log p",
           worst_pole < mpmath.mpf(10) ** -25, f"max |1-p^-s| there {mpmath.nstr(worst_pole, 3)}")
    # Real-factor poles: Gamma(s/2) at s = 0,-2,-4; not at s = -1, -3 (Gamma(-1/2) finite).
    finite = [mpmath.pi ** (-s / 2) * mpmath.gamma(mpmath.mpf(s) / 2) for s in (-1, -3, -5)]
    near = [abs(mpmath.pi ** (-s / 2) * mpmath.gamma((s + mpmath.mpf(10) ** -12) / 2)) for s in (0, -2, -4)]
    report("Prop 9: real factor finite at -1,-3,-5 and ~1e12 near 0,-2,-4",
           all(mpmath.isfinite(v) for v in finite) and min(near) > 10 ** 10,
           f"min |value| at 1e-12 from pole: {mpmath.nstr(min(near), 3)}")


# ---------------------------------------------------------------- 10 content and volume

def sec_volume():
    # vol(a + N Zhat) for rational N = n/d, counted at level Q Zhat with Q a multiple of n:
    # the ball meets Zhat/QZhat after scaling by d: d*(a + N Zhat) = d a + n Zhat has measure (1/n),
    # and scaling by d multiplies measure by |d|_f = 1/d, so vol = (1/n)/(1/d) = d/n = 1/N.
    bad = 0
    for n in range(1, 30):
        for d in range(1, 12):
            if gcd(n, d) != 1:
                continue
            Q = 4 * n
            hits = sum(1 for x in range(Q) if (x - 7 * d) % n == 0)  # d a + n Zhat with a = 7
            vol_scaled = Fraction(hits, Q)
            vol = vol_scaled * d  # undo |d|_f = 1/d
            bad += vol != Fraction(d, n)
    report("Prop 10: vol(a + (n/d) Zhat) = d/n by counting (n<30, d<12)", bad == 0, f"{bad} failures")
    # content r vs finite absolute value 1/r; product over all primes of p^-v_p(r)
    bad = 0
    for _ in range(500):
        r = Fraction(RNG.randint(1, 10 ** 6), RNG.randint(1, 10 ** 6))
        prod = Fraction(1)
        for p in set(factorint(r.numerator)) | set(factorint(r.denominator)):
            prod *= Fraction(p) ** (-val(r, p))
        bad += prod != 1 / r
    report("Prop 10: prod_p |r|_p = 1/r, 500 random r", bad == 0)


# ---------------------------------------------------------------- 11-13 profinite power

def sec_power_target():
    """Prop 12 by brute force: outputs mod N are b^(x mod lambda) for the one base residue c mod N and
    exponents x = e + M t, t over a full period."""
    n = bad = 0
    for N in range(1, 41):
        lam = 1
        for q in range(1, N + 1):
            if gcd(q, N) == 1:
                o = 1
                while pow(q, o, N) != 1 % N:
                    o += 1
                lam = lcm(lam, o)
        for c in range(-N, N):
            if gcd(c, N) != 1:
                continue
            for M in range(1, 7):
                for e in range(-4, 5):
                    outs = {pow(c, e + M * t, N) if N > 1 else 0 for t in range(0, 2 * lam + 2)}
                    D = gcd(N, c ** M - 1)
                    n += 1
                    bad += (len(outs) == 1) != (D == N)
                    for L in range(1, N + 1):
                        if N % L:
                            continue
                        det = len({o % L for o in outs}) == 1
                        bad += det != (D % L == 0)
                    # every output lies in c^e U(D)
                    ce = pow(c, e, D) if D > 1 else 0
                    bad += any(o % D != ce for o in outs) if D > 1 else 0
    report("Prop 12: determination mod N, largest divisor D, output in c^e U(D)", bad == 0,
           f"{n} cases (N<=40, c in [-N,N), M<=6, e in [-4,4]), {bad} failures")


def local_depth(p, n, c, e, M):
    """Largest k such that b^(e+Mt) mod p^k is constant over bases b in c U(p^n) (all units if n=0)
    and all t; M = 0 means exact exponent. Elementary: constant iff b^e = b0^e and b^M = 1 for all b.
    K is increased until the depth is below it (not saturated). Returns None if K limit is hit."""
    K = n + 2
    while p ** K <= 300000:
        mod = p ** K
        step = p ** n if n else 1
        start = c % step if n else 1
        bases = [b for b in range(start, mod, step) if b % p]
        b0 = bases[0]
        e0 = pow(b0, e, mod)
        depth = K
        for b in bases:
            dif = (pow(b, e, mod) - e0) % mod
            if dif:
                depth = min(depth, val(dif, p))
            if M:
                dif = (pow(b, M, mod) - 1) % mod
                if dif:
                    depth = min(depth, val(dif, p))
            if depth == 0:
                return 0
        if depth < K:
            return depth
        K += 1
    return None


def table_F(N, c, e, M):
    """Prop 13 table as stated in the proof file; M = 0 means exact e != 0."""
    g = gcd(e, M) if M else abs(e)
    F = 1
    cand = set(factorint(N)) | {p for p in PR if p <= g + 1}
    for p in cand:
        if N % p == 0:
            n = val(N, p)
            cm = c ** M - 1 if M else 0
            vcm = val(cm, p) if cm else 10 ** 9
            k = min(n + val(g, p), vcm)
        elif p == 2:
            k = 1 if g % 2 else 2 + val(g, 2)
        else:
            k = 1 + val(g, p) if g % (p - 1) == 0 else 0
        F *= p ** k
    return F


def brute_F(N, c, e, M, plist):
    F = 1
    centre = {}
    for p in plist:
        n = val(N, p) if N % p == 0 else 0
        d = local_depth(p, n, c, e, M)
        if d is None:
            return None, None
        F *= p ** d
        if d:
            mod = p ** d
            b0 = next(b for b in range(1, 10 ** 6) if b % p and (n == 0 or (b - c) % p ** n == 0))
            centre[p ** d] = pow(b0, e, mod)
    return F, centre


def crt(pairs):
    x, m = 0, 1
    for mod, r in pairs.items():
        t = ((r - x) * pow(m, -1, mod)) % mod
        x, m = x + m * t, m * mod
    return x % m, m


def sec_power_finest():
    plist = [p for p in PR if p <= 37]
    n = bad = skipped = centre_bad = centre_naive_bad = noncanon = 0
    first_naive = None
    for N in range(1, 31):
        if canon(N) != N:
            continue
        for c in range(1, N + 1):
            if gcd(c, N) != 1:
                continue
            for M in range(0, 7):
                for e in range(-5, 7):
                    if M == 0 and e == 0:
                        continue
                    g = gcd(e, M) if M else abs(e)
                    if g + 1 > 37:
                        continue
                    Fb, cen = brute_F(N, c, e, M, plist)
                    if Fb is None:
                        skipped += 1
                        continue
                    Ft = table_F(N, c, e, M)
                    n += 1
                    noncanon += Ft % 4 == 2
                    if Fb != Ft:
                        bad += 1
                        if bad < 5:
                            print("   table mismatch", (N, c, e, M), "brute", Fb, "table", Ft)
                    # centre of the output coset modulo F
                    r_true, m = crt(cen) if cen else (0, 1)
                    pairs = {}
                    for q in cen:
                        p = factorint(q).popitem()[0]
                        pairs[q] = pow(c, e, q) if N % p == 0 else 1
                    r_rep, _ = crt(pairs) if pairs else (0, 1)
                    centre_bad += r_rep != r_true
                    m = canon(m)
                    if m > 1:
                        naive_ok = gcd(c, m) == 1 and pow(c, e, m) == r_true % m
                        if not naive_ok:
                            centre_naive_bad += 1
                            if first_naive is None:
                                first_naive = (N, c, e, M, m, r_true)
    report("Prop 13: table F = independent local brute force (canonical N<=30, e in [-5,6], M in 0..6)",
           bad == 0, f"{n} cases, {bad} mismatches, {skipped} skipped (K limit)")
    report("Prop 13: table output F is twice an odd number (non-canonical)", True,
           f"{noncanon} of {n} cases (e.g. 2 not dividing N with g odd, or N=4, c=3, M=1)")
    report("Prop 13 repair: centre = CRT(c^e at p|N, 1 at p not dividing N) is the true centre",
           centre_bad == 0, f"{centre_bad} failures")
    report("Prop 13 gap: 'c^e mod canon(F)' is NOT the centre of the output coset", centre_naive_bad > 0,
           f"{centre_naive_bad} of {n} cases; first (N,c,e,M,F,true centre) = {first_naive}")
    # the doc example
    Fb, cen = brute_F(5, 2, 0, 2, plist)
    report("Prop 13 example N=5,c=2,e=0,M=2: D=1, F=24", gcd(5, 2 ** 2 - 1) == 1 and Fb == 24, f"F={Fb}")
    Fb, cen = brute_F(5, 2, 2, 4, plist)
    report("Prop 13 example N=5,c=2,e=2,M=4", Fb == 120 and crt(cen)[0] == 49 and pow(2, 2, 120) == 4,
           f"F={Fb}, true centre {crt(cen)[0]}, c^e = 4 is not even a unit mod 120")
    # Non-canonical N (twice odd): table applied without canonicalisation must not be finer than truth.
    finer = coarser = eq = 0
    for m in range(1, 16, 2):
        N = 2 * m
        for c in range(1, N, 2):
            if gcd(c, N) != 1:
                continue
            for M in range(1, 5):
                for e in range(-3, 5):
                    Fb, _ = brute_F(m, c, e, M, plist)
                    Ft = table_F(N, c, e, M)
                    if Fb is None:
                        continue
                    if Fb % Ft == 0 and Fb != Ft:
                        coarser += 1
                    elif Fb == Ft:
                        eq += 1
                    else:
                        finer += 1
    report("Prop 13: table misapplied to N = 2*odd is never finer than the truth", finer == 0,
           f"equal {eq}, coarser {coarser}, finer (unsafe) {finer}")
    # Random larger inputs.
    n = bad = 0
    for _ in range(300):
        N = canon(RNG.randint(1, 400))
        c = RNG.randint(1, 10 ** 4)
        if gcd(c, N) != 1:
            continue
        M = RNG.randint(1, 16)
        e = RNG.randint(-30, 30)
        g = gcd(e, M)
        if g + 1 > 37 or any(p > 37 for p in factorint(N)):
            continue
        Fb, _ = brute_F(N, c, e, M, plist)
        if Fb is None:
            continue
        n += 1
        bad += Fb != table_F(N, c, e, M)
    report("Prop 13: random canonical N<=400 (primes<=37), c<=1e4, M<=16, |e|<=30", bad == 0,
           f"{n} cases, {bad} mismatches")


# ---------------------------------------------------------------- 14 binomial

def binom_poly(x, k):
    num = 1
    for j in range(k):
        num *= x - j
    q, r = divmod(num, factorial(k))
    assert r == 0
    return q


def sec_binomial():
    n = bad = cons_bad = zero_bad = 0
    for N in range(1, 41):
        for a in range(-12, 13):
            for k in range(0, 9):
                h0 = binom_poly(a, k)
                R = reduce(gcd, (binom_poly(a + N * j, k) - h0 for j in range(1, k + 1)), 0)
                truth = reduce(gcd, (binom_poly(a + N * t, k) - h0 for t in range(-60, 61)), 0)
                n += 1
                bad += abs(R) != abs(truth)
                cons = N // gcd(N, factorial(k))
                cons_bad += (R % cons != 0) if R else (k != 0)
                zero_bad += (R == 0) != (k == 0)
    report("Prop 14: R = gcd over j=1..k equals gcd over t in [-60,60]", bad == 0,
           f"{n} cases (N<=40, a in [-12,12], k<=8), {bad} failures")
    report("Prop 14: conservative radius N/gcd(N,k!) divides R; R=0 iff k=0",
           cons_bad == 0 and zero_bad == 0, f"{cons_bad}, {zero_bad} failures")
    ex = reduce(gcd, (binom_poly(8 * j, 4) for j in range(1, 5)))
    report("Prop 14 example (0,8,4): R=2, conservative radius 1", ex == 2 and 8 // gcd(8, 24) == 1)


# ---------------------------------------------------------------- 15 cyclotomic action

def sec_cyclo():
    n = bad = 0
    for N in range(1, 49):
        for nn in range(1, 49):
            q = lcm(N, nn, 2)
            for c in range(N):
                if gcd(c, N) != 1:
                    continue
                img = {r % nn for r in range(q) if gcd(r, q) == 1 and r % N == c % N}
                n += 1
                bad += (len(img) == 1) != (canon(N) % canon(nn) == 0)
    report("Prop 15: action of c U(N) on n-th roots determined iff canon(n) | canon(N)", bad == 0,
           f"{n} (N, n, c) triples, N,n < 49, {bad} failures")
    # Test idele: component p at p, 1 elsewhere (real 1). Class unit u' has components 1 at p and p^-1
    # at q != p. Its residue mod n = p^j m (gcd(m,p)=1) is CRT(1 mod p^j, p^-1 mod m).
    bad = 0
    for p in (2, 3, 5, 7):
        for nn in range(2, 60):
            j = val(nn, p) if nn % p == 0 else 0
            m = nn // p ** j
            pairs = {}
            if p ** j > 1:
                pairs[p ** j] = 1
            if m > 1:
                pairs[m] = pow(p, -1, m)
            u, _ = crt(pairs) if pairs else (0, 1)
            arith = pow(u, -1, nn)
            if gcd(p, nn) == 1:
                bad += arith != p % nn
            elif m == 1:
                bad += arith != 1 % nn
    report("Prop 15: test idele acts by z -> z^p (n prime to p), trivially on p-power roots", bad == 0,
           f"{bad} failures")


# ---------------------------------------------------------------- M mutants against the author's checks

def load_author():
    spec = importlib.util.spec_from_file_location("cat", ROOT / "proto/catalogue_checks.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def survives(mod, fname, check, patch):
    orig = getattr(mod, fname)
    setattr(mod, fname, patch(orig))
    try:
        with redirect_stdout(io.StringIO()):
            getattr(mod, check)()
        return True
    except AssertionError:
        return False
    finally:
        setattr(mod, fname, orig)


def sec_mutants():
    mod = load_author()

    def k_eps(orig):
        def f(a, b):
            t = mod.vp(b, 2) if b % 2 == 0 else 0
            m = b // 2 ** t
            two = 0 if a % 2 == 0 else (-1) ** (((a - 1) // 2) % 2)  # eps instead of omega
            return mod.jacobi(a, m) * two ** t
        return f

    def jac_squarefree(orig):
        def f(a, m):
            out = 1
            for p in mod.primes(m):
                out *= mod.legendre(a, p)  # exponent dropped
            return out
        return f

    def kexact_zero(orig):
        def f(a, b):
            if b == 0:
                return 1 if abs(a) == 1 else 0
            if b < 0:
                return (-1 if a <= 0 else 1) * mod.kronecker_positive(a, -b)  # (0/-1) = -1
            return mod.kronecker_positive(a, b)
        return f

    def hil_noswap(orig):
        def f(a, b, p):
            if p == 2:
                al, u = mod.unit_of(a, 2)
                be, w = mod.unit_of(b, 2)
                U, W = mod.residue(u, 8), mod.residue(w, 8)
                e_ = lambda v: ((v - 1) // 2) % 2
                o_ = lambda v: ((v * v - 1) // 8) % 2
                return (-1) ** ((e_(U) * e_(W) + al * o_(U) + be * o_(W)) % 2)  # swapped
            return orig(a, b, p)
        return f

    def finest_no_vg(orig):
        def f(N, c, e, M):
            g = gcd(e, M)
            out = 1
            for p in mod.primes(8 * N * g * factorial(g + 1)):
                n = mod.vp(N, p) if N % p == 0 else 0
                if n:
                    cm = pow(c, M, p ** (n + 4)) - 1
                    depth = n if cm == 0 else min(n, mod.vp(cm, p))  # v_p(g) dropped
                elif p == 2:
                    depth = 1 if g % 2 else 2 + mod.vp(g, 2)
                elif g % (p - 1) == 0:
                    depth = 1 + mod.vp(g, p) if g % p == 0 else 1
                else:
                    depth = 0
                out *= p ** depth
            return out
        return f

    rows = [
        ("(a/2) = (-1)^eps(a) instead of (-1)^om(a)", "kronecker_positive", "check_symbols", k_eps),
        ("Jacobi ignores prime multiplicity", "jacobi", "check_symbols", jac_squarefree),
        ("(0/-1) = -1 instead of 1", "kronecker_exact", "check_symbols", kexact_zero),
        ("2-adic: alpha om(u) + beta om(w) (swapped)", "hilbert_formula", "check_hilbert", hil_noswap),
        ("finest modulus without v_p(g) at p|N", "local_finest", "check_power", finest_no_vg),
    ]
    for name, fname, check, patch in rows:
        s = survives(mod, fname, check, patch)
        print(f"   mutant '{name}' vs {check}: {'SURVIVES (check blind)' if s else 'killed'}")
    # our own kron_def distinguishes them
    report("own check kills (a/2) mutant: (3/2) vs (5/2)", kron_def(5, 2) == -1 and kron_def(7, 2) == 1)
    report("own check kills (0/-1) mutant", kron_def(0, -1) == 1 == sym_kron(0, -1))
    report("own check kills Jacobi-multiplicity mutant: (2/9) = 1, (2/3) = -1",
           kron_def(2, 9) == 1 and kron_def(2, 3) == -1)


if __name__ == "__main__":
    for sec in (sec_symbols, sec_hilbert, sec_zeta, sec_volume, sec_power_target, sec_power_finest,
                sec_binomial, sec_cyclo, sec_mutants):
        print(f"== {sec.__name__}")
        sec()
    print(f"\n{len(FAIL)} failed: {FAIL}")
    sys.exit(1 if FAIL else 0)
