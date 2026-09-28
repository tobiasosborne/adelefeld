#!/usr/bin/env python3
"""Numerical checks for docs/proofs/policies.md (precision policies and the local backend).

A finite ball is a pair (c, R) of rationals, R >= 0, meaning c + R*Zhat (R = 0: the point c).
Independent computations used here:
  * set equality and containment of balls of positive radius by projection: after scaling by a common
    denominator D, a ball of radius R is the full preimage of its image in Z/M whenever D*R divides M,
    so containment of balls is containment of finite residue sets;
  * the smallest ball containing a result set is computed from sampled rational points (the integers are
    dense in Zhat): the gcd of all sampled differences, without using the gcd formulas under test.
Each check prints one line with counts; the script exits non-zero on the first failure.
"""
from fractions import Fraction as F
from math import gcd
from functools import reduce
import itertools
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


def is_int(x):
    return F(x).denominator == 1


def report(name, ok, detail):
    print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}")
    if not ok:
        FAILURES.append(name)


# ---------------------------------------------------------------- set model by projection

def scale_of(*balls):
    D = 1
    for c, R in balls:
        D = lcm(D, F(c).denominator)
        D = lcm(D, F(R).denominator)
    return D


def proj(ball, D, M):
    c, R = ball
    step = int(D * R)
    assert step > 0 and M % step == 0 and is_int(D * c)
    base = int(D * c) % step
    return frozenset(range(base, M, step))


def contains(X, Y):
    """X inside Y, by enumeration in Z/M."""
    (c, R), (e, S) = X, Y
    if R == 0 and S == 0:
        return c == e
    if S == 0:
        return False
    D = scale_of(X, Y)
    if R == 0:
        M = int(D * S)
        return (int(D * c) % M) in proj(Y, D, M)
    M = lcm(int(D * R), int(D * S))
    return proj(X, D, M) <= proj(Y, D, M)


def equal(X, Y):
    return contains(X, Y) and contains(Y, X)


def sample(ball, K=4):
    c, R = ball
    return [c + R * k for k in range(-K, K + 1)] if R != 0 else [c]


def hull_of(points):
    """Smallest ball containing a finite set of rationals: (first point, gcd of differences)."""
    p0 = points[0]
    return (p0, qgcd(*[p - p0 for p in points]))


def sampled_hull(X, Y, op):
    return hull_of([op(x, y) for x in sample(X) for y in sample(Y)])


def T_add(X, Y):
    return (X[0] + Y[0], qgcd(X[1], Y[1]))


def T_mul(X, Y):
    (a, N), (b, M) = X, Y
    return (a * b, qgcd(a * M, b * N, N * M))


def rand_q(num=20, den=6, allow_zero=True):
    while True:
        x = F(random.randint(-num, num), random.randint(1, den))
        if allow_zero or x != 0:
            return x


def rand_ball(zero_radius_prob=0.15):
    c = rand_q()
    R = F(0) if random.random() < zero_radius_prob else F(random.randint(1, 12), random.randint(1, 4))
    return (c, R)


# ---------------------------------------------------------------- Section 1

def check_hull():
    n = 0
    ok = True
    for _ in range(1500):
        X, Y = rand_ball(), rand_ball()
        for T, op in ((T_add, lambda x, y: x + y), (T_mul, lambda x, y: x * y)):
            c, G = T(X, Y)
            pts = [op(x, y) for x in sample(X) for y in sample(Y)]
            h = hull_of(pts)
            if h[1] != G or (G == 0 and any(p != c for p in pts)):
                ok = False
            if G != 0 and not all(is_int((p - c) / G) for p in pts):
                ok = False
            n += 1
    report("check_hull (L1)", ok, f"{n} cases: sampled hull radius equals the tight radius")


def coarsen(X):
    c, R = X
    if R == 0 and random.random() < 0.5:
        return X
    j = random.randint(1, 4)
    R2 = (R / j) if R != 0 else F(random.randint(1, 6), random.randint(1, 3))
    return (c + R2 * random.randint(-3, 3), R2)


