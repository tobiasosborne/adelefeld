#!/usr/bin/env python3
"""Adversarial checks for docs/proofs/functions.md (review, milestone 0).

Written independently of proto/functions_checks.py: no function of that file is used to decide a
mathematical question. Every p-adic value is computed here from exact integer powers with exact division
by the p-part of the denominator. The Teichmueller factor is computed as the limit x^(p^K), not by the
author's digit lifting. The author's file is loaded only in section M, as source text, to run the author's
own checks against deliberately wrong formulas (mutants) and see whether they notice.

Run:  python3 -B docs/reviews/m0-proofs/functions_review_checks.py
Uses: standard library only. Runtime about one minute on one core.
"""

import random
import re
import sys
import time
from fractions import Fraction as Fr
from math import gcd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
RNG = random.Random(20260927)
FAIL = []


def report(name, ok, detail=""):
    print(f"[{'ok' if ok else 'FAIL'}] {name}" + (f": {detail}" if detail else ""), flush=True)
    if not ok:
        FAIL.append(name)


# ---------------------------------------------------------------- helpers (own code)

INF = 10 ** 9


def val(x, p):
    """p-adic valuation of an integer or Fraction; INF for zero."""
    x = Fr(x)
    if x == 0:
        return INF
    a, b, v = abs(x.numerator), x.denominator, 0
    while a % p == 0:
        a //= p
        v += 1
    while b % p == 0:
        b //= p
        v -= 1
    return v


def vfact(k, p):
    """v_p(k!) by counting factors of each integer 1..k (no Legendre formula)."""
    return sum(val(i, p) for i in range(1, k + 1))


VF = {}


def vf(k, p):
    key = (k, p)
    if key not in VF:
        VF[key] = 0 if k == 0 else vf(k - 1, p) + val(k, p)
    return VF[key]


def cdisc(p):
    return 2 if p == 2 else 1


