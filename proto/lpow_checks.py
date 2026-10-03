#!/usr/bin/env python3
"""Exact finite-ring oracle of slice 1F.6 (lane f-slice9): rational powers and principal-unit powers at a prime.

Integers and fractions only: no logarithm, no exponential, no p-adic library, no C code. Every claim of the
C header that a fixture row encodes is checked here by exhaustive enumeration modulo a stated power of p:

 rational powers (statement P2 of docs/api-1f6.md, image of t -> t^(e/n) on one branch):
   a unit ball U + p^r Z_p; the roots beta of its points, enumerated modulo p^h, h = r - s + 1 + v(e'), as the
   bucket {beta mod p^h : beta^n' = U mod p^r} (exact by P7(a)); the branch is beta mod p (mod 4 at 2); the image
   {beta^e' mod p^(Erel+1)} is compared with the ball gamma + p^Erel Z_p, Erel = r - s + v(e'), as a SET modulo
   p^(Erel+1): this proves the exponent (a witness at distance exactly p^Erel) and the centre modulo p^Erel.
   OUTPUT PRECISION: the unit centre modulo p^Erel, the exponent Erel (relative; j = 0 rows), checked modulo
   p^(Erel+1).
 exact rational powers: the root of an exact unit U on a branch, by enumeration modulo p^h (unique class, P7(a)),
   raised to e'; rational roots by integer n'-th roots of numerator and denominator. OUTPUT PRECISION: N = 5 (p = 2,
   3) or 4 (p = 5, 7), absolute.
 principal-unit powers (Proposition 18 and P5): the set {u^s mod p^H}, u over the base ball modulo p^H, s over
   the integers s0 + p^B t, 0 <= t < p^H', H' = max(0, k - B), k = max(H - 1, 1) (P7(b): u^s modulo p^H depends
   on s modulo p^k only, and integer exponents are p-adic powers, Proposition 17), compared with
   exp(s0 ell) + p^R Z_p as a SET modulo p^H, H = R + 1 (a witness at distance exactly p^R). alpha = v(log u0')
   is taken as v(u0' - 1) (Lemma 9 item 5: log is an isometry on 1 + p^c Z_p), never by a logarithm. The 2-adic
   cases A = 1 or B = 0 with an unfixed sign or parity: the set is checked to contain residues 1 and 3 modulo 4
   (hull 1 + 2 Z_2, exponent 1) and compared with all odd residues modulo 2^4 (the image is or is not the hull).
   OUTPUT PRECISION: the centre modulo p^R and the exponent R, checked modulo p^(R+1); exact inputs: N = 5 or 4.

Run: python3 -B proto/lpow_checks.py  (writes tests/ref/vectors/f-slice9/*.jsonl and prints the counts).
"""
import json
import sys
from fractions import Fraction
from math import gcd
from pathlib import Path

OUT = Path('tests/ref/vectors/f-slice9')
INF = None


def vp(x, p):
    if x == 0:
        return 10**9
    k = 0
    while x % p == 0:
        x //= p
        k += 1
    return k


def vq(q, p):
    q = Fraction(q)
    if q == 0:
        return 10**9
    return vp(q.numerator, p) - vp(q.denominator, p)


def cfor(p):
    return 2 if p == 2 else 1


def tmod(p):
    return 4 if p == 2 else p


def zmod(q, p, k):
    """the p-integral rational q modulo p^k, an integer in [0, p^k)"""
    q = Fraction(q)
    P = p**k
    return (q.numerator % P) * pow(q.denominator, -1, P) % P