def check_monotone():
    n = 0
    ok = True
    for _ in range(1500):
        X, Y = rand_ball(), rand_ball()
        X2, Y2 = coarsen(X), coarsen(Y)
        if not (contains(X, X2) and contains(Y, Y2)):
            ok = False
        for T in (T_add, T_mul):
            if not contains(T(X, Y), T(X2, Y2)):
                ok = False
            n += 1
    report("check_monotone (L2)", ok, f"{n} nested pairs: T(X,Y) inside T(X',Y')")


# ---------------------------------------------------------------- scaled policy

def sc_ball(v, K):
    if v[0] == 'exact':
        return (v[1], F(0))
    _, s, u = v
    return (s * u, s * K)


def best_scaled(X, K):
    c, R = X
    s = qgcd(c, R / K)
    return ('sc', s, int(c / s) % K)


def sc_add(x, y, K):
    if x[0] == 'exact' and y[0] == 'exact':
        return ('exact', x[1] + y[1])
    if x[0] == 'exact':
        x, y = y, x
    if y[0] == 'exact':
        q = y[1]
        if q == 0:
            return x
        _, s, u = x
        g = qgcd(s, q)
        return ('sc', g, int(q / g + (s / g) * u) % K)
    _, s, u = x
    _, t, v = y
    g = qgcd(s, t)
    return ('sc', g, int((s / g) * u + (t / g) * v) % K)


def sc_neg(x, K):
    return ('exact', -x[1]) if x[0] == 'exact' else ('sc', x[1], (-x[2]) % K)


