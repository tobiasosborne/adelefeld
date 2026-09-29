#!/usr/bin/env python3
"""Numerical checks and reference algorithms for docs/proofs/solvers.md (milestone S).

Part 1 (S.3): partial rational reconstruction.
Part 2 (S.1): linear systems modulo N, kernels, Howell form, certificates, the adelic form.
Part 3 (S.2): roots at a prime by Hensel lifting; real roots with a completeness status.

Two kinds of function stand next to each other:
  * the reference algorithms, written as solvers.md specifies them (recon_partial, howell, linsolve_mod,
    padic_roots, real_roots, and the checkers of their certificates);
  * brute-force oracles that use the definition only (enumeration of all n/d within the bounds, of all of
    (Z/N)^c, of all spans, of all residues modulo p^K; exact rational arithmetic).
Each check compares the two and prints one line with counts. The script exits non-zero on any failure.

The checks named probe_* call the installed FLINT (libflint.so, through ctypes) and record what FLINT does;
they are wrapper tests in the sense of PLAN.md section 7 and are skipped with a line "SKIP" when the library
cannot be loaded. They fail only where solvers.md states a property of FLINT as used by the design.

Run: python3 proto/solvers_checks.py        (about one minute; one core)
"""
from fractions import Fraction as F
from math import gcd, isqrt
from itertools import product
import ctypes
import ctypes.util
import random
import sys
import time

random.seed(20260929)
FAILURES = []
T0 = time.time()

OK = "OK"
NOT_DETERMINED = "NOT_DETERMINED"
NOT_UNIQUE = "NOT_UNIQUE"
NO_SOLUTION = "NO_SOLUTION"
DOMAIN = "DOMAIN"
UNSUPPORTED = "UNSUPPORTED"
LIMIT = "LIMIT"


def report(name, ok, detail):
    print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}")
    if not ok:
        FAILURES.append(name)


def skip(name, detail):
    print(f"SKIP {name}: {detail}")


def vp(x, p):
    """p-adic valuation of a non-zero rational."""
    x = F(x)
    v, a, b = 0, x.numerator, x.denominator
    while a % p == 0:
        a //= p
        v += 1
    while b % p == 0:
        b //= p
        v -= 1
    return v


def prime_factors(n):
    out, q = [], 2
    while q * q <= n:
        if n % q == 0:
            out.append(q)
            while n % q == 0:
                n //= q
        q += 1
    if n > 1:
        out.append(n)
    return out


def xgcd(a, b):
    """(g, s, t) with s a + t b = g = gcd(a, b) >= 0."""
    r0, r1, s0, s1, t0, t1 = a, b, 1, 0, 0, 1
    while r1 != 0:
        q = r0 // r1
        r0, r1 = r1, r0 - q * r1
        s0, s1 = s1, s0 - q * s1
        t0, t1 = t1, t0 - q * t1
    if r0 < 0:
        r0, s0, t0 = -r0, -s0, -t0
    return r0, s0, t0


# ---------------------------------------------------------------------------------------------------------
# FLINT through ctypes (probes only)
# ---------------------------------------------------------------------------------------------------------

def load_flint():
    name = ctypes.util.find_library("flint")
    if name is None:
        return None
    try:
        lib = ctypes.CDLL(name)
    except OSError:
        return None
    try:
        lib.flint_version  # noqa: B018
    except AttributeError:
        pass
    return lib


FLINT = load_flint()
SMALL = 1 << 61          # an fmpz below 2^62 in absolute value is stored inline as a slong (fmpz.h, COEFF_MAX)


def flint_version():
    if FLINT is None:
        return None
    try:
        return (ctypes.c_char * 8).in_dll(FLINT, "flint_version").value.decode()   # char flint_version[]
    except (ValueError, AttributeError):
        return "unknown"


# =========================================================================================================
# Part 1. S.3: partial rational reconstruction
# =========================================================================================================

def sol_brute(m, c, A, B, with_coprime=True):
    """Definition 1.1 by enumeration: all (n, d), d > 0, gcd(n, d) = 1, [gcd(d, m) = 1,] n = c d mod m."""
    out = set()
    for d in range(1, B + 1):
        if with_coprime and gcd(d, m) != 1:
            continue
        for n in range(-A, A + 1):
            if gcd(n, d) == 1 and (n - c * d) % m == 0:
                out.add((n, d))
    return out


def eea_pair(m, c, A):
    """The certificate pair of Lemma 1.4 for 0 <= A < m: (R', T', R, T), consecutive rows of the extended
    Euclidean algorithm on (m, c mod m), with R <= A < R'."""
    assert m >= 1 and 0 <= A < m
    r0, t0, r1, t1 = m, 0, c % m, 1
    while r1 > A:
        q = r0 // r1
        r0, r1 = r1, r0 - q * r1
        t0, t1 = t1, t0 - q * t1
    return r0, t0, r1, t1


def cert_pair_ok(m, c, A, cert):
    """The conditions C1 to C4 of Definition 1.3; what a checker verifies."""
    Rp, Tp, R, T = cert
    c1 = (R - c * T) % m == 0 and (Rp - c * Tp) % m == 0
    c2 = abs(T * Rp - Tp * R) == m
    c3 = 0 <= R <= A < Rp
    c4 = T != 0 and T * Tp <= 0
    return c1 and c2 and c3 and c4


