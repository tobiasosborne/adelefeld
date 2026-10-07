#!/usr/bin/env python3
"""Lane q-review3 (Claude Opus): exact model of adf_qclass_reduce, written from docs/api-3.md 2.2
(algorithm R) and section 4 (Q1), and an exact containment decision for the quotient A/Q that does not
use algorithm R. Nothing is imported from proto/ or from the vectors of lane q-slice2.

Representation: a piece is (L, U, c, H): real interval [L, U] (exact Fractions), finite set c + H Zhat
with c rational and H >= 0 rational (H = 0: the single point c).

Membership of the class of (s, z) in pi([L,U] x (c + H Zhat)): there is a rational q with s + q in [L,U]
and z + q in c + H Zhat. For z in a coset z0 + M Zhat with M a multiple of H (or for z = z0 rational),
z + q in c + H Zhat iff q in c - z0 + H Z, because H Zhat intersect Q = H Z (for rational H > 0:
x in H Zhat iff x/H in Zhat intersect Q = Z). This is the rule used by lanes/q-review1/review_checks.py
(bf_member), re-derived here.
"""
from fractions import Fraction as F
from math import floor, ceil, gcd


# ------------------------------------------------------------------ Q1 (docs/api-3.md 4, Q1)

def binade(v):
    """e with 2^(e-1) <= v < 2^e, v > 0 rational."""
    n, d = v.numerator, v.denominator
    e = n.bit_length() - d.bit_length()
    # now 2^(e-1) < n/d < 2^(e+1)
    while F(2) ** (e - 1) > v:
        e -= 1
    while v >= F(2) ** e:
        e += 1
    return e


def round_near_even(v, p):
    if v == 0:
        return F(0)
    sgn = 1 if v > 0 else -1
    a = abs(v)
    e = binade(a)
    s = F(2) ** (e - p)
    q = a / s
    fl = q.numerator // q.denominator
    rem = q - fl
    if 2 * rem < 1:
        r = fl
    elif 2 * rem > 1:
        r = fl + 1
    else:
        r = fl if fl % 2 == 0 else fl + 1
    return sgn * F(r) * s


