#!/usr/bin/env python3
"""oracle.py: an independent oracle for the driver adf, written by the reviewer `surface`.

Exact arithmetic only (fractions.Fraction).  Nothing of the code under review, and nothing of
tests/ref, is imported.  Sources of every rule:

  finite ball  a + N Zhat, N >= 0 rational          SPEC 4.1 (docs/SPEC.md:152, 173)
  tight sum    (a+b) + gcd(N, M) Zhat                 SPEC 4.3 (docs/SPEC.md:234)
  tight prod   ab + gcd(aM, bN, NM) Zhat              SPEC 4.3 (docs/SPEC.md:235)
  predicates                                          SPEC 4.2 (docs/SPEC.md:219-225)
  cap          radius -> gcd(R, C), exact untouched   SPEC 4.4 item 3 (docs/SPEC.md:277-281)
  printing     q(x), (* ; F), (r ; F), (z ; F)        conventions 9.4 (docs/conventions.md:1180-1206)
  real printer                                        conventions 9.5 (docs/conventions.md:1216-1233)

gcd of rationals: the non-negative generator of the group they generate (docs/SPEC.md:232).
"""
from fractions import Fraction as F
from math import gcd as igcd, floor, ceil
import re
import sys as _sys
_sys.set_int_max_str_digits(0)


def rgcd(*xs):
    """gcd of rationals: the generator of the group; 0 if all are 0 (SPEC 4.3, line 232)."""
    xs = [abs(F(x)) for x in xs if x != 0]
    if not xs:
        return F(0)
    den = 1
    for x in xs:
        den = den * x.denominator // igcd(den, x.denominator)
    g = 0
    for x in xs:
        g = igcd(g, int(x * den))
    return F(g, den)


def is_int(x):
    return F(x).denominator == 1


# ---------------------------------------------------------------- finite balls

class FB:
    """the set a + N Zhat; N = 0 is the point a"""

    def __init__(self, a, N=0):
        self.a = F(a)
        self.N = F(N)
        assert self.N >= 0
        if self.N > 0:
            k = floor(self.a / self.N)
            self.a = self.a - k * self.N          # centre in [0, N)  (conventions 9.4)

    def text_fin(self):
        if self.N == 0:
            return q(self.a)
        return q(self.a) + " mod " + q(self.N)

    def __add__(self, o):
        return FB(self.a + o.a, rgcd(self.N, o.N))

    def __neg__(self):
        return FB(-self.a, self.N)

    def __sub__(self, o):
        return self + (-o)

    def __mul__(self, o):
        return FB(self.a * o.a, rgcd(self.a * o.N, o.a * self.N, self.N * o.N))

    def mul_rat(self, r):
        return FB(self.a * r, abs(r) * self.N)


def pred_equal(x, y):
    if x.N == 0 or y.N == 0:
        return x.N == y.N and x.a == y.a
    return x.N == y.N and is_int((x.a - y.a) / x.N)


def pred_overlaps(x, y):
    g = rgcd(x.N, y.N)
    if g == 0:
        return x.a == y.a
    return is_int((x.a - y.a) / g)


def pred_contains(x, y):
    """first inside second (SPEC 4.2, line 223)"""
    if y.N == 0:
        return x.N == 0 and x.a == y.a
    return is_int(x.N / y.N) and is_int((x.a - y.a) / y.N)


def cap(x, C):
    if C <= 0:
        return None            # DOMAIN (include/adelefeld/scaled.h:160-167)
    if x.N == 0:
        return FB(x.a, 0)
    return FB(x.a, rgcd(x.N, C))


# ---------------------------------------------------------------- printing

def q(x):
    x = F(x)
    if x.denominator == 1:
        return str(x.numerator)
    return "%d/%d" % (x.numerator, x.denominator)


def X(y):
    """floor(log10 |y|), exact"""
    y = abs(F(y))
    assert y != 0
    e = len(str(y.numerator)) - len(str(y.denominator))
    while F(10) ** e > y:
        e -= 1
    while F(10) ** (e + 1) <= y:
        e += 1
    return e


def ceil2(E):
    s = F(10) ** (X(E) - 1)
    return ceil(E / s) * s


def roundq(y, qq):
    s = F(10) ** qq
    t = y / s
    fl = floor(t)
    d = t - fl
    if d > F(1, 2) or (d == F(1, 2) and fl % 2 == 1):
        fl += 1
    return fl * s


def is_decimal(y):
    d = F(y).denominator
    while d % 2 == 0:
        d //= 2
    while d % 5 == 0:
        d //= 5
    return d == 1


def sigdigits(y):
    y = abs(F(y))
    k = 0
    while (y * F(10) ** k).denominator != 1:
        k += 1
    D = int(y * F(10) ** k)
    while D % 10 == 0:
        D //= 10
    return len(str(D))


