#!/usr/bin/env python3
"""Numerical checks for docs/proofs/ideles.md (ideles, unit cosets, idele classes, division).

Finite model: for an integer M that is a multiple of N, the unit coset c U(N) is the full preimage of its
image in (Z/M)^x under the surjection Zhat^x -> (Z/M)^x, and that image is
    img(c, N, M) = { u in [0, M) : gcd(u, M) = 1, u = c mod N }.
So containment and equality of cosets, products of cosets (unions of U(M)-cosets) and additive hulls whose
radius divides M are decided by enumeration in Z/M. Nothing here uses the formulas under test except as the
prediction that is compared.
Each check prints one line with counts; the script exits non-zero on any failure.
"""
from fractions import Fraction as F
from math import gcd
from functools import reduce
import random
import sys

random.seed(20260927)
FAILURES = []


def lcm(a, b):
    return a // gcd(a, b) * b


def qgcd(*xs):
    xs = [F(x) for x in xs if x != 0]
    if not xs:
        return F(0)
    den = reduce(lcm, [x.denominator for x in xs])
    return F(reduce(gcd, [abs(int(x * den)) for x in xs]), den)


def primes_upto(n):
    return [p for p in range(2, n + 1) if all(p % q for q in range(2, int(p ** 0.5) + 1))]


PRIMES = primes_upto(200)
PRIMES_5000 = primes_upto(5000)


def vp(x, p):
    x = F(x)
    if x == 0:
        return None
    v, a, b = 0, x.numerator, x.denominator
    while a % p == 0:
        a //= p
        v += 1
    while b % p == 0:
        b //= p
        v -= 1
    return v


def report(name, ok, detail):
    print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}")
    if not ok:
        FAILURES.append(name)


def img(c, N, M):
    assert M % N == 0
    return frozenset(u for u in range(c % N, M, N) if gcd(u, M) == 1)