def q1(l, h, prec):
    """Q1: (m, rho) for exact 0 <= l <= h <= 1."""
    p = max(prec, 2)
    m = round_near_even((l + h) / 2, p)
    d = max(m - l, h - m)
    if d == 0:
        return m, F(0), d
    e = binade(d)
    s = F(2) ** (e - 30)
    k = -((-d) // s)            # ceil(d / s)
    u = k * s                   # least 30-bit number >= d (k <= 2^30)
    if u == F(2) ** e:
        rho = u + F(2) ** (e + 1 - 30)
    else:
        rho = u + s
    return m, rho, d


# ------------------------------------------------------------------ algorithm R (api-3.md 2.2)

def canon_fin(c, H):
    """Canonical global triple (A, H, d) of c + H Zhat (fball.h set_fmpz3 rule)."""
    c, H = F(c), F(H)
    if H == 0:
        return c.numerator, 0, c.denominator
    d = c.denominator * H.denominator // gcd(c.denominator, H.denominator)
    A, Hn = int(c * d), int(H * d)
    g = gcd(gcd(A, Hn), d)
    A, Hn, d = A // g, Hn // g, d // g
    return A % Hn, Hn, d


def R_construct(lo, hi, a, N):
    """Exact constructed pieces (left, right, c, A) of [lo,hi] x (a + N Zhat), with duplicates."""
    if N == 0:
        fib = [(a, 0)]
    else:
        A, B = N.numerator, N.denominator
        fib = [(a + F(j * A, B), A) for j in range(B)]
    out = []
    for aj, A in fib:
        l, h = lo - aj, hi - aj
        if l == h:
            ns = [floor(l)]
        else:
            ns = range(floor(l), ceil(h))
        for n in ns:
            left = max(l, F(n)) - n
            right = min(h, F(n + 1)) - n
            c = (-n) % A if A else -n
            out.append((left, right, c, A))
    return out


def R_count(lo, hi, a, N):
    if N == 0:
        fib = [(a, 0)]
    else:
        A, B = N.numerator, N.denominator
        fib = [(a + F(j * A, B), A) for j in range(B)]
    t = 0
    for aj, A in fib:
        l, h = lo - aj, hi - aj
        t += 1 if l == h else ceil(h) - floor(l)
    return t


def expected_store(pieces_in, prec):
    """pieces_in: list of (lo, hi, a, N). Returns (count, stored list of (m, rho, A, H, 1), exact list)."""
    cons = []
    for lo, hi, a, N in pieces_in:
        cons.extend(R_construct(lo, hi, a, N))
    rounded = []
    for (l, h, c, A) in cons:
        m, rho, d = q1(l, h, prec)
        rounded.append(((m - rho, m + rho, A, c), (m, rho, c, A, 1), (l, h)))
    rounded.sort(key=lambda t: t[0])
    out, exact = [], []
    for k, v, lh in rounded:
        if out and out[-1][0] == k:
            exact[-1].append(lh)
            continue
        out.append((k, v))
        exact.append([lh])
    return len(cons), [v for k, v in out], exact


# ------------------------------------------------------------------ exact containment, without R

def cover(lo, hi, ivs):
    ivs = sorted(i for i in ivs if i[1] >= lo and i[0] <= hi)
    x = lo
    covered_x = False      # is the point x itself covered?
    for a, b in ivs:
        if a > x:
            return False, x
        if b >= x:
            covered_x = True
            x = b
            if x >= hi:
                return True, None
    if covered_x and x >= hi:
        return True, None
    return False, x


def lcm(a, b):
    return a * b // gcd(a, b)


def shifted(out, z0, lo, hi, positive_only):
    """Intervals of real s such that (s, z) is in some stored piece, for every z in z0 + M Zhat
    (M a common multiple of all positive H) or for z = z0 exactly (positive_only False)."""
    ivs = []
    for (L, U, c, H) in out:
        if H == 0:
            if positive_only:
                continue
            q = c - z0
            ivs.append((L - q, U - q))
            continue
        base = c - z0
        # q = base + k H in [L - hi, U - lo]
        k0 = ceil((L - hi - base) / H)
        k1 = floor((U - lo - base) / H)
        for k in range(k0, k1 + 1):
            q = base + k * H
            ivs.append((L - q, U - q))
    return ivs


class Index:
    """Stored pieces with one common integer modulus Hc (Hc = 0: exact points), by residue."""
    def __init__(self, out):
        self.Hc = int(out[0][3])
        self.by = {}
        for (L, U, c, H) in out:
            r = int(c) % self.Hc if self.Hc else int(c)
            self.by.setdefault(r, []).append((L, U))
        self.Lmin = min(o[0] for o in out)
        self.Umax = max(o[1] for o in out)

    def intervals(self, z0, lo, hi):
        """As shifted(): q = m - z0 with m an integer congruent to c mod Hc (or m = c)."""
        ivs = []
        m0 = ceil(self.Lmin - hi + z0)
        m1 = floor(self.Umax - lo + z0)
        for m in range(m0, m1 + 1):
            r = m % self.Hc if self.Hc else m
            lst = self.by.get(r)
            if not lst:
                continue
            q = m - z0
            for (L, U) in lst:
                ivs.append((L - q, U - q))
        return ivs


def _lcm(a, b):
    return a * b // gcd(a, b)


def _cover_int(lo, hi, ivs):
    ivs.sort()
    x = lo
    hit = False
    for a, b in ivs:
        if b < lo or a > hi:
            continue
        if a > x:
            return False
        if b >= x:
            hit = True
            x = b
            if x >= hi:
                return True
    return hit and x >= hi


def _cover_int_method(self, z0, lo, hi):
    """cover(lo, hi, self.intervals(z0, lo, hi)) in integers scaled by a common denominator."""
    if not hasattr(self, 'Dd'):
        Dd = 1
        for lst in self.by.values():
            for (L, U) in lst:
                Dd = _lcm(Dd, _lcm(L.denominator, U.denominator))
        self.Dd = Dd
        self.byi = {r: [(int(L * Dd), int(U * Dd)) for (L, U) in lst] for r, lst in self.by.items()}
    D = _lcm(_lcm(self.Dd, z0.denominator), _lcm(lo.denominator, hi.denominator))
    k = D // self.Dd
    z0D = int(z0 * D)
    loD, hiD = int(lo * D), int(hi * D)
    ivs = []
    m0 = ceil(self.Lmin - hi + z0)
    m1 = floor(self.Umax - lo + z0)
    for m in range(m0, m1 + 1):
        r = m % self.Hc if self.Hc else m
        lst = self.byi.get(r)
        if not lst:
            continue
        qD = m * D - z0D
        for (L, U) in lst:
            ivs.append((L * k - qD, U * k - qD))
    return _cover_int(loD, hiD, ivs)


Index.cover_int = _cover_int_method


def uniform(out):
    H0 = out[0][3]
    return all(o[3] == H0 for o in out) and F(H0).denominator == 1 and \
        all(F(o[2]).denominator == 1 for o in out)


_IX = {}


def contains(out, lo, hi, a, N):
    if _IX.get('uout') is not out:
        _IX['uout'] = out
        _IX['u'] = uniform(out)
    if _IX['u'] and (N == 0) == (out[0][3] == 0) and (N == 0 or N.numerator % int(out[0][3]) == 0):
        if _IX.get('out') is not out:
            _IX['out'] = out
            _IX['ix'] = Index(out)
        ix = _IX['ix']
        A, B = (0, 1) if N == 0 else (N.numerator, N.denominator)
        for j in range(B):
            aj = a + F(j * A, B)
            if not ix.cover_int(aj, lo, hi):
                ok, x = cover(lo, hi, ix.intervals(aj, lo, hi))
                assert not ok
                return False, (x, aj)
        return True, None
    return contains_slow(out, lo, hi, a, N)


def contains_slow(out, lo, hi, a, N):
    """Is pi([lo,hi] x (a + N Zhat)) a subset of union pi(out)? Returns (ok, witness)."""
    Hs = [H for (_, _, _, H) in out if H > 0]
    if N == 0:
        ok, x = cover(lo, hi, shifted(out, a, lo, hi, False))
        return ok, None if ok else (x, a)
    A, B = N.numerator, N.denominator
    M = A
    for H in Hs:
        Hf = F(H)
        if Hf.denominator != 1:
            raise ValueError('non-integer stored radius')
        M = lcm(M, int(Hf))
    for j in range(B):
        aj = a + F(j * A, B)
        for t in range(M // A):
            z0 = aj + t * A
            ok, x = cover(lo, hi, shifted(out, z0, lo, hi, True))
            if not ok:
                return False, (x, z0)
    return True, None


def point_in(out, s, w):
    """Is the class of (s, w), w rational, in union pi(out)?"""
    for (L, U, c, H) in out:
        if H == 0:
            q = c - w
            if L <= s + q <= U:
                return True
            continue
        base = c - w
        k0 = ceil((L - s - base) / H)
        if base + k0 * H <= U - s:
            return True
    return False
