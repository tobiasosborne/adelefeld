#!/usr/bin/env python3
"""Numerical checks for docs/proofs/ideles.md (ideles, unit cosets, idele classes, division).

Finite model: for an integer M that is a multiple of N, the unit coset c U(N) is the full preimage of its
image in (Z/M)^x under the surjection Zhat^x -> (Z/M)^x, and that image is
    img(c, N, M) = { u in [0, M) : gcd(u, M) = 1, u = c mod N }.
So containment and equality of cosets, products of cosets (unions of U(M)-cosets) and additive hulls whose
radius divides M are decided by enumeration in Z/M. Nothing here uses the formulas under test except as the
prediction that is compared.
Each check prints one line with counts; the script exits non-zero on any failure.

Part 1 (lane m0-proofs-ideles, 2026-09-27): the checks of docs/proofs/ideles.md, cited there by name.
Part 2 (lane i-slice1, 2026-09-29): the reference of slice 1 of milestone 2 (docs/api-2.md section 1): unit
cosets with the exact units, the real kernel of Statement E, the idele of a rational, product and inverse of
ideles. Adapted from part 2 of the unreviewed design lane d-ideles (its unit-coset functions and its kernel
B); the kernel here is the one of docs/api-2.md Statement E (B3 uses max(hi - m, m - lo)). The reference
functions are named ref_*; the checks compare them with enumeration and with exact rational end points.
lanes/i-slice1/gen_vectors.py writes the C vectors from the ref_* functions.
Part 3 (lane i-slice2, 2026-09-30): the reference of slice 2 (docs/api-2.md section 2): the real kernel with
an exact rational factor (Statement F), an idele times a rational, the norm, the class map, class product and
inverse, valuations and absolute values; checked against exact points and trial division.
lanes/i-slice2/gen_vectors.py writes its C vectors.

Run: timeout 180 python3 proto/ideles_checks.py          (all checks)
     timeout 180 python3 proto/ideles_checks.py part2    (part 2 only)
     timeout 180 python3 proto/ideles_checks.py part3    (part 3 only)
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


# ======================================================================================================
# Part 2: reference of slice 1 of milestone 2 (docs/api-2.md section 1), lane i-slice1, 2026-09-29
# ======================================================================================================
#
# A unit coset is a pair (c, N): N >= 1, 1 <= c <= N, gcd(c, N) = 1 (conventions.md 5.6, CV-16), or N = 0 and
# c in {1, -1}, the exact unit (M0-D1). The normal form has N != 2 mod 4 (conventions 5.6).
# A real ball is a pair (m, rho) of Fractions, the closed interval [m - rho, m + rho].
# An idele is (ball, r, coset) with 0 outside the ball and r > 0 a Fraction.
# A reference function that can fail returns (status, value); value is None unless the status is OK.

OK, NOT_DETERMINED, NOT_UNIT, DOMAIN = "OK", "NOT_DETERMINED", "NOT_UNIT", "DOMAIN"
RNG = random.Random(20260929)


# ------------------------------------------------------------------ unit cosets

def ref_uc_set(c, N):
    """adf_ucoset_set_fmpz2: any integer c, N >= 0; the modulus is kept as supplied (CV-17)."""
    if N < 0:
        return DOMAIN, None
    if N == 0:
        return (OK, (c, 0)) if c in (1, -1) else (DOMAIN, None)
    if gcd(c, N) != 1:
        return DOMAIN, None
    r = c % N
    return OK, (r if r else N, N)


def ref_uc_is_canonical(u):
    c, N = u
    return (N >= 1 and 1 <= c <= N and gcd(c, N) == 1) or (N == 0 and c in (1, -1))


def ref_uc_is_normal(u):
    return ref_uc_is_canonical(u) and (u[1] == 0 or u[1] % 4 != 2)


def ref_uc_normal(u):
    """Normal form (conventions 5.6; docs/api-2.md Statement B)."""
    c, N = u
    if N == 0:
        return u
    if N % 4 == 2:
        N //= 2
    return ref_uc_set(c, N)[1]


def ref_uc_gcd(N, N2):
    """gcd with gcd(0, N') = N' (SPEC 5); gcd(0, 0) = 0."""
    if N == 0:
        return N2
    if N2 == 0:
        return N
    return gcd(N, N2)


def ref_uc_mul(u, v):
    """adf_ucoset_mul: (c c') U(gcd(N, N')) in normal form (ideles P10, P11; api-2.md A.1, C.1)."""
    g = ref_uc_gcd(u[1], v[1])
    if g == 0:
        return (u[0] * v[0], 0)
    return ref_uc_normal(ref_uc_set(u[0] * v[0], g)[1])


def ref_uc_inv(u):
    """adf_ucoset_inv: c^-1 U(N) in normal form (P10.2; api-2.md A.2, C.2)."""
    c, N = u
    if N == 0:
        return u
    if N == 1:
        return (1, 1)
    return ref_uc_normal(ref_uc_set(pow(c, -1, N), N)[1])


def ref_uc_equal_set(u, v):
    return ref_uc_normal(u) == ref_uc_normal(v)


def ref_uc_contains(u, v):
    """1 if u is inside v (first inside second, SPEC 4.2; ideles P9.1, api-2.md A.3)."""
    (c, N), (c2, N2) = ref_uc_normal(u), ref_uc_normal(v)
    if N2 == 0:
        return N == 0 and c == c2
    if N == 0:
        return (c - c2) % N2 == 0
    return N % N2 == 0 and (c - c2) % N2 == 0


def ref_uc_overlaps(u, v):
    g = ref_uc_gcd(u[1], v[1])
    return u[0] == v[0] if g == 0 else (u[0] - v[0]) % g == 0


def level_set(u, M):
    """Oracle: the image of the set of u in (Z/M)^x; M a multiple of the modulus of u."""
    c, N = u
    if N == 0:
        return frozenset({c % M})
    return img(c, N, M)


def some_cosets(nmax):
    out = [(1, 0), (-1, 0)]
    for N in range(1, nmax + 1):
        for c in range(1, N + 1):
            if gcd(c, N) == 1:
                out.append((c, N))
    return out


def check_api_ucoset():
    """The ref_uc_* functions against enumeration in (Z/M)^x. A level M = 5 L or 12 L or 28 L (L the lcm of
    the moduli) has an odd prime that makes every coset with N >= 1 have at least two elements there, so an
    exact unit and a coset are told apart."""
    ok = True
    n_pred = n_mul = 0
    cos = some_cosets(14)
    for u in cos:
        ok &= ref_uc_is_canonical(u) and ref_uc_is_normal(ref_uc_normal(u))
        ok &= ref_uc_normal(ref_uc_normal(u)) == ref_uc_normal(u)
        for v in cos:
            base = lcm(max(u[1], 1), max(v[1], 1))
            levels = [base * 5, base * 12, base * 28]
            inside_ = all(level_set(u, M) <= level_set(v, M) for M in levels)
            meet = all(level_set(u, M) & level_set(v, M) for M in levels)
            same = all(level_set(u, M) == level_set(v, M) for M in levels)
            ok &= ref_uc_contains(u, v) == inside_
            ok &= ref_uc_overlaps(u, v) == meet
            ok &= ref_uc_equal_set(u, v) == same
            n_pred += 1
            w = ref_uc_mul(u, v)
            for M in levels[:2]:
                prod = frozenset((x * y) % M for x in level_set(u, M) for y in level_set(v, M))
                ok &= prod == level_set(w, M)
            ok &= ref_uc_is_normal(w)
            ok &= (w[1] == 0) == (u[1] == 0 and v[1] == 0)
            n_mul += 1
        iu = ref_uc_inv(u)
        base = max(u[1], 1)
        for M in (base * 5, base * 12):
            ok &= frozenset(pow(x, -1, M) for x in level_set(u, M)) == level_set(iu, M)
            ok &= (1 % M) in level_set(ref_uc_mul(u, iu), M)       # the point 1 is in x * x^-1
        ok &= ref_uc_is_normal(iu) and ref_uc_equal_set(ref_uc_inv(iu), u)
        if u[1] >= 1:
            ok &= ref_uc_mul(u, iu) == ref_uc_normal((1, u[1])) and ref_uc_mul(u, iu) != (1, 0)
    # the examples of SPEC 5 and conventions 5.6, and the refused pairs
    ok &= ref_uc_equal_set((5, 6), (2, 3)) and ref_uc_normal((5, 6)) == (2, 3)
    ok &= ref_uc_set(-1, 6) == (OK, (5, 6)) and ref_uc_set(0, 1) == (OK, (1, 1))
    ok &= not ref_uc_contains((1, 1), (1, 0)) and ref_uc_contains((1, 0), (1, 1))
    ok &= not ref_uc_overlaps((1, 0), (-1, 0)) and ref_uc_overlaps((-1, 0), (3, 4))
    bad = [(2, 4), (0, 6), (3, 6), (5, 0), (0, 0), (2, 0), (-2, 0), (1, -1), (0, 2)]
    ok &= all(ref_uc_set(c, N)[0] == DOMAIN for c, N in bad)
    report("check_api_ucoset (api-2.md A, B, C; P9, P10, P11)", ok,
           f"{n_pred} ordered pairs of {len(cos)} cosets (2 exact): contains, overlaps, equal_set at 3 levels; "
           f"{n_mul} products and {len(cos)} inverses at 2 levels; {len(bad)} invalid pairs refused")


# ------------------------------------------------------------------ the real kernel (api-2.md Statement E)

def exp2(x):
    """ARF_EXP of x != 0: the e with 2^(e-1) <= |x| < 2^e."""
    x = abs(F(x))
    e = x.numerator.bit_length() - x.denominator.bit_length()
    while F(2) ** e <= x:
        e += 1
    while F(2) ** (e - 1) > x:
        e -= 1
    return e


def rd(x, p):
    """Round x > 0 down to p bits (ARF_RND_FLOOR on a positive number)."""
    x = F(x)
    assert x > 0
    s = F(2) ** (exp2(x) - p)
    return (x / s).__floor__() * s


def ru(x, p):
    """Round x > 0 up to p bits (ARF_RND_CEIL on a positive number)."""
    x = F(x)
    assert x > 0
    s = F(2) ** (exp2(x) - p)
    return -((-x / s).__floor__()) * s


def rn(x, p):
    """Round x > 0 to the nearest p-bit number, ties to even (ARF_RND_NEAR)."""
    x = F(x)
    assert x > 0
    s = F(2) ** (exp2(x) - p)
    return round(x / s) * s


def is_dyadic(x):
    d = F(x).denominator
    return d & (d - 1) == 0


def bits(x):
    """Number of bits of the odd mantissa of the dyadic number x != 0."""
    x = abs(F(x))
    assert is_dyadic(x)
    n = x.numerator
    while n % 2 == 0:
        n //= 2
    return n.bit_length()


def kernel_B(lo, hi, sign, p):
    """Kernel B of api-2.md Statement E5: a ball that contains sign * [lo, hi] and excludes 0, or
    NOT_DETERMINED. 0 < lo <= hi dyadic with at most p bits. Returns (status, (m, rho), step). The radius
    ru(., 30) models a mag; the C radius may be a few ulps larger (mag.rst:15), so B3 and B4 may differ
    from the C in borderline cases, never the status."""
    assert 0 < lo <= hi and bits(lo) <= p and bits(hi) <= p
    if exp2(hi) - exp2(lo) > p:
        return NOT_DETERMINED, None, "B1"
    if lo == hi:
        return OK, (sign * lo, F(0)), "B2"
    m = rn((lo + hi) / 2, p)
    rho = ru(max(hi - m, m - lo), 30)
    if m > rho:
        return OK, (sign * m, rho), "B3"
    rho = ru((hi - lo) / 2, 30)
    return OK, (sign * (lo + rho), rho), "B4"


def abs_bounds(ball, p):
    """Statement E1: l = RD_p(|m| - rho), h = RU_p(|m| + rho)."""
    m, r = ball
    return rd(abs(m) - r, p), ru(abs(m) + r, p)


def sgn(ball):
    return 1 if ball[0] > 0 else -1


def ref_real_mul(x, y, p):
    """E2 and B. Returns (status, ball, step, lo, hi, sign)."""
    p = max(p, 2)
    (lx, ux), (ly, uy) = abs_bounds(x, p), abs_bounds(y, p)
    lo, hi, s = rd(lx * ly, p), ru(ux * uy, p), sgn(x) * sgn(y)
    return kernel_B(lo, hi, s, p) + (lo, hi, s)


def ref_real_inv(x, p):
    """E3 and B."""
    p = max(p, 2)
    lx, ux = abs_bounds(x, p)
    lo, hi, s = rd(1 / ux, p), ru(1 / lx, p), sgn(x)
    return kernel_B(lo, hi, s, p) + (lo, hi, s)


def ref_real_rat(q, p):
    """E4 and B, q != 0."""
    p = max(p, 2)
    q = F(q)
    lo, hi, s = rd(abs(q), p), ru(abs(q), p), (1 if q > 0 else -1)
    return kernel_B(lo, hi, s, p) + (lo, hi, s)


def ball_product_arb(x, y, p):
    """Model of the ball product of arb (the alternative that decision D2-2 rejects): midpoint m1 m2 rounded
    to nearest, radius |m1| r2 + |m2| r1 + r1 r2 plus the rounding error."""
    (m1, r1), (m2, r2) = x, y
    t = m1 * m2
    m = rn(t, p) if t > 0 else -rn(-t, p)
    return m, abs(m1) * r2 + abs(m2) * r1 + r1 * r2 + abs(m - t)


def random_ball(rng, width_exp, big=False):
    """A ball that excludes 0; width_exp controls how close the near end is to 0."""
    man = rng.randint(1, 1 << (400 if big else 20))
    m = F(man, 1 << rng.randint(0, 40)) * rng.choice([1, -1])
    if big and rng.random() < 0.5:
        m *= F(2) ** rng.randint(-3000, 3000)
    gapexp = rng.randint(0, width_exp)
    r = F(0)
    if gapexp:
        r = rd(abs(m) * (1 - F(1, 1 << gapexp)), 30)
    return m, r


def spec5_example():
    """SPEC 5: x = y = 1 +/- (1 - 2^-30)."""
    return (F(1), 1 - F(1, 1 << 30))


def check_api_real_kernel():
    ok = True
    n = nd = n_exact = zone_ok = zone_nd = n_exact_mul = 0
    steps = {}
    for _ in range(4000):
        p = RNG.choice([2, 3, 4, 8, 16, 30, 53, 64, 128, 300])
        pp = max(p, 2)
        big = RNG.random() < 0.1
        x = random_ball(RNG, RNG.choice([0, 3, 40, 200]), big)
        y = random_ball(RNG, RNG.choice([0, 3, 40, 200]), big)
        q = F(RNG.randint(1, 10 ** 12), RNG.randint(1, 10 ** 12)) * RNG.choice([1, -1])
        ax = (abs(x[0]) - x[1], abs(x[0]) + x[1])
        ay = (abs(y[0]) - y[1], abs(y[0]) + y[1])
        cases = [
            (ref_real_mul(x, y, p), ax[0] * ay[0], ax[1] * ay[1], sgn(x) * sgn(y)),
            (ref_real_inv(x, p), 1 / ax[1], 1 / ax[0], sgn(x)),
            (ref_real_rat(q, p), abs(q), abs(q), 1 if q > 0 else -1),
        ]
        for (st, ball, step, lo, hi, s), L, H, s_true in cases:
            n += 1
            steps[step] = steps.get(step, 0) + 1
            ok &= 0 < lo <= L <= H <= hi and s == s_true           # E1 to E4: the end points
            if st == OK:
                m, rr = ball
                ok &= m - rr <= s * L <= m + rr and m - rr <= s * H <= m + rr   # enclosure of the set
                ok &= (m - rr > 0) if s > 0 else (m + rr < 0)                  # 0 is excluded
                ok &= bits(m) <= 2 * pp + 30                                   # E5, size of the midpoint
                n_exact += step == "B2"
            else:
                ok &= st == NOT_DETERMINED
                nd += 1
            if hi < F(2) ** (pp - 1) * lo:                                     # E6
                ok &= st == OK
                zone_ok += 1
            if hi >= F(2) ** (pp + 1) * lo:
                ok &= st == NOT_DETERMINED
                zone_nd += 1
        # exact inputs whose product fits in p bits: an exact result
        if x[1] == 0 and y[1] == 0:
            t = x[0] * y[0]
            if bits(t) <= pp and bits(x[0]) <= pp and bits(y[0]) <= pp:
                st, ball, step = ref_real_mul(x, y, p)[:3]
                ok &= st == OK and ball == (t, 0)
                n_exact_mul += 1
    ok &= all(ref_real_rat(F(k, 7), p)[0] == OK for k in (-3, 1, 10 ** 30) for p in (0, 1, 2, 3))
    # SPEC 5 example: the ball product contains 0 at every precision, the kernel does not above a threshold
    x = spec5_example()
    spec = {}
    for p in range(2, 140):
        m, r = ball_product_arb(x, x, p)
        ok &= m - r <= 0
        spec[p] = ref_real_mul(x, x, p)[0]
    first_ok = min(p for p in spec if spec[p] == OK)
    ok &= all(spec[p] == (OK if p >= first_ok else NOT_DETERMINED) for p in spec)
    st, ball, step, lo, hi, s = ref_real_mul(x, x, 128)
    ok &= st == OK and lo == F(1, 1 << 60) and hi == (2 - F(1, 1 << 30)) ** 2
    report("check_api_real_kernel (api-2.md E)", ok,
           f"{n} results (mul, inv, rational): end points, enclosure, sign, size; {nd} NOT_DETERMINED, "
           f"{n_exact} exact; steps {dict(sorted(steps.items()))}; promised OK in {zone_ok}, promised "
           f"NOT_DETERMINED in {zone_nd}; {n_exact_mul} exact products of exact inputs; SPEC 5 example: "
           f"NOT_DETERMINED below prec {first_ok}, OK from {first_ok} (checked 2 to 139), at 128 by {step}")


# ------------------------------------------------------------------ ideles (api-2.md Statement D)

def ref_idele_set_rat(q, p):
    q = F(q)
    if q == 0:
        return NOT_UNIT, None
    st, ball = ref_real_rat(q, p)[:2]
    assert st == OK
    return OK, (ball, abs(q), (1 if q > 0 else -1, 0))


def ref_idele_mul(x, y, p):
    st, ball = ref_real_mul(x[0], y[0], p)[:2]
    if st != OK:
        return st, None
    return OK, (ball, x[1] * y[1], ref_uc_mul(x[2], y[2]))


def ref_idele_inv(x, p):
    st, ball = ref_real_inv(x[0], p)[:2]
    if st != OK:
        return st, None
    return OK, (ball, 1 / x[1], ref_uc_inv(x[2]))


def inside(point, ball):
    return ball[0] - ball[1] <= point <= ball[0] + ball[1]


def check_api_idele():
    """Points of the input sets (a real point, the content, a unit residue at a level M) multiplied or inverted
    exactly; the result point must lie in the result set."""
    ok = True
    n = npts = nd = 0
    cos = some_cosets(12)
    for _ in range(1500):
        p = RNG.choice([2, 16, 53, 128])
        x = (random_ball(RNG, RNG.choice([0, 3, 20])), F(RNG.randint(1, 40), RNG.randint(1, 40)), RNG.choice(cos))
        y = (random_ball(RNG, RNG.choice([0, 3, 20])), F(RNG.randint(1, 40), RNG.randint(1, 40)), RNG.choice(cos))
        M = lcm(max(x[2][1], 1), max(y[2][1], 1)) * 60
        for name, (st, z) in (("mul", ref_idele_mul(x, y, p)), ("inv", ref_idele_inv(x, p))):
            n += 1
            if st != OK:
                ok &= st == NOT_DETERMINED
                nd += 1
                continue
            ok &= not inside(0, z[0]) and z[1] > 0 and ref_uc_is_normal(z[2])
            for _k in range(4):
                a = x[0][0] + x[0][1] * F(RNG.randint(-8, 8), 8)
                b = y[0][0] + y[0][1] * F(RNG.randint(-8, 8), 8)
                ua = RNG.choice(sorted(level_set(x[2], M)))
                ub = RNG.choice(sorted(level_set(y[2], M)))
                if name == "mul":
                    pt = (a * b, x[1] * y[1], (ua * ub) % M)
                else:
                    pt = (1 / a, 1 / x[1], pow(ua, -1, M))
                ok &= inside(pt[0], z[0]) and pt[1] == z[1] and pt[2] in level_set(z[2], M)
                npts += 1
    for q in (F(-3, 2), F(5), F(-1), F(1, 3)):
        st, z = ref_idele_set_rat(q, 64)
        ok &= st == OK and inside(q, z[0]) and z[1] == abs(q) and z[2] == ((1 if q > 0 else -1), 0)
    ok &= ref_idele_set_rat(0, 64)[0] == NOT_UNIT
    x = ((F(-3), F(1)), F(3, 2), (5, 12))
    st, z = ref_idele_mul(x, ref_idele_inv(x, 64)[1], 64)
    ok &= st == OK and inside(1, z[0]) and z[1] == 1 and z[2] == (1, 12) and z[0][1] > 0
    report("check_api_idele (api-2.md D)", ok,
           f"{n} results of mul and inv; {npts} exact points of the inputs, each result point inside the "
           f"result; {nd} NOT_DETERMINED")


# ======================================================================================================
# Part 3: reference of slice 2 of milestone 2 (docs/api-2.md section 2), lane i-slice2, 2026-09-30
# ======================================================================================================
#
# Adapted from the unreviewed part 2 of lane d-ideles (worktree agent-acc17965b8910c1f2,
# proto/ideles_checks.py lines 879-888 ref_real_scale / ref_real_div_pos, 1022-1029 ref_idele_mul_rat,
# 1045-1061 ref_idele_class / ref_idele_valuation / ref_idele_abs_rat, 1131-1237 the checks). Taken: the
# formulas of the class map (t = |X|/r, unit sign(X) u), of mul_rat and of the valuation. Changed: the scaling
# is by a/b with integers (Statement F), the result carries the rounded ends; the valuation is at a place
# (DOMAIN at "inf", no DOMAIN for a composite, which a place cannot hold); the checks use exact points of the
# input sets and the unit cosets at levels M, as part 2 does.
# A class is (ball, coset) with the ball positive. A place is a prime or the string "inf".

INF = "inf"


def ref_real_scale(x, a, b, sign, p):
    """Statement F and kernel B: the ball x times a/b (a, b >= 1 integers), with the sign `sign`.
    Returns (status, ball, step, lo, hi, sign)."""
    p = max(p, 2)
    lx, ux = abs_bounds(x, p)
    lo, hi = rd(lx * a / F(b), p), ru(ux * a / F(b), p)
    return kernel_B(lo, hi, sign, p) + (lo, hi, sign)


def ref_idele_mul_rat(x, q, p):
    """adf_idele_mul_rat (Statement H)."""
    q = F(q)
    if q == 0:
        return NOT_UNIT, None
    sq = 1 if q > 0 else -1
    st, ball = ref_real_scale(x[0], abs(q.numerator), q.denominator, sgn(x[0]) * sq, p)[:2]
    if st != OK:
        return st, None
    return OK, (ball, x[1] * abs(q), ref_uc_mul(x[2], (sq, 0)))


def ref_idele_norm(x, p):
    """adf_idele_norm: |X| / r (P14.2; Statements F, I.4)."""
    r = x[1]
    return ref_real_scale(x[0], r.denominator, r.numerator, 1, p)


def ref_idele_class(x, p):
    """adf_idclass_set_idele: (|X| / r, sign(X) u) (P15; Statement G.4)."""
    st, ball = ref_idele_norm(x, p)[:2]
    if st != OK:
        return st, None
    return OK, (ball, ref_uc_mul(x[2], (sgn(x[0]), 0)))


def ref_idclass_mul(x, y, p):
    st, ball = ref_real_mul(x[0], y[0], p)[:2]
    if st != OK:
        return st, None
    return OK, (ball, ref_uc_mul(x[1], y[1]))


def ref_idclass_inv(x, p):
    st, ball = ref_real_inv(x[0], p)[:2]
    if st != OK:
        return st, None
    return OK, (ball, ref_uc_inv(x[1]))


def ref_idele_valuation_at(x, place):
    """v_p(r) by repeated division of the numerator and the denominator (P14.4)."""
    if place == INF:
        return DOMAIN, None
    return OK, vp(x[1], place)


def ref_idele_abs_at(x, place):
    st, v = ref_idele_valuation_at(x, place)
    return (st, None) if st != OK else (OK, F(place) ** (-v))


def ref_idele_abs_inf(x):
    m, rho = x[0]
    return (abs(m), rho)


def trial_factor(n):
    """Oracle independent of vp: the factorisation of n >= 1 by trial division (n below about 10^12)."""
    out, p = {}, 2
    while p * p <= n:
        while n % p == 0:
            out[p] = out.get(p, 0) + 1
            n //= p
        p += 1
    if n > 1:
        out[n] = out.get(n, 0) + 1
    return out


def random_point(x, M):
    """An exact point (xi, r, unit residue modulo M) of the idele value x; M a multiple of its modulus."""
    (m, rho), r, u = x
    return (m + rho * F(RNG.randint(-8, 8), 8), r, RNG.choice(sorted(level_set(u, M))))


def check_api_classes():
    """The class map, class product and inverse, mul_rat, norm, valuation and absolute values of the reference
    against exact points of the input sets and the unit cosets at levels M (Statements F to I; P14, P15)."""
    ok = True
    n = npts = nd = n_inv = 0
    cos = some_cosets(12)
    Phi = lambda pt, M: (abs(pt[0]) / pt[1], (pt[2] if pt[0] > 0 else -pt[2]) % M)
    for _ in range(1500):
        p = RNG.choice([2, 8, 16, 53, 128])
        x = (random_ball(RNG, RNG.choice([0, 3, 20, 70])), F(RNG.randint(1, 60), RNG.randint(1, 60)),
             RNG.choice(cos))
        y = (random_ball(RNG, RNG.choice([0, 3, 20])), F(RNG.randint(1, 60), RNG.randint(1, 60)), RNG.choice(cos))
        q = F(RNG.choice([-1, 1]) * RNG.randint(1, 60), RNG.randint(1, 60))
        M = lcm(max(x[2][1], 1), max(y[2][1], 1)) * 60
        # the class of x: every class of a point lies in it; the unit is exactly sign(X) u
        st, cx = ref_idele_class(x, p)
        n += 1
        if st != OK:
            ok &= st == NOT_DETERMINED
            nd += 1
            continue
        ok &= cx[0][0] - cx[0][1] > 0 and ref_uc_is_normal(cx[1])
        ok &= level_set(cx[1], M) == frozenset((w if sgn(x[0]) > 0 else -w) % M for w in level_set(x[2], M))
        for _k in range(4):
            pt = random_point(x, M)
            t, w = Phi(pt, M)
            ok &= inside(t, cx[0]) and w in level_set(cx[1], M)
            npts += 1
        # the norm is the t of the class; valuations and absolute values against trial division
        ok &= ref_idele_norm(x, p)[:2] == (OK, cx[0])
        fn, fd = trial_factor(x[1].numerator), trial_factor(x[1].denominator)
        prod = F(1)
        for pr in set(fn) | set(fd) | {2, 3, 5, 7, 11, 13}:
            v = fn.get(pr, 0) - fd.get(pr, 0)
            ok &= ref_idele_valuation_at(x, pr) == (OK, v) and ref_idele_abs_at(x, pr) == (OK, F(pr) ** (-v))
            prod *= F(pr) ** (-v)
        ok &= prod == 1 / x[1]                                               # P14.2
        ok &= ref_idele_valuation_at(x, INF)[0] == DOMAIN and ref_idele_abs_at(x, INF)[0] == DOMAIN
        ai = ref_idele_abs_inf(x)
        ok &= ai[0] - ai[1] > 0 and all(inside(abs(random_point(x, M)[0]), ai) for _k in range(3))
        # mul_rat: pointwise, and the class of q x is the class of x (G.3)
        st, z = ref_idele_mul_rat(x, q, p)
        if st == OK:
            for _k in range(4):
                pt = random_point(x, M)
                qpt = (pt[0] * q, pt[1] * abs(q), (pt[2] if q > 0 else -pt[2]) % M)
                ok &= inside(qpt[0], z[0]) and qpt[1] == z[1] and qpt[2] in level_set(z[2], M)
                ok &= Phi(qpt, M) == Phi(pt, M)                              # kernel Q^x, pointwise
            st2, cz = ref_idele_class(z, p)
            if st2 == OK:
                ok &= ref_uc_equal_set(cz[1], cx[1]) and abs(cz[0][0] - cx[0][0]) <= cz[0][1] + cx[0][1]
        else:
            ok &= st == NOT_DETERMINED
        # class product and inverse: pointwise (the group law of R_{>0} x Zhat^x, P15.1)
        st, cy = ref_idele_class(y, p)
        if st == OK:
            for name, (st3, c3) in (("mul", ref_idclass_mul(cx, cy, p)), ("inv", ref_idclass_inv(cx, p))):
                if st3 != OK:
                    ok &= st3 == NOT_DETERMINED
                    continue
                ok &= c3[0][0] - c3[0][1] > 0 and ref_uc_is_normal(c3[1])
                n_inv += 1
                for _k in range(4):
                    a, b = Phi(random_point(x, M), M), Phi(random_point(y, M), M)
                    pt = (a[0] * b[0], (a[1] * b[1]) % M) if name == "mul" else (1 / a[0], pow(a[1], -1, M))
                    ok &= inside(pt[0], c3[0]) and pt[1] in level_set(c3[1], M)
    # the product formula on rationals: the norm of the idele of q contains 1, exactly when the ball is exact
    n_q = n_exact = 0
    for _ in range(600):
        q = F(RNG.choice([-1, 1]) * RNG.randint(1, 3000), RNG.choice([1, 2, 8, 64, 3, 7, 35, 3000]))
        p = RNG.choice([2, 8, 16, 53, 128])
        st, x = ref_idele_set_rat(q, p)
        st, t = ref_idele_norm(x, p)[:2]
        ok &= st == OK and inside(1, t)
        st, c = ref_idele_class(x, p)
        ok &= st == OK and inside(1, c[0]) and c[1] == (1, 0)
        if x[0][1] == 0:
            ok &= t == (F(1), F(0)) and c[0] == (F(1), F(0))
            n_exact += 1
        n_q += 1
    # examples: the idele of -6/35 (tests/julia/idclass.jl); the sign on the unit (P15.3)
    x = ref_idele_set_rat(F(-6, 35), 64)[1]
    ok &= [ref_idele_valuation_at(x, pr)[1] for pr in (2, 3, 5, 7, 11)] == [1, 1, -1, -1, 0]
    ok &= [ref_idele_abs_at(x, pr)[1] for pr in (2, 3, 5, 7, 11)] == [F(1, 2), F(1, 3), 5, 7, 1]
    ok &= ref_idele_class(x, 64)[1][1] == (1, 0) and inside(1, ref_idele_class(x, 64)[1][0])
    ok &= ref_idele_class(((F(1), F(0)), F(1), (-1, 0)), 64) == (OK, ((F(1), F(0)), (-1, 0)))
    ok &= ref_idele_class(((F(-1), F(0)), F(1), (-1, 0)), 64) == (OK, ((F(1), F(0)), (1, 0)))
    ok &= ref_idele_class(((F(-5, 2), F(1, 4)), F(3, 2), (5, 36)), 64)[1][1] == (31, 36)
    ok &= ref_idele_mul_rat(x, 0, 64)[0] == NOT_UNIT
    report("check_api_classes (api-2.md F to I; P14, P15)", ok,
           f"{n} ideles: class map, norm, valuations, mul_rat; {npts} exact points in the class; {nd} "
           f"NOT_DETERMINED; {n_inv} class products and inverses; {n_q} rationals: norm contains 1, exact 1 in "
           f"{n_exact}")


PART1 = [check_decomposition, check_unit_cosets, check_canonical, check_products, check_lte, check_power,
         check_power_local, check_norm, check_class_map, check_idele_to_adele, check_noninvertible,
         check_division]
PART2 = [check_api_ucoset, check_api_real_kernel, check_api_idele]
PART3 = [check_api_classes]

if __name__ == "__main__":
    if sys.argv[1:] == ["part3"]:
        todo = PART3
    else:
        todo = PART2 if sys.argv[1:] == ["part2"] else PART1 + PART2 + PART3
    for f in todo:
        f()
    if FAILURES:
        print("FAILED:", ", ".join(FAILURES))
        print(f"{len(todo)} checks, {len(FAILURES)} failed")
        sys.exit(1)
    print("all checks passed")
    print(f"{len(todo)} checks")