def cdiv(a, b):
    """ceil(a/b) for integer a and positive integer b."""
    return -((-a) // b)


def res(x, p, k):
    """Residue modulo p^k of a p-integral rational."""
    x = Fr(x)
    q = p ** k
    return x.numerator * pow(x.denominator, -1, q) % q


FACT = [1]
for _i in range(1, 400):
    FACT.append(FACT[-1] * _i)


def term_mod(num_int, den_int, p, M):
    """num_int/den_int modulo p^M, where num_int is an exact integer divisible enough by p."""
    e = val(den_int, p)
    assert num_int % p ** e == 0, "numerator not divisible by p-part of denominator"
    unit = den_int // p ** e
    return (num_int // p ** e) * pow(unit, -1, p ** M) % p ** M


def series_mod(fn, x, p, M):
    """f(x) modulo p^M for an integer x in the domain, from exact powers of x. Cutoff 2M+12 degrees.

    Justification of the cutoff: for v(x) >= c the degree k term has valuation
    >= k c - v_p(k!) >= k c - (k-1)/(p-1) >= k/2 (checked numerically in section L), so degrees above
    2M+12 are divisible by p^M. For log(1+z) with v(z) >= 1: k - v_p(k) >= k/2 as well.
    """
    q = p ** M
    top = 2 * M + 12
    s = 0
    if fn == "log":
        z = x
        assert z == 0 or val(z, p) >= 1
        for k in range(1, top + 1):
            s += (-1) ** (k + 1) * term_mod(z ** k, k, p, M)
        return s % q
    assert x == 0 or val(x, p) >= cdisc(p)
    for k in range(0, top + 1):
        if fn == "exp":
            sg = 1
        elif fn in ("sin", "sinh"):
            if k % 2 == 0:
                continue
            sg = (-1) ** (k // 2) if fn == "sin" else 1
        else:
            if k % 2:
                continue
            sg = (-1) ** (k // 2) if fn == "cos" else 1
        s += sg * term_mod(x ** k, FACT[k], p, M)
    return s % q


def teich(x, p, M):
    """Torsion factor w of a unit residue x modulo p^M: limit of x^(p^K) at odd p; sign mod 4 at 2."""
    if p == 2:
        return 1 if x % 4 == 1 else p ** M - 1
    return pow(x, p ** M, p ** M)


def decompose(a, p, M):
    """(m, w, u) for a nonzero rational a, with w and u as residues modulo p^M."""
    m = val(a, p)
    unit = res(Fr(a) / Fr(p) ** m, p, M)
    w = teich(unit, p, M)
    u = unit * pow(w, -1, p ** M) % p ** M
    return m, w, u


def Log(a, p, M):
    """Iwasawa Log modulo p^M of a nonzero rational, via log(u) with u in 1+p^c Z_p."""
    _, _, u = decompose(a, p, M + 2)
    return series_mod("log", u - 1, p, M)


def ball_exponent(values, centre, p, M):
    """Largest E <= M with every value congruent to centre modulo p^E."""
    return min([M] + [val((v - centre) % p ** M, p) for v in values if (v - centre) % p ** M])


def full_ball(centre, p, E, M):
    E = min(E, M)
    return {(centre + t * p ** E) % p ** M for t in range(p ** (M - E))}


# ---------------------------------------------------------------- L  Lemma 5 inequalities

def sec_legendre():
    bad = 0
    n = 0
    for p in (2, 3, 5, 7, 13):
        for k in range(1, 3001):
            a = vf(k, p)
            digits, t = 0, k
            while t:
                digits += t % p
                t //= p
            ok = a * (p - 1) == k - digits and a * (p - 1) <= k - 1
            ok = ok and val(k, p) * (p - 1) <= k - 1 and 2 * val(k, p) <= k
            ok = ok and (k * cdisc(p) - a) * 2 >= k           # used for my own oracle cutoff
            bad += not ok
            n += 1
    report("L  Lemma 5 (Legendre, v_p(k) bounds, oracle cutoff bound)", bad == 0, f"{n} cases, {bad} bad")


# ---------------------------------------------------------------- D  domains (Prop 6) and decomposition (Prop 4)

def sec_domains():
    # p=2, v(x)=1 exactly: degree 2^j terms of valuation exactly 1 and 2^j+1 terms exactly 2, for all x.
    bad = 0
    for x in (2, 6, -2, 10, 2 * 12345):
        for j in range(1, 10):
            k = 2 ** j
            bad += (k * val(x, 2) - vf(k, 2)) != 1
            bad += ((k + 1) * val(x, 2) - vf(k + 1, 2)) != 2
    # v(x) = c: term valuations of all five tend to infinity (monotone lower bound k/2 checked in L).
    for p in (2, 3, 5):
        c = cdisc(p)
        for k in range(1, 400):
            bad += (k * c - vf(k, p)) * 2 < k
    # log at v(z) = 0: subsequence p^j has valuation <= -j + 0.
    for p in (2, 3, 5):
        for j in range(1, 12):
            bad += (p ** j * 0 - j) > -j
    report("D1 Prop 6 boundary shells (p=2 v=1 diverges; v=c converges)", bad == 0, f"{bad} bad")

    # Decomposition: exhaustive over units mod p^M; compare with a brute-force search of (w, u).
    bad = cnt = 0
    for p, M in ((2, 8), (3, 5), (5, 4), (7, 3)):
        q = p ** M
        c = cdisc(p)
        mus = [w for w in range(1, q) if pow(w, p - 1, q) == 1] if p > 2 else [1, q - 1]
        for x in range(1, q):
            if x % p == 0:
                continue
            brute = [(w, x * pow(w, -1, q) % q) for w in mus if (x * pow(w, -1, q) - 1) % p ** c == 0]
            m, w, u = decompose(Fr(x) * Fr(p) ** -3, p, M)
            bad += not (len(brute) == 1 and brute[0] == (w, u) and m == -3)
            cnt += 1
        # Teichmueller at 2 in the sense t^(p-1)=1 is 1, and w(-1) = -1 differs from it.
    bad += decompose(-1, 2, 6)[1] != 63
    report("D2 Prop 4 unique p^m w u (odd p: mu_(p-1); p=2: w by x/2^m mod 4)", bad == 0,
           f"{cnt} units, {bad} bad")


# ---------------------------------------------------------------- T  truncation counts (Prop 7)

def counts(p, v, n):
    """The literal formulas of Prop 7, coded from the text of docs/proofs/functions.md lines 170-181."""
    J = max(1, cdiv(2 * n, 2 * v - 1))
    out = {"log": J - 1}
    if v >= cdisc(p):
        d = (p - 1) * v - 1
        K = max(1, cdiv((p - 1) * n - 1, d))
        out.update(exp=K, sin=K // 2, sinh=K // 2, cos=cdiv(K, 2), cosh=cdiv(K, 2))
    return out


def omitted_degrees(fn, T, upto):
    if fn == "exp":
        return range(T, upto)
    if fn in ("sin", "sinh"):
        return range(2 * T + 1, upto, 2)
    if fn in ("cos", "cosh"):
        return range(2 * T, upto, 2)
    return range(T + 1, upto)


def kept_degrees(fn, T):
    if fn == "exp":
        return list(range(T))
    if fn in ("sin", "sinh"):
        return list(range(1, 2 * T, 2))
    if fn in ("cos", "cosh"):
        return list(range(0, 2 * T, 2))
    return list(range(1, T + 1))


def termval(fn, k, v, p):
    """Worst-case valuation of the degree k term when v(x) = v exactly."""
    return k * v - (val(k, p) if fn == "log" else vf(k, p))


def sec_truncation():
    bad = cases = omitted = 0
    tight = {}       # (fn) -> number of cases where the last kept degree alone would violate n
    waste = {}       # (p, fn) -> max (T - T_min)
    worst = None
    for p in (2, 3, 5, 7, 13):
        for fn in ("exp", "sin", "sinh", "cos", "cosh", "log"):
            vmin = 1 if fn == "log" else cdisc(p)
            for v in range(vmin, vmin + 6):
                for n in range(-6, 151):
                    T = counts(p, v, n)[fn]
                    upto = 4 * (T + 2) + 40
                    for k in omitted_degrees(fn, T, upto):
                        omitted += 1
                        if termval(fn, k, v, p) < n:
                            bad += 1
                            worst = worst or (p, fn, v, n, T, k, termval(fn, k, v, p))
                    # minimal count: kept degrees are the first T of the class, so the least safe
                    # count is the number of class degrees up to the last violating degree
                    cls = omitted_degrees(fn, 0, upto)
                    badk = [k for k in cls if termval(fn, k, v, p) < n]
                    Tmin = 0 if not badk else sum(1 for k in cls if k <= max(badk))
                    if Tmin > T:
                        bad += 1
                    waste[(p, fn)] = max(waste.get((p, fn), 0), T - Tmin)
                    tight[fn] = tight.get(fn, 0) + (Tmin == T and T > 0)
                    cases += 1
    report("T1 Prop 7 every omitted term (to 4T+40) has valuation >= n", bad == 0,
           f"{cases} (p,fn,v,n) cases, p in 2,3,5,7,13, v in c..c+5, n in -6..150, "
           f"{omitted} omitted terms, bad={bad}{'' if not worst else ' first=' + str(worst)}")
    w = ", ".join(f"{k[0]}/{k[1]}:{x}" for k, x in sorted(waste.items()) if x)
    report("T2 Prop 7 slack: max surplus terms over the least safe count (info)", True, w)
    report("T3 Prop 7 cases where the count is exactly minimal (info)", True,
           ", ".join(f"{k}={x}" for k, x in tight.items()))
    # Optional repair R-7: T_log = J* - 1 with J* the least k >= 1 such that k v - e(k) >= n,
    # e(k) = largest e with p^e <= k. k v - e(k) is nondecreasing, so every k >= J* is safe.
    bad = cases = saved = 0
    for p in (2, 3, 5, 7, 13):
        for v in range(1, 6):
            for n in range(-6, 151):
                J = 1
                while True:
                    e = 0
                    while p ** (e + 1) <= J:
                        e += 1
                    if J * v - e >= n:
                        break
                    J += 1
                T_alt = J - 1
                bad += any(k * v - val(k, p) < n for k in range(T_alt + 1, 4 * T_alt + 60))
                saved = max(saved, counts(p, v, n)["log"] - T_alt)
                cases += 1
    report("T5 optional log count J* (least k with k v - floor(log_p k) >= n) is safe", bad == 0,
           f"{cases} cases, bad={bad}, max terms saved against Prop 7's T_log: {saved}")
    # zero and the empty sums
    ok = counts(2, 2, -5)["sin"] == 0 and counts(3, 1, 0)["exp"] == 1 and counts(5, 1, 0)["log"] == 0
    report("T4 Prop 7 n <= 0 gives K=1 (constant only) and empty log/sin sums", ok)


# ---------------------------------------------------------------- W  working precision (Prop 8)

def partial_modular(fn, x, p, v, n, W, shift):
    """The algorithm of Prop 8 literally: x modulo p^W, powers modulo p^W, exact division by p^e,
    unit inverse modulo p^(W-e), everything reduced modulo p^n at the end."""
    T = counts(p, v, n)[fn]
    degs = kept_degrees(fn, T)
    qW = p ** W
    y = (res(x, p, W) + shift * qW) if W > 0 else 0
    s = 0
    for k in degs:
        if fn == "log":
            den, sg = k, (-1) ** (k + 1)
        else:
            den = FACT[k]
            sg = (-1) ** (k // 2) if fn in ("sin", "cos") else 1
        e = val(den, p)
        num = 1 if k == 0 else pow(y, k, qW)
        if num % p ** e:
            return None                                  # division not exact: algorithm undefined
        t = (num // p ** e) * pow(den // p ** e, -1, p ** max(W - e, 1))
        s += sg * t
    return s % p ** n


def exact_mod(fn, x, p, n):
    """f(x) mod p^n for a p-integral rational x, exact: numerator a^k exact, denominator b^k k!."""
    x = Fr(x)
    a, b = x.numerator, x.denominator
    top = 2 * n + 14
    s = 0
    for k in range(0 if fn != "log" else 1, top + 1):
        if fn == "log":
            den, sg = k, (-1) ** (k + 1)
        else:
            if fn in ("sin", "sinh") and k % 2 == 0:
                continue
            if fn in ("cos", "cosh") and k % 2 == 1:
                continue
            den = FACT[k]
            sg = (-1) ** (k // 2) if fn in ("sin", "cos") else 1
        s += sg * term_mod(a ** k, den * b ** k, p, n)
    return s % p ** n


def Dof(fn, T, p):
    degs = kept_degrees(fn, T)
    if not degs:
        return 0
    L = max(degs)
    if fn == "log":
        e = 0
        while p ** (e + 1) <= L:
            e += 1
        return e
    return vf(L, p)


def sec_working():
    bad = cases = undefined = 0
    fail_minus1 = fail_noD = 0
    for p in (2, 3, 5, 7):
        for fn in ("exp", "sin", "sinh", "cos", "cosh", "log"):
            vmin = 1 if fn == "log" else cdisc(p)
            for v in (vmin, vmin + 1, vmin + 3):
                for n in (1, 2, 3, 5, 8, 13, 21, 34):
                    T = counts(p, v, n)[fn]
                    D = Dof(fn, T, p)
                    W = max(v, n + D)
                    for trial in range(4):
                        unit = RNG.randrange(1, 10 ** 6) * RNG.choice((1, -1))
                        while unit % p == 0:
                            unit += 1
                        b = RNG.randrange(1, 10 ** 4)
                        while b % p == 0:
                            b += 1
                        extra = RNG.choice((0, 0, 1, 2))
                        x = Fr(unit * p ** (v + extra), b)
                        truth = exact_mod(fn, x, p, n)
                        for shift in (0, 1, RNG.randrange(2, 10 ** 6)):
                            got = partial_modular(fn, x, p, v, n, W, shift)
                            if got is None:
                                undefined += 1
                            elif got != truth:
                                bad += 1
                            cases += 1
                            g1 = partial_modular(fn, x, p, v, n, max(v, W - 1), shift)
                            fail_minus1 += (g1 is None) or g1 != truth
                            g2 = partial_modular(fn, x, p, v, n, max(v, n), shift)
                            fail_noD += (g2 is None) or g2 != truth
    report("W1 Prop 8 W=max(v,n+D): modular partial sum + tail = f(x) mod p^n", bad == 0 and undefined == 0,
           f"{cases} evaluations (p 2,3,5,7; 6 functions; v in c,c+1,c+3; n up to 34; random x and lifts), "
           f"bad={bad}, undefined divisions={undefined}")
    report("W2 Prop 8 necessity probes (info): W-1 wrong in", True,
           f"{fail_minus1} of {cases}; W=max(v,n) (no D) wrong in {fail_noD} of {cases}")


# ---------------------------------------------------------------- R  radius rules (Prop 10)

def sec_radii():
    bad = cases = 0
    detail = []
    for p in (2, 3, 5):
        c = cdisc(p)
        for fn in ("exp", "sin", "sinh", "cos", "cosh"):
            for N in (c, c + 1, c + 2):
                M = 2 * N - val(2, p) + 1
                centres = sorted({(p ** c * t) % p ** N for t in range(p ** N)})
                if len(centres) > 8:
                    centres = centres[:4] + RNG.sample(centres[4:], 4)
                for a in centres + [p ** c]:
                    pts = [a + p ** N * t for t in range(p ** (M - N))]
                    img = {series_mod(fn, y, p, M) for y in pts}
                    fa = series_mod(fn, a, p, M)
                    E = ball_exponent(img, fa, p, M)
                    cases += 1
                    if fn in ("exp", "sin", "sinh"):
                        ok = E == N and img == full_ball(fa, p, N, M)       # hull N and image = ball
                    else:
                        ok = E >= N + 1                                    # proof step 2: excess >= 1
                        if a % p ** N == 0:
                            ok = ok and E == min(M, 2 * N - val(2, p))
                        else:
                            # own conjecture for the non-centred ball, recorded as information only
                            pred = min(N + val(a, p), 2 * N - val(2, p))
                            if E != pred:
                                detail.append((p, fn, N, a, E, pred))
                    bad += not ok
    report("R1 Prop 10: exp/sin/sinh image = f(a)+p^N exactly; cos/cosh within N+1; centred hull 2N-v(2)",
           bad == 0, f"{cases} balls (p 2,3,5; N=c..c+2; all centres mod p^N), bad={bad}")
    below = [d for d in detail if d[4] < d[5]]
    report("R2 cos/cosh non-centred hull vs min(N+v(a), 2N-v(2)) (info, not claimed by the proof)", True,
           f"{len(detail)} deviations (hull exponent E != prediction), {len(below)} with E < prediction; "
           f"(p, fn, N, a, E, pred) e.g. {detail[:3]}")
    # One step outside: a ball at p=2 with N=1 is not in the domain (contains 2).
    report("R3 cos 4 = 9 mod 16 at 2; v_3(cos 3 - 1) = 2; sin 3 = 3 mod 9",
           series_mod("cos", 4, 2, 4) == 9 and val((series_mod("cos", 3, 3, 6) - 1) % 3 ** 6, 3) == 2
           and series_mod("sin", 3, 3, 2) == 3)


# ---------------------------------------------------------------- G  log and Iwasawa Log radii (Prop 11)

def sec_log():
    bad = cases = 0
    for p in (2, 3, 5):
        c = cdisc(p)
        for N in (1, 2, 3, 4):
            M = N + 4
            cen = list(range(1, p ** N, p))
            if len(cen) > 10:
                cen = cen[:5] + RNG.sample(cen[5:], 5)
            for a in cen:
                img = {series_mod("log", a + p ** N * t - 1, p, M) for t in range(p ** (M - N))}
                la = series_mod("log", a - 1, p, M)
                if N >= c:
                    ok = img == full_ball(la, p, N, M)
                else:
                    ok = img == full_ball(0, 2, 2, M)
                bad += not ok
                cases += 1
    report("G1 Prop 11 series log: image of a+p^N (N>=1) is log(a)+p^N, at 2 with N=1 it is 4Z_2",
           bad == 0, f"{cases} balls, bad={bad}")
    bad = cases = 0
    for p in (2, 3, 5):
        c = cdisc(p)
        for m in (-3, -1, 0, 1, 2):
            for unit in [x for x in range(1, 40) if x % p][:7] + [p ** 3 - 1]:
                a = Fr(unit) * Fr(p) ** m
                for r in (1, 2, 3):
                    N = m + r                               # the literal N of the ball a + p^N Z_p
                    M = r + 3
                    pts = [a + Fr(p) ** N * t for t in range(p ** (M - r + 1))]
                    img = {Log(y, p, M) for y in pts}
                    La = Log(a, p, M)
                    rr = N - val(a, p)
                    if rr >= c:
                        ok = img == full_ball(La, p, rr, M)
                    else:
                        ok = img == full_ball(0, 2, 2, M)
                    bad += not ok
                    cases += 1
    report("G2 Prop 11 Iwasawa Log(a+p^N) = Log(a)+p^(N-m) (r>=c); 4Z_2 at p=2, r=1; m in -3..2",
           bad == 0, f"{cases} balls, bad={bad}")
    ok = (series_mod("log", -2, 2, 12) == 0 and val(series_mod("log", 2, 2, 12), 2) == 2
          and val((Log(12, 3, 8) - Log(3, 3, 8)) % 3 ** 8, 3) == 1
          and val((Log(10, 2, 8) - Log(2, 2, 8)) % 2 ** 8, 2) == 2 and Log(2, 2, 6) == 0)
    report("G3 log(-1)=0, v_2(log 3)=2, Log losses at 3,12 (p=3) and 2,10 (p=2), Log(p)=0", ok)


# ---------------------------------------------------------------- Q  roots (Props 13 and 15)

def is_nth_power_mod(a, n, p, K):
    q = p ** K
    return any(pow(z, n, q) == a % q for z in range(q) if z % p)


def criterion(a, n, p):
    """Prop 13 for a unit a (integer residue): torsion condition and v(log u) >= c + v(n)."""
    c, e = cdisc(p), val(n, p)
    M = c + e + 2
    _, w, u = decompose(a, p, M)
    q = p ** M
    mus = {teich(t, p, M) for t in range(1, p)} if p > 2 else {1, q - 1}
    tors = any(pow(t, n, q) == w for t in mus)
    lg = series_mod("log", u - 1, p, M)
    return tors and (lg == 0 or val(lg, p) >= c + e)


def sec_roots():
    bad = cases = cnt_bad = 0
    for p in (2, 3, 5, 7):
        c = cdisc(p)
        for n in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 25):
            e = val(n, p)
            K = max(2 * e + 2, c + e + 1)
            if p ** K > 3000:
                continue
            q = p ** K
            powers = {pow(z, n, q) for z in range(q) if z % p}
            for a in range(1, q):
                if a % p == 0:
                    continue
                crit = criterion(a, n, p)
                emp = a in powers
                if crit:
                    # construct the root t * exp(log(u)/n) and verify it
                    M = K + e + 2
                    _, w, u = decompose(a, p, M)
                    Q = p ** M
                    mus = [teich(t, p, M) for t in range(1, p)] if p > 2 else [1, Q - 1]
                    t = next(t for t in mus if pow(t, n, Q) == w)
                    lg = series_mod("log", u - 1, p, M + e + 2)
                    z = t * series_mod("exp", (lg // p ** e) * pow(n // p ** e, -1, p ** (M + 2)) % p ** (M + 2),
                                       p, M) % Q
                    ok = pow(z, n, Q) == a % Q
                    # count roots: residues z mod p^K2 with z^n = a mod p^(K2+e), K2 >= c+e+1
                    K2 = c + e + 1
                    qq = p ** (K2 + e)
                    nroots = sum(1 for zz in range(p ** K2) if zz % p and pow(zz, n, qq) == a % qq)
                    g = gcd(n, p - 1) if p > 2 else gcd(n, 2)
                    cnt_bad += nroots != g
                else:
                    ok = not emp                       # a finite witness of non-existence
                bad += not ok
                cases += 1
    report("Q1 Prop 13 criterion: true => explicit root verified; false => not an n-th power mod p^K",
           bad == 0, f"{cases} unit classes, p 2,3,5,7, n up to 25 incl. 4,8,16 at 2 and 9 at 3, bad={bad}")
    report("Q2 Prop 13 count gcd(n,p-1) / gcd(n,2) (residues mod p^(c+e+1) with z^n=a mod p^(c+2e+1))",
           cnt_bad == 0, f"bad={cnt_bad}")
    ok = (not criterion(3, 2, 2)) and criterion(9, 2, 2) and all(
        criterion(a, 2, 2) == (a % 8 == 1) for a in range(1, 64, 2)) and all(
        criterion(a, 2, p) == any(z * z % p == a % p for z in range(1, p)) for p in (3, 5, 7)
        for a in range(1, 50) if a % p)
    report("Q3 square tests: 1 mod 8 at 2; quadratic residue at odd p; 3 not a 2-adic square, 9 is", ok)


def root_image_ok(p, n, beta, r, branch_c):
    """Unit level: input ball beta^n + p^r, output ball beta + p^(r-e). Returns (onto, into)."""
    e = val(n, p)
    E = r - e
    M = max(E, branch_c) + 3
    Min = M + e
    qM, qI = p ** M, p ** Min
    a = pow(beta, n, qI)
    S_out = {(beta + p ** E * t) % qM for t in range(p ** (M - E))}
    S_in = {(a + p ** r * t) % qI for t in range(p ** (Min - r))}
    onto = {pow(z, n, qI) for z in S_out} == S_in
    branch = [z for z in range(beta % p ** branch_c, qM, p ** branch_c)]
    pre = {z for z in branch if pow(z, n, qI) in S_in}
    return onto, pre == S_out


def sec_root_precision():
    bad = cases = 0
    for p in (2, 3, 5, 7):
        c = cdisc(p)
        mus = [teich(t, p, 8) for t in range(1, p)] if p > 2 else [1, 2 ** 8 - 1]
        for n in (1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 25):
            e = val(n, p)
            for zeta in mus[:3]:
                for tail in (0, 1, p - 1):
                    beta = zeta * (1 + p ** c * tail)
                    for r in (c + e, c + e + 1):
                        onto, into = root_image_ok(p, n, beta, r, c)
                        bad += not (onto and into)
                        cases += 1
    report("P1 Prop 15 at the guard and one above: branch image = beta + p^(r-v(n)), both directions",
           bad == 0, f"{cases} (p,n,beta,r) cases, torsion and non-torsion beta, bad={bad}")

    # Literal formula with Fractions, negative j, negative N: exponent N - v(n) - (n-1) j.
    bad = cases = 0
    for p in (2, 3, 5):
        c = cdisc(p)
        for n in (2, 3, 4, 5, 9):
            e = val(n, p)
            for j in (-2, -1, 0, 1, 2):
                beta = 1 + p ** c
                b = Fr(p) ** j * beta
                a = b ** n
                m = val(a, p)
                for N in (m + c + e, m + c + e + 1):          # smallest N allowed by the guard, and next
                    Eout = N - e - (n - 1) * j
                    # sample points of the output ball: their n-th powers lie in the input ball
                    okk = all(val(((b + Fr(p) ** Eout * t) ** n) - a, p) >= N for t in range(1, p ** 2 + 1))
                    # a branch point just outside the output ball (possible when Eout-1-j >= c) maps
                    # outside the input ball, so the exponent cannot be lowered
                    if Eout - 1 - j >= c:
                        okk = okk and val((b + Fr(p) ** (Eout - 1)) ** n - a, p) < N
                    bad += not okk
                    cases += 1
    report("P2 Prop 15 literal exponent N - v(n) - (n-1)j with j in -2..2 (Fractions, minimal N)",
           bad == 0, f"{cases} cases, bad={bad}")

    # One step outside the guard with p | n: some input point has no n-th root at all.
    bad = cases = 0
    for p, n in ((2, 2), (2, 4), (2, 6), (3, 3), (3, 9), (5, 5), (3, 6)):
        c, e = cdisc(p), val(n, p)
        r = c + e - 1
        K = 2 * e + 2 + r
        witness = any(not is_nth_power_mod(1 + p ** r * t, n, p, K) for t in range(1, p))
        bad += not witness
        cases += 1
    report("P3 guard - 1 with p | n: input ball 1 + p^(c+v(n)-1) contains a non-n-th power", bad == 0,
           f"{cases} (p,n) pairs, bad={bad}")

    # Refinement probe: p=2 and odd n, r = 1 (outside the stated guard r >= 2).
    bad = cases = 0
    for n in (1, 3, 5, 7, 9):
        for beta in (1, 3, 5, 7, 13):
            onto, into = root_image_ok(2, n, beta, 1, 1)
            bad += not (onto and into)
            cases += 1
    report("P4 (info) p=2, n odd, r=1: unique root image is b + 2^(j+1) (guard is conservative there)",
           bad == 0, f"{cases} cases, bad={bad}")


# ---------------------------------------------------------------- X  powers (Props 17 and 18)

def sec_powers():
    bad = cases = 0
    distinguishing = {"A+beta": 0, "B+alpha": 0, "A+B": 0}
    for p, k in ((2, 8), (3, 5), (5, 4)):
        c = cdisc(p)
        q = p ** k
        per = p ** (k - c)                       # u^s mod p^k depends on s mod p^(k-c) for u in 1+p^c
        for u0 in (1, 1 + p ** c, 1 + p ** (c + 1), 1 + 3 * p ** c, 1 + p ** (c + 2)):
            lg = series_mod("log", u0 - 1, p, k + 4)
            alpha = INF if lg == 0 else val(lg, p)
            for A in range(c, min(c + 3, k) + 1):
                bases = [(u0 + p ** A * t) % q for t in range(p ** (k - A))]
                for s0 in (0, 1, -1, 2, p, p * p, 3 * p + 1):
                    beta = val(s0, p)
                    for B in range(0, 4):
                        if len(bases) * p ** max(0, k - c - B) > 70000:
                            continue
                        exps = sorted({(s0 + p ** B * t) % per for t in range(per)})
                        img = {pow(u, s, q) for u in bases for s in exps}
                        R = min(A + beta, B + alpha, A + B)
                        centre = pow(u0, s0 % per, q)
                        ok = img == full_ball(centre, p, R, k)
                        bad += not ok
                        cases += 1
                        terms = {"A+beta": A + beta, "B+alpha": B + alpha, "A+B": A + B}
                        for nm, x in terms.items():
                            if x < k and all(x < y for m2, y in terms.items() if m2 != nm):
                                distinguishing[nm] += 1
    report("X1 Prop 18 image of (u0+p^A)^(s0+p^B) = u0^s0 + p^min(A+beta, B+alpha, A+B)", bad == 0,
           f"{cases} exhaustive images (2^8, 3^5, 5^4), bad={bad}; cases where each term is the strict "
           f"minimum below k: {distinguishing}")

    # Integer compatibility, including negative s; exp(Log p) = 1.
    bad = 0
    for p, k in ((2, 10), (3, 7), (5, 5)):
        q = p ** k
        for u in (1 + p ** cdisc(p), 1 - p ** cdisc(p), 1 + 7 * p ** (cdisc(p) + 1)):
            lg = series_mod("log", u - 1, p, k + 3)
            for s in (-7, -2, -1, 0, 1, 3, 10):
                bad += series_mod("exp", s * lg % p ** (k + 3), p, k) != pow(u, s, q)
        bad += series_mod("exp", Log(p, p, k), p, k) != 1
    report("X2 Prop 17 u^s = exp(s log u) agrees with integer powers (s in -7..10); exp(Log p) = 1", bad == 0)

    # 2-adic signed extension: x^s = w^(s mod 2) exp(s log u); image rule with A>=2, B>=1; union otherwise.
    bad = cases = union_needed = 0
    k = 8
    q = 2 ** k
    per = 2 ** (k - 2)
    for x0 in (3, 5, 7, 11, 13):
        for A in (1, 2, 3):
            bases = [(x0 + 2 ** A * t) % q for t in range(2 ** (k - A))]
            for s0 in (0, 1, 2, 3):
                for B in (0, 1, 2):
                    exps = sorted({(s0 + 2 ** B * t) % per for t in range(per)})
                    img = {pow(x, s, q) for x in bases for s in exps}
                    # formula check pointwise
                    for x in bases[:4]:
                        _, w, u = decompose(x, 2, k)
                        lg = series_mod("log", u - 1, 2, k + 3)
                        for s in exps[:4]:
                            val_formula = (pow(w, s % 2, q) * series_mod("exp", s * lg % 2 ** (k + 3), 2, k)) % q
                            bad += val_formula != pow(x, s, q)
                    _, w0, u0 = decompose(x0, 2, k)
                    lg0 = series_mod("log", u0 - 1, 2, k + 3)
                    alpha = INF if lg0 == 0 else val(lg0, 2)
                    beta = val(s0, 2)
                    if A >= 2 and B >= 1:
                        R = min(A + beta, B + alpha, A + B)
                        ok = img == full_ball(pow(x0, s0, q), 2, R, k)
                        bad += not ok
                    else:
                        # single-sign principal formula would be wrong whenever the image has both signs
                        signs = {y % 4 for y in img}
                        union_needed += len(signs) == 2
                    cases += 1
    report("X3 Prop 17/18 at 2 for odd units: pointwise formula; ball rule for A>=2,B>=1", bad == 0,
           f"{cases} cases, bad={bad}; A=1 or B=0 cases whose image has both classes mod 4: {union_needed}")


# ---------------------------------------------------------------- M  mutants against the author's checks

def load_author_source():
    return (ROOT / "proto" / "functions_checks.py").read_text()


def run_mutant(src, old, new, check):
    assert src.count(old) == 1, f"mutation anchor not unique: {old!r} ({src.count(old)})"
    mutated = src.replace(old, new)
    ns = {"__name__": "author_mutant"}
    exec(compile(mutated, "functions_checks_mutant", "exec"), ns)
    try:
        ns[check]()
    except AssertionError:
        return "killed"
    except Exception as ex:   # noqa: BLE001  a crash also counts as detection, but is reported
        return f"killed(crash:{type(ex).__name__})"
    return "SURVIVED"


MUTANTS = [
    ("K-1 for factorial series", "k = max(1, ceildiv((p-1)*n-1, (p-1)*v-1))",
     "k = max(1, ceildiv((p-1)*n-1, (p-1)*v-1)-1)", "check_truncation"),
    ("d=(p-1)v instead of (p-1)v-1", "k = max(1, ceildiv((p-1)*n-1, (p-1)*v-1))",
     "k = max(1, ceildiv((p-1)*n-1, (p-1)*v))", "check_truncation"),
    ("T_sin = floor((K-1)/2)", "sin=k//2, sinh=k//2", "sin=(k-1)//2, sinh=(k-1)//2", "check_truncation"),
    ("T_cos = floor(K/2)", "cos=(k+1)//2, cosh=(k+1)//2", "cos=k//2, cosh=k//2", "check_truncation"),
    ("T_log = J-2", "answer = {\"log\": max(1, ceildiv(2*n, 2*v-1)) - 1}",
     "answer = {\"log\": max(1, ceildiv(2*n, 2*v-1)) - 2}", "check_truncation"),
    ("T_log with 2v instead of 2v-1", "answer = {\"log\": max(1, ceildiv(2*n, 2*v-1)) - 1}",
     "answer = {\"log\": max(1, ceildiv(2*n, 2*v)) - 1}", "check_truncation"),
    ("W = max(v, n+D-1)", "w = max(v, n+d)", "w = max(v, n+d-1)", "check_working_precision"),
    ("W = max(v, n) (no D)", "w = max(v, n+d)", "w = max(v, n)", "check_working_precision"),
    ("W = n+D (drop v)", "w = max(v, n+d)", "w = max(1, n+d)", "check_working_precision"),
    ("cos hull 2N (drop v_p(2))", "output_n = 2*exponent-(p == 2)", "output_n = 2*exponent", "check_series_radii"),
    ("cos hull 2N-1 everywhere", "output_n = 2*exponent-(p == 2)", "output_n = 2*exponent-1", "check_series_radii"),
    ("Log r=1 at 2 gives Log(a)+2Z_2", "out_r = 2 if p == 2 and r == 1 else r", "out_r = r", "check_log_radii"),
    ("Log image r+1", "out_r = 2 if p == 2 and r == 1 else r", "out_r = 2 if p == 2 and r == 1 else r+1",
     "check_log_radii"),
    ("root log condition c+v(n) -> 1+v(n)", ">= cdisc(p)+vp(n, p)", ">= 1+vp(n, p)", "check_root_criteria"),
    ("root log condition drops v(n)", ">= cdisc(p)+vp(n, p)", ">= cdisc(p)", "check_root_criteria"),
    ("root out exponent -n j", "nout = nin-e-(n-1)*j", "nout = nin-e-n*j", "check_root_precision"),
    ("root out exponent without v(n)", "nout = nin-e-(n-1)*j", "nout = nin-(n-1)*j", "check_root_precision"),
    ("root guard c+v(n)-1", "relative_in = relative_out+e", "relative_in = relative_out+e-1",
     "check_root_precision"),
    ("power R drops A+B", "r = min(a+beta, b+alpha, a+b, k)", "r = min(a+beta, b+alpha, k)",
     "check_power_precision"),
    ("power R drops B+alpha", "r = min(a+beta, b+alpha, a+b, k)", "r = min(a+beta, a+b, k)",
     "check_power_precision"),
    ("power R = A+B+1 term", "r = min(a+beta, b+alpha, a+b, k)", "r = min(a+beta, b+alpha, a+b+1, k)",
     "check_power_precision"),
    ("power R uses alpha+1", "r = min(a+beta, b+alpha, a+b, k)", "r = min(a+beta, b+alpha+1, a+b, k)",
     "check_power_precision"),
]


def sec_mutants():
    src = load_author_source()
    survived = []
    for name, old, new, check in MUTANTS:
        t0 = time.time()
        out = run_mutant(src, old, new, check)
        print(f"     mutant {name!r} -> {check}: {out} ({time.time() - t0:.1f}s)", flush=True)
        if out == "SURVIVED":
            survived.append(name)
    report("M  author's checks against wrong formulas (info: survivors are weak checks)", True,
           f"{len(MUTANTS) - len(survived)} of {len(MUTANTS)} killed; survivors: {survived}")


# ---------------------------------------------------------------- main

def main():
    t0 = time.time()
    sections = [sec_legendre, sec_domains, sec_truncation, sec_working, sec_radii, sec_log, sec_roots,
                sec_root_precision, sec_powers]
    if "--no-mutants" not in sys.argv:
        sections.append(sec_mutants)
    for s in sections:
        s()
    print(f"total {time.time() - t0:.1f}s; failures: {FAIL}")
    return 1 if FAIL else 0


if __name__ == "__main__":
    sys.exit(main())
