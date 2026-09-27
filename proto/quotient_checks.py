#!/usr/bin/env python3
"""Numerical checks for docs/proofs/quotient.md (reduction modulo Q, splitting, reconstruction).

Independent computations used here:
  * membership of a point (s, w) of the fundamental domain [0,1) x Zhat (w an integer standing for its class)
    in the image of a ball X = I x (a + N Zhat) is decided from the definition: search rationals q on a grid
    finer than needed and test s + q in I and w + q - a in N Zhat prime by prime (v_p(w+q-a) >= v_p(N));
  * sets of finite balls are compared by projection to Z/M (see policies_checks.py);
  * p-adic roots are found by exhaustive search modulo p^k.
The real coordinate s runs over a rational grid that contains all endpoints and all midpoints between them,
which decides equality of finite unions of intervals exactly.
Each check prints one line with counts; the script exits non-zero on any failure.
"""
from fractions import Fraction as F
from math import gcd, floor, ceil
from functools import reduce
import random
import sys

random.seed(20260927)
FAILURES = []


def lcm(a, b):
    return a // gcd(a, b) * b


def primes_upto(n):
    return [p for p in range(2, n + 1) if all(p % q for q in range(2, int(p ** 0.5) + 1))]


PRIMES = primes_upto(200)


def vp(x, p):
    x = F(x)
    v, a, b = 0, x.numerator, x.denominator
    while a % p == 0:
        a //= p
        v += 1
    while b % p == 0:
        b //= p
        v -= 1
    return v


def in_ball(x, a, N):
    """x in a + N Zhat for rationals, by valuations (definition of Zhat as a product of the Z_p)."""
    d = F(x) - F(a)
    if N == 0:
        return d == 0
    if d == 0:
        return True
    ps = {p for p in PRIMES if (d.numerator * d.denominator * N.numerator * N.denominator) % p == 0}
    return all(vp(d, p) >= vp(N, p) for p in ps)


def report(name, ok, detail):
    print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}")
    if not ok:
        FAILURES.append(name)


# ---------------------------------------------------------------- brute-force image membership

def member_brute(s, w, lo, hi, a, N, extra_den=6):
    """Is (s, w) congruent modulo Q to a point of [lo,hi] x (a + N Zhat)?"""
    den = F(a).denominator * F(N).denominator * extra_den
    # q ranges over a + i/den with s + q in [lo, hi]
    imin = ceil((lo - s - a) * den)
    imax = floor((hi - s - a) * den)
    for i in range(imin, imax + 1):
        q = a + F(i, den)
        if lo <= s + q <= hi and in_ball(w + q, a, N):
            return True
    return False


def pieces_int(lo, hi, a, N):
    """Half-open pieces of Proposition 5 and closed pieces of Proposition 6, for integer N."""
    l2, h2 = lo - a, hi - a
    half = list(range(floor(l2), floor(h2) + 1))
    closed = [floor(l2)] if l2 == h2 else list(range(floor(l2), ceil(h2)))
    return l2, h2, half, closed


def member_half(s, w, l2, h2, half, N):
    return any(l2 <= s + n <= h2 and (w + n) % N == 0 for n in half)


def member_closed(s, w, l2, h2, closed, N):
    for n in closed:
        lo_n, hi_n = max(l2, n) - n, min(h2, n + 1) - n
        if lo_n <= s <= hi_n and (w + n) % N == 0:
            return True
        if s == 0 and hi_n == 1 and (w + 1 + n) % N == 0:
            return True
    return False


def rand_interval(den=12, span=5, maxlen=4):
    lo = F(random.randint(-span * den, span * den), den)
    hi = lo + F(random.randint(0, maxlen * den), den)
    return lo, hi


# ---------------------------------------------------------------- Section 1