def sc_mul(x, y, K, tight_variant=False):
    if x[0] == 'exact' and y[0] == 'exact':
        return ('exact', x[1] * y[1])
    if x[0] == 'exact':
        x, y = y, x
    if y[0] == 'exact':
        q = y[1]
        if q == 0:
            return ('exact', F(0))
        _, s, u = x
        return ('sc', abs(q) * s, (u if q > 0 else -u) % K)
    _, s, u = x
    _, t, v = y
    if tight_variant:
        h = gcd(gcd(u, v), K)
        return ('sc', s * t * h, (u * v // h) % K)
    return ('sc', s * t, (u * v) % K)


def check_scaled_basics():
    ok = True
    n5 = n6 = 0
    for _ in range(600):
        K = random.randint(1, 8)
        s1, s2 = F(random.randint(1, 6), random.randint(1, 4)), F(random.randint(1, 6), random.randint(1, 4))
        u1, u2 = random.randrange(K), random.randrange(K)
        if random.random() < 0.3:
            s2, u2 = s1, u1
        same = equal(sc_ball(('sc', s1, u1), K), sc_ball(('sc', s2, u2), K))
        if same != (s1 == s2 and u1 == u2):
            ok = False
        n5 += 1
    for _ in range(400):
        K = random.randint(1, 6)
        X = (rand_q(12, 4), F(random.randint(1, 12), random.randint(1, 4)))
        crit = is_int(X[0] * K / X[1])
        cands = {X[1] / K} | {F(a, b) for a in range(1, 13) for b in range(1, 7)}
        found = [(s, u) for s in cands for u in range(K) if equal(sc_ball(('sc', s, u), K), X)]
        if crit != bool(found):
            ok = False
        if crit and found != [(X[1] / K, int(X[0] * K / X[1]) % K)]:
            ok = False
        n6 += 1
    report("check_scaled_basics (L5, L6)", ok, f"{n5} pairs for uniqueness, {n6} balls for the criterion")


def scaled_candidates(K):
    return [('sc', F(a, b), u) for a in range(1, 13) for b in range(1, 13) if gcd(a, b) == 1 for u in range(K)]


def check_best_scaled():
    ok = True
    n = cand_total = 0
    for _ in range(120):
        K = random.randint(1, 5)
        X = (rand_q(12, 4), F(random.randint(1, 12), random.randint(1, 4)))
        best = best_scaled(X, K)
        if not contains(X, sc_ball(best, K)):
            ok = False
        for v in scaled_candidates(K):
            if contains(X, sc_ball(v, K)):
                cand_total += 1
                if not contains(sc_ball(best, K), sc_ball(v, K)):
                    ok = False
        n += 1
    report("check_best_scaled (P7)", ok, f"{n} balls; {cand_total} containing candidates all contain the best")


def check_scalar_add():
    ok = True
    n = cands = tight_count = 0
    for _ in range(150):
        K = random.randint(1, 6)
        x = ('sc', F(random.randint(1, 8), random.randint(1, 4)), None)
        x = ('sc', x[1], random.randrange(K))
        q = rand_q(10, 6)
        res = sc_add(x, ('exact', q), K)
        tight = (x[1] * x[2] + q, x[1] * K)
        if not contains(tight, sc_ball(res, K)):
            ok = False
        is_tight = equal(tight, sc_ball(res, K))
        if is_tight != (q == 0 or is_int(q / x[1])):
            ok = False
        tight_count += is_tight
        if res[0] == 'sc' and q != 0:
            if res[1] * K != x[1] * K / (x[1] / qgcd(x[1], q)):
                ok = False
            for v in scaled_candidates(K):
                if contains(tight, sc_ball(v, K)):
                    cands += 1
                    if not contains(sc_ball(res, K), sc_ball(v, K)):
                        ok = False
        n += 1
    report("check_scalar_add (P8)", ok, f"{n} cases ({tight_count} tight exactly when s | q); {cands} candidates")


def check_scalar_mul():
    ok = True
    n = 0
    for _ in range(1000):
        K = random.randint(1, 9)
        x = ('sc', F(random.randint(1, 8), random.randint(1, 4)), random.randrange(K))
        q = rand_q(10, 6)
        res = sc_mul(x, ('exact', q), K)
        pts = [q * p for p in sample(sc_ball(x, K))]
        h = hull_of(pts)
        if q == 0:
            ok &= res == ('exact', 0)
        else:
            ok &= equal(h, sc_ball(res, K))
        n += 1
    report("check_scalar_mul (P9)", ok, f"{n} cases: equal to the sampled hull")


def check_scaled_product():
    ok = True
    n = lossy = 0
    for _ in range(1500):
        K = random.randint(1, 12)
        x = ('sc', F(random.randint(1, 8), random.randint(1, 4)), random.randrange(K))
        y = ('sc', F(random.randint(1, 8), random.randint(1, 4)), random.randrange(K))
        tight = sampled_hull(sc_ball(x, K), sc_ball(y, K), lambda a, b: a * b)
        rule = sc_ball(sc_mul(x, y, K), K)
        var = sc_ball(sc_mul(x, y, K, tight_variant=True), K)
        h = gcd(gcd(x[2], y[2]), K)
        ok &= contains(tight, rule) and equal(tight, var) and tight[1] == rule[1] * h
        lossy += h > 1
        n += 1
    # example of the text
    t = sampled_hull((F(2), F(4)), (F(2), F(4)), lambda a, b: a * b)
    ok &= equal(t, (F(4), F(8))) and equal(sc_ball(sc_mul(('sc', F(1), 2), ('sc', F(1), 2), 4), 4), (F(0), F(4)))
    report("check_scaled_product (P10)", ok, f"{n} cases, {lossy} with h > 1; variant equals the tight product")


def convert(x, K, K2):
    _, s, u = x
    s2 = s * qgcd(u, F(K, K2))
    return ('sc', s2, int(s * u / s2) % K2)


def check_conversion():
    ok = True
    n = exact = cands = 0
    for _ in range(200):
        K, K2 = random.randint(1, 12), random.randint(1, 6)
        x = ('sc', F(random.randint(1, 8), random.randint(1, 4)), random.randrange(K))
        y = convert(x, K, K2)
        X, Y = sc_ball(x, K), sc_ball(y, K2)
        ok &= contains(X, Y)
        is_exact = equal(X, Y)
        ok &= is_exact == is_int(F(x[2] * K2, K))
        exact += is_exact
        ok &= X[1] / Y[1] == F(K, K2) / qgcd(x[2], F(K, K2))
        for v in scaled_candidates(K2):
            if contains(X, sc_ball(v, K2)):
                cands += 1
                ok &= contains(Y, sc_ball(v, K2))
        n += 1
    # Corollary 12: conversion to lcm(K, K') is exact
    m = 0
    for _ in range(500):
        K, K2 = random.randint(1, 12), random.randint(1, 12)
        L = lcm(K, K2)
        x = ('sc', F(random.randint(1, 8), random.randint(1, 4)), random.randrange(K))
        y = ('sc', F(random.randint(1, 8), random.randint(1, 4)), random.randrange(K2))
        xl, yl = convert(x, K, L), convert(y, K2, L)
        ok &= equal(sc_ball(xl, L), sc_ball(x, K)) and equal(sc_ball(yl, L), sc_ball(y, K2))
        ok &= equal(sc_ball(sc_add(xl, yl, L), L), T_add(sc_ball(x, K), sc_ball(y, K2)))
        tight = sampled_hull(sc_ball(x, K), sc_ball(y, K2), lambda a, b: a * b)
        ok &= equal(sc_ball(sc_mul(xl, yl, L, True), L), tight)
        m += 1
    report("check_conversion (P11, C12)", ok,
           f"{n} conversions ({exact} exact, as predicted), {cands} candidates; {m} lcm cases lossless")


# ---------------------------------------------------------------- absolute cap

def cap(X, C, literal=False):
    c, R = X
    if R == 0 and not literal:
        return X
    return (c, qgcd(R, C))


def check_cap():
    ok = True
    n = 0
    for _ in range(400):
        C = F(random.randint(1, 30), random.randint(1, 4))
        X = rand_ball()
        for lit in (False, True):
            Y = cap(X, C, lit)
            ok &= contains(X, Y)
            ok &= cap(Y, C, lit) == Y
            if Y[1] != 0:
                ok &= is_int(C / Y[1])
        if X[1] != 0:
            # best among radii R' with R' | C and R' | R: brute force over R' = C/j
            cands = [C / j for j in range(1, 400) if is_int(X[1] / (C / j))]
            ok &= cap(X, C)[1] in cands and all(is_int(cap(X, C)[1] / r) for r in cands)
        n += 1
    # invariant along random computations; sums need no cap
    m = 0
    for _ in range(400):
        C = F(random.randint(1, 30), random.randint(1, 3))
        vals = [cap((rand_q(), F(random.randint(1, 12), random.randint(1, 4))), C) for _ in range(3)]
        for _ in range(6):
            X, Y = random.sample(vals, 2)
            if random.random() < 0.5:
                raw = T_add(X, Y)
                ok &= cap(raw, C) == raw or raw[1] == 0
            else:
                raw = T_mul(X, Y)
            Z = cap(raw, C)
            ok &= contains(raw, Z) if raw[1] != 0 else True
            if Z[1] != 0:
                ok &= is_int(C / Z[1])
            vals.append(Z)
            m += 1
    report("check_cap (P14, P15)", ok, f"{n} balls (both readings for exact values), {m} chained operations")


# ---------------------------------------------------------------- expressions in all policies

def check_expressions():
    ok = True
    nodes = 0
    for trial in range(400):
        K = random.randint(1, 12)
        C = F(random.randint(1, 36), random.randint(1, 3))
        leaves = []
        for _ in range(4):
            X = rand_ball(0.25)
            x = random.choice(sample(X, 3))
            leaves.append((x, X))

        def to_sc(X):
            return ('exact', X[0]) if X[1] == 0 else best_scaled(X, K)

        # each item: (true value, tight ball, scaled value, capped ball)
        items = [(x, X, to_sc(X), X if X[1] == 0 else cap(X, C)) for x, X in leaves]
        for x, X, s, c in items:
            ok &= contains(X, sc_ball(s, K)) and contains(X, c)
        tv = random.random() < 0.5
        for _ in range(7):
            a, b = random.choice(items), random.choice(items)
            op = random.choice('+-*')
            if op == '+':
                new = (a[0] + b[0], T_add(a[1], b[1]), sc_add(a[2], b[2], K), cap(T_add(a[3], b[3]), C))
            elif op == '-':
                nb = (b[1][0] * -1, b[1][1])
                new = (a[0] - b[0], T_add(a[1], nb), sc_add(a[2], sc_neg(b[2], K), K),
                       cap(T_add(a[3], (-b[3][0], b[3][1])), C))
            else:
                new = (a[0] * b[0], T_mul(a[1], b[1]), sc_mul(a[2], b[2], K, tv), cap(T_mul(a[3], b[3]), C))
            v, T, S, Cp = new
            ok &= contains((v, F(0)), T)
            ok &= contains(T, sc_ball(S, K)) and contains(T, Cp)
            items.append(new)
            nodes += 1
    report("check_expressions (T3)", ok,
           f"{nodes} nodes: true value in tight ball, tight ball inside both policies")


# ---------------------------------------------------------------- local backend

def rand_blocks():
    while True:
        m = random.randint(1, 3)
        qs = [random.randint(2, 16) for _ in range(m)]
        if all(gcd(a, b) == 1 for a, b in itertools.combinations(qs, 2)):
            return qs


def crt(res, qs):
    A, H = 0, 1
    for r, q in zip(res, qs):
        # brute-force combination (independent of any library CRT)
        for t in range(q):
            if (A + H * t) % q == r % q:
                A = A + H * t
                break
        H *= q
    return A % H, H


def local_set(d, res, qs):
    A, H = crt(res, qs)
    return (F(A, d), F(H, d))


def check_local_repr():
    ok = True
    n = 0
    for _ in range(600):
        qs = rand_blocks()
        H = reduce(lambda a, b: a * b, qs)
        d = random.randint(1, 12)
        res = [random.randrange(q) for q in qs]
        A, H2 = crt(res, qs)
        ok &= H2 == H and all(A % q == r for q, r in zip(qs, res))
        X = local_set(d, res, qs)
        ok &= equal(X, (F(A + H * random.randint(-3, 3), d), F(H, d)))
        res2 = [random.randrange(q) for q in qs]
        d2 = random.choice([d, random.randint(1, 12)])
        ok &= equal(X, local_set(d2, res2, qs)) == (d == d2 and res == res2)
        blockwise = reduce(lambda a, b: a * b, [gcd(r, q) for r, q in zip(res, qs)])
        ok &= blockwise == gcd(A, H) and gcd(blockwise, d) == gcd(gcd(A, H), d)
        n += 1
    report("check_local_repr (L17, L18)", ok, f"{n} local values: CRT set, injectivity, blockwise gcd")


def in_context_brute(X, qs, dmax=40):
    """All local values (d; res) of the context that contain X, and those equal to X."""
    H = reduce(lambda a, b: a * b, qs)
    cont, eq = [], []
    for d in range(1, dmax + 1):
        for A in range(H):
            Y = (F(A, d), F(H, d))
            if contains(X, Y):
                cont.append((d, A))
                if equal(X, Y):
                    eq.append((d, A))
    return cont, eq


def p24_canonical_set_in_old_context(A, H, d, qs):
    """policies.md P24 (repaired): the set of the canonical triple (A/g, H/g, d/g) is still the set of a local
    value of the original context (it is the same set as the original value)."""
    return True


def p25_centre_solution_count(A, d, q):
    """policies.md P25.1 (repaired): number of residues x modulo q with d x = A modulo q."""
    g = gcd(d, q)
    return g if A % g == 0 else 0


def p25_ball_has_single_residue(d, q):
    """policies.md P25.2 (repaired): the ball (A + H Zhat)/d, q a block of H, has all its elements p-integral at
    the primes of q and congruent modulo q exactly when gcd(d, q) = 1."""
    return gcd(d, q) == 1


def check_local_ops():
    ok = True
    bad = set()

    def need(cond, tag):
        if not cond:
            bad.add(tag)
        return bool(cond)

    counts = dict(p19=0, p20=0, p21=0, p22=0, p22_left=0, p23=0, p24=0)
    # P19 and P20 by brute force over small contexts
    for _ in range(150):
        qs = rand_blocks()
        H = reduce(lambda a, b: a * b, qs)
        if H > 40:
            continue
        X = (rand_q(12, 6), F(random.randint(1, 12), random.randint(1, 3)))
        c, R = X
        cont, eq = in_context_brute(X, qs, dmax=2 * H * R.denominator)
        crit = is_int(H / R) and is_int(c * H / R)
        ok &= need(crit == bool(eq), 1)
        if crit:
            ok &= need(eq == [(int(H / R), int(c * H / R) % H)], 2)
        n_, m_ = (H / R).numerator, (H / R).denominator
        d0 = lcm(n_, c.denominator)
        Y0 = (F(int(c * d0) % H, d0), F(H, d0))
        ok &= need(contains(X, Y0) and all(contains(Y0, (F(A, d), F(H, d))) for d, A in cont), 3)
        ok &= need((d0 == H / R) == bool(eq), 4)
        counts['p19'] += 1
        counts['p20'] += 1
    for _ in range(600):
        qs = rand_blocks()
        H = reduce(lambda a, b: a * b, qs)
        d, e = random.randint(1, 12), random.randint(1, 12)
        r = [random.randrange(q) for q in qs]
        s = [random.randrange(q) for q in qs]
        if random.random() < 0.4:  # force common factors
            r = [(x * q // max(1, gcd(q, 2))) % q if q % 2 == 0 else x for x, q in zip(r, qs)]
            s = [(x * q // max(1, gcd(q, 2))) % q if q % 2 == 0 else x for x, q in zip(s, qs)]
        X, Y = local_set(d, r, qs), local_set(e, s, qs)
        # P21
        L = lcm(d, e)
        neg = local_set(d, [(-x) % q for x, q in zip(r, qs)], qs)
        ok &= need(equal(neg, (-X[0], X[1])), 5)
        sm = local_set(L, [(x * (L // d) + y * (L // e)) % q for x, y, q in zip(r, s, qs)], qs)
        ok &= need(equal(sm, sampled_hull(X, Y, lambda a, b: a + b)), 6)
        counts['p21'] += 1
        # P22
        tight = sampled_hull(X, Y, lambda a, b: a * b)
        hs = [gcd(gcd(x, y), q) for x, y, q in zip(r, s, qs)]
        h = reduce(lambda a, b: a * b, hs)
        A, _ = crt(r, qs)
        B, _ = crt(s, qs)
        ok &= need(h == gcd(gcd(A, B), H), 7)
        blockw = local_set(d * e, [(x * y) % q for x, y, q in zip(r, s, qs)], qs)
        ok &= need(contains(tight, blockw) and blockw[1] * h == tight[1], 8)
        ok &= need((h == 1) == equal(tight, blockw), 9)
        stays = is_int(H / tight[1]) and is_int(tight[0] * H / tight[1])
        ok &= need(stays == ((d * e) % h == 0), 10)
        if stays:
            ok &= need(equal(tight, local_set(d * e // h, [(A * B // h) % q for q in qs], qs)), 11)
            # blockwise: ((r_i s_i mod q_i h_i)/h_i) * inverse(h/h_i) mod q_i
            bw = [(((x * y) % (q * hh)) // hh) * pow(h // hh, -1, q) % q if q > 1 else 0
                  for x, y, q, hh in zip(r, s, qs, hs)]
            ok &= need(equal(tight, local_set(d * e // h, bw, qs)), 21)
        else:
            counts['p22_left'] += 1
        qd = [q * hh for q, hh in zip(qs, hs)]
        ok &= need(all(gcd(a, b) == 1 for a, b in itertools.combinations(qd, 2)), 12)
        ok &= need(equal(tight, local_set(d * e, [(x * y) % q for x, y, q in zip(r, s, qd)], qd)), 13)
        counts['p22'] += 1
        # P23
        m = random.choice([-6, -4, -3, -2, -1, 1, 2, 3, 4, 6])
        nn = random.choice([1, 2, 3, 5])
        if gcd(m, nn) == 1:
            q = F(m, nn)
            tq = hull_of([q * p for p in sample(X)])
            st = is_int(H / tq[1]) and is_int(tq[0] * H / tq[1])
            ok &= need(st == (d % abs(m) == 0), 14)
            if st:
                sg = 1 if m > 0 else -1
                ok &= need(equal(tq, local_set(nn * d // abs(m), [(sg * x) % qq for x, qq in zip(r, qs)], qs)), 15)
            counts['p23'] += 1
        # P24
        g = gcd(gcd(A, H), d)
        if g > 1:
            gi = [gcd(g, q) for q in qs]
            nq = [q // x for q, x in zip(qs, gi)]
            nres = []
            for x, q, gg, qq in zip(r, qs, gi, nq):
                w = pow(g // gg, -1, qq) if qq > 1 else 0
                nres.append(((x // gg) * w) % qq if qq > 1 else 0)
            ok &= need(all(gcd(a, b) == 1 for a, b in itertools.combinations(nq, 2)), 16)
            ok &= need(equal(local_set(d // g, nres, nq), X), 17)
            ok &= need(gcd(gcd(crt(nres, nq)[0], H // g), d // g) == 1, 18)
            counts['p24'] += 1
    # P25 example: no x mod 4 with 2x = 1 mod 4, and (1 + 4 Zhat)/2 = 1/2 + 2 Zhat
    ok &= need(not any((2 * x) % 4 == 1 for x in range(4)), 19)
    ok &= need(equal(local_set(2, [1], [4]), (F(1, 2), F(2))), 20)
    detail = ", ".join(f"{k}={v}" for k, v in counts.items())
    if bad:
        detail += f"; failing assertions {sorted(bad)}"
    report("check_local_ops (P19-P25)", ok, detail)


def check_backend_repairs():
    """P24 and P25 as repaired after review, against brute force; includes the reviewer's witnesses."""
    ok = True
    n24 = n251 = n252 = 0
    # P24: the canonical set is still a local value of the original context (search all (d', A'))
    witnesses = [([4], 2, [2])]
    for _ in range(400):
        qs = rand_blocks()
        H = reduce(lambda a, b: a * b, qs)
        if H > 40:
            continue
        d = random.randint(1, 12)
        # residues divisible by gcd(q_i, d), so that cancellation occurs often
        witnesses.append((qs, d, [(random.randrange(q) * gcd(q, d)) % q for q in qs]))
    for qs, d, r in witnesses:
        H = reduce(lambda a, b: a * b, qs)
        A, _ = crt(r, qs)
        g = gcd(gcd(A, H), d)
        if g == 1:
            continue
        canon = (F(A // g, d // g), F(H // g, d // g))
        ok &= 0 <= A // g < H // g
        brute = any(equal(canon, (F(A2, d2), F(H, d2))) for d2 in range(1, 2 * d + 1) for A2 in range(H))
        ok &= brute == p24_canonical_set_in_old_context(A, H, d, qs)
        n24 += 1
    # P25.1: d x = A modulo q, all small cases
    for q in range(1, 25):
        for d in range(1, 25):
            for A in range(q):
                sols = [x for x in range(q) if (d * x - A) % q == 0]
                ok &= len(sols) == p25_centre_solution_count(A, d, q)
                n251 += 1
    ok &= p25_centre_solution_count(1, 2, 4) == 0 and p25_centre_solution_count(2, 2, 4) == 2
    # P25.2: does the ball determine one residue modulo the block?
    for _ in range(600):
        qs = rand_blocks()
        d = random.randint(1, 12)
        r = [random.randrange(q) for q in qs]
        c, R = local_set(d, r, qs)
        for q in qs:
            if q == 1:
                continue
            pts = [c + R * k for k in range(0, 2 * q * d)]
            integral = all(gcd(x.denominator, q) == 1 for x in pts)
            single = integral and len({(x.numerator * pow(x.denominator, -1, q)) % q for x in pts}) == 1
            ok &= single == p25_ball_has_single_residue(d, q)
            n252 += 1
    # Summary 26: canonical numerator modulus of sum, tight product and exact scalar, against an independent
    # canonicalisation of the set (least denominator d with a ball (A + d R Zhat)/d equal to the set)
    def canon_H(X, dmax):
        c, R = X
        for dd in range(1, dmax + 1):
            if is_int(dd * R) and is_int(dd * c):
                Y = (F(int(dd * c) % int(dd * R), dd), R)
                if equal(X, Y):
                    return int(dd * R)
        return None
    n26 = 0
    for _ in range(300):
        qs = rand_blocks()
        H = reduce(lambda a, b: a * b, qs)
        if H > 60:
            continue
        d, e = random.randint(1, 12), random.randint(1, 12)
        r = [(random.randrange(q) * gcd(q, d)) % q for q in qs]
        t = [(random.randrange(q) * gcd(q, e)) % q for q in qs]
        A, _ = crt(r, qs)
        B, _ = crt(t, qs)
        X, Y = local_set(d, r, qs), local_set(e, t, qs)
        L = lcm(d, e)
        ok &= canon_H(T_add(X, Y), L) == H // gcd(gcd(A * L // d + B * L // e, H), L)
        h = gcd(gcd(A, B), H)
        ok &= canon_H(sampled_hull(X, Y, lambda u, v: u * v), d * e) == H * h // gcd(gcd(A * B, H * h), d * e)
        m, nn = random.choice([(1, 2), (-2, 3), (3, 1), (-4, 5), (6, 1)])
        ok &= canon_H((F(m, nn) * X[0], abs(F(m, nn)) * X[1]), nn * d) == \
            abs(m) * H // gcd(gcd(m * A, abs(m) * H), nn * d)
        n26 += 1
    # the review's two examples: canonical H changes although the raw H does not
    ok &= T_add((F(1, 2), F(1)), (F(1, 2), F(1))) == (F(1), F(1)) and canon_H((F(1), F(1)), 2) == 1
    P = sampled_hull((F(1, 2), F(3)), (F(2, 3), F(2)), lambda u, v: u * v)
    ok &= equal(P, (F(1, 3), F(1))) and canon_H(P, 6) == 3
    # the reviewer's lifts: A = 2 and A = 6 of the residue 2 modulo 4, d = 2, give centres 1 and 3 modulo 4
    ok &= equal(local_set(2, [2], [4]), (F(1), F(2))) and contains((F(3), F(0)), (F(1), F(2)))
    report("check_backend_repairs (P24, P25)", ok,
           f"{n24} cancellations: canonical set still in the old context; {n251} congruences d x = A mod q; "
           f"{n252} (ball, block) residue cases; {n26} canonical moduli (Summary 26)")


def canonical_triple_old(A, H, d):
    """Summary 26 as first written (before gate review G16): cancellation alone, no reduction of A."""
    if H == 0:
        g = gcd(abs(A), d)
        return (A // g, 0, d // g)
    g = gcd(gcd(abs(A), H), d)
    return (A // g, H // g, d // g)


def canonical_triple_new(A, H, d):
    """Summary 26 after gate review G16: reduce A into [0, H) first, then cancel by the gcd."""
    if H == 0:
        g = gcd(abs(A), d)
        return (A // g, 0, d // g)
    R = A % H
    g = gcd(gcd(R, H), d)
    return (R // g, H // g, d // g)


def is_canonical_triple(A, H, d):
    if H == 0:
        return gcd(abs(A), d) == 1
    return 0 <= A < H and gcd(gcd(A, H), d) == 1


def check_s26_centre():
    """G16: the canonical-column proof must reduce the raw numerator modulo H before cancelling.

    The old rule leaves the square of 3 + 4 Zhat as the raw triple (9, 4, 1), which violates 0 <= A < H.
    The corrected rule gives (1, 4, 1). The corrected rule is also compared with the set of the raw triple.
    """
    old = canonical_triple_old(9, 4, 1)
    new = canonical_triple_new(9, 4, 1)
    ok = (old == (9, 4, 1)) and not is_canonical_triple(*old)
    ok &= (new == (1, 4, 1)) and is_canonical_triple(*new)
    # the witness really is the raw tight product (AB, H h, d e) of 3 + 4 Zhat with itself, h = gcd(3, 3, 4) = 1
    ok &= gcd(gcd(3, 3), 4) == 1
    # corrected rule: canonical, idempotent, and the same set as the raw triple, on a deterministic grid
    n = 0
    for A in range(0, 20):
        for H in range(1, 9):
            for d in range(1, 7):
                A2, H2, d2 = canonical_triple_new(A, H, d)
                ok &= is_canonical_triple(A2, H2, d2)
                ok &= canonical_triple_new(A2, H2, d2) == (A2, H2, d2)
                ok &= equal((F(A, d), F(H, d)), (F(A2, d2), F(H2, d2)))
                n += 1
    report("check_s26_centre (S26, G16)", ok,
           f"old rule leaves (9,4,1) for the square of 3 + 4 Zhat; corrected rule gives (1,4,1); "
           f"{n} raw triples agree with the corrected canonical triple")


if __name__ == "__main__":
    check_hull()
    check_monotone()
    check_expressions()
    check_scaled_basics()
    check_best_scaled()
    check_scalar_add()
    check_scalar_mul()
    check_scaled_product()
    check_conversion()
    check_cap()
    check_local_repr()
    check_local_ops()
    check_backend_repairs()
    check_s26_centre()
    if FAILURES:
        print("FAILED:", ", ".join(FAILURES))
        sys.exit(1)
    print("all checks passed")
