#!/usr/bin/env python3
# oracle.py (lane f-review4): my own oracle for log and Log at a prime, and the driver that sends inputs through
# lanes/f-review4/probe.c (new, old, forced-F8, forced-F9 routes and FLINT) and compares.
#
# Oracle (own proof, written here so that the report can cite it):
#  1. log(1+z) for z p-integral, w = v_p(z) >= 1: the degree k term has valuation k w - v_p(k) >= k w - k/2
#     (v_p(k) <= k/2: p^e <= k and p^e >= 2^e >= 2e for e >= 1), so every degree k >= T0 = ceil(2(n+4)/(2w-1)) has
#     valuation >= n + 4, the omitted tail is in p^(n+4) Z_p, and the exact rational partial sum of degrees
#     1 .. T0-1, reduced modulo p^n (its denominator is a unit times a power of p that divides the numerator, as
#     the sum is p-integral), is log(1+z) modulo p^n. The sum is formed EXACTLY as a pair of integers (no
#     reduction anywhere): S(a,b) = sum_{k=a}^{b-1} (-1)^(k+1) z^(k-a+1)/k, S(a,b) = S(a,m) + z^(m-a) S(m,b).
#     For n <= 400 the same value is also formed naively with Fraction, term by term (`naive`), as a check of
#     the splitting code itself.
#  2. Log(x) = log(u), x = p^m w u. At p = 2: u = +-a (a = x/2^m odd), log(a) = log(-a) (2 log(-1) = log(1) = 0),
#     and the raw series at a converges on 1 + 2 Z_2 (functions.md Proposition 6, step 4), so Log(x) = series(a-1)
#     with w = 1. At odd p: a mod p in {1, -1} gives w = +-1 exactly (the unique root of T^(p-1)-1 with that
#     residue, Hensel with unit derivative); otherwise w modulo p^H, H = n + 3, by Newton's iteration on
#     f(T) = T^(p-1) - 1 from the residue (f(a0) = 0 mod p by Fermat; f'(w) a unit; a root modulo p^h gives a root
#     modulo p^(2h)); u_H = a / w modulo p^H as an integer; log is an isometry on 1 + p Z_p at odd p
#     (Proposition 11), so log(u_H) = log(u) modulo p^H, and series(u_H - 1) modulo p^n is Log(x) modulo p^n.
#  3. When the exact rational sum would be too large (bits(z) * T0 > 3e7), `direct` is used instead: the sum
#     modulo p^W, W = n + floor(log_p T0) + 2, dividing each term by p^(v_p k) and by the inverse of the unit part
#     of k (functions.md Proposition 8). This is a re-implementation of the OLD route in Python; where it is
#     used the report says so (it is independent code, not an independent method).
import sys, os, random, subprocess, math, time
from fractions import Fraction
import flint
sys.set_int_max_str_digits(0)

BIG = 2 ** 64 - 59
PRIMES = [2, 3, 5, 7, 11, 65537, BIG]
HERE = os.path.dirname(os.path.abspath(__file__))
PROBE = sys.argv[1] if len(sys.argv) > 1 else None

def vp_int(n, p):
    n = abs(int(n))
    if n == 0:
        return 10 ** 9
    e = 0
    while n % p == 0:
        n //= p; e += 1
    return e

def vp_frac(q, p):
    return vp_int(q.numerator, p) - vp_int(q.denominator, p)