def check_fundamental_domain():
    ok = True
    n = 0
    S = [2, 3, 5, 7]
    for _ in range(500):
        # Lemma 1: x_f with rational components c_p at p in S, and 0 (in Z_p) elsewhere
        comps = {p: F(random.randint(-500, 500), p ** random.randint(0, 4) * random.choice([1, 11, 13]))
                 for p in S}
        q = F(0)
        for p in S:
            k = max(0, -vp(comps[p], p)) if comps[p] != 0 else 0
            y = comps[p] * p ** k  # in Z_(p)
            # integer n_p with y - n_p in p^k Z_p: search
            mod = p ** k
            yn = (y.numerator * pow(y.denominator, -1, mod)) % mod if mod > 1 else 0
            q += F(yn, mod)
        for p in S:
            ok &= comps[p] - q == 0 or vp(comps[p] - q, p) >= 0
        for p in PRIMES[4:30]:
            ok &= q == 0 or vp(q, p) >= 0
        # Proposition 2: representative of (t, x_f) with x_f rational, and invariance under adding rationals
        t = F(random.randint(-900, 900), random.randint(1, 30))
        xf = F(random.randint(-900, 900), random.randint(1, 30))

        def rep(t, xf):
            n_ = floor(t - xf)
            return (t - xf - n_, -n_)

        r0 = rep(t, xf)
        ok &= 0 <= r0[0] < 1
        q0 = F(random.randint(-900, 900), random.randint(1, 30))
        ok &= rep(t + q0, xf + q0) == r0
        n += 1
    # Proposition 3: points of [0,1] x Z (integers standing for classes): congruent iff equal or glued
    pts = [(s, z) for s in (F(0), F(1, 2), F(1)) for z in range(-3, 4)]
    for (s, z) in pts:
        for (s2, z2) in pts:
            r = s2 - s
            brute = r.denominator == 1 and z2 - z == r
            pred = ((s, z) == (s2, z2) or (s == 0 and s2 == 1 and z2 == z + 1)
                    or (s == 1 and s2 == 0 and z == z2 + 1))
            ok &= brute == pred
    # Corollary 4: rationals in (-1/2, 1/2) that lie in Zhat
    found = [F(i, d) for d in range(1, 40) for i in range(-d // 2, d // 2 + 1)
             if abs(F(i, d)) < F(1, 2) and in_ball(F(i, d), 0, F(1))]
    ok &= set(found) == {0}
    report("check_fundamental_domain (L1, P2, P3, C4)", ok, f"{n} points; gluing on 441 pairs; discreteness")


# ---------------------------------------------------------------- Section 2

def check_reduction():
    ok = True
    cases = pts = 0
    for _ in range(250):
        N = random.randint(1, 6)
        lo, hi = rand_interval()
        a = F(random.randint(-20, 20), random.choice([1, 2, 4]))
        l2, h2, half, closed = pieces_int(lo, hi, a, N)
        ok &= len(half) == floor(h2) - floor(l2) + 1
        k = len([j for j in range(floor(l2) - 1, ceil(h2) + 2) if l2 < j < h2])
        ok &= len(closed) == (k + 1 if l2 < h2 else 1)
        for nn in half:  # every half-open piece is non-empty
            ok &= max(l2, nn) < nn + 1 and max(l2, nn) <= h2
        G = 24
        for j in range(G):
            s = F(j, G)
            for w in range(N):
                b = member_brute(s, w, lo, hi, a, F(N))
                ok &= b == member_half(s, w, l2, h2, half, N)
                ok &= b == member_closed(s, w, l2, h2, closed, N)
                pts += 1
        cases += 1
    # the example of SPEC 6: [0.9, 1.1] x (0 mod 2)
    l2, h2, half, closed = pieces_int(F(9, 10), F(11, 10), F(0), 2)
    ok &= half == [0, 1] and closed == [0, 1]
    ok &= member_closed(F(95, 100), 0, l2, h2, closed, 2) and member_closed(F(5, 100), 1, l2, h2, closed, 2)
    ok &= not member_closed(F(5, 100), 0, l2, h2, closed, 2)
    report("check_reduction (P5, P6)", ok, f"{cases} balls, {pts} grid points: brute force = half-open = closed")


def check_full_image():
    ok = True
    n = full_count = 0
    for _ in range(300):
        N = random.randint(1, 5)
        lo, hi = rand_interval(maxlen=6)
        a = F(random.randint(-20, 20), random.choice([1, 2, 4]))
        G = 24
        full = all(member_brute(F(j, G), w, lo, hi, a, F(N)) for j in range(G) for w in range(N))
        ok &= full == (hi - lo >= N)
        full_count += full
        n += 1
    report("check_full_image (P7)", ok, f"{n} balls ({full_count} with full image): full iff hi - lo >= N")


# ---------------------------------------------------------------- Section 3

def proj(c, R, D, M):
    step = int(D * R)
    assert step > 0 and M % step == 0
    return frozenset(range(int(D * c) % step, M, step))


def check_split():
    ok = True
    n = meets = pts = 0
    for _ in range(300):
        A, B = random.randint(1, 12), random.randint(1, 6)
        if gcd(A, B) != 1:
            continue
        N = F(A, B)
        a = F(random.randint(-30, 30), random.randint(1, 6))
        pieces = [(a + k * N, F(A)) for k in range(B)]
        D = lcm(a.denominator, B)
        M = D * A * 2
        whole = proj(a, N, D, M)
        ps = [proj(c, R, D, M) for c, R in pieces]
        ok &= frozenset().union(*ps) == whole and sum(len(x) for x in ps) == len(whole)
        for _ in range(10):
            c = F(random.randint(-30, 30), random.randint(1, 6))
            R = random.randint(1, 12)
            D2 = lcm(D, c.denominator)
            M2 = lcm(D2 * A, D2 * R) * 2
            ball = proj(c, F(R), D2, M2)
            hit = sum(1 for (cc, RR) in pieces if proj(cc, RR, D2, M2) & ball)
            ok &= hit <= 1
            meets += 1
        n += 1
    # reduction of a fractional-radius ball: split, then Proposition 5, against brute force
    for _ in range(120):
        A, B = random.randint(1, 5), random.randint(2, 4)
        if gcd(A, B) != 1:
            continue
        N = F(A, B)
        lo, hi = rand_interval(maxlen=3)
        a = F(random.randint(-20, 20), random.choice([1, 2, 3, 4]))
        G = 24 * lcm(B, a.denominator)
        split = []
        for k in range(B):
            l2, h2, half, closed = pieces_int(lo, hi, a + k * N, A)
            split.append((l2, h2, closed))
        for j in range(0, G, max(1, G // 48)):
            s = F(j, G)
            for w in range(A):
                b = member_brute(s, w, lo, hi, a, N)
                p = any(member_closed(s, w, l2, h2, cl, A) for l2, h2, cl in split)
                ok &= b == p
                pts += 1
    report("check_split (P8)", ok, f"{n} splits (disjoint cover), {meets} integer-radius balls meet <= 1 piece; "
           f"{pts} grid points of fractional reductions")


# ---------------------------------------------------------------- Section 4

def family_of(lo, hi, a, N):
    """Closed pieces (J, m, Ni) of the reduction of I x (a + N Zhat), N = A/B > 0 or N = 0."""
    fam = []
    if N == 0:
        l2, h2 = lo - a, hi - a
        idx = [floor(l2)] if l2 == h2 else list(range(floor(l2), ceil(h2)))
        return [((max(l2, n) - n, min(h2, n + 1) - n), -n, 0) for n in idx]
    A, B = N.numerator, N.denominator
    for k in range(B):
        l2, h2, half, closed = pieces_int(lo, hi, a + k * N, A)
        for n in closed:
            fam.append(((max(l2, n) - n, min(h2, n + 1) - n), -n, A))
    return fam


def canonical(fam, Np):
    """Proposition 9: for each class m mod Np, the subset T_m of [0,1), as a membership function on points."""
    T = {m: [] for m in range(Np)}
    for (al, be), m0, Ni in fam:
        for j in range(Np // Ni):
            m = (m0 + j * Ni) % Np
            T[m].append(('iv', al, be))
            if be == 1:
                T[(m - 1) % Np].append(('pt', F(0), F(0)))
    return T


def T_member(Tm, s):
    for kind, al, be in Tm:
        if kind == 'pt' and s == 0:
            return True
        if kind == 'iv' and al <= s <= be and s < 1:
            return True
    return False


def critical_points(Ts):
    ends = {F(0)}
    for Tm in Ts:
        for _, al, be in Tm:
            ends |= {al, be}
    ends = sorted(e for e in ends if 0 <= e < 1) + [F(1)]
    pts = []
    for x, y in zip(ends, ends[1:]):
        pts += [x, (x + y) / 2]
    return pts


def same_image(fam1, fam2):
    Np = reduce(lcm, [Ni for _, _, Ni in fam1 + fam2], 1)
    T1, T2 = canonical(fam1, Np), canonical(fam2, Np)
    for m in range(Np):
        for s in critical_points([T1[m], T2[m]]):
            if T_member(T1[m], s) != T_member(T2[m], s):
                return False
    return True


def check_translation():
    ok = True
    n = neg = 0
    for _ in range(250):
        A, B = random.randint(1, 6), random.choice([1, 1, 2, 3])
        if gcd(A, B) != 1:
            continue
        N = F(A, B)
        lo, hi = rand_interval(maxlen=3)
        a = F(random.randint(-20, 20), random.choice([1, 2, 4]))
        q0 = F(random.randint(-50, 50), random.randint(1, 12))
        f1 = family_of(lo, hi, a, N)
        f2 = family_of(lo + q0, hi + q0, a + q0, N)
        ok &= f1 == f2 and same_image(f1, f2)
        # the canonical form agrees with brute force on a grid (classes mod A, integer w)
        T = canonical(f1, A)
        G = 12 * lcm(B, a.denominator)
        for j in range(0, G, max(1, G // 24)):
            s = F(j, G)
            for w in range(A):
                ok &= T_member(T[w % A], s) == member_brute(s, w, lo, hi, a, N)
        # negative control: a different ball has a different canonical form exactly when the images differ
        lo3, hi3 = rand_interval(maxlen=3)
        f3 = family_of(lo3, hi3, a, N)
        same = same_image(f1, f3)
        # all endpoints have denominators dividing 12, so the grid j/24 contains every endpoint and midpoint
        brute_same = all(member_brute(F(j, 24), w, lo, hi, a, N) == member_brute(F(j, 24), w, lo3, hi3, a, N)
                         for j in range(24) for w in range(A))
        ok &= same == brute_same
        neg += not same
        n += 1
    # radius 0
    for _ in range(100):
        lo, hi = rand_interval(maxlen=3)
        a = F(random.randint(-20, 20), random.randint(1, 6))
        q0 = F(random.randint(-50, 50), random.randint(1, 12))
        ok &= family_of(lo, hi, a, F(0)) == family_of(lo + q0, hi + q0, a + q0, F(0))
    report("check_translation (P9, P10)", ok, f"{n} balls translated: identical pieces and canonical forms; "
           f"{neg} different balls told apart")


# ---------------------------------------------------------------- Section 5

def check_reconstruct_full():
    ok = True
    n = counts = 0
    hist = {}
    for _ in range(400):
        lo, hi = rand_interval(den=6, span=4, maxlen=3)
        a = F(random.randint(-20, 20), random.randint(1, 6))
        N = F(0) if random.random() < 0.15 else F(random.randint(1, 8), random.randint(1, 5))
        if N > 0:
            k0, k1 = ceil((lo - a) / N), floor((hi - a) / N)
            pred = [a + N * k for k in range(k0, k1 + 1)]
        else:
            pred = [a] if lo <= a <= hi else []
        D = lcm(a.denominator, N.denominator) if N else a.denominator
        brute = sorted({F(i, D) for i in range(floor(lo * D), ceil(hi * D) + 1)
                        if lo <= F(i, D) <= hi and in_ball(F(i, D), a, N)})
        ok &= brute == sorted(pred)
        # no other denominators occur: rationals with denominator up to 12 in I and in the ball
        other = [F(i, d) for d in range(1, 13) for i in range(floor(lo * d), ceil(hi * d) + 1)
                 if lo <= F(i, d) <= hi and in_ball(F(i, d), a, N)]
        ok &= set(other) <= set(pred)
        if N > 0 and hi - lo < N:
            ok &= len(pred) <= 1
        if N > 0 and hi - lo >= N:
            ok &= len(pred) >= 1
        hist[min(len(pred), 3)] = hist.get(min(len(pred), 3), 0) + 1
        n += 1
    report("check_reconstruct_full (P11)", ok, f"{n} balls; candidates 0/1/2/3+: "
           f"{hist.get(0,0)}/{hist.get(1,0)}/{hist.get(2,0)}/{hist.get(3,0)}")


def sqrt_mod(b, p, k):
    """All x mod p^k with x^2 = b mod p^k, by exhaustive search."""
    m = p ** k
    return [x for x in range(m) if (x * x - b) % m == 0]


def check_local_global():
    ok = True
    n = 0
    for p in PRIMES:
        if p == 2:
            roots = [x for x in range(2 ** 12) if ((x * x - 13) * (x * x - 17) * (x * x - 221)) % 2 ** 12 == 0
                     and any((x * x - b) % 2 ** 12 == 0 and x % 2 == 1 for b in (13, 17, 221))]
            ok &= len(roots) > 0
        else:
            k = 4 if p < 50 else 2
            found = False
            for b in (13, 17, 221):
                if b % p == 0:
                    continue
                r1 = [x for x in range(1, p) if (x * x - b) % p == 0]
                if r1:
                    # Hensel lift by the formula of the proof, then verify modulo p^k
                    x = r1[0]
                    for j in range(1, k):
                        t = (-((x * x - b) // p ** j) * pow(2 * x, -1, p)) % p
                        x = x + t * p ** j
                    found |= (x * x - b) % p ** k == 0
            ok &= found
            if p not in (13, 17):
                ok &= any(pow(b, (p - 1) // 2, p) == 1 for b in (13, 17, 221))
        n += 1
    ok &= 17 % 13 == 4 and 13 % 17 == 64 % 17
    c0 = 13 * 17 * 221
    divs = [d for d in range(1, c0 + 1) if c0 % d == 0]
    ok &= not any(((x * x - 13) * (x * x - 17) * (x * x - 221)) == 0 for d in divs for x in (d, -d))
    report("check_local_global (P12)", ok, f"roots at {n} primes p < 200 (mod p^4, p^2, 2^12); "
           f"no rational root among {2 * len(divs)} candidates")


def check_partial():
    ok = True
    n = 0
    for m in range(1, 61):
        for A in range(0, m):
            for B in range(1, m):
                if 2 * A * B >= m:
                    break
                for c in range(m):
                    sols = [(nn, d) for d in range(1, B + 1) if gcd(d, m) == 1
                            for nn in range(-A, A + 1) if gcd(nn, d) == 1 and (nn - c * d) % m == 0]
                    ok &= len(sols) <= 1
                    n += 1
    sols = [(nn, d) for d in (1,) for nn in (-1, 0, 1) if gcd(nn, d) == 1 and (nn - 1 * d) % 2 == 0]
    ok &= sorted(sols) == [(-1, 1), (1, 1)]
    ok &= (1 - 5 * 5) % 6 == 0 and not in_ball(F(1, 5), F(5), F(6))
    report("check_partial (P13)", ok, f"{n} (m, A, B, c) with 2AB < m: at most one solution; sharp at 2AB = m; 1/5")


if __name__ == "__main__":
    check_fundamental_domain()
    check_reduction()
    check_full_image()
    check_split()
    check_translation()
    check_reconstruct_full()
    check_local_global()
    check_partial()
    if FAILURES:
        print("FAILED:", ", ".join(FAILURES))
        sys.exit(1)
    print("all checks passed")