def fmt(y):
    y = F(y)
    if y == 0:
        return "0"
    sgn = "-" if y < 0 else ""
    y = abs(y)
    E = 0
    while (y / F(10) ** E).denominator != 1:
        E -= 1
    D = int(y / F(10) ** E)
    while D % 10 == 0:
        D //= 10
        E += 1
    k = len(str(D))
    Xe = E + k - 1
    if -4 <= Xe <= 20:
        if E >= 0:
            return sgn + str(D) + "0" * E
        s = str(D).rjust(-E + 1, "0")
        return sgn + s[:E] + "." + s[E:]
    ds = str(D)
    mant = ds[0] + ("." + ds[1:] if k > 1 else "")
    return sgn + mant + "e" + str(Xe)


def r_print(mid, rad, n=20):
    """conventions 9.5, 'Printing', steps 1-6"""
    mid, rad = F(mid), F(rad)
    if rad == 0 and (mid == 0 or (is_decimal(mid) and sigdigits(mid) <= n)):
        return fmt(mid)
    if mid == 0:
        return "0 +/- " + fmt(ceil2(rad))
    qq = X(mid) - n + 1
    if rad > 0:
        qq = max(qq, X(rad) - 1)
    while True:
        M = roundq(mid, qq)
        R = ceil2(rad + abs(M - mid))
        if M == 0:
            q2 = X(R) - 1
        else:
            q2 = max(X(M) - n + 1, X(R) - 1)
        if not is_int(M / F(10) ** q2):
            qq = q2
            continue
        return fmt(M) + " +/- " + fmt(R)


def parse_real_text(t):
    """'M +/- R' or 'M' -> (M, R) exact"""
    t = t.strip()
    if "+/-" in t:
        m, r = t.split("+/-")
        return F(m.strip()), F(r.strip())
    return F(t), F(0)


# ---------------------------------------------------------------- values

class Real:
    def __init__(self, lo, hi):
        self.lo, self.hi = F(lo), F(hi)

    @staticmethod
    def pt(x):
        return Real(x, x)

    def __add__(s, o):
        return Real(s.lo + o.lo, s.hi + o.hi)

    def __neg__(s):
        return Real(-s.hi, -s.lo)

    def __sub__(s, o):
        return s + (-o)

    def __mul__(s, o):
        c = [s.lo * o.lo, s.lo * o.hi, s.hi * o.lo, s.hi * o.hi]
        return Real(min(c), max(c))

    def exact(s):
        return s.lo == s.hi


DEC = r"-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?"
UDEC = r"\d+(?:\.\d+)?(?:[eE][+-]?\d+)?"
REAL = r"(%s)\s*(?:\+/-\s*(%s))?" % (DEC, UDEC)
RAT = r"-?\d+(?:/\d+)?"
URAT = r"\d+(?:/\d+)?"
FIN = r"(%s)\s*(?:mod\s*(%s))?" % (RAT, URAT)


def realv(m, r):
    m = F(m)
    r = F(r) if r else F(0)
    return Real(m - r, m + r)


def parse(t):
    """-> ('rat', F) | ('fball', FB) | ('adele', Real, FB) | ('cadele', Real, Real, FB) | None"""
    t = t.strip()
    if re.search(r"[eE][+-]?\d{4,}", t):
        return None             # exponents above 999 are not modelled (cost)
    m = re.fullmatch(RAT, t)
    if m:
        return ("rat", F(t))
    m = re.fullmatch(r"\(\s*\*\s*;\s*%s\s*\)" % FIN, t) or re.fullmatch(FIN.replace("?:mod", "mod"), t)
    if m:
        return ("fball", FB(F(m.group(1)), F(m.group(2)) if m.group(2) else 0))
    m = re.fullmatch(r"\(\s*%s\s*;\s*%s\s*\)" % (REAL, FIN), t)
    if m:
        return ("adele", realv(m.group(1), m.group(2)), FB(F(m.group(3)), F(m.group(4)) if m.group(4) else 0))
    m = re.fullmatch(r"\(\s*\(\s*%s\s*\)\s*\+\s*\(\s*%s\s*\)\s*\*\s*i\s*;\s*%s\s*\)" % (REAL, REAL, FIN), t)
    if m:
        return ("cadele", realv(m.group(1), m.group(2)), realv(m.group(3), m.group(4)),
                FB(F(m.group(5)), F(m.group(6)) if m.group(6) else 0))
    return None


def rtext(x):
    """the text of an exact real (the expectation when the arb is exact)"""
    return r_print((x.lo + x.hi) / 2, (x.hi - x.lo) / 2)