def tail_start(n, w):
    return -(-2 * (n + 4) // (2 * w - 1))

def split_sum(zn, zd, a, b):
    """(N, D) with N/D = sum_{k=a}^{b-1} (-1)^(k+1) z^(k-a+1)/k, z = zn/zd, exact integers, no gcd."""
    if b - a == 1:
        return (zn if a % 2 == 1 else -zn, zd * a)
    m = (a + b) // 2
    n1, d1 = split_sum(zn, zd, a, m)
    n2, d2 = split_sum(zn, zd, m, b)
    zpn = zn ** (m - a); zpd = zd ** (m - a)
    return (n1 * zpd * d2 + zpn * n2 * d1, d1 * zpd * d2)

def reduce_mod(N, D, p, n):
    """N/D p-integral: its residue modulo p^n."""
    N = int(N); D = int(D)
    e = vp_int(D, p)
    pe = p ** e
    assert N % pe == 0, "numerator not divisible by the p part of the denominator"
    N //= pe; D //= pe
    P = p ** n
    return (N % P) * pow(D % P, -1, P) % P

def series_exact(z, p, n):
    w = vp_frac(z, p)
    assert w >= 1
    T0 = tail_start(n, w)
    if T0 <= 1:
        return 0
    N, D = split_sum(flint.fmpz(z.numerator), flint.fmpz(z.denominator), 1, T0)
    return reduce_mod(N, D, p, n)

def series_naive(z, p, n):
    w = vp_frac(z, p)
    T0 = tail_start(n, w)
    s = Fraction(0); zk = Fraction(1)
    for k in range(1, T0):
        zk *= z
        s += zk / k if k % 2 == 1 else -zk / k
    return reduce_mod(s.numerator, s.denominator, p, n)

def series_direct(z, p, n):
    """Proposition 8 with ONE common denominator L = lcm(1..T0-1) = p^e L': S = sum (-1)^(k+1) (L/k) z^k modulo
    p^W, W = n + e, is L times the partial sum modulo p^W; the partial sum is p-integral, so p^e divides S, and
    (S / p^e) L'^(-1) modulo p^n is the partial sum modulo p^n."""
    w = vp_frac(z, p)
    T0 = tail_start(n, w)
    e = 0
    while p ** (e + 1) <= T0 - 1:
        e += 1
    W = n + e
    P = flint.fmpz(p) ** W
    zr = flint.fmpz(z.numerator * pow(z.denominator, -1, int(P)) % int(P))
    L = math.lcm(*range(1, T0))
    S = flint.fmpz(0); zk = flint.fmpz(1)
    for k in range(1, T0):
        zk = zk * zr % P
        term = flint.fmpz(L // k) * zk
        S = S + term if k % 2 == 1 else S - term
    S = int(S % P)
    pe = p ** e
    assert S % pe == 0
    Lp = L // pe
    assert Lp % p != 0
    Pn = p ** n
    return (S // pe) % Pn * pow(Lp % Pn, -1, Pn) % Pn

def series_mod(z, p, n, force=None):
    """log(1+z) modulo p^n, z p-integral with v_p(z) >= 1; returns (residue, method)."""
    if z == 0:
        return 0, 'zero'
    w = vp_frac(z, p)
    T0 = tail_start(n, w)
    bits = (z.numerator.bit_length() + z.denominator.bit_length()) * T0
    if force == 'naive':
        return series_naive(z, p, n), 'naive'
    if force == 'direct' or bits > 3e7:
        if n > 6000 and force is None:
            return None, 'skipped'                          # FLINT is the only check of such a case
        return series_direct(z, p, n), 'direct'
    return series_exact(z, p, n), 'exact'

def hensel_root(a0, p, H):
    """The (p-1)-st root of unity w modulo p^H with w = a0 modulo p (a0 not 0 modulo p)."""
    h = 1; w = a0 % p
    while h < H:
        h2 = min(2 * h, H); mod = p ** h2
        fw = (pow(w, p - 1, mod) - 1) % mod
        fpw = (p - 1) * pow(w, p - 2, mod) % mod
        w = (w - fw * pow(fpw, -1, mod)) % mod
        h = h2
    return w

def Log_mod(x, p, n):
    """Log(x) modulo p^n for a nonzero rational x; returns (residue, method)."""
    m = vp_frac(x, p)
    a = x / Fraction(p) ** m if m >= 0 else x * Fraction(p) ** (-m)
    if p == 2:
        return series_mod(a - 1, 2, n)
    a0 = a.numerator * pow(a.denominator, -1, p) % p
    if a0 == 1:
        return series_mod(a - 1, p, n)
    if a0 == p - 1:
        return series_mod(-a - 1, p, n)
    H = n + 3; P = p ** H
    w = hensel_root(a0, p, H)
    assert (pow(w, p - 1, P) - 1) % P == 0
    u = a.numerator * pow(a.denominator, -1, P) * pow(w, -1, P) % P
    return series_mod(Fraction(u - 1), p, n)

def log_mod(x, p, n):
    """the series log(x) modulo p^n for x in 1 + p Z_p (a rational); at 2 also for x = 3 mod 4 (log = Log)."""
    if p == 2 and (x.numerator - x.denominator) % 4 != 0 or vp_frac(x - 1, p) < 1:
        return Log_mod(x, 2, n)
    return series_mod(x - 1, p, n)

# ------------------------------------------------------------------------------------------------- the driver

class Case:
    def __init__(self, name, p, u, v, M, exact, N, flint=0, mask=7, tag=''):
        self.name, self.p, self.u, self.v, self.M, self.exact, self.N = name, p, Fraction(u), v, M, exact, N
        self.flint, self.mask, self.tag = flint, mask, tag
    def line(self):
        return f"{self.name} {self.p} {self.u.numerator} {self.u.denominator} {self.v} {self.M} {self.exact} {self.N} {self.flint} {self.mask}"
    def centre(self):
        return self.u * Fraction(self.p) ** self.v if self.v >= 0 else self.u / Fraction(self.p) ** (-self.v)

def exact_case(name, p, x, N, **kw):
    """an exact rational input x != 0 (Log) or in 1 + p Z_p (log) as raw fields: x = p^v u, u prime to p."""
    x = Fraction(x)
    v = vp_frac(x, p)
    u = x / Fraction(p) ** v if v >= 0 else x * Fraction(p) ** (-v)
    return Case(name, p, u, v, 0, 1, N, **kw)

def ball_case(name, p, u, v, M, N, **kw):
    """the canonical ball p^v u + p^M Z_p, 0 < u < p^(M-v), p not dividing u."""
    assert 0 < u < p ** (M - v) and u % p != 0
    return Case(name, p, u, v, M, 0, N, **kw)

def parse_ball(f):
    st, ex, v, N, num, den = f
    if st == '-':
        return None
    return (int(st), int(ex), int(v), int(N), int(num), int(den))

def run_probe(cases, probe=PROBE, timeout=600):
    inp = "\n".join(c.line() for c in cases) + "\n"
    r = subprocess.run(["timeout", str(timeout), probe], input=inp, capture_output=True, text=True)
    if r.returncode != 0:
        print("probe exit", r.returncode, r.stderr[:500])
        sys.exit(1)
    out = []
    for line in r.stdout.strip().split("\n"):
        f = line.split()
        assert len(f) == 4 * 6 + 3 + 3, line
        out.append(dict(new=parse_ball(f[0:6]), old=parse_ball(f[6:12]), sum=parse_ball(f[12:18]),
                        bal=parse_ball(f[18:24]), t_old=float(f[24]), t_new=float(f[25]), alias=int(f[26]),
                        fl=None if f[27] == '-' else (int(f[27]), int(f[28]), int(f[29]))))
    return out

def in_ball(res, p, value_mod):
    """value_mod: (residue r, exponent K) of a p-adic number modulo p^K; res a ball (st OK, exact 0)."""
    st, ex, v, N, num, den = res
    r, K = value_mod
    assert K == N, (K, N)
    return (r - num * p ** v) % p ** N == 0 if v >= 0 else False

def check(cases, results, log, stats):
    for c, r in zip(cases, results):
        stats['cases'] += 1
        new = r['new']
        for key in ('old', 'sum', 'bal'):
            if r[key] is not None:
                stats[key] += 1
                if r[key] != new:
                    stats['diff_' + key] += 1
                    log.append(f"DIFF {key}: {c.line()} [{c.tag}] new={new} {key}={r[key]}")
        if not r['alias']:
            stats['alias_fail'] += 1
            log.append(f"ALIAS: {c.line()} new={new}")
        if new[0] != 0:
            stats['status_' + str(new[0])] += 1
            continue
        if new[1] == 1:
            stats['exact_results'] += 1
            continue
        K = new[3]
        if K <= 0:
            stats['K_le_0'] += 1
            continue
        if c.oracle:
            x = c.centre()
            t0 = time.time()
            val, method = (log_mod if c.name == 'log' else Log_mod)(x, c.p, K)
            stats['oracle_' + method] += 1
            stats['oracle_time'] += time.time() - t0
            if val is not None and not in_ball(new, c.p, (val, K)):
                stats['oracle_fail'] += 1
                log.append(f"ORACLE({method}): {c.line()} [{c.tag}] new={new} oracle={val}")
            if c.name == 'log' and K <= 400 and c.exact and stats['naive'] < 400 and (stats['cases'] % 7 == 0) \
                    and val is not None:
                z = x - 1 if not (c.p == 2 and (x.numerator - x.denominator) % 4) else -x - 1
                nv = series_naive(z, c.p, K)
                stats['naive'] += 1
                if nv != val:
                    stats['naive_fail'] += 1
                    log.append(f"NAIVE != EXACT: {c.line()} naive={nv} exact={val}")
            if not c.exact and c.point:
                # the points of the ball a + p^M Z_p that differ from the centre in the first digit beyond the
                # admitted ones: a + p^M and a - p^M (a + p^(M-1) is NOT in the ball)
                for sign in (1, -1):
                    pt = x + sign * Fraction(c.p) ** c.M
                    val2, method2 = (log_mod if c.name == 'log' else Log_mod)(pt, c.p, K)
                    stats['point'] += 1
                    if val2 is not None and not in_ball(new, c.p, (val2, K)):
                        stats['point_fail'] += 1
                        log.append(f"POINT({method2}): {c.line()} [{c.tag}] new={new} f(a{sign:+d}p^M)={val2}")
        if r['fl'] is not None:
            stats['flint'] += 1
            ok, fn, fd = r['fl']
            if not ok:
                stats['flint_refused'] += 1
            else:
                P = c.p ** K
                fr = fn * pow(fd, -1, P) % P if vp_int(fd, c.p) == 0 else None
                if fr is None or not in_ball(new, c.p, (fr, K)):
                    stats['flint_fail'] += 1
                    log.append(f"FLINT: {c.line()} [{c.tag}] new={new} flint={fn}/{fd}")

def with_flags(cases, oracle=True, point=True):
    for c in cases:
        c.oracle = oracle; c.point = point
    return cases

# ------------------------------------------------------------------------------------------------ input families

def log_centres(p):
    cs = [1 + p, 1 + p * p, 1 - p, 1 + 2 * p, Fraction(1 + p, 1 - p), Fraction(1, 1 + p), 1 + p ** 3 + p ** 5]
    if p == 2:
        cs += [3, 7, -5, Fraction(3, 5), Fraction(-1, 3), Fraction(1, 3), 1 - 2 ** 7]
    return [Fraction(c) for c in cs]

def Log_centres(p):
    cs = [2, 3, 5, Fraction(5, 7), -3, 10, 1 + p, Fraction(1, 2), p * 3, Fraction(2, p ** 3), Fraction(-7, p),
          p ** 2 * Fraction(11, 13)]
    return [Fraction(c) for c in cs if c != 0 and c != 1 and c != -1 and (p != 2 or True)]

def rand_unit(rng, p, digits):
    while True:
        u = rng.randrange(1, p ** digits)
        if u % p:
            return u

def rand_principal(rng, p, M, v0):
    """0 < u < p^M with u = 1 modulo p^v0 (the log domain: v0 = 1; the Log inputs need no congruence)."""
    while True:
        u = 1 + p ** v0 * rng.randrange(0, p ** (M - v0))
        if 0 < u < p ** M and u % p:
            return u

def family_exhaustive(p, lo, hi, rng, mask=7):
    cases = []
    for N in range(lo, hi + 1):
        for x in log_centres(p)[:4] + log_centres(p)[7:9]:
            cases.append(exact_case('log', p, x, N, mask=mask, tag='exh-log'))
        for x in Log_centres(p)[:3] + Log_centres(p)[9:11]:
            cases.append(exact_case('Log', p, x, N, mask=mask, tag='exh-Log'))
        # balls: log with M = N, Log with negative v and M such that K = N or K = E
        M = N
        u = rand_principal(rng, p, M, 1)
        cases.append(ball_case('log', p, u, 0, M, N, mask=mask, tag='exh-log-ball'))
        u = rand_principal(rng, p, M, 1)
        cases.append(ball_case('log', p, u, 0, M, N + 3, mask=mask, tag='exh-log-ball-N>M'))
        v = -rng.randrange(1, 4); M = N + v
        u = rand_unit(rng, p, M - v)
        cases.append(ball_case('Log', p, u, v, M, N, mask=mask, tag='exh-Log-ball-negv'))
        v = rng.randrange(1, 4); M = N + v
        u = rand_unit(rng, p, M - v)
        cases.append(ball_case('Log', p, u, v, M, N + 5, mask=mask, tag='exh-Log-ball-posv'))
    return cases

def family_random(p, rng, count, Nlo, Nhi, mask=7, flint=0):
    cases = []
    for i in range(count):
        N = rng.randrange(Nlo, Nhi + 1)
        name = 'log' if i % 2 == 0 else 'Log'
        if rng.random() < 0.4:
            xs = log_centres(p) if name == 'log' else Log_centres(p)
            cases.append(exact_case(name, p, rng.choice(xs), N, mask=mask, flint=flint, tag='rand-exact'))
        else:
            M = N + rng.randrange(-5, 6)
            if M < 3:
                M = 3
            if name == 'log':
                v0 = 1 if (p != 2 or rng.random() < 0.5) else 2
                u = rand_principal(rng, p, M, v0)
                cases.append(ball_case('log', p, u, 0, M, N, mask=mask, flint=flint, tag='rand-log-ball'))
            else:
                v = rng.randrange(-6, 7);
                if M - v < 1:
                    M = v + 1 + rng.randrange(0, 3)
                u = rand_unit(rng, p, M - v)
                cases.append(ball_case('Log', p, u, v, M, N, mask=mask, flint=flint, tag='rand-Log-ball'))
    return cases

def family_large(p, Ns, mask, flint):
    cases = []
    for N in Ns:
        for x in [1 + p, 1 - p, Fraction(1 + p, 1 - p)] + ([3, -5] if p == 2 else []):
            cases.append(exact_case('log', p, x, N, mask=mask, flint=flint, tag=f'large-log-N{N}'))
        for x in [2, Fraction(5, 7), Fraction(2, p ** 3), 1 + p]:
            if x != 2 or p != 2:
                cases.append(exact_case('Log', p, x, N, mask=mask, flint=flint, tag=f'large-Log-N{N}'))
    return cases

def summarize(stats, log, title):
    print(f"== {title}")
    for k in sorted(stats):
        print(f"   {k}: {stats[k] if not isinstance(stats[k], float) else round(stats[k], 3)}")
    for l in log[:50]:
        print("   ", l[:400])
    if len(log) > 50:
        print(f"   ... {len(log)} lines")

from collections import defaultdict

def main():
    mode = sys.argv[2] if len(sys.argv) > 2 else 'all'
    rng = random.Random(20260930)
    log = []
    stats = defaultdict(int)
    if mode in ('selftest',):
        # the oracle against itself: exact vs naive vs direct on small inputs
        bad = 0; n_ = 0
        for p in [2, 3, 5, 7, 11, 65537]:
            for x in log_centres(p):
                for n in [1, 2, 3, 5, 8, 13, 21, 34, 55, 89]:
                    z = x - 1 if not (p == 2 and (x.numerator - x.denominator) % 4) else -x - 1
                    a = series_exact(z, p, n); b = series_naive(z, p, n); c = series_direct(z, p, n)
                    n_ += 1
                    if not (a == b == c):
                        bad += 1; print("selftest mismatch", p, x, n, a, b, c)
        # Hensel root: w^(p-1) = 1 and w = a0 mod p
        for p in [3, 5, 7, 11, 65537, BIG]:
            for a0 in [2, 3, p - 2, 12345 % p]:
                if a0 % p in (0, 1, p - 1):
                    continue
                w = hensel_root(a0, p, 50); P = p ** 50
                n_ += 1
                if pow(w, p - 1, P) != 1 or w % p != a0 % p:
                    bad += 1; print("hensel mismatch", p, a0)
        # Log at p=2 via series(a-1) vs series(-a-1) for a = 3 mod 4 (both must agree: log(-1) = 0)
        for a in [3, 7, Fraction(3, 5), -5, 11]:
            for n in [5, 20, 60]:
                n_ += 1
                if series_exact(Fraction(a) - 1, 2, n) != series_exact(-Fraction(a) - 1, 2, n):
                    bad += 1; print("log(-1) mismatch", a, n)
        print(f"selftest: {n_} checks, {bad} mismatches")
        return
    if mode == 'exhaustive':
        # p = 2, 3: N from 60 to 400, every N, all four routes (old, forced F8, forced F9, new), oracle
        for p in (2, 3):
            cases = with_flags(family_exhaustive(p, 60, 400, rng))
            t0 = time.time()
            res = run_probe(cases)
            stats['probe_time'] += time.time() - t0
            check(cases, res, log, stats)
        summarize(stats, log, 'exhaustive p = 2, 3, N = 60 .. 400')
        return
    if mode == 'small':
        # every prime, N from 1 to 80 exhaustively: the forced routes at word-size moduli, where the library
        # would take the old loop (the boundary handled by all routes)
        for p in PRIMES:
            cases = with_flags(family_exhaustive(p, 1, 80, rng))
            res = run_probe(cases)
            check(cases, res, log, stats)
        summarize(stats, log, 'small: every prime, N = 1 .. 80, all routes forced')
        return
    if mode == 'random':
        for p in PRIMES:
            hi = 400 if p != BIG else 300
            cases = with_flags(family_random(p, rng, 600, 1, hi, flint=1))
            res = run_probe(cases)
            check(cases, res, log, stats)
        summarize(stats, log, 'random: every prime, 600 inputs each, N = 1 .. 400, oracle and FLINT')
        return
    if mode == 'large':
        primes = [int(x) for x in sys.argv[3].split(',')] if len(sys.argv) > 3 else PRIMES
        Ns = [int(x) for x in sys.argv[4].split(',')] if len(sys.argv) > 4 else (1000, 5000, 20000)
        for p in primes:
            for N in Ns:
                mask = 7
                if p == BIG and N >= 5000:
                    mask = 4                                  # old and F8 routes too slow above the word range
                if p == 65537 and N >= 20000:
                    mask = 4
                cases = with_flags(family_large(p, [N], mask, flint=1), point=False)
                t0 = time.time()
                res = run_probe(cases, timeout=900)
                check(cases, res, log, stats)
                print(f"   p={p} N={N}: {len(cases)} cases, probe+oracle {time.time() - t0:.1f} s, "
                      f"max t_new {max(r['t_new'] for r in res):.3f} s, max t_old {max(r['t_old'] for r in res):.3f} s",
                      flush=True)
        summarize(stats, log, 'large: N = 1000, 5000, 20000')
        return
    if mode == 'largeball':
        # random large centres at N = 1000 and 5000: all routes where affordable; oracle = direct (Prop. 8 in
        # Python) or FLINT
        primes = [int(x) for x in sys.argv[3].split(',')] if len(sys.argv) > 3 else PRIMES
        Ns = [int(x) for x in sys.argv[4].split(',')] if len(sys.argv) > 4 else (1000, 5000)
        for p in primes:
            for N in Ns:
                t0 = time.time()
                mask = 7 if not (p == BIG and N > 1000) else 4
                cases = with_flags(family_random(p, rng, 8, N, N, mask=mask, flint=1), point=(N == 1000))
                res = run_probe(cases, timeout=900)
                check(cases, res, log, stats)
                print(f"   p={p} N={N}: {len(cases)} cases, probe+oracle {time.time() - t0:.1f} s, "
                      f"max t_new {max(r['t_new'] for r in res):.3f} s, max t_old {max(r['t_old'] for r in res):.3f} s",
                      flush=True)
        summarize(stats, log, 'largeball: random centres, N = 1000, 5000')
        return
    print("modes: selftest exhaustive small random large largeball")

if __name__ == '__main__':
    main()
