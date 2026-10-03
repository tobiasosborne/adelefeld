#!/usr/bin/env python3
"""Exact-integer oracle for the proposed idele Log API. No p-adic or floating library.

Proofs and precision bounds: docs/design/idele-log.md IL1-IL8.
r is an integer or a pair (numerator, denominator); no floating input is accepted.
expected_ball(p, r, c, M, K) returns the local enclosure at requested exponent K.
M is the unit modulus (the brief calls it N). M=0 means the exact unit c=+1/-1.
Use --fixtures PATH for JSONL fixtures; use --expect p num den c M K for one result.
"""

import argparse
import json
from functools import lru_cache
from math import gcd


def valuation(n, p):
    """Valuation of a nonzero integer; zero has no finite valuation here."""
    if n == 0:
        raise ValueError("valuation of zero")
    n = abs(n)
    e = 0
    while n % p == 0:
        n //= p
        e += 1
    return e


def rational(r):
    if isinstance(r, int):
        a, b = r, 1
    elif isinstance(r, (tuple, list)) and len(r) == 2 and all(isinstance(t, int) for t in r):
        a, b = r
    else:
        raise ValueError("r must be an integer or an integer pair")
    if a <= 0 or b <= 0:
        raise ValueError("content must be positive")
    g = gcd(a, b)
    return a // g, b // g


def unit_part(r, p):
    a, b = rational(r)
    ea, eb = valuation(a, p), valuation(b, p)
    return ea - eb, a // p**ea, b // p**eb


def validate(p, r, c, M):
    if not isinstance(p, int) or p < 2:
        raise ValueError("p must be prime")
    t = 2
    while t * t <= p:
        if p % t == 0:
            raise ValueError("p must be prime")
        t += 1
    rational(r)
    if M < 0 or (M == 0 and c not in (-1, 1)) or (M > 0 and gcd(c, M) != 1):
        raise ValueError("invalid unit pair")


def integer_log_floor(n, p):
    e = 0
    while p ** (e + 1) <= n:
        e += 1
    return e