def show(v):
    k = v[0]
    if k == "rat":
        return q(v[1])
    if k == "fball":
        return "(* ; " + v[1].text_fin() + ")"
    if k == "adele":
        return "(" + rtext(v[1]) + " ; " + v[2].text_fin() + ")"
    if k == "cadele":
        return "((" + rtext(v[1]) + ") + (" + rtext(v[2]) + ")*i ; " + v[3].text_fin() + ")"


def to_cadele(v):
    if v[0] == "adele":
        return ("cadele", v[1], Real.pt(0), v[2])
    return v


def binop(op, x, y):
    kx, ky = x[0], y[0]
    if kx == "rat" and ky == "rat":
        return ("rat", {"add": x[1] + y[1], "sub": x[1] - y[1], "mul": x[1] * y[1]}[op])
    if kx == "rat" or ky == "rat":
        if kx == "rat":
            r, v, left = x[1], y, True
        else:
            r, v, left = y[1], x, False
        if op == "mul":
            if v[0] == "fball":
                return ("fball", v[1].mul_rat(r))
            if v[0] == "adele":
                return ("adele", v[1] * Real.pt(r), v[2].mul_rat(r))
            return ("cadele", v[1] * Real.pt(r), v[2] * Real.pt(r), v[3].mul_rat(r))
        s = -1 if (op == "sub" and not left) else 1
        rr = r * s
        if op == "sub" and left:
            v = neg(v)
        if v[0] == "fball":
            return ("fball", v[1] + FB(rr))
        if v[0] == "adele":
            return ("adele", v[1] + Real.pt(rr), v[2] + FB(rr))
        return ("cadele", v[1] + Real.pt(rr), v[2], v[3] + FB(rr))
    if kx == ky == "fball":
        return ("fball", {"add": x[1] + y[1], "sub": x[1] - y[1], "mul": x[1] * y[1]}[op])
    if kx == ky == "adele":
        f = {"add": lambda a, b: a + b, "sub": lambda a, b: a - b, "mul": lambda a, b: a * b}[op]
        return ("adele", f(x[1], y[1]), f(x[2], y[2]))
    if {kx, ky} <= {"adele", "cadele"}:
        x, y = to_cadele(x), to_cadele(y)
        if op == "add":
            return ("cadele", x[1] + y[1], x[2] + y[2], x[3] + y[3])
        if op == "sub":
            return ("cadele", x[1] - y[1], x[2] - y[2], x[3] - y[3])
        return ("cadele", x[1] * y[1] - x[2] * y[2], x[1] * y[2] + x[2] * y[1], x[3] * y[3])
    return "error: DOMAIN"


def neg(v):
    k = v[0]
    if k == "rat":
        return ("rat", -v[1])
    if k == "fball":
        return ("fball", -v[1])
    if k == "adele":
        return ("adele", -v[1], -v[2])
    return ("cadele", -v[1], -v[2], -v[3])


def div(x, y):
    if y[0] != "rat":
        return "error: DOMAIN"
    if y[1] == 0:
        return "error: NOT_UNIT"
    r = 1 / y[1]
    if x[0] == "rat":
        return ("rat", x[1] * r)
    return binop("mul", x, ("rat", r))


def ball_like(v):
    if v[0] == "rat":
        return FB(v[1])
    if v[0] == "fball":
        return v[1]
    return None


def run(line):
    """-> expected text, or None when the oracle does not model the line"""
    w = line.strip().split(None, 1)
    if len(w) < 2:
        return None
    op, rest = w
    ops = rest.split(" with ")
    vals = [parse(o) for o in ops]
    if any(v is None for v in vals):
        return None
    if op == "show" and len(vals) == 1:
        v = vals[0]
        return show(v)
    if op == "type" and len(vals) == 1:
        return vals[0][0]
    if op == "neg" and len(vals) == 1:
        return show(neg(vals[0]))
    if len(vals) != 2:
        return None
    x, y = vals
    if op in ("add", "sub", "mul"):
        r = binop(op, x, y)
        return r if isinstance(r, str) else show(r)
    if op == "div":
        r = div(x, y)
        return r if isinstance(r, str) else show(r)
    if op in ("equal", "contains", "overlaps"):
        a, b = ball_like(x), ball_like(y)
        if a is None or b is None:
            return "error: DOMAIN"
        f = {"equal": pred_equal, "contains": pred_contains, "overlaps": pred_overlaps}[op]
        return "true" if f(a, b) else "false"
    if op == "cap":
        if y[0] != "rat":
            return "error: DOMAIN"
        if x[0] in ("adele", "cadele"):
            return "error: UNSUPPORTED"
        if x[0] != "fball":
            return "error: DOMAIN"
        r = cap(x[1], y[1])
        return "error: DOMAIN" if r is None else show(("fball", r))
    return None
