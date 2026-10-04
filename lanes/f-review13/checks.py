#!/usr/bin/env python3
"""f-review13 checks of IL1-IL7 and of the library against them (own oracle il_oracle.py).
Usage: python3 -B checks.py SECTION... (sections: sets hprime series lib crt)."""
import sys, random, subprocess
from math import gcd
sys.path.insert(0, 'lanes/f-review13')
import il_oracle as o

PROBE = 'lanes/f-review13/build/probe'
HS = {2: 5, 3: 5, 5: 4, 7: 4}

def contents(p):
    out = []
    for m in (-2, 0, 1, 3):
        for a, b in ((1, 1), (2, 1), (1, 2), (3, 4), (5, 1), (7, 11), (p + 1, 1), (1, p + 1)):
            if a % p == 0 or b % p == 0:
                continue
            num, den = a * p**max(m, 0), b * p**max(-m, 0)
            g = gcd(num, den)
            out.append((num // g, den // g))
    return sorted(set(out))

def moduli(p):
    other = [q for q in (2, 3, 5) if q != p]
    for k in range(0, 4):
        for cof in (1, other[0], other[1], other[0] * other[1]):
            yield p**k * cof

def residues(M, rng):
    cs = [c for c in range(1, M + 1) if gcd(c, M) == 1]
    if len(cs) > 10:
        cs = sorted(set([1, M - 1] + rng.sample(cs, 8)))
    return cs

def grid(rng):
    for p in (2, 3, 5, 7):
        for M in moduli(p):
            for c in residues(M, rng):
                for num, den in contents(p):
                    yield p, num, den, c, M
        for num, den in contents(p):
            for c in (1, -1):
                yield p, num, den, c, 0

def sec_sets():
    rng = random.Random(13)
    n = units = 0
    for p, num, den, c, M in grid(rng):
        H = HS[p]
        im = o.image(p, num, den, c, M, H)
        pr = o.predicted(p, num, den, c, M, H)
        assert im == pr, (p, num, den, c, M, H)
        if M:
            k = o.v(M, p)
            E = max(k, o.dd(p))
            assert len(im) == p**(H - E)
        n += 1
    print('sets: %d cases, image == IL2-IL4 prediction modulo p^H, H = %s' % (n, HS))

def sec_hprime():
    rng = random.Random(14)
    n = 0
    for p, num, den, c, M in grid(rng):
        if M == 0 or rng.random() > 0.15:
            continue
        H = HS[p]
        a = o.image(p, num, den, c, M, H)
        b = {t % p**H for t in o.image(p, num, den, c, M, H + 1)}
        assert a == b, (p, num, den, c, M)
        n += 1
    print('hprime: %d cases, enumeration mod p^(H+1) reduced equals enumeration mod p^H' % n)

def sec_series():
    sys.path.insert(0, 'proto')
    import idlog_checks as d
    n = 0
    for p in (2, 3, 5, 7):
        H = HS[p]
        tab = o.log_table(p, H)
        for a, y in tab.items():
            assert d.log_unit(a, p, H) == y, (p, a)
            n += 1
    print('series: %d units, design oracle log_unit == own exp-inversion Log mod p^H' % n)
    # Sensitivity: the design truncation T = 2H and W = H + floor(log_p(T-1)); smaller choices.
    for p in (2, 3):
        H = HS[p]
        tab = o.log_table(p, H)
        for T in range(2, 2 * H + 1):
            bad = 0
            for a, y in tab.items():
                if p == 2:
                    u = a if a % 4 == 1 else (-a) % p**H
                    z = u - 1
                else:
                    z = pow(a, p - 1, p**(H + 3)) - 1
                s = 0
                for j in range(1, T):
                    e = o.v(j, p)
                    term = (z**j // p**e) * pow(j // p**e, -1, p**H)
                    s += term if j % 2 else -term
                s %= p**H
                if p != 2:
                    s = s * pow(p - 1, -1, p**H) % p**H
                bad += s != y
            print('series: p=%d H=%d truncation T=%d (degrees < T): %d wrong of %d' % (p, H, T, bad, len(tab)))

def run_probe(lines):
    out = subprocess.run(['timeout', '170', PROBE], input='\n'.join(lines) + '\n',
                         capture_output=True, text=True, check=True).stdout.splitlines()
    assert len(out) == len(lines), (len(out), len(lines))
    return out

def ball_fields(C, K):
    if K <= 0 or C == 0:
        return 0, '0', K
    vv = o.v(C, p_cur)
    return vv, str(C // p_cur**vv), K

def expected_at(p, num, den, c, M, N):
    """Smallest ball of exponent K containing the image (from own enumeration), or exact zero."""
    global p_cur
    p_cur = p
    H = HS[p]
    m, a, b = o.unit_part(num, den, p)
    if M == 0:
        if a == b:
            return 'exact=1 v=0 N=0 u=0'
        K = N
    else:
        K = min(N, max(o.v(M, p), o.dd(p)))
    if K > H:
        return None
    im = o.image(p, num, den, c, M, H)
    if K <= 0:
        return 'exact=0 v=0 N=%d u=0' % K
    cls = {t % p**K for t in im}
    assert len(cls) == 1, (p, num, den, c, M, N, cls)
    C = cls.pop()
    vv, u, K = ball_fields(C, K)
    return 'exact=0 v=%d N=%d u=%s' % (vv, K, u)

def sec_lib():
    rng = random.Random(15)
    cases, lines = [], []
    for p, num, den, c, M in grid(rng):
        H = HS[p]
        E = max(o.v(M, p), o.dd(p)) if M else 2
        for N in sorted({-1, 0, 1, 2, E - 1, E, E + 1, H}):
            want = expected_at(p, num, den, c, M, N)
            if want is None:
                continue
            cases.append((p, num, den, c, M, N, want))
            lines.append('at %d %d %d %d %d %d' % (p, num, den, c, M, N))
    out = run_probe(lines)
    bad = 0
    for (p, num, den, c, M, N, want), got in zip(cases, out):
        if got != 'st=0 where=untouched ' + want:
            bad += 1
            if bad <= 10:
                print('MISMATCH', p, num, den, c, M, N, 'want', want, 'got', got)
    print('lib: %d Log_at calls compared with the smallest ball from own enumeration: %d mismatches'
          % (len(cases), bad))

def sec_crt():
    rng = random.Random(16)
    lines, cases = [], []
    inputs = [(4, 1, 1, 9), (1, 1, 2, 9), (3, 4, 5, 36), (1, 1, 3, 8), (5, 1, 7, 16), (1, 3, 1, 2),
              (2, 1, 5, 6), (1, 1, 1, 0), (2, 1, -1, 0), (7, 3, 1, 1), (1, 1, 11, 45)]
    for num, den, c, M in inputs:
        for S in ((), (2,), (3,), (5,), (2, 3), (3, 5), (5, 2), (2, 3, 5), (7,), (2, 7)):
            for N in (-1, 0, 1, 2, 3, 4):
                # Expected C_S: z in Zhat, z = 0 mod 4 unless 2 is more refined, z_p in B_p.
                conds = {2: (0, 4)}
                ok = True
                for p in S:
                    H = HS[p]
                    m, a, b = o.unit_part(num, den, p)
                    K = N if M == 0 else min(N, max(o.v(M, p), o.dd(p)))
                    if K > H:
                        ok = False; break
                    if M == 0 and a == b:
                        C = 0
                    elif K <= 0:
                        C = 0
                    else:
                        cls = {t % p**K for t in o.image(p, num, den, c, M, H)}
                        assert len(cls) == 1
                        C = cls.pop()
                    beta = 2 if p == 2 else 0
                    if K > beta:
                        conds[p] = (C, p**K)
                if not ok:
                    continue
                # CRT by search (no modular inverse): a + R t = b mod q for exactly one t mod q.
                a0, R = 0, 1
                for b0, q in conds.values():
                    ts = [t for t in range(q) if (a0 + R * t - b0) % q == 0]
                    assert len(ts) == 1
                    a0, R = a0 + R * ts[0], R * q
                A = [a0 % R]
                cases.append((num, den, c, M, S, N, 'st=0 where=untouched A=%d H=%d d=1' % (A[0], R)))
                pl = list(S); rng.shuffle(pl)
                lines.append('ref 2 %d %d %d %d %d %d %s' % (num, den, c, M, N, len(pl), ' '.join(map(str, pl))))
    out = run_probe(lines)
    bad = 0
    for case, got in zip(cases, out):
        if got != case[-1]:
            bad += 1
            if bad <= 10:
                print('MISMATCH', case, 'got', got)
    print('crt: %d Log_refine calls compared with brute-force C_S: %d mismatches' % (len(cases), bad))

if __name__ == '__main__':
    for s in sys.argv[1:]:
        globals()['sec_' + s]()