def log_principal(a, p, H):
    """log(a) modulo p^H, a=1 mod p^d. IL7 proves the tail and term precision."""
    if H <= 0:
        return 0
    d = 2 if p == 2 else 1
    if (a - 1) % p**d:
        raise ValueError("not a principal unit")
    T = 2 * H
    D = integer_log_floor(max(1, T - 1), p)
    W = H + D
    mod, work = p**H, p**W
    z = (a - 1) % work
    power = 1
    total = 0
    for j in range(1, T):
        power = power * z % work
        e = valuation(j, p)
        pe = p**e
        assert power % pe == 0
        term = (power // pe) * pow(j // pe, -1, mod) % mod
        total += term if j % 2 else -term
    return total % mod


@lru_cache(maxsize=None)
def log_unit(a, p, H):
    """Iwasawa Log of a unit known modulo p^H; its integer representative suffices (IL6)."""
    if H <= (2 if p == 2 else 1):
        return 0
    mod = p**H
    a %= mod
    if a % p == 0:
        raise ValueError("not a unit")
    if p == 2:
        return log_principal(a if a % 4 == 1 else -a, p, H)
    T = 2 * H
    W = H + integer_log_floor(T - 1, p)
    principal = pow(a, p - 1, p**W)
    return log_principal(principal, p, H) * pow(p - 1, -1, mod) % mod


def log_rational(p, r, c, H):
    _, a, b = unit_part(r, p)
    if H <= 0:
        return 0
    mod = p**H
    if c % p == 0:
        raise ValueError("c must be a local unit")
    u = (a * c * pow(b, -1, mod)) % mod
    return log_unit(u, p, H)


def image_exponent(p, M):
    if M == 0:
        return None
    k = valuation(M, p)
    if p == 2:
        return max(2, k)
    return max(1, k)


def expected_ball(p, r, c, M, K):
    """Expected local _at result. Keys: p, exact, center, exponent, image_exponent.

    exponent=None denotes exact zero. Otherwise center is the canonical integral residue
    in [0,p^exponent) for positive exponent, zero for exponent<=0. No real status is predicted.
    Resource limits of a future C implementation are outside this mathematical oracle.
    """
    validate(p, r, c, M)
    if not isinstance(K, int):
        raise ValueError("requested exponent must be an integer")
    _, a, b = unit_part(r, p)
    E = image_exponent(p, M)
    if M == 0 and a == b:
        return dict(p=p, exact=True, center=0, exponent=None, image_exponent=None)
    L = K if E is None else min(K, E)
    k = None if M == 0 else valuation(M, p)
    # c need not be prime to p at an unrestricted prime. The whole image is centred at zero.
    center = 0 if M > 0 and (k == 0 or (p == 2 and k == 1)) else log_rational(p, r, c, L)
    return dict(p=p, exact=False, center=center, exponent=L, image_exponent=E)


def expected_refinement(p, r, c, M, K):
    """Local condition used by _refine: exact zero is rounded, then intersected with 4 Zhat."""
    b = expected_ball(p, r, c, M, K)
    beta = 2 if p == 2 else 0
    L = max(beta, K if b['exact'] else b['exponent'])
    center = 0 if b['exact'] or (b['exponent'] <= beta) else b['center']
    return center, L


def ball_residues(b, p, H):
    if b['exact']:
        return {0}
    E, center = b['exponent'], b['center']
    if E <= 0:
        return set(range(p**H))
    if E >= H:
        return {center % p**H}
    return set(range(center % p**E, p**H, p**E))


def enumerated_image(p, r, c, M, H):
    """Every local unit modulo p^H satisfying c mod p^k; IL6 proves H'=H suffices."""
    mod = p**H
    _, a, b = unit_part(r, p)
    ru = a * pow(b, -1, mod) % mod
    if M == 0:
        return {log_unit(ru * c % mod, p, H)}, 1
    k = valuation(M, p)
    if k > H:
        raise ValueError("enumeration requires H >= k")
    units = range(1, mod) if k == 0 else range(c % p**k, mod, p**k)
    image = set()
    count = 0
    for u in units:
        if u % p:
            image.add(log_unit(ru * u % mod, p, H))
            count += 1
    return image, count


def contents(p):
    for m in range(-2, 3):
        for a, b in ((1, 1), (p + 1, 1), (1, p + 1), (2 * p + 1, p + 1)):
            yield rational((a * p**max(m, 0), b * p**max(-m, 0)))


def grid():
    for p in (2, 3, 5, 7):
        for k in range(5):
            for cofactor in (1, 11, 143):
                M = p**k * cofactor
                cs = {1, M, M - 1, p, p + 1, 2 * p + 1}
                for c in sorted(cs):
                    if 1 <= c <= M and gcd(c, M) == 1:
                        for r in contents(p):
                            yield p, r, c, M


def check_images(H):
    cases = points = caps = 0
    by_prime = {}
    for p, r, c, M in grid():
        actual, count = enumerated_image(p, r, c, M, H)
        E = image_exponent(p, M)
        expected = ball_residues(expected_ball(p, r, c, M, H), p, H)
        assert actual == expected, (p, r, c, M, H, sorted(actual ^ expected)[:5])
        assert len(actual) == p**(H - E), (p, M, H, len(actual))
        for N in (-2, 0, 1, 2, 3, 4, H, H + 2):
            b = expected_ball(p, r, c, M, N)
            assert b['exponent'] == min(N, E)
            if N > 0:
                cap_mod = p**min(N, E)
                assert all((t - b['center']) % cap_mod == 0 for t in actual)
            if N >= E:
                # Same exponent and residue class as the already-enumerated full image.
                assert (b['center'] - expected_ball(p, r, c, M, H)['center']) % p**E == 0
            else:
                # The coarser class has this size by counting an arithmetic progression.
                assert p**(H - max(N, 0)) > len(actual)
            caps += 1
        cases += 1
        points += count
        old = by_prime.setdefault(p, [0, 0])
        old[0] += 1
        old[1] += count
    assert expected_ball(3, (4, 1), 1, 9, H)['center'] == 3
    assert log_rational(2, (3, 1), 1, 3) == 4
    return dict(cases=cases, enumerated_units=points, cap_checks=caps, by_prime=by_prime, H=H, Hprime=H)


def check_components(H):
    cases = points = 0
    for p, r, c, M in grid():
        _, a, b = unit_part(r, p)
        mod = p**H
        ru = a * pow(b, -1, mod) % mod
        k = valuation(M, p)
        candidates = range(1, mod) if k == 0 else range(c % p**k, mod, p**k)
        actual = {ru * u % mod for u in candidates if u % p}
        if k:
            expected = set(range(ru * c % p**k, mod, p**k))
        elif p == 2:
            expected = set(range(1, mod, 2))
        else:
            expected = {u for u in range(1, mod) if u % p}
            assert 0 not in expected and 1 in expected and 2 in expected
        assert actual == expected
        points += len(actual)
        cases += 1
    return dict(cases=cases, points=points, H=H, normalized_by='p^m')


def check_factor_two(H):
    pairs = 0
    for M in (2, 6, 10, 22, 286):
        for c in range(1, M + 1):
            if gcd(c, M) != 1:
                continue
            Mbar = M // 2
            cbar = (c - 1) % Mbar + 1
            for p in (2, 3, 5, 7):
                for r in ((1, 1), (4, 3), (3, 4)):
                    a, _ = enumerated_image(p, r, c, M, H)
                    b, _ = enumerated_image(p, r, cbar, Mbar, H)
                    assert a == b
                    assert expected_ball(p, r, c, M, H) == expected_ball(p, r, cbar, Mbar, H)
                    pairs += 1
    return dict(pairs=pairs, H=H)


def check_exact(H):
    cases = sign_pairs = kernel = 0
    for p in (2, 3, 5, 7):
        for r in contents(p):
            for c in (-1, 1):
                actual, _ = enumerated_image(p, r, c, 0, H)
                b = expected_ball(p, r, c, 0, H)
                assert actual == ball_residues(b, p, H)
                _, a, den = unit_part(r, p)
                assert b['exact'] == (a == den)
                for N in (-2, 0, 1, 2, H, H + 2):
                    bn = expected_ball(p, r, c, 0, N)
                    if not bn['exact']:
                        assert bn['exponent'] == N
                    if N <= H:
                        assert actual <= ball_residues(bn, p, H)
                cases += 1
                kernel += int(b['exact'])
            assert log_rational(p, r, -1, H) == log_rational(p, r, 1, H)
            sign_pairs += 1
    return dict(cases=cases, sign_pairs=sign_pairs, exact_zero_cases=kernel, H=H)


def check_kernel(H):
    """Check quotient kernels and the independent torsion-removal routes of IL7."""
    units = square_checks = teich_checks = isometry_checks = classes = 0
    for p in (2, 3, 5, 7):
        mod = p**H
        logs = {}
        for a in range(1, mod):
            if a % p == 0:
                continue
            ell = log_unit(a, p, H)
            d = 2 if p == 2 else 1
            assert ell % p**d == 0
            logs[ell] = logs.get(ell, 0) + 1
            # H+1 input for the square route; division by 2 loses one digit.
            if p == 2:
                twice = log_principal(a * a, 2, H + 1)
                assert twice % 2 == 0 and twice // 2 % mod == ell
                square_checks += 1
            else:
                # Lift t^(p-1)=1 digit by digit, independently of the powered log route.
                t = a % p
                for j in range(1, H):
                    err = (pow(t, p - 1, p**(j + 1)) - 1) % p**(j + 1)
                    assert err % p**j == 0
                    derivative = (p - 1) * pow(t, p - 2, p) % p
                    digit = -(err // p**j) * pow(derivative, -1, p) % p
                    t += digit * p**j
                principal = a * pow(t, -1, mod) % mod
                assert principal % p == 1 and pow(t, p - 1, mod) == 1
                assert log_principal(principal, p, H) == ell
                teich_checks += 1
            # Each lift by p^H has the same logarithm modulo p^H (IL6).
            lifted = log_unit(a + mod, p, H + 1) % mod
            assert lifted == ell
            for j in range(d, H):
                delta = (log_unit((a + p**j) % mod, p, H) - ell) % mod
                assert delta and valuation(delta, p) == j
                isometry_checks += 1
            units += 1
        assert set(logs) == set(range(0, mod, p**d))
        multiplicity = 2 if p == 2 else p - 1
        assert set(logs.values()) == {multiplicity}
        classes += len(logs)
    return dict(units=units, output_classes=classes, square_checks=square_checks,
                teich_checks=teich_checks, isometry_checks=isometry_checks, H=H)


def crt(conditions):
    a, R = 0, 1
    for b, q in conditions:
        assert gcd(R, q) == 1
        t = (b - a) * pow(R, -1, q) % q
        a += R * t
        R *= q
    return a % R, R


def check_crt():
    cases = memberships = 0
    for r, c, M in (((4, 1), 1, 9), ((3, 4), 5, 36), ((7, 3), 1, 1), ((1, 1), -1, 0)):
        for S in ((), (2,), (3,), (2, 3), (3, 5), (2, 3, 5)):
            for N in (-2, 0, 1, 2, 3):
                conditions = {}
                for p in S:
                    b, L = expected_refinement(p, r, c, M, N)
                    if L:
                        conditions[p] = (b, p**L)
                conditions.setdefault(2, (0, 4))
                a, R = crt([conditions[p] for p in sorted(conditions)])
                assert a % 4 == 0
                for z in range(2 * R):
                    direct = z % 4 == 0
                    for p in S:
                        b = expected_ball(p, r, c, M, N)
                        L = N if b['exact'] else b['exponent']
                        direct = direct and (L <= 0 or (z - b['center']) % p**L == 0)
                    assert direct == ((z - a) % R == 0)
                    memberships += 1
                cases += 1
    return dict(cases=cases, memberships=memberships, N=[-2, 0, 1, 2, 3])


def check_faults():
    """Finite mathematical mutants only; no C status or real wrapper is tested."""
    witnesses = (
        (3, (3, 1), 1, 9, 5), (3, (1, 3), 1, 9, 5),
        (3, (4, 1), 1, 9, 5), (2, (1, 1), 1, 1, 5),
        (3, (1, 1), 1, 1, 5), (2, (1, 1), 1, 8, 1),
        (2, (1, 1), 1, 2, 5), (3, (3, 1), 1, 0, 5),
    )
    killed = []
    for i, (p, r, c, M, N) in enumerate(witnesses):
        good = expected_ball(p, r, c, M, N)
        bad = dict(good)
        if i in (0, 1):
            # Fault 1: use the absolute input exponent as the image exponent.
            # Fault 2: construct the input at k without m, then let Log subtract m.
            m = unit_part(r, p)[0]
            bad['exponent'] = valuation(M, p) + (m if i == 0 else -m)
        elif i == 2:
            bad['center'] = 0
        elif i == 3:
            bad['exponent'] = 1
        elif i == 4:
            bad['exponent'] = 0
        elif i == 5:
            bad['exponent'] = 3
        elif i == 6:
            bad['exponent'] = 1
        else:
            bad.update(exact=False, exponent=1)
        assert ball_residues(good, p, 5) != ball_residues(bad, p, 5), (i, good, bad)
        killed.append(i + 1)
    return dict(planted=8, rejected=len(killed), ids=killed, H=5)


def fixture_rows():
    for p, r, c, M in grid():
        for K in (0, 1, 2, 4, 6):
            yield dict(p=p, r=list(r), c=c, M=M, requested=K,
                       expected=expected_ball(p, r, c, M, K))
    for p in (2, 3, 5, 7):
        for r in contents(p):
            for c in (-1, 1):
                for K in (0, 2, 6):
                    yield dict(p=p, r=list(r), c=c, M=0, requested=K,
                               expected=expected_ball(p, r, c, 0, K))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--H', type=int, choices=(5, 6), default=5)
    parser.add_argument('--fixtures')
    parser.add_argument('--expect', type=int, nargs=6, metavar=('P', 'NUM', 'DEN', 'C', 'M', 'K'))
    parser.add_argument('--checks', nargs='+',
                        choices=('components', 'images', 'factor_two', 'exact', 'kernel', 'crt', 'faults'))
    args = parser.parse_args()
    if args.expect:
        p, a, b, c, M, K = args.expect
        print(json.dumps(expected_ball(p, (a, b), c, M, K), sort_keys=True))
        return
    for name, fn in (('components', lambda: check_components(args.H)),
                     ('images', lambda: check_images(args.H)),
                     ('factor_two', lambda: check_factor_two(args.H)),
                     ('exact', lambda: check_exact(args.H)),
                     ('kernel', lambda: check_kernel(args.H)),
                     ('crt', check_crt), ('faults', check_faults)):
        if args.checks is not None and name not in args.checks:
            continue
        print(name + ' ' + json.dumps(fn(), sort_keys=True), flush=True)
    if args.fixtures:
        count = 0
        with open(args.fixtures, 'w', encoding='ascii') as f:
            for row in fixture_rows():
                f.write(json.dumps(row, sort_keys=True) + '\n')
                count += 1
        print('fixtures ' + json.dumps(dict(rows=count, path=args.fixtures), sort_keys=True))


if __name__ == '__main__':
    main()
