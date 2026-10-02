"""f-review6 own oracle for local n-th roots (no logarithms, no Teichmueller lifting, no C code).

Facts used, each proved in result.md section "Oracle":
 (O1) Dominance: for a unit y, w a unit, e >= c (c=2 at 2, else 1), s=v_p(n):
      v((y + p^e w)^n - y^n) = s + e exactly (binomial terms k>=2 have strictly larger valuation).
 (O2) Lifting: if U is a unit, t a unit with v(t^n - U) >= s + c, the digit-by-digit linear solve
      gives y = t mod p^c with v(y^n - U) >= s + L for every L; conversely a root rho = t mod p^c
      forces v(t^n - U) >= s + c.  Hence a branch t exists iff v(t^n - U) >= s + c.
 (O3) Exact image: if b = p^j u, u in branch t, (p^j u)^n in x = p^m U + p^M Z_p, and r - s >= c,
      the branch image is exactly b + p^E Z_p, E = M - s - (n-1) j (O1 plus a counting argument).
"""
import math, random

def vp(x, p):
    if x == 0: return 10**9
    k = 0
    while x % p == 0: x //= p; k += 1
    return k

def vq(num, den, p):
    return vp(num, p) - vp(den, p)

def cfor(p): return 2 if p == 2 else 1

def unit_mod(num, den, p, k):
    """unit num/den (p-free) modulo p^k"""
    P = p**k
    return (num % P) * pow(den, -1, P) % P

def branch_ok(p, n, Un, Ud, t):
    """O2: branch t (residue mod p, or 1/3 mod 4 at 2) of the unit U = Un/Ud exists."""
    s = vp(n, p); c = cfor(p)
    K = s + c
    Um = unit_mod(Un, Ud, p, K)
    return (pow(t, n, p**K) - Um) % p**K == 0

def lift(p, n, Un, Ud, t, L):
    """O2: the root of U in branch t modulo p^L (L >= c), None if branch does not exist."""
    s = vp(n, p); c = cfor(p)
    if not branch_ok(p, n, Un, Ud, t): return None
    y = t % p**c
    e = c
    while e < L:
        P = p**(s + e + 1)
        Um = unit_mod(Un, Ud, p, s + e + 1)
        f = (pow(y, n, P) - Um) % P
        assert f % p**(s + e) == 0, "lift invariant"
        q = f // p**(s + e)
        nu = n // p**s
        dv = (nu % p) * pow(y, n - 1, p) % p
        d = (-q) * pow(dv, -1, p) % p
        y = y + d * p**e
        e += 1
    return y % p**L

def idents(p, n, Un, Ud):
    """all valid identifiers of a unit (small p only)"""
    if p == 2: return [t for t in (1, 3) if branch_ok(2, n, Un, Ud, t)]
    return [t for t in range(1, p) if branch_ok(p, n, Un, Ud, t)]