def canon(c, N):
    if N % 2 == 0 and (N // 2) % 2 == 1:
        N //= 2
    return (c % N, N)


def units(N):
    return [c for c in range(N) if gcd(c, N) == 1] if N > 1 else [0]


# ---------------------------------------------------------------- Section 1

def check_decomposition():
    ok = True
    n = 0
    S = [2, 3, 5, 7, 11]
    for _ in range(500):
        comps = {p: F(random.choice([-1, 1]) * random.randint(1, 60), random.randint(1, 60)) for p in S}
        # the idele: comps at S, 1 elsewhere; its r and u
        r = F(1)
        for p in S:
            r *= F(p) ** vp(comps[p], p)
        ok &= r > 0
        for p in S:
            ok &= vp(comps[p] / r, p) == 0
        for q in PRIMES[:30]:
            if q not in S:
                ok &= vp(F(1) / r, q) == 0
        # uniqueness: another positive rational scale leaves a component of non-zero valuation
        r2 = r * F(random.choice(PRIMES[:8]), 1) ** random.choice([-1, 1])
        bad = [p for p in PRIMES[:30] if vp((comps[p] if p in S else F(1)) / r2, p) != 0]
        ok &= len(bad) > 0
        n += 1
    for _ in range(300):
        q = F(random.randint(-500, 500) or 1, random.randint(1, 500))
        r = F(1)
        for p in PRIMES_5000:
            if (q.numerator * q.denominator) % p == 0:
                r *= F(p) ** vp(q, p)
        ok &= r == abs(q) and all(vp(q / r, p) == 0 for p in PRIMES[:40])
        n += 1
    ok &= all(vp(F(p), p) == 1 for p in PRIMES)  # (p)_p: valuation 1 everywhere, not a unit
    report("check_decomposition (L2, P3)", ok, f"{n} ideles and rationals decomposed; (p)_p has v_p = 1 at all p")


# ---------------------------------------------------------------- Section 2

def check_unit_cosets():
    ok = True
    n = 0
    for N in range(1, 41):
        for c in units(N):
            M = N * random.choice([1, 2, 3, 5, 6, 7])
            I = img(c, N, M)
            ok &= len(I) > 0
            local = frozenset(u for u in range(M) if gcd(u, M) == 1 and all(
                (u - c) % (p ** vp(N, p)) == 0 for p in PRIMES if N % p == 0))
            ok &= I == local
            u0 = min(I)
            ok &= I == frozenset((u0 * w) % M for w in img(1, N, M))
            ball = frozenset(range(c % N, M, N))
            ok &= I <= ball
            if any(M % p == 0 and N % p for p in PRIMES):
                ok &= I != ball and any(gcd(x, M) > 1 for x in ball)
            n += 1
    M = 30
    ok &= img(5, 6, M) == img(2, 3, M) and frozenset(range(5, M, 6)) != frozenset(range(2, M, 3))
    report("check_unit_cosets (L5, P6)", ok,
           f"{n} cosets: local description, coset property, not the ball; 5U(6)=2U(3)")


def check_canonical():
    ok = True
    pairs = 0
    cos = [(c, N) for N in range(1, 17) for c in units(N)]
    for (c, N) in cos:
        for (c2, N2) in cos:
            M = lcm(N, N2)
            I1, I2 = img(c, N, M), img(c2, N2, M)
            (cb, Nb), (cb2, Nb2) = canon(c, N), canon(c2, N2)
            pred_in = Nb % Nb2 == 0 and (c - c2) % Nb2 == 0
            ok &= (I1 <= I2) == pred_in
            ok &= (I1 == I2) == ((cb, Nb) == (cb2, Nb2))
            ok &= bool(I1 & I2) == ((c - c2) % gcd(N, N2) == 0)
            pairs += 1
    for N in range(1, 200, 2):
        ok &= img(1, 2 * N, 2 * N * 3) == img(1, N, 2 * N * 3)
    report("check_canonical (L7, P9)", ok,
           f"{pairs} ordered pairs of cosets (moduli <= 16); U(2N) = U(N) for odd N < 200")


# ---------------------------------------------------------------- Section 3

def check_products():
    ok = True
    n = best_checked = 0
    for _ in range(400):
        N, N2 = random.randint(1, 12), random.randint(1, 12)
        c, c2 = random.choice(units(N)), random.choice(units(N2))
        M = lcm(N, N2)
        prod = frozenset((x * y) % M for x in img(c, N, M) for y in img(c2, N2, M))
        g = gcd(N, N2)
        ok &= prod == img(c * c2, g, M)
        # equal moduli, inverse
        if N == N2:
            ok &= prod == img(c * c2, N, M)
        cs = pow(c, -1, N) if N > 1 else 0
        inv = frozenset(pow(u, -1, M) if M > 1 else 0 for u in img(c, N, M))
        ok &= inv == img(cs, N, M)
        ok &= (1 % M) in frozenset((x * y) % M for x in img(c, N, M) for y in inv)
        # best modulus: all K <= 48 for which the product lies in one coset mod K
        for K in range(1, 49):
            M2 = lcm(M, K)
            P2 = {(x * y) % M2 for x in img(c, N, M2) for y in img(c2, N2, M2)}
            one_coset = len({x % K for x in P2}) == 1
            Kb = canon(0, K)[1]
            gb = canon(0, g)[1]
            ok &= one_coset == (gb % Kb == 0)
            best_checked += 1
        ok &= canon(0, g)[1] == gcd(canon(0, N)[1], canon(0, N2)[1])
        n += 1
    report("check_products (P10, P11)", ok,
           f"{n} pairs; {best_checked} candidate moduli: one coset iff Kbar | gbar")


def check_lte():
    ok = True
    n = 0
    for p in PRIMES[:6]:
        for a in range(1 if p > 2 else 2, 5):
            for t in [1, 2, 3, 5, 7]:
                if t % p == 0:
                    continue
                x = 1 + p ** a * t
                for k in list(range(-12, 0)) + list(range(1, 25)):
                    val = vp(F(x) ** k - 1, p)
                    ok &= val == a + vp(k, p)
                    n += 1
    # the hypothesis a >= 2 at p = 2 is needed: x = 3, k = 2 gives v_2(8) = 3, not 1 + 1
    ok &= vp(F(3) ** 2 - 1, 2) == 3
    report("check_lte (L12)", ok, f"{n} cases of v_p(x^k - 1) = v_p(x - 1) + v_p(k)")


def M_k(N, k):
    _, Nb = canon(0, N)
    M = 1
    for p in PRIMES:
        a = vp(Nb, p) if Nb % p == 0 else 0
        e = vp(k, p) if k % p == 0 else 0
        if p == 2:
            b = a + e if a >= 2 else (2 + e if k % 2 == 0 else 0)
        elif a >= 1:
            b = a + e
        else:
            b = 1 + e if k % (p - 1) == 0 else 0
        M *= p ** b
    return M


def check_power():
    ok = True
    n = strict = 0
    for N in range(1, 17):
        for c in units(N):
            for k in [-4, -3, -2, -1, 1, 2, 3, 4]:
                Mk = M_k(N, k)
                extra = 4
                for p in PRIMES:
                    if p <= max(abs(k) + 2, 7) or N % p == 0:
                        extra *= p
                M = lcm(Mk, N) * extra
                I = img(c, N, M)
                P = {pow(u, k, M) for u in I}
                p0 = min(P)
                D = reduce(gcd, [x - p0 for x in P], M)
                ok &= canon(0, D)[1] == Mk
                ok &= Mk % canon(0, N)[1] == 0
                # the hull coset is the class of chat^k mod Mk, any chat = c mod Nbar coprime to Mk
                Nb = canon(0, N)[1]
                chats = [x for x in range(Mk * Nb) if x % Nb == c % Nb and gcd(x, Mk) == 1][:5]
                ok &= len({pow(x, k, Mk) if Mk > 1 else 0 for x in chats}) == 1
                ok &= all((x - pow(chats[0], k, Mk)) % Mk == 0 for x in P) if Mk > 1 else True
                if k in (1, -1):
                    ok &= Mk == Nb
                full = img(pow(chats[0], k, Mk) if Mk > 1 else 0, Mk, M)
                ok &= P <= full
                strict += P != full
                n += 1
    ok &= M_k(1, 2) == 24
    P2 = {pow(u, 2, 120) for u in img(0, 1, 120)}
    ok &= 97 % 24 == 1 and 97 not in P2 and 97 in img(1, 24, 120)
    report("check_power (P13)", ok, f"{n} (coset, k) cases: hull modulus = M_k; {strict} with P_k strictly inside")


def b_formula(a, p, k):
    """Exponent b_p of Proposition 13 for a = v_p(Nbar) (a != 1 at p = 2)."""
    e = vp(k, p) if k % p == 0 else 0
    if p == 2:
        return a + e if a >= 2 else (2 + e if k % 2 == 0 else 0)
    if a >= 1:
        return a + e
    return 1 + e if k % (p - 1) == 0 else 0


def check_power_local():
    """Proposition 13, prime by prime, for exponents with high valuations (the review found k in [-4, 4] thin):
    the largest b with w^k = 1 mod p^b for all w in V_p(p^a), computed by enumeration modulo p^(b+2)."""
    ok = True
    n = 0
    for p in PRIMES[:11]:
        for a in ([0, 2, 3] if p == 2 else [0, 1, 2]):
            for k in list(range(-30, 0)) + list(range(1, 31)):
                b = b_formula(a, p, k)
                top = b + 2
                if p ** top > 300000:
                    continue
                m = p ** top
                V = [w for w in range(1, m) if w % p and (w - 1) % (p ** a) == 0]
                vals = {pow(w, k, m) for w in V}
                bb = 0
                while bb < top and all((x - 1) % p ** (bb + 1) == 0 for x in vals):
                    bb += 1
                # canonical at 2: b = 0 and b = 1 give the same group
                ok &= (bb == b) or (p == 2 and b == 0 and bb == 1)
                n += 1
    # k = 0: the image is {1}; every U(M) has more than one element (Proposition 13.6)
    for M in range(1, 40):
        p = next(q for q in PRIMES if q > 2 and M % q)
        c0 = max((c for c in range(M) if gcd(c, M) == 1), default=0)
        ok &= len(img(1, M, M * p)) > 1 and {pow(u, 0, M * p) for u in img(c0, M, M * p)} == {1}
    report("check_power_local (P13)", ok, f"{n} (p, a, k) cases with |k| <= 30; k = 0 on 39 moduli")


# ---------------------------------------------------------------- Section 4

def check_norm():
    ok = True
    n = 0
    for _ in range(500):
        r = F(random.randint(1, 5000), random.randint(1, 5000))
        prod = F(1)
        for p in PRIMES_5000:  # every prime of r is below 5000, so this is the product over all p
            if (r.numerator * r.denominator) % p == 0:
                prod *= F(p) ** (-vp(r, p))
        ok &= prod == 1 / r
        n += 1
        q = F(random.choice([-1, 1]) * random.randint(1, 3000), random.randint(1, 3000))
        norm = abs(q)
        for p in PRIMES_5000:
            if (q.numerator * q.denominator) % p == 0:
                norm *= F(p) ** (-vp(q, p))
        ok &= norm == 1
    report("check_norm (P14)", ok, f"{n} scales with product of |r|_p = 1/r; 500 rationals with |q| = 1")


def check_class_map():
    ok = True
    n = 0
    M = 840

    def phi(x):
        xi, r, u = x
        return (abs(xi) / r, (u if xi > 0 else -u) % M)

    def psi(x):
        xi, r, u = x
        return (abs(xi) / r, u % M)

    def times_q(q, x):
        xi, r, u = x
        return (q * xi, abs(q) * r, (u if q > 0 else -u) % M)

    U = img(1, 1, M)
    for _ in range(2000):
        x = (F(random.choice([-1, 1]) * random.randint(1, 99), random.randint(1, 99)),
             F(random.randint(1, 99), random.randint(1, 99)), random.choice(sorted(U)))
        y = (F(random.choice([-1, 1]) * random.randint(1, 99), random.randint(1, 99)),
             F(random.randint(1, 99), random.randint(1, 99)), random.choice(sorted(U)))
        q = F(random.choice([-1, 1]) * random.randint(1, 50), random.randint(1, 50))
        ok &= phi(times_q(q, x)) == phi(x)
        xy = (x[0] * y[0], x[1] * y[1], (x[2] * y[2]) % M)
        ok &= phi(xy) == (phi(x)[0] * phi(y)[0], (phi(x)[1] * phi(y)[1]) % M)
        rep = times_q((1 if x[0] > 0 else -1) / x[1], x)
        ok &= rep[1] == 1 and rep[0] > 0 and phi(rep) == (rep[0], rep[2])
        # kernel: phi(x) = (1, 1) exactly for diagonal rationals (x_inf = +-r, u = sign)
        qq = F(random.choice([-1, 1]) * random.randint(1, 50), random.randint(1, 50))
        ok &= phi((qq, abs(qq), 1 if qq > 0 else M - 1)) == (1, 1)
        n += 1
    ok &= psi(times_q(F(-1), (F(1), F(1), 1))) != psi((F(1), F(1), 1))
    report("check_class_map (P15)", ok,
           f"{n} ideles: invariant under Q^x, homomorphism, kernel; without sign fails")


# ---------------------------------------------------------------- Section 5

def check_idele_to_adele():
    ok = True
    n = strict = 0
    for N in range(1, 41):
        for c in units(N):
            L = lcm(N, 2)
            M = L * 2 * 3 * 5 * 7 * 2
            I = img(c, N, M)
            p0 = min(I)
            D = reduce(gcd, [u - p0 for u in I], M)
            ok &= D == L
            cp = c if N % 2 == 0 else (c if c % 2 else c + N)
            ok &= cp % 2 == 1 and all((u - cp) % L == 0 for u in I)
            ok &= all((u - c) % N == 0 for u in I)
            strict += L != N
            ok &= (L == N) == (N % 2 == 0)
            n += 1
    # the smallest ball depends only on the set: 5 U(6) and 2 U(3)
    ok &= lcm(6, 2) == lcm(3, 2)
    report("check_idele_to_adele (P16)", ok, f"{n} cosets: additive hull radius = lcm(N,2); {strict} with N odd")


# ---------------------------------------------------------------- Section 6

def check_noninvertible():
    ok = True
    n = 0
    for _ in range(300):
        a = F(random.randint(-30, 30), random.randint(1, 12))
        N = F(random.randint(1, 30), random.randint(1, 12))
        bad = a.denominator * N.numerator * N.denominator
        for p in PRIMES[:9]:
            if bad % p == 0:
                continue
            for k in (1, 2):
                ok &= any(a + N * z == 0 or vp(a + N * z, p) >= k for z in range(p ** k))
            n += 1
    # the exception of M4: 1/2 + Zhat has no element with 2-coordinate in 2 Z_2
    ok &= all(vp(F(1, 2) + z, 2) == -1 for z in range(64))
    report("check_noninvertible (P17)", ok, f"{n} (ball, prime) cases with a coordinate 0 mod p^k, k <= 2")


def check_division():
    ok = True
    n = coarser = 0
    for _ in range(3000):
        a = F(random.randint(-6, 6), random.randint(1, 3))
        M = F(random.randint(0, 8), random.randint(1, 2))
        N = random.randint(1, 12)
        c = random.choice(units(N))
        r = F(random.randint(1, 9), random.randint(1, 9))
        if a == 0 and M == 0:
            ok &= qgcd(*[F(0) * w for w in range(1, 5)]) == 0  # S = {0}: the rule gives radius gcd(0, 0) = 0
            continue
        cs = pow(c, -1, N) if N > 1 else 0
        L = lcm(N, 2)
        D = lcm(a.denominator, M.denominator)
        A, B = int(D * a), int(D * M)
        Mod = 1
        for x in (B, abs(A) * L, L):
            if x:
                Mod = lcm(Mod, x)
        Mod *= 6
        zs = range(Mod // B) if B else [0]
        W = img(cs, N, Mod)
        if len(zs) * len(W) > 60000:
            continue
        S = {((A + B * z) * w) % Mod for z in zs for w in W}
        p0 = min(S)
        hull = reduce(gcd, [x - p0 for x in S], Mod)
        pred = qgcd(abs(a) * L, M)
        ok &= F(hull, D) == pred
        e = cs if N % 2 == 0 else (cs if cs % 2 else cs + N)
        ok &= all((x - A * e) % hull == 0 for x in S)
        simple = qgcd(abs(a) * N, M)
        ok &= (pred / simple) in (1, 2)
        coarser += pred != simple
        # Proposition 18: division by an exact rational q (both signs) is an exact scaling of the ball
        q = r * random.choice([1, -1])
        pts = [(a + M * z) / q for z in range(-3, 4)]
        ok &= qgcd(*[x - pts[0] for x in pts]) == M / abs(q) and pts[3] == a / q
        n += 1
    report("check_division (P18, P19)", ok,
           f"{n} cases: hull radius = gcd(|a| L, M); simple ball coarser in {coarser}")


if __name__ == "__main__":
    check_decomposition()
    check_unit_cosets()
    check_canonical()
    check_products()
    check_lte()
    check_power()
    check_power_local()
    check_norm()
    check_class_map()
    check_idele_to_adele()
    check_noninvertible()
    check_division()
    if FAILURES:
        print("FAILED:", ", ".join(FAILURES))
        sys.exit(1)
    print("all checks passed")