def lattice_points(m, c, A, B, cert, xmax=None):
    """Proposition 1.5: the points (n, d) of the lattice n = c d mod m with |n| <= A, 0 < d <= B, in the order of
    the enumeration (x = 1, 2, ...; for each x the at most two values of y). Generator."""
    Rp, Tp, R, T = cert
    sg = 1 if T > 0 else -1
    aT, aTp = abs(T), abs(Tp)
    X = B // aT
    if xmax is not None:
        X = min(X, xmax)
    for x in range(1, X + 1):
        ylo = max(0, -((A - x * R) // Rp))          # ceil((x R - A) / R')
        yhi = (x * R + A) // Rp
        for y in range(ylo, yhi + 1):
            d = x * aT + y * aTp
            if d > B:
                break
            n = sg * (x * R - y * Rp)
            yield n, d


def recon_partial(m, c, A, B, limit):
    """Algorithm R of solvers.md 1.7. Returns (status, solutions found, certificate or None).
    status OK: exactly one solution, returned. NO_SOLUTION, NOT_UNIQUE: proved. NOT_DETERMINED: the search
    was cut by the limit with fewer than two solutions found. DOMAIN: m < 1. An empty box (A < 0 or B < 1)
    is NO_SOLUTION; a limit below 0 is taken as 0."""
    if m < 1:
        return DOMAIN, [], None
    if A < 0 or B < 1:
        return NO_SOLUTION, [], None
    limit = max(limit, 0)
    c %= m
    if A >= m:                                            # Proposition 1.6 (a)
        return NOT_UNIQUE, [(c, 1), (c - m, 1)], None
    cert = eea_pair(m, c, A)
    assert cert_pair_ok(m, c, A, cert)
    Rp, Tp, R, T = cert
    if abs(T) > B:                                        # (b)
        return NO_SOLUTION, [], cert
    sg = 1 if T > 0 else -1
    if 2 * A * B < m:                                     # (c)
        if gcd(R, T) == 1:
            return OK, [(sg * R, abs(T))], cert
        return NO_SOLUTION, [], cert
    found = []                                            # (d)
    X = B // abs(T)
    for n, d in lattice_points(m, c, A, B, cert, xmax=limit):
        if gcd(n, d) == 1:
            found.append((n, d))
            if len(found) == 2:
                return NOT_UNIQUE, found, cert
    if X > limit:
        return NOT_DETERMINED, found, cert
    if len(found) == 1:
        return OK, found, cert
    return NO_SOLUTION, [], cert


def check_s3_coprime():
    """Lemma 1.2: for a reduced fraction the congruence forces gcd(d, m) = 1; the two definitions of the set of
    solutions agree; the local meaning (n/d in c + m Z_p for p | m) agrees."""
    cases = bad = pairs = 0
    for m in range(1, 31):
        ps = prime_factors(m)
        for c in range(m):
            A, B = 12, 12
            s1 = sol_brute(m, c, A, B, True)
            s2 = sol_brute(m, c, A, B, False)
            cases += 1
            if s1 != s2:
                bad += 1
            for d in range(1, B + 1):
                for n in range(-A, A + 1):
                    if gcd(n, d) != 1:
                        continue
                    pairs += 1
                    q = F(n, d)
                    local = all(q == c or vp(q - c, p) >= vp(m, p) for p in ps) and \
                        all(vp(F(d), p) == 0 for p in ps)
                    # membership in the ball at p | m needs n/d in Z_p as well: v_p(n/d - c) >= v_p(m) >= 1
                    # forces it; the second conjunct is then implied and is tested separately below
                    local_only = all(q == c or vp(q - c, p) >= vp(m, p) for p in ps)
                    if local != local_only:
                        bad += 1
                    if ((n, d) in s1) != local_only:
                        bad += 1
    report("check_s3_coprime", bad == 0, f"{cases} problems (m <= 30), {pairs} reduced pairs, {bad} failures")


def check_s3_certificate():
    """Lemma 1.4: the rows of the Euclidean algorithm satisfy C1 to C4; gcd(R, T) = gcd(T, m)."""
    n = bad = 0
    for m in range(1, 200):
        for c in range(m):
            for A in (0, 1, 2, 3, 5, m // 3, m // 2, m - 1):
                if not 0 <= A < m:
                    continue
                cert = eea_pair(m, c, A)
                n += 1
                if not cert_pair_ok(m, c, A, cert):
                    bad += 1
                if gcd(cert[2], cert[3]) != gcd(cert[3], m):
                    bad += 1
    big = 0
    for _ in range(2000):
        m = random.getrandbits(random.choice((64, 200, 1000))) + 1
        c = random.randrange(-3 * m, 3 * m)
        A = random.randrange(0, m)
        cert = eea_pair(m, c, A)
        big += 1
        if not cert_pair_ok(m, c, A, cert) or gcd(cert[2], cert[3]) != gcd(cert[3], m):
            bad += 1
    # negative control: a pair with a wrong determinant or a wrong sign must be refused
    ctrl = 0
    for m, c, A in ((35, 12, 5), (97, 40, 9), (64, 24, 7)):
        Rp, Tp, R, T = eea_pair(m, c, A)
        for wrong in ((Rp + m, Tp, R, T), (Rp, Tp, R, -T), (Rp, Tp, R + m, T), (Rp, -Tp if Tp else 1, R, T),
                      (R, T, Rp, Tp)):
            if wrong != (Rp, Tp, R, T) and cert_pair_ok(m, c, A, wrong):
                # a changed tuple may still be a valid pair; it is wrong only if the set it gives differs
                pts = {p for p in lattice_points(m, c, A, 30, wrong)}
                ref = {(n, d) for d in range(1, 31) for n in range(-A, A + 1) if (n - c * d) % m == 0}
                if pts != ref:
                    ctrl += 1
    report("check_s3_certificate", bad == 0 and ctrl == 0,
           f"{n} small and {big} large triples (m, c, A), {bad} failures; accepted wrong pairs: {ctrl}")


def check_s3_param():
    """Proposition 1.5: the parametrisation gives exactly the lattice points of the box, each once, at most two
    per x, and none when |T| > B."""
    n = bad = pts = 0
    for m in range(1, 41):
        for c in range(m):
            for A in range(0, m):
                cert = eea_pair(m, c, A)
                for B in (1, 2, 3, 5, 8, 13, m, 2 * m + 1):
                    ref = {(a, d) for d in range(1, B + 1) for a in range(-A, A + 1) if (a - c * d) % m == 0}
                    lst = list(lattice_points(m, c, A, B, cert))
                    n += 1
                    pts += len(lst)
                    if len(lst) != len(set(lst)) or set(lst) != ref:
                        bad += 1
                    if abs(cert[3]) > B and ref:
                        bad += 1
                    if len(lst) > 2 * (B // abs(cert[3])):
                        bad += 1
    report("check_s3_param", bad == 0,
           f"{n} boxes (m <= 40, all c, all A < m), {pts} lattice points, {bad} failures")


def classify(sols):
    return OK if len(sols) == 1 else (NO_SOLUTION if not sols else NOT_UNIQUE)


def check_s3_complete():
    """Proposition 1.6 and Algorithm R: for every admitted (m, c, A, B) the status and the solution agree with
    the enumeration; c is taken outside [0, m) as well."""
    n = bad = 0
    count = {OK: 0, NO_SOLUTION: 0, NOT_UNIQUE: 0}
    for m in range(1, 37):
        for c in range(-m, 2 * m):
            if not (0 <= c < m) and (c % 5):
                continue
            for A in list(range(0, min(m, 9))) + [m - 1, m, m + 1, 2 * m]:
                for B in (1, 2, 3, 4, 6, 9, m, m + 3):
                    ref = sol_brute(m, c, A, B)
                    st, sols, _ = recon_partial(m, c, A, B, limit=10 ** 9)
                    n += 1
                    count[classify(ref)] += 1
                    if st != classify(ref):
                        bad += 1
                    elif st == OK and set(sols) != ref:
                        bad += 1
                    elif st == NOT_UNIQUE and not (len(set(sols)) == 2 and set(sols) <= ref):
                        bad += 1
    report("check_s3_complete", bad == 0,
           f"{n} problems (m <= 36): one {count[OK]}, none {count[NO_SOLUTION]}, several {count[NOT_UNIQUE]}; "
           f"{bad} failures")


def check_s3_limit():
    """Algorithm R with a search limit: NOT_DETERMINED is returned only if m <= 2 A B, A < m, |T| <= B and
    floor(B/|T|) > limit; every other status is the true one; limit 0 never searches."""
    n = bad = nd = 0
    for m in range(1, 30):
        for c in range(m):
            for A in range(0, m):
                for B in (1, 2, 3, 5, 9, 17, 40):
                    ref = sol_brute(m, c, A, B)
                    cert = eea_pair(m, c, A)
                    for limit in (0, 1, 2, 5):
                        st, sols, _ = recon_partial(m, c, A, B, limit)
                        n += 1
                        if st == NOT_DETERMINED:
                            nd += 1
                            allowed = (m <= 2 * A * B and abs(cert[3]) <= B and B // abs(cert[3]) > limit)
                            if not allowed or not set(sols) <= ref or len(sols) > 1:
                                bad += 1
                        else:
                            if st != classify(ref):
                                bad += 1
                            if st == OK and set(sols) != ref:
                                bad += 1
                        if 2 * A * B < m and st == NOT_DETERMINED:
                            bad += 1
    report("check_s3_limit", bad == 0,
           f"{n} calls with limits 0, 1, 2, 5; NOT_DETERMINED {nd} times; {bad} failures")


def check_s3_ranges():
    """Proposition 1.8: in which ranges the answer can be none, one, several. Ranges: I: 2AB < m;
    II: AB < m <= 2AB; III: m <= AB and A < m; IV: A >= m."""
    table = {k: {OK: 0, NO_SOLUTION: 0, NOT_UNIQUE: 0} for k in ("I", "II", "III", "IV")}
    bad = n = 0
    for m in range(1, 41):
        for c in range(m):
            for A in range(0, m + 3):
                for B in range(1, 12):
                    ref = sol_brute(m, c, A, B)
                    n += 1
                    if A >= m:
                        rg = "IV"
                    elif 2 * A * B < m:
                        rg = "I"
                    elif A * B < m:
                        rg = "II"
                    else:
                        rg = "III"
                    table[rg][classify(ref)] += 1
                    # (i) range I: at most one; (ii) range IV: at least two; (iii) the obstruction by gcd(c, m)
                    if rg == "I" and len(ref) > 1:
                        bad += 1
                    if rg == "IV" and len(ref) < 2:
                        bad += 1
                    g = gcd(c, m)
                    if c % m != 0 and A < g and ref:
                        bad += 1
                    # (iv) Thue with coprimality: every prime factor of m above B, A < m < (A+1)(B+1)
                    if A < m < (A + 1) * (B + 1) and all(p > B for p in prime_factors(m)) and not ref:
                        bad += 1
    wit = [((4, 2, 1, 1), "I"), ((4, 2, 1, 2), "II"), ((4, 2, 1, 4), "III"), ((8, 3, 2, 2), "II"),
           ((2, 1, 1, 1), "II")]
    wbad = 0
    for (m, c, A, B), _ in wit[:4]:
        if sol_brute(m, c, A, B):
            wbad += 1
    if sol_brute(2, 1, 1, 1) != {(1, 1), (-1, 1)}:
        wbad += 1
    ok = bad == 0 and wbad == 0 and table["I"][NOT_UNIQUE] == 0 and table["IV"][OK] == 0 \
        and table["IV"][NO_SOLUTION] == 0 and all(table[k][NO_SOLUTION] > 0 for k in ("I", "II", "III")) \
        and all(table[k][NOT_UNIQUE] > 0 for k in ("II", "III", "IV")) \
        and all(table[k][OK] > 0 for k in ("I", "II", "III"))
    txt = "; ".join(f"{k}: one {v[OK]}, none {v[NO_SOLUTION]}, several {v[NOT_UNIQUE]}" for k, v in table.items())
    report("check_s3_ranges", ok, f"{n} problems (m <= 40). {txt}; {bad + wbad} failures")


def check_s3_edge():
    """The adversarial cases of the brief: m = 1, m = 2, A = 0, c outside [0, m), the example 1/5 = 5 mod 6,
    large random fractions, invalid input."""
    bad = n = 0
    exp = [
        ((1, 0, 0, 1), OK, {(0, 1)}), ((1, 7, 0, 5), OK, {(0, 1)}), ((1, 0, 1, 1), NOT_UNIQUE, None),
        ((2, 1, 1, 1), NOT_UNIQUE, {(1, 1), (-1, 1)}), ((2, 1, 0, 9), NO_SOLUTION, set()),
        ((2, 0, 0, 9), OK, {(0, 1)}), ((2, 1, 1, 0), NO_SOLUTION, set()), ((0, 1, 1, 1), DOMAIN, None),
        ((5, 1, -1, 1), NO_SOLUTION, set()), ((-3, 1, 1, 1), DOMAIN, None), ((6, 5, 1, 5), NOT_UNIQUE, {(-1, 1), (1, 5)}),
        ((6, 5, 1, 4), OK, {(-1, 1)}), ((6, -1, 1, 4), OK, {(-1, 1)}), ((6, 11, 1, 4), OK, {(-1, 1)}),
        ((7, 0, 0, 3), OK, {(0, 1)}), ((7, 3, 0, 3), NO_SOLUTION, set()), ((12, 6, 5, 50), NO_SOLUTION, set()),
        ((101, 34, 1, 3), OK, {(1, 3)}),
    ]
    for (m, c, A, B), st, sols in exp:
        got, s, _ = recon_partial(m, c, A, B, 10 ** 6)
        n += 1
        if got != st:
            bad += 1
        elif sols is not None and st in (OK, NO_SOLUTION) and set(s) != sols:
            bad += 1
        elif sols is not None and st == NOT_UNIQUE and sol_brute(m, c, A, B) != sols:
            bad += 1
    # 1/5 is 5 mod 6 and is not in the adelic ball 5 + 6 Zhat (SPEC 9.2): membership by valuations
    q = F(1, 5)
    in_ball = all(vp(q - 5, p) >= vp(F(6), p) for p in (2, 3, 5))
    if in_ball or (1, 5) not in sol_brute(6, 5, 1, 5):
        bad += 1
    n += 1
    # large random: a fraction n/d within the bounds, modulus above 2 A B, is returned
    big = 0
    for _ in range(3000):
        bits = random.choice((20, 64, 300))
        A = random.getrandbits(bits) + 1
        B = random.getrandbits(bits) + 1
        m = 2 * A * B + 1 + random.getrandbits(bits)
        d = random.randrange(1, B + 1)
        nn = random.randrange(-A, A + 1)
        g = gcd(nn, d)
        nn, d = nn // g, d // g
        big += 1
        if gcd(d, m) != 1:
            # the fraction is not a solution for any residue; the residue of another one is used instead
            continue
        c = nn * pow(d, -1, m) % m + m * random.randrange(-2, 3)
        st, s, _ = recon_partial(m, c, A, B, 0)
        if st != OK or s != [(nn, d)]:
            bad += 1
    report("check_s3_edge", bad == 0, f"{n} fixed cases, {big} large random fractions, {bad} failures")


def check_s3_forget():
    """Proposition 1.10(4): for a finite ball (A + H Zhat)/d with H > 0 and gcd(d, H) = 1 the rationals of the
    ball lie in P(H, c), c = A / d modulo H; the inclusion is proper for H >= 2."""
    n = bad = proper = 0
    for H in range(1, 13):
        for d in range(1, 8):
            if gcd(d, H) != 1:
                continue
            for A in range(H):
                c = A * pow(d, -1, H) % H if H > 1 else 0
                ps = prime_factors(H)
                for k in range(-6, 7):
                    q = F(A + H * k, d)               # precision.md Lemma 1: the rationals of the ball
                    n += 1
                    if not all(q == c or vp(q - c, p) >= vp(H, p) for p in ps):
                        bad += 1
                    if (q.numerator - c * q.denominator) % H or gcd(q.denominator, H) != 1:
                        bad += 1
                # a rational of P(H, c) outside the ball: denominator e prime to H d, e > 1
                e = next(e for e in range(2, 50) if gcd(e, H * d) == 1)
                q = F(c * e + H, e) if H > 1 else F(1, e)
                in_P = (q.numerator - c * q.denominator) % H == 0 and gcd(q.denominator, H) == 1
                in_ball = (q * d - A) / H == int((q * d - A) / H)
                if in_P and not in_ball:
                    proper += 1
                else:
                    bad += 1
    report("check_s3_forget", bad == 0, f"{n} rationals of balls (H <= 12, d <= 7), {proper} witnesses of a proper "
           f"inclusion, {bad} failures")


def flint_reconstruct(a, m, N, D):
    """fmpq_reconstruct_fmpz_2 of the installed FLINT for small arguments. Returns (ret, n, d)."""
    res = (ctypes.c_long * 2)(0, 1)
    ca, cm, cN, cD = (ctypes.c_long(v) for v in (a, m, N, D))
    FLINT.fmpq_reconstruct_fmpz_2.restype = ctypes.c_int
    ret = FLINT.fmpq_reconstruct_fmpz_2(res, ctypes.byref(ca), ctypes.byref(cm), ctypes.byref(cN),
                                        ctypes.byref(cD))
    return ret, res[0], res[1]


def probe_s3_flint():
    """Proposition 1.9: for m > 2, 0 <= a < m, N >= 1, D >= 1 (with or without 2 N D < m) FLINT returns 1 exactly
    when the row of the Euclidean algorithm is a solution, and then returns that row. Counted: the calls
    outside 2 N D < m in which FLINT returns 0 although solutions exist, and those in which it returns one of
    several solutions."""
    if FLINT is None:
        skip("probe_s3_flint", "libflint not found")
        return
    n = bad = zero_but_sol = one_of_several = inside = written = 0
    for m in range(3, 61):
        for a in range(m):
            for N in (1, 2, 3, 4, 7, 11, m - 1):
                for D in (1, 2, 3, 5, 9, m):
                    if not 1 <= N < m:
                        continue
                    Rp, Tp, R, T = eea_pair(m, a, N)
                    row_ok = abs(T) <= D and gcd(R, T) == 1
                    sg = 1 if T > 0 else -1
                    ret, fn, fd = flint_reconstruct(a, m, N, D)
                    n += 1
                    ref = sol_brute(m, a, N, D)
                    if ret == 0 and (fn, fd) != (0, 1):
                        written += 1
                    if ret != (1 if row_ok else 0):
                        bad += 1
                    elif ret == 1 and (fn, fd) != (sg * R, abs(T)):
                        bad += 1
                    elif ret == 1 and (fn, fd) not in ref:
                        bad += 1
                    if 2 * N * D < m:
                        inside += 1
                        if (ret == 1) != (len(ref) == 1) or len(ref) > 1:
                            bad += 1
                    else:
                        if ret == 0 and ref:
                            zero_but_sol += 1
                        if ret == 1 and len(ref) > 1:
                            one_of_several += 1
    report("probe_s3_flint", bad == 0 and zero_but_sol > 0 and one_of_several > 0 and written > 0,
           f"FLINT {flint_version()}: {n} calls, {inside} inside 2ND < m; outside: returns 0 although a solution "
           f"exists {zero_but_sol} times, returns one of several {one_of_several} times; outputs changed at "
           f"return 0: {written} times; {bad} failures")


PART1 = [check_s3_coprime, check_s3_certificate, check_s3_param, check_s3_complete, check_s3_limit,
         check_s3_ranges, check_s3_edge, check_s3_forget, probe_s3_flint]

# =========================================================================================================
# Part 2. S.1: linear systems modulo N
# =========================================================================================================
# A matrix is a list of rows; a row is a list of integers in [0, N). A x = b with x a column: A has r rows
# and c columns. Row spans are spans of row vectors over Z/N.

def pivot_col(row):
    for j, x in enumerate(row):
        if x:
            return j
    return None


def howell(rows, ncols, N):
    """Algorithm H of solvers.md 2.5: the Howell form of the span of the rows over Z/N, as the list of its
    non-zero rows in the order of their pivot columns."""
    T = {}
    stack = [([x % N for x in v], 0) for v in reversed(rows)]
    while stack:
        v, j = stack.pop()
        while j < ncols:
            a = v[j]
            if a == 0:
                j += 1
                continue
            if j not in T:
                g, s, _ = xgcd(a, N)                       # s a + t N = g = gcd(a, N), 1 <= g < N
                T[j] = [(s * x) % N for x in v]            # pivot g
                stack.append(([((N // g) * x) % N for x in v], j + 1))
                break
            w = T[j]
            h = w[j]
            if a % h == 0:
                q = a // h
                v = [(x - q * y) % N for x, y in zip(v, w)]
                j += 1
                continue
            g, s, t = xgcd(a, h)                           # s a + t h = g
            w2 = [(s * x + t * y) % N for x, y in zip(v, w)]
            v = [((h // g) * x - (a // g) * y) % N for x, y in zip(v, w)]
            T[j] = w2
            stack.append(([((N // g) * x) % N for x in w2], j + 1))
            j += 1
    cols = sorted(T)
    H = [T[j] for j in cols]
    for i, j in enumerate(cols):                           # reduce the entries above each pivot
        h = H[i][j]
        for k in range(i):
            q = H[k][j] // h
            if q:
                H[k] = [(x - q * y) % N for x, y in zip(H[k], H[i])]
    return H


def span_brute(rows, ncols, N):
    S = {tuple([0] * ncols)}
    for r in rows:
        r = [x % N for x in r]
        new = set()
        for k in range(N):
            kr = [(k * x) % N for x in r]
            for v in S:
                new.add(tuple((x + y) % N for x, y in zip(v, kr)))
        S = new
    return S


def is_echelon(H, ncols, N):
    """(r1) and the first half of (r2): non-zero rows, pivot columns strictly increasing, every pivot a divisor
    of N with 1 <= pivot < N, entries in [0, N)."""
    last = -1
    for row in H:
        if len(row) != ncols or any(not (0 <= x < N) for x in row):
            return False
        j = pivot_col(row)
        if j is None or j <= last or N % row[j] != 0:
            return False
        last = j
    return True


def reduce_by(v, H, N, start=0):
    """Greedy reduction of the row v by the echelon rows H[start:]; returns (remainder, coefficients)."""
    v = list(v)
    coef = [0] * len(H)
    for i in range(start, len(H)):
        j = pivot_col(H[i])
        h = H[i][j]
        if v[j] % h == 0:
            q = v[j] // h
            coef[i] = q
            v = [(x - q * y) % N for x, y in zip(v, H[i])]
    return v, coef


def is_howell(H, ncols, N):
    """What a checker verifies for the canonical form (Proposition 2.4): echelon, entries above a pivot reduced,
    and for every row i: (N / pivot) row_i reduces to 0 by the rows after it."""
    if not is_echelon(H, ncols, N):
        return False
    for i, row in enumerate(H):
        j = pivot_col(row)
        if any(H[k][j] >= row[j] for k in range(i)):
            return False
        ann = [((N // row[j]) * x) % N for x in row]
        rem, _ = reduce_by(ann, H, N, i + 1)
        if any(rem):
            return False
    return True


def howell_brute(S, ncols, N):
    """The canonical form from the definition, by search in the set S itself. Returns (rows, unique)."""
    piv = {}
    for j in range(ncols):
        vals = {v[j] for v in S if not any(v[:j])} - {0}
        if vals:
            piv[j] = min(vals)
    rows, unique = [], True
    for j in sorted(piv):
        cands = [v for v in S if not any(v[:j]) and v[j] == piv[j]
                 and all(v[k] < piv[k] for k in piv if k > j)]
        if len(cands) != 1:
            unique = False
        rows.append(list(cands[0]) if cands else None)
    return rows, unique


def rand_rows(nrows, ncols, N):
    return [[random.randrange(N) for _ in range(ncols)] for _ in range(nrows)]


MODULI = (1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18)


def check_s1_howell():
    """Proposition 2.3 (counting), 2.4 (uniqueness), 2.5 (Algorithm H): the output has the span of the input,
    is in echelon form with reduced entries, has the Howell property by definition (rows after row i generate
    the vectors of the span whose entries up to the pivot column of row i are zero), its span has
    prod(N / pivot) elements, and it is the only matrix with these properties found by search in the span."""
    n = bad = 0
    for N in MODULI:
        for ncols in (1, 2, 3):
            if N ** ncols > 1800:
                continue
            for _ in range(60 if N > 1 else 3):
                rows = rand_rows(random.randint(0, 4), ncols, N)
                if random.random() < 0.3:                   # rows with many zero divisors
                    rows = [[(x * random.choice([d for d in range(1, N + 1) if N % d == 0])) % N for x in r]
                            for r in rows]
                H = howell(rows, ncols, N)
                S = span_brute(rows, ncols, N)
                n += 1
                ok = span_brute(H, ncols, N) == S and is_howell(H, ncols, N)
                size = 1
                for i, row in enumerate(H):
                    j = pivot_col(row)
                    size *= N // row[j]
                    Sj = {v for v in S if not any(v[:j + 1])}
                    if span_brute(H[i + 1:], ncols, N) != Sj:
                        ok = False
                if size != len(S):
                    ok = False
                B, unique = howell_brute(S, ncols, N)
                if not unique or B != H:
                    ok = False
                if not ok:
                    bad += 1
    report("check_s1_howell", bad == 0, f"{n} row sets over Z/N, N in {MODULI}, up to 3 columns; {bad} failures")


def check_s1_canonical():
    """Proposition 2.4: equal modules give identical output: the rows are replaced by random combinations that
    generate the same span, permuted, and padded with redundant rows."""
    n = bad = 0
    for N in (2, 4, 6, 8, 9, 12, 16, 30, 36, 64, 360, 2 ** 20, 3 ** 9 * 2 ** 5, 10 ** 12 + 39 * 10 ** 6):
        for ncols in (1, 2, 4, 6):
            for _ in range(12):
                rows = rand_rows(random.randint(1, 5), ncols, N)
                H = howell(rows, ncols, N)
                alt = [list(r) for r in rows]
                for _ in range(8):                          # elementary operations keep the span
                    i, k = random.randrange(len(alt)), random.randrange(len(alt))
                    if i != k:
                        q = random.randrange(N)
                        alt[i] = [(x + q * y) % N for x, y in zip(alt[i], alt[k])]
                    u = random.choice([u for u in range(1, min(N, 200)) if gcd(u, N) == 1])
                    alt[k] = [(u * x) % N for x in alt[k]]
                co = [random.randrange(N) for _ in alt]     # one redundant row: a combination of the others
                alt.append([sum(q * r[j] for q, r in zip(co, alt)) % N for j in range(ncols)])
                alt.append([0] * ncols)
                random.shuffle(alt)
                n += 1
                if howell(alt, ncols, N) != H or howell(H, ncols, N) != H or not is_howell(H, ncols, N):
                    bad += 1
    # two different modules must give different output (the form is a function of the module only, and
    # injective): all submodules of (Z/N)^2 generated by two rows, for small N, by their sets
    seen = {}
    inj = 0
    for N in (4, 6, 8, 9, 12):
        for a in product(range(N), repeat=4):
            rows = [list(a[:2]), list(a[2:])]
            key = (N, tuple(map(tuple, howell(rows, 2, N))))
            S = frozenset(span_brute(rows, 2, N))
            if seen.setdefault(key, S) != S:
                bad += 1
            inj += 1
    mods = len({(k[0], v) for k, v in seen.items()})
    report("check_s1_canonical", bad == 0 and mods == len(seen),
           f"{n} modules with changed generators; {inj} pairs of rows over (Z/N)^2, N in (4, 6, 8, 9, 12): "
           f"{mods} modules, {len(seen)} forms; {bad} failures")


def matvec(A, x, N):
    return [sum(a * b for a, b in zip(row, x)) % N for row in A]


def transpose(A, r, c):
    return [[A[i][j] for i in range(r)] for j in range(c)]


def kernel_cert(A, r, c, N):
    """Proposition 2.6: the Howell form of [A^T | I_c]. Returns (E, V, G): the rows with pivot among the first
    r columns, split as (E_i | V_i), and the right blocks G_i of the other rows."""
    M = [[A[i][j] % N for i in range(r)] + [(1 if k == j else 0) % N for k in range(c)] for j in range(c)]
    H = howell(M, r + c, N)
    E, V, G = [], [], []
    for row in H:
        if pivot_col(row) < r:
            E.append(row[:r])
            V.append(row[r:])
        else:
            G.append(row[r:])
    return E, V, G


def linsolve_mod(A, b, r, c, N):
    """Algorithm L of solvers.md 2.8. Returns a dictionary: status OK with x0, G, E, V; or status NO_SOLUTION
    with y (and G, E, V as well); DOMAIN for N < 1."""
    if N < 1:
        return {"status": DOMAIN}
    A = [[x % N for x in row] for row in A]
    b = [x % N for x in b]
    E, V, G = kernel_cert(A, r, c, N)
    rem, coef = reduce_by(b, E, N)
    if not any(rem):
        x0 = [sum(q * V[i][k] for i, q in enumerate(coef)) % N for k in range(c)]
        return {"status": OK, "x0": x0, "G": G, "E": E, "V": V}
    _, _, Y = kernel_cert(transpose(A, r, c), c, r, N)     # generators of the left kernel of A
    for y in Y:
        if sum(p * q for p, q in zip(y, b)) % N:
            return {"status": NO_SOLUTION, "y": y, "G": G, "E": E, "V": V}
    raise AssertionError("Proposition 2.7 violated: no dual certificate")


def linsol_check(sol, A, b, r, c, N, canonical=True):
    """The checker of Proposition 2.6 and 2.7: what is verified, with no elimination."""
    if N < 1 or sol.get("status") not in (OK, NO_SOLUTION):
        return False
    A = [[x % N for x in row] for row in A]
    b = [x % N for x in b]
    E, V, G = sol["E"], sol["V"], sol["G"]
    if len(E) != len(V) or any(len(v) != c or any(not (0 <= x < N) for x in v) for v in V):
        return False
    if not is_echelon(E, r, N) or not is_echelon(G, c, N):                       # (K1)
        return False
    for e, v in zip(E, V):                                                        # (K2)
        if matvec(A, v, N) != e:
            return False
    for g in G:                                                                   # (K3)
        if any(matvec(A, g, N)):
            return False
    prod = 1                                                                      # (K4)
    for row in E + G:
        prod *= N // row[pivot_col(row)]
    if prod != N ** c:
        return False
    if canonical and not is_howell(G, c, N):                                      # (K5)
        return False
    if sol["status"] == OK:                                                       # (K6)
        x0 = sol["x0"]
        return len(x0) == c and all(0 <= x < N for x in x0) and matvec(A, x0, N) == b
    y = sol["y"]                                                                  # (K7)
    if len(y) != r or any(not (0 <= x < N) for x in y):
        return False
    yA = [sum(y[i] * A[i][j] for i in range(r)) % N for j in range(c)]
    return not any(yA) and sum(p * q for p, q in zip(y, b)) % N != 0


def solutions_brute(A, b, r, c, N):
    b = [x % N for x in b]
    return {x for x in product(range(N), repeat=c) if matvec(A, x, N) == b}


def check_s1_solve():
    """Propositions 2.6, 2.7 and Algorithm L against the enumeration of (Z/N)^c: the status; x0 + span(G) is the
    set of all solutions; G is the canonical form of the kernel; the certificate is accepted; in the case
    "none" the vector y exists and is accepted. Exhaustive for N = 4, r, c <= 2; random otherwise."""
    n = bad = none = 0

    def one(A, b, r, c, N):
        nonlocal n, bad, none
        sol = linsolve_mod(A, b, r, c, N)
        ref = solutions_brute(A, b, r, c, N)
        ker = solutions_brute(A, [0] * r, r, c, N)
        n += 1
        ok = linsol_check(sol, A, b, r, c, N)
        ok = ok and span_brute(sol["G"], c, N) == ker
        ok = ok and sol["G"] == howell([list(k) for k in ker], c, N)
        if sol["status"] == OK:
            x0 = sol["x0"]
            ok = ok and {tuple((x + k) % N for x, k in zip(x0, kk)) for kk in ker} == ref and bool(ref)
        else:
            none += 1
            ok = ok and not ref
        if not ok:
            bad += 1

    for r in (1, 2):
        for c in (1, 2):
            for a in product(range(4), repeat=r * c):
                A = [list(a[i * c:(i + 1) * c]) for i in range(r)]
                for b in product(range(4), repeat=r):
                    one(A, list(b), r, c, 4)
    exhaustive = n
    for N in MODULI:
        for r in (0, 1, 2, 3):
            for c in (0, 1, 2, 3):
                if N ** c > 1800:
                    continue
                for _ in range(25 if N > 1 else 2):
                    A = rand_rows(r, c, N)
                    if random.random() < 0.4:
                        d = random.choice([d for d in range(1, N + 1) if N % d == 0])
                        A = [[(x * d) % N for x in row] for row in A]
                    b = [random.randrange(N) for _ in range(r)]
                    if random.random() < 0.5 and c:
                        b = matvec(A, [random.randrange(N) for _ in range(c)], N)
                    b = [x + N * random.randrange(-2, 3) for x in b]          # right-hand side outside [0, N)
                    one(A, b, r, c, N)
    report("check_s1_solve", bad == 0 and none > 0,
           f"{exhaustive} systems modulo 4 (all A, b with r, c <= 2) and {n - exhaustive} random systems, N in "
           f"{MODULI}, r, c <= 3; without solution: {none}; {bad} failures")


def check_s1_cert_sound():
    """Soundness of the checker (Propositions 2.6, 2.7): a changed certificate is refused, or what it claims is
    still true. The claims are tested by enumeration: x0 solves; span(G) is the whole kernel; G is the
    canonical form; no solution exists."""
    n = accepted = bad = 0
    for N in (2, 4, 6, 8, 9, 12):
        for _ in range(140):
            r, c = random.randint(1, 3), random.randint(1, 3)
            if N ** c > 1800:
                continue
            A = rand_rows(r, c, N)
            if random.random() < 0.5:
                d = random.choice([d for d in range(1, N + 1) if N % d == 0])
                A = [[(x * d) % N for x in row] for row in A]
            b = [random.randrange(N) for _ in range(r)]
            sol = linsolve_mod(A, b, r, c, N)
            ker = solutions_brute(A, [0] * r, r, c, N)
            ref = solutions_brute(A, b, r, c, N)
            kerH = howell([list(k) for k in ker], c, N)
            for _ in range(12):
                mut = {k: ([list(x) for x in v] if k in ("G", "E", "V") else (list(v) if k in ("x0", "y") else v))
                       for k, v in sol.items()}
                kind = random.randrange(7)
                if kind == 0 and mut["G"]:
                    del mut["G"][random.randrange(len(mut["G"]))]
                elif kind == 1 and mut["G"]:
                    g = random.choice(mut["G"])
                    g[random.randrange(c)] = random.randrange(N)
                elif kind == 2 and mut["E"]:
                    i = random.randrange(len(mut["E"]))
                    del mut["E"][i]
                    del mut["V"][i]
                elif kind == 3 and mut["V"]:
                    v = random.choice(mut["V"])
                    v[random.randrange(c)] = random.randrange(N)
                elif kind == 4:
                    key = "x0" if mut["status"] == OK else "y"
                    mut[key][random.randrange(len(mut[key]))] = random.randrange(N)
                elif kind == 5:
                    if mut["status"] == OK:
                        mut["status"], mut["y"] = NO_SOLUTION, [random.randrange(N) for _ in range(r)]
                    else:
                        mut["status"], mut["x0"] = OK, [random.randrange(N) for _ in range(c)]
                elif kind == 6 and mut["G"]:
                    g = random.choice(mut["G"])
                    u = random.choice([u for u in range(1, N) if gcd(u, N) == 1])
                    g[:] = [(u * x) % N for x in g]
                else:
                    continue
                n += 1
                if linsol_check(mut, A, b, r, c, N):
                    accepted += 1
                    true = span_brute(mut["G"], c, N) == ker and mut["G"] == kerH
                    if mut["status"] == OK:
                        true = true and tuple(mut["x0"]) in ref
                    else:
                        true = true and not ref
                    if not true:
                        bad += 1
    report("check_s1_cert_sound", bad == 0 and accepted < n,
           f"{n} changed certificates: {n - accepted} refused, {accepted} accepted and true, "
           f"{bad} accepted and false")


def check_s1_duality():
    """Proposition 2.7: for a submodule M of (Z/N)^r and b outside M there is y with y.M = 0 and y.b != 0; and
    the number of elements of M times that of its annihilator is N^r. By enumeration."""
    n = bad = 0
    for N in (2, 3, 4, 6, 8, 9, 12):
        for r in (1, 2, 3):
            if N ** r > 1800:
                continue
            allv = list(product(range(N), repeat=r))
            for _ in range(20):
                gens = rand_rows(random.randint(0, 3), r, N)
                if random.random() < 0.5:
                    d = random.choice([d for d in range(1, N + 1) if N % d == 0])
                    gens = [[(x * d) % N for x in row] for row in gens]
                M = span_brute(gens, r, N)
                perp = [y for y in allv if all(sum(p * q for p, q in zip(y, g)) % N == 0 for g in gens)]
                n += 1
                if len(M) * len(perp) != N ** r:
                    bad += 1
                for b in allv:
                    sep = any(sum(p * q for p, q in zip(y, b)) % N for y in perp)
                    if sep != (b not in M):
                        bad += 1
    report("check_s1_duality", bad == 0, f"{n} submodules of (Z/N)^r, every b of (Z/N)^r; {bad} failures")


def check_s1_edge():
    """The cases of the brief: the zero matrix, N = 1, r = 0, c = 0, N with square factors, one equation
    (Shoup, Theorem 2.5), large N."""
    n = bad = 0

    def expect(A, b, r, c, N, status, G=None, x0=None):
        nonlocal n, bad
        sol = linsolve_mod(A, b, r, c, N)
        n += 1
        ok = sol["status"] == status and linsol_check(sol, A, b, r, c, N)
        if G is not None:
            ok = ok and sol["G"] == G
        if x0 is not None:
            ok = ok and sol["x0"] == x0
        if not ok:
            bad += 1

    expect([[0, 0], [0, 0]], [0, 0], 2, 2, 12, OK, G=[[1, 0], [0, 1]], x0=[0, 0])
    expect([[0, 0], [0, 0]], [0, 5], 2, 2, 12, NO_SOLUTION, G=[[1, 0], [0, 1]])
    expect([[3, 7], [1, 1]], [5, 6], 2, 2, 1, OK, G=[], x0=[0, 0])
    expect([], [], 0, 3, 8, OK, G=[[1, 0, 0], [0, 1, 0], [0, 0, 1]], x0=[0, 0, 0])
    expect([[], []], [0, 0], 2, 0, 8, OK, G=[], x0=[])
    expect([[], []], [0, 4], 2, 0, 8, NO_SOLUTION, G=[])
    expect([], [], 0, 0, 5, OK, G=[], x0=[])
    expect([[2]], [2], 1, 1, 4, OK, G=[[2]])
    expect([[2]], [1], 1, 1, 4, NO_SOLUTION, G=[[2]])
    expect([[6]], [3], 1, 1, 9, OK, G=[[3]])
    expect([[2, 4]], [6], 1, 2, 8, OK, G=[[2, 1], [0, 2]])       # x + 2 y = 0 modulo 4
    if linsolve_mod([[1]], [1], 1, 1, 0)["status"] != DOMAIN:
        bad += 1
    n += 1
    # the example of 2.11: the Chinese remainder image of the forms modulo 2 and 3 is not the form modulo 6
    if howell([[3, 4]], 2, 6) != [[3, 0], [0, 2]] or howell([[3, 0], [0, 2]], 2, 6) != [[3, 0], [0, 2]] \
            or howell([[3, 0], [0, 2]], 2, 2) != [[1, 0]] or howell([[3, 0], [0, 2]], 2, 3) != [[0, 1]] \
            or howell([[2, 1]], 2, 4) != [[2, 1], [0, 2]]:
        bad += 1
    n += 1
    # one equation a z = b modulo N: solvable exactly when gcd(a, N) divides b; the kernel is (N/gcd) Z/N
    one = 0
    for N in range(1, 40):
        for a in range(N):
            for b in range(N):
                sol = linsolve_mod([[a]], [b], 1, 1, N)
                g = gcd(a, N)
                one += 1
                want = OK if b % g == 0 else NO_SOLUTION
                Gw = [[N // g]] if N // g < N else []
                if sol["status"] != want or sol["G"] != Gw or not linsol_check(sol, [[a]], [b], 1, 1, N):
                    bad += 1
    big = 0
    for N in (2 ** 64, 2 ** 89 - 1, 3 ** 40 * 2 ** 30, (2 ** 61 - 1) ** 2, 10 ** 30):
        for _ in range(12):
            r, c = random.randint(1, 5), random.randint(1, 5)
            A = rand_rows(r, c, N)
            if random.random() < 0.5:
                A = [[(x * random.choice((2, 3, 2 ** 20, 3 ** 10))) % N for x in row] for row in A]
            x = [random.randrange(N) for _ in range(c)]
            b = matvec(A, x, N)
            changed = random.random() < 0.3
            if changed:
                b[0] = (b[0] + 1) % N
            sol = linsolve_mod(A, b, r, c, N)
            big += 1
            if not linsol_check(sol, A, b, r, c, N):
                bad += 1
            if sol["status"] == OK and not changed:
                # x - x0 must be in the span of G: greedy reduction by the canonical form (Lemma 2.2)
                diff = [(p - q) % N for p, q in zip(x, sol["x0"])]
                if any(reduce_by(diff, sol["G"], N)[0]):
                    bad += 1
            if sol["status"] == NO_SOLUTION and not changed:
                bad += 1
    report("check_s1_edge", bad == 0, f"{n} fixed cases, {one} single equations (N < 40), {big} systems, N up to "
           f"122 bits; {bad} failures")


def fball_canon(A, H, d):
    """Canonical triple of (A + H Zhat)/d for H > 0 (conventions 5.2)."""
    if d < 0:
        A, d = -A, -d
    A %= H
    g = gcd(gcd(A, H), d)
    return A // g, H // g, d // g


def adelic_system(A, balls, r, c):
    """Proposition 2.9: the system modulo N = lcm(H_i) that is equivalent to A x in the balls, x in Zhat^c.
    balls: list of (A_i, H_i, d_i) with H_i >= 1, d_i >= 1. Returns (A', b', N)."""
    N = 1
    for _, H, _ in balls:
        N = N * H // gcd(N, H)
    A2 = [[(N // balls[i][1]) * balls[i][2] * A[i][j] % N for j in range(c)] for i in range(r)]
    b2 = [(N // balls[i][1]) * balls[i][0] % N for i in range(r)]
    return A2, b2, N


def check_s1_adelic():
    """Proposition 2.9: membership of A x in the balls, tested by exact rational arithmetic on integer
    representatives x (and on x + N z), against the solutions of the reduced system; the enclosing balls of
    the solution set (Proposition 2.10) against the enumeration."""
    n = bad = 0
    for _ in range(400):
        r, c = random.randint(1, 3), random.randint(1, 2)
        balls = []
        for _ in range(r):
            H = random.choice((1, 2, 3, 4, 6, 8, 9, 12))
            d = random.choice((1, 1, 2, 3, 4, 6))
            balls.append(fball_canon(random.randrange(-20, 20), H, d))
        A = [[random.randrange(-6, 7) for _ in range(c)] for _ in range(r)]
        A2, b2, N = adelic_system(A, balls, r, c)
        if N ** c > 3000:
            continue
        sol = linsolve_mod(A2, b2, r, c, N)
        n += 1

        def member(x):
            for i in range(r):
                Ai, Hi, di = balls[i]
                val = F(sum(A[i][j] * x[j] for j in range(c)))
                if ((val * di - Ai) / Hi).denominator != 1:        # precision.md Lemma 1 on the integer point
                    return False
            return True

        ref = {x for x in product(range(N), repeat=c) if member(x)}
        for x in list(ref)[:5]:
            z = [x[j] + N * random.randrange(-3, 4) for j in range(c)]
            if not member(z):
                bad += 1
        if sol["status"] == OK:
            ker = span_brute(sol["G"], c, N)
            got = {tuple((p + q) % N for p, q in zip(sol["x0"], k)) for k in ker}
            if got != ref or not ref:
                bad += 1
            for j in range(c):                                   # enclosure of coordinate j
                rad = N
                for g in sol["G"]:
                    rad = gcd(rad, g[j])
                if {x[j] % rad for x in ref} != {sol["x0"][j] % rad}:
                    bad += 1
                if len({x[j] for x in ref}) != N // rad:          # the enclosure is the smallest ball
                    bad += 1
        elif ref:
            bad += 1
    report("check_s1_adelic", bad == 0, f"{n} systems with right-hand sides (A + H Zhat)/d; {bad} failures")


class FmpzMat(ctypes.Structure):
    _fields_ = [("entries", ctypes.POINTER(ctypes.c_long)), ("r", ctypes.c_long), ("c", ctypes.c_long),
                ("rows", ctypes.POINTER(ctypes.POINTER(ctypes.c_long)))]


def flint_howell(rows, ncols, N):
    """fmpz_mat_howell_form_mod of the installed FLINT on the rows padded with zero rows to at least ncols
    rows. Small entries only. Returns (number of non-zero rows, all rows)."""
    nr = max(len(rows), ncols)
    mat = FmpzMat()
    FLINT.fmpz_mat_init(ctypes.byref(mat), ctypes.c_long(nr), ctypes.c_long(ncols))
    for i, row in enumerate(rows):
        for j, x in enumerate(row):
            mat.rows[i][j] = x % N
    cN = ctypes.c_long(N)
    FLINT.fmpz_mat_howell_form_mod.restype = ctypes.c_long
    k = FLINT.fmpz_mat_howell_form_mod(ctypes.byref(mat), ctypes.byref(cN))
    out = [[mat.rows[i][j] for j in range(ncols)] for i in range(nr)]
    FLINT.fmpz_mat_clear(ctypes.byref(mat))
    return k, out


def probe_s1_flint():
    """Proposition 2.11: fmpz_mat_howell_form_mod of FLINT 3.0.1, with the input padded to at least as many rows
    as columns, returns the form of Algorithm H (same pivots, same reduction), its non-zero rows first."""
    if FLINT is None:
        skip("probe_s1_flint", "libflint not found")
        return
    n = bad = 0
    for N in (2, 3, 4, 6, 8, 9, 12, 16, 36, 360, 2 ** 20, 10 ** 9 + 7, 3 ** 9 * 2 ** 5):
        for ncols in (1, 2, 3, 5):
            for _ in range(40):
                rows = rand_rows(random.randint(1, 6), ncols, N)
                if random.random() < 0.4:
                    rows = [[(x * random.choice((2, 3, 4, 6))) % N for x in row] for row in rows]
                H = howell(rows, ncols, N)
                k, out = flint_howell(rows, ncols, N)
                n += 1
                if k != len(H) or out[:k] != H or any(any(row) for row in out[k:]):
                    bad += 1
    report("probe_s1_flint", bad == 0, f"FLINT {flint_version()}: {n} matrices, N up to 30 bits; {bad} differences")


def probe_s1_hnf():
    """Proposition 2.12: the rows with pivot below N of the integer Hermite form of the rows and N e_j are the
    Howell form. The Hermite form is that of python-flint (its own FLINT, not the installed one)."""
    try:
        import flint
    except ImportError:
        skip("probe_s1_hnf", "python-flint not importable")
        return
    n = bad = full = 0
    for N in (1, 2, 4, 6, 8, 9, 12, 16, 36, 360, 1000, 2 ** 40 + 15 * 2 ** 20):
        for ncols in (1, 2, 3, 4):
            for _ in range(42):
                rows = rand_rows(random.randint(1, 4), ncols, N)
                if random.random() < 0.4:
                    rows = [[(x * random.choice((2, 3, 4, 6))) % N for x in row] for row in rows]
                H = howell(rows, ncols, N)
                M = flint.fmpz_mat(len(rows) + ncols, ncols, [x for r in rows for x in r] +
                                   [N if i == j else 0 for i in range(ncols) for j in range(ncols)])
                B = [[int(x) for x in r] for r in M.hnf().tolist()][:ncols]
                n += 1
                ok = all(N % B[j][j] == 0 for j in range(ncols))
                for j in range(ncols):
                    if B[j][j] == N:
                        full += 1
                        ok = ok and B[j] == [N if i == j else 0 for i in range(ncols)]
                if not ok or [r for j, r in enumerate(B) if B[j][j] < N] != H:
                    bad += 1
    report("probe_s1_hnf", bad == 0, f"python-flint {flint.__version__}: {n} matrices, {full} rows with pivot N; "
           f"{bad} differences")


PART2 = [check_s1_howell, check_s1_canonical, check_s1_solve, check_s1_cert_sound, check_s1_duality,
         check_s1_edge, check_s1_adelic, probe_s1_flint, probe_s1_hnf]
# =========================================================================================================
# Part 3. S.2: roots
# =========================================================================================================
# A polynomial is the list of its integer coefficients, f[i] the coefficient of X^i; [] is the zero polynomial.

def ptrim(f):
    f = list(f)
    while f and f[-1] == 0:
        f.pop()
    return f


def peval(f, a, mod=None):
    r = 0
    for c in reversed(f):
        r = r * a + c
        if mod is not None:
            r %= mod
    return r


def pderiv(f):
    return ptrim([i * c for i, c in enumerate(f)][1:])


def pmul(f, g):
    if not f or not g:
        return []
    out = [0] * (len(f) + len(g) - 1)
    for i, a in enumerate(f):
        for j, b in enumerate(g):
            out[i + j] += a * b
    return out


def pfrom_roots(roots, lead=1):
    f = [lead]
    for r in roots:
        r = F(r)
        f = pmul(f, [-r.numerator, r.denominator])
    return f


def pcompose_affine(f, a, q):
    """The coefficients of f(a + q Y)."""
    out = []
    for c in reversed(f):
        new = [0] * (len(out) + 1)                       # out * (a + q Y) + c
        for i, x in enumerate(out):
            new[i] += a * x
            new[i + 1] += q * x
        new[0] += c
        out = new
    return ptrim(out) if out else []


def val(n, p):
    """v_p of a non-zero integer."""
    v = 0
    while n % p == 0:
        n //= p
        v += 1
    return v


def content_val(g, p):
    return min(val(c, p) for c in g if c)


def strip_content(f, p):
    """f / p^w with w the largest exponent such that p^w divides every coefficient; the same roots."""
    f = ptrim(f)
    w = content_val(f, p)
    return [c // p ** w for c in f]


def root_cert_ok(f, p, a, k, s):
    """Definition 3.1: k > s >= 0, v_p(f'(a)) = s exactly, v_p(f(a)) >= k + s."""
    if not (k > s >= 0 and 0 <= a < p ** k):
        return False
    d = peval(pderiv(f), a, p ** (s + 1))
    return peval(f, a, p ** (k + s)) == 0 and d != 0 and d % (p ** s) == 0


def newton_step(f, p, a, k, s):
    """Proposition 3.3: from a certificate (a, k, s) to the certificate (a+, 2k - s, s)."""
    Fv = peval(f, a, p ** (2 * k)) // p ** (k + s)               # f(a) / p^(k+s) modulo p^(k-s)
    Dv = peval(pderiv(f), a, p ** k) // p ** s                   # f'(a) / p^s modulo p^(k-s), a unit
    q = p ** (k - s)
    step = (Fv % q) * pow(Dv % q, -1, q) % q
    k2 = 2 * k - s
    return (a - p ** k * step) % p ** k2, k2


def padic_roots(f, p, kreq, depth):
    """Algorithm P of solvers.md 3.5. Returns (status, certs, unresolved): certs is the list of (a, K, s), the
    balls a + p^K Z_p with exactly one root each; unresolved the list of (a, e), the classes a + p^e Z_p that
    the depth limit left undecided. status OK: unresolved is empty and the list is complete. The certificates
    refer to f with its content at p removed (strip_content)."""
    f = ptrim(f)
    if not f or kreq < 1 or depth < 0:
        return DOMAIN, [], []
    f = strip_content(f, p)
    certs, unresolved = [], []
    stack = [(0, 0, None)]
    while stack:
        a, e, wprev = stack.pop()
        g = pcompose_affine(f, a, p ** e)
        w = content_val(g, p)
        assert wprev is None or w >= wprev + 2 or not any(                 # Remark 3.6 (not used by the proofs)
            peval([c // p ** w for c in g], b, p) == 0 for b in range(p))
        g = [c // p ** w for c in g]
        dg = pderiv(g)
        for b in range(p):
            if peval(g, b, p) != 0:
                continue
            if peval(dg, b, p) != 0:                                        # a simple root of g modulo p
                s = w - e
                j = max(1, w - 2 * e + 1)
                beta, jj = b, 1
                while jj < j:                                               # Hensel on g, (H1): s = 0 there
                    beta, jj = newton_step(g, p, beta, jj, 0)
                beta %= p ** j
                k = e + j
                a1 = (a + p ** e * beta) % p ** k
                assert root_cert_ok(f, p, a1, k, s), (f, p, a1, k, s)
                K = max(kreq, s + 1)
                while k < K:
                    a1, k = newton_step(f, p, a1, k, s)
                a1 %= p ** K
                assert root_cert_ok(f, p, a1, K, s)
                certs.append((a1, K, s))
            elif e + 1 > depth:
                unresolved.append((a + p ** e * b, e + 1))
            else:
                stack.append((a + p ** e * b, e + 1, w))
    certs.sort()
    unresolved.sort()
    return (OK if not unresolved else NOT_DETERMINED), certs, unresolved


def approx_roots(f, p, K, inside=None):
    """Oracle: all x modulo p^K with f(x) = 0 modulo p^K, level by level (a root modulo p^(i+1) reduces to a
    root modulo p^i). inside = (a, e) restricts to the class a + p^e Z_p."""
    level = [0]
    for i in range(K):
        q = p ** (i + 1)
        nxt = []
        for x in level:
            for t in range(p):
                y = x + t * p ** i
                if inside is not None and i < inside[1] and (y - inside[0]) % p ** (i + 1):
                    continue
                if peval(f, y, q) == 0:
                    nxt.append(y)
        level = nxt
        if len(level) > 200000:
            raise OverflowError
    return level


PADIC_CASES = [
    # (name, f, p, expected number of roots in Z_p or None, all roots simple)
    ("x^2+1 at 2", [1, 0, 1], 2, 0, True),
    ("x^2+1 at 5", [1, 0, 1], 5, 2, True),
    ("x^2-9 at 2", [-9, 0, 1], 2, 2, True),
    ("x^2-17 at 2", [-17, 0, 1], 2, 2, True),
    ("x^2-3 at 2", [-3, 0, 1], 2, 0, True),
    ("x^2-7 at 3", [-7, 0, 1], 3, 2, True),
    ("x^2-2 at 7", [-2, 0, 1], 7, 2, True),
    ("Conrad 4.2 at 3", [1, 2, 2, -7, 1], 3, 2, True),
    ("x^3-10 at 3", [-10, 0, 0, 1], 3, 1, True),
    ("x^3-5 at 3", [-5, 0, 0, 1], 3, 0, True),
    ("Conrad 4.4 at 2", [-8, -2, -1, 1], 2, 3, True),
    ("(x^2-13)(x^2-17)(x^2-221) at 2", pmul(pmul([-13, 0, 1], [-17, 0, 1]), [-221, 0, 1]), 2, 2, True),
    ("(x^2-13)(x^2-17)(x^2-221) at 13", pmul(pmul([-13, 0, 1], [-17, 0, 1]), [-221, 0, 1]), 13, 2, True),
    ("(x^2-13)(x^2-17)(x^2-221) at 3", pmul(pmul([-13, 0, 1], [-17, 0, 1]), [-221, 0, 1]), 3, 2, True),
    ("(x^2-13)(x^2-17)(x^2-221) at 17", pmul(pmul([-13, 0, 1], [-17, 0, 1]), [-221, 0, 1]), 17, 2, True),
    ("x^2(x+3)-style: x^3+3x^2 at 3", [0, 0, 3, 1], 3, None, False),
    ("x^3+2x^2 at 2", [0, 0, 2, 1], 2, None, False),
    ("(x-1)^2(x+2) at 3", pmul(pmul([-1, 1], [-1, 1]), [2, 1]), 3, None, False),
    ("(x-1)^2(x+2) at 5", pmul(pmul([-1, 1], [-1, 1]), [2, 1]), 5, None, False),
    ("content: 9x^2-63 at 3", [-63, 0, 9], 3, 2, True),
    ("27x at 3", [0, 27], 3, 1, True),
    ("constant 12 at 2", [12], 2, 0, True),
    ("constant 12 at 5", [12], 5, 0, True),
    ("(x-3)(x-12)(x-30) at 3", pfrom_roots([3, 12, 30]), 3, 3, True),
    ("(x-1)(x-1-2^6) at 2", pfrom_roots([1, 65]), 2, 2, True),
    ("(3x-1)(x-5) at 2", pmul([-1, 3], [-5, 1]), 2, 2, True),
    ("(2x-1)(x-5) at 2", pmul([-1, 2], [-5, 1]), 2, 1, True),
    ("x^7-x at 7", [0, -1, 0, 0, 0, 0, 0, 1], 7, 7, True),
    ("x^4-4 at 2", [-4, 0, 0, 0, 1], 2, 0, True),
    ("x^2-8 at 2 ... x^2 - 16 at 2", [-16, 0, 1], 2, 2, True),
    ("x^2-p^6 at 5", [-5 ** 6, 0, 1], 5, 2, True),
]


def check_s2_certificate():
    """Proposition 3.2: a ball with a certificate (a, k, s) holds exactly one root, and that root is simple with
    v(f'(root)) = s. Oracle: among the x modulo p^M (M = k + s + 3) with x = a modulo p^(s+1), exactly p^s
    satisfy f(x) = 0 modulo p^M, and all of them are a modulo p^k. The certificate does not depend on the
    representative of a modulo p^k."""
    n = bad = 0
    for name, f, p, _, _ in PADIC_CASES:
        st, certs, _ = padic_roots(f, p, 2, 6)
        f = strip_content(f, p)
        for a, k, s in certs:
            M = k + s + 3
            sols = approx_roots(f, p, M, inside=(a % p ** (s + 1), s + 1))
            n += 1
            if len(sols) != p ** s or any((x - a) % p ** k for x in sols):
                bad += 1
            for t in (1, 2, -3):
                if not root_cert_ok(f, p, (a + t * p ** k) % p ** k, k, s):
                    bad += 1
                a2 = a + t * p ** k                                  # another representative, not reduced
                d = peval(pderiv(f), a2)
                if peval(f, a2) % p ** (k + s) or d % p ** s or d % p ** (s + 1) == 0:
                    bad += 1
    # random search for certificates by definition, tested by the same oracle
    rnd = 0
    for _ in range(300):
        p = random.choice((2, 3, 5))
        f = ptrim([random.randrange(-9, 10) for _ in range(random.randint(2, 5))])
        if len(f) < 2:
            continue
        for k in (1, 2, 3, 4):
            for a in range(p ** k):
                for s in range(k):
                    if root_cert_ok(f, p, a, k, s):
                        rnd += 1
                        sols = approx_roots(f, p, k + s + 3, inside=(a % p ** (s + 1), s + 1))
                        if len(sols) != p ** s or any((x - a) % p ** k for x in sols):
                            bad += 1
    report("check_s2_certificate", bad == 0 and n > 0 and rnd > 0,
           f"{n} certificates of the named cases, {rnd} found by search in random polynomials; {bad} failures")


def check_s2_newton():
    """Proposition 3.3: the Newton step gives a certificate of precision 2k - s with the same s, and the new
    centre is the old one modulo p^k."""
    n = bad = 0
    for name, f, p, _, _ in PADIC_CASES:
        st, certs, _ = padic_roots(f, p, 1, 6)
        f = strip_content(f, p)
        for a, k, s in certs:
            a0, k0 = a, k
            for _ in range(5):
                a1, k1 = newton_step(f, p, a0, k0, s)
                n += 1
                if k1 != 2 * k0 - s or (a1 - a0) % p ** k0 or not root_cert_ok(f, p, a1, k1, s):
                    bad += 1
                a0, k0 = a1, k1
            # requested precisions: the ball returned has precision max(kreq, s + 1) and is nested
            prev = None
            for kreq in (1, 2, 5, 17, 60):
                _, cs, _ = padic_roots(f, p, kreq, 6)
                mine = [c for c in cs if (c[0] - a) % p ** min(c[1], k) == 0]
                n += 1
                if len(mine) != 1 or mine[0][1] != max(kreq, s + 1) or mine[0][2] != s:
                    bad += 1
                elif prev is not None and (mine[0][0] - prev[0]) % p ** prev[1]:
                    bad += 1
                prev = mine[0] if mine else None
    report("check_s2_newton", bad == 0 and n > 0, f"{n} steps and requests; {bad} failures")


def check_s2_descent():
    """Propositions 3.4, 3.5: the list of certified balls and unresolved classes. Oracle: the x modulo p^M
    with f(x) = 0 modulo p^M. Every such x lies in a certified ball or in an unresolved class; every
    certified ball holds p^s of them; the balls and classes are pairwise disjoint; the number of roots is the
    expected one; when no class is unresolved the x of the oracle are exactly those of the certified balls."""
    n = bad = 0
    details = []
    for name, f, p, expect, simple in PADIC_CASES:
        for depth in (0, 1, 3, 8):
            st, certs, unres = padic_roots(f, p, 3, depth)
            n += 1
            ok = True
            # pairwise disjoint: two balls meet exactly when one centre is in the other ball
            items = [(a, k) for a, k, _ in certs] + list(unres)
            for i in range(len(items)):
                for j in range(i):
                    (a, k), (b, l) = items[i], items[j]
                    if (a - b) % p ** min(k, l) == 0:
                        ok = False
            if simple and depth >= 8 and (st != OK or (expect is not None and len(certs) != expect)):
                ok = False
            if not simple and st == OK:
                ok = False                                          # a multiple root is never resolved
            # near a root of multiplicity 2 the approximate roots modulo p^M fill a ball of radius about
            # p^(-M/2), so the oracle needs M above twice the depth of an unresolved class
            M = max([k + s for _, k, s in certs] + [2 * e for _, e in unres] + [3]) + (6 if simple else 3)
            try:
                sols = approx_roots(strip_content(f, p), p, M)
            except OverflowError:
                sols = None
            if sols is not None:
                for x in sols:
                    inc = sum(1 for a, k, _ in certs if (x - a) % p ** k == 0)
                    inu = sum(1 for a, e in unres if (x - a) % p ** e == 0)
                    if inc + inu != 1:
                        ok = False
                for a, k, s in certs:
                    if sum(1 for x in sols if (x - a) % p ** k == 0) != p ** s:
                        ok = False
            if not ok:
                bad += 1
                details.append((name, depth))
    # random polynomials with known roots: products of linear factors with integer roots and a factor
    # without roots
    rnd = 0
    for _ in range(150):
        p = random.choice((2, 3, 5, 7))
        roots = sorted(set(random.randrange(-40, 40) for _ in range(random.randint(1, 4))))
        f = pfrom_roots(roots, lead=random.choice((1, 2, 3, p)))
        # factors without a root in Z_p: x^2 + 1 for p = 3 modulo 4; x^2 + 2 for p = 5 (-2 is no square)
        noroot = random.choice(([1], [1, 0, 1] if p % 4 == 3 else [1], [2, 0, 1] if p == 5 else [1]))
        f = pmul(f, noroot)
        st, certs, unres = padic_roots(f, p, 4, 12)
        rnd += 1
        found = sorted(a for a, _, _ in certs)
        if st != OK or len(certs) != len(roots):
            bad += 1
            details.append(("random", roots, p))
            continue
        for r in roots:
            if sum(1 for a, k, _ in certs if (r - a) % p ** k == 0) != 1:
                bad += 1
    report("check_s2_descent", bad == 0, f"{n} runs of the named cases (depth limits 0, 1, 3, 8), {rnd} random "
           f"products with known integer roots; {bad} failures" + (f" {details[:3]}" if details else ""))


def check_s2_examples():
    """The examples of the brief and of SPEC 9.1, with their values: x^2 + 1 at 2 has the root 1 modulo 2 and
    no root in Z_2; 9 is a square in Z_2 (roots 3 and -3, s = 1); 3 is not; Conrad's examples 4.2, 4.3, 4.4."""
    bad = n = 0

    def run(f, p, kreq, depth=8):
        return padic_roots(f, p, kreq, depth)

    checks = [
        (run([1, 0, 1], 2, 3), (OK, [], [])),
        (run([1, 0, 1], 2, 3, depth=0), (NOT_DETERMINED, [], [(1, 1)])),
        (run([-9, 0, 1], 2, 4), (OK, [(3, 4, 1), (13, 4, 1)], [])),
        (run([-3, 0, 1], 2, 4), (OK, [], [])),
        (run([-7, 0, 1], 3, 5), (OK, [(1 + 3 + 9 + 2 * 81, 5, 0), (3 ** 5 - (1 + 3 + 9 + 2 * 81), 5, 0)], [])),
        (run([-10, 0, 0, 1], 3, 2), (OK, [(4, 2, 1)], [])),
        (run([-5, 0, 0, 1], 3, 2), (OK, [], [])),
        (run([0, 27], 3, 2), (OK, [(0, 2, 0)], [])),
        (run([], 3, 2), (DOMAIN, [], [])),
        (run([12], 5, 2), (OK, [], [])),
    ]
    for got, want in checks:
        n += 1
        if (got[0], sorted(got[1]), sorted(got[2])) != (want[0], sorted(want[1]), sorted(want[2])):
            bad += 1
    # Conrad 4.2: two roots in Z_3, congruent to 2 and 5 modulo 9 (hensel.txt:332-340)
    st, certs, _ = run([1, 2, 2, -7, 1], 3, 2)
    n += 1
    if st != OK or sorted(a % 9 for a, _, _ in certs) != [2, 5]:
        bad += 1
    # Conrad 4.4: x^3 - x^2 - 2x - 8 at 2: roots 1 + 2 + 4 + ... (3 modulo 4), 0 modulo 4, 2 modulo 4
    # (hensel.txt:352-360)
    st, certs, _ = run([-8, -2, -1, 1], 2, 2)
    n += 1
    if st != OK or sorted((a % 4, s) for a, _, s in certs) != [(0, 1), (2, 1), (3, 0)]:
        bad += 1
    report("check_s2_examples", bad == 0, f"{n} fixed cases; {bad} failures")


def pmod(f, p):
    return ptrim([c % p for c in f])


def pdivmod_p(f, g, p):
    f, g = pmod(f, p), pmod(g, p)
    q = [0] * max(0, len(f) - len(g) + 1)
    inv = pow(g[-1], -1, p)
    while len(f) >= len(g):
        c = f[-1] * inv % p
        d = len(f) - len(g)
        q[d] = c
        f = pmod([x - c * (g[i - d] if 0 <= i - d < len(g) else 0) for i, x in enumerate(f)], p)
    return q, f


def pgcd_p(f, g, p):
    f, g = pmod(f, p), pmod(g, p)
    while g:
        f, g = g, pdivmod_p(f, g, p)[1]
    return f


def xp_minus_x_mod(g, p):
    """X^p - X modulo g in F_p[X], by repeated squaring."""
    result, base, e = [1], pdivmod_p([0, 1], g, p)[1], p
    while e:
        if e & 1:
            result = pdivmod_p(pmul(result, base), g, p)[1]
        base = pdivmod_p(pmul(base, base), g, p)[1]
        e >>= 1
    out = list(result) + [0] * max(0, 2 - len(result))
    out[1] -= 1
    return pmod(out, p)


def check_s2_count_mod_p():
    """Proposition 3.7: the number of distinct roots in F_p of a polynomial that is not zero modulo p is the
    degree of gcd(g, X^p - X). Against the evaluation at all residues."""
    n = bad = 0
    for p in (2, 3, 5, 7, 11, 13, 101, 257):
        for _ in range(60):
            g = pmod([random.randrange(p) for _ in range(random.randint(1, 7))], p)
            if random.random() < 0.5:
                for _ in range(random.randint(1, 3)):
                    g = pmod(pmul(g, [random.randrange(p), 1]), p)
            if not g:
                continue
            n += 1
            count = sum(1 for a in range(p) if peval(g, a, p) == 0)
            if len(g) == 1:
                d = 0
            else:
                h = xp_minus_x_mod(g, p)
                d = len(pgcd_p(g, h, p)) - 1 if h else len(g) - 1
            if d != count:
                bad += 1
    report("check_s2_count_mod_p", bad == 0, f"{n} polynomials over F_p, p up to 257; {bad} failures")


# ---- real roots ----

def fpoly(f):
    return [F(c) for c in f]


def fdivmod(f, g):
    f = list(f)
    q = [F(0)] * max(0, len(f) - len(g) + 1)
    while len(f) >= len(g) and any(f):
        c = f[-1] / g[-1]
        d = len(f) - len(g)
        q[d] = c
        f = [x - c * (g[i - d] if 0 <= i - d < len(g) else 0) for i, x in enumerate(f)]
        while f and f[-1] == 0:
            f.pop()
    return q, f


def squarefree_part(f):
    """f / gcd(f, f') over Q, scaled to a primitive integer polynomial with positive leading coefficient."""
    a, b = fpoly(f), fpoly(pderiv(f))
    while b:
        a, b = b, fdivmod(a, b)[1]
    g = fdivmod(fpoly(f), a)[0]
    den = 1
    for c in g:
        den = den * c.denominator // gcd(den, c.denominator)
    gi = [int(c * den) for c in g]
    cont = 0
    for c in gi:
        cont = gcd(cont, c)
    gi = [c // cont for c in gi]
    if gi[-1] < 0:
        gi = [-c for c in gi]
    return gi


def sign(x):
    return (x > 0) - (x < 0)


def sturm_chain(g):
    chain = [fpoly(g), fpoly(pderiv(g))]
    while chain[-1]:
        r = fdivmod(chain[-2], chain[-1])[1]
        chain.append([-c for c in r])
    chain.pop()
    return chain


def sign_changes(vals):
    s = [v for v in vals if v != 0]
    return sum(1 for i in range(1, len(s)) if s[i] != s[i - 1])


def sturm_count(g, lo=None, hi=None):
    """Oracle: the number of distinct real roots of the squarefree g in (lo, hi] (Sturm), or on the whole
    line for lo = hi = None."""
    chain = sturm_chain(g)

    def at(x):
        if x is None:
            return None
        return [sign(sum(c * F(x) ** i for i, c in enumerate(q))) for q in chain]

    if lo is None:
        minus = [sign(q[-1]) * (-1 if (len(q) - 1) % 2 else 1) for q in chain]
        plus = [sign(q[-1]) for q in chain]
        return sign_changes(minus) - sign_changes(plus)
    return sign_changes(at(lo)) - sign_changes(at(hi))


def real_cert_ok(f, n, balls):
    """Proposition 3.8, the checker: balls = list of (lo, hi) exact rationals in increasing order; each has
    lo < hi and f(lo) f(hi) < 0, or lo = hi and f(lo) = 0; they are pairwise disjoint; their number is n."""
    prev = None
    for lo, hi in balls:
        if lo > hi:
            return False
        if lo == hi:
            if peval(fpoly(f), F(lo)) != 0:
                return False
        elif sign(peval(fpoly(f), F(lo))) * sign(peval(fpoly(f), F(hi))) >= 0:
            return False
        if prev is not None and not prev < lo:
            return False
        prev = hi
    return len(balls) == n


def real_roots_ref(f, bits):
    """Reference for Algorithm RR of solvers.md 3.10 with the library's own arithmetic: count by Sturm,
    isolation by bisection of (-B, B], B a power of two above Cauchy's bound, until every interval holds one
    root and is shorter than 2^-bits. Returns (status, n, balls)."""
    f = ptrim(f)
    if not f:
        return DOMAIN, 0, []
    g = squarefree_part(f)
    if len(g) == 1:
        return OK, 0, []
    n = sturm_count(g)
    B = 1
    while B <= 1 + max(abs(c) for c in g[:-1]) / abs(g[-1]):
        B *= 2
    work, balls = [(F(-B), F(B))], []
    while work:
        lo, hi = work.pop()
        k = sturm_count(g, lo, hi)
        if k == 0:
            continue
        if k == 1 and hi - lo <= F(1, 2 ** bits):
            if peval(fpoly(g), hi) == 0:
                balls.append((hi, hi))
            else:
                balls.append((lo, hi))
            continue
        mid = (lo + hi) / 2
        work.append((lo, mid))
        work.append((mid, hi))
    balls.sort()
    # (lo, hi] holds one root and its closure may touch the interval before it: move lo to the right, or
    # find the root itself
    fixed = []
    for lo, hi in balls:
        while lo < hi:
            mid = (lo + hi) / 2
            if peval(fpoly(g), mid) == 0:
                lo = hi = mid
            elif sturm_count(g, mid, hi) == 1:
                lo = mid
                break
            else:
                hi = mid
        fixed.append((lo, hi))
    ok = real_cert_ok(g, n, fixed)
    return (OK if ok else NOT_DETERMINED), n, fixed


REAL_CASES = [
    ("(x-1)(x-2)(x-3)", pfrom_roots([1, 2, 3]), 3),
    ("x^2-2", [-2, 0, 1], 2),
    ("x^2+1", [1, 0, 1], 0),
    ("x", [0, 1], 1),
    ("7", [7], 0),
    ("2x-1", [-1, 2], 1),
    ("(2x-1)(x^2-2)", pmul([-1, 2], [-2, 0, 1]), 3),
    ("(x-1)^2(x+3)", pmul(pmul([-1, 1], [-1, 1]), [3, 1]), 2),
    ("x^3(x-1)^2(x^2+1)", pmul(pmul([0, 0, 0, 1], pmul([-1, 1], [-1, 1])), [1, 0, 1]), 2),
    ("x^5-x-1", [-1, -1, 0, 0, 0, 1], 1),
    ("(1000x^2-2000)(1000x^2-2001)", pmul([-2000, 0, 1000], [-2001, 0, 1000]), 4),
    ("(x^2-2)^2-10^-12 scaled", [4 * 10 ** 12 - 1, 0, -4 * 10 ** 12, 0, 10 ** 12], 4),
    ("Wilkinson 10", pfrom_roots(range(1, 11)), 10),
    ("(x^2-13)(x^2-17)(x^2-221)", pmul(pmul([-13, 0, 1], [-17, 0, 1]), [-221, 0, 1]), 6),
    ("Chebyshev T_6", [-1, 0, 18, 0, -48, 0, 32], 6),
    ("x^4+x^3+x^2+x+1", [1, 1, 1, 1, 1], 0),
    ("x^4-10x^2+1", [1, 0, -10, 0, 1], 4),
    ("(3x+7)(5x-11)(x^2+x+1)", pmul(pmul([7, 3], [-11, 5]), [1, 1, 1]), 2),
]


def check_s2_real_completeness():
    """Proposition 3.8: count plus isolation gives completeness. For each case: the reference isolates n
    intervals; the checker accepts them; n is the expected number; every interval holds exactly one root and
    there is no root outside (Sturm counts on the intervals and on the gaps between them, an independent use
    of the chain); a list with one interval removed, with two intervals merged, or with an interval moved
    off its root is refused."""
    n = bad = mut = 0
    for name, f, expect in REAL_CASES:
        st, cnt, balls = real_roots_ref(f, 20)
        g = squarefree_part(f)
        n += 1
        ok = st == OK and cnt == expect and len(balls) == expect and real_cert_ok(g, cnt, balls)
        mp_count = None
        try:
            import mpmath
            if len(g) > 1:
                mpmath.mp.dps = 40
                rts = mpmath.polyroots([mpmath.mpf(c) for c in reversed(g)], maxsteps=2000, extraprec=400)
                mp_count = sum(1 for z in rts if abs(mpmath.im(z)) < mpmath.mpf(10) ** -25)
            else:
                mp_count = 0
        except Exception:
            mp_count = None
        if mp_count is not None and mp_count != expect:
            ok = False
        for lo, hi in balls:
            inside = 1 if lo == hi else sturm_count(g, lo, hi) + (1 if peval(fpoly(g), lo) == 0 else 0)
            if inside != 1:
                ok = False
        if not ok:
            bad += 1
        if balls:
            mut += 1
            if real_cert_ok(g, cnt, balls[1:]):
                bad += 1
            lo, hi = balls[0]
            w = (hi - lo) if hi > lo else F(1, 2 ** 20)
            mut += 1
            if real_cert_ok(g, cnt, [(hi + w / 8, hi + w / 4)] + balls[1:]) and \
                    sturm_count(g, hi + w / 8, hi + w / 4) == 0:
                bad += 1
        if len(balls) >= 2:
            mut += 1
            merged = [(balls[0][0], balls[1][1])] + balls[2:]
            if real_cert_ok(g, cnt, merged):
                bad += 1
    report("check_s2_real_completeness", bad == 0, f"{n} polynomials, {mut} changed lists; {bad} failures")


class FmpzPoly(ctypes.Structure):
    _fields_ = [("coeffs", ctypes.c_void_p), ("alloc", ctypes.c_long), ("length", ctypes.c_long)]


def flint_poly(f):
    pol = FmpzPoly()
    FLINT.fmpz_poly_init(ctypes.byref(pol))
    for i, c in enumerate(f):
        FLINT.fmpz_poly_set_coeff_si(ctypes.byref(pol), ctypes.c_long(i), ctypes.c_long(c))
    return pol


def flint_fmpz_get(ptr):
    FLINT.fmpz_get_str.restype = ctypes.c_void_p
    s = FLINT.fmpz_get_str(None, 10, ptr)
    out = int(ctypes.cast(s, ctypes.c_char_p).value)
    FLINT.flint_free(ctypes.c_void_p(s))
    return out


def flint_real_roots(g, prec):
    """For the squarefree g (small coefficients): (fmpz_poly_num_real_roots, the real enclosures of
    arb_fmpz_poly_complex_roots as exact rational intervals, the number of enclosures that are not real)."""
    pol = flint_poly(g)
    FLINT.fmpz_poly_num_real_roots.restype = ctypes.c_long
    cnt = FLINT.fmpz_poly_num_real_roots(ctypes.byref(pol))
    deg = len(g) - 1
    FLINT._acb_vec_init.restype = ctypes.c_void_p
    vec = FLINT._acb_vec_init(ctypes.c_long(deg))
    FLINT.arb_fmpz_poly_complex_roots(ctypes.c_void_p(vec), ctypes.byref(pol), 0, ctypes.c_long(prec))
    balls, nonreal = [], 0
    a, b, e = ctypes.c_long(0), ctypes.c_long(0), ctypes.c_long(0)
    for i in range(deg):
        re = ctypes.c_void_p(vec + 96 * i)
        im = ctypes.c_void_p(vec + 96 * i + 48)
        FLINT.arb_is_zero.restype = ctypes.c_int
        if not FLINT.arb_is_zero(im):
            nonreal += 1
            continue
        FLINT.arb_get_interval_fmpz_2exp(ctypes.byref(a), ctypes.byref(b), ctypes.byref(e), re)
        ia, ib, ie = flint_fmpz_get(ctypes.byref(a)), flint_fmpz_get(ctypes.byref(b)), flint_fmpz_get(
            ctypes.byref(e))
        balls.append((F(ia) * F(2) ** ie, F(ib) * F(2) ** ie))
        for z in (a, b, e):
            FLINT.fmpz_clear(ctypes.byref(z))
            z.value = 0
    FLINT._acb_vec_clear(ctypes.c_void_p(vec), ctypes.c_long(deg))
    FLINT.fmpz_poly_clear(ctypes.byref(pol))
    return cnt, balls, nonreal


def probe_s2_flint_real():
    """Proposition 3.9 and Algorithm RR with FLINT 3.0.1 as the engine: for the squarefree part g of each case,
    fmpz_poly_num_real_roots(g) equals the count of the oracle; the real enclosures of
    arb_fmpz_poly_complex_roots pass the checker of Proposition 3.8 (exact sign change at the exact end
    points, disjoint, their number equal to the count), after at most one widening of a ball whose end point
    or centre is a root."""
    if FLINT is None:
        skip("probe_s2_flint_real", "libflint not found")
        return
    n = bad = widened = exact = 0
    for name, f, expect in REAL_CASES:
        g = squarefree_part(f)
        if len(g) < 2 or max(abs(c) for c in g) >= SMALL:
            continue
        for prec in (16, 64, 200):
            cnt, balls, nonreal = flint_real_roots(g, prec)
            n += 1
            fixed = []
            for lo, hi in balls:
                if lo == hi and peval(fpoly(g), lo) == 0:
                    exact += 1
                elif lo == hi or sign(peval(fpoly(g), lo)) * sign(peval(fpoly(g), hi)) >= 0:
                    rad = (hi - lo) / 2 if hi > lo else F(1, 2 ** prec)
                    lo, hi = lo - rad, hi + rad
                    widened += 1
                fixed.append((lo, hi))
            if cnt != expect or len(balls) != expect or nonreal != len(g) - 1 - expect:
                bad += 1
            elif not real_cert_ok(g, cnt, fixed):
                bad += 1
    report("probe_s2_flint_real", bad == 0, f"FLINT {flint_version()}: {n} calls (prec 16, 64, 200); balls widened "
           f"once: {widened}; exact balls: {exact}; {bad} failures")


PART3 = [check_s2_certificate, check_s2_newton, check_s2_descent, check_s2_examples, check_s2_count_mod_p,
         check_s2_real_completeness, probe_s2_flint_real]


def main():
    for fn in PART1 + PART2 + PART3:
        t = time.time()
        fn()
        dt = time.time() - t
        if dt > 20:
            print(f"     ({fn.__name__} took {dt:.0f} s)")
    print(f"total time {time.time() - T0:.1f} s; failures: {len(FAILURES)}" +
          (f" ({', '.join(FAILURES)})" if FAILURES else ""))
    sys.exit(1 if FAILURES else 0)


if __name__ == "__main__":
    main()