def iroot(a, n):
    if a < 0:
        return None
    if a in (0, 1):
        return a
    if n >= a.bit_length():
        return None
    lo, hi = 1, 1 << (a.bit_length() // n + 1)
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if mid**n <= a:
            lo = mid
        else:
            hi = mid - 1
    return lo if lo**n == a else None


def reduce_frac(e, n):
    g = gcd(abs(e), n)
    return e // g, n // g


def seeds_of(p):
    return range(0, 5) if p == 2 else range(0, p + 1)


# ---------------------------------------------------------------- rational powers, unit balls

FRACS = [(1, 2), (3, 2), (-1, 2), (-3, 2), (5, 2), (1, 3), (2, 3), (-2, 3), (4, 3), (1, 4), (3, 4), (-1, 4),
         (2, 4), (4, 6), (6, 4), (-2, 6), (1, 6), (5, 6), (2, 2), (-4, 2), (0, 3), (3, 1)]
RMAX = {2: 6, 3: 4, 5: 3, 7: 2}


def powrat_balls():
    rows = []
    witnesses = 0
    for p in (2, 3, 5, 7):
        c = cfor(p)
        cache = {}
        for (e, n) in FRACS:
            e1, n1 = reduce_frac(e, n)
            for r in range(1, RMAX[p] + 1):
                s = vp(n1, p)
                ve = vp(e1, p) if e1 else 0
                guarded = n1 >= 2 and r >= c + s
                h = r - s + 1 + ve
                Erel = r - s + ve
                buckets = None
                if guarded:
                    key = (n1, r, h)
                    if key not in cache:
                        b = {}
                        for beta in range(p**h):
                            if beta % p == 0:
                                continue
                            b.setdefault(pow(beta, n1, p**r), []).append(beta)
                        cache[key] = b
                    buckets = cache[key]
                for U in range(1, p**r):
                    if U % p == 0:
                        continue
                    ok = []
                    if guarded:
                        for seed in seeds_of(p):
                            # an invalid seed or a branch with no root is DOMAIN: it is simply not listed
                            if seed == 0 or (p == 2 and seed not in (1, 3)) or (p != 2 and seed >= p):
                                continue
                            roots = [beta for beta in buckets.get(U, []) if beta % tmod(p) == seed]
                            if not roots:
                                continue
                            img = {pow(beta, e1, p**(Erel + 1)) for beta in roots}
                            gamma = min(img) % p**Erel
                            want = {(gamma + p**Erel * t) % p**(Erel + 1) for t in range(p)}
                            assert img == want, (p, e, n, U, r, seed, sorted(img), sorted(want))
                            witnesses += 1
                            ok.append([seed, gamma])
                    # E null: n' = 1 (compare with pow_si) or outside the guard (NOT_DETERMINED for every seed);
                    # otherwise the listed seeds are OK with the unit centre gamma, every other seed is DOMAIN
                    rows.append({"p": p, "e": e, "n": n, "U": U, "r": r,
                                 "E": Erel if guarded else -1, "ok": ok})
    return rows, witnesses


# ---------------------------------------------------------------- rational powers, exact inputs

EXACT_UNITS = [Fraction(1), Fraction(-1), Fraction(4), Fraction(9), Fraction(-8), Fraction(16), Fraction(1, 4),
               Fraction(4, 9), Fraction(-27, 8), Fraction(81), Fraction(2), Fraction(3), Fraction(10),
               Fraction(17), Fraction(2, 3), Fraction(-7), Fraction(64), Fraction(25, 49), Fraction(11)]


def root_branch(p, n1, U, seed, h):
    """the unit roots beta of the exact unit U with beta = seed (mod p, mod 4 at 2), modulo p^h, h >= c:
    {beta mod p^h : beta^n1 = U mod p^(h + s)} (P7(a): one class per branch)"""
    s = vp(n1, p)
    Um = zmod(U, p, h + s)
    out = [b for b in range(p**h) if b % p and b % tmod(p) == seed and pow(b, n1, p**(h + s)) == Um]
    return out


def powrat_exact():
    rows = []
    for p in (2, 3, 5, 7):
        c = cfor(p)
        N = 5 if p <= 3 else 4
        for U in EXACT_UNITS:
            if vq(U, p) != 0:
                continue
            for (e, n) in FRACS:
                e1, n1 = reduce_frac(e, n)
                if n1 == 1:
                    continue
                s = vp(n1, p)
                ve = vp(e1, p)
                for j in (0, 1):
                    m = n1 * j
                    ok = []
                    for seed in seeds_of(p):
                        valid = (p == 2 and seed in (1, 3)) or (p != 2 and 1 <= seed < p)
                        # existence: a class modulo p^c in branch seed (R1: Log modulo p^(c+s) <=> beta exists)
                        if not (valid and root_branch(p, n1, U, seed, c) != []):
                            continue                            # not listed: DOMAIN
                        # rational root: +-iroot(|A|)/iroot(B)
                        rat = None
                        A, B = U.numerator, U.denominator
                        if not (A < 0 and n1 % 2 == 0):
                            ra, rb = iroot(abs(A), n1), iroot(B, n1)
                            if ra is not None and rb is not None:
                                q = Fraction(ra if A > 0 else -ra, rb)
                                for cand in ([q, -q] if n1 % 2 == 0 else [q]):
                                    if zmod(cand, p, c) % tmod(p) == seed:
                                        rat = cand
                        ej = e1 * j
                        if rat is not None:
                            val = rat**e1
                            assert (val**n1) == Fraction(U)**e1
                            ok.append([seed, 1, ej, val.numerator, val.denominator])
                        elif N <= ej:
                            ok.append([seed, 0, 0, 0, 1])       # the ball p^N Z_p around 0
                        else:
                            k = N - ej                          # relative digits of the result
                            h = max(k - ve, c)
                            roots = root_branch(p, n1, U, seed, h)
                            assert len(roots) == 1, (p, e, n, U, seed, roots)
                            ok.append([seed, 0, ej, pow(roots[0], e1, p**k), 1])
                    rows.append({"p": p, "e": e, "n": n, "num": U.numerator, "den": U.denominator, "m": m,
                                 "N": N, "ok": ok})
                # m = 1 is not divisible by n' >= 2: DOMAIN for every seed (an empty list)
                rows.append({"p": p, "e": e, "n": n, "num": U.numerator, "den": U.denominator, "m": 1,
                             "N": N, "ok": []})
    return rows


# ---------------------------------------------------------------- principal-unit powers

def princ_alpha(p, u0, cap):
    """alpha = v(log(u0')) capped at cap, u0' = +-u0 in 1 + p^c Z_p, as v(u0' - 1) (isometry, Lemma 9 item 5)"""
    up = u0 if (p != 2 or zmod(u0, 2, 2) == 1) else -u0
    return min(vq(Fraction(up) - 1, p), cap)


def unit_set(p, H, kind, u0, A):
    if kind == 'x':
        return [zmod(u0, p, H)]
    base = int(u0)
    return [(base + p**A * t) % p**H for t in range(p**max(H - A, 0))]


def exp_set(p, H, kind, s0, B):
    k = max(H - 1, 1)
    if kind == 'x':
        return [zmod(s0, p, k)]
    base = int(s0)
    Hp = max(0, k - B)
    return [base + p**B * t for t in range(p**Hp)]


def powunit_value_set(p, H, uk, u0, A, sk, s0, B):
    us = unit_set(p, H, uk, u0, A)
    ss = exp_set(p, H, sk, s0, B)
    P = p**H
    return {pow(u, s, P) for u in us for s in ss}


def powunit_case(p, uk, u0, A, sk, s0, B, Nexact):
    """expected (status, R, centre, exact) for one row; every value is checked by enumeration"""
    c = cfor(p)
    u0 = Fraction(u0)
    s0 = Fraction(s0)
    # exact 1 cases
    if sk == 'x' and s0 == 0:
        return {"st": 0, "exact": 1, "rnum": 1, "rden": 1}
    if uk == 'x' and u0 == 1:
        return {"st": 0, "exact": 1, "rnum": 1, "rden": 1}
    par_fixed = sk == 'x' or B >= 1
    par = (vq(s0, p) == 0) if s0 != 0 else False
    if p == 2:
        sign_fixed = uk == 'x' or A >= 2
        w0 = (1 if zmod(u0, 2, 2) == 1 else -1) if sign_fixed else None
    else:
        sign_fixed, w0 = True, 1
    if uk == 'x' and u0 == -1 and par_fixed:
        val = -1 if par else 1
        S = powunit_value_set(p, 4, uk, u0, A, sk, s0, B)
        assert S == {val % 16}
        return {"st": 0, "exact": 1, "rnum": val, "rden": 1}
    hull = False
    if p == 2 and not sign_fixed:
        if par_fixed and not par:
            beta = vq(s0, p) if s0 != 0 else 10**9
            R = 2 + min(beta, B if sk == 'b' else 10**9)
        else:
            hull = True
    elif p == 2 and w0 == -1 and not par_fixed:
        hull = True
    if hull:
        S4 = powunit_value_set(p, 4, uk, u0, A, sk, s0, B)
        assert all(x % 2 == 1 for x in S4) and {x % 4 for x in S4} == {1, 3}
        full = S4 == set(range(1, 16, 2))
        # P5: the image is all of 1 + 2 Z_2 exactly when the sign is not fixed (A = 1); with w = -1 fixed and
        # B = 0 a residue class modulo 16 is missed (checked here), so the hull is strictly larger than the image
        assert full == (not sign_fixed), (p, uk, u0, A, sk, s0, B)
        return {"st": 0, "exact": 0, "R": 1, "c": 1, "hull": 1, "full": int(full)}
    if uk == 'x' and sk == 'x':
        N = Nexact
        S = powunit_value_set(p, N, uk, u0, A, sk, s0, B)
        assert len(S) == 1
        return {"st": 0, "exact": 0, "R": -1, "N": N, "c": S.pop()}
    if not (p == 2 and not sign_fixed):
        beta = vq(s0, p) if s0 != 0 else 10**9
        Ai = A if uk == 'b' else 10**9
        Bi = B if sk == 'b' else 10**9
        alpha = princ_alpha(p, u0, Ai + 5)
        R = min(Ai + beta, Bi + alpha, Ai + Bi)
    assert R < 10**8
    H = R + 1
    S = powunit_value_set(p, H, uk, u0, A, sk, s0, B)
    cen = min(S) % p**R
    want = {(cen + p**R * t) % p**H for t in range(p)}
    assert S == want, (p, uk, u0, A, sk, s0, B, R, sorted(S)[:10])
    return {"st": 0, "exact": 0, "R": R, "c": cen}


EXACT_U = {2: [1, -1, 3, 5, 7, -3, 9, 17, Fraction(1, 3), Fraction(-5, 3), 33, 65],
           3: [1, 4, 7, -2, 10, 28, Fraction(1, 4), Fraction(7, 4), 82],
           5: [1, 6, 11, -4, 26, 126, Fraction(1, 6), 51],
           7: [1, 8, 15, -6, 50, 344, Fraction(1, 8)]}
EXACT_S = {2: [0, 1, 2, 3, -1, 4, 8, 6, Fraction(1, 3), Fraction(-2, 5), Fraction(4, 3)],
           3: [0, 1, 2, -1, 3, 9, 27, Fraction(1, 2), Fraction(-3, 5), Fraction(9, 2)],
           5: [0, 1, 2, -1, 5, 25, 125, Fraction(1, 2), Fraction(5, 3)],
           7: [0, 1, 3, -1, 7, 49, 343, Fraction(1, 2), Fraction(7, 2)]}
AB = {2: (4, 3), 3: (3, 3), 5: (3, 2), 7: (2, 2)}


def powunit_rows():
    rows = []
    witnesses = hulls = 0
    for p in (2, 3, 5, 7):
        c = cfor(p)
        Amax, Bmax = AB[p]
        Nexact = 5 if p <= 3 else 4
        bases = []
        for A in range(1, Amax + 1):
            for u0 in range(p**A):
                if (p == 2 and u0 % 2 == 1) or (p != 2 and u0 % p == 1):
                    bases.append(('b', u0, A))
        for u in EXACT_U[p]:
            bases.append(('x', Fraction(u), None))
        exps = []
        for B in range(0, Bmax + 1):
            for s0 in range(p**B):
                exps.append(('b', s0, B))
        for s in EXACT_S[p]:
            exps.append(('x', Fraction(s), None))
        for (uk, u0, A) in bases:
            for (sk, s0, B) in exps:
                out = powunit_case(p, uk, u0, A, sk, s0, B, Nexact)
                # uniform fields: A = -1 (B = -1) marks an exact base (exponent); R = -1 an exact-input ball at N
                row = {"p": p, "un": int(u0) if uk == 'b' else u0.numerator, "ud": 1 if uk == 'b' else u0.denominator,
                       "A": A if uk == 'b' else -1,
                       "sn": int(s0) if sk == 'b' else s0.numerator, "sd": 1 if sk == 'b' else s0.denominator,
                       "B": B if sk == 'b' else -1}
                row.update(out)
                if out.get("hull"):
                    hulls += 1
                elif out.get("R", -1) != -1:
                    witnesses += 1
                rows.append(row)
    return rows, witnesses, hulls


def hensel_unit(p, n, target, seed, K):
    """Own digit proof: if y^n=T mod p^k, y+d*p^k corrects the next digit by
    n*y^(n-1)*d = (T-y^n)/p^k mod p. The derivative is a unit in these rows.
    The final equation and seed are checked separately, using exact integer powers.
    """
    assert n % p and seed % p
    y = seed % p
    assert (pow(y, n, p) - target) % p == 0
    for k in range(1, K):
        modulus = p**(k + 1)
        error = (target - pow(y, n, modulus)) % modulus
        assert error % p**k == 0
        derivative = n * pow(y, n - 1, p) % p
        digit = error // p**k * pow(derivative, -1, p) % p
        y += digit * p**k
    assert pow(y, n, p**K) == target % p**K
    assert y % p == seed % p
    return y


def repair6_rows():
    """F6/F7: integer certificates; no C library, log or exp.

    powunit: H=R+1, exhaustive sets at 3,5,7; two certified image points at the
    exact distance p^R at 65537 and 2^64-59. Even p^2 residues at 65537 are too
    many to enumerate. The witnesses certify tightness, not finite-set equality.
    powrat: M=K=2,3,4, j=0, n prime to p, gcd(e,n)=1. Lift the powered value
    directly from seed^e by t^n=x^e mod p^K, rather than use a library root.
    """
    rows_u, rows_r = [], []
    for p in (3, 5, 7, 65537, 2**64 - 59):
        # A, B, base centre, exponent centre, unique minimum, witness input pairs.
        params = [(2, 2, 1+p, 1, 0, (1+p, 1), (1+p+p**2, 1)),
                  (3, 4, 1+p, p, 0, (1+p, p), (1+p+p**3, p)),
                  (3, 0, 1+p, 0, 1, (1+p, 0), (1+p, 1)),
                  (3, 1, 1+p, 0, 1, (1+p, 0), (1+p, p)),
                  (1, 0, 1, 0, 2, (1, 0), (1+p, 1)),
                  (2, 1, 1, 0, 2, (1, 0), (1+p**2, p))]
        for A, B, u, s, term, w1, w2 in params:
            terms = [A+vp(s, p), B+vp(u-1, p), A+B]
            R = terms[term]
            assert all(R < t for i, t in enumerate(terms) if i != term)
            H = R+1
            modulus = p**H
            outputs = []
            for wu, ws in (w1, w2):
                assert (wu-u) % p**A == 0 and (ws-s) % p**B == 0
                outputs.append(pow(wu, ws, modulus))
            assert vp((outputs[1]-outputs[0]) % modulus, p) == R
            centre = pow(u, s, p**R)
            assert all((w-centre) % p**R == 0 for w in outputs)
            assert any((w-centre) % p**(R+1) != 0 for w in outputs)
            if p < 100:
                actual = powunit_value_set(p, H, 'b', u, A, 'b', s, B)
                assert actual == {centre+p**R*t for t in range(p)}
            sv = vp(s, p) if s else 0
            rows_u.append(dict(p=p, A=A, B=B, u=u, su=s//p**sv, sv=sv,
                               R=R, H=H, term=term, c=centre, enumerated=int(p < 100),
                               u1=w1[0], s1=w1[1], y1=outputs[0],
                               u2=w2[0], s2=w2[1], y2=outputs[1]))
    for p in (65537, 2**64 - 59):
        for n, e in ((2, 1), (2, 3), (3, 2), (3, -2), (4, -1), (5, 2)):
            for K in (2, 3, 4):
                for seed, extra in ((1, 0), (2, 1), (42, 2), (p-2, 3)):
                    x = (seed**n + extra*p) % p**K
                    target = pow(x, e, p**K)
                    powered_seed = pow(seed, e, p)
                    centre = hensel_unit(p, n, target, powered_seed, K)
                    rows_r.append(dict(p=p, n=n, e=e, seed=seed, x=x, M=K,
                                       K=K, c=centre, target=target))
    out = Path('tests/ref/vectors/f-repair6')
    out.mkdir(parents=True, exist_ok=True)
    for name, rows in (('powunit.jsonl', rows_u), ('powrat.jsonl', rows_r)):
        (out/name).write_text(''.join(json.dumps(r, separators=(',', ':'))+'\n' for r in rows))
    print(f'repair6 powunit: {len(rows_u)} rows, H=R+1=2..5; '
          f'{sum(r["enumerated"] for r in rows_u)} exhaustive sets, '
          f'{sum(not r["enumerated"] for r in rows_u)} witness pairs')
    print(f'repair6 powrat: {len(rows_r)} rows, centre/equation precision K=2,3,4')


def main():
    if sys.argv[1:] == ['--repair6']:
        repair6_rows()
        return 0
    OUT.mkdir(parents=True, exist_ok=True)
    rb, wb = powrat_balls()
    re_ = powrat_exact()
    ru, wu, hu = powunit_rows()
    for name, rows in (("powrat_balls.jsonl", rb), ("powrat_exact.jsonl", re_), ("powunit.jsonl", ru)):
        with open(OUT / name, "w") as f:
            for r in rows:
                f.write(json.dumps(r, separators=(",", ":")) + "\n")
    print("powrat_balls rows %d (image witnesses %d)" % (len(rb), wb))
    print("powrat_exact rows %d" % len(re_))
    print("powunit rows %d (image witnesses %d, 2-adic hulls %d)" % (len(ru), wu, hu))
    return 0


if __name__ == "__main__":
    sys.exit(main())