def iroot(a, n):
    if a < 0: return None
    if a in (0, 1): return a
    if n >= a.bit_length(): return None
    lo, hi = 1, 1 << (a.bit_length() // n + 1)
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if mid**n <= a: lo = mid
        else: hi = mid - 1
    return lo if lo**n == a else None

def rational_root(p, n, A, B, t):
    """the rational root of the unit A/B in branch t, as (num, den), or None"""
    if A < 0 and n % 2 == 0: return None
    if n > 4096: 
        if abs(A) != 1 or B != 1: return None
        cands = [(1,1)] if A > 0 else [(-1,1)]
        if A > 0 and n % 2 == 0: cands.append((-1,1))
    else:
        ra = iroot(abs(A), n); rb = iroot(B, n)
        if ra is None or rb is None: return None
        q = ra if A > 0 else -ra
        cands = [(q, rb)] + ([(-q, rb)] if n % 2 == 0 else [])
    for (a, b) in cands:
        if unit_mod(a, b, p, cfor(p)) % (4 if p == 2 else p) == t: return (a, b)
    return None

# ---- input representation: dict(kind='ball'|'exact'|'zball'|'zexact', p, m, Un, Ud, M) ----
def fields(x):
    """the raw lball fields for the harness: unum uden v N exact"""
    if x['kind'] == 'zexact': return (0, 1, 0, 0, 1)
    if x['kind'] == 'zball': return (0, 1, 0, x['M'], 0)
    if x['kind'] == 'exact': return (x['Un'], x['Ud'], x['m'], 0, 1)
    return (x['Un'], 1, x['m'], x['M'], 0)

def line(mode, x, n, seed, N, cap=0):
    f = fields(x)
    return "%s %d %d %d %d %d %d %d %d %d %d" % (mode, x['p'], f[0], f[1], f[2], f[3], f[4], n, seed, N, cap)

def parse_ball(tok):
    """tokens: exact v N u -> (exact, v, N, num, den)"""
    ex, v, N, u = int(tok[0]), int(tok[1]), int(tok[2]), tok[3]
    if '/' in u: a, b = u.split('/'); a, b = int(a), int(b)
    else: a, b = int(u), 1
    return (ex, v, N, a, b)

def expect(x, n, seed):
    """expected status for a seeded request (ignoring LIMIT), and the validity of seed"""
    p = x['p']; c = cfor(p)
    if n == 0: return 'DOMAIN'
    if n == 1: return 'OK'
    if x['kind'] == 'zexact': return 'OK' if seed == 0 else 'DOMAIN'
    if x['kind'] == 'zball': return 'NOT_DETERMINED'
    s = vp(n, p)
    if x['kind'] == 'ball' and x['M'] - x['m'] < c + s: return 'NOT_DETERMINED'
    if x['m'] % n: return 'DOMAIN'
    if seed is None: return None
    if p == 2:
        if seed not in (1, 3): return 'DOMAIN'
    elif not (1 <= seed < p): return 'DOMAIN'
    return 'OK' if branch_ok(p, n, x['Un'], x['Ud'], seed) else 'DOMAIN'

def check_value(x, n, seed, Nreq, res):
    """res = (exact, v, N, num, den) of an OK result; return error text or None"""
    p = x['p']; c = cfor(p); ex, v, N, a, b = res
    if n == 1:
        return None if fields(x) == (a, b, v, N, ex) else "n=1 not identity"
    if x['kind'] == 'zexact':
        return None if (ex, v, N, a) == (1, 0, 0, 0) else "zero root wrong"
    s = vp(n, p); j = x['m'] // n
    tmod = 4 if p == 2 else p
    if x['kind'] == 'ball':
        r = x['M'] - x['m']; E = x['M'] - s - (n - 1) * j
        if ex != 0: return "ball input gave exact"
        if N != E: return "exponent %d, exact image %d" % (N, E)
        if v != j: return "valuation %d, expected %d" % (v, j)
        if b != 1 or not (0 < a < p**(N - v)) or a % p == 0: return "noncanonical"
        if a % tmod != seed % tmod: return "branch residue %d != seed %d" % (a % tmod, seed)
        if (pow(a, n, p**r) - x['Un']) % p**r: return "centre^n not in input ball"
        return None
    # exact nonzero input
    rr = rational_root(p, n, x['Un'], x['Ud'], seed)
    if rr is not None:
        if ex != 1: return "rational root %r/%r missed (ball returned)" % rr
        if (a, b) != rr or v != j: return "wrong rational root"
        return None
    if ex == 1:
        # claimed rational: verify
        if a**n * x['Ud'] != x['Un'] * b**n or v != j: return "exact result is not a root"
        return "exact result but oracle found no rational root"
    if N != Nreq: return "exact input: exponent %d != N %d" % (N, Nreq)
    if Nreq <= j:
        return None if (a, v) == (0, 0) else "N<=j: expected zero ball"
    L = Nreq - j
    rho = lift(p, n, x['Un'], x['Ud'], seed, max(L, c))
    if rho is None: return "oracle: no branch"
    if v != j or b != 1: return "valuation/denominator"
    if a != rho % p**L: return "centre %d != root mod p^%d = %d" % (a, L, rho % p**L)
    return None
