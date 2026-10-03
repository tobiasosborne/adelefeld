"""f-review7 own oracle: exact integers and fractions only (no logarithm, no exponential, no C code).

Facts used (proved in result.md, section "Oracle"):
 D1 (dominance) unit y, t in Z_p, e >= c, k >= 1, s = v_p(k): v((y + p^e t)^k - y^k) >= s + e, equal for a unit t.
 D2 (root) for a unit U and a unit residue t (mod p^c) with t^n = U mod p^(s+c) there is a unique beta mod p^L with
    beta = t mod p^c and beta^n = U mod p^(L+s); no root on the branch t exists otherwise. The function nroot
    returns beta and VERIFIES beta^n = U mod p^(L+s) (a certificate, whatever the method).
 D3 (principal powers) for u in 1 + p^c Z_p (at 2: u = w u', u' in 1 + 4 Z_2) and integers k >= 1:
    u^(p^k) in 1 + p^(k+c) Z_p. So for s, s' in Z_p with s = s' mod p^k, k = max(H - c, 1): u^s = u^s' mod p^H,
    and u^s mod p^H depends on u mod p^H only. Integer s >= 0: u^s is the integer power.
"""
from fractions import Fraction as F
import random, subprocess, os, math, sys
sys.set_int_max_str_digits(0)

HERE = os.path.dirname(os.path.abspath(__file__))
INF = 10**30

def vp(x, p):
    if x == 0: return INF
    k = 0
    while x % p == 0: x //= p; k += 1
    return k

def vq(q, p):
    q = F(q)
    if q == 0: return INF
    return vp(q.numerator, p) - vp(q.denominator, p)

def cfor(p): return 2 if p == 2 else 1

def unit_part(q, p):
    q = F(q); m = vq(q, p)
    return m, q / F(p) ** m

def mod_unit(U, p, k):
    """the p-adic integer U (Fraction with denominator prime to p) modulo p^k"""
    U = F(U); P = p ** k
    return U.numerator % P * pow(U.denominator, -1, P) % P

def iroot(a, n):
    """integer n-th root of a >= 0 or None (integer Newton, exact)"""
    if a < 0: return None
    if a < 2: return a
    if n >= a.bit_length(): return None
    x = 1 << (a.bit_length() // n + 1)
    while True:
        y = ((n - 1) * x + a // x ** (n - 1)) // n
        if y >= x: break
        x = y
    for c in (x - 1, x, x + 1):
        if c >= 0 and c ** n == a: return c
    return None

def branch_exists(p, n, U, t):
    s = vp(n, p); c = cfor(p); K = s + c
    return (pow(t, n, p ** K) - mod_unit(U, p, K)) % p ** K == 0

def nroot(p, n, U, t, L):
    """D2: beta mod p^L, beta = t mod p^c, beta^n = U mod p^(L+s); None if the branch does not exist."""
    s = vp(n, p); c = cfor(p); n0 = n // p ** s
    if not branch_exists(p, n, U, t): return None
    L = max(L, c)
    W = L + 2 * s + 4
    P = p ** W
    Um = mod_unit(U, p, W)
    y = t % p ** c
    # digit lifting up to precision 3 (to start Newton), then Newton
    e = c
    for it in range(400):
        f = (pow(y, n, P) - Um) % P
        if f % p ** (L + s) == 0: break
        q = f // p ** s
        dv = n0 * pow(y, n - 1, P) % P
        y = (y - q * pow(dv, -1, P)) % P
    else:
        raise AssertionError("no convergence")
    beta = y % p ** L
    assert (pow(beta, n, p ** (L + s)) - mod_unit(U, p, L + s)) % p ** (L + s) == 0
    assert (beta - t) % p ** c == 0
    return beta

def in_ball(p, val, prec, res):
    """val = p^a w (a, w mod p^prec rel) known; res = (exact, v, N, Fraction u). Is val in the ball res?
       val is given as (a, w, h): the point is p^a w + O(p^(a+h)) (w integer, h relative digits known)."""
    a, w, h = val
    ex, v, N, u = res
    assert not ex
    if u == 0: return a >= N
    # centre p^v u, u integer
    if a >= N and v >= N: return True
    m0 = min(a, v, N)
    if a < N: assert a + h >= N, "not enough digits"
    P = p ** (N - m0)
    x1 = (p ** (a - m0) * (w % p ** max(N - a, 0) if a < N else 0)) if a < N else 0
    x2 = (p ** (v - m0) * int(u)) if v < N else 0
    return (x1 - x2) % P == 0

class Harness:
    def __init__(self, exe=None):
        self.exe = exe or os.path.join(HERE, 'h')
    def run(self, lines, timeout=110):
        out = subprocess.run(['timeout', str(timeout), self.exe], input='\n'.join(lines) + '\n',
                             capture_output=True, text=True)
        res = out.stdout.strip('\n').split('\n') if out.stdout.strip() else []
        self.stderr = out.stderr
        if out.stderr.strip(): print('HARNESS STDERR:', out.stderr[:3000])
        if len(res) != len(lines):
            raise RuntimeError('harness: %d lines for %d; rc %d; %s' % (len(res), len(lines), out.returncode,
                                                                       out.stderr[-2000:]))
        return res

def parse(line):
    t = line.split()
    st = t[0]
    if st != 'OK':
        return st, None, line
    ex, v, N = int(t[1]), int(t[2]), int(t[3])
    u = F(t[4])
    return st, (ex, v, N, u), line

def fields_exact(p, q):
    q = F(q)
    if q == 0: return '0 1 0 0 1'
    m, U = unit_part(q, p)
    return '%d %d %d 0 1' % (U.numerator, U.denominator, m)

def fields_ball(p, centre, N):
    """the canonical ball centre + p^N Z_p, centre a rational in Z[1/p] or with denominators prime to p"""
    c = F(centre)
    if c == 0 or vq(c, p) >= N: return '0 1 0 %d 0' % N
    m, U = unit_part(c, p)
    u = mod_unit(U, p, N - m)
    return '%d 1 %d %d 0' % (u, m, N)
